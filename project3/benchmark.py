"""Benchmark pystats against faststats."""
import csv
import gc
import random
import time

import faststats as f
import pystats as p

SIZES = [(1_000, 50), (100_000, 10), (1_000_000, 3)]
LOW, HIGH = 30.0, 70.0

OPERATIONS = [
    ("mean",          lambda m, d: m.mean(d)),
    ("variance",      lambda m, d: m.variance(d)),
    ("minmax",        lambda m, d: m.minmax(d)),
    ("count_outside", lambda m, d: m.count_outside(d, LOW, HIGH)),
    ("normalize",     lambda m, d: m.normalize(d)),
]


def time_call(func, repeats):
    """Return (average, minimum) seconds over repeats calls."""
    func()  # warm-up, not timed
    times = []
    gc.disable()
    try:
        for _ in range(repeats):
            start = time.perf_counter()
            func()
            times.append(time.perf_counter() - start)
    finally:
        gc.enable()
    return sum(times) / len(times), min(times)


def main():
    random.seed(42)
    rows = []

    print(f"{'size':>9} {'operation':<14} {'python (ms)':>12} {'C (ms)':>10} {'speedup':>9}")
    for size, repeats in SIZES:
        data = [random.gauss(50.0, 10.0) for _ in range(size)]
        for name, op in OPERATIONS:
            py_avg, py_min = time_call(lambda: op(p, data), repeats)
            c_avg, c_min = time_call(lambda: op(f, data), repeats)
            speedup = py_avg / c_avg
            rows.append([size, name, repeats, py_avg, py_min, c_avg, c_min, speedup])
            print(f"{size:>9} {name:<14} {py_avg * 1000:>12.3f} {c_avg * 1000:>10.3f} {speedup:>8.1f}x")

    with open("benchmark_results.csv", "w", newline="") as fh:
        w = csv.writer(fh)
        w.writerow(["size", "operation", "repeats", "py_avg_s", "py_min_s",
                    "c_avg_s", "c_min_s", "speedup"])
        w.writerows(rows)
    print("\nSaved benchmark_results.csv")


if __name__ == "__main__":
    main()