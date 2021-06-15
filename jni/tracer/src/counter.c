#include <sys/types.h>

static uint64_t _cntr;
uint64_t cntr(void)
{
	return (1 + __sync_fetch_and_add(&_cntr, 1));
}

void reset_cntr(void)
{
	__sync_fetch_and_and(&_cntr, 0);
}

