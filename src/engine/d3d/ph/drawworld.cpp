// *********************************************************************** //
//
//  ph/drawworld.cpp -- Primal Hunt's d3d.ren, TU06: 0x10009880..0x1000b2f9,
//  /O1.
//
//  World polygons: lightmaps, detail textures and the environment map
//  (the console variables DetailTextures..LMAnim), the lightmapped
//  polygon draw (0x10009969), the frame's queued lightmap pieces
//  (0x1000a257..0x1000a870), the deferred polygons (0x1000ac3a), the
//  triangle-fan clip-and-draw pair (0x1000acf8, 0x1000addf) and the
//  32-byte vertex's clipper (0x1000aec6; ClipPoly<ClipVert>, ClipNear and
//  ClipLeft 0x1000af61..0x1000b1bc are ph/d3d_clip.h's, emitted here).
//
//  Near:
//    * 0x10009969 d3d_DrawLightmappedPoly: register allocation only
//      (retail keeps nFlags in edi and zero in ebx, the polygon in memory;
//      ours enregisters the polygon).
//    * 0x10009e55 d3d_SetLightmapTexture: retail pushes edi in the
//      prologue, before the early return (shrink-wrapping).
//    * 0x1000a870 d3d_DrawDetailPieces: the plain loop's count lives in
//      the parameter slot with the copy decremented (`while (i--)` that
//      cl 8966 here copy-propagates), and two integer stores scheduled
//      into the x87 prologue.
//    * 0x1000acf8 / 0x1000addf: retail walks the fan with one pointer
//      (`[ebx-0x20]` and `[ebx]`); every spelling tried here makes two.
//
//  The fog colour is read as three bytes (g_FogR/G/B): retail never merges
//  two of them into one word load, as cl does from one struct.
//
// *********************************************************************** //

#include "ltbasedefs.h"
#include <d3d.h>
#include "../../LT2/lithshared/stdlith/struct_bank.h"
#include "ph_decls.h"


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


CD3DConVar     g_CV_DetailTextures("DetailTextures", 1.0f, LTNULL, LTNULL);          // 0x10055478
CD3DConVar     g_CV_DetailTextureAdd("DetailTextureAdd", 1.0f, LTNULL, LTNULL);      // 0x10055418
CD3DConVar     g_CV_DetailTextureAngle("DetailTextureAngle", 0.0f, LTNULL, LTNULL);  // 0x100558e0
CD3DConVar     g_CV_FixSparkleys("FixSparkleys", 0.0f, LTNULL, LTNULL);              // 0x10055458
CD3DConVar     g_CV_LMFullBright("LMFullBright", 0.0f, LTNULL, LTNULL);              // 0x100554b8
CD3DConVar     g_CV_EnvMapWorld("EnvMapWorld", 1.0f, LTNULL, LTNULL);                // 0x10055438
CD3DConVar     g_CV_LMAnim("LMAnim", 1.0f, LTNULL, LTNULL);                          // 0x10055498

static uint32 g_nWorldFlags;                                                       // 0x10056908


// 0x10009959
void d3d_SetWorldFlags(uint32 nFlags)
{
	g_nWorldFlags = nFlags;
}

// 0x10009963
uint32 d3d_GetWorldFlags()
{
	return g_nWorldFlags;
}


// ---------------------------------------------------------------------- //
//  The world's polygons, as the drawer reads them.
// ---------------------------------------------------------------------- //

struct DeferredPoly;

// A lightmap texture, with the frame's polygons deferred on it.
struct LMTexture
{
	uint8       m_pad_00[0x04];
	LMTexture*  m_pNextDirty;                               // +0x04
	DeferredPoly* m_pPolies;                                // +0x08
	uint8       m_pad_0c[0x14 - 0x0c];
	uint32      m_nBytes;                                   // +0x14
	uint32      m_FrameCode;                                // +0x18
	LPDIRECTDRAWSURFACE7 m_pSurface;                        // +0x1c
	uint32      m_bValid;                                   // +0x20
};

struct WorldVertex
{
	LTVector*   m_Vec;                                      // +0x00
	float       m_TU, m_TV;                                 // +0x04
	float       m_LMU, m_LMV;                               // +0x0c
	uint32      m_Color;                                    // +0x14
};

// What the drawer reads of a polygon's texture and surface.
struct TexQueue;
class CRenderStateList;

struct SharedTexture
{
	uint8       m_pad_00[0x1c];
	TexQueue*   m_Queues[3];                                // +0x1c (d3d_QueuePoly's)
	SharedTexture* m_pLinkedTexture;                        // +0x28 (detail or env map)
	uint32      m_LinkType;                                 // +0x2c (0 detail, else env map)
	uint8       m_pad_30[0x34 - 0x30];
	CRenderStateList* m_pStateList;                         // +0x34
};

struct WorldSurface
{
	uint8       m_pad_00[0x2c];
	SharedTexture* m_pTexture;                               // +0x2c
	uint32      m_Flags;                                    // +0x30
};

struct WorldPoly
{
	uint8       m_pad_00[0x14];
	uint16      m_Flags;                                    // +0x14
	uint8       m_pad_16[0x28 - 0x16];
	LTVector*   m_pPlane;                                   // +0x28 (its normal first)
	WorldSurface* m_pSurface;                               // +0x2c
	void*       m_pLights;                                  // +0x30
	uint8       m_pad_34[0x38 - 0x34];
	LTVector    m_vLMOrigin;                                // +0x38
	uint8       m_pad_44[0x48 - 0x44];
	LMTexture*  m_pLightmap;                                // +0x48
	uint8       m_pad_4c[0x50 - 0x4c];
	WorldVertex* m_pTJuncVerts;                             // +0x50
	uint16      m_nVerts;                                   // +0x54
	uint16      m_nTJuncVerts;                              // +0x56
	WorldVertex m_Verts[1];                                 // +0x58
};

// The polygon as the frame draws it.
struct RenderPoly : public WorldPoly
{
};

#define WPF_LMDIRTY     0x8000

// A transformed vertex in the frame's pool.
struct TLVertex
{
	void        SetUV(float u, float v) { m_TU = u; m_TV = v; }

	LTVector    m_Vec;                                      // +0x00
	float       m_RHW;                                      // +0x0c
	uint32      m_Color;                                    // +0x10
	uint32      m_Specular;                                 // +0x14
	float       m_TU, m_TV;                                 // +0x18
};

// A lit, lightmapped vertex.
struct LMVertex : public TLVertex
{
	float       m_LMU, m_LMV;                               // +0x20
};

struct RenderStruct
{
	uint8       m_pad_00[0x4c];
	uint32      m_nLightmapBytes;                           // +0x4c
};

struct SceneWorld
{
	uint8       m_pad_00[0xf8];
	float       m_fLMGridSize;                              // +0xf8
};

