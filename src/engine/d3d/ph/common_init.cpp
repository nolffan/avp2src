// *********************************************************************** //
//
//  MODULE:   COMMON_INIT (.CPP) -- Primal Hunt's d3d.ren, TU11
//            (0x100116e3..0x10011eec, docs/D3D_OBJECTS.md)
//
//  PURPOSE:  The renderer's start-up and shut-down: the DirectDraw device
//            list, d3d_Init / d3d_Term, the display-mode enumeration the
//            engine asks for (GetSupportedModes, FreeModeList) and
//            RenderDLLSetup, which fills in the RenderStruct.
//
// *********************************************************************** //

#include <windows.h>
#include <ddraw.h>
#include <string.h>
#include <stdlib.h>
#include "ltbasedefs.h"


#define LTRENDER_VERSION    3421

#define RENDER_OK           0
#define RENDER_ERROR        1


// The engine's RenderStruct, as far as this file reaches into it.
typedef void* HLTPARAM;

struct RenderStructInit
{
	int         m_RendererVersion;                          // +0x000
	RMode       m_Mode;                                     // +0x004
	void*       m_hWnd;                                     // +0x218
};

class PFormat;
struct SharedTexture;
struct SceneDesc;
struct BlitRequest;
typedef void* HLTBUFFER;

struct RenderStruct
{
	uint8       m_Reserved00[0x14];
	void      (*ConsolePrint)(char* pMsg, ...);             // +0x14
	HLTPARAM  (*GetParameter)(char* pName);                 // +0x18
	float     (*GetParameterValueFloat)(HLTPARAM hParam);   // +0x1c
	char*     (*GetParameterValueString)(HLTPARAM hParam);  // +0x20
	uint8       m_Reserved24[0x3c - 0x24];
	uint32      m_Width;                                    // +0x3c
	uint32      m_Height;                                   // +0x40
	uint8       m_Reserved44[0x70 - 0x44];

	// Filled in by the renderer (RenderDLLSetup).
	int         (*Init)(RenderStructInit* pInit);           // +0x70
	void        (*Term)();                                  // +0x74
	void        (*BindTexture)(SharedTexture* pTexture, LTBOOL bTextureChanged);  // +0x78
	void        (*UnbindTexture)(SharedTexture* pTexture);  // +0x7c
	LTBOOL      (*LoadWorldData)(void* pWorldData);         // +0x80
	void*       (*CreateWorld)(void** pWorldData);          // +0x84
	void        (*DestroyWorld)(void* pWorld);              // +0x88
	void        (*Clear)(LTRect* pRect, uint32 flags);      // +0x8c
	LTBOOL      (*Start3D)();                               // +0x90
	LTBOOL      (*End3D)();                                 // +0x94
	LTBOOL      (*IsIn3D)();                                // +0x98
	LTBOOL      (*StartOptimized2D)();                      // +0x9c
	void        (*EndOptimized2D)();                        // +0xa0
	LTBOOL      (*IsInOptimized2D)();                       // +0xa4
	LTBOOL      (*SetOptimized2DBlend)(LTSurfaceBlend blend);   // +0xa8
	LTBOOL      (*GetOptimized2DBlend)(LTSurfaceBlend* pBlend); // +0xac
	LTBOOL      (*SetOptimized2DColor)(uint32 color);       // +0xb0
	LTBOOL      (*GetOptimized2DColor)(uint32* pColor);     // +0xb4
	int         (*RenderScene)(SceneDesc* pScene);          // +0xb8
	void        (*RenderCommand)(int argc, char** argv);    // +0xbc
	void*       (*GetHook)(char* pName);                    // +0xc0
	void        (*SwapBuffers)(uint32 flags);               // +0xc4
	uint32      (*GetInfoFlags)();                          // +0xc8
	LTBOOL      (*GetScreenFormat)(PFormat* pFormat);       // +0xcc
	HLTBUFFER   (*CreateSurface)(int width, int height);    // +0xd0
	void        (*DeleteSurface)(HLTBUFFER hSurf);          // +0xd4
	void        (*GetSurfaceInfo)(HLTBUFFER hSurf, uint32* pWidth, uint32* pHeight, int* pPitch);  // +0xd8
	void*       (*LockSurface)(HLTBUFFER hSurf);            // +0xdc
	void        (*UnlockSurface)(HLTBUFFER hSurf);          // +0xe0
	LTBOOL      (*OptimizeSurface)(HLTBUFFER hSurf, uint32 transparentColor);  // +0xe4
	void        (*UnoptimizeSurface)(HLTBUFFER hSurf);      // +0xe8
	LTBOOL      (*LockScreen)(int left, int top, int right, int bottom, void** pData, long* pPitch);  // +0xec
	void        (*UnlockScreen)();                          // +0xf0
	void        (*BlitToScreen)(BlitRequest* pRequest);     // +0xf4
	LTBOOL      (*WarpToScreen)(BlitRequest* pRequest);     // +0xf8
	void        (*MakeScreenShot)(const char* pFilename);   // +0xfc
	void        (*ReadConsoleVariables)();                  // +0x100
	void        (*BlitFromScreen)(BlitRequest* pRequest);   // +0x104
};

