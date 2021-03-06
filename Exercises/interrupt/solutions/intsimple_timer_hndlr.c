/*
 *  intsimple.c
 *
 *  This module demonstrates will contain code for handling an
 *  interrupt.
 *
 *  To test is, simply run it.  Note that you may have to do something
 *  to cause the interrupts to be generated (e.g. press a key if
 *  handling the keyboard interrupt).
 *
 *  A good choice on any system would be the timer:
 *    SYSPAGE_ENTRY(qtime)->intr - timer - interrupts 1000 times/sec, don't print on every interrupt
 *
 *  On an x86_64 box a good choice for the interrupt to use would be:
 *    1 - keyboard
 *
 */

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/neutrino.h>
#include <sys/syspage.h>

char *progname = "intsimple";
struct sigevent int_event; // the event to wake up the thread

const struct sigevent *hdlr(void *blah, int id)
{
	static int count = 0;

	// don't print every interrupt
	if (++count > 1500)
	{
		count = 0;
		return &int_event;
	}
	return NULL;
}

int main(int argc, char **argv)
{
	int id;
	int irq = SYSPAGE_ENTRY(qtime)->intr;

	printf("%s:  starting...\n", progname);

	// set up an event for the handler or the kernel to use to wake up
	// this thread.  Use whatever type of event and event handling you want
	SIGEV_INTR_INIT( &int_event );

	// either register an interrupt handler or the event
	id = InterruptAttach(irq, hdlr, NULL, 0, _NTO_INTR_FLAGS_TRK_MSK );
	if (-1 == id) {
		perror ("InterruptAttach()");
		return EXIT_FAILURE;
	}

	while (1)
	{
		// block here waiting for the event
		InterruptWait(0, NULL );

		// if using a high frequency interrupt, don't print every interrupt -- handled in ISR

		printf("%s:  we got an event after 1500 timer ticks and unblocked\n", progname);
	}
}
