// *********************************************************************** //
//
//  MODULE:   SMALL C (.CPP) -- d3d.ren, the 40-row small listing.
//
//  30 of the 40 rows are written here.  What the batch is made of, and what
//  is skipped and why, is in the two headers below.
//
//  WHAT THIS BATCH IS MOSTLY MADE OF
//
//  Sixteen of the rows are the allocator/forwarder family that
//  d3dbatchb.cpp, d3dbatchc.cpp, d3dr2a.cpp and d3dr2c.cpp have already
//  measured in four widths.  This file adds a FIFTH width the earlier
//  batches did not have: the ZERO-argument wrapper that supplies nothing
//  but the global allocator --
//
//      55 8b ec / 83 ec 10      frame
//      57                       push %edi                 ; 4-byte arg area
//      8b c1                    mov  %ecx,%eax
//      c7 04 24 <0x10091df0>    movl $&g_alloc,(%esp)
//      8b c8                    mov  %eax,%ecx
//      e8 <worker>              call
//      8b e5 / 5d / c3          ret                       ; NO `ret $N`
//
//  -- 0x1001c680, 0x10050690 and 0x1007ede0, byte for byte identical apart
//  from the call displacement.  `ret` with no pop and %ecx never loaded
//  from the stack is __thiscall with NO stack arguments, so the source is
//  `Term()` calling `Term(&g_alloc)`, and the three workers are exactly the
//  three that d3dbatchc.cpp already identified as SetSize(0, alloc):
//  0x1001cd10, 0x1002ec30 and 0x1007f3c0.  That is what pins the family.
//
//  A sixth width appears at 0x10050da0 and it is smaller still: `sub $0x8`
//  instead of `sub $0x10` and no `push %edi`, because the callee takes no
//  arguments at all.
//
//  TWO DESTRUCTORS AND A SCALAR-DELETING DESTRUCTOR
//
//  0x1007f3f0, 0x1007dc00 and 0x10035b10 all store a .data address into
//  their object's first dword and 0x10089470 / 0x10089a90 are both TABLES
//  OF CODE POINTERS (0x10089470 starts 0x10035b10 -- itself -- then two
//  copies of the CRT's 0x10080905; 0x10089a90 starts 0x1001d240,
//  0x1001c760, 0x1005a350, 0x1005a310, 0x1007ee20, 0x1007ee00).  They are
//  vptr stores.  They are written here as ordinary members named after
//  their own address, with the vptr assigned explicitly, for the reason
//  d3dbatchb.cpp gives for 0x1007dcc4: a real `~Class()` mangles without an
//  address and could not be placed.
//
//  0x10035b10 and 0x1007dc00 both end `test $0x1,%al ; jne -> push this ;
//  call 0x10080479 ; ret $0x4`, and 0x10080479 is `push 0x4(%esp) ; call
//  0x10080b8e ; pop %ecx ; ret` -- a one-argument __cdecl forwarder into
//  the CRT's free().  That is `operator delete`, and the flag-bit-0 test in
//  front of it is the scalar deleting destructor's `__flags & 1`.
//
//  WHAT IS SKIPPED, AND WHY
//
//   * (Since written -- w57_1007e830_osoiqip.cpp and w57_1007bb20_qipni.cpp
//     -- as the file-scope objects icl generates them from.)
//     0x1007e830 and 0x1007bb20 are REGISTER-ABI CALLERS.  0x1007e830 calls
//     0x1007e0a4, which is four bytes past 0x1007e0a0's `mov 0x4(%esp),%eax`
//     front; 0x1007bb20 calls 0x1007b984, four bytes past 0x1007b980's.
//     Both are static-initialiser thunks (`ctor(&global) ; atexit(dtor)` --
//     0x10080502 is atexit: it forwards to 0x10080484 and turns the result
//     into 0 or -1).  The thunk itself is writable; the call into the
//     register entry is not, and defining the callee
//     in the same file does not fix it.
//
//   * 0x1005063c is not a function.  It is the out-of-line error tail of
//     the body that starts at 0x100505a0 -- it opens `push %edi ; mov
//     -0x8(%ebp),%ebx` with no prologue of its own, restores %ebx/%esi/%edi
//     from an outgoing area it never wrote, and its twin at 0x1005053c is
//     the identical tail of the previous function.  The enumeration split
//     it off; there is no source for it on its own.
//
//   * 0x1001cd40, 0x1001d810 and 0x1007e9f0 are NOT 37/36/37 bytes.  All
//     three run on past the enumerated end into a 200-400 byte
//     insert-into-sorted-array body (0x1001cd40 reaches 0x1001cea0), and
//     0x1001d9c0, 0x1007f5a0, 0x1001da90 and 0x1005a1d0 are four ~190-byte
//     iterator walks in two identical pairs.  They are real functions and
//     they are worth a batch of their own; they are not small ones.
//
//   * 0x10043a30 IS 73 bytes and is written; the 832 in the listing is the
//     distance to the next enumerated start, not its length.
//
// *********************************************************************** //

#include "ltbasedefs.h"


// ======================================================================= //
//  SHARED DECLARATIONS
// ======================================================================= //

// 0x10091df0 -- the global allocator OBJECT, not a pointer to one.  Only its
// ADDRESS is used from this file, so it is opaque here, exactly as
// d3dr2c.cpp models it.
struct C10091df0;                                       // opaque
extern C10091df0 g_10091df0;                            // 0x10091df0

