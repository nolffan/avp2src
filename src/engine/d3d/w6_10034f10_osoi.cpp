// *********************************************************************** //
//
//  MODULE:   W6_10034F10_OS (.CPP)  --  the textured (Optimized2D) blits
//
//  0x10034f10 (1,342 bytes) and 0x10035450 (1,556 bytes), written for the
//  d3d-link: the two blit paths d3dblit.cpp's d3d_BlitToScreen and
//  d3d_BlitWarpedToScreen hand an OPTIMIZED surface to.  /Os (0x90 pads,
//  pushed call arguments).
//
//  An optimized surface (d3dblit.cpp's D3DOptSurface at render data +0x84)
//  is a grid of 64x64 source tiles, each its own texture.  Both functions
//  walk the source rectangle tile by tile and draw one textured quad per
//  tile as a 4-vertex D3DPT_TRIANGLEFAN of D3DFVF_TLVERTEX-shaped vertices
//  (FVF 0x1c4: x, y, z, rhw, diffuse, specular, u, v), modulated by the
//  Optimized2D colour (0x10089448) with the request's +0x14 as alpha.
// *********************************************************************** //

#include "ltbasedefs.h"


// ----------------------------------------------------------------------- //
//  The device, C-style: +0x50 SetRenderState, +0x64 DrawPrimitive, +0x88 GetTexture,
//  +0x8c SetTexture.
// ----------------------------------------------------------------------- //
// Written as the C++ interface: in a COM call the object expression is
// evaluated before the arguments, which is why the shipped 0x10035450
// loads g_pD3DDevice BEFORE it evaluates SetRenderState's third argument.
struct D3DDevice7
{
	virtual void __stdcall v00() = 0;
	virtual void __stdcall v01() = 0;
	virtual void __stdcall v02() = 0;
	virtual void __stdcall v03() = 0;
	virtual void __stdcall v04() = 0;
	virtual void __stdcall v05() = 0;
	virtual void __stdcall v06() = 0;
	virtual void __stdcall v07() = 0;
	virtual void __stdcall v08() = 0;
	virtual void __stdcall v09() = 0;
	virtual void __stdcall v10() = 0;
	virtual void __stdcall v11() = 0;
	virtual void __stdcall v12() = 0;
	virtual void __stdcall v13() = 0;
	virtual void __stdcall v14() = 0;
	virtual void __stdcall v15() = 0;
	virtual void __stdcall v16() = 0;
	virtual void __stdcall v17() = 0;
	virtual void __stdcall v18() = 0;
	virtual void __stdcall v19() = 0;
	virtual long __stdcall SetRenderState(uint32 nState, uint32 nValue) = 0;   // +0x50
	virtual void __stdcall v21() = 0;
	virtual void __stdcall v22() = 0;
	virtual void __stdcall v23() = 0;
	virtual void __stdcall v24() = 0;
	virtual long __stdcall DrawPrimitive(uint32 nPrimType, uint32 nFVF,
	                                     void* pVerts, uint32 nVerts,
	                                     uint32 nFlags) = 0;                   // +0x64
	virtual void __stdcall v26() = 0;
	virtual void __stdcall v27() = 0;
	virtual void __stdcall v28() = 0;
	virtual void __stdcall v29() = 0;
	virtual void __stdcall v30() = 0;
	virtual void __stdcall v31() = 0;
	virtual void __stdcall v32() = 0;
	virtual void __stdcall v33() = 0;
	virtual long __stdcall GetTexture(uint32 nStage, void** ppTexture) = 0;   // +0x88
	virtual long __stdcall SetTexture(uint32 nStage, void* pTexture) = 0;     // +0x8c
};

extern D3DDevice7* g_pD3DDevice;                            // 0x1008d988

#define W6_D3DPT_TRIANGLEFAN    6
#define W6_D3DFVF_TLVERTEX      0x1c4


// ----------------------------------------------------------------------- //
//  d3dblit.cpp's objects.
// ----------------------------------------------------------------------- //
struct D3DOptTile
{
	LTRect		m_Rect;                                     // +0x00, in source pixels
	uint32		m_nWidth;                                   // +0x10, texture size
	uint32		m_nHeight;                                  // +0x14
	void*		m_pTexture;                                 // +0x18
};                                                          // 0x1c

struct D3DOptSurface
{
	uint32		m_nCols;                                    // +0x00
	uint32		m_nRows;                                    // +0x04
	D3DOptTile	m_Tiles[1];                                 // +0x08
};

struct D3DRenderData
{
	void*			m_pSurface;                             // +0x00
	uint8			m_Desc[0x7c];                           // +0x04
	uint32			m_nColorKey;                            // +0x80
	D3DOptSurface*	m_pOptimized;                           // +0x84
	uint32			m_nUnknown88;                           // +0x88
};

