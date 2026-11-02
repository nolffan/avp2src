// *********************************************************************** //
//
//  MODULE:   D3DBATCHB2 (.CPP)  -- "batch_b"
//
//  PURPOSE:  Twenty-three functions from the retail d3d.ren, decoded
//            instruction by instruction from the image.
//
//  WHAT THIS BATCH TURNED OUT TO BE.  Nine of the twenty-five rows are one
//  family, and the family has a NAME: they are the out-of-line members of
//  src/LT2/lithshared/stdlith/dynarray.h's `CMoArray<T,C>`, instantiated over
//  several element types.  The header is in this repo and it says exactly
//  what each wrapper is:
//
//      void CMoArray<T,C>::Remove(DWORD i)        { Remove2(i, &g_DefAlloc); }
//      BOOL CMoArray<T,C>::Insert(DWORD i, const T &v)
//                                         { return Insert2(i, v, &g_DefAlloc); }
//      BOOL CMoArray<T,C>::NiceSetSize(DWORD n)
//                     { return InternalNiceSetSize(n, FALSE, &g_DefAlloc); }
//      BOOL CMoArray<T,C>::SetSize(DWORD n)       { return SetSize2(n, &g_DefAlloc); }
//
//  and `&g_DefAlloc` is the address every one of them plants in its
//  outgoing area: 0x10091df0, the global allocator d3dbatchb.cpp already
//  named `g_10091df0`.  The proof is not the shape alone -- the two-argument
//  worker behind 0x1001ca80 (0x1001c8d0) is `Remove2` line for line: it
//  compares `m_Cache.GetCacheSize()` against `m_Cache.GetWantedCache()*2`
//  (`call 0x1000a690`, `call 0x1000a670`, `add %eax,%eax`, `cmp`), takes the
//  reallocate path on `jae` and otherwise slides the tail down one element
//  with a `rep movsl` of 0x10 dwords (a 64-byte T), and closes with
//  `decl 0x8(%ebx)` -- `--m_nElements`.  The class layout falls out of the
//  same function and agrees with the header's declaration order:
//  vptr +0x00 (CMoArray derives from GenList<T>), m_pArray +0x04,
//  m_nElements +0x08, m_Cache +0x0c (the two-dword DefaultCache whose four
//  accessors d3dbatchc.cpp already reconstructed at 0x1000a660..0x1000a690).
//
//  Only the WRAPPERS are in this batch, and a wrapper's bytes do not depend
//  on T, on the class layout, or on which of the four it is -- `Remove` and
//  `SetSize` are literally the same 38 bytes.  So they are written here as
//  the smallest thing that emits those bytes: one struct per address, the
//  worker declared and not defined, named after its own address the way
//  d3dbatchb.cpp's "Family 4" forwarders are.  The CMoArray reading is
//  recorded in each comment because it is what the header says, not because
//  the bytes needed it.
//
//  THE OTHER FOURTEEN are unrelated to each other and are documented one at
//  a time below.
//
//  TWO ROWS ARE SKIPPED, both for the same reason, and it is the reason
//  below:
//
//    0x10039f50  calls 0x10039bdc with %eax, %edx and %ecx loaded and
//                nothing pushed.  0x10039bd0 is the matching stack shim
//                (`mov 0x4(%esp),%eax ; mov 0x8(%esp),%edx ;
//                 mov 0xc(%esp),%ecx`) falling straight into 0x10039bdc, so
//                the call is to icl's REGISTER ENTRY of that function.
//    0x1007b880  calls 0x1007b580 with the argument in %eax and no shim in
//                front of it at all (0x1007b570 is a bare `ret`), i.e. a
//                register-ABI-only static.
//
//  Neither can be emitted from C++ unless the callee is co-defined in this
//  translation unit and icl independently chooses the same register
//  assignment for it.  That would be a guess.
//
// *********************************************************************** //

#include "ltbasedefs.h"


// ======================================================================= //
//  SHARED DECLARATIONS
// ======================================================================= //

// ---------------------------------------------------------------------- //
//  0x10091df0 -- stdlith's `g_DefAlloc` (l_allocator.h), the global LAlloc
//  every CMoArray convenience wrapper passes to its `2`-suffixed worker.
//  d3dbatchb.cpp reached the same object from the other side and modelled
//  it as an interface with Alloc at vtable +0x04 and Free at +0x08; the
//  same declaration is repeated here because these are separate translation
//  units.  Declared and never defined, so no vtable is emitted for it.
// ---------------------------------------------------------------------- //
struct IAllocator
{
	virtual void* Slot00(uint32 nSize);                 // vtable +0x00
	virtual void* Alloc(uint32 nSize);                  // vtable +0x04
	virtual void  Free(void* pMem);                     // vtable +0x08
};

extern IAllocator g_10091df0;


// ---------------------------------------------------------------------- //
//  The Direct3D 7 device, exactly as d3dopt2d.cpp / d3dbatchb.cpp model it.
//  Only +0x94 is needed here.
// ---------------------------------------------------------------------- //
struct D3DDevice7;

struct D3DDevice7Vtbl
{
	uint8 m_pad_00[0x94];
	// +0x94 -- IDirect3DDevice7::SetTextureStageState (index 37).
	long (__stdcall* SetTextureStageState)(D3DDevice7*, uint32 dwStage,
	                                       uint32 dwState, uint32 dwValue);
};

struct D3DDevice7
{
	D3DDevice7Vtbl* lpVtbl;
};

