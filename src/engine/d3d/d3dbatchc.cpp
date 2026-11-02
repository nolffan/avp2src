// *********************************************************************** //
//
//  MODULE:   BATCH_C (.CPP)
//
//  PURPOSE:  Listing batch C of d3d.ren -- twenty of the twenty-five rows,
//            written against Intel C/C++ 4.0 /O2 (the settled compiler for
//            this target).
//
//  Every offset, constant and branch below was read off the shipped bytes
//  of the retail d3d.ren. Where a caller was
//  needed to pin an argument count or a type, the caller's address is named
//  in the comment so the claim can be re-checked.
//
//  FIVE ROWS ARE NOT HERE; briefly:
//  0x1001e950 (340b of x87 with a hand-scheduled reflection
//  and a normalise), 0x10039850 and 0x1001be08 (icl register-ABI bodies,
//  neither with a pinned entry symbol), 0x100500a0 (484b ring
//  buffer) and 0x10023930 (784b, a guarded one-time table build).
//
//  THIS FILE IS BUILT /Os (build/engine.sh's D3D_OS_TUS). Six rows whose
//  shipped bodies carry /O2's sixteen-byte alignment -- 0x10009c90,
//  0x10009cc0, 0x10009ef0, 0x10023e50, 0x1007b8e0 and 0x1007c140 -- live in
//  d3dbatchc_o2.cpp instead; its header has the measurement.
//
//  CONVENTIONS THIS FILE RELIES ON, all read off the bytes:
//
//   * `push %esi ... add $0x4,%esp` around a body that never touches %esi is
//     icl's four-byte frame slot. In 0x1006a800 that slot is NOT dead -- it
//     holds a two-byte local array -- which is why that one function opens
//     it in the middle of the prologue instead of at the top.
//
//   * `add $0xfffffff8,%esp` before a call and `add $0x8,%esp` after it is a
//     __cdecl callee; the same `add $0xfffffff8,%esp` with NO restore
//     afterwards is a callee that pops (__thiscall/__stdcall). Both forms
//     appear here and they are what fix each callee's declaration.
//
//   * an outgoing area opened once in the prologue (`sub $0xc,%esp`) and
//     freed once in the epilogue means every call in the body is __cdecl
//     into a persistent area -- 0x1007b8e0 (now in d3dbatchc_o2.cpp) and
//     0x10032a00 are both that.  (0x10032a00 now lives in
//     w31_10032a00_qip.cpp.)
//
// *********************************************************************** //

#include "ltbasedefs.h"

#include <stdlib.h>
#include <string.h>


// ======================================================================= //
//  THE ENGINE'S RenderStruct AT 0x1008cbcc
//
//  d3dmid.cpp already pins +0x34 dalloc and +0x38 dfree and the struct below
//  keeps them at those offsets so the two translation units agree.
//
//  THREE NEW SLOTS, and they are the console-variable accessors:
//
//    +0x18  takes ONE argument and 0x10032a50 passes it the string
//           "ForceMode" (0x100a8b80); the answer is null-tested.
//    +0x1c  takes the +0x18 answer and returns a DOUBLE -- 0x1007c1b3
//           spills it with `fstpl`, not `fstps`.
//    +0x20  takes the +0x18 answer and returns something that 0x10032a35
//           hands straight to the CRT's _stricmp at 0x100874b0, so it is a
//           string.
//
//  Every one of the three is __cdecl: 0x1007c196 and 0x1007c1b0 restore the
//  outgoing area themselves.
// ======================================================================= //

struct RenderStructCallbacks
{
	uint8    m_Reserved00[0x18];                        // +0x00..+0x17
	void*   (*GetConsoleVariable)(const char* pszName); // +0x18
	double  (*GetVarValueFloat)(void* pVar);            // +0x1c
	char*   (*GetVarValueString)(void* pVar);           // +0x20
	uint8    m_Reserved24[0x10];                        // +0x24..+0x33
	void*   (*dalloc)(uint32 nSize);                    // +0x34
	void    (*dfree)(void* pMem);                       // +0x38
};

extern RenderStructCallbacks* g_pStruct;                // 0x1008cbcc


// ======================================================================= //
//  THE D3D7 DEVICE AT 0x1008d988 -- copied verbatim from d3dopt2d.cpp so the
//  two files agree on the one slot this file needs (+0x50 SetRenderState).
// ======================================================================= //

