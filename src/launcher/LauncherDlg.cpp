/*******************************************************************************
;
;	MODULE:			LAUNCHERDLG (.CPP)
;
;	PURPOSE:		Launcher main dialog class
;
;	HISTORY:		11/15/2000  [kml]  This file was created
;
;	COMMENT:		Copyright (c) 2000, Monolith Inc.
;
********************************************************************************/


#include "stdafx.h"
#include "Launcher.h"
#include "LauncherDlg.h"
#include "OptionsDlg.h"
#include "DisplayDlg.h"
#include "DisplayMgr.h"
#include "RezFind.h"
#include "DetailSettingsDlg.h"
#include "PlaySound.h"
#include <time.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////
// Button and image coordinates
/////////////////////////////////////////////

#define LD_CLOSE_X			505
#define LD_CLOSE_Y			1
#define LD_MINIMIZE_X		485
#define LD_MINIMIZE_Y		1
#define LD_PLAY_X			417
#define LD_PLAY_Y			19
#define LD_SERVER_X			417
#define LD_SERVER_Y			55
#define LD_DISPLAY_X		417
#define LD_DISPLAY_Y		91
#define LD_OPTIONS_X		417
#define LD_OPTIONS_Y		127
#define LD_QUIT_X			417
#define LD_QUIT_Y			199
#define LD_ANIBUTTONS_X		387
#define LD_ANIBUTTONS_Y		19

extern CDisplayModes* g_pDisplayModes;

/////////////////////////////////////////////////////////////////////////////
// CLauncherDlg dialog

