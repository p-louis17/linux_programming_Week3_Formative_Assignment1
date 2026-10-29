#ifndef SCANNER_H
#define SCANNER_H

#include <stddef.h>

/* A growable list of file names found in a directory. */
typedef struct {
    char  **names;     /* array of heap allocated strings */
    size_t  count;     /* number of names stored */
    size_t  capacity;  /* allocated slots in names */
} FileList;

/* Set up an empty list. Call before first use. */
void filelist_init(FileList *list);

/* Free all names and the array itself. */
void filelist_free(FileList *list);

/* Append a copy of name to the list. Returns 0 on success, -1 on failure. */
int filelist_add(FileList *list, const char *name);

/* Read every entry in dir_path (except "." and "..") into out,
   sorted alphabetically. Returns 0 on success, -1 on failure. */
int scan_directory(const char *dir_path, FileList *out);

#endif