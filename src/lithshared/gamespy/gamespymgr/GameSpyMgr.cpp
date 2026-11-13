/* GameSpyMgr.cpp: GameSpyMgr.lib, written from the release's prebuilt build of it. */

/****************************************************************************
;
;	 MODULE:		GAMESPYMGR (.CPP)
;
;	PURPOSE:		Game Spy Manager
;
;	HISTORY:		09/21/98  [blg]  This file was created
;
;	COMMENT:		Copyright (c) 1998, Monolith Productions, Inc.
;
****************************************************************************/


// Includes...

#include "WONAPI.h"
#include "WONAuth/AuthContext.h"
#include "WONAuth/GetCertOp.h"
#include "WONDir/GetDirOp.h"
#include "WONDir/AddEntityOp.h"
#include "WONDir/RenewEntityOp.h"
#include "WONServer/ServerContext.h"
#include "WONCommon/WriteBuffer.h"

#include "windows.h"
#include "GameSpyMgr.h"
#include "mmsystem.h"
#include "gsutils.h"
#include <stdio.h>

using namespace WONAPI;


// WON strings...

const wchar_t*		WONAPI_AVP2COMMUNITY               = L"AliensVsPredator2";
const wchar_t*		GameSpyMgr_WONAPI_AVP2REGISTERPATH = L"/AliensVsPredator2";
const wchar_t*		GameSpyMgr_WONAPI_TITANSERVERS     = L"/TitanServers/Auth";


// Globals...

WONAPICore*			gpAPICore = NULL;

ServerContextPtr	gDirContext;
AuthContextPtr		gAuthContext;
GetDirOpPtr			gGetDirOp;
AddServiceOpPtr		gAddServiceOp;
RenewServiceOpPtr	gRenewServiceOp;

BOOL				g_SierraInitCompleted = FALSE;


// Statics...

static	char	s_sLine[1024];
static	char*	s_pCur = NULL;
static	BOOL	s_bDataReady = FALSE;


// Prototypes...

static void InitSierraMultiplayer(CGameSpyMgr* pGameSpyMgr);
static void AddDirServer(const char* sAddr);
static void GetDirOpCompletion(AsyncOpPtr theOp, CGameSpyMgr* pGameSpyMgr);
static void RenewRegistration(CGameSpyMgr* pGameSpyMgr);
static void GetCertOpCompletion(AsyncOpPtr theOp, CGameSpyMgr* pGameSpyMgr);
static void AddRegistration(CGameSpyMgr* pGameSpyMgr);
static void RegisterCompletion(AsyncOpPtr theOp, CGameSpyMgr* pGameSpyMgr);

#ifdef AVP2SRC_MODERN
// M-17 (docs/MODERN.md): the master server a server reports to. The
// retail heartbeat went to master<location>.gamespy.com, and the server
// registered with WON's directory too, which is where the retail browser
// read its list from; both are gone. The modern server sends GameSpy's
// heartbeat (\heartbeat\<game port>\gamename\<game>, every
// TIMER_HEARTBEAT, from the game port) to the master the game names
// (GameSpyServer_SetMasterServer: "host" or "host:port", empty for
// master.openspy.net, "none" for none), answers its \secure\ challenge
// (OnSecureQuery, retail's) ahead of the rest of a query, says
// \statechanged\2 when the game ends, and registers with WON no more.
#define GSM_DEFAULT_MASTER		"master.openspy.net"
#define GSM_HEARTBEAT_PORT		27900

static char  s_szMasterServer[128] = "";
static char  s_szMasterIP[32]      = "";
static DWORD s_dwMasterPort        = GSM_HEARTBEAT_PORT;

void GameSpyServer_SetMasterServer(const char* sAddr)
{
	strncpy(s_szMasterServer, sAddr ? sAddr : "", sizeof(s_szMasterServer) - 1);
	s_szMasterServer[sizeof(s_szMasterServer) - 1] = '\0';
	s_szMasterIP[0] = '\0';
}

BOOL GameSpyServer_IsMasterOff()
{
	return(stricmp(s_szMasterServer, "none") == 0);
}

