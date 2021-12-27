#
#	Makefile for sample shared library
#

DEBUG = -g
CC = qcc
LD = qcc

TARGET = -Vgcc_ntox86_64
#TARGET = -Vgcc_ntoarmv7le
#TARGET = -Vgcc_ntoaarch64le

CFLAGS += $(DEBUG) $(TARGET) -shared -Wc,-Wall
LDFLAGS+= $(DEBUG) $(TARGET) -shared

BINS = libfiles.so

all:	$(BINS)

clean:
	rm -f *.o $(BINS) *.tmp

libfiles.so:  files.o
	$(LD) $(LDFLAGS) files.o -o $@
	
