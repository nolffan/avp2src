// *********************************************************************** //
//
//  MODULE:   BIG_24020 (.CPP)
//
//  PURPOSE:  One function -- 0x10024090, the OVERDRAW METER.
//
//  =================== THE EXTENT IS NOT WHAT THE ROW SAYS ================ //
//
//  progress row: "0x10024024, 5,672 bytes, ends 0x1002564c".  BOTH ends
//  are wrong, and in two different ways.
//
//  THE START.  0x10024020..0x10024084 is a COMPLETE, SEPARATE FUNCTION --
//  a /Qip two-entry pair, but the pair belongs to a
//  101-byte list-push, not to the 5.5 KB body.  0x10024020 is
//  `mov 0x4(%esp),%eax` falling into the register entry at 0x10024024
//  (`sub $0xc,%esp`); the body pops the element free list at 0x1008d4fc,
//  links the object onto 0x1008d4bc through +0x14, publishes 0x1008b874
//  into the node's +0x10, and returns at 0x10024058 with its
//  refill arm (`call 0x1007fbf0` on the pool at 0x1008d4e4) at
//  0x10024059..0x10024084.  0x10024085..0x1002408f is eleven bytes of
//  lea-nop padding.  BOTH entries are live and both are reached:
//    * `movl $0x10024020,0x1008ce98` at 0x100256e2, and the
//      `mov $0x10024020,%edx` at 0x10025748 feeding the four stores at
//      0x1002574d..0x1002575f, take the STACK entry's address;
//    * a scan of the whole 0x86b90-byte .text for `e8` displacements
//      finds THREE calls to 0x10024020 -- 0x1001428e (big124a4.cpp's TU)
//      and 0x1006f16c / 0x1006f527 (big6d910.cpp's) -- all from other
//      translation units, and exactly ONE call to 0x10024024;
//    * that one call to 0x10024024 is at 0x10025523, INSIDE the function
//      below, with the object in %eax and `add $0xfffffff8,%esp` before
//      it: the REGISTER entry, i.e. the same-TU form.  That is "a call to f+4
//      where you call f", and it is why the row's start address is four
//      bytes inside a function that ENDS 1,484 bytes before the row's own
//      end.  src/engine/d3d/big124a4.cpp and big6d910.cpp have already
//      decoded and declared 0x10024020, so it is NOT re-derived here.
//
//  THE REAL FUNCTION IS 0x10024090 .. 0x1002563f INCLUSIVE -- 5,552 bytes
//  (0x15B0).  Verified before a byte was decoded:
//    * ONE prologue in the range, at 0x10024090 (`push %ebp ;
//      mov %esp,%ebp ; sub $0xa90,%esp`), and it is 16-byte aligned.
//    * ONE `ret`, at 0x10025539, with ONE epilogue in front of it.
//    * EVERY branch target in the range is inside the range.  A script
//      over all 0x100xxxxx operands in the disassembly returned exactly
//      four outsiders and all four are CALL targets: 0x1000d9b0,
//      0x10014580, 0x10024024 and 0x1002564c.
//    * the 262 bytes after the `ret` -- 0x1002553a..0x1002563f -- are /O2
//      cold-block sinking, six blocks of
//      `flds <+1e-5> ; fcomp ; fnstsw ; sahf ; jb <back> ; fstp.. ;
//       flds <0.0f> ; jmp <back>`, jumping back to 0x1002436c/0x10024376,
//      0x10024673/0x1002467b, 0x10024985/0x1002498d,
//      0x10024c83/0x10024c8b, 0x10024f85/0x10024f8d and
//      0x1002526c/0x10025276.  Every one is the second half of an epsilon
//      test inside THIS frame.
//    * the last of them ends `jmp 0x10025276` at 0x1002563b, i.e. at
//      0x1002563f, and 0x10025640 is `mov 0x4(%esp),%eax ;
//      mov 0x8(%esp),%edx ; mov 0xc(%esp),%ecx` falling into 0x1002564c --
//      the NEXT function, another /Qip pair, with no padding between.
//      That is why the row's end landed on 0x1002564c: it is the register
//      entry of the next function, and this function CALLS it
//      (0x100254cf, `add $0xfffffff0,%esp` and three register arguments).
//
//  The same scan finds NO `e8` reaching 0x10024090 and none reaching
//  0x10025640 or 0x1002564c from outside this function: 0x10024090 is
//  entered only through the four function-pointer slots (its address
//  appears exactly once as an imm32, at 0x1002571f), and 0x10025640's
//  stack entry is dead in this image -- its register entry 0x1002564c has
//  exactly one caller, 0x100254cf, which is the loop below.
//
//  ---------------------------------------------------------------------- //
//  THE CALL SEQUENCE IS NEARLY BLIND -- READ THE CENSUS INSTEAD
//  ---------------------------------------------------------------------- //
//
//  Counted, the range holds FOUR
//  calls, all direct, to four distinct callees, at 1,388 bytes per call:
//
//      0x100241fc  0x1000d9b0      camera transform, one per vertex
//      0x1002547a  0x10014580      screen projection, one per vertex
//      0x100254cf  0x1002564c      2 x signed area of one fan triangle
//                                  (register entry of 0x10025640)
//      0x10025523  0x10024024      push the object on the deferred list
//                                  (register entry of 0x10024020)
//
//  1,388 B/call is far below the useful band, so the decode
//  rests on three other instruments instead.  All three come out exact.
//
//  (1) FRAME ARITHMETIC THAT DECOMPOSES WITHOUT REMAINDER.
//      `sub $0xa90,%esp` = 2,704 bytes, and ascending from %esp:
//
//        -0xa90     12   %ebx/%esi/%edi spill ((%esp), 0x4(%esp), 0x8(%esp))
//        -0xa84     20   pad up to the 8-byte alignment of the temp block
//        -0xa70    240   THIRTY float temps at an 8-BYTE STRIDE, -0xa70,
//                        -0xa68, ... -0x988.  Every one is written exactly
//                        once by an `fstps` and read by the `flds` on the
//                        very next instruction: 6 planes x 5 byte-lerps.
//                        nVerts is tucked into the last slot's unused
//                        upper half at -0x984 (the temps are floats at an
//                        8-byte pitch, so -0x984..-0x981 was free).
//        -0x980    960   aVerts -- THIRTY vertices of 0x20.  The bound is
//                        READ OFF THE FRAME: -0x980 .. -0x5c1 is 0x3c0.
//        -0x5c0      4   pVerts
//        -0x5bc  1,320   SIX clip blocks of 0xdc, at -0x5bc, -0x4e0,
//                        -0x404, -0x328, -0x24c and -0x170.  Each is
//                        int32 abInside[50] (0xc8) followed by the five
//                        int32 `fistpl` landing slots (0x14): block one's
//                        flags are indexed at -0x5bc(%ebp,%edx,4) and its
//                        five conversions land at -0x4f4, -0x4f0, -0x4ec,
//                        -0x4e8 and -0x4e4, the twenty bytes immediately
//                        before block two.  Block SIX's five land at
//                        -0xa8..-0x98, immediately after its own flags at
//                        -0x170 (+0xc8 = -0xa8), which is what proves the
//                        0xdc stride is 200+20 and not 204+16.
//        -0x94       4   pad
//        -0x90     144   the shared `fildl` slot at -0x90 and the named /
//                        spilled scalars up to %ebp.
//        12+20+240+960+4+1320+4+144 = 2,704.  Exact, no remainder.
//
//  (2) AN INSTRUCTION CENSUS WITH NO RESIDUE.  Every mnemonic in the whole
//      5,552 bytes, counted mechanically, and every total predicted by
//      "six planes, five byte-lerps each, one texture pair, one inlined
//      4x4 projective transform, one fan-area loop".  Nothing is left over.
//
//        movzbl  60 = 6 x 5 x 2   (each lerp loads pWalk's and pPrev's byte)
//        fildl   60 = 6 x 5 x 2   (the difference and the base)
//        fistpl  30 = 6 x 5
//        fstps   61 = 56 + 4 (inlined transform) + 1 (g_1008be30)
//                     per plane: near 9, left 10, top 9, right 9,
//                     bottom 10, far 9 -- the two planes that store one
//                     value into TWO position fields use `fsts` for the
//                     first, so they show one fstps fewer
//        fsts     2 = top (y then z) and right (x then z)
//        flds   108 = 6 classify + 12 fDen heads + 18 epsilon (6 -eps,
//                     6 +eps, 6 0.0f) + 4 mid-plane numerators
//                     + 24 position/uv lerps + 30 byte-lerp reloads
//                     + 12 inlined transform + 1 (0.0f into fTotal)
//                     + 1 (0.5f)
//        faddp   40 = 30 byte-lerp bases + 2 (left, bottom numerators)
//                     + 8 (inlined transform)
//        fadds   33 = 24 position/uv lerps + 4 mid-plane denominators
//                     + 4 inlined transform + 1 (g_1008be30)
//        fsubs   28 = 24 position/uv lerps + 4 mid-plane denominators
//        fsub     6 = one fDen head per plane
//        fsubrs   2 = near and far numerators (plane constant minus z)
//        fsubrp   2 = top and right numerators   (DE EA; Intel FSUBP)
//        fmul    51 = 6 x (4 position/uv + 4 colour) + 3 inlined transform
//        fmuls   12 = the inlined transform only (four rows x three terms)
//        fmulp    6 = the fifth (specular) byte lerp of each plane, which
//                     is where fT dies
//        fdivp    6 = one per plane  (DE F1; Intel FDIVRP)
//        fdivrs   1 = `1.0f /` in the inlined transform
//        fchs     8 = 2 classify (left, bottom) + 4 numerator negations
//                     + 2 result negations (left x, bottom y)
//        fcomps   6 = the six classify tests
//        fcomp   12 = 6 in-body (-eps) + 6 sunk (+eps)
//        jbe      7 = far's classify + the six in-body -eps arms
//        jb      10 = 4 classify (left/top/right/bottom) + 6 sunk +eps
//        jae      1 = near's classify
//        fstp    16 = the sunk degenerate arms: 2+3+3+3+3+2
//        fld      2 = left and bottom (`fld %st(0) ; fchs` for -fZ)
//        sar      6 = the six `(pOut - pOutStart) >> 5`
//        shl     17 = 6 `pIn + nIn` + 6 `pIn + (nIn-1)` + 5 `pOut += nIn`
//                     -- the SIXTH `pOut += nIn` is dead and icl dropped it
//        imul     2 = the two `imul $0x20` of the fan-area loop
//        movzwl   5 = pObj->+0x50, the gather counter, and three in the
//                     area loop (all three uint16 promotions of `i`)
//        and      1 = `and $0x1,%eax`, the UseD3DClip mask
//        fstpl 3 / fldl 1 / fadd 1 / fmull 1 / faddl 1 = the DOUBLE
//                     accumulator fTotal at -0x70
//        call 4 / push 1 / pop 1 / ret 1
//
//      A seventh plane, a missing plane, a sixth interpolated byte, a
//      second texture pair or a non-inlined transform would each move at
//      least three of those rows.
//
//  (3) EXPRESSION-BY-EXPRESSION CORRESPONDENCE WITH A DECODED SIBLING.
//      The clipper here is 0x1000f4b0 inlined with all six plane bodies
//      inlined, and the byte-lerp helper 0x10023380 inlined inside those
//      -- exactly what src/engine/d3d/big572b4.cpp already carries for the
//      SAME 32-byte vertex.  It was diffed block by block against that
//      file, not eyeballed.  Plane for plane, with both classify sites:
//
//        bit    plane   classify            here          big572b4
//                               (classify-loop head / same in big572b4)
//        0x01   near    z >= g_1008b908     0x10024282    0x1005743c
//        0x04   left    -z < x              0x10024580    0x10057772
//        0x08   top     y < z               0x10024890    0x10057ac4
//        0x10   right   x < z               0x10024b90    0x10057e1f
//        0x20   bottom  -z < y              0x10024e90    0x10058198
//        0x02   far     z <= g_1008b910     0x10025190    0x10058515
//
//      Same order, same bits, same six epsilon pairs (in line 0x100a5d44,
//      0x100a5d4c, 0x100a5d54, 0x100a5d5c, 0x100a5d64, 0x100a5d6c, all
//      dumped and all 0xb727c5ac = -1e-5; sunk 0x100a5d9c/0x100a5da0,
//      0x100a5d94/0x100a5d98, 0x100a5d8c/0x100a5d90, 0x100a5d84/
//      0x100a5d88, 0x100a5d7c/0x100a5d80, 0x100a5d74/0x100a5d78, all
//      +1e-5 / 0.0f), same degenerate answer, same five bytes in the order
//      r, g, b, a, spec (+0x12, +0x11, +0x10, +0x13, +0x17), same
//      `*pOut = *pPrev` copy, same `pIn = pOutStart ; nIn = pOut -
//      pOutStart ; pOut += nIn` tail with the SAME sixth-plane omission,
//      same `(uint8)` conversions done as `fstps <float slot> ;
//      flds <same slot> ; fistpl <int slot>` with no fnstcw/fldcw dance
//      anywhere (thirty sites, /QIfist).
//
//      Two differences from big572b4, both listed and both accounted for
//      by the census: (a) this vertex carries ONE texture pair, so each
//      plane interpolates two floats (+0x18, +0x1c) where big572b4 also
//      does -- identical -- and (b) big572b4 spills fT to a DOUBLE slot
//      (`fstpl -0x70 ; fmull -0x70`) where this function keeps it in the
//      x87 stack (`fmul %st(1),%st`), which is register pressure, not
//      source.
//
//  ---------------------------------------------------------------------- //
//  WHAT IT IS: the OVERDRAW METER.  A string names it outright.
//  ---------------------------------------------------------------------- //
//
//  0x1002d893 does `flds 0x1008be30 ; movl $0x100a65b0,(%esp)` where
//  0x100a65b0 is "Tri area drawn: %.3f", and 0x1002d8ce reloads the same
//  global for "Overdraw: %.3f" (0x100a65d0) divided by
//  (g_1008b8a8 - g_1008b8a0) * (g_1008b8a4 - g_1008b89c) -- the viewport
//  width times its height.  0x1008be30 is the frame's total drawn triangle
//  area, and THIS FUNCTION IS THE ONLY THING THAT ADDS TO IT.  Searching
//  .text for the literal 0x1008be30 returns exactly FIVE operands and no
//  others: 0x1001ed9c's `movl $0x0,0x1008be30` at frame start, this
//  function's read/write pair at 0x10025511 / 0x1002551d, and the two
//  report sites above.
//
//  So: gather the polygon's positions, transform and clip it exactly as
//  the real draw path does, sum the signed areas of its fan triangles,
//  halve the sum, double it again when the surface's +0x38 flag word has
//  0x80 set (a two-pass surface draws every triangle twice), add that to
//  the meter -- and then, instead of drawing, hand the object to
//  0x10024020 so the real pass still gets it.
//
//  It is installed as a DRAW ENTRY POINT.  0x1002571f stores $0x10024090
//  into all four of 0x1008ce94/98/9c/a0 when g_1008cacc is set; the same
//  four slots get $0x10024020 when g_1008caa4 is set (0x10025748) and
//  0x1003dc80 / 0x1000d9f0 / 0x1000f650 / 0x10073140 -- the real draw
//  paths -- otherwise (0x100256d6..0x10025712).  Three interchangeable
//  implementations of one `void (*)(Obj*)`, which is why this one takes
//  one argument and returns nothing.
//
//  ---------------------------------------------------------------------- //
//  COMPILER: Intel C/C++ 4.0, **/O2** (with /QIfist).  The tells:
//    * OUTGOING ARGUMENTS as `add $-N,%esp` then `mov %reg,K(%esp)` --
//      0x100241f2 and 0x10025470 both `add $0xfffffff8,%esp` then two
//      `mov`s; 0x100254cc and 0x1002551a are the register-entry forms of
//      the same idiom.  There is no `push` anywhere in the function except
//      the prologue's `push %ebp`.
//    * `&p->m` as `mov $K,%reg ; add %p,%reg` -- all six plane tails do
//      `mov $0x20,%ecx ; add %eax,%ecx` for `pOut++` (0x10024508,
//      0x10024810, 0x10024b19, 0x10024e1a, 0x1002511f, 0x10025408), never
//      `lea 0x20(%eax),%ecx`.  (`lea` appears only against %ebp, where
//      both flags use it.)
//    * COLD-BLOCK SINKING below the epilogue, six-fold, 262 bytes.
//  RECORDED AGAINST, not explained away: there is ONE epilogue and every
//  exit cross-jumps to it -- `je 0x1002552b` from the six `nInside == 0`
//  tests and from the six rotated classify-loop guards.  That is the /Os
//  row of the table.  The argument idiom and the sinking are much the
//  stronger evidence, so the verdict is /O2 and the shared epilogue is a
//  `goto`-shaped source, which is how big6d910.cpp spells the same thing.
//
//  ---------------------------------------------------------------------- //
//  NOTHING IS UNDECODED.  There is no `// UNDECODED:` banner in this file
//  because there is no undecoded region: every instruction in
//  0x10024090..0x1002563f is accounted for by a statement below and by a
//  row of the census.  The coin-flips the acceptance test cannot settle
//  are listed at the bottom, each with both exact spellings.
//
// *********************************************************************** //

