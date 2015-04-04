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



#include "gc_gpe_precomp.h"
#include <ceddk.h>

//////////////////////////////////////////////////////////////////////////////
//              GC2DSurf
//////////////////////////////////////////////////////////////////////////////

GC2DSurf::GC2DSurf()
{
    mType = GC2D_OBJ_UNKNOWN;
    mFlag = 0;
    mAcc2D = gcvNULL;

    mDMAAddr = INVALID_PHYSICAL_ADDRESS;
    mPhysicalAddr = INVALID_PHYSICAL_ADDRESS;
    mVirtualAddr = gcvNULL;

    mMapInfo = gcvNULL;
    mGalSurf = gcvNULL;

    mStride = 0;
    mWidth = 0;
    mHeight = 0;
    mSize = 0;
    mFormat = gcvSURF_UNKNOWN;
    mRotate = gcvSURF_0_DEGREE;


    mOwnerFlag = 0;

    mUDMAAddr = INVALID_PHYSICAL_ADDRESS;
    mUVirtualAddr = gcvNULL;
    mUPhysicalAddr = INVALID_PHYSICAL_ADDRESS;
    mUstride = 0;
    mUMapInfo = gcvNULL;

    mVDMAAddr = INVALID_PHYSICAL_ADDRESS;
    mVVirtualAddr = gcvNULL;
    mVPhysicalAddr = INVALID_PHYSICAL_ADDRESS;
    mVstride = 0;
    mVMapInfo = gcvNULL;
}

GC2DSurf::~GC2DSurf()
{
    if (IsValid())
    {
        Denitialize(gcvTRUE);
    }
}

gceSTATUS GC2DSurf::Initialize(
        GC2D_Accelerator_PTR pAcc2D,
        gctUINT flag, gctUINT PhysAddr, gctPOINTER VirtAddr,
        gctUINT width, gctUINT height, gctUINT stride,
        gceSURF_FORMAT format, gceSURF_ROTATION rotate)
{
    // already initialized
    if (mType != GC2D_OBJ_UNKNOWN)
    {
        return gcvSTATUS_INVALID_OBJECT;
    }

    if ((pAcc2D == gcvNULL) || (!pAcc2D->GC2DIsValid()))
    {
        return gcvSTATUS_INVALID_ARGUMENT;
    }

    // check format
    switch (format)
    {
    // list of the supported formats except planar YUV
    case    gcvSURF_X4R4G4B4:
    case    gcvSURF_A4R4G4B4:
    case    gcvSURF_X1R5G5B5:
    case    gcvSURF_A1R5G5B5:
    case    gcvSURF_R5G6B5  :
    case    gcvSURF_X8R8G8B8:
    case    gcvSURF_A8R8G8B8:
    case    gcvSURF_INDEX8  :
    case    gcvSURF_YUY2:
    case    gcvSURF_UYVY:
        break;

    default:
        return gcvSTATUS_INVALID_ARGUMENT;
    }

    // check width , height & stride
    if (width >= MAX_WIDTH_HEIGHT || height >= MAX_WIDTH_HEIGHT
        || stride >= MAX_STRIDE || width > stride)
    {
        return gcvSTATUS_INVALID_ARGUMENT;
    }

    // alignment check
    if (gcmNO_SUCCESS(AlignWidthHeight(&width, &height)))
    {
        return gcvSTATUS_INVALID_ARGUMENT;
    }

    // check buffer, can not wrap without buffer
    if ((PhysAddr == INVALID_PHYSICAL_ADDRESS) && (VirtAddr == gcvNULL))
    {
        return gcvSTATUS_INVALID_ARGUMENT;
    }

    // check flags
    if (flag & (~GC2DSURF_PROPERTY_ALL))
    {
        // unknown flag
        return gcvSTATUS_INVALID_ARGUMENT;
    }

    mFormat = format;
    mRotate = rotate;
    mWidth = width;
    mHeight = height;
    mStride = stride;
    mSize = stride * height;
    mFlag = flag;
    mPhysicalAddr = PhysAddr;
    mVirtualAddr = VirtAddr;

    // Map to GPU address
    gceSTATUS status = gcoHAL_MapUserMemory(mVirtualAddr, mPhysicalAddr, mSize, &mMapInfo, &mDMAAddr);
    if (gcmNO_SUCCESS(status))
    {
        return status;
    }

    if ((mFlag & GC2DSURF_PROPERTY_CACHABLE) && mVirtualAddr)
    {
        CacheRangeFlush(mVirtualAddr, mSize, CACHE_SYNC_ALL);
    }

    // Init the member variables.
    mAcc2D = pAcc2D;
    mGalSurf = gcvNULL;

    mUDMAAddr = INVALID_PHYSICAL_ADDRESS;
    mUPhysicalAddr = INVALID_PHYSICAL_ADDRESS;
    mUVirtualAddr = gcvNULL;
    mUstride = 0;
    mUMapInfo = gcvNULL;

    mVDMAAddr = INVALID_PHYSICAL_ADDRESS;
    mVPhysicalAddr = INVALID_PHYSICAL_ADDRESS;
    mVVirtualAddr = gcvNULL;
    mVstride = 0;
    mVMapInfo = gcvNULL;

    mType = GC2D_OBJ_SURF;

    return gcvSTATUS_OK;
}

