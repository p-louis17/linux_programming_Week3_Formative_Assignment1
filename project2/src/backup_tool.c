#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "fileops.h"
#include "logger.h"

#define PATH_LEN 1024
#define DEFAULT_LOG "backup.log"

/* Return the part of path after the last '/'. */
static const char *base_name(const char *path)
{
    const char *slash = strrchr(path, '/');
    return slash ? slash + 1 : path;
}

/* Work done by the child process. Never returns. */
static void child_copy(const char *src, const char *dst)
{
    size_t copied = 0;

    log_msg("child %d: copying %s to %s", (int)getpid(), src, dst);
    if (copy_file(src, dst, &copied) != 0) {
        int err = errno;
        log_msg("child %d: copy FAILED: %s", (int)getpid(), strerror(err));
        log_close();
        exit(EXIT_FAILURE);
    }

    log_msg("child %d: copy OK, %zu bytes", (int)getpid(), copied);
    log_close();
    exit(EXIT_SUCCESS);
}

int main(int argc, char *argv[])
{
    if (argc != 3 && argc != 4) {
        fprintf(stderr, "usage: %s <source_file> <backup_dir> [log_file]\n",
                argv[0]);
        return 2;
    }

    const char *src = argv[1];
    const char *dir = argv[2];
    const char *log_path = (argc == 4) ? argv[3] : DEFAULT_LOG;

    if (log_open(log_path) != 0) {
        fprintf(stderr, "log: cannot open %s: %s\n", log_path, strerror(errno));
        return 1;
    }
    log_msg("backup started: source=%s dir=%s", src, dir);
    printf("Log file: %s ... OK\n", log_path);

    int status = 1;

    /* Step 1: make sure the backup directory exists. */
    if (ensure_dir(dir) != 0) {
        log_msg("mkdir %s FAILED: %s", dir, strerror(errno));
        fprintf(stderr, "Backup directory %s ... FAILED: %s\n", dir, strerror(errno));
        goto cleanup;
    }
    log_msg("backup directory %s ready", dir);
    printf("Backup directory %s ... OK\n", dir);

    /* Step 2: build the destination path "dir/name". */
    char dst[PATH_LEN];
    int n = snprintf(dst, sizeof(dst), "%s/%s", dir, base_name(src));
    if (n < 0 || (size_t)n >= sizeof(dst)) {
        log_msg("destination path too long");
        fprintf(stderr, "Destination path too long\n");
        goto cleanup;
    }

    /* Step 3: fork. Flush first so the child does not inherit pending output. */
    fflush(stdout);
    pid_t pid = fork();
    if (pid < 0) {
        log_msg("fork FAILED: %s", strerror(errno));
        fprintf(stderr, "fork ... FAILED: %s\n", strerror(errno));
        goto cleanup;
    }
    if (pid == 0) {
        child_copy(src, dst);   /* does not return */
    }

    /* Parent: wait for the child, retrying if a signal interrupts the wait. */
    int wstatus = 0;
    pid_t got;
    do {
        got = waitpid(pid, &wstatus, 0);
    } while (got < 0 && errno == EINTR);

    if (got < 0) {
        log_msg("waitpid FAILED: %s", strerror(errno));
        fprintf(stderr, "waitpid ... FAILED: %s\n", strerror(errno));
        goto cleanup;
    }

    if (WIFEXITED(wstatus) && WEXITSTATUS(wstatus) == 0) {
        log_msg("parent: child %d finished OK", (int)pid);
        printf("Copy %s -> %s ... OK\n", src, dst);
        status = 0;
    } else if (WIFEXITED(wstatus)) {
        log_msg("parent: child %d exited with status %d", (int)pid, WEXITSTATUS(wstatus));
        fprintf(stderr, "Copy %s -> %s ... FAILED (child status %d)\n",
                src, dst, WEXITSTATUS(wstatus));
    } else if (WIFSIGNALED(wstatus)) {
        log_msg("parent: child %d killed by signal %d", (int)pid, WTERMSIG(wstatus));
        fprintf(stderr, "Copy ... FAILED (child killed by signal %d)\n", WTERMSIG(wstatus));
    }

cleanup:
    log_msg("backup finished with status %d", status);
    log_close();
    return status;
}