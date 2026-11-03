// zTools: zErrHandler::srch — binary search of the dedicated-handler
// list (reconstructed from ehsrch.obj). Leaves this->slot at the match,
// or at the insertion point when not found.
#include "ztools.h"

int zErrHandler::srch(void *ob)
{
	int lo = 0;
	int hi = ndh - 1;

	while (lo <= hi)
	{
		slot = (lo + hi) / 2;
		long diff = (long)dl[slot].object - (long)ob;
		if (diff < 0)
			lo = slot + 1;
		else if (diff > 0)
			hi = slot - 1;
		else
			return slot;
	}

	slot = hi + 1;
	return -1;
}
