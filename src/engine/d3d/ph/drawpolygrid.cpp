// *********************************************************************** //
//  ph/drawpolygrid.cpp -- Primal Hunt's d3d.ren, TU31
//  (0x1002be40..0x1002ddb8), /O2 /Ob2.
//
//  PolyGrids: the EnvMapPolyGrids console variable, the triangle clip
//  test, the OT_POLYGRID handlers (0x1004db14..0x1004db24) and the solid
//  and translucent passes.
//
//  /Ob2: the console variable's initialiser is one function (0x1002be40),
//  not a jmp thunk and its body.
//
//  The polygrid draw (0x1002be60, 5,473 bytes; AvP2 1.0.9.6's 0x1002aff0,
//  byte-identical to it there; NOLF's is w40_1005a960_qipni.cpp, without
//  the environment map) is written but near: 5,262 bytes here, where
//  retail expands MatMul, the rotation check and ClipPoly (calling the
//  plane clippers) and calls LTMatrix::operator* -- the /Ob2 budget again.
//  It emits _CVector's operator* and operator+ out of line (retail
//  0x1000fed2/0x1000feaf in Primal Hunt, fogvolume.cpp's copies; AvP2's
//  0x1002cec0/0x1002ce70, here).
//
//  Near: 0x1002d6b0, the environment map normals (AvP2's identical
//  0x1005a630, in the same file).  Ours expands _CVector::Norm (and its
//  Mag) where retail calls it (0x1000e62a): the /Ob2 budget, which also
//  leaves ours a 0xb0 frame against retail's 0xa0 and a different
//  register for the grid object.
// *********************************************************************** //

#include <windows.h>
#include <ddraw.h>
#include <d3d.h>

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


class RTexture;
class CRenderStateList;

// A texture as the engine shares it.
struct SharedTexture
{
	uint8			m_pad_00[0x0c];
	RTexture*		m_pRenderData;                      // +0x0c
	uint8			m_pad_10[0x28 - 0x10];
	SharedTexture*	m_pDetailTexture;                   // +0x28 (a polygrid's environment map)
	int32			m_DetailTextureMode;                // +0x2c
	uint16			m_FrameCode;                        // +0x30
	uint8			m_pad_32[0x34 - 0x32];
	CRenderStateList*	m_pStateList;                   // +0x34
};

// A texture in video memory (d3d_texture.cpp's RTexture), as the binding
// reads it.
class RTexture
{
public:
	uint8			m_pad_00[0x0c];
	LPDIRECTDRAWSURFACE7	m_pSurface;                 // +0x0c
	uint8			m_pad_10[0x30 - 0x10];
	RTexture*		m_pNext;                            // +0x30 its copy for the next stage
	uint8			m_pad_34[0x42 - 0x34];
	uint16			m_Stage;                            // +0x42
	uint16			m_LOD;                              // +0x44
};

class CRenderStateList
{
public:
	void		Restore();                              // 0x10022a86
	void		Apply(CRenderStateList *pList, uint32 iStage);  // 0x10022a97
};

struct SpriteEntry
{
	SharedTexture*	m_pTex;                             // +0x00
};

struct SpriteTracker
{
	uint8			m_pad_00[0x08];
	SpriteEntry*	m_pCurFrame;                        // +0x08
	uint8			m_pad_0c[0x18 - 0x0c];
};

struct LTObject
{
	uint8		m_pad_000[0x88];
	uint32		m_Flags;                                // +0x088
	uint32		m_Flags2;                               // +0x08c
	uint8		m_pad_090[0x97 - 0x90];
	uint8		m_ColorA;                               // +0x097
	uint8		m_pad_098[0x9c - 0x98];
	LTRotation	m_Rotation;                             // +0x09c
	LTVector	m_Scale;                                // +0x0ac
	uint8		m_pad_0b8[0xbc - 0xb8];
	char		m_ObjectType;                           // +0x0bc
	uint8		m_pad_0bd[0x194 - 0xbd];
	LTVector	m_Pos;                                  // +0x194
	uint8		m_pad_1a0[0x1b0 - 0x1a0];
	int8*		m_Data;                                 // +0x1b0 the heights
	uint16*		m_Indices;                              // +0x1b4
	void*		m_pSprite;                              // +0x1b8
	SpriteTracker	m_SpriteTracker;                    // +0x1bc
	float		m_xPan, m_yPan;                         // +0x1d4
	float		m_xScale, m_yScale;                     // +0x1dc
	uint32		m_nTris;                                // +0x1e4
	uint32		m_nIndices;                             // +0x1e8
	LTLink		m_LeafLinks;                            // +0x1ec
	uint32		m_Width;                                // +0x1f8
	uint32		m_Height;                               // +0x1fc
	PGColor		m_ColorTable[256];                      // +0x200
};

