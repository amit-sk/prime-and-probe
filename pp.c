#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <x86intrin.h>
#include "consts.h"

#define REPETITIONS (100000)
#define VICTIM_NUM_LINES_OPTIONS (13)

static volatile uint8_t buffer[NUM_SETS * NUM_LINES * BLOCK_SIZE] __attribute__((aligned(4096))) = {0};
static size_t set_order[NUM_SETS] = {0};
static size_t line_order[NUM_LINES] = {0};

void fisher_yates_shuffle(size_t *array, size_t n)
{
    for (size_t i = n - 1; i > 0; i--)
    {
        size_t j = rand() % (i + 1);
        size_t temp = array[i];
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

void ppinit(void)
{    
    init_orderings();

    /* provided code for ramping up CPU */
    uint32_t dummy;
    uint64_t start = __rdtscp(&dummy);
    while (__rdtscp(&dummy) - start < 1000000000) ;
}

void prime(void)
{
    volatile uint8_t temp = 0;
    for (size_t s = 0; s < NUM_SETS; s++)
    {
        size_t set = set_order[s];

        for (size_t l = 0; l < NUM_LINES; l++)
        {
            size_t line = line_order[l];

            temp ^= buffer[(line * LINE_SEPARATION_IN_BYTES) + (set * BLOCK_SIZE)];
        }
    }
    (void)temp;
}

void probe(uint64_t result[NUM_SETS])
{
    uint32_t dummy = 0;
    volatile uint8_t temp = 0;

    for (size_t s = 0; s < NUM_SETS; s++)
    {
        size_t set = set_order[s];

        for (size_t l = 0; l < NUM_LINES; l++)
        {
            size_t line = line_order[l];

            uint64_t start = __rdtscp(&dummy);
            temp ^= buffer[(line * LINE_SEPARATION_IN_BYTES) + (set * BLOCK_SIZE)];
            uint64_t end = __rdtscp(&dummy);

            /* TODO: remove outliers (?) */
            result[set] += (end - start);  // summing probe time for all lines in the set
        }
    }
}

int main(void)
{
    ppinit();

    uint64_t sum_results[NUM_SETS][VICTIM_NUM_LINES_OPTIONS][NUM_SETS] = {{{0}}};  // sum of probe times for each set, given victim set and line
    uint64_t count_results[NUM_SETS][VICTIM_NUM_LINES_OPTIONS] = {{0}};  // count of each victim set and line randomly chosen

    for (size_t i = 0; i < REPETITIONS; i++)
    {
        prime();
        /* TODO: choose set and line */
        size_t victim_set = rand() % NUM_SETS;
        size_t victim_line = rand() % VICTIM_NUM_LINES_OPTIONS;
        count_results[victim_set][victim_line]++;
        /* TODO: victim */
        probe(&sum_results[victim_set][victim_line][0]);  // TODO: pass correct victim set and line
    }

    /* TODO: report results in csv format */
    printf("victim_set,victim_line,probe_set,count,sum_probe_time\n");
    for (size_t victim_set = 0; victim_set < NUM_SETS; victim_set++)
    {
        for (size_t victim_line = 0; victim_line < VICTIM_NUM_LINES_OPTIONS; victim_line++)
        {
            for (size_t set = 0; set < NUM_SETS; set++)
            {
                printf("%zu,%zu,%zu,%zu,%zu\n", victim_set, victim_line, set, count_results[victim_set][victim_line], sum_results[victim_set][victim_line][set]);
            }
        }
    }
    return 0;
}
