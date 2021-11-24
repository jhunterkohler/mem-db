#ifndef MEMDB_SYS_H_
#define MEMDB_SYS_H_

#include <stdlib.h>

/*
 * Returns negative on failure.
 */
ssize_t page_size();

/*
 * Returns negative on failure.
 */
ssize_t core_count();

#endif
