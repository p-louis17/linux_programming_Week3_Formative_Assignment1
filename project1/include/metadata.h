#ifndef METADATA_H
#define METADATA_H

#include <sys/types.h>
#include <time.h>

/* The metadata we care about for one file. */
typedef struct {
    off_t  size;      /* size in bytes */
    time_t modified;  /* last modification time */
} FileMeta;

/* Fill meta for the file at path.
   Returns 0 on success, -1 if the file cannot be examined. */
int get_file_meta(const char *path, FileMeta *meta);

#endif