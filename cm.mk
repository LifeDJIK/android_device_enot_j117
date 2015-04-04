# Inherit some common CM stuff.
$(call inherit-product, vendor/cm/config/common_full_tablet_wifionly.mk)

# Inherit device configuration
$(call inherit-product, device/enot/j117/device.mk)

# Release name
PRODUCT_RELEASE_NAME := rk29board

## Device identifier. This must come after all inclusions
PRODUCT_DEVICE := j117
PRODUCT_NAME := cm_j117
PRODUCT_BRAND := Android
PRODUCT_MODEL := J117
PRODUCT_MANUFACTURER := Enot

TARGET_SCREEN_WIDTH := 800
TARGET_SCREEN_HEIGHT := 480
