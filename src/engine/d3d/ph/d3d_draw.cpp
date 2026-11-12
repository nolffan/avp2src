// *********************************************************************** //
//
//  MODULE:   D3D_DRAW (.CPP) -- Primal Hunt's d3d.ren, TU15
//            (0x10014020..0x10019f51, docs/D3D_OBJECTS.md)
//
//  PURPOSE:  The world and frame drawing: the vertical fog and draw
//            console variables, the fog functions, the translucent state
//            blocks, the deferred polygon list, the clear, the draw mode
//            and FullDrawScene, the light add/scale passes, the world tree
//            debug boxes and the frame statistics.  The STLport vector and
//            node allocator members it instantiates first follow it
//            (0x10019260..0x10019f51).
//
//  Flags: /O2 /Ob2 (the initialisers are single bodies), and STLport 4.0
//  (lithshared/stl) ahead of VC6's headers: the state blocks are STLport
//  vectors with the node allocator.
//
//  0x10016030 (d3d_DrawPortalPoly, a portal's polygon blended through the
//  portal's transform) is near: the inline texture bind keeps no zero
//  register where retail keeps 0 in %ebx, the transform's terms come out
//  in another order, and the inliner expands ~vector<StageState> one level
//  where retail calls it (0x10019550: ~vector, not ~_Vector_base -- its
//  dead `mov %esi,(%esp)` is _Destroy's first argument).
//
//  0x100162a0 (d3d_DrawPortals: the scene again through each mirror) and
//  0x10018520 (d3d_RenderScene) are near, by the inliner's choices:
//  retail calls the TransformPlane and reflection helpers it uses out of
//  line.  AvP2's vertical fog (0x100135c0, d3d_CalcVFog) is near the same
//  way; Primal Hunt's fogvolume.cpp replaces it.
//  0x10016c60 (d3d_ClipLine) is near: the scheduling of the colour lerp's
//  byte loads, size-exact.  Near, all from VC6's inlining choices or scheduling
//  rather than a known source difference: 0x10014550 (register choice),
//  0x10014620 (which push_back sites expand), 0x100149c0 and 0x10014b80
//  (the vertex copy and the transform's operand order), 0x10015070,
//  0x10015760 (d3d_DrawPolyFlat's drawing spelt out, through the
//  out-of-line transform; size within 4 bytes), 0x10015270 and 0x10017bd0 (the
//  transform's term order), 0x10015400, 0x10017e60, 0x10018400 and
//  0x10019040 (register choice).
//
// *********************************************************************** //

#include <windows.h>
#include <ddraw.h>
#include <d3d.h>
#include <math.h>
#include <vector>
#include "ltbasedefs.h"
#include "../../LT2/lithshared/stdlith/struct_bank.h"


// ---------------------------------------------------------------------- //
//  The rest of the renderer, as this file sees it.
// ---------------------------------------------------------------------- //

// A console variable (common_stuff.cpp's registrar, 0x10011f36).
class CD3DConVar
{
public:
	CD3DConVar(const char* pName, float fDefault, int32* pnValue, float* pfValue);

	int32       m_nValue;                                   // +0x00
	float       m_fValue;                                   // +0x04
	float       m_fDefault;                                 // +0x08
	int32*      m_pnValue;                                  // +0x0c
	float*      m_pfValue;                                  // +0x10
	const char* m_pName;                                    // +0x14
	void*       m_hParam;                                   // +0x18
	CD3DConVar* m_pNext;                                    // +0x1c
};

// Round to nearest in the current FPU mode (LithTech's RoundFloatToInt).
inline int32 RoundFloatToInt(float f)
{
	int32 n;
	__asm fld f
	__asm fistp n
	return n;
}

// The console variables (common_stuff.cpp).
extern float    g_CV_FogNearZ;                              // 0x1005df38
extern float    g_CV_FogFarZ;                               // 0x1004a754
extern float    g_CV_SkyFogNearZ;                           // 0x1005d8b8
extern float    g_CV_SkyFogFarZ;                            // 0x1005d7d8
extern float    g_fFogScale;                                // 0x1005d428
extern int32    g_CV_UseDX6Commands;                        // 0x1004a740
class CGammaTable
{
public:
	CGammaTable();

	uint8   m_Table[256];
};

extern CGammaTable g_GammaTable;                            // 0x10087b00
extern void*    dalloc(uint32 size);                        // 0x10013a66
extern void     dfree(void* ptr);                           // 0x10013ac5

// The device (d3d_init.cpp).
extern LPDIRECT3DDEVICE7    g_pD3DDevice;                   // 0x100638c8

// The punch-through flag (drawworld.cpp).
void d3d_SetWorldFlags(uint32 nFlags);                      // 0x10009959


// ---------------------------------------------------------------------- //
//  The console variables.
// ---------------------------------------------------------------------- //
CD3DConVar  g_CV_VFogMinY("VFogMinY", 0.0f, LTNULL, LTNULL);                    // 0x1005fde0
CD3DConVar  g_CV_VFogMaxY("VFogMaxY", 1300.0f, LTNULL, LTNULL);                 // 0x1005fda8
CD3DConVar  g_CV_VFogMinYVal("VFogMinYVal", 0.5f, LTNULL, LTNULL);              // 0x1005e1d0
CD3DConVar  g_CV_VFogMaxYVal("VFogMaxYVal", 0.0f, LTNULL, LTNULL);              // 0x1005e078
CD3DConVar  g_CV_VFogMax("VFogMax", 180.0f, LTNULL, LTNULL);                    // 0x1005e778
CD3DConVar  g_CV_VFogDensity("VFogDensity", 1800.0f, LTNULL, LTNULL);           // 0x1005e098
CD3DConVar  g_CV_VFog("VFog", 0.0f, LTNULL, LTNULL);                            // 0x1005e698
CD3DConVar  g_CV_DrawWorldTree("DrawWorldTree", -1.0f, LTNULL, LTNULL);         // 0x1005e0c0
CD3DConVar  g_CV_DrawTerrainSections("DrawTerrainSections", 0.0f, LTNULL, LTNULL);  // 0x1005e238
CD3DConVar  g_CV_AlphaTest("AlphaTest", 1.0f, LTNULL, LTNULL);                  // 0x1005e258
CD3DConVar  g_CV_DrawPortals("DrawPortals", 1.0f, LTNULL, LTNULL);              // 0x1005e750
CD3DConVar  g_CV_ShowPortalBounds("ShowPortalBounds", 0.0f, LTNULL, LTNULL);    // 0x1005e6e0
CD3DConVar  g_CV_PortalLightmap("PortalLightmap", 1.0f, LTNULL, LTNULL);        // 0x1005e708
CD3DConVar  g_CV_ShowTexInfo("ShowTexInfo", 0.0f, LTNULL, LTNULL);              // 0x1005e058
CD3DConVar  g_CV_RenderToFront("RenderToFront", 0.0f, LTNULL, LTNULL);          // 0x1005e218

LTVector    g_vLightScaleLast(5.0f, 5.0f, 5.0f);                                // 0x1005fdd0
LTVector    g_vLightAddLast;                                                    // 0x10014220


// ---------------------------------------------------------------------- //
//  Fog.
// ---------------------------------------------------------------------- //
// The view (common_draw.cpp's), as this file reads it.
struct ViewParams
{
	uint8       m_Reserved000[0x1c];
	LTRect      m_Rect;                                     // +0x01c
	uint8       m_Reserved02c[0x88 - 0x2c];
	float       m_NearZ;                                    // +0x088
	float       m_FarZ;                                     // +0x08c
	float       m_FarZ2;                                    // +0x090 (the far clip)
	uint8       m_Reserved094[0xdc - 0x94];
	LTMatrix    m_mView;                                    // +0x0dc
	LTMatrix    m_mProjection;                              // +0x11c
	LTMatrix    m_mWorldToCamera;                           // +0x15c
	uint8       m_Reserved19c[0x1dc - 0x19c];
	LTMatrix    m_mFullTransform;                           // +0x1dc
	LTMatrix    m_mDeviceTransform;                         // +0x21c
	uint8       m_Reserved25c[0x2dc - 0x25c];
	LTMatrix    m_mReallyCloseToCamera;                     // +0x2dc
	float       m_fProjScaleX;                              // +0x31c (the projection, as
	float       m_fProjCenterX;                             // +0x320  the clipped polygons
	float       m_fProjScaleY;                              // +0x324  are projected)
	float       m_fProjCenterY;                             // +0x328
	float       m_fProjScaleZ;                              // +0x32c
	float       m_fProjOffsetZ;                             // +0x330
	LTVector    m_ViewPoints[5];                            // +0x334
	LTPlane     m_ViewPlanes[6];                            // +0x370
	LTPlane     m_ClipPlanes[6];                            // +0x3d0
	LTPlane     m_ReallyClosePlanes[6];                     // +0x430
	LTVector    m_vUp;                                      // +0x490
	LTVector    m_vRight;                                   // +0x49c
	LTVector    m_vForward;                                 // +0x4a8
	LTVector    m_Pos;                                      // +0x4b4
	LTVector    m_SkyViewPos;                               // +0x4c0
	LTBOOL      m_bMirrorView;                              // +0x4cc (through an odd number of mirrors)
	float       m_fMirrorSign;                              // +0x4d0
	LTBOOL      m_bInPortal;                                // +0x4d4
	LTVector    m_vPortalPos;                               // +0x4d8
	LTVector    m_vViewPos;                                 // +0x4e4
	float       m_fVFog;                                    // +0x4f0 (the camera's)
	uint32      m_nVFogMode;                                // +0x4f4 (the camera's)
};

extern ViewParams   g_ViewParams;                           // 0x1005b790
float           g_fSkyFogScale;                             // 0x1005e0b8

// 0x10014230 -- the fog of a point by its distance from the viewer, into
// the specular colour's alpha.
void __fastcall d3d_CalcFog(const LTVector* pPos, uint32* pSpecular)
{
	float fDist;

	fDist = (*pPos - g_ViewParams.m_vViewPos).Mag();
	if (fDist < g_CV_FogNearZ)
	{
		*pSpecular = 0xFFFFFFFF;
		return;
	}

	if (fDist > g_CV_FogFarZ)
	{
		*pSpecular = 0;
		return;
	}

	fDist = (fDist - g_CV_FogNearZ) * g_fFogScale;
	((uint8*)pSpecular)[3] = (uint8)(255 - RoundFloatToInt(fDist));
}

// 0x100142e0 -- the sky's fog of a point, by its view-space depth.
void __fastcall d3d_CalcSkyFog(const LTVector* pPos, uint32* pSpecular)
{
	float fDist;

	if (pPos->z < g_CV_SkyFogNearZ)
	{
		((uint8*)pSpecular)[3] = 0xFF;
		return;
	}

	if (pPos->z > g_CV_SkyFogFarZ)
	{
		((uint8*)pSpecular)[3] = 0;
		return;
	}

	fDist = (pPos->z - g_CV_SkyFogNearZ) * g_fSkyFogScale;
	((uint8*)pSpecular)[3] = (uint8)(255 - RoundFloatToInt(fDist));
}


// The vertical fog's scales (d3d_RenderScene sets them; d3d_viewparams.cpp
// reads the camera's).
float           g_fVFogRangeScale;                          // 0x1005df90
float           g_fVFogValScale;                            // 0x1005e210
float           g_fVFogDensityScale;                        // 0x1005e20c


#ifndef PRIMAL_HUNT
// ---------------------------------------------------------------------- //
//  AvP2's vertical fog (Primal Hunt's fog volumes replace it,
//  fogvolume.cpp).
// ---------------------------------------------------------------------- //

// Where a height is against the fog: 0 below it, 1 above it, 2 in it.
inline uint32 d3d_GetVFogMode(float y)
{
	if (y >= g_CV_VFogMaxY.m_fValue)
		return 1;
	else if (y <= g_CV_VFogMinY.m_fValue)
		return 0;
	else
		return 2;
}

// The fog's density at a height.
inline float d3d_GetVFogVal(float y)
{
	if (y <= g_CV_VFogMinY.m_fValue)
		return g_CV_VFogMinYVal.m_fValue;
	else if (y >= g_CV_VFogMaxY.m_fValue)
		return g_CV_VFogMaxYVal.m_fValue;
	else
		return (y - g_CV_VFogMinY.m_fValue) * g_fVFogRangeScale * g_fVFogValScale + g_CV_VFogMinYVal.m_fValue;
}

// 0x100135c0 -- the vertical fog between the camera and a point: the
// distance through each layer by the mean of the densities at its ends,
// into the specular colour's alpha.
//
// RETAIL BUG: where the segment leaves the camera's layer it is cut at
// VFogMaxY, even when it crosses VFogMinY (camera below the fog, the
// point in it, or the other way round), and the top layer's density is
// taken as VFogMaxYVal at the cut.
void __fastcall d3d_CalcVFog(const LTVector* pPos, uint32* pSpecular)
{
	LTVector vCamera = g_ViewParams.m_Pos;
	LTVector vPos = *pPos;
	float fFog;

	uint32 nMode = d3d_GetVFogMode(pPos->y);
	if (g_ViewParams.m_nVFogMode == nMode)
	{
		if (nMode == 2)
		{
			float fPosFog = (vPos.y - g_CV_VFogMinY.m_fValue) * g_fVFogRangeScale * g_fVFogValScale +
				g_ViewParams.m_fVFog + g_CV_VFogMinYVal.m_fValue;
			fFog = (vCamera - vPos).Mag() * fPosFog * g_fVFogDensityScale;
		}
		else
		{
			float fPosFog = g_ViewParams.m_fVFog;
			fFog = (vCamera - vPos).Mag() * fPosFog * 2.0f * g_fVFogDensityScale;
		}
	}
	else
	{
		LTVector vHit;
		float t;

		t = (g_CV_VFogMaxY.m_fValue - vCamera.y) / (vPos.y - vCamera.y);
		vHit.y = g_CV_VFogMaxY.m_fValue;
		vHit.x = (vPos.x - vCamera.x) * t + vCamera.x;
		vHit.z = (vPos.z - vCamera.z) * t + vCamera.z;

		if (g_ViewParams.m_nVFogMode == 1)
		{
			float fTopFog = g_CV_VFogMaxYVal.m_fValue * 2.0f;
			float fFog1 = (vCamera - vHit).Mag() * fTopFog * g_fVFogDensityScale;
			float fPosFog = d3d_GetVFogVal(vPos.y) + g_CV_VFogMaxYVal.m_fValue;
			fFog = (vHit - vPos).Mag() * fPosFog * g_fVFogDensityScale + fFog1;
		}
		else
		{
			float fCamFog = d3d_GetVFogVal(vCamera.y) + g_CV_VFogMaxYVal.m_fValue;
			float fFog1 = (vCamera - vHit).Mag() * fCamFog * g_fVFogDensityScale;
			float fTopFog = g_CV_VFogMaxYVal.m_fValue * 2.0f;
			fFog = (vHit - vPos).Mag() * fTopFog * g_fVFogDensityScale + fFog1;
		}
	}

	if (fFog > g_CV_VFogMax.m_fValue)
		fFog = g_CV_VFogMax.m_fValue;
	else if (fFog < 0.0f)
		fFog = 0.0f;

	((uint8*)pSpecular)[3] = (uint8)(0xFF - RoundFloatToInt(fFog));
}
#endif


