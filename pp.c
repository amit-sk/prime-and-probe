#include <stdint.h>
#include <stddef.h>
#include "consts.h"

static uint8_t buffer[NUM_SETS * NUM_LINES * BLOCK_SIZE] = {0};
static uint8_t set_order[NUM_SETS] = {0};
static uint8_t line_order[NUM_LINES] = {0};

void fisher_yates_shuffle(uint8_t *array, size_t n)
{
    for (size_t i = n - 1; i > 0; i--)
    {
        size_t j = rand() % (i + 1);
        uint8_t temp = array[i];
        array[i] = array[j];
        array[j] = temp;
    }
}

void init_orderings(void)
{
    for (size_t i = 0; i < sizeof(set_order); i++)
    {
        set_order[i] = i;
    }
    for (size_t i = 0; i < sizeof(line_order); i++)
    {
        line_order[i] = i;
    }

    fisher_yates_shuffle(set_order, sizeof(set_order));
    fisher_yates_shuffle(line_order, sizeof(line_order));
}

void ppinit(void)
{    
    init_orderings();
    int dummy;
    uint64_t start = rdtscp(&dummy);
    while (rdtscp(&dummy) - start < 1000000000) ;
}

void prime(void)
{
    buffer[(line * LINE_SEPERATION_IN_BYTES) + (set * BLOCK_SIZE)] = 1;

}

void probe(uint64_t result[NUM_SETS])
{

}
