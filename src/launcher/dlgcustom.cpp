// *********************************************************************** //
//
//	MODULE:		DLGCUSTOM (.CPP)
//
//	PURPOSE:	The "Advanced Options - Customize" dialog: picking which
//				.REZ files in the Custom folder get loaded with the game,
//				and persisting that choice in the registry.
//
//				Shipped NOLF.exe v1.003: 0x4013e0 - 0x401e01.
//
//				NolfServ.exe, the stand-alone server, compiles this file
//				too (_NOLFSERV), for its wizard's customization page
//				(src/engine/nolfserv/CustomDlg.cpp, which owns the
//				globals there): every function but the dialog proc, with
//				the server's buttons, its own registry values, and a
//				CRegMgr object in place of the launcher's pointer.
//				Retail NolfServ.exe: 0x4015a0 - 0x401ccf.
//
// *********************************************************************** //

#include "stdafx.h"
#include "direct.h"
#include "io.h"
#include "regmgr.h"
#ifndef _NOLFSERV
#include "launcher.h"


// Globals...

CStringList	g_lstRezFiles;			// 0x4168
int			g_bSaveRezFiles;		// 0x4184

#define REZKEY_SAVE			"Save Rez Files"
#define REZKEY_FILES		"Rez Files"
#else
// The server's globals (CustomDlg.cpp), its buttons (IDC_ADD, IDC_REMOVE)
// and its registry values. Its registry is an object, not a pointer, and
// the launcher's checks of the pointer are not in its copy.
extern CStringList	g_lstRezFiles;
extern int			g_bSaveRezFiles;
extern CRegMgr		g_RegMgr;
#define g_pRegMgr			(&g_RegMgr)

#define REZBTN_ADD			1052
#define REZBTN_REMOVE		1034
#define REZKEY_SAVE			"Save Rez Files (Server)"
#define REZKEY_FILES		"Rez Files (Server)"
#endif


// Prototypes...

int		FillAvailableRezList(HWND hList);
int		FillSelectedRezList(HWND hList);
BOOL	AddSelectedRez(HWND hAvail, HWND hSel);
BOOL	RemoveSelectedRez(HWND hAvail, HWND hSel);
BOOL	IsRezInList(HWND hList, const char* sRez);
BOOL	CanAddRez(HWND hAvail, HWND hSel);
BOOL	CanRemoveRez(HWND hAvail, HWND hSel);
void	UpdateRezButtons(HWND hWnd);
int		SaveRezList(HWND hList);
void	SaveRezFiles(HWND hWnd);


#ifndef _NOLFSERV
// *********************************************************************** //
//
//	ROUTINE:	CustomizeDlgProc()   [0x401420]
//
//	PURPOSE:	Dialog proc for the ADV_CUSTOM dialog.
//
//	The gotos are real: the 1037 button and the 1039 double-click run the
//	same block, as do 1038 and 1040, and the shipped code reaches each
//	block from two places rather than carrying two copies.  Written as
//	duplicated bodies VC6 merges one pair and not the other, costing 56
//	bytes.
//
// *********************************************************************** //

BOOL CALLBACK CustomizeDlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
		case WM_INITDIALOG:
		{
			LoadRezFiles();

			HICON hIcon = LoadIcon(g_hInst, "ICON");
			if (hIcon)
				SendMessage(hWnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);

			if (FillAvailableRezList(GetDlgItem(hWnd, 1039)) <= 0)
			{
				ErrorMsg(hWnd, 16, 0);
				EndDialog(hWnd, 0);
			}
			else
			{
				FillSelectedRezList(GetDlgItem(hWnd, 1040));
				UpdateRezButtons(hWnd);
			}

			CheckDlgButton(hWnd, 1043, g_bSaveRezFiles != 0);
			return TRUE;
		}

		case WM_COMMAND:
		{
			if (wParam == IDOK)
			{
				SaveRezList(GetDlgItem(hWnd, 1040));
				SaveRezFiles(hWnd);
				EndDialog(hWnd, 1);
				return TRUE;
			}

			if (wParam == IDCANCEL)
			{
				EndDialog(hWnd, 0);
				return TRUE;
			}

			if (wParam == 1037) goto AddRez;
			if (wParam == 1038) goto RemoveRez;

			if (HIWORD(wParam) == LBN_SELCHANGE)
			{
				UpdateRezButtons(hWnd);
				return TRUE;
			}

			if (HIWORD(wParam) == LBN_DBLCLK)
			{
				if (LOWORD(wParam) == 1039)
				{
AddRez:
					AddSelectedRez(GetDlgItem(hWnd, 1039), GetDlgItem(hWnd, 1040));
					UpdateRezButtons(hWnd);
					return TRUE;
				}

				if (LOWORD(wParam) == 1040)
				{
RemoveRez:
					RemoveSelectedRez(GetDlgItem(hWnd, 1039), GetDlgItem(hWnd, 1040));
					UpdateRezButtons(hWnd);
					return TRUE;
				}
			}

			return TRUE;
		}
	}

	return FALSE;
}
#endif


