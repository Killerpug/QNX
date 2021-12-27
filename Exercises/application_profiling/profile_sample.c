/*
 * appprofile_looper
 * 
 * This is a sample for demonstrating the use of the application profiler
 * tool.  It is a very phoney but effective sample (i.e. it does nothing
 * practical but is useful for demonstrating profiling).
 * 
 * If you run it with no command line arguments then it will loop forever.
 * This is useful for demonstrating profiling while it is still running.
 * 
 * To have it loop only once, run it with the -o argument.  This is useful
 * for demonstrating postmortem profiling.
 * 
 */

#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>
#include <sched.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>

// prototypes
void *other_thread(void *arg);
void dofuncs(void);
uint64_t func1(uint64_t n);
uint64_t func2(uint64_t n);
void options(int argc, char **argv);
void app_prof_hdlr(int signo);

int 	loop_once; /* -o to have it loop only once */
int 	optv; /* -v for verbose operation */
struct  sched_param param;
int     policy;
char 	*progname;

/*
 *  main
 */
int main(int argc, char *argv[])
{
	progname = argv[0];

	printf("%s:  starting...\n", progname);
	options(argc, argv);

	// set up signal handler for handling SIGTERM, which will 
	//        exit() and trigger the dumping of profiling data
	signal(SIGTERM, app_prof_hdlr);

	if (-1 == pthread_getschedparam (pthread_self(), &policy, &param))
	{
		perror ("pthread_getschedparam");
	}


	param.sched_priority = 5;  /* run at low priority, 5, so we don't hog CPU */
	if (-1 == pthread_setschedparam (pthread_self(), policy, &param))
	{
		perror ("pthread_setschedparam");
	}


	if ( pthread_create(NULL, NULL, other_thread, NULL) == -1 )
	{
		perror ("pthread_create");
		return EXIT_FAILURE;
	}

	while (1)
	{
		dofuncs();
		if (loop_once)
			break;
	}
	printf("%s:  exiting...\n", progname);

	return EXIT_SUCCESS;
}

/*
 *  other_thread
 *
 *  This function is the "main" routine for a separate thread
 *
 */

void *
other_thread(void *arg)
{
	while (1)
	{
		dofuncs();
		if (loop_once)
			break;
		delay(1);
	}
	return NULL;
}

/*
 *  dofuncs
 *
 *  This routine simply adds more call layers.
 *
 */
void dofuncs(void)
{
	uint64_t count = 1000000; /* larger is better */

	func1(count);
	func2(count);
}

/*
 *  func1
 *
 *  Burn some CPU time.
 *
 */
uint64_t func1(uint64_t n)
{
	volatile uint64_t a, b, c;

	if (optv)
		printf("%s:  Now in func1()\n", progname);
	for (a = 0; a < 2 * n; a++)
	{
	} /* 50% of func1() time */
	for (b = 0; b < n; b++)
	{
	} /* 25% of func1() time */
	for (c = 0; c < n; c++)
	{
	} /* 25% of func1() time */
	return 1;
}

/*
 *  func2
 *
 *  Burn some more CPU time.
 *
 */
uint64_t func2(uint64_t n)
{
	volatile uint64_t d, e;

	if (optv)
		printf("%s:  Now in func2()\n", progname);
	for (d = 0; d < n; d++)
	{
	} /* 12.5% of func2() time */
	for (e = 0; e < 7 * n; e++)
	{
	} /* 87.5% of func2() time */
	return 1;
}

/*
 *  options
 *
 *  This routine handles the command line options.
 *  We support:
 *      -o      loop only once
 *      -v      verbose operation
 */

void options(int argc, char **argv)
{
	int opt;

	optv = 0;
	loop_once = 0;

	while ((opt = getopt(argc, argv, "ov")) != -1)
	{
		switch (opt)
		{
		case 'o':
			loop_once = 1;
			break;
		case 'v':
			optv = 1;
			break;
		}
	}
}

/*
 *  app_prof_hdlr
 *
 *  This routine is a signal handler for SIGTERM, call exit()
 *  to dump application profiling data to a file.
 *
 */
void app_prof_hdlr(int signo)
{
	printf("inside SIGTERM handler, calling exit() to dump profiling info...\n");
	exit(EXIT_SUCCESS);
}
