/*
 * high_prio_client
 *
 * This is meant to be a high priority client for the hardware server.
 * It will do a series of operations of varying lengths on a 3 ms interval,
 * with a default "deadline" of 1 ms for completion, and will count how
 * many are completed, and print a failure message any time it "misses" a
 * deadline.  It will use ClockCycles() to check for missing any deadlines.
 *
 * -d deadline  length for the deadline in milliseconds (default 1, e.g. 0.7 is 0.7 ms)
 * -p prio      priority (default 40)
 * -t           trigger tracing on failure
 * -v           run verbose (cumulative verbosity level)
 * 
 */

 
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/neutrino.h>
#include <pthread.h>
#include <errno.h>
#include <time.h>
#include <sys/dispatch.h>
#include <inttypes.h>
#include <sys/syspage.h>
#include <sys/trace.h>

#include "hw_server.h"
#include <string.h>

/* connection to server */
int server_coid;
int verbose = 0;


/*
 * 	error_out
 *
 * 	This routine prints the error and exits
 */
void error_out( char *msg, int error )
{
   printf("high_prio_client: %s, errno %d:%s\n", msg, error, strerror(error) );
   exit(EXIT_FAILURE);
}


/* do operations of varying lengths, 10 different lengths for now
 * each operation must be done in 2ms, and is timed outside
 * the call to do_work()
 * Each operation will take oplength * 1000 ns of "hw" time, plus any 
 * other overhead or scheduling problems time.
 */
 
#define NUM_OPS 10
int work_array[NUM_OPS] = {50, 100, 75, 190, 100, 200, 25, 50, 100, 180};

/*
 * 	do_work
 *
 * 	This routine requests work from the hardware server
 */
int do_work()
{
  static unsigned op = 0;
  int ret;
  
  struct send_data_msg msg;
  
  msg.hdr.type = SEND_DATA;
  msg.oplength = work_array[op];
  op = (op +1) % 10;
  
  ret = MsgSend(server_coid, &msg, sizeof(msg), NULL, 0 );
  if( -1 == ret ) 
    error_out( "MsgSend to hw_server", errno );
  return msg.oplength; 
}

/*
 *	find_server
 *
 *	This routine opens the hardware server in preparation for IPC
 */
void find_server()
{
  while( 1 )
  {
    server_coid = name_open( HW_SERVER_NAME, 0 );
    if( server_coid != -1)
       break;
    sleep(1);
  }
  if( -1 == server_coid ) 
     error_out("failed to find server: " HW_SERVER_NAME, errno );
  if(verbose) printf("server_coid is %x\n", server_coid);
} 
  
/*
 * 	main
 */
int main( int argc, char *argv[])
{
  int opt;
  int priority = 40;
  struct sigevent ev;
  struct itimerspec       itime;
  timer_t                 timer_id;
  int                     chid, coid;
  int                     rcvid;
  struct _pulse           pulse;
  int ret;
  int trigger = 0;
  unsigned successes = 0;
  uint64_t clock_before, clock_after, cps, clock_delta_ns;
  int oplength;
  float tmpdeadline;
  int deadline;
  
  deadline = 1000*1000; // default to 1 millisecond

  /* parse arguments */  
  while (( opt = getopt( argc, argv, "d:p:vt" )) != -1 )
  {
    switch( opt )
    {
    case 'd':
    	tmpdeadline = atof( optarg ); // expecting it in milliseonds
    	if ( tmpdeadline == 0 )
    		error_out( "Bad value for -d argument", errno);
    	deadline = tmpdeadline * 1000.0 * 1000.0;
    	break;
    case 'p':
        priority = atoi( optarg );
        break;
    case 'v':
        verbose++;
        break;
    case 't':
    	trigger++;
        break;
    }
  }
  
  if(verbose) printf("priority: %d, verbosity level:%d\n", priority, verbose);
  
  find_server();
    
  chid = ChannelCreate(0);

  coid = ConnectAttach(0, 0, chid, _NTO_SIDE_CHANNEL, 0);
   
  SIGEV_PULSE_INIT( &ev, coid, priority, 0, 0 );
   
  ret = timer_create(CLOCK_REALTIME, &ev, &timer_id);
  if( -1 == ret )
	  error_out( "timer_create", errno );
 
  /* 1 second "start-up time"  */
  itime.it_value.tv_sec = 1;
  itime.it_value.tv_nsec = 0;
  /* run every 7 ticks (or so), given default ticksize of 1ms */ 
  itime.it_interval.tv_sec = 0;
  itime.it_interval.tv_nsec = 7*1000*1000; 
  ret = timer_settime(timer_id, 0, &itime, NULL);
  if( -1 == ret )
	  error_out( "timer_settime", errno );

  cps = SYSPAGE_ENTRY(qtime)->cycles_per_sec;
  if(verbose) printf("cycles per second: %ld\n", cps );
  
  while(1)
  {
    rcvid = MsgReceive( chid, &pulse, sizeof(pulse), NULL );
    if( -1 == rcvid )
    	error_out( "MsgReceive", errno );
    if( 0 == rcvid ) /* pulse */
    {
      clock_before = ClockCycles();
      oplength = do_work();
      clock_after = ClockCycles();
      clock_delta_ns = 1000000000 * (clock_after-clock_before)/cps;
      if(verbose) printf("elapsed: %ld ns,  oplength: %d us\n", clock_delta_ns, oplength );
      if( clock_delta_ns < deadline )
      {
         successes++;
         if (verbose && !(successes %100) ) printf("100 successes\n");
      }
      else
      {
        if (trigger )
          TraceEvent( _NTO_TRACE_STOP );
        printf("missed a deadline for %d us operation!, took:%ld\n", oplength, clock_delta_ns);
        printf("Had made %u previous deadlines.\n", successes);
        successes = 0;
      }
      continue;
    }
    if(verbose) printf("high_prio_client: whoa, somebody sent me a message!\n");
    MsgError( rcvid, ENOSYS );
  }  
}


