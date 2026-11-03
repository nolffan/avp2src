// *********************************************************************** //
//
//	MODULE:		DLGSOUND (.CPP)
//
//	PURPOSE:	3D sound provider setup.  Enumerating the Miles 3D
//				providers is slow and noisy, so it happens once, behind a
//				warning, and the result is cached in a linked list that
//				feeds the 3DSOUND dialog.
//
//				Shipped NOLF.exe v1.003: 0x407140 - 0x407b6f.
//
// *********************************************************************** //

#include "stdafx.h"
#include "mmsystem.h"
#include "mbstring.h"
#include "regmgr.h"
#include "launcher.h"


// The Miles Sound System entry points we use (mss32.dll)...

extern "C"
{
	void	__declspec(dllimport) __stdcall AIL_startup(void);
	void	__declspec(dllimport) __stdcall AIL_shutdown(void);
	void	__declspec(dllimport) __stdcall AIL_set_preference(unsigned int number, int value);
	int		__declspec(dllimport) __stdcall AIL_waveOutOpen(void** drvr, void* lphWaveOut, int nDeviceIndex, void* pFormat);
	void	__declspec(dllimport) __stdcall AIL_waveOutClose(void* drvr);
	int		__declspec(dllimport) __stdcall AIL_enumerate_3D_providers(int* pNext, int* pDest, char** ppName);
	int		__declspec(dllimport) __stdcall AIL_open_3D_provider(int nProvider);
	void	__declspec(dllimport) __stdcall AIL_close_3D_provider(int nProvider);
	void	__declspec(dllimport) __stdcall AIL_3D_provider_attribute(int nProvider, const char* sName, void* pValue);
}


// Globals...

int				g_n3DSamples = 16;			// 0x7b8
int				g_n2DSamples = 32;			// 0x7bc
int				g_nSoundReverb = 100;		// 0x7c0

CSoundProviders	g_SoundProviders;			// 0x4580
SOUNDPROVIDER*	g_pSoundProviders;			// 0x4584
int				g_nSoundProvider;			// 0x4588
char			g_sSoundProviderName[261];	// 0x458c
int				g_bSoundSet;				// 0x4694


// Prototypes...

int		EnumSoundProviders();
BOOL	CALLBACK Sound3DDlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void	Load3DSoundSettings(CRegMgr* pRegMgr);
void	Save3DSoundSettings(CRegMgr* pRegMgr);
void	Init3DSoundDlg(HWND hWnd);
void	Save3DSoundDlg(HWND hWnd);


// *********************************************************************** //
//
//	ROUTINE:	Do3DSoundSetup   [0x407180]
//
//	PURPOSE:	Enumerates the providers (once) and runs the 3DSOUND dialog
//
// *********************************************************************** //

int Do3DSoundSetup(HINSTANCE hInst, HWND hWnd, int bWarnIfNone)
{
	if (!hInst) return(0);

	int nCount = EnumSoundProviders();
	if (nCount < 0) return(0);

	if (nCount == 0)
	{
		if (bWarnIfNone)
		{
			ErrorMsg(hWnd, 65, MB_ICONINFORMATION);
		}

		return(0);
	}

	Load3DSoundSettings(g_pRegMgr);

	if (!DialogBoxParam(hInst, "3DSOUND", hWnd, (DLGPROC)Sound3DDlgProc, 0))
	{
		return(0);
	}

	Save3DSoundSettings(g_pRegMgr);

	g_bSoundSet = 1;

	return(1);
}


// *********************************************************************** //
//
//	ROUTINE:	EnumSoundProviders   [0x407200]
//
//	PURPOSE:	Number of providers found, or -1 if the user backed out of
//				the "this may take a minute" warning
//
// *********************************************************************** //

int EnumSoundProviders()
{
	if (g_pRegMgr)
	{
		if (!g_pRegMgr->Get("Sound Enum Warning", 0))
		{
			if (ErrorMsg(NULL, 67, MB_OKCANCEL | MB_ICONINFORMATION) != IDOK)
			{
				return(-1);
			}
		}

		g_pRegMgr->Set("Sound Enum Warning", 1);
	}

	if (!g_SoundProviders.Enumerate(&g_pSoundProviders, TRUE))
	{
		g_pSoundProviders = NULL;
		return(0);
	}

	if (!g_pSoundProviders) return(0);

	int nCount = 0;
	SOUNDPROVIDER* pProvider = g_pSoundProviders;
	do
	{
		pProvider = pProvider->pNext;
		nCount++;
	}
	while (pProvider);

	return(nCount);
}


