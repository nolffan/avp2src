// ServerOptions.h: interface for the CServerOptionMgr class.
//
//////////////////////////////////////////////////////////////////////

#ifndef SERVEROPTIONS_H
#define SERVEROPTIONS_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "MultiplayerMgrDefs.h"
#include <hash_map>
#include "Globals.h"
#include "ClientServerShared.h"
#ifdef PRIMAL_HUNT
#include "MPRestrictList.h"

// PH: the dedicated server's class and weapon lists: CMPRestrictList's
// layout, but each entry is added allowed or not, as the server's
// configuration says (AVP2XServ.exe 0x407c6b, LoadFromBute; the game's
// own lists add every entry allowed). Name ours.
class CMPAllowList : public CMPRestrictList
{
	public:

		// Add an entry, keeping the list sorted by ID; an entry with the
		// same ID is replaced
		void	Add(int nId, LTBOOL bAllowed, const char* szName)
		{
			if(m_nNumEntries >= MAX_MP_RESTRICT_ENTRIES)
				return;

			for(int i = 0; i < m_nNumEntries; i++)
			{
				if(m_aEntries[i].nId == nId)
				{
					Set(i, nId, bAllowed, szName);
					return;
				}

				if(m_aEntries[i].nId > nId)
				{
					Insert(i, nId, bAllowed, szName);
					return;
				}
			}

			MPRestrictEntry & entry = m_aEntries[m_nNumEntries];
			entry.nId = nId;
			entry.bAllowed = bAllowed;
			strcpy(entry.szName, szName);
			m_nNumEntries++;
		}

		void	Set(int i, int nId, LTBOOL bAllowed, const char* szName)
		{
			if((i >= MAX_MP_RESTRICT_ENTRIES) || (i >= m_nNumEntries))
				return;

			m_aEntries[i].nId = nId;
			m_aEntries[i].bAllowed = bAllowed;
			strcpy(m_aEntries[i].szName, szName);
		}

		// RETAIL BUG: as CMPRestrictList::Insert, the entry at i is copied
		// over all the ones above it and the count is not raised (the
		// client's copy, out of line in its constructor: PHCShell.dll
		// 0x100a88d0)
		LTBOOL	Insert(int i, int nId, LTBOOL bAllowed, const char* szName)
		{
#ifdef AVP2SRC_MODERN
			// M-B75 (RETAIL_BUGS.md): moved up from the top, and counted
			if((i >= MAX_MP_RESTRICT_ENTRIES) || (m_nNumEntries >= MAX_MP_RESTRICT_ENTRIES))
				return LTFALSE;

			for(int j = m_nNumEntries; j > i; j--)
				m_aEntries[j] = m_aEntries[j - 1];

			m_nNumEntries++;
#else
			if(i >= MAX_MP_RESTRICT_ENTRIES)
				return LTFALSE;

			for(int j = i; j < m_nNumEntries; j++)
				m_aEntries[j + 1] = m_aEntries[j];
#endif

			m_aEntries[i].nId = nId;
			m_aEntries[i].bAllowed = bAllowed;
			strcpy(m_aEntries[i].szName, szName);

			return LTTRUE;
		}
};
#endif

class CButeMgr;
class StartGameRequest;

uint32	GetValidLANPort(uint32 nPortID);

class CServerOptions
{
	public:

	enum
	{
		eDefaultScoringValue = 25
	};

	public:

		CServerOptions();

		void		SaveToBute(CButeMgr& buteMgr);
		void		LoadFromBute(CButeMgr& buteMgr);

		void		FillInHostInfo(StartGameRequest &request, MPGameInfo &gi);
#ifdef PRIMAL_HUNT
		// PH: a bit for each allowed entry of the class and weapon lists
		// (AVP2XServ.exe 0x408b60, 0x408ba0)
		uint32		GetAllowedClasses();
		uint32		GetAllowedWeapons();
#endif
		LTBOOL		MapListEmpty();

