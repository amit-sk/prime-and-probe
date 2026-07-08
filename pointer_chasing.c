#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "consts.h"
#include "pointer_chasing.h"

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

void init_orderings(uint16_t *set_order, uint16_t *line_order)
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

void init_linked_list_structure(uint16_t *set_order, uint16_t *line_order, volatile uint16_t *buffer)
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
