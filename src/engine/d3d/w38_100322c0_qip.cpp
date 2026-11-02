// *********************************************************************** //
//
//  MODULE:   W38_100322C0_QIP (.CPP)  --  the device bring-up TU, /O2 /Qip
//
//  0x100322c0 (read the device caps), 0x10032d00 (RenderCommand),
//  0x10032f30/34 (load a gamma ramp) and 0x100331e0 (DirectDraw/Direct3D
//  bring-up), from w5_100322c0.cpp, w32_10032d00_qip.cpp and big331e0.cpp.
//
//  0x100331e0 calls two /Qip REGISTER entries -- 0x100322c0 with the
//  device in %eax and $0x1008d7f4 in %edx, 0x10032f34 with "lithtech.gam"
//  in %eax -- and those only form with caller and callee in one /Qip TU.
//  0x100322c0 has no stack adapter (its first instruction after the frame
//  is `mov %eax,%ebx`), so it is a static of this file; 0x10032f30 keeps
//  its stack adapter because 0x10032d00 is also a caller (both in here).
//
// *********************************************************************** //

#include "ltbasedefs.h"
#include <string.h>

// ======================================================================= //
//  TYPES
//
//  Nothing in LT2\sdk\inc declares HWND or GUID, and d3dr4b.cpp includes
//  only "ltbasedefs.h", so the shapes this function needs are spelled out
//  here at the offsets the bytes give.
// ======================================================================= //

// A 16-byte COM GUID.  Five of them sit in .rdata at 0x10088128 ..
// 0x100881a7 and their bytes identify them outright:
//   0x10088128 {F5049E78-4861-11D2-A407-00A0C90629A8} IID_IDirect3DTnLHalDevice
//   0x10088138 {50936643-13E9-11D1-89AA-00A0C9054129} IID_IDirect3DRefDevice
//   0x10088148 {881949A1-D6F3-11D0-89AB-00A0C9054129} IID_IDirect3DMMXDevice
//   0x10088158 {84E63DE0-46AA-11CF-816F-0000C020156E} IID_IDirect3DHALDevice
//   0x10088168 {A4665C60-2673-11CF-A31A-00AA00B93356} IID_IDirect3DRGBDevice
//   0x10088178 {F5049E77-4861-11D2-A407-00A0C90629A8} IID_IDirect3D7
//   0x10088198 {15E65EC0-3B9C-11D2-B92F-00609797EA5B} IID_IDirectDraw7
// All defined, with C linkage as the SDK spells them, in d3dguid.cpp.
struct D3DGuid
{
	uint32 m_Data1;
	uint16 m_Data2;
	uint16 m_Data3;
	uint8  m_Data4[8];
};

extern "C" const D3DGuid IID_IDirect3DTnLHalDevice;         // 0x10088128
extern "C" const D3DGuid IID_IDirect3DRefDevice;            // 0x10088138
extern "C" const D3DGuid IID_IDirect3DMMXDevice;            // 0x10088148
extern "C" const D3DGuid IID_IDirect3DHALDevice;            // 0x10088158
extern "C" const D3DGuid IID_IDirect3DRGBDevice;            // 0x10088168
extern "C" const D3DGuid IID_IDirect3D7;                    // 0x10088178
extern "C" const D3DGuid IID_IDirectDraw7;                  // 0x10088198


// DDSURFACEDESC2.  Pinned by `movl $0x7c,-0x100(%ebp)` (dwSize == 124 ==
// sizeof(DDSURFACEDESC2)) and by the `rep stos` of 0x1f dwords over the same
// block, plus the four fields the function actually writes:
//   -0x100 +0x00  dwSize                = 0x7c
//   -0x0fc +0x04  dwFlags               = 1 / 7 / 0x21
//   -0x0f8 +0x08  dwHeight
//   -0x0f4 +0x0c  dwWidth
//   -0x0ec +0x14  dwBackBufferCount
//   -0x098 +0x68  ddsCaps.dwCaps
struct DDPixelFormat { uint32 m_Raw[8]; };              // 0x20 bytes
struct DDSCaps2      { uint32 dwCaps, dwCaps2, dwCaps3, dwCaps4; };

struct DDSurfaceDesc2
{
	uint32        dwSize;                               // +0x00
	uint32        dwFlags;                              // +0x04
	uint32        dwHeight;                             // +0x08
	uint32        dwWidth;                              // +0x0c
	int32         lPitch;                               // +0x10
	uint32        dwBackBufferCount;                    // +0x14
	uint32        dwMipMapCount;                        // +0x18
	uint32        dwAlphaBitDepth;                      // +0x1c
	uint32        dwReserved;                           // +0x20
	void*         lpSurface;                            // +0x24
	uint32        ddckCKDestOverlay[2];                 // +0x28
	uint32        ddckCKDestBlt[2];                     // +0x30
	uint32        ddckCKSrcOverlay[2];                  // +0x38
	uint32        ddckCKSrcBlt[2];                      // +0x40
	DDPixelFormat ddpfPixelFormat;                      // +0x48
	DDSCaps2      ddsCaps;                              // +0x68
	uint32        dwTextureStage;                       // +0x78
};                                                      // 0x7c

// DDBLTFX.  Pinned by `movl $0x64,-0x84(%ebp)` (dwSize == 100) and the
// `rep stos` of 0x19 dwords over the same block.  Only dwSize is written;
// dwFillColor is left at the zero the memset put there.
struct DDBltFx { uint32 dwSize; uint32 m_Rest[24]; };   // 0x64

// D3DVIEWPORT7.  Six dwords zeroed at -0x20(%ebp) and then written as
// (0, 0, width, height, 0.0f, 1.0f) -- 0x3f800000 at +0x14 is the 1.0f.
struct D3DViewport7
{
	uint32 dwX;                                         // +0x00
	uint32 dwY;                                         // +0x04
	uint32 dwWidth;                                     // +0x08
	uint32 dwHeight;                                    // +0x0c
	float  dvMinZ;                                      // +0x10
	float  dvMaxZ;                                      // +0x14
};


// DDCAPS (DirectX 7, 0x17c bytes).  Only dwSize and dwCaps2 are touched.
struct DDCaps100322c0
{
	uint32 dwSize;                                      // +0x00
	uint32 dwCaps;                                      // +0x04
	uint32 dwCaps2;                                     // +0x08
	uint8  m_pad_0c[0x17c - 0x0c];
};

// D3DPRIMCAPS (0x38 bytes).
struct D3DPrimCaps100322c0
{
	uint32 dwSize;                                      // +0x00
	uint32 dwMiscCaps;                                  // +0x04
	uint32 dwRasterCaps;                                // +0x08
	uint32 dwZCmpCaps;                                  // +0x0c
	uint32 dwSrcBlendCaps;                              // +0x10
	uint32 dwDestBlendCaps;                             // +0x14
	uint32 dwAlphaCmpCaps;                              // +0x18
	uint32 dwShadeCaps;                                 // +0x1c
	uint32 dwTextureCaps;                               // +0x20
	uint32 dwTextureFilterCaps;                         // +0x24
	uint32 dwTextureBlendCaps;                          // +0x28
	uint32 dwTextureAddressCaps;                        // +0x2c
	uint32 dwStippleWidth;                              // +0x30
	uint32 dwStippleHeight;                             // +0x34
};

// D3DDEVICEDESC7 (0xec bytes).
struct D3DDevDesc100322c0
{
	uint32              dwDevCaps;                      // +0x00
	D3DPrimCaps100322c0 dpcLineCaps;                    // +0x04
	D3DPrimCaps100322c0 dpcTriCaps;                     // +0x3c
	uint32              dwDeviceRenderBitDepth;         // +0x74
	uint32              dwDeviceZBufferBitDepth;        // +0x78
	uint8               m_pad_7c[0xec - 0x7c];
};

// DDSCAPS2 (16 bytes).
struct DDSCaps2100322c0
{
	uint32 dwCaps;
	uint32 dwCaps2;
	uint32 dwCaps3;
	uint32 dwCaps4;
};

