# Builds with whatever the host format is
CC := gcc

CFLAGS += \
	-O3 \
	-W \
	-Wall \
	-D_UNIX_ \

OBJS := \
	pasm.o \
	pasmpp.o \
	pasmexp.o \
	pasmop.o \
	pasmdot.o \
	pasmstruct.o \
	pasmmacro.o \

all: pasm

pasm: $(OBJS)
	$(CC) -o $@ $^

clean:
	$(RM) -f *.o


