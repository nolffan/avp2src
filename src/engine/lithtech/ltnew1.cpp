// *********************************************************************** //
//
//  MODULE:   LTNEW1 (.CPP)   — staging translation unit (name ours)
//
//  PURPOSE:  Twenty-five enumerated rows out of lithtech.exe, read off
//            the retail lithtech.exe.  They are NOT one translation unit;
//            they are eight, and the per-block banners say which source file
//            each one really belongs to.  Everything here compiles at /O2,
//            which is what src/engine/lithtech gets by default.
//
//  WHAT THE ENUMERATION FOUND, AND WHAT THE BYTES SAY
//  --------------------------------------------------
//  Four vtables did most of the naming.  All four were anchored twice, on
//  independent evidence, before a single body was written:
//
//   * 0x004a14e0 — CLTClient's ILTClient vtable, 110 slots.  Slots 1..30 and
//     56..70 are ILTCSBase's own non-pure bodies (0x0041ec90.. and
//     0x0041f640..), which is 45 slots of free anchor: iltcsbase.h declares
//     exactly 30 message accessors then exactly 15 rotation helpers, in that
//     order, at those positions.  So slot = iltcsbase index for 0..70, the
//     ILTClient block starts at slot 71 with the destructor, and
//     slot = iltclient index + 71 from there.  Confirmed by three named
//     rows: StartVideo (idx 13) at slot 84, ProcessAttachments (idx 22) at
//     93, GetSpriteControl (idx 27) at 98 — gaps 9 and 5 in both lists.
//
//     THE TABLE ENDS AT SLOT 109, not at LT2's idx 44.  Slot 110 begins a
//     different vtable (see below), so the shipped ILTClient stops at
//     SetModelLooping (idx 38) and does NOT have LT2's GetModelPlaying,
//     SetModelPlaying, StartQuery, UpdateQuery, GetQueryResults, EndQuery.
//     That is the ILTClient era mismatch clientinterface.h says does not
//     bite the FLAT table; it does bite the virtual one, at the tail.
//
//   * 0x004a16b0 — eight slots, the client's ILTCursor front end.  Every one
//     of the eight is a forward to `m_pClientMgr->m_pCursor->slot[i]`, and
//     the eight targets are exactly the eight bodies cltcursor.cpp already
//     has under 0x004a24c8, in the same order.  iltcursor.h declares exactly
//     eight pure virtuals.  Argument counts agree slot for slot
//     (1,1,1,2,1,1,1,0) and so does the one constant: slot 2 forwards to
//     0x0043ecc0, `mov $0x56,%eax; ret $4` — LT_YES, which is literally what
//     the header's comment on IsCursorModeAvailable promises.
//
//   * 0x004a1440 (16 slots) and 0x004a3890 (15 slots) — BinkVideoInst and
//     SmackVideoInst.  The two tables are the SAME interface: slots 2,3,4,6,
//     13,14 hold the identical shared bodies in both, and the other slots
//     hold one 0x0040xxxx body and one 0x0048bxxx body each.  The names are
//     attested: "BinkVideoInst::Init" (0x004a7310), "BinkVideoInst::
//     InitScreen", "BinkVideoInst::InitTexture" in the first range, and
//     lttexmod.cpp already reads "SmackVideoInst::Init" out of the second.
//
//   * 0x004a2ef4 — CLTServer's ILTServer vtable (slots 1..30 and 56..70 are
//     the same ILTCSBase bodies again).  Two rows land in it; both belong
//     under src/engine/server and are flagged where they are written.
//
//  ONE ROW IS NOT A FUNCTION and one is ALREADY WRITTEN; both are recorded
//  in the block at the foot of this file rather than reconstructed.
//
//  A CONSTANT WORTH STATING ONCE: ltcodes.h has LT_ERROR == 1 and
//  LT_FINISHED == 2.  Every `mov $0x2,%eax` in the video block below is
//  LT_FINISHED, not LT_ERROR — which is what turns three anonymous stubs
//  into ILTVideoMgr's documented "LT_OK = still playing, LT_FINISHED = done
//  playing" contract.
//
// *********************************************************************** //

#include "ltbasedefs.h"
#include "ltcodes.h"
#include "iltcursor.h"
#include "iltvideomgr.h"
#include "de_error.h"
#include "packet.h"
#include <stdio.h>
#include <stdarg.h>


// ======================================================================= //
//  A.  CLTClient — five slots of the ILTClient vtable at 0x004a14e0
//
//  BELONGS IN: a client TU that does not exist yet.  The five are spread
//  over 0x00406ee0..0x00407a6b, which is one contiguous block of ILTClient
//  slot bodies (slots 71..109 all live in 0x004058xx..0x00407c2x), so they
//  are one translation unit with the other 35 — not with anything the repo
//  already compiles.  src/engine/lithtech/clientinterface.h's `CLTClient` is
//  the FLAT function-pointer table, a different view of the same object
//  (vptr at 0x00, the 208 flat slots at 0x020..0x35c, and StartVideo below
//  proves the object keeps going: m_pVideoMgr at 0x360, m_hVideo at 0x3c4).
//  That header is not this reconstruction's to change, so the class is
//  redeclared here on a view — house style, see clientsoundinst.cpp.
//
//  NONE OF THE FIVE IS DECLARED VIRTUAL HERE ON PURPOSE.  A virtual and a
//  non-virtual member compile to the same bytes; only the call sites and the
//  vtable differ, and we emit neither.  Declaring them virtual would make
//  VC6 emit a vtable for the view and demand the other 105 bodies.
// ======================================================================= //

class ILTSpriteControl;

// One client-side object, as far as these three reach it.  Every offset is
// clientmgr.h's already except m_pAttachments, which is exactly the four
// pad bytes clientmgr.h leaves at 0x098 (`uint8 m_pad_98[0x9c - 0x98]`).
class CClientObjectView
{
public:
	uint8  m_pad_00[0x98];
	// 0x098 — the head of this object's attachment chain.  Read only by
	// ProcessAttachments, walked through m_pNext below.
	class CClientAttachment* m_pAttachments;
	uint8  m_pad_9c[0xbc - 0x9c];
	char   m_ObjectType;                     // 0x0bc  OT_*, clientmgr.h's
	uint8  m_pad_bd[0x1c0 - 0xbd];
	// 0x1c0 — for an OT_SPRITE object the sprite's own control block starts
	// here; GetSpriteControl hands its address out as the interface pointer
	// and does nothing else, so the interface IS the sub-object.
	uint8  m_SpriteControl[4];
};

// One attachment record.  Two fields are reached from this file: the next
// link, and (inside 0x00462080) a uint16 at +0x1e.  Nothing else is known
// and nothing else is named.
class CClientAttachment
{
public:
	uint8              m_pad_00[0x24];
	CClientAttachment* m_pNext;              // 0x024
};

// 0x00462080 — clientrender.cpp already documents this body ("create a
// client-side object mirroring a server one: calls a vtable +0x18 on
// g_pClientMgr+0x4c0's +0x08, then 0x00420520 / 0x004207a0 / 0x00420880").
// It is cdecl and it takes TWO arguments of which it reads only the second:
// `sub $0x1c,%esp; push %esi; push %edi;` then `mov 0x2c(%esp),%esi`, which
// is arg 2, and `add $0x1c,%esp; ret`.  The dead first argument is why
// ProcessAttachments below passes hObj as well as the attachment.
void sub_462080(HOBJECT hObj, CClientAttachment* pAttachment);

// 0x00484980 / 0x004848e0 — src/engine/server/stringbank.cpp's, already
// reconstructed there.  clientstreams.cpp names 0x004848e0 sb_CreateString
// in as many words.
HSTRING sb_CopyString(HSTRING hString);
HSTRING sb_CreateString(char* pString);


class CLTClientView
{
public:
	uint8 m_pad_00[0x360];
	// 0x360 — the concrete ILTVideoMgr.  StartVideo calls its slot 0 with
	// three arguments and its slot 4 with one, which is StartOnScreenVideo
	// and StopVideo in iltvideomgr.h's declaration order exactly.
	ILTVideoMgr* m_pVideoMgr;
	uint8 m_pad_364[0x3c4 - 0x364];
	// 0x3c4 — the one video the client has playing.  Its address is taken
	// (the reference argument) and it is also stored through, which is why
	// the `lea` is hoisted above the null test in the shipped code.
	HVIDEO m_hVideo;

	LTRESULT ProcessAttachments(HOBJECT hObj);                        // 0x00406ee0
	LTRESULT GetSpriteControl(HLOCALOBJ hObj, ILTSpriteControl*& pControl); // 0x00406f50
	LTRESULT StartVideo(char* pFilename, uint32 flags);               // 0x00407410
	HSTRING  CopyString(HSTRING hString);                             // 0x00407a30
	HSTRING  CreateString(char* pString);                             // 0x00407a50
};


