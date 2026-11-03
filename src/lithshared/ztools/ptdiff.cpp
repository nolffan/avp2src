// zTools: zPTree::diffpos — bit position of the first difference
// between two strings (reconstructed from ptdiff.obj).
#include "ztools.h"

int zPTree::diffpos(const char *a, const char *b)
{
	int pos = 0;

	while (*a == *b)
	{
		pos += 8;
		a++;
		b++;
	}

	int x = *a ^ *b;
	int bit = 0;

	while (!(x & 1))
	{
		x >>= 1;
		bit++;
	}

	return pos + bit;
}
