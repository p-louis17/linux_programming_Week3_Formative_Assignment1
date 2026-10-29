#include <stdio.h>
#include <string.h>

#include "checksum.h"
#include "compare.h"
#include "logger.h"
#include "metadata.h"
#include "scanner.h"

#define PATH_LEN 1024
#define DEFAULT_LOG "sync.log"

/* Build "dir/name" into buf. Returns 0 on success, -1 if it does not fit. */
static int join_path(char *buf, size_t size, const char *dir, const char *name)
{
    int n = snprintf(buf, size, "%s/%s", dir, name);
    return (n < 0 || (size_t)n >= size) ? -1 : 0;
}

/* Linear search: is name present in list? */
static int contains(const FileList *list, const char *name)
{
    for (size_t i = 0; i < list->count; i++) {
        if (strcmp(list->names[i], name) == 0) {
            return 1;
        }
    }
    return 0;
}

/* Report one file that the destination does not have. */
static void report_missing(const char *src_dir, const char *name)
{
    char path[PATH_LEN];
    FileMeta meta;

    if (join_path(path, sizeof(path), src_dir, name) != 0 ||
        get_file_meta(path, &meta) != 0) {
        logger_log("MISSING %s (metadata unavailable)", name);
        printf("  MISSING  %s\n", name);
        return;
    }
    logger_log("MISSING %s (%lld bytes)", name, (long long)meta.size);
    printf("  MISSING  %s (%lld bytes)\n", name, (long long)meta.size);
}

/* Compare a file present in both folders.
   Returns 1 if it differs, 0 if identical or unreadable. */
static int check_changed(const char *src_dir, const char *dst_dir, const char *name)
{
    char sp[PATH_LEN], dp[PATH_LEN];
    FileMeta sm, dm;
    uint32_t sc, dc;

    if (join_path(sp, sizeof(sp), src_dir, name) != 0 ||
        join_path(dp, sizeof(dp), dst_dir, name) != 0) {
        return 0;
    }
    if (get_file_meta(sp, &sm) != 0 || get_file_meta(dp, &dm) != 0) {
        return 0;
    }

    /* Different size means different content, no need to hash. */
    if (sm.size != dm.size) {
        logger_log("CHANGED %s (size %lld vs %lld)", name,
                   (long long)sm.size, (long long)dm.size);
        printf("  CHANGED  %s (size differs)\n", name);
        return 1;
    }

    if (file_checksum(sp, &sc) != 0 || file_checksum(dp, &dc) != 0) {
        return 0;
    }
    if (sc != dc) {
        logger_log("CHANGED %s (checksum %08x vs %08x)", name,
                   (unsigned)sc, (unsigned)dc);
        printf("  CHANGED  %s (checksum differs)\n", name);
        return 1;
    }
    return 0;
}

int main(int argc, char *argv[])
{
    if (argc != 3 && argc != 4) {
        fprintf(stderr, "usage: %s <source_dir> <dest_dir> [log_file]\n", argv[0]);
        return 2;
    }

    const char *log_path = (argc == 4) ? argv[3] : DEFAULT_LOG;
    logger_open(log_path);  /* if this fails we still run, just without a log */

    FileList src, dst, missing;
    filelist_init(&src);
    filelist_init(&dst);
    filelist_init(&missing);

    int status = 1;

    if (scan_directory(argv[1], &src) != 0 ||
        scan_directory(argv[2], &dst) != 0 ||
        find_missing(&src, &dst, &missing) != 0) {
        goto cleanup;
    }

    logger_log("scan %s (%zu files) -> %s (%zu files)",
               argv[1], src.count, argv[2], dst.count);
    printf("Source: %s (%zu files)\n", argv[1], src.count);
    printf("Destination: %s (%zu files)\n", argv[2], dst.count);

    size_t changed = 0;
    printf("Files needing sync:\n");

    for (size_t i = 0; i < missing.count; i++) {
        report_missing(argv[1], missing.names[i]);
    }
    for (size_t i = 0; i < src.count; i++) {
        if (contains(&dst, src.names[i])) {
            changed += check_changed(argv[1], argv[2], src.names[i]);
        }
    }

    printf("Total: %zu missing, %zu changed\n", missing.count, changed);
    logger_log("done: %zu missing, %zu changed", missing.count, changed);
    status = 0;

cleanup:
    filelist_free(&src);
    filelist_free(&dst);
    filelist_free(&missing);
    logger_close();
    return status;
}