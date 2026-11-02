// *********************************************************************** //
//
//  MODULE:   LOW (.CPP)  -- d3d.ren 0x1000d9b0 .. 0x1001e140, "low"
//
//  PURPOSE:  Twelve functions handed over as tier A -- "no fingerprint of the
//            unknown compiler, so cl should reproduce them byte-for-byte".
//
//  READ THIS BEFORE SPENDING A BUILD CYCLE ON ANY OF THEM.
//
//  ============ THE HEADLINE: THE SCREEN THAT PRODUCED THIS LIST IS ============
//  ============ TOO SHALLOW, AND NONE OF THE TWELVE CAN REPRODUCE.  ============
//
//  The tier filter rejects a body on eleven literal byte strings. Three of
//  its tests are much narrower than the property they stand for, and every
//  row in this listing slips through one of the gaps:
//
//   1. "call arguments STORED into the outgoing area" is tested as the two
//      strings `89 04 24` and `c7 04 24` -- i.e. ONLY a store of %eax, and
//      ONLY to (%esp) with no displacement. The unknown compiler writes the
//      other outgoing slots too, and it uses whichever register is free:
//        1000ef7f  89 0c 24        mov %ecx,(%esp)          <- %ecx, missed
//        1001c495  89 14 24        mov %edx,(%esp)          <- %edx, missed
//        1001c48a  89 44 24 04     mov %eax,0x4(%esp)       <- disp, missed
//        1000ef86  c7 44 24 08 ..  movl $0,0x8(%esp)        <- disp, missed
//      d3dneutral.cpp's own scan already lists this as reject rule (h) and
//      states it correctly; the filter's byte version does not implement it.
//
//   2. "an argument arrived in a register" is tested ONLY on the body's first
//      two bytes. Nine of these twelve take `this` in %ecx and never load it,
//      but the %ecx read is the second or third instruction, so the test
//      never sees it. 0x10015e30 goes further and takes BOTH arguments in
//      %eax and %edx with no stack arguments at all -- a convention cl has no
//      spelling for.
//
//   3. Not tested at ALL: the entry `sub $N,%esp` that exists only to keep
//      %esp 16-byte aligned across a call, unwound as a mismatched
//      `add $M,%esp` just before the pushes. cl never emits it -- it has no
//      stack-alignment policy in 1998 -- and it is unspellable in C. Five of
//      the twelve open with one:
//        1000d9b0  sub $0xc,%esp ... add $0x8,%esp; push; push; call
//        100157e0  sub $0x8,%esp ... push; push; call; add $0x10,%esp
//        10019a24  sub $0xc,%esp ... add $0x8,%esp; push; push; call *%edx
//      The function has no locals in any of these cases; the space is pure
//      alignment. (The `add $-8,%esp` ENCODING of the same idea is on the
//      fingerprint list as "add-neg-esp"; the `sub` encoding is not, and the
//      unknown compiler picks `sub` whenever the constant is positive.)
//
//  Applying d3dneutral.cpp's shape rules (a)..(h), which are the correct
//  statement of the same idea, kills all twelve. Verdict per function is at
//  the head of each block below and every one of them is "will NOT reproduce".
//  My recommendation is to measure ONE of them -- 0x100157e0, the smallest
//  and the only one whose sole defect is the alignment pair -- to confirm the
//  above on the real cl, and skip the other eleven.
//
//  THIRTEENTH ROW, DELETED. The listing's 0x1001dc8e (18 bytes) is not a
//  function. It is the interior of 0x1001dc70 -- it lands in the middle of
//  `movl $0x10091df0,0x8(%esp)` (c7 44 24 08 f0 1d 09 10), four bytes past
//  that instruction's start, and 0x1001dc70 is framed AND dummy-slotted AND
//  mov-arg. Nothing is emitted for it here.
//
//  ============================ WHAT IS HERE ==============================
//
//  Every remaining body was read instruction by instruction and is DECODED,
//  not inferred: the operand of every load and store, the shape of every
//  call, and the real `ret` (four of the listed sizes include trailing
//  padding -- corrected sizes are stated per function). The decodes are
//  compiler-independent and become testable the day the compiler is found.
//  Where a name could not be earned from the bytes, the function and the
//  global are named after their address, which is what the harness maps.
//
// *********************************************************************** //

#include "ltbasedefs.h"
#include "ltmatrix.h"


// ----------------------------------------------------------------------- //
//  Neighbours these bodies reach, decoded far enough to type this file.
// ----------------------------------------------------------------------- //

