// *********************************************************************** //
//  ph/d3d_optimizedsurface.cpp -- Primal Hunt's d3d.ren, TU18
//  (0x1001cb20..0x1001e650), /O1.
//
//  The map puts 0x1001ca40 (the gamma ramp loader, LOADGAMMA) here, but
//  it is /O2 code padded to 16: it is TU17's (d3d_init.cpp).
//
//  Optimised 2D surfaces: a 2D surface cut into 64x64 textures (the
//  RenderData's m_pOptimized) so it can be drawn with Direct3D while in 3D
//  mode, and the Optimized2D render state (blend and colour).  NOLF's d3dopt2d.cpp, w53_10033f30_osoiqip.cpp and
//  w81_10034ed0_osg5.cpp hold the same code a version earlier.
//
//  Then the dynamic lightmap textures and their allocator (0x1001dda5..
//  0x1001e64d: the texture, vtables 0x100482f0/0x10048310, and the
//  allocator, vtables 0x10048330/0x1004833c, singleton 0x1007c1e4).
//
//  The tile fill, its alpha passes, the blit and the warp follow NOLF2's
//  d3d_optimizedsurface.cpp (render_a), this version's DirectDraw 7 form.
//
//  Near: 0x1001d890 (the warp) -- three spill reloads after the inner
//  loop come back in another order (register allocation).  Its inline
//  budget is NOLF2's d3d_GetTextureDirect/d3d_SetTextureDirect wrappers:
//  without them cl 8966 expands two more _CVector constructors than
//  retail does.
// *********************************************************************** //

#include <windows.h>
#include <ddraw.h>
#include <d3d.h>
#include <stdio.h>
#include <string.h>

#include "ltbasedefs.h"
#include "ltpvalue.h"


// ------------------------------------------------------------------------------ //
// Declarations.
// ------------------------------------------------------------------------------ //

enum BPPIdent
{
	BPP_8P=0,
	BPP_8,
	BPP_16,
	BPP_32
};

class PFormat
{
public:
	virtual void	Init(BPPIdent Type, uint32 aMask, uint32 rMask, uint32 gMask, uint32 bMask);

	uint32			GetBPPShift();                      // 0x100374c1
	uint32			GetBytesPerPixel();                 // 0x10037506

	BPPIdent	m_eType;
	uint32		m_Masks[4];
	uint32		m_nBits[4];
	uint32		m_FirstBits[4];
};

class FMConvertRequest
{
public:
					FMConvertRequest();             // 0x10037511

	PFormat			*m_pSrcFormat;
	uint8			*m_pSrc;
	long			m_SrcPitch;
	void			*m_pSrcPalette;

	PFormat			*m_pDestFormat;
	uint8			*m_pDest;
	long			m_DestPitch;

	uint32			m_Width;
	uint32			m_Height;

	uint32			m_Flags;

	PFormat			m_DefaultSrcFormat;
	PFormat			m_DefaultDestFormat;
};

union GenericColor
{
	uint32	dwVal;
	uint16	wVal;
	uint8	bVal;
};

class FormatMgr
{
public:
	LTRESULT		ConvertPixels(FMConvertRequest *pRequest);  // 0x1003764c
	LTRESULT		PValueToFormatColor(PFormat *pFormat, PValue in, GenericColor &out);  // 0x10037780
};

// One texture format the device offers (d3d_texture.cpp).
struct D3DTextureFormat
{
	LTLink			m_Link;                             // +0x00
	DDPIXELFORMAT	m_PF;                               // +0x0c
};

// One 64x64 piece of an optimised surface.
struct OptimizedTile
{
	LTRect				m_Rect;                         // +0x00, in the surface
	uint32				m_Width;                        // +0x10, the texture's size
	uint32				m_Height;                       // +0x14
	LPDIRECTDRAWSURFACE7	m_pSurface;                 // +0x18
};

struct OptimizedSurface
{
	uint32			m_nTilesX;                          // +0x00
	uint32			m_nTilesY;                          // +0x04
	OptimizedTile	m_Tiles[1];                         // +0x08, x across, then y
};

// The render data behind a CSurface.
struct D3DRenderData
{
	LPDIRECTDRAWSURFACE7	m_pSurface;                 // +0x00
	DDSURFACEDESC2			m_Desc;                     // +0x04
	uint32					m_ColorKey;                 // +0x80
	OptimizedSurface*		m_pOptimized;               // +0x84
	LTBOOL					m_bTransparent;             // +0x88
};

#define OPTIMIZED_TILE_SIZE		64
#define NUM_TILE_SIZES			6

extern LPDIRECT3DDEVICE7		g_pD3DDevice;           // 0x100638c8
extern LPDIRECTDRAW7			g_pDirectDraw;          // 0x1005d2a8
extern LPDIRECTDRAWSURFACE7		g_pFrontBuffer;         // 0x1005d2ac
extern LPDIRECTDRAWSURFACE7		g_pPrimarySurface;      // 0x1005d2b0
extern LTBOOL					g_bRunWindowed;         // 0x1005d538
extern LTBOOL					g_bIn3D;                // 0x100638d8
extern LTBOOL					g_bInOptimized2D;       // 0x100638dc
extern int32					g_CV_OptimizeSurfaces;  // 0x1005d750
extern int32					g_CV_Bilinear;          // 0x1005dbb0
extern LTBOOL					g_bCanBilinear;         // 0x1005d2c4
// The device's capabilities (d3d_init.cpp's g_D3DCaps), as far as they
// are read here: the triangle caps (D3DPTEXTURECAPS_ in dwTextureCaps).
struct D3DCaps
{
	uint8					m_pad_000[0x124];
	D3DPRIMCAPS				m_TriCaps;              // +0x124
};

extern D3DCaps					g_D3DCaps;              // 0x100622d8 (d3d_init.cpp)
extern D3DTextureFormat*		g_TextureFormats[];     // 0x100682c8 (d3d_texture.cpp)
extern FormatMgr				g_FormatMgr;            // 0x100661a8 (d3d_texture.cpp)

void*	dalloc_z(uint32 size);                          // 0x10013a7f
void	dfree(void* ptr);                               // 0x10013ac5
void	d3d_SetDefaultBlend();                          // 0x100143a0 (d3d_draw.cpp)
void	d3d_GetScreenFormat(PFormat *pFormat);          // 0x1001eab3 (d3d_surface.cpp)
void	r_SetPFormat(DDPIXELFORMAT* pPF, PFormat* pFormat);  // 0x10011794

#define FORMAT_INTERFACE	4


// ------------------------------------------------------------------------------ //
// Globals.
// ------------------------------------------------------------------------------ //

// The texture sizes a tile can have.
uint32			g_TileSizes[NUM_TILE_SIZES] = {2, 4, 8, 16, 32, 64};   // 0x1004d4d0
uint32			g_Optimized2DColor = 0xFFFFFFFF;        // 0x1004d4e8

