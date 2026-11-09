//*********************************************************************************
//*********************************************************************************
// Project:		Alien vs. Predator 2
// Purpose:		Retrieves attributes from the SoundButes.txt
//*********************************************************************************
// File:		SoundButeMgr.cpp
// Created:		November 29, 2000
// Updated:		November 29, 2000
// Author:		Andy Kaplan
//*********************************************************************************
//*********************************************************************************

#include "stdafx.h"
#include "SoundButeMgr.h"

#ifdef PRIMAL_HUNT
#include "MultiFileImpl.h"
#endif

#ifdef _DEBUG
// #define SOUND_BUTE_DEBUG
#endif

//*********************************************************************************

#define SOUND_BUTES_TAG					"Sound"

#define SOUND_BUTE_PATH					"Path"
#define SOUND_BUTE_WEIGHT				"Weight"
#define SOUND_BUTE_INNER_RAD			"InnerRadius"
#define SOUND_BUTE_OUTER_RAD			"OuterRadius"
#define SOUND_BUTE_PITCH				"Pitch"
#define SOUND_BUTE_VOLUME				"Volume"
#define SOUND_BUTE_LOOP					"LoopSound"
#define SOUND_BUTE_LOCAL				"PlaysoundLocal"
#define SOUND_BUTE_CLIENTLOCAL			"PlayClientLocal"
#define SOUND_BUTE_GETHANDLE			"GetHandle"
#define SOUND_BUTE_TIME					"ServerTime"
#define SOUND_BUTE_SERVERSYNC			"ServerTimeSync"
#define SOUND_BUTE_REVERB				"AllowReverb"
#define SOUND_BUTE_ONCE					"PlayOnce"
#define SOUND_BUTE_PLAYCHANCE			"PlayChance"
#define SOUND_BUTE_PRIORITY				"Priority"
#define SOUND_BUTE_LIP_SYNC				"LipSync"
#define SOUND_BUTE_MIN_DELAY			"MinDelay"
#define SOUND_BUTE_MAX_DELAY			"MaxDelay"

//*********************************************************************************

CSoundButeMgr* g_pSoundButeMgr = LTNULL;

//*********************************************************************************

static char s_aTagName[30];

SoundFiles::~SoundFiles()
{
	for( SoundFileList::iterator iter = m_szSounds.begin(); iter != m_szSounds.end(); ++iter )
	{
		delete[] *iter;
	}

	m_szSounds.clear();
	m_fWeights.clear();

	m_nNumSounds = 0;
}

//*********************************************************************************
// Construction/Destruction
//*********************************************************************************

CSoundButeMgr::CSoundButeMgr()
{
	m_nNumSoundButes = 0;
	m_pSoundButes = LTNULL;
	m_pSoundFiles = LTNULL;
}

CSoundButeMgr::~CSoundButeMgr()
{
	Term();
}

//*********************************************************************************
//
//	ROUTINE:	CSoundButeMgr::Init()
//	PURPOSE:	Init Sound mgr
//
//*********************************************************************************
bool CSoundButeMgr::CountTags(const char *szTagName, void *pData)
{
	if(!szTagName || !pData) return true;

	CSoundButeMgr *pButeMgr = (CSoundButeMgr*)pData;
	++pButeMgr->m_nNumSoundButes;

	// Keep iterating.
	return true;
}

bool CSoundButeMgr::LoadButes(const char *szTagName, void *pData)
{
	if(!szTagName || !pData) return true;

	CSoundButeMgr *pButeMgr = (CSoundButeMgr*)pData;

	// Load the attributes.
	pButeMgr->LoadSoundButes( szTagName, 
		                      &pButeMgr->m_pSoundButes[pButeMgr->m_nNumSoundButes], 
							  &pButeMgr->m_pSoundFiles[pButeMgr->m_nNumSoundButes]  );

	// Store away the name-index pair for quick look-up.
	pButeMgr->m_IndexTable[pButeMgr->m_pSoundFiles[pButeMgr->m_nNumSoundButes].m_szName] = pButeMgr->m_nNumSoundButes;

	// Increment our counter.
	++pButeMgr->m_nNumSoundButes;

	// Keep iterating.
	return true;
}