struct D3DDevice7;

struct D3DDevice7Vtbl
{
	uint8 m_pad_00[0x50];
	// +0x50 -- IDirect3DDevice7::SetRenderState (index 20).
	long (__stdcall* SetRenderState)(D3DDevice7*, uint32 dwState,
	                                 uint32 dwValue);
};

struct D3DDevice7
{
	D3DDevice7Vtbl* lpVtbl;
};

extern D3DDevice7* g_pD3DDevice;                        // 0x1008d988

#define D3DRENDERSTATE_ZWRITEENABLE          14         // 0x0e
#define D3DRENDERSTATE_ALPHABLENDENABLE      27         // 0x1b


// ======================================================================= //
//  THE ALLOCATOR INTERFACE
//
//  0x10009cc0 and 0x1007eb80 both take a pointer whose vtable slot +0x04 is
//  called with ONE pushed argument and %ecx holding the object -- an
//  ordinary virtual __thiscall. Slot +0x00 is never touched here and is only
//  present to put Alloc at +0x04.
// ======================================================================= //

class IAllocator
{
public:
	virtual void* Slot00() = 0;                         // +0x00, unused here
	virtual void* Alloc(uint32 nBytes) = 0;             // +0x04
};


// ======================================================================= //
//  0x1000a690   4 bytes of body, then 11 bytes of icl lea-nop padding.
//               FIFTY-EIGHT direct callers -- the most-called function in
//               the whole listing.
//
//      push %esi ; mov (%ecx),%eax ; add $0x4,%esp ; ret
//
//  WHAT PINS IT: a bare `ret` with %ecx read and never loaded is __thiscall
//  with no arguments, and the whole body is one load of the object's first
//  dword. The `push %esi` / `add $0x4,%esp` pair is icl's dead frame slot --
//  %esi is never read or written between them.
//
//  ITS NEIGHBOURS SETTLE THE CLASS. 0x1000a660/0x1000a670/0x1000a680 are the
//  other three accessors of the same eight-byte object, in the same shape:
//      0x1000a660  mov 0x8(%esp),%eax ; mov %eax,0x4(%ecx) ; ret $0x4
//      0x1000a670  mov 0x4(%ecx),%eax ; ret
//      0x1000a680  mov 0x8(%esp),%eax ; mov %eax,(%ecx)    ; ret $0x4
//  So the class is exactly two dwords with a get/set pair on each, and this
//  is the getter for the first.
//
//  0x10009c90 CALLS it rather than inlining it. icl inlines across the
//  whole TU whatever the definition order, so that caller now lives in
//  d3dbatchc_o2.cpp, where only the declaration is visible.
// ======================================================================= //

class C1000a690
{
public:
	uint32 sub_1000a690();                              // 0x1000a690  DefaultCache::GetCacheSize

	uint32 m_n00;                                       // +0x00  m_CacheSize
	uint32 m_n04;                                       // +0x04  m_WantedCache
};


// ======================================================================= //
//  0x1005f780   4 bytes of body, then padding. Nine callers.
//
//      push %esi ; mov 0x8(%ecx),%eax ; add $0x4,%esp ; ret
//
//  The same shape as 0x1000a690 one field along: __thiscall, no arguments,
//  returns the dword at +0x08.
// ======================================================================= //

class C1005f780
{
public:
	uint32 sub_1005f780();                              // 0x1005f780

	uint32 m_n00;                                       // +0x00
	uint32 m_n04;                                       // +0x04
	uint32 m_n08;                                       // +0x08
};

uint32 C1005f780::sub_1005f780()
{
	return m_n08;
}


// ======================================================================= //
//  0x1004b630   11 bytes of body, then five 0x90 pads. Three callers.
//
//      push %esi ; mov 0x28(%ecx),%ecx ; mov 0x34(%ecx),%eax
//      add $0x4,%esp ; ret
//
//  WHAT PINS IT: bare `ret`, %ecx read and never loaded -- __thiscall, no
//  arguments. TWO loads, and the second is indexed off the FIRST's result,
//  so this is one hop through a stored pointer at +0x28 and then its +0x34.
//  Note %ecx is reused as the intermediate, which is what makes the body
//  eleven bytes rather than thirteen.
// ======================================================================= //

class C1004b630Target
{
public:
	uint8  m_Reserved00[0x34];                          // +0x00..+0x33
	uint32 m_n34;                                       // +0x34
};