struct ViewParams
{
	uint8		m_pad_000[0x88];
	float		m_NearZ;                                // +0x088
	uint8		m_pad_08c[0x90 - 0x8c];
	float		m_FarZ2;                                // +0x090 (the far clip)
	uint8		m_pad_094[0x15c - 0x94];
	LTMatrix	m_mWorldToCamera;                       // +0x15c
	LTMatrix	m_mProjection;                          // +0x19c
	uint8		m_pad_1dc[0x3d0 - 0x1dc];
	LTPlane		m_ClipPlanes[6];                        // +0x3d0
	uint8		m_pad_430[0x4b4 - 0x430];
	LTVector	m_Pos;                                  // +0x4b4
	uint8		m_pad_4c0[0x4d4 - 0x4c0];
	uint32		m_bDrawHidden;                          // +0x4d4
};

typedef void (*DrawObjectFn)(ViewParams* pContext, LTObject* pObject);

extern void		AddDebugMessage(int32 level, const char* pMsg, ...);  // 0x10013b13

class BaseObjectSet
{
public:
	void		Add(LTObject *pObject)
	{
		if(m_nObjects < m_nMaxObjects)
		{
			m_pObjects[m_nObjects] = pObject;
			m_nObjects++;
		}
		else
		{
			AddDebugMessage(1, "Set '%s' overflowed", m_pSetName);
		}
	}

	void		Draw(ViewParams* pContext, DrawObjectFn fn);  // 0x100397e0

	template<class T>
	void		DrawFiltered(ViewParams* pContext, DrawObjectFn fn, T* pTester, BaseObjectSet* pRejects)
	{
		uint32 i;
		LTObject* pObject;

		pRejects->m_nObjects = 0;
		for (i=0; i < m_nObjects; i++)
		{
			pObject = m_pObjects[i];

			if (pObject->m_Flags & FLAG_VISIBLE)
			{
				if (pContext->m_bDrawHidden && (pObject->m_Flags2 & 1))
					continue;
			}
			else
			{
				if (pContext->m_bDrawHidden && !(pObject->m_Flags & 0x400))
					continue;
			}

			if (T::Filter(pObject))
				pRejects->Add(pObject);
			else
				fn(pContext, pObject);
		}
	}

	LTObject	**m_pObjects;                           // +0x00
	uint32		m_nObjects;                             // +0x04
	LTLink		m_Link;                                 // +0x08
	uint32		m_nMaxObjects;                          // +0x14
	const char	*m_pSetName;                            // +0x18
};

struct VisibleSet
{
	uint8			m_pad_000[0x264];
	BaseObjectSet	m_SolidPolyGrids;                   // +0x264
	uint8			m_pad_280[0x284 - 0x280];
	BaseObjectSet	m_TranslucentPolyGrids;             // +0x284
};

class ObjectDrawList
{
public:
	void	Add(LTObject* pObject, DrawObjectFn fn);    // 0x10029980
};

extern ObjectDrawList*	g_pObjectDrawList;              // 0x100713cc
extern ViewParams		g_ViewParams;                 // 0x1005b790
extern int32			g_CV_DrawPolyGrids;             // 0x1004a768
extern CD3DConVar		g_CV_UseD3DClip;                // 0x100622b0 (d3d_init.cpp)

#include "d3d_clip.h"

extern VisibleSet*	d3d_GetVisibleSet();                // 0x1003abb0
extern void			dfree(void* ptr);                   // 0x10013ac5

extern void d3d_DrawPolyGrid(ViewParams* pContext, LTObject* pObject);  // 0x1002be60


// A polygrid vertex (FVF 0x2c4, two texture coordinate pairs).
struct PolyGridVert                                     // 0x28 bytes
{
	LTVector	m_Vec;                                  // +0x00
	float		m_RHW;                                  // +0x0c
	uint32		m_Color;                                // +0x10
	uint32		m_Specular;                             // +0x14
	float		m_TU1, m_TV1;                           // +0x18 the environment map's
	float		m_TU2, m_TV2;                           // +0x20
};

extern void		d3d_CalcEnvMapUV(const LTVector* pEye, const LTVector* pPos,
					const LTVector* pNormal, float* pU, float* pV);  // 0x100115fe



// ---------------------------------------------------------------------- //
//  Globals.
// ---------------------------------------------------------------------- //
CD3DConVar	g_CV_EnvMapPolyGrids("EnvMapPolyGrids", 1.0f, LTNULL, LTNULL);  // 0x100772f8  $E 0x1002be40


// ---------------------------------------------------------------------- //
//  0x1002be60 -- draw a polygrid: one vertex per height, coloured from the
//  grid's colour table, with the sprite's texture (and the texture's
//  environment map on the first stage, when there is one); triangles the
//  frustum cuts are clipped and drawn alone, the rest go in one indexed
//  draw.
// ---------------------------------------------------------------------- //
extern LPDIRECT3DDEVICE7	g_pD3DDevice;               // 0x100638c8
extern RTexture*			g_pCurTextures[];           // 0x10067270
extern uint32				g_iCurTextureStage;         // 0x100638c0
extern uint16				g_CurTextureFrameCode;      // 0x1005d250
extern LTBOOL				g_100638c4;                 // 0x100638c4 (d3d_init.cpp): env maps work
extern CRenderStateList		g_SavedStates;              // 0x10069728
extern uint8				g_LightTableR[256];         // 0x1005fa9c
extern uint8				g_LightTableG[256];         // 0x1005fb9c
extern uint8				g_LightTableB[256];         // 0x1005fc9c

