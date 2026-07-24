#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <x86intrin.h>
#include <inttypes.h>
#include <limits.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <sys/stat.h>

#include "consts.h"
#include "victim.h"
#include "pointer_chasing.h"
#include "pp.h"

#define REPETITIONS (1000000)

#define PRIME_AND_PROBE_RESULTS_FILENAME ("./results/raw/results.csv")
#define PRIME_AND_PROBE_SET_RESULTS_FILENAME ("./results/raw/results_set_%zu_lines.csv")
#define PRIME_AND_PROBE_SET_PER_LINE_RESULTS_FILENAME ("./results/raw/results_set_per_line_%zu_lines.csv")

// static volatile uint8_t buffer[NUM_SETS * NUM_LINES * BLOCK_SIZE] __attribute__((aligned(4096))) = {0};
static volatile doubly_linked_list_elem_t buffer[NUM_SETS * NUM_LINES * BLOCK_SIZE / sizeof(doubly_linked_list_elem_t)] __attribute__((aligned(4096))) = {0};
static uint16_t set_order[NUM_SETS] = {0};
static uint16_t line_order[NUM_LINES] = {0};

#define BUFFER_NUM_ELEMS (NUM_SETS * NUM_LINES * BLOCK_SIZE / sizeof(doubly_linked_list_elem_t))
// verifying underlying type assumptions to ensure type is valid to index the buffer.     
_Static_assert(__builtin_types_compatible_p(__typeof__(buffer[0].next), uint16_t), "expecting uint16_t indexes");
_Static_assert(BUFFER_NUM_ELEMS <= UINT16_MAX, "uint16_t is too small to index the buffer");

// for probing single set per line
typedef struct
{
    uint64_t before[NUM_LINES];
    uint64_t after[NUM_LINES];
} probe_per_line_results_t;

typedef struct
{
    size_t victim_set;
    size_t victim_line_count;
    uint64_t probe[NUM_SETS];
} probe_results_t;

void ppinit(void)
{    
    init_doubly_linked_list_structure(set_order, line_order, buffer);
    init_victim();

    /* provided code for ramping up CPU */
    uint32_t dummy;
    uint64_t start = __rdtscp(&dummy);
    while (__rdtscp(&dummy) - start < 1000000000) ;
}

void prime(void)
{
    uint16_t head = GET_BUFFER_IDX(set_order[0], line_order[0]);
    uint16_t idx = head;

    do {
        idx = buffer[idx].prev;
        _mm_mfence();
    } while (idx != head);
}

void probe(uint64_t result[NUM_SETS])
{
    uint32_t dummy = 0;
    uint16_t idx = GET_BUFFER_IDX(set_order[0], line_order[0]);  // probing all sets in the order of the permutation

    for (size_t s = 0; s < NUM_SETS; s++)
    {
        uint16_t set = set_order[s];

        uint64_t start = __rdtscp(&dummy);
        for (size_t l = 0; l < NUM_LINES; l++)
        {
            // size_t debug_set = 0, debug_line = 0;
            // get_set_and_line_from_buffer_idx(idx, &debug_set, &debug_line);
            // assert(debug_set == set);

            idx = buffer[idx].next;  // using get_set_and_line_from_buffer_idx() I verified that the idx corresponds to the expected set
        }
        uint64_t end = __rdtscp(&dummy);
        result[set] = (end - start);
    }
}

void probe_set_per_line(size_t set, uint64_t result[NUM_LINES])
{
    uint64_t start = 0, end = 0, duration = 0;
    uint32_t dummy = 0;
    uint16_t idx = GET_BUFFER_IDX(set, line_order[0]);

    for (size_t l = 0; l < NUM_LINES; l++)
    {
        start = __rdtscp(&dummy);
        idx = buffer[idx].next;
        end = __rdtscp(&dummy);
        duration = (end - start);
        if (duration < 200)
        {
            result[line_order[l]] = duration;
        }
    }
}

