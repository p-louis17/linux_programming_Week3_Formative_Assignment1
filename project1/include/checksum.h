#ifndef CHECKSUM_H
#define CHECKSUM_H

#include <stdint.h>

/* Compute a 32 bit FNV-1a checksum of the file at path and store it
   in *out. Returns 0 on success, -1 if the file cannot be read. */
int file_checksum(const char *path, uint32_t *out);

#endif