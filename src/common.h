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

/*
 * Array type compatability check macro. Check portability with:
 * `__has_builtin(__builtin_types_compatible_p)`.
 */
#define IS_ARRAY(a) \
    (!__builtin_types_compatible_p(__typeof(a), __typeof(&(a)[0])))

/*
 * Type-safe array size macro.
 */
#define ARRAY_SIZE(a)                                   \
    ({                                                  \
        static_assert(IS_ARRAY(a), "Non-static array"); \
        sizeof(a) / sizeof((a)[0]);                     \
    })

/*
 * Non-re-evaluting generic `max` macro.
 */
#define max(a, b)              \
    ({                         \
        __auto_type __a = (a); \
        __auto_type __b = (b); \
        __a > __b ? __a : __b; \
    })

/*
 * Non-re-evaluating generic `min` macro.
 */
#define min(a, b)              \
    ({                         \
        __auto_type __a = (a); \
        __auto_type __b = (b); \
        __a < __b ? __a : __b; \
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
