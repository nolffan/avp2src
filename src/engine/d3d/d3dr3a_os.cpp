// Split out of d3dr3a.cpp: these functions need the
// OTHER optimisation flag from the rest of that file.  d3d.ren is a
// mixed build and our file boundaries are invented.
// Everything that is not a function body is
// carried over verbatim so no declaration is lost.
// ********************************************************* //
// *********************************************************************** //
//
//  MODULE:   BATCH_A (.CPP)  --  d3d.ren, batch A
//
//  PURPOSE:  Reconstructed source for twelve of the twenty-five rows of
//            batch_a.txt.  Intel C/C++ 4.0, per-file /O2 or /Os.  Every
//            offset, constant and branch below was read off the retail d3d.ren.
//
//  THE FIND IN THIS BATCH: the two 800+ byte rows 0x10002d40 and 0x100030a0
//  are TWO MEMBERS OF A SIX-FUNCTION FAMILY of Sutherland-Hodgman frustum
//  clippers.  Their common caller 0x1000aa90 tests six bits of a clip mask
//  and dispatches one plane each:
//
//      bit 0x01 -> 0x10002310     bit 0x02 -> 0x10003410   (near, z)
//      bit 0x04 -> 0x10002670     bit 0x08 -> 0x100029e0   ( y <  z )
//      bit 0x10 -> 0x10002d40     bit 0x20 -> 0x100030a0   (-z <  y )
//
//  All six take the SAME four arguments and differ only in the plane
//  equation.  0x100029e0 (batch D) and 0x10003410 (batch B) are the same
//  source as the two written here with x/y/z permuted -- a normalised
//  disassembly diff of 0x100029e0 against 0x10002d40 differs in exactly
//  SIX LINES, all of them a +0x00 field where the other has +0x04.  If the
//  two below land, those two land with them.
//
//  A SECOND FIND, and it is a hard number: the flag array in those two
//  clippers is `int32 bInside[50]`, not 32 and not 64.  It is pinned
//  arithmetically, not guessed -- see the note on 0x10002d40.
//
//  A THIRD: 0x1006d3e0 is ltlink.h verbatim.  CheapLTLink::TieOff stores
//  m_pNext BEFORE m_pPrev because the header writes `m_pPrev = m_pNext =
//  this`, and AddAfter reloads pLink->m_pPrev out of memory before its last
//  store because the header writes `pLink->m_pPrev->m_pNext = ...`.  The
//  shipped bytes do both, in that order.
//
//  THIRTEEN ROWS ARE NOT WRITTEN.  The reasons are listed at the bottom.
//
// *********************************************************************** //

#include "ltbasedefs.h"
#include "ltlink.h"


// ----------------------------------------------------------------------- //
//  THE COM SURFACE THIS FILE TOUCHES
//
//  One object, 0x1008d988, already pinned by d3ddraw.cpp / d3dopt2d.cpp as
//  the Direct3D 7 device.  Only slot +0x50 (SetRenderState, index 20) is
//  reached from this file.  Copied verbatim from d3dopt2d.cpp so the two
//  files cannot disagree about the shape of the call.
// ----------------------------------------------------------------------- //
struct D3DDevA;

struct D3DDevAVtbl
{
	uint8 m_pad_00[0x50];
	// +0x50 -- IDirect3DDevice7::SetRenderState (index 20).
	long (__stdcall* SetRenderState)(D3DDevA*, uint32 dwState,
	                                 uint32 dwValue);
};

struct D3DDevA
{
	D3DDevAVtbl* lpVtbl;
};

// 0x1008d988 -- the device.
extern D3DDevA* g_pD3DDevice;

// D3DRENDERSTATETYPE members reached from this file.  The numbers are off
// the shipped bytes; the names are the DX7 enumerators for them.
#define D3DRS_A_ALPHATESTENABLE     15      // 0x0f
#define D3DRS_A_FOGENABLE           28      // 0x1c
#define D3DRS_A_ALPHAFUNC           25      // 0x19
#define D3DRS_A_COLORKEYENABLE      41      // 0x29
#define D3DCMP_A_GREATEREQUAL        5


