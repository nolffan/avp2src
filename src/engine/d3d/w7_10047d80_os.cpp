// ======================================================================= //
//  w7_10047d80.cpp -- d3d.ren functions written for the whole-DLL link
//: the general-plane polygon clipper.
// ======================================================================= //
#include "ltbasedefs.h"


// The vector type whose three out-of-line operators live at 0x100155f0,
// 0x10015640 and 0x10015690 (big48180.cpp's spelling; the link aliases
// carry these names onto the definitions).
struct LTVec
{
	float x, y, z;

	LTVec  sub_100155f0(const LTVec v) const;   // 0x100155f0  operator +
	LTVec  sub_10015640(const LTVec v) const;   // 0x10015640  operator -
	LTVec  sub_10015690(float v) const;         // 0x10015690  operator * (T)
};

// 0x10048150 -- the plane distance, __thiscall with the point by value
// (d3dbatchb.cpp's Plane10048150; big64cc0.cpp's spelling).
struct SPlane
{
	LTVec	m_Normal;                                   // +0x00
	float	m_fDist;                                    // +0x0c

	float	sub_10048150(LTVec v) const;
};

// The vertex this clipper walks is 0x24 bytes (`add $0x24` / `rep movsl`
// with %ecx = 9), four bytes shorter than the frustum family's 0x28 in
// d3dr3a.cpp; the tag is the one 0x10047cf0's reference is mangled with.
struct SClipVertA
{
	LTVec	m_Pos;                                      // +0x00
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
};

struct SPlaneA;

extern unsigned long g_1008be44;                        // 0x1008be44


// ======================================================================= //
//  0x10047d80 -- 975 bytes (then one `nop`).  The seventh member of the
//  Sutherland-Hodgman family (d3dr3a.cpp's 0x10002d40 and siblings): clip
//  the polygon *ppVerts / *pnVerts against the plane **ppPlane, writing
//  the result at *ppOut.  Called by 0x10047cf0 (d3dr3a_os.cpp) once per
//  plane with (&pPlane, &pVerts, &nVerts, &pOut).
//
//  The same shape as the frustum members, with the side test and the
//  crossing written through the plane and the vector operators:
//
//    * inside = plane.Dist(v) > 0.0f, stored to bInside[50] (-0x154) and
//      summed; none inside returns 0, all inside returns 1 untouched;
//    * the crossing parameter is d0 / (d0 - d1) with d0 = Dist(prev),
//      d1 = Dist(cur), guarded by the family's `|denom| > 0.00001f` ||
//      and 0 otherwise; d0 and t are spilled in 8-byte slots (`fstpl`),
//      but they are FLOATS -- icl keeps a float live across a call at full
//      precision -- as the float constants (`flds`) of the guard and of
//      `t = 0.0f` show;
//    * the position is prev + (cur - prev) * t through 0x10015640,
//      0x10015690 and 0x100155f0, copied into the new vertex;
//    * the three floats +0x18..+0x20 and the five bytes +0x12, +0x11,
//      +0x10, +0x13, +0x17 interpolated as in the family, each byte
//      through its own float local;
//    * the tail's double advance of *ppOut, as in every member.
//
//  /Os: `push %edi` x2 / `push %esi` / `push %ebx` saves, `incl` on the
//  counter and `xor %eax,%eax ; inc %eax` for the 1 (at /O2 the saves are
//  stores and the function is 1537 bytes).
//
//  IDENTICAL (was 821 of 1033 bytes differing).  Written
//  as the frustum family is (w1_10001000.cpp / w1_10002310.cpp): the
//  byte conversions through RoundF (the inlined __asm helper, which is
//  the shipped `fstps ; flds ; fistpl` with its own float slot each); the
//  lerp as an inline over (prev, cur, *ppOut, t); the crossing as an
//  inline over the vertex POSITIONS (`&pPrev->m_Pos`, which gives the
//  shipped copy of pPrev and the early *ppOut load); nInside an int32
//  compared uncast inside one `if (nInside != *pnVerts)` block with a
//  single `return 1`; and d0, the denominator and t floats.
// ======================================================================= //
// Round to nearest in the current FPU mode (0x10015190 out of line); the
// family's helper (w1_10001000.cpp): inlined, it is the shipped `fstps ;
// flds ; fistpl` with the float argument in its own slot.
inline int32 RoundF(float f)
{
	int32 n;
	__asm fld f
	__asm fistp n
	return n;
}

// The family's vertex lerp (0x10023380), over this 0x24-byte vertex.
static inline void LerpClipVertP(SClipVertA* a, SClipVertA* b, SClipVertA* dst, float t)
{
	dst->m_18 = (b->m_18 - a->m_18) * t + a->m_18;
	dst->m_1c = (b->m_1c - a->m_1c) * t + a->m_1c;
	dst->m_20 = (b->m_20 - a->m_20) * t + a->m_20;
	dst->m_12 = (uint8)RoundF((b->m_12 - a->m_12) * t + a->m_12);
	dst->m_11 = (uint8)RoundF((b->m_11 - a->m_11) * t + a->m_11);
	dst->m_10 = (uint8)RoundF((b->m_10 - a->m_10) * t + a->m_10);
	dst->m_13 = (uint8)RoundF((b->m_13 - a->m_13) * t + a->m_13);
	dst->m_17 = (uint8)RoundF((b->m_17 - a->m_17) * t + a->m_17);
}

// The crossing, as the family's ClipEdge* over the POSITION (w1's
// fix): t = d0 / (d0 - d1) through the plane distance, then the position
// a + (b - a) * t through the three vector operators.
static inline float ClipEdgePlaneA(SPlaneA** ppPlane, LTVec* a, LTVec* b, LTVec* dst)
{
	float  fD0 = ((SPlane*)*ppPlane)->sub_10048150(*a);
	float  fDenom = fD0 - ((SPlane*)*ppPlane)->sub_10048150(*b);
	float  t;

	if (fDenom < -0.00001f || fDenom > 0.00001f)
		t = fD0 / fDenom;
	else
		t = 0.0f;

	*dst = a->sub_100155f0(b->sub_10015640(*a).sub_10015690((float)t));

	return t;
}

int32 sub_10047d80(SPlaneA** ppPlane, SClipVertA** ppVerts, int32* pnVerts,
                   SClipVertA** ppOut)
{
	uint32       anInside[50];
	SClipVertA*  pOutStart;
	SClipVertA*  pV;
	SClipVertA*  pEnd;
	SClipVertA*  pPrev;
	SClipVertA*  pCur;
	uint32*      pFlag;
	int32        nInside;
	int32        i;
	int32        iPrev;

	++g_1008be44;

	nInside = 0;
	pFlag   = anInside;
	pV      = *ppVerts;
	pEnd    = pV + *pnVerts;

	for (; pV != pEnd; ++pV)
	{
		uint32 bIn = (((SPlane*)*ppPlane)->sub_10048150(pV->m_Pos) > 0.0f);

		nInside += bIn;
		*pFlag++ = bIn;
	}

	if (nInside == 0)
		return 0;

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
				float t = ClipEdgePlaneA(ppPlane, &pPrev->m_Pos, &pCur->m_Pos, &(*ppOut)->m_Pos);

				LerpClipVertP(pPrev, pCur, *ppOut, t);

				++(*ppOut);
			}

			iPrev = i;
			pPrev = pCur;
		}

		*pnVerts = *ppOut - pOutStart;
		*ppVerts = pOutStart;
		*ppOut  += *pnVerts;
	}

	return 1;
}
