// *********************************************************************** //
//  ph/d3d_surface.cpp -- Primal Hunt's d3d.ren, TU19
//  (0x1001e650..0x1001f273), /O1.
//
//  The engine's 2D surfaces (system-memory DirectDraw surfaces behind the
//  RenderStruct's surface slots): create/delete, lock/unlock, the screen
//  lock, the blits to and from the back buffer, and the LockOnFlip
//  console variable.
//
//  The CMoArray<DWORD> members this object instantiates first
//  (0x1001ef88..0x1001f25b) come from StdLith's dynarray.h.
// *********************************************************************** //

#include <windows.h>
#include <ddraw.h>
#include <stdio.h>
#include "ltbasedefs.h"
#include "../../LT2/lithshared/stdlith/dynarray.h"


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

// IDirect3DDevice7's BeginScene and EndScene.
typedef struct IDirect3DDevice7 CD3DDevice;	// g_pD3DDevice's real type, for its link name
struct IDirect3DDevice7
{
public:
	virtual long __stdcall QueryInterface(void*, void**) = 0;
	virtual uint32  __stdcall AddRef() = 0;
	virtual uint32  __stdcall Release() = 0;
	virtual long __stdcall GetCaps(void*) = 0;
	virtual long __stdcall EnumTextureFormats(void*, void*) = 0;
	virtual long __stdcall BeginScene() = 0;
	virtual long __stdcall EndScene() = 0;
};

extern CD3DDevice*				g_pD3DDevice;           // 0x100638c8
extern LPDIRECTDRAW7			g_pDirectDraw;          // 0x1005d2a8
extern LPDIRECTDRAWSURFACE7		g_pBackBuffer;          // 0x1005d2b4
extern LTBOOL					g_bIn3D;                // 0x100638d8
extern uint32					g_bOptimizeSurfaces;    // 0x100638dc

extern void*	dalloc_z(uint32 size);                  // 0x10013a7f (common_stuff.cpp)
extern void		dfree(void* ptr);                       // 0x10013ac5
extern void		AddDebugMessage(int32 level, const char* pMsg, ...);  // 0x10013b13
extern void		InvalidateRect(LTRect *pRect);          // 0x10022e31
extern void		dd_ConsolePrint(const char *pMsg, ...);  // 0x10013ade
extern void		DirtyRectSwap();                        // 0x100230a8 (dirtyrect.cpp)
extern void		ClearDirtyRects();                      // 0x100230fc

extern LPDIRECTDRAWSURFACE7		g_pFrontBuffer;         // 0x1005d2ac
extern LPDIRECTDRAWSURFACE7		g_pPrimarySurface;      // 0x1005d2b0
extern LPDIRECTDRAWCLIPPER		g_pClipper;             // 0x100638d0
extern LTBOOL					g_bRunWindowed;         // 0x1005d538
extern HWND						g_hWnd;                 // 0x1005d430
extern uint32					g_ScreenWidth;          // 0x1005d8cc
extern uint32					g_ScreenHeight;         // 0x1005d9c0
extern int32					g_ScreenScaleX;         // 0x1005d8bc
extern int32					g_ScreenScaleY;         // 0x1005d8c0

// d3d_SwapBuffers' flags.
#define RSWAP_BLIT			2
#define RSWAP_DIRTYRECTS	4


// The render data behind a CSurface.
struct D3DRenderData
{
	LPDIRECTDRAWSURFACE7	m_pSurface;                 // +0x00
	DDSURFACEDESC2			m_Desc;                     // +0x04
	uint32					m_ColorKey;                 // +0x80
	void*					m_pOptimized;               // +0x84
	uint32					m_88;                       // +0x88
};

struct RenderBlitRequest
{
	D3DRenderData*	m_pSurface;                         // +0x00
	uint32			m_BlitOptions;                      // +0x04
	uint32			m_TransparentColor;                 // +0x08
	LTRect*			m_pSrcRect;                         // +0x0c
	LTRect*			m_pDestRect;                        // +0x10
};

#define BLIT_TRANSPARENT	1

extern void		d3d_FreeOptimizedSurface(D3DRenderData *pData);                     // 0x1001cb20 (d3d_optimizedsurface.cpp)
extern LTBOOL	d3d_OptimizeSurface(D3DRenderData *pData, uint32 transparentColor);  // 0x1001cff4
extern void		d3d_BlitOptimized(RenderBlitRequest *pRequest);                      // 0x1001d4fa
extern void		d3d_WarpOptimized(RenderBlitRequest *pRequest);                      // 0x1001d890


