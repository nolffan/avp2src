// zTools: zHashTable::insert (reconstructed from hshins.obj).
// zMinErr is put into quiet mode around the list operations so
// allocation failures report through errno rather than the error
// function; note the original returns from the duplicate check without
// leaving quiet mode.
#include "ztools.h"

void *zHashTable::insert(void *d)
{
	if (!d)
	{
		handle(_nullparg, EINVAL);
		return 0;
	}

	if (!valid())
	{
		handle(_inval, EINVAL);
		return 0;
	}

	unsigned h = hashfunc(d, tabsize);

	zMinErr::quiet();
	errno = 0;

	if (table[h])
	{
		switch (_mode)
		{
		case stacking:
			if (!table[h]->prepend((char *)d))
				goto nomem;
			break;

		case unique:
		{
			zGSLCursor<char> c(*table[h]);

			for (char *t = c.current(); t; t = c.next())
				if (!cf((char *)t, (char *)d))
					return 0;
		}
		// fall through

		case queueing:
			if (!table[h]->append((char *)d))
				goto nomem;
			break;

		default:
			break;
		}
	}
	else
	{
		table[h] = new zGSList<char>((char *)d);

		if (!table[h] || errno)
		{
			if (errno)
				errno = 0;
			goto nomem;
		}

		table[h]->set_dtor(destructor());
	}

	zMinErr::use_error_function();
	incc();
	return d;

nomem:
	handle(_nomem, ENOMEM);
	zMinErr::use_error_function();
	return 0;
}
