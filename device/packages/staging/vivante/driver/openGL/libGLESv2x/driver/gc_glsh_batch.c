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

#if gldPATCHES


#if gcmIS_DEBUG(gcdDEBUG_TRACE)
#   define glmBATCH_ENTER() gcmPRINT("++%s(%d)", __FUNCTION__, __LINE__)
#   define glmBATCH_LEAVE() gcmPRINT("--%s(%d)", __FUNCTION__, __LINE__)
#else
#   define glmBATCH_ENTER() glmBATCH_NOP
#   define glmBATCH_LEAVE() glmBATCH_NOP
#endif

#define glmBATCH_START(_context) \
    { \
        glsBATCH_STATE * batch; \
        glsBATCH_QUEUE * _current_batch = _context->batchInfo.current; \
        if (_current_batch == gcvNULL) { \
            _current_batch = glshBatchCurrent(_context); \
            if (_current_batch == gcvNULL) { \
                glmBATCH_LEAVE(); \
                return GL_OUT_OF_MEMORY; \
            } \
        } \
        batch = &_current_batch->state;

#define glmBATCH_END(_result) \
    } \
    glmBATCH_LEAVE(); \
    return _result;

#define glmBATCH_CALL(_func) \
    { \
        GLenum result = _func; \
        if (result != GL_NO_ERROR) { \
            glmBATCH_LEAVE(); \
            return result; \
        } \
    } \
    glmBATCH_NOP


#define glmBATCH_FIELD(_field) \
    Context->batchInfo.activeState._field

#define glmBATCH_DIFF(_field) \
    (glmBATCH_FIELD(_field) != State->_field)

#define glmBATCH_ARG(_field) \
    glmBATCH_FIELD(_field) = State->_field

static GLvoid glshParseStateAttribute(IN GLContext Context,
                                      IN glsBATCH_STATE * State)
{
    GLbitfield  attributes;
    gctINT      i;

    glmBATCH_ENTER();

    /* Get attributes flags. */
    attributes = State->flags.attributes;

    /* Process all vertices. */
    for (i = 0; (i < gldVERTEX_ELEMENT_COUNT) && (attributes != 0); i++, attributes >>= 1)
    {
        /* Test if this vertex attribute has changed. */
        if (attributes & 1)
        {
            /* Test if vertex attribute genercis are valid. */
            if (State->attributes[i].flags.generics)
            {
                /* Test if vertex attribute genercis are different. */
                if (glmBATCH_DIFF(attributes[i].genericsX) ||
                    glmBATCH_DIFF(attributes[i].genericsY) ||
                    glmBATCH_DIFF(attributes[i].genericsZ) ||
                    glmBATCH_DIFF(attributes[i].genericsW) )
                {
                    /* Set the vertex attribute array generic values. */
                    glshVertexAttrib(Context,
                                     i,
                                     glmBATCH_ARG(attributes[i].genericsX),
                                     glmBATCH_ARG(attributes[i].genericsY),
                                     glmBATCH_ARG(attributes[i].genericsZ),
                                     glmBATCH_ARG(attributes[i].genericsW));
                }
            }

            /* Test if vertex attribute data is valid. */
            if (State->attributes[i].flags.data)
            {
                /* Test if vertex attribute data is different. */
                if (glmBATCH_DIFF(attributes[i].size)       ||
                    glmBATCH_DIFF(attributes[i].type)       ||
                    glmBATCH_DIFF(attributes[i].normalized) ||
                    glmBATCH_DIFF(attributes[i].stride)     ||
                    glmBATCH_DIFF(attributes[i].pointer)    ||
                    glmBATCH_DIFF(attributes[i].buffer)     )
                {
                    /* Check if there is allocated vertex data. */
                    if (Context->batchInfo.activeState.attributes[i].copied)
                    {
                        /* Free the allocated vertex memory. */
                        glshBatchFree(Context, (GLvoid *) Context->batchInfo.activeState.attributes[i].pointer);
                    }

                    /* Set the vertex attribute data. */
                    glshVertexAttribPointer(Context,
                                            i,
                                            glmBATCH_ARG(attributes[i].size),
                                            glmBATCH_ARG(attributes[i].type),
                                            glmBATCH_ARG(attributes[i].normalized),
                                            glmBATCH_ARG(attributes[i].stride),
                                            glmBATCH_ARG(attributes[i].pointer),
                                            glmBATCH_ARG(attributes[i].buffer));

                    /* Copy the copied flag. */
                    glmBATCH_ARG(attributes[i].copied);
                }
            }

            /* Test if vertex attribute enable bit is valid. */
            if (State->attributes[i].flags.enable)
            {
                /* Test if vertex attribute enable bit is different. */
                if (glmBATCH_DIFF(attributes[i].enable))
                {
                    /* Enable or disable the vertex attribute array. */
                    glshVertexAttribArray(Context, i, glmBATCH_ARG(attributes[i].enable));
                }
            }
        }
    }

    glmBATCH_LEAVE();
}

static GLvoid glshParseUniform(IN GLContext Context,
                               IN glsBATCH_STATE_UNIFORM * Uniform)
{
    glsBATCH_STATE_UNIFORM *    uniform;
    glsBATCH_STATE_UNIFORM *    next;

    glmBATCH_ENTER();

    /* Process all uniforms. */
    for (uniform = Uniform; uniform != gcvNULL; uniform = next)
    {
        /* Get next uniform. */
        next = uniform->next;

        /* Set uniform data. */
        glshSetUniformData(Context->program, uniform->uniform, uniform->type, uniform->count, uniform->index, uniform->data);

        /* Free the batched uniform. */
        glshBatchFree(Context, uniform);
    }

    glmBATCH_LEAVE();
}

#define glmBATCH_2D_PARAMETER(_index, _field, _param) \
    if (texture->batchedState.flags._field && (texture->batchedState._field != texture->_field)) \
    { \
        glshSetTexParameter(Context, texture, GL_TEXTURE_2D, _param, texture->batchedState._field); \
    } \
    glmBATCH_NOP

static GLvoid glshParseTexture2D(IN GLContext Context,
                                 IN glsBATCH_STATE * State)
{
    GLint       i;
    GLbitfield  flags;
    GLTexture   texture;

    glmBATCH_ENTER();

    /* Get flags of changed textures. */
    flags = State->flags.textures2D;

    /* Process all changed textures. */
    for (i = 0; (i < gldTEXTURE_SAMPLER_COUNT) && (flags != 0); i++, flags >>= 1)
    {
        /* Test if this texture is changed. */
        if (flags & 1)
        {
            /* Select the active texture. */
            glshActiveTexture(Context, GL_TEXTURE0 + i);

            /* Test if the texture binding has changed. */
            if (glmBATCH_DIFF(textures2D[i]))
            {
                /* Bind texture. */
                glshBindTexture(Context, GL_TEXTURE_2D, (glmBATCH_ARG(textures2D[i]))->texture);
            }

            /* Get the current active texture object. */
            texture = Context->texture2D[i];
            if (texture == gcvNULL)
            {
                texture = &Context->default2D;
            }

            /* Test if the wrap S has changed. */
            glmBATCH_2D_PARAMETER(i, wrapS, GL_TEXTURE_WRAP_S);

            /* Test if the wrap T has changed. */
            glmBATCH_2D_PARAMETER(i, wrapT, GL_TEXTURE_WRAP_T);

            /* Test if the wrap R has changed. */
            glmBATCH_2D_PARAMETER(i, wrapR, GL_TEXTURE_WRAP_R_OES);

            /* Test if the minification filter has changed. */
            glmBATCH_2D_PARAMETER(i, minFilter, GL_TEXTURE_MIN_FILTER);

            /* Test if the magnification filter has changed. */
            glmBATCH_2D_PARAMETER(i, magFilter, GL_TEXTURE_MAG_FILTER);

            /* Test if the anisotropy has changed. */
            glmBATCH_2D_PARAMETER(i, anisotropy, GL_TEXTURE_MAX_ANISOTROPY_EXT);

            /* Test if the maximum mipmap level has changed. */
            glmBATCH_2D_PARAMETER(i, maxLevel, GL_TEXTURE_MAX_LEVEL_APPLE);

            /* Zero the flags. */
            gcoOS_ZeroMemory(&texture->batchedState.flags, gcmSIZEOF(texture->batchedState.flags));
        }
    }

    glmBATCH_LEAVE();
}

#define glmBATCH_CUBIC_PARAMETER(_index, _field, _param) \
    if (texture->batchedState.flags._field && (texture->batchedState._field != texture->_field)) \
    { \
        glshSetTexParameter(Context, texture, GL_TEXTURE_CUBE_MAP, _param, texture->batchedState._field); \
    } \
    glmBATCH_NOP

static GLvoid glshParseTextureCubic(IN GLContext Context,
                                    IN glsBATCH_STATE * State)
{
    GLint       i;
    GLbitfield  flags;
    GLTexture   texture;

    glmBATCH_ENTER();

    /* Get flags of changed textures. */
    flags = State->flags.texturesCubic;

    /* Process all changed textures. */
    for (i = 0; (i < gldTEXTURE_SAMPLER_COUNT) && (flags != 0); i++, flags >>= 1)
    {
        /* test if this texture is changed. */
        if (flags & 1)
        {
            /* Select the active texture. */
            glshActiveTexture(Context, GL_TEXTURE0 + i);

            /* Test if the texture binding has changed. */
            if (glmBATCH_DIFF(texturesCubic[i]))
            {
                /* Bind texture. */
                glshBindTexture(Context, GL_TEXTURE_CUBE_MAP, (glmBATCH_ARG(texturesCubic[i]))->texture);
            }

            /* Get the current active texture object. */
            texture = Context->textureCube[i];
            if (texture == gcvNULL)
            {
                texture = &Context->defaultCube;
            }

            /* Test if the wrap S has changed. */
            glmBATCH_CUBIC_PARAMETER(i, wrapS, GL_TEXTURE_WRAP_S);

            /* Test if the wrap T has changed. */
            glmBATCH_CUBIC_PARAMETER(i, wrapT, GL_TEXTURE_WRAP_T);

            /* Test if the wrap R has changed. */
            glmBATCH_CUBIC_PARAMETER(i, wrapR, GL_TEXTURE_WRAP_R_OES);

            /* Test if the minification filter has changed. */
            glmBATCH_CUBIC_PARAMETER(i, minFilter, GL_TEXTURE_MIN_FILTER);

            /* Test if the magnification filter has changed. */
            glmBATCH_CUBIC_PARAMETER(i, magFilter, GL_TEXTURE_MAG_FILTER);

            /* Test if the anisotropy has changed. */
            glmBATCH_CUBIC_PARAMETER(i, anisotropy, GL_TEXTURE_MAX_ANISOTROPY_EXT);

            /* Test if the maximum mipmap level has changed. */
            glmBATCH_CUBIC_PARAMETER(i, maxLevel, GL_TEXTURE_MAX_LEVEL_APPLE);

            /* Zero the flags. */
            gcoOS_ZeroMemory(&texture->batchedState.flags, gcmSIZEOF(texture->batchedState.flags));
        }
    }

    glmBATCH_LEAVE();
}

#define glmBATCH_3D_PARAMETER(_index, _field, _param) \
    if (texture->batchedState.flags._field && (texture->batchedState._field != texture->_field)) \
    { \
        glshSetTexParameter(Context, texture, GL_TEXTURE_3D_OES, _param, texture->batchedState._field); \
    } \
    glmBATCH_NOP