// The CRT's free-forwarder at 0x10080479, reached as `operator delete`.
// Declared implicitly by the language; nothing to write here.


// ======================================================================= //
//  GROUP 1 -- FRAMELESS ACCESSORS.
//
//  All four are `push %esi ... add $0x4,%esp ; ret`: no frame, no stack
//  arguments, %ecx used as `this`, and the four-byte slot opened by the
//  push is never written.  That dead slot is icl's, not the source's.
// ======================================================================= //

// ----------------------------------------------------------------------- //
//  0x10079f70   15 bytes.  1 caller.
//
//      56                       push %esi                 ; dead slot
//      8b 49 04                 mov  0x4(%ecx),%ecx
//      8b 04 8d <0x10089870>    mov  g[%ecx*4],%eax
//      83 c4 04 / c3            ret
//
//  A dword member at +0x04 used to subscript a global dword table: the
//  scale of 4 in the SIB byte is the element width and there is no bounds
//  test, so this is a plain lookup.
// ----------------------------------------------------------------------- //
extern uint32 g_10089870[];                             // 0x10089870

class C10079f70
{
public:
	uint32	sub_10079f70();

	uint32	m_n00;                                      // +0x00
	uint32	m_nIndex04;                                 // +0x04
};

uint32 C10079f70::sub_10079f70()
{
	return g_10089870[m_nIndex04];
}


// ----------------------------------------------------------------------- //
//  0x1003abd0   12 bytes of body, then a four-byte lea-nop.
//
//      56 / 8b 49 18            this = this->m_p18
//      0f b7 41 2a              movzwl 0x2a(%ecx),%eax
//      83 c4 04 / c3            ret
//
//  `movzwl` fixes the member as a 16-bit unsigned one at +0x2a, and
//  0x1003abe4 -- the constructor twenty bytes further on -- writes exactly
//  +0x28 and +0x2a with `mov %ax,...`, which is the same pair of words.
// ----------------------------------------------------------------------- //
struct Sub1003abd0
{
	uint8	m_Reserved00[0x2a];                         // +0x00..+0x29
	uint16	m_n2a;                                      // +0x2a
};

class C1003abd0
{
public:
	uint32	sub_1003abd0();

	uint8			m_Reserved00[0x18];                 // +0x00..+0x17
	Sub1003abd0*	m_p18;                              // +0x18
};

uint32 C1003abd0::sub_1003abd0()
{
	return m_p18->m_n2a;
}


// ----------------------------------------------------------------------- //
//  0x10043640   15 bytes of body, then eleven bytes of lea-nop.  1 caller.
//
//      56                       push %esi                 ; dead slot
//      0f b7 41 28              movzwl 0x28(%ecx),%eax     ; the index
//      8b 49 1c                 mov  0x1c(%ecx),%ecx       ; the owner
//      c1 e0 05                 shl  $0x5,%eax             ; * 0x20
//      03 81 68 01 00 00        add  0x168(%ecx),%eax      ; + the base
//      83 c4 04 / c3            ret
//
//  `shl $0x5` then `add <pointer member>` is `&base[i]` with a 32-byte
//  element; the index is a 16-bit member of THIS object and the array
//  belongs to the object at +0x1c.
// ----------------------------------------------------------------------- //
struct Elem10043640                                     // 0x20 bytes
{
	uint32	m_n[8];
};

struct Owner10043640
{
	uint8			m_Reserved000[0x168];               // +0x000..+0x167
	Elem10043640*	m_pElements;                        // +0x168
};

class C10043640
{
public:
	Elem10043640*	sub_10043640();

	uint8			m_Reserved00[0x1c];                 // +0x00..+0x1b
	Owner10043640*	m_pOwner;                           // +0x1c
	uint8			m_Reserved20[0x28 - 0x20];          // +0x20..+0x27
	uint16			m_nIndex28;                         // +0x28
};

Elem10043640* C10043640::sub_10043640()
{
	return &m_pOwner->m_pElements[m_nIndex28];
}


// ----------------------------------------------------------------------- //
//  0x1006c6d0   10 bytes of body, then a lea-nop and four 0x90.
//
//      56 / b8 40 00 00 00 / 83 c4 04 / c3
//
//  A constant 0x40, nothing read, nothing written.  %ecx is untouched, but
//  it IS a member: the `push %esi` / `add $0x4,%esp` pair is the unused
//  stack slot icl gives the __thiscall register parameter (`this`).  A
//  free function is the bare `mov ; ret` (8 bytes); a member, or a
//  one-parameter __fastcall, emits the shipped ten.
// ----------------------------------------------------------------------- //
class C1006c6d0
{
public:
	uint32	sub_1006c6d0();
};

uint32 C1006c6d0::sub_1006c6d0()
{
	return 0x40;
}


// ======================================================================= //
//  GROUP 2 -- FRAMELESS SETTERS AND STUBS.
// ======================================================================= //

