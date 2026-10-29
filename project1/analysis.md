# Project 1: Static Analysis of data_sync and data_sync_v2

All commands were run on both binaries by analyze.sh, with output saved in analysis/. Neither binary was executed during the investigation, and ldd was not used. Addresses below come from the first analysis run. The compare_names fix and rebuild may shift some addresses slightly, so check any address you quote against the current files in analysis/.

## 1. Compilation

    gcc -std=c11 -Wall -Wextra -Iinclude -c src/<module>.c -o build/<module>.o
    gcc <objects> -o data_sync            (scanner, compare, main_sync)
    gcc <objects> -o data_sync_v2         (scanner, compare, logger, metadata, checksum, main_sync_v2)

No -g, -s or optimisation flag was used. The full commands are in compile_log.txt.

## 2. What each command is for

| Command | Purpose | What it showed |
|---|---|---|
| file | Quick identification | 64 bit LSB PIE, x86-64, dynamically linked, interpreter /lib64/ld-linux-x86-64.so.2, not stripped, different BuildIDs |
| readelf -h | ELF header | Class, byte order, machine, type, entry point, table sizes |
| readelf -l | Program headers | Segments the loader maps, and the section to segment mapping |
| readelf -S | Section headers | Sizes and flags of .text, .rodata, .data, .bss, .symtab and others |
| readelf -d, objdump -p | Dynamic section | NEEDED libraries, flags, relocation sizes |
| readelf -s, objdump -t | Full symbol table (.symtab) | main and the program's own functions |
| nm -n | Symbols sorted by address | Link order, T and t for text, b and B for bss |
| nm -D, objdump -T | Dynamic symbols (.dynsym) | Imported functions with GLIBC versions |
| objdump -h | Section summary | Same data as readelf -S in another layout |
| objdump -d | Disassembly | Which function calls which import |

## 3. ELF header

| Field | data_sync | data_sync_v2 | Meaning |
|---|---|---|---|
| Class | ELF64 | ELF64 | 64 bit addresses |
| Data | little endian | little endian | Byte order |
| Machine | x86-64 | x86-64 | Target architecture |
| Type | DYN | DYN | Position independent executable |
| Entry point | 0x11e0 | 0x1380 | Address of _start, an offset from the load base |
| Program headers | 13 | 13 | |
| Section headers | 31 | 31 | |

An executable normally has type EXEC, but a PIE is built like a shared object so the loader can place it at a random base address. That is why the entry point is a small offset. At run time the real address is the load base plus this offset. In both binaries the entry point equals the address of .text, so it is the first code in that section, and the kernel hands control to _start, which calls __libc_start_main, which calls main. The entry points differ because v2 has a larger .plt and .plt.sec, which pushes .text later. The difference is layout, not behaviour.

## 4. Program headers and sections

| Segment | Flags | Contains | Purpose |
|---|---|---|---|
| PHDR, INTERP | R | program header table, .interp | Names the dynamic loader |
| LOAD 1 | R | .interp, notes, .gnu.hash, .dynsym, .dynstr, .gnu.version*, .rela.* | Loader metadata |
| LOAD 2 | R E | .init, .plt, .plt.got, .plt.sec, .text, .fini | All executable code |
| LOAD 3 | R | .rodata, .eh_frame_hdr, .eh_frame | Constants and unwind data |
| LOAD 4 | RW | .init_array, .fini_array, .dynamic, .got, .data, .bss | Writable data |
| DYNAMIC | RW | .dynamic | Needed libraries and relocations |
| GNU_STACK | RW | none | Stack is not executable |
| GNU_RELRO | R | .init_array, .fini_array, .dynamic, .got | Made read only after relocation |

Sections present in both: .text, .rodata, .data, .bss, .dynsym, .dynstr, .dynamic, .symtab, .strtab. No .debug_* section exists in either, so no debugging information is available. Sections are not assumed: .plt.sec is present because this toolchain enables control flow protection by default, and a different compiler setup may not produce it.

Sections and segments. Sections are the linker's view: named pieces of the file used by tools such as nm and objdump. Segments are the loader's view: the program headers tell the kernel what to map into memory and with which permissions. Many sections share one segment because they need the same permissions. For example, .init, .plt, .plt.got, .plt.sec, .text and .fini are six sections in one R E segment. Sections .symtab, .strtab, .shstrtab and .comment belong to no segment because they are never loaded, which is why a binary can be stripped without changing how it runs. LOAD 4 has a file size of 0x2b8 and a memory size of 0x2d8 in data_sync, because .bss takes no space in the file but needs memory when loaded.

