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

#define HASH_TABLE_INIT_BUCKET_COUNT 2

struct hash_table; // Predeclare

struct hash_table_interface {
    uint64_t (*hash_fn)(const void *key);
    void *(*key_dup)(struct hash_table *table, const void *key);
    void *(*val_dup)(struct hash_table *table, const void *val);
    void (*free_val)(struct hash_table *table, void *val);
    void (*free_key)(struct hash_table *table, void *key);
    int (*key_cmp)(struct hash_table *table, const void *key_a,
                   const void *key_b);
};

struct hash_table {
    size_t entry_count;
    size_t bucket_count;
    struct hash_table_entry **entries;
    struct hash_table_interface *interface;
};

union hash_table_value {
    void *buf;
    uint64_t u64;
    int64_t i64;
};

struct hash_table_entry {
    void *key;
    union hash_table_value val;
    struct hash_table_entry *next;
    struct hash_table_entry *prev;
};

int hash_table_insert(struct hash_table *table, void *key, void *val);
int hash_table_rm(struct hash_table *table, void *key);
struct hash_table_entry *hash_table_get(struct hash_table *table, void *key);
struct hash_table_entry **hash_table_bucket(struct hash_table *table,
                                            const void *key);

/* Exposes `interface` member functions. */

uint64_t hash_table_hashkey(struct hash_table *table, const void *key);
int hash_table_keycmp(struct hash_table *table, void *key_a, void *key_b);
void *hash_table_setkey(struct hash_table *table,
                        struct hash_table_entry *entry, void *key);
void *hash_table_setval(struct hash_table *table,
                        struct hash_table_entry *entry, void *val);
void hash_table_freekey(struct hash_table *table,
                        struct hash_table_entry *entry);
void hash_table_freeval(struct hash_table *table,
                        struct hash_table_entry *entry);
int hash_table_rehash(struct hash_table *table, size_t bucket_count);

#endif