static GLvoid glshParseTexture3D(IN GLContext Context,
                                 IN glsBATCH_STATE * State)
{
    GLint       i;
    GLbitfield  flags;
    GLTexture   texture;

    glmBATCH_ENTER();

    /* Get flags of changed textures. */
    flags = State->flags.textures3D;

    /* Process all changed textures. */
    for (i = 0; (i < gldTEXTURE_SAMPLER_COUNT) && (flags != 0); i++, flags >>= 1)
    {
        /* test if this texture is changed. */
        if (flags & 1)
        {
            /* Select the active texture. */
            glshActiveTexture(Context, GL_TEXTURE0 + i);

            /* Test if the texture binding has changed. */
            if (glmBATCH_DIFF(textures3D[i]))
            {
                /* Bind texture. */
                glshBindTexture(Context, GL_TEXTURE_3D_OES, (glmBATCH_ARG(textures3D[i]))->texture);
            }

            /* Get the current active texture object. */
            texture = Context->texture3D[i];
            if (texture == gcvNULL)
            {
                texture = &Context->default3D;
            }

            /* Test if the wrap S has changed. */
            glmBATCH_3D_PARAMETER(i, wrapS, GL_TEXTURE_WRAP_S);

            /* Test if the wrap T has changed. */
            glmBATCH_3D_PARAMETER(i, wrapT, GL_TEXTURE_WRAP_T);

            /* Test if the wrap R has changed. */
            glmBATCH_3D_PARAMETER(i, wrapR, GL_TEXTURE_WRAP_R_OES);

            /* Test if the minification filter has changed. */
            glmBATCH_3D_PARAMETER(i, minFilter, GL_TEXTURE_MIN_FILTER);

            /* Test if the magnification filter has changed. */
            glmBATCH_3D_PARAMETER(i, magFilter, GL_TEXTURE_MAG_FILTER);

            /* Test if the anisotropy has changed. */
            glmBATCH_3D_PARAMETER(i, anisotropy, GL_TEXTURE_MAX_ANISOTROPY_EXT);

            /* Test if the maximum mipmap level has changed. */
            glmBATCH_3D_PARAMETER(i, maxLevel, GL_TEXTURE_MAX_LEVEL_APPLE);

            /* Zero the flags. */
            gcoOS_ZeroMemory(&texture->batchedState.flags, gcmSIZEOF(texture->batchedState.flags));
        }
    }

    glmBATCH_LEAVE();
}

#define glmFLAG_DIFFERENT_CAP(_flag, _1, _cap) \
    if (State->flags._flag && glmBATCH_DIFF(_1)) \
    { \
        if ((glmBATCH_FIELD(_1) = State->_1) != GL_FALSE) glshEnable(Context, _cap); \
        else glshDisable(Context, _cap); \
    } \
    glmBATCH_NOP

#define glmFLAG_DIFFERENT_1(_flag, _func, _1) \
    if (State->flags._flag && glmBATCH_DIFF(_1)) \
    { \
        _func(Context, glmBATCH_ARG(_1)); \
    } \
    glmBATCH_NOP

#define glmFLAG_DIFFERENT_2(_flag, _func, _1, _2) \
    if (State->flags._flag && (glmBATCH_DIFF(_1) || glmBATCH_DIFF(_2))) \
    { \
        _func(Context, glmBATCH_ARG(_1), glmBATCH_ARG(_2)); \
    } \
    glmBATCH_NOP

#define glmFLAG_DIFFERENT_4(_flag, _func, _1, _2, _3, _4) \
    if (State->flags._flag && (glmBATCH_DIFF(_1) || glmBATCH_DIFF(_2) || glmBATCH_DIFF(_3) || glmBATCH_DIFF(_4))) \
    { \
        _func(Context, glmBATCH_ARG(_1), glmBATCH_ARG(_2), glmBATCH_ARG(_3), glmBATCH_ARG(_4)); \
    } \
    glmBATCH_NOP

#define glmFLAG_DIFFERENT_TARGET_1(_flag, _func, _target, _1) \
    if (State->flags._flag && (glmBATCH_DIFF(_1))) \
    { \
        _func(Context, _target, glmBATCH_ARG(_1)); \
    } \
    glmBATCH_NOP

#define glmFLAG_DIFFERENT_TARGET_3(_flag, _func, _target, _1, _2, _3) \
    if (State->flags._flag && (glmBATCH_DIFF(_1) || glmBATCH_DIFF(_2) || glmBATCH_DIFF(_3))) \
    { \
        _func(Context, _target, glmBATCH_ARG(_1), glmBATCH_ARG(_2), glmBATCH_ARG(_3)); \
    } \
    glmBATCH_NOP

static GLvoid glshParseState(IN GLContext Context,
                             IN glsBATCH_STATE * State)
{
    glmBATCH_ENTER();

    /* Test if attributes are touched. */
    if (State->flags.attributes)
    {
        /* Parse attribute states. */
        glshParseStateAttribute(Context, State);
    }

    /* Test if the program has changed. */
    glmFLAG_DIFFERENT_1(program, glshSetProgram, program);

    /* Tests if uniforms are touched. */
    if (State->flags.uniform)
    {
        /* Parse the uniforms. */
        glshParseUniform(Context, State->uniform);
    }

    /* Test if the depth range has changed. */
    glmFLAG_DIFFERENT_2(depthRange, glshDepthRangef, depthNear, depthFar);

    /* Test if the viewport has changed. */
    glmFLAG_DIFFERENT_4(viewport, glshViewport, viewportX, viewportY, viewportWidth, viewportHeight);

    /* Test if line width has changed. */
    glmFLAG_DIFFERENT_1(lineWidth, glshLineWidth, lineWidth);

    /* Test if the front face has changed. */
    glmFLAG_DIFFERENT_1(frontFace, glshFrontFace, frontFace);

    /* Test if the cull face has changed. */
    glmFLAG_DIFFERENT_1(cullFace, glshCullFace, cullFace);

    /* Test if the cull enable has changed. */
    glmFLAG_DIFFERENT_CAP(cullEnable, cullEnable, GL_CULL_FACE);

    /* Test if the polygon offset has changed. */
    glmFLAG_DIFFERENT_2(polygonOffset, glshPolygonOffset, offsetFactor, offsetUnits);

    /* Test if the polygon offset enable has changed. */
    glmFLAG_DIFFERENT_CAP(polygonOffsetEnable, offsetEnable, GL_POLYGON_OFFSET_FILL);

    /* Test if the 2D textures are touched. */
    if (State->flags.textures2D)
    {
        /* Parse the 2D textures. */
        glshParseTexture2D(Context, State);
    }

    /* Test if the cubic textures are touched. */
    if (State->flags.texturesCubic)
    {
        /* Parse the cubic textures. */
        glshParseTextureCubic(Context, State);
    }

    /* Test if the 3D textures are touched. */
    if (State->flags.textures3D)
    {
        /* Parse the 3D textures. */
        glshParseTexture3D(Context, State);
    }

    /* Test if the scissor has changed. */
    glmFLAG_DIFFERENT_4(scissor, glshScissor, scissorX, scissorY, scissorWidth, scissorHeight);

    /* Test if the scissor enable has changed. */
    glmFLAG_DIFFERENT_CAP(scissorEnable, scissorEnable, GL_SCISSOR_TEST);

    /* Test if the stencil test has changed. */
    glmFLAG_DIFFERENT_CAP(stencilTestEnable, stencilTestEnable, GL_STENCIL_TEST);

    /* Test if the front stencil function has changed. */
    glmFLAG_DIFFERENT_TARGET_3(stencilFuncFront, glshStencilFuncSeparate,
                               GL_FRONT, stencilFuncFront, stencilRefFront, stencilRefMaskFront);

    /* Test if the back stencil function has changed. */
    glmFLAG_DIFFERENT_TARGET_3(stencilFuncBack, glshStencilFuncSeparate,
                               GL_BACK, stencilFuncBack, stencilRefBack, stencilRefMaskBack);

    /* Test if the front stencil operation has changed. */
    glmFLAG_DIFFERENT_TARGET_3(stencilOpFront, glshStencilOpSeparate,
                               GL_FRONT, stencilFailFront, stencilDepthFailFront, stencilDepthPassFront);

    /* Test if the back stencil operation has changed. */
    glmFLAG_DIFFERENT_TARGET_3(stencilOpBack, glshStencilOpSeparate,
                               GL_BACK, stencilFailBack, stencilDepthFailBack, stencilDepthPassBack);

    /* Test if depth test has changed. */
    glmFLAG_DIFFERENT_CAP(depthTestEnable, depthTestEnable, GL_DEPTH_TEST);

    /* Test if the depth function has changed. */
    glmFLAG_DIFFERENT_1(depthFunc, glshDepthFunc, depthFunc);

    /* Test if the blend enable has changed. */
    glmFLAG_DIFFERENT_CAP(blendEnable, blendEnable, GL_BLEND);

    /* Test if the blend equation has changed. */
    glmFLAG_DIFFERENT_2(blendMode, glshBlendEquationSeparate, blendModeRGB, blendModeAlpha);

    /* Test if the blend function has changed. */
    glmFLAG_DIFFERENT_4(blendFunc, glshBlendFuncSeparate,
                        blendFuncSourceRGB, blendFuncSourceAlpha, blendFuncDestinationRGB, blendFuncDestinationAlpha);

    /* Test if the blend color has changed. */
    glmFLAG_DIFFERENT_4(blendColor, glshBlendColor, blendColorRed, blendColorGreen, blendColorBlue, blendColorAlpha);

    /* Test if the blend color has changed. */
    glmFLAG_DIFFERENT_4(blendColor, glshBlendColor, blendColorRed, blendColorGreen, blendColorBlue, blendColorAlpha);

    /* Test if the dither enable has changed. */
    glmFLAG_DIFFERENT_CAP(ditherEnable, ditherEnable, GL_DITHER);

    /* Test if the color mask has changed. */
    glmFLAG_DIFFERENT_4(colorMask, glshColorMask, colorMaskRed, colorMaskGreen, colorMaskBlue, colorMaskAlpha);

    /* Test if the depth mask has changed. */
    glmFLAG_DIFFERENT_1(depthMask, glshDepthMask, depthMask);

    /* Test if the front stencil mask has changed. */
    glmFLAG_DIFFERENT_TARGET_1(stencilMaskFront, glshStencilMaskSeparate, GL_FRONT, stencilMaskFront);

    /* Test if the back stencil mask has changed. */
    glmFLAG_DIFFERENT_TARGET_1(stencilMaskBack, glshStencilMaskSeparate, GL_BACK, stencilMaskBack);

    /* Test if the clear color has changed. */
    glmFLAG_DIFFERENT_4(clearColor, glshClearColor, clearColorRed, clearColorGreen, clearColorBlue, clearColorAlpha);

    /* Test if the clear depth has changed. */
    glmFLAG_DIFFERENT_1(clearDepth, glshClearDepthf, clearDepth);

    /* Test if the clear stencil has changed. */
    glmFLAG_DIFFERENT_1(clearStencil, glshClearStencil, clearStencil);

    /* Test if the framebuffer has changed. */
    glmFLAG_DIFFERENT_TARGET_1(framebuffer, glshBindFramebuffer, GL_FRAMEBUFFER, framebuffer);

    glmBATCH_LEAVE();
}