extern LPDIRECT3DDEVICE7    g_pD3DDevice;                   // 0x100638c8
extern RenderStruct*        g_pStruct;                      // 0x1005df08
extern SceneWorld*          g_pSceneWorld;                  // 0x1005c208
class RTexture;
extern RTexture*            g_pCurTextures[];               // 0x10067270 (d3d_texture.cpp)
extern uint16               g_CurFrameCode;                 // 0x1005d238
extern uint32               g_WorldColor;                   // 0x1005c154
// A texture stage's texel size (d3d_texture.cpp's, one per stage).
struct TexPair
{
	float   u, v;
};

extern TexPair              g_TexelSize[];                  // 0x100672a8 (d3d_texture.cpp)
extern uint32               g_nTLVertPool;                  // 0x1005fe00
extern uint32               g_nTLVertsUsed;                 // 0x1005e27c
extern TLVertex*            g_TLVertPool;                   // 0x1005e294
extern void                 (__fastcall *g_pfnCalcFog)(const LTVector* pPos, uint32* pSpecular);  // 0x1005e1c4 (d3d_draw.cpp)

LTBOOL  d3d_UpdateAnimatedLightmap(SceneWorld* pWorld, WorldPoly* pPoly, LTBOOL bForce);   // 0x10034000 (lightanim.cpp)
void    d3d_GetLMAxes(int iScale, LTVector* pNormal, LTVector* pP, LTVector* pQ);  // 0x10035860 (lmsurfaces.cpp)
LTBOOL  d3d_GrowTLVertPool(uint32 nWanted);                 // 0x10014900
void    d3d_DisableTexture(uint32 iStage);                  // 0x1000a22b


// A polygon deferred on its lightmap texture.
struct DeferredPoly
{
	RenderPoly* m_pPoly;                                    // +0x00
	uint32      m_Reserved04[2];
	uint32      m_ClipFlags;                                // +0x0c
	DeferredPoly* m_pNext;                                  // +0x10
	uint32      m_Reserved14;
};

extern StructBank       g_DeferredPolyBank;                 // 0x1005e1f0
extern uint32           g_ClipFlags;                        // 0x1005bd0c
LMTexture*              g_pDirtyTextures;                   // 0x10056904


// The fog colour's bytes, read one by one (d3d_init.cpp's g_FogColor:
// retail never loads two of them as a word, as it would from one struct).
extern uint8        g_FogR;                                 // 0x1005dad8
extern uint8        g_FogG;                                 // 0x1005dad9
extern uint8        g_FogB;                                 // 0x1005dada
uint32  d3d_GammaRGB(uint8 r, uint8 g, uint8 b);            // 0x10014340

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

// The view (common_draw.cpp's).
struct ViewParams
{
	uint8       m_pad_000[0x88];
	float       m_NearZ;                                    // +0x088
	uint8       m_pad_08c[0x90 - 0x8c];
	float       m_FarZ2;                                    // +0x090 (the far clip)
	uint8       m_pad_094[0x15c - 0x94];
	LTMatrix    m_mWorldToCamera;                           // +0x15c
	uint8       m_pad_19c[0x1dc - 0x19c];
	LTMatrix    m_mFullTransform;                           // +0x1dc
	uint8       m_pad_21c[0x4b4 - 0x21c];
	LTVector    m_Pos;                                      // +0x4b4
};


LTBOOL  d3d_ClipTLVerts(TLVertex** ppVerts, int* pnVerts, ViewParams* pParams, uint32 flags);  // 0x1000aec6

extern CD3DConVar g_CV_UseD3DClip;                          // 0x100622b0 (d3d_init.cpp)
extern LTBOOL   g_100638c4;                                 // 0x100638c4 (d3d_init.cpp): env maps work
extern uint32   g_TextureStage;                             // 0x100622d0
extern uint32   g_nDrawPrimCalls;                           // 0x1005c144 (common_draw.cpp)
extern ViewParams g_ViewParams;                             // 0x1005b790 (common_draw.cpp)

#include "d3d_clip.h"
extern int32    g_CV_FixTJunc;                              // 0x1005dbb4

// The queue of the frame's lightmapped pieces (drawlightmaps.cpp's).
struct QueuedPoly
{
	RenderPoly* m_pPoly;                                    // +0x00
	uint32      m_iFirstVert;                               // +0x04
	uint32      m_nVerts;                                   // +0x08
	uint32      m_ClipFlags;                                // +0x0c
	QueuedPoly* m_pNext;                                    // +0x10
	uint32      m_Type;                                     // +0x14
};

struct LMBucket;
extern LMBucket*    g_LightmapPolyList;                     // 0x1005fda0 (drawlightmaps.cpp)

QueuedPoly* d3d_QueuePoly(void* pPoly, void* pList, uint32 nType);     // 0x10007ddb (drawmodel_clip.cpp)
void    d3d_DeferPoly(RenderPoly* pPoly);                   // 0x10014970 (d3d_draw.cpp)
LTBOOL  d3d_DrawPolyLightmap(WorldPoly* pPoly, LTBOOL bNew);  // 0x10021cd0 (d3d_texture.cpp)
void    dd_ConsolePrint(const char* pMsg, ...);             // 0x10013ade (common_stuff.cpp)

void    d3d_CheckLightmap(SceneWorld* pWorld, WorldPoly* pPoly);
uint32  d3d_SetLightmapTexture(WorldPoly* pPoly, uint32 iStage);
void    d3d_SetupLMVerts(uint32 nFlags, LMVertex* pOut, WorldVertex* pVerts, int nVerts);
void    d3d_SetupLMVertsCalc(uint32 nFlags, WorldPoly* pPoly, LMVertex* pOut,
	WorldVertex* pVerts, int nVerts);
void    d3d_SetupLMUVs(WorldPoly* pPoly, LMVertex* pOut, WorldVertex* pVerts, int nVerts);
TLVertex* d3d_GetTLVerts(uint32 nVerts);
void    d3d_DrawLMPieces();                                 // 0x1000a257
void    d3d_DrawTLPolyTris(TLVertex* pVerts, int nVerts, ViewParams* pParams, uint32 fvf);
LTBOOL  d3d_ClipLMVerts(ClipVert2** ppVerts, int32* pnVerts, ViewParams* pParams, uint32 flags);  // 0x100085f2 (drawmodel_clip.cpp)

#define MAX_LMPOLY_VERTS    128
#define SURF_LMFLAGS        0x100000
#define FVF_TLVERTEX        (D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_SPECULAR | D3DFVF_TEX1)