struct TexPair
{
	float	u, v;
};

extern TexPair				g_TexelSize[];              // 0x100672a8

extern RTexture*	d3d_CreateRenderTexture(SharedTexture* pTexture, uint32 iStage, LTBOOL bStageCopy);  // 0x10020cd0
extern void			d3d_SetTextureDirect(RTexture* pTexture);   // 0x10007a89
extern void			d3d_RotationToMatrix(LTRotation* pRot, LTMatrix* pMat);  // 0x1003655d
extern void			d3d_SetDetailTextureStage(int nMode);       // 0x1000a172
extern void			d3d_UnsetDetailTextureStage();              // 0x1000a1c1
struct TLVertex;
extern void			d3d_ProjectTLVert(TLVertex* pVert, ViewParams* pParams);  // 0x1002739d (drawmodelshadows.cpp)
extern void*		dalloc(uint32 size);                        // 0x10013a66

int d3d_ClassifyTriangle(uint32 nClipMask, uint32* pOutMask,
                         ClipVert* pV0, ClipVert* pV1, ClipVert* pV2);
void d3d_SetupPolyGridEnvMap(LTObject* pObject, LTVector* pUp, LTVector* pRight, LTVector* pForward);
void d3d_TransformPolyGridVerts(PolyGridVert* pVerts, uint32 nVerts);

PolyGridVert*	g_pPolyGridVerts;                       // 0x100762e8
uint32			g_nPolyGridVerts;                       // 0x10075ee0
uint16*			g_pPolyGridIndices;                     // 0x100762f0
uint32			g_nPolyGridIndices;                     // 0x10075ee4

// The texture coordinates' offsets and scales, and the alpha scale.
float			g_fPolyGridUScale;                      // 0x10075ed0
float			g_fPolyGridUOffset;                     // 0x10075ed4
float			g_fPolyGridVOffset;                     // 0x10075ed8
float			g_fPolyGridVScale;                      // 0x10075edc
float			g_fPolyGridAlpha;                       // 0x100762ec

#define PHFLAG_FOGDISABLE		0x80
#define PHFLAG2_ADDITIVE		0x04
#define PHFLAG2_MULTIPLY		0x08
#define PHOT_MODEL				1
#define PG_UNSIGNED				0x2         // the grid's heights are 0..255
#define PH_FLAG_INTERNAL		0x40000000

#define MAX_ROTATION_COMPONENT	100000.0f

// Set a render state for the life of the object (drawsprite.cpp's).
class StateSet
{
public:
	StateSet(D3DRENDERSTATETYPE state, uint32 val)
	{
		m_State = state;
		g_pD3DDevice->GetRenderState(state, (unsigned long *)&m_OldVal);
		g_pD3DDevice->SetRenderState(state, val);
	}

	~StateSet()
	{
		g_pD3DDevice->SetRenderState(m_State, m_OldVal);
	}

	D3DRENDERSTATETYPE	m_State;
	uint32				m_OldVal;
};

// The object's blend states (drawsprite.cpp's, without the additive alpha).
inline void d3d_GetBlendStates(LTObject *pObject, uint32 &srcBlend, uint32 &destBlend, uint32 &dwFog, uint32 &dwFogColor)
{
	g_pD3DDevice->GetRenderState(D3DRENDERSTATE_FOGCOLOR, (unsigned long *)&dwFogColor);

	g_pD3DDevice->GetRenderState(D3DRENDERSTATE_FOGENABLE, (unsigned long *)&dwFog);
	if ((pObject->m_Flags & PHFLAG_FOGDISABLE) && pObject->m_ObjectType != PHOT_MODEL)
	{
		dwFog = 0;
	}

	if (pObject->m_Flags2 & PHFLAG2_ADDITIVE)
	{
		srcBlend	= D3DBLEND_ONE;
		destBlend	= D3DBLEND_ONE;
		dwFogColor	= 0;
	}
	else if (pObject->m_Flags2 & PHFLAG2_MULTIPLY)
	{
		srcBlend	= D3DBLEND_ZERO;
		destBlend	= D3DBLEND_SRCCOLOR;
		dwFogColor	= 0xFFFFFFFF;
	}
	else
	{
		srcBlend	= D3DBLEND_SRCALPHA;
		destBlend	= D3DBLEND_INVSRCALPHA;
	}
}

