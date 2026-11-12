// *********************************************************************** //
//  ph/drawlightmaps.cpp -- Primal Hunt's d3d.ren, TU40
//  (0x10035c7e..0x10036a96), /O1.
//
//  The world's lightmap pass: the polygons queued for it, their lightmap
//  texture coordinates, and the texture stages that blend the lightmap
//  over the base texture.  No string names this object; "drawlightmaps"
//  is a guess.  .bss from 0x10082ebc.
//
//  Written: the pass itself 0x10035c7e, the lightmapped polygon
//  0x10035e34, the coordinate fills 0x1003600e, 0x1003610b, the
//  lightmap-only draw 0x100361d9, the screen-textured deferred polygon
//  0x10036282, the queue walker 0x100363ec, the queue 0x1003647b, the
//  stage set-up and reset 0x10036498, 0x10036534, the rotation to matrix
//  0x1003655d and the segment walk 0x100366da.
//
//  Near:
//    * 0x100361d9: the loop's stores are scheduled differently (the
//      texture coordinate stores land after the z copy here).
//    * 0x10036498: retail keeps 0, 1 and 2 in registers, cl 8966 here
//      only 0 and 1.
//    * 0x100366da: the two plane-side early outs and the push are laid
//      out of line in retail, and d1 - d2 is kept differently on the x87
//      stack (955 bytes against 956).
//
//  The texel-size scale (0x10035e34, 0x10036282) loads the vertex's
//  coordinate before the global only when it is read into a local first.
// *********************************************************************** //

#include <d3d.h>

#include "ltbasedefs.h"
#include "../../LT2/lithshared/stdlith/struct_bank.h"


struct SharedTexture;
class CRenderStateList;

// One texture's polygons queued for the lightmap pass.
struct LMBucket
{
	SharedTexture	*m_pTexture;	// +0x00
	struct QueuedPoly	*m_pPolies;	// +0x04
	LMBucket		*m_pNext;		// +0x08
};

struct SharedTexture
{
	uint8			m_Pad[0x1c];
	LMBucket		*m_pLMBucket;	// +0x1c
	uint8			m_Pad20[0x34 - 0x20];
	CRenderStateList	*m_pStateList;	// +0x34
};

struct Surface
{
	uint8			m_Pad[0x2c];
	SharedTexture	*m_pTexture;	// +0x2c
	uint32			m_Flags;		// +0x30
};

#define SURF_NOLIGHTMAP_DEFER	0x8000

struct SPolyVertex;

struct WorldPoly
{
	uint8		m_Pad[0x14];
	uint16		m_PageFlags;	// +0x14, 0x3800: the lightmap basis
	uint8		m_Pad16[0x28 - 0x16];
	LTVector	*m_pNormal;		// +0x28
	Surface		*m_pSurface;	// +0x2c
	void		*m_pLMData;		// +0x30
	uint8		m_Pad34[0x38 - 0x34];
	LTVector	m_LMOrigin;		// +0x38
	uint8		m_Pad44[0x54 - 0x44];
	uint16		m_nVertices;	// +0x54
	uint16		m_nExtraVertices;	// +0x56
	SPolyVertex	*m_Vertices[1];	// +0x58 (SPolyVertex, 0x18 bytes each)
};

struct SPolyVertex
{
	LTVector*	m_pVec;                                 // +0x00
	float		m_U;                                    // +0x04
	float		m_V;                                    // +0x08
	float		m_LMU;                                  // +0x0c
	float		m_LMV;                                  // +0x10
	uint8		m_Color[3];                             // +0x14 (b, g, r)
	uint8		m_pad_17;
};

// A vertex with one set of coordinates.
struct TLVertex
{
	LTVector	m_Vec;                                  // +0x00
	float		m_RHW;                                  // +0x0c
	uint32		m_Color;                                // +0x10
	uint32		m_Specular;                             // +0x14
	float		m_TU;                                   // +0x18
	float		m_TV;                                   // +0x1c
};

// A vertex with the base texture's and the lightmap's coordinates.
struct LMTLVertex
{
	void		SetUV(float u, float v)	{ m_TU = u; m_TV = v; }

