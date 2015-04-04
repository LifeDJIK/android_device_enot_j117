LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_COPY_HEADERS_TO := ion
LOCAL_COPY_HEADERS := \
    include/ionalloc.h \
    include/ionalloc_cpp.h
include $(BUILD_COPY_HEADERS)

include $(CLEAR_VARS)
LOCAL_MODULE_TAGS := optional
LOCAL_PREBUILT_LIBS := \
    prebuilt/libion.so
include $(BUILD_MULTI_PREBUILT)