// if both UXXXXAddr and VXXXXAddr are NULL, we take the U component and V component
//  contiguous with Y component
gceSTATUS GC2DSurf::InitializeYUV(
        GC2D_Accelerator_PTR pAcc2D,
        gctUINT flag, gceSURF_FORMAT format,
        gctUINT YPhysAddr, gctPOINTER YVirtAddr, gctUINT YStride,
        gctUINT UPhysAddr, gctPOINTER UVirtAddr, gctUINT UStride,
        gctUINT VPhysAddr, gctPOINTER VVirtAddr, gctUINT VStride,
        gctUINT width, gctUINT height, gceSURF_ROTATION rotate
        )
{
    gceSTATUS status = gcvSTATUS_OK;
    gctBOOL UVContingous =
            (UPhysAddr == INVALID_PHYSICAL_ADDRESS && UVirtAddr == gcvNULL
            && VPhysAddr == INVALID_PHYSICAL_ADDRESS && VVirtAddr == gcvNULL);
    gctUINT32 UVSize;

    // already initialized
    if (mType != GC2D_OBJ_UNKNOWN)
    {
        return gcvSTATUS_INVALID_OBJECT;
    }

    if ((pAcc2D == gcvNULL) || (!pAcc2D->GC2DIsValid()))
    {
        return gcvSTATUS_INVALID_ARGUMENT;
    }

    // check format
    switch (format)
    {
    // list of the supported formats
    case    gcvSURF_YV12:
    case    gcvSURF_I420:
        if (!UVContingous)
        {
            if (((UPhysAddr == INVALID_PHYSICAL_ADDRESS) && (UVirtAddr == gcvNULL))
                || ((VPhysAddr == INVALID_PHYSICAL_ADDRESS) && (VVirtAddr == gcvNULL)))
            {
                return gcvSTATUS_INVALID_ARGUMENT;
            }
        }

        if ((YPhysAddr == INVALID_PHYSICAL_ADDRESS) && (YVirtAddr == gcvNULL))
        {
            return gcvSTATUS_INVALID_ARGUMENT;
        }
        break;

    case    gcvSURF_YUY2:
    case    gcvSURF_UYVY:
        if (!UVContingous)
        {
            return gcvSTATUS_INVALID_ARGUMENT;
        }
        else
        {
            return Initialize(pAcc2D, flag, YPhysAddr, YVirtAddr, width, height, YStride, format, rotate);
        }

    default:
        return gcvSTATUS_INVALID_ARGUMENT;
    }

    // check width , height & stride
    if (width >= MAX_WIDTH_HEIGHT || height >= MAX_WIDTH_HEIGHT
        || YStride >= MAX_STRIDE || UStride >= MAX_STRIDE
        || VStride >= MAX_STRIDE)
    {
        return gcvSTATUS_INVALID_ARGUMENT;
    }

    // alignment check
    if (gcmNO_SUCCESS(AlignWidthHeight(&width, &height)))
    {
        return gcvSTATUS_INVALID_ARGUMENT;
    }

    // check flags
    if (flag & (~GC2DSURF_PROPERTY_ALL))
    {
        // unknown flag
        return gcvSTATUS_INVALID_ARGUMENT;
    }

    if (!(flag & GC2DSURF_PROPERTY_CONTIGUOUS) && YVirtAddr == gcvNULL)
    {
        return gcvSTATUS_INVALID_ARGUMENT;
    }

    // Total size.
    mSize = height * YStride;

    if (UVContingous)
    {
        mSize += mSize >> 1;
    }

    // Map to GPU address
    status = gcoHAL_MapUserMemory(YVirtAddr, YPhysAddr, mSize, &mMapInfo, &mDMAAddr);
    if (gcmNO_SUCCESS(status))
    {
        return status;
    }

    if ((flag & GC2DSURF_PROPERTY_CACHABLE) && YVirtAddr)
    {
        CacheRangeFlush(YVirtAddr, mSize, CACHE_SYNC_ALL);
    }

    mPhysicalAddr = YPhysAddr;
    mVirtualAddr = YVirtAddr;
    mWidth = width;
    mHeight = height;
    mStride = YStride;

    if (UStride == 0)
    {
        mUstride = mStride >> 1;
    }

    if (VStride == 0)
    {
        mVstride = mStride >> 1;
    }

    UVSize = (mHeight >> 1) * mUstride;

    if (UVContingous)
    {
        if (gcvSURF_I420 == format)
        {
            mUDMAAddr = mDMAAddr + mHeight * mStride;
            mVDMAAddr = mUDMAAddr + UVSize;
        }
        else if (gcvSURF_YV12 == format)
        {
            mVDMAAddr = mDMAAddr + mHeight * mStride;
            mUDMAAddr = mVDMAAddr + UVSize;
        }
        else
        {
            goto ERROR_EXIT;
        }
    }
    else
    {
        status = gcoHAL_MapUserMemory(UVirtAddr, UPhysAddr, UVSize, &mUMapInfo, &mUDMAAddr);
        if (gcmNO_SUCCESS(status))
        {
            goto ERROR_EXIT;
        }

        if ((flag & GC2DSURF_PROPERTY_CACHABLE) && UVirtAddr)
        {
            CacheRangeFlush(UVirtAddr, UVSize, CACHE_SYNC_ALL);
        }

        status = gcoHAL_MapUserMemory(VVirtAddr, VPhysAddr, UVSize, &mVMapInfo, &mVDMAAddr);
        if (gcmNO_SUCCESS(status))
        {
            goto ERROR_EXIT;
        }

        if ((flag & GC2DSURF_PROPERTY_CACHABLE) && VVirtAddr)
        {
            CacheRangeFlush(VVirtAddr, UVSize, CACHE_SYNC_ALL);
        }
    }

    mUPhysicalAddr = UPhysAddr;
    mUVirtualAddr = UVirtAddr;
    mVPhysicalAddr = VPhysAddr;
    mVVirtualAddr = VVirtAddr;

    // init the member variables
    mAcc2D = pAcc2D;
    mFormat = format;
    mRotate = rotate;

    mFlag = flag;
    mGalSurf = gcvNULL;


    mType = GC2D_OBJ_SURF;

    return gcvSTATUS_OK;

ERROR_EXIT:

    if (mMapInfo)
    {
        gctSIZE_T mapSize = mHeight*YStride;

        if (UVContingous)
        {
            mapSize += mapSize >> 1;
        }

        gcmVERIFY_OK(gcoHAL_UnmapUserMemory(YVirtAddr, mapSize, mMapInfo, mDMAAddr));
    }

    if (mUMapInfo)
    {
        gcmVERIFY_OK(gcoHAL_UnmapUserMemory(UVirtAddr, UVSize, mUMapInfo, mUDMAAddr));
    }

    if (mVMapInfo)
    {
        gcmVERIFY_OK(gcoHAL_UnmapUserMemory(VVirtAddr, UVSize, mVMapInfo, mVDMAAddr));
    }

    // clear this object
    mType = GC2D_OBJ_UNKNOWN;
    mFlag = 0;
    mAcc2D = gcvNULL;

    mDMAAddr = INVALID_PHYSICAL_ADDRESS;
    mPhysicalAddr = INVALID_PHYSICAL_ADDRESS;
    mVirtualAddr = 0;

    mMapInfo = gcvNULL;

    mGalSurf = gcvNULL;

    mStride = 0;
    mWidth = 0;
    mHeight = 0;
    mSize = 0;
    mFormat = gcvSURF_UNKNOWN;
    mRotate = gcvSURF_0_DEGREE;


    mOwnerFlag = 0;

    mUDMAAddr = INVALID_PHYSICAL_ADDRESS;
    mUVirtualAddr = gcvNULL;
    mUPhysicalAddr = INVALID_PHYSICAL_ADDRESS;
    mUstride = 0;
    mUMapInfo = gcvNULL;

    mVDMAAddr = INVALID_PHYSICAL_ADDRESS;
    mVVirtualAddr = gcvNULL;
    mVPhysicalAddr = INVALID_PHYSICAL_ADDRESS;
    mVstride = 0;
    mVMapInfo = gcvNULL;

    return gcvSTATUS_INVALID_ARGUMENT;
}