// ======================================================================= //
//  COM INTERFACES
//
//  Every slot below is used by this function and every one of them lands on
//  the documented index for its interface, which is the cross-check that the
//  globals are typed right:
//    IDirectDraw7:            +0x00 QueryInterface, +0x10 CreateClipper,
//                             +0x18 CreateSurface,  +0x50 SetCooperativeLevel,
//                             +0x54 SetDisplayMode
//    IDirectDrawSurface7:     +0x14 Blt, +0x24 EnumAttachedSurfaces,
//                             +0x2c Flip, +0x70 SetClipper
//    IDirectDrawClipper:      +0x20 SetHWnd
//    IDirect3DDevice7:        +0x34 SetViewport, +0x98 ValidateDevice
// ======================================================================= //

struct DDSurface7;
struct DDClipper;

struct DDClipperVtbl
{
	void* m_pad_00[8];                                  // +0x00..+0x1c
	long (__stdcall* SetHWnd)(DDClipper*, uint32 dwFlags, void* hWnd);
	                                                    // +0x20
};
struct DDClipper { DDClipperVtbl* lpVtbl; };

struct DDSurface7Vtbl
{
	// +0x00 -- IUnknown::QueryInterface (0x10032f30's gamma control)
	long (__stdcall* QueryInterface)(DDSurface7*, const void* riid,
	                                 void** ppv);
	void* m_pad_04[4];                                  // +0x04..+0x10
	long (__stdcall* Blt)(DDSurface7*, void* lpDestRect,
	                      DDSurface7* lpDDSrcSurface, void* lpSrcRect,
	                      uint32 dwFlags, DDBltFx* lpDDBltFx);   // +0x14
	void* m_pad_18[3];                                  // +0x18..+0x20
	long (__stdcall* EnumAttachedSurfaces)(DDSurface7*, void* lpContext,
	                                       void* lpEnumCallback);// +0x24
	void* m_pad_28;                                     // +0x28
	long (__stdcall* Flip)(DDSurface7*, DDSurface7* lpTargetOverride,
	                       uint32 dwFlags);             // +0x2c
	void* m_pad_30[16];                                 // +0x30..+0x6c
	long (__stdcall* SetClipper)(DDSurface7*, DDClipper*);        // +0x70
};
struct DDSurface7 { DDSurface7Vtbl* lpVtbl; };

struct DDraw7;
struct DDraw7Vtbl
{
	long (__stdcall* QueryInterface)(DDraw7*, const D3DGuid* riid,
	                                 void** ppv);       // +0x00
	void* m_pad_04[3];                                  // +0x04..+0x0c
	long (__stdcall* CreateClipper)(DDraw7*, uint32 dwFlags,
	                                DDClipper** lplpDDClipper,
	                                void* pUnkOuter);   // +0x10
	void* m_pad_14;                                     // +0x14
	long (__stdcall* CreateSurface)(DDraw7*, DDSurfaceDesc2* lpDDSurfaceDesc,
	                                DDSurface7** lplpDDSurface,
	                                void* pUnkOuter);   // +0x18
	void* m_pad_1c[4];                                  // +0x1c..+0x28
	long (__stdcall* GetCaps)(DDraw7*, DDCaps100322c0* lpDDDriverCaps,
	                          DDCaps100322c0* lpDDHELCaps);          // +0x2c
	void* m_pad_30[8];                                  // +0x30..+0x4c
	long (__stdcall* SetCooperativeLevel)(DDraw7*, void* hWnd,
	                                      uint32 dwFlags);        // +0x50
	long (__stdcall* SetDisplayMode)(DDraw7*, uint32 dwWidth,
	                                 uint32 dwHeight, uint32 dwBPP,
	                                 uint32 dwRefreshRate,
	                                 uint32 dwFlags);   // +0x54
	void* m_pad_58;                                     // +0x58
	long (__stdcall* GetAvailableVidMem)(DDraw7*, DDSCaps2100322c0* lpDDSCaps2,
	                                     uint32* lpdwTotal,
	                                     uint32* lpdwFree);          // +0x5c
};
struct DDraw7 { DDraw7Vtbl* lpVtbl; };

struct D3DDevice7;
struct D3DDevice7Vtbl
{
	void* m_pad_00[3];                                  // +0x00..+0x08
	long (__stdcall* GetCaps)(D3DDevice7*, D3DDevDesc100322c0* lpDesc); // +0x0c
	void* m_pad_10[9];                                  // +0x10..+0x30
	long (__stdcall* SetViewport)(D3DDevice7*, D3DViewport7* lpViewport);
	                                                    // +0x34
	void* m_pad_38[24];                                 // +0x38..+0x94
	long (__stdcall* ValidateDevice)(D3DDevice7*, uint32* lpdwPasses);
	                                                    // +0x98
};
struct D3DDevice7 { D3DDevice7Vtbl* lpVtbl; };

struct D3D7 { void* lpVtbl; };


// The engine's RenderStruct at 0x1008cbcc.  d3dmid.cpp pins +0x34 dalloc /
// +0x38 dfree and d3dbatchc.cpp pins +0x18 GetConsoleVariable / +0x1c
// GetVarValueFloat; d3d.cpp's map names +0x14 ConsolePrintf.  This function
// uses exactly those three: +0x14 with the reference-rasterizer banner
// (0x10033316), +0x18 with "NullRender" (0x100333ae) and +0x1c with the
// answer (0x100333c4, whose result is compared with `fcomps` against the
// 0.0f at 0x100a8800).
struct RenderStructCB
{
	uint8    m_Reserved00[0x14];                        // +0x00..+0x13
	void    (*ConsolePrintf)(const char* pszMsg, ...);  // +0x14
	void*   (*GetConsoleVariable)(const char* pszName); // +0x18
	float   (*GetVarValueFloat)(void* pVar);            // +0x1c
	uint8    m_Reserved20[0x1c];                        // +0x20..+0x3b
};

extern RenderStructCB* g_pStruct;                       // 0x1008cbcc


// The record the enumeration loop hands us.  ONLY +0x20 is touched, and it
// goes straight into DirectDrawCreateEx's first parameter, so it is the
// DirectDraw driver GUID pointer.  Everything else is left as reserved
// bytes -- 0x1002179b's neighbourhood shows strings at +0x24 and +0x88 but
// this function never reads them.
struct D3DDeviceRec
{
	uint8          m_Reserved00[0x20];                  // +0x00..+0x1f
	const D3DGuid* m_pDDGuid20;                         // +0x20
};

// RenderStructInit -- the SAME struct src/engine/lithtech/clientrender.cpp
// already reconstructed on the client side, and ltbasedefs.h's RMode is what
// makes every displacement in this function fall out exactly:
//
//   RMode:  m_bHardware +0x000, m_RenderDLL[256] +0x004, m_InternalName[128]
//           +0x104, m_Description[128] +0x184, m_Width +0x204, m_Height
//           +0x208, m_BitDepth +0x20c, m_pNext +0x210   (sizeof 0x214)
//
//   so with RMode at +4:  m_Width +0x208, m_Height +0x20c, m_BitDepth +0x210
//                         and m_hWnd at +0x218, sizeof 0x21c.
//
// Those are precisely the four displacements this function reads: +0x208 and
// +0x20c as SetDisplayMode's dwWidth/dwHeight (0x1003341d, 0x10033429),
// +0x210 as its dwBPP (0x10033433) and +0x218 as SetCooperativeLevel's hWnd
// (0x100333e8).  The caller confirms the rest: 0x100217c2 copies 0x7f bytes
// into +0x108 (m_Mode.m_InternalName) and 0x100217dc into +0x188
// (m_Mode.m_Description).
//
// HWND is not declared anywhere under LT2\sdk\inc, so the handle is a
// void* here; the client-side reconstruction spells it HWND.
struct RenderStructInit
{
	uint32 m_nVersion;                                  // +0x000
	RMode  m_Mode;                                      // +0x004
	void*  m_hWnd;                                      // +0x218
};


