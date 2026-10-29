# Forensic Assessment: data_sync and data_sync_v2

## Purpose and methodology
The objective was to compare two executables described as file synchronization utilities and to assess their likely functionality. Both binaries were built from source I wrote, using gcc -std=c11 -Wall -Wextra -Iinclude. Neither was executed during the investigation, and ldd was not used. The tools were file, readelf, nm and objdump.

## Key findings
Both files are 64 bit little endian x86-64 position-independent executables, dynamically linked with interpreter /lib64/ld-linux-x86-64.so.2. Neither is stripped, and neither contains .debug_* sections. Each has 13 program headers and 31 sections, with the same four LOAD segments (R, R E, R, RW). The dynamic section lists a single dependency, libc.so.6, with BIND_NOW and no RPATH or RUNPATH. main is present in .symtab in both. The entry points are 0x11e0 and 0x1380.

data_sync imports 13 functions through the PLT. They cover directory access (opendir, readdir, closedir), string handling (strcmp, strdup, qsort) and console output. data_sync_v2 imports 26, adding file I/O (fopen, fread, fclose, ferror), time formatting (time, localtime, strftime), file metadata (stat) and extra output functions (vfprintf, fputc, fflush, snprintf).

## Comparative assessment
The structure is identical and the capability differs. data_sync_v2 contains all of the directory and comparison functionality of data_sync, and its imports suggest three extras: timestamped log writing, size and modification time checks, and reading file contents, consistent with checksumming. The extra symbols logger_log, get_file_meta and file_checksum support this reading. The larger .text (0x106e against 0x726), .rodata and .symtab are consistent with the added code. The entry point difference follows from the larger PLT and is not a behavioural signal.

## Conclusion and limitations
Observed evidence: the ELF structure, the symbol names, the import lists, the single libc dependency and the section sizes. Interpretations: that the programs perform synchronization checks, logging and checksumming. Imports show that a function could be called, not that it is. Static inspection cannot confirm which files are read or written, where the log is kept, whether any code path is reachable, or whether the checksum is used for anything beyond comparison. Neither binary imports networking functions such as socket or connect, and this absence is itself only evidence, since code could be loaded another way. These conclusions apply only to binaries compiled from controlled source, and a real unknown binary would also need dynamic analysis in an isolated environment.