// *********************************************************************** //
//  ph/d3d_skyobjects.cpp -- Primal Hunt's d3d.ren, TU16
//  (0x10019f60..0x1001a71a), /O1.
//
//  The sky's objects: the world models, polygrids and sprites the scene
//  lists as sky objects, drawn with the sky's view (TU33's g_SkyContext)
//  before the world.  A sky world model is drawn by walking its BSP back
//  to front and drawing each visible polygon through the clip vertex
//  pipeline, lit by the dynamic lights touching it.
//
//  The object's .bss (0x1005fe68..) is the sky world model's colour and
//  the static vertex array of the polygon draw (0x10060278, 128 vertices,
//  guard 0x10060274).  The render-state list's destructor and its two
//  vectors' destructors are instantiated here first (0x1001a4cc..
//  0x1001a532); the rest of its members are TU21's (d3d_statelist.cpp).
//
//  Built with STLport 4.0 (src/LT2/lithshared/stl), as TU21 is, /O1 /Ob2:
//  the initialiser is one body (a lone `ret`, 0x10019f60) and the 32-byte
//  helpers stay calls.
//
//  Near:
//    * 0x10019f61: retail's three vertex loops count down a copy of the
//      vertex count (VC6's loop reversal); ours keep the index, because the
//      count's address goes to the clipper and VC6 reloads it.  /Oa
//      reverses the light loop below but not these, and costs 0x1001a16e.
//    * 0x1001a2a1: retail expands CRenderStateList's constructor and calls
//      the two vectors' constructors (0x100195c0); ours calls the class's
//      constructor (the /Ob1 one-level constructor gate, guide 09).
//  0x1001a533 (d3d_LightSkyPoly) is identical: its vertex loop counts
//  down (`i > 0`, the unsigned `jbe`) and the distance goes through a
//  VEC_SUB temporary, which AvP2's cl 8168 needs to call MagSqr as retail.
// *********************************************************************** //

#include <d3d.h>
#include <vector>

#include <math.h>

#include "ltbasedefs.h"


// Round to nearest in the current FPU mode (LithTech's RoundFloatToInt).
inline int32 RoundFloatToInt(float f)
{
	int32 n;
	__asm fld f
	__asm fistp n
	return n;
}


// ---------------------------------------------------------------------- //
//  The render-state list (TU21's members).
// ---------------------------------------------------------------------- //
struct RenderState
{
	D3DRENDERSTATETYPE			m_State;                // +0x00
	uint32						m_Value;                // +0x04
};

struct StageState
{
	uint32						m_Stage;                // +0x00
	D3DTEXTURESTAGESTATETYPE	m_State;                // +0x04
	uint32						m_Value;                // +0x08
};

class CRenderStateList
{
public:
						~CRenderStateList() { Restore(); }  // 0x1001a4cc

	void				Restore();                      // 0x10022a86
	void				Apply(CRenderStateList *pList, uint32 iStage);  // 0x10022a97
	void				SetStageState(StageState *pState, uint32 iStage);  // 0x10022add
	void				SetState(RenderState *pState);  // 0x10022b08

	std::vector<RenderState>	m_States;               // +0x00
	std::vector<StageState>		m_StageStates;          // +0x0c
};

extern CRenderStateList		g_SavedStates;              // 0x10069728


// ---------------------------------------------------------------------- //
//  The structures, as this file reads them.
// ---------------------------------------------------------------------- //
struct TLVertex
{
	LTVector	m_Vec;                                  // +0x00
	float		rhw;                                    // +0x0c
	union
	{
		uint32	color;                                  // +0x10
		struct { uint8 b, g, r, a; } rgba;
	};
	uint32		specular;                               // +0x14
	float		tu, tv;                                 // +0x18

	void		SetTexCoords(float u, float v) { tu = u; tv = v; }
};

// The polygon draw's vertices: their destructor does nothing.
#define MAX_SKYPOLY_VERTS	128

struct SkyPolyVerts
{
	~SkyPolyVerts() {}

	TLVertex	m_Verts[MAX_SKYPOLY_VERTS];
};

struct RenderVertex
{
	LTVector	m_Vec;                                  // +0x00
};

struct RenderPolyVert
{
	RenderVertex*	m_pVert;                            // +0x00
	float			tu, tv;                             // +0x04
	uint8			m_pad_0c[0x14 - 0x0c];
	uint8			b, g, r, a;                         // +0x14
};

struct RenderPlane
{
	float		DistTo(LTVector &vec) {return (m_Normal.x*vec.x + m_Normal.y*vec.y + m_Normal.z*vec.z) - m_Dist;}

