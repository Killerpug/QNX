/*
 * condvar_queue_example.c
 *
 * Simple demonstration of data provider and hardware handling thread condvar example
 *
 *  Created on: 2013-05-17
 *
 */

#include <pthread.h>
#include <unistd.h>
#include <malloc.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>

#define PROGNAME "condvar_queue"

struct queueNode
{
	struct queueNode* next_ptr;
	int data;
};

typedef struct queueNode queueNode_t;

int* get_data_and_remove_from_queue();
void write_to_hardware (int* data);
void add_to_queue(int data);
int* get_data_and_remove_from_queue();
int add_element_to_queue(int data);
int print_queue();

int q_n_items; // for illustration purposes, current elements in the queue, this is accessed in a potentially thread un-safe manner
queueNode_t* dataQueuep; // Shared resource for two threads
int data_ready;			 // is there data in the queue?

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

void * hardwareHandler(void *);
void * dataProvider(void *);

int main(void)
{
	data_ready=0;
	dataQueuep = NULL;

	/* pthread_mutex_init(mutex, NULL);		Can do this instead of PTHREAD_MUTEX_INITIALIZER
	 * pthread_condvar_init(cond, NULL);	Can do this instead of PTHREAD_MUTEX_INITIALIZER
	 */

	if ((pthread_create(NULL, NULL, hardwareHandler, NULL)) != EOK)
	{
		fprintf(stderr, "%s: hardwareHandler pthread_create failed.\n", PROGNAME);
		exit(EXIT_FAILURE);
	}

	if ((pthread_create(NULL, NULL, dataProvider, NULL)) != EOK)
	{
		fprintf(stderr, "%s: dataProvider pthread_create failed.\n", PROGNAME);
		exit(EXIT_FAILURE);
	}

	sleep(20);
	return 0;
}
/*
 * hardwareHandler
 *
 * Hardware handling thread removes data from the queue and writes to the hardware
 * (in this case /dev/NULL).
 *
 */

void * hardwareHandler(void *i)
{
	int status;
	int *data=NULL;

	while (1)
	{
	  status = pthread_mutex_lock (&mutex);          // get exclusive access
	  if (status!=EOK)
	  {
			fprintf(stderr, "%s: pthread_mutex_lock failed: %s\n", PROGNAME, strerror(status));
			exit(EXIT_FAILURE);
	  }
	  while (!data_ready)
	  {
		    status = pthread_cond_wait (&cond, &mutex);  // we wait here
	  }
	  if (status!=EOK)
	  {
			fprintf(stderr, "%s: pthread_cond_wait failed: %s\n", PROGNAME, strerror(status));
			exit(EXIT_FAILURE);
	  }

	  /* get and decouple data from the queue */
	  while ((data = get_data_and_remove_from_queue ()) != NULL)
	  {
	    status = pthread_mutex_unlock (&mutex);
		if (status!=EOK)
		{
			fprintf(stderr, "%s: pthread_mutex_lock failed: %s\n", PROGNAME, strerror(status));
			exit(EXIT_FAILURE);
		}
	    write_to_hardware (data); // generate output
	    free (data);              // we don't need it after this
	    status = pthread_mutex_lock (&mutex);
		if (status!=EOK)
		{
			fprintf(stderr, "%s: pthread_mutex_lock failed: %s\n", PROGNAME, strerror(status));
			exit(EXIT_FAILURE);
		}
	  }
	  data_ready = 0;               // reset flag
	  status = pthread_mutex_unlock (&mutex);
	  if (status!=EOK)
	  {
			fprintf(stderr, "%s: pthread_mutex_unlock failed: %s\n", PROGNAME, strerror(status));
			exit(EXIT_FAILURE);
	  }
	  /* do further work */
	}
	return NULL;
}

/*
 *  dataProvider
 *
 *  Data provider thread gets some data, likely from a client process, and adds it to a queue.
 *  Sample is intended to add data in bursts.
 *
 */

