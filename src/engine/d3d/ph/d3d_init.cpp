// *********************************************************************** //
//
//  MODULE:   D3D_INIT (.CPP) -- Primal Hunt's d3d.ren, TU17
//            (0x1001a720..0x1001ca36, docs/D3D_OBJECTS.md)
//
//  PURPOSE:  Device start-up: the DirectDraw/Direct3D error strings, the
//            device's console variables, card detection, the z-buffer and
//            the device creation, the world create/destroy and 3D-frame
//            entry points and the renderer's console commands.
//
//  Not yet written here: d3d_InitDevice (0x1001b8d0) and RenderCommand
//  (0x1001c4b0).  Near: 0x1001b4c0 (the card detection: ours keeps the
//  constant 1 in %edi for the whole function where retail pushes %edi only
//  around the identifier's memset), 0x1001b010.
//
// *********************************************************************** //

#include <windows.h>
#include <ddraw.h>
#include <d3d.h>
#include <math.h>
#include "ltbasedefs.h"


// A console variable (common_stuff.cpp).
typedef void* HLTPARAM;

class CD3DConVar
{
public:
	CD3DConVar(const char* pName, float fDefault, int32* pnValue, float* pfValue);

	int32       m_nValue;                                   // +0x00
	float       m_fValue;                                   // +0x04
	float       m_fDefault;                                 // +0x08
	int32*      m_pnValue;                                  // +0x0c
	float*      m_pfValue;                                  // +0x10
	const char* m_pName;                                    // +0x14
	HLTPARAM    m_hParam;                                   // +0x18
	CD3DConVar* m_pNext;                                    // +0x1c
};


// The console variables this file reads (common_stuff.cpp).
extern CD3DConVar g_CV_TableFogVar;                         // 0x1005d2f0
extern int32 g_CV_Bilinear;                                 // 0x1005d2c4
extern int32 g_CV_FogEnable;                                // 0x1005df34
extern float g_CV_FogNearZ;                                 // 0x1005df38
extern float g_CV_FogFarZ;                                  // 0x1004a754
extern int32 g_CV_FogR, g_CV_FogG, g_CV_FogB;               // 0x1004a748..
extern int32 g_CV_Dither;                                   // 0x1004a794


// The rest of the renderer.
void*   dalloc_z(uint32 size);                              // 0x10013a7f (common_stuff.cpp)
void    dfree(void* ptr);                                   // 0x10013ac5
void    dd_ConsolePrint(const char* pMsg, ...);             // 0x10013ade
void    AddDebugMessage(int32 debugLevel, const char* pMsg, ...);   // 0x10013b13
extern int32 g_CV_NoLMPages;                                // 0x1005d42c

extern LPDIRECTDRAWSURFACE7 g_pPrimarySurface;              // 0x1005d2b0 (common_init.cpp)
extern LPDIRECTDRAWSURFACE7 g_pBackBuffer;                  // 0x1005d2b4
extern LPDIRECTDRAWSURFACE7 g_pZBuffer;                     // 0x1005d2b8
extern LPDIRECTDRAW7        g_pDirectDraw;                  // 0x1005d2a8 (common_init.cpp)
extern uint32               g_1005d2c0;                     // 0x1005d2c0 (common_init.cpp: the surfaces' memory caps)
extern LTBOOL g_bInOptimized2D;                             // 0x100638dc

LTBOOL  d3d_CreateLightmapPages(void* pWorld);              // 0x10035389
void    d3d_TermRenderWorld(void* pWorld);                  // 0x1003559e
void    d3d_FreeLightmapPages();                            // 0x1001f860
void    d3d_EndOptimized2D();                               // 0x1001d335
void    d3d_OnSurfacesRestored();                           // 0x10020640
void    d3d_UnoptimizeAll();                                // 0x10020450

void    sub_1002377f();
void    sub_10018ff0();                                     // d3d_draw.cpp
void    sub_10018f90();                                     // d3d_draw.cpp
void    sub_10030120();
void    sub_10029400();
void    sub_100293d0();

// The dynamic lightmap texture allocator (d3d_optimizedsurface.cpp's).
class CLMAllocator
{
public:
	virtual ~CLMAllocator();
};

class CD3DLMAllocator : public CLMAllocator
{
public:
	CD3DLMAllocator();                                      // 0x1001e4ef

	void*       m_pHead;                                    // +0x04
};

CLMAllocator* d3d_GetLMAllocator();                         // 0x100331d1 (lmdynamic.cpp)

// The world the engine hands the renderer.
struct RenderWorld
{
	uint8       m_Reserved00[0x08];
	void*       m_pWorldData;                               // +0x08
	uint16      m_wFlags;                                   // +0x0c
	uint16      m_Pad0e;
};


// The engine's RenderStruct, as far as this file reaches into it.
typedef void* HLTPARAM;

