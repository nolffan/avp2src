// *********************************************************************** //
//  ph/drawmodelshadows.cpp -- Primal Hunt's d3d.ren, TU27 (0x10025e6c..0x1002938b), /O1.
//
//  Model shadows (ModelShadowAlpha, ModelShadowOffset, ModelShadowProj):
//  the console variables, CD3DModelDraw's two per-polygon shadow draws
//  (0x10025ed1, 0x10027b95), the planar projection matrix (0x10026ddd) and
//  the shadow polygons' clipper and projection (0x10026ea2..0x100279fd).
//
//  Not yet written: CD3DModelDraw's two shadow passes, 0x1002611f (3,262
//  bytes, the planar pass: AvP2 1.0.9.6's 0x10048180, w54_10048180_
//  osoiqipsf.cpp) and 0x10027e40 (5,451 bytes, the projected pass it
//  forks to when ModelShadowProj is set: AvP2's 0x1004b720, big4b720.cpp).
//  Both call the _CVector constructor, Cross, Norm and Mag out of line
//  (0x1000df6e, 0x1000e65d, 0x1000e62a, 0x1000dfc0).
//
//  Still near:
//    * 0x10026ddd SetupPlanar: six x87 operands of three products come out
//      the other way round (VC6 orders a commutative pair by its own
//      numbering; the source's operand order, term order, parameter names
//      and the plane's type do not move it).
//    * 0x100273f4, the near plane's clip: the two parameter reloads after
//      the vertex copy (edi/esi) are swapped.  The other five instances are
//      identical; declaring d3d_clip.h's edge functions (rather than
//      defining them here) is what closed the far plane's.
//    * 0x10025ed1 and 0x10027b95: the u (and v) texture coordinates' three
//      products come out in another order -- the same numbering effect as
//      SetupPlanar; everything else in both is byte-identical.
// *********************************************************************** //

#include <string.h>

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

struct TLVertex
{
	LTVector	m_Vec;                                  // +0x00
	float		rhw;                                    // +0x0c
};

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

// The view frustum's planes: d3d_clip.h's edge functions, which TU01
// (d3d_modelvb.cpp) and TU04 (drawmodel_clip.cpp) define.
float ClipEdgeNear(LTVector* a, LTVector* b, LTVector* dst);
float ClipEdgeLeft(LTVector* a, LTVector* b, LTVector* dst);
float ClipEdgeTop(LTVector* a, LTVector* b, LTVector* dst);
float ClipEdgeRight(LTVector* a, LTVector* b, LTVector* dst);
float ClipEdgeBottom(LTVector* a, LTVector* b, LTVector* dst);
float ClipEdgeFar(LTVector* a, LTVector* b, LTVector* dst);

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


// ---------------------------------------------------------------------- //
//  The engine's structures, as the shadow draws read them.
// ---------------------------------------------------------------------- //