#include "ltbasedefs.h"


// ======================================================================= //
//  THE 32-BYTE DRAW VERTEX -- big124a4.cpp's / big16324.cpp's /
//  big572b4.cpp's RVert, unchanged.  Every offset below is a load or a
//  store in THESE bytes:
//    +0x00..+0x08  the position.  The gather loop writes the three floats
//                  one at a time (0x100240d5, 0x100240dc, 0x100240e7), the
//                  inlined transform writes x/z/y (0x100241bf, 0x100241c4,
//                  0x100241c7) and all six planes lerp them.
//    +0x0c         the reciprocal w.  Written by the inlined transform
//                  (0x100241c1) and by 0x10014580; carried through the
//                  clipper only by the whole-vertex copy.
//    +0x10..+0x13  the four diffuse bytes, interpolated in the order
//                  r, g, b, a -- that is +0x12, +0x11, +0x10, +0x13, read
//                  off plane one at 0x100243b5, 0x100243f9, 0x1002443d,
//                  0x10024481.
//    +0x14..+0x16  never touched except by the copy; present so that +0x17
//                  sits where the bytes put it.
//    +0x17         the one specular byte the clipper interpolates
//                  (0x100244c5).
//    +0x18/+0x1c   the single texture pair (0x10024399 / 0x100243a7).
// ======================================================================= //
struct Vec24090
{
	float		x;                                      // +0x00
	float		y;                                      // +0x04
	float		z;                                      // +0x08
};

