// ----------------------------------------------------------------------- //
//
// MODULE  : Scramble.h
//
// PURPOSE : Primal Hunt: scramble and unscramble a string a character at
//           a time, so a message doesn't show in the dll as text
//
// CREATED : Primal Hunt (Third Law); no source survives, written from
//           PHCShell.dll (0x10136e60 to 0x10136f35; names ours).
//
// ----------------------------------------------------------------------- //

#ifndef __SCRAMBLE_H__
#define __SCRAMBLE_H__

#ifdef PRIMAL_HUNT

#include "ltbasedefs.h"

// The ways to scramble a character; SCRAMBLE_BOTH is SCRAMBLE_NOT then
// SCRAMBLE_ROTATE
#define SCRAMBLE_NONE			0
#define SCRAMBLE_ROTATE			1	// rotated left three bits
#define SCRAMBLE_NOT			2	// every bit flipped
#define SCRAMBLE_BOTH			3

uint8	ScrambleChar(int nMethod, uint8 nChar);
LTBOOL	ScrambleString(const char* szSrc, char* szDest, uint32 nDestLen, int nMethod);

#endif // PRIMAL_HUNT

#endif // __SCRAMBLE_H__
