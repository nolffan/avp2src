// *********************************************************************** //
//
//  w27_10056ab0_osqipni.cpp -- d3d.ren: DrawPolyMgr (0x1008f620), the
//  shipped /Os /Qip TU 0x10056ab0 .. 0x1005919d in address order:
//
//    0x10056ab0/ab4  queue a poly for a later pass
//    0x10056c60      the manager's flush
//    0x100572b0/2b4  the untextured fallback draw      (big572b4.cpp)
//    0x100589c0/9c4  draw a poly's first pass
//    0x10058e60/e64  DrawPolyAdditionalPass
//
//  Merged from w16_10056ab4_osqipni.cpp, w7_10056c60_os.cpp
//  and big572b4.cpp (their notes follow, unchanged).  Every drawer is a
//  /Qip two-entry pair in the shipped bytes -- a `mov 0x4(%esp),%eax`
//  shim at 0x10056ab0 / 0x100572b0 / 0x100589c0 / 0x10058e60 -- which icl
//  emits only for an external function with a caller in its own /Qip TU,
//  and the flush reaches all three through their register entries; so
//  the members are named for their stack entries (the byte comparison places the
//  `$` body 4 bytes above).  Flags /Os /Qip /Qip_no_inlining, the
//  measurement below.
//
// *********************************************************************** //
// *********************************************************************** //
//
//  w16_10056ab4_osqipni.cpp -- d3d.ren, written for the link.
//  DrawPolyMgr's per-poly pass drawers.
//
//  0x10056ab0  queue a poly for a later pass (4-byte stack entry + the
//              413-byte register entry 0x10056ab4 the enumeration lists)
//  0x100589c4  draw a poly's first pass (1177 bytes)
//  0x10058e64  DrawPolyMgr::DrawPolyAdditionalPass (825 bytes)
//
//  0x100589c4 and 0x10058e64 are what 0x10056c60 (w7_10056c60_os.cpp,
//  the manager's flush) calls for pass 0 and for later passes; both call
//  0x10056ab4, which no file defined, so it is written here too rather
//  than leave the link a new unresolved name.  It is named for its STACK
//  entry, sub_10056ab0: icl emits the `mov 0x4(%esp),%eax` shim as
//  `sub_10056ab0` and the body as `sub_10056ab0$`, which the byte comparison places
//  at 0x10056ab0 / 0x10056ab4 -- exactly the shipped pair.
//
//  FLAGS: /Os /Qip /Qip_no_inlining (the `_osqipni` convention in
//  build/engine.sh, added for this file).  /Os: `push %edi` pairs for the
//  dead frame slot, `xor %al,%al ; inc %al` for TRUE, 0x90 padding.
//  /Qip: all three shipped functions are two-entry pairs (a shim at
//  0x10056ab0 / 0x100589c0 / 0x10058e60 in front of each register entry),
//  which is what icl emits for an EXTERNAL __thiscall member with a caller
//  in its own /Qip TU, and 0x100589c4 carries CMoArray<>::
//  InternalNiceSetSize expanded (the vertex array at +0x7bc grown by 0x100
//  through g_DefAlloc).  /Qip_no_inlining: plain /Os /Qip ALSO expands
//  0x10056ab4 into both callers (1472 / 1256 bytes); with it the queue
//  stays a call and the template still expands.
//
//  OUTCOME, all NEAR:
//    * 0x10056ab0 (the shim) byte-identical; 0x10056ab4 413 of 413 bytes
//      with ONE byte different -- `lea 0x8(%ecx,%edx,1),%ebx` where shipped
//      has `lea 0x8(%edx,%ecx,1)`, the bucket address's base and index
//      swapped (not source-reachable: tried a pass pointer, `n + array`,
//      and moving the table call).  Also: `array + n`,
//      `&array[0] + n`, a byte-offset cast, `(pSet->m_Passes08 + iPass)`,
//      the buckets at +0x08 of a pass based at +0x00 (so the 8 joins the
//      bucket term), pSet read after the table call (36 off), int bucket
//      or pass indices (330+ off) -- the base/index pair never moves.  What got it there: dl_Insert's store
//      order (next->prev before prev->next), the node from
//      ObjectBank<>::Allocate (object_bank.h -- its placement-new test is
//      the second `je`), and the constructor's chained self-links.
//    * 0x10058e64 825 of 825 bytes, same frame; the residue is register
//      choice in the first loop's pointer step, the fastcall fog call's
//      argument loads, and the clip-failure `return` trampoline, which
//      shipped places straight after the first `ret` and ours at the end
//      (a 2-byte `jmp` that shifts everything behind it).  Slot order
//      follows the order of the first assignments (stride, colour fn, fog
//      fn), and the else-arm loop needs its argument pointer in a local
//      (`pS`) to get shipped's two induction pointers, not three.
//    * 0x100589c4 1175 against 1177: the same residue plus the start
//      (the vertex count staged through %ecx in shipped).
//  The `return` trampoline in both is the clip failure
//  written as the ELSE of `if (clip(...)) { ... } else return;` -- with
//  the early `if (!clip) return;` icl sinks it to the end of the function,
//  as the else of the block it is hosted straight after the first `ret`,
//  as shipped.  0x10058e64: 587 -> 575, and with a one-site volatile
//  reload of pOut for the fog call's second argument (shipped loads the
//  address-taken pOut twice there, `mov -0x28(%ebp),%eax ; mov
//  -0x28(%ebp),%ecx ; lea 0x14(%eax),%edx`, where icl otherwise reuses
//  one load) the function is size-exact at 825 with 132 bytes differing:
//  the first loop's pOut is carried in %edi across the back edge in
//  shipped (`mov -0x1c,%edi ; add -0x28(%ebp),%edi ; mov %edi,-0x28`)
//  and reloaded at the top in ours.  Loop forms (for-increment, comma,
//  a vertex-pointer local) and the increment's spelling (`nStride +
//  pOut`, `&pOut[nStride]`, integer casts) all build the same.
//  0x100589c4: 1042 -> 1039 (1172 bytes; the same reload costs it, so it
//  is not applied there; a count local `nVerts` gives 1168).
//  The dispatch tables it reads (g_100896f0, g_10089728, g_10089738,
//  g_10089744, g_1008974c, g_1008975c, g_1008977c, g_10089788, all in
//  .data) are new names for d3ddata.cpp to define.
//
// *********************************************************************** //

