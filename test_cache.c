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
int main(void)
{
    const size_t test_set = 0;
    const size_t test_line = 0;
    volatile uint8_t temp = 0;
    uint64_t start, end;
    uint32_t dummy;
    uint64_t results[NUM_TESTS][2] = {{0}};

    init();

    for (size_t i = 0; i < NUM_TESTS; i++)
    {
        temp ^= buffer[(test_line * LINE_SEPARATION_IN_BYTES) + (test_set * BLOCK_SIZE)];

        start = __rdtscp(&dummy);
        temp ^= buffer[(test_line * LINE_SEPARATION_IN_BYTES) + (test_set * BLOCK_SIZE)];
        end = __rdtscp(&dummy);
        uint64_t accesss_duration = end - start;

        _mm_clflush((const void *)&buffer[(test_line * LINE_SEPARATION_IN_BYTES) + (test_set * BLOCK_SIZE)]);
        _mm_mfence();

        start = __rdtscp(&dummy);
        temp ^= buffer[(test_line * LINE_SEPARATION_IN_BYTES) + (test_set * BLOCK_SIZE)];
        end = __rdtscp(&dummy);
        uint64_t after_flush_duration = end - start;

        results[i][0] = accesss_duration;
        results[i][1] = after_flush_duration;
    }

    printf("duration_in_cache,duration_after_clflush\n");
    for (size_t i = 0; i < NUM_TESTS; i++)
    {
        printf("%lu,%lu\n", results[i][0], results[i][1]);
    }

    return 0;
}