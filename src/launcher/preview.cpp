// *********************************************************************** //
//
//	MODULE:		PREVIEW (.CPP)
//
//	PURPOSE:	The game previews carried on Disc 2 - a list of movies and
//				a list of playable demos, each described by an .ini file
//				whose sections name an entry and the file to shell-execute.
//
//				Shipped NOLF.exe v1.003: 0x4067c0 - 0x406e4c.
//
// *********************************************************************** //

#include "stdafx.h"
#include "regmgr.h"
#include "launcher.h"

BOOL CALLBACK PreviewDlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);


// *********************************************************************** //
//
//	ROUTINE:	CPreview::Init   [0x4067c0]
//
// *********************************************************************** //

BOOL CPreview::Init(const char* sName, const char* sFile, const char* sDir)
{
	m_csName = sName;
	m_csFile = sFile;
	m_csDir  = sDir;

	return(TRUE);
}


// *********************************************************************** //
//
//	ROUTINE:	CPreview::GetPath   [0x4067f0]
//
//	PURPOSE:	Directory + file, with the separator supplied if the
//				directory didn't come with one
//
// *********************************************************************** //

CString CPreview::GetPath()
{
	CString csPath = m_csDir;

	int nLen = csPath.GetLength();
	if (nLen <= 0 || csPath[nLen - 1] != '\\')
	{
		csPath += "\\";
	}

	csPath += m_csFile;

	return(csPath);
}


// *********************************************************************** //
//
//	ROUTINE:	CPreview::Run   [0x406890]
//
//	PURPOSE:	Hands the preview file to the shell
//
// *********************************************************************** //

BOOL CPreview::Run()
{
	CString csPath = GetPath();

	if (csPath.IsEmpty())
	{
		return(FALSE);
	}

	if (ShellExecute(NULL, NULL, csPath, NULL, NULL, SW_SHOWNORMAL) <= (HINSTANCE)32)
	{
		return(FALSE);
	}

	return(TRUE);
}


// *********************************************************************** //
//
//	ROUTINE:	CPreviewList::Init   [0x406930]
//
//	PURPOSE:	Reads every section of sIniFile (found in sDir, or the
//				current directory) into the list
//
// *********************************************************************** //

BOOL CPreviewList::Init(const char* sIniFile, const char* sDir)
{
	if (!sIniFile) return(FALSE);

	m_csName = sIniFile;
	m_csDir  = sDir;

	char sCurDir[256];
	if (m_csDir.IsEmpty())
	{
		GetCurrentDirectory(250, sCurDir);
		m_csDir = sCurDir;
	}

	CString csPath;
	csPath = m_csDir;

	int nLen = csPath.GetLength();
	if (nLen > 0 && csPath[nLen - 1] != '\\')
	{
		csPath += "\\";
	}

	csPath += m_csName;

	char sSections[1024];
	if (GetPrivateProfileSectionNames(sSections, 1020, csPath) <= 1)
	{
		return(FALSE);
	}

	char sName[64];
	char sFile[64];

	char* pPos = sSections;
	while (pPos)
	{
		if (!*pPos) break;

		CString csSection = pPos;

		GetPrivateProfileString(csSection, "Name", "", sName, 60, csPath);
		GetPrivateProfileString(csSection, "File", "", sFile, 60, csPath);

		if (!AddPreview(sName, sFile))
		{
			return(FALSE);
		}

		pPos += csSection.GetLength() + 1;
	}

	return(TRUE);
}


// *********************************************************************** //
//
//	ROUTINE:	CPreviewList::Term   [0x406b30]
//
// *********************************************************************** //

void CPreviewList::Term()
{
	POSITION pos = m_list.GetHeadPosition();
	while (pos)
	{
		CPreview* pPreview = (CPreview*)m_list.GetNext(pos);
		if (pPreview)
		{
			delete pPreview;
		}
	}

	m_list.RemoveAll();

	m_csName.Empty();
	m_csDir.Empty();
}


// *********************************************************************** //
//
//	ROUTINE:	CPreviewList::AddPreview   [0x406b80]
//
// *********************************************************************** //

CPreview* CPreviewList::AddPreview(const char* sName, const char* sFile)
{
	if (!sName) return(NULL);
	if (!sFile) return(NULL);

	CPreview* pPreview = new CPreview;

	if (!pPreview->Init(sName, sFile, m_csDir))
	{
		delete pPreview;
		return(NULL);
	}

	m_list.AddTail(pPreview);

	return(pPreview);
}


// *********************************************************************** //
//
//	ROUTINE:	CPreviewList::FillListBox   [0x406cc0]
//
// *********************************************************************** //

BOOL CPreviewList::FillListBox(HWND hList)
{
	if (!hList) return(FALSE);

	SendMessage(hList, LB_RESETCONTENT, 0, 0);

	POSITION pos = m_list.GetHeadPosition();
	while (pos)
	{
		CPreview* pPreview = (CPreview*)m_list.GetNext(pos);

		int nIndex = SendMessage(hList, LB_ADDSTRING, 0,
			(LPARAM)(const char*)pPreview->GetName());

		if (nIndex != LB_ERR)
		{
			SendMessage(hList, LB_SETITEMDATA, nIndex, (LPARAM)pPreview);
		}
	}

	return(TRUE);
}


// *********************************************************************** //
//
//	ROUTINE:	CPreviewList::RunSelected   [0x406d40]
//
// *********************************************************************** //

