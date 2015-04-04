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


#ifndef _gc_glsh_batch
#define _gc_glsh_batch

#include <EGL/egl.h>
#include "gc_egl_common.h"


#define gldBATCH_HEAP_SIZE  (1 << 20)

#define glmBATCH_NOP        for (;;) break

#define glmBATCH(_context, _onerror, _done, _func) \
    if (_context->patchInfo.patchFlags.batching) \
    { \
        GLenum _result = _func; \
        if (_result != GL_NO_ERROR) \
        { \
            switch (_result) \
            { \
                case GL_INVALID_ENUM: \
                    gcmTRACE(gcvLEVEL_ERROR, "%s(%d) => GL_INVALID_ENUM", __FUNCTION__, __LINE__); \
                    break; \
                case GL_INVALID_VALUE: \
                    gcmTRACE(gcvLEVEL_ERROR, "%s(%d) => GL_INVALID_VALUE", __FUNCTION__, __LINE__); \
                    break; \
                case GL_INVALID_OPERATION: \
                    gcmTRACE(gcvLEVEL_ERROR, "%s(%d) => GL_INVALID_OPERATION", __FUNCTION__, __LINE__); \
                    break; \
                case GL_OUT_OF_MEMORY: \
                    gcmTRACE(gcvLEVEL_ERROR, "%s(%d) => GL_OUT_OF_MEMORY", __FUNCTION__, __LINE__); \
                    break; \
                default: \
                    gcmTRACE(gcvLEVEL_ERROR, "%s(%d) => 0x%04x", __FUNCTION__, __LINE__, _result); \
            } \
            _context->error = _result; \
            _onerror; \
        } \
        _done; \
    } \
    glmBATCH_NOP


GLenum glshBatchStart(IN GLContext Context);

GLenum glshBatchStop(IN GLContext Context);

GLenum glshBatchSync(IN GLContext Context, gctBOOL Stall);


GLvoid * glshBatchAllocate(IN GLContext Context,
                           IN GLsizei Bytes);

GLvoid glshBatchFree(IN GLContext Context,
                     IN GLvoid * Memory);

GLvoid glshBatchCompact(IN GLContext Context);


GLenum glshBatchDrawArrays(IN GLContext Context,
                           IN GLenum Mode,
                           IN GLint First,
                           IN GLsizei Count);

GLenum glshBatchDrawElements(IN GLContext Context,
                             IN GLenum Mode,
                             IN GLsizei Count,
                             IN GLenum Type,
                             IN GLvoid const * Indices);

GLenum glshBatchClear(IN GLContext Context,
                      IN GLbitfield Buffers);

GLenum glshBatchFlush(IN GLContext Context);

GLenum glshBatchSwapbuffer(IN GLContext Context,
                           IN EGLDisplay Display,
                           IN EGLSurface Surface,
                           IN gcfEGL_DO_SWAPBUFFER Callback);


typedef struct glsBATCH_STATE_ATTRIBUTE_FLAGS
{
    GLbitfield  generics    : 1;
    GLbitfield  data        : 1;
    GLbitfield  enable      : 1;
}
glsBATCH_STATE_ATTRIBUTE_FLAGS;

typedef struct glsBATCH_STATE_ATTRIBUTE
{
    glsBATCH_STATE_ATTRIBUTE_FLAGS  flags;

    GLfloat                         genericsX;
    GLfloat                         genericsY;
    GLfloat                         genericsZ;
    GLfloat                         genericsW;

    GLint                           size;

    GLenum                          type;

    GLboolean                       normalized;

    GLsizei                         stride;

    const GLvoid *                  pointer;
    GLboolean                       copied;

    GLBuffer                        buffer;

    GLboolean                       enable;
}
glsBATCH_STATE_ATTRIBUTE;