uint64_t probe_set(size_t set)
{
    uint64_t start = 0, end = 0;
    uint32_t dummy = 0;
    uint16_t idx = GET_BUFFER_IDX(set, line_order[0]);

    start = __rdtscp(&dummy);
    for (size_t l = 0; l < NUM_LINES; l++)
    {
        // size_t debug_set = 0, debug_line = 0;
        // get_set_and_line_from_buffer_idx(idx, &debug_set, &debug_line);
        // assert(debug_set == set);

        idx = buffer[idx].next;
    }
    end = __rdtscp(&dummy);
    return (end - start);
}

void prime_and_probe(size_t repetitions)
{
    probe_results_t result = {0};
    probe_results_t *probe_times = calloc(repetitions, sizeof(probe_results_t));
    assert(probe_times != NULL);

    for (size_t i = 0; i < repetitions; i++)
    {
        size_t victim_set = rand() % NUM_SETS;
        size_t victim_line_count = rand() % VICTIM_NUM_LINES_OPTIONS;

        prime();
        victim(victim_set, victim_line_count);
        probe(result.probe);

        result.victim_set = victim_set;
        result.victim_line_count = victim_line_count;
        probe_times[i] = result;
    }

    FILE *fp = fopen(PRIME_AND_PROBE_RESULTS_FILENAME, "w");
    assert(fp != NULL);
    fprintf(fp, "victim_set,victim_line_count,probe_set,probe_time\n");
    for (size_t i = 0; i < repetitions; i++)
    {
        for (size_t set = 0; set < NUM_SETS; set++)
        {
            fprintf(fp, "%zu,%zu,%zu,%zu\n", probe_times[i].victim_set, probe_times[i].victim_line_count, set, probe_times[i].probe[set]);
        }
    }
    fclose(fp);
    free(probe_times);
}

void prime_and_probe_set_per_line(size_t repetitions, size_t set, size_t lines)
{
    probe_per_line_results_t results = {0};
    probe_per_line_results_t *probe_times = calloc(repetitions, sizeof(probe_per_line_results_t));

    for (size_t i = 0; i < repetitions; i++)
    {
        memset(&results, 0, sizeof(results));
        prime();
        probe_set_per_line(set, results.before);
        victim(set, lines);
        probe_set_per_line(set, results.after);
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
    assert(fp != NULL);
    fprintf(fp, "line,before,after\n");
    for (size_t i = 0; i < repetitions; i++)
    {
        for (size_t l = 0; l < NUM_LINES; l++)
        {
            fprintf(fp, "%hu,%lu,%lu\n", line_order[l], (unsigned long)probe_times[i].before[line_order[l]], (unsigned long)probe_times[i].after[line_order[l]]);
        }
    }
    free(probe_times);
    fclose(fp);
}

typedef struct
{
    uint64_t before;
    uint64_t after;
} probe_set_results_t;

void prime_and_probe_set(size_t repetitions, size_t set, size_t lines)
{
    probe_set_results_t results = {0};
    probe_set_results_t *probe_times = calloc(repetitions, sizeof(results));
    size_t before = 0, after = 0;

    for (size_t i = 0; i < repetitions; i++)
    {
        memset(&results, 0, sizeof(results));
        prime();
        before = probe_set(set);
        victim(set, lines);
        after = probe_set(set);

        _mm_mfence();
        results.before = before;
        results.after = after;

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
    assert(fp != NULL);
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
    assert(mkdir("./results", 0755) == 0 || errno == EEXIST);
    assert(mkdir("./results/raw", 0755) == 0 || errno == EEXIST);

    ppinit();

    prime_and_probe(REPETITIONS);
    // for (size_t lines = 0; lines <= NUM_LINES; lines++)
    // {
    //     // prime_and_probe_set_per_line(REPETITIONS, 17, lines);
    //     prime_and_probe_set(REPETITIONS, 17, lines);
    // }

    return 0;
}
