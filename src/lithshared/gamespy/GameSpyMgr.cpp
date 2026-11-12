// *********************************************************************** //
//
//  MODULE:   GAMESPYMGR (.CPP)  -- our replacement for GameSpyMgr.lib
//
//  PURPOSE:  The server-side GameSpy query responder Object.lto links
//            (CGameSpyMgr, src/LT2/lithshared/incs/GameSpyMgr.h; the game
//            derives CMyGameSpyMgr from it).
//
//  WHY THIS IS NOT A RECONSTRUCTION.  The library the source release
//  ships prebuilt is GameSpy middleware, not Monolith code, and it is
//  deliberately not byte-matched. The
//  build compiles every binary from our own source, so this file stands
//  in for it: the same interface, written from the header alone.
//
//  WHAT IT DOES.  It answers GameSpy-protocol status queries that reach
//  the game port (GameServerShell::ProcessPacket hands every packet that
//  starts with a backslash to OnQuery): \basic\ \info\ \rules\ \players\
//  \status\ and \echo\, each answered as `\key\value...` pairs closed by
//  `\queryid\<id>.<packet>` and, on the last packet, `\final\`, sent
//  through the game's CGameSpySender. A LAN query tool still sees the
//  server. What it does NOT do is talk to master.gamespy.com: that
//  service was shut down in 2013, so there is no heartbeat, and \secure\
//  (the master's challenge) gets no answer.
//
// *********************************************************************** //

#include <windows.h>
#include <stdio.h>
#include <string.h>
#include "GameSpyMgr.h"
#ifdef AVP2SRC_MODERN
//  THE MODERN BUILD (AVP2SRC_MODERN, M-P6-02) does report to a master: a
//  heartbeat every minute to the address the game sets with
//  SetGameSpyAddr (default master.openspy.net), and the answer to its
//  \secure\ challenge; see gsmaster.h.
#include "gsmaster.h"
#endif

#define GSM_PACKET_LIMIT	1300	// flush before a UDP datagram grows past this


#ifdef AVP2SRC_MODERN
// M-P6-02: the master's address, resolved at the first heartbeat and again
// every 30 minutes (or after a failure).
static char          s_sMasterAddr[128];
static unsigned long s_nMasterIP      = 0;
static int           s_nMasterPort    = 0;
static DWORD         s_dwResolveTime  = 0;
static BOOL          s_bWinsock       = FALSE;

#define GSM_RESOLVE_INTERVAL	(30 * 60 * 1000)

static BOOL GSM_GetMaster(const char* sAddr, char* sIP, int* pPort)
{
	DWORD dwNow = GetTickCount();

	if (stricmp(sAddr, s_sMasterAddr) != 0 || !s_nMasterIP ||
	    (dwNow - s_dwResolveTime) > GSM_RESOLVE_INTERVAL)
	{
		strncpy(s_sMasterAddr, sAddr, sizeof(s_sMasterAddr) - 1);
		s_sMasterAddr[sizeof(s_sMasterAddr) - 1] = '\0';
		s_dwResolveTime = dwNow;
		if (!GSM_Resolve(sAddr, GSM_HEARTBEAT_PORT, &s_nMasterIP, &s_nMasterPort))
		{
			s_nMasterIP = 0;
			return(FALSE);
		}
	}

	struct in_addr in;
	in.s_addr = s_nMasterIP;
	strcpy(sIP, inet_ntoa(in));
	*pPort = s_nMasterPort;
	return(TRUE);
}
#endif
BOOL CGameSpyMgr::Init(const char* sGameName, const char* sGameVer, const char* sGameKey,
                       int nGameLoc, int nGamePort, int nSpyPort, DWORD dwFlags)
{
	Term();
#ifdef AVP2SRC_MODERN
	// M-P6-02: name lookups need Winsock (the engine has it running
	// already; this only takes a reference)
	if (!s_bWinsock)
	{
		WSADATA wsaData;
		s_bWinsock = (WSAStartup(MAKEWORD(1, 1), &wsaData) == 0);
	}
#endif

	if (!sGameName || !sGameName[0]) return(FALSE);

	m_sGameName = sGameName;
	m_sGameVer  = sGameVer ? sGameVer : "";
	m_sGameKey  = sGameKey ? sGameKey : "";
	m_sGameLoc.Format("%i", nGameLoc);

	m_dwGamePort = (DWORD)nGamePort;
	m_dwSpyPort  = (DWORD)nSpyPort;
	m_dwFlags    = dwFlags;
	m_bInit      = TRUE;

	return(TRUE);
}

