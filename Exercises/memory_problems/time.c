/*
 *  time
 *
 *  This module contains the source code for the /dev/time
 *  resource manager.  This illustrates returning data to a client
 *  that is different on a per-device basis.
 *
 *  This module contains all of the functions necessary.
 *
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

/*
 * we are extending the ocb and the attr (Timeocb_s and Timeattr_s are
 * really Timeocb_t and Timeattr_t below)
 */

struct Timeattr_s;
#define IOFUNC_ATTR_T   struct Timeattr_s
struct Timeocb_s;
#define IOFUNC_OCB_T    struct Timeocb_s

#include <sys/iofunc.h>
#include <sys/dispatch.h>
#include <sys/neutrino.h>
#include <string.h>

/*
 *  Define our device attributes structure.
 */

typedef struct Timeattr_s
{
	iofunc_attr_t attr;
	char * format; // output format of each device
	int time_offset; // time offset for this device in seconds
} Timeattr_t;

/*
 *  Define our open context block structure.
 */

typedef struct Timeocb_s
{
	iofunc_ocb_t ocb; // this has 'IOFUNC_ATTR_T *attr' at its top
	// (ocb.offset is the current position
	// in the buffer)
	char *buffer; // the data returned to the client
	int bufsize; // the size of the buffer
} Timeocb_t;

/*
 * Number of devices, and how long or format strings are allowed to be.
 */

#define NUMDEVICES  3

#define MAX_FORMAT_SIZE   64

/*
 *  Declare the tables used by the resource manager.
 */

/* for a shared target, please change the time directory to something
 *  unique.
 */

#define TIME_DIR "/dev/time"
//#define TIME_DIR "/david/time"

//  device names table
char *devnames[NUMDEVICES] = { TIME_DIR "/now", // offset HNow
		TIME_DIR "/hour", // offset HHour
		TIME_DIR "/min" // offset HMin
		};

// time offset initialization array
int init_offsets[NUMDEVICES] = { 0, 3600, -2400 };

//  formats for each device
char formats[NUMDEVICES][MAX_FORMAT_SIZE + 1] = { "%Y %m %d %H:%M:%S\n",
		"%H\n", "%M\n" };

//  pathname ID table
int pathnameID[NUMDEVICES];

//  device information table
Timeattr_t timeattrs[NUMDEVICES];

/*
 *  some forward declarations
 */

void options(int argc, char **argv);

//  I/O functions
int io_read(resmgr_context_t *ctp, io_read_t *msg, Timeocb_t *tocb);
int io_write(resmgr_context_t *ctp, io_write_t *msg, Timeocb_t *tocb);
Timeocb_t *time_ocb_calloc(resmgr_context_t *ctp, Timeattr_t *tattr);
void time_ocb_free(Timeocb_t *tocb);

//  miscellaneous support functions
char *format_time(char *format, int time_offset);
/*
 *  our connect and I/O functions
 */

resmgr_connect_funcs_t connect_funcs;
resmgr_io_funcs_t io_funcs;

/*
 *  our ocb allocating and freeing functions
 */

iofunc_funcs_t time_ocb_funcs = { _IOFUNC_NFUNCS, time_ocb_calloc,
		time_ocb_free };

/*
 *  the mount structure, we only have one so we statically declare it
 */

iofunc_mount_t time_mount = { 0, 0, 0, 0, &time_ocb_funcs };

/*
 *  our dispatch, resource manager and iofunc variables
 */

dispatch_t *dpp;
resmgr_attr_t rattr;
dispatch_context_t *ctp;

/*
 *  some miscellaneous variables
 */

char *progname = "time"; // for diagnostic messages
int optv; // -v for verbose operation

/*
 * 	main
 */
