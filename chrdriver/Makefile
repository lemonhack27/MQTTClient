KERNELDIR := /home/lemonhack/Study/linuxstudy/kernel/linux-5.4.31
CURRENT_PATH := $(shell pwd)

obj-m := dtsplatform_led.o

build: kernel_modules

kernel_modules:
	$(MAKE) -C $(KERNELDIR) M=$(CURRENT_PATH) modules

clean:
	$(MAKE) -C $(KERNELDIR) M=$(CURRENT_PATH) clean