struct RenderStruct
{
	uint8       m_Reserved00[0x14];
	void      (*ConsolePrint)(char* pMsg, ...);             // +0x14
	HLTPARAM  (*GetParameter)(char* pName);                 // +0x18
	float     (*GetParameterValueFloat)(HLTPARAM hParam);   // +0x1c
	char*     (*GetParameterValueString)(HLTPARAM hParam);  // +0x20
};

extern RenderStruct* g_pStruct;                             // 0x1005df08 (common_stuff.cpp)

struct RenderStructInit
{
	int         m_RendererVersion;                          // +0x000
	RMode       m_Mode;                                     // +0x004
	void*       m_hWnd;                                     // +0x218
};

void    d3d_FreeDevice();                                   // 0x10011800 (common_init.cpp)


// The device.
LPDIRECT3DDEVICE7   g_pD3DDevice;                           // 0x100638c8
LPDIRECT3D7         g_pD3D;                                 // 0x100638cc
LPDIRECTDRAWCLIPPER g_pClipper;                             // 0x100638d0

// What the card detection found.
LTBOOL              g_bCanDoShadows;                        // 0x100622a8
LTBOOL              g_bNoFullbrites;                        // 0x100622ac
LTBOOL              g_bRage128;                             // 0x100638d4
LTBOOL              g_100638c4;                             // 0x100638c4
LTBOOL              g_bIn3D;                                // 0x100638d8
LTBOOL              g_bLightmapPages;                       // 0x100638b8

// The z-buffer formats, as the device enumerates them.
#define MAX_ZFORMATS    64
DDPIXELFORMAT       g_ZBufferFormat;                        // 0x10062868
uint32*             g_pnZFormats;                           // 0x10062434
DDPIXELFORMAT*      g_pZFormats;                            // 0x100638bc
D3DDEVICEDESC7      g_DeviceDesc;                           // 0x100623c0

// The render states as they were last set.
LTBOOL      g_bFogStatesSet;                                // 0x1005df80
LTBOOL      g_bDitherStateSet;                              // 0x1005df84
LTBOOL      g_bFilterStatesSet;                             // 0x1005df88
int32       g_LastFogEnable;                                // 0x1005d560
int32       g_LastFogR, g_LastFogG, g_LastFogB;             // 0x1005dadc..
float       g_LastFogNearZ;                                 // 0x1005db68
float       g_LastFogFarZ;                                  // 0x1005d380
int32       g_LastTableFog;                                 // 0x1005dd78
int32       g_LastDither;                                   // 0x1005d970
int32       g_LastBilinear;                                 // 0x1005d484

struct FogColor
{
	uint8   r, g, b;
};
FogColor    g_FogColor;                                     // 0x1005dad8
float       g_FogColorR, g_FogColorG, g_FogColorB;          // 0x1005d478..