// ---------------------------------------------------------------------- //
//  Globals.
// ---------------------------------------------------------------------- //
CD3DConVar	g_CV_LockOnFlip("LockOnFlip", 1.0f, LTNULL, LTNULL);       // 0x10066170  $E 0x1001e650/0x1001e655

RECT		g_ScreenLockRect;                           // 0x10066190
uint32		g_bScreenLocked;                            // 0x100661a0


// ---------------------------------------------------------------------- //
//  0x1001e66f
// ---------------------------------------------------------------------- //
D3DRenderData* d3d_CreateSurface(int width, int height)
{
	DDSURFACEDESC2 ddsd;
	LPDIRECTDRAWSURFACE7 pSurface;
	D3DRenderData *pData;

	if (!g_pDirectDraw || !width || !height)
		return LTNULL;

	memset(&ddsd, 0, sizeof(ddsd));
	ddsd.dwSize = sizeof(ddsd);
	ddsd.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH;
	ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY;
	ddsd.dwWidth = width;
	ddsd.dwHeight = height;

	if (g_pDirectDraw->CreateSurface(&ddsd, &pSurface, LTNULL) != DD_OK)
		return LTNULL;

	pData = (D3DRenderData*)dalloc_z(sizeof(D3DRenderData));
	if (!pData)
	{
		pSurface->Release();
		return LTNULL;
	}

	pData->m_pSurface = pSurface;
	pData->m_ColorKey = 0xFF000000;
	memset(&pData->m_Desc, 0, sizeof(pData->m_Desc));
	pData->m_Desc.dwSize = sizeof(pData->m_Desc);
	pSurface->GetSurfaceDesc(&pData->m_Desc);
	return pData;
}


// ---------------------------------------------------------------------- //
//  0x1001e71d
// ---------------------------------------------------------------------- //
void d3d_DeleteSurface(D3DRenderData *pData)
{
	if (pData)
	{
		d3d_FreeOptimizedSurface(pData);
		pData->m_pSurface->Release();
		dfree(pData);
	}
}


// ---------------------------------------------------------------------- //
//  0x1001e73e
// ---------------------------------------------------------------------- //
void d3d_GetSurfaceInfo(D3DRenderData *pData, uint32 *pWidth, uint32 *pHeight, long *pPitch)
{
	if (pData)
	{
		if (pWidth)
			*pWidth = pData->m_Desc.dwWidth;

		if (pHeight)
			*pHeight = pData->m_Desc.dwHeight;

		if (pPitch)
			*pPitch = pData->m_Desc.lPitch;
	}
}


// ---------------------------------------------------------------------- //
//  0x1001e76e
// ---------------------------------------------------------------------- //
void* d3d_LockSurface(D3DRenderData *pData)
{
	DDSURFACEDESC2 ddsd;
	HRESULT hResult;

	if (!pData)
		return LTNULL;

	ddsd.dwSize = sizeof(ddsd);
	hResult = pData->m_pSurface->Lock(LTNULL, &ddsd, 0, LTNULL);
	return (hResult == DD_OK) ? ddsd.lpSurface : LTNULL;
}


// ---------------------------------------------------------------------- //
//  0x1001e7a4
// ---------------------------------------------------------------------- //
void d3d_UnlockSurface(D3DRenderData *pData)
{
	if (pData)
		pData->m_pSurface->Unlock(LTNULL);
}


// ---------------------------------------------------------------------- //
//  0x1001e7ba
// ---------------------------------------------------------------------- //
LTBOOL d3d_LockScreen(int left, int top, int right, int bottom, void **pData, long *pPitch)
{
	DDSURFACEDESC2 ddsd;

	if (!g_pBackBuffer || g_bScreenLocked)
		return LTFALSE;

	g_ScreenLockRect.left = left;
	g_ScreenLockRect.top = top;
	g_ScreenLockRect.right = right;
	g_ScreenLockRect.bottom = bottom;

	ddsd.dwSize = sizeof(ddsd);
	if (g_pBackBuffer->Lock(&g_ScreenLockRect, &ddsd, DDLOCK_WAIT, LTNULL) != DD_OK)
		return LTFALSE;

	if (pData)
		*pData = ddsd.lpSurface;

	if (pPitch)
		*pPitch = ddsd.lPitch;

	g_bScreenLocked = LTTRUE;
	return LTTRUE;
}


