// *********************************************************************** //
//
//  MODULE:   RENDERINIT (.CPP)
//
//  PURPOSE:  lithtech.exe 0x004623d0..0x00462936 -- loading, initialising
//            and shutting down the render DLL.  This is the engine side of
//            the RenderStruct contract at 0x004b4358: r_InitRender resolves
//            d3d.ren's RenderDLLSetup, lets it fill the table, calls the
//            table's Init, and publishes the mode; r_TermRender undoes all
//            of it.
//
//  WHAT PINS THE FILE, in one place:
//
//    * 0x004624a8  GetProcAddress(hLib, "RenderDLLSetup")
//    * 0x004624f1  push $0x4b4358 ; call *%eax     -> RenderStruct is the
//                  global at 0x004b4358 and this is the call that fills it.
//    * 0x004624ff  pMode->m_Width / m_Height (RMode +0x204 / +0x208) are
//                  copied into RenderStruct +0x3c / +0x40.
//    * 0x0046251f  call *0x4b43c4  == RenderStruct +0x6c, the renderer's
//                  Init, handed a 0x21c-byte struct built on this frame.
//    * d3d.ren 0x100215a0 (the function RenderDLLSetup stores in +0x6c)
//                  opens with `movl $0xd5d,(%edi)` on that same struct, so
//                  the client's `cmpl $0xd5d` at 0x00462530 is a version
//                  handshake and the 0x21c struct's first dword is an OUT
//                  parameter.  Both halves of the contract were read.
//    * The three OnEvent codes 4 / 5 / 3 hit IClientShell vtable slot 0x4c,
//      and IClientShell::OnEvent is declaration slot 19 == 0x4c in
//      src/LT2/sdk/inc/iclientshell.h.  ltcodes.h then names the codes exactly:
//      LTEVENT_RENDERTERM 4, LTEVENT_RENDERALMOSTINITTED 5,
//      LTEVENT_RENDERINIT 3.
//    * The three error codes are LT_ERRORLOADINGRENDERDLL 51 (0x33),
//      LT_UNABLETORESTOREVIDEO 48 (0x30) and LT_ERRORBINDINGWORLD 43 (0x2b);
//      the last is returned exactly when CClientMgr::CreateRenderWorld
//      (0x00410510, already reconstructed in clientmgr2.cpp) fails, which is
//      "Error binding world to renderer" word for word.
//
//  THE RENDERSTRUCT LAYOUT is the point of this file and is written out in
//  full below.  Every offset in it was read off a store or a load; nothing
//  in the layout is inferred from shape.  See the block comment on the
//  class.
//
// *********************************************************************** //

#include <windows.h>
#include <stdio.h>
#include <string.h>

#include "ltbasedefs.h"
#include "ltlink.h"
#include "ltrect.h"
#include "cluglob.h"                // g_CluClientGlob, 0x004b0ac0
#include "ltcodes.h"
#include "iclientshell.h"
#include "iltcommon.h"         // ILTCommon::GetAttachmentTransform (0x00462080)


