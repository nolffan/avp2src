// Split out of d3dbatchb.cpp: these functions need the
// OTHER optimisation flag from the rest of that file.  d3d.ren is a
// mixed build and our file boundaries are invented.
// Everything that is not a function body is
// carried over verbatim so no declaration is lost.
// ********************************************************* //
// *********************************************************************** //
//
//  MODULE:   BATCH_B (.CPP)
//
//  PURPOSE:  Listing batch B for d3d.ren -- 25 addresses, of which 19 are
//            written here.  Every body below was read instruction by
//            instruction off the retail d3d.ren; nothing is
//            inferred from a caller unless the comment says so.
//
//  THE FOUR TEMPLATE FAMILIES THIS BATCH FOUND.  Six of the nineteen are
//  members of a family whose other members are also in the batch, so one
//  decode covers several functions:
//
//    FAMILY 1 -- the thiscall accessor.  0x1000a670 and 0x1004fb70 are the
//      SAME EIGHT BYTES (`56 8b 41 04 83 c4 04 c3`) at two addresses, and
//      0x1007ee60 differs in exactly ONE byte (`8b 41 08` -- +0x08 instead
//      of +0x04).  Three functions, one shape.
//
//    FAMILY 2 -- the sized allocation helper.  0x1001d950, 0x10021aa0 and
//      0x1002e690 are identical except for how the element count is scaled:
//      `add %eax,%eax` (x2), that twice (x4), and
//      `lea 0x0(,%eax,8) / lea (%edx,%eax,4)` (x12).  One template, three
//      element sizes -- 2, 4 and 12 bytes.
//
//    FAMILY 3 -- the "release the buffer, then reset the link" method.
//      0x1002e6c0 (65 bytes) and 0x10009cf0 (67 bytes) contain the SAME
//      instruction multiset in two different block layouts.  Same source
//      shape modulo one early return; see the two functions for which is
//      which and why.
//
//    FAMILY 4 -- the thiscall forwarder.  0x1000a630 and 0x1001cd10 are
//      byte-identical over all 38 bytes EXCEPT the 4-byte call
//      displacement, and 0x1007c870 is the one-argument cousin.
//
//  ONE FINGERPRINT THIS BATCH ADDS, and it is load-bearing for anyone
//  writing indirect calls in this module:
//
//      d3d.ren uses TWO different call sequences for a virtual call, and
//      they mean different source.
//
//        (a)  mov (%eax),%eax ; call *0x50(%eax)          -- 3-byte call
//        (b)  mov (%ecx),%eax ; mov 0x8(%eax),%eax ; call *%eax
//
//      (a) is what the explicit function-pointer-struct model in
//      src/engine/d3d/d3dopt2d.cpp emits, and d3d_SetOptimized2DBlend is
//      561 bytes BYTE-IDENTICAL with it, so (a) is measured, not assumed.
//      (b) is two bytes longer for the same offset, so it is not a size
//      choice -- it is what icl emits for a REAL C++ virtual call.  So the
//      D3D7 device below keeps the vtable-struct model and the allocator
//      interface below is written as a C++ class with virtuals.  Getting
//      this backwards costs two bytes and a modrm on every call site.
//
//  Style follows src/engine/d3d/d3dopt2d.cpp and d3dhigh.cpp: every
//  function is named sub_<address> (symbols map back to
//  addresses through the name), classes are named after the address of the
//  member being reconstructed where the mangled name would otherwise carry
//  no address, and unknown globals are named after their address.
//
//  SKIPPED, with reasons, at the bottom of this file: 0x10035ab0,
//  0x10037b10, 0x10037bf0, 0x10073500, 0x10011620, 0x1006d550.
//
// *********************************************************************** //

#include "ltbasedefs.h"


// ======================================================================= //
//  SHARED DECLARATIONS
// ======================================================================= //

// ---------------------------------------------------------------------- //
//  The allocator interface.
//
//  Three call sites in this batch reach it and they agree completely:
//
//    0x1001d95f  mov (%ecx),%eax ; mov 0x4(%eax),%eax ; call *%eax
//    0x10009d23  mov (%ecx),%eax ; mov 0x8(%eax),%eax ; call *%eax
//    0x1007dd07  mov (%ecx),%edx ; mov 0x8(%edx),%eax ; call *%eax
//
//  `this` in %ecx, one argument pushed, and NO stack cleanup after the call
//  -- callee-pops, i.e. __thiscall, i.e. an ordinary C++ virtual.  Slot
//  +0x04 takes a byte count and answers a pointer that the caller then null
//  tests; slot +0x08 takes that pointer and answers nothing.  Alloc and
//  Free.  Slot +0x00 is never reached from this batch and is declared only
//  to put the other two at the right offsets; it is NOT named.
//
//  Declared, never defined and never constructed, so no vtable is emitted
//  for it here -- which is what we want, because the real one is in the
//  shipped image.
// ---------------------------------------------------------------------- //
struct IAllocator
{
	virtual void* Slot00(uint32 nSize);                 // vtable +0x00
	virtual void* Alloc(uint32 nSize);                  // vtable +0x04
	virtual void  Free(void* pMem);                     // vtable +0x08
};

// 0x10091df0 -- a global allocator OBJECT, not a pointer to one: 0x1007dd00
// materialises the address with `mov $0x10091df0,%ecx` and dereferences it
// as the vtable holder.  BSS.
extern IAllocator g_10091df0;


// ---------------------------------------------------------------------- //
//  The Direct3D 7 device, exactly as src/engine/d3d/d3dopt2d.cpp models it.
//  Repeated here rather than shared because these are separate translation
//  units; the slot evidence is in that file's header and is not repeated.
// ---------------------------------------------------------------------- //
struct D3DDevice7;

