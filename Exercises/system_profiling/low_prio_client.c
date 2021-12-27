/*
 * low_prio_client
 *
 * This is meant to be a low priority client for the hardware server.
 * It will do a series of operations of varying lengths on a 4 ms interval,
 * 
 * -p priority (default 10)
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
void error_out(char *msg, int error)
{
	printf("low_prio_client: %s, errno %d:%s\n", msg, error, strerror(error));
	exit(EXIT_FAILURE);
}

/* do operations of varying lengths, 10 different lengths for now
 * 
 * Each operation will take oplength * 1000 ns of "hw" time, plus any 
 * other overhead or scheduling problems time.
 */

#define NUM_OPS 10
int
		work_array[NUM_OPS] = { 500, 750, 1000, 350, 200, 1100, 700, 150, 450,
				1050 };

/*
 * 	do_work
 *
 * 	This routine requests work from the hardware server
 */
void do_work()
{
	static int op = 0;
	int ret;

	struct get_data_msg msg;

	msg.hdr.type = GET_DATA;
	msg.bytes_needed = work_array[op];
	op++;
	op %= 10;
	ret = MsgSend(server_coid, &msg, sizeof(msg), NULL, 0);
	if (-1 == ret)
		error_out("MsgSend to hw_server", errno );
}

/*
 *	find_server
 *
 *	This routine opens the hardware server in preparation for IPC
 */
void find_server()
{
	while (1)
	{
		server_coid = name_open(HW_SERVER_NAME, 0);
		if (server_coid != -1)
			break;
		sleep(1);
	}
	if (-1 == server_coid)
		error_out("failed to find server: " HW_SERVER_NAME, errno );
}

/*
 * 	main
 */
int main(int argc, char *argv[])
{
	int opt;
	int priority = 11;
	struct sigevent ev;
	struct itimerspec itime;
	timer_t timer_id;
	int chid, coid;
	int rcvid;
	struct _pulse pulse;
	int ret;

	/* parse arguments */
	while ((opt = getopt(argc, argv, "p:")) != -1)
	{
		switch (opt)
		{
		case 'p':
			priority = atoi(optarg);
			break;
		case 'v':
			verbose++;
			break;
		}
	}

	if (verbose)
		printf("priority: %d, verbosity level:%d\n", priority, verbose);

	chid = ChannelCreate(0);
	if ( -1 == chid )
		error_out( "ChannelCreate", errno );

	find_server();

	coid = ConnectAttach(0, 0, chid, _NTO_SIDE_CHANNEL, 0);
	if ( -1 == coid )
		error_out( "ConnectAttach", errno );

	SIGEV_PULSE_INIT( &ev, coid, priority, 0, 0 );

	ret = timer_create(CLOCK_REALTIME, &ev, &timer_id);
	if (-1 == ret)
		error_out("timer_create", errno );

	/* 1 second "start-up time"  */
	itime.it_value.tv_sec = 1;
	itime.it_value.tv_nsec = 0;
	/* run every 11 ticks (or so), given a ticksize of 1ms */
	itime.it_interval.tv_sec = 0;
	itime.it_interval.tv_nsec = 11 * 1000 * 1000;
	ret = timer_settime(timer_id, 0, &itime, NULL);
	if (-1 == ret)
		error_out("timer_settime", errno );

	while (1)
	{
		rcvid = MsgReceive(chid, &pulse, sizeof(pulse), NULL );
		if (-1 == rcvid)
			error_out( "MsgReceive", errno );
		if (0 == rcvid) /* pulse, assume from timer */
		{
			do_work();
			continue;
		}
		if (verbose)
			printf("low_prio_client: whoa, somebody sent me a message!\n");
		if ( -1 == MsgError(rcvid, ENOSYS ) )
			error_out( "MsgError", errno );
	}
}
