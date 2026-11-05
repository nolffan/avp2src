// AVP2Up.cpp : Defines the entry point for the DLL application.
//
// AVP2Up.dll (AvP2) and AVP2XUp.dll (Primal Hunt): the resources the Sierra
// updater (SierraUp.exe) shows the game in -- its icon, the panel bitmaps
// and the colours of its text (AVP2Up.rc, AVP2XUp.rc). The game names the
// DLL to the updater (CShell.dll's "ResourceDllFile"); there are no exports.
//
// The only code is this DllMain (0x10001000 in both images: mov eax,1 /
// ret 0xc), placed ahead of the C runtime's _CRT_INIT (0x10001010) and
// _DllMainCRTStartup (the entry point, 0x100010e9), which calls it in the
// place of libcmt.lib's own default one: the images' Rich headers count
// one C object fewer than ltmsg.dll's and one C++ object more (with
// stdafx.obj, the two of cl 12.00.8966).

#include "stdafx.h"

BOOL APIENTRY DllMain( HANDLE hModule, 
                       DWORD  ul_reason_for_call, 
                       LPVOID lpReserved
					 )
{
    return TRUE;
}
