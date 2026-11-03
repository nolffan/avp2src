// zTools: zHashTable::default_hash (reconstructed from hshhsh.obj).
#include "ztools.h"

unsigned zHashTable::default_hash(const char *s, unsigned m)
{
	unsigned h = 0;

	while (*s)
		h = (h + h) ^ *s++;

	return h & (m - 1);
}