gceSTATUS GC2DSurf::InitializeGALSurface(
    GC2D_Accelerator_PTR pAcc2D, gcoSURF galSurf, gceSURF_ROTATION rotate)
{
    gceSTATUS status;
    gctUINT32 width, height;
    gctINT stride;
    gceSURF_FORMAT format;

    // already initialized
    if (mType != GC2D_OBJ_UNKNOWN)
    {
        return gcvSTATUS_INVALID_OBJECT;
    }

    if (pAcc2D == gcvNULL)
    {
        return gcvSTATUS_INVALID_ARGUMENT;
    }

    status = gcoSURF_GetAlignedSize(galSurf,
                                        &width,
                                        &height,
                                        &stride);
    if (gcvSTATUS_OK != status)
    {
        GC2D_DebugTrace(GC2D_GCHAL, "[GC2DSurf]: Get aligned size of gal surface failed: %d\r\n", status);
        return gcvSTATUS_INVALID_ARGUMENT;
    }

    status = gcoSURF_GetFormat(galSurf, gcvNULL, &format);
    if (gcvSTATUS_OK != status)
    {
        GC2D_DebugTrace(GC2D_GCHAL, "[GC2DSurf]: Get format of gal surface failed: %d\r\n", status);
        return gcvSTATUS_INVALID_ARGUMENT;
    }

    // check format
    switch (format)
    {
    // list of the supported formats
    case    gcvSURF_X4R4G4B4:
    case    gcvSURF_A4R4G4B4:
    case    gcvSURF_X1R5G5B5:
    case    gcvSURF_A1R5G5B5:
    case    gcvSURF_R5G6B5  :
    case    gcvSURF_X8R8G8B8:
    case    gcvSURF_A8R8G8B8:
    case    gcvSURF_INDEX8  :
    case    gcvSURF_YUY2:
    case    gcvSURF_UYVY:
    case    gcvSURF_YV12:
    case    gcvSURF_I420:
        break;

    default:
        return gcvSTATUS_INVALID_ARGUMENT;
    }

    // check width , height & stride
    if (width >= MAX_WIDTH_HEIGHT || height >= MAX_WIDTH_HEIGHT
        || stride >= MAX_STRIDE)
    {
        return gcvSTATUS_INVALID_ARGUMENT;
    }

    // alignment check
    if (gcmNO_SUCCESS(AlignWidthHeight(&width, &height)))
    {
        return gcvSTATUS_INVALID_ARGUMENT;
    }

    gctUINT32 address[3];
    gctPOINTER memory[3];

    status = gcoSURF_Lock(galSurf, address, memory);
    if (gcvSTATUS_OK != status)
    {
        GC2D_DebugTrace(GC2D_GCHAL, "[GC2DSurf]: Lock gal surface failed: %d\r\n", status);
        return gcvSTATUS_INVALID_ARGUMENT;
    }

    switch (format)
    {
    // list of the supported formats
    case    gcvSURF_X4R4G4B4:
    case    gcvSURF_A4R4G4B4:
    case    gcvSURF_X1R5G5B5:
    case    gcvSURF_A1R5G5B5:
    case    gcvSURF_R5G6B5  :
    case    gcvSURF_X8R8G8B8:
    case    gcvSURF_A8R8G8B8:
    case    gcvSURF_INDEX8  :
    case    gcvSURF_YUY2:
    case    gcvSURF_UYVY:
        mDMAAddr = address[0];
        mVirtualAddr = memory[0];
        break;

    case    gcvSURF_YV12:
        mDMAAddr = address[0];
        mVirtualAddr = memory[0];
        mVDMAAddr = address[1];
        mVVirtualAddr = memory[1];
        mUDMAAddr = address[2];
        mUVirtualAddr = memory[2];
        break;

    case    gcvSURF_I420:
        mDMAAddr = address[0];
        mVirtualAddr = memory[0];
        mUDMAAddr = address[1];
        mUVirtualAddr = memory[1];
        mVDMAAddr = address[2];
        mVVirtualAddr = memory[2];
        break;

    default:
        return gcvSTATUS_INVALID_ARGUMENT;
    }

    // init the member variables
    mAcc2D = pAcc2D;
    mFormat = format;
    mRotate = rotate;
    mWidth = width;
    mHeight = height;
    mStride = stride;
    mSize = height * stride;
    mFlag = 0;
    mMapInfo = gcvNULL;
    mGalSurf = galSurf;

    mUstride = mStride >> 1;
    mUPhysicalAddr = 0;
    mUMapInfo = gcvNULL;

    mVstride = mStride >> 1;
    mVPhysicalAddr = 0;
    mVMapInfo = gcvNULL;

    mType = GC2D_OBJ_SURF;

    return gcvSTATUS_OK;
}

