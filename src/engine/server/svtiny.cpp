// server.dll — the leftover small rows (reconstruction). Build /O1.
//
// *********************************************************************** //
//  WHAT THIS FILE IS, AND WHAT IT IS NOT
//
//  A scratch translation unit for nine bodies that came out of the final
//  small-row triage of server.dll's function list. It is a HOLDING
//  PEN, not a home: every one of these bodies belongs in a TU that already
//  exists, and the "BELONGS IN" line on each says which. Nothing here
//  should ship in this file — fold each one into its real TU and re-measure
//  BOTH images (lithtech.exe compiles 44 of the server TUs a second time at
//  /O2).
//
//  Six of the nine are UNDER EIGHT UNMASKED BYTES, so a byte match can never
//  place them and they can never be credited. They are written anyway
//  because the bodies are forced and the evidence is cheap to record; each
//  is marked UNVERIFIABLE.
//
//  The classes below are VIEWS. Where the real class already exists in the
//  tree the view says so and the fold is a two-line edit; where it does not,
//  the view is named by address, which is this project's convention
//  (svnew.cpp's CGeomView, ltnew1.cpp's CSub445960View).
//
//  SHARED SOURCE. Two of the nine touch source that both images compile:
//  CheapLTLink::AddAfter is src/LT2/sdk/inc/ltlink.h verbatim (a header every
//  engine TU includes), and CObjBase::_v2c is objtypes.h's hierarchy, which
//  lithtech.exe's server half also builds. Neither needs a header CHANGE —
//  AddAfter is already declared and defined in ltlink.h — but any edit to
//  objtypes.h moves both images and must be measured on both.
// *********************************************************************** //
#include <string.h>
#include "ltbasedefs.h"


// ======================================================================= //
//  1.  0x100270be, 22 bytes — CheapLTLink::AddAfter.
//      VERIFIABLE.  BELONGS IN: nothing to write. ltlink.h already has it.
//
//  This row needs no new source at all. The shipped bytes are
//  src/LT2/sdk/inc/ltlink.h's CheapLTLink::AddAfter, compiled OUT OF LINE:
//
//      mov 0x4(%esp),%eax     pLink
//      mov %ecx,(%eax)        pLink->m_pPrev = this
//      mov 0x4(%ecx),%ecx     m_pNext
//      mov %ecx,0x4(%eax)     pLink->m_pNext = m_pNext
//      mov %eax,(%ecx)        pLink->m_pNext->m_pPrev = pLink   (cached)
//      mov (%eax),%ecx        RELOAD pLink->m_pPrev
//      mov %eax,0x4(%ecx)     pLink->m_pPrev->m_pNext = pLink
//      ret $0x4
//
//  Three things pin it, and all three are the header's own text.
//   * `this` in %ecx with one pushed argument and `ret $4` — a __thiscall
//     member taking one pointer, which is AddAfter's signature and not
//     dl_Insert's (that one is a two-argument free inline that forwards).
//   * the double assignment `pLink->m_pPrev->m_pNext = pLink->m_pNext->
//     m_pPrev = pLink` evaluates RIGHT TO LEFT, so the m_pNext->m_pPrev
//     store is emitted first — shipped emits it first.
//   * the RELOAD of pLink->m_pPrev before the last store. VC6 forwarded the
//     store to `pLink->m_pNext` into the third instruction's %ecx (same
//     address, nothing between), but it could not forward pLink->m_pPrev
//     across `mov %eax,(%ecx)`, which may alias. d3dr3a.cpp:31 and
//     d3dr3d.cpp:294 record the identical fingerprint in d3d.ren.
//
//  The call site pins the caller's shape too: at 0x10026edc..0x10026ee5 the
//  manager does `mov %esi,0x8(%esi)` (m_pData = the client), `lea
//  0x7e8(%edi),%ecx` (the head), `call 0x100270be`, `incl 0x7e4(%edi)`.
//  That is dl_AddAfter's body — m_pData, insert, ++m_nElements — with
//  LTList laid out {m_nElements @0x7e4, m_Head @0x7e8}, exactly ltlink.h's
//  member order. So the caller is `dl_AddAfter(&pMgr->m_List, &pMgr->
//  m_List.m_Head, &pClient->m_Link, pClient)` (or dl_AddHead, which is the
//  same three statements).
//
//  WHY THERE IS A COMDAT AT ALL. /O1 is /Os, and /Os declined to inline the
//  seven stores; d3d.ren's /O2 build inlined the same header text. That is
//  the /Ob1-budget difference, and it is the reason this
//  address exists only in server.dll.
//
//  NOT WRITTEN HERE ON PURPOSE: re-spelling AddAfter in this file would
//  create a second definition of a header function every server TU already
//  sees. The row closes by making one server TU call it out of line, which
//  the smgr.cpp caller at 0x10026e?? already will once that function is
//  reconstructed. Recorded as evidence, not as a body.
// ======================================================================= //


