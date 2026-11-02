// *********************************************************************** //
//
//  MODULE:   D3DHIGH (.CPP) -- scratch, NOT part of the repo yet
//
//  PURPOSE:  Decodes of the twelve rows on the "tier A" listing between
//            0x10050410 and 0x1007f77b.
//
//  READ THIS BEFORE BUILDING ANY OF IT.
//
//  ----------------------------------------------------------------------
//  THE LISTING IS CONTAMINATED. ELEVEN OF THE TWELVE CARRY THE UNKNOWN
//  COMPILER AFTER ALL, AND THE SCREEN MISSED THEM BY A BYTE.
//  ----------------------------------------------------------------------
//
//  The tier filter rejects a body containing these exact byte strings:
//
//      55 8b ec           frame pointer
//      33 c0 40           xor/inc for `return 1`
//      83 c4 04           the dummy 4-byte stack slot
//      89 04 24           call argument STORED to (%esp)
//      c7 04 24           call argument constant STORED to (%esp)
//      8d 76 00 ...       GNU-assembler padding
//      83 c4 f8/f4/ec     `add $-8,%esp` where cl writes `sub`
//
//  Every one of those is a LITERAL, and the same idiom in a different
//  encoding walks straight through. What is on this listing:
//
//    * `89 44 24 04` / `89 4c 24 08` / `89 14 24` / `c7 44 24 0c` --
//      accumulate-outgoing-args with a NON-ZERO displacement, or with a
//      register other than %eax. Same fingerprint as `89 04 24`; the
//      pattern list only holds the zero-displacement %eax spelling.
//      Five functions: 0x10050410, 0x1005a050, 0x1005a0b0, 0x1007c440,
//      0x1007f560. cl 6 ALWAYS pushes. None of these can reproduce.
//
//    * `83 ec 08` ... `83 c4 08` -- the dummy stack slot in its 8-byte
//      form. 0x10056aa0 is eleven bytes where cl needs four.
//
//    * `89 7c 24 xx` / `89 74 24 xx` -- callee-saved registers written
//      into the frame with `mov` instead of `push`, in functions that
//      then NEVER USE THEM (0x100598a0 saves %esi and %edi and touches
//      neither). cl pushes, and does not save a register it does not use.
//
//    * argument arriving in %ecx as a `this` that the body reads with
//      `8b 41 04` -- the reg-arg screen only tests `8b 01 / 08 / 10 / c1`.
//
//    * a 12-byte `mov N(%esp),reg` shim in front of the body
//      (0x10079f30 -> 0x10079f3c) -- the shim detector skips the SHIM row,
//      but the BODY row survives and is what got listed.
//
//  So the honest tally for this batch is:
//
//    0x10050e40   the ONLY row with no fingerprint at all. Even so its
//                 register allocation is not forced -- see its note.
//    0x1007f77b   NOT A FUNCTION. The real entry is 0x1007f770 and it is
//                 `55 8b ec`. Excluded, not decoded.
//    everything   decoded below, correct as source, UNVERIFIABLE until the
//    else         compiler is known. Do not spend build cycles on them.
//
//  ----------------------------------------------------------------------
//  ONE PIECE OF NEW EVIDENCE ABOUT THE COMPILER, found on the way.
//  ----------------------------------------------------------------------
//
//  .data has virtual size 0x1a520 but only 0x2000 bytes of raw data, so
//  everything at or above 0x1008b000 in this image is BSS. (A dump that
//  does not know that annotates BSS addresses with whatever string
//  happens to sit at the same file offset inside .data1 -- that is why
//  0x1008be60 shows up below as "Model::SetupTransforms failed for %s.".
//  It is not a string; it is a zero-initialised byte.)
//
//  Inside the raw part, at 0x10089ac0, sits
//
//      "@(#)floor.c\t1.4.7.1 08/22/98 14:05:29"
//
//  -- an SCCS what(1) ident from Sun's fdlibm. Microsoft's CRT does not
//  carry one and never has. Whatever built d3d.ren linked a UNIX-derived
//  libm, which is a much stronger statement than "not cl": it says the
//  toolchain was a port of a UNIX toolchain, which fits the GNU-assembler
//  padding and the accumulate-outgoing-args ABI exactly. Worth grepping
//  the rest of .rdata/.data for more `@(#)` idents -- each one names a
//  source file of the runtime, and together they would identify it.
//
// *********************************************************************** //

