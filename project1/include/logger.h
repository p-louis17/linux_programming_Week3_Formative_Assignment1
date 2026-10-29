#ifndef LOGGER_H
#define LOGGER_H

/* Open (or create) the log file in append mode.
   Returns 0 on success, -1 on failure. */
int logger_open(const char *path);

/* Write one timestamped line, printf style. Does nothing if the
   log is not open. */
void logger_log(const char *fmt, ...);

/* Close the log file. Safe to call even if it was never opened. */
void logger_close(void);

#endif