// 0x10014340 -- a colour through the gamma table.
uint32 d3d_GammaRGB(uint8 r, uint8 g, uint8 b)
{
	return RGB_MAKE(g_GammaTable.m_Table[r], g_GammaTable.m_Table[g], g_GammaTable.m_Table[b]);
}

// 0x10014380
uint32 d3d_MakeRGB(uint8 r, uint8 g, uint8 b)
{
	return RGB_MAKE(r, g, b);
}


// 0x100143a0 -- the default single-texture blend.
void d3d_SetDefaultBlend()
{
	if (g_CV_UseDX6Commands)
	{
		g_pD3DDevice->SetTextureStageState(0, D3DTSS_COLOROP,   D3DTOP_MODULATE);
		g_pD3DDevice->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
		g_pD3DDevice->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
		g_pD3DDevice->SetTextureStageState(0, D3DTSS_ALPHAOP,   D3DTOP_MODULATE);
		g_pD3DDevice->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
		g_pD3DDevice->SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);
	}
	else
	{
		g_pD3DDevice->SetRenderState(D3DRENDERSTATE_TEXTUREMAPBLEND, D3DTBLEND_MODULATEALPHA);
	}
}


// ---------------------------------------------------------------------- //
//  Render state blocks (the class's members are TU21's).
// ---------------------------------------------------------------------- //
struct RenderState
{
	RenderState() {}
	RenderState(uint32 state, uint32 val) { m_State = state; m_Val = val; }

	uint32  m_State;
	uint32  m_Val;
};

struct StageState
{
	uint32  m_Stage;
	uint32  m_State;
	uint32  m_Val;
};

// A set of render and texture stage states to apply.
class RenderStateSet
{
public:
	RenderStateSet() {}
	RenderStateSet(const RenderState& first) : m_RenderStates(1, first) {}

	std::vector<RenderState>    m_RenderStates;             // +0x00
	std::vector<StageState>     m_StageStates;              // +0x0c
};

// The states it replaced, put back by Restore() or when it goes (TU21's
// CRenderStateList: the names are its, so the calls resolve; Apply still
// takes this file's RenderStateSet).
class CRenderStateList
{
public:
	~CRenderStateList() { Restore(); }

	std::vector<RenderState>    m_RenderStates;             // +0x00
	std::vector<StageState>     m_StageStates;              // +0x0c

	void    Restore();                                      // 0x10022a86
	void    Apply(CRenderStateList* pList, uint32 iStage);  // 0x10022a97
	void    SetState(RenderState* pState);                  // 0x10022b08
};

CRenderStateList    g_TranslucentStates;                    // 0x1005e6c0

static RenderStateSet*  g_pAdditiveSet;                     // 0x1005fe04
static RenderStateSet*  g_pTranslucentSet;                  // 0x1005fe08

// 0x10014550
void d3d_TermTranslucentStates()
{
	if (g_pAdditiveSet)
	{
		delete g_pAdditiveSet;
		g_pAdditiveSet = LTNULL;
	}

	if (g_pTranslucentSet)
	{
		delete g_pTranslucentSet;
		g_pTranslucentSet = LTNULL;
	}
}

// 0x10014620
void d3d_SetTranslucentObjectStates(LTBOOL bAdditive)
{
	g_TranslucentStates.Restore();

	if (bAdditive)
	{
		if (!g_pAdditiveSet)
		{
			g_pAdditiveSet = new RenderStateSet(RenderState(D3DRENDERSTATE_ALPHABLENDENABLE, TRUE));
			g_pAdditiveSet->m_RenderStates.push_back(RenderState(D3DRENDERSTATE_ZWRITEENABLE, FALSE));
			g_pAdditiveSet->m_RenderStates.push_back(RenderState(D3DRENDERSTATE_SRCBLEND, D3DBLEND_ONE));
			g_pAdditiveSet->m_RenderStates.push_back(RenderState(D3DRENDERSTATE_DESTBLEND, D3DBLEND_ONE));
		}

		if (g_pAdditiveSet)
			g_TranslucentStates.Apply((CRenderStateList*)g_pAdditiveSet, 0);
	}
	else
	{
		if (!g_pTranslucentSet)
		{
			g_pTranslucentSet = new RenderStateSet(RenderState(D3DRENDERSTATE_ALPHABLENDENABLE, TRUE));
			g_pTranslucentSet->m_RenderStates.push_back(RenderState(D3DRENDERSTATE_ZWRITEENABLE, FALSE));
			g_pTranslucentSet->m_RenderStates.push_back(RenderState(D3DRENDERSTATE_SRCBLEND, D3DBLEND_SRCALPHA));
			g_pTranslucentSet->m_RenderStates.push_back(RenderState(D3DRENDERSTATE_DESTBLEND, D3DBLEND_INVSRCALPHA));
		}

		if (g_pTranslucentSet)
			g_TranslucentStates.Apply((CRenderStateList*)g_pTranslucentSet, 0);
	}
}

// 0x10014870
void d3d_UnsetTranslucentObjectStates(LTBOOL bChangeZ)
{
	g_TranslucentStates.Restore();
}


// ---------------------------------------------------------------------- //
//  Punch-through (alpha test or colour key).
// ---------------------------------------------------------------------- //

// 0x10014880
void d3d_EnablePunchThrough()
{
	if (g_CV_AlphaTest.m_nValue)
	{
		g_pD3DDevice->SetRenderState(D3DRENDERSTATE_ALPHATESTENABLE, TRUE);
		g_pD3DDevice->SetRenderState(D3DRENDERSTATE_ALPHAFUNC, D3DCMP_GREATER);
	}
	else
	{
		g_pD3DDevice->SetRenderState(D3DRENDERSTATE_COLORKEYENABLE, TRUE);
	}

	d3d_SetWorldFlags(1);
}

// 0x100148c0
void d3d_DisablePunchThrough()
{
	if (g_CV_AlphaTest.m_nValue)
	{
		g_pD3DDevice->SetRenderState(D3DRENDERSTATE_ALPHATESTENABLE, FALSE);
	}
	else
	{
		g_pD3DDevice->SetRenderState(D3DRENDERSTATE_COLORKEYENABLE, FALSE);
	}

	d3d_SetWorldFlags(0);
}


// ---------------------------------------------------------------------- //
//  The transformed vertex pool.
// ---------------------------------------------------------------------- //
struct TLVertex
{
	LTVector    m_Vec;                                      // +0x00
	float       rhw;                                        // +0x0c
	uint32      color;                                      // +0x10
	uint32      specular;                                   // +0x14
	float       tu, tv;                                     // +0x18

	void        SetTexCoords(float u, float v) { tu = u; tv = v; }
};

uint32      g_nTLVertPool;                                  // 0x1005fe00
TLVertex*   g_TLVertPool;                                   // 0x1005e294

// 0x10014900 -- grow the pool to nWanted vertices.
LTBOOL d3d_GrowTLVertPool(uint32 nWanted)
{
	TLVertex* pNew;

	pNew = (TLVertex*)dalloc(nWanted * sizeof(TLVertex));
	if (!pNew)
		return FALSE;

	memcpy(pNew, g_TLVertPool, g_nTLVertPool * sizeof(TLVertex));
	if (g_TLVertPool)
		dfree(g_TLVertPool);

	g_nTLVertPool = nWanted;
	g_TLVertPool = pNew;
	return TRUE;
}


// ---------------------------------------------------------------------- //
//  The deferred polygons.
// ---------------------------------------------------------------------- //
struct RenderPoly;

struct DeferredPoly
{
	RenderPoly*     m_pPoly;                                // +0x00
	uint32          m_Reserved04[2];
	uint32          m_ClipFlags;                            // +0x0c
	DeferredPoly*   m_pNext;                                // +0x10
	uint32          m_Reserved14;
};

extern uint32   g_ClipFlags;                                // 0x1005bd0c

StructBank      g_DeferredPolyBank;                         // 0x1005e1f0
DeferredPoly*   g_pDeferredPolies;                          // 0x1005e700

// 0x10014970 -- put a polygon on the deferred list.
void d3d_DeferPoly(RenderPoly* pPoly)
{
	DeferredPoly* pDeferred;

	pDeferred = (DeferredPoly*)sb_Allocate(&g_DeferredPolyBank);
	pDeferred->m_pPoly = pPoly;
	pDeferred->m_pNext = g_pDeferredPolies;
	pDeferred->m_ClipFlags = g_ClipFlags;
	g_pDeferredPolies = pDeferred;
}


// ---------------------------------------------------------------------- //
//  The view and the world's polygons, as this file reads them.
// ---------------------------------------------------------------------- //

struct RenderVertex
{
	LTVector    m_Vec;                                      // +0x00
};

struct RenderPolyVert
{
	RenderVertex*   m_pVert;                                // +0x00
	float           tu, tv;                                 // +0x04
	uint8           m_Reserved0c[0x0c];
};

struct SharedTexture;

struct RenderSurface
{
	uint8           m_Reserved00[0x2c];
	SharedTexture*  m_pTexture;                             // +0x2c
	uint32          m_Flags;                                // +0x30
	uint8           m_Reserved34[0x3a - 0x34];
	uint16          m_TextureFlags;                         // +0x3a
};

// A surface's texture flags: a portal (a mirror), and whether the
// portal's own polygon is drawn over what it shows.
#define TEXFLAG_PORTALMASK  0x7fff
#define TEXFLAG_PORTAL      0x7ffe
#define TEXFLAG_DRAWPORTAL  0x8000

struct RenderPoly
{
	uint8           m_Reserved00[0x18];
	LTVector        m_vCenter;                              // +0x18
	uint8           m_Reserved24[0x28 - 0x24];
	LTPlane*        m_pPlane;                               // +0x28
	RenderSurface*  m_pSurface;                             // +0x2c
	uint8           m_Reserved30[0x50 - 0x30];
	RenderPolyVert* m_pTJuncVerts;                          // +0x50
	uint16          m_nVerts;                               // +0x54
	uint16          m_nTJuncVerts;                          // +0x56
	RenderPolyVert  m_Verts[1];                             // +0x58
};

#define SURF_DOUBLESIDED    0x80

extern int32    g_CV_FixTJunc;                              // 0x1005dbb4
extern int32    g_CV_ShowSplits;                            // 0x1005df54
extern float    g_fFillArea;                                // 0x1005c12c
extern uint32   g_nDrawPrimCalls;                           // 0x1005c144
extern uint32   g_nTrianglesDrawn;                          // 0x1005c148

// The texture stages' bound textures (d3d_texture.cpp).
extern uint32               g_iCurTextureStage;             // 0x100638c0
class RTexture;
extern RTexture*     g_pCurTextures[];                       // 0x10067270 (d3d_texture.cpp)

// Unbind a stage's texture (d3d_DisableTexture, 0x1000a22b, spelt inline).
void d3d_DisableTexture(uint32 iStage);                     // 0x1000a22b

inline void d3d_UnsetTexture(uint32 iStage)
{
	if (g_pCurTextures[iStage])
	{
		g_pD3DDevice->SetTexture(iStage, LTNULL);
		g_pCurTextures[iStage] = LTNULL;
	}
}

// The clipper (drawmodel_clip.cpp, the TU07 helpers).
float   d3d_MatVMul_InPlace_H(LTMatrix* pMat, LTVector* pSrc);              // 0x1000868d
void    d3d_TransformToCamera(TLVertex* pVert, LTMatrix* pMat);             // 0x10008719
LTBOOL  d3d_ClipTLVerts(TLVertex** ppVerts, int* pnVerts, ViewParams* pParams, uint32 flags);  // 0x1000aec6
struct ClipVert;
LTBOOL  ClipPoly(uint32 nClipFlags, ClipVert** ppVerts, int32* pnVerts);  // 0x1000af61 (drawworld.cpp; d3d_clip.h's, for the 32-byte vertex)
void    d3d_ProjectTLVert(TLVertex* pVert, ViewParams* pParams);            // 0x1002739d


// 0x100149c0 -- ShowFillInfo: measure the polygon's screen area and draw it
// later.
void d3d_DrawPolyFillInfo(RenderPoly* pPoly)
{
	TLVertex verts[30];
	TLVertex* pVerts;
	int nVerts;
	uint32 i;
	uint16 iTri;
	float fArea;

	nVerts = pPoly->m_nVerts;
	pVerts = verts;
	for (i = 0; i < pPoly->m_nVerts; i++)
	{
		verts[i].m_Vec.x = pPoly->m_Verts[i].m_pVert->m_Vec.x;
		verts[i].m_Vec.y = pPoly->m_Verts[i].m_pVert->m_Vec.y;
		verts[i].m_Vec.z = pPoly->m_Verts[i].m_pVert->m_Vec.z;
	}

	if (!g_ClipFlags)
	{
		for (i = 0; i < (uint32)nVerts; i++)
			pVerts[i].rhw = d3d_MatVMul_InPlace_H(&g_ViewParams.m_mFullTransform, &pVerts[i].m_Vec);
	}
	else
	{
		for (i = 0; i < (uint32)nVerts; i++)
			d3d_TransformToCamera(&pVerts[i], &g_ViewParams.m_mWorldToCamera);

		if (!ClipPoly(g_ClipFlags, (ClipVert**)&pVerts, (int32*)&nVerts))
			return;

		for (i = 0; i < (uint32)nVerts; i++)
			d3d_ProjectTLVert(&pVerts[i], &g_ViewParams);
	}

	fArea = 0.0f;
	for (iTri = 0; iTri < nVerts - 2; iTri++)
	{
		fArea += pVerts[iTri+1].m_Vec.y * pVerts[0].m_Vec.x - pVerts[iTri+1].m_Vec.x * pVerts[0].m_Vec.y
			+ pVerts[0].m_Vec.y * pVerts[iTri+2].m_Vec.x - pVerts[0].m_Vec.x * pVerts[iTri+2].m_Vec.y
			+ pVerts[iTri+1].m_Vec.x * pVerts[iTri+2].m_Vec.y - pVerts[iTri+1].m_Vec.y * pVerts[iTri+2].m_Vec.x;
	}

	if (pPoly->m_pSurface->m_Flags & SURF_DOUBLESIDED)
		fArea += fArea;

	g_fFillArea += fArea * 0.5f;
	d3d_DeferPoly(pPoly);
}


// 0x10014b80 -- ShowSplits/DrawFlat: the polygon in a colour of its own.
LTBOOL d3d_DrawPolyFlat(RenderPoly* pPoly)
{
	TLVertex verts[40];
	TLVertex* pVerts;
	int nVerts;
	RenderPolyVert* pPolyVerts;
	uint32 color;
	int i;

	color = (uint32)pPoly;
	if (!g_CV_ShowSplits)
		color = (uint32)pPoly->m_pSurface;

	d3d_UnsetTexture(g_iCurTextureStage);

	if (g_CV_FixTJunc)
	{
		pPolyVerts = pPoly->m_pTJuncVerts;
		nVerts = pPoly->m_nTJuncVerts;
	}
	else
	{
		pPolyVerts = pPoly->m_Verts;
		nVerts = pPoly->m_nVerts;
	}

	for (i = 0; i < nVerts; i++)
	{
		MatVMul_H(&verts[i].m_Vec, &g_ViewParams.m_mDeviceTransform, &pPolyVerts[i].m_pVert->m_Vec);
		verts[i].color = color;
		verts[i].specular = 0xFFFFFFFF;
	}

	g_nTrianglesDrawn += nVerts - 2;
	pVerts = verts;
	if (d3d_ClipTLVerts(&pVerts, &nVerts, &g_ViewParams, 0))
	{
		g_pD3DDevice->DrawPrimitive(D3DPT_TRIANGLEFAN, D3DFVF_TLVERTEX, pVerts, nVerts, 0);
		g_nDrawPrimCalls++;
		return TRUE;
	}

	return FALSE;
}