	LTVector	m_Vec;                                  // +0x00
	float		m_RHW;                                  // +0x0c
	union
	{
		uint32	m_Color;                                // +0x10
		uint8	m_ColorB[4];                            //       (b, g, r, a)
	};
	uint32		m_Specular;                             // +0x14
	float		m_TU;                                   // +0x18
	float		m_TV;                                   // +0x1c
	float		m_LMU;                                  // +0x20
	float		m_LMV;                                  // +0x24
};

struct TexPair
{
	float	u, v;
};

// The world being drawn, as far as it is used here.
struct SceneWorld
{
	uint8		m_Pad[0xf8];
	float		m_LMGridSize;                           // +0xf8
};

// A polygon queued for a pass (d3d_draw.cpp's DeferredPoly).
struct QueuedPoly
{
	WorldPoly		*m_pPoly;                           // +0x00
	uint32			m_Reserved04[2];
	uint32			m_ClipFlags;                        // +0x0c
	QueuedPoly		*m_pNext;                           // +0x10
	uint32			m_Reserved14;
};

typedef LTBOOL (*QueuedPolyFn)(WorldPoly *pPoly);

extern LPDIRECT3DDEVICE7	g_pD3DDevice;               // 0x100638c8
extern uint32				g_ClipFlags;                // 0x1005bd0c
extern StructBank			g_DeferredPolyBank;         // 0x1005e1f0
extern int32				g_CV_LightmapsOnly;         // 0x1005df78

extern SceneWorld*			g_pSceneWorld;              // 0x1005c208
extern uint32				g_WorldColor;               // 0x1005c154 (common_draw.cpp)
extern TexPair				g_TexelSize[];              // 0x100672a8

extern uint32				g_nDrawPrimCalls;           // 0x1005c144
extern uint32				g_TextureStage;             // 0x100622d0
extern uint32				g_iCurTextureStage;         // 0x100638c0 (d3d_init.cpp)
extern StructBank			g_ClipPointBank;            // 0x1005e730
extern float				g_fScreenScaleX;            // 0x10052bd8
extern float				g_fScreenScaleY;            // 0x10052bdc
extern float				g_fScreenCenterX;           // 0x10053fe0
extern float				g_fScreenCenterY;           // 0x10053fe4
extern uint8				g_LightTableR[256];         // 0x1005fa9c
extern uint8				g_LightTableG[256];         // 0x1005fb9c
extern uint8				g_LightTableB[256];         // 0x1005fc9c
extern uint8				g_VertexAlpha;              // 0x1005d20c

// The current render context: its first member is the screen-space
// texture the deferred polygons are drawn with.
struct RenderContext
{
	SharedTexture	*m_pScreenTexture;                  // +0x00
};
extern RenderContext*		g_pRenderContext;           // 0x1005b778

class CRenderStateList
{
public:
	void		Restore();                              // 0x10022a86
	void		Apply(CRenderStateList *pList, uint32 iStage);  // 0x10022a97
};
extern CRenderStateList		g_SavedStates;              // 0x10069728

struct ViewParams;
struct ViewParams;
extern ViewParams			g_ViewParams;             // 0x1005b790

LTBOOL	d3d_ClipTLVerts(TLVertex** ppVerts, int* pnVerts, ViewParams* pParams, uint32 flags);  // 0x1000aec6
struct ClipVert2;
LTBOOL	d3d_ClipLMVerts(ClipVert2** ppVerts, int32* pnVerts, ViewParams* pContext, uint32 flags);  // 0x100085f2 (drawmodel_clip.cpp)
void d3d_GetLMAxes(int iBasis, LTVector *pNormal, LTVector *pRight, LTVector *pUp);	// 0x10035860
extern QueuedPoly*	d3d_QueuePoly(void* pPoly, void* pList, uint32 nType);  // 0x10007ddb
int		d3d_SetTexture(SharedTexture* pTexture, uint32 iStage, uint32 iLOD); // 0x100079e4
void	d3d_SetupScreenScale();                         // 0x10007930
struct RenderPoly;
void	d3d_LightLMPoly(RenderPoly* pPoly, ClipVert2* pVerts, uint32 nVerts);  // 0x100083ec (drawmodel_clip.cpp)
uint32	d3d_SetLightmapTexture(WorldPoly* pPoly, uint32 iStage);  // 0x10009e55
void	dd_ConsolePrint(const char* pMsg, ...);         // 0x10013ade
struct RenderPoly;
struct DeferredPoly;
void	d3d_DeferPoly(RenderPoly* pPoly);               // 0x10014970
void	d3d_FreeDeferredPolies(DeferredPoly* pList);    // 0x10014d30
uint32	d3d_GetTextureFlags(SharedTexture *pSharedTexture, uint32 stage);  // 0x10021eb0
LTBOOL	d3d_DrawPolyLightmap(WorldPoly* pPoly, LTBOOL bNew);  // 0x10021cd0 (d3d_texture.cpp)

