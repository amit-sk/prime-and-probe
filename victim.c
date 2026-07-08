#include <x86intrin.h>
#include "pointer_chasing.h"
#include "consts.h"
#include "victim.h"

static volatile uint16_t victim_buffer[NUM_SETS * NUM_LINES * BLOCK_SIZE / sizeof(uint16_t)] __attribute__((aligned(4096))) = {0};
static uint16_t victim_set_order[NUM_SETS] = {0};  // victim only ever accesses one set at a time, so actually no need for set permutations
static uint16_t victim_line_order[NUM_LINES] = {0};

void init_victim(void)
{
    init_orderings(victim_set_order, victim_line_order);
    init_linked_list_structure(victim_set_order, victim_line_order, victim_buffer);
}

void victim(size_t victim_set, size_t victim_lines)
{
    if (victim_set >= NUM_SETS || victim_lines == 0 || victim_lines > NUM_LINES)
    {
        return;
    }
    
    _mm_mfence();

    uint16_t idx = GET_BUFFER_IDX(victim_set, victim_line_order[0]);
    for (size_t l = 0; l < victim_lines; l++)
    {
        idx = victim_buffer[idx];
    }
}
