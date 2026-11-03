// zTools: zSList constructor / destructor / _clear (reconstructed
// from slctdt.obj).
#include "ztools.h"

zSList::zSList(dtorf_t df, void *item) : dtor(df)
{
	if (item)
	{
		zSLink *n = new zSLink(item);
		last = n;
		if (!n)
			handle("Out of memory", ENOMEM);
		else
			n->next = n;
	}
	else
		last = 0;
}

zSList::~zSList()
{
	clear();
}

void zSList::_clear(int destroy)
{
	zSLink *l = last;

	if (!l)
		return;

	zSLink *cur = l->next;
	l->next = 0;
	last = 0;

	while (cur)
	{
		zSLink *t = cur;
		cur = cur->next;

		if (destroy)
		{
			dtor(t->body);
			::operator delete(t->body);
		}

		delete t;
	}
}
