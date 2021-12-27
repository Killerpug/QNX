#include <pthread.h>
#include <stdint.h>

#define MAX_TEXT_LEN    100

typedef struct
{
	volatile unsigned init_flag;  // has the shared memory and control structures been initialized
	pthread_mutex_t mutex;
	pthread_cond_t cond;
	uint64_t data_version;  // for tracking updates, 64-bit count won't wrap during lifetime of a system
	char text[MAX_TEXT_LEN];
} shmem_t;