// ----------------------------------------------------------------------- //
//  0x10023eb0 -- 132 bytes + 12 of lea-nop padding, 1 caller (0x1004fc05).
//  0x10023f40 -- 100 bytes + 12 of lea-nop padding, 1 caller (0x1004fc14).
//
//  A matched pair: the two calls sit nine instructions apart in the shipped
//  renderer's per-frame bracket (0x1004fbc0, also in this batch's list),
//  the first before the world is drawn and the second after.
//
//  WHAT PINS THEM: three-dword outgoing areas (`add $0xfffffff4,%esp`) with
//  `this` at (%esp) through `call *0x50(%eax)` -- SetRenderState -- and the
//  state numbers decode.  0x1008d5e4 selects between two ways of punching
//  transparent texels out: ALPHATESTENABLE(15)+ALPHAFUNC(25)=GREATEREQUAL
//  when it is set, COLORKEYENABLE(41) when it is not.  That is exactly the
//  DX7-vs-DX5 alpha-test/colour-key choice, and it is the only pair of
//  render states in the API that are alternatives for one another.
//
//  The tail of both is the same `sub_1000f630(n)` -- d3dneutral.cpp already
//  has it: a one-line writer for the dword at 0x1008add0, whose reader
//  sub_1000f640 is tested as a flag.  So the flag records "punch-through is
//  on", 1 on the way in and 0 on the way out.
//
//  SHARED EXIT: 0x10023eb0's two arms both fall into the sub_1000f630 call
//  at the END, so the source is one if/else wrapping the state changes with
//  the flag write after it, NOT two returns.
// ----------------------------------------------------------------------- //

extern uint32 g_1008d5e4;                               // 0x1008d5e4

void sub_1000f630(uint32 nValue);                       // d3dneutral.cpp


// 0x10023eb0 -- turn punch-through transparency ON.


// 0x10023f40 -- turn it OFF again.  Note that the "off" arm does NOT
// restore ALPHAFUNC: the shipped code writes only the two enables.


// ----------------------------------------------------------------------- //
//  0x10033120 -- 59 bytes + padding, 1 caller (0x100219af).
//
//  WHAT PINS IT: `sub $8,%esp ; movl $8,(%esp) ; call 0x10081222 ;
//  add $8,%esp ; test %eax,%eax ; je ; mov %eax,%ecx ; call 0x10036320`
//  is the canonical `new T` sequence -- 0x10081222 is `push $1 ;
//  push size ; call 0x10080a0a` i.e. operator new, and 0x10036320 is a
//  constructor (its first instruction is `movl $0x100894a0,(%ebx)`, a
//  vtable store, with %ecx as `this`).  Eight bytes, so a vtable pointer
//  and one dword.  The result is DISCARDED -- there is no store anywhere
//  in the function -- so the object registers itself from its own
//  constructor, which is what 0x10036320 does (it news a 12-byte node and
//  hangs it off 0x100911b8).
//
//  `mov $0x1,%eax` rather than `xor %eax,%eax ; inc %eax` for the answer:
//  this translation unit is one of the /Os ones.
// ----------------------------------------------------------------------- //

class C10036320
{
public:
			C10036320();                                // 0x10036320

	void   *m_pVTable;                                  // +0x00
	uint32	m_04;                                       // +0x04
};

void sub_1002e2d0();
void sub_1007b8c0();                                    // d3dneutral.cpp
void sub_100503c0();



// ----------------------------------------------------------------------- //
//  0x1006a6a0 -- 107 bytes, 1 caller (0x1004c918).  A ONE-ENTRY CACHE.
//
//  WHAT PINS IT: three globals move together.  0x100911a0 holds an object
//  pointer, 0x100911a4 and 0x100911a8 hold the two arguments that produced
//  it, and all three are rewritten in one block.  The guard reads
//  "0x100911a0 non-null AND both saved arguments match" and skips straight
//  to the shared `return 0x100911a0` at the end -- one positive condition
//  wrapping the body, which is why the last test jumps FORWARD to the exit
//  instead of each test owning a return.
//
//  The two calls are icl's real C++ virtual-call sequence,
//  `mov (%ecx),%eax ; mov N(%eax),%eax ; call *%eax`, on the object pointer
//  at 0x100911b8 (d3dneutral.cpp's sub_1006a780 is its getter) -- slot
//  +0x08 with one stack argument to release the old object, then slot
//  +0x04 with two to build the new one.  Callee-pops (__thiscall): the
//  outgoing areas are opened and never closed, and the two `pop`s at the
//  end land back on the saved registers.
//
//  The old pointer is released UNCONDITIONALLY, null or not.
//
//  Its exact twin is 0x1006a710 (globals 0x100911ac / b0 / b4, caller
//  0x1004bdac); it is not on any listing and the source below is its
//  source too, with three names changed.
//
//  Both call sites push the same value TWICE (`push %eax ; push %eax` /
//  `push %edi ; push %edi`), so the two arguments are always equal at the
//  only sites that exist.  They are compared with an integer `cmp`, so they
//  are not floats.
// ----------------------------------------------------------------------- //

class CObj1006a6a0;

class CMgr1006a6a0
{
public:
	virtual void            vf_00();                            // +0x00
	virtual CObj1006a6a0*   vf_04(uint32 nA, uint32 nB);         // +0x04
	virtual void            vf_08(CObj1006a6a0 *pObj);           // +0x08
};