// ======================================================================= //
//  2.  The CObjBase leaves.  UNVERIFIABLE (3, 5 and 7 bytes).
//      BELONGS IN: src/engine/server/objtypes.cpp, with the declarations
//      going into objtypes.h beside the ones it already carries.
//
//  objtypes.h declares CObjBase's fourteen virtuals and leaves four of them
//  ("_v10", "_v14", "_v2c") undefined. These are three of the four, and the
//  vtables name them without ambiguity — every CObjBase-derived table in
//  the image holds them at the same three offsets:
//
//    +0x10  0x1000989c   `33 c0 c3`        xor %eax,%eax; ret
//    +0x14  0x10009897   `33 c0 c2 04 00`  xor %eax,%eax; ret $4
//    +0x2c  0x10024689   `d9 81 a4 01 00 00 c3`  flds 0x1a4(%ecx); ret
//
//  Checked at 0x1004e174+0x10/0x14/0x2c (CObjLineSystem), 0x1004e1ac+…,
//  0x1004e1e8+… (CObjCanvas), and six more; 0x1000989c also fills +0x34.
//  0x1004d354 — the eight-slot table svnew.cpp calls CGeomView's, whose
//  slot 3 is that file's GetDims — carries the same two at +0x10 and +0x14,
//  which is independent confirmation that CGeomView and CObjBase share a
//  root and that svnew.cpp's MOVE-ME note is about this hierarchy.
//
//  The field 0x2c returns is ltobject_internal.h's `float m_Radius; //
//  0x1a4`, the bounding-sphere radius. objtypes.h's CObjBase view stops at
//  0x1a8 and does not name it; the fold should add it.
//
//  ARGUMENT COUNTS ARE FROM THE RETURNS, not guessed: `ret` is no
//  arguments, `ret $4` is one dword. Return types are NOT pinned — every
//  32-bit integer type gives `xor %eax,%eax` — so LTBOOL/LTRESULT is a free
//  choice; LTRESULT is spelled here because LT_OK is 0 and every other
//  CObjBase stub in objtypes.h that returns a status returns LTRESULT.
//  COIN FLIP, no byte consequence: `LTRESULT _v10() { return LT_OK; }`
//  against `LTBOOL _v10() { return LTFALSE; }`.
// ======================================================================= //

// FOLDED: objtypes.h now gives CObjBase slots 4 and 5 (and 13) their inline
// bodies and objtypes.cpp defines _v2c, so the vtable the banks emit names
// real definitions; the view that stood here is gone.


// ======================================================================= //
//  3.  0x100256c0, 7 bytes.  UNVERIFIABLE.
//      BELONGS IN: src/engine/server/objtypes.cpp, as CObjCanvas.
//
//      d9 81 b0 01 00 00 c3   flds 0x1b0(%ecx); ret
//
//  The field is objtypes.h's `float m_fUnknown1b0; // 0x1b0` on CObjCanvas
//  — the draw distance its constructor sets to one. The constructor two
//  bytes above (0x1002569x) is the tie: it zeroes 0x1a8 and 0x1ac, stores
//  `fld1` into 0x1b0 and installs vftable 0x1004e1e8, and 0x1004e1e8+0x2c
//  is this address.
//
//  ONE THING IS NOT SETTLED AND IS FLAGGED RATHER THAN DECIDED. The
//  0x1004e1e8 vtable is FIFTEEN slots (it runs to 0x1004e220; 0x1004e224 is
//  already the float pool, 250.0f), and 0x100256c0 fills BOTH +0x2c and
//  +0x38. objtypes.h says "the base is fourteen virtuals; only a light adds
//  a fifteenth" — CObjCanvas adds one too, and its fifteenth has the same
//  body as its override of the base's _v2c. Two identical COMDATs folded by
//  the linker would explain it, but this image does NOT fold identical
//  COMDATs (0x1000989c and 0x10014d2f are both `33 c0 c3` and both survive),
//  so the more likely reading is one function reached from two slots. Left
//  open; it does not change the byte content of the body.
// ======================================================================= //

