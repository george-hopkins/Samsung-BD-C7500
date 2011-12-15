/* straight from autoconf manual */

#if HAVE_CONFIG_H
# include <config.h>
#endif
#undef malloc

#include <sys/types.h>

void *malloc(size_t size);

/* Allocate an N-byte block of memory from the heap.
 * If N is zero, allocate a 1-byte block.
 */

char *
rpl_malloc(size_t n)
{
    if (n == 0)
	n = 1;
    return malloc (n);
}


