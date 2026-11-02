// Split out of d3dr2a.cpp: these functions need the
// OTHER optimisation flag from the rest of that file.  d3d.ren is a
// mixed build and our file boundaries are invented.
// Everything that is not a function body is
// carried over verbatim so no declaration is lost.
// ********************************************************* //
// *********************************************************************** //
//
//  MODULE:   BATCH A (.CPP) -- d3d.ren, 25-row listing.
//
//  24 of the 25 rows are written here; 0x1001e600 is skipped and the reason
//  is at the bottom of this header.
//
//  WHAT THIS BATCH IS MOSTLY MADE OF
//
//  Nine of the rows are ONE idiom in three widths: a __thiscall member that
//  forwards its own arguments to another __thiscall member of the same
//  object and supplies the global allocator at 0x10091df0 as the last one.
//  d3dbatchb.cpp already reproduces the sibling shape (0x1001cd10 /
//  0x1001e070, `f(0, nValue)`), so the frame, the outgoing-area stores and
//  the `mov %ecx,%eax ; mov %eax,%ecx` this-pointer round trip are all
//  known-good; only the argument list changes.  Diffing the nine gave three
//  distinct decodes:
//
//      arg-list                     ret      rows
//      (a, &g_alloc)                $0x4     0x1000a3c0 0x1001c880 0x1001dd40
//                                            0x1002e9d0 0x1007f8b0
//      (a, 0, &g_alloc)             $0x4     0x1001d270 0x1007c730
//      (a, b, &g_alloc)             $0x8     0x10021bf0 0x1007f360
//
//  Three more rows are ltmatrix.h verbatim -- GetTranslation, SetTranslation
//  and GetBasisVectors -- as expected.  They also
//  settle a question: LTMatrix's accessors copy FLOATS with
//  integer `mov`, not `fld`/`fstp`.  0x1001e480 reads exactly m[0][3],
//  m[1][3], m[2][3] of a 4x4 into +0/+4/+8 of a reference, which is
//  GetTranslation and nothing else, and every one of its moves is an
//  integer move.  So "integer moves for float data" does NOT by itself mean
//  the source is moving bit patterns in this module -- it means icl copies
//  float lvalues with the integer unit.  0x1004b6f0 (a 3-float assignment
//  returning *this) is written as floats for the same reason.
//
//  Two rows are list bookkeeping and they agree with src/LT2/sdk/inc/ltlink.h:
//  0x1006d520's `mov %eax,0x4(%eax) ; mov %eax,(%eax)` is CheapLTLink::
//  TieOff(), whose body is `m_pPrev = m_pNext = this` -- right to left, so
//  m_pNext (+0x04) is stored before m_pPrev (+0x00).  0x10020f80 then walks
//  a list through the SAME +0x00 field, so the traversal is via m_pPrev.
//
//  SKIPPED: 0x1001e600 (87 bytes, tagged reg-arg,x87).  It is a
//  REGISTER-ABI BODY and the listing filter missed it: the destination
//  matrix arrives in %eax and is never loaded from the stack, and the body
//  opens `fsub %st(1),%st` against an x87 value the CALLER left on the
//  stack.  It builds an identity 4x4 and then writes m[2][2] and m[2][3]
//  from (n / (b - a)) times the two incoming values -- a projection-matrix
//  z-range setup -- but no C source produces the entry conditions, so it is
//  not written.
//
// *********************************************************************** //

#include "ltbasedefs.h"


// ======================================================================= //
//  SHARED DECLARATIONS
// ======================================================================= //

// 0x10091df0 -- the global allocator OBJECT (not a pointer to one), already
// identified in d3dbatchb.cpp; the interface is copied from there verbatim so
// the two translation units mangle the extern identically.  Only its ADDRESS
// is used in this file, so none of the three slots is reached from here.
struct IAllocator
{
	virtual void*	Slot00(uint32 nSize);               // vtable +0x00
	virtual void*	Alloc(uint32 nSize);                // vtable +0x04
	virtual void	Free(void* pMem);                   // vtable +0x08
};

extern IAllocator g_10091df0;


