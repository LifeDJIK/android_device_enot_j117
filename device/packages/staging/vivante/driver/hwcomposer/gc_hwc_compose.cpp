/****************************************************************************
*
*    Copyright (c) 2005 - 2012 by Vivante Corp.  All rights reserved.
*
*    The material in this file is confidential and contains trade secrets
*    of Vivante Corporation. This is proprietary information owned by
*    Vivante Corporation. No part of this work may be disclosed,
*    reproduced, copied, transmitted, or used in any way for any purpose,
*    without the express written permission of Vivante Corporation.
*
*****************************************************************************/




#include "gc_hwc.h"
#include "gc_hwc_debug.h"

#include <gc_hal_engine.h>
#include <gc_gralloc_priv.h>


/* Setup framebuffer source(for swap rectangle). */
static gceSTATUS
_CopySwapRect(
    IN hwcContext * Context
    );

/* Setup worm hole source. */
static gceSTATUS
_WormHole(
    IN hwcContext * Context,
    IN gcsRECT * Rect
    );

/* Setup blit source. */
static gceSTATUS
_Blit(
    IN hwcContext * Context,
    IN gctUINT32 Index,
    IN gctUINT32 AlphaDim,
    IN gcsRECT * ClipRect
    );

/* Setup multi-source blit source. */
static gceSTATUS
_MultiSourceBlit(
    IN hwcContext * Context,
    IN gctUINT32 Indices[8],
    IN gctUINT32 AlphaDim[8],
    IN gctUINT32 Count,
    IN gcsRECT * EigenRect,
    IN gcsRECT * ClipRect
    );


/*******************************************************************************
**
**  hwcCompose
**
**  Do actual composition in hwcomposer.
**
**  1. Do swap rectangle optimization if enabled.
**  2. Clear overlay area.
**  3. Fill worm holes with opaque black.
**  4. Compose all layers which can be composed by hwcomposer: dim layer,
**     'clear hole' layer, normal layer and 'overlay clear'.
**
**  INPUT:
**
**      hwcContext * Context
**          hwcomposer context pointer.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
hwcCompose(
    IN hwcContext * Context
    )
{
    gceSTATUS status;
    hwcArea * area;
    /* Hold a mask for layers are composed. */
    gctUINT32 composed = 0U;

    hwcFramebuffer * framebuffer = Context->framebuffer;
    hwcBuffer * target           = framebuffer->target;

#if DUMP_COMPOSE

    ALOGD("COMPOSE %d layers", Context->layerCount);
#endif

#if ENABLE_SWAP_RECTANGLE

    /***************************************************************************
    ** Swap Rectangle Optimization.
    */

    /* Copy swap areas from front buffer to current back buffer.
     * This process is independent because the layer composition will never
     * touch swap areas.
     * So it is safe to put this process before everything. */
    if (Context->swapArea != NULL)
    {
        /* Do swap rectangle optimization. */
        gcmONERROR(
            _CopySwapRect(Context));
    }

#else

    /* Just to avoid warning. */
    (void) _CopySwapRect;
#endif

    /* Setup clipping to valid rectangle. */
    gcmONERROR(
        gco2D_SetClipping(Context->engine,
                          &target->swapRect));

#if CLEAR_FB_FOR_OVERLAY

    /***************************************************************************
    ** Clear framebuffer for overlay.
    */

    /* Clear framebuffer area to (0,0,0,0) for overlay .
     * See HWC_HINT_CLEAR_FB process in 'setupHardwareComposer' function in
     * SurfaceFlinger.cpp. 'setupHardwareCompose' is beofre 'composeSurfaces',
     * which means, this operation should be before composing surfaces. */
    if (Context->hasOverlay)
    {
        for (gctUINT i = 0; i < Context->layerCount; i++)
        {
            hwcLayer * layer;
            gctUINT32 owner;

            if (Context->layers[i].compositionType != HWC_OVERLAY)
            {
                /* Not a overlay layer. */
                continue;
            }

            /* Get shortcuts. */
            layer = &Context->layers[i];
            owner = 1U << i;

            /* Save this overlay layer to composed mask. */
            composed |= owner;

            /* Go through all areas.. */
            area = Context->compositionArea;

            while (area != NULL)
            {
                if (owner & area->owners)
                {
                    area = area->next;
                    continue;
                }

#if ENABLE_SWAP_RECTANGLE
                gcsRECT clipRect;

                /* Intersect with swap rectangle. */
                clipRect.left   = gcmMAX(target->swapRect.left,   area->rect.left);
                clipRect.top    = gcmMAX(target->swapRect.top,    area->rect.top);
                clipRect.right  = gcmMIN(target->swapRect.right,  area->rect.right);
                clipRect.bottom = gcmMIN(target->swapRect.bottom, area->rect.bottom);

                if ((clipRect.left >= clipRect.right)
                ||  (clipRect.top  >= clipRect.bottom)
                )
                {
                    area = area->next;
                    continue;
                }

                gcmONERROR(_Blit(Context, i, 0xFF, &clipRect));
#   else
                gcmONERROR(_Blit(Context, i, 0xFF, &area->rect));
#   endif
                /* Advance to next area. */
                area = area->next;
            }
        }
    }
#endif

    /***************************************************************************
    ** Draw Worm Hole.
    */

    /* Clear worm holes before composing layers.
     * See 'drawWormhole' part in 'composeSurfaces' in SurfaceFlinger.cpp.
     * For 3D path, 'drawWormhole' is beofre draw layers in 'composeSurfaces',
     * which means worm hole clearing should be before draw layers.
     */
    area = Context->compositionArea;

    while (area != NULL)
    {
        gctUINT i;

        /* Find bottom layer. */
        for (i = 0; i < Context->layerCount; i++)
        {
            /* Do not need to skip composed layers. If the fist layer is
             * composed OVERLAY layer, this area is already cleared. */
            if ((1U << i) & area->owners)
            {
                break;
            }
        }

        /* Worm holes
         * type 1: area not covered by any layer.
         * type 2: (rare case) bottom layer has alpha blending. */
        if ((i == Context->layerCount)
        || (Context->layers[i].opaque == gcvFALSE)
        )
        {
#if ENABLE_SWAP_RECTANGLE
            gcsRECT clipRect;

            /* Intersect with swap rectangle. */
            clipRect.left   = gcmMAX(target->swapRect.left,   area->rect.left);
            clipRect.top    = gcmMAX(target->swapRect.top,    area->rect.top);
            clipRect.right  = gcmMIN(target->swapRect.right,  area->rect.right);
            clipRect.bottom = gcmMIN(target->swapRect.bottom, area->rect.bottom);

            if ((clipRect.left >= clipRect.right)
            ||  (clipRect.top  >= clipRect.bottom)
            )
            {
                area = area->next;
                continue;
            }

            gcmONERROR(_WormHole(Context, &clipRect));
#else
            gcmONERROR(_WormHole(Context, &area->rect));
#endif
        }

        /* Advance to next area. */
        area = area->next;
    }

#if ENABLE_CLEAR_HOLE

    /***************************************************************************
    ** Draw CLEAR_HOLE.
    */

    /* For CLEAR_HOLE type, which must be on bottom of each area, needs to be
     * cleared by area. It is safe to make it done here because clear holes
     * will never cover others. */
    if (Context->hasClearHole)
    {
        for (gctUINT i = 0; i < Context->layerCount; i++)
        {
            gctUINT32 owner;

            if (Context->layers[i].compositionType != HWC_CLEAR_HOLE)
            {
                /* Not clear hole. */
                continue;
            }

            /* Compute owner. */
            owner = (1U << i);

            /* Save this clear hole layer to mask. */
            composed |= owner;

            /* Go through all areas. */
            area = Context->compositionArea;

            while (area != NULL)
            {
                if (owner & area->owners)
                {
                    area = area->next;
                    continue;
                }

#if ENABLE_SWAP_RECTANGLE
                gcsRECT clipRect;

                /* Intersect with swap rectangle. */
                clipRect.left   = gcmMAX(target->swapRect.left,   area->rect.left);
                clipRect.top    = gcmMAX(target->swapRect.top,    area->rect.top);
                clipRect.right  = gcmMIN(target->swapRect.right,  area->rect.right);
                clipRect.bottom = gcmMIN(target->swapRect.bottom, area->rect.bottom);

                if ((clipRect.left >= clipRect.right)
                ||  (clipRect.top  >= clipRect.bottom)
                )
                {
                    area = area->next;
                    continue;
                }

                gcmONERROR(_Blit(Context, i, 0xFF, &clipRect));
#   else
                gcmONERROR(_Blit(Context, i, 0xFF, &area->rect));
#   endif
                /* Advance to next area. */
                area = area->next;
            }
        }
    }
