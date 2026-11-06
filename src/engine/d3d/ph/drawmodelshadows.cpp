// *********************************************************************** //
//  ph/drawmodelshadows.cpp -- Primal Hunt's d3d.ren, TU27 (0x10025e6c..0x1002938b), /O1.
//
//  Model shadows (ModelShadowAlpha, ModelShadowOffset, ModelShadowProj):
//  the console variables and the shadow polygons' clipper and projection
//  (0x10026fcd..0x100279fd).  Not yet written: the shadow draws
//  (0x10025ed1, 0x1002611f, 0x10026ddd, 0x10027b95, 0x10027e40) and the
//  arbitrary-plane clip 0x10026ea2 (its only users are those).
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


CD3DConVar	g_CV_ModelShadowAlpha("ModelShadowAlpha", 230.0f, LTNULL, LTNULL);	// 0x1006ead8  $E 0x10025e6c/0x10025e71
CD3DConVar	g_CV_ModelShadowOffset("ModelShadowOffset", 0.05f, LTNULL, LTNULL);	// 0x1006ef28  $E 0x10025e8f/0x10025e94
CD3DConVar	g_CV_ModelShadowProj("ModelShadowProj", 0.0f, LTNULL, LTNULL);	// 0x1006eaf8  $E 0x10025eb2/0x10025eb7


// ---------------------------------------------------------------------- //
//  0x10026ddd -- the matrix that projects from a point light onto a
//  plane (the planar shadow).
// ---------------------------------------------------------------------- //
class ShadowProjection
{
public:
	void		SetupPlanar(LTVector vLight, LTPlane plane);

	LTMatrix	m_mProj;                                // +0x00
};

void ShadowProjection::SetupPlanar(LTVector vLight, LTPlane plane)
{
	float fDot;

	fDot = vLight.x * plane.m_Normal.x + vLight.y * plane.m_Normal.y + vLight.z * plane.m_Normal.z - plane.m_Dist;

	m_mProj.m[0][0] = fDot - vLight.x * plane.m_Normal.x;
	m_mProj.m[0][1] = -(plane.m_Normal.y * vLight.x);
	m_mProj.m[0][2] = -(plane.m_Normal.z * vLight.x);
	m_mProj.m[0][3] = plane.m_Dist * vLight.x;

	m_mProj.m[1][0] = -(plane.m_Normal.x * vLight.y);
	m_mProj.m[1][1] = fDot - vLight.y * plane.m_Normal.y;
	m_mProj.m[1][2] = -(plane.m_Normal.z * vLight.y);
	m_mProj.m[1][3] = plane.m_Dist * vLight.y;

	m_mProj.m[2][0] = -(plane.m_Normal.x * vLight.z);
	m_mProj.m[2][1] = -(plane.m_Normal.y * vLight.z);
	m_mProj.m[2][2] = fDot - vLight.z * plane.m_Normal.z;
	m_mProj.m[2][3] = plane.m_Dist * vLight.z;

	m_mProj.m[3][0] = -plane.m_Normal.x;
	m_mProj.m[3][1] = -plane.m_Normal.y;
	m_mProj.m[3][2] = -plane.m_Normal.z;
	m_mProj.m[3][3] = plane.m_Dist + fDot;
}


// ---------------------------------------------------------------------- //
//  The shadow projector's polygon clipper: a projected-texture vertex
//  (u, v, q), clipped against an arbitrary plane or the view frustum.
// ---------------------------------------------------------------------- //
#define CLIP_EPSILON	0.00001f
#define CLIP_MAX_VERTS	50

union VColor
{
	uint32		m_n;
	uint8		m_b[4];
};

struct ShadowVert                                       // 0x24 bytes
{
	LTVector	m_vPos;                                 // +0x00
	float		m_fRHW;                                 // +0x0c
	VColor		m_Diffuse;                              // +0x10
	VColor		m_Specular;                             // +0x14
	float		m_fU;                                   // +0x18
	float		m_fV;                                   // +0x1c
	float		m_fQ;                                   // +0x20
};

struct TLVertex;

