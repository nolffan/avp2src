// zTools: zSList::append (reconstructed from slapp.obj).
#include "ztools.h"

void *zSList::append(void *a)
{
	zSLink *n = new zSLink(a);

	if (!n)
	{
		handle("Out of memory", ENOMEM);
		return 0;
	}

	if (!last)
	{
		last = n;
		n->next = n;
	}
	else
	{
		n->next = last->next;
		last->next = n;
		last = n;
	}

	return a;
}
