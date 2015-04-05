# LifeDJIK: copyright will be added later

# LifeDJIK: general
LOCAL_PATH := $(call my-dir)
# LifeDJIK: ---

# LifeDJIK: data
LOADER := device/enot/j117/device/prebuilt/RK29xxLoader(L)_V2.31.bin
PACKAGEFILE := device/enot/j117/device/data/package-file.txt
PARAMETER := device/enot/j117/device/data/parameter.txt
RECOVERSCRIPT := device/enot/j117/device/data/recover-script.txt
UPDATESCRIPT := device/enot/j117/device/data/update-script.txt
# LifeDJIK: ---

# LifeDJIK: tools
AFPTOOL := $(HOST_OUT_EXECUTABLES)/afptool
IMG_MAKER := $(HOST_OUT_EXECUTABLES)/img_maker
IMG_UNPACK := $(HOST_OUT_EXECUTABLES)/img_unpack
MKPARAMETER := $(LOCAL_PATH)/mkparameter.py
RKCRC := $(HOST_OUT_EXECUTABLES)/rkcrc
RKKERNEL := $(HOST_OUT_EXECUTABLES)/rkkernel
UPGRADE_TOOL := $(HOST_OUT_EXECUTABLES)/upgrade_tool
# LifeDJIK: ---

# LifeDJIK: loader
INSTALLED_LOADERBINARY_TARGET := $(PRODUCT_OUT)/$(notdir $(LOADER))
$(INSTALLED_LOADERBINARY_TARGET): $(ACP) $(LOADER)
	$(call pretty,"Target loader binary: $@")
	@mkdir -p $(dir $@)
	$(hide) $(ACP) -fp "$(LOADER)" "$@"
	$(call pretty,"Made loader binary: $@")

loaderbinary: $(INSTALLED_LOADERBINARY_TARGET)
# LifeDJIK: ---

# LifeDJIK: parameter
INSTALLED_PARAMETERSOURCE_TARGET := $(PRODUCT_OUT)/parameter.txt
$(eval $(call copy-one-file,$(PARAMETER),$(INSTALLED_PARAMETERSOURCE_TARGET)))

parametersource: $(INSTALLED_PARAMETERSOURCE_TARGET)

INSTALLED_PARAMETERBINARY_TARGET := $(PRODUCT_OUT)/parameter.bin
$(INSTALLED_PARAMETERBINARY_TARGET): $(RKCRC) $(INSTALLED_PARAMETERSOURCE_TARGET)
	$(call pretty,"Target parameter binary: $@")
	$(hide) $(RKCRC) -p $(INSTALLED_PARAMETERSOURCE_TARGET) $@
	$(call pretty,"Made parameter binary: $@")

parameterbinary: $(INSTALLED_PARAMETERBINARY_TARGET)

INSTALLED_PARAMETERIMAGE_TARGET := $(PRODUCT_OUT)/parameter.img
$(INSTALLED_PARAMETERIMAGE_TARGET): $(MKPARAMETER) $(INSTALLED_PARAMETERBINARY_TARGET)
	$(call pretty,"Target parameter image: $@")
	$(hide) $(shell $(MKPARAMETER) $(INSTALLED_PARAMETERBINARY_TARGET) $@)
	$(hide) $(call assert-max-image-size,$@,$(BOARD_PARAMETERIMAGE_PARTITION_SIZE),raw)
	$(call pretty,"Made parameter image: $@")

parameterimage: $(INSTALLED_PARAMETERIMAGE_TARGET)
# LifeDJIK: ---

# LifeDJIK: boot image
INSTALLED_BOOTIMAGE_TARGET := $(PRODUCT_OUT)/boot.img
$(INSTALLED_BOOTIMAGE_TARGET): $(RKKERNEL) $(INTERNAL_BOOTIMAGE_FILES)
	$(call pretty,"Target boot image: $@")
	$(hide) $(RKKERNEL) -pack $(PRODUCT_OUT)/ramdisk.img $@
	$(hide) $(call assert-max-image-size,$@,$(BOARD_BOOTIMAGE_PARTITION_SIZE),raw)
	$(call pretty,"Made boot image: $@")
# LifeDJIK: ---

# LifeDJIK: recovery image
INSTALLED_RECOVERYIMAGE_TARGET := $(PRODUCT_OUT)/recovery.img
$(INSTALLED_RECOVERYIMAGE_TARGET): $(RKKERNEL) $(recovery_ramdisk)
	$(call pretty,"Target recovery image: $@")
	$(hide) $(RKKERNEL) -pack $(PRODUCT_OUT)/ramdisk-recovery.img $@
	$(hide) $(call assert-max-image-size,$@,$(BOARD_RECOVERYIMAGE_PARTITION_SIZE),raw)
	$(call pretty,"Made recovery image: $@")
# LifeDJIK: ---

# LifeDJIK: kernel image
INSTALLED_KERNELIMAGE_TARGET := $(PRODUCT_OUT)/kernel.img
$(INSTALLED_KERNELIMAGE_TARGET): $(RKKERNEL) $(INSTALLED_KERNEL_TARGET)
	$(call pretty,"Target kernel image: $@")
	$(hide) $(RKKERNEL) -pack $(INSTALLED_KERNEL_TARGET) $@
	$(hide) $(call assert-max-image-size,$@,$(BOARD_KERNELIMAGE_PARTITION_SIZE),raw)
	$(call pretty,"Made kernel image: $@")

