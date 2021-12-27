/*
 *  files
 *
 *  This is the second of two files for the listfiles executable.  The other
 *  is main.c.  This one does the filename management side of the work.
 * 
 */

#include <errno.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "files.h"

static char *progname = "sym";

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
	if ((dirp = opendir(dirname)) == NULL)
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
		 */
		filenames[nfilenames] = malloc(strlen(direntp->d_name) + 1);
		strcpy(filenames[nfilenames], direntp->d_name);

		nfilenames++; /* new number of filenames pointed to by the array */
	}
	closedir(dirp);

	*ifilenames = filenames;
	return nfilenames;
}

/*
 * 	sort_filenames
 *
 * 	This routine will sort the array of file names into
 *  alphabetical order.
 */
void sort_filenames(char **filenames, int nfilenames)
{
	char *tmp;
	int i, nswapped;

	if (nfilenames == 1)
		return;

	do
	{
		nswapped = 0;
		for (i = 0; i < nfilenames - 1; i++)
			if (strcmp(filenames[i], filenames[i + 1]) > 0)
			{
				tmp = filenames[i];
				filenames[i] = filenames[i + 1];
				filenames[i + 1] = tmp;
				nswapped++;
			}
	} while (nswapped > 0);
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