// IDirect3DDevice7, the slots used here (drawmodel_env.cpp's view).
struct IDirect3DDevice7
{
public:
	virtual long __stdcall QueryInterface(void*, void**) = 0;
	virtual uint32  __stdcall AddRef() = 0;
	virtual uint32  __stdcall Release() = 0;
	virtual long __stdcall GetCaps(void*) = 0;
	virtual long __stdcall EnumTextureFormats(void*, void*) = 0;
	virtual long __stdcall BeginScene() = 0;
	virtual long __stdcall EndScene() = 0;
	virtual long __stdcall GetDirect3D(void**) = 0;
	virtual long __stdcall SetRenderTarget(void*, uint32) = 0;
	virtual long __stdcall GetRenderTarget(void**) = 0;
	virtual long __stdcall Clear(uint32, void*, uint32, uint32, float, uint32) = 0;
	virtual long __stdcall SetTransform(uint32, void*) = 0;
	virtual long __stdcall GetTransform(uint32, void*) = 0;
	virtual long __stdcall SetViewport(void*) = 0;
	virtual long __stdcall MultiplyTransform(uint32, void*) = 0;
	virtual long __stdcall GetViewport(void*) = 0;
	virtual long __stdcall SetMaterial(void*) = 0;
	virtual long __stdcall GetMaterial(void*) = 0;
	virtual long __stdcall SetLight(uint32, void*) = 0;
	virtual long __stdcall GetLight(uint32, void*) = 0;
	virtual long __stdcall SetRenderState(uint32 state, uint32 value) = 0;
	virtual long __stdcall GetRenderState(uint32 state, uint32* pValue) = 0;
	virtual long __stdcall BeginStateBlock() = 0;
	virtual long __stdcall EndStateBlock(uint32*) = 0;
	virtual long __stdcall PreLoad(void*) = 0;
	virtual long __stdcall DrawPrimitive(uint32, uint32, void*, uint32, uint32) = 0;
	virtual long __stdcall DrawIndexedPrimitive(uint32, uint32, void*, uint32, uint16*, uint32, uint32) = 0;
	virtual long __stdcall SetClipStatus(void*) = 0;
	virtual long __stdcall GetClipStatus(void*) = 0;
	virtual long __stdcall DrawPrimitiveStrided(uint32, uint32, void*, uint32, uint32) = 0;
	virtual long __stdcall DrawIndexedPrimitiveStrided(uint32, uint32, void*, uint32, uint16*, uint32, uint32) = 0;
	virtual long __stdcall DrawPrimitiveVB(void*, uint32, uint32, uint32) = 0;
	virtual long __stdcall DrawIndexedPrimitiveVB(void*, uint32, uint32, uint16*, uint32, uint32) = 0;
	virtual long __stdcall ComputeSphereVisibility(void*, float*, uint32, uint32, uint32*) = 0;
	virtual long __stdcall GetTexture(uint32, void**) = 0;
	virtual long __stdcall SetTexture(uint32, void*) = 0;
	virtual long __stdcall GetTextureStageState(uint32 stage, uint32 type, uint32* pValue) = 0;
	virtual long __stdcall SetTextureStageState(uint32 stage, uint32 type, uint32 value) = 0;
};

#define D3DPT_TRIANGLEFAN		6
#define D3DFVF_TLVERTEX			0x1c4

// A world vertex, and a polygon's reference to one.
struct WorldVert
{
	LTVector	m_Vec;                                  // +0x00
};

struct PolyVert                                         // 0x18 bytes
{
	WorldVert*	m_pVert;                                // +0x00
	uint8		m_pad_04[0x18 - 0x04];
};

struct WorldPoly
{
	uint8		m_pad_00[0x28];
	LTPlane*	m_pPlane;                               // +0x28
	uint8		m_pad_2c[0x50 - 0x2c];
	PolyVert*	m_pFixedVerts;                          // +0x50
	uint16		m_nVerts;                               // +0x54
	uint16		m_nFixedVerts;                          // +0x56
	PolyVert	m_Verts[1];                             // +0x58
};

// The shadow being cast: its frustum and its texture projection.
struct ModelShadowInfo
{
	LTVector	m_vOrigin;                              // +0x00 the projection's origin
	uint8		m_pad_0c[0x60 - 0x0c];
	LTMatrix	m_mTex;                                 // +0x60 rows u, v, -, q
	LTPlane		m_Planes[6];                            // +0xa0
};

// The vertex the device draws (FVF 0x1c4).
struct ShadowTLVert                                     // 0x20 bytes
{
	LTVector	m_vPos;                                 // +0x00
	float		m_fRHW;                                 // +0x0c
	VColor		m_Diffuse;                              // +0x10
	VColor		m_Specular;                             // +0x14
	float		m_fU;                                   // +0x18
	float		m_fV;                                   // +0x1c
};

// The model drawer (TU02's CD3DModelDraw), the members the shadows use.
class CD3DModelDraw
{
public:
	void		DrawShadowPoly(ModelShadowInfo* pInfo, WorldPoly* pPoly);  // 0x10025ed1
	void		DrawProjShadowPoly(ModelShadowInfo* pInfo, WorldPoly* pPoly, float fMaxDist);  // 0x10027b95
};

struct ViewParams;

extern IDirect3DDevice7*	g_pD3DDevice;               // 0x100638c8
extern ViewParams			g_ViewParams;             // 0x1005b790
extern int32				g_CV_FixTJunc;              // 0x1005dbb4

