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




/*
 * VDK API code.
 */

#include "gc_egl_precomp.h"
#include "gc_hal_eglplatform.h"

/*******************************************************************************
***** Version Signature *******************************************************/


NativeDisplayType
veglGetDefaultDisplay(
    void
    )
{
    NativeDisplayType display = (NativeDisplayType) gcvNULL;
    gcoOS_GetDisplay((HALNativeDisplayType*)(&display));
    return display;
}

void
veglReleaseDefaultDisplay(
    IN NativeDisplayType Display
    )
{
    gcoOS_DestroyDisplay(Display);
}

gctBOOL
veglGetDisplayInfo(
    IN VEGLDisplay Display,
    IN NativeWindowType Window,
    OUT VEGLDisplayInfo Info
    )
{
    halDISPLAY_INFO info;

    gcmASSERT(Display != gcvNULL);
    gcmASSERT(Info != gcvNULL);

    /* Get display information. */
    if (gcmIS_ERROR(gcoOS_GetDisplayInfoEx2(
        Display->hdc,
        Window,
        Display->localInfo,
        sizeof(info),
        &info)))
    {
        /* Error. */
        return gcvFALSE;
    }


    /* Return window addresses. */
    Info->logicals[0]  = info.logical;
    Info->physicals[0] = info.physical;
    Info->stride   = info.stride;

    /* Return flip support. */
    Info->flip = info.flip;
    Info->width  = info.width;
    Info->height = info.height;

#ifdef __QNXNTO__
    if (gcmIS_ERROR(gcoOS_GetNextDisplayInfoEx(
        Display->hdc,
        Window,
        sizeof(info),
        &info)))
    {
        return gcvFALSE;
    }

    Info->logical2  = info.logical;
    Info->physical2 = info.physical;
#else
    /* Get virtual display size. */
    if (gcmIS_ERROR(gcoOS_GetDisplayVirtual(Display->hdc, &Info->width, &Info->height)))
    {
        Info->width  = -1;
        Info->height = -1;
    }

    /* 355_FB_MULTI_BUFFER */
    Info->multiBuffer = info.multiBuffer;
#endif

    Info->isCompositor = info.isCompositor;

    /* Success. */
    return gcvTRUE;
}

gctBOOL
veglGetWindowInfo(
    IN VEGLDisplay Display,
    IN NativeWindowType Window,
    OUT gctINT_PTR X,
    OUT gctINT_PTR Y,
    OUT gctUINT_PTR Width,
    OUT gctUINT_PTR Height,
    OUT gctUINT_PTR BitsPerPixel,
    OUT gceSURF_FORMAT * Format
    )
{
    if(gcmIS_ERROR(gcoOS_GetWindowInfoEx(Display->hdc,
                                         Window,
                                         (gctINT_PTR)X,
                                         (gctINT_PTR)Y,
                                         (gctINT_PTR)Width,
                                         (gctINT_PTR)Height,
                                         (gctINT_PTR)BitsPerPixel,
                                         gcvNULL, Format)))
        return gcvFALSE;
    return gcvTRUE;
}

gctBOOL
veglDrawImage(
    IN VEGLDisplay Display,
    IN VEGLSurface Surface,
    IN gctPOINTER Bits,
    IN gctINT Left,
    IN gctINT Top,
    IN gctINT Width,
    IN gctINT Height
    )
{
    gctINT width, height;
    gceORIENTATION orientation;

    width  = Surface->bitsAlignedWidth;
    height = Surface->bitsAlignedHeight;

    gcmVERIFY_OK(gcoSURF_QueryOrientation(
        Surface->swapSurface, &orientation
        ));

    if (orientation == gcvORIENTATION_BOTTOM_TOP)
    {
        height = -height;
    }

    if(gcmIS_ERROR(gcoOS_DrawImageEx(
        Display->hdc,
        Surface->hwnd,
        Left, Top, Left + Width, Top + Height,
        width, height,
        Surface->swapBitsPerPixel,
        Bits,
        Surface->resolveFormat
        )))
        return gcvFALSE;
    else
        return gcvTRUE;
}