// 0x1008d988 -- the device.  Same global the rest of the reconstruction uses.
extern D3DDevice7* g_pD3DDevice;

#define D3DTSS_COLOROP                        1
#define D3DTSS_ALPHAOP                        4
#define D3DTOP_DISABLE                        1
#define D3DTOP_MODULATE                       4


// ======================================================================= //
//  FAMILY A -- CMoArray<T,C>::Remove / ::SetSize
//
//  Thirty-eight bytes, identical in all four but the call displacement:
//
//      55 8b ec                 push %ebp ; mov %esp,%ebp
//      83 ec 10                 sub  $0x10,%esp
//      83 c4 f8                 add  $0xfffffff8,%esp    ; 8-byte arg area
//      8b 45 08                 mov  0x8(%ebp),%eax      ; the one argument
//      c7 44 24 04 <0x10091df0> movl $&g_DefAlloc,0x4(%esp)
//      89 04 24                 mov  %eax,(%esp)
//      8b c1 / 8b c8            mov %ecx,%eax ; mov %eax,%ecx   (the `this`
//                               round trip icl emits for member-to-member)
//      e8 <worker>              call
//      8b e5 / 5d / c2 04 00    ret  $0x4
//
//  `ret $0x4` with `this` in %ecx is ONE stack dword, callee-pops.  Nothing
//  distinguishes `Remove` from `SetSize` in the bytes -- both are
//  `f(n) { g(n, &g_DefAlloc); }` and both compile to exactly this -- so the
//  choice of name in the comments follows the worker, not the wrapper.
// ======================================================================= //

// 0x1001ca80 -- worker 0x1001c8d0, which is `Remove2` verbatim (see the file
// header); T is 64 bytes.  So this one is provably `Remove(DWORD index)`.
struct Arr1001ca80
{
	void sub_1001c8d0(uint32 nIndex, IAllocator* pAlloc);   // Remove2
	void sub_1001ca80(uint32 nIndex);                       // Remove
};

void Arr1001ca80::sub_1001ca80(uint32 nIndex)
{
	sub_1001c8d0(nIndex, &g_10091df0);
}


// 0x1001deb0 -- the same wrapper over the worker at 0x1001dd90.
struct Arr1001deb0
{
	void sub_1001dd90(uint32 nIndex, IAllocator* pAlloc);
	void sub_1001deb0(uint32 nIndex);
};

void Arr1001deb0::sub_1001deb0(uint32 nIndex)
{
	sub_1001dd90(nIndex, &g_10091df0);
}


// 0x1007c840 -- the same wrapper over the worker at 0x1007c760.
struct Arr1007c840
{
	void sub_1007c760(uint32 nIndex, IAllocator* pAlloc);
	void sub_1007c840(uint32 nIndex);
};

void Arr1007c840::sub_1007c840(uint32 nIndex)
{
	sub_1007c760(nIndex, &g_10091df0);
}


// 0x1007f9f0 -- the same wrapper over the worker at 0x1007f8e0.  It is the
// last function below the 0x1007fa10 CRT line, and its own last byte is at
// 0x1007fa15; the CRT starts after it.
struct Arr1007f9f0
{
	void sub_1007f8e0(uint32 nIndex, IAllocator* pAlloc);
	void sub_1007f9f0(uint32 nIndex);
};

void Arr1007f9f0::sub_1007f9f0(uint32 nIndex)
{
	sub_1007f8e0(nIndex, &g_10091df0);
}


// ======================================================================= //
//  FAMILY B -- CMoArray<T,C>::Insert(DWORD index, const T &toInsert)
//
//  Forty-three bytes, identical in all three but the call displacement:
//
//      55 8b ec / 83 ec 18      frame, bigger: three outgoing dwords
//      83 c4 f4                 add  $0xfffffff4,%esp    ; 12-byte arg area
//      8b 45 08 / 8b 55 0c      the two arguments
//      89 04 24 / 89 54 24 04   stored in order
//      8b c1                    mov  %ecx,%eax
//      c7 44 24 08 <0x10091df0> movl $&g_DefAlloc,0x8(%esp)
//      8b c8                    mov  %eax,%ecx
//      e8 <worker> / 8b e5 / 5d / c2 08 00
//
//  `ret $0x8` is two stack dwords; the second is `const T&`, a pointer, so
//  the wrapper is element-type independent and all three are byte-identical.
// ======================================================================= //

// 0x1000a600 -- worker 0x1000a3f0 (Insert2).
struct Arr1000a600
{
	uint32 sub_1000a3f0(uint32 nIndex, const void* pValue, IAllocator* pAlloc);
	uint32 sub_1000a600(uint32 nIndex, const void* pValue);
};

uint32 Arr1000a600::sub_1000a600(uint32 nIndex, const void* pValue)
{
	return sub_1000a3f0(nIndex, pValue, &g_10091df0);
}


// 0x1001d780 -- worker 0x1001d570.
struct Arr1001d780
{
	uint32 sub_1001d570(uint32 nIndex, const void* pValue, IAllocator* pAlloc);
	uint32 sub_1001d780(uint32 nIndex, const void* pValue);
};

uint32 Arr1001d780::sub_1001d780(uint32 nIndex, const void* pValue)
{
	return sub_1001d570(nIndex, pValue, &g_10091df0);
}


