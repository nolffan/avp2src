// *********************************************************************** //
//
//  MODULE:   BATCH_C (.CPP)
//
//  PURPOSE:  Reconstructed d3d.ren functions, listing
//            batch C (25 rows).
//
//  Compiler: Intel C/C++ 4.0, the same as the rest of src/engine/d3d.
//
//
//  WHAT THIS BATCH TURNED OUT TO BE
//  --------------------------------
//
//  The single biggest find is a TWELVE-FUNCTION FAMILY at the very front
//  of .text -- 0x10001000, 0x10001330, 0x10001670, 0x10001990, 0x10001cb0,
//  0x10001fe0, 0x10002310, 0x10002670, 0x100029e0, 0x10002d40, 0x100030a0,
//  0x10003410 -- which is the SUTHERLAND-HODGMAN POLYGON CLIPPER, one
//  out-of-line body per frustum plane, TWICE: once for a 32-byte vertex
//  and once for a 40-byte (two-texture-stage) vertex.  Three of these rows
//  are members of it (0x10001670, 0x10001cb0, 0x10002310).
//
//  Their two drivers are also on the list, in the sense that 0x1000f4b0 IS
//  on it and 0x1000aa90 is byte-for-byte the same function with the other
//  six callees -- diffing them produces ONLY call-displacement and
//  branch-displacement differences.  The driver is written out below; it
//  is what pins the clip functions' signature and the flag bit for each
//  plane.  ONE clip body (0x10001670) is written out as a probe; see the
//  long comment over it for why the other eleven are deliberately not.
//
//  THE OTHER FIND, and it costs the family: the clip bodies convert a
//  float to an integer with a BARE `fistpl` and no control-word dance --
//
//      d9 9d c8 fe ff ff   fstps  -0x138(%ebp)
//      d9 85 c8 fe ff ff   flds   -0x138(%ebp)
//      db 5d b4            fistpl -0x4c(%ebp)
//
//  -- which is /QIfist, not /O2 or /Os.  Every one of the twelve does it
//  five times (four diffuse channels plus the specular alpha).  If this
//  file is measured only at /O2 and /Os, 0x10001670 CANNOT match no
//  matter how right the rest of the body is.  Reported, not forced.
//
//  TWO ROWS ARE NOT FUNCTION STARTS.  0x100148a4 is 148 bytes INSIDE
//  MatMul, which starts at 0x10014720 and is already reconstructed in
//  src/engine/d3d/d3dbatchd.cpp; 0x100148a4 is where its last output
//  column begins.  0x1001dcd9 is inside the CMoArray resize at
//  0x1001dca0, which d3dr2a.cpp already names.  0x100514e0 is likewise
//  mid-body -- it opens with `fstps 0x1c(%esi)`.  All three are skipped.
//
//  Naming: `sub_<address>` throughout, so the address can be read
//  out of the symbol.  Types spelled `Obj<hex>` / `Ctx<hex>`
//  are invented here for structures whose real names are not known.
//
// *********************************************************************** //

#include "ltbasedefs.h"

#include <stdio.h>
#include <string.h>


// ======================================================================= //
//  SHARED DECLARATIONS
// ======================================================================= //

// ---------------------------------------------------------------------- //
//  The engine's RenderStruct at 0x1008cbcc, copied from d3dbatchc.cpp so
//  the two translation units agree.  This file needs +0x10 (run a console
//  string) and +0x18 (look a console variable up by name), both __cdecl.
//
//  +0x10 is NEW here: 0x10021e22 builds "<name> <value>" with the CRT's
//  sprintf and hands the buffer to +0x10, then re-reads the variable
//  through +0x18 -- which is only meaningful if +0x10 EXECUTES the string.
// ---------------------------------------------------------------------- //
struct RenderStructCallbacks
{
	uint8    m_Reserved00[0x10];                        // +0x00..+0x0f
	void    (*RunConsoleString)(const char* pszCommand); // +0x10
	uint8    m_Reserved14[0x04];                        // +0x14..+0x17
	void*   (*GetConsoleVariable)(const char* pszName); // +0x18
	double  (*GetVarValueFloat)(void* pVar);            // +0x1c
	char*   (*GetVarValueString)(void* pVar);           // +0x20
};

extern RenderStructCallbacks* g_pStruct;                // 0x1008cbcc