// 0x10009969 -- a lightmapped polygon: lit vertices, the lightmap
// coordinates (made again from the lightmap vectors when the lightmap is
// new and the clipper made vertices of its own), then either the base
// texture now with the lightmap queued as a piece (world flags set) or
// the lightmap now and the base texture queued.
void d3d_DrawLightmappedPoly(RenderPoly* pPoly)
{
	uint32 nFlags;
	LMVertex verts[MAX_LMPOLY_VERTS];
	LMVertex* pVerts;
	TLVertex* pTLVerts;
	LMVertex* pIn;
	TLVertex* pOut;
	WorldVertex* pPolyVerts;
	int nVerts, nPolyVerts;
	uint32 i;
	LTBOOL bProject, bClipped, bLightmap, bEnvMap, bNew;
	QueuedPoly* pQueued;
	LPDIRECTDRAWSURFACE7 pOldTexture;
	DWORD oldAlphaBlend, oldAlphaFunc, oldZFunc;

	bProject = LTTRUE;
	bClipped = LTFALSE;
	bLightmap = LTTRUE;

	if (pPoly->m_pSurface->m_pTexture->m_LinkType && g_CV_EnvMapWorld.m_nValue && g_100638c4)
		bEnvMap = LTTRUE;
	else
		bEnvMap = LTFALSE;

	nFlags = pPoly->m_pSurface->m_Flags & SURF_LMFLAGS;

	if (g_CV_FixTJunc)
	{
		pPolyVerts = pPoly->m_pTJuncVerts;
		nPolyVerts = pPoly->m_nTJuncVerts;
	}
	else
	{
		pPolyVerts = pPoly->m_Verts;
		nPolyVerts = pPoly->m_nVerts;
	}

	nVerts = nPolyVerts;
	if (nVerts > MAX_LMPOLY_VERTS)
	{
		dd_ConsolePrint("Error: vertex buffer overflow");
		return;
	}

	StateSet fogColor(D3DRENDERSTATE_FOGCOLOR,
		d3d_GammaRGB(g_FogR, g_FogG, g_FogB));

	pVerts = verts;
	d3d_SetupLMVerts(nFlags, verts, pPolyVerts, nVerts);

	if (g_ClipFlags && !bEnvMap)
	{
		if (!d3d_ClipLMVerts((ClipVert2**)&pVerts, (int32*)&nVerts, &g_ViewParams, 0))
			return;

		bProject = LTFALSE;
		bClipped = (pVerts != verts);
	}

	if (g_CV_LMAnim.m_nValue)
		d3d_CheckLightmap(g_pSceneWorld, pPoly);

	bNew = !pPoly->m_pLightmap->m_bValid;
	if (bNew)
		pPoly->m_pLightmap->m_bValid = LTTRUE;

	if (pPoly->m_pLights)
		bNew = d3d_DrawPolyLightmap(pPoly, bNew);
	else
		pPoly->m_pLightmap->m_bValid = LTTRUE;

	if (bNew)
	{
		bLightmap = LTFALSE;
		if (bClipped)
		{
			pVerts = verts;
			nVerts = nPolyVerts;
			d3d_SetupLMVertsCalc(nFlags, pPoly, verts, pPolyVerts, nPolyVerts);
			bProject = LTTRUE;
		}
		else
		{
			d3d_SetupLMUVs(pPoly, pVerts, pPolyVerts, nVerts);
		}
	}

	if (bProject && !bEnvMap)
	{
		if (!d3d_ClipLMVerts((ClipVert2**)&pVerts, (int32*)&nVerts, &g_ViewParams, 0))
			return;
	}

	pTLVerts = d3d_GetTLVerts(nVerts);
	if (!pTLVerts)
		return;

	if (g_nWorldFlags)
	{
		pIn = pVerts;
		pOut = pTLVerts;
		for (i = nVerts; i; i--)
		{
			*pOut = *pIn;
			pOut->m_TU = pIn->m_TU;
			pOut->m_TV = pIn->m_TV;
			pOut++;
			pIn++;
		}

		pQueued = d3d_QueuePoly(pPoly, (QueuedPoly**)&g_LightmapPolyList, 0);
		pQueued->m_iFirstVert = g_nTLVertsUsed;
		pQueued->m_nVerts = nVerts;
		pQueued->m_ClipFlags = bLightmap ? g_ClipFlags : 0;
		pQueued->m_Type = 3;

		if (bLightmap && d3d_SetLightmapTexture(pPoly, g_TextureStage))
			bLightmap = LTFALSE;

		if (!bLightmap)
			g_pD3DDevice->GetTexture(g_TextureStage, &pOldTexture);

		d3d_DrawLMPieces();

		if (!bLightmap)
			g_pD3DDevice->SetTexture(g_TextureStage, pOldTexture);

		pIn = pVerts;
		pOut = pTLVerts;
		for (i = nVerts; i; i--)
		{
			pOut->m_TU = pIn->m_LMU;
			pOut->m_TV = pIn->m_LMV;
			pOut++;
			pIn++;
		}

		g_pD3DDevice->GetRenderState(D3DRENDERSTATE_ALPHABLENDENABLE, &oldAlphaBlend);
		g_pD3DDevice->SetRenderState(D3DRENDERSTATE_ALPHABLENDENABLE, TRUE);
		g_pD3DDevice->GetRenderState(D3DRENDERSTATE_ALPHAFUNC, &oldAlphaFunc);
		g_pD3DDevice->SetRenderState(D3DRENDERSTATE_ALPHAFUNC, D3DCMP_ALWAYS);
		g_pD3DDevice->SetRenderState(D3DRENDERSTATE_SRCBLEND,
			g_CV_Saturate ? D3DBLEND_DESTCOLOR : D3DBLEND_ZERO);
		g_pD3DDevice->SetRenderState(D3DRENDERSTATE_DESTBLEND, D3DBLEND_SRCCOLOR);
		g_pD3DDevice->GetRenderState(D3DRENDERSTATE_ZFUNC, &oldZFunc);
		g_pD3DDevice->SetRenderState(D3DRENDERSTATE_ZFUNC, D3DCMP_EQUAL);
	}
	else
	{
		pIn = pVerts;
		pOut = pTLVerts;
		for (i = nVerts; i; i--)
		{
			*pOut = *pIn;
			pOut->m_TU = pIn->m_LMU;
			pOut->m_TV = pIn->m_LMV;
			pOut++;
			pIn++;
		}
	}

	if (!g_CV_LMFullBright.m_nValue)
	{
		if (bLightmap && !d3d_SetLightmapTexture(pPoly, g_TextureStage))
		{
			d3d_DeferPoly(pPoly);
			return;
		}

		if (bEnvMap)
			d3d_DrawTLPolyTris(pTLVerts, nVerts, &g_ViewParams, FVF_TLVERTEX);
		else
			g_pD3DDevice->DrawPrimitive(D3DPT_TRIANGLEFAN, FVF_TLVERTEX, pTLVerts, nVerts, 0);
	}

	if (g_nWorldFlags)
	{
		g_pD3DDevice->SetRenderState(D3DRENDERSTATE_ALPHABLENDENABLE, oldAlphaBlend);
		g_pD3DDevice->SetRenderState(D3DRENDERSTATE_ALPHAFUNC, oldAlphaFunc);
		g_pD3DDevice->SetRenderState(D3DRENDERSTATE_ZFUNC, oldZFunc);
	}
	else
	{
		pIn = pVerts;
		pOut = pTLVerts;
		for (i = nVerts; i; i--)
		{
			pOut->m_TU = pIn->m_TU;
			pOut->m_TV = pIn->m_TV;
			pOut++;
			pIn++;
		}

		pQueued = d3d_QueuePoly(pPoly, (QueuedPoly**)&g_LightmapPolyList, 0);
		pQueued->m_iFirstVert = g_nTLVertsUsed;
		pQueued->m_nVerts = nVerts;
		pQueued->m_ClipFlags = g_ClipFlags;
		pQueued->m_Type = 3;

		if (g_CV_FixSparkleys.m_nValue)
			d3d_DrawLMPieces();
	}

	g_nTLVertsUsed += nVerts;
	g_nDrawPrimCalls++;
}

