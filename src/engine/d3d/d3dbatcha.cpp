// *********************************************************************** //
//
//  MODULE:   BATCH_A (.CPP)  --  d3d.ren, the 25 highest-leverage rows
//
//  PURPOSE:  Reconstructed source for the batch_a rows.  d3d.ren was
//            built with Intel C/C++ 4.0 at /O2, which reproduces it, so
//            every fingerprint below (frame pointer, dead 4-byte stack slot
//            opened with `push %esi` and closed with `add $0x4,%esp`,
//            `xor %eax,%eax; inc %eax` for 1, `add $0xfffffff4,%esp` for the
//            outgoing-argument area, lea-nop padding) is what icl emits and
//            is NOT something the source has to spell.
//
//  ONE FLAG THAT DOES CONSTRAIN THE SOURCE: icl's /O2 is /Gfsy /Ob1gyti, and
//  /Ob1 inlines ONLY functions the source marks inline.  A member function
//  defined inside its class body is implicitly inline and WOULD be inlined;
//  every accessor below is therefore defined OUT OF LINE, which is what
//  keeps 0x10021a80's `call 0x1000a690` and 0x1001cf00's `call 0x1000a680`
//  as calls instead of folding them away.
//
//  WHAT IS HERE:  19 functions.  17 of the 25 listed rows, plus
//  sub_1000a690 (not on the list but the callee 0x10021a80 needs, and free
//  once its class exists).  Six rows are NOT here and the reason for each is
//  at the bottom of this file -- three of them are register-ABI entries that
//  no C source can produce in isolation, and three are large bodies not
//  fully read.
//
//  Every offset, constant and branch below was read off the retail d3d.ren.
//
// *********************************************************************** //

#include "ltbasedefs.h"


// ----------------------------------------------------------------------- //
//  Interfaces these bodies reach through.
// ----------------------------------------------------------------------- //

// The Direct3D 7 device at 0x1008d988, the same object d3ddraw.cpp and
// d3dblit.cpp model.  Four slots are used here and all four are already
// pinned by the project: +0x50 SetRenderState, +0x54 GetRenderState,
// +0x8c SetTexture, +0x94 SetTextureStageState.
struct D3DDevice;

struct D3DDeviceVtbl
{
	void* _pad_00[20];                                  // +0x00 .. +0x4c
	long (__stdcall* SetRenderState)(D3DDevice*, uint32, uint32);        // +0x50
	long (__stdcall* GetRenderState)(D3DDevice*, uint32, uint32*);       // +0x54
	void* _pad_58[13];                                  // +0x58 .. +0x88
	long (__stdcall* SetTexture)(D3DDevice*, uint32, void*);             // +0x8c
	void* _pad_90;                                      // +0x90
	long (__stdcall* SetTextureStageState)(D3DDevice*, uint32, uint32,
	                                       uint32);     // +0x94
};

struct D3DDevice
{
	D3DDeviceVtbl* lpVtbl;
};

extern D3DDevice* g_pD3DDevice;                         // 0x1008d988


// DirectDraw, as d3dblit.cpp already names the globals.  0x100210d0 calls
// +0x08 Release, +0x4c RestoreDisplayMode and +0x50 SetCooperativeLevel --
// which is exactly IDirectDraw7's layout, and is what identifies that
// function as the DirectDraw teardown.
struct DDSurface;
struct DDSurfaceVtbl
{
	void* _pad_00[2];                                   // +0x00
	uint32 (__stdcall* Release)(DDSurface*);            // +0x08
};
struct DDSurface { DDSurfaceVtbl* lpVtbl; };

struct DDClipper;
struct DDClipperVtbl
{
	void* _pad_00[2];                                   // +0x00
	uint32 (__stdcall* Release)(DDClipper*);            // +0x08
};
struct DDClipper { DDClipperVtbl* lpVtbl; };

struct DDraw;
struct DDrawVtbl
{
	void* _pad_00[2];                                   // +0x00
	uint32 (__stdcall* Release)(DDraw*);                // +0x08
	void* _pad_0c[16];                                  // +0x0c .. +0x48
	long (__stdcall* RestoreDisplayMode)(DDraw*);       // +0x4c
	long (__stdcall* SetCooperativeLevel)(DDraw*, void*, uint32);        // +0x50
};
struct DDraw { DDrawVtbl* lpVtbl; };

extern DDraw*     g_pDirectDraw;                        // 0x1008bf74
extern DDSurface* g_pOffscreen;                         // 0x1008bf68
extern DDSurface* g_pPrimary;                           // 0x1008bf6c
extern DDSurface* g_pWindowedPrimary;                   // 0x1008bf70
extern DDClipper* g_pClipper;                           // 0x1008d97c


