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

static void glshPatch1(IN GLContext Context, IN GLProgram Program)
{
    /* Enable stencil clear. */
    Context->patchInfo.patchFlags.clearStencil = 1;
}

static void glshPatch2(IN GLContext Context, IN GLProgram Program)
{
    /* Enable certain patches. */
    Context->patchInfo.patchFlags.disableEZ = 1;
    Context->patchInfo.patchFlags.uiDepth   = 1;
    Context->patchInfo.patchFlags.alphaKill = 1;

    /* Set cleanup program. */
    Context->patchInfo.patchCleanupProgram = Program;

    /* Enable the batch stack. */
    Context->patchInfo.patchFlags.stack = 1;
    Context->patchInfo.stackSave        = gcvFALSE;
    Context->patchInfo.stackPtr         = gcvNULL;
    Context->patchInfo.stackFreeList    = gcvNULL;
    Context->patchInfo.allowEZ          = gcvFALSE;

    /* Enable UI patch. */
    Context->patchInfo.patchFlags.ui = 1;
    Context->patchInfo.uiSurface     = gcvNULL;
    Context->patchInfo.uiDepth       = gcvNULL;

    if (Context->patchInfo.patchFlags.ui)
    {
        gcoSURF         surface;
        gceSURF_FORMAT  format;
        gctUINT         samples;

        /* Query format for draw surface. */
        gcoSURF_GetFormat(Context->draw, gcvNULL, &format);

        /* Query numer of smaples for draw surface. */
        gcoSURF_GetSamples(Context->draw, &samples);

        /* For for more multi-sampling. */
        if (samples > 1)
        {
            /* Construct a 1x MSAA surface. */
            if (gcmIS_SUCCESS(gcoSURF_Construct(gcvNULL,
                                                Context->drawWidth,
                                                Context->drawHeight,
                                                1,
                                                gcvSURF_RENDER_TARGET_NO_TILE_STATUS,
                                                format,
                                                gcvPOOL_DEFAULT,
                                                &surface)))
            {
                /* Save the surface. */
                Context->patchInfo.uiSurface = surface;

                /* Set bottom to top (ES 2.0). */
                gcoSURF_SetOrientation(surface, gcvORIENTATION_BOTTOM_TOP);
            }
        }
    }
}

static void glshPatch3(IN GLContext Context, IN GLProgram Program)
{
    Context->patchInfo.patchFlags.blurDepth = 1;
    Context->patchInfo.blurProgram          = Program;
}

static void glshPatch4(IN GLContext Context, IN GLProgram Program)
{
    /* Enable Alpha Kill in GLBenchmark. */
    Context->patchInfo.patchFlags.alphaKill = 1;

#if gldBATCHING
    /* Enable batching in GLBenchmark. */
    if (!Context->patchInfo.patchFlags.batching && (glshBatchStart(Context) == GL_NO_ERROR))
    {
        Context->patchInfo.patchFlags.batching = 1;
    }
#endif

    /* Save program handle to clean up. */
    Context->patchInfo.patchCleanupProgram = Program;
}

static void glshPatch5(IN GLContext Context, IN GLProgram Program)
{
#if gldBATCHING
    /* Enable bathcing in Hoverjet. */
    if (!Context->patchInfo.patchFlags.batching && (glshBatchStart(Context) == GL_NO_ERROR))
    {
        Context->patchInfo.patchFlags.batching = 1;
    }
#endif

    /* Save program handle to clean up. */
    Context->patchInfo.patchCleanupProgram = Program;
}

static void glshPatch6(IN GLContext Context, IN GLProgram Program)
{
    /* Save program handle to clean up. */
    Context->patchInfo.patchCleanupProgram = Program;

    /* Reduce depth scale for GLBenchmark 2.5. */
    Context->patchInfo.patchFlags.depthScale = 1;
}