// Bind a shared texture to a stage (drawmodel_clip.cpp's 0x100079e4).
inline int d3d_SetTexture(SharedTexture* pTexture, uint32 iStage, uint32 iLOD)
{
	RTexture *pRenderData, *pCur;

	if (!pTexture)
		return LTFALSE;

	pRenderData = pTexture->m_pRenderData;
	pTexture->m_FrameCode = g_CurTextureFrameCode;

	pCur = LTNULL;
	if (pRenderData)
	{
		for (pCur = pRenderData; pCur; pCur = pCur->m_pNext)
		{
			if (pCur->m_Stage == (uint16)(uint8)iStage)
				break;
		}
	}

	if (!pCur || pCur != g_pCurTextures[iStage])
	{
		if (!pCur)
		{
			pRenderData = pTexture->m_pRenderData;
			if (pRenderData)
			{
				pCur = d3d_CreateRenderTexture(pTexture, iStage, LTTRUE);
				if (!pCur)
					return LTFALSE;

				pCur->m_pNext = pRenderData->m_pNext;
				pRenderData->m_pNext = pCur;
			}
			else
			{
				pCur = d3d_CreateRenderTexture(pTexture, iStage, LTFALSE);
				if (!pCur)
					return LTFALSE;
			}
		}

		d3d_SetTextureDirect(pCur);
	}

	if (pCur->m_LOD != iLOD)
	{
		pCur->m_pSurface->SetLOD(iLOD);
		pCur->m_LOD = (uint16)iLOD;
	}

	return LTTRUE;
}

// Take whatever texture is on a stage off it.
inline void d3d_DisableTexture(uint32 iStage)
{
	if (g_pCurTextures[iStage])
	{
		g_pD3DDevice->SetTexture(iStage, LTNULL);
		g_pCurTextures[iStage] = LTNULL;
	}
}

// The object's transform: its rotation scaled, at its position.  A
// rotation's garbage components are reset to the identity's first.
inline void d3d_SetupTransformation(LTVector* pPos, LTRotation* pRot, LTVector* pScale, LTMatrix* pMat)
{
	if (pRot->m_Quat[0] > MAX_ROTATION_COMPONENT || pRot->m_Quat[0] < -MAX_ROTATION_COMPONENT)
		pRot->m_Quat[0] = 0.0f;
	if (pRot->m_Quat[1] > MAX_ROTATION_COMPONENT || pRot->m_Quat[1] < -MAX_ROTATION_COMPONENT)
		pRot->m_Quat[1] = 0.0f;
	if (pRot->m_Quat[2] > MAX_ROTATION_COMPONENT || pRot->m_Quat[2] < -MAX_ROTATION_COMPONENT)
		pRot->m_Quat[2] = 0.0f;
	if (pRot->m_Quat[3] > MAX_ROTATION_COMPONENT || pRot->m_Quat[3] < -MAX_ROTATION_COMPONENT)
		pRot->m_Quat[3] = 1.0f;

	d3d_RotationToMatrix(pRot, pMat);

	pMat->m[0][0] *= pScale->x;
	pMat->m[1][0] *= pScale->x;
	pMat->m[2][0] *= pScale->x;
	pMat->m[0][1] *= pScale->y;
	pMat->m[1][1] *= pScale->y;
	pMat->m[2][1] *= pScale->y;
	pMat->m[0][2] *= pScale->z;
	pMat->m[1][2] *= pScale->z;
	pMat->m[2][2] *= pScale->z;

	pMat->m[0][3] = pPos->x;
	pMat->m[1][3] = pPos->y;
	pMat->m[2][3] = pPos->z;
}

// Whether a sphere is in the frustum; the planes it straddles are left set
// in *pClipFlags (drawmodel.cpp's d3d_ClipSphere).
inline LTBOOL d3d_ClipSphere(LTVector* pPos, float fRadius, LTPlane* pPlanes, uint32* pClipFlags)
{
	float fDist;
	int i;

	*pClipFlags = 0x3f;
	for (i=0; i < 6; i++)
	{
		fDist = pPlanes[i].DistTo(*pPos);
		if (fDist < -fRadius)
			return LTFALSE;

		if (fDist > fRadius)
			*pClipFlags &= ~(1 << i);
	}

	return LTTRUE;
}

// One grid vertex: its position up from the grid's plane, its colour and
// its texture coordinates.
inline void d3d_SetupPolyGridVert(PolyGridVert* pVert, PGColor* pColor, float fHeight,
	LTVector& vPos, LTVector& vUp, float fU, float fV, LTBOOL bEnvMap)
{
	pVert->m_Vec = vPos + vUp * fHeight;

	((uint8*)&pVert->m_Color)[2] = g_LightTableR[(uint8)(int)pColor->x];
	((uint8*)&pVert->m_Color)[1] = g_LightTableG[(uint8)(int)pColor->y];
	((uint8*)&pVert->m_Color)[0] = g_LightTableB[(uint8)(int)pColor->z];
	((uint8*)&pVert->m_Color)[3] = (uint8)(int)(g_fPolyGridAlpha * pColor->a);

	if (bEnvMap)
	{
		pVert->m_TU2 = (fU + g_fPolyGridUOffset) * g_fPolyGridUScale;
		pVert->m_TV2 = (fV + g_fPolyGridVOffset) * g_fPolyGridVScale;
	}
	else
	{
		pVert->m_TU1 = (fU + g_fPolyGridUOffset) * g_fPolyGridUScale;
		pVert->m_TV1 = (fV + g_fPolyGridVOffset) * g_fPolyGridVScale;
	}
}