// 0x1002ec00 -- worker 0x1002ea00.
struct Arr1002ec00
{
	uint32 sub_1002ea00(uint32 nIndex, const void* pValue, IAllocator* pAlloc);
	uint32 sub_1002ec00(uint32 nIndex, const void* pValue);
};

uint32 Arr1002ec00::sub_1002ec00(uint32 nIndex, const void* pValue)
{
	return sub_1002ea00(nIndex, pValue, &g_10091df0);
}


// ======================================================================= //
//  FAMILY C -- CMoArray<T,C>::NiceSetSize(DWORD newSize)
//
//  Forty-four bytes.  ONE stack argument (`ret $0x4`) but THREE outgoing
//  dwords, and the middle one is a literal zero:
//
//      55 8b ec / 83 ec 18 / 83 c4 f4     frame, 12-byte arg area
//      8b 45 08                 mov  0x8(%ebp),%eax
//      c7 44 24 04 00000000     movl $0x0,0x4(%esp)      <- FALSE
//      89 04 24                 mov  %eax,(%esp)
//      c7 44 24 08 <0x10091df0> movl $&g_DefAlloc,0x8(%esp)
//      8b c1 / 8b c8 / e8 <worker> / 8b e5 / 5d / c2 04 00
//
//  dynarray.h has exactly one member of that shape:
//      NiceSetSize(DWORD n) { return InternalNiceSetSize(n, FALSE, &g_DefAlloc); }
//  (`Fast_NiceSetSize` is the same call with TRUE, so the zero is what says
//  which of the two this is.)  Note the `mov %ecx,%eax` sits AFTER the
//  allocator store here and BEFORE it in Family B -- scheduling, not source.
// ======================================================================= //

// 0x100507b0 -- worker 0x1002e4c0 (InternalNiceSetSize).
struct Arr100507b0
{
	uint32 sub_1002e4c0(uint32 nNewSize, uint32 bFast, IAllocator* pAlloc);
	uint32 sub_100507b0(uint32 nNewSize);
};

uint32 Arr100507b0::sub_100507b0(uint32 nNewSize)
{
	return sub_1002e4c0(nNewSize, LTFALSE, &g_10091df0);
}


// 0x1007f7e0 -- worker 0x1007f420.
struct Arr1007f7e0
{
	uint32 sub_1007f420(uint32 nNewSize, uint32 bFast, IAllocator* pAlloc);
	uint32 sub_1007f7e0(uint32 nNewSize);
};

uint32 Arr1007f7e0::sub_1007f7e0(uint32 nNewSize)
{
	return sub_1007f420(nNewSize, LTFALSE, &g_10091df0);
}


// ======================================================================= //
//  0x1001e410 -- 15 bytes, 2 callers.  A __cdecl free function that calls a
//  __thiscall member on its argument.
//
//      55 8b ec                 push %ebp ; mov %esp,%ebp
//      8b 4d 08                 mov  0x8(%ebp),%ecx      ; arg straight to %ecx
//      e8 <0x100156e0>          call
//      8b e5 / 5d / c3          ret, NO count
//
//  WHAT PINS IT: a bare `ret` with the argument at 0x8(%ebp) is __cdecl with
//  one dword, and %ecx being loaded from that dword and nothing pushed is a
//  __thiscall callee with no arguments.  0x100156e0 confirms it from its own
//  side -- it ends `mov %ecx,%edx ; mov %edx,%ecx ; call 0x1001e140 ; ret`,
//  i.e. it reads %ecx as `this` and pops nothing.  No outgoing area is
//  reserved (no `sub` after the frame) precisely because the callee takes no
//  stack arguments; the frame is still set up and torn down.
//
//  Whether the source returns the callee's answer is not decidable -- %eax
//  survives either way -- so it is written `void`.
// ======================================================================= //
struct Obj100156e0
{
	void sub_100156e0();                                // declared only
};

void sub_1001e410(Obj100156e0* pObj)
{
	pObj->sub_100156e0();
}


// ======================================================================= //
//  0x10015510 -- 23 bytes, then icl's 0x90 padding.  `_CVector<T>::MagSqr`,
//  src/LT2/sdk/inc/ltvector.h line 40, at T=float and out of line.
//
//      56                       push %esi                ; icl's dead slot
//      d9 41 08 / d8 c8         flds 0x8(%ecx) ; fmul %st(0),%st     z*z
//      d9 41 04 / d8 c8         flds 0x4(%ecx) ; fmul %st(0),%st     y*y
//      de c1                    faddp
//      d9 01    / d8 c8         flds (%ecx)    ; fmul %st(0),%st     x*x
//      de c1                    faddp
//      83 c4 04 / c3            add $0x4,%esp ; ret
//
//  WHAT PINS IT: %ecx read and never loaded with a bare `ret` is __thiscall
//  with no arguments; the answer is left in %st(0), so the return type is
//  float.  The terms are evaluated z, y, x -- the rule that n-ary
//  float expressions associate from the LAST term backwards -- which is the
//  header's `(x*x) + (y*y) + (z*z)` and not any other order.  `fmul %st,%st`
//  rather than a second load is `x * x` on one operand.
//
//  It sits 0x20 bytes in front of Norm (0x10015530), which d3dbatchd.cpp
//  already reconstructed from the same header with Mag() inlined into it, so
//  this is the third _CVector member found verbatim in that file.  The
//  header defines MagSqr INSIDE the class body, which is implicitly inline;
//  it is defined out of line here because that is the only way a body gets
//  emitted at all under /Ob1.
// ======================================================================= //
class LTVec
{
public:
	float	sub_10015510() const;                       // MagSqr

