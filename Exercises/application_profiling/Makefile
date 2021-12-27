# 
# Makefile for application profiling
#

DEBUG = -g

CC = qcc
LD = qcc

TARGET = -Vgcc_ntox86_64
#TARGET = -Vgcc_ntoarmv7le
#TARGET = -Vgcc_ntoaarch64le



CFLAGS += $(TARGET) $(DEBUG) -Wall  
LDFLAGS+= $(TARGET) $(DEBUG)


BINS = profile_sample profile_func_instr

all: $(BINS)

profile_sample.o: profile_sample.c
	$(CC) $(CFLAGS) -p $< -c -o $@
	
profile_sample: profile_sample.o
	$(LD) $(LDFLAGS) -p $< -o $@ 


profile_func_instr.o: profile_func_instr.c
	$(CC) $(CFLAGS) -Wc,-finstrument-functions $< -c -o $@
	
profile_func_instr: profile_func_instr.o
	$(LD) $(LDFLAGS) $< -lprofilingS -o $@
	
clean:
	rm -f *.o $(BINS)