void d3d_DrawPolyGrid(ViewParams* pContext, LTObject* pObject)
{
	uint32 srcBlend, destBlend, dwFog, dwFogColor;
	uint32 nClipFlags, nOutMask, nStage, nVerts, x, y, t, i;
	uint32 nTrisPerRow, nIndicesPerRow;
	LTBOOL bEnvMap, bClip;
	SharedTexture* pTex;
	PolyGridVert *pVerts, *pVert;
	ClipVert2 tri[3];
	ClipVert2 *pClipVerts, *pClipVert;
	int32 nClipVerts;
	uint16 *pRowIndices, *pIndices, *pOut;
	LTVector vDims, vScale, vOrigin, vRight, vUp, vForward, vRowStart, vPos;
	LTMatrix mTransform, mCentre, mWorld, mFull;
	float fRadius, fU, fV;

	// Set the correct blend states.
	d3d_GetBlendStates(pObject, srcBlend, destBlend, dwFog, dwFogColor);
	StateSet ssSrcBlend(D3DRENDERSTATE_SRCBLEND, srcBlend);
	StateSet ssDestBlend(D3DRENDERSTATE_DESTBLEND, destBlend);
	StateSet ssFog(D3DRENDERSTATE_FOGENABLE, dwFog);
	StateSet ssFogColor(D3DRENDERSTATE_FOGCOLOR, dwFogColor);

	if (!pObject->m_Data)
		return;

	// Is it in the frustum?
	vDims.x = ((float)pObject->m_Width * 0.5f) * pObject->m_Scale.x;
	vDims.y = pObject->m_Scale.y * 128.0f;
	vDims.z = ((float)pObject->m_Height * 0.5f) * pObject->m_Scale.z;
	fRadius = vDims.Mag() + 1.0f;

	if (!d3d_ClipSphere(&pObject->m_Pos, fRadius, pContext->m_ClipPlanes, &nClipFlags))
		return;

	// Its texture, and the texture's environment map.
	bEnvMap = LTFALSE;
	if (pObject->m_pSprite && pObject->m_SpriteTracker.m_pCurFrame &&
		(pTex = pObject->m_SpriteTracker.m_pCurFrame->m_pTex) != LTNULL)
	{
		if (g_100638c4 && g_pCurTextures[0] && pTex->m_pDetailTexture && pTex->m_DetailTextureMode &&
			g_CV_EnvMapPolyGrids.m_nValue)
		{
			bEnvMap = d3d_SetTexture(pTex->m_pDetailTexture, 0, 0) != 0;
		}

		if (d3d_SetTexture(pObject->m_SpriteTracker.m_pCurFrame->m_pTex, bEnvMap != 0, 0))
		{
			g_SavedStates.Restore();
			if (pObject->m_SpriteTracker.m_pCurFrame->m_pTex->m_pStateList)
				g_SavedStates.Apply(pObject->m_SpriteTracker.m_pCurFrame->m_pTex->m_pStateList, bEnvMap != 0);
		}
		else
		{
			d3d_DisableTexture(g_iCurTextureStage);
			bEnvMap = LTFALSE;
		}
	}
	else
	{
		d3d_DisableTexture(g_iCurTextureStage);
		bEnvMap = LTFALSE;
	}

	nStage = bEnvMap ? 1 : 0;

	// The texture coordinates' offsets and scales.
	g_fPolyGridUOffset = pObject->m_xPan * g_TexelSize[nStage].u;
	g_fPolyGridVOffset = pObject->m_yPan * g_TexelSize[nStage].v;
	g_fPolyGridUScale = 1.0f / pObject->m_xScale * pObject->m_Scale.x * g_TexelSize[nStage].u;
	g_fPolyGridVScale = 1.0f / pObject->m_yScale * g_TexelSize[nStage].v * pObject->m_Scale.z;
	g_fPolyGridUScale = (float)pObject->m_Width / (int32)(pObject->m_Width - 1) * g_fPolyGridUScale;
	g_fPolyGridVScale = (float)pObject->m_Height / (int32)(pObject->m_Height - 1) * g_fPolyGridVScale;
	g_fPolyGridAlpha = (float)pObject->m_ColorA * (1.0f / 255.0f);

	pObject->m_Flags |= PH_FLAG_INTERNAL;
	bClip = (nClipFlags & 0x3f) != 0;

	// The grid's transform: cell space, centred, to the world.
	vScale.x = (float)(pObject->m_Width + 1) * pObject->m_Scale.x / (int32)pObject->m_Width;
	vScale.y = pObject->m_Scale.y;
	vScale.z = (float)(pObject->m_Height + 1) * pObject->m_Scale.z / (int32)pObject->m_Height;
	d3d_SetupTransformation(&pObject->m_Pos, &pObject->m_Rotation, &vScale, &mTransform);

	mCentre.Identity();
	mCentre.m[0][3] = -((float)(pObject->m_Width - 1) * 0.5f);
	mCentre.m[2][3] = -((float)(pObject->m_Height - 1) * 0.5f);
	if (pObject->m_Flags & PG_UNSIGNED)
		mCentre.m[1][3] = -128.0f;

	MatMul(&mWorld, &mTransform, &mCentre);
	mWorld.GetTranslation(vOrigin);
	mWorld.GetBasisVectors(&vRight, &vUp, &vForward);

	// Room for the vertices.
	nVerts = pObject->m_Width * pObject->m_Height;
	if (nVerts > g_nPolyGridVerts)
	{
		dfree(g_pPolyGridVerts);
		g_pPolyGridVerts = (PolyGridVert*)dalloc(nVerts * sizeof(PolyGridVert));
		if (!g_pPolyGridVerts)
		{
			g_nPolyGridVerts = 0;
			return;
		}

		g_nPolyGridVerts = nVerts;
	}

	pVerts = g_pPolyGridVerts;

	// The vertices, a row at a time.
	vRowStart = vOrigin;
	pVert = pVerts;
	fV = 0.0f;
	if (pObject->m_Flags & PG_UNSIGNED)
	{
		uint8 *pRow, *pHeight;

		pRow = (uint8*)pObject->m_Data;
		for (y=pObject->m_Height; y; y--)
		{
			vPos = vRowStart;
			fU = 0.0f;
			pHeight = pRow;
			// RETAIL BUG: unlike the signed grids', these vertices' specular
			// colour is never set (the vertex buffer's is whatever dalloc
			// left there).
			for (x=pObject->m_Width; x; x--)
			{
				d3d_SetupPolyGridVert(pVert, &pObject->m_ColorTable[*pHeight], (float)*pHeight,
					vPos, vUp, fU, fV, bEnvMap);

				pHeight++;
				pVert++;
				vPos += vRight;
				fU += 1.0f;
			}

			vRowStart += vForward;
			pRow += pObject->m_Width;
			fV += 1.0f;
		}
	}
	else
	{
		int8 *pRow, *pHeight;

		pRow = pObject->m_Data;
		for (y=pObject->m_Height; y; y--)
		{
			vPos = vRowStart;
			fU = 0.0f;
			pHeight = pRow;
			for (x=pObject->m_Width; x; x--)
			{
				pVert->m_Specular = 0xFFFFFFFF;
				d3d_SetupPolyGridVert(pVert, &pObject->m_ColorTable[*pHeight + 128], (float)*pHeight,
					vPos, vUp, fU, fV, bEnvMap);

				pHeight++;
				pVert++;
				vPos += vRight;
				fU += 1.0f;
			}

			vRowStart += vForward;
			pRow += pObject->m_Width;
			fV += 1.0f;
		}
	}

	if (bEnvMap)
	{
		d3d_SetupPolyGridEnvMap(pObject, &vUp, &vRight, &vForward);
		d3d_SetDetailTextureStage(pObject->m_SpriteTracker.m_pCurFrame->m_pTex->m_DetailTextureMode);
	}

	if (bClip)
	{
		// Room for the indices of the triangles that need no clipping.
		// RETAIL BUG: the old index buffer is never freed when it grows.
		if (pObject->m_nIndices > g_nPolyGridIndices)
		{
			g_pPolyGridIndices = (uint16*)dalloc(pObject->m_nIndices * sizeof(uint16));
			if (!g_pPolyGridIndices)
			{
				g_nPolyGridIndices = 0;
				return;
			}

			g_nPolyGridIndices = pObject->m_nIndices;
		}

		// Into camera space.
		pVert = pVerts;
		for (i=nVerts; i; i--)
		{
			MatVMul_InPlace_H(&pContext->m_mWorldToCamera, &pVert->m_Vec);
			pVert++;
		}

		// Clip and draw the triangles the frustum cuts; keep the others.
		nTrisPerRow = pObject->m_Width * 2 - 2;
		nIndicesPerRow = nTrisPerRow * 3;
		pRowIndices = pObject->m_Indices;
		pOut = g_pPolyGridIndices;
		for (y=pObject->m_Height - 1; y; y--)
		{
			pIndices = pRowIndices;
			for (t=nTrisPerRow; t; t--)
			{
				switch (d3d_ClassifyTriangle(nClipFlags, &nOutMask,
					(ClipVert*)&pVerts[pIndices[0]], (ClipVert*)&pVerts[pIndices[1]],
					(ClipVert*)&pVerts[pIndices[2]]))
				{
					case 0:
					{
						*pOut++ = pIndices[0];
						*pOut++ = pIndices[1];
						*pOut++ = pIndices[2];
						break;
					}

					case 1:
						break;

					default:
					{
						tri[0] = *(ClipVert2*)&pVerts[pIndices[0]];
						tri[1] = *(ClipVert2*)&pVerts[pIndices[1]];
						tri[2] = *(ClipVert2*)&pVerts[pIndices[2]];

						pClipVerts = tri;
						nClipVerts = 3;
						if (ClipPoly(nOutMask, &pClipVerts, &nClipVerts))
						{
							pClipVert = pClipVerts;
							for (i=nClipVerts; i; i--)
							{
								d3d_ProjectTLVert((TLVertex*)pClipVert, &g_ViewParams);
								pClipVert++;
							}

							g_pD3DDevice->DrawPrimitive(D3DPT_TRIANGLEFAN, D3DFVF_XYZRHW|D3DFVF_DIFFUSE|D3DFVF_SPECULAR|D3DFVF_TEX2,
								pClipVerts, nClipVerts, 0);
						}
						break;
					}
				}

				pIndices += 3;
			}

			pRowIndices += nIndicesPerRow;
		}

		if (pOut - g_pPolyGridIndices > 0)
		{
			d3d_TransformPolyGridVerts(pVerts, nVerts);
			g_pD3DDevice->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, D3DFVF_XYZRHW|D3DFVF_DIFFUSE|D3DFVF_SPECULAR|D3DFVF_TEX2,
				pVerts, nVerts, g_pPolyGridIndices, pOut - g_pPolyGridIndices, 0);
		}
	}
	else
	{
		// Nothing to clip: project them all and draw the grid in one go.
		mFull = pContext->m_mProjection * pContext->m_mWorldToCamera;

		pVert = pVerts;
		for (i=nVerts; i; i--)
		{
			pVert->m_RHW = MatVMul_InPlace_H(&mFull, &pVert->m_Vec);
			pVert++;
		}

		if (pObject->m_nIndices)
		{
			g_pD3DDevice->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, D3DFVF_XYZRHW|D3DFVF_DIFFUSE|D3DFVF_SPECULAR|D3DFVF_TEX2,
				pVerts, nVerts, pObject->m_Indices, pObject->m_nIndices, 0);
		}
	}

	if (bEnvMap)
		d3d_UnsetDetailTextureStage();
}


