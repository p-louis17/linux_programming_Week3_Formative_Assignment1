# Project 2: System Call Tables and Analysis

Traces: traceA_success.txt and traceB_failure.txt (strace -f -tt), and traceA2_rerun.txt (strace -f, rerun on an existing directory). Compiled with gcc -std=gnu11 -Wall -Wextra -Iinclude. The names below are the ones this system showed: mkdir appeared as mkdir, and stat() appeared as newfstatat.

## Task 3: File related calls

| Call | Purpose | Relevant arguments | Return | Made by |
|---|---|---|---|---|
| openat | Open or create a file | "backup.log", O_WRONLY\|O_CREAT\|O_APPEND, 0644 | 3 | program (log_open) |
| mkdir | Create the backup directory | "backup_out", 0755 | 0 (A, B), -1 EEXIST (rerun) | program (ensure_dir) |
| openat | Open the source | "testdata/report.txt", O_RDONLY | 4 (A), -1 ENOENT for "testdata/missing.txt" (B) | child (copy_file) |
| openat | Create the destination | "backup_out/report.txt", O_WRONLY\|O_CREAT\|O_TRUNC, 0644 | 5 | child |
| read | Read source bytes | fd 4, buffer, 4096 | 15, then 0 at end of file | child |
| write | Write copy and log lines | fd 5 with 15 bytes, fd 3 for log, fd 1 and 2 for console | 15, and byte counts | child and parent |
| close | Release a descriptor | fd 4, 5, 3 | 0 | program |
| newfstatat | Check an existing path is a directory | AT_FDCWD, "backup_out" | 0, st_mode S_IFDIR | program (stat in ensure_dir, rerun only) |
| access | Test for ld.so.preload | "/etc/ld.so.preload", R_OK | -1 ENOENT | runtime (dynamic loader) |
| openat, read, close, fstat | Load ld.so.cache and libc.so.6 | "/etc/ld.so.cache", "libc.so.6", read of 832 bytes | 3, 832, 0 | runtime |
| execve | Start the program | "./backup_tool" | 0 | shell and strace, not the program |

Excerpts from traceA_success.txt:

    openat(AT_FDCWD, "backup.log", O_WRONLY|O_CREAT|O_APPEND, 0644) = 3
    mkdir("backup_out", 0755) = 0
    openat(AT_FDCWD, "testdata/report.txt", O_RDONLY) = 4
    openat(AT_FDCWD, "backup_out/report.txt", O_WRONLY|O_CREAT|O_TRUNC, 0644) = 5
    read(4, "important data\n", 4096) = 15
    write(5, "important data\n", 15) = 15
    read(4, "", 4096) = 0

From traceA2_rerun.txt:

    mkdir("backup_out", 0755) = -1 EEXIST (File exists)
    newfstatat(AT_FDCWD, "backup_out", {st_mode=S_IFDIR|0755, ...}, 0) = 0

Program calls and runtime calls. Everything before the first openat of backup.log comes from the shell or the dynamic loader (execve, access, openat of ld.so.cache and libc.so.6, fstat). The three fstat calls in the rerun trace were made by the loader and stdio, and only the newfstatat on backup_out comes from the program. fd 3 is reused: the loader closes its descriptor 3 before the program opens backup.log, because the kernel returns the lowest free number.

## Task 4: Process related calls

| Call | Observed | Meaning |
|---|---|---|
| clone | clone(child_stack=NULL, flags=CLONE_CHILD_CLEARTID\|CLONE_CHILD_SETTID\|SIGCHLD, ...) = 12598 | glibc fork() is implemented with clone. The parent gets the child PID |
| wait4 | wait4(12598, <unfinished ...> | waitpid() in the parent, blocks until the child ends |
| exit_group | exit_group(0) in the child, then in the parent | Ends the process with a status |
| wait4 resumed | [{WIFEXITED(s) && WEXITSTATUS(s) == 0}], 0, NULL) = 12598 | Parent wakes with the child's status |
| SIGCHLD | --- SIGCHLD {si_code=CLD_EXITED, si_pid=15862, si_status=0 ...} --- | Kernel notifies the parent that the child exited |

Interaction. The parent logs, creates the directory, then calls clone. It blocks in wait4 immediately. The child does the copy (openat, openat, read, write, read, close, close), writes its log line, and ends with exit_group. The parent then resumes, logs the result and exits. No execve follows the clone, because the child keeps running the same program image. The child writes to fd 3 without opening it, because it inherited the parent's descriptor, and O_APPEND keeps the two processes' log lines from overwriting each other. execve appears only once, for the original launch.

