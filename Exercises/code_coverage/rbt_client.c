/*
 *  rbt_client
 *
 *  This provides a sample for trying out debugging message passing.
 *  This is for use with the rbt_server process.  This will do a name lookup 
 *  for the name that rbt_server registers.  It will then send rbt_server
 *  whatever message you tell it to through the command line arguments.
 *  rbt_server will receive the message and reply back with a simple reply.
 * 
 *  Run rbt_server first.  Then run rbt_client with command line arguments
 *  telling it what messages to send.  For example:
 * 
 *   rbt_client -s Hello
 * 
 *  The above will cause rbt_client to send the 'say' message, telling 
 *  rbt_server to make the robot say 'Hello'.
 * 
 *  Other ways of running it are:
 * 
 *   rbt_client -rl       <= raise left arm
 *   rbt_client -ll       <= lower left arm
 *   rbt_client -rr       <= raise right arm
 *   rbt_client -lr       <= lower right arm
 *   rbt_client -x        <= tell server to exit
 * 
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/dispatch.h>

#include "rbt_server.h"

void options(int argc, char **argv);

char *name = RBT_SERVER_NAME; /* the name that we rbt_server registers and
 	 	 	 	 	 	 	 	 that we look up */
rs_msgs_t msg; /* the message we will send to rbt_server, gotten
 	 	 	 	  from the command line */
char *msgdesc; /* message description for diagnostics */
char *progname;

int coid;

/*
 *  main
 */
int main(int argc, char **argv)
{

	struct timespec stime;

	progname = argv[0];

	if (argc < 2)
	{
		fprintf(stderr, "ERROR:  No command was given on command line.\n"
			"use: %s command [command*]\n"
			"Commands:\n"
			"-r [l|r]    Raise left (l) arm or right (r) arm.\n"
			"-l [l|r]    Lower left (l) arm or right (r) arm.\n"
			"-s text     Some text for the robot to say\n"
			" -x         Server exit\n", progname);
	}
	/*
	 * go into a loop, looking for rbt_server.  We loop because rbt_server may
	 * not be running yet.
	 */

	/* fill in our nanosleep time for below */
	stime.tv_sec = 1; /* retry once a second */
	stime.tv_nsec = 0;
	while (1)
	{
		if ((coid = name_open(RBT_SERVER_NAME, 0)) != -1)
		{
			break; /* found it, name_open() succeeded */
		}
		nanosleep(&stime, NULL);
	}

	// parse options, and send messages as appropriate
	options(argc, argv);

	if ( name_close(coid) == -1 )
	{
		fprintf(stderr, "%s: name_close failed\n", progname);
	}

	return EXIT_SUCCESS;
}

/*
 *  options
 *
 *  This routine handles the command line options.
 *  We support:
 *      -n [name] 	gives the server the name [name]
 *      -r[l|r]		[l]ower or [r]aise the right arm
 *      -l[l|r]		[l]ower or [r]aise the left arm
 *      -s [text]	say [text]
 *      -x			exit the server
 */
void options(int argc, char **argv)
{
	int c;

	msg.type = 0;

	while ((c = getopt(argc, argv, "xn:r:l:s:")) != -1)
	{
		switch (c)
		{
		case 'n':
			name = optarg;
			break;
		case 'r':
			switch (*optarg)
			{
			case 'l':
				msg.type = RS_MSGTYPE_RAISE_LEFT_ARM;
				msgdesc = "raise left arm";
				break;
			case 'r':
				msg.type = RS_MSGTYPE_RAISE_RIGHT_ARM;
				msgdesc = "raise right arm";
				break;
			}
			break;
		case 'l':
			switch (*optarg)
			{
			case 'l':
				msg.type = RS_MSGTYPE_LOWER_LEFT_ARM;
				msgdesc = "lower left arm";
				break;
			case 'r':
				msg.type = RS_MSGTYPE_LOWER_RIGHT_ARM;
				msgdesc = "lower right arm";
				break;
			}
			break;
		case 's':
			msg.type = RS_MSGTYPE_SAY;
			strncpy(msg.say.text, optarg, RS_MAX_TEXT_LEN);
			msg.say.text[RS_MAX_TEXT_LEN] = '\0';
			msgdesc = "say";
			break;
		case 'x':
			msg.type = RS_MSGTYPE_EXIT;
			msgdesc = "exit";
			break;
		}
		/* send message to rbt_server.  */
		if (MsgSend(coid, &msg, sizeof(msg), NULL, 0) == -1)
		{
			fprintf(stderr, "%s:  MsgSend() failed: %s (%d)\n", progname,
					strerror(errno), errno);
			exit(EXIT_FAILURE);
		}
		printf("%s:  sent '%s' message\n", progname, msgdesc);
	}

	if (msg.type == 0)
	{
		fprintf(stderr, "ERROR:  No valid command was given on command line.\n"
			"use: %s command [command*]\n"
			"Commands:\n"
			"-r [l|r]    Raise left (l) arm or right (r) arm.\n"
			"-l [l|r]    Lower left (l) arm or right (r) arm.\n"
			"-s text     Some text for the robot to say\n"
			" -x         Server exit\n", progname);
		exit(EXIT_FAILURE);
	}
}
