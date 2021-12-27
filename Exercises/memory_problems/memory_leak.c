/*
 *  memory_leak
 * 
 *  This is a simple program that contains a memory leak.  It simply sits in
 *  an infinite loop doing calls to malloc() but never calls free().
 * 
 *  Its command line is:
 *
 *    memory_leak [-v] [-ssize].. [-tsleeptime] [-ffreeiterations]
 *  
 *  Example - if run as follows, it will allocate 16 bytes + 1024 bytes every 
 *  2.5 seconds and free every 3rd iteration
 * 
 *    memory_leak
 * 
 *  Example - if run as follows, it will allocate 400 bytes + 10 bytes +
 *  4000 bytes every .5 seconds (500 milliseconds) and free every 5th iteration.
 *  It will also be verbose and tell you before it does each allocation.
 * 
 *    memory_leak -s400 -s10 -s4000 -t500 -f5 -v
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

void options(int argc, char **argv);

#define MAXNUMSIZES 5
int 	nsizes, freeiter, sizes[MAXNUMSIZES]; /* sizes of mallocs to do */
struct 	timespec sleeptime; /* time to sleep between mallocs in milliseconds */

char *progname;
int verbose = 0;

/*
 * 	main
 */
int main(int argc, char *argv[])
{
	int i, counter;
	void *memptr;

	progname = argv[0];
	counter=0;

	printf("%s:  starting...\n", progname);

	options(argc, argv);

	while (1)
	{
		for (i = 0; i < nsizes; i++)
		{
			if (verbose)
				printf("%s:  +mallocing %d bytes\n", progname, sizes[i]);
			if ( (memptr=malloc(sizes[i])) == NULL )
			{
				fprintf(stderr, "%s:  Out of memory\n", progname);
				exit(EXIT_FAILURE);
			}
			if(freeiter) {
				if ( (++counter % freeiter) == 0)
				{
					counter=0;
					if(verbose)
						printf("%s:  -freeing %d bytes\n", progname, sizes[i]);
					free(memptr);
				}
			}
		}
		if ( nanosleep(&sleeptime, NULL) == -1 )
			perror("nanosleep");
	}

	return EXIT_SUCCESS;
}

/*
 *  options
 *
 *  This routine handles the command line options.
 *  We support:
 *  	-s [int]	number of sizes to leak
 *  	-f [int]    number of iterations to leak until freeing data
 *  	-t [int]	amount of time to sleep
 *  	-v			be verbose
 */

void options(int argc, char **argv)
{
	int opt;

	freeiter = 3;
	nsizes = 0;

	/* default is 2.5 seconds */
	sleeptime.tv_sec = 2;
	sleeptime.tv_nsec = 500000000; /* 500,000,000 nsec = 500 msec = .5 sec */

	while ((opt = getopt(argc, argv, "s:f:t:v")) != -1)
	{
		switch (opt)
		{
		case 's':
			if (nsizes == MAXNUMSIZES)
			{
				fprintf(stderr, "%s:  Too many sizes given.  Maximum is %d\n",
						progname, MAXNUMSIZES);
				exit(EXIT_FAILURE);
			}
			sizes[nsizes] = atoi(optarg);
			nsizes++;
			break;
		case 'f':
			if ( (freeiter = atoi(optarg)) == 1 )
			{
				fprintf(stderr, "%s: Always freeing won't cause a memory leak, use a value > 1 or 0 to never free.\n",
						progname);
				exit(EXIT_FAILURE);
			}
			break;
		case 't':
			sleeptime.tv_sec = atoi(optarg) / 1000;
			sleeptime.tv_nsec = atoi(optarg) % 1000;
			break;
		case 'v':
			verbose = 1;
			break;
		}
	}

	if (nsizes == 0)
	{
		nsizes = 2;
		sizes[0] = 16;
		sizes[1] = 1024;
	}
}
