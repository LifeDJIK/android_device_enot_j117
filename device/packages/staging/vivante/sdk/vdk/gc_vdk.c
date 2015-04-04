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




#include <gc_vdk.h>
#include "gc_hal.h"
#include "gc_hal_eglplatform.h"

/*******************************************************************************
***** Version Signature *******************************************************/

const char * _VDK_PLATFORM = "\n\0$PLATFORM$GAL\n";

struct _vdkPrivate
{
    vdkDisplay  display;
    void *      egl;
};

/*******************************************************************************
** Private functions.
*/
static vdkPrivate _vdk = gcvNULL;

/*******************************************************************************
** Initialization.
*/

VDKAPI vdkPrivate VDKLANG
vdkInitialize(
    void
    )
{
    vdkPrivate  vdk = gcvNULL;
    gceSTATUS   status;

    gcmONERROR(gcoOS_Allocate(gcvNULL, gcmSIZEOF(*vdk), (gctPOINTER *) &vdk));

#if gcdSTATIC_LINK
    vdk->egl = gcvNULL;
#else
    gcmONERROR(gcoOS_LoadEGLLibrary(&vdk->egl));
#endif

    vdk->display = (EGLNativeDisplayType) gcvNULL;

    _vdk = vdk;
    return vdk;

OnError:
    if (vdk != gcvNULL)
    {
        gcoOS_Free(gcvNULL, vdk);
    }

    return gcvNULL;
}

VDKAPI void VDKLANG
vdkExit(
    vdkPrivate Private
    )
{
    if (Private != gcvNULL)
    {
        if (_vdk == Private)
        {
            _vdk = gcvNULL;
        }

        if (Private->egl != gcvNULL)
        {
            gcoOS_FreeEGLLibrary(Private->egl);
        }

        gcoOS_Free(gcvNULL, Private);
    }
}

/*******************************************************************************
** Display.
*/

VDKAPI vdkDisplay VDKLANG
vdkGetDisplayByIndex(
    vdkPrivate Private,
    gctINT DisplayIndex
    )
{
    vdkDisplay  display = (EGLNativeDisplayType) gcvNULL;

    if ((Private != gcvNULL) && (Private->display != (EGLNativeDisplayType) gcvNULL))
    {
        return Private->display;
    }

    if (gcmIS_SUCCESS(gcoOS_GetDisplayByIndex(DisplayIndex, &display)))
    {
        Private->display = display;
    }

    return display;
}

VDKAPI vdkDisplay VDKLANG
vdkGetDisplay(
    vdkPrivate Private
    )
{
    return vdkGetDisplayByIndex(Private, 0);
}

VDKAPI int VDKLANG
vdkGetDisplayInfo(
    vdkDisplay Display,
    int * Width,
    int * Height,
    unsigned long * Physical,
    int * Stride,
    int * BitsPerPixel
    )
{
    return gcmIS_SUCCESS(gcoOS_GetDisplayInfo(Display,
                                              Width, Height,
                                              Physical,
                                              Stride,
                                              BitsPerPixel)) ? 1 : 0;
}

VDKAPI int VDKLANG
vdkGetDisplayInfoEx(
    vdkDisplay Display,
    unsigned int DisplayInfoSize,
    vdkDISPLAY_INFO * DisplayInfo
    )
{
    return gcmIS_SUCCESS(gcoOS_GetDisplayInfoEx(Display,
                                                0,
                                                DisplayInfoSize,
                                                DisplayInfo)) ? 1 : 0;
}

VDKAPI int VDKLANG
vdkGetDisplayVirtual(
    vdkDisplay Display,
    int * Width,
    int * Height
    )
{
    return gcmIS_SUCCESS(gcoOS_GetDisplayVirtual(Display,
                                                 Width, Height)) ? 1 : 0;
}

VDKAPI int VDKLANG
vdkGetDisplayBackbuffer(
    vdkDisplay Display,
    unsigned int * Offset,
    int * X,
    int * Y
    )
{
    gctPOINTER  context;
    gcoSURF     surface;

    return gcmIS_SUCCESS(gcoOS_GetDisplayBackbuffer(Display,
                                                    0, &context, &surface,
                                                    Offset, X, Y)) ? 1 : 0;
}

VDKAPI int VDKLANG
vdkSetDisplayVirtual(
    vdkDisplay Display,
    unsigned int Offset,
    int X,
    int Y
    )
{
    return gcmIS_SUCCESS(gcoOS_SetDisplayVirtual(Display,
                                                 0,
                                                 Offset, X, Y)) ? 1 : 0;
}

VDKAPI void VDKLANG
vdkDestroyDisplay(
    vdkDisplay Display
    )
{
    gcoOS_DestroyDisplay(Display);
}

