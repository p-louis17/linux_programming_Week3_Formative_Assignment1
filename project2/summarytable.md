| System Call | Purpose | Relevant Arguments | Return value or error | Role in the program |
|---|---|---|---|---|
| openat() | Open or create a file | "backup.log", O_WRONLY\|O_CREAT\|O_APPEND, 0644 | 3 | Opens the log |
| openat() | Open source for reading | "testdata/report.txt", O_RDONLY | 4 (A); -1 ENOENT (B, "testdata/missing.txt") | Child opens the source; fails in B |
| read() | Read bytes from a descriptor | fd 4, buffer, 4096 | 15, then 0 (end of file) | Child reads the source |
| write() | Write bytes to a descriptor | fd 5, "important data\n", 15 | 15 | Child writes the copy; also log lines and console output |
| close() | Release a descriptor | fd 4, 5, 3 | 0 | Frees source, destination and log |
| mkdir() | Create a directory | "backup_out", 0755 | 0 | Creates the backup directory |
| clone() | Create a child process (fork) | flags=CLONE_CHILD_CLEARTID\|CLONE_CHILD_SETTID\|SIGCHLD | 12598 in the parent | Starts the child |
| wait4() | Wait for a child | pid 12598 | 12598; status exit code 0 (A), 1 (B) | Parent blocks until the copy ends |
| exit_group() | End the process | status 0 (A) or 1 (B) | does not return | Child and parent exit |
| execve() | Load a program | "./backup_tool" | 0 | Launch by shell, before main |