// 0x1001a720 -- a DirectDraw/Direct3D error's description.
char* d3d_GetErrorString(HRESULT hResult)
{
	switch (hResult)
	{
	case DD_OK:
		return "No error.";
	case D3DERR_LIGHT_SET_FAILED:
		return "D3DERR_LIGHT_SET_FAILED";
	case D3DERR_MATERIAL_SETDATA_FAILED:
		return "D3DERR_MATERIAL_SETDATA_FAILED";
	case D3DERR_MATERIAL_DESTROY_FAILED:
		return "D3DERR_MATERIAL_DESTROY_FAILED";
	case D3DERR_MATERIAL_CREATE_FAILED:
		return "D3DERR_MATERIAL_CREATE_FAILED";
	case D3DERR_SETVIEWPORTDATA_FAILED:
		return "D3DERR_SETVIEWPORTDATA_FAILED";
	case D3DERR_MATRIX_SETDATA_FAILED:
		return "D3DERR_MATRIX_SETDATA_FAILED";
	case D3DERR_MATRIX_DESTROY_FAILED:
		return "D3DERR_MATRIX_DESTROY_FAILED";
	case D3DERR_MATRIX_CREATE_FAILED:
		return "D3DERR_MATRIX_CREATE_FAILED";
	case D3DERR_TEXTURE_LOAD_FAILED:
		return "D3DERR_TEXTURE_LOAD_FAILED";
	case D3DERR_TEXTURE_UNLOCK_FAILED:
		return "D3DERR_TEXTURE_UNLOCK_FAILED";
	case D3DERR_TEXTURE_LOCKED:
		return "D3DERR_TEXTURELOCKED";
	case D3DERR_TEXTURE_NOT_LOCKED:
		return "D3DERR_TEXTURE_NOT_LOCKED";
	case D3DERR_TEXTURE_LOCK_FAILED:
		return "D3DERR_TEXTURE_LOCK_FAILED";
	case D3DERR_TEXTURE_DESTROY_FAILED:
		return "D3DERR_TEXTURE_DESTROY_FAILED";
	case D3DERR_TEXTURE_CREATE_FAILED:
		return "D3DERR_TEXTURE_CREATE_FAILED";
	case D3DERR_TEXTURE_NO_SUPPORT:
		return "D3DERR_TEXTURE_NO_SUPPORT";
	case D3DERR_EXECUTE_CLIPPED_FAILED:
		return "D3DERR_EXECUTE_CLIPPED_FAILED";
	case D3DERR_EXECUTE_FAILED:
		return "D3DERR_EXECUTE_FAILED";
	case D3DERR_EXECUTE_UNLOCK_FAILED:
		return "D3DERR_EXECUTE_UNLOCK_FAILED";
	case D3DERR_EXECUTE_LOCK_FAILED:
		return "D3DERR_EXECUTE_LOCK_FAILED";
	case D3DERR_EXECUTE_DESTROY_FAILED:
		return "D3DERR_EXECUTE_DESTROY_FAILED";
	case D3DERR_EXECUTE_CREATE_FAILED:
		return "D3DERR_EXECUTE_CREATE_FAILED";
	case D3DERR_EXECUTE_NOT_LOCKED:
		return "D3DERR_EXECUTE_NOT_LOCKED";
	case D3DERR_EXECUTE_LOCKED:
		return "D3DERR_EXECUTE_LOCKED";
	case D3DERR_BADMINORVERSION:
		return "D3DERR_BADMINORVERSION";
	case D3DERR_BADMAJORVERSION:
		return "D3DERR_BADMAJORVERSION";
	case DDERR_NOTPALETTIZED:
		return "The surface being used is not a palette-based surface.";
	case DDERR_IMPLICITLYCREATED:
		return "This surface can not be restored because it is an implicitly created surface.";
	case DDERR_XALIGN:
		return "Rectangle provided was not horizontally aligned on required boundary.";
	case DDERR_WRONGMODE:
		return "This surface can not be restored because it was created in a different mode.";
	case DDERR_WASSTILLDRAWING:
		return "Informs DirectDraw that the previous Blt which is transfering information to or from this Surface is incomplete.";
	case DDERR_REGIONTOOSMALL:
		return "Region passed to Clipper::GetClipList is too small.";
	case DDERR_PRIMARYSURFACEALREADYEXISTS:
		return "This process already has created a primary surface.";
	case DDERR_OVERLAYNOTVISIBLE:
		return "Returned when GetOverlayPosition is called on a hidden overlay.";
	case DDERR_NOTLOCKED:
		return "Surface was not locked.  An attempt to unlock a surface that was not locked at all, or by this process, has been attempted.";
	case DDERR_NOTFLIPPABLE:
		return "An attempt has been made to flip a surface that is not flippable.";
	case DDERR_NOTAOVERLAYSURFACE:
		return "Returned when an overlay member is called for a non-overlay surface.";
	case DDERR_NOPALETTEHW:
		return "No hardware support for 16 or 256 color palettes.";
	case DDERR_NOPALETTEATTACHED:
		return "No palette object attached to this surface.";
	case DDERR_NOOVERLAYDEST:
		return "Returned when GetOverlayPosition is called on an overlay that UpdateOverlay has never been called on to establish a destination.";
	case DDERR_NOHWND:
		return "Clipper notification requires an HWND or no HWND has previously been set as the CooperativeLevel HWND.";
	case DDERR_NOEMULATION:
		return "Software emulation not available.";
	case DDERR_NODIRECTDRAWHW:
		return "A hardware-only DirectDraw object creation was attempted but the driver did not support any hardware.";
	case DDERR_NODDROPSHW:
		return "No DirectDraw ROP hardware.";
	case DDERR_NODC:
		return "No DC was ever created for this surface.";
	case DDERR_NOCLIPPERATTACHED:
		return "No clipper object attached to surface object.";
	case DDERR_NOBLTHW:
		return "No blitter hardware present.";
	case DDERR_INVALIDPOSITION:
		return "Returned when the position of the overlay on the destination is no longer legal for that destination.";
	case DDERR_INVALIDDIRECTDRAWGUID:
		return "The GUID passed to DirectDrawCreate is not a valid DirectDraw driver identifier.";
	case DDERR_HWNDSUBCLASSED:
		return "HWND used by DirectDraw CooperativeLevel has been subclassed, this prevents DirectDraw from restoring state.";
	case DDERR_HWNDALREADYSET:
		return "The CooperativeLevel HWND has already been set. It can not be reset while the process has surfaces or palettes created.";
	case DDERR_EXCLUSIVEMODEALREADYSET:
		return "An attempt was made to set the cooperative level when it was already set to exclusive.";
	case DDERR_DIRECTDRAWALREADYCREATED:
		return "A DirectDraw object representing this driver has already been created for this process.";
	case DDERR_CLIPPERISUSINGHWND:
		return "An attempt was made to set a cliplist for a clipper object that is already monitoring an hwnd.";
	case DDERR_CANTDUPLICATE:
		return "Can't duplicate primary & 3D surfaces, or surfaces that are implicitly created.";
	case DDERR_CANTCREATEDC:
		return "Windows can not create any more DCs.";
	case DDERR_BLTFASTCANTCLIP:
		return "Return if a clipper object is attached to the source surface passed into a BltFast call.";
	case DDERR_VERTICALBLANKINPROGRESS:
		return "Vertical blank is in progress.";
	case DDERR_UNSUPPORTEDMASK:
		return "Bitmask in the pixel format requested is unsupported by DirectDraw.";
	case DDERR_UNSUPPORTEDFORMAT:
		return "FOURCC format requested is unsupported by DirectDraw.";
	case DDERR_TOOBIGWIDTH:
		return "Width requested by DirectDraw is too large.";
	case DDERR_TOOBIGSIZE:
		return "Size requested by DirectDraw is too large, but the individual height and width are OK.";
	case DDERR_TOOBIGHEIGHT:
		return "Height requested by DirectDraw is too large.";
	case DDERR_SURFACENOTATTACHED:
		return "The requested surface is not attached.";
	case DDERR_SURFACELOST:
		return "Access to this surface is being refused because the surface memory is gone. The DirectDrawSurface object representing this surface should have Restore called on it.";
	case DDERR_SURFACEISOBSCURED:
		return "Access to surface refused because the surface is obscured.";
	case DDERR_SURFACEBUSY:
		return "Access to this surface is being refused because the surface is already locked by another thread.";
	case DDERR_SURFACEALREADYDEPENDENT:
		return "This surface is already a dependency of the surface it is being made a dependency of.";
	case DDERR_SURFACEALREADYATTACHED:
		return "This surface is already attached to the surface it is being attached to.";
	case DDERR_PALETTEBUSY:
		return "Access to this palette is being refused because the palette is already locked by another thread.";
	case DDERR_COLORKEYNOTSET:
		return "No src color key specified for this operation.";
	case DDERR_OVERLAYCOLORKEYONLYONEACTIVE:
		return "Can only have ony color key active at one time for overlays.";
	case DDERR_OVERLAYCANTCLIP:
		return "The hardware does not support clipped overlays.";
	case DDERR_OUTOFCAPS:
		return "The hardware needed for the requested operation has already been allocated.";
	case DDERR_NOZOVERLAYHW:
		return "Overlay surfaces could not be z layered based on their BltOrder because the hardware does not support z layering of overlays.";
	case DDERR_NOZBUFFERHW:
		return "Operation could not be carried out because there is no hardware support for zbuffer blitting.";
	case DDERR_NOVSYNCHW:
		return "Operation could not be carried out because there is no hardware support for vertical blank synchronized operations.";
	case DDERR_NOTEXTUREHW:
		return "Operation could not be carried out because there is no texture mapping hardware present or available.";
	case DDERR_NOT8BITCOLOR:
		return "DirectDrawSurface is not in 8 bit color mode and the requested operation requires 8 bit color.";
	case DDERR_NOT4BITCOLORINDEX:
		return "DirectDrawSurface is not in 4 bit color index palette and the requested operation requires 4 bit color index palette.";
	case DDERR_NOT4BITCOLOR:
		return "DirectDrawSurface is not in 4 bit color palette and the requested operation requires 4 bit color palette.";
	case DDERR_NOSTRETCHHW:
		return "Operation could not be carried out because there is no hardware support for stretching.";
	case DDERR_NOROTATIONHW:
		return "Operation could not be carried out because there is no rotation hardware present or available.";
	case DDERR_NORASTEROPHW:
		return "Operation could not be carried out because there is no appropriate raster op hardware present or available.";
	case DDERR_NOOVERLAYHW:
		return "Operation could not be carried out because there is no overlay hardware present or available.";
	case DDERR_NOTFOUND:
		return "Requested item was not found.";
	case DDERR_NOMIRRORHW:
		return "Operation could not be carried out because there is no hardware present or available.";
	case DDERR_NOGDI:
		return "There is no GDI present.";
	case DDERR_NOFLIPHW:
		return "Flipping visible surfaces is not supported.";
	case DDERR_NOEXCLUSIVEMODE:
		return "Operation requires the application to have exclusive mode but the application does not have exclusive mode.";
	case DDERR_NOCOOPERATIVELEVELSET:
		return "Create function called without DirectDraw object method SetCooperativeLevel being called.";
	case DDERR_NOCOLORKEYHW:
		return "Operation could not be carried out because there is no hardware support of the destination color key.";
	case DDERR_NOCOLORKEY:
		return "Surface doesn't currently have a color key";
	case DDERR_NOCOLORCONVHW:
		return "Operation could not be carried out because there is no color conversion hardware present or available.";
	case DDERR_NOCLIPLIST:
		return "No cliplist available.";
	case DDERR_NOALPHAHW:
		return "Operation could not be carried out because there is no alpha accleration hardware present or available.";
	case DDERR_NO3D:
		return "There is no 3D present.";
	case DDERR_LOCKEDSURFACES:
		return "Operation could not be carried out because one or more surfaces are locked.";
	case DDERR_INVALIDRECT:
		return "Rectangle provided was invalid.";
	case DDERR_INVALIDPIXELFORMAT:
		return "The pixel format was invalid as specified.";
	case DDERR_INVALIDOBJECT:
		return "DirectDraw received a pointer that was an invalid DIRECTDRAW object.";
	case DDERR_INVALIDMODE:
		return "DirectDraw does not support the requested mode.";
	case DDERR_INVALIDCLIPLIST:
		return "DirectDraw does not support the provided cliplist.";
	case DDERR_INVALIDCAPS:
		return "One or more of the caps bits passed to the callback are incorrect.";
	case DDERR_INCOMPATIBLEPRIMARY:
		return "Unable to match primary surface creation request with existing primary surface.";
	case DDERR_HEIGHTALIGN:
		return "Height of rectangle provided is not a multiple of reqd alignment.";
	case DDERR_EXCEPTION:
		return "An exception was encountered while performing the requested operation.";
	case DDERR_CURRENTLYNOTAVAIL:
		return "Support is currently not available.";
	case DDERR_ALREADYINITIALIZED:
		return "This object is already initialized.";
	case DDERR_CANNOTATTACHSURFACE:
		return "This surface can not be attached to the requested surface.";
	case DDERR_CANNOTDETACHSURFACE:
		return "This surface can not be detached from the requested surface.";
	case DDERR_INVALIDPARAMS:
		return "One or more of the parameters passed to the function are incorrect.";
	case DDERR_UNSUPPORTED:
		return "Action not supported.";
	case DDERR_GENERIC:
		return "Generic failure.";
	case DDERR_OUTOFMEMORY:
		return "DirectDraw does not have enough memory to perform the operation.";
	case DDERR_OUTOFVIDEOMEMORY:
		return "DirectDraw does not have enough memory to perform the operation.";
	default:
		return "Unrecognized error value.";
	}
}