// FOLDED: objtypes.h/objtypes.cpp now give CObjCanvas this override and the
// fifteenth virtual (+0x38) with the same body, which lithtech.exe's server
// half also builds (0x0045bb00 there); the view that stood here is gone.
// The open question above is settled the other way: /OPT:ICF does fold
// here (CObjBase's _v10 and _v34 share one address in both images), so two
// virtuals with one body is the reading, not one function in two slots.


// ======================================================================= //
//  4.  0x10014d2f, 3 bytes.  UNVERIFIABLE.
//      BELONGS IN: src/engine/server/servermessage.cpp, as the LMessageImpl
//      method one slot past GetStatus; the declaration goes in servermsg.h
//      at line 177, after `LTRESULT GetStatus(uint32& flags);`.
//
//      33 c0 c3   xor %eax,%eax; ret
//
//  WHAT PINS IT. It is slot 39 of the 121-slot table at 0x1004de00, whose
//  slot 5 begins the ILTMessage block servermsg.h already maps: slot 30 is
//  WriteRaw (0x10014d1a), 36 WriteObject (0x10014a9e), 37 ResetPos
//  (0x10014d32), 38 GetStatus (0x10014d3e). 39 is the next one, and
//  src/LT2/sdk/inc/iltmessage.h ENDS at GetStatus — which is the SDK-era
//  mismatch SERVER-NOTES.md warns about: the shipped NOLF1 interface has
//  one method the NOLF2 header dropped. It also appears at 0x1004d39c and
//  0x1004d428, two more implementations of the same interface.
//
//  `ret` with no immediate is no arguments; `xor %eax,%eax` is LT_OK. So
//  the shipped body is a stub that answers LT_OK and does nothing — which
//  is why it is three bytes and why it was never folded with the identical
//  0x1000989c (this image does not fold identical COMDATs).
//
//  The method's NAME is not recoverable from the image: nothing calls it,
//  nothing reports it, and the header that would have named it is a
//  generation too late. Named by slot.
// ======================================================================= //

class CLMessageTinyView
{
public:
	virtual LTRESULT _slot39();              // 0x10014d2f
};

LTRESULT CLMessageTinyView::_slot39()
{
	return LT_OK;
}


// ======================================================================= //
//  5.  0x100360f8, 4 bytes.  UNVERIFIABLE.
//      BELONGS IN: src/engine/server/servermessage.cpp, on the class whose
//      vftable is 0x1004e900 — servermessage.cpp:1822 already names that
//      table CMessageObjectHelper's and its constructor 0x10034fb9.
//
//      6a 1c 58 c3   push $0x1c; pop %eax; ret
//
//  Slot 45 (+0xb4) of 0x1004e900. `push imm8; pop %eax` for a constant is
//  /Os choosing four bytes over the five `mov $0x1c,%eax` would cost — the
//  same trade svnew.cpp records at 0x10024cad (`push $1; pop %eax; ret`).
//  `ret` with no immediate: no arguments. So the whole body is a constant
//  28 handed back through a virtual.
//
//  WHAT 28 IS is not settled. It is not a field read and not a size the
//  banks use. Written as a literal rather than given a name it has not
//  earned.
// ======================================================================= //

class CMessageHelperTinyView
{
public:
	virtual int _vb4();                      // 0x100360f8
};

int CMessageHelperTinyView::_vb4()
{
	return 28;
}


// ======================================================================= //
//  6.  0x1002712c, 9 bytes.  VERIFIABLE.
//      BELONGS IN: src/engine/server/servermessage.cpp, beside
//      ph_GetUnknown74 / ph_SetUnknown74 — that file's comment at line 1841
//      already identifies its sibling 0x10027110 as "one of the three
//      function pointers the holder's descriptor carries".
//
//      8b 4c 24 08   mov 0x8(%esp),%ecx
//      8b 01         mov (%ecx),%eax
//      ff 60 04      jmp *0x4(%eax)
//
//  WHAT PINS IT. The `setup` tag is real, not a misread: 0x10026eb0 is
//  `c7 45 dc 2c 71 02 10` — `movl $0x1002712c,-0x24(%ebp)` — the second of
//  a five-dword descriptor the caller builds on its frame at -0x28(%ebp)
//  and hands to 0x1000e507:
//
//      -0x28  0x10027110    (servermessage.cpp's forwarder to ph_GetUnknown74)
//      -0x24  0x1002712c    (this)
//      -0x20  0x1003a438    (CRT)
//      -0x1c  %edi + 0x6e0
//      -0x18  the caller's 0xc(%ebp)
//
//  CALLING CONVENTION, and it is forced. 0x10027110 ends `add $0xc,%esp;
//  ret` — __cdecl, two dwords — so its table-mate is __cdecl and two dwords
//  too. This one takes the SECOND dword as `this` and tail-jumps slot 1 of
//  its vtable. A tail `jmp` only balances if the callee pops nothing the
//  caller was not going to pop itself, so the virtual it reaches takes NO
//  arguments (`ret`, not `ret $4`) and the first parameter is unused here.
//  That is the one non-obvious claim in this body and it is arithmetic, not
//  taste: a `ret $4` callee would leave the __cdecl caller's `add $8` four
//  bytes over.
//
//  Return type has no byte consequence — the virtual's %eax falls through
//  either way.
// ======================================================================= //