// The master's address, looked up once (and again until it is found)
static BOOL ResolveMaster()
{
	if (s_szMasterIP[0]) return(TRUE);
	if (GameSpyServer_IsMasterOff()) return(FALSE);

	char sHost[128];
	strncpy(sHost, s_szMasterServer[0] ? s_szMasterServer : GSM_DEFAULT_MASTER, sizeof(sHost) - 1);
	sHost[sizeof(sHost) - 1] = '\0';

	s_dwMasterPort = GSM_HEARTBEAT_PORT;
	char* pColon = strchr(sHost, ':');
	if (pColon)
	{
		*pColon = '\0';
		if (atoi(pColon + 1) > 0) s_dwMasterPort = atoi(pColon + 1);
	}

	unsigned long nAddr = inet_addr(sHost);
	if (nAddr == INADDR_NONE)
	{
		hostent* pHost = gethostbyname(sHost);
		if (!pHost || !pHost->h_addr_list[0]) return(FALSE);
		memcpy(&nAddr, pHost->h_addr_list[0], 4);
	}

	unsigned char* b = (unsigned char*)&nAddr;
	sprintf(s_szMasterIP, "%d.%d.%d.%d", b[0], b[1], b[2], b[3]);
	return(TRUE);
}
#endif


// Functions...

// ----------------------------------------------------------------------- //
// Replacement for MFC's trace.  Doesn't ASSERT.
// ----------------------------------------------------------------------- //

void MyTrace(const char *pMsg, ...)
{
	#ifdef _DEBUG
		char buf[1024*16];
		va_list marker;

		va_start(marker, pMsg);
		vsprintf(buf, pMsg, marker);
		va_end(marker);

		OutputDebugString(buf);
	#endif
}




// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CGameSpyMgr::CGameSpyMgr
//
//	PURPOSE:	Construction
//
// ----------------------------------------------------------------------- //

CGameSpyMgr::CGameSpyMgr()
{
	gpAPICore = new WONAPICore(true, true, false);

	Clear();
}


// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CGameSpyMgr::Init
//
//	PURPOSE:	Initialization
//
// ----------------------------------------------------------------------- //

BOOL CGameSpyMgr::Init(const char* sGameName, const char* sGameVer, const char* sGameKey, int nGameLoc, const char* sGameAddr, int nGamePort, int nSpyPort, BOOL bLANOnly, DWORD dwFlags)
{
	// Sanity checks...

	if (m_bInit) return(FALSE);
	if (!sGameName) return(FALSE);
	if (!sGameVer) return(FALSE);
	if (!sGameKey) return(FALSE);


	// Set simple members...

	m_sGameName  = sGameName;
	m_sGameVer   = sGameVer;
	m_sGameKey   = sGameKey;
	m_sGameAddr  = sGameAddr;
	m_dwGamePort = nGamePort;
	m_dwSpyPort  = nSpyPort;
	m_dwFlags    = dwFlags;

	m_sGameLoc.Format("%i", nGameLoc);
	m_sGameSpyAddr.Format("master%i.gamespy.com", nGameLoc);

	s_bDataReady = FALSE;


	// Register with the Sierra (WON) directory unless we're LAN only...

	g_SierraInitCompleted = FALSE;

#ifdef AVP2SRC_MODERN
	// M-17/M-18: no WON; the master's address instead (none for a LAN
	// game, or with "none")
	m_sGameSpyAddr.Empty();

	if (!bLANOnly && ResolveMaster())
	{
		m_sGameSpyAddr = s_szMasterIP;
	}
#else
	if (!bLANOnly)
	{
		InitSierraMultiplayer(this);
	}
#endif


	// All done...

	m_bInit = TRUE;

	return(TRUE);
}


// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CGameSpyMgr::Term
//
//	PURPOSE:	Termination
//
// ----------------------------------------------------------------------- //