// Taken off the screen coordinates of each tile drawn, so they meet the
// rasteriser's fill convention.
float			g_fBlitFillBias = 0.51f;                // 0x1004d4ec

uint32			g_SavedFogEnable;                       // 0x100638e0
LTSurfaceBlend	g_Optimized2DBlend;                     // 0x10064ce4


// ------------------------------------------------------------------------------ //
// 0x1001cb20 -- free a surface's tiles.
// ------------------------------------------------------------------------------ //
void d3d_FreeOptimizedSurface(D3DRenderData *pData)
{
	uint32 i, nTiles;
	OptimizedTile *pTile;

	if (pData->m_pOptimized)
	{
		nTiles = pData->m_pOptimized->m_nTilesX * pData->m_pOptimized->m_nTilesY;
		for (i = 0; i < nTiles; i++)
		{
			pTile = &pData->m_pOptimized->m_Tiles[i];
			if (pTile->m_pSurface)
				pTile->m_pSurface->Release();
		}

		dfree(pData->m_pOptimized);
		pData->m_pOptimized = LTNULL;
	}
}


// ------------------------------------------------------------------------------ //
// 0x1001cb73 -- set solid alpha on every pixel converted into a tile.
// ------------------------------------------------------------------------------ //
void d3d_SetSolidAlpha(FMConvertRequest *pRequest, uint32 destAlpha32, GenericColor &tColor)
{
	uint8 *pDestLine = pRequest->m_pDest;
	uint32 destAlpha16 = (uint16)destAlpha32;

	uint32 yCounter = pRequest->m_Height;
	while (yCounter)
	{
		--yCounter;

		uint32 xCounter = pRequest->m_Width;
		if (pRequest->m_pDestFormat->m_eType == BPP_16)
		{
			uint16 *pDest16 = (uint16*)pDestLine;
			while (xCounter)
			{
				xCounter--;
				*pDest16 |= destAlpha16;
				++pDest16;
			}
		}
		else
		{
			uint32 *pDest32 = (uint32*)pDestLine;
			while (xCounter)
			{
				--xCounter;
				*pDest32 |= destAlpha32;
				++pDest32;
			}
		}

		pDestLine += pRequest->m_DestPitch;
	}
}


// ------------------------------------------------------------------------------ //
// 0x1001cbc4 -- give the converted pixels the destination alpha, except
// where the source pixel (its alpha bits ignored) is the colour key, whose
// alpha is cleared.
// ------------------------------------------------------------------------------ //
#define DO_MASK_LOOP(srcType, destType, srcIt, destIt, srcTColor, srcAlphaMask, destAlphaMask)	\
	{																				\
		xCounter = pRequest->m_Width;												\
		srcType tKey = srcTColor;													\
		srcIt = (srcType*)pSrcLine;													\
		destIt = (destType*)pDestLine;												\
		while (xCounter)															\
		{																			\
			if ((*srcIt & ~srcAlphaMask) != tKey)									\
				*destIt |= destAlphaMask;											\
			else																	\
				*destIt &= ~destAlphaMask;											\
			srcIt++; destIt++; xCounter--;											\
		}																			\
	}

void d3d_DoAlphaFromColorKey(FMConvertRequest *pRequest, uint32 srcAlpha32, uint32 destAlpha32, GenericColor &tColor)
{
	uint32 *pSrc32, *pDest32;
	uint16 *pSrc16, *pDest16;
	uint32 xCounter;
	uint8 *pSrcLine = pRequest->m_pSrc;
	uint8 *pDestLine = pRequest->m_pDest;
	uint32 y;

	for (y = pRequest->m_Height; y; y--)
	{
		if (pRequest->m_pSrcFormat->m_eType == BPP_16)
		{
			if (pRequest->m_pDestFormat->m_eType == BPP_16)
				DO_MASK_LOOP(uint16, uint16, pSrc16, pDest16, tColor.wVal, (uint16)srcAlpha32, (uint16)destAlpha32)
			else if (pRequest->m_pDestFormat->m_eType == BPP_32)
				DO_MASK_LOOP(uint16, uint32, pSrc16, pDest32, tColor.wVal, (uint16)srcAlpha32, destAlpha32)
		}
		else
		{
			if (pRequest->m_pDestFormat->m_eType == BPP_16)
				DO_MASK_LOOP(uint32, uint16, pSrc32, pDest16, tColor.dwVal, srcAlpha32, (uint16)destAlpha32)
			else if (pRequest->m_pDestFormat->m_eType == BPP_32)
				DO_MASK_LOOP(uint32, uint32, pSrc32, pDest32, tColor.dwVal, srcAlpha32, destAlpha32)
		}

		pSrcLine += pRequest->m_SrcPitch;
		pDestLine += pRequest->m_DestPitch;
	}
}


