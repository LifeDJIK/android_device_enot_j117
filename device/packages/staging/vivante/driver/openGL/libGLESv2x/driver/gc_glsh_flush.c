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
#include <EGL/egl.h>

GL_APICALL GLvoid GL_APIENTRY glFlush(GLvoid)
{
#if gcdNULL_DRIVER < 3
    glmENTER()
    {
        gcmDUMP_API("${ES20 glFlush}");

        glmPROFILE(context, GLES2_FLUSH, 0);

#if gldPATCHES
        /* Do batching. */
        glmBATCH(context,
                 break,
                 break,
                 (glshBatchFlush(context)));
#endif

        /* Do the flush. */
        glshFlush(context);
	}
	glmLEAVE()
#endif
}

GLenum glshFlush(IN GLContext Context)
{
    /* Do the flush. */
    _glshFlush(Context, gcvFALSE);

    /* Disable batch optmization. */
    Context->batchDirty = GL_TRUE;

    /* Success. */
    return GL_NO_ERROR;
}

GL_APICALL GLvoid GL_APIENTRY glFinish(GLvoid)
{
#if gcdNULL_DRIVER < 3
    glmENTER()
    {
        gcmDUMP_API("${ES20 glFinish}");

        glmPROFILE(context, GLES2_FINISH, 0);

#if gldPATCHES
        /* Do batching. */
        glmBATCH(context,
                 break,
                 break,
                 (glshBatchFlush(context)));
#endif

        /* Do the finish. */
        glshFinish(context);
	}
	glmLEAVE()
#endif
}

GLenum glshFinish(IN GLContext Context)
{
    /* Do the finish. */
    _glshFlush(Context, GL_TRUE);

    /* TODO: Need to do pixmap resolve after hide the pixmap wordaround in HAL
     * Currently design cannot access EGL surface in GLES.
     */
    eglWaitClient();

#if gcdFRAME_DB
    /* Dump the framebuffer database. */
    gcoHAL_DumpFrameDB(gcdFRAME_DB_NAME);
#endif

    /* Disable batch optimization. */
    Context->batchDirty = GL_TRUE;

    /* Success. */
    return GL_NO_ERROR;
}