LTBOOL CSoundButeMgr::Init(ILTCSBase *pInterface, const char* szAttributeFile)
{
	if (g_pSoundButeMgr || !szAttributeFile) return LTFALSE;

#ifdef PRIMAL_HUNT
	// PH: read the bute cache instead, if there is one (0x1002cc20)
	CMultiFile_LITH cacheFile;
	if (ReadCache(szAttributeFile, &cacheFile) == LTTRUE)
	{
		g_pSoundButeMgr = this;
		return LTTRUE;
	}
#endif

	if (!Parse(pInterface, szAttributeFile)) return LTFALSE;


	// Set up global pointer...
	g_pSoundButeMgr = this;

	// Calculate the number of pickup attribute sets
	m_nNumSoundButes = 0;
	m_buteMgr.GetTags(CountTags, this);

	// Create the sound attribute list
	m_pSoundButes = new SoundButes[m_nNumSoundButes];
	m_pSoundFiles = new SoundFiles[m_nNumSoundButes];
	ASSERT( m_pSoundButes && m_pSoundFiles);
	if( m_pSoundButes && m_pSoundFiles )
	{
		// Reset m_nNumSoundButes, so that it can be used to determine
		// the current bute being loaded.
		m_nNumSoundButes = 0;

		// Load them butes!
		m_buteMgr.GetTags(LoadButes, this);
	}

	// Free up butemgr's memory and what-not.

	m_buteMgr.Term();

#ifdef PRIMAL_HUNT
	// PH: and write the bute cache (0x1002cd87)
	CMultiFile_FILE cacheOut;
	WriteCache(szAttributeFile, &cacheOut);
#endif

	return LTTRUE;
}


//*********************************************************************************
//
//	ROUTINE:	CSoundButeMgr::Term()
//	PURPOSE:	Clean up.
//
//*********************************************************************************

void CSoundButeMgr::Term()
{
	g_pSoundButeMgr = LTNULL;

	delete[] m_pSoundButes;
	m_pSoundButes = LTNULL;

	delete[] m_pSoundFiles;
	m_pSoundFiles = LTNULL;
}

//*********************************************************************************

int CSoundButeMgr::GetSoundSetFromName(const char *szName) const
{
	// Look the name up in our index table.
	IndexTable::const_iterator iter = m_IndexTable.find(szName);

	// If we found it, return the index.
	if( iter != m_IndexTable.end() )
	{
		return iter->second;
	}

#ifdef SOUND_BUTE_DEBUG
	g_pInterface->CPrint("SoundButeMgr: Could not find set %s! Defaulting to index -1!", szName);
#endif

	// We didn't find it, return -1 to indicate failure.
	return -1;
}

//*********************************************************************************

const SoundButes & CSoundButeMgr::GetSoundButes(const char *szName) const
{
	int nSet = GetSoundSetFromName(szName);

	if( nSet >= 0 && nSet < m_nNumSoundButes )
		return m_pSoundButes[nSet];

	return m_pSoundButes[0];
}

//*********************************************************************************

const SoundFiles & CSoundButeMgr::GetSoundFiles(const char *szName) const
{
	int nSet = GetSoundSetFromName(szName);

	if( nSet >= 0 && nSet < m_nNumSoundButes )
		return m_pSoundFiles[nSet];

	return m_pSoundFiles[0];
}

//*********************************************************************************

const char* CSoundButeMgr::GetRandomSoundFile(uint32 nSet)
{
	CheckSound(nSet);
	
	if(m_pSoundFiles[nSet].m_nNumSounds == 0 )
		return LTNULL;

	ASSERT( m_pSoundFiles[nSet].m_nNumSounds == m_pSoundFiles[nSet].m_szSounds.size() );

	const int nSoundIndex = GetRandom(0,m_pSoundFiles[nSet].m_nNumSounds-1);
	return m_pSoundFiles[nSet].m_szSounds[nSoundIndex];
}

//*********************************************************************************

const char* CSoundButeMgr::GetRandomSoundFileWeighted(uint32 nSet)
{
	CheckSound(nSet);
	
	if( m_pSoundFiles[nSet].m_nNumSounds == 0 )
		return "";

	ASSERT( m_pSoundFiles[nSet].m_nNumSounds == m_pSoundFiles[nSet].m_szSounds.size() );

	// If there's only one sound... go ahead and use that one
	if(m_pSoundFiles[nSet].m_nNumSounds == 1)
	{
		return m_pSoundFiles[nSet].m_szSounds[0];
	}

	// Get a random weight to test with
	LTFLOAT fRand = GetRandom(0.0f, 1.0f);

	for(uint32 i = 0; i < m_pSoundFiles[nSet].m_nNumSounds; i++)
	{
		if(m_pSoundFiles[nSet].m_fWeights[i] >= fRand)
			return m_pSoundFiles[nSet].m_szSounds[i];
	}

	// Otherwise, if we got here... just return the last sound
	return m_pSoundFiles[nSet].m_szSounds[m_pSoundFiles[nSet].m_nNumSounds - 1];
}

