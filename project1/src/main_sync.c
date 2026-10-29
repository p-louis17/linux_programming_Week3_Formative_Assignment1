#include <stdio.h>

#include "compare.h"
#include "scanner.h"

int main(int argc, char *argv[])
{
    if (argc != 3) {
        fprintf(stderr, "usage: %s <source_dir> <dest_dir>\n", argv[0]);
        return 2;
    }

    FileList src, dst, missing;
    filelist_init(&src);
    filelist_init(&dst);
    filelist_init(&missing);

    int status = 1;

    if (scan_directory(argv[1], &src) != 0) {
        goto cleanup;
    }
    if (scan_directory(argv[2], &dst) != 0) {
        goto cleanup;
    }
    if (find_missing(&src, &dst, &missing) != 0) {
        goto cleanup;
    }

    printf("Source: %s (%zu files)\n", argv[1], src.count);
    printf("Destination: %s (%zu files)\n", argv[2], dst.count);
    printf("Files needing sync: %zu\n", missing.count);
    for (size_t i = 0; i < missing.count; i++) {
        printf("  %s\n", missing.names[i]);
    }
    status = 0;

cleanup:
    filelist_free(&src);
    filelist_free(&dst);
    filelist_free(&missing);
    return status;
}