// The engine's RenderStruct at 0x1008cbcc, as d3dbatchc.cpp and d3dmid.cpp
// model it.  This file needs one more slot than they do: +0x14 is the
// console printer -- 0x10020fb1 calls it __cdecl with a format string and
// one substitution and never unwinds the outgoing area.
struct RenderStructCallbacks
{
	uint8	m_Reserved00[0x14];                         // +0x00..+0x13
	void	(*ConsolePrint)(const char* pszFormat, ...);// +0x14
};

extern RenderStructCallbacks* g_pStruct;                // 0x1008cbcc


// The Direct3D 7 device, +0x94 SetTextureStageState, as d3dbatchb.cpp and
// d3dopt2d.cpp model it.
struct D3DDevice7;

struct D3DDevice7Vtbl
{
	uint8	m_pad_00[0x94];
	long	(__stdcall* SetTextureStageState)(D3DDevice7*, uint32 dwStage,
	                                          uint32 dwState, uint32 dwValue);
};

struct D3DDevice7
{
	D3DDevice7Vtbl* lpVtbl;
};

extern D3DDevice7* g_pD3DDevice;                        // 0x1008d988

#define D3DTSS_COLOROP                        1
#define D3DTOP_SELECTARG1                     2


// ======================================================================= //
//  THE ALLOCATOR-FORWARDING FAMILY -- nine rows, three shapes.
//
//  Every one of them is
//
//      55 8b ec                 push %ebp ; mov %esp,%ebp
//      83 ec NN                 the outgoing area
//      83 c4 NN                 icl's negative-constant unwind of part of it
//      8b 45 08 [8b 55 0c]      load the incoming argument(s)
//      ...                      write the outgoing area
//      8b c1 / 8b c8            mov %ecx,%eax ; mov %eax,%ecx
//      e8 <worker>              call
//      8b e5 / 5d / c2 NN 00    ret
//
//  which is byte for byte d3dbatchb.cpp's 0x1001cd10 with a different
//  argument list, so the shape is already measured.  The constant store is
//  scheduled FIRST in both files regardless of which slot it lands in --
//  0x1001cd10 writes `movl $0,(%esp)` before `mov %eax,0x4(%esp)`, and
//  0x1001c880 writes `movl $g,0x4(%esp)` before `mov %eax,(%esp)` -- so the
//  order below is the source order, not the emitted order.
//
//  Each wrapper gets its own class because each reaches a different worker
//  and nothing here proves any two of them are the same type.  The workers
//  are declared, never defined.
// ======================================================================= //

// ----------------------------------------------------------------------- //
//  Shape 1: one argument through, allocator appended.  `ret $0x4`.
// ----------------------------------------------------------------------- //

class C1000a3c0
{
public:
	void	sub_1000a220(uint32 nArg, IAllocator* pAlloc);
	void	sub_1000a3c0(uint32 nArg);
};



class C1001c880
{
public:
	void	sub_1001c7e0(uint32 nArg, IAllocator* pAlloc);
	void	sub_1001c880(uint32 nArg);
};



class C1001dd40
{
public:
	void	sub_1001dca0(uint32 nArg, IAllocator* pAlloc);
	void	sub_1001dd40(uint32 nArg);
};



class C1002e9d0
{
public:
	void	sub_1002e820(uint32 nArg, IAllocator* pAlloc);
	void	sub_1002e9d0(uint32 nArg);
};



class C1007f8b0
{
public:
	void	sub_1007f810(uint32 nArg, IAllocator* pAlloc);
	void	sub_1007f8b0(uint32 nArg);
};



// ----------------------------------------------------------------------- //
//  Shape 2: one argument through, a literal zero in the middle, allocator
//  last.  0x18-byte frame, `ret $0x4`.
// ----------------------------------------------------------------------- //

class C1001d270
{
public:
	void	sub_1001cd40(uint32 nArg, uint32 nFlags, IAllocator* pAlloc);
	void	sub_1001d270(uint32 nArg);
};