typedef struct gldPATCH_SHADERS
{
    gctCONST_STRING fromVertex;
    gctCONST_STRING fromFragment;
    void (*handler)(IN GLContext Context, IN GLProgram Program);
}
gldPATCH_SHADERS;

static gldPATCH_SHADERS glshShaders[] =
{
    /* Detect Mirada. */
    {
        /* Vertex shader. */
        "gl_Position=fm_local_to_clip_matrix*vec4(fm_position.xyz,1.0);"
        ,
        /* Fragment shader. */
        "gl_FragColor=fm_delta_diffuse_color;"
        ,
        /* Handler. */
        glshPatch1
    },

    /* Detect water_shader (Navigation). */
    {
        /* Vertex shader. */
        "sin(position.x*fm_speed_vector0.x+position.z*fm_speed_vector0.y+"
        "fm_time.x*fm_speed_vector0.z)"
        ,
        /* Fragment shader. */
        "vec2 tex_pos=v_texcoord*0.8+vec2(fm_time.x*0.02*fm_speed_vector0.x,"
        "fm_time.x*0.017*fm_speed_vector0.y)"
        ,
        /* Handler. */
        glshPatch2
    },

    /* Detect horizontal blur filter. */
    {
        /* Vertex shader. */
        "v_texcoord[0]=vec2(fm_texcoord0.x+(-3.0)*scale,fm_texcoord0.y);"
        ,
        /* Fragment shader. */
        "color=w1*max(texture2D(fm_texture1,v_texcoord[0]).rgba-minus_color,"
        "vec4(0.0));"
        ,
        glshPatch3
    },

    /* Detect iris shader. */
    {
        /* Vertex shader. */
        gcvNULL
        ,
        /* Fragment shader. */
        "#ifdefGL_ES"
        "precisionmediumpfloat;"
        "#endif"
        "uniformsamplerCubeuniSamplerCubeRad;"
        "uniformsamplerCubeuniSamplerCubeRad2;"
        "uniformfloatuniSamplerCubeDistance;"
        "uniformfloatuniSamplerCubeDistance2;"
        "uniformvec3uniViewDir;"
        "voidmain()"
        "{"
        "floatw1=uniSamplerCubeDistance2/(uniSamplerCubeDistance+uniSamplerCubeDistance2);"
        "vec3iriscolor=(textureCube(uniSamplerCubeRad,uniViewDir).rgb)*w1+(textureCube(uniSamplerCubeRad2,uniViewDir).rgb)*(1.-w1);"
        "floatirisval=dot(iriscolor,vec3(0.3,0.59,0.11));"
        "irisval=sqrt(.67/irisval);"
        "gl_FragColor=vec4(floor(irisval)/3.0,fract(irisval),1.0,1.0);"
        "}"
        ,
        glshPatch4
    },

    /* Detect torch shader. */
    {
        /* Vertex shader. */
        "#ifdefGL_ES"
        "precisionhighpfloat;"
        "#endif"
        "uniformmat4myMVProjMatrix;"
        "uniformfloatparticleSize;"
        "attributevec4myVertex;"
        "attributevec4myColor;"
        "varyingvec4color;"
        "voidmain(void)"
        "{"
        "gl_PointSize=particleSize;"
        "color=myColor;"
        "gl_Position=myMVProjMatrix*myVertex;"
        "}"
        ,
        /* Fragment shader. */
        "#ifdefGL_ES"
        "precisionmediumpfloat;"
        "#endif"
        "varyingvec4color;"
        "voidmain(void)"
        "{"
        "gl_FragColor=color;"
        "}"
        ,
        glshPatch4
    },

    /* Detect Hoverjet. */
    {
        /* Vertex shader. */
        "attributevec3fm_position;"
        "attributevec3fm_normal;"
        "attributevec2fm_texcoord0;"
        "uniformvec4fm_view_position;"
        "uniformmat4fm_local_to_clip_matrix;"
        "uniformmat4fm_local_to_world_matrix;"
        "varyingmediumpvec3v_normal;"
        "varyingmediumpvec2v_texcoord0;"
        "varyingmediumpvec3v_view_direction;"
        "voidmain(void)"
        "{"
            "highpvec3position=vec3(fm_local_to_world_matrix*vec4(fm_position,1.0));"
            "gl_Position=fm_local_to_clip_matrix*vec4(fm_position,1.0);"
            "v_normal=vec3(fm_local_to_world_matrix*vec4(fm_normal,0.0));"
            "v_texcoord0=fm_texcoord0;"
            "v_view_direction=fm_view_position.xyz-position;"
        "}"
        ,
        /* Fragment shader. */
        "uniformmediumpvec4fm_light_diffuse_color;"
        "uniformmediumpvec4fm_light_direction;"
        "uniformmediumpvec4fm_object_shadow;"
        "uniformmediumpvec4fm_delta_diffuse_color;"
        "uniformmediumpvec4fm_delta_specular_color;"
        "uniformmediumpvec4fm_delta_specular_exponent;"
        "uniformmediumpvec4fm_color_posx;"
        "uniformmediumpvec4fm_color_posy;"
        "uniformmediumpvec4fm_color_negy;"
        "uniformmediumpvec4fm_shadow_bias;"
        "uniformmediumpvec4fm_vn_color;"
        "uniformmediumpvec4fm_vn_scale;"
        "uniformmediumpvec4fm_vn_exponent;"
        "uniformsampler2Dfm_texture0;"
        "varyingmediumpvec3v_normal;"
        "varyingmediumpvec3v_view_direction;"
        "varyingmediumpvec2v_texcoord0;"
        "voidmain(void)"
        "{"
            "mediumpvec3N=normalize(v_normal);"
            "mediumpvec3V=normalize(v_view_direction);"
            "mediumpvec3L=normalize(fm_light_direction.xyz);"
            "mediumpvec3H=normalize(L+V);"
            "mediumpfloatln=dot(L,N);"
            "//mediumpfloatvn=dot(V,N);"
            "mediumpfloathn=dot(H,N);"
            "//mediumpfloatvn_clamped=max(0.0,vn);"
            "mediumpfloatln_clamped=max(0.0,ln);"
            "//Diffuse"
            "mediumpvec3delta_diffuse=ln_clamped*fm_delta_diffuse_color.rgb;"
            "//Fresnel"
            "//mediumpvec3fresnel=pow(1.0-vn_clamped,fm_vn_exponent.x)*fm_vn_scale.x*fm_vn_color.rgb*ln_clamped;"
            "//Shadow"
            "mediumpvec3shadow=clamp(fm_object_shadow.rgb+fm_shadow_bias.xxx,0.0,1.0);"
            "//Ambientdiffuse"
            "mediumpvec3ambient_diffuse=vec3(0.0);"
            "ambient_diffuse+=max(0.0,1.0-abs(N.y))*fm_color_posx.rgb;"
            "ambient_diffuse+=max(0.0,N.y)*fm_color_posy.rgb;"
            "ambient_diffuse+=max(0.0,-N.y)*fm_color_negy.rgb;"
            "//Specular"
            "mediumpvec3delta_specular=vec3(0.0);"
            "if(ln>0.0)"
            "{"
                "delta_specular=pow(max(hn,0.0),fm_delta_specular_exponent.x)*fm_delta_specular_color.rgb;"
            "}"
            "mediumpvec4tex=texture2D(fm_texture0,v_texcoord0);"
            "gl_FragColor.rgb=shadow*tex.rgb*(delta_diffuse+ambient_diffuse)+tex.a*delta_specular;"
            "gl_FragColor.a=1.0;"
        "}"
        ,
        glshPatch5
    },

    /* Detect GLBenchmark 2.5. */
    {
        /* Vertex shader. */
        "void main()"
        "{"
            "gl_Position=mvp*vec4(in_position,1.0);"
            "gl_Position.z=gl_Position.w;"
            "out_texcoord0=in_texcoord0;"
        "}"
        ,
        /* Fragment shader. */
        "void main()"
        "{"
            "gl_FragColor=texture2D(texture_unit0,out_texcoord0);"
        "}"
        ,
        glshPatch6
    },

    { gcvNULL }
};

