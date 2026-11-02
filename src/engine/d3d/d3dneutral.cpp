// *********************************************************************** //
//
//  MODULE:   NEUTRAL (.CPP)   -- candidate additions to src/engine/d3d/d3dmisc.cpp
//
//  PURPOSE:  The functions in d3d.ren whose machine code is COMPILER-NEUTRAL:
//            no fingerprint of the unknown compiler AND no scheduling freedom,
//            so `cl /O2` has only one legal encoding to choose.
//
//  Every body below was read instruction-by-instruction off the retail d3d.ren.
//  Nothing here is inferred from context; where a name could not be earned
//  the function is named after its address and the global after its address.
//
// ----------------------------------------------------------------------- //
//  WHAT THE SCAN WAS THIS TIME
// ----------------------------------------------------------------------- //
//
//  The earlier scan capped at 64 bytes and rejected on four byte strings.
//  This one instead disassembles from every candidate entry point -- the
//  1196 CSV rows PLUS every direct-call target PLUS every dword anywhere in
//  the image that points into .text (1397 starts) -- walks each to its real
//  `ret`, and rejects on SHAPE:
//
//    (a) push %ebp; mov %esp,%ebp                    (fingerprint 1)
//    (b) xor %eax,%eax; inc %eax                     (fingerprint 2)
//    (c) register pushes not balanced by pops, i.e. `push %esi` used as a
//        4-byte allocation and closed with `add $N,%esp`
//                                                    (fingerprint 3)
//    (d) `add $N,%esp` with no matching `sub $N,%esp` and no call in front
//    (e) an argument loaded from N(%esp) BEFORE the callee-saved pushes
//    (f) a lea-nop or 0x90 INSIDE the body (loop-head alignment)
//    (g) a dead spill `mov %edi,N(%esp)` of a register the body never uses
//    (h) outgoing call arguments written with `mov ...,(%esp)` instead of
//        `push`
//
//  (d)..(h) are new -- see the fingerprint notes at the bottom of this file,
//  which are the more valuable half of this pass.
//
//  Survivors in the renderer's own code (below 0x1007fa10; above that is
//  MSVC's own CRT, linked in) were then hand-read and kept only when the
//  instruction order is FORCED: a single load of a global, a run of stores
//  to distinct globals, a store followed by the return value.
//
// *********************************************************************** //

#include "ltbasedefs.h"


// ======================================================================= //
//  TIER 1 -- expected BYTE-IDENTICAL
//
//  For each: the three fingerprints are absent (checked on the bytes,
//  not by eye), and there is no pair of independent operations whose order
//  a scheduler could swap.
// ======================================================================= //


// ----------------------------------------------------------------------- //
//  Empty bodies -- one byte, `c3`.
//
//  There is exactly ONE possible encoding, so these are neutral by
//  construction. The five below are every `c3`-only entry point in the
//  renderer's code; the scan above enumerated them exhaustively.
//
//  NOTE for whoever splices these: 0x1000eeb0 is referenced TWICE in the
//  module-init table (.data 0x10089020 and 0x10089190) and 0x10069c90 twice
//  as well (0x100890a0, 0x100891b0). Identical-COMDAT folding means one
//  shipped byte may stand for several source functions.
// ----------------------------------------------------------------------- //

// 0x1000eeb0 -- 1 byte. Not called; its address sits in the module-init
// table at .data 0x10089020 and again at 0x10089190. An empty per-module
// init.
void sub_1000eeb0()
{
}


// 0x10069c90 -- 1 byte. Same table, slots 0x100890a0 and 0x100891b0; also
// referenced at .text 0x10019173.
void sub_10069c90()
{
}


// 0x1006c670 -- 1 byte. Never called; its address is loaded into a register
// at 0x1006ce1a, 0x1006ce3e, 0x1007b5b9 and 0x1007b5c0, i.e. it is installed
// as a callback. A do-nothing default handler.
void sub_1006c670()
{
}