// ------------------------------------------------------------------------------ //
// 0x1001cd43 -- convert the surface into its tiles.
// ------------------------------------------------------------------------------ //
LTBOOL d3d_FillOptimizedSurface(D3DRenderData *pData, D3DTextureFormat *pFormat, uint32 transparentColor)
{
	LTRect rect;
	FMConvertRequest cRequest;
	PFormat srcFormat, destFormat;
	GenericColor tColor;
	DDSURFACEDESC2 ddsd, tileDesc;
	OptimizedTile *pTile;
	uint8 *pSrcData;
	uint32 x, y;

	d3d_GetScreenFormat(&srcFormat);
	r_SetPFormat(&pFormat->m_PF, &destFormat);

	g_FormatMgr.PValueToFormatColor(&srcFormat, transparentColor, tColor);

	// Take the alpha bits out of the colour key.
	if (srcFormat.GetBytesPerPixel() == 4)
		tColor.dwVal &= ~srcFormat.m_Masks[0];
	else
		tColor.wVal &= ~srcFormat.m_Masks[0];

	cRequest.m_pSrcFormat = &srcFormat;
	cRequest.m_pDestFormat = &destFormat;
	cRequest.m_Flags = 0;
	pData->m_bTransparent = transparentColor != 0xFFFFFFFF;

	memset(&ddsd, 0, sizeof(ddsd));
	ddsd.dwSize = sizeof(ddsd);
	if (pData->m_pSurface->Lock(LTNULL, &ddsd, DDLOCK_WAIT | DDLOCK_READONLY, LTNULL) == DD_OK)
	{
		pSrcData = (uint8*)ddsd.lpSurface;

		for (x = 0; x < pData->m_pOptimized->m_nTilesX; x++)
		{
			for (y = 0; y < pData->m_pOptimized->m_nTilesY; y++)
			{
				pTile = &pData->m_pOptimized->m_Tiles[y * pData->m_pOptimized->m_nTilesX + x];

				memset(&tileDesc, 0, sizeof(tileDesc));
				tileDesc.dwSize = sizeof(tileDesc);
				if (pTile->m_pSurface->Lock(LTNULL, &tileDesc, DDLOCK_WAIT | DDLOCK_WRITEONLY, LTNULL) == DD_OK)
				{
					rect.left = x * OPTIMIZED_TILE_SIZE;
					rect.right = rect.left + OPTIMIZED_TILE_SIZE;
					if (rect.right > (int)pData->m_Desc.dwWidth)
						rect.right = pData->m_Desc.dwWidth;

					rect.top = y * OPTIMIZED_TILE_SIZE;
					rect.bottom = rect.top + OPTIMIZED_TILE_SIZE;
					if (rect.bottom > (int)pData->m_Desc.dwHeight)
						rect.bottom = pData->m_Desc.dwHeight;

					pTile->m_Rect = rect;

					cRequest.m_pSrc = &pSrcData[rect.top * ddsd.lPitch];
					cRequest.m_pSrc += rect.left << srcFormat.GetBPPShift();
					cRequest.m_SrcPitch = ddsd.lPitch;
					cRequest.m_pDest = (uint8*)tileDesc.lpSurface;
					cRequest.m_DestPitch = tileDesc.lPitch;
					cRequest.m_Width = rect.right - rect.left;
					cRequest.m_Height = rect.bottom - rect.top;

					if (g_FormatMgr.ConvertPixels(&cRequest) == LT_OK)
					{
						if (transparentColor == 0xFFFFFFFF)
							d3d_SetSolidAlpha(&cRequest, destFormat.m_Masks[0], tColor);
						else
							d3d_DoAlphaFromColorKey(&cRequest, srcFormat.m_Masks[0], destFormat.m_Masks[0], tColor);
					}

					pTile->m_pSurface->Unlock(LTNULL);
				}
				else
				{
					pData->m_pSurface->Unlock(LTNULL);
					return LTFALSE;
				}
			}
		}

		pData->m_pSurface->Unlock(LTNULL);
		return LTTRUE;
	}
	else
	{
		return LTFALSE;
	}
}


// ------------------------------------------------------------------------------ //
// 0x1001cfe2 -- RenderStruct::UnoptimizeSurface.
// ------------------------------------------------------------------------------ //
void d3d_UnoptimizeSurface(D3DRenderData *pData)
{
	if (pData)
		d3d_FreeOptimizedSurface(pData);
}


// ------------------------------------------------------------------------------ //
// 0x1001cff4 -- RenderStruct::OptimizeSurface: cut the surface into
// textures of up to 64x64.
// ------------------------------------------------------------------------------ //

// The smallest tile size bigger than size.
inline uint32 d3d_GetTileSize(uint32 size)
{
	uint32 i;

	for (i = 0; i < NUM_TILE_SIZES; i++)
	{
		if (size < g_TileSizes[i])
			return g_TileSizes[i];
	}

	return g_TileSizes[NUM_TILE_SIZES-1];
}

LTBOOL d3d_OptimizeSurface(D3DRenderData *pData, uint32 transparentColor)
{
	D3DTextureFormat *pFormat;
	OptimizedTile *pTile;
	DDSURFACEDESC2 ddsd;
	uint32 nTilesX, nTilesY, x, y, width, height;

	if (!pData || !g_CV_OptimizeSurfaces || (g_D3DCaps.m_TriCaps.dwTextureCaps & D3DPTEXTURECAPS_SQUAREONLY))
		return LTFALSE;

	pFormat = g_TextureFormats[FORMAT_INTERFACE];
	if (!pFormat)
		return LTFALSE;

	if (!pData->m_pOptimized)
	{
		nTilesX = pData->m_Desc.dwWidth / OPTIMIZED_TILE_SIZE;
		nTilesY = pData->m_Desc.dwHeight / OPTIMIZED_TILE_SIZE;
		if (nTilesX * OPTIMIZED_TILE_SIZE != pData->m_Desc.dwWidth)
			nTilesX++;

		if (nTilesY * OPTIMIZED_TILE_SIZE != pData->m_Desc.dwHeight)
			nTilesY++;

		if (!nTilesX && !nTilesY)
			return LTFALSE;

		pData->m_pOptimized = (OptimizedSurface*)dalloc_z(sizeof(OptimizedTile) * nTilesX * nTilesY + 8);
		if (!pData->m_pOptimized)
			return LTFALSE;

		pData->m_pOptimized->m_nTilesX = nTilesX;
		pData->m_pOptimized->m_nTilesY = nTilesY;

		for (x = 0; x < nTilesX; x++)
		{
			width = pData->m_Desc.dwWidth - x * OPTIMIZED_TILE_SIZE;
			width = d3d_GetTileSize(width);

			for (y = 0; y < nTilesY; y++)
			{
				height = pData->m_Desc.dwHeight - y * OPTIMIZED_TILE_SIZE;
				height = d3d_GetTileSize(height);

				pTile = &pData->m_pOptimized->m_Tiles[y * nTilesX + x];
				if (g_D3DCaps.m_TriCaps.dwTextureCaps & D3DPTEXTURECAPS_SQUAREONLY)
				{
					pTile->m_Width = LTMAX(width, height);
					pTile->m_Height = LTMAX(width, height);
				}
				else
				{
					pTile->m_Width = width;
					pTile->m_Height = height;
				}

				memset(&ddsd, 0, sizeof(ddsd));
				ddsd.dwSize = sizeof(ddsd);
				ddsd.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT;
				ddsd.ddsCaps.dwCaps = DDSCAPS_TEXTURE;
				ddsd.ddsCaps.dwCaps2 = DDSCAPS2_TEXTUREMANAGE;
				ddsd.dwWidth = pTile->m_Width;
				ddsd.dwHeight = pTile->m_Height;
				memcpy(&ddsd.ddpfPixelFormat, &pFormat->m_PF, sizeof(DDPIXELFORMAT));

				if (g_pDirectDraw->CreateSurface(&ddsd, &pTile->m_pSurface, LTNULL) != DD_OK)
				{
					d3d_FreeOptimizedSurface(pData);
					return LTFALSE;
				}
			}
		}
	}

	if (!d3d_FillOptimizedSurface(pData, pFormat, transparentColor))
	{
		d3d_FreeOptimizedSurface(pData);
		return LTFALSE;
	}

	pData->m_ColorKey = transparentColor;
	return LTTRUE;
}