static gctCONST_STRING glshFindString(IN gctCONST_STRING String,
                                      IN gctCONST_STRING Search,
                                      OUT gctINT_PTR SearchIndex)
{
    gctCONST_STRING source;
    gctINT          sourceIndex;
    gctCONST_STRING search;

    /* Start from the beginning of the source string. */
    source = String;

    /* Reset search index. */
    sourceIndex = 0;
    search      = Search;

    /* Loop until the end of the source string. */
    while (source[sourceIndex] != '\0')
    {
        /* Check if we match the current search character. */
        if (source[sourceIndex] == *search)
        {
            /* Increment the source index. */
            sourceIndex ++;

            /* Increment the search index and test for end of string. */
            if (*++search == '\0')
            {
                /* Return the current index. */
                *SearchIndex = sourceIndex;

                /* Return the location of the searcg string. */
                return source;
            }
        }

        /* Skip any white space in the source string. */
        else if ((source[sourceIndex] == ' ')
                 ||
                 (source[sourceIndex] == '\t')
                 ||
                 (source[sourceIndex] == '\r')
                 ||
                 (source[sourceIndex] == '\n')
                 ||
                 (source[sourceIndex] == '\\')
                 )
        {
            /* Increment the source index. */
            sourceIndex++;
        }

        /* No match. */
        else
        {
            /* Next source character. */
            source++;

            /* Reset search index. */
            sourceIndex = 0;
            search      = Search;
        }
    }

    /* No match. */
    return gcvNULL;
}