	LTVector	m_Normal;                               // +0x00
	float		m_Dist;                                 // +0x0c
};

class CRenderStateList;

struct SharedTexture
{
	uint8				m_pad_00[0x34];
	CRenderStateList*	m_pStates;                      // +0x34
};

#define SURF_TRANSLUCENT	0x8    // Primal Hunt's

struct RenderSurface
{
	uint8			m_pad_00[0x2c];
	SharedTexture*	m_pTexture;                         // +0x2c
	uint32			m_Flags;                            // +0x30
};

class LTObject;

// A dynamic light touching a polygon, and where it is.
struct LightLink
{
	LightLink*	m_pNext;                                // +0x00
	LTObject*	m_pLight;                               // +0x04
	LTVector	m_vPos;                                 // +0x08
};

struct RenderPoly
{
	uint8			m_pad_00[0x28];
	RenderPlane*	m_pPlane;                           // +0x28
	RenderSurface*	m_pSurface;                         // +0x2c
	LightLink*		m_pLights;                          // +0x30
	uint8			m_pad_34[0x50 - 0x34];
	RenderPolyVert*	m_pTJuncVerts;                      // +0x50
	uint16			m_nVerts;                           // +0x54
	uint16			m_nTJuncVerts;                      // +0x56
	RenderPolyVert	m_Verts[1];                         // +0x58
};

#define NFI_NODE_IN		1
#define NFI_NODE_OUT	2

struct RenderNode
{
	RenderPoly*		m_pPoly;                            // +0x00
	RenderNode*		m_Sides[2];                         // +0x04
	uint8			m_pad_0c[0x16 - 0x0c];
	uint8			m_Flags;                            // +0x16
};

struct WorldBsp
{
	uint8			m_pad_00[0x9c];
	RenderNode*		m_pRootNode;                        // +0x9c
	RenderPoly**	m_Polies;                           // +0xa0
	uint32			m_nPolies;                          // +0xa4
};

#define SURF_ADDITIVE_SHIFT	19

class LTObject
{
public:
	virtual void	Slot00();
	virtual void	Slot04();
	virtual void	Slot08();
	virtual void	Slot0c();
	virtual void	Slot10();
	virtual void	Slot14();
	virtual void	Slot18();
	virtual void	Slot1c();
	virtual void	Slot20();
	virtual void	Slot24();
	virtual void	Slot28();
	virtual void	Slot2c();
	virtual void	Slot30();
	virtual void	Slot34();
	virtual LTBOOL	IsMainWorldModel();                 // +0x38

	uint8		m_pad_004[0x88 - 0x04];
	uint32		m_Flags;                                // +0x088
	uint8		m_pad_08c[0x94 - 0x8c];
	uint8		m_ColorR;                               // +0x094
	uint8		m_ColorG;                               // +0x095
	uint8		m_ColorB;                               // +0x096
	uint8		m_ColorA;                               // +0x097
	uint8		m_pad_098[0xbc - 0x98];
	char		m_ObjectType;                           // +0x0bc
	uint8		m_pad_0bd[0x1b0 - 0xbd];
	union
	{
		float		m_LightRadius;                      // +0x1b0 (lights)
		WorldBsp*	m_pWorldBsp;                        // +0x1b0 (world models)
	};
};

struct FrameCounters
{
	uint8		m_pad_00[0xb4];
	LTObject**	m_SkyObjects;                           // +0xb4
	int			m_nSkyObjects;                          // +0xb8
};

// The sky's view (TU33's).
struct ViewParams
{
	uint8		m_pad_000[0x15c];
	LTMatrix	m_mWorldToCamera;                       // +0x15c
	uint8		m_pad_19c[0x4b4 - 0x19c];
	LTVector	m_vPos;                                 // +0x4b4
};

extern LPDIRECT3DDEVICE7	g_pD3DDevice;               // 0x100638c8
extern uint32		g_iCurTextureStage;                 // 0x100638c0
extern float		g_fTexelSizeU;                      // 0x100672a8
extern float		g_fTexelSizeV;                      // 0x100672ac
extern uint8		g_VertexAlpha;                      // 0x1005d20c
extern int32		g_CV_FixTJunc;                      // 0x1005dbb4
extern int32		g_CV_ShowSkySplits;                 // 0x1005dad0
extern int32		g_CV_Saturate;                      // 0x1005d384
extern float		g_CV_SkyFogNearZ;                   // 0x1005d8b8
extern float		g_CV_SkyFogFarZ;                    // 0x1005d7d8
extern uint32		g_nSkyLightsApplied;                // 0x1005c164
extern LTVector		g_vGlobalLightColor;                // 0x1005b780
extern FrameCounters*	g_pFrameCounters;               // 0x1005c14c
extern ViewParams	g_SkyContext;                       // 0x10079450
extern void			(__fastcall *g_pfnCalcSkyFog)(const LTVector* pPos, uint32* pSpecular);  // 0x1005e6d8 (d3d_draw.cpp)

