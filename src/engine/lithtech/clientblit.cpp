// *********************************************************************** //
//
//  MODULE:   CLIENTDRAW (.CPP)   (file name ours)
//
//  PURPOSE:  lithtech.exe — the client half of the engine<->renderer
//            boundary.  Everything here reaches d3d.ren through the
//            RenderStruct at 0x004b4358, so the comments carry as much
//            weight as the code: every slot touched is written down with
//            what the client PUSHES and what it does with the ANSWER,
//            because that is what names a function inside d3d.ren.
//
//  Build /O2 /Gy.
//
// *********************************************************************** //
//
//  THE RENDER TABLE, AS THIS RUN USES IT
//  -------------------------------------
//  0x004b4358 is the RenderStruct d3d.ren's RenderDLLSetup fills in
//  (see src/engine/d3d/d3d.cpp).  The client reaches it two ways and BOTH
//  name the same object:
//
//    * by absolute address, `call *0x4b43xx` — slots 0x6c..0xfc;
//    * through the pointer g_pRender at 0x004b17a4, `call *0xNN(%eax)` —
//      slots 0xc4..0xe8.
//
//  g_pRender IS &(the struct at 0x4b4358).  Proof, not assumption:
//  0x0040eeb0 takes a RenderStruct* argument, stores it into 0x004b17a4,
//  and copies its +0x3c and +0x40 into 0x004b1630 / 0x004b1634 — and
//  0x4b4358+0x3c / +0x40 are exactly 0x004b4394 / 0x004b4398, the
//  g_ScreenWidth / g_ScreenHeight clientmgr.h already names.  So a
//  displacement off g_pRender and an absolute 0x4b43xx are the same slot
//  arithmetic and can be pooled.
//
//  Slots this run pins down (all cdecl — every site cleans its own stack):
//
//    +0xb0  RenderScene(SceneDesc* pScene)                     0x004119f0
//           ONE argument: a ~0x160-byte SceneDesc built entirely on
//           sub_411700's stack (laid out in full below).  Answer IGNORED.
//           Guarded by g_bRenderInited AND by a second global 0x004a9eb8
//           being non-null; when that second global is 0 the whole scene is
//           still assembled and only the render call is skipped.
//
//    +0xb4  Fn_b4(a, b)                                        0x0041de93
//           Two dwords, cdecl, answer ignored.  Its one caller (0x0041de80)
//           is a two-argument cdecl forwarder that null-tests the struct
//           address first.
//
//    +0xb8  void* GetRenderObject(const char* pszName)         5 call sites
//           ONE argument, a STRING LITERAL: "LPDIRECTDRAW" (0x004a73c0) and
//           "BACKBUFFER" (0x004a73b4).  The answer is a pointer the caller
//           keeps and fails out on if null; at 0x00401e31 the BACKBUFFER
//           answer is handed straight to a COM-style `call *0x3c(%edi)`.
//           So this is the renderer's "hand me a named DirectDraw object"
//           accessor.  Every site tests g_bRenderInited first.
//
//    +0xbc  FlipScreen(uint32 dwFlags)                         0x00410949
//           ONE dword — CClientMgr::FlipScreen's own parameter, passed
//           through untouched.  Answer IGNORED.  See the body below.
//
//    +0xec  BlitToScreen(RenderBlitRequest* pReq)              0x0040d3d1,
//                                                              0x0040de44
//           ONE argument.  Reached only when the DESTINATION surface is the
//           screen surface (`cmp $0x4b1620,%edi`), and the request carries
//           the SOURCE surface's renderer data.  Answer IGNORED at both
//           sites; the caller unconditionally sets the destination's
//           CSurface::m_nFlags |= 4 afterwards (the "renderer has to
//           re-optimize me" bit).  The slot pointer is null-tested before
//           use, so a renderer may legally leave it empty.
//
//    +0xf0  BlitWarpedToScreen(RenderBlitRequest* pReq)        0x0040e8d7
//           ONE argument — the SAME request class as +0xec, with its two
//           trailing fields used: a point array and the count 4.  Unlike
//           +0xec the answer IS checked: non-zero means the renderer took
//           it (mark the dest dirty, return LT_OK), zero falls back to the
//           software path sub_40e330(dest, src, pts, 4, mode).
//
//    +0xfc  BlitFromScreen(RenderBlitRequest* pReq)            0x0040d432
//           ONE argument.  The mirror of +0xec: reached only when the
//           SOURCE is the screen surface, and the request carries the
//           DESTINATION surface's renderer data.  Answer IGNORED.  Also
//           null-tested before use.  No m_nFlags update — the surface being
//           written is the client's, not the renderer's.
//
//    +0xf4  Fn_f4(void* p)                                     0x00403971
//           ONE argument, the address of a stack local, in a `ret $0x10`
//           function with a 0x208-byte frame that loops to 3000.
//
//    +0xc4  Fn_c4(void* p)                                     0x0040eed0
//           Called once, from the renderer-install function 0x0040eeb0,
//           immediately after g_pRender is published; the argument is the
//           address 0x004b1758.
//
//    +0xd4  void* LockSurface(void* pRenderData)               5 call sites
//           ONE argument, CSurface::m_pRenderData.  Returns the pixel
//           pointer; the caller takes the PITCH from CSurface+0x18 rather
//           than from the renderer.  (That is what makes CSurface+0x18 the
//           pitch — clientmgr.h has it padded over.)
//
//    +0xd8  void UnlockSurface(void* pRenderData)              20 call sites
//           ONE argument.  Always the partner of a +0xd4 or +0xe4 lock.
//
//    +0xdc  LTBOOL OptimizeSurface(void* pRenderData,
//                                  HLTCOLOR hTransColor)       3 call sites
//           Two arguments — already in clientmgr.h, and sub_40d7d0 below is
//           its guard.
//
//    +0xe0  void UnoptimizeSurface(void* pRenderData)          0x0040ce4b
//
//    +0xe4  LTBOOL LockScreen(int left, int top, int right, int bottom,
//                             void** ppPixels, uint32* pPitch)  10 sites
//           SIX arguments, pushed right-to-left as
//               &pitch, &pixels, g_ScreenHeight(0x4b1634),
//               g_ScreenWidth(0x4b1630), 0, 0
//           so the rect is the whole screen at every site in this run.
//           Answer is a flag: non-zero means the two out-params are filled.
//
//    +0xe8  void UnlockScreen()                                16 sites
//           NO arguments — `call *0xe8(%edx)` with nothing pushed and no
//           stack cleanup.  Partner of +0xe4.
//
//  So 0xd4/0xd8 are the surface lock pair, 0xe4/0xe8 the screen lock pair,
//  0xdc/0xe0 the optimize pair, and 0xec/0xf0/0xfc the three
//  hardware-accelerated blit entries that let the renderer do a copy the
//  software path would otherwise do pixel by pixel.
//
// *********************************************************************** //

