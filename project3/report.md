# Technical Report: C Extension for Data Analysis

## Implementation
pystats.py implements mean, variance (population, divides by n), minmax, count_outside (counts x < low or x > high) and normalize ((x - min) / (max - min), returning zeros for a constant dataset) in pure Python. faststats.c implements the same five operations through the CPython C API. Both reject bool and non-numeric items with TypeError, raise ValueError for empty input (except count_outside, which returns 0) and for low > high, and return floats, a tuple, an int or a list as the baseline does. The extension was built with gcc -O2 -Wall -Wextra on Python 3.12.3 and compiled without warnings.

## Correctness
test_equivalence.py ran 76 checks and all passed (equivalence_results.txt). They cover small datasets, single and constant values, negatives, mixed int and float, tuples, seeded random data at 1,000, 100,000 and 1,000,000 values, empty input, seven kinds of invalid input, bad bounds and return types. Comparison was exact equality, not a tolerance, because both versions add in the same order and the build does not use -ffast-math. Not covered: NaN, where comparison order changes results, and integers above 2^53, which the C version converts to double.

## Performance
Averages over 50, 10 and 3 repeats for 1,000, 100,000 and 1,000,000 values (benchmark_results.csv, speedup_chart.png):

| Operation | 1,000 | 100,000 | 1,000,000 |
|---|---|---|---|
| mean | 26.3x | 26.9x | 20.9x |
| variance | 44.2x | 46.9x | 42.7x |
| minmax | 24.3x | 20.1x | 20.9x |
| count_outside | 24.3x | 24.9x | 22.4x |
| normalize | 13.8x | 10.0x | 7.3x |

Every operation was faster at every size. mean, minmax and count_outside gained about 20 to 27x. variance gained the most, but the Python baseline validates the data twice (variance calls mean, and each calls the check) and loops twice, so part of that gap comes from repeated work in the baseline and not only from the C loop. normalize gained least, and its speedup fell as the dataset grew. Both versions must create one Python float per element, and that cost is unchanged by C. The likely causes of the cost growing faster than the data are allocation and cache pressure, which this benchmark did not isolate. Speedups were broadly steady between 1,000 and 100,000 values and dipped for some operations at 1,000,000, possibly from cache effects, also unmeasured. The 1,000,000 case used only 3 repeats, so those averages are less reliable.

## Limitations
- The C functions copy the data into a second array, so memory use grows with input size.
- Integers beyond 2^53 may compare differently from Python, and very large ints raise OverflowError.
- NaN behaviour is not guaranteed equivalent.
- Results depend on this machine; the CPU and the 3.12.3 interpreter should be stated with them.
- The extension is built for one Python version and platform.
- Speedups include conversion cost, because the input is a Python list. A NumPy-style array would avoid that cost, but was outside the assignment.

## Conclusion
The C extension gave the same results as the pure Python baseline in every tested case and ran 7x to 47x faster. The largest gains came from avoiding interpreter overhead and repeated validation, and the smallest came where the output had to be built as Python objects.