struct D3DDevice7Vtbl
{
	uint8 m_pad_00[0x50];
	// +0x50 -- IDirect3DDevice7::SetRenderState (index 20).
	long (__stdcall* SetRenderState)(D3DDevice7*, uint32 dwState,
	                                 uint32 dwValue);
	uint8 m_pad_54[0x40];
	// +0x94 -- IDirect3DDevice7::SetTextureStageState (index 37).
	long (__stdcall* SetTextureStageState)(D3DDevice7*, uint32 dwStage,
	                                       uint32 dwState, uint32 dwValue);
};

struct D3DDevice7
{
	D3DDevice7Vtbl* lpVtbl;
};

// 0x1008d988 -- the device.  Same global d3ddraw.cpp and d3dopt2d.cpp use.
extern D3DDevice7* g_pD3DDevice;

// D3DRENDERSTATETYPE / D3DBLEND / D3DTEXTURESTAGESTATETYPE / D3DTEXTUREOP,
// spelled out because the toolchain ships DirectX 5's d3d.h.
#define D3DRENDERSTATE_ZWRITEENABLE          14         // 0x0e
#define D3DRENDERSTATE_SRCBLEND              19         // 0x13
#define D3DRENDERSTATE_DESTBLEND             20         // 0x14
#define D3DRENDERSTATE_ALPHABLENDENABLE      27         // 0x1b

#define D3DBLEND_ONE                          2
#define D3DBLEND_SRCALPHA                     5
#define D3DBLEND_INVSRCALPHA                  6

#define D3DTSS_COLOROP                        1
#define D3DTOP_MODULATE                       4
#define D3DTOP_ADDSIGNED                      8


// ======================================================================= //
//  FAMILY 1 -- THE THISCALL ACCESSOR
//
//  0x1000a670   56 8b 41 04 83 c4 04 c3
//  0x1004fb70   56 8b 41 04 83 c4 04 c3      <- the same eight bytes
//  0x1007ee60   56 8b 41 08 83 c4 04 c3      <- one byte apart
//
//      push %esi              ; the dummy 4-byte slot, opened with a push
//      mov  0x4(%ecx),%eax    ; (or 0x8) -- `this` really is in %ecx
//      add  $0x4,%esp         ; and closed with an add, not a pop
//      c3                     ; plain ret: no stack arguments
//
//  Three separate classes: the shipped image folds nothing, and 0x1000a670
//  and 0x1004fb70 are byte-for-byte the same function emitted twice.
//
//  WHAT PINS 0x1000a670's CLASS: the call site at 0x1001d843 does
//  `lea 0xc(%ebx),%ecx; call 0x1000a670`, i.e. it is invoked on a
//  subobject twelve bytes into its owner -- the same subobject the two
//  Family 3 methods below tie off through 0x1000a680, which is the
//  neighbouring `mov %eax,(%ecx)` setter for the SAME class's +0x00.  So
//  this little class has a field at +0x00 (setter 0x1000a680, getter
//  0x1000a690) and a field at +0x04 (getter 0x1000a670), and it is the
//  member at +0x0c of two larger objects.
// ======================================================================= //

struct Node1000a670
{
	uint32  m_n00;                                      // +0x00
	uint32  m_n04;                                      // +0x04

	uint32 sub_1000a670() const;
	// 0x1000a680, the +0x00 setter: `56 8b 44 24 08 89 01 83 c4 04 c2 04 00`.
	// Declared only -- it is not in this batch.
	void   sub_1000a680(uint32 nValue);
};



// 0x1004fb70 -- the same eight bytes at another address, so another class
// with the same accessor.  Four callers.
struct Obj1004fb70
{
	uint32  m_n00;                                      // +0x00
	uint32  m_n04;                                      // +0x04

	uint32 sub_1004fb70() const;
};



// 0x1007ee60 -- the +0x08 instantiation.  27 callers, the second most
// reached function in this batch.
struct Obj1007ee60
{
	uint32  m_n00;                                      // +0x00
	uint32  m_n04;                                      // +0x04
	uint32  m_n08;                                      // +0x08

	uint32 sub_1007ee60() const;
};



// ======================================================================= //
//  0x1000d9d0 -- 21 bytes, the two-field thiscall setter.  13 callers.
//
//      56              push %esi              ; dummy slot
//      8b 44 24 08     mov  0x8(%esp),%eax    ; first stack argument
//      8b 54 24 0c     mov  0xc(%esp),%edx    ; second
//      89 41 18        mov  %eax,0x18(%ecx)
//      89 51 1c        mov  %edx,0x1c(%ecx)
//      83 c4 04        add  $0x4,%esp
//      c2 08 00        ret  $0x8
//
//  Both arguments move with `mov`, not `fld`/`fstp`, so they are bit
//  patterns -- integers or pointers, NOT floats (the integer-moves-for-float-data rule).
//  Two adjacent dwords at +0x18/+0x1c written in argument order; there is
//  nothing to schedule, so this is a forced body.
//
//  The listing's 128 bytes is the gap to the next function; the `ret` is
//  at 0x1000d9e2 and everything past 0x1000d9e5 is padding.
// ======================================================================= //

struct Obj1000d9d0
{
	uint8   m_pad_00[0x18];                             // +0x00
	uint32  m_n18;                                      // +0x18
	uint32  m_n1c;                                      // +0x1c

	void sub_1000d9d0(uint32 nA, uint32 nB);
};



