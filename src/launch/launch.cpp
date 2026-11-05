// ----------------------------------------------------------------------- //
//
//	MODULE:		LAUNCH (.CPP)
//
//	PURPOSE:	Launch.dll's export, GetLithTechCommandLine
//				(0x10001820), and the launcher settings it reads from the
//				registry (0x10002190): the engine's command line as NOLF's
//				launcher would have built it, for a lithtech.exe started
//				without one.
//
// ----------------------------------------------------------------------- //

#include "launch.h"


// Globals...

char	g_sGameName[]		= "NOLF";				// 0x10033c40
char	g_sVersion[]		= "1.0";				// 0x10033c48
char	g_sGameExe[]		= "GAME\\NOLF.EXE";		// 0x10033c4c
char	g_sGameRezFile[]	= "nolf.rez";			// 0x10033c5c
char	g_sCustomRez[]		= "custom";				// 0x10033c68
char	g_sPatchRez[]		= "nolfp";				// 0x10033c70

char	g_sCommands[256]		= { "" };				// 0x10037ce4

BOOL	g_bDisableSound = FALSE;					// 0x10037de4
BOOL	g_bDisableMusic = FALSE;					// 0x10037de8
BOOL	g_bDisableMovies = FALSE;					// 0x10037dec
BOOL	g_bDisableLightMap = FALSE;					// 0x10037df0
BOOL	g_bDisableFog = FALSE;						// 0x10037df4
BOOL	g_bDisableLines = FALSE;					// 0x10037df8
BOOL	g_bDisableModelFB = FALSE;					// 0x10037dfc
BOOL	g_bEnableOptSurf = FALSE;					// 0x10037e00
BOOL	g_bEnableTjuncs = FALSE;					// 0x10037e04
BOOL	g_bEnableTripBuf = FALSE;					// 0x10037e08
BOOL	g_bEnableMipSharp = FALSE;					// 0x10037e0c


// *********************************************************************** //
//
//	ROUTINE:	GetLithTechCommandLine   [0x10001820]
//
//	PURPOSE:	Fills pArgs (nMaxArgs strings of nMaxArgLen characters)
//				with the engine's command line: argv, then the rez files
//				and the launcher's options and saved commands. *pnArgs
//				gets the number of arguments.
//
//	RETAIL BUG:	every argument is copied with strncpy(pArgs[n], s,
//				nMaxArgLen), which leaves the copy unterminated when s is
//				nMaxArgLen characters or longer (lithtech.exe passes 200):
//				a long argument or saved command word runs on into the
//				next slot.
//
// *********************************************************************** //