CD3DConVar g_CV_UseD3DClip("UseD3DClip", 0.0f, LTNULL, LTNULL);      // 0x100622b0
CD3DConVar g_CV_MipMapBias("MipMapBias", 0.0f, LTNULL, LTNULL);      // 0x10062280
CD3DConVar g_CV_Trilinear("Trilinear", 0.0f, LTNULL, LTNULL);        // 0x10062890
CD3DConVar g_CV_Anisotropic("Anisotropic", 0.0f, LTNULL, LTNULL);    // 0x10062440


// The fog render states, when the console variables have changed them.
static void d3d_SetFogStates()
{
	if (!g_pD3DDevice)
		return;

	// No table fog without the hardware, nor with an empty range.
	if (!(g_DeviceDesc.dpcTriCaps.dwRasterCaps & D3DPRASTERCAPS_FOGTABLE))
		g_CV_FogEnable = 0;

	if (g_CV_FogNearZ == g_CV_FogFarZ)
		g_CV_FogEnable = 0;

	if (g_bFogStatesSet &&
		g_LastFogEnable == g_CV_FogEnable &&
		g_LastFogR == g_CV_FogR &&
		g_LastFogG == g_CV_FogG &&
		g_LastFogB == g_CV_FogB &&
		fabs(g_LastFogNearZ - g_CV_FogNearZ) <= 0.001f &&
		fabs(g_LastFogFarZ - g_CV_FogFarZ) <= 0.001f &&
		g_LastTableFog == g_CV_TableFogVar.m_nValue)
	{
		return;
	}

	g_FogColor.r = (uint8)LTCLAMP(g_CV_FogR, 0, 255);
	g_FogColor.g = (uint8)LTCLAMP(g_CV_FogG, 0, 255);
	g_FogColor.b = (uint8)LTCLAMP(g_CV_FogB, 0, 255);

	if (g_CV_TableFogVar.m_nValue)
		g_pD3DDevice->SetRenderState(D3DRENDERSTATE_FOGTABLEMODE, D3DFOG_LINEAR);
	else
		g_pD3DDevice->SetRenderState(D3DRENDERSTATE_FOGTABLEMODE, D3DFOG_NONE);

	g_LastTableFog = g_CV_TableFogVar.m_nValue;

	g_pD3DDevice->SetRenderState(D3DRENDERSTATE_FOGCOLOR,
		RGB_MAKE(g_FogColor.r, g_FogColor.g, g_FogColor.b));
	g_pD3DDevice->SetRenderState(D3DRENDERSTATE_FOGTABLESTART, *(DWORD*)&g_CV_FogNearZ);
	g_pD3DDevice->SetRenderState(D3DRENDERSTATE_FOGTABLEEND, *(DWORD*)&g_CV_FogFarZ);
	g_pD3DDevice->SetRenderState(D3DRENDERSTATE_FOGENABLE, g_CV_FogEnable);

	g_FogColorR = (float)((int)g_FogColor.r - 128) * 2.0f;
	g_FogColorG = (float)((int)g_FogColor.g - 128) * 2.0f;
	g_FogColorB = (float)((int)g_FogColor.b - 128) * 2.0f;

	g_LastFogEnable = g_CV_FogEnable;
	g_LastFogR = g_CV_FogR;
	g_LastFogG = g_CV_FogG;
	g_LastFogB = g_CV_FogB;
	g_LastFogNearZ = g_CV_FogNearZ;
	g_LastFogFarZ = g_CV_FogFarZ;
	g_bFogStatesSet = LTTRUE;
}

