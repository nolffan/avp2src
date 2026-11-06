// *********************************************************************** //
//
//  ph/drawworld.cpp -- Primal Hunt's d3d.ren, TU06: 0x10009880..0x1000b2f9,
//  /O1.
//
//  World polygons: lightmaps, detail textures and the environment map
//  (the console variables DetailTextures..LMAnim).
//
// *********************************************************************** //

#include "ltbasedefs.h"
#include <d3d.h>
#include "../../LT2/lithshared/stdlith/struct_bank.h"


// A console variable (common_stuff.cpp's registrar, 0x10011f36).
struct CConVar
{
	CConVar(const char* pszName, float fDefault, int32* pnValue,
	        float* pfValue);                                // 0x10011f36

	int32       m_nValue;                                   // +0x00
	float       m_fValue;                                   // +0x04
	float       m_fDefault;                                 // +0x08
	int32*      m_pnValue;                                  // +0x0c
	float*      m_pfValue;                                  // +0x10
	const char* m_pszName;                                  // +0x14
	void*       m_hVar;                                     // +0x18
	CConVar*    m_pNext;                                    // +0x1c
};


CConVar     g_CV_DetailTextures("DetailTextures", 1.0f, LTNULL, LTNULL);          // 0x10055478
CConVar     g_CV_DetailTextureAdd("DetailTextureAdd", 1.0f, LTNULL, LTNULL);      // 0x10055418
CConVar     g_CV_DetailTextureAngle("DetailTextureAngle", 0.0f, LTNULL, LTNULL);  // 0x100558e0
CConVar     g_CV_FixSparkleys("FixSparkleys", 0.0f, LTNULL, LTNULL);              // 0x10055458
CConVar     g_CV_LMFullBright("LMFullBright", 0.0f, LTNULL, LTNULL);              // 0x100554b8
CConVar     g_CV_EnvMapWorld("EnvMapWorld", 1.0f, LTNULL, LTNULL);                // 0x10055438
CConVar     g_CV_LMAnim("LMAnim", 1.0f, LTNULL, LTNULL);                          // 0x10055498

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

// A lightmap texture.
struct LMTexture
{
	uint8       m_pad_00[0x14];
	uint32      m_nBytes;                                   // +0x14
	uint32      m_FrameCode;                                // +0x18
	LPDIRECTDRAWSURFACE7 m_pSurface;                        // +0x1c
	uint32      m_bValid;                                   // +0x20
};

struct WorldVertex
{
	LTVector*   m_Vec;                                      // +0x00
	float       m_TU, m_TV;                                 // +0x04
	uint32      m_LMU, m_LMV;                               // +0x0c
	uint32      m_Pad14;
};

struct WorldPoly
{
	uint8       m_pad_00[0x14];
	uint16      m_Flags;                                    // +0x14
	uint8       m_pad_16[0x28 - 0x16];
	void*       m_pLMInfo;                                  // +0x28
	uint8       m_pad_2c[0x38 - 0x2c];
	LTVector    m_vLMOrigin;                                // +0x38
	uint8       m_pad_44[0x48 - 0x44];
	LMTexture*  m_pLightmap;                                // +0x48
};

#define WPF_LMDIRTY     0x8000

// A lit, lightmapped vertex.
struct LMVertex
{
	void        SetUV(float u, float v) { m_TU = u; m_TV = v; }

	LTVector    m_Vec;                                      // +0x00
	float       m_RHW;                                      // +0x0c
	uint32      m_Color;                                    // +0x10
	uint32      m_Specular;                                 // +0x14
	float       m_TU, m_TV;                                 // +0x18
	uint32      m_LMU, m_LMV;                               // +0x20
};

struct LMVertexUV
{
	uint8       m_pad_00[0x20];
	float       m_LMU, m_LMV;                               // +0x20
};

// A transformed vertex in the frame's pool.
struct TLVertex
{
	uint8       m_Data[0x20];
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
extern LMTexture*           g_pCurTextures[];               // 0x10067270
extern uint16               g_CurFrameCode;                 // 0x1005d238
extern uint32               g_WorldColor;                   // 0x1005c154
extern float                g_fTexelSizeU;                  // 0x100672a8
extern float                g_fTexelSizeV;                  // 0x100672ac
extern uint32               g_nTLVertPool;                  // 0x1005fe00
extern uint32               g_nTLVertsUsed;                 // 0x1005e27c
extern TLVertex*            g_TLVertPool;                   // 0x1005e294
extern void                 (__fastcall *g_pfnSetupSpecular)(LMVertex* pVert, uint32* pSpecular);  // 0x1005e1c4

void    d3d_UpdateAnimatedLightmap(SceneWorld* pWorld, WorldPoly* pPoly, LTBOOL bForce);   // 0x10034000
void    d3d_GetLightmapVectors(uint32 iScale, void* pLMInfo, LTVector* pP, LTVector* pQ);  // 0x10035860
LTBOOL  d3d_GrowTLVertPool(uint32 nWanted);                 // 0x10014900
void    d3d_DisableTexture(uint32 iStage);                  // 0x1000a22b


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
	if (pLM)
	{
		if (pLM != g_pCurTextures[iStage] && pLM->m_bValid)
		{
			if (pLM->m_FrameCode != g_CurFrameCode)
			{
				g_pStruct->m_nLightmapBytes += pLM->m_nBytes;
				pLM->m_FrameCode = g_CurFrameCode;
			}

			g_pD3DDevice->SetTexture(iStage, pLM->m_pSurface);
			g_pCurTextures[iStage] = pLM;
		}

		return 1;
	}