// ----------------------------------------------------------------------- //
//  0x10050770   18 bytes, then two lea-nops.
//
//      8b 44 24 04              mov  0x4(%esp),%eax        ; the pointer
//      56                       push %esi                  ; dead slot
//      33 d2                    xor  %edx,%edx
//      89 50 04                 mov  %edx,0x4(%eax)        ; +0x04 FIRST
//      89 10                    mov  %edx,(%eax)           ; then +0x00
//      83 c4 04 / c2 04 00      ret  $0x4
//
//  %ecx is never used and the argument comes off the stack, so this is not
//  __thiscall.  The store ORDER is the whole content: +0x04 before +0x00 is
//  what a chained assignment compiles to, right to left -- the same
//  evidence d3dr2a.cpp used to read CheapLTLink::TieOff().
// ----------------------------------------------------------------------- //
struct Pair10050770
{
	uint32	m_n00;                                      // +0x00
	uint32	m_n04;                                      // +0x04
};

// NOT CLOSED (measured): the shipped bytes open with
// `mov 0x4(%esp),%eax` and then `push %esi` -- the /Qip two-entry shape
// (a stack adapter falling into a register entry that takes pPair in %eax
// and reserves a slot).  As a __thiscall member the slot appears but the
// argument is loaded after it (13 bytes differ, worse than this spelling's
// 10), so the function wants a /Qip translation unit.
void __stdcall sub_10050770(Pair10050770* pPair)
{
	pPair->m_n00 = pPair->m_n04 = 0;
}


// ----------------------------------------------------------------------- //
//  0x10059c70   7 bytes.
//
//      56 / 83 c4 04 / c2 04 00
//
//  One stack argument, popped, and no body at all -- the dead slot is the
//  only thing between the entry and the return.  A do-nothing override.
// ----------------------------------------------------------------------- //
class C10059c70
{
public:
	void	sub_10059c70(uint32 nArg);
};

void C10059c70::sub_10059c70(uint32 /*nArg*/)
{
}


// ----------------------------------------------------------------------- //
//  0x10015e20   10 bytes of body, then `mov %esi,%esi` and four 0x90.
//
//      56 / e8 <0x100039d0> / 83 c4 04 / c3
//
//  0x100039d0 opens `push %ebp ; mov %esp,%ebp` and never reads 0x8(%ebp),
//  so it takes no arguments; nothing is stored into the four bytes the
//  push opens and nothing is returned.  A one-line forwarder.
// ----------------------------------------------------------------------- //
extern void sub_100039d0();                             // 0x100039d0

void sub_10015e20()
{
	sub_100039d0();
}


// ======================================================================= //
//  GROUP 3 -- THE ALLOCATOR WRAPPERS, ZERO-ARGUMENT WIDTH.
//
//  See the module header.  Each gets its own class because each reaches a
//  different worker and nothing proves any two of them are the same type.
// ======================================================================= //

class C1001c680
{
public:
	void	sub_1001cd10(C10091df0* pAlloc);            // 0x1001cd10
	void	sub_1001c680();
};

void C1001c680::sub_1001c680()
{
	sub_1001cd10(&g_10091df0);
}


class C10050690
{
public:
	LTBOOL	sub_1002ec30(C10091df0* pAlloc);            // 0x1002ec30
	LTBOOL	sub_10050690();
};

LTBOOL C10050690::sub_10050690()
{
	return sub_1002ec30(&g_10091df0);
}


class C1007ede0
{
public:
	LTBOOL	sub_1007f3c0(C10091df0* pAlloc);            // 0x1007f3c0
	LTBOOL	sub_1007ede0();
};

LTBOOL C1007ede0::sub_1007ede0()
{
	return sub_1007f3c0(&g_10091df0);
}


// ----------------------------------------------------------------------- //
//  0x10050da0   19 bytes, then nine 0x90.  1 caller.
//
//      55 8b ec / 83 ec 08      frame -- EIGHT, not sixteen
//      8b c1 / 8b c8            the %ecx round trip
//      e8 <0x10050d30>          call
//      8b e5 / 5d / c3          ret
//
//  It is ObjectBank<Node56ab4>::AllocVoid (`return Allocate();`, slot +0x04
//  of the bank's vtable 0x100897d0), and is now the template's own,
//  byte-identical: d3dr5b.cpp instantiates it at /Os /Qip.  The stand-in
//  C10050da0::sub_10050da0 that was defined here was a second copy.
// ----------------------------------------------------------------------- //


// ======================================================================= //
//  GROUP 4 -- THE ONE-ARGUMENT FORWARDER, d3dbatchb.cpp's 0x1007c870 shape.
//
//      55 8b ec / 83 ec 10 / 57         frame + 4-byte arg area
//      8b 45 08 / 89 04 24              the one argument, straight through
//      8b c1 / 8b c8                    the %ecx round trip
//      e8 <worker> / 8b e5 / 5d         call
//      c2 04 00                         ret $0x4
// ======================================================================= //

class C1001cf40
{
public:
	void	sub_1001dc50(uint32 nArg);                  // 0x1001dc50
	void	sub_1001cf40(uint32 nArg);
};

void C1001cf40::sub_1001cf40(uint32 nArg)
{
	sub_1001dc50(nArg);
}


class C1001dbb0
{
public:
	void	sub_1001e040(uint32 nArg);                  // 0x1001e040
	void	sub_1001dbb0(uint32 nArg);
};

void C1001dbb0::sub_1001dbb0(uint32 nArg)
{
	sub_1001e040(nArg);
}


class C1005a2f0
{
public:
	void	sub_1005a3e0(uint32 nArg);                  // 0x1005a3e0
	void	sub_1005a2f0(uint32 nArg);
};