static gctPOINTER glshBatchThread(gctPOINTER Context)
{
    GLContext           context = (GLContext) Context;
    gceSTATUS           status;
    GLuint              index;
    glsBATCH_QUEUE *    batch;

    glmBATCH_ENTER();

    /* Copy the TLS from the source thread. */
    if (gcmIS_ERROR(gcoOS_CopyTLS(context->batchInfo.tls)))
    {
        /* Error! */
        glmBATCH_LEAVE();
        return gcvNULL;
    }

    /* Thread run loop. */
    for (;;)
    {
        /* Check if there are batches to process. */
        while (context->batchInfo.consumer.counter == context->batchInfo.producer.counter)
        {
            /* Wait for the signal. */
            gcmERR_BREAK(gcoOS_WaitSignal(gcvNULL, context->batchInfo.consumer.signal, gcvINFINITE));
        }

        /* Compute index into batch. */
        index = context->batchInfo.consumer.counter % gcmCOUNTOF(context->batchInfo.queue);

        /* Get pointer to the batch. */
        batch = &context->batchInfo.queue[index];

        /* Dispatch on command. */
        switch (batch->u.command)
        {
            case glvBATCH_STOP:
                /* Terminate thread. */
                glmBATCH_LEAVE();
                return gcvNULL;

            case glvBATCH_SYNC:
                /* Parse the modified states. */
                glshParseState(context, &batch->state);

                /* Commit and possible stall. */
                gcoHAL_Commit(gcvNULL, batch->u.sync.stall);

                /* Signal the sync signal. */
                gcoOS_Signal(gcvNULL, context->batchInfo.syncSignal, gcvTRUE);
                break;

            case glvBATCH_DRAW_ARRAYS:
                /* Parse the modified states. */
                glshParseState(context, &batch->state);

                /* Draw arrays. */
                glshDrawArrays(context,
                               batch->u.drawArrays.mode,
                               batch->u.drawArrays.first,
                               batch->u.drawArrays.count);
                break;

            case glvBATCH_DRAW_ELEMENTS:
                /* Parse the modified states. */
                glshParseState(context, &batch->state);

                /* Draw elements. */
                glshDrawElements(context,
                                 batch->u.drawElements.mode,
                                 batch->u.drawElements.count,
                                 batch->u.drawElements.type,
                                 batch->u.drawElements.indices,
                                 batch->u.drawElements.elementArrayBuffer);

                /* Check if there is allocated element memory. */
                if (batch->u.drawElements.elementArrayBuffer == gcvNULL)
                {
                    /* Free the element memory. */
                    glshBatchFree(context, (GLvoid *) batch->u.drawElements.indices);
                }
                break;

            case glvBATCH_CLEAR:
                /* Parse the modified states. */
                glshParseState(context, &batch->state);

                /* Perform the clear. */
                glshClear(context, batch->u.clear.flags);
                break;

            case glvBATCH_FLUSH:
                /* Parse the modified states. */
                glshParseState(context, &batch->state);

                /* Commit. */
                gcoHAL_Commit(gcvNULL, gcvFALSE);
                break;

            case glvBATCH_SWAPBUFFER:
                /* Parse the modified states. */
                glshParseState(context, &batch->state);

                /* Swap the buffers. */
                (*batch->u.swapbuffer.callback)(batch->u.swapbuffer.display, batch->u.swapbuffer.surface);
                break;
        }

        /* Increment consumer count. */
        context->batchInfo.consumer.counter += 1;

        /* Signal the producer. */
        gcmERR_BREAK(gcoOS_Signal(gcvNULL, context->batchInfo.producer.signal, gcvTRUE));

        /* Check if we have more than one heap. */
        if ((context->batchInfo.heap != gcvNULL) && (context->batchInfo.heap->next != gcvNULL))
        {
            /* Compact the memory heap. */
            glshBatchCompact(context);
        }
    }

    /* Terminate thread. */
    glmBATCH_LEAVE();
    return gcvNULL;
}

static glsBATCH_QUEUE * glshBatchCurrent(IN GLContext Context)
{
    gceSTATUS           status;
    gctINT              index;
    gctSIZE_T           i;
    glsBATCH_QUEUE *    current;

    glmBATCH_ENTER();

    /* Get the current batch. */
    current = Context->batchInfo.current;

    /* Check if we need to allocate one. */
    if (current == gcvNULL)
    {
        /* Wait until there is room in the batch queue. */
        while ((Context->batchInfo.producer.counter - Context->batchInfo.consumer.counter) == gcmCOUNTOF(Context->batchInfo.queue))
        {
            /* Wait util the consumer is finished. */
            gcmONERROR(gcoOS_WaitSignal(gcvNULL, Context->batchInfo.producer.signal, gcvINFINITE));
        }

        /* Compute index into queue. */
        index = Context->batchInfo.producer.counter % gcmCOUNTOF(Context->batchInfo.queue);

        /* Get current batch from the queue. */
        current = Context->batchInfo.current = &Context->batchInfo.queue[index];

        /* Zero out the flags. */
        gcoOS_ZeroMemory(&current->state.flags, gcmSIZEOF(current->state.flags));

        /* Zero out the vertex flags. */
        for (i = 0; i < gldVERTEX_ELEMENT_COUNT; i++)
        {
            gcoOS_ZeroMemory(&current->state.attributes[i].flags, gcmSIZEOF(current->state.attributes[i].flags));
        }
    }

    /* Return current batch. */
    glmBATCH_LEAVE();
    return current;

OnError:
    /* Error - no batch returned. */
    glmBATCH_LEAVE();
    return gcvNULL;
}

static GLenum glshBatchEnqueue(IN GLContext Context)
{
    glmBATCH_ENTER();

    /* Increment the consumer count. */
    Context->batchInfo.producer.counter += 1;

    /* Signal the consumer. */
    if (gcmIS_ERROR(gcoOS_Signal(gcvNULL, Context->batchInfo.consumer.signal, gcvTRUE)))
    {
        /* Something is wrong. */
        glmBATCH_LEAVE();
        return GL_INVALID_OPERATION;
    }

    /* Current batch has been enqueued. */
    Context->batchInfo.current = gcvNULL;

    /* Success. */
    glmBATCH_LEAVE();
    return GL_NO_ERROR;
}


/*
 *  Initialize the batch states by copying the current GL context states.
 */
GLenum glshBatchStart(IN GLContext Context)
{
    gceSTATUS           status;

    glmBATCH_ENTER();

    /* Get the TLS. */
    gcmONERROR(gcoOS_GetTLS(&Context->batchInfo.tls));

    /* Zero out the producer/consumer and thread. */
    Context->batchInfo.producer.counter = 0;
    Context->batchInfo.producer.signal  = gcvNULL;
    Context->batchInfo.consumer.counter = 0;
    Context->batchInfo.consumer.signal  = gcvNULL;
    Context->batchInfo.syncSignal       = gcvNULL;
    Context->batchInfo.thread           = gcvNULL;
    Context->batchInfo.current          = gcvNULL;
    Context->batchInfo.heap             = gcvNULL;

    /* Create the signals. */
    gcmONERROR(gcoOS_CreateSignal(gcvNULL, gcvFALSE, &Context->batchInfo.producer.signal));
    gcmONERROR(gcoOS_CreateSignal(gcvNULL, gcvFALSE, &Context->batchInfo.consumer.signal));
    gcmONERROR(gcoOS_CreateSignal(gcvNULL, gcvFALSE, &Context->batchInfo.syncSignal));

    /* Create the thread. */
    gcmONERROR(gcoOS_CreateThread(gcvNULL, (gcTHREAD_ROUTINE)glshBatchThread, Context, &Context->batchInfo.thread));

    /* Initialize the active state. */
    Context->batchInfo.program       = Context->program;
    Context->batchInfo.activeTexture = Context->textureUnit;

    /* Success. */
    glmBATCH_LEAVE();
    return GL_NONE;

OnError:
    /* Roll back thread. */
    if (Context->batchInfo.thread != gcvNULL)
    {
        gcoOS_CloseThread(gcvNULL, Context->batchInfo.thread);
        Context->batchInfo.thread = gcvNULL;
    }

    /* Roll back signals. */
    if (Context->batchInfo.producer.signal != gcvNULL)
    {
        gcmVERIFY_OK(gcoOS_DestroySignal(gcvNULL, Context->batchInfo.producer.signal));
        Context->batchInfo.producer.signal = gcvNULL;
    }

    if (Context->batchInfo.consumer.signal != gcvNULL)
    {
        gcmVERIFY_OK(gcoOS_DestroySignal(gcvNULL, Context->batchInfo.consumer.signal));
        Context->batchInfo.consumer.signal = gcvNULL;
    }

    if (Context->batchInfo.syncSignal != gcvNULL)
    {
        gcmVERIFY_OK(gcoOS_DestroySignal(gcvNULL, Context->batchInfo.syncSignal));
        Context->batchInfo.syncSignal = gcvNULL;
    }

    /* Out of memory. */
    glmBATCH_LEAVE();
    return GL_OUT_OF_MEMORY;
}

/*
 *  Stop the batch thread.
 */
GLenum glshBatchStop(IN GLContext Context)
{
    glsBATCH_QUEUE *    batch;
    glsBATCH_HEAP *     heap;

    glmBATCH_ENTER();

    /* Get the current batch. */
    batch = glshBatchCurrent(Context);
    if (batch == gcvNULL)
    {
        /* Out of memory. */
        glmBATCH_LEAVE();
        return GL_OUT_OF_MEMORY;
    }

    /* Stop the thread. */
    batch->u.command = glvBATCH_STOP;
    glmBATCH_CALL((glshBatchEnqueue(Context)));

    /* Close the thread. */
    gcoOS_CloseThread(gcvNULL, Context->batchInfo.thread);
    Context->batchInfo.thread = gcvNULL;

    /* Destroy the signals. */
    gcmVERIFY_OK(gcoOS_DestroySignal(gcvNULL, Context->batchInfo.producer.signal));
    Context->batchInfo.producer.signal = gcvNULL;

    gcmVERIFY_OK(gcoOS_DestroySignal(gcvNULL, Context->batchInfo.consumer.signal));
    Context->batchInfo.consumer.signal = gcvNULL;

    gcmVERIFY_OK(gcoOS_DestroySignal(gcvNULL, Context->batchInfo.syncSignal));
    Context->batchInfo.syncSignal = gcvNULL;

    /* Free the memory heaps. */
    while (Context->batchInfo.heap != gcvNULL)
    {
        /* Unlink the heap from the linked list. */
        heap                    = Context->batchInfo.heap;
        Context->batchInfo.heap = heap->next;

        /* Free the heap. */
        gcmVERIFY_OK(gcoOS_Free(gcvNULL, heap));
    }

    /* Success. */
    glmBATCH_LEAVE();
    return GL_NO_ERROR;
}

/*
 *  Synchronize with the batch thread.
 */
GLenum glshBatchSync(IN GLContext Context,
                     IN gctBOOL Stall)
{
    glsBATCH_QUEUE *    batch;

    glmBATCH_ENTER();

    /* Get the current batch. */
    batch = glshBatchCurrent(Context);
    if (batch == gcvNULL)
    {
        /* Out of memory. */
        glmBATCH_LEAVE();
        return GL_OUT_OF_MEMORY;
    }

    /* Synchronize with the batch thread. */
    batch->u.sync.command = glvBATCH_SYNC;
    batch->u.sync.stall   = Stall;
    glmBATCH_CALL(glshBatchEnqueue(Context));

    /* Wait for the sync signal. */
    if (gcmIS_ERROR(gcoOS_WaitSignal(gcvNULL, Context->batchInfo.syncSignal, gcvINFINITE)))
    {
        /* Something went wrong. */
        glmBATCH_LEAVE();
        return GL_INVALID_OPERATION;
    }

    /* Success. */
    glmBATCH_LEAVE();
    return GL_NO_ERROR;
}


