#include <stdarg.h>
#include <stdio.h>
#include <time.h>

#include "logger.h"

/* The open log file. static means only this file can touch it. */
static FILE *log_file = NULL;

int logger_open(const char *path)
{
    log_file = fopen(path, "a");
    if (log_file == NULL) {
        perror("logger: fopen");
        return -1;
    }
    return 0;
}

void logger_log(const char *fmt, ...)
{
    if (log_file == NULL) {
        return;
    }

    /* Timestamp prefix, e.g. 2026-10-27 14:05:09 */
    char stamp[32];
    time_t now = time(NULL);
    struct tm *local = localtime(&now);
    strftime(stamp, sizeof(stamp), "%Y-%m-%d %H:%M:%S", local);
    fprintf(log_file, "[%s] ", stamp);

    /* Forward the printf style arguments to vfprintf. */
    va_list args;
    va_start(args, fmt);
    vfprintf(log_file, fmt, args);
    va_end(args);

    fputc('\n', log_file);
    fflush(log_file);
}

void logger_close(void)
{
    if (log_file != NULL) {
        fclose(log_file);
        log_file = NULL;
    }
}