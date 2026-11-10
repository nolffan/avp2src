// server.dll — the server interface's segment query (reconstruction).
// Build /O1.
//
// Split from smgr.cpp: si_IntersectSegment is a translation unit of its own
// in both images, between the console's and the rest of the interface
// (server.dll 0x100343c6, lithtech.exe 0x0047a6d0). The file name is ours.
// PH: Primal Hunt's engine saw the STL's <string> here, through a header
// not yet identified (AvP2 1.0.9.6's did not): this unit ends in <string>'s
// guarded initializer of std::ctype<unsigned short>::id (server.dll 0x100343e9: guard 0x1008ec84, atexit of the empty
// 0x1002eaeb; lithtech.exe 0x0047a700: guard 0x004f8014, atexit of the empty
// 0x00401790).
#if defined(PRIMAL_HUNT)
#include <string>
#endif
#include "servermgr.h"

LTBOOL si_IntersectSegment(IntersectQuery* pQuery, IntersectInfo* pInfo)
{
	if (!g_pServerMgr)
		return LTFALSE;

	return w_IntersectSegment(pQuery, pInfo, &g_pServerMgr->m_Unknown210, 1);
}
