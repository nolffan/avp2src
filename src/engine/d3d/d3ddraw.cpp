// *********************************************************************** //
//
//  MODULE:   D3DDRAW (.CPP)
//
//  PURPOSE:  The RenderStruct draw entry points d3d.ren fills in.
//
//  These are reached only through the RenderStruct table that
//  RenderDLLSetup (0x10021410) hands back to the engine; see d3d.cpp for
//  the slot-to-address mapping and how it was established.
//
//  READ THIS BEFORE ADDING FUNCTIONS HERE.
//
//  d3d.ren was NOT built with the compiler the rest of the game was built
//  with, and until that is settled only functions with no stack frame can
//  reproduce. The evidence:
//
//    * d3d.ren pads between functions with multi-byte lea-nops --
//      `8d 76 00` and `8d bc 27 00 00 00 00`, 214 and 368 of them. It is
//      the ONLY module of the retail install that contains a single one. server.dll and
//      lithtech.exe have zero; they pad with 0x90 and 0xcc.
//    * Nothing cl 6.00 emits looks like it. Start3D was compiled at
//      revisions 8168, 8447 and 8966, each at /O2 and /Od: every one comes
//      out 46 or 71 bytes against the shipped 65, with 33 or 41 differing
//      bytes, and no object we produce contains that padding.
//    * The shipped code keeps a frame pointer on every function (`push
//      %ebp; mov %esp,%ebp`), which /O2 omits, while still optimising tails
//      into a shared `xor %eax,%eax` -- and it allocates locals it never
//      uses (Start3D's `sub $0x4,%esp`). It saves registers with
//      `mov %ebx,(%esp)` after the frame is opened rather than `push`, and
//      adjusts the stack with `add $0xfffffff8,%esp` rather than `sub $8`.
//
//  It IS the shipped 2001 binary, not a later rebuild: it links at
//  2001-04-25 20:26:46 with MS linker 6.0, two minutes before server.dll,
//  and it carries the MSVC static CRT (R6002, "Runtime Error!"), which is
//  what the Rich header's C objects are. A Rich header only records objects
//  made by Microsoft tools, so the renderer's own objects need not appear
//  in it at all -- which is why that header cannot be used to argue about
//  what compiled this code.
//
//  So: a function that is `mov <global>,%eax; ret` reproduces under any
//  compiler and is real work. Anything with a frame is blocked. Do not
//  spend a day writing bodies that cannot be verified.
//
// *********************************************************************** //

#include "ltbasedefs.h"


// The Direct3D 7 device, called the way d3d.ren calls it: through an
// explicit vtable pointer with `this` pushed as the first argument. Only the
// slots this file reaches are named; the rest hold their offsets.
//
// The two slots used here are pinned WITHOUT the DX7 headers (the toolchain
// ships the DirectX 5 d3d.h, which stops at IDirect3DDevice2). Start3D calls
// +0x14 and End3D calls +0x18 on the same object, both taking nothing but
// `this` and both answering an HRESULT that is tested against zero -- which
// is the adjacent BeginScene/EndScene pair, at indices 5 and 6 of
// IDirect3DDevice7. End3D confirms it from the other side: it compares the
// answer against 0x887601c2, DDERR_SURFACELOST, and recovers by calling
// +0x6c (Restore) on three surfaces.
struct D3DDevice;

struct D3DDeviceVtbl
{
	void*     _QueryInterface;                          // +0x00
	void*     _AddRef;                                  // +0x04
	void*     _Release;                                 // +0x08
	void*     _GetCaps;                                 // +0x0c
	void*     _EnumTextureFormats;                      // +0x10
	long (__stdcall* BeginScene)(D3DDevice*);           // +0x14
	long (__stdcall* EndScene)(D3DDevice*);             // +0x18
};

struct D3DDevice
{
	D3DDeviceVtbl* lpVtbl;
};

extern D3DDevice* g_pD3DDevice;                         // 0x1008d988

// 0x1008d7c8 -- non-zero between Start3D and End3D.
LTBOOL g_bIn3D;


