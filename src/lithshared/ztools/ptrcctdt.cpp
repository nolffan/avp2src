// zTools: zPtrColl constructor / destructor (reconstructed from
// ptrcctdt.obj).
#include "ztools.h"

zPtrColl::zPtrColl(cleanup_behaviour cb, dtorf_t df)
	: dtor(df), flags((short)cb), _count(0)
{
}

zPtrColl::~zPtrColl()
{
}
