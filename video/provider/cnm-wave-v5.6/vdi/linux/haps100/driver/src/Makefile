obj-m = umr3usb.o
umr3usb-objs := \
  umr3usb_device.o \
  ../../../library/src/umr3_driver_bus.o \
  ../../../library/src/umr3_driver_fctx.o \
  ../../../library/src/umr3_driver_ioctl_linux.o \
  ../../../library/src/umr3_driver_log.o \
  ../../../library/src/umr3_driver_memory.o \
  ../../../library/src/umr3_driver_module_linux.o \
  ../../../library/src/umr3_driver_queue.o \
  ../../../library/src/umr3_driver_rw_usb.o \
  ../../../library/src/umr3_driver_rw.o \
  ../../../library/src/umr3_driver_strings.o \
  ../../../library/src/umr3_driver_tree.o	\
  ../../../library/src/umr3_driver_watchdog.o

ifdef DEBUG
	DEBFLAGS := -DUMR3_TRACE_LEVEL=TRACE_LEVEL_VERBOSE
else
	DEBFLAGS :=
endif #DEBUG

# default gcc
CC = /usr/bin/gcc

EXTRA_CFLAGS += $(DEBFLAGS) -DUMR3_USB
EXTRA_CFLAGS += -I${src} -I${src}/../include -I${src}/../../../library/include -I${src}/../../../common/include
EXTRA_CFLAGS += -Wall
KDIR  := /lib/modules/$(shell uname -r)/build
PWD   := $(shell pwd)


default:
	$(MAKE) -C $(KDIR) M=$(PWD)

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