// 0x10009e30 -- rebuild a polygon's lightmap if it's been marked dirty.
void d3d_CheckLightmap(SceneWorld* pWorld, WorldPoly* pPoly)
{
	if (pPoly->m_Flags & WPF_LMDIRTY)
	{
		pPoly->m_Flags &= ~WPF_LMDIRTY;
		d3d_UpdateAnimatedLightmap(pWorld, pPoly, LTFALSE);
	}
}

// 0x10009e55 -- bind a polygon's lightmap to a texture stage.
uint32 d3d_SetLightmapTexture(WorldPoly* pPoly, uint32 iStage)
{
	LMTexture* pLM;

	pLM = pPoly->m_pLightmap;
	if (!pLM)
		return 0;

	if (pLM != (LMTexture*)g_pCurTextures[iStage])
	{
		if (pLM->m_bValid)
		{
			if (pLM->m_FrameCode != g_CurFrameCode)
			{
				g_pStruct->m_nLightmapBytes += pLM->m_nBytes;
				pLM->m_FrameCode = g_CurFrameCode;
			}

			g_pD3DDevice->SetTexture(iStage, pLM->m_pSurface);
			g_pCurTextures[iStage] = (RTexture*)pLM;
		}
	}

	return 1;
}

// 0x10009ebb -- a polygon's vertices, lit, with their own lightmap
// coordinates.
void d3d_SetupLMVerts(uint32 nFlags, LMVertex* pOut, WorldVertex* pVerts, int nVerts)
{
	while (nVerts > 0)
	{
		pOut->m_Vec.x = pVerts->m_Vec->x;
		pOut->m_Vec.y = pVerts->m_Vec->y;
		pOut->m_Vec.z = pVerts->m_Vec->z;
		pOut->m_Color = g_WorldColor;
		g_pfnCalcFog(&pOut->m_Vec, &pOut->m_Specular);
		pOut->SetUV(pVerts->m_TU, pVerts->m_TV);
		pOut->m_LMU = pVerts->m_LMU;
		pOut->m_LMV = pVerts->m_LMV;

		pOut++;
		pVerts++;
		nVerts--;
	}
}

// 0x10009f1c -- a polygon's vertices, lit, with lightmap coordinates
// from its lightmap vectors.
void d3d_SetupLMVertsCalc(uint32 nFlags, WorldPoly* pPoly, LMVertex* pOut,
	WorldVertex* pVerts, int nVerts)
{
	LTVector vP, vQ, vPos;

	d3d_GetLMAxes((pPoly->m_Flags >> 11) & 7, (LTVector*)pPoly->m_pPlane, &vP, &vQ);

	while (nVerts > 0)
	{
		pOut->m_Vec.x = pVerts->m_Vec->x;
		pOut->m_Vec.y = pVerts->m_Vec->y;
		pOut->m_Vec.z = pVerts->m_Vec->z;
		pOut->m_Color = g_WorldColor;
		g_pfnCalcFog(&pOut->m_Vec, &pOut->m_Specular);

		vPos.x = pVerts->m_Vec->x - pPoly->m_vLMOrigin.x;
		vPos.y = pVerts->m_Vec->y - pPoly->m_vLMOrigin.y;
		vPos.z = pVerts->m_Vec->z - pPoly->m_vLMOrigin.z;

		pOut->SetUV(pVerts->m_TU, pVerts->m_TV);
		pOut->m_LMU = (vPos.Dot(vP) / g_pSceneWorld->m_fLMGridSize + 0.5f) * g_TexelSize[0].u;
		pOut->m_LMV = (vPos.Dot(vQ) / g_pSceneWorld->m_fLMGridSize + 0.5f) * g_TexelSize[0].v;

		pVerts++;
		pOut++;
		nVerts--;
	}
}

// 0x1000a016 -- only the lightmap coordinates of a polygon's vertices.
void d3d_SetupLMUVs(WorldPoly* pPoly, LMVertex* pOut, WorldVertex* pVerts, int nVerts)
{
	LTVector vP, vQ, vPos;

	d3d_GetLMAxes((pPoly->m_Flags >> 11) & 7, (LTVector*)pPoly->m_pPlane, &vP, &vQ);

	while (nVerts > 0)
	{
		vPos = *pVerts->m_Vec - pPoly->m_vLMOrigin;
		pOut->m_LMU = (vPos.Dot(vP) / g_pSceneWorld->m_fLMGridSize + 0.5f) * g_TexelSize[0].u;
		pOut->m_LMV = (vPos.Dot(vQ) / g_pSceneWorld->m_fLMGridSize + 0.5f) * g_TexelSize[0].v;

		pOut++;
		pVerts++;
		nVerts--;
	}
}

// 0x1000a0e4 -- room for nVerts in the frame's vertex pool.
TLVertex* d3d_GetTLVerts(uint32 nVerts)
{
	if (g_nTLVertsUsed + nVerts > g_nTLVertPool)
	{
		if (!d3d_GrowTLVertPool(g_nTLVertPool + nVerts + 1500))
			return LTNULL;
	}

	return &g_TLVertPool[g_nTLVertsUsed];
}


// 0x1000a11b -- draw a polygon now if it's lightmapped, else queue it on
// its texture.
void d3d_DrawTexturedPoly(RenderPoly* pPoly)
{
	LMTexture* pTex;
	DeferredPoly* pDeferred;

	if (pPoly->m_pLights)
	{
		d3d_DrawLightmappedPoly(pPoly);
	}
	else
	{
		pTex = pPoly->m_pLightmap;
		if (pTex)
		{
			pDeferred = (DeferredPoly*)sb_Allocate(&g_DeferredPolyBank);
			if (pDeferred)
			{
				if (!pTex->m_pPolies)
				{
					pTex->m_pNextDirty = g_pDirtyTextures;
					g_pDirtyTextures = pTex;
				}

				pDeferred->m_ClipFlags = g_ClipFlags;
				pDeferred->m_pPoly = pPoly;
				pDeferred->m_pNext = pTex->m_pPolies;
				pTex->m_pPolies = pDeferred;
			}
		}
	}
}

// 0x1000a172 -- the second stage for a detail texture pass.
void d3d_SetDetailTextureStage(int nMode)
{
	switch (nMode)
	{
		case 1:
			g_pD3DDevice->SetTextureStageState(1, D3DTSS_COLOROP,
				g_CV_DetailTextureAdd.m_nValue ? D3DTOP_ADDSIGNED : D3DTOP_MODULATE);
			g_pD3DDevice->SetTextureStageState(1, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
			break;

		case 2:
			g_pD3DDevice->SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_MODULATEALPHA_ADDCOLOR);
			break;
	}
}