typedef struct glsBATCH_STATE_TEXTURE_FLAGS
{
    GLbitfield  wrapS           : 1;
    GLbitfield  wrapT           : 1;
    GLbitfield  wrapR           : 1;
    GLbitfield  minFilter       : 1;
    GLbitfield  magFilter       : 1;
    GLbitfield  anisotropy      : 1;
    GLbitfield  maxLevel        : 1;
}
glsBATCH_STATE_TEXTURE_FLAGS;

typedef struct glsBATCH_STATE_TEXTURE
{
    glsBATCH_STATE_TEXTURE_FLAGS    flags;

    GLuint                          texture;

    GLenum                          wrapS;
    GLenum                          wrapT;
    GLenum                          wrapR;

    GLenum                          minFilter;
    GLenum                          magFilter;

    GLuint                          anisotropy;

    GLuint                          maxLevel;
}
glsBATCH_STATE_TEXTURE;

typedef struct glsBATCH_STATE_UNIFORM
{
    /* Pointer to next batched uniform in linked list. */
    struct glsBATCH_STATE_UNIFORM * next;

    /* Pointer to the GLUniform owning this batch data. */
    GLUniform                       uniform;

    /* Type of data. */
    gcSHADER_TYPE                   type;

    /* Number of uniforms. */
    GLsizei                         count;

    /* Uniform starting index. */
    GLint                           index;

    /* Pointer to the uniform data. */
    GLvoid *                        data;
}
glsBATCH_STATE_UNIFORM;

typedef struct glsBATCH_STATE_FLAGS
{
    GLbitfield  attributes          : gldVERTEX_ELEMENT_COUNT;
    GLbitfield  program             : 1;
    GLbitfield  uniform             : 1;
    GLbitfield  depthRange          : 1;
    GLbitfield  viewport            : 1;
    GLbitfield  lineWidth           : 1;
    GLbitfield  frontFace           : 1;
    GLbitfield  cullFace            : 1;
    GLbitfield  cullEnable          : 1;
    GLbitfield  polygonOffset       : 1;
    GLbitfield  polygonOffsetEnable : 1;
    GLbitfield  textures2D          : gldTEXTURE_SAMPLER_COUNT;
    GLbitfield  texturesCubic       : gldTEXTURE_SAMPLER_COUNT;
    GLbitfield  textures3D          : gldTEXTURE_SAMPLER_COUNT;
    GLbitfield  scissor             : 1;
    GLbitfield  scissorEnable       : 1;
    GLbitfield  stencilTestEnable   : 1;
    GLbitfield  stencilFuncFront    : 1;
    GLbitfield  stencilFuncBack     : 1;
    GLbitfield  stencilOpFront      : 1;
    GLbitfield  stencilOpBack       : 1;
    GLbitfield  depthTestEnable     : 1;
    GLbitfield  depthFunc           : 1;
    GLbitfield  blendEnable         : 1;
    GLbitfield  blendMode           : 1;
    GLbitfield  blendFunc           : 1;
    GLbitfield  blendColor          : 1;
    GLbitfield  ditherEnable        : 1;
    GLbitfield  colorMask           : 1;
    GLbitfield  depthMask           : 1;
    GLbitfield  stencilMaskFront    : 1;
    GLbitfield  stencilMaskBack     : 1;
    GLbitfield  clearColor          : 1;
    GLbitfield  clearDepth          : 1;
    GLbitfield  clearStencil        : 1;
    GLbitfield  framebuffer         : 1;
}
glsBATCH_STATE_FLAGS;

