#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "logger.h"

#define LINE_MAX_LEN 512

/* File descriptor of the log, or -1 when closed. static: private here. */
static int log_fd = -1;

/* Sequence number of the next entry. */
static unsigned int log_seq = 1;

int log_open(const char *path)
{
    /* O_APPEND: every write goes to the end of the file, even when
       the parent and a forked child both write through this fd. */
    log_fd = open(path, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (log_fd < 0) {
        return -1;
    }
    return 0;
}

void log_msg(const char *fmt, ...)
{
    if (log_fd < 0) {
        return;
    }

    char line[LINE_MAX_LEN];

    /* Build the prefix "[n] " first, then append the message after it. */
    int used = snprintf(line, sizeof(line), "[%u] ", log_seq++);
    if (used < 0 || (size_t)used >= sizeof(line)) {
        return;
    }

    va_list args;
    va_start(args, fmt);
    int body = vsnprintf(line + used, sizeof(line) - (size_t)used, fmt, args);
    va_end(args);
    if (body < 0) {
        return;
    }

    size_t len = strlen(line);
    if (len < sizeof(line) - 1) {
        line[len++] = '\n';
    }

    /* One write() per entry, so each log line is one system call. */
    if (write(log_fd, line, len) < 0) {
        /* Logging must never crash the backup; ignore the failure. */
    }
}

void log_close(void)
{
    if (log_fd >= 0) {
        close(log_fd);
        log_fd = -1;
    }
}