struct RenderBlitRequest
{
	D3DRenderData*	m_pRenderData;                          // +0x00
	uint32			m_nFlags;                               // +0x04
	uint32			m_nColorKey;                            // +0x08
	LTRect*			m_pSrcRect;                             // +0x0c
	LTRect*			m_pDestRect;                            // +0x10
	float			m_fAlpha14;                             // +0x14, 0..1
	uint32			m_nUnknown18;                           // +0x18
};

// One D3DTLVERTEX.
struct W6TLVert
{
	float		x, y, z, rhw;                               // +0x00
	union
	{
		uint32	color;                                      // +0x10
		struct
		{
			uint8	b, g, r, a;
		};
	};
	uint32		specular;                                   // +0x14
	float		tu, tv;                                     // +0x18
};                                                          // 0x20

#define OPT_TILE_SIZE           64

extern uint32 g_Optimized2DColor;                           // 0x10089448
extern float  g_1008944c;                                   // 0x1008944c, 0.01f: the pixel-centre bias

extern void d3d_AddDirtyRect(LTRect* pRect);                // 0x1003b3d0


// ======================================================================= //
//  0x10034f10 -- the straight textured blit (d3d_BlitToScreen's path).
//
//  Source rect -> destination rect, scaled by the two rects' size ratios
//  (doubles).  For each 64x64 tile the source rect touches: the part of
//  it inside the rect, mapped to the screen, biased by -0.01, clamped at
//  0; texture coordinates are the part's offsets in the tile over the
//  tile texture's size.  Stage 0's texture is saved first and put back
//  after; the destination rect is added to the dirty list.
//
//  NEAR: 358 instructions against 357, 1,344 bytes against 1,342, four
//  blocks: one colour store (-0x100) scheduled late, the `mov $1.0f ;
//  add $-12,%esp` pair two stores early, and the v1.tv store hoisted above
//  the v2/v3 tv pair (one extra fxch).  Was 1,352 / 23 blocks; closed by
//  the column tile in pTile (`pTile = &pOpt->m_Tiles[nTileX]`, which is
//  what spills nTileX and keeps the LTMIN result in a register), the rhw
//  chain 0,1,2,3, and the y(3,2) pair before the z chain.  No better: all
//  24 colour chain orders, the four middle vertex statements permuted, the
//  z chain at every position, the v0/v1 tv pair split, reversed or moved,
//  the z chain reversed.  What was measured and kept before that:
//    * the four rect extents are FLOAT locals: icl spills them as doubles
//      (`fildl ; fstpl -0x38(%ebp)`); declared double they force the
//      aligned-stack prologue (`push %ebx ; mov %esp,%ebx ; and $-8`)
//      that the shipped function does not have;
//    * `xEnd = x + 64` as its own statement: the shipped LTMIN pair reuses
//      one `lea 0x40(%edi)` for both evaluations of the inner LTMIN;
//    * the tile as `&m_Tiles[row * nCols] + col` (38 -> 23 mnemonic
//      blocks; the shipped code builds the row base first);
//    * the pairs of each chained vertex store in the shipped order.
// ======================================================================= //
void sub_10034f10(RenderBlitRequest* pRequest)
{
	D3DOptSurface* pOpt;
	D3DOptTile*    pTile;
	LTRect*        pSrc;
	LTRect*        pDest;
	W6TLVert       verts[4];
	void*          pOldTexture;
	float          fSrcW, fSrcH, fDestW, fDestH;
	float          fX0, fX1, fY0, fY1;
	float          fU0, fU1, fV0, fV1;
	float          fInvW, fInvH;
	uint32         nAlpha;
	int32          x, y, xEnd, yEnd;
	int32          nTileX, nTileY;

	pOpt = pRequest->m_pRenderData->m_pOptimized;
	if (!pOpt)
		return;

	pSrc  = pRequest->m_pSrcRect;
	pDest = pRequest->m_pDestRect;

	fSrcW  = (float)(pSrc->right - pSrc->left);
	fSrcH  = (float)(pSrc->bottom - pSrc->top);
	fDestW = (float)(pDest->right - pDest->left);
	fDestH = (float)(pDest->bottom - pDest->top);

	verts[0].color = verts[3].color = verts[2].color = verts[1].color = g_Optimized2DColor;

	nAlpha = (uint32)(pRequest->m_fAlpha14 * 255.0f);
	verts[0].a = verts[1].a = verts[2].a = verts[3].a = (uint8)nAlpha;

	verts[0].rhw = verts[1].rhw = verts[2].rhw = verts[3].rhw = 1.0f;

	pOldTexture = LTNULL;
	g_pD3DDevice->GetTexture(0, &pOldTexture);

	for (x = pSrc->left; x < pSrc->right; x = xEnd)
	{
		xEnd  = x + OPT_TILE_SIZE;
		nTileX = x / OPT_TILE_SIZE;
		pTile = &pOpt->m_Tiles[nTileX];
		xEnd  = LTMIN(pTile->m_Rect.right,
		              LTMIN(xEnd, pSrc->right));

		for (y = pSrc->top; y < pSrc->bottom; y = yEnd)
		{
			yEnd  = y + OPT_TILE_SIZE;
			nTileY = y / OPT_TILE_SIZE;
			pTile = &pOpt->m_Tiles[nTileY * pOpt->m_nCols] + nTileX;
			yEnd  = LTMIN(pTile->m_Rect.bottom,
			              LTMIN(yEnd, pSrc->bottom));

			fX0 = (x    - pSrc->left) * (fDestW / fSrcW) + pDest->left;
			fX1 = (xEnd - pSrc->left) * (fDestW / fSrcW) + pDest->left;
			fY0 = (y    - pSrc->top)  * (fDestH / fSrcH) + pDest->top;
			fY1 = (yEnd - pSrc->top)  * (fDestH / fSrcH) + pDest->top;

			fInvW = 1.0f / (float)pTile->m_nWidth;
			fU0 = (x    - pTile->m_Rect.left) * fInvW;
			fU1 = (xEnd - pTile->m_Rect.left) * fInvW;

			fInvH = 1.0f / (float)pTile->m_nHeight;
			fV0 = (y    - pTile->m_Rect.top) * fInvH;
			fV1 = (yEnd - pTile->m_Rect.top) * fInvH;

			fX0 = LTMAX(0.0f, fX0 - g_1008944c);
			fY0 = LTMAX(0.0f, fY0 - g_1008944c);
			fX1 = LTMAX(0.0f, fX1 - g_1008944c);
			fY1 = LTMAX(0.0f, fY1 - g_1008944c);

			verts[0].tu = verts[3].tu = fU0;
			verts[3].y  = verts[2].y = fY1;
			verts[0].z  = verts[3].z = verts[2].z = verts[1].z = 1.0f;
			verts[2].tu = verts[1].tu = fU1;
			verts[2].tv = verts[3].tv = fV1;
			verts[0].tv = verts[1].tv = fV0;
			verts[0].x  = verts[3].x = fX0;
			verts[2].x  = verts[1].x = fX1;
			verts[0].y  = verts[1].y = fY0;

			g_pD3DDevice->SetTexture(0, pTile->m_pTexture);
			g_pD3DDevice->DrawPrimitive(W6_D3DPT_TRIANGLEFAN,
			                                    W6_D3DFVF_TLVERTEX, verts, 4, 0);
		}
	}

	g_pD3DDevice->SetTexture(0, pOldTexture);

	d3d_AddDirtyRect(pDest);
}


