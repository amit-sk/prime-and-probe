import argparse
import numpy as np
import pandas as pd
import matplotlib.pyplot as plt
import os.path
import re

LINES_PATTERN = r"(\d+)_lines"

def read_results(file_path, lines):
    data = pd.read_csv(file_path)
    if 'line' in data.columns:
        # measured per line access, not entire set
        output_file_path = os.path.join('./results', 'probe_set_per_line', f'{lines}_lines.png')
    else:
        # measured entire set access, not per line
        output_file_path = os.path.join('./results', 'probe_set', f'{lines}_lines.png')

    df = data[['before', 'after']]
    bottom = df.quantile(0.1)
    top = df.quantile(0.9)
    df = df[
        df['before'].between(bottom['before'], top['before'], inclusive='both') &
        df['after'].between(bottom['after'], top['after'], inclusive='both')
    ]
    # import ipdb; ipdb.set_trace()

    show_histogram_of_results(df, output_file_path, lines)

    df['after-before'] = df['after'] - df['before']
    diff_output_file_path = os.path.join('./results', 'probe_set_diff', f'{lines}_lines.png')
    show_histogram_of_diff(df, diff_output_file_path, lines)

def show_histogram_of_diff(df, output_file_path, lines):
    if lines != 0:
        df = df[(df['after-before'] > -20) & (df['after-before'] < 20)]  # where i see most of the data, filtering outliers

    data = df['after-before']

    bin_size = 1
    bins = np.arange(data.min(), data.max() + bin_size, bin_size)
    plt.figure(figsize=(8, 4))
    plt.hist(data, bins=bins, alpha=0.5)

    xmin = data.min()
    xmax = data.max()
    tick_gap=10
    plt.xticks(np.arange(tick_gap * (xmin // tick_gap), xmax + tick_gap, tick_gap), rotation=45)

    plt.xlabel("Probe time difference (after - before)")
    plt.ylabel("Count")
    plt.title(f"Probe time difference distribution: after minus before victim run with lines={lines}")
    plt.tight_layout()
    plt.savefig(output_file_path)
    # plt.show()


def show_histogram_of_results(df, output_file_path, lines):
    bin_size = 1
    bins = np.arange(df.min().min(), df.max().max() + bin_size, bin_size)
    plt.figure(figsize=(8, 4))
    plt.hist(df["before"], bins=bins, alpha=0.5, label="before victim")
    plt.hist(df["after"], bins=bins, alpha=0.5, label="after victim")

    xmin = df.min().min()
    xmax = df.max().max()
    tick_gap=10
    plt.xticks(np.arange(tick_gap * (xmin // tick_gap), xmax + tick_gap, tick_gap), rotation=45)

    plt.xlabel("Probe time")
    plt.ylabel("Count")
    plt.title(f"Probe time distribution: before vs after victim run with lines={lines}")
    plt.legend()
    plt.tight_layout()
    plt.savefig(output_file_path)
    # plt.show()


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("file", type=str, help="path to the CSV file containing the results")
    args = parser.parse_args()

    lines = int(re.search(LINES_PATTERN, args.file).group(1))
    read_results(args.file, lines)

if __name__ == "__main__":
    main()
