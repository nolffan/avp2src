// *********************************************************************** //
//
//  MODULE:   GAMESPYCLIENTMGR (.CPP)  -- our replacement for
//            GameSpyClientMgr.lib
//
//  PURPOSE:  The client-side server browser CShell.dll links
//            (CGameSpyClientMgr, CGameSpyServer, CGameSpyPlayer; headers in
//            src/LT2/lithshared/incs and src/LT2/lithshared/gamespy). Only the
//            multiplayer "Join" folder (FolderJoin.cpp) uses it.
//
//  WHY THIS IS NOT A RECONSTRUCTION.  The prebuilt library is the GameSpy
//  C Engine SDK plus thin Monolith wrappers -- middleware, deliberately not
//  byte-matched. The build compiles
//  every binary from our own source, so this file stands in for it: the
//  same interface, written from the headers alone.
//
//  WHAT IT DOES.  The SDK filled its list from master.gamespy.com, which
//  was shut down in 2013. With no master there is nothing to list, so the
//  manager initialises, refreshes to an empty list and stays idle; the Join
//  folder shows no servers. The server and player wrappers are complete
//  but are never handed a server.
//
// *********************************************************************** //

#include <windows.h>
#include <string.h>
#include <stdlib.h>
#include "GameSpyClientMgr.h"
#ifdef AVP2_MODERN
//  THE MODERN BUILD (AVP2_MODERN, M-P6-02) does fill the list: from the
//  master the game names with GameSpyClient_SetMasterServer (default
//  master.openspy.net), then by querying each server; see gsmaster.h and
//  the first #ifdef below.
#include <stdio.h>
#include "gsmaster.h"
#endif

#ifdef AVP2_MODERN

// ----------------------------------------------------------------------- //
//  M-P6-02: the modern build's browser. RefreshServers asks the master
//  (gsmaster.h) for the list over TCP, then queries every listed server
//  with \status\ over UDP, GSCM_MAX_UPDATES at a time; Update, called every
//  frame by the Join folder, moves both along without blocking (except the
//  master's name lookup when a refresh starts). A server is listed once it
//  has answered; one that never answers is left out.
// ----------------------------------------------------------------------- //

#define GSL_MAX_SERVERS		1024	// servers taken from one master list
#define GSL_MAX_PAIRS		224		// keys kept per server
#define GSL_DATA_SIZE		6144	// key and value text kept per server
#define GSL_MAX_KEY			63
#define GSL_MAX_VALUE		100		// the Join folder formats values into 128-byte buffers
#define GSL_LIST_BUFFER		(GSL_MAX_SERVERS * 6 + 1024)
#define GSL_LIST_TIMEOUT	10000	// ms for the whole master conversation
#define GSL_QUERY_RETRY		1000	// ms before a query is sent again
#define GSL_QUERY_TRIES		3		// sends before a server is given up (SERVER_TIMEOUT)

enum { GSS_WAITING, GSS_QUERYING, GSS_DONE, GSS_FAILED };

struct GServerImplementation
{
	unsigned long	nIP;			// network order
	unsigned short	nPort;			// host order
	int				nState;			// GSS_*
	int				nTries;
	DWORD			dwSent;			// first send, for the ping
	DWORD			dwLastSend;
	int				nPing;
	int				nPacketsGot;
	int				nPackets;		// from the packet carrying \final\, 0 until then
	int				nPairs;
	int				nDataUsed;
	short			aKey[GSL_MAX_PAIRS];
	short			aValue[GSL_MAX_PAIRS];
	char			sData[GSL_DATA_SIZE];
};

struct GServerListImplementation
{
	int				nState;			// sl_*
	SOCKET			hTCP;
	SOCKET			hUDP;
	DWORD			dwListStart;
	BOOL			bSentRequest;
	int				nRecv;
	char			aRecv[GSL_LIST_BUFFER];

	GServerImplementation*	aServers;
	int				nServers;
	int				nNextQuery;
	int				nActive;
	int				nFinished;

	GServerImplementation*	apListed[GSL_MAX_SERVERS];
	int				nListed;
};

static char s_sMasterServer[128] = "";
static BOOL s_bWinsock = FALSE;

void GameSpyClient_SetMasterServer(const char* sAddr)
{
	strncpy(s_sMasterServer, sAddr ? sAddr : "", sizeof(s_sMasterServer) - 1);
	s_sMasterServer[sizeof(s_sMasterServer) - 1] = '\0';
}


// --- one server's keys -------------------------------------------------- //

static char* GSL_GetValue(GServerImplementation* pServer, const char* sKey)
{
	if (!pServer || !sKey) return(NULL);
	for (int i = 0; i < pServer->nPairs; i++)
	{
		if (stricmp(&pServer->sData[pServer->aKey[i]], sKey) == 0)
			return(&pServer->sData[pServer->aValue[i]]);
	}
	return(NULL);
}