// ======================================================================= //
//  0x1007b964 -- 27 bytes, a CONSTRUCTOR.  15 callers.
//
//      56                       push %esi         ; dummy slot
//      33 c0                    xor  %eax,%eax    ; 0, hoisted: used 3 times
//      c7 41 18 <0x100a9810>    movl $g,0x18(%ecx)
//      89 49 10                 mov  %ecx,0x10(%ecx)   ; <- stores `this`
//      89 41 04                 mov  %eax,0x4(%ecx)
//      89 01                    mov  %eax,(%ecx)
//      89 41 14                 mov  %eax,0x14(%ecx)
//      83 c4 04                 add  $0x4,%esp
//      8b c1                    mov  %ecx,%eax    ; return `this`
//      c3
//
//  WHAT PINS IT AS A CONSTRUCTOR rather than an init method: the tail is
//  `mov %ecx,%eax` AFTER the stack adjustment and with a plain `ret`, which
//  is icl's standard constructor epilogue (a constructor answers `this`),
//  and the body writes `this` into its own +0x10 -- a self-referential link
//  head, which is what a list anchor does when it is constructed empty.
//
//  WHAT PINS THE STORE ORDER: +0x18, +0x10, +0x04, +0x00, +0x14.  That is
//  neither declaration order nor address order, so these are ASSIGNMENTS IN
//  THE CONSTRUCTOR BODY, not a member-initialiser list (which icl emits in
//  declaration order).
//
//  The `xor %eax,%eax` is hoisted above every store because zero is used
//  three times -- the same reused-constant behaviour d3d_ResetCounters
//  already proves byte-identical.
//
//  The class is NAMED after the address on purpose: a constructor mangles
//  as ??0<class>@@..., so the address has to live in the class name for
//  it to be found.
//
//  0x100a9810 is sixteen zero bytes in .data1, immediately in front of the
//  string "[FULBRITE] - ".  Its contents are not this function's evidence,
//  so it is declared as an unbounded byte array and NOT named.
// ======================================================================= //

extern uint8 g_100a9810[];

struct sub_1007b964
{
	uint32  m_n00;                                      // +0x00
	uint32  m_n04;                                      // +0x04
	uint32  m_n08;                                      // +0x08  untouched
	uint32  m_n0c;                                      // +0x0c  untouched
	void*   m_p10;                                      // +0x10
	uint32  m_n14;                                      // +0x14
	void*   m_p18;                                      // +0x18

	sub_1007b964();
};



// ======================================================================= //
//  THE VECTOR MATHS -- three functions, and all three are the SDK's own
//  _CVector members, which src/LT2/sdk/inc/ltvector.h still carries verbatim.
//
//  The class is spelled out locally as `Vec3` rather than pulled from that
//  header because the header's members are inline templates: including it
//  emits nothing unless something forces an instantiation, and the point
//  here is to name the out-of-line bodies sub_<address>.
// ======================================================================= //

struct Vec3
{
	float x;                                            // +0x00
	float y;                                            // +0x04
	float z;                                            // +0x08

	Vec3() {}
	Vec3(float mx, float my, float mz) { x = mx; y = my; z = mz; }

	Vec3  sub_100155f0(const Vec3 v) const;
	float sub_10027f50(Vec3 v) const;
};


// ---------------------------------------------------------------------- //
//  0x100155f0 -- 68 bytes, `operator +`.  17 callers, the most reached
//  function in this batch.
//
//      57 56 53                 push %edi/%esi/%ebx
//      8b 5c 24 10              mov  0x10(%esp),%ebx   ; hidden return slot
//      83 ec 10                 sub  $0x10,%esp        ; the local temp
//      d9 01 / d8 44 24 24      flds (%ecx)  ; fadds 0x24(%esp)   -> x + v.x
//      d9 41 04 / d8 44 24 28   flds 4(%ecx) ; fadds 0x28(%esp)   -> y + v.y
//      d9 c9                    fxch %st(1)
//      d9 5c 24 04              fstps 0x4(%esp)        ; temp.x
//      d9 5c 24 08              fstps 0x8(%esp)        ; temp.y
//      d9 41 08 / d8 44 24 2c   flds 8(%ecx) ; fadds 0x2c(%esp)   -> z + v.z
//      8d 74 24 04              lea  0x4(%esp),%esi
//      8b fb                    mov  %ebx,%edi
//      d9 5c 24 0c              fstps 0xc(%esp)        ; temp.z
//      b9 03 00 00 00           mov  $0x3,%ecx
//      f3 a5                    rep movsl              ; temp -> return slot
//      83 c4 10                 add  $0x10,%esp
//      8b c3                    mov  %ebx,%eax         ; answer the slot
//      5b 5e 5f                 pops
//      c2 10 00                 ret  $0x10
//
//  WHAT PINS IT.  `ret $0x10` is sixteen bytes of stack arguments: the
//  hidden return pointer plus three floats, so `this` is in %ecx and the
//  operand is passed BY VALUE, which is exactly the SDK's signature
//  `_CVector<T> operator + (const _CVector<T> v) const`.  The operands come
//  off `this` and off the stack in x/y/z order, and the results land in a
//  16-byte local at 0x4(%esp) which is then copied to the caller's slot
//  with `rep movsl` -- that copy is the tell that the source builds a
//  TEMPORARY and returns it (`return Vec3(...)`), rather than writing
//  through the return slot directly.
//
//  The `fxch` before the two stores is pure scheduling: both sums are
//  computed before either is stored, so the second one is on top and has to
//  be swapped down.
// ---------------------------------------------------------------------- //


// Moved here from d3dbatchb.cpp (/O2), unchanged: at /O2 icl elides the
// temporary and stores the three sums straight into the return slot (41
// bytes, `push %esi` frame); at /Os it keeps the 16-byte temporary and the
// `rep movsl` copy exactly as shipped. Byte-identical at /Os.
Vec3 Vec3::sub_100155f0(const Vec3 v) const
{
	return Vec3(x + v.x, y + v.y, z + v.z);
}


