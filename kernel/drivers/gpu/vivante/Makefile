# Vivante GPU

ifeq ($(CONFIG_VIVANTE_GPU_DEBUG),y)
EXTRA_CFLAGS += \
	-DDBG=1 \
	-DDEBUG \
	-D_DEBUG
else
EXTRA_CFLAGS += \
	-DDBG=0
endif

ifeq ($(CONFIG_VIVANTE_GPU_PROFILER),y)
EXTRA_CFLAGS += \
	-DVIVANTE_PROFILER=1
else
EXTRA_CFLAGS += \
	-DVIVANTE_PROFILER=0
endif

ifeq ($(CONFIG_VIVANTE_GPU_NEWSIGNAL),y)
EXTRA_CFLAGS += \
	-DUSE_NEW_LINUX_SIGNAL=1
else
EXTRA_CFLAGS += \
	-DUSE_NEW_LINUX_SIGNAL=0
endif

ifeq ($(CONFIG_VIVANTE_GPU_POWER_MANAGEMENT),y)
EXTRA_CFLAGS += \
	-DgcdPOWER_MANAGEMENT=1
else
EXTRA_CFLAGS += \
	-DgcdPOWER_MANAGEMENT=0
endif

ifeq ($(CONFIG_VIVANTE_GPU_ANDROID),y)
EXTRA_CFLAGS += \
	-DANDROID=1
endif

ifeq ($(CONFIG_VIVANTE_GPU_NO_DMA_COHERENT),y)
EXTRA_CFLAGS += \
	-DNO_DMA_COHERENT
endif

#~ bank_bit_start = 0
#~ bank_bit_end = 0
#~ bank_channel_bit = 0
#~ 
#~ ifeq ($(CONFIG_GPU_VIVANTE_USE_BANK_ALIGNMENT), y)
#~     ccflags-y  += -DgcdENABLE_BANK_ALIGNMENT=1
#~     ifneq ($(bank_bit_start), 0)
#~ 	        ifneq ($(bank_bit_end), 0)
#~ 	            ccflags-y  += -DgcdBANK_BIT_START=$(bank_bit_start)
#~ 	            ccflags-y  += -DgcdBANK_BIT_END=$(bank_bit_end)
#~ 	        endif
#~     endif
#~ 
#~     ifneq ($(bank_channel_bit), 0)
#~         ccflags-y  += -DgcdBANK_CHANNEL_BIT=$(bank_channel_bit)
#~     endif
#~ endif

ifeq ($(CONFIG_SMP),y)
EXTRA_CFLAGS += \
	-DgcdSMP=1
else
EXTRA_CFLAGS += \
	-DgcdSMP=0
endif

EXTRA_CFLAGS += \
	-DNO_USER_DIRECT_ACCESS_FROM_KERNEL=1 \
	-DgcdPAGED_MEMORY_CACHEABLE=0 \
	-DgcdNONPAGED_MEMORY_CACHEABLE=0 \
	-DgcdNONPAGED_MEMORY_BUFFERABLE=0 \
	-DgcdCACHE_FUNCTION_UNIMPLEMENTED=0 \
	-DgcdENABLE_VG=0 \
	-DgcdSUPPORT_SWAP_RECTANGLE=0 \
	-DgcdENABLE_OUTER_CACHE_PATCH=1 \
	-DgcdUSE_OPENCL=0 \
	-DUSE_PLATFORM_DRIVER=1 \
	-DENUM_WORKAROUND=0

#~ 	-DgcdNONPAGED_MEMORY_BUFFERABLE=1 \

EXTRA_CFLAGS += \
	-I$(obj)/hal/user \
	-I$(obj)/hal/kernel \
	-I$(obj)/hal/kernel/inc \
	-I$(obj)/hal/os/linux/kernel \
	-I$(obj)/arch/XAQ2/hal/kernel

obj-$(CONFIG_VIVANTE_GPU) += galcore.o

galcore-objs += \
	hal/kernel/gc_hal_kernel.o \
	hal/kernel/gc_hal_kernel_command.o \
	hal/kernel/gc_hal_kernel_db.o \
	hal/kernel/gc_hal_kernel_debug.o \
	hal/kernel/gc_hal_kernel_event.o \
	hal/kernel/gc_hal_kernel_heap.o \
	hal/kernel/gc_hal_kernel_mmu.o \
	hal/kernel/gc_hal_kernel_video_memory.o \
	hal/os/linux/kernel/gc_hal_kernel_device.o \
	hal/os/linux/kernel/gc_hal_kernel_driver.o \
	hal/os/linux/kernel/gc_hal_kernel_linux.o \
	hal/os/linux/kernel/gc_hal_kernel_math.o \
	hal/os/linux/kernel/gc_hal_kernel_os.o \
	arch/XAQ2/hal/kernel/gc_hal_kernel_context.o \
	arch/XAQ2/hal/kernel/gc_hal_kernel_hardware.o
