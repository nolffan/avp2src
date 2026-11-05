// cdaudio.h -- cdaudio.dll's play lists, and what the driver includes.
//
// In a header of its own, ahead of the code: the retail object's COMDATs
// put every inline function and template instance after the last of the
// driver's own functions (BaseDelete at 0x100019a0 to ~CMoArray at
// 0x10002350), which is how cl 12.00 places those it reads through an
// automatic precompiled header (/YX), and not those of the .cpp itself.

#ifndef __CDAUDIO_H__
#define __CDAUDIO_H__

#include <afxwin.h>
#include <mmsystem.h>
#include <stdlib.h>
#include <string.h>

#include "redmgr.h"
#include "dynarray.h"
#include "ltbasetypes.h"
#include "musicmgr.h"


// ----------------------------------------------------------------------- //
//  A play list: a named list of track numbers, linked into the driver's
//  list of them through m_Link (whose m_pData is the play list). new'd by
//  CPlayListMgr::CreatePlayList (operator new(0x30), 0x10001ac1).
// ----------------------------------------------------------------------- //

class CPlayList
{
public:

	CPlayList()
	{
		m_Link.m_pData = this;
		m_iCurSong = (DWORD)-1;
		m_bLoop = FALSE;
		m_dwUnknown2c = 0;
	}

	~CPlayList();

	// Adds a track to the list: at its end, or at its front. (Only ever
	// called for the end, 0x100014e0. The front's Insert, inlined and then
	// thrown away as dead, still counts against cl's per-function inline
	// budget in cdaudio_AddSongToList: that is why the retail build calls
	// BaseNew (0x100019c0) and BaseDelete (0x100019a0) out of line there,
	// where GenAppend (0x10001c10), the same Append on its own, inlines
	// them. A plain m_Songs.Append inlines them too.)
	BOOL AddSong(DWORD nTrack, BOOL bFront = FALSE)
	{
		if (bFront) return m_Songs.Insert(0, nTrack);
		return m_Songs.Append(nTrack);
	}

	// (a call of its own in CreatePlayList, not the two stores written
	// out: that is what leaves _InitArray and _DeleteAndDestroyArray
	// out of line there, as the retail build has them, 0x10002240 and
	// 0x10002260)
	void ClearFlags()
	{
		m_bLoop = FALSE;
		m_dwUnknown2c = 0;
	}

	void Term()
	{
		m_sName.Empty();
		m_iCurSong = 0;
		m_Songs.Term();
	}

	LTLink              m_Link;         // 0x00
	CString             m_sName;        // 0x0c
	CMoArray<DWORD>     m_Songs;        // 0x10: track numbers
	DWORD               m_iCurSong;     // 0x24: index into m_Songs
	LTBOOL              m_bLoop;        // 0x28
	DWORD               m_dwUnknown2c;  // 0x2c: cleared, never read
};


// ----------------------------------------------------------------------- //
//  The play lists (0x10015020), and the one playing (0x1001502c).
// ----------------------------------------------------------------------- //

class CPlayListMgr
{
public:

	CPlayListMgr()
	{
		m_pCurPlayList = NULL;
		m_Head.TieOff();
	}

	~CPlayListMgr() { Term(); }

	void Term()
	{
		LTLink *pCur, *pNext;

		pCur = m_Head.m_pNext;
		while (pCur != &m_Head)
		{
			pNext = pCur->m_pNext;
			pCur->Remove();
			delete (CPlayList*)pCur->m_pData;
			pCur = pNext;
		}

		m_Head.TieOff();
		m_pCurPlayList = NULL;
	}

	// 0x10001a80. NEAR (304 bytes both): one register pair swapped in
	// the inlined m_Songs.Term() (m_nElements and the cache size loaded
	// into eax/edx the other way round, 0x10001b69); every cl revision held
	// agrees, and so do name/Reset/SetLoop wrappers, SetSize(0) or
	// RemoveAll() for Term(), and ClearFlags() in the constructor.
	CPlayList* CreatePlayList(const char *pName)
	{
		if (!pName) return NULL;

		CPlayList *pPlayList = new CPlayList;
		if (!pPlayList) return NULL;

		pPlayList->m_sName = pName;
		pPlayList->m_iCurSong = 0;
		pPlayList->m_Songs.Term();
		pPlayList->ClearFlags();
		dl_Insert(&m_Head, &pPlayList->m_Link);

		return pPlayList;
	}

	CPlayList* GetPlayList(const char *pName)
	{
		for (LTLink *pCur = m_Head.m_pNext; pCur != &m_Head; pCur = pCur->m_pNext)
		{
			CPlayList *pPlayList = (CPlayList*)pCur->m_pData;
			if (strcmp(pPlayList->m_sName, pName) == 0) return pPlayList;
		}

		return NULL;
	}

	LTLink      m_Head;             // 0x10015020
	CPlayList*  m_pCurPlayList;     // 0x1001502c
};


// 0x10002290. Defined here, after CPlayListMgr: cl emits the inline
// functions it defers in the order their definitions come, and the retail
// object has ~CPlayList (and the ~CMoArray it brings in, 0x10002350) last,
// after CreatePlayList and everything it needs.
inline CPlayList::~CPlayList()
{
	Term();
}



#endif // __CDAUDIO_H__
