#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <time.h>
#include <unistd.h>

#define PID_FILE     "server_controller.pid"
#define INTERVAL_SEC 5

typedef enum { STATE_NORMAL, STATE_MAINTENANCE } ServiceState;

/* Flags set by the handlers and read by the main loop.
   sig_atomic_t writes are safe inside a signal handler. */
static volatile sig_atomic_t got_usr1 = 0;
static volatile sig_atomic_t got_usr2 = 0;
static volatile sig_atomic_t got_int  = 0;
static volatile sig_atomic_t got_term = 0;

/* The handler does nothing except record which signal arrived. */
static void on_signal(int signo)
{
    switch (signo) {
    case SIGUSR1: got_usr1 = 1; break;
    case SIGUSR2: got_usr2 = 1; break;
    case SIGINT:  got_int  = 1; break;
    case SIGTERM: got_term = 1; break;
    default: break;
    }
}

/* Register on_signal for all four signals using sigaction(). */
static int install_handlers(void)
{
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = on_signal;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    const int signals[] = { SIGUSR1, SIGUSR2, SIGINT, SIGTERM };
    for (size_t i = 0; i < sizeof(signals) / sizeof(signals[0]); i++) {
        if (sigaction(signals[i], &sa, NULL) != 0) {
            perror("sigaction");
            return -1;
        }
    }
    return 0;
}

static const char *state_name(ServiceState s)
{
    return (s == STATE_NORMAL) ? "NORMAL" : "MAINTENANCE";
}

/* Print one line and flush, so output is visible even through a pipe. */
static void say(const char *msg)
{
    printf("[Server Controller] %s\n", msg);
    fflush(stdout);
}

static int write_pid_file(void)
{
    FILE *fp = fopen(PID_FILE, "w");
    if (fp == NULL) {
        perror("fopen " PID_FILE);
        return -1;
    }
    fprintf(fp, "%d\n", (int)getpid());
    fclose(fp);
    return 0;
}

/* Time left until deadline on the monotonic clock, never negative. */
static struct timespec time_left(const struct timespec *deadline)
{
    struct timespec now, left;
    clock_gettime(CLOCK_MONOTONIC, &now);
    left.tv_sec  = deadline->tv_sec  - now.tv_sec;
    left.tv_nsec = deadline->tv_nsec - now.tv_nsec;
    if (left.tv_nsec < 0) {
        left.tv_sec--;
        left.tv_nsec += 1000000000L;
    }
    if (left.tv_sec < 0) {
        left.tv_sec = 0;
        left.tv_nsec = 0;
    }
    return left;
}

int main(void)
{
    ServiceState state = STATE_NORMAL;

    if (install_handlers() != 0) {
        return EXIT_FAILURE;
    }

    /* Block the four signals while the loop works. pselect() will unblock
       them atomically while it waits, so no signal can slip through. */
    sigset_t block, wait_mask;
    sigemptyset(&block);
    sigaddset(&block, SIGUSR1);
    sigaddset(&block, SIGUSR2);
    sigaddset(&block, SIGINT);
    sigaddset(&block, SIGTERM);
    if (sigprocmask(SIG_BLOCK, &block, &wait_mask) != 0) {
        perror("sigprocmask");
        return EXIT_FAILURE;
    }

    if (write_pid_file() != 0) {
        return EXIT_FAILURE;
    }

    printf("[Server Controller] Started with PID %d, state %s\n",
           (int)getpid(), state_name(state));
    fflush(stdout);

    struct timespec deadline;
    clock_gettime(CLOCK_MONOTONIC, &deadline);
    deadline.tv_sec += INTERVAL_SEC;

    for (;;) {
        /* SIGTERM first: emergency exit, no cleanup. */
        if (got_term) {
            say("EMERGENCY TERMINATION: SIGTERM received, exiting immediately.");
            _exit(EXIT_FAILURE);
        }

        /* SIGINT: graceful shutdown with cleanup. */
        if (got_int) {
            say("Graceful shutdown: SIGINT received, cleaning up...");
            if (unlink(PID_FILE) != 0) {
                perror("unlink " PID_FILE);
            }
            say("Cleanup complete. Goodbye.");
            return EXIT_SUCCESS;
        }

        if (got_usr1) {
            got_usr1 = 0;
            printf("[Server Controller] STATUS REPORT: state=%s, pid=%d\n",
                   state_name(state), (int)getpid());
            fflush(stdout);
        }

        if (got_usr2) {
            got_usr2 = 0;
            state = (state == STATE_NORMAL) ? STATE_MAINTENANCE : STATE_NORMAL;
            printf("[Server Controller] Mode changed to %s\n", state_name(state));
            fflush(stdout);
        }

        /* Wait for the periodic tick, or until a signal interrupts us. */
        struct timespec left = time_left(&deadline);
        if (left.tv_sec == 0 && left.tv_nsec == 0) {
            if (state == STATE_NORMAL) {
                say("Service running normally...");
            } else {
                say("Service in MAINTENANCE mode...");
            }
            deadline.tv_sec += INTERVAL_SEC;   /* keep a steady 5 s rhythm */
            continue;
        }

        if (pselect(0, NULL, NULL, NULL, &left, &wait_mask) < 0 && errno != EINTR) {
            perror("pselect");
            unlink(PID_FILE);
            return EXIT_FAILURE;
        }
    }
}