#include "ltbasedefs.h"


// ----------------------------------------------------------------------- //
//  Views.  Every offset below is a shipped displacement; the offsets that
//  overlap clientmgr.h / clientmgr2.cpp agree with them.
// ----------------------------------------------------------------------- //

// clientmgr.h's CSurface, plus the two fields this run proves:
//   +0x18 is the pitch (sub_40d290 reads it into the slot the renderer's
//         own LockScreen would have filled), and
//   +0x1c bit 2 (0x4) is the "the renderer has to optimize me again" bit —
//         set after every accelerated blit, cleared by sub_40d7d0 once
//         OptimizeSurface has run.
class CSurfaceView
{
public:
	uint8    m_pad_00[0x0c];
	void*    m_pRenderData;                  // 0x00c
	uint32   m_nWidth;                       // 0x010
	uint32   m_nHeight;                      // 0x014
	uint32   m_nPitch;                       // 0x018
	uint32   m_nFlags;                       // 0x01c
	HLTCOLOR m_hTransColor;                  // 0x020
	void*    m_pUserData;                    // 0x024
	float    m_fAlpha;                       // 0x028
};

// The sub-object at CClientMgr+0x11f0.  FlipScreen calls one thiscall
// method on it and nothing else here reaches inside, so only the method
// exists.  (0x00429e60 walks a list off +0x20 / +0x1c and returns a flag
// nobody here reads.)
class CSub11F0
{
public:
	void Update();                                // 0x00429e60
};

// CClientMgr, as far as this file reaches it.
class CClientMgrView
{
public:
	uint8            m_pad_00[0x1c8];
	// clientmgr2.cpp: the renderer's per-world object, made through
	// +0x7c and given back through +0x80.
	void*            m_pRenderWorld;              // 0x0001c8
	uint8            m_pad_1cc[0x11f0 - 0x1cc];
	CSub11F0         m_Sub11f0;                   // 0x0011f0

