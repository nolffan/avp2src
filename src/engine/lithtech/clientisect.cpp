// *********************************************************************** //
//
//  MODULE:   CLIENTISECT (.CPP)   (file name ours) -- lithtech.exe only
//
//  ci_IntersectSegment (AvP2 1.0.9.6 0x00404aa0, Primal Hunt 0x00405070):
//  a translation unit of its own between the client file manager's run and
//  ltcounters.cpp's.  In Primal Hunt the function is followed by the unit's
//  <string> guard pair (0x004050a0, 0x004050c0).
//
// *********************************************************************** //

#if defined(PRIMAL_HUNT) && !defined(LT_SERVER_BUILD)
// PH: Primal Hunt's engine saw the STL's <string> here, through a header
// not yet identified (AvP2 1.0.9.6's did not): its translation unit ends
// in <string>'s guarded initializer of std::ctype<unsigned short>::id
// (guard 0x004f8014, atexit of the empty 0x00401790; 0x4050a0).
#include <string>
#endif
#include "clientinterface.h"
#include "clientmgr.h"


// Two guards, both positive, with the one failure answer at the end: the
// manager itself may not exist yet, and without a connection there is no
// world to intersect.
LTBOOL ci_IntersectSegment(ClientIntersectQuery* pQuery, ClientIntersectInfo* pInfo)
{
	if (g_pClientMgr && g_pClientMgr->m_pConnection)
		return sub_4305f0(pQuery, pInfo, g_pClientMgr->m_World, 0);

	return LTFALSE;
}