	float	x;
	float	y;
	float	z;
};

float LTVec::sub_10015510() const
{
	return (x * x) + (y * y) + (z * z);
}


// ======================================================================= //
//  0x100647e0 -- 21 bytes of body, then icl lea-nop padding.  A two-dword
//  setter, the two-argument cousin of the accessors in d3dbatchc.cpp.
//
//      56                       push %esi                ; the dead slot
//      8b 44 24 08              mov  0x8(%esp),%eax
//      8b 54 24 0c              mov  0xc(%esp),%edx
//      89 41 18 / 89 51 1c      mov  %eax,0x18(%ecx) ; mov %edx,0x1c(%ecx)
//      83 c4 04 / c2 08 00      add  $0x4,%esp ; ret $0x8
//
//  WHAT PINS IT: `ret $0x8` with %ecx never loaded is __thiscall with two
//  stack dwords; both are stored with plain `mov`, so they are integers or
//  pointers and NOT floats (the integer-moves-for-float-data rule
//  read the other way round).  The stores are in ascending order, +0x18 then
//  +0x1c, which is source order.  %esi is pushed and never touched -- the
//  dead 4-byte slot, not a local.
// ======================================================================= //
struct Obj100647e0
{
	uint8  m_pad_00[0x18];                              // +0x00
	uint32 m_n18;                                       // +0x18
	uint32 m_n1c;                                       // +0x1c

	void sub_100647e0(uint32 nA, uint32 nB);
};

void Obj100647e0::sub_100647e0(uint32 nA, uint32 nB)
{
	m_n18 = nA;
	m_n1c = nB;
}


// ======================================================================= //
//  0x10023300 -- 43 bytes of body, then padding.  1 caller (0x1003d203).
//  A __cdecl forwarder that supplies the last two of four arguments.
//
//      83 ec 14                 sub  $0x14,%esp          ; 4 args + align
//      8b 44 24 18 / 8b 54 24 1c   the two arguments
//      89 54 24 04 / 89 04 24      stored, second first
//      c7 44 24 08 <0x1008b880>    movl $&g_1008b880,0x8(%esp)
//      c7 44 24 0c 00000000        movl $0x0,0xc(%esp)
//      e8 <0x1000f2c0> / 83 c4 14 / c3
//
//  WHAT PINS IT: no frame, no %ecx, `add $0x14,%esp` after the call and a
//  bare `ret` -- __cdecl both ways, two dwords in, four dwords out, answer
//  passed straight through in %eax (the caller at 0x1003d20b does
//  `test %eax,%eax ; je`, so there IS a return value).
//
//  0x10014550 is the same forwarder one argument further along --
//  `g(a, b, c, 0)` for three passed-in dwords -- which is what says the
//  third argument is an ordinary pointer and the fourth is the literal zero
//  rather than the other way round.  0x1008b880 is a large global object:
//  0x1000f2c0 reads +0x1dc..+0x218 off it and 0x1007b8e0 reads +0x4d4, and
//  0x1008ba5c (= 0x1008b880 + 0x1dc) is separately reachable as an absolute
//  address from nine other sites.  It is NOT named here.
// ======================================================================= //
extern uint8  g_1008b880[];                             // 0x1008b880
extern uint32 sub_1000f2c0(void* pA, void* pB, void* pC, uint32 nD);

uint32 sub_10023300(void* pA, void* pB)
{
	return sub_1000f2c0(pA, pB, g_1008b880, 0);
}


// ======================================================================= //
//  0x10035bd4 -- 42 bytes.  Release the COM object at +0x0c and clear the
//  two dwords in front of it.
//
//      55 8b ec / 83 ec 0c      frame, 12-byte outgoing area
//      53 / 8b d9               push %ebx ; mov %ecx,%ebx
//      8b 43 0c / 85 c0 / 74 0d if (m_p0c)
//      8b 10 / 50 / ff 52 08        m_p0c->lpVtbl->Release(m_p0c)
//      c7 43 0c 00000000            m_p0c = NULL
//      33 c0                    xor %eax,%eax
//      89 43 08 / 89 43 04      m_n08 = 0 ; m_n04 = 0      <- IN THAT ORDER
//      5b / 8b e5 / 5d / c3     ret, NO count
//
//  WHAT PINS IT:
//   * `mov (%eax),%edx ; push %eax ; call *0x8(%edx)` with no cleanup is the
//     function-pointer-struct COM shape, distinct from a C++
//     virtual call, and +0x08 on a DirectDraw surface is Release -- the slot
//     d3dblit.cpp already pins.
//   * a bare `ret` with %ecx as `this` is __thiscall with no arguments.
//   * the release arm stores an IMMEDIATE zero while the two tail stores
//     share one `xor %eax,%eax`, so the NULL in the branch and the two
//     zeroes after it are separate source statements.
//   * the tail stores descend, +0x08 then +0x04.  The class's constructor
//     two functions earlier (0x10035bb4) writes +0x04, +0x08, +0x0c
//     ASCENDING from the same `xor %eax,%eax`, which is what says icl is
//     preserving source order in both and that the descent here is real.
//     IF THIS ONE MISSES BY TWO BYTES, swap those two statements: that is
//     the only degree of freedom left in it.
//
//  THE CLASS comes from 0x10035bb4, which is its constructor: it plants a
//  vtable pointer (0x10089480) at +0x00, zeroes +0x04, +0x08 and +0x0c, and
//  is invoked on a block of exactly 0x10 bytes by 0x10036254 below.  So the
//  object is four dwords with the first taken by the vptr.  No virtual is
//  declared here because none is called; the slot is spelled out instead, so
//  that no vtable of ours is emitted.
// ======================================================================= //
struct DDSurface;