/*******************************************************************************
** Windows
*/

vdkWindow
vdkCreateWindow(
    vdkDisplay Display,
    int X,
    int Y,
    int Width,
    int Height
    )
{
    vdkWindow   window;

    return gcmIS_SUCCESS(gcoOS_CreateWindow(Display,
                                            X, Y,
                                            Width, Height,
                                            (HALNativeWindowType *) &window)) ? window : 0;
}

VDKAPI int VDKLANG
vdkGetWindowInfo(
    vdkWindow Window,
    int * X,
    int * Y,
    int * Width,
    int * Height,
    int * BitsPerPixel,
    unsigned int * Offset
    )
{
    gceSURF_FORMAT  format;

    if (_vdk == gcvNULL)
    {
        return 0;
    }

    return gcmIS_SUCCESS(gcoOS_GetWindowInfoEx(_vdk->display,
                                               Window,
                                               X, Y,
                                               Width, Height,
                                               BitsPerPixel,
                                               Offset,
                                               &format)) ? 1 : 0;
}

VDKAPI void VDKLANG
vdkDestroyWindow(
    vdkWindow Window
    )
{
    if (_vdk != gcvNULL)
    {
        gcoOS_DestroyWindow(_vdk->display, Window);
    }
}

/*
    vdkDrawImage

    Draw a rectangle from a bitmap to the window client area.

    PARAMETERS:

        vdkWindow Window
            Pointer to the window data structure returned by vdkCreateWindow.

        int Left
            Left coordinate of rectangle.

        int Top
            Top coordinate of rectangle.

        int Right
            Right coordinate of rectangle.

        int Bottom
            Bottom coordinate of rectangle.

        int Width
            Width of the specified bitmap.

        int Height
            Height of the specified bitmap.  If height is negative, the bitmap
            is bottom-to-top.

        int BitsPerPixel
            Color depth of the bitmap.

        void * Bits
            Pointer to the bits of the bitmap.

    RETURNS:

        1 if the rectangle has been copied to the window, or 0 if there is an
        error.

*/
VDKAPI int VDKLANG
vdkDrawImage(
    vdkWindow Window,
    int Left,
    int Top,
    int Right,
    int Bottom,
    int Width,
    int Height,
    int BitsPerPixel,
    void * Bits
    )
{
    if (_vdk == gcvNULL)
    {
        return 0;
    }

    return gcmIS_SUCCESS(gcoOS_DrawImage(_vdk->display,
                                         Window,
                                         Left, Top, Right, Bottom,
                                         Width, Height,
                                         BitsPerPixel,
                                         Bits)) ? 1 : 0;
}

VDKAPI int VDKLANG
vdkShowWindow(
    vdkWindow Window
    )
{
    if (_vdk == gcvNULL)
    {
        return 0;
    }

    return gcmIS_SUCCESS(gcoOS_ShowWindow(_vdk->display, Window)) ? 1 : 0;
}

VDKAPI int VDKLANG
vdkHideWindow(
    vdkWindow Window
    )
{
    if (_vdk == gcvNULL)
    {
        return 0;
    }

    return gcmIS_SUCCESS(gcoOS_HideWindow(_vdk->display, Window)) ? 1 : 0;
}

VDKAPI void VDKLANG
vdkSetWindowTitle(
    vdkWindow Window,
    const char * Title
    )
{
    if (_vdk != gcvNULL)
    {
        gcoOS_SetWindowTitle(_vdk->display, Window, Title);
    }
}

VDKAPI void VDKLANG
vdkCapturePointer(
    vdkWindow Window
    )
{
    if (_vdk != gcvNULL)
    {
        gcoOS_CapturePointer(_vdk->display, Window);
    }
}

/*******************************************************************************
** Events.
*/

VDKAPI int VDKLANG
vdkGetEvent(
    vdkWindow Window,
    vdkEvent * Event
    )
{
    if (_vdk == gcvNULL)
    {
        return 0;
    }

    return gcmIS_SUCCESS(gcoOS_GetEvent(_vdk->display, Window, Event)) ? 1 : 0;
}

/*******************************************************************************
** EGL Support. ****************************************************************
*/

EGL_ADDRESS
vdkGetAddress(
    vdkPrivate Private,
    const char * Function
    )
{
#if gcdSTATIC_LINK
    return (EGL_ADDRESS) eglGetProcAddress(Function);
#else
    union gcsVARIANT
    {
        void *      ptr;
        EGL_ADDRESS func;
    }
    address;

    if ((Private != gcvNULL)
        &&
        gcmIS_SUCCESS(gcoOS_GetProcAddress(gcvNULL,
                                           Private->egl,
                                           Function,
                                           &address.ptr))
        )
    {
        return address.func;
    }

    return gcvNULL;
 #endif
}