// ------------------------------------------------------------------------------ //
// 0x1001d237 -- RenderStruct::StartOptimized2D.
// ------------------------------------------------------------------------------ //
LTBOOL d3d_StartOptimized2D()
{
	if (!g_pD3DDevice || !g_bIn3D)
		return LTFALSE;

	if (g_bInOptimized2D)
		return LTTRUE;

	g_pD3DDevice->GetRenderState(D3DRENDERSTATE_FOGENABLE, &g_SavedFogEnable);
	g_pD3DDevice->SetRenderState(D3DRENDERSTATE_FOGENABLE, FALSE);
	g_pD3DDevice->SetRenderState(D3DRENDERSTATE_ZENABLE, FALSE);
	g_pD3DDevice->SetRenderState(D3DRENDERSTATE_ZWRITEENABLE, FALSE);

	if (!g_CV_Bilinear && g_bCanBilinear)
	{
		g_pD3DDevice->SetTextureStageState(0, D3DTSS_MINFILTER, D3DTFN_POINT);
		g_pD3DDevice->SetTextureStageState(0, D3DTSS_MAGFILTER, D3DTFG_POINT);
		g_pD3DDevice->SetTextureStageState(0, D3DTSS_MIPFILTER, D3DTFP_POINT);
	}

	d3d_SetDefaultBlend();
	g_pD3DDevice->SetRenderState(D3DRENDERSTATE_ALPHABLENDENABLE, TRUE);
	g_pD3DDevice->SetRenderState(D3DRENDERSTATE_SRCBLEND, D3DBLEND_SRCALPHA);
	g_pD3DDevice->SetRenderState(D3DRENDERSTATE_DESTBLEND, D3DBLEND_INVSRCALPHA);
	g_pD3DDevice->SetTextureStageState(0, D3DTSS_ADDRESS, D3DTADDRESS_CLAMP);

	g_bInOptimized2D = LTTRUE;
	return LTTRUE;
}


// ------------------------------------------------------------------------------ //
// 0x1001d335 -- RenderStruct::EndOptimized2D.
// ------------------------------------------------------------------------------ //
void d3d_EndOptimized2D()
{
	if (!g_pD3DDevice || !g_bIn3D || !g_bInOptimized2D)
		return;

	g_pD3DDevice->SetRenderState(D3DRENDERSTATE_ZENABLE, TRUE);
	g_pD3DDevice->SetRenderState(D3DRENDERSTATE_ZWRITEENABLE, TRUE);
	g_pD3DDevice->SetRenderState(D3DRENDERSTATE_FOGENABLE, g_SavedFogEnable);

	if (!g_CV_Bilinear && g_bCanBilinear)
	{
		g_pD3DDevice->SetTextureStageState(0, D3DTSS_MINFILTER, D3DTFN_LINEAR);
		g_pD3DDevice->SetTextureStageState(0, D3DTSS_MAGFILTER, D3DTFG_LINEAR);
		g_pD3DDevice->SetTextureStageState(0, D3DTSS_MIPFILTER, D3DTFP_POINT);
	}

	g_pD3DDevice->SetRenderState(D3DRENDERSTATE_ALPHABLENDENABLE, FALSE);
	g_pD3DDevice->SetTextureStageState(0, D3DTSS_ADDRESS, D3DTADDRESS_WRAP);

	g_bInOptimized2D = LTFALSE;
}


// ------------------------------------------------------------------------------ //
// 0x1001d3fb -- RenderStruct::IsInOptimized2D.
// ------------------------------------------------------------------------------ //
LTBOOL d3d_IsInOptimized2D()
{
	return g_bInOptimized2D;
}


// ------------------------------------------------------------------------------ //
// 0x1001d401 -- RenderStruct::SetOptimized2DBlend.
// ------------------------------------------------------------------------------ //
LTBOOL d3d_SetOptimized2DBlend(LTSurfaceBlend blend)
{
	g_Optimized2DBlend = blend;

	switch (blend)
	{
		case LTSURFACEBLEND_ALPHA:
		{
			g_pD3DDevice->SetRenderState(D3DRENDERSTATE_SRCBLEND, D3DBLEND_SRCALPHA);
			g_pD3DDevice->SetRenderState(D3DRENDERSTATE_DESTBLEND, D3DBLEND_INVSRCALPHA);
		}
		break;

		case LTSURFACEBLEND_SOLID:
		{
			g_pD3DDevice->SetRenderState(D3DRENDERSTATE_SRCBLEND, D3DBLEND_ONE);
			g_pD3DDevice->SetRenderState(D3DRENDERSTATE_DESTBLEND, D3DBLEND_ZERO);
		}
		break;

		case LTSURFACEBLEND_ADD:
		{
			g_pD3DDevice->SetRenderState(D3DRENDERSTATE_SRCBLEND, D3DBLEND_ONE);
			g_pD3DDevice->SetRenderState(D3DRENDERSTATE_DESTBLEND, D3DBLEND_ONE);
		}
		break;

		case LTSURFACEBLEND_MULTIPLY:
		{
			g_pD3DDevice->SetRenderState(D3DRENDERSTATE_SRCBLEND, D3DBLEND_DESTCOLOR);
			g_pD3DDevice->SetRenderState(D3DRENDERSTATE_DESTBLEND, D3DBLEND_ZERO);
		}
		break;

		case LTSURFACEBLEND_MULTIPLY2:
		{
			g_pD3DDevice->SetRenderState(D3DRENDERSTATE_SRCBLEND, D3DBLEND_DESTCOLOR);
			g_pD3DDevice->SetRenderState(D3DRENDERSTATE_DESTBLEND, D3DBLEND_SRCCOLOR);
		}
		break;

		case LTSURFACEBLEND_MASK:
		{
			g_pD3DDevice->SetRenderState(D3DRENDERSTATE_SRCBLEND, D3DBLEND_ZERO);
			g_pD3DDevice->SetRenderState(D3DRENDERSTATE_DESTBLEND, D3DBLEND_INVSRCCOLOR);
		}
		break;

		case LTSURFACEBLEND_MASKADD:
		{
			g_pD3DDevice->SetRenderState(D3DRENDERSTATE_SRCBLEND, D3DBLEND_ONE);
			g_pD3DDevice->SetRenderState(D3DRENDERSTATE_DESTBLEND, D3DBLEND_INVSRCCOLOR);
		}
		break;

		default:
		{
			return LTFALSE;
		}
	}

	return LTTRUE;
}


// ------------------------------------------------------------------------------ //
// 0x1001d4c8 -- RenderStruct::SetOptimized2DColor.
// ------------------------------------------------------------------------------ //
LTBOOL d3d_SetOptimized2DColor(uint32 color)
{
	g_Optimized2DColor = color | 0xFF000000;
	return LTTRUE;
}


// ------------------------------------------------------------------------------ //
// 0x1001d4da -- RenderStruct::GetOptimized2DBlend.
// ------------------------------------------------------------------------------ //
LTBOOL d3d_GetOptimized2DBlend(LTSurfaceBlend *pBlend)
{
	*pBlend = g_Optimized2DBlend;
	return LTTRUE;
}


