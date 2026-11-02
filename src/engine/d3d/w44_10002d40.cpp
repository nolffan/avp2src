// *********************************************************************** //
//
//  w44_10002d40.cpp  --  d3d.ren, two of the 40-byte-vertex frustum clippers
//
//     0x10002d40  right   in = (x < z),   crossing x =  z
//     0x100030a0  bottom  in = (-z < y),  crossing y = -z
//
//  Dispatched by 0x1000aa90 (d3dr3b.cpp) on bits 0x10 / 0x20; the other
//  four 40-byte bodies are w1_10002310.cpp, the 32-byte twins
//  w1_10001000.cpp, whose header records how the shared shape was
//  measured.  Moved here from d3dr3a.cpp, where they were written as
//  plain C (C casts for the bytes, separate return-1 test): ~1400 of
//  ~850 bytes differed.  Flags: /O2 (lea-nop padding to 16).
//
//  w1_10001000.cpp's lever, applied: the vertex's x/y/z an LTVector at +0x00 and the
//  ClipEdge* inlines taking LTVector*; nInside int32 compared uncast; the
//  pinned pair stored z first in the source (icl emits them reversed).
//
// *********************************************************************** //

#include "ltbasedefs.h"

struct SClipVertA                                       // 0x28 bytes
{
	LTVector v;                                         // +0x00  x, y, z
	float	m_0c;                                       // +0x0c
	uint8	m_10;                                       // +0x10
	uint8	m_11;                                       // +0x11
	uint8	m_12;                                       // +0x12
	uint8	m_13;                                       // +0x13
	uint8	m_14;                                       // +0x14
	uint8	m_15;                                       // +0x15
	uint8	m_16;                                       // +0x16
	uint8	m_17;                                       // +0x17
	float	m_18;                                       // +0x18
	float	m_1c;                                       // +0x1c
	float	m_20;                                       // +0x20
	float	m_24;                                       // +0x24
};

extern uint32  g_1008be44;                              // 0x1008be44  clip counter

#define CLIP_EPSILON    0.00001f
#define CLIP_MAX_VERTS  50

// Round to nearest in the current FPU mode; 0x10015190 out of line.
inline int32 RoundF(float f)
{
	int32 n;
	__asm fld f
	__asm fistp n
	return n;
}

// 0x10023380, the vertex lerp, over the 40-byte vertex.
static inline void LerpClipVertA(SClipVertA* a, SClipVertA* b, SClipVertA* dst, float t)
{
	dst->m_18 = (b->m_18 - a->m_18) * t + a->m_18;
	dst->m_1c = (b->m_1c - a->m_1c) * t + a->m_1c;
	dst->m_20 = (b->m_20 - a->m_20) * t + a->m_20;
	dst->m_24 = (b->m_24 - a->m_24) * t + a->m_24;
	dst->m_12 = (uint8)RoundF((b->m_12 - a->m_12) * t + a->m_12);
	dst->m_11 = (uint8)RoundF((b->m_11 - a->m_11) * t + a->m_11);
	dst->m_10 = (uint8)RoundF((b->m_10 - a->m_10) * t + a->m_10);
	dst->m_13 = (uint8)RoundF((b->m_13 - a->m_13) * t + a->m_13);
	dst->m_17 = (uint8)RoundF((b->m_17 - a->m_17) * t + a->m_17);
}

static inline float ClipEdgeRightA(LTVector* a, LTVector* b, LTVector* dst)
{
	float fDenom = (b->x - a->x) - b->z + a->z;
	float t;
	float fZ;

	if (fDenom < -CLIP_EPSILON || fDenom > CLIP_EPSILON)
		t = -(a->x - a->z) / fDenom;
	else
		t = 0.0f;

	dst->y = (b->y - a->y) * t + a->y;
	fZ = (b->z - a->z) * t + a->z;
	dst->z = fZ;
	dst->x = fZ;

	return t;
}

static inline float ClipEdgeBottomA(LTVector* a, LTVector* b, LTVector* dst)
{
	float fDenom = (b->y - a->y) + b->z - a->z;
	float t;
	float fZ;

	if (fDenom < -CLIP_EPSILON || fDenom > CLIP_EPSILON)
		t = -(a->y + a->z) / fDenom;
	else
		t = 0.0f;

	dst->x = (b->x - a->x) * t + a->x;
	fZ = (b->z - a->z) * t + a->z;
	dst->z = fZ;
	dst->y = -fZ;

	return t;
}


