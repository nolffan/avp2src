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
#include <math.h>

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

// The shadow being cast: its frustum and its texture projection.  Its
// (empty) constructor is out of line, folded with drawmodel.cpp's empty
// ones (0x1000b981).
struct ModelShadowInfo
{
	ModelShadowInfo();

	LTVector	m_vOrigin;                              // +0x00 the projection's origin
	LTVector	m_vRight;                               // +0x0c
	LTVector	m_vUp;                                  // +0x18
	LTVector	m_vDir;                                 // +0x24
	LTPlane		m_Plane;                                // +0x30 through the model, facing away
	LTVector	m_vPos;                                 // +0x40 the model's position
	LTVector	m_vCorner;                              // +0x4c the texture's corner
	float		m_fSizeX;                               // +0x58
	float		m_fSizeY;                               // +0x5c
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

// The model's geometry: its pieces, each its own first LOD (the vertex
// and triangle arrays) followed by its lower ones.
struct ShadowModelTri                                   // 0x20 bytes
{
	uint16		m_Indices[3];                           // +0x00
	uint8		m_pad_06[0x20 - 0x06];
};

struct PieceLOD                                         // 0x24 bytes
{
	void*		m_pVertsVTable;                         // +0x00 CMoArray<ModelVert>
	void*		m_pVerts;                               // +0x04
	uint32		m_nVerts;                               // +0x08
	uint32		m_nVertsAlloc;                          // +0x0c
	void*		m_pTrisVTable;                          // +0x10 CMoArray<ModelTri>
	ShadowModelTri*	m_pTris;                            // +0x14
	uint32		m_nTris;                                // +0x18
	uint8		m_pad_1c[0x24 - 0x1c];
};

struct ModelPiece : public PieceLOD
{
	PieceLOD*	GetLOD(uint32 iLOD)
	{
		if (iLOD == 0)
			return this;

		iLOD--;
		if (iLOD < m_nLODs)
			return &m_pLODs[iLOD];

		return LTNULL;
	}

	uint8		m_pad_24[0x38 - 0x24];
	PieceLOD*	m_pLODs;                                // +0x38
	uint32		m_nLODs;                                // +0x3c
};

struct ShadowModel
{
	uint8		m_pad_00[0x38];
	ModelPiece**	m_pPieces;                          // +0x38
	uint32		m_nPieces;                              // +0x3c
	uint8		m_pad_40[0x74 - 0x40];
	uint32		m_VertFlags;                            // +0x74
	uint8		m_pad_78[0x94 - 0x78];
	uint32		m_nLODs;                                // +0x94
	uint8		m_pad_98[0xd4 - 0x98];
	uint32		m_bShadow;                              // +0xd4
	float		m_fShadowLength;                        // +0xd8
	float		m_fShadowStart;                         // +0xdc
	float		m_fShadowSizeX;                         // +0xe0
	float		m_fShadowSizeY;                         // +0xe4
	LTVector	m_vShadowOffset;                        // +0xe8
};

// The model being drawn.
struct ShadowModelInstance
{
	uint8		m_pad_000[0x194];
	LTVector	m_Pos;                                  // +0x194
	LTVector	m_Dims;                                 // +0x1a0
#ifdef PRIMAL_HUNT
	uint8		m_pad_1ac[0x260 - 0x1ac];
#else
	uint8		m_pad_1ac[0x24c - 0x1ac];
#endif
	uint32		m_HiddenPieces;                         // +0x260 (AvP2 +0x24c)
};

// A light the model is lit by.
struct ShadowModelLight                                 // 0x28 bytes
{
	LTVector	m_Pos;                                  // +0x00
	float		m_fRadius;                              // +0x0c
	uint8		m_pad_10[0x28 - 0x10];
};

// A transformed model vertex.
struct ShadowModelTLVert                                // 0x20 bytes
{
	LTVector	m_Vec;                                  // +0x00
	uint8		m_pad_0c[0x20 - 0x0c];
};

#define MAX_MODEL_SHADOWS		8

// The model drawer (TU02's CD3DModelDraw), the members the shadows use.
class CD3DModelDraw
{
public:
	void		DrawShadowPoly(ModelShadowInfo* pInfo, WorldPoly* pPoly);  // 0x10025ed1
	void		DrawProjShadowPoly(ModelShadowInfo* pInfo, WorldPoly* pPoly, float fMaxDist);  // 0x10027b95
	void		DrawShadows();                          // 0x1002611f
	void		DrawProjShadows(uint32 nMaxShadows);    // 0x10027e40