// ======================================================================= //
//  THE ENGINE <-> RENDERER CONTRACT
// ======================================================================= //
//
//  RenderStruct is the private engine<->renderer boundary.  ONE instance
//  exists, the global at 0x004b4358, and it is 0x150 bytes: 0x00462110
//  clears 0x54 dwords starting there (0x4b4358 + 0x150 == 0x4b44a8, which is
//  the render DLL's HMODULE and the first thing past the struct).
//
//  It is in three parts.
//
//   1. +0x00 .. +0x38   FIFTEEN function pointers the ENGINE installs, for
//                       the renderer to call back into the engine with.
//                       0x00462110 writes every one of them from a literal;
//                       that function IS the layout, and the bodies it
//                       points at name the slots (see the table).
//   2. +0x3c .. +0x68   engine-owned data: the mode, the "renderer is up"
//                       flag, and five statistics counters.
//   3. +0x6c .. +0xfc   THIRTY-SEVEN function pointers the RENDERER
//                       installs.  d3d.ren's RenderDLLSetup (0x10021410)
//                       writes exactly these 37 and nothing else.
//   4. +0x100.. +0x14c  engine-owned data again: the global light state and
//                       the two pan-info records.
//
//  ---- part 1, the callbacks the ENGINE installs (0x00462110) ----------- //
//
//   off   stub       what the stub does (read off the stub's own body)
//   ----  ---------  -----------------------------------------------------
//   +0x00 0x00462080 create a client-side object mirroring a server one:
//                    calls a vtable +0x18 on g_pClientMgr+0x4c0's +0x08,
//                    then 0x00420520 / 0x004207a0 / 0x00420880 (set pos and
//                    rotation) -- the last two are clientmgr.h's already.
//   +0x04 0x00462240 build a zeroed CClientFileRef-shaped 0xc-byte record
//                    around the argument and hand it to 0x0041ff90, which
//                    lttexmod.cpp already calls "resolve one CClientFileRef
//                    to a SharedTexture".  So: LoadTexture(name).
//   +0x08 0x00462270 SharedTexture -> TextureData, loading through
//                    0x00461e30 if m_pEngineData (+0x08) is still null.
//                    NOTE the out-parameter is the caller's own argument
//                    slot -- the frame trick.
//   +0x0c 0x00462290 release that TextureData: 0x00461dc0 unless the data
//                    carries TDFLAG_TEXMOD (bit 0x40 of TextureData+0x18).
//                    This is 0x00462900 minus the renderer call and minus
//                    the write-back.
//   +0x10 0x004622b0 print one string to the client console
//                    (0x0041abe0 on g_ClientConsoleState at 0x004b2f00).
//                    Byte-identical to 0x0041e930 and NOT folded with it,
//                    which is worth knowing: /OPT:ICF did not fold these.
//   +0x14 0x004622d0 the same, with printf formatting: _vsnprintf into a
//                    0x12b-byte buffer, then 0x0041b8c0.
//   +0x18 0x00462310 find a console variable by name (0x0041b3e0 on
//                    g_ClientConsoleState).  CONFIRMED FROM THE OTHER SIDE:
//                    d3d.ren 0x100215c8 calls +0x18 with "windowed" and
//                    0x1002160b with "SysMem".
//   +0x1c 0x00462330 that variable's float value (var+0x18), 0.0f if null.
//                    d3d.ren pairs it with the "SysMem" lookup.
//   +0x20 0x00462350 that variable's string value (var+0x14), null if null.
//                    d3d.ren pairs it with the "windowed" lookup.
//   +0x24 0x00404470 a bare `ret`.  An empty function, folded with every
//                    other empty function in the image, so it cannot be
//                    named from its body.
//   +0x28 0x00462380 thiscall slot 0 of the vtable on g_pClientMgr+0x1ec.
//   +0x2c 0x00462360 thiscall slot 1 of that same vtable.
//   +0x30 0x004623a0 thiscall 0x00411450 on g_pClientMgr.
//   +0x34 0x004623b0 dalloc  (0x00428fd0)
//   +0x38 0x004623c0 dfree   (0x00429080)
//
//  ---- part 2 and 4, the data --------------------------------------- //
//
//   +0x3c  uint32   m_Width          <- RMode.m_Width, at 0x00462507
//   +0x40  uint32   m_Height         <- RMode.m_Height, at 0x0046251a
//                                    0x0040eeb0 reads both back out of the
//                                    pointer as 0x3c(%eax)/0x40(%eax), which
//                                    is the independent proof of the base.
//   +0x44  uint32   m_bInited        1 after a good init (0x004625c1), 0 on
//                                    every failure and in r_TermRender.
//                                    25 absolute reads across the client --
//                                    this is the LT_NOTINITIALIZED gate.
//   +0x48..+0x54                     never referenced by the client.
//   +0x58..+0x68  five uint32 counters.  0x0042bedd..0x0042bf5a registers
//                                    their ADDRESSES in the console's
//                                    counter table at 0x004b3148 next to the
//                                    strings "      TagVisibleLeaves",
//                                    "      FlushObjectQueues",
//                                    "        Models", "        WorldModels",
//                                    "        Translucent" -- in that order,
//                                    +0x58 first.
//   +0x104 SharedTexture*            the environment map; 0x0041e0fd stores
//                                    the result of 0x0041ff90 into it right
//                                    before printing "Environment map set
//                                    to %s."
//   +0x108 PanInfo[2]                clientmgr.h's g_GlobalPanInfo, 0x28
//                                    bytes, which ends at +0x130 exactly.
//   +0x130 LTVector m_GlobalLightDir getter 0x004071d0 (`ret $4`), setter
//                                    0x00407292 (`ret $0xc`, three floats).
//                                    0x00462110 defaults it to
//                                    (0,-2,-1)/sqrt(5) -- the fldl of the
//                                    literal 5.0 and the fsqrt at 0x00462125
//                                    are that normalisation.
//   +0x13c LTVector m_GlobalLightColor  getter 0x004072c0, setter 0x004072f0
//   +0x148 uint32                    read as a float via fildll at
//                                    0x00407338; zeroed by 0x00462110.
//   +0x14c uint32                    used at 0x00411a0e / 0x00411bd3 /
//                                    0x00411c6b; zeroed by 0x00462110.
//
//  ---- part 3, the renderer's own 37 (d3d.ren 0x10021410) ------------ //
//
//   off    d3d.ren     what the CLIENT does with it
//   -----  ----------  --------------------------------------------------
//   +0x6c  0x100215a0  Init(RenderStructInit*) -> 0 on success; writes 0xd5d
//                      into the struct's first dword.  Called ONCE, here.
//   +0x70  0x100211d0  Term(), no arguments.  Called once, in r_TermRender,
//                      only when the DLL handle is non-null.
//   +0x74  0x10039fe0  (SharedTexture*, LTBOOL) -- 0x004628e0; the client
//                      calls it after changing a texture's contents and for
//                      every texture whose m_pRenderData is null, so it is
//                      the bind/upload entry.
//   +0x78  0x1003aac0  (SharedTexture*) -- 0x00462900; the unbind, called
//                      immediately before the engine frees the data.
//   +0x7c  0x10033160  CreateWorld            (clientmgr.h)
//   +0x80  0x10032c00  DestroyWorld           (clientmgr.h)
//   +0x84  0x100262f0  ClearScreen            (clientmgr.h)
//   +0x88  0x10032e50  Start3D                (clientmgr.h)
//   +0x8c  0x10032ea0  End3D                  (clientmgr.h)
//   +0x90  0x10032f20  IsIn3D                 (clientmgr.h)
//   +0x94  0x10034930  StartOptimized2D       (clientmgr.h)
//   +0x98  0x10034b00  EndOptimized2D         (clientmgr.h)
//   +0x9c  0x10034c60  a no-argument getter, never called       [d3d.cpp]
//   +0xa0  0x10034c70  SetOptimized2DBlend(LTSurfaceBlend)     [d3d.cpp]
//                      0x00409262 turns the answer into a bool with
//                      neg/sbb/inc, i.e. tests it `== 0`.
//   +0xa4  0x10034ed0  GetOptimized2DBlend -- dead              [d3d.cpp]
//   +0xa8  0x10034eb0  SetOptimized2DColor                   [clientmgr.h]
//   +0xac  0x10034ef0  GetOptimized2DColor -- dead              [d3d.cpp]
//   +0xb0  0x1002d1c0  RenderScene(SceneDesc*)                  [d3d.cpp]
//   +0xb4  0x10032d00  two dwords, answer ignored (0x0041de93). The call
//                      sits behind a null test on &g_RenderStruct that the
//                      compiler folded to a constant, so the source reads
//                      the struct through a pointer that is its address.
//   +0xb8  0x10020fd0  GetRenderObject(const char*)             [d3d.cpp]
//   +0xbc  0x100371e0  FlipScreen(uint32 dwFlags)               [d3d.cpp]
//                      called from CClientMgr::FlipScreen (0x00410930,
//                      clientmgr.h) and only while m_bInited.
//   +0xc0  0x10032e40
//   +0xc4  0x10036df0  called once at renderer install           [d3d.cpp]
//   +0xc8  0x100367b0
//   +0xcc  0x100368e0
//   +0xd0  0x10036920
//   +0xd4  0x10036950  LockSurface(pRenderData)                 [d3d.cpp]
//   +0xd8  0x100369a0  UnlockSurface(pRenderData)  20 call sites [d3d.cpp]
//   +0xdc  0x10034580  OptimizeSurface                       [clientmgr.h]
//   +0xe0  0x10034510  UnoptimizeSurface                     [clientmgr.h]
//   +0xe4  0x100369d0  LockScreen(l,t,r,b,&pixels,&pitch)       [d3d.cpp]
//   +0xe8  0x10036a90  UnlockScreen()                           [d3d.cpp]
//   +0xec  0x10036cf0  BlitToScreen(RenderBlitRequest*)         [d3d.cpp]
//   +0xf0  0x10036da0  BlitWarpedToScreen(RenderBlitRequest*)   [d3d.cpp]
//   +0xf4  0x10036e40  one pointer to a stack local (0x00403971)
//   +0xf8  0x10021e50  no arguments; CClientMgr::SetGlobals (0x00410540)
//                      calls it if m_bInited and the pointer is non-null.
//                      d3d.ren also calls it from its own Init.
//   +0xfc  0x10036ae0  BlitFromScreen(RenderBlitRequest*)       [d3d.cpp]
//
// ======================================================================= //


// lttexmod.cpp owns the real ones; these are the two fields this file
// reads, at the offsets lttexmod.cpp already proved.
class SharedTexture;

class TextureData
{
public:
	uint8  m_pad_00[0x18];
	uint32 m_Flags;                                             // 0x18
	uint8  m_pad_1c[0x2c - 0x1c];
	// The DTX header's command string, which 0x00461e30 parses for
	// "DetailTex" / "EnvMap" / "EnvMapAlpha".
	char   m_CommandString[0xac - 0x2c];                        // 0x2c
	// The rest is what 0x00461dc0 (r_UnloadSystemTexture) touches: the
	// link on g_TextureDataList, the byte count it adds to that list's
	// total, the owning texture and the pixel buffer.
	LTLink         m_Link;                                      // 0xac
	uint32         m_AllocSize;                                 // 0xb8
	uint8          m_pad_bc[0xc0 - 0xbc];
	SharedTexture* m_pSharedTexture;                            // 0xc0
	uint8          m_pad_c4[0xc8 - 0xc4];
	void*          m_pDataBuffer;                               // 0xc8
};

#define TDFLAG_TEXMOD 0x40

class CRenderFileIdent;

class SharedTexture
{
public:
	uint8             m_pad_00[0x08];
	TextureData*      m_pEngineData;                            // 0x08
	// What 0x00461e30 reads and writes: the file the texture comes from,
	// and the second texture its command string links to plus how
	// (0 detail texture, 1 environment map, 2 environment map in alpha).
	uint8             m_pad_0c[0x24 - 0x0c];
	CRenderFileIdent* m_pFile;                                  // 0x24
	SharedTexture*    m_pLinkedTexture;                         // 0x28
	uint32            m_nLinkType;                              // 0x2c
};


// The 0x21c-byte block r_InitRender builds on its frame and hands to the
// renderer's Init.  Read straight off the frame: the memset at 0x0046242b
// covers 0x87 dwords from esp+0xd8, the struct assignment at 0x0046245a
// copies 0x85 dwords (sizeof(RMode)) into esp+0xdc, and the HWND goes to
// esp+0x2f0.  The pointer handed to Init is esp+0xd8 and the dword the
// client checks afterwards is at esp+0xd8 too, so the version word is
// FIRST, not last.
struct RenderStructInit
{
	// OUT.  d3d.ren stores 0xd5d here as the first instruction of its Init
	// (0x100215b0, `movl $0xd5d,(%edi)`); the client refuses the renderer
	// if it comes back as anything else.
	uint32 m_nVersion;                                          // 0x000
	RMode  m_Mode;                                              // 0x004
	HWND   m_hWnd;                                              // 0x218
};

