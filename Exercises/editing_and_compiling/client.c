/*
 * client
 *
 * A QNX msg passing client.  It's purpose is to send a string of text to a
 * server.  The server calculates a checksum and replies back with it.  This
 * expects the reply to come back as an int
 *
 * This program program must be started with commandline args.
 * See  if(argc != 4) below
 *
 * To complete the exercise, put in the code, as explained in the comments below
 * Look up function arguments in the course book or the QNX documentation.
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/neutrino.h>
#include <sys/netmgr.h>     //strangely, #define for ND_LOCAL_NODE is in here

/*
 * 	main
 */
int main(int argc, char* argv[])
{
	int coid; //Connection ID to server
	char* outgoing_string; //ptr to string we're sending to server, to make it
	//easy, servers assumes a max of 256 chars!!!!
	int incoming_checksum; //space for sever's reply
	int status; //status return value used for ConnectAttach and MsgSend
	int server_pid; //servers process ID
	int server_chid; //servers channel ID

	if (4 != argc)
	{
		printf(
				"ERROR: This program must be started with commandline arguments, for example:\n\n");
		printf("   cli 482834 1 abcdefghi    \n\n");
		printf(" 1st arg(482834): server's pid\n");
		printf(" 2nd arg(1): server's chid\n");
		printf(" 3rd arg(abcdefghi): string to send to server\n"); //to make it 
		//easy, let's not bother handling spaces
		exit(-1);
	}

	server_pid = atoi(argv[1]);
	server_chid = atoi(argv[2]);

	printf("attempting to establish connection with server pid: %d, chid %d\n",
			server_pid, server_chid);

	coid = ConnectAttach(ND_LOCAL_NODE, server_pid, server_chid,
			_NTO_SIDE_CHANNEL, 0); //PUT CODE HERE to establish a connection to the server's channel
	if (-1 == coid)
	{ //was there an error attaching to server?
		perror("ConnectAttach"); //look up error code and print out
	}

	outgoing_string = argv[3]:
	printf("Sending string: %s\n", outgoing_string);

	status = MsgSend(coid, outgoing_string, strlen(outgoing_string) + 1,
			&incoming_checksum, sizeof(incoming_checksum));//PUT CODE HERE to send message to server and get the reply
	if (-1 == status)
	{ //was there an error sending to server?
		perror("MsgSend");
	}

	printf("received checksum=%d from server\n", incoming_checksum);
	print("MsgSend return status: %d\n", status);

}