## 5. Symbols and functions

main is present in .symtab in both binaries (T main at 0x172b and 0x20c3), and absent from nm -D. The dynamic table holds only what the loader needs, and nothing calls main by name. The program's own functions appear as T (global) or t (local). In data_sync_v2 the four static helpers (join_path, contains, report_missing, check_changed) appear as t, and the static variable log_file appears as b in .bss. After the fix, compare_names appears as t in both binaries. Before the fix it appeared as T, which showed that the build did not match the intended source. Symbol evidence can reveal such a difference.

Link order is visible in the addresses: _start, then scanner functions, compare, the v2 only modules (logger, metadata, checksum), and main last.

## 6. Imports and linking

Both binaries are dynamically linked. Evidence: the INTERP segment names /lib64/ld-linux-x86-64.so.2, the dynamic section lists NEEDED libc.so.6, and the symbols carry version tags such as @GLIBC_2.2.5. The only dependency in both is libc.so.6. FLAGS shows BIND_NOW and FLAGS_1 shows NOW and PIE (0x08000001 in objdump -p), so all imports are resolved at startup, and with the GNU_RELRO segment the GOT becomes read only. No RPATH or RUNPATH is present. readelf -d and objdump -p agree, which is a useful cross-check.

| Imported function group | data_sync | data_sync_v2 only |
|---|---|---|
| Directory | opendir, readdir, closedir | |
| Strings and memory | strcmp, strdup, realloc, free, qsort | |
| Output | printf, fprintf, fwrite, perror | puts, fputc, fflush, snprintf, vfprintf |
| File I/O | | fopen, fread, fclose, ferror |
| Time | | time, localtime, strftime |
| Metadata | | stat |
| Runtime | __libc_start_main, __stack_chk_fail | same |

data_sync imports 13 functions through the PLT, and v2 imports 26. The relocation counts agree: .rela.plt is 0x138 bytes (13 entries of 24 bytes) in v1 and 0x270 (26 entries) in v2.

Three terms. An undefined symbol is any symbol an object references but does not define (U in nm). A dynamically imported symbol is an undefined symbol in .dynsym that the loader resolves at run time, shown with a version tag. A shared library dependency is a NEEDED entry in .dynamic. It names a file, not individual functions.

The imports do not match the source line by line. printf("...\n") with no format arguments became puts, and fprintf(stderr, "...\n") became fwrite. The compiler chose cheaper equivalents.

## 7. Why some symbols do not appear in nm

- In a stripped binary, .symtab and .strtab are removed, so plain nm reports no symbols, and main and all function names disappear. nm -D still lists the imports, because the loader needs .dynsym.
- Plain nm reads .symtab and nm -D reads .dynsym, so each shows a different set. main is in the first only.
- A static function that the compiler inlines may have no symbol at all.
- A statically linked binary has no imports, so nm -D shows nothing and the library code appears as ordinary symbols.
- Optimisation levels, link time optimisation and symbol visibility options change which symbols exist and how they are named.

## 8. Where the imports are used

objdump -d of data_sync, matched to the function ranges from nm -n:

| Call | Function |
|---|---|
| strcmp at 0x1499 | compare_names |
| opendir at 0x14bb | scan_directory |
| strcmp at 0x14fd and 0x151b | scan_directory (the "." and ".." checks) |
| readdir at 0x157d | scan_directory |
| strcmp at 0x1694 | find_missing |

Every call goes through a name@plt stub. qsort does not call strcmp directly. It calls compare_names through a function pointer, and compare_names calls strcmp.

## 9. Comparison and explanation

| Property | data_sync | data_sync_v2 |
|---|---|---|
| .text | 0x726 | 0x106e |
| .rodata | 0xbf | 0x297 |
| .bss | 0x10 | 0x18 |
| .dynsym entries | 20 | 33 |
| .symtab entries | 57 | 83 |
| PLT imports | 13 | 26 |

How the causes map to the differences:
- Source changes explain nearly all of it. Three added modules bring more code, strings, symbols and imports.
- The .bss growth of 8 bytes matches the static pointer log_file.
- Optimisation explains why the import list does not mirror the source (puts, fwrite).
- Linking did not cause a difference. Both link the same way against libc only.
- Stripping did not cause a difference. Neither binary is stripped.

## 10. Limits of static evidence

An import shows that a function could be called, not that it is. Static inspection cannot show which files are touched, whether a path is reachable, or what values are used. The binaries were built from controlled source, and a real unknown file would also need dynamic analysis in an isolated environment.