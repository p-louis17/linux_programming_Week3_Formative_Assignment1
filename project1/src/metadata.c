#define _DEFAULT_SOURCE
#include <stdio.h>
#include <sys/stat.h>

#include "metadata.h"

int get_file_meta(const char *path, FileMeta *meta)
{
    struct stat st;

    if (stat(path, &st) != 0) {
        perror("metadata: stat");
        return -1;
    }

    meta->size = st.st_size;
    meta->modified = st.st_mtime;
    return 0;
}