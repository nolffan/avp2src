// *********************************************************************** //
//  ph/d3d_surface.cpp -- Primal Hunt's d3d.ren, TU19
//  (0x1001e650..0x1001f273), /O1.
//
//  The engine's 2D surfaces (system-memory DirectDraw surfaces behind the
//  RenderStruct's surface slots): create/delete, lock/unlock, the screen
//  lock, the blits to and from the back buffer, and the LockOnFlip
//  console variable.
//
//  Not yet written here: the screenshot (0x1001eb09), the flip
//  (0x1001ed75), and the CMoArray<DWORD> members this object instantiates
//  first (0x1001ef88..0x1001f25b).
// *********************************************************************** //

#include <windows.h>
#include <ddraw.h>
#include "ltbasedefs.h"


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
class CD3DDevice
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
extern uint32					g_bIn3D;                // 0x100638d8
extern uint32					g_bOptimizeSurfaces;    // 0x100638dc

extern void*	dalloc_z(uint32 size);                  // 0x10013a7f (common_stuff.cpp)
extern void		dfree(void* ptr);                       // 0x10013ac5
extern void		AddDebugMessage(int level, const char* pMsg, ...);  // 0x10013b13
extern void		InvalidateRect(LTRect *pRect);          // 0x10022e31


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

extern void		d3d_UnoptimizeSurface(D3DRenderData *pData);                        // 0x1001cb20
extern void		d3d_OptimizeSurface(D3DRenderData *pData, uint32 transparentColor);  // 0x1001cff4
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
		d3d_UnoptimizeSurface(pData);
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
//  0x1001eab3 / 0x1001eac2 -- a copy of the renderer's mode description;
//  0x1001eac2 is its class's (compiler-made) assignment operator.
// ---------------------------------------------------------------------- //
class RenderModeInfo
{
public:
	virtual void	Clear();

	uint32		m_Flags;                                // +0x04
	uint32		m_Data08[4];                            // +0x08
	uint32		m_Data18[4];                            // +0x18
	uint32		m_Data28[4];                            // +0x28
};

extern RenderModeInfo	g_RenderModeInfo;               // 0x1005d260

void d3d_GetRenderModeInfo(RenderModeInfo *pInfo)
{
	*pInfo = g_RenderModeInfo;
}