void glshPatchLink(IN GLContext Context,
                   IN GLProgram Program)
{
    gctCONST_STRING     searchVS;
    gctINT              searchVSIndex = 0;
    gctCONST_STRING     searchFS;
    gctINT              searchFSIndex = 0;
    gldPATCH_SHADERS *  shader;

    /* Loop all entries of the patch table. */
    for (shader = glshShaders;
         (shader->fromVertex != gcvNULL) || (shader->fromFragment != gcvNULL);
         shader++)
    {
        /* Check if there is a vertex shader search string. */
        if ((Program->vertexShader->source != gcvNULL)
            &&
            (shader->fromVertex != gcvNULL)
            )
        {
            /* Find the search string in the vertex shader. */
            searchVS = glshFindString(Program->vertexShader->source,
                                      shader->fromVertex,
                                      &searchVSIndex);
        }
        else
        {
            /* No vertex shader search string */
            searchVS = gcvNULL;
        }

        /* Check if there is a fragment shader search string. */
        if ((Program->fragmentShader->source != gcvNULL)
            &&
            (shader->fromFragment != gcvNULL)
            )
        {
            /* Find the search string in the fragment shader. */
            searchFS = glshFindString(Program->fragmentShader->source,
                                      shader->fromFragment,
                                      &searchFSIndex);
        }
        else
        {
            /* No fragment shader search string */
            searchFS = gcvNULL;
        }

        /* Check if we need a search string from both shaders. */
        if ((shader->fromVertex != gcvNULL)
            &&
            (shader->fromFragment != gcvNULL)
            )
        {
            /* If any search turned up nothing, abort. */
            if ((searchVS == gcvNULL) || (searchFS == gcvNULL))
            {
                continue;
            }
        }

        /* Check if we need a search string from the vertex shader. */
        else if (shader->fromVertex != gcvNULL)
        {
            /* If the search turned up nothing, abort. */
            if (searchVS == gcvNULL)
            {
                continue;
            }
        }

        /* Check if we need a search string from the fragment shader. */
        else if (shader->fromFragment != gcvNULL)
        {
            /* If the search turned up nothing, abort. */
            if (searchFS == gcvNULL)
            {
                continue;
            }
        }

        /* Check if there is a handler. */
        if (shader->handler != gcvNULL)
        {
            /* Call the handler. */
            shader->handler(Context, Program);
        }
    }
}

