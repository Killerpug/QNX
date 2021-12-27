/* System Profiling exercise
 * 
 * sys_prof_ex
 * 
 * Compile and run this program, then use the IDE to capture a trace log.
 * The course notes suggest several things to identify within the log.
 *  
 */

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/neutrino.h>
#include <sys/syspage.h>
#include <inttypes.h>
#include <sys/trace.h>
#include <sys/iofunc.h>
#include <sys/dispatch.h>
#include <time.h>
#include <sys/syspage.h>
#include <semaphore.h>

#define PROGNAME "sys_prof_exer: " //prefix for emitted messages
#define TIMER_INTERRUPT_COUNT 500 //num of int's before handler returns event
#define PULSE_CODE_TIMER_EVENT (_PULSE_CODE_MINAVAIL + 1) //when timer goes off
void init_timer(int chid);
void init_workers(void);
const struct sigevent* interrupt_handler(void* area, int id);
void* interrupt_thread(void* data);
void* worker_thread(void* arg);

//to hold an incoming pulse, or, if an unexpected msg arrives, it's msg type
typedef union
{
	short type;
	struct _pulse pulse;
} notify_t; //pulse is used to notify us that some kind event occured

struct sigevent int_event; //interrupt handler periodically causes this event
sem_t thread_sem; //controls number of dispatched of worker threads

/*
 * 	main
 */
int main(int argc, char* argv[])
{
	int chid;
	int rcvid;
	notify_t recv_buf;
	pthread_attr_t thread_attr;
	struct sched_param param;

	printf("System Profiling Demo\n");

	chid = ChannelCreate(_NTO_CHF_DISCONNECT);
	if (-1 == chid)
	{
		perror(PROGNAME "ConnectAttach");
		exit(EXIT_FAILURE);
	}

	init_timer(chid);
	init_workers();

	pthread_attr_init(&thread_attr);
	if ( EOK != pthread_attr_setinheritsched(&thread_attr, PTHREAD_EXPLICIT_SCHED) )
	{
		perror(PROGNAME "pthread_attr_setinheritsched");
		exit(EXIT_FAILURE);
	}
	param.sched_priority = 9;
	if ( EOK != pthread_attr_setschedparam(&thread_attr, &param) )
	{
		perror(PROGNAME "pthread_attr_setschedparam");
		exit(EXIT_FAILURE);
	}
	if ( EOK != pthread_attr_setschedpolicy(&thread_attr, SCHED_NOCHANGE) )
	{
		perror(PROGNAME "pthread_attr_setschedpolicy");
		exit(EXIT_FAILURE);
	}
	if ( EOK != pthread_create(NULL, &thread_attr, interrupt_thread, NULL) )
	{
		perror(PROGNAME "pthread_create");
		exit(EXIT_FAILURE);
	}

	//loop receiving messages(actually pulses) and processing them
	while (1)
	{
		rcvid = MsgReceive(chid, &recv_buf, sizeof(recv_buf), NULL);
		if (-1 == rcvid)
		{
			perror(PROGNAME "MsgReceive");
		} else if (0 == rcvid)
		{
			//we received a pulse,
			//we need to deal with the system pulses appropriately
			switch (recv_buf.pulse.code)
			{
			/* system disconnect pulse */
			case _PULSE_CODE_DISCONNECT:
				ConnectDetach(recv_buf.pulse.scoid);
				printf(PROGNAME "disconnect from a client %X\n",
						recv_buf.pulse.scoid);
				break;
				/* our timer pulse */
			case PULSE_CODE_TIMER_EVENT:
				printf("timer went off\n");
				break;
			default:
				printf(PROGNAME "unexpected pulse code: %d\n",
						recv_buf.pulse.code);
				break;
			}
		} else
		{ //we must have received a message
			MsgError(rcvid, ENOSYS); //tell the client we don't handle msgs
		}
	}
}

/*
 * 	init_workers
 *
 * 	This routine initializes and creates the worker threads
 */
void init_workers(void)
{
	int i;
	pthread_attr_t thread_attr;
	struct sched_param param;

	if ( -1 == (sem_init(&thread_sem, 0, 3)) )
	{
		perror(PROGNAME "sem_init");
		exit(EXIT_FAILURE);
	}

	if (ThreadCtl(_NTO_TCTL_IO, 0) == -1)
	{ //Get permission for nanospin_calibrate()
		perror(PROGNAME "ThreadCtl");
		exit(EXIT_FAILURE);
	}

	if (nanospin_calibrate(1) != EOK)
	{
		perror(PROGNAME "nanospin_calibrate");
	}


	pthread_attr_init(&thread_attr);
	if ( EOK != pthread_attr_setinheritsched(&thread_attr, PTHREAD_EXPLICIT_SCHED) )
	{
			perror(PROGNAME "pthread_attr_setinheritsched");
			exit(EXIT_FAILURE);
	}
	param.sched_priority = 3;
	if ( EOK != pthread_attr_setschedparam(&thread_attr, &param) )
	{
		perror(PROGNAME "pthread_attr_setschedparam");
		exit(EXIT_FAILURE);
	}
	if ( EOK != pthread_attr_setschedpolicy(&thread_attr, SCHED_NOCHANGE) )
		{
			perror(PROGNAME "pthread_attr_setschedpolicy");
			exit(EXIT_FAILURE);
		}

	for (i = 0; i < 5; ++i)
	{
		if ( EOK != pthread_create(NULL, &thread_attr, worker_thread, &thread_sem) )
		{
			perror(PROGNAME "pthread_create");
			exit(EXIT_FAILURE);
		}
	}
}

