#ifndef MEMDB_HASH_TABLE_H_
#define MEMDB_HASH_TABLE_H_

#include "common.h"

// Not considering other machines currently.
static_assert(__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__);
static_assert(__CHAR_BIT__ == 8);

uint32_t murmur_hash_x86_32(const void *key, size_t len, uint32_t seed);

uint32_t rotl_32(uint32_t x, size_t n);

int bit_at(const void *target, size_t i);

void bin(char *dest, const void *src, size_t bits);

void hex(char *dest, const void *src, size_t bytes);

#endif