gctBOOL GC2DSurf::IsValid() const
{
#ifdef DEBUG
    if (!mAcc2D || !mAcc2D->GC2DIsValid())
    {
        return gcvFALSE;
    }

    if ((mFormat == gcvSURF_UNKNOWN) || (mAcc2D == gcvNULL))
    {
        return gcvFALSE;
    }

    if (mGalSurf && (mMapInfo || mLockUserMemory
        || mUMapInfo || mLockUserUMemory
        || mVMapInfo || mLockUserVMemory))
    {
        return gcvFALSE;
    }
#endif
    return (mType == GC2D_OBJ_SURF);
}

gceSTATUS GC2DSurf::Denitialize(gctBOOL sync)
{
    gcmASSERT(IsValid());

    if (!IsValid())
    {
        return gcvSTATUS_INVALID_OBJECT;
    }

    if (mGalSurf)
    {
        gcmVERIFY_OK(gcoSURF_Unlock(mGalSurf, mVirtualAddr));
    }
    else
    {
        gctBOOL UVContingous = gcvFALSE;
        gctUINT32 UVSize = 0;

        switch (mFormat)
        {
        case    gcvSURF_YV12:
        case    gcvSURF_I420:
            gcmASSERT((mUMapInfo == gcvNULL && mVMapInfo == gcvNULL)
                || (mUMapInfo != gcvNULL && mVMapInfo != gcvNULL));

            if ((mUMapInfo == gcvNULL) && (mVMapInfo == gcvNULL))
            {
                UVContingous = gcvTRUE;
            }

            UVSize = mHeight * mUstride;

        default:
            break;
        }

        if (mMapInfo)
        {
            gctSIZE_T mapSize = mHeight * mStride;

            if (UVContingous)
            {
                mapSize += mapSize >> 1;
            }

            gcmVERIFY_OK(gcoHAL_UnmapUserMemory(mVirtualAddr, mapSize, mMapInfo, mDMAAddr));
        }

        if (mUMapInfo)
        {
            gcmVERIFY_OK(gcoHAL_UnmapUserMemory(mUVirtualAddr, UVSize, mUMapInfo, mUDMAAddr));
        }

        if (mVMapInfo)
        {
            gcmVERIFY_OK(gcoHAL_UnmapUserMemory(mVVirtualAddr, UVSize, mVMapInfo, mVDMAAddr));
        }
    }
    // clear this object
    mType = GC2D_OBJ_UNKNOWN;
    mFlag = 0;
    mAcc2D = gcvNULL;

    mDMAAddr = INVALID_PHYSICAL_ADDRESS;
    mPhysicalAddr = INVALID_PHYSICAL_ADDRESS;
    mVirtualAddr = 0;

    mMapInfo = gcvNULL;
    mGalSurf = gcvNULL;

    mStride = 0;
    mWidth = 0;
    mHeight = 0;
    mSize = 0;
    mFormat = gcvSURF_UNKNOWN;
    mRotate = gcvSURF_0_DEGREE;


    mOwnerFlag = 0;

    mUDMAAddr = INVALID_PHYSICAL_ADDRESS;
    mUVirtualAddr = gcvNULL;
    mUPhysicalAddr = INVALID_PHYSICAL_ADDRESS;
    mUstride = 0;
    mUMapInfo = gcvNULL;

    mVDMAAddr = INVALID_PHYSICAL_ADDRESS;
    mVVirtualAddr = gcvNULL;
    mVPhysicalAddr = INVALID_PHYSICAL_ADDRESS;
    mVstride = 0;
    mVMapInfo = gcvNULL;

    return gcvSTATUS_OK;
}

