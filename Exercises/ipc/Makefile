#
#	Makefile for QNX native ipc course module
#

# build option variables
DEBUG = -g

# specification of compiler and linker  
CC = qcc
LD = qcc

# specifications for target platform and compiler version
TARGET = -Vgcc_ntox86_64
#TARGET = -Vgcc_ntoarmv7le
#TARGET = -Vgcc_ntoaarch64le


# compile and link options
CFLAGS += $(TARGET) $(DEBUG) -Wall
LDFLAGS+= $(TARGET) $(DEBUG)

# binaries to be built
BINS = server client pulse_client disconnect_server disconnect_client \
unblock_server unblock_client event_server event_client \
shmem_posix_creator shmem_posix_user shmem_qnx_server shmem_qnx_client\
shmem_mutex_recovery

# uncomment for the pulse client and server exercise:
#BINS += pulse_server

# uncomment for the name_attach/name_open exerise:
#BINS += name_lookup_server name_lookup_client

# uncomment for the multi-part message exercise:
#BINS += iov_client iov_server

# make target to build all
all: $(BINS)

# make target to clean up object files, binaries and stripped (.tmp) files
clean:
	rm -f *.o $(BINS)
	cd solutions; make clean_solutions

# dependencies

server.o: server.c msg_def.h
client.o: client.c msg_def.h

pulse_server.o: pulse_server.c msg_def.h
pulse_client.o: pulse_client.c msg_def.h

name_lookup_server.o: name_lookup_server.c msg_def.h
name_lookup_client.o: name_lookup_client.c msg_def.h

iov_server.o: iov_server.c iov_server.h
iov_client.o: iov_client.c iov_server.h

disconnect_server.o: disconnect_server.c msg_def.h
disconnect_client.o: disconnect_client.c msg_def.h

unblock_server.o: unblock_server.c msg_def.h
unblock_client.o: unblock_client.c msg_def.h

event_server.o: event_server.c event_server.h
event_client.o: event_client.c event_server.h