// 0x1000a1c1
void d3d_UnsetDetailTextureStage()
{
	g_pD3DDevice->SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_DISABLE);
	g_pD3DDevice->SetTextureStageState(1, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
}

// 0x1000a1ea
void d3d_SetDetailTextureOp()
{
	g_pD3DDevice->SetTextureStageState(1, D3DTSS_COLOROP,
		g_CV_DetailTextureAdd.m_nValue ? D3DTOP_ADDSIGNED : D3DTOP_MODULATE);
}

// 0x1000a20e
void d3d_UnsetDetailTextureOp()
{
	g_pD3DDevice->SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_DISABLE);
	d3d_DisableTexture(1);
}

// 0x1000a22b -- unbind a stage's texture.
void d3d_DisableTexture(uint32 iStage)
{
	if (g_pCurTextures[iStage])
	{
		g_pD3DDevice->SetTexture(iStage, LTNULL);
		g_pCurTextures[iStage] = LTNULL;
	}
}

// ---------------------------------------------------------------------- //
//  The frame's lightmapped pieces: the polygons queued (d3d_QueuePoly) on
//  their texture's first queue, drawn texture by texture.
// ---------------------------------------------------------------------- //

// A texture's queue of pieces, chained on g_LightmapPolyList.
struct TexQueue
{
	SharedTexture* m_pTexture;                              // +0x00
	QueuedPoly* m_pPolys;                                   // +0x04
	TexQueue*   m_pNext;                                    // +0x08
};

// What the piece drawer reads of the texture bound to stage 0.
struct RTextureInfo
{
	uint8       m_pad_00[0x34];
	float       m_fDetailScale;                             // +0x34
	float       m_fDetailCos;                               // +0x38
	float       m_fDetailSin;                               // +0x3c
};

class RTexture
{
public:
	virtual void        Slot00();
	virtual uint32      GetType();                          // +0x04

	uint8       m_pad_04[0x18 - 0x04];
	RTextureInfo* m_pInfo;                                  // +0x18
};

class CRenderStateList
{
public:
	void        Restore();                                  // 0x10022a86
	void        Apply(CRenderStateList* pList, uint32 iStage);  // 0x10022a97
};

extern CRenderStateList g_SavedStates;                      // 0x10069728 (d3d_savedstates.cpp)
extern StructBank   g_ClipPointBank;                        // 0x1005e730
extern int32        g_CV_LightmapsOnly;                     // 0x1005df78 (common_stuff.cpp)
extern CD3DConVar   g_CV_DetailTextureScale;                // 0x100742b8 (drawpolymgr.cpp)

float   g_fDetailScale;                                     // 0x100554d8
float   g_fDetailCos;                                       // 0x10055900
float   g_fDetailSin;                                       // 0x10055410

void    d3d_FreeDeferredPolies(DeferredPoly* pList);        // 0x10014d30 (d3d_draw.cpp)
uint32  d3d_GetTextureFlags(SharedTexture* pTexture, uint32 iStage);    // 0x10021eb0 (d3d_texture.cpp)
int     d3d_SetTexture(SharedTexture* pTexture, uint32 iStage, uint32 flags);  // 0x100079e4 (drawmodel_clip.cpp)
uint32  d3d_MakeRGB(uint8 r, uint8 g, uint8 b);             // 0x10014380 (d3d_draw.cpp)

void    d3d_DrawPieceQueue(TexQueue* pQueue, SharedTexture* pTexture, LTBOOL bFree, LTBOOL bNoScale);

// Save a texture stage state, set it, and put it back when the scope
// closes.
class StageStateSet
{
public:
	StageStateSet(uint32 iStage, D3DTEXTURESTAGESTATETYPE state, uint32 val);
	~StageStateSet();

	uint32      m_iStage;
	D3DTEXTURESTAGESTATETYPE m_State;
	DWORD       m_OldVal;
};

// 0x1000a257 -- the queued pieces, texture by texture; with the
// lightmaps over the world (no world flags), the translucent textures'
// pieces wait for a second pass blended by their alpha.
void d3d_DrawLMPieces()
{
	TexQueue *pQueue, *pNext, *pDeferred;
	SharedTexture* pTexture;
	DWORD oldAlphaBlend;

	StageStateSet colorArg1(1, D3DTSS_COLORARG1, D3DTA_TEXTURE);
	StageStateSet colorArg2(1, D3DTSS_COLORARG2, D3DTA_CURRENT);

	if (!g_nWorldFlags && !g_CV_LMFullBright.m_nValue)
	{
		g_pD3DDevice->GetRenderState(D3DRENDERSTATE_ALPHABLENDENABLE, &oldAlphaBlend);
		g_pD3DDevice->SetRenderState(D3DRENDERSTATE_ALPHABLENDENABLE, TRUE);
		if (g_CV_Saturate)
			g_pD3DDevice->SetRenderState(D3DRENDERSTATE_SRCBLEND, D3DBLEND_DESTCOLOR);
		else
			g_pD3DDevice->SetRenderState(D3DRENDERSTATE_SRCBLEND, D3DBLEND_ZERO);
		g_pD3DDevice->SetRenderState(D3DRENDERSTATE_DESTBLEND, D3DBLEND_SRCCOLOR);
	}

	StateSet fogColor(D3DRENDERSTATE_FOGCOLOR, d3d_GammaRGB(g_FogR, g_FogG, g_FogB));

	pDeferred = LTNULL;
	pQueue = (TexQueue*)g_LightmapPolyList;
	while (pQueue)
	{
		pNext = pQueue->m_pNext;

		if (g_CV_LightmapsOnly)
		{
			d3d_FreeDeferredPolies((DeferredPoly*)pQueue->m_pPolys);
		}
		else if (pQueue->m_pPolys)
		{
			if (d3d_GetTextureFlags(pTexture = pQueue->m_pPolys->m_pPoly->m_pSurface->m_pTexture, 0) && !g_nWorldFlags && !g_CV_LMFullBright.m_nValue)
			{
				if (g_CV_Saturate)
					d3d_DrawPieceQueue(pQueue, pTexture, LTFALSE, LTFALSE);

				pQueue->m_pNext = pDeferred;
				pDeferred = pQueue;
				pQueue = pNext;
				continue;
			}

			d3d_DrawPieceQueue(pQueue, pTexture, LTTRUE, LTFALSE);
		}

		pQueue->m_pTexture->m_Queues[0] = LTNULL;
		sb_Free(&g_ClipPointBank, pQueue);
		pQueue = pNext;
	}

	if (!g_nWorldFlags && !g_CV_LMFullBright.m_nValue)
	{
		g_pD3DDevice->SetRenderState(D3DRENDERSTATE_SRCBLEND, D3DBLEND_SRCALPHA);
		if (g_CV_Saturate)
			g_pD3DDevice->SetRenderState(D3DRENDERSTATE_DESTBLEND, D3DBLEND_INVSRCALPHA);

		for (pQueue = pDeferred; pQueue; pQueue = pNext)
		{
			pNext = pQueue->m_pNext;
			d3d_DrawPieceQueue(pQueue, pQueue->m_pPolys->m_pPoly->m_pSurface->m_pTexture,
				LTTRUE, g_CV_Saturate);

			pQueue->m_pTexture->m_Queues[0] = LTNULL;
			sb_Free(&g_ClipPointBank, pQueue);
		}

		g_pD3DDevice->SetRenderState(D3DRENDERSTATE_ALPHABLENDENABLE, oldAlphaBlend);
	}

	g_LightmapPolyList = LTNULL;
	g_nTLVertsUsed = 0;
}