// 0x1000a770 -- transform a 3-vector in place by a 3x4 row-major matrix.
// Reads the vector at +0x00/+0x04/+0x08 and the matrix rows at +0x00.. ,
// +0x10.. and +0x20.. with the translation at +0x0c/+0x1c/+0x2c, and stores
// the three results back over the vector (fstps 0x8, 0x0, 0x4). Returns void.
void sub_1000a770(LTVector* pVec, const float* pMat34);

// 0x1006be50 -- framed, 0x11e0 bytes of stack, three arguments; walks its
// SECOND argument's +0x44. Not decoded here.
void sub_1006be50(void* pA, void* pB, uint32 nC);

// 0x10014ab0 -- transpose-copy: reads the 4x4 at its SECOND argument and
// writes the transpose to its FIRST (src+0x00,+0x10,+0x20 -> dst+0x00,+0x04,
// +0x08; src+0x04,+0x14,+0x24 -> dst+0x10,+0x14,+0x18; ...).
void sub_10014ab0(void* pDest, const void* pSrc);

// 0x1007fc50 -- linear table search, four arguments (key, base, count,
// element width), `repz cmpsb` per element, answering the element index or
// -1. It lives above 0x1007fa10 so it is library code, not renderer code.
// (Note in passing: it loads its key from 0x4(%esp) and re-loads the SAME
// 0x4(%esp) as the compare source inside the loop, never touching its second
// argument at all. Whatever it was compiled from, the shipped code compares
// element 0 against itself and therefore always answers 0 for a non-zero
// count. Recorded because it is surprising, not because it changes anything
// below -- the three callers are decoded from their own bytes.)
int32 sub_1007fc50(const void* pKey, const void* pBase, uint32 nCount,
                   uint32 nWidth);


// ======================================================================= //
//  0x1000d9b0   31 bytes (size correct)   17 callers
//
//  WILL NOT REPRODUCE. Opens `sub $0xc,%esp` with no locals and unwinds it
//  as `add $0x8,%esp` immediately before the two pushes -- alignment padding
//  cl does not emit. Everything else about it is ordinary.
//
//  WHAT PINS IT: the callee takes (vector, 3x4 matrix) and the second
//  argument here is `arg2 + 0x15c` formed by `lea 0x15c(%eax),%eax`, so the
//  matrix is a member at +0x15c of the second parameter. The caller at
//  0x1000bbde loops pushing a fixed global (0x1008b880) as the second
//  argument and a pointer advancing by 0x28 as the first, which is the
//  vector. Nothing is returned: 0x1000a770 stores its three results through
//  the vector pointer and the callers discard %eax.
// ======================================================================= //

void sub_1000d9b0(LTVector* pVec, uint8* pObject)
{
	sub_1000a770(pVec, (const float*)(pObject + 0x15c));
}


// ======================================================================= //
//  0x1000ef60   55 bytes (size correct)   1 caller (0x100125e6)
//
//  WILL NOT REPRODUCE. All three outgoing arguments are written with `mov`
//  into a 0x14-byte outgoing-argument area (`mov %ecx,(%esp)`,
//  `mov %edx,0x4(%esp)`, `movl $0x0,0x8(%esp)`) rather than pushed, and the
//  second parameter is loaded at 0x8(%esp) BEFORE the `sub $0x14,%esp` --
//  d3dneutral.cpp reject rules (e) and (h).
//
//  WHAT PINS IT: a 16-bit field at +0x14 of the SECOND parameter, tested for
//  bit 15 with `test $0x8000,%eax` on a `movzwl`, cleared with
//  `and $0x7fff` and stored back as a WORD (`mov %ax,0x14(%edx)`), and the
//  whole rest of the body is inside that test. The third argument to
//  0x1006be50 is the literal 0.
// ======================================================================= //

struct Struct1000ef60                                   // second parameter
{
	uint8  m_Unknown00[0x14];                           // +0x00
	uint16 m_wFlags14;                                  // +0x14
};

void sub_1000ef60(void* pA, Struct1000ef60* pB)
{
	if (pB->m_wFlags14 & 0x8000)
	{
		pB->m_wFlags14 &= 0x7fff;
		sub_1006be50(pA, pB, 0);
	}
}