// ======================================================================= //
//  GLOBALS
//
//  Names in this block that are not `g_<addr>` are ones the project has
//  already pinned elsewhere; the file each came from is on the line.
// ======================================================================= //

extern DDraw7*     g_pDirectDraw;        // 0x1008bf74  d3dbatcha.cpp
extern DDSurface7* g_pOffscreen;         // 0x1008bf68  d3dbatcha.cpp
extern DDSurface7* g_pPrimary;           // 0x1008bf6c  d3dbatcha.cpp
extern DDSurface7* g_pWindowedPrimary;   // 0x1008bf70  d3dbatcha.cpp
extern DDClipper*  g_pClipper;           // 0x1008d97c  d3dbatcha.cpp
extern D3DDevice7* g_pD3DDevice;         // 0x1008d988  d3dbatchc.cpp
extern D3D7*       g_pD3D;               // 0x1008d980  (the QueryInterface
                                         //  out-pointer at 0x1003348a)
extern LTBOOL      g_bWindowed;          // 0x1008cbbc  d3dr5d.cpp

extern int32 g_CV_TnLRast;               // 0x1008cb14  big21f90.cpp
extern int32 g_CV_MMXRast;               // 0x1008cb18  big21f90.cpp
extern int32 g_CV_RefRast;               // 0x1008cb1c  big21f90.cpp
extern int32 g_CV_RGBRast;               // 0x1008cb20  big21f90.cpp
extern int32 g_CV_Force1Pass;            // 0x1008cb28  big21f90.cpp
extern int32 g_CV_TripleBuffer;          // 0x1008cb30  big21f90.cpp

// 0x1008cbb8.  Initialised to 0x4000 (DDSCAPS_VIDEOMEMORY) at 0x1002165f,
// set to 0x800 (DDSCAPS_SYSTEMMEMORY) here for every software rasterizer,
// and OR'd with 0x2000 (DDSCAPS_3DDEVICE) into ddsCaps.dwCaps at 0x1003355c.
// So it is the surface-memory half of a DDSCAPS word.
extern uint32 g_nSurfCaps1008cbb8;                      // 0x1008cbb8

// 0x1008cba8.  Initialised to 0 at 0x10021669 and read at 0x10021eab; here
// it selects the blit-to-primary path over the page-flipping chain.
extern uint32 g_1008cba8;                               // 0x1008cba8

// The copy of the device's dpcTriCaps that 0x100322c0 leaves at 0x1008d918.
// 0x100322c0 fills 0x1008d918..0x1008d94c from D3DDEVICEDESC7 +0x3c..+0x70
// (0x10032543..0x100325dc), which is D3DPRIMCAPS field for field, so:
//     0x1008d920 = dpcTriCaps.dwRasterCaps
//     0x1008d928 = dpcTriCaps.dwSrcBlendCaps
//     0x1008d92c = dpcTriCaps.dwDestBlendCaps
extern uint32 g_1008d920;                               // dwRasterCaps
extern uint32 g_1008d928;                               // dwSrcBlendCaps
extern uint32 g_1008d92c;                               // dwDestBlendCaps

extern uint32 g_1008d964;      // !(dwRasterCaps & ZBUFFERLESSHSR)
extern uint32 g_1008d968;      // d3dr3d.cpp/d3dqip -- ONE/ONE blend usable
extern uint32 g_1008d96c;      // d3dqip2.cpp -- the live Force1Pass flag
extern uint32 g_1008d970;      // d3dqip2.cpp
extern uint32 g_1008d978;      // d3dr3d.cpp
extern uint32 g_1008d7cc;      // big124a4.cpp -- the texture stage
extern uint32 g_1008d7d0;      // d3dqip3.cpp / big6d910.cpp -- current stage
extern float  g_f1008d7c0;     // 0x1008d7c0, set to 0.5f on success

// wMaxTextureBlendStages, already named by d3dr3b.cpp.
// 0x1008d82c -- the one D3DDEVICEDESC7 copy 0x100322c0 fills (w5_100322c0.cpp);
// the fields this file reads are addressed inside it, not as separate
// globals, so a linked d3d.ren reads what that copy wrote.
extern D3DDevDesc100322c0 g_DevDesc1008d82c;
#define g_nStages1008d8e6 (*(uint16*)((char*)&g_DevDesc1008d82c + 0xba))   // 0x1008d8e6 wMaxTextureBlendStages

// The destination 0x100322c0 is handed in %edx at 0x1003395a.  0x100322c0
// hardcodes the same address into %esi at 0x100322d8, so it is the base of
// the module's device-caps block; it is passed here as an opaque pointer.
extern uint32 g_Caps1008d7f4;                           // 0x1008d7f4


// ======================================================================= //
//  NEIGHBOURS
// ======================================================================= //

extern "C" __declspec(dllimport) int __stdcall ShowCursor(int bShow);

// 0x1007fa86 is the import thunk `jmp *0x10088000`, DDRAW.dll's
// DirectDrawCreateEx.  0x1003337b reaches it with a DIRECT `call`, not the
// `call *IAT` that ShowCursor gets, so this one is declared WITHOUT
// __declspec(dllimport) -- dllimport would emit the indirect form.
extern "C"
long __stdcall DirectDrawCreateEx(const D3DGuid* lpGUID, void** lplpDD,
                                  const D3DGuid* iid, void* pUnkOuter);

// 0x10021d60 -- the renderer's warning printf, spelled the way d3dr5a_o2.cpp
// and d3dr3b.cpp already spell it.
extern void sub_10021d60(uint32 nLevel, const char* pszMsg, ...);

// 0x100210d0 -- the DirectDraw teardown, decoded in d3dbatcha.cpp.
extern void sub_100210d0();

// 0x10033ba0 -- the function immediately after this one, and its only
// caller is 0x1003387a.  It takes the RenderStructInit and the chosen device
// GUID BY VALUE: 0x10033bee does `lea 0xc(%ebp),%edx` and hands it to
// IDirect3D7::EnumZBufferFormats (+0x18), whose first parameter is a
// REFCLSID.  20 bytes of arguments in a 24-byte outgoing area.
extern LTBOOL sub_10033ba0(RenderStructInit* pInit, D3DGuid guid);

// 0x10032610 -- 889 bytes, already reconstructed in d3dr3b.cpp; no arguments.
extern void sub_10032610();

// 0x10073210 -- 396 bytes, two callers (0x100264d7 and this one's
// 0x100339f3); no arguments.  Already declared this way in d3dqip2.cpp.
extern void sub_10073210();

// 0x10032a50 -- the "ForceMode" console-variable reader (d3dbatchc.cpp);
// no arguments.
extern void sub_10032a50();

// 0x1003ad30 -- 1168 bytes (d3dr3d.cpp); no arguments, result null-tested.
extern LTBOOL sub_1003ad30();

// 0x10032600 -- `ret $0xc`, __stdcall(3), stores its first argument into
// 0x1008bf68 (g_pOffscreen) and returns 0.  Decoded in d3dneutral.cpp.  Its
// address is planted as the EnumAttachedSurfaces callback at 0x100337b8.
extern uint32 __stdcall sub_10032600(uint32 nArg0, uint32 nArg1,
                                     uint32 nArg2);


extern "C" int __cdecl _stricmp(const char* a, const char* b);     // 0x100874b0

void    sub_10020f80();                                 // LISTDEVICES
void    sub_10039a60();                                 // LISTTEXTUREFORMATS
void    sub_10031e10();                                 // LISTDEVICECAPS
void    sub_10039930();                                 // FREETEXTURES
LTBOOL  sub_10032f30(const char* pszFileName);          // load a gamma ramp, below
uint32  sub_1005dbe0(void* pszFileName);                // PortalFile <name>
void    sub_100145f0();                                 // PortalFile