typedef struct glsBATCH_STATE
{
    glsBATCH_STATE_FLAGS        flags;

    glsBATCH_STATE_ATTRIBUTE    attributes[gldVERTEX_ELEMENT_COUNT];

    GLProgram                   program;
    glsBATCH_STATE_UNIFORM *    uniform;

    GLfloat                     depthNear;
    GLfloat                     depthFar;

    GLint                       viewportX;
    GLint                       viewportY;
    GLsizei                     viewportWidth;
    GLsizei                     viewportHeight;

    GLfloat                     lineWidth;

    GLenum                      frontFace;
    GLenum                      cullFace;
    GLboolean                   cullEnable;

    GLfloat                     offsetFactor;
    GLfloat                     offsetUnits;
    GLboolean                   offsetEnable;

    glsBATCH_STATE_TEXTURE *    textures2D[gldTEXTURE_SAMPLER_COUNT];
    glsBATCH_STATE_TEXTURE *    texturesCubic[gldTEXTURE_SAMPLER_COUNT];
    glsBATCH_STATE_TEXTURE *    textures3D[gldTEXTURE_SAMPLER_COUNT];

    GLint                       scissorX;
    GLint                       scissorY;
    GLsizei                     scissorWidth;
    GLsizei                     scissorHeight;
    GLboolean                   scissorEnable;

    GLboolean                   stencilTestEnable;
    GLenum                      stencilFuncFront;
    GLint                       stencilRefFront;
    GLuint                      stencilRefMaskFront;
    GLenum                      stencilFuncBack;
    GLint                       stencilRefBack;
    GLuint                      stencilRefMaskBack;
    GLenum                      stencilFailFront;
    GLenum                      stencilDepthFailFront;
    GLenum                      stencilDepthPassFront;
    GLenum                      stencilFailBack;
    GLenum                      stencilDepthFailBack;
    GLenum                      stencilDepthPassBack;

    GLboolean                   depthTestEnable;
    GLenum                      depthFunc;

    GLboolean                   blendEnable;
    GLenum                      blendModeRGB;
    GLenum                      blendModeAlpha;
    GLenum                      blendFuncSourceRGB;
    GLenum                      blendFuncSourceAlpha;
    GLenum                      blendFuncDestinationRGB;
    GLenum                      blendFuncDestinationAlpha;
    GLfloat                     blendColorRed;
    GLfloat                     blendColorGreen;
    GLfloat                     blendColorBlue;
    GLfloat                     blendColorAlpha;

    GLboolean                   ditherEnable;

    GLboolean                   colorMaskRed;
    GLboolean                   colorMaskGreen;
    GLboolean                   colorMaskBlue;
    GLboolean                   colorMaskAlpha;

    GLboolean                   depthMask;

    GLuint                      stencilMaskFront;
    GLuint                      stencilMaskBack;

    GLfloat                     clearColorRed;
    GLfloat                     clearColorGreen;
    GLfloat                     clearColorBlue;
    GLfloat                     clearColorAlpha;

    GLfloat                     clearDepth;

    GLint                       clearStencil;

    GLuint                      framebuffer;
}
glsBATCH_STATE;


GLenum glshBatchVertexAttribute(IN GLContext Context,
                                IN gctUINT Index,
                                IN gctFLOAT X,
                                IN gctFLOAT Y,
                                IN gctFLOAT Z,
                                IN gctFLOAT W);

GLenum glshBatchVertexAttributePointer(IN GLContext Context,
                                       IN gctUINT Index,
                                       IN gctINT Size,
                                       IN GLenum Type,
                                       IN GLboolean Normalized,
                                       IN GLsizei Stride,
                                       IN const GLvoid * Pointer,
                                       IN GLBuffer Buffer);

GLenum glshBatchVertexAttributeSet(IN GLContext Context,
                                   IN gctUINT Index,
                                   IN gctBOOL Enable);

GLenum glshBatchProgram(IN GLContext Context,
                        IN GLuint Program);

GLenum glshBatchUniform(IN GLContext Context,
                        IN GLint Location,
                        IN gcSHADER_TYPE Type,
                        IN GLsizei Count,
                        IN void const * Data);

GLenum glshBatchDepthRange(IN GLContext Context,
                           IN GLclampf Near,
                           IN GLclampf Far);

GLenum glshBatchViewport(IN GLContext Context,
                         IN GLint X,
                         IN GLint Y,
                         IN GLsizei Width,
                         IN GLsizei Height);