struct RVert24090                                       // 0x20 bytes
{
	Vec24090	m_vPos;                                 // +0x00
	float		m_fRHW;                                 // +0x0c
	uint8		m_b;                                    // +0x10
	uint8		m_g;                                    // +0x11
	uint8		m_r;                                    // +0x12
	uint8		m_a;                                    // +0x13
	uint8		m_sb;                                   // +0x14
	uint8		m_sg;                                   // +0x15
	uint8		m_sr;                                   // +0x16
	uint8		m_sa;                                   // +0x17
	float		m_fU;                                   // +0x18
	float		m_fV;                                   // +0x1c
};


// ======================================================================= //
//  THE SOURCE VERTEX at pObj+0x54 -- big6d910.cpp's SrcVert6d910 and
//  big572b4.cpp's 0x18-byte record.  Stride 0x18 from `add $0x18,%edx` at
//  0x100240e1, and +0x00 is a POINTER: 0x100240d0 does `mov (%edx),%ecx`
//  and then reads (%ecx), 0x4(%ecx) and 0x8(%ecx).  THIS function reads
//  nothing else out of the record -- it wants positions only -- so the
//  remaining 20 bytes are padding here and are named in big6d910.cpp.
// ======================================================================= //
struct SrcVert24090                                     // 0x18 bytes
{
	Vec24090*	m_pPos00;                               // +0x00
	uint8		m_pad_04[0x18 - 0x04];                  // +0x04 .. +0x17
};


// ======================================================================= //
//  THE OBJECT.  Three members are read and they are big124a4.cpp's and
//  big6d910.cpp's, unchanged:
//    +0x28  the context/holder pointer (0x100254f3), whose +0x38 flag word
//           is big124a4.cpp's Ctx124a4::m_nFlags38 (0x100254f6); bit 0x80
//           is tested at 0x100254f9.
//    +0x50  a uint16 count -- `movzwl 0x50(%edx),%ecx` at 0x100240ad.
//    +0x54  the array of 0x18-byte source vertices.
// ======================================================================= //
struct Ctx24090
{
	uint8		m_pad_00[0x38];                         // +0x00 .. +0x37
	uint32		m_nFlags38;                             // +0x38
};

struct Obj24090
{
	uint8			m_pad_00[0x28];                     // +0x00 .. +0x27
	Ctx24090*		m_pCtx28;                           // +0x28
	uint8			m_pad_2c[0x50 - 0x2c];              // +0x2c .. +0x4f
	uint16			m_nCount50;                         // +0x50
	uint8			m_pad_52[0x54 - 0x52];              // +0x52 .. +0x53
	SrcVert24090	m_aSrc54[1];                        // +0x54
};


// ======================================================================= //
//  THE TRANSFORM at 0x1008b880.  0x1008b880 + 0x1dc = 0x1008ba5c, and the
//  sixteen floats 0x1008ba5c..0x1008ba98 are exactly the sixteen operands
//  of the inlined projective transform at 0x1002411b..0x100241ce, in four
//  rows of four.  big6d910.cpp's XForm6d910 names the same block from the
//  same +0x1dc..+0x218 offsets; this is that struct, cut to the block.
// ======================================================================= //
struct XForm24090
{
	uint8		m_pad_000[0x1dc];                       // +0x000 .. +0x1db
	float		m_f1dc;                                 // 0x1008ba5c
	float		m_f1e0;                                 // 0x1008ba60
	float		m_f1e4;                                 // 0x1008ba64
	float		m_f1e8;                                 // 0x1008ba68
	float		m_f1ec;                                 // 0x1008ba6c
	float		m_f1f0;                                 // 0x1008ba70
	float		m_f1f4;                                 // 0x1008ba74
	float		m_f1f8;                                 // 0x1008ba78
	float		m_f1fc;                                 // 0x1008ba7c
	float		m_f200;                                 // 0x1008ba80
	float		m_f204;                                 // 0x1008ba84
	float		m_f208;                                 // 0x1008ba88
	float		m_f20c;                                 // 0x1008ba8c
	float		m_f210;                                 // 0x1008ba90
	float		m_f214;                                 // 0x1008ba94
	float		m_f218;                                 // 0x1008ba98
};


// ======================================================================= //
//  GLOBALS -- every one is an address this function touches.  The first
//  six are the clipper's state and are already named by d3dr3a.cpp,
//  big124a4.cpp, big6d910.cpp and big572b4.cpp; only 0x1008be30 is new,
//  and the string at 0x100a65b0 names it.
// ======================================================================= //
extern uint32		g_1008b874;                         // clip flag word
extern RVert24090*	g_1008b87c;                         // clip scratch cursor
extern uint8		g_1008b880[];                       // the transform
extern float		g_1008b908;                         // near plane
extern float		g_1008b910;                         // far plane
extern uint32		g_1008be44;                         // clipped-poly count
extern uint32		g_1008d7a0;                         // UseD3DClip

// 0x1008be30 -- the frame's total drawn triangle area.  Zeroed by
// 0x1001eda0, added to only here, reported by 0x1002d893 ("Tri area drawn:
// %.3f") and 0x1002d8ce ("Overdraw: %.3f").
extern float		g_1008be30;


// ======================================================================= //
//  THE FOUR CALLEES.  None is defined here; all four are decoded
//  elsewhere and none may be inlined.
// ======================================================================= //

// 0x1000d9b0 -- d3dlow.cpp: transform a position by the 3x4 at +0x15c of
// the second argument (it tail-calls 0x1000a770).  big572b4.cpp and
// big6d910.cpp both call it in exactly this loop.
extern "C" void  sub_1000d9b0(RVert24090* pVert, uint8* pXForm);

// 0x10014580 -- the screen projection through +0x31c..+0x330, decoded in
// full at that address and called the same way by big124a4.cpp,
// big572b4.cpp and d3dr2a.cpp.
extern "C" void  sub_10014580(RVert24090* pVert, uint8* pXForm);

// 0x10024020 / 0x10025640 -- the two /Qip pairs this function calls at
// their REGISTER entries (0x100254cf -> 0x1002564c, 0x10025523 ->
// 0x10024024).  Their decodes, and the proof of the extents, are in
// big24020.cpp; the bodies live HERE because icl emits
// the register entry, and a caller that reaches it, only when caller and
// callee share one /Qip translation unit.  Measured, this file at
// /O2 /Qip /Qip_no_inlining (D3D_QIP_NI_TUS): 0x10024020, 0x10024024,
// 0x10025640 and 0x1002564c all byte-identical (they were 50 and 32
// bytes off as big24020.cpp's stack-only /O2 functions).  Plain /Qip
// inlines 0x10025640 into this caller and emits only its stack entry
// (the same /Qip_no_inlining finding as d3dqip4.cpp and the w*_qipni
// files).  0x10024090 itself is far off either way (7989 bytes differing
// at /O2, 8067 at /Qip /Qip_no_inlining; see note (9) below for /QIfist).

// 0x10024020 -- __cdecl, one argument (register entry 0x10024024: %eax).
// Pops an element off the 0x18-byte pool 0x1008d4e4 through StdLith's
// inline sb_Allocate (struct_bank.h; out of line at 0x1000a6a0), stamps
// the object and the current clip flags into it and links it onto the
// deferred list 0x1008d4bc.  Declared identically in big124a4.cpp and
// big6d910.cpp.  The element is stored through untested, as 0x1003dc80
// (w15_1000d9f0_os.cpp) does -- the shipped `xor %eax,%eax` at
// 0x1002407f is sb_Allocate's own NULL return, not a test here.
struct Elem24024                                        // 0x18 bytes
{
	void*       m_pOwner;                               // +0x00
	uint32      m_n04;                                  // +0x04  not written here
	uint32      m_n08;                                  // +0x08  not written here
	uint32      m_n0c;                                  // +0x0c  not written here
	uint32      m_nClipFlags10;                         // +0x10
	Elem24024*  m_pNext14;                              // +0x14
};