struct DDSurfaceVtbl
{
	void*  _QueryInterface;                             // +0x00
	void*  _AddRef;                                     // +0x04
	uint32 (__stdcall* Release)(DDSurface*);            // +0x08
};

struct DDSurface
{
	DDSurfaceVtbl* lpVtbl;
};

struct Elem10035bb4
{
	uint32     m_pVtbl;                                 // +0x00
	uint32     m_n04;                                   // +0x04
	uint32     m_n08;                                   // +0x08
	DDSurface* m_pSurface;                              // +0x0c

	// 0x10035bb4, the constructor 0x10036254 below invokes on a fresh
	// 0x10-byte block.  DECLARED ONLY -- it is not in this batch, and it
	// has to exist for `new Elem10035bb4` to emit the null test and the
	// constructor call that the shipped bytes contain.
	     Elem10035bb4();
	void sub_10035bd4();                             // w18_10035a70_osoiqip.cpp
};

// 0x10035bd4 is the register entry of icl's /Qip two-entry pair at
// 0x10035bd0 (a four-byte stack entry `mov 0x4(%esp),%eax` in front of it,
// which vtable 0x10089480's slot +0x1c points at), and the destructor
// 0x10035a70 calls it directly.  It is written there, with that caller, in
// w18_10035a70_osoiqip.cpp (both entries byte-identical).  This file's copy
// -- also byte-identical at /Os, whatever its old comment said -- was
// removed so the shipped function has one body; nothing named it.


// ======================================================================= //
//  0x10036254 -- 62 bytes of body, then padding.  2 callers.  A CONSTRUCTOR
//  BODY: allocate one Elem10035bb4, then fill three members and answer
//  `this`.
//
//      55 8b ec / 83 ec 10      frame
//      56 / 53 / 57             push %esi ; push %ebx ; push %edi
//                               (%edi is the dead slot -- it is never popped;
//                                the `add $0x8` after the call reclaims it
//                                together with the pushed size)
//      8b d9                    mov %ecx,%ebx            ; this
//      6a 10 / e8 <0x10081222>  operator new(0x10)
//      83 c4 08                 the 4-byte size + the 4-byte dead slot
//      8b f0 / 85 f6 / 74 21    if (p)
//      8b ce / e8 <0x10035bb4>       p->Elem10035bb4()
//      8b d6                         %edx = p
//                               else %edx = 0   (33 d2 at 0x10036290)
//      8b 45 0c / 8b 4d 08      the two arguments, second read first
//      89 13 / 89 4b 04 / 89 43 08   +0x00 = p ; +0x04 = arg0 ; +0x08 = arg1
//      8b c3                    return this
//      5b / 5e / 8b e5 / 5d / c2 08 00
//
//  WHAT PINS IT:
//   * `push $0x10 ; call ; test ; je ; ctor ; else zero` is the MSVC/icl
//     expansion of `new T` for a T with a non-trivial constructor, and 0x10
//     is sizeof(Elem10035bb4) -- the same object 0x10035bb4 initialises four
//     dwords of.
//   * `ret $0x8` plus `mov %ebx,%eax` on every path is a constructor: two
//     arguments in, `this` back out.
//   * the three stores ascend +0x00, +0x04, +0x08, which is initialiser-list
//     order and NOT the order the arguments are read in.
//
//  Written as a plain member returning `this` rather than as a real
//  constructor, because `??0...` has no address in its name and symbols
//  map back to addresses through the name.
// ======================================================================= //
struct Node10036254
{
	Elem10035bb4* m_pElem;                              // +0x00
	uint32        m_n04;                                // +0x04
	Node10036254* m_pNext;                              // +0x08

	Node10036254* sub_10036254(uint32 nValue, Node10036254* pNext);
};

Node10036254* Node10036254::sub_10036254(uint32 nValue, Node10036254* pNext)
{
	m_pElem = new Elem10035bb4;
	m_n04   = nValue;
	m_pNext = pNext;

	return this;
}


// 0x100362e4 (the recursive list Find) is d3dqip3.cpp's
// Node10036420::sub_100362e0: the shipped function is /Qip's external
// two-entry pair (`mov 0x4(%esp),%eax` at 0x100362e0, the body at +4, the
// recursion into +4), byte-identical there.  The second reconstruction
// that stood here, Node10036254::sub_100362e4 (/Os, no /Qip, `sub $0xc`
// against the pair's `sub $0x10`, one byte off), had no caller and was
// linked nowhere, so it is removed rather than carried as a duplicate.


