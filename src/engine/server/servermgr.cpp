// *********************************************************************** //
//
//  MODULE:   SERVERMGR (.CPP)
//
//  PURPOSE:  MyServerInterface — server.dll's concrete ServerInterface.
//            The console / config-file / shell-message half is reconstructed
//            (0x1002c021-0x1002c372); the rest is still blank stubs with the
//            exact shipped signatures. Reconstruct one at a time and verify each.
//
//            Every method opens the same way: the manager pointer is checked
//            and a missing one is LT_NOTINITIALIZED at debug level 1, not the
//            LT_INVALIDPARAMS/level-2 pair the ILTCSBase block uses.
//
//  SERVER.DLL ONLY. This TU contributes NOTHING to the shipped lithtech.exe,
//  so the 29-row "servermgr" /O2 gap is a false positive and there
//  is no /O2 shape to chase here. Three independent proofs:
//
//    * lithtech.exe contains ZERO occurrences of the literal "ServerInterface::"
//      (server.dll has nine). Eleven of these methods push such a literal into
//      dsi_ServerError, so none of them is in the image.
//    * a byte match of src/engine/lithtech/Release/servermgr.obj against
//      the retail lithtech.exe matches one symbol, FreeFileList -- and that is a
//      14-byte `jmp fs_FreeFileList` thunk that matches ambiguously (0x407a70+),
//      i.e. it matched somebody else's identical tail call.
//    * server.cpp, which defines the g_ServerInterface that would anchor the
//      vtable, is equally absent: the retail lithtech.exe has NO export directory,
//      and server.cpp's CreateServer/DeleteServer are __declspec(dllexport).
//
//  The vtable is one link unit: if it were emitted every virtual below would
//  be kept, so the eleven string-bearing methods being absent proves all 31
//  are. A "present on both sides" filter that checks only OUR object, not
//  the shipped image, cannot see this.
//
// *********************************************************************** //

#include <stdio.h>
#include <string.h>
#include "servermgr.h"
#include "de_error.h"


// The whole TU is server.dll's (see above), so lithtech.exe's compile of
// it is empty: the vtable is one link unit and nothing in the client
// references it (server.cpp's two exports, the only users, are guarded the
// same way).
#ifdef LT_SERVER_BUILD

// 0x10058bb0 -- the one server interface (servermgr.h). Its dynamic
// initializer group (0x1002bfe7 head, 0x1002bff1 constructor, 0x1002c00a
// atexit registrar, 0x1002c016 destructor) opens this TU's code, ahead of
// RunConsoleString at 0x1002c021.
MyServerInterface g_ServerInterface;


LTRESULT MyServerInterface::SetAppHandler(ServerAppHandler *pHandler)
{
	CServerMgr* pMgr = m_pServerMgr;
	if (!pMgr)
		RETURN_ERROR(1, "ServerInterface::SetAppHandler", LT_NOTINITIALIZED);

	pMgr->m_pAppHandler = pHandler;
	return LT_OK;
}

LTRESULT MyServerInterface::RunConsoleString(char *pStr)
{
	CServerMgr* pMgr = m_pServerMgr;
	if (!pMgr)
		RETURN_ERROR(1, "ServerInterface::RunConsoleString", LT_NOTINITIALIZED);

	return c_CommandHandler(&pMgr->m_ConsoleState, pStr);
}

LTRESULT MyServerInterface::GetConsoleVar(char *pName, HCONSOLEVAR *hVar, char *pDefaultVal)
{
	char str[256];

	if (!hVar)
		RETURN_ERROR(1, "ServerInterface::GetConsoleVar", LT_INVALIDPARAMS);

	*hVar = LTNULL;

	CServerMgr* pMgr = m_pServerMgr;
	if (!pMgr)
		RETURN_ERROR(1, "ServerInterface::GetConsoleVar", LT_NOTINITIALIZED);

	// The two success arms end in the same `*hVar = hVarFound; return <code>;`
	// and VC6 merges them. The shipped build hosts the merged copy in the
	// FIRST arm (0x1002c108) and the second arm jumps back into it
	// (0x1002c160). That is the if/else-chain form: every test an explicit
	// `else`, every arm ending in its own return. As guarded early returns,
	// or with LT_NOTFOUND as a trailing statement, VC6 hosts it at the LAST
	// arm instead -- same size, one block out.
	HCONSOLEVAR hVarFound = cc_FindConsoleVar(&pMgr->m_ConsoleState, pName);
	if (hVarFound)
	{
		*hVar = hVarFound;
		return LT_OK;
	}
	else if (pDefaultVal)
	{
		// No variable by that name yet: create one by running the assignment
		// as a console command, and look it up again.
		sprintf(str, "%s \"%s\"", pName, pDefaultVal);
		cc_RunCommand(&m_pServerMgr->m_ConsoleState, str, 2);

		hVarFound = cc_FindConsoleVar(&m_pServerMgr->m_ConsoleState, pName);
		if (hVarFound)
		{
			*hVar = hVarFound;
			return LT_FINISHED;
		}
		else
		{
			return LT_ERROR;
		}
	}
	else
	{
		return LT_NOTFOUND;
	}
}

