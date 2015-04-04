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


#include "gc_glsh_precomp.h"

static void
_SetCulling(
    GLContext Context
    )
{
    if (Context->cullEnable)
    {
        if ((  (Context->cullMode  == GL_FRONT)
            && (Context->cullFront == GL_CCW)
            )
        ||  (   (Context->cullMode  == GL_BACK)
             && (Context->cullFront == GL_CW)
             )
        )
        {
            gcmVERIFY_OK(
                gco3D_SetCulling(Context->engine, gcvCULL_CW));
        }
        else
        {
            gcmVERIFY_OK(
                gco3D_SetCulling(Context->engine, gcvCULL_CCW));
        }
    }
    else
    {
        gcmVERIFY_OK(
            gco3D_SetCulling(Context->engine, gcvCULL_NONE));
    }
}

gceCOMPARE
_glshTranslateFunc(
    GLenum Func
    )
{
    switch (Func)
    {
    case GL_NEVER:
        return gcvCOMPARE_NEVER;

    case GL_ALWAYS:
        return gcvCOMPARE_ALWAYS;

    case GL_LESS:
        return gcvCOMPARE_LESS;

    case GL_LEQUAL:
        return gcvCOMPARE_LESS_OR_EQUAL;

    case GL_EQUAL:
        return gcvCOMPARE_EQUAL;

    case GL_GREATER:
        return gcvCOMPARE_GREATER;

    case GL_GEQUAL:
        return gcvCOMPARE_GREATER_OR_EQUAL;

    case GL_NOTEQUAL:
        return gcvCOMPARE_NOT_EQUAL;
    }

    return gcvCOMPARE_INVALID;
}

static gceSTENCIL_OPERATION
_glshTranslateOperation(
    GLenum Operation
    )
{
    switch (Operation)
    {
    case GL_KEEP:
        return gcvSTENCIL_KEEP;

    case GL_ZERO:
        return gcvSTENCIL_ZERO;

    case GL_REPLACE:
        return gcvSTENCIL_REPLACE;

    case GL_INCR:
        return gcvSTENCIL_INCREMENT_SATURATE;

    case GL_DECR:
        return gcvSTENCIL_DECREMENT_SATURATE;

    case GL_INVERT:
        return gcvSTENCIL_INVERT;

    case GL_INCR_WRAP:
        return gcvSTENCIL_INCREMENT;

    case GL_DECR_WRAP:
        return gcvSTENCIL_DECREMENT;
    }

    return gcvSTENCIL_OPERATION_INVALID;
}

static gceSTATUS
_glshTranslateBlendFunc(
    IN GLenum BlendFuncGL,
    OUT gceBLEND_FUNCTION * BlendFunc
    )
{
    switch (BlendFuncGL)
    {
    case GL_ZERO:
        *BlendFunc = gcvBLEND_ZERO;
        break;

    case GL_ONE:
        *BlendFunc = gcvBLEND_ONE;
        break;

    case GL_SRC_COLOR:
        *BlendFunc = gcvBLEND_SOURCE_COLOR;
        break;

    case GL_ONE_MINUS_SRC_COLOR:
        *BlendFunc = gcvBLEND_INV_SOURCE_COLOR;
        break;

    case GL_DST_COLOR:
        *BlendFunc = gcvBLEND_TARGET_COLOR;
        break;

    case GL_ONE_MINUS_DST_COLOR:
        *BlendFunc = gcvBLEND_INV_TARGET_COLOR;
        break;

    case GL_SRC_ALPHA:
        *BlendFunc = gcvBLEND_SOURCE_ALPHA;
        break;

    case GL_ONE_MINUS_SRC_ALPHA:
        *BlendFunc = gcvBLEND_INV_SOURCE_ALPHA;
        break;

    case GL_DST_ALPHA:
        *BlendFunc = gcvBLEND_TARGET_ALPHA;
        break;

    case GL_ONE_MINUS_DST_ALPHA:
        *BlendFunc = gcvBLEND_INV_TARGET_ALPHA;
        break;

    case GL_CONSTANT_COLOR:
        *BlendFunc = gcvBLEND_CONST_COLOR;
        break;

    case GL_ONE_MINUS_CONSTANT_COLOR:
        *BlendFunc = gcvBLEND_INV_CONST_COLOR;
        break;

    case GL_CONSTANT_ALPHA:
        *BlendFunc = gcvBLEND_CONST_ALPHA;
        break;

    case GL_ONE_MINUS_CONSTANT_ALPHA:
        *BlendFunc = gcvBLEND_INV_CONST_ALPHA;
        break;

    case GL_SRC_ALPHA_SATURATE:
        *BlendFunc = gcvBLEND_SOURCE_ALPHA_SATURATE;
        break;

    default:
        gcmTRACE(gcvLEVEL_WARNING,
                 "%s: unknown enum 0x%04X",
                 __FUNCTION__, BlendFuncGL);
        return gcvSTATUS_INVALID_ARGUMENT;
    }

    return gcvSTATUS_OK;
}

static gceSTATUS
_glshTranslateBlendMode(
    IN GLenum BlendModeGL,
    OUT gceBLEND_MODE * BlendMode
    )
{
    switch (BlendModeGL)
    {
    case GL_FUNC_ADD:
        *BlendMode = gcvBLEND_ADD;
        break;

    case GL_FUNC_SUBTRACT:
        *BlendMode = gcvBLEND_SUBTRACT;
        break;

    case GL_FUNC_REVERSE_SUBTRACT:
        *BlendMode = gcvBLEND_REVERSE_SUBTRACT;
        break;

    case GL_MIN_EXT:
        *BlendMode = gcvBLEND_MIN;
        break;

    case GL_MAX_EXT:
        *BlendMode = gcvBLEND_MAX;
        break;

    default:
        gcmTRACE(gcvLEVEL_WARNING,
                 "%s: unknown enum 0x%04X",
                 __FUNCTION__, BlendModeGL);
        return gcvSTATUS_INVALID_ARGUMENT;
    }

    return gcvSTATUS_OK;
}

static gceSTATUS
_SetLineWidth(
    GLContext Context,
    GLfloat LineWidth
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Context=0x%x LineWidth=%f", Context, LineWidth);

    do
    {
        /* Validate the width. */
        if (LineWidth <= 0.0f)
        {
            gcmFOOTER_ARG("LineWidth=%f", LineWidth);
            return gcvSTATUS_INVALID_ARGUMENT;
        }

        /* Make sure the width is within the supported range. */
        if (LineWidth < Context->lineWidthRange[0])
        {
            LineWidth = Context->lineWidthRange[0];
        }

        if (LineWidth > Context->lineWidthRange[1])
        {
            LineWidth = Context->lineWidthRange[1];
        }

        /* Set the line width for the hardware. */
        gcmONERROR(
            gco3D_SetAALineWidth(Context->engine,
                                 gcoMATH_Floor(LineWidth + 0.5f)));
    }
    while (gcvFALSE);

OnError:
    gcmFOOTER();
    return status;
}

