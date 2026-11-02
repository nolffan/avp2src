// *********************************************************************** //
//
//  MODULE:   CLTCLIENT (.CPP)   — staging translation unit (name ours)
//
//  PURPOSE:  Functions read out of lithtech.exe's 0x0043d580..0x0043f91b
//            run.  Everything below was read off the retail lithtech.exe.
//
//  THE RANGE IS NOT ONE TRANSLATION UNIT — IT IS FIVE, AND THREE OF THEM
//  ARE ALREADY IN THE CLIENT'S LINK.  Of the 51 enumerated rows, 38 belong to
//  sources the repo already has and 13 are new.  The ATTRIBUTION block at
//  the foot of this file names every one of the 38 with its proof, so the
//  run can be credited without being rewritten; defining any of them here
//  would duplicate a symbol the link already has.  The split is:
//
//      0x0043d550..0x0043de3f   ..\server\lmessage.cpp   (LMessageImpl)   6
//      0x0043de60..0x0043e32f   an UNIDENTIFIED client TU               3  <- 1 written
//      0x0043e330..0x0043ea9x   ..\server\localdriver.cpp (CLocalDriver) 17
//      0x0043eba0..0x0043ed af  the ILTCursor implementation            10  <- ALL written
//      0x0043edb0..0x0043f91b   clientmusic.cpp (CDMBank, CLTDirectMusicMgr) 15
//
//  WHAT THIS FILE DEFINES: the ten ILTCursor/ILTCursorInst bodies (a whole
//  self-contained TU, pinned by src/LT2/sdk/inc/iltcursor.h slot for slot), plus
//  the bitmap class at 0x0043de60/0x0043df20 and its caller 0x0040ce70.
//
//  BUILD: /O2, like the rest of src/engine/lithtech.  The cursor block's
//  neighbourhood pads with `nop`, has no ebp frames and hoists the imported
//  ShowCursor address out of its loops, which is what /O2 gives.  (Its two
//  NEIGHBOURS do not: see the flag findings in the attribution block —
//  localdriver.cpp's client compile is NOT /O2 and clientmusic.cpp's is not
//  either.  Both are recorded there because they are worth more than this
//  file is.)
//
// *********************************************************************** //

#include <windows.h>

#include "ltbasedefs.h"
#include "iltcursor.h"
#include "collection.h"
#include "cltdib.h"
#include "cluglob.h"                // g_CluClientGlob, 0x004b0ac0

// windows.h renames engine methods out from under us otherwise.
#undef PostMessage
#undef GetMessage
#undef PeekMessage
#undef CopyFile
#undef PlaySound
#undef GetClassName


// ======================================================================= //
//  0x0043eba0 .. 0x0043edaf — the engine's ILTCursor
// ======================================================================= //
//
//  WHAT PINS THE WHOLE BLOCK.  Two vtables and one SDK header, and after
//  those there is nothing left to guess:
//
//   * 0x004a24c8 is exactly EIGHT slots (0x004a24c8..0x004a24e4; 0x004a24e8
//     is separately referenced, as the vptr 0x0043eca0 stores back into the
//     object it deletes).  src/LT2/sdk/inc/iltcursor.h declares ILTCursor with
//     exactly eight pure virtuals, and every slot agrees with its
//     declaration — argument count, return type and constant alike:
//
//        slot 0 (+0x00) SetCursorMode             0x0043ec10  ret $4
//        slot 1 (+0x04) GetCursorMode             0x0043ec90  ret $4
//        slot 2 (+0x08) IsCursorModeAvailable     0x0043ecc0  ret $4
//        slot 3 (+0x0c) LoadCursorBitmapResource  0x0043ecd0  ret $8
//        slot 4 (+0x10) FreeCursor                0x0043eca0  ret $4
//        slot 5 (+0x14) SetCursor                 0x0043ec60  ret $4
//        slot 6 (+0x18) IsValidCursor             0x0043ec50  ret $4
//        slot 7 (+0x1c) RefreshCursor             0x0043ed80  ret
//
//     The constants settle it beyond the shapes: slot 2 is a bare
//     `mov $0x56,%eax`, and ltcodes.h has LT_YES == 86 == 0x56 — which is
//     word for word what the header's comment on IsCursorModeAvailable asks
//     for ("Since we can't detect this, just return LT_YES for now").
//     Slot 0 answers LT_UNSUPPORTED (83 == 0x53), slot 5 LT_INVALIDPARAMS
//     (60 == 0x3c), slot 3 LT_NOTINITIALIZED (73 == 0x49) and
//     LT_MISSINGCURSORRESOURCE (88 == 0x58) — the last of which exists in
//     ltcodes.h for this one function and nothing else.
//
//   * 0x004a24e8 is THREE slots, all __purecall (0x004928e7), and 0x004a24f4
//     is three real bodies — 0x0043ed60, 0x0043ed70 and 0x00459f60.
//     iltcursor.h's ILTCursorInst declares exactly three pure virtuals:
//     IsValid, SetData, GetData.  0x00459f60 is `mov 0x4(%ecx),%eax; ret`,
//     i.e. GetData, so +0x04 is the handle and the class is eight bytes —
//     which is the size 0x0043ecd0 passes to operator new.
//
//   * the abstract base's vtable exists at all, and 0x0043eca0 stores it,
//     because iltcursor.h gives ILTCursorInst an inline `~ILTCursorInst() {}`.
//     `delete hCursor` on a static type of ILTCursorInst* therefore inlines
//     that destructor, and a polymorphic class's inlined destructor restores
//     its own vptr before the free.  That single store is the strongest
//     evidence in the block that the shipped source really did include this
//     header rather than something like it.
//
//   * the owner at +0x0c is CClientMgr.  0x0043ecd0 reads
//     `*(this+0x0c) + 0x4c0` and calls its +0x330, and
//     src/engine/lithtech/clientinterface.h already puts GetEngineHook at
//     +0x330 of the client's function-pointer table.  The one call site,
//     0x0040fbdb, hands the manager in (`push %ebx` where +0x1518 is
//     m_bConsoleReady and +0x504 is m_Unknown504, both already named in
//     clientmgr2.cpp) and files the answer at CClientMgr+0x1614.
//
//   * "cres_hinstance" (0x004a8364) is the hook name, and the module handle
//     it fetches goes straight into LoadImageA with IMAGE_CURSOR — so the
//     cursors come out of cshell.dll's resources, exactly as the header's
//     comment on LoadCursorBitmapResource says.
//
//  Names of the two non-interface members and of +0x08 are ours; everything
//  else is the SDK's.

// ----------------------------------------------------------------------- //
//  Views for what this TU does not own
// ----------------------------------------------------------------------- //

// src/engine/lithtech/clientinterface.h's CLTClient, as far as the cursor
// reaches it: one function pointer at +0x330.  Declared as a view rather
// than by including that header so this file stays standalone; the offset
// and the prototype are that header's, not ours.
class CLTClientTableView
{
public:
	uint8    m_pad_000[0x330];
	LTRESULT (*GetEngineHook)(char* pName, void** pData);   // 0x330
};

