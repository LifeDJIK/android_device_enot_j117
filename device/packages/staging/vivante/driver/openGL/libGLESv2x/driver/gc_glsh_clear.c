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

void
_glshInitializeClear(
    GLContext Context
    )
{
    /* Set default clear color value. */
    Context->clearRed   = 0.0f;
    Context->clearGreen = 0.0f;
    Context->clearBlue  = 0.0f;
    Context->clearAlpha = 0.0f;
    gcmVERIFY_OK(gco3D_SetClearColorF(Context->engine, 0.0f, 0.0f, 0.0f, 0.0f));

    /* Set default clear depth value. */
    Context->clearDepth = 1.0f;
    gcmVERIFY_OK(gco3D_SetClearDepthF(Context->engine, 1.0f));

    /* Set default clear stencil value. */
    Context->clearStencil = 0;
    gcmVERIFY_OK(gco3D_SetClearStencil(Context->engine, 0));

#if gldUSE_3D_RENDERING_CLEAR
    Context->clearShaderExist = GL_FALSE;
    Context->clearProgram     = 0;
    Context->clearVertShader  = 0;
    Context->clearFragShader  = 0;
#endif

    Context->copyTexShaderExist = GL_FALSE;
    Context->copyTexShaderInside = GL_FALSE;
    Context->copyTexProgram     = 0;
    Context->copyTexVertShader  = 0;
    Context->copyTexFragShader  = 0;
    Context->copyTexFBO         = 0;
}

void
_glshDeinitializeClear(
    GLContext Context
    )
{
#if gldUSE_3D_RENDERING_CLEAR
    if (Context->clearShaderExist)
    {
        if (Context->clearProgram != 0)
        {
            glDeleteProgram(Context->clearProgram);
            Context->clearProgram     = 0;
        }

        if (Context->clearVertShader != 0)
        {
            glDeleteShader(Context->clearVertShader);
            Context->clearVertShader  = 0;
        }

        if (Context->clearFragShader != 0)
        {
            glDeleteShader(Context->clearFragShader);
            Context->clearFragShader  = 0;
        }

        if (Context->copyTexFBO != 0)
        {
            glDeleteFramebuffers(1,&Context->copyTexFBO);
            Context->copyTexFBO = 0;
        }

        Context->clearShaderExist = GL_FALSE;
    }
#endif

    if (Context->copyTexShaderExist)
    {
        if (Context->copyTexProgram != 0)
        {
            glDeleteProgram(Context->copyTexProgram);
            Context->copyTexProgram     = 0;
        }

        if (Context->copyTexVertShader != 0)
        {
            glDeleteShader(Context->copyTexVertShader);
            Context->copyTexVertShader  = 0;
        }

        if (Context->copyTexFragShader != 0)
        {
            glDeleteShader(Context->copyTexFragShader);
            Context->copyTexFragShader  = 0;
        }

        if (Context->copyTexFBO != 0)
        {
            glDeleteFramebuffers(1,&Context->copyTexFBO);
            Context->copyTexFBO = 0;
        }

        Context->copyTexShaderExist = GL_FALSE;
    }
}

GL_APICALL void GL_APIENTRY glClearColor(GLclampf Red,
                                         GLclampf Green,
                                         GLclampf Blue,
                                         GLclampf Alpha)
{
#if gcdNULL_DRIVER < 3
	glmENTER4(glmARGFLOAT, Red, glmARGFLOAT, Green, glmARGFLOAT, Blue, glmARGFLOAT, Alpha)
    {
        gcmDUMP_API("${ES20 glClearColor 0x%08X 0x%08X 0x%08X 0x%08X}",
                    *(GLuint *) &Red, *(GLuint *) &Green, *(GLuint *) &Blue, *(GLuint *) &Alpha);

        /* Profile the call. */
        glmPROFILE(context, GLES2_CLEARCOLOR, 0);

#if gldPATCHES
        /* Do batching. */
        glmBATCH(context,
                 break,
                 break,
                 (glshBatchClearColor(context, Red, Green, Blue, Alpha)));
#endif

        /* Set the clear color. */
        glshClearColor(context, Red, Green, Blue, Alpha);
    }
    glmLEAVE()
#endif
}