// ---------------------------------------------------------------------- //
//  0x10027f50 -- 31 bytes, `Dot`.  11 callers.
//
//      56                       push %esi              ; dummy slot
//      d9 41 08 / d8 4c 24 10   flds 8(%ecx) ; fmuls 0x10(%esp)  -> z * v.z
//      d9 41 04 / d8 4c 24 0c   flds 4(%ecx) ; fmuls 0x0c(%esp)  -> y * v.y
//      de c1                    faddp
//      d9 01    / d8 4c 24 08   flds (%ecx)  ; fmuls 0x08(%esp)  -> x * v.x
//      de c1                    faddp
//      83 c4 04                 add  $0x4,%esp
//      c2 0c 00                 ret  $0xc
//
//  WHAT PINS IT: `ret $0xc` with `this` in %ecx is one by-value Vec3, and
//  the three products are the three component pairs.  The evaluation order
//  is z, then y, then x -- the module-wide RIGHT-TO-LEFT n-ary arithmetic
//  fingerprint, so the source is the plain three-term sum written left to
//  right and the compiler turned it round:
//
//      T Dot(_CVector<T> v) const { return (x*v.x) + (y*v.y) + (z*v.z); }
//
//  which is the SDK header's own line.
// ---------------------------------------------------------------------- //


// ---------------------------------------------------------------------- //
//  0x10048150 -- 34 bytes, the same dot product with one extra
//  instruction.  5 callers.
//
//  Byte-for-byte 0x10027f50 up to the second `faddp`, then
//
//      d8 61 0c                 fsubs 0xc(%ecx)
//
//  before the epilogue.  So the object is twelve bytes of vector followed
//  by a float at +0x0c and the answer is (dot - that float): a PLANE, and
//  this is the signed distance from a point to it.  LTPlane in
//  src/LT2/sdk/inc/ltbasedefs.h is laid out exactly that way, m_Normal then
//  m_Dist.
//
//  The subtraction is emitted LAST, after the whole three-term sum, which
//  is what `a + b + c - d` gives: C groups it as `(((a+b)+c)-d)`, the sum
//  is the n-ary chain that gets reversed, and the subtract stays outside
//  it.
// ---------------------------------------------------------------------- //
struct Plane10048150
{
	Vec3   m_Normal;                                    // +0x00
	float  m_fDist;                                     // +0x0c

	float sub_10048150(Vec3 v) const;
};



// ======================================================================= //
//  FAMILY 2 -- THE SIZED ALLOCATION HELPER, three instantiations.
//
//  0x1001d950 (40 bytes, 4 callers), 0x10021aa0 (42 bytes, 4 callers) and
//  0x1002e690 (48 bytes, 4 callers).  0x1001d950 in full:
//
//      55 8b ec                 push %ebp ; mov %esp,%ebp
//      83 ec 0c                 sub  $0xc,%esp
//      8b 45 08                 mov  0x8(%ebp),%eax    ; count
//      03 c0                    add  %eax,%eax         ; * 2
//      8b 4d 0c                 mov  0xc(%ebp),%ecx    ; the allocator
//      50                       push %eax
//      8b 01 / 8b 40 04 / ff d0 call *pAlloc->Alloc
//      85 c0                    test %eax,%eax
//      74 0a                    je   L_null
//      8b d0                    mov  %eax,%edx
//   L_join:
//      8b c2                    mov  %edx,%eax
//      8b e5 / 5d               mov  %ebp,%esp ; pop %ebp
//      c2 08 00                 ret  $0x8
//   L_null:
//      33 d2                    xor  %edx,%edx
//      eb f4                    jmp  L_join
//
//  The ONLY difference between the three is the scaling of the count:
//
//      0x1001d950   03 c0                          n * 2
//      0x10021aa0   03 c0 ... 03 c0                n * 4
//      0x1002e690   8d 14 c5 00000000 / 8d 14 82   n * 8 + n * 4 = n * 12
//
//  so this is one template over the element type, instantiated at 2, 4 and
//  12 bytes.
//
//  TWO THINGS PIN THE SHAPE.
//
//  First, the shared exit sits at the FIRST arm and the null arm jumps BACK
//  into it.  Per the shared-exit lever that is FULL ELSE-NESTING --
//  an if/else writing one local, not a guarded early return (which would
//  host the exit at the last arm).  The `mov %eax,%edx` / `mov %edx,%eax`
//  round trip is that local: the answer lives in %edx across the join and
//  is moved to %eax only once.
//
//  Second, `ret $0x8` with the arguments read from 0x8(%ebp) and 0xc(%ebp)
//  is two stack dwords, callee-pops.  The CALL SITES also load %ecx first
//  (0x1001d85a does `mov %ebx,%ecx` immediately before the call), so these
//  really are member functions -- but the body overwrites %ecx with its
//  second argument at once and never reads `this`, so __stdcall free
//  functions emit the identical bytes and are written that way here.  That
//  is a deliberate simplification and it is the only place in this file
//  where the source shape is not the shipped shape.
//
//  ABOUT THE NULL TEST: it is redundant -- the value is returned unchanged
//  either way.  icl 4.0 does not fold it, and this file has three more
//  independent demonstrations of the same weakness (Family 3 tests the same
//  pointer twice in a row, 0x1007dcc4 tests it three times).  So the
//  redundancy is in the source and the compiler keeps it.
// ======================================================================= //

// 0x1001d950 -- element size 2.
// ONE byte out, and it is the frame: shipped reserves `sub $0xc,%esp` where
// this reserves `sub $0x4`. It is not outgoing-argument space -- both push a
// single argument -- so the shipped source has locals this does not. Adding a
// third local (a named `nBytes` for the doubled count) is NOT the answer: it
// takes the function from 1 differing byte to 32. Same one-byte frame
// difference on sub_1002e690, so whatever it is, it is shared.
// 0x1001d950 -- MOVED to d3dbatcha.cpp beside its byte-identical witness
// 0x1001ced0. It is a __thiscall member, not a __stdcall free
// function, and it needs that file's /Os.


// 0x10021aa0 -- element size 4.
// 0x10021aa0 -- MOVED to d3dbatcha.cpp beside its byte-identical witness
// 0x1001ced0. It is a __thiscall member, not a __stdcall free
// function, and it needs that file's /Os.


