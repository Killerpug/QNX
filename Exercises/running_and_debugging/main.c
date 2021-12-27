/*
 *  listfiles
 * 
 *  This is a program for exercising basic debugging techniques (examing
 *  variables, setting breakpoints, ...).
 * 
 *  It displays a the names of the files in /proc/boot.  If you wish to
 *  have it display for a different directory, run it with the -d
 *  command line argument.
 *
 *  Note that there are two source files for this executable:
 * 
 *  main.c - This one contains main() and options() (for parsing
 *  command line arguments).
 * 
 *  files.c - This one contains the code for doing the real work - managing
 *  the filenames.
 * 
 *  Its command line is:
 * 
 *    listfiles [-ddirectory_to_list]
 * 
 *  Example - get a list of the files in /proc/boot
 * 
 *    listfiles
 * 
 *  Example - get a list of the files in /tmp
 * 
 *    listfiles -d/tmp
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

extern int read_filenames(char *dirname, char ***tfilenames);
extern void sort_filenames(char **filenames, int nfilenames);
extern void display_filenames(char **filenames, int nfilenames);
extern void cleanup_filenames(char **filenames, int nfilenames);
static void options(int argc, char *argv[]);

static char *dirname = "/proc/boot";
char *progname;

/*
 * 	main
 */
int main(int argc, char *argv[])
{
	int nfilenames;
	char **filenames;

	progname = argv[0];

	options(argc, argv);

	nfilenames = read_filenames(dirname, &filenames);
	if (nfilenames == 0)
		exit(EXIT_FAILURE);

	sort_filenames(filenames, nfilenames);

	display_filenames(filenames, nfilenames);

	cleanup_filenames(filenames, nfilenames);

	return EXIT_SUCCESS;
}

/*
 * 	options
 *
 * 	This routine handles the command line options.
 *  We support:
 *  	-d [directory]	get a list of files from the [directory] directory
 */
static void options(int argc, char *argv[])
{
	int c;

	while ((c = getopt(argc, argv, "d:")) != -1)
	{
		switch (c)
		{
		case 'd':
			dirname = optarg;
			break;
		}
	}
}

