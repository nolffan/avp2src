// zTools: zSLCursor constructor (reconstructed from slcctdt.obj).
#include "ztools.h"

zSLCursor::zSLCursor(zSList &l) : list(l)
{
	if (!list.last)
	{
		cp = 0;
		pp = 0;
	}
	else
	{
		pp = list.last;
		cp = pp->next;
	}
}
