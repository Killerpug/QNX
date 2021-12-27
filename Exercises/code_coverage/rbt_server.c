/*
 *  rbt_server
 *
 *  This provides a sample for trying out debugging message passing.
 *  This is for use with the rbt_client process.  This will register a name
 *  so that rbt_client can find us.  It will then go into a receive loop
 *  and wait for messages.  As it receives messages, it will handle them
 *  and reply.
 * 
 *  This is pretending to be a process managing the actions of a robot.
 *  The robot pretends to be able to talk and to raise and lower it's left 
 *  and right arms.  To get it to do this, a program called rbt_client
 *  can send it messages.
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
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/dispatch.h>

#include "rbt_server.h"
#include <signal.h>

static void say(int rcvid, char *text);
static void raise_left_arm(int rcvid);
static void lower_left_arm(int rcvid);
static void raise_right_arm(int rcvid);
static void lower_right_arm(int rcvid);
static void handle_pulse(struct _pulse *pulse);

/*
 * rbs_msgs_t is a union of all the types of messages we expect to receive.
 * In our MsgReceive() below, we will need a message buffer that is big
 * enough for our largest expected message since we could receive any of them
 * at any time.  A union is an easy way of doing that.  We expect to receive
 * pulse messages (of type struct _pulse) and the message from rbt_client
 */
typedef union
{
	uint16_t type;
	struct _pulse pulse;
	rs_msgs_t rbt_msg;
} message_buf_t;

#define LOWERED	0
#define RAISED		1

int left_arm_state = LOWERED;
int right_arm_state = LOWERED;

char *progname;

/*
 *  sigexit
 *
 *  This routine is a signal handler to exit when hit by a signal
 *
 */
void sigexit(int sig)
{
	printf("server exiting based on SIGTERM\n");
	exit(0);
}

/*
 * 	main
 */
int main(int argc, char **argv, char **envp)
{
	int rcvid;
	message_buf_t msg;
	name_attach_t *attach;

	progname = argv[0];

	signal(SIGTERM, sigexit);

	/* register the name that rbt_client will look up in order to find us. */
	if ((attach = name_attach(NULL, RBT_SERVER_NAME, 0)) == NULL)
	{
		fprintf(stderr, "%s:  name_attach failed: %s\n", progname, strerror(
				errno));
		exit(EXIT_FAILURE);
	}

	while (1)
	{
		/*
		 * wait for a message.  If there is none already then this will not
		 * return until there is one.
		 */
		rcvid = MsgReceive(attach->chid, &msg, sizeof(msg), NULL);
		if (rcvid == -1)
		{ //was there an error receiving msg?
			if (EINTR == errno )
				continue; // ignore signal interrupt, generally caused by IDE signal to collect data
			perror("MsgReceive"); //look up errno code and print
			break; //failure, give up
		}
		else if (rcvid > 0)
		{ //msg has been received
			switch (msg.type)
			{
			case _IO_CONNECT:
				MsgReply(rcvid, EOK, NULL, 0);
				break;
			case RS_MSGTYPE_SAY:
				say(rcvid, msg.rbt_msg.say.text);
				break;
			case RS_MSGTYPE_RAISE_LEFT_ARM:
				raise_left_arm(rcvid);
				break;
			case RS_MSGTYPE_LOWER_LEFT_ARM:
				lower_left_arm(rcvid);
				break;
			case RS_MSGTYPE_RAISE_RIGHT_ARM:
				raise_right_arm(rcvid);
				break;
			case RS_MSGTYPE_LOWER_RIGHT_ARM:
				lower_right_arm(rcvid);
				break;
			case RS_MSGTYPE_EXIT:
				printf("server exiting from client request\n");
				MsgReply(rcvid, EOK, NULL, 0);
				exit(EXIT_SUCCESS);
				break;
			default:
				// MsgError back to any other msg types
				MsgError(rcvid, ENOSYS);
				break;
			}
		}
		/*
		 * rcvid will be 0 if we received a pulse message.  Our client is not
		 * sending pulse messages so likely a system pulse.
		 */
		else if (rcvid == 0)
		{/* Pulse received */
			handle_pulse(&msg.pulse);
			continue;
		}

	}
	return EXIT_SUCCESS;
}

