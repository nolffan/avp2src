/****************************************************************************
;
;	MODULE:			Launcher (.cpp)
;
;	PURPOSE:		Launcher application main header file
;
;	HISTORY:		11/16/2000 [kml] This file was created
;
;	COMMENT:		Copyright (c) 2000, Monolith Productions, Inc.
;
****************************************************************************/

#include "stdafx.h"
#include "Launcher.h"
#include "LauncherDlg.h"
#include "MessageBoxDlg.h"
#include "RezFind.h"
#include "Utils.h"

// The name of this executable on the disc (the autorun launcher)
#ifdef PRIMAL_HUNT
#define LAUNCHER_EXE	"PRIMALHUNT.EXE"
#else
#define LAUNCHER_EXE	"AVP2.EXE"
#endif

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CLauncherApp

BEGIN_MESSAGE_MAP(CLauncherApp, CWinApp)
	//{{AFX_MSG_MAP(CLauncherApp)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG
	ON_COMMAND(ID_HELP, CWinApp::OnHelp)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CLauncherApp construction

// AVP2.exe 0x404bd0
CLauncherApp::CLauncherApp()
{
	m_bDisableSound = FALSE;
	m_bDisableMusic = FALSE;
	m_bDisableMovies = FALSE;
	m_bDisableJoysticks = FALSE;
	m_bDisableTripleBuffering = FALSE;
	m_bDisableHardwareCursor = FALSE;
	m_bRestoreDefaults = FALSE;
	m_bSaveCommands = FALSE;
	m_dwNumLauncherRuns = 0;
	m_dwNumGameRuns = 0;
	m_chDrive = 0;
	m_bUseDisplaySettings = FALSE;
	m_bUseDetailSettings = FALSE;
	m_dwDetailSetting = DETAIL_MEDIUM;
	m_bGoMultiHost = FALSE;
	m_bGoMultiJoin = FALSE;
	m_bGoGame = FALSE;
	m_bGoMultiFind = FALSE;
	m_bGoServer = FALSE;
	m_bCDLauncherRunning = FALSE;
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CLauncherApp object

CLauncherApp theApp;

/////////////////////////////////////////////////////////////////////////////
// CLauncherApp initialization

// AVP2.exe 0x404f50
BOOL CLauncherApp::InitInstance()
{
	// Standard initialization
	// If you are not using these features and wish to reduce the size
	//  of your final executable, you should remove from the following
	//  the specific initialization routines you do not need.

#ifdef _AFXDLL
	Enable3dControls();			// Call this when using MFC in a shared DLL
#else
	Enable3dControlsStatic();	// Call this when linking to MFC statically
#endif

#ifdef AVP2SRC_MODERN
	// M-8: the game is where the program is (avp2cmds.txt, the archives,
	// lithtech.exe), however it was started
	CString csM8Dir;
	if(M8ProgramDir(csM8Dir))
		SetCurrentDirectory(csM8Dir);
#endif

	// Load some strings
#ifdef PRIMAL_HUNT
	m_csAvP2Name.LoadString(IDS_AVP2NAME);
#endif
	m_csAppName.LoadString(IDS_APPNAME);
	m_csAppCD1Check.LoadString(IDS_APPCD1CHECK);
	m_csAppCD2Check.LoadString(IDS_APPCD2CHECK);
	m_csRezBase.LoadString(IDS_REZBASE);
	m_csSetupExe.LoadString(IDS_SETUPEXE);
	m_csServerExe.LoadString(IDS_SERVEREXE);
	m_csLanguage.LoadString(IDS_LANGUAGE);
	m_csAppVersion.LoadString(IDS_APPVERSION);

	// Check if we are already running...
	char sPath[128];
	char sName[128];
	char sExt[32];
	char sExe[128];
	GetModuleFileName(NULL, sPath, 125);
	_splitpath(sPath, NULL, NULL, sName, sExt);
	sprintf(sExe, "%s%s", sName, sExt);
	if(ExistProcess(sExe, 2))
	{
		return FALSE;
	}

#ifdef AVP2SRC_MODERN
	// RETAIL BUG fixed (MODERN.md M-B106): the two checks look for this
	// directory's LITHTECH.EXE and SETUP.EXE, by their full paths, as the
	// directory was built for -- not for any process of those names
	char sDir[MAX_PATH] = { "" };
	DWORD dwDirLen = GetCurrentDirectory(MAX_PATH - 1, sDir);
	if(dwDirLen == 0 || dwDirLen >= MAX_PATH - 1) sDir[0] = '\0';
	if(sDir[0] && sDir[strlen(sDir) - 1] != '\\') strcat(sDir, "\\");

	char sFullExe[MAX_PATH + 16];
	sprintf(sFullExe, "%sLITHTECH.EXE", sDir);
	if(ExistProcess(sFullExe, 1))
	{
		return FALSE;
	}

	sprintf(sFullExe, "%sSETUP.EXE", sDir);
	if(ExistProcess(sFullExe, 1))
	{
		return FALSE;
	}
#else
	// RETAIL BUG: sDir is built and then not used -- the two checks below
	// look for the processes by their bare names
	char sDir[128] = { "" };
	GetCurrentDirectory(125, sDir);
	if(strlen(sDir) > 3) strcat(sDir, "\\");

	sprintf(sExe, "LITHTECH.EXE");
	if(ExistProcess(sExe, 1))
	{
		return FALSE;
	}

	sprintf(sExe, "SETUP.EXE");
	if(ExistProcess(sExe, 1))
	{
		return FALSE;
	}
#endif

	// Set the CD-ROM root directory string...
	BOOL bBogus;
	m_csCDRoot.Format("%c:\\", CD_GetDriveLetterWithGame(bBogus));

	// Was the disc's own launcher started (the autorun) and did it start us?
	if(!m_csCDRoot.IsEmpty())
	{
		if(ExistProcess(m_csCDRoot + LAUNCHER_EXE, 1))
		{
			m_bCDLauncherRunning = TRUE;
		}
	}

	// Load the registry settings
	m_bInstalled = FALSE;
	m_RegMgr.Init();

#ifdef PRIMAL_HUNT
	// PH: Primal Hunt runs on AvP2's install, so find that first
	BOOL bAvP2Installed = FALSE;
	if(OpenAvP2RegistryKey())
	{
		char p[MAX_PATH];
		DWORD dwSize = sizeof(char) * MAX_PATH;
		memset(p,0,dwSize);
		if(m_RegMgr.GetField("InstallDir",p,dwSize) && p[0])
		{
			m_csAvP2InstallDir = p;
			bAvP2Installed = TRUE;
		}
	}

#ifdef AVP2SRC_MODERN
	// M-8: the AvP2 beside us first (..\AVP2, as the build installs the
	// two games), the registry's only when there is none
	CString csM8AvP2;
	if(M8AvP2Beside(csM8AvP2))
	{
		m_csAvP2InstallDir = csM8AvP2;
		bAvP2Installed = TRUE;
	}
#endif

	if(!bAvP2Installed)
	{
		MessageBox(IDS_AVP2NOTINSTALLED, IDS_AVP2NOTFOUND, MB_OK | MB_ICONSTOP);
		return FALSE;
	}
#endif

#ifdef AVP2SRC_MODERN
	// M-8: the settings are read from the registry when the key is there,
	// and the rest is done without it
	BOOL bM8Key = OpenRegistryKey();
	if(TRUE)
#else
	if(OpenRegistryKey())
#endif
	{
		char p[MAX_PATH];
		DWORD dwSize = sizeof(char) * MAX_PATH;
		memset(p,0,dwSize);
		if(m_RegMgr.GetField("InstallDir",p,dwSize) && p[0])
		{
			// We're already installed
			m_bInstalled = TRUE;
			m_csInstallDir = p;

			// Run from the install directory, not the disc
			if(m_bCDLauncherRunning)
				SetCurrentDirectory(p);
		}

		m_dwNumLauncherRuns = GetRegistryDWord("Num Launcher Runs");
#ifdef AVP2SRC_MODERN
		// M-8: no install record -- the game is here; and with no
		// launcher record, only the game's own first run (autoexec.cfg's
		// NumRuns) asks for the detail level
		if(!m_bInstalled)
		{
			m_bInstalled = TRUE;
			m_csInstallDir = csM8Dir;
		}
		if(!bM8Key)
			m_dwNumLauncherRuns = 1;
#endif
		m_bDisableSound = GetRegistryBool("Disable Sound");
		m_bDisableMusic = GetRegistryBool("Disable Music");
		m_bDisableMovies = GetRegistryBool("Disable Movies");
		m_bDisableJoysticks = GetRegistryBool("Disable Joysticks");
		m_bDisableTripleBuffering = GetRegistryBool("Disable Triple Buffering");
		m_bDisableHardwareCursor = GetRegistryBool("Disable Hardware Cursor");

		// The command lines the updater and the content installers leave
		dwSize = sizeof(char) * MAX_PATH;
		memset(p,0,dwSize);
		if(m_RegMgr.GetField("Update Command Line",p,dwSize) && p[0])
		{
			m_csUpdateCmdLine = p;
		}

		dwSize = sizeof(char) * MAX_PATH;
		memset(p,0,dwSize);
		if(m_RegMgr.GetField("Content Command Line",p,dwSize) && p[0])
		{
			m_csContentCmdLine = p;
		}

#ifdef AVP2SRC_MODERN
#ifndef PRIMAL_HUNT
		// M-8: the 1.0.9.6 patch's archive, which its installer names in
		// the registry, from the game directory when the registry does not
		if(m_csUpdateCmdLine.IsEmpty() && M8FileExists("AVP2P1.REZ"))
			m_csUpdateCmdLine = "-rez AVP2P1.REZ";
#endif
#endif

#ifdef PRIMAL_HUNT
		// PH: a patch's own rez file
		dwSize = sizeof(char) * MAX_PATH;
		memset(p,0,dwSize);
		if(m_RegMgr.GetField("PatchRez",p,dwSize) && p[0])
		{
			m_csPatchRez = p;
		}
#endif

		// Check for a command string
		m_bSaveCommands = GetRegistryBool("Save Commands");
		if(m_bSaveCommands)
		{
			dwSize = sizeof(char) * MAX_PATH;
			memset(p,0,dwSize);
			if(m_RegMgr.GetField("Commands",p,dwSize) && p[0])
			{
				m_csCommands = p;
			}
		}

		ReadConfigFile("autoexec.cfg");

		// Find rez files
		FindRezFiles(CD_GetDriveLetterWithGame(bBogus),m_csRezBase);

		// Check for command line params
		CString csCmdLine = m_lpCmdLine;
		csCmdLine.MakeUpper();
		if(csCmdLine.Find("-GOMULTIHOST") != -1)
			m_bGoMultiHost = TRUE;
		if(csCmdLine.Find("-GOMULTIJOIN") != -1)
			m_bGoMultiJoin = TRUE;
		if(csCmdLine.Find("-GOMULTIFIND") != -1)
			m_bGoMultiFind = TRUE;
		if(csCmdLine.Find("-GOSERVER") != -1)
			m_bGoServer = TRUE;
		if(csCmdLine.Find("-GOGAME") != -1)
			m_bGoGame = TRUE;

		// Check for GameSpy command line params
		if(csCmdLine.Find("CONNECT") != -1)
			m_bGoMultiHost = TRUE;
	}

	// Change dialog colors
	SetDialogBkColor(RGB(0, 0, 0), RGB(255, 255, 255));

	// Make our custom brush and font
	m_BkBrush.CreateSolidBrush((RGB(0,0,0)));
	LOGFONT lf;
	memset(&lf,0,sizeof(LOGFONT));
	lf.lfHeight = 20;
	strcpy(lf.lfFaceName,"Arial Narrow");
	m_Font.CreateFontIndirect(&lf);

#ifdef AVP2SRC_MODERN
	// M-8: AVP2.exe / PrimalHunt.exe start the game as Launch would, with
	// no dialog, and wait for it
	if(g_bM8Direct)
	{
		CLauncherDlg dlg;
		dlg.M8LaunchDirect();
	}
	else
	{
#endif
	CLauncherDlg dlg;
	m_pMainWnd = &dlg;
	int nResponse = dlg.DoModal();
#ifdef AVP2SRC_MODERN
	}
#endif

	m_dwNumLauncherRuns++;

	// Write out some registry stuff
	if(m_bInstalled)
	{
		m_RegMgr.SetField("Num Launcher Runs", m_dwNumLauncherRuns);
		m_RegMgr.SetField("Disable Sound", m_bDisableSound);
		m_RegMgr.SetField("Disable Music", m_bDisableMusic);
		m_RegMgr.SetField("Disable Movies", m_bDisableMovies);
		m_RegMgr.SetField("Disable Joysticks", m_bDisableJoysticks);
		m_RegMgr.SetField("Disable Triple Buffering", m_bDisableTripleBuffering);
		m_RegMgr.SetField("Disable Hardware Cursor", m_bDisableHardwareCursor);

		// Set the language
		m_RegMgr.SetField("Language", (char *)(LPCTSTR)m_csLanguage);
	}

	// Clean up
	m_RegMgr.Term();
	m_BkBrush.DeleteObject();
	m_Font.DeleteObject();

#ifdef AVP2SRC_MODERN
	// M-8: the direct start exits with the game's exit code (what
	// CWinApp::ExitInstance returns)
	if(g_bM8Direct)
		m_msgCur.wParam = g_dwM8ExitCode;
#endif

	// Since the dialog has been closed, return FALSE so that we exit the
	//  application, rather than start the application's message pump.
	return FALSE;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CLauncherApp::MessageBox
//
//	PURPOSE:	Graphical MessageBox function
//
// ----------------------------------------------------------------------- //
// AVP2.exe 0x405740
int CLauncherApp::MessageBox(const char *szText, const char *szCaption, DWORD dwType)
{
	CMessageBoxDlg dlg;
	dlg.Init(szText,szCaption,dwType);
	return dlg.DoModal();
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CLauncherApp::MessageBox
//
//	PURPOSE:	Graphical MessageBox function
//
// ----------------------------------------------------------------------- //
// AVP2.exe 0x405970
int CLauncherApp::MessageBox(UINT nIDText, UINT nIDCaption, DWORD dwType)
{
	// Load the text and caption strings from the resource file
	CString csText,csCaption;
	if(!csText.LoadString(nIDText))
	{
		csText.Format("ERROR - Could not load string id: %d",nIDText);
		MessageBox(csText,"ERROR");
	}

#ifdef AVP2SRC_MODERN
	// RETAIL BUG fixed (MODERN.md M-B105): the caption's error goes in the
	// caption, as the text's does in the text, and the message is kept
	if(!csCaption.LoadString(nIDCaption))
	{
		csCaption.Format("ERROR - Could not load string id: %d",nIDCaption);
		MessageBox(csCaption,"ERROR");
	}
#else
	// RETAIL BUG: a caption that will not load overwrites the text, not the
	// caption, so the message itself is lost
	if(!csCaption.LoadString(nIDCaption))
	{
		csText.Format("ERROR - Could not load string id: %d",nIDCaption);
		MessageBox(csText,"ERROR");
	}
#endif
	
	return MessageBox(csText,csCaption,dwType);
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CLauncherApp::GetRegistryBool
//
//	PURPOSE:	Gets a BOOL value from the registry under the
//				key that's already open
//
// ----------------------------------------------------------------------- //
// AVP2.exe 0x405a70
BOOL CLauncherApp::GetRegistryBool(char *szField)
{
	DWORD dwResult;
	return(m_RegMgr.GetField(szField,&dwResult) && (dwResult == 1));
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CLauncherApp::GetRegistryDWord
//
//	PURPOSE:	Gets a DWORD value from the registry under the
//				key that's already open
//
// ----------------------------------------------------------------------- //
// AVP2.exe 0x405aa0
DWORD CLauncherApp::GetRegistryDWord(char *szField)
{
	DWORD dwResult = 0;
	m_RegMgr.GetField(szField,&dwResult);
	return dwResult;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CLauncherApp::CD_VerifyGame
//
//	PURPOSE:	Verifies that the correct CD is in the drive
//
// ----------------------------------------------------------------------- //
// AVP2.exe 0x405ad0
BOOL CLauncherApp::CD_VerifyGame()
{

	// Check if the game CD is already in the drive...
	BOOL bDisc2 = FALSE;
	{
		CWaitCursor wc;

		if(CD_ExistGame(bDisc2))
		{
			return(TRUE);
		}
	}

	// Ask the user to insert the game CD...
	BOOL bContinue = TRUE;
	while(bContinue)
	{
		DWORD dwMsg;
		if(bDisc2)
		{
			dwMsg = IDS_INSERTCD2;
		}
		else
		{
			dwMsg = IDS_INSERTCD;
		}

		int nRet = MessageBox(dwMsg, IDS_APPNAME, MB_OKCANCEL | MB_ICONEXCLAMATION);

		if(nRet == IDOK)
		{
			CWaitCursor wc;
			if(CD_ExistGame(bDisc2))
			{
				FindRezFiles(CD_GetDriveLetterWithGame(bDisc2),m_csRezBase);
				return(TRUE);
			}
		}
		else
		{
			return(FALSE);
		}
	}


	// If we get here, we couldn't verify the CD...
	return(FALSE);
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CLauncherApp::CD_ExistGame
//
//	PURPOSE:	Looks for the game CD
//
// ----------------------------------------------------------------------- //
// AVP2.exe 0x405bd0
BOOL CLauncherApp::CD_ExistGame(BOOL & bDisc2)
{
	// Use the new function to search across multiple drives...
	char chDrive = CD_GetDriveLetterWithGame(bDisc2);
	if(chDrive == 0) return(FALSE);


	// Update the rez files if necessary...
	char* sGameRez = GetGameRezFile();

	if(!sGameRez || sGameRez[0] == '\0')
	{
		FindRezFiles(chDrive,m_csRezBase);
	}

	// If we get here, it's the game cd...
	return(TRUE);
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CLauncherApp::CD_ExistFile
//
//	PURPOSE:	Determines if the given file exists
//
// ----------------------------------------------------------------------- //
// AVP2.exe 0x405c20
BOOL CLauncherApp::CD_ExistFile(const char* sFile)
{
	if(!sFile) return(FALSE);
	if(sFile[0] == '\0') return(FALSE);

	OFSTRUCT ofs;

	HFILE hFile = OpenFile(sFile, &ofs, OF_EXIST);

	return(hFile != HFILE_ERROR);
}


// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CLauncherApp::CD_GetDriveLetterWithGame
//
//	PURPOSE:	Gets the CD ROM drive letter with the game on it
//
// ----------------------------------------------------------------------- //

// AVP2.exe 0x405c70
char CLauncherApp::CD_GetDriveLetterWithGame(BOOL & bDisc2)
{
	// Check if we've already found the cd-rom drive...
	if(m_chDrive != 0)
	{
		return(m_chDrive);
	}

	bDisc2 = FALSE;

	// Look for a registry override setting...
	char sDrive[32];
	DWORD bufsize = 30;

	sDrive[0] = '\0';
	if(m_RegMgr.GetField("CdRom Drive", sDrive, bufsize))
	{
		if(sDrive[0] > 20)
		{
			char chDrive = sDrive[0];
	
			char cTest = CD_TestDriveLetterForGameDisc1(chDrive, bDisc2);
			if(cTest)
			{
				return (cTest);
			}
		}
	}

	// Try to find a CD ROM drive with the game EXE on it...
	char chDrive   = 'A';
	while(chDrive <= 'Z')
	{
		char cTest = CD_TestDriveLetterForGameDisc1(chDrive, bDisc2);
		if(cTest) return (cTest);

		chDrive++;
	}

	// If we get here, we didn't find one...
	return(0);
}


// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CLauncherApp::CD_TestDriveLetterForGameDisc1
//
//	PURPOSE:	Tests a potential CD ROM drive to see if it contains
//				the game disc 1
//
// ----------------------------------------------------------------------- //
// AVP2.exe 0x405d00
char CLauncherApp::CD_TestDriveLetterForGameDisc1(char chDrive, BOOL & bDisc2)
{
	char sDir[256];
	sprintf(sDir, "%c:\\", chDrive);
	if(GetDriveType(sDir) == DRIVE_CDROM)
	{
		char sBuf[256];
		sprintf(sBuf, "%c:%s", chDrive, m_csAppCD1Check);

		if(CD_ExistFile(sBuf))
		{
			m_chDrive = chDrive;
			return(chDrive);
		}	
	}

	// Check and see if they left disk 2 in by mistake...
	if(CD_TestDriveLetterForGameDisc2(chDrive))
	{
		bDisc2 = TRUE;
	}

	return (0);
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CLauncherApp::CD_TestDriveLetterForGameDisc2
//
//	PURPOSE:	Tests a potential CD ROM drive to see if it contains
//				the game disc 2
//
// ----------------------------------------------------------------------- //
// AVP2.exe 0x405db0
char CLauncherApp::CD_TestDriveLetterForGameDisc2(char chDrive)
{
	char sDir[256];
	sprintf(sDir, "%c:\\", chDrive);
	if(GetDriveType(sDir) == DRIVE_CDROM)
	{
		char sBuf[256];
		sprintf(sBuf, "%c:%s", chDrive, m_csAppCD2Check);

		if(CD_ExistFile(sBuf))
		{
			return(chDrive);
		}	
	}

	return (0);
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CLauncherApp::GetAdvancedOptionsCommandLineString()
//
//	PURPOSE:	Assembles the command line string based on the
//				advanced options
//
// ----------------------------------------------------------------------- //
// AVP2.exe 0x405e40
CString CLauncherApp::GetAdvancedOptionsCommandLineString()
{
	CString sAdv;

	if(m_bDisableMusic)
	{
		sAdv += " +DisableMusic 1";
	}
	else
	{
		sAdv += " +DisableMusic 0";
	}

	if(m_bDisableSound)
	{
		sAdv += " +DisableSound 1";
	}
	else
	{
		sAdv += " +DisableSound 0";
	}

	if(m_bDisableMovies)
	{
		sAdv += " +DisableMovies 1";
	}

	if(m_bDisableJoysticks)
	{
		sAdv += " +DisableJoystick 1";
	}

	if(m_bDisableTripleBuffering)
	{
		sAdv += " +EnableTripBuf 0";
	}
	else
	{
		sAdv += " +EnableTripBuf 1";
	}

	if(m_bDisableHardwareCursor)
	{
		sAdv += " +DisableHardwareCursor 1";
	}
	else
	{
		sAdv += " +DisableHardwareCursor 0";
	}

	return(sAdv);
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CLauncherApp::ReadConfigFile
//
//	PURPOSE:	Reads a config file mainly to set m_dwNumGameRuns
//
// ----------------------------------------------------------------------- //
// AVP2.exe 0x405f60
BOOL CLauncherApp::ReadConfigFile(const char* sFile)
{
	// Sanity checks...
	if(!sFile) return(FALSE);


	// Verify that the file exists...
	OFSTRUCT ofs;
	HFILE hFile = OpenFile(sFile, &ofs, OF_EXIST);
	if(hFile == HFILE_ERROR) return(FALSE);


	// Open the file...
	char strBuff[512];
	FILE* pFile = fopen(sFile, "r");
	if(!pFile) return(FALSE);


	// Declare some local variables we'll need...
	char sNumRuns[64];
	strcpy(sNumRuns, "0");


	// Scan for the parameters we are looking for...
	while(fgets(strBuff, 512, pFile))
	{
		char* ptr = strBuff;
		if(*ptr == '"') ptr++;

		if(strnicmp(ptr, "NumRuns", 7) == 0)
		{
			ptr += 7;
			while(*ptr == '"' || *ptr == ' ') ptr++;
			strcpy(sNumRuns, ptr);

			ptr = &sNumRuns[strlen(sNumRuns) - 1];
			while((*ptr == '"' || *ptr == '\n') && ptr > sNumRuns) ptr--;
			*(ptr + 1) = '\0';

			break;
		}
	}


	m_dwNumGameRuns = atoi(sNumRuns);


	// Close the file...
	fclose(pFile);


	// All done...
	return(TRUE);
}

#ifdef PRIMAL_HUNT
// PrimalHunt.exe 0x4062e0: Primal Hunt's own key
BOOL CLauncherApp::OpenRegistryKey()
{
	return(m_RegMgr.OpenKey(HKEY_LOCAL_MACHINE,"SOFTWARE","Third Law Interactive",(char *)(LPCTSTR)m_csAppName,(char *)(LPCTSTR)m_csAppVersion));
}

// PrimalHunt.exe 0x406320: AvP2's
BOOL CLauncherApp::OpenAvP2RegistryKey()
{
	return(m_RegMgr.OpenKey(HKEY_LOCAL_MACHINE,"SOFTWARE","Monolith Productions",(char *)(LPCTSTR)m_csAvP2Name,(char *)(LPCTSTR)m_csAppVersion));
}
#else
// AVP2.exe 0x4060b0
BOOL CLauncherApp::OpenRegistryKey()
{
	return(m_RegMgr.OpenKey(HKEY_LOCAL_MACHINE,"SOFTWARE","Monolith Productions",(char *)(LPCTSTR)m_csAppName,(char *)(LPCTSTR)m_csAppVersion));
}
#endif