//*********************************************************************************

void CSoundButeMgr::LoadSoundButes(const char * szTagName, SoundButes * pButes, SoundFiles * pFiles)
{
	ASSERT( pButes && pFiles );
	if( !pButes || !pFiles )
		return;

	//be sure to clear the flags
	pButes->m_nFlags = 0;


	SAFE_STRCPY(pFiles->m_szName, szTagName);

	// The parent needs to be the base parent.
	SAFE_STRCPY(pFiles->m_szParent, GetString(szTagName, "Parent") );
	while( !GetString(pFiles->m_szParent, "Parent").IsEmpty() )
	{
		SAFE_STRCPY(pFiles->m_szParent, GetString(pFiles->m_szParent, "Parent") );
	}



	// This will be used by weight and sound parameters
	char aAttribName[64];

	// Make a value to accumulate the weights
	LTFLOAT fWeightAccum = 0;


	// Get the sound path and filename
	int nNumSoundButes = 0;
	sprintf(aAttribName, "%s%d", SOUND_BUTE_PATH, nNumSoundButes);

	while(Exist(szTagName, aAttribName))
	{
		CString strSoundFileName = GetString(szTagName, aAttribName, CString(""));

		if( !strSoundFileName.IsEmpty() )
		{
			// Store the sound file name.
			pFiles->m_szSounds.push_back( new char[strSoundFileName.GetLength() + 1] );

			SAFE_STRCPY(pFiles->m_szSounds.back(), strSoundFileName);

			// Read the weight value
			sprintf(aAttribName, "%s%d", SOUND_BUTE_WEIGHT, nNumSoundButes);
			const LTFLOAT fWeight = (float)GetDouble(szTagName, aAttribName, 1.0f);
			pFiles->m_fWeights.push_back( fWeight );

			fWeightAccum += fWeight;
		}

		++nNumSoundButes;
		sprintf(aAttribName, "%s%d", SOUND_BUTE_PATH, nNumSoundButes);
	}

	pFiles->m_nNumSounds = pFiles->m_szSounds.size();

	ASSERT( pFiles->m_fWeights.size() == pFiles->m_nNumSounds );

	// The weight values are evened out into a range that's easy to work with (to speed up calculations later)
	LTFLOAT fCurrentWeightAccum = 0.0f;
	for( SoundFiles::WeightList::iterator weight_iter = pFiles->m_fWeights.begin(); 
	     weight_iter != pFiles->m_fWeights.end(); ++weight_iter )
	{
		fCurrentWeightAccum += (*weight_iter) / fWeightAccum;

		(*weight_iter) = fCurrentWeightAccum;
	}

	pButes->m_fInnerRad	= (LTFLOAT)GetDouble(szTagName, SOUND_BUTE_INNER_RAD);
	pButes->m_fOuterRad	= (LTFLOAT)GetDouble(szTagName, SOUND_BUTE_OUTER_RAD);
	pButes->m_fPlayChance = (LTFLOAT)GetDouble(szTagName, SOUND_BUTE_PLAYCHANCE);

	pButes->m_fPitch		= (LTFLOAT)GetDouble(szTagName, SOUND_BUTE_PITCH);
	if(pButes->m_fPitch != 1.0f) pButes->m_nFlags |= PLAYSOUND_CTRL_PITCH;

	pButes->m_nVolume		= (uint8)GetInt(szTagName, SOUND_BUTE_VOLUME);
	if(pButes->m_nVolume != 100) pButes->m_nFlags |= PLAYSOUND_CTRL_VOL;

	pButes->m_ePriority	= (SoundPriority)GetInt(szTagName, SOUND_BUTE_PRIORITY);

	LTBOOL bTest = (LTBOOL)GetInt(szTagName, SOUND_BUTE_LOCAL);
	pButes->m_nFlags |= bTest?PLAYSOUND_LOCAL:PLAYSOUND_3D;

	bTest = (LTBOOL)GetInt(szTagName, SOUND_BUTE_CLIENTLOCAL);
	if(bTest) pButes->m_nFlags |= PLAYSOUND_CLIENTLOCAL;

	// If we're looping, we better also have the handle.
	bTest = (LTBOOL)GetInt(szTagName, SOUND_BUTE_LOOP);
	if(bTest) pButes->m_nFlags |= PLAYSOUND_LOOP | PLAYSOUND_GETHANDLE;

	bTest = (LTBOOL)GetInt(szTagName, SOUND_BUTE_GETHANDLE);
	if(bTest) pButes->m_nFlags |= PLAYSOUND_GETHANDLE;

	bTest = (LTBOOL)GetInt(szTagName, SOUND_BUTE_TIME);
	if(bTest) pButes->m_nFlags |= PLAYSOUND_TIME;

	bTest = (LTBOOL)GetInt(szTagName, SOUND_BUTE_SERVERSYNC);
	if(bTest) pButes->m_nFlags |= PLAYSOUND_TIMESYNC;

	bTest = (LTBOOL)GetInt(szTagName, SOUND_BUTE_REVERB);
	if(bTest) pButes->m_nFlags |= PLAYSOUND_REVERB;

	bTest = (LTBOOL)GetInt(szTagName, SOUND_BUTE_ONCE);
	if(bTest) pButes->m_nFlags |= PLAYSOUND_ONCE;

	pButes->m_bLipSync	= (LTBOOL)GetInt(szTagName, SOUND_BUTE_LIP_SYNC);

	pButes->m_fMinDelay	= (LTFLOAT)GetDouble(szTagName, SOUND_BUTE_MIN_DELAY);
	pButes->m_fMaxDelay	= (LTFLOAT)GetDouble(szTagName, SOUND_BUTE_MAX_DELAY);
}

