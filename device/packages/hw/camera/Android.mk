LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_SRC_FILES := prebuilt/camera.rk29board.so
LOCAL_MODULE := $(basename $(notdir $(LOCAL_SRC_FILES)))
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_SUFFIX := $(suffix $(LOCAL_SRC_FILES))
OVERRIDE_BUILT_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)
LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)/hw
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE_TAGS := optional
LOCAL_PREBUILT_LIBS := \
    prebuilt/libjpeghwenc.so \
    prebuilt/libjpeghwdec.so \
    prebuilt/librkswscale.so
include $(BUILD_MULTI_PREBUILT)
