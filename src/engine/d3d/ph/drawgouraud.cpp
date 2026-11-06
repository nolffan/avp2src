// *********************************************************************** //
//  ph/drawgouraud.cpp -- Primal Hunt's d3d.ren, TU24
//  (0x100238a6..0x10024534), /O1.
//
//  Gouraud world polygons and the multipass gouraud path (the
//  MultipassGouraud console variable): queueing a polygon, filling the
//  TL vertices with the gamma-corrected vertex colours, and the blend
//  setup.
//
//  Not yet written here: the dynamic-light pass 0x10023c66.
//  Still near: 0x100238e2 (the fog colour's bytes load as a word, and
//  esi/edi swap), 0x10024079 (where the shared `pop ecx` tail lands) and
//  0x1002426c (the texel-size products' operand order).
// *********************************************************************** //

#include <windows.h>
#include <ddraw.h>
#include <d3d.h>
#include <string.h>
#include "ltbasedefs.h"
#include "../../LT2/lithshared/stdlith/struct_bank.h"


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

extern LPDIRECT3DDEVICE7	g_pD3DDevice;               // 0x100638c8


// A polygon vertex as the world stores it, and the TL vertex drawn.
struct SPolyVertex
{
	LTVector*	m_pVec;                                 // +0x00
	float		m_U;                                    // +0x04
	float		m_V;                                    // +0x08
	uint8		m_pad_0c[0x14 - 0x0c];
	uint8		m_Color[3];                             // +0x14 (b, g, r)
	uint8		m_pad_17;
};

struct TLVertex
{
	void		SetUV(float u, float v)	{ m_TU = u; m_TV = v; }

	LTVector	m_Vec;                                  // +0x00
	float		m_RHW;                                  // +0x0c
	uint8		m_Color[4];                             // +0x10 (b, g, r, a)
	uint32		m_Specular;                             // +0x14
	float		m_TU;                                   // +0x18
	float		m_TV;                                   // +0x1c
};

struct RenderPoly;

// A polygon queued for a later pass (d3d_draw.cpp's queue, 0x10007ddb).
struct QueuedPoly
{
	RenderPoly*	m_pPoly;                                // +0x00
	uint32		m_iFirstVert;                           // +0x04
	uint32		m_nVerts;                               // +0x08
	uint32		m_ClipFlags;                            // +0x0c
	QueuedPoly*	m_pNext;                                // +0x10
	uint32		m_Type;                                 // +0x14
};

// The gouraud queue: one bucket of polygons per texture.
struct GouraudBucket
{
	struct SharedTexture*	m_pTexture;                 // +0x00
	QueuedPoly*		m_pPolies;                          // +0x04
	GouraudBucket*	m_pNext;                            // +0x08
};

struct SharedTexture
{
	uint8			m_pad_00[0x20];
	GouraudBucket*	m_pGouraudBucket;                   // +0x20
	uint8			m_pad_24[0x28 - 0x24];
	SharedTexture*	m_pDetailTexture;                   // +0x28
	SharedTexture*	m_pEnvMap;                          // +0x2c
	uint8			m_pad_30[0x34 - 0x30];
	class CRenderStateList*	m_pStateList;               // +0x34
};

struct Surface
{
	uint8			m_pad_00[0x2c];
	SharedTexture*	m_pTexture;                         // +0x2c
};

struct RenderPoly
{
	uint8			m_pad_00[0x2c];
	Surface*		m_pSurface;                         // +0x2c
	void*			m_pLights;                          // +0x30
	uint8			m_pad_34[0x50 - 0x34];
	SPolyVertex*	m_pTJuncVerts;                      // +0x50
	uint16			m_nVerts;                           // +0x54
	uint16			m_nTJuncVerts;                      // +0x56
	SPolyVertex		m_Verts[1];                         // +0x58
};

extern uint8	g_GammaTableR[256];                     // 0x1005f79c
extern uint8	g_GammaTableG[256];                     // 0x1005f89c
extern uint8	g_GammaTableB[256];                     // 0x1005f99c
extern uint8	g_LightTableR[256];                     // 0x1005fa9c
extern uint8	g_LightTableG[256];                     // 0x1005fb9c
extern uint8	g_LightTableB[256];                     // 0x1005fc9c
extern uint8	g_VertexAlpha;                          // 0x1005d20c
extern void		(__fastcall *g_pfnSetupSpecular)(TLVertex* pVert, uint32* pSpecular);  // 0x1005e1c4
extern uint32	g_ClipFlags;                            // 0x1005bd0c
extern float	g_fTexelSizeU;                          // 0x100672a8
extern float	g_fTexelSizeV;                          // 0x100672ac
extern GouraudBucket*	g_GouraudPolyList;              // 0x1005e728