int main(int argc, char **argv)
{
	int i;

	printf("%s:  starting...\n", progname);

	options(argc, argv);

	dpp = dispatch_create();
	if (dpp == NULL)
	{
		perror("dispatch_create");
		exit(-1);
	}
	memset(&rattr, 0, sizeof(rattr));

	/*
	 * initialize the connect functions and I/O functions tables to
	 * their defaults and then override the defaults with the
	 * functions that we are providing.
	 */

	iofunc_func_init(_RESMGR_CONNECT_NFUNCS, &connect_funcs, _RESMGR_IO_NFUNCS,
			&io_funcs);
	io_funcs.read = io_read;
	io_funcs.write = io_write;

	/*
	 *  call resmgr_attach to register our 3 prefixes with the
	 *  process manager, and also to let it know about our connect
	 *  and I/O functions.
	 *
	 *  On error, returns -1 and errno is set.
	 */

	for (i = 0; i < NUMDEVICES; i++)
	{
		if (optv)
		{
			printf("%s:  attaching pathname %s\n", progname, devnames[i]);
		}

		/*
		 *  for this sample program we are using the same mount structure
		 *  for all devices, we are using it solely for providing the
		 *  addresses of our ocb calloc and free functions.
		 */

		iofunc_attr_init(&timeattrs[i].attr, S_IFCHR | 0666, NULL, NULL);
		timeattrs[i].attr.mount = &time_mount;
		timeattrs[i].format = formats[i];
		timeattrs[i].time_offset = init_offsets[i];
		pathnameID[i] = resmgr_attach(dpp, &rattr, devnames[i], _FTYPE_ANY, 0,
				&connect_funcs, &io_funcs, &timeattrs[i]);

		if (pathnameID[i] == -1)
		{
			fprintf(stderr, "%s:  couldn't attach pathname %s, errno %d\n",
					progname, devnames[i], errno);
			exit(1);
		}
	}

	if (optv)
	{
		printf("%s:  entering dispatch loop\n", progname);
	}

	ctp = dispatch_context_alloc(dpp);
	if (ctp == NULL)
	{
		perror("dispatch_context_alloc");
		exit(-1);
	}

	while (1)
	{
		if ((ctp = dispatch_block(ctp)) == NULL)
		{
			fprintf(stderr, "%s:  dispatch_block failed: %s\n", progname,
					strerror(errno));
			exit(1);
		}
		if ( -1 == dispatch_handler(ctp) )
			perror("dispatch_handler");
	}
}

/*
 *  time_ocb_calloc
 *
 *  The purpose of this is to give us a place to allocate our own ocb.
 *  It is called as a result of the open being done
 *  (e.g. iofunc_open_default causes it to be called).  We register
 *  it through the mount structure.
 */

Timeocb_t *time_ocb_calloc(resmgr_context_t *ctp, Timeattr_t *tattr)
{
	Timeocb_t *tocb;

	if (optv)
	{
		printf("%s:  in time_ocb_calloc \n", progname);
	}

	if ((tocb = calloc(1, sizeof(Timeocb_t))) == NULL)
	{
		if (optv)
		{
			printf("%s:  couldn't allocate %ld bytes\n", progname,
					sizeof(Timeocb_t));
		}
		return (NULL);
	}
	// do anything else to our part of the ocb we wish
	tocb -> buffer = NULL;
	return (tocb);
}

/*
 *  time_ocb_free
 *
 *  The purpose of this is to give us a place to free our ocb.
 *  It is called as a result of the close being done
 *  (e.g. iofunc_close_ocb_default causes it to be called).  We register
 *  it through the mount structure.
 */

void time_ocb_free(Timeocb_t *tocb)
{
	if (optv)
	{
		printf("%s:  in time_ocb_free \n", progname);
	}

	free(tocb);
}

/*
 *  io_read
 *
 *  At this point, the client has called their library "read"
 *  function, and expects zero or more bytes.
 *  We are getting the format which we are handling for
 *  this request via the "tocb -> ocb . attr -> format" parameter.
 */