void C1005a2f0::sub_1005a2f0(uint32 nArg)
{
	sub_1005a3e0(nArg);
}


// ----------------------------------------------------------------------- //
//  0x1001e0c0   28 bytes, then four 0x90.  1 caller.
//
//  The same shape.  0x1001e0a0 is `mov 0x4(%ecx),%ecx ; mov 0x8(%esp),%eax
//  ; lea (%ecx,%eax,4),%eax` -- `&this->m_p04[i]` over a dword array -- so
//  the argument is an index and the answer is a pointer.
// ----------------------------------------------------------------------- //
class C1001e0c0
{
public:
	uint32*	sub_1001e0a0(uint32 nIndex);                // 0x1001e0a0
	uint32*	sub_1001e0c0(uint32 nIndex);

	uint32  m_nUnknown00;                               // +0x00
	uint32* m_pArray04;                                 // +0x04
};

uint32* C1001e0c0::sub_1001e0c0(uint32 nIndex)
{
	return sub_1001e0a0(nIndex);
}


// ----------------------------------------------------------------------- //
//  0x1001e0a0   17 bytes, then `8d 76 00` and 0x90 to 0x1001e0c0.  1 caller
//  (0x1001e0c0 above, `call 0x1001e0a0` at 0x1001e0d1).
//
//      56 / 8b 49 04 / 8b 44 24 08 / 8d 04 81 / 83 c4 04 / c2 04 00
//
//  Byte for byte the body of 0x10009d40 (d3dbatcha.cpp), and the image
//  carries BOTH copies -- two functions in the source, not one folded by
//  the linker -- so this one is written out too.  It used to be linked
//  as an /ALTERNATENAME onto 0x10009d40, which made the call resolve but
//  left the shipped 0x1001e0a0 with no function of ours to score.
//  Defined AFTER its caller so that /Os does not inline it (the shipped
//  0x1001e0c0 calls it).
// ----------------------------------------------------------------------- //
uint32* C1001e0c0::sub_1001e0a0(uint32 nIndex)
{
	return m_pArray04 + nIndex;
}


// ----------------------------------------------------------------------- //
//  0x1007ee00   28 bytes, then four 0x90.
//
//  The same shape but `ret $0x8`: TWO stack arguments, of which the second
//  is never loaded.  A dead parameter, which is
//  ordinary.  0x1007ee00 is slot +0x14 of the vtable at 0x10089a90, so the
//  signature is fixed by the interface and not by this body.
//
//  0x1007f120 supplies &g_10091df0 itself, so nothing but the first
//  argument travels.
// ----------------------------------------------------------------------- //
class C1007ee00
{
public:
	void	sub_1007f120(uint32 nArg);                  // 0x1007f120
	void	sub_1007ee00(uint32 nArg, uint32 nUnused);
};

void C1007ee00::sub_1007ee00(uint32 nArg, uint32 /*nUnused*/)
{
	sub_1007f120(nArg);
}


// ======================================================================= //
//  GROUP 5 -- TWO-ARGUMENT FORWARDERS THAT SUPPLY A MEMBER.
// ======================================================================= //

// ----------------------------------------------------------------------- //
//  0x10050800   35 bytes of body, then two lea-nops.  1 caller (0x100506d0,
//               below).
//
//      55 8b ec / 83 ec 10      frame
//      8b c1                    mov  %ecx,%eax
//      83 c4 f8                 add  $0xfffffff8,%esp      ; 8-byte area
//      8b 50 08                 mov  0x8(%eax),%edx        ; this->m_n08
//      8b 4d 08                 mov  0x8(%ebp),%ecx        ; the argument
//      89 14 24                 mov  %edx,(%esp)           ; callee arg 1
//      89 4c 24 04              mov  %ecx,0x4(%esp)        ; callee arg 2
//      8b c8 / e8 <0x1002ec00>  call
//      8b e5 / 5d / c2 04 00    ret $0x4
//
//  The member goes in FIRST and our argument second.  0x1001e040 and
//  0x1005a3e0 are the same function over two other classes, which is what
//  says the shape is the source's and not a schedule.
// ----------------------------------------------------------------------- //
class C10050800
{
public:
	void	sub_1002ec00(uint32 nA, uint32 nB);         // 0x1002ec00
	void	sub_10050800(uint32 nArg);
	void	sub_100506d0(uint32 nArg);

	uint8	m_Reserved00[0x08];                         // +0x00..+0x07
	uint32	m_n08;                                      // +0x08
};

void C10050800::sub_10050800(uint32 nArg)
{
	sub_1002ec00(m_n08, nArg);
}

// 0x100506d0 -- 28 bytes, the plain one-argument forwarder into it.  The
// call is to 0x10050800 itself, not past it, so this pair is an ordinary
// stack call even though both halves live in one translation unit.
void C10050800::sub_100506d0(uint32 nArg)
{
	sub_10050800(nArg);
}


// ======================================================================= //
//  GROUP 6 -- THE DESTRUCTORS.
// ======================================================================= //

extern uint8 g_10089a90[];                              // vtable, 0x10089a90
extern uint8 g_10089470[];                              // vtable, 0x10089470

