/*
 * shmem_qnx_client.c
 *
 * Illustrate the use of QNX shared memory handles to securely setup a shared memory object between a client and server process.
 */

#include <stdio.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/neutrino.h>
#include <sys/netmgr.h>     // #define for ND_LOCAL_NODE is in here
#include <sys/iofunc.h>
#include <sys/dispatch.h>

#include "shmem_qnx.h" // defines messages between client and server

#define DEFAULT_CLIENT_STRING "hello from client"

int main(int argc, char **argv) {
	int coid;
	int status;
	unsigned len;
	int mem_fd;
	char *mem_ptr;
	get_shmem_msg_t get_msg;
	get_shmem_resp_t get_resp;
	release_shmem_msg_t release_msg;
	changed_shmem_msg_t changed_msg;
	changed_shmem_resp_t changed_resp;

	/* find our server */
	coid = name_open(SHMEM_SERVER_NAME, 0);
	if (-1 == coid)
	{
		perror("name_open");
		exit(EXIT_FAILURE);
	}

	get_msg.type = GET_SHMEM_MSG_TYPE;
	get_msg.shared_mem_bytes = 8192;  // share two pages, first for data to server, second for data from server

	/* send a get message to the server to get a shared memory handle from the server */
	status = MsgSend(coid, &get_msg, sizeof(get_msg), &get_resp, sizeof(get_resp));
	if (-1 == status)
	{
		perror("Get shmem MsgSend");
		exit(EXIT_FAILURE);
	}

	mem_fd = shm_open_handle(get_resp.mem_handle, O_RDWR);
	if( mem_fd == -1 ) {
		perror("shm_open_handle");
		exit(EXIT_FAILURE);
	}

	mem_ptr = mmap(NULL, 8192, PROT_READ|PROT_WRITE, MAP_SHARED, mem_fd, 0);
	if(mem_ptr == MAP_FAILED) {
		perror("mmap");
		exit(EXIT_FAILURE);
	}

	/* once mapped, we don't need the fd anymore */
	close(mem_fd);

	/* put some data into the shared memory object */
	if( argc > 1 ) {
		len = strlen(argv[1]);
		memcpy(mem_ptr+20, argv[1], len);
	} else
	{
		len = sizeof(DEFAULT_CLIENT_STRING) -1;
		memcpy(mem_ptr+20, DEFAULT_CLIENT_STRING, len);
	}

	/* build the update message */
	changed_msg.type = CHANGED_SHMEM_MSG_TYPE;
	changed_msg.offset = 20; // arbitrarily do this at byte 20
	changed_msg.length = len;

	status = MsgSend( coid, &changed_msg, sizeof(changed_msg), &changed_resp, sizeof(changed_resp));
	if( status == -1 ) {
		perror("Change shmem MsgSend");
		exit(EXIT_FAILURE);
	}

	printf("Got from server: \n");
	write(STDOUT_FILENO, mem_ptr+changed_resp.offset, changed_resp.length);
	write(STDOUT_FILENO, "\n", 1);

	/* we're done, so cleanup -- unmap the memory and tell the server we're done */
	(void)munmap(mem_ptr, 8192);
	release_msg.type = RELEASE_SHMEM_MSG_TYPE;
	status = MsgSend( coid, &release_msg, sizeof(release_msg), NULL, 0);
	if( status == -1 ) {
		perror("MsgSend");
		exit(EXIT_FAILURE);
	}

	return EXIT_SUCCESS;
}