// ======================================================================= //
//  0x100157e0   24 bytes (size correct)   1 caller (0x1001a7f5)
//
//  NEAR, AND A TOOLCHAIN RESIDUAL (measured under icl 4.0). Every shipped
//  instruction is reproduced; ours has ONE extra: a spurious `83 c4 00`
//  (`add $0x0,%esp`) between the `sub $0x8,%esp` and the first push. icl 4.0
//  /Os emits that pre-push adjustment in every esp-framed function that
//  pushes call arguments (it is `add $0x8` in 0x10019a24, which matches),
//  and never drops it when it is zero; d3d.ren contains no `add $0x0,%esp`
//  anywhere. Tried: /O2 (/O1, /Ox, /G4-/G6, /Qsfalign-, /Qsfalign16, /Gs,
//  /Ob0, /Qip, /Oi-, /Qrcd) -- /O2 builds the call with stores, not pushes;
//  the rest keep the `add $0`. Callee declared C++, extern "C", variadic,
//  through a pointer: no change. Same residual as 0x1001ead0 (d3dr2b.cpp),
//  0x10050cc0 and 0x10050de0.
//
//  Also tried as the SDK's by-value
//  `LTMatrix LTMatrix::MakeInverseTransform() { LTMatrix mRet;
//  Mat_InverseTransformation(this, &mRet); return mRet; }` (it is exactly
//  that ABI: hidden return pointer, returned in %eax, ret $4), with and
//  without a constructor, and inside w3_10014580_os.cpp's TU between
//  0x10015770 and 0x100158c0 where the address puts it, with the callee
//  0x10014ab0 defined in the same TU: the `add $0x0,%esp` stays every time.
//
//  Not a compiler revision either: this file compiled with icl 4.5 (build
//  00015, toolchains/icl-4.5) keeps the `add $0x0,%esp` byte for byte AND
//  loses eight of the file's nine matches (1 of 10 against 9 of 10); icl
//  5.0.1 (build 010525Z) passes the arguments with stores instead of pushes
//  (`mov %ecx,(%esp)`, `mov %ebp,0x4(%esp)`) and matches nothing of the
//  file. Both measured with d3dlow.obj's own recorded command, only the
//  compiler directory swapped.
//
//  (Older cl-era note:) the ONLY thing cl got wrong was the alignment pair
//  `sub $0x8,%esp` before the two pushes, unwound by `add $0x10,%esp`.
//
//  WHAT PINS IT: `ret $0x4` with %ecx never loaded is __thiscall with one
//  argument. %ecx is pushed SECOND and the argument FIRST, so the callee's
//  parameter order is (arg, this) -- and 0x10014ab0 writes the transpose to
//  its first parameter and reads its second, so `this` is the source and the
//  argument is the destination. The answer is `mov %ebx,%eax`, i.e. the
//  argument, not `this`. The call site agrees: %ecx = &local(-0x300) and the
//  pushed pointer = &local(-0x2c0), two 0x40-byte objects 0x40 apart, and it
//  keeps using -0x300 afterwards.
// ======================================================================= //

class C100157e0
{
public:
	// 0x100157e0
	LTMatrix* sub_100157e0(LTMatrix* pDest);
};

LTMatrix* C100157e0::sub_100157e0(LTMatrix* pDest)
{
	sub_10014ab0(this, pDest);
	return pDest;
}


// ======================================================================= //
//  0x10015e30 moved to big17c14.cpp: it is a /Qip internal-linkage
//  (register-entry) static and has to sit in its caller's TU.
// ======================================================================= //


// ======================================================================= //
//  0x10019a24   75 bytes (size correct)   1 caller (0x1001a87a)
//
//  BYTE-IDENTICAL under icl /Os. The one lever was statement order: the
//  +0x698 store (m_p04->m_n88) has to be written FIRST -- then icl loads
//  +0x88 ahead of the +0x690 store, as shipped. Written in member order it
//  schedules the literal-1 store first (18 bytes differ).
//
//  NOT A SHIM CASE, though it looks like one: 0x10019a20 holds a loose
//  `mov 0x4(%esp),%eax` four bytes ahead of this entry. It is dead -- %eax
//  is overwritten by the second instruction here and nothing calls or
//  references 0x10019a20 -- so the function really does begin at 0x10019a24.
//
//  WHAT PINS IT: `ret` with no operand and %ecx never loaded is __thiscall
//  with no arguments. Three adjacent members at +0x690, +0x694, +0x698 are
//  filled from this->m_p04, the literal 1, and this->m_p04->+0x88; then the
//  global at 0x1008beb0 is dereferenced, its +0xe4 tested, and if non-null
//  called with (&this->m_690, that object's +0xe8). Note the source of
//  +0x690 is loaded TWICE (`mov 0x4(%ecx),%eax` then `mov 0x4(%ecx),%edx`),
//  once for the store and once to reach +0x88 -- two separate reads of the
//  same member, so the source has two separate mentions of it.
// ======================================================================= //

