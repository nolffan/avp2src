// *********************************************************************** //
//
//  ph/d3d_clip.h -- Primal Hunt's d3d.ren: the homogeneous polygon
//  clipper, shared by the model drawer (TU01, d3d_modelvb.cpp) and the
//  world drawer (TU04, drawmodel_clip.cpp).
//
//  Templates and inline functions over the clip vertex, so each object
//  that uses them carries its own copies: TU01 (/O2 /Ob2) expands most of
//  them into its two drivers, TU04 (/O1) calls them out of line, and the
//  linker keeps the first copy of each. TU04's are the ones the image has
//  of what TU01 expands (ClipPoly<ClipVert2> 0x10008779, ClipNear and
//  ClipLeft 0x10008895 and 0x100089cc, ClipEdgeTop..ClipEdgeFar
//  0x10008b01..0x10008c72).
//
// *********************************************************************** //

#ifndef __PH_D3D_CLIP_H__
#define __PH_D3D_CLIP_H__


#define CLIP_EPSILON    0.00001f
#define CLIP_MAX_VERTS  50

// Round to nearest in the FPU's mode. Every colour byte the clippers
// interpolate is `fstp t ; fld t ; fistp n`, a float parameter stored and
// reloaded for an x87 store: the original's assembly helper, which no
// C spelling under cl 8966 gives (a cast calls _ftol).
inline int32 RoundF(float f)
{
	int32 n;
	__asm fld f
	__asm fistp n
	return n;
}

union VColor
{
	uint32      m_n;
	uint8       m_b[4];
};

// The 32-byte bank vertex (FVF 0x1c4) as the clippers see it.
struct ClipVert
{
	LTVector    m_vPos;                                     // +0x00
	float       m_fRHW;                                     // +0x0c
	VColor      m_Diffuse;                                  // +0x10
	VColor      m_Specular;                                 // +0x14
	float       m_fU;                                       // +0x18
	float       m_fV;                                       // +0x1c
};

// The 40-byte bank vertex (FVF 0x2c4): two texture coordinate pairs.
struct ClipVert2
{
	LTVector    m_vPos;                                     // +0x00
	float       m_fRHW;                                     // +0x0c
	VColor      m_Diffuse;                                  // +0x10
	VColor      m_Specular;                                 // +0x14
	float       m_fU;                                       // +0x18
	float       m_fV;                                       // +0x1c
	float       m_fU2;                                      // +0x20
	float       m_fV2;                                      // +0x24
};

extern uint32   g_nClipPasses;                              // 0x1005c124
extern int32    g_bUseD3DClip;                              // 0x100622b0
extern void*    g_pClipScratch;                             // 0x1005bd14
extern float    g_fNearZ;                                   // 0x1005b818
extern float    g_fFarZ;                                    // 0x1005b820

inline void LerpClipVert(ClipVert* a, ClipVert* b, ClipVert* dst, float t)
{
	dst->m_fU = (b->m_fU - a->m_fU) * t + a->m_fU;
	dst->m_fV = (b->m_fV - a->m_fV) * t + a->m_fV;
	dst->m_Diffuse.m_b[2]  = (uint8)RoundF((b->m_Diffuse.m_b[2]  - a->m_Diffuse.m_b[2])  * t + a->m_Diffuse.m_b[2]);
	dst->m_Diffuse.m_b[1]  = (uint8)RoundF((b->m_Diffuse.m_b[1]  - a->m_Diffuse.m_b[1])  * t + a->m_Diffuse.m_b[1]);
	dst->m_Diffuse.m_b[0]  = (uint8)RoundF((b->m_Diffuse.m_b[0]  - a->m_Diffuse.m_b[0])  * t + a->m_Diffuse.m_b[0]);
	dst->m_Diffuse.m_b[3]  = (uint8)RoundF((b->m_Diffuse.m_b[3]  - a->m_Diffuse.m_b[3])  * t + a->m_Diffuse.m_b[3]);
	dst->m_Specular.m_b[3] = (uint8)RoundF((b->m_Specular.m_b[3] - a->m_Specular.m_b[3]) * t + a->m_Specular.m_b[3]);
}