// ---------------------------------------------------------------------- //
//  The Direct3D 7 device at 0x1008d988.  Slots +0x50 SetRenderState,
//  +0x54 GetRenderState and +0x94 SetTextureStageState are already
//  established elsewhere in src/engine/d3d.
//
//  +0xc0 IS NEW, and it is IDirect3DDevice7::GetInfo -- method index 48,
//  which is 48*4 = 0xc0.  0x1002d0b0 calls it with dwDevInfoID 1 and
//  dwSize 0x2c and then prints ten of the eleven DWORDs that come back
//  under the field names of D3DDEVINFO_TEXTUREMANAGER.  The strings ARE
//  the field names, so the identification is not a guess, and the ONE
//  field it skips is at +0x04 -- dwApproxBytesDownloaded, exactly the
//  second member of that structure.
// ---------------------------------------------------------------------- //
struct D3DDevice7;

struct D3DDevice7Vtbl
{
	uint8 m_pad_00[0x50];                               // +0x00..+0x4f
	long (__stdcall* SetRenderState)(D3DDevice7*, uint32 dwState,
	                                 uint32 dwValue);   // +0x50
	long (__stdcall* GetRenderState)(D3DDevice7*, uint32 dwState,
	                                 uint32* pdwValue); // +0x54
	uint8 m_pad_58[0x3c];                               // +0x58..+0x93
	long (__stdcall* SetTextureStageState)(D3DDevice7*, uint32 dwStage,
	                                       uint32 dwState,
	                                       uint32 dwValue);  // +0x94
	uint8 m_pad_98[0x28];                               // +0x98..+0xbf
	long (__stdcall* GetInfo)(D3DDevice7*, uint32 dwDevInfoID,
	                          void* pDevInfoStruct, uint32 dwSize); // +0xc0
};

struct D3DDevice7 { D3DDevice7Vtbl* lpVtbl; };

extern D3DDevice7* g_pD3DDevice;                        // 0x1008d988

#define D3DRENDERSTATE_SRCBLEND       19                // 0x13
#define D3DRENDERSTATE_DESTBLEND      20                // 0x14
#define D3DRENDERSTATE_FOGENABLE      28                // 0x1c
#define D3DRENDERSTATE_FOGCOLOR       34                // 0x22

#define D3DBLEND_ZERO                  1
#define D3DBLEND_ONE                   2
#define D3DBLEND_SRCCOLOR              3
#define D3DBLEND_SRCALPHA              5
#define D3DBLEND_INVSRCALPHA           6

#define D3DDEVINFOID_TEXTUREMANAGER    1

struct D3DDevInfo_TextureManager                        // 0x2c bytes
{
	uint32 bThrashing;                                  // +0x00
	uint32 dwApproxBytesDownloaded;                     // +0x04
	uint32 dwNumEvicts;                                 // +0x08
	uint32 dwNumVidCreates;                             // +0x0c
	uint32 dwNumTexturesUsed;                           // +0x10
	uint32 dwNumUsedTexInVid;                           // +0x14
	uint32 dwWorkingSet;                                // +0x18
	uint32 dwWorkingSetBytes;                           // +0x1c
	uint32 dwTotalManaged;                              // +0x20
	uint32 dwTotalBytes;                                // +0x24
	uint32 dwLastPri;                                   // +0x28
};


// ---------------------------------------------------------------------- //
//  DirectDraw, as d3dblit.cpp / d3dbatcha.cpp already name the globals.
//  0x10039110 needs +0x08 Release on a surface and +0x18 CreateSurface on
//  the IDirectDraw7 -- method index 6, 6*4 = 0x18, and the four arguments
//  (this, &desc, &out, NULL) are exactly that signature.
// ---------------------------------------------------------------------- //
struct DDSurface;
struct DDSurfaceVtbl
{
	void*  m_pad_00[2];                                 // +0x00
	uint32 (__stdcall* Release)(DDSurface*);            // +0x08
};
struct DDSurface { DDSurfaceVtbl* lpVtbl; };

struct DDraw;
struct DDrawVtbl
{
	void* m_pad_00[6];                                  // +0x00..+0x17
	long  (__stdcall* CreateSurface)(DDraw*, void* pDesc,
	                                 DDSurface** ppSurface,
	                                 void* pUnkOuter);  // +0x18
};
struct DDraw { DDrawVtbl* lpVtbl; };

extern DDraw* g_pDirectDraw;                            // 0x1008bf74


