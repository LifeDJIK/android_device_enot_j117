# LifeDJIK: copyright will be added later

# LifeDJIK: overlays
DEVICE_PACKAGE_OVERLAYS += device/enot/j117/device/overlay
# LifeDJIK: ---

# LifeDJIK: tools, parameter and kernel image
PRODUCT_PACKAGES += rkcrc
CUSTOM_MODULES += $(PRODUCT_OUT)/parameter.img
CUSTOM_MODULES += $(PRODUCT_OUT)/kernel.img
# LifeDJIK: ---

# LifeDJIK: product characteristics
PRODUCT_BUILD_PROP_OVERRIDES += BUILD_UTC_DATE=0
PRODUCT_NAME := full_j117
PRODUCT_DEVICE := j117
PRODUCT_CHARACTERISTICS := tablet,sdcard
# LifeDJIK: ---

# LifeDJIK: USB
PRODUCT_DEFAULT_PROPERTY_OVERRIDES += persist.sys.usb.config=mtp
PRODUCT_PACKAGES += com.android.future.usb.accessory
PRODUCT_COPY_FILES += frameworks/native/data/etc/android.hardware.usb.accessory.xml:system/etc/permissions/android.hardware.usb.accessory.xml
PRODUCT_COPY_FILES += frameworks/native/data/etc/android.hardware.usb.host.xml:system/etc/permissions/android.hardware.usb.host.xml
# LifeDJIK: ---

# LifeDJIK: GPU & VPU & OMX & camera & wlan & sensors
PRODUCT_PACKAGES += \
    libion \
    libvpu \
    libyuvtorgb \
    libOMX_Core \
    libomxvpu \
    libomxvpu_enc \
    librk_demux \
    librk_on2 \
    libstagefrighthw \
    camera.rk29board \
    libjpeghwdec \
    libjpeghwenc \
    librkswscale \
    overlay.rk29board \
    sensors.rk29board \
    libI420colorconvert

PRODUCT_PACKAGES += \
    gralloc.rk29board \
    libGAL \
    copybit.rk29board \
    libEGL_VIVANTE \
    libGLESv1_CM_VIVANTE \
    libGLESv2_VIVANTE \
    libGLSLC
# LifeDJIK: ---

# LifeDJIK: values for debugging
ADDITIONAL_DEFAULT_PROPERTIES += ro.allow.mock.location=0
ADDITIONAL_DEFAULT_PROPERTIES += ro.adb.secure=0
ADDITIONAL_DEFAULT_PROPERTIES += ro.debuggable=1
PRODUCT_PROPERTY_OVERRIDES += persist.sys.strictmode.visual=false
PRODUCT_PROPERTY_OVERRIDES += persist.sys.strictmode.disable=true
PRODUCT_PROPERTY_OVERRIDES += persist.service.adb.enable=1                                                    
PRODUCT_PROPERTY_OVERRIDES += persist.service.debuggable=1
#PRODUCT_PROPERTY_OVERRIDES += hwui.disable_vsync=true
#~ PRODUCT_PROPERTY_OVERRIDES += debug.sf.hw=1
#~ PRODUCT_PROPERTY_OVERRIDES += hwui.render_dirty_regions=false
#~ PRODUCT_PROPERTY_OVERRIDES += opengl.vivante.texture=1
#~ PRODUCT_PROPERTY_OVERRIDES += sys.hwc.compose_policy=6
#~ PRODUCT_PROPERTY_OVERRIDES += video.accelerate.hw=1
#~ PRODUCT_PROPERTY_OVERRIDES += windowsmgr.max_events_per_sec=300
#~ PRODUCT_PROPERTY_OVERRIDES += pm.sleep_mode=1
#~ PRODUCT_PROPERTY_OVERRIDES += persist.sys.ui.hw=true
# LifeDJIK: ---

# LifeDJIK: settings for low ram
PRODUCT_PROPERTY_OVERRIDES += ro.config.low_ram=true
PRODUCT_PROPERTY_OVERRIDES += dalvik.vm.jit.codecachesize=0
# LifeDJIK: ---

# LifeDJIK: build setup_ext4_fs
PRODUCT_PACKAGES += make_ext4fs setup_ext4_fs
# LifeDJIK: ---

# LifeDJIK: screen density
PRODUCT_PROPERTY_OVERRIDES += ro.sf.lcd_density=120
# LifeDJIK: ---

# LifeDJIK: force OpenGL ES version
PRODUCT_PROPERTY_OVERRIDES += ro.opengles.version=131072
# LifeDJIK: ---

# LifeDJIK: locale
PRODUCT_PROPERTY_OVERRIDES += persist.sys.timezone=Europe/Athens
PRODUCT_PROPERTY_OVERRIDES += ro.product.locale.language=uk
PRODUCT_PROPERTY_OVERRIDES += ro.product.locale.region=UA
# LifeDJIK: ---