#endif

    /***************************************************************************
    ** Compose layers (BLITTER, DIM).
    */

    /* Check multi-source blit feature. */
    if (Context->multiSourceBlt == gcvFALSE)
    {
        /* Singal source blit path.
         * Check compose type: by area or by layer.
         *   1) By default, we compose by layer.
         *   2) If hasDim, we compose by area for performance
         *   3) But if there's any YUV layer, we always compose by layer. */
        gctBOOL composeByArea = gcvFALSE;

        if (Context->hasDim)
        {
            /* Compose by area for dim optimization. */
            composeByArea = gcvTRUE;

            for (gctUINT i = 0; i < Context->layerCount; i++)
            {
                if ((Context->layers[i].source)
                &&  (Context->layers[i].yuv)
                )
                {
                    /* But if there's any yuv layer, it is better to compose all
                     * by layer. */
                    composeByArea = gcvFALSE;
                    break;
                }
            }
        }

        if (composeByArea)
        {
            /* Compose by area.
             * We only need to go through all areas
             * BLITTER: blit area
             * DIM: only blit area if on bottom
             *      use alphaDim for non-bottom DIM areas
             * CLEAR_HOLE & OVERLAY: skip because already composed */
            area = Context->compositionArea;

            while (area != NULL)
            {
                gcsRECT clipRect;

                if ((area->owners & ~composed) == 0U)
                {
                    /* Reject this area because has no layers. */
                    area = area->next;
                    continue;
                }

#if ENABLE_SWAP_RECTANGLE
                /* Intersect with swap rectangle. */
                clipRect.left   = gcmMAX(target->swapRect.left,   area->rect.left);
                clipRect.top    = gcmMAX(target->swapRect.top,    area->rect.top);
                clipRect.right  = gcmMIN(target->swapRect.right,  area->rect.right);
                clipRect.bottom = gcmMIN(target->swapRect.bottom, area->rect.bottom);

                if ((clipRect.left >= clipRect.right)
                ||  (clipRect.top  >= clipRect.bottom)
                )
                {
                    /* Skip areas out of swap rectangle. */
                    area = area->next;
                    continue;
                }
#else
                clipRect = area->rect;
#endif

                /* Go through all areas. */
                for (gctUINT32 i = 0; i < Context->layerCount; i++)
                {
                    gctUINT32 alphaDim;
                    hwcLayer * layer;
                    gctUINT32 owner;

                    /* Compute owner. */
                    owner = (1U << i);

                    if ((!(area->owners & owner))
                    ||  (composed & owner)
                    )
                    {
                        if (owner > area->owners)
                        {
                            /* No more layers. */
                            break;
                        }

                        /* No such layer or already composed. */
                        continue;
                    }

                    /* Get shortcut. */
                    layer = &Context->layers[i];

                    if ((layer->compositionType == HWC_DIM)
                    &&  (((owner - 1U) & area->owners))
                    )
                    {
                        /* Skip DIM layer if it is not on bottom.
                         * Non-bottom DIM layers are done with optimization. */
                        continue;
                    }

                    /* Compute alphaDim. */
                    alphaDim = 0xFF;

                    for (gctUINT32 j = (i + 1); j < Context->layerCount; j++)
                    {
                        if (((1U << j) & area->owners)
                        &&  (Context->layers[j].compositionType == HWC_DIM)
                        )
                        {
                            alphaDim *= (0xFF - (Context->layers[j].color32 >> 24));
                            alphaDim /= 0xFF;
                        }
                    }

                    /* Precision correction. */
                    if (alphaDim < 0xFF)
                    {
                        alphaDim += 1;
                    }

                    /* Start single-source blit. */
                    gcmONERROR(
                        _Blit(Context, i, alphaDim, &clipRect));
                }

                /* Advance to next area. */
                area = area->next;

                /* Commit commands. */
                gcmONERROR(gcoHAL_Commit(Context->hal, gcvFALSE));
            }

            /* All areas done. */
            return gcvSTATUS_OK;
        }

        /* else compose by layer */

        /* Go through all layers.
         * BLITTER: blit layer
         * DIM: blit layer
         * CLEARHOLE & OVERLAY: skip because already composed */
        for (gctUINT32 i = 0; i < Context->layerCount; i++)
        {
            if ((1U << i) & composed)
            {
                /* Already composed: CLEAR_HOLE/OVERLAY. */
                continue;
            }

            /* Get shortcut. */
            hwcLayer * layer = &Context->layers[i];

            for (gctUINT32 j = 0; j < layer->clipCount; j++)
            {
#if ENABLE_SWAP_RECTANGLE
                gcsRECT clipRect;

                /* Shortcut to dest clip rect. */
                gcsRECT_PTR rect = &layer->clipRects[j];

                /* Intersect with swap rectangle. */
                clipRect.left   = gcmMAX(target->swapRect.left,   rect->left);
                clipRect.top    = gcmMAX(target->swapRect.top,    rect->top);
                clipRect.right  = gcmMIN(target->swapRect.right,  rect->right);
                clipRect.bottom = gcmMIN(target->swapRect.bottom, rect->bottom);

                if ((clipRect.left >= clipRect.right)
                ||  (clipRect.top  >= clipRect.bottom)
                )
                {
                    /* Skip clipRect out of swap rectangle. */
                    continue;
                }

                /* Start single-source blit. */
                gcmONERROR(
                    _Blit(Context, i, 0xFF, &clipRect));
#else
                /* Start single-source blit. */
                gcmONERROR(
                    _Blit(Context, i, 0xFF, &layer->clipRects[j]));
#endif
            }

            /* Commit commands. */
            gcmONERROR(gcoHAL_Commit(Context->hal, gcvFALSE));
        }

        /* Done. */
        return gcvSTATUS_OK;
    }

    /* Multi-source blit path.
     * Some layers can never be composed by multi-source blit. Let's call such
     * layers as 'barrier'. 'Barrier' layers are always composed by layer but
     * never by area.
     * Between 'barrier' layers, we try multi-source blit by area.
     *
     * BLITTER: barrier if YUV, stretch or mirror(no Ex feature)
     * DIM:  do DIM optimization if start is 0, and this is the first DIM layer.
     * CLEAR_HOLE & OVERLAY: skip because already composed */
    for (gctUINT start = 0; start < Context->layerCount; start++)
    {
        hwcLayer * layer;
        /* Record how many layers between start and barrier layer. */
        gctUINT between = 0;
        /* Record the barrier layer index. */
        gctUINT barrier;

        if ((1U << start) & composed)
        {
            /* Already composed: CLEAR_HOLE/OVERLAY. */
            continue;
        }

        /* Try to find a barrier layer. */
        for (barrier = start; barrier < Context->layerCount; barrier++)
        {
            if ((1U << barrier) & composed)
            {
                /* Already composed: CLEAR_HOLE/OVERLAY. */
                continue;
            }

            /* Get shortcut. */
            layer = &Context->layers[barrier];

            if ((layer->source != gcvNULL)
            &&  (  (layer->stretch)
                || (layer->yuv)
                || (  (layer->hMirror || layer->vMirror)
                   && (Context->multiSourceBltEx == gcvFALSE)))
            )
            {
                /* Barrier blttiter layer. */
                break;
            }

            if ((layer->compositionType == HWC_DIM)
            &&  ((start != 0) || (barrier == 0))
            )
            {
                /* Dim as barrier if start layer is not bottom.
                 * This is because, if start layer is not bottom, this DIM layer
                 * may cover layers under start.
                 * Another case, the layer on bottom is DIM.
                 * The bottom DIM layer can not use DIM optimization. */
                break;
            }

            /* Inc layer counter between start and barrier. */
            between ++;
        }

#if DUMP_COMPOSE
        ALOGD("  TRY multi-source between %d,%d",
             start, barrier);
#endif

        /* Check layer between. */
        if (between <= 1)
        {
            /* _Blit by layer from start and barrier(include) if,
             * 1. There's no layer between start and barrier, which means the
             * start layer is the barrier layer.
             * 2. there's only one layer between start and barrier, which
             * means the start layer is the only layer. */
            for (gctUINT i = start;
                 i <= barrier && i < Context->layerCount;
                 i++)
            {
                if ((1U << i) & composed)
                {
                    /* Already composed: CLEAR_HOLE/OVERLAY. */
                    continue;
                }

                /* Get shortcut. */
                layer = &Context->layers[i];

                for (gctUINT j = 0; j < layer->clipCount; j++)
                {
#if ENABLE_SWAP_RECTANGLE
                    gcsRECT clipRect;

                    /* Shortcut to dest clip rect. */
                    gcsRECT_PTR rect = &layer->clipRects[j];

                    /* Intersect with swap rectangle. */
                    clipRect.left   = gcmMAX(target->swapRect.left,   rect->left);
                    clipRect.top    = gcmMAX(target->swapRect.top,    rect->top);
                    clipRect.right  = gcmMIN(target->swapRect.right,  rect->right);
                    clipRect.bottom = gcmMIN(target->swapRect.bottom, rect->bottom);

                    if ((clipRect.left >= clipRect.right)
                    ||  (clipRect.top  >= clipRect.bottom)
                    )
                    {
                        /* Skip clipRect out of swap rectangle. */
                        continue;
                    }

                    /* Start single-source blit. */
                    gcmONERROR(
                        _Blit(Context, i, 0xFF, &clipRect));
#else
                    /* Start single-source blit. */
                    gcmONERROR(
                        _Blit(Context, i, 0xFF, &layer->clipRects[j]));
#endif
                }
            }

            /* We have handled layers till barrier. */
            start = barrier;

            /* Go to next batch directly. */
            gcmONERROR(gcoHAL_Commit(Context->hal, gcvFALSE));

            /* Go to next batch directly. */
            continue;
        }

        /* Try multi-source blit by area between start and barrier. */
        area = Context->compositionArea;

        while (area != NULL)
        {
            gcsRECT clipRect;

            if ((area->owners & ~composed) == 0U)
            {
                /* Reject this area because has no layers. */
                area = area->next;
                continue;
            }

#if ENABLE_SWAP_RECTANGLE
            /* Intersect with swap rectangle. */
            clipRect.left   = gcmMAX(target->swapRect.left,   area->rect.left);
            clipRect.top    = gcmMAX(target->swapRect.top,    area->rect.top);
            clipRect.right  = gcmMIN(target->swapRect.right,  area->rect.right);
            clipRect.bottom = gcmMIN(target->swapRect.bottom, area->rect.bottom);

            if ((clipRect.left >= clipRect.right)
            ||  (clipRect.top  >= clipRect.bottom)
            )
            {
                /* Skip areas out of swap rectangle. */
                area = area->next;
                continue;
            }
#else
            clipRect = area->rect;
#endif

            /* Multi-source blit locals. */
            gctUINT32 sourceIndices[8];
            gctUINT32 sourceAlphas[8];
            gctUINT32 sourceCount = 0;

            /* Eigen rect. */
            /* Value -1  means any value is OK.
             * Otherwise means specified values. */
            gcsRECT eigenRect;

            /* Correspond masks. Since different condition has different
             * constraints, we need remember the constraints as well. */
            gcsRECT maskRect = {0};

            /* YUV input existed.
             * Multi-source blit can only support one YUV input. */
            gctBOOL hasYuv      = gcvFALSE;

            /* Multi-source layers initialized? */
            gctBOOL initialized = gcvFALSE;

            /* Get initial left eigenvalue. */
            if (Context->multiSourceBltEx)
            {
                eigenRect.left  = -1;
            }
            else
            {
                /* Framebuffer alignment constraint. */
                eigenRect.left  = clipRect.left & framebuffer->alignMask;
                maskRect.left   = framebuffer->alignMask;
            }

            /* Get other initial eigenvalues. */
            eigenRect.top    =
            eigenRect.right  =
            eigenRect.bottom = -1;

            /* Try multi-source blit between start and barrier. */
            for (gctUINT32 i = start; i < barrier; i++)
            {
                gctUINT32 owner = (1U << i);
                gctUINT32 alphaDim;

                if (!(area->owners & owner)
                ||  (owner & composed)
                )
                {
                    if (owner > area->owners)
                    {
                        /* No more layers. */
                        break;
                    }

                    /* No such layer or already composed. */
                    continue;
                }

                /* Get short cut. */
                layer = &Context->layers[i];

                if (layer->compositionType == HWC_DIM)
                {
                    /* Skip DIM layer because DIM optimization is used.
                     * If runs here, DIM layer is not compose by barrier
                     * and this layer should not be not on bottom. */
                    continue;
                }

                /* Compute alphaDim. */
                alphaDim = 0xFF;

                for (gctUINT32 j = (i + 1); j < barrier; j++)
                {
                    if (((1U << j) & area->owners)
                    &&  (Context->layers[j].compositionType == HWC_DIM)
                    )
                    {
                        alphaDim *= (0xFF - (Context->layers[j].color32 >> 24));
                        alphaDim /= 0xFF;
                    }
                }

                /* Precision correction. */
                if (alphaDim < 0xFF)
                {
                    alphaDim += 1;
                }

                /* Handled by multi-source blit? */
                gctBOOL handled = gcvFALSE;

                do
                {
                    gctINT offset;
                    gctINT eigen;
                    gctINT alignMask;
                    gctINT transform;

                    if (layer->source == gcvNULL)
                    {
                        /* This layer must be non-bottom DIM layer. */
                        handled = gcvTRUE;
                        break;
                    }

                    /* FIXME: yuv is already disabled. */
                    if (hasYuv && layer->yuv)
                    {
                        /* Multi-source blit can only support ONE yuv layer. */
                        break;
                    }

                    /* Check 'Multi-Source Blit Ex' feature. */
                    if (Context->multiSourceBltEx && !layer->yuv)
                    {
                        /* If 'Ex' feature is avaible, there's no buffer address
                         * alignment limitation for RGB source. */
                        handled = gcvTRUE;
                        break;
                    }

                    /* Get shortcut. */
                    alignMask = layer->alignMask;

                    /* Translate rotation/mirrors to tranform flags.
                     * Bit 0: 90 deg flag.
                     * Bit 1: H mirror flag.
                     * Bit 2: V mirror flag. */
                    transform = (layer->rotation == gcvSURF_0_DEGREE) ? 0
                              : (layer->rotation == gcvSURF_90_DEGREE) ? 4
                              : (layer->rotation == gcvSURF_180_DEGREE) ? 3
                              : 7;

                    if (layer->hMirror)
                    {
                        /* Transoform H bit. */
                        if (transform & 1)
                        {
                            transform &= ~1;
                        }
                        else
                        {
                            transform |= 1;
                        }
                    }

                    if (layer->vMirror)
                    {
                        /* Transoform V bit. */
                        if (transform & 2)
                        {
                            transform &= ~2;
                        }
                        else
                        {
                            transform |= 2;
                        }
                    }

                    /* Check start address alignments.
                     * We need to move buffer address to an aligned one, so we
                     * need compute coorespoinding coordinate offsets.
                     * The 'eigenvalues' are actually left coodinate offsets in
                     * 'original coord sys'.
                     *
                     * Left coordinate becomes at left,bottom,right,top when 0,90,
                     * 180,270 degree rotated.
                     * So the first step is to compute needed coordinate (need left
                     * for 0 deg, bottom for 90 deg, etc) to 'original coord sys',
                     * and then move the left coordinate in 'original coord sys' to
                     * aligned one.
                     *
                     * [+]-------------------------+
                     *  |        ^                 |
                     *  |        L (270 deg)       |
                     *  |        v                 |
                     *  |     +--------------+     |
                     *  |     | blit rect    |     |
                     *  |<-L->|              |<-L->|
                     *  |(0   |              |(180 |
                     *  | deg)|              | deg)|
                     *  |     +--------------+     |
                     *  |        ^                 |
                     *  |        L (90 deg)        |
                     *  |        v                 |
                     *  +--------------------------+
                     */
                    switch (transform)
                    {
                    case 0: case 2:
                    default:
                        /* 0 deg, V mirror. */
                        offset = (layer->dstRect.left - clipRect.left);
                        eigen  = (layer->orgRect.left - offset) & alignMask;

                        if (eigenRect.left < 0)
                        {
                            /* Get new left eigenvalue. */
                            eigenRect.left = eigen;
                            maskRect.left  = alignMask;
                            handled        = gcvTRUE;
                        }
                        else
                        {
                            if (maskRect.left == alignMask)
                            {
                                /* Check with last left eigenvalue. */
                                handled = (eigenRect.left == eigen);
                            }
                            else if (maskRect.left > alignMask)
                            {
                                /* Check with masked last left eigenvalue. */
                                handled = ((eigenRect.left & alignMask) == eigen);
                            }
                            else
                            {
                                /* Stronger constraint comes. */
                                if (eigenRect.left == (eigen & maskRect.left))
                                {
                                    handled = gcvTRUE;

                                    /* Update mask. */
                                    maskRect.left = alignMask;

                                    /* Update left eigenvalue. */
                                    eigenRect.left = eigen;
                                }
                                else
                                {
                                    handled = gcvFALSE;
                                }
                            }
                        }
                        break;

                    case 4: case 5:
                        /* 90 deg, 90 deg + H mirror. */
                        offset = (layer->dstRect.bottom - clipRect.bottom);
                        eigen  = (layer->orgRect.left + offset) & alignMask;

                        if (eigenRect.bottom < 0)
                        {
                            /* Get new bottom eigenvalue. */
                            eigenRect.bottom = eigen;
                            maskRect.bottom  = alignMask;
                            handled          = gcvTRUE;
                        }
                        else
                        {
                            if (maskRect.bottom == alignMask)
                            {
                                /* Check with last bottom eigenvalue. */
                                handled = (eigenRect.bottom == eigen);
                            }
                            else if (maskRect.bottom > alignMask)
                            {
                                /* Check with masked last bottom eigenvalue. */
                                handled = ((eigenRect.bottom & alignMask) == eigen);
                            }
                            else
                            {
                                /* Stronger constraint comes. */
                                if (eigenRect.bottom == (eigen & maskRect.bottom))
                                {
                                    handled = gcvTRUE;

                                    /* Update mask. */
                                    maskRect.bottom = alignMask;

                                    /* Update bottom eigenvalue. */
                                    eigenRect.bottom = eigen;
                                }
                                else
                                {
                                    handled = gcvFALSE;
                                }
                            }
                        }
                        break;

                    case 3: case 1:
                        /* 180 deg, H mirror. */
                        offset = (layer->dstRect.right - clipRect.right);
                        eigen  = (layer->orgRect.left + offset) & alignMask;

                        if (eigenRect.right < 0)
                        {
                            /* Get new right eigenvalue. */
                            eigenRect.right = eigen;
                            maskRect.right  = alignMask;
                            handled         = gcvTRUE;
                        }
                        else
                        {
                            if (maskRect.right == alignMask)
                            {
                                /* Check with last right eigenvalue. */
                                handled = (eigenRect.right == eigen);
                            }
                            else if (maskRect.right > alignMask)
                            {
                                /* Check with masked last right eigenvalue. */
                                handled = ((eigenRect.right & alignMask) == eigen);
                            }
                            else
                            {
                                /* Stronger constraint comes. */
                                if (eigenRect.right == (eigen & maskRect.right))
                                {
                                    handled = gcvTRUE;

                                    /* Update mask. */
                                    maskRect.right = alignMask;

                                    /* Update right eigenvalue. */
                                    eigenRect.right = eigen;
                                }
                                else
                                {
                                    handled = gcvFALSE;
                                }
                            }
                        }
                        break;

                    case 7: case 6:
                        /* 270 deg, 90 deg + V mirror. */
                        offset = (layer->dstRect.top - clipRect.top);
                        eigen  = (layer->orgRect.left - offset) & alignMask;

                        if (eigenRect.top < 0)
                        {
                            /* Get new top eigenvalue. */
                            eigenRect.top = eigen;
                            maskRect.top  = alignMask;
                            handled       = gcvTRUE;
                        }
                        else
                        {
                            if (maskRect.top == alignMask)
                            {
                                /* Check with last top eigenvalue. */
                                handled = (eigenRect.top == eigen);
                            }
                            else if (maskRect.top > alignMask)
                            {
                                /* Check with masked last top eigenvalue. */
                                handled = ((eigenRect.top & alignMask) == eigen);
                            }
                            else
                            {
                                /* Stronger constraint comes. */
                                if (eigenRect.top == (eigen & maskRect.top))
                                {
                                    handled = gcvTRUE;

                                    /* Update mask. */
                                    maskRect.top = alignMask;

                                    /* Update top eigenvalue. */
                                    eigenRect.top = eigen;
                                }
                                else
                                {
                                    handled = gcvFALSE;
                                }
                            }
                        }
                        break;
                    }

                    if (handled == gcvFALSE)
                    {
                        /* Do not need check more. */
                        break;
                    }

                    /* Check YUV420 format, YUV420 has more limitations. */
                    if ((layer->yuv == gcvFALSE)
                    ||  (  (layer->format != gcvSURF_NV12)
                        && (layer->format != gcvSURF_NV21)
                        && (layer->format != gcvSURF_YV12)
                        && (layer->format != gcvSURF_I420)
                        )
                    )
                    {
                        /* Not YUV420, done. */
                        break;
                    }

                    /* Check again for top coordinate alignment for YUV420.
                     *
                     * Top coordinate becomes at top,left,bottom,right when 0,90,
                     * 180,270 degree rotated.
                     * So the first step is to compute needed coordinate (need top
                     * for 0 deg, left for 90 deg, etc) to 'original coord sys',
                     * and then move the top coordinate in 'original coord sys' to
                     * aligned one.
                     *
                     *  [+]------------------------+
                     *  |        ^                 |
                     *  |        T (0 deg)         |
                     *  |        v                 |
                     *  |     +--------------+     |
                     *  |     | blit rect    |     |
                     *  |<-T->|              |<-T->|
                     *  |(90  |              |(270 |
                     *  | deg)|              | deg)|
                     *  |     +--------------+     |
                     *  |        ^                 |
                     *  |        T (180 deg)       |
                     *  |        v                 |
                     *  +--------------------------+
                     */
                    switch (transform)
                    {
                    case 0: case 1:
                    default:
                        /* 0 deg, H mirror. */
                        offset = (layer->dstRect.top - clipRect.top);
                        eigen  = (layer->orgRect.top - offset) & 1;

                        if (eigenRect.top < 0)
                        {
                            /* Get new top eigenvalue. */
                            eigenRect.top = eigen;
                            maskRect.top  = 1;
                            handled       = gcvTRUE;
                        }
                        else
                        {
                            /* Check with masked last top eigenvalue. */
                            handled = ((eigenRect.top & 1) == eigen);
                        }
                        break;

                    case 4: case 6:
                        /* 90 deg, 90 deg + V mirror. */
                        offset = (layer->dstRect.left - clipRect.left);
                        eigen  = (layer->orgRect.top - offset) & 1;

                        if (eigenRect.left < 0)
                        {
                            /* Get new left eigenvalue. */
                            eigenRect.left = eigen;
                            maskRect.left  = 1;
                            handled        = gcvTRUE;
                        }
                        else
                        {
                            /* Check with masked last left eigenvalue. */
                            handled = ((eigenRect.left & 1) == eigen);
                        }
                        break;

                    case 3: case 2:
                        /* 180 deg, V mirror. */
                        offset = (layer->dstRect.bottom - clipRect.bottom);
                        eigen  = (layer->orgRect.top + offset) & 1;

                        if (eigenRect.bottom < 0)
                        {
                            /* Get new bottom eigenvalue. */
                            eigenRect.bottom = eigen;
                            maskRect.bottom  = 1;
                            handled          = gcvTRUE;
                        }
                        else
                        {
                            /* Check with masked last bottom eigenvalue. */
                            handled = ((eigenRect.bottom & 1) == eigen);
                        }
                        break;

                    case 7: case 5:
                        /* 270 deg, 90 deg + H mirror. */
                        offset = (layer->dstRect.right - clipRect.right);
                        eigen  = (layer->orgRect.top + offset) & 1;

                        if (eigenRect.right < 0)
                        {
                            /* Get new right eigenvalue. */
                            eigenRect.right = eigen;
                            maskRect.right  = 1;
                            handled         = gcvTRUE;
                        }
                        else
                        {
                            /* Check with masked last right eigenvalue. */
                            handled = ((eigenRect.right & 1) == eigen);
                        }
                        break;
                    }
                }
                while (gcvFALSE);

                if (initialized && handled)
                {
                    /* Has layers before and the new layer can be handled. */

                    /* Update source array. */
                    sourceAlphas[sourceCount] = alphaDim;
                    sourceIndices[sourceCount++] = i;

                    /* Update hasYuv flag. */
                    hasYuv = (layer->source && layer->yuv);

                    /* Check source count limitation. */
                    if ( ((!Context->layers[sourceIndices[0]].opaque)
                       && (sourceCount >= Context->maxSource - 1))
                    || (sourceCount >= Context->maxSource)
                    )
                    {
                        /* Trigger multi-source blit. */
                        gcmONERROR(
                            _MultiSourceBlit(Context,
                                             sourceIndices,
                                             sourceAlphas,
                                             sourceCount,
                                             &eigenRect,
                                             &clipRect));

                        /* Reset flags. */
                        sourceCount = 0U;
                        hasYuv      = gcvFALSE;
                        initialized = gcvFALSE;

                        eigenRect.top    =
                        eigenRect.right  =
                        eigenRect.bottom = -1;
                    }
                }

                else if (initialized)
                {
                    /* Has layers before but the new layer can not be handled. */
                    if (sourceCount > 1)
                    {
                        /* Trigger multi-source blit. */
                        gcmONERROR(
                        _MultiSourceBlit(Context,
                                         sourceIndices,
                                         sourceAlphas,
                                         sourceCount,
                                         &eigenRect,
                                         &clipRect));
                    }
                    else
                    {
                        /* Use single-source blit instead. */
                        gcmONERROR(
                            _Blit(Context,
                                  sourceIndices[0],
                                  sourceAlphas[0],
                                  &clipRect));
                    }

                    /* Reset flags. */
                    sourceCount = 0U;
                    hasYuv      = gcvFALSE;
                    initialized = gcvFALSE;

                    eigenRect.top    =
                    eigenRect.right  =
                    eigenRect.bottom = -1;

                    /* Need check this layer again. */
                    i--;
                }

                else if (handled)
                {
                    /* First layer comes. */
                    initialized = gcvTRUE;

                    /* Update source array. */
                    sourceAlphas[sourceCount] = alphaDim;
                    sourceIndices[sourceCount++] = i;

                    /* Update hasYuv flag. */
                    hasYuv = (layer->source && layer->yuv);
                }

                else
                {
                    /* A single single-source blit layer comes.
                     * Start single-source blit immediately. */
                    gcmONERROR(_Blit(Context, i, alphaDim, &clipRect));
                }
            }

            /* Start multi-source blit for accumulated layers if any. */
            if (sourceCount > 1)
            {
                /* Trigger multi-source blit. */
                gcmONERROR(
                    _MultiSourceBlit(Context,
                                     sourceIndices,
                                     sourceAlphas,
                                     sourceCount,
                                     &eigenRect,
                                     &clipRect));
            }

            else if (sourceCount == 1)
            {
                /* Use single-source blit instead. */
                gcmONERROR(
                    _Blit(Context, sourceIndices[0], sourceAlphas[0], &clipRect));
            }

            /* Advance to next area. */
            area = area->next;

            /* Commit commands. */
            gcmONERROR(gcoHAL_Commit(Context->hal, gcvFALSE));
        }

        /* Check whether we need blit the barrier layer. */
        if (barrier >= Context->layerCount)
        {
            /* All layers done. */
            break;;
        }

        /* Now we need to compose the barrier layer. */
        layer = &Context->layers[barrier];

        for (gctUINT32 j = 0; j < layer->clipCount; j++)
        {
#if ENABLE_SWAP_RECTANGLE
            gcsRECT clipRect;

            /* Shortcut to dest clip rect. */
            gcsRECT_PTR rect = &layer->clipRects[j];

            /* Intersect with swap rectangle. */
            clipRect.left   = gcmMAX(target->swapRect.left,   rect->left);
            clipRect.top    = gcmMAX(target->swapRect.top,    rect->top);
            clipRect.right  = gcmMIN(target->swapRect.right,  rect->right);
            clipRect.bottom = gcmMIN(target->swapRect.bottom, rect->bottom);

            if ((clipRect.left >= clipRect.right)
            ||  (clipRect.top  >= clipRect.bottom)
            )
            {
                /* Skip clipRect out of swap rectangle. */
                continue;
            }

            /* Start single-source blit. */
            gcmONERROR(
                _Blit(Context, barrier, 0xFF, &clipRect));
#else
            /* Start single-source blit. */
            gcmONERROR(
                _Blit(Context, barrier, 0xFF, &layer->clipRects[j]));
#endif
        }

        /* We have handled layers till barrier. */
        start = barrier;

        /* Commit commands. */
        gcmONERROR(gcoHAL_Commit(Context->hal, gcvFALSE));
    }

    return gcvSTATUS_OK;