#include "ltbasedefs.h"


// ---------------------------------------------------------------------- //
//  0x10050e40 -- THE ONE ROW THAT IS ACTUALLY CLEAN.
//
//  Thirty-two bytes, cdecl, three stack arguments, no frame, no dummy
//  slot, no mov-arg, no reg-arg, one honest `push %ebx`.
//
//  Reads one dword out of the object at +0x44, hands it back whole through
//  the THIRD pointer, and hands (v >> 2) & 0x1f back through the SECOND.
//  Five bits starting at bit 2 -- a bitfield accessor, and the raw dword is
//  returned alongside it.
//
//  It has an immediate sibling at 0x10050e10 (the previous vtable slot)
//  which is byte-for-byte the same shape except that its dword comes from
//  p->m_p28->m_n30 rather than p->m_n44, and which allocates ebx/ecx/edx
//  differently -- so the pair is one source function written twice.
//
//  WHAT IS NOT PINNED, and it is the reason this one is uncertain:
//  the shipped code holds FOUR values live at once (v, the extracted
//  field, and BOTH out-pointers) and therefore has to spend %ebx. cl /O2
//  loads an out-pointer immediately before the store it feeds, so it needs
//  only three registers, emits no `push %ebx`, and comes out shorter. The
//  store ORDER below (third pointer, then second) is read off the bytes and
//  is the best chance of forcing the overlap; if cl still declines to keep
//  both live, no rewriting of this source will change that and the row
//  should be dropped rather than chased.
// ---------------------------------------------------------------------- //

struct Sub10050e40Object
{
	uint8   m_Pad00[0x44];                              // +0x00
	uint32  m_n44;                                      // +0x44
};

void sub_10050e40(Sub10050e40Object* pObject, uint32* pOutField,
                  uint32* pOutRaw)
{
	uint32 nValue = pObject->m_n44;

	*pOutRaw   = nValue;
	*pOutField = (nValue >> 2) & 0x1f;
}


// ---------------------------------------------------------------------- //
//  0x1007f77b -- NOT A FUNCTION, and nothing is written for it.
//
//  The enumeration split this row five bytes inside the real body. The
//  bytes at 0x1007f77b decode as `add $0x24,%al`, which is the tail of the
//  `mov %eax,(%esp)` at 0x1007f77a. The function begins at 0x1007f770:
//
//      55 8b ec           push %ebp; mov %esp,%ebp
//      83 ec 10           sub  $0x10,%esp
//      57                 push %edi                  (the dummy slot)
//      8b 45 08           mov  0x8(%ebp),%eax
//      89 04 24           mov  %eax,(%esp)           (mov-arg)
//      8b c1 / 8b c8      mov  %ecx,%eax; mov %eax,%ecx   (a no-op pair)
//      e8 ...             call 0x1007fa20
//      8b e5 5d c2 04 00  leave-equivalent; ret $4
//
//  -- frame, dummy slot AND mov-arg, plus a redundant %ecx round trip that
//  no optimiser would leave in. It is a one-line forwarding wrapper
//  (`{ Impl(a); }`) and there are four of them in a row at 0x1007f730,
//  0x1007f750, 0x1007f770 and 0x1007f790. Not worth a build cycle.
// ---------------------------------------------------------------------- //


