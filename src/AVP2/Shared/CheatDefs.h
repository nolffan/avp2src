// ----------------------------------------------------------------------- //
//
// MODULE  : CheatDefs.h
//
// PURPOSE : List enumerated definitions for the available cheat codes
//
// CREATED : --/--/--
//
// (c) 2000 Monolith Productions, Inc.  All Rights Reserved
//
// ----------------------------------------------------------------------- //

#ifndef __CHEATDEFS_H
#define __CHEATDEFS_H

enum CheatCode
{
	CHEAT_NONE = -1,

	// Player modification cheats
	CHEAT_HEALTH,
	CHEAT_ARMOR,
	CHEAT_FULL_WEAPONS,
	CHEAT_AMMO,
	CHEAT_TEARS,
	CHEAT_GOD,
	CHEAT_KFA,
	CHEAT_TELEPORT,
	CHEAT_CLIP,
	CHEAT_CHARACTER,
	CHEAT_RELOADBUTES,
	CHEAT_3RDPERSON,

	// Debug information cheats
	CHEAT_POS,
	CHEAT_ROT,
	CHEAT_DIMS,
	CHEAT_VEL,
	CHEAT_FRAMERATE,
	CHEAT_TRIGGERBOX,

	// Position control cheats
	CHEAT_POSWEAPON,
	CHEAT_POSWEAPON_MUZZLE,
	CHEAT_WEAPON_BREACH,
	CHEAT_LIGHTSCALE,
	CHEAT_LIGHTADD,
	CHEAT_VERTEXTINT,
	CHEAT_FOV,

	// AI cheats
	CHEAT_REMOVEAI,

	// Misc cheats
	CHEAT_CONFIG,
	CHEAT_MISSIONS,

	// Easter egg cheats
	CHEAT_MILLERTIME,

#ifdef PRIMAL_HUNT
	// Primal Hunt: two more codes after mpmillertime in CheatMgr's table
	// (PHCShell.dll's strings "OCgRIOQMU" and "OCKLJ", 0x101d6a7c and
	// 0x101d6a70); the second's name is ours
	CHEAT_NEXTLEVEL,
	CHEAT_PH30,
#endif

	// Must exist for array size information
	CHEAT_MAX
};

#endif
