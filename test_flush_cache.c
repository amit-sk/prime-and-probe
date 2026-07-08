#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <x86intrin.h>
#include "consts.h"

#define NUM_TESTS (10000)

static volatile uint8_t buffer[NUM_SETS * NUM_LINES * BLOCK_SIZE] __attribute__((aligned(4096))) = {0};

void init(void)
{
    /* provided code for ramping up CPU */
    uint32_t dummy;
    uint64_t start = __rdtscp(&dummy);
    while (__rdtscp(&dummy) - start < 1000000000) ;
}

void prime_entire_set(size_t set)
{
    volatile uint8_t temp = 0;
    for (size_t l = 0; l < NUM_LINES; l++)
    {
        temp ^= buffer[(l * LINE_SEPARATION_IN_BYTES) + (set * BLOCK_SIZE)];
    }
}

void test_cache_set(size_t set, uint64_t repetitions)
{
    uint64_t start, end;
    uint32_t dummy;
    uint64_t results[repetitions][2];  // [][0] = duration in cache, [][1] = duration after clflush

    for (size_t i = 0; i < repetitions; i++)
    {
        prime_entire_set(set);

        start = __rdtscp(&dummy);
        prime_entire_set(set);
        end = __rdtscp(&dummy);
        uint64_t accesss_duration = end - start;

        for (size_t l = 0; l < NUM_LINES; l++)
        {
            _mm_clflush((const void *)&buffer[(l * LINE_SEPARATION_IN_BYTES) + (set * BLOCK_SIZE)]);
        }
        _mm_mfence();

        start = __rdtscp(&dummy);
        prime_entire_set(set);
        end = __rdtscp(&dummy);
        uint64_t after_flush_duration = end - start;

        results[i][0] = accesss_duration;
        results[i][1] = after_flush_duration;
    }

    printf("duration_in_cache,duration_after_clflush\n");
    for (size_t i = 0; i < repetitions; i++)
    {
        printf("%lu,%lu\n", results[i][0], results[i][1]);
    }
}

void test_cache_set_flush_single_line(size_t set, uint64_t repetitions)
{
    size_t line = 0;  // flushing the first line in the set (so the prefetcher won't affect the results)
    uint64_t start, end;
    uint32_t dummy;
    uint64_t results[repetitions][2];  // [][0] = duration in cache, [][1] = duration after clflush

    for (size_t i = 0; i < repetitions; i++)
    {
        prime_entire_set(set);

        start = __rdtscp(&dummy);
        prime_entire_set(set);
        end = __rdtscp(&dummy);
        uint64_t accesss_duration = end - start;

        _mm_clflush((const void *)&buffer[(line * LINE_SEPARATION_IN_BYTES) + (set * BLOCK_SIZE)]);
        _mm_mfence();

        start = __rdtscp(&dummy);
        prime_entire_set(set);
        end = __rdtscp(&dummy);
        uint64_t after_flush_duration = end - start;

        results[i][0] = accesss_duration;
        results[i][1] = after_flush_duration;
    }

    printf("duration_in_cache,duration_after_clflush\n");
    for (size_t i = 0; i < repetitions; i++)
    {
        printf("%lu,%lu\n", results[i][0], results[i][1]);
    }
}

void test_cache_line(size_t set, size_t line, uint64_t repetitions)
{
    volatile uint8_t temp = 0;
    uint64_t start, end;
    uint32_t dummy;
    uint64_t results[repetitions][2];  // [][0] = duration in cache, [][1] = duration after clflush

    for (size_t i = 0; i < repetitions; i++)
    {
        temp ^= buffer[(line * LINE_SEPARATION_IN_BYTES) + (set * BLOCK_SIZE)];

        start = __rdtscp(&dummy);
        temp ^= buffer[(line * LINE_SEPARATION_IN_BYTES) + (set * BLOCK_SIZE)];
        end = __rdtscp(&dummy);
        uint64_t accesss_duration = end - start;

        _mm_clflush((const void *)&buffer[(line * LINE_SEPARATION_IN_BYTES) + (set * BLOCK_SIZE)]);
        _mm_mfence();

        start = __rdtscp(&dummy);
        temp ^= buffer[(line * LINE_SEPARATION_IN_BYTES) + (set * BLOCK_SIZE)];
        end = __rdtscp(&dummy);
        uint64_t after_flush_duration = end - start;

        results[i][0] = accesss_duration;
        results[i][1] = after_flush_duration;
    }

    printf("duration_in_cache,duration_after_clflush\n");
    for (size_t i = 0; i < repetitions; i++)
    {
        printf("%lu,%lu\n", results[i][0], results[i][1]);
    }
}

int main(void)
{
    const size_t test_set = 0;
    const size_t test_line = 0;

    init();

    // test_cache_line(test_set, test_line, NUM_TESTS);
    // test_cache_set(test_set, NUM_TESTS);
    test_cache_set_flush_single_line(test_set, NUM_TESTS);

    return 0;
}