// CClientMgr, as far as the cursor reaches it.  NOT polymorphic at offset
// zero — clientmgr2.cpp's CClientMgrView opens with a plain pad and so does
// clientmgr.h's CClientMgr — so the pad here is a plain pad too.
class CClientMgrCursorView
{
public:
	uint8               m_pad_000[0x4c0];
	CLTClientTableView* m_pClientDE;                        // 0x4c0
};


// ----------------------------------------------------------------------- //
//  ILTCursorInst — vtable 0x004a24f4, eight bytes
// ----------------------------------------------------------------------- //

// NOTE on the one vptr store.  iltcursor.h gives ILTCursorInst a
// user-provided `ILTCursorInst() {}`, so a strict reading of the ABI wants
// TWO vptr stores in a CLTCursorInst construction: the base's, then the
// derived's.  The shipped code at 0x0043ed3b has exactly one.  That is VC6
// /O2 killing the dead base store once the empty base constructor is
// inlined, not evidence that the base constructor is absent — but if this
// comes out two instructions long, that assumption is where to look.
class CLTCursorInst : public ILTCursorInst
{
public:
	CLTCursorInst()       { m_pData = LTNULL; }

	virtual LTRESULT IsValid();                             // 0x0043ed60
	virtual void     SetData(const void* pData);            // 0x0043ed70
	virtual void*    GetData();                             // 0x00459f60

public:
	// +0x00 is the vptr ILTCursorInst brings.
	void* m_pData;                                          // +0x04
};

// 0x0043ed60 — eleven bytes: `mov 0x4(%ecx),%eax; neg %eax; sbb %eax,%eax;
// add $0x57,%eax; ret`.  The neg/sbb pair is -(m_pData != 0), so the answer
// is 0x57 when the handle is null and 0x56 when it is not: LT_NO and LT_YES,
// which are adjacent in ltcodes.h and are why VC6 could fold the select into
// arithmetic at all.
LTRESULT CLTCursorInst::IsValid()
{
	return m_pData ? LT_YES : LT_NO;
}

// 0x0043ed70 — ten bytes, `ret $4`, and no value in %eax on the way out,
// which is what says the slot is void and not LTRESULT.  The const goes
// away on the store; iltcursor.h declares the parameter const and GetData's
// return not, so the member itself cannot be const-qualified.
void CLTCursorInst::SetData(const void* pData)
{
	m_pData = (void*)pData;
}

// 0x00459f60 — outside this run and ICF-folded (`mov 0x4(%ecx),%eax; ret`
// is not unique in this image), so it earns no credit here.  Defined only
// because the vtable above needs all three slots filled.
void* CLTCursorInst::GetData()
{
	return m_pData;
}


// ----------------------------------------------------------------------- //
//  ILTCursor — vtable 0x004a24c8, sixteen bytes
// ----------------------------------------------------------------------- //

class CLTCursor : public ILTCursor
{
public:
	// Defined in the class so VC6 inlines it into cursor_Create, which is
	// where the shipped build put it -- there is no out-of-line constructor
	// at any address.  The store order is the shipped order and it is the
	// REVERSE of declaration order: +0x0c, then +0x08, then +0x04.  The two
	// zeros come out of the same register the null test zeroed, which is why
	// they are stored last.
	CLTCursor(CClientMgrCursorView* pClientMgr)             // in 0x0043eba0
	{
		m_pClientMgr = pClientMgr;
		m_pUnknown08 = LTNULL;
		m_CursorMode = CM_None;
	}

	virtual LTRESULT SetCursorMode(CursorMode cMode);       // 0x0043ec10
	virtual LTRESULT GetCursorMode(CursorMode& cMode);      // 0x0043ec90
	virtual LTRESULT IsCursorModeAvailable(CursorMode cMode);   // 0x0043ecc0
	virtual LTRESULT LoadCursorBitmapResource(const char* pName,
	                                          HLTCURSOR& hCursor);  // 0x0043ecd0
	virtual LTRESULT FreeCursor(const HLTCURSOR hCursor);   // 0x0043eca0
	virtual LTRESULT SetCursor(HLTCURSOR hCursor);          // 0x0043ec60
	virtual LTRESULT IsValidCursor(HLTCURSOR hCursor);      // 0x0043ec50
	virtual LTRESULT RefreshCursor();                       // 0x0043ed80

	// NOT virtual: the vtable stops at eight slots, and both of these are
	// reached by a direct `call` with %ecx loaded, so they are thiscall
	// members that the interface does not publish.
	LTRESULT ShowCursorMode(CursorMode cMode);              // 0x0043ebd0
	LTRESULT sub_0048fa90(CursorMode eOldMode);             // 0x0048fa90

public:
	// +0x00 is the vptr ILTCursor brings.
	CursorMode             m_CursorMode;                    // +0x04
	// Written once, by the constructor, and never read in this image.  Its
	// only proven property is that it is four bytes at +0x08 and starts at
	// zero (name and type ours).
	void*                  m_pUnknown08;                    // +0x08
	CClientMgrCursorView*  m_pClientMgr;                    // +0x0c
};


// 0x0043eba0 — `new CLTCursor(pClientMgr)`, with the constructor inlined.
// Thirty-nine bytes and a plain `ret`, so the function is cdecl and free,
// not a member: `push $0x10; call operator new; add $0x4,%esp;` then the
// compiler's own null guard around the four stores, and `xor %eax,%eax` on
// the guard's other arm.
//
// 0x0010 is the class size, which is what fixes the layout: a vptr and
// three dwords, no more.  The one caller is 0x0040fbdb, in CClientMgr::Init
// (it is the code that sets m_bConsoleReady at +0x1518), and it files the
// result at CClientMgr+0x1614.
//
// A cdecl free function and a STATIC MEMBER are indistinguishable here —
// both are cdecl with the argument left on the caller's stack — so
// `CLTCursor::Create` is equally consistent with the bytes.  Written free
// because nothing in the image reaches it through the class.
//
// NOTE on residual risk: the shipped stores run +0x0c, +0x08, +0x04 —
// descending, i.e. the reverse of declaration order.  Whether VC6 schedules
// them that way from a declaration-order body is not something the bytes can
// answer; if this comes out size-exact with the three stores permuted, that
// is the reason, and the fix is to reorder the body rather than the class.
ILTCursor* cursor_Create(CClientMgrCursorView* pClientMgr)
{
	return new CLTCursor(pClientMgr);
}




// 0x0043ebd0 — fifty-three bytes, `ret $4`, and %ecx is loaded by the caller
// and never read: a thiscall member that happens not to touch `this`.
//
// It is a `switch` and not an if/else chain, and that is not a guess: the
// dispatch is `sub $0x0,%eax; je` then `dec %eax; jne`, the descending
// subtract chain VC6 emits only for switch on a dense set.  A two-armed
// if/else would have read `test %eax,%eax` and `cmp $0x1,%eax`.
//
// The CM_Hardware arm is laid out FIRST (it is the chain's fall-through) and
// CM_None second, which is the order used here.  The imported ShowCursor
// address is hoisted into %esi ahead of both loops by the optimizer, not by
// the source.
LTRESULT CLTCursor::ShowCursorMode(CursorMode cMode)
{
	switch (cMode)
	{
		case CM_Hardware:
		{
			// do/while, not while: the shipped loops test only at the
			// BOTTOM (`push $1; call *%esi; test %eax,%eax; jl` back to
			// the push).  Written as `while (...) ;` VC6 peels the first
			// iteration into a pre-test and the function comes out 80
			// bytes against 53.
			do ; while (::ShowCursor(TRUE) < 0);

			break;
		}

		case CM_None:
		{
			do ; while (::ShowCursor(FALSE) >= 0);

			break;
		}
	}

	return LT_OK;
}


