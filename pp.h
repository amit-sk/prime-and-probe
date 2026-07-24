#include <stdint.h>
#include <stddef.h>
#include "consts.h"

/*
 * creates permutations for the order of set+line accesses (to avoid prefetching effects)
 * and initializes the linked list data structures used for the prime-and-probe attack.
 */
void ppinit(void);

/*
 * primes all sets in the order of the permutation, by following the linked list structure backwards.
 */
void prime(void);

/*
 * probes all sets in the order of the permutation, by following the linked list structure forwards.
 * the time taken to probe each set is recorded in the result array.
 */
void probe(uint64_t result[NUM_SETS]);

/* 
 * probes a single set, without using set permutations (i.e. `set` points to the actual set index to be accessed).
 * line order is determined by the `line_order` array, which is a permutation of the lines in the set.
 * measurements are done for each line in the set, and returned in the result array.
 * NOTE: because of timer overhead, this did not produce useful results.
*/
void probe_set_per_line(size_t set, uint64_t result[NUM_LINES]);

/*
 * probes a single set, without using set permutations (i.e. `set` points to the actual set index to be accessed).
 * line order is determined by the `line_order` array, which is a permutation of the lines in the set.
 * measurements are done for the entire set, and the total time is returned.
*/
uint64_t probe_set(size_t set);
