#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include "fileops.h"

#define BUF_SIZE 4096

int ensure_dir(const char *path)
{
    if (mkdir(path, 0755) == 0) {
        return 0;
    }

    if (errno != EEXIST) {
        return -1;
    }

    /* Something with this name exists. Accept it only if it is a directory. */
    struct stat st;
    if (stat(path, &st) != 0) {
        return -1;
    }
    if (!S_ISDIR(st.st_mode)) {
        errno = ENOTDIR;
        return -1;
    }
    return 0;
}

int copy_file(const char *src, const char *dst, size_t *copied)
{
    int in = open(src, O_RDONLY);
    if (in < 0) {
        return -1;
    }

    int out = open(dst, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (out < 0) {
        int saved = errno;
        close(in);
        errno = saved;
        return -1;
    }

    char buf[BUF_SIZE];
    size_t total = 0;
    int result = 0;
    int saved = 0;

    for (;;) {
        ssize_t n = read(in, buf, sizeof(buf));
        if (n < 0) {
            if (errno == EINTR) {
                continue;           /* interrupted, try again */
            }
            saved = errno;
            result = -1;
            break;
        }
        if (n == 0) {
            break;                  /* end of file */
        }

        /* write() may accept fewer bytes than requested, so loop. */
        ssize_t done = 0;
        while (done < n) {
            ssize_t w = write(out, buf + done, (size_t)(n - done));
            if (w < 0) {
                if (errno == EINTR) {
                    continue;
                }
                saved = errno;
                result = -1;
                break;
            }
            done += w;
        }
        if (result != 0) {
            break;
        }
        total += (size_t)n;
    }

    /* Close both descriptors. A failed close of the output can mean
       data was lost, so report it if nothing else failed. */
    close(in);
    if (close(out) != 0 && result == 0) {
        saved = errno;
        result = -1;
    }

    if (result != 0) {
        errno = saved;
        return -1;
    }

    *copied = total;
    return 0;
}