// 0x00406ee0, 102 bytes — NOT the 67 the enumeration reported.  The row was
// cut at 0x00406f23, which is the target of the `jne` at 0x00406ee7, i.e.
// the function's own success arm; the real extent is 0x00406ee0..0x00406f46
// and the padding after it is twelve `nop`s.
//
// WHAT PINS IT.
//  * the name is the check literal at 0x004a7b3c, and the check is the
//    ordinary RETURN_ERROR shape: `push $0x3c` (LT_INVALIDPARAMS) into
//    0x0042d760, `cmp $0x2,0x004b0ab0` (level 2), then the printer with
//    0x004a7160 ("60") and 0x004a7164 (" ").
//  * the error arm falls through and the body is jumped over, which is the
//    `if (!ok) { err }` guard form, not a trailing else.
//  * `mov 0x98(%edi),%esi` before the loop and `mov 0x24(%esi),%esi` inside
//    it: the list head is a field of the object and the link is at +0x24.
//    The head is read once, the test is at the top and the back edge jumps
//    to the body — a rotated `while (p)`, not a do/while.
//  * the next link is read AFTER the call, so the loop cannot be written
//    with the advance hoisted above it.
//  * `push %esi; push %edi; call 0x00462080; add $0x8,%esp` — cdecl, two
//    arguments, hObj first.
LTRESULT CLTClientView::ProcessAttachments(HOBJECT hObj)
{
	if (!hObj)
		RETURN_ERROR(2, "CLTClient::ProcessAttachments", LT_INVALIDPARAMS);

	CClientAttachment* pAttachment = ((CClientObjectView*)hObj)->m_pAttachments;

	while (pAttachment)
	{
		sub_462080(hObj, pAttachment);

		pAttachment = pAttachment->m_pNext;
	}

	return LT_OK;
}


// 0x00406f50, 91 bytes.
//
// WHAT PINS IT.
//  * `cmpb $0x3,0xbc(%eax)` — the object-type byte clientmgr.h already puts
//    at 0x0bc, against OT_SPRITE (ltbasedefs.h: 3).
//  * ONE `if` with two terms, not two ifs: both tests jump to the SAME
//    error block and that block sits AFTER the body, which is
//    the shared-error-after-the-body shape.  Two separate guards would put
//    the report between them.
//  * `add $0x1c0,%eax; mov %eax,(%ecx)` — the out-parameter is written with
//    the ADDRESS of a sub-object, so the second argument is a reference to
//    a pointer and the interface is embedded in the object at 0x1c0.
//  * `xor %eax,%eax; ret $0x8` on the success arm: LT_OK returned inside
//    the if, so the report is the function's last statement.
LTRESULT CLTClientView::GetSpriteControl(HLOCALOBJ hObj, ILTSpriteControl*& pControl)
{
	if (hObj && ((CClientObjectView*)hObj)->m_ObjectType == OT_SPRITE)
	{
		pControl = (ILTSpriteControl*)((CClientObjectView*)hObj)->m_SpriteControl;

		return LT_OK;
	}

	RETURN_ERROR(2, "CLTClient::GetSpriteControl", LT_INVALIDPARAMS);
}


// 0x00407410, 129 bytes.
//
// WHAT PINS IT.
//  * slot 84 of 0x004a14e0 and the literal at 0x004a7c14; iltclient.h's
//    StartVideo(char*, uint32) is the only two-argument video call and the
//    function is `ret $0x8`.
//  * `mov 0x3c4(%edi),%eax` … `lea 0x3c4(%edi),%esi` — the field is read
//    for the test and its ADDRESS is materialised once, before the test,
//    and used both to clear it and as the last call's third argument.  That
//    is one member passed by reference, not two.
//  * `call *0x10(%edx)` with one argument and `call *(%eax)` with three:
//    iltvideomgr.h declares StartOnScreenVideo(char*,uint32,HVIDEO&) at
//    slot 0 and StopVideo(HVIDEO) at slot 4 (+0x10).  Nothing else in that
//    header has those two argument counts at those two slots.
//  * the guard is the error-falls-through form again, and the final call is
//    a `call` (not a tail `jmp`) because three registers are still to pop.
LTRESULT CLTClientView::StartVideo(char* pFilename, uint32 flags)
{
	if (!pFilename)
		RETURN_ERROR(2, "CLTClient::StartVideo", LT_INVALIDPARAMS);

	if (m_hVideo)
	{
		m_pVideoMgr->StopVideo(m_hVideo);

		m_hVideo = LTNULL;
	}

	return m_pVideoMgr->StartOnScreenVideo(pFilename, flags, m_hVideo);
}


// 0x00407a30 and 0x00407a50, 25 bytes each — slots 45 and 46 of 0x004a14e0,
// which iltcsbase.h declares as CopyString(HSTRING) then CreateString(char*)
// in exactly that order.  The call targets confirm it independently:
// 0x00484980 is stringbank.cpp's sb_CopyString and 0x004848e0 is the
// sb_CreateString clientstreams.cpp already names by address.
//
// WHAT PINS THE SHAPE.  The SUCCESS path falls through and the null case is
// jumped forward to (`test %eax,%eax; je` over the call, `xor %eax,%eax;
// ret $4` at the end).  Written the other way round — `if (!h) return
// LTNULL;` first — VC6 puts the `xor`/`ret` inline and jumps the call, which
// is the opposite layout.  Arm order tells you which branch the
// source guarded.
//
// Note that BOTH forwarded callees ALSO null-check their argument
// (stringbank.cpp: `if (!hString) return LTNULL;`).  The test here is
// therefore redundant and is in the source anyway; that is what shipped.
HSTRING CLTClientView::CopyString(HSTRING hString)
{
	if (hString)
		return sb_CopyString(hString);

	return LTNULL;
}


HSTRING CLTClientView::CreateString(char* pString)
{
	if (pString)
		return sb_CreateString(pString);

	return LTNULL;
}


// ======================================================================= //
//  B.  The client's ILTCursor front end — vtable 0x004a16b0, slot 7
//
//  BELONGS IN: the same client TU as block A (the eight bodies run
//  0x00405dc0..0x00405f27, immediately below the ILTClient slot bodies).
//
//  WHAT THE VTABLE IS.  Eight consecutive 0x30-byte bodies, each of them
//  `if (!this->+0x04) return LT_NOTINITIALIZED;` followed by a forward to
//  `(*(this->+0x04 + 0x1614))->slot[i]` for i = 0..7 in order.  The object
//  at CClientMgr+0x1614 is cltcursor.cpp's CLTCursor: that file already
//  reads the one writer, 0x0040fbdb in CClientMgr::Init, and files the
//  result at CClientMgr+0x1614, and it already maps CLTCursor's vtable
//  0x004a24c8 slot for slot onto iltcursor.h.  Slot 7 there is
//  RefreshCursor, and 0x0043ed80 — the ShowCursor(TRUE/FALSE) counter loop
//  this forwards to — is the body cltcursor.cpp writes for it.
//
//  The class name is ours.  Nothing in the image names it; it is the eight
//  ILTCursor slots CLTClient hands out, holding a CClientMgr back pointer.
// ======================================================================= //

// CClientMgr, as far as this reaches it.
class CClientMgrCursorView
{
public:
	uint8      m_pad_00[0x1614];
	ILTCursor* m_pCursor;                    // 0x1614
};

class CClientCursorProxy
{
public:
	uint8                 m_pad_00[0x04];    // 0x000  the vptr
	CClientMgrCursorView* m_pClientMgr;      // 0x004

	LTRESULT SetCursorMode(CursorMode cMode);                         // 0x00405dc0
	LTRESULT RefreshCursor();                                         // 0x00405f10
};

// 0x00405dc0, 33 bytes — slot 0.  lt2new2.cpp writes slots 1, 2, 4, 5, 6
// and leaves this one; same shape, the argument pushed and a `call *(%eax)`
// through slot 0 of the cursor, `ret $4`.
LTRESULT CClientCursorProxy::SetCursorMode(CursorMode cMode)
{
	if (!m_pClientMgr)
		return LT_NOTINITIALIZED;

	return m_pClientMgr->m_pCursor->SetCursorMode(cMode);
}

// 0x00405f10, 24 bytes.
//
// WHAT PINS IT.
//  * `mov $0x49,%eax` is 73 = LT_NOTINITIALIZED, and the guard is the
//    error-inline form (`jne` over it), so the source is an early return.
//  * `jmp *0x1c(%eax)` — a TAIL jump, so the value is returned directly and
//    the frame is empty; and +0x1c is slot 7, RefreshCursor, the only
//    zero-argument slot in iltcursor.h other than none.  The function is
//    `ret` with no immediate, which agrees.
LTRESULT CClientCursorProxy::RefreshCursor()
{
	if (!m_pClientMgr)
		return LT_NOTINITIALIZED;

	return m_pClientMgr->m_pCursor->RefreshCursor();
}


