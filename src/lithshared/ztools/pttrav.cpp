// zTools: zPTree::_trav — in-order traversal calling the visitor on
// each node (reconstructed from pttrav.obj; the right-branch recursion
// is a tail call the compiler folds into a loop).
#include "ztools.h"

void zPTree::_trav(stvf_t vf, void *supplementary, zPTreeNode *n)
{
	if (!n)
	{
		n = root;
		if (!n)
			return;
	}

	vf(n->symbol, n->body, supplementary);

	if (n->left && n->left->index > n->index)
		_trav(vf, supplementary, n->left);

	if (n->right && n->right->index > n->index)
		_trav(vf, supplementary, n->right);
}