extern CMgr1006a6a0    *g_100911b8;                     // 0x100911b8
extern CObj1006a6a0    *g_100911a0;                     // 0x100911a0
extern uint32           g_100911a4;                     // 0x100911a4
extern uint32           g_100911a8;                     // 0x100911a8

CObj1006a6a0 *sub_1006a6a0(uint32 nA, uint32 nB)
{
	if (!g_100911a0 || g_100911a4 != nA || g_100911a8 != nB)
	{
		g_100911b8->vf_08(g_100911a0);

		g_100911a0 = g_100911b8->vf_04(nA, nB);
		g_100911a4 = nA;
		g_100911a8 = nB;
	}

	return g_100911a0;
}


// ----------------------------------------------------------------------- //
//  0x1006d3e0 -- 122 bytes + padding, 1 caller (0x10039a28).
//
//  THIS IS ltlink.h.  Five iterations (%ebx walks 0..0x14 by 4 over a dword
//  array at 0x10091954, %ebp walks 0..0x3c by 12 over a 12-byte-element
//  array at 0x10091968 -- 12 bytes is sizeof(LTLink): m_pPrev, m_pNext,
//  m_pData).  Two induction variables over two arrays, which is what two
//  differently-typed arrays subscripted by the same i produce.
//
//  WHAT PINS THE HEADER CALLS, instruction for instruction:
//
//    CheapLTLink::TieOff  -- `m_pPrev = m_pNext = (LTLink*)this;`
//        stores +0x04 FIRST and +0x00 second.  The shipped code writes
//        0x10(%esp) then 0x0c(%esp).  Declaration order would be the other
//        way round; the chained assignment is what reverses it.
//
//    CheapLTLink::AddAfter --
//        pLink->m_pPrev = this;                       -> mov %eax,0x0c(%esp)
//        pLink->m_pNext = m_pNext;                    -> mov 0x4(%eax),%eax
//                                                        mov %eax,0x10(%esp)
//        pLink->m_pPrev->m_pNext = pLink->m_pNext->m_pPrev = pLink;
//                                                     -> mov %ecx,(%eax)
//                                                        mov 0x0c(%esp),%eax
//                                                        mov %edx,0x4(%eax)
//        That RELOAD of pLink->m_pPrev out of the local, three instructions
//        after storing it, is the header's third line written longhand --
//        icl 4.0 does not forward it.
//
//  The dword stored to 0x14(%esp) -- the third word of the LTLink, i.e.
//  m_pData -- is never read back, so it is the m_pData assignment and the
//  local link is only ever used as a splice.
// ----------------------------------------------------------------------- //

extern void   *g_10091954[5];                           // 0x10091954
extern LTLink  g_10091968[5];                           // 0x10091968

void sub_10039730(LTLink *pList);



// ----------------------------------------------------------------------- //
//  0x1001e520 -- 219 bytes + padding, 1 caller (0x10027841).
//
//  A 16-BIT FRAME STAMP AND ITS WRAPAROUND.  The whole body is
//
//      if (p->m_0c == 0xFFFF) { p->m_0c = 1; <clear every stamp>; }
//      p->m_0c++;
//
//  and the increment is SHARED: both the fast path and the tail of the
//  reset jump to the same four instructions at 0x1001e533, which reload
//  0x0c(%edi) rather than reusing the 1 that was just written.  That reload
//  is what says the increment is a separate statement after the `if`, not
//  part of either arm.
//
//  THREE CLEAR LOOPS, and the first and third are THE SAME LOOP.  Both walk
//  m_p88[0 .. m_n8c) with a 0x30-byte stride writing a zero word at +0x2c;
//  the middle one walks a pointer array m_pa0[0 .. m_na4) writing a zero
//  word at +0x42 through each pointer.  icl 4.0 does no dead-store
//  elimination through memory, so the duplicate survived /O2 in the shipped
//  image and it is written out longhand here.
//
//  Every count test is `jbe` / `jb`, so the counts are unsigned.
//
//  BYTE-IDENTICAL (see the note at the function): only the entry edge
//  reloads 0x1008beac -- the guard's read is the volatile one, the loop is
//  a do-while whose back-edge compare leaves the global in %eax for the
//  head.
// ----------------------------------------------------------------------- //

struct S1001e520Elem                                    // 0x30 bytes
{
	uint8	m_pad_00[0x2c];
	uint16	m_2c;                                       // +0x2c
	uint16	m_2e;                                       // +0x2e
};

struct S1001e520Node
{
	uint8	m_pad_00[0x42];
	uint16	m_42;                                       // +0x42
};