// 0x100145f0 -- 1 byte. Called five times (0x100264e1, 0x1002653e,
// 0x1002d938, 0x10032cc6, 0x10032e00). The call at 0x10032cc6 pushes
// nothing, so it takes no arguments.
void sub_100145f0()
{
}


// 0x1007b8c0 -- 1 byte. Called from 0x10032cd0 and 0x1003312b; the first is
// in a run of five no-argument calls, so no arguments.
void sub_1007b8c0()
{
}


// ----------------------------------------------------------------------- //
//  Constant returns
// ----------------------------------------------------------------------- //

// 0x1005dbe0 -- 3 bytes: `33 c0 c3`. The exact twin of d3d_Slot_c0
// (0x10032e40), already byte-identical in d3dmisc.cpp, so this shape is
// measured, not assumed.
//
// Its one call site (0x10032df6) writes one dword to the outgoing-argument
// slot first, and then returns the answer unchanged -- so it takes one
// argument, ignores it, and always answers zero. cl emits `xor eax,eax; ret`
// whatever the parameter list is, the parameter being dead.
uint32 sub_1005dbe0(void* pUnused)
{
	return 0;
}


// ----------------------------------------------------------------------- //
//  Getters -- one load, one ret. Order forced: there is nothing to order.
// ----------------------------------------------------------------------- //

extern LTBOOL g_bIn3D;                                  // 0x1008d7c8, defined in d3ddraw.cpp
uint32  g_1008add0;                                     // 0x1008add0
uint32  g_100911b8;                                     // 0x100911b8


// 0x10032f20 -- RenderStruct slot +0x90, IsIn3D (named in d3d.cpp's table),
// 6 bytes: `a1 c8 d7 08 10 c3`.
//
// The global is pinned as the in-3D flag by its writers: Start3D (slot +0x88
// = 0x10032e50) touches it at 0x10032e57 and End3D (+0x8c = 0x10032ea0) at
// 0x10032ea7. It is the neighbour of g_bInOptimized2D (0x1008d7c4), which
// d3dmisc.cpp already has.
// d3d_IsIn3D (0x10032f20) lives in d3ddraw.cpp, which owns the 3D bracket.


// 0x1000f640 -- 6 bytes, `a1 d0 ad 08 10 c3`. Reads 0x1008add0. Both callers
// (0x100126f8, 0x1001429b) immediately `test %eax,%eax`, so it is read as a
// flag; the storage is a dword and 0x1008add0's initialiser in .data is 0.
// Paired with sub_1000f630 below, which is the only writer.
uint32 sub_1000f640()
{
	return g_1008add0;
}


// 0x1006a780 -- 6 bytes, `a1 b8 11 09 10 c3`. Reads 0x100911b8. Called from
// 0x10032cda, in the same run of no-argument calls as sub_1007b8c0.
uint32 sub_1006a780()
{
	return g_100911b8;
}


// 0x10015470 -- 6 bytes, `b8 80 1d 09 10 c3`: `return &g_10091d80;`. Not a
// load -- the ADDRESS is the answer. The caller at 0x1001944f puts the
// result straight into %ecx and dereferences it. Declared `extern` with no
// bound because the object's size is not determined by this function.
extern uint8 g_10091d80[];

void* sub_10015470()
{
	return g_10091d80;
}


// 0x1007b8d0 -- 6 bytes, `b8 10 1a 09 10 c3`: `return &g_10091a10;`. Eight
// callers. 0x10091a10 is the dword immediately past the 16-entry table that
// sub_1007a1d0 (below) fills, so the two are neighbours in one .bss block,
// but nothing in this function proves they are one object -- kept separate.
extern uint8 g_10091a10[];

void* sub_1007b8d0()
{
	return g_10091a10;
}


// ----------------------------------------------------------------------- //
//  Setters -- load the argument, store it, return. Nothing to reorder.
// ----------------------------------------------------------------------- //

