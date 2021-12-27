/*
 *  mallocbad
 * 
 *  This is a sample program for demonstrating detecting allocation errors
 *  using the IDE.  This program allocates two things:
 *  1. an array of pointers
 *  2. for each array element, it allocates memory for a string
 * 
 *  The errors are:
 *  1. the amount of memory it allocates for the strings are too short by
 *     1 byte.  No room is allocated for the null terminator.  As a result
 *     when a null terminator is written, it is written past the end of the
 *     malloced memory.
 *  2. the array and all the strings are freed twice.
 * 
 *  The strings are entries that it finds by walking through the /proc/boot
 *  directory (by default).
 * 
 *  Its command line is:
 * 
 *    mallocbad [-ddirectory_to_list]
 * 
 *  Example - get a list of the files in /proc/boot
 * 
 *    mallocbad
 * 
 *  Example - get a list of the files in /tmp
 * 
 *    mallocbad -d/tmp
 * 
 */

#include <errno.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

int read_filenames(char *dirname, char ***tfilenames);
void display_filenames(char **filenames, int nfilenames);
void cleanup_filenames(char **filenames, int nfilenames);
void options(int argc, char *argv[]);

char *dirname = "/proc/boot";
char *progname;

/*
 * 	main
 */
int main(int argc, char *argv[])
{
	int nfilenames;
	char **filenames;

	options(argc, argv);

	nfilenames = read_filenames(dirname, &filenames);
	if ( 0 == nfilenames )
	{
		fprintf(stderr, "%s: Unable to read any files\n", progname);
		exit(EXIT_FAILURE);
	}

	display_filenames(filenames, nfilenames);

	/*
	 * ERROR: cleanup_filenames() is already being called from 
	 * display_filenames().  One of the two should not be done.
	 */
	cleanup_filenames(filenames, nfilenames);

	return EXIT_SUCCESS;
}

/*
 * 	read_filenames
 *
 * 	This routine reads the file names from a given directory
 */
int read_filenames(char *dirname, char ***ifilenames)
{
	char **filenames = NULL, **tmp;
	int nfilenames = 0;
	DIR *dirp;
	struct dirent *direntp;

	/*
	 * open the given directory
	 */
	if ( NULL == (dirp = opendir(dirname)) )
	{
		fprintf(stderr, "%s:  opendir(%s) failed: %s\n", progname, dirname,
				strerror(errno));
		return 0;
	}
	while (1)
	{
		/*
		 * read a directory entry.  'direntp' will point to a structure that
		 * contains 'd_name', the filename
		 */
		direntp = readdir(dirp);
		if (direntp == NULL)
			break;

		/*
		 * make the array of pointers to filenames big enough to fit
		 * one more.  Note that if 'filenames' is NULL, this will
		 * act like a malloc().
		 */
		tmp = realloc(filenames, (nfilenames + 1) * sizeof(char *));
		if (tmp == NULL)
		{
			*ifilenames = filenames;
			return nfilenames;
		}
		filenames = tmp;

		/*
		 * allocate room for the filename and copy it in
		 * ERROR: the code below should be strlen (direntp->d_name)+1
		 */
		filenames[nfilenames] = malloc(strlen(direntp->d_name));
		if ( filenames[nfilenames] == NULL )
		{
			fprintf(stderr, "%s:  malloc failed\n", progname);
			return 0;
		}
		strcpy(filenames[nfilenames], direntp->d_name);

		nfilenames++; /* new number of filenames pointed to by the array */
	}
	if ( closedir(dirp) == -1 )
		perror("closedir");

	*ifilenames = filenames;
	return nfilenames;
}

/*
 * 	display_filenames
 *
 * 	This routine will print the contents of an array containing file names
 */
void display_filenames(char **filenames, int nfilenames)
{
	int i;

	for (i = 0; i < nfilenames; i++)
	{
		printf("%s\n", filenames[i]);
	}
	cleanup_filenames(filenames, nfilenames);
}

/*
 * 	cleanup_filenames
 *
 * 	This routine will free the dynamically created file names from an array
 */
void cleanup_filenames(char **filenames, int nfilenames)
{
	int i;

	for (i = 0; i < nfilenames; i++)
	{
		if (filenames[i] != NULL)
			free(filenames[i]);
	}
	free(filenames);
}

/*
 * 	options
 *
 * 	This routine handles the command line options.
 *  We support:
 *  	-d [directory]	get a list of files from the [directory] directory
 */
void options(int argc, char *argv[])
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

	progname = argv[0];
}
