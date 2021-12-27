#
#	Makefile for app using a static library
#

DEBUG = -g
CC = qcc
LD = qcc

TARGET = -Vgcc_ntox86_64
#TARGET = -Vgcc_ntoarmv7le
#TARGET = -Vgcc_ntoaarch64le

CFLAGS += $(DEBUG) $(TARGET) -I ../debugging_static_lib -Wc,-Wall
LDFLAGS+= $(DEBUG) $(TARGET) -L ../debugging_static_lib

BINS = app_that_uses_static_lib

all:	$(BINS)

clean:
	rm -f *.o $(BINS)

app_that_uses_static_lib: app_that_uses_static_lib.o
	$(LD) $(LDFLAGS) app_that_uses_static_lib.o -Bstatic -lfiles -Bdynamic -o $@
	
	