// The dither render state, when it has changed.
static void d3d_SetDitherState()
{
	if (g_bDitherStateSet && g_LastDither == g_CV_Dither)
		return;

	if (!g_pD3DDevice)
		return;

	g_pD3DDevice->SetRenderState(D3DRENDERSTATE_DITHERENABLE, g_CV_Dither);
	g_LastDither = g_CV_Dither;
	g_bDitherStateSet = LTTRUE;
}

// The texture filters on both stages.
static void d3d_SetFilterStates()
{
	if (!g_pD3DDevice)
		return;

	if (g_CV_Anisotropic.m_nValue)
	{
		g_pD3DDevice->SetTextureStageState(0, D3DTSS_MINFILTER, D3DTFN_ANISOTROPIC);
		g_pD3DDevice->SetTextureStageState(0, D3DTSS_MAGFILTER, D3DTFN_ANISOTROPIC);
		g_pD3DDevice->SetTextureStageState(0, D3DTSS_MAXANISOTROPY, g_CV_Anisotropic.m_nValue);
		g_pD3DDevice->SetTextureStageState(1, D3DTSS_MINFILTER, D3DTFN_ANISOTROPIC);
		g_pD3DDevice->SetTextureStageState(1, D3DTSS_MAGFILTER, D3DTFN_ANISOTROPIC);
		g_pD3DDevice->SetTextureStageState(1, D3DTSS_MAXANISOTROPY, g_CV_Anisotropic.m_nValue);
	}
	else
	{
		g_pD3DDevice->SetTextureStageState(0, D3DTSS_MINFILTER, g_CV_Bilinear ? D3DTFN_LINEAR : D3DTFN_POINT);
		g_pD3DDevice->SetTextureStageState(0, D3DTSS_MAGFILTER, g_CV_Bilinear ? D3DTFG_LINEAR : D3DTFG_POINT);
		g_pD3DDevice->SetTextureStageState(1, D3DTSS_MINFILTER, g_CV_Bilinear ? D3DTFN_LINEAR : D3DTFN_POINT);
		g_pD3DDevice->SetTextureStageState(1, D3DTSS_MAGFILTER, g_CV_Bilinear ? D3DTFG_LINEAR : D3DTFG_POINT);
	}

	g_pD3DDevice->SetTextureStageState(0, D3DTSS_MIPFILTER, g_CV_Trilinear.m_nValue ? D3DTFP_LINEAR : D3DTFP_POINT);
	g_pD3DDevice->SetTextureStageState(0, D3DTSS_MIPMAPLODBIAS, *(DWORD*)&g_CV_MipMapBias.m_fValue);
	g_pD3DDevice->SetTextureStageState(1, D3DTSS_MIPFILTER, g_CV_Trilinear.m_nValue ? D3DTFP_LINEAR : D3DTFP_POINT);
	g_pD3DDevice->SetTextureStageState(1, D3DTSS_MIPMAPLODBIAS, *(DWORD*)&g_CV_MipMapBias.m_fValue);

	g_bFilterStatesSet = LTTRUE;
	g_LastBilinear = g_CV_Bilinear;
}

