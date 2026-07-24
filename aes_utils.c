#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "aes_utils.h"

int tobinary(const char *data, aes_t aes)
{
    unsigned int value;
    char byte_string[3] = {0};

    if (strlen(data) != AESSIZE * 2)
        return -1;

    for (int i = 0; i < AESSIZE; ++i) {
        byte_string[0] = data[2 * i];
        byte_string[1] = data[2 * i + 1];

        if (!isxdigit((unsigned char)byte_string[0]) ||
            !isxdigit((unsigned char)byte_string[1]) ||
            sscanf(byte_string, "%2x", &value) != 1) {
            return -1;
        }

        aes[i] = (uint8_t)value;
    }

    return 0;
}

void tostring(const aes_t aes, char text[AESSIZE * 2 + 1])
{
    for (int i = 0; i < AESSIZE; ++i)
        sprintf(text + 2 * i, "%02x", aes[i]);
}

void randaes(aes_t aes)
{
    for (int i = 0; i < AESSIZE; ++i) {
        aes[i] = (uint8_t)(rand() & 0xff);
    }
}

int parse_count(const char *text, unsigned long long *count)
{
    char *end;
    unsigned long long value;

    if (text[0] == '\0' || text[0] == '-')
        return -1;

    errno = 0;
    value = strtoull(text, &end, 10);

    if (errno != 0 || *end != '\0')
        return -1;

    *count = value;
    return 0;
}
