"""Run the statistics operations with either the pure Python or the C implementation.

Examples:
  python3 app.py --impl python --op summary --size 1000
  python3 app.py --impl c --op count_outside --size 100000 --low 30 --high 70
  python3 app.py --impl c --op minmax --data numbers.txt
  python3 app.py --impl c --op summary --size 1000000 --time
"""
import argparse
import random
import sys
import time

OPERATIONS = ["mean", "variance", "minmax", "count_outside", "normalize", "summary"]


def load_module(name):
    """Import pystats or faststats, with a clear message if it is missing."""
    try:
        if name == "python":
            import pystats
            return pystats
        import faststats
        return faststats
    except ImportError as err:
        print(f"error: cannot import the {name} implementation: {err}", file=sys.stderr)
        if name == "c":
            print("Build it first: python3 setup.py build_ext --inplace", file=sys.stderr)
        sys.exit(2)


def read_numbers(path):
    """Read one number per line. Blank lines are skipped."""
    values = []
    try:
        with open(path) as fh:
            for lineno, line in enumerate(fh, start=1):
                text = line.strip()
                if not text:
                    continue
                try:
                    values.append(float(text))
                except ValueError:
                    print(f"error: {path} line {lineno}: not a number: {text!r}",
                          file=sys.stderr)
                    sys.exit(2)
    except OSError as err:
        print(f"error: cannot read {path}: {err}", file=sys.stderr)
        sys.exit(2)
    return values


def run(mod, op, data, low, high):
    """Run one operation and return its result."""
    if op == "mean":
        return mod.mean(data)
    if op == "variance":
        return mod.variance(data)
    if op == "minmax":
        return mod.minmax(data)
    if op == "count_outside":
        return mod.count_outside(data, low, high)
    if op == "normalize":
        return mod.normalize(data)
    raise ValueError(f"unknown operation {op}")


def show(result):
    """Print a result, shortening long lists."""
    if isinstance(result, list) and len(result) > 6:
        head = ", ".join(f"{x:.6g}" for x in result[:3])
        tail = ", ".join(f"{x:.6g}" for x in result[-3:])
        return f"[{head}, ..., {tail}] ({len(result)} values)"
    return repr(result)


def main():
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--impl", choices=["python", "c"], required=True,
                        help="which implementation to use")
    parser.add_argument("--op", choices=OPERATIONS, default="summary",
                        help="operation to run (default: summary)")
    source = parser.add_mutually_exclusive_group()
    source.add_argument("--data", help="text file with one number per line")
    source.add_argument("--size", type=int, default=1000,
                        help="generate this many random values (default: 1000)")
    parser.add_argument("--seed", type=int, default=42, help="random seed (default: 42)")
    parser.add_argument("--low", type=float, default=30.0, help="lower bound for count_outside")
    parser.add_argument("--high", type=float, default=70.0, help="upper bound for count_outside")
    parser.add_argument("--time", action="store_true", help="print the time taken")
    args = parser.parse_args()

    mod = load_module(args.impl)

    if args.data:
        data = read_numbers(args.data)
    else:
        if args.size < 0:
            print("error: --size must not be negative", file=sys.stderr)
            return 2
        random.seed(args.seed)
        data = [random.gauss(50.0, 10.0) for _ in range(args.size)]

    ops = OPERATIONS[:-1] if args.op == "summary" else [args.op]
    print(f"implementation: {args.impl}, values: {len(data)}")

    for op in ops:
        start = time.perf_counter()
        try:
            result = run(mod, op, data, args.low, args.high)
        except (ValueError, TypeError, OverflowError) as err:
            print(f"{op}: {type(err).__name__}: {err}")
            continue
        elapsed = time.perf_counter() - start
        line = f"{op}: {show(result)}"
        if args.time:
            line += f"   [{elapsed * 1000:.3f} ms]"
        print(line)
    return 0


if __name__ == "__main__":
    sys.exit(main())