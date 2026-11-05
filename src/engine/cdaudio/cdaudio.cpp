// cdaudio.cpp -- cdaudio.dll, the engine's CD-audio music driver.
//
// Reconstructed from the retail Primal Hunt cdaudio.dll (2002-05-07). Its
// one export, MusicDLLSetup, fills the engine's SMusicMgr (musicmgr.h)
// with the functions below; they play Red Book tracks through lithshared's
// CRedMgr (MCI), a "song" being a track number and a play list a list of
// them. A hidden window of its own receives MCI's MM_MCINOTIFY, on which
// the next track of the current list is started.
//
// One object (its Rich header: four C++ objects of cl 12.00.8966, the
// other three library members), MFC linked statically for CRedMgr's
// CObject and the play lists' CString names. Addresses are the retail
// image's.

#include "cdaudio.h"


// ----------------------------------------------------------------------- //
//  Globals
// ----------------------------------------------------------------------- //

// .data: the window's class name and caption are arrays of their own, not
// literals (which /GF would put after them, each in a COMDAT of its own),
// and the 100 after them (0x100120a0, 0x100120a8, 0x100120b0).

static char     s_szWndClass[] = "CDAudio";
static char     s_szWndCaption[] = "Caption";
DWORD           g_dwUnknown100120b0 = 100;  // referenced by nothing


// The two objects come first in .bss (cl puts the uninitialised ones
// there ahead of the rest), the others in the order they are defined here:
// they are initialised (to zero, which cl still puts in .bss), as the
// retail object's layout of them shows.

CRedMgr         g_RedMgr;                   // 0x10015030
CPlayListMgr    g_PlayLists;                // 0x10015020
DWORD           g_nCurTrack = 0;            // 0x10015088: the track playing
LTBOOL          g_bRepeatTrack = FALSE;     // 0x1001508c: cleared wherever a
                                            //   track starts and never set;
                                            //   when set, a finished track
                                            //   would start again
LTBOOL          g_bPaused = FALSE;          // 0x10015090
DWORD           g_dwPausePos = 0;           // 0x10015094: TMSF where Pause
                                            //   stopped
DWORD           g_dwUnknown10015098 = 0;    // 0x10015098 and 0x1001509c:
DWORD           g_dwUnknown1001509c = 0;    //   referenced by nothing
SMusicMgr*      g_pMusicMgr = NULL;         // 0x100150a0
HWND            g_hWnd = NULL;              // 0x100150a4: the driver's own
                                            //   window
HWND            g_hWndApp = NULL;           // 0x100150a8: the engine's (its
                                            //   parent)
HINSTANCE       g_hInstance = NULL;         // 0x100150ac


static void         cdaudio_Term();
static LTBOOL       cdaudio_Init(SMusicMgr *pMusicMgr);
static BOOL         cdaudio_CreateWindow();
static LRESULT CALLBACK cdaudio_WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
static LTBOOL       cdaudio_SetDataDirectory(const char *szDataDir);
static LTBOOL       cdaudio_InitInstruments(const char *szDLSFile, const char *szStyleFile);
static void*        cdaudio_CreateSong(const char *pFileName);
static void*        cdaudio_GetSong(const char *pFileName);
static void         cdaudio_DestroySong(void *pSong);
static LTBOOL       cdaudio_PlayBreak(void *pSong, uint32 dwPlayBoundaryFlags);
static void*        cdaudio_CreatePlayList(const char *szPlayListName);
static void*        cdaudio_GetPlayList(const char *szPlayListName);
static LTBOOL       cdaudio_AddSongToList(void *pPlayList, void *pSong);
static void*        cdaudio_GetSongFromList(void *pPlayList, unsigned long dwIndex);
static void         cdaudio_RemoveList(void *pPlayList);
static LTBOOL       cdaudio_PlayList(void *pPlayList, void *pTransition, LTBOOL bLoop, uint32 dwPlayBoundaryFlags);
static LTBOOL       cdaudio_PlayMotif(const char *szMotifName, LTBOOL bLoop);
static LTBOOL       cdaudio_StopMotif(const char *szMotifName);
static void         cdaudio_DestroyAllSongs();
static void         cdaudio_Stop(uint32 dwPlayBoundaryFlags);
static LTBOOL       cdaudio_Pause(uint32 dwPlayBoundaryFlags);
static LTBOOL       cdaudio_Resume();
static uint16       cdaudio_GetVolume();
static void         cdaudio_SetVolume(unsigned short vol);
static void         cdaudio_OnMCINotify();


// ---- 0x100010f0 ------------------------------------------------------- //

