#########
#

LEDSCAPE_OBJS = ledscape.o pru.o bitslice.o util.o
LEDSCAPE_LIB := libledscape.a


all: $(LEDSCAPE_LIB) ws281x.bin matrix.bin


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
	-D_BSD_SOURCE \
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
COMPILE.ar = $(CROSS_COMPILE)ar




#####
#
# The TI "app_loader" is the userspace library for talking to
# the PRU and mapping memory between it and the ARM.
#
APP_LOADER_DIR ?= ../am335x/app_loader
CFLAGS += -I$(APP_LOADER_DIR)/include

#####
#
# The TI PRU assembler looks like it has macros and includes,
# but it really doesn't.  So instead we use cpp to pre-process the
# file and then strip out all of the directives that it adds.
# PASM also doesn't handle multiple statements per line, so we
# insert hard newline characters for every ; in the file.
#
PASM_DIR ?= ../am335x/pasm
PASM := $(PASM_DIR)/pasm

%.bin: %.p $(PASM)
	$(CPP) - < $< | perl -p -e 's/^#.*//; s/;/\n/g; s/BYTE\((\d+)\)/t\1/g' > $<.i
	$(PASM) -V3 -b $<.i $(basename $@)
	$(RM) $<.i

#####
#
# Build the LEDscape library as a static lib
libledscape.a: $(LEDSCAPE_OBJS)
	$(COMPILE.ar) rcs $@ $^

# Compile the .c files to objects
%.o: %.c
	$(COMPILE.o) 

.PHONY: clean

clean:
	rm -rf \
		*.o \
		*.i \
		.*.o.d \
		*~ \
		ws281x.bin \
		matrix.bin \