// ----------------------------------------------------------------------- //
//  0x10002d40 -- right plane
// ----------------------------------------------------------------------- //
int32 sub_10002d40(uint32* pUnused, void** ppVertsArg, uint32* pnVertsArg, void** ppOutArg)
{
	SClipVertA** ppVerts = (SClipVertA**)ppVertsArg;
	SClipVertA** ppOut   = (SClipVertA**)ppOutArg;
	int32*       pnVerts = (int32*)pnVertsArg;
	uint32     anInside[CLIP_MAX_VERTS];
	SClipVertA*  pOutStart;
	SClipVertA*  pV;
	SClipVertA*  pEnd;
	SClipVertA*  pPrev;
	SClipVertA*  pCur;
	uint32*    pFlag;
	int32      nInside;
	int32      i;
	int32      iPrev;

	++g_1008be44;

	nInside = 0;
	pFlag   = anInside;
	pV      = *ppVerts;
	pEnd    = pV + *pnVerts;

	for (; pV != pEnd; ++pV)
	{
		uint32 bIn = (pV->v.x < pV->v.z);

		nInside += bIn;
		*pFlag++ = bIn;
	}

	if (nInside == 0)
		return LTFALSE;

	if (nInside != *pnVerts)
	{
		pOutStart = *ppOut;
		iPrev     = *pnVerts - 1;
		pPrev     = *ppVerts + iPrev;

		for (i = 0; i < *pnVerts; ++i)
		{
			pCur = *ppVerts + i;

			if (anInside[iPrev])
				*(*ppOut)++ = *pPrev;

			if (anInside[iPrev] != anInside[i])
			{
				float t = ClipEdgeRightA(&pPrev->v, &pCur->v, &(*ppOut)->v);

				LerpClipVertA(pPrev, pCur, *ppOut, t);

				++(*ppOut);
			}

			iPrev = i;
			pPrev = pCur;
		}

		*pnVerts = *ppOut - pOutStart;
		*ppVerts = pOutStart;
		*ppOut  += *pnVerts;
	}

	return LTTRUE;
}


// ----------------------------------------------------------------------- //
//  0x100030a0 -- bottom plane
// ----------------------------------------------------------------------- //
int32 sub_100030a0(uint32* pUnused, void** ppVertsArg, uint32* pnVertsArg, void** ppOutArg)
{
	SClipVertA** ppVerts = (SClipVertA**)ppVertsArg;
	SClipVertA** ppOut   = (SClipVertA**)ppOutArg;
	int32*       pnVerts = (int32*)pnVertsArg;
	uint32     anInside[CLIP_MAX_VERTS];
	SClipVertA*  pOutStart;
	SClipVertA*  pV;
	SClipVertA*  pEnd;
	SClipVertA*  pPrev;
	SClipVertA*  pCur;
	uint32*    pFlag;
	int32      nInside;
	int32      i;
	int32      iPrev;

	++g_1008be44;

	nInside = 0;
	pFlag   = anInside;
	pV      = *ppVerts;
	pEnd    = pV + *pnVerts;

	for (; pV != pEnd; ++pV)
	{
		uint32 bIn = (-pV->v.z < pV->v.y);

		nInside += bIn;
		*pFlag++ = bIn;
	}

	if (nInside == 0)
		return LTFALSE;

	if (nInside != *pnVerts)
	{
		pOutStart = *ppOut;
		iPrev     = *pnVerts - 1;
		pPrev     = *ppVerts + iPrev;

		for (i = 0; i < *pnVerts; ++i)
		{
			pCur = *ppVerts + i;

			if (anInside[iPrev])
				*(*ppOut)++ = *pPrev;

			if (anInside[iPrev] != anInside[i])
			{
				float t = ClipEdgeBottomA(&pPrev->v, &pCur->v, &(*ppOut)->v);

				LerpClipVertA(pPrev, pCur, *ppOut, t);

				++(*ppOut);
			}

			iPrev = i;
			pPrev = pCur;
		}

		*pnVerts = *ppOut - pOutStart;
		*ppVerts = pOutStart;
		*ppOut  += *pnVerts;
	}

	return LTTRUE;
}


