#include "../src/hash_table.c"

#include <string.h>
#include <stdio.h>

const char hello_world[] = "Hello world!";
const char hello_world_hex[] = "48656c6c6f20776f726c6421";
const char hello_world_bin[] =
    "0100100001100101011011000110110001101111001000000111011101101111011100100"
    "11011000110010000100001";

void test_rotl_32()
{
    // Generated rotations
    uint32_t inputs[] = {
        0x33eaeb09, 0x67d5d612, 0xcfabac24, 0x9f575849, 0x3eaeb093, 0x7d5d6126,
        0xfabac24c, 0xf5758499, 0xeaeb0933, 0xd5d61267, 0xabac24cf, 0x5758499f,
        0xaeb0933e, 0x5d61267d, 0xbac24cfa, 0x758499f5, 0xeb0933ea, 0xd61267d5,
        0xac24cfab, 0x58499f57, 0xb0933eae, 0x61267d5d, 0xc24cfaba, 0x8499f575,
        0x0933eaeb, 0x1267d5d6, 0x24cfabac, 0x499f5758, 0x933eaeb0, 0x267d5d61,
        0x4cfabac2, 0x99f57584,
    };

    for (int i = 0; i < 32; i++) {
        assert(rotl_32(inputs[0], i) == inputs[i]);
    }
}

void test_bit_at()
{
    size_t bit_len = strlen(hello_world_bin);

    for (int i = 0; i < bit_len; i++) {
        assert(bit_at(hello_world, i) == (hello_world_bin[i] == '1'));
    }
}

void test_bin()
{
    char buf[100];
    bin(buf, hello_world, strlen(hello_world) * 8);
    assert(!strcmp(buf, hello_world_bin));
}

void test_hex()
{
    char buf[100];
    hex(buf, hello_world, strlen(hello_world));
    assert(!strcmp(buf, hello_world_hex));
}

void test_murmur_hash_x86_32()
{
    // NOTE: Well constructed test
    struct {
        const char *key;
        size_t len;
        uint32_t seed;
        uint32_t expected;
    } cases[] = {
        {                "",  0,          0,          0},
        {                "",  0,          1, 0x514e28b7},
        {                "",  0, 0xffffffff, 0x81f16f39},
        {"\xff\xff\xff\xff",  4,          0, 0x76293b50},
        {"\x21\x43\x65\x87",  4,          0, 0xf55b516b},
        {"\x21\x43\x65\x87",  4, 0x5082edee, 0x2362f9de},
        {    "\x21\x43\x65",  3,          0, 0x7e4a8634},
        {        "\x21\x43",  2,          0, 0xa0f7b07a},
        {            "\x21",  1,          0, 0x72661cf4},
        {"\x00\x00\x00\x00",  4,          0, 0x2362F9DE},
        {    "\x00\x00\x00",  3,          0, 0x85F0B427},
        {        "\x00\x00",  2,          0, 0x30F4C306},
        {            "\x00",  1,          0, 0x514E28B7},
        {   "Hello, world!", 13,       1234, 0xfaf6cdb3},
        {   "Hello, world!", 13,       4321, 0xbf505788},
    };

    for (int i = 0; i < sizeof(cases) / sizeof(*cases); i++) {
        uint32_t hash =
            murmur_hash_x86_32(cases[i].key, cases[i].len, cases[i].seed);
        assert(hash == cases[i].expected);
    }
}

int main()
{
    test_rotl_32();
    test_bit_at();
    test_bin();
    test_hex();
    test_murmur_hash_x86_32();
    printf("Success\n");
}
