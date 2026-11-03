// *********************************************************************** //
//
//	MODULE:		UTILS (.H)
//
//	PURPOSE:	Gernal purpose utilities
//
//	HISTORY:	02/15/95 [blg] This file was created
//
//	NOTICE:		Copyright (c) 1995, Monolith Productions, Inc.
//
// *********************************************************************** //

#ifndef _UTILS_H_
#define _UTILS_H_

#pragma warning( disable : 4786 )

#include "shlwapi.h"

// Macros

#define PACKVERSION(major,minor) MAKELONG(minor,major)

// Prototypes...

BOOL	ExistProcess(const char* sExe, int thresh = 0, HANDLE* phProcess = NULL);


// (The NOLF2 launcher's Utils.h also has <string> and a CaselessLesser
// std::string comparator here. Neither is the shipped era's: nothing uses
// the comparator, and <string> made every file including this header emit
// std::ctype<unsigned short>::id's guarded initialiser and its atexit thunk
// (Utils.obj, launcher.obj), which NOLF.exe does not have.)

// EOF...

#endif