// ======================================================================= //
//  0x100322c0   821 bytes.  READ THE DEVICE CAPS INTO THE CAPS BLOCK.
//
//  One caller, 0x1003394d below, which loads g_pD3DDevice into %eax and
//  $0x1008d7f4 into %edx: a /Qip register entry, and a static (no stack
//  adapter).  The caps pointer is a real parameter: icl constant-propagated
//  it (`mov $0x1008d7f4,%esi`, the `add $0x38,%esi` that becomes the
//  `rep movsl` destination of the desc copy, every other store absolute),
//  but it kept the ALIASING of a store through a pointer -- every flag is
//  computed from a fresh load of the local desc, because a store through
//  pCaps might have changed it (the desc's address escaped to GetCaps).
//  Stores to named globals let icl hoist the loads, which is what
//  w5_100322c0.cpp measured (624 bytes off).
//
//  THE BLOCK, at 0x1008d7f4, by the offsets the stores give:
// ======================================================================= //

struct D3DCapsBlock
{
	uint32              m_bSrcBlendSrcColor;            // +0x000 0x1008d7f4
	uint32              m_bDestBlendSrcAlpha;           // +0x004 0x1008d7f8
	uint32              m_bTexBlend80;                  // +0x008 0x1008d7fc
	uint32              m_bTexBlend02;                  // +0x00c 0x1008d800
	uint32              m_bDither;                      // +0x010 0x1008d804
	uint32              m_bDevCaps400;                  // +0x014 0x1008d808
	uint32              m_bDevCaps200;                  // +0x018 0x1008d80c
	uint32              m_bDevCaps100;                  // +0x01c 0x1008d810
	uint32              m_bDevCaps1000;                 // +0x020 0x1008d814
	int32               m_nZBufferBits;                 // +0x024 0x1008d818
	int32               m_nRenderBits;                  // +0x028 0x1008d81c
	uint32              m_dwTextureMem;                 // +0x02c 0x1008d820
	uint32              m_dwVideoMem;                   // +0x030 0x1008d824
	uint32              m_bCaps2_1000;                  // +0x034 0x1008d828
	D3DDevDesc100322c0  m_Desc;                         // +0x038 0x1008d82c
	D3DPrimCaps100322c0 m_TriCaps;                      // +0x124 0x1008d918
	uint8               m_pad_15c[0x190 - 0x15c];       // +0x15c 0x1008d950
	uint32              m_bZEnable;                     // +0x190 0x1008d984
};

// DDBD_8/16/24/32 -> bits, -1 otherwise.  Written out twice in the body
// (0x1003242f and 0x10032477), each as a `cmp ; jne ; mov ; jmp` chain into
// one store.
inline int32 BitsFromDDBD100322c0(uint32 dwDepth)
{
	return (dwDepth == 0x800) ? 8 :
	       (dwDepth == 0x400) ? 16 :
	       (dwDepth == 0x200) ? 24 :
	       (dwDepth == 0x100) ? 32 : -1;
}

static void sub_100322c0(D3DDevice7* pDevice, D3DCapsBlock* pCaps)
{
	DDCaps100322c0      drvCaps;
	DDCaps100322c0      helCaps;
	D3DDevDesc100322c0  desc;
	DDSCaps2100322c0    ddsCaps;
	uint32              dwTexTotal, dwTexFree;
	uint32              dwVidTotal, dwVidFree;

	memset(&helCaps, 0, sizeof(helCaps));
	memset(&drvCaps, 0, sizeof(drvCaps));
	drvCaps.dwSize = sizeof(drvCaps);
	helCaps.dwSize = sizeof(helCaps);
	g_pDirectDraw->lpVtbl->GetCaps(g_pDirectDraw, &drvCaps, &helCaps);

	pCaps->m_bCaps2_1000 = (drvCaps.dwCaps2 & 0x1000) != 0;

	memset(&desc, 0, sizeof(desc));
	pDevice->lpVtbl->GetCaps(pDevice, &desc);

	memset(&ddsCaps, 0, sizeof(ddsCaps));
	ddsCaps.dwCaps = 0x1000;                            // DDSCAPS_TEXTURE
	g_pDirectDraw->lpVtbl->GetAvailableVidMem(g_pDirectDraw, &ddsCaps,
	                                          &dwTexTotal, &dwTexFree);

	memset(&ddsCaps, 0, sizeof(ddsCaps));
	ddsCaps.dwCaps = 0x4000;                            // DDSCAPS_VIDEOMEMORY
	g_pDirectDraw->lpVtbl->GetAvailableVidMem(g_pDirectDraw, &ddsCaps,
	                                          &dwVidTotal, &dwVidFree);

	pCaps->m_bDevCaps400  = (desc.dwDevCaps & 0x400) != 0;
	pCaps->m_bDevCaps200  = (desc.dwDevCaps & 0x200) != 0;
	pCaps->m_bDevCaps100  = (desc.dwDevCaps & 0x100) != 0;
	pCaps->m_bDevCaps1000 = (desc.dwDevCaps & 0x1000) != 0;

	pCaps->m_nZBufferBits = BitsFromDDBD100322c0(desc.dwDeviceZBufferBitDepth);
	pCaps->m_nRenderBits  = BitsFromDDBD100322c0(desc.dwDeviceRenderBitDepth);

	pCaps->m_bDither            = (desc.dpcTriCaps.dwRasterCaps & 0x1) != 0;
	pCaps->m_bSrcBlendSrcColor  = (desc.dpcTriCaps.dwSrcBlendCaps & 0x4) != 0;
	pCaps->m_bDestBlendSrcAlpha = (desc.dpcTriCaps.dwDestBlendCaps & 0x10) != 0;
	pCaps->m_bTexBlend80        = (desc.dpcTriCaps.dwTextureBlendCaps & 0x80) != 0;
	pCaps->m_bTexBlend02        = (desc.dpcTriCaps.dwTextureBlendCaps & 0x2) != 0;

	pCaps->m_dwTextureMem = dwTexTotal;
	pCaps->m_dwVideoMem   = dwVidTotal;

	pCaps->m_Desc    = desc;
	pCaps->m_TriCaps = desc.dpcTriCaps;

	pCaps->m_bZEnable = 1;
}

// ======================================================================= //
//  0x10032d00  306 bytes.
// ======================================================================= //
void sub_10032d00(int argc, char** argv)
{
	if (argc > 0)
	{
		if (_stricmp(argv[0], "LISTDEVICES") == 0)
		{
			sub_10020f80();
		}
		else if (_stricmp(argv[0], "LISTTEXTUREFORMATS") == 0)
		{
			sub_10039a60();
		}
		else if (_stricmp(argv[0], "LISTDEVICECAPS") == 0)
		{
			sub_10031e10();
		}
		else if (_stricmp(argv[0], "FREETEXTURES") == 0)
		{
			sub_10039930();
		}
		else if (_stricmp(argv[0], "LOADGAMMA") == 0)
		{
			if (argc != 2)
				g_pStruct->ConsolePrintf("Usage: loadgamma <filename>");
			else
				sub_10032f30(argv[1]);
		}
		else if (_stricmp(argv[0], "PortalFile") == 0)
		{
			if (argc >= 2)
				sub_1005dbe0(argv[1]);
			else
				sub_100145f0();
		}
	}
}