extern "C" __declspec(dllexport) BOOL GetLithTechCommandLine(int argc, char** argv, int* pnArgs, char** pArgs, int nMaxArgs, int nMaxArgLen)
{
	if (!pnArgs) return(FALSE);


	// Start with the arguments we were given...

	int nArgs = (argc > nMaxArgs) ? nMaxArgs : argc;

	for (int i = 0; i < nArgs; i++)
	{
		strncpy(pArgs[i], argv[i], nMaxArgLen);
	}


	// Add the rez files...

	FindRezFiles(FindCdRom());

	char* sGameRez = GetGameRezFile();
	char* sSoundRez = GetSoundRezFile();

	if (sGameRez && strlen(sGameRez) > 0)
	{
		if (nArgs < nMaxArgs)
		{
			strncpy(pArgs[nArgs], "-rez", nMaxArgLen);
			nArgs++;
		}
		if (nArgs < nMaxArgs)
		{
			strncpy(pArgs[nArgs], sGameRez, nMaxArgLen);
			nArgs++;
		}
	}
	else
	{
		return(FALSE);
	}

	if (sSoundRez && strlen(sSoundRez) > 0)
	{
		if (nArgs < nMaxArgs)
		{
			strncpy(pArgs[nArgs], "-rez", nMaxArgLen);
			nArgs++;
		}
		if (nArgs < nMaxArgs)
		{
			strncpy(pArgs[nArgs], sSoundRez, nMaxArgLen);
			nArgs++;
		}
	}

	if (nArgs < nMaxArgs)
	{
		strncpy(pArgs[nArgs], "-rez", nMaxArgLen);
		nArgs++;
	}
	if (nArgs < nMaxArgs)
	{
		strncpy(pArgs[nArgs], g_sCustomRez, nMaxArgLen);
		nArgs++;
	}

	if (nArgs < nMaxArgs)
	{
		strncpy(pArgs[nArgs], "-rez", nMaxArgLen);
		nArgs++;
	}
	if (nArgs < nMaxArgs)
	{
		strncpy(pArgs[nArgs], g_sPatchRez, nMaxArgLen);
		nArgs++;
	}


	// Single player, in a window titled for the game...

	if (nArgs < nMaxArgs)
	{
		strncpy(pArgs[nArgs], "-multiplayer", nMaxArgLen);
		nArgs++;
	}
	if (nArgs < nMaxArgs)
	{
		strncpy(pArgs[nArgs], "0", nMaxArgLen);
		nArgs++;
	}

	if (nArgs < nMaxArgs)
	{
		strncpy(pArgs[nArgs], "-windowtitle", nMaxArgLen);
		nArgs++;
	}
	if (nArgs < nMaxArgs)
	{
		strncpy(pArgs[nArgs], g_sGameName, nMaxArgLen);
		nArgs++;
	}


	// Add the launcher's options and saved commands...

	if (ReadSettings())
	{
		if (g_bDisableMusic)
		{
			if (nArgs < nMaxArgs)
			{
				strncpy(pArgs[nArgs], "+DisableMusic", nMaxArgLen);
				nArgs++;
			}
			if (nArgs < nMaxArgs)
			{
				strncpy(pArgs[nArgs], "1", nMaxArgLen);
				nArgs++;
			}
		}

		if (g_bDisableSound)
		{
			if (nArgs < nMaxArgs)
			{
				strncpy(pArgs[nArgs], "+DisableSound", nMaxArgLen);
				nArgs++;
			}
			if (nArgs < nMaxArgs)
			{
				strncpy(pArgs[nArgs], "1", nMaxArgLen);
				nArgs++;
			}
		}

		if (g_bDisableMovies)
		{
			if (nArgs < nMaxArgs)
			{
				strncpy(pArgs[nArgs], "+DisableMovies", nMaxArgLen);
				nArgs++;
			}
			if (nArgs < nMaxArgs)
			{
				strncpy(pArgs[nArgs], "1", nMaxArgLen);
				nArgs++;
			}
		}

		if (g_bDisableLightMap)
		{
			if (nArgs < nMaxArgs)
			{
				strncpy(pArgs[nArgs], "+DisableLightMap", nMaxArgLen);
				nArgs++;
			}
			if (nArgs < nMaxArgs)
			{
				strncpy(pArgs[nArgs], "1", nMaxArgLen);
				nArgs++;
			}
		}

		if (g_bDisableModelFB)
		{
			if (nArgs < nMaxArgs)
			{
				strncpy(pArgs[nArgs], "+DisableModelFB", nMaxArgLen);
				nArgs++;
			}
			if (nArgs < nMaxArgs)
			{
				strncpy(pArgs[nArgs], "1", nMaxArgLen);
				nArgs++;
			}
		}

		if (g_bDisableFog)
		{
			if (nArgs < nMaxArgs)
			{
				strncpy(pArgs[nArgs], "+DisableFog", nMaxArgLen);
				nArgs++;
			}
			if (nArgs < nMaxArgs)
			{
				strncpy(pArgs[nArgs], "1", nMaxArgLen);
				nArgs++;
			}
		}

		if (g_bDisableLines)
		{
			if (nArgs < nMaxArgs)
			{
				strncpy(pArgs[nArgs], "+DisableLines", nMaxArgLen);
				nArgs++;
			}
			if (nArgs < nMaxArgs)
			{
				strncpy(pArgs[nArgs], "1", nMaxArgLen);
				nArgs++;
			}
		}

		if (g_bEnableOptSurf)
		{
			if (nArgs < nMaxArgs)
			{
				strncpy(pArgs[nArgs], "+EnableOptSurf", nMaxArgLen);
				nArgs++;
			}
			if (nArgs < nMaxArgs)
			{
				strncpy(pArgs[nArgs], "1", nMaxArgLen);
				nArgs++;
			}
		}

		if (g_bEnableTripBuf)
		{
			if (nArgs < nMaxArgs)
			{
				strncpy(pArgs[nArgs], "+EnableTripBuf", nMaxArgLen);
				nArgs++;
			}
			if (nArgs < nMaxArgs)
			{
				strncpy(pArgs[nArgs], "1", nMaxArgLen);
				nArgs++;
			}
		}

		if (g_bEnableTjuncs)
		{
			if (nArgs < nMaxArgs)
			{
				strncpy(pArgs[nArgs], "+EnableTjuncs", nMaxArgLen);
				nArgs++;
			}
			if (nArgs < nMaxArgs)
			{
				strncpy(pArgs[nArgs], "1", nMaxArgLen);
				nArgs++;
			}
		}

		if (g_bEnableMipSharp)
		{
			if (nArgs < nMaxArgs)
			{
				strncpy(pArgs[nArgs], "+EnableMipSharp", nMaxArgLen);
				nArgs++;
			}
			if (nArgs < nMaxArgs)
			{
				strncpy(pArgs[nArgs], "1", nMaxArgLen);
				nArgs++;
			}
		}


		// The commands the launcher saved, one argument a word...

		char seps[] = " \t\n";
		char* token = NULL;

		token = strtok(g_sCommands, seps);

		while (token)
		{
			if (nArgs < nMaxArgs)
			{
				strncpy(pArgs[nArgs], token, nMaxArgLen);
				nArgs++;
			}

			token = strtok(NULL, seps);
		}
	}


	// All done...

	*pnArgs = nArgs;

	return(TRUE);
}