// 0x10032f20 -- `mov 0x1008d7c8,%eax; ret`, six bytes.
LTBOOL d3d_IsIn3D()
{
	return g_bIn3D;
}


// 0x10032e50 -- open a 3D scene, 65 bytes. Three refusals share one
// `xor %eax,%eax` epilogue: already in 3D, no device, or BeginScene failed.
//
// The DECODE is settled and is not the problem; see the header note. Kept
// here because it is correct source and costs nothing, and because it is the
// measuring stick for any candidate compiler: whatever builds d3d.ren must
// turn this into 65 bytes with a frame pointer and a dead 4-byte local.
LTBOOL d3d_Start3D()
{
	if (g_bIn3D || !g_pD3DDevice)
		return LTFALSE;

	if (g_pD3DDevice->lpVtbl->BeginScene(g_pD3DDevice))
		return LTFALSE;

	g_bIn3D = LTTRUE;

	return LTTRUE;
}


// ---------------------------------------------------------------------- //
//  Decoded, and correct as source, but NOT verifiable until the compiler
//  question above is settled. Every one of these carries a stack frame or
//  the `xor %eax,%eax; inc %eax` idiom for `return 1` -- three bytes where
//  cl emits five for `mov $1,%eax` -- and cl emits neither shape.
//
//  Measured against cl 8168 /O2, so the gap is on record rather than
//  asserted:
//
//      d3d_IsIn3D                 6 / 6     BYTE-IDENTICAL
//      d3d_SetOptimized2DColor   20 / 26    12 bytes differ
//      d3d_DestroyWorld          26 / 33    15 bytes differ
//      d3d_Start3D               46 / 65    33 bytes differ
//      d3d_CreateWorld          110 / 128   72 bytes differ
//      d3d_End3D                113 / 128   72 bytes differ
//
//  The pattern is the same every time: ours is SHORTER because cl omits the
//  frame the shipped build keeps and spells `return 1` in five bytes rather
//  than three. Nothing about the decodes is in question -- re-measure these
//  four first against any candidate compiler.
// ---------------------------------------------------------------------- //


// 0x10021c50 / 0x10021cb0 -- the renderer's allocator pair. CreateWorld asks
// 0x10021c50 for 0x10 bytes and DestroyWorld hands the same pointer to
// 0x10021cb0, which is what pairs them.
extern void* d3d_Alloc(uint32 nSize);
extern void  d3d_Free(void* pMem);

// 0x1006d130 -- tears a render world's contents down before it is freed.
extern void  d3d_TermRenderWorld(void* pRenderWorld);

// 0x10089448 -- the colour Optimized2D drawing is modulated by.
uint32 g_Optimized2DColor;


// 0x10032c00 -- RenderStruct +0x80, 33 bytes. `g_pRenderDestroyWorld` on the
// client side. Tear down, then free; both on the same pointer, and the whole
// body is guarded on it being non-null.
void d3d_DestroyWorld(void* pRenderWorld)
{
	if (pRenderWorld)
	{
		d3d_TermRenderWorld(pRenderWorld);
		d3d_Free(pRenderWorld);
	}
}


// 0x10034eb0 -- RenderStruct +0xa8, 26 bytes. `g_pRenderSetOptimized2DColor`.
// The alpha byte is forced to 0xff rather than taken from the caller, and the
// answer is unconditional.


// 0x1008d7c4 -- non-zero while an Optimized2D batch is open.
extern LTBOOL g_bInOptimized2D;

// 0x1008ca34 -- the NoLMPages console variable. 0x1008d978 -- whether
// lightmap pages are available; CreateWorld clears it on first failure so the
// warning is printed once.
extern LTBOOL g_bNoLMPages;
extern LTBOOL g_bLightmapPages;

// 0x1006cc90 -- build a render world's lightmap pages; zero means it failed.
extern LTBOOL d3d_CreateLightmapPages(void* pRenderWorld);
// 0x10021d10 / 0x10021d60 -- the renderer's one- and two-argument warning
// printers. The two-argument form is called with a leading 0 here.
extern void sub_10021d10(const char* pszMsg, ...);  // 0x10021d10 (was d3d_Warn/d3d_Print; icl treats a callee named *Warn* as cold)
extern void sub_10021d60(uint32 nLevel, const char* pszMsg, ...);  // 0x10021d60 AddDebugMessage (was d3d_Warning*; icl treats a callee named *Warn*/*Error* as cold)