// ======================================================================= //
//  C.  BinkVideoInst and SmackVideoInst — vtables 0x004a1440 / 0x004a3890
//
//  BELONGS IN: two client TUs neither of which the repo has.  Bink's bodies
//  run 0x00401b50..0x0040262x; Smack's are inside the 0x0048a000-0x0048c1a0
//  range lttexmod.cpp already fences off as RAD middleware glue.
//
//  WHY THEY ARE ONE INTERFACE.  Line the two tables up and six slots hold
//  the IDENTICAL body in both — slot 2 (0x0048a6b0), slot 3 (0x0048a710),
//  slot 4 (0x0048a740), slot 6 (0x0048b390), slot 13 (0x0048b1b0), slot 14
//  (0x0048b230) — and every other slot holds one 0x0040xxxx address in
//  Bink's and one 0x0048bxxx address in Smack's, at the same index.  Bink's
//  table has one extra slot on the end (0x00401c00).  So there is an
//  abstract VideoInst with fifteen virtuals whose non-pure bodies were
//  compiled into the Smack TU, and BinkVideoInst adds a sixteenth.
//
//  THE THREE SLOTS BELOW.  Slot 11 is the one that is not guesswork: it
//  returns 2 when its helper says yes and 0 when it says no, and ltcodes.h
//  has LT_FINISHED == 2 and LT_OK == 0 — iltvideomgr.h's GetVideoStatus
//  contract word for word ("LT_OK = still playing, LT_FINISHED = done
//  playing").  Both helpers it calls compare a frame counter against a
//  total minus one (`dec %ecx; cmp; sete`), which is the same answer from
//  the other side.  Slots 9 and 10 fail into the same LT_FINISHED, and slot
//  10's target opens by asking the render hook for "BACKBUFFER" — so it is
//  the per-frame blit.  The names of 9 and 10 are OURS.
//
//  THE LAYOUTS ARE NOT THE SAME CLASS'S.  Slot 9 tests +0x60 in Bink and
//  +0xa0 in Smack, and the "is it done" helpers read +0x08/+0x0c in Bink
//  and +0x0c/+0x374 in Smack.  Only +0x18 and +0x20 line up.  That is why
//  the two classes are declared separately below rather than sharing a
//  base with data in it.
// ======================================================================= //

class BinkVideoInst
{
public:
	uint8    m_pad_00[0x18];
	// 0x018 — the open BINK.  Everything in the class is guarded on it.
	void*    m_pBink;
	uint8    m_pad_1c[0x20 - 0x1c];
	// 0x020 — the DirectDraw surface the frame is blitted to; released
	// through its own COM vtable slot 2 by the teardown at 0x00402150.
	void*    m_pSurface;
	uint8    m_pad_24[0x60 - 0x24];
	// 0x060 — 1 for an on-screen video.  0x00402120 (slot 7) reads it as a
	// dword and so does this.
	uint32   m_nMode;

	// 0x00402540 (slot 9's body) and 0x004022e0 (slot 10's).  Both are
	// thiscall with no arguments and both return LTRESULT; names ours.
	LTRESULT DrawScreen();
	LTRESULT DrawFrame();
	// 0x004022c0 — `return m_pBink->frame == m_pBink->frames - 1;`
	// (name ours).  Thiscall, no arguments, and it null-checks m_pBink
	// itself, which is why the caller's own check is redundant and present
	// anyway.
	int      IsDone();

	LTRESULT UpdateScreen();                                          // 0x004021a0
	LTRESULT Update();                                                // 0x004021c0
	LTRESULT GetStatus();                                             // 0x004021e0
};

class SmackVideoInst
{
public:
	uint8    m_pad_00[0x18];
	class SmackLtn1* m_pSmack;               // 0x018
	class SmackVideoMgrLtn1* m_pMgr;         // 0x01c
	void*    m_pSurface;                     // 0x020
	uint8    m_pad_24[0x64 - 0x24];
	class TextureDataLtn1* m_pTextureData;   // 0x064
	uint8    m_Texture[0x9c - 0x68];         // 0x068 SharedTexture

	LTRESULT DrawFrame();                    // 0x0048b440
	int      IsDone();                       // 0x0048b410
	LTRESULT UpdateTextureVideo();           // 0x0048b820

	LTRESULT Update();                                                // 0x0048b290
	LTRESULT GetStatus();                                             // 0x0048b2b0
};


// 0x004021a0 / 0x004021c0 / 0x004021e0 — Bink's UpdateScreen, Update and
// GetStatus — now live in binkvideo.cpp as BinkVideoInstK's own (vtable
// 0x004a1440's slots name them there).  The notes that pinned them moved
// with them; Smack's two twins stay below.


// 0x004021c0 (Bink) and 0x0048b290 (Smack), 25 bytes each.  The two are
// byte-identical apart from the tail jump's target, which is what says they
// are the same source compiled into two classes.
LTRESULT SmackVideoInst::Update()
{
	if (m_pSmack && m_pSurface)
		return DrawFrame();

	return LT_FINISHED;
}


// 0x004021e0 (Bink) and 0x0048b2b0 (Smack), 26 bytes each; again identical
// apart from the call target.
//
// WHAT PINS THE TERNARY.  `call; neg %eax; sbb %eax,%eax; and $0x2,%eax` is
// VC6's branchless select of 2-or-0 from a truth value.  A branching source
// (`if (IsDone()) return LT_FINISHED; return LT_OK;`) emits `test`/`je` and
// two return sites instead; the conditional expression is the only spelling
// that folds to the three-instruction sequence.
//
// Note the arm order: the null case is the one JUMPED to, so the guard is
// positive and the failure return is the function's last statement — the
// same shape as slots 9 and 10 above, which is what one expects of three
// consecutive slots written together.
LTRESULT SmackVideoInst::GetStatus()
{
	if (m_pSmack)
		return IsDone() ? LT_FINISHED : LT_OK;

	return LT_FINISHED;
}


// 0x0048b410, 35 bytes — the Smack "is it done" helper GetStatus calls.
// Frame count at +0x0c of the Smack record, current frame at +0x374,
// loaded in that order; no record answers 1 (done), from the fall-through-
// jumped arm, so the guard is positive and `return 1;` comes last.
class SmackLtn1
{
public:
	uint8  m_pad_00[0x08];
	uint32 m_nHeight;                        // 0x008
	uint32 m_nFrames;                        // 0x00c
	uint8  m_pad_10[0x374 - 0x10];
	uint32 m_nFrameNum;                      // 0x374
};

int SmackVideoInst::IsDone()
{
	if (m_pSmack)
		return m_pSmack->m_nFrameNum == m_pSmack->m_nFrames - 1;

	return 1;
}


// 0x0048b820, 188 bytes -- SmackVideoInst::UpdateTextureVideo (the name
// is the error literal at 0x004af778), the tail-jump target of slot 4's
// 0x0048b270.  Bink's twin is 0x00402540 ("BinkVideoInst::
// UpdateTextureVideo", unwritten).  HOME: smackvideo.cpp; written here
// beside IsDone, which it calls twice.
//
//  * the two guards `je` ONE report block after both success returns:
//    a positive `&&` guard around the body, RETURN_ERROR(LT_FINISHED) the
//    last statement ("2" at 0x004a7480 -- the double-expanded code).
//  * manager +0x20/+0x24/+0x28/+0x2c are _SmackWait@4 / _SmackToBuffer@28
//    / _SmackDoFrame@4 / _SmackNextFrame@4 (fntdraw.cpp's Init stores them
//    there); none is followed by a stack adjustment.
//  * _SmackToBuffer(smack, 0, 0, pitch, smack->Height, buffer, 0): the
//    texture data's +0xd8 is the pitch and +0xd4 the buffer.
//  * 0x004628e0 is clientrender.cpp's rif_BindTexture(&m_Texture, TRUE).
class SmackVideoMgrLtn1
{
public:
	uint8  m_pad_00[0x20];
	uint32 (__stdcall *m_pSmackWait)(SmackLtn1* pSmack);            // 0x20
	void   (__stdcall *m_pSmackToBuffer)(SmackLtn1* pSmack, uint32 left,
	                                     uint32 top, uint32 pitch,
	                                     uint32 height, void* pBuf,
	                                     uint32 flags);             // 0x24
	uint32 (__stdcall *m_pSmackDoFrame)(SmackLtn1* pSmack);         // 0x28
	void   (__stdcall *m_pSmackNextFrame)(SmackLtn1* pSmack);       // 0x2c
};

