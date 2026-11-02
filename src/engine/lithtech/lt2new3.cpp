// *********************************************************************** //
//
//  MODULE:   LTNEW3 (.CPP)  -- scratch file, NOT a drop-in TU
//
//  PURPOSE:  The 25-address 37..42-byte enumeration out of lithtech.exe.
//            Sixteen are written below; nine are reported at the foot of
//            this file (three mid-function fragments, two compiler
//            generated dynamic initialisers, two mis-sized rows that are
//            really ~260 and ~526 bytes, one address that is not a function
//            at all, and one that the repo ALREADY BUILDS from
//            src/lithshared/stdlith/struct_bank.cpp).
//
//  THE "FAMILY" THE BATCH SIZE HINTED AT turned out to be four small ones,
//  not one:
//
//    1. THREE INTERFACE SLOTS off two vtables that the earlier passes had
//       already anchored -- CLTClient's ILTClient/ILTCSBase table at
//       0x004a14e0 and the eight-slot ILTCursor front end that begins at
//       0x004a1698.  ILTCSBase's own bodies at slots 1-30/56-70 and the
//       named slots 47-54 pin the whole run, so slot 36 IS
//       ResetModelAnimation, slot 88 IS StartMessage, and cursor slot 3 IS
//       LoadCursorBitmapResource.  No guessing was needed for any of them.
//
//    2. A CONSTRUCTOR/DESTRUCTOR SET -- 0x0048ac40, 0x00490210, 0x004441e0
//       are all ctor/dtor bodies, each identified by the vptr immediate it
//       writes and by the scalar-deleting wrapper that calls it.
//
//    3. ONE EXACT TWIN PAIR -- 0x0048a590 and 0x0048a5c0 are the same 41
//       bytes with one displacement changed (0x20 against 0x24).  They are
//       two static callback descriptors' entry points, at 0x004af4d8 and
//       0x004af4e0.
//
//    4. THE REST are singletons: a struct-bank wrapper, a palette reader, a
//       console command, a varargs error setter, a list search, a surface
//       unlock guard and CNetMgr::FreeSessionList.
//
//  GROUND TRUTH: the retail lithtech.exe only, cl 8447 /O2.
//
//  ONE FUNCTION HERE IS SHARED CODE: 0x004556e0 is CNetMgr::FreeSessionList,
//  already DECLARED (and not defined) at src/engine/server/servermgr.h:1204.
//  It belongs in src/engine/server/netmgr.cpp; only lithtech.exe calls it,
//  but that file is compiled into server.dll too, so both images must be
//  re-measured after moving it.  Nothing else below is shared:
//  every other body was checked against the retail server.dll's address
//  space by way of its callers, and all of them sit in client-only runs.
//
// *********************************************************************** //

#include <windows.h>            // CloseHandle
#include <stdlib.h>             // atoi
#include <string.h>             // stricmp
#include <stdarg.h>             // va_list / va_start

#include "ltbasedefs.h"         // LTRESULT, LTLink, NetSession, HMESSAGEWRITE
#include "ltcodes.h"            // LT_OK, LT_NOTINITIALIZED
#include "ltlink.h"             // dl_TieOff
#include "iltcursor.h"          // ILTCursor, HLTCURSOR
#include "ltanimtracker.h"      // LTAnimTracker
#include "iltmodel.h"           // ILTModel


// ======================================================================= //
//
//  A.  THE THREE VTABLE SLOTS
//
//  ANCHORING, ONCE, FOR ALL THREE.  A vtable scan on 0x004a14e0 gives a 329-slot
//  run that is several tables back to back.  The seams and the anchors:
//
//    * slots 0..70 are ILTCSBase in iltcsbase.h declaration order.  Slots
//      1-30 (0x0041ec90..0x0041f5f0) and 56-70 (0x0041f640..0x0041fb80) are
//      ILTCSBase's OWN non-pure bodies, 45 slots of free anchor.  Independent
//      confirmations inside that range: slot 47 -> 0x00407a70 FreeString,
//      48/49 -> CompareStrings/CompareStringsUpper, 50 -> GetStringData,
//      53/54 -> GetTime/GetFrameTime (`flds 0x125c` / `flds 0x1258` off
//      g_pClientMgr at 0x004b17e0 -- 0x00407b40/0x00407b50 verified here).
//      => slot 36 is iltcsbase.h index 36 = ResetModelAnimation.
//
//    * slot 71 is CLTClient's destructor and slot = iltclient.h index + 71
//      from there.  Confirmed on the spot: slot 90 -> 0x00407590, whose own
//      error site pushes the literal "CLTClient::EndMessage2" (0x004a7c2c),
//      and iltclient.h puts EndMessage2 at index 19; 19 + 71 = 90.
//      => slot 88 is index 17 = StartMessage.
//
//    * the table ENDS at slot 109.  Slot 110 (0x004a1698) begins an
//      eight-slot table whose eight bodies are 0x00405dc0, 0x00405df0,
//      0x00405e20, 0x00405e50, 0x00405e80, 0x00405eb0, 0x00405ee0,
//      0x00405f10 -- all 0x30 bytes apart, all
//      `if (!this->+0x04) return 73;` then a forward through
//      `(*(this->+0x04 + 0x1614))->slot[i]` for i = 0..7 IN ORDER (call
//      displacements 0x00,0x04,0x08,0x0c,0x10,0x14,0x18,0x1c).  iltcursor.h
//      declares exactly eight pure virtuals and the argument counts agree
//      slot for slot: 1,1,1,2,1,1,1,0 -- and ours, slot 3, is the only
//      TWO-argument one in the run (`ret $0x8` against seven `ret $0x4`).
//      => 0x00405e50 is LoadCursorBitmapResource and can be nothing else.
//      (ltnew1.cpp already wrote slot 7 of this same table.)
//
// ======================================================================= //


// ----------------------------------------------------------------------- //
//  A1.  0x00405e50 -- the client's ILTCursor front end, slot 3
//       LoadCursorBitmapResource(const char *pName, HLTCURSOR &hCursor)
//
//  BELONGS IN: the client TU that holds 0x00405dc0..0x00407c2x (the
//  ILTClient slot bodies).  ltnew1.cpp's CClientCursorProxy is the same
//  class; the name is repeated here because this is a separate scratch TU.
// ----------------------------------------------------------------------- //

// CClientMgr as far as this reaches it.  cltcursor.cpp already proves the
// field: 0x0040fbdb in CClientMgr::Init files the CLTCursor it makes at
// CClientMgr+0x1614.
class CClientMgrCursorView3
{
public:
	uint8      m_pad_00[0x1614];
	ILTCursor* m_pCursor;                                  // 0x1614
};

class CClientCursorProxy3
{
public:
	LTRESULT LoadCursorBitmapResource(const char* pName,
	                                  HLTCURSOR& hCursor);  // 0x00405e50

public:
	uint8                  m_pad_00[0x04];                 // 0x000  the vptr
	CClientMgrCursorView3* m_pClientMgr;                   // 0x004
};

// WHAT PINS IT
//   * `ret $0x8` on BOTH exits -> __thiscall member, two stack arguments.
//   * `mov 0x4(%ecx),%eax; test %eax,%eax; jne` with `mov $0x49,%eax;
//     ret $8` INLINE right after the jne -> an early return, not a guarded
//     body.  0x49 is 73 = LT_NOTINITIALIZED.  (Contrast A2 below, where the
//     failure block sits at the FAR end and the source is a positive
//     guard.)
//   * the two arguments are re-read through the moving stack: `mov
//     0x8(%esp),%edx; push %edx` picks up parameter 2 (hCursor) FIRST, then
//     `mov 0x8(%esp),%edx; push %edx` -- same displacement, esp now four
//     lower -- picks up parameter 1 (pName).  Right-to-left, so the callee's
//     list is (pName, hCursor), which is iltcursor.h's order.
//   * `call *0xc(%eax)` = slot 3 of ILTCursor, and it is a CALL with
//     `ret $0x8`, not a `jmp`: VC6 will not tail-jump when the callee has
//     stack arguments to clean.  Slot 7 (RefreshCursor, zero arguments) in
//     the same run IS a `jmp *0x1c(%eax)`, which is the control.
LTRESULT CClientCursorProxy3::LoadCursorBitmapResource(const char* pName,
                                                       HLTCURSOR& hCursor)
{
	if (!m_pClientMgr)
		return LT_NOTINITIALIZED;

	return m_pClientMgr->m_pCursor->LoadCursorBitmapResource(pName, hCursor);
}


// ----------------------------------------------------------------------- //
//  A2.  0x00407560 -- CLTClient, ILTClient slot 88 = StartMessage
//  A3.  0x00407730 -- CLTClient, ILTCSBase slot 36 = ResetModelAnimation
//
//  BELONGS IN: the same unwritten client TU as A1.
// ----------------------------------------------------------------------- //