struct S1001e520Body
{
	uint8			m_pad_00[0x88];
	S1001e520Elem  *m_p88;                              // +0x88
	uint32			m_n8c;                              // +0x8c
	uint8			m_pad_90[0xa0 - 0x90];
	S1001e520Node **m_pa0;                              // +0xa0
	uint32			m_na4;                              // +0xa4
};

struct S1001e520Owner
{
	void			 *m_00;                             // +0x00
	S1001e520Body    *m_04;                             // +0x04
};

struct S1001e520Root
{
	uint8			   m_pad_000[0x18c];
	S1001e520Owner   **m_p18c;                          // +0x18c
	uint32			   m_n190;                          // +0x190
};

struct S1001e520Stamped
{
	uint8	m_pad_00[0x0c];
	uint16	m_0c;                                       // +0x0c
};

extern S1001e520Root *g_1008beac;                       // 0x1008beac



// ----------------------------------------------------------------------- //
//  0x10015d60 -- 180 bytes including the alternate entry, 1 caller
//  (0x1002d971).  BUILDS A 32-ENTRY WAVE TABLE AND ITS FORWARD DIFFERENCES.
//
//  WHAT PINS IT: `fstpl (%esp) ; call 0x1007fdd0`, and 0x1007fdd0 is
//  `fldl 0x4(%esp) ; fsin` -- the CRT's sin.  A double argument is why the
//  prologue realigns the stack (`and $0xfffffff0,%esp`) and why icl emits
//  the second, non-realigning entry at 0x10015d6e.
//
//  The constants decode without any ambiguity: 0.03125 = 1/32 and
//  6.2831855 = 2*PI, so the angle is (i/32) * 2PI over 32 samples.  The
//  clamp constants come in pairs -- 0.0 at .data1+0xa70 for the test and a
//  SECOND 0.0 at +0xa74 for the assignment, 1.0 at +0xa80 for the test and
//  a SECOND 1.0 at +0xa84 for the assignment.  icl does not pool duplicate
//  literals here, so those four are four literals in the source.
//
//  Both the load of the global and the clamp are INSIDE the loop: the call
//  to sin clobbers the compiler's knowledge of 0x1008cb7c, and the shipped
//  code recomputes fMin and fAmp all thirty-two times.
//
//  Association: `fSin * fAmp + fAmp + fMin` -- `fmul %st,%st(2)` then two
//  `faddp`s -- is ((fSin*fAmp) + fAmp) + fMin, left to right.  With
//  fMin in [0,1] that runs the table from fMin up to 1.0.
//
//  The second loop's `mov $0x1f,%ecx ; and %edx,%ecx` is `& 31` on the
//  incremented index, not a modulo: a signed `% 32` would need the sign
//  correction and there is none.  `fsubrs` (FSUBR m32) makes the difference
//  table[next] - table[i], not the other way round.
//
//  The store `fstps 0x1008b7d0(,%ebp,4)` after `inc %ebp` is the SAME slot
//  as g_1008b7d4[i] -- icl folded the -4 into the displacement.
//
//  THE ONE COIN-FLIP IN THIS FILE.  The result of sin is used STRAIGHT off
//  the x87 stack -- `fmul %st,%st(2)` -- with no narrowing store anywhere
//  between the call and the final `fstps` into the table.  It is written
//  `float fSin = (float)sin(...)`, which is what the source most likely
//  said, on the evidence from the clippers below that icl 4.0 keeps float
//  locals at extended precision and narrows ONLY when a value feeds an
//  integer conversion (fDenom and fT there never touch memory; the five
//  colour floats do, and only because of the `(uint8)` cast).  IF THIS ONE
//  MISSES with an `fstps <slot> ; flds <slot>` pair sitting immediately
//  after `call 0x1007fdd0`, the answer is that icl DOES narrow on
//  assignment: change `float fSin` to `double fSin` and drop the `(float)`
//  cast.  Nothing else in the function changes.
// ----------------------------------------------------------------------- //

#define MATH_A_TWOPI    6.283185307f

extern float g_1008cb7c;                                // 0x1008cb7c
extern float g_1008b754[32];                            // 0x1008b754
extern float g_1008b7d4[32];                            // 0x1008b7d4

// sin comes from <math.h>, which ltbasetypes.h already pulls in.



// ----------------------------------------------------------------------- //
//  THE ARRAY-AND-COUNT SUB-OBJECT
//
//  d3dr2c.cpp already pins the first eight bytes of this thing: 0x1007b8d0
//  is `mov $0x10091a10,%eax ; ret`, a getter for one global object, and
//  several of its members are an array pointer at +0x00 with an element
//  count at +0x04 which 0x1007b8e0 walks.  0x1005d9c0 below reaches two
//  more members of the SAME shape, +0x14 and +0x18: the guard it puts
//  around the append is `count < m_14` and the argument it hands the
//  overflow message is m_18, formatted with "Set '%s' overflowed".  A
//  capacity and a name -- so the sub-object is a fixed-capacity named set.
//
//  Named CSetA / C10091a10A rather than d3dr2c.cpp's names so the two files
//  cannot collide.
// ----------------------------------------------------------------------- //

