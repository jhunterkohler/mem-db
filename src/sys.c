#include <unistd.h>
#include "sys.h"

ssize_t page_size()
{
    return sysconf(_SC_PAGESIZE);
}

ssize_t core_count()
{
    return sysconf(_SC_NPROCESSORS_ONLN);
}