// *********************************************************************** //
//
//	ROUTINE:	ReadSettings   [0x10002190]
//
//	PURPOSE:	Reads the launcher's options, and its saved commands if it
//				was told to save them, from the registry
//
// *********************************************************************** //

BOOL ReadSettings()
{
	CRegMgr regMgr;

	if (!regMgr.Init("Monolith Productions", g_sGameName, g_sVersion, NULL, HKEY_LOCAL_MACHINE, NULL))
	{
		return(FALSE);
	}

	UINT32 dwSize = 254;
	char sBuf[256];
	sBuf[0] = '\0';

	BOOL bSaveCommands = regMgr.Get("Save Commands", 0);

	if (bSaveCommands)
	{
		regMgr.Get("Commands", sBuf, dwSize, NULL);
		strcpy(g_sCommands, sBuf);
	}

	g_bDisableSound     = regMgr.Get("Disable Sound", 0);
	g_bDisableMusic     = regMgr.Get("Disable Music", 0);
	g_bDisableMovies    = regMgr.Get("Disable Movies", 0);
	g_bDisableLightMap  = regMgr.Get("Disable LightMap", 0);
	g_bDisableFog       = regMgr.Get("Disable Fog", 0);
	g_bDisableLines     = regMgr.Get("Disable Lines", 0);
	g_bDisableModelFB   = regMgr.Get("Disable ModelFB", 0);
	g_bEnableOptSurf    = regMgr.Get("Enable OptSurf", 0);
	g_bEnableTripBuf    = regMgr.Get("Enable TripBuf", 0);
	g_bEnableTjuncs     = regMgr.Get("Enable Tjuncs", 0);
	g_bEnableMipSharp   = regMgr.Get("Enable MipSharp", 0);

	return(TRUE);
}