// ---------------------------------------------------------------------- //
//  DDPIXELFORMAT (0x20 bytes) and just enough of DDSURFACEDESC2 to place
//  the five members 0x10039110 fills: dwSize +0x00, dwFlags +0x04,
//  dwHeight +0x08, dwWidth +0x0c, ddpfPixelFormat +0x48, ddsCaps +0x68.
//  The total is 0x7c, which is what the function stores into dwSize and
//  what its `rep stos $0x1f` zeroes.
// ---------------------------------------------------------------------- //
struct DDPixelFormat                                    // 0x20 bytes
{
	uint32 dwSize;
	uint32 dwFlags;
	uint32 dwFourCC;
	uint32 dwRGBBitCount;
	uint32 dwRBitMask;
	uint32 dwGBitMask;
	uint32 dwBBitMask;
	uint32 dwRGBAlphaBitMask;
};

struct DDSurfaceDesc2                                   // 0x7c bytes
{
	uint32        dwSize;                               // +0x00
	uint32        dwFlags;                              // +0x04
	uint32        dwHeight;                             // +0x08
	uint32        dwWidth;                              // +0x0c
	uint32        m_pad_10[0x0e];                       // +0x10..+0x47
	DDPixelFormat ddpfPixelFormat;                      // +0x48
	uint32        m_pad_68_dwCaps;                      // +0x68  ddsCaps.dwCaps
	uint32        m_pad_6c[4];                          // +0x6c..+0x7b
};

#define DDSD_CAPS                      0x00000001
#define DDSD_HEIGHT                    0x00000002
#define DDSD_WIDTH                     0x00000004
#define DDSD_PIXELFORMAT               0x00001000

#define DDSCAPS_OFFSCREENPLAIN         0x00000040
#define DDSCAPS_SYSTEMMEMORY           0x00000800


// ---------------------------------------------------------------------- //
//  The renderer's warning printers, already named in d3dblit.cpp and
//  d3dbatchd.cpp.  0x10021d10 takes a format and one argument; 0x10021d60
//  takes a level, a format and one argument.
// ---------------------------------------------------------------------- //
extern void sub_10021d10(const char* pszMsg, ...);  // 0x10021d10 (was d3d_Warn/d3d_Print; icl treats a callee named *Warn* as cold)
extern void sub_10021d60(uint32 nLevel, const char* pszMsg, ...);  // 0x10021d60 AddDebugMessage (was d3d_Warning*; icl treats a callee named *Warn*/*Error* as cold)


// ---------------------------------------------------------------------- //
//  0x1007b8d0 -- the module's one big context object, returned by value in
//  %eax and used by five of the functions below at offsets from +0x144 to
//  +0x31c.  Only the members those five touch are named.
// ---------------------------------------------------------------------- //
struct Obj1003b7f0;

struct Ctx1007b8d0
{
	uint8         m_pad_000[0x144];                     // +0x000..+0x143
	void**        m_pList144;                           // +0x144
	uint32        m_nCount148;                          // +0x148
	uint8         m_pad_14c[0x18];                      // +0x14c..+0x163
	void**        m_pList164;                           // +0x164
	uint32        m_nCount168;                          // +0x168
	uint8         m_pad_16c[0x178];                     // +0x16c..+0x2e3
	Obj1003b7f0** m_pList2e4;                           // +0x2e4
	uint32        m_nCount2e8;                          // +0x2e8
	uint8         m_pad_2ec[0x18];                      // +0x2ec..+0x303
	Obj1003b7f0** m_pSet304;                            // +0x304
	uint32        m_nSetCount308;                       // +0x308
	uint8         m_pad_30c[0x0c];                      // +0x30c..+0x317
	uint32        m_nSetMax318;                         // +0x318
	const char*   m_pszSetName31c;                      // +0x31c
};

extern Ctx1007b8d0* sub_1007b8d0();                     // 0x1007b8d0


// ---------------------------------------------------------------------- //
//  0x1008b880 -- the unbounded byte array d3dr2d.cpp already declares.
// ---------------------------------------------------------------------- //
extern uint8 g_1008b880[];


// ======================================================================= //
//  0x1002e2d0 -- 114 bytes (the row says 128; the `ret` is at 0x1002e341
//  and 0x1002e342 onward is lea-nop padding).  DECODED, every instruction.
//
//  Three memory pools are created and one counter is zeroed.  This is the
//  one function in the batch that names the OTHER two: 0x1000eef0 hands
//  0x1000a6a0 the pools at 0x1008d4c8 and 0x1008d4e4, and the element
//  sizes set here -- 0xc and 0x18 -- are EXACTLY the highest member offset
//  plus four that 0x1000eef0 and its caller touch in each (0x08 and 0x14).
//  Two independent functions agreeing on two sizes is what pins both.
//
//  0x1007fb20 takes (pool, elementSize, count); 0x1007fb70 takes a fourth
//  argument, so it is the growable form.  Neither is renamed here.
// ======================================================================= //

