# Prime+Probe
As part of an undergraduate's project in micro-architectural attacks and weird gates.

## Compiling and Running
Compiling:

`make pp`

Running:

`taskset -c 0 ./pp`

(Assuming core 0 has a L1 cache with specifications fitting to those in consts.h)

Analyzing results:

`python ./read_results.py`

All in one:

```bash
make pp && taskset -c 0 ./pp && python ./read_results.p
```

Expects the raw results to be in `./results/raw/results.csv` (very large file). Creates the prime+probe heatmap (saved to `./results/prime_probe_heatmap.png`).

## Probe Single Set

Work on a single set. For i=0,...,NUM_LINES: Prime it, run victim with lines=i, probe.

To switch to this instead of full P+P: in `run_pp.c` in `main`, comment out call to `prime_and_probe` and comment in the for loop. Compiling and running like above will save raw results to `./results/raw/results_set_$i_lines.csv`, and print the file name for each $i as it does. 

Analyzing results using: `python ./read_set_results <results_file_name>`. Results will include histogram of probe times before and after victim, in `./results/probe_set/$i_lines.png`, and histogram of diff between after minus before times, in `./results/probe_set_diff`.

Compile, run and analyze all results:

```bash
make pp && for f in `taskset -c 0 ./pp`; do python ./read_set_results.py $f; done
```

(Still assuming core 0 has a L1 cache with specifications fitting to those in consts.h)