// The message the client's message manager hands out.  The MEMBER OFFSETS
// are not guesses: src/engine/server/servermsg.h's LMessageImpl -- the same
// class, read off server.dll -- puts m_MessageID at +0x1c and the owning
// queue entry's back pointer at +0x08, and this function writes +0x1c of a
// pointer that is exactly (allocator result + 8).  So the allocator hands
// back the CONTAINER and the LMessageImpl is its member at +0x08.  The
// reader agrees: CLTClient::EndMessage2 (0x00407590) does
// `mov 0x1c(%esi),%cl` on that same pointer and passes the byte on as the
// message id.
class CLTMessageView
{
public:
	uint8  m_pad_00[0x1c];
	uint32 m_MessageID;                                    // 0x1c
};

class CLTMessageEntryView
{
public:
	uint8          m_pad_00[0x08];
	CLTMessageView m_Message;                              // 0x08
};

// 0x004113e0 -- __thiscall on the sub-object at CLTClient+0x3c0, no
// arguments, returns the container or NULL.  (It builds a 0x1000-byte
// buffer, seeds the container's +0x0c from mgr+0x4bc -- the object-IO hook
// servermsg.h calls m_pObjectIO at LMessageImpl+0x04 -- and sets bit 0 of
// +0x40.)
class CClientMessageMgrView
{
public:
	CLTMessageEntryView* Alloc();                          // 0x004113e0
};

class CLTClientView3
{
public:
	HMESSAGEWRITE StartMessage(uint8 messageID);           // 0x00407560
	LTRESULT      ResetModelAnimation(HLOCALOBJ hObj);     // 0x00407730

public:
	uint8                 m_pad_00[0x10];
	// iltcsbase.h's aggregate pointers sit at +0x08..+0x1c after the vptr
	// and the four-byte embedded ILTMath: m_pCommonLT, m_pPhysicsLT,
	// m_pModelLT, m_pTransformLT, m_pLightAnimLT, m_pSoundMgr.  ltnew2.cpp
	// pinned +0x14 as m_pTransformLT from the constructor's own stores, so
	// +0x10 is m_pModelLT -- and the two slots this body calls, 0x28 and
	// 0x44, are iltmodel.h's GetMainTracker and ResetAnim.
	ILTModel*             m_pModelLT;                      // 0x10
	uint8                  m_pad_14[0x3c0 - 0x14];
	// A POINTER, not an embedded object: the shipped code is
	// `mov 0x3c0(%ecx),%ecx`, which loads the member.  An embedded object
	// would be `lea 0x3c0(%ecx),%ecx`.
	CClientMessageMgrView* m_pMessageMgr;                  // 0x3c0
};

// WHAT PINS IT (0x00407560)
//   * `ret $0x4` on both exits -> member, one stack argument.
//   * `mov 0x3c0(%ecx),%ecx; call 0x4113e0` -- %ecx is loaded FROM the
//     member, so the allocator is a __thiscall member of an object held BY
//     POINTER at +0x3c0... except that no `lea` appears, so the member IS
//     the pointer.  (An embedded object would be `lea 0x3c0(%ecx),%ecx`.)
//     Zero arguments: nothing is pushed and nothing is cleaned.
//   * `test %eax,%eax; je 0x407582` with the `xor %eax,%eax; ret $4` block
//     at the FAR END -> the positive-guard shape, `if (p) { ...; return x; }
//     return LTNULL;`.  Writing `if (!p) return LTNULL;` puts that block
//     INLINE right after the test, which is what A1 above actually has --
//     the two shapes are side by side in this file and they differ.
//   * `add $0x8,%eax` is computed ONCE and used both for the store and for
//     the return value -> one lvalue, `&pEntry->m_Message`.
//   * `mov 0x4(%esp),%ecx; and $0xff,%ecx; mov %ecx,0x1c(%eax)` is a FOUR
//     byte store of a zero-extended byte parameter: the field is uint32 and
//     the parameter is uint8, which is exactly iltclient.h's
//     `StartMessage(uint8 messageID)`.  The control is one interface slot
//     away: ILTCSBase::WriteToMessageByte (0x0041ece0) has the same uint8
//     parameter and does NOT mask it -- because there it is forwarded to
//     another uint8 parameter and never widened.
HMESSAGEWRITE CLTClientView3::StartMessage(uint8 messageID)
{
	CLTMessageEntryView* pEntry = m_pMessageMgr->Alloc();

	if (pEntry)
	{
		CLTMessageView* pMsg = &pEntry->m_Message;

		pMsg->m_MessageID = messageID;

		return (HMESSAGEWRITE)pMsg;
	}

	return LTNULL;
}

// WHAT PINS IT (0x00407730)
//   * `ret $0x4` -> member, one stack argument.
//   * `lea 0x8(%esp),%ecx` with NO `sub` in the prologue: after the single
//     `push %esi` that is the parameter's own stack slot, reused as the
//     out-parameter's home.  A local whose address is taken and whose
//     parameter is dead is exactly what VC6 puts there; there is no second
//     local, so the source has one.
//   * the two slots are 0x28 and 0x44 of the interface at this+0x10.
//     iltmodel.h: 0x28 is GetMainTracker(HOBJECT, LTAnimTracker*&) -- two
//     arguments, which is what is pushed -- and 0x44 is
//     ResetAnim(LTAnimTracker*) -- one.  The reference parameter is why the
//     address of the local is what gets pushed.
//   * `test %eax,%eax; jne 0x407755` where 0x407755 is the epilogue, and
//     %eax is not touched between the second call and the `ret`: the SAME
//     variable carries both results out.  Two separate statements
//     (`if (m_pModelLT->GetMainTracker(...) == LT_OK) m_pModelLT->
//     ResetAnim(...); return LT_OK;`) would need an `xor %eax,%eax` before
//     the ret; there is none.
LTRESULT CLTClientView3::ResetModelAnimation(HLOCALOBJ hObj)
{
	LTAnimTracker* pTracker;
	LTRESULT       dResult;
	ILTModel*      pModelLT = m_pModelLT;

	dResult = pModelLT->GetMainTracker(hObj, pTracker);
	if (dResult == LT_OK)
		dResult = pModelLT->ResetAnim(pTracker);

	return dResult;
}


// ======================================================================= //
//
//  B.  0x0043afc0 -- the surface-unlock guard
//
//  BELONGS IN: the client blit/surface TU.  src/engine/lithtech/
//  clientblit.cpp already carries the two facts this needs and neither is
//  re-derived here:
//     * 0x004b17a4 is g_pRender, the RenderStruct the .ren publishes;
//       +0xd8 is `void UnlockSurface(void *pRenderData)` (20 call sites,
//       one argument, always CSurface::m_pRenderData) and +0xe8 is
//       `void UnlockScreen()` (16 sites, NO arguments and no cleanup).
//     * 0x004b1620 is the screen's CSurface OBJECT, statically allocated;
//       every use in the run is `cmp $0x4b1620,%reg` against a CSurface*,
//       i.e. the ADDRESS.
//  So this is the "unlock whichever kind of surface this is" helper that
//  the two big blit paths at 0x0043af25 and 0x0043b1db call.
//
// ======================================================================= //

class CSurfaceView3
{
public:
	uint8  m_pad_00[0x0c];
	void*  m_pRenderData;                                  // 0x0c
};

class RenderStructUnlockView
{
public:
	uint8 m_pad_00[0xd8];
	void  (*UnlockSurface)(void* pRenderData);             // 0xd8
	uint8 m_pad_dc[0xe8 - 0xdc];
	void  (*UnlockScreen)();                               // 0xe8
};

// cissurf.cpp defines both with its own class names; these one-line views
// carry those names so the references link against the definitions.
class RenderStructQ40C : public RenderStructUnlockView {};
class CSurfaceQ40C : public CSurfaceView3 {};
extern RenderStructQ40C* g_pRender;                        // 0x004b17a4
extern CSurfaceQ40C      g_ScreenSurface;                  // 0x004b1620

// WHAT PINS IT
//   * bare `ret` after `pop %ecx` -> __cdecl free function, one argument,
//     void.  The `pop %ecx` (not `add $0x4,%esp`) is the one-dword cleanup.
//   * `cmp $0x4b1620,%eax; jne 0x43afd6` -- the EQUAL arm falls through, so
//     the screen case is the `if` and the surface case is the `else`.
//     Inverting the test (`if (p != &g_ScreenSurface)`) swaps the blocks.
//   * `jmp *0xe8(%eax)` is a TAIL jump: the screen call takes no arguments
//     and there is nothing to clean, which is the only case VC6 turns into
//     a jump.  The other arm keeps its `call` because it has one argument.
//   * g_pRender is re-LOADED in each arm (`a1` in one, `8b 15` in the
//     other) rather than hoisted -- the two arms are two independent
//     statements, not one call through a cached pointer.
//   * `mov 0xc(%eax),%ecx` is read from the parameter, so the second arm's
//     argument is the surface's own m_pRenderData and not the surface.
void r_UnlockSurface(CSurfaceView3* pSurface)              // 0x0043afc0
{
	if (pSurface == &g_ScreenSurface)
		g_pRender->UnlockScreen();
	else
		g_pRender->UnlockSurface(pSurface->m_pRenderData);
}