// 0x1002e690 -- element size 12.  The scaling is `lea 0x0(,%eax,8),%edx`
// then `lea (%edx,%eax,4),%edx`, i.e. 8n + 4n, which is icl's strength
// reduction of a multiply by twelve -- not two separate terms in the
// source.
// 0x1002e690 -- MOVED to d3dbatcha.cpp beside its byte-identical witness
// 0x1001ced0. It is a __thiscall member, not a __stdcall free
// function, and it needs that file's /Os.


// ======================================================================= //
//  FAMILY 3 -- RELEASE THE BUFFER, THEN RESET THE LINK.
//
//  0x1002e6c0 (65 bytes, 8 callers) and 0x10009cf0 (67 bytes, 4 callers)
//  contain the SAME instructions in two different orders.  Both are
//  __thiscall with `ret $0x8`; both save %ebx to hold `this` across the
//  calls; both open a dead 4-byte slot with `push %edi`.
//
//  0x1002e6c0's layout, condition-first:
//
//      8b 43 04     mov  0x4(%ebx),%eax     ; the buffer
//      85 c0        test %eax,%eax
//      74 14        je   TAIL                <- skip the whole block
//      74 0b        je   ZERO                <- the SAME test again
//      ...          pAlloc->Free(buffer)
//   ZERO:
//      c7 43 04 0   movl $0x0,0x4(%ebx)
//   TAIL:
//      ...          (this + 0xc)->sub_1000a680(0)
//
//  0x10009cf0's layout, tail-first:
//
//      85 c0        test %eax,%eax
//      75 1b        jne  BLOCK
//   TAIL:
//      ...          (this + 0xc)->sub_1000a680(0)
//      c2 08 00     ret  $0x8
//   BLOCK:
//      74 0b        je   ZERO
//      ...          pAlloc->Free(buffer)
//   ZERO:
//      c7 43 04 0   movl $0x0,0x4(%ebx)
//      eb cf        jmp  TAIL
//
//  Same compiler and same flags cannot give one source two layouts, so the
//  two SOURCES differ, and the difference is where the tail sits.  In
//  0x1002e6c0 the tail is written once after a positive `if`; in 0x10009cf0
//  the tail is reached first on the null path, which is what an INVERTED
//  EARLY RETURN gives -- the tail is written twice in the source and icl
//  cross-jumps the second copy back to the first, which is precisely the
//  `jmp` at 0x10009d31.  That is the reading below.
//
//  THE DUPLICATED TEST is the interesting part and it is in both.  The same
//  pointer is tested twice with no intervening write.  The natural source
//  is a user guard wrapped round an inline release helper that guards
//  again; it is written out longhand here because that is what the bytes
//  say happens, and because icl 4.0 demonstrably does not collapse it.
//
//  BOTH TAKE TWO ARGUMENTS (`ret $0x8`) and read only the first.  The
//  second is dead -- the dead-parameter lever -- and is declared so the
//  stack cleanup comes out right.
// ======================================================================= //

// ---------------------------------------------------------------------- //
//  0x1002e6c0 -- the condition-first member.
// ---------------------------------------------------------------------- //
struct Obj1002e6c0
{
	uint32        m_n00;                                // +0x00
	void*         m_p04;                                // +0x04  the buffer
	uint32        m_n08;                                // +0x08
	Node1000a670  m_Link0c;                             // +0x0c

	void sub_1002e6c0(IAllocator* pAlloc, uint32 nUnused);
};



// ---------------------------------------------------------------------- //
//  0x10009cf0 -- the tail-first member.  Written with the inverted guard
//  and a duplicated tail, which is the only shape that puts the tail ahead
//  of the release block.
// ---------------------------------------------------------------------- //
struct Obj10009cf0
{
	uint32        m_n00;                                // +0x00
	void*         m_p04;                                // +0x04  the buffer
	uint32        m_n08;                                // +0x08
	Node1000a670  m_Link0c;                             // +0x0c

	void sub_10009cf0(IAllocator* pAlloc, uint32 nUnused);
};



// ======================================================================= //
//  FAMILY 4 -- THE THISCALL FORWARDER.
//
//  0x1000a630 and 0x1001cd10 are 38 bytes each and differ ONLY in the four
//  bytes of the call displacement:
//
//      55 8b ec                 push %ebp ; mov %esp,%ebp
//      83 ec 10                 sub  $0x10,%esp
//      83 c4 f8                 add  $0xfffffff8,%esp   ; 8-byte arg area
//      8b 45 08                 mov  0x8(%ebp),%eax     ; the one argument
//      c7 04 24 00 00 00 00     movl $0x0,(%esp)        ; callee arg 1 = 0
//      89 44 24 04              mov  %eax,0x4(%esp)     ; callee arg 2 = it
//      8b c1 / 8b c8            mov  %ecx,%eax ; mov %eax,%ecx
//      e8 <target>              call
//      8b e5 / 5d / c2 04 00    ret  $0x4
//
//  WHAT PINS THEM: `ret $0x4` with `this` in %ecx is one stack argument;
//  the callee gets that argument plus a literal zero and the SAME `this`.
//  So each is a one-line convenience wrapper that supplies a default.
//
//  The `mov %ecx,%eax ; mov %eax,%ecx` round trip is not source -- it is
//  what icl emits whenever a thiscall member forwards to another thiscall
//  member on the same object, and it appears identically in all three
//  members of this family.  Do not try to write it away.
//
//  The callees (0x1000a110 and 0x1001c7e0) are declared, not reconstructed.
// ======================================================================= //

struct Obj1000a630
{
	// 0x1000a110, the two-argument worker.  Declared only.
	void sub_1000a110(uint32 nA, uint32 nB);
	void sub_1000a630(uint32 nValue);
};



struct Obj1001cd10
{
	// 0x1001c7e0, the two-argument worker.  Declared only.
	void sub_1001c7e0(uint32 nA, uint32 nB);
	void sub_1001cd10(uint32 nValue);
};