gctBOOL
veglGetDisplayBackBuffer(
    IN VEGLDisplay Display,
    IN NativeWindowType Window,
    OUT VEGLBackBuffer BackBuffer,
    OUT gctBOOL_PTR Flip
    )
{
    BackBuffer->context = gcvNULL;
    BackBuffer->surface = gcvNULL;
    if(gcmIS_ERROR(gcoOS_GetDisplayBackbufferEx(Display->hdc,
                                    Window,
                                    Display->localInfo,
                                    &BackBuffer->context,
                                    &BackBuffer->surface,
                                    &BackBuffer->offset,
                                    &BackBuffer->origin.x,
                                    &BackBuffer->origin.y)))
        *Flip = gcvFALSE;
    else
        *Flip = gcvTRUE;

    return gcvTRUE;
}

gctBOOL
veglSetDisplayFlip(
    IN NativeDisplayType Display,
    IN NativeWindowType Window,
    IN VEGLBackBuffer BackBuffer
    )
{
    gctUINT offset = 0;
    gctINT x = 0, y = 0;
    gceSTATUS status;

    if (BackBuffer != gcvNULL)
    {
        offset = BackBuffer->offset;
        x = BackBuffer->origin.x;
        y = BackBuffer->origin.y;
    }

    status = gcoOS_SetDisplayVirtual(Display, Window, offset, x, y);

    if(gcmIS_ERROR(status))
        return gcvFALSE;
    else
        return gcvTRUE;
}

gctBOOL
veglSetDisplayFlipRegions(
    IN NativeDisplayType Display,
    IN NativeWindowType Window,
    IN VEGLBackBuffer BackBuffer,
    IN gctINT NumRects,
    IN gctINT_PTR Rects
    )
{
#ifndef __QNXNTO__
    return veglSetDisplayFlip(Display, Window, BackBuffer);
#else
    gceSTATUS status = gcoOS_DisplayBufferRegions(Display, Window, NumRects, Rects);
    if(gcmIS_ERROR(status))
        return gcvFALSE;
    else
        return gcvTRUE;
#endif
}

gctBOOL
veglInitLocalDisplayInfo(
    VEGLDisplay Display
    )
{
    gceSTATUS status = gcoOS_InitLocalDisplayInfo(&Display->localInfo);
    if(gcmIS_ERROR(status))
        return gcvFALSE;
    else
        return gcvTRUE;
}

gctBOOL
veglDeinitLocalDisplayInfo(
    VEGLDisplay Display
    )
{
    gceSTATUS status = gcoOS_DeinitLocalDisplayInfo(&Display->localInfo);
    if(gcmIS_ERROR(status))
        return gcvFALSE;
    else
        return gcvTRUE;
}


gctBOOL
veglDrawPixmap(
    IN VEGLDisplay Display,
    IN EGLNativePixmapType Pixmap,
    IN gctINT Left,
    IN gctINT Top,
    IN gctINT Right,
    IN gctINT Bottom,
    IN gctINT Width,
    IN gctINT Height,
    IN gctINT BitsPerPixel,
    IN gctPOINTER Bits
    )
{
    gceSTATUS status = gcvSTATUS_OK;
    status = gcoOS_DrawPixmap(
                    Display->hdc,
                    Pixmap,
                    Left, Top, Right, Bottom,
                    Width, Height,
                    BitsPerPixel, Bits
                    );
    if(gcmIS_ERROR(status))
        return gcvFALSE;
    else
        return gcvTRUE;
}

EGLint
veglGetNativeVisualId(
    IN VEGLDisplay dpy,
    IN VEGLConfig Config
    )
{
    gctINT visualId = 0;
    gcoOS_GetNativeVisualId(dpy->hdc, &visualId);
    return visualId;
}

gctBOOL
veglGetPixmapInfo(
    IN NativeDisplayType Display,
    IN NativePixmapType Pixmap,
    OUT gctUINT_PTR Width,
    OUT gctUINT_PTR Height,
    OUT gctUINT_PTR BitsPerPixel,
    OUT gceSURF_FORMAT * Format
    )
{
    if (gcmIS_ERROR(gcoOS_GetPixmapInfoEx(Display,
                                          Pixmap,
                                         (gctINT_PTR)Width,
                                         (gctINT_PTR)Height,
                                         (gctINT_PTR)BitsPerPixel,
                                         gcvNULL, gcvNULL, Format)))
    {
        return gcvFALSE;
    }

    /* Success. */
    return gcvTRUE;
}