static void GSL_AddPair(GServerImplementation* pServer, const char* pKey, int nKeyLen,
                        const char* pValue, int nValueLen)
{
	if (nKeyLen <= 0) return;
	if (nKeyLen > GSL_MAX_KEY) nKeyLen = GSL_MAX_KEY;
	if (nValueLen > GSL_MAX_VALUE) nValueLen = GSL_MAX_VALUE;
	if (pServer->nPairs >= GSL_MAX_PAIRS) return;
	if (pServer->nDataUsed + nKeyLen + nValueLen + 2 > GSL_DATA_SIZE) return;

	// the same key again (a repeated packet): keep the first
	char sKey[GSL_MAX_KEY + 1];
	memcpy(sKey, pKey, nKeyLen);
	sKey[nKeyLen] = '\0';
	if (GSL_GetValue(pServer, sKey)) return;

	char* p = &pServer->sData[pServer->nDataUsed];
	pServer->aKey[pServer->nPairs] = (short)pServer->nDataUsed;
	memcpy(p, sKey, nKeyLen + 1);
	p += nKeyLen + 1;
	pServer->aValue[pServer->nPairs] = (short)(p - pServer->sData);
	memcpy(p, pValue, nValueLen);
	p[nValueLen] = '\0';
	pServer->nDataUsed += nKeyLen + nValueLen + 2;
	pServer->nPairs++;
}

static void GSL_AddString(GServerImplementation* pServer, const char* sKey, const char* sValue)
{
	GSL_AddPair(pServer, sKey, (int)strlen(sKey), sValue, (int)strlen(sValue));
}

// One reply packet: \key\value...[\final\]\queryid\<id>.<packet>
static void GSL_ParseReply(GServerImplementation* pServer, const char* pData, int nLen)
{
	const char* p    = pData;
	const char* pEnd = pData + nLen;
	BOOL bFinal  = FALSE;
	int  nPacket = 1;

	while (p < pEnd && *p == '\\')
	{
		const char* pKey = ++p;
		while (p < pEnd && *p != '\\') p++;
		int nKeyLen = (int)(p - pKey);
		if (p >= pEnd) break;

		const char* pValue = ++p;
		while (p < pEnd && *p != '\\') p++;
		int nValueLen = (int)(p - pValue);

		if (nKeyLen == 5 && strnicmp(pKey, "final", 5) == 0)
		{
			bFinal = TRUE;		// "\final\" has no value: what follows is the next key
			continue;
		}
		if (nKeyLen == 7 && strnicmp(pKey, "queryid", 7) == 0)
		{
			const char* pDot = pValue;
			while (pDot < pValue + nValueLen && *pDot != '.') pDot++;
			if (pDot < pValue + nValueLen) nPacket = atoi(pDot + 1);
			continue;
		}
		GSL_AddPair(pServer, pKey, nKeyLen, pValue, nValueLen);
	}

	pServer->nPacketsGot++;
	if (bFinal) pServer->nPackets = (nPacket > 0) ? nPacket : 1;
}


// --- the list ------------------------------------------------------------ //

static void GSL_CloseSockets(GServerListImplementation* pList)
{
	if (pList->hTCP != INVALID_SOCKET) closesocket(pList->hTCP);
	if (pList->hUDP != INVALID_SOCKET) closesocket(pList->hUDP);
	pList->hTCP = INVALID_SOCKET;
	pList->hUDP = INVALID_SOCKET;
}

static void GSL_Reset(GServerListImplementation* pList)
{
	GSL_CloseSockets(pList);
	if (pList->aServers) free(pList->aServers);
	pList->aServers     = NULL;
	pList->nServers     = 0;
	pList->nNextQuery   = 0;
	pList->nActive      = 0;
	pList->nFinished    = 0;
	pList->nListed      = 0;
	pList->nRecv        = 0;
	pList->bSentRequest = FALSE;
	pList->nState       = sl_idle;
}

static void GSL_SetNonBlocking(SOCKET hSocket)
{
	u_long nOn = 1;
	ioctlsocket(hSocket, FIONBIO, &nOn);
}