// 0x0048fa90 — `xor %eax,%eax; ret $0x4`.  Outside this run, ICF-folded, and
// so unnameable from the image: it is a one-argument thiscall answering zero
// whose result SetCursorMode discards.  Named after its address on purpose;
// defining it here only gives the call below something to reach, and the
// linker folds the body onto whichever copy it keeps.
LTRESULT CLTCursor::sub_0048fa90(CursorMode eOldMode)
{
	return LT_OK;
}


// 0x0043ec10 — slot 0, sixty-two bytes.
//
// Read off the bytes: the availability test calls this object's OWN slot 2
// (`mov (%esi),%eax; call *0x8(%eax)`) and treats a zero answer as the
// failure — which, since slot 2 answers LT_YES and nothing else, is a test
// that never fires in the shipped build.  Written as the bytes have it
// rather than as it was probably meant.
//
// The guard's arm has its own epilogue (`pop %edi; mov $0x53,%eax; pop %esi;
// ret $4`) and the body shares the function's tail with the fall-through,
// which is the ordinary shape for one constant return and one computed one.
LTRESULT CLTCursor::SetCursorMode(CursorMode cMode)
{
	if (!IsCursorModeAvailable(cMode))
		return LT_UNSUPPORTED;

	if (cMode != m_CursorMode)
	{
		ShowCursorMode(cMode);

		CursorMode eOldMode = m_CursorMode;

		m_CursorMode = cMode;

		sub_0048fa90(eOldMode);
	}

	return LT_OK;
}


// 0x0043ec90 — slot 1, fourteen bytes.  Always LT_OK and always fills the
// reference in, as iltcursor.h's comment promises.
//
// Byte matching currently pairs this address with
// collection.obj's CMoArray<uint8>::operator[] (14 shipped against our 12).
// That pairing is a coincidence of shape, not a match: "copy one dword to
// an out parameter and answer LT_OK" is two instructions and this image has
// several.
LTRESULT CLTCursor::GetCursorMode(CursorMode& cMode)
{
	cMode = m_CursorMode;

	return LT_OK;
}


// 0x0043ecc0 — slot 2, eight bytes: `mov $0x56,%eax; ret $0x4`.  Below the
// enumeration's floor and not in it; written because the vtable needs the slot
// and because the constant is what names the whole class.
LTRESULT CLTCursor::IsCursorModeAvailable(CursorMode cMode)
{
	return LT_YES;
}


// 0x0043ecd0 — slot 3, a hundred and thirty-seven bytes, `ret $0x8`.
//
// Read off the bytes:
//
//   * one scalar local, and only one: the entry `push %ecx` reserves the
//     four bytes at -0x4 that the hook's out-parameter is taken from
//     (`lea 0x8(%esp),%ecx` after two more pushes).  The loaded HCURSOR
//     never reaches the frame — it lives in %edi.
//   * the null test is on the OWNER, not on either argument, and answers
//     LT_NOTINITIALIZED.
//   * the hook's failure arm shares the function's tail with the success
//     arm, because both leave a computed value in %eax; the two constant
//     returns get their own epilogues.  So the hook's own result is what
//     comes back, not a substituted code.
//   * LoadImageA's six arguments push as (hInst, pName, 2, 0, 0, 0), and
//     IMAGE_CURSOR is 2.
//   * `new CLTCursorInst` is followed by a VIRTUAL SetData
//     (`mov (%esi),%edx; call *0x4(%edx)`), which is what says the variable
//     holding it is typed HLTCURSOR and not CLTCursorInst*.  There is no
//     null check on it either — the `je` past the stores is the compiler's
//     own guard on operator new, and %esi doubles as both the zero constant
//     and the merge register.
LTRESULT CLTCursor::LoadCursorBitmapResource(const char* pName,
                                             HLTCURSOR& hCursor)
{
	HINSTANCE hInst;

	if (!m_pClientMgr)
		return LT_NOTINITIALIZED;

	LTRESULT dResult = m_pClientMgr->m_pClientDE->GetEngineHook(
	                       "cres_hinstance", (void**)&hInst);

	if (dResult != LT_OK)
		return dResult;

	HCURSOR hWinCursor = (HCURSOR)LoadImage(hInst, pName, IMAGE_CURSOR,
	                                        0, 0, 0);

	if (!hWinCursor)
		return LT_MISSINGCURSORRESOURCE;

	HLTCURSOR hNewCursor = new CLTCursorInst;

	hNewCursor->SetData(hWinCursor);

	hCursor = hNewCursor;

	return LT_OK;
}


// 0x0043eca0 — slot 4, twenty-eight bytes.  `delete` and nothing else; the
// `movl $0x4a24e8,(%eax)` between the push and the free is ILTCursorInst's
// own inline destructor, which iltcursor.h declares and which restores the
// base vptr because the class is polymorphic.
LTRESULT CLTCursor::FreeCursor(const HLTCURSOR hCursor)
{
	if (hCursor)
		delete hCursor;

	return LT_OK;
}


// 0x0043ec60 — slot 5, forty-four bytes.  No null check on the handle: the
// first thing it does is call through it.  IsValid answers LT_YES or LT_NO,
// so — as in SetCursorMode — the zero test never fires; again written as the
// bytes have it.
LTRESULT CLTCursor::SetCursor(HLTCURSOR hCursor)
{
	if (!hCursor->IsValid())
		return LT_INVALIDPARAMS;

	::SetCursor((HCURSOR)hCursor->GetData());

	return LT_OK;
}


// 0x0043ec50 — slot 6, eleven bytes.  The whole body is the forwarded call:
// `mov 0x4(%esp),%ecx; mov (%ecx),%eax; call *(%eax); ret $0x4`.  `this` is
// clobbered on the way, which is legal precisely because it is never read.
LTRESULT CLTCursor::IsValidCursor(HLTCURSOR hCursor)
{
	return hCursor->IsValid();
}


// 0x0043ed80 — slot 7, forty-eight bytes: the same switch as
// ShowCursorMode above, over the member instead of a parameter, and with a
// plain `ret` because the slot takes nothing.  The source duplicates the
// switch rather than calling the helper — there is no call in the body.
LTRESULT CLTCursor::RefreshCursor()
{
	switch (m_CursorMode)
	{
		case CM_Hardware:
		{
			// do/while, not while: the shipped loops test only at the
			// BOTTOM (`push $1; call *%esi; test %eax,%eax; jl` back to
			// the push).  Written as `while (...) ;` VC6 peels the first
			// iteration into a pre-test and the function comes out 80
			// bytes against 53.
			do ; while (::ShowCursor(TRUE) < 0);

			break;
		}

		case CM_None:
		{
			do ; while (::ShowCursor(FALSE) >= 0);

			break;
		}
	}

	return LT_OK;
}