// ======================================================================= //
//
//  C.  0x0045d990 -- the 8-bit palette source's pixel read
//
//  BELONGS IN: the surface format-conversion TU at 0x0045d5xx..0x0045dbxx.
//
//  WHAT PINS THE CLASS.  Two call sites, 0x0045d82e and 0x0045d8f1, and
//  both pass `lea <stack slot>,%ecx` as `this` -- a stack-local object one
//  pointer wide.  0x0045d8af writes the palette into that same slot
//  (`mov %eax,0x34(%esp)`) before the loop starts.  And the proof that the
//  body below is right is that the SAME function is expanded INLINE fifty
//  bytes away, at 0x0045d918..0x0045d955, reading the palette straight out
//  of `0x34(%esp)` instead of out of `(%ecx)` and producing the identical
//  five instructions.  Its sibling converter, 0x0045d9c0, is the shift/mask
//  path for non-palettised sources and is a different (much bigger) class.
//
// ======================================================================= //

// Four bytes.  The field NAMES are ours; the field POSITIONS are the bytes':
// the result is built from +1, +2 and +3 of the entry and +0 is never read.
struct SPaletteEntry3
{
	uint8 m_Unknown0;                                      // 0
	uint8 m_r;                                             // 1
	uint8 m_g;                                             // 2
	uint8 m_b;                                             // 3
};

class CPaletteSource3
{
public:
	uint32 ReadPixel(uint8* pSrc);                         // 0x0045d990

public:
	SPaletteEntry3* m_pPalette;                            // 0x00
};

// WHAT PINS IT
//   * `ret $0x4` with %ecx read but never written -> __thiscall member, one
//     stack argument.  A __cdecl free function would be a bare `ret`.
//   * `mov (%ecx),%ecx` -> ONE member, and it is a pointer (loaded, not
//     `lea`'d).
//   * `xor %eax,%eax; mov (%edx),%al` is the zero-extension of a `uint8`
//     load; the index is *pSrc.
//   * `lea (%ecx,%eax,4),%eax` -> a FOUR-byte stride, so the palette is an
//     array of four-byte entries, not a `uint8*` walked by hand.
//   * the packing is the giveaway.  `mov +1,%dh` and `mov +2,%dl` put two
//     bytes into the two halves of ONE register, `shl $0x8,%edx` moves the
//     pair up, and `or %ecx,%edx` drops the third byte in.  That is VC6's
//     reassociation of `(a << 16) | (b << 8) | c` on three bytes, and the
//     LOAD ORDER (+1, then +2, then +3) is the source's term order.
uint32 CPaletteSource3::ReadPixel(uint8* pSrc)
{
	SPaletteEntry3* pEntry = &m_pPalette[*pSrc];

	return (pEntry->m_r << 16) | (pEntry->m_g << 8) | pEntry->m_b;
}


// ======================================================================= //
//
//  D.  0x00460d80 -- a per-instance reset: one float, one dword, two links
//
//  BELONGS IN: the TU that owns 0x00460d00..0x00460e00.  Sole caller is
//  0x00413ec0, inside the 495-byte __thiscall at 0x00413d30, which passes
//  its own `this` and `this->+0x18`.
//
// ======================================================================= //

class SUnknown460D80Owner
{
public:
	uint8  m_pad_00[0x125c];
	uint32 m_Unknown125c;                                  // 0x125c
};

class SUnknown460D80
{
public:
	uint8                 m_pad_00[0x28];
	float                 m_fUnknown28;                    // 0x28
	uint32                m_Unknown2c;                     // 0x2c
	uint8                 m_pad_30[0x44 - 0x30];
	LTLink                m_Link44;                        // 0x44
	LTLink                m_Link50;                        // 0x50
	uint8                 m_pad_5c[0x78 - 0x5c];
	SUnknown460D80Owner*  m_pOwner78;                      // 0x78
};

// WHAT PINS IT
//   * bare `ret`, no cleanup, `this` never in %ecx -> __cdecl, two
//     arguments, void.
//   * `flds 0x8(%esp); fstps 0x28(%eax)` -> parameter two is a `float` and
//     +0x28 is a `float`.  An integer parameter copied to an integer field
//     would be two `mov`s -- which is exactly what the NEXT statement is.
//   * `mov 0x125c(%ecx),%edx; mov %edx,0x2c(%eax)` is an INTEGER copy, and
//     the function proves in its own first two instructions that VC6 does
//     NOT use integer moves for float-to-float here.  So +0x2c and
//     owner+0x125c are dword-typed, not `float`.  (COIN-FLIP, see the foot
//     of this file: CClientMgr+0x125c IS a float -- CLTClient::GetTime at
//     0x00407b40 is `mov 0x4b17e0,%eax; flds 0x125c(%eax)` -- so either
//     +0x78 is NOT the client manager, or VC6 copies a float member to a
//     float member with `mov` and only uses the FPU when a *parameter* is
//     the source.  The uint32 spelling is the one that cannot miss.)
//   * `mov %ecx,0x4(%ecx); mov %ecx,(%ecx)` twice, on two addresses TWELVE
//     bytes apart, +4 stored BEFORE +0.  ltlink.h's
//     `CheapLTLink::TieOff() { m_pPrev = m_pNext = (LTLink*)this; }`
//     assigns m_pNext (+4) first and m_pPrev (+0) second -- that exact
//     order -- and the twelve-byte spacing makes them LTLink, not
//     CheapLTLink (eight).  So this is `dl_TieOff` twice, inline from the
//     SDK header, not two hand-written pairs.
//   * `lea 0x44(%eax),%ecx` for the first and `add $0x50,%eax` for the
//     second: two separate lvalues, first +0x44 then +0x50.
void sub_00460d80(SUnknown460D80* p, float fValue)
{
	p->m_fUnknown28 = fValue;
	p->m_Unknown2c  = p->m_pOwner78->m_Unknown125c;

	dl_TieOff(&p->m_Link44);
	dl_TieOff(&p->m_Link50);
}


// ======================================================================= //
//
//  E.  0x00463cf0 -- a two-argument forwarder used as a CALLBACK
//
//  NOT reached by any `call`: the only reference in the image is
//  `movl $0x463cf0,0x2c(%esp)` at 0x0046393f, which fills the first slot of
//  a stack-built callback struct whose second slot gets 0x00463d10 and
//  whose fourth gets `<obj>+0x6e0`.  So the row's "gap" tag is right about
//  the enumeration and wrong about the extent: the function ends at
//  0x00463d0e (0x1e bytes of code, 0x20 with the pad), not at the 41 the
//  enumeration gives -- 0x00463d10 is the NEXT function, a nine-byte
//  `mov 0x8(%esp),%ecx; mov (%ecx),%eax; jmp *0x4(%eax)` thunk.
//
// ======================================================================= //

class SUnknown463CF0Target
{
public:
	uint8 m_pad_000[0xa2c];
	uint8 m_Unknown0a2c;                                   // 0xa2c
};

// 0x0042fcd0, __cdecl: `return h ? h->m_Unknown74 : NULL;` -- one of a run
// of five identical guarded accessors at 0x0042fcd0/0x0042fce0/0x0042fcf0.
extern SUnknown463CF0Target* sub_0042fcd0(HOBJECT hObj);

// 0x0046b7a0, __cdecl, two arguments; it is itself a default-argument
// wrapper -- `sub_0046b7c0(a, b, 0, 0)` and nothing else: the file
// manager's fs_OpenFile (server/filemgr.cpp), so +0xa2c is a CFileMgr.
class CFileMgr;
extern ILTStream* fs_OpenFile(CFileMgr* pFileMgr, char* pName);

// WHAT PINS IT
//   * bare `ret` after ONE `add $0xc,%esp` -> __cdecl, and the single
//     cleanup covers BOTH calls (one dword for the first, two for the
//     second).  VC6 only merges the two cleanups when the calls are
//     adjacent statements, which is what a nested call expression is.
//   * the second argument is re-read from `0xc(%esp)` AFTER the first call
//     has pushed -- i.e. after the push it is the original `0x8(%esp)`, the
//     second parameter -- so the outer call's operands are evaluated in the
//     order (inner result, then parameter two)... and pushed the other way,
//     which is right-to-left for __cdecl.
//   * `add $0xa2c,%eax` with NO null test between it and the call -> the
//     source takes the member's address off the accessor's result
//     unguarded; a guarded version would branch.
void sub_00463cf0(HOBJECT hObj, void* pArg)
{
	fs_OpenFile((CFileMgr*)&sub_0042fcd0(hObj)->m_Unknown0a2c, (char*)pArg);
}


