// ----------------------------------------------------------------------- //
//
//	MODULE:		CDCHECK (.CPP)
//
//	PURPOSE:	Launch.dll's CD-ROM detection (0x100010a0..0x10001721):
//				the drive holding NOLF's play disc, found by its
//				\GAME\NOLF.EXE. nolfsrc's launcher (cdcheck.cpp) has the
//				later form of the same routines.
//
// ----------------------------------------------------------------------- //

#include "launch.h"


// *********************************************************************** //
//
//	ROUTINE:	CheckCd   [0x100010a0]
//
//	PURPOSE:	Makes sure the play disc is in the drive, prompting the
//				user until it is (or until they cancel). Nothing in the
//				DLL calls it.
//
// *********************************************************************** //

BOOL CheckCd(HWND hWnd)
{
	if (FindCd()) return(TRUE);

	BOOL bLoop = TRUE;

	while (bLoop)
	{
		char sMsg[128];
		char sTitle[64];

		strcpy(sMsg, "Please insert the game CD-ROM into the drive.");
		strcpy(sTitle, g_sGameName);

		int nRet = MessageBox(hWnd, sMsg, sTitle, MB_OKCANCEL | MB_ICONEXCLAMATION);

		if (nRet == IDOK)
		{
			if (FindCd()) return(TRUE);

			Sleep(1000);

			if (FindCd()) return(TRUE);
		}
		else
		{
			return(FALSE);
		}
	}

	return(FALSE);
}


// *********************************************************************** //
//
//	ROUTINE:	FindCd   [0x10001190]
//
//	PURPOSE:	Is the play disc in a drive?
//
// *********************************************************************** //

BOOL FindCd()
{
	char chCdRom = FindCdRom();

	return(chCdRom != 0);
}


// *********************************************************************** //
//
//	ROUTINE:	ExistFile   [0x100011c0]
//
//	PURPOSE:	Determines if the given file exists
//
// *********************************************************************** //

BOOL ExistFile(const char* sFile)
{
	if (!sFile) return(FALSE);
	if (sFile[0] == '\0') return(FALSE);

	OFSTRUCT ofs;
	HFILE hFile = OpenFile(sFile, &ofs, OF_EXIST);

	return(hFile != HFILE_ERROR);
}


// *********************************************************************** //
//
//	ROUTINE:	GetCdRomDrive   [0x10001230]
//
//	PURPOSE:	The first CD-ROM drive: the one the registry names, else
//				the current directory's, else the first of A: to Z:.
//				Whatever disc is in it. Nothing in the DLL calls it.
//
// *********************************************************************** //

char GetCdRomDrive()
{
	// Check the drive stored in the registry first...

	CRegMgr regMgr;

	if (regMgr.Init("Monolith Productions", g_sGameName, g_sVersion, NULL, HKEY_LOCAL_MACHINE, NULL))
	{
		char sRoot[32];
		char sBuf[30];
		UINT32 dwSize = 30;
		sBuf[0] = '\0';

		if (regMgr.Get("CdRom Drive", sBuf, dwSize, NULL))
		{
			if (sBuf[0] > 20)
			{
				char chDrive = sBuf[0];

				wsprintf(sRoot, "%c:\\", chDrive);

				if (GetDriveType(sRoot) == DRIVE_CDROM)
				{
					return(chDrive);
				}
			}
		}
	}


	// Then the current directory's drive...

	char sDir[256];
	GetCurrentDirectory(255, sDir);
	sDir[3] = '\0';

	if (GetDriveType(sDir) == DRIVE_CDROM)
	{
		return(sDir[0]);
	}


	// Check all of the drives...

	char ch = 'A';

	for (int i = 0; i < 26; i++)
	{
		wsprintf(sDir, "%c:\\", ch);

		if (GetDriveType(sDir) == DRIVE_CDROM)
		{
			return(ch);
		}

		ch++;
	}

	return(0);
}


// *********************************************************************** //
//
//	ROUTINE:	FindCdRom   [0x100014d0]
//
//	PURPOSE:	Finds the CD-ROM drive holding the play disc: the drive
//				the registry names, else the first of A: to Z:. The answer
//				is kept for the next call.
//
// *********************************************************************** //

char FindCdRom()
{
	static char s_chCdRom;			// 0x10037ce0


	// Use the cached value if we have one...

	if (s_chCdRom) return(s_chCdRom);


	// Check the drive stored in the registry first...

	CRegMgr regMgr;

	if (regMgr.Init("Monolith Productions", g_sGameName, g_sVersion, NULL, HKEY_LOCAL_MACHINE, NULL))
	{
		char sRoot[32];
		char sBuf[30];
		UINT32 dwSize = 30;
		sBuf[0] = '\0';

		if (regMgr.Get("CdRom Drive", sBuf, dwSize, NULL))
		{
			if (sBuf[0] > 20)
			{
				char chDrive = sBuf[0];

				wsprintf(sRoot, "%c:\\", chDrive);

				if (GetDriveType(sRoot) == DRIVE_CDROM)
				{
					char sFile[256];
					wsprintf(sFile, "%c:\\%s", chDrive, g_sGameExe);

					if (ExistFile(sFile))
					{
						s_chCdRom = chDrive;
						return(chDrive);
					}
				}
			}
		}
	}


	// Check all of the drives...

	char ch = 'A';

	while (ch <= 'Z')
	{
		char sRoot[256];
		wsprintf(sRoot, "%c:\\", ch);

		if (GetDriveType(sRoot) == DRIVE_CDROM)
		{
			char sFile[256];
			wsprintf(sFile, "%c:\\%s", ch, g_sGameExe);

			if (ExistFile(sFile))
			{
				s_chCdRom = ch;
				return(ch);
			}
		}

		ch++;
	}

	return(0);
}