// The master's binary list (6 bytes per server) -> aServers.
static void GSL_TakeList(GServerListImplementation* pList, const char* pData, int nLen)
{
	int nCount = nLen / 6;
	if (nCount > GSL_MAX_SERVERS) nCount = GSL_MAX_SERVERS;
	if (nCount <= 0) return;

	pList->aServers = (GServerImplementation*)malloc(nCount * sizeof(GServerImplementation));
	if (!pList->aServers) return;

	for (int i = 0; i < nCount; i++)
	{
		const unsigned char* p = (const unsigned char*)pData + i * 6;
		unsigned long  nIP;
		unsigned short nPort = (unsigned short)((p[4] << 8) | p[5]);
		memcpy(&nIP, p, 4);
		if (!nIP || nIP == INADDR_NONE || !nPort) continue;

		BOOL bDuplicate = FALSE;
		for (int j = 0; j < pList->nServers; j++)
		{
			if (pList->aServers[j].nIP == nIP && pList->aServers[j].nPort == nPort)
			{
				bDuplicate = TRUE;
				break;
			}
		}
		if (bDuplicate) continue;

		GServerImplementation* pServer = &pList->aServers[pList->nServers++];
		memset(pServer, 0, sizeof(*pServer) - sizeof(pServer->sData));
		pServer->sData[0] = '\0';
		pServer->nIP    = nIP;
		pServer->nPort  = nPort;
		pServer->nState = GSS_WAITING;

		struct in_addr in;
		in.s_addr = nIP;
		GSL_AddString(pServer, "hostaddr", inet_ntoa(in));
	}
}

// The master conversation, one step.
static void GSL_UpdateListXfer(GServerListImplementation* pList, const char* sGameName, const char* sSecretKey)
{
	BOOL bDone = FALSE;

	fd_set rd, ex;
	struct timeval tv = { 0, 0 };
	FD_ZERO(&rd);
	FD_ZERO(&ex);
	FD_SET(pList->hTCP, &rd);
	FD_SET(pList->hTCP, &ex);

	int nReady = select(0, &rd, NULL, &ex, &tv);
	if (nReady == SOCKET_ERROR || FD_ISSET(pList->hTCP, &ex))
	{
		bDone = TRUE;			// could not connect
	}
	else if (nReady > 0 && FD_ISSET(pList->hTCP, &rd))
	{
		int nSpace = GSL_LIST_BUFFER - 1 - pList->nRecv;
		int nGot   = (nSpace > 0) ? recv(pList->hTCP, &pList->aRecv[pList->nRecv], nSpace, 0) : 0;
		if (nGot > 0)
		{
			pList->nRecv += nGot;
			pList->aRecv[pList->nRecv] = '\0';
		}
		else if (nGot == 0 || WSAGetLastError() != WSAEWOULDBLOCK)
		{
			bDone = TRUE;		// the master closes when the list is sent
		}
	}

	if (!pList->bSentRequest)
	{
		// \basic\\secure\<challenge>: answer it and ask for the list
		char sChallenge[80];
		if (!bDone && GSM_FindKey(pList->aRecv, "secure", sChallenge, sizeof(sChallenge)) &&
		    pList->nRecv > 0 && pList->aRecv[pList->nRecv - 1] != '\\')
		{
			char sValidate[96];
			char sRequest[512];
			GSM_Validate(sChallenge, sSecretKey, sValidate);
			sprintf(sRequest,
			        "\\gamename\\%.64s\\enctype\\0\\validate\\%s\\final\\"
			        "\\queryid\\1.1\\"
			        "\\list\\cmp\\gamename\\%.64s\\final\\",
			        sGameName, sValidate, sGameName);
			send(pList->hTCP, sRequest, (int)strlen(sRequest), 0);
			pList->bSentRequest = TRUE;
			pList->nRecv = 0;
		}
	}
	else if (pList->nRecv >= 7 && memcmp(&pList->aRecv[pList->nRecv - 7], "\\final\\", 7) == 0)
	{
		bDone = TRUE;
	}

	if (!bDone && (GetTickCount() - pList->dwListStart) > GSL_LIST_TIMEOUT) bDone = TRUE;
	if (!bDone) return;

	// Take what came (an \error\ reply is not a list).
	if (pList->bSentRequest &&
	    !(pList->nRecv >= 7 && (memcmp(pList->aRecv, "\\error\\", 7) == 0 || memcmp(pList->aRecv, "\\fatal\\", 7) == 0)))
	{
		int nLen = pList->nRecv;
		if (nLen >= 7 && memcmp(&pList->aRecv[nLen - 7], "\\final\\", 7) == 0) nLen -= 7;
		GSL_TakeList(pList, pList->aRecv, nLen);
	}

	closesocket(pList->hTCP);
	pList->hTCP = INVALID_SOCKET;

	if (pList->nServers > 0)
	{
		pList->hUDP = socket(AF_INET, SOCK_DGRAM, 0);
		if (pList->hUDP != INVALID_SOCKET)
		{
			GSL_SetNonBlocking(pList->hUDP);
			pList->nState = sl_querying;
			return;
		}
	}

	GSL_Reset(pList);
}