class TextureDataLtn1
{
public:
	uint8  m_pad_00[0xd4];
	void*  m_pData;                          // 0x0d4
	uint32 m_nPitch;                         // 0x0d8
};

struct SharedTextureLtn1;
void rif_BindTextureLtn1(SharedTextureLtn1* pTexture, LTBOOL bChanged);  // 0x004628e0

LTRESULT SmackVideoInst::UpdateTextureVideo()
{
	if (m_pSmack && m_pTextureData)
	{
		if (IsDone())
			return LT_OK;

		if (!m_pMgr->m_pSmackWait(m_pSmack))
		{
			m_pMgr->m_pSmackToBuffer(m_pSmack, 0, 0, m_pTextureData->m_nPitch,
			                         m_pSmack->m_nHeight,
			                         m_pTextureData->m_pData, 0);
			m_pMgr->m_pSmackDoFrame(m_pSmack);

			rif_BindTextureLtn1((SharedTextureLtn1*)m_Texture, LTTRUE);

			if (!IsDone())
				m_pMgr->m_pSmackNextFrame(m_pSmack);
		}

		return LT_OK;
	}

	RETURN_ERROR(1, "SmackVideoInst::UpdateTextureVideo", LT_FINISHED);
}


// ======================================================================= //
//  D.  Two rows of src/engine/lithtech/clientmusic.cpp
//
//  BELONGS IN: src/engine/lithtech/clientmusic.cpp.  MOVE THEM THERE.
//  build/engine.sh compiles that file alone under NOPOOL_FLAGS
//  (/Ot /Og /Oy /Ob1 /Oi /Gs /Gy) rather than /O2.  The only difference
//  between those two sets is /GF — string POOLING, which is a .data
//  property and masked — so the code bytes are the same either way and
//  these two are safe to measure from here.  They are written on views so
//  that moving them is an edit to clientmusic.h plus a copy, with no
//  duplicate symbol in the meantime.
//
//  WHAT PLACES THEM.  0x00440560 is slot 6 of the CLTDirectMusicMgr vtable
//  at 0x004a2500, and that vtable is pinned by clientmusic.h five times
//  over: slots 3,4,5,8,9,10,11 are TermLevel 0x00440230, Play 0x00440420,
//  Stop 0x00440480, SetVolume 0x004405a0, ChangeIntensity 0x00440620,
//  PlaySecondary 0x00440940, StopSecondary 0x00440ae0 — every one of them
//  an address clientmusic.h already carries, and every one at the index
//  iltdirectmusic.h declares it at.  Slot 6 is Pause and slot 7 (0x00440580,
//  "CLTDirectMusicMgr::UnPause\n") is UnPause.  The trace literal at
//  0x004ab9a8 says "CLTDirectMusicMgr::Pause nStart=%i\n" outright.
//
//  0x00442080 is the sibling of clientmusic.h's CDMPlayingList::FindByName
//  (0x00442030): same class, same walk, different key.
// ======================================================================= //

// 0x0043eef0 — clientmusic.h's; defined in block G below.
void dm_Printf(int nLevel, char* pFormat, ...);

// clientmusic.h's LTDMEnactTypes comes from iltdirectmusic.h; spelled out
// here so this file does not have to pull that header in for one argument.
typedef int LTDMEnactTypesView;

class CLTDirectMusicMgrView
{
public:
	LTRESULT Pause(const LTDMEnactTypesView nStart);                  // 0x00440560
	LTRESULT UnPause();                                               // 0x00440580
};

// 0x00440560, 25 bytes.  A trace and a success — the whole body.
//
// WHAT PINS IT.  `push` argument, `push` format, `push $4`, call, `add
// $0xc,%esp`, `xor %eax,%eax`, `ret $4`.  Three cdecl arguments, level 4
// like every other trace in the class, no m_bInitialized test at all (which
// every REAL entry point in clientmusic.cpp has and this one does not), and
// LT_OK unconditionally.  Pause is a stub in the shipped engine.
LTRESULT CLTDirectMusicMgrView::Pause(const LTDMEnactTypesView nStart)
{
	dm_Printf(4, "CLTDirectMusicMgr::Pause nStart=%i\n", nStart);

	return LT_OK;
}

// 0x00440580, 18 bytes.  Slot 7, Pause's twin: `push` format (0x004ab9cc,
// "CLTDirectMusicMgr::UnPause\n"), `push $4`, call 0x0043eef0, `add $8`,
// `xor %eax,%eax`, plain `ret` -- no arguments, no initialised test, LT_OK.
LTRESULT CLTDirectMusicMgrView::UnPause()
{
	dm_Printf(4, "CLTDirectMusicMgr::UnPause\n");

	return LT_OK;
}


// clientmusic.h's CDMPlayingNode and CDMPlayingList, as far as this needs
// them; the offsets are that header's exactly.
class CDMPlayingNodeView
{
public:
	CDMPlayingNodeView* m_pNext;             // 0x000
	uint8  m_pad_04[0x08 - 0x04];
	void*  m_pSegState;                      // 0x008
};

class CDMPlayingListView
{
public:
	CDMPlayingNodeView* m_pHead;             // 0x000
	CDMPlayingNodeView* m_pTail;             // 0x004

	CDMPlayingNodeView* FindBySegState(void* pSegState);              // 0x00442080
};

// 0x00442080, 26 bytes.  The name is ours; what it does is not.
//
// WHAT PINS IT.  Both call sites are 0x0043f7cf and 0x0043f7f1, on
// `g_pDirectMusicMgr + 0x74` and `+ 0x7c` — the two CDMPlayingLists
// clientmusic.cpp already calls m_SecondaryList and m_MotifList — and each
// hands the answer straight to 0x00442270, which clientmusic.h declares as
// CDMPlayingList::StopNode(pMgr, pNode, nEnact, bOnlyIfStopped).  So the
// return type is CDMPlayingNode* and the key is what a playing node is
// identified by from outside: the segment state at +0x08.
//
// The walk itself: `mov (%ecx),%eax` is m_pHead, `mov (%eax),%eax` is
// m_pNext at +0x00, and the key is hoisted into %ecx in the preheader
// BEFORE the loop — the rotated `while (p)` form.  The two returns share
// one `ret`, with `xor %eax,%eax` falling into it, which is one `return
// LTNULL;` after the loop and one `return pNode;` inside it.
CDMPlayingNodeView* CDMPlayingListView::FindBySegState(void* pSegState)
{
	CDMPlayingNodeView* pNode = m_pHead;

	while (pNode)
	{
		if (pSegState == pNode->m_pSegState)   // operand order survives
			return pNode;

		pNode = pNode->m_pNext;
	}

	return LTNULL;
}


// ======================================================================= //
//  E.  The un-named C++ subsystem at 0x0048c540 and up, plus two callers
//
//  lttexmod.cpp already fences this range off ("a different, un-named C++
//  subsystem built on 16-byte link records, std::map lookups (0x004a03d0 /
//  0x004a0560) and SEH frames") and leaves it unidentified.  Nothing below
//  guesses either: the classes are views
//  named by address and the only claims made are ones the bytes force.
//
//  BELONGS IN: an unidentified client TU.  Not clientmusic.cpp — the list
//  helpers at 0x0048de30..0x0048dee0 that clientmusic.h borrows are shared
//  code, not that file's.
// ======================================================================= //

// 0x004a03d0 and 0x004a0560 sit ABOVE the app region's 0x004928bf, in the
// template/library block.  Both are thiscall with no arguments and both
// return a record pointer or null.  They are declared, never defined: the
// call is a rel32 and therefore masked, so all that has to be right is the
// `lea` that forms their `this` and the fact that they are members.
class CRecCursor
{
public:
	class CLinkRecView* Advance();           // 0x004a03d0
	class CLinkRecView* First();             // 0x004a0560
};

class CLinkRecView
{
public:
	uint8 m_pad_00[0x14];
	void* m_pValue;                          // 0x014
};

// The four wrappers are __thiscall — every call site loads %ecx first
// (`mov %esi,%ecx` at 0x00421431, `mov 0x120(%ebx),%ecx` at 0x004214ab and
// 0x004214c7) — and not one of the four reads `this`.  So they are members
// of whatever object the caller keeps at +0x120, and that object is not
// otherwise reached from here.  Name ours, by the address of the first.
class CSub48e680Owner
{
public:
	class CNode20View* GetValue20(class CNode20View* pNode);          // 0x0048e680
	class CNode04View* GetValue04(class CNode04View* pNode);          // 0x0048e6c0
	class CNode24View* GetValue24(class CNode24View* pNode);          // 0x0048e6e0
	class CNode1cView* GetValue1c(class CNode1cView* pNode);          // 0x0048e700
};