GLenum glshClearColor(IN GLContext Context,
                      GLclampf Red,
                      GLclampf Green,
                      GLclampf Blue,
                      GLclampf Alpha)
{
    /* Set the clear color. */
    Context->clearRed   = gcmCLAMP(Red,   0.0f, 1.0f);
    Context->clearGreen = gcmCLAMP(Green, 0.0f, 1.0f);
    Context->clearBlue  = gcmCLAMP(Blue,  0.0f, 1.0f);
    Context->clearAlpha = gcmCLAMP(Alpha, 0.0f, 1.0f);

    /* Send clear color to HAL. */
    if (gcmIS_ERROR(gco3D_SetClearColorF(Context->engine,
                                         Context->clearRed,
                                         Context->clearGreen,
                                         Context->clearBlue,
                                         Context->clearAlpha)))
    {
        gl2mERROR(GL_INVALID_OPERATION);
        return GL_INVALID_OPERATION;
    }

    /* Success. */
    return GL_NO_ERROR;
}

GL_APICALL void GL_APIENTRY glClearDepthf(GLclampf Depth)
{
#if gcdNULL_DRIVER < 3
	glmENTER1(glmARGFLOAT, Depth)
	{
        gcmDUMP_API("${ES20 glClearDepthf 0x%08X}", *(GLuint *) &Depth);

        /* Profile the call. */
        glmPROFILE(context, GLES2_CLEARDEPTHF, 0);

#if gldPATCHES
        /* Do batching. */
        glmBATCH(context,
                 break,
                 break,
                 (glshBatchClearDepth(context, Depth)));
#endif

        /* Set the clear depth. */
        glshClearDepthf(context, Depth);
    }
    glmLEAVE()
#endif
}

GLenum glshClearDepthf(IN GLContext Context,
                       IN GLclampf Depth)
{
    /* Set the clear depth. */
    Context->clearDepth = gcmCLAMP(Depth, 0.0f, 1.0f);

    /* Send clear depth to HAL. */
    if (gcmIS_ERROR(gco3D_SetClearDepthF(Context->engine, Context->clearDepth)))
	{
		gl2mERROR(GL_INVALID_OPERATION);
		return GL_INVALID_OPERATION;
	}

    /* Success. */
    return GL_NO_ERROR;
}

GL_APICALL void GL_APIENTRY glClearStencil(GLint S)
{
#if gcdNULL_DRIVER < 3
	glmENTER1(glmARGINT, S)
	{
        gcmDUMP_API("${ES20 glClearStencil 0x%08X}", S);

        /* Profile the call. */
        glmPROFILE(context, GLES2_CLEARSTENCIL, 0);

#if gldPATCHES
        /* Do batching. */
        glmBATCH(context,
                 break,
                 break,
                 (glshBatchClearStencil(context, S)));
#endif

        /* Set the clear stencil. */
        glshClearStencil(context, S);
    }
    glmLEAVE()
#endif
}

GLenum glshClearStencil(IN GLContext Context,
                        IN GLint S)
{
    /* Set the clear stencil. */
    Context->clearStencil = S;

    /* Send clear stencil to HAL. */
    if (gcmIS_ERROR(gco3D_SetClearStencil(Context->engine, S)))
	{
		gl2mERROR(GL_INVALID_OPERATION);
        return GL_INVALID_OPERATION;
	}

    /* Success. */
    return GL_NO_ERROR;
}

#if gldUSE_3D_RENDERING_CLEAR
#define VERTEX_INDEX        0