// ======================================================================= //
//  0x1003b964 -- 85 bytes.  1 caller.  Fire a callback carried by the
//  argument, between two members of `this`.
//
//      55 8b ec / 83 ec 18      frame
//      57 / 53                  push %edi (dead slot) ; push %ebx
//      8b 55 08 / 8b d9         %edx = the argument ; %ebx = this
//      8b 82 a8 01 00 00        %eax = pCtx->+0x1a8
//      85 c0 / 75 07            if it is zero, fall into the epilogue
//      5b / 8b e5 / 5d / c2 04 00                 <- the shared exit, FIRST
//      57 / 8b cb / 89 14 24    this->sub_1003d950(pCtx)
//      89 55 fc                 pCtx spilled -- the call clobbers %edx
//      e8 <0x1003d950>
//      83 c4 f0                 16-byte outgoing area
//      8b 55 fc                 pCtx back
//      89 1c 24                 arg0 = this
//      89 54 24 04              arg1 = pCtx
//      8b 82 ac 01 00 00 / 89 44 24 08   arg2 = pCtx->+0x1ac
//      ff 92 a8 01 00 00        call *0x1a8(%edx)
//      83 c4 10                 CALLER cleans 16 -> the pointer is __cdecl
//      8b cb / e8 <0x1003b9c4>  this->sub_1003b9c4()
//      eb c2                    jmp back to the epilogue
//
//  WHAT PINS IT:
//   * `ret $0x4` with %ecx as `this` is __thiscall with one dword.
//   * the indirect call takes THREE dwords and the caller adds the whole
//     16-byte reservation back, so the function pointer is __cdecl; a
//     __stdcall pointer would have left a 4-byte correction.
//   * the epilogue is emitted at the TOP and the body's tail jumps BACK to
//     it.  Per the shared-exit lever that is an early `return` written
//     before the body, not `if (cond) { body }` -- the positive-condition
//     form puts the single exit after the body.
//   * both direct calls load %ecx from %ebx, i.e. they are members of the
//     SAME object as this function; neither takes a return value.
//
//  THE FUNCTION IS 85 BYTES, 0x1003b964..0x1003b9b8, and it is the /Qip
//  external pair's register entry (the `mov 0x4(%esp),%eax` stack entry
//  is 0x1003b960; its one caller 0x1003b740 enters at +4).  The listing's
//  0x1003b982 is a PHANTOM: it is the `jne` target at 0x1003b979, the
//  first instruction of the body after the hosted exit (`push %edi`), and
//  control only reaches it from inside this function -- no call, no
//  pointer to it anywhere in the image.
//
//  IDENTICAL.  The shipped allocation -- only `this` in a
//  callee-saved register, pCtx in %edx spilled to -0x4(%ebp) around the
//  0x1003d950 call -- is icl's COLD-CALL layout (w22_1007dde0_os.cpp,
//  d3dr5c_o2.cpp at 0x100038e0): a call whose decorated name contains
//  "fault" (warn, err, ...) is treated as cold and its live values are
//  spilled around it.  0x1003d950 saves the device state and installs the
//  callback's defaults, and declared as sub_1003d950_SetDefaultStates
//  (aliased onto the definition in w6_1003b9c4_os.cpp) the function is
//  byte-identical; renaming the context type to carry "Default" does the
//  same, renaming 0x1003b9c4 alone does not.  Earlier notes, kept:
//
//  NEAR (measured).  The test is written on the pointer's VALUE as an
//  integer: tested as a pointer, icl's static predictor takes the non-null
//  arm as hot, the body falls through and the exit sinks to the end (the
//  0x100037b0/0x100037d0 lever); as `(uint32)... == 0` the exit is hosted
//  first and the body jumps back, as shipped.  (`== 0` on the pointer, an
//  `if (p) {...}` wrap and `(uint32)... != 0` wrap all keep the old
//  layout.)  What is left is allocation: shipped keeps only `this` in a
//  callee-saved register (%ebx) and holds pCtx in %edx, spilled to
//  -0x4(%ebp) across 0x1003d950; ours keeps both in %esi/%ebx.  Not moved
//  by: /Os /Qip with 0x1003d950 and 0x1003b9c4 in the same TU (their
//  bodies copied into a d3dqip3.cpp probe), a local copy of pCtx or of
//  `this`, a block-scoped copy for the callback, an explicit `else`, or a
//  second guard.
// ======================================================================= //
struct Obj1003b964;

struct Ctx1003b964
{
	uint8  m_pad_000[0x1a8];                            // +0x000
	void   (*m_pfnCallback)(void* pObj, Ctx1003b964* pCtx, uint32 nParam);
	uint32 m_n1ac;                                      // +0x1ac
};

struct Obj1003b964
{
	// 0x1003d950 (w6_1003b9c4_os.cpp), declared under a name carrying
	// "Default" -- the cold-name lever; aliased onto sub_1003d950.
	void sub_1003d950_SetDefaultStates(Ctx1003b964* pCtx);
	void sub_1003b9c4();                                // declared only
	void sub_1003b964(Ctx1003b964* pCtx);
};

void Obj1003b964::sub_1003b964(Ctx1003b964* pCtx)
{
	if ((uint32)pCtx->m_pfnCallback == 0)
		return;

	sub_1003d950_SetDefaultStates(pCtx);
	pCtx->m_pfnCallback(this, pCtx, pCtx->m_n1ac);
	sub_1003b9c4();
}


