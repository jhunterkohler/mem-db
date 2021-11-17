#include <unistd.h>
#include "sys.h"

size_t page_size()
{
    return sysconf(_SC_PAGESIZE);
}

size_t core_count()
{
    return sysconf(_SC_NPROCESSORS_ONLN);
}