extern QueuedPoly*	d3d_QueuePoly(void* pPoly, void* pList, uint32 nType);  // 0x10007ddb


struct ViewParams;
class RTexture
{
public:
	virtual void	Slot00();
	virtual void	Slot04();
	virtual LTBOOL	IsTranslucent();                    // +0x08
};

class CRenderStateList
{
public:
	void		Restore();                              // 0x10022a86
	void		Apply(CRenderStateList *pList, uint32 iStage);  // 0x10022a97
};

#define MAX_GOURAUD_VERTS	128
#define FVF_TLVERTEX		(D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_SPECULAR | D3DFVF_TEX1)

extern ViewParams		g_ViewParams;                   // 0x1005b790
extern CRenderStateList	g_SavedStates;                  // 0x10069728
extern StructBank		g_DeferredPolyBank;             // 0x1005e1f0
extern StructBank		g_ClipPointBank;                // 0x1005e730
extern QueuedPoly*		g_WorldPolyList;                // 0x1005fda0
extern uint32			g_nTLVertsUsed;                 // 0x1005e27c
extern uint32			g_nDrawPrimCalls;               // 0x1005c144
extern uint32			g_bCanEnvMap;                   // 0x100638c4
extern uint32			g_bDrawWorldFog;                // 0x1005e798
extern uint32			g_BindTextureStage;             // 0x100638c0
extern RTexture*		g_pCurTextures[];               // 0x10067270
extern int32			g_CV_FixTJunc;                  // 0x1005dbb4
extern int32			g_CV_Saturate;                  // 0x1005d384
extern int32			g_CV_FogEnable;                 // 0x1005df34
extern CD3DConVar		g_CV_EnvMapWorld;               // 0x10055438
extern CD3DConVar		g_CV_FixSparkleys;              // 0x10055458
extern CD3DConVar		g_CV_LMFullBright;              // 0x100554b8
extern CD3DConVar		g_CV_LMDynamic;                 // 0x1007c220

struct FogColor
{
	uint8	r, g, b;
};
extern FogColor			g_FogColor;                     // 0x1005dad8

extern void			dd_ConsolePrint(const char* pMsg, ...);                    // 0x10013ade
extern uint32		d3d_GammaRGB(uint8 r, uint8 g, uint8 b);                   // 0x10014340
extern LTBOOL		d3d_DrawPolyFlat(RenderPoly* pPoly);                       // 0x10014b80
extern uint32		d3d_GetWorldFlags();                                       // 0x10009963
extern TLVertex*	d3d_GetTLVerts(uint32 nVerts);                             // 0x1000a0e4
extern void			d3d_DisableTexture(uint32 iStage);                         // 0x1000a22b
extern void			d3d_FlushWorldPieces();                                    // 0x1000ac2b
extern void			d3d_DrawTLPolyTris(TLVertex* pVerts, int nVerts, ViewParams* pParams, uint32 fvf);  // 0x1000acf8
extern LTBOOL		d3d_ClipTLVerts(TLVertex** ppVerts, int* pnVerts, ViewParams* pParams, uint32 flags);  // 0x1000aec6
extern void			d3d_LightPolyDynamic(RenderPoly* pPoly, TLVertex* pVerts, int nVerts);  // 0x1001a533
extern int			d3d_SetTexture(SharedTexture* pTexture, uint32 iStage, uint32 flags);  // 0x100079e4

void d3d_SetupGouraudVerts(TLVertex* pOut, SPolyVertex* pIn, int nVerts);
void d3d_SetupLightmapVerts(TLVertex* pOut, SPolyVertex* pIn, int nVerts);
void d3d_ScaleTexCoords(TLVertex* pVerts, uint32 nVerts);
void d3d_DrawPolyLights(RenderPoly* pPoly, TLVertex* pVerts, int nVerts);     // 0x10023c66

// A polygon's vertices: its T-junction-fixed set when FixTJunc is on (the
// shape of d3d_draw.cpp's 0x10015230).
inline void d3d_GetPolyVerts(RenderPoly* pPoly, SPolyVertex** ppVerts, int* pnVerts)
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

// Save a render state, set it, and put it back when the scope closes.
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


