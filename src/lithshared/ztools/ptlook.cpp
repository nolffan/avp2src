// zTools: zPTree::lookup — Patricia-tree search (reconstructed from
// ptlook.obj). Leaves p/q/preview/sbits set up for a following insert.
//
// Byte-identical. The walk is a `for (;;)` whose first statement is the
// bit-range exit (`if (i > sbits) { q = p; return 0; }`), and a match is
// a positive `if (!strcmp(...)) { preview = 0; return q->body; }` with the
// refusal after it. As a `while (i <= sbits)` loop with `q = p` after it,
// the q = p block fell into the shared return 0 instead of the error
// path's epilogue being the shared one.
#include "ztools.h"

void *zPTree::lookup(const char *s)
{
	if (!s)
	{
		handle(_nullparg, EINVAL);
		return 0;
	}

	p = root;
	q = 0;
	preview = 1;
	sbits = (strlen(s) << 3) + 7;

	if (!p)
		return 0;

	int i = p->index;

	for (;;)
	{
		if (i > sbits)
		{
			q = p;
			return 0;
		}

		q = p->ptr(bit(s, i));

		if (!q)
			return 0;

		if (q->index <= i)
		{
			if (!strcmp(s, q->symbol))
			{
				preview = 0;
				return q->body;
			}
			return 0;
		}

		p = q;
		i = p->index;
	}
}