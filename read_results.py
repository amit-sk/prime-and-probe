"""Prime+Probe sweep heatmaps.

Reads a sweep CSV (victim_set, victim_line_count, probe_set, probe_time) and
always writes TWO figures as separate files:

  results/prime_probe_heatmap_absolute.png
      Raw average probe time per (accessed lines, cache set). A large per-set
      baseline (probe-order position, TLB, etc.) dominates and shows up as
      vertical stripes.

  results/prime_probe_heatmap_normalized.png
      Each cache set's own line_count==0 baseline subtracted out, so the
      stripes and the constant measurement overhead are removed and only the
      eviction signal remains -- the extra probe time caused by the victim
      touching that set. Zero is pinned to the bottom of the scale.

Memory note: the sweep CSV can be ~1GB of text. We scan it with polars in
streaming mode and keep only the diagonal rows (probe_set == victim_set, ~1/64
of the file) before collecting, so peak memory stays small (a full-file pandas
load would OOM on a small machine).
"""
import numpy as np
import polars as pl
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt

ABSOLUTE_OUTPUT = "./results/prime_probe_heatmap.png"
NORMALIZED_OUTPUT = "./results/prime_probe_heatmap_normalized.png"


def load_diagonal(file_path):
    """Return the probed==victim rows only.

    Streams the (potentially huge) CSV, filtering to the diagonal on the fly so
    only ~1/64 of the rows are ever materialized.
    """
    schema = {
        "victim_set": pl.UInt16,
        "victim_line_count": pl.UInt16,
        "probe_set": pl.UInt16,
        "probe_time": pl.UInt32,
    }
    lazy = (
        pl.scan_csv(file_path, schema_overrides=schema)
        .filter(pl.col("probe_set") == pl.col("victim_set"))
        .select(["victim_set", "victim_line_count", "probe_time"])
    )
    try:
        return lazy.collect(engine="streaming")
    except TypeError:  # older polars
        return lazy.collect(streaming=True)


def build_heatmaps(diag):
    """Return (heatmap_abs, heatmap_norm, num_sets, num_lines) as numpy grids."""
    num_sets = int(diag["victim_set"].max()) + 1
    num_lines = int(diag["victim_line_count"].max())
    print(f"detected geometry: {num_sets} sets, up to {num_lines} victim lines")

    # Clip gross outliers (scheduler hiccups, odd page-boundary sets) before
    # aggregating so a few huge samples don't dominate a set's mean.
    bottom = diag["probe_time"].quantile(0.05)
    top = diag["probe_time"].quantile(0.95)
    clipped = diag.filter(
        (pl.col("probe_time") >= bottom) & (pl.col("probe_time") <= top)
    )

    avg = clipped.group_by(["victim_line_count", "victim_set"]).agg(
        pl.col("probe_time").mean()
    )

    # Pivot into a (num_lines+1 x num_sets) grid without pandas/pyarrow.
    heatmap_abs = np.full((num_lines + 1, num_sets), np.nan)
    lines = avg["victim_line_count"].to_numpy()
    sets = avg["victim_set"].to_numpy()
    means = avg["probe_time"].to_numpy()
    heatmap_abs[lines, sets] = means

    # Subtract each set's own 0-access baseline (the line_count==0 row).
    heatmap_norm = heatmap_abs - heatmap_abs[0, :]
    return heatmap_abs, heatmap_norm, num_sets, num_lines


def save_heatmap(data, num_sets, num_lines, title, cbar_label, output_path, vmin=None):
    fig, ax = plt.subplots(figsize=(12, 4))
    im = ax.imshow(
        data,
        aspect="auto",
        origin="lower",
        vmin=vmin,
        vmax=np.nanmax(data),
    )
    fig.colorbar(im, ax=ax, label=cbar_label)
    ax.set_title(title)
    ax.set_xlabel("Cache set")
    ax.set_ylabel("Victim cache lines accessed")
    ax.set_xticks(range(0, num_sets, 4))
    ax.set_yticks(range(0, num_lines + 1))

    fig.tight_layout()
    fig.savefig(output_path, dpi=200, bbox_inches="tight")
    plt.close(fig)
    print(f"wrote {output_path}")


def read_results(file_path, absolute_output=ABSOLUTE_OUTPUT, normalized_output=NORMALIZED_OUTPUT):
    diag = load_diagonal(file_path)
    heatmap_abs, heatmap_norm, num_sets, num_lines = build_heatmaps(diag)

    save_heatmap(
        heatmap_abs, num_sets, num_lines,
        "Prime+Probe response to synthetic victim accesses — absolute",
        "Mean probe time (timer ticks)", absolute_output,
    )
    save_heatmap(
        heatmap_norm, num_sets, num_lines,
        "Prime+Probe response to synthetic victim accesses — per-set normalized",
        "Baseline-subtracted mean probe time (timer ticks)", normalized_output, vmin=0,
    )


def main():
    file_path = "./results/raw/results.csv"
    read_results(file_path)


if __name__ == "__main__":
    main()