extern void		dd_ConsolePrint(const char* pMsg, ...);  // 0x10013ade
extern void		d3d_TransformToCamera(TLVertex* pVert, LTMatrix* pMat);  // 0x10008719
extern LTBOOL	d3d_ClipTLVertsRC(uint32 clipFlags, TLVertex** ppVerts, int* pnVerts);  // 0x1000af61
extern void		d3d_ProjectTLVert(TLVertex* pVert, ViewParams* pContext);  // 0x1002739d
extern int		d3d_SetTexture(SharedTexture* pTexture, uint32 iStage, uint32 flags);  // 0x100079e4
extern void		d3d_DisableTexture(uint32 iStage);      // 0x1000a22b
extern void		d3d_SetTranslucentObjectStates(LTBOOL bAdditive);  // 0x10014620
extern void		d3d_UnsetTranslucentObjectStates(LTBOOL bChangeZ);  // 0x10014870
extern void		d3d_DrawPolyGrid(ViewParams* pContext, LTObject* pObject);  // 0x1002be60
extern void		d3d_DrawSprite(ViewParams* pContext, LTObject* pObject);  // 0x1002e420

void		d3d_LightSkyPoly(RenderPoly* pPoly, TLVertex* pVerts, uint32 nVerts);


// ---------------------------------------------------------------------- //
//  Globals.
// ---------------------------------------------------------------------- //
LTVector	g_vSkyModelColor;                           // 0x1005fe68  $E 0x10019f60


// ---------------------------------------------------------------------- //
//  The sky world models.
// ---------------------------------------------------------------------- //

// Room for the polygon's vertices?
inline LTBOOL d3d_CheckVertexCount(int nVerts)
{
	if (nVerts > MAX_SKYPOLY_VERTS)
	{
		dd_ConsolePrint("Error: vertex buffer overflow");
		return FALSE;
	}

	return TRUE;
}

// 0x10019f61 -- draw one of a sky world model's polygons.
void d3d_DrawSkyPoly(RenderPoly* pPoly)
{
	static SkyPolyVerts verts;
	TLVertex *pVerts, *pVert;
	RenderPolyVert* pSrc;
	uint32 nVerts, i;
	SharedTexture* pTexture;

	if (g_CV_FixTJunc)
	{
		pSrc = pPoly->m_pTJuncVerts;
		nVerts = pPoly->m_nTJuncVerts;
	}
	else
	{
		nVerts = pPoly->m_nVerts;
		pSrc = pPoly->m_Verts;
	}

	if (!d3d_CheckVertexCount(nVerts))
		return;

	for (i = 0; i < nVerts; i++)
	{
		pVert = &verts.m_Verts[i];
		pVert->m_Vec = pSrc->m_pVert->m_Vec;
		pVert->rgba.r = (uint8)RoundFloatToInt((float)pSrc->r * g_vSkyModelColor.x);
		pVert->rgba.g = (uint8)RoundFloatToInt((float)pSrc->g * g_vSkyModelColor.y);
		pVert->rgba.b = (uint8)RoundFloatToInt((float)pSrc->b * g_vSkyModelColor.z);
		pVert->rgba.a = g_VertexAlpha;
		pVert->SetTexCoords(pSrc->tu, pSrc->tv);
		pSrc++;
	}

	pVerts = verts.m_Verts;
	d3d_LightSkyPoly(pPoly, pVerts, nVerts);

	for (i = 0; i < nVerts; i++)
		d3d_TransformToCamera(&pVerts[i], &g_SkyContext.m_mWorldToCamera);

	if (!d3d_ClipTLVertsRC(0x3F, &pVerts, (int*)&nVerts))
		return;

	if (g_CV_ShowSkySplits || !pPoly->m_pSurface->m_pTexture ||
		!d3d_SetTexture(pPoly->m_pSurface->m_pTexture, g_iCurTextureStage, 0))
	{
		d3d_DisableTexture(g_iCurTextureStage);
	}

	for (i = 0; i < nVerts; i++)
	{
		pVert = &pVerts[i];
		g_pfnCalcSkyFog(&pVert->m_Vec, &pVert->specular);
		d3d_ProjectTLVert(pVert, &g_SkyContext);
		pVert->tu *= g_fTexelSizeU;
		pVert->tv *= g_fTexelSizeV;
	}

	g_SavedStates.Restore();

	pTexture = pPoly->m_pSurface->m_pTexture;
	if (pTexture && pTexture->m_pStates)
		g_SavedStates.Apply(pTexture->m_pStates, 1);

	g_pD3DDevice->DrawPrimitive(D3DPT_TRIANGLEFAN, D3DFVF_TLVERTEX, pVerts, nVerts, 0);

	g_SavedStates.Restore();
}


