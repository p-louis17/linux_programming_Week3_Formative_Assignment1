# Week 3 Formative Assignment

Four projects on Linux internals: ELF forensics, system call tracing, a CPython C extension, and a signal driven service. This file explains how to build and run everything. The combined written report is the exported document `LouisPascal_Week3_Formative_Assignment1.pdf` (or `.pdf`) in the root folder.

## Repository layout

```
Week3_Formative_Assignment1/
  README.md
  LouisPascal_Week3_Formative_Assignment1.pdf
  project1/   ELF forensics (data_sync, data_sync_v2)
  project2/   backup_tool and strace traces
  project3/   faststats C extension, application, tests, benchmark
  project4/   server_controller signal service
```

Run every command from inside the project folder named in its section. Several commands use relative paths and fail from another directory.

## Requirements

* Linux x86-64 with `gcc`, `make`, `binutils` (readelf, nm, objdump) and `file`
* `strace` (Project 2)
* Python 3.12 with headers and setuptools (Project 3)
* `matplotlib` (Project 3, only for the graph)

Install on Ubuntu or Debian:

```
sudo apt install build-essential strace python3-dev python3-setuptools python3-matplotlib
```

Makefile recipe lines must start with a real tab character, not spaces.

## Project 1: ELF forensics

Layout: `include/`, `src/`, `Makefile`, `analyze.sh`.

Build both programs, recording the compile commands:

```
cd project1
make clean && make 2>&1 | tee compile_log.txt
```

Optional functional test. This runs the programs, which is fine because they are your own source:

```
mkdir -p test/src test/dst
touch test/src/a.txt test/src/c.txt test/dst/e.txt
echo "hello" > test/src/b.txt
echo "hello world" > test/dst/b.txt
echo "abcd" > test/src/d.txt
echo "abce" > test/dst/d.txt
./data_sync test/src test/dst
./data_sync_v2 test/src test/dst
cat sync.log
```

Static analysis. This runs `file`, `readelf`, `nm` and `objdump` on both binaries and writes 26 raw output files to `analysis/`. It never executes the binaries and never uses `ldd`:

```
sh analyze.sh
```

Write-ups:
* `analysis.md`: what each command shows, ELF header, segments and sections, symbols, imports and linking, and the comparison explained.
* `comparison_table.md`: the Task 5 table.
* `forensic_report.md`: the one page forensic report (Task 6).

If you change any source, run `make clean && make` before `analyze.sh`, so the evidence matches the code. Addresses quoted in `analysis.md` then need checking against the new files in `analysis/`.

## Project 2: System call forensics

Layout: `include/`, `src/`, `Makefile`, `testdata/`.

Build and run:

```
cd project2
make 2>&1 | tee compile_log.txt
mkdir -p testdata
echo "important data" > testdata/report.txt
./backup_tool testdata/report.txt backup_out
cat backup.log
```

Usage: `./backup_tool <source_file> <backup_dir> [log_file]`.

Traces. The removal lines make `mkdir` succeed on a fresh directory:

```
rm -rf backup_out backup.log
strace -f -tt -o traceA_success.txt ./backup_tool testdata/report.txt backup_out

rm -rf backup_out backup.log
strace -f -tt -o traceB_failure.txt ./backup_tool testdata/missing.txt backup_out

strace -f -o traceA2_rerun.txt ./backup_tool testdata/report.txt backup_out
```

The third command is run without deleting `backup_out`, so `mkdir` returns EEXIST. To read a trace, filter it:

```
grep -E 'openat|mkdir|read\(|write\(|close|clone|wait4|exit_group|execve|newfstatat|access' traceA_success.txt
```

To see the exit status of the failing run, do not trace it and do not pipe it:

```
./backup_tool testdata/missing.txt backup_out; echo $?
```

Expected: `1`.

Write-up: `table_and_analysis.md` holds the file call table, the process call table, the success against failure comparison, the summary table and the technical analysis, with trace excerpts.

## Project 3: CPython C extension

Layout: `pystats.py`, `fastlib/faststats.c`, `setup.py`, `app.py`, test, benchmark and plot scripts. Run everything from `project3/`, not from `fastlib/`.

Build the extension. This produces `faststats.cpython-312-x86_64-linux-gnu.so` in the current folder:

```
cd project3
python3 setup.py build_ext --inplace 2>&1 | tee compile_log.txt
```

Quick check:

```
python3 -c "import faststats, pystats; d=[1,2,3,4,5]; print(faststats.mean(d), pystats.mean(d))"
```

Use the application. `app.py` runs the operations with either implementation:

```
python3 app.py --impl python --op summary --size 1000
python3 app.py --impl c --op summary --size 1000
python3 app.py --impl c --op count_outside --size 100000 --low 30 --high 70
python3 app.py --impl c --op minmax --data numbers.txt
python3 app.py --impl c --op summary --size 1000000 --time
```