	// d3d_modelvb.cpp's vertex transform (0x10004660).
	void		TransformVerts(PieceLOD* pLOD, PieceLOD* pPrevLOD, ShadowModelTLVert* pOut,
					uint32 flags, uint32 vertFlags, LTVector* pBox, uint32 bLight,
					LTVector* pMin, LTVector* pMax);

	ShadowModel*	m_pModel;                           // +0x000
	ShadowModelInstance* m_pInstance;                   // +0x004
	uint8		m_pad_008[0x3c - 0x08];
	ShadowModelLight m_Lights[16];                      // +0x03c
	uint32		m_nLights;                              // +0x2bc
	uint8		m_pad_2c0[0x4d0 - 0x2c0];
	LTMatrix	m_mLightTransform;                      // +0x4d0
	uint8		m_pad_510[0x5ec - 0x510];
	uint32		m_TransformFlags;                       // +0x5ec
	uint8		m_pad_5f0[0x60c - 0x5f0];
	uint32		m_iLOD;                                 // +0x60c
	uint32		m_bBlendLOD;                            // +0x610
	uint8		m_pad_614[0x644 - 0x614];
	LTVector	m_vShadowDirs[MAX_MODEL_SHADOWS];       // +0x644
	uint8		m_pad_6a4[0x828 - 0x6a4];
	void*		m_pTLVertsVTable;                       // +0x828 CMoArray<ModelTLVert>
	ShadowModelTLVert*	m_pTLVerts;                     // +0x82c
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


// The world's object manager: the segment query around a point.
struct SegmentObject;
struct SegmentQuery;
typedef void (*SegmentQueryFn)(SegmentObject* pObject, SegmentQuery* pQuery);

class ShadowObjectMgr
{
public:
	virtual void	Slot00();
	virtual void	Slot04();
	virtual void	Slot08();
	virtual void	FindObjectsInSphere(LTVector* pPos, SegmentQueryFn fn, void* pUser, uint32 flags); // +0x0c
};

struct SceneWorld
{
	uint8			m_pad_00[0x6c];
	ShadowObjectMgr	m_ObjectMgr;                        // +0x6c
};

extern SceneWorld*	g_pSceneWorld;                      // 0x1005c208
extern void		d3d_GetSegmentPolies(SegmentObject* pObject, SegmentQuery* pQuery);  // 0x100366da

// The polygons a light's shadow falls on (drawlightmaps.cpp's
// SegmentPolyList), and the query that gathers them.
#define MAX_SHADOW_POLIES	256

struct ShadowPolyList
{
	WorldPoly*	m_Polies[MAX_SHADOW_POLIES];            // +0x000
	uint32		m_nPolies;                              // +0x400
};

struct ShadowPolyQuery
{
	ShadowPolyList*	m_pList;                            // +0x00
	LTVector	m_vStart;                               // +0x04
	LTVector	m_vEnd;                                 // +0x10
	float		m_fRadius;                              // +0x1c
	LTVector	m_vPos;                                 // +0x20
	LTVector	m_vDir;                                 // +0x2c
};

// The planar projection from a point light (0x10026ddd).
class ShadowProjection
{
public:
	void		SetupPlanar(LTVector vLight, LTPlane plane);

	LTMatrix	m_mProj;                                // +0x00
};

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
//  0x1002611f -- the model's shadows: for each shadow direction (the
//  first straight down) the polygons below the model are gathered and the
//  falloff texture is projected onto them through the shadow's frustum.
//  With ModelShadowProj set, the projected shadows draw instead.
// ---------------------------------------------------------------------- //
#define MAX_PROJ_SHADOWS		8

#define D3DPT_TRIANGLELIST		4

#define D3DTSS_COLOROP			1
#define D3DTSS_COLORARG1		2
#define D3DTSS_COLORARG2		3
#define D3DTSS_ALPHAOP			4
#define D3DTSS_ALPHAARG1		5
#define D3DTSS_ALPHAARG2		6
#define D3DTSS_ADDRESS			12

#define D3DRENDERSTATE_ZENABLE			7
#define D3DRENDERSTATE_ZWRITEENABLE		14
#define D3DRENDERSTATE_SRCBLEND			19
#define D3DRENDERSTATE_DESTBLEND		20
#define D3DRENDERSTATE_ALPHABLENDENABLE	27
#define D3DRENDERSTATE_FOGENABLE		28

extern int32				g_CV_MaxModelShadows;       // 0x1004a77c
extern LTBOOL				g_bCanDoShadows;            // 0x100622a8
struct IDirectDrawSurface7;
extern IDirectDrawSurface7*	g_pFalloffTexture;          // 0x10068314
extern void		gr_GetPerpendicularVector(LTVector* pVec, LTVector* pRef, LTVector* pPerp);  // 0x10032c6c

// A render state set for the life of the object.
class ShadowRenderState
{
public:
	ShadowRenderState(uint32 state, uint32 value)
	{
		m_State = state;
		g_pD3DDevice->GetRenderState(m_State, &m_OldValue);
		g_pD3DDevice->SetRenderState(m_State, value);
	}

