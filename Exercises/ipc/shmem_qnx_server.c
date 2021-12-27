/*
 * shmem_qnx_server.c
 *
 * Illustrate the use of QNX shared memory handles to securely setup a shared memory object between a client and server process.
 *
 */

#include <fcntl.h>
#include <stdio.h>
#include <sys/mman.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/neutrino.h>
#include <process.h>
#include <string.h>
#include <sys/iofunc.h>
#include <sys/dispatch.h>

#include "shmem_qnx.h" // defines messages between client & server


/* create a secured shared-memory object, updating a handle to it. */

int create_shared_memory(unsigned nbytes, int client_pid, void **ptr, shm_handle_t *handle) {
	int fd;
	int ret;

	/* create an anonymous shared memory object */
	fd = shm_open(SHM_ANON, O_RDWR|O_CREAT, 0600);
	if( fd == -1 ) {
		perror("shm_open");
		return -1;
	}

	/* allocate the memory for the object */
	ret = ftruncate(fd, nbytes);
	if( ret == -1 ) {
		perror("ftruncate");
		close(fd);
		return -1;
	}

	/* get a local mapping to the object */
	*ptr = mmap(NULL, nbytes, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
	if(*ptr == MAP_FAILED) {
		perror("mmap");
		close(fd);
		return -1;
	}

	printf("fd is %d, client_pid is %d\n", fd, client_pid);

	/* get a handle for the client to map the object */
	ret = shm_create_handle( fd, client_pid, O_RDWR, handle, 0);

	if( ret == -1 ) {
		perror("shmem_create_handle");
		close(fd);
		munmap( *ptr, nbytes );
		return -1;
	}

	/* we no longer need the fd, so cleanup */
	(void)close(fd);

	return 0;
}

typedef union
{
	uint16_t type;
	struct _pulse pulse;
	get_shmem_msg_t get_shmem;
	changed_shmem_msg_t changed_shmem;
} recv_buf_t;

#define DEFAULT_RESPONSE "Answer from server"

int main(int argc, char **argv)
{
	int rcvid;
	recv_buf_t rbuf;
	int status;
	name_attach_t *att;
	struct _msg_info msg_info;
	int client_scoid = 0; // no client yet
	char *shmem_ptr;
	unsigned shmem_memory_size;
	get_shmem_resp_t get_resp;
	changed_shmem_resp_t changed_resp;
	char *resp;
	unsigned resp_len;

	/* If we have any arguments, use first as response, otherwise use the default */
	if( argc > 1 ) {
		resp = argv[1];
		resp_len = strlen(resp);
	} else {
		resp = DEFAULT_RESPONSE;
		resp_len = sizeof(DEFAULT_RESPONSE)-1;
	}

	// register our name
	att = name_attach(NULL, SHMEM_SERVER_NAME, 0);

	if (NULL == att) {
		perror("name_attach");
		exit(EXIT_FAILURE);
	}

	while (1) {
		rcvid = MsgReceive(att->chid, &rbuf, sizeof(rbuf), &msg_info );

		if (rcvid == -1) {
			perror("MsgReceive");
			exit(EXIT_FAILURE);
		} else if (0 == rcvid) {
			if(rbuf.pulse.code == _PULSE_CODE_DISCONNECT) {
				// a client went away -- was it ours?
				if( rbuf.pulse.scoid == client_scoid ) {
					client_scoid = 0;
					munmap(shmem_ptr, shmem_memory_size);
				}
				ConnectDetach(rbuf.pulse.scoid);
			}
		} else {
			// we got a message
			switch (rbuf.type) {
			case GET_SHMEM_MSG_TYPE:
				shmem_memory_size = rbuf.get_shmem.shared_mem_bytes;
				if( shmem_memory_size > 64*1024 ) {
					MsgError(rcvid, EINVAL);
					continue;
				}
				status = create_shared_memory(shmem_memory_size,  msg_info.pid,  (void *)&shmem_ptr, &get_resp.mem_handle);
				if( status != 0 ) {
					MsgError(rcvid, errno);
					continue;
				}
				client_scoid = msg_info.scoid;

				status = MsgReply(rcvid, EOK, &get_resp, sizeof(get_resp));
				if (-1 == status) {
					perror("MsgReply");
					(void)MsgError(rcvid, errno);
				}
				break;

			case CHANGED_SHMEM_MSG_TYPE:
			{
				if(msg_info.scoid != client_scoid) {
					// only the current client may tell us to update/change the memory
					(void)MsgError(rcvid, EPERM);
					continue;
				}

				const unsigned offset = rbuf.changed_shmem.offset;
				const unsigned nbytes = rbuf.changed_shmem.length;
				if( (nbytes > shmem_memory_size) || (offset > shmem_memory_size) || ((nbytes + offset) > shmem_memory_size)) {
					// oh no you don't
					MsgError(rcvid, EBADMSG);
					continue;
				}

				printf("Got from client:\n");
				write(STDOUT_FILENO, shmem_ptr+offset, nbytes);
				write(STDOUT_FILENO, "\n", +1 );

				changed_resp.offset = 4096+30; // 2nd page for answer, 30 offset into page is arbitrary
				memcpy(shmem_ptr+changed_resp.offset, resp, resp_len);
				changed_resp.length = resp_len;
				status = MsgReply(rcvid, EOK, &changed_resp, sizeof(changed_resp));
				if (-1 == status) {
					// reply failed... try to unblock client with the error, just in case we still can
					perror("MsgReply");
					(void)MsgError(rcvid, errno);
				}
				break;
			}

			case RELEASE_SHMEM_MSG_TYPE:
				if(msg_info.scoid != client_scoid) {
					// only the current client may tell us to release the memory
					(void)MsgError(rcvid, EPERM);
					continue;
				}
				client_scoid = 0;
				munmap(shmem_ptr, shmem_memory_size);
				status = MsgReply(rcvid, EOK, NULL, 0);
				if (-1 == status) {
					// reply failed... try to unblock client with the error, just in case we still can
					perror("MsgReply");
					(void)MsgError(rcvid, errno);
				}
				break;

			default:
				// unknown message type, unblock client with an error
				if (MsgError(rcvid, ENOSYS) == -1)
					perror("MsgError");
				break;
			}
		}
	}
	return 0;
}
