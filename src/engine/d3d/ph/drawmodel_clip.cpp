// *********************************************************************** //
//
//  ph/drawmodel_clip.cpp -- Primal Hunt's d3d.ren, TU04:
//  0x10007930..0x10008c72, /O1.
//
//  World polygon drawing: the deferred polygons, texture binding, the
//  lightmapped polygon draw and its clipper, which is ph/d3d_clip.h's
//  ClipPoly over the 40-byte vertex (its out-of-line copies are this
//  object's: ClipPoly<ClipVert2> 0x10008779, ClipNear and ClipLeft
//  0x10008895, 0x100089cc, the edge functions 0x10008b01..0x10008c72).
//  The name is the map's guess.
//
//  The SDK and StdLith inlines this object calls out of line are its
//  COMDATs: sb_Allocate (0x10007e36), LTMatrix::Inverse (0x10008197),
//  LTMatrix::Apply4x4 (0x1000832e), _CVector::MagSqr (0x100085d3),
//  MatVMul_InPlace_H (0x1000868d), with a counter adder's destructor
//  (0x100083ae), each after the first function to use it.
//
//  Not yet written: 0x10007b41 (the lightmapped polygon, 666 bytes),
//  0x10007e5d (its dynamic lights, 826 bytes, the user of Inverse,
//  Apply4x4 and the counter adder) and 0x100083ec (487 bytes).
//
//  Near: 0x100079e4 (d3d_SetTexture), where retail keeps the second test
//  of pRenderData that cl 8966 here threads away (164 against 165).
//
// *********************************************************************** //

#include <d3d.h>

#include "ltbasedefs.h"
#include "..\..\LT2\lithshared\stdlith\struct_bank.h"
#include "d3d_clip.h"




// The render context's projection, and the texture size the half-texel
// offsets are scaled for.
struct RenderContext
{
	void*       m_pMainWorld;                               // +0x00
	float       m_fCenterX;                                 // +0x04
	float       m_fCenterY;                                 // +0x08
	float       m_fWidth;                                   // +0x0c
	float       m_fHeight;                                  // +0x10
};

struct TexelOffset
{
	void        Init(float fU, float fV) { m_fU = fU; m_fV = fV; }

	float       m_fU;
	float       m_fV;
};

// A model piece deferred until the end of the model.
struct DeferredPiece
{
	void*       m_pPiece;                                   // +0x00
	uint8       m_pad_04[0x0c - 0x04];
	uint32      m_ClipFlags;                                // +0x0c
	DeferredPiece* m_pNext;                                 // +0x10
};

extern RenderContext*   g_pRenderContext;                   // 0x1005b778
extern TexelOffset      g_TexelOffsets[];                   // 0x100672a8
extern uint32           g_iTexelOffset;                     // 0x100622d0
extern uint32           g_iCurTextureStage;                 // 0x100638c0
extern uint32           g_ClipFlags;                        // 0x1005bd0c
extern StructBank       g_DeferredPolyBank;                 // 0x1005e1f0

struct SharedTexture;
int     d3d_SetTexture(SharedTexture* pTexture, uint32 iStage, uint32 iLOD); // 0x100079e4
void    d3d_DrawDeferredPiece(void* pPiece);                // 0x10007b41

static float            g_fScreenScaleX;                    // 0x10052bd8
static float            g_fScreenScaleY;                    // 0x10052bdc
static float            g_fScreenCenterX;                   // 0x10053fe0
static float            g_fScreenCenterY;                   // 0x10053fe4
static DeferredPiece*   g_pDeferredPieces;                  // 0x10053fe8


// 0x10007930
void d3d_SetupScreenScale()
{
	if (g_pRenderContext)
	{
		g_fScreenScaleX = 1.0f / g_pRenderContext->m_fWidth * g_TexelOffsets[g_iTexelOffset].m_fU;
		g_fScreenScaleY = 1.0f / g_pRenderContext->m_fHeight * g_TexelOffsets[g_iTexelOffset].m_fV;
		*(uint32*)&g_fScreenCenterX = *(uint32*)&g_pRenderContext->m_fCenterX;
		*(uint32*)&g_fScreenCenterY = *(uint32*)&g_pRenderContext->m_fCenterY;
	}
}

// 0x10007976 -- draw the deferred pieces and free them.
void d3d_DrawDeferredPieces()
{
	DeferredPiece *pCur, *pNext;

	if (!g_pDeferredPieces)
		return;

	d3d_SetTexture((SharedTexture*)g_pRenderContext->m_pMainWorld, g_iCurTextureStage, 0);
	d3d_SetupScreenScale();

	pCur = g_pDeferredPieces;
	while (pCur)
	{
		pNext = pCur->m_pNext;
		g_ClipFlags = pCur->m_ClipFlags;
		d3d_DrawDeferredPiece(pCur->m_pPiece);
		sb_Free(&g_DeferredPolyBank, pCur);
		pCur = pNext;
	}

	g_pDeferredPieces = LTNULL;
}


