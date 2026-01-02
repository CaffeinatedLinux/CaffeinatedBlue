ProCapture-objs :=	  \
	sources/audio-resample/resample.o			\
	sources/audio-resample/resample-core.o		\
	sources/supports/math.o						\
	sources/supports/sort.o                     \
	sources/supports/sha1.o						\
	sources/supports/heap.o						\
	sources/supports/utils.o					\
	sources/supports/image-buffer.o				\
	sources/supports/shared-image-buffer.o		\
	sources/supports/osd-bound-rect.o           \
	sources/supports/karray.o                   \
	sources/supports/color-matrix.o				\
	sources/supports/resource-pool.o			\
	sources/supports/timer-event-manager.o      \
	sources/supports/notify-event-manager.o     \
	sources/supports/sw-timer-event-manager.o   \
	sources/supports/audio-miscs.o				\
	sources/front-end/front-end-utils.o  		\
	sources/front-end/front-end.o				\
	sources/drivers/irq-control.o             	\
	sources/drivers/gpio-master.o             	\
	sources/drivers/i2c-master.o              	\
	sources/drivers/device-info.o             	\
	sources/drivers/pcie-dma-controller.o     	\
	sources/drivers/pcie-dma-desc-chain.o     	\
	sources/drivers/spi-master.o              	\
	sources/drivers/qspiflash-micron.o       	\
	sources/drivers/audio-capture.o           	\
	sources/drivers/video-capture.o           	\
	sources/drivers/timestamp.o               	\
	sources/drivers/pcie-ring-dma-controller.o 	\
	sources/drivers/pcie-memory-writer.o      	\
	sources/drivers/pi7c9x2gxxx.o				\
	sources/drivers/pwm-fan-control.o			\
	sources/drivers/sdianc.o					\
	sources/picopng/picopng.o					\
	sources/dma/mw-dma-mem.o					\
	sources/dma/mw-dma-user.o					\
	sources/dma/mw-dma-kernel.o					\
	sources/dma/mw-dma-phy.o					\
	sources/ospi/ospi-linux.o					\
	sources/ospi/linux-file.o  					\
	sources/avstream/parameter-manager.o        \
	sources/avstream/xi-driver.o		        \
	sources/avstream/v4l2.o				        \
	sources/avstream/v4l2-sg-buf.o				\
	sources/avstream/mw-capture-impl.o			\
	sources/avstream/mw-stream.o				\
	sources/avstream/mw-event-dev.o				\
	sources/avstream/mw-event-ioctl.o			\
	sources/avstream/alsa.o				        \
	sources/avstream/params_parser.o			\
	sources/avstream/capture.o                  \
	sources/avstream/mw-uio-drv.o               \
	sources/avstream/mw-uio-ioctl.o                \
	sources/avstream/mw-netlink.o

obj-m = ProCapture.o

ccflags-y += -I$(src) -I$(src)/public -I$(src)/sources
# strip debug symbols
ldflags-y += -S

LATEST_KERNEL := $(shell rpm -qa kernel-devel | sort -V | tail -n1 | sed 's/kernel-devel-//')

ifneq ($(KERNELRELEASE),)
KERNELDIR ?= /lib/modules/$(KERNELRELEASE)/build
else
KERNELDIR ?= /lib/modules/$(LATEST_KERNEL)/build
endif

PWD := $(shell pwd)

all:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules

install:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules_install

clean:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) clean

