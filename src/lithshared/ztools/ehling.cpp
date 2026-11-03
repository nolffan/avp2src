// zTools: zErrHandling — indirection base class over zErrHandler
// (reconstructed from ehling.obj). Defines the global default handler
// (a zErrHandler named "_", C linkage) and the shared error strings,
// which are lazily assigned by the first constructor call.
#include "ztools.h"

extern "C" zErrHandler _("Global Error: ");

char *zErrHandling::_inval;
char *zErrHandling::_overflow;
char *zErrHandling::_range;
char *zErrHandling::_nullparg;
char *zErrHandling::_exists;
char *zErrHandling::_badarg;
char *zErrHandling::_nosuch;
char *zErrHandling::_nomem;

zErrHandling::zErrHandling(zErrHandler *p) : hp(p ? p : &_)
{
	if (!_nomem)
	{
		_nomem = "Out of memory";
		_inval = "Data structure is invalid";
		_overflow = "Overflow";
		_nosuch = "No such file, handle or object";
		_range = "Out of range";
		_exists = "Target already exisits";
		_nullparg = "Null pointer argument";
		_badarg = "Bad argument value";
	}
}

zErrHandling::~zErrHandling()
{
	set_ef(0);
}