// *********************************************************************** //
//
//  MODULE:   BIG_572B4 (.CPP)
//
//  PURPOSE:  One function -- 0x100572b4, the UNTEXTURED FALLBACK DRAW.
//
//  ===================== THE EXTENT, VERIFIED ============================ //
//
//  The row says 5,904 bytes, 0x100572b4 .. 0x100589c4.  That is RIGHT to
//  within the six bytes of padding and dead stub at the end; the real body
//  is 0x100572b4 .. 0x100589bd INCLUSIVE (5,898 bytes) and the extent
//  checks all pass:
//
//    * 0x100572b4 IS a function start.  A whole-image scan for `e8 rel32`
//      landing on it finds exactly one caller, 0x100571b8
//      (`mov -0x34(%ebp),%eax ; mov %esi,%ecx ; push %eax ; call`), so
//      it is reached "via call".  The four bytes at
//      0x100572b0 (`mov 0x4(%esp),%eax`) are the /Qip STACK ENTRY of the
//      same function -- icl's /Qip two-entry pattern -- and nothing in
//      .text calls or references 0x100572b0 at all.  The same 4 bytes sit
//      dead at 0x100589c0 in front of the NEXT function's register entry
//      at 0x100589c4, so the pattern repeats and is not an accident.
//    * exactly ONE `ret` in the range: `c2 04 00` at 0x100588d3, reached
//      through the single epilogue
//      `pop %ebx ; pop %esi ; pop %edi ; mov %ebp,%esp ; pop %ebp` at
//      0x100588cd.  `ret $0x4` with the one argument read from 0x8(%ebp)
//      makes it __stdcall.
//    * there is no second `push %ebp ; mov %esp,%ebp` anywhere inside.
//    * the 232 bytes AFTER the `ret` are seven /O2-sunk cold blocks --
//      six clip-epsilon arms and one `return` trampoline -- and every one
//      of them jumps BACK into the body: 0x100588e1 -> 0x1005750e,
//      0x10058904 -> 0x10057858, 0x10058929 -> 0x10057bb9,
//      0x1005894e -> 0x10057f11, 0x10058976 -> 0x1005828f,
//      0x1005899e -> 0x10058601 and 0x100589b9 -> 0x100588cd.  A branch
//      pair like that cannot cross a function boundary.
//    * nothing branches PAST 0x100589bd; 0x100589be/bf are 0x90 padding.
//    * 0x100589c4, which the enumeration also lists as a 553-byte
//      function start, really is one: it has its own prologue
//      (`push %ebp ; mov %esp,%ebp ; mov $0x1458,%eax ; call _chkstk`),
//      it is called from 0x10057150 by the same caller, and it is a
//      __thiscall (`mov %ecx,-0x18(%ebp)`).  Both boundaries are real.
//
//  ================= WHY THE CALL SEQUENCE IS NEARLY USELESS ============= //
//
//      0x100572b4..0x100589c4:
//      5,904 bytes, 7 calls (6 direct, 1 indirect), 843 bytes per call
//
//  Six direct calls to six distinct callees across 5.9 KB is the weakest
//  band there is.  The seven call sites are 0x100572bc (_chkstk),
//  0x100572c8, 0x100572f4, 0x10057303, 0x100573a6, 0x1005886f and the
//  indirect 0x100588b6 -- five bunched in the first 250 bytes and two in
//  the last 100.  **The 5,316 bytes from 0x100573ab to 0x1005886e -- 90.0%
//  of the function -- contain not one call of any kind**, and 5,178 of
//  those (87.7%) are the six inlined clip planes.  The call-sequence test can say nothing
//  whatever about that region.  What pins it is below, and it is not the
//  call sequence.
//
//  ================= WHAT PINS THE CALL-FREE 87.7% ====================== //
//
//  THE SIX PLANES ARE THE SAME INSTRUCTIONS AS 0x100124a4's.  0x1000f4b0
//  is inlined here with all six of its plane bodies inlined too, exactly
//  as big124a4.cpp records for 0x10012be9..0x100140c9.  Diffing the two,
//  plane against plane, mnemonic stream against mnemonic stream (frame
//  displacements, literal-pool addresses and branch targets normalised
//  away):
//
//      plane          mine            0x100124a4      differing
//      bit 0x01 near  0x10057407      0x10012be6      206 vs 212 insns
//      bit 0x04 left  0x1005772e      0x10012f5b      216 vs 214, 6
//      bit 0x08 top   0x10057a85      0x100132df      212 vs 210, 6
//      bit 0x10 right 0x10057de0      0x1001365b      212 vs 210, 6
//      bit 0x20 botm  0x10058159      0x100139d7      216 vs 214, 6
//      bit 0x02 far   0x100584da      0x10013d5b      202 vs 203, 15
//
//  For the four middle planes the ENTIRE 210-odd-instruction body differs
//  in exactly SIX slots, and all six are one idiom: this function emits
//  `mov 0x1008be44,%edx ; inc %edx ; mov %edx,0x1008be44` where
//  0x100124a4 emits `incl 0x1008be44`, plus the `lea` of the flag array
//  moving one slot in the schedule.  Planes 1 and 6 differ a little more
//  only because their slice boundaries fall inside the gate and the
//  write-back.  Two independently compiled copies of the same 1,264
//  instructions is not something a wrong decode produces.
//
//  THE FRAME DECOMPOSES EXACTLY.  `mov $0x1700,%eax ; call 0x100805a0`
//  is _chkstk for 5,888 bytes, and every byte of it is named:
//
//      -0x1700 .. -0x16e0   0x20  alignment slack, never referenced
//      -0x16e0 .. -0x15f4   0xec  THIRTY `float` temps, 8 apart:
//                                 6 planes x { fRed fGreen fBlue fAlpha
//                                 fSpec }.  Each is `fstps` then `flds`
//                                 then `fistpl`, so each is 4 bytes in an
//                                 8-byte slot.
//      -0x15f4              0x04  pData      (out-param of 0x1000a700)
//      -0x15f0              0x04  nVerts     (out-param of 0x1000a700)
//      -0x15ec              0x04  pVerts
//      -0x15e8 .. -0x05e8 0x1000  RVert aVerts[128]  -- 128 * 0x20 EXACTLY,
//                                 and 128 is the bound 0x1000a740 enforces
//                                 (d3dbatchd.cpp: `cmpl $0x80,arg ; jg`).
//      -0x05e8 .. -0x00c0  0x528  SIX plane blocks of 0xdc each:
//                                 0xc8 = int32 bInside[50] followed by
//                                 0x14 = five int temps for the five
//                                 (uint8)float conversions.  The six
//                                 `lea` bases are -0x5e8, -0x50c, -0x430,
//                                 -0x354, -0x278, -0x19c -- 0xdc apart --
//                                 and the six five-int groups start at
//                                 -0x520, -0x444, -0x368, -0x28c, -0x1b0,
//                                 -0xd4, each exactly 200 bytes into its
//                                 block.  -0x5e8 + 6*0xdc = -0xc0.
//      -0x00c0              0x04  the shared int the fildl reads (all 60
//                                 `movzbl`->`fildl` pairs use this one
//                                 slot and no other)
//      -0x00b8 .. -0x0064         pOut / pStart / pPrev / i / j / nInside
//                                 / the six 64-bit fT spills
//
//  THE INSTRUCTION CENSUS LEAVES NO ROOM FOR ANYTHING ELSE.  1,470
//  instructions in the whole function:
//
//      mov 547   flds 94   movzbl 60   fildl 60   add 60   fstps 56
//      fmull 54  je 42     sub 36      cmp 33     faddp 32  fistpl 30
//      test 29   fsubs 28  fadds 28    lea 19     sahf 18   fnstsw 18
//      shl 17    push 16   fstp 16     xor 15     inc 13    fstpl 12
//      fcomp 12  jb 10     jne 9       fxch 8     fchs 8    rep 7
//      jmp 7     jle 7     jl 7        jbe 7      call 7    sar 6
//      fsub 6    fdivp 6   fcomps 6    pop 4      movl 4    nop 2
//      fsubrs 2  fsubrp 2  fsts 2      fld 2      dec 2     ret 1
//      jae 1     and 1
//
//  Every one of those totals is a multiple of six, or six plus a small
//  head/tail remainder, and each falls out of the six-plane structure:
//
//    * 60 `movzbl` + 60 `fildl` = 6 planes x 5 channels x 2 loads.  Each
//      colour lerp is `movzbl m,%edx ; movzbl m,%eax ; sub ; mov %edx,T ;
//      fildl T ; fmull fT ; mov %eax,T ; fildl T ; faddp`.
//    * 30 `fistpl` = 6 x 5, the (uint8) conversion of each channel; the
//      30 `fstps` to -0x15f8.. are their 6 x 5 float temps.
//    * 6 `fdivp` = one divide per plane; 6 `fcomps` = one classify
//      compare per plane; 12 `fcomp` = the two epsilon compares per plane
//      (one in line, one in the sunk block); 12 `fstpl` = the fT store on
//      both sides of each of those six branches.
//    * 8 `fchs` = the two classify negations (planes 2 and 5) + the six
//      `-(...)` numerators; 8 `fxch` = the six divides that need the
//      operands swapped plus planes 2 and 5's x/y negation.
//    * 2 `fsts`+4 `fld`/`fsub`-pairs are the four planes that write the
//      interpolated coordinate to two fields (3 and 4 with `fsts`, 2 and
//      5 with `fld %st(0) ; fchs ; fxch`).
//    * 7 `rep movsl`: ONE in the vertex build (`$3`, the position copy)
//      and SIX in the plane rebuilds (`$8`, the 32-byte `*pOut=*pPrev`).
//    * 17 `shl $5` / 6 `sar $5`: 0x20-byte vertex indexing, 2 per plane
//      + the setup.
//    * 7 `call` = the 5 in the head + 1 in the tail + 1 indirect.
//
//  There is no residue.  Nothing in the census is unaccounted for by the
//  structure written below.
//
//  THE SIX EPSILON PAIRS.  0x100aa6d0/6d8/6e0/6e8/6f0/6f8 are six private
//  copies of -0.00001f (0xb727c5ac) with 1.0f padding after each, and
//  0x100aa700..0x100aa72c are six private (+0.00001f, 0.0f) pairs, one per
//  sunk block.  Dumped from the image, not assumed.  The +1e-5f copies are
//  consumed in REVERSE plane order (plane 1 uses 0x100aa728/72c, plane 6
//  uses 0x100aa700/704) because the sunk blocks are emitted last-first.
//
//  ---------------------------------------------------------------------- //
//  WHAT IT IS.  The caller, 0x10056c60, is the material/pass walker: at
//  0x10057192 it prints "Material '%s', pass %d, stage %d: TextureSrcInitFn
//  failed." and then, at 0x100571ab, `if (nPass == 0) sub_100572b4(pObj)`.
//  So this is the FALLBACK DRAW taken when a material's first pass cannot
//  bind its texture: unbind stage 0 (`sub_100232b0(0)`, which d3dbatchd.cpp
//  decodes as "unbinds a texture stage"), then walk the object's geometry
//  ring and push every piece through the transform, the clipper and
//  DrawPrimitive with no texture and no texture coordinates ever written.
//  That is why the u/v fields of aVerts are never initialised here and why
//  the six-plane clipper still interpolates them.
//
//  ---------------------------------------------------------------------- //
//  COMPILER: Intel C/C++ 4.0, **/O2**, same verdict and same evidence as
//  big124a4.cpp for the same code:
//    * `&p->m` is `mov $K,%reg ; add %p,%reg` -- 0x100574bb
//      `mov $0x20,%eax ; add %edi,%eax`, and the identical idiom at
//      0x100576d1, 0x100577fb, 0x10057b53, ... one per plane -- not
//      `lea 0x20(%edi),%eax`.  (`lea` appears only for LOCALS, where both
//      flags use it.)
//    * the COM call at 0x1005887d builds its outgoing area with
//      `add $0xffffffe8,%esp` then five `mov`s.
//    * cold-block sinking BELOW the epilogue, seven blocks of it.
//  The one /Os-looking thing -- every exit reaching ONE epilogue -- is the
//  source shape, not the flag: a `return` and a `continue` out of a loop
//  nest have one tail by construction.
//  The direct calls DO use `push` for their arguments, but so does
//  0x100124a4's identical `sub_1000d9b0` loop at 0x10012b98, and that TU
//  is /O2, so `push` here is not the /Os row.
//  MEASURED AGAINST THAT: the stack entry at 0x100572b0 exists only
//  because this function shares a /Qip TU with its caller 0x10056c60,
//  which is /Os (measured, for it and for the drawers).  This body
//  compiled alone: /O2 8065 bytes (7516 differ), /Os 6608 (5870 differ);
//  in this /Os /Qip /Qip_no_inlining file 6624 (5930), shim identical.
//  /Os is the closer of the two by 1457 bytes, so it is built /Os here.
//
//  ---------------------------------------------------------------------- //
//  REGION MAP.
//
//  0x100572b4..0x100572de  PROLOGUE, _chkstk 0x1700, `sub_100232b0(0)`,
//      and the empty-ring test `if (pHead->m_pPrev == pHead) return;`.
//
//  0x100572e1..0x1005730d  per element: `sub_1000a700(pElem, &pData,
//      &nVerts)` (d3dbatchd.cpp: "where a piece of geometry's index data
//      lives and how many"), then the 128-vertex guard `sub_1000a740`.
//      The failure arm returns through the trampoline at 0x100589b9.
//
//  0x10057313..0x1005737c  THE VERTEX BUILD.  `pVerts = aVerts`, then
//      nVerts iterations of `rep movsl $3` from `*(void**)(pData + i*0x18)`
//      -- a 0x18-byte record whose first field is a pointer to a 12-byte
//      position -- into aVerts[i], followed by one dword store of the
//      ELEMENT POINTER to aVerts[i]+0x10.  See coin-flip (1).
//
//  0x1005737c..0x100573ba  `g_1008b874 = 0x3f`, then nVerts calls to
//      `sub_1000d9b0(pWalk, g_1008b880)` (d3dlow.cpp: transform by the 3x4
//      at +0x15c), then the flag word is RE-READ from the global.  This is
//      big16324.cpp's 0x100169f0..0x10016a2f verbatim, including the dead
//      `mov $0x3f,%edx`.  0x1000d9b0 does not write the global -- it tail-
//      calls 0x1000a770, a pure matrix-vector multiply -- so the value is
//      still 0x3f and ALL SIX PLANES RUN.  That is the one substantive
//      difference from 0x100124a4, where the same six blocks are dead.
//
//  0x100573ba..0x10057407  `if (g_1008d7a0) { nClipFlags &= 1;
//      if (!nClipFlags) goto draw; }` then `pOut = g_1008b87c`.
//      0x1008d7a0 is UseD3DClip; the shape is byte-identical to
//      0x10012bb0..0x10012bc6.
//
//  0x10057407..0x10058841  0x1000f4b0 INLINED, six planes.  See above.
//
//  0x10058841..0x100588b9  THE DRAW.  pVerts/nVerts written back, nVerts
//      calls to `sub_10014580(pWalk, g_1008b880)` (the screen projection,
//      decoded in full at that address), then
//      DrawPrimitive(6, 0x1c4, pVerts, nVerts, 0) through vtable +0x64.
//      6 is D3DPT_TRIANGLEFAN and 0x1c4 is XYZRHW|DIFFUSE|SPECULAR|TEX1,
//      which is exactly the 32-byte vertex.
//
//  0x100588b9..0x100588d6  the ring advance `pElem = pElem->m_pPrev` and
//      the single epilogue.
//
//  0x100588d6..0x100589bd  the seven sunk cold blocks.
//
//  NOTHING IS UNDECODED.  This file carries no undecoded-region banner
//  because there is no undecoded region: every instruction in
//  0x100572b4..0x100589bd is accounted for by a statement below.  The
//  coin-flips are at the bottom.
//
// *********************************************************************** //

#include "ltbasedefs.h"
#include "../../LT2/lithshared/stdlith/dynarray.h"
#include "../../LT2/lithshared/stdlith/object_bank.h"

// ======================================================================= //
//  THE TYPES.  The tags C1008f620, W7Poly56c60 and W7MatPass56c60 are the
//  ones w7_10056c60_os.cpp's calls are mangled with; the layouts here are
//  read off these three functions.
// ======================================================================= //

// One source vertex record of a world poly (big572b4.cpp's SrcRef572b4):
// a pointer to the position, then fields the vertex functions read.
struct SrcRec589c4                                      // 0x18 bytes
{
	LTVector*	m_pPos;                                 // +0x00
	uint8		m_pad_04[0x14];                         // +0x04
};

// The world poly.  Its first two dwords are the link 0x10056ab4 ties it
// into a bucket node with.
struct W7Poly56c60
{
	W7Poly56c60*	m_pPrev00;                          // +0x00
	W7Poly56c60*	m_pNext04;                          // +0x04
	uint32			m_nSaved08;                         // +0x08  first saved vert << 16 | count
	uint8			m_pad_0c[0x14 - 0x0c];              // +0x0c
	uint16			m_wClip14;                          // +0x14  low 6 bits: planes to clip
	uint8			m_pad_16[0x2c - 0x16];              // +0x16
	uint32			m_n2c;                              // +0x2c
	uint8			m_pad_30[0x4c - 0x30];              // +0x30
	SrcRec589c4*	m_pFixVerts4c;                      // +0x4c  T-junction-fixed verts
	uint16			m_nVerts50;                         // +0x50
	uint16			m_nFixVerts52;                      // +0x52
	SrcRec589c4		m_Verts54[1];                       // +0x54
};

// A saved (already transformed) vertex: position, rhw.
struct SavedVert589c4                                   // 0x10 bytes
{
	LTVector	m_Pos;                                  // +0x00
	uint32		m_n0c;                                  // +0x0c
};

struct W7MatStage56c60                                  // 0x24 bytes
{
	uint8	m_pad_00[0x18];                             // +0x00
	uint32	m_nVertFn18;                                // +0x18  g_100896f8
	uint32	m_nDrawFn1c;                                // +0x1c  g_10089728, and
	                                                    //   g_10089718 (0x10056c60)
	uint32	m_nInitFn20;                                // +0x20  g_1008977c
};