extern "C" __declspec(dllexport) void MusicDLLSetup(SMusicMgr *pMusicMgr)
{
	pMusicMgr->Init             = cdaudio_Init;
	pMusicMgr->Term             = cdaudio_Term;
	pMusicMgr->SetDataDirectory = cdaudio_SetDataDirectory;
	pMusicMgr->InitInstruments  = cdaudio_InitInstruments;
	pMusicMgr->CreateSong       = cdaudio_CreateSong;
	pMusicMgr->GetSong          = cdaudio_GetSong;
	pMusicMgr->DestroySong      = cdaudio_DestroySong;
	pMusicMgr->PlayBreak        = cdaudio_PlayBreak;
	pMusicMgr->CreatePlayList   = cdaudio_CreatePlayList;
	pMusicMgr->GetPlayList      = cdaudio_GetPlayList;
	pMusicMgr->AddSongToList    = cdaudio_AddSongToList;
	pMusicMgr->GetSongFromList  = cdaudio_GetSongFromList;
	pMusicMgr->RemoveList       = cdaudio_RemoveList;
	pMusicMgr->PlayList         = cdaudio_PlayList;
	pMusicMgr->PlayMotif        = cdaudio_PlayMotif;
	pMusicMgr->StopMotif        = cdaudio_StopMotif;
	pMusicMgr->DestroyAllSongs  = cdaudio_DestroyAllSongs;
	pMusicMgr->Stop             = cdaudio_Stop;
	pMusicMgr->Pause            = cdaudio_Pause;
	pMusicMgr->Resume           = cdaudio_Resume;
	pMusicMgr->GetVolume        = cdaudio_GetVolume;
	pMusicMgr->SetVolume        = cdaudio_SetVolume;
}


// ---- 0x10001190 ------------------------------------------------------- //

static void cdaudio_Term()
{
	g_RedMgr.Term();

	g_PlayLists.Term();

	if (g_hWnd)
	{
		DestroyWindow(g_hWnd);
		g_hWnd = NULL;
	}

	g_hWndApp = NULL;
	g_hInstance = NULL;
	g_pMusicMgr = NULL;
}


// ---- 0x10001230 ------------------------------------------------------- //

static LTBOOL cdaudio_Init(SMusicMgr *pMusicMgr)
{
	if (pMusicMgr && pMusicMgr->m_hWnd && pMusicMgr->m_hInstance)
	{
		cdaudio_Term();

		pMusicMgr->m_bValid = FALSE;
		g_hInstance = (HINSTANCE)pMusicMgr->m_hInstance;
		g_hWndApp = (HWND)pMusicMgr->m_hWnd;

		if (cdaudio_CreateWindow())
		{
			if (!g_RedMgr.Init(g_hWnd))
			{
				DestroyWindow(g_hWnd);
				g_hWnd = NULL;
			}
			else
			{
				if (g_RedMgr.IsVolumeControl())
					pMusicMgr->m_ulCaps = MUSIC_VOLUMECONTROL;
				else
					pMusicMgr->m_ulCaps = 0;

				g_pMusicMgr = pMusicMgr;
				pMusicMgr->m_bValid = TRUE;

				g_bPaused = FALSE;
				g_dwPausePos = 0;
				g_pMusicMgr->m_bEnabled = TRUE;

				return TRUE;
			}
		}
	}

	return FALSE;
}


// ---- 0x100012d0 ------------------------------------------------------- //
// The hidden child window MCI notifies when a track ends.

static BOOL cdaudio_CreateWindow()
{
	WNDCLASS wc;

	if (!GetClassInfo(g_hInstance, s_szWndClass, &wc))
	{
		wc.style         = 0;
		wc.lpfnWndProc   = cdaudio_WndProc;
		wc.cbClsExtra    = 0;
		wc.cbWndExtra    = 0;
		wc.hInstance     = g_hInstance;
		wc.hIcon         = NULL;
		wc.hCursor       = NULL;
		wc.hbrBackground = NULL;
		wc.lpszMenuName  = NULL;
		wc.lpszClassName = s_szWndClass;

		if (!RegisterClass(&wc)) return FALSE;
	}

	g_hWnd = CreateWindow(s_szWndClass, s_szWndCaption, WS_CHILD,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
		g_hWndApp, NULL, g_hInstance, NULL);

	return g_hWnd != NULL;
}


// ---- 0x10001390 ------------------------------------------------------- //
// RETAIL BUG: every MM_MCINOTIFY is taken for "the track finished", whatever
// its wParam says. A play that a new one supersedes (MCI_NOTIFY_SUPERSEDED)
// or that is stopped (MCI_NOTIFY_ABORTED) notifies too, so starting a play
// list while a track is playing can skip on to the list's second track.
// (Stop and Pause guard against it by clearing the list or setting
// g_bPaused first; PlayList does not.)