#define RENDERSTRUCT_VERSION 0xd5d


// The table itself.  Only the members this file touches are typed; the rest
// are declared at their offsets so nothing below them can drift.  EVERY pad
// is written as arithmetic.
class RenderStruct
{
public:
	// ---- part 1: what the engine installs (0x00462110) ---------------- //
	void*    m_pfnCreateClientObject_00;                        // 0x000
	void*    m_pfnLoadTexture_04;                               // 0x004
	void*    m_pfnGetTextureData_08;                            // 0x008
	void*    m_pfnFreeTextureData_0c;                           // 0x00c
	void*    m_pfnConsolePrint_10;                              // 0x010
	void*    m_pfnConsolePrintf_14;                             // 0x014
	void*    m_pfnGetConsoleVar_18;                             // 0x018
	void*    m_pfnGetVarValueFloat_1c;                          // 0x01c
	void*    m_pfnGetVarValueString_20;                         // 0x020
	void*    m_pfnEmpty_24;                                     // 0x024
	void*    m_pfnShellSlot0_28;                                // 0x028
	void*    m_pfnShellSlot1_2c;                                // 0x02c
	void*    m_pfnSub411450_30;                                 // 0x030
	void*    m_pfnAlloc_34;                                     // 0x034
	void*    m_pfnFree_38;                                      // 0x038

	// ---- part 2: engine data ------------------------------------------ //
	uint32   m_Width;                                           // 0x03c
	uint32   m_Height;                                          // 0x040
	uint32   m_bInited;                                         // 0x044
	uint8    m_pad_048[0x58 - 0x48];                            // 0x048
	uint32   m_nTagVisibleLeaves;                               // 0x058
	uint32   m_nFlushObjectQueues;                              // 0x05c
	uint32   m_nModels;                                         // 0x060
	uint32   m_nWorldModels;                                    // 0x064
	uint32   m_nTranslucent;                                    // 0x068

	// ---- part 3: what d3d.ren installs -------------------------------- //
	int      (*Init)(RenderStructInit* pInit);                  // 0x06c
	void     (*Term)();                                         // 0x070
	void     (*BindTexture)(SharedTexture* pTexture, LTBOOL bChanged);// 0x074
	void     (*UnbindTexture)(SharedTexture* pTexture);         // 0x078
	uint8    m_pad_07c[0xc4 - 0x7c];                            // 0x07c
	// Handed the screen PFormat to fill in; 0x0040eeb0 is its one caller.
	void     (*GetScreenFormat)(void* pFormat);                 // 0x0c4
	uint8    m_pad_0c8[0x100 - 0xc8];                           // 0x0c8

	// ---- part 4: engine data ------------------------------------------ //
	uint8    m_pad_100[0x130 - 0x100];                          // 0x100
	LTVector m_GlobalLightDir;                                  // 0x130
	LTVector m_GlobalLightColor;                                // 0x13c
	uint32   m_Unknown148;                                      // 0x148
	uint32   m_Unknown14c;                                      // 0x14c
};

extern RenderStruct g_RenderStruct;                             // 0x004b4358


// The TextureData budget, initialised by 0x004627b0 with 10,000,000 bytes
// and emptied by r_TermRender.  ltlink.h's LTList is m_nElements then
// m_Head, which puts the head at +0x0c of this record -- and 0x0046288f
// walks from 0x004b4138 (== head.m_pNext) stopping at 0x004b4134 (== &head),
// so the two agree exactly.
struct TextureDataList
{
	uint32 m_nMaxBytes;                                         // 0x00
	uint32 m_nCurBytes;                                         // 0x04
	LTList m_List;                                              // 0x08
};

// DEFINED HERE, not extern: its LTList member's constructor is the file's
// dynamic initialiser -- 0x00461d90 (`jmp`, the .CRT$XC entry at 0x004a70c0)
// and 0x00461da0 (m_nElements = 0, then the head tied off), sitting just
// before this file's first function 0x00461dc0.  No destructor, so no atexit.
TextureDataList g_TextureDataList;                              // 0x004b4128


// ======================================================================= //
//  What this file calls and does not define
// ======================================================================= //

// The client manager, as much of it as this file reads.  +0x61c is
// clientmgr2.cpp's m_pClientShell and +0x1610 is what it calls
// m_Unknown1610 -- it is a pointer to a polymorphic object, because
// 0x004627ff loads its vptr and calls slot 4.
class CSub1610
{
public:
	virtual void v0() = 0;
	virtual void v1() = 0;
	virtual void v2() = 0;
	virtual void v3() = 0;
	virtual void Term() = 0;                                    // slot 0x10
};

class CClientMgrView
{
public:
	uint8         m_pad_000[0x61c];
	IClientShell* m_pClientShell;                               // 0x0061c
	// The 0x12c-byte error string SetupError formats into (0x0041fc60 adds
	// 0x620 to the manager and passes 0x12c as the length).
	char          m_ErrorString[0x12c];                         // 0x00620
	uint8         m_pad_74c[0x1610 - 0x74c];
	CSub1610*     m_pSub1610;                                   // 0x01610

	// A varargs member is __cdecl, which is why the shipped call pushes
	// `this` LAST and cleans up itself.                        0x0041fc60
	LTRESULT SetupError(LTRESULT dResult, ...);
	// clientmgr2.cpp already reconstructed the middle three.
	void   Sub4105c0();                                         // 0x004105c0
	LTBOOL CreateRenderWorld();                                 // 0x00410510
	void   DestroyRenderWorld();                                // 0x00410530
	void   SetGlobals();                                        // 0x00410540
	void   FreeList1110Contents();                              // 0x00410590
	// clientmgr2.cpp writes this one as a cdecl free function `cm_Sub410610`
	// and its BODY matches either way, but the call site here loads %ecx
	// with the manager first, so the original is a member.      0x00410610
	void   Sub410610();
	// 0x00411450 -- clientmgr2.cpp's CClientMgrView::NextFrameCode; the
	// renderer's +0x30 callback tail-calls it on g_pClientMgr.
	uint16 NextFrameCode();
};

// 0x0042e740 -- the main window (clientinput.cpp names it).
HWND dsi_GetMainWindow();

// 0x0042d760 -- clientworld.cpp reconstructed it: break into the debugger if
// 0x004b0b1c is set.  Every RETURN_ERROR site in the engine calls it with
// the code it is about to return.
void sub_0042d760(int nError);

// 0x0042e6f0 -- the client's console printf (clientconsole.cpp names it
// con_ConsolePrint).
void con_ConsolePrint(char* pFormat, ...);

// 0x0041e930 -- print one already-formatted string to the client console.
// Byte-for-byte the same body as the +0x10 render callback at 0x004622b0,
// and NOT folded with it.
void con_PrintString(char* pStr);

// 0x0041b840 -- thiscall 0x0041cd50 on the console object at 0x004b2af8.
void con_Sub41b840();

// 0x0040eeb0 -- publish the struct: stores it in g_pRender (0x004b17a4),
// copies its +0x3c/+0x40 into 0x004b1630/0x004b1634, calls the table's
// +0xc4 and then walks a list.  Returns LTFALSE if any element refused.
// Defined below (its real home is cissurf.cpp).
LTBOOL r_PublishRenderStruct(RenderStruct* pStruct);            // 0x0040eeb0

// 0x0040f060 -- restores a block of globals from 0x004b175c.. into
// 0x004b15c4.. and answers whether it worked.  Defined in clientmgr2.cpp.
LTBOOL r_UnpublishRenderStruct();                               // 0x0040f060

// 0x00462110 -- reinstall the fifteen engine callbacks and the default
// global-light state.  A NON-ZERO argument clears the whole 0x150 bytes; a
// zero one clears only the first 0x100 (so the light state below it is kept
// and then rewritten).  Defined below, before the callbacks.
void r_InitRenderStruct(int bFullClear);