GLenum glshBatchLineWidth(IN GLContext Context,
                          IN gctFLOAT LineWidth);

GLenum glshBatchFrontFace(IN GLContext Context,
                          IN GLenum FrontFace);

GLenum glshBatchCullFace(IN GLContext Context,
                         IN GLenum CullFace);

GLenum glshBatchSet(IN GLContext Context,
                    IN GLenum Capability,
                    IN gctBOOL Enable);

GLenum glshBatchOffset(IN GLContext Context,
                       IN GLfloat Factor,
                       IN GLfloat Units);

GLenum glshBatchActiveTexture(IN GLContext Context,
                              IN GLenum Texture);

GLenum glshBatchTextureParameter(IN GLContext Context,
                                 IN GLenum Target,
                                 IN GLenum Parameter,
                                 IN GLint Value);

GLenum glshBatchTextureBinding(IN GLContext Context,
                               IN GLenum Target,
                               IN GLuint Texture);

GLenum glshBatchScissor(IN GLContext Context,
                        IN GLint X,
                        IN GLint Y,
                        IN GLsizei Width,
                        IN GLsizei Height);

GLenum glshBatchStencilFunction(IN GLContext Context,
                                IN GLenum Face,
                                IN GLenum Compare,
                                IN GLint Reference,
                                IN GLuint Mask);

GLenum glshBatchStencilOperation(IN GLContext Context,
                                 IN GLenum Face,
                                 IN GLenum Fail,
                                 IN GLenum DepthFail,
                                 IN GLenum DepthPass);

GLenum glshBatchDepthCompare(IN GLContext Context,
                             IN GLenum Compare);

GLenum glshBatchBlendMode(IN GLContext Context,
                          IN GLenum ModeColor,
                          IN GLenum ModeAlpha);

GLenum glshBatchBlendFunction(IN GLContext Context,
                              IN GLenum SrcFunctionColor,
                              IN GLenum SrcFunctionAlpha,
                              IN GLenum DstFunctionColor,
                              IN GLenum DstFunctionAlpha);

GLenum glshBatchBlendColor(IN GLContext Context,
                           IN GLclampf Red,
                           IN GLclampf Green,
                           IN GLclampf Blue,
                           IN GLclampf Alpha);

GLenum glshBatchColorMask(IN GLContext Context,
                          IN GLboolean Red,
                          IN GLboolean Green,
                          IN GLboolean Blue,
                          IN GLboolean Alpha);

GLenum glshBatchDepthMask(IN GLContext Context,
                          IN GLboolean Enable);

GLenum glshBatchStencilMask(IN GLContext Context,
                            IN GLenum Face,
                            IN GLuint Mask);

GLenum glshBatchClearColor(IN GLContext Context,
                           IN GLclampf Red,
                           IN GLclampf Green,
                           IN GLclampf Blue,
                           IN GLclampf Alpha);

GLenum glshBatchClearDepth(IN GLContext Context,
                           IN GLclampf Depth);

GLenum glshBatchClearStencil(IN GLContext Context,
                             IN GLint Stencil);

GLenum glshBatchFramebuffer(IN GLContext Context,
                            IN GLenum Target,
                            IN GLuint Framebuffer);


typedef enum gleBATCH_COMMAND
{
    glvBATCH_STOP,
    glvBATCH_SYNC,
    glvBATCH_DRAW_ARRAYS,
    glvBATCH_DRAW_ELEMENTS,
    glvBATCH_CLEAR,
    glvBATCH_FLUSH,
    glvBATCH_SWAPBUFFER,
}
gleBATCH_COMMAND;

typedef struct glsBATCH_SYNC
{
    /* Batch command. */
    gleBATCH_COMMAND    command;

    /* Stall request. */
    gctBOOL             stall;
}
glsBATCH_SYNC;

