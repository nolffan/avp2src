// *********************************************************************** //
//
//  MODULE:   W18_1003BB10_OS (.CPP)  -- /Os
//
//  PURPOSE:  0x1003bb10 .. 0x1003d345 (6,198 bytes), slot +0x00 of the
//            vtable at 0x10089530 -- the class d3dr2d_os.cpp's 0x1003b924
//            constructs (vptr, +0x28 = 0, 32 bytes at +0x2c cleared).
//            Reached only through that table; written for the d3d-link.
//
//  WHAT IT IS.  Draw one TL polygon (32-byte D3DTLVERTEX-shaped vertices,
//  FVF 0x1c4) as a TRIANGLEFAN, `ret $0xc`: (pVerts, nVerts, nFlags).
//
//    * fewer than three vertices: nothing, answer 1;
//    * g_n1008b874 (the clip flags) = 0x3f;
//    * if neither this->+0x28 nor bit 0x40 of nFlags is set, the
//      polygon is handed to the out-of-line clipper 0x10023300
//      (&pVerts, &nVerts) and, if that keeps anything, drawn as is;
//    * otherwise the near plane is swapped for 0.01 (0x3c23d70a), every
//      vertex is moved through g_Mat1008bb5c in place (ltmatrix.h's
//      MatVMul_InPlace, the three rows at 0x1008bb5c/6c/7c), the polygon
//      is run through the six-plane homogeneous clipper INLINE -- the
//      same source as big3c40.cpp's 0x10003c40 (the planes, their order
//      1/4/8/0x10/0x20/2, the five byte lerps +0x12/+0x11/+0x10/+0x13/
//      +0x17 and the 0xdc-byte per-pass frame blocks are identical;
//      here pIn/nIn are pVerts/nVerts themselves) -- then projected
//      through g_Mat1008ba1c with the homogeneous divide, z taken at
//      z + 7.0 with its own 1/w, which becomes the RHW; drawn, and the
//      near plane put back.  A polygon the clipper rejects returns
//      straight out, WITHOUT restoring the near plane (0x1003d341 jumps
//      to the shared exit) -- that is the shipped behaviour and it is
//      kept.
//
//  Every exit but the first answers 0.
//
//  FLAGS.  /Os: `*pOut = *pPrev` is a `rep movsl` of 8 dwords (/O2 --
//  big3c40.cpp -- copies field by field), the padding after the last
//  sunk block is 0x90s, and the call to 0x10023300 pushes its arguments.
//  The float-to-byte conversions in the shipped bytes are a bare `fistpl`
//  (`fstps T ; flds T ; fistpl I`), which is icl's /Qrcd; there is no
//  /Os /Qrcd flag group, so at /Os each of the thirty conversions expands
//  to the fnstcw/fldcw sequence -- a NEAR, logic identical.
//
// *********************************************************************** //

#include "ltbasedefs.h"


// ---------------------------------------------------------------------- //
//  The vertex -- big3c40.cpp's Vert3c40, the same 32 bytes (spelled the
//  same so the clipper globals below link to the same symbols).
// ---------------------------------------------------------------------- //
struct Vec3c40
{
	float			x;                                  // +0x00
	float			y;                                  // +0x04
	float			z;                                  // +0x08
};

union VColor3c40
{
	uint32			m_n;
	uint8			m_b[4];
};

struct Vert3c40
{
	Vec3c40			m_vPos;                             // +0x00
	float			m_fRHW;                             // +0x0c
	VColor3c40		m_Diffuse10;                        // +0x10
	VColor3c40		m_Specular14;                       // +0x14
	float			m_fU0;                              // +0x18
	float			m_fV0;                              // +0x1c
};                                                      // = 0x20

// big17c14.cpp's 4x4.
struct LTMat
{
	float			m[4][4];
};

// IDirect3DDevice7 -- DrawPrimitive, +0x64.
struct D3DDevice7;
struct D3DDevice7Vtbl
{
	void*	m_pad_00[25];                                   // +0x00..+0x60
	long	(__stdcall* DrawPrimitive)(D3DDevice7*, uint32 nPrimType,
	                                   uint32 dwFVF, void* pVerts,
	                                   uint32 nVerts, uint32 dwFlags);// +0x64
};
struct D3DDevice7 { D3DDevice7Vtbl* lpVtbl; };

extern D3DDevice7*		g_pD3DDevice;                   // 0x1008d988