static void GSL_SendQuery(GServerListImplementation* pList, GServerImplementation* pServer)
{
	struct sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
	addr.sin_family      = AF_INET;
	addr.sin_addr.s_addr = pServer->nIP;
	addr.sin_port        = htons(pServer->nPort);

	static const char s_sQuery[] = "\\status\\";
	sendto(pList->hUDP, s_sQuery, sizeof(s_sQuery) - 1, 0, (struct sockaddr*)&addr, sizeof(addr));

	pServer->dwLastSend = GetTickCount();
	if (!pServer->nTries) pServer->dwSent = pServer->dwLastSend;
	pServer->nTries++;
}

static void GSL_Finish(GServerListImplementation* pList, GServerImplementation* pServer, BOOL bAnswered)
{
	pServer->nState = bAnswered ? GSS_DONE : GSS_FAILED;
	pList->nActive--;
	pList->nFinished++;

	if (bAnswered && pList->nListed < GSL_MAX_SERVERS)
	{
		char sPing[16];
		sprintf(sPing, "%d", pServer->nPing > 0 ? pServer->nPing : 1);
		GSL_AddString(pServer, "ping", sPing);
		pList->apListed[pList->nListed++] = pServer;
	}
}

// The server queries, one step. TRUE when a server was added to the list.
static BOOL GSL_UpdateQueries(GServerListImplementation* pList)
{
	BOOL  bNew  = FALSE;
	DWORD dwNow = GetTickCount();
	int   i;

	// replies
	for (;;)
	{
		char aPacket[1500];
		struct sockaddr_in from;
		int nFromLen = sizeof(from);
		int nGot = recvfrom(pList->hUDP, aPacket, sizeof(aPacket) - 1, 0, (struct sockaddr*)&from, &nFromLen);
		if (nGot == SOCKET_ERROR)
		{
			if (WSAGetLastError() == WSAECONNRESET) continue;	// an ICMP "port unreachable"
			break;
		}
		if (nGot <= 0) break;
		aPacket[nGot] = '\0';

		for (i = 0; i < pList->nServers; i++)
		{
			GServerImplementation* pServer = &pList->aServers[i];
			if (pServer->nState != GSS_QUERYING) continue;
			if (pServer->nIP != from.sin_addr.s_addr || pServer->nPort != ntohs(from.sin_port)) continue;

			if (!pServer->nPacketsGot) pServer->nPing = (int)(GetTickCount() - pServer->dwSent);
			GSL_ParseReply(pServer, aPacket, nGot);
			if (pServer->nPackets && pServer->nPacketsGot >= pServer->nPackets)
			{
				GSL_Finish(pList, pServer, TRUE);
				bNew = TRUE;
			}
			break;
		}
	}

	// timeouts and resends
	for (i = 0; i < pList->nServers; i++)
	{
		GServerImplementation* pServer = &pList->aServers[i];
		if (pServer->nState != GSS_QUERYING) continue;
		if ((dwNow - pServer->dwLastSend) < GSL_QUERY_RETRY) continue;

		if (pServer->nPacketsGot)
		{
			// part of the reply came: take what there is
			GSL_Finish(pList, pServer, TRUE);
			bNew = TRUE;
		}
		else if (pServer->nTries >= GSL_QUERY_TRIES)
		{
			GSL_Finish(pList, pServer, FALSE);
		}
		else
		{
			GSL_SendQuery(pList, pServer);
		}
	}

	// new queries
	while (pList->nActive < GSCM_MAX_UPDATES && pList->nNextQuery < pList->nServers)
	{
		GServerImplementation* pServer = &pList->aServers[pList->nNextQuery++];
		pServer->nState = GSS_QUERYING;
		pList->nActive++;
		GSL_SendQuery(pList, pServer);
	}

	if (pList->nFinished >= pList->nServers)
	{
		GSL_CloseSockets(pList);
		pList->nState = sl_idle;
	}

	return(bNew);
}

static BOOL GSL_IsListed(GServerListImplementation* pList, void* pHandle)
{
	if (!pList || !pHandle) return(FALSE);
	for (int i = 0; i < pList->nListed; i++)
	{
		if ((void*)pList->apListed[i] == pHandle) return(TRUE);
	}
	return(FALSE);
}


// --- sorting ------------------------------------------------------------- //

static const char*  s_sSortKey  = "";
static GCompareMode s_eSortMode = cm_stricase;
static BOOL         s_bSortUp   = TRUE;