// ---------------------------------------------------------------------- //
//  0x1001e83f
// ---------------------------------------------------------------------- //
void d3d_UnlockScreen()
{
	if (g_pBackBuffer && g_bScreenLocked)
	{
		g_pBackBuffer->Unlock(&g_ScreenLockRect);
		g_bScreenLocked = LTFALSE;
		InvalidateRect((LTRect*)&g_ScreenLockRect);
	}
}


// ---------------------------------------------------------------------- //
//  0x1001e871
// ---------------------------------------------------------------------- //
void d3d_BlitFromScreen(RenderBlitRequest *pRequest)
{
	D3DRenderData *pData;
	DDBLTFX fx;
	RECT rcSrc, rcDest;
	LTRect *pRect;

	pData = pRequest->m_pSurface;

	if (g_bIn3D)
	{
		AddDebugMessage(20, "Warning: drawing a nonoptimized surface while in 3D mode.");
		if (g_pD3DDevice)
			g_pD3DDevice->EndScene();
	}

	pRect = pRequest->m_pSrcRect;
	rcSrc.left = pRect->left;
	rcSrc.top = pRect->top;
	rcSrc.right = pRect->right;
	rcSrc.bottom = pRect->bottom;
	pRect = pRequest->m_pDestRect;
	rcDest.left = pRect->left;
	rcDest.top = pRect->top;
	rcDest.right = pRect->right;
	rcDest.bottom = pRect->bottom;
	fx.dwSize = sizeof(fx);

	pData->m_pSurface->Blt(&rcDest, g_pBackBuffer, &rcSrc, 0, &fx);

	if (g_bIn3D)
	{
		if (g_pD3DDevice)
			g_pD3DDevice->BeginScene();
	}
}


// ---------------------------------------------------------------------- //
//  0x1001e920
// ---------------------------------------------------------------------- //
void d3d_BlitToScreen_NonOptimized(RenderBlitRequest *pRequest)
{
	D3DRenderData *pData;
	DDBLTFX fx;
	RECT rcSrc, rcDest;
	LTRect *pRect;
	DDCOLORKEY ck;
	uint32 flags;

	pData = pRequest->m_pSurface;

	if (g_bIn3D)
	{
		AddDebugMessage(20, "Warning: drawing a nonoptimized surface while in 3D mode.");
		if (g_pD3DDevice)
			g_pD3DDevice->EndScene();
	}

	pRect = pRequest->m_pSrcRect;
	rcSrc.left = pRect->left;
	rcSrc.top = pRect->top;
	rcSrc.right = pRect->right;
	rcSrc.bottom = pRect->bottom;
	pRect = pRequest->m_pDestRect;
	rcDest.left = pRect->left;
	rcDest.top = pRect->top;
	rcDest.right = pRect->right;
	rcDest.bottom = pRect->bottom;
	flags = 0;
	fx.dwSize = sizeof(fx);

	if (pRequest->m_BlitOptions & BLIT_TRANSPARENT)
	{
		if (pData->m_ColorKey != pRequest->m_TransparentColor)
		{
			ck.dwColorSpaceHighValue = ck.dwColorSpaceLowValue = pRequest->m_TransparentColor;
			pData->m_pSurface->SetColorKey(DDCKEY_SRCBLT, &ck);
			pData->m_ColorKey = pRequest->m_TransparentColor;
		}

		flags = DDBLT_KEYSRC;
	}

	g_pBackBuffer->Blt(&rcDest, pData->m_pSurface, &rcSrc, flags, &fx);

	if (g_bIn3D)
	{
		if (g_pD3DDevice)
			g_pD3DDevice->BeginScene();
	}

	InvalidateRect((LTRect*)&rcDest);
}