static void glshPatchUI(IN GLContext Context, gctBOOL UI)
{
    if (UI)
    {
        gctUINT samples;

        if ((Context->patchInfo.uiDepth == gcvNULL)
            &&
            (Context->framebuffer == gcvNULL)
            &&
            gcmIS_SUCCESS(gcoSURF_GetSamples(Context->draw, &samples))
            &&
            (samples > 1)
            &&
            (++Context->patchInfo.uiCount == 2)
            )
        {
            /* Downsample the current draw buffer into the UI surface. */
            gcoSURF_Resolve(Context->draw, Context->patchInfo.uiSurface);

            /* Reset the draw and depth target. */
            gco3D_UnsetTarget(Context->engine, Context->draw);
            gco3D_UnsetTarget(Context->engine, Context->depth);
            Context->framebufferChanged = GL_TRUE;

            /* Save current read buffer. */
            Context->patchInfo.uiRead = Context->read;

            /* Swap the UI surface into the current draw surface. */
            gcoSURF_Swap(Context->draw, Context->patchInfo.uiSurface);
            Context->read = Context->draw;

            /* Remove depth buffer. */
            Context->patchInfo.uiDepth = Context->depth;
            Context->depth             = gcvNULL;
        }
    }
    else
    {
        if (Context->patchInfo.uiDepth != gcvNULL)
        {
            /* Reset the draw and depth target. */
            gco3D_UnsetTarget(Context->engine, Context->draw);
            Context->framebufferChanged = gcvTRUE;

            /* Swap the UI surface out of the current draw surface. */
            gcoSURF_Swap(Context->draw, Context->patchInfo.uiSurface);

            /* Re-enable the depth buffer. */
            Context->depth             = Context->patchInfo.uiDepth;
            Context->patchInfo.uiDepth = gcvNULL;

            /* Restore the read buffer. */
            Context->read = Context->patchInfo.uiRead;
        }

        /* Reset UI counter. */
        Context->patchInfo.uiCount = 0;
    }
}

GLbitfield glshPatchClear(IN GLContext Context,
                          IN GLbitfield Mask)
{
    GLbitfield  mask = Mask;

    if (Mask & GL_DEPTH_BUFFER_BIT)
    {
        /* Test if we need to clear the stencil as well. */
        if (Context->patchInfo.patchFlags.clearStencil)
        {
            mask |= GL_STENCIL_BUFFER_BIT;
        }

        /* Check if depth mask is disabled for UI mode. */
        if (Context->patchInfo.patchFlags.uiDepth
            &&
            (Context->depthMask == GL_FALSE)
            )
        {
            /* Enable depth mask again. */
            glDepthMask(GL_TRUE);
        }
    }

    /* Test if we need a UI patch. */
    if (Context->patchInfo.patchFlags.ui)
    {
        if (Context->patchInfo.uiSurface != gcvNULL)
        {
            /* Get out of the UI mode. */
            glshPatchUI(Context, gcvFALSE);
        }
    }

    if (Context->patchInfo.patchFlags.stack
        &&
        (Mask & GL_COLOR_BUFFER_BIT)
        &&
        (Context->framebuffer == gcvNULL)
        )
    {
        Context->patchInfo.stackPtr  = gcvNULL;
        Context->patchInfo.stackSave = gcvTRUE;
    }

    /* Return the (updated) mask. */
    return mask;
}

gctBOOL glshQueryPatchEZ(IN GLContext Context)
{
    if (Context->patchInfo.patchFlags.stack)
    {
        return Context->patchInfo.allowEZ || (Context->colorWrite == 0);
    }

    /* Test if EZ should be disabled. */
    if (Context->patchInfo.patchFlags.disableEZ)
    {
        /* Always disable EZ. */
        return gcvFALSE;
    }

    /* EZ is allowed by default. */
    return gcvTRUE;
}