class CNode20View { public: uint8 m_pad_00[0x20]; CRecCursor m_Cursor20; };
class CNode04View { public: uint8 m_pad_00[0x04]; CRecCursor m_Cursor04; };
class CNode24View { public: uint8 m_pad_00[0x24]; CRecCursor m_Cursor24; };
class CNode1cView { public: uint8 m_pad_00[0x1c]; CRecCursor m_Cursor1c; };

// 0x0048e680 / 0x0048e6c0 / 0x0048e6e0 / 0x0048e700 — 25 bytes each, and
// all four the same source over a different member and a different one of
// the two cursor calls.
//
// WHAT PINS THE SHAPE.  `test %eax,%eax; jne` over a bare `ret $4` — the
// null arm returns %eax WITHOUT zeroing it, because the compare has just
// proved it zero.  That is `return LTNULL;` written as a guard (the error
// arm inline, the body jumped to), which is the vanishing
// null test.  The payload is `mov 0x14(%eax),%eax`, one dword at +0x14 of
// whatever the cursor handed back.
//
// WHY THE FOUR ARE ORDERED AS THEY ARE.  0x004214c1..0x004214f5 is a nested
// walk that reads them off: `m_124 = GetValue04(m_124)` advances the outer
// cursor, `m_128 = GetValue24(m_124)` opens the inner one on the new outer
// record, and 0x004214ab's `m_128 = GetValue1c(m_128)` advances the inner.
// So 0x0048e6c0 and 0x0048e700 are "next" (both through 0x004a03d0) and
// 0x0048e6e0 is "first" (through 0x004a0560).  0x0048e680 is a "next" on a
// fifth member and has one call site, 0x00421602.
CNode20View* CSub48e680Owner::GetValue20(CNode20View* pNode)
{
	CLinkRecView* pRec = pNode->m_Cursor20.Advance();

	if (!pRec)
		return LTNULL;

	return (CNode20View*)pRec->m_pValue;
}


CNode04View* CSub48e680Owner::GetValue04(CNode04View* pNode)
{
	CLinkRecView* pRec = pNode->m_Cursor04.Advance();

	if (!pRec)
		return LTNULL;

	return (CNode04View*)pRec->m_pValue;
}


CNode24View* CSub48e680Owner::GetValue24(CNode24View* pNode)
{
	CLinkRecView* pRec = pNode->m_Cursor24.First();

	if (!pRec)
		return LTNULL;

	return (CNode24View*)pRec->m_pValue;
}


CNode1cView* CSub48e680Owner::GetValue1c(CNode1cView* pNode)
{
	CLinkRecView* pRec = pNode->m_Cursor1c.Advance();

	if (!pRec)
		return LTNULL;

	return (CNode1cView*)pRec->m_pValue;
}


// 0x0048df10, 26 bytes — REMOVED.  It is src/lithshared/rezmgr's
// rezdir.cpp CRezItm::CRezItm (??0CRezItmX380@@AAE@XZ), which RezMgr.lib
// matches byte-identical at this address; the view that stood here never did.


// 0x0045a430, 26 bytes, and 0x00477810, 24 bytes — one pair, so they are
// written together even though they are in different translation units
// (0x0045a430 sits with the constructor at 0x0045a470 that installs vtable
// 0x004a2c00, which lttexmod.cpp already places in this subsystem).
//
// 0x0045a430 IS THE SAME LIST WALK AS CDMPlayingList::FindBySegState ABOVE
// OVER A DIFFERENT LAYOUT: head at +0x00 of the list, key at +0x00 of the
// node, next at +0x08 of the node.  Same rotated `while`, same hoisted key,
// same shared `ret` with `xor %eax,%eax` falling into it.
class CKeyedNodeView
{
public:
	void*           m_pKey;                  // 0x000
	uint8           m_pad_04[0x08 - 0x04];
	CKeyedNodeView* m_pNext;                 // 0x008
};

class CKeyedListView
{
public:
	CKeyedNodeView* m_pHead;                 // 0x000

	CKeyedNodeView* FindByKey(void* pKey);                            // 0x0045a430
};

CKeyedNodeView* CKeyedListView::FindByKey(void* pKey)
{
	CKeyedNodeView* pNode = m_pHead;

	while (pNode)
	{
		if (pNode->m_pKey == pKey)
			return pNode;

		pNode = pNode->m_pNext;
	}

	return LTNULL;
}


// The object 0x00477810 asks about: it owns one of those lists at +0x14.
class CKeyedListOwnerView
{
public:
	uint8          m_pad_00[0x14];
	CKeyedListView m_List14;                 // 0x014
};

// 0x004a8380 — a file-scope object whose first dword is the code address
// 0x00411d10.  Only its ADDRESS is ever used, as the key above.  It is a
// dependency record like serverengine.cpp's two, {callback, parent}: the
// .data words are {0x00411d10, 0x004ab398}, and 0x004ab398 is
// serverengine.cpp's g_DepUnlinkRecord ({dl_UnlinkAndFree 0x0043c4b0, 0}).
// Defined in block E below, next to its callback (name ours).
class CDepRecord;
extern CDepRecord g_Unknown4a8380;

// 0x00477810, 24 bytes.  cdecl (plain `ret`), TWO arguments, and it reads
// only the second: `mov 0x8(%esp),%eax` with nothing pushed yet is arg 2,
// and its one call site, 0x00477877, pushes two (`push %ebx; push %ebp;
// call; add $0x8,%esp`).  The first parameter is dead.  That is not an
// inference from the call site alone — the callee's own frame proves which
// slot it reads.
//
// WHAT PINS THE RETURN.  `neg; sbb; neg` normalises the pointer to 0 or 1,
// and the caller tests it with `test %eax,%eax; jne`.  So the result is a
// boolean, not the node: `!= NULL` in the source, not a cast.
//
// The name is ours and says only what the body does.
LTBOOL sub_477810(void* pUnused, CKeyedListOwnerView* pOwner)
{
	return pOwner->m_List14.FindByKey(&g_Unknown4a8380) != LTNULL;
}


// 0x00445960, 26 bytes as enumerated; the real extent is
// 0x00445960..0x004459a6, 70 bytes (the row was cut at the `je` target
// 0x0044597a).  The body is src/engine/server/commonmodel.cpp's
// CUnknownD974::sub_100166de, identical on both images; nothing is written
// for it here.


// ======================================================================= //
//  F.  TWO ROWS THAT ARE NOT THIS FILE'S — MOVE THEM (the first is moved)
//
//  Both are slots of an ILTCSBase-derived vtable in lithtech.exe whose
//  bodies live in the SERVER engine's own address blocks, which means their
//  source belongs under src/engine/server/ and a copy compiled only into
//  this client TU leaves server.dll unserved.  They are written here so the
//  bodies exist and are measured; once relocated, BOTH images need
//  re-measuring.
// ======================================================================= //

// 0x0046eb30 — CLTServer::SetModelFilenames: MOVED. It is now written once,
// in src/engine/server/servermessage.cpp beside SetObjectFilenames, against
// the SDK's ILTServer so the forward goes through slot 118 (0x1d8); that copy
// is byte-identical on both images (server.dll 0x1002ef12, lithtech.exe
// 0x0046eb30). The view that stood here called slot 0 and matched neither.


// 0x0047c1d0, 21 bytes.
//
// BELONGS IN: src/engine/server/.  clientsoundinst.cpp already fences
// 0x0047bc50-0x0047c62x off as "the SERVER engine, compiled into
// lithtech.exe as well", and this is inside it.
//
// WHAT PINS IT.  The callee, 0x0043bcc0, is src/engine/server/packet.cpp's
// sm_BreakAttachments, and the identification is structural, not by
// address: it reads `pObject->+0x188` for the SObjData, `+0x04` for the
// list head, `+0x08` for each node's m_pData, `(%esi)` for the link type,
// compares it against the third argument and then against 2 — which is
// packet.cpp's sm_BreakAttachments statement for statement.  The four
// pushes here are (this->+0x04, hObj, 1, 0), so the manager comes out of
// the object at +0x04 and the type is 1.  `add $0x10,%esp` then `ret $4`
// with no value set: the function returns void.
//
// The vtable is 0x004a31ec (slot 126); that table's slots 0 and 72..76 are
// __purecall, so it is an abstract class's, and its slot 125 (0x0047c1a0)
// sets the object's change flag 0x40 and CREATES a type-1 link through
// 0x0043bdb0 — packet.cpp's sm_CreateInterObjectLink.  A create/remove
// pair on the same link type.  Which interface it is is NOT settled here,
// so the class and the method are named by address.
class CSub47c1d0View
{
public:
	uint8 m_pad_00[0x04];                    // 0x000  the vptr
	class CLinkMgrView* m_pMgr;              // 0x004

	void RemoveAttachments(HOBJECT hObj);                             // 0x0047c1d0
};

