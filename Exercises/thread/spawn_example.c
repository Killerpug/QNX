/*
 * spawn_example.c
 *
 *  Created on: 2012-06-14
 *      Author: dagibbs
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>

void sig_func(int sig )
{
/*  This is a dummy function, just required so we can point
 *  a signal handler at it.  It will never actually be called
 *  since we'll never unmask the SIGCHLD signal.
 */

	pid_t pid;
	int status;
	printf("got sigchld\n");
	pid = wait(&status);
	printf("child died: pid %d, status %d\n", pid, status );
}
int main(int argc, char **argv)
{
	pid_t pid;
	int status;
	sigset_t set;

	/* Setup the signal handling so that if the child dies
	 * before we enter the sigwaitinfo() the signal will still
	 * be held pending.
	 */
	signal( SIGCHLD, sig_func );
	if (-1 == sigemptyset(&set))
	{
		perror("sigemptyset");
		exit(EXIT_FAILURE);
	}
	if (-1 == sigaddset(&set, SIGCHLD ))
	{
		perror("sigaddset");
		exit(EXIT_FAILURE);
	}

	printf("my (parent) pid is %d\n", getpid() );

	pthread_sigmask( SIG_BLOCK, &set, NULL );

	/* launch the child process */
	pid = spawnl(P_NOWAIT, "/system/xbin/sleep", "sleep", "30", NULL );
	if (-1 == pid)
	{
		perror("spawnl()");
		exit(EXIT_FAILURE);
	}

	printf("child pid is %d\n", pid);
	printf("View the process list in the IDE or at the command line.\n");
	printf("In the IDE Target Navigator menu try group->by PID family\n");
	printf("With pidin, try 'pidin family' to get parent/child information.\n");

	/* wait for the SIGCHLD signal, or return immediately if already pending */
	if (-1 == sigwaitinfo(&set, NULL ))
	{
		perror("sigwaitinfo");
		exit(EXIT_FAILURE);
	}

	printf("Child has died, pidin should now show it as a zombie\n");
	sleep(30);

	/* get the status of the dead child and clean up the zombie */
	pid = wait(&status);
	if (-1 == pid)
	{
		perror("wait");
		exit(EXIT_FAILURE);
	}
	printf("child process: %d, died with status %x\n", pid, status);
	printf("Zombie is now gone as we've waited on the child process.\n");

	sleep(30);
	return 0;
}