// *********************************************************************** //
//
//	ROUTINE:	Sound3DDlgProc   [0x407280]
//
// *********************************************************************** //

BOOL CALLBACK Sound3DDlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
		case WM_INITDIALOG:
		{
			Init3DSoundDlg(hWnd);
			return(TRUE);
		}

		case WM_COMMAND:
		{
			if (wParam == 1)
			{
				Save3DSoundDlg(hWnd);
				EndDialog(hWnd, 1);
				return(TRUE);
			}
			else if (wParam == 2)
			{
				EndDialog(hWnd, 0);
				return(TRUE);
			}

			return(TRUE);
		}
	}

	return(FALSE);
}


// *********************************************************************** //
//
//	ROUTINE:	Load3DSoundSettings   [0x4072f0]
//
// *********************************************************************** //

void Load3DSoundSettings(CRegMgr* pRegMgr)
{
	if (!pRegMgr) return;

	g_nSoundProvider = pRegMgr->Get("Sound Provider", g_nSoundProvider);

	UINT dwSize = 261;
	pRegMgr->Get("Sound Provider Name", g_sSoundProviderName, dwSize, g_sSoundProviderName);

	g_n3DSamples   = pRegMgr->Get("Sound 3D Samples", g_n3DSamples);
	g_n2DSamples   = pRegMgr->Get("Sound 2D Samples", g_n2DSamples);
	g_nSoundReverb = pRegMgr->Get("Sound Reverb", g_nSoundReverb);
}


// *********************************************************************** //
//
//	ROUTINE:	LoadSoundSettings   [0x407380]
//
// *********************************************************************** //

void LoadSoundSettings(CRegMgr* pRegMgr)
{
	Load3DSoundSettings(pRegMgr);
}


// *********************************************************************** //
//
//	ROUTINE:	Save3DSoundSettings   [0x407390]
//
// *********************************************************************** //

void Save3DSoundSettings(CRegMgr* pRegMgr)
{
	if (!pRegMgr) return;

	pRegMgr->Set("Sound Provider", g_nSoundProvider);
	pRegMgr->Set("Sound Provider Name", g_sSoundProviderName);
	pRegMgr->Set("Sound 3D Samples", g_n3DSamples);
	pRegMgr->Set("Sound 2D Samples", g_n2DSamples);
	pRegMgr->Set("Sound Reverb", g_nSoundReverb);
}


// *********************************************************************** //
//
//	ROUTINE:	Init3DSoundDlg   [0x407400]
//
//	PURPOSE:	Fills the provider list and the sample/reverb controls
//
//	NEAR (451/451).  Identical instruction for instruction and identical
//	in length, and Ghidra decompiles the shipped function to the same C.
//	The shipped code puts hWnd in ebx and its materialized zero in ebp;
//	ours does the reverse, and all 31 differing bytes are modrm register
//	fields.  Pure allocation.  NOLF.exe was built with cl 8797/8569/8799,
//	which we do not hold, so this may not be
//	source-reachable.  Tried without effect: nSel, nIndex or pProvider at
//	function scope, a for loop, do/while, `if (hList && pProvider)`,
//	`if (hWnd)` wrapping the body, `== NULL` / `!= NULL` spellings,
//	`!stricmp`, a named stricmp result, a named sName or sNone local, a
//	hDlg copy of hWnd, the ternary BST_ form, and a g_nSoundReverb local.
//
// *********************************************************************** //