// ======================================================================= //
//
//  E2.  0x00463d20 -- `mov $0x1,%eax; ret`, 6 bytes
//
//  Two slots in the image point here and nothing calls it: the "Portal" and
//  "Mirror" rows of clientdsi.cpp's built-in surface-effect table (0x004a9e38,
//  their UpdateEffect) and the world tree's box query, which installs it as
//  the default test when the caller left one out (worldtree.cpp, 0x0048d8b6).
//  Both hand it two arguments and it looks at neither.  One function, not a
//  fold: the image has seven separate `mov $1,%eax; ret` bodies, so there is
//  no identical-code folding to have merged it.  C linkage, as clientdsi.cpp
//  declares it (name ours, by address).
//
// ======================================================================= //

extern "C" int sub_463d20(void* p1, void* p2)
{
	return 1;
}


// ======================================================================= //
//
//  F.  0x00466870 -- the "Portal" console command
//
//  WHAT NAMES IT: the dword at 0x004ad0ac IS this address, and the dword
//  twelve bytes into the same table entry, 0x004ad0a8, is 0x004a9e90 ->
//  "Portal".  The table is a run of twelve-byte records {pad, name, fn}:
//  the next two are {0, "ShowGameVars", 0x004668f0} and
//  {0, 0x004ad144, 0x004668a0}.
//
// ======================================================================= //

// 0x004b48c8 is the client's g_pServerMgr (server/smgr.cpp).
class CServerMgr;
extern CServerMgr* g_pServerMgr;                           // 0x004b48c8

// 0x00476790, __cdecl (owner, pName, nValue) -- it offsets the owner by
// 0x1a4 and hands the name to a lookup at 0x00422120.
extern void sub_00476790(void* pOwner, char* pName, int nValue);

// WHAT PINS IT
//   * bare `ret`, `add $0x10,%esp` -> __cdecl.  Two parameters read
//     (`0x4(%esp)` and `0xc(%esp)` after one push) -- the console command
//     signature `void (int argc, char **argv)`.
//   * `cmpl $0x2,0x4(%esp); jl 0x466899` is a SIGNED compare, so argc is
//     `int`, and 0x466899 is the function's only `ret`.  Note where the
//     `push %esi`/`pop %esi` pair sits: INSIDE the guarded region
//     (0x466877 / 0x466898), shrink-wrapped, and the `jl` lands on the
//     `ret` past the `pop`.
//   * argv[1] goes through 0x004937a7 -- the one-argument forwarder to
//     0x0049371c (atol) that ltnew2.cpp already identified as atoi -- and
//     its answer is pushed as the LAST argument of the three-argument call.
//   * the global at 0x004b48c8 is LOADED (`8b 15`), so it is a pointer
//     variable, and it is pushed FIRST of the three, i.e. it is parameter
//     one.
//   * one `add $0x10,%esp` cleans four dwords: atoi's one plus the outer
//     call's three, merged, so nothing separates the two statements.
void PortalCommand(int argc, char** argv)                  // 0x00466870
{
	if (argc < 2)
		return;

	sub_00476790(g_pServerMgr, argv[0], atoi(argv[1]));
}


// ======================================================================= //
//
//  G.  0x00476370 -- the varargs "set the load error" helper
//
//  WHAT PINS THE ROLE.  Twenty call sites, and two of them name it: at
//  0x004029f5 it is `sub(this->+0x2c, 0x35, g_LTEmptyString)` immediately
//  before the site's own error report, and at 0x0040299a it is
//  `sub(%esi, 0x34, 0x4a74bc, "BaseClass")` immediately before an error
//  report that pushes "LT_MISSINGCLASS" and "LoadServerBinaries" -- and
//  0x34 IS LT_MISSINGCLASS.  So the second parameter is a message code and
//  the rest are that message's format arguments.
//
//  The worker, 0x0042d770, is the resource-DLL message formatter: it
//  null-terminates parameter one immediately, bails to "<missing resource
//  DLL>" when the resource module (0x004b3204) is unset, and otherwise
//  masks parameter three to sixteen bits and searches an eight-byte-stride
//  table at 0x004a9b88 for it.
//
// ======================================================================= //

class SLoadErrorSink
{
public:
	uint8 m_pad_00[0x64];
	int   m_nMessageCode;                                  // 0x64
	char  m_MessageBuf[300];                               // 0x68
};

// 0x0042d770, __cdecl (pBuf, nBufSize, nMessageCode, va_list): the
// client's dsi_SetupMessage (clientdsi.cpp).
extern int dsi_SetupMessage(char* pBuf, int nBufSize, int nMessageCode,
                            va_list marker);

// WHAT PINS IT
//   * bare `ret` after `add $0x10,%esp` -> __cdecl, void.
//   * `lea 0x10(%esp),%eax` -- with only ONE push made and no frame
//     reserved, that address is `<parameter 2's slot> + 4`.  Taking the
//     address of the slot past the last named parameter and passing it on
//     is `va_start(marker, nMessageCode)` and nothing else; there is no
//     local for it to be.
//   * the four pushes are, right-to-left, (this+0x68, 0x12c, code, marker)
//     -- so the buffer is at +0x68 and it is 0x12c = 300 bytes.
//   * `mov 0x1c(%esp),%ecx` AFTER the call re-reads parameter 2 (the four
//     pushes put its slot at +0x1c) and stores it at +0x64.  It is read a
//     SECOND time rather than kept, and the store lands AFTER the call, so
//     the assignment is the second statement, not the first.
void sub_00476370(SLoadErrorSink* pSink, int nMessageCode, ...)
{
	va_list marker;

	va_start(marker, nMessageCode);
	dsi_SetupMessage(pSink->m_MessageBuf, 300, nMessageCode, marker);

	pSink->m_nMessageCode = nMessageCode;
}


// ======================================================================= //
//
//  H.  0x0048a590 and 0x0048a5c0 -- THE TWIN PAIR
//
//  Forty-one bytes each and byte-for-byte identical but for one
//  displacement: `call *0x20(%edx)` against `call *0x24(%edx)`.  Neither is
//  in a vtable; each is the FIRST dword of a two-dword static descriptor
//  (0x004af4d8 = {0x0048a590, 0x004ab398} and 0x004af4e0 =
//  {0x0048a5c0, 0x004ab398}), and the constructor at 0x0048a5f0 plants
//  those two descriptor addresses into the +0x04 of two sixteen-byte
//  records at +0x00 and +0x10 of the object it builds, with the owner in
//  each record's +0x08.  Two hooks, same owner, adjacent interface slots.
//
//  The interface they call is the video instance: 0x20 and 0x24 are slots 8
//  and 9 of the ten-slot tables at 0x004a1454 (Bink) and 0x004a38a4
//  (Smack), and BOTH tables hold the same two shared bodies there,
//  0x0048b1b0 and 0x0048b230.
//
//  BELONGS IN: the RAD middleware glue TU (lttexmod.cpp already fences off
//  0x0048a000..0x0048c1a0 as exactly that).  Nothing here is shared with
//  server.dll.
//
// ======================================================================= //

class CVideoInstView3
{
public:
	virtual void _v00();
	virtual void _v04();
	virtual void _v08();
	virtual void _v0c();
	virtual void _v10();
	virtual void _v14();
	virtual void _v18();
	virtual void _v1c();
	virtual void _v20(CVideoInstView3* pOther);            // 0x20
	virtual void _v24(CVideoInstView3* pOther);            // 0x24
};

class SUnknown48A590Arg
{
public:
	uint8            m_pad_00[0x04];
	CVideoInstView3* m_pInst;                              // 0x04
};

// WHAT PINS THEM (both)
//   * bare `ret` with four stack slots read (0x4, 0x8, 0xc, 0x10) and
//     nothing cleaned -> __cdecl, four arguments.  Being a stored callback
//     is why the signature is fixed even though two of the four are barely
//     used.
//   * the TWO member loads happen BEFORE the `test`, unconditionally.  VC6
//     does not speculate loads above a branch, so both dereferences are in
//     the source ahead of the `if`.  The load ORDER is parameter 2's member
//     first, then parameter 1's, which is the declaration order below.
//   * `test %edx,%edx; jne <the shared xor/ret>` -> the guarded body is the
//     ZERO case: `if (!nUnknown) { ... }`.
//   * inside, `movl $0x0,(%edx)` writes through parameter 4 and the `push
//     %eax` before it carries parameter 1's member as the callee's ONE
//     argument -- so the store to *pOut and the call are two separate
//     statements, in that order.  The call's answer is discarded.
//   * `xor %eax,%eax` is the ONLY exit value and it is shared by both
//     paths: `return LT_OK;` as the last statement.
LTRESULT sub_0048a590(SUnknown48A590Arg* pSrc, SUnknown48A590Arg* pDest,
                      uint32 nUnknown, uint32* pOut)
{
	CVideoInstView3* pDestInst = pDest->m_pInst;
	CVideoInstView3* pSrcInst  = pSrc->m_pInst;

	if (!nUnknown)
	{
		*pOut = 0;
		pDestInst->_v20(pSrcInst);
	}

	return LT_OK;
}

