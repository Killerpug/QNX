/*
 * 	trace_user_events
 *
 *	This application demonstrates the type of user events that can
 *	be added.
 */

#include <stdlib.h>
#include <stdio.h>
#include <sys/neutrino.h>
#include <sys/trace.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

struct mydata
{
	int type; // 0 - trace, 1 - warn, 2 = error
	int line;
	char source_file[20];
} my_data_a;

/*
 * 	main
 */
int main(int argc, char *argv[])
{
	// name my thread!
	if ( pthread_setname_np( 0, "sample_trace_main") )
		printf( "pthread_setname_np failed\n" );

	printf("Hello World!\n");

	if ( -1 == trace_logi(_NTO_TRACE_USERFIRST, 3, 5) )
		perror("trace_logi");
	if ( -1 ==  trace_logf(_NTO_TRACE_USERFIRST + 1, "Hello world") )
		perror("trace_logf");

	my_data_a.type = 0;
	strcpy(my_data_a.source_file, __FILE__ );
	my_data_a.line = __LINE__;
	if ( -1 ==  trace_logb(_NTO_TRACE_USERFIRST + 2, &my_data_a, sizeof(my_data_a)) )
		perror("trace_logb");

	pause();
	return EXIT_SUCCESS;
}