// 0x10014d30 -- free a deferred list.
void d3d_FreeDeferredPolies(DeferredPoly* pList)
{
	DeferredPoly* pNext;

	while (pList)
	{
		pNext = pList->m_pNext;
		sb_Free(&g_DeferredPolyBank, pList);
		pList = pNext;
	}
}


// ---------------------------------------------------------------------- //
//  RenderStruct functions.
// ---------------------------------------------------------------------- //
#define CLEARSCREEN_SCREEN  1
#define CLEARSCREEN_RENDER  2

class PFormat;

union GenericColor
{
	uint32	dwVal;
	uint16	wVal;
	uint8	bVal;
};

class FormatMgr
{
public:
	LTRESULT    PValueFromFormatColor(PFormat* pFormat, GenericColor in, uint32& out);   // 0x10037780
};

// pixelformat.cpp's, as far as this file reads it.
class PFormat
{
public:
	void*       m_pVTable;
	uint32      m_eType;                                    // +0x04
};
typedef PFormat ScreenFormat;

#define BPP_16  2

extern FormatMgr                g_FormatMgr;                // 0x100661a8
extern ScreenFormat             g_ScreenFormat;             // 0x1005d260
extern LPDIRECTDRAWSURFACE7     g_pBackBuffer;              // 0x1005d2b4
extern uint32                   g_ScreenWidth;              // 0x1005d8cc
extern uint32                   g_ScreenHeight;             // 0x1005d9c0
uint32                          g_bSceneCleared;            // 0x1005e1c0

LTBOOL  d3d_HasStencil();                                   // 0x1001b460 (d3d_init.cpp)
void    InvalidateRect(LTRect* pRect);                      // 0x10022e31

// A 0..255 colour component as 0..1.
inline float ColorScale(float f)
{
	return f * (1.0f / 255.0f);
}

// 0x10014d70
void d3d_Clear(LTRect* pRect, uint32 flags, LTVector* pClearColor)
{
	D3DRECT clearRect, *pClearRect;
	uint32 nRects, realFlags, dwColor;
	DDBLTFX fx;
	uint32 fillColor;
	float r, g, b;

	if (!g_pD3DDevice)
		return;

	if (pClearColor)
	{
		r = pClearColor->x * (1.0f / 255.0f);
		g = pClearColor->y * (1.0f / 255.0f);
		b = pClearColor->z * (1.0f / 255.0f);
		dwColor = RGBA_MAKE((int)(r * 255.0f), (int)(g * 255.0f), (int)(b * 255.0f), 0xFF);
	}
	else
	{
		dwColor = 0xFF000000;
	}

	if (!pRect || (pRect->left == 0 && pRect->top == 0 &&
		pRect->right == (int)g_ScreenWidth && pRect->bottom == (int)g_ScreenHeight))
	{
		pClearRect = LTNULL;
		nRects = 0;
	}
	else
	{
		clearRect.x1 = pRect->left;
		clearRect.y1 = pRect->top;
		clearRect.x2 = pRect->right;
		clearRect.y2 = pRect->bottom;
		pClearRect = &clearRect;
		nRects = 1;
	}

	realFlags = 0;
	if (flags & CLEARSCREEN_SCREEN)
	{
		if (!pClearRect)
		{
			memset(&fx, 0, sizeof(fx));
			fx.dwSize = sizeof(fx);
			if (g_ScreenFormat.m_eType == BPP_16)
			{
				g_FormatMgr.PValueFromFormatColor((PFormat*)&g_ScreenFormat, *(GenericColor*)&dwColor, fillColor);
				fx.dwFillColor = (uint16)fillColor;
			}
			else
			{
				fx.dwFillColor = dwColor;
			}

			g_pBackBuffer->Blt(LTNULL, LTNULL, LTNULL, DDBLT_COLORFILL | DDBLT_WAIT, &fx);
		}
		else
		{
			realFlags = D3DCLEAR_TARGET;
		}
	}

	if (flags & CLEARSCREEN_RENDER)
	{
		realFlags |= D3DCLEAR_ZBUFFER;
		if (d3d_HasStencil())
			realFlags |= D3DCLEAR_STENCIL;
	}

	if (realFlags)
		g_pD3DDevice->Clear(nRects, pClearRect, realFlags, dwColor, 1.0f, 0);

	g_bSceneCleared = FALSE;
	InvalidateRect(pRect);
}


// ---------------------------------------------------------------------- //
//  The draw mode: which routine draws each kind of world polygon.
// ---------------------------------------------------------------------- //
typedef void (*DrawPolyFn)(RenderPoly* pPoly);

DrawPolyFn  g_pfnDrawGouraudPoly;                           // 0x1005e280
DrawPolyFn  g_pfnDrawDeferredPoly;                          // 0x1005e770
DrawPolyFn  g_pfnDrawLightmapPoly;                          // 0x1005fd9c
DrawPolyFn  g_pfnDrawTexturedPoly;                          // 0x1005e6bc

uint32      g_bDrawWorldFirstPass;                          // 0x1005e1c8
uint32      g_bDrawWorldTextured;                           // 0x1005e1cc
uint32      g_bDrawWorldFog;                                // 0x1005e798

struct RenderContext
{
	void*       m_pMainWorld;                               // +0x00
};

typedef struct SceneWorld SceneWorldFlags;  // common_draw.cpp's
struct SceneWorld
{
	uint8       m_Reserved000[0x180];
	uint32      m_Flags;                                    // +0x180
};

extern int32            g_CV_ShowFillInfo;                  // 0x1005df4c
extern int32            g_CV_DrawFlat;                      // 0x1005df74
extern int32            g_CV_LightMap;                      // 0x1004a790
extern LTBOOL           g_bLightmapPages;                   // 0x100638b8
extern SceneWorldFlags* g_pSceneWorld;                      // 0x1005c208
extern RenderContext*   g_pRenderContext;                   // 0x1005b778
extern LTBOOL           g_bMultiPass;                       // 0x10062278
extern LTBOOL           g_bFogCapable;                      // 0x10062888

void    d3d_QueueGouraudPoly(void* pPoly);                  // 0x100238c5 (drawgouraud.cpp)
void    d3d_DrawLightmapPoly(RenderPoly* pPoly);            // 0x100083bf
void    d3d_QueueLightmapPoly(void* pPoly);                 // 0x1003647b
void    d3d_DrawTexturedPoly(RenderPoly* pPoly);            // 0x1000a11b
void    d3d_SetLightmapStages();                            // 0x10036498
void    d3d_CanvasPreFrame();                               // 0x1002377f

float   d3d_MatVMul_H(LTVector* pDest, LTMatrix* pMat, LTVector* pSrc);  // 0x1000dcec
void    d3d_GetPolyVerts(RenderPoly* pPoly, RenderPolyVert** ppVerts, int* pnVerts);    // 0x10015230
LTBOOL  d3d_DrawTLPoly(TLVertex** ppVerts, int* pnVerts);   // 0x10015270

// A vertex as the D3D_OVERLOADS headers give it: with a constructor.
struct TLVertexC : public TLVertex
{
	TLVertexC() {}
};

// 0x100158c0 (its body follows d3d_FullDrawScene, its first caller that
// does not expand it).
static void d3d_SetDrawFunctions(int32 nMode)
{
	DrawPolyFn pfn;

	g_pfnDrawGouraudPoly = (DrawPolyFn)d3d_QueueGouraudPoly;
	g_pfnDrawDeferredPoly = (DrawPolyFn)d3d_DeferPoly;
	g_pfnDrawLightmapPoly = (DrawPolyFn)d3d_DrawLightmapPoly;

	if (g_bMultiPass)
	{
		pfn = (DrawPolyFn)d3d_QueueLightmapPoly;
		g_pfnDrawLightmapPoly = pfn;
		g_pfnDrawTexturedPoly = pfn;
	}
	else
	{
		g_pfnDrawTexturedPoly = (DrawPolyFn)d3d_DrawTexturedPoly;
	}

	if (g_CV_ShowFillInfo)
	{
		pfn = (DrawPolyFn)d3d_DrawPolyFillInfo;
		g_pfnDrawLightmapPoly = pfn;
		g_pfnDrawTexturedPoly = pfn;
		g_pfnDrawGouraudPoly = pfn;
		g_pfnDrawDeferredPoly = pfn;
	}
	else if (g_CV_DrawFlat)
	{
		pfn = (DrawPolyFn)d3d_DeferPoly;
		g_pfnDrawLightmapPoly = pfn;
		g_pfnDrawTexturedPoly = pfn;
		g_pfnDrawGouraudPoly = pfn;
		g_pfnDrawDeferredPoly = pfn;
	}
	else if (!g_CV_LightMap || !g_bLightmapPages || !(g_pSceneWorld->m_Flags & 4))
	{
		pfn = (DrawPolyFn)d3d_QueueGouraudPoly;
		g_pfnDrawTexturedPoly = pfn;
		g_pfnDrawLightmapPoly = pfn;
	}

	if (nMode == 1)
	{
		pfn = (DrawPolyFn)d3d_QueueGouraudPoly;
		g_pfnDrawTexturedPoly = pfn;
		g_pfnDrawLightmapPoly = pfn;
	}

	if (!g_pRenderContext->m_pMainWorld)
		g_pfnDrawLightmapPoly = (DrawPolyFn)d3d_QueueGouraudPoly;
}

// 0x10014f30
void d3d_SetDrawMode(int32 nMode)
{
	d3d_SetDrawFunctions(nMode);

	if (nMode == 1)
	{
		g_bDrawWorldTextured = 1;
		g_bDrawWorldFirstPass = 1;
	}
	else
	{
		g_bDrawWorldFirstPass = 0;
		g_bDrawWorldTextured = 1;
	}

	if (g_bFogCapable)
		g_bDrawWorldFog = (nMode != 1);
	else
		g_bDrawWorldFog = 0;

	if (g_bMultiPass)
		d3d_SetLightmapStages();
	else
		d3d_CanvasPreFrame();
}


// ---------------------------------------------------------------------- //
//  The deferred polygons, drawn.
// ---------------------------------------------------------------------- //
class DrawPolyMgr
{
public:
	void    Flush();                                        // 0x1002aee2
};

struct GouraudBucket;
GouraudBucket*      g_GouraudPolyList;                      // 0x1005e728
extern CD3DConVar   g_CV_DrawPolyMgr;                       // 0x10074258
extern DrawPolyMgr  g_DrawPolyMgr;                          // 0x100746d8

void    d3d_SetGouraudBlend(LTBOOL bAdditive);              // 0x1002429c
void    d3d_FlushGouraudPolies();                           // 0x100242c2
void    d3d_DrawWorldPieces();                              // 0x1000ac3a
void    d3d_DrawDeferredPieces();                           // 0x10007976
void    d3d_MultiPassFlush();                               // 0x10035c7e
void    d3d_FlushWorldPieces();                             // 0x1000ac2b
void    d3d_ResetLightmapStages();                          // 0x10036534

// 0x10015070
void d3d_FlushDeferredPolies(LTBOOL bAdditive)
{
	DeferredPoly *pCur, *pNext;
	RenderPoly* pPoly;
	RenderPolyVert* pPolyVerts;
	TLVertex* pVerts;
	int nVerts, i;
	uint32 color;

	if (g_GouraudPolyList)
	{
		d3d_SetGouraudBlend(bAdditive);
		d3d_FlushGouraudPolies();
		d3d_CanvasPreFrame();
	}

	d3d_DrawWorldPieces();
	d3d_DrawDeferredPieces();

	if (g_bMultiPass)
		d3d_MultiPassFlush();
	else
		d3d_FlushWorldPieces();

	if (g_bMultiPass)
		d3d_ResetLightmapStages();
	else
		d3d_CanvasPreFrame();

	if (g_pDeferredPolies)
	{
		d3d_UnsetTexture(g_iCurTextureStage);

		for (pCur = g_pDeferredPolies; pCur; pCur = pNext)
		{
			TLVertexC verts[40];

			g_ClipFlags = pCur->m_ClipFlags;
			pNext = pCur->m_pNext;
			pPoly = pCur->m_pPoly;

			color = (uint32)pPoly;
			if (!g_CV_ShowSplits)
				color = (uint32)pPoly->m_pSurface;

			d3d_DisableTexture(g_iCurTextureStage);
			d3d_GetPolyVerts(pPoly, &pPolyVerts, &nVerts);
			for (i = 0; i < nVerts; i++)
			{
				d3d_MatVMul_H(&verts[i].m_Vec, &g_ViewParams.m_mDeviceTransform, &pPolyVerts[i].m_pVert->m_Vec);
				verts[i].color = color;
				verts[i].specular = 0xFFFFFFFF;
			}

			g_nTrianglesDrawn += nVerts - 2;
			pVerts = verts;
			d3d_DrawTLPoly(&pVerts, &nVerts);

			sb_Free(&g_DeferredPolyBank, pCur);
		}

		g_pDeferredPolies = LTNULL;
	}

	if (g_CV_DrawPolyMgr.m_nValue)
		g_DrawPolyMgr.Flush();
}

// 0x10015230
void d3d_GetPolyVerts(RenderPoly* pPoly, RenderPolyVert** ppVerts, int* pnVerts)
{
	if (g_CV_FixTJunc)
	{
		*ppVerts = pPoly->m_pTJuncVerts;
		*pnVerts = pPoly->m_nTJuncVerts;
	}
	else
	{
		*ppVerts = pPoly->m_Verts;
		*pnVerts = pPoly->m_nVerts;
	}
}

// 0x10015270 -- transform (or clip), project and draw a TL fan.
LTBOOL d3d_DrawTLPoly(TLVertex** ppVerts, int* pnVerts)
{
	TLVertex* pVert;
	int i;

	if (!g_ClipFlags)
	{
		pVert = *ppVerts;
		for (i = *pnVerts; i; i--)
		{
			pVert->rhw = MatVMul_InPlace_H(&g_ViewParams.m_mFullTransform, &pVert->m_Vec);
			pVert++;
		}
	}
	else
	{
		pVert = *ppVerts;
		for (i = *pnVerts; i; i--)
		{
			d3d_TransformToCamera(pVert, &g_ViewParams.m_mWorldToCamera);
			pVert++;
		}

		if (!ClipPoly(g_ClipFlags, (ClipVert**)ppVerts, (int32*)pnVerts))
			return FALSE;

		pVert = *ppVerts;
		for (i = *pnVerts; i; i--)
		{
			d3d_ProjectTLVert(pVert, &g_ViewParams);
			pVert++;
		}
	}

	g_pD3DDevice->DrawPrimitive(D3DPT_TRIANGLEFAN, D3DFVF_TLVERTEX, *ppVerts, *pnVerts, 0);
	g_nDrawPrimCalls++;
	return TRUE;
}