static gctBOOL glshBatchUploadAttribute(IN GLContext Context,
                                        IN glsBATCH_STATE_ATTRIBUTE * Attribute,
                                        IN GLint First,
                                        IN GLsizei Count)
{
    GLint       srcStride;
    GLint       dstStride;
    GLsizei     bytes;
    GLvoid *    memory;
    GLubyte *   dst;
    GLubyte *   src;
    GLint       i;

    glmBATCH_ENTER();

    /* Make sure the data is dirty. */
    gcmASSERT(Attribute->flags.data);

    /* Dispatch on attribute type. */
    switch (Attribute->type)
    {
        case GL_BYTE:
        case GL_UNSIGNED_BYTE:
            /* Destination stride equals the size of the attribute. */
            dstStride = Attribute->size;
            break;

        case GL_SHORT:
        case GL_UNSIGNED_SHORT:
        case GL_HALF_FLOAT_OES:
            /* Destination stride equals the size of the attribute * 2-bytes per attribute. */
            dstStride = Attribute->size * 2;
            break;

        case GL_INT_10_10_10_2_OES:
        case GL_UNSIGNED_INT_10_10_10_2_OES:
            /* Destination stride equals the 4-bytes per attribute. */
            dstStride = 4;
            break;

        default:
            /* Destination stride equals the size of the attribute * 4-bytes per attribute. */
            dstStride = Attribute->size * 4;
            break;
    }

    /* Get the source stride. */
    srcStride = Attribute->stride;
    if (srcStride == 0)
    {
        /* If source stride is not specified - it is packed. */
        srcStride = dstStride;
    }

    /* Compute the number of bytes to allocate. */
    bytes = First + Count * dstStride;

    /* Allocate memory from the batch heap. */
    memory = glshBatchAllocate(Context, bytes);
    if (memory == gcvNULL)
    {
        /* Out of memory. */
        glmBATCH_LEAVE();
        return gcvFALSE;
    }

    /* Set source and destination pointers. */
    src = (GLubyte *) Attribute->pointer + First * srcStride;
    dst = (GLubyte *) memory + First * dstStride;

    /* Test if both source and destination stride are the same. */
    if (srcStride == dstStride)
    {
        /* Copy the the vertex array to batch memory. */
        gcoOS_MemCopy(dst, src, Count * dstStride);
    }
    else
    {
        /* Walk each vertex. */
        for (i = First; i < Count; i++)
        {
            /* Copy one vertex to batch memory. */
            gcoOS_MemCopy(dst, src, dstStride);

            /* Move to next vertex. */
            src += srcStride;
            dst += dstStride;
        }
    }

    /* Set new attribute pointer. */
    Attribute->pointer = memory;
    Attribute->copied  = GL_TRUE;

    /* Success. */
    glmBATCH_LEAVE();
    return gcvTRUE;
}

static gctBOOL glshBatchUploadIndex(IN GLContext Context,
                                    IN GLenum Type,
                                    IN gctCONST_POINTER Indices,
                                    IN GLsizei Count,
                                    OUT gctCONST_POINTER * Pointer,
                                    OUT GLuint * MinIndex,
                                    OUT GLuint * MaxIndex)
{
    GLsizei     bytes = 0;
    GLvoid *    memory;
    GLubyte *   src;
    GLubyte *   dst;
    GLsizei     i;
    GLuint      index = 0;
    GLuint      minIndex = ~0U, maxIndex = 0;

    glmBATCH_ENTER();

    /* Compute number of bytes required for uploading the indices. */
    switch (Type)
    {
        case GL_UNSIGNED_BYTE:
            bytes = Count;
            break;

        case GL_UNSIGNED_SHORT:
            bytes = Count * 2;
            break;

        case GL_UNSIGNED_INT:
            bytes = Count * 4;
            break;
    }

    /* Allocate memory from the batch heap. */
    memory = glshBatchAllocate(Context, bytes);
    if (memory == gcvNULL)
    {
        /* Out of memory. */
        glmBATCH_LEAVE();
        return gcvFALSE;
    }

    /* Set source and destination pointers. */
    src = (GLubyte *) Indices;
    dst = (GLubyte *) memory;

    /* Process each index. */
    for (i = 0; i < Count; i++)
    {
        /* Dispatch on index type. */
        switch (Type)
        {
            case GL_UNSIGNED_BYTE:
                /* Copy one 1-byte index. */
                index = *dst = *src;
                src  += 1;
                dst  += 1;
                break;

            case GL_UNSIGNED_SHORT:
                /* Copy one 2-byte index. */
                index = *(GLushort *) dst = *(GLushort *) src;
                src  += 2;
                dst  += 2;
                break;

            case GL_UNSIGNED_INT:
                /* Copy one 4-byte index. */
                index = *(GLuint *) dst = *(GLuint *) src;
                src  += 4;
                dst  += 4;
                break;
        }

        /* Keep track of minimum and maximum index values. */
        if (index < minIndex) minIndex = index;
        if (index > maxIndex) maxIndex = index;
    }

    /* Return pointer to index memory and minimum and maximum index values. */
    *Pointer  = memory;
    *MinIndex = minIndex;
    *MaxIndex = maxIndex;

    /* Success. */
    glmBATCH_LEAVE();
    return gcvTRUE;
}

GLvoid * glshBatchAllocate(IN GLContext Context,
                           IN GLsizei Bytes)
{
    glsBATCH_HEAP *         heap;
    GLsizei                 bytes;
    glsBATCH_HEAP_NODE *    node;

    glmBATCH_ENTER();

    /* Align the requested number of bytes to a multitude of 4. */
    bytes = gcmALIGN(Bytes, 4);

    /* Get the current heap. */
    heap = Context->batchInfo.heap;

    /* Check if the current heap has enough available bytes. */
    if ((heap == gcvNULL) || (bytes > heap->free->bytes))
    {
        /* Allocate a new heap. */
        if (gcmIS_ERROR(gcoOS_Allocate(gcvNULL,
                                       gcmMAX(bytes, gldBATCH_HEAP_SIZE) + gcmSIZEOF(glsBATCH_HEAP) + gcmSIZEOF(glsBATCH_HEAP_NODE),
                                       (gctPOINTER *) &heap)))
        {
            /* Out of memory. */
            glmBATCH_LEAVE();
            return gcvNULL;
        }

        /* Initialize the heap. */
        heap->next      = Context->batchInfo.heap;
        heap->memory    = (glsBATCH_HEAP_NODE *) (heap + 1);
        heap->free      = heap->memory;

        /* Set the new heap. */
        Context->batchInfo.heap = heap;

        /* Initialize the free node. */
        heap->free->bytes = gcmMAX(bytes, gldBATCH_HEAP_SIZE);
    }

    /* Get the next free node. */
    node = heap->free;

    /* Create a new free node. */
    heap->free        = (glsBATCH_HEAP_NODE *) ((GLubyte *) (node + 1) + bytes);
    heap->free->bytes = node->bytes - bytes - gcmSIZEOF(glsBATCH_HEAP_NODE);

    /* Mark the original free node as used. */
    node->bytes = -bytes;

    /* Return pointer to the memory. */
    glmBATCH_LEAVE();
    return node + 1;
}

GLvoid glshBatchFree(IN GLContext Context,
                     IN GLvoid * Memory)
{
    glsBATCH_HEAP_NODE *    node;

    glmBATCH_ENTER();

    /* Only process non-NULL memory. */
    if (Memory != gcvNULL)
    {
        /* Pointer to the memory node. */
        node = (glsBATCH_HEAP_NODE *) Memory - 1;

        /* Mark the node as free. */
        node->bytes = -node->bytes;
    }
}

GLvoid glshBatchCompact(IN GLContext Context)
{
    glsBATCH_HEAP *         heap;
    glsBATCH_HEAP *         previous = gcvNULL;
    glsBATCH_HEAP *         next;
    glsBATCH_HEAP_NODE *    node;

    glmBATCH_ENTER();

    /* Process all heaps. */
    for (heap = Context->batchInfo.heap; heap != gcvNULL; heap = next)
    {
        /* Process all nodes. */
        for (node = heap->memory; node < heap->free;)
        {
            /* Check if node is used. */
            if (node->bytes < 0)
            {
                /* Bail out. */
                break;
            }

            /* Move to next node. */
            node = (glsBATCH_HEAP_NODE *) ((GLubyte * ) (node + 1) + node->bytes);
        }

        /* Get pointer to next heap. */
        next = heap->next;

        /* Check if the entire heap is empty. */
        if (node == heap->free)
        {
            /* Check if there was a previous heap. */
            if (previous != gcvNULL)
            {
                /* Update the previous heap. */
                previous->next = next;
            }
            else
            {
                /* Update the current heap. */
                Context->batchInfo.heap = next;
            }

            /* Free the heap. */
            gcmVERIFY_OK(gcoOS_Free(gcvNULL, heap));
        }
        else
        {
            /* Since this heap is not free, remember its pointer for updates. */
            previous = heap;
        }
    }
}

static GLvoid glshCopyProgramUniforms(IN glsBATCH_STATE * State,
                                      IN GLProgram Program)
{
    glmBATCH_ENTER();

    /* Set the unfiorm flag. */
    State->flags.uniform = 1;

    /* Move the uniforms from the program to the batch. */
    State->uniform     = Program->batchHead;
    Program->batchHead = gcvNULL;
    Program->batchTail = gcvNULL;
}

static GLboolean glshValidatePrimitiveMode(IN GLenum Mode);


/*
 *  glDrawArrays
 */
GLenum glshBatchDrawArrays(IN GLContext Context,
                           IN GLenum Mode,
                           IN gctINT First,
                           IN gctINT Count)
{
    glsBATCH_QUEUE *            batch;
    gctINT                      i;
    glsBATCH_STATE_ATTRIBUTE *  attribute;
    GLbitfield                  attributeFlags;
    GLenum                      result;

    glmBATCH_ENTER();

    /* Validate the primitive mode. */
    if (!glshValidatePrimitiveMode(Mode))
    {
        /* Invalid mode. */
        glmBATCH_LEAVE();
        return GL_INVALID_ENUM;
    }

    /* Test for valid first and count. */
    if ((First < 0) || (Count < 0))
    {
        /* Invalid value. */
        glmBATCH_LEAVE();
        return GL_INVALID_VALUE;
    }

    /* Only process draw when count is positive. */
    if ((Count > 0)
        &&
        /* And when culling is disabled or is not set to front and back. */
        (!Context->cullEnable || (Context->cullMode != GL_FRONT_AND_BACK))
        )
    {
        /* Get the current batch. */
        batch = glshBatchCurrent(Context);
        if (batch == gcvNULL)
        {
            /* Out of memory. */
            glmBATCH_LEAVE();
            return GL_OUT_OF_MEMORY;
        }

        /* For any in-memory array - we need to copy the data. */
        attributeFlags = batch->state.flags.attributes;
        for (i = 0, attribute = batch->state.attributes;
             (i < gldVERTEX_ELEMENT_COUNT) && (attributeFlags != 0);
             i++, attribute++, attributeFlags >>= 1)
        {
            /* Has the vertex changed. */
            if ((attributeFlags & 1)
                &&
                /* Has the data changed. */
                attribute->flags.data
                &&
                /* Is it enabled? */
                (!attribute->flags.enable || attribute->enable)
                &&
                /* Is it not part of a stream? */
                (attribute->buffer == gcvNULL)
                &&
                /* Make sure it is not copied yet. */
                !attribute->copied
                )
            {
                /* Upload the vertex data into a dynamic stream. */
                if (!glshBatchUploadAttribute(Context, attribute, First, Count))
                {
                    /* Out of memory. */
                    glmBATCH_LEAVE();
                    return GL_OUT_OF_MEMORY;
                }
            }
        }

        /* Check if the current program has changed uniforms. */
        if ((Context->batchInfo.program != gcvNULL) && (Context->batchInfo.program->batchHead != gcvNULL))
        {
            /* Copy the uniforms. */
            glshCopyProgramUniforms(&batch->state, Context->batchInfo.program);
        }

        /* Initialize the batch command. */
        batch->u.drawArrays.command = glvBATCH_DRAW_ARRAYS;
        batch->u.drawArrays.mode    = Mode;
        batch->u.drawArrays.first   = First;
        batch->u.drawArrays.count   = Count;

        /* Enqueue the batch. */
        result = glshBatchEnqueue(Context);
        glmBATCH_LEAVE();
        return result;
    }

    /* Succes. */
    glmBATCH_LEAVE();
    return GL_NO_ERROR;
}

/*
 *  glDrawElements
 */