class C1007c730
{
public:
	void	sub_1007c290(uint32 nArg, uint32 nFlags, IAllocator* pAlloc);
	void	sub_1007c730(uint32 nArg);
};



// ----------------------------------------------------------------------- //
//  Shape 3: two arguments through, allocator appended.  `ret $0x8`.
// ----------------------------------------------------------------------- //

class C10021bf0
{
public:
	void	sub_10021ad0(uint32 nA, uint32 nB, IAllocator* pAlloc);
	void	sub_10021bf0(uint32 nA, uint32 nB);
};



class C1007f360
{
public:
	void	sub_1007f150(uint32 nA, uint32 nB, IAllocator* pAlloc);
	void	sub_1007f360(uint32 nA, uint32 nB);
};



// ======================================================================= //
//  LTVECTOR / LTMATRIX, out of line.  Bodies from src/LT2/sdk/inc/ltvector.h and
//  ltmatrix.h; the classes are renamed so the members can be placed by address.
// ======================================================================= //

// The 3-float vector.  0x1004b6f0 is the assignment operator: it copies
// three dwords from the argument into `this` at +0x00, +0x04, +0x08 in that
// order and answers `this` in %eax, which is `return *this` on a reference
// return.  _CVector<T> declares no operator=, so the shipped one is the
// implicit member-wise copy; it is spelled out here because an implicit
// member cannot carry a sub_<address> name.
class Vec1004b6f0
{
public:
	Vec1004b6f0&	sub_1004b6f0(const Vec1004b6f0& v);

	float	x;
	float	y;
	float	z;
};

// 0x1004b6f0 -- 27 bytes of body then a lea-nop to 0x1004b710.  2 callers.
//
// WHAT PINS IT: `ret $0x4` with %ecx never loaded is __thiscall with one
// stack argument; the three dwords land at +0x00/+0x04/+0x08 of %ecx read
// from +0x00/+0x04/+0x08 of the argument, so source and destination are the
// same layout; and the answer is `mov %ecx,%eax`, i.e. `this`, which a
// reference return produces and a void one does not.


// The 4x4, indexed [row][column] exactly as ltmatrix.h says -- a translation
// lives in m[0][3], m[1][3], m[2][3], i.e. +0x0c, +0x1c, +0x2c.
class Mat1001e480
{
public:
	void	sub_1001e480(Vec1004b6f0& vec);             // GetTranslation
	void	sub_1001e4d0(Vec1004b6f0* pRight, Vec1004b6f0* pUp,
	                     Vec1004b6f0* pForward);        // GetBasisVectors
	void	sub_10028010(Vec1004b6f0 vTrans);           // SetTranslation

	float	m[4][4];
};


// 0x1001e480 -- 26 bytes of body, 2 callers.  LTMatrix::GetTranslation,
// ltmatrix.h:769.
//
// WHAT PINS IT: the three source offsets are 0x0c, 0x1c, 0x2c -- the fourth
// column of a 4x4 -- and they land at +0x00, +0x04, +0x08 of the argument in
// that order.  `ret $0x4` is the one reference argument.  Every move is an
// integer `mov`; the header's members are floats, so this is icl copying
// float lvalues with the integer unit and NOT a bit-pattern source.


// 0x1001e4d0 -- 73 bytes of body (0x1001e4d0..0x1001e518), 2 callers.
// LTMatrix::GetBasisVectors, ltmatrix.h:663.
//
// WHAT PINS IT: `ret $0xc` is three pointer arguments, and the nine loads
// are 0x00/0x10/0x20 -> pRight, 0x04/0x14/0x24 -> pUp, 0x08/0x18/0x28 ->
// pForward -- the first three COLUMNS of the 3x3, read in the header's
// order.  The three destination pointers are loaded up front into %eax,
// %ebx and %edx (arg0, arg1, arg2 -- note %edx takes arg2 before %ebx takes
// arg1, which is scheduling), and the last one is answered through %ecx
// because %eax has been reused as a scratch by then.