// 0x0048a5c0 -- the same 41 bytes with `call *0x24(%edx)` for
// `call *0x20(%edx)`.  Everything above applies unchanged.
LTRESULT sub_0048a5c0(SUnknown48A590Arg* pSrc, SUnknown48A590Arg* pDest,
                      uint32 nUnknown, uint32* pOut)
{
	CVideoInstView3* pDestInst = pDest->m_pInst;
	CVideoInstView3* pSrcInst  = pSrc->m_pInst;

	if (!nUnknown)
	{
		*pOut = 0;
		pDestInst->_v24(pSrcInst);
	}

	return LT_OK;
}


// ======================================================================= //
//
//  I.  THREE CONSTRUCTOR / DESTRUCTOR BODIES
//
// ======================================================================= //

// ----------------------------------------------------------------------- //
//  I1.  0x0048ac40 -- the destructor behind vtable 0x004a38a4
//
//  WHAT PINS THE ROLE: 0x0048ac20 is the scalar deleting destructor --
//  `mov %ecx,%esi; call 0x48ac40; testb $0x1,0x8(%esp); jne <free>; mov
//  %esi,%eax; ret $4` -- and it is slot 0 of the vtable at 0x004a38a4.  So
//  0x0048ac40 is the destructor proper.
//
//  WHAT PINS THE HIERARCHY: the matching constructor, 0x0048aba0, opens by
//  writing 0x004a147c to (%this) and only LATER (0x0048abf1) overwrites it
//  with 0x004a38a4.  Two vptr constants, base first then derived, both at
//  offset 0 and with no call between them -> single inheritance with the
//  base constructor INLINED.  The destructor is the mirror: 0x004a38a4 in,
//  0x004a147c out.  And 0x004a147c really is a ten-slot table --
//  {0x00401c00, 0x00491b40, then eight copies of _purecall 0x004928e7},
//  running 0x004a147c..0x004a14a0, immediately before the float constant at
//  0x004a14a4 -- which is the same width as the derived table
//  0x004a38a4..0x004a38c8.
// ----------------------------------------------------------------------- //

class CUnknown4A147C
{
public:
	// Inline and EMPTY: its whole expansion in the derived destructor is
	// the single `movl $0x4a147c,(%esi)` at the very end.  Anything in its
	// body would appear AFTER that store, and nothing does.
	virtual ~CUnknown4A147C() {}
	virtual void _v04();
	virtual void _v08() = 0;
	virtual void _v0c() = 0;
	virtual void _v10() = 0;
	virtual void _v14() = 0;
	virtual void _v18() = 0;
	virtual void _v1c() = 0;
	virtual void _v20() = 0;
	virtual void _v24() = 0;

public:
	uint8  m_pad_04[0x10 - 0x04];
};

class CUnknownPair48AC
{
public:
	~CUnknownPair48AC() { m_p4 = NULL; m_p0 = NULL; }

	void*  m_p0;                                           // 0x10
	void*  m_p4;                                           // 0x14
};

// The member at +0x68.  0x0045a350 is its constructor (`push $0x0; call
// 0x45a370; return this`) and 0x0045a370 is `{ m_p00 = 0; m_p04 = arg; }`;
// 0x0045a360 is a bare `jmp 0x0045a390` -- a tail-called one-line method,
// not a destructor (a destructor would be emitted where the member's
// destruction belongs, which is AFTER the body, and this call is BEFORE
// the two stores).
class CUnknown45A350
{
public:
	void Release();                                        // 0x0045a360
};

class CClass4A38A4 : public CUnknown4A147C
{
public:
	virtual ~CClass4A38A4();                               // 0x0048ac40
	void Term();                                           // 0x0048ac70

public:
	CUnknownPair48AC m_Pair10;                             // 0x10
	uint8           m_pad_18[0x68 - 0x18];
	CUnknown45A350  m_Unknown68;                           // 0x68
};

// WHAT PINS IT
//   * bare `ret`, `this` taken out of %ecx -> __thiscall, no arguments.
//   * `movl $0x4a38a4,(%esi)` is the FIRST instruction after the prologue,
//     which is where VC6 puts a destructor's own vptr store and nowhere
//     else.
//   * `call 0x48ac70` with %ecx untouched -> a non-virtual member call on
//     `this`.  VC6 does not devirtualise a call through `this`, so a
//     virtual one would be `mov (%ecx),%eax; call *0xNN(%eax)`.
//   * `lea 0x68(%esi),%ecx; call 0x45a360` -> a member sub-object at +0x68,
//     called BEFORE the two stores.  Were it the member's destructor it
//     would have to come after them (body, then members, then base).
//   * the two stores are +0x14 THEN +0x10, descending, from one `xor
//     %eax,%eax`, and they come AFTER both calls but BEFORE the base vptr
//     store.  That is destruction order: body, then MEMBERS, then base.  So
//     +0x10/+0x14 are a derived-class member with an inline destructor, and
//     the base is 0x10 bytes.  Written as two body statements on base
//     fields, VC6 hoists the base vptr store above them (size-exact, two
//     blocks); as the member, identical.
//   * the trailing `movl $0x4a147c,(%esi)` with nothing after it is the
//     inlined empty base destructor.  A base destructor that was NOT inline
//     would be a `call`, and there is none.
CClass4A38A4::~CClass4A38A4()
{
	Term();
	m_Unknown68.Release();
}


// ----------------------------------------------------------------------- //
//  I2.  0x00490210 -- the constructor behind vtable 0x004a3bb4
//
//  Sole caller 0x0048f130.  This is the textbook VC6 constructor order --
//  base constructor, then the derived vptr, then the members -- which is
//  what tells I1 apart from it: there the vptr store is FIRST.
// ----------------------------------------------------------------------- //

// Polymorphic: the shipped constructor calls 0x004901b0 and only THEN
// writes 0x004a3bb4 to (%this).  A base that wrote no vptr of its own would
// leave the derived store as the only one, which is what an EMPTY base
// would give -- and then the vptr's offset would depend on the empty-base
// rule rather than on the bytes.  A polymorphic base fixes the vptr at
// offset 0 and hides the base's own store inside 0x004901b0, which is
// exactly what is visible.
class CUnknown4901B0
{
public:
	CUnknown4901B0(void* pArg);                            // 0x004901b0
	virtual void _v00();
};

class CClass4A3BB4 : public CUnknown4901B0
{
public:
	CClass4A3BB4(void* pArg);                              // 0x00490210

public:
	uint8  m_pad[0x10 - 0x04];
	void*  m_pUnknown10;                                   // 0x10
	void*  m_pUnknown14;                                   // 0x14
	uint8  m_pad_18[0x20 - 0x18];
	int32  m_nUnknown20;                                   // 0x20
};

// WHAT PINS IT
//   * `ret $0x4` and `mov %esi,%eax` on the way out -> __thiscall, one
//     argument, returning `this`: a constructor.
//   * `push %eax; call 0x4901b0` with %ecx still holding `this` -> the base
//     constructor, taking the same one argument, called FIRST.  The derived
//     vptr store follows it, which is the order VC6 uses and the reverse of
//     a destructor's.
//   * the zero stores are +0x10 then +0x14, ASCENDING, from one `xor
//     %eax,%eax` -- declaration order, which is how VC6 emits zero-inits.
//   * `movl $0xffffffff,0x20(%esi)` is the seven-byte `c7` form, not `orl
//     $-1`, so it is a plain assignment of a full dword.
CClass4A3BB4::CClass4A3BB4(void* pArg)
	: CUnknown4901B0(pArg)
{
	m_pUnknown10 = NULL;
	m_pUnknown14 = NULL;
	m_nUnknown20 = -1;
}


// ----------------------------------------------------------------------- //
//  I3.  0x004441e0 -- a destructor with a CRITICAL_SECTION base
//
//  WHAT PINS THE ROLE AND THE OWNER.  0x004445a0 is the OWNING class's
//  destructor -- `movl $0x4a2678,(%esi); lea 0x2c(%esi),%ecx; call
//  0x4441e0; lea 0x4(%esi),%ecx; call 0x4441e0` -- two members of one type
//  at +0x04 and +0x2c, destroyed in reverse declaration order.  Its
//  constructor, 0x00444570, is the mirror: 0x004441b0 on +0x04 then on
//  +0x2c, then the vptr, then two zeroes.  So this type is 0x28 bytes and
//  0x004441e0 is its destructor.
//
//  WHAT PINS THE LAYOUT.  The last thing the body does is `mov %esi,%ecx;
//  call 0x00444170`, and 0x00444170 is `push %ecx; call
//  KERNEL32!DeleteCriticalSection; ret` -- a __thiscall wrapper whose
//  `this` IS the CRITICAL_SECTION.  A base destructor is the last call in a
//  destructor and a base subobject sits at offset 0, and a Win32
//  CRITICAL_SECTION is 0x18 bytes -- which is exactly where the next thing
//  the body touches begins.  0x00444180 is the matching
//  EnterCriticalSection wrapper.
//
//  The 0x18..0x23 that follows is an embedded polymorphic sub-object: the
//  `movl $0x4a2648,0x18(%esi)` is that sub-object's vptr and the two zero
//  stores at +0x20 and +0x1c are its own two fields, +0x08 then +0x04.
//  0x004a2648 is a table of .text pointers (0x00439fc0, 0x00459f30,
//  0x0041d520, ...), so it is a vtable and not data.  0x18 + 0x0c = 0x24,
//  where the HANDLE lives, and 0x24 + 4 = 0x28, the stride the owner
//  proves.  Every byte of the 0x28 is accounted for.
// ----------------------------------------------------------------------- //

