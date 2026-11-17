// ----------------------------------------------------------------------- //
//
// MODULE  : m25_consolekey.h
//
// PURPOSE : The modern build's console (docs/MODERN.md, M-25): the default
//           key of the ToggleConsole control, in the profiles.
//
//           A profile keeps the bindings of each command as
//           "key@mouse@joystick" (CUserProfile, ProfileMgr.cpp).  The
//           retail Default.prf (which "Restore defaults" reads too) and
//           each profile from before do not have ToggleConsole.  A
//           profile that has never had the console key gets the key to
//           the left of 1 (DirectInput's "##41": ` and ~ on a US
//           keyboard) when the control has no key and no other control
//           uses that key.  "ConsoleKey" in the [Basic] section of the
//           profile records that it has; after that, the player's own
//           choice stays.
//
//           ProfileMgr.cpp includes this, one time.
//
//           Modern build only (AVP2SRC_MODERN).
//
// ----------------------------------------------------------------------- //

#ifndef __M25_CONSOLEKEY_H__
#define __M25_CONSOLEKEY_H__

#include "CommandIDs.h"

#define M25_CONSOLE_KEY_KEY		"ConsoleKey"
#define M25_CONSOLE_KEY			1
#define M25_CONSOLE_DEFAULT		"##41"

static LTBOOL M25_IsKeyBound(const char* pName)
{
	return pName[0] && pName[0] != ' ';
}

// The default key into the keyboard column of ToggleConsole, if that
// column is empty and no other command has the key.
static void M25Console_DefaultBinding(CUserProfile* pProfile)
{
	CBindingData* pData = LTNULL;
	int c;

	for (c = 0; c < g_kNumCommands && !pData; c++)
	{
		if (g_CommandArray[c].nCommandID == COMMAND_ID_TOGGLE_CONSOLE)
			pData = pProfile->FindBinding(g_CommandArray[c].nType, c);
	}

	if (!pData || M25_IsKeyBound(pData->strRealName[0]))
		return;

	// ToggleConsole is a shared control: the key must be free for all
	// the species
	for (c = 0; c < g_kNumCommands; c++)
	{
		CBindingData* pOther = pProfile->FindBinding(g_CommandArray[c].nType, c);

		if (pOther && pOther != pData && !stricmp(pOther->strRealName[0], M25_CONSOLE_DEFAULT))
			return;
	}

	SAFE_STRCPY(pData->strRealName[0], M25_CONSOLE_DEFAULT);
}

#endif // __M25_CONSOLEKEY_H__
