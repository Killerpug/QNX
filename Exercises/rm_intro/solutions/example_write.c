
/*
 *  example.c
 *
 *  This module contains the source code for the /dev/example device.
 *
 *  If using a shared target, please change EXAMPLE_NAME to something
 *  unique for you, so to avoid testing somebody else's code.
 *
 *
 *  This module contains all of the functions necessary.
 *
*/

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/iofunc.h>
#include <sys/dispatch.h>
#include <sys/neutrino.h>
#include <sys/resmgr.h>

/* default name for this device: /dev/example */

#define EXAMPLE_NAME "/dev/example"

/* change to something else if sharing a target */
// #define EXAMPLE_NAME "/dev/dagexample"

void options (int argc, char *argv[]);

/*
 *  these prototypes are needed since we are using their names in main ()
*/

int io_open (resmgr_context_t *ctp, io_open_t  *msg, RESMGR_HANDLE_T *handle, void *extra);
int io_read (resmgr_context_t *ctp, io_read_t  *msg, RESMGR_OCB_T *ocb);
int io_write(resmgr_context_t *ctp, io_write_t *msg, RESMGR_OCB_T *ocb);

/*
 *  our connect and I/O functions
*/

resmgr_connect_funcs_t  connect_funcs;
resmgr_io_funcs_t       io_funcs;

/*
 *  our dispatch, resource manager and iofunc variables
*/

dispatch_t              *dpp;
resmgr_attr_t           rattr;
dispatch_context_t      *ctp;
iofunc_attr_t           ioattr;

char    *progname = "example";
int     optv;                               // -v for verbose operation

int main (int argc, char *argv[])
{
	int ret;

    printf ("%s:  starting...\n", progname);

    options (argc, argv);

    /*
     * allocate and initialize a dispatch structure for use by our
     * main loop
    */

    dpp = dispatch_create_channel(-1, DISPATCH_FLAG_NOLOCK);
    if (dpp == NULL) {
        fprintf (stderr, "%s:  couldn't dispatch_create: %s\n",
                         progname, strerror (errno));
        exit (EXIT_FAILURE);
    }

    /*
     * set up the resource manager attributes structure, we'll
     * use this as a way of passing information to resmgr_attach().
     * For now, we just use defaults.
    */

    memset (&rattr, 0, sizeof (rattr)); /* using the defaults for rattr */

    /*
     * initialize the connect functions and I/O functions tables to
     * their defaults by calling iofunc_func_init().
     *
     * connect_funcs, and io_funcs variables are already declared.
     *
    */
     iofunc_func_init (_RESMGR_CONNECT_NFUNCS, &connect_funcs,
                      _RESMGR_IO_NFUNCS, &io_funcs);

    /* over-ride the connect_funcs handler for open with our io_open,
     * and over-ride the io_funcs handlers for read and write with our
     * io_read and io_write handlers
     */

    connect_funcs.open = io_open;
    io_funcs.read = io_read;
    io_funcs.write = io_write;


    /* initialize our device description structure
     */

    iofunc_attr_init (&ioattr, S_IFCHR | 0666, NULL, NULL);
    /*
     *  call resmgr_attach to register our prefix with the
     *  process manager, and register our connect and I/O
     *  functions with the resmgr library.
     *
    */
    ret = resmgr_attach (dpp, &rattr, EXAMPLE_NAME, _FTYPE_ANY, 0,
                                     &connect_funcs, &io_funcs, &ioattr);
    if (ret == -1) {
        fprintf (stderr, "%s:  couldn't attach pathname: %s\n",
                         progname, strerror (errno));
        exit (EXIT_FAILURE);
    }

    /*
     * Allocate the dispatch context for this resource manager,
     * this is the receive-loop data.
     */

    ctp = dispatch_context_alloc (dpp);
    if (NULL == ctp )
    {
    	perror( "dispatch_context_alloc");
    	exit( 1 );
    }

    /* our main operation loop */
    while (1) {
    	/* block waiting for client requests */
        if (dispatch_block (ctp) == NULL) {
            fprintf (stderr, "%s:  dispatch_block failed: %s\n",
                             progname, strerror (errno));
            exit (EXIT_FAILURE);
        }

        // for debug purposes it can be useful to look at every message & pulse received
        if (optv > 1) {
                if( ctp->message_context.rcvid == 0 ) {
                    printf("pulse code: %d\n", ctp->message_context.msg->pulse.code );
                } else {
                    printf("message type: %d\n", ctp->message_context.msg->type );
                }
        }

        /* handle client requests, calling out into our handlers where appropriate */
        dispatch_handler (ctp);
    }
}

