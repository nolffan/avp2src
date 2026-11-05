// *********************************************************************** //
//
//  ph/drawmodel_clip.cpp -- Primal Hunt's d3d.ren, TU04:
//  0x10007930..0x10008c72, /O1.
//
//  Model clipping: the deferred (clipped) model pieces, drawn after the
//  rest of the model, and the polygon clippers' edge intersections.
//
// *********************************************************************** //

#include "ltbasedefs.h"
#include "..\..\LT2\lithshared\stdlith\struct_bank.h"


#define CLIP_EPSILON    0.00001f


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
extern void*            g_pDevice;                          // 0x100638c0
extern uint32           g_ClipFlags;                        // 0x1005bd0c
extern StructBank       g_DeferredPieceBank;                // 0x1005e1f0

void    d3d_SetupModelProjection(void* pWorld, void* pDevice, uint32 nFlags); // 0x100079e4
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

	d3d_SetupModelProjection(g_pRenderContext->m_pMainWorld, g_pDevice, 0);
	d3d_SetupScreenScale();

	pCur = g_pDeferredPieces;
	while (pCur)
	{
		pNext = pCur->m_pNext;
		g_ClipFlags = pCur->m_ClipFlags;
		d3d_DrawDeferredPiece(pCur->m_pPiece);
		sb_Free(&g_DeferredPieceBank, pCur);
		pCur = pNext;
	}

	g_pDeferredPieces = LTNULL;
}


extern float g_fFarZ;                                       // 0x1005b820

// 0x10008b01 -- the intersection with the top plane (y == z).
float ClipEdgeTop(LTVector* a, LTVector* b, LTVector* dst)
{
	float fDenom = (b->y - a->y) - b->z + a->z;
	float t;
	float fZ;

	if (fDenom < -CLIP_EPSILON || fDenom > CLIP_EPSILON)
		t = -(a->y - a->z) / fDenom;
	else
		t = 0.0f;

	dst->x = (b->x - a->x) * t + a->x;
	fZ = (b->z - a->z) * t + a->z;
	dst->z = fZ;
	dst->y = fZ;

	return t;
}

// 0x10008b5d -- the right plane (x == z).
float ClipEdgeRight(LTVector* a, LTVector* b, LTVector* dst)
{
	float fDenom = (b->x - a->x) - b->z + a->z;
	float t;
	float fZ;

	if (fDenom < -CLIP_EPSILON || fDenom > CLIP_EPSILON)
		t = -(a->x - a->z) / fDenom;
	else
		t = 0.0f;

	dst->y = (b->y - a->y) * t + a->y;
	fZ = (b->z - a->z) * t + a->z;
	dst->z = fZ;
	dst->x = fZ;

	return t;
}

// 0x10008bb9 -- the bottom plane (y == -z). NEAR: retail adds a->z + a->y,
// in either spelling cl 8966 here loads a->y first.
float ClipEdgeBottom(LTVector* a, LTVector* b, LTVector* dst)
{
	float fDenom = (b->y - a->y) + b->z - a->z;
	float t;
	float fZ;

	if (fDenom < -CLIP_EPSILON || fDenom > CLIP_EPSILON)
		t = -(a->y + a->z) / fDenom;
	else
		t = 0.0f;

	dst->x = (b->x - a->x) * t + a->x;
	fZ = (b->z - a->z) * t + a->z;
	dst->z = fZ;
	dst->y = -fZ;

	return t;
}

// 0x10008c17 -- the far plane (z == g_fFarZ).
float ClipEdgeFar(LTVector* a, LTVector* b, LTVector* dst)
{
	float fDenom = b->z - a->z;
	float t;

	if (fDenom < -CLIP_EPSILON || fDenom > CLIP_EPSILON)
		t = (g_fFarZ - a->z) / fDenom;
	else
		t = 0.0f;

	dst->x = (b->x - a->x) * t + a->x;
	dst->y = (b->y - a->y) * t + a->y;
	dst->z = g_fFarZ;

	return t;
}
