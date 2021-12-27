/*
 * hw_server
 *
 * multi-threaded server that takes messages and deals with "hardware"
 * 
 * Uses a hardware control structure with a semaphore for locking.
 * Registers name with name_attach(), to be found by name_open().
 * Uses a varying delay (nanospin) to represent the hw access
 * 
 * -t number of threads (default 4)
 * -v verbose (multiple vs possible)
 * 
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <semaphore.h>
#include <sys/dispatch.h>
#include <sys/neutrino.h>
#include <pthread.h>
#include <errno.h>
#include <sys/trace.h>
#include <sys/syspage.h>
#include <inttypes.h>

#include "hw_server.h"
#include <string.h>

/* #define PRIO_FIX */

#ifndef PRIO_FIX
struct hw_descriptor
{
	sem_t lock;
	int irq;
	int state;
	void *ibuf;
	void *obuf;
};
#else
struct hw_descriptor
{
	pthread_mutex_t lock;
	int irq;
	int state;
	void *ibuf;
	void *obuf;
};
#endif

/* globals */
struct hw_descriptor hw_device;
struct sched_param sched_param;
name_attach_t *attach;
int verbose = 0;

/*
 * 	error_out
 *
 * 	This routine prints the error and exits
 */
void error_out(char *msg, int error)
{
	printf("hw_server: %s, errno %d:%s\n", msg, error, strerror(error));
	exit(EXIT_FAILURE);
}

/*
 * init
 *
 * This routine initializes various attributes of the server
 * -- name_attach()
 * -- sem_init()
 * -- ticksize
 */
void init()
{
	int ret;
	struct _clockperiod cp;

#ifndef PRIO_FIX
	if (sem_init(&hw_device.lock, 0, 1) == -1)
		error_out("sem_init failed", errno );
#else
	if( pthread_mutex_init( &hw_device.lock, NULL ) == -1)
		error_out( "mutex init failed", errno );
#endif

	attach = name_attach(NULL, HW_SERVER_NAME, 0);
	if (NULL == attach)
		error_out("name_attach failed", errno );

	/* need 1ms ticksize for this exercise */
	ClockPeriod(CLOCK_REALTIME, NULL, &cp, 0);
	if (cp.nsec > 1000000)
	{
		cp.nsec = 1000000;
		cp.fract = 0;
		ret = ClockPeriod(CLOCK_REALTIME, &cp, NULL, 0);
		if (-1 == ret)
			error_out("failed to set 1ms ticksize", errno );
	}
}

/*
 * 	nanospin_clock
 *
 * This routine simply keeps the CPU busy for a designated period of time
 */
void nanospin_clock(unsigned nsec)
{
	uint64_t cps, clock_start, clock_cur;

	cps = SYSPAGE_ENTRY(qtime)->cycles_per_sec;

	clock_start = ClockCycles();
	while (1)
	{
		clock_cur = ClockCycles();
		if (clock_cur < clock_start)
			return; /* wrapped, might happen on SH */

		if ((1000000000 * (clock_cur - clock_start) / cps) > nsec)
			return; /* done */
	}
}

/*
 * 	hw_lock
 *
 * 	This routine simply locks a semaphore
 */
#ifndef PRIO_FIX
void hw_lock()
{
	if ( -1 == sem_wait(&hw_device.lock) )
		error_out( "sem_wait", errno );
}

/*
 * 	hw_unlock
 *
 * 	This routine simply unlocks a semaphore
 */
void hw_unlock()
{
	if ( -1 == sem_post(&hw_device.lock) )
		error_out ( "sem_post", errno );
}
#else
/*
 * 	hw_lock
 *
 * 	This routine simply locks a mutex
 */
void hw_lock()
{
	if ( -1 == pthread_mutex_lock( &hw_device.lock ) )
		error_out ( "pthread_mutex_lock", errno );
}

/*
 * 	hw_unlock
 *
 * 	This routine simply unlocks a mutex
 */
void hw_unlock()
{
	if ( -1 == pthread_mutex_unlock( &hw_device.lock ) )
		error_out ( "pthread_mutex_unlock", errno );
}
#endif