GLenum glshBatchDrawElements(IN GLContext Context,
                             IN GLenum Mode,
                             IN gctINT Count,
                             IN GLenum Type,
                             IN gctCONST_POINTER Indices)
{
    glsBATCH_QUEUE *            batch;
    gctCONST_POINTER            indices;
    gctUINT                     minIndex = 1, maxIndex = 0;
    gctINT                      i;
    glsBATCH_STATE_ATTRIBUTE *  attribute;
    GLbitfield                  attributeFlags;
    GLenum                      result;

    glmBATCH_ENTER();

    /* Validate the primitive mode. */
    if (!glshValidatePrimitiveMode(Mode))
    {
        /* Invalid mode. */
        glmBATCH_LEAVE();
        return GL_INVALID_ENUM;
    }

    /* Test for valid count. */
    if (Count < 0)
    {
        /* Invalid value. */
        glmBATCH_LEAVE();
        return GL_INVALID_VALUE;
    }

    /* Verify the index type. */
    switch (Type)
    {
        case GL_UNSIGNED_BYTE:
        case GL_UNSIGNED_SHORT:
        case GL_UNSIGNED_INT:
            break;

        default:
            /* Invalid index type. */
            glmBATCH_LEAVE();
            return GL_INVALID_ENUM;
    }

    /* Only process draw when count is positive. */
    if ((Count > 0)
        &&
        /* And when culling is disabled or is not set to front and back. */
        (!Context->cullEnable || (Context->cullMode != GL_FRONT_AND_BACK))
        )
    {
        /* Get the current batch. */
        batch = glshBatchCurrent(Context);
        if (batch == gcvNULL)
        {
            /* Out of memory. */
            glmBATCH_LEAVE();
            return GL_OUT_OF_MEMORY;
        }

        /* Get pointer to indices. */
        indices = Indices;

        /* Check if there is a buffer attached to the element array. */
        if (Context->elementArrayBuffer == gcvNULL)
        {
            /* Upload the indices into a dynamic stream. */
            if (!glshBatchUploadIndex(Context, Type, Indices, Count, &indices, &minIndex, &maxIndex))
            {
                /* Out of memory. */
                glmBATCH_LEAVE();
                return GL_OUT_OF_MEMORY;
            }
        }

        /* For any in-memory array - we need to copy the data. */
        attributeFlags = batch->state.flags.attributes;
        for (i = 0, attribute = batch->state.attributes;
             (i < gldVERTEX_ELEMENT_COUNT) && (attributeFlags != 0);
             i++, attribute++, attributeFlags >>= 1)
        {
            /* Has the vertex changed. */
            if ((attributeFlags & 1)
                &&
                /* Is it enabled. */
                Context->vertexArray[i].enable
                &&
                /* Has the data changed. */
                attribute->flags.data
                &&
                /* Is it not part of a buffer? */
                (attribute->buffer == gcvNULL)
                &&
                /* Make sure it is not copied yet. */
                !attribute->copied
                )
            {
                /* See if we need to get the index range or not. */
                if (minIndex > maxIndex)
                {
                    /* Get the index range for this batch. */
                    if (!gcmIS_SUCCESS(gcoINDEX_GetIndexRange(Context->elementArrayBuffer->index,
                                                              Type,
                                                              gcmPTR2INT(Indices),
                                                              Count,
                                                              &minIndex, &maxIndex)))
                    {
                        /* Out of memory. */
                        glmBATCH_LEAVE();
                        return GL_OUT_OF_MEMORY;
                    }
                }

                /* Upload the vertex data into a dynamic stream. */
                if (!glshBatchUploadAttribute(Context, attribute, minIndex, maxIndex - minIndex + 1))
                {
                    /* Out of memory. */
                    glmBATCH_LEAVE();
                    return GL_OUT_OF_MEMORY;
                }
            }
        }

        /* Check if the current program has changed uniforms. */
        if ((Context->batchInfo.program != gcvNULL) && (Context->batchInfo.program->batchHead != gcvNULL))
        {
            /* Copy the uniforms. */
            glshCopyProgramUniforms(&batch->state, Context->batchInfo.program);
        }

        /* Initialize the batch command. */
        batch->u.drawElements.command            = glvBATCH_DRAW_ELEMENTS;
        batch->u.drawElements.mode               = Mode;
        batch->u.drawElements.count              = Count;
        batch->u.drawElements.type               = Type;
        batch->u.drawElements.indices            = indices;
        batch->u.drawElements.elementArrayBuffer = Context->elementArrayBuffer;

        /* Enqueue the batch. */
        result = glshBatchEnqueue(Context);
        glmBATCH_LEAVE();
        return result;
    }

    /* Succes. */
    glmBATCH_LEAVE();
    return GL_NO_ERROR;
}

/*
 *  glClear
 */
GLenum glshBatchClear(IN GLContext Context,
                      IN GLbitfield Buffers)
{
    glsBATCH_QUEUE *    batch;
    GLenum              result;

    glmBATCH_ENTER();

    /* Test for valid buffer bits. */
    if (Buffers & ~(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT))
    {
        /* Invalid buffers. */
        glmBATCH_LEAVE();
        return GL_INVALID_VALUE;
    }

    /* Get current batch. */
    batch = glshBatchCurrent(Context);
    if (batch == gcvNULL)
    {
        /* Out of memory. */
        glmBATCH_LEAVE();
        return GL_OUT_OF_MEMORY;
    }

    /* Initialize the batch command. */
    batch->u.clear.command = glvBATCH_CLEAR;
    batch->u.clear.flags   = Buffers;

    if (batch->u.clear.flags != 0)
    {
        /* Enqueue the batch. */
        result = glshBatchEnqueue(Context);
        glmBATCH_LEAVE();
        return result;
    }

    /* Success. */
    glmBATCH_LEAVE();
    return GL_NO_ERROR;
}

/*
 *  glFlush
 *  glFinish
 */
GLenum glshBatchFlush(IN GLContext Context)
{
    glsBATCH_QUEUE *    batch;
    GLenum              result;

    glmBATCH_ENTER();

    /* Get current batch. */
    batch = glshBatchCurrent(Context);
    if (batch == gcvNULL)
    {
        /* Out of memory. */
        glmBATCH_LEAVE();
        return GL_OUT_OF_MEMORY;
    }

    /* Initialize the batch command. */
    batch->u.command = glvBATCH_FLUSH;

    /* Enqueue the batch. */
    result = glshBatchEnqueue(Context);
    glmBATCH_LEAVE();
    return result;
}

/*
 *  eglSwapBuffers
 */
GLenum glshBatchSwapbuffer(IN GLContext Context,
                           IN EGLDisplay Display,
                           IN EGLSurface Surface,
                           IN gcfEGL_DO_SWAPBUFFER Callback)
{
    glsBATCH_QUEUE *    batch;
    GLenum              result;

    glmBATCH_ENTER();

    /* Get current batch. */
    batch = glshBatchCurrent(Context);
    if (batch == gcvNULL)
    {
        /* Out of memory. */
        glmBATCH_LEAVE();
        return GL_OUT_OF_MEMORY;
    }

    /* Initialize the batch command. */
    batch->u.swapbuffer.command  = glvBATCH_SWAPBUFFER;
    batch->u.swapbuffer.display  = Display;
    batch->u.swapbuffer.surface  = Surface;
    batch->u.swapbuffer.callback = Callback;

    /* Enqueue the batch. */
    result = glshBatchEnqueue(Context);
    glmBATCH_LEAVE();
    return result;
}


static GLboolean glshValidatePrimitiveMode(IN GLenum Mode)
{
    glmBATCH_ENTER();

    switch (Mode)
    {
        case GL_POINTS:
        case GL_LINE_STRIP:
        case GL_LINE_LOOP:
        case GL_LINES:
        case GL_TRIANGLE_STRIP:
        case GL_TRIANGLE_FAN:
        case GL_TRIANGLES:
            glmBATCH_LEAVE();
            return GL_TRUE;
    }

    glmBATCH_LEAVE();
    return GL_FALSE;
}

static GLboolean glshValidateFrontFace(IN GLenum FrontFace)
{
    glmBATCH_ENTER();

    switch (FrontFace)
    {
        case GL_CW:
        case GL_CCW:
            glmBATCH_LEAVE();
            return GL_TRUE;
    }

    glmBATCH_LEAVE();
    return GL_FALSE;
}

static GLboolean glshValidateCullFace(IN GLenum CullFace)
{
    glmBATCH_ENTER();

    switch (CullFace)
    {
        case GL_FRONT:
        case GL_BACK:
        case GL_FRONT_AND_BACK:
            glmBATCH_LEAVE();
            return GL_TRUE;
    }

    glmBATCH_LEAVE();
    return GL_FALSE;
}

static GLboolean glshValidateWrap(IN GLenum Wrap)
{
    glmBATCH_ENTER();

    switch (Wrap)
    {
        case GL_CLAMP_TO_EDGE:
        case GL_REPEAT:
        case GL_MIRRORED_REPEAT:
            glmBATCH_LEAVE();
            return GL_TRUE;
    }

    glmBATCH_LEAVE();
    return GL_FALSE;
}

static GLboolean glshValidateFilter(IN GLenum Filter, IN GLboolean Minification)
{
    glmBATCH_ENTER();

    switch (Filter)
    {
        case GL_NEAREST:
        case GL_LINEAR:
            return GL_TRUE;

        case GL_NEAREST_MIPMAP_NEAREST:
        case GL_NEAREST_MIPMAP_LINEAR:
        case GL_LINEAR_MIPMAP_NEAREST:
        case GL_LINEAR_MIPMAP_LINEAR:
            return Minification;
    }

    return GL_FALSE;
}

static GLboolean glshValidateCompare(IN GLenum Compare)
{
    glmBATCH_ENTER();

    switch (Compare)
    {
        case GL_NEVER:
        case GL_LESS:
        case GL_EQUAL:
        case GL_LEQUAL:
        case GL_GREATER:
        case GL_NOTEQUAL:
        case GL_GEQUAL:
        case GL_ALWAYS:
            glmBATCH_LEAVE();
            return GL_TRUE;
    }

    glmBATCH_LEAVE();
    return GL_FALSE;
}

static GLboolean glshValidateOperation(IN GLenum Operation)
{
    glmBATCH_ENTER();

    switch (Operation)
    {
        case GL_KEEP:
        case GL_ZERO:
        case GL_REPLACE:
        case GL_INCR:
        case GL_DECR:
        case GL_INVERT:
        case GL_INCR_WRAP:
        case GL_DECR_WRAP:
            glmBATCH_LEAVE();
            return GL_TRUE;
    }

    glmBATCH_LEAVE();
    return GL_FALSE;
}

static GLboolean glshValidateBlendMode(IN GLenum BlendMode)
{
    glmBATCH_ENTER();

    switch (BlendMode)
    {
        case GL_FUNC_ADD:
        case GL_FUNC_SUBTRACT:
        case GL_FUNC_REVERSE_SUBTRACT:
        case GL_MIN_EXT:
        case GL_MAX_EXT:
            glmBATCH_LEAVE();
            return GL_TRUE;
    }

    glmBATCH_LEAVE();
    return GL_FALSE;
}

static GLboolean glshValidateBlendFunction(IN GLenum BlendFunction)
{
    glmBATCH_ENTER();

    switch (BlendFunction)
    {
        case GL_ZERO:
        case GL_ONE:
        case GL_SRC_COLOR:
        case GL_ONE_MINUS_SRC_COLOR:
        case GL_DST_COLOR:
        case GL_ONE_MINUS_DST_COLOR:
        case GL_SRC_ALPHA:
        case GL_ONE_MINUS_SRC_ALPHA:
        case GL_DST_ALPHA:
        case GL_ONE_MINUS_DST_ALPHA:
        case GL_CONSTANT_COLOR:
        case GL_ONE_MINUS_CONSTANT_COLOR:
        case GL_CONSTANT_ALPHA:
        case GL_ONE_MINUS_CONSTANT_ALPHA:
        case GL_SRC_ALPHA_SATURATE:
            glmBATCH_LEAVE();
            return GL_TRUE;
    }

    glmBATCH_LEAVE();
    return GL_FALSE;
}


/*
 *  glVertexAttrib{1234}f
 *  glVertexAttrib{1234}fv
 */