// 0x00461dc0 -- release one TextureData (lttexmod.cpp already declares it).
void sub_461dc0(TextureData* pData);

// The engine's shared RETURN_ERROR format, "LT ERROR: %s returned %s (%s)".
// It is a char* VARIABLE at 0x004a7110 pointing at the literal at 0x004a7114,
// not a literal operand -- the shipped code loads it before pushing it.
extern char* g_pReturnErrorFormat;                              // 0x004a7110
extern int   g_DebugLevel;                                      // 0x004b0ab0

// DEFINED HERE: the current mode, in this file's .bss between
// g_TextureDataList and g_RenderStruct.  POD, so no initialiser.
RMode g_CurRenderMode;                                          // 0x004b4140
// DEFINED HERE: the first dword past the RenderStruct, uninitialised.
HINSTANCE g_hRenderDLL;                                         // 0x004b44a8
// Non-zero while r_InitRender is on the stack; the guard that makes
// r_InitRender a no-op if the renderer re-enters it:
// g_CluClientGlob.m_bInInitRender (0x004b0b18).
// Non-zero if the engine clipped the cursor and must let it go: the
// "CursorCenter" console variable (..\server\dsi.cpp's lithtech block).
extern int   g_CV_CursorCenter;                                 // 0x004a9ec4

typedef void (*RenderDLLSetupFn)(RenderStruct* pStruct);


// ======================================================================= //
//  The engine's side of the table: the fifteen callbacks (0x00462110)
//  and the system-memory texture release they share
// ======================================================================= //

// The manager behind the pointer at 0x004b0b14 (ccline.cpp's
// g_pCcnClientMgr), as much of it as the callbacks read: an embedded
// polymorphic object at +0x1ec (a MEMBER -- the stubs `lea 0x1ec` straight
// into %ecx with no null test) and the thiscall at 0x00411450.
class CRenderCallbackSub
{
public:
	virtual void Slot0();                                       // +0x00
	virtual void Slot1();                                       // +0x04
};

class CRenderCallbackMgr
{
public:
	uint8              m_pad_000[0x1ec];
	CRenderCallbackSub m_Sub1ec;                                // 0x1ec
	uint8              m_pad_1f0[0x4c0 - 0x1f0];
	// ccline.cpp's m_pClientDE; rs_ProcessAttachment (0x00462091) reads
	// its +0x08, the ILTCommon.
	class CRenderClientDEView* m_pClientDE;                     // 0x4c0
	uint8              m_pad_4c4[0x1534 - 0x4c4];
	// clientmgr.h's m_pFileMgr (0x00461e8e).
	class CRenderFileMgr* m_pFileMgr;                           // 0x1534
};

// 0x004b0b14 -- g_CluClientGlob.m_pClientMgr, read through this view.

// cchandler.cpp's CClientFileRef view: a default constructor zeroing all
// three fields, which is why 0x00462240 stores only two zeros (the
// filename store kills the constructor's).
class CRenderFileRef
{
public:
	CRenderFileRef()
	{
		m_nFileType = 0;
		m_pFilename = LTNULL;
		m_nLocalID  = 0;
	}

	uint32      m_nFileType;                                    // 0x00
	const char* m_pFilename;                                    // 0x04
	uint16      m_nLocalID;                                     // 0x08
};

// 0x0041ff90 -- resolve a file ref to a SharedTexture (lttexmod.cpp).
SharedTexture* sub_41ff90(CRenderCallbackMgr* pMgr, CRenderFileRef* pRef);

// 0x00461e30 -- r_LoadSystemTexture (the name its RETURN_ERROR prints):
// load a SharedTexture's system-memory data.  Defined below.
LTRESULT sub_461e30(SharedTexture* pTexture, TextureData** ppData, LTBOOL bBind);

// The console variable as the two value getters read it.
struct RenderConsoleVar
{
	uint8  m_pad_00[0x14];
	char*  m_pStringVal;                                        // 0x14
	float  m_FloatVal;                                          // 0x18
};

// servermgr.h's ConsoleState and ..\server\console.cpp's two entries, by
// their own signatures.
class ConsoleState;
extern ConsoleState g_ClientConsoleState;                       // 0x004b2f00

void              con_RunString(ConsoleState* pState, char* pStr);         // 0x0041abe0
HCONSOLEVAR       con_FindVar(ConsoleState* pState, char* pName);          // 0x0041b3e0
void              con_Print(char* pFormat, ...);                                 // 0x0041b8c0
void              con_Sub41b820(int nStage);                                     // 0x0041b820
// clientconsole.cpp defines it with three ints (the rect, the printer and
// the RenderStruct, passed straight through to CConsole::Init).
void              con_Sub41b850(int pRect, int pPrint, int pStruct);                    // 0x0041b850

void* dalloc(uint32 size);                                      // 0x00428fd0
void  dfree(void* ptr);                                         // 0x00429080


// 0x00461dc0 -- 100 bytes.  Release one system-memory TextureData: take its
// bytes off g_TextureDataList's running total, detach it from its
// SharedTexture, unlink it (CheapLTLink::Remove, TieOff included), drop the
// list's count and free the pixel buffer and then the record.
void sub_461dc0(TextureData* pData)
{
	g_TextureDataList.m_nCurBytes -= pData->m_AllocSize;
	pData->m_pSharedTexture->m_pEngineData = LTNULL;
	pData->m_Link.Remove();
	g_TextureDataList.m_List.m_nElements--;
	dfree(pData->m_pDataBuffer);
	dfree(pData);
}


// ---- 0x00461e30, r_LoadSystemTexture ----------------------------------- //

// cmodelload.cpp's CFileIdentQ474: the name is at +0x1c (0x0046204d).
class CRenderFileIdent
{
public:
	uint8 m_pad_00[0x1c];
	char* m_pName;                                              // 0x1c
};

class CRenderStreamView
{
public:
	virtual void _p00() = 0;                                    // ~ILTStream
	virtual void Release() = 0;                                 // 0x04
};

// 0x00404890 -- cmodelload.cpp's cm_OpenStream: a stream over one file.
CRenderStreamView* cm_OpenStream(CRenderFileMgr* pFileMgr, CRenderFileIdent* pIdent);

// 0x0042eab0 -- clientdsi.cpp's dtx_Create: read a DTX out of a stream.
LTRESULT dtx_Create(CRenderStreamView* pStream, TextureData** ppData,
                    LTBOOL bA, LTBOOL bB);

// 0x004628e0 -- defined below.
void rif_BindTexture(SharedTexture* pTexture, LTBOOL bChanged);

// A 0x20-byte local nothing reads again, built by the constructor at
// 0x004441a0 (`mov %ecx,%eax`, zero +0x08/+0x0c, `ret`) -- the same body
// clientmgr3.cpp calls LThreadMessage's.  Only the frame slot is known.
class CRenderLoadScratch
{
public:
	CRenderLoadScratch();                                       // 0x004441a0
	uint8 m_pad_00[0x20];
};

// dl_AddHead, but with the head's m_pNext read AGAIN for the back link
// (0x00461ef0 reloads 0x004b4138 after the store to pLink->m_pNext).  The
// SDK's CheapLTLink::AddAfter reads pLink->m_pNext there instead, which
// VC6 forwards from the register -- 6 bytes short and one block off.
inline void rif_AddHead(LTList* pList, LTLink* pLink, void* pObj)
{
	pLink->m_pData = pObj;
	pLink->m_pPrev = &pList->m_Head;
	pLink->m_pNext = pList->m_Head.m_pNext;
	pList->m_Head.m_pNext->m_pPrev = pLink;
	pLink->m_pPrev->m_pNext = pLink;
	++pList->m_nElements;
}