/* Same idea as _glshInitializeState, but with a context that already has valid state. */
void
_glshInitializeObjectStates(
    GLContext Context
    )
{
    gceSTATUS status;
    gceBLEND_MODE rgbMode = gcvBLEND_ADD, aMode = gcvBLEND_ADD;
    gceBLEND_FUNCTION rgbFunc = gcvBLEND_ZERO, aFunc = gcvBLEND_ZERO;
    GLfloat depthBias;

    /* Initialize states. */
    gcmVERIFY_OK(
        gco3D_EnableBlending(Context->engine, Context->blendEnable));

    _glshTranslateBlendFunc(Context->blendFuncSourceRGB,   &rgbFunc);
    _glshTranslateBlendFunc(Context->blendFuncSourceAlpha, &aFunc);
    gcmVERIFY_OK(gco3D_SetBlendFunction(Context->engine,
                                        gcvBLEND_SOURCE,
                                        rgbFunc,
                                        aFunc));

    _glshTranslateBlendFunc(Context->blendFuncTargetRGB,   &rgbFunc);
    _glshTranslateBlendFunc(Context->blendFuncTargetAlpha, &aFunc);
    gcmVERIFY_OK(gco3D_SetBlendFunction(Context->engine,
                                        gcvBLEND_TARGET,
                                        rgbFunc,
                                        aFunc));

    _glshTranslateBlendMode(Context->blendModeRGB,   &rgbMode);
    _glshTranslateBlendMode(Context->blendModeAlpha, &aMode);
    gcmVERIFY_OK(
        gco3D_SetBlendMode(Context->engine, rgbMode, aMode));

    gcmVERIFY_OK(
        gco3D_SetBlendColorF(Context->engine,
                             Context->blendColorRed,
                             Context->blendColorGreen,
                             Context->blendColorBlue,
                             Context->blendColorAlpha));

    _SetCulling(Context);

    gcmVERIFY_OK(
        gco3D_EnableDepthWrite(Context->engine, Context->depthMask));

    gcmVERIFY_OK(
        gco3D_SetClearDepthF(Context->engine, Context->clearDepth));

    Context->depthDirty  = GL_TRUE;

    gco3D_EnableDither(
        Context->engine, Context->ditherEnable);

	Context->viewDirty   = GL_TRUE;

    gcmONERROR(
        gco3D_SetStencilReference(Context->engine,
                                  (gctUINT8) Context->stencilRefFront, gcvTRUE));
    gcmONERROR(
        gco3D_SetStencilReference(Context->engine,
                                  (gctUINT8) Context->stencilRefFront, gcvFALSE));

    gcmVERIFY_OK(
        gco3D_SetStencilCompare(Context->engine,
                                gcvSTENCIL_FRONT,
                                _glshTranslateFunc(Context->stencilFuncFront)));
    gcmVERIFY_OK(
        gco3D_SetStencilCompare(Context->engine,
                                gcvSTENCIL_BACK,
                                _glshTranslateFunc(Context->stencilFuncBack)));
    gcmONERROR(
        gco3D_SetStencilMask(Context->engine,
                             (gctUINT8) Context->stencilMaskFront));
    gcmONERROR(
        gco3D_SetStencilWriteMask(Context->engine,
                                  (gctUINT8) Context->stencilWriteMask));
    gcmVERIFY_OK(
        gco3D_SetStencilFail(Context->engine,
                             gcvSTENCIL_FRONT,
                             Context->frontFail =
                                _glshTranslateOperation(
                                    Context->stencilOpFailFront)));
    gcmVERIFY_OK(
        gco3D_SetStencilFail(Context->engine,
                             gcvSTENCIL_BACK,
                             Context->backFail =
                                 _glshTranslateOperation(
                                     Context->stencilOpFailBack)));
    gcmVERIFY_OK(
        gco3D_SetStencilDepthFail(Context->engine,
                                  gcvSTENCIL_FRONT,
                                  Context->frontZFail =
                                      _glshTranslateOperation(
                                          Context->stencilOpDepthFailFront)));
    gcmVERIFY_OK(
        gco3D_SetStencilDepthFail(Context->engine,
                                  gcvSTENCIL_BACK,
                                  Context->backZFail =
                                      _glshTranslateOperation(
                                          Context->stencilOpDepthFailBack)));
    gcmVERIFY_OK(
        gco3D_SetStencilPass(Context->engine,
                             gcvSTENCIL_FRONT,
                             Context->frontZPass =
                                 _glshTranslateOperation(
                                     Context->stencilOpDepthPassFront)));
    gcmVERIFY_OK(
        gco3D_SetStencilPass(Context->engine,
                             gcvSTENCIL_BACK,
                             Context->backZPass =
                                 _glshTranslateOperation(
                                     Context->stencilOpDepthPassBack)));

    gcmVERIFY_OK(
        gco3D_SetColorWrite(Context->engine, Context->colorWrite));

    depthBias = gcoMATH_Divide(Context->offsetUnits, 65535.f);
    gcmONERROR(
        gco3D_SetDepthScaleBiasF(Context->engine,
                                 Context->offsetFactor,
                                 depthBias));

    gcmVERIFY_OK(
        _SetLineWidth(Context, Context->lineWidth));

OnError:
    return;
}

void
_glshInitializeState(
    GLContext Context
    )
{
    gceSTATUS status;
    gceBLEND_MODE rgbMode = gcvBLEND_ADD, aMode;
    gceBLEND_FUNCTION rgbFunc = gcvBLEND_ZERO, aFunc = gcvBLEND_ZERO;

    gcmHEADER_ARG("Context=0x%x", Context);

    /* Initialize states. */
    gcmVERIFY_OK(
        gco3D_SetLastPixelEnable(Context->engine, gcvFALSE));

    Context->blendEnable = GL_FALSE;
    gcmVERIFY_OK(
        gco3D_EnableBlending(Context->engine, Context->blendEnable));

    Context->blendFuncSourceRGB   = GL_ONE;
    Context->blendFuncSourceAlpha = GL_ONE;
    _glshTranslateBlendFunc(Context->blendFuncSourceRGB,   &rgbFunc);
    _glshTranslateBlendFunc(Context->blendFuncSourceAlpha, &aFunc);
    gcmVERIFY_OK(gco3D_SetBlendFunction(Context->engine,
                                        gcvBLEND_SOURCE,
                                        rgbFunc,
                                        aFunc));

    Context->blendFuncTargetRGB   = GL_ZERO;
    Context->blendFuncTargetAlpha = GL_ZERO;
    _glshTranslateBlendFunc(Context->blendFuncTargetRGB,   &rgbFunc);
    _glshTranslateBlendFunc(Context->blendFuncTargetAlpha, &aFunc);
    gcmVERIFY_OK(gco3D_SetBlendFunction(Context->engine,
                                        gcvBLEND_TARGET,
                                        rgbFunc,
                                        aFunc));

    Context->blendModeRGB   = GL_FUNC_ADD;
    Context->blendModeAlpha = GL_FUNC_ADD;
    _glshTranslateBlendMode(Context->blendModeRGB,   &rgbMode);
    _glshTranslateBlendMode(Context->blendModeAlpha, &aMode);
    gcmVERIFY_OK(
        gco3D_SetBlendMode(Context->engine, rgbMode, aMode));

    Context->blendColorRed   = 0.0f;
    Context->blendColorGreen = 0.0f;
    Context->blendColorBlue  = 0.0f;
    Context->blendColorAlpha = 0.0f;
    gcmVERIFY_OK(
        gco3D_SetBlendColorF(Context->engine,
                             Context->blendColorRed,
                             Context->blendColorGreen,
                             Context->blendColorBlue,
                             Context->blendColorAlpha));

    gcmVERIFY_OK(
        gco3D_SetAlphaTest(Context->engine, gcvFALSE));

    Context->cullEnable = GL_FALSE;
    Context->cullMode   = GL_BACK;
    Context->cullFront  = GL_CCW;
    _SetCulling(Context);

    Context->depthMask = GL_TRUE;
    gcmVERIFY_OK(
        gco3D_EnableDepthWrite(Context->engine, Context->depthMask));

    Context->depthTest  = GL_FALSE;
    Context->depthFunc  = GL_LESS;
    Context->depthNear  = 0.0f;
    Context->depthFar   = 1.0f;
    Context->depthDirty = GL_TRUE;

    Context->ditherEnable = GL_TRUE;
    gco3D_EnableDither(
        Context->engine, Context->ditherEnable);

    Context->viewportX      = 0;
    Context->viewportY      = 0;
    Context->viewportWidth  = Context->drawWidth;
    Context->viewportHeight = Context->drawHeight;
    Context->scissorEnable  = GL_FALSE;
    Context->scissorX       = 0;
    Context->scissorY       = 0;
    Context->scissorWidth   = Context->drawWidth;
    Context->scissorHeight  = Context->drawHeight;
    Context->viewDirty      = GL_TRUE;

    Context->stencilEnable           = GL_FALSE;
    Context->stencilRefFront         = 0;
    Context->stencilRefBack          = 0;
    Context->stencilFuncFront        = GL_ALWAYS;
    Context->stencilFuncBack         = GL_ALWAYS;
    Context->stencilMaskFront        = ~0U;
    Context->stencilMaskBack         = ~0U;
    Context->stencilWriteMask        = ~0U;
    Context->stencilOpFailFront      = GL_KEEP;
    Context->stencilOpFailBack       = GL_KEEP;
    Context->stencilOpDepthFailFront = GL_KEEP;
    Context->stencilOpDepthFailBack  = GL_KEEP;
    Context->stencilOpDepthPassFront = GL_KEEP;
    Context->stencilOpDepthPassBack  = GL_KEEP;
    gcmONERROR(
        gco3D_SetStencilReference(Context->engine,
                                  (gctUINT8) Context->stencilRefFront, gcvTRUE));
    gcmONERROR(
        gco3D_SetStencilReference(Context->engine,
                                  (gctUINT8) Context->stencilRefFront, gcvFALSE));
    gcmVERIFY_OK(
        gco3D_SetStencilCompare(Context->engine,
                                gcvSTENCIL_FRONT,
                                _glshTranslateFunc(Context->stencilFuncFront)));
    gcmVERIFY_OK(
        gco3D_SetStencilCompare(Context->engine,
                                gcvSTENCIL_BACK,
                                _glshTranslateFunc(Context->stencilFuncBack)));
    gcmONERROR(
        gco3D_SetStencilMask(Context->engine,
                             (gctUINT8) Context->stencilMaskFront));
    gcmONERROR(
        gco3D_SetStencilWriteMask(Context->engine,
                                  (gctUINT8) Context->stencilWriteMask));
    gcmVERIFY_OK(
        gco3D_SetStencilFail(Context->engine,
                             gcvSTENCIL_FRONT,
                             Context->frontFail =
                                _glshTranslateOperation(
                                    Context->stencilOpFailFront)));
    gcmVERIFY_OK(
        gco3D_SetStencilFail(Context->engine,
                             gcvSTENCIL_BACK,
                             Context->backFail =
                                 _glshTranslateOperation(
                                     Context->stencilOpFailBack)));
    gcmVERIFY_OK(
        gco3D_SetStencilDepthFail(Context->engine,
                                  gcvSTENCIL_FRONT,
                                  Context->frontZFail =
                                      _glshTranslateOperation(
                                          Context->stencilOpDepthFailFront)));
    gcmVERIFY_OK(
        gco3D_SetStencilDepthFail(Context->engine,
                                  gcvSTENCIL_BACK,
                                  Context->backZFail =
                                      _glshTranslateOperation(
                                          Context->stencilOpDepthFailBack)));
    gcmVERIFY_OK(
        gco3D_SetStencilPass(Context->engine,
                             gcvSTENCIL_FRONT,
                             Context->frontZPass =
                                 _glshTranslateOperation(
                                     Context->stencilOpDepthPassFront)));
    gcmVERIFY_OK(
        gco3D_SetStencilPass(Context->engine,
                             gcvSTENCIL_BACK,
                             Context->backZPass =
                                 _glshTranslateOperation(
                                     Context->stencilOpDepthPassBack)));

    Context->colorEnableRed   = GL_TRUE;
    Context->colorEnableGreen = GL_TRUE;
    Context->colorEnableBlue  = GL_TRUE;
    Context->colorEnableAlpha = GL_TRUE;
    Context->colorWrite       = 0xF;
    gcmVERIFY_OK(
        gco3D_SetColorWrite(Context->engine, Context->colorWrite));

    Context->offsetEnable = GL_FALSE;
    Context->offsetFactor = 0.0f;
    Context->offsetUnits  = 0.0f;
    gcmONERROR(
        gco3D_SetDepthScaleBiasF(Context->engine,
                                 Context->offsetFactor,
                                 Context->offsetUnits));

    Context->sampleAlphaToCoverage = GL_FALSE;
    Context->sampleCoverage        = GL_FALSE;
    Context->sampleCoverageValue   = 1.0f;
    Context->sampleCoverageInvert  = GL_FALSE;

    /* Initialize alignment. */
    Context->packAlignment = Context->unpackAlignment = 4;

    /* Initialize PA. */
    gcmVERIFY_OK(
        gco3D_SetFill(Context->engine, gcvFILL_SOLID));
    gcmVERIFY_OK(
        gco3D_SetShading(Context->engine, gcvSHADING_SMOOTH));

    /* Initialize line width. */
    Context->lineWidth = 1.0f;

    Context->lineWidthRange[0] = 1.0f;
    Context->lineWidthRange[1] = 1.0f;

    if (gcoHAL_IsFeatureAvailable(Context->hal,
                                  gcvFEATURE_WIDE_LINE) == gcvSTATUS_TRUE)
    {
        /* Wide lines are supported. */
        Context->lineWidthRange[1] = 8192.0f;

    }

    /* Set the hardware states. */
    gcmVERIFY_OK(
        gco3D_SetAntiAliasLine(Context->engine, gcvTRUE));

    gcmVERIFY_OK(
        _SetLineWidth(Context, Context->lineWidth));

    /* Initialize hint. */
    Context->mipmapHint = GL_DONT_CARE;
    Context->fragShaderDerivativeHint = GL_DONT_CARE;

    /* Initialize flush state. */
    Context->textureFlushNeeded = gcvFALSE;

    /* Adjust z far plane. */
    if (Context->adjustZFar)
    {
        gcmVERIFY_OK(
            gco3D_SetDepthPlaneF(Context->engine,
                                 0.0f, Context->zFarValue));
    }

    gcmFOOTER_NO();
    return;

OnError:
    gcmFOOTER();
    return;
}