//////////////////////////////////////////////////////////////////////////////
//              GC2D_SurfWrapper
//////////////////////////////////////////////////////////////////////////////
GC2D_SurfWrapper::GC2D_SurfWrapper(
        GC2D_Accelerator_PTR pAcc2D,
        int                  width,
        int                  height,
        EGPEFormat           format): mAllocFlagByUser(0), mAcc2D(gcvNULL)
{
    gceSURF_FORMAT    gcFormat = EGPEFormatToGALFormat(format);
    int                bpp = GetFormatSize(gcFormat);

    GC2D_ENTER_ARG("pAcc2D = 0x%08p, width = %d, height = %d, format = %d",
        pAcc2D, width, height, format);

    if (pAcc2D && pAcc2D->GC2DIsValid() && (gcFormat != gcvSURF_UNKNOWN) && (bpp != 0))
    {
        gctUINT32   AlignedWidth = width;
        gctUINT32   AlignedHeight = height;

        AlignWidthHeight(&AlignedWidth, &AlignedHeight);
        if (AlignedWidth == width && AlignedHeight == height)
        {
            void *bits = AllocNonCacheMem(width*height*bpp);
            if (bits)
            {
                // wrapped by GC2DSurf
                mAcc2D = pAcc2D;
                if (gcmIS_SUCCESS(mGC2DSurf.Initialize(pAcc2D,
                        GC2DSURF_PROPERTY_DEFAULT, 0, bits,
                        width, height, width*GetFormatSize(gcFormat), gcFormat, gcvSURF_0_DEGREE)))
                {
                    GPESurf::Init(width, height, mGC2DSurf.mVirtualAddr, mGC2DSurf.mStride, format);
                    mAllocFlagByUser = 1;
                    m_fOwnsBuffer = GC2D_OBJ_SURF;

                    GC2D_LEAVE();
                    return;
                }
                else
                {
                    FreeNonCacheMem(bits);
                    mAcc2D = gcvNULL;
                }
            }
        }
    }

    DDGPESurf::DDGPESurf(width, height, format);

    GC2D_LEAVE();
}

