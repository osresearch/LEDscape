#CROSS_COMPILE?=arm-arago-linux-gnueabi-

LIBDIR_APP_LOADER?=../../app_loader/lib
INCDIR_APP_LOADER?=../../app_loader/include
BINDIR?=../bin

CFLAGS+= -Wall -I$(INCDIR_APP_LOADER) -D__DEBUG -O2 -mtune=cortex-a8 -march=armv7-a
LDFLAGS+=-L$(LIBDIR_APP_LOADER) -lprussdrv -lpthread
PASM := ../../utils/pasm_2
OBJDIR=obj
TARGET=$(BINDIR)/ws281x

_DEPS = 
DEPS = $(patsubst %,$(INCDIR_APP_LOADER)/%,$(_DEPS))

_OBJ = ws281x.o
OBJ = $(patsubst %,$(OBJDIR)/%,$(_OBJ))

$(OBJDIR)/%.o: %.c $(DEPS)
	@mkdir -p obj
	$(CROSS_COMPILE)gcc $(CFLAGS) -c -o $@ $< 

all: $(TARGET) ws281x.bin
$(TARGET): $(OBJ)
	$(CROSS_COMPILE)gcc $(CFLAGS) -o $@ $^ $(LDFLAGS)

ws281x.bin: ws281x.p ws281x.hp
	$(CPP) - < $< | grep -v '^#' | sed 's/;/\n/g' > $<.i
	$(PASM) -V3 -b $<.i
	cp $@ ../bin/


.PHONY: clean

clean:
	rm -rf $(OBJDIR)/ *~  $(INCDIR_APP_LOADER)/*~  $(TARGET) ../bin/ws281x.bin ws281x.bin