// ======================================================================= //
//  0x0043df20 — one member of the unidentified class at 0x004a1ab4
// ======================================================================= //
//
// The class is a ~0x458-byte object built on the STACK by its constructor at
// 0x0043de60 (the callers at 0x0040ce70 and 0x0041c5a0 reserve 0x45c and
// 0x478 and hand `lea` of a frame slot in), and it carries TWO vptrs:
// 0x004a1ab4 at +0x000 and 0x004a1ab8 at +0x444.  Below those it has a
// three-or-four-dword array header at +0x448..+0x454 whose teardown calls
// 0x004152b0 — which is collection.h's `BaseDelete<T>(LAlloc*, T*, uint32)`,
// THREE arguments, not the two this comment first claimed; the element count
// is dead for a trivial type, which is what made it look like a two-argument
// free.  Its partner is 0x0040c060, `BaseNew<T>(LAlloc*, T*, uint32)`.  Named
// in clientstreams.cpp, which reuses collection.h directly rather
// than modelling a view — so the array header at +0x448..+0x454 below is
// no longer blocked on naming the template, only on its member order.
// The allocator is g_DefAlloc at 0x004b4e88.  The neighbourhood is
// font work: 0x0041c5cc pushes "ConsoleFont" a few instructions after
// building one of these.
//
// That is as far as the class is identified, so only the one member whose
// body needs nothing else is written here.  See the attribution block for
// why 0x0043de60 and 0x0043df40 are NOT.
// The class itself (PFormat, the palette entry and the bitmap) is in
// cltdib.h, which clientconsole.cpp's console font includes too.

// 0x0043de60 — 182 bytes.  The two vptr stores, then CMoArray's
// `Clear(); Init();` expanded (both of Init's teardowns inlined against the
// freshly cleared header), then ResetCounters as the body's one statement.
CObj4a1ab4View::CObj4a1ab4View()
{
	ResetCounters();
}

// 0x0043df20 — twenty-one bytes, thiscall, no arguments: `xor %eax,%eax`
// then three `mov %eax,disp32(%ecx)` and `ret`.  Three adjacent dwords
// cleared through ONE shared zero register, in ascending order, is VC6 /O2
// on three integer assignments; a float zero would have gone through the x87
// stack or a direct `movl` immediate per store, and /Os would have used
// `andl $0,mem`.
//
// Called twice: by the constructor at 0x0043df0c, as its last act, and at
// 0x0043df5b by 0x0043df40, which re-runs it on the object it is handed.
void CObj4a1ab4View::ResetCounters()
{
	m_Unknown438 = 0;
	m_Unknown43c = 0;
	m_Unknown440 = 0;
}


// ----------------------------------------------------------------------- //
//  0x0043df40 (1006 bytes) -- read a PCX image from a stream into the bitmap.
// ----------------------------------------------------------------------- //
//
// The 128-byte PCX header goes on the stack; only 8 bits per pixel is
// accepted, as one plane (palettised: PFormat::Init(0,...), type 0, which
// is what InitFont's `m_nBitDepth == 0` test reads back) or three (RGB:
// PFormat::InitPValue).  The pixels go in a bytesPerLine * height *
// bytes-per-pixel buffer (CMoArray<uint8>::SetSize2, 0x0040bfe0, on
// g_DefAlloc); the 768-byte palette is read from the last 0x300 bytes of
// the stream and the position put back.  The RLE decode writes each plane
// of an RGB row into its own byte of the 32-bit pixel, every store
// bounds-checked against the buffer.  A run longer than what is left of the
// row, or a stream error at the end, resets the bitmap's counters and
// answers 0.  (names ours)
//
// A suspected "start missing at 0x0043dfa9" is
// wrong: 0x0043dfa9 is the one-plane arm of the plane-count test.
//
// NOT YET IDENTICAL: 999 bytes against 1006 (near).  Settled so far:
//   * the frame is 0x3e8 because the stream length is iltstream.h's inline
//     `GetLen()` -- its local shares a slot with y (shipped 0x0043e0c7 and
//     0x0043e14b both use 0x14(%esp)); a named `len` cost a dword (0x3ec);
//   * the plane test is the nested `if (!= 1) { if (!= 3) return; ... }`:
//     byte compares with the default return inline between them (0x0043df94);
//     a switch gives `movsbl; dec; sub`, a flat if/else chain tail-merges
//     the returns;
//   * nPlane at 0x18(%esp), nCol at 0x20(%esp) follow the statement order in
//     the row set-up and the plane-advance block.
// Left: shipped reloads pDib into %ebx at the row loop's EXIT (0x0043e2de)
// and reads it from its arg slot at the row head (0x0043e155, the 7 bytes);
// ours reloads at the loop bottom and uses %ebx at the head -- and the
// vtable temp of the byte Read is %edx in shipped, %eax in ours.  No effect:
// `>>` for the reads, `&pData[...]`, operand order, declaration order.
struct CPcxHeaderQ41C                        // 0x80 bytes
{
	char  m_Manufacturer;                    // 0x00
	char  m_Version;                         // 0x01
	char  m_Encoding;                        // 0x02
	char  m_BitsPerPixel;                    // 0x03
	short m_XMin;                            // 0x04
	short m_YMin;                            // 0x06
	short m_XMax;                            // 0x08
	short m_YMax;                            // 0x0a
	short m_HRes;                            // 0x0c
	short m_VRes;                            // 0x0e
	uint8 m_EgaPalette[48];                  // 0x10
	char  m_Reserved;                        // 0x40
	char  m_nPlanes;                         // 0x41
	short m_BytesPerLine;                    // 0x42
	short m_PaletteInfo;                     // 0x44
	uint8 m_Filler[58];                      // 0x46
};

// The pixel buffer's SetSize2 is a CALL here (0x0043e015 -> 0x0040bfe0,
// `push $0x4b4e88` first), where the real template at /O2 expands it however
// the call is spelled (measured: SetSize2, SetSize, and one inline wrapper
// above SetSize all inline it) -- VC6's inline budget, which this view
// states rather than reproduces, as cltrest.cpp's CMsgArray and
// servermessage.cpp's CMsgArrayQ12 do for the same body.
class CDibArrayQ41C
{
public:
	LTBOOL SetSize2(uint32 newSize, LAlloc* pAlloc);        // 0x0040bfe0
};

