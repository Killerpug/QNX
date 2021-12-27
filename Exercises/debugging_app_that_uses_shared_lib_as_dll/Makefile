#
#	Makefile for app using a DLL
#

DEBUG = -g
CC = qcc
LD = qcc

TARGET = -Vgcc_ntox86_64
#TARGET = -Vgcc_ntoarmv7le
#TARGET = -Vgcc_ntoaarch64le

CFLAGS += $(DEBUG) $(TARGET) -Wall
LDFLAGS+= $(DEBUG) $(TARGET)

BINS = app_that_uses_so_as_dll

all:	$(BINS)

clean:
	rm -f *.o $(BINS)