static LRESULT CALLBACK cdaudio_WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (uMsg == MM_MCINOTIFY && g_RedMgr.IsValid())
	{
		cdaudio_OnMCINotify();
		return 0;
	}

	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}


// ---- 0x100013d0 (the linker folded InitInstruments into it) ------------ //

static LTBOOL cdaudio_SetDataDirectory(const char *szDataDir)
{
	return TRUE;
}

static LTBOOL cdaudio_InitInstruments(const char *szDLSFile, const char *szStyleFile)
{
	return TRUE;
}


// ---- 0x100013e0 ------------------------------------------------------- //
// A song is a track number: its "file name" is the number, as text.

static void* cdaudio_CreateSong(const char *pFileName)
{
	if (!pFileName) return NULL;
	if (!g_pMusicMgr) return NULL;

	return (void*)atoi(pFileName);
}


// ---- 0x10001400 ------------------------------------------------------- //

static void* cdaudio_GetSong(const char *pFileName)
{
	if (!g_pMusicMgr) return NULL;

	return cdaudio_CreateSong(pFileName);
}


// ---- 0x10001420 ------------------------------------------------------- //

static void cdaudio_DestroySong(void *pSong)
{
}


// ---- 0x10001430 ------------------------------------------------------- //

static void* cdaudio_CreatePlayList(const char *szPlayListName)
{
	if (!szPlayListName) return NULL;
	if (!g_pMusicMgr) return NULL;

	return g_PlayLists.CreatePlayList(szPlayListName);
}


// ---- 0x10001460 ------------------------------------------------------- //

static void* cdaudio_GetPlayList(const char *szPlayListName)
{
	if (!szPlayListName) return NULL;
	if (!g_pMusicMgr) return NULL;

	return g_PlayLists.GetPlayList(szPlayListName);
}


// ---- 0x100014e0 ------------------------------------------------------- //

static LTBOOL cdaudio_AddSongToList(void *pPlayList, void *pSong)
{
	if (pPlayList && pSong && g_pMusicMgr)
	{
		CPlayList *pList = (CPlayList*)pPlayList;
		return pList->AddSong((DWORD)pSong);
	}

	return FALSE;
}


// ---- 0x10001600 ------------------------------------------------------- //
// RETAIL BUG: never gives the song back -- NULL whatever the list and
// index (the retail code tests the list and loads g_pMusicMgr, then
// returns 0 on every path).

static void* cdaudio_GetSongFromList(void *pPlayList, unsigned long dwIndex)
{
	if (!pPlayList || !g_pMusicMgr) return NULL;

	return NULL;
}


// ---- 0x10001610 ------------------------------------------------------- //
// RETAIL BUG: removing the list that is playing leaves
// g_PlayLists.m_pCurPlayList pointing at the deleted list, which the next
// MM_MCINOTIFY (cdaudio_OnMCINotify) and Resume read.

static void cdaudio_RemoveList(void *pPlayList)
{
	if (!pPlayList) return;
	if (!g_pMusicMgr) return;

	CPlayList *pList = (CPlayList*)pPlayList;
	pList->m_Link.Remove();
	delete pList;
}


// ---- 0x10001650 ------------------------------------------------------- //

static LTBOOL cdaudio_PlayList(void *pPlayList, void *pTransition, LTBOOL bLoop, uint32 dwPlayBoundaryFlags)
{
	if (g_RedMgr.IsValid() && pPlayList && g_pMusicMgr)
	{
		CPlayList *pList = (CPlayList*)pPlayList;
		g_PlayLists.m_pCurPlayList = pList;

		if (pList->m_Songs.GetSize() != 0)
		{
			if (pList->m_Songs.GetSize() > 0) pList->m_iCurSong = 0;

			pList->m_bLoop = bLoop;

			DWORD nTrack = pList->m_Songs[pList->m_iCurSong];
			if (nTrack)
			{
				g_dwPausePos = 0;
				g_nCurTrack = nTrack;
				g_bRepeatTrack = FALSE;

				if (g_pMusicMgr->m_bEnabled && !g_bPaused)
				{
					return g_RedMgr.PlayTrack(nTrack);
				}
			}
		}
	}

	return FALSE;
}


// ---- 0x100016d0 ------------------------------------------------------- //

static void cdaudio_Stop(uint32 dwPlayBoundaryFlags)
{
	if (!g_RedMgr.IsValid()) return;
	if (!g_pMusicMgr) return;

	g_bPaused = FALSE;
	g_dwPausePos = 0;
	g_PlayLists.m_pCurPlayList = NULL;

	g_RedMgr.Stop();
}


