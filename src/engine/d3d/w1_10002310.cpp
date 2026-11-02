// *********************************************************************** //
//
//  w1_10002310.cpp  --  d3d.ren, the 40-byte-vertex frustum clippers
//
//  Four of the six single-plane clippers dispatched by 0x1000aa90 (the
//  40-byte twin of 0x1000f4b0); 0x10002d40 and 0x100030a0 are in
//  d3dr3a.cpp.  The same source as w1_10001000.cpp over the 40-byte vertex
//  (two more texture coordinates, +0x20/+0x24; `idiv $0x28` in the tail).
//
//     0x10002310  near    in = (z >= g_1008b908), crossing z = g_1008b908
//     0x10002670  left    in = (-z < x),          crossing x = -z
//     0x100029e0  top     in = (y < z),           crossing y =  z
//     0x10003410  far     in = (z <= g_1008b910), crossing z = g_1008b910
//
//  The references (d3dr3b.cpp) spell the lists `void **` and the count
//  `uint32 *`; the bodies cast them.  Flags: /O2 (lea-nop padding to 16).
//
//  HOW THE SOURCE IS SHAPED, and each point was measured on 0x10001000
//  (a byte diff against the shipped bytes; S/O = differing shipped/our lines):
//
//    * the colour channels go through RoundF, an __asm `fld ; fistp`
//      helper -- the out-of-line copy of it is 0x10015190 (d3dbatchd_o2.cpp
//      writes it the same way).  icl 4.0 INLINES a function containing
//      __asm, and the inlined copy is exactly the shipped
//      `fstps tmp ; flds tmp ; fistpl n ; mov n,%dl`: the float argument
//      is materialised in its own slot, then loaded and stored by the asm.
//      It also explains the `push %ebp ; mov %esp,%ebp` frame every body
//      has (an __asm function gets a frame pointer; plain C at /O2 gives
//      an %esp frame).  No C cast reproduces it -- all were measured,
//      and /Qrcd emits a 64-bit fistpll -- and it is also the only
//      spelling with the shipped SEMANTICS: fistp rounds in the current
//      mode (nearest), where a C cast truncates.  Pure C, /O2: ~1300 of
//      ~820 bytes differ; /O2 /Qrcd: ~800; with RoundF: ~30 lines.
//    * tu/tv and the five bytes are the vertex LERP 0x10023380, inlined:
//      the field order is its own (+0x18, +0x1c, [+0x20, +0x24,] +0x12,
//      +0x11, +0x10, +0x13, +0x17), and the shipped body reloads *ppOut
//      once before it and keeps it in a slot -- the evaluation of the
//      inline call's `dst` argument.  S 182 -> 150.
//    * t and the plane-pinned x/y/z are a second inline (ClipEdge*): it
//      is what gives the Sutherland-Hodgman loop the shipped register
//      assignment (i in %esi, pCur %edi, pPrev %ebx).  S 144 -> 35.
//    * the flag array is 50 dwords: -0x114 down to the five 4-byte
//      RoundF slots at -0x4c, which makes the frame 0x144 exactly.
//    * `if (nInside != *pnVerts) { ... }` then one `return LTTRUE` gives
//      the single shared success epilogue the tail jumps back to.  35 -> 30.
//
//    * (from the fix to w1_10001000.cpp) the ClipEdge*
//      inlines take the vertex POSITION, an LTVector at +0x00
//      (`&pPrev->v`), not the vertex: that is the shipped %edx copy of
//      pPrev, the single a->z load kept on the x87 stack, and *ppOut
//      loaded before the epsilon compare; nInside is an int32 compared
//      with *pnVerts uncast, which puts the `return LTFALSE` block last,
//      as shipped; the top plane stores z, then y, in the source (icl
//      emits them reversed).  All four byte-identical.
//  Earlier dead ends, for the record: fNum first, float locals
//  for a->z, parameter order, references, `?:`, `>= && <=`, /Qip,
//  /Qip_no_inlining, /Qrcd, folding the lerp into the edge function;
//  nested if / else-if / goto / result var for the return placement.
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
extern float   g_1008b908;                              // 0x1008b908  near plane z
extern float   g_1008b910;                              // 0x1008b910  far plane z

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