gceSTATUS
_ClearRect(
    GLContext Context,
    GLbitfield Mask
    )
{
    /* Define result. */
    gceSTATUS status = gcvSTATUS_OK;

    do
    {
        /* Scissor bounding box coordinate. */
        GLfloat Xs, Ys, Ws, Hs;

        /* Viewport bounding box size. */
        GLfloat widthViewport, heightViewport;

        /* Normalized coordinates. */
        GLfloat normLeft, normTop, normWidth, normHeight;

        /* Rectangle coordinates. */
        GLfloat left, top, right, bottom;
        GLfloat vertexBuffer[4 * 3];

        /* Color states. */
        GLboolean   colorMask[4];
        GLfloat     clearColor[4];

        /* Depth states. */
        GLfloat     depthClear = 0.0f;
        GLenum      depthFunc = GL_LESS;
        GLboolean   depthWriteMask = GL_FALSE;

        /* Stencil states. */
        GLint       stencilClear = 0;
        GLint       stencilRef = 0;
        GLenum      stencilFunc = GL_ALWAYS;
        GLuint      stencilWriteMask = 0xffff;
        GLuint      stencilMask = 0xffff;
        GLenum      stencilFail = GL_KEEP;
        GLenum      stencilzFail = GL_KEEP;
        GLenum      stencilzPass = GL_KEEP;

        GLboolean   alphaBlend;
        GLboolean   depthTest;
        GLboolean   stencilTest;
        GLboolean   cullEnable;
        GLProgram   program;
        GLBuffer    currentBuffer = gcvNULL;
        gctBOOL     rectPrimAvail = gcvFALSE;

        /* Restore vertex data */
#if gldUSE_VERTEX_ARRAY
        GLint       size;
        GLenum      type;
        GLboolean   normalized;
        GLsizei     stride;
        GLBuffer    buffer;
        gctBOOL     attribEnable;
        const void* ptr = gcvNULL;
#else
        GLint       size;
        GLenum      type;
        GLboolean   normalized;
        GLsizei     stride;
        GLBuffer    buffer;
        GLboolean   attribEnable;
        const void* ptr = gcvNULL;
#endif

        GLint       shaderCompiled;
        GLint       shaderLinked;
        GLint       u_clearColorLocation;

        GLchar*     vertSource = "  attribute vec3 a_position;                  \n\
                                    void main(void)                             \n\
                                    {                                           \n\
                                        vec4 pos;                               \n\
                                        pos = vec4(a_position, 1.0);            \n\
                                        gl_Position = pos;                      \n\
                                    }";

        GLchar*     fragSource = "  uniform vec4 u_clearColor;                  \n\
                                    void main(void)                             \n\
                                    {                                           \n\
                                        vec4 cur_color;                         \n\
                                        cur_color = u_clearColor;               \n\
                                        gl_FragColor = cur_color;               \n\
                                    }";

        /* Get color states. */
        colorMask[0] = Context->colorEnableRed != 0;
        colorMask[1] = Context->colorEnableGreen != 0;
        colorMask[2] = Context->colorEnableBlue != 0;
        colorMask[3] = Context->colorEnableAlpha != 0;

        /* Get clear color */
        clearColor[0] = Context->clearRed;
        clearColor[1] = Context->clearGreen;
        clearColor[2] = Context->clearBlue;
        clearColor[3] = Context->clearAlpha;

        alphaBlend  = Context->blendEnable;
        depthTest   = Context->depthTest;
        stencilTest = Context->stencilEnable;
        cullEnable  = Context->cullEnable;
        program     = Context->program;
        currentBuffer = Context->arrayBuffer;

        /* Must restore previous vertex data */
#if gldUSE_VERTEX_ARRAY
        size            = Context->vertexArray[VERTEX_INDEX].size;
        type            = Context->vertexArrayGL[VERTEX_INDEX].type;
        normalized      = (GLboolean)Context->vertexArray[VERTEX_INDEX].normalized;
        stride          = Context->vertexArrayGL[VERTEX_INDEX].stride;
        buffer          = Context->vertexArrayGL[VERTEX_INDEX].buffer;
        attribEnable    = Context->vertexArray[VERTEX_INDEX].enable;
        ptr             = Context->vertexArray[VERTEX_INDEX].pointer;
#else
        size            = Context->vertexArray[VERTEX_INDEX].size;
        type            = Context->vertexArray[VERTEX_INDEX].type;
        normalized      = Context->vertexArray[VERTEX_INDEX].normalized;
        stride          = Context->vertexArray[VERTEX_INDEX].stride;
        buffer          = Context->vertexArray[VERTEX_INDEX].buffer;
        attribEnable    = Context->vertexArray[VERTEX_INDEX].enable;

        if (buffer == gcvNULL)
        {
            ptr = Context->vertexArray[VERTEX_INDEX].ptr;
        }
        else
        {
            /* offset */
            ptr = (const void*)Context->vertexArray[VERTEX_INDEX].offset;
        }
#endif

        /* Program color states. */
        if (Mask & GL_COLOR_BUFFER_BIT)
        {
            glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        }
        else
        {
            glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
        }

        if (Context->depth)
        {
            /* Get depth states. */
            depthClear              = Context->clearDepth;
            depthWriteMask          = Context->depthMask;
            depthFunc               = Context->depthFunc;

            /* Get stencil states. */
            stencilClear            = Context->clearStencil;
            stencilWriteMask        = Context->stencilWriteMask;
            stencilFunc             = Context->stencilFuncFront;
            stencilRef              = Context->stencilRefFront;
            stencilMask             = Context->stencilMaskFront;
            stencilFail             = Context->stencilOpFailFront;
            stencilzFail            = Context->stencilOpDepthFailFront;
            stencilzPass            = Context->stencilOpDepthPassFront;

            /* Always pass depth test. */
            glDepthFunc(GL_ALWAYS);

            if (Mask & GL_DEPTH_BUFFER_BIT)
            {
                /* Depth test: accept new depth. */
                glDepthMask(GL_TRUE);
                glEnable(GL_DEPTH_TEST);
            }
            else
            {
                /* Not clearing depth, disable writing. */
                glDepthMask(GL_FALSE);
            }

            /* Always pass stencil test. */
            glStencilFunc(GL_ALWAYS, stencilClear, 0xFFFF);

            if (Mask & GL_STENCIL_BUFFER_BIT)
            {
                /* Clearing, replace with clear value. */
                glStencilOp(GL_REPLACE, GL_REPLACE, GL_REPLACE);
                glStencilMask(stencilWriteMask);
                glEnable(GL_STENCIL_TEST);
            }
            else
            {
                /* Not clearing, diable writing. */
                glStencilMask(0x0000);
            }
        }

        /* disable some states to render. */
        glDisable(GL_BLEND);
        glDisable(GL_CULL_FACE);

        widthViewport = (GLfloat)Context->viewportWidth;
        heightViewport = (GLfloat)Context->viewportHeight;

        Xs = (GLfloat)(Context->scissorX - Context->viewportX);
        Ys = (GLfloat)(Context->scissorY - Context->viewportY);
        Ws = (GLfloat)Context->scissorWidth;
        Hs = (GLfloat)Context->scissorHeight;

        /* Normalize coordinates. */
        normLeft   = Xs / widthViewport;
        normTop    = Ys / heightViewport;
        normWidth  = Ws / widthViewport;
        normHeight = Hs / heightViewport;

        /* Transform coordinates. */
        left   = normLeft   * 2.0f  - 1.0f;
        top    = normTop    * 2.0f  - 1.0f;
        right  = normWidth  * 2.0f  + left;
        bottom = normHeight * 2.0f  + top;

        /* Create two triangles. */
        vertexBuffer[ 0] = left;
        vertexBuffer[ 1] = top;
        vertexBuffer[ 2] = depthClear;

        vertexBuffer[ 3] = right;
        vertexBuffer[ 4] = top;
        vertexBuffer[ 5] = depthClear;

        vertexBuffer[ 6] = left;
        vertexBuffer[ 7] = bottom;
        vertexBuffer[ 8] = depthClear;

        vertexBuffer[ 9] = right;
        vertexBuffer[10] = bottom;
        vertexBuffer[11] = depthClear;

        /* Initialize 3D clear shader */
        if (!Context->clearShaderExist)
        {
            Context->clearProgram = glCreateProgram();
            Context->clearVertShader = glCreateShader(GL_VERTEX_SHADER);
            glShaderSource(Context->clearVertShader, 1, (const char**)&vertSource, gcvNULL);
            glCompileShader(Context->clearVertShader);
            glGetShaderiv(Context->clearVertShader, GL_COMPILE_STATUS, &shaderCompiled);

            if (!shaderCompiled)
            {
	            gcmTRACE_ZONE(gcvLEVEL_ERROR, gldZONE_DRIVER,
				              "Vertex shader compile failed\n");
                gcmONERROR(gcvSTATUS_FALSE);
            }

            Context->clearFragShader = glCreateShader(GL_FRAGMENT_SHADER);
            glShaderSource(Context->clearFragShader, 1, (const char**)&fragSource, gcvNULL);
            glCompileShader(Context->clearFragShader);
            glGetShaderiv(Context->clearFragShader, GL_COMPILE_STATUS, &shaderCompiled);

            if (!shaderCompiled)
            {
	            gcmTRACE_ZONE(gcvLEVEL_ERROR, gldZONE_DRIVER,
				              "Fragment shader compile failed\n");
                gcmONERROR(gcvSTATUS_FALSE);
            }

            glAttachShader(Context->clearProgram, Context->clearVertShader);
            glAttachShader(Context->clearProgram, Context->clearFragShader);

            glBindAttribLocation(Context->clearProgram, VERTEX_INDEX, "a_position");

            glLinkProgram(Context->clearProgram);
            glGetProgramiv(Context->clearProgram, GL_LINK_STATUS, &shaderLinked);

            if (!shaderLinked)
            {
	            gcmTRACE_ZONE(gcvLEVEL_ERROR, gldZONE_DRIVER,
				              "Program linked failed\n");
                gcmONERROR(gcvSTATUS_FALSE);
            }

            Context->clearShaderExist = GL_TRUE;
        }

        glUseProgram(Context->clearProgram);
        u_clearColorLocation = glGetUniformLocation(Context->clearProgram, "u_clearColor");
        glUniform4fv(u_clearColorLocation, 1, clearColor);

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glEnableVertexAttribArray(VERTEX_INDEX);
        glVertexAttribPointer(VERTEX_INDEX, 3, GL_FLOAT, GL_FALSE, 0, vertexBuffer);

        rectPrimAvail = gcoHAL_IsFeatureAvailable(gcvNULL, gcvFEATURE_RECT_PRIMITIVE);

        /* Draw the clear rect. */
        if (rectPrimAvail)
        {
            glDrawArrays(0x7, 0, 4);
        }
        else
        {
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        }

        glDisableVertexAttribArray(VERTEX_INDEX);

        /* Restore color states. */
        glColorMask(colorMask[0], colorMask[1], colorMask[2], colorMask[3]);

        if (Context->depth)
        {
            /* Restore depth states. */
            glDepthFunc(depthFunc);
            glDepthMask(depthWriteMask);

            /* Restore stencil states. */
            glStencilFunc(stencilFunc, stencilRef, stencilMask);
            glStencilOp(stencilFail, stencilzFail, stencilzPass);
            glStencilMask(stencilWriteMask);
        }

        /* Restore changed states. */
        if (alphaBlend)
        {
            glEnable(GL_BLEND);
        }
        else
        {
            glDisable(GL_BLEND);
        }

        if (!depthTest)
        {
            glDisable(GL_DEPTH_TEST);
        }
        else
        {
            glEnable(GL_DEPTH_TEST);
        }

        if (!stencilTest)
        {
            glDisable(GL_STENCIL_TEST);
        }
        else
        {
            glEnable(GL_STENCIL_TEST);
        }

        if (cullEnable)
        {
            glEnable(GL_CULL_FACE);
        }
        else
        {
            glDisable(GL_CULL_FACE);
        }

        if (program)
        {
            glUseProgram(program->object.name);
        }

        if (buffer == gcvNULL)
        {
            glBindBuffer(GL_ARRAY_BUFFER, 0);
            glVertexAttribPointer(VERTEX_INDEX, size, type, normalized, stride, ptr);
        }
        else
        {
            glBindBuffer(GL_ARRAY_BUFFER, buffer->object.name);
            glVertexAttribPointer(VERTEX_INDEX, size, type, normalized, stride, ptr);
        }

        if (currentBuffer != gcvNULL)
        {
            glBindBuffer(GL_ARRAY_BUFFER, currentBuffer->object.name);
        }
        else
        {
            glBindBuffer(GL_ARRAY_BUFFER, 0);
        }

        if (attribEnable)
        {
            glEnableVertexAttribArray(VERTEX_INDEX);
        }
    }
    while (gcvFALSE);

OnError:
    /* Return status. */
    return status;
}
#endif