struct ViewParams
{
	uint8		m_pad_000[0x15c];
	LTMatrix	m_mWorldToCamera;                       // +0x15c
	uint8		m_pad_19c[0x1dc - 0x19c];
	LTMatrix	m_mFullTransform;                       // +0x1dc
	uint8		m_pad_21c[0x31c - 0x21c];
	float		m_fProjScaleX;                          // +0x31c
	float		m_fProjCenterX;                         // +0x320
	float		m_fProjScaleY;                          // +0x324
	float		m_fProjCenterY;                         // +0x328
	float		m_fProjScaleZ;                          // +0x32c
	float		m_fProjOffsetZ;                         // +0x330
};

extern uint32	g_ClipFlags;                            // 0x1005bd0c
extern uint32	g_nClipPasses;                          // 0x1005c124
extern int32	g_bUseD3DClip;                          // 0x100622b0
extern void*	g_pClipScratch;                         // 0x1005bd14
extern float	g_fNearZ;                               // 0x1005b818
extern float	g_fFarZ;                                // 0x1005b820

extern float	d3d_MatVMul_InPlace_H(LTMatrix* pMat, LTVector* pSrc);     // 0x1000868d
extern void		d3d_TransformToCamera(TLVertex* pVert, LTMatrix* pMat);    // 0x10008719

// Round to nearest in the FPU's mode (d3d_modelvb.cpp's RoundF).
inline int32 RoundF(float f)
{
	int32 n;
	__asm fld f
	__asm fistp n
	return n;
}

// 0x10026fcd
inline void LerpShadowVert(ShadowVert* a, ShadowVert* b, ShadowVert* dst, float t)
{
	dst->m_fU = (b->m_fU - a->m_fU) * t + a->m_fU;
	dst->m_fV = (b->m_fV - a->m_fV) * t + a->m_fV;
	dst->m_fQ = (b->m_fQ - a->m_fQ) * t + a->m_fQ;
	dst->m_Diffuse.m_b[2]  = (uint8)RoundF((b->m_Diffuse.m_b[2]  - a->m_Diffuse.m_b[2])  * t + a->m_Diffuse.m_b[2]);
	dst->m_Diffuse.m_b[1]  = (uint8)RoundF((b->m_Diffuse.m_b[1]  - a->m_Diffuse.m_b[1])  * t + a->m_Diffuse.m_b[1]);
	dst->m_Diffuse.m_b[0]  = (uint8)RoundF((b->m_Diffuse.m_b[0]  - a->m_Diffuse.m_b[0])  * t + a->m_Diffuse.m_b[0]);
	dst->m_Diffuse.m_b[3]  = (uint8)RoundF((b->m_Diffuse.m_b[3]  - a->m_Diffuse.m_b[3])  * t + a->m_Diffuse.m_b[3]);
	dst->m_Specular.m_b[3] = (uint8)RoundF((b->m_Specular.m_b[3] - a->m_Specular.m_b[3]) * t + a->m_Specular.m_b[3]);
}

// An arbitrary clip plane (0x100270d4, 0x10027117).
class ShadowClipPlane
{
public:
	LTBOOL Inside(const LTVector& vPt)
	{
		return m_pPlane->DistTo(vPt) > 0.0f;
	}

	float Intersect(const LTVector& vA, const LTVector& vB, LTVector& vOut)
	{
		float fDistA, fDenom, t;

		fDistA = m_pPlane->DistTo(vA);
		fDenom = fDistA - m_pPlane->DistTo(vB);

		if (fDenom < -CLIP_EPSILON || fDenom > CLIP_EPSILON)
			t = fDistA / fDenom;
		else
			t = 0.0f;

		vOut = vA + (vB - vA) * t;
		return t;
	}

	LTPlane*	m_pPlane;
};

// The view frustum's planes (d3d_modelvb.cpp's edge functions).
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

#define FRUSTUM_PLANE(NAME, INSIDE, EDGE) \
struct NAME \
{ \
	LTBOOL Inside(const LTVector& v) { return INSIDE; } \
	float Intersect(const LTVector& vA, const LTVector& vB, LTVector& vOut) \
		{ return EDGE((LTVector*)&vA, (LTVector*)&vB, &vOut); } \
};

