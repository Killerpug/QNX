/*
 * 	files.h
 */
#ifndef FILES_H_
#define FILES_H_

int read_filenames (char *dirname, char ***tfilenames);
void sort_filenames (char **filenames, int nfilenames);
void display_filenames (char **filenames, int nfilenames);
void cleanup_filenames (char **filenames, int nfilenames);

#endif /*FILES_H_*/