// ======================================================================= //
//  0x10050c20 (54 bytes) and 0x1003b8e0 (54 bytes) -- TWINS.  The same
//  fifty-four bytes; only three constants differ (the global tested, the
//  sub-object offset, and the callback).  Register a callback with a
//  sub-object of the singleton at 0x10091a10, but only if a global is set.
//
//      55 8b ec / 83 ec 08      frame
//      a1 <global> / 85 c0 / 75 04    if it is zero...
//      8b e5 / 5d / c3               ...return                <- exit, FIRST
//      e8 <0x1007b8d0>          the singleton accessor
//      05 <offset>              += 0x2c4 (0x10050c20) / 0x304 (0x1003b8e0)
//      83 c4 f8                 8-byte outgoing area
//      8b c8                    that address becomes `this`
//      c7 04 24 <0x1008b880>    arg0 = &g_1008b880
//      c7 44 24 04 <callback>   arg1 = a code address
//      e8 <0x1007b8e0>
//      eb d7                    jmp back to the epilogue
//
//  WHAT PINS IT:
//   * 0x1007b8d0 is five bytes long -- `mov $0x10091a10,%eax ; ret` -- so it
//     is an accessor returning the address of a global object, and
//     `add $0x304,%eax ; mov %eax,%ecx` is `&p->member_at_0x304` used as a
//     `this`.
//   * 0x1007b8e0 ends `ret $0x8`: __thiscall, two stack dwords, and its own
//     body reads its first argument at 0x20(%esp) and then `0x4d4(%esi)`, so
//     that argument is a pointer to a large object -- which is what
//     0x1008b880 is (see 0x10023300 above).  The second is never loaded as
//     data in the prologue and both call sites hand it a .text address, so
//     it is a function pointer.
//   * the epilogue is at the top with a backward `jmp` into it, so the
//     source is `if (!g) return;` and not `if (g) { ... }`.
//
//  Neither the singleton nor the registrar is named; there is no evidence
//  here for what they are, only for their shapes.
// ======================================================================= //
extern void sub_10050bf0(void* pCtx, void* pItem);      // the 0x10050c20 callback (w27_10050900_os.cpp)
extern void sub_1003b760();                             // the 0x1003b8e0 callback

extern uint32 g_100893f4;                               // 0x100893f4
extern uint32 g_1008ed80;                               // 0x1008ed80

struct Reg1007b8e0
{
	void sub_1007b8e0(void* pObj, void (*pfnCallback)());
};

struct Root1007b8d0
{
	uint8       m_pad_000[0x2c4];                       // +0x000
	Reg1007b8e0 m_Reg2c4;                               // +0x2c4
	uint8       m_pad_2c5[0x304 - 0x2c4 - 1];
	Reg1007b8e0 m_Reg304;                               // +0x304
};

extern Root1007b8d0* sub_1007b8d0();                    // 0x1007b8d0

void sub_10050c20()
{
	if (!g_100893f4)
		return;

	sub_1007b8d0()->m_Reg2c4.sub_1007b8e0(g_1008b880, (void (*)())sub_10050bf0);
}

void sub_1003b8e0()
{
	if (!g_1008ed80)
		return;

	sub_1007b8d0()->m_Reg304.sub_1007b8e0(g_1008b880, sub_1003b760);
}


// ======================================================================= //
//  0x1004fb80 -- 58 bytes.  Walk an eleven-entry table of 0x14-byte records
//  in .data, calling each record's function pointer if a companion dword is
//  set, and clearing that dword either way.
//
//      55 / 53                  push %ebp ; push %ebx
//      33 db / 33 ed            %ebx = 0 (the byte index) ; %ebp = 0
//      <lea-nop alignment to 0x1004fb90>
//   L: 8b 83 <0x100895b4>       %eax = rec->+0x00
//      85 c0 / 74 0c            if (%eax)
//      8b 93 <0x100895b8>           %edx = rec->+0x04
//      85 d2 / 74 02                if (%edx)
//      ff d0                            (*%eax)()
//      89 ab <0x100895b8>       rec->+0x04 = 0
//      83 c3 14                 %ebx += 0x14
//      81 fb dc 00 00 00 / 72 d9   while (%ebx < 0xdc)
//      5b / 5d / c3
//
//  WHAT PINS IT: 0xdc / 0x14 == 11 exactly, so the stride is the record size
//  and the bound is the element count strength-reduced into a byte offset.
//  `call *%eax` with nothing pushed and nothing cleaned is a no-argument
//  pointer.  The zero is hoisted into %ebp because the store is then six
//  bytes instead of ten -- a size choice, not a source one -- and the clear
//  is OUTSIDE both tests because it happens on every iteration.  A bottom
//  test with the body ahead of it is the `for` loop with a known-positive
//  trip count, not a `while`.
//
//  The record's other twelve bytes are never touched here; the table is not
//  named beyond its address.
// ======================================================================= //
struct Rec100895b4
{
	void   (*m_pfn)();                                  // +0x00
	uint32 m_bSet;                                      // +0x04
	uint32 m_pad_08[3];                                 // +0x08
};

extern Rec100895b4 g_100895b4[11];                      // 0x100895b4



