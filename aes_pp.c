#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "aes_utils.h"
#include "AES/aes.h"

int main(int argc, char **argv)
{
    aes_t key;
    AES_KEY aeskey;
    unsigned long long n;

    if (argc != 3) {
        fprintf(stderr, "Usage: %s <32-hex-digit-key> <number-of-encryptions>\n", argv[0]);
        return EXIT_FAILURE;
    }

    if (tobinary(argv[1], key) != 0) {
        fprintf(stderr, "Error: the key must be exactly 32 hexadecimal digits.\n");
        return EXIT_FAILURE;
    }

    if (parse_count(argv[2], &n) != 0) {
        fprintf(stderr, "Error: the number of encryptions must be a nonnegative integer.\n");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}