// ---------------------------------------------------------------------- //
//  0x1002d3d0 -- which of the frustum planes a triangle is wholly
//  outside (1), straddles (2, with the planes still to clip in
//  *pOutMask) or is inside (0).
// ---------------------------------------------------------------------- //
// (d3d_clip.h's ClipVert: only its position is read.)

#define CLIP_NEAR(v)	((v)->m_vPos.z >= g_ViewParams.m_NearZ)
#define CLIP_LEFT(v)	(-(v)->m_vPos.z < (v)->m_vPos.x)
#define CLIP_TOP(v)		((v)->m_vPos.y < (v)->m_vPos.z)
#define CLIP_RIGHT(v)	((v)->m_vPos.x < (v)->m_vPos.z)
#define CLIP_BOTTOM(v)	(-(v)->m_vPos.z < (v)->m_vPos.y)
#define CLIP_FAR(v)		((v)->m_vPos.z <= g_ViewParams.m_FarZ2)

#define CLIP_TEST(bit, test, remaining)                                  \
	if (nClipMask & (bit))                                               \
	{                                                                    \
		nInside = test(pV0) + test(pV1) + test(pV2);                     \
		if (nInside == 0)                                                \
			return 1;                                                    \
		if (nInside != 3)                                                \
		{                                                                \
			*pOutMask = nClipMask & (remaining);                         \
			return 2;                                                    \
		}                                                                \
	}

