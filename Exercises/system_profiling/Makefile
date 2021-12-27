#
#	Makefile for system profiling
#

DEBUG = -g
PROFILE = -p

CC = qcc
LD = qcc

TARGET = -Vgcc_ntox86_64
#TARGET = -Vgcc_ntoarmv7le
#TARGET = -Vgcc_ntoaarch64le

CFLAGS += $(TARGET) -Wall
LDFLAGS+= $(TARGET)

BINS = sys_prof_ex trace_user_events hw_server cpu_burner \
high_prio_client low_prio_client fixed_server

all: $(BINS)

clean:
	rm -f *.o $(BINS)

fixed_server: hw_server.c hw_server.h
	$(CC) $(CFLAGS) $(LDFLAGS) -D PRIO_FIX hw_server.c -o fixed_server	