extern void sub_1001ead0();                             // 0x1001ead0
extern void sub_1007fb20(void* pPool, uint32 nElemSize, uint32 nCount);
extern void sub_1007fb70(void* pPool, uint32 nElemSize, uint32 nCount,
                         uint32 nGrow);

extern uint8  g_1008d500[];                             // 0x1008d500
extern uint8  g_1008d4c8[];                             // 0x1008d4c8
extern uint8  g_1008d4e4[];                             // 0x1008d4e4
extern uint32 g_1008d524;                               // 0x1008d524

void sub_1002e2d0()
{
	sub_1001ead0();

	sub_1007fb20(g_1008d500, 0x100, 0x10);
	sub_1007fb20(g_1008d4c8, 0x0c, 0x64);
	sub_1007fb70(g_1008d4e4, 0x18, 0x80, 0x300);

	g_1008d524 = 0;
}


// 0x1000eef0 -- moved to d3dr3c_os.cpp (its shipped tail says /Os).


// 0x10040850 -- moved to d3dr3c_os.cpp (its shipped tail says /Os).


// 0x10021dc0 -- moved to w34_10021dc0_ossf.cpp (/Os /Qsfalign16: no
// stack-realigning entry, as shipped).


// ======================================================================= //
//  0x10043d70 -- 152 bytes (the row says 160; the last `ret` is at
//  0x10043e06 and the rest is lea-nop padding).  DECODED.
//
//  Hand every entry of the +0x164/+0x168 array to the ring buffer at
//  0x1008ef3c together with the callback 0x10043a80.
//
//  The call is `mov 0x1008ef3c,%ecx ; call 0x100500a0` -- a plain MSVC
//  __thiscall with `this` LOADED FROM a global pointer, and the arguments
//  on the stack in the order (item, callback).  0x100688e0 below makes the
//  identical call with a different callback, which is what identifies the
//  argument order.
//
//  TWO GUARDS FROM ONE `test`.  `test %eax,%eax ; je 0x10043df8` is the
//  source's `if (count == 0) return;`, and `jbe 0x10043de9` five bytes
//  later reuses THE SAME FLAGS for the `for` loop's zero-trip guard --
//  a second, unreachable exit block with its own epilogue.  Two separate
//  source-level exits, so the `if` is written out longhand.
// ======================================================================= //

class C100500a0
{
public:
	void sub_100500a0(void* pItem, void (*pfn)(void*));  // 0x100500a0
};

extern C100500a0* g_p1008ef3c;                          // 0x1008ef3c
extern uint32     g_100893f8;                           // 0x100893f8
extern void       sub_10043a80(void* pItem);            // 0x10043a80

void sub_10043d70()
{
	Ctx1007b8d0* pCtx;
	uint32       i;

	if (!g_100893f8)
		return;

	pCtx = sub_1007b8d0();

	if (pCtx->m_nCount168 == 0)
		return;

	for (i = 0; i < pCtx->m_nCount168; ++i)
		g_p1008ef3c->sub_100500a0(pCtx->m_pList164[i], sub_10043a80);
}


// ======================================================================= //
//  0x100688e0 -- 38 bytes.  DECODED, every instruction.
//
//  The 367 the listing gives is the distance to the NEXT next function:
//  0x100688e0's `ret` is at 0x10068905 and 0x10068906..0x1006890f is
//  lea-nop padding, so the body really is 38 bytes.
//
//  The same __thiscall on 0x1008ef3c that 0x10043d70 makes, with a
//  different callback and with the item coming from the SECOND parameter.
//  The first parameter is dead -- and the sibling at 0x10043d70 shows why
//  the argument order is (item, callback) and not the other way round.
// ======================================================================= //

extern void sub_10066f00(void* pItem);                  // 0x10066f00

void sub_100688e0(uint32 nUnused, void* pItem)
{
	g_p1008ef3c->sub_100500a0(pItem, sub_10066f00);
}


// ======================================================================= //
//  0x10042da0 -- 38 bytes (then lea-nops to 0x10042dd0).  The same body
//  as 0x100688e0 above with 0x10040e10 (big40e10.cpp) as the callback:
//  `movl $0x10040e10,0x4(%esp)` at 0x10042db5 is the only reference to
//  that function's address in the image.  Its own address is handed to
//  0x1007b8e0 by d3dr3a.cpp's walk.
//
//  The image carries both copies, so both are written; this one used to
//  be linked as an /ALTERNATENAME onto 0x100688e0, which resolved the
//  reference but left the shipped 0x10042da0 with nothing to score.
//  0x10040e10 takes (pCtx, pObject) -- the dispatcher passes 0x1008b880
//  and the item -- and is handed over as the dispatcher's one-argument
//  callback type, as d3dr3c's other callbacks are.
// ======================================================================= //

