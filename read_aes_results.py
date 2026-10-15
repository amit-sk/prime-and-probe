import argparse

import pandas as pd
import polars as pl
import numpy as np
import matplotlib
matplotlib.use("Agg")           # headless; set before pyplot is imported
import matplotlib.pyplot as plt
from matplotlib.lines import Line2D


CIPHER_TEXT_SIZE = 16
PROBE_SET_COLUMN_NAME = 'probe_set_{}'

# Standard AES Inverse S-Box array
INV_SBOX = np.array([
    0x52, 0x09, 0x6a, 0xd5, 0x30, 0x36, 0xa5, 0x38, 0xbf, 0x40, 0xa3, 0x9e, 0x81, 0xf3, 0xd7, 0xfb,
    0x7c, 0xe3, 0x39, 0x82, 0x9b, 0x2f, 0xff, 0x87, 0x34, 0x8e, 0x43, 0x44, 0xc4, 0xde, 0xe9, 0xcb,
    0x54, 0x7b, 0x94, 0x32, 0xa6, 0xc2, 0x23, 0x3d, 0xee, 0x4c, 0x95, 0x0b, 0x42, 0xfa, 0xc3, 0x4e,
    0x08, 0x2e, 0xa1, 0x66, 0x28, 0xd9, 0x24, 0xb2, 0x76, 0x5b, 0xa2, 0x49, 0x6d, 0x8b, 0xd1, 0x25,
    0x72, 0xf8, 0xf6, 0x64, 0x86, 0x68, 0x98, 0x16, 0xd4, 0xa4, 0x5c, 0xcc, 0x5d, 0x65, 0xb6, 0x92,
    0x6c, 0x70, 0x48, 0x50, 0xfd, 0xed, 0xb9, 0xda, 0x5e, 0x15, 0x46, 0x57, 0xa7, 0x8d, 0x9d, 0x84,
    0x90, 0xd8, 0xab, 0x00, 0x8c, 0xbc, 0xd3, 0x0a, 0xf7, 0xe4, 0x58, 0x05, 0xb8, 0xb3, 0x45, 0x06,
    0xd0, 0x2c, 0x1e, 0x8f, 0xca, 0x3f, 0x0f, 0x02, 0xc1, 0xaf, 0xbd, 0x03, 0x01, 0x13, 0x8a, 0x6b,
    0x3a, 0x91, 0x11, 0x41, 0x4f, 0x67, 0xdc, 0xea, 0x97, 0xf2, 0xcf, 0xce, 0xf0, 0xb4, 0xe6, 0x73,
    0x96, 0xac, 0x74, 0x22, 0xe7, 0xad, 0x35, 0x85, 0xe2, 0xf9, 0x37, 0xe8, 0x1c, 0x75, 0xdf, 0x6e,
    0x47, 0xf1, 0x1a, 0x71, 0x1d, 0x29, 0xc5, 0x89, 0x6f, 0xb7, 0x62, 0x0e, 0xaa, 0x18, 0xbe, 0x1b,
    0xfc, 0x56, 0x3e, 0x4b, 0xc6, 0xd2, 0x79, 0x20, 0x9a, 0xdb, 0xc0, 0xfe, 0x78, 0xcd, 0x5a, 0xf4,
    0x1f, 0xdd, 0xa8, 0x33, 0x88, 0x07, 0xc7, 0x31, 0xb1, 0x12, 0x10, 0x59, 0x27, 0x80, 0xec, 0x5f,
    0x60, 0x51, 0x7f, 0xa9, 0x19, 0xb5, 0x4a, 0x0d, 0x2d, 0xe5, 0x7a, 0x9f, 0x93, 0xc9, 0x9c, 0xef,
    0xa0, 0xe0, 0x3b, 0x4d, 0xae, 0x2a, 0xf5, 0xb0, 0xc8, 0xeb, 0xbb, 0x3c, 0x83, 0x53, 0x99, 0x61,
    0x17, 0x2b, 0x04, 0x7e, 0xba, 0x77, 0xd6, 0x26, 0xe1, 0x69, 0x14, 0x63, 0x55, 0x21, 0x0c, 0x7d
], dtype=np.uint8)

