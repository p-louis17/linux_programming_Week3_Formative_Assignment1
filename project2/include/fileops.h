#ifndef FILEOPS_H
#define FILEOPS_H

#include <stddef.h>

/* Make sure path is a directory, creating it if needed.
   Returns 0 on success, -1 on failure with errno set. */
int ensure_dir(const char *path);

/* Copy the file at src to dst (created or truncated).
   On success returns 0 and stores the byte count in *copied.
   On failure returns -1 with errno set. */
int copy_file(const char *src, const char *dst, size_t *copied);

#endif