// One DirectDraw driver, as the device enumeration lists it.
struct D3DDeviceInfo
{
	uint8       m_Reserved00[0x20];
	GUID*       m_pDriverGUID;                              // +0x20
	char        m_Name[100];                                // +0x24
	char        m_Desc[100];                                // +0x88
};

// A pixel format (lttexmod.cpp's).
// pixelformat.cpp's (TU41)
enum BPPIdent
{
	BPP_8P=0,
	BPP_8,
	BPP_16,
	BPP_32,
	BPP_S3TC_DXT1,
	BPP_S3TC_DXT3,
	BPP_S3TC_DXT5,
	BPP_32P,
	NUM_BIT_TYPES
};

class PFormat
{
public:
	virtual void Init(BPPIdent Type, uint32 aMask, uint32 rMask,
	                  uint32 gMask, uint32 bMask);

	uint32      m_nType;
	uint32      m_Masks[4];
	uint32      m_nBits[4];
	uint32      m_nShifts[4];
};

struct VisibleSet
{
	LTBOOL      Init();                                     // 0x10039aa0
};


extern RenderStruct* g_pStruct;                             // 0x1005df08 (common_stuff.cpp)
extern int32 g_1005d4a8;                                    // 0x1005d4a8 (common_stuff.cpp)

extern LTBOOL g_bRunWindowed;                               // 0x1005d538
extern int32  g_CV_SysMem;                                  // 0x1005d8c8
extern HWND   g_hWnd;                                       // 0x1005d430
extern uint32 g_1005d2c0;                                   // 0x1005d2c0
extern uint32 g_ScreenWidth, g_ScreenHeight;                // 0x1005d8cc, 0x1005d9c0
extern uint32 g_1005d8bc, g_1005d8c0;                       // 0x1005d8bc, 0x1005d8c0
extern uint32 g_1005d5f0;                                   // 0x1005d5f0
extern uint32 g_1005df7c, g_1005df80, g_1005df84, g_1005df88;
extern LPDIRECTDRAWSURFACE4 g_100638d0;                     // 0x100638d0 (d3d_init.cpp)

void    d3d_CreateConsoleVariables();                       // 0x10013ba7
void    d3d_ReadConsoleVariables();                         // 0x10013bcd
void    AddDebugMessage(int32 debugLevel, const char* pMsg, ...);   // 0x10013b13
void    dfree(void* ptr);                                   // 0x10013ac5
void    MyStrUpr(char* pStr);                               // 0x10013c30
LTBOOL  d3d_EnumDevices();                                  // 0x10032af0
LTBOOL  d3d_InitDevice(D3DDeviceInfo* pDevice, RenderStructInit* pInit);  // 0x1001b8d0
void    d3d_ReleaseDevice();                        // 0x1001c3f0
void    sub_1001c450();                                     // 0x1001c450
void    sub_1001c480();                                     // 0x1001c480
void    sub_10014550();                                     // 0x10014550
VisibleSet* d3d_GetVisibleSet();                            // 0x1003abb0

