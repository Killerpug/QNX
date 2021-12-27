/*
 * death_pulse.c
 *
 *
 * Demonstrate requesting and information in the system manager death pulse.
 *
 * No code changes required.
 *
 * Note: this is included as part of the Processes, Threads, and Synchronization module, but uses OS features not taught until the IPC module,
 *       including the creation of channels, and the receiving and interpretation of pulses.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/neutrino.h>
#include <process.h>
#include <sys/procmgr.h>


int main(void)
{
	int chid, coid;
	int rcvid;
	struct _pulse pulse;
	struct sigevent ev;

	// Create a channel that is private to this process -- no other process can connect to it.
	chid = ChannelCreate( _NTO_CHF_PRIVATE );
	if (chid == -1)
	{
		//was there an error creating the channel?
		perror("ChannelCreate()"); //look up the errno code and print
		exit(EXIT_FAILURE);
	}

	// To ask for pulse delivery, we need a connection to our own channel.
	coid = ConnectAttach( 0, 0, chid, _NTO_SIDE_CHANNEL, 0 );
	if (coid == -1)
	{
		//was there an error creating the channel?
		perror("ConnectAttach()"); //look up the errno code and print
		exit(EXIT_FAILURE);
	}

	SIGEV_PULSE_INIT( &ev, coid, 10, 1, 0 );
	SIGEV_MAKE_UPDATEABLE(&ev);  //permit the process manager to modify the pulse before returning it

	// need to register the event to allow delivery
	MsgRegisterEvent(&ev, SYSMGR_COID);

	// request process death notifications
	procmgr_event_notify( PROCMGR_EVENT_PROCESS_DEATH, &ev );

	printf("Waiting for process death notifications (Death Pulse) from the process manager.\n");

	while (1)
	{
		rcvid = MsgReceive( chid, &pulse, sizeof pulse, NULL );
		if (rcvid == -1)
		{
			// there was an error receiving the pulse
			perror("MsgReceive"); //look up errno code and print
			exit(EXIT_FAILURE); //give up
		}

		if( pulse.code == 1 )
		{
			printf("Process with pid: %d died\n", pulse.value.sival_int );
		}
		else
		{
			printf("Unexpected pulse, code: %d\n", pulse.code );
		}
	}
	return EXIT_SUCCESS;
}
