# prime+probe
As part of an undergraduate's project in micro-architectural attacks and weird gates.

## compiling and running
Compiling:

`gcc -O2 -Wall -Wextra -std=c11 -o pp pp.c victim.c`

Running:

`taskset -c 0 ./pp > results.csv`

(Assuming core 0 has a L1 cache with specifications fitting to those in consts.h)

Analyzing results:

`python ./read_results.py`

Expects the results to be in the results.csv file. Creates the prime+probe heatmap (also saved to prime_probe_heatmap.png).