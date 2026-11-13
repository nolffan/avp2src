// M-8 (docs/MODERN.md): the modern build's launchers. AVP2.exe and
// PrimalHunt.exe start the game directly, as if Launch had been clicked;
// AVP2Launcher.exe and PrimalHuntLauncher.exe are the launcher. Both are
// the same objects (src/launcher/) and one of m8_entry.cpp's two builds.
// Neither needs the registry or a disc: with no install record the game is
// where the program is, and Primal Hunt plays on the AvP2 beside it
// (..\AVP2, the layout the build installs: build/avp2cmds.sh).
//
// Included by src/launcher/Launcher.cpp and LauncherDlg.cpp, after their
// own headers, in AVP2SRC_MODERN blocks.

#ifndef M8_LAUNCH_H
#define M8_LAUNCH_H

extern BOOL		g_bM8Direct;		// m8_entry.cpp: TRUE in AVP2.exe / PrimalHunt.exe
extern DWORD	g_dwM8ExitCode;		// the game's exit code, for the direct start

// A file (not a directory) of that name exists
static inline BOOL M8FileExists(const char* sFile)
{
	DWORD dwAttr = GetFileAttributes(sFile);
	return(dwAttr != 0xFFFFFFFF && !(dwAttr & FILE_ATTRIBUTE_DIRECTORY));
}

// The program's own directory, without a trailing backslash
static inline BOOL M8ProgramDir(CString& csDir)
{
	char sPath[MAX_PATH];
	DWORD dwLen = GetModuleFileName(NULL, sPath, sizeof(sPath));
	if (dwLen == 0 || dwLen >= sizeof(sPath)) return(FALSE);
	char* pSlash = strrchr(sPath, '\\');
	if (!pSlash) return(FALSE);
	*pSlash = '\0';
	csDir = sPath;
	return(TRUE);
}

// A directory and a file in it, one backslash between
static inline CString M8Path(const CString& csDir, const char* sFile)
{
	CString csPath = csDir;
	if (!csPath.IsEmpty() && csPath[csPath.GetLength() - 1] != '\\') csPath += "\\";
	return(csPath + sFile);
}

#ifdef PRIMAL_HUNT
// AvP2 beside Primal Hunt: <our directory>\..\AVP2, if AVP2.REZ is there
static inline BOOL M8AvP2Beside(CString& csDir)
{
	CString csOurs;
	if (!M8ProgramDir(csOurs)) return(FALSE);
	char sFull[MAX_PATH];
	char* pFile;
	DWORD dwLen = GetFullPathName(M8Path(csOurs, "..\\AVP2"), sizeof(sFull), sFull, &pFile);
	if (dwLen == 0 || dwLen >= sizeof(sFull)) return(FALSE);
	if (!M8FileExists(M8Path(sFull, "AVP2.REZ"))) return(FALSE);
	csDir = sFull;
	return(TRUE);
}
#endif

#endif
