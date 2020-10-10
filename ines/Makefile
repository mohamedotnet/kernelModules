ifneq ($(KERNELRELEASE),)
	obj-m := inesbDriver.o
else
	KERN_DIR ?= /usr/src/linux-headers-4.15.0-54-generic/
	PWD := $(shell pwd)
default:
	$(MAKE) -C $(KERN_DIR) M=$(PWD) modules
endif