// 0x00461e30 -- 586 bytes.  The frame is, low to high, the file ref (its
// constructor is the three zero stores at 0x00461e65..0x00461e6d), the
// manager pointer (spilled at 0x00461e8a), the 0x20-byte scratch local and
// ltbasedefs.h's ConParse (0x1508 bytes; its constructor is the zero store
// to +0x104 at 0x00461e5e).  After the load the data goes on the head of
// g_TextureDataList (dl_AddHead, inline), and each of the three command-
// string keys links a second texture; DetailTex returns on its own.
LTRESULT sub_461e30(SharedTexture* pTexture, TextureData** ppData, LTBOOL bBind)
{
	CRenderLoadScratch  scratch;
	ConParse            parse;
	CRenderFileRef      ref;
	CRenderCallbackMgr* pMgr;
	CRenderFileIdent*   pIdent;
	CRenderStreamView*  pStream;
	LTRESULT            dResult;

	*ppData = LTNULL;

	pIdent = pTexture->m_pFile;
	if (!pIdent)
		return LT_NOTINITIALIZED;

	pMgr = (CRenderCallbackMgr*)g_CluClientGlob.m_pClientMgr;
	pStream = cm_OpenStream(pMgr->m_pFileMgr, pIdent);
	if (pStream)
	{
		dResult = dtx_Create(pStream, ppData, LTFALSE, LTFALSE);
		pStream->Release();
		if (dResult != LT_OK)
			return dResult;

		rif_AddHead(&g_TextureDataList.m_List, &(*ppData)->m_Link, *ppData);
		g_TextureDataList.m_nCurBytes += (*ppData)->m_AllocSize;

		pTexture->m_pEngineData = *ppData;
		(*ppData)->m_pSharedTexture = pTexture;

		if (bBind)
			rif_BindTexture(pTexture, LTFALSE);

		parse.Init((*ppData)->m_CommandString);
		if (parse.ParseFind("DetailTex", LTFALSE, 1))
		{
			ref.m_nFileType = 1;
			ref.m_pFilename = parse.m_Args[1];
			pTexture->m_pLinkedTexture = sub_41ff90(pMgr, &ref);
			pTexture->m_nLinkType = 0;
			return LT_OK;
		}

		parse.Init((*ppData)->m_CommandString);
		if (parse.ParseFind("EnvMap", LTFALSE, 1))
		{
			ref.m_nFileType = 1;
			ref.m_pFilename = parse.m_Args[1];
			pTexture->m_pLinkedTexture = sub_41ff90(pMgr, &ref);
			pTexture->m_nLinkType = 1;
		}

		parse.Init((*ppData)->m_CommandString);
		if (parse.ParseFind("EnvMapAlpha", LTFALSE, 1))
		{
			ref.m_nFileType = 1;
			ref.m_pFilename = parse.m_Args[1];
			pTexture->m_pLinkedTexture = sub_41ff90(pMgr, &ref);
			pTexture->m_nLinkType = 2;
		}

		return LT_OK;
	}

	sub_0042d760(LT_MISSINGFILE);
	if (g_DebugLevel >= 1)
		con_ConsolePrint(g_pReturnErrorFormat, "r_LoadSystemTexture",
		                 "LT_MISSINGFILE", pIdent->m_pName);
	return LT_MISSINGFILE;
}


// 0x004105c0 -- 65 bytes.  REAL HOME: clientmgr2.cpp (it sits between that
// file's 0x00410590 and 0x00410610); written here because this file owns
// CClientMgrView's declaration of it.  Restart the console on the top half
// of the screen, printing through 0x0041e930.
void CClientMgrView::Sub4105c0()
{
	LTRect rect;

	rect.left   = 0;
	rect.top    = 0;
	rect.right  = g_RenderStruct.m_Width;
	rect.bottom = g_RenderStruct.m_Height / 2;

	con_Sub41b820(0);
	con_Sub41b850((int)&rect, (int)con_PrintString, (int)&g_RenderStruct);
}


// The fifteen callbacks r_InitRenderStruct installs, defined below it (the
// first is ltnew1.cpp's and still unwritten; the empty one is a bare `ret`
// folded with every other empty function in the image).
class LTObject;
class CRenderAttachment;
LTObject*          rs_ProcessAttachment(LTObject* pObject,
                                     CRenderAttachment* pAttachment); // 0x00462080
SharedTexture*     rs_GetSharedTexture(const char* pFilename);  // 0x00462240
TextureData*       rs_GetTexture(SharedTexture* pTexture);      // 0x00462270
void               rs_FreeTexture(SharedTexture* pTexture);     // 0x00462290
void               rs_RunConsoleString(const char* pStr);       // 0x004622b0
void               rs_ConsolePrintf(const char* pMsg, ...);     // 0x004622d0
RenderConsoleVar*  rs_GetParameter(const char* pName);          // 0x00462310
float              rs_GetParameterValueFloat(RenderConsoleVar* pVar); // 0x00462330
char*              rs_GetParameterValueString(RenderConsoleVar* pVar); // 0x00462350
void               rs_Empty();                                  // 0x00404470
void               rs_ShellSlot0();                             // 0x00462380
void               rs_ShellSlot1();                             // 0x00462360
void               rs_Sub411450();                              // 0x004623a0
void*              rs_Alloc(uint32 size);                       // 0x004623b0
void               rs_Free(void* ptr);                          // 0x004623c0

// +0x00, 0x00462080 -- 112 bytes.  cdecl, two arguments, only the second
// read (0x0046208b); ltnew1.cpp's ProcessAttachments calls it once per
// attachment and ignores the result.  LithTech's ProcessAttachment: ask
// ILTCommon (slot +0x18, GetAttachmentTransform, 0x004620a2) for the
// attachment's world transform -- the 0x1c-byte frame is exactly one
// LTransform, m_Pos at +0x00 (0x004620cb) and m_Rot at +0x0c (0x004620d9)
// -- then find the child by its uint16 id at +0x1e (0x004620b1, through
// 0x00420520) and move (0x004207a0, bForce 0) and rotate (0x00420880) it.
// Returns the child: an explicit 0 when the transform fails (0x004620aa),
// the null lookup result as is (0x004620c5), the object on success.
class CRenderClientDEView
{
public:
	uint8      m_pad_00[0x08];
	ILTCommon* m_pCommonLT;                                     // 0x08
};

class CRenderAttachment
{
public:
	uint8  m_pad_00[0x1e];
	uint16 m_nChildID;                                          // 0x1e
};

LTObject* rs_GetObject(CRenderCallbackMgr* pMgr, uint16 nID);  // 0x00420520
void      rs_MoveObject(CRenderCallbackMgr* pMgr, LTObject* pObj,
                        LTVector* pPos, int bForce);           // 0x004207a0
void      rs_RotateObject(CRenderCallbackMgr* pMgr, LTObject* pObj,
                          LTRotation* pRot);                   // 0x00420880

LTObject* rs_ProcessAttachment(LTObject* pObject, CRenderAttachment* pAttachment)
{
	CRenderCallbackMgr* pMgr = (CRenderCallbackMgr*)g_CluClientGlob.m_pClientMgr;
	LTransform tAttachment;
	LTObject* pAttachedObj;

	if (pMgr->m_pClientDE->m_pCommonLT->GetAttachmentTransform(
	        (HATTACHMENT)pAttachment, tAttachment, LTTRUE) != LT_OK)
		return LTNULL;

	pAttachedObj = rs_GetObject(pMgr, pAttachment->m_nChildID);
	if (!pAttachedObj)
		return LTNULL;

	rs_MoveObject(pMgr, pAttachedObj, &tAttachment.m_Pos, LTFALSE);
	rs_RotateObject(pMgr, pAttachedObj, &tAttachment.m_Rot);
	return pAttachedObj;
}

// DEFINED HERE: the one RenderStruct.  Its LTVector members give it an empty
// implicit constructor, which is the dynamic initialiser 0x004620f0 (`jmp`,
// the .CRT$XC entry at 0x004a70c4) and 0x00462100 (a bare `ret`) sitting
// right here, between rs_ProcessAttachment and r_InitRenderStruct.
RenderStruct g_RenderStruct;                                    // 0x004b4358