// 0x10015400 -- an LTMatrix as a D3DMATRIX (transposed).
void d3d_SetD3DMat(LTMatrix* pSrc, D3DMATRIX* pDest)
{
	D3DMATRIX Out;

	Out._11 = pSrc->m[0][0]; Out._12 = pSrc->m[1][0]; Out._13 = pSrc->m[2][0]; Out._14 = pSrc->m[3][0];
	Out._21 = pSrc->m[0][1]; Out._22 = pSrc->m[1][1]; Out._23 = pSrc->m[2][1]; Out._24 = pSrc->m[3][1];
	Out._31 = pSrc->m[0][2]; Out._32 = pSrc->m[1][2]; Out._33 = pSrc->m[2][2]; Out._34 = pSrc->m[3][2];
	Out._41 = pSrc->m[0][3]; Out._42 = pSrc->m[1][3]; Out._43 = pSrc->m[2][3]; Out._44 = pSrc->m[3][3];

	*pDest = Out;
}


// ---------------------------------------------------------------------- //
//  The full-screen colour passes.
// ---------------------------------------------------------------------- //
union RGBColor
{
	struct { uint8 b, g, r, a; } rgb;
	uint32 color;
};

// A vertex for the static batches: its destructor does nothing.
struct QuadVerts
{
	~QuadVerts() {}

	TLVertex    m_Verts[4];
};

// Save a render state, set it, and put it back when the scope closes.
class StateSet
{
public:
	StateSet(uint32 state, uint32 val)
	{
		m_State = state;
		g_pD3DDevice->GetRenderState((D3DRENDERSTATETYPE)state, &m_OldVal);
		g_pD3DDevice->SetRenderState((D3DRENDERSTATETYPE)state, val);
	}

	~StateSet()
	{
		g_pD3DDevice->SetRenderState((D3DRENDERSTATETYPE)m_State, m_OldVal);
	}

	uint32  m_State;
	DWORD   m_OldVal;
};

extern int32    g_CV_LightAddPoly;                          // 0x1004a744
extern LTBOOL   g_bLightAddPolyCapable;                     // 0x10062860
extern int32    g_CV_InvertHack;                            // 0x1005df14

void d3d_DrawLightAddPoly(const LTVector* pAdd);
void d3d_DrawLightScalePoly(const LTVector* pScale);


// ---------------------------------------------------------------------- //
//  FullDrawScene.
// ---------------------------------------------------------------------- //
#define CSTART_NONE 0

class CounterFinal
{
public:
	CounterFinal(unsigned long startMode = CSTART_NONE);    // 0x10013fb7
	unsigned long   m_Data[2];
};

typedef CounterFinal Counter;

void            cnt_StartCounterFinal(CounterFinal& cCounter);  // 0x10013fd5
unsigned long   cnt_EndCounterFinal(CounterFinal& cCounter);    // 0x10013fe0

inline void cnt_StartCounter(Counter& cCounter) { cnt_StartCounterFinal(cCounter); }
inline unsigned long cnt_EndCounter(Counter& cCounter) { return cnt_EndCounterFinal(cCounter); }

class CountAdder
{
public:
	CountAdder(uint32* pNum)
	{
		m_pNum = pNum;
		cnt_StartCounter(m_Counter);
	}

	~CountAdder()
	{
		*m_pNum += cnt_EndCounter(m_Counter);
	}

	Counter     m_Counter;
	uint32*     m_pNum;
};

class LTObject
{
public:
	uint8       m_Reserved000[0xbc];
	char        m_ObjectType;                               // +0x0bc
};

#define DRAWMODE_OBJECTLIST 2
#define NUM_OBJECTTYPES     11

struct SceneDesc
{
	int32       m_DrawMode;                                 // +0x00
	uint32*     m_pTicks_Render_Objects;                    // +0x04
	uint8       m_Reserved08[0x34 - 0x08];
	struct RenderWorld* m_pWorld;                           // +0x34
	uint8       m_Reserved38[0x50 - 0x38];
	LTVector    m_GlobalLightScale;                         // +0x50
	uint8       m_Reserved5c[0x74 - 0x5c];
	LTVector    m_GlobalLightAdd;                           // +0x74
	float       m_FrameTime;                                // +0x80
	uint8       m_Reserved84[0xd4 - 0x84];
	float       m_FarZ;                                     // +0xd4
	LTVector    m_Pos;                                      // +0xd8 (the camera's)
	uint8       m_Reservede4[0xf4 - 0xe4];
	LTObject**  m_pObjectList;                              // +0xf4
	int         m_ObjectListSize;                           // +0xf8
};

struct RenderStruct
{
	uint8       m_Reserved00[0x14];
	void        (*ConsolePrint)(const char* pMsg, ...);     // +0x14
	uint8       m_Reserved18[0x28 - 0x18];
	uint32      (*GetTime)();                               // +0x28
	uint8       m_Reserved2c[0x48 - 0x2c];
	uint32      m_nCounter48;                               // +0x48
	uint32      m_nCounter4c;                               // +0x4c
	uint8       m_Reserved50[0x58 - 0x50];
	uint32      m_WorldTicks;                               // +0x58
	uint32      m_ObjectTicks;                              // +0x5c
	uint8       m_Reserved60[0x138 - 0x60];
	LTVector    m_vSunDir;                                  // +0x138
};

typedef void (*ProcessObjectFn)(LTObject* pObject);

// drawobjects.cpp's record, whole: the table's symbol is its start
// (0x1004da88), and the frame's two hooks sit at +0x0c and +0x10. (This
// view had them at +0x00/+0x04 against the table's +0x0c: displacements
// are masked relocations, so the bytes matched, but the link resolves the
// symbol to the table's start, and every visible model ran
// d3d_TermModels, OT_MODEL's m_ModuleTerm, in place of d3d_ProcessModel.)
struct ObjectHandler
{
	void            (*m_ModuleInit)();                      // +0x00
	void            (*m_ModuleTerm)();                      // +0x04
	uint32          m_bModuleInitted;                       // +0x08
	void            (*m_PreFrameFn)();                      // +0x0c
	ProcessObjectFn m_ProcessObjectFn;                      // +0x10
};

// A visible portal: its polygon and the transform it's drawn through.
struct PortalRef
{
	RenderPoly*     m_pPoly;                                // +0x00
	LTMatrix*       m_pTransform;                           // +0x04
};

#define MAX_VISIBLE_PORTALS 32

struct VisibleSet
{
	void    ClearSet();                                     // 0x10039e80

	uint8       m_Reserved000[0x40];
	PortalRef   m_Portals[MAX_VISIBLE_PORTALS];             // +0x040
	uint32      m_nPortals;                                 // +0x140
};

extern RenderStruct*    g_pStruct;                          // 0x1005df08
extern ObjectHandler    g_ObjectHandlers[NUM_OBJECTTYPES];  // 0x1004da88

VisibleSet* d3d_GetVisibleSet();                            // 0x1003abb0
void    d3d_InitObjectQueues();                             // 0x10029430
void    d3d_FlushObjectQueues();                            // 0x10029450
void    d3d_InitFrameState();                               // 0x1001159b (common_draw.cpp)
void    d3d_QueueVisibleObjects();                          // 0x1003a2c0
extern CD3DConVar g_CV_UseD3DClip;                          // 0x100622b0 (d3d_init.cpp)
void    d3d_FreeLitObjects();                               // 0x10011517 (common_draw.cpp)

// The objects of an object-list scene.
static void d3d_ProcessObjectList(LTObject** pObjectList, int objectListSize)
{
	int i;
	LTObject* pObject;

	for (i = 0; i < objectListSize; i++)
	{
		pObject = pObjectList[i];
		if (pObject && pObject->m_ObjectType >= 0 && pObject->m_ObjectType < NUM_OBJECTTYPES &&
			g_ObjectHandlers[pObject->m_ObjectType].m_ProcessObjectFn)
		{
			g_ObjectHandlers[pObject->m_ObjectType].m_ProcessObjectFn(pObject);
		}
	}
}

void d3d_DrawDeferredList(DeferredPoly* pList);

// The end of the world pass: flush the queues and the deferred polygons.
static void d3d_EndWorldDraw()
{
	if (g_GouraudPolyList)
	{
		d3d_SetGouraudBlend(FALSE);
		d3d_FlushGouraudPolies();
		d3d_CanvasPreFrame();
	}

	d3d_DrawWorldPieces();
	d3d_DrawDeferredPieces();

	if (g_bMultiPass)
		d3d_MultiPassFlush();
	else
		d3d_FlushWorldPieces();

	if (g_bMultiPass)
		d3d_ResetLightmapStages();
	else
		d3d_CanvasPreFrame();

	if (g_pDeferredPolies)
	{
		d3d_DisableTexture(g_iCurTextureStage);
		d3d_DrawDeferredList(g_pDeferredPolies);
		g_pDeferredPolies = LTNULL;
	}

	if (g_CV_DrawPolyMgr.m_nValue)
		g_DrawPolyMgr.Flush();
}

// 0x100154c0
void d3d_FullDrawScene(SceneDesc* pDesc)
{
	Counter cTicks;
	D3DMATRIX mat;

	g_pD3DDevice->SetRenderState(D3DRENDERSTATE_CLIPPING, g_CV_UseD3DClip.m_nValue != 0);

	d3d_SetD3DMat(&g_ViewParams.m_mDeviceTransform, &mat);
	g_pD3DDevice->SetTransform(D3DTRANSFORMSTATE_WORLD, &mat);
	d3d_SetD3DMat(&g_ViewParams.m_mView, &mat);
	g_pD3DDevice->SetTransform(D3DTRANSFORMSTATE_VIEW, &mat);
	d3d_SetD3DMat(&g_ViewParams.m_mProjection, &mat);
	g_pD3DDevice->SetTransform(D3DTRANSFORMSTATE_PROJECTION, &mat);

	d3d_InitObjectQueues();
	d3d_InitFrameState();

	if (pDesc->m_DrawMode == DRAWMODE_OBJECTLIST)
	{
		d3d_GetVisibleSet()->ClearSet();
		d3d_ProcessObjectList(pDesc->m_pObjectList, pDesc->m_ObjectListSize);
	}
	else
	{
		CountAdder cntWorld(&g_pStruct->m_WorldTicks);

		d3d_SetDrawMode(0);
		d3d_QueueVisibleObjects();
		d3d_EndWorldDraw();
	}

	{
		CountAdder cntObjects(pDesc->m_pTicks_Render_Objects);
		CountAdder cntFlush(&g_pStruct->m_ObjectTicks);
		d3d_FlushObjectQueues();
	}

	d3d_FreeLitObjects();
	d3d_DrawLightScalePoly(&pDesc->m_GlobalLightScale);
	d3d_DrawLightAddPoly(&pDesc->m_GlobalLightAdd);
}


// 0x10015760 -- draw a deferred list flat (d3d_DrawPolyFlat's drawing,
// through the out-of-line transform) and free it.
void d3d_DrawDeferredList(DeferredPoly* pList)
{
	DeferredPoly *pCur, *pNext;
	RenderPoly* pPoly;
	RenderPolyVert* pPolyVerts;
	TLVertex verts[40];
	TLVertex* pVerts;
	int nVerts, i;
	uint32 color;

	for (pCur = pList; pCur; pCur = pNext)
	{
		pNext = pCur->m_pNext;
		g_ClipFlags = pCur->m_ClipFlags;
		pPoly = pCur->m_pPoly;

		color = (uint32)pPoly;
		if (!g_CV_ShowSplits)
			color = (uint32)pPoly->m_pSurface;

		d3d_UnsetTexture(g_iCurTextureStage);

		if (g_CV_FixTJunc)
		{
			pPolyVerts = pPoly->m_pTJuncVerts;
			nVerts = pPoly->m_nTJuncVerts;
		}
		else
		{
			pPolyVerts = pPoly->m_Verts;
			nVerts = pPoly->m_nVerts;
		}

		for (i = 0; i < nVerts; i++)
		{
			d3d_MatVMul_H(&verts[i].m_Vec, &g_ViewParams.m_mDeviceTransform, &pPolyVerts[i].m_pVert->m_Vec);
			verts[i].color = color;
			verts[i].specular = 0xFFFFFFFF;
		}

		g_nTrianglesDrawn += nVerts - 2;
		pVerts = verts;
		if (d3d_ClipTLVerts(&pVerts, &nVerts, &g_ViewParams, 0))
		{
			g_pD3DDevice->DrawPrimitive(D3DPT_TRIANGLEFAN, D3DFVF_TLVERTEX, pVerts, nVerts, 0);
			g_nDrawPrimCalls++;
		}

		sb_Free(&g_DeferredPolyBank, pCur);
	}
}

// 0x100159a0
void d3d_DrawLightAddPoly(const LTVector* pAdd)
{
	static QuadVerts quad;
	TLVertex* verts = quad.m_Verts;
	RGBColor theColor;

	if (!g_CV_LightAddPoly || !g_bLightAddPolyCapable)
		return;

	if (pAdd->x < 0.001f && pAdd->y < 0.001f && pAdd->z < 0.001f)
		return;

	theColor.rgb.r = (uint8)(pAdd->x * 255.0f);
	theColor.rgb.g = (uint8)(pAdd->y * 255.0f);
	theColor.rgb.b = (uint8)(pAdd->z * 255.0f);
	theColor.rgb.a = 0xFF;

	verts[0].m_Vec.x = (float)g_ViewParams.m_Rect.left;
	verts[0].m_Vec.y = (float)g_ViewParams.m_Rect.top;
	verts[0].m_Vec.z = 0.0f;
	verts[0].color = theColor.color;

	verts[1].m_Vec.x = (float)g_ViewParams.m_Rect.right;
	verts[1].m_Vec.y = (float)g_ViewParams.m_Rect.top;
	verts[1].m_Vec.z = 0.0f;
	verts[1].color = theColor.color;

	verts[2].m_Vec.x = (float)g_ViewParams.m_Rect.right;
	verts[2].m_Vec.y = (float)g_ViewParams.m_Rect.bottom;
	verts[2].m_Vec.z = 0.0f;
	verts[2].color = theColor.color;

	verts[3].m_Vec.x = (float)g_ViewParams.m_Rect.left;
	verts[3].m_Vec.y = (float)g_ViewParams.m_Rect.bottom;
	verts[3].m_Vec.z = 0.0f;
	verts[3].color = theColor.color;

	d3d_UnsetTexture(g_iCurTextureStage);

	StateSet ssFogEnable(D3DRENDERSTATE_FOGENABLE, FALSE);
	StateSet ssZEnable(D3DRENDERSTATE_ZENABLE, FALSE);
	StateSet ssZWriteEnable(D3DRENDERSTATE_ZWRITEENABLE, FALSE);
	StateSet ssAlphaBlendEnable(D3DRENDERSTATE_ALPHABLENDENABLE, TRUE);
	StateSet ssSrcBlend(D3DRENDERSTATE_SRCBLEND, D3DBLEND_ONE);
	StateSet ssDestBlend(D3DRENDERSTATE_DESTBLEND, D3DBLEND_ONE);

	g_pD3DDevice->DrawPrimitive(D3DPT_TRIANGLEFAN, D3DFVF_TLVERTEX, verts, 4, 0);
}