#ifndef DERIVED_FROM_GPE
GC2D_SurfWrapper::GC2D_SurfWrapper(
        GC2D_Accelerator_PTR  pAcc2D,
        int                   width,
        int                   height,
        int                   stride,
        EGPEFormat            format,
        EDDGPEPixelFormat     pixelFormat): mAllocFlagByUser(0), mAcc2D(gcvNULL)
{
    gceSURF_FORMAT    gcFormat = DDGPEFormatToGALFormat(pixelFormat);
    int                bpp = GetFormatSize(gcFormat);

    GC2D_ENTER_ARG("pAcc2D = 0x%08p, width = %d, height = %d, stride = %, format = %d, pixelFormat = %d",
        pAcc2D, width, height, stride, format, pixelFormat);

    if (pAcc2D && pAcc2D->GC2DIsValid() && (gcFormat != gcvSURF_UNKNOWN) && (bpp != 0))
    {
        gctUINT32   AlignedWidth = width;
        gctUINT32   AlignedHeight = height;

        AlignWidthHeight(&AlignedWidth, &AlignedHeight);
        if (AlignedWidth == width && AlignedHeight == height)
        {
            // wrapped by GC2DSurf
            void *bits = AllocNonCacheMem(width*height*bpp);
            if (bits)
            {
                mAcc2D = pAcc2D;
                if (gcmIS_SUCCESS(mGC2DSurf.Initialize(pAcc2D,
                        GC2DSURF_PROPERTY_DEFAULT, 0, bits,
                        width, height, width*GetFormatSize(gcFormat), gcFormat, gcvSURF_0_DEGREE)))
                {
                    Init(width, height, mGC2DSurf.mVirtualAddr, mGC2DSurf.mStride, format, pixelFormat);
                    mAllocFlagByUser = 1;
                    m_fOwnsBuffer = GC2D_OBJ_SURF;

                    GC2D_LEAVE();
                    return;
                }
                else
                {
                    FreeNonCacheMem(bits);
                    mAcc2D = gcvNULL;
                }
            }
        }
    }

    DDGPESurf::DDGPESurf(width,
                         height,
                         stride,
                         format,
                         pixelFormat);
    GC2D_LEAVE();
}
#endif