// StdLith's StructBank (struct_bank.h), as w15_1000d9f0_os.cpp has it.
struct StructLink124a4
{
	StructLink124a4*  m_pSLNext;
};

struct Pool124a4
{
	uint32            m_StructSize;                     // +0x00
	uint32            m_AlignedStructSize;              // +0x04
	uint32            m_CacheSize;                      // +0x08
	uint32            m_nPages;                         // +0x0c
	uint32            m_nTotalObjects;                  // +0x10
	void*             m_PageHead;                       // +0x14
	StructLink124a4*  m_FreeListHead;                   // +0x18  0x1008d4fc
};

extern Pool124a4    g_1008d4e4;                         // 0x1008d4e4  element pool
extern Elem24024*   g_1008d4bc;                         // 0x1008d4bc  deferred list

// 0x1007fbf0 -- StdLith's sb_AllocateNewStructPage.
extern "C" int  sub_1007fbf0(Pool124a4* pBank, uint32 nAllocations);

inline void* sb_Allocate124a4(Pool124a4* pBank)
{
	StructLink124a4* pRet;

	pRet = pBank->m_FreeListHead;
	if (!pRet)
	{
		sub_1007fbf0(pBank, pBank->m_CacheSize);
		pRet = pBank->m_FreeListHead;
		if (!pRet)
			return 0;
	}

	pBank->m_FreeListHead = pRet->m_pSLNext;
	return pRet;
}

extern "C" void sub_10024020(Obj24090* pObj)
{
	Elem24024* pElem;

	pElem = (Elem24024*)sb_Allocate124a4(&g_1008d4e4);
	pElem->m_pOwner       = pObj;
	pElem->m_pNext14      = g_1008d4bc;
	pElem->m_nClipFlags10 = g_1008b874;
	g_1008d4bc            = pElem;
}

// 0x10025640 -- TWICE the signed area of the triangle (a, b, c), read
// straight off 0x1002564c..0x10025674 (defined at the end of the file, in
// its shipped place after this caller).
extern "C" float sub_10025640(const RVert24090* pA, const RVert24090* pB,
                              const RVert24090* pC);


// ======================================================================= //
//  The clip flag array's bound.  Read off the frame, not assumed: each
//  pass's array is 0xc8 = 200 bytes (fifty int32) and the six arrays are
//  0xdc apart at -0x5bc, -0x4e0, -0x404, -0x328, -0x24c and -0x170, with
//  the five int32 conversion slots filling the remaining 0x14.  Same
//  fifty as big16324.cpp, big572b4.cpp, big6d910.cpp and big0f6f0.cpp.
// ======================================================================= //


// ----------------------------------------------------------------------- //
//  0x10023380's body, INLINE, as a macro so that no call can be emitted
//  for it.  d3dbatchd.cpp describes that helper as "two floats at +0x18
//  and +0x1c interpolated normally, then five bytes (+0x12, +0x11, +0x10,
//  +0x13, +0x17)"; this vertex has ONE texture pair, so this is the
//  helper's own shape, unextended.  There is no `call 0x10023380`
//  anywhere in the shipped 5.5 KB.
//
//  The byte differences are taken in the INTEGER unit (`movzbl ; movzbl ;
//  sub` at 0x100243b5..0x100243bd) and only then converted; the result
//  goes back through a named float slot and a bare `fistpl`
//  (`fstps -0xa70(%ebp) ; flds -0xa70(%ebp) ; fistpl -0x4f4(%ebp)` at
//  0x100243db) -- that is /QIfist, with no fnstcw/fldcw dance at any of
//  the thirty sites.  Under plain /O2 a `(uint8)` cast off a float emits
//  the control-word dance instead and those thirty sites will differ in
//  bytes; they contain no calls, so the call sequence cannot show the difference.
// ----------------------------------------------------------------------- //
// Round to nearest in the current FPU mode (w1_10001000.cpp's helper; the
// out-of-line copy is 0x10015190): `fstps tmp ; flds tmp ; fistpl n` is
// this __asm body inlined.
inline int32 RoundF(float f)
{
	int32 n;
	__asm fld f
	__asm fistp n
	return n;
}


// (THE REAL CLIPPER, KEPT -- attempted -> near.  The six
// CLIP24090 passes are now d3dr3c_os.cpp's driver 0x1000f4b0 over the six
// plane bodies, `static inline` (copied from w55_10016324_os.cpp), called
// as `if (!sub_1000f4b0(g_1008b874, &pVerts, &nVerts)) goto L_Done;`.
// Three things together, each measured:
//    * /Qsfalign16 (engine.sh's new D3D_QIP_NI_SF_TUS): without it icl
//      gives the realigning entry (fTotal is a double held in the frame);
//    * in each plane body `pV = &(*ppVerts)[0]` rather than `*ppVerts`:
//      that is what keeps the driver's pVerts a MEMORY scalar (-0x68,
//      stored at every plane's end and re-read at every use, as shipped);
//      with the plain spelling icl promotes it and the frame comes out
//      0xa88 against 0xa90.  With it the frame is exact and every slot
//      lands where shipped has it: nVerts -0x984, aVerts -0x980, pVerts
//      -0x5c0, the six clip blocks from -0x5bc;
//    * nVerts assigned BEFORE pVerts (`nGather = ..; nVerts = nGather;`
//      first): pfx 20 -> 24, 2813 -> 2799 bytes differing;
//    * the reject as `goto L_Done` (the shared epilogue) and not `return`:
//      with `return` icl lays the epilogue after the sunk epsilon arms,
//      ends the body in `ret`, and the epilogue test rejects the
//      pairing; with the goto the body is the shipped 5,552 bytes with the
//      `ret` at the shipped place.
// Now: 5,552 of 5,552 bytes, first 24 identical, frame 0xa90 exact,
// 2,783 bytes differing (register choice), normalised instruction match
// 0.58 -> 0.90; 0x10024020/0x10025640 still identical.  The out-of-line
// copies icl emits of the static inlines under /Qip_no_inlining are
// unreferenced and unplaced (as in w30_1000f020_osqipni.cpp).)
// (RoundF above, at all thirty sites -- 8,560 -> 5,040
// bytes against the shipped 5,564.  The frame is 0xa78 against 0xa90;
// the thirty RoundF slot pairs sit exactly as shipped, 0x18 bytes lower.)


// ----------------------------------------------------------------------- //
//  The six clip passes.  Everything except the plane test, the
//  denominator, the numerator and which of x/y/z the plane pins is
//  identical; each plane body below is inlined once, which is what the six
//  near-identical shipped blocks are.
//
//  The classify loop is emitted pointer-walking (`add $0x20,%edx ;
//  cmp %esi,%edx` at 0x10024292/0x100242a6) and the edge loop
//  index-walking on the flag array and pointer-walking on the vertices
//  (`mov -0x5bc(%ebp,%edx,4),%eax` at 0x100242f0, `add $0x20,%esi` at
//  0x1002451a); the plane bodies below are written that way.
//
//  `if (nInside == 0)` rejects the WHOLE polygon: the shipped target is
//  0x1002552b, the register-restore epilogue, NOT the area loop -- a
//  polygon that is entirely off-screen contributes no area and is not
//  even handed to 0x10024020.  The `cmp %edx,%ebx ; je 0x1002552b` guard
//  at the TOP of each classify loop (0x10024274, 0x10024572, ...) is the
//  same reject: icl rotated the loop and folded "the loop never ran,
//  therefore nInside is still zero".
// ----------------------------------------------------------------------- //


// ======================================================================= //
//  0x10024090 .. 0x1002563f   (5,552 bytes)
//
//  Installed into 0x1008ce94..0x1008cea0 by 0x1002571f, so the signature
//  is the same `void (*)(Obj*)` that 0x10024020, 0x1003dc80, 0x1000d9f0,
//  0x1000f650 and 0x10073140 fill those slots with.  One stack argument
//  (`mov 0x8(%ebp),%edx` at 0x100240a4), no return value.
//
//  The declaration order below is the frame order the shipped code uses,
//  lowest slot first: the thirty float temps of the six lerp blocks are
//  compiler temps and are not named; nVerts sits at -0x984 in their last
//  slot's spare half; aVerts is -0x980, pVerts -0x5c0, the six clip
//  blocks -0x5bc..-0x95 and every named scalar is in -0x90..-0x01.
// ======================================================================= //
#define CLIP_EPSILON    0.00001f
#define CLIP_MAX_VERTS  50

