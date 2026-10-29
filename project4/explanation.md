# Signal-Driven Service Controller: Technical Explanation

Compilation: gcc -Wall -Wextra -o server_controller server_controller.c (no warnings).

## Registering and handling signals
main() calls install_handlers(), which uses sigaction() to register one handler, on_signal, for SIGUSR1, SIGUSR2, SIGINT and SIGTERM. sigaction() was chosen over signal() because its behaviour is defined by POSIX and does not vary between systems. The handler does one thing: it sets a volatile sig_atomic_t flag for the signal it received. It calls no printf, malloc or other function that is unsafe inside a handler.

## How flags keep the main loop safe
All printing, state changes and cleanup happen in the main loop, which checks the flags on each pass. The four signals are blocked with sigprocmask() while the loop works, and the loop waits with pselect(), which unblocks them atomically only during the wait. A signal that arrives while the loop is busy stays pending and ends the next wait immediately, so it is never lost and never interrupts a half-finished printf. The loop keeps a monotonic deadline, so an interrupted wait resumes with the remaining time and the five second tick stays steady. In demo_log.txt, ticks stayed about five seconds apart even though signals arrived between them. Flags coalesce: two SIGUSR2 signals delivered before the loop runs would set the flag once, causing one toggle.

## Maintaining and changing state
The state is an enum, ServiceState, held in a local variable in main(). Only the main loop changes it. When got_usr2 is set, the loop clears it and switches between STATE_NORMAL and STATE_MAINTENANCE. The log confirms this: after the first SIGUSR2 the status report showed MAINTENANCE and the next tick said "Service in MAINTENANCE mode...". After the second SIGUSR2 the report showed NORMAL.

## SIGINT versus SIGTERM
SIGINT starts a graceful shutdown. The loop prints a message, removes server_controller.pid with unlink(), prints a completion message and returns EXIT_SUCCESS. The log shows exit status 0 and the PID file removed.

SIGTERM follows the assignment's definition of an emergency termination. The loop prints a message and calls _exit(EXIT_FAILURE) at once, skipping cleanup. The log shows exit status 1 and the PID file still present, which is a stale file that a later start must handle. In many real services SIGTERM is the normal polite shutdown request, so this behaviour is specific to this assignment.

## Limitations
SIGKILL and SIGSTOP cannot be caught, so they bypass all of this. The PID file is written without checking whether another instance is running.