# AES S-Box for key schedule
SBOX = [
    0x63, 0x7c, 0x77, 0x7b, 0xf2, 0x6b, 0x6f, 0xc5, 0x30, 0x01, 0x67, 0x2b, 0xfe, 0xd7, 0xab, 0x76,
    0xca, 0x82, 0xc9, 0x7d, 0xfa, 0x59, 0x47, 0xf0, 0xad, 0xd4, 0xa2, 0xaf, 0x9c, 0xa4, 0x72, 0xc0,
    0xb7, 0xfd, 0x93, 0x26, 0x36, 0x3f, 0xf7, 0xcc, 0x34, 0xa5, 0xe5, 0xf1, 0x71, 0xd8, 0x31, 0x15,
    0x04, 0xc7, 0x23, 0xc3, 0x18, 0x96, 0x05, 0x9a, 0x07, 0x12, 0x80, 0xe2, 0xeb, 0x27, 0xb2, 0x75,
    0x09, 0x83, 0x2c, 0x1a, 0x1b, 0x6e, 0x5a, 0xa0, 0x52, 0x3b, 0xd6, 0xb3, 0x29, 0xe3, 0x2f, 0x84,
    0x53, 0xd1, 0x00, 0xed, 0x20, 0xfc, 0xb1, 0x5b, 0x6a, 0xcb, 0xbe, 0x39, 0x4a, 0x4c, 0x58, 0xcf,
    0xd0, 0xef, 0xaa, 0xfb, 0x43, 0x4d, 0x33, 0x85, 0x45, 0xf9, 0x02, 0x7f, 0x50, 0x3c, 0x9f, 0xa8,
    0x51, 0xa3, 0x40, 0x8f, 0x92, 0x9d, 0x38, 0xf5, 0xbc, 0xb6, 0xda, 0x21, 0x10, 0xff, 0xf3, 0xd2,
    0xcd, 0x0c, 0x13, 0xec, 0x5f, 0x97, 0x44, 0x17, 0xc4, 0xa7, 0x7e, 0x3d, 0x64, 0x5d, 0x19, 0x73,
    0x60, 0x81, 0x4f, 0xdc, 0x22, 0x2a, 0x90, 0x88, 0x46, 0xee, 0xb8, 0x14, 0xde, 0x5e, 0x0b, 0xdb,
    0xe0, 0x32, 0x3a, 0x0a, 0x49, 0x06, 0x24, 0x5c, 0xc2, 0xd3, 0xac, 0x62, 0x91, 0x95, 0xe4, 0x79,
    0xe7, 0xc8, 0x37, 0x6d, 0x8d, 0xd5, 0x4e, 0xa9, 0x6c, 0x56, 0xf4, 0xea, 0x65, 0x7a, 0xae, 0x08,
    0xba, 0x78, 0x25, 0x2e, 0x1c, 0xa6, 0xb4, 0xc6, 0xe8, 0xdd, 0x74, 0x1f, 0x4b, 0xbd, 0x8b, 0x8a,
    0x70, 0x3e, 0xb5, 0x66, 0x48, 0x03, 0xf6, 0x0e, 0x61, 0x35, 0x57, 0xb9, 0x86, 0xc1, 0x1d, 0x9e,
    0xe1, 0xf8, 0x98, 0x11, 0x69, 0xd9, 0x8e, 0x94, 0x9b, 0x1e, 0x87, 0xe9, 0xce, 0x55, 0x28, 0xdf,
    0x8c, 0xa1, 0x89, 0x0d, 0xbf, 0xe6, 0x42, 0x68, 0x41, 0x99, 0x2d, 0x0f, 0xb0, 0x54, 0xbb, 0x16
]

RCON = [0x00, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x1B, 0x36]

