#
#	Makefile for running and debugging
#




DEBUG = -g
CC = qcc
LD = qcc

TARGET = -Vgcc_ntox86_64
#TARGET = -Vgcc_ntoarmv7le
#TARGET = -Vgcc_ntoaarch64le

CFLAGS += $(DEBUG) $(TARGET) -Wc,-Wall
LDFLAGS+= $(DEBUG) $(TARGET)

BINS = listfiles mallocbad

all: $(BINS)

clean:
	-rm -f *.o $(BINS)
	
listfiles: main.o files.o
	$(LD) $(LDFLAGS) $< files.o -o $@
