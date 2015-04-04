# LifeDJIK: copyright will be added later

# LifeDJIK: general settings and tools
LOCAL_PATH := $(call my-dir)
RKCRC := $(HOST_OUT_EXECUTABLES)/rkcrc
MKPARAMETER := $(LOCAL_PATH)/mkparameter.py
# LifeDJIK: ---

# LifeDJIK: parameter
PARAMETER := device/enot/j117/device/data/parameter.txt

INSTALLED_PARAMETER_TARGET := $(PRODUCT_OUT)/parameter.bin
$(INSTALLED_PARAMETER_TARGET): $(RKCRC) $(PARAMETER)
	$(call pretty,"Target parameter: $@")
	$(hide) $(RKCRC) -p $(PARAMETER) $@
	$(call pretty,"Made parameter: $@")

INSTALLED_PARAMETERIMAGE_TARGET := $(PRODUCT_OUT)/parameter.img
$(INSTALLED_PARAMETERIMAGE_TARGET): $(RKCRC) $(MKPARAMETER) $(INSTALLED_PARAMETER_TARGET)
	$(call pretty,"Target parameter image: $@")
	$(hide) $(shell $(MKPARAMETER) $(INSTALLED_PARAMETER_TARGET) $@)
	$(hide) $(call assert-max-image-size,$@,$(BOARD_PARAMETERIMAGE_PARTITION_SIZE),raw)
	$(call pretty,"Made parameter image: $@")

parameterimage: $(INSTALLED_PARAMETERIMAGE_TARGET)
# LifeDJIK: ---

# LifeDJIK: boot image
INSTALLED_BOOTIMAGE_TARGET := $(PRODUCT_OUT)/boot.img
$(INSTALLED_BOOTIMAGE_TARGET): $(RKCRC) $(INTERNAL_BOOTIMAGE_FILES)
	$(call pretty,"Target boot image: $@")
	$(hide) $(RKCRC) -k $(PRODUCT_OUT)/ramdisk.img $@
	$(hide) $(call assert-max-image-size,$@,$(BOARD_BOOTIMAGE_PARTITION_SIZE),raw)
	$(call pretty,"Made boot image: $@")
# LifeDJIK: ---

# LifeDJIK: recovery image
INSTALLED_RECOVERYIMAGE_TARGET := $(PRODUCT_OUT)/recovery.img
$(INSTALLED_RECOVERYIMAGE_TARGET): $(RKCRC) $(recovery_ramdisk)
	$(call pretty,"Target recovery image: $@")
	$(hide) $(RKCRC) -k $(PRODUCT_OUT)/ramdisk-recovery.img $@
	$(hide) $(call assert-max-image-size,$@,$(BOARD_RECOVERYIMAGE_PARTITION_SIZE),raw)
	$(call pretty,"Made recovery image: $@")
# LifeDJIK: ---

# LifeDJIK: kernel image
INSTALLED_KERNELIMAGE_TARGET := $(PRODUCT_OUT)/kernel.img
$(INSTALLED_KERNELIMAGE_TARGET): $(RKCRC) $(INSTALLED_KERNEL_TARGET)
	$(call pretty,"Target kernel image: $@")
	$(hide) $(RKCRC) -k $(INSTALLED_KERNEL_TARGET) $@
	$(hide) $(call assert-max-image-size,$@,$(BOARD_KERNELIMAGE_PARTITION_SIZE),raw)
	$(call pretty,"Made kernel image: $@")

kernelimage: $(INSTALLED_KERNELIMAGE_TARGET)
# LifeDJIK: ---