void Init3DSoundDlg(HWND hWnd)
{
	if (!hWnd) return;

	HWND hList = GetDlgItem(hWnd, 1075);

	if (hList)
	{
		CString csNone;
		csNone.LoadString(66);

		int nIndex = SendMessage(hList, LB_ADDSTRING, 0, (LPARAM)(const char*)csNone);
		if (nIndex != LB_ERR)
		{
			SendMessage(hList, LB_SETITEMDATA, nIndex, 0);
		}
	}

	int nSel = 0;

	if (hList)
	{
		SOUNDPROVIDER* pProvider = g_pSoundProviders;
		while (pProvider)
		{
			int nIndex = SendMessage(hList, LB_ADDSTRING, 0, (LPARAM)pProvider->sName);
			if (nIndex != LB_ERR)
			{
				SendMessage(hList, LB_SETITEMDATA, nIndex, pProvider->nType);
			}

			if (stricmp(pProvider->sName, g_sSoundProviderName) == 0)
			{
				nSel = nIndex;
			}

			pProvider = pProvider->pNext;
		}
	}

	SendMessage(hList, LB_SETCURSEL, nSel, 0);

	SetDlgItemInt(hWnd, 1076, g_n3DSamples, FALSE);
	SetDlgItemInt(hWnd, 1077, g_n2DSamples, FALSE);

	CheckDlgButton(hWnd, 1078, g_nSoundReverb == 0);
	CheckDlgButton(hWnd, 1079, g_nSoundReverb == 30);
	CheckDlgButton(hWnd, 1080, g_nSoundReverb == 60);
	CheckDlgButton(hWnd, 1081, g_nSoundReverb == 100);
	CheckDlgButton(hWnd, 1082, g_nSoundReverb == 125);
	CheckDlgButton(hWnd, 1083, g_nSoundReverb == 150);
}


// *********************************************************************** //
//
//	ROUTINE:	Save3DSoundDlg   [0x4075d0]
//
//	The _mbscpy is deliberate and is the only one in the launcher: every
//	other copy here is strcpy.  strcpy is an intrinsic and expands to
//	repnz scas + rep movs; _mbscpy is not, and calls the CRT.  The shipped
//	code calls, so the source called _mbscpy.
//
// *********************************************************************** //

void Save3DSoundDlg(HWND hWnd)
{
	if (!hWnd) return;

	HWND hList = GetDlgItem(hWnd, 1075);
	if (hList)
	{
		int nIndex = SendMessage(hList, LB_GETCURSEL, 0, 0);
		if (nIndex != LB_ERR)
		{
			int nData = SendMessage(hList, LB_GETITEMDATA, nIndex, 0);
			if (nData != LB_ERR)
			{
				g_nSoundProvider = nData;
			}

			char sName[261];
			if (SendMessage(hList, LB_GETTEXT, nIndex, (LPARAM)sName) != LB_ERR)
			{
				_mbscpy((unsigned char*)g_sSoundProviderName, (unsigned char*)sName);
			}
		}
	}

	g_n3DSamples = GetDlgItemInt(hWnd, 1076, NULL, FALSE);
	g_n2DSamples = GetDlgItemInt(hWnd, 1077, NULL, FALSE);

	if (IsDlgButtonChecked(hWnd, 1078) == BST_CHECKED) g_nSoundReverb = 0;
	if (IsDlgButtonChecked(hWnd, 1079) == BST_CHECKED) g_nSoundReverb = 30;
	if (IsDlgButtonChecked(hWnd, 1080) == BST_CHECKED) g_nSoundReverb = 60;
	if (IsDlgButtonChecked(hWnd, 1081) == BST_CHECKED) g_nSoundReverb = 100;
	if (IsDlgButtonChecked(hWnd, 1082) == BST_CHECKED) g_nSoundReverb = 125;
	if (IsDlgButtonChecked(hWnd, 1083) == BST_CHECKED) g_nSoundReverb = 150;
}


// *********************************************************************** //
//
//	ROUTINE:	Get3DSoundCommands   [0x407720]
//
// *********************************************************************** //

CString Get3DSoundCommands()
{
	if (!g_bSoundSet)
	{
		return(CString(""));
	}

	CString csCommands;
	CString csTemp;

	csTemp.Format("++3dsoundprovider %i", g_nSoundProvider);
	csCommands += csTemp;
	csCommands += " ";

	csTemp.Format("++3dsoundprovidername \"%s\"", g_sSoundProviderName);
	csCommands += csTemp;
	csCommands += " ";

	csTemp.Format("++max3dvoices %i ++maxswvoices %i", g_n3DSamples, g_n2DSamples);
	csCommands += csTemp;
	csCommands += " ";

	csTemp.Format("++reverblevel %f", g_nSoundReverb * 0.01f);
	csCommands += csTemp;
	csCommands += " ";

	return(csCommands);
}


