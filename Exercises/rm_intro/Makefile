
#
#	Makefile for resmgr project (rmintro)
#



DEBUG = -g
CC = qcc
LD = qcc

TARGET = -Vgcc_ntox86_64
#TARGET = -Vgcc_ntoarmv7le
#TARGET = -Vgcc_ntoaarch64le

CFLAGS += $(DEBUG) $(TARGET) -Wall
LDFLAGS+= $(DEBUG) $(TARGET)

BINS = example

all:	$(BINS)

clean:
	rm -f *.o $(BINS)
	cd solutions; make clean