// 0x1000f630 -- 10 bytes:
//      8b 44 24 04     mov  0x4(%esp),%eax
//      a3 d0 ad 08 10  mov  %eax,0x1008add0
//      c3              ret
// The writer half of the 0x1008add0 pair; called from 0x10023f2b and
// 0x10023f9b. Plain `ret`, so __cdecl.
void sub_1000f630(uint32 nValue)
{
	g_1008add0 = nValue;
}


uint32 g_1008bf68;                                      // 0x1008bf68

// 0x10032600 -- 14 bytes, and it is `ret $0xc`: callee-pops, three dwords of
// arguments, i.e. __stdcall(3). Only the first is touched.
//
//      8b 44 24 04     mov  0x4(%esp),%eax
//      a3 68 bf 08 10  mov  %eax,0x1008bf68
//      33 c0           xor  %eax,%eax
//      c2 0c 00        ret  $0xc
//
// The zeroing of %eax comes AFTER the store, which is the only order cl will
// emit for `g = a; return 0;`. Never called directly -- its address is
// stored as a callback at 0x100337bc -- so the argument types are not
// recoverable and are left as dwords. 0x1008bf68 is the most-referenced
// global in the module (33 references), a device/context pointer.
uint32 __stdcall sub_10032600(uint32 nArg0, uint32 nArg1, uint32 nArg2)
{
	g_1008bf68 = nArg0;
	return 0;
}


// ----------------------------------------------------------------------- //
//  Single immediate store
// ----------------------------------------------------------------------- //

uint32 g_1008e730;                                      // 0x1008e730

// 0x1003b3a0 -- 11 bytes:
//      c7 05 30 e7 08 10 00 00 00 00   movl $0x0,0x1008e730
//      c3                              ret
// ONE constant, ONE use, so cl takes the immediate-store form rather than
// materialising zero in a register -- the register form only appears once a
// constant is reused, which is exactly what d3d_ResetCounters (four uses,
// already byte-identical) demonstrates. Called from 0x1003738c.
void sub_1003b3a0()
{
	g_1008e730 = 0;
}


// ----------------------------------------------------------------------- //
//  Runs of stores to distinct globals -- the shape d3d_ResetCounters already
//  proved byte-identical. Store order is source order; cl does not reorder
//  stores to distinct objects, and there are no loads to interleave.
// ----------------------------------------------------------------------- //

uint32 g_1008fea0;                                      // 0x1008fea0
uint32 g_1008fe64;                                      // 0x1008fe64
uint32 g_1008fe9c;                                      // 0x1008fe9c
uint32 g_1008fe60;                                      // 0x1008fe60

// 0x1005dbc0 -- 23 bytes, instruction-for-instruction the same function as
// d3d_ResetCounters (0x1001eab0, 23/23 byte-identical): one `xor %eax,%eax`
// then four `a3` absolute stores. Different globals, same schedule.
//
//      33 c0   xor %eax,%eax
//      a3 a0 fe 08 10          -> 0x1008fea0
//      a3 64 fe 08 10          -> 0x1008fe64
//      a3 9c fe 08 10          -> 0x1008fe9c
//      a3 60 fe 08 10          -> 0x1008fe60
//      c3
//
// The four are NOT consecutive and NOT monotonic (a0, 64, 9c, 60), so this
// is four separate assignments in that source order, not a memset. The same
// four are written together at 0x1005db54..0x1005db7f inside 0x1005db50,
// which is this function's initialising counterpart.
//
// Not called; its address is in the module-init table at .data 0x1008963c.
void sub_1005dbc0()
{
	g_1008fea0 = 0;
	g_1008fe64 = 0;
	g_1008fe9c = 0;
	g_1008fe60 = 0;
}