void CGameSpyMgr::Term()
{
#ifdef AVP2SRC_MODERN
	// M-17: tell the master the game is over
	if (m_bInit && m_pSender && !m_sGameSpyAddr.IsEmpty())
	{
		StartResponse(m_sGameSpyAddr, s_dwMasterPort);

		DWORD dwPort = GetSpyPort();
		if (UseGamePortForHeartbeat()) dwPort = GetGamePort();

		SendResponseInfo("heartbeat", dwPort);
		SendResponseInfo("gamename", m_sGameName);
		SendResponseInfo("statechanged", 2);

		FlushResponse();
	}
#endif

	// Release the WON objects...

	gDirContext     = NULL;
	gAuthContext    = NULL;
	gAddServiceOp   = NULL;
	gRenewServiceOp = NULL;

	if (gpAPICore)
	{
		gpAPICore->Shutdown();
		delete gpAPICore;
		gpAPICore = NULL;
	}

	if (!m_bInit) return;

	Clear();
}


// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CGameSpyMgr::StartResponse
//
//	PURPOSE:	Starts a new response
//
// ----------------------------------------------------------------------- //

void CGameSpyMgr::StartResponse(const char* sAddr, DWORD dwPort)
{
	m_sResponseAddr  = sAddr;
	m_dwResponsePort = dwPort;
	m_sResponse[0]   = '\0';
	m_nPacket        = 1;

	MyTrace("Starting a new response to %s on port# %lu\n", sAddr, dwPort);
}


// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CGameSpyMgr::SendResponse
//
//	PURPOSE:	Sends the current response
//
// ----------------------------------------------------------------------- //

void CGameSpyMgr::SendResponse()
{
	// Make sure we've actually send some stuff...

	if (m_nPacket == 0 && strlen(m_sResponse) <= 0) return;


	// Check if are packet is big enough to send yet...

	int nLen = strlen(m_sResponse);

	if (nLen + 50 < MAX_RESPONSE)
	{
		return;
	}


	// Send this portion of the response...

	AddResponseQueryID();

	m_nPacket++;


	// Send the response string via our socket...

	MyTrace("Sending response:\n");
	MyTrace("%s\n", m_sResponse);

	SendSocketString(m_sResponseAddr, m_dwResponsePort, m_sResponse);


	// Clear the buffer...

	m_sResponse[0] = '\0';
}


// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CGameSpyMgr::FlushResponse
//
//	PURPOSE:	Flushes the current response
//
// ----------------------------------------------------------------------- //

void CGameSpyMgr::FlushResponse()
{
	// Make sure we've actually send some stuff...

	if (m_nPacket <= 1 && strlen(m_sResponse) <= 0)
	{
		m_nPacket++;
		m_dwQueryID++;
		return;
	}


	// Add the "final" keyword...

	strcat(m_sResponse, "\\final\\");


	// Add the query ID...

	AddResponseQueryID();

	m_nPacket++;
	m_dwQueryID++;


	// Send the response string via our socket...

	MyTrace("Flushing response:\n");
	MyTrace("%s\n", m_sResponse);

	SendSocketString(m_sResponseAddr, m_dwResponsePort, m_sResponse);


	// Clear the buffer...

	m_sResponse[0] = '\0';
}


// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CGameSpyMgr::SendResponseInfo
//
//	PURPOSE:	Sends the given key/value pair to the given response
//				string (may not actually send it until the buffer is full
//				or FlushResponse() is called).
//
// ----------------------------------------------------------------------- //

void CGameSpyMgr::SendResponseInfo(const char* sKey, const char* sValue)
{
	// Sanity checks...

	if (!sKey) return;
	if (!sValue) return;


	// Check if we need to flush the response because it's getting too big...

	int nLen1 = strlen(m_sResponse) + 50;
	int nLen2 = strlen(sKey);
	int nLen3 = strlen(sValue);

	if (nLen1 + nLen2 + nLen3 >= MAX_RESPONSE)
	{
		SendResponse();
	}


	// Add the key and value info...

	strcat(m_sResponse, "\\");
	strcat(m_sResponse, sKey);
	strcat(m_sResponse, "\\");
	strcat(m_sResponse, sValue);
}

void CGameSpyMgr::SendResponseInfo(const char* sKey, int nValue)
{
	// Use the main function to do the real work...

	char sValue[32];
	sprintf(sValue, "%i", nValue);
	
	SendResponseInfo(sKey, sValue);
}


// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CGameSpyMgr::AddResponseQueryID
//
//	PURPOSE:	Adds the query ID to the given response string
//
// ----------------------------------------------------------------------- //