// 0x1001b010 -- the render states the console variables control.
void d3d_ReadExtraConsoleVariables()
{
	d3d_SetFogStates();
	d3d_SetDitherState();
	d3d_SetFilterStates();
}


// 0x1001b460 -- whether the z-buffer format has a stencil.
LTBOOL d3d_HasStencil()
{
	return g_ZBufferFormat.dwStencilBitDepth > 0;
}

// 0x1001b470 -- the z-buffer formats' enumeration callback.
HRESULT CALLBACK d3d_EnumZBufferFormatsCallback(DDPIXELFORMAT* pFormat, void* pContext)
{
	if (*g_pnZFormats < MAX_ZFORMATS)
	{
		g_pZFormats[*g_pnZFormats] = *pFormat;
		(*g_pnZFormats)++;
	}

	return D3DENUMRET_OK;
}


// 0x1001b4c0 -- turn off what the card cannot do.  The ForceMode game
// variable can name the card instead.
void d3d_DetectCard()
{
	DDDEVICEIDENTIFIER2 id;
	HLTPARAM hParam;

	if (!g_pDirectDraw || !g_pStruct)
		return;

	memset(&id, 0, sizeof(id));
	g_bCanDoShadows = LTTRUE;
	g_bNoFullbrites = LTFALSE;

	if (g_pDirectDraw->GetDeviceIdentifier(&id, 0) != DD_OK)
		return;

	hParam = g_pStruct->GetParameter("ForceMode");
	if (hParam && stricmp(g_pStruct->GetParameterValueString(hParam), "PowerVR") == 0)
		goto PowerVR;

	hParam = g_pStruct->GetParameter("ForceMode");
	if (hParam && stricmp(g_pStruct->GetParameterValueString(hParam), "Permedia2") == 0)
		goto Permedia2;

	hParam = g_pStruct->GetParameter("ForceMode");
	if (hParam && stricmp(g_pStruct->GetParameterValueString(hParam), "Rage128") == 0)
		goto Rage128;

	if (id.dwVendorId == 0x104C)
	{
Permedia2:
		g_pStruct->ConsolePrint("PERMEDIA 2 DETECTED: disabling lightmapping and shadows");
		g_bLightmapPages = LTFALSE;
		g_bCanDoShadows = LTFALSE;
	}
	else if (id.dwVendorId == 0x1033)
	{
		if (id.dwDeviceId == 0x46 || id.dwDeviceId == 0x2A)
		{
PowerVR:
			g_pStruct->ConsolePrint("POWERVR DETECTED: disabling shadows and fullbrites");
			g_bCanDoShadows = LTFALSE;
			g_bNoFullbrites = LTTRUE;
		}
	}
	else if (id.dwVendorId == 0x1002)
	{
		if (id.dwDeviceId == 0x5246)
		{
Rage128:
			g_bRage128 = LTTRUE;
		}
	}
	else if (id.dwVendorId == 0x121A)
	{
		if (id.dwDeviceId == 5)
			g_100638c4 = LTFALSE;
	}
}