// ---------------------------------------------------------------------- //
//  Globals.
// ---------------------------------------------------------------------- //
CD3DConVar	g_CV_MultipassGouraud("MultipassGouraud", 1.0f, LTNULL, LTNULL);  // 0x1006b658  $E 0x100238a6/0x100238ab


// ---------------------------------------------------------------------- //
//  0x100238c5
// ---------------------------------------------------------------------- //
void d3d_QueueGouraudPoly(void* pPoly)
{
	d3d_QueuePoly(pPoly, &g_GouraudPolyList, 1)->m_ClipFlags = g_ClipFlags;
}


// ---------------------------------------------------------------------- //
//  0x100238e2 -- a gouraud polygon in passes: the vertex colours
//  modulated onto what the world pass drew (or queued for the world
//  pieces), then the fog colour over it, then the lights.
// ---------------------------------------------------------------------- //
// The pass's own vertex buffer (0x1006b678, 0x2000 bytes); its empty
// destructor is the atexit thunk 0x10024078.
struct GouraudVertBuffer
{
	~GouraudVertBuffer() {}

	TLVertex	m_Verts[MAX_GOURAUD_VERTS * 2];
};

void d3d_DrawMultipassGouraud(RenderPoly* pPoly, LTBOOL bLightmap)
{
	static GouraudVertBuffer s_Buffer;
	SPolyVertex* pPolyVerts;
	TLVertex* pVerts;
	TLVertex* pOut;
	int nVerts;
	LTBOOL bProject, bEnvMap;
	uint32 oldAlphaBlend, oldAlphaFunc, oldZFunc;
	QueuedPoly* pQueued;
	uint32 fogColor;

	bProject = TRUE;
	if (pPoly->m_pSurface->m_pTexture->m_pEnvMap && g_CV_EnvMapWorld.m_nValue && g_bCanEnvMap)
		bEnvMap = TRUE;
	else
		bEnvMap = FALSE;

	d3d_GetPolyVerts(pPoly, &pPolyVerts, &nVerts);

	if (nVerts > MAX_GOURAUD_VERTS)
	{
		dd_ConsolePrint("Error: vertex buffer overflow");
		return;
	}

	pVerts = s_Buffer.m_Verts;
	if (bLightmap)
		d3d_SetupLightmapVerts(pVerts, pPolyVerts, nVerts);
	else
		d3d_SetupGouraudVerts(pVerts, pPolyVerts, nVerts);

	if (g_ClipFlags && !bEnvMap)
	{
		if (!d3d_ClipTLVerts(&pVerts, &nVerts, &g_ViewParams, 0))
			return;

		bProject = FALSE;
	}

	if (!g_CV_LMDynamic.m_nValue)
		d3d_LightPolyDynamic(pPoly, pVerts, nVerts);

	if (bProject && !bEnvMap)
	{
		if (!d3d_ClipTLVerts(&pVerts, &nVerts, &g_ViewParams, 0))
			return;
	}

	pOut = d3d_GetTLVerts(nVerts);
	if (!pOut)
		return;

	memcpy(pOut, pVerts, nVerts * sizeof(TLVertex));

	if (d3d_GetWorldFlags())
	{
		pQueued = d3d_QueuePoly(pPoly, &g_WorldPolyList, 0);
		pQueued->m_iFirstVert = g_nTLVertsUsed;
		pQueued->m_nVerts = nVerts;
		pQueued->m_ClipFlags = g_ClipFlags;
		pQueued->m_Type = 3;

		d3d_FlushWorldPieces();

		g_pD3DDevice->GetRenderState(D3DRENDERSTATE_ALPHABLENDENABLE, (unsigned long*)&oldAlphaBlend);
		g_pD3DDevice->SetRenderState(D3DRENDERSTATE_ALPHABLENDENABLE, TRUE);
		g_pD3DDevice->GetRenderState(D3DRENDERSTATE_ALPHAFUNC, (unsigned long*)&oldAlphaFunc);
		g_pD3DDevice->SetRenderState(D3DRENDERSTATE_ALPHAFUNC, D3DCMP_ALWAYS);

		if (g_CV_Saturate && bLightmap)
			g_pD3DDevice->SetRenderState(D3DRENDERSTATE_SRCBLEND, D3DBLEND_DESTCOLOR);
		else
			g_pD3DDevice->SetRenderState(D3DRENDERSTATE_SRCBLEND, D3DBLEND_ZERO);

		g_pD3DDevice->SetRenderState(D3DRENDERSTATE_DESTBLEND, D3DBLEND_SRCCOLOR);
		g_pD3DDevice->GetRenderState(D3DRENDERSTATE_ZFUNC, (unsigned long*)&oldZFunc);
		g_pD3DDevice->SetRenderState(D3DRENDERSTATE_ZFUNC, D3DCMP_EQUAL);
	}

	if (!g_CV_LMFullBright.m_nValue)
	{
		fogColor = d3d_GammaRGB(g_FogColor.r, g_FogColor.g, g_FogColor.b);
		StateSet fogState(D3DRENDERSTATE_FOGCOLOR, fogColor);

		d3d_DisableTexture(g_BindTextureStage);
		g_pD3DDevice->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG2);

		if (bEnvMap)
			d3d_DrawTLPolyTris(pOut, nVerts, &g_ViewParams, FVF_TLVERTEX);
		else
			g_pD3DDevice->DrawPrimitive(D3DPT_TRIANGLEFAN, FVF_TLVERTEX, pOut, nVerts, 0);

		if (pPoly->m_pLights)
			d3d_DrawPolyLights(pPoly, pVerts, nVerts);

		g_pD3DDevice->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
	}

	if (d3d_GetWorldFlags())
	{
		g_pD3DDevice->SetRenderState(D3DRENDERSTATE_ALPHABLENDENABLE, oldAlphaBlend);
		g_pD3DDevice->SetRenderState(D3DRENDERSTATE_ALPHAFUNC, oldAlphaFunc);
		g_pD3DDevice->SetRenderState(D3DRENDERSTATE_ZFUNC, oldZFunc);
	}
	else
	{
		pQueued = d3d_QueuePoly(pPoly, &g_WorldPolyList, 0);
		pQueued->m_iFirstVert = g_nTLVertsUsed;
		pQueued->m_nVerts = nVerts;
		pQueued->m_ClipFlags = g_ClipFlags;
		pQueued->m_Type = bLightmap ? 3 : 1;

		if (g_CV_FixSparkleys.m_nValue)
			d3d_FlushWorldPieces();
	}

	g_nTLVertsUsed += nVerts;
	g_nDrawPrimCalls++;
}