/*
 *  io_open
 *
 *  We are called here when the client does an open.
 *  Do the default open handling and, if verbose, print.
 *
*/

int
io_open (resmgr_context_t *ctp, io_open_t *msg, RESMGR_HANDLE_T *handle, void *extra)
{
    if (optv) {
        printf ("%s:  in io_open\n", progname);
    }

    return iofunc_open_default (ctp, msg, handle, extra);
}

/*
 *  io_read
 *
 *  At this point, the client has called their library "read"
 *  function, and expects zero or more bytes.  Currently our
 *  /dev/example resource manager returns zero bytes to
 *  indicate EOF -- no more bytes expected.
 *
 *  After our exercises, it will return some data.
*/

int
io_read (resmgr_context_t *ctp, io_read_t *msg, RESMGR_OCB_T *ocb)
{
    int status;
	char data[] = "hello "; // made up data
	size_t nb;
	
    if (optv) {
        printf ("%s:  in io_read\n", progname);
    }

	if ((status = iofunc_read_verify(ctp, msg, ocb, NULL)) != EOK)
        return status;

    // No special xtypes
    if ((msg->i.xtype & _IO_XTYPE_MASK) != _IO_XTYPE_NONE) {
        return ENOSYS;   // causes MsgError( ctp->rcvid, ENOSYS );
    }

    // how much data do we have?
    nb = sizeof(data);

    // we return the lesser of what we have and what the client requested
	nb = min( nb, msg->i.nbytes );

	// copy the data back to the client and tell client's read() to return nb
	MsgReply(ctp->rcvid, nb, data, nb);

	// mark access time for update if any data was read
    if (nb > 0) {
    	ocb->attr->flags |= IOFUNC_ATTR_ATIME;
    }

    return _RESMGR_NOREPLY;
}

/*
 *  io_write
 *
 *  At this point, the client has called their library "write"
 *  function, and expects that our resource manager will write
 *  the number of bytes that they have specified to some device.
 *
 *  Currently, for /dev/example, all of the clients writes always
 *  work -- they just go into Deep Outer Space.
 *
 *  After our updates, they will be displayed on standard out.
*/

int
io_write (resmgr_context_t *ctp, io_write_t *msg, RESMGR_OCB_T *ocb)
{
    int status;
    ssize_t nb;
    char *buf;

    if (optv) {
        printf ("\n\n\n%s:  in io_write, of %d bytes\n\n\n", progname, msg->i.nbytes);
    }

    if ((status = iofunc_write_verify(ctp, msg, ocb, NULL)) != EOK) {
        return status;
    }

    // No special xtypes
    if ((msg->i.xtype & _IO_XTYPE_MASK) != _IO_XTYPE_NONE) {
        return ENOSYS;
    }

    // if they tried to write 0 bytes, tell them it worked, and we're done
    if( msg->i.nbytes == 0 ) {
    	MsgReply(ctp->rcvid, 0, NULL, 0 );
    	return _RESMGR_NOREPLY;
    }

    /* find somewhere to put the data -- in a real driver, this might be a hardware output buffer */
    buf = malloc(msg->i.nbytes);
    if(buf == NULL) {
    	// oops, too big
    	return ENOMEM;
    }

    /* copy the data into our buffer, skipping our header */
    nb = resmgr_msgget(ctp, buf, msg->i.nbytes, sizeof(*msg));

    /* if we failed getting the data, return failure to client */
    if(nb == -1) {
    	free(buf);
    	return errno;
    }

    // dump that data out to stdout (printf would do this, but easier to
	// use write than figure out the correct format string since this
	// isn't null-terminated data)
	status = write( STDOUT_FILENO, buf, nb );
	free(buf);
	if(status == -1) {
		return errno;
	}

	/* unblock the client with the correct number of bytes written */
	MsgReply(ctp->rcvid, nb, NULL, 0);

	// if we actually handled any data, mark that a write was done for
	// time updates (POSIX stuff)
    if (nb > 0) {
    	ocb->attr->flags |= IOFUNC_ATTR_MTIME | IOFUNC_ATTR_CTIME;
    }

    return _RESMGR_NOREPLY;
}

/*
 *  options
 *
 *  This routine handles the command line options.
 *  For our simple /dev/example, we support:
 *      -v      verbose operation
*/

void
options (int argc, char *argv[])
{
    int     opt;

    optv = 0;

    while ((opt = getopt (argc, argv, "v")) != -1) {
        switch (opt) {
        case 'v':
            optv++;
            break;
        }
    }

}