// ---------------------------------------------------------------------- //
//  0x1001ea0c
// ---------------------------------------------------------------------- //
void d3d_BlitToScreen(RenderBlitRequest *pRequest)
{
	D3DRenderData *pData;

	if (g_pBackBuffer && pRequest && (pData = pRequest->m_pSurface) != LTNULL)
	{
		if (g_bOptimizeSurfaces && g_bIn3D && !pData->m_pOptimized)
			d3d_OptimizeSurface(pData, pRequest->m_TransparentColor);

		if (pData->m_pOptimized && g_bOptimizeSurfaces && g_bIn3D)
			d3d_BlitOptimized(pRequest);
		else
			d3d_BlitToScreen_NonOptimized(pRequest);
	}
}


// ---------------------------------------------------------------------- //
//  0x1001ea75
// ---------------------------------------------------------------------- //
LTBOOL d3d_WarpToScreen(RenderBlitRequest *pRequest)
{
	D3DRenderData *pData;

	if (g_pBackBuffer && pRequest && (pData = pRequest->m_pSurface) != LTNULL &&
		pData->m_pOptimized && g_bOptimizeSurfaces && g_bIn3D)
	{
		d3d_WarpOptimized(pRequest);
		return LTTRUE;
	}

	return LTFALSE;
}


// ---------------------------------------------------------------------- //
//  The pixel format and conversion request (ph/pixelformat.cpp's shapes);
//  the screen's format is common_init.cpp's g_ScreenFormat.  0x1001eac2
//  is PFormat's (compiler-made) assignment operator.
// ---------------------------------------------------------------------- //
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

	BPPIdent	m_eType;                                // +0x04
	uint32		m_Masks[4];                             // +0x08
	uint32		m_nBits[4];                             // +0x18
	uint32		m_FirstBits[4];                         // +0x28
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

class FormatMgr
{
public:
	LTRESULT		ConvertPixels(FMConvertRequest *pRequest);  // 0x1003764c
};

extern PFormat		g_ScreenFormat;                     // 0x1005d260
extern FormatMgr	g_FormatMgr;                        // 0x100661a8 (d3d_texture.cpp)


// ---------------------------------------------------------------------- //
//  0x1001eab3
// ---------------------------------------------------------------------- //
void d3d_GetScreenFormat(PFormat *pFormat)
{
	*pFormat = g_ScreenFormat;
}


// ---------------------------------------------------------------------- //
//  0x1001eb09
// ---------------------------------------------------------------------- //
void d3d_MakeScreenShot(const char *pFilename)
{
	FMConvertRequest request;
	CMoArray<DWORD> buffer;
	DDSURFACEDESC2 ddsd;
	BITMAPFILEHEADER bmfh;
	BITMAPINFOHEADER bmih;
	uint32 width, height, x, y;
	DWORD *pPixel;
	LTRESULT dResult;
	FILE *fp;

	if (!g_pBackBuffer)
		return;

	memset(&ddsd, 0, sizeof(ddsd));
	ddsd.dwSize = sizeof(ddsd);
	g_pBackBuffer->GetSurfaceDesc(&ddsd);

	width = ddsd.dwWidth;
	height = ddsd.dwHeight;
	if (!buffer.SetSize(width * height))
		return;

	ddsd.dwSize = sizeof(ddsd);
	HRESULT hResult = g_pBackBuffer->Lock(NULL, &ddsd, DDLOCK_WAIT, NULL);
	if (hResult != DD_OK)
	{
		dd_ConsolePrint("ScreenShot: g_pOffscreen->Lock returned %d.", hResult);
		return;
	}

	*request.m_pSrcFormat = g_ScreenFormat;
	request.m_pSrc = (uint8*)ddsd.lpSurface;
	request.m_SrcPitch = ddsd.lPitch;
	request.m_pDestFormat->Init(BPP_32, 0xFF000000, 0x00FF0000, 0x0000FF00, 0x000000FF);
	request.m_pDest = (uint8*)buffer.GetArray();
	request.m_DestPitch = width * sizeof(DWORD);
	request.m_Width = width;
	request.m_Height = height;
	request.m_Flags = 0;
	dResult = g_FormatMgr.ConvertPixels(&request);

	g_pBackBuffer->Unlock(NULL);

	if (dResult != LT_OK)
	{
		dd_ConsolePrint("ScreenShot: FormatMgr::ConvertPixels returned %d.", dResult);
		return;
	}

	fp = fopen(pFilename, "wb");
	if (!fp)
	{
		dd_ConsolePrint("ScreenShot: fopen(%s) failed.", pFilename);
		return;
	}

	memset(&bmfh, 0, sizeof(bmfh));
	memset(&bmih, 0, sizeof(bmih));

	bmfh.bfType = 0x4D42;	// 'BM'
	bmfh.bfSize = sizeof(bmfh) + sizeof(bmih) + width * height * 3;
	bmfh.bfOffBits = sizeof(bmfh) + sizeof(bmih);

	bmih.biSize = sizeof(bmih);
	bmih.biWidth = width;
	bmih.biHeight = height;
	bmih.biPlanes = 1;
	bmih.biBitCount = 24;
	bmih.biCompression = BI_RGB;

	fwrite(&bmfh, sizeof(bmfh), 1, fp);
	fwrite(&bmih, sizeof(bmih), 1, fp);

	// The bitmap goes bottom up.
	for (y = 0; y < height; y++)
	{
		pPixel = &buffer[(height - y - 1) * width];
		for (x = 0; x < width; x++)
		{
			fwrite(pPixel, 3, 1, fp);
			pPixel++;
		}
	}

	fclose(fp);
	dd_ConsolePrint("ScreenShot: Created %s successfully.", pFilename);
}


