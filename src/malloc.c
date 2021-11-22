#include "malloc.h"

void *xmalloc(size_t size)
{
    void *mem = malloc(size);
    if (!mem)
        fatal_errno();
    return mem;
}

void *xzalloc(size_t size)
{
    void *mem = calloc(1, size);
    if (!mem)
        fatal_errno();
    return mem;
}

void *xrealloc(void *ptr, size_t size)
{
    void *mem = realloc(ptr, size);
    if (!mem)
        fatal_errno();
    return mem;
}

char *xstrdup(char *str)
{
    char *cpy = strdup(str);
    if (!cpy)
        fatal_errno();
    return cpy;
}

void *xmemdup(void *src, size_t size)
{
    void *dest = xmalloc(size);
    return memmove(dest, src, size);
}
