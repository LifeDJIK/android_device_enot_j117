LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
LOCAL_SRC_FILES := setup_ext4_fs.c
LOCAL_MODULE := setup_ext4_fs
LOCAL_MODULE_TAGS := optional
LOCAL_SHARED_LIBRARIES += libcutils
include $(BUILD_EXECUTABLE)
