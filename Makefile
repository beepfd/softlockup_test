# Makefile for softlockup_test kernel module
# WARNING: This module is for testing purposes only!

# If KERNELRELEASE is defined, we've been invoked from the kernel build system
ifneq ($(KERNELRELEASE),)
	obj-m := softlockup_test.o
else
	# Kernel build directory - automatically detect current running kernel  
	KDIR := /lib/modules/$(shell uname -r)/build
	PWD := $(shell pwd)

# Default target
all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules
	@echo ""
	@echo "⚠️  WARNING: This module is DANGEROUS and should only be used in test environments!"
	@echo "   Do NOT load this module on production systems!"

# Clean target
clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
	rm -f *.o *.ko *.mod.c *.mod.o .*.cmd *.symvers *.order
	rm -rf .tmp_versions/

# Install target (not recommended for this dangerous module)
install: all
	@echo "⚠️  WARNING: Installing this dangerous module system-wide is NOT recommended!"
	@read -p "Are you sure you want to install this dangerous module? [y/N]: " confirm && \
	if [ "$$confirm" = "y" ] || [ "$$confirm" = "Y" ]; then \
		$(MAKE) -C $(KDIR) M=$(PWD) modules_install; \
		depmod -a; \
	else \
		echo "Installation cancelled."; \
	fi

# Load module with warning
load: all
	@echo "⚠️  DANGER: Loading softlockup_test module!"
	@echo "   This will likely cause system instability or crash!"
	@read -p "Continue loading the dangerous module? [y/N]: " confirm && \
	if [ "$$confirm" = "y" ] || [ "$$confirm" = "Y" ]; then \
		sudo insmod softlockup_test.ko; \
		echo "Module loaded. Monitor system closely!"; \
	else \
		echo "Module loading cancelled."; \
	fi

# Unload module
unload:
	@echo "Attempting to unload softlockup_test module..."
	@if lsmod | grep -q softlockup_test; then \
		sudo rmmod softlockup_test; \
		echo "Module unloaded successfully."; \
	else \
		echo "Module is not currently loaded."; \
	fi

# Force unload (last resort)
force-unload:
	@echo "⚠️  Force unloading module (may cause system instability)..."
	sudo rmmod -f softlockup_test 2>/dev/null || echo "Force unload failed or module not loaded"

# Help target
help:
	@echo "Available targets for softlockup_test module:"
	@echo "  all          - Build the kernel module"
	@echo "  clean        - Clean build files"
	@echo "  install      - Install module system-wide (NOT RECOMMENDED)"
	@echo "  load         - Load the module with safety warning"
	@echo "  unload       - Unload the module if possible"
	@echo "  force-unload - Force unload module (use as last resort)"
	@echo "  help         - Show this help message"
	@echo ""
	@echo "⚠️  SAFETY WARNING:"
	@echo "   This module is designed to cause soft lockups and system instability."
	@echo "   Only use in isolated test environments with proper backups!"

.PHONY: all clean install load unload force-unload help

endif