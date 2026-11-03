// ltmsg.dll — resource-only DLL carrying the engine's error message strings
// (and two leftover dev-era network dialogs). lithtech.exe loads it with
// LoadLibrary and pulls messages by string ID; there are no exports.
//
// There is no DllMain here either: the retail image's entry point is the C
// runtime's _DllMainCRTStartup, which calls the C runtime's own default
// DllMain (libcmt.lib's dllmain.obj, returning TRUE) -- its Rich header counts
// that one C object more, and its code starts with _CRT_INIT, not with a
// DllMain of its own. This file only gives the link its object (the Rich
// header's second C++ object).
#include <windows.h>