		uint8		GetScoring( uint32 nScoringClass );
		void		SetScoring( uint32 nScoringClass, uint8 nScoring );

		void		CleanServerName();

		char		szCfgName[MAX_MP_CFG_NAME_LENGTH];

		// -------------------------------------------------------------------------
		// Basic game information
		char		szServerName[MAX_MP_HOST_NAME_LENGTH];
		char		szPassword[MAX_MP_PASSWORD_LENGTH];
		char		szSCPassword[MAX_MP_PASSWORD_LENGTH];
		uint8		nGameType;
		uint32		nPortID;
		uint32		nBandwidth;
		LTBOOL		bLocked;
		LTBOOL		bDedicated;
		LTBOOL		bLANConnection;

		// -------------------------------------------------------------------------
		// Advanced options
		LTFLOAT		fSpeed;
		LTFLOAT		fRespawn;
		LTFLOAT		fDamage;

		LTBOOL		bLocationDamage;
		LTBOOL		bFriendlyFire;
		LTBOOL		bFriendlyNames;
		LTBOOL		bMaskLoss;
		LTBOOL		bClassWeapons;
		uint8		nExosuit;
		uint8		nQueenMolt;

		// Defines a hash map of class id's to scorings.
		typedef std::hash_map< uint32, uint8 > ScoringClassHashMap;
		ScoringClassHashMap m_hashScoring;

		// -------------------------------------------------------------------------
		// Game specific settings below this point...

		// Deathmatch
		uint8		nDM_MaxTotalPlayers;
		uint8		nDM_MaxPlayers[MAX_TEAMS];
		uint8		nDM_FragLimit;
		uint16		nDM_ScoreLimit;
		uint8		nDM_TimeLimit;
		uint8		nDM_Lifecycle;
		StringList	sDM_Maps;

		// Team Deathmatch
		uint8		nTeam_MaxTotalPlayers;
		uint8		nTeam_MaxPlayers[MAX_TEAMS];
		uint8		nTeam_FragLimit;
		uint16		nTeam_ScoreLimit;
		uint8		nTeam_TimeLimit;
		uint8		nTeam_Lifecycle;
		StringList	sTeam_Maps;

		// Hunt
		uint8		nHunt_MaxTotalPlayers;
		uint8		nHunt_FragLimit;
		uint8		nHunt_TimeLimit;
		uint8		nHunt_HunterRace;
		uint8		nHunt_PreyRace;
		uint8		nHunt_Ratio;
		StringList	sHunt_Maps;

		// Survivor
		uint8		nSurv_MaxTotalPlayers;
		uint16		nSurv_ScoreLimit;
		uint8		nSurv_RoundLimit;
		uint8		nSurv_TimeLimit;
		uint8		nSurv_SurvivorRace;
		uint8		nSurv_MutateRace;
		StringList	sSurv_Maps;

		// Overrun
		uint8		nOver_MaxTotalPlayers;
		uint8		nOver_RoundLimit;
		uint8		nOver_TimeLimit;
		uint8		nOver_DefenderRace;
		uint8		nOver_DefenderLives;
		uint8		nOver_AttackerRace;
		uint8		nOver_AttackerLives;
		StringList	sOver_Maps;

		// Evac
		uint8		nEvac_MaxTotalPlayers;
		uint8		nEvac_RoundLimit;
		uint8		nEvac_TimeLimit;
		uint8		nEvac_DefenderRace;
		uint8		nEvac_DefenderLives;
		uint8		nEvac_AttackerRace;
		uint8		nEvac_AttackerLives;
		StringList	sEvac_Maps;

#ifdef PRIMAL_HUNT
		// PH: the classes and the weapons the server allows (AVP2XServ.exe
		// 0x4066a0: two lists of 0x904 bytes at +0x140 and +0xa44, cleared
		// by their constructors and again at the end of this one; names
		// ours)
		CMPAllowList	m_ClassList;
		CMPAllowList	m_WeaponList;
#endif
};




#endif	// SERVEROPTIONS_H