LMBucket*	g_LightmapPolyList;                         // 0x1005fda0
LTBOOL		g_bLightmapModulate2X;                      // 0x10082ebc

LTBOOL	d3d_DrawLightmapPoly(WorldPoly *pPoly);
void	d3d_SetupLMPolyVerts(WorldPoly *pPoly, LMTLVertex *pOut, SPolyVertex *pIn, int nVerts);
void	d3d_SetupLMPolyCoords(WorldPoly *pPoly, LMTLVertex *pOut, SPolyVertex *pIn, int nVerts);
LTBOOL	d3d_DrawLightmapOnly(WorldPoly *pPoly);
LTBOOL	d3d_DrawScreenLMPoly(WorldPoly *pPoly);
LTBOOL	d3d_FlushPolyQueue(QueuedPoly *pList, QueuedPolyFn fn, LTBOOL bFree, QueuedPoly **ppDeferred);

#define FVF_LMTLVERTEX	(D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_SPECULAR | D3DFVF_TEX2)


// 0x10035c7e -- the lightmap pass: each texture's polygons drawn with
// their lightmaps; the ones drawn over twice (a second pass with the
// lightmap alone, blended) kept aside and redrawn at the end, and the
// deferred ones drawn with the screen texture.
void d3d_MultiPassFlush()
{
	LMBucket *pCur, *pNext, *pDrawn;
	QueuedPoly *pDeferred;
	unsigned long oldColorOp;

	pDrawn = LTNULL;
	pDeferred = LTNULL;

	pCur = g_LightmapPolyList;
	while(pCur)
	{
		pNext = pCur->m_pNext;

		if(pCur->m_pPolies)
		{
			if(d3d_GetTextureFlags(pCur->m_pPolies->m_pPoly->m_pSurface->m_pTexture, g_iCurTextureStage))
			{
				if(d3d_FlushPolyQueue(pCur->m_pPolies, d3d_DrawLightmapPoly, LTFALSE, &pDeferred))
				{
					pCur->m_pNext = pDrawn;
					pDrawn = pCur;
					pCur->m_pTexture->m_pLMBucket = LTNULL;
					pCur = pNext;
					continue;
				}

				d3d_FreeDeferredPolies((DeferredPoly*)pCur->m_pPolies);
			}
			else
			{
				d3d_FlushPolyQueue(pCur->m_pPolies, d3d_DrawLightmapPoly, LTTRUE, &pDeferred);
			}
		}

		pCur->m_pTexture->m_pLMBucket = LTNULL;
		sb_Free(&g_ClipPointBank, pCur);
		pCur = pNext;
	}

	if(pDeferred)
	{
		if(g_pRenderContext && g_pRenderContext->m_pScreenTexture)
			d3d_SetTexture(g_pRenderContext->m_pScreenTexture, g_TextureStage, 0);

		d3d_SetupScreenScale();
		d3d_FlushPolyQueue(pDeferred, d3d_DrawScreenLMPoly, LTTRUE, (QueuedPoly**)-1);
	}

	if(pDrawn)
	{
		g_pD3DDevice->SetRenderState(D3DRENDERSTATE_ALPHABLENDENABLE, TRUE);
		g_pD3DDevice->GetTextureStageState(1, D3DTSS_COLOROP, &oldColorOp);
		g_pD3DDevice->SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_DISABLE);

		for(pCur=pDrawn; pCur; pCur=pNext)
		{
			pNext = pCur->m_pNext;

			if(pCur->m_pPolies && pCur->m_pPolies->m_pPoly->m_pSurface->m_pTexture)
			{
				d3d_SetTexture(pCur->m_pPolies->m_pPoly->m_pSurface->m_pTexture, g_iCurTextureStage, 0);
				d3d_FlushPolyQueue(pCur->m_pPolies, d3d_DrawLightmapOnly, LTTRUE, (QueuedPoly**)-1);
			}

			sb_Free(&g_ClipPointBank, pCur);
		}

		g_pD3DDevice->SetRenderState(D3DRENDERSTATE_ALPHABLENDENABLE, FALSE);
		g_pD3DDevice->SetTextureStageState(1, D3DTSS_COLOROP, oldColorOp);
	}

	g_LightmapPolyList = LTNULL;
}