	// 0x00410930  (clientmgr.h declares it on CClientMgr)
	void FlipScreen(uint32 dwFlags);
};

// The object at CClientMgr+0x150c.  Two vptrs (clientmgr2.cpp); nothing
// here calls one of its own virtuals, so they stay plain fields.
class CClientShell
{
public:
	void**           m_pVtable00;                 // 0x000
	void**           m_pVtable04;                 // 0x004
	uint8            m_pad_08[0x78 - 0x08];
	CClientMgrView*  m_pClientMgr;                // 0x078

	// 0x00414860 / 0x004148a0
	LTBOOL CreateRenderWorld();
	void   DestroyRenderWorld();
};


// ----------------------------------------------------------------------- //
//  The render table, as this file reaches it.
// ----------------------------------------------------------------------- //

// 0x004b439c = RenderStruct+0x44.  Every site that is about to touch a
// render slot tests this first; d3d.ren sets it in RenderDLLSetup.  It and
// the +0xbc slot below are fields of clientrender.cpp's g_RenderStruct
// (0x004b4358), reached through that definition, not globals of their own.
class RenderStruct
{
public:
	uint8  m_pad_00[0x44];
	int    m_bInited;                                            // 0x044  0x004b439c
	uint8  m_pad_48[0xbc - 0x48];
	void   (*FlipScreen)(uint32 dwFlags);                        // 0x0bc  0x004b4414
};
extern RenderStruct g_RenderStruct;                                  // 0x004b4358

// +0x7c / +0x80 — clientmgr2.cpp's names.  The maker takes the ADDRESS of a
// one-field request struct, not the manager itself: the shipped code
// reserves four bytes with `push %ecx`, stores the manager into them and
// pushes their address.
struct RenderWorldRequest { CClientMgrView* m_pClientMgr; };
extern void* (*g_pRenderCreateWorld)(RenderWorldRequest* pRequest);  // 0x004b43d4
extern void  (*g_pRenderDestroyWorld)(void* pRenderWorld);           // 0x004b43d8

// +0xbc.  One dword in, nothing out.  (name ours) -- RenderStruct::FlipScreen.

// +0xdc — clientmgr.h already declares this one on RenderStruct; here it is
// as the flat pointer the 0x004b17a4 path resolves to.
class RenderStructView
{
public:
	uint8  m_pad_00[0xdc];
	LTBOOL (*OptimizeSurface)(void* pRenderData, HLTCOLOR hTransColor);  // 0x0dc
};
// The pointer itself is cissurf.cpp's RenderStructQ40C* (declared, not
// included); this TU reads the one slot through the view.
class RenderStructQ40C;
extern RenderStructQ40C* g_pRender;                                  // 0x004b17a4


// ----------------------------------------------------------------------- //
//  0x00414860 — CClientShell::CreateRenderWorld
//
//  WHY THIS IS HERE AND NOT LEFT ALONE.  src/engine/lithtech/clientmgr2.cpp
//  already carries this function and it does not reproduce.  A diff against
//  the shipped bytes (ship 63 / ours 96) says why, and it is one fault, not
//  a shape problem:
//
//      S  mov 0x78(%ecx),%esi        <- the manager, loaded ONCE
//      O  mov %ecx,%esi              <- `this`, kept instead
//      O  mov 0x78(%esi),%eax        ...and m_pClientMgr reloaded at every
//      O  mov 0x78(%esi),%ecx           mention, three times over
//
//  Writing `m_pClientMgr->m_pRenderWorld` at each mention makes VC6 hold
//  `this` in the callee-saved register and reload the member; the shipped
//  code holds the MEMBER in that register across the call, which only a
//  local can express.  So the source cached it:
//
//      CClientMgrView* pClientMgr = m_pClientMgr;
//
//  The second correction is the shape.  The shipped tail has exactly ONE
//  `mov $0x1,%eax; pop; pop; ret` block, jumped to from all three arms, and
//  the LTFALSE arm falls out through `pop %esi; pop %ecx; ret` with no
//  `xor %eax,%eax` in front of it — eax is already zero from the `test`
//  that branched.  Three guarded `return LTTRUE`s let VC6 if-convert the
//  last pair into `setne` (that is what the existing version emits).  ONE
//  positive condition wrapping the body hosts the shared answer at the END,
//  which is the layout the shipped bytes have.
//
//  RENDER SLOT +0x7c: pushed a pointer to a four-byte stack struct holding
//  the CClientMgr, cdecl (`push %eax` / `add $0x4,%esp` merged into the
//  frame teardown).  The answer is the renderer's per-world object and it
//  is STORED, at CClientMgr+0x1c8; a null answer is the only failure this
//  function reports.
// ----------------------------------------------------------------------- //
// LTBOOL CClientShell::CreateRenderWorld -- REMOVED: src/engine/lithtech/clientmgr2.cpp owns this
// and both functions now match byte-identically there, using the
// finding (the manager into a block-scoped local).