// src/engine/server/packet.cpp's, 0x0043bcc0 in this image.
void sm_BreakAttachments(class CLinkMgrView* pMgr, HOBJECT pObject,
                         int nType, int bNotify);

void CSub47c1d0View::RemoveAttachments(HOBJECT hObj)
{
	sm_BreakAttachments(m_pMgr, hObj, 1, 0);
}


// ======================================================================= //
//  G.  Four more unwritten rows
// ======================================================================= //

// ---- 0x0043eef0 — dm_Printf ------------------------------------------- //
//
// BELONGS IN: src/engine/lithtech/clientmusic.cpp (clientmusic.h declares
// it, and its twins dm_Error 0x0043edb0 / dm_Warning 0x0043ee50 sit just
// below it).  Declared above in block D for Pause.
//
// Read off the bytes: `cmp %ecx(nLevel),%eax(g_nDMDebugLevel); jl` skips
// everything; `mov 0x4b0ab4,%cl` into buf[0] and a 0x7c-dword + word + byte
// `rep stos` is VC6's `char buf[500] = ""`; _vsnprintf (0x0049318e) gets
// 0x1f3 = sizeof - 1 and buf[499] is then zeroed; one trailing '\n' is
// stripped with two separate strlens; and the line goes to the console at
// colour 0x80ff80 (dm_Warning's is 0xff80).
// 0x004a9fb0 — the "LTDMConsoleOutput" console variable (2 in the image);
// dsi.cpp defines it under the server's name.
extern int g_CV_LTDMConsoleOutput;           // 0x004a9fb0

// 0x0041b880 — the client console's print entry (clientdsi.cpp's name).
void sub_41b880(uint32 nColor, int nUnknown, char* pMsg);

// 0x0043edb0 / 0x0043ee50, 155 bytes each — dm_Error and dm_Warning.
// dm_Printf's body with the level fixed (1 and 2) and their own colours
// (0xff0080 and 0xff80).
void dm_Error(char* pFormat, ...)
{
	if (g_CV_LTDMConsoleOutput >= 1)
	{
		char pMsg[500] = "";
		va_list marker;

		va_start(marker, pFormat);
		_vsnprintf(pMsg, 499, pFormat, marker);
		pMsg[499] = '\0';
		va_end(marker);

		if (pMsg[strlen(pMsg) - 1] == '\n')
			pMsg[strlen(pMsg) - 1] = '\0';

		sub_41b880(0xff0080, 0, pMsg);
	}
}


void dm_Warning(char* pFormat, ...)
{
	if (g_CV_LTDMConsoleOutput >= 2)
	{
		char pMsg[500] = "";
		va_list marker;

		va_start(marker, pFormat);
		_vsnprintf(pMsg, 499, pFormat, marker);
		pMsg[499] = '\0';
		va_end(marker);

		if (pMsg[strlen(pMsg) - 1] == '\n')
			pMsg[strlen(pMsg) - 1] = '\0';

		sub_41b880(0xff80, 0, pMsg);
	}
}


void dm_Printf(int nLevel, char* pFormat, ...)
{
	if (g_CV_LTDMConsoleOutput >= nLevel)
	{
		char pMsg[500] = "";
		va_list marker;

		va_start(marker, pFormat);
		_vsnprintf(pMsg, 499, pFormat, marker);
		pMsg[499] = '\0';
		va_end(marker);

		if (pMsg[strlen(pMsg) - 1] == '\n')
			pMsg[strlen(pMsg) - 1] = '\0';

		sub_41b880(0x80ff80, 0, pMsg);
	}
}


// ---- 0x0043ef90 — dm_NotInitError, 19 bytes ---------------------------- //
// Every CLTDirectMusicMgr entry point's not-initialised refusal
// (clientmusic.cpp); the format is the string at 0x004ab424.
void dm_NotInitError(char* pFnName)
{
	dm_Error("ERROR! LTDirectMusic not Initialized (%s)\n", pFnName);
}


// ---- 0x00442030 — CDMPlayingList::FindByName --------------------------- //
//
// BELONGS IN: src/engine/lithtech/clientmusic.cpp (clientmusic.h declares
// it: "the first node whose segment answers to this name").  Its sibling
// CDMSegmentDefList::FindByName at 0x00441fe0 is the same body over the
// segment list itself (unwritten, not on this list).
//
// Null-name refusal first; walk m_pHead/m_pNext; for each node the segment
// at +0x0c is tested, then GetName() (0x004420a0) is tested and called
// AGAIN for the compare — two calls in the bytes — and the compare is the
// CRT's case-insensitive one (0x00492f30).
class CDMSegmentLtn1
{
public:
	const char* GetName();                                          // 0x004420a0
};

class CDMPlayingNodeLtn1
{
public:
	CDMPlayingNodeLtn1* m_pNext;             // 0x000
	uint8  m_pad_04[0x0c - 0x04];
	CDMSegmentLtn1* m_pSegment;              // 0x00c
};

class CDMPlayingListLtn1
{
public:
	CDMPlayingNodeLtn1* m_pHead;             // 0x000
	CDMPlayingNodeLtn1* m_pTail;             // 0x004

	CDMPlayingNodeLtn1* FindByName(const char* pName);               // 0x00442030
};

CDMPlayingNodeLtn1* CDMPlayingListLtn1::FindByName(const char* pName)
{
	if (pName)
	{
		CDMPlayingNodeLtn1* pNode = m_pHead;

		while (pNode)
		{
			CDMSegmentLtn1* pSegment = pNode->m_pSegment;

			if (pSegment && pSegment->GetName())
			{
				if (stricmp(pSegment->GetName(), pName) == 0)
					return pNode;
			}

			pNode = pNode->m_pNext;
		}
	}

	return LTNULL;
}


// ---- 0x00441fe0 — CDMSegmentDefList::FindByName ---------------------- //
//
// BELONGS IN: src/engine/lithtech/clientmusic.cpp (clientmusic.h declares
// it).  The sibling of the walk above, over the segments themselves: the
// list head at +0x00, each segment's m_pNext at +0x00, GetName() tested
// and then called again for the stricmp.
class CDMSegmentDefLtn1
{
public:
	CDMSegmentDefLtn1* m_pNext;              // 0x000

	const char* GetName();                                          // 0x004420a0
};

class CDMSegmentDefListLtn1
{
public:
	CDMSegmentDefLtn1* m_pHead;              // 0x000

	CDMSegmentDefLtn1* FindByName(const char* pName);                // 0x00441fe0
};

CDMSegmentDefLtn1* CDMSegmentDefListLtn1::FindByName(const char* pName)
{
	if (pName)
	{
		CDMSegmentDefLtn1* pSegment = m_pHead;

		while (pSegment)
		{
			if (pSegment->GetName())
			{
				if (stricmp(pSegment->GetName(), pName) == 0)
					return pSegment;
			}

			pSegment = pSegment->m_pNext;
		}
	}

	return LTNULL;
}


// ---- 0x004420b0 .. 0x00442240 — the rest of clientmusic's list helpers - //
//
// BELONGS IN: src/engine/lithtech/clientmusic.cpp (clientmusic.h declares
// all five; the addresses run on from the two FindByNames above).  Views,
// with clientmusic.h's offsets.
//
// The banks' inline Free is written out: `if (p) { *(void**)p = free;
// free = p; }` onto the free-list word (bank + 0x10), with its null test
// kept right after a Remove that cannot have changed the pointer, the way
// TermLevel has it.
// The two COM interfaces 0x004421b0 and 0x00442270 call through, with
// only the slots they use: IUnknown::Release (0x08) on the segment and the
// segment state, IDirectMusicPerformance::Stop (0x14) and IsPlaying (0x38).
class IDMUnknownQ7
{
public:
	virtual long __stdcall QueryInterface(void* riid, void** ppv);
	virtual unsigned long __stdcall AddRef();
	virtual unsigned long __stdcall Release();
};

class IDMPerformanceQ7 : public IDMUnknownQ7
{
public:
	virtual long __stdcall Init(void*, void*, void*);
	virtual long __stdcall PlaySegment(void*, uint32, int64, void**);
	virtual long __stdcall Stop(IDMUnknownQ7* pSegment, IDMUnknownQ7* pSegState,
	                            int32 mtTime, uint32 dwFlags);
	virtual long __stdcall GetSegmentState(void**, int32);
	virtual long __stdcall SetPrepareTime(uint32);
	virtual long __stdcall GetPrepareTime(uint32*);
	virtual long __stdcall SetBumperLength(uint32);
	virtual long __stdcall GetBumperLength(uint32*);
	virtual long __stdcall SendPMsg(void*);
	virtual long __stdcall MusicToReferenceTime(int32, int64*);
	virtual long __stdcall ReferenceToMusicTime(int64, int32*);
	virtual long __stdcall IsPlaying(IDMUnknownQ7* pSegment, IDMUnknownQ7* pSegState);
};