class C1004b630
{
public:
	uint32 sub_1004b630();                              // 0x1004b630

	uint8            m_Reserved00[0x28];                // +0x00..+0x27
	C1004b630Target* m_p28;                             // +0x28
};

uint32 C1004b630::sub_1004b630()
{
	return m_p28->m_n34;
}


// ======================================================================= //
//  0x100158b0   11 bytes of body, then five 0x90 pads. Sixteen callers.
//
//      push %esi ; lea 0x18c(%ecx),%eax ; add $0x4,%esp ; ret
//
//  WHAT PINS IT: `lea`, not `mov` -- the answer is the ADDRESS of a member
//  at +0x18c, not its contents. Bare `ret` with %ecx never loaded is
//  __thiscall with no arguments.
//
//  WHERE THE ANSWER GOES: 0x10017c7f loads `this` from another object's
//  +0x04, calls this, and passes the result as the first of two pointer
//  arguments to 0x10015770. So it hands out a pointer to an interior
//  sub-object; its type is not established here and the member is left
//  untyped on purpose.
//
//  ITS NEIGHBOUR 0x100158c0 is the same function with `mov 0x1dc(%ecx),%eax`
//  -- a getter for a different field -- which is a second confirmation that
//  this family of one-line accessors is what this class is made of.
// ======================================================================= //

class C100158b0
{
public:
	uint32* sub_100158b0();                             // 0x100158b0

	uint8  m_Reserved00[0x18c];                         // +0x000..+0x18b
	uint32 m_n18c;                                      // +0x18c
};

uint32* C100158b0::sub_100158b0()
{
	return &m_n18c;
}


// ======================================================================= //
//  0x1001c790   14 bytes of body, then twelve 0x90 pads. Four callers.
//  0x10009ef0   14 bytes of body, then padding.         Three callers.
//
//  THE TWO ARE THE SAME FUNCTION with one literal changed:
//
//      push %esi
//      mov 0x8(%esp),%eax          <- the one argument
//      shl $0x6,%eax               <- 0x1001c790; 0x10009ef0 uses shl $0x5
//      add 0x4(%ecx),%eax
//      add $0x4,%esp
//      ret $0x4
//
//  WHAT PINS THEM:
//   * `ret $0x4` with %ecx read and never loaded: __thiscall, one argument.
//   * the index is scaled FIRST and the base added SECOND, and the base is
//     read from +0x04 -- the same +0x04-is-the-table-base layout d3dlow.cpp
//     already established for 0x1001c480's class.
//   * the shift is by 6 and by 5, so the element widths are 0x40 and 0x20 --
//     and 0x40 and 0x20 are exactly two of the three widths d3dlow.cpp found
//     in the 0x1001c480 / 0x1001cf60 search family. These are the same two
//     element types, indexed instead of searched.
//
//  Written as `&m_pElements[nIndex]` rather than as pointer arithmetic on a
//  byte pointer, because a byte-pointer spelling would have to say `* 0x40`
//  and that is a constant this file would then be inventing; the element
//  width is a fact about the type.
// ======================================================================= //

struct Elem1001c790                                     // 0x40 bytes
{
	uint8 m_Reserved00[0x40];
};

class C1001c790
{
public:
	Elem1001c790* sub_1001c790(uint32 nIndex);          // 0x1001c790

	uint32         m_n00;                               // +0x00
	Elem1001c790*  m_pElements;                         // +0x04
};

Elem1001c790* C1001c790::sub_1001c790(uint32 nIndex)
{
	return &m_pElements[nIndex];
}


// ======================================================================= //
//  THE VECTOR PAIR -- 0x10015640 and 0x1004b640
//
//  Both are LTVector methods; both take their operand BY VALUE, which is
//  what the SDK's own `_CVector<T>` does (`operator - (const _CVector<T> v)`
//  in src/LT2/sdk/inc/ltvector.h, note: no reference). The local struct below
//  stands in for _CVector<float> so that the method names can carry their
//  addresses, which is how a symbol is mapped to its address.
// ======================================================================= //

struct Vec3
{
	Vec3() {}
	Vec3(float mx, float my, float mz) { x = mx; y = my; z = mz; }

	// 0x10015640 -- 66 bytes, then twelve 0x90 pads. 26 callers.
	Vec3 sub_10015640(const Vec3 v) const;

