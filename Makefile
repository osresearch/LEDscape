###########
#
# Top level build file for LEDscape 
#

all: $(LEDSCAPE_LIB) c-examples

###########
#
# Determine if we are using a cross compiler here, so the results can trickle down
#
ifeq ($(shell uname -m),armv7l)
# We are on the BeagleBone Black itself;
# do not cross compile.
export CROSS_COMPILE:=
else
# We are not on the BeagleBone and might be cross compiling.
# If the environment does not set CROSS_COMPILE, set our
# own.  Install a cross compiler with something like:
#
# sudo apt-get install gcc-arm-linux-gnueabi
#
export CROSS_COMPILE?=arm-linux-gnueabi-
endif


###########
# 
# The correct way to reserve the GPIO pins on the BBB is with the
# capemgr and a Device Tree file.  But it doesn't work.
#
# SLOT_FILE=/sys/devices/bone_capemgr.8/slots
# dts: LEDscape.dts
#	@SLOT="`grep LEDSCAPE $(SLOT_FILE) | cut -d: -f1`"; \
#	if [ ! -z "$$SLOT" ]; then \
#		echo "Removing slot $$SLOT"; \
#		echo -$$SLOT > $(SLOT_FILE); \
#	fi
#	dtc -O dtb -o /lib/firmware/BB-LEDSCAPE-00A0.dtbo -b 0 -@ LEDscape.dts
#	echo BB-LEDSCAPE > $(SLOT_FILE)


#####
#
# The TI "app_loader" is the userspace library for talking to
# the PRU and mapping memory between it and the ARM.
#
APP_LOADER_DIR ?= ./am335x/app_loader
APP_LOADER_LIB := $(APP_LOADER_DIR)/lib/libprussdrv.a

$(APP_LOADER_LIB):
	$(MAKE) -C $(APP_LOADER_DIR)/interface


###########
# 
# PRU Libraries and PRU assembler are built from their own trees.
# 
PASM_DIR ?= ./am335x/pasm
PASM := $(PASM_DIR)/pasm

$(PASM):
	$(MAKE) -C $(PASM_DIR)


###########
# 
# The LEDscape library gets built as an archive
#
LEDSCAPE_DIR ?= ./lib
LEDSCAPE_LIB := $(LEDSCAPE_DIR)/libledscape.a

.PHONY: $(LEDSCAPE_LIB)
$(LEDSCAPE_LIB): $(APP_LOADER_LIB) $(PASM)
	$(MAKE) -C $(LEDSCAPE_DIR)


#########
#
# C language examples are
#
C_EXAMPLES_DIR ?= ./c-examples

.PHONY: c-examples
c-examples: $(LEDSCAPE_LIB)
	$(MAKE) -C $(C_EXAMPLES_DIR)



.PHONY: clean
clean:
	rm -rf \
		*.o \
		*.i \
		.*.o.d \
		*~
	$(MAKE) -C $(APP_LOADER_DIR)/interface clean
	$(MAKE) -C $(PASM_DIR) clean
	$(MAKE) -C $(LEDSCAPE_DIR) clean
	$(MAKE) -C $(C_EXAMPLES_DIR) clean