GLenum glshBatchVertexAttribute(IN GLContext Context,
                                IN gctUINT Index,
                                IN gctFLOAT X,
                                IN gctFLOAT Y,
                                IN gctFLOAT Z,
                                IN gctFLOAT W)
{
    glsBATCH_STATE_ATTRIBUTE *  attribute;

    glmBATCH_ENTER();

    /* Test for valid index. */
    if (Index >= gldVERTEX_ELEMENT_COUNT)
    {
        /* Index is out of range. */
        glmBATCH_LEAVE();
        return GL_INVALID_VALUE;
    }

    /* Get current batch. */
    glmBATCH_START(Context)
    {
        /* Get pointer to batch vertex attribute. */
        attribute = &batch->attributes[Index];

        /* Vertex attribute has changed. */
        batch->flags.attributes |= 1 << Index;

        /* Update the vertex attribute generic value. */
        attribute->flags.generics = 1;
        attribute->genericsX      = X;
        attribute->genericsY      = Y;
        attribute->genericsZ      = Z;
        attribute->genericsW      = W;
    }
    glmBATCH_END(GL_NO_ERROR)
}

/*
 *  glVertexAttribPointer
 */
GLenum glshBatchVertexAttributePointer(IN GLContext Context,
                                       IN gctUINT Index,
                                       IN gctINT Size,
                                       IN GLenum Type,
                                       IN GLboolean Normalized,
                                       IN GLsizei Stride,
                                       IN const GLvoid * Pointer,
                                       IN GLBuffer Buffer)
{
    gctINT                      minSize;
    glsBATCH_STATE_ATTRIBUTE *  attribute;

    /* Test for valid index. */
    glmBATCH_ENTER();

    if (Index >= gldVERTEX_ELEMENT_COUNT)
    {
        /* Index is out of range. */
        glmBATCH_LEAVE();
        return GL_INVALID_VALUE;
    }

    /* Verify the vertex attribute type. */
    switch (Type)
    {
        case GL_BYTE:
        case GL_UNSIGNED_BYTE:
        case GL_SHORT:
        case GL_UNSIGNED_SHORT:
        case GL_FIXED:
        case GL_FLOAT:
        case GL_INT:
        case GL_UNSIGNED_INT:
        case GL_HALF_FLOAT_OES:
            minSize = 1;
            break;

        case GL_INT_10_10_10_2_OES:
        case GL_UNSIGNED_INT_10_10_10_2_OES:
            minSize = 3;
            break;

        default:
            /* Invalid vertex atribute type. */
            glmBATCH_LEAVE();
            return GL_INVALID_ENUM;
    }

    /* Test vertex size. */
    if ((Size < minSize) || (Size > 4))
    {
        /* Invalid vertex size. */
        glmBATCH_LEAVE();
        return GL_INVALID_VALUE;
    }

    /* Test for valid stride. */
    if (Stride < 0)
    {
        /* Invalid stride. */
        glmBATCH_LEAVE();
        return GL_INVALID_VALUE;
    }

    /* Get current batch. */
    glmBATCH_START(Context)
    {
        /* Vertex attribute has changed. */
        batch->flags.attributes |= 1 << Index;

        /* Get pointer to batch vertex attribute. */
        attribute = &batch->attributes[Index];

        /* Update the vertex attribute. */
        attribute->flags.data = 1;
        attribute->size       = Size;
        attribute->type       = Type;
        attribute->normalized = Normalized;
        attribute->stride     = Stride;
        attribute->pointer    = Pointer;
        attribute->copied     = GL_FALSE;
        attribute->buffer     = Buffer;
    }
    glmBATCH_END(GL_NO_ERROR)
}

/*
 *  glEnableVertexAttribArray
 *  glDisableVertexAttribArray
 */
GLenum glshBatchVertexAttributeSet(IN GLContext Context,
                                   IN gctUINT Index,
                                   IN gctBOOL Enable)
{
    glmBATCH_ENTER();

    /* Test for valid index. */
    if (Index >= gldVERTEX_ELEMENT_COUNT)
    {
        /* Index is out of range. */
        glmBATCH_LEAVE();
        return GL_INVALID_VALUE;
    }

    /* Get current batch. */
    glmBATCH_START(Context)
    {
        /* Vertex attribute has changed. */
        batch->flags.attributes |= 1 << Index;

        /* Update the vertex attribute enable. */
        batch->attributes[Index].flags.enable = 1;
        batch->attributes[Index].enable       = (GLboolean)Enable;
    }
    glmBATCH_END(GL_NO_ERROR)
}

/*
 *  glUseProgram
 */
GLenum glshBatchProgram(IN GLContext Context,
                        IN GLuint Program)
{
    GLProgram   object;

    glmBATCH_ENTER();

    if (Program == 0)
    {
        object = gcvNULL;
    }
    else
    {
        /* Find the program. */
        object = (GLProgram) _glshFindObject(&Context->shaderObjects, Program);

        /* Test if the program is found. */
        if (object == gcvNULL)
        {
            glmBATCH_LEAVE();
            return GL_INVALID_VALUE;
        }

        /* Tets if program is linked correctly. */
        if (!object->linkStatus || (object->statesSize == 0))
        {
            glmBATCH_LEAVE();
            return GL_INVALID_VALUE;
        }
    }

    /* Get current batch. */
    glmBATCH_START(Context)
    {
        /* Update the shader program. */
        batch->flags.program = 1;
        batch->program       = object;

        /* Also save in context for uniform management. */
        Context->batchInfo.program = object;
    }
    glmBATCH_END(GL_NO_ERROR)
}

/*
 *  glUniform{1234}{if}
 *  glUniform{1234}{if}v
 *  glUniformMatrix{234}fv
 */
GLenum glshBatchUniform(IN GLContext Context,
                        IN GLint Location,
                        IN gcSHADER_TYPE Type,
                        IN GLsizei Count,
                        IN void const * Data)
{
    GLProgram                   program;
    GLint                       location;
    GLint                       index;
    GLUniform                   uniform;
    glsBATCH_STATE_UNIFORM *    batch;
    GLsizei                     count;
    GLsizei                     bytes;

    glmBATCH_ENTER();

    /* Do nothing if there is no data. */
    if ((Count == 0) || (Data == gcvNULL))
    {
        glmBATCH_LEAVE();
        return GL_NO_ERROR;
    }

    /* Validate that count is positive. */
    if (Count < 0)
    {
        glmBATCH_LEAVE();
        return GL_INVALID_VALUE;
    }

    /* Get the current program. */
    program = Context->batchInfo.program;
    if (program == gcvNULL)
    {
        /* There must be a valid program. */
        glmBATCH_LEAVE();
        return GL_INVALID_OPERATION;
    }

    /* Decode the location. */
    location = Location & 0xFFFF;
    index    = Location >> 16;

    /* Validate the decoded location. */
    if ((location >= program->uniformCount) || (index < 0))
    {
        glmBATCH_LEAVE();
        return GL_INVALID_OPERATION;
    }

    /* Get the uniform location. */
    uniform = &program->uniforms[location];

    /* Validate if the uniform is an array if count > 1. */
    if ((Count > 1) && (uniform->length == 1))
    {
        glmBATCH_LEAVE();
        return GL_INVALID_OPERATION;
    }

    /* Validate the type matches. */
    if (Type != uniform->type)
    {
        /* Dispatch on the mismatching type. */
        switch (uniform->type)
        {
            case gcSHADER_BOOLEAN_X1:
                /* BOOL uniforms can be called from glUniform1{if} or glUniform1{if}v. */
                if ((Type != gcSHADER_INTEGER_X1) && (Type != gcSHADER_FLOAT_X1))
                {
                    /* Mismatch in type. */
                    glmBATCH_LEAVE();
                    return GL_INVALID_OPERATION;
                }
                break;

            case gcSHADER_BOOLEAN_X2:
                /* BVEC2 uniforms can be called from glUniform2{if} or glUniform2{if}v. */
                if ((Type != gcSHADER_INTEGER_X2) && (Type != gcSHADER_FLOAT_X2))
                {
                    /* Mismatch in type. */
                    glmBATCH_LEAVE();
                    return GL_INVALID_OPERATION;
                }
                break;

            case gcSHADER_BOOLEAN_X3:
                /* BVEC3 uniforms can be called from glUniform3{if} or glUniform3{if}v. */
                if ((Type != gcSHADER_INTEGER_X3) && (Type != gcSHADER_FLOAT_X3))
                {
                    /* Mismatch in type. */
                    glmBATCH_LEAVE();
                    return GL_INVALID_OPERATION;
                }
                break;

            case gcSHADER_BOOLEAN_X4:
                /* BVEC4 uniforms can be called from glUniform4{if} or glUniform4{if}v. */
                if ((Type != gcSHADER_INTEGER_X4) && (Type != gcSHADER_FLOAT_X4))
                {
                    /* Mismatch in type. */
                    glmBATCH_LEAVE();
                    return GL_INVALID_OPERATION;
                }
                break;

            case gcSHADER_SAMPLER_2D:
            case gcSHADER_SAMPLER_CUBIC:
            case gcSHADER_SAMPLER_3D:
                /* SAMPLER uniforms can be called from glUniform1i or glUniform1iv. */
                if (Type != gcSHADER_INTEGER_X1)
                {
                    /* Mismatch in type. */
                    glmBATCH_LEAVE();
                    return GL_INVALID_OPERATION;
                }
                break;

            default:
                /* Mismatch in type. */
                glmBATCH_LEAVE();
                return GL_INVALID_OPERATION;
        }
    }

    count = gcmMAX(Count, (GLsizei)(uniform->length));
    bytes = count * uniform->elementLength;

    /* Allocate a new batch uniform structure plus the uniform storage. */
    batch = glshBatchAllocate(Context, gcmSIZEOF(glsBATCH_STATE_UNIFORM) + bytes);
    if (batch == gcvNULL)
    {
        /* Out of memory. */
        glmBATCH_LEAVE();
        return GL_OUT_OF_MEMORY;
    }

    /* Initialize the batch uniform. */
    batch->next    = gcvNULL;
    batch->uniform = uniform;
    batch->type    = Type;
    batch->count   = count;
    batch->index   = index;
    batch->data    = batch + 1;

    /* Append the uniform batch to the end of the programs tail. */
    if (program->batchHead == gcvNULL)
    {
        program->batchHead = batch;
        program->batchTail = batch;
    }
    else
    {
        program->batchTail->next = batch;
        program->batchTail       = batch;
    }

    /* Copy the uniform data into the batch uniform memory. */
    gcoOS_MemCopy((GLubyte *) batch->data, Data, bytes);

    /* Success. */
    glmBATCH_LEAVE();
    return GL_NO_ERROR;
}

/*
 *  glDepthRangef
 */
GLenum glshBatchDepthRange(IN GLContext Context,
                           IN GLclampf Near,
                           IN GLclampf Far)
{
    glmBATCH_ENTER();

    /* Get current batch. */
    glmBATCH_START(Context)
    {
        /* Clamp depth range between 0 and 1. */
        batch->flags.depthRange     = 1;
        batch->depthNear = gcmMIN(gcmMAX(Near, 0.0f), 1.0f);
        batch->depthFar  = gcmMIN(gcmMAX(Far,  0.0f), 1.0f);
    }
    glmBATCH_END(GL_NO_ERROR)
}

/*
 *  glViewport
 */
GLenum glshBatchViewport(IN GLContext Context,
                         IN GLint X,
                         IN GLint Y,
                         IN GLsizei Width,
                         IN GLsizei Height)
{
    glmBATCH_ENTER();

    /* Width and height cannot be negative. */
    if ((Width < 0) || (Height < 0))
    {
        /* Invalid width or height. */
        glmBATCH_LEAVE();
        return GL_INVALID_VALUE;
    }

    /* Get current batch. */
    glmBATCH_START(Context)
    {
        /* Update viewport. */
        batch->flags.viewport = 1;
        batch->viewportX      = X;
        batch->viewportY      = Y;
        batch->viewportWidth  = Width;
        batch->viewportHeight = Height;
    }
    glmBATCH_END(GL_NO_ERROR)
}

/*
 *  glLineWidth
 */
GLenum glshBatchLineWidth(IN GLContext Context,
                          IN gctFLOAT LineWidth)
{
    glmBATCH_ENTER();

    /* Test line width. */
    if (LineWidth <= 0)
    {
        /* Invalid line width. */
        glmBATCH_LEAVE();
        return GL_INVALID_VALUE;
    }

    /* Get current batch. */
    glmBATCH_START(Context)
    {
        /* Update line width. */
        batch->flags.lineWidth = 1;
        batch->lineWidth       = LineWidth;
    }
    glmBATCH_END(GL_NO_ERROR)
}