class ObjLine40e10;
extern void sub_10040e10(void* pCtx1008b880, ObjLine40e10* pObj); // 0x10040e10

void sub_10042da0(uint32 nUnused, void* pItem)
{
	g_p1008ef3c->sub_100500a0(pItem, (void (*)(void*))sub_10040e10);
}


// ======================================================================= //
//  0x1005a490 -- 178 bytes (the row says 192; the `ret` is at 0x1005a541).
//  DECODED, every instruction.  TWO callers, 0x1005a9a5 and 0x10061aab,
//  and both pass five arguments and clean 0x10 -- five out-parameters
//  around one object.
//
//  WHAT IT IS: pick the alpha-blend mode for a surface and, with it, the
//  fog colour that mode needs.  The two GetRenderState calls are
//  D3DRENDERSTATE_FOGCOLOR (0x22) and D3DRENDERSTATE_FOGENABLE (0x1c) --
//  the current values are read out through the caller's pointers, so the
//  caller can restore them, and then two of them are OVERWRITTEN:
//
//    +0x8c bit 0x04 -> ONE / ONE          (additive)  and fog colour 0
//    +0x8c bit 0x08 -> ZERO / SRCCOLOR    (modulate)  and fog colour ~0
//    otherwise      -> SRCALPHA / INVSRCALPHA         fog colour untouched
//
//  Black fog under an additive blend and white fog under a modulating one
//  is the standard fix for fog on a non-lerp blend, and it is what makes
//  the fifth out-parameter identifiable as the fog colour.
//
//  The `mov $0x2,%eax ; mov %eax,(%ecx) ; mov %eax,(%edx)` in the first
//  arm -- one register feeding two stores of the same constant -- is why
//  ONE/ONE is written as two separate assignments of the same value.
// ======================================================================= //

struct Obj1005a490
{
	uint8  m_pad_00[0x88];                              // +0x00..+0x87
	uint32 m_nFlags88;                                  // +0x88
	uint32 m_nFlags8c;                                  // +0x8c
};

void sub_1005a490(Obj1005a490* pObj, uint32* pSrcBlend, uint32* pDestBlend,
                  uint32* pFogEnable, uint32* pFogColor)
{
	g_pD3DDevice->lpVtbl->GetRenderState(g_pD3DDevice,
	                                     D3DRENDERSTATE_FOGCOLOR, pFogColor);
	g_pD3DDevice->lpVtbl->GetRenderState(g_pD3DDevice,
	                                     D3DRENDERSTATE_FOGENABLE, pFogEnable);

	if (pObj->m_nFlags88 & 0x80)
		*pFogEnable = 0;

	if (pObj->m_nFlags8c & 0x04)
	{
		*pSrcBlend  = D3DBLEND_ONE;
		*pDestBlend = D3DBLEND_ONE;
		*pFogColor  = 0x00000000;
	}
	else if (pObj->m_nFlags8c & 0x08)
	{
		*pSrcBlend  = D3DBLEND_ZERO;
		*pDestBlend = D3DBLEND_SRCCOLOR;
		*pFogColor  = 0xffffffff;
	}
	else
	{
		*pSrcBlend  = D3DBLEND_SRCALPHA;
		*pDestBlend = D3DBLEND_INVSRCALPHA;
	}
}


// 0x1003b7f0 -- moved to d3dr3c_os.cpp (its shipped tail says /Os).

struct Obj1003b7f0;


// ======================================================================= //
//  0x1002d0b0 -- 257 bytes.  DECODED, every instruction.
//
//  `ShowTexInfo` -- ask the device for D3DDEVINFO_TEXTUREMANAGER and print
//  ten of its eleven DWORDs.  The strings ARE the field names, which is
//  what identifies both the structure and the +0xc0 vtable slot, and the
//  skipped field is +0x04, dwApproxBytesDownloaded.
//
//  The header line "ShowTexInfo ---..." is printed with ONE argument, and
//  each row is printed with the format and one DWORD, all through
//  0x10021d10 which d3dbatchd.cpp already names as the one-argument
//  warning printer.
// ======================================================================= //

