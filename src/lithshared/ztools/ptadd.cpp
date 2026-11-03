// zTools: zPTree::add — insert after a failed lookup (reconstructed
// from ptadd.obj). Relies on p/q/preview/sbits left by lookup().
//
// Byte-identical. Three source facts, each measured: the two allocation
// failures are separate handle()+return blocks (VC6 cross-jumps them;
// a combined `!n || !n->symbol` test sinks the block to the tail); the
// re-walk's direction local `wd` is uninitialised (shipped loads its
// never-written home on the no-iteration path); and the re-walk reads
// through q (`bit(s, q->index)`, `q->ptr(wd)`) after `p = q`.
#include "ztools.h"

void *zPTree::add(const char *s, void *b)
{
	if (!preview)
	{
		handle("No prior lookup", EINVAL);
		return 0;
	}

	sbits -= 7;
	preview = 0;
	int i = sbits;

	if (!s || !b)
	{
		handle(_nullparg, EINVAL);
		return 0;
	}

	if (q)
		i = diffpos(s, q->symbol);
	else
		i--;

	zPTreeNode *n = new zPTreeNode;

	if (!n)
	{
		handle(_nomem, ENOMEM);
		return 0;
	}

	n->index = i;
	n->body = b;
	n->symbol = new char[(sbits >> 3) + 1];

	if (!n->symbol)
	{
		handle(_nomem, ENOMEM);
		return 0;
	}
	strcpy(n->symbol, s);

	int d = bit(s, i);
	n->ptr(d) = n;

	int wd;

	if (!p)
		root = n;
	else if (i >= p->index)
		p->ptr(bit(s, p->index)) = n;
	else
	{
		p = 0;
		q = root;

		if (q->index <= i)
		{
			do
			{
				p = q;
				wd = bit(s, q->index);
				q = q->ptr(wd);
			} while (q->index <= i);
		}

		if (p)
			p->ptr(wd) = n;
		else
			root = n;
	}

	n->ptr(!d) = q;
	incc();
	return b;
}
