#include <stdint.h>

#define AESSIZE 16
typedef uint8_t aes_t[AESSIZE];

/*
 * Convert 32 hexadecimal characters into a 16-byte AES value.
 * Returns 0 on success and -1 on invalid input.
 */
int tobinary(const char *data, aes_t aes);

/* Convert a 16-byte AES value into 32 hexadecimal characters. */
void tostring(const aes_t aes, char text[AESSIZE * 2 + 1]);

/* Generate a random 16-byte plaintext. */
void randaes(aes_t aes);

/*
 * Parse a string as an unsigned long long integer (for argv parsing).
 * Returns 0 on success and -1 on invalid input.
 */
int parse_count(const char *text, unsigned long long *count);
