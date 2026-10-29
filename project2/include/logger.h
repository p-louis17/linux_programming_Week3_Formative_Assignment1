#ifndef LOGGER_H
#define LOGGER_H

/* Open (or create) the log file. Returns 0 on success, -1 on failure
   with errno set by open(). */
int log_open(const char *path);

/* Write one numbered line, printf style: "[1] message".
   Does nothing if the log is not open. */
void log_msg(const char *fmt, ...);

/* Close the log file. Safe to call if it was never opened. */
void log_close(void);

#endif