#define MAX_LMPOLY_VERTS		64
#define MAX_LMPASS_VERTS		128

// 0x10035e34 -- a polygon with its lightmap on the second stage (its
// dynamic lightmap, else its static one), or deferred when it has none.
LTBOOL d3d_DrawLightmapPoly(WorldPoly *pPoly)
{
	LMTLVertex verts[MAX_LMPASS_VERTS], *pVerts, *pOut;
	SPolyVertex *pIn;
	CRenderStateList *pStateList;
	float tu, tv;
	uint32 i;
	int nVerts;

	pIn = (SPolyVertex*)pPoly->m_Vertices;
	if(pPoly->m_nVertices > MAX_LMPASS_VERTS)
	{
		dd_ConsolePrint("Error: vertex buffer overflow");
		return LTFALSE;
	}

	pOut = verts;
	for(i=0; i < pPoly->m_nVertices; i++)
	{
		pOut->m_Vec.x = pIn->m_pVec->x;
		pOut->m_Vec.y = pIn->m_pVec->y;
		pOut->m_Vec.z = pIn->m_pVec->z;
		pOut->m_Color = g_WorldColor;
		pOut->SetUV(pIn->m_U, pIn->m_V);
		pOut->m_LMU = pIn->m_LMU;
		pOut->m_LMV = pIn->m_LMV;
		pIn++;
		pOut++;
	}

	pVerts = verts;
	nVerts = pPoly->m_nVertices;
	if(!d3d_ClipLMVerts((ClipVert2**)&pVerts, (int32*)&nVerts, &g_ViewParams, 0))
		return LTFALSE;

	if(pPoly->m_pLMData && d3d_DrawPolyLightmap(pPoly, 0))
	{
		if(pVerts == verts)
		{
			d3d_SetupLMPolyCoords(pPoly, pVerts, (SPolyVertex*)pPoly->m_Vertices, nVerts);
		}
		else
		{
			pVerts = verts;
			nVerts = pPoly->m_nVertices;
			d3d_SetupLMPolyVerts(pPoly, verts, (SPolyVertex*)pPoly->m_Vertices, nVerts);
			if(!d3d_ClipLMVerts((ClipVert2**)&pVerts, (int32*)&nVerts, &g_ViewParams, 0))
				return LTFALSE;
		}
	}
	else if(!d3d_SetLightmapTexture(pPoly, g_TextureStage))
	{
		d3d_DeferPoly((RenderPoly*)pPoly);
		return LTFALSE;
	}

	d3d_SetTexture(pPoly->m_pSurface->m_pTexture, g_iCurTextureStage, 0);

	pOut = pVerts;
	for(i=nVerts; i; i--)
	{
		tu = pOut->m_TU;
		tv = pOut->m_TV;
		pOut->SetUV(tu * g_TexelSize[0].u, tv * g_TexelSize[0].v);
		pOut++;
	}

	g_SavedStates.Restore();
	pStateList = pPoly->m_pSurface->m_pTexture->m_pStateList;
	if(pStateList)
		g_SavedStates.Apply(pStateList, g_iCurTextureStage);

	g_pD3DDevice->DrawPrimitive(D3DPT_TRIANGLEFAN, FVF_LMTLVERTEX, pVerts, nVerts, 0);
	g_SavedStates.Restore();
	g_nDrawPrimCalls++;
	return LTTRUE;
}