extern uint32			g_n1008b874;                    // 0x1008b874  clip flags
extern uint32			g_n1008d7a0;                    // 0x1008d7a0  UseD3DClip
extern Vert3c40*		g_p1008b87c;                    // 0x1008b87c  clip output bank
extern float			g_f1008b908;                    // 0x1008b908  near plane
extern float			g_f1008b910;                    // 0x1008b910  far plane
extern uint32			g_n1008be44;                    // 0x1008be44  clip passes run
extern LTMat			g_Mat1008ba1c;                  // 0x1008ba1c  projection
extern LTMat			g_Mat1008bb5c;                  // 0x1008bb5c  world -> camera

// 0x10023300 -- the out-of-line clipper forwarder (d3dr2b.cpp).
uint32 sub_10023300(void* pA, void* pB);

#define W18_D3DPT_TRIANGLEFAN       6
#define W18_D3DFVF_TLVERTEX         0x1c4


// ltmatrix.h's inline LTMatrix::Apply(LTVector&): three scalar
// temporaries, then the three stores.
static inline void W18Apply(LTMat& mat, Vec3c40& s)
{
	float x, y, z;

	x = s.x*mat.m[0][0] + s.y*mat.m[0][1] + s.z*mat.m[0][2] + mat.m[0][3];
	y = s.x*mat.m[1][0] + s.y*mat.m[1][1] + s.z*mat.m[1][2] + mat.m[1][3];
	z = s.x*mat.m[2][0] + s.y*mat.m[2][1] + s.z*mat.m[2][2] + mat.m[2][3];

	s.x = x;
	s.z = z;
	s.y = y;
}


// Round to nearest in the current FPU mode (w1_10001000.cpp's helper;
// 0x10015190 out of line): icl inlines a function holding __asm as
// `fstps f ; flds f ; fistpl n` -- the thirty bare `fistpl` sites.
inline int32 RoundF(float f)
{
	int32 n;
	__asm fld f
	__asm fistp n
	return n;
}

// ======================================================================= //
//  THE CLIPPER -- 0x1000f4b0 and its six plane bodies, which the shipped
//  function INLINES.  The bodies are the ones w1_10001000.cpp /
//  w44_10001670.cpp compile byte-identical out of line, on this file's
//  32-byte Vert3c40; the driver is d3dr3c_os.cpp's 0x1000f4b0 (ClipPoly;
//  the planes in order 0x10001000 near, 0x10001330 left, 0x10001670 top,
//  0x10001990 right, 0x10001cb0 bottom, 0x10001fe0 far).
//  Inlined, the driver's pVerts / nVerts keep their own frame slots
//  (-0x590 / -0x58c), stored and reloaded around every plane, because
//  their addresses are handed to the plane bodies.
// ======================================================================= //
#define CLIP_EPSILON    0.00001f
#define CLIP_MAX_VERTS  50

static inline void LerpVert3c40(Vert3c40* a, Vert3c40* b, Vert3c40* dst, float t)
{
	dst->m_fU0 = (b->m_fU0 - a->m_fU0) * t + a->m_fU0;
	dst->m_fV0 = (b->m_fV0 - a->m_fV0) * t + a->m_fV0;
	dst->m_Diffuse10.m_b[2]  = (uint8)RoundF((b->m_Diffuse10.m_b[2]  - a->m_Diffuse10.m_b[2])  * t + a->m_Diffuse10.m_b[2]);
	dst->m_Diffuse10.m_b[1]  = (uint8)RoundF((b->m_Diffuse10.m_b[1]  - a->m_Diffuse10.m_b[1])  * t + a->m_Diffuse10.m_b[1]);
	dst->m_Diffuse10.m_b[0]  = (uint8)RoundF((b->m_Diffuse10.m_b[0]  - a->m_Diffuse10.m_b[0])  * t + a->m_Diffuse10.m_b[0]);
	dst->m_Diffuse10.m_b[3]  = (uint8)RoundF((b->m_Diffuse10.m_b[3]  - a->m_Diffuse10.m_b[3])  * t + a->m_Diffuse10.m_b[3]);
	dst->m_Specular14.m_b[3] = (uint8)RoundF((b->m_Specular14.m_b[3] - a->m_Specular14.m_b[3]) * t + a->m_Specular14.m_b[3]);
}


static inline float ClipEdgeNear(Vec3c40* a, Vec3c40* b, Vec3c40* dst)
{
	float fDenom = b->z - a->z;
	float t;

	if (fDenom < -CLIP_EPSILON || fDenom > CLIP_EPSILON)
		t = (g_f1008b908 - a->z) / fDenom;
	else
		t = 0.0f;

	dst->x = (b->x - a->x) * t + a->x;
	dst->y = (b->y - a->y) * t + a->y;
	dst->z = g_f1008b908;

	return t;
}