/*
 * 	say
 *
 * 	This routine pretends we make the robot say the text
 */
static void say(int rcvid, char *text)
{
	printf("%s:  robot said '%s'\n", progname, text);
	if (MsgReply(rcvid, EOK, NULL, 0) == -1)
	{
		fprintf(stderr, "%s:  MsgReply() failed\n", progname);
	}
}

/*
 * 	raise_left_arm
 *
 * 	This routine pretends we make the robot raise its left arm
 */
static void raise_left_arm(int rcvid)
{
	if (left_arm_state == LOWERED)
	{
		printf("%s:  robot raised left arm\n", progname);
		left_arm_state = RAISED;
	} else
		printf("%s:  left arm already raised\n", progname);
	if (MsgReply(rcvid, EOK, NULL, 0) == -1)
	{
		fprintf(stderr, "%s:  MsgReply() failed\n", progname);
	}
}

/*
 * 	lower_left_arm
 *
 * 	This routine pretends we make the robot lower its left arm
 */
static void lower_left_arm(int rcvid)
{
	if (left_arm_state == RAISED)
	{
		printf("%s:  robot lowered left arm\n", progname);
		left_arm_state = LOWERED;
	} else
		printf("%s:  left arm already lowered\n", progname);
	if (MsgReply(rcvid, EOK, NULL, 0) == -1)
	{
		fprintf(stderr, "%s:  MsgReply() failed\n", progname);
	}
}

/*
 * 	raise_right_arm
 *
 * 	This routine pretends we make the robot raise its right arm
 */
static void raise_right_arm(int rcvid)
{
	if (right_arm_state == LOWERED)
	{
		printf("%s:  robot raised right arm\n", progname);
		right_arm_state = RAISED;
	} else
		printf("%s:  right arm already raised\n", progname);
	if (MsgReply(rcvid, EOK, NULL, 0) == -1)
	{
		fprintf(stderr, "%s:  MsgReply() failed\n", progname);
	}
}

/*
 * 	lower_right_arm
 *
 * 	This routine pretends we make the robot lower its right arm
 */
static void lower_right_arm(int rcvid)
{
	if (right_arm_state == RAISED)
	{
		printf("%s:  robot lowered right arm\n", progname);
		right_arm_state = LOWERED;
	} else
		printf("%s:  right arm already lowered\n", progname);
	if (MsgReply(rcvid, EOK, NULL, 0) == -1)
	{
		fprintf(stderr, "%s:  MsgReply() failed\n", progname);
	}
}

/*
 * handle_pulse
 * 
 * Because we did a name_attach(), we can expect the kernel to
 * send us pulse messages under certain circumstances.  See the
 * name_attach() docs for more.
 */
static void handle_pulse(struct _pulse *pulse)
{
	switch (pulse->code)
	{
	case _PULSE_CODE_DISCONNECT:
		/*
		 * a client disconnected all its connections (called
		 * name_close() for each name_open() of our name) or
		 * terminated
		 */
		if(ConnectDetach(pulse->scoid) == -1)
		{
			fprintf(stderr, "%s: ConnectDetach() failed\n", progname);
		}
		break;
	case _PULSE_CODE_UNBLOCK:
		/*
		 * REPLY blocked client wants to unblock (was hit by
		 * a signal or timed out).  It is up to you if you
		 * reply now or later.
		 *
		 */
		// unblock client just in case we made a mistake
		if ( MsgError(pulse->value.sival_int, EINTR) == -1 )
		{
			fprintf(stderr, "%s: MsgError() failed\n", progname);
		}
		printf("%s: unexpected unblock pulse for client %x\n", progname,
				pulse->value.sival_int);
		break;
	case _PULSE_CODE_THREADDEATH:
		/*
		 * a thread in this process died
		 *
		 * does not apply to this process
		 */
		break;
	default:
		/* a pulse sent by one of your processes? */
		fprintf(stderr, "%s:  unexpected pulse, code = 0x%X\n", progname,
				pulse->code);
	}
}
