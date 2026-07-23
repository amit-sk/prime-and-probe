#include <stdint.h>
#include <stddef.h>

typedef struct
{
    uint16_t prev;
    uint16_t next;
} doubly_linked_list_elem_t;

#define GET_BUFFER_IDX(set, line) ((((line) * LINE_SEPARATION_IN_BYTES) + ((set) * BLOCK_SIZE)) / sizeof(doubly_linked_list_elem_t))

void get_set_and_line_from_buffer_idx(uint16_t idx, size_t *set, size_t *line);
void print_set_and_line_from_elem_index(uint16_t idx);
void init_doubly_linked_list_structure(uint16_t *set_order, uint16_t *line_order, volatile doubly_linked_list_elem_t *buffer);
