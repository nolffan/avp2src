// *********************************************************************** //
//  ph/drawlightmaps.cpp -- Primal Hunt's d3d.ren, TU40
//  (0x10035c7e..0x10036a96), /O1.
//
//  The world's lightmap pass: the polygons queued for it, their lightmap
//  texture coordinates, and the texture stages that blend the lightmap
//  over the base texture.  No string names this object; "drawlightmaps"
//  is a guess.  .bss from 0x10082ebc.
//
//  Written: the coordinate fills 0x1003600e, 0x1003610b, the lightmap-only
//  draw 0x100361d9 (a near: the loop's stores are scheduled differently),
//  the queue walker 0x100363ec, the queue 0x1003647b and the stage set-up
//  and reset 0x10036498 (a near: retail keeps 0, 1 and 2 in registers),
//  0x10036534.  Not yet: the pass itself 0x10035c7e, the per-polygon
//  draws 0x10035e34, 0x10036282, and 0x1003655d, 0x100366da.
// *********************************************************************** //

#include <d3d.h>

#include "ltbasedefs.h"
#include "../../LT2/lithshared/stdlith/struct_bank.h"


struct Surface
{
	uint8		m_Pad[0x30];
	uint32		m_Flags;		// +0x30
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
	uint8		m_Pad30[0x38 - 0x30];
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
	uint8		m_pad_0c[0x18 - 0x0c];
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
	uint32		m_Color;                                // +0x10
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
extern uint32				g_VertexColor;              // 0x1005c154
extern TexPair				g_TexelSize[];              // 0x100672a8

struct ViewParams;
extern ViewParams			g_ViewParams;               // 0x1005b790

LTBOOL	d3d_ClipTLVerts(TLVertex** ppVerts, int* pnVerts, ViewParams* pParams, uint32 flags);  // 0x1000aec6
void d3d_GetLMAxes(int iBasis, LTVector *pNormal, LTVector *pRight, LTVector *pUp);	// 0x10035860
extern QueuedPoly*	d3d_QueuePoly(void* pPoly, void* pList, uint32 nType);  // 0x10007ddb

QueuedPoly*	g_LightmapPolyList;                         // 0x1005fda0
LTBOOL		g_bLightmapModulate2X;                      // 0x10082ebc


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
		pOut->m_Color = g_VertexColor;
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


#define MAX_LMPOLY_VERTS		64

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
		pOut->m_Color = g_VertexColor;
		pIn++;
		pOut++;
	}

	pVerts = verts;
	nVerts = pPoly->m_nVertices;
	if(!d3d_ClipTLVerts(&pVerts, &nVerts, &g_ViewParams, 0))
		return LTFALSE;

	g_pD3DDevice->DrawPrimitive(D3DPT_TRIANGLEFAN, D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_SPECULAR | D3DFVF_TEX1,
		pVerts, nVerts, 0);
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
