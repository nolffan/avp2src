// ----------------------------------------------------------------------- //
//
// MODULE  : Scramble.cpp
//
// PURPOSE : Primal Hunt: scramble and unscramble a string a character at
//           a time, so a message doesn't show in the dll as text
//
// CREATED : Primal Hunt (Third Law); no source survives, written from
//           PHCShell.dll (0x10136e60 to 0x10136f35; names ours).
//
// ----------------------------------------------------------------------- //

#include "stdafx.h"

#ifdef PRIMAL_HUNT

#include "Scramble.h"

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	ScrambleChar()
//
//	PURPOSE:	Scramble one character (PHCShell 0x10136e60)
//
// ----------------------------------------------------------------------- //

uint8 ScrambleChar(int nMethod, uint8 nChar)
{
	switch(nMethod)
	{
		case SCRAMBLE_NONE:
			return nChar;

		case SCRAMBLE_ROTATE:
			nChar = (nChar >> 5) | (nChar << 3);
			break;

		case SCRAMBLE_NOT:
			nChar = ~nChar;
			break;

		case SCRAMBLE_BOTH:
			nChar = ScrambleChar(SCRAMBLE_NOT, nChar);
			nChar = ScrambleChar(SCRAMBLE_ROTATE, nChar);
			break;

		default:
			break;
	}

	return nChar;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	ScrambleString()
//
//	PURPOSE:	Scramble a string into a buffer, as much of it as fits
//				(PHCShell 0x10136ec0)
//
// ----------------------------------------------------------------------- //

LTBOOL ScrambleString(const char* szSrc, char* szDest, uint32 nDestLen, int nMethod)
{
	uint32 i;

	for(i = 0; szSrc[i] && i < nDestLen - 1; i++)
	{
		szDest[i] = ScrambleChar(nMethod, szSrc[i]);
	}

	szDest[i] = '\0';

	return LTTRUE;
}

#endif // PRIMAL_HUNT
