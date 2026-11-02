// *********************************************************************** //
//
//  w1_10001000.cpp  --  d3d.ren, the 32-byte-vertex frustum clippers
//
//  Five of the six single-plane Sutherland-Hodgman clippers dispatched by
//  0x1000f4b0 (d3dr3c_os.cpp); the sixth, 0x10001670, is in d3dr3c.cpp.
//
//     0x10001000  near    in = (z >= g_1008b908), crossing z = g_1008b908
//     0x10001330  left    in = (-z < x),          crossing x = -z
//     0x10001990  right   in = (x < z),           crossing x =  z
//     0x10001cb0  bottom  in = (-z < y),          crossing y = -z
//     0x10001fe0  far     in = (z <= g_1008b910), crossing z = g_1008b910
//
//  Every side test, denominator, numerator and crossing store was read off
//  the shipped bytes (a normalised diff of each body against 0x10001000).
//  Flags: /O2 -- each body is followed by multi-byte lea-nop padding to 16.
//  The first parameter is never read (see d3dr3c.cpp's note on the driver).
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
//    * the ClipEdge* inlines take the vertex's POSITION, an LTVector
//      at +0x00 (`&pPrev->v`), not the vertex: the shipped code copies
//      pPrev into %edx for it and loads a->z ONCE (kept on the x87 stack
//      for the numerator), and loads *ppOut before the epsilon compare.
//      Passing the ClipVert* itself gave neither -- 33 -> 15 differing
//      lines on 0x10001000 (a `(CVec*)&pPrev->x` cast does the same).
//    * nInside is an int32 compared with *pnVerts uncast: as uint32
//      the `return LTFALSE` block sat after the return-1 block instead
//      of last, 16-aligned, as shipped.  15 -> identical.
//    * 0x10001990 stores the pinned plane value z first, then x, in the
//      source: icl emits them reversed (x, then z) as shipped.
//  Earlier dead ends, for the record: fNum first, float locals
//  for a->z, parameter order, references, `?:`, `>= && <=`, /Qip,
//  /Qip_no_inlining, /Qrcd, folding the lerp into the edge function;
//  nested if / else-if / goto / result var for the return placement.
// *********************************************************************** //

#include "ltbasedefs.h"

struct ClipVert                                         // 32 bytes
{
	LTVector v;                                         // +0x00  x, y, z
	float  rhw;                                         // +0x0c
	uint8  b, g, r, a;                                  // +0x10..+0x13
	uint8  sb, sg, sr, sa;                              // +0x14..+0x17
	float  tu;                                          // +0x18
	float  tv;                                          // +0x1c
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

// 0x10023380, the vertex lerp (out of line there, inlined here).
static inline void LerpClipVert(ClipVert* a, ClipVert* b, ClipVert* dst, float t)
{
	dst->tu = (b->tu - a->tu) * t + a->tu;
	dst->tv = (b->tv - a->tv) * t + a->tv;
	dst->r  = (uint8)RoundF((b->r  - a->r)  * t + a->r);
	dst->g  = (uint8)RoundF((b->g  - a->g)  * t + a->g);
	dst->b  = (uint8)RoundF((b->b  - a->b)  * t + a->b);
	dst->a  = (uint8)RoundF((b->a  - a->a)  * t + a->a);
	dst->sa = (uint8)RoundF((b->sa - a->sa) * t + a->sa);
}

static inline float ClipEdgeNear(LTVector* a, LTVector* b, LTVector* dst)
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

static inline float ClipEdgeLeft(LTVector* a, LTVector* b, LTVector* dst)
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

static inline float ClipEdgeRight(LTVector* a, LTVector* b, LTVector* dst)
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

static inline float ClipEdgeBottom(LTVector* a, LTVector* b, LTVector* dst)
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

static inline float ClipEdgeFar(LTVector* a, LTVector* b, LTVector* dst)
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
//  0x10001000 -- near plane
// ----------------------------------------------------------------------- //
LTBOOL sub_10001000(uint32* pUnused, ClipVert** ppVerts, int32* pnVerts, ClipVert** ppOut)
{
	uint32     anInside[CLIP_MAX_VERTS];
	ClipVert*  pOutStart;
	ClipVert*  pV;
	ClipVert*  pEnd;
	ClipVert*  pPrev;
	ClipVert*  pCur;
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
				float t = ClipEdgeNear(&pPrev->v, &pCur->v, &(*ppOut)->v);

				LerpClipVert(pPrev, pCur, *ppOut, t);

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
//  0x10001330 -- left plane
// ----------------------------------------------------------------------- //
LTBOOL sub_10001330(uint32* pUnused, ClipVert** ppVerts, int32* pnVerts, ClipVert** ppOut)
{
	uint32     anInside[CLIP_MAX_VERTS];
	ClipVert*  pOutStart;
	ClipVert*  pV;
	ClipVert*  pEnd;
	ClipVert*  pPrev;
	ClipVert*  pCur;
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
				float t = ClipEdgeLeft(&pPrev->v, &pCur->v, &(*ppOut)->v);

				LerpClipVert(pPrev, pCur, *ppOut, t);

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
//  0x10001990 -- right plane
// ----------------------------------------------------------------------- //
LTBOOL sub_10001990(uint32* pUnused, ClipVert** ppVerts, int32* pnVerts, ClipVert** ppOut)
{
	uint32     anInside[CLIP_MAX_VERTS];
	ClipVert*  pOutStart;
	ClipVert*  pV;
	ClipVert*  pEnd;
	ClipVert*  pPrev;
	ClipVert*  pCur;
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
				float t = ClipEdgeRight(&pPrev->v, &pCur->v, &(*ppOut)->v);

				LerpClipVert(pPrev, pCur, *ppOut, t);

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
//  0x10001cb0 -- bottom plane
// ----------------------------------------------------------------------- //
LTBOOL sub_10001cb0(uint32* pUnused, ClipVert** ppVerts, int32* pnVerts, ClipVert** ppOut)
{
	uint32     anInside[CLIP_MAX_VERTS];
	ClipVert*  pOutStart;
	ClipVert*  pV;
	ClipVert*  pEnd;
	ClipVert*  pPrev;
	ClipVert*  pCur;
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
				float t = ClipEdgeBottom(&pPrev->v, &pCur->v, &(*ppOut)->v);

				LerpClipVert(pPrev, pCur, *ppOut, t);

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
//  0x10001fe0 -- far plane
// ----------------------------------------------------------------------- //
LTBOOL sub_10001fe0(uint32* pUnused, ClipVert** ppVerts, int32* pnVerts, ClipVert** ppOut)
{
	uint32     anInside[CLIP_MAX_VERTS];
	ClipVert*  pOutStart;
	ClipVert*  pV;
	ClipVert*  pEnd;
	ClipVert*  pPrev;
	ClipVert*  pCur;
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
				float t = ClipEdgeFar(&pPrev->v, &pCur->v, &(*ppOut)->v);

				LerpClipVert(pPrev, pCur, *ppOut, t);

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