OnError:
    ALOGE("Failed in %s: status=%d", __FUNCTION__, status);
    return status;
}


gceSTATUS
_CopySwapRect(
    IN hwcContext * Context
    )
{
    gceSTATUS status;
    hwcFramebuffer * framebuffer = Context->framebuffer;
    hwcBuffer * target           = framebuffer->target;
    hwcArea * area               = Context->swapArea;


    /***************************************************************************
    ** Setup Source.
    */

    /* Setup source index. */
    gcmONERROR(
        gco2D_SetCurrentSourceIndex(Context->engine, 0U));

    /* Setup source. */
    gcmONERROR(
        gco2D_SetGenericSource(Context->engine,
                               &target->prev->physical,
                               1U,
                               &framebuffer->stride,
                               1U,
                               framebuffer->tiling,
                               framebuffer->format,
                               gcvSURF_0_DEGREE,
                               framebuffer->res.right,
                               framebuffer->res.bottom));

    /* Setup mirror. */
    gcmONERROR(
        gco2D_SetBitBlitMirror(Context->engine,
                               gcvFALSE,
                               gcvFALSE));

    /* Disable alhpa blending. */
    gcmONERROR(
        gco2D_DisableAlphaBlend(Context->engine));

    /* Disable premultiply. */
    gcmONERROR(
        gco2D_SetPixelMultiplyModeAdvanced(Context->engine,
                                           gcv2D_COLOR_MULTIPLY_DISABLE,
                                           gcv2D_COLOR_MULTIPLY_DISABLE,
                                           gcv2D_GLOBAL_COLOR_MULTIPLY_DISABLE,
                                           gcv2D_COLOR_MULTIPLY_DISABLE));

    /* Setup clipping to full screen. */
    gcmONERROR(
        gco2D_SetClipping(Context->engine,
                          &framebuffer->res));


    /***************************************************************************
    ** Setup Target.
    */

    gcmONERROR(
        gco2D_SetGenericTarget(Context->engine,
                               &target->physical,
                               1U,
                               &framebuffer->stride,
                               1U,
                               framebuffer->tiling,
                               framebuffer->format,
                               gcvSURF_0_DEGREE,
                               framebuffer->res.right,
                               framebuffer->res.bottom));

    /***************************************************************************
    ** Copy Swap Areas.
    */

    while (area != NULL)
    {
        /* Blit only layers without owners. No owner means we need copy area
         * from front buffer to current. */
        if (area->owners == 0U)
        {
            /* Do batchblit. */
            gcmONERROR(
                gco2D_BatchBlit(Context->engine,
                                1U,
                                &area->rect,
                                &area->rect,
                                0xCC,
                                0xCC,
                                framebuffer->format));

#if DUMP_COMPOSE

        ALOGD("  SWAP RECT: [%d,%d,%d,%d] (%08x)=>(%08x)",
             area->rect.left,
             area->rect.top,
             area->rect.right,
             area->rect.bottom,
             target->prev->physical,
             target->physical);
#endif
        }

        /* Advance to next area. */
        area = area->next;
    }

    return gcvSTATUS_OK;

OnError:
    ALOGE("Failed in %s: status=%d", __FUNCTION__, status);
    return status;
}