// The RenderStruct functions in other modules.
void    BindTexture(SharedTexture* pTexture, LTBOOL bTextureChanged);   // 0x10022830
void    UnbindTexture(SharedTexture* pTexture);             // 0x10022940
LTBOOL  d3d_LoadWorldData(void* pWorldData);                // 0x1001c3a0
void*   d3d_CreateWorld(void** pWorldData);                 // 0x1001c310
void    d3d_DestroyWorld(void* pWorld);                     // 0x1001c380
void    d3d_Clear(LTRect* pRect, uint32 flags);             // 0x10014d70
LTBOOL  d3d_Start3D();                                      // 0x1001c990
LTBOOL  d3d_End3D();                                        // 0x1001c9c0
LTBOOL  d3d_IsIn3D();                                       // 0x1001ca30
LTBOOL  d3d_StartOptimized2D();                             // 0x1001d237
void    d3d_EndOptimized2D();                               // 0x1001d335
LTBOOL  d3d_IsInOptimized2D();                              // 0x1001d3fb
LTBOOL  d3d_SetOptimized2DBlend(LTSurfaceBlend blend);      // 0x1001d401
LTBOOL  d3d_GetOptimized2DBlend(LTSurfaceBlend* pBlend);    // 0x1001d4da
LTBOOL  d3d_SetOptimized2DColor(uint32 color);              // 0x1001d4c8
LTBOOL  d3d_GetOptimized2DColor(uint32* pColor);            // 0x1001d4ea
struct D3DRenderData;   // a surface's data (d3d_surface.cpp): an HLTBUFFER to the engine
LTBOOL  d3d_OptimizeSurface(D3DRenderData* pData, uint32 transparentColor);  // 0x1001cff4
void    d3d_UnoptimizeSurface(D3DRenderData* pData);        // 0x1001cfe2
int     d3d_RenderScene(SceneDesc* pScene);                 // 0x10018520
void    d3d_RenderCommand(int argc, char** argv);           // 0x1001c4b0
void    d3d_SwapBuffers(uint32 flags);                      // 0x1001ed75
uint32  d3d_GetInfoFlags();                                 // 0x1001c980
LTBOOL  d3d_GetScreenFormat(PFormat* pFormat);              // 0x1001eab3
HLTBUFFER d3d_CreateSurface(int width, int height);         // 0x1001e66f
void    d3d_DeleteSurface(HLTBUFFER hSurf);                 // 0x1001e71d
void    d3d_GetSurfaceInfo(HLTBUFFER hSurf, uint32* pWidth, uint32* pHeight, int* pPitch);  // 0x1001e73e
void*   d3d_LockSurface(HLTBUFFER hSurf);                   // 0x1001e76e
void    d3d_UnlockSurface(HLTBUFFER hSurf);                 // 0x1001e7a4
LTBOOL  d3d_LockScreen(int left, int top, int right, int bottom, void** pData, long* pPitch);  // 0x1001e7ba
void    d3d_UnlockScreen();                                 // 0x1001e83f
void    d3d_MakeScreenShot(const char* pFilename);          // 0x1001eb09
void    d3d_BlitToScreen(BlitRequest* pRequest);            // 0x1001ea0c
void    d3d_BlitFromScreen(BlitRequest* pRequest);          // 0x1001e871
LTBOOL  d3d_WarpToScreen(BlitRequest* pRequest);            // 0x1001ea75


// The DirectDraw device list, filled by d3d_EnumDevices.
LTLink  g_Devices(LTLink_Init);                             // 0x1005d298

// The back buffer's pixel format.
PFormat g_ScreenFormat;                                     // 0x1005d260

RMode*          g_pModeList;                                // 0x1005d258
D3DDeviceInfo*  g_pCurDevice;                               // 0x1005d2a4
LPDIRECTDRAW4           g_pDirectDraw;                      // 0x1005d2a8
LPDIRECTDRAWSURFACE4    g_pFrontBuffer;                     // 0x1005d2ac
LPDIRECTDRAWSURFACE4    g_pPrimarySurface;                  // 0x1005d2b0
LPDIRECTDRAWSURFACE4    g_pBackBuffer;                      // 0x1005d2b4


// 0x100116fe
void d3d_FreeDeviceList()
{
	LTLink *pCur, *pNext;

	pCur = g_Devices.m_pNext;
	while (pCur != &g_Devices)
	{
		pNext = pCur->m_pNext;
		dfree(pCur->m_pData);
		pCur = pNext;
	}

	dl_TieOff(&g_Devices);
}

// 0x1001172f
void d3d_ListDevices()
{
	LTLink* pCur;

	for (pCur = g_Devices.m_pPrev; pCur != &g_Devices; pCur = pCur->m_pPrev)
	{
		g_pStruct->ConsolePrint("Device: %s", ((D3DDeviceInfo*)pCur->m_pData)->m_Name);
	}
}

// 0x1001175d
void* d3d_GetRenderObject(char* pName)
{
	if (strcmp(pName, "LPDIRECTDRAW") == 0)
		return g_pDirectDraw;
	else if (strcmp(pName, "BACKBUFFER") == 0)
		return g_pBackBuffer;
	else
		return LTNULL;
}

