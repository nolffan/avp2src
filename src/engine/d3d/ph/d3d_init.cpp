// *********************************************************************** //
//
//  MODULE:   D3D_INIT (.CPP) -- Primal Hunt's d3d.ren, TU17
//            (0x1001a720..0x1001cb13, docs/D3D_OBJECTS.md)
//
//  PURPOSE:  Device start-up: the DirectDraw/Direct3D error strings, the
//            device's console variables, card detection, the z-buffer and
//            the device creation, the world create/destroy and 3D-frame
//            entry points and the renderer's console commands.
//
//  The device bring-up (d3d_InitDevice), the console commands
//  (d3d_RenderCommand, LISTDEVICECAPS inline) and the gamma ramp loader
//  (0x1001ca40, after the TU's last function: /O2 code padded to 16)
//  follow AvP2 1.0.9.6's (w38_100322c0_qip.cpp, w26_10031e10.cpp),
//  Primal Hunt's evolution of them.  The fog, dither and filter states'
//  "last set" globals are common_stuff.cpp's (TU12's .bss), so this file
//  reads them as externs.
//
//  Flags: /O2 (so /Ob1), not /Ob2: under /Ob2 the gamma ramp loader is
//  expanded into both its callers, which retail calls.  The render-state
//  helpers of d3d_ReadExtraConsoleVariables are `inline`.
//
//  Near: 0x1001b4c0 (the card detection): every instruction matches but
//  the blocks after the vendor tests are laid out differently -- retail's
//  shared epilogue follows the Rage128 block, with the 0x121A test and the
//  Permedia block (each with its own `ret`) after it; ours ends on the
//  Permedia block.  No if/else or goto arrangement tried reproduces it.
//
// *********************************************************************** //

#include <windows.h>
#include <ddraw.h>
#include <d3d.h>
#include <math.h>
#include <stdio.h>
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
extern uint32               g_ScreenWidth;                  // 0x1005d8cc (common_init.cpp)
extern uint32               g_ScreenHeight;                 // 0x1005d9c0

// The screen's pixel format (pixelformat.cpp's PFormat).
class PFormat
{
public:
	void*       m_pVTable;
	uint32      m_eType;                                    // +0x04
};

#define BPP_16  2

extern PFormat              g_ScreenFormat;                 // 0x1005d260 (common_init.cpp)

LTBOOL  d3d_CreateLightmapPages(void* pWorld);              // 0x10035389
void    d3d_TermRenderWorld(void* pWorld);                  // 0x1003559e
void    d3d_ResetLightmapPages();                           // 0x1001f860 (d3d_texture.cpp)
void    d3d_EndOptimized2D();                               // 0x1001d335
void    d3d_FreeAllTextures();                              // 0x10020640 (d3d_texture.cpp)
void    d3d_TermTextures();                                 // 0x10020450 (d3d_texture.cpp)

// The subsystems' start-up and shut-down.  Two of them are empty and the
// link folded them into other empty functions (/OPT:ICF), so retail
// calls those: 0x1002377f (draw_canvas.cpp) and 0x10030120
// (drawworldmodel.cpp).
void    d3d_CanvasPreFrame();                               // 0x1002377f (draw_canvas.cpp)
void    d3d_NullPreFrame();                                 // 0x10030120 (drawworldmodel.cpp)
void    d3d_InitDraw();                                     // 0x10018f90 (d3d_draw.cpp)
void    d3d_TermDraw();                                     // 0x10018ff0 (d3d_draw.cpp)
void    d3d_InitObjectModules();                            // 0x100293d0 (drawobjects.cpp)
void    d3d_TermObjectModules();                            // 0x10029400 (drawobjects.cpp)

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

// What the device can do, read when it is created (d3d_InitDevice).
struct D3DCaps
{
	LTBOOL          m_bSrcBlendSrcColor;                    // +0x000 0x100622d8
	LTBOOL          m_bDestBlendSrcAlpha;                   // +0x004
	LTBOOL          m_bTBlendAdd;                           // +0x008
	LTBOOL          m_bTBlendModulate;                      // +0x00c
	LTBOOL          m_bDither;                              // +0x010
	LTBOOL          m_bDrawPrim;                            // +0x014
	LTBOOL          m_bVidMemTextures;                      // +0x018
	LTBOOL          m_bSysMemTextures;                      // +0x01c
	LTBOOL          m_bAGPTextures;                         // +0x020
	int32           m_nZBufferBits;                         // +0x024
	int32           m_nDeviceBits;                          // +0x028
	uint32          m_dwTextureMem;                         // +0x02c
	uint32          m_dwVideoMem;                           // +0x030
	LTBOOL          m_bSurfacesLargerThanScreen;            // +0x034
	D3DDEVICEDESC7  m_Desc;                                 // +0x038
	D3DPRIMCAPS     m_TriCaps;                              // +0x124
};