// 0x1003600e -- a polygon's vertices with both sets of coordinates.
void d3d_SetupLMPolyVerts(WorldPoly *pPoly, LMTLVertex *pOut, SPolyVertex *pIn, int nVerts)
{
	LTVector right, up, vec;
	int i;

	d3d_GetLMAxes((pPoly->m_PageFlags >> 11) & 7, pPoly->m_pNormal, &right, &up);

	for(i=0; i < nVerts; i++)
	{
		pOut->m_Vec.x = pIn->m_pVec->x;
		pOut->m_Vec.y = pIn->m_pVec->y;
		pOut->m_Vec.z = pIn->m_pVec->z;
		pOut->m_Color = g_WorldColor;
		pOut->SetUV(pIn->m_U, pIn->m_V);

		vec = *pIn->m_pVec - pPoly->m_LMOrigin;
		pOut->m_LMU = (right.Dot(vec) / g_pSceneWorld->m_LMGridSize + 0.5f) * g_TexelSize[1].u;
		pOut->m_LMV = (up.Dot(vec) / g_pSceneWorld->m_LMGridSize + 0.5f) * g_TexelSize[1].v;

		pIn++;
		pOut++;
	}
}


// 0x1003610b -- the lightmap coordinates alone.
void d3d_SetupLMPolyCoords(WorldPoly *pPoly, LMTLVertex *pOut, SPolyVertex *pIn, int nVerts)
{
	LTVector right, up, vec;
	int i;

	d3d_GetLMAxes((pPoly->m_PageFlags >> 11) & 7, pPoly->m_pNormal, &right, &up);

	for(i=0; i < nVerts; i++)
	{
		vec = *pIn->m_pVec - pPoly->m_LMOrigin;
		pOut->m_LMU = (right.Dot(vec) / g_pSceneWorld->m_LMGridSize + 0.5f) * g_TexelSize[1].u;
		pOut->m_LMV = (up.Dot(vec) / g_pSceneWorld->m_LMGridSize + 0.5f) * g_TexelSize[1].v;

		pIn++;
		pOut++;
	}
}


// 0x100361d9 -- draw a polygon's lightmap alone (the lightmap is bound).
LTBOOL d3d_DrawLightmapOnly(WorldPoly *pPoly)
{
	TLVertex verts[MAX_LMPOLY_VERTS], *pVerts, *pOut;
	SPolyVertex *pIn;
	uint32 i;
	int nVerts;

	pIn = (SPolyVertex*)pPoly->m_Vertices;
	pOut = verts;
	for(i=0; i < pPoly->m_nVertices; i++)
	{
		pOut->m_Vec.x = pIn->m_pVec->x;
		pOut->m_Vec.y = pIn->m_pVec->y;
		pOut->m_Vec.z = pIn->m_pVec->z;
		pOut->m_TU = g_TexelSize[0].u * pIn->m_U;
		pOut->m_TV = g_TexelSize[0].v * pIn->m_V;
		pOut->m_Color = g_WorldColor;
		pIn++;
		pOut++;
	}

	pVerts = verts;
	nVerts = pPoly->m_nVertices;
	if(!d3d_ClipTLVerts(&pVerts, &nVerts, (ViewParams*)&g_ViewParams, 0))
		return LTFALSE;

	g_pD3DDevice->DrawPrimitive(D3DPT_TRIANGLEFAN, D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_SPECULAR | D3DFVF_TEX1,
		pVerts, nVerts, 0);
	return LTTRUE;
}


