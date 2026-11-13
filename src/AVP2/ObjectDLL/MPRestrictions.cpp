// ----------------------------------------------------------------------- //
//
// MODULE  : MPRestrictions.cpp
//
// PURPOSE : The server's class and weapon restrictions: the classes and
//           weapons the multiplayer game's settings list, and whether each
//           is allowed. MultiplayerMgr::UpdateRestrictions fills them from
//           MPGameInfo; CGameServerShell asks them whether a player may
//           take a class (VerifyClient) or a weapon (IsWeaponAllowed).
//
// CREATED : Primal Hunt (Third Law); no source survives, written from
//           PHObject.lto (0x10155b50..0x10155eb7, between Polygon's code
//           and SenseLevels'; the lists are .bss 0x10337d30). The file's
//           name, and every name in it, are ours. AVP2XServ.exe keeps the
//           same pair of lists, and only their initializer (0x409e20).
//
// ----------------------------------------------------------------------- //

#include "stdafx.h"

#ifdef PRIMAL_HUNT

#include "MPRestrictList.h"

// The server's list: CMPRestrictList's, but each entry is added allowed or
// not (as ServerOptions.h's CMPAllowList)
class CMPServerList : public CMPRestrictList
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

			MPRestrictEntry & entry = m_aEntries[i];
			entry.nId = nId;
			entry.bAllowed = bAllowed;
			strcpy(entry.szName, szName);
		}

		// RETAIL BUG: as CMPRestrictList::Insert, the entry at i is copied
		// over all the ones above it and the count is not raised
		void	Insert(int i, int nId, LTBOOL bAllowed, const char* szName)
		{
#ifdef AVP2SRC_MODERN
			// M-B75 (RETAIL_BUGS.md): moved up from the top, and counted
			if((i >= MAX_MP_RESTRICT_ENTRIES) || (m_nNumEntries >= MAX_MP_RESTRICT_ENTRIES))
				return;

			for(int j = m_nNumEntries; j > i; j--)
				m_aEntries[j] = m_aEntries[j - 1];

			m_nNumEntries++;
#else
			if(i >= MAX_MP_RESTRICT_ENTRIES)
				return;

			for(int j = i; j < m_nNumEntries; j++)
				m_aEntries[j + 1] = m_aEntries[j];
#endif

			MPRestrictEntry & entry = m_aEntries[i];
			entry.nId = nId;
			entry.bAllowed = bAllowed;
			strcpy(entry.szName, szName);
		}
};

struct MPRestrictLists
{
	CMPServerList	m_ClassList;
	CMPServerList	m_WeaponList;
};

// PH: the $E initializer 0x10155b50
static MPRestrictLists s_MPRestrictLists;

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	MPRestrict_Clear() (0x10155b90)
//
// ----------------------------------------------------------------------- //

void MPRestrict_Clear()
{
	s_MPRestrictLists.m_ClassList.Clear();
	s_MPRestrictLists.m_WeaponList.Clear();
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	MPRestrict_AddClass() / MPRestrict_AddWeapon()
//				(0x10155bc0, 0x10155cf0)
//
// ----------------------------------------------------------------------- //

void MPRestrict_AddClass(int nId, LTBOOL bAllowed, const char* szName)
{
	s_MPRestrictLists.m_ClassList.Add(nId, bAllowed, szName);
}

void MPRestrict_AddWeapon(int nId, LTBOOL bAllowed, const char* szName)
{
	s_MPRestrictLists.m_WeaponList.Add(nId, bAllowed, szName);
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	MPRestrict_IsClassAllowed() / MPRestrict_IsWeaponAllowed()
//				(0x10155e20, 0x10155e40)
//
//	PURPOSE:	Is the list's entry i allowed?
//
// ----------------------------------------------------------------------- //

// RETAIL BUG: both let i be the list's count, one past its last entry
// (that entry is cleared, so it reads as not allowed)
LTBOOL MPRestrict_IsClassAllowed(int i)
{
#ifdef AVP2SRC_MODERN
	// M-B76 (RETAIL_BUGS.md): the last entry is the count less one
	if (i >= s_MPRestrictLists.m_ClassList.m_nNumEntries || i < 0) return LTFALSE;
#else
	if (i > s_MPRestrictLists.m_ClassList.m_nNumEntries || i < 0) return LTFALSE;
#endif

	return s_MPRestrictLists.m_ClassList.m_aEntries[i].bAllowed;
}

LTBOOL MPRestrict_IsWeaponAllowed(int i)
{
#ifdef AVP2SRC_MODERN
	// M-B76 (RETAIL_BUGS.md): the last entry is the count less one
	if (i >= s_MPRestrictLists.m_WeaponList.m_nNumEntries || i < 0) return LTFALSE;
#else
	if (i > s_MPRestrictLists.m_WeaponList.m_nNumEntries || i < 0) return LTFALSE;
#endif

	return s_MPRestrictLists.m_WeaponList.m_aEntries[i].bAllowed;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	MPRestrict_FindClass() / MPRestrict_FindWeapon()
//				(0x10155e60, 0x10155e90)
//
//	PURPOSE:	The list's entry for a class or weapon ID, or -1
//
// ----------------------------------------------------------------------- //

int MPRestrict_FindClass(int nId)
{
	for (int i = 0; i < s_MPRestrictLists.m_ClassList.m_nNumEntries; i++)
	{
		if (s_MPRestrictLists.m_ClassList.m_aEntries[i].nId == nId) return i;
	}

	return -1;
}

int MPRestrict_FindWeapon(int nId)
{
	for (int i = 0; i < s_MPRestrictLists.m_WeaponList.m_nNumEntries; i++)
	{
		if (s_MPRestrictLists.m_WeaponList.m_aEntries[i].nId == nId) return i;
	}

	return -1;
}

#endif // PRIMAL_HUNT