static int GSL_Compare(const void* pA, const void* pB)
{
	char* sA = GSL_GetValue(*(GServerImplementation**)pA, s_sSortKey);
	char* sB = GSL_GetValue(*(GServerImplementation**)pB, s_sSortKey);
	if (!sA) sA = "";
	if (!sB) sB = "";

	int nCmp;
	switch (s_eSortMode)
	{
		case cm_int:
		{
			int a = atoi(sA), b = atoi(sB);
			nCmp = (a < b) ? -1 : ((a > b) ? 1 : 0);
			break;
		}
		case cm_float:
		{
			double a = atof(sA), b = atof(sB);
			nCmp = (a < b) ? -1 : ((a > b) ? 1 : 0);
			break;
		}
		case cm_strcase: nCmp = strcmp(sA, sB);  break;
		default:         nCmp = stricmp(sA, sB); break;
	}

	return(s_bSortUp ? nCmp : -nCmp);
}


// --- CGameSpyClientMgr ---------------------------------------------------- //

BOOL CGameSpyClientMgr::Init(const char* sGameName, const char* sSecretKey)
{
	Term();

	strncpy(m_sGameName, sGameName ? sGameName : "", GSCM_MAX_STRING - 1);
	m_sGameName[GSCM_MAX_STRING - 1] = '\0';
	strncpy(m_sSecretKey, sSecretKey ? sSecretKey : "", GSCM_MAX_STRING - 1);
	m_sSecretKey[GSCM_MAX_STRING - 1] = '\0';

	if (!s_bWinsock)
	{
		WSADATA wsaData;
		s_bWinsock = (WSAStartup(MAKEWORD(1, 1), &wsaData) == 0);
	}

	if (!CreateServerList()) return(FALSE);

	m_bInit = TRUE;
	return(TRUE);
}

void CGameSpyClientMgr::Term()
{
	FreeServerList();
	Clear();
}

void CGameSpyClientMgr::Clear()
{
	m_bInit           = FALSE;
	m_pServerList     = NULL;
	m_sGameName[0]    = '\0';
	m_sSecretKey[0]   = '\0';
	m_nGetServerIndex = 0;
	m_nProgress       = 0;
	m_bNewServer      = FALSE;
}

BOOL CGameSpyClientMgr::CreateServerList()
{
	GServerListImplementation* pList = (GServerListImplementation*)malloc(sizeof(GServerListImplementation));
	if (!pList) return(FALSE);

	memset(pList, 0, sizeof(*pList));
	pList->hTCP = INVALID_SOCKET;
	pList->hUDP = INVALID_SOCKET;
	GSL_Reset(pList);

	m_pServerList = pList;
	return(TRUE);
}

void CGameSpyClientMgr::FreeServerList()
{
	if (m_pServerList)
	{
		GSL_Reset(m_pServerList);
		free(m_pServerList);
	}
	m_pServerList = NULL;
}

int CGameSpyClientMgr::GetNumServers()
{
	return(m_pServerList ? m_pServerList->nListed : 0);
}

int CGameSpyClientMgr::GetState()
{
	return(m_pServerList ? m_pServerList->nState : sl_idle);
}

void CGameSpyClientMgr::ClearServers()
{
	if (m_pServerList) GSL_Reset(m_pServerList);
	m_nGetServerIndex = 0;
	m_bNewServer      = FALSE;
}

