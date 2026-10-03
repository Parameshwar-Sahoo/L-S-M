obj-m += libsys_driver.o

DEFAULT_KDIR := /lib/modules/$(shell uname -r)/build
GENERIC_KDIR := $(wildcard /usr/src/linux-headers-*-generic)

ifeq ($(wildcard $(DEFAULT_KDIR)),)
    KDIR ?= $(firstword $(GENERIC_KDIR))
else
    KDIR ?= $(DEFAULT_KDIR)
endif

PWD := $(shell pwd)
EXTRA_CFLAGS += -I$(PWD)/include

all:
	@if [ -z "$(KDIR)" ] || [ ! -d "$(KDIR)" ]; then \
		echo "NOTE: Linux kernel build directory not found at $(DEFAULT_KDIR)."; \
		echo "Skipping kernel module binary build (SmartLMS C++ user-space will automatically use SimulatedHardware fallback)."; \
		echo "To compile the kernel driver, please install kernel headers: sudo apt-get install linux-headers-$$(uname -r)"; \
		exit 0; \
	else \
		$(MAKE) -C $(KDIR) M=$(PWD) modules; \
	fi

clean:
	@if [ -n "$(KDIR)" ] && [ -d "$(KDIR)" ]; then \
		$(MAKE) -C $(KDIR) M=$(PWD) clean; \
	else \
		rm -f *.o *.ko *.mod *.mod.c *.order *.symvers .*.cmd; \
		rm -rf .tmp_versions; \
	fi

install:
	@if [ -f libsys_driver.ko ]; then \
		sudo insmod libsys_driver.ko; \
		sudo chmod 666 /dev/libsys_driver; \
		echo "Driver loaded to /dev/libsys_driver"; \
	else \
		echo "ERROR: libsys_driver.ko not built. Run make first."; \
	fi

uninstall:
	@sudo rmmod libsys_driver 2>/dev/null || true
	@echo "Driver removed."

.PHONY: all clean install uninstall
