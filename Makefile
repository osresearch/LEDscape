#########
#
# The top level targets link in the two .o files for now.
#
TARGETS += teensy-udp-rx
TARGETS += rgb-test
TARGETS += udp-rx

LEDSCAPE_OBJS = ledscape.o pru.o
LEDSCAPE_LIB := libledscape.a

all: $(TARGETS) ws281x.bin


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

CFLAGS += \
	-std=c99 \
	-W \
	-Wall \
	-Wp,-MMD,$(dir $@).$(notdir $@).d \
	-Wp,-MT,$@ \
	-I. \
	-O2 \
	-mtune=cortex-a8 \
	-march=armv7-a \

LDFLAGS += \

LDLIBS += \
	-lpthread \

COMPILE.o = $(CROSS_COMPILE)gcc $(CFLAGS) -c -o $@ $< 
COMPILE.a = $(CROSS_COMPILE)gcc -c -o $@ $< 
COMPILE.link = $(CROSS_COMPILE)gcc $(LDFLAGS) -o $@ $^ $(LDLIBS)


#####
#
# The TI "app_loader" is the userspace library for talking to
# the PRU and mapping memory between it and the ARM.
#
APP_LOADER_DIR ?= ./am335x/app_loader
APP_LOADER_LIB := $(APP_LOADER_DIR)/lib/libprussdrv.a
CFLAGS += -I$(APP_LOADER_DIR)/include
LDLIBS += $(APP_LOADER_LIB)

#####
#
# The TI PRU assembler looks like it has macros and includes,
# but it really doesn't.  So instead we use cpp to pre-process the
# file and then strip out all of the directives that it adds.
# PASM also doesn't handle multiple statements per line, so we
# insert hard newline characters for every ; in the file.
#
PASM_DIR ?= ./am335x/pasm
PASM := $(PASM_DIR)/pasm

%.bin: %.p $(PASM)
	$(CPP) - < $< | perl -p -e 's/^#.*//; s/;/\n/g' > $<.i
	$(PASM) -V3 -b $<.i $(basename $@)
	$(RM) $<.i

%.o: %.c
	$(COMPILE.o)

$(foreach O,$(TARGETS),$(eval $O: $O.o $(LEDSCAPE_OBJS) $(APP_LOADER_LIB)))

$(TARGETS):
	$(COMPILE.link)


.PHONY: clean

clean:
	rm -rf \
		*.o \
		*.i \
		.*.o.d \
		*~ \
		$(INCDIR_APP_LOADER)/*~ \
		$(TARGET) \
		ws281x.bin \


###########
# 
# The correct way to reserve the GPIO pins on the BBB is with the
# capemgr and a Device Tree file.  But it doesn't work.
#
SLOT_FILE=/sys/devices/bone_capemgr.8/slots
dts: LEDscape.dts
	@SLOT="`grep LEDSCAPE $(SLOT_FILE) | cut -d: -f1`"; \
	if [ ! -z "$$SLOT" ]; then \
		echo "Removing slot $$SLOT"; \
		echo -$$SLOT > $(SLOT_FILE); \
	fi
	dtc -O dtb -o /lib/firmware/BB-LEDSCAPE-00A0.dtbo -b 0 -@ LEDscape.dts
	echo BB-LEDSCAPE > $(SLOT_FILE)


###########
# 
# PRU Libraries and PRU assembler are build from their own trees.
# 
$(APP_LOADER_LIB):
	$(MAKE) -C $(APP_LOADER_DIR)/interface

$(PASM):
	$(MAKE) -C $(PASM_DIR)

# Include all of the generated dependency files
-include .*.o.d
