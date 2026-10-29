#ifndef COMPARE_H
#define COMPARE_H

#include "scanner.h"

/* Find names that are in src but not in dst. Both lists must be sorted
   (scan_directory guarantees this). The result is appended to out, which
   must already be initialised. Returns 0 on success, -1 on failure. */
int find_missing(const FileList *src, const FileList *dst, FileList *out);

#endif