void CGameSpyMgr::Term()
{
#ifdef AVP2SRC_MODERN
	// M-P6-02: tell the master we are gone (statechanged 2), so the server
	// leaves the list at once rather than when it expires
	if (m_bInit && m_pSender && m_timerHeartbeat)
	{
		char sIP[32];
		int  nPort;
		if (GSM_GetMaster(m_sGameSpyAddr, sIP, &nPort))
		{
			char sBeat[256];
			sprintf(sBeat, "\\heartbeat\\%d\\gamename\\%.64s\\statechanged\\2",
			        (int)m_dwGamePort, (const char*)m_sGameName);
			SendSocketString(sIP, (DWORD)nPort, sBeat);
		}
	}
	m_timerHeartbeat = 0;
#endif
	m_bInit = FALSE;
}

void CGameSpyMgr::Update()
{
	if (!m_bInit) return;
	UpdateHeartbeat();
}

BOOL CGameSpyMgr::UpdateHeartbeat()
{
#ifdef AVP2SRC_MODERN
	// M-P6-02: a heartbeat every TIMER_HEARTBEAT, sent from the game port
	// (the port the master is told to query, GSMF_USEGAMEPORTFORHEARTBEAT)
	if (!m_pSender) return(FALSE);

	DWORD dwNow = GetTickCount();
	if (m_timerHeartbeat && (dwNow - m_timerHeartbeat) < TIMER_HEARTBEAT) return(FALSE);
	m_timerHeartbeat = dwNow ? dwNow : 1;

	char sIP[32];
	int  nPort;
	if (!GSM_GetMaster(m_sGameSpyAddr, sIP, &nPort)) return(FALSE);

	char sBeat[256];
	sprintf(sBeat, "\\heartbeat\\%d\\gamename\\%.64s",
	        (int)m_dwGamePort, (const char*)m_sGameName);
	return(SendSocketString(sIP, (DWORD)nPort, sBeat));
#else
	// The master server this used to report to no longer exists.
	return(FALSE);
#endif
}


// ----------------------------------------------------------------------- //
//  Queries
// ----------------------------------------------------------------------- //

BOOL CGameSpyMgr::OnQuery(CGameSpyQueryInfo* pQueryInfo)
{
	if (!pQueryInfo || !pQueryInfo->IsValid()) return(FALSE);

	CString sData = pQueryInfo->GetData();
	CString sAddr = pQueryInfo->GetAddr();

	return(OnQuery((char*)(LPCTSTR)sAddr, pQueryInfo->GetPort(),
	               (char*)(LPCTSTR)sData, sData.GetLength()));
}

BOOL CGameSpyMgr::OnQuery(char* sAddr, DWORD dwPort, char* sData, int nLen)
{
	if (!m_bInit || !sAddr || !sData || nLen <= 0) return(FALSE);

	// The packet is not NUL-terminated on the wire.
	char sQuery[MAX_RESPONSE];
	if (nLen >= (int)sizeof(sQuery)) nLen = sizeof(sQuery) - 1;
	memcpy(sQuery, sData, nLen);
	sQuery[nLen] = '\0';

	StartResponse(sAddr, dwPort);

	BOOL bAnswered = FALSE;
#ifdef AVP2SRC_MODERN
	// M-P6-02: the master reads \validate\ as the first key of the reply,
	// so a challenge is answered before anything else asked with it
	char sChallenge[80];
	if (GSM_FindKey(sQuery, "secure", sChallenge, sizeof(sChallenge)))
	{
		bAnswered |= OnSecureQuery(sChallenge);
	}
#endif
	CString sToken = GetFirstQueryToken(sQuery);

	while (!sToken.IsEmpty())
	{
		if (sToken.CompareNoCase("basic") == 0)
		{
			bAnswered |= OnBasicQuery();
		}
		else if (sToken.CompareNoCase("info") == 0)
		{
			bAnswered |= OnInfoQuery();
		}
		else if (sToken.CompareNoCase("rules") == 0)
		{
			bAnswered |= OnRulesQuery();
		}
		else if (sToken.CompareNoCase("players") == 0)
		{
			bAnswered |= OnPlayersQuery();
		}
		else if (sToken.CompareNoCase("status") == 0)
		{
			bAnswered |= OnStatusQuery();
		}
		else if (sToken.CompareNoCase("echo") == 0)
		{
			CString sValue = GetNextQueryToken();
			bAnswered |= OnEchoQuery(sValue);
		}
		else if (sToken.CompareNoCase("secure") == 0)
		{
			CString sValue = GetNextQueryToken();
#ifndef AVP2SRC_MODERN
			bAnswered |= OnSecureQuery(sValue);
#endif
		}

		sToken = GetNextQueryToken();
	}

	if (bAnswered) SendResponse();

	return(bAnswered);
}

BOOL CGameSpyMgr::OnEchoQuery(const char* sQuery)
{
	SendResponseInfo("echo", sQuery ? sQuery : "");
	return(TRUE);
}

