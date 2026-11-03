// zTools: zPTree::insert — self-contained insert (reconstructed from
// ptins.obj). Walks like lookup while recording the branch bit taken at
// each level; on an exact match returns the existing body, otherwise
// splices a new node, replaying the recorded bits for the re-walk.
//
// Byte-identical. Each of these was measured to matter: strlen named as
// `len` before `q = 0; p = root;` with i a separate copy (i keeps a stack
// home, len stays in esi); `d` uninitialised (shipped loads its home);
// the two allocation failures as separate handle()+return blocks; the
// re-walk starting `q = root; p = 0;` and reading `q->ptr(d)`; and the
// splice as a positive `if (p) { ... } else root = n;` nest, which makes
// VC6 cross-jump both `left` stores into the second one.
#include "ztools.h"

void *zPTree::insert(const char *s, void *b)
{
	int path[32];

	preview = 0;

	if (!s || !b)
	{
		handle(_nullparg, EINVAL);
		return 0;
	}

	int len = strlen(s) << 3;
	q = 0;
	p = root;

	int *w = path;
	int i = len;
	int d;
	sbits = len;

	while (p)
	{
		if (p->index > len + 7)
		{
			q = p;
			break;
		}

		d = bit(s, p->index);
		*w++ = d;
		q = p->ptr(d);

		if (!q)
			break;

		if (q->index <= p->index)
		{
			if (!strcmp(s, q->symbol))
				return q->body;
			break;
		}

		p = q;
	}

	if (q)
		i = diffpos(s, q->symbol);
	else
		i = len - 1;

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

	int nd = bit(s, i);
	n->ptr(nd) = n;

	if (p)
	{
		if (i >= p->index)
			p->ptr(d) = n;
		else
		{
			q = root;
			p = 0;
			w = path;

			if (q->index <= i)
			{
				do
				{
					d = *w++;
					p = q;
					q = q->ptr(d);
				} while (q->index <= i);
			}

			if (p)
				p->ptr(w[-1]) = n;
			else
				root = n;
		}
	}
	else
		root = n;

	n->ptr(!nd) = q;
	incc();
	return b;
}