struct C1008b880A;
extern C1008b880A g_1008b880;                           // 0x1008b880

class CSetA
{
public:
	void	sub_1007b8e0(void *pParam, void (*pfn)());  // 0x1007b8e0

	void  **m_pArray;                                   // +0x00
	uint32	m_nCount;                                   // +0x04
	uint8	m_pad_08[0x14 - 0x08];
	uint32	m_nMax;                                     // +0x14
	char   *m_pName;                                    // +0x18
};

struct C10091a10A
{
	uint8	m_pad_000[0x264];
	CSetA	m_264;                                      // +0x264
	uint8	m_pad_280[0x284 - 0x264 - 0x1c];
	CSetA	m_284;                                      // +0x284
	uint8	m_pad_2a0[0x2a4 - 0x284 - 0x1c];
	CSetA	m_2a4;                                      // +0x2a4
};

C10091a10A *sub_1007b8d0();


// ----------------------------------------------------------------------- //
//  0x10042dd0 -- 169 bytes + padding, 1 caller (0x1004fd41, the shutdown
//  half of the frame bracket at 0x1004fbc0).
//
//  Exactly the shape d3dr2c.cpp already wrote for 0x1005db10 and
//  0x100646d0, plus a fog bracket.  The two guards are GUARDED EARLY
//  RETURNS, not a wrapping `if`: the first test jumps FORWARD over its own
//  epilogue into the body, and the second jumps to a second epilogue parked
//  after the end -- three epilogues, so three exits in the source.
//
//  0x1008cb0c gates the two SetRenderState(FOGENABLE) calls, off before the
//  walk and back on after it, which is what a pass that must not be fogged
//  does.  The walk itself is `pW->m_2a4.sub_1007b8e0(&g_1008b880,
//  sub_10042da0)` -- `this` is 0x10091a10+0x2a4 (`add $0x2a4,%ebx ;
//  mov %ebx,%ecx`) and the count tested for zero is 0x2a8, i.e. +0x04 of
//  the same sub-object.
// ----------------------------------------------------------------------- //

extern uint32 g_100893fc;                               // 0x100893fc
extern uint32 g_1008cb0c;                               // 0x1008cb0c

void sub_10042da0();



// ----------------------------------------------------------------------- //
//  0x1005d9c0 -- 279 bytes + padding, 1 caller (0x1004fbf3).
//
//  A FILTERED COPY between two of the sets above: it empties m_284, walks
//  m_264, and appends the survivors.
//
//  WHAT PINS THE PREDICATE: the same four-test shape appears in 0x1004fbc0
//  and again in 0x1007b8e0, and in 0x1007b8e0 the flag is read as
//  `0x4d4(%esi)` off an argument that its callers set to 0x1008b880.
//  0x1008b880 + 0x4d4 = 0x1008bd54, which is precisely the absolute address
//  this function tests.  So the two are one member of one global object,
//  read here through its own name and there through a parameter.
//
//  The `test $0x4,%ah` is bit 0x400 of the SAME dword the `test $0x1,%al`
//  read -- %eax is not reloaded between them.
//
//  The count is re-read from memory at the bottom of every iteration
//  (0x1005da70, 0x1005daa0, 0x1005dac1 each reload 0x268(%ebp) before the
//  compare), so the loop condition is the member access, not a cached copy.
//  Every compare is unsigned.
// ----------------------------------------------------------------------- //

struct S1005d9c0Item
{
	uint8	m_pad_00[0x88];
	uint32	m_88;                                       // +0x88
	uint32	m_8c;                                       // +0x8c
	uint8	m_pad_90[0x97 - 0x90];
	uint8	m_97;                                       // +0x97
};

struct C1008b880A
{
	uint8	m_pad_000[0x4d4];
	uint32	m_4d4;                                      // +0x4d4
};

extern uint32 g_100893d8;                               // 0x100893d8

void sub_1005a960(C1008b880A *pCtx, S1005d9c0Item *pItem);
void sub_10021d60(uint32 nLevel, const char* pszMsg, ...);  // 0x10021d60 AddDebugMessage (was d3d_Warning*; icl treats a callee named *Warn*/*Error* as cold)