LTBOOL dib_LoadQ41C(ILTStream* pStream, CObj4a1ab4View* pDib)
{
	CPcxHeaderQ41C hdr;
	PFormat format;
	uint8 palette[0x300];
	uint32 nBytesPerPixel, nSize;
	int width, height, y;
	uint32 curPos, nPlane, nCol;
	int nLeft, nCount, j;
	uint8 *pData, *pStart, *pEnd, *pRow, *pOut;
	uint8 value;
	uint32 i;

	pDib->ResetCounters();

	pStream->Read(&hdr, sizeof(hdr));
	if (hdr.m_BitsPerPixel != 8)
		return LTFALSE;

	if (hdr.m_nPlanes != 1)
	{
		if (hdr.m_nPlanes != 3)
			return LTFALSE;
		format.InitPValue();
	}
	else
		format.Init(0, 0, 0, 0, 0);

	nBytesPerPixel = format.GetBytesPerPixel();
	width  = hdr.m_XMax - hdr.m_XMin + 1;
	height = hdr.m_YMax - hdr.m_YMin + 1;
	nSize  = hdr.m_BytesPerLine * height * nBytesPerPixel;

	if (!((CDibArrayQ41C*)&pDib->m_Data)->SetSize2(nSize, &g_DefAlloc))
		return LTFALSE;

	pStart = pDib->m_Data.GetArray();
	pEnd   = pStart + nSize;

	pDib->m_Unknown438 = width;
	pDib->m_Unknown43c = height;
	pDib->m_Unknown440 = hdr.m_BytesPerLine * nBytesPerPixel;
	pDib->m_Format = format;

	pStream->GetPos(&curPos);
	pStream->SeekTo(pStream->GetLen() - 0x300);
	pStream->Read(palette, 0x300);
	pStream->SeekTo(curPos);

	for (i = 0; i < 256; i++)
	{
		pDib->m_Palette[i].m_Pad = 0;
		pDib->m_Palette[i].m_R = palette[i*3 + 0];
		pDib->m_Palette[i].m_G = palette[i*3 + 1];
		pDib->m_Palette[i].m_B = palette[i*3 + 2];
	}

	pData = pDib->m_Data.GetArray();
	if (pData)
	{
		for (y = 0; y < height; y++)
		{
			pRow   = pData + y * pDib->m_Unknown440;
			nPlane = 0;
			pOut   = pRow;
			nLeft  = hdr.m_nPlanes * hdr.m_BytesPerLine;
			nCol   = hdr.m_BytesPerLine;

			while (nLeft > 0)
			{
				pStream->Read(&value, 1);
				if ((value & 0xc0) == 0xc0)
				{
					nCount = value & 0x3f;
					pStream->Read(&value, 1);
				}
				else
				{
					nCount = 1;
				}

				if (nCount > nLeft)
				{
					pDib->ResetCounters();
					return LTFALSE;
				}

				if (pDib->m_Format.m_nBitDepth == 0)
				{
					for (j = 0; j < nCount; j++)
					{
						if (pOut >= pStart && pOut <= pEnd)
							*pOut = value;
						pOut++;
						nLeft--;
					}
				}
				else
				{
					for (j = 0; j < nCount; j++)
					{
						nLeft--;
						if (pOut >= pStart && pOut <= pEnd)
						{
							if (nPlane == 0)
								*(uint32*)pOut = (*(uint32*)pOut & 0xffff) | ((uint32)value << 16);
							else if (nPlane == 1)
								*(uint32*)pOut = (*(uint32*)pOut & 0xff00ff) | ((uint32)value << 8);
							else if (nPlane == 2)
								*(uint32*)pOut = (*(uint32*)pOut & 0xffff00) | value;
						}

						pOut += nBytesPerPixel;
						if (--nCol == 0)
						{
							nPlane++;
							pOut = pRow;
							nCol = hdr.m_BytesPerLine;
						}
					}
				}
			}
		}
	}

	if (pStream->ErrorStatus())
	{
		pDib->ResetCounters();
		return LTFALSE;
	}

	return LTTRUE;
}


// 0x0040ce70 — 184 bytes.  clientinterface.h's ci_CreateSurfaceFromBitmap
// (SDK slot 0x16c).  HOME IS cissurf.cpp (it sits between cis_IsRowColor
// 0x0040cce0 and cis_LoadDIB 0x0040cf30); written here because this file
// owns the bitmap class.  A 0x45c frame for the bitmap, the load, and the
// bitmap's implicit destructor inlined on both exits (~CMoArray -> Term).
HSURFACE cis_CreateSurfaceFromDIBQ43D(CObj4a1ab4View* pDib);           // 0x0040c260
LTBOOL   cis_LoadDIBQ43D(char* pBitmapName, CObj4a1ab4View* pDib);     // 0x0040cf30

HSURFACE ci_CreateSurfaceFromBitmap(char* pBitmapName)
{
	CObj4a1ab4View dib;

	if (!cis_LoadDIBQ43D(pBitmapName, &dib))
		return LTNULL;

	return cis_CreateSurfaceFromDIBQ43D(&dib);
}


// ----------------------------------------------------------------------- //
//  0x0041cd50 (279 bytes) -- the console's backdrop loader.
// ----------------------------------------------------------------------- //
//
// HOME is the console TU (ccline.cpp's CcnConsole, 0x0041c910..0x0041d033,
// sits either side of it); written HERE because this file owns the bitmap
// class it builds on its stack (0x45c frame, the same as 0x0040ce70), and
// the bitmap's implicit destructor is what most of the bytes are.
//
// What pins it: FreeSurface (0x0041c570) first, then "console.pcx"/"rb"
// through 0x00483f50 (demodtx.cpp: fopen + CFileStream); no stream answers
// 1 at once.  0x0043df40 reads the PCX into the bitmap; on success
// 0x0040c260 makes the surface (stored at +0x3a4 and pushed from %eax, not
// reloaded), the client table's +0x160 OptimizeSurface(surface, 0) goes to
// +0x3ac, and SetAlpha (0x0041ce70) re-applies +0x3a8.  The stream is
// released through its vtable +0x04 (iltstream.h's Release) on both arms,
// and the answer is %ebx: 1 until the load succeeds, then 0 -- an LTRESULT
// (LT_ERROR / LT_OK).  The field names are ccline.cpp's.  (names ours)
//
// IDENTICAL.  The table goes through a named local (pTable): with the chain
// written inline, VC6 loads +0x3a8 for SetAlpha into %edx ABOVE the +0x3ac
// store and the Release dispatch takes %eax (5 mnemonic blocks); shipped
// stores first and reuses %eax for the alpha.
class ILTStream;
ILTStream* sub_483f50Q41C(char* pFilename, char* pMode);          // 0x00483f50
LTBOOL     dib_LoadQ41C(ILTStream* pStream, CObj4a1ab4View* pDib);  // 0x0043df40

class CClientTableQ41C
{
public:
	uint8    m_pad_000[0x160];
	LTRESULT (*OptimizeSurface)(HSURFACE hSurface, uint32 hTransparentColor);  // 0x160
};

class CClientMgrQ41C
{
public:
	uint8             m_pad_000[0x4c0];
	CClientTableQ41C* m_pClientDE;           // 0x4c0
};

// 0x004b0b14 -- g_CluClientGlob.m_pClientMgr, read through this view.

class CConsoleQ41C
{
public:
	void     FreeSurface();                            // 0x0041c570
	void     SetAlpha(float fAlpha);                   // 0x0041ce70
	LTRESULT LoadBackground();                         // 0x0041cd50

	uint8        m_pad_000[0x3a4];
	HSURFACE     m_hSurface;                 // 0x3a4
	float        m_fAlpha;                   // 0x3a8
	LTRESULT     m_nOptimized;               // 0x3ac
	uint8        m_pad_3b0[0x3b4 - 0x3b0];
	HWND         m_hMainWnd;                 // 0x3b4  (clientconsole.cpp's name)
};

