/****************************************************************************
;
;	 MODULE:		GAMESPYCLIENTMGR (.CPP)
;
;	PURPOSE:		Game Spy Client Manager
;
;	COMMENT:		GameSpyClientMgr.lib, written from the release's
;					prebuilt build of it. The server list comes from
;					WON's directory servers; the servers on it are then
;					queried with the GameSpy C Engine.
;
****************************************************************************/


// Includes...

#include "WONAPI.h"
#include "WONServer/ServerContext.h"
#include "WONAuth/AuthContext.h"
#include "WONAuth/CDKey.h"
#include "WONAuth/GetCertOp.h"
#include "WONDir/GetDirOp.h"
#include "WONMisc/GetVersionOp.h"
#include "WONMisc/GetMOTDOp.h"

#include "windows.h"
#include "gamespyclientmgr.h"
#include "stdio.h"

using namespace WONAPI;


// The C Engine's server list internals we drive directly...

extern "C"
{
	GError	InitUpdateList(GServerList serverlist);
	void	ServerListModeChange(GServerList serverlist, GServerListState newstate);
	void	DoSyncLoop(GServerList serverlist);
	void	ServerListAddServer(GServerList serverlist, unsigned long ip, unsigned short port, GQueryType qtype);
	void	ServerListSetQueryType(GServerList serverlist, GQueryType querytype);
}


// Globals...

const wchar_t*			WONAPI_AVP2COMMUNITY         = L"AliensVsPredator2";
const char*				WONAPI_AVP2PRODUCTSTRING     = "AliensVsPredator2";
const wchar_t*			WONAPI_DIRPATH_AVP2SERVERS   = L"/AliensVsPredator2";
const wchar_t*			WONAPI_DIRPATH_TITANSERVERS  = L"/TitanServers/Auth";

WONAPICore*				gAPICore;

ServerContextPtr		GSClient_gDirContext;
AuthContextPtr			GSClient_gAuthContext;
GetDirOpPtr				GSClient_gGetAuthServersOp;
GetDirOpPtr				GSClient_gGetAVP2ServersOp;
GetCertOpPtr			GSClient_gGetCertOp;
GetVersionOpPtr			GSClient_gGetVersionOp;
GetMOTDOpPtr			GSClient_gGetMOTDOp;


// Prototypes...

void GSCM_CallBackFunction(GServerList pServerList, int nMsg, void* pInstance, void* pParam1, void* pParam2);
void GetAVP2ServersOpCompletion(AsyncOpPtr theOp, CGameSpyClientMgr* pGameSpyClientMgr);
void GetVersionOpCompletion(AsyncOpPtr theOp, CGameSpyClientMgr* pGameSpyClientMgr);


// Functions...

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CGameSpyClientMgr::CGameSpyClientMgr
//
//	PURPOSE:	Constructor
//
// ----------------------------------------------------------------------- //