static inline void LerpRVert(RVert24090* a, RVert24090* b, RVert24090* dst, float t)
{
	dst->m_fU = (b->m_fU - a->m_fU) * t + a->m_fU;
	dst->m_fV = (b->m_fV - a->m_fV) * t + a->m_fV;
	dst->m_r  = (uint8)RoundF((b->m_r  - a->m_r)  * t + a->m_r);
	dst->m_g  = (uint8)RoundF((b->m_g  - a->m_g)  * t + a->m_g);
	dst->m_b  = (uint8)RoundF((b->m_b  - a->m_b)  * t + a->m_b);
	dst->m_a  = (uint8)RoundF((b->m_a  - a->m_a)  * t + a->m_a);
	dst->m_sa = (uint8)RoundF((b->m_sa - a->m_sa) * t + a->m_sa);
}


static inline float ClipEdgeNear(Vec24090* a, Vec24090* b, Vec24090* dst)
{
	float fDenom = b->z - a->z;
	float t;

	if (fDenom < -CLIP_EPSILON || fDenom > CLIP_EPSILON)
		t = (g_1008b908 - a->z) / fDenom;
	else
		t = 0.0f;

	dst->x = (b->x - a->x) * t + a->x;
	dst->y = (b->y - a->y) * t + a->y;
	dst->z = g_1008b908;

	return t;
}


static inline float ClipEdgeLeft(Vec24090* a, Vec24090* b, Vec24090* dst)
{
	float fDenom = (b->x - a->x) + b->z - a->z;
	float t;
	float fZ;

	if (fDenom < -CLIP_EPSILON || fDenom > CLIP_EPSILON)
		t = -(a->x + a->z) / fDenom;
	else
		t = 0.0f;

	dst->y = (b->y - a->y) * t + a->y;
	fZ = (b->z - a->z) * t + a->z;
	dst->z = fZ;
	dst->x = -fZ;

	return t;
}


static inline float ClipEdgeTop(Vec24090* a, Vec24090* b, Vec24090* dst)
{
	float fDenom = (b->y - a->y) - b->z + a->z;
	float t;
	float fZ;

	if (fDenom < -CLIP_EPSILON || fDenom > CLIP_EPSILON)
		t = -(a->y - a->z) / fDenom;
	else
		t = 0.0f;

	dst->x = (b->x - a->x) * t + a->x;
	fZ = (b->z - a->z) * t + a->z;
	dst->z = fZ;
	dst->y = fZ;

	return t;
}


static inline float ClipEdgeRight(Vec24090* a, Vec24090* b, Vec24090* dst)
{
	float fDenom = (b->x - a->x) - b->z + a->z;
	float t;
	float fZ;

	if (fDenom < -CLIP_EPSILON || fDenom > CLIP_EPSILON)
		t = -(a->x - a->z) / fDenom;
	else
		t = 0.0f;

	dst->y = (b->y - a->y) * t + a->y;
	fZ = (b->z - a->z) * t + a->z;
	dst->z = fZ;
	dst->x = fZ;

	return t;
}


static inline float ClipEdgeBottom(Vec24090* a, Vec24090* b, Vec24090* dst)
{
	float fDenom = (b->y - a->y) + b->z - a->z;
	float t;
	float fZ;

	if (fDenom < -CLIP_EPSILON || fDenom > CLIP_EPSILON)
		t = -(a->y + a->z) / fDenom;
	else
		t = 0.0f;

	dst->x = (b->x - a->x) * t + a->x;
	fZ = (b->z - a->z) * t + a->z;
	dst->z = fZ;
	dst->y = -fZ;

	return t;
}


static inline float ClipEdgeFar(Vec24090* a, Vec24090* b, Vec24090* dst)
{
	float fDenom = b->z - a->z;
	float t;

	if (fDenom < -CLIP_EPSILON || fDenom > CLIP_EPSILON)
		t = (g_1008b910 - a->z) / fDenom;
	else
		t = 0.0f;

	dst->x = (b->x - a->x) * t + a->x;
	dst->y = (b->y - a->y) * t + a->y;
	dst->z = g_1008b910;

	return t;
}


// 0x10001000
static inline LTBOOL sub_10001000(uint32* pUnused, RVert24090** ppVerts, int32* pnVerts, RVert24090** ppOut)
{
	uint32     anInside[CLIP_MAX_VERTS];
	RVert24090*  pOutStart;
	RVert24090*  pV;
	RVert24090*  pEnd;
	RVert24090*  pPrev;
	RVert24090*  pCur;
	uint32*    pFlag;
	int32      nInside;
	int32      i;
	int32      iPrev;

	++g_1008be44;

	nInside = 0;
	pFlag   = anInside;
	pV      = &(*ppVerts)[0];
	pEnd    = pV + *pnVerts;

	for (; pV != pEnd; ++pV)
	{
		uint32 bIn = (pV->m_vPos.z >= g_1008b908);

		nInside += bIn;
		*pFlag++ = bIn;
	}

	if (nInside == 0)
		return LTFALSE;

	if (nInside != *pnVerts)
	{
		pOutStart = *ppOut;
		iPrev     = *pnVerts - 1;
		pPrev     = *ppVerts + iPrev;

		for (i = 0; i < *pnVerts; ++i)
		{
			pCur = *ppVerts + i;

			if (anInside[iPrev])
				*(*ppOut)++ = *pPrev;

			if (anInside[iPrev] != anInside[i])
			{
				float t = ClipEdgeNear(&pPrev->m_vPos, &pCur->m_vPos, &(*ppOut)->m_vPos);

				LerpRVert(pPrev, pCur, *ppOut, t);

				++(*ppOut);
			}

			iPrev = i;
			pPrev = pCur;
		}

		*pnVerts = *ppOut - pOutStart;
		*ppVerts = pOutStart;
		*ppOut  += *pnVerts;
	}

	return LTTRUE;
}


// 0x10001330
static inline LTBOOL sub_10001330(uint32* pUnused, RVert24090** ppVerts, int32* pnVerts, RVert24090** ppOut)
{
	uint32     anInside[CLIP_MAX_VERTS];
	RVert24090*  pOutStart;
	RVert24090*  pV;
	RVert24090*  pEnd;
	RVert24090*  pPrev;
	RVert24090*  pCur;
	uint32*    pFlag;
	int32      nInside;
	int32      i;
	int32      iPrev;

	++g_1008be44;

	nInside = 0;
	pFlag   = anInside;
	pV      = &(*ppVerts)[0];
	pEnd    = pV + *pnVerts;

	for (; pV != pEnd; ++pV)
	{
		uint32 bIn = (-pV->m_vPos.z < pV->m_vPos.x);

		nInside += bIn;
		*pFlag++ = bIn;
	}

	if (nInside == 0)
		return LTFALSE;

	if (nInside != *pnVerts)
	{
		pOutStart = *ppOut;
		iPrev     = *pnVerts - 1;
		pPrev     = *ppVerts + iPrev;

		for (i = 0; i < *pnVerts; ++i)
		{
			pCur = *ppVerts + i;

			if (anInside[iPrev])
				*(*ppOut)++ = *pPrev;

			if (anInside[iPrev] != anInside[i])
			{
				float t = ClipEdgeLeft(&pPrev->m_vPos, &pCur->m_vPos, &(*ppOut)->m_vPos);

				LerpRVert(pPrev, pCur, *ppOut, t);

				++(*ppOut);
			}

			iPrev = i;
			pPrev = pCur;
		}

		*pnVerts = *ppOut - pOutStart;
		*ppVerts = pOutStart;
		*ppOut  += *pnVerts;
	}

	return LTTRUE;
}


// 0x10001670
static inline LTBOOL sub_10001670(uint32* pUnused, RVert24090** ppVerts, int32* pnVerts, RVert24090** ppOut)
{
	uint32     anInside[CLIP_MAX_VERTS];
	RVert24090*  pOutStart;
	RVert24090*  pV;
	RVert24090*  pEnd;
	RVert24090*  pPrev;
	RVert24090*  pCur;
	uint32*    pFlag;
	int32      nInside;
	int32      i;
	int32      iPrev;

	++g_1008be44;

	nInside = 0;
	pFlag   = anInside;
	pV      = &(*ppVerts)[0];
	pEnd    = pV + *pnVerts;

	for (; pV != pEnd; ++pV)
	{
		uint32 bIn = (pV->m_vPos.y < pV->m_vPos.z);

		nInside += bIn;
		*pFlag++ = bIn;
	}

	if (nInside == 0)
		return LTFALSE;

	if (nInside != *pnVerts)
	{
		pOutStart = *ppOut;
		iPrev     = *pnVerts - 1;
		pPrev     = *ppVerts + iPrev;

		for (i = 0; i < *pnVerts; ++i)
		{
			pCur = *ppVerts + i;

			if (anInside[iPrev])
				*(*ppOut)++ = *pPrev;

			if (anInside[iPrev] != anInside[i])
			{
				float t = ClipEdgeTop(&pPrev->m_vPos, &pCur->m_vPos, &(*ppOut)->m_vPos);

				LerpRVert(pPrev, pCur, *ppOut, t);

				++(*ppOut);
			}

			iPrev = i;
			pPrev = pCur;
		}

		*pnVerts = *ppOut - pOutStart;
		*ppVerts = pOutStart;
		*ppOut  += *pnVerts;
	}

	return LTTRUE;
}