struct W7MatPass56c60                                   // 0x8c bytes
{
	uint32				m_bBlend00;                     // +0x00
	uint8				m_pad_04[0x0c - 0x04];          // +0x04
	uint32				m_bInit0c;                      // +0x0c
	uint32				m_nColorFn10;                   // +0x10  g_10089738
	uint32				m_nFogFn14;                     // +0x14  g_10089744
	uint32				m_nBucketFn18;                  // +0x18  g_100896f0
	uint32				m_nStages1c;                    // +0x1c
	W7MatStage56c60		m_Stages20[3];                  // +0x20
};

// One bucket list head / entry: {prev, next, data}.
struct Link56ab4
{
	Link56ab4*	m_pPrev;                                // +0x00
	Link56ab4*	m_pNext;                                // +0x04
	void*		m_pData;                                // +0x08
};

// A queued-poly node (0x10050d00's layout): the polys hang off the head
// at +0x00; +0x0c is the bucket key; two links at +0x10 and +0x1c tie the
// node into its bucket and into the pass's node list.
struct Node56ab4
{
	// 0x10050d00, the same constructor out of line (thiscall, returns
	// this); here it is expanded behind the placement new's null test.
	Node56ab4()
	{
		m_pPrev00 = m_pNext04 = (W7Poly56c60*)this;
		m_Link10.m_pPrev = m_Link10.m_pNext = &m_Link10;
		m_Link10.m_pData = this;
		m_Link1c.m_pPrev = m_Link1c.m_pNext = &m_Link1c;
		m_Link1c.m_pData = this;
	}

	W7Poly56c60*	m_pPrev00;                          // +0x00
	W7Poly56c60*	m_pNext04;                          // +0x04
	uint32			m_n08;                              // +0x08
	uint32			m_nKey0c;                           // +0x0c
	Link56ab4		m_Link10;                           // +0x10
	Link56ab4		m_Link1c;                           // +0x1c
};

struct NodePass56ab4                                    // 0x1dc bytes
{
	uint8			m_pad_000[8];
	Link56ab4		m_Buckets[32];                      // +0x008
	uint8			m_pad_180[0x188 - 0x180];           // +0x188
	Link56ab4		m_Nodes188;                         // +0x190
	uint8			m_pad_194[0x1dc - 0x194 - 8];           // +0x19c
};

struct NodeSet56ab4
{
	NodePass56ab4	m_Passes08[3];                      // +0x00
};

class C1008f620;

typedef void (C1008f620::*ColorFn589c4)(SrcRec589c4* pSrc, void* pVert);
typedef void (__fastcall* FogFn589c4)(void* pPos, void* pSpecular);
typedef void (C1008f620::*VertFn589c4)(SrcRec589c4* pSrc, void* pUV, uint32 n);
typedef int32 (__fastcall* ClipFn589c4)(uint32 nPlanes, uint8** ppVerts, int32* pnVerts);

struct Ctx589c4;
typedef void (C1008f620::*InitFn589c4)(Ctx589c4* pCtx);
typedef void (C1008f620::*DrawFn589c4)(W7Poly56c60* pPoly, uint32 iStage, uint32 nValue);
typedef void (* BucketFn56ab4)(W7Poly56c60* pPoly, uint32* pBucket, uint32* pKey);

// 0x100589c4's staging block: the poly, one value per stage filled by the
// stage init functions, the vertex count, the stride and the vertices.
struct Ctx589c4
{
	W7Poly56c60*	m_pPoly;                            // -0x143c
	uint32			m_nStage[3];                        // -0x1438
	int32			m_nVerts;                           // -0x142c
	uint32			m_nStride;                          // -0x1428
	uint8			m_Verts[0x1400];                    // -0x1424
};

// DrawPolyMgr.  It opens with a material of its own (the passes at +0x24
// and the node set at +0x1c8 are what 0x10056ab4 reads through `this`).
class C1008f620
{
public:
	void	sub_10056ab0(W7Poly56c60* pPoly, uint32 iPass);               // 0x10056ab0 / 0x10056ab4
	void	sub_10056c60();                                            // 0x10056c60
	void	sub_100572b0(struct Geom572b4* pGeom);                     // 0x100572b0 / 0x100572b4
	void	sub_100589c0(W7Poly56c60* pPoly, W7MatPass56c60* pPass,
	                     uint32 nNextPass);                            // 0x100589c0 / 0x100589c4
	void	sub_10058e60(W7Poly56c60* pPoly, W7MatPass56c60* pPass,
	                     uint32 nNextPass);                            // 0x10058e60 / 0x10058e64

	uint8					m_pad_000[0x24];            // +0x000
	W7MatPass56c60			m_Passes24[3];              // +0x024
	NodeSet56ab4*			m_pNodes1c8;                // +0x1c8
	uint8					m_pad_1cc[0x7b8 - 0x1cc];   // +0x1cc
	VertFn589c4				m_pVertFn7b8;               // +0x7b8
	CMoArray<SavedVert589c4> m_SavedVerts;              // +0x7bc
	uint32					m_nSaved7d0;                // +0x7d0
	ObjectBank<Node56ab4>	m_NodeBank;                 // +0x7d4  (its StructBank at +0x7d8)
};

// IDirect3DDevice7, under the tag and name big44ff4.cpp and others give
// 0x1008d988; only DrawPrimitive (+0x64) is used here.
struct D3DDevice7;
struct D3DDevice7Vtbl
{
	void*	m_Slots00[25];                                                  // +0x00
	long (__stdcall* DrawPrimitive)(D3DDevice7*, uint32 nType, uint32 nFVF,
	                                void* pVerts, uint32 nVerts, uint32 nFlags); // +0x64
};
struct D3DDevice7 { D3DDevice7Vtbl* lpVtbl; };

extern D3DDevice7*		g_pD3DDevice;                   // 0x1008d988
extern int32			g_CV_FixTJunc;                  // 0x1008cb84  FixTJunc
extern BucketFn56ab4	g_100896f0[2];                  // 0x100896f0
// The three member-function tables are declared as what they hold in the
// image -- one code address per entry -- because the generated d3ddata.cpp cannot
// define a pointer-to-member type; each use reads its entry as the member
// pointer it is (a single-inheritance member pointer is that one dword).
extern void*			g_10089728[4];                  // 0x10089728  DrawFn589c4
extern void*			g_10089738[3];                  // 0x10089738  ColorFn589c4
extern FogFn589c4*		g_10089744[2];                  // 0x10089744  -> 0x1008ce8c / 0x1008ce88
extern uint32			g_1008974c[4];                  // 0x1008974c  vertex stride by stage count
extern ClipFn589c4		g_1008975c[4];                  // 0x1008975c  clipper by stage count
extern void*			g_1008977c[3];                  // 0x1008977c  InitFn589c4
extern uint32			g_10089788[4];                  // 0x10089788  FVF by stage count

extern void sub_10021d10(const char* pszMsg, ...);      // 0x10021d10


// ======================================================================= //
//  0x10056ab0 -- the shim, then the 413-byte body at 0x10056ab4 (`ret $0x8`).  Queue
//  pPoly for pass iPass: 0x100896f0[pass.m_nBucketFn18] gives its bucket
//  (0..31) and key; the bucket's node list is searched for that key and,
//  if no node has it, a node is taken from the struct bank at +0x7d8,
//  constructed (0x10050d00's self-linked layout), keyed and tied onto the
//  bucket and onto the pass's node list.  The poly goes in after the
//  node's head.
// ======================================================================= //
inline void Insert56ab4(W7Poly56c60* pAfter, W7Poly56c60* pPoly)
{
	pPoly->m_pPrev00 = pAfter;
	pPoly->m_pNext04 = pAfter->m_pNext04;
	pPoly->m_pNext04->m_pPrev00 = pPoly;
	pPoly->m_pPrev00->m_pNext04 = pPoly;
}

inline void Insert56ab4(Link56ab4* pAfter, Link56ab4* pLink)
{
	pLink->m_pPrev = pAfter;
	pLink->m_pNext = pAfter->m_pNext;
	pLink->m_pNext->m_pPrev = pLink;
	pLink->m_pPrev->m_pNext = pLink;
}

void C1008f620::sub_10056ab0(W7Poly56c60* pPoly, uint32 iPass)
{
	uint32			nBucket;
	uint32			nKey;
	NodeSet56ab4*	pSet;
	Link56ab4*		pHead;
	Link56ab4*		pCur;
	Node56ab4*		pNode;

	pSet = m_pNodes1c8;
	g_100896f0[m_Passes24[iPass].m_nBucketFn18](pPoly, &nBucket, &nKey);

	pHead = &pSet->m_Passes08[iPass].m_Buckets[nBucket];
	for (pCur = pHead->m_pNext; pCur != pHead; pCur = pCur->m_pNext)
	{
		pNode = (Node56ab4*)pCur->m_pData;
		if (pNode->m_nKey0c == nKey)
		{
			Insert56ab4((W7Poly56c60*)pNode, pPoly);
			return;
		}
	}

	pNode = m_NodeBank.Allocate();
	if (!pNode)
		return;

	pNode->m_nKey0c = nKey;
	Insert56ab4(pHead, &pNode->m_Link10);
	Insert56ab4(&pSet->m_Passes08[iPass].m_Nodes188, &pNode->m_Link1c);
	Insert56ab4((W7Poly56c60*)pNode, pPoly);
}


// ======================================================================= //
//  The Direct3D device, the C COM interface (slots +0x50 SetRenderState,
//  +0x54 GetRenderState, +0x94 SetTextureStageState of IDirect3DDevice7),
//  under the tag d3ddata.cpp defines 0x1008d988 with.
// ======================================================================= //
struct D3DDevTSSR5C;
struct W7DevVtbl56c60
{
	void*	m_Slots00[20];                                              // +0x00
	long (__stdcall* SetRenderState)(D3DDevTSSR5C*, uint32 nState,
	                                 uint32 nValue);                    // +0x50
	long (__stdcall* GetRenderState)(D3DDevTSSR5C*, uint32 nState,
	                                 uint32* pValue);                   // +0x54
	void*	m_Slots58[15];                                              // +0x58
	long (__stdcall* SetTextureStageState)(D3DDevTSSR5C*, uint32 nStage,
	                                       uint32 nType, uint32 nValue); // +0x94
};
struct D3DDevTSSR5C { W7DevVtbl56c60* lpVtbl; };

extern D3DDevTSSR5C* g_1008d988;                                    // 0x1008d988


// ======================================================================= //
//  0x10056c60 -- 1,614 bytes (then `nop`, and 0x100572b0 is the next
//  function's /Qip stack entry).  DrawPolyMgr's flush -- the object is the
//  global at 0x1008f620 (d3dr5b.cpp's 0x10026500 calls it on it) and the
//  pass drawers it dispatches to are 0x100572b4 (big572b4.cpp, the
//  fallback), 0x100589c4 and 0x10058e64 (`DrawPolyMgr::DrawPoly
//  AdditionalPass`, d3dr5d.cpp):
//
//    * the frame code: m_768 = g_1008beb4->+0x0c (a word);
//    * ALPHABLENDENABLE / SRCBLEND / DESTBLEND are read into three locals
//      up front and written back at the end;
//    * the built-in material at +0x20.. is set up one of three ways, by
//      g_1008f600, else g_1008f5e0, else the default (flat constant
//      stores, grouped by value exactly as shipped);
//    * m_7d0 = 0, then for every node on the m_76c list, for every pass of
//      its material: stop at the first pass with an empty poly list; set
//      the pass's blend states (SRC/DEST only when blending), each
//      stage's COLOROP/ALPHAOP (and their two args unless the op is
//      DISABLE), choose the vertex-setup function(s) from the tables at
//      0x100896f8 / 0x10089708, then for each queued poly run every
//      stage's TextureSrcInitFn (the member-function table at
//      0x10089718) and draw it with 0x100589c4 (pass 0) or 0x10058e64,
//      or warn and fall back to 0x100572b4; finally unlink the poly's two
//      links (0x10050d00's node layout) and push it on the free list at
//      m_7f0.
//
//  The init-function call is `mov table(,%edx,4),%eax ; mov %esi,%ecx ;
//  call *%eax` with two stack arguments: a pointer-to-member-function
//  table, called through `this`.
//
//  NEAR (1626 against 1614 bytes): the control flow, every call and every
//  store are the shipped ones; the residual is register/slot allocation
//  (the default-material constants take %edx where the shipped code
//  reloads %eax, and the stage/offset induction variables sit in other
//  registers).  The failure arm is a `goto` to the unlink, as the shipped
//  block falls straight into it (the `break`-and-test spelling was 1652).
//  The calls to 0x100572b4, 0x100589c4 and 0x10058e64 are /Qip register
//  entries (`mov pGeom,%eax` as well as the push); in this file all three
//  are (1626 bytes, 693 differ, the same as alone at /Os -- the
//  residual is the allocation above).  The fields are read through
//  W7View56c60 because C1008f620 carries the drawers' layout.
//
//  Later (1624 bytes, 22 -> 13 mnemonic blocks): icl emits each run of
//  equal-value stores in its own order, and the reversed chains below put
//  three of the runs into ship's; the first-in-list pointer is read once
//  into pWalk and that local is what the TextureSrcInitFn calls take
//  (ship's -0x38 slot).  LEFT: the 4s take %edx where ship reloads %eax
//  (every order of the four default groups measured: the current one is
//  best), ship re-reads m_pFirst04 at the head of each walk loop where we
//  read it once after the init loop (a single loop with the iPass test
//  inside, an if/else nNextPass and `!iPass` do not move it), and the frame
//  is 0x58 against ship's 0x60.
// ======================================================================= //
struct W7Link56c60
{
	W7Link56c60*	m_pPrev;                            // +0x00
	W7Link56c60*	m_pNext;                            // +0x04
	void*			m_pData;                            // +0x08
};