extern void		dd_ConsolePrint(const char* pMsg, ...); // 0x10013ade

LTBOOL d3d_TransformShadowPoly(ShadowVert** ppVerts, int32* pnVerts, ViewParams* pParams, uint32 flags);

#define MAX_SHADOWPOLY_VERTS	128

// The polygon's vertices: the T-junction-fixed set, or its own.
inline int32 d3d_GetPolyVerts(WorldPoly* pPoly, PolyVert** ppVerts)
{
	int32 nVerts;

	if (g_CV_FixTJunc)
	{
		*ppVerts = pPoly->m_pFixedVerts;
		nVerts = pPoly->m_nFixedVerts;
	}
	else
	{
		nVerts = pPoly->m_nVerts;
		*ppVerts = pPoly->m_Verts;
	}

	return nVerts;
}

// Room for the polygon's vertices?
inline LTBOOL d3d_CheckVertexCount(int32 nVerts)
{
	if (nVerts > MAX_SHADOWPOLY_VERTS)
	{
		dd_ConsolePrint("Error: vertex buffer overflow");
		return LTFALSE;
	}

	return LTTRUE;
}

// Clip a polygon against a run of planes, through the clip scratch buffer.
inline LTBOOL d3d_ClipToPlanes(ShadowVert** ppVerts, int32* pnVerts, LTPlane* pPlanes, int32 nPlanes)
{
	ShadowClipPlane plane;
	ShadowVert *pVerts, *pOut;
	int32 nVerts;

	pOut = (ShadowVert*)g_pClipScratch;
	pVerts = *ppVerts;
	nVerts = *pnVerts;

	while (nPlanes)
	{
		nPlanes--;
		plane.m_pPlane = pPlanes;
		if (!d3d_ClipToPlane(&plane, &pVerts, &nVerts, &pOut))
			return LTFALSE;

		pPlanes++;
	}

	*ppVerts = pVerts;
	*pnVerts = nVerts;
	return LTTRUE;
}



CD3DConVar	g_CV_ModelShadowAlpha("ModelShadowAlpha", 230.0f, LTNULL, LTNULL);	// 0x1006ead8  $E 0x10025e6c/0x10025e71
CD3DConVar	g_CV_ModelShadowOffset("ModelShadowOffset", 0.05f, LTNULL, LTNULL);	// 0x1006ef28  $E 0x10025e8f/0x10025e94
CD3DConVar	g_CV_ModelShadowProj("ModelShadowProj", 0.0f, LTNULL, LTNULL);	// 0x1006eaf8  $E 0x10025eb2/0x10025eb7


