/****************************************************************************
;
;	 MODULE:		GAMESPYPLAYER (.CPP)
;
;	PURPOSE:		Game Spy Player Encapsulation Class
;
;	COMMENT:		GameSpyClientMgr.lib, written from the release's
;					prebuilt build of it.
;
****************************************************************************/


// Includes...

#include <windows.h>
#include "gamespyplayer.h"


// Functions...

BOOL CGameSpyPlayer::Init(GServer pGServer, int nIndex)
{
	// Sanity checks...

	if (!pGServer) return(FALSE);


	// Set simple members...

	m_pGServer = pGServer;
	m_nIndex   = nIndex;


	// All done...

	m_bInit = TRUE;
	return(TRUE);
}

void CGameSpyPlayer::Term()
{
	Clear();
}

void CGameSpyPlayer::Clear()
{
	m_pGServer = NULL;
	m_bInit    = FALSE;
	m_nIndex   = 0;
}

char* CGameSpyPlayer::GetName()
{
	if (!IsInitialized()) return("");

	return(GetStringValue("player", ""));
}

int CGameSpyPlayer::GetRace()
{
	if (!IsInitialized()) return(0);

	return(GetIntValue("race", 0));
}

int CGameSpyPlayer::GetFrags()
{
	if (!IsInitialized()) return(0);

	return(GetIntValue("frags", 0));
}

int CGameSpyPlayer::GetScore()
{
	if (!IsInitialized()) return(0);

	return(GetIntValue("score", 0));
}

int CGameSpyPlayer::GetPing()
{
	if (!IsInitialized()) return(9999);

	return(GetIntValue("ping", 0));
}

char* CGameSpyPlayer::GetStringValue(char* sKey, char* sDefault)
{
	if (!IsInitialized()) return(sDefault);

	return(ServerGetPlayerStringValue(m_pGServer, m_nIndex, sKey, sDefault));
}

int CGameSpyPlayer::GetIntValue(char* sKey, int nDefault)
{
	if (!IsInitialized()) return(nDefault);

	return(ServerGetPlayerIntValue(m_pGServer, m_nIndex, sKey, nDefault));
}

float CGameSpyPlayer::GetFloatValue(char* sKey, float fDefault)
{
	if (!IsInitialized()) return(fDefault);

	return((float)ServerGetPlayerFloatValue(m_pGServer, m_nIndex, sKey, fDefault));
}
