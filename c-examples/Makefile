#########
#
# The top level targets link in the two .o files for now.
#
TARGETS += matrix-test
TARGETS += fire
TARGETS += matrix-udp-rx
TARGETS += opc-rx
#TARGETS += udp-rx
#TARGETS += rgb-test

LEDSCAPE_LIB_DIR = ../lib
LEDSCAPE_LIB := $(LEDSCAPE_LIB_DIR)/libledscape.a

all: $(TARGETS) 


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


#####
#
# The TI "app_loader" is the userspace library for talking to
# the PRU and mapping memory between it and the ARM.
#
APP_LOADER_DIR ?= ../am335x/app_loader
APP_LOADER_LIB := $(APP_LOADER_DIR)/lib/libprussdrv.a
CFLAGS += -I$(APP_LOADER_DIR)/include -I$(LEDSCAPE_LIB_DIR)
LDLIBS += $(APP_LOADER_LIB)


%.o: %.c
	$(COMPILE.o)

$(foreach O,$(TARGETS),$(eval $O: $O.o $(LEDSCAPE_OBJS) $(APP_LOADER_LIB) $(LEDSCAPE_LIB)))

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
		$(TARGETS) \


# Include all of the generated dependency files
-include .*.o.d