GL_APICALL void GL_APIENTRY
glViewport(
    GLint x,
    GLint y,
    GLsizei width,
    GLsizei height
    )
{
#if gcdNULL_DRIVER < 3
	glmENTER4(glmARGINT, x, glmARGINT, y, glmARGINT, width, glmARGINT, height)
	{
        gcmDUMP_API("${ES20 glViewport 0x%08X 0x%08X 0x%08X 0x%08X}",
                    x, y, width, height);

        glmPROFILE(context, GLES2_VIEWPORT, 0);

#if gldPATCHES
        /* Do batching. */
        glmBATCH(context,
                 break,
                 break,
                 (glshBatchViewport(context, x, y, width, height)));
#endif

        glshViewport(context, x, y, width, height);
    }
    glmLEAVE()
#endif
}

GLenum glshViewport(IN GLContext Context,
                    IN GLint X,
                    IN GLint Y,
                    IN GLsizei Width,
                    IN GLsizei Height)
{
    if ((Width < 0) || (Height < 0))
    {
        gcmTRACE_ZONE(gcvLEVEL_WARNING, gldZONE_STATE,
                      "%s(%d): Negative size %dx%d",
                      __FUNCTION__, __LINE__, Width, Height);
        gl2mERROR(GL_INVALID_VALUE);
        return GL_INVALID_VALUE;
    }

    if ((Context->viewportX      != X)               ||
        (Context->viewportY      != Y)               ||
        (Context->viewportWidth  != (GLuint) Width)  ||
        (Context->viewportHeight != (GLuint) Height) )
    {
        Context->viewportX      = X;
        Context->viewportY      = Y;
        Context->viewportWidth  = Width;
        Context->viewportHeight = Height;

        Context->viewDirty = GL_TRUE;

        /* Disable batch optmization. */
        Context->batchDirty = GL_TRUE;
    }

    return GL_NO_ERROR;
}

GL_APICALL void GL_APIENTRY
glPixelStorei(
    GLenum pname,
    GLint param
    )
{
#if gcdNULL_DRIVER < 3
    glmENTER2(glmARGENUM, pname, glmARGINT, param)
	{
    gcmDUMP_API("${ES20 glPixelStorei 0x%08X 0x%08X}", pname, param);

    glmPROFILE(context, GLES2_PIXELSTOREI, 0);

    switch (pname)
    {
    case GL_UNPACK_ALIGNMENT:
        context->unpackAlignment = param;
        break;

    case GL_PACK_ALIGNMENT:
        context->packAlignment = param;
        break;

    default:
        gcmTRACE_ZONE(gcvLEVEL_WARNING, gldZONE_STATE,
                      "%s(%d): Unknown pname=0x%04x",
                      __FUNCTION__, __LINE__, pname);
        gl2mERROR(GL_INVALID_ENUM);
        break;
    }

	}
	glmLEAVE();
#endif
}

GL_APICALL void GL_APIENTRY
glDisable(
    GLenum cap
    )
{
#if gcdNULL_DRIVER < 3
    glmENTER1(glmARGENUM, cap)
	{
        gcmDUMP_API("${ES20 glDisable 0x%08X}", cap);

        glmPROFILE(context, GLES2_DISABLE, 0);

#if gldPATCHES
        /* Do batching. */
        glmBATCH(context,
                 break,
                 break,
                 (glshBatchSet(context, cap, gcvFALSE)));
#endif

        glshDisable(context, cap);
    }
    glmLEAVE()
#endif
}