// ------------------------------------------------------------------------------ //
// 0x1001d4ea -- RenderStruct::GetOptimized2DColor.
// ------------------------------------------------------------------------------ //
LTBOOL d3d_GetOptimized2DColor(uint32 *pColor)
{
	*pColor = g_Optimized2DColor;
	return LTTRUE;
}


// ------------------------------------------------------------------------------ //
// The blits of an optimised surface: one textured quad per tile.
// ------------------------------------------------------------------------------ //
struct RenderBlitRequest
{
	D3DRenderData*	m_pSurface;                         // +0x00
	uint32			m_BlitOptions;                      // +0x04
	uint32			m_TransparentColor;                 // +0x08
	LTRect*			m_pSrcRect;                         // +0x0c
	LTRect*			m_pDestRect;                        // +0x10
	float			m_Alpha;                            // +0x14
	LTWarpPt*		m_pWarpPts;                         // +0x18
	int				m_nWarpPts;                         // +0x1c
};

struct TLVertex
{
	LTVector	m_Vec;                                  // +0x00
	float		rhw;                                    // +0x0c
	union
	{
		uint32	color;                                  // +0x10
		struct { uint8 b, g, r, a; } rgb;
	};
	uint32		specular;                               // +0x14
	float		tu, tv;                                 // +0x18

	void		SetTCoords(float u, float v)
	{
		tu = u;
		tv = v;
	}
};

void	InvalidateRect(LTRect *pRect);                  // 0x10022e31 (dirtyrect.cpp)

inline void d3d_GetTextureDirect(LPDIRECTDRAWSURFACE7 *ppTexture, uint32 iStage)
{
	g_pD3DDevice->GetTexture(iStage, ppTexture);
}

inline void d3d_SetTextureDirect(LPDIRECTDRAWSURFACE7 pTexture, uint32 iStage)
{
	g_pD3DDevice->SetTexture(iStage, pTexture);
}


// 0x1001d4fa -- RenderStruct::BlitToScreen for an optimised surface: the
// source rectangle, tile by tile, scaled into the destination rectangle.
void d3d_BlitOptimized(RenderBlitRequest *pRequest)
{
	D3DRenderData *pData;
	OptimizedSurface *pTiles;
	OptimizedTile *pTile;
	LTRect *pSrcRect, *pDestRect;
	float srcRectWidth, srcRectHeight, destRectWidth, destRectHeight;
	float destLeft, destRight, destTop, destBottom;
	float tLeft, tRight, tTop, tBottom;
	TLVertex verts[4];
	LPDIRECTDRAWSURFACE7 pOldTexture;
	int x, y, xEnd, yEnd, nXTile, nYTile;

	pData = pRequest->m_pSurface;
	if (pRequest->m_TransparentColor != pData->m_ColorKey)
		d3d_OptimizeSurface(pData, pRequest->m_TransparentColor);

	pTiles = pData->m_pOptimized;
	if (!pTiles)
		return;

	g_pD3DDevice->SetRenderState(D3DRENDERSTATE_ALPHABLENDENABLE, g_Optimized2DBlend != LTSURFACEBLEND_SOLID);

	pSrcRect = pRequest->m_pSrcRect;
	pDestRect = pRequest->m_pDestRect;
	srcRectWidth = (float)(pSrcRect->right - pSrcRect->left);
	srcRectHeight = (float)(pSrcRect->bottom - pSrcRect->top);
	destRectWidth = (float)(pDestRect->right - pDestRect->left);
	destRectHeight = (float)(pDestRect->bottom - pDestRect->top);

	verts[0].color = verts[1].color = verts[2].color = verts[3].color = g_Optimized2DColor;
	verts[0].rgb.a = verts[1].rgb.a = verts[2].rgb.a = verts[3].rgb.a = (uint8)(pRequest->m_Alpha * 255.0f);
	verts[0].rhw = verts[1].rhw = verts[2].rhw = verts[3].rhw = 1.0f;

	pOldTexture = LTNULL;
	g_pD3DDevice->GetTexture(0, &pOldTexture);

	for (x = pSrcRect->left; x < pSrcRect->right; x = xEnd)
	{
		nXTile = x / OPTIMIZED_TILE_SIZE;
		pTile = &pTiles->m_Tiles[nXTile];
		xEnd = x + OPTIMIZED_TILE_SIZE;
		xEnd = LTMIN(pTile->m_Rect.right, LTMIN(xEnd, pSrcRect->right));

		for (y = pSrcRect->top; y < pSrcRect->bottom; y = yEnd)
		{
			nYTile = y / OPTIMIZED_TILE_SIZE;
			pTile = &pTiles->m_Tiles[nYTile * pTiles->m_nTilesX + nXTile];
			yEnd = y + OPTIMIZED_TILE_SIZE;
			yEnd = LTMIN(pTile->m_Rect.bottom, LTMIN(yEnd, pSrcRect->bottom));

			destLeft = (float)pDestRect->left + ((x - pSrcRect->left) / srcRectWidth) * destRectWidth;
			destRight = (float)pDestRect->left + ((xEnd - pSrcRect->left) / srcRectWidth) * destRectWidth;
			destTop = (float)pDestRect->top + ((y - pSrcRect->top) / srcRectHeight) * destRectHeight;
			destBottom = (float)pDestRect->top + ((yEnd - pSrcRect->top) / srcRectHeight) * destRectHeight;

			tLeft = (x - pTile->m_Rect.left) / (float)pTile->m_Width;
			tRight = (xEnd - pTile->m_Rect.left) / (float)pTile->m_Width;
			tTop = (y - pTile->m_Rect.top) / (float)pTile->m_Height;
			tBottom = (yEnd - pTile->m_Rect.top) / (float)pTile->m_Height;

			destLeft = LTMAX(0.0f, destLeft - g_fBlitFillBias);
			destTop = LTMAX(0.0f, destTop - g_fBlitFillBias);
			destRight = LTMAX(0.0f, destRight - g_fBlitFillBias);
			destBottom = LTMAX(0.0f, destBottom - g_fBlitFillBias);

			verts[0].m_Vec.Init(destLeft, destTop, 1.0f);
			verts[1].m_Vec.Init(destRight, destTop, 1.0f);
			verts[2].m_Vec.Init(destRight, destBottom, 1.0f);
			verts[3].m_Vec.Init(destLeft, destBottom, 1.0f);

			verts[0].SetTCoords(tLeft, tTop);
			verts[1].SetTCoords(tRight, tTop);
			verts[2].SetTCoords(tRight, tBottom);
			verts[3].SetTCoords(tLeft, tBottom);

			g_pD3DDevice->SetTexture(0, pTile->m_pSurface);
			g_pD3DDevice->DrawPrimitive(D3DPT_TRIANGLEFAN, D3DFVF_TLVERTEX, verts, 4, 0);
		}
	}

	g_pD3DDevice->SetTexture(0, pOldTexture);
	InvalidateRect(pDestRect);
}


