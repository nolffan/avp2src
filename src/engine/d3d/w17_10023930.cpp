// *********************************************************************** //
//
//  w17_10023930.cpp -- 0x10023930, a gamma-corrected packed colour.  /O2:
//  multi-byte nop padding before it and inside, fsqrt inline (intrinsics
//  on), and the long shld/neg float-to-int expansion (no /Qrcd).  The
//  double conversion temporaries give it icl's 14-byte stack-aligning
//  entry.
//
//  The fog-colour callers (bigb910.cpp, w2_10011620_os.cpp,
//  w3_10014580_os.cpp) hand it g_1008caf4/f8/fc -- three FLOATS, 0..1,
//  that their declarations carry as dwords; the name is extern "C", so
//  the float parameters here link to their `uint32` spelling unchanged.
//  On the first call a 256-entry curve is built,
//  table[i] = sqrt(i / 512) * 255 clamped to 0..255 (a function-static
//  object: icl's guard bit at 0x1008cbf0, the table at 0x1008cbf4); the
//  answer is 0x00RRGGBB with each channel looked up through it.
// *********************************************************************** //

#include "ltbasedefs.h"
#include <math.h>

struct GammaTable10023930
{
	uint8	m_Table[256];

	GammaTable10023930()
	{
		uint32	i;
		float	fVal;

		for (i = 0; i < 256; i++)
		{
			fVal = (float)sqrt((float)i * (1.0f / 512.0f)) * 255.0f;
			if (fVal < 0.0f)
				fVal = 0.0f;
			else if (fVal > 255.0f)
				fVal = 255.0f;

			m_Table[i] = (uint8)fVal;
		}
	}
};

extern "C" uint32 sub_10023930(float r, float g, float b)
{
	static GammaTable10023930 s_Gamma;
	uint8	nR, nG, nB;

	nR = s_Gamma.m_Table[(uint8)(r * 255.0f)];
	nG = s_Gamma.m_Table[(uint8)(g * 255.0f)];
	nB = s_Gamma.m_Table[(uint8)(b * 255.0f)];

	return (nR << 16) | (nG << 8) | nB;
}