GC2D_SurfWrapper::GC2D_SurfWrapper(               // Create video-memory surface
                        GC2D_Accelerator_PTR  pAcc2D,
                        int                   width,
                        int                   height,
                        void *                pBits,            // virtual address of allocated bits
                        int                   stride,
                        EGPEFormat            format): mAllocFlagByUser(0), mAcc2D(gcvNULL)
{
    gceSURF_FORMAT    gcFormat = EGPEFormatToGALFormat(format);
    EDDGPEPixelFormat    pixelFormat = EGPEFormatToEDDGPEPixelFormat[format];

    GC2D_ENTER_ARG("pAcc2D = 0x%08p, width = %d, height = %d, pBits = 0x%08p, stride = %, format = %d",
        pAcc2D, width, height, pBits, stride, format);

    if (pAcc2D && pAcc2D->GC2DIsValid() && (gcFormat != gcvSURF_UNKNOWN))
    {
        gctUINT32   AlignedWidth = width;
        gctUINT32   AlignedHeight = height;

        AlignWidthHeight(&AlignedWidth, &AlignedHeight);
        if (AlignedWidth == width && AlignedHeight == height)
        {
            // wrapped by GC2DSurf
            mAcc2D = pAcc2D;
            if (gcmIS_SUCCESS(mGC2DSurf.Initialize(pAcc2D,
                        GC2DSURF_PROPERTY_DEFAULT, 0, pBits,
                        width, height, width*GetFormatSize(gcFormat), gcFormat, gcvSURF_0_DEGREE)))
            {
            #ifdef DERIVED_FROM_GPE
                    Init(width, height, mGC2DSurf.mVirtualAddr, mGC2DSurf.mStride, format);
            #else
                Init(width, height, mGC2DSurf.mVirtualAddr, mGC2DSurf.mStride, format, pixelFormat);
            #endif
                m_fOwnsBuffer = GC2D_OBJ_SURF;

                GC2D_LEAVE();
                return;
            }
            else
            {
                mAcc2D = gcvNULL;
            }
        }
    }

    DDGPESurf::DDGPESurf(width,
                         height,
                         pBits,
                         stride,
                         format);
    GC2D_LEAVE();
}


GC2D_SurfWrapper::~GC2D_SurfWrapper()
{
    GC2D_ENTER();

    if (m_fOwnsBuffer == GC2D_OBJ_SURF)
    {
        gcmASSERT(mGC2DSurf.IsValid());

        mGC2DSurf.Denitialize(gcvTRUE);
        if (mAllocFlagByUser)
        {
            FreeNonCacheMem(mGC2DSurf.mVirtualAddr);
            mAllocFlagByUser = 0;
        }

        mAcc2D = gcvNULL;
        m_fOwnsBuffer = 0;
    }

    GC2D_LEAVE();
}