// 0x10015cc0
void d3d_DrawLightScalePoly(const LTVector* pScale)
{
	static QuadVerts quad;
	TLVertex* verts = quad.m_Verts;
	RGBColor theColor;
	float fScale;

	if (pScale->x > 0.99f && pScale->y > 0.99f && pScale->z > 0.99f &&
		pScale->x < 1.01f && pScale->y < 1.01f && pScale->z < 1.01f && !g_CV_InvertHack)
		return;

	fScale = g_CV_InvertHack ? 255.0f : 127.5f;
	theColor.rgb.r = (uint8)(pScale->x * fScale);
	theColor.rgb.g = (uint8)(pScale->y * fScale);
	theColor.rgb.b = (uint8)(pScale->z * fScale);
	theColor.rgb.a = 0xFF;

	verts[0].m_Vec.x = (float)g_ViewParams.m_Rect.left;
	verts[0].m_Vec.y = (float)g_ViewParams.m_Rect.top;
	verts[0].m_Vec.z = 0.0f;
	verts[0].color = theColor.color;

	verts[1].m_Vec.x = (float)g_ViewParams.m_Rect.right;
	verts[1].m_Vec.y = (float)g_ViewParams.m_Rect.top;
	verts[1].m_Vec.z = 0.0f;
	verts[1].color = theColor.color;

	verts[2].m_Vec.x = (float)g_ViewParams.m_Rect.right;
	verts[2].m_Vec.y = (float)g_ViewParams.m_Rect.bottom;
	verts[2].m_Vec.z = 0.0f;
	verts[2].color = theColor.color;

	verts[3].m_Vec.x = (float)g_ViewParams.m_Rect.left;
	verts[3].m_Vec.y = (float)g_ViewParams.m_Rect.bottom;
	verts[3].m_Vec.z = 0.0f;
	verts[3].color = theColor.color;

	d3d_UnsetTexture(g_iCurTextureStage);

	StateSet ssFogEnable(D3DRENDERSTATE_FOGENABLE, FALSE);
	StateSet ssZEnable(D3DRENDERSTATE_ZENABLE, FALSE);
	StateSet ssZWriteEnable(D3DRENDERSTATE_ZWRITEENABLE, FALSE);
	StateSet ssAlphaBlendEnable(D3DRENDERSTATE_ALPHABLENDENABLE, TRUE);
	StateSet ssSrcBlend(D3DRENDERSTATE_SRCBLEND, g_CV_InvertHack ? D3DBLEND_INVDESTCOLOR : D3DBLEND_DESTCOLOR);
	StateSet ssDestBlend(D3DRENDERSTATE_DESTBLEND, g_CV_InvertHack ? D3DBLEND_ZERO : D3DBLEND_SRCCOLOR);

	g_pD3DDevice->DrawPrimitive(D3DPT_TRIANGLEFAN, D3DFVF_TLVERTEX, verts, 4, 0);
}



// ---------------------------------------------------------------------- //
//  The portals' polygons.
// ---------------------------------------------------------------------- //

// A texture in video memory (d3d_texture.cpp's), with what binding reads.
class RTexture
{
public:
	uint8                   m_Reserved00[0x0c];
	LPDIRECTDRAWSURFACE7    m_pSurface;                     // +0x0c
	uint8                   m_Reserved10[0x30 - 0x10];
	RTexture*               m_pNext;                        // +0x30
	uint8                   m_Reserved34[0x42 - 0x34];
	uint16                  m_Stage;                        // +0x42
	uint16                  m_LOD;                          // +0x44
};

struct SharedTexture
{
	uint8       m_Reserved00[0x0c];
	RTexture*   m_pRenderData;                              // +0x0c
	uint8       m_Reserved10[0x30 - 0x10];
	uint16      m_FrameCode;                                // +0x30
};

extern RTexture*    g_pCurTextures[];                       // 0x10067270
extern uint16       g_CurTextureFrameCode;                  // 0x1005d250
struct TexPair
{
	float   u, v;
};

extern TexPair      g_TexelSize[];                          // 0x100672a8 (d3d_texture.cpp)
extern void         (__fastcall *g_pfnCalcFog)(const LTVector* pPos, uint32* pSpecular);  // 0x1005e1c4

RTexture*   d3d_CreateRenderTexture(SharedTexture* pTexture, uint32 iStage, LTBOOL bStageCopy);  // 0x10020cd0
void        d3d_SetTextureDirect(RTexture* pTexture);       // 0x10007a89

// Bind a shared texture to a stage at a mip level (drawmodel_clip.cpp's
// d3d_SetTexture, 0x100079e4, spelt inline).
inline int d3d_BindTexture(SharedTexture* pTexture, uint32 iStage, uint32 iLOD)
{
	RTexture *pRenderData, *pCur;

	if (!pTexture)
		return LTFALSE;

	pRenderData = pTexture->m_pRenderData;
	pTexture->m_FrameCode = g_CurTextureFrameCode;

	for (pCur = pRenderData; pCur; pCur = pCur->m_pNext)
	{
		if (pCur->m_Stage == (uint16)(uint8)iStage)
			break;
	}

	if (pCur)
	{
		if (pCur != g_pCurTextures[iStage])
			d3d_SetTextureDirect(pCur);
	}
	else if (pRenderData)
	{
		pCur = d3d_CreateRenderTexture(pTexture, iStage, LTTRUE);
		if (!pCur)
			return LTFALSE;

		pCur->m_pNext = pRenderData->m_pNext;
		pRenderData->m_pNext = pCur;
		d3d_SetTextureDirect(pCur);
	}
	else
	{
		pCur = d3d_CreateRenderTexture(pTexture, iStage, LTFALSE);
		if (!pCur)
			return LTFALSE;

		d3d_SetTextureDirect(pCur);
	}

	if (pCur->m_LOD != iLOD)
	{
		pCur->m_pSurface->SetLOD(iLOD);
		pCur->m_LOD = (uint16)iLOD;
	}

	return LTTRUE;
}

#define MAX_PORTALPOLY_VERTS    128

// 0x10016030 -- draw a portal's polygon blended over the scene, its
// vertices through the portal's transform.
void d3d_DrawPortalPoly(RenderPoly* pPoly, LTMatrix* pTransform)
{
	TLVertex verts[MAX_PORTALPOLY_VERTS];
	TLVertex *pVerts, *pVert;
	RenderPolyVert* pPolyVerts;
	int nVerts;
	uint32 i;
	RenderState state;

	if (!d3d_BindTexture(pPoly->m_pSurface->m_pTexture, g_iCurTextureStage, 0))
		return;

	CRenderStateList states;

	state.m_State = D3DRENDERSTATE_ALPHABLENDENABLE;
	state.m_Val = TRUE;
	states.SetState(&state);

	d3d_GetPolyVerts(pPoly, &pPolyVerts, &nVerts);

	pVert = verts;
	for (i = nVerts; i; i--)
	{
		MatVMul_H(&pVert->m_Vec, pTransform, &pPolyVerts->m_pVert->m_Vec);
		pVert->color = 0xFFFFFFFF;
		pVert->SetTexCoords(pPolyVerts->tu * g_TexelSize[0].u, pPolyVerts->tv * g_TexelSize[0].v);
		g_pfnCalcFog(&pPolyVerts->m_pVert->m_Vec, &pVert->specular);
		pVert++;
		pPolyVerts++;
	}

	g_ClipFlags = 0x3F;
	pVerts = verts;
	if (d3d_ClipTLVerts(&pVerts, &nVerts, &g_ViewParams, 0))
	{
		g_nTrianglesDrawn += nVerts - 2;
		g_pD3DDevice->DrawPrimitive(D3DPT_TRIANGLEFAN, D3DFVF_TLVERTEX, pVerts, nVerts, 0);
	}
}

extern SceneDesc*   g_pSceneDesc;                           // 0x1005c14c (common_draw.cpp)

// What the portal pass reads of the frame's set-up (common_draw.cpp).
struct ViewBox
{
	float       m_fOffX;                                    // +0x00
	float       m_fOffY;                                    // +0x04
	float       m_fFocal;                                   // +0x08
	float       m_fHalfW;                                   // +0x0c
	float       m_fHalfH;                                   // +0x10
	float       m_fNear;                                    // +0x14
	float       m_fFar;                                     // +0x18
};

struct RenderWorld
{
	uint8       m_Reserved00[0x0c];
	uint16      m_FrameCode;                                // +0x0c
};

extern CD3DConVar   g_CV_NearZ;                             // 0x1005b748 (d3d_viewparams.cpp)
extern uint16       g_CurFrameCode;                         // 0x1005d238
extern uint32       g_nFrameTime;                           // 0x1005bc88
struct ProjectParams
{
	float   m_fScaleX;                                      // +0x31c
	float   m_fCenterX;                                     // +0x320
	float   m_fScaleY;                                      // +0x324
	float   m_fCenterY;                                     // +0x328
	float   m_fScaleZ;                                      // +0x32c
	float   m_fOffsetZ;                                     // +0x330
};

extern void         (*g_pfnPostQueueHook)();                // 0x10072808
extern int32        g_CV_LightMap;                          // 0x1004a790

void    d3d_IncFrameCode(RenderWorld* pWorld);              // 0x10010160 (common_draw.cpp)
void    d3d_InitViewBox(ViewBox* pBox, float fNearZ, float fFarZ, ViewParams* pPrev,
	float fMinX, float fMinY, float fMaxX, float fMaxY);    // 0x10010383
int32   d3d_InitSceneContext(ViewParams* pParams, ViewBox* pBox,
	float fMinX, float fMinY, float fMaxX, float fMaxY,
	LTMatrix* pCamera, LTVector vScale);                    // 0x100104cb

// Whether the scene through a portal is clipped to the portal's polygon
// by the stencil buffer.
int32   g_bPortalStencilClip = 1;                           // 0x1004ac60

// 0x100162a0 -- draw the scene through each visible portal (a mirror):
// mark the portal's polygon in the stencil buffer and push the depth
// buffer to the far plane there, then draw the scene again from the
// camera reflected in the portal's plane, clipped to the polygon's
// screen box, and blend the portal's own polygon over it.
void d3d_DrawPortals(SceneDesc* pDesc, VisibleSet* pVisibleSet)
{
	ViewParams savedParams;
	PortalRef portals[MAX_VISIBLE_PORTALS];
	TLVertex verts[40];
	TLVertex* pVerts;
	RenderPolyVert* pPolyVerts;
	int nVerts, i;
	uint32 nPortals, iPortal;
	RenderPoly* pPoly;
	RenderSurface* pSurface;
	LTMatrix* pTransform;
	RenderWorld* pWorld;
	LTPlane plane;
	LTVector vOrigin;
	LTMatrix mCamera, mReflect, mMirrorCamera, mScene;
	ViewBox viewBox;
	float fMinX, fMinY, fMinZ, fMaxX, fMaxY, fZ, fRHW;
	uint32 color;
	int32 nSaveLightMap;

	// The scene through a portal draws no portals itself.
	g_pfnPostQueueHook = LTNULL;

	nPortals = pVisibleSet->m_nPortals;
	if (!nPortals)
		return;

	memcpy(portals, pVisibleSet->m_Portals, nPortals * sizeof(PortalRef));

	pWorld = pDesc->m_pWorld;
	for (iPortal = 0; iPortal < nPortals; iPortal++)
	{
		pPoly = portals[iPortal].m_pPoly;
		pTransform = portals[iPortal].m_pTransform;
		pSurface = pPoly->m_pSurface;
		if ((pSurface->m_TextureFlags & TEXFLAG_PORTALMASK) != TEXFLAG_PORTAL)
			continue;

		// Only from the portal's front.
		plane = pTransform->TransformPlane(*pPoly->m_pPlane);
		if (plane.m_Normal.Dot(g_ViewParams.m_Pos) - plane.m_Dist < 0.0f)
			continue;

		// The camera, reflected in the portal's plane.
		mCamera.Identity();
		mCamera.SetBasisVectors(&g_ViewParams.m_vRight, &g_ViewParams.m_vUp, &g_ViewParams.m_vForward);
		mCamera.SetTranslation(g_ViewParams.m_Pos);

		vOrigin = plane.m_Normal * plane.m_Dist;
		mReflect.SetupReflectionMatrix(plane.m_Normal, vOrigin);
		MatMul(&mMirrorCamera, &mReflect, &mCamera);
		mScene = mMirrorCamera;

		d3d_IncFrameCode(pWorld);
		g_CurFrameCode = pWorld->m_FrameCode;
		g_nFrameTime = g_pStruct->GetTime();

		// Mark the portal's polygon in the stencil buffer.
		g_pD3DDevice->SetRenderState(D3DRENDERSTATE_STENCILENABLE, TRUE);
		g_pD3DDevice->SetRenderState(D3DRENDERSTATE_STENCILPASS, D3DSTENCILOP_REPLACE);
		g_pD3DDevice->SetRenderState(D3DRENDERSTATE_STENCILFUNC, D3DCMP_ALWAYS);
		g_pD3DDevice->SetRenderState(D3DRENDERSTATE_STENCILREF, 1);

		g_ClipFlags = 0x3f;

		color = (uint32)pPoly;
		if (!g_CV_ShowSplits)
			color = (uint32)pPoly->m_pSurface;

		d3d_DisableTexture(g_iCurTextureStage);
		d3d_GetPolyVerts(pPoly, &pPolyVerts, &nVerts);
		for (i = 0; i < nVerts; i++)
		{
			d3d_MatVMul_H(&verts[i].m_Vec, pTransform, &pPolyVerts[i].m_pVert->m_Vec);
			verts[i].color = color;
			verts[i].specular = 0xFFFFFFFF;
		}

		g_nTrianglesDrawn += nVerts - 2;
		pVerts = verts;
		if (d3d_DrawTLPoly(&pVerts, &nVerts))
		{
			g_pD3DDevice->SetRenderState(D3DRENDERSTATE_STENCILREF, 1);
			g_pD3DDevice->SetRenderState(D3DRENDERSTATE_STENCILPASS, D3DSTENCILOP_KEEP);
			g_pD3DDevice->SetRenderState(D3DRENDERSTATE_STENCILFUNC, D3DCMP_EQUAL);

			// Push the depth buffer back to the far plane over the
			// polygon, and find its screen box.
			fZ = g_ViewParams.m_FarZ - 1.0f;
			fRHW = 1.0f / fZ;
			fZ = (fZ * g_ViewParams.m_fProjScaleZ + g_ViewParams.m_fProjOffsetZ) * fRHW;

			fMinX = fMinY = fMinZ = 100000.0f;
			fMaxX = fMaxY = -100000.0f;
			for (i = 0; i < nVerts; i++)
			{
				pVerts[i].color = 0x00FF0000;
				pVerts[i].rhw = fRHW;
				pVerts[i].m_Vec.z = fZ;

				if (pVerts[i].m_Vec.x < fMinX)
					fMinX = pVerts[i].m_Vec.x;
				if (pVerts[i].m_Vec.y < fMinY)
					fMinY = pVerts[i].m_Vec.y;
				if (fZ < fMinZ)
					fMinZ = fZ;
				if (pVerts[i].m_Vec.x > fMaxX)
					fMaxX = pVerts[i].m_Vec.x;
				if (pVerts[i].m_Vec.y > fMaxY)
					fMaxY = pVerts[i].m_Vec.y;
			}

			g_pD3DDevice->SetRenderState(D3DRENDERSTATE_ZENABLE, TRUE);
			g_pD3DDevice->SetRenderState(D3DRENDERSTATE_ZWRITEENABLE, TRUE);
			g_pD3DDevice->SetRenderState(D3DRENDERSTATE_ZFUNC, D3DCMP_ALWAYS);
			g_pD3DDevice->DrawPrimitive(D3DPT_TRIANGLEFAN, D3DFVF_TLVERTEX, pVerts, nVerts, 0);
			g_pD3DDevice->SetRenderState(D3DRENDERSTATE_ZFUNC, D3DCMP_LESSEQUAL);

			if (!g_bPortalStencilClip || g_CV_ShowPortalBounds.m_nValue)
			{
				g_pD3DDevice->SetRenderState(D3DRENDERSTATE_STENCILENABLE, FALSE);
				g_pD3DDevice->SetRenderState(D3DRENDERSTATE_STENCILFUNC, D3DCMP_EQUAL);
			}

			// The scene from the reflected camera, in the polygon's box.
			savedParams = g_ViewParams;

			d3d_InitViewBox(&viewBox, g_CV_NearZ.m_fValue, pDesc->m_FarZ, &g_ViewParams,
				fMinX, fMinY, fMaxX, fMaxY);
			d3d_InitSceneContext(&g_ViewParams, &viewBox, fMinX, fMinY, fMaxX, fMaxY,
				&mScene, LTVector(1.0f, 1.0f, 1.0f));

			g_ViewParams.m_vPortalPos = pPoly->m_vCenter;
			g_ViewParams.m_bMirrorView = !savedParams.m_bMirrorView;
			g_ViewParams.m_fMirrorSign = g_ViewParams.m_bMirrorView ? -1.0f : 1.0f;
			g_ViewParams.m_bInPortal = LTTRUE;

			nSaveLightMap = g_CV_LightMap;
			if (!g_CV_PortalLightmap.m_nValue)
				g_CV_LightMap = 0;

			d3d_FullDrawScene(pDesc);

			g_CV_LightMap = nSaveLightMap;
			g_ViewParams = savedParams;

			// The portal's own polygon over it.
			g_pD3DDevice->SetRenderState(D3DRENDERSTATE_ZFUNC, D3DCMP_ALWAYS);
			if (pSurface->m_TextureFlags & TEXFLAG_DRAWPORTAL)
				d3d_DrawPortalPoly(pPoly, portals[iPortal].m_pTransform);
			g_pD3DDevice->SetRenderState(D3DRENDERSTATE_ZFUNC, D3DCMP_LESSEQUAL);
		}

		g_pD3DDevice->SetRenderState(D3DRENDERSTATE_STENCILENABLE, FALSE);
	}
}