| Option | Meaning |
|---|---|
| `--impl python` or `c` | Which implementation to use (required) |
| `--op` | `mean`, `variance`, `minmax`, `count_outside`, `normalize` or `summary` (default `summary`, which runs the first five) |
| `--size N` | Generate N random values (default 1000) |
| `--data FILE` | Read values from a text file, one per line |
| `--seed`, `--low`, `--high` | Random seed, and the bounds for `count_outside` |
| `--time` | Print the time for each operation |

Save the demonstration as evidence:

```
python3 app.py --impl python --op summary --size 1000 | tee app_demo.txt
python3 app.py --impl c --op summary --size 1000 | tee -a app_demo.txt
python3 app.py --impl python --op summary --size 0 | tee -a app_demo.txt
python3 app.py --impl c --op summary --size 0 | tee -a app_demo.txt
python3 app.py --impl c --op summary --size 1000000 --time | tee -a app_demo.txt
```

The first two blocks should show identical values. The two empty runs should report `ValueError` for every operation except `count_outside`, which prints 0.

Equivalence tests (expect `76 passed, 0 failed`):

```
python3 test_equivalence.py 2>&1 | tee equivalence_results.txt
```

Benchmark. Plug the laptop in and close heavy programs first. It takes under a minute:

```
python3 benchmark.py 2>&1 | tee benchmark_output.txt
```

This writes `benchmark_results.csv`. Draw the graph from that file:

```
python3 plot.py
```

This writes `speedup_chart.png`. The write-ups are `analysis.md` (how the C API works, how the build works, trade-offs) and `report.md` (correctness, performance, limitations).

Using the module in your own code:

```python
import faststats
faststats.mean([1, 2, 3])
faststats.count_outside(data, 30.0, 70.0)   # counts x < low or x > high
```

If `import faststats` fails with `ModuleNotFoundError`, you are in the wrong folder or the build did not finish. `app.py --impl c` prints the same build hint.

## Project 4: Signal driven service

Layout: `server_controller.c`, `demo.sh`.

Compile:

```
cd project4
gcc -Wall -Wextra -o server_controller server_controller.c 2>&1 | tee compile_log.txt
```

No output means no warnings.

Scripted demo. It starts the service, sends every signal with `kill` at timed intervals and writes the result to `demo_log.txt`. It takes about 25 seconds:

```
sh demo.sh
cat demo_log.txt
```

Manual demo with two terminals. Terminal 1:

```
./server_controller
```

Terminal 2, from the same folder:

```
PID=$(cat server_controller.pid)
kill -SIGUSR1 $PID     # status report
kill -SIGUSR2 $PID     # toggle NORMAL and MAINTENANCE
kill -SIGINT  $PID     # graceful shutdown, removes the PID file
```

For SIGTERM, start the service again and run `kill -SIGTERM $(cat server_controller.pid)`. The service exits at once and leaves `server_controller.pid` behind. Remove it with `rm server_controller.pid`.

| Signal | Behaviour | Exit status |
|---|---|---|
| SIGUSR1 | Prints state and PID | keeps running |
| SIGUSR2 | Toggles NORMAL and MAINTENANCE | keeps running |
| SIGINT | Graceful shutdown, cleanup | 0 |
| SIGTERM | Emergency exit, no cleanup | 1 |

The write-up is `explanation.md`.

## Deliverables by project

| Project | Files to submit |
|---|---|
| 1 | `include/`, `src/`, `Makefile`, `compile_log.txt`, `analyze.sh`, `analysis/`, `analysis.md`, `comparison_table.md`, `forensic_report.md` |
| 2 | `include/`, `src/`, `Makefile`, `compile_log.txt`, `traceA_success.txt`, `traceB_failure.txt`, `traceA2_rerun.txt`, `table_and_analysis.md` |
| 3 | `pystats.py`, `fastlib/faststats.c`, `setup.py`, the built `.so`, `compile_log.txt`, `app.py`, `app_demo.txt`, `test_equivalence.py`, `equivalence_results.txt`, `benchmark.py`, `benchmark_results.csv`, `benchmark_output.txt`, `plot.py`, `speedup_chart.png`, `analysis.md`, `report.md` |
| 4 | `server_controller.c`, `compile_log.txt`, `demo.sh`, `demo_log.txt`, `explanation.md` |
| Root | `README.md`, `Combined_Report.docx` |

## Cleaning up

Projects 1 and 2 have a `make clean` target. For Project 3, remove build output with `rm -rf build *.so`. Delete any stray `project3/fastlib/compile_log.txt`. Do not delete the trace files, `analysis/`, `app_demo.txt` or `demo_log.txt` before submitting, since they are the evidence.

## Notes on evidence

Trace files, timings, PIDs and addresses differ on every run or rebuild. The write-ups quote the specific files included here, so submit those files and do not regenerate them afterwards unless you also update the quoted numbers.