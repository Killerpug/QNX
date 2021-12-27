/*
 *  app_that_uses_so_as_dll
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
 *    app_that_uses_so_as_dll [-ddirectory_to_list]
 * 
 *  Example - get a list of the files in /proc/boot
 * 
 *    app_that_uses_so_as_dll
 * 
 *  Example - get a list of the files in /tmp
 * 
 *    app_that_uses_so_as_dll -d/tmp
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dlfcn.h>

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
	void *hdl;

	int (*rdfn)(char *, char ***);
	int (*sfn)(char **, int);
	int (*dfn)(char **, int);
	int (*cfn)(char **, int);

	progname = argv[0];

	options(argc, argv);

	hdl = dlopen("/tmp/libfiles.so", RTLD_NOW );
	if (NULL == hdl)
	{
		printf("dll open failed because: %s\n", dlerror());
		exit(1);
	}

	rdfn = dlsym(hdl, "read_filenames");
	if (NULL == rdfn)
	{
		printf("dlsym open failed because: %s\n", dlerror());
		exit(1);
	}
	sfn = dlsym(hdl, "sort_filenames");
	if (NULL == rdfn)
	{
		printf("dlsym open failed because: %s\n", dlerror());
		exit(1);
	}
	dfn = dlsym(hdl, "display_filenames");
	if (NULL == rdfn)
	{
		printf("dlsym open failed because: %s\n", dlerror());
		exit(1);
	}
	cfn = dlsym(hdl, "cleanup_filenames");
	if (NULL == rdfn)
	{
		printf("dlsym open failed because: %s\n", dlerror());
		exit(1);
	}
	nfilenames = (*rdfn)(dirname, &filenames);

	if (nfilenames == 0)
		exit(EXIT_FAILURE);

	//sort_filenames (filenames, nfilenames);
	(*sfn)(filenames, nfilenames);
	//display_filenames (filenames, nfilenames);
	(*dfn)(filenames, nfilenames);
	//cleanup_filenames (filenames, nfilenames);
	(*cfn)(filenames, nfilenames);

	return EXIT_SUCCESS;
}

/*
 * 	options
 * This routine handles the command line options.
 *  We support:
 *      -c [name] 	gives the server the name [name]
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

