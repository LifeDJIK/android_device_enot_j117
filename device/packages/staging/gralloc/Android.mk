##############################################################################
#
#    Copyright (c) 2005 - 2012 by Vivante Corp.  All rights reserved.
#
#    The material in this file is confidential and contains trade secrets
#    of Vivante Corporation. This is proprietary information owned by
#    Vivante Corporation. No part of this work may be disclosed,
#    reproduced, copied, transmitted, or used in any way for any purpose,
#    without the express written permission of Vivante Corporation.
#
##############################################################################

# --- Modified for Enot J117

# --- Config ---

LOCAL_PATH := $(call my-dir)

# --- Moved here
#include $(LOCAL_PATH)/../../Android.mk.def

# Select prebuilt binary types.
FIXED_ARCH_TYPE              ?= arm-android

# Set this variable to Kernel directory.
KERNEL_DIR                   ?= $(ANDROID_BUILD_TOP)/kernel/

# Cross compiler for building kernel module.
CROSS_COMPILE                ?= $(ANDROID_BUILD_TOP)/prebuilt/linux-x86/toolchain/arm-eabi-4.4.3/bin/arm-eabi-

# <ro.hardware> property, which comes from 'Hardware' field of
# 'cpuinfo' of the device.
RO_HARDWARE                  ?= rk29board

# Enable this to allocate video memory via coherent DMA.
NO_DMA_COHERENT              ?= 1

# Enable to start GPU clock in drver.
ENABLE_GPU_CLOCK_BY_DRIVER   ?= 0

# Enable platform driver model, available after linux 2.6.x.
USE_PLATFORM_DRIVER          ?= 1

# Build as vanilla Linux in Android environment with VDK backend.
USE_LINUX_MODE_FOR_ANDROID   ?= 0

# Force all video memory cached.
FORCE_ALL_VIDEO_MEMORY_CACHED ?= 0

# Enable caching for non paged memory.
NONPAGED_MEMORY_CACHEABLE    ?= 0

# Enable buffering for non paged memory
NONPAGED_MEMORY_BUFFERABLE   ?= 0

# Enable swap rectangle - EXPERIMENTAL
SUPPORT_SWAP_RECTANGLE       ?= 1

# Enable memory bank alignment
USE_BANK_ALIGNMENT           ?= 0
BANK_BIT_START               ?= 0
BANK_BIT_END                 ?= 0
BANK_CHANNEL_BIT             ?= 0

# Enable debug.
DEBUG                        ?= 0

# Build OpenCL
USE_OPENCL ?= 0

# Enable/disable deferred resolves for 3D apps - used for HWC - EXPERIMENTAL.
DEFER_RESOLVES               ?= 0

# CopyBlt Optimization - EXPERIMENTAL.
COPYBLT_OPTIMIZATION         ?= 1

# Use linear buffer for GPU apps. Make sure HWC has 2D composition.
ENABLE_LINEAR_BUFFER_FOR_GPU ?= 0

# Enable outer cache patch.
ENABLE_OUTER_CACHE_PATCH     ?= 1

# Enable/disable android unaligned linear surface composition address adjust
ENABLE_ANDROID_UNALIGNED_LINEAR_COMPOSITION_ADJUST    ?= 0