void sub_1002d0b0()
{
	D3DDevInfo_TextureManager info;

	g_pD3DDevice->lpVtbl->GetInfo(g_pD3DDevice, D3DDEVINFOID_TEXTUREMANAGER,
	                              &info, sizeof(info));

	sub_10021d10("ShowTexInfo ----------------------------");
	sub_10021d10("bThrashing        - %d", info.bThrashing);
	sub_10021d10("dwNumEvicts       - %d", info.dwNumEvicts);
	sub_10021d10("dwNumVidCreates   - %d", info.dwNumVidCreates);
	sub_10021d10("dwNumTexturesUsed - %d", info.dwNumTexturesUsed);
	sub_10021d10("dwNumUsedTexInVid - %d", info.dwNumUsedTexInVid);
	sub_10021d10("dwWorkingSet      - %d", info.dwWorkingSet);
	sub_10021d10("dwWorkingSetBytes - %d", info.dwWorkingSetBytes);
	sub_10021d10("dwTotalManaged    - %d", info.dwTotalManaged);
	sub_10021d10("dwTotalBytes      - %d", info.dwTotalBytes);
	sub_10021d10("dwLastPri         - %d", info.dwLastPri);
}


// ======================================================================= //
//  0x10039110 -- 255 bytes.  DECODED, every instruction.
//
//  Release the cached 32x32 system-memory surface at 0x1008da7c and make a
//  new one in the pixel format carried at +0x0c of the object at
//  0x1008e6c4.
//
//  0x7c into dwSize and `rep stos` of 0x1f dwords is sizeof(DDSURFACEDESC2)
//  twice over, which is what fixes the structure; 0x1007 into dwFlags is
//  DDSD_CAPS|DDSD_HEIGHT|DDSD_WIDTH|DDSD_PIXELFORMAT and names the four
//  members that are filled; 0x840 into +0x68 is
//  DDSCAPS_SYSTEMMEMORY|DDSCAPS_OFFSCREENPLAIN.  The eight dword moves
//  from +0x0c of 0x1008e6c4 into +0x48 of the descriptor are one
//  DDPIXELFORMAT copied whole, unrolled rather than `rep movsl`.
//
//  THREE separate return blocks, each with its own epilogue, and the last
//  branch is `test %eax,%eax ; je <return TRUE>` -- the shape of
//  `if (hr != DD_OK) return LTFALSE;` followed by `return LTTRUE;`, not of
//  `return hr == DD_OK`.
//
//  The reload of 0x1008da7c between the null test and the Release: two
//  plain source reads are CSE'd into one load; the shipped second load is
//  a volatile read into the pointer the Release goes through.
//
//  IDENTICAL with three more levers (each measured alone):
//    * the pixel format is copied with memcpy, which icl (/O2, intrinsics
//      on) expands through `lea -0x38(%ebp),%edi` and eight moves via
//      %edi; a struct assignment addresses the frame directly and moves
//      the `add $0xfffffff0,%esp` above the copy;
//    * the scalar stores do not come out in source order: written size,
//      flags, height, width, caps they are emitted size, width, flags,
//      height, caps, as shipped; written width before height, the two
//      stores swap.
// ======================================================================= //

struct Fmt10039110
{
	uint8         m_pad_00[0x0c];                       // +0x00..+0x0b
	DDPixelFormat m_pf;                                 // +0x0c
};

extern DDSurface*   g_1008da7c;                         // 0x1008da7c
extern Fmt10039110* g_1008e6c4;                         // 0x1008e6c4

LTBOOL sub_10039110()
{
	DDSurfaceDesc2 ddsd;
	Fmt10039110*   pFmt;

	if (g_1008da7c)
	{
		DDSurface* pSurface = *(DDSurface* volatile*)&g_1008da7c;

		pSurface->lpVtbl->Release(pSurface);
		g_1008da7c = LTNULL;
	}

	pFmt = g_1008e6c4;
	if (!pFmt)
		return LTFALSE;

	memset(&ddsd, 0, sizeof(ddsd));

	ddsd.dwSize           = sizeof(ddsd);
	ddsd.dwFlags          = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH |
	                        DDSD_PIXELFORMAT;
	ddsd.dwHeight         = 32;
	ddsd.dwWidth          = 32;
	ddsd.m_pad_68_dwCaps  = DDSCAPS_SYSTEMMEMORY | DDSCAPS_OFFSCREENPLAIN;
	memcpy(&ddsd.ddpfPixelFormat, &pFmt->m_pf, sizeof(DDPixelFormat));

	if (g_pDirectDraw->lpVtbl->CreateSurface(g_pDirectDraw, &ddsd,
	                                         &g_1008da7c, LTNULL) != 0)
	{
		return LTFALSE;
	}

	return LTTRUE;
}


