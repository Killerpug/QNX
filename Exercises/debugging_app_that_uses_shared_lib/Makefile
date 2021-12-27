#
#	Makefile for app using a shared library
#

DEBUG = -g
CC = qcc
LD = qcc

TARGET = -Vgcc_ntox86_64
#TARGET = -Vgcc_ntoarmv7le
#TARGET = -Vgcc_ntoaarch64le

CFLAGS += $(DEBUG) $(TARGET) -Wall -I ../debugging_shared_lib
LDFLAGS+= $(DEBUG) $(TARGET) -L ../debugging_shared_lib

BINS = app_that_uses_so

all:	$(BINS)

clean:
	rm -f *.o $(BINS)

app_that_uses_so: app_that_uses_lib.o
	$(LD) $(LDFLAGS) app_that_uses_lib.o -Bdynamic -lfiles -o $@