GLenum glshDisable(IN GLContext Context,
                   IN GLenum Cap)
{
    switch (Cap)
    {
        case GL_BLEND:
            Context->blendEnable = GL_FALSE;
            if (gcmIS_ERROR(gco3D_EnableBlending(Context->engine, gcvFALSE)))
            {
                gl2mERROR(GL_INVALID_OPERATION);
                return GL_INVALID_OPERATION;
            }
            break;

        case GL_CULL_FACE:
            Context->cullEnable = GL_FALSE;
            if (gcmIS_ERROR(gco3D_SetCulling(Context->engine, gcvCULL_NONE)))
            {
                gl2mERROR(GL_INVALID_OPERATION);
                return GL_INVALID_OPERATION;
            }
            break;

        case GL_DEPTH_TEST:
            Context->depthTest  = GL_FALSE;
            Context->depthDirty = GL_TRUE;
            break;

        case GL_DITHER:
            Context->ditherEnable = GL_FALSE;
            gco3D_EnableDither(Context->engine, gcvFALSE);
            break;

        case GL_STENCIL_TEST:
            Context->stencilEnable = GL_FALSE;
            Context->depthDirty    = GL_TRUE;
            break;

        case GL_SCISSOR_TEST:
            Context->scissorEnable = GL_FALSE;
            Context->viewDirty     = GL_TRUE;
            break;

        case GL_POLYGON_OFFSET_FILL:
            Context->offsetEnable = GL_FALSE;
            if (gcmIS_ERROR(gco3D_SetDepthScaleBiasF(Context->engine, 0.0f, 0.0f)))
            {
                gl2mERROR(GL_INVALID_OPERATION);
                return GL_INVALID_OPERATION;
            }
            break;

        case GL_SAMPLE_ALPHA_TO_COVERAGE:
            Context->sampleAlphaToCoverage = GL_FALSE;
            break;

        case GL_SAMPLE_COVERAGE:
            Context->sampleCoverage = GL_FALSE;
            break;

        case GL_TIMESTAMP_VIV0:
        case GL_TIMESTAMP_VIV1:
        case GL_TIMESTAMP_VIV2:
        case GL_TIMESTAMP_VIV3:
        case GL_TIMESTAMP_VIV4:
        case GL_TIMESTAMP_VIV5:
        case GL_TIMESTAMP_VIV6:
        case GL_TIMESTAMP_VIV7:
            if (gcmIS_ERROR(gcoHAL_SetTimer(Context->hal,
                                            Cap - GL_TIMESTAMP_VIV0,
                                            gcvFALSE)))
            {
                gl2mERROR(GL_INVALID_OPERATION);
                return GL_INVALID_OPERATION;
            }
            break;

        default:
            gcmTRACE_ZONE(gcvLEVEL_WARNING, gldZONE_STATE,
                          "%s(%d): Unknown cap=0x%04x",
                          __FUNCTION__, __LINE__, Cap);
            gl2mERROR(GL_INVALID_ENUM);
            return GL_INVALID_ENUM;
    }

    /* Disable batch optmization. */
    Context->batchDirty = GL_TRUE;

    return GL_NO_ERROR;
}

GL_APICALL void GL_APIENTRY
glEnable(
    GLenum cap
    )
{
#if gcdNULL_DRIVER < 3
    glmENTER1(glmARGENUM, cap)
	{
        gcmDUMP_API("${ES20 glEnable 0x%08X}", cap);

        glmPROFILE(context, GLES2_ENABLE, 0);

#if gldPATCHES
        /* Do batching. */
        glmBATCH(context,
                 break,
                 break,
                 (glshBatchSet(context, cap, gcvTRUE)));
#endif

        glshEnable(context, cap);
    }
    glmLEAVE()
#endif
}

GLenum glshEnable(IN GLContext Context,
                  IN GLenum Cap)
{
    GLfloat depthBias = 0.0f;

    switch (Cap)
    {
        case GL_BLEND:
#if gldPATCHES
            /* Route the call through the patch logic. */
            glshPatchBlend(Context, gcvTRUE);
#endif

            Context->blendEnable = GL_TRUE;
            gcmVERIFY_OK(gco3D_EnableBlending(Context->engine, gcvTRUE));
            break;

        case GL_CULL_FACE:
            Context->cullEnable = GL_TRUE;
            _SetCulling(Context);
            break;

        case GL_DEPTH_TEST:
            Context->depthTest  = GL_TRUE;
            Context->depthDirty = GL_TRUE;
            break;

        case GL_DITHER:
            Context->ditherEnable = GL_TRUE;
            gco3D_EnableDither(Context->engine, gcvTRUE);
            break;

        case GL_SCISSOR_TEST:
            Context->scissorEnable = GL_TRUE;
            Context->viewDirty     = GL_TRUE;
            break;

        case GL_STENCIL_TEST:
            Context->stencilEnable = GL_TRUE;
            Context->depthDirty    = GL_TRUE;
            break;

        case GL_POLYGON_OFFSET_FILL:
            Context->offsetEnable = GL_TRUE;
            depthBias = gcoMATH_Divide(Context->offsetUnits, 65535.f);

            if (gcmIS_ERROR(
                            gco3D_SetDepthScaleBiasF(Context->engine,
                                                     Context->offsetFactor,
                                                     depthBias)))
            {
                gl2mERROR(GL_INVALID_OPERATION);
                return GL_INVALID_OPERATION;
            }
            break;

        case GL_SAMPLE_ALPHA_TO_COVERAGE:
            Context->sampleAlphaToCoverage = GL_TRUE;
            break;

        case GL_SAMPLE_COVERAGE:
            Context->sampleCoverage = GL_TRUE;
            break;

        case GL_TIMESTAMP_VIV0:
        case GL_TIMESTAMP_VIV1:
        case GL_TIMESTAMP_VIV2:
        case GL_TIMESTAMP_VIV3:
        case GL_TIMESTAMP_VIV4:
        case GL_TIMESTAMP_VIV5:
        case GL_TIMESTAMP_VIV6:
        case GL_TIMESTAMP_VIV7:
            if (gcmIS_ERROR(gcoHAL_SetTimer(Context->hal,
                                            Cap - GL_TIMESTAMP_VIV0,
                                            gcvTRUE)))
            {
                gl2mERROR(GL_INVALID_OPERATION);
                return GL_INVALID_OPERATION;
            }
            break;

        default:
            gcmTRACE_ZONE(gcvLEVEL_WARNING, gldZONE_STATE,
                          "%s(%d): Unknown Cap=0x%04x",
                          __FUNCTION__, __LINE__, Cap);
            gl2mERROR(GL_INVALID_ENUM);
            return GL_INVALID_ENUM;
    }

    /* Disable batch optmization. */
    Context->batchDirty = GL_TRUE;

    return GL_NO_ERROR;
}

GL_APICALL void GL_APIENTRY
glBlendFunc(
    GLenum sfactor,
    GLenum dfactor
    )
{
#if gcdNULL_DRIVER < 3
	glmENTER2(glmARGENUM, sfactor, glmARGENUM, dfactor)
	{
        gcmDUMP_API("${ES20 glBlendFunc 0x%08X 0x%08X}", sfactor, dfactor);

        glmPROFILE(context, GLES2_BLENDFUNC, 0);

#if gldPATCHES
        /* Do batching. */
        glmBATCH(context,
                 break,
                 break,
                 (glshBatchBlendFunction(context, sfactor, sfactor, dfactor, dfactor)));
#endif

        glshBlendFuncSeparate(context, sfactor, sfactor, dfactor, dfactor);
    }
    glmLEAVE()
#endif
}

GL_APICALL void GL_APIENTRY
glBlendFuncSeparate(
    GLenum srcRGB,
    GLenum dstRGB,
    GLenum srcAlpha,
    GLenum dstAlpha
    )
{
#if gcdNULL_DRIVER < 3
	glmENTER4(glmARGENUM, srcRGB, glmARGENUM, dstRGB, glmARGENUM, srcAlpha, glmARGENUM, dstAlpha)
	{
        gcmDUMP_API("${ES20 glBlendFuncSeparate 0x%08X 0x%08X 0x%08X 0x%08X}",
                    srcRGB, dstRGB, srcAlpha, dstAlpha);

        glmPROFILE(context, GLES2_BLENDFUNCSEPARATE, 0);

#if gldPATCHES
        /* Do batching. */
        glmBATCH(context,
                 break,
                 break,
                 (glshBatchBlendFunction(context, srcRGB, srcAlpha, dstRGB, dstAlpha)));
#endif

        glshBlendFuncSeparate(context, srcRGB, srcAlpha, dstRGB, dstAlpha);
    }
    glmLEAVE()
#endif
}