	// 0x1004b640 -- 34 bytes, then padding. Six callers.
	void sub_1004b640(const Vec3 v);

	float x;
	float y;
	float z;
};


// ----------------------------------------------------------------------- //
//  0x10015640
//
//      push edi ; push esi ; push ebx
//      mov 0x10(%esp),%ebx          <- FIRST stack slot
//      sub $0x10,%esp
//      flds (%ecx)   ; fsubs 0x24(%esp)
//      flds 0x4(%ecx); fsubs 0x28(%esp)
//      fxch %st(1)
//      fstps 0x4(%esp)              <- x into the temp
//      fstps 0x8(%esp)              <- y
//      flds 0x8(%ecx); fsubs 0x2c(%esp)
//      lea 0x4(%esp),%esi ; mov %ebx,%edi
//      fstps 0xc(%esp)              <- z
//      mov $0x3,%ecx ; rep movsl
//      add $0x10,%esp ; mov %ebx,%eax ; pop ... ; ret $0x10
//
//  WHAT PINS IT:
//   * `ret $0x10` is sixteen bytes of arguments and there are only three
//     float loads from the stack (0x24, 0x28, 0x2c after the 0x10 frame,
//     i.e. entry+8, +0xc, +0x10). The FOURTH dword -- entry+4, taken into
//     %ebx before the frame opens -- is therefore the hidden return
//     pointer, and %eax is set to it on the way out. That is __thiscall
//     returning a 12-byte struct by value: %ecx this, then the hidden
//     pointer, then the by-value operand.
//   * `this` is the LEFT operand: the subtraction is `flds (%ecx)` minus the
//     stack, not the other way round.
//   * the answer is built in a stack temporary at 0x4(%esp) and only then
//     `rep movsl`-ed three dwords into the caller's slot. icl 4.0 does not
//     construct the returned temporary directly into the return slot, so
//     the copy is real and has to be in the source shape too.
//   * the store order is x, y, z -- the `fxch %st(1)` exists precisely to
//     undo the x87 stack's reversal so that x lands first.
// ----------------------------------------------------------------------- //
Vec3 Vec3::sub_10015640(const Vec3 v) const
{
	return Vec3(x - v.x, y - v.y, z - v.z);
}


// ----------------------------------------------------------------------- //
//  0x1004b640
//
//      push %esi
//      flds (%ecx)   ; fadds 0x8(%esp)
//      flds 0x4(%ecx); fadds 0xc(%esp)
//      fxch %st(1)
//      fstps (%ecx) ; fstps 0x4(%ecx)
//      flds 0x8(%ecx); fadds 0x10(%esp) ; fstps 0x8(%ecx)
//      add $0x4,%esp ; ret $0xc
//
//  WHAT PINS IT: `ret $0xc` is exactly the three floats and there is no
//  hidden pointer, so nothing is returned -- an in-place `+=`. The results
//  go back into %ecx's own three floats, x then y then z, with the same
//  `fxch` fix-up as above. The `push %esi` / `add $0x4,%esp` slot is dead.
// ----------------------------------------------------------------------- //
void Vec3::sub_1004b640(const Vec3 v)
{
	x += v.x;
	y += v.y;
	z += v.z;
}


// ======================================================================= //
//  0x1007f3c0 / 0x1001d7e0 / 0x1002ec30   36 bytes each, then padding.
//                                          4, 3 and 3 callers.
//
//  THE THREE ARE THE SAME FUNCTION over three different classes -- byte for
//  byte identical apart from the call target:
//
//      push %ebp ; mov %esp,%ebp ; sub $0x10,%esp
//      add $0xfffffff8,%esp
//      mov 0x8(%ebp),%eax
//      movl $0x0,(%esp)             <- first argument, the constant 0
//      mov %eax,0x4(%esp)           <- second argument, ours
//      mov %ecx,%eax ; mov %eax,%ecx
//      call <0x1007eea0 | 0x1001d2a0 | 0x1002e710>
//      mov %ebp,%esp ; pop %ebp ; ret $0x4
//
//  WHAT PINS THEM:
//   * `ret $0x4` with %ecx never loaded from the stack: __thiscall, one
//     argument.
//   * the callee is entered with %ecx unchanged and its outgoing area is NOT
//     restored afterwards, so it too is __thiscall on the same object and
//     pops its own two arguments.
//   * the callee's answer is left in %eax and returned untouched.
//   * the constant zero is the callee's FIRST argument -- lowest address in
//     the outgoing area -- and our argument is its second.
//
//  WHAT THEY MEAN. 0x1007eea0 opens `mov 0x8(%ebp),%esi ; cmp 0x8(%ebx),%esi`
//  -- it compares its first argument against the object's element count at
//  +0x08 -- and on a change it calls the array allocator 0x1007eb80 and
//  stores the block at +0x04. It is SetSize(n, allocator). So each of these
//  three is that array's Term(): set the size to zero.
//
//  The `mov %ecx,%eax ; mov %eax,%ecx` round trip is icl's, not the
//  source's; the same pointless pair shows up at 0x10032a0c and 0x10032a18.
// ======================================================================= //