// 0x10036282 -- a deferred polygon, its gouraud colour lit through the
// light tables, with the screen texture over it at its screen position.
//
// RETAIL BUG: unlike 0x10035e34 there is no check of the vertex count
// against the 64-vertex buffer.
LTBOOL d3d_DrawScreenLMPoly(WorldPoly *pPoly)
{
	LMTLVertex verts[MAX_LMPOLY_VERTS], *pVerts, *pOut;
	CRenderStateList *pStateList;
	SPolyVertex *pIn;
	float tu, tv;
	uint32 i;
	int nVerts;

	pIn = (SPolyVertex*)pPoly->m_Vertices;
	pOut = verts;
	for(i=0; i < pPoly->m_nVertices; i++)
	{
		pOut->m_Vec.x = pIn->m_pVec->x;
		pOut->m_Vec.y = pIn->m_pVec->y;
		pOut->m_Vec.z = pIn->m_pVec->z;
		pOut->m_ColorB[2] = g_LightTableR[pIn->m_Color[2]];
		pOut->m_ColorB[1] = g_LightTableG[pIn->m_Color[1]];
		pOut->m_ColorB[0] = g_LightTableB[pIn->m_Color[0]];
		pOut->m_ColorB[3] = g_VertexAlpha;
		pOut->SetUV(pIn->m_U, pIn->m_V);
		pOut->m_LMU = (g_fScreenCenterX + pIn->m_pVec->x) * g_fScreenScaleX;
		pOut->m_LMV = (g_fScreenCenterY + pIn->m_pVec->z) * g_fScreenScaleY;
		pIn++;
		pOut++;
	}

	pVerts = verts;
	nVerts = pPoly->m_nVertices;
	d3d_LightLMPoly((RenderPoly*)pPoly, (ClipVert2*)verts, nVerts);
	if(!d3d_ClipLMVerts((ClipVert2**)&pVerts, (int32*)&nVerts, &g_ViewParams, 0))
		return LTFALSE;

	d3d_SetTexture(pPoly->m_pSurface->m_pTexture, g_iCurTextureStage, 0);

	pOut = pVerts;
	for(i=nVerts; i; i--)
	{
		tu = pOut->m_TU;
		tv = pOut->m_TV;
		pOut->SetUV(tu * g_TexelSize[0].u, tv * g_TexelSize[0].v);
		pOut++;
	}

	g_SavedStates.Restore();
	pStateList = pPoly->m_pSurface->m_pTexture->m_pStateList;
	if(pStateList)
		g_SavedStates.Apply(pStateList, g_iCurTextureStage);

	g_pD3DDevice->DrawPrimitive(D3DPT_TRIANGLEFAN, FVF_LMTLVERTEX, pVerts, nVerts, 0);
	g_SavedStates.Restore();
	return LTTRUE;
}


// 0x100363ec -- run fn over a queue, or move the polygons whose surface
// asks for it onto *ppDeferred (unless that is -1), freeing the queue's
// entries as it goes when bFree is set.
LTBOOL d3d_FlushPolyQueue(QueuedPoly *pList, QueuedPolyFn fn, LTBOOL bFree, QueuedPoly **ppDeferred)
{
	QueuedPoly *pCur, *pNext, *pNew;
	LTBOOL bRet;

	bRet = LTFALSE;
	for(pCur=pList; pCur; pCur=pNext)
	{
		pNext = pCur->m_pNext;

		if((pCur->m_pPoly->m_pSurface->m_Flags & SURF_NOLIGHTMAP_DEFER) && ppDeferred != (QueuedPoly**)-1)
		{
			pNew = (QueuedPoly*)sb_Allocate(&g_DeferredPolyBank);
			if(pNew)
			{
				bRet |= LTTRUE;
				pNew->m_pPoly = pCur->m_pPoly;
				pNew->m_ClipFlags = pCur->m_ClipFlags;
				pNew->m_pNext = *ppDeferred;
				*ppDeferred = pNew;
			}
		}
		else
		{
			g_ClipFlags = pCur->m_ClipFlags;
			bRet |= fn(pCur->m_pPoly);
		}

		if(bFree)
			sb_Free(&g_DeferredPolyBank, pCur);
	}

	return bRet;
}


// 0x1003647b
void d3d_QueueLightmapPoly(void *pPoly)
{
	d3d_QueuePoly(pPoly, &g_LightmapPolyList, 0)->m_ClipFlags = g_ClipFlags;
}


// 0x10036498 -- the base texture lit by the vertex colour on stage 0,
// the lightmap over it on stage 1.
void d3d_SetLightmapStages()
{
	uint32 colorOp;

	g_pD3DDevice->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
	g_pD3DDevice->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
	g_pD3DDevice->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);

	colorOp = D3DTOP_MODULATE;
	if(g_bLightmapModulate2X)
		colorOp = D3DTOP_MODULATE2X;

	if(g_CV_LightmapsOnly)
		g_pD3DDevice->SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
	else
		g_pD3DDevice->SetTextureStageState(1, D3DTSS_COLOROP, colorOp);

	g_pD3DDevice->SetTextureStageState(1, D3DTSS_COLORARG1, D3DTA_TEXTURE);
	g_pD3DDevice->SetTextureStageState(1, D3DTSS_COLORARG2, D3DTA_CURRENT);
}


// 0x10036534
void d3d_ResetLightmapStages()
{
	g_pD3DDevice->SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_DISABLE);
	g_pD3DDevice->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
}


// ---------------------------------------------------------------------- //
//  0x1003655d -- a rotation (quaternion) to a matrix.
// ---------------------------------------------------------------------- //

