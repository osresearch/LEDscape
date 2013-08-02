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
#export CROSS_COMPILE
endif

LIBDIR_APP_LOADER?=./am335x/app_loader/lib
INCDIR_APP_LOADER?=./am335x/app_loader/include

CFLAGS += \
	-std=c99 \
	-Wall \
	-I$(INCDIR_APP_LOADER) \
	-D__DEBUG \
	-O2 \
	-mtune=cortex-a8 \
	-march=armv7-a \

LDFLAGS += \
	-L$(LIBDIR_APP_LOADER) \
	-lprussdrv \
	-lpthread \

PASM := ./am335x/pasm/pasm
TARGET := rgb-test

_DEPS = 
DEPS = $(patsubst %,$(INCDIR_APP_LOADER)/%,$(_DEPS))

OBJS = ledscape.o pru.o

%.o: %.c $(DEPS)
	$(CROSS_COMPILE)gcc $(CFLAGS) -c -o $@ $< 

all: rgb-test udp-rx ws281x.bin
rgb-test: rgb-test.o $(OBJS)
	$(CROSS_COMPILE)gcc $(CFLAGS) -o $@ $^ $(LDFLAGS)

udp-rx: udp-rx.o $(OBJS)
	$(CROSS_COMPILE)gcc $(CFLAGS) -o $@ $^ $(LDFLAGS)

ws281x.bin: ws281x.p ws281x.hp
	$(CPP) - < $< | grep -v '^#' | sed 's/;/\n/g' > $<.i
	$(PASM) -V3 -b $<.i


.PHONY: clean

clean:
	rm -rf *.o *.i  *~  $(INCDIR_APP_LOADER)/*~  $(TARGET) ../bin/ws281x.bin ws281x.bin

SLOT_FILE=/sys/devices/bone_capemgr.8/slots
dts: LEDscape.dts
	@SLOT="`grep LEDSCAPE $(SLOT_FILE) | cut -d: -f1`"; \
	if [ ! -z "$$SLOT" ]; then \
		echo "Removing slot $$SLOT"; \
		echo -$$SLOT > $(SLOT_FILE); \
	fi
	dtc -O dtb -o /lib/firmware/BB-LEDSCAPE-00A0.dtbo -b 0 -@ LEDscape.dts
	echo BB-LEDSCAPE > $(SLOT_FILE)


# Libraries and compiler
depend:
	$(MAKE) -C am335x/app_loader/interface
	$(MAKE) -C am335x/pasm