// ---------------------------------------------------------------------- //
//  0x10023f80 / 0x10023ffc -- fill TL vertices from a polygon's.
// ---------------------------------------------------------------------- //
void d3d_SetupGouraudVerts(TLVertex* pOut, SPolyVertex* pIn, int nVerts)
{
	int i;

	for (i=0; i < nVerts; i++)
	{
		pOut->m_Vec.x = pIn->m_pVec->x;
		pOut->m_Vec.y = pIn->m_pVec->y;
		pOut->m_Vec.z = pIn->m_pVec->z;
		pOut->m_Color[2] = g_GammaTableR[pIn->m_Color[2]];
		pOut->m_Color[1] = g_GammaTableG[pIn->m_Color[1]];
		pOut->m_Color[0] = g_GammaTableB[pIn->m_Color[0]];
		pOut->m_Color[3] = g_VertexAlpha;
		g_pfnSetupSpecular(pOut, &pOut->m_Specular);
		pOut->SetUV(pIn->m_U, pIn->m_V);
		pIn++;
		pOut++;
	}
}

void d3d_SetupLightmapVerts(TLVertex* pOut, SPolyVertex* pIn, int nVerts)
{
	int i;

	for (i=0; i < nVerts; i++)
	{
		pOut->m_Vec.x = pIn->m_pVec->x;
		pOut->m_Vec.y = pIn->m_pVec->y;
		pOut->m_Vec.z = pIn->m_pVec->z;
		pOut->m_Color[2] = g_LightTableR[pIn->m_Color[2]];
		pOut->m_Color[1] = g_LightTableG[pIn->m_Color[1]];
		pOut->m_Color[0] = g_LightTableB[pIn->m_Color[0]];
		pOut->m_Color[3] = g_VertexAlpha;
		g_pfnSetupSpecular(pOut, &pOut->m_Specular);
		pOut->SetUV(pIn->m_U, pIn->m_V);
		pIn++;
		pOut++;
	}
}