class CCritSecBase
{
public:
	// Out of line -- it is CALLED, so it cannot be an in-class definition.
	~CCritSecBase();                                       // 0x00444170
	void Enter();                                          // 0x00444180

public:
	CRITICAL_SECTION m_CS;                                 // 0x00, 0x18 bytes
};

class CUnknown4A2648
{
public:
	// Inline: its whole expansion is the vptr store plus its two fields.
	virtual ~CUnknown4A2648() { m_pUnknown08 = NULL; m_pUnknown04 = NULL; }

public:
	void* m_pUnknown04;                                    // +0x04
	void* m_pUnknown08;                                    // +0x08
};

class CUnknown4441E0 : public CCritSecBase
{
public:
	~CUnknown4441E0();                                     // 0x004441e0
	void Clear();                                          // 0x00444270

public:
	CUnknown4A2648 m_Queue;                                // 0x18
	HANDLE         m_hHandle;                              // 0x24
};

// WHAT PINS IT
//   * bare `ret`, `this` from %ecx -> __thiscall, no arguments.
//   * NO vptr store at the top: this class is not polymorphic itself, which
//     is consistent with the owner destroying it by a direct `call` rather
//     than through a slot.
//   * `call 0x444270` with %ecx = this -> a non-virtual member call; it
//     reads `0x20(%ecx)`, i.e. m_Queue.m_pUnknown08, and walks a list.
//   * `mov 0x24(%esi),%eax; push %eax; call *0x4a1074` -- the import at
//     0x004a1074 is KERNEL32!CloseHandle, so +0x24 is a HANDLE, and the
//     result is discarded.
//   * the vptr store, the two zeroes and the base call come in the order a
//     member destructor and then a base destructor must: `movl
//     $0x4a2648,0x18(%esi)` (the member's own prologue), `+0x20 = 0`,
//     `+0x1c = 0` (its body), then `call 0x444170` (the base).
CUnknown4441E0::~CUnknown4441E0()
{
	Clear();
	CloseHandle(m_hHandle);
}


// ======================================================================= //
//
//  J.  0x00491aa0 -- CNode: the next sibling with the same name
//
//  BELONGS IN: the INI/section-tree TU at 0x00491a50..0x00492850 that
//  ltnew2.cpp already models (CNode / CNodeList).  The class is repeated
//  here under a private name because this is a separate scratch TU; the
//  field offsets are ltnew2's and are unchanged.
//
//  WHAT PINS IT AS "SAME NAME AGAIN".  Its twin sixty bytes later,
//  CNodeList::Find at 0x00491af0, is the identical loop with the search key
//  coming from the ARGUMENT; here the key comes from `this` (`mov
//  0x8(%edi),%eax`, reloaded every iteration) and the walk starts at
//  `this->m_pNext` rather than at a list head.  All eight call sites
//  (0x004417a8 .. 0x00441eeb) use it the same way -- 0x00441990 is `mov
//  %esi,%ecx; call; mov %eax,%esi; test; jne <loop top>` -- an iterator.
//
//  0x00492f30 is stricmp, not strcmp: it opens with the CRT's locale check
//  (`cmpl $0x0,0x4b4f30`, the lock at 0x004b547c) and its mismatch path
//  calls the case-folder at 0x00497c2c before comparing.  This
//  distinction is invisible in a relocation-masked
//  byte compare, so it is stated here explicitly.
//
// ======================================================================= //

class CIniNode
{
public:
	CIniNode* GetNextSameName();                           // 0x00491aa0

public:
	CIniNode* m_pNext;                                     // 0x00
	uint32    m_Unknown04;                                 // 0x04
	char*     m_pName;                                     // 0x08
};

// WHAT PINS IT
//   * bare `ret`, `this` from %ecx into %edi -> __thiscall, no arguments.
//   * `mov (%edi),%esi` before the first test -> the walk starts at
//     `m_pNext`, not at `this`.
//   * the exit test is DUPLICATED -- once before the loop (`test %esi,%esi;
//     je end`) and once at the bottom (`test %esi,%esi; jne top`) -- which
//     is a rotated `while`, and the single `mov %esi,%eax` at the shared
//     exit is `return pCur;` for BOTH the found and the exhausted case.
//     Two separate returns (`return pCur;` inside and `return NULL;`
//     after) would give two exit blocks.
//   * `mov 0x8(%edi),%eax` sits INSIDE the loop, reloaded every iteration:
//     the source names `m_pName` at the call, it is not hoisted to a local.
//   * `push %eax` (this->m_pName) then `push %ecx` (pCur->m_pName) ->
//     right-to-left, so the call is stricmp(pCur->m_pName, m_pName).
//     CNodeList::Find at 0x00491af0 pushes in the same relation (the node's
//     name first), which is the cross-check.
//   * `add $0x8,%esp` -> __cdecl, so it is the CRT's stricmp and not a
//     member.
CIniNode* CIniNode::GetNextSameName()
{
	CIniNode* pCur;

	pCur = m_pNext;
	while (pCur)
	{
		if (stricmp(pCur->m_pName, m_pName) == 0)
			break;

		pCur = pCur->m_pNext;
	}

	return pCur;
}


// ======================================================================= //
//
//  K.  0x004556e0 -- CNetMgr::FreeSessionList   ** SHARED CODE **
//
//  This one is already DECLARED, with this exact address, at
//  src/engine/server/servermgr.h:1204, and its only caller is
//  ci_FreeSessionList (0x004082d1, src/engine/lithtech/clientinterface.cpp).
//  Its home is src/engine/server/netmgr.cpp, which lithtech.exe and
//  server.dll BOTH compile -- moving it there changes both images and both
//  have to be re-measured.  It is written here so the body is
//  not lost, not because it belongs here.
//
//  THE OFFSET IS NOT A GUESS.  `mov 0x1098(%ecx),%esi` is NetSession's
//  m_pNext, and ltbasedefs.h's NetSession puts it at exactly 0x1098:
//  vptr(4) + m_dwFlags(4) + two LTGUIDs(32) + m_dwMaxPlayers(4) +
//  m_dwCurPlayers(4) + m_Ping(4) + m_HostIP[32] + m_HostPort(4) = 0x58,
//  then m_sName[MAX_NETSESSIONNAME_LEN=4096] + m_sPassword[64] = 0x1040.
//  0x58 + 0x1040 = 0x1098.
//
// ======================================================================= //

class CNetMgr
{
public:
	LTRESULT FreeSessionList(NetSession* pListHead);       // 0x004556e0
};

// WHAT PINS IT
//   * `ret $0x4` -> member, one stack argument.  `this` is never read,
//     which is why the body below touches no member.
//   * `mov 0x4(%esp),%ecx; test %ecx,%ecx; je <second xor/ret>` -- the exit
//     block is DUPLICATED (0x004556ff and 0x00455705, both `xor %eax,%eax;
//     ret $4`).  Two copies of the same tiny epilogue are a rotated
//     `while`, exactly as in J above.
//   * `test %ecx,%ecx; je` again INSIDE the loop, immediately before `mov
//     (%ecx),%eax; push $0x1; call *(%eax)`, is not a source test: it is
//     the null check VC6 wraps around `delete p`, and the `$0x1` is the
//     scalar deleting destructor's "and free the memory" flag.  Writing
//     `if (pCur) delete pCur;` would produce a second, redundant test.
//   * the next pointer is read BEFORE the delete (`mov 0x1098(%ecx),%esi`
//     at the top of the loop body), so the source keeps it in a local.
//   * `xor %eax,%eax` on both exits -> `return LT_OK;` as the last
//     statement, shared by the empty-list path.
LTRESULT CNetMgr::FreeSessionList(NetSession* pListHead)
{
	NetSession* pCur;
	NetSession* pNext;

	pCur = pListHead;
	while (pCur)
	{
		pNext = pCur->m_pNext;
		delete pCur;
		pCur = pNext;
	}

	return LT_OK;
}


// ======================================================================= //
//
//  L.  0x00428370 -- a 65-byte member-to-member copy
//
//  Five call sites, 0x00427697 .. 0x00427a65, all of the form
//  `lea <dest>,%ecx; push <source>; call` -- a copy from one object of this
//  type to another.  The destination at 0x00427697 is a stack object
//  (`lea -0x50(%ebp),%ecx`), so the type is small enough to live on the
//  stack.
//
// ======================================================================= //