gceSTATUS
_WormHole(
    IN hwcContext * Context,
    IN gcsRECT * Rect
    )
{
    gceSTATUS status;
    hwcFramebuffer * framebuffer = Context->framebuffer;
    hwcBuffer * target           = framebuffer->target;

    /* Disable alpha blending. */
    gcmONERROR(
        gco2D_DisableAlphaBlend(Context->engine));

    /* No premultiply. */
    gcmONERROR(
        gco2D_SetPixelMultiplyModeAdvanced(Context->engine,
                                           gcv2D_COLOR_MULTIPLY_DISABLE,
                                           gcv2D_COLOR_MULTIPLY_DISABLE,
                                           gcv2D_GLOBAL_COLOR_MULTIPLY_DISABLE,
                                           gcv2D_COLOR_MULTIPLY_DISABLE));

    /* Setup Target. */
    gcmONERROR(
        gco2D_SetGenericTarget(Context->engine,
                               &target->physical,
                               1U,
                               &framebuffer->stride,
                               1U,
                               framebuffer->tiling,
                               framebuffer->format,
                               gcvSURF_0_DEGREE,
                               framebuffer->res.right,
                               framebuffer->res.bottom));

#if DUMP_COMPOSE

        ALOGD("  WORMHOLE: [%d,%d,%d,%d] (%08x)",
             Rect->left,
             Rect->top,
             Rect->right,
             Rect->bottom,
             framebuffer->target->physical);
#endif

    /* Perform a Clear. */
    gcmONERROR(
        gco2D_Clear(Context->engine,
                    1U,
                    Rect,
                    0x00000000,
                    0xCC,
                    0xCC,
                    framebuffer->format));


    return gcvSTATUS_OK;

OnError:
    ALOGE("Failed in %s: status=%d", __FUNCTION__, status);
    return status;
}