// ---------------------------------------------------------------------- //
//  0x1007c870 -- 28 bytes, the one-argument cousin.  6 callers.
//
//      55 8b ec / 83 ec 10      frame
//      57                       push %edi              ; 4-byte arg area
//      8b 45 08                 mov  0x8(%ebp),%eax
//      89 04 24                 mov  %eax,(%esp)
//      8b c1 / 8b c8            the same %ecx round trip
//      e8 <0x10009d40>          call
//      8b e5 / 5d / c2 04 00    ret  $0x4
//
//  The four-byte outgoing area is opened with `push %edi` rather than an
//  `add`, which is the same one-byte trick the accessors use for their
//  dummy slot.
//
//  0x10009d40 is `mov 0x4(%ecx),%ecx ; mov 0x8(%esp),%eax ;
//  lea (%ecx,%eax,4),%eax`, i.e. `&m_pArray[i]` -- so this forwards an
//  index into a subscript on the same object.  Whether the source returns
//  that pointer or discards it is NOT decidable from the bytes: %eax
//  survives the epilogue either way and no extra instruction is emitted.
//  Written `void` for that reason.
// ---------------------------------------------------------------------- //
struct Obj1007c870
{
	// 0x10009d40, `&this->m_p04[nIndex]`.  Declared only.
	void* sub_10009d40(uint32 nIndex);
	void  sub_1007c870(uint32 nIndex);
};



// ======================================================================= //
//  0x1007dcc4 -- 84 bytes, a DESTRUCTOR BODY.  3 callers.
//
//      55 8b ec / 83 ec 10      frame
//      57                       push %edi              ; dummy slot
//      53                       push %ebx              ; real save
//      8b d9                    mov  %ecx,%ebx         ; this
//      c7 03 <0x10089a20>       movl $vtbl0,(%ebx)
//      8b cb / e8 <0x1007def4>  this->sub_1007def4()
//      8b 43 08                 mov  0x8(%ebx),%eax    ; the buffer
//      c7 43 04 <0x10089a60>    movl $vtbl1,0x4(%ebx)
//      85 c0 / 75 0c            test ; jne  T2
//      c7 43 0c 0               movl $0x0,0xc(%ebx)    <- first arm's target
//      5b / 8b e5 / 5d / c3     epilogue, plain ret: NO arguments
//   T2:
//      75 09                    jne  T3                <- the same test
//      c7 43 10 0               movl $0x0,0x10(%ebx)   <- second arm's target
//      eb e9                    jmp  back to +0x0c
//   T3:
//      b9 <0x10091df0>          mov  $g_alloc,%ecx     ; hoisted above the je
//      74 08                    je   SKIP              <- the same test again
//      8b 11 / 50 / 8b 42 08 / ff d0    g_alloc.Free(buffer)
//   SKIP:
//      c7 43 08 0               movl $0x0,0x8(%ebx)
//      eb df                    jmp  back to +0x10
//
//  WHAT PINS IT AS A DESTRUCTOR: it writes 0x10089a20 to +0x00 and
//  0x10089a60 to +0x04, and both of those are TABLES OF CODE POINTERS in
//  .data (0x10089a20 starts 0x1007dbd0, 0x1007dd20, 0x1007dde0, ...;
//  0x10089a60 starts 0x1001d240, 0x1001c760, ...).  Two vptrs at +0x00 and
//  +0x04 is multiple inheritance, and re-pointing them at the *base*
//  tables partway through the body is exactly and only what a derived
//  destructor does.  Written here as a plain method named after its own
//  address, because a real `~Class()` mangles without an address and
//  could not be placed.
//
//  WHAT PINS THE NESTING: three tests of the SAME pointer, and each one's
//  false path lands on a DIFFERENT cleanup store -- +0x0c for the first,
//  +0x10 for the second, +0x08 for the third.  That fixes the nesting
//  uniquely: the store a test skips to is the statement that follows the
//  block it guards.  There is no other arrangement of three ifs and three
//  stores that produces these three targets.
//
//  The `mov $0x10091df0,%ecx` above the third `je` is scheduling, not
//  source: the address constant is materialised on both paths because it
//  is free to do so.
//
//  IDENTICAL.  The three tests are dynarray.h's CMoArray Term chain over a
//  subobject at +0x04 (vptr 0x10089a60, m_pArray +0x08, m_nElements +0x0c,
//  cache +0x10) -- the destructor is `vptr; sub_1007def4(); ~CMoArray()`.
//  Written as the header's inline chain (as d3dr5a.cpp's 0x1007e850 is),
//  pAlloc stays a pointer and the vtable read goes through %ecx.  The
//  earlier hand-folded version (two nested switches and an if, the free
//  through an `IAllocator*` local) got 24 of 29 and folded the vtable load
//  to an absolute `mov 0x10091df0,%eax`.  The stack shim at 0x1007dcc0 is
//  just icl's /Qip external entry for a zero-argument member: 0x10009a60
//  calls +0 pushing nothing, as 0x1007dc50's shim is likewise ignored.
// ======================================================================= //

extern uint8 g_10089a20[];                              // vtable, base 0
extern uint8 g_10089a60[];                              // vtable, base 1

// dynarray.h's CMoArray<T>::Term chain, written as the header has it --
// Term -> SetSize2(0, pAlloc) -> _DeleteAndDestroyArray -> BaseDelete --
// exactly as d3dr5a.cpp's MoArray1007e270 does for 0x1007e850 (the same
// three-test figure over +0x40).  Through the inline chain `pAlloc` stays
// a pointer: `mov $0x10091df0,%ecx ; je ; mov (%ecx),%edx`.
template<class T>
inline void BaseDelete1007dcc4(IAllocator* pAlloc, T* ptr, uint32 nElements)
{
	T* pTemp;

	if (!ptr)
		return;

	pTemp = ptr;
	while (nElements)
	{
		--nElements;
		pTemp->~T();
		++pTemp;
	}

	pAlloc->Free(ptr);
}