class CUnknown428370
{
public:
	void CopyBuffer(CUnknown428370* pOther);               // 0x00428370

public:
	uint32 m_Unknown00;                                    // 0x00
	char   m_Buf[65];                                      // 0x04 .. 0x44
};

// WHAT PINS IT
//   * `ret $0x4` with `this` out of %ecx -> __thiscall, one argument.
//   * the loop is a BYTE loop (`mov (%esi,%edx,1),%cl; mov %cl,(%edx)`),
//     which a compiler-generated `operator=` is not: VC6 copies an
//     aggregate with `rep movsd` or a memcpy call.  This is an explicit
//     loop in the source.
//   * the induction variable is the DESTINATION ADDRESS -- %edx starts at
//     `this+4` and is `inc`'d -- and the source is reached as
//     `(pOther - this) + %edx`, one subtraction hoisted into %esi before
//     the loop.  That is VC6's strength reduction of `m_Buf[i] =
//     pOther->m_Buf[i]`, where both arrays share one index.  A source that
//     walked two pointers by hand comes out differently
//     ("Loops want the array indexed, not walked").
//   * the trip count is read off the bound: %edi holds `-4 - this`, so
//     `lea (%edi,%edx,1),%ecx` after the `inc` is `i + 1`, and
//     `cmp $0x41; jb` keeps going while `i + 1 < 65`.  Indices 0..64
//     inclusive -- 65 bytes, ending at offset 0x44.
//   * the compare is `jb`, unsigned, so the index is unsigned.
void CUnknown428370::CopyBuffer(CUnknown428370* pOther)
{
	uint32 i;

	for (i = 0; i < sizeof(m_Buf); i++)
		m_Buf[i] = pOther->m_Buf[i];
}


// ----------------------------------------------------------------------- //
// 0x004419b0 -- CLTDirectMusicMgr::ReadStyles.
//
// MOVE-ME: its home is src/engine/lithtech/dmextra.cpp, the TU that holds
// the rest of CLTDirectMusicMgr's 0x00440f00..0x004419xx run (ReadDLSBanks
// 0x00441960 just before it, LoadStyleQ440 0x00441610 which it calls).  It
// is declared at clientmusic.h:603 as returning void, but the shipped body
// ends `mov $0x1,%eax; ret $0x4` -- it answers LTTRUE.  Written here on a
// view class rather than in dmextra.cpp.
//
// Read off the bytes: GetKey(LTNULL, "STYLE") (the string at 0x004abe08;
// the NULL section is pushed second, i.e. it is the first argument), then
// for each key the first word's value and the control file go to
// LoadStyle, whose answer is ignored; GetNextKey (0x00491aa0) steps.
// ----------------------------------------------------------------------- //
class CControlFileWordQ12
{
public:
	CControlFileWordQ12* m_pNext;                          // 0x000
	CControlFileWordQ12* m_pPrev;                          // 0x004
	char*                m_sVal;                           // 0x008
	int                  m_nStrLen;                        // 0x00c

	char* GetVal() { return m_sVal; }
};

class CControlFileKeyQ12
{
public:
	CControlFileWordQ12* GetFirstWord();                   // 0x00491a90
	CControlFileKeyQ12*  GetNextKey();                     // 0x00491aa0
};

class CControlFileMgrQ12
{
public:
	CControlFileKeyQ12* GetKey(const char* sSectionName,
	                           const char* sKeyName);      // 0x00491e70
};

class CLTDirectMusicMgrQ12
{
public:
	LTBOOL LoadStyleQ440(const char* sStyleName,
	                     CControlFileMgrQ12* pControlFile); // 0x00441610
	LTBOOL ReadStyles(CControlFileMgrQ12* pControlFile);   // 0x004419b0
};

LTBOOL CLTDirectMusicMgrQ12::ReadStyles(CControlFileMgrQ12* pControlFile)
{
	CControlFileKeyQ12* pKey = pControlFile->GetKey(LTNULL, "STYLE");

	while (pKey)
	{
		CControlFileWordQ12* pWord = pKey->GetFirstWord();

		if (pWord)
			LoadStyleQ440(pWord->GetVal(), pControlFile);

		pKey = pKey->GetNextKey();
	}

	return LTTRUE;
}


// ----------------------------------------------------------------------- //
// 0x00442ec0 -- the inlined constructor of the DirectMusic file manager
// g_DMFileMgr at 0x004b3fe8 (dmloader.cpp:258 declares it extern; nobody
// defines it).  It is the middle third of the dynamic-initialiser group
// 0x00442eb0 `call 0x442ec0; jmp 0x442ef0`, 0x00442ef0 registering
// 0x00442f00 with atexit.
//
// MOVE-ME: the definition belongs at the head of src/engine/lithtech/
// dmloader.cpp -- the group sits immediately before 0x00442f20, the first
// of that TU's bodies -- under dmloader.cpp's own CDMFileMgr name.  Written
// here on a view.
//
// Read off the bytes: seven zero stores, +0x18/+0x1c/+0x20 (the loaded
// list's head, tail, count), +0x24/+0x28/+0x2c (the unloaded list's), and
// +0x34, SKIPPING +0x30 (m_nLimit, which Init 0x0042d520 sets) and the
// critical section at the front.  So the lists have an in-class
// constructor and there is a flag at +0x34 that the manager's constructor
// clears.  The destructor 0x00442f00 is `if (+0x34) Term()` with Term the
// tail call to 0x0042d540 (clientworld.cpp's CLockedOwnerView::Term), so
// +0x34 is the "initialised" flag.
// ----------------------------------------------------------------------- //
class CDMFileRecordQ12;

class CDMFileRecordListQ12
{
public:
	CDMFileRecordListQ12() { m_pHead = LTNULL; m_pTail = LTNULL; m_nCount = 0; }

public:
	CDMFileRecordQ12* m_pHead;                             // 0x000
	CDMFileRecordQ12* m_pTail;                             // 0x004
	uint32            m_nCount;                            // 0x008
};

class CDMFileMgrQ12
{
public:
	CDMFileMgrQ12() { m_bInitialized = LTFALSE; }
	~CDMFileMgrQ12() { if (m_bInitialized) Term(); }

	void Term();                                           // 0x0042d540

public:
	CRITICAL_SECTION     m_CriticalSection;                // 0x000
	CDMFileRecordListQ12 m_Loaded;                         // 0x018
	CDMFileRecordListQ12 m_Unloaded;                       // 0x024
	uint32               m_nLimit;                         // 0x030
	LTBOOL               m_bInitialized;                   // 0x034
};

CDMFileMgrQ12 g_DMFileMgrQ12;                              // 0x004b3fe8


