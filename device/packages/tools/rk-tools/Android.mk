LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := afptool
LOCAL_MODULE_PATH := $(HOST_OUT_EXECUTABLES)
LOCAL_SRC_FILES := afptool.c
LOCAL_MODULE_TAGS := optional
include $(BUILD_HOST_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_MODULE := img_maker
LOCAL_MODULE_PATH := $(HOST_OUT_EXECUTABLES)
LOCAL_SRC_FILES := img_maker.c
LOCAL_CFLAGS := -DUSE_OPENSSL
LOCAL_C_INCLUDES := external/openssl/include
LOCAL_SHARED_LIBRARIES := libssl
LOCAL_MODULE_TAGS := optional
include $(BUILD_HOST_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_MODULE := img_unpack
LOCAL_MODULE_PATH := $(HOST_OUT_EXECUTABLES)
LOCAL_SRC_FILES := img_unpack.c
LOCAL_CFLAGS := -DUSE_OPENSSL
LOCAL_C_INCLUDES := external/openssl/include
LOCAL_SHARED_LIBRARIES := libssl
LOCAL_MODULE_TAGS := optional
include $(BUILD_HOST_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_MODULE := rkcrc
LOCAL_MODULE_PATH := $(HOST_OUT_EXECUTABLES)
LOCAL_SRC_FILES := rkcrc.c
LOCAL_MODULE_TAGS := optional
include $(BUILD_HOST_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_MODULE := rkkernel
LOCAL_MODULE_PATH := $(HOST_OUT_EXECUTABLES)
LOCAL_SRC_FILES := rkkernel.c
LOCAL_MODULE_TAGS := optional
include $(BUILD_HOST_EXECUTABLE)