// 0x10034b00 -- RenderStruct +0x98, `g_pRenderEndOptimized2D`. Declared so
// End3D can close an batch the caller left open; the body is not written
// (972 bytes, and unverifiable until the compiler question is settled).
extern void   d3d_EndOptimized2D();

// 0x10039930 -- run after a lost surface has been restored.
extern void   d3d_OnSurfacesRestored();

// The three surfaces End3D restores when EndScene reports the device lost.
// 0x6c is IDirectDrawSurface7::Restore -- index 27, and all three are called
// through the same slot on three different objects.
struct DDSurface;
struct DDSurfaceVtbl
{
	uint8   _pad_00[0x6c];
	long (__stdcall* Restore)(DDSurface*);              // +0x6c
};
struct DDSurface { DDSurfaceVtbl* lpVtbl; };

extern DDSurface* g_pPrimarySurface;                    // 0x1008bf6c
extern DDSurface* g_pBackSurface;                       // 0x1008bf68
extern DDSurface* g_pZBuffer;                           // 0x1008bf64

// DDERR_SURFACELOST. End3D tests EndScene's answer against it exactly, not
// with FAILED(), which is why the constant is spelled out.
#define DDERR_SURFACELOST 0x887601c2


// 0x10032ea0 -- RenderStruct +0x8c, `g_pRenderEnd3D`, 128 bytes. Closes the
// scene, and if the device was lost restores the three surfaces in the order
// primary, back, z -- each through Restore, and only while the previous one
// answered success.
LTBOOL d3d_End3D()
{
	// One test: as two `if ... return` statements the not-in-3D exit is
	// hosted after the body instead of inline (84 bytes differ).
	if (!g_bIn3D || !g_pD3DDevice)
		return LTFALSE;

	if (g_bInOptimized2D)
		d3d_EndOptimized2D();

	g_bIn3D = LTFALSE;

	if (g_pD3DDevice->lpVtbl->EndScene(g_pD3DDevice) == DDERR_SURFACELOST)
	{
		if (g_pPrimarySurface->lpVtbl->Restore(g_pPrimarySurface) >= 0)
		{
			g_pBackSurface->lpVtbl->Restore(g_pBackSurface);
			g_pZBuffer->lpVtbl->Restore(g_pZBuffer);
			d3d_OnSurfacesRestored();
		}
	}

	return LTTRUE;
}


// 0x10033160 -- RenderStruct +0x7c, `g_pRenderCreateWorld`, 128 bytes. The
// object is 0x10 bytes; +0x0c is a uint16 seeded to 0xffff and +0x08 takes
// the request's first field. Note the store order: +0x0c before +0x08.
//
// The lightmap arm prints its warning ONCE -- the failure path clears
// g_bLightmapPages -- and NoLMPages short-circuits it entirely.
void* d3d_CreateWorld(void** pRequest)
{
	void** pWorld = (void**)d3d_Alloc(0x10);

	if (!pWorld)
		return LTNULL;

	// The request's field is read into a local ahead of the two stores: the
	// shipped code loads it before materialising 0xffff.  Read in the store
	// itself, the same bytes come out with the two loads the other way round
	// (16 bytes differ).
	void* pFirst = *pRequest;

	*(uint16*)((uint8*)pWorld + 0x0c) = 0xffff;
	pWorld[2] = pFirst;

	if (g_bNoLMPages)
	{
		sub_10021d10("Warning: NoLMPages is TRUE");
	}
	else if (g_bLightmapPages)
	{
		if (!d3d_CreateLightmapPages(pWorld))
		{
			sub_10021d60(0, "Warning: unable to create lightmap pages.  "
			              "Lightmapping disabled.");
			g_bLightmapPages = LTFALSE;
		}
	}

	return pWorld;
}