/*******************************************************************************
** Time. ***********************************************************************
*/

/*
    vdkGetTicks

    Get the number of milliseconds since the system started.

    PARAMETERS:

        None.

    RETURNS:

        unsigned int
            The number of milliseconds the system has been running.
*/
VDKAPI unsigned int VDKLANG
vdkGetTicks(
    void
    )
{
    return gcoOS_GetTicks();
}

/*******************************************************************************
** Pixmaps. ********************************************************************
*/


VDKAPI vdkPixmap VDKLANG
vdkCreatePixmap(
    vdkDisplay Display,
    int Width,
    int Height,
    int BitsPerPixel
    )
{
    vdkPixmap   pixmap;

    if (gcmIS_SUCCESS(gcoOS_CreatePixmap(Display,
                                         Width, Height,
                                         BitsPerPixel,
                                         (HALNativePixmapType *) &pixmap)))
    {
        return pixmap;
    }

    return 0;
}

VDKAPI int VDKLANG
vdkGetPixmapInfo(
    vdkPixmap Pixmap,
    int * Width,
    int * Height,
    int * BitsPerPixel,
    int * Stride,
    void ** Bits
    )
{
    if (_vdk == gcvNULL)
    {
        return 0;
    }

    return gcmIS_SUCCESS(gcoOS_GetPixmapInfo(_vdk->display,
                                             Pixmap,
                                             Width, Height,
                                             BitsPerPixel,
                                             Stride,
                                             Bits)) ? 1 : 0;
}

VDKAPI void VDKLANG
vdkDestroyPixmap(
    vdkPixmap Pixmap
    )
{
    if (_vdk != gcvNULL)
    {
        gcoOS_DestroyPixmap(_vdk->display, Pixmap);
    }
}

/*
    vdkDrawPixmap

    Draw a rectangle from a bitmap to the pixmap client area.

    PARAMETERS:

        vdkPixmap Pixmap
            Pointer to the pixmap data structure returned by vdkCreatePixmap.

        int Left
            Left coordinate of rectangle.

        int Top
            Top coordinate of rectangle.

        int Right
            Right coordinate of rectangle.

        int Bottom
            Bottom coordinate of rectangle.

        int Width
            Width of the specified bitmap.

        int Height
            Height of the specified bitmap.  If height is negative, the bitmap
            is bottom-to-top.

        int BitsPerPixel
            Color depth of the bitmap.

        void * Bits
            Pointer to the bits of the bitmap.

    RETURNS:

        1 if the rectangle has been copied to the pixmap, or 0 if there is an
        error.
*/
VDKAPI int VDKLANG
vdkDrawPixmap(
    vdkPixmap Pixmap,
    int Left,
    int Top,
    int Right,
    int Bottom,
    int Width,
    int Height,
    int BitsPerPixel,
    void * Bits
    )
{
    if (_vdk == gcvNULL)
    {
        return 0;
    }

    return gcmIS_SUCCESS(gcoOS_DrawPixmap(_vdk->display,
                                          Pixmap,
                                          Left, Top, Right, Bottom,
                                          Width, Height,
                                          BitsPerPixel,
                                          Bits)) ? 1 : 0;
}

/*******************************************************************************
** ClientBuffers. **************************************************************
*/

/* GL_VIV_direct_texture */
#ifndef GL_VIV_direct_texture
#define GL_VIV_YV12						0x8FC0
#define GL_VIV_NV12						0x8FC1
#define GL_VIV_YUY2						0x8FC2
#define GL_VIV_UYVY						0x8FC3
#define GL_VIV_NV21						0x8FC4
#endif

VDKAPI vdkClientBuffer VDKLANG
vdkCreateClientBuffer(
    int Width,
    int Height,
    int Format,
    int Type
    )
{
    vdkClientBuffer clientBuffer;

    if (gcmIS_SUCCESS(gcoOS_CreateClientBuffer(Width, Height,
                                               Format, Type,
                                               (gctPOINTER *) &clientBuffer)))
    {
        return clientBuffer;
    }

    return gcvNULL;
}

VDKAPI int VDKLANG
vdkGetClientBufferInfo(
    vdkClientBuffer ClientBuffer,
    int * Width,
    int * Height,
    int * Stride,
    void ** Bits
    )
{
    return gcmIS_SUCCESS(gcoOS_GetClientBufferInfo(ClientBuffer,
                                                   Width, Height,
                                                   Stride, Bits)) ? 1 : 0;
}

VDKAPI int VDKLANG
vdkDestroyClientBuffer(
    vdkClientBuffer ClientBuffer
    )
{
    return gcmIS_SUCCESS(gcoOS_DestroyClientBuffer(ClientBuffer)) ? 1 : 0;
}