class C1007f3c0
{
public:
	LTBOOL sub_1007f3c0(void* pAllocator);              // 0x1007f3c0
	LTBOOL sub_1007eea0(uint32 nNewSize, void* pAllocator);
};

LTBOOL C1007f3c0::sub_1007f3c0(void* pAllocator)
{
	return sub_1007eea0(0, pAllocator);
}

class C1001d7e0
{
public:
	LTBOOL sub_1001d7e0(void* pAllocator);              // 0x1001d7e0
	LTBOOL sub_1001d2a0(uint32 nNewSize, void* pAllocator);
};

LTBOOL C1001d7e0::sub_1001d7e0(void* pAllocator)
{
	return sub_1001d2a0(0, pAllocator);
}

class C1002ec30
{
public:
	LTBOOL sub_1002ec30(void* pAllocator);              // 0x1002ec30
	LTBOOL sub_1002e710(uint32 nNewSize, void* pAllocator);
};

LTBOOL C1002ec30::sub_1002ec30(void* pAllocator)
{
	return sub_1002e710(0, pAllocator);
}


// ======================================================================= //
//  0x1007eb80   84 bytes, /Os (the next function starts unaligned). Four callers, one
//               of which is 0x1007eea0 -- the SetSize whose Term() is
//               0x1007f3c0 above.
//
//      mov 0x8(%ebp),%esi          <- nCount
//      mov 0xc(%ebp),%ecx          <- allocator
//      mov %esi,%ebx               <- the loop counter is a second copy
//      mov %esi,%eax ; shl $0x4,%eax ; push %eax
//      mov (%ecx),%eax ; mov 0x4(%eax),%eax ; call *%eax
//      mov %eax,%ecx ; test %ecx,%ecx ; je -> return 0
//      mov %ecx,%eax                       <- the running element pointer
//      test %esi,%esi ; je -> return the block
//      xor %edx,%edx
//   L: dec %ebx
//      test %eax,%eax ; jne -> store four zero dwords at 0,4,8,0xc
//      add $0x10,%eax
//      test %ebx,%ebx ; jne L
//      return the block
//
//  WHAT IT IS: CMoArray<T>::_AllocateTArray(nElements, pAlloc) for a
//  sixteen-byte T whose constructor zeroes four dwords -- stdlith's
//  dynarray.h member, with l_allocator.h's inline BaseNew (the LNew_Array
//  macro) expanded inside it. Written here in that shape:
//   * it is __thiscall with an unused `this`: the caller 0x1007eee4 does
//     `mov %ebx,%ecx` (the array) before the call, and `ret $0x8` pops the
//     two stack arguments. As a free __stdcall function the frame came out
//     as a 4-byte `push %esi` slot; as a member it is the shipped
//     `sub $0xc,%esp`.
//   * the `mov %esi,%ebx` BEFORE the Alloc call is BaseNew's by-value
//     nElements parameter: the inlined copy is the counter, the original
//     feeds the size and the guard.
//   * the counter is decremented at the TOP (`dec %ebx` before the element
//     test) and tested at the bottom: BaseNew's `while (n) { --n; ... }`.
//   * the per-element `test %eax,%eax` is the placement new's own null test.
//   * the `mov %ecx,%edx ; mov %edx,%eax` tail and the `xor %edx,%edx`
//     null arm are the inlined BaseNew's two returns merging into the
//     wrapper's tPtr.
//
//  IDENTICAL.  The residual was BLOCK PLACEMENT: the shipped image puts
//  the four constructor stores OUT OF LINE after the `ret` (`jne` to them,
//  `jmp` back to the `add $0x10`), and every spelling of the placement new
//  (if, if/else, ternary, continue, explicit operator-new call, every
//  constructor form, a template BaseNew, /Os with /Ob1, /Ob2, /Oi-, /Qrcd,
//  /Oa, /Ow, /G6, /Qip, /O1) left them as the fall-through of a `je`.  The
//  lever that closed 0x1007ebe0 and 0x1004e360 is what does it: icl sinks
//  the non-zero arm of a `switch` out of line.  The per-element null test
//  written as `switch ((uint32)pTemp) { case 0: break; default: <stores> }`
//  is byte-identical.  Keeping the placement new inside the switch arm
//  tests the pointer twice (96 bytes); an inline Clear() member in the arm
//  re-forms the address through `and` (91).  The stores are written out.
// ======================================================================= //