// ----------------------------------------------------------------------- //
//  0x1007f3f0   33 bytes of body, then a lea-nop and twelve 0x90.
//               1 caller (0x1007dc00, below).
//
//      55 8b ec / 83 ec 10      frame
//      8b c1                    mov  %ecx,%eax
//      57                       push %edi                  ; 4-byte area
//      8b c8                    mov  %eax,%ecx
//      c7 04 24 <0x10091df0>    movl $&g_alloc,(%esp)      ; hoisted
//      c7 00 <0x10089a90>       movl $vtbl,(%eax)          ; the vptr
//      e8 <0x1007f3c0>          call
//      8b e5 / 5d / c3          ret
//
//  WHAT PINS IT: 0x10089a90 holds 0x1001d240, 0x1001c760, 0x1005a350,
//  0x1005a310, 0x1007ee20, 0x1007ee00 -- six code addresses -- so the store
//  into (%eax) is a vptr store, and re-pointing the vptr at the base table
//  and then tearing an array down is a destructor and nothing else.
//  0x1007f3c0 is d3dbatchc.cpp's SetSize(0, alloc), i.e. the array's
//  Term(), and it takes the SAME `this`, so the array is the object.
//
//  The allocator store is emitted before the vptr store; d3dr2a.cpp already
//  measured that the outgoing-area constant is scheduled first regardless
//  of source order, so the source order is the natural one: vptr, then the
//  teardown.
// ----------------------------------------------------------------------- //
class C1007f3f0
{
public:
	LTBOOL	sub_1007f3c0(C10091df0* pAlloc);            // 0x1007f3c0
	void	sub_1007f3f0();

	void*	m_pVtbl;                                    // +0x00
	void*	m_pData;                                    // +0x04
	uint32	m_nSize;                                    // +0x08
};

void C1007f3f0::sub_1007f3f0()
{
	m_pVtbl = g_10089a90;

	sub_1007f3c0(&g_10091df0);
}


// ----------------------------------------------------------------------- //
//  0x1007dc00   43 bytes.  1 caller.
//
//      55 8b ec / 83 ec 10 / 57 / 53      frame, dead slot, %ebx saved
//      8b d9                              this
//      b9 3c 00 00 00 / 03 cb             %ecx = this + 0x3c
//      e8 <0x1007f3f0>                    the member destructor above
//      8b cb / e8 <0x1007dcc4>            this->sub_1007dcc4()
//      8b 45 08 / a8 01                   test $0x1,%al on the flags
//      75 07                              jne  -> delete
//      5b / 8b e5 / 5d / c2 04 00         ret  $0x4
//      57 / 53 / e8 <0x10080479>          push this ; operator delete
//
//  `mov $0x3c,%ecx ; add %ebx,%ecx` rather than `lea 0x3c(%ebx),%ecx` is
//  icl's, not the source's -- the same address arithmetic appears at
//  0x100158de as `mov $0x188,%ecx ; add %ebx,%ecx`.
//
//  0x1007dcc4 is already reconstructed in d3dbatchb.cpp as a destructor
//  BODY taking no stack arguments, so this is the derived layer: destroy
//  the member at +0x3c, run the base body, then honour the delete flag.
// ----------------------------------------------------------------------- //
class C1007dc00
{
public:
	void	sub_1007dcc4();                             // 0x1007dcc4
	void	sub_1007dc00(uint32 nFlags);

	uint8		m_Reserved00[0x3c];                     // +0x00..+0x3b
	C1007f3f0	m_Member3c;                             // +0x3c
};

void C1007dc00::sub_1007dc00(uint32 nFlags)
{
	m_Member3c.sub_1007f3f0();

	sub_1007dcc4();

	if ((nFlags & 1) == 0)
		return;

	::operator delete(this);
}


// ----------------------------------------------------------------------- //
//  0x10035b10   36 bytes.
//
//      83 ec 0c                 sub  $0xc,%esp             ; no frame
//      8b 44 24 10              mov  0x10(%esp),%eax       ; the flags
//      c7 01 <0x10089470>       movl $vtbl,(%ecx)          ; the vptr
//      a8 01 / 75 06            test $0x1,%al ; jne
//      83 c4 0c / c2 04 00      ret  $0x4
//      89 0c 24                 mov  %ecx,(%esp)           ; `this`
//      e8 <0x10080479>          operator delete
//      eb f0                    jmp  back to the ret
//
//  0x10089470's first entry IS 0x10035b10, so this function is slot 0 of
//  its own vtable -- which is where a scalar deleting destructor lives.
//  Note the shared exit: the delete arm jumps BACKWARD into the return, so
//  the no-delete case is the fall-through and the guard is one positive
//  condition at the end of the body.
// ----------------------------------------------------------------------- //
class C10035b10
{
public:
	void	sub_10035b10(uint32 nFlags);

	void*	m_pVtbl;                                    // +0x00
};

void C10035b10::sub_10035b10(uint32 nFlags)
{
	m_pVtbl = g_10089470;

	if ((nFlags & 1) == 0)
		return;

	::operator delete(this);
}


// ======================================================================= //
//  GROUP 7 -- THE REST.
// ======================================================================= //