gctBOOL
veglGetPixmapBits(
    IN NativeDisplayType Display,
    IN NativePixmapType Pixmap,
    OUT gctPOINTER * Bits,
    OUT gctINT_PTR Stride,
    OUT gctUINT32_PTR Physical
    )
{
    /* Query pixmap bits. */
    if (gcmIS_ERROR(gcoOS_GetPixmapInfo(
                        Display,
                        Pixmap,
                        gcvNULL,
                        gcvNULL,
                        gcvNULL,
                        Stride,
                        Bits)))
    {
        return gcvFALSE;
    }

    if (Physical != gcvNULL)
    {
        /* No physical address for pixmaps. */
        *Physical = ~0U;
    }

    /* Success. */
    return gcvTRUE;
}

gctBOOL
veglGetWindowBits(
    IN NativeDisplayType Display,
    IN NativeWindowType Window,
    OUT gctPOINTER * Logical,
    OUT gctUINT32_PTR Physical,
    OUT gctINT32_PTR Stride
    )
{
    halDISPLAY_INFO info;
    gctUINT offset;

    /* Get display information. */
    if (gcmIS_ERROR(gcoOS_GetDisplayInfoEx(
                        Display,
                        Window,
                        sizeof(info),
                        &info)))
    {
        /* Error. */
        return gcvFALSE;
    }

    /* Get window information. */
    if (gcmIS_ERROR(gcoOS_GetWindowInfo(
                        Display,
                        Window,
                        gcvNULL,
                        gcvNULL,
                        gcvNULL,
                        gcvNULL,
                        gcvNULL,
                        &offset)))
    {
        /* Error. */
        return gcvFALSE;
    }

    if ((offset == ~0U) || (info.logical == gcvNULL) || (info.physical == ~0U))
    {
        /* No offset. */
        return gcvFALSE;
    }

    /* Compute window addresses. */
    *Logical  = (gctUINT8_PTR) info.logical + offset;
    *Physical = info.physical + offset;
    *Stride   = info.stride;

    /* Success. */
    return gcvTRUE;
}

gctBOOL
veglCopyPixmapBits(
    IN VEGLDisplay Display,
    IN NativePixmapType Pixmap,
    IN gctUINT DstWidth,
    IN gctUINT DstHeight,
    IN gctINT DstStride,
    IN gceSURF_FORMAT DstFormat,
    OUT gctPOINTER DstBits
    )
{
    if(gcmIS_ERROR(gcoOS_CopyPixmapBits(Display->hdc,
                                        Pixmap,
                                        DstWidth,
                                        DstHeight,
                                        DstStride,
                                        DstFormat,
                                        DstBits)))
        return gcvFALSE;
    return gcvTRUE;
}



gctBOOL
veglIsColorFormatSupport(
    IN NativeDisplayType Display,
    IN VEGLConfig       Config
    )
{
    return gcvTRUE;
}

gctBOOL
veglIsValidDisplay(
    IN NativeDisplayType Display
    )
{
    if(gcmIS_ERROR(gcoOS_IsValidDisplay((HALNativeDisplayType)Display)))
    {
        return gcvFALSE;
    }

    return gcvTRUE;
}

#if defined(EGL_API_WL)

struct wl_egl_window *
wl_egl_window_create(struct wl_surface *surface, int width, int height)
{
    struct wl_egl_window* window = gcvNULL;
    gctUINT i;
    gceSTATUS status = gcvSTATUS_OK;

    gcmASSERT(surface);

    do
    {
        gcmONERROR(gcoOS_AllocateMemory(gcvNULL, sizeof *window, (gctPOINTER) &window));

        gcmONERROR(gcoOS_ZeroMemory(window, sizeof *window));

        window->surface     = surface;
        window->info.width  = width;
        window->info.height = height;
        window->info.format = gcvSURF_R5G6B5;
        window->info.bpp    = 16;

        for (i=0; i<WL_EGL_NUM_BACKBUFFERS ; i++)
        {
            gcmONERROR(gcoSURF_Construct(gcvNULL, width, height, 1, gcvSURF_BITMAP, window->info.format, gcvPOOL_DEFAULT, &window->backbuffers[i].info.surface));
        }
    }
    while (gcvFALSE);

    return window;

OnError:
    wl_egl_window_destroy(window);
    return gcvNULL;
}

