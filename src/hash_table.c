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

int hash_table_keycmp(struct hash_table *table, void *key_a, void *key_b)
{
    return table->interface->key_cmp(table, key_a, key_b);
}

void *hash_table_setkey(struct hash_table *table,
                        struct hash_table_entry *entry, void *key)
{
    if (entry->key != NULL)
        hash_table_freekey(table, entry);

    return entry->key = table->interface->key_dup
                            ? table->interface->key_dup(table, key)
                            : key;
}

void *hash_table_setval(struct hash_table *table,
                        struct hash_table_entry *entry, void *val)
{
    if (entry->val != NULL)
        hash_table_freeval(table, entry);

    return entry->val = table->interface->val_dup
                            ? table->interface->val_dup(table, val)
                            : val;
}

void hash_table_freekey(struct hash_table *table,
                        struct hash_table_entry *entry)
{
    if (table->interface->free_key)
        table->interface->free_key(table, entry->key);
}

void hash_table_freeval(struct hash_table *table,
                        struct hash_table_entry *entry)
{
    if (table->interface->free_val)
        table->interface->free_val(table, entry->val);
}

uint64_t hash_table_hashkey(struct hash_table *table, const void *key)
{
    return table->interface->hash_fn(key);
}

struct hash_table_entry **hash_table_bucket(struct hash_table *table,
                                            const void *key)
{
    return table->entries +
           (hash_table_hashkey(table, key) % table->bucket_count);
}

int hash_table_insert(struct hash_table *table, void *key, void *val)
{
    table->entry_count += 1;

    if (table->entry_count == table->bucket_count) {
        int err = hash_table_rehash(table, 3 * table->bucket_count >> 1);
        if (err)
            return err;
    }

    struct hash_table_entry **bucket = hash_table_bucket(table, key);
    struct hash_table_entry *last = NULL;
    struct hash_table_entry *entry = *bucket;

    while (entry && hash_table_keycmp(table, key, entry->key)) {
        last = entry;
        entry = entry->next;
    }

    if (!entry) {
        if (!(entry = calloc(1, sizeof(*entry))))
            return 1;

        if (last)
            last->next = entry;
        else
            *bucket = entry;

        if (!hash_table_setkey(table, entry, key))
            return 1;
    }

    if (!hash_table_setval(table, entry, val))
        return 1;
}

int hash_table_rm(struct hash_table *table, void *key)
{
    struct hash_table_entry **bucket = hash_table_bucket(table, key);
    struct hash_table_entry *entry = *bucket;

    while (entry && !hash_table_keycmp(table, entry->key, key)) {
        entry = entry->next;
    }

    if (entry) {
        if (entry->prev)
            entry->prev->next = entry->next;
        else
            *bucket = entry->next;

        hash_table_freekey(table, entry);
        hash_table_freeval(table, entry);
        free(entry);

        return 1;
    } else {
        return 0;
    }
}

struct hash_table_entry *hash_table_get(struct hash_table *table, void *key)
{
    struct hash_table_entry *entry = *hash_table_bucket(table, key);
    while (entry && hash_table_keycmp(table, entry->key, key))
        entry = entry->next;
    return entry;
}

struct hash_table *hash_table_create(struct hash_table_interface *interface)
{
    struct hash_table *table = malloc(sizeof(*table));

    table->bucket_count = HASH_TABLE_INIT_BUCKET_COUNT;
    table->entry_count = 0;
    table->interface = interface;
    table->entries = malloc(table->entry_count * sizeof(*table->entries));

    return table;
}