// ----------------------------------------------------------------------- //
//  0x004148a0 — CClientShell::DestroyRenderWorld
//
//  Same single fault as its partner, and the same fix.  The diff:
//
//      S  mov 0x78(%ecx),%esi
//      O  mov %ecx,%esi ... mov 0x78(%eax),%eax ... mov 0x78(%esi),%ecx
//      S  movl $0x0,0x1c8(%esi)
//      O  movl $0x0,0x1c8(%ecx)     <- a third reload just to store the 0
//
//  With the manager in a local, one register holds it across the call and
//  the store lands on the same register the test loaded from.
//
//  RENDER SLOT +0x80: one argument, the per-world object handed back by
//  +0x7c.  Answer IGNORED (`add $0x4,%esp` and straight into the store).
//  No g_bRenderInited test here — the field is the whole state.
// ----------------------------------------------------------------------- //
// void CClientShell::DestroyRenderWorld -- REMOVED: src/engine/lithtech/clientmgr2.cpp owns this
// and both functions now match byte-identically there, using the
// finding (the manager into a block-scoped local).


// ----------------------------------------------------------------------- //
//  0x00410930 — CClientMgr::FlipScreen  (clientmgr.h names it; the client
//  interface reaches it as `g_pClientMgr->FlipScreen(flags)` from
//  ci_FlipScreen, 0x00408c90.)
//
//  What pins it: `ret $0x4` with no prologue at all — one dword parameter,
//  thiscall, and `this` is used only to reach the sub-object at +0x11f0.
//  35 bytes, three statements.
//
//  RENDER SLOT +0xbc: `mov 0x4(%esp),%eax; push %eax; call *0x4b4414;
//  pop %ecx`.  The one argument is this function's own dwFlags, forwarded
//  untouched — the SDK's FlipScreen flags reach d3d.ren verbatim.  Answer
//  IGNORED, and the call is skipped entirely when g_bRenderInited is 0
//  while the +0x11f0 update still runs.
//
//  CONFIDENCE: the body is small enough that most of it is relocation
//  (two calls and one global), so its call targets are worth checking
//  before it is counted.
// ----------------------------------------------------------------------- //
void CClientMgrView::FlipScreen(uint32 dwFlags)
{
	m_Sub11f0.Update();

	if (g_RenderStruct.m_bInited)
		g_RenderStruct.FlipScreen(dwFlags);
}


// ----------------------------------------------------------------------- //
//  0x0040d7d0 — re-optimize a surface if the renderer marked it dirty.
//  (name ours; not enumerated, but it is the gate in front of every
//  accelerated blit and it is 53 bytes.)
//
//  What pins it: one stack argument and a plain `ret`, so cdecl with one;
//  the two `test $0x2,%al` / `test $0x4,%al` against ONE load of +0x1c are
//  an `&&`, not a mask compare; and the tail `and $0xfb,%al` clears exactly
//  the bit the blits set.
//
//  RENDER SLOT +0xdc: two arguments, (CSurface::m_pRenderData,
//  CSurface::m_hTransColor), through g_pRender.  Answer IGNORED here even
//  though clientmgr.h types it LTBOOL.
//
//  This is what bit 2 of CSurface::m_nFlags MEANS: the three accelerated
//  blit slots set it on whatever surface they wrote, and the next blit that
//  wants to READ that surface has to hand it back to OptimizeSurface first.
// ----------------------------------------------------------------------- //
void sub_40d7d0(CSurfaceView* pSurface)
{
	if (pSurface && (pSurface->m_nFlags & 2) && (pSurface->m_nFlags & 4))
	{
		((RenderStructView*)g_pRender)->OptimizeSurface(pSurface->m_pRenderData,
		                                                pSurface->m_hTransColor);
		pSurface->m_nFlags &= ~4;
	}
}