GROUP_COLORS = ["#4C72B0", "#DD8452", "#55A868", "#C44E52"]   # ciphertext-byte groups 0..3
BYTE_COLORS = ["#1f77b4", "#ff7f0e", "#2ca02c", "#d62728"]    # the 4 bytes within a subplot

KEY_HEATMAP_OUTPUT = "./results/aes_key_hypotheses.png"
TABLE_LOCATION_OUTPUT = "./results/aes_table_location.png"
RECOVERY_CURVE_OUTPUT = "./results/aes_recovery_curve.png"


def _read_aes_pp_results(file_path):
    with open(file_path, 'r') as f:
        header = f.readline().strip().split(',')
        cache_sets_count = max(int(col.strip('probe_set_')) for col in header if col.startswith('probe_set_')) + 1

    probe_columns = [f"probe_set_{i}" for i in range(cache_sets_count)]
    return pl.read_csv(
        file_path,
        columns=["ciphertext", *probe_columns],
        schema_overrides={column: pl.UInt32 for column in probe_columns},
    )


def round10_key(master_key_hex):
    """Expand an AES-128 master key and return round-key 10 as 16 ints (ground truth)."""
    master = bytes.fromhex(master_key_hex)
    words = [list(master[i:i + 4]) for i in range(0, 16, 4)]
    for i in range(4, 44):
        tmp = list(words[i - 1])
        if i % 4 == 0:
            tmp = [SBOX[tmp[1]], SBOX[tmp[2]], SBOX[tmp[3]], SBOX[tmp[0]]]
            tmp[0] ^= RCON[i // 4]
        words.append([words[i - 4][j] ^ tmp[j] for j in range(4)])
    return np.array(sum(words[40:44], []), dtype=int)


def score_matrices(df, prepared_data, num_sets, verbose=True):
    """Per ciphertext byte: a [key_byte (256) x table_start_set (num_sets)] score matrix."""
    matrices = []
    for byte_index in range(CIPHER_TEXT_SIZE):
        if verbose:
            print(f'  correlation for ciphertext byte {byte_index}...')
        results = run_correlation(df, byte_index, prepared_data=prepared_data)
        matrix = (
            results.pivot(index='key_byte_value', columns='table_start_set', values='score')
            .sort_index()
            .reindex(columns=range(num_sets))
            .to_numpy()
        )
        matrices.append(matrix)
    return matrices


def estimate(matrices):
    """Independent per-byte table location + key pick (no joint/pooled estimation).

    Each ciphertext byte picks its (key, table start set) by a global argmax over
    its OWN 256 x num_sets score matrix -- the bytes are never combined.
    """
    col_max = [m.max(axis=0) for m in matrices]                 # best score vs start_set
    indep_set = [int(np.argmax(c)) for c in col_max]            # each byte's own table set
    indep_key = [int(np.argmax(matrices[i][:, indep_set[i]]))   # key at that set
                 for i in range(CIPHER_TEXT_SIZE)]
    return col_max, indep_set, indep_key


def make_key_heatmap(matrices, indep_set, indep_key, n_rows, title, output_path, true_k10=None):
    """Per-byte key-hypothesis scores, each row taken at that byte's own best table set."""
    profile = np.vstack([matrices[i][:, indep_set[i]] for i in range(16)])
    norm = ((profile - profile.min(1, keepdims=True))
            / (profile.max(1, keepdims=True) - profile.min(1, keepdims=True) + 1e-12))

    plt.rcParams.update({"font.size": 8, "axes.edgecolor": "#888", "axes.linewidth": .6,
                         "figure.facecolor": "white"})
    fig, ax = plt.subplots(figsize=(13, 5))
    image = ax.imshow(norm, aspect="auto", cmap="viridis")

    ax.scatter(indep_key, range(16), marker="_", s=120, color="w", lw=1.4)
    handles = [Line2D([], [], marker="_", color="w", mew=1.6, ls="", ms=12, label="script's pick")]
    n_correct = None
    if true_k10 is not None:
        ax.scatter(true_k10, range(16), marker="o", s=90,
                   facecolors="none", edgecolors="#E8000B", lw=1.8)
        handles.insert(0, Line2D([], [], marker="o", mfc="none", mec="#E8000B", mew=1.8,
                                 ls="", ms=9, label="true key"))
        n_correct = int(sum(indep_key[i] == int(true_k10[i]) for i in range(16)))

    ax.legend(handles=handles, loc="upper right", framealpha=.85, fontsize=7.5)
    ax.set_yticks(range(16))
    ax.set_yticklabels([f"byte {i}" for i in range(16)])
    for i in range(16):
        ax.get_yticklabels()[i].set_color(GROUP_COLORS[i % 4])
    ax.set_xlabel("key-byte hypothesis (0-255)")
    ax.set_ylabel("ciphertext byte")
    fig.colorbar(image, ax=ax, fraction=.025, pad=.01).set_label("normalised score (per row)")

    suptitle = f"AES key-hypothesis heatmap  -  {n_rows:,} traces"
    if n_correct is not None:
        suptitle += f",  recovered {n_correct}/16 bytes"
    if title:
        suptitle = f"{title}:  " + suptitle
    fig.suptitle(suptitle, fontsize=12, fontweight="bold")

    fig.tight_layout()
    fig.savefig(output_path, dpi=140, bbox_inches="tight")
    plt.close(fig)
    print(f"wrote {output_path}" + (f"  (recovered {n_correct}/16)" if n_correct is not None else ""))


def make_table_location(col_max, indep_set, num_sets, output_path):
    """Per-group subplot, 4 independent byte curves (best score = max over key guesses).

    Each curve's dot marks that byte's own peak (its independent table-location
    estimate); the dashed line marks the set the group's four bytes agree on.
    All panels share one vertical scale so peak-strength differences show.
    """
    plt.rcParams.update({"font.size": 8})
    ymax = max(c.max() for c in col_max)
    fig, axes = plt.subplots(2, 2, figsize=(13, 7), sharex=True, sharey=True)
    for group, ax in zip(range(4), axes.ravel()):
        bytes_in_group = [group + 4 * j for j in range(4)]
        for k, i in enumerate(bytes_in_group):
            ax.plot(range(num_sets), col_max[i], color=BYTE_COLORS[k], lw=1.2,
                    label=f"byte {i}  (peak set {indep_set[i]})")
            ax.plot(indep_set[i], col_max[i][indep_set[i]], "o",
                    color=BYTE_COLORS[k], ms=4)
        # The set the four independent estimates agree on (mode of their peaks).
        group_peaks = [indep_set[i] for i in bytes_in_group]
        common = max(set(group_peaks), key=group_peaks.count)
        ax.axvline(common, color="#333", lw=1.0, ls="--",
                   label=f"independent estimate -> set {common}")
        ax.set_title(f"Group {group}  (bytes {group}, {group + 4}, {group + 8}, {group + 12})")
        # Keep the legend off the peak: opposite side to where it sits.
        loc = "upper left" if common > num_sets / 2 else "upper right"
        ax.legend(fontsize=6.5, loc=loc, framealpha=.9)
    axes[0, 0].set_ylim(0, ymax * 1.05)          # shared scale (sharey) across all panels
    for ax in axes[:, 0]:
        ax.set_ylabel("best score (max over 256 key guesses)")
    for ax in axes[-1]:
        ax.set_xlabel("cache set (table start position)")

    fig.suptitle("Independent estimates of AES T-table locations",
                 fontsize=13, fontweight="bold")
    fig.tight_layout()
    fig.savefig(output_path, dpi=140, bbox_inches="tight")
    plt.close(fig)
    print(f"wrote {output_path}")


def recovery_prefixes(n_rows, num_points=16, start=2000):
    """Log-spaced prefix sizes, from `start` traces up to the full capture."""
    start = min(start, n_rows)
    prefixes = np.unique(np.geomspace(start, n_rows, num_points).astype(int))
    if prefixes[-1] != n_rows:
        prefixes = np.append(prefixes, n_rows)
    return prefixes.tolist()


def recovery_curve(df, true_k10, prefixes):
    """Correctly recovered key bytes using only the first N traces, for each N.

    This analyses PREFIXES of a single capture, so it traces how recovery of
    THIS run grew with data -- it is not a success rate, which would require many
    independent captures at each N.
    """
    points = []
    for n in prefixes:
        sub = df.head(int(n))
        # Tiny prefixes leave some ciphertext-byte values unseen (empty bins ->
        # 0/0 in the correlation); ignore those warnings, the result is just noise.
        with np.errstate(all='ignore'):
            prepared = _prepare_correlation_data(sub)
            matrices = score_matrices(sub, prepared, prepared[4], verbose=False)
        _, _, indep_key = estimate(matrices)
        n_correct = int(sum(indep_key[i] == int(true_k10[i]) for i in range(16)))
        points.append((int(n), n_correct))
        print(f'  N={int(n):>9,}: recovered {n_correct}/16')
    return points


def make_recovery_curve(points, output_path):
    """Figure: correctly recovered key bytes vs. number of encryptions (one capture)."""
    xs = [p[0] for p in points]
    ys = [p[1] for p in points]
    plt.rcParams.update({"font.size": 9})
    fig, ax = plt.subplots(figsize=(8, 5))
    ax.plot(xs, ys, marker="o", ms=5, color="#4C72B0", lw=1.5)
    ax.set_xscale("log")
    ax.set_ylim(-0.5, 16.5)
    ax.set_yticks(range(0, 17, 2))
    ax.axhline(16, color="#888", lw=0.8, ls=":")
    ax.grid(True, which="both", alpha=0.25)
    ax.set_xlabel("number of encryptions (prefix of the capture)")
    ax.set_ylabel("correctly recovered key bytes (of 16)")
    ax.set_title("AES key bytes recovered vs. number of encryptions\n"
                 "(recovery from one capture's prefixes; not a success rate)")
    fig.tight_layout()
    fig.savefig(output_path, dpi=140, bbox_inches="tight")
    plt.close(fig)
    print(f"wrote {output_path}")


def read_results(file_path, master_key_hex=None, limit=None, title="",
                 key_heatmap_output=KEY_HEATMAP_OUTPUT,
                 table_location_output=TABLE_LOCATION_OUTPUT,
                 plot_recovery=False, recovery_curve_output=RECOVERY_CURVE_OUTPUT):
    df = _read_aes_pp_results(file_path)
    if limit:
        df = df.head(limit)
    n_rows = df.height
    true_k10 = round10_key(master_key_hex) if master_key_hex else None

    print('Preparing correlation data...')
    prepared_data = _prepare_correlation_data(df)
    num_sets = prepared_data[4]
    print(f'Scoring {CIPHER_TEXT_SIZE} ciphertext bytes over {num_sets} sets...')
    matrices = score_matrices(df, prepared_data, num_sets)
    col_max, indep_set, indep_key = estimate(matrices)

    # Each T-table group (index % 4) indexes one table; its four bytes should
    # independently land on the same start set. Report whether they agree.
    print('\nPer-byte independent table start sets:')
    for group in range(4):
        bytes_in_group = [group + 4 * j for j in range(CIPHER_TEXT_SIZE // 4)]
        sets = [indep_set[i] for i in bytes_in_group]
        agreement = 'agree' if len(set(sets)) == 1 else 'DISAGREE'
        print(f'  group {group} (bytes {bytes_in_group}): {sets}  [{agreement}]')

    print('\nFinal round key:')
    print(' '.join(f'{b:02X}' for b in indep_key))
    k0_bytes = reverse_key_schedule_128(bytes(indep_key))
    print('Original AES-128 master key (Round Key 0):')
    print(' '.join(f'{b:02X}' for b in k0_bytes))

    make_key_heatmap(matrices, indep_set, indep_key, n_rows, title,
                     key_heatmap_output, true_k10=true_k10)
    make_table_location(col_max, indep_set, num_sets, table_location_output)

    if plot_recovery:
        if true_k10 is None:
            print('\nskipping recovery curve: --key is required to judge correctness')
        else:
            prefixes = recovery_prefixes(n_rows)
            print(f'\nBuilding recovery curve over {len(prefixes)} capture prefixes '
                  f'(re-runs the attack per prefix; this is slow)...')
            points = recovery_curve(df, true_k10, prefixes)
            make_recovery_curve(points, recovery_curve_output)


def _prepare_correlation_data(df):
    probe_columns = [column for column in df.columns if column.startswith('probe_set_')]
    num_sets = len(probe_columns)
    lower = df.select([
        pl.col(column).quantile(0.05, interpolation='nearest').alias(column)
        for column in probe_columns
    ]).row(0)
    upper = df.select([
        pl.col(column).quantile(0.95, interpolation='nearest').alias(column)
        for column in probe_columns
    ]).row(0)
    clipped = df.with_columns([
        pl.col(column).clip(lower[index], upper[index]).alias(column)
        for index, column in enumerate(probe_columns)
    ])

    statistics = clipped.select(
        [pl.col(column).mean().alias(f'mean_{column}') for column in probe_columns]
        + [pl.col(column).std(ddof=0).alias(f'std_{column}') for column in probe_columns]
    ).row(0)
    mean_Y = np.asarray(statistics[:num_sets], dtype=np.float64)[None, :]
    std_Y = np.asarray(statistics[num_sets:], dtype=np.float64)[None, :]

    # Scatter each group's aggregate into full 256-length arrays indexed by the
    # ciphertext-byte value. A value that never occurs (possible on small
    # prefixes/--limit) simply stays zero instead of shifting the rows.
    byte_counts = np.zeros((CIPHER_TEXT_SIZE, 256), dtype=np.int64)
    byte_sums = np.zeros((CIPHER_TEXT_SIZE, 256, num_sets), dtype=np.float64)
    for byte_index in range(CIPHER_TEXT_SIZE):
        grouped = clipped.with_columns(
            pl.col('ciphertext').str.slice(byte_index * 2, 2)
            .str.to_integer(base=16).alias('cipher_byte')
        ).group_by('cipher_byte').agg(
            [pl.len().alias('count')]
            + [pl.col(column).sum().alias(column) for column in probe_columns]
        )
        cipher_byte = grouped['cipher_byte'].to_numpy()
        byte_counts[byte_index, cipher_byte] = grouped['count'].to_numpy()
        byte_sums[byte_index, cipher_byte] = grouped.select(probe_columns).to_numpy().astype(
            np.float64, copy=False
        )

    return mean_Y, std_Y, byte_counts, byte_sums, num_sets


def run_correlation(df, ciphertext_byte_index, prepared_data=None):
    if prepared_data is None:
        prepared_data = _prepare_correlation_data(df)

    mean_Y, std_Y, byte_counts, byte_sums, num_sets = prepared_data
    counts_by_value = byte_counts[ciphertext_byte_index]
    sums_by_value = byte_sums[ciphertext_byte_index]
    N = counts_by_value.sum()

    cache_line_size = 64  # bytes
    table_entry_size = 4  # bytes
    table_cache_lines_count = INV_SBOX.size * table_entry_size // cache_line_size
    entries_per_cache_line = cache_line_size // table_entry_size

    cipher_values = np.arange(256, dtype=np.uint8)
    line_by_key_and_value = INV_SBOX[np.bitwise_xor(
        np.arange(256, dtype=np.uint8)[:, None], cipher_values[None, :]
    )] // entries_per_cache_line

    results = []
    for byte_value in range(256):
        line_by_value = line_by_key_and_value[byte_value]
        counts = np.bincount(
            line_by_value, weights=counts_by_value, minlength=table_cache_lines_count
        )
        sums = np.empty((table_cache_lines_count, num_sets), dtype=np.float64)
        for relative_line in range(table_cache_lines_count):
            sums[relative_line] = sums_by_value[line_by_value == relative_line].sum(axis=0)

        probability = counts / N
        cov = sums / N - probability[:, None] * mean_Y
        std_X = np.sqrt(probability * (1 - probability))[:, None]
        denom = std_X * std_Y
        M = np.abs(cov / denom)

        # for each possible set the table starts in 
        relative_line_indices = np.arange(table_cache_lines_count)
        for table_start_set in range(num_sets):
            set_indices = (table_start_set + relative_line_indices) % num_sets  # which sets the relative cache lines map to
            total_score = np.sum(M[relative_line_indices, set_indices])
            results.append({
                'key_byte_value': byte_value,
                'table_start_set': table_start_set,
                'score': total_score
            })

    results_df = pd.DataFrame(results)
    results_df['key_hex'] = results_df['key_byte_value'].apply(lambda x: f"0x{x:02X}")
    return results_df[['key_hex', 'key_byte_value', 'table_start_set', 'score']]

def reverse_key_schedule_128(k10_bytes: bytes) -> bytes:
    """AI gen code that reverses AES-128 key schedule from Round Key 10 to Round Key 0."""
    # Convert 16-byte key to four 32-bit words
    w = [int.from_bytes(k10_bytes[i:i+4], 'big') for i in range(0, 16, 4)]

    def g(word, rcon_idx):
        # RotWord + SubWord + Rcon
        b0 = (word >> 24) & 0xFF
        b1 = (word >> 16) & 0xFF
        b2 = (word >> 8) & 0xFF
        b3 = word & 0xFF
        
        # RotWord: [b1, b2, b3, b0] -> SubWord + Rcon
        res0 = SBOX[b1] ^ RCON[rcon_idx]
        res1 = SBOX[b2]
        res2 = SBOX[b3]
        res3 = SBOX[b0]
        
        return (res0 << 24) | (res1 << 16) | (res2 << 8) | res3

    # Reverse 10 rounds
    for r in range(10, 0, -1):
        w3 = w[3] ^ w[2]
        w2 = w[2] ^ w[1]
        w1 = w[1] ^ w[0]
        w0 = w[0] ^ g(w3, r)
        w = [w0, w1, w2, w3]

    # Reconstruct 16-byte master key
    return b''.join(word.to_bytes(4, 'big') for word in w)


def main():
    parser = argparse.ArgumentParser(
        description="Recover the AES-128 key from prime+probe measurements and plot diagnostics."
    )
    parser.add_argument("input_file", nargs="?", default="./results/raw/aes_results.csv",
                        help="CSV produced by aes_pp (default: ./results/raw/aes_results.csv)")
    parser.add_argument("--key", default=None,
                        help="known 32-hex master key; adds true-key markers + recovered count")
    parser.add_argument("--limit", type=int, default=None,
                        help="use only the first N rows (subsample)")
    parser.add_argument("--title", default="", help="optional prefix for the key-heatmap title")
    parser.add_argument("--heatmap-out", default=KEY_HEATMAP_OUTPUT,
                        help=f"key-hypothesis heatmap path (default: {KEY_HEATMAP_OUTPUT})")
    parser.add_argument("--table-out", default=TABLE_LOCATION_OUTPUT,
                        help=f"table-location figure path (default: {TABLE_LOCATION_OUTPUT})")
    parser.add_argument("--recovery-curve", action="store_true",
                        help="also plot correctly-recovered bytes vs. number of encryptions, "
                             "by re-running the attack on prefixes of the capture "
                             "(requires --key; slow)")
    parser.add_argument("--recovery-out", default=RECOVERY_CURVE_OUTPUT,
                        help=f"recovery-curve figure path (default: {RECOVERY_CURVE_OUTPUT})")
    args = parser.parse_args()
    read_results(args.input_file, master_key_hex=args.key, limit=args.limit, title=args.title,
                 key_heatmap_output=args.heatmap_out, table_location_output=args.table_out,
                 plot_recovery=args.recovery_curve, recovery_curve_output=args.recovery_out)


if __name__ == "__main__":
    main()