// 0x10016c40 -- RenderScene's portal pass: the frame's visible set
// through the portals.
void d3d_DrawVisiblePortals()
{
	d3d_DrawPortals(g_pSceneDesc, d3d_GetVisibleSet());
}




// ---------------------------------------------------------------------- //
//  Debug lines and boxes.
// ---------------------------------------------------------------------- //


// A clipped line end's texture coordinates, colour and specular alpha,
// interpolated from a to b (d3d_clip.h's LerpClipVert over a TLVertex).
inline void d3d_LerpLineVert(TLVertex* a, TLVertex* b, TLVertex* dst, float t)
{
	dst->tu = (b->tu - a->tu) * t + a->tu;
	dst->tv = (b->tv - a->tv) * t + a->tv;
	((uint8*)&dst->color)[2]    = (uint8)RoundFloatToInt((((uint8*)&b->color)[2]    - ((uint8*)&a->color)[2])    * t + ((uint8*)&a->color)[2]);
	((uint8*)&dst->color)[1]    = (uint8)RoundFloatToInt((((uint8*)&b->color)[1]    - ((uint8*)&a->color)[1])    * t + ((uint8*)&a->color)[1]);
	((uint8*)&dst->color)[0]    = (uint8)RoundFloatToInt((((uint8*)&b->color)[0]    - ((uint8*)&a->color)[0])    * t + ((uint8*)&a->color)[0]);
	((uint8*)&dst->color)[3]    = (uint8)RoundFloatToInt((((uint8*)&b->color)[3]    - ((uint8*)&a->color)[3])    * t + ((uint8*)&a->color)[3]);
	((uint8*)&dst->specular)[3] = (uint8)RoundFloatToInt((((uint8*)&b->specular)[3] - ((uint8*)&a->specular)[3]) * t + ((uint8*)&a->specular)[3]);
}

// ---------------------------------------------------------------------- //
//  0x10016c60 -- clip the line pVerts[0]..pVerts[1] (camera space) in
//  place against the planes clipFlags selects: 0x01 near, 0x04 left,
//  0x08 top, 0x10 right, 0x20 bottom, 0x02 far, tested in that order.
//  FALSE when the line is wholly outside one of them.
// ---------------------------------------------------------------------- //
LTBOOL d3d_ClipLine(TLVertex* pVerts, uint32 clipFlags)
{
	TLVertex*   v = pVerts;
	float       t;

	if (clipFlags & 1)
	{
		if (v[0].m_Vec.z >= g_ViewParams.m_NearZ)
		{
			if (v[1].m_Vec.z < g_ViewParams.m_NearZ)
			{
				t = (g_ViewParams.m_NearZ - v[0].m_Vec.z) / (v[1].m_Vec.z - v[0].m_Vec.z);
				v[1].m_Vec.x = (v[1].m_Vec.x - v[0].m_Vec.x) * t + v[0].m_Vec.x;
				v[1].m_Vec.y = (v[1].m_Vec.y - v[0].m_Vec.y) * t + v[0].m_Vec.y;
				v[1].m_Vec.z = g_ViewParams.m_NearZ;
				d3d_LerpLineVert(&v[0], &v[1], &v[1], t);
			}
		}
		else
		{
			if (v[1].m_Vec.z >= g_ViewParams.m_NearZ)
			{
				t = (g_ViewParams.m_NearZ - v[0].m_Vec.z) / (v[1].m_Vec.z - v[0].m_Vec.z);
				v[0].m_Vec.x = (v[1].m_Vec.x - v[0].m_Vec.x) * t + v[0].m_Vec.x;
				v[0].m_Vec.y = (v[1].m_Vec.y - v[0].m_Vec.y) * t + v[0].m_Vec.y;
				v[0].m_Vec.z = g_ViewParams.m_NearZ;
				d3d_LerpLineVert(&v[0], &v[1], &v[0], t);
			}
			else
				return FALSE;
		}
	}

	if (clipFlags & 4)
	{
		if (-v[0].m_Vec.z < v[0].m_Vec.x)
		{
			if (-v[1].m_Vec.z >= v[1].m_Vec.x)
			{
				t = -(v[0].m_Vec.z + v[0].m_Vec.x) / ((v[1].m_Vec.x - v[0].m_Vec.x) + v[1].m_Vec.z - v[0].m_Vec.z);
				v[1].m_Vec.y = (v[1].m_Vec.y - v[0].m_Vec.y) * t + v[0].m_Vec.y;
				v[1].m_Vec.z = (v[1].m_Vec.z - v[0].m_Vec.z) * t + v[0].m_Vec.z;
				v[1].m_Vec.x = -v[1].m_Vec.z;
				d3d_LerpLineVert(&v[0], &v[1], &v[1], t);
			}
		}
		else
		{
			if (-v[1].m_Vec.z < v[1].m_Vec.x)
			{
				t = -(v[0].m_Vec.z + v[0].m_Vec.x) / ((v[1].m_Vec.x - v[0].m_Vec.x) + v[1].m_Vec.z - v[0].m_Vec.z);
				v[0].m_Vec.y = (v[1].m_Vec.y - v[0].m_Vec.y) * t + v[0].m_Vec.y;
				v[0].m_Vec.z = (v[1].m_Vec.z - v[0].m_Vec.z) * t + v[0].m_Vec.z;
				v[0].m_Vec.x = -v[0].m_Vec.z;
				d3d_LerpLineVert(&v[0], &v[1], &v[0], t);
			}
			else
				return FALSE;
		}
	}

	if (clipFlags & 8)
	{
		if (v[0].m_Vec.y < v[0].m_Vec.z)
		{
			if (v[1].m_Vec.y >= v[1].m_Vec.z)
			{
				t = -(v[0].m_Vec.y - v[0].m_Vec.z) / ((v[1].m_Vec.y - v[0].m_Vec.y) - v[1].m_Vec.z + v[0].m_Vec.z);
				v[1].m_Vec.x = (v[1].m_Vec.x - v[0].m_Vec.x) * t + v[0].m_Vec.x;
				v[1].m_Vec.z = (v[1].m_Vec.z - v[0].m_Vec.z) * t + v[0].m_Vec.z;
				v[1].m_Vec.y = v[1].m_Vec.z;
				d3d_LerpLineVert(&v[0], &v[1], &v[1], t);
			}
		}
		else
		{
			if (v[1].m_Vec.y < v[1].m_Vec.z)
			{
				t = -(v[0].m_Vec.y - v[0].m_Vec.z) / ((v[1].m_Vec.y - v[0].m_Vec.y) - v[1].m_Vec.z + v[0].m_Vec.z);
				v[0].m_Vec.x = (v[1].m_Vec.x - v[0].m_Vec.x) * t + v[0].m_Vec.x;
				v[0].m_Vec.z = (v[1].m_Vec.z - v[0].m_Vec.z) * t + v[0].m_Vec.z;
				v[0].m_Vec.y = v[0].m_Vec.z;
				d3d_LerpLineVert(&v[0], &v[1], &v[0], t);
			}
			else
				return FALSE;
		}
	}

	if (clipFlags & 0x10)
	{
		if (v[0].m_Vec.x < v[0].m_Vec.z)
		{
			if (v[1].m_Vec.x >= v[1].m_Vec.z)
			{
				t = -(v[0].m_Vec.x - v[0].m_Vec.z) / ((v[1].m_Vec.x - v[0].m_Vec.x) - v[1].m_Vec.z + v[0].m_Vec.z);
				v[1].m_Vec.y = (v[1].m_Vec.y - v[0].m_Vec.y) * t + v[0].m_Vec.y;
				v[1].m_Vec.z = (v[1].m_Vec.z - v[0].m_Vec.z) * t + v[0].m_Vec.z;
				v[1].m_Vec.x = v[1].m_Vec.z;
				d3d_LerpLineVert(&v[0], &v[1], &v[1], t);
			}
		}
		else
		{
			if (v[1].m_Vec.x < v[1].m_Vec.z)
			{
				t = -(v[0].m_Vec.x - v[0].m_Vec.z) / ((v[1].m_Vec.x - v[0].m_Vec.x) - v[1].m_Vec.z + v[0].m_Vec.z);
				v[0].m_Vec.y = (v[1].m_Vec.y - v[0].m_Vec.y) * t + v[0].m_Vec.y;
				v[0].m_Vec.z = (v[1].m_Vec.z - v[0].m_Vec.z) * t + v[0].m_Vec.z;
				v[0].m_Vec.x = v[0].m_Vec.z;
				d3d_LerpLineVert(&v[0], &v[1], &v[0], t);
			}
			else
				return FALSE;
		}
	}

	if (clipFlags & 0x20)
	{
		if (-v[0].m_Vec.z < v[0].m_Vec.y)
		{
			if (-v[1].m_Vec.z >= v[1].m_Vec.y)
			{
				t = -(v[0].m_Vec.z + v[0].m_Vec.y) / ((v[1].m_Vec.y - v[0].m_Vec.y) + v[1].m_Vec.z - v[0].m_Vec.z);
				v[1].m_Vec.x = (v[1].m_Vec.x - v[0].m_Vec.x) * t + v[0].m_Vec.x;
				v[1].m_Vec.z = (v[1].m_Vec.z - v[0].m_Vec.z) * t + v[0].m_Vec.z;
				v[1].m_Vec.y = -v[1].m_Vec.z;
				d3d_LerpLineVert(&v[0], &v[1], &v[1], t);
			}
		}
		else
		{
			if (-v[1].m_Vec.z < v[1].m_Vec.y)
			{
				t = -(v[0].m_Vec.z + v[0].m_Vec.y) / ((v[1].m_Vec.y - v[0].m_Vec.y) + v[1].m_Vec.z - v[0].m_Vec.z);
				v[0].m_Vec.x = (v[1].m_Vec.x - v[0].m_Vec.x) * t + v[0].m_Vec.x;
				v[0].m_Vec.z = (v[1].m_Vec.z - v[0].m_Vec.z) * t + v[0].m_Vec.z;
				v[0].m_Vec.y = -v[0].m_Vec.z;
				d3d_LerpLineVert(&v[0], &v[1], &v[0], t);
			}
			else
				return FALSE;
		}
	}

	if (clipFlags & 2)
	{
		if (v[0].m_Vec.z <= g_ViewParams.m_FarZ2)
		{
			if (v[1].m_Vec.z > g_ViewParams.m_FarZ2)
			{
				t = (g_ViewParams.m_FarZ2 - v[0].m_Vec.z) / (v[1].m_Vec.z - v[0].m_Vec.z);
				v[1].m_Vec.x = (v[1].m_Vec.x - v[0].m_Vec.x) * t + v[0].m_Vec.x;
				v[1].m_Vec.y = (v[1].m_Vec.y - v[0].m_Vec.y) * t + v[0].m_Vec.y;
				v[1].m_Vec.z = g_ViewParams.m_FarZ2;
				d3d_LerpLineVert(&v[0], &v[1], &v[1], t);
			}
		}
		else
		{
			if (v[1].m_Vec.z <= g_ViewParams.m_FarZ2)
			{
				t = (g_ViewParams.m_FarZ2 - v[0].m_Vec.z) / (v[1].m_Vec.z - v[0].m_Vec.z);
				v[0].m_Vec.x = (v[1].m_Vec.x - v[0].m_Vec.x) * t + v[0].m_Vec.x;
				v[0].m_Vec.y = (v[1].m_Vec.y - v[0].m_Vec.y) * t + v[0].m_Vec.y;
				v[0].m_Vec.z = g_ViewParams.m_FarZ2;
				d3d_LerpLineVert(&v[0], &v[1], &v[0], t);
			}
			else
				return FALSE;
		}
	}

	return TRUE;
}