kernelimage: $(INSTALLED_KERNELIMAGE_TARGET)
# LifeDJIK: ---

# LifeDJIK: system image
INSTALLED_SYSTEMIMAGE_TARGET := $(PRODUCT_OUT)/system.img
# LifeDJIK: ---

# LifeDJIK: update image
UPDATE_BOOTIMAGE_TARGET := $(PRODUCT_OUT)/update/Image/boot.img
$(eval $(call copy-one-file,$(INSTALLED_BOOTIMAGE_TARGET),$(UPDATE_BOOTIMAGE_TARGET)))

UPDATE_KERNELIMAGE_TARGET := $(PRODUCT_OUT)/update/Image/kernel.img
$(eval $(call copy-one-file,$(INSTALLED_KERNELIMAGE_TARGET),$(UPDATE_KERNELIMAGE_TARGET)))

UPDATE_RECOVERYIMAGE_TARGET := $(PRODUCT_OUT)/update/Image/recovery.img
$(eval $(call copy-one-file,$(INSTALLED_RECOVERYIMAGE_TARGET),$(UPDATE_RECOVERYIMAGE_TARGET)))

UPDATE_SYSTEMIMAGE_TARGET := $(PRODUCT_OUT)/update/Image/system.img
$(eval $(call copy-one-file,$(INSTALLED_SYSTEMIMAGE_TARGET),$(UPDATE_SYSTEMIMAGE_TARGET)))

UPDATE_IMAGES := $(UPDATE_BOOTIMAGE_TARGET) $(UPDATE_KERNELIMAGE_TARGET) $(UPDATE_RECOVERYIMAGE_TARGET) $(UPDATE_SYSTEMIMAGE_TARGET)

UPDATE_LOADERBINARY_TARGET := $(PRODUCT_OUT)/update/$(notdir $(INSTALLED_LOADERBINARY_TARGET))
$(UPDATE_LOADERBINARY_TARGET): $(ACP) $(INSTALLED_LOADERBINARY_TARGET)
	$(call pretty,"Update loader binary: $@")
	@mkdir -p $(dir $@)
	$(hide) $(ACP) -fp "$(INSTALLED_LOADERBINARY_TARGET)" "$@"
	$(call pretty,"Made loader binary: $@")

UPDATE_BINARIES := $(UPDATE_LOADERBINARY_TARGET)

UPDATE_PARAMETER_TARGET := $(PRODUCT_OUT)/update/parameter
$(eval $(call copy-one-file,$(INSTALLED_PARAMETERSOURCE_TARGET),$(UPDATE_PARAMETER_TARGET)))

UPDATE_PACKAGEFILE_TARGET := $(PRODUCT_OUT)/update/package-file
$(eval $(call copy-one-file,$(PACKAGEFILE),$(UPDATE_PACKAGEFILE_TARGET)))

UPDATE_RECOVERSCRIPT_TARGET := $(PRODUCT_OUT)/update/recover-script
$(eval $(call copy-one-file,$(RECOVERSCRIPT),$(UPDATE_RECOVERSCRIPT_TARGET)))

UPDATE_UPDATESCRIPT_TARGET := $(PRODUCT_OUT)/update/update-script
$(eval $(call copy-one-file,$(UPDATESCRIPT),$(UPDATE_UPDATESCRIPT_TARGET)))

UPDATE_DATA := $(UPDATE_PARAMETER_TARGET) $(UPDATE_PACKAGEFILE_TARGET) $(UPDATE_RECOVERSCRIPT_TARGET) $(UPDATE_UPDATESCRIPT_TARGET)

UPDATE_OBJECTS := $(UPDATE_IMAGES) $(UPDATE_BINARIES) $(UPDATE_DATA)

UPDATE_DIRECTORY := $(PRODUCT_OUT)/update
$(UPDATE_DIRECTORY): $(UPDATE_OBJECTS)

INSTALLED_UPDATEBINARY_TARGET := $(PRODUCT_OUT)/update.bin
$(INSTALLED_UPDATEBINARY_TARGET): $(AFPTOOL) $(UPDATE_DIRECTORY)
	$(call pretty,"Target update binary: $@")
	$(hide) $(AFPTOOL) -pack $(UPDATE_DIRECTORY) $@
	$(call pretty,"Made update binary: $@")

updatebinary: $(INSTALLED_UPDATEBINARY_TARGET)

INSTALLED_UPDATEIMAGE_TARGET := $(PRODUCT_OUT)/update.img
$(INSTALLED_UPDATEIMAGE_TARGET): $(IMG_MAKER) $(UPDATE_LOADERBINARY_TARGET) $(INSTALLED_UPDATEBINARY_TARGET)
	$(call pretty,"Target update image: $@")
	$(hide) $(IMG_MAKER) -rk29 "$(UPDATE_LOADERBINARY_TARGET)" 1 2 42 $(INSTALLED_UPDATEBINARY_TARGET) $@
	$(call pretty,"Made update image: $@")

updateimage: $(INSTALLED_UPDATEIMAGE_TARGET)
# LifeDJIK: ---