// 0x1001a16e -- draw a sky world model: its BSP back to front, each
// polygon facing the sky's viewer.
void d3d_DrawSkyWorldModel(LTObject* pObject)
{
	RenderNode* stack[512];
	RenderNode **pStack, *pNode;
	RenderPoly* pPoly;
	RenderPlane* pPlane;
	WorldBsp* pBsp;
	float fColorScale;
	int iSide;

	if (!pObject->IsMainWorldModel())
		return;

	fColorScale = 1.0f / 255.0f;
	g_vSkyModelColor.x = g_vGlobalLightColor.x * ((float)pObject->m_ColorR * fColorScale);
	g_vSkyModelColor.y = g_vGlobalLightColor.y * ((float)pObject->m_ColorG * fColorScale);
	g_vSkyModelColor.z = g_vGlobalLightColor.z * ((float)pObject->m_ColorB * fColorScale);

	pBsp = pObject->m_pWorldBsp;
	if (!pBsp->m_nPolies)
		return;

	pNode = pBsp->m_pRootNode;
	pStack = stack;
	for (;;)
	{
		while (pNode->m_Flags & (NFI_NODE_IN | NFI_NODE_OUT))
		{
			if (pStack == stack)
				return;

			pNode = *--pStack;
		}

		pPoly = pNode->m_pPoly;
		pPlane = (pPoly && pPoly->m_pSurface) ? pPoly->m_pPlane : LTNULL;

		iSide = (pPlane->m_Normal.Dot(g_SkyContext.m_vPos) - pPlane->m_Dist > 0.0f);
		if (iSide && !(pPoly->m_pSurface->m_Flags & SURF_INVISIBLE))
			d3d_DrawSkyPoly(pPoly);

		*pStack++ = pNode->m_Sides[iSide];
		pNode = pNode->m_Sides[!iSide];
	}
}


// A world model's first surface's flags.
inline uint32 d3d_GetWorldModelSurfaceFlags(LTObject* pObject)
{
	if (pObject->m_pWorldBsp && pObject->IsMainWorldModel() && pObject->m_pWorldBsp->m_nPolies > 0)
		return pObject->m_pWorldBsp->m_Polies[0]->m_pSurface->m_Flags;

	return 0;
}


// 0x1001a2a1 -- draw the sky objects (TU33's sky draw).
void d3d_DrawSkyObjects()
{
	CRenderStateList states;
	RenderState rs;
	uint32 oldFogEnable, oldColorOp;
	int i;
	LTObject* pObject;
	uint32 surfFlags;

	rs.m_State = D3DRENDERSTATE_ZWRITEENABLE;
	rs.m_Value = FALSE;
	states.SetState(&rs);

	rs.m_State = D3DRENDERSTATE_ZENABLE;
	rs.m_Value = FALSE;
	states.SetState(&rs);

	rs.m_State = D3DRENDERSTATE_FOGSTART;
	rs.m_Value = *((uint32*)&g_CV_SkyFogNearZ);
	states.SetState(&rs);

	rs.m_State = D3DRENDERSTATE_FOGEND;
	rs.m_Value = *((uint32*)&g_CV_SkyFogFarZ);
	states.SetState(&rs);

	{
		StageState ss;

		ss.m_Stage = 1;
		ss.m_State = D3DTSS_COLOROP;
		ss.m_Value = D3DTOP_DISABLE;
		states.SetStageState(&ss, 1);
	}

	g_pD3DDevice->GetRenderState(D3DRENDERSTATE_FOGENABLE, (unsigned long*)&oldFogEnable);
	g_pD3DDevice->GetTextureStageState(1, D3DTSS_COLOROP, (unsigned long*)&oldColorOp);

	for (i = 0; i < g_pFrameCounters->m_nSkyObjects; i++)
	{
		pObject = g_pFrameCounters->m_SkyObjects[i];
		if (!(pObject->m_Flags & FLAG_VISIBLE))
			continue;

		if (pObject->m_ObjectType == OT_WORLDMODEL)
		{
			if (pObject->m_Flags & FLAG_FOGDISABLE)
			{
				RenderState rsFog;

				rsFog.m_State = D3DRENDERSTATE_FOGENABLE;
				rsFog.m_Value = FALSE;
				states.SetState(&rsFog);
			}

			surfFlags = d3d_GetWorldModelSurfaceFlags(pObject);
			if (surfFlags & SURF_TRANSLUCENT)
			{
				d3d_SetTranslucentObjectStates((surfFlags >> SURF_ADDITIVE_SHIFT) & 1);
			}
			else
			{
				RenderState rsSrc, rsDest;

				rsSrc.m_State = D3DRENDERSTATE_SRCBLEND;
				rsSrc.m_Value = D3DBLEND_SRCALPHA;
				states.SetState(&rsSrc);

				rsDest.m_State = D3DRENDERSTATE_DESTBLEND;
				rsDest.m_Value = D3DBLEND_ONE;
				states.SetState(&rsDest);

				d3d_UnsetTranslucentObjectStates(FALSE);
			}

			g_VertexAlpha = pObject->m_ColorA;
			d3d_DrawSkyWorldModel(pObject);
		}
		else if (pObject->m_ObjectType == OT_POLYGRID)
		{
			if (pObject->m_ColorA != 0xFF)
				d3d_SetTranslucentObjectStates(FALSE);
			else
				d3d_UnsetTranslucentObjectStates(FALSE);

			d3d_DrawPolyGrid(&g_SkyContext, pObject);
		}
		else if (pObject->m_ObjectType == OT_SPRITE)
		{
			d3d_SetTranslucentObjectStates(FALSE);
			d3d_DrawSprite(&g_SkyContext, pObject);
		}
	}

	d3d_UnsetTranslucentObjectStates(FALSE);
}