// ----------------------------------------------------------------------- //
//  Texture binding.
// ----------------------------------------------------------------------- //

// A texture in video memory (d3d_texture.cpp's RTexture), with what the
// binding reads.
class RTexture
{
public:
	void*       m_pVTable;                                  // +0x00
	float       m_fInvWidth;                                // +0x04
	float       m_fInvHeight;                               // +0x08
	LPDIRECTDRAWSURFACE7 m_pSurface;                        // +0x0c
	uint32      m_MemoryUse;                                // +0x10
	uint16      m_FrameCode;                                // +0x14
	uint16      m_AlphaRef;                                 // +0x16
	RTexture*   m_pTexture;                                 // +0x18
	LTLink      m_Link;                                     // +0x1c
	uint16      m_Width;                                    // +0x28
	uint16      m_Height;                                   // +0x2a
	SharedTexture* m_pSharedTexture;                        // +0x2c
	RTexture*   m_pNext;                                    // +0x30
	uint8       m_pad_34[0x42 - 0x34];                      // +0x34
	uint16      m_Stage;                                    // +0x42
	uint16      m_LOD;                                      // +0x44
};

struct SharedTexture
{
	uint8       m_pad_00[0x0c];                             // +0x00
	RTexture*   m_pRenderData;                              // +0x0c
	uint8       m_pad_10[0x30 - 0x10];                      // +0x10
	uint16      m_FrameCode;                                // +0x30
};

struct RenderStruct
{
	uint8       m_pad_00[0x48];                             // +0x00
	uint32      m_TextureMemoryUsed;                        // +0x48
};

extern RenderStruct*        g_pStruct;                      // 0x1005df08
extern LPDIRECT3DDEVICE7    g_pD3DDevice;                   // 0x100638c8
extern LTLink               g_RTextures;                    // 0x10068300
extern RTexture*            g_pCurTextures[];               // 0x10067270
extern uint16               g_CurFrameCode;                 // 0x1005d238
extern uint16               g_CurTextureFrameCode;          // 0x1005d250
extern uint32               g_nTextureChanges;              // 0x1005d22c

RTexture* d3d_CreateRenderTexture(SharedTexture* pTexture, uint32 iStage,
                                  LTBOOL bStageCopy);       // 0x10020cd0
void d3d_SetTextureDirect(RTexture* pTexture);              // 0x10007a89

// 0x100079e4 -- bind a shared texture to a stage at a mip level: its copy
// for that stage, made (and chained to the others) if there is none yet.
int d3d_SetTexture(SharedTexture* pTexture, uint32 iStage, uint32 iLOD)
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

// 0x10007a89 -- put a texture on its stage: first use this frame moves it
// to the end of the texture list (most recently used) and counts its
// memory.
void d3d_SetTextureDirect(RTexture* pTexture)
{
	g_pCurTextures[pTexture->m_Stage] = pTexture;

	if (pTexture->m_FrameCode != g_CurFrameCode)
	{
		dl_Remove(&pTexture->m_Link);
		dl_Insert(g_RTextures.m_pPrev, &pTexture->m_Link);
		g_pStruct->m_TextureMemoryUsed += pTexture->m_MemoryUse;
		pTexture->m_FrameCode = g_CurFrameCode;
	}

	g_pD3DDevice->SetTexture(pTexture->m_Stage & 0xFF, pTexture->m_pSurface);
	if (pTexture->m_AlphaRef)
		g_pD3DDevice->SetRenderState(D3DRENDERSTATE_ALPHAREF, pTexture->m_AlphaRef);

	g_TexelOffsets[pTexture->m_Stage].Init(pTexture->m_fInvWidth, pTexture->m_fInvHeight);
	g_nTextureChanges++;
}


// ----------------------------------------------------------------------- //
//  Polygon queues.
// ----------------------------------------------------------------------- //

// A queued polygon (d3d_draw.cpp's DeferredPoly).
struct QueuedPoly
{
	void*       m_pPoly;                                    // +0x00
	uint8       m_pad_04[0x0c - 0x04];                      // +0x04
	uint32      m_ClipFlags;                                // +0x0c
	QueuedPoly* m_pNext;                                    // +0x10
};

// A texture's queue of polygons, one per kind, chained on a list.
struct TexQueue
{
	void*       m_pTexture;                                 // +0x00
	QueuedPoly* m_pPolys;                                   // +0x04
	TexQueue*   m_pNext;                                    // +0x08
};

struct QueueTexture
{
	uint8       m_pad_00[0x1c];                             // +0x00
	TexQueue*   m_Queues[4];                                // +0x1c
};

struct QueueSurface
{
	uint8       m_pad_00[0x2c];                             // +0x00
	QueueTexture* m_pTexture;                               // +0x2c
};