// ---------------------------------------------------------------------- //
//  0x10025ed1 -- draw the shadow onto one world polygon: lift it off the
//  polygon, clip it to the shadow's frustum, project the shadow texture
//  onto it and draw it as a fan.
// ---------------------------------------------------------------------- //
void CD3DModelDraw::DrawShadowPoly(ModelShadowInfo* pInfo, WorldPoly* pPoly)
{
	ShadowVert verts[MAX_SHADOWPOLY_VERTS];
	ShadowTLVert tlVerts[MAX_SHADOWPOLY_VERTS];
	int32 nVerts, i;
	PolyVert* pSrc;
	ShadowVert *pVerts, *pVert;

	nVerts = d3d_GetPolyVerts(pPoly, &pSrc);
	if (!d3d_CheckVertexCount(nVerts))
		return;

	for (i=0; i < nVerts; i++)
	{
		verts[i].m_vPos = pSrc[i].m_pVert->m_Vec;
		verts[i].m_vPos += pPoly->m_pPlane->m_Normal * g_CV_ModelShadowOffset.m_fValue;
	}

	// Clip it to the shadow's frustum.
	pVerts = verts;
	if (!d3d_ClipToPlanes(&pVerts, &nVerts, pInfo->m_Planes, 6))
		return;

	if (pVerts != verts)
	{
		memcpy(verts, pVerts, nVerts * sizeof(ShadowVert));
		pVerts = verts;
	}

	for (i=0; i < nVerts; i++)
	{
		pVert = &pVerts[i];

		pVert->m_Diffuse.m_n = 0;
		pVert->m_Diffuse.m_b[3] = (uint8)g_CV_ModelShadowAlpha.m_nValue;

		pVert->m_fU = pInfo->m_mTex.m[0][0] * pVert->m_vPos.x + pInfo->m_mTex.m[0][1] * pVert->m_vPos.y +
			pInfo->m_mTex.m[0][2] * pVert->m_vPos.z + pInfo->m_mTex.m[0][3];
		pVert->m_fV = pInfo->m_mTex.m[1][0] * pVert->m_vPos.x + pInfo->m_mTex.m[1][1] * pVert->m_vPos.y +
			pInfo->m_mTex.m[1][2] * pVert->m_vPos.z + pInfo->m_mTex.m[1][3];
		pVert->m_fQ = pInfo->m_mTex.m[3][0] * pVert->m_vPos.x + pInfo->m_mTex.m[3][1] * pVert->m_vPos.y +
			pInfo->m_mTex.m[3][2] * pVert->m_vPos.z + pInfo->m_mTex.m[3][3];

		pVert->m_Specular.m_n = 0xFFFFFFFF;
	}

	g_ClipFlags = 0x3f;
	if (!d3d_TransformShadowPoly(&pVerts, &nVerts, (ViewParams*)&g_ViewParams, 0))
		return;

	for (i=0; i < nVerts; i++)
	{
		tlVerts[i].m_vPos = pVerts[i].m_vPos;
		tlVerts[i].m_fRHW = pVerts[i].m_fRHW;
		tlVerts[i].m_Diffuse = pVerts[i].m_Diffuse;
		tlVerts[i].m_Specular = pVerts[i].m_Specular;
		tlVerts[i].m_fU = pVerts[i].m_fU / pVerts[i].m_fQ;
		tlVerts[i].m_fV = pVerts[i].m_fV / pVerts[i].m_fQ;
	}

	g_pD3DDevice->DrawPrimitive(D3DPT_TRIANGLEFAN, D3DFVF_TLVERTEX, tlVerts, nVerts, 0);
}

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

	fDot = vLight.x * plane.m_Normal.x + (vLight.y * plane.m_Normal.y + vLight.z * plane.m_Normal.z) - plane.m_Dist;

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


// The arbitrary-plane instance (0x10026ea2), which the shadow draws
// (0x10025ed1, 0x10027b95; not yet written here) call.
template LTBOOL d3d_ClipToPlane<ShadowVert, ShadowClipPlane>(ShadowClipPlane*, ShadowVert**, int32*, ShadowVert**);


// ---------------------------------------------------------------------- //
//  0x100271e6 -- project a shadow polygon, clipping it first when it
//  crosses the frustum.
// ---------------------------------------------------------------------- //
LTBOOL d3d_ClipShadowPoly(uint32 nClipFlags, ShadowVert** ppVerts, int32* pnVerts);
void d3d_ProjectTLVert(TLVertex* pVert, ViewParams* pParams);

LTBOOL d3d_TransformShadowPoly(ShadowVert** ppVerts, int32* pnVerts, ViewParams* pParams, uint32 flags)
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
void d3d_ProjectTLVert(TLVertex* pVert, ViewParams* pParams)
{
	pVert->rhw = 1.0f / pVert->m_Vec.z;
	pVert->m_Vec.x = (pVert->m_Vec.x * pParams->m_fProjScaleX + pVert->m_Vec.z * pParams->m_fProjCenterX) * pVert->rhw;
	pVert->m_Vec.y = (pVert->m_Vec.y * pParams->m_fProjScaleY + pVert->m_Vec.z * pParams->m_fProjCenterY) * pVert->rhw;
	pVert->m_Vec.z = (pVert->m_Vec.z * pParams->m_fProjScaleZ + pParams->m_fProjOffsetZ) * pVert->rhw;
}


