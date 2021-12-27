#ifndef _IOV_SERVER_H_
#define _IOV_SERVER_H_

#include <sys/iomsg.h>

#define CKSUM_SERVER_NAME "cksum"
#define CKSUM_IOV_MSG_TYPE (_IO_MAX + 2)

//layout of msg's should always defined by a struct, and ID'd by a msg type 
// number as the first member
typedef struct
{
	uint16_t msg_type;
	unsigned data_size;
} cksum_header_t;

// checksum reply is an int

#endif //_IOV_SERVER_H_