// 0x10028010 -- 25 bytes of body then a lea-nop, 1 caller.
// LTMatrix::SetTranslation(float, float, float), ltmatrix.h:781.
//
// WHAT PINS IT: `ret $0xc` is three floats, and they go to 0x0c, 0x1c and
// 0x2c -- the same three slots GetTranslation reads.  The STORES come out
// x, z, y (0x0c, 0x2c, 0x1c) because y is held in %edx across the third
// load; the offsets, not the store order, are what identify it, and the
// source order is x, y, z as the header writes it.


// ======================================================================= //
//  0x1004b710   11 bytes   1 caller (0x1004b775)
//
//  The whole body is `lea 0x198(%ecx),%eax`: a __thiscall member with no
//  arguments (`ret`, no immediate) answering the address of a member at
//  +0x198.  The `push %esi` / `add $0x4,%esp` around it is icl's dead
//  four-byte slot.
//
//  Its caller sits four bytes past a call to 0x1004b6f0, so the pointer this
//  hands back is very likely the destination of that vector assignment --
//  but the caller's own bytes are the only evidence for that and it is not
//  needed to fix these eleven, so the member is typed as a bare pointer.
// ======================================================================= //

class C1004b710
{
public:
	void*	sub_1004b710();

	uint8	m_Reserved00[0x198];                        // +0x000..+0x197
	uint32	m_n198;                                     // +0x198
};



// ======================================================================= //
//  0x10012380   19 bytes   1 caller
//
//  `if (g_1008d4c4) sub_10011620();`
//
//  WHAT PINS IT: the global is LOADED into %eax and tested, but %eax is not
//  an argument -- 0x10011620 never reads it (its first act is
//  `mov $0x2,%eax`) and nothing is written to an outgoing area, so the call
//  takes nothing.  d3dbatchb.cpp already records that 0x10011620 walks a
//  list rooted at 0x1008d4c4, which is why the guard tests exactly that
//  root.
// ======================================================================= //

extern void* g_1008d4c4;                                // 0x1008d4c4

void sub_10011620();

void sub_10012380()
{
	if (g_1008d4c4)
		sub_10011620();
}


// ======================================================================= //
//  0x1006d520   42 bytes of body   1 caller (0x1003add1)
//
//  Five doubly-linked list heads tied off and five counters zeroed.
//
//  WHAT PINS IT: `mov %eax,0x4(%eax)` immediately followed by
//  `mov %eax,(%eax)` is CheapLTLink::TieOff() -- `m_pPrev = m_pNext = this`
//  evaluates right to left, so m_pNext at +0x04 is stored first and m_pPrev
//  at +0x00 second.  The links stride 0x0c (sizeof LTLink minus m_pData is
//  8, so the array element is the full 12-byte link) and the counters stride
//  4, in two SEPARATE arrays -- 0x10091954 for the counts and 0x10091968 for
//  the links -- which is why this is not an array of LTList.  The loop bound
//  is carried on the counter offset: `cmp $0x14,%edx` after `add $0x4,%edx`,
//  i.e. five iterations.
// ======================================================================= //

struct Link1006d520
{
	Link1006d520*	m_pPrev;                            // +0x00
	Link1006d520*	m_pNext;                            // +0x04
	void*			m_pData;                            // +0x08

	void TieOff() { m_pPrev = m_pNext = this; }
};

extern uint32       g_10091954[5];                      // 0x10091954
extern Link1006d520 g_10091968[5];                      // 0x10091968

// Moved here from d3dr2a.cpp and made a member.  Two things were wrong with
// the free function at /O2, and both are measured:
//   * /O2 schedules the loop differently (the counter store first, the link
//     address formed with `add %edx,%eax` after it); /Os gives the shipped
//     `add %ecx,%eax ; add $0xc,%ecx ; TieOff ; counter` order.
//   * the shipped `push %esi ... add $0x4,%esp` is icl's four-byte dead slot
//     for a __thiscall `this` -- a free function has no frame at all.  The
//     `this` is never read, so the owning class is not recoverable from
//     these bytes; it is named for the address.
// Byte-identical with both.
class C1006d520
{
public:
	void sub_1006d520();
};

