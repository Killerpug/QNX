#
#	Makefile for memory problems
#

DEBUG = -g
CC = qcc
LD = qcc

TARGET = -Vgcc_ntox86_64
#TARGET = -Vgcc_ntoarmv7le
#TARGET = -Vgcc_ntoaarch64le

CFLAGS += $(DEBUG) $(TARGET) -Wc,-Wall
LDFLAGS+= $(DEBUG) $(TARGET)

BINS = mallocbad memory_leak time

all:	$(BINS)

clean:
	rm -f *.o $(BINS) *.tmp