struct Struct10019a24Sink                               // *0x1008beb0
{
	uint8  m_Unknown00[0xe4];                           // +0x00
	void   (*m_pfnSubmit_e4)(void* pRecord, void* pUser); // +0xe4
	void*  m_pUser_e8;                                  // +0xe8
};

extern Struct10019a24Sink* g_p1008beb0;                 // 0x1008beb0

struct Struct10019a24Source
{
	uint8  m_Unknown00[0x88];                           // +0x00
	uint32 m_n88;                                       // +0x88
};

struct Struct10019a24Record
{
	Struct10019a24Source* m_pSource;                    // +0x00 (obj +0x690)
	uint32 m_n04;                                       // +0x04 (obj +0x694)
	uint32 m_n08;                                       // +0x08 (obj +0x698)
};

class C10019a24
{
public:
	// 0x10019a24
	void sub_10019a24();

	Struct10019a24Source*  m_pUnknown00;                // +0x00
	Struct10019a24Source*  m_p04;                       // +0x04
	uint8                  m_Unknown08[0x690 - 8];      // +0x08
	Struct10019a24Record   m_Record690;                 // +0x690
};

void C10019a24::sub_10019a24()
{
	m_Record690.m_n08     = m_p04->m_n88;
	m_Record690.m_pSource = m_p04;
	m_Record690.m_n04     = 1;

	if (g_p1008beb0->m_pfnSubmit_e4)
	{
		g_p1008beb0->m_pfnSubmit_e4(&m_Record690, g_p1008beb0->m_pUser_e8);
	}
}


// ======================================================================= //
//  0x1001c480 / 0x1001cf60 / 0x1001d980   59 bytes each (sizes correct)
//  No direct callers -- reached through a pointer, i.e. virtual.
//
//  WILL NOT REPRODUCE. All four outgoing arguments are written into a
//  0x14-byte outgoing area with `mov`, none pushed.
//
//  THE THREE ARE THE SAME FUNCTION with one literal changed -- element width
//  0x40, 0x20 and 0x02 -- so they are three instantiations of one template
//  or three overrides of one virtual over differently sized elements. Every
//  other byte is identical.
//
//  WHAT PINS THEM:
//   * `ret $0x8` with %ecx never loaded: __thiscall, two arguments.
//   * the search takes (arg1, this->m_p04, this->m_n08, WIDTH) -- so +0x04 is
//     the table base and +0x08 the element count.
//   * `cmp $0xffffffff,%eax; setne %dl` with %edx pre-zeroed: the answer is
//     the COMPARISON `!= -1`, not a cast of the index (this is the known
//     trap, and it is exactly what the bytes say).
//   * the index is stored through the second argument (`mov %eax,(%ecx)`)
//     BEFORE the boolean is moved into %eax, and it is stored
//     unconditionally -- including the -1.
// ======================================================================= //

class C1001c480
{
public:
	// 0x1001c480 (width 0x40), 0x1001cf60 (0x20), 0x1001d980 (0x02)
	LTBOOL sub_1001c480(const void* pKey, int32* pnIndexOut);
	LTBOOL sub_1001cf60(const void* pKey, int32* pnIndexOut);
	LTBOOL sub_1001d980(const void* pKey, int32* pnIndexOut);

	uint32      m_nUnknown00;                           // +0x00
	const void* m_pTable04;                             // +0x04
	uint32      m_nCount08;                             // +0x08
};

LTBOOL C1001c480::sub_1001c480(const void* pKey, int32* pnIndexOut)
{
	int32 nIndex = sub_1007fc50(pKey, m_pTable04, m_nCount08, 0x40);
	*pnIndexOut = nIndex;
	return nIndex != -1;
}

LTBOOL C1001c480::sub_1001cf60(const void* pKey, int32* pnIndexOut)
{
	int32 nIndex = sub_1007fc50(pKey, m_pTable04, m_nCount08, 0x20);
	*pnIndexOut = nIndex;
	return nIndex != -1;
}

LTBOOL C1001c480::sub_1001d980(const void* pKey, int32* pnIndexOut)
{
	int32 nIndex = sub_1007fc50(pKey, m_pTable04, m_nCount08, 0x02);
	*pnIndexOut = nIndex;
	return nIndex != -1;
}