// *********************************************************************** //
//
//	ROUTINE:	FillAvailableRezList()   [0x4015e0]
//
//	PURPOSE:	List every *.rez in the "Custom" folder.  Returns the number
//				of files added (0 if the folder is missing or empty).
//
//	The _findnext() result is named in the loop body (`int nRet = ...;
//	if (nRet != 0) break;`).  With the call tested directly -- while,
//	for(;;) + break, return-in-loop, a backward goto, do/while(TRUE) --
//	VC6 rotates the loop, duplicating the test at the bottom (228 bytes);
//	the named local keeps the single top test and the unconditional back
//	edge the shipped code has (0x401676 / 0x401698).
//
// *********************************************************************** //

int FillAvailableRezList(HWND hList)
{
	if (!hList)
		return 0;

	SendMessage(hList, LB_RESETCONTENT, 0, 0);

	if (_chdir("Custom") != 0)
		return 0;

	struct _finddata_t fileinfo;
	long hFind = _findfirst("*.rez", &fileinfo);
	if (hFind == -1)
	{
		_chdir("..");
		return 0;
	}

	SendMessage(hList, LB_ADDSTRING, 0, (LPARAM)fileinfo.name);
	int nCount = 1;

	while (TRUE)
	{
		int nRet = _findnext(hFind, &fileinfo);
		if (nRet != 0)
			break;

		SendMessage(hList, LB_ADDSTRING, 0, (LPARAM)fileinfo.name);
		nCount++;
	}

	_chdir("..");
	return nCount;
}


// *********************************************************************** //
//
//	ROUTINE:	FillSelectedRezList()   [0x4016c0]
//
//	PURPOSE:	Fill the "always load" list box from g_lstRezFiles.
//
// *********************************************************************** //

int FillSelectedRezList(HWND hList)
{
	if (!hList)
		return 0;

	SendMessage(hList, LB_RESETCONTENT, 0, 0);

	int nCount = 0;
	POSITION pos = g_lstRezFiles.GetHeadPosition();
	while (pos)
	{
		CString csRez = g_lstRezFiles.GetNext(pos);
		SendMessage(hList, LB_ADDSTRING, 0, (LPARAM)(const char*)csRez);
		nCount++;
	}

	return nCount;
}


// *********************************************************************** //
//
//	ROUTINE:	AddSelectedRez()   [0x401730]
//
//	PURPOSE:	Copy the current selection of hAvail into hSel.
//
// *********************************************************************** //

BOOL AddSelectedRez(HWND hAvail, HWND hSel)
{
	if (!hAvail)
		return FALSE;

	if (!hSel)
		return FALSE;

	int nIndex = SendMessage(hAvail, LB_GETCURSEL, 0, 0);
	if (nIndex == LB_ERR)
		return FALSE;

	char sRez[256];
	if (SendMessage(hAvail, LB_GETTEXT, nIndex, (LPARAM)sRez) == LB_ERR)
		return FALSE;

	if (IsRezInList(hSel, sRez))
		return TRUE;

	return (SendMessage(hSel, LB_ADDSTRING, 0, (LPARAM)sRez) != LB_ERR);
}


