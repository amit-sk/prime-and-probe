Undergraduate's project in micro-architectural attacks and weird gates.

# Prime+Probe

## Compiling and Running
Compiling:

```bash
make pp
```

Running:

```bash
taskset -c 0 ./pp
```

(Assuming core 0 has a L1 cache with specifications fitting to those in consts.h)

Analyzing results:

```bash
python ./read_results.py
```

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

# AES attack

## Run AES to get measurements

The program generates `n` random plaintexts, and encrypts them while performing Prime+Probe, measuring the access time to each set after encryption. 

## Compiling and Running

Compile:

```bash
make aes_pp
```

Args: a 32-digit hexadecimal number that represents an AES `key`, and a number of encryptions `n`. 

Example:

```bash
./aes_pp 00112233445566778899aabbccddeeff 1000
```

The measurements are saved to `./results/raw/aes_results.csv`. The file will contain `n` entries, with the plaintext, ciphertext and measurements collected for each set.