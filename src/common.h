#ifndef MEMDB_COMMON_H_
#define MEMDB_COMMON_H_

#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdnoreturn.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

#define IS_ARRAY(a) \
    (!__builtin_types_compatible_p(__typeof(a), __typeof(&(a)[0])))

#define ARRAY_SIZE(a)                                   \
    ({                                                  \
        static_assert(IS_ARRAY(a), "Non-static array"); \
        sizeof(a) / sizeof((a)[0]);                     \
    })

noreturn static inline void fatal(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    fprintf(stderr, "Error: ");
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    exit(1);
}

noreturn static inline void fatal_errno()
{
    fatal("%s\n", strerror(errno));
}

#endif