// One queued poly: 0x10050d00's node (a head at +0x00 whose +0x04 is the
// first entry, then two links at +0x10 and +0x1c).
struct W7QNode56c60
{
	W7QNode56c60*	m_pFree00;                          // +0x00  free-list link
	W7QNode56c60*	m_pFirst04;                         // +0x04
	uint8			m_pad_08[0x10 - 0x08];              // +0x08
	W7Link56c60		m_Link10;                           // +0x10
	W7Link56c60		m_Link1c;                           // +0x1c
};

// One entry of a pass's poly list: its +0x04 is the next entry and its
// +0x08 the poly.
struct W7Entry56c60
{
	void*			m_p00;                              // +0x00
	W7Entry56c60*	m_pNext04;                          // +0x04
	W7QNode56c60*	m_pPoly08;                          // +0x08
};

struct W7StageStates56c60                               // 0x18 bytes
{
	uint32	m_nColorOp;                                 // +0x00
	uint32	m_nColorArg1;                               // +0x04
	uint32	m_nColorArg2;                               // +0x08
	uint32	m_nAlphaOp;                                 // +0x0c
	uint32	m_nAlphaArg1;                               // +0x10
	uint32	m_nAlphaArg2;                               // +0x14
};

struct W7NodePass56c60                                  // 0x1dc bytes
{
	uint8				m_pad_000[0x180];               // +0x000
	uint32				m_nSrcBlend180;                 // +0x180
	uint32				m_nDestBlend184;                // +0x184
	W7Entry56c60		m_List188;                      // +0x188  head
	W7StageStates56c60	m_Stages194[3];                 // +0x194
};

struct W7Node56c60
{
	struct W7Mat56c60*	m_pMat00;                       // +0x00
	W7Node56c60*		m_pNext04;                      // +0x04
	W7NodePass56c60		m_Passes08[1];                  // +0x08
};

struct W7Mat56c60
{
	char				m_szName00[0x20];               // +0x00
	uint32				m_nPasses20;                    // +0x20
	W7MatPass56c60		m_Passes24[1];                  // +0x24
};

struct W7FrameInfo56c60
{
	uint8	m_pad_00[0x0c];                             // +0x00
	uint16	m_wFrame0c;                                 // +0x0c
};

struct Geom572b4;

// DrawPolyMgr (C1008f620) as the flush reads it: the built-in material's
// fields as flat dwords, the node list, the vertex-function slots.  It is
// read through this view of `this` because the class above carries the
// drawers' layout (the CMoArray at +0x7bc, the ObjectBank at +0x7d4).
struct W7View56c60
{
	uint8		m_pad_000[0x20];                        // +0x000
	uint32		m_n20, m_n24;                           // +0x020
	uint8		m_pad_028[0x34 - 0x28];                 // +0x028
	uint32		m_n34, m_n38, m_n3c, m_n40;             // +0x034
	uint8		m_pad_044[0x5c - 0x44];                 // +0x044
	uint32		m_n5c, m_n60;                           // +0x05c
	uint8		m_pad_064[0x80 - 0x64];                 // +0x064
	uint32		m_n80, m_n84;                           // +0x080
	uint8		m_pad_088[0xb0 - 0x88];                 // +0x088
	uint32		m_nb0;                                  // +0x0b0
	uint8		m_pad_0b4[0xc0 - 0xb4];                 // +0x0b4
	uint32		m_nc0, m_nc4, m_nc8, m_ncc;             // +0x0c0
	uint8		m_pad_0d0[0xe8 - 0xd0];                 // +0x0d0
	uint32		m_ne8, m_nec;                           // +0x0e8
	uint8		m_pad_0f0[0x368 - 0xf0];                // +0x0f0
	uint32		m_n368, m_n36c, m_n370, m_n374;         // +0x368
	uint32		m_n378, m_n37c, m_n380, m_n384;         // +0x378
	uint32		m_n388, m_n38c, m_n390, m_n394;         // +0x388
	uint8		m_pad_398[0x530 - 0x398];               // +0x398
	uint32		m_n530, m_n534;                         // +0x530
	uint8		m_pad_538[0x768 - 0x538];               // +0x538
	uint16		m_wFrame768;                            // +0x768
	uint8		m_pad_76a[0x76c - 0x76a];               // +0x76a
	W7Node56c60*	m_pNodes76c;                        // +0x76c
	uint8		m_pad_770[0x7ac - 0x770];               // +0x770
	void*		m_pVertFns7ac[3];                       // +0x7ac
	void*		m_pVertFn7b8;                           // +0x7b8
	uint8		m_pad_7bc[0x7d0 - 0x7bc];               // +0x7bc
	uint32		m_n7d0;                                 // +0x7d0
	uint8		m_pad_7d4[0x7f0 - 0x7d4];               // +0x7d4
	W7QNode56c60*	m_pFreePolys7f0;                    // +0x7f0
};

// (declared on the drawers' poly type: it is the first poly of the node,
// and the table keeps the mangled name d3ddata.cpp already aliases)
typedef int32 (C1008f620::*W7InitFn56c60)(W7Poly56c60* pFirst, uint32 nStage);

extern W7FrameInfo56c60*	g_1008beb4;                 // 0x1008beb4
extern uint32				g_1008f600;                 // 0x1008f600
extern uint32				g_1008f5e0;                 // 0x1008f5e0
extern void*				g_100896f8[4];              // 0x100896f8  one-stage vertex fns
extern void*				g_10089708[4];              // 0x10089708  by stage count
extern W7InitFn56c60		g_10089718[4];              // 0x10089718  TextureSrcInitFn

extern void sub_10021d60(uint32 nLevel, const char* pszMsg, ...);  // d3d_Warning
// (0x100572b0 / 0x100572b4 is a C1008f620 member, declared in the class.)               // 0x100572b0 / 0x100572b4

void C1008f620::sub_10056c60()
{
	W7View56c60*		v = (W7View56c60*)this;
	uint32				nSaveBlend, nSaveSrc, nSaveDest;
	W7Node56c60*		pNode;
	W7Mat56c60*			pMat;
	W7NodePass56c60*	pNodePass;
	W7MatPass56c60*		pMatPass;
	W7Entry56c60*		pHead;
	W7Entry56c60*		pEntry;
	W7Entry56c60*		pNextEntry;
	W7QNode56c60*		pPoly;
	W7QNode56c60*		pWalk;
	W7QNode56c60*		pWalkNext;
	uint32				iPass, iStage, nNextPass;

	v->m_wFrame768 = g_1008beb4->m_wFrame0c;

	g_1008d988->lpVtbl->GetRenderState(g_1008d988, 0x1b, &nSaveBlend);
	g_1008d988->lpVtbl->GetRenderState(g_1008d988, 0x13, &nSaveSrc);
	g_1008d988->lpVtbl->GetRenderState(g_1008d988, 0x14, &nSaveDest);

	if (g_1008f600)
	{
		v->m_n40 = v->m_n34 = v->m_n20 = 1;
		v->m_n60 = v->m_n5c = v->m_n3c = v->m_n38 = v->m_n24 = 0;
	}
	else if (g_1008f5e0)
	{
		v->m_nc4 = v->m_nc0 = v->m_n60 = v->m_n5c = v->m_n3c = v->m_n38 = v->m_n34 = v->m_n24 = 0;
		v->m_nec = v->m_ne8 = v->m_ncc = v->m_nc8 = v->m_nb0 = v->m_n40 = 1;
		v->m_n20 = 2;
		v->m_n530 = 9;
		v->m_n534 = 3;
	}
	else
	{
		v->m_n394 = v->m_n388 = v->m_n84 = v->m_n80 = v->m_n20 = 1;
		v->m_n37c = v->m_n370 = v->m_n3c = v->m_n60 = v->m_n5c = v->m_n38 = v->m_n34 = v->m_n24 = 0;
		v->m_n390 = v->m_n384 = v->m_n378 = v->m_n36c = v->m_n40 = 2;
		v->m_n368 = v->m_n374 = v->m_n380 = v->m_n38c = 4;
	}

	v->m_n7d0 = 0;

	for (pNode = v->m_pNodes76c; pNode; pNode = pNode->m_pNext04)
	{
		pMat = pNode->m_pMat00;

		for (iPass = 0; iPass < pMat->m_nPasses20; iPass++)
		{
			pNodePass = &pNode->m_Passes08[iPass];
			pMatPass  = &pMat->m_Passes24[iPass];

			if (pNodePass->m_List188.m_pNext04 == &pNodePass->m_List188)
				break;

			g_1008d988->lpVtbl->SetRenderState(g_1008d988, 0x1b, pMatPass->m_bBlend00);
			if (pMatPass->m_bBlend00)
			{
				g_1008d988->lpVtbl->SetRenderState(g_1008d988, 0x13, pNodePass->m_nSrcBlend180);
				g_1008d988->lpVtbl->SetRenderState(g_1008d988, 0x14, pNodePass->m_nDestBlend184);
			}

			for (iStage = 0; iStage < pMatPass->m_nStages1c; iStage++)
			{
				W7StageStates56c60* pStates = &pNodePass->m_Stages194[iStage];

				g_1008d988->lpVtbl->SetTextureStageState(g_1008d988, iStage, 1, pStates->m_nColorOp);
				if (pStates->m_nColorOp != 1)
				{
					g_1008d988->lpVtbl->SetTextureStageState(g_1008d988, iStage, 2, pStates->m_nColorArg1);
					g_1008d988->lpVtbl->SetTextureStageState(g_1008d988, iStage, 3, pStates->m_nColorArg2);
				}
				g_1008d988->lpVtbl->SetTextureStageState(g_1008d988, iStage, 4, pStates->m_nAlphaOp);
				if (pStates->m_nAlphaOp != 1)
				{
					g_1008d988->lpVtbl->SetTextureStageState(g_1008d988, iStage, 5, pStates->m_nAlphaArg1);
					g_1008d988->lpVtbl->SetTextureStageState(g_1008d988, iStage, 6, pStates->m_nAlphaArg2);
				}
			}

			if (pMatPass->m_nStages1c == 1)
			{
				v->m_pVertFn7b8 = g_100896f8[pMatPass->m_Stages20[0].m_nVertFn18];
			}
			else
			{
				for (iStage = 0; iStage < pMatPass->m_nStages1c; iStage++)
					v->m_pVertFns7ac[iStage] = g_100896f8[pMatPass->m_Stages20[iStage].m_nVertFn18];
				v->m_pVertFn7b8 = g_10089708[pMatPass->m_nStages1c];
			}

			pHead = &pNodePass->m_List188;
			for (pEntry = pHead->m_pNext04; pEntry != pHead; pEntry = pNextEntry)
			{
				pPoly      = pEntry->m_pPoly08;
				pNextEntry = pEntry->m_pNext04;

				pWalk = pPoly->m_pFirst04;
				if (pWalk != pPoly)
				{
					for (iStage = 0; iStage < pMatPass->m_nStages1c; iStage++)
					{
						if (!(this->*g_10089718[pMatPass->m_Stages20[iStage].m_nDrawFn1c])(
						        (W7Poly56c60*)pWalk, iStage))
						{
							sub_10021d60(1, "Material '%s', pass %d, stage %d: TextureSrcInitFn failed.",
							             pMat, iPass, iStage);
							if (iPass == 0)
								sub_100572b0((Geom572b4*)pPoly);
							goto Unlink;
						}
					}

					nNextPass = (iPass == pMat->m_nPasses20 - 1) ? 0 : iPass + 1;

					if (iPass == 0)
					{
						for (pWalk = pPoly->m_pFirst04; pWalk != pPoly; pWalk = pWalkNext)
						{
							pWalkNext = pWalk->m_pFirst04;
							sub_100589c0((W7Poly56c60*)pWalk, pMatPass, nNextPass);
						}
					}
					else
					{
						for (pWalk = pPoly->m_pFirst04; pWalk != pPoly; pWalk = pWalkNext)
						{
							pWalkNext = pWalk->m_pFirst04;
							sub_10058e60((W7Poly56c60*)pWalk, pMatPass, nNextPass);
						}
					}
				}

Unlink:
				pPoly->m_Link10.m_pPrev->m_pNext = pPoly->m_Link10.m_pNext;
				pPoly->m_Link10.m_pNext->m_pPrev = pPoly->m_Link10.m_pPrev;
				pPoly->m_Link10.m_pPrev = &pPoly->m_Link10;
				pPoly->m_Link10.m_pNext = &pPoly->m_Link10;
				pPoly->m_Link1c.m_pPrev->m_pNext = pPoly->m_Link1c.m_pNext;
				pPoly->m_Link1c.m_pNext->m_pPrev = pPoly->m_Link1c.m_pPrev;
				pPoly->m_Link1c.m_pPrev = &pPoly->m_Link1c;
				pPoly->m_Link1c.m_pNext = &pPoly->m_Link1c;

				pPoly->m_pFree00 = v->m_pFreePolys7f0;
				v->m_pFreePolys7f0  = pPoly;
			}
		}
	}

	g_1008d988->lpVtbl->SetRenderState(g_1008d988, 0x1b, nSaveBlend);
	g_1008d988->lpVtbl->SetRenderState(g_1008d988, 0x13, nSaveSrc);
	g_1008d988->lpVtbl->SetRenderState(g_1008d988, 0x14, nSaveDest);
}





