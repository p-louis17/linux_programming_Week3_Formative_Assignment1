"""Pure Python baseline for the Project 3 statistics operations."""


def _check(data):
    """Validate that data is a non-string sequence of int/float values.

    Returns the data unchanged. Raises TypeError for bad input.
    """
    if isinstance(data, (str, bytes)):
        raise TypeError("data must be a sequence of numbers, not a string")
    try:
        iter(data)
    except TypeError:
        raise TypeError("data must be a sequence of numbers") from None
    for x in data:
        if isinstance(x, bool) or not isinstance(x, (int, float)):
            raise TypeError(f"all values must be int or float, got {type(x).__name__}")
    return data


def mean(data):
    data = _check(data)
    n = len(data)
    if n == 0:
        raise ValueError("mean of empty dataset")
    total = 0.0
    for x in data:
        total += x
    return total / n


def variance(data):
    """Population variance (divides by n)."""
    data = _check(data)
    n = len(data)
    if n == 0:
        raise ValueError("variance of empty dataset")
    m = mean(data)
    acc = 0.0
    for x in data:
        d = x - m
        acc += d * d
    return acc / n


def minmax(data):
    """Return (minimum, maximum) as floats."""
    data = _check(data)
    if len(data) == 0:
        raise ValueError("minmax of empty dataset")
    lo = hi = float(data[0])
    for x in data:
        if x < lo:
            lo = float(x)
        if x > hi:
            hi = float(x)
    return lo, hi


def count_outside(data, low, high):
    """Count values with x < low or x > high."""
    data = _check(data)
    if low > high:
        raise ValueError("low must not be greater than high")
    count = 0
    for x in data:
        if x < low or x > high:
            count += 1
    return count


def normalize(data):
    """Scale values to [0, 1] using the dataset minimum and maximum."""
    data = _check(data)
    if len(data) == 0:
        raise ValueError("normalize of empty dataset")
    lo, hi = minmax(data)
    span = hi - lo
    if span == 0.0:
        return [0.0] * len(data)
    return [(x - lo) / span for x in data]