// 0x1000a48c
StageStateSet::StageStateSet(uint32 iStage, D3DTEXTURESTAGESTATETYPE state, uint32 val)
{
	m_iStage = iStage;
	m_State = state;
	g_pD3DDevice->GetTextureStageState(iStage, state, &m_OldVal);
	g_pD3DDevice->SetTextureStageState(m_iStage, m_State, val);
}

// 0x1000a4d1
StageStateSet::~StageStateSet()
{
	g_pD3DDevice->SetTextureStageState(m_iStage, m_State, m_OldVal);
}

void    d3d_DrawDetailPieces(TexQueue* pQueue, LTBOOL bFree, LTBOOL bNoScale);
void    d3d_DrawPieceList(TexQueue* pQueue, LTBOOL bFree, LTBOOL bNoScale);

// 0x1000a4e8 -- one texture's pieces: with its environment map, with its
// detail texture, or plain.
void d3d_DrawPieceQueue(TexQueue* pQueue, SharedTexture* pTexture, LTBOOL bFree, LTBOOL bNoScale)
{
	StateSet fogColor(D3DRENDERSTATE_FOGCOLOR, d3d_GammaRGB(g_FogR, g_FogG, g_FogB));

	if (g_100638c4 && pTexture->m_LinkType && g_CV_EnvMapWorld.m_nValue &&
		d3d_SetTexture(pTexture->m_pLinkedTexture, 0, 0))
	{
		d3d_SetTexture(pTexture, 1, 0);
		g_SavedStates.Restore();
		if (pTexture->m_pStateList)
			g_SavedStates.Apply(pTexture->m_pStateList, 1);

		g_fDetailScale = g_CV_DetailTextureScale.m_fValue;
		d3d_SetDetailTextureStage(pTexture->m_LinkType);
		d3d_DrawDetailPieces(pQueue, bFree, bNoScale);
		d3d_UnsetDetailTextureStage();
	}
	else
	{
		d3d_SetTexture(pTexture, 0, 0);
		if (pTexture->m_pStateList)
			g_SavedStates.Apply(pTexture->m_pStateList, 0);

		if (g_100638c4 && g_CV_DetailTextures.m_nValue && g_pCurTextures[0] &&
			pTexture->m_pLinkedTexture && !pTexture->m_LinkType &&
			d3d_SetTexture(pTexture->m_pLinkedTexture, 1, 0))
		{
			g_fDetailScale = g_CV_DetailTextureScale.m_fValue;
			if (g_pCurTextures[0]->GetType() == 1)
			{
				g_fDetailScale *= g_pCurTextures[0]->m_pInfo->m_fDetailScale;
				g_fDetailCos = g_pCurTextures[0]->m_pInfo->m_fDetailCos;
				g_fDetailSin = g_pCurTextures[0]->m_pInfo->m_fDetailSin;
			}
			else
			{
				g_fDetailCos = 1.0f;
				g_fDetailSin = 0.0f;
			}

			d3d_SetDetailTextureOp();
			d3d_DrawDetailPieces(pQueue, bFree, bNoScale);
			d3d_UnsetDetailTextureOp();
		}
		else
		{
			d3d_DrawPieceList(pQueue, bFree, bNoScale);
		}
	}

	g_SavedStates.Restore();
}

inline void d3d_SetFogColor(uint32 color)
{
	g_pD3DDevice->SetRenderState(D3DRENDERSTATE_FOGCOLOR, color);
}

// 0x1000a6bd -- a queue's pieces from the vertex pool, switching the
// blend and fog colour as their kinds change.
void d3d_DrawPieceList(TexQueue* pQueue, LTBOOL bFree, LTBOOL bNoScale)
{
	uint32 bBlend, bSaturate, bLit, nVerts;
	QueuedPoly *pPoly, *pNext;
	TLVertex *pVerts, *pVert;

	bBlend = 1;
	bSaturate = 1;
	pPoly = pQueue->m_pPolys;
	while (pPoly)
	{
		pNext = pPoly->m_pNext;

		if (bBlend != (pPoly->m_Type & 1))
		{
			bBlend = pPoly->m_Type & 1;
			g_pD3DDevice->SetRenderState(D3DRENDERSTATE_ALPHABLENDENABLE, bBlend);
			if (!bBlend)
				d3d_SetFogColor(d3d_MakeRGB(g_FogR, g_FogG, g_FogB));
			else
				d3d_SetFogColor(d3d_GammaRGB(g_FogR, g_FogG, g_FogB));
		}

		if (bBlend && bSaturate != ((pPoly->m_Type >> 1) & 1))
		{
			bSaturate = (pPoly->m_Type >> 1) & 1;
			if (g_CV_Saturate && bSaturate)
				g_pD3DDevice->SetRenderState(D3DRENDERSTATE_SRCBLEND, D3DBLEND_DESTCOLOR);
			else
				g_pD3DDevice->SetRenderState(D3DRENDERSTATE_SRCBLEND, D3DBLEND_ZERO);
		}

		bLit = (pPoly->m_Type >> 2) & 1;
		pVerts = &g_TLVertPool[pPoly->m_iFirstVert];
		if (!bNoScale)
		{
			pVert = pVerts;
			nVerts = pPoly->m_nVerts;
			while (nVerts--)
			{
				if (!bLit)
					pVert->m_Color = 0xFFFFFFFF;

				pVert->SetUV(pVert->m_TU * g_TexelSize[0].u, pVert->m_TV * g_TexelSize[0].v);
				pVert++;
			}
		}

		g_pD3DDevice->DrawPrimitive(D3DPT_TRIANGLEFAN, FVF_TLVERTEX, pVerts, pPoly->m_nVerts, 0);

		if (bFree)
			sb_Free(&g_DeferredPolyBank, pPoly);

		pPoly = pNext;
	}

	if (!bBlend || !bSaturate)
	{
		g_pD3DDevice->SetRenderState(D3DRENDERSTATE_ALPHABLENDENABLE, TRUE);
		d3d_SetFogColor(d3d_GammaRGB(g_FogR, g_FogG, g_FogB));
		if (g_CV_Saturate)
			g_pD3DDevice->SetRenderState(D3DRENDERSTATE_SRCBLEND, D3DBLEND_DESTCOLOR);
		else
			g_pD3DDevice->SetRenderState(D3DRENDERSTATE_SRCBLEND, D3DBLEND_ZERO);
	}
}