void d3d_RotationToMatrix(LTRotation *pRot, LTMatrix *pMat)
{
	static float s, xs, ys, zs, wx, wy, wz, xx, xy, xz, yy, yz, zz;

	s = 2.0f / ((pRot->m_Quat[0]*pRot->m_Quat[0]) + (pRot->m_Quat[1]*pRot->m_Quat[1]) + (pRot->m_Quat[2]*pRot->m_Quat[2]) + (pRot->m_Quat[3]*pRot->m_Quat[3]));

	xs = pRot->m_Quat[0] * s;
	ys = pRot->m_Quat[1] * s;
	zs = pRot->m_Quat[2] * s;

	wx = pRot->m_Quat[3] * xs;
	wy = pRot->m_Quat[3] * ys;
	wz = pRot->m_Quat[3] * zs;

	xx = pRot->m_Quat[0] * xs;
	xy = pRot->m_Quat[0] * ys;
	xz = pRot->m_Quat[0] * zs;

	yy = pRot->m_Quat[1] * ys;
	yz = pRot->m_Quat[1] * zs;

	zz = pRot->m_Quat[2] * zs;

	pMat->m[0][0] = 1.0f - (yy + zz);
	pMat->m[0][1] = xy - wz;
	pMat->m[0][2] = xz + wy;
	pMat->m[0][3] = 0.0f;

	pMat->m[1][0] = xy + wz;
	pMat->m[1][1] = 1.0f - (xx + zz);
	pMat->m[1][2] = yz - wx;
	pMat->m[1][3] = 0.0f;

	pMat->m[2][0] = xz - wy;
	pMat->m[2][1] = yz + wx;
	pMat->m[2][2] = 1.0f - (xx + yy);
	pMat->m[2][3] = 0.0f;

	pMat->m[3][0] = 0.0f;
	pMat->m[3][1] = 0.0f;
	pMat->m[3][2] = 0.0f;
	pMat->m[3][3] = 1.0f;
}


// ---------------------------------------------------------------------- //
//  0x100366da -- walk a world model's BSP along a segment and collect
//  the polygons (of this frame, facing the viewer and the segment) whose
//  bounding sphere the segment passes within its radius of.
// ---------------------------------------------------------------------- //
struct LTPlaneRef
{
	LTVector	m_Normal;                               // +0x00
	float		m_Dist;                                 // +0x0c

	float		DistTo(const LTVector &vec) const	{ return m_Normal.Dot(vec) - m_Dist; }
};

struct BspPoly
{
	uint8		m_Pad[0x18];
	LTVector	m_vCenter;                              // +0x18
	float		m_fRadius;                              // +0x24
	LTPlaneRef	*m_pPlane;                              // +0x28
	Surface		*m_pSurface;                            // +0x2c
	uint8		m_Pad30[0x46 - 0x30];
	uint16		m_wFrameCode;                           // +0x46
};

struct BspNode
{
	BspPoly		*m_pPoly;                               // +0x00
	BspNode		*m_Sides[2];                            // +0x04
	uint8		m_Pad0c[0x16 - 0x0c];
	uint8		m_Flags;                                // +0x16

	LTPlaneRef*	GetPlane()	{ return (m_pPoly && m_pPoly->m_pSurface) ? m_pPoly->m_pPlane : LTNULL; }
};

#define NF_IN		1
#define NF_OUT		2

class BspTree
{
public:
	virtual void		Slot00();
	virtual BspNode*	GetRootNode();                  // +0x04
};

struct SegmentObject
{
	uint8		m_Pad[0x54];
	uint32		m_bHidden;                              // +0x54
	uint8		m_Pad58[0xbc - 0x58];
	uint8		m_ObjectType;                           // +0xbc
	uint8		m_Pad0bd[0x1b0 - 0xbd];
	BspTree		*m_pBsp;                                // +0x1b0
};

#define MAX_SEGMENT_POLIES	256

struct SegmentPolyList
{
	BspPoly		*m_Polies[MAX_SEGMENT_POLIES];          // +0x000
	uint32		m_nPolies;                              // +0x400
};

struct SegmentQuery
{
	SegmentPolyList	*m_pList;                           // +0x00
	LTVector	m_vStart;                               // +0x04
	LTVector	m_vEnd;                                 // +0x10
	float		m_fRadius;                              // +0x1c
};

