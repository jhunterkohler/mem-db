#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "common.h"

static inline void *check_alloc(void *ptr)
{
    if (ptr) {
        return ptr;
    } else {
        fatal("Failed to allocate memory\n");
    }
}

void *xzalloc(size_t size)
{
    return check_alloc(calloc(1, size));
}

void *xmalloc(size_t size)
{
    return check_alloc(malloc(size));
}

char *xstrdup(const char *str)
{
    return check_alloc(strdup(str));
}

noreturn void fatal(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    fprintf(stderr, "Error: ");
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    exit(1);
}
