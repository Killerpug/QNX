/*
 * 	hw_server.h
 */


#include <sys/iomsg.h>

#define HW_SERVER_NAME "HW_SERVER"

struct hw_msg_hdr
{
	short type;
	short subtype;
};

struct send_data_msg
{
	struct hw_msg_hdr hdr;
	unsigned oplength;
/* data follows structure in memory */
};

struct get_data_msg
{
	struct hw_msg_hdr hdr;
	unsigned bytes_needed;
};

typedef union hw_msgs
{
	struct _pulse pulse;
	struct send_data_msg snd;
	struct get_data_msg get;
	struct hw_msg_hdr hdr;
} hw_msgs_t;

/* message types */

/* don't conflict with system messages */
#define MSG_BASE (_IO_MAX + 1 )
#define SEND_DATA MSG_BASE
#define GET_DATA (MSG_BASE + 1)