int d3d_ClassifyTriangle(uint32 nClipMask, uint32* pOutMask,
                         ClipVert* pV0, ClipVert* pV1, ClipVert* pV2)
{
	int nInside;

	CLIP_TEST(0x04, CLIP_LEFT,   0x3f)
	CLIP_TEST(0x10, CLIP_RIGHT,  0x3b)
	CLIP_TEST(0x08, CLIP_TOP,    0x2b)
	CLIP_TEST(0x20, CLIP_BOTTOM, 0x23)
	CLIP_TEST(0x01, CLIP_NEAR,   0x03)
	CLIP_TEST(0x02, CLIP_FAR,    0x02)

	return 0;
}


// ---------------------------------------------------------------------- //
//  0x1002d6b0 -- the grid's normals, from its heights, and their
//  environment map coordinates.  Each vertex's height (signed, a byte)
//  gives two central differences, along the row and down the column (one
//  sided on the edges), which tilt pUp by pRight and pForward.
//
//  RETAIL BUG: the interior differences are previous minus next
//  (h[-W] - h[W], h[-1] - h[1]), and so is the last row's and column's
//  (h[-W] - h[0], h[-1] - h[0]), but the first row's and column's are next
//  minus this (h[W] - h[0], h[1] - h[0]): the wrong sign, so the edge
//  normals there tilt the wrong way.  AvP2 1.0.9.6 has the same code.
// ---------------------------------------------------------------------- //
void d3d_SetupPolyGridEnvMap(LTObject* pObject, LTVector* pUp, LTVector* pRight, LTVector* pForward)
{
	PolyGridVert *pRowV, *pV;
	int8 *pRowH, *pH;
	uint32 x, y;
	float fUp, fInvForward, fInvRight;
	LTVector vF, vR, vN;

	pRowV = g_pPolyGridVerts;
	pRowH = pObject->m_Data;

	fUp = pUp->MagSqr() / 128.0f;
	fInvForward = 1.0f / pForward->Mag();
	fInvRight = 1.0f / pRight->Mag();

	y = pObject->m_Height;
	while (y--)
	{
		pV = pRowV;
		pH = pRowH;

		x = pObject->m_Width;
		while (x--)
		{
			if (y < pObject->m_Height - 1)
			{
				if (y > 0)
					vF = *pForward * ((pH[-(int32)pObject->m_Width] - pH[pObject->m_Width]) * fInvForward * fUp * 0.5f);
				else
					vF = *pForward * ((pH[-(int32)pObject->m_Width] - pH[0]) * fInvForward * fUp * 0.5f);
			}
			else
			{
				vF = *pForward * ((pH[pObject->m_Width] - pH[0]) * fInvForward * fUp * 0.5f);
			}

			if (x < pObject->m_Width - 1)
			{
				if (x > 0)
					vR = *pRight * ((pH[-1] - pH[1]) * fInvRight * fUp * 0.5f);
				else
					vR = *pRight * ((pH[-1] - pH[0]) * fInvRight * fUp * 0.5f);
			}
			else
			{
				vR = *pRight * ((pH[1] - pH[0]) * fInvRight * fUp * 0.5f);
			}

			vN = vF + (vR + *pUp);
			vN.Norm();

			d3d_CalcEnvMapUV(&g_ViewParams.m_Pos, &pV->m_Vec, &vN, &pV->m_TU1, &pV->m_TV1);

			pV++;
			pH++;
		}

		pRowV += pObject->m_Width;
		pRowH += pObject->m_Width;
	}
}