// 0x1001a533 -- add the dynamic lights touching a sky polygon to its
// vertices' colours.
//
// RETAIL BUG: it lights pPoly->m_nVerts vertices and ignores nVerts, the
// count the caller filled (the T-junction vertex count when FixTJunc is
// set), so with FixTJunc some of the drawn vertices go unlit.
void d3d_LightSkyPoly(RenderPoly* pPoly, TLVertex* pVerts, uint32 nVerts)
{
	LightLink* pLink;
	LTObject* pLight;
	LTVector vLightPos, vDelta;
	RenderPlane* pPlane;
	float r, g, b, fDist, fRadiusSqr, fInvRadius, fDistSqr, fScale;
	int32 newR, newG, newB;
	uint32 i;
	TLVertex* pVert;

	for (pLink = pPoly->m_pLights; pLink; pLink = pLink->m_pNext)
	{
		pLight = pLink->m_pLight;
		vLightPos = pLink->m_vPos;

		r = (float)pLight->m_ColorR;
		g = (float)pLight->m_ColorG;
		b = (float)pLight->m_ColorB;
		r -= 255.0f - r;
		g -= 255.0f - g;
		b -= 255.0f - b;

		g_nSkyLightsApplied++;
		pPlane = pPoly->m_pPlane;
		fDist = pPlane->DistTo(vLightPos);
		if (fDist < 0.0f)
			fDist = -fDist;

		if (fDist >= pLight->m_LightRadius)
			continue;

		fRadiusSqr = pLight->m_LightRadius * pLight->m_LightRadius;
		fInvRadius = 1.0f / pLight->m_LightRadius;

		pVert = pVerts;
		for (i = pPoly->m_nVerts; i > 0; i--)
		{
			VEC_SUB(vDelta, pVert->m_Vec, vLightPos);
			fDistSqr = vDelta.MagSqr();
			if (fDistSqr < fRadiusSqr)
			{
				fDist = (float)sqrt(fDistSqr);
				fScale = 1.0f - fDist * fInvRadius;

				newR = (int32)(fScale * r) + pVert->rgba.r;
				newG = (int32)(fScale * g) + pVert->rgba.g;
				newB = (int32)(fScale * b) + pVert->rgba.b;

				if (g_CV_Saturate)
				{
					newR += newR;
					newG += newG;
					newB += newB;
				}

				if (newR > 255)
					newR = 255;
				else if (newR < 0)
					newR = 0;

				if (newG > 255)
					newG = 255;
				else if (newG < 0)
					newG = 0;

				if (newB > 255)
					newB = 255;
				else if (newB < 0)
					newB = 0;

				pVert->rgba.r = (uint8)newR;
				pVert->rgba.g = (uint8)newG;
				pVert->rgba.b = (uint8)newB;
			}

			pVert++;
		}
	}
}