// ======================================================================= //
//  0x1001d240   42 bytes, NOT 44 -- the `ret $0x4` ends at 0x1001d269 and
//                0x1001d26a is `8b f6` padding. No direct callers.
//
//  WILL NOT REPRODUCE, and it is the least confidently SOURCED of the twelve
//  even though every instruction is read. Three callee-saved registers are
//  pushed to serve one `rep movsl`, a 0x10-byte frame is opened for an
//  8-byte temporary, and %ecx is never touched.
//
//  WHAT PINS IT: `ret $0x4` with %ecx unused, an 8-byte block copied into the
//  incoming pointer and that pointer returned in %eax. That is the
//  hidden-return-pointer convention for returning an 8-byte struct BY VALUE:
//  the caller passes the destination, the callee fills it and hands it back.
//
//  THE ODDITY, recorded exactly: the temporary is eight bytes at 0x4(%esp)
//  and only its FIRST dword is initialised (`movl $0x0,0x4(%esp)`). The
//  `rep movsl` with %ecx = 2 then copies both dwords, so the second dword of
//  the returned object is whatever was on the stack. No source shape is
//  written that would reproduce that on purpose; the body below says
//  what the bytes say and no more, and the second member is deliberately
//  left unassigned rather than invented.
// ======================================================================= //

struct Struct1001d240
{
	uint32 m_n00;
	uint32 m_n04;

	// A one-argument constructor that leaves m_n04 alone is what the
	// shipped code builds: the value is made in a frame temporary and
	// block-copied (`rep movsl`, 2 dwords) into the return slot.
	Struct1001d240() {}
	Struct1001d240(uint32 n) { m_n00 = n; }
};

class C1001d240
{
public:
	// 0x1001d240
	Struct1001d240 sub_1001d240();
};

//  IDENTICAL as `return Struct1001d240(0);` -- a returned TEMPORARY is
//  built in the frame and copied out; a named local with the constructor
//  or with a member store is constructed straight into the return slot
//  (20 bytes, no copy).  A local copied into a second local also matches.
Struct1001d240 C1001d240::sub_1001d240()
{
	// m_n04 is NOT written by the shipped code; see the note above.
	return Struct1001d240(0);
}


// ======================================================================= //
//  0x1001e0e0   82 bytes, NOT 84 -- `ret $0x18` ends at 0x1001e131 and
//                0x1001e132 is `8b f6` padding.  1 caller (0x1001598c)
//
//  BYTE-IDENTICAL under icl /Os once +0x08 and +0x10 are FLOAT. It opens
//  `sub $0xc,%esp` to spill the fifth argument (`mov %edx,0x4(%esp)`,
//  reloaded at 0x1001e120). With those two members uint32 icl emits the same
//  29 instructions in a `sub $0x8` frame; as float the zero stores are still
//  integer `mov %eax` but icl reserves the extra slot. Store order does not
//  matter (icl reschedules it); int16 members or 'short' params cost bytes.
//
//  WHAT PINS IT: `ret $0x18` with %ecx never loaded is __thiscall with six
//  stack arguments. The store widths and offsets are unambiguous --
//    +0x00 dword = arg1        `mov %edi,(%ecx)`
//    +0x04 WORD  = arg2        `mov %si,0x4(%ecx)`     (movzwl-loaded)
//    +0x06 WORD  = arg3        `mov %bp,0x6(%ecx)`
//    +0x08 float = 0           `mov %eax,0x8(%ecx)`, %eax zeroed at entry
//    +0x0c WORD  = arg4        `mov %bx,0xc(%ecx)`
//    +0x0e WORD  = arg5        `mov %ax,0xe(%ecx)`     (the spilled one)
//    +0x10 float = 0           `mov %eax,0x10(%ecx)`
//    +0x14 dword = arg6        `mov %edx,0x14(%ecx)`
//  -- so +0x0a and +0x12 are never written, which is exactly the padding a
//  dword member at +0x08 and +0x10 implies. The four movzwl loads are what a
//  compiler emits reading `unsigned short` parameters off the stack.
//
//  It is a constructor or an Init: the one caller (0x10015950) runs it eight
//  times over an array whose stride is 0x1c, passing six zeros each time and
//  writing -1 into element+0x18 itself, which fixes the element at 0x1c
//  bytes -- 0x18 written here plus one dword the caller owns.
// ======================================================================= //