typedef struct glsBATCH_DRAW_ARRAYS
{
    /* Batch command. */
    gleBATCH_COMMAND    command;

    /* Primitive mode. */
    GLenum              mode;

    /* First vertex. */
    GLint               first;

    /* Vertex count. */
    GLsizei             count;
}
glsBATCH_DRAW_ARRAYS;

typedef struct glsBATCH_DRAW_ELEMENTS
{
    /* Batch command. */
    gleBATCH_COMMAND    command;

    /* Primitive mode. */
    GLenum              mode;

    /* Element count. */
    GLsizei             count;

    /* Element type. */
    GLenum              type;

    /* Element indices. */
    GLvoid const *      indices;

    /* Bound element array buffer. */
    GLBuffer            elementArrayBuffer;
}
glsBATCH_DRAW_ELEMENTS;

typedef struct glsBATCH_CLEAR
{
    /* Batch command. */
    gleBATCH_COMMAND    command;

    /* Clear flags. */
    GLbitfield          flags;
}
glsBATCH_CLEAR;

typedef struct glsBATCH_SWAPBUFFER
{
    /* Batch command. */
    gleBATCH_COMMAND        command;

    /* Display. */
    EGLDisplay              display;

    /* Surface. */
    EGLSurface              surface;

    /* Callback function. */
    gcfEGL_DO_SWAPBUFFER    callback;
}
glsBATCH_SWAPBUFFER;

typedef union gluBATCH_COMMAND
{
    /* Just a command. */
    gleBATCH_COMMAND        command;

    /* Sycnhronization. */
    glsBATCH_SYNC           sync;

    /* Draw arrays. */
    glsBATCH_DRAW_ARRAYS    drawArrays;

    /* Draw elements. */
    glsBATCH_DRAW_ELEMENTS  drawElements;

    /* Clear. */
    glsBATCH_CLEAR          clear;

    /* Swapbuffer. */
    glsBATCH_SWAPBUFFER     swapbuffer;
}
gluBATCH_COMMAND;

typedef struct glsBATCH_QUEUE
{
    /* Command. */
    gluBATCH_COMMAND    u;

    /* Batch structure. */
    glsBATCH_STATE      state;
}
glsBATCH_QUEUE;

typedef struct glsBATCH_CLIENT
{
    /* Index into queue. */
    volatile gctUINT    counter;

    /* Handshake signal. */
    gctSIGNAL           signal;
}
glsBATCH_CLIENT;

typedef struct glsBATCH_HEAP_NODE glsBATCH_HEAP_NODE;
typedef struct glsBATCH_HEAP glsBATCH_HEAP;

struct glsBATCH_HEAP_NODE
{
    /* Number of bytes for this block. Positive is free, negative is allocated. */
    GLsizei         bytes;
};

struct glsBATCH_HEAP
{
    /* Pointer to next heap. */
    struct glsBATCH_HEAP *  next;

    /* Pointer to the heap memory pool. */
    glsBATCH_HEAP_NODE *    memory;

    /* Pointer to the next free node. */
    glsBATCH_HEAP_NODE *    free;
};

typedef struct glsBATCH_INFO
{
    /* Source TLS pointer. */
    gcsTLS_PTR          tls;

    /* Batch thread. */
    gctHANDLE           thread;

    /* Producer information. */
    glsBATCH_CLIENT     producer;

    /* Consumer information. */
    glsBATCH_CLIENT     consumer;

    /* Synchronization signal. */
    gctSIGNAL           syncSignal;

    /* Batch queue. */
    glsBATCH_QUEUE      queue[128];

    /* Current entry in the batch queue. */
    glsBATCH_QUEUE *    current;

    /* Fast memory heap. */
    glsBATCH_HEAP *     heap;

    /* Active states. */
    glsBATCH_STATE      activeState;
    GLProgram           program;
    GLuint              activeTexture;
}
glsBATCH_INFO;

#endif /* _gc_glsh_batch */
