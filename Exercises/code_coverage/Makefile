# 
# Makefile for code coverage
#

DEBUG = -g
CC = qcc
LD = qcc

TARGET = -Vgcc_ntox86_64
#TARGET = -Vgcc_ntoarmv7le
#TARGET = -Vgcc_ntoaarch64le

CFLAGS += $(TARGET) $(DEBUG) -Wall
LDFLAGS+= $(TARGET) $(DEBUG)

BINS = rbt_client rbt_server

all:	$(BINS)

rbt_server.o: rbt_server.c rbt_server.h
	$(CC) $(CFLAGS) -Wc,-ftest-coverage -Wc,-fprofile-arcs  -O0 rbt_server.c -c -o rbt_server.o
	
rbt_server: rbt_server.o
	$(LD) $(LDFLAGS) -ftest-coverage -fprofile-arcs $< -o $@ 


clean:
	rm -f *.o $(BINS) *.bb *.bbg *.gcno
