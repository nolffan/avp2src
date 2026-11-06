// *********************************************************************** //
//  ph/lightanim.cpp -- Primal Hunt's d3d.ren, TU37
//  (0x10033790..0x10034de7), /O2.
//
//  Animated lightmaps (LMAnimStatic): the lookup tables the lightmap
//  blenders index, and the blenders themselves.  Written: the console
//  variable, the two table builders and the texel lighter 0x10033f30
//  (near: same size and shape, the registers differ: retail keeps the
//  light in %edi, cl 8966 here in %esi).  Not yet written: 0x10033840,
//  0x10033a20, 0x10034000, 0x100346c0, 0x100349f0 and 0x10034bf0.
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


// 0x10033840 (477 bytes) and 0x10033a20 (1293) are not written yet.


// The high half of a 32x32 multiply: an __asm helper, inlined (the
// operands and the result go through memory around a one-operand imul at
// every use; AvP2 1.0.9.6's w17_1006ad20_qip.cpp has the same helper).
inline int32 MulHigh(int32 a, int32 b)
{
	int32 r;
	__asm mov  eax, a
	__asm imul b
	__asm mov  r, edx
	return r;
}

// A dynamic light, set up in lightmap space.
struct LMLightInfo
{
	uint8		m_pad_00[0x30];
	float		m_fDist;                                // +0x30
	uint8		m_pad_34[0x38 - 0x34];
	float		m_fInvRadius;                           // +0x38
	uint8		m_pad_3c[0x4c - 0x3c];
	int32		m_nColor[3];                            // +0x4c (16.16)
};


// ---------------------------------------------------------------------- //
//  0x10033f30 -- light one 5:5:5 texel, attenuated through the ramp.
// ---------------------------------------------------------------------- //
void d3d_LightTexel(uint16 *pTexel, LMLightInfo *pInfo)
{
	int32 nAtten, r, g, b;

	nAtten = g_LMAnimRamp[(int32)((1.0f - pInfo->m_fDist * pInfo->m_fInvRadius) * 63.0f)];
	r = (*pTexel & 0x7c00) >> 7;
	r += MulHigh(nAtten, pInfo->m_nColor[0]);
	g = (*pTexel & 0x3e0) >> 2;
	g += MulHigh(nAtten, pInfo->m_nColor[1]);
	b = (*pTexel & 0x1f) << 3;
	b += MulHigh(nAtten, pInfo->m_nColor[2]);

	*pTexel = (uint16)(((g_LMClampTable[r + 0xff] << 7) & 0x7c00) |
		((g_LMClampTable[g + 0xff] << 2) & 0x3e0) |
		(g_LMClampTable[b + 0xff] >> 3));
}


// 0x10034000 (1720 bytes), 0x100346c0 (811), 0x100349f0 (502) and
// 0x10034bf0 (503) are not written yet.