// *********************************************************************** //
//
//	ROUTINE:	CSoundProviders::CSoundProviders   [0x4078b0]
//
// *********************************************************************** //

CSoundProviders::CSoundProviders()
{
	m_pProviders = NULL;
}


// *********************************************************************** //
//
//	ROUTINE:	CSoundProviders::~CSoundProviders   [0x4078c0]
//
// *********************************************************************** //

CSoundProviders::~CSoundProviders()
{
	FreeProviders(m_pProviders);

	m_pProviders = NULL;
}


// *********************************************************************** //
//
//	ROUTINE:	CSoundProviders::SetPreferences   [0x4078e0]
//
// *********************************************************************** //

void CSoundProviders::SetPreferences()
{
	AIL_set_preference(27, 1);
	AIL_set_preference(15, 0);
	AIL_set_preference(33, 1);
	AIL_set_preference(40, 0x40000);
}


// *********************************************************************** //
//
//	ROUTINE:	CSoundProviders::Enumerate   [0x407910]
//
//	PURPOSE:	Walks the Miles 3D providers once, tagging each with the
//				engine's provider number and whether it supports EAX
//
//	The strncpy destination is a named local (sDest).  Written as
//	pNew->sName twice, VC6 keeps pNew live across the strncpy call and
//	moves the provider type into edi (565 bytes); the shipped code keeps
//	only &pNew->sName in esi and stores the terminator through it
//	(0x407aea / 0x407af7).
//
// *********************************************************************** //

BOOL CSoundProviders::Enumerate(SOUNDPROVIDER** ppProviders, BOOL bCheckEAX)
{
	PlaySound(NULL, NULL, SND_PURGE);

	if (!m_pProviders)
	{
		void* hDriver;

		AIL_startup();

		SetPreferences();

		if (AIL_waveOutOpen(&hDriver, NULL, -1, NULL))
		{
			AIL_shutdown();
			return(FALSE);
		}

		int nNext = 0;
		int nProvider;
		char* sName;

		while (AIL_enumerate_3D_providers(&nNext, &nProvider, &sName))
		{
			int nType;
			int bEAX = 0;

			if (strcmp(sName, "Microsoft DirectSound3D with Creative Labs EAX(TM)") == 0)
			{
				nType = 2;
			}
			else if (strstr(sName, "Aureal A3D Interactive(TM)"))
			{
				nType = 4;
			}
			else if (strstr(sName, "Microsoft DirectSound3D hardware support"))
			{
				nType = 1;
			}
			else if (strcmp(sName, "Microsoft DirectSound3D software emulation") == 0)
			{
				nType = 3;
			}
			else if (strstr(sName, "Intel Realistic Sound Experience(TM)"))
			{
				nType = 5;
			}
			else
			{
				nType = strstr(sName, "Miles 2D Stereo Positional Audio") ? 6 : 7;
			}

			if (bCheckEAX)
			{
				if (AIL_open_3D_provider(nProvider)) continue;

				int nEnvironment;
				AIL_3D_provider_attribute(nProvider, "EAX environment selection", &nEnvironment);

				if (nEnvironment != -1)
				{
					bEAX = 1;
				}

				AIL_close_3D_provider(nProvider);
			}

			SOUNDPROVIDER* pNew = new SOUNDPROVIDER;

			pNew->pNext = m_pProviders;
			m_pProviders = pNew;
			pNew->bEAX = bEAX;
			m_pProviders->nType = nType;
			pNew->nProvider = nProvider;

			char* sDest = pNew->sName;
			strncpy(sDest, sName, 260);
			sDest[260] = '\0';
		}

		AIL_waveOutClose(hDriver);
		AIL_shutdown();
	}

	*ppProviders = m_pProviders;

	return(TRUE);
}


// *********************************************************************** //
//
//	ROUTINE:	CSoundProviders::FreeProviders   [0x407b50]
//
// *********************************************************************** //

void CSoundProviders::FreeProviders(SOUNDPROVIDER* pProviders)
{
	while (pProviders)
	{
		SOUNDPROVIDER* pNext = pProviders->pNext;
		delete pProviders;
		pProviders = pNext;
	}
}


// EOF...
