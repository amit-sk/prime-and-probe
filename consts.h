#ifndef CONSTS_H
#define CONSTS_H

#define BLOCK_SIZE (64)
#define NUM_SETS (64)
#define LINE_SEPERATION_IN_BYTES (NUM_SETS * BLOCK_SIZE)

// Assuming assignment to logical cores 0-11
#define NUM_LINES (12)

#endif /* CONSTS_H */