// AVP2.exe 0x406120
CLauncherDlg::CLauncherDlg(CWnd* pParent /*=NULL*/)
	: CMoveDialog(CLauncherDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CLauncherDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	// Note that LoadIcon does not require a subsequent DestroyIcon in Win32
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

// AVP2.exe 0x4061e0
void CLauncherDlg::DoDataExchange(CDataExchange* pDX)
{
	CMoveDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CLauncherDlg)
	DDX_Control(pDX, IDC_ANIMATE2, m_aniButtons);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CLauncherDlg, CMoveDialog)
	//{{AFX_MSG_MAP(CLauncherDlg)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_CLOSE, OnClose)
	ON_BN_CLICKED(IDC_MINIMIZE, OnMinimize)
	ON_BN_CLICKED(IDC_PLAY, OnPlay)
	ON_BN_CLICKED(IDC_SERVER, OnServer)
	ON_BN_CLICKED(IDC_OPTIONS, OnOptions)
	ON_BN_CLICKED(IDC_DISPLAY, OnDisplay)
	ON_BN_CLICKED(IDC_QUIT, OnQuit)
	ON_COMMAND(ID_HELP, OnHelp)
	ON_WM_DESTROY()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CLauncherDlg message handlers

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CLauncherDlg::OnInitDialog (AVP2.exe 0x406220)
//
//	PURPOSE:	Initialization
//
// ----------------------------------------------------------------------- //
BOOL CLauncherDlg::OnInitDialog()
{
	//***************************************************
	// Do this BEFORE calling CMoveDialog::OnInitDialog()
	//
	SetBackGround(IDB_BACKGROUND);
	//***************************************************

	CMoveDialog::OnInitDialog();

	CenterWindow();

	// Here's where we define what buttons go in our dialog
	AddButton(IDC_CLOSE,LD_CLOSE_X,LD_CLOSE_Y);
	AddButton(IDC_MINIMIZE,LD_MINIMIZE_X,LD_MINIMIZE_Y);
	AddButton(IDC_SERVER,LD_SERVER_X,LD_SERVER_Y);
	AddButton(IDC_DISPLAY,LD_DISPLAY_X,LD_DISPLAY_Y);
	AddButton(IDC_OPTIONS,LD_OPTIONS_X,LD_OPTIONS_Y);
	AddButton(IDC_QUIT,LD_QUIT_X,LD_QUIT_Y);

	// Figure out if we need to load the "Play" button or the "Install" button
	if(theApp.m_bInstalled)
	{
		CWnd* pWnd = GetDlgItem(IDC_PLAY);
		if(pWnd)
			pWnd->SetWindowText("Play");

		// Check for command-line params that automatically launch the game
		if(theApp.m_bGoMultiHost)
		{
			if(LaunchGame(ACTION_HOSTGAME))
			{
				EndDialog(ACTION_HOSTGAME);
				return TRUE;
			}
		}

		if(theApp.m_bGoMultiJoin)
		{
			if(LaunchGame(ACTION_JOINGAME))
			{
				EndDialog(ACTION_JOINGAME);
				return TRUE;
			}
		}

		if(theApp.m_bGoMultiFind)
		{
			if(LaunchGame(ACTION_FINDINTERNETSERVERS))
			{
				EndDialog(ACTION_FINDINTERNETSERVERS);
				return TRUE;
			}
		}

		if(theApp.m_bGoGame)
		{
			if(LaunchGame(ACTION_PLAY))
			{
				EndDialog(ACTION_PLAY);
				return TRUE;
			}
		}

		if(theApp.m_bGoServer)
		{
			if(LaunchGame(ACTION_STANDALONESERVER))
			{
				EndDialog(ACTION_STANDALONESERVER);
				return TRUE;
			}
		}
	}
	else
	{
		GetDlgItem(IDC_SERVER)->EnableWindow(FALSE);
		GetDlgItem(IDC_DISPLAY)->EnableWindow(FALSE);
		GetDlgItem(IDC_OPTIONS)->EnableWindow(FALSE);
	}

	// We add this guy last because the images loaded by this function are
	// dependent on the text of the button -- which may have been altered above
	AddButton(IDC_PLAY,LD_PLAY_X,LD_PLAY_Y);

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	// Set window title
	SetWindowText("Aliens vs. Predator");

	// Set up our animation control
	if(!m_aniButtons.Open(IDR_AVI_BUTTONS))
	{
		MessageBox("AVI Open Error!");
	}
	m_aniButtons.SetWindowPos(NULL,LD_ANIBUTTONS_X,LD_ANIBUTTONS_Y,0,0,SWP_NOZORDER | SWP_NOSIZE);
	m_aniButtons.Play(0,-1,-1);

	// Play one of the intro sounds
	srand(time(NULL));
	switch(rand() % 3)
	{
		case 0:
		{
			PlaySound(IDR_INTRO);
			break;
		}

		case 1:
		{
			PlaySound(IDR_INTRO2);
			break;
		}

		case 2:
		{
			PlaySound(IDR_INTRO3);
			break;
		}

		default:
		{
			PlaySound(IDR_INTRO);
			break;
		}
	}

	return TRUE;  // return TRUE  unless you set the focus to a control
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CLauncherDlg::OnPaint (AVP2.exe 0x406500)
//
//	PURPOSE:	Paints the dialog
//
// ----------------------------------------------------------------------- //
void CLauncherDlg::OnPaint() 
{
	CPaintDC dc(this); // device context for painting

	//***************************************************
	// Call this BEFORE doing any painting on the dialog!
	//
	PaintBackGround(&dc);
	//***************************************************

	if (IsIconic())
	{
		SendMessage(WM_ICONERASEBKGND, (WPARAM) dc.GetSafeHdc(), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CMoveDialog::OnPaint();
	}
}

// The system calls this to obtain the cursor to display while the user drags
//  the minimized window.
// AVP2.exe 0x4065f0
HCURSOR CLauncherDlg::OnQueryDragIcon()
{
	return (HCURSOR) m_hIcon;
}

// AVP2.exe 0x407740 (one with the vcall thunk to OnCancel)
void CLauncherDlg::OnClose() 
{
	// Close us
	OnCancel();
}

// AVP2.exe 0x406600
void CLauncherDlg::OnMinimize() 
{
	// Minimize	us
	ShowWindow(SW_MINIMIZE);
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CLauncherDlg::OnPlay (AVP2.exe 0x406610)
//
//	PURPOSE:	Play button handler
//
// ----------------------------------------------------------------------- //
void CLauncherDlg::OnPlay() 
{
	// Launch the game or the installer
	if(theApp.m_bInstalled)
	{
#ifdef PRIMAL_HUNT
		// PH: no disc check
		if(LaunchGame(ACTION_PLAY))
		{
			EndDialog(ACTION_PLAY);
		}
#else
#ifdef AVP2SRC_MODERN
		// M-8: no disc check (MODERN.md, the rules)
		if(LaunchGame(ACTION_PLAY))
		{
			EndDialog(ACTION_PLAY);
		}
#else
		if(theApp.CD_VerifyGame())
		{
			if(LaunchGame(ACTION_PLAY))
			{
				EndDialog(ACTION_PLAY);
			}
		}
#endif
#endif
	}
	else
	{
		if(WinExec(theApp.m_csSetupExe,SW_SHOW) > 31)
		{
			// Just let it happen...
			EndDialog(ACTION_INSTALL);
		}
		else
		{
			// Couldn't find the setup exe
			theApp.MessageBox(IDS_CANTLAUNCHSETUP, IDS_APPNAME, MB_OK | MB_ICONSTOP);
		}
	}
}

// AVP2.exe 0x406990: the Enter key plays
void CLauncherDlg::OnOK() 
{
	OnPlay();
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CLauncherDlg::OnOptions (AVP2.exe 0x4066a0)
//
//	PURPOSE:	Options button handler
//
// ----------------------------------------------------------------------- //
void CLauncherDlg::OnOptions() 
{
	// See if we need to display the warning
	if(!theApp.GetRegistryBool("OptionsWarning"))
	{
		if(theApp.MessageBox( IDS_OPTIONS_WARNING, IDS_APPNAME, MB_OKCANCEL ) == IDCANCEL)
		{
			return;
		}

		theApp.m_RegMgr.SetField("OptionsWarning",1);
	}

	COptionsDlg dlg;
	dlg.DoModal();
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CLauncherDlg::OnDisplay (AVP2.exe 0x406880)
//
//	PURPOSE:	Display button handler
//
// ----------------------------------------------------------------------- //
void CLauncherDlg::OnDisplay() 
{
	// See if we need to display the warning
	if(!theApp.GetRegistryBool("DisplayWarning"))
	{
		if(theApp.MessageBox( IDS_DISPLAY_WARNING, IDS_APPNAME, MB_OKCANCEL ) == IDCANCEL)
		{
			return;
		}

		theApp.m_RegMgr.SetField("DisplayWarning",1);
	}

	CDisplayDlg dlg;
	if(dlg.DoModal() == IDOK)
	{
		theApp.m_bUseDisplaySettings = TRUE;
	}
}

// AVP2.exe 0x406680
void CLauncherDlg::OnServer()
{
	if( LaunchGame( ACTION_STANDALONESERVER ) )
	{
		EndDialog( ACTION_STANDALONESERVER );
	}
}

// AVP2.exe 0x407740
void CLauncherDlg::OnQuit() 
{
	OnCancel();
}

// AVP2.exe 0x406980: F1 does nothing
void CLauncherDlg::OnHelp() 
{
}

// AVP2.exe 0x4069a0
BOOL CLauncherDlg::OnCommand(WPARAM wParam, LPARAM lParam) 
{
	return CMoveDialog::OnCommand(wParam, lParam);
}

// AVP2.exe 0x4069b0
void CLauncherDlg::OnDestroy() 
{
	CMoveDialog::OnDestroy();

	if(g_pDisplayModes)
	{
		delete g_pDisplayModes;
		g_pDisplayModes = NULL;
	}
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CLauncherDlg::LaunchGame (AVP2.exe 0x406a40)
//
//	PURPOSE:	Launches the game: writes the command line to avp2cmds.txt
//				and runs lithtech.exe with -cmdfile
//
// ----------------------------------------------------------------------- //
BOOL CLauncherDlg::LaunchGame(DWORD dwAction)
{
	if(dwAction == ACTION_STANDALONESERVER)
	{
		if(WinExec(theApp.m_csServerExe,SW_SHOW) > 31)
		{
			// Just let it happen...
			return TRUE;
		}
		else
		{
			theApp.MessageBox(IDS_CANTLAUNCHSERVER, IDS_APPNAME, MB_OK | MB_ICONSTOP);
			return FALSE;
		}
	}

	// See if we need to bring up the detail settings dialog box
#ifdef AVP2SRC_MODERN
	// M-8: not for the direct start, which keeps the game's own settings
	if(!g_bM8Direct && ((theApp.m_dwNumGameRuns <= 0) || (theApp.m_dwNumLauncherRuns <= 0) || (theApp.m_bRestoreDefaults)))
#else
	if((theApp.m_dwNumGameRuns <= 0) || (theApp.m_dwNumLauncherRuns <= 0) || (theApp.m_bRestoreDefaults))
#endif
	{
		CDetailSettingsDlg dlg;
		int nRet = dlg.DoModal();
		if(nRet == IDCANCEL)
		{
			return FALSE;
		}
		else
		{
			theApp.m_bUseDetailSettings = TRUE;
		}
	}

	char* sGameRez = GetGameRezFile();
	char* sSoundRez = GetSoundRezFile();
	char* sAlienRez = GetAlienRezFile();
	char* sMarineRez = GetMarineRezFile();
	char* sPredatorRez = GetPredatorRezFile();
	char* sMultiRez = GetMultiRezFile();
#ifdef PRIMAL_HUNT
	char* sAvP2XRez = GetAvP2XRezFile();	// PH: (not used)
#endif

#ifdef AVP2SRC_MODERN
#ifdef PRIMAL_HUNT
	// M-8: Primal Hunt's worlds here and AvP2's archives beside it
	if (!M8FileExists("AVP2X.REZ") || !M8FileExists(M8Path(theApp.m_csAvP2InstallDir, "AVP2.REZ")))
#else
	if (!sGameRez || sGameRez[0] == '\0')
#endif
#else
	if (!sGameRez || sGameRez[0] == '\0')
#endif
	{
		theApp.MessageBox(IDS_CANTFINDREZFILE, IDS_APPNAME, MB_OK | MB_ICONEXCLAMATION);
		return FALSE;
	}

	CString sNewCmdLine;

	// Set the window title...
	sNewCmdLine += "-windowtitle \"";
	sNewCmdLine += theApp.m_csAppName;
	sNewCmdLine += "\"";

#ifdef PRIMAL_HUNT
#ifdef AVP2SRC_MODERN
	// M-8: the rez stack of build/avp2cmds.sh -- AvP2's archives from
	// AvP2's directory, then Primal Hunt's own, then a patch's, custom last
	static const char* const s_aM8AvP2Rez[] = { "AVP2.REZ", "SOUNDS.REZ", "ALIEN.REZ", "MARINE.REZ", "PREDATOR.REZ", "MULTI.REZ" };
	static const char* const s_aM8Rez[] = { "AVP2P.REZ", "AVP2P1.REZ", "AVP2X.REZ", "MULTI.REZ", "AVP2DLL.REZ", "AVP2L.REZ", "DIALOGUE.REZ" };
	int iM8;
	for (iM8 = 0; iM8 < sizeof(s_aM8AvP2Rez) / sizeof(s_aM8AvP2Rez[0]); iM8++)
	{
		CString csRez = M8Path(theApp.m_csAvP2InstallDir, s_aM8AvP2Rez[iM8]);
		if (M8FileExists(csRez))
		{
			sNewCmdLine += " -rez \"";
			sNewCmdLine += csRez;
			sNewCmdLine += "\"";
		}
	}
	for (iM8 = 0; iM8 < sizeof(s_aM8Rez) / sizeof(s_aM8Rez[0]); iM8++)
	{
		if (M8FileExists(s_aM8Rez[iM8]))
		{
			sNewCmdLine += " -rez ";
			sNewCmdLine += s_aM8Rez[iM8];
		}
	}

	if (!theApp.m_csPatchRez.IsEmpty())
	{
		sNewCmdLine += " ";
		sNewCmdLine += theApp.m_csPatchRez;
	}

	sNewCmdLine += " -rez custom";
#else
	// Set rez files... (PH: quoted, and AvP2's multiplayer, dialogue and
	// patch rez files)
	sNewCmdLine += " -rez \"";
	sNewCmdLine += sGameRez;
	sNewCmdLine += "\"";

	if (sSoundRez && sSoundRez[0] != '\0')
	{
		sNewCmdLine += " -rez \"";
		sNewCmdLine += sSoundRez;
		sNewCmdLine += "\"";
	}

	if (sAlienRez && sAlienRez[0] != '\0')
	{
		sNewCmdLine += " -rez \"";
		sNewCmdLine += sAlienRez;
		sNewCmdLine += "\"";
	}

	if (sMarineRez && sMarineRez[0] != '\0')
	{
		sNewCmdLine += " -rez \"";
		sNewCmdLine += sMarineRez;
		sNewCmdLine += "\"";
	}

	if (sPredatorRez && sPredatorRez[0] != '\0')
	{
		sNewCmdLine += " -rez \"";
		sNewCmdLine += sPredatorRez;
		sNewCmdLine += "\"";
	}

	if (sMultiRez && sMultiRez[0] != '\0')
	{
		sNewCmdLine += " -rez \"";
		sNewCmdLine += sMultiRez;
		sNewCmdLine += "\"";
	}

	sNewCmdLine += " -rez multi.rez";

	sNewCmdLine += " -rez \"";	// " -rez <game>dll.rez" (DLL rez file)
	sNewCmdLine += theApp.m_csRezBase;
	sNewCmdLine += "dll.rez\"";

	sNewCmdLine += " -rez \"";	// " -rez <game>l.rez"  (language rez file)
	sNewCmdLine += theApp.m_csRezBase;
	sNewCmdLine += "l.rez\"";

	sNewCmdLine += " -rez dialogue.rez";

	sNewCmdLine += " ";
	sNewCmdLine += theApp.m_csPatchRez;

	sNewCmdLine += " -rez custom";
#endif

#else
	// Set rez files...
	sNewCmdLine += " -rez ";
	sNewCmdLine += sGameRez;

	if (sSoundRez && sSoundRez[0] != '\0')
	{
		sNewCmdLine += " -rez ";
		sNewCmdLine += sSoundRez;
	}

	if (sAlienRez && sAlienRez[0] != '\0')
	{
		sNewCmdLine += " -rez ";
		sNewCmdLine += sAlienRez;
	}

	if (sMarineRez && sMarineRez[0] != '\0')
	{
		sNewCmdLine += " -rez ";
		sNewCmdLine += sMarineRez;
	}

	if (sPredatorRez && sPredatorRez[0] != '\0')
	{
		sNewCmdLine += " -rez ";
		sNewCmdLine += sPredatorRez;
	}

	if (sMultiRez && sMultiRez[0] != '\0')
	{
		sNewCmdLine += " -rez ";
		sNewCmdLine += sMultiRez;
	}

	sNewCmdLine += " -rez ";	// " -rez <game>dll.rez" (DLL rez file)
	sNewCmdLine += theApp.m_csRezBase;
	sNewCmdLine += "dll.rez";

	sNewCmdLine += " -rez ";	// " -rez <game>l.rez"  (language rez file)
	sNewCmdLine += theApp.m_csRezBase;
	sNewCmdLine += "l.rez";

	sNewCmdLine += " -rez custom";

	sNewCmdLine += " -rez ";	// " -rez <game>p.rez"	(patch rez file)
	sNewCmdLine += theApp.m_csRezBase;
	sNewCmdLine += "p.rez";

	sNewCmdLine += " -rez ";	// " -rez <game>p2.rez"  (patch2 rez file)
	sNewCmdLine += theApp.m_csRezBase;
	sNewCmdLine += "p2.rez";

#endif

	// What the updater and the content installers left in the registry...
	if (!theApp.m_csUpdateCmdLine.IsEmpty())
	{
		sNewCmdLine += " ";
		sNewCmdLine += theApp.m_csUpdateCmdLine;
	}

	if (!theApp.m_csContentCmdLine.IsEmpty())
	{
		sNewCmdLine += " ";
		sNewCmdLine += theApp.m_csContentCmdLine;
	}

	// Set multiplayer stuff...
	switch(dwAction)
	{
		case ACTION_HOSTGAME:
		{
			sNewCmdLine += " +launcheraction host";
			break;
		}

		case ACTION_JOINGAME:
		{
			sNewCmdLine += " +launcheraction join";
			break;
		}
	}

	// Add the advanced options stuff as necessary...
	CString sAdv = theApp.GetAdvancedOptionsCommandLineString();
	if (!sAdv.IsEmpty())
	{
		sNewCmdLine += " ";
		sNewCmdLine += sAdv;
	}

	// Add display settings to the command-line if requested...
	if (theApp.m_bUseDisplaySettings && g_pDisplayModes)
	{
		CString sDisplay = g_pDisplayModes->GetCommands();
		if (!sDisplay.IsEmpty())
		{
			sNewCmdLine += " ";
			sNewCmdLine += sDisplay;
		}
	}

	// Add the detail setting if necessary...
	if (theApp.m_bUseDetailSettings)
	{
		CString sDetail = " ++PerformanceConfig ";

#ifdef AVP2SRC_MODERN
		// RETAIL BUG fixed (MODERN.md M-B102): Low is the low-detail
		// profile, High the high one
		switch(theApp.m_dwDetailSetting)
		{
			case DETAIL_LOW:
			{
				sDetail += ".DefaultLow";
				break;
			}

			case DETAIL_MEDIUM:
			{
				sDetail += ".DefaultMid";
				break;
			}

			case DETAIL_HIGH:
			{
				sDetail += ".DefaultHigh";
				break;
			}

			default:
			{
				sDetail += ".DefaultMid";
				break;
			}
		}
#else
		// RETAIL BUG: low and high are the wrong way round -- the detail
		// dialog's "Low" button gives the game the high-detail profile and
		// "High" the low one
		switch(theApp.m_dwDetailSetting)
		{
			case DETAIL_LOW:
			{
				sDetail += ".DefaultHigh";
				break;
			}

			case DETAIL_MEDIUM:
			{
				sDetail += ".DefaultMid";
				break;
			}

			case DETAIL_HIGH:
			{
				sDetail += ".DefaultLow";
				break;
			}

			default:
			{
				sDetail += ".DefaultMid";
				break;
			}
		}
#endif

		sNewCmdLine += sDetail;
	}

	// Add the run/start specified command-line parameters...
	sNewCmdLine += " ";
	sNewCmdLine += theApp.m_lpCmdLine;

	// Add the user specified command-line parameters...
	sNewCmdLine += " ";
	sNewCmdLine += theApp.m_csCommands;

#ifdef PRIMAL_HUNT
	// PH: the debug check, left in: hold shift to see the command line
	if (GetAsyncKeyState(VK_SHIFT) & 0x80000000)
	{
		int nRet = theApp.MessageBox(sNewCmdLine, "Command Line String", MB_OKCANCEL);
		if (nRet == IDCANCEL)
			return(FALSE);
	}
#endif

	// Time to run
	STARTUPINFO			si;
	PROCESS_INFORMATION pi;

	GetStartupInfo(&si);

#ifdef AVP2SRC_MODERN
	// RETAIL BUGS fixed (MODERN.md M-B103, M-B107): the command line is
	// written as it is, not as a format string, and from the CString,
	// whatever its length, not a copy in 1024 bytes of stack
	FILE* pFile = fopen("avp2cmds.txt", "w");
	if (!pFile)
	{
		return FALSE;
	}

	fputs(sNewCmdLine, pFile);
	fclose(pFile);
#else
	// RETAIL BUG: the command line, whatever its length, is copied into
	// 1024 bytes of stack
	char sString[1024];
	strcpy(sString, sNewCmdLine);

	// The command line goes to the engine in a file
	// RETAIL BUG: written with the command line as fprintf's format string,
	// so a '%' in it (the user's own commands) is taken for a conversion
	FILE* pFile = fopen("avp2cmds.txt", "w");
	if (!pFile)
	{
		return FALSE;
	}

	fprintf(pFile, sString);
	fclose(pFile);
#endif

	CString sRun;
	if (theApp.m_bCDLauncherRunning && !theApp.m_csInstallDir.IsEmpty())
	{
		sRun = theApp.m_csInstallDir + "\\lithtech.exe -cmdfile avp2cmds.txt";
	}
	else
	{
		sRun = "lithtech.exe -cmdfile avp2cmds.txt";
	}

	if (!CreateProcess(NULL, sRun.GetBuffer(0), NULL, NULL, FALSE, CREATE_DEFAULT_ERROR_MODE | CREATE_NEW_PROCESS_GROUP,	NULL, NULL, &si, &pi))
	{
		theApp.MessageBox(IDS_CANTLAUNCHCLIENTEXE, IDS_APPNAME, MB_OK | MB_ICONEXCLAMATION);
		return(FALSE);
	}

#ifdef AVP2SRC_MODERN
	// M-8: the direct start waits for the game and takes its exit code
	if (g_bM8Direct)
	{
		if (WaitForSingleObject(pi.hProcess, INFINITE) == WAIT_OBJECT_0)
			GetExitCodeProcess(pi.hProcess, &g_dwM8ExitCode);
	}
	CloseHandle(pi.hThread);
	CloseHandle(pi.hProcess);
#endif

	return TRUE;
}