// The stack's points are plain floats (no constructor to run on the
// static array).
struct SegmentPoint
{
	float		x, y, z;
};

struct SegmentStackEntry
{
	LTVector&	Start()	{ return *(LTVector*)&m_vStart; }
	LTVector&	End()	{ return *(LTVector*)&m_vEnd; }

	BspNode		*m_pNode;                               // +0x00
	SegmentPoint	m_vStart;                           // +0x04
	SegmentPoint	m_vEnd;                             // +0x10
};

#define MAX_SEGMENT_STACK	512

struct SegmentStack
{
	~SegmentStack() {}

	SegmentStackEntry	m_Entries[MAX_SEGMENT_STACK];
};

struct ViewParams
{
	uint8		m_Pad[0x4b4];
	LTVector	m_vViewPos;                             // +0x4b4
};

extern uint16	g_CurFrameCode;                         // 0x1005d238 (common_draw.cpp)

#define OT_WORLDMODEL	2

void d3d_GetSegmentPolies(SegmentObject *pObject, SegmentQuery *pQuery)
{
	static SegmentStack s_Stack;
	SegmentStackEntry *pStack;
	BspNode *pNode;
	SegmentPolyList *pList;
	LTVector pt1, pt2, vDir, vMid;
	float d1, d2, diff, t;
	uint32 i, side;

	if (pObject->m_bHidden || pObject->m_ObjectType != OT_WORLDMODEL)
		return;

	pt1 = pQuery->m_vStart;
	pt2 = pQuery->m_vEnd;
	vDir = pt2 - pQuery->m_vStart;
	vDir.Norm(1.0f);

	pStack = s_Stack.m_Entries;
	pNode = pObject->m_pBsp->GetRootNode();

	for (;;)
	{
		if (!(pNode->m_Flags & (NF_IN|NF_OUT)))
		{
			d1 = pNode->GetPlane()->DistTo(pt1);
			d2 = pNode->GetPlane()->DistTo(pt2);

			if (d1 > pQuery->m_fRadius && d2 > pQuery->m_fRadius)
			{
				pNode = pNode->m_Sides[1];
				continue;
			}

			if (d1 < -pQuery->m_fRadius && d2 < -pQuery->m_fRadius)
			{
				pNode = pNode->m_Sides[0];
				continue;
			}

			side = (d1 > 0.0f) ? 1 : 0;

			diff = d1 - d2;
			if (diff > 0.00001f || diff < -0.00001f)
				t = d1 / diff;
			else
				t = 0.0f;

			vMid = pt1 + (pt2 - pt1) * t;

			if (pNode->m_pPoly->m_wFrameCode == g_CurFrameCode &&
				pQuery->m_pList->m_nPolies < MAX_SEGMENT_POLIES &&
				pNode->GetPlane()->DistTo(g_ViewParams.m_vViewPos) > 1.0f &&
				pNode->GetPlane()->m_Normal.Dot(vDir) < 0.7f &&
				(vMid - pNode->m_pPoly->m_vCenter).Mag() < pNode->m_pPoly->m_fRadius + pQuery->m_fRadius)
			{
				pList = pQuery->m_pList;
				for (i=0; i < pList->m_nPolies; i++)
				{
					if (pList->m_Polies[i] == pNode->m_pPoly)
						break;
				}

				if (i == pList->m_nPolies)
				{
					pList->m_Polies[pList->m_nPolies] = pNode->m_pPoly;
					pQuery->m_pList->m_nPolies++;
				}
			}

			if (pStack < &s_Stack.m_Entries[MAX_SEGMENT_STACK])
			{
				if (t > 0.0f && t < 1.0f)
				{
					pStack->Start() = vMid;
					pStack->End() = pt2;
					pt2 = vMid;
				}
				else
				{
					pStack->Start() = pt1;
					pStack->End() = pt2;
					pStack->m_pNode = pNode->m_Sides[!side];
				}

				pStack->m_pNode = pNode->m_Sides[!side];
				pStack++;
				pNode = pNode->m_Sides[side];
				continue;
			}
		}

		if (pStack == s_Stack.m_Entries)
			return;

		pStack--;
		pNode = pStack->m_pNode;
		pt1 = pStack->Start();
		pt2 = pStack->End();
	}
}