// ======================================================================= //
//  0x10035450 -- the warped textured blit (d3d_BlitWarpedToScreen's path).
//
//  The request's +0x18 is the engine's LTWarpPt array (source x/y, dest
//  x/y; only the dest pair of points 0, 1 and 3 is read).  The whole
//  surface is drawn, tile by tile, onto the parallelogram those points
//  span: vRight = pt1 - pt0 and vDown = pt3 - pt0, each rescaled
//  (LTVector::Norm) to screen units per source pixel -- |v| / source
//  width or height -- so a tile at source (x, y) of size (w, h) lands at
//  origin + vRight*x + vDown*y with its other corners +vRight*w, +vDown*h.
//  z is 1 (the origin's), rhw 1; every tile's texture spans 0..1.
//  Alpha blending is switched on unless the surface has no +0x88 and the
//  alpha is exactly 1.  Stage 0's texture is saved and restored.
//
//  /Oi-: the two magnitudes and the two in Norm() are CALLS to the CRT
//  sqrt (0x1007fc98), which is why this file is _osoi.
//
//  BYTE-IDENTICAL.  What it took, each measured:
//    * the device as a C++ interface (see D3DDevice7 above): the shipped
//      code loads g_pD3DDevice before evaluating SetRenderState's third
//      argument, the object-expression-first order of a COM method call;
//    * the origin as an LTVector local (icl scalar-replaces it: x and y
//      spilled as doubles, z = 1.0f constant-propagated into the first
//      vertex's z), assigned BEFORE vRight, so vDown reads it from the
//      spills rather than re-reading pt0;
//    * `(float)xPos` / `(float)yPos` inline in the inner loop (icl hoists
//      the y conversion to the inner preheader);
//    * a twelve-vertex array: the frame is 0x270, 0x100 more than four
//      vertices need, and the array sits at -0x254 with the temporaries
//      below it;
//    * the colour / rhw chains and the eight texture-coordinate stores in
//      the orders below, which are what make icl emit the shipped store
//      order (it does not keep source order for these).
// ======================================================================= //
// LTWarpPt is ltbasetypes.h's: source_x, source_y, dest_x, dest_y (0x10).

