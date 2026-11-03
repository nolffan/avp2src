// *********************************************************************** //
//
//	MODULE:		GAMEHELP (.CPP)
//
//	PURPOSE:	Small helpers for the main GAME dialog - music, web links
//				and the customize sub-dialog launcher.
//
// *********************************************************************** //

#include "stdafx.h"
#include "mmsystem.h"
#include "regmgr.h"
#include "launcher.h"


// Prototypes...

BOOL	CALLBACK CustomizeDlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);


// *********************************************************************** //
//
//	ROUTINE:	StopMusic
//
//	PURPOSE:	Stops the launcher music (no-op in the retail build)
//
// *********************************************************************** //

void StopMusic(HWND hWnd)
{
}


// *********************************************************************** //
//
//	ROUTINE:	PlayMusic
//
//	PURPOSE:	Plays a WAVE resource asynchronously
//
// *********************************************************************** //

BOOL PlayMusic(const char* sSound)
{
	if (!sSound) return(FALSE);

	HMODULE hMod = GetModuleHandle(NULL);
	if (!hMod) return(FALSE);

	return(PlaySound(sSound, hMod, SND_RESOURCE | SND_ASYNC | SND_NODEFAULT));
}


// *********************************************************************** //
//
//	ROUTINE:	OpenWebLink
//
//	PURPOSE:	Opens a URL (loaded from the string table) in the browser
//
// *********************************************************************** //

BOOL OpenWebLink(HWND hWnd, int nUrlID, int nErrID)
{
	CString csUrl;

	if (!csUrl.LoadString(nUrlID))
	{
		return(FALSE);
	}

	if (ShellExecute(hWnd, NULL, csUrl, NULL, NULL, SW_SHOWNORMAL) <= (HINSTANCE)32)
	{
		if (nErrID > 0)
		{
			ErrorMsg(hWnd, nErrID, g_csCaption, MB_ICONEXCLAMATION);
		}

		return(FALSE);
	}

	return(TRUE);
}


// *********************************************************************** //
//
//	ROUTINE:	DoCustomizeDialog
//
//	PURPOSE:	Runs the command-line customize sub-dialog
//
// *********************************************************************** //

void DoCustomizeDialog(HWND hWnd)
{
	DialogBoxParam(g_hInst, "CUSTOMIZE", hWnd, (DLGPROC)CustomizeDlgProc, 0);
}