// The renderer's allocator.  Three call sites here go through it and all
// three do it the SAME way: the object into %ecx, one argument pushed, the
// vptr loaded from (%ecx), the slot loaded out of the vtable into a register
// and `call *%eax` -- callee-pops, `this` in %ecx.  That is a C++ virtual
// call, and it is a different shape from the C-style COM vtable calls in the
// same module (`mov (%eax),%eax; call *0x50(%eax)`), which is why this is
// spelled with `virtual` and the device is not.
//   +0x04  Alloc(size)   (0x1001ced0 passes count*0x20)
//   +0x08  Free(p)       (0x1001cf00 / 0x1007ebe0)
class CD3DAlloc
{
public:
	virtual void  Slot00();                             // vtbl +0x00
	virtual void* Alloc(uint32 nSize);                  // vtbl +0x04
	virtual void  Free(void* pData);                    // vtbl +0x08
};


// Neighbours, declared only -- defining them here would let /Ob1 do nothing
// but it would also invent bytes at addresses this file is not claiming.
extern "C" __declspec(dllimport) int __stdcall ShowCursor(int bShow);

extern void sub_10032c30();                             // 0x10032c30
extern void sub_1000a8a0(uint32 nA, uint32 nB, uint32 nC, uint32 nD);
                                                        // 0x1000a8a0

extern uint32 g_n1008ee00;                              // 0x1008ee00
extern uint32 g_n1008e688;                              // 0x1008e688


// ======================================================================= //
//  0x1000a680   13 bytes + 3 of padding      76 CALLERS
//  0x1000a690    7 bytes                     (not on the listing; the
//                                             callee 0x10021a80 needs)
//
//  DECODED.  The most-reached function in the module and it is four
//  instructions:
//
//      1000a680  56              push %esi          <- icl's dead slot
//      1000a681  8b 44 24 08     mov 0x8(%esp),%eax <- the one argument
//      1000a685  89 01           mov %eax,(%ecx)    <- through `this`
//      1000a687  83 c4 04        add $0x4,%esp
//      1000a68a  c2 04 00        ret $0x4
//
//  WHAT PINS IT: `ret $0x4` with %ecx never loaded is __thiscall with one
//  dword argument, and the store is to (%ecx) with no displacement, so the
//  object is four bytes wide and the value lives at its start.  0x1000a690
//  is the same object's reader -- `mov (%ecx),%eax`, no arguments, plain
//  `ret`.  Both are reached by CALL from inside the module (0x10021a91,
//  0x1001cf32, 0x1007ebfe), never inlined, which is /Ob1 doing what /Ob1
//  does to a member defined outside its class body.
//
//  The class is dynarray.h's DefaultCache (m_CacheSize at +0x00): these are
//  its SetCacheSize and GetCacheSize, out of line in the /Qip translation
//  units (w64_10009f10_qip.cpp, d3dr5b.cpp, w7_100500a0_qip.cpp emit them,
//  byte-identical).  They were also defined here, a second copy of each, and
//  0x1000a690's was left over and unplaced; now this spelling only declares
//  them, aliased onto dynarray.h's in link/aliases.rsp.
// ======================================================================= //

class CValue1000a680
{
public:
	void   sub_1000a680(uint32 nValue);                 // DefaultCache::SetCacheSize
	uint32 sub_1000a690();                              // DefaultCache::GetCacheSize

	uint32 m_nValue00;                                  // +0x00  m_CacheSize
};


// ======================================================================= //
//  0x10021a80   30 bytes (listing says 32; the last two are `8b f6`)
//               20 callers
//
//  DECODED.  Framed, __thiscall, no arguments:
//
//      mov %ecx,%ebx
//      mov $0xc,%ecx / add %ebx,%ecx      <- &this->m_Value0c, icl's
//                                            two-instruction spelling of lea
//      call 0x1000a690                    <- the reader above
//      add 0x8(%ebx),%eax
//
//  WHAT PINS IT: the callee is the +0x00 reader, invoked on `this + 0x0c`,
//  so the object at +0x0c is a CValue1000a680; the answer is that value plus
//  the dword at +0x08, added IN THAT ORDER (the call result is in %eax and
//  %eax is the destination of the `add`).  The listing tags this x87; there
//  is no floating point in it at all -- both terms are integers.
// ======================================================================= //

class C10021a80
{
public:
	uint32 sub_10021a80();

	uint32         m_nUnknown00;                        // +0x00
	uint32         m_nUnknown04;                        // +0x04
	uint32         m_nBase08;                           // +0x08
	CValue1000a680 m_Value0c;                           // +0x0c
};

uint32 C10021a80::sub_10021a80()
{
	return m_Value0c.sub_1000a690() + m_nBase08;
}


// ======================================================================= //
//  0x1001ced0   41 bytes (listing says 44; three are `8d 76 00`)
//               4 callers
//
//  DECODED.  Two arguments, __thiscall by its `ret $0x8`, and `this` is
//  never touched -- a dead parameter, which is normal.
//
//      mov 0x8(%ebp),%eax / shl $0x5,%eax   <- argument 0 times 32
//      mov 0xc(%ebp),%ecx                   <- argument 1, the allocator
//      push %eax / mov (%ecx),%eax / mov 0x4(%eax),%eax / call *%eax
//      test %eax,%eax / je -> edx = 0
//      mov %eax,%edx
//      mov %edx,%eax / ret $0x8
//
//  WHAT PINS IT: the multiply by 0x20 is a `shl $5` on the first argument
//  and the product is the ONLY thing passed, so this allocates an array of
//  count elements of a 32-byte type.  It is a MEMBER and not a free
//  function: `ret $0x8` with two stack arguments would also fit __stdcall,
//  but both call sites load %ecx immediately before the call
//  (0x1001cd8b `mov -0x10(%ebp),%ecx`, 0x1001d2e4 `mov %ebx,%ecx`), so
//  `this` is really being passed and really is unused.  The shared exit sits at the FIRST arm
//  (the `mov %eax,%edx` for the success case), which by the shared-exit
//  lever is full else-nesting, so the two answers are written as an if/else
//  and not as a guarded return.
// ======================================================================= //

