#
# Vivante GC800
#
obj-$(CONFIG_VIVANTE_GPU) += galcore.o

galcore-objs := \
	      hal/os/linux/kernel/gc_hal_kernel_linux.o		\
	      hal/os/linux/kernel/gc_hal_kernel_debug.o		\
	      hal/os/linux/kernel/gc_hal_kernel_device.o	\
	      hal/os/linux/kernel/gc_hal_kernel_os.o		\
	      hal/os/linux/kernel/gc_hal_kernel_driver.o	\
	      hal/kernel/gc_hal_kernel_mmu.o			\
	      hal/kernel/gc_hal_kernel_heap.o			\
	      hal/kernel/gc_hal_kernel_video_memory.o		\
	      hal/kernel/gc_hal_kernel_command.o		\
	      hal/kernel/gc_hal_kernel_event.o			\
	      hal/kernel/gc_hal_kernel.o			\
	      arch/XAQ2/hal/kernel/gc_hal_kernel_hardware.o

ifeq ($(CONFIG_VIVANTE_GPU_ANDROID),y)
EXTRA_CFLAGS += -DANDROID=1
else
EXTRA_CFLAGS += -DANDROID=0
endif

ifeq ($(CONFIG_VIVANTE_GPU_DEBUG),y)
EXTRA_CFLAGS += -DDBG=1
else
EXTRA_CFLAGS += -DDBG=0
endif

ifeq ($(CONFIG_VIVANTE_GPU_PROFILER),y)
EXTRA_CFLAGS += -DVIVANTE_PROFILER=1
else
EXTRA_CFLAGS += -DVIVANTE_PROFILER=0
endif

ifeq ($(CONFIG_VIVANTE_GPU_NEWSIGNAL),y)
EXTRA_CFLAGS += -DUSE_NEW_LINUX_SIGNAL=1
else
EXTRA_CFLAGS += -DUSE_NEW_LINUX_SIGNAL=0
endif

#
# Do a kernel flush_cache_all instead of user flush_cache_range
#
EXTRA_CFLAGS += -DFLUSH_CACHE_ALL_IN_KERNEL=1

#
# Make mmap() be cacheable and implement gckOS_CacheFlush()
#
EXTRA_CFLAGS += -DFIXED_MMAP_AS_CACHEABLE=0

EXTRA_CFLAGS += \
	-DLINUX \
	-DDRIVER \
	-DENUM_WORKAROUND=0 \
	-DNO_DMA_COHERENT \
	-DENABLE_ARM_L2_CACHE=0 \
	-DgcdNO_POWER_MANAGEMENT=0 \
	-DUSE_PLATFORM_DRIVER=1 \
	-DENABLE_GPU_CLOCK_BY_DRIVER=0 \
	-DNO_USER_DIRECT_ACCESS_FROM_KERNEL=0 \
	-DgcdkREPORT_VIDMEM_USAGE=0 \
	-I drivers/gpu/vivante/hal/inc \
	-I drivers/gpu/vivante/hal/inc \
	-I drivers/gpu/vivante/hal/kernel \
	-I drivers/gpu/vivante/arch/XAQ2/hal/kernel \
	-I drivers/gpu/vivante/hal/user \
	-DgcdENABLE_BANK_ALIGNMENT=0 \
	-Wframe-larger-than=1032 \
	-Werror \
	-DgcdNULL_DRIVER_TEST=0
