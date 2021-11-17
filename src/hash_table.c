#include "hash_table.h"

#define MURMUR_C1 0xcc9e2d51
#define MURMUR_C2 0x1b873593
#define MURMUR_C3 0xe6546b64
#define MURMUR_C4 0x85ebca6b
#define MURMUR_C5 0xc2b2ae35

uint32_t murmur_hash_x86_32(const void *key, size_t len, uint32_t seed)
{
    const uint32_t *blocks = key;

    size_t block_len = len >> 2;
    uint32_t hash = seed;

    for (int i = 0; i < block_len; i++) {
        uint32_t block = blocks[i];

        block = rotl_32(block * MURMUR_C1, 15) * MURMUR_C2;
        hash = rotl_32(hash ^ block, 13) * 5 + MURMUR_C3;
    }

    const uint8_t *tail = (const uint8_t *)(blocks + block_len);
    uint32_t tail_block = 0;

    switch (len & 3) {
    case 3:
        tail_block ^= tail[2] << 16;
    case 2:
        tail_block ^= tail[1] << 8;
    case 1:
        tail_block ^= tail[0];
        tail_block = rotl_32(tail_block * MURMUR_C1, 15) * MURMUR_C2;
        hash ^= tail_block;
    }

    hash ^= len;

    hash ^= hash >> 16;
    hash *= MURMUR_C4;
    hash ^= hash >> 13;
    hash *= MURMUR_C5;
    hash ^= hash >> 16;

    return hash;
}

uint32_t rotl_32(uint32_t x, size_t n)
{
    return (x << n) | (x >> (32 - n));
}

int bit_at(const void *target, size_t i)
{
    return (((const char *)target)[i >> 3] >> (7 - (i & 7))) & 1;
}

void bin(char *dest, const void *src, size_t bits)
{
    for (int i = 0; i < bits; i++) {
        dest[i] = bit_at(src, i) ? '1' : '0';
    }

    dest[bits] = '\0';
}

void hex(char *dest, const void *src, size_t bytes)
{
    for (int i = 0; i < bytes; i++) {
        char c = ((const char *)src)[i];

        char x = c >> 4;
        char y = c & 15;

        dest[(i << 1)] = x + (x < 10 ? '0' : 'a' - 10);
        dest[(i << 1) + 1] = y + (y < 10 ? '0' : 'a' - 10);
    }

    dest[bytes << 1] = '\0';
}