void * dataProvider(void *unused)
{
	srand(getpid());
	int r_sleep, n_loops, i;

	while(1)
	{
		n_loops = rand() % 10;
		for ( i = 0; i < n_loops; i++ )
		{
			add_to_queue( n_loops * 100 + i );
			sched_yield();  // for demonstration purposes, allow the reader thread a chance to run
		}

		r_sleep = rand() %15;
		delay( r_sleep*100 + 1);

	}
	return NULL;
}

/*
 * add_to_queue
 *
 * Add integer "data" elements to a queue. It is hard-coded as 10 integers just to keep things simple
 */
void add_to_queue(int data)
{
	int status;

	status = pthread_mutex_lock (&mutex);     // get exclusive access
	if (status!=EOK)
	{
		fprintf(stderr, "%s: pthread_mutex_lock failed: %s\n", PROGNAME, strerror(status));
		exit(EXIT_FAILURE);
	}
	add_element_to_queue(data);
	data_ready = 1;                  // set the flag

	status = pthread_mutex_unlock (&mutex);   // release exclusivity
	if (status!=EOK)
	{
		fprintf(stderr, "%s: pthread_unmutex_lock failed: %s\n", PROGNAME, strerror(status));
		exit(EXIT_FAILURE);
	}
	status = pthread_cond_signal (&cond);     // notify a waiter
	if (status!=EOK)
	{
		fprintf(stderr, "%s: pthread_cond_signal failed: %s\n", PROGNAME, strerror(status));
		exit(EXIT_FAILURE);
	}
	return;
}

/*
 * add_element_to_queue
 *
 * Add a data element (integer) to our shared resource (a queue)
 */
int add_element_to_queue(int data)
{
	queueNode_t* newp;
	queueNode_t* endofqueuep=dataQueuep;

	printf("adding data to queue, data: %d\n", data);

	newp = malloc(sizeof(queueNode_t));
	if (newp == NULL)
	{
		errno = ENOMEM;
		exit(EXIT_FAILURE);
	}

	newp->next_ptr = NULL;
	newp->data = data;

	if (dataQueuep == NULL)
	{
		dataQueuep = newp;
	}else{
		while(endofqueuep->next_ptr != NULL)
		{
			endofqueuep = endofqueuep->next_ptr;
		}
		endofqueuep->next_ptr = newp;
	}
	q_n_items++;
	return EOK;
}

/*
 * get_data_and_remove_from_queue
 *
 * Create a new data element (simply an integer in this example) and give it back
 */
int* get_data_and_remove_from_queue()
{
	int* theData=NULL;

	if( dataQueuep )
	{
		// at least one element in the queue

		theData = malloc(sizeof(int)); 	// Make a place to put the data
		if (theData == NULL)
		{
			errno = ENOMEM;
			exit (EXIT_FAILURE);
		}

		// not empty queue, therefore we can pull the top element off.

		queueNode_t* curp=dataQueuep;  	// Get the location of the front of the queue
		printf("removing data from queue, data: %d\n", curp->data);
		*theData=curp->data;			// Get the data

		dataQueuep=dataQueuep->next_ptr; // Move the head of the queue down one spot to remove the element
		free(curp);						// Free the queue element
		q_n_items--;
		//print_queue();
	}

	return theData;
}

/*
 * write_to_hardware
 *
 * Pretend to write the data to some hardware (stdout in this instance)
 * Note: this is intentionally a potentially long operation.
 */
void write_to_hardware (int* data)
{
	char buf[255];

	int ret;

	ret = sprintf( buf, "Data written to hardware: %d, queue length: %d\n", *data, q_n_items ); // unsafe access to q_n_items
	ret = write( STDOUT_FILENO, buf, ret );

	if (-1 == ret)
	{
		perror("write ");
		exit(EXIT_FAILURE);
	}
	delay(2);
}

/*
 * print_queue
 *
 * Display the contents of the queue
 */
int print_queue()
{
	queueNode_t* current_ptr = dataQueuep;

	if (dataQueuep == NULL)
	{
		printf("no data in queue\n");
	}
	else
	{
		printf("queue of data (front to back): \n");

		while (current_ptr != NULL)
		{
			printf("data: %d ", current_ptr->data);
			current_ptr = current_ptr->next_ptr;
		}
	}
	printf("\n");

	return 0;
}