gctBOOL glshQueryPatchAlphaKill(IN GLContext Context)
{
    return Context->patchInfo.patchFlags.alphaKill;
}

static gctBOOL glshPatchBatch(IN GLContext Context,
                              IN glsPATCH_BATCH * Batch,
                              IN gctBOOL Store)
{
    gctINT          i;
    gctSIZE_T       bytes;
    GLProgram       program;
    gctUINT8_PTR    data;

    if (Store)
    {
        /* Copy the currently bound vertex and element buffers. */
        Batch->vertexBuffer  = Context->arrayBuffer;
        Batch->elementBuffer = Context->elementArrayBuffer;

        /* Copy the current vertex array information. */
        gcoOS_MemCopy(Batch->vertexArray,
                      Context->vertexArray,
                      gcmSIZEOF(Context->vertexArray));

        /* Copy the current program. */
        program = Batch->program = Context->program;

        /* Process all uniforms. */
        for (i = 0, bytes = 0; i < program->uniformCount; i++)
        {
            /* Compute the total number of bytes required for saving the
             * uniforms. */
            bytes += program->uniforms[i].bytes;
        }

        /* Allocate the uniform save area. */
        if (gcmIS_ERROR(gcoOS_Allocate(gcvNULL, bytes, &Batch->uniformData)))
        {
            return gcvFALSE;
        }

        /* Initialize the save pointer. */
        data = (gctUINT8_PTR) Batch->uniformData;

        /* Process all uniforms. */
        for (i = 0; i < program->uniformCount; i++)
        {
            /* Save the unifmr data into the save area. */
            gcoOS_MemCopy(data,
                          program->uniforms[i].data,
                          program->uniforms[i].bytes);

            /* Advance the save pointer. */
            data += program->uniforms[i].bytes;
        }

        /* Store all bound textures. */
        gcoOS_MemCopy(Batch->textures2D,
                      Context->texture2D,
                      gcmSIZEOF(Batch->textures2D));

        gcoOS_MemCopy(Batch->texturesCube,
                      Context->textureCube,
                      gcmSIZEOF(Batch->texturesCube));
    }
    else
    {
        /* Restore the vertex and element buffers. */
        Context->arrayBuffer        = Batch->vertexBuffer;
        Context->elementArrayBuffer = Batch->elementBuffer;

        /* Restore the vertex array information. */
        gcoOS_MemCopy(Context->vertexArray,
                      Batch->vertexArray,
                      gcmSIZEOF(Context->vertexArray));

        if (Context->program != Batch->program)
        {
            /* Restore the current program. */
            Context->program      = Batch->program;
            Context->programDirty = GL_TRUE;
        }

        /* Initialize the save pointer. */
        program = Batch->program;
        data    = (gctUINT8_PTR) Batch->uniformData;

        /* Process all uniforms. */
        for (i = 0; i < program->uniformCount; i++)
        {
            /* Restore the uniform data from the save area. */
            gcoOS_MemCopy(program->uniforms[i].data,
                          data,
                          program->uniforms[i].bytes);

            /* Mark the uniform as dirty. */
            program->uniforms[i].dirty = GL_TRUE;

            /* Advance the save pointer. */
            data += program->uniforms[i].bytes;
        }

        /* FRee the uniform data. */
        gcoOS_Free(gcvNULL, Batch->uniformData);

        /* Restore all bound textures. */
        gcoOS_MemCopy(Context->texture2D,
                      Batch->textures2D,
                      gcmSIZEOF(Batch->textures2D));

        gcoOS_MemCopy(Context->textureCube,
                      Batch->texturesCube,
                      gcmSIZEOF(Batch->texturesCube));

        /* Batch is dirty. */
        Context->batchDirty = GL_TRUE;
    }

    /* Batch processed. */
    return gcvTRUE;
}