class C1001ced0
{
public:
	void* sub_1001ced0(uint32 nCount, CD3DAlloc* pAlloc);

	uint32 m_nUnknown00;                                // +0x00, never read
};

void* C1001ced0::sub_1001ced0(uint32 nCount, CD3DAlloc* pAlloc)
{
	void* pRet;
	void* p;

	p = pAlloc->Alloc(nCount * 32);

	if (p)
		pRet = p;
	else
		pRet = LTNULL;

	return pRet;
}


// ======================================================================= //
//  0x1001cf00   62 bytes + `8b f6`          16 callers
//  0x1007ebe0   64 bytes                     4 callers
//
//  DECODED, and the two are the SAME function compiled twice: instruction
//  for instruction identical, differing only in which block icl made the
//  fall-through.  0x1001cf00 jumps `je` over the release body to a tail
//  placed last; 0x1007ebe0 jumps `jne` to a release body placed last and
//  falls into the tail, which costs it the one extra `jmp` -- 64 bytes
//  against 62.  Which source shape selects which layout is not pinned, so
//  both are written IDENTICALLY here: whichever layout icl picks, exactly
//  one of the two addresses must match, and the other one tells
//  which polarity to invert.
//
//  WHAT PINS THE BODY:
//    * `ret $0x8` with `this` in %ecx: __thiscall, two arguments, and the
//      second is never read -- another dead parameter.
//    * `test %eax,%eax; je X; je Y` -- TWO conditional jumps on one flags
//      value, the second unreachable.  icl kept a dead branch, which is what
//      a source-level double test of the same pointer looks like; it is
//      written that way below deliberately.  Do not "fix" it.
//    * the release is `pAlloc->Free(m_pData04)` -- object into %ecx, the
//      pointer pushed, vtable slot +0x08.
//    * the tail is unconditional and is the +0x0c value's setter with a
//      literal 0: `m_Count0c.sub_1000a680(0)`.
// ======================================================================= //

class C1001cf00
{
public:
	void sub_1001cf00(CD3DAlloc* pAlloc, uint32 nUnused);

	uint32         m_nUnknown00;                        // +0x00
	void*          m_pData04;                           // +0x04
	uint32         m_nUnknown08;                        // +0x08
	CValue1000a680 m_Count0c;                           // +0x0c
};

void C1001cf00::sub_1001cf00(CD3DAlloc* pAlloc, uint32 nUnused)
{
	if (m_pData04)
	{
		if (m_pData04)
			pAlloc->Free(m_pData04);

		m_pData04 = LTNULL;
	}

	m_Count0c.sub_1000a680(0);
}


class C1007ebe0
{
public:
	void sub_1007ebe0(CD3DAlloc* pAlloc, uint32 nUnused);

	uint32         m_nUnknown00;                        // +0x00
	void*          m_pData04;                           // +0x04
	uint32         m_nUnknown08;                        // +0x08
	CValue1000a680 m_Count0c;                           // +0x0c
};

// The twin of C1001cf00's 0x1001cf00 -- identical instructions, differing
// only in which block icl left as the fall-through: here the release body
// is SUNK below the epilogue and jumps back.  Guard polarity is not it (an
// inverted early return is 74 bytes against 62), nor a while/for loop, a
// goto, an empty else, or an integer rather than pointer test -- all come
// out laid out as 0x1001cf00.  A `switch` is what icl sinks: its non-zero
// arm goes out of line exactly as shipped, and it is byte-identical.  The
// same lever closed 0x1003b690 (d3dmid.cpp) and moved 0x1004e360 below.
void C1007ebe0::sub_1007ebe0(CD3DAlloc* pAlloc, uint32 nUnused)
{
	switch ((uint32)m_pData04)
	{
		case 0: break;
		default:
			if (m_pData04)
				pAlloc->Free(m_pData04);
			m_pData04 = LTNULL;
	}
	m_Count0c.sub_1000a680(0);
}