GLenum glshBlendFuncSeparate(IN GLContext Context,
                             IN GLenum SourceRGB,
                             IN GLenum SourceAlpha,
                             IN GLenum DestinationRGB,
                             IN GLenum DestinationAalpha)
{
    gceBLEND_FUNCTION   blendSourceRGB, blendSourceAlpha;
    gceBLEND_FUNCTION   blendTargetRGB, blendTargetAlpha;
	gceSTATUS           status;

    if (gcmIS_ERROR(_glshTranslateBlendFunc(SourceRGB, &blendSourceRGB)))
    {
        gcmTRACE_ZONE(gcvLEVEL_WARNING, gldZONE_STATE,
                      "%s(%d): Unknown SourceRGB=0x%04x",
                      __FUNCTION__, __LINE__, SourceRGB);

        gl2mERROR(GL_INVALID_ENUM);
        return GL_INVALID_ENUM;
    }

    if (gcmIS_ERROR(_glshTranslateBlendFunc(DestinationRGB, &blendTargetRGB)))
    {
        gcmTRACE_ZONE(gcvLEVEL_WARNING, gldZONE_STATE,
                      "%s(%d): Unknown DestinationRGB=0x%04x",
                      __FUNCTION__, __LINE__, DestinationRGB);

        gl2mERROR(GL_INVALID_ENUM);
        return GL_INVALID_ENUM;
    }

    if (gcmIS_ERROR(_glshTranslateBlendFunc(SourceAlpha, &blendSourceAlpha)))
    {
        gcmTRACE_ZONE(gcvLEVEL_WARNING, gldZONE_STATE,
                      "%s(%d): Unknown SourceAlpha=0x%04x",
                      __FUNCTION__, __LINE__, SourceAlpha);

        gl2mERROR(GL_INVALID_ENUM);
        return GL_INVALID_ENUM;
    }

    if (gcmIS_ERROR(_glshTranslateBlendFunc(DestinationAalpha, &blendTargetAlpha)))
    {
        gcmTRACE_ZONE(gcvLEVEL_WARNING, gldZONE_STATE,
                      "%s(%d): Unknown DestinationAalpha=0x%04x",
                      __FUNCTION__, __LINE__, DestinationAalpha);

        gl2mERROR(GL_INVALID_ENUM);
        return GL_INVALID_ENUM;
    }

    Context->blendFuncSourceRGB   = SourceRGB;
    Context->blendFuncSourceAlpha = SourceAlpha;
    Context->blendFuncTargetRGB   = DestinationRGB;
    Context->blendFuncTargetAlpha = DestinationAalpha;

    gcmONERROR(gco3D_SetBlendFunction(Context->engine,
                                      gcvBLEND_SOURCE,
                                      blendSourceRGB, blendSourceAlpha));

    gcmONERROR(gco3D_SetBlendFunction(Context->engine,
                                      gcvBLEND_TARGET,
                                      blendTargetRGB, blendTargetAlpha));

    /* Disable batch optmization. */
    Context->batchDirty = GL_TRUE;

    return GL_NO_ERROR;

OnError:
    gl2mERROR(GL_INVALID_OPERATION);
    return GL_INVALID_OPERATION;
}

GL_APICALL void GL_APIENTRY
glBlendColor(
    GLclampf red,
    GLclampf green,
    GLclampf blue,
    GLclampf alpha
    )
{
#if gcdNULL_DRIVER < 3
    glmENTER4(glmARGFLOAT, red, glmARGFLOAT, green, glmARGFLOAT, blue, glmARGFLOAT, alpha)
	{
        gcmDUMP_API("${ES20 glBlendColor 0x%08X 0x%08X 0x%08X 0x%08X}",
                    *(GLuint*) &red, *(GLuint*) &green, *(GLuint*) &blue,
                    *(GLuint*) &alpha);

        glmPROFILE(context, GLES2_BLENDCOLOR, 0);

#if gldPATCHES
        /* Do batching. */
        glmBATCH(context,
                 break,
                 break,
                 (glshBatchBlendColor(context, red, green, blue, alpha)));
#endif

        glshBlendColor(context, red, green, blue, alpha);
    }
    glmLEAVE()
#endif
}

GLenum glshBlendColor(IN GLContext Context,
                      IN GLclampf Red,
                      IN GLclampf Green,
                      IN GLclampf Blue,
                      IN GLclampf Alpha)
{
    Context->blendColorRed   = gcmMAX(0.0f, gcmMIN(1.0f, Red));
    Context->blendColorGreen = gcmMAX(0.0f, gcmMIN(1.0f, Green));
    Context->blendColorBlue  = gcmMAX(0.0f, gcmMIN(1.0f, Blue));
    Context->blendColorAlpha = gcmMAX(0.0f, gcmMIN(1.0f, Alpha));

    if (gcmIS_ERROR(gco3D_SetBlendColorF(Context->engine,
                                         Context->blendColorRed,
                                         Context->blendColorGreen,
                                         Context->blendColorBlue,
                                         Context->blendColorAlpha)))
	{
		gl2mERROR(GL_INVALID_OPERATION);
		return GL_INVALID_OPERATION;
	}

    /* Disable batch optmization. */
    Context->batchDirty = GL_TRUE;

    return GL_NO_ERROR;
}

GL_APICALL void GL_APIENTRY
glBlendEquation(
    GLenum mode
    )
{
#if gcdNULL_DRIVER < 3
	glmENTER1(glmARGENUM, mode)
	{
        gcmDUMP_API("${ES20 glBlendEquation 0x%08X}", mode);

        glmPROFILE(context, GLES2_BLENDEQUATION, 0);

#if gldPATCHES
        /* Do batching. */
        glmBATCH(context,
                 break,
                 break,
                 (glshBatchBlendMode(context, mode, mode)));
#endif

        glshBlendEquationSeparate(context, mode, mode);
    }
    glmLEAVE()
#endif
}

GL_APICALL void GL_APIENTRY
glBlendEquationSeparate(
    GLenum modeRGB,
    GLenum modeAlpha
    )
{
#if gcdNULL_DRIVER < 3
	glmENTER2(glmARGENUM, modeRGB, glmARGENUM, modeAlpha)
	{
        gcmDUMP_API("${ES20 glBlendEquationSeparate 0x%08X 0x%08X}",
                    modeRGB, modeAlpha);

        glmPROFILE(context, GLES2_BLENDEQUATIONSEPARATE, 0);

#if gldPATCHES
        /* Do batching. */
        glmBATCH(context,
                 break,
                 break,
                 (glshBatchBlendMode(context, modeRGB, modeAlpha)));
#endif

        glshBlendEquationSeparate(context, modeRGB, modeAlpha);
    }
    glmLEAVE()
#endif
}

GLenum glshBlendEquationSeparate(IN GLContext Context,
                                 IN GLenum ModeRGB,
                                 IN GLenum ModeAlpha)
{
    gceBLEND_MODE   blendModeRGB, blendModeAlpha;

    if (gcmIS_ERROR(_glshTranslateBlendMode(ModeRGB, &blendModeRGB)))
    {
        gcmTRACE_ZONE(gcvLEVEL_WARNING, gldZONE_STATE,
                      "%s(%d): Unknown ModeRGB=0x%04x",
                      __FUNCTION__, __LINE__, ModeRGB);

        gl2mERROR(GL_INVALID_ENUM);
        return GL_INVALID_ENUM;
    }

    if (gcmIS_ERROR(_glshTranslateBlendMode(ModeAlpha, &blendModeAlpha)))
    {
        gcmTRACE_ZONE(gcvLEVEL_WARNING, gldZONE_STATE,
                      "%s(%d): Unknown ModeAlpha=0x%04x",
                      __FUNCTION__, __LINE__, ModeAlpha);

        gl2mERROR(GL_INVALID_ENUM);
        return GL_INVALID_ENUM;
    }

    Context->blendModeRGB   = ModeRGB;
    Context->blendModeAlpha = ModeAlpha;

    if (gcmIS_ERROR(gco3D_SetBlendMode(Context->engine, blendModeRGB, blendModeAlpha)))
	{
		gl2mERROR(GL_INVALID_OPERATION);
		return GL_INVALID_OPERATION;
	}

    /* Disable batch optmization. */
    Context->batchDirty = GL_TRUE;

    return GL_NO_ERROR;
}

GL_APICALL void GL_APIENTRY
glDepthFunc(
    GLenum func
    )
{
#if gcdNULL_DRIVER < 3
	glmENTER1(glmARGENUM, func)
	{
        gcmDUMP_API("${ES20 glDepthFunc 0x%08X}", func);

        glmPROFILE(context, GLES2_DEPTHFUNC, 0);

#if gldPATCHES
        /* Do batching. */
        glmBATCH(context,
                 break,
                 break,
                 (glshBatchDepthCompare(context, func)));
#endif

        glshDepthFunc(context, func);
    }
    glmLEAVE()
#endif
}