// 0x00462110 -- 297 bytes.
//
// THE CLEAR.  `mov $0x54,%ecx ; jne ; mov $0x40,%ecx` in front of ONE
// `rep stos` is two memsets of different constant sizes with their common
// tail merged -- an if/else, not a ternary (a ternary of two constants is
// branchless).
//
// THE LIGHT.  LTVector::Init(0,-2,-1) then Norm(), both inline: VC6 folds
// Mag()'s x*x+y*y+z*z to the double literal 5.0 and takes fsqrt of it before
// the clear, Norm's early return on a zero magnitude is the fcoms/jne, and
// 0*inv folds to a plain zero store.  The two dword clears after Norm are
// duplicated into both of its exits.
void r_InitRenderStruct(int bFullClear)
{
	if (bFullClear)
		memset(&g_RenderStruct, 0, sizeof(g_RenderStruct));
	else
		memset(&g_RenderStruct, 0, 0x100);

	g_RenderStruct.m_pfnCreateClientObject_00 = rs_ProcessAttachment;
	g_RenderStruct.m_pfnLoadTexture_04        = rs_GetSharedTexture;
	g_RenderStruct.m_pfnGetTextureData_08     = rs_GetTexture;
	g_RenderStruct.m_pfnFreeTextureData_0c    = rs_FreeTexture;
	g_RenderStruct.m_pfnConsolePrint_10       = rs_RunConsoleString;
	g_RenderStruct.m_pfnConsolePrintf_14      = rs_ConsolePrintf;
	g_RenderStruct.m_pfnGetConsoleVar_18      = rs_GetParameter;
	g_RenderStruct.m_pfnGetVarValueFloat_1c   = rs_GetParameterValueFloat;
	g_RenderStruct.m_pfnGetVarValueString_20  = rs_GetParameterValueString;
	g_RenderStruct.m_pfnEmpty_24              = rs_Empty;
	g_RenderStruct.m_pfnShellSlot0_28         = rs_ShellSlot0;
	g_RenderStruct.m_pfnShellSlot1_2c         = rs_ShellSlot1;
	g_RenderStruct.m_pfnSub411450_30          = rs_Sub411450;
	g_RenderStruct.m_pfnAlloc_34              = rs_Alloc;
	g_RenderStruct.m_pfnFree_38               = rs_Free;

	g_RenderStruct.m_GlobalLightDir.Init(0.0f, -2.0f, -1.0f);
	g_RenderStruct.m_GlobalLightDir.Norm();

	g_RenderStruct.m_Unknown148 = 0;
	g_RenderStruct.m_Unknown14c = 0;
}


// +0x04, 0x00462240 -- 43 bytes.
SharedTexture* rs_GetSharedTexture(const char* pFilename)
{
	CRenderFileRef ref;

	ref.m_nFileType = 0;
	ref.m_pFilename = pFilename;
	return sub_41ff90((CRenderCallbackMgr*)g_CluClientGlob.m_pClientMgr, &ref);
}

// +0x08, 0x00462270 -- 31 bytes.
TextureData* rs_GetTexture(SharedTexture* pTexture)
{
	TextureData* pData;

	if (pTexture->m_pEngineData)
		return pTexture->m_pEngineData;

	sub_461e30(pTexture, &pData, 0);
	return pData;
}

// +0x0c, 0x00462290 -- 25 bytes.
void rs_FreeTexture(SharedTexture* pTexture)
{
	TextureData* pData;

	pData = pTexture->m_pEngineData;
	if (pData && !(pData->m_Flags & TDFLAG_TEXMOD))
		sub_461dc0(pData);
}

// +0x10, 0x004622b0 -- 19 bytes.
void rs_RunConsoleString(const char* pStr)
{
	con_RunString(&g_ClientConsoleState, (char*)pStr);
}

// +0x14, 0x004622d0 -- 51 bytes.
void rs_ConsolePrintf(const char* pMsg, ...)
{
	va_list marker;
	char    msg[300];

	va_start(marker, pMsg);
	_vsnprintf(msg, 299, pMsg, marker);
	va_end(marker);

	con_Print(msg);
}

// +0x18, 0x00462310 -- 19 bytes.
RenderConsoleVar* rs_GetParameter(const char* pName)
{
	return (RenderConsoleVar*)con_FindVar(&g_ClientConsoleState, (char*)pName);
}

// +0x1c, 0x00462330 -- 19 bytes.
float rs_GetParameterValueFloat(RenderConsoleVar* pVar)
{
	if (pVar)
		return pVar->m_FloatVal;
	return 0.0f;
}

// +0x20, 0x00462350 -- 15 bytes.  (The enumerated 110-byte row at this
// address is this function plus the next four.)
char* rs_GetParameterValueString(RenderConsoleVar* pVar)
{
	if (pVar)
		return pVar->m_pStringVal;
	return LTNULL;
}

// +0x2c, 0x00462360.
void rs_ShellSlot1()
{
	((CRenderCallbackMgr*)g_CluClientGlob.m_pClientMgr)->m_Sub1ec.Slot1();
}

// +0x28, 0x00462380.
void rs_ShellSlot0()
{
	((CRenderCallbackMgr*)g_CluClientGlob.m_pClientMgr)->m_Sub1ec.Slot0();
}

// +0x30, 0x004623a0.
void rs_Sub411450()
{
	((CClientMgrView*)g_CluClientGlob.m_pClientMgr)->NextFrameCode();
}

// +0x34, 0x004623b0 -- 14 bytes.
void* rs_Alloc(uint32 size)
{
	return dalloc(size);
}

// +0x38, 0x004623c0 -- 12 bytes.
void rs_Free(void* ptr)
{
	dfree(ptr);
}


// ======================================================================= //
//  Handing the new table to the surface code -- 0x0040eeb0
// ======================================================================= //
//
// REAL HOME: src/engine/lithtech/cissurf.cpp.  The address sits in that
// file's 0x0040c000..0x0040f2ff run, between cis_FreeAllSurfaces
// (0x0040ee40) and the list walk it tail-calls, surface_RestoreAll
// (0x0040eef0, written in clientmgr2.cpp), and the five globals it writes
// are cissurf.cpp's.  It is written here because this file owns its one
// caller and the RenderStruct it reads.  The enumerated 124-byte row is this
// 61-byte body plus the first 60 bytes of surface_RestoreAll behind the
// `jmp` (VC6's tail call: no frame, no arguments to pass).
// fmconvert.cpp's name for PFormat, whose GetBytesPerPixel it defines.
class FMFormat
{
public:
	uint32 GetBytesPerPixel();                                  // 0x0045f230
};

// cissurf.cpp defines g_pRender and g_ScreenFormat with its own class
// names; these two one-line views carry those names (clientmgr.h has the
// same RenderStructQ40C) so the references link against the definitions.
class RenderStructQ40C : public RenderStruct {};
class PFormatQ40C : public FMFormat {};

// The screen size this publishes (0x004b1630 / 0x004b1634) is not a global
// of its own: it is cissurf.cpp's g_ScreenSurface's m_nWidth / m_nHeight.
class CSurfaceQ40C
{
public:
	uint8  m_pad_00[0x10];
	int    m_nWidth;                                            // 0x010
	int    m_nHeight;                                           // 0x014
};

extern RenderStructQ40C* g_pRender;                             // 0x004b17a4
extern CSurfaceQ40C  g_ScreenSurface;                           // 0x004b1620
extern PFormatQ40C   g_ScreenFormat;                            // 0x004b1758
extern uint32        g_nBytesPerPixel;                          // 0x004b1794

LTBOOL surface_RestoreAll();                                    // 0x0040eef0

LTBOOL r_PublishRenderStruct(RenderStruct* pStruct)
{
	g_pRender = (RenderStructQ40C*)pStruct;
	g_ScreenSurface.m_nWidth  = pStruct->m_Width;
	g_ScreenSurface.m_nHeight = pStruct->m_Height;

	pStruct->GetScreenFormat(&g_ScreenFormat);
	g_nBytesPerPixel = g_ScreenFormat.GetBytesPerPixel();

	return surface_RestoreAll();
}


// ======================================================================= //
//  The list the renderer's texture memory lives on
// ======================================================================= //