LTRESULT CConsoleQ41C::LoadBackground()
{
	CObj4a1ab4View bitmap;
	ILTStream* pStream;
	CClientTableQ41C* pTable;
	LTRESULT dResult;

	FreeSurface();

	dResult = LT_ERROR;

	pStream = sub_483f50Q41C("console.pcx", "rb");
	if (!pStream)
		return LT_ERROR;

	if (dib_LoadQ41C(pStream, &bitmap))
	{
		m_hSurface = cis_CreateSurfaceFromDIBQ43D(&bitmap);
		pTable = ((CClientMgrQ41C*)g_CluClientGlob.m_pClientMgr)->m_pClientDE;
		m_nOptimized = pTable->OptimizeSurface(m_hSurface, 0);
		SetAlpha(m_fAlpha);
		dResult = LT_OK;
	}

	pStream->Release();

	return dResult;
}


// 0x0041c5a0 (830 bytes), the console font, is clientconsole.cpp's
// CConsoleView::InitFont: it sits between FreeSurface (0x0041c570) and
// Free29c (0x0041c8e0) in the console TU, and the bitmap it builds is
// cltdib.h's.


// ======================================================================= //
//  clientmusic.cpp's six record banks — WRITTEN HERE, HOME IS clientmusic.cpp
// ======================================================================= //
//
// The real home of everything in this block is clientmusic.cpp (the
// addresses 0x0043efb0..0x0043f515 sit between that file's CDM* list code
// and CLTDirectMusicMgr::ChangeIntensity).  It is written here only because
// this is the file this batch owns; the objects are the banks clientmusic.h
// declares as `extern CDMBank g_*Bank`, and moving the block into
// clientmusic.cpp means turning those externs into the static members below.
//
// WHAT THE GUARD BYTE IS.  Every one of the six atexit destructors opens
// `mov 0x004b3fdc,%cl; mov $bit,%al; test; jne out; or %al,%cl` with a
// different bit (1, 2, 4, 8, 0x10, 0x20) of one shared byte, while the six
// constructors are unguarded.  VC6 emits exactly that — one `$S` byte for
// the whole TU, one bit per object in definition order, destructor-only —
// for the out-of-class DEFINITION OF A STATIC DATA MEMBER.  A plain
// file-scope global (`CDMBank g_X;`) gets no guard at all; a static member
// of a class template instantiated implicitly guards its constructor too.
// Measured in a scratch TU at /O2, cl 8447.  So in the original the banks
// are static members of some class, defined in this order:
//
//      bit 0x01  pending    0x004b3fc8   ctor 0x0043efc0  dtor 0x0043eff0
//      bit 0x02  segment    0x004b3f50   ctor 0x0043f090  dtor 0x0043f0c0
//      bit 0x04  playing    0x004b3f98   ctor 0x0043f1a0  dtor 0x0043f1d0
//      bit 0x08  band       0x004b3fb0   ctor 0x0043f280  dtor 0x0043f2b0
//      bit 0x10  unknown58  0x004b3f68   ctor 0x0043f350  dtor 0x0043f380
//      bit 0x20  unknown60  0x004b3f80   ctor 0x0043f460  dtor 0x0043f490
//
// WHAT DIFFERS BETWEEN THE SIX is only how a chunk's record array is
// released, so the bank is a template over its record type and each chunk
// goes back through `delete []`:
//   * pending, band, unknown60 — records with no destructor: a bare
//     operator delete of the chunk.
//   * playing — records with an EMPTY destructor: the array cookie is read
//     and `dec`ed (0x0043f22f..0x0043f235) and the loop then vanishes.
//   * segment (0x54-byte records) and unknown58 (0x50) — a real destructor,
//     called per element backwards (0x0043f141, 0x0043f3fa) through the
//     CRT's __ehvec_dtor shape that `delete []` expands to.  Both call one 15-byte body at
//     0x0043f440 (`if (m_pName) delete m_pName;`), identical COMDATs the
//     linker folded.
// Record and bank names are ours.

struct CDMBankNode
{
	CDMBankNode* m_pNext;                    // 0x000
	CDMBankNode* m_pPrev;                    // 0x004
	void*        m_pRecords;                 // 0x008  the chunk, new T[n]
};

template<class T>
class CDMRecordBank
{
public:
	int          m_bInitialized;             // 0x000
	uint32       m_Unknown04;                // 0x004
	CDMBankNode* m_pHead;                    // 0x008
	CDMBankNode* m_pTail;                    // 0x00c
	void*        m_pFree;                    // 0x010
	uint32       m_Unknown14;                // 0x014

	CDMRecordBank()
	{
		m_pHead = LTNULL;
		m_pTail = LTNULL;
		m_bInitialized = 0;
	}

	~CDMRecordBank()
	{
		if (m_bInitialized)
		{
			CDMBankNode* pNode;

			while ((pNode = m_pHead) != LTNULL)
			{
				if (pNode->m_pPrev)
					pNode->m_pPrev->m_pNext = pNode->m_pNext;
				else
					m_pHead = pNode->m_pNext;

				if (pNode->m_pNext)
					pNode->m_pNext->m_pPrev = pNode->m_pPrev;
				else
					m_pTail = pNode->m_pPrev;

				delete [] (T*)pNode->m_pRecords;
				delete pNode;
			}

			m_pFree = LTNULL;
			m_bInitialized = 0;
		}
	}
};

// Records with nothing to destroy.
struct CDMPlainRecord
{
	uint32 m_Data;
};

// Records with a destructor that does nothing.
struct CDMEmptyDtorRecord
{
	uint32 m_Data;

	~CDMEmptyDtorRecord() {}
};

// 0x0043f440: the record types that own a name at +0x4c.
struct CDMSegmentRecord
{
	uint8  m_pad_00[0x4c];
	char*  m_pName;                          // 0x04c
	uint32 m_nStopEnact;                     // 0x050

	~CDMSegmentRecord();
};

struct CDMUnknown58Record
{
	uint8  m_pad_00[0x4c];
	char*  m_pName;                          // 0x04c

	~CDMUnknown58Record();
};

// Out of line: the teardown loops CALL it (0x0043f146, 0x0043f3ff), which
// /Ob1 would not do for a body written in the class.
CDMSegmentRecord::~CDMSegmentRecord()
{
	if (m_pName)
		delete m_pName;
}

CDMUnknown58Record::~CDMUnknown58Record()
{
	if (m_pName)
		delete m_pName;
}

class CDMBanks
{
public:
	static CDMRecordBank<CDMPlainRecord>      s_PendingBank;
	static CDMRecordBank<CDMSegmentRecord>    s_SegmentBank;
	static CDMRecordBank<CDMEmptyDtorRecord>  s_PlayingBank;
	static CDMRecordBank<CDMPlainRecord>      s_BandBank;
	static CDMRecordBank<CDMUnknown58Record>  s_Unknown58Bank;
	static CDMRecordBank<CDMPlainRecord>      s_Unknown60Bank;
};

CDMRecordBank<CDMPlainRecord>      CDMBanks::s_PendingBank;     // 0x004b3fc8
CDMRecordBank<CDMSegmentRecord>    CDMBanks::s_SegmentBank;     // 0x004b3f50
CDMRecordBank<CDMEmptyDtorRecord>  CDMBanks::s_PlayingBank;     // 0x004b3f98
CDMRecordBank<CDMPlainRecord>      CDMBanks::s_BandBank;        // 0x004b3fb0
CDMRecordBank<CDMUnknown58Record>  CDMBanks::s_Unknown58Bank;   // 0x004b3f68
CDMRecordBank<CDMPlainRecord>      CDMBanks::s_Unknown60Bank;   // 0x004b3f80