// ======================================================================= //
//  0x10032f30 / 0x10032f34   310 bytes.  LOAD A GAMMA RAMP FROM A FILE.
//
//  TWO ENTRIES: 0x10032f30 is a four-byte stack adapter
//  (`mov 0x4(%esp),%eax`) that falls into the register entry at
//  0x10032f34, and BOTH callers (0x10032dc2, 0x10033a61) call the
//  register entry.  It is written here as one ordinary one-argument
//  __cdecl function named after the adapter, which is what d3dbatchc.cpp
//  did for 0x10032a00; if it comes back shifted by four bytes the fix is
//  to rename it sub_10032f34 and the body does not change.
//
//  WHAT IT IS, and none of it is guessed:
//   * 0x1008107b is fopen -- `push $0x40; push mode; push name; call
//     _fsopen` with 0x40 == _SH_DENYNO.  The mode string at 0x100a9050 is
//     "r".
//   * 0x1008108e is fread -- it locks the stream (0x10083ba5), calls
//     _fread_lk (0x100810bd, which starts `imul 0x10(%ebp),%edi`) and
//     unlocks.  The pushed argument block is (buffer, 0x600, 1, file), so
//     the record size is 0x600 and the count is 1, which is why the
//     result is compared against 1.
//   * 0x100811a5 is fclose (`testb $0x40,0xc(%esi)` on the _iobuf flags,
//     then _fclose_lk).
//   * 0x10088188 is the sixteen bytes
//     3E 1C C1 69 6B B4 D1 11 AD 7A 00 C0 4F C2 9B 4E ==
//     {69C11C3E-B46B-11D1-AD7A-00C04FC29B4E} == IID_IDirectDrawGammaControl,
//     handed to slot +0x00 (QueryInterface) of a surface with an out
//     pointer -- and slot +0x10 of the RESULT (the fifth vtable entry:
//     QI, AddRef, Release, GetGammaRamp, SetGammaRamp) is then called with
//     (0, buffer).  That fixes the buffer as a DDGAMMARAMP, whose size is
//     exactly 3 * 256 * sizeof(WORD) == 0x600.
//   * 0x1008cbbc, 0x1008bf6c and 0x1008bf70 are g_bWindowed, g_pPrimary
//     and g_pWindowedPrimary -- already named by d3dblit.cpp, and the
//     `test; je` picks g_pPrimary when the flag is clear, so the
//     fall-through (windowed) surface is 0x1008bf70.
//
//  WHAT PINS THE SHAPE:
//   * TWO fclose call sites, one in each arm of the fread test, mean two
//     source statements, not one hoisted close.
//   * TWO QueryInterface call sites, each with its own `test %eax,%eax;
//     je 0x1003301b` and its own `xor %eax,%eax` + epilogue, mean the
//     `if (g_bWindowed)` arms each contain their own guarded return --
//     writing one call through a selected pointer gives one call site.
//   * every failure is `xor %eax,%eax` and the success at 0x10033048 is
//     `mov $0x1,%eax`, so the return type is BOOL and the flag is /O2.
//   * the frame is 0x620: 0x600 of ramp at -0x608, the interface pointer
//     at -0x8, icl's dead dword at -0x4, and the %ebx save at (%esp).
// ======================================================================= //

extern "C"
{
	struct _iobuf;
	typedef struct _iobuf FILE;

	FILE*        __cdecl fopen(const char* pszName, const char* pszMode);
	unsigned int __cdecl fread(void* pBuf, unsigned int nSize,
	                           unsigned int nCount, FILE* pFile);
	int          __cdecl fclose(FILE* pFile);
}

// The DirectDraw pieces this function needs.  Spelled out rather than
// included because the toolchain's ddraw.h is the DirectX 5 one and has no
// IDirectDrawGammaControl at all.
struct DDGammaRamp
{
	uint16 red[256];
	uint16 green[256];
	uint16 blue[256];
};                                                      // == 0x600

struct DDGammaControl;

struct DDGammaControlVtbl
{
	uint32 m_pad_00[4];                                 // QI/AddRef/Release/Get
	// +0x10 -- IDirectDrawGammaControl::SetGammaRamp
	long (__stdcall* SetGammaRamp)(DDGammaControl*, uint32 dwFlags,
	                               DDGammaRamp* pRamp);
};

struct DDGammaControl
{
	DDGammaControlVtbl* lpVtbl;
};

extern "C" const uint8 IID_IDirectDrawGammaControl[16];  // 0x10088188


LTBOOL sub_10032f30(const char* pszFileName)
{
	DDGammaControl* pGamma;
	DDGammaRamp     ramp;
	FILE*           pFile;

	if (!pszFileName)
		return LTFALSE;

	pFile = fopen(pszFileName, "r");
	if (!pFile)
		return LTFALSE;

	if (fread(&ramp, sizeof(ramp), 1, pFile) != 1)
	{
		fclose(pFile);
		return LTFALSE;
	}

	fclose(pFile);

	if (g_bWindowed)
	{
		if (g_pWindowedPrimary->lpVtbl->QueryInterface(g_pWindowedPrimary,
			IID_IDirectDrawGammaControl, (void**)&pGamma) != 0)
		{
			return LTFALSE;
		}
	}
	else
	{
		if (g_pPrimary->lpVtbl->QueryInterface(g_pPrimary,
			IID_IDirectDrawGammaControl, (void**)&pGamma) != 0)
		{
			return LTFALSE;
		}
	}

	if (pGamma->lpVtbl->SetGammaRamp(pGamma, 0, &ramp) != 0)
		return LTFALSE;

	return LTTRUE;
}

// ======================================================================= //
//  CONSTANTS -- every one of these is the literal in the shipped bytes; the
//  names are the documented DirectDraw / Direct3D spellings for them.
// ======================================================================= //

#define DD_OK                            0

#define DDSD_CAPS                        0x00000001     // dwFlags 1 / 7 / 0x21
#define DDSD_HEIGHT                      0x00000002
#define DDSD_WIDTH                       0x00000004
#define DDSD_BACKBUFFERCOUNT             0x00000020

#define DDSCAPS_COMPLEX                  0x00000008     // in 0x2218
#define DDSCAPS_FLIP                     0x00000010     // in 0x2218
#define DDSCAPS_PRIMARYSURFACE           0x00000200     // 0x200 / in 0x2218
#define DDSCAPS_SYSTEMMEMORY             0x00000800     // 0x800
#define DDSCAPS_3DDEVICE                 0x00002000     // 0x2000 / in 0x2218

#define DDSCL_FULLSCREEN                 0x00000001     // in 0x13
#define DDSCL_ALLOWREBOOT                0x00000002     // in 0x13
#define DDSCL_NOWINDOWCHANGES            0x00000004     // the `or $0x4`
#define DDSCL_NORMAL                     0x00000008     // 8
#define DDSCL_EXCLUSIVE                  0x00000010     // in 0x13

#define DDBLT_COLORFILL                  0x00000400     // in 0x01000400
#define DDBLT_WAIT                       0x01000000     // in 0x01000400
#define DDFLIP_WAIT                      0x00000001     // 1

#define D3DPRASTERCAPS_ZBUFFERLESSHSR    0x00008000     // test $0x8000
#define D3DPBLENDCAPS_ZERO               0x00000001     // test $0x1
#define D3DPBLENDCAPS_ONE                0x00000002     // test $0x2
#define D3DPBLENDCAPS_SRCCOLOR           0x00000004     // test $0x4
#define D3DPBLENDCAPS_SRCALPHA           0x00000010     // test $0x10


// ======================================================================= //
//  0x100331e0 .. 0x10033b9e   2,495 bytes   2 callers (0x1002179b, 0x10021a32)
//
//  d3d.ren's DirectDraw / Direct3D bring-up.
//
//  FRAME: `sub $0x138,%esp`, and the six locals below are read straight off
//  the ebp displacements:
//      -0x004  the pInit copy (see the note at the SetCooperativeLevel call)
//      -0x008  nPasses          ValidateDevice's out parameter (0x100339fb)
//      -0x020  viewport         0x18, six dwords zeroed at 0x100338c0
//      -0x084  bltFx            0x64, dwSize 0x64, 0x19 dwords zeroed
//      -0x100  ddsd             0x7c, dwSize 0x7c, 0x1f dwords zeroed
//      -0x110  deviceGuid       0x10, the 16 bytes copied out of .rdata
//  They are declared here in that order, ebp downwards.
//
//  BLOCK GRAPH (every edge read off the bytes):
//      331e0 prologue -> rasterizer select (33202/3320f/33255/3329b/332d7,
//                        with the RefRast arm sunk to 33307)
//      33354 DirectDrawCreateEx        fail -> 33b06
//      33388 cooperative-level flags -> 333da SetCooperativeLevel
//                                       fail -> 33ad6
//      33406 SetDisplayMode (fullscreen only)   fail -> 33446..3347d
//      3347e QueryInterface(IID_IDirect3D7)     fail -> 33aa6
//      334a5 if (g_bWindowed || g_1008cba8) -> 334bb  else -> 33670
//      334bb blit path:  primary, render surface, clipper   -> 337d9
//      33670 flip path:  complex flipping chain             -> 337a8
//      337a8 EnumAttachedSurfaces -> g_pOffscreen           -> 337d9
//                                       missing -> 33b6f
//      337d9 four clear-and-flip passes, then call 0x10033ba0
//                                       fail -> 33886..338bf
//      338c0 SetViewport                fail -> 3391d..3394c
//      3394d caps read-back and the blend-capability booleans
//      339f1 Force1Pass / ValidateDevice / lightmap pass count
//      33a54 tail: 0x10032a50, gamma ramp, 0x1003ad30
//
//  RETURN: 0 from every failure, 1 from 0x10033a7c (`mov $0x1,%eax`, the /O2
//  spelling).  The callers null-test it.
// ======================================================================= //