struct QueuePolyHdr
{
	uint8       m_pad_00[0x2c];                             // +0x00
	QueueSurface* m_pSurface;                               // +0x2c
};

extern StructBank       g_ClipPointBank;                    // 0x1005e730

// 0x10007ddb -- queue a polygon on its texture's queue of that kind,
// starting the queue (and chaining it on pList) on the texture's first.
QueuedPoly* d3d_QueuePoly(void* pPoly, void* pList, uint32 nType)
{
	QueueTexture* pTexture = ((QueuePolyHdr*)pPoly)->m_pSurface->m_pTexture;
	TexQueue** ppQueue = &pTexture->m_Queues[nType];
	TexQueue* pQueue = *ppQueue;
	QueuedPoly* pQueued;

	if (!pQueue)
	{
		pQueue = (TexQueue*)sb_Allocate(&g_ClipPointBank);
		pQueue->m_pTexture = pTexture;
		*ppQueue = pQueue;
		pQueue->m_pPolys = LTNULL;
		pQueue->m_pNext = *(TexQueue**)pList;
		*(TexQueue**)pList = pQueue;
	}

	pQueued = (QueuedPoly*)sb_Allocate(&g_DeferredPolyBank);
	pQueued->m_pNext = pQueue->m_pPolys;
	pQueue->m_pPolys = pQueued;
	pQueued->m_pPoly = pPoly;
	return pQueued;
}

// 0x100083bf -- the lightmapped polygon draw while deferring: the polygon
// is kept, with the clip flags, for d3d_DrawDeferredPieces.
void d3d_DeferLightmapPoly(void* pPoly)
{
	DeferredPiece* pDeferred;

	pDeferred = (DeferredPiece*)sb_Allocate(&g_DeferredPolyBank);
	if (pDeferred)
	{
		pDeferred->m_ClipFlags = g_ClipFlags;
		pDeferred->m_pNext = g_pDeferredPieces;
		g_pDeferredPieces = pDeferred;
		pDeferred->m_pPiece = pPoly;
	}
}

// ----------------------------------------------------------------------- //
//  The lightmapped polygons' vertices: transformed and projected, or
//  taken to view space, clipped and then projected.
// ----------------------------------------------------------------------- //

struct ViewParams
{
	uint8       m_pad_000[0x15c];                           // +0x000
	LTMatrix    m_mWorldToCamera;                           // +0x15c
	uint8       m_pad_19c[0x1dc - 0x19c];                   // +0x19c
	LTMatrix    m_mFullTransform;                           // +0x1dc
};

void d3d_ProjectTLVert(ClipVert2* pVert, ViewParams* pParams); // 0x1002739d

// 0x10008719 -- a vertex into view space.
inline void d3d_TransformToCamera(ClipVert2* pVert, LTMatrix* pMat)
{
	float x = pMat->m[0][0]*pVert->m_vPos.x + pMat->m[0][1]*pVert->m_vPos.y + pMat->m[0][2]*pVert->m_vPos.z + pMat->m[0][3];
	float y = pMat->m[1][0]*pVert->m_vPos.x + pMat->m[1][1]*pVert->m_vPos.y + pMat->m[1][2]*pVert->m_vPos.z + pMat->m[1][3];
	float z = pMat->m[2][0]*pVert->m_vPos.x + pMat->m[2][1]*pVert->m_vPos.y + pMat->m[2][2]*pVert->m_vPos.z + pMat->m[2][3];

	pVert->m_vPos.z = z;
	pVert->m_vPos.x = x;
	pVert->m_vPos.y = y;
}

// 0x100085f2 -- with nothing to clip against, the vertices go straight
// through the full transform; otherwise to view space, through the
// clipper and then projected.
LTBOOL d3d_ClipLMVerts(ClipVert2** ppVerts, int32* pnVerts, ViewParams* pParams)
{
	ClipVert2* pVert;
	int32 nVerts;

	if (!g_ClipFlags)
	{
		pVert = *ppVerts;
		for (nVerts = *pnVerts; nVerts; nVerts--)
		{
			pVert->m_fRHW = MatVMul_InPlace_H(&pParams->m_mFullTransform, &pVert->m_vPos);
			pVert++;
		}
		return LTTRUE;
	}

	pVert = *ppVerts;
	for (nVerts = *pnVerts; nVerts; nVerts--)
	{
		d3d_TransformToCamera(pVert, &pParams->m_mWorldToCamera);
		pVert++;
	}

	if (!ClipPoly(g_ClipFlags, ppVerts, pnVerts))
		return LTFALSE;

	pVert = *ppVerts;
	for (nVerts = *pnVerts; nVerts; nVerts--)
	{
		d3d_ProjectTLVert(pVert, pParams);
		pVert++;
	}

	return LTTRUE;
}
