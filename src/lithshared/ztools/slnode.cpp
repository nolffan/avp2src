// zTools: zSLink cached node allocator (reconstructed from slnode.obj).
#include "ztools.h"

zSLink *zSLink::cache;

void *zSLink::operator new(size_t)
{
	if (cache)
	{
		zSLink *t = cache;
		cache = t->next;
		return t;
	}

	return ::operator new(sizeof(zSLink));
}

void zSLink::operator delete(void *p)
{
	((zSLink *)p)->next = cache;
	cache = (zSLink *)p;
}

void zSLink::freecache()
{
	while (cache)
	{
		zSLink *n = cache->next;
		::operator delete(cache);
		cache = n;
	}
}
