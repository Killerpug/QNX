#include <sys/siginfo.h>
#include <stdint.h>

/* if sharing a target, change this to something unique for you */
#define RECV_NAME "MSG_RECEIVER"

struct notification_request_msg
{
	uint16_t type;
	struct sigevent ev;
};

#define REQUEST_NOTIFICATIONS (_IO_MAX+100)