// ======================================================================= //
//  ATTRIBUTION ONLY — 38 enumerated rows whose bodies belong elsewhere
// ======================================================================= //
//
//  ----------------------------------------------------------------------
//  A.  ..\server\lmessage.cpp — 6 rows.  ALREADY SOURCED.
//  ----------------------------------------------------------------------
//
//  build/engine.sh's lithtech line compiles ..\server\lmessage.cpp, and
//  twenty-nine of its symbols already match in this run
//  (0x0043d550 ReadByteFL through 0x0043de30 GetStatus).  The six enumerated
//  rows are the six that file's client compile does not yet reproduce.
//
//  What names them exactly: 0x004ab3a0 holds the `static char*` that
//  0x0043d5ee pushes, and it reads
//  "LMessageImpl::ReadStringFL".  That single dereference identifies the
//  whole vtable, and the vtable then identifies every row, because
//  src/LT2/sdk/inc/iltmessage.h declares ILTMessage's readers in exactly this
//  order — ReadStringFL, ReadHStringFL, ReadHStringAsStringFL, ReadRawFL:
//
//      vtable 0x004a22d4 (LMessageImpl)     vtable 0x004a2b6c (the other one)
//        slot 4  0x0043d580 ReadStringFL      same body (folded)
//        slot 5  0x0043d630 ReadHStringFL     same
//        slot 6  0x0043d6e0 ReadHStringAsStringFL  same
//        slot 7  0x0043ddc0 ReadRawFL         0x00431fa0 (the guard-word one)
//        slot 9  0x0043d820 ReadCompVectorFL  same
//        slot 13 0x0043d9f0 ReadMessageFL     same
//
//   0x0043d580  ReadStringFL          174.  NOW IDENTICAL: `for (i = 0;`
//               after the checks, and a read then `while (ch)` drain.
//   0x0043d630  ReadHStringFL         170.  NOW IDENTICAL: one exit, the
//               `nExtra = 0` arm first.  (Both edits in lmessage.cpp;
//               server.dll unchanged, identical.)
//   0x0043d6e0  ReadHStringAsStringFL 208.
//   0x0043d820  ReadCompVectorFL      281.  Already on the near list,
//               COUNT-EXACT (281 against 281).
//   0x0043d9f0  ReadMessageFL         189.
//   0x0043ddc0  ReadRawFL             64.  NOW IDENTICAL: lmsg_AfterRead
//               moved from servermsg.cpp into lmessage.cpp as an `inline`
//               defined just above ReadRawFL.  /O2 expands it; server.dll's
//               /O1 compile declines and emits it out of line right after
//               ReadRawFL (0x10014cf8), which is where server.dll has it.
//               (Before: ours was 36 because lmessage.cpp called it and the
//               client's compile INLINES it:
//               the shipped tail reads the owner's +0x38 and +0x34 as
//               uint16, compares the difference `jg`, and on the
//               non-positive side sets +0x38 to 1 under `+0x40 & 1`.
//               Same source, different -O.  The sibling at 0x00431fa0 is
//               the debug message class's ReadRawFL — it re-reads four
//               bytes and compares them against 0xABCDEFAF — and is NOT
//               this row.)
//
//  ----------------------------------------------------------------------
//  B.  ..\server\localdriver.cpp — 17 rows.  ALREADY SOURCED, AND THE
//      BLOCKER IS A COMPILER FLAG, NOT A LINE OF SOURCE.
//  ----------------------------------------------------------------------
//
//  This is the largest single finding in the range and it is worth acting on
//  before anything else here.  0x0043e330..0x0043ea99 is the client's
//  compile of src/engine/server/localdriver.cpp, and it is compiled WITHOUT
//  optimization: every function opens `push %ebp; mov %esp,%ebp`, spills
//  `this` to -0x4(%ebp) and RELOADS IT BEFORE EVERY SINGLE STORE, shares no
//  subexpression, and the block pads with `int3` where the rest of the image
//  pads with `nop`.  build/engine.sh compiles it into lithtech.exe at /O2,
//  so all seventeen rows are guaranteed to miss no matter what the source
//  says.  localdriver.cpp's own header comment already records that
//  server.dll needs /Os-without-/Og for this file; the client needs
//  something weaker still, and the seventeen rows below are the payoff for
//  finding it.  (The file's server compile is fine — do not touch the
//  source.)
//
//  The identification is not from the flags, it is from the code:
//
//    * 0x0043e3e0 stores 0x004a223c (CBaseDriver's vtable) at +0x00,
//      120.0f at +0x4c and 0 at +0x44 — servermgr.h's m_Timeout and
//      m_DriverFlags — and only then its own 0x004a244c.
//    * 0x0043e560 memcpy's "local" (0x004a8528) into this+0x04 and answers
//      1.  localdriver.cpp's comment on CLocalDriver::Init already says
//      "0x10014fb0 writes 'local' here".
//    * 0x0043e370 clears +0x18, then +0x00, then +0x0c — the asymmetric
//      order localdriver.cpp's comment on CLocalQueue::CLocalQueue records
//      verbatim, and 0x0043e3a0 is its destructor's `if (m_pData) { delete;
//      m_pData = 0; }`.
//    * 0x0043e330 is ldrv_Create: `operator new(0xa0)` — CLocalDriver's
//      size — null-checked, then the constructor.
//
//  0x004a244c is therefore CLocalDriver's vtable, THIRTY-ONE slots
//  (0x004a244c..0x004a24c4): CBaseDriver's twenty-seven plus the four
//  localdriver.cpp already declares.  0x004a24c8 is the next vtable, not
//  slot 31 — which is how the cursor class above was found.  Slots 5..22
//  hold the base's shared defaults (0x0042d180/90/a0/b0/c0, 0x00432030/40),
//  exactly as clientworld.cpp records for CBaseDriver itself.
//
//      row          size  localdriver.cpp symbol            vtable slot
//      0x0043e330    55   ldrv_Create                       —
//      0x0043e370    43   CLocalQueue::CLocalQueue          —
//      0x0043e3a0    53   CLocalQueue::~CLocalQueue         —
//      0x0043e3e0   245   CLocalDriver::CLocalDriver        —
//      0x0043e4e0    13   CLocalDriver::GetRecvSize         26 (+0x68)
//      0x0043e4f0    46   CLocalDriver::??_G (scalar delete)  0 (+0x00)
//      0x0043e520    59   CLocalDriver::~CLocalDriver       —
//      0x0043e560    62   CLocalDriver::Init                 1 (+0x04)
//      0x0043e5a0   336   CLocalDriver::Term                 2 (+0x08)
//      0x0043e6f0   170   CLocalDriver::Update               3 (+0x0c)
//      0x0043e7a0    51   CLocalDriver::Connect              4 (+0x10)
//     (0x0043e7e0    64   CLocalDriver::Disconnect          23 (+0x5c) — already named)
//      0x0043e820    56   CLocalDriver::SendPacket          24 (+0x60)
//      0x0043e860   200   CLocalDriver::ReceivePacket       25 (+0x64)
//      0x0043e930    28   CLocalDriver::_p6c                27 (+0x6c)
//      0x0043e950    42   CLocalDriver::_p70                28 (+0x70)
//      0x0043e980   106   CLocalDriver::_p74                29 (+0x74)
//      0x0043e9f0   424   CLocalDriver::_p78                30 (+0x78)
//
//  A second, smaller consequence: the four rows the near list currently
//  credits to localdriver.obj in lithtech.exe — 0x0048f9c0 (_p6c),
//  0x004871e0 (ldrv_Create), 0x00491e30 (~CLocalQueue) and 0x00401c40
//  (~CLocalDriver) — are all mispaired.  The real addresses are the ones in
//  the table above, and the pairings the tool made are elsewhere in the
//  image.  Do not chase them as nears.
//
//  ----------------------------------------------------------------------
//  C.  src/engine/lithtech/clientmusic.cpp — 15 rows.  SOURCE FILE EXISTS.
//  ----------------------------------------------------------------------
//
//  clientmusic.h ALREADY DECLARES the six globals this block constructs and
//  destroys, at exactly these addresses:
//
//      g_SegmentBank     0x004b3f50   (free list at +0x10 = 0x004b3f60)
//      g_Unknown58Bank   0x004b3f68   (0x004b3f78)
//      g_Unknown60Bank   0x004b3f80   (0x004b3f90)
//      g_PlayingBank     0x004b3f98   (0x004b3fa8)
//      g_BandBank        0x004b3fb0   (0x004b3fc0)
//      g_PendingBank     0x004b3fc8   (0x004b3fd8)
//
//  so the block is CDMBank's constructor and destructor, six times over,
//  with the object folded to absolute addresses because each instance is a
//  file-scope global.  The .CRT$XCU table at 0x004a7004 carries the six
//  initializers in its slots 33..38 (0x0043efb0, 0x0043f080, 0x0043f190,
//  0x0043f270, 0x0043f340, 0x0043f450), each a ten-byte
//  `call <ctor>; jmp <atexit thunk>` that the enumeration does not list.
//
//  CDMBank's layout falls straight out: +0x00 a flag the destructor tests
//  and clears last, +0x08 the list head, +0x0c the list tail, +0x10 the free
//  list.  Each node is {+0x00 pNext, +0x04 pPrev, +0x08 pData}, and the
//  teardown unlinks from the head, frees pData, frees the node, and repeats.
//
//  Two of the six differ, and the difference is the element type:
//
//    * 0x0043f0c0 (g_SegmentBank, 195 bytes) and 0x0043f1d0 (g_PlayingBank,
//      152) release pData through VC6's ARRAY delete — `mov -0x4(%eax)` for
//      the count, then `operator delete(p - 4)`.
//    * 0x0043f380 (g_Unknown58Bank, 188) does the same and runs a
//      destructor over each element first, backwards, with a 0x50-byte
//      stride: 0x0043f440, `if (p->m_0x4c) operator delete(p->m_0x4c);`.
//      So CDMUnknown58 is 0x50 bytes with one owned pointer at +0x4c.
//
//  Every destructor opens `mov 0x004b3fdc,%cl; test <bit>,%cl; jne out;
//  or <bit>,%cl` with a DIFFERENT bit per object (1, 2, 4, 8, 0x10, 0x20)
//  in one shared byte at 0x004b3fdc.  That byte is not a member of any of
//  the six — it is one bitmask for all of them — and what puts it there is
//  not settled here.  It matters for reproduction and should be settled
//  before these are attempted.
//
//      0x0043efc0    18   CDMBank::CDMBank      g_PendingBank
//      0x0043eff0   134   CDMBank::~CDMBank     g_PendingBank      bit 0x01
//      0x0043f090    18   CDMBank::CDMBank      g_SegmentBank
//      0x0043f0c0   195   CDMBank::~CDMBank     g_SegmentBank      bit 0x02
//      0x0043f1a0    18   CDMBank::CDMBank      g_PlayingBank
//      0x0043f1d0   152   CDMBank::~CDMBank     g_PlayingBank      bit 0x04
//      0x0043f280    18   CDMBank::CDMBank      g_BandBank
//      0x0043f2b0   134   CDMBank::~CDMBank     g_BandBank         bit 0x08
//      0x0043f350    18   CDMBank::CDMBank      g_Unknown58Bank
//      0x0043f380   188   CDMBank::~CDMBank     g_Unknown58Bank    bit 0x10
//      0x0043f440    15   CDMUnknown58::~CDMUnknown58
//      0x0043f460    18   CDMBank::CDMBank      g_Unknown60Bank
//      0x0043f490   134   CDMBank::~CDMBank     g_Unknown60Bank    bit 0x20
//      0x0043f520   874   NOT ChangeIntensity (that is 0x00440620): the
//                         DirectMusic notification thread Init hands to
//                         _beginthread (0x0043fa79).  Written as
//                         dm_NotificationThread in clientmusic.cpp.  It
//                         reaches g_PlayingBank (0x0043f6ef `mov
//                         $0x4b3f98,%ecx`) and the manager singleton at
//                         0x004b3fe0, which is what places it.
//      0x0043f890   139   CLTDirectMusicMgr::CLTDirectMusicMgr.  NOT yet in
//                         clientmusic.cpp.  Stores 0x004a2500 (the vtable
//                         whose slots 2..8 clientmusic.cpp already matches:
//                         InitLevel 0x0043fcd0, TermLevel 0x00440230, Play
//                         0x00440420, Stop 0x00440480, SetVolume
//                         0x004405a0), zeroes +0x04..+0x18, +0x40..+0x80,
//                         +0xb8..+0xc8, sets +0xac and +0xb0 to 64 and
//                         +0xb4 to 0xac44 (44100 — the sample rate), and
//                         publishes `this` to the singleton at 0x004b3fe0.
//
//  UPDATE: the six destructors and 0x0043f440 ARE now written, in the
//  "record banks" block above, which also settles the guard byte (VC6's
//  destructor guard for out-of-class static data member definitions).  The
//  paragraph below is the original reasoning for leaving them out; the
//  /GF difference turned out not to matter for these bodies.
//
//  These were NOT written here for a mechanical reason as well as a
//  duplicate-symbol one: build/engine.sh compiles clientmusic.cpp alone
//  under NOPOOL_FLAGS (/Ot /Og /Oy /Ob1 /Oi /Gs /Gy), not /O2, so a body
//  compiled into this file could not reproduce them even if it were correct.
//
//  ----------------------------------------------------------------------
//  D.  SKIPPED DELIBERATELY — 2 rows in the unidentified TU
//  ----------------------------------------------------------------------
//
//   0x0043de60  182 bytes.  NOW WRITTEN AND IDENTICAL (CObj4a1ab4View's
//               constructor, above): a PFormat MEMBER at +0 and a
//               CMoArray<uint8> member at +0x444.  Its caller 0x0040ce70
//               (ci_CreateSurfaceFromBitmap) is written above too.
//   0x0043df40 1006 bytes.  Builds a second instance of the same class in
//               its own frame (`movl $0x4a1ab4,0x40(%esp)`), calls
//               0x0043df20 on its argument, then reads 0x80 bytes through
//               its third argument's slot 2 and branches on byte 3 of the
//               result being 8 and on byte 0xb9 being 1 or 3 — a bit-depth
//               and a format.  Same blocker, an order of magnitude more of
//               it.
//
// *********************************************************************** //