// 0x10001990
static inline LTBOOL sub_10001990(uint32* pUnused, RVert24090** ppVerts, int32* pnVerts, RVert24090** ppOut)
{
	uint32     anInside[CLIP_MAX_VERTS];
	RVert24090*  pOutStart;
	RVert24090*  pV;
	RVert24090*  pEnd;
	RVert24090*  pPrev;
	RVert24090*  pCur;
	uint32*    pFlag;
	int32      nInside;
	int32      i;
	int32      iPrev;

	++g_1008be44;

	nInside = 0;
	pFlag   = anInside;
	pV      = &(*ppVerts)[0];
	pEnd    = pV + *pnVerts;

	for (; pV != pEnd; ++pV)
	{
		uint32 bIn = (pV->m_vPos.x < pV->m_vPos.z);

		nInside += bIn;
		*pFlag++ = bIn;
	}

	if (nInside == 0)
		return LTFALSE;

	if (nInside != *pnVerts)
	{
		pOutStart = *ppOut;
		iPrev     = *pnVerts - 1;
		pPrev     = *ppVerts + iPrev;

		for (i = 0; i < *pnVerts; ++i)
		{
			pCur = *ppVerts + i;

			if (anInside[iPrev])
				*(*ppOut)++ = *pPrev;

			if (anInside[iPrev] != anInside[i])
			{
				float t = ClipEdgeRight(&pPrev->m_vPos, &pCur->m_vPos, &(*ppOut)->m_vPos);

				LerpRVert(pPrev, pCur, *ppOut, t);

				++(*ppOut);
			}

			iPrev = i;
			pPrev = pCur;
		}

		*pnVerts = *ppOut - pOutStart;
		*ppVerts = pOutStart;
		*ppOut  += *pnVerts;
	}

	return LTTRUE;
}


// 0x10001cb0
static inline LTBOOL sub_10001cb0(uint32* pUnused, RVert24090** ppVerts, int32* pnVerts, RVert24090** ppOut)
{
	uint32     anInside[CLIP_MAX_VERTS];
	RVert24090*  pOutStart;
	RVert24090*  pV;
	RVert24090*  pEnd;
	RVert24090*  pPrev;
	RVert24090*  pCur;
	uint32*    pFlag;
	int32      nInside;
	int32      i;
	int32      iPrev;

	++g_1008be44;

	nInside = 0;
	pFlag   = anInside;
	pV      = &(*ppVerts)[0];
	pEnd    = pV + *pnVerts;

	for (; pV != pEnd; ++pV)
	{
		uint32 bIn = (-pV->m_vPos.z < pV->m_vPos.y);

		nInside += bIn;
		*pFlag++ = bIn;
	}

	if (nInside == 0)
		return LTFALSE;

	if (nInside != *pnVerts)
	{
		pOutStart = *ppOut;
		iPrev     = *pnVerts - 1;
		pPrev     = *ppVerts + iPrev;

		for (i = 0; i < *pnVerts; ++i)
		{
			pCur = *ppVerts + i;

			if (anInside[iPrev])
				*(*ppOut)++ = *pPrev;

			if (anInside[iPrev] != anInside[i])
			{
				float t = ClipEdgeBottom(&pPrev->m_vPos, &pCur->m_vPos, &(*ppOut)->m_vPos);

				LerpRVert(pPrev, pCur, *ppOut, t);

				++(*ppOut);
			}

			iPrev = i;
			pPrev = pCur;
		}

		*pnVerts = *ppOut - pOutStart;
		*ppVerts = pOutStart;
		*ppOut  += *pnVerts;
	}

	return LTTRUE;
}


// 0x10001fe0
static inline LTBOOL sub_10001fe0(uint32* pUnused, RVert24090** ppVerts, int32* pnVerts, RVert24090** ppOut)
{
	uint32     anInside[CLIP_MAX_VERTS];
	RVert24090*  pOutStart;
	RVert24090*  pV;
	RVert24090*  pEnd;
	RVert24090*  pPrev;
	RVert24090*  pCur;
	uint32*    pFlag;
	int32      nInside;
	int32      i;
	int32      iPrev;

	++g_1008be44;

	nInside = 0;
	pFlag   = anInside;
	pV      = &(*ppVerts)[0];
	pEnd    = pV + *pnVerts;

	for (; pV != pEnd; ++pV)
	{
		uint32 bIn = (pV->m_vPos.z <= g_1008b910);

		nInside += bIn;
		*pFlag++ = bIn;
	}

	if (nInside == 0)
		return LTFALSE;

	if (nInside != *pnVerts)
	{
		pOutStart = *ppOut;
		iPrev     = *pnVerts - 1;
		pPrev     = *ppVerts + iPrev;

		for (i = 0; i < *pnVerts; ++i)
		{
			pCur = *ppVerts + i;

			if (anInside[iPrev])
				*(*ppOut)++ = *pPrev;

			if (anInside[iPrev] != anInside[i])
			{
				float t = ClipEdgeFar(&pPrev->m_vPos, &pCur->m_vPos, &(*ppOut)->m_vPos);

				LerpRVert(pPrev, pCur, *ppOut, t);

				++(*ppOut);
			}

			iPrev = i;
			pPrev = pCur;
		}

		*pnVerts = *ppOut - pOutStart;
		*ppVerts = pOutStart;
		*ppOut  += *pnVerts;
	}

	return LTTRUE;
}


// 0x1000f4b0 -- d3dr3c_os.cpp's driver.
static inline LTBOOL sub_1000f4b0(uint32 nClipFlags, RVert24090** ppVerts, int32* pnVerts)
{
	RVert24090*     pOut;
	RVert24090*     pVerts;
	int32      nVerts;
	uint32     anUnused[6];

	if (g_1008d7a0)
	{
		nClipFlags &= 1;
		if (!nClipFlags)
			return LTTRUE;
	}

	pOut   = g_1008b87c;
	pVerts = *ppVerts;
	nVerts = *pnVerts;

	if (nClipFlags & 0x01)
	{
		if (!sub_10001000(&anUnused[0], &pVerts, &nVerts, &pOut))
			return LTFALSE;
	}

	if (nClipFlags & 0x04)
	{
		if (!sub_10001330(&anUnused[1], &pVerts, &nVerts, &pOut))
			return LTFALSE;
	}

	if (nClipFlags & 0x08)
	{
		if (!sub_10001670(&anUnused[2], &pVerts, &nVerts, &pOut))
			return LTFALSE;
	}

	if (nClipFlags & 0x10)
	{
		if (!sub_10001990(&anUnused[3], &pVerts, &nVerts, &pOut))
			return LTFALSE;
	}

	if (nClipFlags & 0x20)
	{
		if (!sub_10001cb0(&anUnused[4], &pVerts, &nVerts, &pOut))
			return LTFALSE;
	}

	if (!(nClipFlags & 0x02) ||
	    sub_10001fe0(&anUnused[5], &pVerts, &nVerts, &pOut))
	{
		*ppVerts = pVerts;
		*pnVerts = nVerts;
		return LTTRUE;
	}

	return LTFALSE;
}