// ---------------------------------------------------------------------- //
//  0x1001c480 / 0x10050410 / 0x1005a0b0 / 0x1007c440 / 0x1007f560
//
//  FIVE COPIES OF ONE FUNCTION. Every one of them is vtable slot 10
//  (offset +0x28) of a twelve-slot table, and the five tables live at
//  0x10089360, 0x100896b0(+0x100896e0), 0x100897c0, 0x100899f0 and
//  0x10089a80. The bodies are IDENTICAL except for one immediate: the
//  element size passed as the fourth argument, which is 0x40, 0x0c, 0x10,
//  0x04 and 0x04 respectively. That is a template instantiated five times
//  -- `sizeof(T)` is the only thing that varies.
//
//      __thiscall, two stack arguments, `ret $0x8`:
//
//      long n = sub_1007fc50(pKey, this->m_pArray, this->m_nCount,
//                            sizeof(T));
//      *pOutIndex = n;
//      return n != -1;
//
//  The `!= -1` is a COMPARISON, not a cast: `33 d2 / 83 f8 ff / 0f 95 c2`
//  -- zero %edx, compare, `setne %dl`. The index is stored to the caller's
//  out-parameter WHETHER OR NOT the search succeeded (-1 lands there on
//  failure), which is read off the unconditional `mov %eax,(%ecx)`.
//
//  ABOUT 0x1007fc50: it is above the 0x1007fa10 CRT line but it is NOT a
//  Microsoft CRT routine -- it is a linear `repz cmpsb` search returning an
//  index, or -1. It is left as an extern and NOT reconstructed. One thing
//  to flag while it is on the record: the routine loads its scan cursor
//  from `0x4(%esp)` (its FIRST argument, the key) and its comparison
//  operand from `0x14(%esp)` after four pushes, which is that same first
//  argument -- so it compares the key against itself on iteration zero and
//  never reads its second argument (the array base) at all. Read four
//  times against the raw bytes; if it is what it looks like, every one of
//  these five Find() calls returns 0 for any non-empty table. It is not
//  fixed here, but do not "correct" it when transcribing.
//
//  All five are blocked on accumulate-outgoing-args:
//      89 44 24 04 / 89 4c 24 08 / 89 14 24 / c7 44 24 0c NN 00 00 00
//  cl emits four `push`es for this and cannot be talked out of it.
//
//  Written as member functions because the `this` really does arrive in
//  %ecx; there is no way to spell __thiscall on a free function in cl 6.
// ---------------------------------------------------------------------- //

// 0x1007fc50 -- (key, base, count, width) -> index, or -1.
extern "C" long sub_1007fc50(const void* pKey, const void* pBase,
                             uint32 nCount, uint32 nWidth);

struct IndexedTableBase
{
	void*   m_pVtbl;                                    // +0x00
	void*   m_pArray;                                   // +0x04
	uint32  m_nCount;                                   // +0x08
};

// 0x10050410 -- element size 0x0c.
struct Table0c : public IndexedTableBase
{
	LTBOOL sub_10050410(const void* pKey, long* pOutIndex);
};

LTBOOL Table0c::sub_10050410(const void* pKey, long* pOutIndex)
{
	long nIndex = sub_1007fc50(pKey, m_pArray, m_nCount, 0x0c);

	*pOutIndex = nIndex;

	return nIndex != -1;
}

// 0x1005a0b0 -- element size 0x10. Referenced from TWO vtables
// (0x100897c8 and 0x10089ab8), so the two classes share one instantiation.
struct Table10 : public IndexedTableBase
{
	LTBOOL sub_1005a0b0(const void* pKey, long* pOutIndex);
};

LTBOOL Table10::sub_1005a0b0(const void* pKey, long* pOutIndex)
{
	long nIndex = sub_1007fc50(pKey, m_pArray, m_nCount, 0x10);

	*pOutIndex = nIndex;

	return nIndex != -1;
}

// 0x1007c440 -- element size 0x04.
struct Table04a : public IndexedTableBase
{
	LTBOOL sub_1007c440(const void* pKey, long* pOutIndex);
};

LTBOOL Table04a::sub_1007c440(const void* pKey, long* pOutIndex)
{
	long nIndex = sub_1007fc50(pKey, m_pArray, m_nCount, 0x04);

	*pOutIndex = nIndex;

	return nIndex != -1;
}

// 0x1007f560 -- element size 0x04, a second instantiation with the same
// size as 0x1007c440. Two identical bodies at two addresses, so the two
// T's are distinct types that happen to be four bytes wide.
struct Table04b : public IndexedTableBase
{
	LTBOOL sub_1007f560(const void* pKey, long* pOutIndex);
};

LTBOOL Table04b::sub_1007f560(const void* pKey, long* pOutIndex)
{
	long nIndex = sub_1007fc50(pKey, m_pArray, m_nCount, 0x04);

	*pOutIndex = nIndex;

	return nIndex != -1;
}