class CDMSegmentQ7
{
public:
	CDMSegmentQ7* m_pNext;                   // 0x000
	uint8  m_pad_04[0x08 - 0x04];
	IDMUnknownQ7* m_pDMSegment;              // 0x008
	uint8  m_pad_0c[0x4c - 0x0c];
	char*  m_pName;                          // 0x04c

	int SetName(const char* sName);                                 // 0x00441f20
};

class CDMPlayingNodeQ7
{
public:
	CDMPlayingNodeQ7* m_pNext;               // 0x000
	uint8  m_pad_04[0x08 - 0x04];
	IDMUnknownQ7* m_pSegState;               // 0x008
};

class CDMPendingNodeQ7
{
public:
	CDMPendingNodeQ7* m_pNext;               // 0x000
};

class CLTDirectMusicMgrQ7;

class CDMSegmentListQ7
{
public:
	CDMSegmentQ7* m_pHead;                   // 0x000
	CDMSegmentQ7* m_pTail;                   // 0x004

	void Remove(CDMSegmentQ7* pNode);                               // 0x0048dee0
};

class CDMPendingListQ7
{
public:
	CDMPendingNodeQ7* m_pHead;               // 0x000
	CDMPendingNodeQ7* m_pTail;               // 0x004

	void Remove(CDMPendingNodeQ7* pNode);                           // 0x0048dee0
};

// clientmusic.h's CDMSegmentDefList, with the head it walks.
class CDMSegmentDefListQ7
{
public:
	CDMSegmentQ7* m_pHead;                   // 0x000
	CDMSegmentQ7* m_pTail;                   // 0x004

	void StopSegment(CLTDirectMusicMgrQ7* pMgr, CDMSegmentQ7* pSegment,
	                 int nEnact, int bOnlyIfStopped);               // 0x004421b0
	void Remove(CDMSegmentQ7* pNode);                               // 0x0048dee0
	void StopAll(CLTDirectMusicMgrQ7* pMgr, int bOnlyIfStopped);    // 0x00442180
};

class CDMPlayingListQ7
{
public:
	CDMPlayingNodeQ7* m_pHead;               // 0x000
	CDMPlayingNodeQ7* m_pTail;               // 0x004

	void StopNode(CLTDirectMusicMgrQ7* pMgr, CDMPlayingNodeQ7* pNode,
	              int nEnact, int bOnlyIfStopped);                  // 0x00442270
	void Remove(CDMPlayingNodeQ7* pNode);                           // 0x0048dee0
	void StopAll(CLTDirectMusicMgrQ7* pMgr, int bOnlyIfStopped);    // 0x00442240
};

class CDMIntensityQ7
{
public:
	int m_nUnknown00;                        // 0x000
	int m_nUnknown04;                        // 0x004
	CDMSegmentListQ7 m_Segments;             // 0x008

	~CDMIntensityQ7();                                              // 0x004420b0
};

class CDMTransitionQ7
{
public:
	int   m_nEnact;                          // 0x000
	int   m_bManual;                         // 0x004
	void* m_pDMSegment;                      // 0x008
};

class CLTDirectMusicMgrQ7
{
public:
	uint8 m_pad_00[0x14];
	IDMPerformanceQ7* m_pPerformance;        // 0x014
	uint8 m_pad_18[0x68 - 0x18];
	CDMPendingListQ7 m_PendingList;          // 0x068
	uint8 m_pad_70[0xa0 - 0x70];
	int   m_nNumIntensities;                 // 0x0a0
	uint8 m_pad_a4[0xbc - 0xa4];
	int   m_nNumTransitions;                 // 0x0bc
	CDMTransitionQ7* m_aTransitions;         // 0x0c0

	void ClearPendingList();                                        // 0x00442100
	CDMTransitionQ7* GetTransition(int nFrom, int nTo);             // 0x00442130
};

// The free-list words of the segment (0x004b3f50), pending (0x004b3fc8) and
// playing (0x004b3f98) banks, each at its bank's +0x10.  The banks are
// cltcursor.cpp's, static members of CDMBanks there; restated here with only
// what this block reads (the free list, m_pFree), so the stores below land
// on those objects rather than on globals of their own.
template<class T>
class CDMRecordBank
{
public:
	int          m_bInitialized;             // 0x000
	uint32       m_Unknown04;                // 0x004
	void*        m_pHead;                    // 0x008
	void*        m_pTail;                    // 0x00c
	void*        m_pFree;                    // 0x010
	uint32       m_Unknown14;                // 0x014
};

struct CDMPlainRecord;
struct CDMSegmentRecord;
struct CDMEmptyDtorRecord;

class CDMBanks
{
public:
	static CDMRecordBank<CDMPlainRecord>      s_PendingBank;     // 0x004b3fc8
	static CDMRecordBank<CDMSegmentRecord>    s_SegmentBank;     // 0x004b3f50
	static CDMRecordBank<CDMEmptyDtorRecord>  s_PlayingBank;     // 0x004b3f98
};

#define g_pSegmentBankFreeQ7 (CDMBanks::s_SegmentBank.m_pFree)   // 0x004b3f60
#define g_pPendingBankFreeQ7 (CDMBanks::s_PendingBank.m_pFree)   // 0x004b3fd8
#define g_pPlayingBankFreeQ7 (CDMBanks::s_PlayingBank.m_pFree)   // 0x004b3fa8

// 0x004420b0, 71 bytes.  Clear each segment's name (SetName(0) at
// 0x00441f20), unlink it, delete the allocated name if one is still set,
// and push it back onto the segment bank.
CDMIntensityQ7::~CDMIntensityQ7()
{
	CDMSegmentQ7* pSegment = m_Segments.m_pHead;

	while (pSegment)
	{
		pSegment->SetName(LTNULL);
		m_Segments.Remove(pSegment);

		if (pSegment)
		{
			if (pSegment->m_pName)
				delete [] pSegment->m_pName;

			*(void**)pSegment = g_pSegmentBankFreeQ7;
			g_pSegmentBankFreeQ7 = pSegment;
		}

		pSegment = m_Segments.m_pHead;
	}
}

// 0x00442100, 46 bytes.  Empty the pending list onto its bank.
void CLTDirectMusicMgrQ7::ClearPendingList()
{
	CDMPendingNodeQ7* pNode = m_PendingList.m_pHead;

	while (pNode)
	{
		m_PendingList.Remove(pNode);

		if (pNode)
		{
			*(void**)pNode = g_pPendingBankFreeQ7;
			g_pPendingBankFreeQ7 = pNode;
		}

		pNode = m_PendingList.m_pHead;
	}
}

// 0x00442130, 71 bytes.  Both intensities are range-checked against
// 0xbc (nFrom first), and the row stride is m_nNumIntensities + 1.
CDMTransitionQ7* CLTDirectMusicMgrQ7::GetTransition(int nFrom, int nTo)
{
	if (nFrom <= m_nNumTransitions && nFrom > 0 &&
		nTo <= m_nNumTransitions && nTo > 0)
	{
		return &m_aTransitions[nTo * (m_nNumIntensities + 1) + nFrom + 1];
	}

	return m_aTransitions;
}

// 0x00442180, 47 bytes.  The next link is read before the segment is
// stopped; enact 1 is LTDMEnactDefault.
void CDMSegmentDefListQ7::StopAll(CLTDirectMusicMgrQ7* pMgr, int bOnlyIfStopped)
{
	CDMSegmentQ7* pSegment = m_pHead;

	while (pSegment)
	{
		CDMSegmentQ7* pNext = pSegment->m_pNext;

		StopSegment(pMgr, pSegment, 1, bOnlyIfStopped);

		pSegment = pNext;
	}
}

// 0x004421b0, 137 bytes.  Stop one segment definition and give it back:
// Stop(segment, NULL, 0, 0) on the performance unless it is not playing
// and bOnlyIfStopped is set (IsPlaying answering anything but S_OK), then
// Release the DirectMusic segment, clear the name, unlink it, delete the
// name if one survived and push it onto the segment bank.  nEnact is not
// read.  The bank's null retest is gone: pSegment was tested on entry.
void CDMSegmentDefListQ7::StopSegment(CLTDirectMusicMgrQ7* pMgr, CDMSegmentQ7* pSegment,
                                      int nEnact, int bOnlyIfStopped)
{
	if (!pMgr || !pSegment || !pSegment->m_pDMSegment)
		return;

	if (pMgr->m_pPerformance->IsPlaying(pSegment->m_pDMSegment, LTNULL) != 0 && bOnlyIfStopped)
		return;

	pMgr->m_pPerformance->Stop(pSegment->m_pDMSegment, LTNULL, 0, 0);

	if (pSegment->m_pDMSegment)
		pSegment->m_pDMSegment->Release();

	pSegment->SetName(LTNULL);
	Remove(pSegment);

	if (pSegment)
	{
		if (pSegment->m_pName)
			delete [] pSegment->m_pName;

		*(void**)pSegment = g_pSegmentBankFreeQ7;
		g_pSegmentBankFreeQ7 = pSegment;
	}
}