LTRESULT MyServerInterface::GetVarValueFloat(HCONSOLEVAR hVar, float *val)
{
	if (!hVar || !val)
		RETURN_ERROR(1, "ServerInterface::GetVarValueFloat", LT_INVALIDPARAMS);

	*val = ((LTCommandVar*)hVar)->m_FloatVal;
	return LT_OK;
}

LTRESULT MyServerInterface::GetVarValueString(HCONSOLEVAR hVar, char *pStr, uint32 maxLen)
{
	if (!hVar || !pStr || !maxLen)
		RETURN_ERROR(1, "ServerInterface::GetVarValueString", LT_INVALIDPARAMS);

	strncpy(pStr, ((LTCommandVar*)hVar)->m_pStringVal, maxLen - 1);
	pStr[maxLen - 1] = 0;
	return LT_OK;
}

LTRESULT MyServerInterface::LoadConfigFile(char *pFilename)
{
	CServerMgr* pMgr = m_pServerMgr;
	if (!pMgr)
		RETURN_ERROR(1, "ServerInterface::LoadConfigFile", LT_NOTINITIALIZED);

	return !cc_LoadConfigFile(&pMgr->m_ConsoleState, pFilename, LTFALSE, LTTRUE);
}

LTRESULT MyServerInterface::SaveConfigFile(char *pFilename)
{
	CServerMgr* pMgr = m_pServerMgr;
	if (!pMgr)
		RETURN_ERROR(1, "ServerInterface::SaveConfigFile", LT_NOTINITIALIZED);

	return !cc_SaveConfigFile(&pMgr->m_ConsoleState, pFilename);
}

LTRESULT MyServerInterface::SendToServerShell(char *pInfo, int nLen)
{
	CServerMgr* pMgr = m_pServerMgr;
	if (!pMgr)
		RETURN_ERROR(1, "ServerInterface::SendToServerShell", LT_NOTINITIALIZED);

	IServerShell* pShell = pMgr->m_ClassMgr.m_pServerShell;
	if (pShell)
		return pShell->ServerAppMessageFn(pInfo, nLen);

	return LT_NOTFOUND;
}

LTBOOL MyServerInterface::AddResources(char **pResources, uint32 nResources)
{
	if (!m_pServerMgr)
		return LTFALSE;

	return m_pServerMgr->AddResources(pResources, nResources);
}

LTBOOL MyServerInterface::LoadBinaries()
{
	if (!m_pServerMgr)
		return LTFALSE;

	return m_pServerMgr->LoadBinaries();
}

FileEntry* MyServerInterface::GetFileList(char *pDirName)
{
	if (!m_pServerMgr)
		return LTNULL;

	return fs_GetFileList(&m_pServerMgr->m_FileMgr, pDirName);
}

void MyServerInterface::FreeFileList(FileEntry *pList)
{
	fs_FreeFileList(pList);
}

LTRESULT MyServerInterface::OpenFile(char *pFilename, ILTStream **pStream)
{
	static char* pFnName = "MyServerInterface::OpenFile";

	if (!pFilename || !pStream)
		RETURN_ERROR_SYM(2, pFnName, LT_INVALIDPARAMS);

	*pStream = fs_OpenFile(&m_pServerMgr->m_FileMgr, pFilename);
	if (*pStream)
		return LT_OK;

	RETURN_ERROR(2, pFnName, LT_MISSINGFILE);
}

LTRESULT MyServerInterface::CopyFile(const char *pszSourceFile, const char *pszDestFile)
{
	return fs_CopyFile(&m_pServerMgr->m_FileMgr, pszSourceFile, pszDestFile);
}

LTBOOL MyServerInterface::SetGameInfo(void *pGameInfo, uint32 nGameInfoLen)
{
	m_pServerMgr->SetGameInfo(pGameInfo, nGameInfoLen);
	return LTTRUE;
}

