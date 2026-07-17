#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <x86intrin.h>
#include <inttypes.h>
#include <limits.h>
#include <string.h>
#include <assert.h>

#include "consts.h"
#include "victim.h"
#include "pointer_chasing.h"

#define REPETITIONS (1000000)

#define PRIME_AND_PROBE_RESULTS_FILENAME ("./results/raw/results.csv")
#define PRIME_AND_PROBE_SET_RESULTS_FILENAME ("./results/raw/results_set_%zu_lines.csv")
#define PRIME_AND_PROBE_SET_PER_LINE_RESULTS_FILENAME ("./results/raw/results_set_per_line_%zu_lines.csv")

// static volatile uint8_t buffer[NUM_SETS * NUM_LINES * BLOCK_SIZE] __attribute__((aligned(4096))) = {0};
static volatile uint16_t prime_buffer[NUM_SETS * NUM_LINES * BLOCK_SIZE / sizeof(uint16_t)] __attribute__((aligned(4096))) = {0};
static uint16_t prime_set_order[NUM_SETS] = {0};
static uint16_t prime_line_order[NUM_LINES] = {0};

static volatile uint16_t probe_buffer[NUM_SETS * NUM_LINES * BLOCK_SIZE / sizeof(uint16_t)] __attribute__((aligned(4096))) = {0};
static uint16_t probe_set_order[NUM_SETS] = {0};
static uint16_t probe_line_order[NUM_LINES] = {0};

#define BUFFER_NUM_ELEMS (NUM_SETS * NUM_LINES * BLOCK_SIZE / sizeof(uint16_t))
_Static_assert(BUFFER_NUM_ELEMS <= UINT16_MAX, "uint16_t is too small to index the buffer");

// for probing single set
typedef struct
{
    uint64_t before[NUM_LINES];
    uint64_t after[NUM_LINES];
} test_results_t;


void ppinit(void)
{    
    init_linked_list_structure(prime_set_order, prime_line_order, prime_buffer);
    init_opposite_linked_list_structure(prime_set_order, prime_line_order, prime_buffer, probe_set_order, probe_line_order, probe_buffer);
    init_victim();

    /* provided code for ramping up CPU */
    uint32_t dummy;
    uint64_t start = __rdtscp(&dummy);
    while (__rdtscp(&dummy) - start < 1000000000) ;
}

void prime(void)
{
    uint16_t head = GET_BUFFER_IDX(prime_set_order[0], prime_line_order[0]);
    uint16_t idx = head;

    do {
        idx = prime_buffer[idx];
        _mm_mfence();
    } while (idx != head);
}

void probe(uint64_t result[NUM_SETS], uint64_t counts[NUM_SETS])
{
    uint32_t dummy = 0;
    uint16_t idx = GET_BUFFER_IDX(probe_set_order[0], probe_line_order[0]);  // probing all sets in the order of the permutation

    for (size_t s = 0; s < NUM_SETS; s++)
    {
        uint16_t set = probe_set_order[s];
        uint64_t set_duration = 0;

        uint64_t start = __rdtscp(&dummy);
        for (size_t l = 0; l < NUM_LINES; l++)
        {
            // size_t debug_set = 0, debug_line = 0;
            // get_set_and_line_from_buffer_idx(idx, &debug_set, &debug_line);
            // assert(debug_set == set);

            idx = probe_buffer[idx];  // using get_set_and_line_from_buffer_idx() I verified that the idx corresponds to the expected set
        }
        uint64_t end = __rdtscp(&dummy);

        set_duration = (end - start);
        if (set_duration < 1500)  // appears to be way above reasonable
        {
            result[set] += set_duration;  // summing probe time for all lines in the set
            counts[set]++;  // to later calculate the average probe time for the set
        }
    }
}

void probe_set(size_t set, uint64_t result[NUM_LINES])
{
    /* 
        probes a single set, without using set permutations (i.e. `set` points to the actual set index to be accessed).
        measurements are done for each line in the set, and returned in the result array.
    */
    uint64_t start = 0, end = 0, duration = 0;
    uint32_t dummy = 0;
    uint16_t idx = GET_BUFFER_IDX(set, probe_line_order[0]);

    for (size_t l = 0; l < NUM_LINES; l++)
    {
        start = __rdtscp(&dummy);
        idx = probe_buffer[idx];
        end = __rdtscp(&dummy);
        duration = (end - start);
        if (duration < 200)
        {
            result[probe_line_order[l]] = duration;
        }
    }
}

uint64_t probe_set_whole_set_meas(size_t set)
{
    /*
        probes a single set, without using set permutations (i.e. `set` points to the actual set index to be accessed).
        measurements are done for the entire set, and the total time is returned.
    */
    uint64_t start = 0, end = 0;
    uint32_t dummy = 0;
    uint16_t idx = GET_BUFFER_IDX(set, probe_line_order[0]);

    start = __rdtscp(&dummy);
    for (size_t l = 0; l < NUM_LINES; l++)
    {
        // size_t debug_set = 0, debug_line = 0;
        // get_set_and_line_from_buffer_idx(idx, &debug_set, &debug_line);
        // assert(debug_set == set);

        idx = probe_buffer[idx];
    }
    end = __rdtscp(&dummy);
    return (end - start);
}