// 0x00442240, 47 bytes.  The same over the playing nodes, through StopNode.
void CDMPlayingListQ7::StopAll(CLTDirectMusicMgrQ7* pMgr, int bOnlyIfStopped)
{
	CDMPlayingNodeQ7* pNode = m_pHead;

	while (pNode)
	{
		CDMPlayingNodeQ7* pNext = pNode->m_pNext;

		StopNode(pMgr, pNode, 1, bOnlyIfStopped);

		pNode = pNext;
	}
}

// 0x00442270, 112 bytes.  StopSegment's twin for a playing node, keyed by
// its segment state: IsPlaying(NULL, state), Stop(NULL, state, 0, 0),
// Release the state, unlink the node and push it onto the playing bank.
void CDMPlayingListQ7::StopNode(CLTDirectMusicMgrQ7* pMgr, CDMPlayingNodeQ7* pNode,
                                int nEnact, int bOnlyIfStopped)
{
	if (!pMgr || !pNode || !pNode->m_pSegState)
		return;

	if (pMgr->m_pPerformance->IsPlaying(LTNULL, pNode->m_pSegState) != 0 && bOnlyIfStopped)
		return;

	pMgr->m_pPerformance->Stop(LTNULL, pNode->m_pSegState, 0, 0);

	if (pNode->m_pSegState)
		pNode->m_pSegState->Release();

	Remove(pNode);

	if (pNode)
	{
		*(void**)pNode = g_pPlayingBankFreeQ7;
		g_pPlayingBankFreeQ7 = pNode;
	}
}


// ---- 0x0047bc50 — CPacket::ReadType<float> ----------------------------- //
//
// BELONGS IN: src/engine/server/packet.cpp's instantiation driver, beside
// the uint8/uint16/uint32 pointers (0x00459f80 is ReadType<uint8>).  Same
// template, T = float: the payload is `flds (%esi,%eax,1)` and the error
// arm returns the uninitialised val with `flds 0x8(%esp)`.  Forced the way
// packet.cpp forces the others — by taking its address.
typedef float (CPacket::*PacketReadFnFloatLtn1)(float*);
PacketReadFnFloatLtn1 g_pPacketReadFloatLtn1 = &CPacket::ReadType;


// ---- 0x00411d10 — the model file-type's release callback --------------- //
//
// BELONGS IN: the clientmgr.cpp TU (0x00411bf0..0x00411d40 are CClientMgr
// helpers; clientmgr2.cpp/clientmgr3.cpp carry the neighbours).
//
// Reached only through a pointer: it is the first dword of the .data
// object 0x004a8380 (g_Unknown4a8380 in block E), whose ADDRESS 0x00477810
// uses as a list key.  cdecl, three arguments, the first never read.
// When the third is zero, the record at +0x04 of the second is handed to
// 0x00477830 with its first dword (a loaded model — 0x00477830 compares it
// against CClientModel::m_pModel at +0x1dc) and the record itself (which
// 0x00477830 clears through: `movl $0,(%eax)`).  Always returns 0.
// Names ours.
struct CModelRecLtn1
{
	void* m_pModel;                          // 0x000
};

struct CModelOwnerLtn1
{
	uint8 m_pad_00[0x04];
	CModelRecLtn1* m_pRec;                   // 0x004
};

extern class CClientMgrLtn1* g_pClientMgr;  // 0x004b17e0

// 0x00477830 (name ours).
void sub_477830(CClientMgrLtn1* pClientMgr, void* pModel, CModelRecLtn1* pRec);

int sub_411d10(void* pUnused, CModelOwnerLtn1* pOwner, int bFlag)
{
	if (!bFlag)
	{
		CModelRecLtn1* pRec = pOwner->m_pRec;

		if (pRec && pRec->m_pModel)
			sub_477830(g_pClientMgr, pRec->m_pModel, pRec);
	}

	return 0;
}

// 0x004a8380 — the record the callback above is reached through: .data
// {0x00411d10, 0x004ab398}.  serverengine.h's CDepRecord, restated because
// this TU does not include that header (its last two words are that
// header's own; the image has only these two here -- "carddesc" follows at
// 0x004a8388).  The parent is serverengine.cpp's g_DepUnlinkRecord
// (0x004ab398), the same parent its g_ModelHashCleanup names.  Defined
// once, here; cmodelload.cpp and csvis.cpp reach it as an extern.
class CDepLink;

class CDepRecord
{
public:
	LTRESULT (*m_pfnRun)(void* pArg1, class CDepLink* pLink, int nArg3);  // 0x00
	void*  m_pUnknown04;                     // 0x04
	uint32 m_Unknown08;                      // 0x08
	uint32 m_Unknown0c;                      // 0x0c
};

extern CDepRecord g_DepUnlinkRecord;         // 0x004ab398

CDepRecord g_Unknown4a8380 =
{
	(LTRESULT (*)(void*, CDepLink*, int))sub_411d10, &g_DepUnlinkRecord
};


// *********************************************************************** //
//  NOT RECONSTRUCTED, AND WHY
// *********************************************************************** //
//
//  0x00405690, 24 bytes — NOT A FUNCTION.  It is the interior of the
//  function that starts at 0x00405680, whose real extent is
//  0x00405680..0x004056a8 (40 bytes).  The proof is in the register flow,
//  not in the neighbourhood: 0x00405683 computes `add $0x6c,%eax` and
//  0x00405695 pushes that same %eax, so %eax is live INTO 0x00405690 from
//  three instructions above it.  The row is the target of the `jne` at
//  0x00405686 and nothing in the image references 0x00405690 — no call, no
//  data pointer.  The whole function is
//
//      if (!m_p10) return 0x15;
//      return sub_4356d0(m_p10 + 0x6c, arg) ? 0x40 : 0x41;
//
//  (`add $0x6c,%eax; jne` is the null test folded into the offsetting, and
//  `neg; sbb; add $0x41` is the two-constant select, LT_INSIDE/LT_OUTSIDE).
//  It is a `ret $4` thiscall sitting immediately after
//  CommonLT::GetPolyTextureFlags's error block, so it belongs to that same
//  client TU: it is now written there, as ClientCommonLT::GetPointStatus in
//  src/engine/lithtech/cltapi.cpp (identical), and 0x00405690 is
//  not a function.
//
//  0x00459f80, 119 bytes — SOURCE ALREADY EXISTS.  It is
//  src/engine/server/packet.h's `CPacket::ReadType<uint8>`, instantiated by
//  packet.cpp's packet_ForceInstantiation.  Checked instruction by
//  instruction against the template, not by size:
//
//      mov 0x38(%ecx),%dx        m_Pos            (packet.h: 0x38)
//      and $0xffff,%eax          pos as uint32
//      lea 0x1(%eax),%esi        end = pos + sizeof(T),  T = uint8
//      mov 0x58(%ecx),%edi       m_Data.m_AllocSize (0x50 + 0x08)
//      cmp %edi,%esi / jae       !(end < m_Data.m_AllocSize)
//      mov 0x34(%ecx),%di        m_DataLen        (packet.h: 0x34)
//      cmp %edi,%esi / ja        !(end <= m_DataLen)
//      mov 0x54(%ecx),%esi       m_Data.m_pData   (0x50 + 0x04)
//      mov (%esi,%eax,1),%al     val = *(T*)&m_pData[pos]
//      mov %dx,0x38(%ecx)        m_Pos = pos + sizeof(T)
//      or  $0x2,%esi -> 0x40     m_Flags |= PACKETFLAG_READOVERFLOW
//      mov 0xb(%esp),%al         return the UNINITIALISED val
//
//  and the error call is REPORT_ERROR_INFO's four arguments in order:
//  g_pErrorFormat, "CPacket::ReadType" (0x004a864c), "LT_ERROR"
//  (0x004a7360, so the code is stringized symbolically) and "ERROR: Packet
//  read overrun!" (0x004a8660) as the description, at level 1.  That is
//  packet.h's line for line.  Writing a second copy here would duplicate a
//  COMDAT for no gain; if the row still counts as todo, the gap
//  is in how the instantiation is credited, not in the source.
//
//  SIZE CORRECTIONS FOR THE ENUMERATION (both are one function split at a
//  branch target, the same failure mode as 0x00405690 but with the row
//  landing on the real start):
//    0x00406ee0  67 bytes reported, 102 actual (0x00406ee0..0x00406f46).
//    0x00445960  26 bytes reported,  70 actual (0x00445960..0x004459a6).
//
// *********************************************************************** //