// A colour's bytes.
struct ColorBytes
{
	uint8       b, g, r, a;
};

#define COLORBYTES(c)   (*(ColorBytes*)&(c))

class CMulTable
{
public:
	uint8       m_Table[256][256];
};

extern CMulTable    g_MulTable;                             // 0x10087c00 (d3d_tables.cpp): a * b / 255
void    d3d_CalcEnvMapUV(const LTVector* pEye, const LTVector* pPos,
	const LTVector* pNormal, float* pU, float* pV);         // 0x100115fe (common_draw.cpp)

#define MAX_DETAIL_VERTS    80

void    d3d_DrawLMPolyTris(LMVertex* pVerts, int nVerts, ViewParams* pParams, uint32 fvf);  // 0x1000addf
#define FVF_LMVERTEX        (D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_SPECULAR | D3DFVF_TEX2)

// 0x1000a870 -- a queue's pieces with a second texture: the environment
// map (its coordinates from the eye's reflection, the polygons clipped
// again from their own vertices), or the detail texture (its coordinates
// the base texture's, scaled and turned).
void d3d_DrawDetailPieces(TexQueue* pQueue, LTBOOL bFree, LTBOOL bNoScale)
{
	LMVertex verts[MAX_DETAIL_VERTS];
	QueuedPoly *pPoly, *pNext;
	RenderPoly* pRenderPoly;
	TLVertex* pSrc;
	WorldVertex* pPolyVert;
	LMVertex* pDst;
	uint32 bBlend, bSaturate, bLit;
	int nVerts, i;
	float fScaleU, fScaleV, fCosU, fNegSinV, fSinU, fCosV;

	fScaleU = g_TexelSize[1].u * g_fDetailScale;
	fScaleV = g_TexelSize[1].v * g_fDetailScale;
	fCosU = g_fDetailCos * fScaleU;
	fNegSinV = -(g_fDetailSin * fScaleV);
	fSinU = g_fDetailSin * fScaleU;
	fCosV = g_fDetailCos * fScaleV;

	bBlend = 1;
	bSaturate = 1;
	pPoly = pQueue->m_pPolys;

	while (pPoly)
	{
		pNext = pPoly->m_pNext;
		pRenderPoly = pPoly->m_pPoly;

		if (bBlend != (pPoly->m_Type & 1))
		{
			bBlend = pPoly->m_Type & 1;
			g_pD3DDevice->SetRenderState(D3DRENDERSTATE_ALPHABLENDENABLE, bBlend);
			if (!bBlend)
				d3d_SetFogColor(d3d_MakeRGB(g_FogR, g_FogG, g_FogB));
			else
				d3d_SetFogColor(d3d_GammaRGB(g_FogR, g_FogG, g_FogB));
		}

		if (bBlend && bSaturate != ((pPoly->m_Type >> 1) & 1))
		{
			bSaturate = (pPoly->m_Type >> 1) & 1;
			if (g_CV_Saturate && bSaturate)
				g_pD3DDevice->SetRenderState(D3DRENDERSTATE_SRCBLEND, D3DBLEND_DESTCOLOR);
			else
				g_pD3DDevice->SetRenderState(D3DRENDERSTATE_SRCBLEND, D3DBLEND_ZERO);
		}

		bLit = (pPoly->m_Type >> 2) & 1;

		if (pRenderPoly->m_pSurface->m_pTexture->m_LinkType && g_CV_EnvMapWorld.m_nValue)
		{
			if (g_CV_FixTJunc)
			{
				pPolyVert = pRenderPoly->m_pTJuncVerts;
				nVerts = pRenderPoly->m_nTJuncVerts;
			}
			else
			{
				pPolyVert = pRenderPoly->m_Verts;
				nVerts = pRenderPoly->m_nVerts;
			}

			pDst = verts;
			for (i = nVerts; i; i--)
			{
				pDst->m_Vec.x = pPolyVert->m_Vec->x;
				pDst->m_Vec.y = pPolyVert->m_Vec->y;
				pDst->m_Vec.z = pPolyVert->m_Vec->z;

				if (bLit)
				{
					COLORBYTES(pDst->m_Color).r = g_MulTable.m_Table[COLORBYTES(pPolyVert->m_Color).r][COLORBYTES(g_WorldColor).r];
					COLORBYTES(pDst->m_Color).g = g_MulTable.m_Table[COLORBYTES(pPolyVert->m_Color).g][COLORBYTES(g_WorldColor).g];
					COLORBYTES(pDst->m_Color).b = g_MulTable.m_Table[COLORBYTES(pPolyVert->m_Color).b][COLORBYTES(g_WorldColor).b];
					COLORBYTES(pDst->m_Color).a = g_MulTable.m_Table[COLORBYTES(pPolyVert->m_Color).a][COLORBYTES(g_WorldColor).a];
				}
				else
				{
					pDst->m_Color = g_WorldColor;
				}

				pDst->m_LMU = g_TexelSize[1].u * pPolyVert->m_TU;
				pDst->m_LMV = g_TexelSize[1].v * pPolyVert->m_TV;
				g_pfnCalcFog(&pDst->m_Vec, &pDst->m_Specular);
				d3d_CalcEnvMapUV(&g_ViewParams.m_Pos, pPolyVert->m_Vec, pRenderPoly->m_pPlane,
					&pDst->m_TU, &pDst->m_TV);
				pDst->m_TU *= g_fDetailScale;
				pDst->m_TV *= g_fDetailScale;

				pDst++;
				pPolyVert++;
			}

			g_ClipFlags = pPoly->m_ClipFlags;
			d3d_DrawLMPolyTris(verts, nVerts, &g_ViewParams, FVF_LMVERTEX);
		}
		else
		{
			pSrc = &g_TLVertPool[pPoly->m_iFirstVert];
			pDst = verts;
			nVerts = pPoly->m_nVerts;
			i = nVerts;
			while (i--)
			{
				*(TLVertex*)pDst = *pSrc;
				if (!bLit)
					pDst->m_Color = 0xFFFFFFFF;

				pDst->SetUV(pSrc->m_TU * g_TexelSize[0].u, pSrc->m_TV * g_TexelSize[0].v);
				pDst->m_LMU = pSrc->m_TU * fCosU + pSrc->m_TV * fNegSinV;
				pDst->m_LMV = pSrc->m_TU * fSinU + pSrc->m_TV * fCosV;

				pDst++;
				pSrc++;
			}

			g_pD3DDevice->DrawPrimitive(D3DPT_TRIANGLEFAN, FVF_LMVERTEX, verts, nVerts, 0);
		}

		if (bFree)
			sb_Free(&g_DeferredPolyBank, pPoly);

		pPoly = pNext;
	}

	if (!bBlend || !bSaturate)
	{
		g_pD3DDevice->SetRenderState(D3DRENDERSTATE_ALPHABLENDENABLE, TRUE);
		d3d_SetFogColor(d3d_GammaRGB(g_FogR, g_FogG, g_FogB));
		if (g_CV_Saturate)
			g_pD3DDevice->SetRenderState(D3DRENDERSTATE_SRCBLEND, D3DBLEND_DESTCOLOR);
		else
			g_pD3DDevice->SetRenderState(D3DRENDERSTATE_SRCBLEND, D3DBLEND_ZERO);
	}
}