// ======================================================================= //
//  0x1004e2f0   12 bytes + `8b f6`          35 CALLERS
//
//  DECODED.  __thiscall, one argument:
//
//      push %esi / mov 0x8(%esp),%eax / lea (%ecx,%eax,4),%eax
//      add $0x4,%esp / ret $0x4
//
//  WHAT PINS IT: the scale in the `lea` is 4 and the base is %ecx ITSELF,
//  with no load -- so the array is embedded at the object's own start, not
//  reached through a pointer.  That is what separates this from 0x10009d40
//  and 0x1007ee40 below, which load their base from +0x04 first.  No bounds
//  check, no null check: a raw subscript.
//
//  Indexing the one-element member array DIRECTLY (`&m_Array00[nIndex]` or
//  `m_Array00 + nIndex`) makes icl build the *4 as `mov; add; lea (,2)`
//  and add `this` last -- 20 bytes against 14.  Through a pointer to the
//  array's first element it is the single `lea (%ecx,%eax,4)` shipped has
//  (as is a cast of `this`); byte-identical.
// ======================================================================= //

class C1004e2f0
{
public:
	uint32* sub_1004e2f0(uint32 nIndex);

	uint32 m_Array00[1];                                // +0x00
};

uint32* C1004e2f0::sub_1004e2f0(uint32 nIndex)
{
	uint32* p = m_Array00;
	return &p[nIndex];
}


// ======================================================================= //
//  0x1004e360 / 0x1004e3d0 -- MOVED to w35_1004e360_os.cpp:
//  written there over the real dynarray.h (a CMoArray at +0x34), which
//  this file cannot include without changing three of its functions.
// ======================================================================= //


// ======================================================================= //
//  0x10009d40   17 bytes (listing says 32; the rest is padding)
//               4 callers
//
//  DECODED.  __thiscall subscript, element width 4, base loaded from +0x04:
//
//      push %esi / mov 0x4(%ecx),%ecx / mov 0x8(%esp),%eax
//      lea (%ecx,%eax,4),%eax / add $0x4,%esp / ret $0x4
//
//  WHAT PINS IT: the base is a LOAD from 0x4(%ecx), so the array is heap
//  and the object holds a pointer to it -- and the load is emitted BEFORE
//  the argument, which is the schedule a `pointer + index` (rather than
//  `index*n + pointer`) gives when no shift is needed.
// ======================================================================= //

class C10009d40
{
public:
	uint32* sub_10009d40(uint32 nIndex);

	uint32  m_nUnknown00;                               // +0x00
	uint32* m_pArray04;                                 // +0x04
};

uint32* C10009d40::sub_10009d40(uint32 nIndex)
{
	return m_pArray04 + nIndex;
}


// ======================================================================= //
//  0x1007ee40   15 bytes (listing says 20)   3 callers
//
//  DECODED.  Same subscript, element width 16:
//
//      push %esi / mov 0x8(%esp),%eax / shl $0x4,%eax
//      add 0x4(%ecx),%eax / add $0x4,%esp / ret $0x4
//
//  WHAT PINS IT: `shl $0x4` then `add <base>` -- the index is scaled first
//  and the base folded in as a memory operand, which is `p + i` on a
//  16-byte element type.  The base is at +0x04, the same slot 0x10009d40
//  and 0x10050790 use, so these three are the same container at three
//  instantiations.
// ======================================================================= //

struct S1007ee40Element                                 // 16 bytes
{
	uint8 m_Bytes[16];
};

class C1007ee40
{
public:
	S1007ee40Element* sub_1007ee40(uint32 nIndex);

	uint32             m_nUnknown00;                    // +0x00
	S1007ee40Element*  m_pArray04;                      // +0x04
};

S1007ee40Element* C1007ee40::sub_1007ee40(uint32 nIndex)
{
	return m_pArray04 + nIndex;
}


// ======================================================================= //
//  0x10050790   24 bytes (listing says 32)   3 callers
//
//  DECODED.  Same subscript, element width 12:
//
//      push %esi / mov 0x8(%esp),%eax
//      lea 0x0(,%eax,8),%edx / lea (%edx,%eax,4),%eax
//      add 0x4(%ecx),%eax / add $0x4,%esp / ret $0x4
//
//  WHAT PINS IT: i*8 + i*4 in two `lea`s is icl multiplying by 12, and the
//  base is again the pointer at +0x04.
// ======================================================================= //

struct S10050790Element                                 // 12 bytes
{
	uint8 m_Bytes[12];
};

class C10050790
{
public:
	S10050790Element* sub_10050790(uint32 nIndex);

	uint32             m_nUnknown00;                    // +0x00
	S10050790Element*  m_pArray04;                      // +0x04
};

S10050790Element* C10050790::sub_10050790(uint32 nIndex)
{
	return m_pArray04 + nIndex;
}


// ======================================================================= //
//  0x10021f10   45 bytes (listing says 48)   4 callers
//
//  DECODED.  A population count over 32 bits, unrolled twice by icl:
//
//      push %ebx / mov 0x8(%esp),%eax
//      xor %ebx,%ebx          <- the count
//      xor %edx,%edx          <- the loop variable
//    L: test $0x1,%al / je +1 / inc %ebx
//      inc %edx / shr $1,%eax
//      movzwl %dx,%ecx / cmp $0x20,%ecx / jge done
//      <the same six instructions again>
//      jl L
//      mov %ebx,%eax
//
//  WHAT PINS IT:
//    * `test $0x1,%al` on the low byte and a BRANCH around `inc` -- the
//      source tests the bit and increments, it does not add the bit;
//    * `movzwl %dx,%ecx` before every compare: the loop variable is SIXTEEN
//      bits wide, zero-extended and then compared SIGNED against 32, which
//      is exactly `uint16 i; i < 32` after integral promotion.  An `int`
//      counter needs no movzwl and the function would be shorter;
//    * no guard before the first iteration, so the trip count is a constant.
// ======================================================================= //