void C1006d520::sub_1006d520()
{
	uint32 i;

	for (i = 0; i < 5; i++)
	{
		g_10091968[i].TieOff();
		g_10091954[i] = 0;
	}
}



// ======================================================================= //
//  0x100503c0   48 bytes   1 caller (0x10033130)
//
//  Walk eleven 20-byte descriptors; call each one's hook if it has one, then
//  mark it.
//
//  WHAT PINS IT: the stride is 0x14 and the bound is 0xdc, so 11 elements;
//  the hook is at +0x00 and is invoked as `call *%eax` with nothing written
//  to an outgoing area and nothing unwound, so it takes no arguments; the
//  flag written is at +0x08 (0x100895b8 - 0x100895b0) and the value 1 is
//  hoisted into %ebp before the loop, which is what a loop-invariant literal
//  does.  The store happens on BOTH paths -- it is after the `je`, not
//  inside the guarded arm.
//
//  (The listing tags this row x87.  There is no floating point in it.)
// ======================================================================= //

struct Elem100503c0
{
	void	(*m_pfnHook)();                             // +0x00
	uint32	m_n04;                                      // +0x04
	uint32	m_n08;                                      // +0x08
	uint32	m_n0c;                                      // +0x0c
	uint32	m_n10;                                      // +0x10
};

extern Elem100503c0 g_100895b0[11];                     // 0x100895b0



// ======================================================================= //
//  0x1007e7f4   52 bytes   2 callers (0x1007e427, 0x1007e7ac)
//
//  Clear a field and hand one member to an interface method.
//
//  WHAT PINS IT: the object called is `((IFace**)this->m_p08)[this->m_n18]`
//  -- an array of pointers indexed by a member -- and the call is the
//  FUNCTION-POINTER-STRUCT form (`mov (%eax),%eax ; call *0x1c(%eax)` with
//  the object itself written to (%esp) as the first argument), not icl's
//  two-load C++ virtual call.  `ret` with no immediate and %ecx never
//  loaded is __thiscall with no arguments.  m_n5c is zeroed BEFORE the call
//  is set up, and the zero is reused as the third argument, which is why
//  icl parks it in %edi and spills %edi to a local rather than pushing it.
//
//  The four bytes at 0x1007e7f0 in front of this (`mov 0x4(%esp),%eax`) are
//  an unreferenced calling-convention shim -- nothing calls them, both
//  callers target 0x1007e7f4 -- so they are not part of this function.
// ======================================================================= //

struct IFace1007e7f4;

struct IFace1007e7f4Vtbl
{
	uint8	m_pad_00[0x1c];
	long	(__stdcall* Slot1c)(IFace1007e7f4*, uint32 nA, uint32 nB);
};

struct IFace1007e7f4
{
	IFace1007e7f4Vtbl* lpVtbl;
};

class C1007e7f4
{
public:
	void	sub_1007e7f4();

	uint8			m_Reserved00[0x08];                 // +0x00..+0x07
	IFace1007e7f4**	m_pObjects;                         // +0x08
	uint8			m_Reserved0c[0x0c];                 // +0x0c..+0x17
	uint32			m_nIndex;                           // +0x18
	uint8			m_Reserved1c[0x40];                 // +0x1c..+0x5b
	uint32			m_n5c;                              // +0x5c
	uint8			m_Reserved60[0x08];                 // +0x60..+0x67
	uint32			m_n68;                              // +0x68
};

void C1007e7f4::sub_1007e7f4()
{
	IFace1007e7f4* pObj;

	m_n5c = 0;

	pObj = m_pObjects[m_nIndex];
	pObj->lpVtbl->Slot1c(pObj, m_n68, 0);
}


// ======================================================================= //
//  0x10032cc0   55 bytes of body   1 caller
//
//  Five no-argument calls in a row, then one guarded virtual call.
//
//  WHAT PINS IT: the five callees are already known -- 0x100145f0 and
//  0x1007b8c0 are the one-byte `ret`s d3dneutral.cpp reproduces, 0x1006a780
//  is `return g_100911b8` -- and the answer of the LAST one is what is
//  tested.  The call on it is icl's real C++ virtual (`mov (%eax),%edx` then
//  `mov (%edx),%eax` then `call *%eax`, `this` staying in %ecx and never
//  pushed) at slot +0x00, with a literal 1 as the only argument and no stack
//  cleanup, i.e. __thiscall.  The `sub $0x8,%esp` prologue holds no locals;
//  it is icl's alignment padding, reused as the one-dword outgoing area by
//  the `push %edi` inside the guard.
// ======================================================================= //