// MOVED to smgr.cpp (beside ph_AcceptAlways, its lithtech.exe neighbour at
// 0x00463d20): lithtech.exe has this body at 0x00463d10 and svtiny.cpp is
// not in its build.


// ======================================================================= //
//  7.  0x10010e03, 16 bytes.  VERIFIABLE.
//      BELONGS IN: src/engine/server/saveio.cpp or servermsg.cpp — the two
//      TUs whose COMDATs bracket it — and the class itself is not yet in
//      the tree under any name. See the OPEN QUESTION below.
//
//      83 61 2c 00   andl $0x0,0x2c(%ecx)
//      57            push %edi
//      8d 79 34      lea  0x34(%ecx),%edi
//      33 c0         xor  %eax,%eax
//      ab            stos %eax,%es:(%edi)
//      ab            stos %eax,%es:(%edi)
//      8b c1         mov  %ecx,%eax
//      5f            pop  %edi
//      c3            ret
//
//  IT IS A CONSTRUCTOR, and the call sites say so twice over.
//   * 0x1002ce15 is the factory: `add $0x4,%ecx; push %ecx; call sb_Allocate
//     (0x10001c86); mov %eax,%esi; test %esi,%esi; je skip; mov %esi,%ecx;
//     call 0x10010e03; skip: mov %esi,%eax; ret` — allocate, then construct
//     only if the allocation succeeded, then hand the pointer back.
//   * 0x100106de constructs it on the frame of sav_RestoreObject
//     (0x100106b6, saveio.cpp line 615) at -0x90(%ebp), alongside a
//     CString::Clear at -0x524 (0x10001bd7) and a CObjBase at -0x2a0
//     (0x1002465b).
//  `mov %ecx,%eax` before the epilogue is the constructor returning `this`.
//  There is no vptr store and no base-constructor call, so the class has no
//  virtuals and whatever precedes 0x2c is trivially constructed.
//
//  ITS SIZE IS 0x54 BYTES, from the pool the factory draws on: 0x1002ce31
//  initialises that same bank with `push $0x20; push $0x54; push %esi; call
//  sb_Init (0x1003d170)` — 84-byte elements, 32 to a block.
//
//  THE ONE COIN FLIP is the pair at 0x34/0x38. Two `andl $0x0,disp(%ecx)`
//  would be eight bytes against the nine `push %edi / lea / xor / stos /
//  stos / pop %edi` costs, so /Os did NOT choose the string op for size —
//  it is an inlined constant memset, the same expansion as
//  "new T[n] for a memset-constructed POD". Both spellings, exactly:
//
//      A (written below)  memset(m_a34, 0, sizeof(m_a34));
//      B                  memset(&m_n34, 0, 8);
//
//  A and B differ only in how the eight bytes are declared, not in output.
//  What is NOT interchangeable, and is the actual risk on this row, is
//  writing the pair as two assignments (`m_n34 = 0; m_n38 = 0;`), which
//  gives two `andl` and loses the %edi save entirely.
//
//  OPEN QUESTION — THE CLASS. Its identity is not settled. It is 0x54
//  bytes, pool-allocated by the manager whose vtable region is 0x1004e24c,
//  and also stack-constructed by saveio.cpp's per-object restore pass. The
//  view below is named by address on purpose; DO NOT invent a name for it
//  when folding.
// ======================================================================= //

class CSub10010E03View
{
public:
	CSub10010E03View();                      // 0x10010e03

	uint8  m_pad_00[0x2c];
	void*  m_p2c;                            // 0x2c
	uint32 m_n30;                            // 0x30  untouched by the ctor
	uint32 m_a34[2];                         // 0x34
	uint8  m_pad_3c[0x54 - 0x3c];
};

CSub10010E03View::CSub10010E03View()
{
	m_p2c = LTNULL;
	memset(m_a34, 0, sizeof(m_a34));
}