/* Single-source blit. */
gceSTATUS
_Blit(
    IN hwcContext * Context,
    IN gctUINT32 Index,
    IN gctUINT32 AlphaDim,
    IN gcsRECT * ClipRect
    )
{
    gceSTATUS status;
    gcsRECT srcRect;

    hwcFramebuffer * framebuffer = Context->framebuffer;
    hwcBuffer * target           = framebuffer->target;

    hwcLayer * layer = &Context->layers[Index];


    /***************************************************************************
    ** Dim detection.
    */

    /* We are handling DIM differently as last hwcomposer version.
     * DIM alpha 255 is special because it is actually a clear. This 'solid dim'
     * is handled in 'DIM Optimization' section in hwcSet.
     *
     * For normal DIM case, it is doing DIM lie,
     *
     *   CsAs = (0, 0, 0, alpha)
     *
     *   Cd' = Cs + Cd * (1 - As)
     *   Ad' = As + Ad * (1 - As)
     *
     * That is,
     *   Cd' = Cd * (1 - As) = Cd * (1 - alpha)
     *
     * So, can easily 'premultiply' a (1 - alpha) value for each layers before
     * this DIM layer and skip the DIM layer.
     * This optimization will reduce a layer!
     */

    /***************************************************************************
    ** Setup Source.
    */

    /* Setup source index. */
    gcmONERROR(
        gco2D_SetCurrentSourceIndex(Context->engine, 0U));

    /* Setup source. */
    if (layer->source != gcvNULL)
    {
        /* Get shortcuts. */
        gctFLOAT hfactor = layer->hfactor;
        gctFLOAT vfactor = layer->vfactor;

        /* Compute delta(s). */
        gctFLOAT dl = (layer->dstRect.left   - ClipRect->left)   * hfactor;
        gctFLOAT dt = (layer->dstRect.top    - ClipRect->top)    * vfactor;
        gctFLOAT dr = (layer->dstRect.right  - ClipRect->right)  * hfactor;
        gctFLOAT db = (layer->dstRect.bottom - ClipRect->bottom) * vfactor;

        if (layer->hMirror)
        {
            gctFLOAT tl = dl;

            dl = -dr;
            dr = -tl;
        }

        if (layer->vMirror)
        {
            gctFLOAT tt = dt;

            dt = -db;
            db = -tt;
        }

        /* Compute source rectangle. */
        srcRect.left   = gctINT(layer->srcRect.left   - dl + 0.5f);
        srcRect.top    = gctINT(layer->srcRect.top    - dt + 0.5f);
        srcRect.right  = gctINT(layer->srcRect.right  - dr + 0.5f);
        srcRect.bottom = gctINT(layer->srcRect.bottom - db + 0.5f);

        /* Adjust very small source rectangle. */
        if (srcRect.right == srcRect.left)
        {
            if ((dr + dl > 0.0f)
            &&  (srcRect.right < layer->srcRect.right)
            )
            {
                srcRect.right  += 1;
            }
            else
            {
                srcRect.left   -= 1;
            }
        }

        if (srcRect.bottom == srcRect.top)
        {
            if ((db + dt > 0.0f)
            &&  (srcRect.bottom < layer->srcRect.bottom)
            )
            {
                srcRect.bottom += 1;
            }
            else
            {
                srcRect.top    -= 1;
            }
        }

        /* Set source address. */
        gcmONERROR(
            gco2D_SetGenericSource(Context->engine,
                                   layer->addresses,
                                   layer->addressNum,
                                   layer->strides,
                                   layer->strideNum,
                                   layer->tiling,
                                   layer->format,
                                   layer->rotation,
                                   layer->width,
                                   layer->height));

        /* Setup mirror. */
        gcmONERROR(
            gco2D_SetBitBlitMirror(Context->engine,
                                   layer->hMirror,
                                   layer->vMirror));


        /* Set source rect. */
        gcmONERROR(
            gco2D_SetSource(Context->engine,
                            &srcRect));
    }

    else
    {
        gcmONERROR(
            gco2D_LoadSolidBrush(Context->engine,
                                 /* This should not be taken. */
                                 gcvSURF_UNKNOWN,
                                 gcvTRUE,
                                 layer->color32,
                                 0));
    }

    /* Setup alpha blending. */
    if (layer->opaque)
    {
        /* The layer is at the very bottom in this area, and it needs
         * alpha blending. Where is the alpha blending target then?
         * This is actually another type of 'WormHole' and target area
         * should be cleared as [0,0,0,0] before blitting this layer.
         * But we can easily disable alpha blending to get the same
         * result. */
        gcmONERROR(
            gco2D_DisableAlphaBlend(Context->engine));
    }

    else
    {
        gcmONERROR(
            gco2D_EnableAlphaBlendAdvanced(Context->engine,
                                           layer->srcAlphaMode,
                                           layer->dstAlphaMode,
                                           layer->srcGlobalAlphaMode,
                                           layer->dstGlobalAlphaMode,
                                           layer->srcFactorMode,
                                           layer->dstFactorMode));
    }

    /* Setup premultiply. */
    if (AlphaDim < 0xFF)
    {
        gctUINT32 srcGlobalAlpha = (layer->srcGlobalAlpha >> 8) * AlphaDim;
        gctUINT32 dstGlobalAlpha = (layer->dstGlobalAlpha >> 8) * AlphaDim;

        srcGlobalAlpha &= 0xFF000000;
        dstGlobalAlpha &= 0xFF000000;

        /* Dim optimization. */
        gcmONERROR(
            gco2D_SetPixelMultiplyModeAdvanced(Context->engine,
                                               layer->srcPremultSrcAlpha,
                                               layer->dstPremultDstAlpha,
                                               gcv2D_GLOBAL_COLOR_MULTIPLY_ALPHA,
                                               layer->dstDemultDstAlpha));

        gcmONERROR(
            gco2D_SetSourceGlobalColorAdvanced(Context->engine,
                                               srcGlobalAlpha));

        gcmONERROR(
            gco2D_SetTargetGlobalColorAdvanced(Context->engine,
                                               dstGlobalAlpha));
    }

    else
    {
        gcmONERROR(
            gco2D_SetPixelMultiplyModeAdvanced(Context->engine,
                                               layer->srcPremultSrcAlpha,
                                               layer->dstPremultDstAlpha,
                                               layer->srcPremultGlobalMode,
                                               layer->dstDemultDstAlpha));

        gcmONERROR(
            gco2D_SetSourceGlobalColorAdvanced(Context->engine,
                                               layer->srcGlobalAlpha));

        gcmONERROR(
            gco2D_SetTargetGlobalColorAdvanced(Context->engine,
                                               layer->dstGlobalAlpha));
    }


    /***************************************************************************
    ** Setup Target.
    */

    gcmONERROR(
        gco2D_SetGenericTarget(Context->engine,
                               &target->physical,
                               1U,
                               &framebuffer->stride,
                               1U,
                               framebuffer->tiling,
                               framebuffer->format,
                               gcvSURF_0_DEGREE,
                               framebuffer->res.right,
                               framebuffer->res.bottom));


    /***************************************************************************
    ** Start Single-Source Blit(Clear).
    */

    if (layer->source != gcvNULL)
    {
#if ENABLE_FILTER_STRETCH
        /* Stretch or yuv which can not use bit blit. */
        if ((layer->stretch)
        ||  (  (layer->yuv)
            && ((Context->opf == gcvFALSE) && (layer->addressNum > 1)))
        )
#else
        /* YUV and this yuv can not use bit blit. */
        if ((layer->yuv)
        &&  (  (layer->stretch)
            || ((Context->opf == gcvFALSE) && (layer->addressNum > 1)))
        )
#endif
        {
            gcsRECT tmpRect;
            gcsRECT dstRect;
            gceSURF_ROTATION rotation;
            gctINT hkernel;
            gctINT vkernel;

            /* Adjust too small rectangle. */
            if (srcRect.right - srcRect.left == 1)
            {
                if (srcRect.right < layer->srcRect.right)
                {
                    srcRect.right += 1;
                }
                else
                {
                    srcRect.left  -= 1;
                }
            }

            if (srcRect.bottom - srcRect.top == 1)
            {
                if (srcRect.bottom < layer->srcRect.bottom)
                {
                    srcRect.bottom += 1;
                }
                else
                {
                    srcRect.top    -= 1;
                }
            }

            /* Translate to dest rotation. */
            switch (layer->rotation)
            {
            case gcvSURF_0_DEGREE:
            default:
                hkernel = layer->hkernel;
                vkernel = layer->vkernel;
                rotation = gcvSURF_0_DEGREE;
                break;

            case gcvSURF_90_DEGREE:
                hkernel = layer->vkernel;
                vkernel = layer->hkernel;
                rotation = gcvSURF_270_DEGREE;

                tmpRect = srcRect;

                srcRect.left   = layer->width - tmpRect.bottom;
                srcRect.top    = tmpRect.left;
                srcRect.right  = layer->width - tmpRect.top;
                srcRect.bottom = tmpRect.right;

                dstRect.left   = framebuffer->res.bottom - ClipRect->bottom;
                dstRect.top    = ClipRect->left;
                dstRect.right  = framebuffer->res.bottom - ClipRect->top;
                dstRect.bottom = ClipRect->right;

                ClipRect = &dstRect;

                /* Update mirror. */
                gcmONERROR(
                    gco2D_SetBitBlitMirror(Context->engine,
                                           layer->vMirror,
                                           layer->hMirror));
                break;

            case gcvSURF_180_DEGREE:
                hkernel = layer->hkernel;
                vkernel = layer->vkernel;
                rotation = gcvSURF_180_DEGREE;

                tmpRect = srcRect;

                srcRect.left   = layer->width  - tmpRect.right;
                srcRect.top    = layer->height - tmpRect.bottom;
                srcRect.right  = layer->width  - tmpRect.left;
                srcRect.bottom = layer->height - tmpRect.top;

                dstRect.left   = framebuffer->res.right  - ClipRect->right;
                dstRect.top    = framebuffer->res.bottom - ClipRect->bottom;
                dstRect.right  = framebuffer->res.right  - ClipRect->left;
                dstRect.bottom = framebuffer->res.bottom - ClipRect->top;

                ClipRect = &dstRect;
                break;

            case gcvSURF_270_DEGREE:
                hkernel = layer->vkernel;
                vkernel = layer->hkernel;
                rotation = gcvSURF_90_DEGREE;

                tmpRect = srcRect;

                srcRect.left   = tmpRect.top;
                srcRect.top    = layer->height - tmpRect.right;
                srcRect.right  = tmpRect.bottom;
                srcRect.bottom = layer->height - tmpRect.left;

                dstRect.left   = ClipRect->top;
                dstRect.top    = framebuffer->res.right - ClipRect->right;
                dstRect.right  = ClipRect->bottom;
                dstRect.bottom = framebuffer->res.right - ClipRect->left;

                ClipRect = &dstRect;

                /* Update mirror. */
                gcmONERROR(
                    gco2D_SetBitBlitMirror(Context->engine,
                                           layer->vMirror,
                                           layer->hMirror));
                break;
            }

            /* Use filterBlit to blit YUV source if YUV blit not supported. */
            /* Set kernel size. */
            gcmONERROR(
                gco2D_SetKernelSize(Context->engine,
                                    hkernel,
                                    vkernel));

            gcmONERROR(
                gco2D_SetFilterType(Context->engine,
                                    gcvFILTER_SYNC));

            /* Trigger filter blit. */
            gcmONERROR(
                gco2D_FilterBlitEx(Context->engine,
                                   layer->addresses[0],
                                   layer->strides[0],
                                   layer->addresses[1],
                                   layer->strides[1],
                                   layer->addresses[2],
                                   layer->strides[2],
                                   layer->format,
                                   gcvSURF_0_DEGREE,
                                   layer->width,
                                   layer->height,
                                   &srcRect,
                                   target->physical,
                                   framebuffer->stride,
                                   framebuffer->format,
                                   rotation,
                                   framebuffer->res.right,
                                   framebuffer->res.bottom,
                                   ClipRect,
                                   gcvNULL));


#if DUMP_COMPOSE

        ALOGD("  FILTER: layer[%d] (format=%d,alpha=%d,mirror=%d,%d,kernel=%d,%d): "
             "[%d,%d,%d,%d] (%08x) => [%d,%d,%d,%d] (%08x,rot=%d)",
             Index,
             layer->format,
             AlphaDim,
             layer->hMirror,
             layer->vMirror,
             hkernel,
             vkernel,
             srcRect.left,
             srcRect.top,
             srcRect.right,
             srcRect.bottom,
             layer->addresses[0],
             ClipRect->left,
             ClipRect->top,
             ClipRect->right,
             ClipRect->bottom,
             framebuffer->target->physical,
             rotation);
#endif
        }

        else if (layer->stretch)
        {
            /* Update stretch factors. */
            gcmONERROR(
                gco2D_SetStretchFactors(Context->engine,
                                        (gctINT32) (layer->hfactor * 65536),
                                        (gctINT32) (layer->vfactor * 65536)));
            /* StretchBlit. */
            gcmONERROR(
                gco2D_StretchBlit(Context->engine,
                                  1U,
                                  ClipRect,
                                  0xCC,
                                  0xCC,
                                  framebuffer->format));
#if DUMP_COMPOSE

        ALOGD("  STRETCH: layer[%d] (format=%d,alpha=%d,rot=%d,mirror=%d,%d): "
             "[%d,%d,%d,%d] (%08x) => [%d,%d,%d,%d] (%08x)",
             Index,
             layer->format,
             AlphaDim,
             layer->rotation,
             layer->hMirror,
             layer->vMirror,
             srcRect.left,
             srcRect.top,
             srcRect.right,
             srcRect.bottom,
             layer->addresses[0],
             ClipRect->left,
             ClipRect->top,
             ClipRect->right,
             ClipRect->bottom,
             framebuffer->target->physical);
#endif
        }

        else
        {
            /* Do bit blit. */
            gcmONERROR(
                gco2D_Blit(Context->engine,
                           1U,
                           ClipRect,
                           0xCC,
                           0xCC,
                           framebuffer->format));

#if DUMP_COMPOSE
        ALOGD("  BLIT: layer[%d] (format=%d,alpha=%d,rot=%d,mirror=%d,%d): "
             "[%d,%d,%d,%d] (%08x) => [%d,%d,%d,%d] (%08x)",
             Index,
             layer->format,
             AlphaDim,
             layer->rotation,
             layer->hMirror,
             layer->vMirror,
             srcRect.left,
             srcRect.top,
             srcRect.right,
             srcRect.bottom,
             layer->addresses[0],
             ClipRect->left,
             ClipRect->top,
             ClipRect->right,
             ClipRect->bottom,
             framebuffer->target->physical);
#endif
        }
    }

    else if (layer->opaque)
    {
        /* Do clear. */
        gcmONERROR(
            gco2D_Clear(Context->engine,
                        1U,
                        ClipRect,
                        layer->color32,
                        0xCC,
                        0xCC,
                        framebuffer->format));

#if DUMP_COMPOSE
        ALOGD("  CLEAR: layer[%d]: color32=0x%08x => [%d,%d,%d,%d] (%08x)",
             Index,
             layer->color32,
             ClipRect->left,
             ClipRect->top,
             ClipRect->right,
             ClipRect->bottom,
             framebuffer->target->physical);
#endif
    }

    else
    {
        /* Do bit blit. */
        gcmONERROR(
            gco2D_Blit(Context->engine,
                       1U,
                       ClipRect,
                       0xF0,
                       0xF0,
                       framebuffer->format));

#if DUMP_COMPOSE
        ALOGD("  BLIT: layer[%d]: color32=0x%08x => [%d,%d,%d,%d] (%08x)",
             Index,
             layer->color32,
             ClipRect->left,
             ClipRect->top,
             ClipRect->right,
             ClipRect->bottom,
             framebuffer->target->physical);
#endif
    }

    return gcvSTATUS_OK;

OnError:
    ALOGE("Failed in %s: status=%d", __FUNCTION__, status);
    return status;
}


