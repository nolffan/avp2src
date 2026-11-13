/****************************************************************************
;
;	 MODULE:		REZFIND (.CPP)
;
;	PURPOSE:		Routines to find the rez files
;
;	HISTORY:		08/11/98 [blg] This file was created
;
;	COMMENT:		Copyright (c) 1998, Monolith Productions Inc.
;
****************************************************************************/


// Includes...

#include "StdAfx.h"
#include "Io.h"
#include "RezFind.h"


// Globals... (AVP2.exe 0x42dfa0 - 0x42e2a0)

char	g_sGameRez[128] = { "" };
char	g_sSoundRez[128] = { "" };
char	g_sAlienRez[128] = { "" };
char	g_sMarineRez[128] = { "" };
char	g_sPredatorRez[128] = { "" };
char	g_sMultiRez[128] = { "" };
#ifdef PRIMAL_HUNT
char	g_sAvP2XRez[128] = { "" };
#endif


// Prototypes...

BOOL ExistRezFile(const char* sFile);
BOOL ExistRezDir(const char* sDir);


// Functions...

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	FindRezFiles (AVP2.exe 0x407ca0; its end lies in the pages
//				the protection wrapper keeps encrypted, from 0x408000)
//
//	PURPOSE:	Looks for game and sound rez stuff
//
// ----------------------------------------------------------------------- //

BOOL FindRezFiles(char chCdRom, CString& csRezBase)
{
	// Clear the strings...

	g_sGameRez[0] = '\0';
	g_sSoundRez[0] = '\0';
	g_sAlienRez[0] = '\0';
	g_sMarineRez[0] = '\0';
	g_sPredatorRez[0] = '\0';
	g_sMultiRez[0] = '\0';
#ifdef PRIMAL_HUNT
	g_sAvP2XRez[0] = '\0';
#endif


	// Look for game rez file in the current directory and on the cd-rom drive...

	CString sGameRezFile = csRezBase + ".rez";
	CString sAlienRezFile = "Alien.rez";
	CString sMarineRezFile = "Marine.rez";
	CString sPredatorRezFile = "Predator.rez";
	CString sMultiRezFile = "Multi.rez";
#ifdef PRIMAL_HUNT
	CString sAvP2XRezFile = "AVP2X.rez";
#endif

	if (ExistRezFile(sGameRezFile))
	{
		strcpy(g_sGameRez, sGameRezFile);
	}

	// If necessary, look for a game rez directory in the current directory...

	if (g_sGameRez[0] == '\0')
	{
		if (ExistRezDir(csRezBase))
		{
			strcpy(g_sGameRez, csRezBase);
		}
	}

	// If necessary, look for game rez file on the CD-ROM drive...

	if (g_sGameRez[0] == '\0')
	{
		if (chCdRom > 0)
		{
			char sPath[256];
			wsprintf(sPath, "%c:\\data\\%s.rez", chCdRom, csRezBase);

			if (ExistRezFile(sPath))
			{
				strcpy(g_sGameRez, sPath);
			}
		}
	}


	// Look for the campaigns' and multiplayer rez files in the current directory...

	if (ExistRezFile(sAlienRezFile))
	{
		strcpy(g_sAlienRez, sAlienRezFile);
	}

	if (ExistRezFile(sMarineRezFile))
	{
		strcpy(g_sMarineRez, sMarineRezFile);
	}

	if (ExistRezFile(sPredatorRezFile))
	{
		strcpy(g_sPredatorRez, sPredatorRezFile);
	}

	if (ExistRezFile(sMultiRezFile))
	{
		strcpy(g_sMultiRez, sMultiRezFile);
	}

#ifdef PRIMAL_HUNT
	// PH: Primal Hunt's own (its code is encrypted in the retail image:
	// the string pool alone says it looks for it)
	if (ExistRezFile(sAvP2XRezFile))
	{
		strcpy(g_sAvP2XRez, sAvP2XRezFile);
	}
#endif


	// If necessary, look for them on the CD-ROM drive...

	if (chCdRom > 0)
	{
		char sPath[256];

		if (g_sAlienRez[0] == '\0')
		{
			wsprintf(sPath, "%c:\\%s", chCdRom, sAlienRezFile);

			if (ExistRezFile(sPath))
			{
				strcpy(g_sAlienRez, sPath);
			}
		}

		if (g_sMarineRez[0] == '\0')
		{
			wsprintf(sPath, "%c:\\%s", chCdRom, sMarineRezFile);

			if (ExistRezFile(sPath))
			{
				strcpy(g_sMarineRez, sPath);
			}
		}

		if (g_sPredatorRez[0] == '\0')
		{
			wsprintf(sPath, "%c:\\%s", chCdRom, sPredatorRezFile);

			if (ExistRezFile(sPath))
			{
				strcpy(g_sPredatorRez, sPath);
			}
		}

		if (g_sMultiRez[0] == '\0')
		{
			wsprintf(sPath, "%c:\\%s", chCdRom, sMultiRezFile);

			if (ExistRezFile(sPath))
			{
				strcpy(g_sMultiRez, sPath);
			}
		}
	}


	// Look for sound rez file in the current directory...

	if (ExistRezFile("sounds.rez"))
	{
		strcpy(g_sSoundRez, "sounds.rez");
	}


	// If necessary, look for a sound directory...

	if (g_sSoundRez[0] == '\0')
	{
		if (ExistRezDir("sounds"))
		{
			strcpy(g_sSoundRez, "sounds");
		}
	}


	// Determine if we have both rez strings (from here on the code is
	// encrypted in the retail image: NOLF2's)...

	if (g_sGameRez[0] == '\0' || g_sSoundRez[0] == '\0')
	{
		return(FALSE);
	}


	// All done...

	return(TRUE);
}

// AVP2.exe 0x408100 - 0x408150 (encrypted)
char* GetGameRezFile()
{
	return(g_sGameRez);
}

char* GetSoundRezFile()
{
	return(g_sSoundRez);
}

char* GetAlienRezFile()
{
	return(g_sAlienRez);
}

char* GetMarineRezFile()
{
	return(g_sMarineRez);
}

char* GetPredatorRezFile()
{
	return(g_sPredatorRez);
}

char* GetMultiRezFile()
{
	return(g_sMultiRez);
}

#ifdef PRIMAL_HUNT
// PrimalHunt.exe 0x408970 (encrypted)
char* GetAvP2XRezFile()
{
	return(g_sAvP2XRez);
}
#endif


// ----------------------------------------------------------------------- //
//
//	ROUTINE:	ExistRezFile (AVP2.exe 0x408160, encrypted)
//
//	PURPOSE:	Determines if the given file exists
//
// ----------------------------------------------------------------------- //

BOOL ExistRezFile(const char* sFile)
{
	// Sanity checks...

	if (!sFile) return(FALSE);
	if (sFile[0] == '\0') return(FALSE);


	// Check if this file exists...

    struct _finddata_t c_file;
    long   hFile;

    hFile = _findfirst(sFile, &c_file);
	if (hFile == -1L) return(FALSE);
#ifdef AVP2SRC_MODERN
	_findclose(hFile);	// M-B101, as Launch.dll's: the handle given back
#endif

	if (c_file.attrib & _A_SUBDIR)
	{
		return(FALSE);
	}


	// All done...

	return(TRUE);
}


// ----------------------------------------------------------------------- //
//
//	ROUTINE:	ExistRezDir (AVP2.exe 0x4081c0, encrypted)
//
//	PURPOSE:	Determines if the given directory exists
//
// ----------------------------------------------------------------------- //

BOOL ExistRezDir(const char* sDir)
{
	// Sanity checks...

	if (!sDir) return(FALSE);
	if (sDir[0] == '\0') return(FALSE);


	// Check if this directory exists...

    struct _finddata_t c_file;
    long   hFile;

    hFile = _findfirst(sDir, &c_file);
	if (hFile == -1L) return(FALSE);
#ifdef AVP2SRC_MODERN
	_findclose(hFile);	// M-B101, as Launch.dll's: the handle given back
#endif

	if (!(c_file.attrib & _A_SUBDIR))
	{
		return(FALSE);
	}


	// All done...

	return(TRUE);
}