GPU_TYPE  := \
	$(firstword \
		$(filter XAQ2 GCCORE Unified,\
			$(notdir $(wildcard $(call my-dir)/arch/*)) \
		) \
	)

PROPERTY  := \
	$(firstword \
		$(strip $(RO_HARDWARE)) \
		$(TARGET_BOOTLOADER_BOARD_NAME) \
		$(TARGET_BOARD_PLATFORM)\
		default \
	)

AQROOT    := $(call my-dir)
AQARCH    := $(AQROOT)/arch/$(GPU_TYPE)

ARCH_TYPE := $(TARGET_ARCH)
TAG       := VIVANTE

CFLAGS        := -DLINUX

ifeq ($(USE_LINUX_MODE_FOR_ANDROID),1)
CFLAGS        += -DUSE_VDK=1
else
CFLAGS        += -DUSE_VDK=0
endif

ifeq ($(SUPPORT_SWAP_RECTANGLE),1)
CFLAGS        += -DgcdSUPPORT_SWAP_RECTANGLE=1
endif

ifeq ($(DEFER_RESOLVES),1)
CFLAGS        += -DgcdDEFER_RESOLVES=1
endif

ifeq ($(COPYBLT_OPTIMIZATION),1)
CFLAGS        += -DgcdCOPYBLT_OPTIMIZATION=1
endif

PLATFORM_SDK_VERSION ?= 12

ifeq ($(ENABLE_LINEAR_BUFFER_FOR_GPU),1)
CFLAGS        += -DgcdGPU_LINEAR_BUFFER_ENABLED=1
endif

ifeq ($(ENABLE_OUTER_CACHE_PATCH),1)
CFLAGS        += -DgcdENABLE_OUTER_CACHE_PATCH=1
endif

ifeq ($(USE_BANK_ALIGNMENT), 1)
    CFLAGS += -DgcdENABLE_BANK_ALIGNMENT=1
    ifneq ($(BANK_BIT_START), 0)
	        ifneq ($(BANK_BIT_END), 0)
	            CFLAGS += -DgcdBANK_BIT_START=$(BANK_BIT_START)
	            CFLAGS += -DgcdBANK_BIT_END=$(BANK_BIT_END)
	        endif
    endif

    ifneq ($(BANK_CHANNEL_BIT), 0)
        CFLAGS += -DgcdBANK_CHANNEL_BIT=$(BANK_CHANNEL_BIT)
    endif
endif

CFLAGS        += -DANDROID_SDK_VERSION=$(PLATFORM_SDK_VERSION)
CFLAGS        += -fno-strict-aliasing -fno-short-enums
CFLAGS        += -Wall -Wno-missing-field-initializers -Wno-unused-parameter

ifeq ($(DEBUG), 1)
CFLAGS        += -DDBG=1 -DDEBUG -D_DEBUG -O0 -g -DLOG_NDEBUG=0
endif

ifeq ($(ENABLE_ANDROID_UNALIGNED_LINEAR_COMPOSITION_ADJUST), 1)
CFLAGS        += -DgcdANDROID_UNALIGNED_LINEAR_COMPOSITION_ADJUST=1
else
CFLAGS        += -DgcdANDROID_UNALIGNED_LINEAR_COMPOSITION_ADJUST=0
endif

ifeq ($(FORCE_ALL_VIDEO_MEMORY_CACHED), 1)
CFLAGS        += -DgcdPAGED_MEMORY_CACHEABLE=1
else
CFLAGS        += -DgcdPAGED_MEMORY_CACHEABLE=0
endif

# --- Headers ---

include $(CLEAR_VARS)
LOCAL_COPY_HEADERS_TO :=
LOCAL_COPY_HEADERS := gralloc_priv.h
include $(BUILD_COPY_HEADERS)

# --- Build ---

#
# gralloc.<property>.so
#
ifneq ($(USE_LINUX_MODE_FOR_ANDROID), 1)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	gralloc.cpp \
	gc_gralloc_alloc.cpp \
	gc_gralloc_fb.cpp \
	gc_gralloc_map.cpp

# With front buffer rendering, gralloc always provides the same buffer  when
# GRALLOC_USAGE_HW_FB. Obviously there is no synchronization with the display.
# Can be used to test non-VSYNC-locked rendering.
LOCAL_CFLAGS := \
	$(CFLAGS) \
	-DLOG_TAG=\"v_gralloc\"

#~ 	-DDISABLE_FRONT_BUFFER

LOCAL_CFLAGS += \
	-DLOG_NDEBUG=1

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/include/hal/user \
	$(LOCAL_PATH)/include/hal/os/linux/user \
	$(LOCAL_PATH)/include/hal/inc

LOCAL_SHARED_LIBRARIES := \
	liblog \
	libGAL \
	libbinder \
	libutils

# --- We have no libosdm
#LOCAL_SHARED_LIBRARIES += libosdm
#LOCAL_C_INCLUDES += kernel/include/video \
#		    hardware/dspg/dmw96/libosdm/
#LOCAL_CFLAGS += -DUSE_OSDM

LOCAL_MODULE         := gralloc.$(PROPERTY)
LOCAL_MODULE_TAGS    := optional
LOCAL_MODULE_PATH    := $(TARGET_OUT_SHARED_LIBRARIES)/hw
LOCAL_PRELINK_MODULE := false
include $(BUILD_SHARED_LIBRARY)

endif
