// zTools: zHashTable constructor / destructor / clear (reconstructed
// from hshctdt.obj).
#include "ztools.h"

zErrHandler zHashTable::ceh("zHashTable: ");

zHashTable::zHashTable(dtorf_t df, cmpf_t f, int tablesize, hashfunc_t hf,
					   cleanup_behaviour cb, mode m)
	: zPtrColl(cb, df), zErrHandling(&ceh), _mode(m)
{
	if (!f || !tablesize || !hf)
	{
		invalidate();
		handle(tablesize ? _nullparg : _badarg, EINVAL);
		return;
	}

	table = (zGSList<char> **)new char[tablesize * sizeof(zGSList<char> *)];

	if (!table)
	{
		invalidate();
		handle(_nomem, ENOMEM);
		return;
	}

	memset(table, 0, tablesize * sizeof(zGSList<char> *));
	tabsize = tablesize;
	hashfunc = hf;
	cf = f;
}

zHashTable::~zHashTable()
{
	if (!leave())
	{
		clear();
		delete table;
	}
}

void zHashTable::clear()
{
	if (!table)
		return;

	int p = purge();

	for (unsigned i = 0; i < tabsize; i++)
	{
		if (table[i])
		{
			if (p)
				table[i]->purge();
			else
				table[i]->clear();

			delete table[i];
			table[i] = 0;
		}
	}
}