FRUSTUM_PLANE(ClipPlaneNear,   (v.z >= g_fNearZ),  ClipEdgeNear)
FRUSTUM_PLANE(ClipPlaneLeft,   (-v.z < v.x),       ClipEdgeLeft)
FRUSTUM_PLANE(ClipPlaneTop,    (v.y < v.z),        ClipEdgeTop)
FRUSTUM_PLANE(ClipPlaneRight,  (v.x < v.z),        ClipEdgeRight)
FRUSTUM_PLANE(ClipPlaneBottom, (-v.z < v.y),       ClipEdgeBottom)
FRUSTUM_PLANE(ClipPlaneFar,    (v.z <= g_fFarZ),   ClipEdgeFar)

// Clip a polygon against one plane, into *ppOut (0x10026ea2, and the
// frustum's 0x100273f4..0x100279fd).
template<class V, class P>
LTBOOL d3d_ClipToPlane(P* pPlane, V** ppVerts, int32* pnVerts, V** ppOut)
{
	static uint32 anInside[CLIP_MAX_VERTS];
	V *pV, *pEnd, *pPrev, *pCur, *pOutStart;
	uint32* pFlag;
	int32 nInside, i, iPrev;
	float t;

	nInside = 0;
	++g_nClipPasses;

	pFlag = anInside;
	pV = *ppVerts;
	pEnd = pV + *pnVerts;
	for (; pV != pEnd; ++pV)
	{
		uint32 bIn = pPlane->Inside(pV->m_vPos);

		*pFlag = bIn;
		nInside += bIn;
		pFlag++;
	}

	if (nInside == 0)
		return LTFALSE;

	if (nInside != *pnVerts)
	{
		pOutStart = *ppOut;
		iPrev = *pnVerts - 1;
		pPrev = &(*ppVerts)[iPrev];
		for (i = 0; i < *pnVerts; i++)
		{
			pCur = &(*ppVerts)[i];

			if (anInside[iPrev])
			{
				**ppOut = *pPrev;
				++(*ppOut);
			}

			if (anInside[iPrev] != anInside[i])
			{
				t = pPlane->Intersect(pPrev->m_vPos, pCur->m_vPos, (*ppOut)->m_vPos);
				LerpShadowVert(pPrev, pCur, *ppOut, t);
				++(*ppOut);
			}

			iPrev = i;
			pPrev = pCur;
		}

		*pnVerts = *ppOut - pOutStart;
		*ppVerts = pOutStart;
		*ppOut += *pnVerts;
	}

	return LTTRUE;
}


// The arbitrary-plane instance (0x10026ea2), which the shadow draws
// (0x10025ed1, 0x10027b95; not yet written here) call.
template LTBOOL d3d_ClipToPlane<ShadowVert, ShadowClipPlane>(ShadowClipPlane*, ShadowVert**, int32*, ShadowVert**);


// ---------------------------------------------------------------------- //
//  0x100271e6 -- project a shadow polygon, clipping it first when it
//  crosses the frustum.
// ---------------------------------------------------------------------- //
LTBOOL d3d_ClipShadowPoly(uint32 nClipFlags, ShadowVert** ppVerts, int32* pnVerts);
void d3d_ProjectTLVert(TLVertex* pVert, ViewParams* pParams);

LTBOOL d3d_TransformShadowPoly(ShadowVert** ppVerts, int32* pnVerts, ViewParams* pParams)
{
	ShadowVert* pVert;
	int32 i;

	if (!g_ClipFlags)
	{
		pVert = *ppVerts;
		for (i = *pnVerts; i; i--)
		{
			pVert->m_fRHW = d3d_MatVMul_InPlace_H(&pParams->m_mFullTransform, &pVert->m_vPos);
			pVert++;
		}
	}
	else
	{
		pVert = *ppVerts;
		for (i = *pnVerts; i; i--)
		{
			d3d_TransformToCamera((TLVertex*)pVert, &pParams->m_mWorldToCamera);
			pVert++;
		}

		if (!d3d_ClipShadowPoly(g_ClipFlags, ppVerts, pnVerts))
			return LTFALSE;

		pVert = *ppVerts;
		for (i = *pnVerts; i; i--)
		{
			d3d_ProjectTLVert((TLVertex*)pVert, pParams);
			pVert++;
		}
	}

	return LTTRUE;
}