// ----------------------------------------------------------------------- //
//  THE CLIPPER FAMILY
//
//  The vertex is 40 bytes (`add $0x28,%edx` walks the input, the whole-
//  vertex copy moves ten dwords).  Its layout, entirely from what the six
//  clippers touch:
//
//     +0x00 +0x04 +0x08   x, y, z   -- compared and interpolated as floats;
//                                      z is the homogeneous divisor, which
//                                      is why the frustum planes are
//                                      x < z, y < z, -z < y and so on
//     +0x0c             copied, never interpolated
//     +0x10 .. +0x13    four bytes interpolated one at a time, in the
//                       order +0x12, +0x11, +0x10, +0x13 -- a B,G,R,A
//                       colour written R,G,B,A
//     +0x14 .. +0x16    copied, never interpolated
//     +0x17             a fifth byte, interpolated after the colour
//     +0x18 .. +0x24    four more floats interpolated as a block
//
//  Each of the four byte interpolations goes through its own named float
//  local (`fstps -0x138(%ebp) ; flds -0x138(%ebp) ; fistpl`) -- five
//  distinct slots, so five distinct source variables, and the `fistpl`
//  with no control-word save/restore around it is /QIfist.
//
//  bInside[50] IS PINNED, not guessed.  The frame is `sub $0x144,%esp`
//  with %edi/%esi/%ebx parked at the bottom three dwords, so the locals
//  occupy exactly 0x138 bytes below %ebp.  The five float temporaries sit
//  at -0x138, -0x130, -0x128, -0x120 and -0x118 (icl gives each an
//  eight-byte slot), ending at -0x114; the array starts at -0x114; and the
//  lowest-addressed slot ABOVE the array that the code touches is -0x4c.
//  So the array is at most (0x114-0x4c)/4 = 50 entries, and it must be at
//  least 50 or the frame would not be 0x144.  50 exactly.
// ----------------------------------------------------------------------- //

struct SClipVertA                                       // 0x28 bytes
{
	float	x;                                          // +0x00
	float	y;                                          // +0x04
	float	z;                                          // +0x08
	float	m_0c;                                       // +0x0c
	uint8	m_10;                                       // +0x10
	uint8	m_11;                                       // +0x11
	uint8	m_12;                                       // +0x12
	uint8	m_13;                                       // +0x13
	uint8	m_14;                                       // +0x14
	uint8	m_15;                                       // +0x15
	uint8	m_16;                                       // +0x16
	uint8	m_17;                                       // +0x17
	float	m_18;                                       // +0x18
	float	m_1c;                                       // +0x1c
	float	m_20;                                       // +0x20
	float	m_24;                                       // +0x24
};

struct SPlaneA                                          // 0x10 bytes
{
	float	m_00, m_04, m_08, m_0c;
};

// 0x1008be44 -- bumped once on entry by every one of the six clippers and
// by 0x10047d80.  A clipped-polygon counter.
extern uint32 g_1008be44;


// ----------------------------------------------------------------------- //
//  0x10002d40 -- 852 bytes + 12 of lea-nop padding, 1 caller (0x1000ab87,
//  the bit-0x10 arm of the clip-mask dispatcher).  CLIPS TO x < z.
//
//  The first parameter is DEAD -- nothing in the body reads 0x8(%ebp) --
//  but all six family members have it and 0x10047cf0 (below) passes
//  the address of a plane pointer through it, so it is the plane the
//  general-purpose clipper needs and these hard-wired ones do not.
//
//  THE SHAPE, and every branch of it was read:
//
//    * a pointer-walking first pass fills bInside[] and sums it;
//    * `nInside == 0` returns 0 and the zero-trip exit of that loop was
//      THREADED onto the same return by icl, which is why one `je` at the
//      top of the function lands on the failure epilogue;
//    * `nInside == *pnVerts` returns 1 with nothing touched;
//    * the Sutherland-Hodgman pass re-derives pCur from `(*ppVerts)[i]`
//      every iteration -- it does not hoist the base -- because the body
//      writes through *ppOut and icl will not prove the two do not alias;
//    * bInside[j] is RELOADED after the whole-vertex copy for the same
//      reason, which is what makes the second test a compare against a
//      register rather than two loads;
//    * the epsilon test is `fDenom < -0.00001f || fDenom > 0.00001f`: the
//      first comparison's false arm falls into the second, and the second's
//      true arm jumps BACKWARDS into the first's division block.  A
//      short-circuit ||, not an if/else if.
//
//  x AND z ARE MADE EQUAL BY CONSTRUCTION at the crossing: the same
//  interpolated z is stored to +0x00 and then to +0x08 (`fsts (%ecx) ;
//  fstps 0x8(%ecx)`), so the new vertex sits exactly on the plane.
//
//  THE TAIL DOUBLE-ADVANCES *ppOut.  `*ppOut` already points past the
//  vertices that were just written, and the shipped code then adds
//  *pnVerts * 40 to it again (it re-reads *pnVerts from memory to do it).
//  That is what the bytes say and it is written out as they say it.
// ----------------------------------------------------------------------- //