// *********************************************************************** //
//
//	ROUTINE:	RemoveSelectedRez()   [0x4017f0]
//
//	PURPOSE:	Drop the current selection from hSel and keep a sane
//				selection afterwards.
//
// *********************************************************************** //

BOOL RemoveSelectedRez(HWND hAvail, HWND hSel)
{
	if (!hAvail)
		return FALSE;

	if (!hSel)
		return FALSE;

	int nIndex = SendMessage(hSel, LB_GETCURSEL, 0, 0);
	if (nIndex == LB_ERR)
		return FALSE;

	if (SendMessage(hSel, LB_DELETESTRING, nIndex, 0) == LB_ERR)
		return FALSE;

	if (SendMessage(hSel, LB_SETCURSEL, nIndex, 0) == LB_ERR)
	{
		if (nIndex > 0)
			SendMessage(hSel, LB_SETCURSEL, nIndex - 1, 0);
	}

	return TRUE;
}


// *********************************************************************** //
//
//	ROUTINE:	IsRezInList()   [0x401870]
//
//	PURPOSE:	Is sRez already in the list box?
//
// *********************************************************************** //

BOOL IsRezInList(HWND hList, const char* sRez)
{
	if (!hList)
		return FALSE;

	if (!sRez)
		return FALSE;

	int nCount = SendMessage(hList, LB_GETCOUNT, 0, 0);
	if (nCount == LB_ERR)
		return FALSE;

	char sText[256];
	for (int i = 0; i < nCount; i++)
	{
		if (SendMessage(hList, LB_GETTEXT, i, (LPARAM)sText) == LB_ERR)
			continue;

		if (strcmp(sRez, sText) == 0)
			return TRUE;
	}

	return FALSE;
}


// *********************************************************************** //
//
//	ROUTINE:	CanAddRez()   [0x401920]
//
//	PURPOSE:	Enable state for the "Add >" button.
//
// *********************************************************************** //

BOOL CanAddRez(HWND hAvail, HWND hSel)
{
	if (!hAvail)
		return FALSE;

	if (!hSel)
		return FALSE;

	int nIndex = SendMessage(hAvail, LB_GETCURSEL, 0, 0);
	if (nIndex == LB_ERR)
		return FALSE;

	char sRez[256];
	if (SendMessage(hAvail, LB_GETTEXT, nIndex, (LPARAM)sRez) == LB_ERR)
		return FALSE;

	return !IsRezInList(hSel, sRez);
}


// *********************************************************************** //
//
//	ROUTINE:	CanRemoveRez()   [0x4019c0]
//
//	PURPOSE:	Enable state for the "< Remove" button.
//
// *********************************************************************** //

BOOL CanRemoveRez(HWND hAvail, HWND hSel)
{
	if (!hAvail)
		return FALSE;

	if (!hSel)
		return FALSE;

	return (SendMessage(hSel, LB_GETCURSEL, 0, 0) != LB_ERR);
}


// *********************************************************************** //
//
//	ROUTINE:	UpdateRezButtons()   [0x4019f0]
//
// *********************************************************************** //

void UpdateRezButtons(HWND hWnd)
{
	if (!hWnd)
		return;

#ifndef _NOLFSERV
	EnableWindow(GetDlgItem(hWnd, 1037),
		CanAddRez(GetDlgItem(hWnd, 1039), GetDlgItem(hWnd, 1040)));

	EnableWindow(GetDlgItem(hWnd, 1038),
		CanRemoveRez(GetDlgItem(hWnd, 1039), GetDlgItem(hWnd, 1040)));
#else
	// The server's copy finds the answer, then the button: the button's
	// GetDlgItem() comes after CanAddRez() / CanRemoveRez() (0x4019b0).
	BOOL bEnable = CanAddRez(GetDlgItem(hWnd, 1039), GetDlgItem(hWnd, 1040));
	HWND hButton = GetDlgItem(hWnd, REZBTN_ADD);
	EnableWindow(hButton, bEnable);

	bEnable = CanRemoveRez(GetDlgItem(hWnd, 1039), GetDlgItem(hWnd, 1040));
	hButton = GetDlgItem(hWnd, REZBTN_REMOVE);
	EnableWindow(hButton, bEnable);
#endif
}