// 0x1001b680 -- make the z-buffer and the device on it: the formats with a
// stencil first, each tried until one works.
LTBOOL d3d_CreateZBufferAndDevice(RenderStructInit* pInit, GUID guid)
{
	uint32 bTried[MAX_ZFORMATS];
	uint32 nZFormats;
	DDPIXELFORMAT zFormats[MAX_ZFORMATS];
	DDPIXELFORMAT* pFormat;
	DDSURFACEDESC2 ddsd;
	uint32 i, iBest;

	memset(bTried, 0, sizeof(bTried));

	nZFormats = 0;
	g_pZFormats = zFormats;
	g_pnZFormats = &nZFormats;

	if (g_pD3D->EnumZBufferFormats(guid, d3d_EnumZBufferFormatsCallback, LTNULL) != D3D_OK || !nZFormats)
	{
		AddDebugMessage(1, "Unable to find an acceptable z-buffer format.");
		return LTFALSE;
	}

	while (nZFormats > 0)
	{
		iBest = (uint32)-1;
		for (i = 0; i < nZFormats; i++)
		{
			if (!bTried[i])
			{
				if (iBest == (uint32)-1)
					iBest = i;
				else if (zFormats[iBest].dwStencilBitMask == 0 && zFormats[i].dwStencilBitMask != 0)
					iBest = i;
			}
		}

		if (iBest >= MAX_ZFORMATS)
			return LTFALSE;

		pFormat = &zFormats[iBest];

		memset(&ddsd, 0, sizeof(ddsd));
		ddsd.dwSize = sizeof(ddsd);
		ddsd.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT;
		ddsd.dwWidth = pInit->m_Mode.m_Width;
		ddsd.dwHeight = pInit->m_Mode.m_Height;
		ddsd.ddsCaps.dwCaps = g_1005d2c0 | DDSCAPS_ZBUFFER;
		ddsd.ddpfPixelFormat = *pFormat;

		if (g_pDirectDraw->CreateSurface(&ddsd, &g_pZBuffer, LTNULL) != DD_OK)
		{
			AddDebugMessage(1, "Failed to make z-buffer.");
			d3d_FreeDevice();
			return LTFALSE;
		}

		if (FAILED(g_pBackBuffer->AddAttachedSurface(g_pZBuffer)))
		{
			g_pZBuffer->Release();
			return LTFALSE;
		}

		if (SUCCEEDED(g_pD3D->CreateDevice(guid, g_pBackBuffer, &g_pD3DDevice)))
		{
			g_ZBufferFormat = *pFormat;
			AddDebugMessage(1, "ZBuffer: Z Mask: %d, Stencil mask: %d",
				pFormat->dwRGBZBitMask, pFormat->dwStencilBitMask);
			return LTTRUE;
		}

		g_pBackBuffer->DeleteAttachedSurface(0, g_pZBuffer);
		g_pZBuffer->Release();
		g_pZBuffer = LTNULL;

		bTried[iBest] = LTTRUE;
	}

	return LTFALSE;
}