// 0x1001d890 -- RenderStruct::WarpToScreen for an optimised surface: the
// whole surface, tile by tile, onto the parallelogram the first, second and
// fourth warp points span.
void d3d_WarpOptimized(RenderBlitRequest *pRequest)
{
	LTWarpPt *pWarpPts = pRequest->m_pWarpPts;
	LPDIRECTDRAWSURFACE7 pOldTexture = LTNULL;

	D3DRenderData *pData = pRequest->m_pSurface;
	OptimizedSurface *pTiles = pData->m_pOptimized;
	OptimizedTile *pTile;

	if (!pTiles)
		return;

	uint32 nXTiles = pTiles->m_nTilesX;
	uint32 nYTiles = pTiles->m_nTilesY;

	float srcRectWidth = (float)(pRequest->m_pSrcRect->right - pRequest->m_pSrcRect->left);
	float srcRectHeight = (float)(pRequest->m_pSrcRect->bottom - pRequest->m_pSrcRect->top);

	g_pD3DDevice->SetRenderState(D3DRENDERSTATE_ALPHABLENDENABLE, pData->m_bTransparent || (pRequest->m_Alpha != 1.0f));

	d3d_GetTextureDirect(&pOldTexture, 0);

	TLVertex verts[12];
	verts[0].color = verts[1].color = verts[2].color = verts[3].color = g_Optimized2DColor;
	verts[0].rgb.a = verts[1].rgb.a = verts[2].rgb.a = verts[3].rgb.a = (uint8)(pRequest->m_Alpha * 255.0f);
	verts[0].rhw = verts[1].rhw = verts[2].rhw = verts[3].rhw = 1.0f;

	LTVector vWarpStart(pWarpPts[0].dest_x, pWarpPts[0].dest_y, 1.0f);
	LTVector vWarpDirX, vWarpDirY;
	vWarpDirX = LTVector(pWarpPts[1].dest_x, pWarpPts[1].dest_y, 1.0f) - vWarpStart;
	vWarpDirY = LTVector(pWarpPts[3].dest_x, pWarpPts[3].dest_y, 1.0f) - vWarpStart;

	float fScaleX = VEC_MAG(vWarpDirX) / srcRectWidth;
	float fScaleY = VEC_MAG(vWarpDirY) / srcRectHeight;

	vWarpDirX.Norm(fScaleX);
	vWarpDirY.Norm(fScaleY);

	uint32 xPos, yPos = 0;
	uint32 i, j;

	for (i = 0; i < nYTiles; i++)
	{
		xPos = 0;

		for (j = 0; j < nXTiles; j++)
		{
			pTile = &pTiles->m_Tiles[i * nXTiles + j];

			verts[0].m_Vec = vWarpStart + (vWarpDirX * (float)xPos) + (vWarpDirY * (float)yPos);
			verts[1].m_Vec = verts[0].m_Vec + (vWarpDirX * (float)pTile->m_Width);
			verts[2].m_Vec = verts[0].m_Vec + (vWarpDirX * (float)pTile->m_Width) + (vWarpDirY * (float)pTile->m_Height);
			verts[3].m_Vec = verts[0].m_Vec + (vWarpDirY * (float)pTile->m_Height);

			verts[0].SetTCoords(0.0f, 0.0f);
			verts[1].SetTCoords(1.0f, 0.0f);
			verts[2].SetTCoords(1.0f, 1.0f);
			verts[3].SetTCoords(0.0f, 1.0f);

			d3d_SetTextureDirect(pTile->m_pSurface, 0);
			g_pD3DDevice->DrawPrimitive(D3DPT_TRIANGLEFAN, D3DFVF_TLVERTEX, verts, 4, 0);

			xPos += pTile->m_Width;
		}

		yPos += pTiles->m_Tiles[i * nXTiles].m_Height;
	}

	d3d_SetTextureDirect(pOldTexture, 0);
}


// ------------------------------------------------------------------------------ //
// The dynamic lightmap textures (0x1001dda5..0x1001e64d): a texture filled
// with a radial falloff when it is made, that can also be copied from or to
// the back buffer and drawn over the screen as a quad.  The allocator keeps
// them in a list behind a list head node; it is a singleton (lmdynamic.cpp's
// g_pLMAllocator, 0x1007c1e4), made by d3d_InitSubsystems (0x1001c480).
// ------------------------------------------------------------------------------ //
extern LPDIRECTDRAWSURFACE7		g_pBackBuffer;          // 0x1005d2b4

char*	d3d_GetErrorString(HRESULT hResult);            // 0x1001a720 (d3d_init.cpp)

static char*		g_pLastLMTextureError;           // 0x10066168
static D3DTLVERTEX	g_LMTextureVerts[4];             // 0x100650e8

class CLMTexture
{
public:
	virtual			~CLMTexture() {}

	virtual bool	Init(uint32 width, uint32 height) = 0;
	virtual void	GetDims(uint32 *pWidth, uint32 *pHeight) = 0;
	virtual bool	SetStates() = 0;
	virtual bool	DrawToScreen(uint32 x, uint32 y, uint32 color, bool bBlend) = 0;
	virtual bool	CopyToScreen(uint32 x, uint32 y) = 0;
	virtual bool	CopyFromScreen(uint32 x, uint32 y) = 0;
};

class CD3DLMTexture : public CLMTexture
{
public:
					CD3DLMTexture()
					{
						m_Width = 0;
						m_Height = 0;
						m_pSurface = LTNULL;
					}

	virtual			~CD3DLMTexture() { Term(); }

	virtual bool	Init(uint32 width, uint32 height);
	virtual void	GetDims(uint32 *pWidth, uint32 *pHeight);
	virtual bool	SetStates();
	virtual bool	DrawToScreen(uint32 x, uint32 y, uint32 color, bool bBlend);
	virtual bool	CopyToScreen(uint32 x, uint32 y);
	virtual bool	CopyFromScreen(uint32 x, uint32 y);
	virtual void	Term();

	uint32					m_Width;                    // +0x04
	uint32					m_Height;                   // +0x08
	LPDIRECTDRAWSURFACE7	m_pSurface;                 // +0x0c
};


inline bool d3d_IsPowerOfTwo(uint32 size)
{
	return !(size & (size - 1));
}

// The largest power of two in size, up to 256.
inline uint32 d3d_PowerOfTwoBelow(uint32 size)
{
	uint32 bit;

	for (bit = 256; !(bit & size); bit >>= 1)
		;

	return bit;
}


