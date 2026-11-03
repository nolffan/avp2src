// zTools: zSList::prepend (reconstructed from slprep.obj).
//
// Byte-identical. Both branches end in one shared `last->next = n;` (for an
// empty list `last` has just become n, so that is the self-link). Writing
// `n->next = n` in the empty branch loses shipped's `mov %eax,%ecx` copy.
#include "ztools.h"

void *zSList::prepend(void *a)
{
	zSLink *n = new zSLink(a);

	if (!n)
	{
		handle("Out of memory", ENOMEM);
		return 0;
	}

	if (!last)
		last = n;
	else
		n->next = last->next;

	last->next = n;

	return a;
}
