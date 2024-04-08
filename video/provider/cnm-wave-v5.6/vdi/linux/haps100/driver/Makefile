# get KERNEL version
KERNEL = $(shell uname -r)

# set module name
MODULE = umr3usb
FNAME  = umr3_driver_module_linux.c 

# set pathes
SRC    = ./src
INIT   = /etc/init.d
UDEV   = /etc/udev/rules.d

# get driver version
DRIVER_MJ  = $(shell grep '\#define UMR3_DRIVER_VERSION_MJ' ../../library/include/umr3_driver_version.h |  awk '{ print $$3}')
DRIVER_MN  = $(shell grep '\#define UMR3_DRIVER_VERSION_MN' ../../library/include/umr3_driver_version.h |  awk '{ print $$3}')
DRIVER_SUB = $(shell grep '\#define UMR3_DRIVER_VERSION_SUB' ../../library/include/umr3_driver_version.h |  awk '{ print $$3}')

# the 'depmod' utility to be used
DEPMOD = $(shell which depmod 2> /dev/null)
# fallback if not found in the PATH:
ifeq (x$(DEPMOD),x)
DEPMOD = /sbin/depmod
endif
# the 'chkconfig' utility to be used
CHKCONFIG = $(shell which chkconfig 2> /dev/null)
# fallback if not found in the PATH:
ifeq (x$(CHKCONFIG),x)
CHKCONFIG = /sbin/chkconfig
endif
# the 'update-rc.d' utility to be used
UPDATERC = $(shell which update-rc.d 2> /dev/null)
# fallback if not found in the PATH:
ifeq (x$(UPDATERC),x)
UPDATERC = /usr/sbin/update-rc.d
endif

# path to the module
MODULE_PATH = /lib/modules/$(KERNEL)/misc/$(MODULE).ko

.PHONY: all stop uninstall install start deinstall clean help version

all:
		@echo "----------------------------------------------------"
		@echo "- Compiling UMRBus 3.0 USB driver module $(DRIVER_MJ).$(DRIVER_MN).$(DRIVER_SUB)"
		@echo "----------------------------------------------------"
		chmod u+rwx $(SRC)
		chmod u+rwx ../../library/src
		$(MAKE) -C $(SRC)
		
stop:
		@(test -f $(INIT)/$(MODULE) && . $(INIT)/$(MODULE) stop) || echo "UMRBus 3.0 USB driver not installed !" 
		
uninstall:	stop
		@echo "----------------------------------------------------"
		@echo "- Removing UMRBus 3.0 USB driver module $(DRIVER_MJ).$(DRIVER_MN).$(DRIVER_SUB)"
		@echo "----------------------------------------------------"
ifneq ("$(wildcard $(MODULE_PATH))","")
		rm -f /lib/modules/$(KERNEL)/misc/$(MODULE).ko
endif
ifneq (,$(wildcard $(CHKCONFIG)))
		$(CHKCONFIG) --del $(MODULE) || echo "UMRBus 3.0 USB driver is not installed !" 
else
		$(UPDATERC) -f $(MODULE) remove || echo "UMRBus 3.0 USB driver is not installed !" 
endif
ifneq ("$(wildcard $(INIT)/$(MODULE))","")
		rm -f $(INIT)/$(MODULE)
endif
ifneq ("$(wildcard $(UDEV)/51-$(MODULE).rules)","")
		rm -f $(UDEV)/51-$(MODULE).rules
endif

start:
		@(test -f $(INIT)/$(MODULE) && . $(INIT)/$(MODULE) start) || echo "UMRBus 3.0 USB driver not installed !" 

install:	uninstall
		@echo "----------------------------------------------------"
		@echo "- Installing UMRBus 3.0 USB driver module $(DRIVER_MJ).$(DRIVER_MN).$(DRIVER_SUB)"
		@echo "----------------------------------------------------"
		test -d /lib/modules/$(KERNEL)/misc || mkdir -p /lib/modules/$(KERNEL)/misc
		cp $(SRC)/$(MODULE).ko /lib/modules/$(KERNEL)/misc
		$(DEPMOD)
		cp $(MODULE) $(INIT)
ifneq (,$(wildcard $(CHKCONFIG)))       
		$(CHKCONFIG) --add $(MODULE)
else
		$(UPDATERC) $(MODULE) defaults
endif
		@echo "KERNEL==\"$(MODULE)*\", GROUP=\"uucp\", MODE=\"0666\"" > $(UDEV)/51-$(MODULE).rules
		chmod 766 $(INIT)/$(MODULE)
		$(INIT)/$(MODULE) start 

load:	all
	chmod 777 $(SRC)/$(MODULE).ko
	sudo /sbin/insmod $(SRC)/$(MODULE).ko
	sudo chmod 666 /dev/$(MODULE)0

unload: 
	sudo /sbin/modprobe -r $(MODULE)

deinstall:	uninstall

clean:
		chmod u+rwx $(SRC)
		$(MAKE) -C $(SRC) clean
		rm -f $(SRC)/Module.markers
		rm -f $(SRC)/Module.symvers

help:		
		@echo ""
		@echo "UMRBus 3.0 USB Kernel Mode Driver installer $(DRIVER_MJ).$(DRIVER_MN).$(DRIVER_SUB)"
		@echo ""
		@echo "usage  : make {target}"
		@echo ""
		@echo "targets:"
		@echo "    [all]     - Compile only"
		@echo "    install   - Compile and install (for automatic start during boot)"
		@echo "    start     - Compile and install and start driver immediately"
		@echo "    stop      - Stop driver"
		@echo "    clean     - Delete all generated files during compile, including kernel module [$(MODULE).ko]"
		@echo "    uninstall - Uninstall driver and remove start during boot"
		@echo ""
		@cat README
		
version:	
		@echo "UMRBus 3.0 USB Kernel Mode Driver installer $(DRIVER_MJ).$(DRIVER_MN).$(DRIVER_SUB)"
