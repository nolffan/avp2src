// *********************************************************************** //
//  ph/lightanim.cpp -- Primal Hunt's d3d.ren, TU37
//  (0x10033790..0x10034de7), /O2.
//
//  Animated lightmaps (LMAnimStatic): the lookup tables the lightmap
//  blenders index, and the blenders themselves.  Only the console
//  variable and the two table builders are written so far.
//
//  AvP2 1.0.9.6's counterpart is w17_1006ad20_qip.cpp (0x1006c4d0) and
//  d3dr3b_o2.cpp (0x1006c510).
// *********************************************************************** //

#include "ltbasedefs.h"


// The renderer's console variable (TU12's, constructor 0x10011f36).
class CD3DConVar
{
public:
	CD3DConVar(const char* pName, float fDefault, int32* pnValue, float* pfValue);

	int32		m_nValue;
	float		m_fValue;
	float		m_fDefault;
	int32*		m_pnValue;
	float*		m_pfValue;
	const char*	m_pName;
	void*		m_hParam;
	CD3DConVar*	m_pNext;
};


// ---------------------------------------------------------------------- //
//  Globals.
// ---------------------------------------------------------------------- //
CD3DConVar	g_CV_LMAnimStatic("LMAnimStatic", 0.0f, LTNULL, LTNULL);    // 0x1007dd60  $E 0x10033790

int32		g_LMAnimRamp[64];                           // 0x1007dc60
uint16		g_LMClampTable[0x300];                      // 0x1007d660


// ---------------------------------------------------------------------- //
//  0x100337b0 -- the 16.16 intensity ramp, 0..1 over 64 steps, scaled
//  and clamped at 1.
// ---------------------------------------------------------------------- //
void d3d_InitLMAnimRamp(float fScale)
{
	uint16 i;

	for (i = 0; i < 64; i++)
	{
		float fVal = (float)i * (1.0f / 63.0f);
		fVal *= fScale;

		if (fVal > 1.0f)
			fVal = 1.0f;

		g_LMAnimRamp[i] = (int32)(fVal * 65536.0f);
	}
}


// ---------------------------------------------------------------------- //
//  0x10033810 -- the clamp table the blenders index with a channel +
//  0xff: 256 zeros, the identity, 256 x 0xff.
// ---------------------------------------------------------------------- //
void d3d_InitLMClampTable()
{
	uint16 i;

	for (i = 0; i < 0x100; i++)
	{
		g_LMClampTable[i] = 0;
		g_LMClampTable[0x100 + i] = i;
		g_LMClampTable[0x200 + i] = 0xff;
	}
}


// 0x10033840 (477 bytes), 0x10033a20 (1293), 0x10033f30 (208),
// 0x10034000 (1720), 0x100346c0 (811), 0x100349f0 (502) and 0x10034bf0
// (503) are not written yet.