// 0x10017bd0
void d3d_DrawLine(LTVector vStart, LTVector vEnd, uint32 color0, uint32 color1)
{
	TLVertex verts[2];

	verts[0].color = color0;
	verts[0].specular = 0xFFFFFFFF;
	MatVMul_H(&verts[0].m_Vec, &g_ViewParams.m_mWorldToCamera, &vStart);

	verts[1].color = color1;
	verts[1].specular = 0xFFFFFFFF;
	MatVMul_H(&verts[1].m_Vec, &g_ViewParams.m_mWorldToCamera, &vEnd);

	if (d3d_ClipLine(verts, 0x3F))
	{
		verts[0].rhw = 1.0f / verts[0].m_Vec.z;
		verts[0].m_Vec.x = (verts[0].m_Vec.x * g_ViewParams.m_fProjScaleX + verts[0].m_Vec.z * g_ViewParams.m_fProjCenterX) * verts[0].rhw;
		verts[0].m_Vec.y = (verts[0].m_Vec.y * g_ViewParams.m_fProjScaleY + verts[0].m_Vec.z * g_ViewParams.m_fProjCenterY) * verts[0].rhw;
		verts[0].m_Vec.z = (verts[0].m_Vec.z * g_ViewParams.m_fProjScaleZ + g_ViewParams.m_fProjOffsetZ) * verts[0].rhw;

		verts[1].rhw = 1.0f / verts[1].m_Vec.z;
		verts[1].m_Vec.x = (verts[1].m_Vec.x * g_ViewParams.m_fProjScaleX + verts[1].m_Vec.z * g_ViewParams.m_fProjCenterX) * verts[1].rhw;
		verts[1].m_Vec.y = (verts[1].m_Vec.y * g_ViewParams.m_fProjScaleY + verts[1].m_Vec.z * g_ViewParams.m_fProjCenterY) * verts[1].rhw;
		verts[1].m_Vec.z = (verts[1].m_Vec.z * g_ViewParams.m_fProjScaleZ + g_ViewParams.m_fProjOffsetZ) * verts[1].rhw;

		g_pD3DDevice->DrawPrimitive(D3DPT_LINELIST, D3DFVF_TLVERTEX, verts, 2, 0);
	}
}

// 0x10017e60 -- the four edges of the bottom face, the four of the top
// face, then the four uprights.
void d3d_DrawWireframeBox(const LTVector* pMin, const LTVector* pMax, uint32 color)
{
	d3d_DrawLine(LTVector(pMin->x, pMin->y, pMin->z), LTVector(pMax->x, pMin->y, pMin->z), color, color);
	d3d_DrawLine(LTVector(pMin->x, pMin->y, pMin->z), LTVector(pMin->x, pMin->y, pMax->z), color, color);
	d3d_DrawLine(LTVector(pMax->x, pMin->y, pMin->z), LTVector(pMax->x, pMin->y, pMax->z), color, color);
	d3d_DrawLine(LTVector(pMin->x, pMin->y, pMax->z), LTVector(pMax->x, pMin->y, pMax->z), color, color);

	d3d_DrawLine(LTVector(pMin->x, pMax->y, pMin->z), LTVector(pMax->x, pMax->y, pMin->z), color, color);
	d3d_DrawLine(LTVector(pMin->x, pMax->y, pMin->z), LTVector(pMin->x, pMax->y, pMax->z), color, color);
	d3d_DrawLine(LTVector(pMax->x, pMax->y, pMin->z), LTVector(pMax->x, pMax->y, pMax->z), color, color);
	d3d_DrawLine(LTVector(pMin->x, pMax->y, pMax->z), LTVector(pMax->x, pMax->y, pMax->z), color, color);

	d3d_DrawLine(LTVector(pMin->x, pMin->y, pMin->z), LTVector(pMin->x, pMax->y, pMin->z), color, color);
	d3d_DrawLine(LTVector(pMax->x, pMin->y, pMin->z), LTVector(pMax->x, pMax->y, pMin->z), color, color);
	d3d_DrawLine(LTVector(pMax->x, pMin->y, pMax->z), LTVector(pMax->x, pMax->y, pMax->z), color, color);
	d3d_DrawLine(LTVector(pMin->x, pMin->y, pMax->z), LTVector(pMin->x, pMax->y, pMax->z), color, color);
}


// One world-tree node: its box and its four children.
struct WorldTreeNode
{
	uint8           m_Reserved00[0x18];
	LTVector        m_BBoxMin;                              // +0x18
	LTVector        m_BBoxMax;                              // +0x24
	uint8           m_Reserved30[0x4c - 0x30];
	WorldTreeNode*  m_Children[4];                          // +0x4c
};

// 0x100183b0
void d3d_DrawWorldTree_R(WorldTreeNode* pNode, uint32 nDepth, uint32 nMaxDepth)
{
	uint32 i;

	if (nDepth > nMaxDepth)
		return;

	d3d_DrawWireframeBox(&pNode->m_BBoxMin, &pNode->m_BBoxMax, 0xFFFFFFFF);

	if (pNode->m_Children[0])
	{
		for (i = 0; i < 4; i++)
			d3d_DrawWorldTree_R(pNode->m_Children[i], nDepth + 1, nMaxDepth);
	}
}


// The terrain sections' boxes.
struct TerrainSection
{
	uint8   m_Reserved00[0x48];
	uint8   m_Key[0xc0 - 0x48];                             // +0x48
};

struct TerrainBox
{
	uint8       m_Reserved00[0x18];
	LTVector    m_vMin;                                     // +0x18
	LTVector    m_vMax;                                     // +0x24
};

class TerrainModel
{
public:
	virtual LTBOOL  IsHidden();                             // slot 0

	uint8           m_Reserved04[0x13c - 4];
	TerrainSection* m_pSections;                            // +0x13c
	uint32          m_nSections;                            // +0x140
};

struct TerrainModelRef
{
	uint32          m_Reserved00;
	TerrainModel*   m_pModel;                               // +0x04
};

class TerrainLookup
{
public:
	virtual TerrainBox* Find(void* pKey);                   // slot 0
};

struct TerrainWorld
{
	uint8               m_Reserved000[0x6c];
	TerrainLookup       m_Lookup;                           // +0x06c
	uint8               m_Reserved070[0x18c - 0x70];
	TerrainModelRef**   m_pModels;                          // +0x18c
	uint32              m_nModels;                          // +0x190
};

// 0x10018400 -- DrawTerrainSections.
void d3d_DrawTerrainSections(TerrainWorld* pWorld)
{
	uint32 i, j;
	TerrainModel* pModel;
	TerrainSection* pSection;
	TerrainBox* pBox;

	StateSet ssZEnable(D3DRENDERSTATE_ZENABLE, TRUE);

	d3d_UnsetTexture(g_iCurTextureStage);

	for (i = 0; i < pWorld->m_nModels; i++)
	{
		pModel = pWorld->m_pModels[i]->m_pModel;
		if (pModel->IsHidden())
			continue;

		for (j = 0; j < pModel->m_nSections; j++)
		{
			pSection = &pModel->m_pSections[j];
			pBox = pWorld->m_Lookup.Find(pSection->m_Key);
			if (pBox)
				d3d_DrawWireframeBox(&pBox->m_vMin, &pBox->m_vMax, (uint32)pSection + (uint32)pModel);
		}
	}
}


// ---------------------------------------------------------------------- //
//  RenderScene.
// ---------------------------------------------------------------------- //

// The world's tree: its root node, whose box the debug pass draws.
struct WorldTree
{
	uint8           m_Reserved00[0x0c];
	WorldTreeNode   m_RootNode;                             // +0x0c
};

struct RenderSceneWorld
{
	uint8       m_Reserved000[0x6c];
	WorldTree   m_WorldTree;                                // +0x06c
};

// The model drawer's environment-map scale and pan (drawmodel.cpp).
class CD3DModelDraw
{
public:
	uint8       m_Reserved000[0x61c];
	float       m_fEnvOffsetU;                              // +0x61c
	float       m_fEnvOffsetV;                              // +0x620
	float       m_fEnvScaleU;                               // +0x624
	float       m_fEnvScaleV;                               // +0x628
};

extern CD3DModelDraw    g_ModelDraw;                        // 0x100569b0

extern LPDIRECTDRAW7        g_pDirectDraw;                  // 0x1005d2a8
extern LPDIRECTDRAWSURFACE7 g_pPrimarySurface;              // 0x1005d2b0
extern LPDIRECTDRAWSURFACE7 g_pBackBuffer;                  // 0x1005d2b4
extern LPDIRECTDRAWSURFACE7 g_pZBuffer;                     // 0x1005d2b8
extern LTBOOL       g_bIn3D;                                // 0x100638d8
extern LTBOOL       g_bInOptimized2D;                       // 0x100638dc
extern uint32       g_bAnimateLightmaps;                    // 0x100628b0
extern void         (*g_pfnPostQueueHook)();                // 0x10072808
extern CD3DConVar   g_CV_TableFogVar;                       // 0x1005d2f0
extern float        g_CV_LightSaturate;                     // 0x1005d8b0
extern float        g_CV_WarbleScale;                       // 0x1005d8b4
extern float        g_CV_WarbleSpeed;                       // 0x1005dea0
extern float        g_CV_EnvPanSpeed;                       // 0x1004a72c
extern float        g_CV_EnvScale;                          // 0x1005d5a8
extern int32        g_CV_Saturate;                          // 0x1005d384
extern int32        g_CV_Wireframe;                         // 0x1005df64
extern int32        g_CV_ShowTextureCounts;                 // 0x1005df5c
extern int32        g_CV_ShowPolyCounts;                    // 0x1005df70
extern int32        g_CV_ModelProfile;                      // 0x1005da08
extern float        g_fVFogValScale;                        // 0x1005e210
extern float        g_fVFogRangeScale;                      // 0x1005df90
extern float        g_fVFogDensityScale;                    // 0x1005e20c
extern LTVector     g_vGlobalLightColor;                    // 0x1005b780
extern LTVector     g_vLightAdd;                            // 0x1005bc90
extern uint32       g_WorldColor;                           // 0x1005c154
uint8               g_GammaTableR[256];                     // 0x1005f79c
uint8               g_GammaTableG[256];                     // 0x1005f89c
uint8               g_GammaTableB[256];                     // 0x1005f99c
uint8               g_LightTableR[256];                     // 0x1005fa9c
uint8               g_LightTableG[256];                     // 0x1005fb9c
uint8               g_LightTableB[256];                     // 0x1005fc9c
struct LMBucket;
extern LMBucket*    g_LightmapPolyList;                     // 0x1005fda0 (drawlightmaps.cpp)
struct GouraudBucket;
extern GouraudBucket* g_GouraudPolyList;                   // 0x1005e728
uint32              g_nTLVertsUsed;                         // 0x1005e27c
uint32              g_nModelsClipped;                       // 0x1005e278
uint32              g_nModelsUnclipped;                     // 0x1005e774
extern float        g_fWarbleTime;                          // 0x100572b0 (drawmodel.cpp)
extern float        g_fWarbleFrac;                          // 0x100588a8 (drawmodel.cpp)
extern uint32       g_nTextureChanges;                      // 0x1005d22c
extern uint32       g_1005bd18;                             // 0x1005bd18 (texture uploads)
extern uint32       g_nWorldPoliesDrawn;                    // 0x1005c120
extern uint32       g_nDrawPrimCalls;                       // 0x1005c144
extern uint32       g_nSkyLightsApplied;                    // 0x1005c164
extern uint32       g_nLitObjects;                          // 0x1005bcac
extern uint32       g_nLitPolysUploaded;                             // 0x1005bd10
extern int32        g_nModelTriangles;                      // 0x1005bd04
extern uint32       g_nParticlesDrawn;                             // 0x1005b770 (particles drawn)
extern uint32       g_nClipPasses;                          // 0x1005c124
extern uint32       g_nVisibleLeaves;                       // 0x1005b78c
extern uint32       g_nSkyPortalPoliesDrawn;                // 0x1005c150
extern uint32       g_1005bd08;                             // 0x1005bd08
extern uint32       g_nDynamicLights;                       // 0x1005bcb0
extern uint32       g_nLightPolyRejects;                    // 0x1005c128
extern uint32       g_nLightmapUploadSaves;                 // 0x1005b774

void    dd_ConsolePrint(const char* pMsg, ...);             // 0x10013ade
void    AddDebugMessage(int32 debugLevel, const char* pMsg, ...);  // 0x10013b13
int32   d3d_InitFrame(SceneDesc* pDesc, void* pClipScratch, uint32 nScratch); // 0x10011162 (common_draw.cpp)
void    d3d_InitLMAnimRamp(float fScale);                   // 0x100337b0 (lightanim.cpp)
void    d3d_InitModelSun();                                 // 0x1000b67d (drawmodel.cpp)
#ifdef PRIMAL_HUNT
void    __fastcall d3d_CalcFogVolume(const LTVector* pPos, uint32* pSpecular);  // 0x1000fa33 (fogvolume.cpp)
void    d3d_ClearFogVolumes();                              // 0x1000fc43 (fogvolume.cpp)
#define d3d_CalcVFog    d3d_CalcFogVolume
#endif

void (__fastcall *g_pfnCalcFog)(const LTVector* pPos, uint32* pSpecular);       // 0x1005e1c4
void (__fastcall *g_pfnCalcSkyFog)(const LTVector* pPos, uint32* pSpecular);    // 0x1005e6d8
LTVector    g_vNegSunDir;                                   // 0x1005e288
extern uint32   g_LastWorldColor;                           // 0x1005fdc8
static int32    g_LastRenderToFront;                        // 0x1005fe10
static LTBOOL   g_bSaturateTableValid;                      // 0x1005df0c
static float    g_fSaturateTableLast;                       // 0x1005d72c
LTBOOL          g_bWarbleTableValid;                        // 0x1005df7c (common_init.cpp resets it)
static float    g_fWarbleScaleLast;                         // 0x1005d4f0
static float    g_fFogRange;                                // 0x1005d338

// No fog (table fog does it).
void __fastcall d3d_CalcNoFog(const LTVector* pPos, uint32* pSpecular)
{
}

// |a - b|.
#define FDIFF(a, b)     (((a) < (b)) ? ((b) - (a)) : ((a) - (b)))

// A colour ramp: 256 bytes of a 16.16 accumulator stepped by fScale,
// clamped at 255.
#define BUILD_RAMP(pTable, fScale) \
{ \
	uint8* pRamp = (pTable); \
	uint32 nStep = (uint32)((fScale) * 65536.0f); \
	uint32 nAccum = 0; \
	for (int i = 256; i; i--) \
	{ \
		*pRamp++ = (uint8)(nAccum >> 16); \
		nAccum += nStep; \
		if (nAccum > (255 << 16)) \
			nAccum = (255 << 16); \
	} \
}