// ----------------------------------------------------------------------- //
//  0x100030a0 -- 868 bytes + 12 of lea-nop padding, 1 caller (0x1000abb4,
//  the bit-0x20 arm).  CLIPS TO -z < y.
//
//  The same source as 0x10002d40 with two differences, both read off the
//  bytes:
//
//    * the side test is `flds 0x8(%edx) ; fchs ; fcomps 0x4(%edx)` --
//      -z compared against y -- and the plane function is y + z, so the
//      denominator is a four-term sum with the two z terms ADDED where
//      0x10002d40 subtracts them, and the numerator is -(y + z);
//    * the crossing vertex has y = -z rather than x = z, and icl spells
//      that `fld %st(0) ; fchs ; fxch %st(1) ; fstps 0x8(%ecx) ;
//      fstps 0x4(%ecx)` -- FIVE instructions where 0x10002d40 needs two.
//      That duplicate-and-negate is only emitted for two DIFFERENT values;
//      `pOut->y = -(pOut->z = v)` would be `fsts ; fchs ; fstps`.  So the
//      source names the interpolated z and stores it twice, once negated,
//      and that is what fZ is below.
//
//  Sixteen bytes longer than 0x10002d40 and everything else identical,
//  which is why the two are written as one shape.
// ----------------------------------------------------------------------- //


// ----------------------------------------------------------------------- //
//  0x10047cf0 -- 123 bytes + padding, 2 callers (0x10049cf4, 0x1004e504).
//  THE GENERAL-PLANE DRIVER, and it is what identifies the first argument
//  of the whole clipper family.
//
//  It sets up the same three locals the frustum dispatcher at 0x1000aa90
//  sets up, at the same three stack slots (0x10 = the scratch cursor read
//  out of 0x1008b87c, 0x14 = the vertex pointer, 0x18 = the count), and
//  then loops calling 0x10047d80 with `(&pPlane, &pVerts, &nVerts, &pOut)`.
//  0x10047d80 opens with the SAME `incl 0x1008be44` the six frustum
//  clippers open with, so it is the seventh member of the family and the
//  first argument is a plane -- passed by address of a local, which is
//  exactly what the frustum members ignore.
//
//  The plane stride is 0x10, four floats.
//
//  The countdown is a rotated `while`: `test %ebx,%ebx ; je` guards the
//  loop and `test %ebx,%ebx ; jne` closes it, with the decrement hoisted
//  ahead of the call into the argument setup (%ebx is callee-saved, so icl
//  is free to sink it there) and the plane advance after it.
// ----------------------------------------------------------------------- //

extern SClipVertA *g_1008b87c;                          // 0x1008b87c

int32 sub_10047d80(SPlaneA **ppPlane, SClipVertA **ppVerts, int32 *pnVerts,
                   SClipVertA **ppOut);

LTBOOL sub_10047cf0(SClipVertA **ppVerts, int32 *pnVerts, SPlaneA *pPlanes,
                    uint32 nPlanes)
{
	SClipVertA *pOut;
	SClipVertA *pVerts;
	int32		nVerts;
	SPlaneA	   *pPlane;

	pOut   = g_1008b87c;
	pVerts = *ppVerts;
	nVerts = *pnVerts;

	while (nPlanes != 0)
	{
		nPlanes--;
		pPlane = pPlanes;

		if (!sub_10047d80(&pPlane, &pVerts, &nVerts, &pOut))
			return LTFALSE;

		pPlanes++;
	}

	*ppVerts = pVerts;
	*pnVerts = nVerts;

	return LTTRUE;
}


