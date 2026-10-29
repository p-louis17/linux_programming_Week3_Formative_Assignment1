"""Draw a grouped bar chart of speedup from benchmark_results.csv."""
import csv

import matplotlib
matplotlib.use("Agg")  # write to a file, no window needed
import matplotlib.pyplot as plt

OPERATIONS = ["mean", "variance", "minmax", "count_outside", "normalize"]


def load(path):
    """Return {(operation, size): speedup} from the CSV."""
    speedups = {}
    sizes = []
    with open(path, newline="") as fh:
        for row in csv.DictReader(fh):
            size = int(row["size"])
            speedups[(row["operation"], size)] = float(row["speedup"])
            if size not in sizes:
                sizes.append(size)
    return speedups, sorted(sizes)


def main():
    speedups, sizes = load("benchmark_results.csv")

    fig, ax = plt.subplots(figsize=(10, 5.5))
    group_width = 0.8
    bar_width = group_width / len(sizes)

    for j, size in enumerate(sizes):
        # Shift each size's bar left or right of the group centre.
        offset = (j - (len(sizes) - 1) / 2) * bar_width
        xs = [i + offset for i in range(len(OPERATIONS))]
        ys = [speedups[(op, size)] for op in OPERATIONS]
        bars = ax.bar(xs, ys, width=bar_width, label=f"n = {size:,}")
        for bar, y in zip(bars, ys):
            ax.text(bar.get_x() + bar.get_width() / 2, y + 0.6, f"{y:.1f}x",
                    ha="center", va="bottom", fontsize=8)

    ax.axhline(1.0, color="gray", linestyle="--", linewidth=1)
    ax.text(len(OPERATIONS) - 0.5, 1.5, "break-even (1x)", color="gray",
            ha="right", fontsize=8)

    ax.set_xticks(range(len(OPERATIONS)))
    ax.set_xticklabels(OPERATIONS)
    ax.set_ylabel("Speedup (Python time / C extension time)")
    ax.set_title("C extension speedup over pure Python, by operation and dataset size")
    ax.set_ylim(0, max(speedups.values()) * 1.15)
    ax.legend(title="Dataset size")
    ax.grid(axis="y", alpha=0.3)

    fig.tight_layout()
    fig.savefig("speedup_chart.png", dpi=150)
    print("Saved speedup_chart.png")


if __name__ == "__main__":
    main()