void sub_10024090(Obj24090* pObj)
{
	RVert24090      aVerts[30];
	RVert24090*     pVerts;
	int32           nVerts;

	SrcVert24090*   pSrc;
	RVert24090*     pWalk;
	uint16          nGather;
	int32           nLeft;

	XForm24090*     pM;
	float           fOOW, fSx, fSy, fSz;

	double          fTotal;
	uint16          i;

	// ------------------------------------------------------------------ //
	//  0x100240a4 -- THE GATHER.  Positions only: the loop writes +0x00,
	//  +0x04 and +0x08 and nothing else, which is why the colours and the
	//  texture pair the clipper interpolates below are never initialised.
	//  That is the whole point -- this pass measures AREA, not pixels.
	//
	//  `mov (%edx),%ecx` is emitted THREE times (0x100240d0, 0x100240d7,
	//  0x100240df) for the one source pointer, which is "no
	//  CSE through memory" and says the source is three statements, not
	//  `pWalk->m_vPos = *pSrc->m_pPos00` (that would be `rep movsl $3`,
	//  which is what big572b4.cpp's copy of this loop emits).
	//
	//  The counter is uint16 and counts DOWN: `dec %ebx` at 0x100240d2,
	//  then `mov %ebx,%ecx ; movzwl %cx,%ecx ; test %ecx,%ecx ; jg` at
	//  0x100240ea.  The 16-bit truncation on every iteration is what makes
	//  it a 16-bit SOURCE variable rather than a reversed induction
	//  variable, and the `jg` is the signed compare of the promoted value.
	//  ONE `movzwl 0x50(%edx)` in the whole function (0x100240ad) feeds
	//  both it and nVerts, so pObj->m_nCount50 is read once.
	// ------------------------------------------------------------------ //
	nGather = pObj->m_nCount50;
	nVerts  = nGather;
	pSrc    = pObj->m_aSrc54;
	pVerts  = aVerts;
	pWalk   = aVerts;


	while (nGather > 0)
	{
		pWalk->m_vPos.x = pSrc->m_pPos00->x;
		pWalk->m_vPos.y = pSrc->m_pPos00->y;
		pWalk->m_vPos.z = pSrc->m_pPos00->z;

		pSrc++;
		pWalk++;
		nGather--;
	}

	// ------------------------------------------------------------------ //
	//  0x10024102 -- THE CLIP DECISION.  g_1008b874 is read here and read
	//  AGAIN at 0x1002420a; big6d910.cpp and 0x1000a8a0 both re-read it in
	//  the same place, so the second read is source and not a missed CSE.
	// ------------------------------------------------------------------ //
	if (!g_1008b874)
	{
		// -------------------------------------------------------------- //
		//  0x1002411b -- NO CLIPPING WANTED.  One projective transform per
		//  vertex through 0x1008b880's +0x1dc..+0x218 block, `fdivrs 1.0f`
		//  for the reciprocal (0x1002413f; 0x100a5d38 was dumped and is
		//  0x3f800000).  The identical instruction sequence, operand for
		//  operand, is 0x1006da4e in big6d910.cpp, 0x1000a8d0 inside
		//  0x1000a8a0 and 0x10012a65 inside big124a4.cpp -- REUSED, not
		//  re-derived.  The one difference is that this copy stores the
		//  four results straight into the vertex (`fstps (%eax)`,
		//  `fstps 0xc(%eax)`, `fstps 0x8(%eax)`, `fstps 0x4(%eax)` at
		//  0x100241bf) where big6d910.cpp assembles a three-float local
		//  and `rep movsl`s it.
		// -------------------------------------------------------------- //
		pM     = (XForm24090*)g_1008b880;
		pWalk  = pVerts;
		nLeft  = nVerts;

		while (nLeft--)
		{
			fOOW = 1.0f / (pWalk->m_vPos.x * pM->m_f20c
			             + pWalk->m_vPos.y * pM->m_f210
			             + pWalk->m_vPos.z * pM->m_f214
			             + pM->m_f218);

			fSx = (pWalk->m_vPos.x * pM->m_f1dc
			     + pWalk->m_vPos.y * pM->m_f1e0
			     + pWalk->m_vPos.z * pM->m_f1e4
			     + pM->m_f1e8) * fOOW;

			fSy = (pWalk->m_vPos.x * pM->m_f1ec
			     + pWalk->m_vPos.y * pM->m_f1f0
			     + pWalk->m_vPos.z * pM->m_f1f4
			     + pM->m_f1f8) * fOOW;

			fSz = (pWalk->m_vPos.x * pM->m_f1fc
			     + pWalk->m_vPos.y * pM->m_f200
			     + pWalk->m_vPos.z * pM->m_f204
			     + pM->m_f208) * fOOW;

			// 0x100241bf -- the four stores are ALL at the end, after the
			// last load of the source position, and they come out in the
			// order x, rhw, z, y.  That is why the three results go
			// through named floats: writing pWalk->m_vPos.x before the y
			// row would feed the NEW x into it, and a `vScreen` struct
			// assigned home in one go would be `rep movsl` (which is what
			// big6d910.cpp's copy of this block emits and this one does
			// not).
			pWalk->m_vPos.x = fSx;
			pWalk->m_vPos.y = fSy;
			pWalk->m_vPos.z = fSz;
			pWalk->m_fRHW   = fOOW;
			pWalk++;
		}

		// 0x100241d4 -- straight to the area sum; the screen projection
		// below is the clipped path's, and this path has already done it.
		goto L_Area;
	}

	// ------------------------------------------------------------------ //
	//  0x100241e0 -- the clipped path's pre-transform, world -> camera.
	//  `test %ecx,%ecx ; je` over a `dec %esi ; jne` loop, exactly
	//  big572b4.cpp's 0x1005739d and big6d910.cpp's 0x1006db49.
	// ------------------------------------------------------------------ //
	pWalk = pVerts;
	nLeft = nVerts;

	while (nLeft--)
	{
		sub_1000d9b0(pWalk, g_1008b880);
		pWalk++;
	}

	// -- 0x1002420a: the RE-READ.  0x1000d9b0 does not write the global
	//    (it tail-calls 0x1000a770, a pure matrix-vector multiply), so the
	//    value cannot have changed and all six planes below are live code.
	//    When the loop above did not run, icl's `je 0x1002420f` lands
	//    directly on the store at 0x1002420f with the FIRST read still in
	//    %eax -- a cross-jump, not a third read.
	if (!sub_1000f4b0(g_1008b874, &pVerts, &nVerts))
		goto L_Done;

	// ------------------------------------------------------------------ //
	//  0x1002544b -- camera space -> screen, one call per vertex, the same
	//  `test ; je` over `dec ; jne` as the pre-transform.  0x10014580 is
	//  the screen projection through the second argument's +0x31c..+0x330
	//  and it is what writes +0x0c.  Reached ONLY by the clipped path: the
	//  no-clip path did the whole projective transform itself and jumped
	//  over this loop.
	// ------------------------------------------------------------------ //
	pWalk = pVerts;
	nLeft = nVerts;

	while (nLeft--)
	{
		sub_10014580(pWalk, g_1008b880);
		pWalk++;
	}

L_Area:
	// ------------------------------------------------------------------ //
	//  0x10025488 -- THE FAN AREA SUM.  The accumulator is a DOUBLE:
	//  `flds 0x100a5d3c ; fstpl -0x70(%ebp)` initialises it from the float
	//  literal 0.0f (0x100a5d3c was dumped and is 0x00000000), and every
	//  later touch is `faddl`/`fstpl`/`fldl`/`fmull`.
	//
	//  `i` is uint16: `movzwl %ax,%eax` / `movzwl %cx,%ecx` on the index
	//  and `movzwl %dx,%edx` before the SIGNED `cmp ; jl`, three of the
	//  five movzwl in the function.  The bound `nVerts - 2` is RELOADED
	//  from -0x984 on every iteration (0x100254d8), so it is written out
	//  in the condition and not hoisted into a local.  pVerts is loaded
	//  THREE separate times per iteration (0x100254a7, 0x100254b2,
	//  0x100254c6) -- three source subscripts, no CSE through memory.
	//
	//  0x1002564c is the /Qip register entry of 0x10025640; the three
	//  arguments arrive in %eax, %edx, %ecx with `add $0xfffffff0,%esp`
	//  reserving the outgoing area, which decodes as an
	//  ordinary call.  Written as a plain call to 0x10025640, so the
	//  acceptance test will show one shipped-only 0x1002564c and one
	//  ours-only 0x10025640: the same function, four bytes apart.
	// ------------------------------------------------------------------ //
	fTotal = 0.0f;

	for (i = 0; i < nVerts - 2; i++)
		fTotal += sub_10025640(pVerts, &pVerts[i + 1], &pVerts[i + 2]);

	// ------------------------------------------------------------------ //
	//  0x100254f0 -- a two-pass surface draws every triangle twice, so it
	//  covers twice the area.  `mov 0x28(%eax),%eax ; mov 0x38(%eax),%eax
	//  ; test $0x80,%eax` -- big124a4.cpp's Ctx124a4::m_nFlags38, tested
	//  there for 0x100000 and here for 0x80.
	//
	//  0x10025508 -- `flds 0.5f ; fmull fTotal ; fadds 0x1008be30 ;
	//  fstps 0x1008be30`: the 0.5f and the double multiply come FIRST and
	//  the global is the SECOND operand of the add, which is the source's
	//  own left-to-right order.  0x100a5d40 was dumped and is 0x3f000000.
	// ------------------------------------------------------------------ //
	if (pObj->m_pCtx28->m_nFlags38 & 0x80)
		fTotal += fTotal;

	g_1008be30 = 0.5f * fTotal + g_1008be30;

	// -- 0x10025523: hand the object to the deferred list so that the real
	//    draw pass still gets it.  The shipped call reaches 0x10024024,
	//    the /Qip REGISTER entry of 0x10024020, with pObj in %eax; same
	//    translation unit, same function, and the acceptance test will
	//    show it as one shipped-only 0x10024024 and one ours-only
	//    0x10024020.
	sub_10024020(pObj);

L_Done:
	// -- 0x1002552b: the ONE epilogue.  Every early exit -- the six
	//    `nInside == 0` rejects and the six rotated classify guards --
	//    cross-jumps here rather than duplicating it.
	;
}