// ======================================================================= //
//  0x100438f0 -- 307 bytes (the `ret` is at 0x10043a22).  DECODED, every
//  instruction.
//
//  Draw one pass over the +0x144/+0x148 array with alpha blending on:
//  SRCALPHA/INVSRCALPHA, stage 1 colour op ARG1 (state 1 value 1), colour
//  arg1 = 1 with state 2 value 2, alpha op with state 3 value 1, then hand
//  the array to 0x1007b8e0 with the callback 0x10015c70, then put stage 1
//  back to (1,1,1).
//
//  `add $0x144,%ebx` BEFORE the null test on +0x148 is what says +0x144
//  is the container and +0x148 its count: the `this` handed to 0x1007b8e0
//  is the container's address, and d3dbatchc.cpp already reconstructs
//  0x1007b8e0 as `C1007b8e0::sub_1007b8e0(pContext, pfn)` with the element
//  array at +0x00 and the count at +0x04.
//
//  The three SetTextureStageState values are read literally; no D3D
//  TSS symbol is asserted for them because stage/state/value are all
//  small integers here and guessing the enum would not add evidence.
// ======================================================================= //

struct Ctx1007b8e0;
typedef void (*Fn1007b8e0)(Ctx1007b8e0* pContext, void* pElem);

class C1007b8e0
{
public:
	void sub_1007b8e0(Ctx1007b8e0* pContext, Fn1007b8e0 pfn);  // 0x1007b8e0
};

extern void sub_10015c70(Ctx1007b8e0* pContext, void* pElem);  // 0x10015c70

void sub_100438f0()
{
	Ctx1007b8d0* pCtx;
	C1007b8e0*   pArray;

	if (!g_100893f8)
		return;

	pCtx   = sub_1007b8d0();
	pArray = (C1007b8e0*)&pCtx->m_pList144;

	if (pCtx->m_nCount148 == 0)
		return;

	g_pD3DDevice->lpVtbl->SetRenderState(g_pD3DDevice,
	                                     D3DRENDERSTATE_SRCBLEND,
	                                     D3DBLEND_SRCALPHA);
	g_pD3DDevice->lpVtbl->SetRenderState(g_pD3DDevice,
	                                     D3DRENDERSTATE_DESTBLEND,
	                                     D3DBLEND_INVSRCALPHA);

	g_pD3DDevice->lpVtbl->SetTextureStageState(g_pD3DDevice, 1, 1, 1);
	g_pD3DDevice->lpVtbl->SetTextureStageState(g_pD3DDevice, 1, 2, 2);
	g_pD3DDevice->lpVtbl->SetTextureStageState(g_pD3DDevice, 1, 3, 1);

	pArray->sub_1007b8e0((Ctx1007b8e0*)g_1008b880, sub_10015c70);

	g_pD3DDevice->lpVtbl->SetTextureStageState(g_pD3DDevice, 1, 1, 1);
}


// ======================================================================= //
//  0x1005fac0 -- 173 bytes (the `ret` is at 0x1005fb6c).  DECODED as
//  arithmetic; the SOURCE SHAPE is inferred.
//
//  0x1005a550 is the SAME 173 bytes with a different stride and different
//  store offsets, so this decode covers two rows' worth of the image even
//  though only one of them is on the list.
//
//  Transform an array of vertices in place by a 4x4 matrix WITH the
//  homogeneous divide -- ltmatrix.h's MatVMul_H, one vertex at a time,
//  writing back over the source.  Every operand is read off the bytes:
//
//     w    = m[3][0]*x + m[3][1]*y + m[3][2]*z + m[3][3]
//     oow  = 1.0f / w              (`fdivrs` against a 1.0f at 0x100aac10)
//     x'   = (m[0][0]*x + m[0][1]*y + m[0][2]*z + m[0][3]) * oow
//     y'   = (m[1][0]*x + ...) * oow
//     z'   = (m[2][0]*x + ...) * oow
//
//  and every one of the four-term sums is associated from the LAST term
//  backwards, which is this compiler's habit and the reason the `flds` for
//  the m[..][2] term comes first.
//
//  The loop is a counted countdown (`add $0x20,%edx ; dec %ecx ; jne`)
//  with the y and z stores rotated into the NEXT iteration's head, which
//  is why the epilogue repeats `fstps -0x18(%edx) ; fstps -0x1c(%edx)`.
//
//  THE ONE COIN FLIP for this function is whether the source spells the
//  body out or calls the header's MatVMul_InPlace_H.  It is spelled out
//  here because the header's version writes into an `LTVector temp` and
//  then does `*pSrc = temp`, and there is no temp in the bytes -- the
//  three results are stored straight through the vertex pointer.  If it
//  misses, the alternative is
//      MatVMul_InPlace_H((LTMatrix*)pMat, (LTVector*)pVert);
//  inside the same loop, with ltmatrix.h included.
// ======================================================================= //