inline void LerpClipVert(ClipVert2* a, ClipVert2* b, ClipVert2* dst, float t)
{
	dst->m_fU = (b->m_fU - a->m_fU) * t + a->m_fU;
	dst->m_fV = (b->m_fV - a->m_fV) * t + a->m_fV;
	dst->m_fU2 = (b->m_fU2 - a->m_fU2) * t + a->m_fU2;
	dst->m_fV2 = (b->m_fV2 - a->m_fV2) * t + a->m_fV2;
	dst->m_Diffuse.m_b[2]  = (uint8)RoundF((b->m_Diffuse.m_b[2]  - a->m_Diffuse.m_b[2])  * t + a->m_Diffuse.m_b[2]);
	dst->m_Diffuse.m_b[1]  = (uint8)RoundF((b->m_Diffuse.m_b[1]  - a->m_Diffuse.m_b[1])  * t + a->m_Diffuse.m_b[1]);
	dst->m_Diffuse.m_b[0]  = (uint8)RoundF((b->m_Diffuse.m_b[0]  - a->m_Diffuse.m_b[0])  * t + a->m_Diffuse.m_b[0]);
	dst->m_Diffuse.m_b[3]  = (uint8)RoundF((b->m_Diffuse.m_b[3]  - a->m_Diffuse.m_b[3])  * t + a->m_Diffuse.m_b[3]);
	dst->m_Specular.m_b[3] = (uint8)RoundF((b->m_Specular.m_b[3] - a->m_Specular.m_b[3]) * t + a->m_Specular.m_b[3]);
}

inline float ClipEdgeNear(LTVector* a, LTVector* b, LTVector* dst)
{
	float fDenom = b->z - a->z;
	float t;

	if (fDenom < -CLIP_EPSILON || fDenom > CLIP_EPSILON)
		t = (g_fNearZ - a->z) / fDenom;
	else
		t = 0.0f;

	dst->x = (b->x - a->x) * t + a->x;
	dst->y = (b->y - a->y) * t + a->y;
	dst->z = g_fNearZ;

	return t;
}