uint32 sub_10021f10(uint32 nBits)
{
	uint32 nCount;
	uint16 i;

	nCount = 0;

	for (i = 0; i < 32; i++)
	{
		if (nBits & 1)
			nCount++;

		nBits >>= 1;
	}

	return nCount;
}


// ======================================================================= //
//  0x10011420   43 bytes (size correct)       3 callers
//
//  DECODED.  A forwarder, __cdecl (plain `ret`, caller cleans), no frame
//  pointer -- one of the very few functions in the module without one:
//
//      sub $0x14,%esp
//      mov 0x18(%esp),%eax / mov 0x1c(%esp),%edx / mov 0x20(%esp),%ecx
//      mov %edx,0x4(%esp) / mov %ecx,0x8(%esp) / mov %eax,(%esp)
//      movl $0x0,0xc(%esp)
//      call 0x1000a8a0
//      add $0x14,%esp / ret
//
//  WHAT PINS IT: 0x14 of outgoing area for four dwords plus icl's alignment
//  slack, the three incoming arguments passed straight through in order and
//  a literal 0 as the fourth.  Nothing is returned -- %eax is left as the
//  callee left it and no caller reads it.
// ======================================================================= //

void sub_10011420(uint32 nA, uint32 nB, uint32 nC)
{
	sub_1000a8a0(nA, nB, nC, 0);
}


// ======================================================================= //
//  0x10079f84 -- MOVED to w29_10073c00_osoiqip.cpp as
//  C10079f84::sub_10079f80, PFormat::IsSame: it is the register entry of a
//  /Qip two-entry pair (shim 0x10079f80), and all eight callers are the
//  pixel converters in that TU.  Identical there, shim included.
// ======================================================================= //


// ======================================================================= //
//  0x100115b0   99 bytes (size correct)       6 callers
//
//  DECODED.  Turns texture stage 1 off:
//
//      xor %eax,%eax / inc %eax               <- the constant 1, CSEd
//      mov 0x1008d988,%edx
//      mov %eax,0xc(%esp) / mov %eax,0x8(%esp) / mov %eax,0x4(%esp)
//      mov (%edx),%eax / mov %edx,(%esp) / call *0x94(%eax)
//      mov 0x1008e688,%eax / test %eax,%eax / je end
//      mov 0x1008d988,%eax                    <- RELOADED, a second read
//      movl $0x0,0x8(%esp) / mov %eax,(%esp) / movl $0x1,0x4(%esp)
//      mov (%eax),%eax / call *0x8c(%eax)
//      movl $0x0,0x1008e688
//
//  WHAT PINS IT: slot +0x94 with four arguments (device, 1, 1, 1) is
//  SetTextureStageState(stage 1, D3DTSS_COLOROP, D3DTOP_DISABLE); slot +0x8c
//  with three (device, 1, NULL) is SetTexture(stage 1, none).  The device is
//  loaded once for the first call and RELOADED for the second, which is what
//  two separate statements reading the same global give.  0x1008e688 is the
//  flag saying stage 1 currently has a texture bound, cleared here.
//  The `1`s in the first call are one CSEd `xor/inc`; the `1` in the second
//  is a literal `movl $0x1` -- a different expression, so a different
//  constant.
// ======================================================================= //

void sub_100115b0()
{
	g_pD3DDevice->lpVtbl->SetTextureStageState(g_pD3DDevice, 1, 1, 1);

	if (g_n1008e688)
	{
		g_pD3DDevice->lpVtbl->SetTexture(g_pD3DDevice, 1, LTNULL);
		g_n1008e688 = 0;
	}
}


// ======================================================================= //
//  0x1000adc0   85 bytes (listing says 88)   10 callers
//
//  DECODED.  A scoped render-state saver's constructor:
//
//      mov %ecx,%ebx / mov 0x8(%ebp),%eax
//      mov $0x4,%edx / add %ebx,%edx          <- &this->m_nOld04
//      mov %eax,(%ebx)                        <- m_nState00 = argument 0
//      mov 0x1008d988,%ecx
//      (device, argument 0, &m_nOld04) -> call *0x54(%ecx)
//      mov 0xc(%ebp),%eax / mov 0x1008d988,%edx / mov (%ebx),%ecx
//      (device, m_nState00, argument 1) -> call *0x50(%eax)
//      mov %ebx,%eax                          <- answers `this`
//      ret $0x8
//
//  WHAT PINS IT: +0x54 GetRenderState writes through a pointer to `this+4`,
//  +0x50 SetRenderState is handed the new value; the object is eight bytes,
//  the state id at +0x00 and the saved value at +0x04.  Answering `this`
//  through %eax is a constructor's return, but it is written as an ordinary
//  member here because the harness maps functions by the address in their
//  name and a constructor's mangled name has no room for one.
//  Note that the FIRST call is handed the PARAMETER and the second is handed
//  the MEMBER reloaded from (%ebx) -- so the source names the member the
//  second time, not the argument.
// ======================================================================= //