#ifdef PRIMAL_HUNT

//*********************************************************************************
//
//	ROUTINE:	SoundFiles::Write() / Read()
//	PURPOSE:	Primal Hunt: a sound set's files in the bute cache
//
//*********************************************************************************

LTBOOL SoundFiles::Write(CMultiFile* pFile)
{
	if (!pFile) return LTFALSE;

	pFile->WriteDWord(m_nNumSounds);
	pFile->WriteString(m_szName);
	pFile->WriteString(m_szParent);

	pFile->WriteInt(m_szSounds.size());
	for( SoundFileList::iterator iter = m_szSounds.begin(); iter != m_szSounds.end(); ++iter )
	{
		pFile->WriteString(*iter);
	}

	pFile->WriteInt(m_fWeights.size());
	for( WeightList::iterator weight_iter = m_fWeights.begin(); weight_iter != m_fWeights.end(); ++weight_iter )
	{
		pFile->WriteFloat(*weight_iter);
	}

	return LTTRUE;
}

LTBOOL SoundFiles::Read(CMultiFile* pFile)
{
	if (!pFile) return LTFALSE;

	pFile->ReadDWord(&m_nNumSounds);
	pFile->ReadString(m_szName, SOUND_BUTES_STRING_SIZE);
	pFile->ReadString(m_szParent, SOUND_BUTES_STRING_SIZE);

	int nNumSounds = pFile->ReadInt();
	for (int i=0; i < nNumSounds; i++)
	{
		char szSound[SOUND_BUTES_STRING_SIZE];
		pFile->ReadString(szSound, SOUND_BUTES_STRING_SIZE);

		m_szSounds.push_back( new char[strlen(szSound) + 1] );
		strcpy(m_szSounds.back(), szSound);
	}

	int nNumWeights = pFile->ReadInt();
	for (i=0; i < nNumWeights; i++)
	{
		m_fWeights.push_back( pFile->ReadFloat() );
	}

	return LTTRUE;
}

//*********************************************************************************
//
//	ROUTINE:	SoundButes::Write() / Read()
//	PURPOSE:	Primal Hunt: a sound set's attributes in the bute cache
//
//*********************************************************************************

LTBOOL SoundButes::Write(CMultiFile* pFile)
{
	if (!pFile) return LTFALSE;

	pFile->WriteFloat(m_fInnerRad);
	pFile->WriteFloat(m_fOuterRad);
	pFile->WriteFloat(m_fPitch);
	pFile->WriteFloat(m_fPlayChance);
	pFile->WriteByte(m_nVolume);
	pFile->WriteByte(m_ePriority);
	pFile->WriteDWord(m_nFlags);
	pFile->WriteBool(m_bLipSync);
	pFile->WriteFloat(m_fMinDelay);
	pFile->WriteFloat(m_fMaxDelay);

	return LTTRUE;
}

LTBOOL SoundButes::Read(CMultiFile* pFile)
{
	if (!pFile) return LTFALSE;

	pFile->ReadFloat(&m_fInnerRad);
	pFile->ReadFloat(&m_fOuterRad);
	pFile->ReadFloat(&m_fPitch);
	pFile->ReadFloat(&m_fPlayChance);
	pFile->ReadByte(&m_nVolume);
	m_ePriority = (SoundPriority)pFile->ReadByte();
	pFile->ReadDWord(&m_nFlags);
	pFile->ReadBool(&m_bLipSync);
	pFile->ReadFloat(&m_fMinDelay);
	pFile->ReadFloat(&m_fMaxDelay);

	return LTTRUE;
}

