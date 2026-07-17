#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "consts.h"
#include "pointer_chasing.h"

void get_set_and_line_from_buffer_idx(uint16_t idx, size_t *set, size_t *line)
{
    size_t byte_offset = (size_t)idx * sizeof(uint16_t);

    *line = byte_offset / LINE_SEPARATION_IN_BYTES;
    *set = (byte_offset % LINE_SEPARATION_IN_BYTES) / BLOCK_SIZE;
}

// for debugging pointer chasing
void print_set_and_line_from_elem_index(uint16_t idx)
{
    size_t set;
    size_t line;

    get_set_and_line_from_buffer_idx(idx, &set, &line);
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
    * create shuffled set_order and line_order arrays, then use them to initialize the buffer as a linked list that
    * iterates through all lines for each specific set in a random line order, then move to the next set in the random set order,
    * according to the shuffled set_order and line_order arrays 
    */
   init_orderings(set_order, line_order);

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

void create_opposite_linked_list_structure(volatile uint16_t *source_buffer, volatile uint16_t *dest_buffer)
{
    uint16_t head = GET_BUFFER_IDX(0, 0);
    uint16_t src_idx = GET_BUFFER_IDX(0, 0);

    do {
        uint16_t next_idx = source_buffer[src_idx];
        dest_buffer[next_idx] = src_idx;
        src_idx = next_idx;
    } while (src_idx != head);
}

void init_opposite_linked_list_structure(uint16_t *src_set_order, uint16_t *src_line_order, volatile uint16_t *source_buffer, uint16_t *dest_set_order, uint16_t *line_order, volatile uint16_t *dest_buffer)
{
    /*
    * create a linked list in dest_buffer that is the opposite of the linked list in source_buffer
    * creating also the set_order and line_order arrays for the dest_buffer linked list
    */
    create_opposite_linked_list_structure(source_buffer, dest_buffer);
    for (size_t i = 0; i < NUM_SETS; i++)
    {
        dest_set_order[i] = src_set_order[NUM_SETS - 1 - i];
    }
    for (size_t i = 0; i < NUM_LINES; i++)
    {
        line_order[i] = src_line_order[NUM_LINES - 1 - i];
    }
}
