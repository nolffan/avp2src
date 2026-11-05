/*******************************************************************************
;
;	MODULE:			LAUNCHER (.H)
;
;	PURPOSE:		Launcher application main header file
;
;	HISTORY:		11/15/2000  [kml]  This file was created
;
;	COMMENT:		Copyright (c) 2000, Monolith Inc.
;
********************************************************************************/

#if !defined(AFX_LAUNCHER_H__1E6712C7_B0F3_11D4_A2BA_00010229388A__INCLUDED_)
#define AFX_LAUNCHER_H__1E6712C7_B0F3_11D4_A2BA_00010229388A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

// Includes
#include "resource.h"
#include "RegMgr32.h"

// Defines
#define ACTION_HOSTGAME				100
#define ACTION_JOINGAME				101
#define ACTION_FINDINTERNETSERVERS	102
#define ACTION_STANDALONESERVER		103
#define ACTION_INSTALL				104
#define ACTION_UNINSTALL			105
#define ACTION_PLAY					106

#define DETAIL_LOW					0
#define DETAIL_MEDIUM				1
#define DETAIL_HIGH					2

/////////////////////////////////////////////////////////////////////////////
// CLauncherApp:
// See Launcher.cpp for the implementation of this class
//

/////////////////////////////////////////////////////////////////////////////
// CLauncherApp:
// See Launcher.cpp for the implementation of this class
//
// AVP2.exe: vtable 0x422d08, theApp at 0x42de38 (0x164 bytes)

class CLauncherApp : public CWinApp
{
public:
	CLauncherApp();

	CRegMgr32 m_RegMgr;						// Registry manager object (0xc0)
	CBrush	m_BkBrush;						// Black background brush for edit and list boxes (0xcc)
	CFont	m_Font;							// Font for edit and list boxes (0xd4)
	char	m_chDrive;						// Which drive is the CDROM? (0xdc)
	BOOL	m_bInstalled;					// Is the game installed? (0xe0)
	BOOL	m_bRestoreDefaults;				// Should all default settings be restored?
	BOOL	m_bSaveCommands;				// Should the command-line commands be saved?
	BOOL	m_bUseDetailSettings;			// Should the detail settings be used?
	BOOL	m_bUseDisplaySettings;			// Should the display settings be used?
	BOOL	m_bDisableSound;				// Disables sound (0xf4)
	BOOL	m_bDisableMusic;				// Disables music
	BOOL	m_bDisableMovies;				// Disables movies
	BOOL	m_bDisableJoysticks;			// Disables Joysticks
	BOOL	m_bDisableTripleBuffering;		// Disables triple buffering
	BOOL	m_bDisableHardwareCursor;		// Disables the hardware cursor (0x108)
	CString	m_csUpdateCmdLine;				// "Update Command Line" from the registry (0x10c)
	CString	m_csContentCmdLine;				// "Content Command Line" from the registry (0x110)
	BOOL	m_bGoMultiHost;					// Jump right into a multiplayer host game (0x114)
	BOOL	m_bGoMultiJoin;					// Jump right into a multiplayer join game
	BOOL	m_bGoGame;						// Jump right into a single player game
	BOOL	m_bGoMultiFind;					// Jump right into find Internet servers
	BOOL	m_bGoServer;					// Jump right into the stand-alone server
	DWORD	m_dwNumLauncherRuns;			// How many times the launcher has been run (0x128)
	DWORD	m_dwNumGameRuns;				// How many times the game has been run
	DWORD	m_dwDetailSetting;				// Which detail level does the user want? (0x130)
	CString m_csAppName;					// Application name (0x134)
	CString m_csAppCD1Check;				// "Please insert CD #1"
	CString m_csAppCD2Check;				// "Please insert CD #2"
	CString m_csRezBase;					// Base rez directory/file (0x140)
	CString m_csSetupExe;					// Name of installer executable
	CString	m_csServerExe;					// Name of stand-alone server executable
	CString m_csLanguage;					// Language (for language rez files)
	CString m_csCDRoot;						// Root CDRom drive (0x150)
	CString m_csCommands;					// Command-line commands specified by the user (0x154)
	CString	m_csInstallDir;					// Directory where the game is installed
	CString m_csAppVersion;					// Application version (0x15c)
	BOOL	m_bCDLauncherRunning;			// The disc's own launcher is running (0x160)
#ifdef PRIMAL_HUNT
	CString	m_csPatchRez;					// PH: "PatchRez" from the registry (0x164)
	CString	m_csAvP2Name;					// PH: AvP2's registry key, "Aliens vs. Predator 2" (0x168)
	CString	m_csAvP2InstallDir;				// PH: where AvP2 is installed (0x16c)
#endif

	int		MessageBox(const char *szText, const char *szCaption, DWORD dwType = MB_OK);
	int		MessageBox(UINT nIDText, UINT nIDCaption, DWORD dwType = MB_OK);
	BOOL	GetRegistryBool(char *szField);
	DWORD	GetRegistryDWord(char *szField);
	BOOL	OpenRegistryKey();
#ifdef PRIMAL_HUNT
	BOOL	OpenAvP2RegistryKey();
#endif
	BOOL	CD_VerifyGame();
	BOOL	CD_ExistGame(BOOL & bDisk2);
	BOOL	CD_ExistFile(const char* sFile);
	char	CD_GetDriveLetterWithGame(BOOL & bDisk2);
	char	CD_TestDriveLetterForGameDisc1(char chDrive, BOOL & bDisk2);
	char	CD_TestDriveLetterForGameDisc2(char chDrive);
	CString GetAdvancedOptionsCommandLineString();
	BOOL	ReadConfigFile(const char* sFile);

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CLauncherApp)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

// Implementation

	//{{AFX_MSG(CLauncherApp)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

extern CLauncherApp theApp;


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_LAUNCHER_H__1E6712C7_B0F3_11D4_A2BA_00010229388A__INCLUDED_)