// ----------------------------------------------------------------------- //
//  0x1007f7b0   37 bytes of body, then a lea-nop and two 0x90.  1 caller.
//
//      55 8b ec / 83 ec 10 / 57 / 53      frame, dead slot, %ebx saved
//      8b 5d 08                 mov  0x8(%ebp),%ebx        ; the argument
//      57                       push %edi                  ; 4-byte area
//      8b 03 / 89 04 24         *pArg into the outgoing slot
//      8b c1 / 8b c8            the %ecx round trip
//      e8 <0x10009d40>          call
//      ff 03                    incl (%ebx)                ; AFTER the call
//      8b 00                    mov  (%eax),%eax           ; the answer
//      5b / 8b e5 / 5d / c2 04 00
//
//  0x10009d40 is `&this->m_p04[i]` over a dword array (d3dbatchb.cpp
//  identifies it), so the argument is a pointer to a running index: read
//  it, subscript with it, THEN step it, then return the element.  The
//  increment is emitted after the call, which is why it is written as its
//  own statement rather than as `m_nIndex++` inside the call.
// ----------------------------------------------------------------------- //
struct Cursor1007f7b0
{
	uint32	m_nIndex;                                   // +0x00
};

class C1007f7b0
{
public:
	uint32*	sub_10009d40(uint32 nIndex);                // 0x10009d40
	uint32	sub_1007f7b0(Cursor1007f7b0* pCursor);
};

uint32 C1007f7b0::sub_1007f7b0(Cursor1007f7b0* pCursor)
{
	uint32* pSlot = sub_10009d40(pCursor->m_nIndex);

	pCursor->m_nIndex++;

	return *pSlot;
}


// 0x1007e7c0 moved to d3dr4b.cpp (also /Os): its inline accessors turn
// on icl's inliner for this TU, which then folds 0x10050800 into 0x100506d0
// and 0x1007f3f0 into 0x1007dc00 (both measured, both lose identity).


// ----------------------------------------------------------------------- //
//  0x10023200   40 bytes, then eight 0x90.
//
//      55 8b ec / 83 ec 10      frame
//      8d 45 f8 / 50            lea  -0x8(%ebp),%eax ; push
//      ff 15 <KERNEL32 IAT>     QueryPerformanceFrequency
//      c7 05 <0x10089410> 1     movl $0x1,g_lo
//      c7 05 <0x10089414> 0     movl $0x0,g_hi
//      8b e5 / 5d / c3          ret
//
//  The eight-byte local the frequency is read into is never looked at
//  again, and the two globals are set to a literal 1/0 pair regardless.
//  That is a dead store through memory, which icl's /O2 does not eliminate,
//  so it is written
//  longhand.  The call is `ff 15` through the import table, which is what
//  __declspec(dllimport) emits -- d3dbatcha.cpp declares ShowCursor the
//  same way.
// ----------------------------------------------------------------------- //
extern "C" __declspec(dllimport) int __stdcall
	QueryPerformanceFrequency(void* pCount);

extern uint32 g_10089410;                               // 0x10089410
extern uint32 g_10089414;                               // 0x10089414

void sub_10023200()
{
	__int64 nFrequency;

	QueryPerformanceFrequency(&nFrequency);

	g_10089410 = 1;
	g_10089414 = 0;
}


// ----------------------------------------------------------------------- //
//  0x10068c50   48 bytes.  1 caller.
//
//      55 8b ec / 83 ec 10      frame
//      83 c4 f0                 add  $0xfffffff0,%esp      ; 0x10-byte area
//      33 c0                    the shared zero
//      c7 44 24 04 3f800000     arg 2 = 1.0f
//      c7 04 24 <"DrawWorldModels">   arg 1
//      b9 <0x1009110c>          `this`
//      89 44 24 08 / 89 44 24 0c      args 3 and 4 = 0
//      e8 <0x10021f40>          call
//      8b e5 / 5d / c3          ret
//
//  This is the console-variable registration shape and there are dozens of
//  it in the image: 0x10009860 is byte for byte the same function with
//  "ModelMinTri", 0.2f and the bank at 0x1008ab90.  The float goes in as an
//  immediate bit pattern because that is how icl materialises a float
//  literal argument, not because the parameter is an integer.
// ----------------------------------------------------------------------- //
struct Bank10021f40
{
	void	sub_10021f40(const char* pszName, float fValue,
	                     uint32 nA, uint32 nB);         // 0x10021f40
};

extern Bank10021f40 g_1009110c;                         // 0x1009110c

void sub_10068c50()
{
	g_1009110c.sub_10021f40("DrawWorldModels", 1.0f, 0, 0);
}


// 0x10043a30 moved to d3dr5a_o2.cpp: it is /O2 (each guard duplicates the
// epilogue, lea-nop padding), and there it is byte-identical.


// ----------------------------------------------------------------------- //
//  0x1001d1c0   50 bytes of body, then `mov %esi,%esi` and 0x90 padding.
//
//      55 8b ec / 83 ec 10      frame
//      57 57 56 53              dead slot, then %edi/%esi/%ebx saved
//      8b 45 0c / 8b 00 / 50    push *(uint32*)arg2
//      8b c1 / 8b c8            the %ecx round trip
//      e8 <0x10009ef0>          call
//      8b 5d 08                 mov  0x8(%ebp),%ebx        ; the hidden ret
//      8b f0 / 8b fb            %esi = source, %edi = dest
//      b9 08 00 00 00 / f3 a5   rep movsl, EIGHT dwords
//      8b c3                    return the hidden pointer
//      5b 5e 5f / 8b e5 / 5d / c2 08 00
//
//  WHAT PINS IT: `ret $0x8` with `this` in %ecx is two stack dwords, and
//  the FIRST of them is written by `rep movsl` and then returned in %eax --
//  that is MSVC's hidden return pointer for a struct returned by value, and
//  it is the lowest stack argument for a __thiscall member.  0x10009ef0 is
//  `&this->m_p04[i]` with `shl $0x5`, a 32-byte element, and 8 dwords is
//  exactly 32 bytes: the source dereferences the subscript and copies it
//  out.
// ----------------------------------------------------------------------- //
struct Elem10009ef0                                     // 0x20 bytes
{
	uint32	m_n[8];
};

