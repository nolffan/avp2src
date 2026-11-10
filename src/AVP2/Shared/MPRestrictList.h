// ----------------------------------------------------------------------- //
//
// MODULE  : MPRestrictList.h
//
// PURPOSE : CMPRestrictList - Primal Hunt's list of the classes or weapons
//           a multiplayer server can allow or forbid.
//
//           Third Law's class and weapon restrictions. Nothing of this is
//           in the AvP2 1.0.9.6 release; the layout is the one every copy
//           shares (0x904 bytes: 32 entries of 0x48 and a count):
//           CCharacterButeMgr (+0x1178) lists the "_MP" character sets,
//           CWeaponMgr (+0x119c) the "_MP" weapons (both kept sorted by
//           ID: PHObject.lto 0x1000be8d, LoadButes; 0x100329e6,
//           LoadWeaponButes), and CServerOptions keeps one of each
//           (AVP2XServ.exe 0x4066a0, its constructor) whose IDs and allowed
//           flags FillInHostInfo hands on in MPGameInfo. Names ours.
//
// ----------------------------------------------------------------------- //

#ifndef __MP_RESTRICT_LIST_H__
#define __MP_RESTRICT_LIST_H__

#ifdef PRIMAL_HUNT

#define MAX_MP_RESTRICT_ENTRIES		32
#define MAX_MP_RESTRICT_NAME		64

struct MPRestrictEntry
{
	int			nId;								// The class or weapon ID
	LTBOOL		bAllowed;							// Is it allowed?
	char		szName[MAX_MP_RESTRICT_NAME];		// The class or weapon name
};

class CMPRestrictList
{
	public:

		CMPRestrictList()		{ Clear(); }

		void	Clear()
		{
			m_nNumEntries = 0;
			memset(m_aEntries, 0, sizeof(m_aEntries));
		}

		// Add an entry, allowed, keeping the list sorted by ID; an entry
		// with the same ID is replaced
		void	Add(int nId, const char* szName)
		{
			if(m_nNumEntries >= MAX_MP_RESTRICT_ENTRIES)
				return;

			for(int i = 0; i < m_nNumEntries; i++)
			{
				if(m_aEntries[i].nId == nId)
				{
					Set(i, nId, szName);
					return;
				}

				if(m_aEntries[i].nId > nId)
				{
					Insert(i, nId, szName);
					return;
				}
			}

			MPRestrictEntry & entry = m_aEntries[m_nNumEntries];
			entry.nId = nId;
			entry.bAllowed = LTTRUE;
			strcpy(entry.szName, szName);
			m_nNumEntries++;
		}

		void	Set(int i, int nId, const char* szName)
		{
			if((i >= MAX_MP_RESTRICT_ENTRIES) || (i >= m_nNumEntries))
				return;

			m_aEntries[i].nId = nId;
			m_aEntries[i].bAllowed = LTTRUE;
			strcpy(m_aEntries[i].szName, szName);
		}

		// RETAIL BUG: the entries are moved up from the bottom, so the one
		// at i is copied over all the ones above it, and the count is not
		// raised, so the last one is lost: a list read in out of ID order
		// comes out wrong (PHObject.lto 0x10032a9d, 0x1000bee5)
		void	Insert(int i, int nId, const char* szName)
		{
			if(i >= MAX_MP_RESTRICT_ENTRIES)
				return;

			for(int j = i; j < m_nNumEntries; j++)
				m_aEntries[j + 1] = m_aEntries[j];

			m_aEntries[i].nId = nId;
			m_aEntries[i].bAllowed = LTTRUE;
			strcpy(m_aEntries[i].szName, szName);
		}

#ifdef _CLIENTBUILD
		// (the client's only: the server build of this header declares none of
		// these, and adding them moves unrelated server code, e.g. CWeaponMgr::
		// LoadWeaponButes and Breakable's property-table initialiser)
		// An entry's ID and whether it's allowed (the client's host options
		// folder, PHCShell.dll 0x10019810, 0x10018ba0).
		// RETAIL BUG: the index is checked against the count with '>', so the
		// entry just past the last one passes
		int		GetId(int i)
		{
			if((i > m_nNumEntries) || (i < 0))
				return -1;

			return m_aEntries[i].nId;
		}

		// An entry's name, or none (CServerOptions' constructor, PHCShell.dll
		// 0x100a4fb0); the index is checked as GetId's is
		const char*	GetName(int i)
		{
			if((i > m_nNumEntries) || (i < 0))
				return LTNULL;

			return m_aEntries[i].szName;
		}

		// The index of the entry with this ID, or -1
		int		FindIndex(int nId)
		{
			for(int i = 0; i < m_nNumEntries; i++)
			{
				if(m_aEntries[i].nId == nId)
					return i;
			}

			return -1;
		}

		// RETAIL BUG: an index one past the last entry passes the check
		// (harmless: FindIndex never returns it)
		LTBOOL	IsAllowed(int i)
		{
			if((i > m_nNumEntries) || (i < 0))
				return LTFALSE;

			return m_aEntries[i].bAllowed;
		}

		void	SetAllowed(int i, LTBOOL bAllowed)
		{
			if((i > m_nNumEntries) || (i < 0))
				return;

			m_aEntries[i].bAllowed = bAllowed;
		}
#endif

		MPRestrictEntry	m_aEntries[MAX_MP_RESTRICT_ENTRIES];
		int				m_nNumEntries;
};

#endif // PRIMAL_HUNT

#endif // __MP_RESTRICT_LIST_H__