// RenderScene: draw the frame's scene, with the per-frame tables and the
// debug statistics around it.
LTBOOL d3d_RenderScene(SceneDesc* pDesc)
{
	uint8 clipScratch[0x5000];
	CounterFinal cnt;

	if (!pDesc || !g_pPrimarySurface || !g_pD3DDevice || !g_pDirectDraw || !g_bIn3D)
		return LTFALSE;

	if (g_bInOptimized2D)
	{
		AddDebugMessage(0, "Error: tried to render 3D while in optimized 2D mode.");
		return LTFALSE;
	}

	// Swap the front and back buffers' roles when RenderToFront changes.
	if (g_CV_RenderToFront.m_nValue != g_LastRenderToFront)
	{
		LPDIRECTDRAWSURFACE7 pOldBack = g_pBackBuffer;

		g_pBackBuffer = g_pPrimarySurface;
		g_pPrimarySurface = pOldBack;

		g_pBackBuffer->AddAttachedSurface(g_pZBuffer);
		g_pPrimarySurface->DeleteAttachedSurface(0, g_pZBuffer);
		g_pD3DDevice->SetRenderTarget(g_pBackBuffer, 0);

		g_LastRenderToFront = g_CV_RenderToFront.m_nValue;
	}

	g_bAnimateLightmaps = g_CV_DrawPortals.m_nValue && d3d_HasStencil();

	if (!g_bSaturateTableValid || g_CV_LightSaturate != g_fSaturateTableLast)
	{
		d3d_InitLMAnimRamp(g_CV_LightSaturate);
		g_fSaturateTableLast = g_CV_LightSaturate;
		g_bSaturateTableValid = LTTRUE;
	}

	g_ModelDraw.m_fEnvOffsetU = pDesc->m_Pos.x * g_CV_EnvPanSpeed + 0.5f;
	g_ModelDraw.m_fEnvOffsetV = pDesc->m_Pos.z * g_CV_EnvPanSpeed + 0.5f;
	g_ModelDraw.m_fEnvScaleU = g_ModelDraw.m_fEnvScaleV = 1.0f / g_CV_EnvScale * 0.5f;

	g_nModelsClipped = 0;
	g_nModelsUnclipped = 0;

	LTVector vNegSunDir = -g_pStruct->m_vSunDir;
	g_vNegSunDir = vNegSunDir;

	if (d3d_InitFrame(pDesc, clipScratch, sizeof(clipScratch)))
	{
		// The fog functions.
		g_pfnCalcSkyFog = d3d_CalcSkyFog;
		g_fSkyFogScale = 1.0f / (g_CV_SkyFogFarZ - g_CV_SkyFogNearZ) * 255.0f;
		g_fFogRange = g_CV_FogFarZ - g_CV_FogNearZ;
		g_fFogScale = 1.0f / g_fFogRange * 255.0f;

		if (g_CV_TableFogVar.m_nValue)
		{
			g_pfnCalcFog = d3d_CalcNoFog;
		}
		else if (g_CV_VFog.m_nValue)
		{
			g_pfnCalcFog = d3d_CalcVFog;
#ifdef PRIMAL_HUNT
			d3d_ClearFogVolumes();
#endif
			g_fVFogValScale = g_CV_VFogMaxYVal.m_fValue - g_CV_VFogMinYVal.m_fValue;
			g_fVFogRangeScale = 1.0f / (g_CV_VFogMaxY.m_fValue - g_CV_VFogMinY.m_fValue);
			g_fVFogDensityScale = 255.0f / g_CV_VFogDensity.m_fValue;
		}
		else
		{
			g_pfnCalcFog = d3d_CalcFog;
		}

		// Rebuild the colour ramps when the light scale or the world's
		// colour changes.
		if (FDIFF(g_vLightAdd.x, g_vLightScaleLast.x) > 0.001f ||
			FDIFF(g_vLightAdd.y, g_vLightScaleLast.y) > 0.001f ||
			FDIFF(g_vLightAdd.z, g_vLightScaleLast.z) > 0.001f ||
			g_WorldColor != g_LastWorldColor)
		{
			if (g_CV_Saturate)
			{
				BUILD_RAMP(g_GammaTableR, g_vGlobalLightColor.x * 2.0f);
				BUILD_RAMP(g_GammaTableG, g_vGlobalLightColor.y * 2.0f);
				BUILD_RAMP(g_GammaTableB, g_vGlobalLightColor.z * 2.0f);
			}
			else
			{
				BUILD_RAMP(g_GammaTableR, g_vGlobalLightColor.x);
				BUILD_RAMP(g_GammaTableG, g_vGlobalLightColor.y);
				BUILD_RAMP(g_GammaTableB, g_vGlobalLightColor.z);
			}

			BUILD_RAMP(g_LightTableR, g_vGlobalLightColor.x);
			BUILD_RAMP(g_LightTableG, g_vGlobalLightColor.y);
			BUILD_RAMP(g_LightTableB, g_vGlobalLightColor.z);

			g_vLightScaleLast = g_vLightAdd;
			g_LastWorldColor = g_WorldColor;
		}

		g_LightmapPolyList = LTNULL;
		g_GouraudPolyList = 0;
		g_pDeferredPolies = LTNULL;
		g_nTLVertsUsed = 0;

		g_pStruct->m_nCounter4c = 0;
		g_pStruct->m_nCounter48 = 0;

		g_pD3DDevice->SetRenderState(D3DRENDERSTATE_FILLMODE,
			g_CV_Wireframe ? D3DFILL_WIREFRAME : D3DFILL_SOLID);

		// The scene, with the portals drawn after the object queues.
		if (!g_CV_ShowPortalBounds.m_nValue)
		{
			g_pfnPostQueueHook = d3d_DrawVisiblePortals;
			d3d_FullDrawScene(pDesc);
			g_pfnPostQueueHook = LTNULL;
		}

		g_pD3DDevice->SetRenderState(D3DRENDERSTATE_FILLMODE, D3DFILL_SOLID);

		if (g_CV_ShowTextureCounts)
		{
			AddDebugMessage(0, "Texture changes: %d", g_nTextureChanges);
			AddDebugMessage(0, "Texture uploads: %d", g_1005bd18);
		}

		if (g_CV_ShowPolyCounts)
		{
			g_pStruct->ConsolePrint("World Polies Processed: %d", g_nWorldPoliesDrawn);
			g_pStruct->ConsolePrint("World Polies Drawn: %d", g_nDrawPrimCalls);
			g_pStruct->ConsolePrint("Num Light Tests: %d", g_nSkyLightsApplied);
			g_pStruct->ConsolePrint("Num Lit Polies: %d (textured and uploaded: %d)", g_nLitObjects, g_nLitPolysUploaded);
			g_pStruct->ConsolePrint("Model triangles drawn: %d", g_nModelTriangles);
			g_pStruct->ConsolePrint("Particles drawn: %d", g_nParticlesDrawn);
			g_pStruct->ConsolePrint("Num Clip Tests: %d", g_nClipPasses);
			g_pStruct->ConsolePrint("Visible Leaves: %d", g_nVisibleLeaves);
			g_pStruct->ConsolePrint("Sky Portals: %d", g_nSkyPortalPoliesDrawn);
			g_pStruct->ConsolePrint("Sky Polies (fragments): %d", g_1005bd08);
			g_pStruct->ConsolePrint("Portals: %d", d3d_GetVisibleSet()->m_nPortals);
			g_pStruct->ConsolePrint("Visible lights: %d, rejected: %d", g_nDynamicLights, g_nLightPolyRejects);
			g_pStruct->ConsolePrint("Texture upload saves: %d", g_nLightmapUploadSaves);
			g_pStruct->ConsolePrint("Triangles: %d", g_nTrianglesDrawn);
		}

		if (g_CV_ShowFillInfo)
		{
			g_pStruct->ConsolePrint("Tri area drawn: %.3f", g_fFillArea);
			g_pStruct->ConsolePrint("Overdraw: %.3f", g_fFillArea /
				(float)((g_ViewParams.m_Rect.right - g_ViewParams.m_Rect.left) *
				        (g_ViewParams.m_Rect.bottom - g_ViewParams.m_Rect.top)));
		}

		if (g_CV_ShowTexInfo.m_nValue)
		{
			D3DDEVINFO_TEXTUREMANAGER info;

			g_pD3DDevice->GetInfo(D3DDEVINFOID_TEXTUREMANAGER, &info, sizeof(info));
			dd_ConsolePrint("ShowTexInfo ----------------------------");
			dd_ConsolePrint("bThrashing        - %d", info.bThrashing);
			dd_ConsolePrint("dwNumEvicts       - %d", info.dwNumEvicts);
			dd_ConsolePrint("dwNumVidCreates   - %d", info.dwNumVidCreates);
			dd_ConsolePrint("dwNumTexturesUsed - %d", info.dwNumTexturesUsed);
			dd_ConsolePrint("dwNumUsedTexInVid - %d", info.dwNumUsedTexInVid);
			dd_ConsolePrint("dwWorkingSet      - %d", info.dwWorkingSet);
			dd_ConsolePrint("dwWorkingSetBytes - %d", info.dwWorkingSetBytes);
			dd_ConsolePrint("dwTotalManaged    - %d", info.dwTotalManaged);
			dd_ConsolePrint("dwTotalBytes      - %d", info.dwTotalBytes);
			dd_ConsolePrint("dwLastPri         - %d", info.dwLastPri);
		}
	}

	// The world tree's boxes, down to DrawWorldTree's depth.
	if (g_CV_DrawWorldTree.m_nValue > -1 && g_pSceneWorld)
	{
		StateSet ssZEnable(D3DRENDERSTATE_ZENABLE, TRUE);

		d3d_UnsetTexture(g_iCurTextureStage);
		d3d_DrawWorldTree_R(&((RenderSceneWorld*)g_pSceneWorld)->m_WorldTree.m_RootNode,
			0, g_CV_DrawWorldTree.m_nValue);
	}

	if (g_CV_DrawTerrainSections.m_nValue && g_pSceneWorld)
		d3d_DrawTerrainSections((TerrainWorld*)g_pSceneWorld);

	d3d_CanvasPreFrame();

	if (!g_bWarbleTableValid || g_CV_WarbleScale != g_fWarbleScaleLast)
	{
		AddDebugMessage(9, "Rebuilding warble table");
		d3d_InitModelSun();
		g_bWarbleTableValid = LTTRUE;
		g_fWarbleScaleLast = g_CV_WarbleScale;
	}

	g_fWarbleTime += g_CV_WarbleSpeed * g_pSceneDesc->m_FrameTime;
	g_fWarbleFrac = g_fWarbleTime - (float)floor(g_fWarbleTime);

	if (g_CV_ModelProfile)
	{
		g_pStruct->ConsolePrint("ModelProfile: %d clipped, %d unclipped",
			g_nModelsClipped, g_nModelsUnclipped);
	}

	g_bSceneCleared++;
	return LTFALSE;
}


// ---------------------------------------------------------------------- //
//  Init and term.
// ---------------------------------------------------------------------- //
StructBank      g_ClipVertBank;                             // 0x1005e0e0
StructBank      g_ClipPointBank;                            // 0x1005e730
uint32          g_nClipVerts;                               // 0x1005e298
uint32          g_LastWorldColor;                           // 0x1005fdc8 (RenderScene's)

void d3d_InitWorldBanks();                                  // 0x100115a0 (common_draw.cpp)
void d3d_TermWorldBanks();                                  // 0x100115da

// 0x10018f90
void d3d_InitDraw()
{
	d3d_InitWorldBanks();

	g_nClipVerts = 0;
	sb_Init(&g_ClipVertBank, 0x100, 0x10);
	sb_Init(&g_ClipPointBank, 0xc, 0x64);
	sb_Init2(&g_DeferredPolyBank, sizeof(DeferredPoly), 0x80, 0x300);
	g_TLVertPool = LTNULL;
	g_LastWorldColor = 0;
}

// 0x10018ff0
void d3d_TermDraw()
{
	d3d_TermWorldBanks();

	sb_Term(&g_ClipVertBank);
	sb_Term(&g_ClipPointBank);
	sb_Term(&g_DeferredPolyBank);

	if (g_TLVertPool)
	{
		dfree(g_TLVertPool);
		g_TLVertPool = LTNULL;
	}
}


// ---------------------------------------------------------------------- //
//  Animated vertex light.
// ---------------------------------------------------------------------- //
struct LightAnimFrame
{
	uint8       m_Reserved00[0x08];
	uint8*      m_R;                                        // +0x08
	uint8*      m_G;                                        // +0x0c
	uint8*      m_B;                                        // +0x10
	uint8       m_nVerts;                                   // +0x14
	uint8       m_Reserved15[3];
};

struct LightAnim
{
	uint8           m_Reserved00[0x24];
	uint8**         m_Frames;                               // +0x24
	uint8           m_Reserved28[0x30 - 0x28];
	uint16          m_nVerts;                               // +0x30
	uint16          m_Reserved32;
	uint32          m_iFrames[2];                           // +0x34
	uint8           m_Blend;                                // +0x3c
};

struct LightAnimRef
{
	uint16      m_Reserved00;
	uint16      m_iVert;                                    // +0x02
};

struct LitVertex
{
	uint8       m_Reserved00[0x14];
	uint8       m_Color[4];                                 // +0x14 (b, g, r, a)
};

class CMulTable
{
public:
	CMulTable();

	uint8   m_Table[256][256];
};

extern CMulTable g_MulTable;                                // 0x10087c00
// d3d_tables.cpp's saturating add: an identity table, then 256 bytes of
// 0xFF, so a sum of two bytes indexes it (into m_Full) clamped.
class CIdentTable
{
public:
			CIdentTable();

	uint8	m_Ident[256];
	uint8	m_Full[256];
};

extern CIdentTable g_IdentTable;                            // 0x10097c00 (d3d_tables.cpp)

// 0x10019040 -- add a light animation's blended frame to the vertices'
// colours.
LTBOOL d3d_AddLightAnim(LitVertex* pVerts, uint32 nVerts, LightAnim* pAnim, LightAnimRef* pRef)
{
	LightAnimFrame *pFrame1, *pFrame2;
	uint8 blend, invBlend;
	uint16 iVert;
	uint32 i;

	iVert = pRef->m_iVert;
	if (iVert >= pAnim->m_nVerts)
		return FALSE;

	blend = pAnim->m_Blend;
	pFrame1 = (LightAnimFrame*)(pAnim->m_Frames[pAnim->m_iFrames[0]] + iVert * sizeof(LightAnimFrame));
	pFrame2 = (LightAnimFrame*)(pAnim->m_Frames[pAnim->m_iFrames[1]] + iVert * sizeof(LightAnimFrame));

	if (blend == 0)
		pFrame2 = pFrame1;
	else if (blend == 0xFF)
		pFrame1 = pFrame2;

	if (nVerts >= LTMIN(pFrame1->m_nVerts, pFrame2->m_nVerts))
		nVerts = LTMIN(pFrame1->m_nVerts, pFrame2->m_nVerts);

	if (pFrame1 == pFrame2)
	{
		for (i = 0; i < nVerts; i++)
		{
			pVerts[i].m_Color[2] = g_IdentTable.m_Ident[pFrame1->m_R[i] + pVerts[i].m_Color[2]];
			pVerts[i].m_Color[1] = g_IdentTable.m_Ident[pFrame1->m_G[i] + pVerts[i].m_Color[1]];
			pVerts[i].m_Color[0] = g_IdentTable.m_Ident[pFrame1->m_B[i] + pVerts[i].m_Color[0]];
		}
	}
	else
	{
		invBlend = 255 - blend;
		for (i = 0; i < nVerts; i++)
		{
			pVerts[i].m_Color[2] = g_IdentTable.m_Ident[g_IdentTable.m_Ident[g_MulTable.m_Table[pFrame2->m_R[i]][blend] + g_MulTable.m_Table[pFrame1->m_R[i]][invBlend]] + pVerts[i].m_Color[2]];
			pVerts[i].m_Color[1] = g_IdentTable.m_Ident[g_IdentTable.m_Ident[g_MulTable.m_Table[pFrame2->m_G[i]][blend] + g_MulTable.m_Table[pFrame1->m_G[i]][invBlend]] + pVerts[i].m_Color[1]];
			pVerts[i].m_Color[0] = g_IdentTable.m_Ident[g_IdentTable.m_Ident[g_MulTable.m_Table[pFrame2->m_B[i]][blend] + g_MulTable.m_Table[pFrame1->m_B[i]][invBlend]] + pVerts[i].m_Color[0]];
		}
	}

	return TRUE;
}