BOOL CPreviewList::RunSelected(HWND hList)
{
	if (!hList) return(FALSE);

	int nIndex = SendMessage(hList, LB_GETCURSEL, 0, 0);
	if (nIndex == LB_ERR) return(FALSE);

	CPreview* pPreview = (CPreview*)SendMessage(hList, LB_GETITEMDATA, nIndex, 0);
	if ((int)pPreview == LB_ERR) return(FALSE);

	if (!pPreview) return(FALSE);

	return(pPreview->Run() != 0);
}


// The destructor is defined in the class body: VC6 inlines it into
// AddPreview (which has an EH frame to hang the member lifetimes on) and
// emits the out-of-line COMDAT at 0x406db0 for CPreviewList::Term, which
// has no EH frame and so cannot take the inline copy.


// *********************************************************************** //
//
//	ROUTINE:	CPreview::GetName   [0x406e30]
//
// *********************************************************************** //

CString CPreview::GetName()
{
	return(m_csName);
}


// *********************************************************************** //
//
//	ROUTINE:	DoPreviewDialog   [0x4054b0]
//
//	PURPOSE:	Builds the movie and demo lists off Disc 2 and runs
//				whichever of the three preview dialogs applies
//
// *********************************************************************** //

BOOL DoPreviewDialog(HINSTANCE hInst, HWND hWnd)
{
	if (!hInst) return(FALSE);

	char chCdRom;

	{
		CWaitCursor wait;

		BOOL bOtherCd;
		chCdRom = FindCdRom(&bOtherCd);

		if (!chCdRom)
		{
			ErrorMsg(hWnd, 10, g_csCaption, MB_ICONEXCLAMATION);
			return(FALSE);
		}
	}

	CString csMovies;
	csMovies += chCdRom;
	csMovies += ":\\Previews\\Movies";

	CString csDemos;
	csDemos += chCdRom;
	csDemos += ":\\Previews\\Demos";

	{
		CWaitCursor wait;

		if (g_pMovies)
		{
			delete g_pMovies;
			g_pMovies = NULL;
		}

		g_pMovies = new CPreviewList;

		if (!g_pMovies->Init("Movies.ini", csMovies))
		{
			if (g_pMovies)
			{
				delete g_pMovies;
				g_pMovies = NULL;
			}
		}

		if (g_pDemos)
		{
			delete g_pDemos;
			g_pDemos = NULL;
		}

		g_pDemos = new CPreviewList;

		if (!g_pDemos->Init("Demos.ini", csDemos))
		{
			if (g_pDemos)
			{
				delete g_pDemos;
				g_pDemos = NULL;
			}
		}
	}

	if (!g_pDemos && !g_pMovies)
	{
		ErrorMsg(hWnd, 11, g_csCaption, MB_ICONINFORMATION);
		return(FALSE);
	}

	CString csDialog = "PREVIEWS";

	if (g_pDemos)
	{
		if (!g_pMovies) csDialog = "DEMOS";
	}
	else
	{
		if (g_pMovies) csDialog = "MOVIES";
	}

	DialogBoxParam(hInst, csDialog, hWnd, (DLGPROC)PreviewDlgProc, 0);

	if (g_pMovies)
	{
		delete g_pMovies;
		g_pMovies = NULL;
	}

	if (g_pDemos)
	{
		delete g_pDemos;
		g_pDemos = NULL;
	}

	return(TRUE);
}


// *********************************************************************** //
//
//	ROUTINE:	PreviewDlgProc   [0x405990]
//
//	PURPOSE:	Dialog procedure shared by the PREVIEWS, MOVIES and DEMOS
//				dialogs - two list boxes, each with a Play button
//
// *********************************************************************** //

BOOL CALLBACK PreviewDlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
		case WM_INITDIALOG:
		{
			HICON hIcon = LoadIcon(g_hInst, "ICON");
			if (hIcon)
			{
				SendMessage(hWnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
			}

			HWND hList = GetDlgItem(hWnd, 1006);
			if (hList && g_pMovies)
			{
				g_pMovies->FillListBox(hList);
			}

			hList = GetDlgItem(hWnd, 1008);
			if (hList && g_pDemos)
			{
				g_pDemos->FillListBox(hList);
			}

			return(TRUE);
		}

		case WM_COMMAND:
		{
			if (wParam == 1)
			{
				EndDialog(hWnd, 0);
				return(TRUE);
			}

			if (wParam == 2)
			{
				EndDialog(hWnd, 0);
				return(TRUE);
			}

			if (wParam == 1002)
			{
				if (!g_pMovies) break;

				g_pMovies->RunSelected(GetDlgItem(hWnd, 1006));
				break;
			}

			if (wParam == 1004)
			{
				if (!g_pDemos) break;

				g_pDemos->RunSelected(GetDlgItem(hWnd, 1008));
				break;
			}

			UINT nCode = HIWORD(wParam);
			if (nCode == LBN_DBLCLK)
			{
				if ((wParam & 0xffff) == 1006)
				{
					SendMessage(hWnd, WM_COMMAND, 1002, 0);
				}
				else if ((wParam & 0xffff) == 1008)
				{
					SendMessage(hWnd, WM_COMMAND, 1004, 0);
				}
			}

			break;
		}
	}

	return(FALSE);
}


// EOF...