// *********************************************************************** //
//
//  NOT WRITTEN, BUT MEASURED — layout facts for whoever takes the four big
//  ones.  None of these is a guess; each offset below was read off a
//  shipped displacement.
//
// *********************************************************************** //
//
//  A.  THE BLIT REQUEST the three screen-blit slots take.
//  ------------------------------------------------------
//  0x0040d290 (+0xec and +0xfc), 0x0040dcc0 (+0xec) and 0x0040e4d0 (+0xf0)
//  all build the SAME object on the stack and all three initialise it the
//  same way at the TOP of the function, before any argument checking:
//
//      +0x00 = 0 ; +0x04 = 0 ; +0x08 UNTOUCHED ; +0x0c = 0 ; +0x10 = 0 ;
//      +0x14 = 0x3f800000 (1.0f)
//
//  Five of six fields set, one skipped, a float defaulted to 1.0 — that is
//  a CONSTRUCTOR, and it is inlined (no out-of-line body exists), so it
//  belongs inside the class body.  The frames are
//      0x0040d290  body+0x30      0x0040dcc0  body+0x3c
//      0x0040e4d0  body+0x78 (two extra fields, not constructed)
//
//      class RenderBlitRequest              // 0x18 bytes, 0x20 with the tail
//      {
//      public:
//          RenderBlitRequest()
//          {
//              m_pSurfaceData = LTNULL;
//              m_bTransparent = 0;
//              m_pSrcRect     = LTNULL;
//              m_pDestRect    = LTNULL;
//              m_fAlpha       = 1.0f;
//          }
//          void*   m_pSurfaceData;   // +0x00  CSurface::m_pRenderData
//          uint32  m_bTransparent;   // +0x04  see below
//          void*   m_pUnknown08;     // +0x08  = g_Sub4b161c (0x004b161c),
//                                    //        the only field the ctor skips
//          void*   m_pSrcRect;       // +0x0c  16 bytes on the caller's stack
//          void*   m_pDestRect;      // +0x10  16 bytes on the caller's stack
//          float   m_fAlpha;         // +0x14  CSurface::m_fAlpha
//          void*   m_pPoints;        // +0x18  0x0040e4d0 only
//          uint32  m_nPoints;        // +0x1c  0x0040e4d0 only: literal 4
//      };
//
//  m_pSurfaceData is always the surface that is NOT the screen: the source
//  for +0xec / +0xf0, the destination for +0xfc.
//
//  m_bTransparent is derived differently at each site and that difference
//  is worth keeping, because it says what the two blitter callbacks are:
//      0x0040d290:  sete on (pBlitFn == 0x0040d710)   — so 0x0040d710 is
//                   the transparent per-pixel blitter and 0x0040d660 the
//                   opaque one (those are the only two values that reach
//                   the accelerated path at all: the function bails to the
//                   software route unless pBlitFn is one of the two).
//      0x0040dcc0:  sete on (param5 == 0)
//      0x0040e4d0:  setne on (arg9 != 0) — arg9 is the same value the
//                   software fallback sub_40e330 gets as ITS last argument,
//                   i.e. clientmgr.h's nMode for that function
//
//  m_pSrcRect / m_pDestRect point at two 16-byte blocks the caller zeroes
//  at entry and then fills.  In 0x0040dcc0 they are unambiguous: the DEST
//  block defaults to { 0, 0, hDest->m_nWidth, hDest->m_nHeight } and the
//  SRC block to { 0, 0, hSrc->m_nWidth, hSrc->m_nHeight } when the
//  respective LTRect* parameter is null — which also confirms
//  clientmgr.h's guessed parameter order for that function (param2 is the
//  destination rect, param3 the source rect).
//
//  B.  THE SceneDesc slot +0xb0 takes, from 0x00411700.
//  ----------------------------------------------------
//  0x00411700 is `LTBOOL RenderCamera(CClientMgr* pClientMgr,
//  CClientObject* pCamera, uint32 nDrawMode, X p3, Y p4)` — five cdecl
//  parameters, `ret` (caller cleans).  It opens by constructing something
//  at frame+0xa4 with 0x00411a30(this, 0, 0, 0, 0), refuses when
//  g_bRenderInited is 0 or when CClientMgr+0x152c is already 1 (a
//  re-entrancy guard it sets to 1 around the render and clears after),
//  then fills a ~0x160-byte stack struct and hands ONE pointer to it to
//  slot +0xb0.  Answer ignored.  Afterwards it bumps CClientMgr+0x158c
//  (a frame counter) and zeroes 0x004b44a4.
//
//  The struct, offset by offset (frame base = the address pushed):
//
//    +0x000  uint32     nDrawMode           <- parameter 2, verbatim
//    +0x004  void*      = 0x004b15a4        } six .data addresses, pushed
//    +0x008  void*      = 0x004b15a0        } as constants — six pointers
//    +0x00c  void*      = 0x004b1598        } to engine-side counters or
//    +0x010  void*      = 0x004b159c        } console variables the
//    +0x014  void*      = 0x004b1594        } renderer writes back into
//    +0x018  void*      = 0x004b1590        }
//    +0x01c  uint32     = [0x004b3228]      } six .data VALUES, read out
//    +0x020  uint32     = [0x004b322c]      } (0x4b3218..0x4b3230 is a
//    +0x024  uint32     = [0x004b3230]      }  contiguous block of six
//    +0x028  uint32     = [0x004b3218]      }  dwords, so these are almost
//    +0x02c  uint32     = [0x004b321c]      }  certainly render-option
//    +0x030  uint32     = [0x004b3220]      }  console variables)
//    +0x034  void*      the renderer's world object — reached as
//                       pClientMgr->m_pClientShell2->GetClientMgr()
//                           ->m_pRenderWorld (0x1c8), via TWO calls to
//                       CClientShell::GetClientMgr (0x00414b10); 0 when
//                       there is no shell
//    +0x038  LTVector   pClientMgr+0x75c
//    +0x044  LTVector   pClientMgr+0x750
//    +0x050  LTVector   pClientMgr+0x768   <- clientmgr.h's
//                                             m_vGlobalLightScale
//    +0x05c  LTVector   pCamera+0x1c4      <- CClientCamera::m_LightAdd
//    +0x068  uint32     pClientMgr+0x1258
//    +0x06c  SkyDef     pClientMgr+0x11a0, copied BY VALUE with
//                       `rep movsl` count 12 — 48 bytes, exactly the
//                       SkyDef clientmgr.h already models
//    +0x09c  HOBJECT*   = &this->[+0x0ec] — points INTO the struct
//    +0x0a0  uint32     how many entries that array got
//    +0x0a4  LTRect     pCamera+0x1a8 (m_Rect) when
//                       pCamera->m_bFullScreen (+0x1c0) is 0, otherwise
//                       { 0, 0, g_ScreenWidth, g_ScreenHeight }
//    +0x0b4  float      pCamera+0x1b8 (m_fFovX)
//    +0x0b8  float      pCamera+0x1bc (m_fFovY)
//    +0x0bc  uint32     = [0x004a9f00]
//    +0x0c0  LTVector   pCamera+0x18c (m_Pos)
//    +0x0cc  LTRotation pCamera+0x09c (m_Rotation), four floats, stored
//                       x, z, y, w — 0x9c->+0xcc, 0xa4->+0xd4,
//                       0xa0->+0xd0, 0xa8->+0xd8
//    +0x0dc  parameter 3
//    +0x0e0  parameter 4
//    +0x0e4  ModelHookFn pClientMgr+0x1524
//    +0x0e8  void*       pClientMgr+0x1528
//    +0x0ec  HOBJECT[]   the draw list, filled in place
//
//  The draw list is built from THIRTY uint16 ids at pClientMgr+0x1160,
//  walked with a `mov (%esi),%dx` / `add $2,%esi` / 30-count loop; each is
//  passed as `sub_420520(pClientMgr, id)` and appended only when that
//  returns non-null.  clientmgr2.cpp models the same storage as fifteen
//  int32s set to -1 with a `rep stos` of 15 — thirty 0xffff halves, which
//  is consistent, but the reader here is 16-bit and its bound is 30.
//
//  C.  What is at 0x004b1620.
//  --------------------------
//  clientmgr.h calls 0x004b1620 `int g_ScreenSurface`.  Every use in this
//  run is `cmp $0x4b1620,%reg` against a CSurface* — the ADDRESS, not the
//  contents.  So 0x004b1620 is the screen's CSurface OBJECT, statically
//  allocated, and "is this handle the screen?" is a pointer comparison
//  against it.  Every accelerated blit decision in 0x0040d290 /
//  0x0040dcc0 / 0x0040e4d0 turns on that one test.
//
// *********************************************************************** //