// ---------------------------------------------------------------------- //
//  0x10056aa0 -- eleven bytes: `sub $8; xor %eax,%eax; add $8; ret $4`.
//
//  A slot that always refuses, reached from TWO vtables (0x10089768 and
//  0x10089778 both point at it). One stack argument, stdcall, answer zero.
//  The argument is never touched, so its type is unrecoverable; the
//  companion at 0x10059870 in the same family is the `return 1` twin
//  (`56 / 33 c0 40 / 83 c4 04 / c2 0c 00`).
//
//  BLOCKED: the eight-byte dummy stack slot. As a __stdcall free function
//  icl emits `33 c0 c2 04 00` (no frame); as a __thiscall member, as here,
//  it emits the four-byte `push %esi ... add $0x4,%esp` slot every other
//  `this`-ignoring member in d3d.ren has -- never `sub $0x8`. MEASURED and
//  all still four bytes (or a real frame): return types bool/char/short/
//  __int64/float; argument types float/char/short/4-byte struct/reference;
//  virtual, const, a member of a virtual-base class; dead locals (double,
//  __int64, 8-byte array, 8-byte struct, address-taken via an inline);
//  dead calls (`if (0) f(a, b)`, code after the return, a same-TU helper
//  inlined by /Qip); and the flags /O1 /O2 /Ox /Os /Qip /G5 /G6 /Op /Qrcd
//  /Qpc32 /Qpc64 /Qsfalign8 /Qsfalign16 /QxM /Gs. The only other `sub $0x8
//  ; add $0x8` stub in the image (0x10003ad0, void) is unmatched too.
//
//  SOLVED: it is __fastcall.  icl gives every REGISTER parameter a dead
//  four-byte slot -- the thiscall `push %esi ... add $0x4` is `this`'s --
//  and __fastcall has two, %ecx and %edx: `sub $0x8`.  `ret $0x4` is the
//  third argument, on the stack.  Its two table-mates are the witness:
//  0x10050e60 / 0x100525b0 (the slots before it at 0x10089760) and
//  0x10053e20 / 0x100553d0 (at 0x10089770) all open by saving %ecx and
//  %edx and reading 0x8(%ebp) -- a table of three-argument __fastcall
//  callbacks, of which this is the one that refuses.  IDENTICAL.
// ---------------------------------------------------------------------- //

LTBOOL __fastcall sub_10056aa0(void* pArg1, void* pArg2, void* pArg3)
{
	return LTFALSE;
}


// ---------------------------------------------------------------------- //
//  0x100598a0 -- three colour channels through three 256-byte ramps, plus
//  a fourth byte from a global.
//
//  stdcall, `ret $8`, two pointers. Source object supplies three adjacent
//  BYTES at +0x14, +0x15 and +0x16; each indexes its own table, and the
//  results land in the destination's four-byte field at +0x10 in the order
//
//      src +0x16 -> ramp @0x1008d1bc -> dst +0x12
//      src +0x15 -> ramp @0x1008d2bc -> dst +0x11
//      src +0x14 -> ramp @0x1008d3bc -> dst +0x10
//      byte @0x1008be60             -> dst +0x13
//
//  -- IN THAT ORDER, highest destination byte first, which is what fixes
//  the statement order below. The three ramp bases are exactly 0x100 apart,
//  which is what makes them 256-entry byte tables rather than three
//  unrelated globals; all four addresses are in BSS (see the header note
//  about .data's raw size, and ignore a dump's string annotations here).
//
//  The immediate sibling at 0x10059880 writes 0xffffffff into the SAME
//  dst+0x10 field, which is what identifies it as a packed 32-bit colour:
//  +0x10/+0x11/+0x12 are the three channels and +0x13 is alpha, and the
//  sibling is the "opaque white" case of this one.
//
//  THE FRAME: `89 7c 24 04` and `89 74 24 08` save %edi and %esi into the
//  frame this function opens with `83 ec 0c`, and then NEITHER IS USED.
//  That is not a blocker: it is what icl emits for a __THISCALL member
//  whose body clobbers %ecx (here the table index), measured -- the same
//  statements as a __stdcall free function open no frame at all, and as a
//  member with `this` never read they come out byte-identical, frame and
//  dead saves included. So it is `ret $8` over two stack arguments with
//  `this` in %ecx, not a two-argument stdcall. The owning class is not
//  recoverable from these bytes; it is named for the address.
// ---------------------------------------------------------------------- //