// 0x1001ddea -- make the texture, a radial falloff from the centre: white
// at the edges, a quarter grey in the middle.
//
// RETAIL BUG: when CreateSurface fails it still locks the (null) surface.
bool CD3DLMTexture::Init(uint32 width, uint32 height)
{
	DDPIXELFORMAT pf;
	DDSURFACEDESC2 ddsd;
	LPDIRECTDRAWSURFACE7 pSurface;
	HRESULT hResult;
	RECT rect;
	uint32 bytesPerPixel, rowSkip, x, y, i;
	float fCenterX, fCenterY, fInvRadiusSqr, fDistX, fDistY, fDist, fValue;
	uint8 *pBits, value;
	bool bWidthOk, bHeightOk;

	bWidthOk = d3d_IsPowerOfTwo(width);
	bHeightOk = d3d_IsPowerOfTwo(height);

	if (!bWidthOk)
		width = d3d_PowerOfTwoBelow(width);

	if (!bHeightOk)
		height = d3d_PowerOfTwoBelow(height);

	Term();

	memset(&pf, 0, sizeof(pf));
	pf.dwSize = sizeof(pf);
	g_pLastLMTextureError = d3d_GetErrorString(g_pBackBuffer->GetPixelFormat(&pf));

	memset(&ddsd, 0, sizeof(ddsd));
	ddsd.dwSize = sizeof(ddsd);
	ddsd.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT;
	ddsd.ddsCaps.dwCaps = DDSCAPS_TEXTURE;
	ddsd.ddsCaps.dwCaps2 = DDSCAPS2_TEXTUREMANAGE;
	ddsd.dwWidth = width;
	ddsd.dwHeight = height;
	memcpy(&ddsd.ddpfPixelFormat, &pf, sizeof(pf));

	pSurface = LTNULL;
	hResult = g_pDirectDraw->CreateSurface(&ddsd, &pSurface, LTNULL);
	g_pLastLMTextureError = d3d_GetErrorString(hResult);
	if (hResult == DD_OK)
	{
		m_Width = width;
		m_Height = height;
		m_pSurface = pSurface;
	}

	rect.left = 0;
	rect.top = 0;
	rect.right = width;
	rect.bottom = height;
	hResult = pSurface->Lock(&rect, &ddsd, DDLOCK_WAIT | DDLOCK_NOSYSLOCK, LTNULL);
	if (hResult == DD_OK)
	{
		bytesPerPixel = ddsd.ddpfPixelFormat.dwRGBBitCount >> 3;
		rowSkip = (ddsd.lPitch / bytesPerPixel - width) * bytesPerPixel;
		fCenterX = (float)width * 0.5f;
		fCenterY = (float)height * 0.5f;
		pBits = (uint8*)ddsd.lpSurface;
		fInvRadiusSqr = 1.0f / (fCenterX * fCenterX + fCenterY * fCenterY);

		for (y = 0; y < height; y++)
		{
			for (x = 0; x < width; x++)
			{
				fDistX = (float)x - fCenterX;
				fDistY = (float)y - fCenterY;
				fDist = (fDistX * fDistX + fDistY * fDistY) * fInvRadiusSqr * 2.0f;
				fValue = fDist + 0.25f - fDist * 0.25f;
				if (fValue > 1.0f)
					fValue = 1.0f;

				value = (uint8)(fValue * 255.0f);
				for (i = 0; i < bytesPerPixel; i++)
					pBits[i] = value;

				pBits += bytesPerPixel;
			}

			pBits += rowSkip;
		}

		pSurface->Unlock(&rect);
	}

	return hResult == DD_OK;
}


// 0x1001e068 -- free the texture.
void CD3DLMTexture::Term()
{
	if (m_pSurface)
	{
		m_pSurface->Release();
		m_pSurface = LTNULL;
	}

	m_Width = 0;
	m_Height = 0;
}


// 0x1001e086
void CD3DLMTexture::GetDims(uint32 *pWidth, uint32 *pHeight)
{
	*pWidth = m_Width;
	*pHeight = m_Height;
}


// 0x1001e09b -- the texture on stage 0, blended with the diffuse alpha,
// multiplied into the screen.
bool CD3DLMTexture::SetStates()
{
	LPDIRECTDRAWSURFACE7 pSurface;
	int i;

	pSurface = m_pSurface;

	for (i = 0; i < 8; i++)
	{
		g_pD3DDevice->SetTexture(i, LTNULL);
		g_pD3DDevice->SetTextureStageState(i, D3DTSS_COLOROP, D3DTOP_DISABLE);
		g_pD3DDevice->SetTextureStageState(i, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
	}

	g_pD3DDevice->SetTexture(0, pSurface);
	g_pD3DDevice->SetTextureStageState(0, D3DTSS_ADDRESS, D3DTADDRESS_CLAMP);
	g_pD3DDevice->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
	g_pD3DDevice->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
	g_pD3DDevice->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_BLENDDIFFUSEALPHA);
	g_pD3DDevice->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
	g_pD3DDevice->SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);
	g_pD3DDevice->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG2);
	g_pD3DDevice->SetRenderState(D3DRENDERSTATE_ALPHABLENDENABLE, TRUE);
	g_pD3DDevice->SetRenderState(D3DRENDERSTATE_SRCBLEND, D3DBLEND_ZERO);
	g_pD3DDevice->SetRenderState(D3DRENDERSTATE_DESTBLEND, D3DBLEND_SRCCOLOR);

	return true;
}


