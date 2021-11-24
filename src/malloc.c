#include "malloc.h"

static inline void *check_alloc(void *mem)
{
    if (!mem)
        fatal_errno();
    return mem;
}

void *zalloc(size_t size)
{
    return calloc(1, size);
}

void *memdup(void *src, size_t size)
{
    void *dest = malloc(size);
    return dest ? memmove(dest, src, size) : NULL;
}

void *xmalloc(size_t size)
{
    return check_alloc(malloc(size));
}

void *xzalloc(size_t size)
{
    return check_alloc(zalloc(size));
}

void *xrealloc(void *ptr, size_t size)
{
    return check_alloc(realloc(ptr, size));
}

char *xstrdup(char *str)
{
    return check_alloc(strdup(str));
}
