#include "victim.h"
#include "consts.h"

static volatile uint8_t buffer[NUM_SETS * NUM_LINES * BLOCK_SIZE] __attribute__((aligned(4096))) = {0};

void victim(size_t victim_set, size_t victim_lines)
{
    if (victim_set >= NUM_SETS || victim_lines == 0 || victim_lines > NUM_LINES)
    {
        return;
    }
    
    volatile uint8_t temp = 0;
    for (size_t l = 0; l < victim_lines; l++)
    {
        temp ^= buffer[(l * LINE_SEPARATION_IN_BYTES) + (victim_set * BLOCK_SIZE)];
    }

    (void)temp;
}
