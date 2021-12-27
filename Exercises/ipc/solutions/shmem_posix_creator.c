/*
 *  shmem_posix_creator.c
 *
 *  This module demonstrates shared memory with mutexes and condvars
 *  by creating some shared memory and putting something in it, then updating
 *  it on a regular basis.  On each update, it will broadcast the condvar
 *  letting any readers know that data has changed.
 *
 *  This one is meant to be run in tandem with one more more instances of shmem_posix_user.c.
 *
 *  Run it as: shmem_posix_creator shared_memory_object_name
 *  Example: shmem_posix_creator /wally
 *
 *  The condvar is a notification mechanism, the mutex makes sure that only
 *  one process accesses the shared memory at a time, and the data_version is needed
 *  because the condvar notification is lost if no one is waiting at the time it's
 *  sent.
 *
 *  This models a "global" state or configuration area that multiple processes may wish
 *  to access, and if needed, wait on state/configuration changes.
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

/* shmem.h contains the structure that is overlaid on the shared memory */
#include "shmem_posix.h"

/* on any failures after creating our object we need to remove it */
void unlink_and_exit(char *name)
{
	(void)shm_unlink(name);
	exit(EXIT_FAILURE);
}

int main(int argc, char *argv[])
{
	int fd;
	shmem_t *ptr;
	int ret;
	pthread_mutexattr_t mutex_attr;
	pthread_condattr_t cond_attr;

	if (argc != 2)
	{
		printf("ERROR: use: shmem_posix_creator shared_memory_object_name\n");
		printf("Example: shmem_posix_creator /wally\n");
		exit(EXIT_FAILURE);
	}
	if (*argv[1] != '/')
	{
		printf("ERROR: the shared memory name should start with a leading '/' character\n");
		exit(EXIT_FAILURE);
	}

	printf("Creating shared memory object: '%s'\n", argv[1]);

	/* create the shared memory object */

	fd = shm_open(argv[1], O_RDWR | O_CREAT | O_EXCL, 0660);
	if (fd == -1)
	{
		perror("shm_open()");
		unlink_and_exit(argv[1]);
	}

	/* set the size of the shared memory object, allocating at least one page of memory */

	ret = ftruncate(fd, sizeof(shmem_t));
	if (ret == -1)
	{
		perror("ftruncate");
		unlink_and_exit(argv[1]);
	}

	/* get a pointer to the shared memory */

	ptr = mmap(0, sizeof(shmem_t), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (ptr == MAP_FAILED)
	{
		perror("mmap");
		unlink_and_exit(argv[1]);
	}

	/* don't need fd anymore, so close it */
	close(fd);

	/* now set up our mutex and condvar for the synchronization and notification */

	pthread_mutexattr_init(&mutex_attr);
	pthread_mutexattr_setpshared(&mutex_attr, PTHREAD_PROCESS_SHARED);
	ret = pthread_mutex_init(&ptr->mutex, &mutex_attr);
	if (ret != EOK)
	{
		perror("pthread_mutex_init");
		unlink_and_exit(argv[1]);
	}

	pthread_condattr_init(&cond_attr);
	pthread_condattr_setpshared(&cond_attr, PTHREAD_PROCESS_SHARED);
	ret = pthread_cond_init(&ptr->cond, &cond_attr);
	if (ret != EOK)
	{
		perror("pthread_cond_init");
		unlink_and_exit(argv[1]);
	}

	/*
	 * our memory is now "setup", so set the init_flag
	 * it was guaranteed to be zero at allocation time
	 */
	ptr->init_flag = 1;

	printf("Shared memory created and init_flag set to let users know shared memory object is usable.\n");

	while (1) {
		sleep(1);

		/* lock the mutex because we're about to update shared data */
		ret = pthread_mutex_lock(&ptr->mutex);
		if (ret != EOK)
		{
			perror("pthread_mutex_lock");
			unlink_and_exit(argv[1]);
		}

		ptr->data_version++;
		snprintf(ptr->text, sizeof(ptr->text), "data update: %lu", ptr->data_version);

		/* finished accessing shared data, unlock the mutex */
		ret = pthread_mutex_unlock(&ptr->mutex);
		if (ret != EOK)
		{
			perror("pthread_mutex_unlock");
			unlink_and_exit(argv[1]);
		}

		/* wake up any readers that may be waiting */
		ret = pthread_cond_broadcast(&ptr->cond);
		if (ret != EOK)
		{
			perror("pthread_cond_broadcast");
			unlink_and_exit(argv[1]);
		}
	}

	/* we'll never exit the above loop but here's the cleanup anyway */

	/* unmap() not actually needed on termination as all memory mappings are freed on process termination */
	if (munmap(ptr, sizeof(shmem_t)) == -1)
	{
		perror("munmap");
	}

	/* but the name must be removed */
	if (shm_unlink(argv[1]) == -1)
	{
		perror("shm_unlink");
	}

	return EXIT_SUCCESS;
}