struct DefCache1007dcc4
{
	uint32	m_CacheSize;                                // +0x00
	uint32	m_WantedCache;                              // +0x04

	uint32	GetCacheSize() const	{ return m_CacheSize; }
	void	SetCacheSize(uint32 n)	{ m_CacheSize = n; }
	uint32	GetWantedCache() const	{ return m_WantedCache; }
};

struct MoArray1007dcc4
{
	void*			m_pVTable;                      // +0x00
	uint32*			m_pArray;                       // +0x04
	uint32			m_nElements;                    // +0x08
	DefCache1007dcc4	m_Cache;                        // +0x0c

	uint32	GetNumAllocatedElements() const
	{
		return m_nElements + m_Cache.GetCacheSize();
	}

	void	_DeleteAndDestroyArray(IAllocator* pAlloc, uint32 nElements)
	{
		if (m_pArray)
		{
			BaseDelete1007dcc4(pAlloc, m_pArray, nElements);
			m_pArray = LTNULL;
		}

		m_Cache.SetCacheSize(0);
	}

	LTBOOL	SetSize2(uint32 newSize, IAllocator* pAlloc)
	{
		if ((newSize == m_nElements) && (newSize != 0))
			return LTTRUE;

		if (m_pArray)
			_DeleteAndDestroyArray(pAlloc, GetNumAllocatedElements());

		m_nElements = newSize;
		if (newSize > 0)
			m_Cache.SetCacheSize(m_Cache.GetWantedCache());

		return LTTRUE;
	}

	void	Term(IAllocator* pAlloc = &g_10091df0)	{ SetSize2(0, pAlloc); }

	void	Destruct()
	{
		m_pVTable = g_10089a60;
		Term();
	}
};

struct Obj1007dcc4
{
	void*   m_pVtbl00;                                  // +0x00
	MoArray1007dcc4	m_Array04;                          // +0x04

	// 0x1007def4.  Declared only.
	void sub_1007def4();
	void sub_1007dcc4();
};

void Obj1007dcc4::sub_1007dcc4()
{
	m_pVtbl00 = g_10089a20;

	sub_1007def4();

	m_Array04.Destruct();
}


// ======================================================================= //
//  0x10011560 -- 65 bytes, one SetTextureStageState behind a two-way
//  choice.  3 callers.
//
//      55 8b ec / 83 ec 10      frame
//      a1 <0x1008ae80>          mov  g,%eax
//      85 c0                    test %eax,%eax
//      a1 <0x1008d988>          mov  g_pD3DDevice,%eax   <- hoisted
//      74 07                    je   L0
//      ba 08 00 00 00           mov  $0x8,%edx
//      eb 05                    jmp  L1
//   L0: ba 04 00 00 00          mov  $0x4,%edx
//   L1: 83 c4 f0                add  $0xfffffff0,%esp    ; 16-byte arg area
//      33 c9 / 41               xor %ecx,%ecx ; inc %ecx ; the reused 1
//      89 54 24 0c              mov  %edx,0xc(%esp)      ; dwValue
//      89 04 24                 mov  %eax,(%esp)         ; this
//      89 4c 24 04              mov  %ecx,0x4(%esp)      ; dwStage  = 1
//      89 4c 24 08              mov  %ecx,0x8(%esp)      ; dwState  = 1
//      8b 00 / ff 90 94 000000  call *+0x94
//      8b e5 / 5d / c3          plain ret: no arguments
//
//  WHAT PINS IT.  The 16-byte outgoing area with `this` at (%esp) and no
//  cleanup after the call is the __stdcall COM shape d3dopt2d.cpp already
//  proves on this same object and this same +0x94 slot, so the callee is
//  SetTextureStageState(dwStage, dwState, dwValue).  dwStage = 1 and
//  dwState = 1 = D3DTSS_COLOROP; dwValue is 8 = D3DTOP_ADDSIGNED when the
//  global is set and 4 = D3DTOP_MODULATE when it is not.  Both are valid
//  D3DTEXTUREOP values and the pair is the standard detail/lightmap choice
//  for the second texture stage, which is a third agreement on the slot.
//
//  Two codegen notes.  The device pointer is loaded BETWEEN the test and
//  the branch -- pure scheduling, and free.  The literal 1 goes into %ecx
//  via `xor`/`inc` because it is used twice, the same reused-constant
//  behaviour d3d_ResetCounters proves; the literal 8 or 4 is used once and
//  stays an immediate.  Both fall out of writing the value as a local.
//
//  0x1008ae80 is read as a flag and nothing here names it further.
// ======================================================================= //

extern uint32 g_1008ae80;

// Moved here from d3dbatchb.cpp (/O2): /O2 materialises the reused 1 as
// `mov $0x1,%ecx`, /Os as the shipped `xor %ecx,%ecx; inc %ecx`. Byte-
// identical at /Os and nothing else changed.
void sub_10011560()
{
	D3DDevice7*       pDev = g_pD3DDevice;
	uint32            nColorOp;

	if (g_1008ae80)
		nColorOp = D3DTOP_ADDSIGNED;
	else
		nColorOp = D3DTOP_MODULATE;

	pDev->lpVtbl->SetTextureStageState(pDev, 1, D3DTSS_COLOROP, nColorOp);
}