static void glshBatchPlay(IN GLContext Context)
{
    glsPATCH_BATCH      savedBatch;
    glsPATCH_BATCH *    batch;

    /* Disable stack saving. */
    Context->patchInfo.stackSave = gcvFALSE;

    if (Context->patchInfo.stackPtr != gcvNULL)
    {
        /* Save the current batch. */
        glshPatchBatch(Context, &savedBatch, gcvTRUE);

        /* Turn on EZ. */
        Context->patchInfo.allowEZ = gcvTRUE;
        Context->depthDirty        = GL_TRUE;

        /* Process all batches on the stack. */
        while (Context->patchInfo.stackPtr != gcvNULL)
        {
            /* Pop a batch structure from the top of the stack. */
            batch                       = Context->patchInfo.stackPtr;
            Context->patchInfo.stackPtr = batch->next;

            /* Restore the batch. */
            glshPatchBatch(Context, batch, gcvFALSE);

            /* Execute the batch. */
            glDrawElements(batch->mode,
                           batch->count,
                           batch->type,
                           batch->indices);

            /* Push the batch on top of the free list. */
            batch->next                      = Context->patchInfo.stackFreeList;
            Context->patchInfo.stackFreeList = batch;
        }

        /* Restore the current batch. */
        glshPatchBatch(Context, &savedBatch, gcvFALSE);

        /* Turn off EZ. */
        Context->patchInfo.allowEZ = gcvFALSE;
        Context->depthDirty        = GL_TRUE;
    }
}

gctBOOL glshPatchDraw(IN GLContext Context, IN GLenum Mode, IN GLsizei Count,
                      IN GLenum Type, IN const GLvoid * Indices)
{
    /* Test if we need to optimize the UI. */
    if (Context->patchInfo.patchFlags.uiDepth
        ||
        Context->patchInfo.patchFlags.ui
        )
    {
        /* If we need to render 2 triangles and both culling and dithering are
         * disabled, and depth function is set to ALWAYS, and blending is turned
         * on, we are in UI mode. */
        if ((Mode == GL_TRIANGLES)
            &&
            (Count == 6)
            &&
            (Context->cullEnable == GL_FALSE)
            &&
            (Context->ditherEnable == GL_FALSE)
            &&
            (Context->depthFunc == GL_ALWAYS)
            &&
            (Context->blendEnable == GL_TRUE)
            )
        {
            if (Context->patchInfo.patchFlags.ui
                &&
                (Context->patchInfo.uiSurface != gcvNULL)
                )
            {
                /* Enter the UI mode. */
                glshPatchUI(Context, gcvTRUE);
            }

            if (Context->patchInfo.patchFlags.uiDepth && Context->depthMask)
            {
                /* We don't need depth writes in UI mode. */
                glDepthMask(GL_FALSE);
            }
        }
    }

    /* Check if we are in the blur program. */
    if (Context->patchInfo.patchFlags.blurDepth
        &&
        (Context->program == Context->patchInfo.blurProgram)
        )
    {
        /* Since we don't care about Z, set compare to always. */
        glDepthFunc(GL_ALWAYS);
    }

    if (Context->patchInfo.patchFlags.stack
        &&
        Context->patchInfo.stackSave
        )
    {
        if ((Context->arrayBuffer != gcvNULL)
            &&
            (Context->elementArrayBuffer != gcvNULL)
            )
        {
            glsPATCH_BATCH * batch;

            /* Get a batch structure from the free list. */
            batch = Context->patchInfo.stackFreeList;

            if (batch == gcvNULL)
            {
                /* Allocate a batch structure. */
                gcoOS_Allocate(gcvNULL,
                               gcmSIZEOF(glsPATCH_BATCH),
                               (gctPOINTER *) &batch);

                if (batch == gcvNULL)
                {
                    /* Process the batch. */
                    return gcvTRUE;
                }
            }
            else
            {
                /* Unlink this batch from the free list. */
                Context->patchInfo.stackFreeList = batch->next;
            }

            /* Save the current batch into the batch. */
            if (!glshPatchBatch(Context, batch, gcvTRUE))
            {
                /* Push batch structure on top of free list. */
                batch->next = Context->patchInfo.stackFreeList;
                Context->patchInfo.stackFreeList = batch->next;

                /* Did not save the batch - process now. */
                return gcvTRUE;
            }

            /* Save draw arguments. */
            batch->mode    = Mode;
            batch->count   = Count;
            batch->type    = Type;
            batch->indices = Indices;

            /* Push the batch on top of the stack. */
            batch->next = Context->patchInfo.stackPtr;
            Context->patchInfo.stackPtr = batch;

            /* Batch is saved - return. */
            return gcvFALSE;
        }
    }

    if (Context->patchInfo.patchFlags.depthScale && (Context->depthFar == 1.0f))
    {
        /* Reduce depth scale by a little. */
        *(GLuint *) &Context->depthFar -= 1;
        Context->depthDirty = GL_TRUE;
    }

    /* Process the batch. */
    return gcvTRUE;
}