// ======================================================================= //
//  THE 32-BYTE DRAW VERTEX -- big124a4.cpp's / big16324.cpp's RVert,
//  unchanged, and every offset below is a load or a store in THESE bytes:
//  +0x00..+0x08 the position (0x10057350's `rep movsl $3` and the six
//  planes' x/y/z lerps), +0x0c the reciprocal w (0x10014580 writes it),
//  +0x10..+0x13 the four diffuse bytes the clipper interpolates in the
//  order r, g, b, a -- 0x12, 0x11, 0x10, 0x13 at 0x10057560, 0x100575ab,
//  0x100575f6, 0x10057641 -- and +0x17 the one specular byte it also
//  interpolates (0x1005768c), +0x18/+0x1c the texture coordinates
//  (0x10057542 / 0x10057551).
// ======================================================================= //
struct LTVec3
{
	float x;                                            // +0x00
	float y;                                            // +0x04
	float z;                                            // +0x08
};

struct RVert                                            // 0x20 bytes
{
	LTVec3 m_vPos;                                      // +0x00
	float  m_fRHW;                                      // +0x0c
	uint8  m_b;                                         // +0x10
	uint8  m_g;                                         // +0x11
	uint8  m_r;                                         // +0x12
	uint8  m_a;                                         // +0x13
	uint8  m_sb;                                        // +0x14
	uint8  m_sg;                                        // +0x15
	uint8  m_sr;                                        // +0x16
	uint8  m_sa;                                        // +0x17
	float  m_fU;                                        // +0x18
	float  m_fV;                                        // +0x1c
};


// ======================================================================= //
//  THE SOURCE RECORD.  Stride 0x18 is read off `add $0x18,%edx` at
//  0x1005736f, and the first field is a POINTER, not an embedded vector:
//  0x10057343 is `mov (%eax,%edx,1),%esi` and %esi is then the `rep movsl`
//  SOURCE.  Only that first dword is touched here; the remaining 0x14
//  bytes are named by nothing in this function.  0x100589c4 -- the sibling
//  the same caller reaches at 0x10057150 -- reads the same array the same
//  way at 0x10058a6e, which is what makes 0x18 a record stride rather than
//  a coincidence of this one loop.
// ======================================================================= //
struct SrcRef572b4                                      // 0x18 bytes
{
	LTVec3* m_pPos;                                     // +0x00
	uint8   m_pad_04[0x14];                             // +0x04 .. +0x17
};


// ======================================================================= //
//  THE RING NODE.  This function reads ONLY +0x04: 0x100572d3
//  `mov 0x4(%ebx),%esi` seeds the walk and 0x100588bf `mov 0x4(%eax),%eax`
//  advances it, both compared against the head at 0x8(%ebp).  +0x00 is
//  named from the CALLER's unlink of the same node shape at
//  0x100571c3..0x100571d2 (`h->m_p00->m_p04 = h->m_p04 ;
//  h->m_p04->m_p00 = h->m_p00`), which makes +0x00 the forward link and
//  +0x04 the back link -- so this walk runs the ring BACKWARDS.  The
//  element is also 0x1000a700's geometry object, whose +0x4c..+0x54
//  d3dbatchd.cpp already decoded.
// ======================================================================= //
struct Geom572b4
{
	Geom572b4* m_pNext;                                 // +0x00
	Geom572b4* m_pPrev;                                 // +0x04
};


// (IDirect3DDevice7 and g_pD3DDevice: declared above, the same view.)

#define D3DPT_TRIANGLEFAN                       6
#define D3DFVF_CLIPVERTEX                   0x1c4  // XYZRHW|DIFFUSE|SPEC|TEX1


// ======================================================================= //
//  GLOBALS.  Every one is an address these bytes touch, and every name is
//  one an earlier decode already fixed.
// ======================================================================= //

extern uint32       g_1008b874;                         // clip flags
extern RVert*       g_1008b87c;                         // scratch cursor
extern uint32       g_1008be44;                         // clipped-poly count
extern float        g_1008b908;                         // near plane
extern float        g_1008b910;                         // far plane
extern uint8        g_1008b880[];                       // the transform object

extern uint32       g_1008d7a0;                         // UseD3DClip


// ======================================================================= //
//  NEIGHBOURS.  Each signature is read off the call site in these bytes
//  and cross-checked against the decode named.
// ======================================================================= //

// 0x100232b0 -- d3dbatchd.cpp, "unbinds a texture stage".  One argument,
// the literal 0 at 0x100572c6; big44ff4.cpp declares it the same way.
extern "C" void   sub_100232b0(uint32 nStage);

// 0x1000a700 -- d3dbatchd.cpp.  (geometry, &data, &count); the pushes at
// 0x100572f1..0x100572f3 are &nVerts, &pData, pElem, so the arguments in
// source order are pElem, &pData, &nVerts.
extern "C" void   sub_1000a700(void* pGeom, void** ppOutData,
                               uint32* pnOutCount);

// 0x1000a740 -- d3dbatchd.cpp.  `cmpl $0x80,arg ; jg`, SIGNED, so the
// count is int32 and the bound is 128 -- which is exactly what the
// 0x1000-byte aVerts block in this frame holds.
extern "C" LTBOOL sub_1000a740(int32 nVerts);

// 0x1000d9b0 -- d3dlow.cpp, __cdecl (vertex, transform object).  Its body
// is four instructions of argument shuffling and a tail call to 0x1000a770
// with `pXForm + 0x15c`, and 0x1000a770 (dumped in full) is a pure 3x4
// matrix-vector multiply that writes x, y, z back in place and touches no
// global.  So it does NOT change g_1008b874 -- the reload at 0x100573b4 is
// icl declining to keep a global live across a call, which is the same
// thing big16324.cpp records at 0x10016a2f.
extern "C" void   sub_1000d9b0(RVert* pVert, uint8* pXForm);

// 0x10014580 -- the screen projection through +0x31c..+0x330, decoded in
// full at that address; d3dr2a.cpp and big124a4.cpp both call it __cdecl
// (vertex, transform object).
extern "C" void   sub_10014580(RVert* pVert, uint8* pXForm);


// The frame says fifty: each plane's flag array is 0xc8 = 200 bytes and
// the six arrays are 0xdc apart (-0x5e8, -0x50c, -0x430, -0x354, -0x278,
// -0x19c).  Same constant as big124a4.cpp and big6d910.cpp.
#define CLIP572B4_MAX	50


// ======================================================================= //
//  0x100572b4 -- the untextured fallback draw.  __stdcall: `ret $0x4` at
//  0x100588d3 and the single argument read from 0x8(%ebp) at 0x100572cd
//  and again at 0x100588bc.  The one call site, 0x100571b8, is
//  `push %eax ; call` with nothing else on the stack.
//  A C1008f620 MEMBER, not a __stdcall free function.  The
//  one call site loads `mov %esi,%ecx` (the caller's `this`) before the
//  push, and as a member the body reads its argument from 0x8(%ebp) with
//  no `push %eax` save around __chkstk -- the shipped opening, 38 bytes
//  identical (the free __stdcall form saved and used %eax, 3).  With the
//  RoundF helper on the thirty byte lerps (fistpl, not the control-word
//  dance): 6624 -> 5876 bytes (shipped 5904), 5930 -> 5070 differing;
//  it now pairs as a near.
// ======================================================================= //
// Round to nearest in the current FPU mode: w1_10001000.cpp's helper
// (0x10015190 out of line).  Inlined, it is the shipped `fstps t ;
// flds t ; fistpl n` at each of the thirty byte lerps.
inline int32 RoundF(float f)
{
	int32 n;
	__asm fld f
	__asm fistp n
	return n;
}

