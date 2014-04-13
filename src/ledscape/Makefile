#########
#
# Build the PRU and LEDscape libraries as well as the PRU firmware.
#
#
TARGETS-y += $(LIBDIR)/libledscape.a
TARGETS-y += $(LIBDIR)/matrix.bin
TARGETS-y += $(LIBDIR)/ws281x.bin

include ../../Makefile.common

$(LIBDIR)/libledscape.a: \
	$(OBJDIR)/ledscape.o \
	$(OBJDIR)/pru.o \
	$(OBJDIR)/bitslice.o \
	$(OBJDIR)/util.o \


