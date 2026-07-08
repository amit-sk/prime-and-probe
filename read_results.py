import pandas as pd
import matplotlib.pyplot as plt

def read_results(file_path):
    df = pd.read_csv(file_path)
    df['avg'] = df['sum_probe_time'] / df['count']

    # only need results for when the probed set == the victim set
    results = df[df['probe_set'] == df['victim_set']]
    results = results[['victim_set', 'victim_line_count', 'avg']].reset_index(drop=True)

    heatmap_data = results.pivot(
        index="victim_line_count",
        columns="victim_set",
        values="avg",
    )
    # vmin = results["avg"].quantile(0.05)
    # vmax = results["avg"].quantile(0.95)
    plt.figure(figsize=(12, 4))
    plt.imshow(
        heatmap_data,
        aspect="auto",
        origin="lower",
        # vmin=vmin,
        # vmax=vmax
    )

    plt.colorbar(label="Average probe time")
    plt.xlabel("Cache set")
    plt.ylabel("Number of accessed lines")
    plt.title("Average probe time by cache set and victim accesses")

    plt.xticks(range(0, 64, 4))
    plt.yticks(range(0, 13))

    plt.tight_layout()
    plt.savefig("./results/prime_probe_heatmap.png", dpi=200, bbox_inches="tight")
    plt.show()

def main():
    file_path = "./results/raw/results.csv"
    read_results(file_path)

if __name__ == "__main__":
    main()
