// zTools: zErrHandler constructor (reconstructed from ehctdt.obj).
#include "ztools.h"

_dhandler zErrHandler::dl[MAX_DEDICATED];
int zErrHandler::ndh;

zErrHandler::zErrHandler(const char *s)
{
	mode = fcall;
	prevmode = fcall;
	default_ef = zMinErr::catcher;
	evalue = 0;
	id = s;
}
