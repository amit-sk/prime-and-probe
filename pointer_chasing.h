#include <stdint.h>
#include <stddef.h>

#define GET_BUFFER_IDX(set, line) ((((line) * LINE_SEPARATION_IN_BYTES) + ((set) * BLOCK_SIZE)) / sizeof(uint16_t))

void print_set_and_line_from_elem_index(uint16_t idx);
void init_orderings(uint16_t *set_order, uint16_t *line_order);
void init_linked_list_structure(uint16_t *set_order, uint16_t *line_order, volatile uint16_t *buffer);