// ---- 0x10001700 ------------------------------------------------------- //

static LTBOOL cdaudio_Pause(uint32 dwPlayBoundaryFlags)
{
	if (!g_RedMgr.IsValid()) return FALSE;
	if (!g_pMusicMgr) return FALSE;

	LTBOOL bRet = g_RedMgr.Pause();
	g_dwPausePos = g_RedMgr.GetPosition();
	g_bPaused = TRUE;

	return bRet;
}


// ---- 0x10001740 ------------------------------------------------------- //

static LTBOOL cdaudio_Resume()
{
	if (g_RedMgr.IsValid() && g_pMusicMgr && g_pMusicMgr->m_bEnabled)
	{
		CPlayList *pList = g_PlayLists.m_pCurPlayList;
		if (pList)
		{
			DWORD nTrack = pList->m_Songs[pList->m_iCurSong];
			if (!nTrack)
			{
				g_PlayLists.m_pCurPlayList = NULL;
			}
			else
			{
				g_bPaused = FALSE;

				if (!g_dwPausePos)
				{
					g_nCurTrack = nTrack;
					g_bRepeatTrack = FALSE;

					if (g_pMusicMgr->m_bEnabled) return g_RedMgr.PlayTrack(nTrack);
					return TRUE;
				}

				DWORD nPauseTrack = MCI_TMSF_TRACK(g_dwPausePos);
				if (nPauseTrack)
				{
					UINT32 nLen = g_RedMgr.GetTrackLen(nPauseTrack);

					g_bRepeatTrack = FALSE;
					g_nCurTrack = nPauseTrack;

					LTBOOL bRet = g_RedMgr.Play(nPauseTrack, MCI_TMSF_MINUTE(g_dwPausePos), MCI_TMSF_SECOND(g_dwPausePos), MCI_TMSF_FRAME(g_dwPausePos),
						nPauseTrack, MCI_MSF_MINUTE(nLen), MCI_MSF_SECOND(nLen), MCI_MSF_FRAME(nLen));
					g_dwPausePos = 0;

					return bRet;
				}
			}
		}
	}

	return FALSE;
}


// ---- 0x10001830 (the linker folded PlayMotif and StopMotif into it) ---- //

static LTBOOL cdaudio_PlayBreak(void *pSong, uint32 dwPlayBoundaryFlags)
{
	return FALSE;
}

static LTBOOL cdaudio_PlayMotif(const char *szMotifName, LTBOOL bLoop)
{
	return FALSE;
}

static LTBOOL cdaudio_StopMotif(const char *szMotifName)
{
	return FALSE;
}


// ---- 0x10001840 ------------------------------------------------------- //

static void cdaudio_DestroyAllSongs()
{
	g_PlayLists.Term();

	cdaudio_Stop(0);
}


// ---- 0x100018b0 ------------------------------------------------------- //

static uint16 cdaudio_GetVolume()
{
	if (!g_pMusicMgr) return 0;

	return (uint16)g_RedMgr.GetVolume();
}


// ---- 0x100018d0 ------------------------------------------------------- //

static void cdaudio_SetVolume(unsigned short vol)
{
	if (!g_pMusicMgr) return;

	g_RedMgr.SetVolume(vol);
}


// ---- 0x100018f0 ------------------------------------------------------- //
// MM_MCINOTIFY: a track has finished. Start the next of the current list,
// or the list again from its start when it loops; with no next track the
// list is forgotten.

static void cdaudio_OnMCINotify()
{
	if (!g_pMusicMgr) return;
	if (!g_pMusicMgr->m_bEnabled) return;

	if (g_nCurTrack && g_bRepeatTrack)
	{
		g_RedMgr.PlayTrack(g_nCurTrack);
		return;
	}

	CPlayList *pList = g_PlayLists.m_pCurPlayList;
	if (!pList) return;
	if (g_bPaused) return;
	if (g_dwPausePos) return;

	DWORD nTrack = 0;
	DWORD iNext = pList->m_iCurSong + 1;
	if (iNext < pList->m_Songs.GetSize())
	{
		pList->m_iCurSong = iNext;
		nTrack = pList->m_Songs[iNext];
	}
	else if (pList->m_bLoop)
	{
		cdaudio_PlayList(pList, NULL, TRUE, 0);
		return;
	}

	if (!nTrack)
	{
		g_PlayLists.m_pCurPlayList = NULL;
		return;
	}

	g_nCurTrack = nTrack;
	g_bRepeatTrack = FALSE;

	if (g_pMusicMgr->m_bEnabled) g_RedMgr.PlayTrack(nTrack);
}

