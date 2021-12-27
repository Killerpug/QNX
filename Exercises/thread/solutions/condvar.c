/*
 *  condvar.c
 *
 *  Objectives:
 *
 *      condvar.c was a two-state example.  The producer (or
 *      state 0) did something which caused the consumer (or
 *      state 1) to run.  State 1 did something which caused
 *      a return to state 0.  Each thread implemented one of
 *      the states.
 *
 *      This example will have 4 states in its state machine 
 *      with the following state transitions:
 *
 *        State 0 -> State 1
 *        State 1 -> State 2 if state 1's internal variable indicates "even"
 *        State 1 -> State 3 if state 1's internal variable indicates "odd"
 *        State 2 -> State 0
 *        State 3 -> State 0
 *    
 *      And, of course, one thread implementing each state, sharing the
 *      same state variable and condition variable for notification of
 *      change in the state variable.
 *
 *      Note: Error checking has been left out in much of this example
 *      to increase readability.  Production code should not leave out
 *      this error checking.
 *
 */

#include <stdio.h>
#include <unistd.h>
#include <sys/neutrino.h>
#include <pthread.h>
#include <sched.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

/*
 *  our global variables.
 */

volatile int state; // which state we are in

/*
 *  our mutex and condition variable
 */

pthread_mutex_t mutex;
pthread_cond_t cond;

void *state_0(void *);
void *state_1(void *);
void *state_2(void *);
void *state_3(void *);

char *progname = "condvar";

int main()
{
	int ret;

	ret = pthread_mutex_init(&mutex, NULL );
	if (ret != EOK)
	{
		fprintf(stderr, " %s:  pthread_mutex_init failed: %s\n", progname, strerror(ret));
		exit(EXIT_FAILURE);
	}

	ret = pthread_cond_init(&cond, NULL );
	if (ret != EOK)
	{
		fprintf(stderr, " %s:  pthread_cond_init failed: %s\n", progname, strerror(ret));
		exit(EXIT_FAILURE);
	}

	state = 0;

	pthread_create(NULL, NULL, state_1, NULL);
	pthread_create(NULL, NULL, state_0, NULL);
	pthread_create(NULL, NULL, state_2, NULL);
	pthread_create(NULL, NULL, state_3, NULL);

	sleep(20); // let the threads run
	printf("%s:  main, exiting\n", progname);

	return 0;
}

/*
 *  state 0 handler (was producer)
 */

void *
state_0(void *arg)
{
	while (1)
	{
		pthread_mutex_lock(&mutex);
		while (state != 0)
		{
			pthread_cond_wait(&cond, &mutex);
		}
		printf("%s:  transit 0 -> 1\n", progname);
		state = 1;
		pthread_cond_broadcast(&cond);
		pthread_mutex_unlock(&mutex);
		/* don't chew all the CPU time */
		delay(100);
	}
	return (NULL);
}

/*
 *  state 1 handler (was consumer)
 */

void *
state_1(void *arg)
{
	int i = 1;

	while (1)
	{
		pthread_mutex_lock(&mutex);
		while (state != 1)
		{
			pthread_cond_wait(&cond, &mutex);
		}

		if (++i & 0x1)
		{
			state = 3;
		}
		else
		{
			state = 2;
		}

		printf("%s:  transit 1 -> %d\n", progname, state);

		pthread_cond_broadcast(&cond);
		pthread_mutex_unlock(&mutex);
	}
	return (NULL);
}

void *
state_2(void *arg)
{
	while (1)
	{
		pthread_mutex_lock(&mutex);
		while (state != 2)
		{
			pthread_cond_wait(&cond, &mutex);
		}
		printf("%s:  transit 2 -> 0\n", progname);
		state = 0;
		pthread_cond_broadcast(&cond);
		pthread_mutex_unlock(&mutex);
	}
	return (NULL);
}

void *
state_3(void *arg)
{
	while (1)
	{
		pthread_mutex_lock(&mutex);
		while (state != 3)
		{
			pthread_cond_wait(&cond, &mutex);
		}
		printf("%s:  transit 3 -> 0\n", progname);
		state = 0;
		pthread_cond_broadcast(&cond);
		pthread_mutex_unlock(&mutex);
	}
	return (NULL);
}
