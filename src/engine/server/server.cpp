// *********************************************************************** //
//
//  MODULE:   SERVER (.CPP)
//
//  PURPOSE:  server.dll's two exports (exact signatures from
//            src/LT2/sdk/inc/server_interface.h), at 0x1002c6be and 0x1002cbb7.
//
//            There is no heap-allocated interface object: the returned
//            ServerInterface is one global MyServerInterface, and only the
//            CServerMgr behind it is new'd. Its m_pServerMgr is also the
//            "already instanced" flag, which is how a second CreateServer
//            gets refused.
//
// *********************************************************************** //

#define __SERVERAPI_EXPORT__
#include "servermgr.h"


// g_ServerInterface (0x10058bb0) is defined in servermgr.cpp: its initializer
// group 0x1002bfe7 sits at the head of that TU's code, before RunConsoleString.


// server.dll only. lithtech.exe has no export directory, so neither export
// is in it, and the interface they hand out (servermgr.cpp) is not either.
#ifdef LT_SERVER_BUILD
SERVERAPI SI_CREATESTATUS CreateServer(int version, LTGUID &appGuid, ServerInterface **ppServer)
{
	*ppServer = LTNULL;

	if (g_ServerInterface.m_pServerMgr)
		return SI_ALREADYINSTANCED;

	if (version != SI_VERSION)
		return SI_INVALIDVERSION;

	int status = dsi_Init();
	if (status != 0)
		return (status == 1) ? SI_CANTLOADRESOURCEMODULE : SI_ERRORINITTING;

	g_ServerInterface.m_pServerMgr = new CServerMgr;
	if (g_ServerInterface.m_pServerMgr)
	{
		if (g_ServerInterface.m_pServerMgr->Init())
		{
			g_ServerInterface.m_pServerMgr->m_NetMgr.m_Guid = appGuid;
			*ppServer = &g_ServerInterface;
			return SI_OK;
		}

		delete g_ServerInterface.m_pServerMgr;
		g_ServerInterface.m_pServerMgr = LTNULL;
	}

	dsi_Term();
	return SI_ERRORINITTING;
}


SERVERAPI void DeleteServer()
{
	if (g_ServerInterface.m_pServerMgr)
	{
		g_ServerInterface.m_pServerMgr->Term();

		delete g_ServerInterface.m_pServerMgr;
		g_ServerInterface.m_pServerMgr = LTNULL;

		dsi_Term();
	}
}
#endif


// ---- the object map's constructor --------------------------------------- //
//
// 0x1002cbe3 — the manager's object-id map (CServerMgr+0x8fc) constructor.
// It sits in this TU's run of CServerMgr code (right after DeleteServer), but
// it is written once, as servermgr.h's CUnknown8FC::CUnknown8FC in smgr.cpp:
// the same 34-of-34-byte near (the shipped build sinks the first of Init's
// two zero pushes past three of the four field stores; cl 8168 and 8966
// schedule it exactly as 8447 does), calling the same Init at 0x1000c4cf. The
// local CObjectMapCtorView copy that stood here was a second definition of
// that one function under another class name, whose declared-only destructor
// and Init could not link, so it is gone. One thing its probing established
// is worth keeping: a member initialiser list puts the vptr store AFTER the
// member stores, where body assignments put it first, and the shipped order
// is vptr-then-members.