struct W6TLVertV
{
	LTVector	pos;                                        // +0x00
	float		rhw;                                        // +0x0c
	union
	{
		uint32	color;                                      // +0x10
		struct
		{
			uint8	b, g, r, a;
		};
	};
	uint32		specular;                                   // +0x14
	float		tu, tv;                                     // +0x18
};                                                          // 0x20

#define W6_D3DRS_ALPHABLENDENABLE   27


void sub_10035450(RenderBlitRequest* pRequest)
{
	D3DOptSurface* pOpt;
	D3DOptTile*    pTile;
	LTWarpPt*      pPts;
	LTRect*        pSrc;
	W6TLVertV      verts[12];                   // 0x180 bytes of frame; four used
	void*          pOldTexture;
	LTVector       vRight, vDown;
	float          fSrcW, fSrcH;
	LTVector       vOrigin;
	float          fScaleRight, fScaleDown;
	uint32         nAlpha;
	uint32         nCols, nRows;
	uint32         iRow, iCol;
	uint32         xPos, yPos;

	pPts = (LTWarpPt*)pRequest->m_nUnknown18;
	pOldTexture = LTNULL;

	pOpt = pRequest->m_pRenderData->m_pOptimized;
	if (!pOpt)
		return;

	pSrc  = pRequest->m_pSrcRect;
	fSrcW = (float)(pSrc->right - pSrc->left);
	fSrcH = (float)(pSrc->bottom - pSrc->top);

	nCols = pOpt->m_nCols;
	nRows = pOpt->m_nRows;

	g_pD3DDevice->SetRenderState(W6_D3DRS_ALPHABLENDENABLE,
		pRequest->m_pRenderData->m_nUnknown88 || pRequest->m_fAlpha14 != 1.0f);

	g_pD3DDevice->GetTexture(0, &pOldTexture);

	verts[0].color = verts[3].color = verts[2].color = verts[1].color = g_Optimized2DColor;

	nAlpha = (uint32)(pRequest->m_fAlpha14 * 255.0f);
	verts[0].a = verts[1].a = verts[2].a = verts[3].a = (uint8)nAlpha;

	verts[0].rhw = verts[1].rhw = verts[2].rhw = verts[3].rhw = 1.0f;

	vOrigin = LTVector(pPts[0].dest_x, pPts[0].dest_y, 1.0f);

	vRight = LTVector(pPts[1].dest_x - pPts[0].dest_x,
	                  pPts[1].dest_y - pPts[0].dest_y, 0.0f);

	vDown = LTVector(pPts[3].dest_x - vOrigin.x, pPts[3].dest_y - vOrigin.y, 0.0f);

	fScaleRight = vRight.Mag() / fSrcW;
	fScaleDown  = vDown.Mag() / fSrcH;

	vRight.Norm(fScaleRight);
	vDown.Norm(fScaleDown);

	yPos = 0;
	for (iRow = 0; iRow < nRows; iRow++)
	{
		xPos = 0;

		for (iCol = 0; iCol < nCols; iCol++)
		{
			pTile = &pOpt->m_Tiles[iRow * nCols + iCol];

			verts[0].pos = vOrigin + vRight * (float)xPos + vDown * (float)yPos;
			verts[1].pos = verts[0].pos + vRight * (float)pTile->m_nWidth;
			verts[2].pos = verts[0].pos + vRight * (float)pTile->m_nWidth +
			               vDown * (float)pTile->m_nHeight;
			verts[3].pos = verts[0].pos + vDown * (float)pTile->m_nHeight;

			verts[1].tu = 1.0f;
			verts[2].tu = 1.0f;
			verts[2].tv = 1.0f;
			verts[3].tv = 1.0f;
			verts[0].tv = 0.0f;
			verts[0].tu = 0.0f;
			verts[3].tu = 0.0f;
			verts[1].tv = 0.0f;

			g_pD3DDevice->SetTexture(0, pTile->m_pTexture);
			g_pD3DDevice->DrawPrimitive(W6_D3DPT_TRIANGLEFAN,
			                                    W6_D3DFVF_TLVERTEX, verts, 4, 0);

			xPos += pTile->m_nWidth;
		}

		yPos += pOpt->m_Tiles[iRow * nCols].m_nHeight;
	}

	g_pD3DDevice->SetTexture(0, pOldTexture);
}