/*
 *  glFrontFace
 */
GLenum glshBatchFrontFace(IN GLContext Context,
                          IN GLenum FrontFace)
{
    glmBATCH_ENTER();

    /* Validate the front face. */
    if (!glshValidateFrontFace(FrontFace))
    {
        /* Invalid front face. */
        glmBATCH_LEAVE();
        return GL_INVALID_ENUM;
    }

    /* Get current batch. */
    glmBATCH_START(Context)
    {
        /* Update the front face. */
        batch->flags.frontFace = 1;
        batch->frontFace       = FrontFace;
    }
    glmBATCH_END(GL_NO_ERROR)
}

/*
 *  glCullFace
 */
GLenum glshBatchCullFace(IN GLContext Context,
                         IN GLenum CullFace)
{
    glmBATCH_ENTER();

    /* Validate the cull face. */
    if (!glshValidateCullFace(CullFace))
    {
        /* Invalid front face. */
        glmBATCH_LEAVE();
        return GL_INVALID_ENUM;
    }

    /* Get current batch. */
    glmBATCH_START(Context)
    {
        /* Update the cull face. */
        batch->flags.cullFace = 1;
        batch->cullFace       = CullFace;
    }
    glmBATCH_END(GL_NO_ERROR)
}

/*
 *  glEnable
 *  glDisable
 */
GLenum glshBatchSet(IN GLContext Context,
                    IN GLenum Capability,
                    IN gctBOOL Enable)
{
    glmBATCH_ENTER();

    /* Get current batch. */
    glmBATCH_START(Context)
    {
        /* Dispatch on capability. */
        switch (Capability)
        {
            case GL_CULL_FACE:
                batch->flags.cullEnable = 1;
                batch->cullEnable       = (GLboolean)Enable;
                break;

            case GL_POLYGON_OFFSET_FILL:
                batch->flags.polygonOffsetEnable = 1;
                batch->offsetEnable              = (GLboolean)Enable;
                break;

            case GL_SCISSOR_TEST:
                batch->flags.scissorEnable = 1;
                batch->scissorEnable       = (GLboolean)Enable;
                break;

            case GL_STENCIL_TEST:
                batch->flags.stencilTestEnable = 1;
                batch->stencilTestEnable       = (GLboolean)Enable;
                break;

            case GL_DEPTH_TEST:
                batch->flags.depthTestEnable = 1;
                batch->depthTestEnable       = (GLboolean)Enable;
                break;

            case GL_BLEND:
                batch->flags.blendEnable = 1;
                batch->blendEnable       = (GLboolean)Enable;
                break;

            case GL_DITHER:
                batch->flags.ditherEnable = 1;
                batch->ditherEnable       = (GLboolean)Enable;
                break;

            default:
                /* Unknown capability. */
                glmBATCH_LEAVE();
                return GL_INVALID_ENUM;
        }
    }
    glmBATCH_END(GL_NO_ERROR)
}

/*
 *  glPolygonOffset
 */
GLenum glshBatchOffset(IN GLContext Context,
                       IN GLfloat Factor,
                       IN GLfloat Units)
{
    glmBATCH_ENTER();

    /* Get current batch. */
    glmBATCH_START(Context)
    {
        /* Update polygon offset. */
        batch->flags.polygonOffset = 1;
        batch->offsetFactor        = Factor;
        batch->offsetUnits         = Units;
    }
    glmBATCH_END(GL_NO_ERROR)
}

/*
 *  glActiveTexture
 */
GLenum glshBatchActiveTexture(IN GLContext Context,
                              IN GLenum Texture)
{
    GLint   texture;

     glmBATCH_ENTER();

   /* Validate texture. */
    texture = (GLint) (Texture - GL_TEXTURE0);
    if ((texture < 0) || (texture > (GLint)(Context->vertexSamplers + Context->fragmentSamplers)))
    {
        glmBATCH_LEAVE();
        return GL_INVALID_ENUM;
    }

    /* Save active texture. */
    Context->batchInfo.activeTexture = texture;

    /* Success. */
    glmBATCH_LEAVE();
    return GL_NO_ERROR;
}

/*
 *  glTexParameter[if]
 *  glTexParameter[if]v
 */
GLenum glshBatchTextureParameter(IN GLContext Context,
                                 IN GLenum Target,
                                 IN GLenum Parameter,
                                 IN GLint Value)
{
    glsBATCH_STATE_TEXTURE *    texture;
    GLint                       activeTexture;

    glmBATCH_ENTER();

    /* Get active texture. */
    activeTexture = Context->batchInfo.activeTexture;

    glmBATCH_START(Context)
    {
        /* Verify target. */
        switch (Target)
        {
            case GL_TEXTURE_2D:
                /* Get pointer to current texture unit. */
                texture = batch->textures2D[activeTexture];
                break;

            case GL_TEXTURE_CUBE_MAP:
                /* Get pointer to current texture unit. */
                texture = batch->texturesCubic[activeTexture];
                break;

            case GL_TEXTURE_3D_OES:
                /* Get pointer to current texture unit. */
                texture = batch->textures3D[activeTexture];
                break;

            default:
                /* Invalid target. */
                glmBATCH_LEAVE();
                return GL_INVALID_ENUM;
        }

        /* Dispatch on paramater. */
        switch (Parameter)
        {
            case GL_TEXTURE_WRAP_S:
            {
                /* Validate the wrap. */
                if (!glshValidateWrap(Value))
                {
                    /* Invalid wrap value. */
                    glmBATCH_LEAVE();
                    return GL_INVALID_VALUE;
                }

                /* S wrapping has changed. */
                texture->flags.wrapS = 1;
                texture->wrapS       = Value;
                break;
            }

            case GL_TEXTURE_WRAP_T:
            {
                /* Validate the wrap. */
                if (!glshValidateWrap(Value))
                {
                    /* Invalid wrap value. */
                    glmBATCH_LEAVE();
                    return GL_INVALID_VALUE;
                }

                /* T wrapping has changed. */
                texture->flags.wrapT = 1;
                texture->wrapT       = Value;
                break;
            }

            case GL_TEXTURE_MIN_FILTER:
            {
                /* Validate the minification filter. */
                if (!glshValidateFilter(Value, GL_TRUE))
                {
                    /* Invalid minification filter value. */
                    glmBATCH_LEAVE();
                    return GL_INVALID_VALUE;
                }

                /* Minification filter has changed. */
                texture->flags.minFilter = 1;
                texture->minFilter       = Value;
                break;
            }

            case GL_TEXTURE_MAG_FILTER:
            {
                /* Validate the magnification filter. */
                if (!glshValidateFilter(Value, GL_FALSE))
                {
                    /* Invalid magnification filter value. */
                    glmBATCH_LEAVE();
                    return GL_INVALID_VALUE;
                }

                /* Magnification filter has changed. */
                texture->flags.magFilter = 1;
                texture->magFilter       = Value;
                break;
            }

            case GL_TEXTURE_WRAP_R_OES:
            {
                /* Validate the wrap. */
                if (!glshValidateWrap(Value))
                {
                    /* Invalid wrap value. */
                    glmBATCH_LEAVE();
                    return GL_INVALID_VALUE;
                }

                /* R wrapping has changed. */
                texture->flags.wrapR = 1;
                texture->wrapR       = Value;
                break;
            }

            case GL_TEXTURE_MAX_ANISOTROPY_EXT:
                /* Validate the anisotropy. */
                if (Value < 1)
                {
                    /* Invalid anisotropy value. */
                    glmBATCH_LEAVE();
                    return GL_INVALID_VALUE;
                }

                /* Update anisotropy. */
                texture->flags.anisotropy = 1;
                texture->anisotropy       = Value;
                break;

            case GL_TEXTURE_MAX_LEVEL_APPLE:
                /* Validate the maximum level. */
                if (Value < 0)
                {
                    /* Invalid maximum level. */
                    glmBATCH_LEAVE();
                    return GL_INVALID_VALUE;
                }

                /* Update maximum level. */
                texture->flags.maxLevel = 1;
                texture->maxLevel       = Value;
                break;

            default:
                /* Invalid parameter. */
                glmBATCH_LEAVE();
                return GL_INVALID_ENUM;
        }

        switch (Target)
        {
            case GL_TEXTURE_2D:
                /* Texture has changed. */
                batch->flags.textures2D |= 1 << activeTexture;
                break;

            case GL_TEXTURE_CUBE_MAP:
                /* Texture has changed. */
                batch->flags.texturesCubic |= 1 << activeTexture;
                break;

            case GL_TEXTURE_3D_OES:
                /* Texture has changed. */
                batch->flags.textures3D |= 1 << activeTexture;
                break;
        }

    }
    glmBATCH_END(GL_NO_ERROR)
}

/*
 *  glBindTexture
 */
GLenum glshBatchTextureBinding(IN GLContext Context,
                               IN GLenum Target,
                               IN GLuint Texture)
{
    GLint       activeTexture;
    GLTexture   texture;

    glmBATCH_ENTER();

    /* Get active texture. */
    activeTexture = Context->batchInfo.activeTexture;

    /* Test for default texture. */
    if (Texture == 0)
    {
        texture = gcvNULL;
    }
    else
    {
        /* Find the requested texture object. */
        texture = (GLTexture) _glshFindObject(&Context->textureObjects, Texture);
        if (texture == gcvNULL)
        {
            /* Create a new texture object. */
            texture = glshNewTexture(Context, Texture);
            if (texture == gcvNULL)
            {
                /* Out of memory. */
                glmBATCH_LEAVE();
                return GL_OUT_OF_MEMORY;
            }
        }
    }

    /* Get current batch. */
    glmBATCH_START(Context)
    {
        /* Verify target. */
        switch (Target)
        {
            case GL_TEXTURE_2D:
                /* Test for default texture. */
                if (texture == gcvNULL)
                {
                    /* Point to default texture. */
                    texture = &Context->default2D;
                }

                /* Update the 2D texture binding. */
                batch->flags.textures2D         |= 1 << activeTexture;
                batch->textures2D[activeTexture] = &texture->batchedState;
                break;

            case GL_TEXTURE_CUBE_MAP:
                /* Test for default texture. */
                if (texture == gcvNULL)
                {
                    /* Point to default texture. */
                    texture = &Context->defaultCube;
                }

                /* Update the cubic texture binding. */
                batch->flags.texturesCubic         |= 1 << activeTexture;
                batch->texturesCubic[activeTexture] = &texture->batchedState;
                break;

            case GL_TEXTURE_3D_OES:
                /* Test for default texture. */
                if (texture == gcvNULL)
                {
                    /* Point to default texture. */
                    texture = &Context->default3D;
                }

                /* Update the 3D texture binding. */
                batch->flags.textures3D         |= 1 << activeTexture;
                batch->textures3D[activeTexture] = &texture->batchedState;
                break;

            default:
                /* Invalid target. */
                glmBATCH_LEAVE();
                return GL_INVALID_ENUM;
        }
    }
    glmBATCH_END(GL_NO_ERROR)
}

/*
 *  glScissor
 */
GLenum glshBatchScissor(IN GLContext Context,
                        IN GLint X,
                        IN GLint Y,
                        IN GLsizei Width,
                        IN GLsizei Height)
{
    glmBATCH_ENTER();

    /* Width and height cannot be negative. */
    if ((Width < 0) || (Height < 0))
    {
        /* Invalid width or height. */
        glmBATCH_LEAVE();
        return GL_INVALID_VALUE;
    }

    /* Get current batch. */
    glmBATCH_START(Context)
    {
        /* Update scissor box. */
        batch->flags.scissor = 1;
        batch->scissorX      = X;
        batch->scissorY      = Y;
        batch->scissorWidth  = Width;
        batch->scissorHeight = Height;
    }
    glmBATCH_END(GL_NO_ERROR)
}

