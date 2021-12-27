/*
 * shmem_qnx.h
 *
 */


#include <stdint.h>
#include <sys/iomsg.h>
#include <sys/mman.h>

#define GET_SHMEM_MSG_TYPE (_IO_MAX+200)
#define CHANGED_SHMEM_MSG_TYPE (_IO_MAX+201)
#define RELEASE_SHMEM_MSG_TYPE (_IO_MAX+202)

#define SHMEM_SERVER_NAME "shmem_server"

// ask for a shared_mem_bytes sized object
typedef struct get_shmem_msg {
	uint16_t type;
	unsigned shared_mem_bytes;
} get_shmem_msg_t;

// response provides a handle to that object
typedef struct get_shmem_resp {
	shm_handle_t mem_handle;
} get_shmem_resp_t;

// inform the server that length bytes starting at offset have been changed
typedef struct changed_shmem_msg {
	uint16_t type;
	unsigned offset;
	unsigned length;
} changed_shmem_msg_t;

// reply that length bytes offset have been changed
typedef struct changed_shmem_resp {
	unsigned offset;
	unsigned length;
} changed_shmem_resp_t;

// release the shared memory on the server side
typedef struct release_shmem_msg {
	uint16_t type;
} release_shmem_msg_t;

// reply is only success/failure, no data