GLenum glshDepthFunc(IN GLContext Context,
                     IN GLenum Function)
{
    gceCOMPARE compare;

    compare = _glshTranslateFunc(Function);
    if (compare == gcvCOMPARE_INVALID)
    {
        gcmTRACE_ZONE(gcvLEVEL_WARNING, gldZONE_STATE,
                      "%s(%d): Unknown Function=0x%04x",
                      __FUNCTION__, __LINE__, Function);

        gl2mERROR(GL_INVALID_ENUM);
        return GL_INVALID_ENUM;
    }

    Context->depthFunc  = Function;
    Context->depthDirty = GL_TRUE;

    /* Disable batch optmization. */
    Context->batchDirty = GL_TRUE;

    return GL_NO_ERROR;
}

GL_APICALL void GL_APIENTRY
glDepthMask(
    GLboolean flag
    )
{
#if gcdNULL_DRIVER < 3
    glmENTER1(glmARGINT, flag)
	{
        gcmDUMP_API("${ES20 glDepthMask 0x%08X}", flag);

        glmPROFILE(context, GLES2_DEPTHMASK, 0);

#if gldPATCHES
        /* Do batching. */
        glmBATCH(context,
                 break,
                 break,
                 (glshBatchDepthMask(context, flag)));
#endif

        glshDepthMask(context, flag);
    }
    glmLEAVE()
#endif
}

GLenum glshDepthMask(IN GLContext Context,
                     IN GLboolean Flag)
{
    Context->depthMask = Flag;

    if (gcmIS_ERROR(gco3D_EnableDepthWrite(Context->engine, Flag)))
	{
		gl2mERROR(GL_INVALID_OPERATION);
		return GL_INVALID_OPERATION;
	}

    /* Disable batch optmization. */
    Context->batchDirty = GL_TRUE;

    return GL_NO_ERROR;
}

GL_APICALL void GL_APIENTRY
glDepthRangef(
    GLclampf zNear,
    GLclampf zFar
    )
{
#if gcdNULL_DRIVER < 3
    glmENTER2(glmARGFLOAT, zNear, glmARGFLOAT, zFar)
	{
        gcmDUMP_API("${ES20 glDepthRangef 0x%08X 0x%08X}",
                    *(GLuint*) &zNear, *(GLuint*) &zFar);

        glmPROFILE(context, GLES2_DEPTHRANGEF, 0);

#if gldPATCHES
        /* Do batching. */
        glmBATCH(context,
                 break,
                 break,
                 (glshBatchDepthRange(context, zNear, zFar)));
#endif

        glshDepthRangef(context, zNear, zFar);
    }
    glmLEAVE()
#endif
}

GLenum glshDepthRangef(IN GLContext Context,
                       IN GLclampf Near,
                       IN GLclampf Far)
{
    Context->depthNear  = gcmMAX(0.0f, gcmMIN(1.0f, Near));
    Context->depthFar   = gcmMAX(0.0f, gcmMIN(1.0f, Far));
    Context->depthDirty = GL_TRUE;

    /* Disable batch optmization. */
    Context->batchDirty = GL_TRUE;

    return GL_NO_ERROR;
}

GL_APICALL void GL_APIENTRY
glFrontFace(
    GLenum mode
    )
{
#if gcdNULL_DRIVER < 3
    glmENTER1(glmARGENUM, mode)
	{
        gcmDUMP_API("${ES20 glFrontFace 0x%08X}", mode);

        glmPROFILE(context, GLES2_FRONTFACE, 0);

#if gldPATCHES
        /* Do batching. */
        glmBATCH(context,
                 break,
                 break,
                 (glshBatchFrontFace(context, mode)));
#endif

        glshFrontFace(context, mode);
    }
    glmLEAVE()
#endif
}

GLenum glshFrontFace(IN GLContext Context,
                     IN GLenum FrontFace)
{
    Context->cullFront = FrontFace;

    if (Context->cullEnable)
    {
        _SetCulling(Context);
    }

    /* Disable batch optmization. */
    Context->batchDirty = GL_TRUE;

    return GL_NO_ERROR;
}

GL_APICALL void GL_APIENTRY
glStencilFunc(
    GLenum func,
    GLint ref,
    GLuint mask
    )
{
#if gcdNULL_DRIVER < 3
	glmENTER3(glmARGENUM, func, glmARGINT, ref, glmARGUINT, mask)
	{
        gcmDUMP_API("${ES20 glStencilFunc 0x%08X 0x%08X 0x%08X}",
                    func, ref, mask);

        glmPROFILE(context, GLES2_STENCILFUNC, 0);

#if gldPATCHES
        /* Do batching. */
        glmBATCH(context,
                 break,
                 break,
                 (glshBatchStencilFunction(context, GL_FRONT_AND_BACK, func, ref, mask)));
#endif

        glshStencilFuncSeparate(context, GL_FRONT_AND_BACK, func, ref, mask);
    }
    glmLEAVE()
#endif
}

GL_APICALL void GL_APIENTRY
glStencilFuncSeparate(
    GLenum face,
    GLenum func,
    GLint ref,
    GLuint mask
    )
{
#if gcdNULL_DRIVER < 3
	glmENTER4(glmARGENUM, face, glmARGENUM, func, glmARGINT, ref, glmARGUINT, mask)
	{
        gcmDUMP_API("${ES20 glStencilFuncSeparate 0x%08X 0x%08X 0x%08X 0x%08X}",
                    face, func, ref, mask);

        glmPROFILE(context, GLES2_STENCILFUNCSEPARATE, 0);

#if gldPATCHES
        /* Do batching. */
        glmBATCH(context,
                 break,
                 break,
                 (glshBatchStencilFunction(context, face, func, ref, mask)));
#endif

        glshStencilFuncSeparate(context, face, func, ref, mask);
    }
    glmLEAVE()
#endif
}

GLenum glshStencilFuncSeparate(IN GLContext Context,
                               IN GLenum Face,
                               IN GLenum Function,
                               IN GLint Reference,
                               IN GLuint Mask)
{
    gceCOMPARE  compare;
    gceSTATUS   status;

    compare = _glshTranslateFunc(Function);
    if (compare == gcvCOMPARE_INVALID)
    {
        gcmTRACE_ZONE(gcvLEVEL_WARNING, gldZONE_STATE,
                      "%s(%d): Unknown Function=0x%04x",
                      __FUNCTION__, __LINE__, Function);

        gl2mERROR(GL_INVALID_ENUM);
        return GL_INVALID_ENUM;
    }

    if ((Face == GL_FRONT) || (Face == GL_FRONT_AND_BACK))
    {
        Context->stencilFuncFront = Function;
        Context->stencilRefFront  = Reference;
        Context->stencilMaskFront = Mask;
        Context->stencilWriteMask = Mask;

        gcmONERROR(gco3D_SetStencilCompare(Context->engine,
                                           (Context->cullFront == GL_CW) ? gcvSTENCIL_FRONT : gcvSTENCIL_BACK,
                                           compare));

        gcmONERROR(gco3D_SetStencilReference(Context->engine,
                                             (gctUINT8) Reference,
                                             Context->cullFront == GL_CW));

        gcmONERROR(gco3D_SetStencilMask(Context->engine, (gctUINT8) Mask));

        gcmONERROR(gco3D_SetStencilWriteMask(Context->engine, (gctUINT8) Mask));
    }

    if ((Face == GL_BACK) || (Face == GL_FRONT_AND_BACK))
    {
        Context->stencilFuncBack  = Function;
        Context->stencilRefBack   = Reference;
        Context->stencilMaskBack  = Mask;
        Context->stencilWriteMask = Mask;

        gcmONERROR(gco3D_SetStencilCompare(Context->engine,
                                           (Context->cullFront == GL_CW) ? gcvSTENCIL_BACK : gcvSTENCIL_FRONT,
                                           compare));

        gcmONERROR(gco3D_SetStencilReference(Context->engine,
                                             (gctUINT8) Reference,
                                             Context->cullFront != GL_CW));

        gcmONERROR(gco3D_SetStencilMask(Context->engine, (gctUINT8) Mask));

        gcmONERROR(gco3D_SetStencilWriteMask(Context->engine, (gctUINT8) Mask));
    }

    /* Disable batch optmization. */
    Context->batchDirty = GL_TRUE;

    return GL_NO_ERROR;

OnError:
	gl2mERROR(GL_INVALID_OPERATION);
    return GL_INVALID_OPERATION;
}

GL_APICALL void GL_APIENTRY
glStencilMask(
    GLuint mask
    )
{
#if gcdNULL_DRIVER < 3
	glmENTER1(glmARGUINT, mask)
	{
        gcmDUMP_API("${ES20 glStencilMask 0x%08X}", mask);

        glmPROFILE(context, GLES2_STENCILMASK, 0);

#if gldPATCHES
        /* Do batching. */
        glmBATCH(context,
                 break,
                 break,
                 (glshBatchStencilMask(context, GL_FRONT_AND_BACK, mask)));
#endif

        glshStencilMaskSeparate(context, GL_FRONT_AND_BACK, mask);
    }
    glmLEAVE()
#endif
}