void glshPatchDeleteProgram(IN GLContext Context,
                            IN GLProgram Program)
{
    /* Test if the program matches the cleanup program. */
    if (Context->patchInfo.patchCleanupProgram == Program)
    {
        /* Test for UI patch. */
        if (Context->patchInfo.patchFlags.ui)
        {
            /* Check if we have a valid UI surface. */
            if (Context->patchInfo.uiSurface != gcvNULL)
            {
                /* Get out of the UI mode. */
                glshPatchUI(Context, gcvFALSE);

                /* Destroy the UI surface. */
                gcoSURF_Destroy(Context->patchInfo.uiSurface);
                Context->patchInfo.uiSurface = gcvNULL;
            }

            /* Remove the UI patch flag. */
            Context->patchInfo.patchFlags.ui = 0;
        }

        /* Disable patches. */
        Context->patchInfo.patchFlags.uiDepth   = 0;
        Context->patchInfo.patchFlags.blurDepth = 0;
        Context->patchInfo.patchFlags.disableEZ = 0;
        Context->patchInfo.patchFlags.alphaKill = 0;

        /* Test for batching. */
        if (Context->patchInfo.patchFlags.batching)
        {
            /* Disable batching. */
            Context->patchInfo.patchFlags.batching = 0;

            /* Stop the batching. */
            glshBatchStop(Context);
        }

        /* Clean up the stack. */
        if (Context->patchInfo.patchFlags.stack)
        {
            /* Process the current stack. */
            while (Context->patchInfo.stackPtr != gcvNULL)
            {
                /* Pop the top of the list. */
                glsPATCH_BATCH * batch = Context->patchInfo.stackPtr;
                Context->patchInfo.stackPtr = batch->next;

                /* Free the structure. */
                gcoOS_Free(gcvNULL, batch);
            }

            /* Process the list of free batch structures. */
            while (Context->patchInfo.stackFreeList != gcvNULL)
            {
                /* Pop the top of the list. */
                glsPATCH_BATCH * batch = Context->patchInfo.stackFreeList;
                Context->patchInfo.stackFreeList = batch->next;

                /* Free the structure. */
                gcoOS_Free(gcvNULL, batch);
            }

            /* Turn off the stack option. */
            Context->patchInfo.patchFlags.stack = 0;
        }

        /* Cleanup down, so remove program. */
        Context->patchInfo.patchCleanupProgram = gcvNULL;
    }
}

void glshPatchBlend(IN GLContext Context, IN gctBOOL Enable)
{
    /* Check if blending gets enabled and we have a valid stack. */
    if (Context->patchInfo.patchFlags.stack
        &&
        Enable
        &&
        Context->patchInfo.stackSave
        )
    {
        /* Play the batches. */
        glshBatchPlay(Context);
    }
}

#endif /* gldPATCHES */