struct Elem1007eb80                                     // 0x10 bytes
{
	uint32 m_n00;                                       // +0x00
	uint32 m_n04;                                       // +0x04
	uint32 m_n08;                                       // +0x08
	uint32 m_n0c;                                       // +0x0c
};

inline Elem1007eb80* BaseNew1007eb80(IAllocator* pAlloc, Elem1007eb80* pDummy,
                                     uint32 nElements)
{
	Elem1007eb80 *ptr, *pTemp;

	ptr = (Elem1007eb80*)pAlloc->Alloc(sizeof(Elem1007eb80) * nElements);
	if (!ptr)
		return LTNULL;

	pTemp = ptr;
	while (nElements)
	{
		--nElements;
		switch ((uint32)pTemp)
		{
			case 0: break;
			default:
				pTemp->m_n00 = 0;
				pTemp->m_n04 = 0;
				pTemp->m_n08 = 0;
				pTemp->m_n0c = 0;
		}
		++pTemp;
	}

	return ptr;
}

class Arr1007eb80
{
public:
	Elem1007eb80* sub_1007eb80(uint32 nElements, IAllocator* pAlloc);
};

Elem1007eb80* Arr1007eb80::sub_1007eb80(uint32 nElements, IAllocator* pAlloc)
{
	Elem1007eb80* tPtr = BaseNew1007eb80(pAlloc, (Elem1007eb80*)0, nElements);

	return tPtr;
}



// ======================================================================= //
//  0x10032a00   79 bytes: an eight-byte stack-ABI adapter at 0x10032a00
//               falling straight into the register-ABI body at 0x10032a08.
//               Three callers, and ALL THREE call 0x10032a08 -- nothing in
//               the image calls 0x10032a00 at all.
//
//      10032a00  mov 0x4(%esp),%eax        <- adapter: arg1 into %eax
//      10032a04  mov 0x8(%esp),%edx        <-          arg2 into %edx
//      10032a08  push %ebx ; sub $0x8,%esp
//                ... the real body, reading %eax and %edx ...
//
//  This is the icl dual entry. The function is named
//  after the ADAPTER, 0x10032a00, because that is the address an external
//  caller would link against and therefore the address the public symbol
//  must carry; the in-file callers reach the register entry through a local
//  label. If this comes back mismatched by exactly eight bytes of shift, the
//  fix is to rename it sub_10032a08 -- the body below does not change.
//
//  WHAT IT DOES: "is console variable <name> set to <value>?", case
//  insensitively.
//
//  WHAT PINS IT:
//   * 0x10032a50 loads %esi with "ForceMode" (0x100a8b80) once and calls this
//     three times with %edx = "PowerVR", "Permedia2" and "Rage128". So the
//     first argument is a variable NAME and the second a candidate value.
//   * `call *0x18(%eax)` on 0x1008cbcc with the name, answer null-tested.
//   * `call *0x20(%eax)` with that handle, answer handed to 0x100874b0 --
//     which is the CRT's _stricmp (it indexes the locale table at
//     0x10091ed4 and runs the `or %al,%al` / `cmp %al,%ah` byte loop).
//   * the compare's own answer is tested against zero, so the result is
//     "the strings are EQUAL", not the comparison value.
//   * the string from +0x20 is _stricmp's FIRST argument (it goes to
//     (%esp)); our value is the second, held in %ebx across the two calls.
//
//  THE ARM ORDER IS THE SOURCE SHAPE. Both failures reach 0x10032a3e and the
//  success is out of line at 0x10032a45, i.e. the false answer is the
//  fall-through and the true answer is the jump target. That is a leading
//  guarded `return LTFALSE`, then a `return LTTRUE` inside the second test,
//  then a trailing `return LTFALSE` -- the two false returns merge into the
//  one the third statement emits, and the true return gets pushed past it.
//  Writing it as `if (a && b) return LTTRUE; return LTFALSE;` puts the true
//  answer in the fall-through instead, which is the wrong way round.
// ======================================================================= //

