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


#ifndef _gc_glsh_patch
#define _gc_glsh_patch

#include "gc_glsh_batch.h"

typedef struct glsPATCH_FLAGS
{
    gctUINT clearStencil    : 1;
    gctUINT disableEZ       : 1;
    gctUINT stack           : 1;
    gctUINT blurDepth       : 1;
    gctUINT uiDepth         : 1;
    gctUINT alphaKill       : 1;
    gctUINT ui              : 1;
    gctUINT batching        : 1;
    gctUINT depthScale      : 1;
}
glsPATCH_FLAGS;

typedef struct glsPATCH_BATCH
{
    struct glsPATCH_BATCH * next;

    GLenum                  mode;
    GLsizei                 count;
    GLenum                  type;
    const GLvoid *          indices;

    GLBuffer                vertexBuffer;
    GLBuffer                elementBuffer;

#if gldUSE_VERTEX_ARRAY
    gcsVERTEXARRAY          vertexArray[gldVERTEX_ELEMENT_COUNT];
#else
    GLAttribute             vertexArray[gldVERTEX_ELEMENT_COUNT];
#endif

    GLProgram               program;
    gctPOINTER              uniformData;

    GLTexture               textures2D[8];
    GLTexture               texturesCube[8];
}
glsPATCH_BATCH;

typedef struct glsPATCH_INFO
{
    /* Patch flags. */
    glsPATCH_FLAGS          patchFlags;

    /* Program handle for cleanup. */
    GLProgram               patchCleanupProgram;

    /* Allow early Z. */
    gctBOOL                 allowEZ;

    /* UI counter. */
    gctINT                  uiCount;

    /* UI surface. */
    gcoSURF                 uiSurface;

    /* Depth buffer. */
    gcoSURF                 uiDepth;

    /* Save read buffer. */
    gcoSURF                 uiRead;

    /* Save batches on stack. */
    gctBOOL                 stackSave;

    /* Top of stack pointer. */
    glsPATCH_BATCH *        stackPtr;

    /* List of free batches. */
    glsPATCH_BATCH *        stackFreeList;

    /* Horizontal blur program. */
    GLProgram               blurProgram;
}
glsPATCH_INFO;

void glshPatchLink(IN GLContext Context, IN GLProgram Program);

GLbitfield glshPatchClear(IN GLContext Context, IN GLbitfield Mask);

gctBOOL glshPatchDraw(IN GLContext Context, IN GLenum Mode, IN GLsizei Count,
                      IN GLenum Type, IN const GLvoid * Indices);

gctBOOL glshQueryPatchEZ(IN GLContext Context);

gctBOOL glshQueryPatchAlphaKill(IN GLContext Context);

void glshPatchBlend(IN GLContext Context, IN gctBOOL Enable);

void glshPatchDeleteProgram(IN GLContext Context, IN GLProgram Program);

#endif /* _gc_glsh_patch */
