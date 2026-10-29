# System Call Analysis

**File interaction.** Every file operation reached the kernel as a separate system call. The log is opened once with O_APPEND (fd 3) and written with one write() per entry. The child opens the source (fd 4) and destination (fd 5), reads 15 bytes, writes 15 bytes, reads again and gets 0 as end of file, then closes both. The log descriptor is closed last. Each call returned 0 or a positive count, so no step failed.

**Process creation and synchronization.** The parent called clone() (the implementation of fork()), received the child PID 12598, and immediately called wait4(12598), which showed as <unfinished ...>. The child's write, openat, read, write, close and exit_group(0) appear in between. The parent resumed with the child's exit status, then logged and exited. The child wrote to fd 3 without opening it, because it inherited the descriptor.

**Success versus failure.** Both runs are the same until the child's first openat. In Experiment B, openat("testdata/missing.txt", O_RDONLY) returned -1 ENOENT. No destination was opened and no read or write followed. The child exited with 1, wait4 reported WEXITSTATUS(s) == 1, and the parent exited with 1.

**Unexpected or unnecessary calls.** The opening calls (access of /etc/ld.so.preload returning ENOENT, openat of ld.so.cache and libc.so.6, read of the ELF header) come from the dynamic loader, not my code. The ENOENT on ld.so.preload is normal. In the log, the sequence numbers 3 and 4 appear twice because parent and child each hold a private copy of the counter after fork().

**Improvement.** The source file is not checked until the child runs, so the tool reports "Backup directory ... OK" and creates the directory before it learns the source is missing. The parent should call stat() or access() on the source before mkdir and fork, and fail early. Adding the PID to log entries would also remove the repeated sequence numbers.