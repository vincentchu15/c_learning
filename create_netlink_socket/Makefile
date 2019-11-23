PWD := $(shell pwd)
KERNEL_VERSION := $(shell uname -r)
KERNEL_DIR = /usr/src/linux-headers-$(KERNEL_VERSION)/

MODULE_NAME = netlink_kernel
obj-m := $(MODULE_NAME).o

all:
	make -C $(KERNEL_DIR) M=$(PWD) modules
clean:
	make -C $(KERNEL_DIR) M=$(PWD) clean