// ---------------------------------------------------------------------- //
//  0x10027281 -- clip against the planes in nClipFlags, into the clip
//  scratch buffer. FALSE when nothing is left.
// ---------------------------------------------------------------------- //
LTBOOL d3d_ClipShadowPoly(uint32 nClipFlags, ShadowVert** ppVerts, int32* pnVerts)
{
	ShadowVert* pOut;
	ShadowVert* pVerts;
	int32 nVerts;
	ClipPlaneNear planeNear;
	ClipPlaneLeft planeLeft;
	ClipPlaneTop planeTop;
	ClipPlaneRight planeRight;
	ClipPlaneBottom planeBottom;
	ClipPlaneFar planeFar;

	if (g_bUseD3DClip)
	{
		nClipFlags &= 1;
		if (!nClipFlags)
			return LTTRUE;
	}

	pOut = (ShadowVert*)g_pClipScratch;
	pVerts = *ppVerts;
	nVerts = *pnVerts;

	if (nClipFlags & 0x01)
	{
		if (!d3d_ClipToPlane(&planeNear, &pVerts, &nVerts, &pOut))
			return LTFALSE;
	}
	if (nClipFlags & 0x04)
	{
		if (!d3d_ClipToPlane(&planeLeft, &pVerts, &nVerts, &pOut))
			return LTFALSE;
	}
	if (nClipFlags & 0x08)
	{
		if (!d3d_ClipToPlane(&planeTop, &pVerts, &nVerts, &pOut))
			return LTFALSE;
	}
	if (nClipFlags & 0x10)
	{
		if (!d3d_ClipToPlane(&planeRight, &pVerts, &nVerts, &pOut))
			return LTFALSE;
	}
	if (nClipFlags & 0x20)
	{
		if (!d3d_ClipToPlane(&planeBottom, &pVerts, &nVerts, &pOut))
			return LTFALSE;
	}
	if (!(nClipFlags & 0x02) || d3d_ClipToPlane(&planeFar, &pVerts, &nVerts, &pOut))
	{
		*ppVerts = pVerts;
		*pnVerts = nVerts;
		return LTTRUE;
	}

	return LTFALSE;
}


// ---------------------------------------------------------------------- //
//  0x1002739d -- a camera-space vertex to the screen.
// ---------------------------------------------------------------------- //
struct TLVertex
{
	LTVector	m_Vec;                                  // +0x00
	float		rhw;                                    // +0x0c
};

void d3d_ProjectTLVert(TLVertex* pVert, ViewParams* pParams)
{
	float fRHW;

	pVert->rhw = fRHW = 1.0f / pVert->m_Vec.z;
	pVert->m_Vec.x = (pParams->m_fProjScaleX * pVert->m_Vec.x + pParams->m_fProjCenterX * pVert->m_Vec.z) * fRHW;
	pVert->m_Vec.y = (pParams->m_fProjCenterY * pVert->m_Vec.z + pParams->m_fProjScaleY * pVert->m_Vec.y) * fRHW;
	pVert->m_Vec.z = (pParams->m_fProjScaleZ * pVert->m_Vec.z + pParams->m_fProjOffsetZ) * fRHW;
}


// ---------------------------------------------------------------------- //
//  The rest of the object's console variables, defined further down the
//  file (their initialisers, from 0x10027b34, follow the code above).
// ---------------------------------------------------------------------- //
CD3DConVar	g_CV_ModelShadowProjRes("ModelShadowProjRes", 128.0f, LTNULL, LTNULL);	// 0x1006ff88  $E 0x10027b34/0x10027b39
CD3DConVar	g_CV_ModelShadowProjLOD("ModelShadowProjLOD", 1.0f, LTNULL, LTNULL);	// 0x1006ff68  $E 0x10027b57/0x10027b5c
CD3DConVar	g_CV_ModelShadowProjShow("ModelShadowProjShow", 0.0f, LTNULL, LTNULL);	// 0x1006ff48  $E 0x10027b76/0x10027b7b