D3DCaps             g_D3DCaps;                              // 0x100622d8
uint32              g_ZBufferState;                         // 0x10062438

LTBOOL              g_bFullbriteCapable;                    // 0x100622a0
LTBOOL              g_bLightAddPolyCapable;                 // 0x10062860
LTBOOL              g_bFogCapable;                          // 0x10062888

// The render states as they were last set (common_stuff.cpp's .bss).
extern LTBOOL   g_bFogStatesSet;                            // 0x1005df80
extern LTBOOL   g_bDitherStateSet;                          // 0x1005df84
extern LTBOOL   g_bFilterStatesSet;                         // 0x1005df88
extern int32    g_LastFogEnable;                            // 0x1005d560
extern int32    g_LastFogR, g_LastFogG, g_LastFogB;         // 0x1005dadc..
extern float    g_LastFogNearZ;                             // 0x1005db68
extern float    g_LastFogFarZ;                              // 0x1005d380
extern int32    g_LastTableFog;                             // 0x1005dd78
extern int32    g_LastDither;                               // 0x1005d970
extern int32    g_LastBilinear;                             // 0x1005d484

struct FogColor
{
	uint8   r, g, b;
};
extern FogColor g_FogColor;                                 // 0x1005dad8
extern float    g_FogColorR, g_FogColorG, g_FogColorB;      // 0x1005d478..


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
inline void d3d_SetFogStates()
{
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
inline void d3d_SetDitherState()
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
inline void d3d_SetFilterStates()
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
	if (!g_pD3DDevice)
		return;

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

	g_bCanDoShadows = LTTRUE;
	g_bNoFullbrites = LTFALSE;
	memset(&id, 0, sizeof(id));

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

	if (id.dwVendorId != 0x104C)
	{
		if (id.dwVendorId == 0x1033)
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
	else
	{
Permedia2:
		g_pStruct->ConsolePrint("PERMEDIA 2 DETECTED: disabling lightmapping and shadows");
		g_bLightmapPages = LTFALSE;
		g_bCanDoShadows = LTFALSE;
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


// The rest of the device start-up (the other files').
struct D3DDeviceInfo
{
	uint8       m_Reserved00[0x20];
	GUID*       m_pDriverGUID;                              // +0x20
};

extern LTBOOL               g_bRunWindowed;                 // 0x1005d538 (common_init.cpp)
extern int32                g_1005d4a8;                     // 0x1005d4a8 (common_stuff.cpp: blit, not flip)
extern IDirectDrawSurface7* g_pFrontBuffer;                 // 0x1005d2ac (common_init.cpp)
extern int32                g_CV_TripleBuffer;              // 0x1005df10 (common_stuff.cpp)
extern int32                g_CV_Force1Pass;                // 0x1005df18
extern int32                g_CV_RGBRast;                   // 0x1005df20
extern int32                g_CV_RefRast;                   // 0x1005df24
extern int32                g_CV_MMXRast;                   // 0x1005df28
extern int32                g_CV_TnLRast;                   // 0x1005df2c
extern LTBOOL               g_bMultiPass;                   // 0x10062278

void    d3d_SetDefaultBlend();                              // 0x100143a0 (d3d_draw.cpp)
void    d3d_SetEnvMapStage(uint32* pSaved);                 // 0x10025397 (drawmodel_env.cpp)
void    d3d_UnsetEnvMapStage(uint32* pSaved);               // 0x1002541b
void    d3d_SetDetailTextureOp();                           // 0x1000a1ea (drawworld.cpp)
void    d3d_UnsetDetailTextureOp();                         // 0x1000a20e
void    d3d_SetLightmapStages();                            // 0x10036498 (drawlightmaps.cpp)
LTBOOL  d3d_InitTextures();                                 // 0x1001f930 (d3d_texture.cpp)
HRESULT WINAPI d3d_EnumAttachedCallback(LPDIRECTDRAWSURFACE7 pSurface,
	LPDDSURFACEDESC2 pDesc, LPVOID pContext);               // 0x1001c300

LTBOOL  d3d_LoadGammaRamp(const char* pFilename);           // 0x1001ca40

LTBOOL              g_bCanValidateEnvMap;                   // 0x100622a4
uint32              g_TextureStage;                         // 0x100622d0
uint32              g_iCurTextureStage;                     // 0x100638c0
float               g_fLightmapOffset;                      // 0x100638b4

// A DDBD_ depth as bits.
#define DDBD_TO_BITS(dst, depth) \
	if ((depth) == DDBD_8) \
		(dst) = 8; \
	else if ((depth) == DDBD_16) \
		(dst) = 16; \
	else if ((depth) == DDBD_24) \
		(dst) = 24; \
	else \
		(dst) = ((depth) == DDBD_32) ? 32 : -1;

// What the device and DirectDraw can do, into g_D3DCaps.
inline void d3d_ReadDeviceCaps(LPDIRECT3DDEVICE7 pDevice)
{
	DDCAPS drvCaps, helCaps;
	D3DDEVICEDESC7 desc;
	DDSCAPS2 ddsCaps;
	DWORD dwTexTotal, dwTexFree;
	DWORD dwVidTotal, dwVidFree;

	memset(&helCaps, 0, sizeof(helCaps));
	memset(&drvCaps, 0, sizeof(drvCaps));
	drvCaps.dwSize = sizeof(drvCaps);
	helCaps.dwSize = sizeof(helCaps);
	g_pDirectDraw->GetCaps(&drvCaps, &helCaps);

	g_D3DCaps.m_bSurfacesLargerThanScreen = (drvCaps.dwCaps2 & DDCAPS2_WIDESURFACES) != 0;

	memset(&desc, 0, sizeof(desc));
	pDevice->GetCaps(&desc);

	memset(&ddsCaps, 0, sizeof(ddsCaps));
	ddsCaps.dwCaps = DDSCAPS_TEXTURE;
	g_pDirectDraw->GetAvailableVidMem(&ddsCaps, &dwTexTotal, &dwTexFree);

	memset(&ddsCaps, 0, sizeof(ddsCaps));
	ddsCaps.dwCaps = DDSCAPS_VIDEOMEMORY;
	g_pDirectDraw->GetAvailableVidMem(&ddsCaps, &dwVidTotal, &dwVidFree);

	g_D3DCaps.m_bDrawPrim = (desc.dwDevCaps & D3DDEVCAPS_DRAWPRIMTLVERTEX) != 0;
	g_D3DCaps.m_bVidMemTextures = (desc.dwDevCaps & D3DDEVCAPS_TEXTUREVIDEOMEMORY) != 0;
	g_D3DCaps.m_bSysMemTextures = (desc.dwDevCaps & D3DDEVCAPS_TEXTURESYSTEMMEMORY) != 0;
	g_D3DCaps.m_bAGPTextures = (desc.dwDevCaps & D3DDEVCAPS_TEXTURENONLOCALVIDMEM) != 0;

	DDBD_TO_BITS(g_D3DCaps.m_nZBufferBits, desc.dwDeviceZBufferBitDepth)
	DDBD_TO_BITS(g_D3DCaps.m_nDeviceBits, desc.dwDeviceRenderBitDepth)

	g_D3DCaps.m_bDither = (desc.dpcTriCaps.dwRasterCaps & D3DPRASTERCAPS_DITHER) != 0;
	g_D3DCaps.m_bSrcBlendSrcColor = (desc.dpcTriCaps.dwSrcBlendCaps & D3DPBLENDCAPS_SRCCOLOR) != 0;
	g_D3DCaps.m_bDestBlendSrcAlpha = (desc.dpcTriCaps.dwDestBlendCaps & D3DPBLENDCAPS_SRCALPHA) != 0;
	g_D3DCaps.m_bTBlendAdd = (desc.dpcTriCaps.dwTextureBlendCaps & D3DPTBLENDCAPS_ADD) != 0;
	g_D3DCaps.m_bTBlendModulate = (desc.dpcTriCaps.dwTextureBlendCaps & D3DPTBLENDCAPS_MODULATE) != 0;

	g_D3DCaps.m_dwTextureMem = dwTexTotal;
	g_D3DCaps.m_dwVideoMem = dwVidTotal;

	g_ZBufferState = D3DZB_TRUE;

	g_D3DCaps.m_Desc = desc;
	g_D3DCaps.m_TriCaps = desc.dpcTriCaps;
}

// Can the device add the environment map, and modulate the detail
// texture, in one pass?
inline void d3d_TestMultiTexture()
{
	uint32 savedStates[2];
	DWORD nPasses;

	d3d_SetEnvMapStage(savedStates);
	g_bCanValidateEnvMap = g_pD3DDevice->ValidateDevice(&nPasses) == D3D_OK;
	d3d_UnsetEnvMapStage(savedStates);

	d3d_SetDetailTextureOp();
	g_100638c4 = g_pD3DDevice->ValidateDevice(&nPasses) == D3D_OK;
	if (g_D3DCaps.m_Desc.wMaxSimultaneousTextures < 2)
		g_100638c4 = LTFALSE;
	d3d_UnsetDetailTextureOp();
}

// 0x1001b8d0 -- bring DirectDraw and the Direct3D device up on a driver:
// the rasterizer, the cooperative level and mode, the surfaces, the
// device, the viewport, what the device can do and its default states.
LTBOOL d3d_InitDevice(D3DDeviceInfo* pDevice, RenderStructInit* pInit)
{
	GUID deviceGuid;
	DWORD flags;
	HLTPARAM hParam;
	DDSURFACEDESC2 ddsd;
	DDBLTFX bltFx;
	D3DVIEWPORT7 viewport;
	DWORD nLightmapPasses;
	int i;

	ShowCursor(FALSE);

	if (g_CV_RefRast)
	{
		g_pStruct->ConsolePrint("USING DIRECT3D REFERENCE RASTERIZER (SLOOOOOOOW!)");
		g_1005d2c0 = DDSCAPS_SYSTEMMEMORY;
		deviceGuid = IID_IDirect3DRefDevice;
	}
	else if (g_CV_RGBRast)
	{
		g_1005d2c0 = DDSCAPS_SYSTEMMEMORY;
		deviceGuid = IID_IDirect3DRGBDevice;
	}
	else if (g_CV_MMXRast)
	{
		g_1005d2c0 = DDSCAPS_SYSTEMMEMORY;
		deviceGuid = IID_IDirect3DMMXDevice;
	}
	else if (g_CV_TnLRast)
	{
		deviceGuid = IID_IDirect3DTnLHalDevice;
	}
	else
	{
		deviceGuid = IID_IDirect3DHALDevice;
	}

	g_pDirectDraw = LTNULL;
	if (DirectDrawCreateEx(pDevice->m_pDriverGUID, (void**)&g_pDirectDraw, IID_IDirectDraw7, LTNULL) != DD_OK)
	{
		AddDebugMessage(1, "DirectDrawCreateEx failed.");
		d3d_FreeDevice();
		return LTFALSE;
	}

	flags = g_bRunWindowed ? DDSCL_NORMAL : (DDSCL_EXCLUSIVE | DDSCL_ALLOWREBOOT | DDSCL_FULLSCREEN);
	hParam = g_pStruct->GetParameter("NullRender");
	if (hParam && g_pStruct->GetParameterValueFloat(hParam) != 0.0f)
		flags |= DDSCL_NOWINDOWCHANGES;

	if (g_pDirectDraw->SetCooperativeLevel((HWND)pInit->m_hWnd, flags) != DD_OK)
	{
		AddDebugMessage(1, "SetCooperativeLevel failed.");
		d3d_FreeDevice();
		return LTFALSE;
	}

	if (!g_bRunWindowed)
	{
		if (g_pDirectDraw->SetDisplayMode(pInit->m_Mode.m_Width, pInit->m_Mode.m_Height, pInit->m_Mode.m_BitDepth, 0, 0) != DD_OK)
		{
			AddDebugMessage(1, "SetDisplayMode failed.");
			d3d_FreeDevice();
			return LTFALSE;
		}
	}

	if (g_pDirectDraw->QueryInterface(IID_IDirect3D7, (void**)&g_pD3D) != DD_OK)
	{
		AddDebugMessage(1, "QueryInterface(IID_IDirect3D7) failed.. are you running NT 4.0?");
		d3d_FreeDevice();
		return LTFALSE;
	}

	if (g_bRunWindowed || g_1005d4a8)
	{
		// A plain primary, a separate 3D render surface and a clipper.
		memset(&ddsd, 0, sizeof(ddsd));
		ddsd.dwSize = sizeof(ddsd);
		ddsd.dwFlags = DDSD_CAPS;
		ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
		if (g_pDirectDraw->CreateSurface(&ddsd, &g_pFrontBuffer, LTNULL) != DD_OK)
		{
			AddDebugMessage(1, "Unable to create a primary surface.");
			d3d_FreeDevice();
			return LTFALSE;
		}

		memset(&ddsd, 0, sizeof(ddsd));
		ddsd.dwSize = sizeof(ddsd);
		ddsd.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH;
		ddsd.ddsCaps.dwCaps = g_1005d2c0 | DDSCAPS_3DDEVICE;
		ddsd.dwBackBufferCount = 0;
		ddsd.dwWidth = pInit->m_Mode.m_Width;
		ddsd.dwHeight = pInit->m_Mode.m_Height;
		if (g_pDirectDraw->CreateSurface(&ddsd, &g_pPrimarySurface, LTNULL) != DD_OK)
		{
			AddDebugMessage(1, "Failed to make rendering surface.");
			d3d_FreeDevice();
			return LTFALSE;
		}

		g_pBackBuffer = g_pPrimarySurface;

		if (g_pDirectDraw->CreateClipper(0, &g_pClipper, LTNULL) == DD_OK)
		{
			g_pClipper->SetHWnd(0, (HWND)pInit->m_hWnd);
			g_pFrontBuffer->SetClipper(g_pClipper);
		}
	}
	else
	{
		// A flipping chain.
		memset(&ddsd, 0, sizeof(ddsd));
		ddsd.dwSize = sizeof(ddsd);
		ddsd.dwFlags = DDSD_CAPS | DDSD_BACKBUFFERCOUNT;
		ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE | DDSCAPS_FLIP | DDSCAPS_COMPLEX | DDSCAPS_3DDEVICE;
		ddsd.dwBackBufferCount = g_CV_TripleBuffer ? 2 : 1;
		if (g_pDirectDraw->CreateSurface(&ddsd, &g_pPrimarySurface, LTNULL) != DD_OK)
		{
			if (ddsd.dwBackBufferCount == 2)
			{
				AddDebugMessage(0, "Unable to use triple buffering.");
				ddsd.dwBackBufferCount = 1;
				if (g_pDirectDraw->CreateSurface(&ddsd, &g_pPrimarySurface, LTNULL) != DD_OK)
				{
					AddDebugMessage(1, "Unable to create a primary surface.");
					d3d_FreeDevice();
					return LTFALSE;
				}
			}
			else
			{
				AddDebugMessage(1, "Unable to create a primary surface.");
				d3d_FreeDevice();
				return LTFALSE;
			}
		}

		g_pBackBuffer = LTNULL;
		g_pPrimarySurface->EnumAttachedSurfaces(LTNULL, d3d_EnumAttachedCallback);
		if (!g_pBackBuffer)
		{
			AddDebugMessage(1, "Couldn't get pointer to offscreen surface.");
			d3d_FreeDevice();
			return LTFALSE;
		}
	}

	// Clear the chain.
	memset(&bltFx, 0, sizeof(bltFx));
	bltFx.dwSize = sizeof(bltFx);
	for (i = 0; i < 4; i++)
	{
		g_pBackBuffer->Blt(LTNULL, LTNULL, LTNULL, DDBLT_COLORFILL | DDBLT_WAIT, &bltFx);
		g_pPrimarySurface->Flip(LTNULL, DDFLIP_WAIT);
	}

	if (!d3d_CreateZBufferAndDevice(pInit, deviceGuid))
	{
		AddDebugMessage(1, "Failed to create device.");
		d3d_FreeDevice();
		g_CV_TnLRast = 0;
		return LTFALSE;
	}

	memset(&viewport, 0, sizeof(viewport));
	viewport.dwX = 0;
	viewport.dwY = 0;
	viewport.dwWidth = pInit->m_Mode.m_Width;
	viewport.dwHeight = pInit->m_Mode.m_Height;
	viewport.dvMinZ = 0.0f;
	viewport.dvMaxZ = 1.0f;
	if (g_pD3DDevice->SetViewport(&viewport) != D3D_OK)
	{
		AddDebugMessage(1, "IDirect3DDevice::SetCurrentViewport failed.");
		d3d_FreeDevice();
		return LTFALSE;
	}

	d3d_ReadDeviceCaps(g_pD3DDevice);

	// The default states.
	g_pD3DDevice->SetTextureStageState(0, D3DTSS_TEXCOORDINDEX, 0);
	g_pD3DDevice->SetTextureStageState(1, D3DTSS_TEXCOORDINDEX, 1);
	g_pD3DDevice->SetRenderState(D3DRENDERSTATE_FOGTABLEMODE, D3DFOG_LINEAR);
	g_pD3DDevice->SetRenderState(D3DRENDERSTATE_FOGENABLE, FALSE);
	g_pD3DDevice->SetRenderState(D3DRENDERSTATE_ANTIALIAS, D3DANTIALIAS_NONE);
	g_pD3DDevice->SetRenderState(D3DRENDERSTATE_CULLMODE, D3DCULL_NONE);
	g_pD3DDevice->SetRenderState(D3DRENDERSTATE_COLORKEYENABLE, FALSE);
	g_pD3DDevice->SetRenderState(D3DRENDERSTATE_ZENABLE, g_ZBufferState);
	g_pD3DDevice->SetRenderState(D3DRENDERSTATE_ZWRITEENABLE, TRUE);
	g_pD3DDevice->SetRenderState(D3DRENDERSTATE_ZFUNC, D3DCMP_LESSEQUAL);
	g_pD3DDevice->SetRenderState(D3DRENDERSTATE_SPECULARENABLE, FALSE);
	g_pD3DDevice->SetRenderState(D3DRENDERSTATE_SHADEMODE, D3DSHADE_GOURAUD);
	g_pD3DDevice->SetRenderState(D3DRENDERSTATE_TEXTUREPERSPECTIVE, TRUE);
	g_pD3DDevice->SetRenderState(D3DRENDERSTATE_DITHERENABLE, TRUE);
	d3d_SetDefaultBlend();
	g_pD3DDevice->SetRenderState(D3DRENDERSTATE_FILLMODE, D3DFILL_SOLID);
	g_pD3DDevice->SetRenderState(D3DRENDERSTATE_CLIPPING, g_CV_UseD3DClip.m_nValue != 0);
	g_pD3DDevice->SetRenderState(D3DRENDERSTATE_LIGHTING, FALSE);
	g_pD3DDevice->SetRenderState(D3DRENDERSTATE_EXTENTS, FALSE);
	g_pD3DDevice->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
	g_pD3DDevice->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
	g_pD3DDevice->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_CURRENT);

	d3d_TestMultiTexture();

	g_bFullbriteCapable = !(g_D3DCaps.m_TriCaps.dwRasterCaps & D3DPRASTERCAPS_ZBUFFERLESSHSR);
	g_bFogCapable = g_bFullbriteCapable && (g_D3DCaps.m_TriCaps.dwSrcBlendCaps & D3DPBLENDCAPS_SRCALPHA);
	g_bLightmapPages = (g_D3DCaps.m_TriCaps.dwSrcBlendCaps & D3DPBLENDCAPS_ZERO) &&
		(g_D3DCaps.m_TriCaps.dwDestBlendCaps & D3DPBLENDCAPS_SRCCOLOR);
	g_bLightAddPolyCapable = (g_D3DCaps.m_TriCaps.dwSrcBlendCaps & D3DPBLENDCAPS_ONE) &&
		(g_D3DCaps.m_TriCaps.dwDestBlendCaps & D3DPBLENDCAPS_ONE);

	// One or two passes for the lightmaps.
	g_TextureStage = 0;
	g_iCurTextureStage = 0;
	g_bMultiPass = g_CV_Force1Pass;
	if (g_bMultiPass)
	{
		d3d_SetLightmapStages();
		if (g_pD3DDevice->ValidateDevice(&nLightmapPasses) == D3D_OK &&
			g_D3DCaps.m_Desc.wMaxSimultaneousTextures > 1)
		{
			g_TextureStage = 1;
			AddDebugMessage(0, "Using 1 pass lightmapping!");
		}
		else
		{
			g_bMultiPass = LTFALSE;
			AddDebugMessage(0, "Force1Pass doesn't work on this card.  Using 2 pass lightmapping.");
		}
	}
	else
	{
		AddDebugMessage(0, "Using 2 pass lightmapping.");
	}

	d3d_DetectCard();
	d3d_LoadGammaRamp("lithtech.gam");

	if (!d3d_InitTextures())
	{
		d3d_FreeDevice();
		return LTFALSE;
	}

	g_fLightmapOffset = 0.5f;
	return LTTRUE;
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
		d3d_ResetLightmapPages();

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
	d3d_TermTextures();

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
	d3d_CanvasPreFrame();
	d3d_TermDraw();
	d3d_NullPreFrame();
	d3d_TermObjectModules();

	delete d3d_GetLMAllocator();
}

// 0x1001c480
LTBOOL d3d_InitSubsystems()
{
	d3d_InitDraw();
	d3d_NullPreFrame();
	d3d_InitObjectModules();

	new CD3DLMAllocator;

	return LTTRUE;
}

// The console commands' handlers in the other files.
void    d3d_ListDevices();                                  // 0x1001172f (common_init.cpp)
void    d3d_ListTextureFormats();                           // 0x10020690 (d3d_texture.cpp)
void    d3d_LoadPortalFile(const char* pFilename);          // 0x100442c3
void    d3d_UnloadPortalFile();                             // 0x1002377f (empty: folded)

#define YESNO(x)    ((x) ? "Yes" : "No")

// LISTDEVICECAPS: the driver and what the device can do.
inline void d3d_ListDeviceCaps()
{
	DDDEVICEIDENTIFIER2 id;

	if (g_pDirectDraw)
	{
		memset(&id, 0, sizeof(id));
		if (g_pDirectDraw->GetDeviceIdentifier(&id, 0) == DD_OK)
		{
			g_pStruct->ConsolePrint("---------------------------------------------------------------");
			g_pStruct->ConsolePrint("Driver: %s", id.szDriver);
			g_pStruct->ConsolePrint("Description: %s", id.szDescription);
			g_pStruct->ConsolePrint("Product: 0x%x, Version: 0x%x, SubVersion: 0x%x, Build: 0x%x",
				HIWORD(id.liDriverVersion.HighPart), LOWORD(id.liDriverVersion.HighPart),
				HIWORD(id.liDriverVersion.LowPart), LOWORD(id.liDriverVersion.LowPart));
			g_pStruct->ConsolePrint("VendorID: 0x%x, DeviceID: 0x%x, SubSysID: 0x%x, Revision: 0x%x",
				id.dwVendorId, id.dwDeviceId, id.dwSubSysId, id.dwRevision);
			g_pStruct->ConsolePrint("Width: %d, Height: %d, BitDepth: %d",
				g_ScreenWidth, g_ScreenHeight, g_ScreenFormat.m_eType == BPP_16 ? 16 : 32);
			g_pStruct->ConsolePrint("---------------------------------------------------------------");
		}
	}

	g_pStruct->ConsolePrint("Portals: %s", YESNO(g_ZBufferFormat.dwStencilBitDepth > 0));
	g_pStruct->ConsolePrint("Model fullbrites: %s, Gouraud fullbrites: %s",
		YESNO(g_bFullbriteCapable), YESNO(g_bFogCapable));
	g_pStruct->ConsolePrint("Lightmap capable: %s, Light add poly: %s",
		YESNO(g_bLightmapPages), YESNO(g_bLightAddPolyCapable));
	g_pStruct->ConsolePrint("DrawPrim: %s, Dither: %s",
		YESNO(g_D3DCaps.m_bDrawPrim), YESNO(g_D3DCaps.m_bDither));
	g_pStruct->ConsolePrint("Src blend SRCCOLOR: %s, Dest blend SRCALPHA: %s",
		YESNO(g_D3DCaps.m_bSrcBlendSrcColor), YESNO(g_D3DCaps.m_bDestBlendSrcAlpha));
	g_pStruct->ConsolePrint("TBlend Add: %s, TBlend Modulate (gouraud): %s",
		YESNO(g_D3DCaps.m_bTBlendAdd), YESNO(g_D3DCaps.m_bTBlendModulate));
	g_pStruct->ConsolePrint("LINEARMIPNEAREST supported: %s",
		YESNO(g_D3DCaps.m_TriCaps.dwTextureFilterCaps & D3DPTFILTERCAPS_LINEARMIPNEAREST));
	g_pStruct->ConsolePrint("Square textures only: %s, Vid mem textures: %s",
		YESNO(g_D3DCaps.m_TriCaps.dwTextureCaps & D3DPTEXTURECAPS_SQUAREONLY), YESNO(g_D3DCaps.m_bVidMemTextures));
	g_pStruct->ConsolePrint("System mem textures: %s, AGP mem textures: %s",
		YESNO(g_D3DCaps.m_bSysMemTextures), YESNO(g_D3DCaps.m_bAGPTextures));
	g_pStruct->ConsolePrint("Texture Memory: %d, Video Memory: %d",
		g_D3DCaps.m_dwTextureMem, g_D3DCaps.m_dwVideoMem);
	g_pStruct->ConsolePrint("ZBuffer Depth: %d, Device Depth: %d",
		g_D3DCaps.m_nZBufferBits, g_D3DCaps.m_nDeviceBits);
	g_pStruct->ConsolePrint("Z-test: %s, Table fog: %s, Palette alpha: %s",
		YESNO(g_D3DCaps.m_TriCaps.dwRasterCaps & D3DPRASTERCAPS_ZTEST),
		YESNO(g_D3DCaps.m_TriCaps.dwRasterCaps & D3DPRASTERCAPS_FOGTABLE),
		YESNO(g_D3DCaps.m_TriCaps.dwTextureCaps & D3DPTEXTURECAPS_ALPHAPALETTE));
	g_pStruct->ConsolePrint("Max texture size: (%d x %d)",
		g_D3DCaps.m_Desc.dwMaxTextureWidth, g_D3DCaps.m_Desc.dwMaxTextureHeight);
	g_pStruct->ConsolePrint("Surfaces larger than screen: %s",
		YESNO(g_D3DCaps.m_bSurfacesLargerThanScreen));
}

// 0x1001c4b0 -- the renderer's console commands.
void d3d_RenderCommand(int argc, char** argv)
{
	if (argc <= 0)
		return;

	if (stricmp(argv[0], "LISTDEVICES") == 0)
	{
		d3d_ListDevices();
	}
	else if (stricmp(argv[0], "LISTTEXTUREFORMATS") == 0)
	{
		d3d_ListTextureFormats();
	}
	else if (stricmp(argv[0], "LISTDEVICECAPS") == 0)
	{
		d3d_ListDeviceCaps();
	}
	else if (stricmp(argv[0], "FREETEXTURES") == 0)
	{
		d3d_FreeAllTextures();
	}
	else if (stricmp(argv[0], "LOADGAMMA") == 0)
	{
		if (argc != 2)
			g_pStruct->ConsolePrint("Usage: loadgamma <filename>");
		else
			d3d_LoadGammaRamp(argv[1]);
	}
	else if (stricmp(argv[0], "PortalFile") == 0)
	{
		if (argc >= 2)
			d3d_LoadPortalFile(argv[1]);
		else
			d3d_UnloadPortalFile();
	}
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
			d3d_FreeAllTextures();
		}
	}

	return LTTRUE;
}

// 0x1001ca30
LTBOOL d3d_IsIn3D()
{
	return g_bIn3D;
}

// 0x1001ca40 -- LOADGAMMA: set the primary surface's gamma ramp from a
// file (the three 256-entry tables of a DDGAMMARAMP, as written).
LTBOOL d3d_LoadGammaRamp(const char* pFilename)
{
	DDGAMMARAMP ramp;
	LPDIRECTDRAWGAMMACONTROL pGamma;
	FILE* fp;

	if (!pFilename)
		return LTFALSE;

	fp = fopen(pFilename, "r");
	if (!fp)
		return LTFALSE;

	if (fread(&ramp, sizeof(ramp), 1, fp) != 1)
	{
		fclose(fp);
		return LTFALSE;
	}

	fclose(fp);

	if (g_bRunWindowed)
	{
		if (g_pFrontBuffer->QueryInterface(IID_IDirectDrawGammaControl, (void**)&pGamma) != DD_OK)
			return LTFALSE;
	}
	else
	{
		if (g_pPrimarySurface->QueryInterface(IID_IDirectDrawGammaControl, (void**)&pGamma) != DD_OK)
			return LTFALSE;
	}

	return pGamma->SetGammaRamp(0, &ramp) == DD_OK;
}