void CGameSpyMgr::AddResponseQueryID()
{
	// Add the query ID...

	if (m_dwQueryID > 1000) m_dwQueryID = 1;

	char sID[32];
	sprintf(sID, "%i.%i", m_dwQueryID, m_nPacket);

	strcat(m_sResponse, "\\");
	strcat(m_sResponse, "queryid");
	strcat(m_sResponse, "\\");
	strcat(m_sResponse, sID);
}


// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CGameSpyMgr::Update
//
//	PURPOSE:	Updates GameSpy input and output
//
//	RETURNS:	int = number of queries handled
//
// ----------------------------------------------------------------------- //

void CGameSpyMgr::Update()
{
	// Update the heartbeat...

	UpdateHeartbeat();
}


// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CGameSpyMgr::OnQuery
//
//	PURPOSE:	Processes the given query
//
// ----------------------------------------------------------------------- //

BOOL CGameSpyMgr::OnQuery(CGameSpyQueryInfo* pQueryInfo)
{
	// Sanity checks...

	if (!pQueryInfo) return(FALSE);
	if (!pQueryInfo->IsValid()) return(FALSE);


	// Start a new response for this query...

	StartResponse(pQueryInfo->GetAddr(), pQueryInfo->GetPort());


	// Get each query element and process it...

	CString sQuery = pQueryInfo->GetData();
#ifdef AVP2SRC_MODERN
	CString sToken;
	// M-17: a master's \secure\ is answered first, whatever comes before
	// it in the query (OpenSpy's after a heartbeat: \status\\secure\...),
	// so that \validate\ is in the reply's first packet
	sToken = GetFirstQueryToken(sQuery);

	while (!sToken.IsEmpty())
	{
		if (stricmp(sToken, "secure") == 0)
		{
			CString sValue = GetNextQueryToken();
			if (!sValue.IsEmpty())
			{
				OnSecureQuery(sValue);
			}
			break;
		}

		sToken = GetNextQueryToken();
	}

	sToken = GetFirstQueryToken(sQuery);
#else
	CString sToken = GetFirstQueryToken(sQuery);
#endif

	while (!sToken.IsEmpty())
	{
		if (stricmp(sToken, "echo") == 0)
		{
			CString sEcho = GetNextQueryToken();
			if (!sEcho.IsEmpty())
			{
				OnEchoQuery(sEcho);
			}
		}

		else if (stricmp(sToken, "secure") == 0)
		{
			CString sValue = GetNextQueryToken();
#ifndef AVP2SRC_MODERN
			if (!sValue.IsEmpty())
			{
				OnSecureQuery(sValue);
			}
#endif
		}

		else if (stricmp(sToken, "basic") == 0)
		{
			OnBasicQuery();
		}

		else if (stricmp(sToken, "info") == 0)
		{
			OnInfoQuery();
		}

		else if (stricmp(sToken, "rules") == 0)
		{
			OnRulesQuery();
		}

		else if (stricmp(sToken, "players") == 0)
		{
			OnPlayersQuery();
		}

		else if (stricmp(sToken, "status") == 0)
		{
			OnStatusQuery();
		}

		else
		{
			MyTrace("Unknown query token: %s\n", sToken.GetBuffer(0));
		}

		sToken = GetNextQueryToken();
	}


	// Flush the reponse to send any remaining info...

	FlushResponse();


	// All done...

	return(TRUE);
}

BOOL CGameSpyMgr::OnQuery(char* sAddr, DWORD dwPort, char* sData, int nLen)
{
	// Sanity checks...

	if (!sAddr) return(FALSE);
	if (!sData) return(FALSE);


	// Setup the query object...

	CGameSpyQueryInfo	qi;

	qi.SetAddr(sAddr);
	qi.SetPort(dwPort);
	qi.SetData(sData);


	// Process the query...

	return(OnQuery(&qi));
}


// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CGameSpyMgr::OnEchoQuery
//
//	PURPOSE:	Processes an "echo" query command
//
// ----------------------------------------------------------------------- //

BOOL CGameSpyMgr::OnEchoQuery(const char* sEcho)
{
	// Sanity checks...

	if (!sEcho) return(FALSE);


	// Echo back the info we were given...

	SendResponseInfo("echo", sEcho);


	// All done...

	return(TRUE);
}


// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CGameSpyMgr::OnSecureQuery
//
//	PURPOSE:	Processes a "secure" query command
//
// ----------------------------------------------------------------------- //

BOOL CGameSpyMgr::OnSecureQuery(const char* sValue)
{
	// Sanity checks...

	if (!sValue) return(FALSE);


	// Encrypt the value...

	// RETAIL BUG: sValue comes from the query packet and is copied without a
	// length check, so a "secure" value of 64 characters or more overruns
	// sEncrypt.

	char sEncrypt[64];
#ifdef AVP2SRC_MODERN
	// M-B47 (RETAIL_BUGS.md): as much of it as fits
	strncpy(sEncrypt, sValue, sizeof(sEncrypt) - 1);
	sEncrypt[sizeof(sEncrypt) - 1] = '\0';
#else
	strcpy(sEncrypt, sValue);
#endif

	gs_encrypt((unsigned char*)sEncrypt, VALIDATE_SIZE, m_sGameKey);


	// Encode the value...

	char sEncode[64] = { "" };

	gs_encode((unsigned char*)sEncrypt, VALIDATE_SIZE, sEncode);


	// Echo back the info we were given...

	SendResponseInfo("validate", sEncode);


	// All done...

	return(TRUE);
}


// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CGameSpyMgr::OnBasicQuery
//
//	PURPOSE:	Processes a "basic" query command
//
// ----------------------------------------------------------------------- //

BOOL CGameSpyMgr::OnBasicQuery()
{
	// Send the basic info...

	SendResponseInfo("gamename", m_sGameName);
	SendResponseInfo("gamever", m_sGameVer);
	SendResponseInfo("location", m_sGameLoc);


	// All done...

	return(TRUE);
}


// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CGameSpyMgr::OnStatusQuery
//
//	PURPOSE:	Processes a "status" query command
//
// ----------------------------------------------------------------------- //

BOOL CGameSpyMgr::OnStatusQuery()
{
	// Send all the status info...

	OnBasicQuery();
	OnInfoQuery();
	OnRulesQuery();
	OnPlayersQuery();


	// All done...

	return(TRUE);
}


// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CGameSpyMgr::UpdateHeartbeat
//
//	PURPOSE:	Updates the heartbeat for GameSpy as necessary
//
// ----------------------------------------------------------------------- //

BOOL CGameSpyMgr::UpdateHeartbeat()
{
	// Check if it's time to update...

	static DWORD timeLastUpdate = 0;

	DWORD timeCur   = GetTickCount();
	DWORD timeDelta = timeCur - timeLastUpdate;

	timeLastUpdate = timeCur;

	if (timeDelta >= m_timerHeartbeat)
	{
		m_timerHeartbeat = TIMER_HEARTBEAT;

#ifndef AVP2SRC_MODERN
		RenewRegistration(this);
#endif
	}
	else
	{
		m_timerHeartbeat -= timeDelta;
		return(FALSE);
	}


	// Update the heartbeat with a response...

#ifdef AVP2SRC_MODERN
	// M-17: to the master named, once its address is known
	if (m_sGameSpyAddr.IsEmpty())
	{
		if (!ResolveMaster()) return(FALSE);
		m_sGameSpyAddr = s_szMasterIP;
	}

	StartResponse(m_sGameSpyAddr, s_dwMasterPort);
#else
	StartResponse(m_sGameSpyAddr, 27900);
#endif

	DWORD dwPort = GetSpyPort();
	if (UseGamePortForHeartbeat()) dwPort = GetGamePort();

	SendResponseInfo("heartbeat", dwPort);
	SendResponseInfo("gamename", m_sGameName);

	FlushResponse();


	// All done...

	return(TRUE);
}


// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CGameSpyMgr::GetFirstQueryToken
//
//	PURPOSE:	Gets the first query token in the given query string
//
// ----------------------------------------------------------------------- //

CString CGameSpyMgr::GetFirstQueryToken(const char* sQuery)
{
	// Sanity checks...

	if (!sQuery) return("");


	// Copy the buffer to our line buffer...

	memset(s_sLine, 0, 1024);
	strncpy(s_sLine, sQuery, 1020);

	s_pCur = s_sLine;


	// Get the next token since it will really be the first one...

	return(GetNextQueryToken());
}


// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CGameSpyMgr::GetNextQueryToken
//
//	PURPOSE:	Gets the next query token from our internal query string
//
// ----------------------------------------------------------------------- //

CString CGameSpyMgr::GetNextQueryToken()
{
	// Sanity checks...

	if (s_sLine[0] == '\0') return("");
	if (s_pCur == NULL) return("");


	// Skip over leading garbage...

	if (*s_pCur == '\0') return("");

	while (*s_pCur < 32 || *s_pCur > 200 || *s_pCur == '\\')
	{
		s_pCur++;

		if (*s_pCur == '\0') return("");
	}


	// Get the token...

	CString sToken;

	while (*s_pCur >= 32 && *s_pCur <= 200 && *s_pCur != '\\')
	{
		sToken += *s_pCur;
		s_pCur++;
	}


	// All done...

	return(sToken);
}


// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CGameSpyMgr::SendSocketString
//
//	PURPOSE:	Sends the given string through our socket
//
// ----------------------------------------------------------------------- //

BOOL CGameSpyMgr::SendSocketString(const char* sAddr, DWORD dwPort, const char* sText, int nLength)
{
	// Sanity checks...

	if (!sText) return(FALSE);
	if (!sAddr) return(FALSE);


	// Check if we should calculate the string length...

	if (nLength < 0)
	{
		nLength = strlen(sText);
	}


	// Send the string...

	if (m_pSender)
	{
		// Let the send handler do it...

		m_pSender->SendTo(sText, nLength, sAddr, dwPort);
	}
	

	// All done...

	return(TRUE);
}


// ----------------------------------------------------------------------- //
//
//	ROUTINE:	InitSierraMultiplayer
//
//	PURPOSE:	Sets up the WON authentication and directory registration
//
// ----------------------------------------------------------------------- //

static void InitSierraMultiplayer(CGameSpyMgr* pGameSpyMgr)
{
	// Create the authentication context...

	gAuthContext = new AuthContext;
	pGameSpyMgr->m_pAuthContext = gAuthContext;

	HashFileList aFileList;
	aFileList.push_back(std::string("lithtech.exe"));
	aFileList.push_back(std::string("server.dll"));
	aFileList.push_back(std::string("d3d.ren"));

	gAuthContext->SetHashFileList(WONAPI_AVP2COMMUNITY, aFileList);
	gAuthContext->SetCommunity(WONAPI_AVP2COMMUNITY);


	// Read the directory servers from the config file...

	gDirContext = new ServerContext;

	// RETAIL BUG: the file is never checked for NULL (fgets crashes without
	// SierraUp.cfg) and never closed, and a DirectoryServer line without
	// two quotes (or a quoted address of 256 characters or more) runs off
	// the strstr results or the end of sAddr.

	char sLine[256];
	FILE* pFile = fopen("SierraUp.cfg", "r");

	while (fgets(sLine, 256, pFile))
	{
		if (strnicmp(sLine, "DirectoryServer", 15) == 0)
		{
			char* pStart = strstr(sLine, "\"") + 1;
			char* pEnd   = strstr(pStart, "\"");

			char sAddr[256];
			strncpy(sAddr, pStart, pEnd - pStart);
			sAddr[pEnd - pStart] = '\0';

			AddDirServer(sAddr);
		}
	}


	// Set up the registration ops...

	short nPort = (short)(htonl(pGameSpyMgr->GetGamePort()) >> 16);

	WriteBuffer aNetAddr;
	aNetAddr.AppendShort(nPort);

	gAddServiceOp = new AddServiceOp(gDirContext);
	gAddServiceOp->SetNetAddr(aNetAddr.ToByteBuffer());
	gAddServiceOp->SetPath(GameSpyMgr_WONAPI_AVP2REGISTERPATH);
	gAddServiceOp->SetLifespan(300);
	gAddServiceOp->SetCompletion(new ParamCompletion<AsyncOpPtr, CGameSpyMgr*>(RegisterCompletion, pGameSpyMgr));
	gAddServiceOp->SetFlags(0x38);

	gRenewServiceOp = new RenewServiceOp(gDirContext);
	gRenewServiceOp->SetNetAddr(IPAddr((const char*)pGameSpyMgr->GetGameAddr(), pGameSpyMgr->GetGamePort()));
	gRenewServiceOp->SetPath(GameSpyMgr_WONAPI_AVP2REGISTERPATH);
	gRenewServiceOp->SetLifespan(300);
	gRenewServiceOp->SetCompletion(new ParamCompletion<AsyncOpPtr, CGameSpyMgr*>(RegisterCompletion, pGameSpyMgr));


	// Get the auth servers from the directory...

	gGetDirOp = new GetDirOp(gDirContext);
	gGetDirOp->SetPath(GameSpyMgr_WONAPI_TITANSERVERS);
	gGetDirOp->SetFlags(DIR_GF_DECOMPSERVICES | DIR_GF_SERVADDNETADDR);
	gGetDirOp->SetCompletion(new ParamCompletion<AsyncOpPtr, CGameSpyMgr*>(GetDirOpCompletion, pGameSpyMgr));
	gGetDirOp->RunAsync(20000);
}


