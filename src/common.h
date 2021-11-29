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

#define __static_assert _Static_assert

#define __types_compatible(a, b) \
    __builtin_types_compatible_p(__typeof(a), __typeof(b))

#define __is_array(a) (!__types_compatible((a), &(a)[0]))

#define __static_assert_array(a) \
    __static_assert(__is_array(a), "Non-array type: " #a)

#define ARRAY_SIZE(a)               \
    ({                              \
        __static_assert_array(a);   \
        sizeof(a) / sizeof((a)[0]); \
    })

#define max(a, b)                                                            \
    ({                                                                       \
        __typeof(a) __a = (a);                                               \
        __typeof(b) __b = (b);                                               \
        __static_assert(__types_compatible(a, b),                            \
                        "max(" #a "," #b ") called on incompatible types."); \
        __a > __b ? __a : __b;                                               \
    })

#define min(a, b)                                                            \
    ({                                                                       \
        __typeof(a) __a = (a);                                               \
        __typeof(b) __b = (b);                                               \
        __static_assert(__types_compatible(a, b),                            \
                        "min(" #a "," #b ") called on incompatible types."); \
        __a < __b ? __a : __b;                                               \
    })

#define container_of(ptr, type, member)                     \
    ({                                                      \
        const __typeof(((type *)0)->member) *__ptr = (ptr); \
        (type *)((char *)__ptr - offsetof(type, member));   \
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
