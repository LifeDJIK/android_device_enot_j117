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


LOCAL_PATH := $(call my-dir)
include $(LOCAL_PATH)/../../../../../Android.mk.def

ifdef FIXED_ARCH_TYPE

#
# libGLSLC
#
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
    $(FIXED_ARCH_TYPE)/libGLSLC.so

LOCAL_MODULE         := libGLSLC
LOCAL_MODULE_SUFFIX  := .so
LOCAL_MODULE_TAGS    := optional
LOCAL_MODULE_CLASS   := SHARED_LIBRARIES
include $(BUILD_PREBUILT)

endif

