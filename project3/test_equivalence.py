"""Check that pystats and faststats give equivalent results."""
import random
import sys

import faststats as f
import pystats as p

passed = 0
failed = 0


def report(name, ok, detail=""):
    global passed, failed
    if ok:
        passed += 1
        print(f"PASS  {name}")
    else:
        failed += 1
        print(f"FAIL  {name}  {detail}")


def same_result(name, call_py, call_c):
    """Both must return equal values, or raise the same exception type."""
    try:
        a = call_py()
        a_err = None
    except Exception as e:
        a, a_err = None, type(e)
    try:
        b = call_c()
        b_err = None
    except Exception as e:
        b, b_err = None, type(e)

    if a_err or b_err:
        report(name, a_err == b_err, f"python={a_err} c={b_err}")
    else:
        report(name, a == b, f"python={a!r:.80} c={b!r:.80}")


def all_ops(label, data, low, high):
    same_result(f"{label}: mean", lambda: p.mean(data), lambda: f.mean(data))
    same_result(f"{label}: variance", lambda: p.variance(data), lambda: f.variance(data))
    same_result(f"{label}: minmax", lambda: p.minmax(data), lambda: f.minmax(data))
    same_result(f"{label}: count_outside",
                lambda: p.count_outside(data, low, high),
                lambda: f.count_outside(data, low, high))
    same_result(f"{label}: normalize", lambda: p.normalize(data), lambda: f.normalize(data))


random.seed(42)

# Small datasets
all_ops("small ints", [1, 2, 3, 4, 5], 2, 4)
all_ops("single value", [42], 0, 100)
all_ops("two values", [3.5, -1.25], 0, 1)
all_ops("negatives", [-5, -10, -3, -8], -9, -4)
all_ops("constant", [7, 7, 7, 7], 7, 7)
all_ops("mixed int/float", [1, 2.5, 3, 4.75], 2, 4)

# Large datasets
for size in (1_000, 100_000, 1_000_000):
    data = [random.gauss(50.0, 10.0) for _ in range(size)]
    all_ops(f"gauss n={size}", data, 30.0, 70.0)

big_ints = [random.randint(-1000, 1000) for _ in range(100_000)]
all_ops("random ints n=100000", big_ints, -500, 500)

# Other sequence types
all_ops("tuple", (1, 2, 3, 4, 5), 2, 4)

# Edge cases: empty input
all_ops("empty list", [], 0, 1)

# Edge cases: invalid input
for label, bad in [("string item", [1, "a", 3]),
                   ("None item", [1, None]),
                   ("bool item", [True, False]),
                   ("whole string", "12345"),
                   ("bytes", b"123"),
                   ("not a sequence", 5),
                   ("nested list", [[1, 2], [3]])]:
    same_result(f"invalid ({label}): mean", lambda b=bad: p.mean(b), lambda b=bad: f.mean(b))
    same_result(f"invalid ({label}): normalize", lambda b=bad: p.normalize(b), lambda b=bad: f.normalize(b))

# Edge case: bad bounds
same_result("bounds low>high", lambda: p.count_outside([1, 2], 5, 1),
            lambda: f.count_outside([1, 2], 5, 1))

# Return types must also match, not just values
d = [1, 2, 3]
types_ok = (type(p.mean(d)) is type(f.mean(d))
            and type(p.minmax(d)) is type(f.minmax(d))
            and type(p.count_outside(d, 1, 2)) is type(f.count_outside(d, 1, 2))
            and type(p.normalize(d)) is type(f.normalize(d)))
report("return types match", types_ok)

print(f"\n{passed} passed, {failed} failed")
sys.exit(1 if failed else 0)