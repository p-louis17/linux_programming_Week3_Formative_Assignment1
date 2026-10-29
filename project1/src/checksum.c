#include <stdio.h>

#include "checksum.h"

#define FNV_OFFSET 2166136261u
#define FNV_PRIME  16777619u
#define CHUNK_SIZE 4096

int file_checksum(const char *path, uint32_t *out)
{
    FILE *fp = fopen(path, "rb");
    if (fp == NULL) {
        perror("checksum: fopen");
        return -1;
    }

    uint32_t hash = FNV_OFFSET;
    unsigned char buf[CHUNK_SIZE];
    size_t n;

    /* Read in chunks and fold every byte into the hash. */
    while ((n = fread(buf, 1, sizeof(buf), fp)) > 0) {
        for (size_t i = 0; i < n; i++) {
            hash ^= buf[i];
            hash *= FNV_PRIME;
        }
    }

    int failed = ferror(fp);
    fclose(fp);
    if (failed) {
        fprintf(stderr, "checksum: read error on %s\n", path);
        return -1;
    }

    *out = hash;
    return 0;
}