// 0x10011794
void r_SetPFormat(DDPIXELFORMAT* pPF, PFormat* pFormat)
{
	BPPIdent nType;

	if (pPF->dwRGBBitCount == 16)
		nType = BPP_16;
	else
		nType = BPP_32;

	pFormat->Init(nType, pPF->dwRGBAlphaBitMask, pPF->dwRBitMask, pPF->dwGBitMask, pPF->dwBBitMask);
}

// 0x100117bb
LTBOOL r_GetBufferFormatOfSurface(LPDIRECTDRAWSURFACE4 pSurface, PFormat* pFormat)
{
	DDPIXELFORMAT pf;

	memset(&pf, 0, sizeof(pf));
	pf.dwSize = sizeof(pf);
	if (pSurface->GetPixelFormat(&pf) < 0)
		return LTFALSE;

	r_SetPFormat(&pf, pFormat);
	return LTTRUE;
}

// 0x10011800
void d3d_FreeDevice()
{
	ShowCursor(TRUE);
	d3d_ReleaseDevice();

	if (g_pFrontBuffer)
	{
		g_pFrontBuffer->Release();
		g_pFrontBuffer = LTNULL;

		if (g_pPrimarySurface)
		{
			g_pPrimarySurface->Release();
			g_pPrimarySurface = LTNULL;
		}

		g_pBackBuffer = LTNULL;

		if (g_100638d0)
		{
			g_100638d0->Release();
			g_100638d0 = LTNULL;
		}
	}
	else
	{
		if (g_pPrimarySurface)
		{
			g_pPrimarySurface->Release();
			g_pPrimarySurface = LTNULL;
		}

		if (g_pBackBuffer)
			g_pBackBuffer = LTNULL;
	}

	if (g_pDirectDraw)
	{
		g_pDirectDraw->RestoreDisplayMode();
		g_pDirectDraw->SetCooperativeLevel(NULL, DDSCL_NORMAL);
		g_pDirectDraw->Release();
		g_pDirectDraw = LTNULL;
	}
}

// 0x100118aa
void d3d_Term()
{
	d3d_FreeDeviceList();
	sub_1001c450();
	d3d_FreeDevice();
	sub_10014550();
}

static LTBOOL d3d_IsNullRenderOn();
static D3DDeviceInfo* d3d_FindDevice(char* pName);