GL_APICALL void GL_APIENTRY
glStencilMaskSeparate(
    GLenum face,
    GLuint mask
    )
{
#if gcdNULL_DRIVER < 3
	glmENTER2(glmARGENUM, face, glmARGUINT, mask)
	{
        gcmDUMP_API("${ES20 glStencilMaskSeparate 0x%08X 0x%08X}", face, mask);

        glmPROFILE(context, GLES2_STENCILMASKSEPARATE, 0);

#if gldPATCHES
        /* Do batching. */
        glmBATCH(context,
                 break,
                 break,
                 (glshBatchStencilMask(context, face, mask)));
#endif

        glshStencilMaskSeparate(context, face, mask);
    }
    glmLEAVE()
#endif
}

GLenum glshStencilMaskSeparate(IN GLContext Context,
                               IN GLenum Face,
                               IN GLuint Mask)
{
    gceSTATUS   status;

    if ((Face == GL_FRONT) || (Face == GL_FRONT_AND_BACK))
    {
        Context->stencilWriteMask = Mask;
        gcmONERROR(gco3D_SetStencilWriteMask(Context->engine, (gctUINT8) Mask));
    }

    if ((Face == GL_BACK) || (Face == GL_FRONT_AND_BACK))
    {
        Context->stencilWriteMask = Mask;
        gcmONERROR(gco3D_SetStencilWriteMask(Context->engine, (gctUINT8) Mask));
    }

    /* Disable batch optmization. */
    Context->batchDirty = GL_TRUE;

	return GL_NO_ERROR;

OnError:
	gl2mERROR(GL_INVALID_OPERATION);
    return GL_INVALID_OPERATION;
}

GL_APICALL void GL_APIENTRY
glStencilOp(
    GLenum fail,
    GLenum zfail,
    GLenum zpass
    )
{
#if gcdNULL_DRIVER < 3
	glmENTER3(glmARGENUM, fail, glmARGENUM, zfail, glmARGENUM, zpass)
	{
        gcmDUMP_API("${ES20 glStencilOp 0x%08X 0x%08X 0x%08X}", fail, zfail, zpass);

        glmPROFILE(context, GLES2_STENCILOP, 0);

#if gldPATCHES
        /* Do batching. */
        glmBATCH(context,
                 break,
                 break,
                 (glshBatchStencilOperation(context, GL_FRONT_AND_BACK, fail, zfail, zpass)));
#endif

        glshStencilOpSeparate(context, GL_FRONT_AND_BACK, fail, zfail, zpass);
    }
    glmLEAVE()
#endif
}

GL_APICALL void GL_APIENTRY
glStencilOpSeparate(
    GLenum face,
    GLenum fail,
    GLenum zfail,
    GLenum zpass
    )
{
#if gcdNULL_DRIVER < 3
	glmENTER4(glmARGENUM, face, glmARGENUM, fail, glmARGENUM, zfail, glmARGENUM, zpass)
	{
        gcmDUMP_API("${ES20 glStencilOpSeparate 0x%08X 0x%08X 0x%08X 0x%08X}",
                    face, fail, zfail, zpass);

        glmPROFILE(context, GLES2_STENCILOPSEPARATE, 0);

#if gldPATCHES
        /* Do batching. */
        glmBATCH(context,
                 break,
                 break,
                 (glshBatchStencilOperation(context, face, fail, zfail, zpass)));
#endif

        glshStencilOpSeparate(context, face, fail, zfail, zpass);
    }
    glmLEAVE()
#endif
}

GLenum glshStencilOpSeparate(IN GLContext Context,
                             IN GLenum Face,
                             IN GLenum Fail,
                             IN GLenum DepthFail,
                             IN GLenum DepthPass)
{
    gceSTENCIL_OPERATION    opFail, opDepthFail, opDepthPass;
	gceSTATUS               status;

    opFail = _glshTranslateOperation(Fail);
    if (opFail == gcvSTENCIL_OPERATION_INVALID)
    {
        gcmTRACE_ZONE(gcvLEVEL_WARNING, gldZONE_STATE,
                      "%s(%d): Unknown Fail=0x%04x",
                      __FUNCTION__, __LINE__, Fail);

        gl2mERROR(GL_INVALID_ENUM);
        return GL_INVALID_ENUM;
    }

    opDepthFail = _glshTranslateOperation(DepthFail);
    if (opDepthFail == gcvSTENCIL_OPERATION_INVALID)
    {
        gcmTRACE_ZONE(gcvLEVEL_WARNING, gldZONE_STATE,
                      "%s(%d): Unknown DepthFail=0x%04x",
                      __FUNCTION__, __LINE__, DepthFail);

        gl2mERROR(GL_INVALID_ENUM);
        return GL_INVALID_ENUM;
    }

    opDepthPass = _glshTranslateOperation(DepthPass);
    if (opDepthPass == gcvSTENCIL_OPERATION_INVALID)
    {
        gcmTRACE_ZONE(gcvLEVEL_WARNING, gldZONE_STATE,
                      "%s(%d): Unknown DepthPass=0x%04x",
                      __FUNCTION__, __LINE__, DepthPass);

        gl2mERROR(GL_INVALID_ENUM);
        return GL_INVALID_ENUM;
    }

    if ((Face == GL_FRONT) || (Face == GL_FRONT_AND_BACK))
    {
        Context->stencilOpFailFront      = Fail;
        Context->stencilOpDepthFailFront = DepthFail;
        Context->stencilOpDepthPassFront = DepthPass;

        gcmONERROR(gco3D_SetStencilFail(Context->engine,
                                        (Context->cullFront == GL_CW) ? gcvSTENCIL_FRONT : gcvSTENCIL_BACK,
                                        Context->frontFail = opFail));
        gcmONERROR(gco3D_SetStencilDepthFail(Context->engine,
                                             (Context->cullFront == GL_CW) ? gcvSTENCIL_FRONT : gcvSTENCIL_BACK,
                                             Context->frontZFail = opDepthFail));
        gcmONERROR(gco3D_SetStencilPass(Context->engine,
                                        (Context->cullFront == GL_CW) ? gcvSTENCIL_FRONT : gcvSTENCIL_BACK,
                                        Context->frontZPass = opDepthPass));
    }

    if ((Face == GL_BACK) || (Face == GL_FRONT_AND_BACK))
    {
        Context->stencilOpFailBack      = Fail;
        Context->stencilOpDepthFailBack = DepthFail;
        Context->stencilOpDepthPassBack = DepthPass;

        gcmONERROR(gco3D_SetStencilFail(Context->engine,
                                        (Context->cullFront == GL_CW) ? gcvSTENCIL_BACK : gcvSTENCIL_FRONT,
                                        Context->backFail = opFail));
        gcmONERROR(gco3D_SetStencilDepthFail(Context->engine,
                                             (Context->cullFront == GL_CW) ? gcvSTENCIL_BACK : gcvSTENCIL_FRONT,
                                             Context->backZFail = opDepthFail));
        gcmONERROR(gco3D_SetStencilPass(Context->engine,
                                        (Context->cullFront == GL_CW) ? gcvSTENCIL_BACK : gcvSTENCIL_FRONT,
                                        Context->backZPass = opDepthPass));
    }

    /* Disable batch optmization. */
    Context->batchDirty = GL_TRUE;

    return GL_NO_ERROR;

OnError:
	gl2mERROR(GL_INVALID_OPERATION);
    return GL_INVALID_OPERATION;
}

GL_APICALL void GL_APIENTRY
glScissor(
    GLint x,
    GLint y,
    GLsizei width,
    GLsizei height
    )
{
#if gcdNULL_DRIVER < 3
    glmENTER4(glmARGINT, x, glmARGINT, y, glmARGINT, width, glmARGINT, height)
	{
        gcmDUMP_API("${ES20 glScissor 0x%08X 0x%08X 0x%08X 0x%08X}",
                    x, y, width, height);

        glmPROFILE(context, GLES2_SCISSOR, 0);

#if gldPATCHES
        /* Do batching. */
        glmBATCH(context,
                 break,
                 break,
                 (glshBatchScissor(context, x, y, width, height)));
#endif

        glshScissor(context, x, y, width, height);
    }
    glmLEAVE()
#endif
}