// *********************************************************************** //
//
//  REGION MAP -- what each region does and what pins it
//
//  0x10024090..0x100240f5   the gather.  Positions only, three separate
//      dereferences of pSrc->m_pPos00 per vertex, uint16 down-counter.
//      PINNED BY: the three `mov (%edx),%ecx` reloads, the 0x18 stride,
//      the single `movzwl 0x50(%edx)`.  Call-free.
//
//  0x100240f6..0x100241d8   the no-clip fast path: the inlined 4x4
//      projective transform.  PINNED BY: the sixteen absolute operands
//      0x1008ba5c..0x1008ba98 = 0x1008b880 + 0x1dc..0x218, which is the
//      block big6d910.cpp, big124a4.cpp and 0x1000a8a0 already decoded
//      instruction for instruction; `fdivrs 0x100a5d38` = 1.0f (dumped).
//      Call-free.
//
//  0x100241e0..0x10024227   the clipped path's pre-transform (four calls'
//      worth of loop, one call site) and the flag re-read.  WITNESSED by
//      the acceptance test: 0x1000d9b0.
//
//  0x10024228..0x10025449   the clipper, 4.6 KB, six planes, CALL-FREE.
//      PINNED BY: the block-by-block diff against big572b4.cpp (same
//      vertex, same planes, same order, same constants -- the twelve
//      epsilon addresses were all dumped), the frame decomposition
//      (six 0xdc blocks = 50 flags + 5 conversion slots, thirty 8-byte
//      float temps = 6 x 5), and the census rows that only six planes of
//      five byte-lerps and one texture pair can produce.
//      THIS IS THE REGION THE ACCEPTANCE TEST CANNOT SEE AT ALL and it is
//      83% of the function.
//
//  0x1002544b..0x10025487   the screen projection loop.  WITNESSED:
//      0x10014580.
//
//  0x10025488..0x100254ee   the fan-area sum.  WITNESSED: 0x1002564c.
//      Its shape is pinned by the double accumulator (fstpl/fldl/faddl/
//      fmull on -0x70), the three pVerts reloads and the two `imul $0x20`.
//
//  0x100254f0..0x10025539   the two-pass doubling, the 0.5f scale, the
//      meter update and the hand-off.  WITNESSED: 0x10024024.  The meter
//      is named outright by "Tri area drawn: %.3f" at 0x100a65b0.
//
//  0x1002553a..0x1002563f   the six sunk epsilon arms.  Call-free, but
//      each one's `jb` target lands in the middle of a plane's divide, so
//      they are pinned by the plane they belong to.
//
//  NO REGION IS UNDECODED.
//
// *********************************************************************** //
//
//  COIN-FLIPS -- each with both exact spellings, so one build settles it.
//
//  (1) THE GATHER LOOP'S FORM.  Written as an explicit uint16 down-count:
//          nGather = pObj->m_nCount50;
//          nVerts  = nGather;
//          while (nGather > 0) { ... ; pSrc++; pWalk++; nGather--; }
//      The alternative is the forward-indexed form with icl reversing the
//      induction variable:
//          for (i = 0; i < nVerts; i++)
//          {
//              aVerts[i].m_vPos.x = pObj->m_aSrc54[i].m_pPos00->x;
//              aVerts[i].m_vPos.y = pObj->m_aSrc54[i].m_pPos00->y;
//              aVerts[i].m_vPos.z = pObj->m_aSrc54[i].m_pPos00->z;
//          }
//      with the SAME `uint16 i` the area loop uses.  The written form is
//      the one the bytes argue for -- the counter is truncated to sixteen
//      bits on EVERY iteration (`mov %ebx,%ecx ; movzwl %cx,%ecx` at
//      0x100240ea), which a compiler-invented reversed counter would not
//      need -- but the alternative would explain why both of this
//      function's loop counters are uint16 while the planes' i/j are
//      int32, so it is worth one build.
//
//  (2) THE WHOLE-VERTEX COPY.  Written as the struct assignment
//          *pOut = *pPrev;
//      The shipped copy is EIGHT dword `mov`s (0x100242fb..0x10024336 and
//      its five siblings), NOT `rep movsl $8` -- and `rep movsl $8` is
//      exactly what this same source line emits in big572b4.cpp
//      (0x100574cd), same vertex, same /O2.  So either the source is not
//      a struct assignment or something else about the TU differs.  The
//      alternative spelling to test is
//          memcpy(pOut, pPrev, sizeof(RVert24090));
//      which icl expands inline as dword moves for a constant size.  This
//      is the one place where the written form is known NOT to reproduce
//      the shipped instruction selection; it is structurally identical
//      (all 32 bytes, same direction) either way.
//
//  (3) THE METER UPDATE.  Written
//          g_1008be30 = 0.5f * fTotal + g_1008be30;
//      because the shipped order is `flds 0.5f ; fmull fTotal ;
//      fadds 0x1008be30 ; fstps 0x1008be30` -- the global is the SECOND
//      operand of the add.  The alternative is
//          g_1008be30 += 0.5f * fTotal;
//      which normally puts `flds 0x1008be30` first.
//
//  (4) THE INLINED TRANSFORM'S THREE RESULTS.  Written as three named
//      floats fSx/fSy/fSz stored home at the end.  The alternative is
//          Vec24090 vScreen;
//          vScreen.x = ...; vScreen.y = ...; vScreen.z = ...;
//          pWalk->m_vPos.x = vScreen.x;
//          pWalk->m_vPos.y = vScreen.y;
//          pWalk->m_vPos.z = vScreen.z;
//      (big6d910.cpp's `pWalk->m_vPos = vScreen` is NOT a candidate: that
//      is `rep movsl $3` and the shipped bytes are four `fstps`.)
//
//  (5) THE FIRST FLAG READ.  Written as two reads of the global:
//          if (!g_1008b874) { ... }        // 0x10024102
//          ...
//          nClipFlags = g_1008b874;        // 0x1002420a
//      The alternative is big6d910.cpp's
//          nFlags = g_1008b874;
//          if (!nFlags) { ... }
//          ...
//          nFlags = g_1008b874;
//          nClipFlags = nFlags;
//      which needs a store the shipped bytes do not have at 0x10024107.
//
//  (6) THE SIX REJECTS.  Written `goto L_Done;`.  The alternative is a
//      bare `return;` in each plane, which at /O2 duplicates the four-
//      instruction register restore six times; the shipped code has ONE
//      epilogue and six `je 0x1002552b`, so the goto is the written form.
//
//  (7) pOutStart.  Written only inside the macro (`pOutStart = pOut;`).
//      The alternative is big6d910.cpp's outer pair
//          pOut = g_1008b87c;  pOutStart = g_1008b87c;
//      with the macro's assignment removed.  The written form matches the
//      byte count: there is exactly ONE store to -0x70 between 0x10024228
//      and 0x10024527, hoisted to 0x10024236.
//
//  (8) THE AREA HELPER'S RETURN TYPE.  Declared `float`.  `double` gives
//      identical bytes at this call site (the value is left in %st(0)
//      either way); float is chosen because 0x1002564c's own arithmetic
//      is entirely `flds`/`fmuls`.  The DOUBLE is fTotal, and that one is
//      pinned by `fstpl`/`fldl`/`faddl`/`fmull` on -0x70.
//
//  (9) NOT A SOURCE FLIP BUT A FLAG ONE, recorded because it decides
//      thirty call-free sites: every `(uint8)` conversion here is
//      `fstps <float slot> ; flds <same slot> ; fistpl <int slot>` with
//      no fnstcw/fldcw pair anywhere in the function.  That is /QIfist.
//      Under plain /O2 those thirty sites emit the control-word dance and
//      will differ in bytes; big6d910.cpp records the same finding.
//
// *********************************************************************** //


// ======================================================================= //
//  0x10025640 .. 0x10025674 (stack entry 0x10025640, register entry
//  0x1002564c; decoded in big24020.cpp, FUNCTION C).  Written as ONE
//  alternating chain, in the order the six products are emitted:
//      a.x*b.y - a.y*b.x + a.y*c.x - a.x*c.y + c.y*b.x - c.x*b.y
//  Measured: the three-pair grouping big24020.cpp had,
//      (b.y*a.x - a.y*b.x) + (a.y*c.x - a.x*c.y) + (c.y*b.x - c.x*b.y),
//  is re-associated by icl (the pairs come out C/A last-first, 30 bytes
//  off); the flat chain is identical whichever way round each product's
//  operands are written.
// ======================================================================= //
extern "C" float sub_10025640(const RVert24090* pA, const RVert24090* pB,
                              const RVert24090* pC)
{
	return pA->m_vPos.x*pB->m_vPos.y - pA->m_vPos.y*pB->m_vPos.x
	     + pA->m_vPos.y*pC->m_vPos.x - pA->m_vPos.x*pC->m_vPos.y
	     + pC->m_vPos.y*pB->m_vPos.x - pC->m_vPos.x*pB->m_vPos.y;
}