// *********************************************************************** //
//
//  THE THIRTEEN ROWS THAT ARE NOT WRITTEN
//
//  Not one of them is blocked by the compiler; they are the ones not
//  finished to the standard the rest of this file is at, and every one of
//  them has a partial decode recorded here.
//
//    0x10020ce0   160 bytes.  FULLY DECODED AS CONTROL FLOW and not
//                 written only because its arguments cannot be typed.  Ten
//                 stack dwords in, forwarded to two calls: the first is
//                 `0x10020e80(&L28, a3, a4, a1, a2)` filling a 28-byte
//                 local, the second is `0x1001ef30(a0, &L28, a5, a6, a7,
//                 a8, a9, <12-byte struct by value>)` where the struct is
//                 a local set to three 0x3f800000 dwords -- an
//                 LTVector(1,1,1) -- copied into the outgoing area with
//                 `rep movsl ; %ecx = 3`.  Returns 1.  THE COIN-FLIP IF
//                 SOMEONE PICKS IT UP: the three 1.0f stores go out at
//                 0x4c, 0x48, 0x44 -- DESCENDING -- where an LTVector
//                 constructor writes x,y,z ascending; if a written
//                 `LTVector(1,1,1)` misses on those three displacements,
//                 the answer is three separate float locals, not a vector.
//
//    0x1007a280   292 bytes, __thiscall on %ecx.  Builds nine 256-byte
//                 divide tables at `this + 0x70 + 0x100*k` and nine
//                 pointers at `this + 0x970 + 4*k` pointing into a byte
//                 block that starts at `this + 0x994` with strides
//                 1,2,4,8,16,32,64,128 (the ninth points at this+0xa93).
//                 The inner loop is `div` by (1<<k)-1 and by 255.
//                 Decoded but not written: the two nested loops share an
//                 induction variable through 0x8(%esp) and it is not
//                 settled whether the outer one is `for(k=0;k<9;k++)` over
//                 a table of shifts or over the pointer array itself.
//
//    0x1004fbc0   639 bytes.  The per-frame bracket that calls 0x10023eb0
//                 and 0x10023f40 (both written above), 0x1005d9c0 and
//                 0x10042dd0 (both written above).  Skipped because its
//                 middle is a FUNCTION-LOCAL STATIC with an atexit
//                 registration -- `movl $0x1004fe40,(%esp) ; call
//                 0x10080502` guarded by a bit of the once-flag byte at
//                 0x1008ef38 -- around a stdlith CMoArray, and 0x1004fe40
//                 is the destructor thunk the compiler synthesised for it.
//                 That is three interlocking source shapes and it deserves
//                 a batch of its own.
//
//    0x10047d80   975 bytes.  The general-plane clipper, callee of
//                 0x10047cf0 above.  Same family, but its own algorithm --
//                 no bInside[] array, four float locals at -0x160 ..
//                 -0x178, frame 0x188.  Left for whoever takes the family
//                 next; the four-argument signature is settled by
//                 0x10047cf0 above.
//
//    NINE ROWS NOT WRITTEN.  The clipper family and the two render-state
//    pairs were taken in depth rather than guessing at the rest:
//
//        0x10039a60  368   0x10069ed0  728   0x1006bb30  800
//        0x1005a630  816   0x100517bc  740   0x1007d880  504
//        0x1000ac10  423   0x10020d80  248   0x100123a0  260
//
//    Two notes on those that cost nothing to record.  0x100517bc has ZERO
//    callers in the module, so its arity has to come from the body alone.
//    0x1000ac10's own caller is 0x1001575c, and 0x1000ac10 sits directly
//    after the clip-mask dispatcher 0x1000aa90 -- it is very likely a
//    seventh member of the family's translation unit and worth pairing
//    with the two clippers above rather than decoding cold.
//
// *********************************************************************** //


// 0x1001e520 is /Os, not /O2: `xor %eax,%eax ; inc %eax` for the 1 and no
// loop-alignment nops, so it was moved here from d3dr3a.cpp.
//
// BYTE-IDENTICAL as a guarded do-while whose GUARD alone reads the global
// through a volatile.  The shipped loop head `mov 0x18c(%eax)` is entered
// from the back edge with g_1008beac still in %eax from the back-edge
// compare, but the entry edge reloads it (0x1001e559): the guard's load of
// the global was not reusable, the back edge's was.  Measured:
//   * plain for-loop: the guard keeps g in %eax and tests the count in
//     %edx, no entry reload (216 bytes);
//   * volatile read in the body (the previous version): 219 bytes, but
//     both edges reload, so the inner `xor %edx,%edx` schedules above the
//     0x18c load and the back edge lands 5 bytes early (9 differing);
//   * volatile in the for-condition: the same 9;
//   * `if (i < volatile g->n) do {...} while (++i < g->n);`: identical.
void sub_1001e520(S1001e520Stamped *pThing)
{
	S1001e520Body  *pBody;
	uint32			i, j;

	if (pThing->m_0c == 0xFFFF)
	{
		pThing->m_0c = 1;

		i = 0;
		if (i < (*(S1001e520Root* volatile*)&g_1008beac)->m_n190)
		{
			do
			{
				pBody = g_1008beac->m_p18c[i]->m_04;

				for (j = 0; j < pBody->m_n8c; j++)
					pBody->m_p88[j].m_2c = 0;

				for (j = 0; j < pBody->m_na4; j++)
					pBody->m_pa0[j]->m_42 = 0;

				for (j = 0; j < pBody->m_n8c; j++)
					pBody->m_p88[j].m_2c = 0;
			}
			while (++i < g_1008beac->m_n190);
		}
	}

	pThing->m_0c++;
}
