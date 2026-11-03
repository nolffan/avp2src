// zTools: zMinErr statics and constructor (reconstructed from minerr.obj).
#include "ztools.h"

unsigned long zMinErr::caller_ip;
erf_t zMinErr::ef;
int zMinErr::_quiet;

zMinErr::zMinErr()
{
	if (!ef)
		ef = catcher;
}
