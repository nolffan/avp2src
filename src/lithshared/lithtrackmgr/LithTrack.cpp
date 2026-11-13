//*********************************************************************************
// LithTrack.cpp
//
// LithTrackMgr, as AvP2's client links it (LithTrackMgrClient.lib), written
// from the release's prebuilt build of it.
//*********************************************************************************

#include "lithtrackmgr.h"
#include "lithtrack.h"
#include <string.h>

//*********************************************************************************

LithTrack::LithTrack()
{
	Init();
}

//---------------------------------------------------------------------------------

LithTrack::~LithTrack()
{
	Term();
}

//---------------------------------------------------------------------------------

LTBOOL LithTrack::Init(char *szName)
{
	Name(szName);

	memset(m_pList, 0, sizeof(m_pList));
	m_nNumKeys = 0;

	// Every track starts out with two keys
	m_pList[0] = new LithTrackKey;
	if(!m_pList[0]) return LTFALSE;

	m_pList[1] = new LithTrackKey;
	if(!m_pList[1])
	{
		delete m_pList[0];
		return LTFALSE;
	}

	m_pList[1]->m_vPos.z += TRACKMGR_DEFAULT_NEW_KEY_POS_OFFSET;
	m_pList[1]->m_fTime = TRACKMGR_DEFAULT_NEW_KEY_TIME_OFFSET;

	m_nNumKeys = 2;

	return LTTRUE;
}

//---------------------------------------------------------------------------------

LTBOOL LithTrack::Load(FILE *file)
{
	return LTFALSE;
}

//---------------------------------------------------------------------------------

void LithTrack::Term()
{
	for(int i = 0; i < TRACKMGR_MAX_TRACK_KEYS; i++)
	{
		if(m_pList[i])
		{
			delete m_pList[i];
			m_pList[i] = LTNULL;
		}
	}

	m_nNumKeys = 0;
}

//---------------------------------------------------------------------------------

void LithTrack::Name(char *szName)
{
	if(!szName)
	{
		memset(m_szName, 0, TRACK_NAME_SIZE);
		return;
	}

	int nLength = strlen(szName);
	strncpy(m_szName, szName, (nLength + 1 < TRACK_NAME_SIZE) ? nLength + 1 : TRACK_NAME_SIZE);
}

//---------------------------------------------------------------------------------

LTBOOL LithTrack::Copy(LithTrack *pTrack)
{
	if(!pTrack) return LTFALSE;

	Term();

	for(int i = 0; i < pTrack->NumKeys(); i++)
	{
		m_pList[i] = new LithTrackKey;
		if(!m_pList[i]) return LTFALSE;

		m_pList[i]->Init();
		m_pList[i]->Copy(pTrack->Key(i));
	}

	m_nNumKeys = pTrack->NumKeys();

	return LTTRUE;
}

//---------------------------------------------------------------------------------

LTBOOL LithTrack::InsertKey(int nKey)
{
	if((nKey < 0) || (nKey >= m_nNumKeys) || (m_nNumKeys == TRACKMGR_MAX_TRACK_KEYS))
	{
		if(g_pTrackMgr) g_pTrackMgr->Send_To_Console(ERROR_NEW_KEY);
		return LTFALSE;
	}

	// Move the keys after the new one up a slot
	for(int i = m_nNumKeys; i > nKey + 1; i--)
	{
		m_pList[i] = m_pList[i - 1];
		m_pList[i - 1] = LTNULL;
	}

	m_pList[nKey + 1] = new LithTrackKey;
	m_pList[nKey + 1]->Init();
	m_nNumKeys++;

	if(nKey + 1 == m_nNumKeys - 1)
	{
		// The new key is the last one, so put it out in front of the previous key
		LTRotation rRot;
		LTVector vR, vU, vF;
		ILTMath *pMath = g_pTrackMgr->Interface()->GetMathLT();
		LTVector vRot = m_pList[nKey]->m_vRot;

		pMath->SetupEuler(rRot, MATH_DEGREES_TO_RADIANS(vRot.x), MATH_DEGREES_TO_RADIANS(vRot.y), MATH_DEGREES_TO_RADIANS(vRot.z));
		pMath->GetRotationVectors(rRot, vR, vU, vF);

		vF *= TRACKMGR_DEFAULT_NEW_KEY_POS_OFFSET;

		m_pList[nKey + 1]->m_vPos = m_pList[nKey]->m_vPos + vF;
		m_pList[nKey + 1]->m_fTime = m_pList[nKey]->m_fTime + TRACKMGR_DEFAULT_NEW_KEY_TIME_OFFSET;
	}
	else
	{
		// Put the new key half way between its neighbours
		m_pList[nKey + 1]->m_vPos = (m_pList[nKey]->m_vPos + m_pList[nKey + 2]->m_vPos) * 0.5f;
		m_pList[nKey + 1]->m_fTime = (m_pList[nKey]->m_fTime + m_pList[nKey + 2]->m_fTime) * 0.5f;
	}

	return LTTRUE;
}

//---------------------------------------------------------------------------------

LTBOOL LithTrack::DeleteKey(int nKey)
{
	if((nKey > 0) && (nKey < TRACKMGR_MAX_TRACK_KEYS) && m_pList[nKey] && (m_nNumKeys > 2))
	{
		delete m_pList[nKey];
		m_pList[nKey] = LTNULL;
		m_nNumKeys--;

		// RETAIL BUG: the keys after the deleted one are moved down a slot, but
		// the last slot is never cleared, so it and the one below it hold the
		// same key (Term deletes it twice).
#ifdef AVP2SRC_MODERN
		// M-B42 (RETAIL_BUGS.md): and the last slot moved from is cleared
		int i;
		for(i = nKey; (i + 1 < TRACKMGR_MAX_TRACK_KEYS) && m_pList[i + 1]; i++)
			m_pList[i] = m_pList[i + 1];
		m_pList[i] = LTNULL;
#else
		for(int i = nKey; (i + 1 < TRACKMGR_MAX_TRACK_KEYS) && m_pList[i + 1]; i++)
			m_pList[i] = m_pList[i + 1];
#endif

		return LTTRUE;
	}

	if(g_pTrackMgr) g_pTrackMgr->Send_To_Console(ERROR_DELETE_KEY);
	return LTFALSE;
}

//---------------------------------------------------------------------------------

LithTrackKey* LithTrack::Key(int nKey)
{
	if((nKey >= 0) && (nKey < TRACKMGR_MAX_TRACK_KEYS))
	{
		if(g_pTrackMgr && !m_pList[nKey]) g_pTrackMgr->Send_To_Console(ERROR_KEY_NULL);
		return m_pList[nKey];
	}

	if(g_pTrackMgr) g_pTrackMgr->Send_To_Console(ERROR_KEY_OUT_OF_RANGE);
	return LTNULL;
}
