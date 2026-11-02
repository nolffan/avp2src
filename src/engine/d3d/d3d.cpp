// *********************************************************************** //
//
//  MODULE:   D3D (.CPP)
//
//  PURPOSE:  Notes on d3d.ren's engine<->renderer boundary: the device
//            slots pinned without the DX7 headers, and the RenderStruct map.
//
//  THE CODE IS NOT HERE.  The three exports -- GetSupportedModes
//  (0x100212d0), FreeModeList (0x100213e0), RenderDLLSetup (0x10021410) --
//  and the renderer's Init/Term/GetRenderObject are written in
//  w26_10020fd0_osoiqip.cpp: the shipped TU is /Os /Oi- /Qip (its strcmp
//  and memset are CALLS, and Init calls 0x10021070 through its /Qip
//  register entry), which this file's /Os does not reproduce.  The map
//  below is the one RenderDLLSetup there fills; its RenderStruct type is
//  written out in that file.
//
// *********************************************************************** //

#include "ltbasedefs.h"

// ---------------------------------------------------------------------- //
//  IDirect3DDevice7 SLOTS, pinned without the DX7 headers
// ---------------------------------------------------------------------- //
//
//  The toolchain ships DirectX 5's d3d.h, which stops at IDirect3DDevice2,
//  so every DX7 slot in this target has to be pinned from the bytes. What is
//  established so far, all on the device at 0x1008d988:
//
//    +0x14  index  5  BeginScene()            ret 4  (this only)
//    +0x18  index  6  EndScene()              ret 4
//    +0x50  index 20  SetRenderState(state, value)          ret 0xc
//    +0x54  index 21  GetRenderState(state, DWORD* pOut)    ret 0xc
//    +0x94  index 37  SetTextureStageState(stage, type, v)  ret 0x10
//
//  BeginScene/EndScene: the adjacent no-argument pair whose answer is tested
//  against zero, with End3D comparing it to 0x887601c2 (DDERR_SURFACELOST)
//  and recovering through IDirectDrawSurface7 +0x6c (Restore, index 27).
//
//  SetRenderState is pinned seven times over by SetOptimized2DBlend
//  (0x10034c70). Its seven arms push seven (SRCBLEND, DESTBLEND) pairs, and
//  each is exactly the D3DBLEND pair implementing the LTSurfaceBlend member
//  of the same ordinal in ltbasedefs.h -- SRCALPHA/INVSRCALPHA, ONE/ZERO,
//  ONE/ONE, DESTCOLOR/ZERO, DESTCOLOR/SRCCOLOR, ZERO/INVSRCCOLOR,
//  ONE/INVSRCCOLOR, including MULTIPLY2's dest*src + src*dest trick. Seven
//  independent agreements is not a coincidence.
//
//  Confirmed a second way by 0x10023c40, which expresses ONE operation two
//  ways and picks between them on the UseDX6Commands console variable: six
//  SetTextureStageState calls through +0x94 (MODULATE / TEXTURE / DIFFUSE),
//  or a single +0x50 call setting TEXTUREMAPBLEND (0x15) to MODULATEALPHA
//  (4). The old and new APIs for the same thing, side by side.
//
// ---------------------------------------------------------------------- //
//  THE BRIDGE INTO d3d.ren, and it is the ci_SetupInterface play again
// ---------------------------------------------------------------------- //
//
//  lithtech.exe resolves RenderDLLSetup by name at 0x004624a8 and calls it
//  at 0x004624f6 with ONE argument: `push $0x4b4358`. So the engine's
//  RenderStruct lives at 0x004b4358, and d3d.ren's RenderDLLSetup
//  (0x10021410, ending 0x1002157c) writes 37 function pointers into it at
//  slots 0x6c..0xfc -- all 37 distinct.
//
//  That arithmetic turns every name the client side already has into a
//  NAME FOR A d3d.ren FUNCTION. The check is immediate: the instruction
//  right after the call is `call *0x4b43c4`, which is 0x4b4358 + 0x6c --
//  the first slot written.
//
//    slot   d3d.ren     name, and where it came from
//    ----   ----------  ------------------------------------------------
//    +0x6c  0x100215a0  Init(RenderStructInit*) -- called right after
//    +0x70  0x100211d0  Term (0x004627d0 calls it)
//    +0x74  0x10039fe0  BindTexture(SharedTexture*, bUpload) (0x004628e0)
//    +0x78  0x1003aac0  UnbindTexture(SharedTexture*); clienttexmgr.cpp
//    +0x7c  0x10033160  CreateWorld              [clientmgr.h]
//    +0x80  0x10032c00  DestroyWorld             [clientmgr.h]
//    +0x84  0x100262f0  ClearScreen(LTRect*,flags) [clientmgr.h]
//    +0x88  0x10032e50  Start3D                  [clientmgr.h]
//    +0x8c  0x10032ea0  End3D                    [clientmgr.h]
//    +0x90  0x10032f20  IsIn3D                   [clientmgr.h]
//    +0x94  0x10034930  StartOptimized2D         [clientmgr.h]
//    +0x98  0x10034b00  EndOptimized2D           [clientmgr.h]
//    +0x9c  0x10034c60  a no-argument getter, never called -- NOT named
//    +0xa0  0x10034c70  SetOptimized2DBlend(LTSurfaceBlend)
//    +0xa4  0x10034ed0  GetOptimized2DBlend(LTSurfaceBlend&) -- dead
//    +0xa8  0x10034eb0  SetOptimized2DColor      [clientmgr.h]
//    +0xac  0x10034ef0  GetOptimized2DColor(HLTCOLOR&) -- dead
//    +0xb0  0x1002d1c0  RenderScene(SceneDesc*)
//    +0xb4  0x10032d00  RenderCommand(argc, argv) -- console commands
//    +0xb8  0x10020fd0  GetRenderObject(const char*) -- "LPDIRECTDRAW",
//                       "BACKBUFFER"; the answer is COM-called
//    +0xbc  0x100371e0  FlipScreen(uint32 dwFlags)
//    +0xc0  0x10032e40
//    +0xc4  0x10036df0  called once at renderer install, with 0x004b1758
//    +0xc8  0x100367b0
//    +0xcc  0x100368e0
//    +0xd0  0x10036920
//    +0xd4  0x10036950  LockSurface(pRenderData)
//    +0xd8  0x100369a0  UnlockSurface(pRenderData)
//    +0xdc  0x10034580  OptimizeSurface
//    +0xe0  0x10034510  UnoptimizeSurface
//    +0xe4  0x100369d0  LockScreen(l,t,r,b,&pixels,&pitch)
//    +0xe8  0x10036a90  UnlockScreen()
//    +0xec  0x10036cf0  BlitToScreen(RenderBlitRequest*)
//    +0xf0  0x10036da0  BlitWarpedToScreen(RenderBlitRequest*) -- the only
//                       blit whose answer is checked; zero falls back to
//                       the software path
//    +0xf4  0x10036e40  MakeScreenShot(const char* pFilename)
//    +0xf8  0x10021e50
//    +0xfc  0x10036ae0  BlitFromScreen(RenderBlitRequest*)
//
//  TWENTY-SIX of the thirty-seven now have names, and several have exact
//  signatures. Eleven came from `clientmgr.h`'s existing `g_pRender*`
//  globals; the rest were recovered by reconstructing the CLIENT functions
//  that call each slot, which is ordinary lithtech.exe work our toolchain
//  can verify -- and it pays into two targets at once.
//
//  One more bridge, found the same way: **`g_pRender` at 0x004b17a4 IS
//  `&RenderStruct`**. 0x0040eeb0 stores its argument there and copies that
//  object's +0x3c and +0x40 into g_ScreenWidth/g_ScreenHeight, and
//  0x4b4358+0x3c/+0x40 are exactly those. So the `call *0xNN(%reg)` sites
//  and the absolute `call *0x4b43xx` sites are the same slot space -- which
//  is how the eleven slots with no absolute reference (+0xc0..+0xe8) were
//  reached at all.
//
//  RenderStruct is **0x150 bytes**, and it has four parts. (An earlier note
//  here said "at least 0x20c", reading 0x004624ff's loads of +0x204/+0x208 as
//  RenderStruct fields. They are not: they come off the RMode argument, which
//  that instruction reloads from 0x300(%esp), and 0x204/0x208 are RMode's
//  m_Width/m_Height. The DESTINATIONS are RenderStruct +0x3c/+0x40. The size
//  is fixed instead by 0x00462110's `rep stos` of 0x54 dwords, and by the
//  HMODULE at 0x004b44a8 being the next object.)
//
//    +0x00..+0x38  FIFTEEN CALLBACKS THE ENGINE INSTALLS -- the traffic in
//                  the other direction. 0x00462110 writes every one from a
//                  literal: +0x04 LoadTexture, +0x08 GetTextureData,
//                  +0x0c FreeTextureData, +0x10 ConsolePrint,
//                  +0x14 ConsolePrintf, +0x18 GetConsoleVar,
//                  +0x1c GetVarValueFloat, +0x20 GetVarValueString,
//                  +0x34 dalloc, +0x38 dfree. Confirmed from d3d.ren's own
//                  side: its Init calls +0x18 with "windowed" then +0x20,
//                  and +0x18 with "SysMem" then +0x1c.
//    +0x3c/+0x40   m_Width / m_Height      +0x44  m_bRenderInited
//    +0x58..+0x68  five statistics counters, named by the console counter
//                  table at 0x0042bedd: TagVisibleLeaves, FlushObjectQueues,
//                  Models, WorldModels, Translucent, in that order
//    +0x6c..+0xfc  the renderer's 37 function pointers, above
//    +0x104        SharedTexture* environment map
//    +0x108        PanInfo[2]  (clientmgr.h's g_GlobalPanInfo)
//    +0x130        LTVector m_GlobalLightDir, defaulted to (0,-2,-1)/sqrt(5)
//    +0x13c        LTVector m_GlobalLightColor
//
//  AND THE VERSION HANDSHAKE, read off both binaries at once. Slot +0x6c is
//  `Init(RenderStructInit*)`, and RenderStructInit is 0x21c bytes:
//  `{ uint32 m_nVersion; RMode m_Mode; HWND m_hWnd; }`. The version word is
//  an OUT parameter -- d3d.ren's 0x100215a0 opens by storing 0xd5d into it,
//  and the client rejects the DLL if anything else comes back.
//
// ---------------------------------------------------------------------- //