// 0x1008d1bc, 0x1008d2bc, 0x1008d3bc -- three 256-entry byte ramps in BSS.
extern uint8 g_ColorRamp2[256];                         // 0x1008d1bc
extern uint8 g_ColorRamp1[256];                         // 0x1008d2bc
extern uint8 g_ColorRamp0[256];                         // 0x1008d3bc

// 0x1008be60 -- a single BSS byte, copied straight into the alpha slot.
extern uint8 g_nByte1008be60;

struct ColorSource
{
	uint8   m_Pad00[0x14];                              // +0x00
	uint8   m_n14;                                      // +0x14
	uint8   m_n15;                                      // +0x15
	uint8   m_n16;                                      // +0x16
};

struct ColorDest
{
	uint8   m_Pad00[0x10];                              // +0x00
	uint8   m_n10;                                      // +0x10
	uint8   m_n11;                                      // +0x11
	uint8   m_n12;                                      // +0x12
	uint8   m_n13;                                      // +0x13
};



// ---------------------------------------------------------------------- //
//  0x1005a050 -- forward two adjacent members of an array element into a
//  helper as OUT parameters.
//
//  __thiscall, two stack arguments, `ret $0x8`. The index arithmetic is
//  `lea 0x770(%edi,%eax,4)` with %edi already holding nIndex*4, so the
//  stride is EIGHT and the array of pairs starts at this+0x770; the second
//  lea is the same address plus four. Both are handed to 0x10039f90 as the
//  third and fourth arguments.
//
//  0x10039f90 itself is `if (pObject->m_p0c) { *pOutA = m_p0c->m_n04;
//  *pOutB = m_p0c->m_n08; return 1; } else <register-convention tail call>`
//  -- so this is a two-dword getter and the pair at this+0x770+n*8 is where
//  the answer is cached.
//
//  BLOCKED: mov-arg (four stores into the outgoing area), plus %esi and
//  %edi saved into the frame with `mov`.
// ---------------------------------------------------------------------- //

// 0x10039f90 -- fills two out-dwords from pObject->m_p0c; returns 1 on the
// fast path.
extern uint32 sub_10039f90(void* pObject, uint32 nIndex, uint32* pOutA,
                           uint32* pOutB);

struct Sub1005a050Arg
{
	uint8   m_Pad00[0x28];                              // +0x00
	uint8*  m_p28;                                      // +0x28
};

struct CachedPair
{
	uint32  m_nA;                                       // +0x00
	uint32  m_nB;                                       // +0x04
};

struct Sub1005a050Owner
{
	uint8       m_Pad000[0x770];                        // +0x000
	CachedPair  m_Pairs[1];                             // +0x770
	uint32 sub_1005a050(Sub1005a050Arg* pArg, uint32 nIndex);
};

uint32 Sub1005a050Owner::sub_1005a050(Sub1005a050Arg* pArg, uint32 nIndex)
{
	return sub_10039f90(*(void**)(pArg->m_p28 + 0x30), nIndex,
	                    &m_Pairs[nIndex].m_nA,
	                    &m_Pairs[nIndex].m_nB);
}


// ---------------------------------------------------------------------- //
//  0x10079c70 / 0x10079d70 (the unpack-one-pixel template, uint8 and uint16
//  sources) and 0x10079f30 (the image-size helper) -- MOVED to the pixel
//  converter family's TU, w29_10073c00_osoiqip.cpp.  0x10079f30
//  is a /Qip two-entry pair (shim 0x10079f30, register entry 0x10079f3c)
//  whose caller 0x10073cc7 (the copier) enters it by register, so it has to
//  share the copier's /Qip TU; the unpacks moved with it, the family being
//  one shipped module 0x10073c00..0x10079f84 (identical in both places).
// ---------------------------------------------------------------------- //


class C100598a0
{
public:
	void sub_100598a0(const ColorSource* pSrc, ColorDest* pDest);
};

void C100598a0::sub_100598a0(const ColorSource* pSrc, ColorDest* pDest)
{
	pDest->m_n12 = g_ColorRamp2[pSrc->m_n16];
	pDest->m_n11 = g_ColorRamp1[pSrc->m_n15];
	pDest->m_n10 = g_ColorRamp0[pSrc->m_n14];
	pDest->m_n13 = g_nByte1008be60;
}