/*
 *  glStencilFunc
 *  glStencilFuncSeparate
 */
GLenum glshBatchStencilFunction(IN GLContext Context,
                                IN GLenum Face,
                                IN GLenum Compare,
                                IN GLint Reference,
                                IN GLuint Mask)
{
    glmBATCH_ENTER();

    /* Verify the stencil compare function. */
    if (!glshValidateCompare(Compare))
    {
        /* Invalid stencil compare function. */
        glmBATCH_LEAVE();
        return GL_INVALID_ENUM;
    }

    /* Get current batch. */
    glmBATCH_START(Context)
    {
        /* Dispatch on face. */
        switch (Face)
        {
            case GL_FRONT:
                batch->flags.stencilFuncFront = 1;
                batch->stencilFuncFront       = Compare;
                batch->stencilRefFront        = Reference;
                batch->stencilRefMaskFront    = Mask;
                break;

            case GL_BACK:
                batch->flags.stencilFuncBack = 1;
                batch->stencilFuncBack       = Compare;
                batch->stencilRefBack        = Reference;
                batch->stencilRefMaskBack    = Mask;
                break;

            case GL_FRONT_AND_BACK:
                batch->flags.stencilFuncFront = 1;
                batch->stencilFuncFront       = Compare;
                batch->stencilRefFront        = Reference;
                batch->stencilRefMaskFront    = Mask;

                batch->flags.stencilFuncBack = 1;
                batch->stencilFuncBack       = Compare;
                batch->stencilRefBack        = Reference;
                batch->stencilRefMaskBack    = Mask;
                break;

            default:
                /* Invalid face. */
                glmBATCH_LEAVE();
                return GL_INVALID_ENUM;
        }
    }
    glmBATCH_END(GL_NO_ERROR)
}

/*
 *  glStencilOp
 *  glStencilOpSeparate
 */
GLenum glshBatchStencilOperation(IN GLContext Context,
                                 IN GLenum Face,
                                 IN GLenum Fail,
                                 IN GLenum DepthFail,
                                 IN GLenum DepthPass)
{
    glmBATCH_ENTER();

    /* Verify the operations. */
    if (!glshValidateOperation(Fail)      ||
        !glshValidateOperation(DepthFail) ||
        !glshValidateOperation(DepthPass) )
    {
        /* Invalid operation. */
        glmBATCH_LEAVE();
        return GL_INVALID_ENUM;
    }

    /* Get current batch. */
    glmBATCH_START(Context)
    {
        /* Dispatch on face. */
        switch (Face)
        {
            case GL_FRONT:
                batch->flags.stencilOpFront  = 1;
                batch->stencilFailFront      = Fail;
                batch->stencilDepthFailFront = DepthFail;
                batch->stencilDepthPassFront = DepthPass;
                break;

            case GL_BACK:
                batch->flags.stencilOpBack  = 1;
                batch->stencilFailBack      = Fail;
                batch->stencilDepthFailBack = DepthFail;
                batch->stencilDepthPassBack = DepthPass;
                break;

            case GL_FRONT_AND_BACK:
                batch->flags.stencilOpFront  = 1;
                batch->stencilFailFront      = Fail;
                batch->stencilDepthFailFront = DepthFail;
                batch->stencilDepthPassFront = DepthPass;

                batch->flags.stencilOpBack  = 1;
                batch->stencilFailBack      = Fail;
                batch->stencilDepthFailBack = DepthFail;
                batch->stencilDepthPassBack = DepthPass;
                break;

            default:
                /* Invalid face. */
                glmBATCH_LEAVE();
                return GL_INVALID_ENUM;
        }
    }
    glmBATCH_END(GL_NO_ERROR)
}

/*
 *  glDepthFunc
 */
GLenum glshBatchDepthCompare(IN GLContext Context,
                             IN GLenum Compare)
{
    glmBATCH_ENTER();

    /* Verify the depth compare function. */
    if (!glshValidateCompare(Compare))
    {
        /* Invalid depth compare function. */
        glmBATCH_LEAVE();
        return GL_INVALID_ENUM;
    }

    /* Get current batch. */
    glmBATCH_START(Context)
    {
        /* Update depth compare. */
        batch->flags.depthFunc = 1;
        batch->depthFunc       = Compare;
    }
    glmBATCH_END(GL_NO_ERROR)
}

/*
 *  glBlendEquation
 *  glBlendEquationSeparate
 */
GLenum glshBatchBlendMode(IN GLContext Context,
                          IN GLenum ModeColor,
                          IN GLenum ModeAlpha)
{
    glmBATCH_ENTER();

    /* Validate the blending modes. */
    if (!glshValidateBlendMode(ModeColor) || !glshValidateBlendMode(ModeAlpha))
    {
        /* Invalid blending mode. */
        glmBATCH_LEAVE();
        return GL_INVALID_ENUM;
    }

    /* Get current batch. */
    glmBATCH_START(Context)
    {
        /* Set batch parameters. */
        batch->flags.blendMode = 1;
        batch->blendModeRGB    = ModeColor;
        batch->blendModeAlpha  = ModeAlpha;
    }
    glmBATCH_END(GL_NO_ERROR)
}

/*
 *  glBlendFunc
 *  glBlendFuncSeparate
 */

GLenum glshBatchBlendFunction(IN GLContext Context,
                              IN GLenum SrcFunctionColor,
                              IN GLenum SrcFunctionAlpha,
                              IN GLenum DstFunctionColor,
                              IN GLenum DstFunctionAlpha)
{
    glmBATCH_ENTER();

    /* Validate the blend functions. */
    if (!glshValidateBlendFunction(SrcFunctionColor) ||
        !glshValidateBlendFunction(SrcFunctionAlpha) ||
        !glshValidateBlendFunction(DstFunctionColor) ||
        !glshValidateBlendFunction(DstFunctionAlpha) )
    {
        /* Invalid blending function. */
        glmBATCH_LEAVE();
        return GL_INVALID_ENUM;
    }

    /* Get current batch. */
    glmBATCH_START(Context)
    {
        /* Set batch parameters. */
        batch->flags.blendFunc           = 1;
        batch->blendFuncSourceRGB        = SrcFunctionColor;
        batch->blendFuncSourceAlpha      = SrcFunctionAlpha;
        batch->blendFuncDestinationRGB   = DstFunctionColor;
        batch->blendFuncDestinationAlpha = DstFunctionAlpha;
    }
    glmBATCH_END(GL_NO_ERROR)
}

/*
 *  glBlendColor
 */
GLenum glshBatchBlendColor(IN GLContext Context,
                           IN GLclampf Red,
                           IN GLclampf Green,
                           IN GLclampf Blue,
                           IN GLclampf Alpha)
{
    glmBATCH_ENTER();

    /* Get current batch. */
    glmBATCH_START(Context)
    {
        /* Clamp colors between 0 and 1. */
        batch->flags.blendColor = 1;
        batch->blendColorRed    = gcmMIN(gcmMAX(Red,   0.0f), 1.0f);
        batch->blendColorGreen  = gcmMIN(gcmMAX(Green, 0.0f), 1.0f);
        batch->blendColorBlue   = gcmMIN(gcmMAX(Blue,  0.0f), 1.0f);
        batch->blendColorAlpha  = gcmMIN(gcmMAX(Alpha, 0.0f), 1.0f);
    }
    glmBATCH_END(GL_NO_ERROR)
}

/*
 *  glColorMask
 */
GLenum glshBatchColorMask(IN GLContext Context,
                          IN GLboolean Red,
                          IN GLboolean Green,
                          IN GLboolean Blue,
                          IN GLboolean Alpha)
{
    glmBATCH_ENTER();

    /* Get current batch. */
    glmBATCH_START(Context)
    {
        /* Update color mask. */
        batch->flags.colorMask = 1;
        batch->colorMaskRed    = Red;
        batch->colorMaskGreen  = Green;
        batch->colorMaskBlue   = Blue;
        batch->colorMaskAlpha  = Alpha;
    }
    glmBATCH_END(GL_NO_ERROR)
}

/*
 *  glDepthMask
 */
GLenum glshBatchDepthMask(IN GLContext Context,
                          IN GLboolean Enable)
{
    glmBATCH_ENTER();

    /* Get current batch. */
    glmBATCH_START(Context)
    {
        /* Set depth mask. */
        batch->flags.depthMask = 1;
        batch->depthMask       = Enable;
    }
    glmBATCH_END(GL_NO_ERROR)
}

/*
 *  glStencilMask
 *  glStencilMaskSeparate
 */
GLenum glshBatchStencilMask(IN GLContext Context,
                            IN GLenum Face,
                            IN GLuint Mask)
{
    glmBATCH_ENTER();

    /* Get current batch. */
    glmBATCH_START(Context)
    {
        /* Dispatch on face. */
        switch (Face)
        {
            case GL_FRONT:
                batch->flags.stencilMaskFront = 1;
                batch->stencilMaskFront       = Mask;
                break;

            case GL_BACK:
                batch->flags.stencilMaskBack = 1;
                batch->stencilMaskBack       = Mask;
                break;

            case GL_FRONT_AND_BACK:
                batch->flags.stencilMaskFront = 1;
                batch->stencilMaskFront       = Mask;

                batch->flags.stencilMaskBack = 1;
                batch->stencilMaskBack       = Mask;
                break;

            default:
                /* Invalid face. */
                glmBATCH_LEAVE();
                return GL_INVALID_ENUM;
        }
    }
    glmBATCH_END(GL_NO_ERROR)
}

/*
 *  glClearColor
 */
GLenum glshBatchClearColor(IN GLContext Context,
                           IN GLclampf Red,
                           IN GLclampf Green,
                           IN GLclampf Blue,
                           IN GLclampf Alpha)
{
    glmBATCH_ENTER();

    /* Get current batch. */
    glmBATCH_START(Context)
    {
        /* Clamp clear color between 0 and 1. */
        batch->flags.clearColor = 1;
        batch->clearColorRed    = gcmMIN(gcmMAX(Red,   0.0f), 1.0f);
        batch->clearColorGreen  = gcmMIN(gcmMAX(Green, 0.0f), 1.0f);
        batch->clearColorBlue   = gcmMIN(gcmMAX(Blue,  0.0f), 1.0f);
        batch->clearColorAlpha  = gcmMIN(gcmMAX(Alpha, 0.0f), 1.0f);
    }
    glmBATCH_END(GL_NO_ERROR)
}

/*
 *  glClearDepthf
 */
GLenum glshBatchClearDepth(IN GLContext Context,
                           IN GLclampf Depth)
{
    glmBATCH_ENTER();

    /* Get current batch. */
    glmBATCH_START(Context)
    {
        /* Clamp clear depth between 0 and 1. */
        batch->flags.clearDepth = 1;
        batch->clearDepth       = gcmMIN(gcmMAX(Depth, 0.0f), 1.0f);
    }
    glmBATCH_END(GL_NO_ERROR)
}

/*
 *  glClearStencil
 */
GLenum glshBatchClearStencil(IN GLContext Context,
                             IN GLint Stencil)
{
    glmBATCH_ENTER();

    /* Get current batch. */
    glmBATCH_START(Context)
    {
        /* Update stencil clear value. */
        batch->flags.clearStencil = 1;
        batch->clearStencil       = Stencil;
    }
    glmBATCH_END(GL_NO_ERROR)
}

/*
 *  glBindFramebuffer
 */
GLenum glshBatchFramebuffer(IN GLContext Context,
                            IN GLenum Target,
                            IN GLuint Framebuffer)
{
    glmBATCH_ENTER();

    /* Validate the target. */
    if (Target != GL_FRAMEBUFFER)
    {
        /* Invalid target. */
        glmBATCH_LEAVE();
        return GL_INVALID_ENUM;
    }

    /* Get current batch. */
    glmBATCH_START(Context)
    {
        /* Update framebuffer. */
        batch->flags.framebuffer = 1;
        batch->framebuffer       = Framebuffer;
    }
    glmBATCH_END(GL_NO_ERROR);
}

#endif