// ---------------------------------------------------------------------- //
//  0x10024079 -- a gouraud polygon, drawn now: textured, or flat when the
//  texture will not bind; a translucent texture gets a second, additive
//  pass when the world is fogged.
// ---------------------------------------------------------------------- //
void d3d_DrawGouraudPoly(RenderPoly* pPoly)
{
	TLVertex verts[MAX_GOURAUD_VERTS];
	TLVertex* pVerts;
	TLVertex* pVert;
	SPolyVertex* pPolyVerts;
	int nVerts, i;
	CRenderStateList* pStateList;
	uint8 color[4];

	if (pPoly->m_pLights && g_CV_LMDynamic.m_nValue)
	{
		d3d_DrawMultipassGouraud(pPoly, FALSE);
		return;
	}

	d3d_GetPolyVerts(pPoly, &pPolyVerts, &nVerts);

	if (nVerts > MAX_GOURAUD_VERTS)
	{
		dd_ConsolePrint("Error: vertex buffer overflow");
		return;
	}

	pVerts = verts;
	d3d_SetupGouraudVerts(verts, pPolyVerts, nVerts);

	if (!g_CV_LMDynamic.m_nValue)
		d3d_LightPolyDynamic(pPoly, pVerts, nVerts);

	if (!d3d_ClipTLVerts(&pVerts, &nVerts, &g_ViewParams, 0))
		return;

	if (!d3d_SetTexture(pPoly->m_pSurface->m_pTexture, g_BindTextureStage, 0))
	{
		d3d_DrawPolyFlat(pPoly);
		return;
	}

	d3d_ScaleTexCoords(pVerts, nVerts);

	if (g_pCurTextures[g_BindTextureStage]->IsTranslucent() && g_bDrawWorldFog)
	{
		g_pD3DDevice->DrawPrimitive(D3DPT_TRIANGLEFAN, FVF_TLVERTEX, pVerts, nVerts, 0);

		if (g_CV_FogEnable)
			g_pD3DDevice->SetRenderState(D3DRENDERSTATE_FOGENABLE, FALSE);

		color[3] = 0xFF;
		color[2] = g_GammaTableR[255];
		color[1] = g_GammaTableG[255];
		color[0] = g_GammaTableB[255];
		pVert = pVerts;
		for (i=nVerts; i; i--)
		{
			*(uint32*)pVert->m_Color = *(uint32*)color;
			pVert++;
		}

		g_pD3DDevice->SetRenderState(D3DRENDERSTATE_ALPHABLENDENABLE, TRUE);
		g_pD3DDevice->DrawPrimitive(D3DPT_TRIANGLEFAN, FVF_TLVERTEX, pVerts, nVerts, 0);
		g_pD3DDevice->SetRenderState(D3DRENDERSTATE_ALPHABLENDENABLE, FALSE);

		if (g_CV_FogEnable)
			g_pD3DDevice->SetRenderState(D3DRENDERSTATE_FOGENABLE, TRUE);
	}
	else
	{
		pStateList = pPoly->m_pSurface->m_pTexture->m_pStateList;
		if (pStateList)
			g_SavedStates.Apply(pStateList, 0);

		g_pD3DDevice->DrawPrimitive(D3DPT_TRIANGLEFAN, FVF_TLVERTEX, pVerts, nVerts, 0);
		g_SavedStates.Restore();
	}

	g_nDrawPrimCalls++;
}


// ---------------------------------------------------------------------- //
//  0x1002426c -- scale texture coordinates to the bound texture's texels.
// ---------------------------------------------------------------------- //
void d3d_ScaleTexCoords(TLVertex* pVerts, uint32 nVerts)
{
	while (nVerts--)
	{
		pVerts->SetUV(pVerts->m_TU * g_fTexelSizeU, pVerts->m_TV * g_fTexelSizeV);
		pVerts++;
	}
}


// ---------------------------------------------------------------------- //
//  0x1002429c
// ---------------------------------------------------------------------- //
void d3d_SetGouraudBlend(LTBOOL bAdditive)
{
	if (!bAdditive)
	{
		g_pD3DDevice->SetRenderState(D3DRENDERSTATE_SRCBLEND, D3DBLEND_SRCALPHA);
		g_pD3DDevice->SetRenderState(D3DRENDERSTATE_DESTBLEND, D3DBLEND_ONE);
	}
}


// ---------------------------------------------------------------------- //
//  0x100242c2 -- draw the gouraud queue, a texture at a time.
// ---------------------------------------------------------------------- //
static void d3d_DrawGouraudList(QueuedPoly* pList);

void d3d_FlushGouraudPolies()
{
	GouraudBucket *pCur, *pNext;

	for (pCur = g_GouraudPolyList; pCur; pCur = pNext)
	{
		pNext = pCur->m_pNext;
		d3d_DrawGouraudList(pCur->m_pPolies);
		pCur->m_pTexture->m_pGouraudBucket = LTNULL;
		sb_Free(&g_ClipPointBank, pCur);
	}

	g_GouraudPolyList = LTNULL;
}


