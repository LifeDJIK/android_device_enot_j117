cp arch/XAQ2/hal/kernel/gc* ../../../kernel/drivers/gpu/vivante/v4/arch/XAQ2/hal/kernel/
cp hal/kernel/gc* ../../../kernel/drivers/gpu/vivante/v4/hal/kernel/
cp hal/kernel/inc/gc* ../../../kernel/drivers/gpu/vivante/v4/hal/kernel/inc/
#cp hal/inc/gc* ../../../kernel/drivers/gpu/vivante/v4/hal/kernel/inc/
cp hal/os/linux/kernel/gc* ../../../kernel/drivers/gpu/vivante/v4/hal/os/linux/kernel/
#../vendor/dspg/common/scripts/buildkernel.sh ../device/dspg/common/config/kernel.config

#adb remount
#adb push /opt/arm/boot.img /sdcard/
#adb shell sync
#adb shell sync
#adb shell sync
