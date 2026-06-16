#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <x86intrin.h>
#include <inttypes.h>
#include <limits.h>
#include <string.h>

#include "consts.h"
#include "victim.h"

#define REPETITIONS (1000000)
#define GET_BUFFER_IDX(set, line) ((((line) * LINE_SEPARATION_IN_BYTES) + ((set) * BLOCK_SIZE)) / sizeof(uint16_t))

// static volatile uint8_t buffer[NUM_SETS * NUM_LINES * BLOCK_SIZE] __attribute__((aligned(4096))) = {0};
static volatile uint16_t buffer[NUM_SETS * NUM_LINES * BLOCK_SIZE / sizeof(uint16_t)] __attribute__((aligned(4096))) = {0};
static uint16_t set_order[NUM_SETS] = {0};
static uint16_t line_order[NUM_LINES] = {0};

#define BUFFER_NUM_ELEMS (NUM_SETS * NUM_LINES * BLOCK_SIZE / sizeof(uint16_t))
_Static_assert(BUFFER_NUM_ELEMS <= UINT16_MAX, "uint16_t is too small to index the buffer");


// for debugging pointer chasing
void print_set_and_line_from_elem_index(uint16_t idx)
{
    size_t byte_offset = (size_t)idx * sizeof(uint16_t);

    size_t line = byte_offset / LINE_SEPARATION_IN_BYTES;
    size_t rem  = byte_offset % LINE_SEPARATION_IN_BYTES;
    size_t set  = rem / BLOCK_SIZE;

    printf("idx=%u, set=%zu, line=%zu\n", idx, set, line);
}

void fisher_yates_shuffle(uint16_t *array, size_t n)
{
    for (size_t i = n - 1; i > 0; i--)
    {
        size_t j = rand() % (i + 1);
        uint16_t temp = array[i];
        array[i] = array[j];
        array[j] = temp;
    }
}

void init_orderings(void)
{
    for (size_t i = 0; i < NUM_SETS; i++)
    {
        set_order[i] = i;
    }
    for (size_t i = 0; i < NUM_LINES; i++)
    {
        line_order[i] = i;
    }

    fisher_yates_shuffle(set_order, NUM_SETS);
    fisher_yates_shuffle(line_order, NUM_LINES);
}

void init_linked_list_structure(void)
{
    /*
    * iterate through lines for each set in a random order, then move to the next set in a random order,
    * according to the pre-shuffled set_order and line_order arrays 
    */
    for (uint16_t s = 0; s < NUM_SETS; s++)
    {
        uint16_t set = set_order[s];
        for (uint16_t l = 0; l < NUM_LINES; l++)
        {
            uint16_t line = line_order[l];
            uint16_t curr_idx = GET_BUFFER_IDX(set, line);
            
            // Calculate next indices
            uint16_t next_l = (l + 1) % NUM_LINES;
            uint16_t next_s = (next_l == 0) ? (s + 1) % NUM_SETS : s;
            uint16_t next_set = set_order[next_s];
            uint16_t next_line = line_order[next_l];
            uint16_t next_idx = GET_BUFFER_IDX(next_set, next_line);
            
            buffer[curr_idx] = next_idx;
        }
    }
}

void ppinit(void)
{    
    init_orderings();
    init_linked_list_structure();

    /* provided code for ramping up CPU */
    uint32_t dummy;
    uint64_t start = __rdtscp(&dummy);
    while (__rdtscp(&dummy) - start < 1000000000) ;
}

void prime(void)
{
    uint16_t idx = GET_BUFFER_IDX(set_order[0], line_order[0]);

    for (size_t s = 0; s < NUM_SETS; s++)
    {
        for (size_t l = 0; l < NUM_LINES; l++)
        {
            idx = buffer[idx];
        }
    }
}

void probe(uint64_t result[NUM_SETS], uint64_t counts[NUM_SETS])
{
    uint32_t dummy = 0;
    uint16_t idx = GET_BUFFER_IDX(set_order[0], line_order[0]);

    for (size_t s = 0; s < NUM_SETS; s++)
    {
        uint16_t set = set_order[s];
        uint64_t set_duration = 0;

        uint64_t start = __rdtscp(&dummy);
        for (size_t l = 0; l < NUM_LINES; l++)
        {
            idx = buffer[idx];
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

void probe17(uint64_t result[NUM_LINES])
{
    uint64_t start = 0, end = 0, duration = 0;
    uint32_t dummy = 0;
    const size_t set = set_order[17];
    uint16_t idx = GET_BUFFER_IDX(set, line_order[0]);

    for (size_t l = 0; l < NUM_LINES; l++)
    {
        start = __rdtscp(&dummy);
        idx = buffer[idx];
        end = __rdtscp(&dummy);
        duration = (end - start);
        if (duration < 200)
        {
            result[line_order[l]] = duration;
        }
    }
}

typedef struct
{
    uint64_t before[NUM_LINES];
    uint64_t after[NUM_LINES];
} test_results_t;

int main(void)
{
    ppinit();

    // uint64_t sum_results[NUM_SETS][VICTIM_NUM_LINES_OPTIONS][NUM_SETS] = {{{0}}};  // sum of probe times for each set, given victim set and line
    // uint64_t count_results[NUM_SETS][VICTIM_NUM_LINES_OPTIONS][NUM_SETS] = {{{0}}};  // count of accepted (non-outlier) values for each victim set and line randomly chosen
    test_results_t results = {0};
    test_results_t *probe_times = calloc(REPETITIONS, sizeof(test_results_t));

    for (size_t i = 0; i < REPETITIONS; i++)
    {
        // prime();
        // size_t victim_set = rand() % NUM_SETS;
        // size_t victim_line_count = rand() % VICTIM_NUM_LINES_OPTIONS;
        // victim(victim_set, victim_line_count);
        // probe(&sum_results[victim_set][victim_line_count][0], &count_results[victim_set][victim_line_count][0]);

        memset(&results, 0, sizeof(results));
        prime();
        probe17(results.before);
        victim(set_order[17], 10);
        probe17(results.after);
        // if (results.before[0] > 500 || results.after[0] > 500)
        // {
        //     continue; // will remain 0. to filter later.
        // }
        memcpy(&probe_times[i], &results, sizeof(test_results_t));
    }

    printf("line,before,after\n");
    for (size_t i = 0; i < REPETITIONS; i++)
    {
        for (size_t l = 0; l < NUM_LINES; l++)
        {
            printf("%hu,%lu,%lu\n", line_order[l], (unsigned long)probe_times[i].before[line_order[l]], (unsigned long)probe_times[i].after[line_order[l]]);
        }
    }

    // printf("victim_set,victim_line_count,probe_set,count,sum_probe_time\n");
    // for (size_t victim_set = 0; victim_set < NUM_SETS; victim_set++)
    // {
    //     for (size_t victim_line_count = 0; victim_line_count < VICTIM_NUM_LINES_OPTIONS; victim_line_count++)
    //     {
    //         for (size_t set = 0; set < NUM_SETS; set++)
    //         {
    //             printf("%zu,%zu,%zu,%zu,%zu\n", victim_set, victim_line_count, set, count_results[victim_set][victim_line_count][set], sum_results[victim_set][victim_line_count][set]);
    //         }
    //     }
    // }
    return 0;
}
