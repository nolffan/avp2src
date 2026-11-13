// ----------------------------------------------------------------------- //
//
// MODULE  : m7_padbindings.h
//
// PURPOSE : The modern build's controller (docs/MODERN.md, M-7): its
//           default bindings.  A profile keeps each command's bindings as
//           "key@mouse@joystick" (CUserProfile, ProfileMgr.cpp), per
//           species; the joystick is the engine's ##gamepad, so its column
//           is the pad's.  A profile that has never had them -- the retail
//           Default.prf, which Customize Controls' "Restore defaults" reads
//           too, and any profile made before -- gets these where its
//           column is empty and the button is not already taken; "PadLayout"
//           in its [Basic] section records that it has (and the player's
//           own changes are kept from then on).
//
//           The sticks are not bound (the client shell reads them:
//           m7_pad.h), nor is Start (the menu key).  ProfileMgr.cpp
//           includes this, once.
//
//           Modern build only (AVP2SRC_MODERN).
//
// ----------------------------------------------------------------------- //

#ifndef __M7_PADBINDINGS_H__
#define __M7_PADBINDINGS_H__

#include "CommandIDs.h"

#define M7_PAD_LAYOUT_KEY		"PadLayout"
#define M7_PAD_LAYOUT			1

struct M7PadBinding
{
	int			m_nCommandID;
	int			m_nType;			// COM_SHARED, or the species'
	const char*	m_pTrigger;			// the ##gamepad object (p7_gamepad.h)
};

static const M7PadBinding c_M7PadDefaults[] =
{
	{ COMMAND_ID_FIRING,			COM_SHARED,		"Right Trigger" },
	{ COMMAND_ID_ALT_FIRING,		COM_SHARED,		"Left Trigger" },
	{ COMMAND_ID_JUMP,				COM_SHARED,		"A Button" },
	{ COMMAND_ID_RUN,				COM_SHARED,		"Left Stick Click" },
	{ COMMAND_ID_CENTERVIEW,		COM_SHARED,		"Right Stick Click" },
	{ COMMAND_ID_SCOREDISPLAY,		COM_SHARED,		"Back" },
	{ COMMAND_ID_NEXT_VISIONMODE,	COM_SHARED,		"D-Pad Up" },

	{ COMMAND_ID_DUCK,				COM_MARINE,		"B Button" },
	{ COMMAND_ID_ACTIVATE,			COM_MARINE,		"X Button" },
	{ COMMAND_ID_RELOAD,			COM_MARINE,		"Y Button" },
	{ COMMAND_ID_NEXT_WEAPON,		COM_MARINE,		"Right Bumper" },
	{ COMMAND_ID_PREV_WEAPON,		COM_MARINE,		"Left Bumper" },
	{ COMMAND_ID_SHOULDERLAMP,		COM_MARINE,		"D-Pad Down" },
	{ COMMAND_ID_FLARE,				COM_MARINE,		"D-Pad Left" },
	{ COMMAND_ID_HACKING_DEVICE,	COM_MARINE,		"D-Pad Right" },

	{ COMMAND_ID_DUCK,				COM_PREDATOR,	"B Button" },
	{ COMMAND_ID_ACTIVATE,			COM_PREDATOR,	"X Button" },
	{ COMMAND_ID_RELOAD,			COM_PREDATOR,	"Y Button" },
	{ COMMAND_ID_NEXT_WEAPON,		COM_PREDATOR,	"Right Bumper" },
	{ COMMAND_ID_PREV_WEAPON,		COM_PREDATOR,	"Left Bumper" },
	{ COMMAND_ID_CLOAK,				COM_PREDATOR,	"D-Pad Down" },
	{ COMMAND_ID_ZOOM_OUT,			COM_PREDATOR,	"D-Pad Left" },
	{ COMMAND_ID_ZOOM_IN,			COM_PREDATOR,	"D-Pad Right" },

	{ COMMAND_ID_DUCK,				COM_ALIEN,		"B Button" },
	{ COMMAND_ID_POUNCE_JUMP,		COM_ALIEN,		"X Button" },
	{ COMMAND_ID_WALLWALK_TOGGLE,	COM_ALIEN,		"Y Button" },
	{ COMMAND_ID_WALLWALK,			COM_ALIEN,		"Left Bumper" },
};

#define M7_NUM_PAD_DEFAULTS		(sizeof(c_M7PadDefaults) / sizeof(c_M7PadDefaults[0]))

static LTBOOL M7_IsBound(const char* pName)
{
	return pName[0] && pName[0] != ' ';
}

// The defaults, into a profile's joystick column: where it is empty and the
// button is not taken by a command of the same species (or a shared one).
// bReset empties the column first (Controller's "Restore defaults").
static void M7Pad_DefaultBindings(CUserProfile* pProfile, LTBOOL bReset)
{
	int c, k;
	unsigned int i;

	if (bReset)
	{
		for (c = 0; c < g_kNumCommands; c++)
		{
			CBindingData* pData = pProfile->FindBinding(g_CommandArray[c].nType, c);

			if (pData && g_CommandArray[c].nCommandID >= 0)
			{
				strcpy(pData->strRealName[2], " ");
				strcpy(pData->strTriggerName[2], " ");
			}
		}
	}

	for (i = 0; i < M7_NUM_PAD_DEFAULTS; i++)
	{
		const M7PadBinding& def = c_M7PadDefaults[i];
		CBindingData* pData = LTNULL;
		LTBOOL bTaken = LTFALSE;

		for (c = 0; c < g_kNumCommands && !pData; c++)
		{
			if (g_CommandArray[c].nCommandID == def.m_nCommandID && (int)g_CommandArray[c].nType == def.m_nType)
				pData = pProfile->FindBinding(def.m_nType, c);
		}

		if (!pData || M7_IsBound(pData->strRealName[2]))
			continue;

		for (k = 0; k < g_kNumCommands && !bTaken; k++)
		{
			int nType = (int)g_CommandArray[k].nType;
			CBindingData* pOther;

			if (nType != def.m_nType && nType != COM_SHARED && def.m_nType != COM_SHARED)
				continue;

			pOther = pProfile->FindBinding(nType, k);
			if (pOther && !stricmp(pOther->strRealName[2], def.m_pTrigger))
				bTaken = LTTRUE;
		}

		if (!bTaken)
		{
			SAFE_STRCPY(pData->strRealName[2], def.m_pTrigger);
			SAFE_STRCPY(pData->strTriggerName[2], def.m_pTrigger);
		}
	}
}

#endif // __M7_PADBINDINGS_H__