struct Key1001d1c0
{
	uint32	m_nIndex;                                   // +0x00
};

class C1001d1c0
{
public:
	Elem10009ef0*	sub_10009ef0(uint32 nIndex);        // 0x10009ef0
	Elem10009ef0	sub_1001d1c0(const Key1001d1c0* pKey);
};

Elem10009ef0 C1001d1c0::sub_1001d1c0(const Key1001d1c0* pKey)
{
	return *sub_10009ef0(pKey->m_nIndex);
}


// ----------------------------------------------------------------------- //
//  0x100591a0   50 bytes of body, then eight 0x90.
//
//      53                       %ebx saved
//      8b 5c 24 10              arg 3
//      8b 44 24 0c              arg 2
//      8b 54 24 08              arg 1
//      83 ec 10                 the locals
//      d9 84 d9 70 07 00 00     flds  0x770(%ecx,%ebx,8)
//      d8 4a 04                 fmuls 0x4(%edx)
//      89 7c 24 08              mov   %edi,0x8(%esp)        ; dead store
//      d9 18                    fstps (%eax)
//      d9 84 d9 74 07 00 00     flds  0x774(%ecx,%ebx,8)
//      d8 4a 08                 fmuls 0x8(%edx)
//      d9 58 04                 fstps 0x4(%eax)
//      83 c4 10 / 5b / c2 0c 00 ret  $0xc
//
//  WHAT PINS IT: `this` in %ecx with a `(%ecx,%ebx,8)` index says a
//  two-float table at +0x770 subscripted by the third argument; the source
//  reads +0x04 and +0x08 of the first argument and writes +0x00 and +0x04
//  of the second, so the second is the destination.  `mov %edi,0x8(%esp)`
//  into a slot nothing reads is icl's own register save, not a local.
// ----------------------------------------------------------------------- //
struct Scale100591a0                                    // 8 bytes
{
	float	m_fX;                                       // +0x00
	float	m_fY;                                       // +0x04
};

struct Src100591a0
{
	float	m_f00;                                      // +0x00
	float	m_f04;                                      // +0x04
	float	m_f08;                                      // +0x08
};

struct Dest100591a0
{
	float	m_f00;                                      // +0x00
	float	m_f04;                                      // +0x04
};

class C100591a0
{
public:
	void	sub_100591a0(const Src100591a0* pSrc, Dest100591a0* pDest,
	                     uint32 nIndex);

	uint8			m_Reserved000[0x770];               // +0x000..+0x76f
	Scale100591a0	m_Scale770[1];                      // +0x770
};

void C100591a0::sub_100591a0(const Src100591a0* pSrc, Dest100591a0* pDest,
                             uint32 nIndex)
{
	pDest->m_f00 = m_Scale770[nIndex].m_fX * pSrc->m_f04;
	pDest->m_f04 = m_Scale770[nIndex].m_fY * pSrc->m_f08;
}


// ----------------------------------------------------------------------- //
//  0x10015910   50 bytes of body, then twelve 0x90.  1 caller.
//
//      55 8b ec / 83 ec 10 / 57 / 53      frame, dead slot, %ebx saved
//      8b 5d 08                 the argument
//      8b c1 / 53 / 8b c8       push it ; the %ecx round trip
//      e8 <0x100158d0>          call
//      85 c0 / 74 15            je  -> the zero arm, out of line
//      8b 48 04                 mov  0x4(%eax),%ecx        ; the object
//      57                       push %edi                  ; 4-byte area
//      8b 01                    mov  (%ecx),%eax           ; its vptr
//      89 1c 24                 mov  %ebx,(%esp)           ; the argument
//      8b 40 0c / ff d0         vtable slot +0x0c
//      5b / 8b e5 / 5d / c2 04 00
//   L: 33 c0 / eb f5            xor %eax,%eax ; jmp back into the epilogue
//
//  `mov (%ecx),%eax ; mov 0xc(%eax),%eax ; call *%eax` is icl's real C++
//  virtual call, not the function-pointer-struct form, so the object at
//  +0x04 of the lookup result is a polymorphic C++ object and +0x0c is its
//  fourth slot.
//
//  WHAT PINS THE SHAPE: the shared exit is hosted at the END of the
//  successful path and the zero return jumps BACKWARD into it.  Per the
//  shared-exit lever that is ONE positive condition wrapping the
//  body, not a guarded early return -- 0x1001da90 shows the other
//  arrangement, where the zero arm is inline and the body jumps back.
// ----------------------------------------------------------------------- //
class IObjBase10015910
{
public:
	virtual uint32	Slot00();
	virtual uint32	Slot04();
	virtual uint32	Slot08();
	virtual uint32	Slot0c(uint32 nArg);
};
class IObj10015910 : public IObjBase10015910
{
public:
	virtual uint32	Slot00();
	virtual uint32	Slot04();
	virtual uint32	Slot08();
	virtual uint32	Slot0c(uint32 nArg);
};