	~ShadowRenderState()
	{
		g_pD3DDevice->SetRenderState(m_State, m_OldValue);
	}

	uint32		m_State;
	uint32		m_OldValue;
};

// The shadow's first direction, straight down.
struct ShadowDownVector
{
	ShadowDownVector() { m_Vec.Init(0.0f, -1.0f, 0.0f); }
	~ShadowDownVector() {}

	LTVector	m_Vec;
};

void CD3DModelDraw::DrawShadows()
{
	ModelShadowInfo info;
	ShadowPolyList polyLists[MAX_MODEL_SHADOWS];
	LTVector vLocalDirs[MAX_MODEL_SHADOWS];
	ShadowPolyQuery query;
	ShadowProjection proj;
	LTMatrix mBasis, mScale;
	LTVector vStart;
	void* pOldTexture;
	int32 nShadows, i;
	uint32 j, nTotal;

	nShadows = LTMIN(g_CV_MaxModelShadows, MAX_MODEL_SHADOWS);
	if (nShadows < 0)
		nShadows = 0;

	if (!nShadows)
		return;

	if (!g_bCanDoShadows || !g_pFalloffTexture || !m_pModel->m_bShadow)
		return;

	if (g_CV_ModelShadowProj.m_nValue)
	{
		DrawProjShadows(nShadows);
		return;
	}

	float fShadowLength = m_pModel->m_fShadowLength;
	LTVector vShadowOffset = m_pModel->m_vShadowOffset;
	float fSizeX = m_pModel->m_fShadowSizeX;
	float fSizeY = m_pModel->m_fShadowSizeY;
	float fShadowStart = m_pModel->m_fShadowStart;

	static ShadowDownVector s_vDown;
	m_vShadowDirs[0] = s_vDown.m_Vec;
	m_vShadowDirs[0].Norm(1.0f);

	// Gather the polygons along each direction.
	nTotal = 0;
	for (i=0; i < nShadows; i++)
	{
		float fLength = fShadowStart + fShadowLength;

		query.m_pList = &polyLists[i];
		polyLists[i].m_nPolies = 0;

		vStart = m_pInstance->m_Pos - m_vShadowDirs[i] * fShadowStart;
		query.m_vPos = vStart;

		vLocalDirs[i] = (m_pInstance->m_Pos + vShadowOffset) - vStart;
		vLocalDirs[i].Norm(1.0f);
		query.m_vDir = vLocalDirs[i];

		query.m_vStart = query.m_vPos;
		query.m_vEnd = query.m_vPos + query.m_vDir * fLength;
		query.m_fRadius = LTMAX(fSizeY, fSizeX);

		g_pSceneWorld->m_ObjectMgr.FindObjectsInSphere(&m_pInstance->m_Pos, d3d_GetSegmentPolies, &query, 0);
		nTotal += polyLists[i].m_nPolies;
	}

	if (!nTotal)
		return;

	g_pD3DDevice->GetTexture(0, &pOldTexture);
	g_pD3DDevice->SetTexture(0, g_pFalloffTexture);

	ShadowRenderState alphaBlend(D3DRENDERSTATE_ALPHABLENDENABLE, LTTRUE);
	ShadowRenderState zWrite(D3DRENDERSTATE_ZWRITEENABLE, LTFALSE);

	// Draw each.
	for (i=0; i < nShadows; i++)
	{
		float fLength = fShadowStart + fShadowLength;

		if (polyLists[i].m_nPolies)
		{
			info.m_fSizeX = fSizeX;
			info.m_fSizeY = fSizeY;

			vStart = m_pInstance->m_Pos - vLocalDirs[i] * fShadowStart;
			info.m_vOrigin = vStart;
			info.m_Plane.m_Normal = vLocalDirs[i];
			info.m_vPos = info.m_vOrigin + vLocalDirs[i] * fLength;
			info.m_Plane.m_Dist = info.m_Plane.m_Normal.Dot(info.m_vPos);

			info.m_vDir = vLocalDirs[i];
			gr_GetPerpendicularVector(&info.m_vDir, LTNULL, &info.m_vUp);
			info.m_vRight = info.m_vDir.Cross(info.m_vUp);

			// The frustum's corners.
			LTVector vC0 = info.m_vPos - info.m_vRight * fSizeX;
			LTVector vC1 = info.m_vPos + info.m_vRight * fSizeX;
			LTVector vC2 = info.m_vPos + info.m_vUp * fSizeY;
			LTVector vC3;
			vC3 = info.m_vPos - info.m_vUp * fSizeY;
			info.m_vCorner = (info.m_vPos - info.m_vRight * fSizeX) - info.m_vUp * fSizeY;

			// Its planes: the two caps and the four sides.
			info.m_Planes[0].m_Normal = info.m_vDir;
			info.m_Planes[0].m_Dist = info.m_vDir.Dot(m_pInstance->m_Pos);
			info.m_Planes[1].m_Normal = -info.m_vDir;
			info.m_Planes[1].m_Dist = info.m_Planes[1].m_Normal.Dot(m_pInstance->m_Pos + info.m_vDir * fShadowLength);

			info.m_Planes[2].m_Normal = (vC0 - info.m_vOrigin).Cross(info.m_vUp);
			info.m_Planes[2].m_Normal.Norm(1.0f);
			info.m_Planes[2].m_Dist = info.m_Planes[2].m_Normal.Dot(info.m_vOrigin);

			info.m_Planes[4].m_Normal = info.m_vUp.Cross(vC1 - info.m_vOrigin);
			info.m_Planes[4].m_Normal.Norm(1.0f);
			info.m_Planes[4].m_Dist = info.m_Planes[4].m_Normal.Dot(info.m_vOrigin);

			info.m_Planes[3].m_Normal = (vC2 - info.m_vOrigin).Cross(info.m_vRight);
			info.m_Planes[3].m_Normal.Norm(1.0f);
			info.m_Planes[3].m_Dist = info.m_Planes[3].m_Normal.Dot(info.m_vOrigin);

			info.m_Planes[5].m_Normal = info.m_vRight.Cross(vC3 - info.m_vOrigin);
			info.m_Planes[5].m_Normal.Norm(1.0f);
			info.m_Planes[5].m_Dist = info.m_Planes[5].m_Normal.Dot(info.m_vOrigin);

			// The texture projection.
			proj.SetupPlanar(info.m_vOrigin, info.m_Plane);

			mBasis.Init(
				info.m_vRight.x, info.m_vRight.y, info.m_vRight.z, -info.m_vRight.Dot(info.m_vCorner),
				info.m_vUp.x, info.m_vUp.y, info.m_vUp.z, -info.m_vUp.Dot(info.m_vCorner),
				0.0f, 0.0f, 1.0f, 0.0f,
				0.0f, 0.0f, 0.0f, 1.0f);

			mScale.Init(
				1.0f / (info.m_fSizeX * 2.0f), 0.0f, 0.0f, 0.0f,
				0.0f, 1.0f / (info.m_fSizeY * 2.0f), 0.0f, 0.0f,
				0.0f, 0.0f, 1.0f, 0.0f,
				0.0f, 0.0f, 0.0f, 1.0f);

			info.m_mTex = mScale * mBasis * proj.m_mProj;

			for (j=0; j < polyLists[i].m_nPolies; j++)
				DrawShadowPoly(&info, polyLists[i].m_Polies[j]);
		}
	}

	g_pD3DDevice->SetTexture(0, pOldTexture);
}


// ---------------------------------------------------------------------- //
//  0x10026ddd -- the matrix that projects from a point light onto a
//  plane (the planar shadow).
// ---------------------------------------------------------------------- //
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


// ---------------------------------------------------------------------- //
//  0x10027e40 -- the projected shadows: the nearest lights (up to
//  nMaxShadows of them) each render the model, flattened along the light's
//  direction, into a square render target, which is then projected onto
//  the world polygons around the model.
// ---------------------------------------------------------------------- //

// The render targets the shadow is drawn into (lmdynamic.cpp's buffers).
class ShadowTarget
{
public:
	virtual void	Slot00();
	virtual void	Slot04();
	virtual void	Slot08();
	virtual void	SetTexture();                                     // +0x0c
	virtual void	Show(uint32 x, uint32 y, uint32 color, uint32 flags); // +0x10
	virtual void	CopyFrom(uint32 x, uint32 y);                     // +0x14
	virtual void	BeginDraw(uint32 x, uint32 y);                    // +0x18
};

extern void*	d3d_GetLMBuffer(uint32 width, uint32 height);   // 0x100331d7
extern void*	d3d_GetLMBuffer2(uint32 width, uint32 height);  // 0x10033227

struct RenderStruct
{
	uint8		m_pad_00[0x14];
	void		(*ConsolePrint)(char* pMsg);            // +0x14
};

extern RenderStruct*	g_pStruct;                      // 0x1005df08


void CD3DModelDraw::DrawProjShadows(uint32 nMaxShadows)
{
	LTVector vLightPositions[MAX_PROJ_SHADOWS];
	LTVector vLightDirs[MAX_PROJ_SHADOWS];
	float fLightDist[MAX_PROJ_SHADOWS];
	float fLightRadius[MAX_PROJ_SHADOWS];
	ShadowPolyList polyLists[MAX_PROJ_SHADOWS];
	ShadowPolyQuery query;
	ShadowTLVert verts[4];
	ShadowProjection proj;
	LTMatrix mCamToWorld, mInvView, mBasis, mScale, mTemp;
	LTVector vObjPos, vDims, vLight, vTemp, vDir, vRight, vUp, vPos;
	LTVector vRightS, vUpS, vUpHalf, vLeft, vRightP, vTop, vBottom;
	LTVector vBox[2], vMin, vMax;
	ShadowModelLight* pLight;
	ShadowPolyList* pList;
	float* pRadius;
	ModelPiece* pPiece;
	PieceLOD *pLOD, *pPrevLOD;
	ShadowModelTLVert *pTLVerts, *pVert;
	ShadowModelTri* pTri;
	ShadowTarget *pTarget, *pBuffer;
	WorldPoly** ppPoly;
	LTPlane* pPlane;
	void* pOldTexture;
	uint32 oldStages[7], oldStates[6];
	uint32 nShadows, i, j, k, nRes, iLOD, iPiece, nVerts, nTris;
	float fDist, fMag, fSize, fRes, fScale, fMaxX, fMaxY, fUScale, fVScale, fHalf, fRadius;
	float d1, d2;

	vObjPos = m_pInstance->m_Pos;
	vDims = m_pInstance->m_Dims;

	// Find the nearest lights.
	nShadows = 0;
	for (i=0, pLight=m_Lights; i < m_nLights; i++, pLight++)
	{
		LTVector vLightPos = pLight->m_Pos;
		MatVMul_InPlace(&m_mLightTransform, &vLightPos);

		LTVector vTo(vObjPos.x - vLightPos.x, vObjPos.y - vLightPos.y, vObjPos.z - vLightPos.z);
		fDist = vTo.Mag();
		LTVector vLightDir = vTo;
		vLightDir.Norm(1.0f);

		if (nShadows == nMaxShadows)
		{
			for (j=0; j < nMaxShadows; j++)
			{
				if (fLightDist[j] > fDist)
					break;
			}

			if (j == nMaxShadows)
				continue;
		}
		else
		{
			j = nShadows;
		}

		VEC_COPY(vLightPositions[j], vLightPos);
		VEC_COPY(vLightDirs[j], vLightDir);
		fLightDist[j] = fDist;
		fLightRadius[j] = (float)sqrt(pLight->m_fRadius);

		if (nShadows < nMaxShadows)
			nShadows++;
	}

	if (!nShadows)
		return;

	nRes = (g_CV_ModelShadowProjRes.m_nValue < 8) ? 8 :
		((g_CV_ModelShadowProjRes.m_nValue > 256) ? 256 : g_CV_ModelShadowProjRes.m_nValue);
	if (nRes & (nRes - 1))
	{
		g_pStruct->ConsolePrint("Invalid texture size for shadows -- must be power of 2...");
		nRes = 128;
		g_CV_ModelShadowProjRes.m_nValue = 128;
		g_CV_ModelShadowProjRes.m_fValue = 128.0f;
	}

	iLOD = m_iLOD + g_CV_ModelShadowProjLOD.m_nValue;
	if (iLOD >= m_pModel->m_nLODs - 1)
		iLOD = m_pModel->m_nLODs - 1;

	fMag = vDims.Mag();
	fSize = fMag + fMag;

	// Save the device's state.
	g_pD3DDevice->GetTexture(0, &pOldTexture);
	g_pD3DDevice->SetTexture(0, LTNULL);
	g_pD3DDevice->SetTexture(1, LTNULL);
	g_pD3DDevice->GetTextureStageState(0, D3DTSS_ADDRESS, &oldStages[0]);
	g_pD3DDevice->GetTextureStageState(0, D3DTSS_COLORARG1, &oldStages[1]);
	g_pD3DDevice->GetTextureStageState(0, D3DTSS_COLORARG2, &oldStages[2]);
	g_pD3DDevice->GetTextureStageState(0, D3DTSS_COLOROP, &oldStages[3]);
	g_pD3DDevice->GetTextureStageState(0, D3DTSS_ALPHAARG1, &oldStages[4]);
	g_pD3DDevice->GetTextureStageState(0, D3DTSS_ALPHAARG2, &oldStages[5]);
	g_pD3DDevice->GetTextureStageState(0, D3DTSS_ALPHAOP, &oldStages[6]);
	g_pD3DDevice->GetRenderState(D3DRENDERSTATE_ZENABLE, &oldStates[0]);
	g_pD3DDevice->GetRenderState(D3DRENDERSTATE_ZWRITEENABLE, &oldStates[1]);
	g_pD3DDevice->GetRenderState(D3DRENDERSTATE_ALPHABLENDENABLE, &oldStates[2]);
	g_pD3DDevice->GetRenderState(D3DRENDERSTATE_FOGENABLE, &oldStates[3]);
	g_pD3DDevice->GetRenderState(D3DRENDERSTATE_SRCBLEND, &oldStates[4]);
	g_pD3DDevice->GetRenderState(D3DRENDERSTATE_DESTBLEND, &oldStates[5]);
	g_pD3DDevice->SetRenderState(D3DRENDERSTATE_FOGENABLE, LTFALSE);

	pTarget = (ShadowTarget*)d3d_GetLMBuffer2(nRes, nRes);

	pList = polyLists;
	pRadius = fLightRadius;
	for (i=0; i < nShadows; i++)
	{
		// The polygons around the model, toward the light.
		query.m_pList = pList;
		pList->m_nPolies = 0;

		fRadius = *pRadius;
		VEC_COPY(vPos, vLightPositions[i]);
		VEC_COPY(vDir, vLightDirs[i]);

		query.m_vPos = vPos;
		query.m_vDir = vDir;
		query.m_vStart = vPos;
		query.m_vEnd = vPos + vDir * fRadius;
		query.m_fRadius = fSize;

		g_pSceneWorld->m_ObjectMgr.FindObjectsInSphere(&vObjPos, d3d_GetSegmentPolies, &query, 0);

		if (pList->m_nPolies)
		{
			// The light's basis.
			LTVector vWorldUp(0.0f, 1.0f, 0.0f);

			if (fabs(vDir.y) > 0.9999f)
			{
				vDir.z += 0.01f;
				vDir.Norm(1.0f);
			}

			vRight = vDir.Cross(vWorldUp);
			vRight.Norm(1.0f);
			vUp = vRight.Cross(vDir);
			vUp.Norm(1.0f);

			// Draw the model into the target, black on white.
			pTarget->BeginDraw(0, 0);
			g_pD3DDevice->SetRenderState(D3DRENDERSTATE_ZENABLE, LTFALSE);
			g_pD3DDevice->SetRenderState(D3DRENDERSTATE_ZWRITEENABLE, LTFALSE);
			g_pD3DDevice->SetRenderState(D3DRENDERSTATE_ALPHABLENDENABLE, LTFALSE);

			fRes = (float)(int32)nRes;
			for (k=0; k < 4; k++)
			{
				verts[k].m_fRHW = 0.0f;
				verts[k].m_Diffuse.m_n = 0xFFFFFFFF;
				verts[k].m_fU = 0.0f;
				verts[k].m_fV = 0.0f;
			}

			verts[0].m_vPos.Init(0.0f, 0.0f, 0.0f);
			verts[1].m_vPos.Init(0.0f, fRes, 0.0f);
			verts[2].m_vPos.Init(fRes, fRes, 0.0f);
			verts[3].m_vPos.Init(fRes, 0.0f, 0.0f);
			g_pD3DDevice->DrawPrimitive(D3DPT_TRIANGLEFAN, D3DFVF_TLVERTEX, verts, 4, 0);

			for (k=0; k < 4; k++)
				verts[k].m_Diffuse.m_n = 0;

			// The model's vertices, flattened into the light's plane.
			pTLVerts = m_pTLVerts;
			fMaxX = 0.0f;
			fMaxY = 0.0f;
			mInvView = g_ViewParams.m_mWorldToCamera;
			mInvView.Inverse();
			mCamToWorld = mInvView;

			for (iPiece=0; iPiece < m_pModel->m_nPieces; iPiece++)
			{
				pPiece = m_pModel->m_pPieces[iPiece];
				pLOD = pPiece->GetLOD(iLOD);
				if (!pLOD)
					continue;

				pPrevLOD = m_bBlendLOD ? pPiece->GetLOD(iLOD + 1) : LTNULL;
				if (m_pInstance->m_HiddenPieces & (1 << iPiece))
					continue;

				TransformVerts(pLOD, pPrevLOD, pTLVerts, m_TransformFlags, m_pModel->m_VertFlags,
					vBox, 0, &vMin, &vMax);

				pVert = pTLVerts;
				nVerts = pLOD->m_nVerts;
				if (nVerts)
				{
					fScale = fRes / fMag;
					do
					{
						MatVMul_InPlace(&mCamToWorld, &pVert->m_Vec);

						vTemp = pVert->m_Vec - vObjPos;
						pVert->m_Vec.x = vRight.Dot(vTemp) * fScale * 0.5f;
						pVert->m_Vec.y = vUp.Dot(vTemp) * fScale * 0.5f;
						pVert->m_Vec.z = 0.0f;

						if (fabs(pVert->m_Vec.x) > fMaxX)
							fMaxX = (float)fabs(pVert->m_Vec.x);

						if (fabs(pVert->m_Vec.y) > fMaxY)
							fMaxY = (float)fabs(pVert->m_Vec.y);

						pVert++;
					}
					while (--nVerts);
				}

				pTLVerts += pLOD->m_nVerts;
			}

			// Draw its triangles, scaled to fill the target.
			fUScale = (fRes - 2.0f) / (fMaxX * 2.0f);
			fVScale = (fRes - 2.0f) / (fMaxY * 2.0f);
			pTLVerts = m_pTLVerts;
			for (iPiece=0; iPiece < m_pModel->m_nPieces; iPiece++)
			{
				pPiece = m_pModel->m_pPieces[iPiece];
				pLOD = pPiece->GetLOD(iLOD);
				if (!pLOD)
					continue;

				if (m_pInstance->m_HiddenPieces & (1 << iPiece))
					continue;

				pTri = pLOD->m_pTris;
				nTris = pLOD->m_nTris;
				if (nTris)
				{
					fHalf = fRes * 0.5f;
					do
					{
						verts[0].m_vPos = pTLVerts[pTri->m_Indices[0]].m_Vec;
						verts[1].m_vPos = pTLVerts[pTri->m_Indices[1]].m_Vec;
						verts[2].m_vPos = pTLVerts[pTri->m_Indices[2]].m_Vec;

						verts[0].m_vPos.x *= fUScale;
						verts[1].m_vPos.x *= fUScale;
						verts[2].m_vPos.x *= fUScale;
						verts[0].m_vPos.y *= fVScale;
						verts[1].m_vPos.y *= fVScale;
						verts[2].m_vPos.y *= fVScale;

						verts[0].m_vPos.x += fHalf;
						verts[1].m_vPos.x += fHalf;
						verts[2].m_vPos.x += fHalf;
						verts[0].m_vPos.y += fHalf;
						verts[1].m_vPos.y += fHalf;
						verts[2].m_vPos.y += fHalf;

						g_pD3DDevice->DrawPrimitive(D3DPT_TRIANGLELIST, D3DFVF_TLVERTEX, verts, 3, 0);
						pTri++;
					}
					while (--nTris);
				}

				pTLVerts += pLOD->m_nVerts;
			}

			pBuffer = (ShadowTarget*)d3d_GetLMBuffer(nRes, nRes);
			pBuffer->BeginDraw(0, 0);
			pTarget->CopyFrom(0, 0);

			if (g_CV_ModelShadowProjShow.m_nValue)
				pBuffer->Show(nRes, (nShadows - i - 1) * nRes, 0xFFFFFFFF, 0);

			// The shadow's frustum and projection.
			ModelShadowInfo info;

			info.m_vRight = vRight;
			info.m_vUp = vUp;
			info.m_vDir = vDir;
			info.m_fSizeX = fSize;
			info.m_fSizeY = fSize;
			info.m_vOrigin = vPos;
			info.m_Plane.m_Normal = vDir;
			info.m_vPos = vObjPos;
			info.m_Plane.m_Dist = vDir.Dot(vObjPos);

			vRightS = vRight * fSize;
			vLeft = vObjPos - vRightS * 0.5f;
			vRightP = vObjPos + vRightS * 0.5f;
			vUpS = vUp * fSize;
			vTop = vObjPos + vUpS * 0.5f;
			vBottom = vObjPos - vUpS * 0.5f;
			vUpHalf = vUpS * 0.5f;
			info.m_vCorner = vObjPos - vRightS * 0.5f - vUpHalf;

			info.m_Planes[0].m_Normal = vDir;
			info.m_Planes[0].m_Dist = vDir.Dot(vObjPos - vDir * fMag);
			info.m_Planes[1].m_Normal = -info.m_vDir;
			info.m_Planes[1].m_Dist = info.m_Planes[1].m_Normal.Dot(vObjPos + info.m_vDir * fRadius);

			info.m_Planes[2].m_Normal = (vLeft - info.m_vOrigin).Cross(info.m_vUp);
			info.m_Planes[2].m_Normal.Norm(1.0f);
			info.m_Planes[2].m_Dist = info.m_Planes[2].m_Normal.Dot(info.m_vOrigin);

			info.m_Planes[3].m_Normal = info.m_vUp.Cross(vRightP - info.m_vOrigin);
			info.m_Planes[3].m_Normal.Norm(1.0f);
			info.m_Planes[3].m_Dist = info.m_Planes[3].m_Normal.Dot(info.m_vOrigin);

			info.m_Planes[4].m_Normal = (vTop - info.m_vOrigin).Cross(info.m_vRight);
			info.m_Planes[4].m_Normal.Norm(1.0f);
			info.m_Planes[4].m_Dist = info.m_vOrigin.Dot(info.m_Planes[4].m_Normal);

			info.m_Planes[5].m_Normal = info.m_vRight.Cross(vBottom - info.m_vOrigin);
			info.m_Planes[5].m_Normal.Norm(1.0f);
			info.m_Planes[5].m_Dist = info.m_vOrigin.Dot(info.m_Planes[5].m_Normal);

			proj.SetupPlanar(info.m_vOrigin, info.m_Plane);

			mBasis.Init(
				info.m_vRight.x, info.m_vRight.y, info.m_vRight.z, -info.m_vRight.Dot(info.m_vCorner),
				info.m_vUp.x, info.m_vUp.y, info.m_vUp.z, -info.m_vUp.Dot(info.m_vCorner),
				0.0f, 0.0f, 1.0f, 0.0f,
				0.0f, 0.0f, 0.0f, 1.0f);

			mScale.Init(
				fUScale / info.m_fSizeX, 0.0f, 0.0f, (1.0f - fUScale) * 0.5f,
				0.0f, fVScale / info.m_fSizeY, 0.0f, (1.0f - fVScale) * 0.5f,
				0.0f, 0.0f, 1.0f, 0.0f,
				0.0f, 0.0f, 0.0f, 1.0f);

			mTemp = mScale * mBasis;
			info.m_mTex = mTemp * proj.m_mProj;

			// Project it onto the polygons on the model's far side.
			pBuffer->SetTexture();
			g_pD3DDevice->SetRenderState(D3DRENDERSTATE_ZENABLE, LTTRUE);
			g_pD3DDevice->SetRenderState(D3DRENDERSTATE_ZWRITEENABLE, LTFALSE);

			ppPoly = pList->m_Polies;
			for (k=0; k < pList->m_nPolies; k++)
			{
				pPlane = (*ppPoly)->m_pPlane;
				vLight = pPlane->m_Normal;

				d1 = vLight.Dot(vPos) - pPlane->m_Dist;
				d2 = vLight.Dot(vObjPos) - pPlane->m_Dist;

				if ((d1 > 0.0f || d2 < 0.0f) && (d1 < 0.0f || d2 > 0.0f))
					DrawProjShadowPoly(&info, *ppPoly, fRadius);

				ppPoly++;
			}
		}

		pRadius++;
		pList++;
	}

	// Restore the device's state.
	g_pD3DDevice->SetTextureStageState(0, D3DTSS_ADDRESS, oldStages[0]);
	g_pD3DDevice->SetTextureStageState(0, D3DTSS_COLORARG1, oldStages[1]);
	g_pD3DDevice->SetTextureStageState(0, D3DTSS_COLORARG2, oldStages[2]);
	g_pD3DDevice->SetTextureStageState(0, D3DTSS_COLOROP, oldStages[3]);
	g_pD3DDevice->SetTextureStageState(0, D3DTSS_ALPHAARG1, oldStages[4]);
	g_pD3DDevice->SetTextureStageState(0, D3DTSS_ALPHAARG2, oldStages[5]);
	g_pD3DDevice->SetTextureStageState(0, D3DTSS_ALPHAOP, oldStages[6]);
	g_pD3DDevice->SetRenderState(D3DRENDERSTATE_ZENABLE, oldStates[0]);
	g_pD3DDevice->SetRenderState(D3DRENDERSTATE_ZWRITEENABLE, oldStates[1]);
	g_pD3DDevice->SetRenderState(D3DRENDERSTATE_ALPHABLENDENABLE, oldStates[2]);
	g_pD3DDevice->SetRenderState(D3DRENDERSTATE_FOGENABLE, oldStates[3]);
	g_pD3DDevice->SetRenderState(D3DRENDERSTATE_SRCBLEND, oldStates[4]);
	g_pD3DDevice->SetRenderState(D3DRENDERSTATE_DESTBLEND, oldStates[5]);
	g_pD3DDevice->SetTexture(0, pOldTexture);
}