GLenum glshScissor(IN GLContext Context,
                   IN GLint X,
                   IN GLint Y,
                   IN GLsizei Width,
                   IN GLsizei Height)
{
    /* Save scissor box. */
    Context->scissorX      = X;
    Context->scissorY      = Y;
    Context->scissorWidth  = Width;
    Context->scissorHeight = Height;

    Context->viewDirty = GL_TRUE;

    /* Disable batch optmization. */
    Context->batchDirty = GL_TRUE;

    return GL_NO_ERROR;
}

GL_APICALL void GL_APIENTRY
glColorMask(
    GLboolean red,
    GLboolean green,
    GLboolean blue,
    GLboolean alpha
    )
{
#if gcdNULL_DRIVER < 3
    glmENTER4(glmARGINT, red, glmARGINT, green, glmARGINT, blue, glmARGINT, alpha)
	{
        gcmDUMP_API("${ES20 glColorMask 0x%08X 0x%08X 0x%08X 0x%08X}",
                    red, green, blue, alpha);

        glmPROFILE(context, GLES2_COLORMASK, 0);

#if gldPATCHES
        /* Do batching. */
        glmBATCH(context,
                 break,
                 break,
                 (glshBatchColorMask(context, red, green, blue, alpha)));
#endif

        glshColorMask(context, red, green, blue, alpha);
    }
    glmLEAVE()
#endif
}

GLenum glshColorMask(IN GLContext Context,
                     IN GLboolean Red,
                     IN GLboolean Green,
                     IN GLboolean Blue,
                     IN GLboolean Alpha)
{
    Context->colorEnableRed   = Red;
    Context->colorEnableGreen = Green;
    Context->colorEnableBlue  = Blue;
    Context->colorEnableAlpha = Alpha;

    Context->colorWrite = ((Red   ? 0x1 : 0x0) |
                           (Green ? 0x2 : 0x0) |
                           (Blue  ? 0x4 : 0x0) |
                           (Alpha ? 0x8 : 0x0) );

    if (gcmIS_ERROR(gco3D_SetColorWrite(Context->engine, Context->colorWrite)))
	{
		gl2mERROR(GL_INVALID_OPERATION);
		return GL_INVALID_OPERATION;
	}

    /* Disable batch optmization. */
    Context->batchDirty = GL_TRUE;

    return GL_NO_ERROR;
}

GL_APICALL void GL_APIENTRY
glCullFace(
    GLenum mode
    )
{
#if gcdNULL_DRIVER < 3
    glmENTER1(glmARGENUM, mode)
	{
        gcmDUMP_API("${ES20 glCullFace 0x%08X}", mode);

        glmPROFILE(context, GLES2_CULLFACE, 0);

#if gldPATCHES
        /* Do batching. */
        glmBATCH(context,
                 break,
                 break,
                 (glshBatchCullFace(context, mode)));
#endif

        glshCullFace(context, mode);
    }
    glmLEAVE()
#endif
}

GLenum glshCullFace(IN GLContext Context,
                    IN GLenum Mode)
{
    switch (Mode)
    {
        case GL_FRONT:
        case GL_BACK:
        case GL_FRONT_AND_BACK:
            Context->cullMode = Mode;

            if (Context->cullEnable)
            {
                _SetCulling(Context);
            }
            break;

        default:
            gcmTRACE_ZONE(gcvLEVEL_WARNING, gldZONE_STATE,
                          "%s(%d): Unknown Mode=0x%04x",
                          __FUNCTION__, __LINE__, Mode);

            gl2mERROR(GL_INVALID_ENUM);
            return GL_INVALID_ENUM;
    }

    /* Disable batch optmization. */
    Context->batchDirty = GL_TRUE;

    return GL_NO_ERROR;
}

GL_APICALL void GL_APIENTRY
glPolygonOffset(
    GLfloat factor,
    GLfloat units
    )
{
#if gcdNULL_DRIVER < 3
	glmENTER2(glmARGFLOAT, factor, glmARGFLOAT, units)
	{
        gcmDUMP_API("${ES20 glPolygonOffset 0x%08X 0x%08X}",
                    *(GLuint*) &factor, *(GLuint*) &units);

        glmPROFILE(context, GLES2_POLYGONOFFSET, 0);

#if gldPATCHES
        /* Do batching. */
        glmBATCH(context,
                 break,
                 break,
                 (glshBatchOffset(context, factor, units)));
#endif

        glshPolygonOffset(context, factor, units);
    }
    glmLEAVE()
#endif
}

GLenum glshPolygonOffset(IN GLContext Context,
                         IN GLfloat Factor,
                         IN GLfloat Units)
{
    /* Save state values. */
    Context->offsetFactor = Factor;
    Context->offsetUnits  = Units;

    if (Context->offsetEnable)
    {
        Units = gcoMATH_Divide(Units, 65535.f);

        /* Set depth offset in hardware. */
        if (gcmIS_ERROR(gco3D_SetDepthScaleBiasF(Context->engine, Factor, Units)))
		{
			gl2mERROR(GL_INVALID_OPERATION);
            return GL_INVALID_OPERATION;
		}
    }

    /* Disable batch optmization. */
    Context->batchDirty = GL_TRUE;

    return GL_NO_ERROR;
}

GL_APICALL void GL_APIENTRY
glSampleCoverage(
    GLclampf value,
    GLboolean invert
    )
{
#if gcdNULL_DRIVER < 3
    glmENTER2(glmARGFLOAT, value, glmARGFLOAT, invert)
	{
    gcmDUMP_API("${ES20 glSampleCoverage 0x%08X 0x%08X}",
                *(GLuint*) &value, invert);

    glmPROFILE(context, GLES2_SAMPLECOVERAGE, 0);

    context->sampleCoverageValue  = value;
    context->sampleCoverageInvert = invert;

	}
	glmLEAVE();
#endif
}

GL_APICALL void GL_APIENTRY
glLineWidth(
    GLfloat width
    )
{
#if gcdNULL_DRIVER < 3
    glmENTER1(glmARGFLOAT, width)
	{
        gcmDUMP_API("${ES20 glLineWidth 0x%08X}", *(GLuint*) &width);

        glmPROFILE(context, GLES2_LINEWIDTH, 0);

#if gldPATCHES
        /* Do batching. */
        glmBATCH(context,
                 break,
                 break,
                 (glshBatchLineWidth(context, width)));
#endif

        glshLineWidth(context, width);
    }
    glmLEAVE()
#endif
}

GLenum glshLineWidth(IN GLContext Context,
                     IN GLfloat Width)
{
    gceSTATUS   status;

    status = _SetLineWidth(Context, Width);

    if (gcmIS_SUCCESS(status))
    {
        Context->lineWidth = Width;
    }
    else
    {
        gl2mERROR(GL_INVALID_VALUE);
        return GL_INVALID_VALUE;
    }

    /* Disable batch optmization. */
    Context->batchDirty = GL_TRUE;

    return GL_NO_ERROR;
}

GL_APICALL void GL_APIENTRY
glHint(
    GLenum target,
    GLenum mode
    )
{
#if gcdNULL_DRIVER < 3
    glmENTER2(glmARGENUM, target, glmARGENUM, mode)
	{
    gcmDUMP_API("${ES20 glHint 0x%08X 0x%08X}", target, mode);

    glmPROFILE(context, GLES2_HINT, 0);

    switch (target)
    {
    case GL_GENERATE_MIPMAP_HINT:
        switch (mode)
        {
        case GL_FASTEST:
        case GL_NICEST:
        case GL_DONT_CARE:
            context->mipmapHint = mode;
            break;

        default:
            gcmTRACE_ZONE(gcvLEVEL_WARNING, gldZONE_STATE,
                          "%s(%d): Unknown mode=0x%04x",
                          __FUNCTION__, __LINE__, mode);

            gl2mERROR(GL_INVALID_ENUM);
            break;
        }
        break;

    case GL_FRAGMENT_SHADER_DERIVATIVE_HINT_OES:
        switch (mode)
        {
        case GL_FASTEST:
        case GL_NICEST:
        case GL_DONT_CARE:
            context->fragShaderDerivativeHint = mode;
            break;

        default:
            gcmTRACE_ZONE(gcvLEVEL_WARNING, gldZONE_STATE,
                          "%s(%d): Unknown mode=0x%04x",
                          __FUNCTION__, __LINE__, mode);

            gl2mERROR(GL_INVALID_ENUM);
            break;
        }
        break;

    default:
        gcmTRACE_ZONE(gcvLEVEL_WARNING, gldZONE_STATE,
                      "%s(%d): Unknown target=0x%04x",
                      __FUNCTION__, __LINE__, target);

        gl2mERROR(GL_INVALID_ENUM);
        break;
    }

	}
	glmLEAVE();
#endif
}
