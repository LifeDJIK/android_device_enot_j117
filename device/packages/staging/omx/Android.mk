LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE_TAGS := optional
LOCAL_PREBUILT_LIBS := \
    prebuilt/libOMX_Core.so \
    prebuilt/libomxvpu.so \
    prebuilt/libomxvpu_enc.so \
    prebuilt/librk_demux.so \
    prebuilt/librk_on2.so \
    prebuilt/libstagefrighthw.so
include $(BUILD_MULTI_PREBUILT)