//*********************************************************************************
//
//	ROUTINE:	CSoundButeMgr::WriteCache()
//	PURPOSE:	Primal Hunt: write the sound sets to the bute cache
//
//*********************************************************************************

LTBOOL CSoundButeMgr::WriteCache(const char* szFile, CMultiFile* pFile)
{
	if (!g_pInterface || !pFile || !szFile) return LTFALSE;

	// The base class's comes back LTFALSE, so the cache is never written
	if (!CHierarchicalButeMgr::WriteCache(szFile, pFile))
	{
		pFile->Close();
		return LTFALSE;
	}

	pFile->WriteInt(m_nNumSoundButes);

	for (int i=0; i < m_nNumSoundButes; i++)
	{
		m_pSoundButes[i].Write(pFile);
		m_pSoundFiles[i].Write(pFile);
	}

	pFile->Close();
	return LTTRUE;
}

//*********************************************************************************
//
//	ROUTINE:	CSoundButeMgr::ReadCache()
//	PURPOSE:	Primal Hunt: read the sound sets from the bute cache
//
//*********************************************************************************

LTBOOL CSoundButeMgr::ReadCache(const char* szFile, CMultiFile* pFile)
{
	if (!g_pInterface || !pFile || !szFile) return LTFALSE;

	// The base class's comes back LTFALSE, so the cache is never read
	if (!CHierarchicalButeMgr::ReadCache(szFile, pFile))
	{
		pFile->Close();
		return LTFALSE;
	}

	m_nNumSoundButes = pFile->ReadInt();

	m_pSoundButes = new SoundButes[m_nNumSoundButes];
	m_pSoundFiles = new SoundFiles[m_nNumSoundButes];

	for (int i=0; i < m_nNumSoundButes; i++)
	{
		m_pSoundButes[i].Read(pFile);
		m_pSoundFiles[i].Read(pFile);

		// Store away the name-index pair for quick look-up.
		m_IndexTable[m_pSoundFiles[i].m_szName] = i;
	}

	pFile->Close();
	return LTTRUE;
}

#endif // PRIMAL_HUNT


//*********************************************************************************
//
// CSoundButeMgrPlugin is used to help facilitate populating the DEdit object
// properties that use SoundMgr
//
//*********************************************************************************

#ifndef _CLIENTBUILD  // Server-side only

// Plugin statics

DBOOL CSoundButeMgrPlugin::sm_bInitted = DFALSE;
CSoundButeMgr CSoundButeMgrPlugin::sm_ButeMgr;

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CSoundButeMgrPlugin::PreHook_EditStringList
//
//	PURPOSE:	Fill the string list
//
// ----------------------------------------------------------------------- //

LTRESULT	CSoundButeMgrPlugin::PreHook_EditStringList(
	const char* szRezPath, 
	const char* szPropName, 
	char* const * aszStrings, 
	uint32* pcStrings, 
	const uint32 cMaxStrings, 
	const uint32 cMaxStringLength,
	const char* szParent)
{
	if (!sm_bInitted)
	{
		char szFile[256];

		// Create the bute mgr if necessary
		if(!g_pSoundButeMgr)
		{
#ifdef _WIN32
			sprintf(szFile, "%s\\%s", szRezPath, SOUND_BUTES_DEFAULT_FILE);
#else
			sprintf(szFile, "%s/%s", szRezPath, SOUND_BUTES_DEFAULT_FILE);
#endif
			sm_ButeMgr.SetInRezFile(LTFALSE);
			sm_ButeMgr.Init(g_pLTServer, szFile);
			sm_bInitted = LTTRUE;
		}
	}

	// Be sure the null sound is selectable.
	if((*pcStrings) + 1 < cMaxStrings)
	{
		strcpy(aszStrings[(*pcStrings)++], "None");
	}

	// Add an entry for each character...
	uint32 nNumSounds = g_pSoundButeMgr->GetNumSoundButes();

	for (uint32 i = 0; i < nNumSounds; i++)
	{
		_ASSERT(cMaxStrings > (*pcStrings) + 1);

		if((*pcStrings) + 1 < cMaxStrings)
		{
			if(!szParent || !stricmp(g_pSoundButeMgr->GetSoundButeParent(i), szParent))
				strcpy(aszStrings[(*pcStrings)++], g_pSoundButeMgr->GetSoundButeName(i));
		}
	}

	return LT_UNSUPPORTED;
}


#endif //_CLIENTBUILD
