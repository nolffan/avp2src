// zTools: zErrHandler::handle — dispatch an error to the dedicated or
// default handler per the current mode (reconstructed from ehhandle.obj).
#include "ztools.h"

void zErrHandler::handle(void *ob, const char *s, int e)
{
	char buf[160];

	if (mode == quickest)
	{
		evalue = (short)e;
		return;
	}

	int i = -1;

	if (ndh)
		i = srch(ob);

	if (i == -1)
	{
		if (mode == fcall)
		{
			strcpy(buf, id);
			strncat(buf, s, 79);
			default_ef(buf, e);
		}
		else if (mode == logging)
			evalue = (short)e;
		return;
	}

	if (mode == fcall)
		dl[i].handler(s, e);
	else if (mode == logging)
		dl[i].lasterr = (short)e;
}