// 0x100118be
int d3d_Init(RenderStructInit* pInit)
{
	char* pStr;
	RECT wndRect, screenRect;
	D3DDeviceInfo* pDevice;
	LTLink* pCur;

	pInit->m_RendererVersion = LTRENDER_VERSION;

	// Read the initial console variables in.
	d3d_CreateConsoleVariables();
	d3d_ReadConsoleVariables();

	// Run windowed?
	pStr = g_pStruct->GetParameterValueString(g_pStruct->GetParameter("windowed"));
	g_bRunWindowed = (pStr && atoi(pStr) == 1);

	g_CV_SysMem = (int32)g_pStruct->GetParameterValueFloat(g_pStruct->GetParameter("SysMem"));

	// Init globals.
	g_hWnd = (HWND)pInit->m_hWnd;
	g_1005d4a8 = 0;
	g_1005d2c0 = 0x4000;
	g_1005d8c0 = g_1005d8bc = 1;
	g_ScreenWidth = pInit->m_Mode.m_Width;
	g_ScreenHeight = pInit->m_Mode.m_Height;

	if (!d3d_EnumDevices())
		return 10;

	// First try to find your favorite device.
	if (pInit->m_Mode.m_InternalName[0])
	{
		pDevice = d3d_FindDevice(pInit->m_Mode.m_InternalName);
		if (pDevice)
		{
			if (d3d_InitDevice(pDevice, pInit))
				goto DEVICE_OK;

			AddDebugMessage(1, "Can't initialize hardware device: %s", pInit->m_Mode.m_InternalName);
		}
		else
		{
			AddDebugMessage(1, "Can't find hardware device: %s", pInit->m_Mode.m_InternalName);
		}
	}

	// If you can't, pick any one that works.
	for (pCur = g_Devices.m_pNext; pCur != &g_Devices; pCur = pCur->m_pNext)
	{
		pDevice = (D3DDeviceInfo*)pCur->m_pData;
		if (d3d_InitDevice(pDevice, pInit))
			goto DEVICE_OK;
	}
	pDevice = LTNULL;

DEVICE_OK:
	if (!pDevice)
	{
		d3d_Term();
		AddDebugMessage(0, "Can't find any d3d devices to use!");
		return RENDER_ERROR;
	}

	// Copy the info for the device we used into the RMode.
	strncpy(pInit->m_Mode.m_InternalName, pDevice->m_Name, sizeof(pInit->m_Mode.m_InternalName)-1);
	strncpy(pInit->m_Mode.m_Description, pDevice->m_Desc, sizeof(pInit->m_Mode.m_Description)-1);

	AddDebugMessage(0, "Using Direct3D Device %s", pDevice->m_Name);

	g_1005d5f0 = 0;
	g_1005df7c = 0;
	g_1005df80 = 0;
	g_1005df84 = 0;
	g_1005df88 = 0;

	// Either maximize the window or set its size.
	if (g_bRunWindowed || d3d_IsNullRenderOn())
	{
		GetWindowRect(GetDesktopWindow(), &screenRect);

		// Setup the client rectangle.
		// RETAIL BUG: the top is centred on the screen's width, not its
		// height, so it is the same as the left.
		wndRect.left = ((screenRect.right - screenRect.left) - pInit->m_Mode.m_Width) / 2;
		wndRect.top = ((screenRect.right - screenRect.left) - pInit->m_Mode.m_Width) / 2;
		wndRect.right = wndRect.left + pInit->m_Mode.m_Width;
		wndRect.bottom = wndRect.top + pInit->m_Mode.m_Height;

		// Figure out the full window coordinates given the client coordinates.
		AdjustWindowRect(&wndRect, WS_OVERLAPPEDWINDOW, FALSE);

		if (wndRect.left < 0)
		{
			wndRect.right -= wndRect.left;
			wndRect.left = 0;
		}

		if (wndRect.top < 0)
		{
			wndRect.bottom -= wndRect.top;
			wndRect.top = 0;
		}

		SetWindowPos(g_hWnd, 0, wndRect.left, wndRect.top,
			wndRect.right - wndRect.left, wndRect.bottom - wndRect.top, SWP_NOREPOSITION);
	}
	else
	{
		SetWindowPos(g_hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE);
	}

	if (!r_GetBufferFormatOfSurface(g_pPrimarySurface, &g_ScreenFormat))
	{
		d3d_Term();
		AddDebugMessage(0, "r_GetBufferFormatOfSurface failed.");
		return RENDER_ERROR;
	}

	sub_1001c480();

	g_pStruct->m_Width /= g_1005d8bc;
	g_pStruct->m_Height /= g_1005d8c0;

	if (!d3d_GetVisibleSet()->Init())
	{
		d3d_Term();
		AddDebugMessage(0, "VisibleSet::Init failed (invalid object list size?).");
		return RENDER_ERROR;
	}

	return RENDER_OK;
}

// 0x10011b8e
static LTBOOL d3d_IsNullRenderOn()
{
	return g_pStruct->GetParameterValueFloat(g_pStruct->GetParameter("nullrender")) != 0.0f;
}

// 0x10011bb7
static D3DDeviceInfo* d3d_FindDevice(char* pName)
{
	char upperName[100], upperDevice[100];
	LTLink* pCur;
	D3DDeviceInfo* pDevice;

	strncpy(upperName, pName, sizeof(upperName));
	MyStrUpr(upperName);

	for (pCur = g_Devices.m_pNext; pCur != &g_Devices; pCur = pCur->m_pNext)
	{
		pDevice = (D3DDeviceInfo*)pCur->m_pData;

		strncpy(upperDevice, pDevice->m_Name, sizeof(upperDevice));
		MyStrUpr(upperDevice);

		if (strcmp(upperName, upperDevice) == 0)
			return pDevice;
	}

	return LTNULL;
}

// 0x10011c34
HRESULT WINAPI d3d_EnumModesCallback(LPDDSURFACEDESC2 pDesc, LPVOID pContext)
{
	RMode* pMode;

	if (pDesc->ddpfPixelFormat.dwRGBBitCount == 16 || pDesc->ddpfPixelFormat.dwRGBBitCount == 32)
	{
		pMode = (RMode*)malloc(sizeof(RMode));
		if (pMode)
		{
			pMode->m_bHardware = LTTRUE;
			strncpy(pMode->m_InternalName, g_pCurDevice->m_Name, sizeof(pMode->m_InternalName)-1);
			strncpy(pMode->m_Description, g_pCurDevice->m_Desc, sizeof(pMode->m_Description)-1);
			pMode->m_Width = pDesc->dwWidth;
			pMode->m_Height = pDesc->dwHeight;
			pMode->m_BitDepth = pDesc->ddpfPixelFormat.dwRGBBitCount;

			pMode->m_pNext = g_pModeList;
			g_pModeList = pMode;
		}
	}

	return DDENUMRET_OK;
}


