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
make pp && taskset -c 0 ./pp && python ./read_results.py
```

Expects the raw results to be in `./results/raw/results.csv` (very large file; it is streamed, so memory stays low). Creates two heatmaps:

- `./results/prime_probe_heatmap.png` - absolute average probe time per (accessed lines, cache set).
- `./results/prime_probe_heatmap_normalized.png` - the same with each set's own 0-access baseline subtracted, isolating the eviction signal.

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
taskset -c 0 ./aes_pp 00112233445566778899aabbccddeeff 100000
```

The measurements are saved to `./results/raw/aes_results.csv`. The file will contain `n` entries, with the plaintext, ciphertext and measurements collected for each set.

## Running analysis - correlation attacks

Correlation attack to recover the AES final-round key (and the original master key). If there isn't enough data, this will likely fail. Alongside the recovered key it writes two diagnostic figures:

- `./results/aes_key_hypotheses.png` — per ciphertext byte, the normalized correlation score for every one of the 256 key-byte guesses (bright column = winning guess).
- `./results/aes_table_location.png` — per T-table group, each byte's independent estimate of where its table starts in the cache (a sharp peak = table located).

With `--recovery-curve` (and `--key`) it also writes `./results/aes_recovery_curve.png` — correctly recovered key bytes vs. number of encryptions, obtained by re-running the attack on increasing prefixes of the capture. This is the recovery trajectory of that one capture, not a success rate (a success rate would require many independent captures at each size).

Defaults to the results file `./results/raw/aes_results.csv`:

```bash
python ./read_aes_results.py
```

Options:

- `input_file` — path to the `aes_pp` CSV (default `./results/raw/aes_results.csv`).
- `--key <32-hex>` — the known master key. Adds true-key markers to the key-hypothesis heatmap and reports how many of the 16 bytes were recovered. Omit it and the figures still render, just without the ground-truth overlay (the recovery itself does not use the key).
- `--limit N` — use only the first `N` traces (subsample).
- `--title <text>` — prefix for the key-hypothesis heatmap title.
- `--heatmap-out <path>` / `--table-out <path>` — override the figure output paths.
- `--recovery-curve` — also plot recovered bytes vs. number of encryptions over prefixes of the capture (requires `--key`; slow, as it re-runs the attack per prefix). `--recovery-out <path>` overrides its output path.

Example (passing the capture's key to check recovery):

```bash
python ./read_aes_results.py results/raw/aes_results.csv --key 00112233445566778899aabbccddeeff
```

Example output:
```
Preparing correlation data...
Scoring 16 ciphertext bytes over 64 sets...
  correlation for ciphertext byte 0...
  ...
  correlation for ciphertext byte 15...

Per-byte independent table start sets:
  group 0 (bytes [0, 4, 8, 12]): [41, 41, 41, 41]  [agree]
  group 1 (bytes [1, 5, 9, 13]): [25, 25, 25, 25]  [agree]
  group 2 (bytes [2, 6, 10, 14]): [9, 9, 9, 9]  [agree]
  group 3 (bytes [3, 7, 11, 15]): [57, 57, 57, 57]  [agree]

Final round key:
36 D0 24 46 1D 84 B8 37 5F C0 F9 C0 4C BA B6 BB
Original AES-128 master key (Round Key 0):
00 11 22 33 44 55 66 77 88 99 AA BB CC DD EE FF
wrote ./results/aes_key_hypotheses.png  (recovered 16/16)
wrote ./results/aes_table_location.png
```