class C1000adc0
{
public:
	C1000adc0* sub_1000adc0(uint32 nState, uint32 nValue);

	uint32 m_nState00;                                  // +0x00
	uint32 m_nOld04;                                    // +0x04
};

C1000adc0* C1000adc0::sub_1000adc0(uint32 nState, uint32 nValue)
{
	m_nState00 = nState;

	g_pD3DDevice->lpVtbl->GetRenderState(g_pD3DDevice, nState, &m_nOld04);
	g_pD3DDevice->lpVtbl->SetRenderState(g_pD3DDevice, m_nState00, nValue);

	return this;
}


// ======================================================================= //
//  0x10044f74 -- MOVED to d3dbatcha_o2.cpp.  It is byte-identical at /O2
//  and 84 bytes out at this file's /Os (the /O2 frame, `mov %ebx,(%esp)`
//  and `add $-N,%esp` outgoing areas, is in the shipped bytes).
// ======================================================================= //


// ======================================================================= //
//  0x1001e360 -- MOVED to big66f08.cpp as XForm66f08::sub_1001e360.  It is
//  byte-identical at /O2 and one instruction out at this file's /Os (the
//  constant 1 is `mov $0x1,%eax`, which /Os spells `xor; inc`); big66f08.cpp
//  is /O2 and holds its callers' class.  The decode notes moved with it.
// ======================================================================= //


// ======================================================================= //
//  0x100210d0   249 bytes (listing says 252)   14 callers
//
//  DECODED.  This is the DirectDraw teardown, and the five globals it walks
//  are the five d3dblit.cpp already named -- 0x1008bf70 g_pWindowedPrimary,
//  0x1008bf6c g_pPrimary, 0x1008bf68 g_pOffscreen, 0x1008d97c g_pClipper,
//  0x1008bf74 g_pDirectDraw.  That naming is what identifies the three
//  IDirectDraw7 slots it uses: +0x4c with `this` alone is RestoreDisplayMode,
//  +0x50 with (this, NULL, 8) is SetCooperativeLevel(NULL, DDSCL_NORMAL),
//  +0x08 is Release.
//
//  WHAT PINS THE SHAPE, and it is the one thing here worth arguing about:
//  the function does NOT read as a straight run of `if (p) { Release; p =
//  NULL; }`.  It branches once on g_pWindowedPrimary and the two sides are
//  genuinely different code:
//
//    windowed  (bf70 != 0):  release bf70; if (bf6c) release it;
//                            bf68 = NULL UNCONDITIONALLY;
//                            if (d97c) release it
//    otherwise (bf70 == 0):  if (bf6c) release it;
//                            if (bf68) bf68 = NULL;
//                            -- and the clipper is never touched
//
//  A straight-line source cannot produce that: the second path SKIPS the
//  clipper block entirely (`je`/`jmp` both to 0x1002114b, the DirectDraw
//  block) and turns the offscreen store conditional, and no optimiser can
//  invent either from the other.  So the else really is in the source, and
//  it makes sense: the windowed path owns a separate primary and a clipper,
//  the fullscreen path's offscreen is an attached back buffer it does not
//  own.  Both paths then converge on the DirectDraw object itself.
//
//  The `if (g_pOffscreen) g_pOffscreen = LTNULL;` in the else arm is odd
//  source and is written as the bytes have it: `mov`, `test`, `je` past a
//  store of zero, with no call.
//
//  THE RELOAD.  Every surface and clipper release RE-READS its global after
//  the test -- `mov G,%eax; test; je; mov G,%eax; mov (%eax),%edx; push
//  %eax; call *8(%edx)` -- with no store in between, and reads it ONCE for
//  both the vtable and the argument.  Written plainly, icl keeps the tested
//  value in %eax (4 instructions short, 229 bytes against 249).  A single
//  volatile read of the global into a local for the call is the only
//  spelling measured to give both properties (a volatile global reads twice
//  per call, a C++ virtual `p->Release()` and inline SafeRelease helpers
//  change nothing); DirectDraw's own block does NOT reload before its first
//  call, so it is left plain.  Byte-identical with this form.
// ======================================================================= //