// 0x1000ac2b
void d3d_FlushWorldPieces()
{
	if (g_LightmapPolyList)
		d3d_DrawLMPieces();
}


// ---------------------------------------------------------------------- //
//  The deferred polygons, and the triangle fans drawn through the
//  clipper.
// ---------------------------------------------------------------------- //

// 0x1000ac3a -- the polygons deferred on their textures, texture by
// texture, under the fog volume colour.
void d3d_DrawWorldPieces()
{
	LMTexture *pTex, *pNextTex;
	DeferredPoly *pDeferred, *pNext;

	StateSet fogColor(D3DRENDERSTATE_FOGCOLOR,
		d3d_GammaRGB(g_FogR, g_FogG, g_FogB));

	pTex = g_pDirtyTextures;
	while (pTex)
	{
		pDeferred = pTex->m_pPolies;
		pNextTex = pTex->m_pNextDirty;
		while (pDeferred)
		{
			pNext = pDeferred->m_pNext;
			g_ClipFlags = pDeferred->m_ClipFlags;
			d3d_DrawLightmappedPoly(pDeferred->m_pPoly);
			sb_Free(&g_DeferredPolyBank, pDeferred);
			pDeferred = pNext;
		}

		pTex->m_pNextDirty = LTNULL;
		pTex->m_pPolies = LTNULL;
		pTex = pNextTex;
	}

	g_pDirtyTextures = LTNULL;
}


// 0x1000acf8 -- a triangle fan of transformed vertices: through the
// clipper whole when it needn't clip (or D3D clips all but the near
// plane), else triangle by triangle.
void d3d_DrawTLPolyTris(TLVertex* pVerts, int nVerts, ViewParams* pParams, uint32 fvf)
{
	TLVertex verts[32];
	TLVertex* pClipVerts;
	int nClipVerts;
	int i;

	if (g_ClipFlags && (!g_CV_UseD3DClip.m_nValue || (g_ClipFlags & 1)))
	{
		i = 2;
		while (nVerts > 2)
		{
			verts[0] = pVerts[0];
			verts[1] = pVerts[i-1];
			verts[2] = pVerts[i];
			pClipVerts = verts;
			nClipVerts = 3;
			if (d3d_ClipTLVerts(&pClipVerts, &nClipVerts, pParams, 0))
				g_pD3DDevice->DrawPrimitive(D3DPT_TRIANGLEFAN, fvf, pClipVerts, nClipVerts, 0);

			i++;
			nVerts--;
		}
	}
	else
	{
		if (d3d_ClipTLVerts(&pVerts, &nVerts, pParams, 0))
			g_pD3DDevice->DrawPrimitive(D3DPT_TRIANGLEFAN, fvf, pVerts, nVerts, 0);
	}
}

// 0x1000addf -- the same for lightmapped vertices.
void d3d_DrawLMPolyTris(LMVertex* pVerts, int nVerts, ViewParams* pParams, uint32 fvf)
{
	LMVertex verts[32];
	LMVertex* pClipVerts;
	int nClipVerts;
	int i;

	if (g_ClipFlags && (!g_CV_UseD3DClip.m_nValue || (g_ClipFlags & 1)))
	{
		i = 2;
		while (nVerts > 2)
		{
			verts[0] = pVerts[0];
			verts[1] = pVerts[i-1];
			verts[2] = pVerts[i];
			pClipVerts = verts;
			nClipVerts = 3;
			if (d3d_ClipLMVerts((ClipVert2**)&pClipVerts, (int32*)&nClipVerts, pParams, 0))
				g_pD3DDevice->DrawPrimitive(D3DPT_TRIANGLEFAN, fvf, pClipVerts, nClipVerts, 0);

			i++;
			nVerts--;
		}
	}
	else
	{
		if (d3d_ClipLMVerts((ClipVert2**)&pVerts, (int32*)&nVerts, pParams, 0))
			g_pD3DDevice->DrawPrimitive(D3DPT_TRIANGLEFAN, fvf, pVerts, nVerts, 0);
	}
}

void d3d_ProjectTLVert(TLVertex* pVert, ViewParams* pParams);  // 0x1002739d (drawmodelshadows.cpp)

// A vertex into view space (ICF folds it into drawmodel_clip.cpp's
// ClipVert2 copy, 0x10008719).
inline void d3d_TransformToCamera(ClipVert* pVert, LTMatrix* pMat)
{
	float x = pMat->m[0][0]*pVert->m_vPos.x + pMat->m[0][1]*pVert->m_vPos.y + pMat->m[0][2]*pVert->m_vPos.z + pMat->m[0][3];
	float y = pMat->m[1][0]*pVert->m_vPos.x + pMat->m[1][1]*pVert->m_vPos.y + pMat->m[1][2]*pVert->m_vPos.z + pMat->m[1][3];
	float z = pMat->m[2][0]*pVert->m_vPos.x + pMat->m[2][1]*pVert->m_vPos.y + pMat->m[2][2]*pVert->m_vPos.z + pMat->m[2][3];

	pVert->m_vPos.z = z;
	pVert->m_vPos.x = x;
	pVert->m_vPos.y = y;
}

// 0x1000aec6 -- with nothing to clip against, the vertices go straight
// through the full transform; otherwise to view space, through the
// clipper (ClipPoly<ClipVert> 0x1000af61, its ClipNear and ClipLeft
// 0x1000b07d, 0x1000b1bc) and then projected.
LTBOOL d3d_ClipTLVerts(TLVertex** ppVerts, int* pnVerts, ViewParams* pParams, uint32 flags)
{
	ClipVert* pVert;
	int nVerts;

	if (!g_ClipFlags)
	{
		pVert = (ClipVert*)*ppVerts;
		for (nVerts = *pnVerts; nVerts; nVerts--)
		{
			pVert->m_fRHW = MatVMul_InPlace_H(&pParams->m_mFullTransform, &pVert->m_vPos);
			pVert++;
		}
		return LTTRUE;
	}

	pVert = (ClipVert*)*ppVerts;
	for (nVerts = *pnVerts; nVerts; nVerts--)
	{
		d3d_TransformToCamera(pVert, &pParams->m_mWorldToCamera);
		pVert++;
	}

	if (!ClipPoly(g_ClipFlags, (ClipVert**)ppVerts, (int32*)pnVerts))
		return LTFALSE;

	pVert = (ClipVert*)*ppVerts;
	for (nVerts = *pnVerts; nVerts; nVerts--)
	{
		d3d_ProjectTLVert((TLVertex*)pVert, pParams);
		pVert++;
	}

	return LTTRUE;
}