int io_read(resmgr_context_t *ctp, io_read_t *msg, Timeocb_t *tocb)
{
	int nleft;
	int onbytes;
	int status;

	if (optv)
	{
		printf("%s:  in io_read, offset is %ld, nbytes %d\n", progname,
				tocb -> ocb . offset, msg -> i.nbytes);
	}

	if ((status = iofunc_read_verify(ctp, msg, &tocb->ocb, NULL)) != EOK)
		return (status);

	// No special xtypes
	if ((msg->i.xtype & _IO_XTYPE_MASK) != _IO_XTYPE_NONE)
	{
		return (ENOSYS);
	}

	/*
	 *  the first time in, get the time and set up
	 *  the size
	 */

	if (tocb -> buffer == NULL)
	{
		/* format the output based on the format string for the device currently being used */
		tocb -> buffer = format_time(tocb -> ocb.attr -> format,
				tocb -> ocb.attr -> time_offset);
		tocb -> bufsize = strlen(tocb -> buffer) + 1;
	}

	/*
	 *  on all reads (first and subsequent) calculate
	 *  how many bytes we can return to the client,
	 *  based upon the number of bytes available (nleft)
	 *  and the client's buffer size
	 */

	nleft = tocb -> bufsize - tocb -> ocb . offset;
	onbytes = min (msg -> i.nbytes, nleft);

	/*
	 *  do the MsgReply here.  Why are we replying instead of having
	 *  the resmgr API do it?  To show that you can.
	 */

	if (onbytes)
	{
		if ( -1 == MsgReply(ctp -> rcvid, onbytes, tocb -> buffer + tocb -> ocb . offset,
				onbytes) )
			perror("MsgReply");
	} else
	{
		if ( -1 == MsgReply(ctp -> rcvid, 0, NULL, 0) )
			perror("MsgReply");
	}

	/*
	 *  advance the offset to reflect the number
	 *  of bytes returned to the client.
	 */

	tocb -> ocb . offset += onbytes;

	if (msg->i.nbytes > 0)
		tocb->ocb.attr->attr.flags |= IOFUNC_ATTR_ATIME;

	/*
	 *  return _RESMGR_NOREPLY because we've done the
	 *  MsgReply ourselves...
	 */

	return (_RESMGR_NOREPLY);
}

/*
 *  io_write
 *
 *  At this point, the client has called their library "write"
 *  function.  Writing is not defined for /dev/time, so we just
 *  swallow any bytes that they may have written, just like
 *  /dev/null.  An alternate approach is to return an error at
 *  open time if we detect that the device has been opened for
 *  writing.
 */

int io_write(resmgr_context_t *ctp, io_write_t *msg, Timeocb_t *tocb)
{
	int status;

	if (optv)
	{
		printf("%s:  in io_write\n", progname);
	}

	if ((status = iofunc_write_verify(ctp, msg, &tocb->ocb, NULL)) != EOK)
		return (status);

	// No special xtypes
	if ((msg->i.xtype & _IO_XTYPE_MASK) != _IO_XTYPE_NONE)
	{
		return (ENOSYS);
	}

	_IO_SET_WRITE_NBYTES (ctp, msg -> i.nbytes); // indicate how many we wrote

	if (msg->i.nbytes > 0)
		tocb->ocb.attr->attr.flags |= IOFUNC_ATTR_MTIME | IOFUNC_ATTR_CTIME;

	return (_RESMGR_NPARTS (0));
}

/*
 *  format_time()
 *
 * Format a time based on the correct device and time offset
 * value.
 *
 *  We assume that the malloc always works.
 */

char *format_time(char *format, int time_offset)
{
	char *ptr;
	time_t now;

	ptr = malloc(64);
	time(&now);
	now += time_offset;
	strftime(ptr, 64, format, localtime(&now));
	return (ptr);
}

/*
 *  options
 *
 *  This routine handles the command line options.
 *  For our /dev/time family, we support:
 *      -v      verbose operation
 */

void options(int argc, char **argv)
{
	int opt;

	optv = 0;

	while ((opt = getopt(argc, argv, "v")) != -1)
	{
		switch (opt)
		{
		case 'v':
			optv = 1;
			break;
		}
	}
}