BOOL CGameSpyClientMgr::RefreshServers(BOOL bAsync)
{
	if (!m_bInit || !m_pServerList) return(FALSE);

	ClearServers();
	m_nProgress = 100;

	GServerListImplementation* pList = m_pServerList;

	unsigned long nIP;
	int nPort;
	if (!GSM_Resolve(s_sMasterServer, GSM_LIST_PORT, &nIP, &nPort)) return(TRUE);

	pList->hTCP = socket(AF_INET, SOCK_STREAM, 0);
	if (pList->hTCP == INVALID_SOCKET) return(TRUE);
	GSL_SetNonBlocking(pList->hTCP);

	struct sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
	addr.sin_family      = AF_INET;
	addr.sin_addr.s_addr = nIP;
	addr.sin_port        = htons((unsigned short)nPort);

	if (connect(pList->hTCP, (struct sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR &&
	    WSAGetLastError() != WSAEWOULDBLOCK)
	{
		GSL_Reset(pList);
		return(TRUE);
	}

	pList->dwListStart = GetTickCount();
	pList->nState      = sl_listxfer;
	m_nProgress        = 0;

	// The synchronous form: run the whole refresh now.
	if (!bAsync)
	{
		while (pList->nState != sl_idle)
		{
			Update();
			Sleep(10);
		}
	}

	return(TRUE);
}

CGameSpyServer* CGameSpyClientMgr::GetServer(int nIndex)
{
	if (!m_pServerList || nIndex < 0 || nIndex >= m_pServerList->nListed) return(NULL);
	return(CreateGameSpyServer(m_pServerList->apListed[nIndex]));
}

CGameSpyServer* CGameSpyClientMgr::GetServerFromHandle(void* pHandle)
{
	if (!GSL_IsListed(m_pServerList, pHandle)) return(NULL);
	return(CreateGameSpyServer((GServer)pHandle));
}

CGameSpyServer* CGameSpyClientMgr::GetFirstServer()
{
	m_nGetServerIndex = 0;
	return(GetServer(m_nGetServerIndex));
}

CGameSpyServer* CGameSpyClientMgr::GetNextServer()
{
	m_nGetServerIndex++;
	return(GetServer(m_nGetServerIndex));
}

BOOL CGameSpyClientMgr::ExistNewServer()
{
	BOOL bNew = m_bNewServer;
	m_bNewServer = FALSE;
	return(bNew);
}

void CGameSpyClientMgr::SortServersByName(BOOL bAscending)     { SortServers(bAscending, "hostname", cm_stricase); }
void CGameSpyClientMgr::SortServersByPing(BOOL bAscending)     { SortServers(bAscending, "ping", cm_int); }
void CGameSpyClientMgr::SortServersByPlayers(BOOL bAscending)  { SortServers(bAscending, "numplayers", cm_int); }
void CGameSpyClientMgr::SortServersByMap(BOOL bAscending)      { SortServers(bAscending, "mapname", cm_stricase); }
void CGameSpyClientMgr::SortServersByGameType(BOOL bAscending) { SortServers(bAscending, "gametype", cm_stricase); }

void CGameSpyClientMgr::SortServers(BOOL bAscending, char* sSortKey, GCompareMode eCompareMode)
{
	if (!m_pServerList || m_pServerList->nListed < 2 || !sSortKey) return;

	s_sSortKey  = sSortKey;
	s_eSortMode = eCompareMode;
	s_bSortUp   = bAscending;
	qsort(m_pServerList->apListed, m_pServerList->nListed, sizeof(GServerImplementation*), GSL_Compare);
}

void CGameSpyClientMgr::Update()
{
	GServerListImplementation* pList = m_pServerList;
	if (!m_bInit || !pList) return;

	switch (pList->nState)
	{
		case sl_listxfer:
			GSL_UpdateListXfer(pList, m_sGameName, m_sSecretKey);
			break;

		case sl_querying:
			if (GSL_UpdateQueries(pList)) m_bNewServer = TRUE;
			m_nProgress = pList->nServers ? (pList->nFinished * 100 / pList->nServers) : 100;
			break;
	}
}

void CGameSpyClientMgr::HandleCallback(GServerList pServerList, int nMsg, void* pParam1, void* pParam2)
{
	if (nMsg == LIST_PROGRESS)
	{
		m_nProgress  = (int)pParam2;
		m_bNewServer = TRUE;
	}
}

CGameSpyServer* CGameSpyClientMgr::CreateGameSpyServer(GServer pGServer)
{
	if (!pGServer) return(NULL);
	if (!m_CurServer.Init(pGServer)) return(NULL);
	return(&m_CurServer);
}

#else

// ----------------------------------------------------------------------- //
//  CGameSpyClientMgr
// ----------------------------------------------------------------------- //

BOOL CGameSpyClientMgr::Init(const char* sGameName, const char* sSecretKey)
{
	Term();

	strncpy(m_sGameName, sGameName ? sGameName : "", GSCM_MAX_STRING - 1);
	m_sGameName[GSCM_MAX_STRING - 1] = '\0';
	strncpy(m_sSecretKey, sSecretKey ? sSecretKey : "", GSCM_MAX_STRING - 1);
	m_sSecretKey[GSCM_MAX_STRING - 1] = '\0';

	if (!CreateServerList()) return(FALSE);

	m_bInit = TRUE;
	return(TRUE);
}

void CGameSpyClientMgr::Term()
{
	FreeServerList();
	Clear();
}

void CGameSpyClientMgr::Clear()
{
	m_bInit           = FALSE;
	m_pServerList     = NULL;
	m_sGameName[0]    = '\0';
	m_sSecretKey[0]   = '\0';
	m_nGetServerIndex = 0;
	m_nProgress       = 0;
	m_bNewServer      = FALSE;
}

BOOL CGameSpyClientMgr::CreateServerList()
{
	// There is no master to create a list against; the (always empty) list
	// needs no state of its own.
	m_pServerList = NULL;
	return(TRUE);
}

void CGameSpyClientMgr::FreeServerList()
{
	m_pServerList = NULL;
}

int CGameSpyClientMgr::GetNumServers()
{
	return(0);
}

int CGameSpyClientMgr::GetState()
{
	return(sl_idle);
}

void CGameSpyClientMgr::ClearServers()
{
	m_nGetServerIndex = 0;
	m_bNewServer      = FALSE;
}

BOOL CGameSpyClientMgr::RefreshServers(BOOL bAsync)
{
	if (!m_bInit) return(FALSE);

	ClearServers();
	m_nProgress = 100;
	return(TRUE);
}

CGameSpyServer* CGameSpyClientMgr::GetServer(int nIndex)
{
	return(NULL);
}

CGameSpyServer* CGameSpyClientMgr::GetServerFromHandle(void* pHandle)
{
	return(NULL);
}

CGameSpyServer* CGameSpyClientMgr::GetFirstServer()
{
	m_nGetServerIndex = 0;
	return(GetServer(m_nGetServerIndex));
}

CGameSpyServer* CGameSpyClientMgr::GetNextServer()
{
	m_nGetServerIndex++;
	return(GetServer(m_nGetServerIndex));
}

BOOL CGameSpyClientMgr::ExistNewServer()
{
	BOOL bNew = m_bNewServer;
	m_bNewServer = FALSE;
	return(bNew);
}

void CGameSpyClientMgr::SortServersByName(BOOL bAscending)     { SortServers(bAscending, "hostname", cm_stricase); }
void CGameSpyClientMgr::SortServersByPing(BOOL bAscending)     { SortServers(bAscending, "ping", cm_int); }
void CGameSpyClientMgr::SortServersByPlayers(BOOL bAscending)  { SortServers(bAscending, "numplayers", cm_int); }
void CGameSpyClientMgr::SortServersByMap(BOOL bAscending)      { SortServers(bAscending, "mapname", cm_stricase); }
void CGameSpyClientMgr::SortServersByGameType(BOOL bAscending) { SortServers(bAscending, "gametype", cm_stricase); }

void CGameSpyClientMgr::SortServers(BOOL bAscending, char* sSortKey, GCompareMode eCompareMode)
{
	// an empty list is sorted
}

void CGameSpyClientMgr::Update()
{
}

void CGameSpyClientMgr::HandleCallback(GServerList pServerList, int nMsg, void* pParam1, void* pParam2)
{
	if (nMsg == LIST_PROGRESS)
	{
		m_nProgress  = (int)pParam2;
		m_bNewServer = TRUE;
	}
}

CGameSpyServer* CGameSpyClientMgr::CreateGameSpyServer(GServer pGServer)
{
	if (!pGServer) return(NULL);
	if (!m_CurServer.Init(pGServer)) return(NULL);
	return(&m_CurServer);
}
#endif


// ----------------------------------------------------------------------- //
//  CGameSpyServer -- one browser row. Never handed a server (see above).
// ----------------------------------------------------------------------- //

BOOL CGameSpyServer::Init(GServer pServer)
{
	Term();
	if (!pServer) return(FALSE);
	m_pGServer = pServer;
	m_bInit    = TRUE;
	return(TRUE);
}

void CGameSpyServer::Term()
{
	for (int i = 0; i < m_nNumPlayers; i++) m_aPlayers[i].Term();
	Clear();
}

void CGameSpyServer::Clear()
{
	m_pGServer     = NULL;
	m_bInit        = FALSE;
	m_nNumPlayers  = 0;
	m_nPlayerIndex = 0;
}

char* CGameSpyServer::GetName()       { return(GetStringValue("hostname")); }
char* CGameSpyServer::GetGameType()   { return(GetStringValue("gametype")); }
char* CGameSpyServer::GetMap()        { return(GetStringValue("mapname")); }
int   CGameSpyServer::GetPing()       { return(GetIntValue("ping", 9999)); }
int   CGameSpyServer::GetNumPlayers() { return(GetIntValue("numplayers")); }
int   CGameSpyServer::GetMaxPlayers() { return(GetIntValue("maxplayers")); }
char* CGameSpyServer::GetAddress()    { return(GetStringValue("hostaddr")); }

#ifdef AVP2_MODERN
// M-P6-02: the server's answer to \status\ (and "hostaddr", "ping")
char* CGameSpyServer::GetStringValue(char* sKey, char* sDefault)
{
	char* sValue = GSL_GetValue(m_pGServer, sKey);
	return(sValue ? sValue : sDefault);
}

int CGameSpyServer::GetIntValue(char* sKey, int nDefault)
{
	char* sValue = GSL_GetValue(m_pGServer, sKey);
	return(sValue ? atoi(sValue) : nDefault);
}

float CGameSpyServer::GetFloatValue(char* sKey, float fDefault)
{
	char* sValue = GSL_GetValue(m_pGServer, sKey);
	return(sValue ? (float)atof(sValue) : fDefault);
}
#else
char* CGameSpyServer::GetStringValue(char* sKey, char* sDefault)
{
	return(sDefault);
}

int CGameSpyServer::GetIntValue(char* sKey, int nDefault)
{
	return(nDefault);
}

float CGameSpyServer::GetFloatValue(char* sKey, float fDefault)
{
	return(fDefault);
}
#endif

int CGameSpyServer::UpdatePlayers()
{
	for (int i = 0; i < m_nNumPlayers; i++) m_aPlayers[i].Term();
	m_nNumPlayers  = 0;
	m_nPlayerIndex = 0;

	if (!m_bInit) return(0);

	int nPlayers = GetNumPlayers();
	if (nPlayers > GSS_MAX_PLAYERS) nPlayers = GSS_MAX_PLAYERS;

	for (int j = 0; j < nPlayers; j++)
	{
		if (m_aPlayers[m_nNumPlayers].Init(m_pGServer, j)) m_nNumPlayers++;
	}

	return(m_nNumPlayers);
}

CGameSpyPlayer* CGameSpyServer::GetPlayer(int nIndex)
{
	if (nIndex < 0 || nIndex >= m_nNumPlayers) return(NULL);
	return(&m_aPlayers[nIndex]);
}

CGameSpyPlayer* CGameSpyServer::GetFirstPlayer()
{
	m_nPlayerIndex = 0;
	return(GetPlayer(m_nPlayerIndex));
}

CGameSpyPlayer* CGameSpyServer::GetNextPlayer()
{
	m_nPlayerIndex++;
	return(GetPlayer(m_nPlayerIndex));
}

void CGameSpyServer::SortPlayers(BOOL bAscending, int nKey)
{
	// insertion sort by the requested key
	for (int i = 1; i < m_nNumPlayers; i++)
	{
		for (int j = i; j > 0; j--)
		{
			CGameSpyPlayer* a = &m_aPlayers[j - 1];
			CGameSpyPlayer* b = &m_aPlayers[j];
			int nCmp;
			switch (nKey)
			{
				case GSS_SORT_PING:  nCmp = a->GetPing() - b->GetPing(); break;
				case GSS_SORT_FRAGS: nCmp = a->GetFrags() - b->GetFrags(); break;
				default:             nCmp = stricmp(a->GetName(), b->GetName()); break;
			}
			if (!bAscending) nCmp = -nCmp;
			if (nCmp <= 0) break;

			char tmp[sizeof(CGameSpyPlayer)];
			memcpy(tmp, a, sizeof(tmp));
			memcpy(a, b, sizeof(tmp));
			memcpy(b, tmp, sizeof(tmp));
		}
	}
}


// ----------------------------------------------------------------------- //
//  CGameSpyPlayer
// ----------------------------------------------------------------------- //

BOOL CGameSpyPlayer::Init(GServer pGServer, int nIndex)
{
	Term();
	if (!pGServer) return(FALSE);
	m_pGServer = pGServer;
	m_nIndex   = nIndex;
	m_bInit    = TRUE;
	return(TRUE);
}

void CGameSpyPlayer::Term()
{
	Clear();
}

void CGameSpyPlayer::Clear()
{
	m_pGServer = NULL;
	m_nIndex   = 0;
	m_bInit    = FALSE;
}

char* CGameSpyPlayer::GetName()  { return(GetStringValue("player")); }
int   CGameSpyPlayer::GetFrags() { return(GetIntValue("frags")); }
int   CGameSpyPlayer::GetPing()  { return(GetIntValue("ping")); }

#ifdef AVP2_MODERN
// M-P6-02: a player's keys are the server's "<key>_<index>"
static char* GSL_GetPlayerValue(GServer pServer, char* sKey, int nIndex)
{
	char sPlayerKey[GSL_MAX_KEY + 16];
	if (!sKey || strlen(sKey) > GSL_MAX_KEY) return(NULL);
	sprintf(sPlayerKey, "%s_%d", sKey, nIndex);
	return(GSL_GetValue(pServer, sPlayerKey));
}

char* CGameSpyPlayer::GetStringValue(char* sKey, char* sDefault)
{
	char* sValue = GSL_GetPlayerValue(m_pGServer, sKey, m_nIndex);
	return(sValue ? sValue : sDefault);
}

int CGameSpyPlayer::GetIntValue(char* sKey, int nDefault)
{
	char* sValue = GSL_GetPlayerValue(m_pGServer, sKey, m_nIndex);
	return(sValue ? atoi(sValue) : nDefault);
}

float CGameSpyPlayer::GetFloatValue(char* sKey, float fDefault)
{
	char* sValue = GSL_GetPlayerValue(m_pGServer, sKey, m_nIndex);
	return(sValue ? (float)atof(sValue) : fDefault);
}
#else
char* CGameSpyPlayer::GetStringValue(char* sKey, char* sDefault)
{
	return(sDefault);
}

int CGameSpyPlayer::GetIntValue(char* sKey, int nDefault)
{
	return(nDefault);
}

float CGameSpyPlayer::GetFloatValue(char* sKey, float fDefault)
{
	return(fDefault);
}
#endif
