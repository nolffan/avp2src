// zTools: zHashTable::lookup (reconstructed from hshlook.obj).
// Byte-identical. The bucket test is a positive guard around the scan
// (`if (table[h]) { ... } return 0;`); an early `if (!l) return 0;` loses
// the dead &table[h] lea the shipped object keeps and swaps two registers.
#include "ztools.h"

void *zHashTable::lookup(void *d) const
{
	if (!d)
	{
		handle(_nullparg, EINVAL);
		return 0;
	}

	if (!valid())
	{
		handle(_inval, EINVAL);
		return 0;
	}

	unsigned h = hashfunc(d, tabsize);

	if (table[h])
	{
		zGSLCursor<char> c(*table[h]);

		for (char *t = c.current(); t; t = c.next())
			if (!cf((char *)t, (char *)d))
				return t;
	}

	return 0;
}