## Task 5: Successful and failed execution

| | Experiment A (valid source) | Experiment B (missing source) |
|---|---|---|
| Source open | openat(..., "testdata/report.txt", O_RDONLY) = 4 | openat(..., "testdata/missing.txt", O_RDONLY) = -1 ENOENT (No such file or directory) |
| Destination open | = 5 | not attempted |
| read and write | 15 bytes read and written | none |
| Child exit | exit_group(0) | exit_group(1) |
| wait4 status | WEXITSTATUS(s) == 0 | WEXITSTATUS(s) == 1 |
| Parent exit | exit_group(0) | exit_group(1) |

Experiment B excerpt:

    openat(AT_FDCWD, "testdata/missing.txt", O_RDONLY) = -1 ENOENT (No such file or directory)
    write(3, "[4] child 12857: copy FAILED: No"..., 56) = 56
    exit_group(1) = ?
    wait4 resumed>[{WIFEXITED(s) && WEXITSTATUS(s) == 1}], 0, NULL) = 12857

The trace shows the cause independently of the printed message: the kernel rejected the first openat with ENOENT, and nothing after it touched the destination. The program handles it properly. The child checks the return value, logs strerror(errno), closes its log descriptor and exits with 1, and the parent decodes the status and exits with 1. Running the tool without strace and printing $? gave 1, which agrees with exit_group(1). One shortcoming is visible: the tool had already printed "Backup directory ... OK" and created the directory, so an empty backup_out remains.

## Task 6: Summary table

| System Call | Purpose | Relevant Arguments | Return value or error | Role in the program |
|---|---|---|---|---|
| openat() | Open or create a file | "backup.log", O_WRONLY\|O_CREAT\|O_APPEND, 0644 | 3 | Opens the log |
| openat() | Open the source | "testdata/report.txt", O_RDONLY | 4 (A), -1 ENOENT (B, missing.txt) | Child opens the source, fails in B |
| read() | Read bytes | fd 4, buffer, 4096 | 15, then 0 | Child reads the source |
| write() | Write bytes | fd 5, "important data\n", 15 | 15 | Child writes the copy, also log and console |
| close() | Release a descriptor | fd 4, 5, 3 | 0 | Frees source, destination, log |
| mkdir() | Create a directory | "backup_out", 0755 | 0, or -1 EEXIST on rerun | Creates the backup directory |
| newfstatat() | Check a path | "backup_out" | 0, S_IFDIR | Accepts an existing directory |
| clone() | Create a child process | flags=CLONE_CHILD_CLEARTID\|CLONE_CHILD_SETTID\|SIGCHLD | 12598 in the parent | Starts the copy process |
| wait4() | Wait for a child | pid 12598 | 12598, status 0 (A) or 1 (B) | Parent blocks until the copy ends |
| exit_group() | End the process | status 0 or 1 | does not return | Child and parent exit |
| execve() | Load a program | "./backup_tool" | 0 | Launch by the shell, before main |

## Task 7: Technical analysis

File interaction. Every file operation reached the kernel as its own system call. The log is opened once with O_APPEND (fd 3) and written with one write per entry. The child opens the source (fd 4) and destination (fd 5), reads 15 bytes, writes 15 bytes, reads again and gets 0 as end of file, then closes both. The log descriptor closes last. Each call returned 0 or a positive count, so no step failed in Experiment A.

Process creation and synchronization. The parent called clone, received PID 12598 and called wait4 at once, shown as <unfinished ...>. The child's calls appear in between, ending with exit_group(0). The line "<... wait4 resumed>" shows the parent waking with the status, and the SIGCHLD record in the rerun trace shows the kernel's notification.

Success versus failure. Both runs are identical until the child's first openat. In B it returned -1 ENOENT, so no destination was opened and no read or write followed. The child exited with 1, wait4 reported WEXITSTATUS(s) == 1, and the parent exited with 1.

Unexpected or unnecessary calls. The opening calls (access of ld.so.preload returning ENOENT, openat of ld.so.cache and libc.so.6, fstat) come from the loader, not from the program, and the ENOENT there is normal. The log contains repeated sequence numbers, 3 and 4 twice, because after fork each process has its own copy of the counter.

Improvement. Check the source with stat() before creating the directory and forking, so a missing source fails early and leaves nothing behind. A second improvement is to write to a temporary name and rename it when the copy finishes, so a failure part way does not leave a partial file. I expect this from reading the code and did not test it.

Limits. Only one failure type was traced. A permission failure would show EACCES, which was not run.