GL_APICALL void GL_APIENTRY glClear(GLbitfield Mask)
{
#if gcdNULL_DRIVER < 3
    glmENTER1(glmARGHEX, Mask)
    {
        gcmDUMP_API("${ES20 glClear 0x%08X}", Mask);

        /* Profile the call. */
        glmPROFILE(context, GLES2_CLEAR, 0);

#if gldPATCHES
        /* Do batching. */
        glmBATCH(context,
                 break,
                 break,
                 (glshBatchClear(context, Mask)));
#endif

        /* Do the clear. */
        glshClear(context, Mask);
    }
    glmLEAVE()
#endif
}

GLenum glshClear(IN GLContext Context, IN GLenum Mask)
{
    gceSTATUS   status;
    gctUINT     depthFlags;
    gctINT      left, top, right, bottom;
    gcoSURF     surf;
#if gldUSE_3D_RENDERING_CLEAR
    /* Collect color and depth masks, if clearing using 3D_RENDERING_CLEAR. */
    GLbitfield  renderingClearMask = 0;
#endif

#if gldFBO_DATABASE
    /* Check if we have a current database and we are not playing back. */
    if (Context->dbEnable && !Context->playing)
    {
        /* Store this clear into the database. */
        if (gcmIS_SUCCESS(glshAddClear(Context, Mask)))
        {
            /* Don't process clear now. */
            return GL_NO_ERROR;
        }

        /* Play back current database as-is. */
        if (gcmIS_ERROR(glshPlayDatabase(Context,
                                         (Context->framebuffer == gcvNULL)
                                         ? Context->database.currentDB
                                         : Context->framebuffer->database.currentDB)))
        {
            gl2mERROR(GL_INVALID_OPERATION);
            return GL_INVALID_OPERATION;
        }
    }
#endif

#if gldPATCHES
    /* Route the clear through the patch code. */
    Mask = glshPatchClear(Context, Mask);
#endif

    /* Compute scissor window. */
    left   = Context->scissorX;
    top    = Context->scissorY;
    right  = left + Context->scissorWidth;
    bottom = top  + Context->scissorHeight;

    /* Update frame buffer. */
    if (!_glshFrameBuffer(Context))
    {
        return Context->error;
    }

    if (Mask & GL_COLOR_BUFFER_BIT)
    {
        if (Context->framebuffer != gcvNULL)
        {
            surf = ((Context->framebuffer->color.object == gcvNULL)
                    ? Context->draw
                    : _glshGetFramebufferSurface(&Context->framebuffer->color));

            if (surf == gcvNULL)
            {
                /* Nothing to do. */
                status = gcvSTATUS_OK;
            }
            else
            {
                /* For some cases clear can't be done by hw, using 3D rendering to do clear will avoid driver use software to do
                 * clear. This optimization only works when clearing the context->draw buffer. */
                if (Context->scissorEnable)
                {
                    if ((left   == 0)               &&
                        (top    == 0)               &&
                        (right  >= Context->width)  &&
                        (bottom >= Context->height) )
                    {
                        /* Still fullscreen clear. */
                        status = gcoSURF_Clear(surf, gcvCLEAR_COLOR);
                    }
#if gldUSE_3D_RENDERING_CLEAR
                    else if (gcmIS_SUCCESS(gcoSURF_IsRenderable(surf))
                             &&
                             (((left   & Context->resolveMaskX)      != 0) ||
                              ((top    & Context->resolveMaskY)      != 0) ||
                              ((right  & Context->resolveMaskWidth)  != 0) ||
                              ((bottom & Context->resolveMaskHeight) != 0) )
                             )
                    {
                        /* Use 3D rendering clear. */
                        renderingClearMask |= GL_COLOR_BUFFER_BIT;
                        status              = gcvSTATUS_OK;
                    }
#endif
                    else
                    {
                        /* Partial clear. */
	                    status = gcoSURF_ClearRect(surf, left, top, right, bottom, gcvCLEAR_COLOR);
                    }
                }
                else
                {
                    status = gcoSURF_Clear(surf, gcvCLEAR_COLOR);
                }
            }

            Context->framebuffer->needResolve = GL_TRUE;
        }
        else
        {
            /* For some cases clear can't be done by hw, using 3D rendering to do clear will avoid driver use software to do clear.
             * This optimization only works when clearing the context->draw buffer. */
            if (Context->scissorEnable)
            {
                if ((left   == 0)               &&
                    (top    == 0)               &&
                    (right  >= Context->width)  &&
                    (bottom >= Context->height) )
                {
                    /* Still fullscreen clear. */
                    status = gcoSURF_Clear(Context->draw, gcvCLEAR_COLOR);
                }
#if gldUSE_3D_RENDERING_CLEAR
                else if (((left   & Context->resolveMaskX)      != 0) ||
                         ((top    & Context->resolveMaskY)      != 0) ||
                         ((right  & Context->resolveMaskWidth)  != 0) ||
                         ((bottom & Context->resolveMaskHeight) != 0) )
                {
                    /* Use 3D rendering clear. */
                    renderingClearMask |= GL_COLOR_BUFFER_BIT;
                    status              = gcvSTATUS_OK;
                }
#endif
                else
                {
                    /* Partial clear. */
                    status = gcoSURF_ClearRect(Context->draw, left, top, right, bottom, gcvCLEAR_COLOR);
                }
            }
            else
            {
                status = gcoSURF_Clear(Context->draw, gcvCLEAR_COLOR);
            }
        }

        if (gcmIS_ERROR(status))
        {
            gl2mERROR(GL_INVALID_OPERATION);
            return GL_INVALID_OPERATION;
        }
    }

    depthFlags = 0;

    if (Mask & GL_DEPTH_BUFFER_BIT)
    {
        depthFlags |= gcvCLEAR_DEPTH;
    }

    if (Mask & GL_STENCIL_BUFFER_BIT)
    {
        depthFlags |= gcvCLEAR_STENCIL;
    }

    if (depthFlags != 0)
    {
        if (Context->framebuffer != gcvNULL)
        {
            /* Update frame buffer. */
            if (!_glshFrameBuffer(Context))
            {
                return Context->error;
            }

            surf = ((Context->framebuffer->depth.object == gcvNULL)
                    ? gcvNULL
                    : _glshGetFramebufferSurface(&Context->framebuffer->depth));

            if (surf == gcvNULL)
            {
                /* Nothing to do. */
                status = gcvSTATUS_OK;
            }
            else
            {
                /* For some cases clear can't be done by hw, using 3D rendering to do clear will avoid driver use software to do
                 * clear. This optimization only works when clearing the context->draw buffer.
                 * For stencil buffer clear, we should use 3D rendering to do clear with mask bits.
                 */
                if (Context->scissorEnable)
                {
                    if ((left   == 0)               &&
                        (top    == 0)               &&
                        (right  >= Context->width)  &&
                        (bottom >= Context->height) )
                    {
                        /* Still fullscreen clear. */
                        status = gcoSURF_Clear(surf, depthFlags);
                    }
#if gldUSE_3D_RENDERING_CLEAR
                    else if (gcmIS_SUCCESS(gcoSURF_IsRenderable(surf))
                             &&
                             (((left   & Context->resolveMaskX)      != 0) ||
                              ((top    & Context->resolveMaskY)      != 0) ||
                              ((right  & Context->resolveMaskWidth)  != 0) ||
                              ((bottom & Context->resolveMaskHeight) != 0) )
                             )
                    {
                        /* Use 3D rendering clear. */
                        renderingClearMask |= Mask & ~GL_COLOR_BUFFER_BIT;
                        status              = gcvSTATUS_OK;
                    }
#endif
                    else
                    {
                        /* Partial clear. */
                        status = gcoSURF_ClearRect(surf, left, top, right, bottom, depthFlags);
                    }
                }
                else
                {
                    status = gcoSURF_Clear(surf, depthFlags);
                }
            }

            Context->framebuffer->needResolve = GL_TRUE;
        }
        else
        {
            if (Context->depth == gcvNULL)
            {
                /* Nothing to do. */
                status = gcvSTATUS_OK;
            }
            else
            {
                /* For some cases clear can't be done by hw, using 3D rendering to do clear will avoid driver use software to do
                 * clear. This optimization only works when clearing the context->draw buffer. */
                if (Context->scissorEnable)
                {
                    if ((left   == 0)               &&
                        (top    == 0)               &&
                        (right  >= Context->width)  &&
                        (bottom >= Context->height) )
                    {
                        /* Still fullscreen clear. */
                        status = gcoSURF_Clear(Context->depth, depthFlags);
                    }
#if gldUSE_3D_RENDERING_CLEAR
                    else if (((left   & Context->resolveMaskX)      != 0) ||
                             ((top    & Context->resolveMaskY)      != 0) ||
                             ((right  & Context->resolveMaskWidth)  != 0) ||
                             ((bottom & Context->resolveMaskHeight) != 0))
                    {
                        /* Use 3D rendering clear. */
                        renderingClearMask |= Mask & ~GL_COLOR_BUFFER_BIT;
                        status              = gcvSTATUS_OK;
                    }
#endif
                    else
                    {
                        /* Partial clear. */
                        status = gcoSURF_ClearRect(Context->depth, left, top, right, bottom, depthFlags);
                    }
                }
                else
                {
                    status = gcoSURF_Clear(Context->depth, depthFlags);
                }
            }
        }

        if (gcmIS_ERROR(status))
        {
            gl2mERROR(GL_INVALID_OPERATION);
            return GL_INVALID_OPERATION;
        }
    }

#if gldUSE_3D_RENDERING_CLEAR
    if (renderingClearMask != 0)
    {
		/* Clear color and/or depth/stencil buffers using shader. */
        if (gcmIS_ERROR(_ClearRect(Context, renderingClearMask)))
        {
            gl2mERROR(GL_INVALID_OPERATION);
            return GL_INVALID_OPERATION;
        }
    }
#endif

    /* Disable batch optmization. */
    Context->batchDirty = GL_TRUE;

    /* Success. */
    return GL_NO_ERROR;
}