/*
 * hw_out
 *
 * lock hardware structure for safe access
 * pretend to do hardware work by delaying some time with nanospin_clock()
 * unlock hardware structure
 */
void hw_out(int period)
{
	hw_lock();
	if (verbose)
		printf("hw_out(high) post-lock, period: %d\n", period);

	nanospin_clock(period * 1000);

	if (verbose)
		printf("hw_out(high) pre-unlock\n");
	hw_unlock();
	if (verbose)
		printf("hw_out(high) post-unlock\n");
}

/*
 * hw_in
 *
 * lock hardware structure for safe access
 * pretend to do hardware work by delaying some time with nanospin_clock()
 * unlock hardware structure
 */
void hw_in(int length)
{

	hw_lock();
	if (verbose)
		printf("hw_in(low) post-lock, length %d\n", length);

	nanospin_clock(length * 1000);

	if (verbose)
	{
		pthread_getschedparam(0, NULL, &sched_param );
		printf("hw_in(low) pre-unlock: %d\n", sched_param.sched_priority);
	}

	hw_unlock();
	if (verbose)
		printf("hw_in(low) post-unlock\n");
}

/*
 * mainloop
 *
 * while 1 loop receiving messages and processing them *
 * called by multiple threads
 */
void mainloop()
{
	int rcvid;
	hw_msgs_t msg;

	while (1)
	{
		rcvid = MsgReceive(attach->chid, &msg, sizeof(msg), NULL );
		if (verbose > 2)
			printf("hw_server: unblocked from receive\n");
		if (-1 == rcvid)
			error_out("MsgReceive failed", errno );
		if (0 == rcvid)
		{
			/* pulse handling code, should it be needed */
			if (verbose)
				printf("got a pulse\n");
			continue;
		}
		if (verbose > 1)
			printf("hw_server: got a message, type:%d expecting: %d or %d\n",
					msg.hdr.type, SEND_DATA, GET_DATA );
		switch (msg.hdr.type)
		{
		case _IO_CONNECT:
			if ( -1 == MsgReply(rcvid, EOK, NULL, 0) )
				perror( "MsgReply" );
			break;
		case SEND_DATA:
			if (verbose)
				printf("started send (high prio)\n");
			hw_out(msg.snd.oplength);
			if (verbose)
				printf("finished send (high prio)\n");
			if ( -1 == MsgReply(rcvid, EOK, NULL, 0) )
				perror( "MsgReply" );
			break;
		case GET_DATA:
			if (verbose)
				printf("started get (low prio)\n");
			hw_in(msg.get.bytes_needed);
			if (verbose)
				printf("finished get (low prio)\n");
			if ( -1 == MsgReply(rcvid, EOK, NULL, 0) )
				perror( "MsgReply" );
			break;
		default:
			if (verbose)
				printf("hwserver: Unexpected message\n");
			if ( -1 == MsgError(rcvid, ENOSYS ) )
				perror( "MsgError" );
			break;
		}
	}
}

/*
 * thread_func
 *
 * just calls mainloop, mainly prototype change,
 * but could do per-thread setup if needed
 */

void * thread_func(void * thread_data)
{
	mainloop();
	return NULL;
}

/*
 * create_threads
 *
 * create the threads, which will go into the mainloop
 */
void create_threads(int num_threads)
{
	int i;
	int ret;

	/* if one thread, become the main loop */
	if (1 == num_threads)
		mainloop();
	else
		for (i = 0; i < num_threads; i++)
		{
			ret = pthread_create(NULL, NULL, thread_func, NULL );
			if (-1 == ret)
				error_out("pthread_create", errno );
		}
}

/*
 * main
 */
int main(int argc, char *argv[])
{
	int opt;
	int num_threads = 4;

	/* parse arguments */
	while ((opt = getopt(argc, argv, "t:v")) != -1)
	{
		switch (opt)
		{
		case 't':
			num_threads = atoi(optarg);
			break;
		case 'v':
			verbose++;
			break;
		}
	}

	if (verbose)
		printf("threads %d, verbositly level %d\n", num_threads, verbose);

	init();

	create_threads(num_threads);

	/* wait to be killed */
	pause();
	return EXIT_FAILURE;
}