extern "C"
{

// 0x10011cc5
__declspec(dllexport) RMode* GetSupportedModes()
{
	LTLink* pCur;
	D3DDeviceInfo* pDevice;
	LPDIRECTDRAW pDD;
	LPDIRECTDRAW4 pDD4;

	g_pModeList = LTNULL;
	d3d_EnumDevices();

	for (pCur = g_Devices.m_pNext; pCur != &g_Devices; pCur = pCur->m_pNext)
	{
		pDevice = (D3DDeviceInfo*)pCur->m_pData;

		if (DirectDrawCreate(pDevice->m_pDriverGUID, &pDD, LTNULL) == DD_OK)
		{
			if (pDD->QueryInterface(IID_IDirectDraw4, (void**)&pDD4) == DD_OK)
			{
				g_pCurDevice = pDevice;
				pDD4->EnumDisplayModes(0, LTNULL, LTNULL, d3d_EnumModesCallback);
				pDD4->Release();
			}

			pDD->Release();
		}
	}

	d3d_FreeDeviceList();
	return g_pModeList;
}

// 0x10011d54
__declspec(dllexport) void FreeModeList(RMode* pModes)
{
	RMode *pCur, *pNext;

	pCur = pModes;
	while (pCur)
	{
		pNext = pCur->m_pNext;
		free(pCur);
		pCur = pNext;
	}
}


// 0x10011d72
__declspec(dllexport) void RenderDLLSetup(RenderStruct* pStruct)
{
	g_pStruct = pStruct;

	pStruct->Init                   = d3d_Init;
	pStruct->Term                   = d3d_Term;
	pStruct->BindTexture            = BindTexture;
	pStruct->UnbindTexture          = UnbindTexture;
	pStruct->LoadWorldData          = d3d_LoadWorldData;
	pStruct->CreateWorld            = d3d_CreateWorld;
	pStruct->DestroyWorld           = d3d_DestroyWorld;
	pStruct->Clear                  = d3d_Clear;
	pStruct->Start3D                = d3d_Start3D;
	pStruct->End3D                  = d3d_End3D;
	pStruct->IsIn3D                 = d3d_IsIn3D;
	pStruct->StartOptimized2D       = d3d_StartOptimized2D;
	pStruct->EndOptimized2D         = d3d_EndOptimized2D;
	pStruct->SetOptimized2DBlend    = d3d_SetOptimized2DBlend;
	pStruct->GetOptimized2DBlend    = d3d_GetOptimized2DBlend;
	pStruct->SetOptimized2DColor    = d3d_SetOptimized2DColor;
	pStruct->GetOptimized2DColor    = d3d_GetOptimized2DColor;
	pStruct->IsInOptimized2D        = d3d_IsInOptimized2D;
	pStruct->OptimizeSurface        = (LTBOOL (*)(HLTBUFFER, uint32))d3d_OptimizeSurface;
	pStruct->UnoptimizeSurface      = (void (*)(HLTBUFFER))d3d_UnoptimizeSurface;
	pStruct->RenderScene            = d3d_RenderScene;
	pStruct->RenderCommand          = d3d_RenderCommand;
	pStruct->GetHook                = d3d_GetRenderObject;
	pStruct->SwapBuffers            = d3d_SwapBuffers;
	pStruct->GetInfoFlags           = d3d_GetInfoFlags;
	pStruct->GetScreenFormat        = d3d_GetScreenFormat;
	pStruct->CreateSurface          = d3d_CreateSurface;
	pStruct->DeleteSurface          = d3d_DeleteSurface;
	pStruct->GetSurfaceInfo         = d3d_GetSurfaceInfo;
	pStruct->LockSurface            = d3d_LockSurface;
	pStruct->UnlockSurface          = d3d_UnlockSurface;
	pStruct->LockScreen             = d3d_LockScreen;
	pStruct->UnlockScreen           = d3d_UnlockScreen;
	pStruct->MakeScreenShot         = d3d_MakeScreenShot;
	pStruct->ReadConsoleVariables   = d3d_ReadConsoleVariables;
	pStruct->BlitToScreen           = d3d_BlitToScreen;
	pStruct->BlitFromScreen         = d3d_BlitFromScreen;
	pStruct->WarpToScreen           = d3d_WarpToScreen;
}

}
