import argparse
import numpy as np
import pandas as pd
import matplotlib.pyplot as plt


def read_results(file_path, lines):
    df = pd.read_csv(file_path)[['before','after']]
    plt.figure(figsize=(8, 4))
    plt.hist(df["before"], bins=60, alpha=0.5, label="before victim")
    plt.hist(df["after"], bins=60, alpha=0.5, label="after victim")

    xmin = min(df["before"].min(), df["after"].min())
    xmax = max(df["before"].max(), df["after"].max())
    tick_gap=10
    plt.xticks(np.arange(tick_gap * (xmin // tick_gap), xmax + tick_gap, tick_gap))

    plt.xlabel("Probe time")
    plt.ylabel("Count")
    plt.title(f"Probe time distribution: before vs after victim run with lines={lines}")
    plt.legend()
    plt.tight_layout()
    plt.savefig(f'test_set17_lines{lines}.png')
    plt.show()


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("file", type=str, help="path to the CSV file containing the results", default="test17_results.csv")
    parser.add_argument("l", type=int, help="lines used by victim from 0 to 12")
    args = parser.parse_args()
    read_results(args.file, args.l)

if __name__ == "__main__":
    main()