// 0x1007a1d0 -- 161 bytes, SIXTEEN `c7 05 <abs> <imm32>` stores and a `ret`.
// The single largest forced-order body in the module.
//
//   0x100919d0 <- 0x00000000      0x100919f0 <- 0x88000000
//   0x100919d4 <- 0x11000000      0x100919f4 <- 0x99000000
//   0x100919d8 <- 0x22000000      0x100919f8 <- 0xaa000000
//   0x100919dc <- 0x33000000      0x100919fc <- 0xbb000000
//   0x100919e0 <- 0x44000000      0x10091a00 <- 0xcc000000
//   0x100919e4 <- 0x55000000      0x10091a04 <- 0xdd000000
//   0x100919e8 <- 0x66000000      0x10091a08 <- 0xee000000
//   0x100919ec <- 0x77000000      0x10091a0c <- 0xff000000
//
// A 4-bit-to-8-bit alpha replication table, one dword per nibble, value
// (i * 0x11) << 24. Consumed at 0x100777d4 and 0x10078a64.
//
// Why it is neutral: sixteen DIFFERENT immediates, so nothing is a reused
// constant and cl has no reason to put any of them in a register; sixteen
// stores to distinct addresses, which cl will not reorder; no loads at all,
// so there is nothing to schedule against. The array must live in .bss (if
// it were an initialised aggregate there would be no code at all), so the
// source is a run of assignments, written out longhand -- neither compiler
// unrolls, and the shipped body is straight-line, so the source was too.
//
// Not called; module-init table entry at .data 0x100891f0.
uint32 g_AlphaExpand4to8[16];                           // 0x100919d0

void sub_1007a1d0()
{
	g_AlphaExpand4to8[0]  = 0x00000000;
	g_AlphaExpand4to8[1]  = 0x11000000;
	g_AlphaExpand4to8[2]  = 0x22000000;
	g_AlphaExpand4to8[3]  = 0x33000000;
	g_AlphaExpand4to8[4]  = 0x44000000;
	g_AlphaExpand4to8[5]  = 0x55000000;
	g_AlphaExpand4to8[6]  = 0x66000000;
	g_AlphaExpand4to8[7]  = 0x77000000;
	g_AlphaExpand4to8[8]  = 0x88000000;
	g_AlphaExpand4to8[9]  = 0x99000000;
	g_AlphaExpand4to8[10] = 0xaa000000;
	g_AlphaExpand4to8[11] = 0xbb000000;
	g_AlphaExpand4to8[12] = 0xcc000000;
	g_AlphaExpand4to8[13] = 0xdd000000;
	g_AlphaExpand4to8[14] = 0xee000000;
	g_AlphaExpand4to8[15] = 0xff000000;
}


// ======================================================================= //
//  TIER 2 -- decoded with confidence, encoding NOT certain.
//
//  Both of these have exactly one degree of freedom left, named per
//  function. Expect SIZE-EXACT with a handful of differing bytes, the way
//  d3d_GetSurfaceInfo came out. Build them, but do not be surprised, and
//  un-pick them without ceremony if they miss.
// ======================================================================= //

uint32 g_1008d7e4;                                      // 0x1008d7e4

// 0x10032990 -- 14 bytes:
//      33 c0                   xor  %eax,%eax
//      8b 15 e4 d7 08 10       mov  0x1008d7e4,%edx
//      85 d2                   test %edx,%edx
//      0f 97 c0                seta %al
//      c3                      ret
//
// `seta` (unsigned above) against zero, not `setne`, so the source really is
// a `> 0` on an UNSIGNED value and neither compiler folded it to `!= 0`.
// Two callers, 0x10026420 and 0x1002d2b9.
//
// THE DEGREE OF FREEDOM: the shipped build loads the global into %edx and
// then `test`s the register. cl is at least as likely to emit
// `cmp DWORD PTR ?g, 0` and skip the register entirely, which is 10 bytes
// instead of 11 and shifts everything after it.
LTBOOL sub_10032990()
{
	return g_1008d7e4 > 0;
}