// 0x004627b0 -- 27 bytes.  Two things pin it as ltlink.h's dl_InitList plus
// two assignments rather than five hand-written stores: the stores come out
// +0x10, +0x0c, +0x08, +0x04, +0x00, and `m_pPrev = m_pNext = this` is
// exactly the order CheapLTLink::TieOff writes them in (the inner
// assignment first).  The only caller is r_InitRender, with 10,000,000.
void rif_InitTextureDataList(TextureDataList* pList, uint32 nMaxBytes)
{
	dl_InitList(&pList->m_List);
	pList->m_nCurBytes = 0;
	pList->m_nMaxBytes = nMaxBytes;
}


// ======================================================================= //
//  Texture bind and release
// ======================================================================= //

// 0x004628e0 -- 29 bytes.  Hand a texture to the renderer, but only while
// the renderer is up.  lttexmod.cpp (0x00488911, with the literal 1) and
// clientmgr2.cpp's tm_ReloadTextures (with 0, for every texture that has no
// render-side copy) are the two shapes of caller, which is what says the
// second argument is "the contents changed" and not a flag on the texture.
void rif_BindTexture(SharedTexture* pTexture, LTBOOL bChanged)
{
	if (g_RenderStruct.m_bInited)
		g_RenderStruct.BindTexture(pTexture, bChanged);
}


// 0x00462900 -- 55 bytes.  Give a texture's data back.  clienttexmgr.cpp
// already decoded this one and its note is right in every particular: the
// renderer's own release goes first (and only while the renderer is up),
// then the engine's data goes unless it carries lttexmod.cpp's
// TDFLAG_TEXMOD, then the field is cleared.
//
// The write-back at 0x0046292e is INSIDE the `if`, not after it -- that is
// what separates this from the +0x0c callback at 0x00462290, whose body is
// otherwise the same three tests.
//
// clientmgr2.cpp calls this address `dfree`.  It is not dfree; dfree is
// 0x00429080, which this file's +0x38 callback wraps.
void tex_FreeTextureData(SharedTexture* pTexture)
{
	TextureData* pData;

	if (g_RenderStruct.m_bInited)
		g_RenderStruct.UnbindTexture(pTexture);

	pData = pTexture->m_pEngineData;
	if (pData && !(pData->m_Flags & TDFLAG_TEXMOD))
	{
		sub_461dc0(pData);
		pTexture->m_pEngineData = LTNULL;
	}
}


// ======================================================================= //
//  Shutting the renderer down
// ======================================================================= //

// 0x004627d0 -- 269 bytes.
//
// The whole body sits under one positive condition and the shared `xor
// %eax,%eax; ret` is at the END, which is the shape the shared-exit
// lever predicts for exactly that source.
//
// The second argument only ever decides whether a failed 0x0040f060 is an
// error: clientinterface.cpp passes 1, r_InitRender passes 0.  It is
// therefore "this is a real shutdown, complain if the screen cannot be put
// back", not a flag on the renderer.
//
// clientmgr.h declares this address `void sub_4627d0(...)`.  It returns
// LTRESULT -- 0x0046284d is `mov $0x30,%eax` -- and every caller discards it.
LTRESULT r_TermRender(CClientMgrView* pClientMgr, LTBOOL bReportFailure)
{
	LTLink*      pCur;
	LTLink*      pNext;
	TextureData* pData;

	if (g_RenderStruct.m_bInited)
	{
		if (pClientMgr->m_pClientShell)
			pClientMgr->m_pClientShell->OnEvent(LTEVENT_RENDERTERM, 0);

		if (pClientMgr->m_pSub1610)
			pClientMgr->m_pSub1610->Term();

		if (!r_UnpublishRenderStruct() && bReportFailure == 1)
		{
			pClientMgr->SetupError(LT_UNABLETORESTOREVIDEO);
			sub_0042d760(LT_UNABLETORESTOREVIDEO);
			if (g_DebugLevel >= 1)
				con_ConsolePrint(g_pReturnErrorFormat, "r_TermRender", "48", " ");
			return LT_UNABLETORESTOREVIDEO;
		}

		pClientMgr->Sub410610();
		pClientMgr->FreeList1110Contents();
		pClientMgr->DestroyRenderWorld();

		if (g_hRenderDLL)
		{
			g_RenderStruct.Term();
			::FreeLibrary(g_hRenderDLL);
			g_hRenderDLL = LTNULL;
		}

		// Both loads happen before the free, m_pData first: the shipped
		// pair is `mov 0x8(%eax),%edx ; mov 0x4(%eax),%esi`.
		pCur = g_TextureDataList.m_List.m_Head.m_pNext;
		while (pCur != &g_TextureDataList.m_List.m_Head)
		{
			pData = (TextureData*)pCur->m_pData;
			pNext = pCur->m_pNext;
			sub_461dc0(pData);
			pCur = pNext;
		}

		if (g_CV_CursorCenter)
			::ClipCursor(LTNULL);

		g_RenderStruct.m_bInited = 0;
		r_InitRenderStruct(0);
	}

	return LT_OK;
}


// ======================================================================= //
//  Bringing the renderer up
// ======================================================================= //