// ---------------------------------------------------------------------- //
//  The rest of the object's console variables, defined further down the
//  file (their initialisers, from 0x10027b34, follow the code above).
// ---------------------------------------------------------------------- //
CD3DConVar	g_CV_ModelShadowProjRes("ModelShadowProjRes", 128.0f, LTNULL, LTNULL);	// 0x1006ff88  $E 0x10027b34/0x10027b39
CD3DConVar	g_CV_ModelShadowProjLOD("ModelShadowProjLOD", 1.0f, LTNULL, LTNULL);	// 0x1006ff68  $E 0x10027b57/0x10027b5c
CD3DConVar	g_CV_ModelShadowProjShow("ModelShadowProjShow", 0.0f, LTNULL, LTNULL);	// 0x1006ff48  $E 0x10027b76/0x10027b7b


// ---------------------------------------------------------------------- //
//  0x10027b95 -- the projected shadow onto one world polygon: as
//  DrawShadowPoly, but the shadow fades out with the distance from the
//  projection's origin.
// ---------------------------------------------------------------------- //
void CD3DModelDraw::DrawProjShadowPoly(ModelShadowInfo* pInfo, WorldPoly* pPoly, float fMaxDist)
{
	ShadowVert verts[MAX_SHADOWPOLY_VERTS];
	ShadowTLVert tlVerts[MAX_SHADOWPOLY_VERTS];
	int32 nVerts, i;
	PolyVert* pSrc;
	ShadowVert *pVerts, *pVert;
	float fAlpha;
	LTMatrix* pMat;

	nVerts = d3d_GetPolyVerts(pPoly, &pSrc);
	if (!d3d_CheckVertexCount(nVerts))
		return;

	for (i=0; i < nVerts; i++)
	{
		verts[i].m_vPos = pSrc[i].m_pVert->m_Vec;
		verts[i].m_vPos += pPoly->m_pPlane->m_Normal * g_CV_ModelShadowOffset.m_fValue;
	}

	// Clip it to the shadow's frustum.
	pVerts = verts;
	if (!d3d_ClipToPlanes(&pVerts, &nVerts, pInfo->m_Planes, 6))
		return;

	if (pVerts != verts)
	{
		memcpy(verts, pVerts, nVerts * sizeof(ShadowVert));
		pVerts = verts;
	}

	for (i=0; i < nVerts; i++)
	{
		pVert = &pVerts[i];
		pMat = &pInfo->m_mTex;

		fAlpha = 1.0f - (pVerts[i].m_vPos - pInfo->m_vOrigin).Mag() / fMaxDist;
		if (fAlpha < 0.0f)
			fAlpha = 0.0f;

		pVert->m_Diffuse.m_n = 0xFFFFFFFF;
		pVert->m_Diffuse.m_b[3] = (uint8)(g_CV_ModelShadowAlpha.m_fValue * fAlpha);

		pVert->m_fU = pMat->m[0][0] * pVert->m_vPos.x + pMat->m[0][1] * pVert->m_vPos.y +
			pMat->m[0][2] * pVert->m_vPos.z + pMat->m[0][3];
		pVert->m_fV = pMat->m[1][0] * pVert->m_vPos.x + pMat->m[1][1] * pVert->m_vPos.y +
			pMat->m[1][2] * pVert->m_vPos.z + pMat->m[1][3];
		pVert->m_fQ = pMat->m[3][0] * pVert->m_vPos.x + pMat->m[3][1] * pVert->m_vPos.y +
			pMat->m[3][2] * pVert->m_vPos.z + pMat->m[3][3];

		pVert->m_Specular.m_n = 0;
	}

	g_ClipFlags = 0x3f;
	if (!d3d_TransformShadowPoly(&pVerts, &nVerts, (ViewParams*)&g_ViewParams, 0))
		return;

	for (i=0; i < nVerts; i++)
	{
		tlVerts[i].m_vPos = pVerts[i].m_vPos;
		tlVerts[i].m_fRHW = pVerts[i].m_fRHW;
		tlVerts[i].m_Diffuse = pVerts[i].m_Diffuse;
		tlVerts[i].m_Specular = pVerts[i].m_Specular;
		tlVerts[i].m_fU = pVerts[i].m_fU / pVerts[i].m_fQ;
		tlVerts[i].m_fV = pVerts[i].m_fV / pVerts[i].m_fQ;
	}

	g_pD3DDevice->DrawPrimitive(D3DPT_TRIANGLEFAN, D3DFVF_TLVERTEX, tlVerts, nVerts, 0);
}