// ---------------------------------------------------------------------- //
//  0x1001ed75
// ---------------------------------------------------------------------- //
void d3d_SwapBuffers(uint32 flags)
{
	RECT rcLock, rcClient, rcSrc, rcDest;
	DDSURFACEDESC2 ddsd;
	DDBLTFX fx;
	POINT pt;
	HWND hWnd;

	if (!g_pPrimarySurface)
		return;

	if (flags & RSWAP_DIRTYRECTS)
	{
		DirtyRectSwap();
		return;
	}

	// Lock a corner of the back buffer, so the card finishes the frame
	// before we flip.
	if (g_CV_LockOnFlip.m_nValue && !g_bScreenLocked)
	{
		rcLock.left = 0;
		rcLock.top = 0;
		rcLock.right = 4;
		rcLock.bottom = 4;
		ddsd.dwSize = sizeof(ddsd);
		if (SUCCEEDED(g_pBackBuffer->Lock(&rcLock, &ddsd, DDLOCK_WAIT, NULL)))
			g_pBackBuffer->Unlock(&rcLock);
	}

	if (g_pFrontBuffer)
	{
		if (g_bRunWindowed)
		{
			GetClientRect(g_hWnd, &rcClient);
			pt.x = rcClient.left;
			pt.y = rcClient.top;
			ClientToScreen(g_hWnd, &pt);
			hWnd = g_hWnd;
		}
		else
		{
			hWnd = GetDesktopWindow();
			GetWindowRect(hWnd, &rcClient);
			pt.x = rcClient.left;
			pt.y = rcClient.top;
		}

		if (!g_pClipper && g_pDirectDraw)
		{
			if (g_pDirectDraw->CreateClipper(0, &g_pClipper, NULL) == DD_OK && g_pClipper)
			{
				g_pClipper->SetHWnd(0, hWnd);
				g_pFrontBuffer->SetClipper(g_pClipper);
			}
		}

		rcSrc.left = 0;
		rcSrc.top = 0;
		rcSrc.right = g_ScreenWidth;
		rcSrc.bottom = g_ScreenHeight;

		rcDest.left = pt.x;
		rcDest.top = pt.y;
		rcDest.right = pt.x + g_ScreenWidth * g_ScreenScaleX;
		rcDest.bottom = pt.y + g_ScreenHeight * g_ScreenScaleY;

		memset(&fx, 0, sizeof(fx));
		fx.dwSize = sizeof(fx);

		rcDest.left = pt.x;
		rcDest.top = pt.y;
		rcDest.right = pt.x + g_ScreenWidth;
		rcDest.bottom = pt.y + g_ScreenHeight;

		memset(&fx, 0, sizeof(fx));
		fx.dwSize = sizeof(fx);

		g_pFrontBuffer->Blt(&rcDest, g_pPrimarySurface, &rcSrc, DDBLT_WAIT, &fx);
	}
	else if (flags & RSWAP_BLIT)
	{
		g_pPrimarySurface->Blt(NULL, g_pBackBuffer, NULL, DDBLT_WAIT, NULL);
	}
	else
	{
		g_pPrimarySurface->Flip(NULL, DDFLIP_WAIT);
	}

	ClearDirtyRects();
}