// ---------------------------------------------------------------------- //
//  0x1002430a
// ---------------------------------------------------------------------- //
void d3d_DrawGouraudEnvMap(RenderPoly* pPoly);
void d3d_DrawGouraudDetail(RenderPoly* pPoly);

static void d3d_DrawGouraudList(QueuedPoly* pList)
{
	QueuedPoly *pCur, *pNext;
	SharedTexture* pTexture;

	for (pCur = pList; pCur; pCur = pNext)
	{
		pNext = pCur->m_pNext;
		g_ClipFlags = pCur->m_ClipFlags;

		if (g_CV_MultipassGouraud.m_nValue)
		{
			d3d_DrawMultipassGouraud(pCur->m_pPoly, TRUE);
		}
		else
		{
			pTexture = pCur->m_pPoly->m_pSurface->m_pTexture;
			if (g_bCanEnvMap && pTexture && pTexture->m_pDetailTexture)
			{
				if (pTexture->m_pEnvMap)
					d3d_DrawGouraudEnvMap(pCur->m_pPoly);
				else
					d3d_DrawGouraudDetail(pCur->m_pPoly);
			}
			else
			{
				d3d_DrawGouraudPoly(pCur->m_pPoly);
			}
		}

		sb_Free(&g_DeferredPolyBank, pCur);
	}
}


// ---------------------------------------------------------------------- //
//  0x1002438e -- queue the polygon's vertices for the world pieces' pass.
// ---------------------------------------------------------------------- //
void d3d_DrawGouraudEnvMap(RenderPoly* pPoly)
{
	SPolyVertex* pPolyVerts;
	TLVertex* pVerts;
	int nVerts;
	QueuedPoly* pQueued;

	if (pPoly->m_pLights && g_CV_LMDynamic.m_nValue)
	{
		d3d_DrawMultipassGouraud(pPoly, FALSE);
		return;
	}

	if (!g_CV_EnvMapWorld.m_nValue)
		d3d_DrawGouraudPoly(pPoly);

	d3d_GetPolyVerts(pPoly, &pPolyVerts, &nVerts);

	pVerts = d3d_GetTLVerts(nVerts);
	if (!pVerts)
		return;

	d3d_SetupGouraudVerts(pVerts, pPolyVerts, nVerts);

	if (!g_CV_LMDynamic.m_nValue)
		d3d_LightPolyDynamic(pPoly, pVerts, nVerts);

	pQueued = d3d_QueuePoly(pPoly, &g_WorldPolyList, 0);
	pQueued->m_iFirstVert = g_nTLVertsUsed;
	pQueued->m_nVerts = nVerts;
	pQueued->m_ClipFlags = g_ClipFlags;
	pQueued->m_Type = 4;

	g_nTLVertsUsed += nVerts;
	g_nDrawPrimCalls++;
}


// ---------------------------------------------------------------------- //
//  0x10024443 -- the same, clipped here first.
// ---------------------------------------------------------------------- //
void d3d_DrawGouraudDetail(RenderPoly* pPoly)
{
	TLVertex verts[MAX_GOURAUD_VERTS];
	TLVertex* pVerts;
	SPolyVertex* pPolyVerts;
	TLVertex* pOut;
	int nVerts;
	QueuedPoly* pQueued;

	if (pPoly->m_pLights && g_CV_LMDynamic.m_nValue)
	{
		d3d_DrawMultipassGouraud(pPoly, FALSE);
		return;
	}

	d3d_GetPolyVerts(pPoly, &pPolyVerts, &nVerts);

	d3d_SetupGouraudVerts(verts, pPolyVerts, nVerts);

	pVerts = verts;
	if (!g_CV_LMDynamic.m_nValue)
		d3d_LightPolyDynamic(pPoly, verts, nVerts);

	if (!d3d_ClipTLVerts(&pVerts, &nVerts, &g_ViewParams, 0))
		return;

	pOut = d3d_GetTLVerts(nVerts);
	if (!pOut)
		return;

	memcpy(pOut, pVerts, nVerts * sizeof(TLVertex));

	pQueued = d3d_QueuePoly(pPoly, &g_WorldPolyList, 0);
	pQueued->m_iFirstVert = g_nTLVertsUsed;
	pQueued->m_nVerts = nVerts;
	pQueued->m_ClipFlags = 0;
	pQueued->m_Type = 4;

	g_nTLVertsUsed += nVerts;
	g_nDrawPrimCalls++;
}