void C1008f620::sub_100572b0(Geom572b4* pHead)
{
	RVert        aVerts[128];
	RVert*       pVerts;
	int32        nVerts;
	SrcRef572b4* pData;

	Geom572b4*   pElem;
	RVert*       pOut;
	RVert*       pWalk;
	int32        nClipFlags;
	int32        nLeft;
	int32        i;

	// -- 0x100572c5: `push $0x0 ; call 0x100232b0` --------------------- //
	sub_100232b0(0);

	// -- 0x100572d3 / 0x100588b9: the ring walk, BACKWARDS through +0x04.
	//    The head test `cmp %ebx,%esi ; je <epilogue>` at 0x100572d6 and
	//    the tail test `cmp %edx,%eax ; jne 0x100572e1` at 0x100588c2 are
	//    the two halves of one `for`.
	for (pElem = pHead->m_pPrev; pElem != pHead; pElem = pElem->m_pPrev)
	{
		// -- 0x100572e2: the three pushes are &nVerts, &pData, pElem --- //
		sub_1000a700(pElem, (void**)&pData, (uint32*)&nVerts);

		// -- 0x100572fc: the 128-vertex guard.  The failure arm reaches
		//    the epilogue through the sunk trampoline at 0x100589b9.
		if (!sub_1000a740(nVerts))
			return;

		// -- 0x10057313: THE VERTEX BUILD.  `lea -0x15e8(%ebp),%eax`
		//    then `mov %eax,-0x15ec(%ebp)`; the loop guard at 0x10057327
		//    is `test ; jle` and the bottom at 0x10057372 is `cmp ; jl`,
		//    both SIGNED, so nVerts is int32.
		pVerts = aVerts;

		for (i = 0; i < nVerts; i++)
		{
			// 0x10057343 loads the record's first dword and 0x10057350
			// is `rep movsl` with $3 -- a 12-byte struct assignment.
			pVerts[i].m_vPos = *pData[i].m_pPos;

			// 0x1005735e: `mov %edx,0x10(%eax,%ebx,1)` with %edx the
			// RING ELEMENT itself, reloaded from its slot at 0x10057358.
			// See coin-flip (1).
			*(uint32*)&pVerts[i].m_b = (uint32)pElem;
		}

		// -- 0x10057391: `movl $0x3f,0x1008b874`, with the dead
		//    `mov $0x3f,%edx` at 0x1005738a.  big16324.cpp records the
		//    same pair at 0x100169f0 and the same coin-flip.
		g_1008b874 = 0x3f;

		// -- 0x1005739d: `test ; je` over a `dec ; jne` loop.
		pWalk = pVerts;
		nLeft = nVerts;

		while (nLeft--)
		{
			sub_1000d9b0(pWalk, g_1008b880);
			pWalk++;
		}

		// -- 0x100573b4: the flag word is RE-READ from the global.  It
		//    cannot have changed (0x1000d9b0 tail-calls 0x1000a770, a
		//    pure 3x4 transform), so nClipFlags is always 0x3f here and
		//    -- unlike 0x100124a4's copy -- all six planes are LIVE code
		//    in this function.  The reload is icl refusing to keep a
		//    global in a register across the calls.
		nClipFlags = g_1008b874;

		// -- 0x100573c0: UseD3DClip.  Byte-identical to 0x10012bb0.
		if (g_1008d7a0)
		{
			nClipFlags &= 1;
			if (!nClipFlags)
				goto L_Draw;
		}

		// -- 0x100573d8: 0x1000f4b0's own `mov 0x1008b87c,%eax`.
		pOut = g_1008b87c;


		// -------------------------------------------------------------- //
		//  0x100573fe -- bit 0x01, the NEAR plane.  The classify is
		//  fcomps 0x1008b908 / jae at 0x1005743c; the epsilon pair is 0x100aa6d0
		//  (-0.00001f) in line and 0x100588d6 out of line (+0.00001f, 0.0f).
		// -------------------------------------------------------------- //
		if (nClipFlags & 0x01)
		{
			int32   bInside[CLIP572B4_MAX];
			float   fRed, fGreen, fBlue, fAlpha, fSpec;
			RVert  *pStart, *pEnd, *pCur, *pPrev;
			int32  *pFlag;
			int32   nInside, nFlag, i, j;
			float   fDenom, fT;

			g_1008be44++;

			nInside = 0;
			pFlag   = bInside;
			pStart  = pVerts;
			pEnd    = pStart + nVerts;

			for (pCur = pStart; pCur != pEnd; pCur++)
			{
				nFlag = (pCur->m_vPos.z >= g_1008b908) ? 1 : 0;
				nInside += nFlag;
				*pFlag = nFlag;
				pFlag++;
			}

			if (nInside == 0)
				continue;

			if (nInside != nVerts)
			{
				pStart = pOut;
				pPrev  = pVerts + (nVerts - 1);
				j      = nVerts - 1;

				for (i = 0; i < nVerts; i++)
				{
					pCur = &pVerts[i];

					if (bInside[j])
					{
						*pOut = *pPrev;
						pOut++;
					}

					if (bInside[j] != bInside[i])
					{
						fDenom = pCur->m_vPos.z - pPrev->m_vPos.z;

						if (fDenom < -0.00001f || fDenom > 0.00001f)
							fT = (g_1008b908 - pPrev->m_vPos.z) / fDenom;
						else
							fT = 0.0f;

						pOut->m_vPos.x =
							(pCur->m_vPos.x - pPrev->m_vPos.x) * fT + pPrev->m_vPos.x;
						pOut->m_vPos.y =
							(pCur->m_vPos.y - pPrev->m_vPos.y) * fT + pPrev->m_vPos.y;
						pOut->m_vPos.z = g_1008b908;

						pOut->m_fU = (pCur->m_fU - pPrev->m_fU) * fT + pPrev->m_fU;
						pOut->m_fV = (pCur->m_fV - pPrev->m_fV) * fT + pPrev->m_fV;

						fRed   = (pCur->m_r  - pPrev->m_r)  * fT + pPrev->m_r;
						pOut->m_r  = (uint8)RoundF(fRed);
						fGreen = (pCur->m_g  - pPrev->m_g)  * fT + pPrev->m_g;
						pOut->m_g  = (uint8)RoundF(fGreen);
						fBlue  = (pCur->m_b  - pPrev->m_b)  * fT + pPrev->m_b;
						pOut->m_b  = (uint8)RoundF(fBlue);
						fAlpha = (pCur->m_a  - pPrev->m_a)  * fT + pPrev->m_a;
						pOut->m_a  = (uint8)RoundF(fAlpha);
						fSpec  = (pCur->m_sa - pPrev->m_sa) * fT + pPrev->m_sa;
						pOut->m_sa = (uint8)RoundF(fSpec);

						pOut++;
					}

					pPrev = pCur;
					j = i;
				}

				nVerts = (int32)(pOut - pStart);
				pVerts = pStart;
				pOut  += nVerts;
			}
		}

		// -------------------------------------------------------------- //
		//  0x10057734 -- bit 0x04, the LEFT plane.  The classify is
		//  fchs / fcomps (%edx) / jb at 0x10057772; the epsilon pair is 0x100aa6d8
		//  (-0.00001f) in line and 0x100588f9 out of line (+0.00001f, 0.0f).
		// -------------------------------------------------------------- //
		if (nClipFlags & 0x04)
		{
			int32   bInside[CLIP572B4_MAX];
			float   fRed, fGreen, fBlue, fAlpha, fSpec;
			RVert  *pStart, *pEnd, *pCur, *pPrev;
			int32  *pFlag;
			int32   nInside, nFlag, i, j;
			float   fDenom, fT, fZ;

			g_1008be44++;

			nInside = 0;
			pFlag   = bInside;
			pStart  = pVerts;
			pEnd    = pStart + nVerts;

			for (pCur = pStart; pCur != pEnd; pCur++)
			{
				nFlag = (-pCur->m_vPos.z < pCur->m_vPos.x) ? 1 : 0;
				nInside += nFlag;
				*pFlag = nFlag;
				pFlag++;
			}

			if (nInside == 0)
				continue;

			if (nInside != nVerts)
			{
				pStart = pOut;
				pPrev  = pVerts + (nVerts - 1);
				j      = nVerts - 1;

				for (i = 0; i < nVerts; i++)
				{
					pCur = &pVerts[i];

					if (bInside[j])
					{
						*pOut = *pPrev;
						pOut++;
					}

					if (bInside[j] != bInside[i])
					{
						fDenom = pCur->m_vPos.x - pPrev->m_vPos.x
						       + pCur->m_vPos.z - pPrev->m_vPos.z;

						if (fDenom < -0.00001f || fDenom > 0.00001f)
							fT = -(pPrev->m_vPos.x + pPrev->m_vPos.z) / fDenom;
						else
							fT = 0.0f;

						pOut->m_vPos.y =
							(pCur->m_vPos.y - pPrev->m_vPos.y) * fT + pPrev->m_vPos.y;
						fZ = (pCur->m_vPos.z - pPrev->m_vPos.z) * fT + pPrev->m_vPos.z;
						pOut->m_vPos.z = fZ;
						pOut->m_vPos.x = -fZ;

						pOut->m_fU = (pCur->m_fU - pPrev->m_fU) * fT + pPrev->m_fU;
						pOut->m_fV = (pCur->m_fV - pPrev->m_fV) * fT + pPrev->m_fV;

						fRed   = (pCur->m_r  - pPrev->m_r)  * fT + pPrev->m_r;
						pOut->m_r  = (uint8)RoundF(fRed);
						fGreen = (pCur->m_g  - pPrev->m_g)  * fT + pPrev->m_g;
						pOut->m_g  = (uint8)RoundF(fGreen);
						fBlue  = (pCur->m_b  - pPrev->m_b)  * fT + pPrev->m_b;
						pOut->m_b  = (uint8)RoundF(fBlue);
						fAlpha = (pCur->m_a  - pPrev->m_a)  * fT + pPrev->m_a;
						pOut->m_a  = (uint8)RoundF(fAlpha);
						fSpec  = (pCur->m_sa - pPrev->m_sa) * fT + pPrev->m_sa;
						pOut->m_sa = (uint8)RoundF(fSpec);

						pOut++;
					}

					pPrev = pCur;
					j = i;
				}

				nVerts = (int32)(pOut - pStart);
				pVerts = pStart;
				pOut  += nVerts;
			}
		}

		// -------------------------------------------------------------- //
		//  0x10057a8b -- bit 0x08, the TOP plane.  The classify is
		//  fcomps 0x8(%edx) / jb at 0x10057ac4; the epsilon pair is 0x100aa6e0
		//  (-0.00001f) in line and 0x1005891e out of line (+0.00001f, 0.0f).
		// -------------------------------------------------------------- //
		if (nClipFlags & 0x08)
		{
			int32   bInside[CLIP572B4_MAX];
			float   fRed, fGreen, fBlue, fAlpha, fSpec;
			RVert  *pStart, *pEnd, *pCur, *pPrev;
			int32  *pFlag;
			int32   nInside, nFlag, i, j;
			float   fDenom, fT, fZ;

			g_1008be44++;

			nInside = 0;
			pFlag   = bInside;
			pStart  = pVerts;
			pEnd    = pStart + nVerts;

			for (pCur = pStart; pCur != pEnd; pCur++)
			{
				nFlag = (pCur->m_vPos.y < pCur->m_vPos.z) ? 1 : 0;
				nInside += nFlag;
				*pFlag = nFlag;
				pFlag++;
			}

			if (nInside == 0)
				continue;

			if (nInside != nVerts)
			{
				pStart = pOut;
				pPrev  = pVerts + (nVerts - 1);
				j      = nVerts - 1;

				for (i = 0; i < nVerts; i++)
				{
					pCur = &pVerts[i];

					if (bInside[j])
					{
						*pOut = *pPrev;
						pOut++;
					}

					if (bInside[j] != bInside[i])
					{
						fDenom = pCur->m_vPos.y - pPrev->m_vPos.y
						       - pCur->m_vPos.z + pPrev->m_vPos.z;

						if (fDenom < -0.00001f || fDenom > 0.00001f)
							fT = -(pPrev->m_vPos.y - pPrev->m_vPos.z) / fDenom;
						else
							fT = 0.0f;

						pOut->m_vPos.x =
							(pCur->m_vPos.x - pPrev->m_vPos.x) * fT + pPrev->m_vPos.x;
						fZ = (pCur->m_vPos.z - pPrev->m_vPos.z) * fT + pPrev->m_vPos.z;
						pOut->m_vPos.y = fZ;
						pOut->m_vPos.z = fZ;

						pOut->m_fU = (pCur->m_fU - pPrev->m_fU) * fT + pPrev->m_fU;
						pOut->m_fV = (pCur->m_fV - pPrev->m_fV) * fT + pPrev->m_fV;

						fRed   = (pCur->m_r  - pPrev->m_r)  * fT + pPrev->m_r;
						pOut->m_r  = (uint8)RoundF(fRed);
						fGreen = (pCur->m_g  - pPrev->m_g)  * fT + pPrev->m_g;
						pOut->m_g  = (uint8)RoundF(fGreen);
						fBlue  = (pCur->m_b  - pPrev->m_b)  * fT + pPrev->m_b;
						pOut->m_b  = (uint8)RoundF(fBlue);
						fAlpha = (pCur->m_a  - pPrev->m_a)  * fT + pPrev->m_a;
						pOut->m_a  = (uint8)RoundF(fAlpha);
						fSpec  = (pCur->m_sa - pPrev->m_sa) * fT + pPrev->m_sa;
						pOut->m_sa = (uint8)RoundF(fSpec);

						pOut++;
					}

					pPrev = pCur;
					j = i;
				}

				nVerts = (int32)(pOut - pStart);
				pVerts = pStart;
				pOut  += nVerts;
			}
		}

		// -------------------------------------------------------------- //
		//  0x10057de6 -- bit 0x10, the RIGHT plane.  The classify is
		//  fcomps 0x8(%edx) / jb at 0x10057e1f; the epsilon pair is 0x100aa6e8
		//  (-0.00001f) in line and 0x10058943 out of line (+0.00001f, 0.0f).
		// -------------------------------------------------------------- //
		if (nClipFlags & 0x10)
		{
			int32   bInside[CLIP572B4_MAX];
			float   fRed, fGreen, fBlue, fAlpha, fSpec;
			RVert  *pStart, *pEnd, *pCur, *pPrev;
			int32  *pFlag;
			int32   nInside, nFlag, i, j;
			float   fDenom, fT, fZ;

			g_1008be44++;

			nInside = 0;
			pFlag   = bInside;
			pStart  = pVerts;
			pEnd    = pStart + nVerts;

			for (pCur = pStart; pCur != pEnd; pCur++)
			{
				nFlag = (pCur->m_vPos.x < pCur->m_vPos.z) ? 1 : 0;
				nInside += nFlag;
				*pFlag = nFlag;
				pFlag++;
			}

			if (nInside == 0)
				continue;

			if (nInside != nVerts)
			{
				pStart = pOut;
				pPrev  = pVerts + (nVerts - 1);
				j      = nVerts - 1;

				for (i = 0; i < nVerts; i++)
				{
					pCur = &pVerts[i];

					if (bInside[j])
					{
						*pOut = *pPrev;
						pOut++;
					}

					if (bInside[j] != bInside[i])
					{
						fDenom = pCur->m_vPos.x - pPrev->m_vPos.x
						       - pCur->m_vPos.z + pPrev->m_vPos.z;

						if (fDenom < -0.00001f || fDenom > 0.00001f)
							fT = -(pPrev->m_vPos.x - pPrev->m_vPos.z) / fDenom;
						else
							fT = 0.0f;

						pOut->m_vPos.y =
							(pCur->m_vPos.y - pPrev->m_vPos.y) * fT + pPrev->m_vPos.y;
						fZ = (pCur->m_vPos.z - pPrev->m_vPos.z) * fT + pPrev->m_vPos.z;
						pOut->m_vPos.x = fZ;
						pOut->m_vPos.z = fZ;

						pOut->m_fU = (pCur->m_fU - pPrev->m_fU) * fT + pPrev->m_fU;
						pOut->m_fV = (pCur->m_fV - pPrev->m_fV) * fT + pPrev->m_fV;

						fRed   = (pCur->m_r  - pPrev->m_r)  * fT + pPrev->m_r;
						pOut->m_r  = (uint8)RoundF(fRed);
						fGreen = (pCur->m_g  - pPrev->m_g)  * fT + pPrev->m_g;
						pOut->m_g  = (uint8)RoundF(fGreen);
						fBlue  = (pCur->m_b  - pPrev->m_b)  * fT + pPrev->m_b;
						pOut->m_b  = (uint8)RoundF(fBlue);
						fAlpha = (pCur->m_a  - pPrev->m_a)  * fT + pPrev->m_a;
						pOut->m_a  = (uint8)RoundF(fAlpha);
						fSpec  = (pCur->m_sa - pPrev->m_sa) * fT + pPrev->m_sa;
						pOut->m_sa = (uint8)RoundF(fSpec);

						pOut++;
					}

					pPrev = pCur;
					j = i;
				}

				nVerts = (int32)(pOut - pStart);
				pVerts = pStart;
				pOut  += nVerts;
			}
		}

		// -------------------------------------------------------------- //
		//  0x1005815f -- bit 0x20, the BOTTOM plane.  The classify is
		//  fchs / fcomps 0x4(%edx) / jb at 0x10058198; the epsilon pair is 0x100aa6f0
		//  (-0.00001f) in line and 0x1005896b out of line (+0.00001f, 0.0f).
		// -------------------------------------------------------------- //
		if (nClipFlags & 0x20)
		{
			int32   bInside[CLIP572B4_MAX];
			float   fRed, fGreen, fBlue, fAlpha, fSpec;
			RVert  *pStart, *pEnd, *pCur, *pPrev;
			int32  *pFlag;
			int32   nInside, nFlag, i, j;
			float   fDenom, fT, fZ;

			g_1008be44++;

			nInside = 0;
			pFlag   = bInside;
			pStart  = pVerts;
			pEnd    = pStart + nVerts;

			for (pCur = pStart; pCur != pEnd; pCur++)
			{
				nFlag = (-pCur->m_vPos.z < pCur->m_vPos.y) ? 1 : 0;
				nInside += nFlag;
				*pFlag = nFlag;
				pFlag++;
			}

			if (nInside == 0)
				continue;

			if (nInside != nVerts)
			{
				pStart = pOut;
				pPrev  = pVerts + (nVerts - 1);
				j      = nVerts - 1;

				for (i = 0; i < nVerts; i++)
				{
					pCur = &pVerts[i];

					if (bInside[j])
					{
						*pOut = *pPrev;
						pOut++;
					}

					if (bInside[j] != bInside[i])
					{
						fDenom = pCur->m_vPos.y - pPrev->m_vPos.y
						       + pCur->m_vPos.z - pPrev->m_vPos.z;

						if (fDenom < -0.00001f || fDenom > 0.00001f)
							fT = -(pPrev->m_vPos.y + pPrev->m_vPos.z) / fDenom;
						else
							fT = 0.0f;

						pOut->m_vPos.x =
							(pCur->m_vPos.x - pPrev->m_vPos.x) * fT + pPrev->m_vPos.x;
						fZ = (pCur->m_vPos.z - pPrev->m_vPos.z) * fT + pPrev->m_vPos.z;
						pOut->m_vPos.z = fZ;
						pOut->m_vPos.y = -fZ;

						pOut->m_fU = (pCur->m_fU - pPrev->m_fU) * fT + pPrev->m_fU;
						pOut->m_fV = (pCur->m_fV - pPrev->m_fV) * fT + pPrev->m_fV;

						fRed   = (pCur->m_r  - pPrev->m_r)  * fT + pPrev->m_r;
						pOut->m_r  = (uint8)RoundF(fRed);
						fGreen = (pCur->m_g  - pPrev->m_g)  * fT + pPrev->m_g;
						pOut->m_g  = (uint8)RoundF(fGreen);
						fBlue  = (pCur->m_b  - pPrev->m_b)  * fT + pPrev->m_b;
						pOut->m_b  = (uint8)RoundF(fBlue);
						fAlpha = (pCur->m_a  - pPrev->m_a)  * fT + pPrev->m_a;
						pOut->m_a  = (uint8)RoundF(fAlpha);
						fSpec  = (pCur->m_sa - pPrev->m_sa) * fT + pPrev->m_sa;
						pOut->m_sa = (uint8)RoundF(fSpec);

						pOut++;
					}

					pPrev = pCur;
					j = i;
				}

				nVerts = (int32)(pOut - pStart);
				pVerts = pStart;
				pOut  += nVerts;
			}
		}

		// -------------------------------------------------------------- //
		//  0x100584e0 -- bit 0x02, the FAR plane.  The classify is
		//  fcomps 0x1008b910 / jbe at 0x10058515; the epsilon pair is 0x100aa6f8
		//  (-0.00001f) in line and 0x10058993 out of line (+0.00001f, 0.0f).
		// -------------------------------------------------------------- //
		if (nClipFlags & 0x02)
		{
			int32   bInside[CLIP572B4_MAX];
			float   fRed, fGreen, fBlue, fAlpha, fSpec;
			RVert  *pStart, *pEnd, *pCur, *pPrev;
			int32  *pFlag;
			int32   nInside, nFlag, i, j;
			float   fDenom, fT;

			g_1008be44++;

			nInside = 0;
			pFlag   = bInside;
			pStart  = pVerts;
			pEnd    = pStart + nVerts;

			for (pCur = pStart; pCur != pEnd; pCur++)
			{
				nFlag = (pCur->m_vPos.z <= g_1008b910) ? 1 : 0;
				nInside += nFlag;
				*pFlag = nFlag;
				pFlag++;
			}

			if (nInside == 0)
				continue;

			if (nInside != nVerts)
			{
				pStart = pOut;
				pPrev  = pVerts + (nVerts - 1);
				j      = nVerts - 1;

				for (i = 0; i < nVerts; i++)
				{
					pCur = &pVerts[i];

					if (bInside[j])
					{
						*pOut = *pPrev;
						pOut++;
					}

					if (bInside[j] != bInside[i])
					{
						fDenom = pCur->m_vPos.z - pPrev->m_vPos.z;

						if (fDenom < -0.00001f || fDenom > 0.00001f)
							fT = (g_1008b910 - pPrev->m_vPos.z) / fDenom;
						else
							fT = 0.0f;

						pOut->m_vPos.x =
							(pCur->m_vPos.x - pPrev->m_vPos.x) * fT + pPrev->m_vPos.x;
						pOut->m_vPos.y =
							(pCur->m_vPos.y - pPrev->m_vPos.y) * fT + pPrev->m_vPos.y;
						pOut->m_vPos.z = g_1008b910;

						pOut->m_fU = (pCur->m_fU - pPrev->m_fU) * fT + pPrev->m_fU;
						pOut->m_fV = (pCur->m_fV - pPrev->m_fV) * fT + pPrev->m_fV;

						fRed   = (pCur->m_r  - pPrev->m_r)  * fT + pPrev->m_r;
						pOut->m_r  = (uint8)RoundF(fRed);
						fGreen = (pCur->m_g  - pPrev->m_g)  * fT + pPrev->m_g;
						pOut->m_g  = (uint8)RoundF(fGreen);
						fBlue  = (pCur->m_b  - pPrev->m_b)  * fT + pPrev->m_b;
						pOut->m_b  = (uint8)RoundF(fBlue);
						fAlpha = (pCur->m_a  - pPrev->m_a)  * fT + pPrev->m_a;
						pOut->m_a  = (uint8)RoundF(fAlpha);
						fSpec  = (pCur->m_sa - pPrev->m_sa) * fT + pPrev->m_sa;
						pOut->m_sa = (uint8)RoundF(fSpec);

						pOut++;
					}

					pPrev = pCur;
					j = i;
				}

				nVerts = (int32)(pOut - pStart);
				pVerts = pStart;
				pOut  += nVerts;
			}
		}

L_Draw:
		// -- 0x10058841..0x10058853 is the compiler copying the last
		//    plane's working pVerts/nVerts (-0x64 / -0xac) back into the
		//    slots the draw reads (-0x15ec / -0x15f0); in source they are
		//    the same two variables, so there is no statement for it.
		//    0x10058856 is also where the "D3D will clip it" shortcut at
		//    0x100573d2 lands.
		// -- 0x10058862: `test ; je` over a `dec ; jne` loop, the same
		//    shape as the pre-transform above.
		pWalk = pVerts;
		nLeft = nVerts;

		while (nLeft--)
		{
			sub_10014580(pWalk, g_1008b880);
			pWalk++;
		}

		// -- 0x1005887d: `add $0xffffffe8,%esp` and five `mov`s into the
		//    outgoing area, then `mov (%eax),%eax ; call *0x64(%eax)`.
		//    6 is D3DPT_TRIANGLEFAN and 0x1c4 is XYZRHW|DIFFUSE|SPECULAR
		//    |TEX1, which is exactly this 32-byte vertex.
		g_pD3DDevice->lpVtbl->DrawPrimitive(g_pD3DDevice,
		                                    D3DPT_TRIANGLEFAN,
		                                    D3DFVF_CLIPVERTEX,
		                                    pVerts, nVerts, 0);
	}
}