static inline float ClipEdgeLeft(Vec3c40* a, Vec3c40* b, Vec3c40* dst)
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


static inline float ClipEdgeTop(Vec3c40* a, Vec3c40* b, Vec3c40* dst)
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


static inline float ClipEdgeRight(Vec3c40* a, Vec3c40* b, Vec3c40* dst)
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


static inline float ClipEdgeBottom(Vec3c40* a, Vec3c40* b, Vec3c40* dst)
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


static inline float ClipEdgeFar(Vec3c40* a, Vec3c40* b, Vec3c40* dst)
{
	float fDenom = b->z - a->z;
	float t;

	if (fDenom < -CLIP_EPSILON || fDenom > CLIP_EPSILON)
		t = (g_f1008b910 - a->z) / fDenom;
	else
		t = 0.0f;

	dst->x = (b->x - a->x) * t + a->x;
	dst->y = (b->y - a->y) * t + a->y;
	dst->z = g_f1008b910;

	return t;
}


// 0x10001000
// Each plane body opens `pV = &(*ppVerts)[0];` rather than
// `pV = *ppVerts;` -- w23_100525b0_os.cpp's finding: it keeps the
// inlined driver's pVerts a memory slot, as the shipped 0x1003bb10 has it
// (normalised instruction diff 339 -> 306).
static inline LTBOOL ClipPlaneNear(uint32* pUnused, Vert3c40** ppVerts, int32* pnVerts, Vert3c40** ppOut)
{
	uint32     anInside[CLIP_MAX_VERTS];
	Vert3c40*  pOutStart;
	Vert3c40*  pV;
	Vert3c40*  pEnd;
	Vert3c40*  pPrev;
	Vert3c40*  pCur;
	uint32*    pFlag;
	int32      nInside;
	int32      i;
	int32      iPrev;

	++g_n1008be44;

	nInside = 0;
	pFlag   = anInside;
	pV      = &(*ppVerts)[0];
	pEnd    = pV + *pnVerts;

	for (; pV != pEnd; ++pV)
	{
		uint32 bIn = (pV->m_vPos.z >= g_f1008b908);

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
				float t = ClipEdgeNear(&pPrev->m_vPos, &pCur->m_vPos, &(*ppOut)->m_vPos);

				LerpVert3c40(pPrev, pCur, *ppOut, t);

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


// 0x10001330
static inline LTBOOL ClipPlaneLeft(uint32* pUnused, Vert3c40** ppVerts, int32* pnVerts, Vert3c40** ppOut)
{
	uint32     anInside[CLIP_MAX_VERTS];
	Vert3c40*  pOutStart;
	Vert3c40*  pV;
	Vert3c40*  pEnd;
	Vert3c40*  pPrev;
	Vert3c40*  pCur;
	uint32*    pFlag;
	int32      nInside;
	int32      i;
	int32      iPrev;

	++g_n1008be44;

	nInside = 0;
	pFlag   = anInside;
	pV      = &(*ppVerts)[0];
	pEnd    = pV + *pnVerts;

	for (; pV != pEnd; ++pV)
	{
		uint32 bIn = (-pV->m_vPos.z < pV->m_vPos.x);

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
				float t = ClipEdgeLeft(&pPrev->m_vPos, &pCur->m_vPos, &(*ppOut)->m_vPos);

				LerpVert3c40(pPrev, pCur, *ppOut, t);

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


// 0x10001670
static inline LTBOOL ClipPlaneTop(uint32* pUnused, Vert3c40** ppVerts, int32* pnVerts, Vert3c40** ppOut)
{
	uint32     anInside[CLIP_MAX_VERTS];
	Vert3c40*  pOutStart;
	Vert3c40*  pV;
	Vert3c40*  pEnd;
	Vert3c40*  pPrev;
	Vert3c40*  pCur;
	uint32*    pFlag;
	int32      nInside;
	int32      i;
	int32      iPrev;

	++g_n1008be44;

	nInside = 0;
	pFlag   = anInside;
	pV      = &(*ppVerts)[0];
	pEnd    = pV + *pnVerts;

	for (; pV != pEnd; ++pV)
	{
		uint32 bIn = (pV->m_vPos.y < pV->m_vPos.z);

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
				float t = ClipEdgeTop(&pPrev->m_vPos, &pCur->m_vPos, &(*ppOut)->m_vPos);

				LerpVert3c40(pPrev, pCur, *ppOut, t);

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


// 0x10001990
static inline LTBOOL ClipPlaneRight(uint32* pUnused, Vert3c40** ppVerts, int32* pnVerts, Vert3c40** ppOut)
{
	uint32     anInside[CLIP_MAX_VERTS];
	Vert3c40*  pOutStart;
	Vert3c40*  pV;
	Vert3c40*  pEnd;
	Vert3c40*  pPrev;
	Vert3c40*  pCur;
	uint32*    pFlag;
	int32      nInside;
	int32      i;
	int32      iPrev;

	++g_n1008be44;

	nInside = 0;
	pFlag   = anInside;
	pV      = &(*ppVerts)[0];
	pEnd    = pV + *pnVerts;

	for (; pV != pEnd; ++pV)
	{
		uint32 bIn = (pV->m_vPos.x < pV->m_vPos.z);

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
				float t = ClipEdgeRight(&pPrev->m_vPos, &pCur->m_vPos, &(*ppOut)->m_vPos);

				LerpVert3c40(pPrev, pCur, *ppOut, t);

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


// 0x10001cb0
static inline LTBOOL ClipPlaneBottom(uint32* pUnused, Vert3c40** ppVerts, int32* pnVerts, Vert3c40** ppOut)
{
	uint32     anInside[CLIP_MAX_VERTS];
	Vert3c40*  pOutStart;
	Vert3c40*  pV;
	Vert3c40*  pEnd;
	Vert3c40*  pPrev;
	Vert3c40*  pCur;
	uint32*    pFlag;
	int32      nInside;
	int32      i;
	int32      iPrev;

	++g_n1008be44;

	nInside = 0;
	pFlag   = anInside;
	pV      = &(*ppVerts)[0];
	pEnd    = pV + *pnVerts;

	for (; pV != pEnd; ++pV)
	{
		uint32 bIn = (-pV->m_vPos.z < pV->m_vPos.y);

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
				float t = ClipEdgeBottom(&pPrev->m_vPos, &pCur->m_vPos, &(*ppOut)->m_vPos);

				LerpVert3c40(pPrev, pCur, *ppOut, t);

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


// 0x10001fe0
static inline LTBOOL ClipPlaneFar(uint32* pUnused, Vert3c40** ppVerts, int32* pnVerts, Vert3c40** ppOut)
{
	uint32     anInside[CLIP_MAX_VERTS];
	Vert3c40*  pOutStart;
	Vert3c40*  pV;
	Vert3c40*  pEnd;
	Vert3c40*  pPrev;
	Vert3c40*  pCur;
	uint32*    pFlag;
	int32      nInside;
	int32      i;
	int32      iPrev;

	++g_n1008be44;

	nInside = 0;
	pFlag   = anInside;
	pV      = &(*ppVerts)[0];
	pEnd    = pV + *pnVerts;

	for (; pV != pEnd; ++pV)
	{
		uint32 bIn = (pV->m_vPos.z <= g_f1008b910);

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
				float t = ClipEdgeFar(&pPrev->m_vPos, &pCur->m_vPos, &(*ppOut)->m_vPos);

				LerpVert3c40(pPrev, pCur, *ppOut, t);

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


// 0x1000f4b0 -- d3dr3c_os.cpp's driver.
static inline LTBOOL ClipPoly(uint32 nClipFlags, Vert3c40** ppVerts, int32* pnVerts)
{
	Vert3c40*     pOut;
	Vert3c40*     pVerts;
	int32      nVerts;
	uint32     anUnused[6];

	if (g_n1008d7a0)
	{
		nClipFlags &= 1;
		if (!nClipFlags)
			return LTTRUE;
	}

	pOut   = g_p1008b87c;
	pVerts = *ppVerts;
	nVerts = *pnVerts;

	if (nClipFlags & 0x01)
	{
		if (!ClipPlaneNear(&anUnused[0], &pVerts, &nVerts, &pOut))
			return LTFALSE;
	}

	if (nClipFlags & 0x04)
	{
		if (!ClipPlaneLeft(&anUnused[1], &pVerts, &nVerts, &pOut))
			return LTFALSE;
	}

	if (nClipFlags & 0x08)
	{
		if (!ClipPlaneTop(&anUnused[2], &pVerts, &nVerts, &pOut))
			return LTFALSE;
	}

	if (nClipFlags & 0x10)
	{
		if (!ClipPlaneRight(&anUnused[3], &pVerts, &nVerts, &pOut))
			return LTFALSE;
	}

	if (nClipFlags & 0x20)
	{
		if (!ClipPlaneBottom(&anUnused[4], &pVerts, &nVerts, &pOut))
			return LTFALSE;
	}

	if (!(nClipFlags & 0x02) ||
	    ClipPlaneFar(&anUnused[5], &pVerts, &nVerts, &pOut))
	{
		*ppVerts = pVerts;
		*pnVerts = nVerts;
		return LTTRUE;
	}

	return LTFALSE;
}


struct C1003bb10
{
	void*	m_pVTable;                                  // +0x00  0x10089530
	uint8	m_pad_04[0x28 - 0x04];
	uint32	m_n28;                                      // +0x28

	int32	sub_1003bb10(Vert3c40* pVerts, int32 nVerts, uint32 nFlags);
};


int32 C1003bb10::sub_1003bb10(Vert3c40* pVerts, int32 nVerts, uint32 nFlags)
{
	float		fSaveNear;
	int32		i;
	Vert3c40*	pDraw = pVerts;
	int32		nDraw = nVerts;

	if (nDraw < 3)
		return LTTRUE;

	g_n1008b874 = 0x3f;

	if (m_n28 || (nFlags & 0x40))
	{
	fSaveNear   = g_f1008b908;
	g_f1008b908 = 0.01f;

	// LTMatrix::Apply(LTVector&) (ltmatrix.h) through g_Mat1008bb5c, in
	// place.
	for (i = 0; i < nDraw; i++)
		W18Apply(g_Mat1008bb5c, pDraw[i].m_vPos);

	// ------------------------------------------------------------------ //
	//  The clipper, inline.  With UseD3DClip set only the near plane is
	//  clipped here, and with no plane left to clip it goes straight to
	//  the projection.
	// ------------------------------------------------------------------ //
	if (!ClipPoly(g_n1008b874, &pDraw, &nDraw))
		return LTFALSE;

	// ------------------------------------------------------------------ //
	//  Project: x and y through g_Mat1008ba1c with the homogeneous divide;
	//  z at z + 7.0 with its own 1/w, which is also the vertex's RHW.
	// ------------------------------------------------------------------ //
	for (i = 0; i < nDraw; i++)
	{
		Vert3c40* pV = &pDraw[i];
		Vec3c40   vTemp;
		float     fOneOverW, fZ;

		fOneOverW = 1.0f / (g_Mat1008ba1c.m[3][0]*pV->m_vPos.x
		                  + g_Mat1008ba1c.m[3][1]*pV->m_vPos.y
		                  + g_Mat1008ba1c.m[3][2]*pV->m_vPos.z
		                  + g_Mat1008ba1c.m[3][3]);

		vTemp.x = fOneOverW * (g_Mat1008ba1c.m[0][0]*pV->m_vPos.x
		                     + g_Mat1008ba1c.m[0][1]*pV->m_vPos.y
		                     + g_Mat1008ba1c.m[0][2]*pV->m_vPos.z
		                     + g_Mat1008ba1c.m[0][3]);
		vTemp.y = fOneOverW * (g_Mat1008ba1c.m[1][0]*pV->m_vPos.x
		                     + g_Mat1008ba1c.m[1][1]*pV->m_vPos.y
		                     + g_Mat1008ba1c.m[1][2]*pV->m_vPos.z
		                     + g_Mat1008ba1c.m[1][3]);

		fZ = pV->m_vPos.z + 7.0f;

		fOneOverW = 1.0f / (g_Mat1008ba1c.m[3][0]*pV->m_vPos.x
		                  + g_Mat1008ba1c.m[3][1]*pV->m_vPos.y
		                  + g_Mat1008ba1c.m[3][2]*fZ
		                  + g_Mat1008ba1c.m[3][3]);

		pV->m_fRHW = fOneOverW;

		vTemp.z = fOneOverW * (g_Mat1008ba1c.m[2][0]*pV->m_vPos.x
		                     + g_Mat1008ba1c.m[2][1]*pV->m_vPos.y
		                     + g_Mat1008ba1c.m[2][2]*fZ
		                     + g_Mat1008ba1c.m[2][3]);

		pV->m_vPos = vTemp;
	}

	g_pD3DDevice->lpVtbl->DrawPrimitive(g_pD3DDevice,
		W18_D3DPT_TRIANGLEFAN, W18_D3DFVF_TLVERTEX, pDraw, nDraw, 0);

	g_f1008b908 = fSaveNear;
	}
	else
	{
		// The out-of-line clipper, and the polygon drawn as it comes back.
		if (sub_10023300(&pDraw, &nDraw))
		{
			g_pD3DDevice->lpVtbl->DrawPrimitive(g_pD3DDevice,
				W18_D3DPT_TRIANGLEFAN, W18_D3DFVF_TLVERTEX,
				pDraw, nDraw, 0);
		}
	}

	return LTFALSE;
}
