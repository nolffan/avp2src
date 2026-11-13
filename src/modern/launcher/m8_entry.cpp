// M-8 (docs/MODERN.md): which of the modern build's two programs this is.
// The launcher's objects are the same in both; this one object, compiled
// twice, tells them apart: with M8_DIRECT 1 it is AVP2.exe / PrimalHunt.exe,
// which start the game directly, as if Launch had been clicked; with 0 it
// is AVP2Launcher.exe / PrimalHuntLauncher.exe, the launcher itself.

#include <windows.h>

BOOL	g_bM8Direct = M8_DIRECT;
DWORD	g_dwM8ExitCode = 0;		// the game's, for the direct start
