LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_COPY_HEADERS_TO :=
LOCAL_COPY_HEADERS := \
    include/yuvtorgb.h
include $(BUILD_COPY_HEADERS)

include $(CLEAR_VARS)
LOCAL_MODULE_TAGS := optional
LOCAL_PREBUILT_LIBS := \
    prebuilt/libyuvtorgb.so
include $(BUILD_MULTI_PREBUILT)