inline float ClipEdgeLeft(LTVector* a, LTVector* b, LTVector* dst)
{
	float fDenom = b->x - a->x + b->z - a->z;
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

inline float ClipEdgeTop(LTVector* a, LTVector* b, LTVector* dst)
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

inline float ClipEdgeRight(LTVector* a, LTVector* b, LTVector* dst)
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

inline float ClipEdgeBottom(LTVector* a, LTVector* b, LTVector* dst)
{
	float fDenom = b->y - a->y + b->z - a->z;
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

inline float ClipEdgeFar(LTVector* a, LTVector* b, LTVector* dst)
{
	float fDenom = b->z - a->z;
	float t;

	if (fDenom < -CLIP_EPSILON || fDenom > CLIP_EPSILON)
		t = (g_fFarZ - a->z) / fDenom;
	else
		t = 0.0f;

	dst->x = (b->x - a->x) * t + a->x;
	dst->y = (b->y - a->y) * t + a->y;
	dst->z = g_fFarZ;

	return t;
}


#define CLIP_PLANE(NAME, INSIDE, EDGE) \
template<class V> \
LTBOOL NAME(uint8* pUnused, V** ppVerts, int32* pnVerts, V** ppOut) \
{ \
	static uint32 anInside[CLIP_MAX_VERTS]; \
	V *pV, *pEnd, *pPrev, *pCur, *pOutStart; \
	uint32* pFlag; \
	int32 nInside, i, iPrev; \
	float t; \
 \
	nInside = 0; \
	++g_nClipPasses; \
 \
	pV = *ppVerts; \
	pEnd = pV + *pnVerts; \
	pFlag = anInside; \
	for (; pV != pEnd; ++pV) \
	{ \
		uint32 bIn = INSIDE; \
 \
		*pFlag = bIn; \
		nInside += bIn; \
		pFlag++; \
	} \
 \
	if (nInside == 0) \
		return LTFALSE; \
 \
	if (nInside != *pnVerts) \
	{ \
		pOutStart = *ppOut; \
		iPrev = *pnVerts - 1; \
		pPrev = &(*ppVerts)[iPrev]; \
		for (i = 0; i < *pnVerts; i++) \
		{ \
			pCur = &(*ppVerts)[i]; \
 \
			if (anInside[iPrev]) \
			{ \
				**ppOut = *pPrev; \
				++(*ppOut); \
			} \
 \
			if (anInside[iPrev] != anInside[i]) \
			{ \
				t = EDGE(&pPrev->m_vPos, &pCur->m_vPos, &(*ppOut)->m_vPos); \
				LerpClipVert(pPrev, pCur, *ppOut, t); \
				++(*ppOut); \
			} \
 \
			iPrev = i; \
			pPrev = pCur; \
		} \
 \
		*pnVerts = *ppOut - pOutStart; \
		*ppVerts = pOutStart; \
		*ppOut += *pnVerts; \
	} \
 \
	return LTTRUE; \
}

#define CLIP_PLANE_V(NAME, V, INSIDE, EDGE) \
inline LTBOOL NAME(uint8* pUnused, V** ppVerts, int32* pnVerts, V** ppOut) \
{ \
	static uint32 anInside[CLIP_MAX_VERTS]; \
	V *pV, *pEnd, *pPrev, *pCur, *pOutStart; \
	uint32* pFlag; \
	int32 nInside, i, iPrev; \
	float t; \
 \
	nInside = 0; \
	++g_nClipPasses; \
 \
	pV = *ppVerts; \
	pEnd = pV + *pnVerts; \
	pFlag = anInside; \
	for (; pV != pEnd; ++pV) \
	{ \
		uint32 bIn = INSIDE; \
 \
		*pFlag = bIn; \
		nInside += bIn; \
		pFlag++; \
	} \
 \
	if (nInside == 0) \
		return LTFALSE; \
 \
	if (nInside != *pnVerts) \
	{ \
		pOutStart = *ppOut; \
		iPrev = *pnVerts - 1; \
		pPrev = &(*ppVerts)[iPrev]; \
		for (i = 0; i < *pnVerts; i++) \
		{ \
			pCur = &(*ppVerts)[i]; \
 \
			if (anInside[iPrev]) \
			{ \
				**ppOut = *pPrev; \
				++(*ppOut); \
			} \
 \
			if (anInside[iPrev] != anInside[i]) \
			{ \
				t = EDGE(&pPrev->m_vPos, &pCur->m_vPos, &(*ppOut)->m_vPos); \
				LerpClipVert(pPrev, pCur, *ppOut, t); \
				++(*ppOut); \
			} \
 \
			iPrev = i; \
			pPrev = pCur; \
		} \
 \
		*pnVerts = *ppOut - pOutStart; \
		*ppVerts = pOutStart; \
		*ppOut += *pnVerts; \
	} \
 \
	return LTTRUE; \
}

CLIP_PLANE_V(ClipNear, ClipVert, (pV->m_vPos.z >= g_fNearZ), ClipEdgeNear)
CLIP_PLANE_V(ClipLeft, ClipVert, (-pV->m_vPos.z < pV->m_vPos.x), ClipEdgeLeft)
CLIP_PLANE_V(ClipNear, ClipVert2, (pV->m_vPos.z >= g_fNearZ), ClipEdgeNear)
CLIP_PLANE_V(ClipLeft, ClipVert2, (-pV->m_vPos.z < pV->m_vPos.x), ClipEdgeLeft)
CLIP_PLANE(ClipTop,    (pV->m_vPos.y < pV->m_vPos.z),  ClipEdgeTop)
CLIP_PLANE(ClipRight,  (pV->m_vPos.x < pV->m_vPos.z),  ClipEdgeRight)
CLIP_PLANE(ClipBottom, (-pV->m_vPos.z < pV->m_vPos.y), ClipEdgeBottom)
CLIP_PLANE(ClipFar,    (pV->m_vPos.z <= g_fFarZ),      ClipEdgeFar)



// Clip a polygon against the planes in nClipFlags, into the clip scratch
// buffer. FALSE when nothing is left.
template<class V>
inline LTBOOL ClipPoly(uint32 nClipFlags, V** ppVerts, int32* pnVerts)
{
	V* pOut;
	V* pVerts;
	int32 nVerts;
	uint8 cNear, cLeft, cTop, cRight, cBottom, cFar;

	if (g_bUseD3DClip)
	{
		nClipFlags &= 1;
		if (!nClipFlags)
			return LTTRUE;
	}

	pOut = (V*)g_pClipScratch;
	pVerts = *ppVerts;
	nVerts = *pnVerts;

	if (nClipFlags & 0x01)
	{
		if (!ClipNear(&cNear, &pVerts, &nVerts, &pOut))
			return LTFALSE;
	}
	if (nClipFlags & 0x04)
	{
		if (!ClipLeft(&cLeft, &pVerts, &nVerts, &pOut))
			return LTFALSE;
	}
	if (nClipFlags & 0x08)
	{
		if (!ClipTop(&cTop, &pVerts, &nVerts, &pOut))
			return LTFALSE;
	}
	if (nClipFlags & 0x10)
	{
		if (!ClipRight(&cRight, &pVerts, &nVerts, &pOut))
			return LTFALSE;
	}
	if (nClipFlags & 0x20)
	{
		if (!ClipBottom(&cBottom, &pVerts, &nVerts, &pOut))
			return LTFALSE;
	}
	if (!(nClipFlags & 0x02) ||
	    ClipFar(&cFar, &pVerts, &nVerts, &pOut))
	{
		*ppVerts = pVerts;
		*pnVerts = nVerts;
		return LTTRUE;
	}
	return LTFALSE;
}


#endif  // __PH_D3D_CLIP_H__
