#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <x86intrin.h>
#include <inttypes.h>
#include <limits.h>

#include "consts.h"
#include "victim.h"

#define REPETITIONS (1000000)
#define GET_BUFFER_IDX(set, line) (((line * NUM_SETS) + (set * BLOCK_SIZE)) / sizeof(uint16_t))

// static volatile uint8_t buffer[NUM_SETS * NUM_LINES * BLOCK_SIZE] __attribute__((aligned(4096))) = {0};
static volatile uint16_t buffer[NUM_SETS * NUM_LINES * BLOCK_SIZE / sizeof(uint16_t)] __attribute__((aligned(4096))) = {0};
static uint16_t set_order[NUM_SETS] = {0};
static uint16_t line_order[NUM_LINES] = {0};

#define BUFFER_NUM_ELEMS (NUM_SETS * NUM_LINES * BLOCK_SIZE / sizeof(uint16_t))
_Static_assert(BUFFER_NUM_ELEMS <= UINT16_MAX, "uint16_t is too small to index the buffer");


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

uint64_t probe17()
{
    uint64_t start = 0, end = 0;
    uint32_t dummy = 0;
    const size_t set = set_order[17];
    uint16_t idx = GET_BUFFER_IDX(set, line_order[0]);

    start = __rdtscp(&dummy);
    for (size_t l = 0; l < NUM_LINES; l++)
    {
        idx = buffer[idx];
    }
    end = __rdtscp(&dummy);
    return (end - start);
}

int main(void)
{
    ppinit();

    uint64_t sum_results[NUM_SETS][VICTIM_NUM_LINES_OPTIONS][NUM_SETS] = {{{0}}};  // sum of probe times for each set, given victim set and line
    uint64_t count_results[NUM_SETS][VICTIM_NUM_LINES_OPTIONS][NUM_SETS] = {{{0}}};  // count of accepted (non-outlier) values for each victim set and line randomly chosen
    // uint64_t *probe_times = calloc(REPETITIONS, sizeof(uint64_t[2]));

    for (size_t i = 0; i < REPETITIONS; i++)
    {
        prime();
        size_t victim_set = rand() % NUM_SETS;
        size_t victim_line = rand() % VICTIM_NUM_LINES_OPTIONS;
        victim(victim_set, victim_line);
        probe(&sum_results[victim_set][victim_line][0], &count_results[victim_set][victim_line][0]);

        // uint64_t before = probe17();
        // victim(set_order[17], 10);
        // uint64_t after = probe17();
        // if (before > 500 || after > 500)
        // {
        //     continue; // will remain 0. to filter later.
        // }
        // probe_times[i*2] = before;
        // probe_times[i*2 + 1] = after;
    }

    // printf("before,after\n");
    // for (size_t i = 0; i < REPETITIONS; i++)    {
    //     printf("%" PRIu64 ",%" PRIu64 "\n", probe_times[i*2], probe_times[i*2 + 1]);
    // }

    printf("victim_set,victim_line,probe_set,count,sum_probe_time\n");
    for (size_t victim_set = 0; victim_set < NUM_SETS; victim_set++)
    {
        for (size_t victim_line = 0; victim_line < VICTIM_NUM_LINES_OPTIONS; victim_line++)
        {
            for (size_t set = 0; set < NUM_SETS; set++)
            {
                printf("%zu,%zu,%zu,%zu,%zu\n", victim_set, victim_line, set, count_results[victim_set][victim_line][set], sum_results[victim_set][victim_line][set]);
            }
        }
    }
    return 0;
}