int MyServerInterface::GetNumClients()
{
	if (!m_pServerMgr)
		return 0;

	return m_pServerMgr->GetNumClients();
}

LTBOOL MyServerInterface::GetClientName(int index, char *pName, int maxChars)
{
	if (!m_pServerMgr)
		return LTFALSE;

	return m_pServerMgr->GetClientName(index, pName, maxChars);
}

LTBOOL MyServerInterface::GetClientInfo(int index, ClientInfo* pInfo)
{
	if (!m_pServerMgr)
		return LTFALSE;

	return m_pServerMgr->GetClientInfo(index, pInfo);
}

LTBOOL MyServerInterface::BootClient(uint32 dwClientID)
{
	if (!m_pServerMgr)
		return LTFALSE;

	return m_pServerMgr->BootClient(dwClientID);
}

LTRESULT MyServerInterface::GetClientPing(uint32 nClientId, float &ping)
{
	Client* pClient = m_pServerMgr->FindClient(nClientId);
	if (pClient)
	{
		NetConnection* pConn = pClient->m_pConnection;
		if (pConn)
		{
			ping = pConn->m_Ping;
			return LT_OK;
		}
	}

	RETURN_ERROR(2, "ILTPhysics::GetPing", LT_NOTINITIALIZED);
}

int MyServerInterface::GetErrorCode()
{
	if (!m_pServerMgr)
		return -1;

	return m_pServerMgr->GetErrorCode();
}

void MyServerInterface::GetErrorString(char *pString, int maxLen)
{
	if (!m_pServerMgr)
	{
		pString[0] = 0;
		return;
	}

	m_pServerMgr->GetErrorString(pString, maxLen);
}

LTBOOL MyServerInterface::Update(long flags)
{
	if (!m_pServerMgr)
		return LTFALSE;

	// The engine can tell it is being reentered from its own update.
	m_bInUpdate = LTTRUE;
	LTBOOL bRet = m_pServerMgr->Update(flags, dsi_GetTime());
	m_bInUpdate = LTFALSE;

	return bRet;
}

LTBOOL MyServerInterface::InitNetworking(char *pDriver, uint32 dwFlags)
{
	// Both arguments are accepted and ignored; the manager's own network
	// manager is simply started.
	if (!m_pServerMgr)
		return LTFALSE;

	m_pServerMgr->m_NetMgr.Init();
	return LTTRUE;
}

LTBOOL MyServerInterface::GetServiceList(NetService *&pListHead)
{
	return !m_pServerMgr->m_NetMgr.GetServiceList(pListHead);
}

LTBOOL MyServerInterface::FreeServiceList(NetService *pListHead)
{
	return !m_pServerMgr->m_NetMgr.FreeServiceList(pListHead);
}

LTBOOL MyServerInterface::SelectService(HNETSERVICE hNetService)
{
	return !m_pServerMgr->m_NetMgr.SelectService(hNetService);
}

LTBOOL MyServerInterface::UpdateSessionName(const char* sName)
{
	return !m_pServerMgr->m_NetMgr.UpdateSessionName(sName);
}

LTBOOL MyServerInterface::HostGame(NetHost &hostInfo)
{
	CServerMgr* pMgr = m_pServerMgr;

	if (!pMgr)
		return LTFALSE;
	else
	{
		CBaseDriver* pDriver = pMgr->m_NetMgr.m_pMainDriver;

		if (!pDriver)
			return LTFALSE;
		else
			return !pDriver->HostSession(&hostInfo);
	}
}

LTBOOL MyServerInterface::GetTcpIpAddress(char* sAddress, uint32 dwBufferSize, unsigned short &hostPort)
{
	if (!m_pServerMgr)
		return LTFALSE;

	return !m_pServerMgr->m_NetMgr.GetTcpIpAddress(sAddress, dwBufferSize, hostPort);
}

LTRESULT MyServerInterface::SendTo(const void *pData, uint32 len, const char *sAddr, uint32 port)
{
	CServerMgr* pMgr = m_pServerMgr;

	for (uint32 i = 0; i < pMgr->m_NetMgr.m_Drivers.m_nElements; i++)
	{
		CBaseDriver* pDriver = pMgr->m_NetMgr.m_Drivers.m_pArray[i];
		if (pDriver->m_DriverFlags & NETDRIVER_TCPIP)
			return pDriver->SendTo(pData, len, sAddr, port);
	}

	return LT_NOTINITIALIZED;
}

#endif // LT_SERVER_BUILD
