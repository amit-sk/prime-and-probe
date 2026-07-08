#ifndef CONSTS_H
#define CONSTS_H

#define BLOCK_SIZE (64)
#define NUM_SETS (64)
#define LINE_SEPARATION_IN_BYTES (NUM_SETS * BLOCK_SIZE)
#define NUM_LINES (12)  // Assuming assignment to logical cores 0-11
#define CACHE_SIZE (NUM_SETS * NUM_LINES * BLOCK_SIZE)

// assignment allows values from 0-12
#define VICTIM_NUM_LINES_OPTIONS (NUM_LINES + 1)

#endif /* CONSTS_H */