CGameSpyClientMgr::CGameSpyClientMgr()
{
	Clear();

	gAPICore = new WONAPICore(true, true, false);
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CGameSpyClientMgr::Init
//
//	PURPOSE:	Initialization
//
// ----------------------------------------------------------------------- //

BOOL CGameSpyClientMgr::Init(const char* sGameName, const char* sSecretKey, const char* sVersion)
{
	// Sanity checks...

	if (!sGameName) return(FALSE);
	if (!sSecretKey) return(FALSE);
	if (!sVersion) return(FALSE);


	// Set simple members...

	strncpy(m_sGameName, sGameName, GSCM_MAX_STRING);
	strncpy(m_sSecretKey, sSecretKey, GSCM_MAX_STRING);
	strncpy(m_sVersion, sVersion, GSCM_MAX_STRING);


	// Create the WON contexts...

	GSClient_gDirContext  = new ServerContext;
	GSClient_gAuthContext = new AuthContext;

	m_pAuthContext = GSClient_gAuthContext;


	// Flag that we are initialized...

	if (m_pAuthContext) m_bInitialized = TRUE;

	m_bForceCDKeyValid = FALSE;


	// All done...

	return(m_bInitialized);
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CGameSpyClientMgr::GetCDKey
//
//	PURPOSE:	Gets the CD key from the registry
//
// ----------------------------------------------------------------------- //

BOOL CGameSpyClientMgr::GetCDKey(char *szCDKey)
{
	if (!m_bInitialized || !szCDKey) return(FALSE);

	CDKey aKey(WONAPI_AVP2PRODUCTSTRING);
	aKey.LoadFromRegistry(REGKEY_LOCAL_MACHINE);

	if (m_bForceCDKeyValid)
	{
		strcpy(szCDKey, "VALIDCDKEY");
	}
	else
	{
		strcpy(szCDKey, aKey.GetString().c_str());
	}

	return(TRUE);
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CGameSpyClientMgr::SetupCDKey
//
//	PURPOSE:	Sets up the CD key, from the registry or the given key
//
// ----------------------------------------------------------------------- //

BOOL CGameSpyClientMgr::SetupCDKey(char *szCDKey)
{
	if (!m_bInitialized) return(FALSE);

	CDKey aKey(WONAPI_AVP2PRODUCTSTRING);

	if (!szCDKey)
	{
		aKey.LoadFromRegistry(REGKEY_LOCAL_MACHINE);
	}
	else
	{
		aKey.Init(szCDKey);
	}

	aKey.SaveToRegistry(REGKEY_LOCAL_MACHINE);

	GSClient_gAuthContext->SetCDKey(WONAPI_AVP2COMMUNITY, aKey);

	if (szCDKey && stricmp(szCDKey, "VALIDCDKEY") == 0)
	{
		m_bForceCDKeyValid = TRUE;
	}

	return(aKey.IsValid() || m_bForceCDKeyValid);
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CGameSpyClientMgr::IsCDKeyValid
//
//	PURPOSE:	Determines if the CD key in the registry is valid
//
// ----------------------------------------------------------------------- //

BOOL CGameSpyClientMgr::IsCDKeyValid()
{
	if (!m_bInitialized) return(FALSE);

	CDKey aKey(WONAPI_AVP2PRODUCTSTRING);
	aKey.LoadFromRegistry(REGKEY_LOCAL_MACHINE);

	return(aKey.IsValid() || m_bForceCDKeyValid);
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CGameSpyClientMgr::SetupUserIdentity
//
//	PURPOSE:	Sets up the user's identity
//
// ----------------------------------------------------------------------- //

BOOL CGameSpyClientMgr::SetupUserIdentity()
{
	if (!m_bInitialized) return(FALSE);

	GSClient_gAuthContext->SetCommunity(WONAPI_AVP2COMMUNITY);

	return(TRUE);
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CGameSpyClientMgr::SetupDirServers
//
//	PURPOSE:	Adds the directory servers listed in sierraup.cfg
//
// ----------------------------------------------------------------------- //

BOOL CGameSpyClientMgr::SetupDirServers()
{
	if (!m_bInitialized) return(FALSE);

	FILE* fp;
	char szLine[256];
	char szServer[256];
	int nServers = 0;

	// RETAIL BUG: the file is neither checked for having opened nor closed.

	fp = fopen("sierraup.cfg", "r");

	while (fgets(szLine, 256, fp))
	{
		if (strnicmp(szLine, "DirectoryServer", 15) == 0)
		{
			char* pStart = strstr(szLine, "\"") + 1;
			char* pEnd   = strstr(pStart, "\"");
			int   nLen   = pEnd - pStart;

			strncpy(szServer, pStart, nLen);
			szServer[nLen] = '\0';

			AddDirServer(szServer);
			nServers++;
		}
	}

	return(nServers > 0);
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CGameSpyClientMgr::SetupMOTD
//
//	PURPOSE:	Gets the system and game messages of the day
//
// ----------------------------------------------------------------------- //

BOOL CGameSpyClientMgr::SetupMOTD(char *szSysMsg, char *szGameMsg, int nMaxLength)
{
	if (!m_bInitialized || nMaxLength < 1 || !szSysMsg || !szGameMsg) return(FALSE);

	GSClient_gGetMOTDOp = new GetMOTDOp(WONAPI_AVP2PRODUCTSTRING);

	if (GSClient_gGetMOTDOp->Run(OP_MODE_BLOCK, 30000) != WS_Success) return(FALSE);

	if (GSClient_gGetMOTDOp->SysMOTDIsNew())
	{
		strncpy(szSysMsg, GSClient_gGetMOTDOp->GetSysMOTD()->data(), nMaxLength - 1);
	}
	else
	{
		szSysMsg[0] = '\0';
	}

	if (GSClient_gGetMOTDOp->GameMOTDIsNew())
	{
		strncpy(szGameMsg, GSClient_gGetMOTDOp->GetGameMOTD()->data(), nMaxLength - 1);
	}
	else
	{
		szGameMsg[0] = '\0';
	}

	return(TRUE);
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CGameSpyClientMgr::SetupVersionCheck
//
//	PURPOSE:	Starts checking our version against the valid ones
//
// ----------------------------------------------------------------------- //

BOOL CGameSpyClientMgr::SetupVersionCheck()
{
	if (!m_bInitialized) return(FALSE);

	GSClient_gGetVersionOp = new GetVersionOp(GSClient_gDirContext, WONAPI_AVP2PRODUCTSTRING);
	GSClient_gGetVersionOp->SetCompletion(new ParamCompletion<AsyncOpPtr, CGameSpyClientMgr*>(GetVersionOpCompletion, this));
	GSClient_gGetVersionOp->RunAsync(30000);

	SetVersionCheckState(VERSIONCHECKSTATE_INPROGRESS);

	return(TRUE);
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CGameSpyClientMgr::SetupAuthentication
//
//	PURPOSE:	Gets the auth servers and our certificate
//
// ----------------------------------------------------------------------- //

BOOL CGameSpyClientMgr::SetupAuthentication(char *szFailedMsg, int nMaxLength)
{
	// Get the auth servers...

	GSClient_gGetAuthServersOp = new GetDirOp(GSClient_gDirContext);
	GSClient_gGetAuthServersOp->SetFlags(DIR_GF_DECOMPSERVICES | DIR_GF_SERVADDPATH | DIR_GF_SERVADDNETADDR);
	GSClient_gGetAuthServersOp->SetPath(WONAPI_DIRPATH_TITANSERVERS);

	WONStatus aStatus = GSClient_gGetAuthServersOp->Run(OP_MODE_BLOCK, 30000);
	if (aStatus != WS_Success)
	{
		strncpy(szFailedMsg, WONStatusToString(aStatus), nMaxLength - 1);
		return(FALSE);
	}

	GSClient_gAuthContext->AddAddressesFromDir(GSClient_gGetAuthServersOp->GetDirEntityList());


	// Get our certificate...

	GSClient_gGetCertOp = new GetCertOp(GSClient_gAuthContext);

	aStatus = GSClient_gGetCertOp->Run(OP_MODE_BLOCK, 30000);
	if (aStatus != WS_Success)
	{
		strncpy(szFailedMsg, WONStatusToString(aStatus), nMaxLength - 1);
		return(FALSE);
	}

	return(TRUE);
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CGameSpyClientMgr::Term
//
//	PURPOSE:	Termination
//
// ----------------------------------------------------------------------- //

void CGameSpyClientMgr::Term()
{
	// Free the WON objects...

	GSClient_gDirContext       = NULL;
	GSClient_gAuthContext      = NULL;
	GSClient_gGetAuthServersOp = NULL;
	GSClient_gGetAVP2ServersOp = NULL;
	GSClient_gGetCertOp        = NULL;
	GSClient_gGetVersionOp     = NULL;
	GSClient_gGetMOTDOp        = NULL;

	gAPICore->Shutdown();
	delete gAPICore;


	// Free the server list if we have one...

	FreeServerList();


	// Clear all members...

	Clear();
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CGameSpyClientMgr::AddDirServer
//
//	PURPOSE:	Adds a directory server ("host:port")
//
// ----------------------------------------------------------------------- //

BOOL CGameSpyClientMgr::AddDirServer(const char* sHostAndPortString)
{
	IPAddr anAddr(sHostAndPortString);

	if (!anAddr.IsValid()) return(FALSE);

	GSClient_gDirContext->AddAddress(anAddr);

	return(TRUE);
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CGameSpyClientMgr::Clear
//
//	PURPOSE:	Clears all member variables...
//
// ----------------------------------------------------------------------- //

void CGameSpyClientMgr::Clear()
{
	// Clear all member variables...

	m_bInitialized        = FALSE;
	m_bLANOnly            = FALSE;
	m_nVersionCheckState  = VERSIONCHECKSTATE_NONE;
	m_pServerList         = NULL;
	m_sGameName[0]        = '\0';
	m_sSecretKey[0]       = '\0';
	m_nGetServerIndex     = 0;
	m_nProgress           = 0;
	m_bNewServer          = FALSE;
	m_pAuthContext        = NULL;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CGameSpyClientMgr::CreateServerList
//
//	PURPOSE:	Creates the server list if it has not yet been created
//
// ----------------------------------------------------------------------- //

BOOL CGameSpyClientMgr::CreateServerList()
{
	// If we already have a server list, just return...

	if (m_pServerList)
	{
		return(TRUE);
	}


	// Create a new server list...

	m_pServerList = ServerListNew(m_sGameName, m_sGameName, m_sSecretKey, GSCM_MAX_UPDATES, GSCM_CallBackFunction, GCALLBACK_FUNCTION, this);
	if (!m_pServerList)
	{
		return(FALSE);
	}


	// All done...

	return(TRUE);
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CGameSpyClientMgr::FreeServerList
//
//	PURPOSE:	Frees the server list if there is one
//
// ----------------------------------------------------------------------- //

void CGameSpyClientMgr::FreeServerList()
{
	// If we don't have a server list, just return...

	if (!m_pServerList)
	{
		return;
	}


	// Free the server list...

	ServerListFree(m_pServerList);
	m_pServerList = NULL;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CGameSpyClientMgr::ClearServers
//
//	PURPOSE:	Clears all data from the server list
//
// ----------------------------------------------------------------------- //

void CGameSpyClientMgr::ClearServers()
{
	// If we don't have a server list, just return...

	if (!m_pServerList)
	{
		return;
	}


	// Clear the server list...

	ServerListClear(m_pServerList);
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CGameSpyClientMgr::Update
//
//	PURPOSE:	Updates everything.  This function should be called as
//				part of the update loop.
//
// ----------------------------------------------------------------------- //

void CGameSpyClientMgr::Update()
{
	// Update the server list processing if we have one...

	if (m_pServerList)
	{
		ServerListThink(m_pServerList);
	}
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CGameSpyClientMgr::GetNumServers
//
//	PURPOSE:	Gets the number of servers in the server list
//
// ----------------------------------------------------------------------- //

int CGameSpyClientMgr::GetNumServers()
{
	// Sanity checks...

	if (!m_pServerList)
	{
		return(0);
	}


	// Count the servers...

	return(ServerListCount(m_pServerList));
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CGameSpyClientMgr::SortServersBy*
//
//	PURPOSE:	Sorts the server list by the given key
//
// ----------------------------------------------------------------------- //

void CGameSpyClientMgr::SortServersByName(BOOL bAscending)
{
	if (!m_pServerList) return;

	SortServers(bAscending, "hostname", cm_stricase);
}

void CGameSpyClientMgr::SortServersByPing(BOOL bAscending)
{
	if (!m_pServerList) return;

	SortServers(bAscending, "ping", cm_int);
}

void CGameSpyClientMgr::SortServersByPlayers(BOOL bAscending)
{
	if (!m_pServerList) return;

	SortServers(bAscending, "numplayers", cm_int);
}

void CGameSpyClientMgr::SortServersByMap(BOOL bAscending)
{
	if (!m_pServerList) return;

	SortServers(bAscending, "mapname", cm_stricase);
}

void CGameSpyClientMgr::SortServersByGameType(BOOL bAscending)
{
	if (!m_pServerList) return;

	SortServers(bAscending, "gametype", cm_stricase);
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CGameSpyClientMgr::SortServers
//
//	PURPOSE:	Sorts the server list
//
// ----------------------------------------------------------------------- //

void CGameSpyClientMgr::SortServers(BOOL bAscending, char* sSortKey, GCompareMode eCompareMode)
{
	// Sanity checks...

	if (!m_pServerList) return;


	// Sort the server list...

	ServerListSort(m_pServerList, (gbool)bAscending, sSortKey, eCompareMode);
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CGameSpyClientMgr::GetState
//
//	PURPOSE:	Gets the state of the server list
//
// ----------------------------------------------------------------------- //

int CGameSpyClientMgr::GetState()
{
	// Sanity checks...

	if (!m_pServerList)
	{
		return(sl_idle);
	}


	// Get the state of the server list...

	GServerListState state = ServerListState(m_pServerList);
	return((int)state);
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CGameSpyClientMgr::RefreshServers
//
//	PURPOSE:	Refreshes the server list: gets it from the directory
//				server, then queries the servers on it
//
// ----------------------------------------------------------------------- //

BOOL CGameSpyClientMgr::RefreshServers(BOOL bAsync)
{
	// Kill any request still running...

	if (GSClient_gGetAVP2ServersOp.get())
	{
		GSClient_gGetAVP2ServersOp->Kill(true);
	}


	// Sanity checks...

	if (!IsInitialized())
	{
		return(FALSE);
	}


	// Create a server list if necessary...

	if (!CreateServerList())
	{
		return(FALSE);
	}


	// Check if we are in the idle state or not...

	if (GetState() == sl_idle)
	{
		ServerListHalt(m_pServerList);
	}


	// Clear the current list...

	ClearServers();


	// Get the list from the directory server...

	m_nProgress = 0;
	InitUpdateList(m_pServerList);

	GSClient_gGetAVP2ServersOp = new GetDirOp(GSClient_gDirContext);
	GSClient_gGetAVP2ServersOp->SetFlags(DIR_GF_DECOMPSERVICES | DIR_GF_SERVADDPATH | DIR_GF_SERVADDNETADDR);
	GSClient_gGetAVP2ServersOp->SetPath(WONAPI_DIRPATH_AVP2SERVERS);
	GSClient_gGetAVP2ServersOp->SetAuthType(AUTH_TYPE_SESSION, GSClient_gAuthContext);

	if (bAsync)
	{
		ServerListModeChange(m_pServerList, sl_listxfer);
		GSClient_gGetAVP2ServersOp->SetCompletion(new ParamCompletion<AsyncOpPtr, CGameSpyClientMgr*>(GetAVP2ServersOpCompletion, this));
		GSClient_gGetAVP2ServersOp->RunAsync(30000);
	}
	else
	{
		GSClient_gGetAVP2ServersOp->Run(OP_MODE_BLOCK, OP_TIMEOUT_INFINITE);
		GetAVP2ServersOpCompletion(GSClient_gGetAVP2ServersOp.get(), this);
	}


	// All done...

	return(TRUE);
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CGameSpyClientMgr::KillGetServersOp
//
//	PURPOSE:	Halts the server list update
//
// ----------------------------------------------------------------------- //

void CGameSpyClientMgr::KillGetServersOp()
{
	if (GetState() != sl_idle)
	{
		ServerListHalt(m_pServerList);
	}
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	GetAVP2ServersOpCompletion
//
//	PURPOSE:	Adds the servers the directory server listed, and
//				queries them
//
// ----------------------------------------------------------------------- //

void GetAVP2ServersOpCompletion(AsyncOpPtr theOp, CGameSpyClientMgr* pGameSpyClientMgr)
{
	GetDirOpPtr aGetDirOp = (GetDirOp*)theOp.get();

	if (aGetDirOp->GetStatus() == WS_Killed) return;
	if (GSClient_gGetAVP2ServersOp.get() != aGetDirOp.get()) return;

	if (aGetDirOp->GetStatus() == WS_Success)
	{
		ServerListSetQueryType(pGameSpyClientMgr->m_pServerList, qt_status);

		DirEntityList::const_iterator anItr;
		for (anItr = aGetDirOp->GetDirEntityList().begin(); anItr != aGetDirOp->GetDirEntityList().end(); ++anItr)
		{
			IPAddr anAddr = (*anItr)->GetNetAddrAsIP();
			ServerListAddServer(pGameSpyClientMgr->m_pServerList, htonl(anAddr.GetHost()), anAddr.GetPort(), qt_status);
			pGameSpyClientMgr->m_bNewServer = TRUE;
		}
	}

	ServerListModeChange(pGameSpyClientMgr->m_pServerList, sl_querying);
	DoSyncLoop(pGameSpyClientMgr->m_pServerList);
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	GetVersionOpCompletion
//
//	PURPOSE:	Checks our version against the valid ones
//
// ----------------------------------------------------------------------- //

void GetVersionOpCompletion(AsyncOpPtr theOp, CGameSpyClientMgr* pGameSpyClientMgr)
{
	if (theOp.get() != GSClient_gGetVersionOp.get()) return;

	GetVersionOpPtr aGetVersionOp = (GetVersionOp*)theOp.get();

	if (aGetVersionOp->GetStatus() != WS_Success)
	{
		pGameSpyClientMgr->SetVersionCheckState(VERSIONCHECKSTATE_FAILED);
		return;
	}

	if (aGetVersionOp->GetHighestValidVersion()[0])
	{
		if (!aGetVersionOp->VersionIsValid(pGameSpyClientMgr->GetVersion()))
		{
			pGameSpyClientMgr->SetVersionCheckState(VERSIONCHECKSTATE_REQUIREPATCH);
			return;
		}

		if (!aGetVersionOp->VersionIsLatest(pGameSpyClientMgr->GetVersion()))
		{
			pGameSpyClientMgr->SetVersionCheckState(VERSIONCHECKSTATE_OPTIONALPATCH);
			return;
		}
	}

	pGameSpyClientMgr->SetVersionCheckState(VERSIONCHECKSTATE_SUCCESS);
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CGameSpyClientMgr::RefreshLANServers
//
//	PURPOSE:	Refreshes the server list from the LAN
//
// ----------------------------------------------------------------------- //

BOOL CGameSpyClientMgr::RefreshLANServers()
{
	// Sanity checks...

	if (!IsInitialized())
	{
		return(FALSE);
	}


	// Create a server list if necessary...

	if (!CreateServerList())
	{
		return(FALSE);
	}


	// Check if we are in the idle state or not...

	if (GetState() == sl_idle)
	{
		ServerListHalt(m_pServerList);
	}


	// Clear the current list...

	ClearServers();


	// Search the LAN...

	m_nProgress = 0;
	ServerListLANUpdate(m_pServerList, FALSE, 27800, 27900, 8);


	// Remove the servers of other games...
	// RETAIL BUG: servers are removed by their "hostport" (game port), but
	// ServerListRemoveServer looks them up by query port; one it does not
	// find stays in the list while the count still goes down.

	int i = 0;
	int nCount = ServerListCount(m_pServerList);

	while (i < nCount)
	{
		GServer pServer = ServerListGetServer(m_pServerList, i);

		if (stricmp(m_sGameName, ServerGetStringValue(pServer, "gamename", "")) != 0)
		{
			char* sAddress = ServerGetAddress(pServer);
			int   nPort    = atoi(ServerGetStringValue(pServer, "hostport", ""));

			ServerListRemoveServer(m_pServerList, sAddress, nPort);
			nCount--;
		}
		else
		{
			i++;
		}
	}


	// All done...

	return(TRUE);
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CGameSpyClientMgr::GetServer
//
//	PURPOSE:	Gets the server at the requested index
//
// ----------------------------------------------------------------------- //

CGameSpyServer* CGameSpyClientMgr::GetServer(int nIndex)
{
	// Sanity checks...

	if (!m_pServerList)
	{
		return(NULL);
	}


	// Get the server from the list...

	GServer pServer = ServerListGetServer(m_pServerList, nIndex);
	if (!pServer) return(NULL);


	// Create a GameSpyServer object for this server...

	CGameSpyServer* pGameSpyServer = CreateGameSpyServer(pServer);
	if (!pGameSpyServer) return(NULL);


	// All done...

	return(pGameSpyServer);
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CGameSpyClientMgr::GetServerFromHandle
//
//	PURPOSE:	Gets the server from the given handle
//
// ----------------------------------------------------------------------- //

CGameSpyServer* CGameSpyClientMgr::GetServerFromHandle(void* pHandle)
{
	// Sanity checks...

	if (!m_pServerList) return(NULL);
	if (!pHandle) return(NULL);


	// Get the server from the list...

	GServer pServer = (GServer)pHandle;
	if (!pServer) return(NULL);


	// Create a GameSpyServer object for this server...

	CGameSpyServer* pGameSpyServer = CreateGameSpyServer(pServer);
	if (!pGameSpyServer) return(NULL);


	// All done...

	return(pGameSpyServer);
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CGameSpyClientMgr::GetFirstServer
//
//	PURPOSE:	Gets the first server in the server list
//
// ----------------------------------------------------------------------- //

CGameSpyServer* CGameSpyClientMgr::GetFirstServer()
{
	// Sanity checks...

	if (!m_pServerList)
	{
		return(NULL);
	}


	// Reset our get-server index...

	m_nGetServerIndex = 0;


	// Get the first server...

	return(GetServer(m_nGetServerIndex));
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CGameSpyClientMgr::GetNextServer
//
//	PURPOSE:	Gets the next server in the server list
//
// ----------------------------------------------------------------------- //

CGameSpyServer* CGameSpyClientMgr::GetNextServer()
{
	// Sanity checks...

	if (!m_pServerList)
	{
		return(NULL);
	}


	// Increment our get-server index...

	m_nGetServerIndex++;


	// Get the next server...

	return(GetServer(m_nGetServerIndex));
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CGameSpyClientMgr::CreateGameSpyServer
//
//	PURPOSE:	Creates a CGameSpyServer object from a GServer type
//
// ----------------------------------------------------------------------- //

CGameSpyServer* CGameSpyClientMgr::CreateGameSpyServer(GServer pGServer)
{
	// Sanity checks...

	if (!pGServer) return(NULL);


	// Create and initialize a new CGameSpyServer object...

	m_CurServer.Term();

	if (!m_CurServer.Init(pGServer))
	{
		return(NULL);
	}


	// All done...

	return(&m_CurServer);
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CGameSpyClientMgr::ExistNewServer
//
//	PURPOSE:	Determines if there is a new server.  After this function
//				is called, it will not return true again until there is a
//				new new server.
//
// ----------------------------------------------------------------------- //

BOOL CGameSpyClientMgr::ExistNewServer()
{
	// If there is a new server, clear the flag and return that we have a new one...

	if (m_bNewServer)
	{
		m_bNewServer = FALSE;
		return(TRUE);
	}
	else
	{
		return(FALSE);
	}
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	GSCM_CallBackFunction
//
//	PURPOSE:	Call-back function for async server updating
//
// ----------------------------------------------------------------------- //

void GSCM_CallBackFunction(GServerList pServerList, int nMsg, void* pInstance, void* pParam1, void* pParam2)
{
	// Sanity checks...

	if (!pServerList) return;


	// Get our CGameSpyClientMgr pointer...

	CGameSpyClientMgr* pGameSpyClientMgr = (CGameSpyClientMgr*)pInstance;
	if (!pGameSpyClientMgr) return;


	// Let the manager handle the update...

	pGameSpyClientMgr->HandleCallback(pServerList, nMsg, pParam1, pParam2);
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CGameSpyClientMgr::HandleCallback
//
//	PURPOSE:	Handles the callback function
//
// ----------------------------------------------------------------------- //

void CGameSpyClientMgr::HandleCallback(GServerList pServerList, int nMsg, void* pParam1, void* pParam2)
{
	// Sanity checks...

	if (pServerList != m_pServerList)
	{
		return;
	}


	// Handle the message...

	switch(nMsg)
	{
		case LIST_PROGRESS:
		{
			m_bNewServer = TRUE;
			m_nProgress  = (int)pParam2;
			break;
		}

		case LIST_STATECHANGED:
		{
			switch(GetState())
			{
				case sl_idle:		// done updating
				{
					m_nProgress = 0;
					break;
				}

				case sl_listxfer:	// getting list
				{
					break;
				}

				case sl_lanlist:	// searching lan
				{
					break;
				}

				case sl_querying:	// querying servers
				{
					break;
				}
			}
		}
	}
}