/*
 * 	worker_thread
 *
 * 	This routine is the "main" for the worker threads. They don't do much
 */
void* worker_thread(void* arg)
{
	struct timespec thread_spin_time;

	thread_spin_time.tv_sec = 0;
	thread_spin_time.tv_nsec = 50000;

	while (1)
	{
		if ( -1 == sem_wait(&thread_sem) )
		{
			perror(PROGNAME "sem_wait");
			exit(EXIT_FAILURE);
		}
		if ( EOK != nanospin(&thread_spin_time) )
		{
			perror(PROGNAME "nanospin");
		}
		//	sem_post(&thread_sem);
	}
	return NULL;
}

/*
 * interrupt_handler
 *
 * this runs on every timer interrupt, but only returns an event
 * every TIMER_INTERRUPT_COUNT events
*/
const struct sigevent* interrupt_handler(void *area, int id)
{
	static int counter = 0;

	// return an event only every X interrupts
	if (++counter == TIMER_INTERRUPT_COUNT)
	{
		counter = 0;
		return &int_event;
	} else
	{
		return NULL;
	}
}

/*
 *	interrupt_thread
 *
 *	attach interrupt handler, then block waiting for it to return an event
 */
void* interrupt_thread(void *data)
{
	if (ThreadCtl(_NTO_TCTL_IO, 0) == -1)
	{ //Get permission to attach to an IRQ
		perror(PROGNAME "ThreadCtl");
		exit(EXIT_FAILURE);
	}

	// Set up a SIGEV_INTR event structure for the ISR to return. The ISR
	// will deliver this event after every X interrupts.  When
	// it does, InterruptWait() will unblock.

	// this macro sets up event structure for a SIGEV_INTR event
	SIGEV_INTR_INIT(&int_event);

	//Read the SYSPAGE to determine the timer interrupt, and attach the 
	//interrupt_handler function to it.
	if (InterruptAttach(SYSPAGE_ENTRY(qtime)->intr, interrupt_handler, NULL, 0,
			_NTO_INTR_FLAGS_TRK_MSK) == -1)
	{
		perror(PROGNAME "InterruptAttach");
		exit(EXIT_FAILURE);
	}

	while (1)
	{
		if ( -1 == InterruptWait(0, NULL) ) //block waiting for interrupt_handler to return a SIGEV_INTR event
		{
			perror( "InterruptWait" );
			exit(EXIT_FAILURE);
			/* Depending on the intended purpose, one may wish to consider when ETIMEDOUT error occurs*/
		}
		printf("%s: %d timer interrupts have occurred\n", PROGNAME,
				TIMER_INTERRUPT_COUNT);
		if ( -1 == sem_post(&thread_sem) )
		{
			perror("sem_post");
			exit(EXIT_FAILURE);
		}
	}
}

/*
 * 	init_timer
 *
 * 	specify timer values, timer event, create timer and start it
 */

void init_timer(int chid)
{
	struct sigevent sigev; //event to occur when timer goes off
	int timerid; //timer ID
	struct itimerspec tspec; //holds timing values
	int self_coid; //connection to the channel within the process

	//connect to the channel within this process
	self_coid = ConnectAttach(0, 0, chid, _NTO_SIDE_CHANNEL, 0);
	if (-1 == self_coid)
	{
		perror(PROGNAME "ConnectAttach");
		exit(EXIT_FAILURE);
	}

	//set up the event struct for pulse event and to send pulse to ourself
	SIGEV_PULSE_INIT(&sigev, self_coid, 6, PULSE_CODE_TIMER_EVENT, 0 );

	//create the timer (doesn't start ticking)
	if (timer_create(CLOCK_REALTIME, &sigev, &timerid) == -1)
	{
		perror(PROGNAME "timer_create");
		exit(EXIT_FAILURE);
	}

	//set up struct within timing values
	tspec.it_value.tv_sec = 1;
	tspec.it_value.tv_nsec = 0;
	tspec.it_interval.tv_sec = 1;
	tspec.it_interval.tv_nsec = 0;

	//start the timer
	if (timer_settime(timerid, 0, &tspec, NULL ) == -1)
	{
		perror(PROGNAME "timer_settime");
		exit(EXIT_FAILURE);
	}
}