// MOVED to w31_10032a00_qip.cpp beside its only caller,
// 0x10032a50, which enters it at the register entry 0x10032a08: /O2 /Qip
// makes the pair byte-identical.


// ======================================================================= //
//  0x1006a800   95 bytes of body, then one 0x90 pad. Five callers. /Os.
//
//  WHAT IT IS: a run-length decoder that paints a 1024-byte buffer with
//  alternating runs of 0x00 and 0xFF -- a one-bit mask, unpacked.
//
//      push edi ; push esi ; push ebp ; push ebx
//      mov 0x18(%esp),%ebp         <- entry+8   : the source length
//      mov 0x14(%esp),%ebx         <- entry+4   : the source
//      push %esi                   <- FOUR BYTES OF LOCAL, and it is USED
//      xor %edi,%edi
//      movb $0xff,0x1(%esp)
//      movb $0x0,(%esp)
//      mov 0x20(%esp),%edx         <- entry+0xc : the destination
//      test %ebp,%ebp
//      lea 0x400(%edx),%esi        <- the limit, dest + 1024
//      je -> return 1
//   L: movzbl (%ebx),%ecx ; inc %ebx ; dec %ebp
//      mov %edx,%eax ; add %ecx,%eax
//      cmp %eax,%esi ; jae ok
//      -> return 0
//  ok: mov (%esp,%edi,1),%al
//      test %ecx,%ecx ; je skip
//      f: mov %al,(%edx) ; inc %edx ; dec %ecx ; jne f
//  skip: xor %eax,%eax ; test %edi,%edi ; sete %al
//        test %ebp,%ebp ; mov %eax,%edi ; jne L
//      return 1
//
//  WHAT PINS IT:
//   * a bare `ret` with all three arguments read off %esp before any frame
//     is opened: __cdecl, three arguments, at entry+4, +8 and +0xc.
//   * the four-byte slot opened with `push %esi` is INDEXED --
//     `mov (%esp,%edi,1),%al` -- so it is a real two-entry array of bytes,
//     not icl's dead frame slot. This is the one place in this file where
//     that idiom is not padding.
//   * the two bytes are written 0xFF first at index 1, then 0 at index 0 --
//     and icl emits these two stores in the REVERSE of source order, so the
//     source assigns [0] first. Measured: written [1] then [0], the stores
//     come out 0 first, with the limit's `lea` scheduled between them.
//   * the destination is read from its stack slot AFTER the `push %esi`
//     (0x20(%esp)), not with the other two arguments in the prologue: the
//     running pointer is a LOCAL copy of the argument, and the limit is
//     taken from the argument itself. Writing through the parameter hoists
//     its load into the prologue with the others (18 bytes differ).
//   * `inc %ebx` (pSrc) precedes `dec %ebp` (nSrcLen): the same reversal,
//     so the source decrements the length first.
//   * %edi is the array index and is recomputed as `sete` of "was it zero" --
//     a strict toggle between 0 and 1, i.e. the runs alternate colour.
//   * `cmp %eax,%esi ; jae` is unsigned and is the LIMIT against
//     dest+run, so the overflow test is `pDst + nRun > pLimit` and it is
//     checked before anything is written.
//   * both inner and outer loops are guard-plus-do-while, which is a `while`
//     in the source, not a `do`; the run fill uses the run count itself as
//     its counter (%ecx is consumed by the fill).
//   * 0x400 is the destination's fixed capacity -- 32x32 bytes.
// ======================================================================= //

LTBOOL sub_1006a800(const uint8* pSrc, uint32 nSrcLen, uint8* pDst)
{
	uint8        nColors[2];
	uint32       nIndex;
	uint32       nRun;
	uint8        nColor;
	uint8*       pOut;
	const uint8* pLimit;

	nIndex = 0;
	nColors[0] = 0;
	nColors[1] = 0xFF;
	pOut = pDst;
	pLimit = pDst + 1024;

	while (nSrcLen != 0)
	{
		nRun = *pSrc;
		nSrcLen--;
		pSrc++;

		if (pOut + nRun > pLimit)
			return LTFALSE;

		nColor = nColors[nIndex];
		while (nRun != 0)
		{
			*pOut = nColor;
			pOut++;
			nRun--;
		}

		nIndex = (nIndex == 0);
	}

	return LTTRUE;
}



