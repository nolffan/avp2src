//*********************************************************************************
// LithTrackList.cpp
//
// LithTrackMgr, as AvP2's client links it (LithTrackMgrClient.lib), written
// from the release's prebuilt build of it.
//*********************************************************************************

#include "lithtrackmgr.h"
#include "lithtracklist.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//*********************************************************************************

LithTrackList::LithTrackList()
{
	Init(DNULL);
}

//---------------------------------------------------------------------------------

LithTrackList::~LithTrackList()
{
	Term();
}

//---------------------------------------------------------------------------------

DBOOL LithTrackList::Init(char *szFile)
{
	memset(m_pList, 0, sizeof(m_pList));
	m_nNumTracks = 0;

	if(szFile)
	{
		FILE *file = fopen(szFile, "rb");

		if(file)
		{
			if(!LoadTracks(file))
				fclose(file);
		}
	}

	return DTRUE;
}

//---------------------------------------------------------------------------------

void LithTrackList::Term()
{
	for(int i = 0; i < TRACKMGR_MAX_TRACKS; i++)
	{
		if(m_pList[i])
		{
			delete m_pList[i];
			m_pList[i] = DNULL;
		}
	}

	m_nNumTracks = 0;
}

//---------------------------------------------------------------------------------

DBOOL LithTrackList::New(char *szName)
{
	int nTrack = m_nNumTracks;

	if(nTrack < TRACKMGR_MAX_TRACKS)
	{
		m_pList[nTrack] = new LithTrack;

		if(m_pList[nTrack])
		{
			char szNewName[TRACK_NAME_BUFFER_SIZE];
			CreateNewName(szNewName, szName ? szName : TRACKMGR_TRACK_NAME);

			if(m_pList[nTrack]->Init(szNewName))
			{
				m_nNumTracks++;
				return DTRUE;
			}

			delete m_pList[nTrack];
			m_pList[nTrack] = DNULL;
		}
	}

	if(g_pTrackMgr) g_pTrackMgr->Send_To_Console(ERROR_NEW_TRACK);
	return DFALSE;
}

//---------------------------------------------------------------------------------

DBOOL LithTrackList::Copy(int nTrack, char *szName)
{
	if((nTrack >= 0) && (nTrack < TRACKMGR_MAX_TRACKS) && m_pList[nTrack])
	{
		if(szName && Track(szName))
		{
			if(g_pTrackMgr) g_pTrackMgr->Send_To_Console(ERROR_COPY_TRACK_2);
			return DFALSE;
		}

		if(!New(szName)) return DFALSE;

		if(!m_pList[m_nNumTracks - 1]->Copy(m_pList[nTrack]))
		{
			Delete(m_nNumTracks - 1);
			return DFALSE;
		}

		return DTRUE;
	}

	if(g_pTrackMgr) g_pTrackMgr->Send_To_Console(ERROR_COPY_TRACK_1);
	return DFALSE;
}

//---------------------------------------------------------------------------------

DBOOL LithTrackList::Copy(char *szName, char *szName2)
{
	int nTrack = -1;
	Track(szName, &nTrack);

	return Copy(nTrack, szName2);
}

//---------------------------------------------------------------------------------

DBOOL LithTrackList::Delete(int nTrack)
{
	if((nTrack >= 0) && (nTrack < TRACKMGR_MAX_TRACKS) && m_pList[nTrack])
	{
		delete m_pList[nTrack];
		m_pList[nTrack] = DNULL;
		m_nNumTracks--;

		// RETAIL BUG: the tracks after the deleted one are moved down a slot,
		// but the last slot is never cleared, so it and the one below it hold
		// the same track (Term deletes it twice).
		for(int i = nTrack; (i + 1 < TRACKMGR_MAX_TRACKS) && m_pList[i + 1]; i++)
			m_pList[i] = m_pList[i + 1];

		return DTRUE;
	}

	if(g_pTrackMgr) g_pTrackMgr->Send_To_Console(ERROR_DELETE_TRACK);
	return DFALSE;
}

//---------------------------------------------------------------------------------

DBOOL LithTrackList::Delete(char *szName)
{
	int nTrack = -1;
	Track(szName, &nTrack);

	return Delete(nTrack);
}

//---------------------------------------------------------------------------------

LithTrack* LithTrackList::Track(int nTrack)
{
	if((nTrack >= 0) && (nTrack < TRACKMGR_MAX_TRACKS))
	{
		if(g_pTrackMgr && !m_pList[nTrack]) g_pTrackMgr->Send_To_Console(ERROR_TRACK_NULL);
		return m_pList[nTrack];
	}

	if(g_pTrackMgr) g_pTrackMgr->Send_To_Console(ERROR_TRACK_OUT_OF_RANGE);
	return DNULL;
}

//---------------------------------------------------------------------------------

LithTrack* LithTrackList::Track(char *szName, int *nTrack)
{
	for(int i = 0; i < m_nNumTracks; i++)
	{
		if(m_pList[i] && !strnicmp(m_pList[i]->Name(), szName, TRACK_NAME_SIZE))
		{
			if(nTrack) *nTrack = i;
			return m_pList[i];
		}
	}

	if(g_pTrackMgr) g_pTrackMgr->Send_To_Console(ERROR_TRACK_NULL);
	return DNULL;
}

//---------------------------------------------------------------------------------

DBOOL LithTrackList::LoadTracks(FILE *file)
{
	return DFALSE;
}

//---------------------------------------------------------------------------------

DBOOL LithTrackList::CreateNewName(char *szNewName, const char *szName)
{
	char szBase[TRACK_NAME_BUFFER_SIZE];
	int nLength = strlen(szName);
	int i;
	if(nLength > TRACK_NAME_SIZE) nLength = TRACK_NAME_SIZE;

	strncpy(szBase, szName, nLength + 1);

	// Split any number off the end of the name
	int nDigits = 0;
	i = nLength;

	while(1)
	{
		if(i <= 0)
			break;

		if((szBase[i - 1] < '0') || (szBase[i - 1] > '9'))
			break;

		nDigits++;
		i--;
	}

	int nNum;

	if(nDigits)
	{
		nNum = atoi(&szBase[i]);
		szBase[i] = 0;
		sprintf(szNewName, "%s%d", szBase, nNum);
	}
	else
	{
		nNum = 0;
		sprintf(szNewName, "%s", szBase);
	}

	// Count up until the name isn't used
	while(1)
	{
		LithTrack *pTrack = DNULL;

		for(int j = 0; j < m_nNumTracks; j++)
		{
			if(m_pList[j] && !strnicmp(m_pList[j]->Name(), szNewName, TRACK_NAME_SIZE))
				pTrack = m_pList[j];
		}

		if(!pTrack) break;

		nNum++;
		sprintf(szNewName, "%s%d", szBase, nNum);
	}

	return DTRUE;
}