// *********************************************************************** //
//  WHAT THE ACCEPTANCE TEST CANNOT SEE, AND THE COIN-FLIPS
// *********************************************************************** //
//
//  The call-sequence test witnesses seven call sites in 5,904 bytes.  It says NOTHING
//  about 0x100573ab..0x1005886e -- 5,316 bytes, 90.0% of the function,
//  1,264 of its 1,470 instructions.  The evidence for that region is the
//  instruction-for-instruction agreement with 0x100124a4's already-decoded
//  copy of the same inlined clipper (six differing slots per plane, all
//  one idiom), the exact decomposition of the 0x1700 frame, and the
//  instruction census -- all set out in the header.  Quote those three
//  alongside any call-sequence figure; the figure alone would be worth little
//  here.
//
//  (1) THE ELEMENT POINTER IN THE DIFFUSE DWORD.  0x1005735e is
//      `mov %edx,0x10(%eax,%ebx,1)` and %edx is the ring element,
//      reloaded from its own slot at 0x10057358.  That slot (-0x68) is
//      written in exactly two places in the whole function -- 0x100572de
//      `mov %esi,-0x68(%ebp)` from `pHead->m_pPrev`, and 0x100588c4 from
//      `pElem->m_pPrev` -- so it is unambiguously the element pointer and
//      not a colour read out of it.  +0x10 is unambiguously the diffuse
//      dword: the FVF at 0x10058885 is 0x1c4 and the clipper interpolates
//      +0x10..+0x13 as four bytes.  So the shipped source really does put
//      a pointer where the vertex colour goes.  On a texture-init failure
//      path with no u/v ever written that reads as either a per-piece
//      junk colour or an unfixed bug; either way the bytes are the bytes.
//      Written here, following big16324.cpp's identical problem at +0x14,
//      as a cast through the byte view:
//
//          *(uint32*)&pVerts[i].m_b = (uint32)pElem;
//
//      The alternative spelling, if icl objects to the cast or costs
//      bytes on it, is a second view in the struct --
//      `uint32 m_nColor;` at +0x10 with the clipper reaching its bytes as
//      `((uint8*)&pOut->m_nColor)[2]` and so on -- and then
//
//          pVerts[i].m_nColor = (uint32)pElem;
//
//      Same object code; only the source is at issue.
//
//  (2) `float fT` VERSUS `double fT`.  All six planes spill fT with
//      `fstpl` and reload it with `fmull`, i.e. 64 bits.  That is NOT
//      evidence of a `double`: 0x100124a4's near plane emits the same
//      `fdivp ; fstpl -0xb0(%ebp) ; ... fmull -0xb0(%ebp)` for source
//      that big124a4.cpp writes as `float fT`, and 0x10002d40 keeps the
//      same value in %st and never spills it at all.  `float` is written
//      here to match both files.  The one-word alternative is
//
//          double  fDenom, fT, fZ;      /* instead of float */
//
//      in all six blocks.  It is worth one build, because if fT really is
//      a double the multiply operand type changes throughout.
//
//  (3) THE DEAD 0x3f, exactly as big16324.cpp records it.  0x1005738a
//      materialises `mov $0x3f,%edx` and the only reader of that register
//      is the join at 0x100573ba, reached when nVerts == 0.  Written as
//
//          g_1008b874 = 0x3f;  ...  nClipFlags = g_1008b874;
//
//      The other reading is
//
//          nClipFlags = 0x3f;  g_1008b874 = nClipFlags;
//
//      with nClipFlags reloaded from the global anyway because the
//      intervening calls may write it.
//
//  (4) `incl` VERSUS LOAD-INC-STORE on g_1008be44.  Each plane opens with
//      `mov 0x1008be44,%edx ; inc %edx ; mov %edx,0x1008be44` (0x10057414,
//      0x10057748, 0x10057a9f, 0x10057dfa, 0x10058173, 0x100584f4) where
//      0x100124a4's six planes all emit `incl 0x1008be44`.  That is the
//      ONLY systematic difference between the two copies of the clipper,
//      and it is six instructions out of 1,264.  Written as
//
//          g_1008be44++;
//
//      to match big124a4.cpp.  If the two forms turn out to be a source
//      distinction rather than scheduling, the alternative is
//
//          g_1008be44 = g_1008be44 + 1;
//
//      and it is worth testing on ONE plane before touching the other
//      five.
//
//  (5) THE RING DIRECTION.  This function reads +0x04 to advance, and
//      +0x04 is the BACK link if the caller's unlink at
//      0x100571c3..0x100571d2 is read as `next` at +0x00 / `prev` at
//      +0x04.  Nothing in these bytes distinguishes that from the
//      opposite naming; only the field OFFSET is proved.  `m_pPrev` is
//      the name used above; swapping the two names in `Geom572b4` changes
//      no code.
//
//  (6) THE DEAD `pOut += nVerts` IN PLANE 6.  Planes 1..5 emit the
//      `shl $0x5 ; add` that updates pOut after the rebuild; plane 6 does
//      not, because nothing reads pOut afterwards.  Written uniformly in
//      all six (a dead store to a register-allocated local, which icl
//      removes); the alternative is to omit the line from plane 6 only.
//
//  (7) `continue` VERSUS `goto`.  The six "no vertex survived this plane"
//      exits all jump to 0x100588b9, which is the ring advance -- exactly
//      where `continue` in the `for` lands.  0x100124a4's equivalents go
//      to a shared tail and big124a4.cpp spells them `goto L_Done`.
//      Written as `continue` here because the target IS the increment.
//
//  NOT A COIN-FLIP, RECORDED FOR THE NEXT READER: the four bytes at
//  0x100572b0 (`mov 0x4(%esp),%eax`) and at 0x100589c0 are /Qip stack
//  entries with no caller anywhere in .text.  They are not part of this
//  function's body; icl emits them in front of an external function that
//  has a caller in its own /Qip TU (here 0x10056c60), and in this file
//  both are byte-identical.
//
// *********************************************************************** //