class C1006a780
{
public:
	virtual void	Slot00(uint32 nArg);
};

void  sub_100145f0();
void  sub_1002dab0();
void  sub_1007b8c0();
void  sub_1004fb80();

// Declared exactly as d3dneutral.cpp declares it -- `return g_100911b8` --
// so the two units agree on the symbol; the cast is what this call site
// does with the answer.
uint32 sub_1006a780();



// ======================================================================= //
//  0x10040810   62 bytes   1 caller
//
//  `if (g_1008cb2c) SetTextureStageState(0, D3DTSS_COLOROP,
//                                        D3DTOP_SELECTARG1);`
//
//  WHAT PINS IT: `call *0x94(%eax)` on 0x1008d988 with four outgoing dwords
//  is IDirect3DDevice7::SetTextureStageState, whose slot is already fixed by
//  d3dopt2d.cpp; the arguments are (device, 0, 1, 2) read straight off
//  (%esp), 0x4, 0x8 and 0xc.  The whole body is inside one positive test of
//  0x1008cb2c, and the `add $0xfffffff0,%esp` that opens the outgoing area
//  is INSIDE that test, not in the prologue, which is what a single guarding
//  `if` looks like.
//
//  (The listing tags this row x87 as well; there is no floating point.)
// ======================================================================= //

extern uint32 g_1008cb2c;                               // 0x1008cb2c



// ======================================================================= //
//  0x1005fa80   59 bytes   1 caller (0x100642c6)
//
//  Run a 0x20-byte-strided array through 0x10014580 against the global at
//  0x1008b880.
//
//  WHAT PINS IT: __cdecl, two arguments -- nothing is popped, the count
//  comes off 0x14(%esp) after three pushes and the pointer off 0x10(%esp).
//  0x10014580 is one of the x87 transform helpers d3dneutral.cpp lists, and
//  0x1008b880 is the same fixed global d3dlow.cpp sees pushed as the matrix
//  operand of 0x1000a770, so this is "transform N elements".  The eight-byte
//  outgoing area is opened once by the prologue and unwound once at the end,
//  so the callee is __cdecl too.
//
//  THE LOOP FORM: guard at the top, `dec %ebx` as the FIRST instruction of
//  the body, `test %ebx,%ebx ; jne` at the bottom.  `while (nCount--)` puts
//  the decrement there without relying on the scheduler.  If this comes back
//  near with the `dec` in the wrong place, the other reading is
//  `while (nCount) { ...; nCount--; }` -- try that one and nothing else.
// ======================================================================= //

extern uint8 g_1008b880[];                              // 0x1008b880

void sub_10014580(void* pElem, void* pXForm);



// ======================================================================= //
//  0x10020f80   65 bytes   1 caller (0x10032e28)
//
//  Print the name of every device on a list.
//
//  WHAT PINS IT: the head is the ADDRESS 0x1008bf78 (materialised three
//  separate times -- once for the guard compare, once as the loop-invariant
//  sentinel in %esi, and once for the initial load), the cursor advances
//  through the +0x00 field, and the payload is at +0x08, which is
//  ltlink.h's { m_pPrev, m_pNext, m_pData } with the walk running through
//  m_pPrev.  The name is `m_pData + 0x24`, formed by `lea 0x24(%eax),%eax`,
//  and the format string at 0x100a53d0 is "Device: %s".  The printer is
//  reached as `mov 0x1008cbcc,%eax ; call *0x14(%eax)` -- one load, so it is
//  a pointer to a function-pointer struct, and nothing is unwound after the
//  call, so it is __cdecl.
// ======================================================================= //