BOOL CGameSpyMgr::OnSecureQuery(const char* sValue)
{
#ifdef AVP2SRC_MODERN
	// M-P6-02: prove to the master that we know the game's secret key
	if (!sValue || !sValue[0] || m_sGameKey.IsEmpty()) return(FALSE);

	char sResponse[96];
	GSM_Validate(sValue, m_sGameKey, sResponse);
	SendResponseInfo("validate", sResponse);
	return(TRUE);
#else
	// The master's validation challenge: nothing left to validate against.
	return(FALSE);
#endif
}

BOOL CGameSpyMgr::OnBasicQuery()
{
	SendResponseInfo("gamename", m_sGameName);
	SendResponseInfo("gamever", m_sGameVer);
	SendResponseInfo("location", m_sGameLoc);
	return(TRUE);
}

BOOL CGameSpyMgr::OnStatusQuery()
{
	OnBasicQuery();
	OnInfoQuery();
	OnRulesQuery();
	OnPlayersQuery();
	return(TRUE);
}


// ----------------------------------------------------------------------- //
//  Responses
// ----------------------------------------------------------------------- //

void CGameSpyMgr::StartResponse(const char* sAddr, DWORD dwPort)
{
	m_sResponseAddr  = sAddr;
	m_dwResponsePort = dwPort;
	m_sResponse[0]   = '\0';
	m_nPacket        = 0;
	m_dwQueryID++;
}

void CGameSpyMgr::SendResponseInfo(const char* sKey, const char* sValue)
{
	if (!sKey) return;
	if (!sValue) sValue = "";

	int nAdd = 2 + (int)strlen(sKey) + (int)strlen(sValue);
	if ((int)strlen(m_sResponse) + nAdd > GSM_PACKET_LIMIT) FlushResponse();
	if ((int)strlen(m_sResponse) + nAdd > MAX_RESPONSE) return;

	strcat(m_sResponse, "\\");
	strcat(m_sResponse, sKey);
	strcat(m_sResponse, "\\");
	strcat(m_sResponse, sValue);
}

void CGameSpyMgr::SendResponseInfo(const char* sKey, int nValue)
{
	char sValue[32];
	sprintf(sValue, "%i", nValue);
	SendResponseInfo(sKey, sValue);
}

void CGameSpyMgr::AddResponseQueryID()
{
	char sID[64];
	sprintf(sID, "\\queryid\\%i.%i", (int)m_dwQueryID, m_nPacket + 1);
	strcat(m_sResponse, sID);
}

// Sends what has accumulated as one packet and starts the next.
void CGameSpyMgr::FlushResponse()
{
	if (!m_sResponse[0]) return;
	AddResponseQueryID();
	SendSocketString(m_sResponseAddr, m_dwResponsePort, m_sResponse);
	m_sResponse[0] = '\0';
	m_nPacket++;
}

// Sends the last packet of a response.
void CGameSpyMgr::SendResponse()
{
	strcat(m_sResponse, "\\final\\");
	AddResponseQueryID();
	SendSocketString(m_sResponseAddr, m_dwResponsePort, m_sResponse);
	m_sResponse[0] = '\0';
	m_nPacket = 0;
}

BOOL CGameSpyMgr::SendSocketString(const char* sAddr, DWORD dwPort, const char* sText, int nLength)
{
	if (!m_pSender || !sAddr || !sText) return(FALSE);
	if (nLength < 0) nLength = (int)strlen(sText);
	m_pSender->SendTo(sText, (unsigned long)nLength, sAddr, dwPort);
	return(TRUE);
}


// ----------------------------------------------------------------------- //
//  Query tokens: "\basic\\info\" -> "basic", "info"; empty when done.
// ----------------------------------------------------------------------- //

static char s_sQueryBuf[MAX_RESPONSE];
static char* s_pQueryPos = NULL;

CString CGameSpyMgr::GetFirstQueryToken(const char* sQuery)
{
	strncpy(s_sQueryBuf, sQuery ? sQuery : "", sizeof(s_sQueryBuf) - 1);
	s_sQueryBuf[sizeof(s_sQueryBuf) - 1] = '\0';
	s_pQueryPos = s_sQueryBuf;
	return(GetNextQueryToken());
}

CString CGameSpyMgr::GetNextQueryToken()
{
	CString sToken;
	if (!s_pQueryPos) return(sToken);

	while (*s_pQueryPos == '\\') s_pQueryPos++;
	if (!*s_pQueryPos) { s_pQueryPos = NULL; return(sToken); }

	char* pStart = s_pQueryPos;
	while (*s_pQueryPos && *s_pQueryPos != '\\') s_pQueryPos++;

	char c = *s_pQueryPos;
	*s_pQueryPos = '\0';
	sToken = pStart;
	*s_pQueryPos = c;

	return(sToken);
}
