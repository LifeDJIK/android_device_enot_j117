LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
ALL_PREBUILT += $(INSTALLED_KERNEL_TARGET)

-include vendor/enot/j117/AndroidBoardVendor.mk