// 0x004623d0 -- 977 bytes.  LTRESULT r_InitRender(CClientMgr*, RMode*).
//
// The signature is pinned by three call sites, all cdecl with two pushes:
// 0x004035c1 (regaining focus, with &g_CurRenderMode), 0x004116ec and
// 0x0042dffb/0x0042e01c (SetRenderMode, which tries the requested mode and
// then falls back).
//
// FRAME.  `sub $0x2e4` plus four pushes; two aggregates and nothing else:
//   esp+0x010  char buf[200]         the sprintf target, 0xc8 bytes
//   esp+0x0d8  RenderStructInit init 0x21c bytes, ending exactly at the top
// 0xc8 + 0x21c == 0x2e4, so there is no third local and no spilled scalar --
// the window handle lives in %ebp for the whole function and the constant 0
// lives in %edi.  `init` is declared first because it takes the HIGH end.
//
// THE THREE IDENTICAL ERROR ARMS.  The load failure, the missing export and
// the refused Init all run the same four statements and the compiler
// cross-jumped them into one tail at 0x00462741/0x00462771/0x0046278b.  They
// are written out in full below, three times, because that is what the
// source did -- aiming two of them at the third is not the same source.
//
// WHY THE FLAG CLEAR IS LAST.  `g_CluClientGlob.m_bInInitRender = 0` is emitted AFTER the
// console print on every arm.  A store to a global cannot be moved across a
// call, so that is source order, not scheduling: the clear belongs to the
// arm and follows the whole error report.
LTRESULT r_InitRender(CClientMgrView* pClientMgr, RMode* pMode)
{
	RenderStructInit init;
	char             buf[200];
	HWND             hWnd;
	RenderDLLSetupFn pSetupFn;

	if (g_CluClientGlob.m_bInInitRender)
		return LT_OK;

	g_CluClientGlob.m_bInInitRender = 1;

	r_TermRender(pClientMgr, LTFALSE);

	hWnd = dsi_GetMainWindow();
	::ShowWindow(hWnd, SW_RESTORE);

	rif_InitTextureDataList(&g_TextureDataList, 10000000);

	memset(&init, 0, sizeof(init));
	// m_hWnd before the mode copy: 0x00462453 stores %ebp ahead of the
	// `rep movsl`.
	init.m_hWnd = hWnd;
	init.m_Mode = *pMode;

	g_hRenderDLL = ::LoadLibrary(pMode->m_RenderDLL);
	if (!g_hRenderDLL)
	{
		pClientMgr->SetupError(LT_ERRORLOADINGRENDERDLL, pMode->m_RenderDLL);
		sub_0042d760(LT_ERRORLOADINGRENDERDLL);
		if (g_DebugLevel >= 1)
			con_ConsolePrint(g_pReturnErrorFormat, "r_InitRender",
			                 "LT_ERRORLOADINGRENDERDLL", pMode->m_RenderDLL);
		g_CluClientGlob.m_bInInitRender = 0;
		return LT_ERRORLOADINGRENDERDLL;
	}

	pSetupFn = (RenderDLLSetupFn)::GetProcAddress(g_hRenderDLL, "RenderDLLSetup");
	if (!pSetupFn)
	{
		::FreeLibrary(g_hRenderDLL);
		g_hRenderDLL = LTNULL;
		pClientMgr->SetupError(LT_ERRORLOADINGRENDERDLL, pMode->m_RenderDLL);
		sub_0042d760(LT_ERRORLOADINGRENDERDLL);
		if (g_DebugLevel >= 1)
			con_ConsolePrint(g_pReturnErrorFormat, "r_InitRender",
			                 "LT_ERRORLOADINGRENDERDLL", pMode->m_RenderDLL);
		g_CluClientGlob.m_bInInitRender = 0;
		return LT_ERRORLOADINGRENDERDLL;
	}

	pSetupFn(&g_RenderStruct);

	g_RenderStruct.m_Width  = pMode->m_Width;
	g_RenderStruct.m_Height = pMode->m_Height;

	if (g_RenderStruct.Init(&init) != 0 || init.m_nVersion != RENDERSTRUCT_VERSION)
	{
		::FreeLibrary(g_hRenderDLL);
		g_hRenderDLL = LTNULL;
		pClientMgr->SetupError(LT_ERRORLOADINGRENDERDLL, pMode->m_RenderDLL);
		sub_0042d760(LT_ERRORLOADINGRENDERDLL);
		if (g_DebugLevel >= 1)
			con_ConsolePrint(g_pReturnErrorFormat, "r_InitRender",
			                 "LT_ERRORLOADINGRENDERDLL", pMode->m_RenderDLL);
		g_CluClientGlob.m_bInInitRender = 0;
		return LT_ERRORLOADINGRENDERDLL;
	}

	pClientMgr->Sub4105c0();

	if (!r_PublishRenderStruct(&g_RenderStruct))
	{
		::FreeLibrary(g_hRenderDLL);
		g_hRenderDLL = LTNULL;
		// No filename on this one -- the shipped call pushes two arguments.
		pClientMgr->SetupError(LT_UNABLETORESTOREVIDEO);
		sub_0042d760(LT_UNABLETORESTOREVIDEO);
		if (g_DebugLevel >= 1)
			con_ConsolePrint(g_pReturnErrorFormat, "r_InitRender", "48", " ");
		g_CluClientGlob.m_bInInitRender = 0;
		return LT_UNABLETORESTOREVIDEO;
	}

	g_RenderStruct.m_bInited = 1;

	if (pClientMgr->m_pClientShell)
		pClientMgr->m_pClientShell->OnEvent(LTEVENT_RENDERALMOSTINITTED, 0);

	if (!pClientMgr->CreateRenderWorld())
	{
		g_RenderStruct.m_bInited = 0;
		::FreeLibrary(g_hRenderDLL);
		g_hRenderDLL = LTNULL;
		// and no SetupError on this one either.
		sub_0042d760(LT_ERRORBINDINGWORLD);
		if (g_DebugLevel >= 1)
			con_ConsolePrint(g_pReturnErrorFormat, "r_InitRender", "43", " ");
		g_CluClientGlob.m_bInInitRender = 0;
		return LT_ERRORBINDINGWORLD;
	}

	::SetFocus(hWnd);
	pClientMgr->SetGlobals();

	g_CurRenderMode = init.m_Mode;

	// "CardDesc" prints m_InternalName (RMode +0x104), not m_Description
	// (+0x184).  The shipped `lea 0x1e0(%esp)` is +0x104 into the copy and
	// nothing in this function ever touches +0x184.
	sprintf(buf, "CardDesc %s", init.m_Mode.m_InternalName);
	con_PrintString(buf);
	sprintf(buf, "RenderDLL %s", init.m_Mode.m_RenderDLL);
	con_PrintString(buf);
	sprintf(buf, "ScreenWidth %d", init.m_Mode.m_Width);
	con_PrintString(buf);
	sprintf(buf, "ScreenHeight %d", init.m_Mode.m_Height);
	con_PrintString(buf);
	sprintf(buf, "BitDepth %d", init.m_Mode.m_BitDepth);
	con_PrintString(buf);

	con_Sub41b840();

	if (pClientMgr->m_pClientShell)
		pClientMgr->m_pClientShell->OnEvent(LTEVENT_RENDERINIT, 0);

	g_CluClientGlob.m_bInInitRender = 0;
	return LT_OK;
}


// ======================================================================= //
//  DELIBERATELY NOT WRITTEN
// ======================================================================= //
//
//  0x00462080 -- the one callback stub still unwritten (ltnew1.cpp
//  declares it); it needs the CClientMgr sub-object at +0x4c0 and its
//  vtable.  The other fourteen are written above r_InitRender.
//
//  0x00404470 -- a bare `ret`, folded with every other empty function.
//

// ======================================================================= //
//  THINGS IN EXISTING REPO SOURCES THAT LOOK WRONG (nothing was edited)
// ======================================================================= //
//
//  1. src/engine/d3d/d3d.cpp says "RenderStruct is at least 0x20c bytes:
//     0x004624ff reads pStruct+0x204 and +0x208 straight back out into
//     0x004b4394/0x004b4398."  Those two loads are off the RMODE argument,
//     not off RenderStruct.  0x004624f8 reloads the function's second
//     parameter from 0x300(%esp) and 0x204/0x208 are RMode's m_Width and
//     m_Height (ltbasedefs.h).  The DESTINATIONS 0x004b4394/0x004b4398 are
//     RenderStruct +0x3c/+0x40, which d3d.cpp's own later paragraph already
//     says.  RenderStruct is 0x150 bytes, fixed by 0x00462110's
//     `rep stos` of 0x54 dwords from 0x004b4358 and by 0x004b44a8 (the
//     HMODULE) being the next object.
//
//  2. src/engine/lithtech/clientmgr.h calls RenderStruct+0x044
//     `m_nCanOptimizeSurfaces`, and separately declares the SAME ADDRESS
//     (0x004b439c) as `g_bRenderInited`.  g_bRenderInited is the right
//     reading: r_InitRender writes 1 into it after a good init and 0 on
//     every failure, and r_TermRender's whole body is under it.
//     clientinterface.cpp's two `g_pRender->m_nCanOptimizeSurfaces` tests
//     are therefore "is the renderer up", which is also what makes them
//     return LT_NOTINITIALIZED.  Only the name is wrong; the offset and the
//     emitted code are right, so nothing needs rebuilding.
//
//  3. src/engine/lithtech/clientworld.cpp declares 0x0042d760 as
//     `void sub_0042d760()`, cdecl with no arguments.  Its BODY takes no
//     argument -- but its callers all push one and clean it up themselves
//     (0x0046247a, 0x004624d6, 0x00462576, 0x004625f9, 0x0046275f, each
//     inside a combined `add $N,%esp`).  It is a dead parameter: the error
//     code the caller is about to return.  The body still matches; any
//     CALLER written against the no-argument declaration will not.
//
//  4. src/engine/lithtech/clientmgr2.cpp declares 0x00410610 as the cdecl
//     free function `cm_Sub410610()`.  The call at 0x00462854 loads %ecx
//     with the client manager first, exactly like the two thiscalls that
//     follow it, so the original is a member function that ignores `this`.
//     Again the body is unaffected; only call sites are.
//
//  5. src/engine/lithtech/clientmgr2.cpp declares 0x00462900 as
//     `void dfree(void* pMem)`.  clienttexmgr.cpp already flagged this and
//     is correct: dfree is 0x00429080 (the +0x38 render callback wraps it),
//     and 0x00462900 is the texture-data release written above.
//
//  6. Worth recording because it contradicts a common lever:
//     0x0041e930 and 0x004622b0 are BYTE-FOR-BYTE identical bodies at two
//     addresses in this image, and 0x004105f2 takes the address of the
//     first.  /OPT:ICF did not fold them.  So a repeated body in
//     lithtech.exe is not automatically evidence of folding.
