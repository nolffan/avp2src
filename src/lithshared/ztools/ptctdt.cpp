// zTools: zPTree constructor / cleanup (reconstructed from ptctdt.obj).
// The destructor is the inline { cleanup(); } from ztools.h; its
// out-of-line copy and the deleting-destructor thunks are emitted
// automatically for the vtable.
#include "ztools.h"

zErrHandler zPTree::ceh("zSymTab: ");

zPTree::zPTree(dtorf_t df, cleanup_behaviour cb)
	: zErrHandling(&ceh), zPtrColl(cb, df), root(0), preview(0)
{
}

void zPTree::cleanup(zPTreeNode *n)
{
	if (!n)
	{
		n = root;
		if (!n)
			return;
	}

	if (n->left && n->left->index > n->index)
		cleanup(n->left);

	if (n->right && n->right->index > n->index)
		cleanup(n->right);

	delete n->symbol;

	if (purge())
	{
		destroy(n->body);
		delete n->body;
	}

	delete n;
}