void
wl_egl_window_destroy(struct wl_egl_window *window)
{
    gctUINT i;

    if (window != gcvNULL)
    {
        for (i=0; i<WL_EGL_NUM_BACKBUFFERS ; i++)
        {
            if (window->backbuffers[i].info.surface != gcvNULL)
            {
                gcoSURF_Destroy(window->backbuffers[i].info.surface);
            }
        }

        gcoOS_FreeMemory(gcvNULL, window);
    }
}
#endif

#if defined(LINUX) && defined(EGL_API_FB) && !defined(__APPLE__)

NativeDisplayType
fbGetDisplay(
    void
    )
{
    NativeDisplayType display = gcvNULL;
    gcoOS_GetDisplay((HALNativeDisplayType*)(&display));
    return display;
}

EGLNativeDisplayType
fbGetDisplayByIndex(
    IN gctINT DisplayIndex
    )
{
    NativeDisplayType display = gcvNULL;
    gcoOS_GetDisplayByIndex(DisplayIndex, (HALNativeDisplayType*)(&display));
    return display;
}

void
fbGetDisplayGeometry(
    IN NativeDisplayType Display,
    OUT gctINT_PTR Width,
    OUT gctINT_PTR Height
    )
{
    gcoOS_GetDisplayInfo((HALNativeDisplayType)Display, Width, Height, gcvNULL, gcvNULL, gcvNULL);
}

void
fbDestroyDisplay(
    IN NativeDisplayType Display
    )
{
    gcoOS_DestroyDisplay((HALNativeDisplayType)Display);
}

NativeWindowType
fbCreateWindow(
    IN NativeDisplayType Display,
    IN gctINT X,
    IN gctINT Y,
    IN gctINT Width,
    IN gctINT Height
    )
{
    NativeWindowType Window = gcvNULL;
    gcoOS_CreateWindow((HALNativeDisplayType)Display, X, Y, Width, Height, (HALNativeWindowType*)(&Window));
    return Window;
}

void
fbGetWindowGeometry(
    IN NativeWindowType Window,
    OUT gctINT_PTR X,
    OUT gctINT_PTR Y,
    OUT gctINT_PTR Width,
    OUT gctINT_PTR Height
    )
{
    gcoOS_GetWindowInfo(gcvNULL, (HALNativeWindowType)Window, X, Y, Width, Height, gcvNULL, gcvNULL);
}

void
fbDestroyWindow(
    IN NativeWindowType Window
    )
{
    gcoOS_DestroyWindow(gcvNULL, (HALNativeWindowType)Window);
}

NativePixmapType
fbCreatePixmap(
    IN NativeDisplayType Display,
    IN gctINT Width,
    IN gctINT Height
    )
{
    NativePixmapType Pixmap = gcvNULL;
    gcoOS_CreatePixmap((HALNativeDisplayType)Display, Width, Height, 32, (HALNativePixmapType*)(&Pixmap));
    return Pixmap;
}

NativePixmapType
fbCreatePixmapWithBpp(
    IN NativeDisplayType Display,
    IN gctINT Width,
    IN gctINT Height,
    IN gctINT BitsPerPixel
    )
{
    NativePixmapType Pixmap = gcvNULL;
    gcoOS_CreatePixmap((HALNativeDisplayType)Display, Width, Height, BitsPerPixel, (HALNativePixmapType*)(&Pixmap));
    return Pixmap;
}

void
fbGetPixmapGeometry(
    IN NativePixmapType Pixmap,
    OUT gctINT_PTR Width,
    OUT gctINT_PTR Height
    )
{
    gcoOS_GetPixmapInfo(gcvNULL, (HALNativePixmapType)Pixmap, Width, Height, gcvNULL, gcvNULL, gcvNULL);
}

void
fbDestroyPixmap(
    IN NativePixmapType Pixmap
    )
{
    gcoOS_DestroyPixmap(gcvNULL, (HALNativePixmapType)Pixmap);
}

#endif