class C1001e0e0
{
public:
	// 0x1001e0e0
	void sub_1001e0e0(uint32 n00, uint16 w04, uint16 w06,
	                  uint16 w0c, uint16 w0e, uint32 n14);

	uint32 m_n00;                                       // +0x00
	uint16 m_w04;                                       // +0x04
	uint16 m_w06;                                       // +0x06
	// +0x08 and +0x10 are FLOAT. Declared uint32 they compile to the same
	// instructions in a 0x8-byte frame; as float icl reserves one more slot
	// (`sub $0xc,%esp`, as shipped) -- the only thing that closed this one.
	float  m_f08;                                       // +0x08  always 0
	uint16 m_w0c;                                       // +0x0c
	uint16 m_w0e;                                       // +0x0e
	float  m_f10;                                       // +0x10  always 0
	uint32 m_n14;                                       // +0x14
	uint32 m_n18;                                       // +0x18  caller's
};

void C1001e0e0::sub_1001e0e0(uint32 n00, uint16 w04, uint16 w06,
                             uint16 w0c, uint16 w0e, uint32 n14)
{
	m_n00 = n00;
	m_w04 = w04;
	m_w06 = w06;
	m_f08 = 0.0f;
	m_w0c = w0c;
	m_f10 = 0.0f;
	m_n14 = n14;
	m_w0e = w0e;
}


// ======================================================================= //
//  0x1001e140   208 bytes (size correct)   9 callers
//
//  WILL NOT REPRODUCE, and by the widest margin of the reproducible-looking
//  ones. The shipped body loads all sixteen arguments up front, holding five
//  in registers and SPILLING TEN into a 0x2c-byte local area, then stores
//  them out. cl emits sixteen independent `mov ARG,%eax; mov %eax,N(%ecx)`
//  pairs with no frame at all -- about 120 bytes against 208. The kinds are
//  right; the allocation is a different compiler.
//
//  WHAT PINS IT: `ret $0x40` with %ecx never loaded is __thiscall with
//  sixteen dword arguments, and the mapping is exactly argument i -> +4*(i-1)
//  for all sixteen (traced through every spill slot: arg7..arg16 go out via
//  0x28,0x24,0x20,0x1c,0x18,0x14,0x10,0x0c,0x08,0x04(%esp) in that order).
//
//  IT IS A 4x4 MATRIX Init. The caller at 0x100156e0 is the identity
//  constructor: it writes 0x3f800000 -- 1.0f -- into arguments 1, 6, 11 and
//  16 and zero into the other twelve, which is the diagonal of a row-major
//  4x4. Callers 0x1001f1d2/0x1001f249/0x1001f2d8/0x1001f397/0x1001f624/
//  0x1001f6a3 sit in the same neighbourhood as the transposes.
//
//  DECLARED uint32, NOT float, DELIBERATELY -- the same trap d3dmisc.cpp
//  documents for d3d_TransposeRotation. The shipped code moves all sixteen
//  values with integer `mov`s. Float parameters get `fld`/`fstp` out of cl
//  and that is a different function; the true source is certainly
//  `float m11..m44`, and this declaration preserves the machine-code KIND at
//  the cost of the declared type. Named after its address rather than
//  LTMatrix::Init because the class it belongs to is not established.
// ======================================================================= //

class C1001e140
{
public:
	// 0x1001e140
	void sub_1001e140(uint32 m11, uint32 m12, uint32 m13, uint32 m14,
	                  uint32 m21, uint32 m22, uint32 m23, uint32 m24,
	                  uint32 m31, uint32 m32, uint32 m33, uint32 m34,
	                  uint32 m41, uint32 m42, uint32 m43, uint32 m44);

	uint32 m[16];                                       // +0x00 .. +0x3c
};

void C1001e140::sub_1001e140(uint32 m11, uint32 m12, uint32 m13, uint32 m14,
                             uint32 m21, uint32 m22, uint32 m23, uint32 m24,
                             uint32 m31, uint32 m32, uint32 m33, uint32 m34,
                             uint32 m41, uint32 m42, uint32 m43, uint32 m44)
{
	m[0]  = m11;  m[1]  = m12;  m[2]  = m13;  m[3]  = m14;
	m[4]  = m21;  m[5]  = m22;  m[6]  = m23;  m[7]  = m24;
	m[8]  = m31;  m[9]  = m32;  m[10] = m33;  m[11] = m34;
	m[12] = m41;  m[13] = m42;  m[14] = m43;  m[15] = m44;
}
