/*
 * 	rbt_server.h
 */

#include <inttypes.h>
#include <sys/iomsg.h>

/*
 * this is the name rbt_server attaches -- redefine to something personalized
 * if sharing a target
 */
#define RBT_SERVER_NAME			"rbt_server"
/* #define RBT_SERVER_NAME			"wally_rbt_server" */

#define RS_MAX_TEXT_LEN			100

#define RS_MSGTYPE_SAY				(_IO_MAX+1) /* _IO_MAX defined in iomsg.h */
#define RS_MSGTYPE_RAISE_LEFT_ARM	(_IO_MAX+2)
#define RS_MSGTYPE_LOWER_LEFT_ARM	(_IO_MAX+3)
#define RS_MSGTYPE_RAISE_RIGHT_ARM	(_IO_MAX+4)
#define RS_MSGTYPE_LOWER_RIGHT_ARM	(_IO_MAX+5)
#define RS_MSGTYPE_EXIT 		    (_IO_MAX+6)

/* message structure for the RS_MSGTYPE_SAY message */
typedef struct
{
	uint16_t type; /* RS_MSGTYPE_SAY */
	char text[RS_MAX_TEXT_LEN + 1];
} rs_msg_say_t;

/* all other RS_MSGTYPE_* messages are simple commands with no data */

/*
 * union of all the possible messages that rbt_server is expecting
 */
typedef union
{
	uint16_t type;
	rs_msg_say_t say;
} rs_msgs_t;

/* replies for all messages is simply EOK in the MsgReply() status argument */