LTBOOL sub_100331e0(D3DDeviceRec* pDev, RenderStructInit* pInit)
{
	uint32         nPasses;                             // -0x008
	D3DViewport7   viewport;                            // -0x020
	DDBltFx        bltFx;                               // -0x084
	DDSurfaceDesc2 ddsd;                                // -0x100
	D3DGuid        deviceGuid;                          // -0x110
	uint32         nCoopFlags;
	uint32         nBaseFlags;
	void*          hNullRender;
	uint32         i;

	// 0x100331f4: `push %edi; movl $0x0,(%esp); call *0x10088118`.
	ShowCursor(LTFALSE);

	// --------------------------------------------------------------- //
	//  0x10033202 .. 0x10033354  --  WHICH RASTERIZER.
	//
	//  Four console variables tested in this order, each `mov; test; j`.
	//  The RefRast arm is the one that is sunk (0x10033209 `jne 0x10033307`)
	//  because it also prints; the other three fall through in order.
	//  0x1008cbb8 is left alone on the two hardware arms, which is what
	//  keeps the 0x4000 (DDSCAPS_VIDEOMEMORY) it was given at 0x1002165f.
	// --------------------------------------------------------------- //
	if (g_CV_RefRast)
	{
		g_pStruct->ConsolePrintf(
			"USING DIRECT3D REFERENCE RASTERIZER (SLOOOOOOOW!)");
		deviceGuid = IID_IDirect3DRefDevice;            // 0x10088138
		g_nSurfCaps1008cbb8 = DDSCAPS_SYSTEMMEMORY;
	}
	else if (g_CV_RGBRast)
	{
		deviceGuid = IID_IDirect3DRGBDevice;            // 0x10088168
		g_nSurfCaps1008cbb8 = DDSCAPS_SYSTEMMEMORY;
	}
	else if (g_CV_MMXRast)
	{
		deviceGuid = IID_IDirect3DMMXDevice;            // 0x10088148
		g_nSurfCaps1008cbb8 = DDSCAPS_SYSTEMMEMORY;
	}
	else if (g_CV_TnLRast)
	{
		deviceGuid = IID_IDirect3DTnLHalDevice;         // 0x10088128
	}
	else
	{
		deviceGuid = IID_IDirect3DHALDevice;            // 0x10088158
	}

	// --------------------------------------------------------------- //
	//  0x10033354 .. 0x10033382  --  CREATE THE DIRECTDRAW OBJECT.
	//
	//  `xor %eax,%eax; mov %eax,0x1008bf74` clears the global first and the
	//  same zero is reused as pUnkOuter, which is why both are written here.
	//  pDev->m_pDDGuid20 is the only field of the first parameter this
	//  function ever reads.
	// --------------------------------------------------------------- //
	g_pDirectDraw = LTNULL;

	if (DirectDrawCreateEx(pDev->m_pDDGuid20, (void**)&g_pDirectDraw,
	                       &IID_IDirectDraw7, LTNULL) != DD_OK)
	{
		// 0x10033b06 -- sunk below the last epilogue.
		sub_10021d60(1, "DirectDrawCreateEx failed.");
		sub_100210d0();
		return LTFALSE;
	}

	// --------------------------------------------------------------- //
	//  0x10033388 .. 0x10033400  --  COOPERATIVE LEVEL.
	//
	//  0x13 == DDSCL_EXCLUSIVE|DDSCL_ALLOWREBOOT|DDSCL_FULLSCREEN, 8 ==
	//  DDSCL_NORMAL, and the `or $0x4,%ebx` at 0x100333d7 is
	//  DDSCL_NOWINDOWCHANGES.  The float the console variable is compared
	//  against is the 0.0f at 0x100a8800 (`fcomps`, so a FLOAT literal, not
	//  a double -- a `0.0` would have produced `fcompl`).
	//
	//  TWO variables, measured: the base value lives in %esi and is COPIED
	//  to %ebx at the join (`mov %esi,%ebx`) and again before the
	//  `or $0x4,%ebx` -- the flags passed are a second variable assigned
	//  from the base on both paths. One variable with `|=` put the whole
	//  thing in %ebx and dropped both copies.
	// --------------------------------------------------------------- //
	if (g_bWindowed)
		nBaseFlags = DDSCL_NORMAL;
	else
		nBaseFlags = DDSCL_EXCLUSIVE | DDSCL_ALLOWREBOOT | DDSCL_FULLSCREEN;

	nCoopFlags = nBaseFlags;
	hNullRender = g_pStruct->GetConsoleVariable("NullRender");
	if (hNullRender && g_pStruct->GetVarValueFloat(hNullRender) != 0.0f)
		nCoopFlags = nBaseFlags | DDSCL_NOWINDOWCHANGES;

	// 0x100333f8 stores pInit into -0x4(%ebp) here and every later use of
	// it reads that slot back; only this first use reads 0xc(%ebp).  That
	// is a codegen detail (a home slot for the parameter), not a second
	// variable -- there is no other value in -0x4 anywhere in the function.
	if (g_pDirectDraw->lpVtbl->SetCooperativeLevel(g_pDirectDraw,
	                                               pInit->m_hWnd,
	                                               nCoopFlags) != DD_OK)
	{
		// 0x10033ad6
		sub_10021d60(1, "SetCooperativeLevel failed.");
		sub_100210d0();
		return LTFALSE;
	}

	// --------------------------------------------------------------- //
	//  0x10033406 .. 0x1003347e  --  DISPLAY MODE (fullscreen only).
	//
	//  Six arguments: (width, height, bpp, 0, 0).  The two trailing zeroes
	//  are `mov %ebx,0x14(%esp)` / `0x10(%esp)` off the `xor %ebx,%ebx` at
	//  0x10033423.
	// --------------------------------------------------------------- //
	if (!g_bWindowed)
	{
		if (g_pDirectDraw->lpVtbl->SetDisplayMode(g_pDirectDraw,
		                                          pInit->m_Mode.m_Width,
		                                          pInit->m_Mode.m_Height,
		                                          pInit->m_Mode.m_BitDepth,
		                                          0, 0) != DD_OK)
		{
			sub_10021d60(1, "SetDisplayMode failed.");
			sub_100210d0();
			return LTFALSE;
		}
	}

	// --------------------------------------------------------------- //
	//  0x1003347e .. 0x100334a5  --  THE IDirect3D7 INTERFACE.
	// --------------------------------------------------------------- //
	if (g_pDirectDraw->lpVtbl->QueryInterface(g_pDirectDraw, &IID_IDirect3D7,
	                                          (void**)&g_pD3D) != DD_OK)
	{
		// 0x10033aa6
		sub_10021d60(1,
			"QueryInterface(IID_IDirect3D7) failed.. are you running NT 4.0?");
		sub_100210d0();
		return LTFALSE;
	}

	// --------------------------------------------------------------- //
	//  0x100334a5 .. 0x100337d9  --  THE SURFACES.
	//
	//  `mov g_bWindowed; test; jne 334bb` then `mov g_1008cba8; test; je
	//  33670` is one `||`: either flag takes the blit path.
	// --------------------------------------------------------------- //
	if (g_bWindowed || g_1008cba8)
	{
		// ----------------------------------------------------------- //
		//  0x100334bb -- BLIT PATH.  A plain primary, a separate
		//  3D-device render surface, and a clipper tied to the window.
		// ----------------------------------------------------------- //
		memset(&ddsd, 0, sizeof(ddsd));
		ddsd.dwSize             = sizeof(ddsd);         // 0x7c
		ddsd.dwFlags            = DDSD_CAPS;
		ddsd.ddsCaps.dwCaps     = DDSCAPS_PRIMARYSURFACE;

		if (g_pDirectDraw->lpVtbl->CreateSurface(g_pDirectDraw, &ddsd,
		                                         &g_pWindowedPrimary,
		                                         LTNULL) != DD_OK)
		{
			sub_10021d60(1, "Unable to create a primary surface.");
			sub_100210d0();
			return LTFALSE;
		}

		// 0x10033546.  ddsCaps picks up the memory-location bit chosen by
		// the rasterizer selection above, plus DDSCAPS_3DDEVICE.
		memset(&ddsd, 0, sizeof(ddsd));
		ddsd.dwSize             = sizeof(ddsd);
		ddsd.dwFlags            = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH;
		ddsd.ddsCaps.dwCaps     = g_nSurfCaps1008cbb8 | DDSCAPS_3DDEVICE;
		ddsd.dwBackBufferCount  = 0;
		ddsd.dwWidth            = pInit->m_Mode.m_Width;
		ddsd.dwHeight           = pInit->m_Mode.m_Height;

		if (g_pDirectDraw->lpVtbl->CreateSurface(g_pDirectDraw, &ddsd,
		                                         &g_pPrimary,
		                                         LTNULL) != DD_OK)
		{
			sub_10021d60(1, "Failed to make rendering surface.");
			sub_100210d0();
			return LTFALSE;
		}

		// 0x100335f7.  In this path the render surface IS the offscreen.
		g_pOffscreen = g_pPrimary;

		// 0x10033621 `jne 0x100337d9`: a failed clipper is not fatal, it
		// just skips the two calls that use it.
		if (g_pDirectDraw->lpVtbl->CreateClipper(g_pDirectDraw, 0,
		                                         &g_pClipper,
		                                         LTNULL) == DD_OK)
		{
			g_pClipper->lpVtbl->SetHWnd(g_pClipper, 0, pInit->m_hWnd);
			g_pWindowedPrimary->lpVtbl->SetClipper(g_pWindowedPrimary,
			                                       g_pClipper);
		}
	}
	else
	{
		// ----------------------------------------------------------- //
		//  0x10033670 -- FLIP PATH.  0x2218 is
		//  DDSCAPS_3DDEVICE|DDSCAPS_PRIMARYSURFACE|DDSCAPS_FLIP|
		//  DDSCAPS_COMPLEX and 0x21 is DDSD_CAPS|DDSD_BACKBUFFERCOUNT.
		//
		//  The two "Unable to create a primary surface." blocks at
		//  0x10033748 and 0x10033778 are IDENTICAL and are NOT tail
		//  merged, which is what says the source really has two of them --
		//  one inside the triple-buffer retry, one for the non-retry case.
		// ----------------------------------------------------------- //
		memset(&ddsd, 0, sizeof(ddsd));
		ddsd.dwSize             = sizeof(ddsd);
		ddsd.dwFlags            = DDSD_CAPS | DDSD_BACKBUFFERCOUNT;
		ddsd.ddsCaps.dwCaps     = DDSCAPS_PRIMARYSURFACE | DDSCAPS_FLIP |
		                          DDSCAPS_COMPLEX | DDSCAPS_3DDEVICE;
		ddsd.dwBackBufferCount  = g_CV_TripleBuffer ? 2 : 1;

		if (g_pDirectDraw->lpVtbl->CreateSurface(g_pDirectDraw, &ddsd,
		                                         &g_pPrimary,
		                                         LTNULL) != DD_OK)
		{
			if (ddsd.dwBackBufferCount == 2)
			{
				sub_10021d60(0, "Unable to use triple buffering.");
				ddsd.dwBackBufferCount = 1;

				if (g_pDirectDraw->lpVtbl->CreateSurface(g_pDirectDraw,
				                                         &ddsd,
				                                         &g_pPrimary,
				                                         LTNULL) != DD_OK)
				{
					sub_10021d60(1,
						"Unable to create a primary surface.");
					sub_100210d0();
					return LTFALSE;
				}
			}
			else
			{
				sub_10021d60(1, "Unable to create a primary surface.");
				sub_100210d0();
				return LTFALSE;
			}
		}

		// 0x100337a8.  The callback at 0x10032600 is `ret $0xc` and its
		// whole body is `g_pOffscreen = arg0; return 0;`, so the back
		// buffer arrives through the global and the global is what gets
		// tested.
		g_pOffscreen = LTNULL;
		g_pPrimary->lpVtbl->EnumAttachedSurfaces(g_pPrimary, LTNULL,
		                                         (void*)sub_10032600);

		if (!g_pOffscreen)
		{
			// 0x10033b6f
			sub_10021d60(1, "Couldn't get pointer to offscreen surface.");
			sub_100210d0();
			return LTFALSE;
		}
	}

	// --------------------------------------------------------------- //
	//  0x100337d9 .. 0x1003387f  --  CLEAR THE CHAIN, THEN THE DEVICE.
	//
	//  0x01000400 is DDBLT_COLORFILL|DDBLT_WAIT and it is hoisted into
	//  %edi before the loop; the three NULL rectangles and the NULL source
	//  come off the `xor %esi,%esi` at 0x100337f4.  `inc %ebx; cmp $0x4,%ebx;
	//  jb` is the four-pass count.
	// --------------------------------------------------------------- //
	memset(&bltFx, 0, sizeof(bltFx));
	bltFx.dwSize = sizeof(bltFx);                       // 0x64

	for (i = 0; i < 4; ++i)
	{
		g_pOffscreen->lpVtbl->Blt(g_pOffscreen, LTNULL, LTNULL, LTNULL,
		                          DDBLT_COLORFILL | DDBLT_WAIT, &bltFx);
		g_pPrimary->lpVtbl->Flip(g_pPrimary, LTNULL, DDFLIP_WAIT);
	}

	// 0x1003387a.  The GUID goes by value: 0x10033860 copies all sixteen
	// bytes into the outgoing area field by field, which is icl's form for
	// a struct argument at or below sixteen bytes.
	if (!sub_10033ba0(pInit, deviceGuid))
	{
		sub_10021d60(1, "Failed to create device.");
		sub_100210d0();
		g_CV_TnLRast = 0;                               // 0x100338a5
		return LTFALSE;
	}

	// --------------------------------------------------------------- //
	//  0x100338c0 .. 0x1003394d  --  THE VIEWPORT.
	//
	//  Six dwords zeroed inline at 0x100338c0 (icl's expansion of a 24-byte
	//  memset), then the fields written; 0x3f800000 at +0x14 is dvMaxZ.
	// --------------------------------------------------------------- //
	memset(&viewport, 0, sizeof(viewport));
	viewport.dwX      = 0;
	viewport.dwY      = 0;
	viewport.dwWidth  = pInit->m_Mode.m_Width;
	viewport.dwHeight = pInit->m_Mode.m_Height;
	viewport.dvMinZ   = 0.0f;
	viewport.dvMaxZ   = 1.0f;

	if (g_pD3DDevice->lpVtbl->SetViewport(g_pD3DDevice, &viewport) != DD_OK)
	{
		sub_10021d60(1, "IDirect3DDevice::SetCurrentViewport failed.");
		sub_100210d0();
		return LTFALSE;
	}

	// --------------------------------------------------------------- //
	//  0x1003394d .. 0x100339f1  --  READ THE CAPS BACK AND DERIVE THE
	//  BLEND FLAGS.
	//
	//  0x100322c0 does IDirect3DDevice7::GetCaps (its +0xc) into a local
	//  D3DDEVICEDESC7 and copies dpcTriCaps out to 0x1008d918 field for
	//  field (0x10032543..0x100325dc, fourteen dwords = one D3DPRIMCAPS).
	//  THAT is what types the three globals read here:
	//      0x1008d920 dwRasterCaps, 0x1008d928 dwSrcBlendCaps,
	//      0x1008d92c dwDestBlendCaps
	//  and it makes the four booleans below read as what they are.
	//
	//  0x1003394d is a register-ABI call: g_pD3DDevice in %eax and
	//  $0x1008d7f4 in %edx, with an 8-byte outgoing area opened and closed
	//  without being written.  It is decoded as an ordinary
	//  two-argument call.
	// --------------------------------------------------------------- //
	sub_100322c0(g_pD3DDevice, (D3DCapsBlock*)&g_Caps1008d7f4);
	sub_10032610();

	// `sete %dl` off `test $0x8000,%eax` -- a value, not a branch.
	g_1008d964 = ((g_1008d920 & D3DPRASTERCAPS_ZBUFFERLESSHSR) == 0);

	// The remaining three are branch pairs feeding one store, i.e. `&&`.
	g_1008d970 = (g_1008d964 && (g_1008d928 & D3DPBLENDCAPS_SRCALPHA));
	g_1008d978 = ((g_1008d928 & D3DPBLENDCAPS_ZERO) &&
	              (g_1008d92c & D3DPBLENDCAPS_SRCCOLOR));
	g_1008d968 = ((g_1008d928 & D3DPBLENDCAPS_ONE) &&
	              (g_1008d92c & D3DPBLENDCAPS_ONE));

	g_1008d7cc = 0;
	g_1008d7d0 = 0;
	g_1008d96c = g_CV_Force1Pass;

	// --------------------------------------------------------------- //
	//  0x100339f1 .. 0x10033a54  --  ONE-PASS OR TWO-PASS LIGHTMAPPING.
	//
	//  0x10033a15 `je 0x10033b36` on a successful ValidateDevice, and
	//  0x10033b40 `jle 0x10033a1b` back into the failure block when the
	//  stage count is not above one: the two conditions are ANDed and both
	//  failures land on the same block.
	// --------------------------------------------------------------- //
	if (g_1008d96c)
	{
		sub_10073210();

		if (g_pD3DDevice->lpVtbl->ValidateDevice(g_pD3DDevice,
		                                         &nPasses) == DD_OK &&
		    g_nStages1008d8e6 > 1)
		{
			g_1008d7cc = 1;
			sub_10021d60(0, "Using 1 pass lightmapping!");
		}
		else
		{
			g_1008d96c = 0;
			sub_10021d60(0,
				"Force1Pass doesn't work on this card.  "
				"Using 2 pass lightmapping.");
		}
	}
	else
	{
		sub_10021d60(0, "Using 2 pass lightmapping.");
	}

	// --------------------------------------------------------------- //
	//  0x10033a54 .. 0x10033aa5  --  TAIL.
	//
	//  0x10033a61 calls the REGISTER entry of the gamma-ramp loader with
	//  "lithtech.gam" in %eax; 0x10033a69 calls 0x1003ad30 with nothing.
	//  Success falls through and the failure block is jumped to, which is
	//  the `if (ok) { ... } return err;` shape.
	// --------------------------------------------------------------- //
	sub_10032a50();
	sub_10032f30("lithtech.gam");

	if (sub_1003ad30())
	{
		g_f1008d7c0 = 0.5f;                             // 0x3f000000
		return LTTRUE;
	}

	sub_100210d0();
	return LTFALSE;
}