// ======================================================================= //
//  0x100589c4 -- 1177 bytes (`ret $0xc`; the shim at 0x100589c0).  Draw
//  pPoly's first pass: build its vertices (position copied, then the
//  pass's colour function, the fog function on (position, specular), the
//  manager's current UV function), run the stage init functions when the
//  poly and the pass ask for it, clip to the planes in the poly's low six
//  flag bits, set each stage, DrawPrimitive(TRIANGLEFAN).  When there is
//  a next pass, save the clipped positions into the manager's vertex
//  array (grown 0x100 at a time), record where in the poly, clear its
//  clip bits if the clip left the vertices where they were, and queue it
//  for pass 1.
// ======================================================================= //
void C1008f620::sub_100589c0(W7Poly56c60* pPoly, W7MatPass56c60* pPass,
                             uint32 nNextPass)
{
	Ctx589c4		ctx;
	uint8*			pOut;
	FogFn589c4		pfnFog;
	ColorFn589c4	pfnColor;
	SrcRec589c4*	pSrc;
	SavedVert589c4*	pDst;
	int32			i;
	uint32			iStage;

	ctx.m_pPoly = pPoly;

	if (g_CV_FixTJunc)
	{
		ctx.m_nVerts = pPoly->m_nFixVerts52;
		pSrc         = pPoly->m_pFixVerts4c;
	}
	else
	{
		ctx.m_nVerts = pPoly->m_nVerts50;
		pSrc         = pPoly->m_Verts54;
	}

	ctx.m_nStride = g_1008974c[pPass->m_nStages1c];
	pfnFog        = *g_10089744[pPass->m_nFogFn14];
	pfnColor      = ((ColorFn589c4*)g_10089738)[pPass->m_nColorFn10];

	pOut = ctx.m_Verts;
	for (i = 0; i < ctx.m_nVerts; i++)
	{
		*(LTVector*)pOut = *pSrc[i].m_pPos;
		(this->*pfnColor)(&pSrc[i], pOut);
		pfnFog(pOut, pOut + 0x14);
		(this->*m_pVertFn7b8)(&pSrc[i], pOut + 0x18, 0);
		pOut += ctx.m_nStride;
	}

	ctx.m_nStage[1] = 0;
	ctx.m_nStage[2] = 0;
	ctx.m_nStage[0] = 0;

	if (pPoly->m_n2c && pPass->m_bInit0c)
	{
		for (iStage = 0; iStage < pPass->m_nStages1c; iStage++)
			(this->*((InitFn589c4*)g_1008977c)[pPass->m_Stages20[iStage].m_nInitFn20])(&ctx);
	}

	pOut = ctx.m_Verts;
	if (g_1008975c[pPass->m_nStages1c](pPoly->m_wClip14 & 0x3f, &pOut, &ctx.m_nVerts))
	{
		for (iStage = 0; iStage < pPass->m_nStages1c; iStage++)
			(this->*((DrawFn589c4*)g_10089728)[pPass->m_Stages20[iStage].m_nDrawFn1c])(pPoly, iStage,
			                                                          ctx.m_nStage[iStage]);

		g_pD3DDevice->lpVtbl->DrawPrimitive(g_pD3DDevice, 6,
		                                  g_10089788[pPass->m_nStages1c],
		                                  pOut, ctx.m_nVerts, 0);

		if (!nNextPass)
			return;
	}
	else
		return;

	if (m_SavedVerts.GetSize() - m_nSaved7d0 < (uint32)ctx.m_nVerts)
	{
		if (!m_SavedVerts.NiceSetSize(m_SavedVerts.GetSize() + 0x100))
			return;
	}

	pPoly->m_nSaved08 = (uint16)ctx.m_nVerts | (m_nSaved7d0 << 16);

	if (pOut == ctx.m_Verts)
		pPoly->m_wClip14 &= ~0x3f;

	pDst = m_SavedVerts.GetArray() + m_nSaved7d0;
	for (i = 0; i < ctx.m_nVerts; i++)
	{
		pDst[i].m_Pos = *(LTVector*)pOut;
		pDst[i].m_n0c = ((uint32*)pOut)[3];
		pOut += ctx.m_nStride;
	}

	m_nSaved7d0 += ctx.m_nVerts;

	sub_10056ab0(pPoly, 1);
}


// ======================================================================= //
//  0x10058e64 -- 825 bytes (`ret $0xc`; the shim at 0x10058e60).  Draw a
//  later pass of pPoly.  If the poly was clipped (low six flag bits), its
//  vertices are rebuilt from source, clipped again, and must come out the
//  count saved by the first pass ("nQVerts != nVertices" otherwise); the
//  saved positions then overwrite the rebuilt ones.  If it was not, the
//  vertices are built straight from the saved positions (the fog
//  function then reads the source position).  Then each stage, the draw,
//  and the queue for the next pass.
//
//  NEAR (825 of 825 bytes, 7 mnemonic blocks).  The T-junction arm reads
//  the vertex pointer before the count (ship loads +0x4c ahead of the
//  count's store).  LEFT: the first build loop -- ship carries pOut + stride
//  into the next iteration in %edi (the rep movsl destination) and ours
//  reloads it from the frame; unchanged by the increment in the for
//  header (either order), a while loop, `pOut = &pOut[nStride]`, and the
//  fog call without the volatile double read (worse).
// ======================================================================= //
void C1008f620::sub_10058e60(W7Poly56c60* pPoly, W7MatPass56c60* pPass,
                             uint32 nNextPass)
{
	int32			nVerts;
	uint8			verts[0x1400];
	uint8*			pOut;
	uint8*			pV;
	FogFn589c4		pfnFog;
	ColorFn589c4	pfnColor;
	uint32			nStride;
	SrcRec589c4*	pSrc;
	SrcRec589c4*	pS;
	SavedVert589c4*	pSaved;
	uint32			nSaved;
	int32			i;
	uint32			iStage;

	nStride  = g_1008974c[pPass->m_nStages1c];
	pfnColor = ((ColorFn589c4*)g_10089738)[pPass->m_nColorFn10];
	pfnFog   = *g_10089744[pPass->m_nFogFn14];

	if (g_CV_FixTJunc)
	{
		pSrc   = pPoly->m_pFixVerts4c;
		nVerts = pPoly->m_nFixVerts52;
	}
	else
	{
		nVerts = pPoly->m_nVerts50;
		pSrc   = pPoly->m_Verts54;
	}

	nSaved = (uint16)pPoly->m_nSaved08;
	pSaved = &m_SavedVerts.GetArray()[pPoly->m_nSaved08 >> 16];

	if (pPoly->m_wClip14 & 0x3f)
	{
		pOut = verts;
		for (i = 0; i < nVerts; i++)
		{
			*(LTVector*)pOut = *pSrc[i].m_pPos;
			(this->*pfnColor)(&pSrc[i], pOut);
			pfnFog(pOut, *(uint8* volatile*)&pOut + 0x14);  // shipped loads pOut twice here
			(this->*m_pVertFn7b8)(&pSrc[i], pOut + 0x18, 0);
			pOut += nStride;
		}

		pOut = verts;
		if (g_1008975c[pPass->m_nStages1c](pPoly->m_wClip14 & 0x3f, &pOut, &nVerts))
		{
			if (nSaved != (uint32)nVerts)
			{
				sub_10021d10("DrawPolyMgr::DrawPolyAdditionalPass: nQVerts != nVertices", pPass);
				return;
			}

			pV = verts;
			for (i = 0; i < nVerts; i++)
			{
				*(LTVector*)pV = pSaved->m_Pos;
				((uint32*)pV)[3] = pSaved->m_n0c;
				pV += nStride;
				pSaved++;
			}
		}
		else
			return;
	}
	else
	{
		pOut = verts;
		pV   = verts;
		for (i = 0; i < nVerts; i++)
		{
			*(LTVector*)pV = pSaved->m_Pos;
			((uint32*)pV)[3] = pSaved->m_n0c;
			pS = &pSrc[i];
			(this->*pfnColor)(pS, pV);
			pfnFog(pSrc[i].m_pPos, pV + 0x14);
			(this->*m_pVertFn7b8)(pS, pV + 0x18, 0);
			pSaved++;
			pV += nStride;
		}
	}

	for (iStage = 0; iStage < pPass->m_nStages1c; iStage++)
		(this->*((DrawFn589c4*)g_10089728)[pPass->m_Stages20[iStage].m_nDrawFn1c])(pPoly, iStage, 0);

	g_pD3DDevice->lpVtbl->DrawPrimitive(g_pD3DDevice, 6,
	                                  g_10089788[pPass->m_nStages1c],
	                                  pOut, nVerts, 0);

	if (nNextPass)
		sub_10056ab0(pPoly, nNextPass);
}
