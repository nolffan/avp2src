/****************************************************************************
;
;	 MODULE:		REZFIND (.CPP)
;
;	PURPOSE:		Routines to find the rez files: Launch.dll's
;					0x100023f0..0x100026a6. nolfsrc's launcher has the
;					later form of this file (src/engine/launcher/
;					RezFind.cpp there, from the NOLF2 source release's
;					launcher), which this one follows.
;
;	COMMENT:		Copyright (c) 1998-2002, Monolith Productions Inc.
;
****************************************************************************/


// Includes...

#include "launch.h"
#include <io.h>


// Globals...

char	g_sGameRez[128] = { "" };		// 0x10037e4c
char	g_sSoundRez[128] = { "" };	// 0x10037ecc


// Prototypes...

BOOL ExistRezFile(const char* sFile);
BOOL ExistRezDir(const char* sDir);


// Functions...

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	FindRezFiles   [0x100023f0]
//
//	PURPOSE:	Looks for game and sound rez stuff
//
// ----------------------------------------------------------------------- //

BOOL FindRezFiles(char chCdRom)
{
	// Clear the strings...

	g_sGameRez[0] = '\0';
	g_sSoundRez[0] = '\0';


	// Look for game rez file in the current directory...

	if (ExistRezFile(g_sGameRezFile))
	{
		strcpy(g_sGameRez, g_sGameRezFile);
	}


	// If necessary, look for a game rez directory in the current directory...

	if (g_sGameRez[0] == '\0')
	{
		if (ExistRezDir(g_sGameName))
		{
			strcpy(g_sGameRez, g_sGameName);
		}
	}


	// If necessary, look for game rez file on the CD-ROM drive...

	if (g_sGameRez[0] == '\0')
	{
		if (chCdRom > 0)
		{
			char sPath[256];
			wsprintf(sPath, "%c:\\data\\%s", chCdRom, g_sGameRezFile);

			if (ExistRezFile(sPath))
			{
				strcpy(g_sGameRez, sPath);
			}
		}
	}


	// Look for sound rez file in the current directory...

	if (ExistRezFile("sound.rez"))
	{
		strcpy(g_sSoundRez, "sound.rez");
	}


	// If necessary, look for a sound directory...

	if (g_sSoundRez[0] == '\0')
	{
		if (ExistRezDir("sound"))
		{
			strcpy(g_sSoundRez, "sound");
		}
	}


	// Determine if we have both rez strings...

	if (g_sGameRez[0] == '\0' || g_sSoundRez[0] == '\0')
	{
		return(FALSE);
	}


	// All done...

	return(TRUE);
}

// [0x10002580]
char* GetGameRezFile()
{
	return(g_sGameRez);
}

// [0x100025a0]
char* GetSoundRezFile()
{
	return(g_sSoundRez);
}


// ----------------------------------------------------------------------- //
//
//	ROUTINE:	ExistRezFile   [0x100025c0]
//
//	PURPOSE:	Determines if the given file exists
//
//	RETAIL BUG:	the _findfirst handle is never given back (_findclose),
//				here or in ExistRezDir: each call leaks one.
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

	if (c_file.attrib & _A_SUBDIR)
	{
		return(FALSE);
	}


	// All done...

	return(TRUE);
}


// ----------------------------------------------------------------------- //
//
//	ROUTINE:	ExistRezDir   [0x10002640]
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

	if (!(c_file.attrib & _A_SUBDIR))
	{
		return(FALSE);
	}


	// All done...

	return(TRUE);
}