// ======================================================================= //
//
//  THE OTHER NINE ROWS -- no code, and why
//
// ======================================================================= //
//
//  A. ONE IS ALREADY IN THE BUILD, FROM A DIFFERENT SOURCE TREE.
//
//     0x00490ba0 (37) is stdlith's **sb_Init2**, and
//     src/lithshared/stdlith/struct_bank.cpp already has it:
//
//         int sb_Init2(StructBank *pBank, DWORD structSize,
//                      DWORD cacheSize, DWORD nPreallocations)
//         {
//             sb_Init(pBank, structSize, cacheSize);
//             return sb_AllocateNewStructPage(pBank, nPreallocations);
//         }
//
//     build/libs.sh builds struct_bank.cpp into StdLith.lib and the client
//     links it, which is why `grep -rn 0x00490ba0 src/engine/` finds
//     nothing -- the owner is outside src/engine.  The bytes agree term for
//     term: `push arg3; push arg2; push pBank; call 0x00490b50` (which
//     clientworld.cpp already names sb_Init) then `push arg4; push pBank;
//     call 0x00490c20` (which clientsoundmgr.cpp already names
//     sb_AllocateNewStructPage), with the two __cdecl cleanups merged into
//     one `add $0x14,%esp` and the second call's %eax returned untouched.
//     Sixteen of its ~40 call sites are visible in one run at
//     0x0045a48d..0x0045a5cf, each `(bank, size, 0x20, N)`.
//     ACTION: none.  The row should be marked accounted-for against
//     src/lithshared, not re-written.
//
//  B. TWO ARE COMPILER-GENERATED DYNAMIC INITIALISERS, not source
//     functions.  Both are the second half of the standard VC6 initialiser
//     group `call <inlined ctor>; jmp <atexit registration>`.
//
//       0x00442ec0 (38) is reached ONLY from 0x00442eb0, which is
//                  `call 0x442ec0; jmp 0x442ef0`, and 0x00442ef0 is
//                  `push $0x442f00; call 0x0049307e (atexit); pop; ret`.
//                  So 0x00442ec0 is the inlined constructor of a file-scope
//                  object: it zeroes seven dwords at 0x004b4000, 0x004b4004,
//                  ...0x004b4014 and 0x004b401c, SKIPPING 0x004b4018.  The
//                  registered destructor 0x00442f00 reads 0x004b401c and,
//                  when it is non-zero, calls 0x0042d540 with %ecx =
//                  0x004b3fe8 -- so the object actually starts at
//                  0x004b3fe8 and 0x004b4000 is its +0x18.  Reproducing it
//                  means declaring the right global in the right
//                  translation unit, not writing a function.
//                  DONE (later pass): g_DMFileMgrQ12 above defines it, and
//                  its _$E ctor and _$F dtor halves are IDENTICAL at
//                  0x00442ec0 and 0x00442f00.
//
//       0x00461d90 (42) IS an entry in the `.CRT$XC` table -- the dword at
//                  0x004a70c0, inside the 0x004a7000..0x004a70d8 run that
//                  ltnew2.cpp identified.  Its body is the familiar
//                  `jmp <next 16-byte boundary>` + eleven NOPs, and the
//                  target 0x00461da0 is
//                      *(0x4b4130) = 0; *(0x4b4138) = 0x4b4134;
//                      *(0x4b4134) = 0x4b4134;
//                  i.e. `dl_InitList` on a file-scope LTList at 0x004b4130
//                  -- m_nElements at +0x00 and the LTLink head tied off at
//                  +0x04, with m_pNext (+0x08) stored before m_pPrev
//                  (+0x04), which is ltlink.h's TieOff order.  A file-scope
//                  `LTList g_Xxx;` in the owning TU, not a function.
//                  DONE (later pass): it is g_TextureDataList's m_List,
//                  defined in clientrender.cpp; its _$E is IDENTICAL.
//
//  C. TWO ARE MIS-SIZED -- real functions, but not 38/39 bytes.  The size
//     is the distance to the next enumerated address and that address is a
//     branch target INSIDE the body in both cases.
//
//       0x004524e0  38 stated.  0x004524e0..0x004524ff is the
//                   `testl $0x200000,0x88(%esi); je` guard and its
//                   `xor %eax,%eax; pop; add $0x78,%esp; ret` early
//                   return; the `je` target 0x00452506 is what the
//                   enumeration charged a 488-byte "gap" row to.  The real
//                   extent is 0x004524e0..0x004526ee, ~526 bytes, and it
//                   opens `sub $0x78,%esp` -- a 0x78-byte frame no
//                   38-byte function has.  Out of scope for a tiny-function
//                   file; it should be re-enumerated at its true
//                   size.
//
//       0x004359c0  39 stated.  The `jne 0x4359e7` five instructions in is
//                   the next enumerated address, and 222 more bytes are
//                   charged to a "gap" row after it.  The real extent is
//                   0x004359c0..0x00435ac5, ~261 bytes; the body reaches
//                   0x00435ac0 by a `je` with an 0xe6-byte displacement,
//                   which is by itself proof the function is longer than
//                   39 bytes.  Same recommendation.
//
//  D. THREE ARE MID-FUNCTION BLOCKS, not functions.
//
//       0x00473b60  the loop body of the 16-byte-tagged function at
//                   0x00473b50, which really runs to 0x00473b87.  It is
//                   fallen into from `je 0x473b80`'s NOT-taken path at
//                   0x00473b5e and re-entered by `jne 0x473b61` at
//                   0x00473b76.  It opens `push %esi` but reads %eax, %edx
//                   and %ecx that only 0x00473b50 sets (%edx is the `xor
//                   %edx,%edx` at 0x00473b54), and its own `pop %esi` is
//                   matched by that one push.
//
//       0x00476468  the loop body of the function at 0x00476450, whose
//                   stated size of 24 ends exactly here.  Reached by the
//                   fall-through of `je 0x47647e` at 0x00476466 and
//                   re-entered by `jne 0x476469`.  It uses %esi (the
//                   object, loaded at 0x00476455), %eax and %edx (set at
//                   0x0047645b/0x0047645e), and it `pop %edi`s at 0x0047647d
//                   the register it pushed at 0x00476468 -- but leaves
//                   0x00476450's `push %esi` for the real epilogue to pop.
//                   The real extent is 0x00476450..0x0047648f, 63 bytes.
//
//       0x0045002e  not a function at ALL: it is MID-INSTRUCTION.  The
//                   function is 0x00450020, and 0x0045002e falls inside the
//                   `c7 01 00 00 00 00` (`movl $0x0,(%ecx)`) that starts at
//                   0x0045002a.  Disassembling from 0x0045002e gives
//                   `add %al,(%eax)` on two of that instruction's zero
//                   bytes.  0x00450020's real extent is
//                   0x00450020..0x00450081 (98 bytes): a two-argument
//                   `ret $0x8` that clears the out-parameter, checks
//                   `cmpb $0x1,0xbc(%eax)`, hands back `eax + 0x1c0` and
//                   otherwise reports LT_INVALIDPARAMS (0x3c) through the
//                   0x0042d760 / 0x0042e6f0 pair with the literal
//                   "LT_INVALIDPARAMS".  The three neighbouring "ptr" rows
//                   (0x00450056 size 2, 0x00450058 size 42, 0x00450082
//                   size 0) are fragments of the same body and of the next
//                   one.
//
// ======================================================================= //
//
//  COIN-FLIPS NOT SETTLED
//
// ======================================================================= //
//
//   1. 0x00460d80, the type of +0x2c and of owner+0x125c.  Shipped:
//         mov 0x125c(%ecx),%edx
//         mov %edx,0x2c(%eax)
//      An INTEGER copy, which is what `uint32 = uint32` gives -- and this
//      file ships that spelling because it cannot miss.  The tension is
//      that CClientMgr+0x125c IS a float (CLTClient::GetTime, 0x00407b40,
//      is `mov 0x4b17e0,%eax; flds 0x125c(%eax); ret`).  Either +0x78 is
//      not the client manager, or VC6 copies float member to float member
//      with `mov` and reserves the FPU for the case where a *parameter* is
//      the source -- which is what the same function's first statement,
//      `flds 0x8(%esp); fstps 0x28(%eax)`, would then be.  The alternative
//      spelling is exactly:
//          float m_fUnknown2c;  float m_fUnknown125c;
//          p->m_fUnknown2c = p->m_pOwner78->m_fUnknown125c;
//      One build settles it (not run), and guessing wrong here costs
//      four bytes, not a shape.
//
//   2. SETTLED (identical): the two stores are a member destructor, see
//      the function.  Original note kept for the record:
//      0x0048ac40, the two null stores.  `m_pUnknown14 = NULL;
//      m_pUnknown10 = NULL;` (shipped order, what is written) versus
//      `m_pUnknown10 = m_pUnknown14 = NULL;` -- a chained assignment
//      evaluates right to left and emits +0x14 then +0x10 as well.  Both
//      give the one `xor %eax,%eax` and the same two stores.
//      DISPROVED: the ascending pair `m_pUnknown10 = NULL; m_pUnknown14 =
//      NULL;`, which comes out ascending.
//
//   3. 0x0043afc0, the arm spelling.  `if (p == &g_ScreenSurface)
//      UnlockScreen(); else UnlockSurface(...);` (written) versus
//      `if (p == &g_ScreenSurface) { UnlockScreen(); return; }
//      UnlockSurface(...);` -- the true arm is a TAIL JUMP either way, so
//      the `return` is invisible.  DISPROVED: the inverted test, which
//      swaps the two blocks.
//
//   4. 0x00466870, the guard spelling.  `if (argc < 2) return;` (written)
//      versus `if (argc >= 2) { ... }` -- the "error" path has no body at
//      all, so both produce `jl` onto the function's single `ret`.  What is
//      NOT a coin-flip is the shrink-wrap: the `push %esi` is inside the
//      guarded region in both, because %esi is only live there.
//
//   5. 0x0048a590 / 0x0048a5c0, the two hoisted loads.  They are in the
//      source ahead of the `if` (VC6 does not speculate loads across a
//      branch), but whether the source names two locals -- as written --
//      or writes one expression whose subterms VC6 then commons is not
//      decidable from 41 bytes.  What IS decided: they cannot both live
//      inside the `if`.
//
//   6. 0x0045d990, the packing expression.  `(r << 16) | (g << 8) | b`
//      (written) versus `(((r << 8) | g) << 8) | b`, which is literally
//      what the three instructions do.  VC6 reassociates the first into the
//      second -- the `dh`/`dl` pair is its standard byte-packing idiom --
//      but if it does not, the second spelling is the exact one.  The LOAD
//      ORDER (+1, +2, +3) is fixed either way and rules out any term order
//      but r, g, b.
//
//   7. 0x00476370's parameter type.  `int nMessageCode` (written): it is
//      pushed as a full dword and only 0x0042d770 masks it to sixteen bits
//      internally, so `uint32` or `uint16` here would each add a mask this
//      function does not have.  Stated because the SDK spells the same
//      thing `int messageCode` in ILTCSBase::WriteToMessageFormattedHString
//      and that is the precedent followed.
//
// ======================================================================= //