#define VS(g) (*(DDSurface* volatile*)&(g))
#define VC(g) (*(DDClipper* volatile*)&(g))
void sub_100210d0()
{
	ShowCursor(1);
	sub_10032c30();

	if (g_pWindowedPrimary)
	{
		{ DDSurface* p = VS(g_pWindowedPrimary); p->lpVtbl->Release(p); }
		g_pWindowedPrimary = LTNULL;

		if (g_pPrimary)
		{
			{ DDSurface* p = VS(g_pPrimary); p->lpVtbl->Release(p); }
			g_pPrimary = LTNULL;
		}

		g_pOffscreen = LTNULL;

		if (g_pClipper)
		{
			{ DDClipper* p = VC(g_pClipper); p->lpVtbl->Release(p); }
			g_pClipper = LTNULL;
		}
	}
	else
	{
		if (g_pPrimary)
		{
			{ DDSurface* p = VS(g_pPrimary); p->lpVtbl->Release(p); }
			g_pPrimary = LTNULL;
		}

		if (g_pOffscreen)
			g_pOffscreen = LTNULL;
	}

	if (g_pDirectDraw)
	{
		g_pDirectDraw->lpVtbl->RestoreDisplayMode(g_pDirectDraw);
		g_pDirectDraw->lpVtbl->SetCooperativeLevel(g_pDirectDraw, LTNULL, 8);
		g_pDirectDraw->lpVtbl->Release(g_pDirectDraw);
		g_pDirectDraw = LTNULL;
	}
}


// *********************************************************************** //
//
//  THE SIX ROWS THAT ARE NOT HERE
//
// ----------------------------------------------------------------------- //
//  0x100385e0 (80 bytes, 3 callers) -- REGISTER-ABI ENTRY, cannot be spelled
//
//  Its first instruction is `cmp $0x3,%eax` with no prologue and no load,
//  and it ends in a plain `ret`: the one argument arrives in %eax and there
//  is no stack-ABI shim in front of it (0x100385de is the previous
//  function's `ret`, 0x100385df is a `nop`).  Its three callers set %eax and
//  `call` it directly.  That convention is icl's own choice between icl's
//  own functions; no C declaration selects it, and any function this file
//  emitted would start with a `mov 0x4(%esp),%eax` the shipped bytes do not
//  have, putting every later byte four out of place.
//
//  The DECODE is complete and is recorded here so it need not
//  be redone.  Globals: g = 0x1008da80, p4 = 0x1008da68,
//  p5 = 0x1008da64, p6 = 0x1008da60.
//
//      if (nType == 3)  return LTFALSE;
//      if (!g)          return LTFALSE;
//      if (nType == 4)  n = p4;
//      else if (nType == 5) n = p5;
//      else if (nType == 6) n = p6;
//      else             return LTFALSE;
//      return (n != 0);
//
//  Two separate `xor %eax,%eax` blocks: 0x10038602 serves the two early
//  guards, 0x100385fe serves both the final `else` and the `n == 0` case, so
//  the last arm's zero and the trailing refusal are one answer.  The true
//  answer is `mov $0x1,%eax`, NOT `xor/inc` -- a boolean conversion, not a
//  literal `return 1`.
//
// ----------------------------------------------------------------------- //
//  0x1007a580 (256 bytes, 3 callers) -- REGISTER-ABI ENTRY, same reason
//
//  Takes its object in %eax and a depth counter in %edx, ends in a plain
//  `ret`, and RECURSES into itself with `mov %ebp,%eax; inc %edx;
//  call 0x1007a580`.  It walks a list at object+0x98 through +0x24, refuses
//  below depth 0x20, skips nodes whose +0x58 already equals the global at
//  0x1008bea4 and stamps that global into +0x58 after visiting, and
//  dispatches through a 20-byte-strided table at 0x100895c0 indexed by the
//  signed byte at node+0xbc.  Worth having; not spellable as an entry point.
//
// ----------------------------------------------------------------------- //
//  0x10039210 (528 bytes, 3 callers) -- REGISTER-ABI ENTRY, same reason
//
//  `mov %eax,-0x8(%ebp); mov %ecx,-0x4(%ebp); mov %edx,%esi` in the
//  prologue: three arguments, all in registers, none on the stack.
//
// ----------------------------------------------------------------------- //
//  0x10015320 (329 bytes, 7 callers) -- DECODED but the source shape is not
//  settled, so it is left out rather than spent.
//
//  It is a lookup-or-create over a list hung off argument0+0x0c and chained
//  through +0x30, keyed on the 16-bit field at node+0x42 against the LOW
//  BYTE of argument 1 (`movzbl %dl,%edx`), with the search loop unrolled
//  twice by icl.  On a hit it compares the node against g_Table[arg1]
//  (0x1008e684, dword-strided) and calls 0x1000a7d0 only when they differ;
//  on a miss it calls 0x10039bd0(arg0, arg1, 1) and links the result in
//  after the head, or 0x10039bd0(arg0, arg1, 0) when the list was empty, and
//  answers 0 if that fails.  The tail -- `if (node->w44 != arg2) { node->
//  m_p0c->vtbl+0xbc(arg2); node->w44 = arg2; } return 1` -- appears TWICE in
//  the shipped code, once at 0x1001539f and once at 0x100153dc, and the
//  create paths jump into the first copy without re-testing the g_Table
//  comparison.  It is undecided whether that is icl duplicating one
//  source tail or the source containing it twice, and the two spellings give
//  different bytes.
//
// ----------------------------------------------------------------------- //
//  0x1006d210 (404 bytes) and 0x1002b1b0 (4442 bytes) -- not read.
//
//  Both are ordinary framed __thiscall bodies with stack arguments, so
//  neither has an ABI problem; they are simply longer than this pass had
//  room for.  0x1006d210 opens by zeroing a 0x2c-byte local block and
//  copying +0x0c, +0x10, +0x18 of `this` into it, which is a descriptor
//  being filled before a call.  0x1002b1b0 opens by handing
//  (&local(-0x234), 0x1008b9dc, &arg0) to a function -- a format string and
//  a varargs list, so its first act is a formatted-message call.
//
// *********************************************************************** //