	return 0;
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
		g_pfnSetupSpecular(pOut, &pOut->m_Specular);
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

	d3d_GetLightmapVectors((pPoly->m_Flags >> 11) & 7, pPoly->m_pLMInfo, &vP, &vQ);

	while (nVerts > 0)
	{
		pOut->m_Vec.x = pVerts->m_Vec->x;
		pOut->m_Vec.y = pVerts->m_Vec->y;
		pOut->m_Vec.z = pVerts->m_Vec->z;
		pOut->m_Color = g_WorldColor;
		g_pfnSetupSpecular(pOut, &pOut->m_Specular);

		vPos.x = pVerts->m_Vec->x - pPoly->m_vLMOrigin.x;
		vPos.y = pVerts->m_Vec->y - pPoly->m_vLMOrigin.y;
		vPos.z = pVerts->m_Vec->z - pPoly->m_vLMOrigin.z;

		pOut->SetUV(pVerts->m_TU, pVerts->m_TV);
		((LMVertexUV*)pOut)->m_LMU = (vPos.Dot(vP) / g_pSceneWorld->m_fLMGridSize + 0.5f) * g_fTexelSizeU;
		((LMVertexUV*)pOut)->m_LMV = (vPos.Dot(vQ) / g_pSceneWorld->m_fLMGridSize + 0.5f) * g_fTexelSizeV;

		pVerts++;
		pOut++;
		nVerts--;
	}
}

// 0x1000a016 -- only the lightmap coordinates of a polygon's vertices.
void d3d_SetupLMUVs(WorldPoly* pPoly, LMVertexUV* pOut, WorldVertex* pVerts, int nVerts)
{
	LTVector vP, vQ, vPos;

	d3d_GetLightmapVectors((pPoly->m_Flags >> 11) & 7, pPoly->m_pLMInfo, &vP, &vQ);

	while (nVerts > 0)
	{
		vPos = *pVerts->m_Vec - pPoly->m_vLMOrigin;
		pOut->m_LMU = (vPos.Dot(vP) / g_pSceneWorld->m_fLMGridSize + 0.5f) * g_fTexelSizeU;
		pOut->m_LMV = (vPos.Dot(vQ) / g_pSceneWorld->m_fLMGridSize + 0.5f) * g_fTexelSizeV;

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

// A texture's list of the frame's polygons to draw with it.
struct PolyTexture
{
	uint8       m_pad_00[0x04];
	PolyTexture* m_pNextDirty;                              // +0x04
	struct DeferredPoly* m_pPolies;                         // +0x08
};

struct DeferredPoly
{
	struct RenderPoly* m_pPoly;                             // +0x00
	uint32      m_Reserved04[2];
	uint32      m_ClipFlags;                                // +0x0c
	DeferredPoly* m_pNext;                                  // +0x10
	uint32      m_Reserved14;
};

struct RenderPoly
{
	uint8       m_pad_00[0x30];
	uint32      m_bLightmapped;                             // +0x30
	uint8       m_pad_34[0x48 - 0x34];
	PolyTexture* m_pTexture;                                // +0x48
};

extern StructBank       g_DeferredPolyBank;                 // 0x1005e1f0
extern uint32           g_ClipFlags;                        // 0x1005bd0c
PolyTexture*            g_pDirtyTextures;                   // 0x10056904

void    d3d_DrawLightmappedPoly(RenderPoly* pPoly);         // 0x10009969

// 0x1000a11b -- draw a polygon now if it's lightmapped, else queue it on
// its texture.
void d3d_DrawTexturedPoly(RenderPoly* pPoly)
{
	PolyTexture* pTex;
	DeferredPoly* pDeferred;

	if (pPoly->m_bLightmapped)
	{
		d3d_DrawLightmappedPoly(pPoly);
	}
	else
	{
		pTex = pPoly->m_pTexture;
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

// The frame's lightmapped pieces, drawn in one batch.
extern void*    g_pLMPieces;                                // 0x1005fda0

void    d3d_DrawLMPieces();                                 // 0x1000a257

// 0x1000ac2b
void d3d_FlushWorldPieces()
{
	if (g_pLMPieces)
		d3d_DrawLMPieces();
}