static inline float ClipEdgeNearA(LTVector* a, LTVector* b, LTVector* dst)
{
	float fDenom = b->z - a->z;
	float t;

	if (fDenom < -CLIP_EPSILON || fDenom > CLIP_EPSILON)
		t = (g_1008b908 - a->z) / fDenom;
	else
		t = 0.0f;

	dst->x = (b->x - a->x) * t + a->x;
	dst->y = (b->y - a->y) * t + a->y;
	dst->z = g_1008b908;

	return t;
}

static inline float ClipEdgeLeftA(LTVector* a, LTVector* b, LTVector* dst)
{
	float fDenom = (b->x - a->x) + b->z - a->z;
	float t;
	float fZ;

	if (fDenom < -CLIP_EPSILON || fDenom > CLIP_EPSILON)
		t = -(a->x + a->z) / fDenom;
	else
		t = 0.0f;

	dst->y = (b->y - a->y) * t + a->y;
	fZ = (b->z - a->z) * t + a->z;
	dst->z = fZ;
	dst->x = -fZ;

	return t;
}

static inline float ClipEdgeTopA(LTVector* a, LTVector* b, LTVector* dst)
{
	float fDenom = (b->y - a->y) - b->z + a->z;
	float t;
	float fZ;

	if (fDenom < -CLIP_EPSILON || fDenom > CLIP_EPSILON)
		t = -(a->y - a->z) / fDenom;
	else
		t = 0.0f;

	dst->x = (b->x - a->x) * t + a->x;
	fZ = (b->z - a->z) * t + a->z;
	dst->z = fZ;
	dst->y = fZ;

	return t;
}

static inline float ClipEdgeFarA(LTVector* a, LTVector* b, LTVector* dst)
{
	float fDenom = b->z - a->z;
	float t;

	if (fDenom < -CLIP_EPSILON || fDenom > CLIP_EPSILON)
		t = (g_1008b910 - a->z) / fDenom;
	else
		t = 0.0f;

	dst->x = (b->x - a->x) * t + a->x;
	dst->y = (b->y - a->y) * t + a->y;
	dst->z = g_1008b910;

	return t;
}


// ----------------------------------------------------------------------- //
//  0x10002310 -- near plane
// ----------------------------------------------------------------------- //
int32 sub_10002310(uint32* pUnused, void** ppVertsArg, uint32* pnVertsArg, void** ppOutArg)
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
		uint32 bIn = (pV->v.z >= g_1008b908);

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
				float t = ClipEdgeNearA(&pPrev->v, &pCur->v, &(*ppOut)->v);

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
//  0x10002670 -- left plane
// ----------------------------------------------------------------------- //
int32 sub_10002670(uint32* pUnused, void** ppVertsArg, uint32* pnVertsArg, void** ppOutArg)
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
		uint32 bIn = (-pV->v.z < pV->v.x);

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
				float t = ClipEdgeLeftA(&pPrev->v, &pCur->v, &(*ppOut)->v);

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
//  0x100029e0 -- top plane
// ----------------------------------------------------------------------- //
int32 sub_100029e0(uint32* pUnused, void** ppVertsArg, uint32* pnVertsArg, void** ppOutArg)
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
		uint32 bIn = (pV->v.y < pV->v.z);

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
				float t = ClipEdgeTopA(&pPrev->v, &pCur->v, &(*ppOut)->v);

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
//  0x10003410 -- far plane
// ----------------------------------------------------------------------- //
int32 sub_10003410(uint32* pUnused, void** ppVertsArg, uint32* pnVertsArg, void** ppOutArg)
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
		uint32 bIn = (pV->v.z <= g_1008b910);

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
				float t = ClipEdgeFarA(&pPrev->v, &pCur->v, &(*ppOut)->v);

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

