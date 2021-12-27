/*
 * cpu_burner
 *
 * process to burn CPU at a mid-level priority, causing the priority
 * inversion that we're going to detect.
 * 
 * -p priority
 * 
 * 
 */ 
  
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/neutrino.h>
#include <pthread.h>
#include <errno.h>
#include <time.h>
#include <string.h>
#include <sys/syspage.h>
#include <inttypes.h>



/*
 * 	error_out
 *
 * 	This routine prints the error and exits
 */
void error_out( char *msg, int error )
{
   printf("cpu_burner: %s, errno %d:%s\n", msg, error, strerror(error) );
   exit(EXIT_FAILURE);
}

/*
 * 	nanospin_clock
 *
 * This routine simply keeps the CPU busy for a designated period of time
 */
void nanospin_clock(unsigned nsec )
{
  uint64_t cps, clock_start, clock_cur;
  
  cps = SYSPAGE_ENTRY(qtime)->cycles_per_sec;

  clock_start = ClockCycles();
  while(1)
  {
    clock_cur = ClockCycles();
    if (clock_cur < clock_start) return; /* wrapped, might happen on SH */
  
    if( (1000000000 * (clock_cur-clock_start)/cps) > nsec ) return; /* done */
  }
}

/*
 * 	calculate
 *
 * 	This simulates a large calculation
 */
void calculate( int value)
{
     nanospin_clock( value*1000 );
}

/*
 * 	main
 */
int main( int argc, char **argv )
{
  
  int opt;
  int priority = 15;
  struct sigevent ev;
  struct itimerspec       itime;
  timer_t                 timer_id;
  int                     chid, coid;
  int                     rcvid;
  struct _pulse           pulse;
  int ret;
  
  /* parse arguments */  
  while (( opt = getopt( argc, argv, "p:" )) != -1 ) 
  {
    switch( opt )
    {
      case 'p':
         priority = atoi( optarg );
         break;
    }
  }
  
  chid = ChannelCreate(0);
  if ( -1 == chid )
	  error_out( "ChannelCreate", errno );

  coid = ConnectAttach(0, 0, chid, _NTO_SIDE_CHANNEL, 0);
  if ( -1 == coid )
	  error_out( "ConnectAttach", errno );
   
  SIGEV_PULSE_INIT( &ev, coid, priority, 0, 0 );
   
  ret = timer_create(CLOCK_REALTIME, &ev, &timer_id);
  if( -1 == ret )
	  error_out( "timer_create", errno );
 
  /* 2 second "start-up time"  */
  itime.it_value.tv_sec = 2;
  itime.it_value.tv_nsec = 0;
  /* run every 13 ticks (or so), given a ticksize of 1ms */ 
  itime.it_interval.tv_sec = 0;
  itime.it_interval.tv_nsec = 13*1000*1000; 
  ret = timer_settime(timer_id, 0, &itime, NULL);
  if( -1 == ret )
	  error_out( "timer_settime", errno );

  while(1)
  {
    rcvid = MsgReceive( chid, &pulse, sizeof(pulse), NULL );
    if( -1 == rcvid )
    	error_out( "MsgReceive", errno );
    if( 0 == rcvid ) /* pulse */
    {
      /* burn 2.1 ms of CPU this tick */
      calculate( 2100 );
      continue;
    }
    printf("cpu_burner: whoa, somebody sent me a message!\n");
    if ( -1 == MsgError( rcvid, ENOSYS ) )
    	error_out( "MsgError", errno );
  }  
}
