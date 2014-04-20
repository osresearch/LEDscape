#########
#
# Build the PRU and LEDscape libraries as well as the PRU firmware.
#
#
TARGETS-y += $(LIBDIR)/matrix.bin
TARGETS-y += $(LIBDIR)/ws281x.bin

LIB-y += libledscape.a

libledscape.srcs += \
	ledscape.c \
	pru.c \
	util.c \
	config.c \
	fixed-font.c \

include ../../Makefile.common

