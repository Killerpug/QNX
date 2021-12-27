/*
 *  shmem_posix_user.c
 *
 *  This module demonstrates shared memory with mutexes and condvars
 *  by opening some shared memory with retries, then waiting for changes
 *  in the shared memory area.  The access and retries are done in
 *  get_shared_memory_pointer() and the waiting for changes in the
 *  main loop.
 *
 *  This one is meant to be run in tandem with shmem_posix_creator.c.
 *
 *  Run it as: shmem_posix_user shared_memory_object_name
 *  Example: shmem_posix_user /wally
 *
 *  The condvar is a notification mechanism, the mutex makes sure that only
 *  one process accesses the shared memory at a time, and the data_version is needed
 *  because the condvar notification is lost if no one is waiting at the time it's
 *  sent.
 *
 *  This models waiting for changes to a global state/configuration, then updating
 *  a local cache of that configuration within this process.
 *
 */

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>

/* shmem_posix.h contains the structure that is overlaid on the shared memory */
#include "shmem_posix.h"


/* function to setup access to the shared memory.
 * It takes a retry count to allow for a bounded number of
 * retries in case the user of the shared memory is started
 * before or in parallel with the creator of it.
 */

void *get_shared_memory_pointer( char *name, unsigned num_retries )
{
	unsigned tries;
	shmem_t *ptr;
	int fd;

	for (tries = 0;;) {
		fd = shm_open(name, O_RDWR, 0);
		if (fd != -1) break;
		++tries;
		if (tries > num_retries) {
			perror("shmn_open");
			return MAP_FAILED;
		}
		/* wait one second then try again */
		sleep(1);
	}

	for (tries = 0;;) {
		ptr = mmap(0, sizeof(shmem_t), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
		if (ptr != MAP_FAILED) break;
		++tries;
		if (tries > num_retries) {
			perror("mmap");
			return MAP_FAILED;
		}
		/* wait one second then try again */
		sleep(1);
	}

	/* no longer need fd */
	(void)close(fd);

	for (tries=0;;) {
		if (ptr->init_flag) break;
		++tries;
		if (tries > num_retries) {
			fprintf(stderr, "init flag never set\n");
			(void)munmap(ptr, sizeof(shmem_t));
			return MAP_FAILED;
		}
		/* wait on second then try again */
		sleep(1);
	}

	return ptr;
}

int main(int argc, char *argv[])
{
	int ret;
	shmem_t *ptr;
	uint64_t last_version = 0;
	char local_data_copy[MAX_TEXT_LEN];

	if (argc != 2)
	{
		printf("ERROR: use: shmem_posix_user shared_memory_object_name\n");
		printf("Example: shmem_posix_user /wally\n");
		exit(EXIT_FAILURE);
	}

	if (*argv[1] != '/')
	{
		printf("ERROR: the shared memory name should start with a leading '/' character\n");
		exit(EXIT_FAILURE);
	}

	/* try to get access to the shared memory object, retrying for 100 times (100 seconds) */
	ptr = get_shared_memory_pointer(argv[1], 100);
	if (ptr == MAP_FAILED)
	{
		fprintf(stderr, "Unable to access object '%s' - was creator run with same name?\n", argv[1]);
		exit(EXIT_FAILURE);
	}

	while (1) {
		/* lock the mutex because we're about to access shared data */
		ret = pthread_mutex_lock(&ptr->mutex);
		if (ret != EOK)
		{
			perror("pthread_mutex_lock");
			exit(EXIT_FAILURE);
		}

		/* wait for changes to the shared memory object */
		while (last_version == ptr->data_version) {
			ret = pthread_cond_wait(&ptr->cond, &ptr->mutex); /* does an unlock, wait, lock */
			if (ret != EOK)
			{
				perror("pthread_cond_wait");
				exit(EXIT_FAILURE);
			}
		}

		/* update local version and data */
		last_version = ptr->data_version;
		strlcpy(local_data_copy, ptr->text, sizeof(local_data_copy));

		/* finished accessing shared data, unlock the mutex */
		ret = pthread_mutex_unlock(&ptr->mutex);
		if (ret != EOK)
		{
			perror("pthread_mutex_unlock");
			exit(EXIT_FAILURE);
		}

		printf("Data in shared memory was: '%s'\n", local_data_copy);
	}

	return EXIT_SUCCESS;
}
