ifneq ($(KERNELRELEASE),)
MODULE_NAME := hid_gamdias_hermes_p2
obj-m := $(MODULE_NAME).o
else
MODULE_NAME := hid_gamdias_hermes_p2
KERNEL_VERSION ?= $(shell uname -r)
KDIR ?= /lib/modules/$(KERNEL_VERSION)/build
MODULE_DIR := /lib/modules/$(KERNEL_VERSION)/extra
MODPROBE_CONF := /etc/modprobe.d/$(MODULE_NAME).conf
HID_DRIVER_DIR := /sys/bus/hid/drivers/$(MODULE_NAME)
UDEV_RULE := /etc/udev/rules.d/99-hermes-p2.rules
UDEV_HELPER := /usr/local/sbin/hermes-p2-bind

.PHONY: all modules clean install uninstall
.ONESHELL:
SHELL := /bin/sh

all: modules

modules:
	$(MAKE) -C $(KDIR) M=$(CURDIR) modules

clean:
	$(MAKE) -C $(KDIR) M=$(CURDIR) clean

install:
	set -e
	if [ "$(KERNEL_VERSION)" != "$$(uname -r)" ]; then
		echo "install is only supported for the running kernel" >&2
		exit 1
	fi
	test -f $(MODULE_NAME).ko
	built_for=$$(modinfo -F vermagic ./$(MODULE_NAME).ko)
	built_for=$${built_for%% *}
	if [ "$$built_for" != "$(KERNEL_VERSION)" ]; then
		echo "module was built for $$built_for, not $(KERNEL_VERSION); run make clean && make" >&2
		exit 1
	fi
	install -D -m 0644 $(MODULE_NAME).ko $(MODULE_DIR)/$(MODULE_NAME).ko
	printf '%s\n' 'softdep hid_generic pre: $(MODULE_NAME)' > $(MODPROBE_CONF)
	install -D -m 0644 99-hermes-p2.rules $(UDEV_RULE)
	install -D -m 0755 hermes-p2-bind $(UDEV_HELPER)
	depmod -a $(KERNEL_VERSION)
	udevadm control --reload-rules
	modprobe $(MODULE_NAME)
	for path in /sys/bus/hid/devices/0003:1B80:B51D.*; do
		[ -e "$$path" ] || continue
		device=$${path##*/}
		if [ -L "$$path/driver" ]; then
			driver=$$(readlink -f "$$path/driver")
			if [ "$$driver" = /sys/bus/hid/drivers/$(MODULE_NAME) ]; then
				continue
			fi
			if [ "$$driver" != /sys/bus/hid/drivers/hid-generic ]; then
				continue
			fi
			printf '%s' "$$device" > /sys/bus/hid/drivers/hid-generic/unbind
		fi
		printf '%s' "$$device" > $(HID_DRIVER_DIR)/bind
	done

uninstall:
	set -e
	if [ "$(KERNEL_VERSION)" != "$$(uname -r)" ]; then
		echo "uninstall is only supported for the running kernel" >&2
		exit 1
	fi
	rm -f $(MODPROBE_CONF) $(UDEV_RULE)
	udevadm control --reload-rules
	udevadm settle
	rm -f $(MODULE_DIR)/$(MODULE_NAME).ko
	depmod -a $(KERNEL_VERSION)
	modprobe hid_generic
	if [ -d /sys/module/$(MODULE_NAME) ]; then
		rmmod $(MODULE_NAME)
	fi
	for path in /sys/bus/hid/devices/0003:1B80:B51D.*; do
		[ -e "$$path" ] || continue
		device=$${path##*/}
		if [ ! -L "$$path/driver" ]; then
			printf '%s' "$$device" > /sys/bus/hid/drivers/hid-generic/bind
		fi
	done
	rm -f $(UDEV_HELPER)

endif
