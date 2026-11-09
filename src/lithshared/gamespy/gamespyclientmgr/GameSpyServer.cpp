/****************************************************************************
;
;	 MODULE:		GAMESPYSERVER (.CPP)
;
;	PURPOSE:		Game Spy Server Encapsulation Class
;
;	COMMENT:		GameSpyClientMgr.lib, written from the release's
;					prebuilt build of it.
;
****************************************************************************/


// Includes...

#include <windows.h>
#include "gamespyserver.h"


// Functions...

BOOL CGameSpyServer::Init(GServer pGServer)
{
	// Sanity checks...

	if (!pGServer) return(FALSE);


	// Set simple members...

	m_pGServer = pGServer;


	// All done...

	m_bInit = TRUE;
	return(TRUE);
}

void CGameSpyServer::Term()
{
	// Term the players...

	for (int i = 0; i < GSS_MAX_PLAYERS; i++)
	{
		m_aPlayers[i].Term();
	}


	// Clear our members...

	Clear();
}

void CGameSpyServer::Clear()
{
	m_pGServer     = NULL;
	m_bInit        = FALSE;
	m_nPlayerIndex = 0;
	m_nNumPlayers  = 0;
}

char* CGameSpyServer::GetName()
{
	if (!IsInitialized()) return("");

	return(GetStringValue("hostname", ""));
}

char* CGameSpyServer::GetGameType()
{
	if (!IsInitialized()) return("");

	return(GetStringValue("gametype", ""));
}

BOOL CGameSpyServer::GetDedicated()
{
	if (!IsInitialized()) return(FALSE);

	return(GetIntValue("ded", 0));
}

int CGameSpyServer::GetPing()
{
	if (!IsInitialized()) return(9999);

	return(ServerGetPing(m_pGServer));
}

int CGameSpyServer::GetNumPlayers()
{
	if (!IsInitialized()) return(0);

	return(GetIntValue("numplayers", 0));
}

int CGameSpyServer::GetMaxPlayers()
{
	if (!IsInitialized()) return(0);

	return(GetIntValue("maxplayers", 0));
}

char* CGameSpyServer::GetMap()
{
	if (!IsInitialized()) return("");

	return(GetStringValue("mapname", ""));
}

char* CGameSpyServer::GetAddress()
{
	if (!IsInitialized()) return("");

	return(ServerGetAddress(m_pGServer));
}

char* CGameSpyServer::GetStringValue(char* sKey, char* sDefault)
{
	if (!IsInitialized()) return(sDefault);

	return(ServerGetStringValue(m_pGServer, sKey, sDefault));
}

int CGameSpyServer::GetIntValue(char* sKey, int nDefault)
{
	if (!IsInitialized()) return(nDefault);

	return(ServerGetIntValue(m_pGServer, sKey, nDefault));
}

float CGameSpyServer::GetFloatValue(char* sKey, float fDefault)
{
	if (!IsInitialized()) return(fDefault);

	return((float)ServerGetFloatValue(m_pGServer, sKey, fDefault));
}

CGameSpyPlayer* CGameSpyServer::GetPlayer(int nIndex)
{
	if (nIndex >= m_nNumPlayers) return(NULL);
	if (nIndex >= GSS_MAX_PLAYERS) return(NULL);
	if (!m_aPlayers[nIndex].IsInitialized()) return(NULL);

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

int CGameSpyServer::UpdatePlayers()
{
	// Sanity checks...

	if (!m_pGServer) return(0);


	// Term the current players...

	for (int i = 0; i < GSS_MAX_PLAYERS; i++)
	{
		m_aPlayers[i].Term();
	}


	// Get the number of players...

	int nPlayers = GetNumPlayers();
	if (nPlayers > GSS_MAX_PLAYERS) nPlayers = GSS_MAX_PLAYERS;


	// Init each player...

	for (i = 0; i < nPlayers; i++)
	{
		m_aPlayers[i].Init(m_pGServer, i);
	}

	m_nNumPlayers = nPlayers;


	// All done...

	return(nPlayers);
}

void CGameSpyServer::SortPlayers(BOOL bAscending, int nKey)
{
	// Sanity checks...

	if (m_nNumPlayers <= 1) return;


	// Bubble sort the players...

	for (int i = 0; i < m_nNumPlayers - 1; i++)
	{
		CGameSpyPlayer* pPlayer1 = GetPlayer(i);

		for (int j = i + 1; j < m_nNumPlayers; j++)
		{
			CGameSpyPlayer* pPlayer2 = GetPlayer(j);
			BOOL bSwap = FALSE;

			switch (nKey)
			{
				case GSS_SORT_NAME:
				{
					if (bAscending)
					{
						if (stricmp(pPlayer1->GetName(), pPlayer2->GetName()) > 0) bSwap = TRUE;
						else bSwap = FALSE;
					}
					else
					{
						if (stricmp(pPlayer1->GetName(), pPlayer2->GetName()) < 0) bSwap = TRUE;
					}
					break;
				}

				case GSS_SORT_PING:
				{
					if (bAscending)
					{
						if (pPlayer1->GetPing() > pPlayer2->GetPing()) bSwap = TRUE;
						else bSwap = FALSE;
					}
					else
					{
						if (pPlayer1->GetPing() < pPlayer2->GetPing()) bSwap = TRUE;
					}
					break;
				}

				case GSS_SORT_FRAGS:
				{
					if (bAscending)
					{
						if (pPlayer1->GetFrags() > pPlayer2->GetFrags()) bSwap = TRUE;
						else bSwap = FALSE;
					}
					else
					{
						if (pPlayer1->GetFrags() < pPlayer2->GetFrags()) bSwap = TRUE;
					}
					break;
				}
			}

			if (bSwap)
			{
				int nIndex1 = (int)pPlayer1->GetHandle();
				int nIndex2 = (int)pPlayer2->GetHandle();

				pPlayer1->Term();
				pPlayer1->Init(m_pGServer, nIndex2);

				pPlayer2->Term();
				pPlayer2->Init(m_pGServer, nIndex1);
			}
		}
	}
}