/* Multi-source blit. */
gceSTATUS
_MultiSourceBlit(
    IN hwcContext * Context,
    IN gctUINT32 Indices[8],
    IN gctUINT32 AlphaDim[8],
    IN gctUINT32 Count,
    IN gcsRECT * EigenRect,
    IN gcsRECT * ClipRect
    )
{
    gceSTATUS status;

    hwcFramebuffer * framebuffer = Context->framebuffer;
    hwcBuffer * target           = framebuffer->target;

    gctUINT32 sourceMask = 0U;
    gctUINT32 sourceNum  = 0U;

    /* Blit rectangle, which is the same rectangle on source and dest.
     * This rectangle is actually clipRect against a new 'initial point' */
    gcsRECT blitRect;

    /* Surface size, which is the same size on source and dest. */
    gctUINT32 width;
    gctUINT32 height;

    /* Target physical address. */
    gctUINT32 dstAddress;

    if (EigenRect->top    < 0)    { EigenRect->top     = 0; }
    if (EigenRect->right  < 0)    { EigenRect->right   = 0; }
    if (EigenRect->bottom < 0)    { EigenRect->bottom  = 0; }


    /***************************************************************************
    ** Determine blit rectangle and surface size.
    */

    /*  Blit rectangle is the clipRect against a new 'initial point'
     *
     *    orig 'initial point'
     *   [+]--------------------------------------...
     *    | \
     *    |   \
     *    |     \  new 'initial point'
     *    |     [+]-------------------------+
     *    |      |        ^                 |
     *    |      |        T (270 deg)       |
     *    |      |        v                 |
     *    |      |     +--------------+     |
     *    |      |     | blit rect    |     |
     *    |      |<-L->|              |<-R->|
     *    |      |(0   |              |(180 |
     *    |      | deg)|              | deg)|
     *    |      |     +--------------+     |
     *    |      |        ^                 |
     *    |      |        B (90 deg)        |
     *    |      |        v                 |
     *    |      +--------------------------+
     *    .
     *    .
     *    .
     *
     * L, T, R, B: are eigenvalues represented by eigen rectangle.
     */
    blitRect.left   = EigenRect->left;
    blitRect.top    = EigenRect->top;
    blitRect.right  = EigenRect->left + (ClipRect->right  - ClipRect->left);
    blitRect.bottom = EigenRect->top  + (ClipRect->bottom - ClipRect->top);

    width           = blitRect.right  + EigenRect->right;
    height          = blitRect.bottom + EigenRect->bottom;


    /***************************************************************************
    ** Setup target.
    */

    /* Compute offset to dest 'initial point'. */
    gctINT dx = ClipRect->left - blitRect.left;
    gctINT dy = ClipRect->top  - blitRect.top;

    /* Move target physical to dest initial point. */
    dstAddress = target->physical
               + framebuffer->stride * dy
               + framebuffer->bytesPerPixel * dx;

    /* Setup target. */
    gcmONERROR(
        gco2D_SetGenericTarget(Context->engine,
                               &dstAddress,
                               1U,
                               &framebuffer->stride,
                               1U,
                               framebuffer->tiling,
                               framebuffer->format,
                               gcvSURF_0_DEGREE,
                               width,
                               height));

#if DUMP_COMPOSE

    ALOGD("  MULTI-SOURCE: eigenRect[%d,%d,%d,%d] blitRect[%d,%d,%d,%d]",
         EigenRect->left,
         EigenRect->top,
         EigenRect->right,
         EigenRect->bottom,
         blitRect.left,
         blitRect.top,
         blitRect.right,
         blitRect.bottom);

    ALOGD("   ClipRect[%d,%d,%d,%d] dx=%d,dy=%d: 0x%08x->0x%08x",
         ClipRect->left,
         ClipRect->top,
         ClipRect->right,
         ClipRect->bottom,
         dx, dy,
         framebuffer->target->physical,
         dstAddress);
#endif


    /***************************************************************************
    ** Check Alpha Blending on first source.
    */

    if (!Context->layers[Indices[0]].opaque)
    {
        /* Setup source index. */
        gcmONERROR(
            gco2D_SetCurrentSourceIndex(Context->engine, 0U));

        /* This layer is the first layer in this multi-source blit batch.
         * Hardware limitation, first layer will NOT do alpha blending with
         * target. So we need to insert target surface as the first source and
         * treat the actual first layer as the second. */
        gcmONERROR(
            gco2D_SetGenericSource(Context->engine,
                                   &dstAddress,
                                   1U,
                                   &framebuffer->stride,
                                   1U,
                                   framebuffer->tiling,
                                   framebuffer->format,
                                   gcvSURF_0_DEGREE,
                                   width,
                                   height));

        /* Setup mirror. */
        gcmONERROR(
            gco2D_SetBitBlitMirror(Context->engine,
                                   gcvFALSE,
                                   gcvFALSE));

        /* Set source rect. */
        gcmONERROR(
            gco2D_SetSource(Context->engine,
                            &blitRect));

        /* ROP. */
        gcmONERROR(
            gco2D_SetROP(Context->engine,
                         0xCC,
                         0xCC));

        /* Can never have alpha blending for this case. */
        gcmONERROR(
            gco2D_DisableAlphaBlend(Context->engine));

        gcmONERROR(
            gco2D_SetPixelMultiplyModeAdvanced(Context->engine,
                                               gcv2D_COLOR_MULTIPLY_DISABLE,
                                               gcv2D_COLOR_MULTIPLY_DISABLE,
                                               gcv2D_GLOBAL_COLOR_MULTIPLY_DISABLE,
                                               gcv2D_COLOR_MULTIPLY_DISABLE));

        /* Target source inserted at index 0. So we need to update
         * index and source count. */
        sourceNum  = 1U;
        sourceMask = 1U;

#if DUMP_COMPOSE
        ALOGD("   layer: Framebuffer target as source 0");
#endif
    }


    /***************************************************************************
    ** Setup color source(s) and Start Blit.
    */

    for (gctUINT32 j = 0; j < Count; j++)
    {
        /* Get shortcuts. */
        hwcLayer * layer = &Context->layers[Indices[j]];

        /* Setup source index. */
        gcmONERROR(
            gco2D_SetCurrentSourceIndex(Context->engine, sourceNum));

        /* Setup source. */
        if (layer->source != gcvNULL)
        {
            /* Source physical address offset. */
            gctUINT32 addresses[3];

            /* Source size. */
            gctINT srcWidth;
            gctINT srcHeight;

            /* Deltas. */
            gctINT dl;
            gctINT dt;
            gctINT dr;
            gctINT db;

            /* Compute deltas. */
            dl = layer->dstRect.left   - (ClipRect->left   - EigenRect->left);
            dt = layer->dstRect.top    - (ClipRect->top    - EigenRect->top);
            dr = layer->dstRect.right  - (ClipRect->right  + EigenRect->right);
            db = layer->dstRect.bottom - (ClipRect->bottom + EigenRect->bottom);

            if (layer->hMirror)
            {
                gctINT tl = dl;

                dl = -dr;
                dr = -tl;
            }

            if (layer->vMirror)
            {
                gctINT tt = dt;

                dt = -db;
                db = -tt;
            }

            /* Compute offset to source initial point. */
            switch (layer->rotation)
            {
            case gcvSURF_0_DEGREE:
            default:
                dx = layer->orgRect.left - dl;
                dy = layer->orgRect.top  - dt;

                srcWidth  = width;
                srcHeight = height;
                break;

            case gcvSURF_90_DEGREE:
                dx = layer->orgRect.left + db;
                dy = layer->orgRect.top  - dl;

                srcWidth  = height;
                srcHeight = width;
                break;

            case gcvSURF_180_DEGREE:
                dx = layer->orgRect.left + dr;
                dy = layer->orgRect.top  + db;

                srcWidth  = width;
                srcHeight = height;
                break;

            case gcvSURF_270_DEGREE:
                dx = layer->orgRect.left - dt;
                dy = layer->orgRect.top  + dr;

                srcWidth  = height;
                srcHeight = width;
                break;
            }

            /* Get original source physical addresses. */
            if (layer->yuv)
            {
                switch (layer->format)
                {
                case gcvSURF_YUY2:
                case gcvSURF_UYVY:
                    addresses[0] = layer->addresses[0]
                                 + layer->strides[0] * dy
                                 + 2 * dx;
                    break;

                case gcvSURF_NV12:
                case gcvSURF_NV21:
                    addresses[0] = layer->addresses[0]
                                 + layer->strides[0] * dy
                                 + 1 * dx;

                    addresses[1] = layer->addresses[1]
                                 + layer->strides[1] * dy / 2
                                 + 1 * dx;
                    break;

                case gcvSURF_NV16:
                case gcvSURF_NV61:
                    addresses[0] = layer->addresses[0]
                                 + layer->strides[0] * dy
                                 + 1 * dx;

                    addresses[1] = layer->addresses[1]
                                 + layer->strides[1] * dy
                                 + 1 * dx;
                    break;

                case gcvSURF_YV12:
                case gcvSURF_I420:
                default:
                    addresses[0] = layer->addresses[0]
                                 + layer->strides[0] * dy
                                 + 1 * dx;

                    addresses[1] = layer->addresses[1]
                                 + layer->strides[1] * dy / 2
                                 + 1 * dx / 2;

                    addresses[2] = layer->addresses[2]
                                 + layer->strides[2] * dy / 2
                                 + 1 * dx / 2;
                    break;
                }
            }

            else
            {
                /* RGB surfaces will have only one plane. */
                addresses[0] = layer->addresses[0]
                             + layer->strides[0] * dy
                             + layer->bytesPerPixel * dx;
            }

#if DUMP_COMPOSE

            ALOGD("   layer[%d]: (format=%d,alpha=%d,rot=%d,mirror=%d,%d) dx=%d,dy=%d: %08x->%08x",
                 Indices[j],
                 layer->format,
                 AlphaDim[j],
                 layer->rotation,
                 layer->hMirror,
                 layer->vMirror,
                 dx, dy,
                 layer->addresses[0],
                 addresses[0]);
#endif

            /* Setup source. */
            gcmONERROR(
                gco2D_SetGenericSource(Context->engine,
                                       addresses,
                                       layer->addressNum,
                                       layer->strides,
                                       layer->strideNum,
                                       layer->tiling,
                                       layer->format,
                                       layer->rotation,
                                       srcWidth,
                                       srcHeight));

            /* Setup mirror. */
            gcmONERROR(
                gco2D_SetBitBlitMirror(Context->engine,
                                       layer->hMirror,
                                       layer->vMirror));


            /* Set source rect (equal to dstRect). */
            gcmONERROR(
                gco2D_SetSource(Context->engine,
                                &blitRect));

            /* Set ROP. */
            gcmONERROR(
                gco2D_SetROP(Context->engine,
                             0xCC,
                             0xCC));
        }

        else
        {
            /* Color source is still needed if uses patthen only. We set dummy
             * color source to framebuffer target. */
            gcmONERROR(
                gco2D_SetGenericSource(Context->engine,
                                       &dstAddress,
                                       1U,
                                       &framebuffer->stride,
                                       1U,
                                       framebuffer->tiling,
                                       framebuffer->format,
                                       gcvSURF_0_DEGREE,
                                       width,
                                       height));

            /* Setup mirror. */
            gcmONERROR(
                gco2D_SetBitBlitMirror(Context->engine,
                                       gcvFALSE,
                                       gcvFALSE));

            /* Set source rect (equal to dstRect). */
            gcmONERROR(
                gco2D_SetSource(Context->engine,
                                &blitRect));

            gcmONERROR(
                gco2D_LoadSolidBrush(Context->engine,
                                     /* This should not be taken. */
                                     gcvSURF_UNKNOWN,
                                     gcvTRUE,
                                     layer->color32,
                                     0U));

            /* Set ROP: use pattern only. */
            gcmONERROR(
                gco2D_SetROP(Context->engine,
                             0xF0,
                             0xF0));
#if DUMP_COMPOSE

            ALOGD("   layer[%d]: color32=0x%08x",
                 Indices[j], layer->color32);
#endif
        }

        /* Setup alpha blending. */
        if (layer->opaque)
        {
            /* Disable alpha blending. */
            gcmONERROR(
                gco2D_DisableAlphaBlend(Context->engine));
        }

        else
        {
            gcmONERROR(
                gco2D_EnableAlphaBlendAdvanced(Context->engine,
                                               layer->srcAlphaMode,
                                               layer->dstAlphaMode,
                                               layer->srcGlobalAlphaMode,
                                               layer->dstGlobalAlphaMode,
                                               layer->srcFactorMode,
                                               layer->dstFactorMode));
        }

        /* Setup premultiply. */
        if (AlphaDim[j] < 0xFF)
        {
            gctUINT32 srcGlobalAlpha = (layer->srcGlobalAlpha >> 8) * AlphaDim[j];
            gctUINT32 dstGlobalAlpha = (layer->dstGlobalAlpha >> 8) * AlphaDim[j];

            srcGlobalAlpha &= 0xFF000000;
            dstGlobalAlpha &= 0xFF000000;

            /* Dim optimization. */
            gcmONERROR(
                gco2D_SetPixelMultiplyModeAdvanced(Context->engine,
                                                   layer->srcPremultSrcAlpha,
                                                   layer->dstPremultDstAlpha,
                                                   gcv2D_GLOBAL_COLOR_MULTIPLY_ALPHA,
                                                   layer->dstDemultDstAlpha));

            gcmONERROR(
                gco2D_SetSourceGlobalColorAdvanced(Context->engine,
                                                   srcGlobalAlpha));

            gcmONERROR(
                gco2D_SetTargetGlobalColorAdvanced(Context->engine,
                                                   dstGlobalAlpha));
        }

        else
        {
            gcmONERROR(
                gco2D_SetPixelMultiplyModeAdvanced(Context->engine,
                                                   layer->srcPremultSrcAlpha,
                                                   layer->dstPremultDstAlpha,
                                                   layer->srcPremultGlobalMode,
                                                   layer->dstDemultDstAlpha));

            gcmONERROR(
                gco2D_SetSourceGlobalColorAdvanced(Context->engine,
                                                   layer->srcGlobalAlpha));

            gcmONERROR(
                gco2D_SetTargetGlobalColorAdvanced(Context->engine,
                                                   layer->dstGlobalAlpha));
        }

        /* Append mask to sourceMask. */
        sourceMask = ((sourceMask << 1U) | 1U);
        sourceNum++;
    }


    /***************************************************************************
    ** Trigger Multi-Source Blit.
    */

    gcmONERROR(
        gco2D_MultiSourceBlit(Context->engine,
                              sourceMask,
                              &blitRect,
                              1U));

    return gcvSTATUS_OK;

OnError:
    ALOGE("Failed in %s: status=%d", __FUNCTION__, status);
    return status;
}