void prime_and_probe(size_t repetitions)
{
    uint64_t sum_results[NUM_SETS][VICTIM_NUM_LINES_OPTIONS][NUM_SETS] = {{{0}}};  // sum of probe times for each set, given victim set and line
    uint64_t count_results[NUM_SETS][VICTIM_NUM_LINES_OPTIONS][NUM_SETS] = {{{0}}};  // count of accepted (non-outlier) values for each victim set and line randomly chosen

    for (size_t i = 0; i < repetitions; i++)
    {
        prime();
        size_t victim_set = rand() % NUM_SETS;
        size_t victim_line_count = rand() % VICTIM_NUM_LINES_OPTIONS;
        victim(victim_set, victim_line_count);
        probe(&sum_results[victim_set][victim_line_count][0], &count_results[victim_set][victim_line_count][0]);
    }

    FILE *fp = fopen(PRIME_AND_PROBE_RESULTS_FILENAME, "w");
    fprintf(fp, "victim_set,victim_line_count,probe_set,count,sum_probe_time\n");
    for (size_t victim_set = 0; victim_set < NUM_SETS; victim_set++)
    {
        for (size_t victim_line_count = 0; victim_line_count < VICTIM_NUM_LINES_OPTIONS; victim_line_count++)
        {
            for (size_t set = 0; set < NUM_SETS; set++)
            {
                fprintf(fp, "%zu,%zu,%zu,%zu,%zu\n", victim_set, victim_line_count, set, count_results[victim_set][victim_line_count][set], sum_results[victim_set][victim_line_count][set]);
            }
        }
    }
    fclose(fp);
}

void prime_and_probe_set(size_t repetitions, size_t set, size_t lines)
{
    test_results_t results = {0};
    test_results_t *probe_times = calloc(repetitions, sizeof(test_results_t));

    for (size_t i = 0; i < repetitions; i++)
    {
        memset(&results, 0, sizeof(results));
        prime();
        probe_set(set, results.before);
        victim(set, lines);
        probe_set(set, results.after);
        // if (results.before[0] > 500 || results.after[0] > 500)
        // {
        //     continue; // will remain 0. to filter later.
        // }
        probe_times[i] = results;
    }

    char filename[sizeof(PRIME_AND_PROBE_SET_PER_LINE_RESULTS_FILENAME) + 20];
    snprintf(filename, sizeof(filename), PRIME_AND_PROBE_SET_PER_LINE_RESULTS_FILENAME, lines);
    printf("%s\n", filename);
    FILE *fp = fopen(filename, "w");
    fprintf(fp, "line,before,after\n");
    for (size_t i = 0; i < repetitions; i++)
    {
        for (size_t l = 0; l < NUM_LINES; l++)
        {
            fprintf(fp, "%hu,%lu,%lu\n", probe_line_order[l], (unsigned long)probe_times[i].before[probe_line_order[l]], (unsigned long)probe_times[i].after[probe_line_order[l]]);
        }
    }
    free(probe_times);
    fclose(fp);
}

struct probe_set_whole_set_meas_results
{
    uint64_t before;
    uint64_t after;
};

void prime_and_probe_set_whole_set_meas(size_t repetitions, size_t set, size_t lines)
{
    struct probe_set_whole_set_meas_results results = {0};
    struct probe_set_whole_set_meas_results *probe_times = calloc(repetitions, sizeof(results));

    for (size_t i = 0; i < repetitions; i++)
    {
        memset(&results, 0, sizeof(results));
        prime();
        results.before = probe_set_whole_set_meas(set);
        victim(set, lines);
        results.after = probe_set_whole_set_meas(set);

        // if (results.before > 300 || results.after > 300)
        // {
        //     continue; // will remain 0. to filter later.
        // }
        probe_times[i] = results;
    }

    char filename[sizeof(PRIME_AND_PROBE_SET_RESULTS_FILENAME) + 20];
    snprintf(filename, sizeof(filename), PRIME_AND_PROBE_SET_RESULTS_FILENAME, lines);
    printf("%s\n", filename);
    FILE *fp = fopen(filename, "w");
    fprintf(fp, "before,after\n");
    for (size_t i = 0; i < repetitions; i++)
    {
        for (size_t l = 0; l < NUM_LINES; l++)
        {
            fprintf(fp, "%lu,%lu\n", (unsigned long)probe_times[i].before, (unsigned long)probe_times[i].after);
        }
    }
    free(probe_times);
    fclose(fp);
}

int main(void)
{
    ppinit();

    prime_and_probe(REPETITIONS);
    // for (size_t lines = 0; lines <= NUM_LINES; lines++)
    // {
    //     // prime_and_probe_set(REPETITIONS, 17, lines);
    //     prime_and_probe_set_whole_set_meas(REPETITIONS, 17, lines);
    // }

    return 0;
}
