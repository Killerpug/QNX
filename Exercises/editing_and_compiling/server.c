/*
 * server
 *
 * A QNX msg passing server.  It should receive a string from a client,
 * calculate a checksum on it, and reply back the client with the checksum.
 *
 * The server prints out its pid and chid so the client can be made aware of
 * them.
 *
 * Using the comments below, put code in to complete the program.  Look up
 * function arguments in the course book or the QNX documentation.
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/neutrino.h>
#include <process.h>

int calculate_checksum(char *text);

/*
 * 	main
 */
int main(void)
{
	int chid;
	int pid;
	int rcvid;
	char msg[256]; //To make it easy let's assume a max string of 256 bytes!!!!
	int status;
	int checksum;

	chid = ChannelCreate(0);//PUT CODE HERE to create a channel
	if (-1 == chid)
	{ //was there an error creating the channel?
		perror("ChannelCreate()"); //look up the errno code and print
		exit(EXIT_FAILURE);
	}

	pid = getpid(); //get our own pid
	printf("Server's pid: %d, chid: %d\n", pid, chid); //print our pid/chid so
	//client can be told where to
	//connect

	while (1)
	{
		rcvid = MsgReceive(chid, msg, sizeof(msg), NULL); //PUT CODE HERE to receive msg from client
		if (rcvid == -1)
		{ //was there an error receiving msg?
			perror("MsgReceive"); //look up errno code and print
			break; //try receiving another msg
		};

		checksum = calculate_checksum(msg);

		status = MsgReply(rcvid, EOK, &checksum, sizeof(checksum));//PUT CODE HERE TO reply to client with checksum
		if (-1 == status)
		{
			perror("MsgReply");
		}
	}
	return 0;
}

/*
 * 	calculate_checksum
 *
 * 	This routine simply takes a string and returns an integer based on it.
 */
int calculate_checksum(char *text)
{
	char *c;
	int cksum = 0;

	for (c = text; *c != 0; c++)
		cksum += *c;
	return cksum;
}