# LifeDJIK: disable setup wizard
PRODUCT_PROPERTY_OVERRIDES += ro.setupwizard.enterprise_mode=0
PRODUCT_PROPERTY_OVERRIDES += ro.setupwizard.mode=DISABLED
# LifeDJIK: ---

# LifeDJIK: WiFi
PRODUCT_PACKAGES += dhcpcd.conf
PRODUCT_PROPERTY_OVERRIDES += wifi.interface=wlan0
PRODUCT_PROPERTY_OVERRIDES += wifi.supplicant_scan_interval=180
PRODUCT_COPY_FILES += frameworks/native/data/etc/android.hardware.wifi.xml:system/etc/permissions/android.hardware.wifi.xml
PRODUCT_COPY_FILES += frameworks/native/data/etc/android.hardware.wifi.direct.xml:system/etc/permissions/android.hardware.wifi.direct.xml
# LifeDJIK: ---

# LifeDJIK: touchscreen
PRODUCT_COPY_FILES += frameworks/native/data/etc/android.hardware.touchscreen.multitouch.xml:system/etc/permissions/android.hardware.touchscreen.multitouch.xml
#PRODUCT_COPY_FILES += frameworks/native/data/etc/android.hardware.touchscreen.multitouch.distinct.xml:system/etc/permissions/android.hardware.touchscreen.multitouch.distinct.xml
# LifeDJIK: ---

# LifeDJIK: location
PRODUCT_COPY_FILES += frameworks/native/data/etc/android.hardware.location.xml:system/etc/permissions/android.hardware.location.xml
#PRODUCT_COPY_FILES += frameworks/native/data/etc/android.hardware.location.gps.xml:system/etc/permissions/android.hardware.location.gps.xml
# LifeDJIK: ---

# LifeDJIK: only WiFi present (3G support for modems will be added later)
PRODUCT_PROPERTY_OVERRIDES += ro.carrier=wifi-only
#PRODUCT_COPY_FILES += frameworks/native/data/etc/android.hardware.telephony.gsm.xml:system/etc/permissions/android.hardware.telephony.gsm.xml
#PRODUCT_COPY_FILES += frameworks/native/data/etc/android.hardware.telephony.cdma.xml:system/etc/permissions/android.hardware.telephony.cdma.xml
# LifeDJIK: ---

# LifeDJIK: camera
PRODUCT_COPY_FILES += frameworks/native/data/etc/android.hardware.camera.xml:system/etc/permissions/android.hardware.camera.xml
PRODUCT_COPY_FILES += frameworks/native/data/etc/android.hardware.camera.front.xml:system/etc/permissions/android.hardware.camera.front.xml
PRODUCT_COPY_FILES += frameworks/native/data/etc/android.hardware.camera.autofocus.xml:system/etc/permissions/android.hardware.camera.autofocus.xml
PRODUCT_COPY_FILES += frameworks/native/data/etc/android.hardware.camera.flash-autofocus.xml:system/etc/permissions/android.hardware.camera.flash-autofocus.xml
# LifeDJIK: ---

# LifeDJIK: Google Market/Play fix (hm, not working?)
PRODUCT_COPY_FILES += frameworks/native/data/etc/android.hardware.bluetooth.xml:system/etc/permissions/android.hardware.bluetooth.xml
# LifeDJIK: ---

# LifeDJIK: audio
PRODUCT_PACKAGES += audio.primary.rk29board
PRODUCT_PACKAGES += libaudioutils
PRODUCT_PACKAGES += libtinyalsa
PRODUCT_PACKAGES += tinymix
PRODUCT_PACKAGES += tinyplay
PRODUCT_PACKAGES += tinycap
# LifeDJIK: ---

# LifeDJIK: lights
PRODUCT_PACKAGES += lights.rk29board
# LifeDJIK: ---

# LifeDJIK: power
PRODUCT_PACKAGES += power.rk29board
# LifeDJIK: ---

# LifeDJIK: copy device files
PRODUCT_COPY_FILES += \
    $(call find-copy-subdir-files,*,device/enot/j117/device/prebuilt/root,root) \
    $(call find-copy-subdir-files,*,device/enot/j117/device/prebuilt/system,system)
# LifeDJIK: ---

# LifeDJIK: inherited configuration
$(call inherit-product, frameworks/native/build/phone-hdpi-512-dalvik-heap.mk)
$(call inherit-product, build/target/product/generic_no_telephony.mk)
$(call inherit-product, build/target/product/languages_full.mk)
#$(call inherit-product, device/common/gps/gps_eu_supl.mk)
# LifeDJIK: ---