// ----------------------------------------------------------------------- //
//
//	ROUTINE:	AddDirServer
//
//	PURPOSE:	Adds a directory server address to the directory context
//
// ----------------------------------------------------------------------- //

static void AddDirServer(const char* sAddr)
{
	if (gDirContext == NULL) return;

	IPAddr aAddr(sAddr);
	gDirContext->AddAddress(aAddr);
}


// ----------------------------------------------------------------------- //
//
//	ROUTINE:	GetDirOpCompletion
//
//	PURPOSE:	Gets our certificate once we have the auth servers
//
// ----------------------------------------------------------------------- //

static void GetDirOpCompletion(AsyncOpPtr theOp, CGameSpyMgr* pGameSpyMgr)
{
	GetDirOpPtr aGetDirOp = (GetDirOp*)theOp.get();

	if (aGetDirOp->Succeeded())
	{
		gAuthContext->AddAddressesFromDir(aGetDirOp->GetDirEntityList());

		GetCertOpPtr aGetCertOp = new GetCertOp(gAuthContext);
		aGetCertOp->SetCompletion(new ParamCompletion<AsyncOpPtr, CGameSpyMgr*>(GetCertOpCompletion, pGameSpyMgr));
		aGetCertOp->RunAsync(20000);
	}
}


// ----------------------------------------------------------------------- //
//
//	ROUTINE:	RenewRegistration
//
//	PURPOSE:	Renews our directory registration
//
// ----------------------------------------------------------------------- //

static void RenewRegistration(CGameSpyMgr* pGameSpyMgr)
{
	gRenewServiceOp->RunAsync(20000);
}


// ----------------------------------------------------------------------- //
//
//	ROUTINE:	GetCertOpCompletion
//
//	PURPOSE:	Registers with the directory once we have our certificate
//
// ----------------------------------------------------------------------- //

static void GetCertOpCompletion(AsyncOpPtr theOp, CGameSpyMgr* pGameSpyMgr)
{
	GetCertOpPtr aGetCertOp = (GetCertOp*)theOp.get();

	if (aGetCertOp->Succeeded())
	{
		AddRegistration(pGameSpyMgr);

		g_SierraInitCompleted = TRUE;
	}
}


// ----------------------------------------------------------------------- //
//
//	ROUTINE:	AddRegistration
//
//	PURPOSE:	Registers with the directory
//
// ----------------------------------------------------------------------- //

static void AddRegistration(CGameSpyMgr* pGameSpyMgr)
{
	gAddServiceOp->RunAsync(20000);
}


// ----------------------------------------------------------------------- //
//
//	ROUTINE:	RegisterCompletion
//
//	PURPOSE:	Re-registers if a renewal finds our service gone
//
// ----------------------------------------------------------------------- //

static void RegisterCompletion(AsyncOpPtr theOp, CGameSpyMgr* pGameSpyMgr)
{
	if (theOp->GetStatus() == WS_DirServ_ServiceNotFound)
	{
		AddRegistration(pGameSpyMgr);
	}
}