// ---------------------------------------------------------------------- //
//  0x1002daf0 -- the OT_POLYGRID ProcessObject handler.
// ---------------------------------------------------------------------- //
void d3d_ProcessPolyGrid(LTObject *pObject)
{
	d3d_GetVisibleSet()->m_SolidPolyGrids.Add(pObject);
}


class Tester_PolyGrid
{
public:
	static bool Filter(LTObject *pObj) {
		return pObj->m_ColorA != 0xFF; }
};

// ---------------------------------------------------------------------- //
//  0x1002db40 -- draw the opaque ones now, set the rest aside.
// ---------------------------------------------------------------------- //
void d3d_DrawSolidPolyGrids()
{
	if (g_CV_DrawPolyGrids)
	{
		VisibleSet* pSet = d3d_GetVisibleSet();
		Tester_PolyGrid tester;

		pSet->m_SolidPolyGrids.DrawFiltered(&g_ViewParams, d3d_DrawPolyGrid, &tester,
			&pSet->m_TranslucentPolyGrids);
	}
}


static void d3d_QueuePolyGrid(ViewParams* pContext, LTObject* pObject);

// ---------------------------------------------------------------------- //
//  0x1002dc20
// ---------------------------------------------------------------------- //
void d3d_DrawTranslucentPolyGrids()
{
	if (g_CV_DrawPolyGrids)
		d3d_GetVisibleSet()->m_TranslucentPolyGrids.Draw(&g_ViewParams, d3d_QueuePolyGrid);
}

// 0x1002dc50
static void d3d_QueuePolyGrid(ViewParams* pContext, LTObject* pObject)
{
	g_pObjectDrawList->Add(pObject, d3d_DrawPolyGrid);
}


// ---------------------------------------------------------------------- //
//  0x1002dc70 / 0x1002dc90 -- the module's init and term: the shared
//  vertex and index buffers.
// ---------------------------------------------------------------------- //
void d3d_PolyGridModuleInit()
{
	g_pPolyGridVerts = LTNULL;
	g_nPolyGridVerts = 0;
	g_pPolyGridIndices = LTNULL;
	g_nPolyGridIndices = 0;
}

void d3d_PolyGridModuleTerm()
{
	dfree(g_pPolyGridVerts);
	dfree(g_pPolyGridIndices);
	g_pPolyGridVerts = LTNULL;
	g_nPolyGridVerts = 0;
	g_pPolyGridIndices = LTNULL;
	g_nPolyGridIndices = 0;
}


// ---------------------------------------------------------------------- //
//  0x1002dcd0 -- project polygrid vertices (0x28 bytes apart) in place.
// ---------------------------------------------------------------------- //
void d3d_TransformPolyGridVerts(PolyGridVert* pVerts, uint32 nVerts)
{
	for (; nVerts; nVerts--)
	{
		pVerts->m_RHW = MatVMul_InPlace_H(&g_ViewParams.m_mProjection, &pVerts->m_Vec);
		pVerts++;
	}
}
