// ----------------------------------------------------------------------- //
//
//	MODULE:		LAUNCH (.H)
//
//	PURPOSE:	Launch.dll: NOLF's launcher DLL, which the engine loads for
//				its one export, GetLithTechCommandLine (lithtech.exe's
//				command-line parser: src/engine/lithtech/clientwin.cpp).
//				AvP2 and Primal Hunt ship the same file, unchanged: a
//				1999 Debug build (/ZI /Od, the debug C runtime), so every
//				string in it is still NOLF's.
//
//				None of the names below is in the image (it has no
//				symbols); the shapes follow nolfsrc's launcher
//				(src/engine/launcher/cdcheck.cpp and RezFind.cpp), where the same
//				routines live in NOLF.exe.
//
// ----------------------------------------------------------------------- //

#ifndef __LAUNCH_H__
#define __LAUNCH_H__


// Includes...

#include <windows.h>
#include <string.h>
#include "regmgr.h"


// Globals (launch.cpp)...

extern char g_sGameName[];			// 0x10033c40 "NOLF"
extern char g_sVersion[];			// 0x10033c48 "1.0"
extern char g_sGameExe[];			// 0x10033c4c "GAME\\NOLF.EXE"
extern char g_sGameRezFile[];		// 0x10033c5c "nolf.rez"
extern char g_sCustomRez[];			// 0x10033c68 "custom"
extern char g_sPatchRez[];			// 0x10033c70 "nolfp"


// Prototypes...

// cdcheck.cpp
BOOL	CheckCd(HWND hWnd);
BOOL	FindCd();
BOOL	ExistFile(const char* sFile);
char	GetCdRomDrive();
char	FindCdRom();

// launch.cpp
BOOL	ReadSettings();

// RezFind.cpp
BOOL	FindRezFiles(char chCdRom);
char*	GetGameRezFile();
char*	GetSoundRezFile();


#endif