// ======================================================================= //
//  0x10023d70 -- 211 bytes, six SetRenderState calls round one if/else.
//  4 callers.  (The listing's 224 is the gap to the next function.)
//
//  Unconditionally:
//      SetRenderState(0x1b, 1)      ALPHABLENDENABLE on
//      SetRenderState(0x0e, 0)      ZWRITEENABLE off
//  then on the single stack argument:
//      arg != 0:   SRCBLEND = 2 (ONE),      DESTBLEND = 2 (ONE)
//      arg == 0:   SRCBLEND = 5 (SRCALPHA), DESTBLEND = 6 (INVSRCALPHA)
//
//  WHAT PINS IT.  Every call is the measured COM shape: 12-byte outgoing
//  area, `this` reloaded from 0x1008d988 for each call, `call *0x50(%eax)`,
//  no cleanup.  The four state numbers are the ones d3dopt2d.cpp already
//  pins seven times over, and the two blend PAIRS are two rows of that same
//  table -- (ONE, ONE) is LTSURFACEBLEND_ADD and (SRCALPHA, INVSRCALPHA) is
//  LTSURFACEBLEND_ALPHA.  So the argument selects additive blending against
//  ordinary alpha blending, with Z writes off either way: this is the
//  render-state preamble for a translucent pass.
//
//  `ret` with no count and the argument at 0x8(%ebp) is __cdecl with one
//  dword.  The if/else is a true if/else: the then-arm ends in
//  `eb 40 jmp` over the else-arm to the shared epilogue at 0x10023e3f.
// ======================================================================= //



// *********************************************************************** //
//
//  WHAT WAS NOT WRITTEN, AND WHY.  Six of the twenty-five.  Each entry
//  records what the bytes say, so it need not be decoded
//  again.
//
// ----------------------------------------------------------------------- //
//  0x10035ab0 (31 bytes, 3 callers) -- NOT SOURCE.  It is the compiler's
//  own deleting destructor:
//
//      call 0x100362a4          ; the real destructor, `this` in %ecx
//      mov  0x8(%ebp),%eax      ; the hidden flags argument
//      a8 01                    ; test $0x1,%al
//      75 07                    ; jne  -> operator delete
//
//  `test $1` on a hidden trailing argument is the scalar-deleting-dtor
//  thunk, generated whenever a class has a virtual destructor.  icl names
//  these `_$E...`, which are skipped because
//  they carry no address of their own.  Nothing to write: it appears by
//  itself once the class it belongs to exists.
//
// ----------------------------------------------------------------------- //
//  0x10037b10 (224 bytes, 5 callers) and 0x10037bf0 (832 bytes, 10
//  callers) -- REGISTER-ABI ENTRY POINTS.  0x10037b10 reads %eax and %edx
//  as its arguments before touching the stack (`mov %edx,%ebx` /
//  `mov %eax,%ebp` at 0x10037b18); 0x10037bf0 reads %eax, %edx AND %ecx.
//  These are icl's private convention for a static function whose call
//  sites it can all see, so reproducing the ENTRY requires the function and
//  all ten of its callers to be static in one translation unit -- which is
//  not something this batch can arrange.  Nothing about them is otherwise
//  hard, and they are worth revisiting once a whole shipped TU is
//  assembled.
//
//  For the record, since it was decoded on the way to that conclusion:
//  0x10037b10 formats a DDPIXELFORMAT for the console.  It prints "%sNONE"
//  when the descriptor pointer is null; otherwise, if bit 2 of +0x10 is set
//  it renders +0x14 as a four-character code through "FOURCC( %s )" into a
//  0x100-byte local, else it calls 0x100375f0 to fill that local, and then
//  prints "%s%s - %d bits (%d %d %d %d) (%x %x %x %x)" with +0x18 as the
//  bit depth, +0x34/+0x38/+0x3c/+0x40 as four counts and +0x1c/+0x20/
//  +0x24/+0x28 as four masks, through slot +0x14 of the RenderStruct
//  pointer parked at 0x1008cbcc.  That is a DDPIXELFORMAT: dwFlags at
//  +0x10, dwFourCC at +0x14, dwRGBBitCount at +0x18 and the four masks
//  from +0x1c.
//
// ----------------------------------------------------------------------- //
//  0x10073500 (308 bytes, 3 callers) -- SPHERICAL INTERPOLATION, and it
//  opens with a REALIGNING DUAL PROLOGUE:
//
//      53 / 8b dc               push %ebx ; mov %esp,%ebx
//      83 ec 08 / 83 e4 f0      sub $8 ; and $0xfffffff0,%esp
//      83 c4 08 / eb 03         add $8 ; jmp past the second entry
//      53 / 8b dc               <- the ALTERNATE entry, 0x1007350e
//
//  Two entry points into one body, the first aligning the stack to 16 and
//  the second assuming it is already aligned.  That is a compiler-generated
//  pair, not source, and until it is understood the body cannot be placed.
//  The body itself is a clean quaternion slerp: a four-term dot product of
//  the two inputs, a comparison against a global float that flips the sign
//  of all four components of one input, `acos` (0x10080230) and three
//  `sin`s (0x1007fdd0), and two four-iteration unrolled loops driven by the
//  `mov $0xfffffffc,%eax ; inc %eax ; jne` counting-up-to-zero idiom.
//
// ----------------------------------------------------------------------- //
//  0x10011620 (601 bytes, 3 callers) and 0x1006d550 (466 bytes, 4 callers)
//  -- TOO BIG FOR THIS BATCH, and both call into the register ABI.
//  0x10011620 walks a list at 0x1008d4c4 and calls 0x10011980 with three
//  register arguments (%eax = the node, %edx = a pointer, %ecx = a flag)
//  twice, so it has the same problem as 0x10037b10 at its call sites rather
//  than at its entry.  0x1006d550 is a surface Lock/Blt: it clamps two
//  dimensions against +0x48/+0x49 of its first argument, searches a
//  five-entry table at 0x10089820, then either Blts (COM slot +0x14 with a
//  0x64-byte DDBLTFX zeroed by memset) or Locks (slot +0x64 with a 0x7c-byte
//  DDSURFACEDESC2 and flags 0x801 = DDLOCK_WAIT|DDLOCK_SURFACEMEMORYPTR).
//  Both are ordinary work, just more of it than fits here.
//
// *********************************************************************** //