// ======================================================================= //
//  0x1001ead0 -- 73 bytes.  Three struct banks, initialised back to back.
//
//      83 ec 04                 sub $0x4,%esp            ; the dead slot
//      b8 40 00 00 00           mov $0x40,%eax
//      50 / 50 / 6a 14          push %eax ; push %eax ; push $0x14
//      68 <0x1008bef4>          push &bank
//      e8 <0x1007fb70> / 83 c4 10
//      b8 40 00 00 00 / 83 c4 10 / 50 / 50 / 6a 08 / 68 <0x1008bed8>
//      e8 <0x1007fb70> / 83 c4 10
//      68 00 08 00 00 / 68 00 02 00 00 / 6a 0c / 68 <0x1008bebc>
//      e8 <0x1007fb70>
//      83 c4 14                 0x10 of arguments + the 4-byte dead slot
//      c3
//
//  WHAT IT IS: 0x1007fb70 is stdlith's `sb_Init2`, and the identification is
//  not a guess -- its whole body is
//      sb_Init(pBank, structSize, cacheSize);
//      sb_AllocateNewStructPage(pBank, nPreallocations);
//  which is struct_bank.h's `sb_Init2` line for line.  The three globals are
//  0x1c apart (0x1008bebc, 0x1008bed8, 0x1008bef4), and `StructBank` in that
//  header is exactly seven dwords == 0x1c.  So this is three consecutive
//  banks over 0x14-, 8- and 0xc-byte structures.
//
//  WHAT PINS THE BYTES: arguments are PUSHED here, right to left, so the
//  reading is unambiguous: (bank, 0x14, 0x40, 0x40), (bank, 8, 0x40, 0x40),
//  (bank, 0xc, 0x200, 0x800).  `mov $0x40,%eax ; push %eax ; push %eax` is
//  icl noticing the repeated literal; the third call has no repeat and
//  pushes both of its large values as immediates.  A bare `ret` with no
//  arguments read is __cdecl, no parameters.
// ======================================================================= //
struct StructBank
{
	uint32 m_StructSize;                                // +0x00
	uint32 m_AlignedStructSize;                         // +0x04
	uint32 m_CacheSize;                                 // +0x08
	uint32 m_nPages;                                    // +0x0c
	uint32 m_nTotalObjects;                             // +0x10
	void*  m_PageHead;                                  // +0x14
	void*  m_FreeListHead;                              // +0x18
};                                                      // == 0x1c

// 0x1007fb70 -- sb_Init2(pBank, structSize, cacheSize, nPreallocations).
extern int sub_1007fb70(StructBank* pBank, uint32 nStructSize,
                        uint32 nCacheSize, uint32 nPrealloc);

extern StructBank g_1008bebc;                           // 0x1008bebc
extern StructBank g_1008bed8;                           // 0x1008bed8
extern StructBank g_1008bef4;                           // 0x1008bef4



// ======================================================================= //
//  0x10011500 (84 bytes) and 0x100733a0 (88 bytes) -- two texture-stage
//  pairs, the same function twice with different constants.
//
//  Both open `83 c4 f0` (a 16-byte outgoing area, four dwords), build the
//  call with stores rather than pushes, reload the device from 0x1008d988
//  for each call, and go through `call *0x94(%eax)` with no cleanup --
//  IDirect3DDevice7::SetTextureStageState, __stdcall, the slot d3d.cpp pins
//  from 0x10023c40.
//
//  0x10011500:  (stage 1, state 1, value 1) then (stage 1, state 4, value 1)
//  0x100733a0:  (stage 1, state 1, value 1) then (stage 0, state 1, value 4)
//
//  State 1 is D3DTSS_COLOROP and state 4 is D3DTSS_ALPHAOP; value 1 is
//  D3DTOP_DISABLE and value 4 is D3DTOP_MODULATE -- the same numbering
//  d3dbatchb.cpp already uses for D3DTOP_MODULATE at this slot.  So
//  0x10011500 turns stage 1 off entirely, colour and alpha, and 0x100733a0
//  turns stage 1's colour off and puts stage 0 back to MODULATE.  Two ways
//  of saying "one texture only".
//
//  WHAT PINS THE BYTES: `33 c0 40` -- xor/inc for the literal 1 -- appears
//  once per call, and in 0x10011500 the SECOND call reuses that 1 for two of
//  its three arguments while spelling 4 out as an immediate.  0x100733a0's
//  second call shares no value with the first and so writes all three as
//  immediates and drops the xor/inc.  That difference is the clearest
//  evidence that the constants above are read correctly.
// ======================================================================= //

// 0x10011500 -- disable texture stage 1.
void sub_10011500()
{
	g_pD3DDevice->lpVtbl->SetTextureStageState(g_pD3DDevice,
		1, D3DTSS_COLOROP, D3DTOP_DISABLE);
	g_pD3DDevice->lpVtbl->SetTextureStageState(g_pD3DDevice,
		1, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
}


// 0x100733a0 -- drop stage 1's colour op and restore stage 0's.
void sub_100733a0()
{
	g_pD3DDevice->lpVtbl->SetTextureStageState(g_pD3DDevice,
		1, D3DTSS_COLOROP, D3DTOP_DISABLE);
	g_pD3DDevice->lpVtbl->SetTextureStageState(g_pD3DDevice,
		0, D3DTSS_COLOROP, D3DTOP_MODULATE);
}


// 0x1001ead0: NEAR, toolchain residual. Every shipped instruction is
// reproduced; icl 4.0 /Os adds one spurious `83 c4 00` (`add $0x0,%esp`)
// before the first push, which shifts the remaining 41 bytes. d3d.ren has no
// `add $0x0,%esp` anywhere; no flag or callee declaration removes it (see
// d3dlow.cpp 0x100157e0 for the sweep), and /O2 turns the pushes into
// stores (41 -> 72 bytes differ).
void sub_1001ead0()
{
	sub_1007fb70(&g_1008bef4, 0x14, 0x40, 0x40);
	sub_1007fb70(&g_1008bed8, 8, 0x40, 0x40);
	sub_1007fb70(&g_1008bebc, 0xc, 0x200, 0x800);
}
