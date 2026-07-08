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
    plt.figure(figsize=(8, 4))
    plt.hist(df["before"], bins=60, alpha=0.5, label="before victim")
    plt.hist(df["after"], bins=60, alpha=0.5, label="after victim")

    xmin = min(df["before"].min(), df["after"].min())
    xmax = max(df["before"].max(), df["after"].max())
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