// *********************************************************************** //
//
//	ROUTINE:	SaveRezList()   [0x401a60]
//
//	PURPOSE:	Replace g_lstRezFiles with the contents of the list box.
//
// *********************************************************************** //

int SaveRezList(HWND hList)
{
	if (!hList)
		return 0;

	g_lstRezFiles.RemoveAll();

	int nCount = SendMessage(hList, LB_GETCOUNT, 0, 0);
	if (nCount == LB_ERR)
		return 0;

	int nAdded = 0;
	char sRez[256];
	for (int i = 0; i < nCount; i++)
	{
		if (SendMessage(hList, LB_GETTEXT, i, (LPARAM)sRez) == LB_ERR)
			continue;

		g_lstRezFiles.AddTail(sRez);
		nAdded++;
	}

	return nAdded;
}


// *********************************************************************** //
//
//	ROUTINE:	SaveRezFiles()   [0x401b00]
//
//	PURPOSE:	Persist the "always load these rez files" check box and the
//				pipe-separated file list.
//
// *********************************************************************** //

void SaveRezFiles(HWND hWnd)
{
#ifndef _NOLFSERV
	if (!g_pRegMgr)
		return;
#endif

	int bSave = (IsDlgButtonChecked(hWnd, 1043) == BST_CHECKED);
	g_pRegMgr->Set(REZKEY_SAVE, bSave);

	if (!bSave)
		return;

	char sRezFiles[2048];
	sRezFiles[0] = '\0';

	POSITION pos = g_lstRezFiles.GetHeadPosition();
	while (pos)
	{
		CString csRez = g_lstRezFiles.GetNext(pos);
		if (!csRez.IsEmpty())
		{
			if (sRezFiles[0])
				strcat(sRezFiles, "|");

			strcat(sRezFiles, csRez);
		}
	}

	g_pRegMgr->Set(REZKEY_FILES, sRezFiles);
}


// *********************************************************************** //
//
//	ROUTINE:	LoadRezFiles()   [0x401c10]
//
//	PURPOSE:	Read the persisted rez file list back into g_lstRezFiles.
//
// *********************************************************************** //

void LoadRezFiles()
{
#ifndef _NOLFSERV
	if (!g_pRegMgr)
		return;
#endif

	g_lstRezFiles.RemoveAll();

	if (!g_pRegMgr->Get(REZKEY_SAVE, 0))
	{
		g_bSaveRezFiles = 0;
		return;
	}

	g_bSaveRezFiles = 1;

	UINT nSize = 2046;
	char sRez[128];
	char sRezFiles[2048];
	sRezFiles[0] = '\0';

	g_pRegMgr->Get(REZKEY_FILES, sRezFiles, nSize, NULL);

	if (strlen(sRezFiles) == 0)
		return;

	int nIndex = 0;
	sRez[0] = '\0';

	if (sRezFiles[0])
	{
		char* pPos = sRezFiles;
		do
		{
			if (*pPos == '|')
			{
				sRez[nIndex] = '\0';
				if (strlen(sRez) > 0)
				{
					g_lstRezFiles.AddTail(sRez);
					sRez[0] = '\0';
				}
				nIndex = 0;
			}
			else
			{
				sRez[nIndex] = *pPos;
				nIndex++;
			}

			pPos++;
		}
		while (*pPos);
	}

	sRez[nIndex] = '\0';
	if (strlen(sRez) == 0)
		return;

	g_lstRezFiles.AddTail(sRez);
}


// *********************************************************************** //
//
//	ROUTINE:	GetCustomRezCommands()   [0x401d30]
//
//	PURPOSE:	Build the " -rez custom\<file>" command-line fragment.
//
// *********************************************************************** //

CString GetCustomRezCommands()
{
	CString csCommands;

	POSITION pos = g_lstRezFiles.GetHeadPosition();
	while (pos)
	{
		CString csRez = g_lstRezFiles.GetNext(pos);
		if (!csRez.IsEmpty())
		{
			csCommands += " -rez ";
			csCommands += "custom\\";
			csCommands += csRez;
		}
	}

	return csCommands;
}


// EOF...