struct Mat1005fac0
{
	float m[4][4];
};

struct Vert1005fac0                                     // 32 bytes
{
	float  x, y, z;
	uint32 m_pad_0c[5];
};

//  THE LOOP SPELLING is what the bytes pin: the count itself is the
//  countdown register (`mov 0xc(%esp),%ecx ; test %ecx,%ecx ; je`, then
//  `dec %ecx ; jne`) and the vertex pointer walks separately.  An index
//  loop gives `cmp`/`jb` on an end pointer; `while (nCount--)` copies the
//  count out of %eax first; `for (; nCount; --nCount, ++pV)` is identical.
void sub_1005fac0(Vert1005fac0* pVerts, uint32 nCount, Mat1005fac0* pMat)
{
	Vert1005fac0* pV = pVerts;

	for (; nCount; --nCount, ++pV)
	{

		float oow = 1.0f / (pMat->m[3][0]*pV->x + pMat->m[3][1]*pV->y +
		                    pMat->m[3][2]*pV->z + pMat->m[3][3]);

		float fX = (pMat->m[0][0]*pV->x + pMat->m[0][1]*pV->y +
		            pMat->m[0][2]*pV->z + pMat->m[0][3]) * oow;
		float fY = (pMat->m[1][0]*pV->x + pMat->m[1][1]*pV->y +
		            pMat->m[1][2]*pV->z + pMat->m[1][3]) * oow;
		float fZ = (pMat->m[2][0]*pV->x + pMat->m[2][1]*pV->y +
		            pMat->m[2][2]*pV->z + pMat->m[2][3]) * oow;

		pV->x = fX;
		pV->y = fY;
		pV->z = fZ;
	}
}


// ======================================================================= //
//  THE FRUSTUM CLIPPER
//
//  0x1000f4b0 -- 374 bytes.  DECODED, every instruction.
//
//  This is the driver for the twelve-body clip family, and 0x1000aa90 is
//  the SAME FUNCTION for the 40-byte vertex: diffing 0x1000f4b0 against
//  0x1000aa90 instruction for instruction produces only six call
//  displacements and the branch displacements, nothing else.  So this
//  decode is worth two rows of the image.
//
//  The six planes and their flag bits, from the six guarded call sites:
//
//     bit 0x01  ->  0x10001000 / 0x10002310    near   (z >= g_1008b908)
//     bit 0x04  ->  0x10001330 / 0x10002670    left   (x > -z)
//     bit 0x08  ->  0x10001670 / 0x100029e0    top    (y <  z)
//     bit 0x10  ->  0x10001990 / 0x10002d40    right  (x <  z)
//     bit 0x20  ->  0x10001cb0 / 0x100030a0    bottom (y > -z)
//     bit 0x02  ->  0x10001fe0 / 0x10003410    far    (z <= g_1008b910)
//
//  and the order the driver applies them in is near, left, top, right,
//  bottom, far -- NOT the numeric order of the bits, which is what makes
//  the far plane's 0x02 test come last.
//
//  Each call passes FOUR arguments: a scratch dword that the callee never
//  reads, &pVerts, &nVerts, &pOut.  The scratch dwords are six DISTINCT
//  consecutive stack slots (0x1c, 0x20, 0x24, 0x28, 0x2c, 0x30), one per
//  plane, so they are written here as an array indexed by the plane and
//  not as one reused local.
//
//  `and $0x1,%ebx` in the guard MUTATES the flag word -- after it, only
//  bit 0 can survive -- so 0x1008d7a0 is a "clip against the near plane
//  only" switch and the source really does assign back into the parameter.
// ======================================================================= //

// 0x1000f4b0 itself is in d3dr3c_os.cpp (its shipped tail says /Os).
// 0x10001670 (top plane, in = y < z, crossing y = z), once written here as
// a probe, is in w44_10001670.cpp: byte-identical there with the shape
// w1_10001000.cpp measured for the other five 32-byte clippers (RoundF,
// the inlined vertex lerp, a ClipEdge inline taking the LTVector at +0,
// int32 nInside compared uncast).  The /QIfist guess above was wrong:
// the bare fistpl is RoundF's inline asm.
