#
#	Makefile for sample static library
#

DEBUG = -g
CC = qcc
LD = qcc

TARGET = -Vgcc_ntox86_64
#TARGET = -Vgcc_ntoarmv7le
#TARGET = -Vgcc_ntoaarch64le

CFLAGS += $(DEBUG) $(TARGET) -Wc,-Wall
LDFLAGS+= $(DEBUG) $(TARGET)

BINS = libfiles.a

all:	$(BINS)

clean:
	rm -f *.o $(BINS)


all:	$(BINS)

libfiles.a: files.o
	$(LD) $(LDFLAGS) files.o -A $@
	