// ======================================================================= //
//  0x1000a690 is dynarray.h's DefaultCache::GetCacheSize (the class is
//  DefaultCache: m_CacheSize, m_WantedCache), out of line in the /Qip
//  translation units (w64_10009f10_qip.cpp, d3dr5b.cpp,
//  w7_100500a0_qip.cpp emit it, byte-identical; the shipped callers never
//  inline it).  It was also defined here, a second copy, left over and
//  unplaced; this spelling is now aliased onto dynarray.h's in
//  link/aliases.rsp.
// ======================================================================= //


// ======================================================================= //
//  0x1000a6a0   55 bytes of body (0x1000a6a0..0x1000a6d6), then nine 0x90
//               pads. Four callers.
//
//  WHAT IT IS: pop one node off a fixed-size pool's free list, growing the
//  pool once if the list is empty.
//
//      mov 0x4(%esp),%ecx          <- the ONLY argument, read before any
//      sub $0xc,%esp                  frame is opened
//      mov 0x18(%ecx),%eax
//      test %eax,%eax ; je refill
//   P: mov (%eax),%edx
//      add $0xc,%esp
//      mov %edx,0x18(%ecx)
//      ret                         <- %eax is still the node
//  refill:
//      mov %ecx,(%esp) ; mov 0x8(%ecx),%ecx ; mov %ecx,0x4(%esp)
//      call 0x1007fbf0
//      mov 0x10(%esp),%ecx         <- the argument is RELOADED
//      mov 0x18(%ecx),%eax
//      test %eax,%eax ; jne P
//      add $0xc,%esp ; xor %eax,%eax ; ret
//
//  WHAT PINS IT:
//   * a bare `ret` with the argument taken straight off 0x4(%esp): __cdecl,
//     one argument. 0x1000ef0b confirms it -- the call site writes a single
//     dword, the address of a static pool object at 0x1008d4e4, and uses the
//     answer immediately (`mov %ebp,(%eax)`), so the return value is the
//     node.
//   * +0x18 is the free-list head and the node's FIRST dword is its next
//     pointer: `mov (%eax),%edx ; mov %edx,0x18(%ecx)` is the pop.
//   * the outgoing area is opened once by the prologue and released by each
//     exit, so 0x1007fbf0 is __cdecl. It is passed (pool, pool->+0x08), and
//     its own body (`mov 0x4(%esi),%eax ; imul %edi,%eax ; add $0x8,%eax`
//     then 0x10021ce0, the renderer's allocator) shows +0x04 is the element
//     size and +0x08 the number of elements to grow by.
//   * THE ARM ORDER IS THE SOURCE SHAPE. The pop is the FALL-THROUGH of the
//     empty test and the refill sits out of line after the pop's `ret`,
//     jumping back into it. That is a leading `if (list is empty) { grow;
//     re-read; if (still empty) return NULL; }` with the pop as the
//     function's common tail -- not an if/else, and not a loop (the retry
//     happens exactly once; `jne P` targets the pop, not the test).
// ======================================================================= //

struct FreeNode1000a6a0
{
	FreeNode1000a6a0* m_pNext;                          // +0x00
};

struct Pool1000a6a0
{
	uint32            m_n00;                            // +0x00
	uint32            m_nElemSize04;                    // +0x04
	uint32            m_nGrowCount08;                   // +0x08
	uint8             m_Reserved0c[0x0c];               // +0x0c..+0x17
	FreeNode1000a6a0* m_pFree18;                        // +0x18
};

extern void sub_1007fbf0(Pool1000a6a0* pPool, uint32 nCount);

void* sub_1000a6a0(Pool1000a6a0* pPool)
{
	FreeNode1000a6a0* pNode;

	pNode = pPool->m_pFree18;
	if (pNode == LTNULL)
	{
		sub_1007fbf0(pPool, pPool->m_nGrowCount08);

		pNode = pPool->m_pFree18;
		if (pNode == LTNULL)
			return LTNULL;
	}

	pPool->m_pFree18 = pNode->m_pNext;
	return pNode;
}