struct Link10020f80
{
	Link10020f80*	m_pPrev;                            // +0x00
	Link10020f80*	m_pNext;                            // +0x04
	uint8*			m_pData;                            // +0x08
};

extern Link10020f80 g_1008bf78;                         // 0x1008bf78

void sub_10020f80()
{
	Link10020f80* pCur;

	pCur = g_1008bf78.m_pPrev;

	while (pCur != &g_1008bf78)
	{
		uint8* pData = pCur->m_pData;

		g_pStruct->ConsolePrint("Device: %s", pData + 0x24);
		pCur = pCur->m_pPrev;
	}
}


// ======================================================================= //
//  0x1002dab0   75 bytes   1 caller (0x10032ccb, in sub_10032cc0 above)
//
//  Shutdown: one bare call, three teardowns of fixed globals, then one
//  conditional release that nulls its own pointer.
//
//  WHAT PINS IT: no frame pointer at all -- `sub $0xc,%esp` at entry and
//  `add $0xc,%esp` at exit, with every argument written by `mov` into
//  (%esp), so all four callees are __cdecl with one argument (or none, for
//  0x1001e920, which has nothing written before it).  The three arguments to
//  0x1007fbe0 are ADDRESSES of globals, not loads.  0x1008d524, by contrast,
//  IS loaded, tested, passed and then stored back as zero -- a pointer
//  variable, and the store is inside the same `if` as the call.
//
//  0x1007fbe0 is above 0x1007fa10, so it is library code, not renderer code;
//  it is declared, not reconstructed.
// ======================================================================= //

extern uint8 g_1008d4c8[];                              // 0x1008d4c8
extern uint8 g_1008d4e4[];                              // 0x1008d4e4
extern uint8 g_1008d500[];                              // 0x1008d500
extern void* g_p1008d524;                               // 0x1008d524

void sub_1001e920();
void sub_1007fbe0(void* p);
void sub_10021cb0(void* p);



// ======================================================================= //
//  0x10068890   73 bytes   1 caller
//
//  Two early returns, then one __thiscall on a sub-object.
//
//  WHAT PINS IT: `add $0x224,%ecx` looks dead -- nothing reads %ecx
//  afterwards in this function -- until you notice the call target.
//  0x1007b8e0 is C1007b8e0::sub_1007b8e0 from d3dbatchc.cpp: a __thiscall
//  taking (context, callback) with the element array at +0x00 and the COUNT
//  at +0x04.  So %ecx IS live into the call; it is `this`, and the guard
//  `mov 0x228(%ecx),%eax ; test ; jne` -- 0x224 + 4 -- is exactly that
//  count being tested for zero.  0x1007b8d0 is `return &g_10091a10`
//  (d3dneutral.cpp), so the sub-object is at g_10091a10 + 0x224.
//
//  THE THREE EPILOGUES: each return writes its own `mov %ebp,%esp ; pop
//  %ebp ; ret` and nothing is shared, which is the run-of-guarded-returns
//  shape -- two `if (...) return;` and then the body, NOT nested `if`s.
//
//  The context is the same 0x1008b880 that 0x1005fa80 above hands to
//  0x10014580; only its +0x4d4 is read by the callee.
// ======================================================================= //

struct Ctx1007b8e0;
struct Elem1007b8e0;

typedef void (*Fn1007b8e0)(Ctx1007b8e0* pContext, Elem1007b8e0* pElem);

class C1007b8e0
{
public:
	void	sub_1007b8e0(Ctx1007b8e0* pContext, Fn1007b8e0 pfn);

	Elem1007b8e0**	m_pElements;                        // +0x00
	uint32			m_nCount;                           // +0x04
};

class C10068890
{
public:
	uint8		m_Reserved00[0x224];                    // +0x000..+0x223
	C1007b8e0	m_List224;                              // +0x224
};

extern uint32 g_1009110c;                               // 0x1009110c

// Declared as d3dneutral.cpp declares it (`return &g_10091a10`).
void* sub_1007b8d0();
void sub_10067aa0(Ctx1007b8e0* pContext, Elem1007b8e0* pElem);