// 0x1001e1a5 -- draw the texture over the screen at (x, y).
bool CD3DLMTexture::DrawToScreen(uint32 x, uint32 y, uint32 color, bool bBlend)
{
	DWORD oldStates[5];
	float fLeft, fTop, fRight, fBottom;
	int i;

	SetStates();

	g_pD3DDevice->GetRenderState(D3DRENDERSTATE_ZENABLE, &oldStates[0]);
	g_pD3DDevice->GetRenderState(D3DRENDERSTATE_ZWRITEENABLE, &oldStates[1]);
	g_pD3DDevice->GetRenderState(D3DRENDERSTATE_ALPHABLENDENABLE, &oldStates[2]);
	g_pD3DDevice->GetRenderState(D3DRENDERSTATE_SRCBLEND, &oldStates[3]);
	g_pD3DDevice->GetRenderState(D3DRENDERSTATE_DESTBLEND, &oldStates[4]);

	g_pD3DDevice->SetRenderState(D3DRENDERSTATE_ZENABLE, FALSE);
	g_pD3DDevice->SetRenderState(D3DRENDERSTATE_ZWRITEENABLE, FALSE);
	g_pD3DDevice->SetRenderState(D3DRENDERSTATE_ALPHABLENDENABLE, bBlend != false);
	if (!bBlend)
	{
		g_pD3DDevice->SetRenderState(D3DRENDERSTATE_SRCBLEND, D3DBLEND_SRCALPHA);
		g_pD3DDevice->SetRenderState(D3DRENDERSTATE_DESTBLEND, D3DBLEND_INVSRCALPHA);
	}

	g_pD3DDevice->SetTextureStageState(0, D3DTSS_ADDRESS, D3DTADDRESS_CLAMP);

	for (i = 0; i < 4; i++)
	{
		g_LMTextureVerts[i].sz = 0.0f;
		g_LMTextureVerts[i].rhw = 1.0f;
		g_LMTextureVerts[i].color = color;
		g_LMTextureVerts[i].specular = 0;
	}

	fLeft = (float)x;
	fTop = (float)y;
	fRight = fLeft + (float)m_Width;
	fBottom = fTop + (float)m_Height;

	g_LMTextureVerts[0].sx = fLeft;
	g_LMTextureVerts[0].sy = fTop;
	g_LMTextureVerts[0].tu = 0.0f;
	g_LMTextureVerts[0].tv = 0.0f;

	g_LMTextureVerts[1].sx = fLeft;
	g_LMTextureVerts[1].sy = fBottom;
	g_LMTextureVerts[1].tu = 0.0f;
	g_LMTextureVerts[1].tv = 1.0f;

	g_LMTextureVerts[2].sx = fRight;
	g_LMTextureVerts[2].sy = fBottom;
	g_LMTextureVerts[2].tu = 1.0f;
	g_LMTextureVerts[2].tv = 1.0f;

	g_LMTextureVerts[3].sx = fRight;
	g_LMTextureVerts[3].sy = fTop;
	g_LMTextureVerts[3].tu = 1.0f;
	g_LMTextureVerts[3].tv = 0.0f;

	g_pD3DDevice->DrawPrimitive(D3DPT_TRIANGLEFAN, D3DFVF_TLVERTEX, g_LMTextureVerts, 4, 0);

	g_pD3DDevice->SetRenderState(D3DRENDERSTATE_ZENABLE, oldStates[0]);
	g_pD3DDevice->SetRenderState(D3DRENDERSTATE_ZWRITEENABLE, oldStates[1]);
	g_pD3DDevice->SetRenderState(D3DRENDERSTATE_ALPHABLENDENABLE, oldStates[2]);
	g_pD3DDevice->SetRenderState(D3DRENDERSTATE_SRCBLEND, oldStates[3]);
	g_pD3DDevice->SetRenderState(D3DRENDERSTATE_DESTBLEND, oldStates[4]);
	g_pD3DDevice->SetTextureStageState(0, D3DTSS_ADDRESS, D3DTADDRESS_WRAP);

	return true;
}


// 0x1001e3d6 -- copy the texture to the back buffer at (x, y).
bool CD3DLMTexture::CopyToScreen(uint32 x, uint32 y)
{
	RECT rect;
	HRESULT hResult;
	LPDIRECTDRAWSURFACE7 pSurface = m_pSurface;

	rect.left = 0;
	rect.top = 0;
	rect.right = m_Width;
	rect.bottom = m_Height;

	hResult = g_pBackBuffer->BltFast(x, y, pSurface, &rect, DDBLTFAST_WAIT);
	g_pLastLMTextureError = d3d_GetErrorString(hResult);
	return hResult == DD_OK;
}


// 0x1001e426 -- copy the back buffer at (x, y) into the texture.
bool CD3DLMTexture::CopyFromScreen(uint32 x, uint32 y)
{
	RECT rect;
	HRESULT hResult;
	LPDIRECTDRAWSURFACE7 pSurface = m_pSurface;

	rect.left = 0;
	rect.top = 0;
	rect.right = m_Width;
	rect.bottom = m_Height;

	hResult = pSurface->BltFast(x, y, g_pBackBuffer, &rect, DDBLTFAST_WAIT);
	g_pLastLMTextureError = d3d_GetErrorString(hResult);
	return hResult == DD_OK;
}


// One render target in the manager's list.
class CLMTextureNode
{
public:
					CLMTextureNode(CLMTextureNode *pPrev, CLMTextureNode *pNext);
					~CLMTextureNode();

	CLMTextureNode*	Find(CLMTexture *pTexture)
	{
		if (pTexture == m_pTexture)
			return this;

		if (m_pNext)
			return m_pNext->Find(pTexture);

		return LTNULL;
	}

	CLMTexture*		m_pTexture;                      // +0x00
	CLMTextureNode*	m_pPrev;                        // +0x04
	CLMTextureNode*	m_pNext;                        // +0x08
};

// 0x1001e476
CLMTextureNode::CLMTextureNode(CLMTextureNode *pPrev, CLMTextureNode *pNext)
{
	m_pTexture = new CD3DLMTexture;
	m_pPrev = pPrev;
	m_pNext = pNext;
}

// 0x1001e4b0
CLMTextureNode::~CLMTextureNode()
{
	if (m_pTexture)
		delete m_pTexture;

	if (m_pPrev)
		m_pPrev->m_pNext = m_pNext;

	if (m_pNext)
		m_pNext->m_pPrev = m_pPrev;
}


class CLMAllocator
{
public:
	virtual					~CLMAllocator() {}

	virtual CLMTexture*	Alloc(uint32 width, uint32 height) = 0;
	virtual void			Free(CLMTexture *pTexture) = 0;
};

class CD3DLMAllocator : public CLMAllocator
{
public:
							CD3DLMAllocator();
	virtual					~CD3DLMAllocator();

	virtual CLMTexture*	Alloc(uint32 width, uint32 height);
	virtual void			Free(CLMTexture *pTexture);

	CLMTextureNode*		m_pHead;                    // +0x04
};

extern CLMAllocator*	g_pLMAllocator;         // 0x1007c1e4

// 0x1001e4ef
CD3DLMAllocator::CD3DLMAllocator()
{
	if (!g_pLMAllocator)
		g_pLMAllocator = this;

	m_pHead = new CLMTextureNode(LTNULL, LTNULL);
}

// 0x1001e545
CD3DLMAllocator::~CD3DLMAllocator()
{
	CLMTextureNode *pNext;

	if (g_pLMAllocator == this)
		g_pLMAllocator = LTNULL;

	while (m_pHead)
	{
		pNext = m_pHead->m_pNext;
		delete m_pHead;
		m_pHead = pNext;
	}
}

// 0x1001e5ad
CLMTexture* CD3DLMAllocator::Alloc(uint32 width, uint32 height)
{
	CLMTexture *pTexture;

	m_pHead->m_pNext = new CLMTextureNode(m_pHead, m_pHead->m_pNext);
	pTexture = m_pHead->m_pNext ? m_pHead->m_pNext->m_pTexture : LTNULL;

	if (!pTexture->Init(width, height))
	{
		delete m_pHead->m_pNext;
		pTexture = LTNULL;
	}

	return pTexture;
}

// 0x1001e619
void CD3DLMAllocator::Free(CLMTexture *pTexture)
{
	delete m_pHead->Find(pTexture);
}