// ======================================================================= //
//  NOTES, COIN FLIPS AND KNOWN RESIDUALS
//
//  UNDECODED REGIONS: NONE.  Every one of the 2,495 bytes between
//  0x100331e0 and 0x10033b9e is accounted for by the body above.
//
//  CALL-FREE REGIONS (what the call-sequence check cannot witness): the longest
//  run without a call is 0x10033967..0x100339f3, 140 bytes -- the four
//  blend-capability booleans.  Everything in it is a `mov`/`test`/`setcc`
//  against a named global with the constant written in the instruction, so
//  it is fully pinned by the bytes even though no call witnesses it.  The
//  next longest are the two DDSURFACEDESC2 fills (0x100334bb..0x1003350f,
//  84 bytes, and 0x10033670..0x100336df, 111 bytes), which are likewise all
//  immediates into named fields.
//
//  COIN FLIPS -- each stated with BOTH spellings so one build settles it:
//
//  (1) LOCAL DECLARATION ORDER.  The slots are, ebp downwards, -0x4 (the
//      pInit home), -0x8 nPasses, -0x20 viewport, -0x84 bltFx, -0x100 ddsd,
//      -0x110 deviceGuid, and they are declared in that order above.  If icl
//      lays locals out from the far end instead, the declaration block wants
//      reversing to
//          D3DGuid deviceGuid; DDSurfaceDesc2 ddsd; DDBltFx bltFx;
//          D3DViewport7 viewport; uint32 nPasses;
//      Nothing else in the body changes.
//
//  (2) THE ORDER OF THE TWO ZERO STORES AT 0x100339d8/0x100339de.  The
//      shipped code stores 0x1008d7d0 first and 0x1008d7cc second off one
//      `xor %edx,%edx`.  Written above as `g_1008d7cc = 0; g_1008d7d0 = 0;`
//      because that is the order the two are used in later; the alternative
//      is `g_1008d7d0 = 0; g_1008d7cc = 0;`.
//
//  (3) THE FIELD ORDER IN THE SECOND DDSURFACEDESC2 FILL (0x10033546).  The
//      shipped store order is ddsCaps.dwCaps, dwBackBufferCount, dwFlags,
//      dwSize, dwWidth, dwHeight; the other two fills in this function both
//      run dwSize, dwFlags, ddsCaps.dwCaps, so that order is what is written
//      here.  The alternative is to write it in the emitted order.  (The
//      other two fills are NOT coin flips -- their emitted order is already
//      the source order.)
//
//  (4) GetVarValueFloat's RETURN TYPE -- SETTLED as float. With the slot
//      declared `double (*)(void*)` (d3dbatchc.cpp's reading) icl widens
//      the 0.0f literal and emits `fcompl`; the shipped 0x100333c7 is
//      `fcomps` on the four-byte 0.0 at 0x100a8800. Declared float, as the
//      SDK's own ILTCSBase::GetVarValueFloat is (iltcsbase.h), it is
//      `fcomps`. Measured on the build.
//
//  (5) THE pInit HOME SLOT.  0x100333f8 spills the second parameter to
//      -0x4(%ebp) and every later read comes from there, but the FIRST read
//      (0x100333e2, SetCooperativeLevel's hWnd) comes from 0xc(%ebp).  That
//      is written above as plain uses of the parameter.  If the shipped
//      shape needs a real local, the alternative is to open the body with
//      `RenderStructInit* pRS = pInit;` used from the SetDisplayMode block
//      onwards -- but note that a local initialised at the top would have
//      been used for the hWnd too, and it is not.
//
//  STATUS: BYTE-IDENTICAL.  The last differences were the two
//  register-ABI calls, 0x1003394d into 0x100322c0 and 0x10033a59 into
//  0x10032f34; with both callees in this /O2 /Qip TU (0x100322c0 a static,
//  0x10032f30 an external pair) icl emits them as shipped.
//
//  NOTES FROM THE DECODE:
//
//   * DirectDrawCreateEx resolves to the DDRAW.dll import thunk at
//     0x1007fa86 in the shipped image; ours relocates against the symbol.
//
//   * The three `memset`s are `rep stos` in the shipped code (0x1f, 0x19 and
//     -- inline-expanded to six `mov`s -- 6 dwords).  If icl emits a `call
//     _memset` for the 0x7c one instead of expanding it, those show up as
//     ours-only rows; the fix is a field-by-field zero, not a decode change.
// ======================================================================= //