// ======================================================================= //
//  0x1001d950 -- element size 2.  Same family as 0x1001ced0 above, and it
//  moved here from d3dbatchb.cpp for two reasons that travel together.
//
//  IT IS A MEMBER, NOT A FREE FUNCTION. There were exactly five __stdcall
//  symbols in src/engine/d3d, all five were near-misses, and every one of
//  them was SHORT OF FRAME by eight bytes -- `sub $0x4,%esp` where the
//  shipped code reserves `sub $0xc`. 0x1001ced0 is the same body at a
//  different element size, written as a member, and it is byte-identical
//  including that reserve. __stdcall suppresses icl's dead-frame reserve.
//
//  AND IT NEEDS /Os. In our own objects an /O2 member never reserves less
//  than 0x10; only /Os members reach 12. d3dbatchb.cpp is /O2 and has seven
//  other byte-identical functions, so the bodies move rather than the file.
//
//  The call sites agree: all four do `mov %ebx,%ecx` immediately before the
//  `call` and place two dwords in a pre-allocated outgoing area.
// ======================================================================= //
class C1001d950
{
public:
	void* sub_1001d950(uint32 nCount, CD3DAlloc* pAlloc);

	uint32 m_nUnknown00;                                // +0x00, never read
};

void* C1001d950::sub_1001d950(uint32 nCount, CD3DAlloc* pAlloc)
{
	void* pRet;
	void* p;

	p = pAlloc->Alloc(nCount * 2);

	if (p)
		pRet = p;
	else
		pRet = LTNULL;

	return pRet;
}


// ======================================================================= //
//  0x10021aa0 -- element size 4.  Same family as 0x1001ced0 above, and it
//  moved here from d3dbatchb.cpp for two reasons that travel together.
//
//  IT IS A MEMBER, NOT A FREE FUNCTION. There were exactly five __stdcall
//  symbols in src/engine/d3d, all five were near-misses, and every one of
//  them was SHORT OF FRAME by eight bytes -- `sub $0x4,%esp` where the
//  shipped code reserves `sub $0xc`. 0x1001ced0 is the same body at a
//  different element size, written as a member, and it is byte-identical
//  including that reserve. __stdcall suppresses icl's dead-frame reserve.
//
//  AND IT NEEDS /Os. In our own objects an /O2 member never reserves less
//  than 0x10; only /Os members reach 12. d3dbatchb.cpp is /O2 and has seven
//  other byte-identical functions, so the bodies move rather than the file.
//
//  The call sites agree: all four do `mov %ebx,%ecx` immediately before the
//  `call` and place two dwords in a pre-allocated outgoing area.
// ======================================================================= //
class C10021aa0
{
public:
	void* sub_10021aa0(uint32 nCount, CD3DAlloc* pAlloc);

	uint32 m_nUnknown00;                                // +0x00, never read
};

void* C10021aa0::sub_10021aa0(uint32 nCount, CD3DAlloc* pAlloc)
{
	void* pRet;
	void* p;

	p = pAlloc->Alloc(nCount * 4);

	if (p)
		pRet = p;
	else
		pRet = LTNULL;

	return pRet;
}


// ======================================================================= //
//  0x1002e690 -- element size 12.  Same family as 0x1001ced0 above, and it
//  moved here from d3dbatchb.cpp for two reasons that travel together.
//
//  IT IS A MEMBER, NOT A FREE FUNCTION. There were exactly five __stdcall
//  symbols in src/engine/d3d, all five were near-misses, and every one of
//  them was SHORT OF FRAME by eight bytes -- `sub $0x4,%esp` where the
//  shipped code reserves `sub $0xc`. 0x1001ced0 is the same body at a
//  different element size, written as a member, and it is byte-identical
//  including that reserve. __stdcall suppresses icl's dead-frame reserve.
//
//  AND IT NEEDS /Os. In our own objects an /O2 member never reserves less
//  than 0x10; only /Os members reach 12. d3dbatchb.cpp is /O2 and has seven
//  other byte-identical functions, so the bodies move rather than the file.
//
//  The call sites agree: all four do `mov %ebx,%ecx` immediately before the
//  `call` and place two dwords in a pre-allocated outgoing area.
// ======================================================================= //
class C1002e690
{
public:
	void* sub_1002e690(uint32 nCount, CD3DAlloc* pAlloc);

	uint32 m_nUnknown00;                                // +0x00, never read
};

void* C1002e690::sub_1002e690(uint32 nCount, CD3DAlloc* pAlloc)
{
	void* pRet;
	void* p;

	p = pAlloc->Alloc(nCount * 12);

	if (p)
		pRet = p;
	else
		pRet = LTNULL;

	return pRet;
}
