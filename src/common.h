#ifndef MEMDB_COMMON_H_
#define MEMDB_COMMON_H_

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdnoreturn.h>
#include <assert.h>

#define IS_ARRAY(a) \
    (!__builtin_types_compatible_p(__typeof(a), __typeof(&(a)[0])))

#define ARRAY_SIZE(a)                                   \
    ({                                                  \
        static_assert(IS_ARRAY(a), "Non-static array"); \
        sizeof(a) / sizeof((a)[0]);                     \
    })

noreturn void fatal(const char *fmt, ...);

void *xzalloc(size_t size);
void *xmalloc(size_t size);
char *xstrdup(const char *str);

#endif
