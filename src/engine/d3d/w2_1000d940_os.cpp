// *********************************************************************** //
//
//  w2_1000d940_os.cpp -- sub_1000d940 (written for the d3d.ren link).
//
//  The perspective divide of one clipped vertex through the transform
//  object's +0x31c..+0x330 terms: 1/z into +0x0c, then x, y and z scaled.
//  The same body sub_1000a8a0 runs inline for each vertex after its
//  clipper (w2_1000a8a0_os.cpp), written the same way: x is scaled by the
//  reciprocal still on the x87 stack, y and z by the stored +0x0c.
//
//  Five callers pass (pVert, g_1008b880); big0f6f0.cpp and big64cc0.cpp
//  spell it `void sub_1000d940(RVert*, uint8*)`, which is the definition
//  here; the extern "C" spellings in big6d910.cpp, big70d10.cpp and
//  bigb910.cpp are aliased.
//
//  /Os: 0x90 padding.
//
// *********************************************************************** //

#include "ltbasedefs.h"


// The clipped vertex: position, 1/w (the rest is not touched).
struct RVert
{
	LTVector m_vPos;                                        // +0x00
	float    m_fRHW;                                        // +0x0c
};

// The transform object's perspective terms.
struct XForm1000d940
{
	uint8   m_pad_000[0x31c];                               // +0x000
	float   m_f31c;                                         // +0x31c
	float   m_f320;                                         // +0x320
	float   m_f324;                                         // +0x324
	float   m_f328;                                         // +0x328
	float   m_f32c;                                         // +0x32c
	float   m_f330;                                         // +0x330
};


void sub_1000d940(RVert* pVert, uint8* pXForm)
{
	XForm1000d940* pM = (XForm1000d940*)pXForm;
	float fOOW;

	fOOW = 1.0f / pVert->m_vPos.z;
	pVert->m_fRHW = fOOW;
	pVert->m_vPos.x = (pVert->m_vPos.x * pM->m_f31c + pVert->m_vPos.z * pM->m_f320) * fOOW;
	pVert->m_vPos.y = (pVert->m_vPos.y * pM->m_f324 + pVert->m_vPos.z * pM->m_f328) * pVert->m_fRHW;
	pVert->m_vPos.z = (pVert->m_vPos.z * pM->m_f32c + pM->m_f330) * pVert->m_fRHW;
}