// 0x1000a6e0 -- 25 bytes. Unlink a node from a doubly-linked list and tie it
// off against itself. Three callers: 0x1000a7fa, 0x10050d71, 0x10050d7c.
//
//      8b 44 24 04     mov  0x4(%esp),%eax     ; p
//      8b 10           mov  (%eax),%edx        ; p->m_pNext
//      8b 48 04        mov  0x4(%eax),%ecx     ; p->m_pPrev
//      89 4a 04        mov  %ecx,0x4(%edx)     ; p->m_pNext->m_pPrev = p->m_pPrev
//      8b 50 04        mov  0x4(%eax),%edx     ; RELOAD p->m_pPrev
//      8b 08           mov  (%eax),%ecx        ; RELOAD p->m_pNext
//      89 0a           mov  %ecx,(%edx)        ; p->m_pPrev->m_pNext = p->m_pNext
//      89 40 04        mov  %eax,0x4(%eax)     ; p->m_pPrev = p
//      89 00           mov  %eax,(%eax)        ; p->m_pNext = p
//      c3
//
// The two reloads are the point: the first store may alias the node, so both
// fields are re-read. Any correct compiler must reload them, which is what
// makes the middle of this function forced rather than scheduled. Within
// each statement the DESTINATION base is computed before the source value,
// consistently, in both statements -- so the two loads of a pair are not
// independent-and-swappable, they are ordered by the expression.
//
// THE DEGREE OF FREEDOM: register naming only. The shipped build takes
// %eax=p, %edx=destination base, %ecx=value; cl may well pick %ecx then
// %edx. Same eleven instructions, same 25 bytes, up to three differing
// modrm bytes -- the d3d_GetSurfaceInfo outcome.
struct DLink
{
	DLink* m_pNext;                                     // +0x00
	DLink* m_pPrev;                                     // +0x04
};

void sub_1000a6e0(DLink* pLink)
{
	pLink->m_pNext->m_pPrev = pLink->m_pPrev;
	pLink->m_pPrev->m_pNext = pLink->m_pNext;
	pLink->m_pPrev = pLink;
	pLink->m_pNext = pLink;
}