// 0x1001c300 -- the attached back buffer's enumeration callback.
HRESULT WINAPI d3d_EnumAttachedCallback(LPDIRECTDRAWSURFACE7 pSurface,
	LPDDSURFACEDESC2 pDesc, LPVOID pContext)
{
	g_pBackBuffer = pSurface;
	return DDENUMRET_CANCEL;
}

// 0x1001c310
void* d3d_CreateWorld(void** pWorldData)
{
	RenderWorld* pWorld;

	pWorld = (RenderWorld*)dalloc_z(sizeof(RenderWorld));
	if (!pWorld)
		return LTNULL;

	pWorld->m_pWorldData = *pWorldData;
	pWorld->m_wFlags = 0xFFFF;

	if (g_CV_NoLMPages)
	{
		dd_ConsolePrint("Warning: NoLMPages is TRUE");
		return pWorld;
	}

	if (g_bLightmapPages)
	{
		if (!d3d_CreateLightmapPages(pWorld))
		{
			AddDebugMessage(0, "Warning: unable to create lightmap pages.  Lightmapping disabled.");
			g_bLightmapPages = LTFALSE;
		}
	}

	return pWorld;
}

// 0x1001c380
void d3d_DestroyWorld(void* pWorld)
{
	if (pWorld)
	{
		d3d_TermRenderWorld(pWorld);
		dfree(pWorld);
	}
}

// 0x1001c3a0 -- rebuild the world's lightmap pages.
void d3d_RebindWorld(void* pWorld)
{
	if (pWorld)
		d3d_TermRenderWorld(pWorld);

	if (g_bLightmapPages)
	{
		d3d_FreeLightmapPages();

		if (pWorld)
		{
			if (!d3d_CreateLightmapPages(pWorld))
			{
				AddDebugMessage(0, "Warning: unable to create lightmap pages.  Lightmapping disabled.");
				g_bLightmapPages = LTFALSE;
			}
		}
	}
}

// 0x1001c3f0
void d3d_ReleaseDevice()
{
	d3d_UnoptimizeAll();

	if (g_pClipper)
	{
		g_pClipper->Release();
		g_pClipper = LTNULL;
	}

	if (g_pD3DDevice)
	{
		g_pD3DDevice->Release();
		g_pD3DDevice = LTNULL;
	}

	if (g_pZBuffer)
	{
		g_pZBuffer->Release();
		g_pZBuffer = LTNULL;
	}

	if (g_pD3D)
	{
		g_pD3D->Release();
		g_pD3D = LTNULL;
	}
}

// 0x1001c450
void d3d_TermSubsystems()
{
	sub_1002377f();
	sub_10018ff0();
	sub_10030120();
	sub_10029400();

	delete d3d_GetLMAllocator();
}

// 0x1001c480
LTBOOL d3d_InitSubsystems()
{
	sub_10018f90();
	sub_10030120();
	sub_100293d0();

	new CD3DLMAllocator;

	return LTTRUE;
}

// 0x1001c980
uint32 d3d_GetInfoFlags()
{
	return 0;
}

// 0x1001c990
LTBOOL d3d_Start3D()
{
	if (g_bIn3D || !g_pD3DDevice)
		return LTFALSE;

	if (g_pD3DDevice->BeginScene() != D3D_OK)
		return LTFALSE;

	g_bIn3D = LTTRUE;
	return LTTRUE;
}

// 0x1001c9c0
LTBOOL d3d_End3D()
{
	if (!g_bIn3D || !g_pD3DDevice)
		return LTFALSE;

	if (g_bInOptimized2D)
		d3d_EndOptimized2D();

	g_bIn3D = LTFALSE;

	if (g_pD3DDevice->EndScene() == DDERR_SURFACELOST)
	{
		if (g_pPrimarySurface->Restore() >= 0)
		{
			g_pBackBuffer->Restore();
			g_pZBuffer->Restore();
			d3d_OnSurfacesRestored();
		}
	}

	return LTTRUE;
}

// 0x1001ca30
LTBOOL d3d_IsIn3D()
{
	return g_bIn3D;
}