struct Ent10015910
{
	uint32			m_n00;                              // +0x00
	IObj10015910*	m_pObj;                             // +0x04
};

class C10015910
{
public:
	Ent10015910*	sub_100158d0(uint32 nArg);          // 0x100158d0
	uint32			sub_10015910(uint32 nArg);
};

// NEAR, one residual: the shipped code loads `mov 0x4(%eax),%ecx` BEFORE
// the `push %edi` that opens the outgoing area and the vptr load before the
// argument store; ours opens the area first.  Same bytes otherwise.  Tried,
// all emit the same order: an IObj* local, the early-return form, `?:`, an
// assignment inside the condition, `(*p).Slot0c`, an inline Ent accessor
// (GetObj), a void* / uint32 member cast to IObj*, an int parameter, and a
// four-byte struct argument (worse).  An in-class inline member on Ent also
// flips icl's inliner on for the whole TU (0x100506d0 and 0x1007dc00 lose).
// Flags measured on the whole file: /G6 (icl's default) and /G7 unchanged,
// /G5 worse (7 matched of 28); /Oa and /Ow are not icl 4.0 options.  The
// same vptr-load-above-the-argument-store swap is 0x100362a4's residual.
// 0x100158d0 beside it in one TU (identical there too) and
// the flag sets /Os /Oi-, /Os /Qip /Qip_no_inlining (same 9), /Os /Oi- /Qip
// (71), /Os /G5 (23); an out-of-class inline Ent::Call(n) wrapping the
// virtual call (same 9, and costs the file 2 identicals).  icl 4.0 rejects
// `__thiscall` on a function-pointer type, so d3dblit.cpp's slot-local
// lever (0x100368e0) cannot be tried here.
uint32 C10015910::sub_10015910(uint32 nArg)
{
	Ent10015910* pEnt = sub_100158d0(nArg);

	if (pEnt)
		return pEnt->m_pObj->Slot0c(nArg);

	return 0;
}


// ----------------------------------------------------------------------- //
//  0x1007a0d0   93 bytes of body, then a three-byte lea-nop.  1 caller.
//
//      55 8b ec / 83 ec 20 / 57 / 53      frame, dead slot, %ebx saved
//      8b d9                    this
//      b8 <0x10089390>          the table address, materialised ONCE
//      8b cb
//      89 03                    (%ebx)      = it
//      89 43 38                 0x38(%ebx)  = it
//      e8 <0x1007a3b0>          this->sub_1007a3b0()
//      83 c4 ec                 add $0xffffffec,%esp       ; five dwords
//      8d 4b 38                 %ecx = &this->m_38
//      ...0x1f, 0x2, 0x7e0, 0xf800, 0x0 into the outgoing area
//      8b 43 38 / 8b 00 / ff d0 the virtual call, slot 0
//      8b cb / e8 <0x1007a280>  this->sub_1007a280()
//      8b c3                    return this
//      5b / 8b e5 / 5d / c3
//
//  WHAT PINS IT: 0x10089390 holds ONE code address (0x1007a3f0) and then
//  ordinary data, so it is a one-slot vtable; the object it belongs to is
//  the one at +0x38, because `lea 0x38(%ebx),%ecx` is the `this` of the
//  call whose vptr is read from `0x38(%ebx)`.  The same address also goes
//  into (%ebx) -- two identical vtables folded to one COMDAT by the
//  linker, which is why this object and its member share it.
//
//  0xf800 / 0x07e0 / 0x001f with a leading 2 is RGB565 at two bytes a
//  pixel, so the call is a pixel-format setup; 0x1007a3b0 is the same call
//  with 3, 0xff0000, 0xff00, 0xff, i.e. the 32-bit case.
//
//  The two vptr stores are written through casts rather than as a real
//  constructor for the reason d3dbatchb.cpp gives for 0x1007dcc4: `??0`
//  mangles without an address.
// ----------------------------------------------------------------------- //
// Spelled as a table of code pointers (as d3dr4a.cpp spells 0x10089a20)
// so d3ddata.cpp can define it: slot 0 is 0x1007a3f0
// (w22_1007a3f0_os.cpp), a uint8[] spelling cannot carry the pointer.
extern void* g_vtbl_10089390[];                         // vtable, 0x10089390

class Fmt1007a0d0
{
public:
	virtual uint32	Slot00(uint32 nA, uint32 nB, uint32 nC,
	                       uint32 nD, uint32 nE);
};

class C1007a0d0
{
public:
	void		sub_1007a3b0();                         // 0x1007a3b0
	void		sub_1007a280();                         // 0x1007a280
	C1007a0d0*	sub_1007a0d0();

	uint8		m_Reserved00[0x38];                     // +0x00..+0x37
	Fmt1007a0d0	m_Fmt38;                                // +0x38
};

C1007a0d0* C1007a0d0::sub_1007a0d0()
{
	*(void**)this = g_vtbl_10089390;
	*(void**)&m_Fmt38 = g_vtbl_10089390;

	sub_1007a3b0();

	m_Fmt38.Slot00(2, 0, 0xf800, 0x07e0, 0x001f);

	sub_1007a280();

	return this;
}
