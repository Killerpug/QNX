/* Do NOT do the condition variable exercise in this source file.
 * Work in condvar.c instead.
 */

/*
 *  prodcons.c
 *
 *  This module demonstrates POSIX condition variables by
 *  way of a producer and consumer.  Since we only have
 *  two threads waiting for the signal, and at any given
 *  point one of them is running, we can just use the
 *  pthread_cond_signal call to awaken the other thread.
 *
 */

#include <stdio.h>
#include <sys/neutrino.h>
#include <pthread.h>
#include <sched.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

/*
 *  our mutex and condition variable
 */

pthread_mutex_t mutex =
PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond =
PTHREAD_COND_INITIALIZER;

/*
 *  our global variables.
 */

volatile int state = 0; // used as a state variable
volatile int product = 0; // the output of the producer

void *producer(void *);
void *consumer(void *);
void do_producer_work(void);
void do_consumer_work(void);

char *progname = "prodcons";

int main()
{
	if ((pthread_create(NULL, NULL, consumer, NULL)) != EOK)
	{
		fprintf(stderr, "%s: consumer pthread_create failed.\n", progname);
		exit(EXIT_FAILURE);
	}

	if ((pthread_create(NULL, NULL, producer, NULL)) != EOK)
	{
		fprintf(stderr, "%s: producer pthread_create failed.\n", progname);
		exit(EXIT_FAILURE);
	}

	sleep(20); // let the threads run
	printf("%s:  main, exiting\n", progname);
	return 0;
}

/*
 *  producer
 */

void *
producer(void *arg)
{
	int ret;

	while (1)
	{
		ret = pthread_mutex_lock(&mutex);
		if (ret != EOK)
		{
			fprintf(stderr, "%s: pthread_mutex_lock failed: %s\n", progname, strerror(ret));
			exit(EXIT_FAILURE);
		}
		while (state == 1)
		{
			ret = pthread_cond_wait(&cond, &mutex);
			if (ret != EOK)
			{
				fprintf(stderr, "%s: pthread_cond_wait failed: %s\n", progname, strerror(ret));
				exit(EXIT_FAILURE);
			}
		}
		printf("%s:  produced %d, state %d\n", progname, ++product, state);
		state = 1;
		ret = pthread_cond_signal(&cond);
		if (ret != EOK)
		{
			fprintf(stderr, "%s: pthread_cond_signal failed: %s\n", progname, strerror(ret));
			exit(EXIT_FAILURE);
		}
		ret = pthread_mutex_unlock(&mutex);
		if (ret != EOK)
		{
			fprintf(stderr, "%s: pthread_mutex_unlock failed: %s\n", progname, strerror(ret));
			exit(EXIT_FAILURE);
		}
		do_producer_work();
	}
	return (NULL);
}

/*
 *  consumer
 */

void *
consumer(void *arg)
{
	while (1)
	{
		int ret;

		ret = pthread_mutex_lock(&mutex);
		if (ret != EOK)
		{
			fprintf(stderr, "%s: pthread_mutex_lock failed: %s\n", progname, strerror(ret));
			exit(EXIT_FAILURE);
		}
		while (state == 0)
		{
			ret = pthread_cond_wait(&cond, &mutex);
			if (ret != EOK)
			{
				fprintf(stderr, "%s: pthread_cond_wait failed: %s\n", progname, strerror(ret));
				exit(EXIT_FAILURE);
			}
		}
		printf("%s:  consumed %d, state %d\n", progname, product, state);
		state = 0;
		ret = pthread_cond_signal(&cond);
		if (ret != EOK)
		{
			fprintf(stderr, "%s: pthread_cond_signal failed: %s\n", progname, strerror(ret));
			exit(EXIT_FAILURE);
		}
		ret = pthread_mutex_unlock(&mutex);
		if (ret != EOK)
		{
			fprintf(stderr, "%s: pthread_mutex_unlock failed: %s\n", progname, strerror(ret));
			exit(EXIT_FAILURE);
		}
		do_consumer_work();
	}
	return (NULL);
}

void do_producer_work(void)
{
	delay(100); // pretend to be doing something
}

void do_consumer_work(void)
{
	delay(100); // pretend to be doing something
}