// *********************************************************************** //
//
//  WHAT WAS REJECTED, AND WHY -- five NEW fingerprints of the unknown
//  compiler, each of which rules out a whole class at a glance.
//
// ----------------------------------------------------------------------- //
//  0. THE COMPILER IS NOT MICROSOFT'S, AND THE RICH HEADER SAYS SO OUT LOUD
//
//  d3d.ren's Rich header lists 109 objects from cl(C) 12.00.8168, 8 from
//  cl(C++) 8168, 3 from cl(C++) 8447 -- and EIGHTY objects with prodid 0,
//  build 0, "unmarked". lithtech.exe, built by the toolchain this project
//  owns, has FOUR such. A non-Microsoft compiler emits COFF with no Rich
//  record at all, so it lands in exactly that bucket. The ~120 Microsoft
//  objects are the CRT pulled out of LIBC.LIB by link.exe 6.00.8447; the 80
//  unmarked ones are the renderer.
//
//  That also draws the boundary. `mov <arg>,(%esp)` call setup and lea-nop
//  padding stop dead at ~0x1007fa10 and `push %ebp; mov %esp,%ebp` becomes
//  dominant above it: EVERYTHING FROM 0x1007fa10 UP IS MSVC's OWN CRT and
//  should be excluded from d3d.ren function counts entirely (roughly 190 of
//  the 1196 enumerated rows).
//
// ----------------------------------------------------------------------- //
//  1. OUTGOING CALL ARGUMENTS ARE STORED, NOT PUSHED
//
//  0x1001e920, in full:
//      83 ec 0c                 sub  $0xc,%esp
//      c7 04 24 f4 be 08 10     movl $0x1008bef4,(%esp)
//      e8 ..                    call 0x1007fbe0
//      c7 04 24 d8 be 08 10     movl $0x1008bed8,(%esp)
//      e8 ..                    call 0x1007fbe0
//      ...
//      83 c4 0c                 add  $0xc,%esp
//
//  One `sub` at entry opens a fixed outgoing-argument area; each call writes
//  its arguments into it with `mov` and there is no `push` anywhere. cl NEVER
//  does this -- it pushes and cleans up per call. Counted across .text:
//  895 `mov %eax,(%esp)` and 441 `movl $imm,(%esp)`, all of them below
//  0x1007fa10.
//
//  This single fingerprint disqualifies every function in the module that
//  calls anything. It is why the tier-1 list above contains no calls at all.
//
// ----------------------------------------------------------------------- //
//  2. GAS ALIGNMENT PADDING, INCLUDING INSIDE FUNCTION BODIES
//
//  Lea-nops sit between functions. They also appear INSIDE
//  bodies, in front of loop heads -- 0x1006c4d0 carries
//  `8d b4 26 00 00 00 00` at 0x1006c4d9 and 0x100256a0 carries `90` plus the
//  same seven bytes at 0x100256a8, both immediately before the backward
//  branch target. Full counts in .text: 214 x `8d 76 00`, 368 x
//  `8d bc 27 00 00 00 00`, 213 x `8d b4 26 00 00 00 00`.
//
//  Those three byte strings are the i386 multi-byte-nop table of the GNU
//  assembler, and `lea 0x0(%esi,%eiz,1),%esi` -- an explicit %eiz null index
//  -- is a GAS encoding. cl pads with 0x90 or 0xcc and never aligns a loop
//  head this way. So: a function containing a lea-nop is unreachable for us
//  even if everything else about it looks neutral.
//
// ----------------------------------------------------------------------- //
//  3. ADDRESS CONSTANTS ARE HOISTED INTO A REGISTER; cl STORES THEM INLINE
//
//  0x10021580 and 0x10015d30 are list-head tie-offs, and both open by
//  materialising the address in a register and then storing the register:
//
//      10015d30: b8 84 b0 08 10   mov $0x1008b084,%eax
//      10015d35: ba 78 b0 08 10   mov $0x1008b078,%edx
//      10015d3a: a3 84 b0 08 10   mov %eax,0x1008b084
//      ...
//
//  Measured against a binary we KNOW is cl output: lithtech.exe contains
//  ZERO occurrences of `mov $X,%eax` followed by `mov %eax,X` for any X.
//  d3d.ren contains two, plus four `movl $X,X`. cl writes
//  `mov DWORD PTR ?g, OFFSET ?g` as one immediate store; it does not CSE an
//  address constant into a register even when the address is used twice.
//
//  So REJECTED, with the sizes that would have come out wrong:
//    0x10021580  26 bytes  (cl would spend 20 on the pair, not 15)
//    0x10015d30  33 bytes
//    0x1003b1c0 116 bytes  -- same hoist, for 0 and 0xffff, plus byte and
//                             word stores; the largest global-constructor
//                             body in the module and the biggest loss
//
// ----------------------------------------------------------------------- //
//  4. DEAD SPILLS OF CALLEE-SAVED REGISTERS INTO A FRAME NOTHING ELSE USES
//
//  This generalises the third fingerprint. 0x1001e4a0 in full:
//
//      53              push %ebx
//      83 ec 10        sub  $0x10,%esp
//      8b 44 24 18     mov  0x18(%esp),%eax
//      8b 54 24 20     mov  0x20(%esp),%edx
//      8b 5c 24 1c     mov  0x1c(%esp),%ebx
//      89 7c 24 08     mov  %edi,0x8(%esp)      <-- %edi is never read or
//      89 51 2c        mov  %edx,0x2c(%ecx)         written by this function
//      ...
//
//  The 0x10-byte frame exists only to hold that one dead store. The same
//  dead `mov %edi,N(%esp)` appears in 0x10003bb0, 0x10003bf0, 0x100591e0,
//  0x1000eec0, 0x10015480 and 0x10035c00, all of which are otherwise
//  perfectly forced-order struct copies and all of which are therefore lost.
//  0x10003ad0 is the reductio: `sub $0x8,%esp; add $0x8,%esp; ret $0x4` --
//  an empty function with a frame. cl writes `ret 4` and stops.
//
// ----------------------------------------------------------------------- //
//  5. n-ARY ARITHMETIC IS EVALUATED RIGHT-TO-LEFT, INCLUDING FLOAT
//
//  Every matrix-vector routine in the module accumulates its dot product
//  from the LAST term backwards. 0x10014940, first output component:
//
//      flds 0x8(%eax) ; fmuls 0x8(%edx)    -> m02*z
//      flds 0x4(%eax) ; fmuls 0x4(%edx)    -> m01*y
//      faddp                               -> (m02*z + m01*y)
//      flds (%eax)    ; fmuls (%edx)       -> m00*x
//      faddp                               -> ((m02*z + m01*y) + m00*x)
//      fadds 0xc(%eax)                     -> + m03
//
//  cl evaluates `m00*x + m01*y + m02*z + m03` strictly left to right and is
//  not permitted to re-associate the float adds, so it produces the terms in
//  the opposite order. `fsubrp` in 0x1002564c is the same effect on a
//  subtraction: the operands arrive reversed, so the reverse-subtract form
//  is used.
//
//  This rules out ALL the floating-point maths in the module in one stroke,
//  and it is a large set -- 0x10014a50, 0x10014940, 0x1000a770, 0x10023230,
//  0x1000d940, 0x10014580, 0x1004b5c0, 0x1005f9a0, 0x100159d0, 0x100149a0,
//  0x10073400, 0x10003ae0 and more, several hundred bytes each, every one of
//  them otherwise straight-line and call-free.
//
// ----------------------------------------------------------------------- //
//  6. SOME FUNCTIONS TAKE THEIR ARGUMENTS IN REGISTERS, NOT ON THE STACK
//
//  0x1001e8d4 reads %eax on its first instruction with no preceding load;
//  0x1002564c reads %eax, %edx and %ecx and RETURNS THREE VALUES on the x87
//  stack; 0x100265a8 and 0x10003a80 do the same with %eax / %ecx+%edx. There
//  is no MSVC calling convention that does this (__fastcall is %ecx,%edx and
//  never %eax first), so these are either regparm-style or compiler-outlined
//  fragments, and none of them is expressible in C for cl.
//
// ----------------------------------------------------------------------- //
//  ONE MORE REJECTION WORTH RECORDING, BECAUSE OF ITS SIZE
//
//  0x10030f30 is 1968 bytes and is the DirectDraw/Direct3D HRESULT-to-string
//  table: `mov 0x4(%esp),%eax`, then ~120 `cmp $0x8876xxxx,%eax; je <far>`
//  in a linear chain, then ~120 separate `mov $<string>,%eax; ret` tails laid
//  out at the end in reverse order of the tests. The comparison ORDER is
//  forced by the source, and every constant is a documented DDERR/D3DERR
//  value, so this decodes cleanly and would be the single largest verifiable
//  win in the module -- but the block LAYOUT (all tails moved to the end,
//  reached by 6-byte near jumps, instead of falling through inline) is a
//  scheduling decision, and cl lays if-chains out the other way. Worth a
//  speculative build cycle on its own some day; not worth spending one of
//  today's.
//
//  Likewise 0x10021410, RenderDLLSetup itself: 365 bytes of `mov 0x4(%esp),
//  %eax`, one store of the pointer to 0x1008cbcc, and then 37 `movl $<fn>,
//  N(%eax)` in slot order 0x6c..0xfc. Utterly forced -- but it cannot be
//  built until the 37 functions it names exist, so it is a milestone, not a
//  candidate.
//
// *********************************************************************** //
