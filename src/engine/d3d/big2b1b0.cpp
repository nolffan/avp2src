// *********************************************************************** //
//
//  MODULE:   BIG_2B1B0 (.CPP)
//
//  PURPOSE:  One function -- 0x1002b1b0, 7,568 bytes.  d3d.ren's
//            DRAW-ONE-CLIPPED-LINE primitive: given two world-space
//            endpoints BY VALUE and a D3DCOLOR for each, it transforms
//            both through the global clip matrix at 0x1008b9dc (an
//            out-of-line call to ltmatrix.h's MatVMul_H at 0x10023230),
//            stamps the two diffuse colours and a white specular on two
//            32-byte FVF-0x1c4 vertices, clips the segment against the
//            six frustum planes -- Cohen-Sutherland on both endpoints,
//            interpolating position, one uv pair and five colour bytes at
//            every crossing -- projects the survivors to screen with the
//            six globals at 0x1008bb9c..0x1008bbb0, and issues ONE
//            DrawPrimitive(D3DPT_LINELIST, 0x1c4, &v[0], 2, 0).  If both
//            endpoints fall outside any one plane it returns having drawn
//            nothing.
//
//            It is the STANDALONE form of a clipper this project has now
//            decoded three times inside other functions: big40e10.cpp
//            (0x10040e10, the LINESYSTEM pass), big44ff4.cpp (0x10044ff4,
//            the wireframe box) and big051a0.cpp (0x100051a0, the polygon
//            version).  Every plane predicate, every numerator and
//            denominator and the whole LERP field order below were
//            checked against those three operand for operand.
//
//  Compiler: Intel C/C++ 4.0, /O2.  There is NO /Qip two-entry pair here
//            -- no `mov 0x4(%esp),%eax` stack entry above the prologue,
//            just thirteen bytes of padding -- and all four arguments are
//            read off the frame from ebp+8 upward with a bare `ret`, so
//            the convention is plain __cdecl.  (Two of the four arguments
//            are by-value twelve-byte structs, which cannot travel in
//            registers, so this fixes the CONVENTION and is not by itself
//            evidence about the whole TU's flags.)  The /O2 tells:
//              * every outgoing argument block is `add $-N,%esp` followed
//                by `mov %reg,K(%esp)`; there is not one argument `push`
//                in 7.5 KB (0x1002b1b9 + 0x1002b1c5/0x1002b1c8/0x1002b1cc
//                for the two MatVMul_H calls, 0x1002c04b + 0x1002c143..
//                0x1002c15a for the DrawPrimitive);
//              * ONE epilogue, 0x1002c167..0x1002c16a, three
//                instructions, reached by all seven exits;
//              * the SIX `else` arms -- the clip-the-FIRST-endpoint arms
//                -- are all sunk below that epilogue, to
//                0x1002c16b..0x1002cf3b, and every one of them jumps
//                BACKWARDS into the body.
//
// ----------------------------------------------------------------------- //
//  MEASURED, STILL ATTEMPTED.
//    * The sixty conversions go through RoundF (the inlined __asm helper):
//      the shipped `fstps ; flds ; fistpl` and its 32-bit slots.  With it
//      the body is 7,619 bytes against the shipped 7,563 (4,917 before),
//      the frame's x87 spills appear at double width as shipped, and no
//      double local is needed -- fT/fVal are float, so the /Qsfalign8
//      realign stub (14 bytes, shipped has none) is gone at plain /O2.
//    * Plane 1's tests are spelled with the near-plane global on the left
//      (it is loaded first), the six inline-arm inner tests are positive
//      (`if (v1 out)`, falling into the arm as shipped), and the six sunk
//      arms `goto done` -- ONE epilogue as shipped, not seven.
//    * It cannot PAIR whatever the bytes: the function enumeration
//      splits the row at 0x1002c30a (a bogus call target inside the sunk
//      arms, an "interior fildl"), so the row is 4,442
//      bytes and a whole-function compile is never within 15% of it.
//      Only byte-identity scores it.  Residual: x87 operand order and
//      spill-slot reuse in the arms (6,484 bytes differ, the same
//      instruction count to within 4%).
// ----------------------------------------------------------------------- //
//  The missing 8 frame bytes, localised.  The shipped
//  frame has one more 8-byte, two-use slot than ours (-0x30: plane 2's
//  inner test spills v[1].x with `fstpl` before `jb`); every other x87
//  slot matches in order and use count.  Ours keeps v[1].x on the x87
//  stack into the arm (`jae arm ; fstp %st(0) ; jmp plane3`).  Plane 5,
//  the same CFG and the same spelling, spills as shipped.  Deleting PLANE 1
//  makes plane 2 spill (diagnostic only), deleting either of plane 1's
//  lerps does not, so it is plane 1's tests/doubles that change icl's
//  choice for plane 2.  Measured without effect on it: the inner test as
//  `v[1].x <= -v[1].z`, `!(v[1].x > -v[1].z)`, `!(-v[1].z < v[1].x)`,
//  `-v[1].z - v[1].x >= 0`, a `goto` past the arm; plane 1's tests as
//  `v[0].z >= near`, `near > v[1].z`, `v[1].z < near` in the sunk arm.
//  If the frame closes the row pairs (the opening is then 18+ bytes).
// ----------------------------------------------------------------------- //
//  THE EXTENT -- CONFIRMED AT BOTH ENDS.  THE ROW IS 8 BYTES TOO LONG.
// ----------------------------------------------------------------------- //
//
//  The enumeration carries `0x1002b1b0, 7,576 bytes`, which ends at
//  0x1002cf48.  The true extent is **0x1002b1b0..0x1002cf40, 7,568
//  bytes**.
//
//        0x1002b1b0..0x1002cf40  7,568 bytes  1577 instructions
//        ret               1        0x1002c16a
//        push %ebp frames  1        0x1002b1b0
//        branch targets    0 outside the range or mid-instruction
//        last instruction  ends flush at the stated end, BUT the final 5
//                          bytes are alignment padding -- the body ends
//                          at 0x1002cf3b
//
//  (a) THE START.  There is no /Qip register-entry defect and
//      the bytes agree -- there is no `mov 0x4(%esp),%eax` pair above the
//      prologue, only padding, and the previous function's `ret` is three
//      instructions up:
//
//        1002b198  movl $0x100234e0,0x1008ce8c   <- previous function
//        1002b1a2  ret                           <- its ONLY exit
//        1002b1a3  lea 0x0(%esi),%esi            <- 6 bytes of padding
//        1002b1a9  lea 0x0(%edi,%eiz,1),%edi     <- 7 more, 13 in all
//        1002b1b0  push %ebp                     <- our prologue, on a
//        1002b1b1  mov %esp,%ebp                    16-byte boundary
//        1002b1b3  sub $0x428,%esp
//
//      Thirteen bytes of lea-nop padding immediately above a `push %ebp`
//      on a 16-byte boundary is a function start, and the row's address
//      is right.
//
//  (b) THE END, AND WHY THE ROW IS EIGHT BYTES LONG.
//
//        1002cf36  jmp 0x1002b472        <- plane 1's sunk arm rejoining
//                                           the body; the LAST instruction,
//                                           5 bytes, ending at 0x1002cf3b
//        1002cf3b  nop                   <- 1 byte
//        1002cf3c  lea 0x0(%esi,%eiz,1),%esi  <- 4 bytes; 5 of padding to
//                                           the 16-byte boundary
//        1002cf40  mov 0x4(%esp),%eax    <- the NEXT function's /Qip
//        1002cf44  mov 0x8(%esp),%edx       STACK entry, TWO argument
//        1002cf48  push %ebp                loads, register entry here
//        1002cf49  mov %esp,%ebp
//
//      The row's stated 7,576 bytes end at 0x1002cf48 -- eight bytes past
//      0x1002cf40, i.e. exactly across the next function's two-argument
//      /Qip stack entry, which is the mirror of the nine rows the sweep
//      found addressed AT a register entry.  7,576 - 8 = 7,568 and
//      0x1002cf40 - 0x1002b1b0 = 0x1d90 = 7,568.  Nothing in .text
//      branches into 0x1002cf3b..0x1002cf3f.
//
//  ONE `ret` and ONE prologue over the corrected range, with 3,536 bytes
//  of code after the `ret` -- all six sunk `else` arms, each reached only
//  by a forward branch from above the `ret` and each ending in a `jmp`
//  back above it (0x1002c390 -> 0x1002c00d, 0x1002c5ea -> 0x1002bdcd,
//  0x1002c83c -> 0x1002bb63, 0x1002ca8e -> 0x1002b918, 0x1002cce8 ->
//  0x1002b6cd, 0x1002cf36 -> 0x1002b472), and every one of them using
//  THIS frame.  There is no second prologue and no padded-then-prologue
//  boundary anywhere inside.
//
// ----------------------------------------------------------------------- //
//  THE SIGNATURE, AND THE CALL SITES IT WAS READ FROM
// ----------------------------------------------------------------------- //
//
//      void sub_1002b1b0(LTVec2b1b0 vStart, LTVec2b1b0 vEnd,
//                        uint32 nColor0, uint32 nColor1);
//
//  Thirty-two bytes of arguments, all four read off the frame:
//
//      1002b1c2  lea 0x8(%ebp),%edx     -> MatVMul_H's pSrc for v[0]
//      1002b1de  lea 0x14(%ebp),%edx    -> MatVMul_H's pSrc for v[1]
//      1002b1db  mov 0x20(%ebp),%eax ; mov %eax,-0x224(%ebp)
//      1002b20d  mov 0x24(%ebp),%eax ; mov %eax,-0x204(%ebp)
//
//  ebp+8 and ebp+0x14 have their ADDRESSES taken and are handed to
//  MatVMul_H as `LTVector*`, so they are twelve-byte BY-VALUE vectors,
//  not pointers (a pointer parameter would be `mov 0x8(%ebp),%edx`).
//  ebp+0x20 and ebp+0x24 go straight into the two vertices' +0x10
//  diffuse dwords.  `ret` with no immediate and no callee pop: __cdecl.
//  Return type `void` -- the six reject paths jump to the epilogue at
//  0x1002c167 with nothing in %eax, and the draw path leaves
//  DrawPrimitive's HRESULT there by accident.
//
//  TWELVE call sites, ALL of them in ONE caller, 0x1002dd60 -- the
//  wireframe-box drawer, 4 turns x 3 edges:
//
//      1002ddd8  1002de36  1002de95  1002df0b  1002df81  1002dff7
//      1002e06e  1002e0e4  1002e15a  1002e1d1  1002e248  1002e2be
//
//  and every one of them writes the same 32 bytes.  0x1002ddd8's block is
//  the model:
//
//      1002dda2  mov %esi,0x30(%esp)         ; a corner into a scratch
//      1002dd9e  lea 0x20(%esp),%edx         ; the first point
//      1002dd9a  mov %esp,%eax               ; the argument slot
//      1002dda8  mov (%edx),%ecx ; mov %ecx,(%eax)        ; three dwords
//      1002ddac  mov 0x4(%edx),%ecx ; mov %ecx,0x4(%eax)  ; -- a
//      1002ddb2  mov 0x8(%edx),%edx ; mov %edx,0x8(%eax)  ; BY-VALUE copy
//      1002ddb8  lea 0xc(%esp),%eax          ; the second argument slot
//      1002ddbc  lea 0x30(%esp),%edx         ; the second point
//      1002ddc0  ...                         ; three more dwords
//      1002ddd0  mov %esi,0x18(%esp)         ; nColor0
//      1002ddd4  mov %esi,0x1c(%esp)         ; nColor1  -- the SAME value
//      1002ddd8  call 0x1002b1b0
//
//  All twelve pass the same colour twice, but the callee reads the two
//  slots separately into the two vertices, so they are two parameters.
//
// ----------------------------------------------------------------------- //
//  THE ACCEPTANCE TEST, AND HOW LITTLE OF THE FUNCTION IT SEES
// ----------------------------------------------------------------------- //
//
//      7,568 bytes, 3 calls (2 direct, 1 indirect), 2523 bytes per call
//
//  THREE calls in 7.5 KB, and **ONE distinct direct callee**:
//
//      1002b1d4   call 0x10023230        ; MatVMul_H, v[0]
//      1002b206   call 0x10023230        ; MatVMul_H, v[1]   -- same target
//      1002c164   call *0x64(%ecx)       ; DrawPrimitive, indirect
//
//  That is not merely the weak band, it is nearly the
//  vacuous one: two calls to the same function and one indirect.  A clean
//  3 of 3 discriminates almost nothing, and everything below the header
//  is carried by the other four instruments.
//
//  EXPECTED: 3 shipped / 3 ours, 3 in agreement (2 by resolved target,
//  1 by position only -- the DrawPrimitive through *0x64), 0 shipped-only,
//  0 ours-only.
//
//  NO _chkstk: the frame is 0x428 = 1,064 bytes, far under the probe
//  threshold, so the benign large-frame residual does not apply
//  and there is nothing for /Ge to close.  Do NOT add it.
//
//  AND IT DOES NOT NEED /Oi-.  There is no CRT-intrinsic shape anywhere
//  in the 7,568 bytes: the census below has no `fsqrt`, no `fabs`, no
//  `push %ecx ; push %ecx ; fstpl (%esp)` double-argument call, and no
//  `rep movsl` or `rep stos` of any kind -- the two by-value LTVector
//  copies happen in the CALLER, not here, and both vertices are filled
//  field by field.  /Oi- therefore has nothing to win, and the general
//  risk it carries (turning open-coded block copies into ours-only
//  `memcpy` rows) has nothing to act on either.  Leave the flag off.
//
//  WHAT THE CHECK ACTUALLY WITNESSES.  Taking the call-bearing blocks
//  generously -- the whole head down to the first plane test, and the
//  whole DrawPrimitive argument block --
//
//      0x1002b1b0..0x1002b223    115 B   both direct calls
//      0x1002c13d..0x1002c167     42 B   the indirect call
//
//  that is **157 bytes, 2.1% of the function**.  The two call-free
//  stretches are
//
//      0x1002b223..0x1002c13d   3,866 B  51.1%   the six plane tests and
//                                                the six INLINE clip arms
//                                                plus the screen transform
//      0x1002c167..0x1002cf3b   3,540 B  46.8%   the epilogue and all six
//                                                SUNK clip arms
//
//  -- **7,406 bytes, 97.9%, containing not one call of any kind.**  Quote
//  the 3 of 3 only beside that number.
//
// ----------------------------------------------------------------------- //
//  INSTRUMENT 1 -- THE FRAME DECOMPOSES EXACTLY, SLOTS DUMPED NOT ASSUMED
// ----------------------------------------------------------------------- //
//  `sub $0x428,%esp` at 0x1002b1b3 is 1,064 bytes.  Every ebp-relative
//  displacement in the function was extracted
//  (`grep -oE '\-0x[0-9a-f]+\(%ebp\)'`, 190 distinct) and every one falls
//  in one of the five blocks below; nothing is left over and nothing is
//  assumed.
//
//   ebp-0x100 .. -0x001   256   THIRTY-TWO 8-byte x87 spill slots.
//                               TWENTY-NINE are used, -0x8 down to -0xe8,
//                               contiguous; -0xf0 and -0xf8 are never
//                               referenced; -0x100 is the int<->float
//                               scratch DWORD (see below), which occupies
//                               the low half of the last slot.
//
//                               These are icl's x87 spills, not source
//                               locals -- proved by -0x20, which holds
//                               `(double)v[0].x` purely so plane 2's two
//                               tests and its two arms can share one
//                               load.  Every one is written with `fstl`
//                               or `fstpl` (64 bits) and read with
//                               `fldl`/`fsubl`/`faddl`/`fmull`: icl spills
//                               x87 temporaries at DOUBLE width, so their
//                               size says nothing about the source type.
//                               The complete map, each with the
//                               instruction that writes it:
//
//                                 -0x08  1002b238 + 10 more  (double)v[0].z
//                                 -0x10  1002b24f + 10 more  (double)v[1].z
//                                 -0x18  1002b231  (double)g_f1008b908
//                                 -0x20  1002b486  (double)v[0].x   P2 test
//                                 -0x28  1002b6da  (double)v[0].y   P3 test
//                                 -0x30  1002b4a3  (double)v[1].x   P2 test
//                                 -0x38  1002b925  (double)v[0].x   P4 test
//                                 -0x40  1002bddf  (double)g_f1008b910
//                                 -0x48  1002b6f1  (double)v[1].y   P3 test
//                                 -0x50  1002bb77  (double)v[0].y   P5 test
//                                 -0x58  1002b2b1  fT, plane 1 inline arm
//                                 -0x60  1002b93c  (double)v[1].x   P4 test
//                                 -0x68  1002b50c  fT, plane 2 inline arm
//                                 -0x70  1002bb94  (double)v[1].y   P5 test
//                                 -0x78  1002b75b  fT, plane 3 inline arm
//                                 -0x80  1002b9a6  fT, plane 4 inline arm
//                                 -0x88  1002bbfd  fT, plane 5 inline arm
//                                 -0x90  1002be4f  fT, plane 6 inline arm
//                                 -0x98  1002ccfa  (double)v[1].z   P1 sunk
//                                 -0xa0  1002caa7  (double)v[1].x   P2 sunk
//                                 -0xa8  1002c84e  (double)v[1].y   P3 sunk
//                                 -0xb0  1002cd66  fT, plane 1 sunk arm
//                                 -0xb8  1002c5fc  (double)v[1].x   P4 sunk
//                                 -0xc0  1002cb1a  fT, plane 2 sunk arm
//                                 -0xc8  1002c3a9  (double)v[1].y   P5 sunk
//                                 -0xd0  1002c8c2  fT, plane 3 sunk arm
//                                 -0xd8  1002c670  fT, plane 4 sunk arm
//                                 -0xe0  1002c41c  fT, plane 5 sunk arm
//                                 -0xe8  1002c1d2  fT, plane 6 sunk arm
//
//                               TWELVE of them are the interpolant, one
//                               per arm, and each is written by the ONLY
//                               `fstl` in its arm; that is the row that
//                               says there are exactly twelve arms.
//
//   ebp-0x100             (4)   THE INT<->FLOAT SCRATCH.  Every one of
//                               the 120 `movzbl` byte reads is followed by
//                               `mov %reg,-0x100(%ebp) ; fildl
//                               -0x100(%ebp)`.  120 stores and 120
//                               `fildl`, all at this ONE address --
//                               icl reuses a single dword for every
//                               integer-to-x87 hand-off.
//
//   ebp-0x1f4 .. -0x105   240   SIXTY 4-byte `fistpl` destinations, 4
//                               apart, TWELVE GROUPS OF FIVE, and the
//                               groups descend in SOURCE order:
//                                 P1 inline  -0x1f4 -0x1f0 -0x1ec -0x1e8 -0x1e4
//                                 P1 sunk    -0x1e0 -0x1dc -0x1d8 -0x1d4 -0x1d0
//                                 P2 inline  -0x1cc -0x1c8 -0x1c4 -0x1c0 -0x1bc
//                                 P2 sunk    -0x1b8 -0x1b4 -0x1b0 -0x1ac -0x1a8
//                                 P3 inline  -0x1a4 -0x1a0 -0x19c -0x198 -0x194
//                                 P3 sunk    -0x190 -0x18c -0x188 -0x184 -0x180
//                                 P4 inline  -0x17c -0x178 -0x174 -0x170 -0x16c
//                                 P4 sunk    -0x168 -0x164 -0x160 -0x15c -0x158
//                                 P5 inline  -0x154 -0x150 -0x14c -0x148 -0x144
//                                 P5 sunk    -0x140 -0x13c -0x138 -0x134 -0x130
//                                 P6 inline  -0x12c -0x128 -0x124 -0x120 -0x11c
//                                 P6 sunk    -0x118 -0x114 -0x110 -0x10c -0x108
//                               -0x104 is the 4-byte pad above the
//                               8-aligned scratch slot.
//
//   ebp-0x214 .. -0x1f5    32   v[1], the SECOND draw vertex.
//   ebp-0x234 .. -0x215    32   v[0], the FIRST.  `lea -0x234(%ebp),%edx`
//                               at 0x1002c053 is the pointer handed to
//                               DrawPrimitive with a count of 2, which is
//                               what makes these ONE `Vert2b1b0 v[2]` and
//                               fixes the array bound at two.
//                               Field offsets, all read off the bytes:
//                                 +0x00 x   -0x234 / -0x214
//                                 +0x04 y   -0x230 / -0x210
//                                 +0x08 z   -0x22c / -0x20c
//                                 +0x0c rhw -0x228 / -0x208  (`fsts` at
//                                           0x1002c059 / 0x1002c0c9)
//                                 +0x10 diffuse  -0x224 / -0x204, read
//                                           BYTEWISE at +0x10/+0x11/
//                                           +0x12/+0x13
//                                 +0x14 specular -0x220 / -0x200, only
//                                           byte +0x17 (-0x21d / -0x1fd)
//                                           ever interpolated
//                                 +0x18 tu  -0x21c / -0x1fc
//                                 +0x1c tv  -0x218 / -0x1f8
//                               0x20 bytes, and FVF 0x1c4 at 0x1002c15a
//                               is XYZRHW|DIFFUSE|SPECULAR|TEX1 = exactly
//                               those eight fields.
//
//   ebp-0x410 .. -0x235   476   SIXTY float temps, 8 apart with only the
//                               low 4 bytes live (`fstps X ; flds X`),
//                               again TWELVE GROUPS OF FIVE, and this
//                               block runs the OTHER way -- ASCENDING in
//                               source order:
//                                 P1 inline  -0x410 -0x408 -0x400 -0x3f8 -0x3f0
//                                 P1 sunk    -0x3e8 -0x3e0 -0x3d8 -0x3d0 -0x3c8
//                                 P2 inline  -0x3c0 -0x3b8 -0x3b0 -0x3a8 -0x3a0
//                                 P2 sunk    -0x398 -0x390 -0x388 -0x380 -0x378
//                                 P3 inline  -0x370 -0x368 -0x360 -0x358 -0x350
//                                 P3 sunk    -0x348 -0x340 -0x338 -0x330 -0x328
//                                 P4 inline  -0x320 -0x318 -0x310 -0x308 -0x300
//                                 P4 sunk    -0x2f8 -0x2f0 -0x2e8 -0x2e0 -0x2d8
//                                 P5 inline  -0x2d0 -0x2c8 -0x2c0 -0x2b8 -0x2b0
//                                 P5 sunk    -0x2a8 -0x2a0 -0x298 -0x290 -0x288
//                                 P6 inline  -0x280 -0x278 -0x270 -0x268 -0x260
//                                 P6 sunk    -0x258 -0x250 -0x248 -0x240 -0x238
//                               The topmost slot's tail is clipped by
//                               v[0] at -0x234, hence 476 rather than 480.
//
//   ebp-0x428 .. -0x411    24   never referenced -- alignment slack.  The
//                               function uses no callee-saved register at
//                               all (a whole-function register scan finds
//                               only %eax, %ecx, %edx, %ebp and %esp), so
//                               unlike its siblings there is no
//                               %ebx/%esi/%edi save block down here.
//
//     476 + 32 + 32 + 240 + 256 + 24 = 1,064 = 0x428, exactly.
//
//  TWO INDEPENDENT BOUNDS COME OFF THAT FRAME.  The vertex array is TWO,
//  from `lea -0x234(%ebp)` plus the count 2 at 0x1002c143 and the 32-byte
//  gap to v[1].  And the number of clip ARMS is TWELVE, from three
//  independent counts that all agree: 12 groups of 5 int slots, 12 groups
//  of 5 float temps, and 12 `fstl` interpolant spills.
//
//  Two outgoing argument blocks sit below the frame and are NOT part of
//  the 0x428:
//      0x1002b1b9  add $0xfffffff0,%esp   16 bytes, the two MatVMul_H
//                                         calls' three arguments, released
//                                         at 0x1002b210 by `add $0x10,%esp`
//      0x1002c04b  add $0xffffffe8,%esp   24 bytes, DrawPrimitive's six,
//                                         never released -- the epilogue is
//                                         `mov %ebp,%esp`
//
// ----------------------------------------------------------------------- //
//  INSTRUMENT 2 -- THE INSTRUCTION CENSUS, WITH NO RESIDUE
// ----------------------------------------------------------------------- //
//  **1,575 instructions** in 0x1002b1b0..0x1002cf3b (there are
//  1,577 over the padded range; the two extra are the `nop` and the
//  `lea`-nop at 0x1002cf3b/0x1002cf3c).  Every row below is decomposed BY
//  OPERAND, and the column sums to 1,575 exactly.
//
//  The three multipliers used throughout: 12 = the clip arms (6 planes x
//  2 endpoints), 5 = the interpolated colour bytes per arm, 4 = the
//  interpolated floats per arm (two positions + tu + tv).
//
//    mnemonic   count   decomposition
//    ---------  -----   -----------------------------------------------
//    mov          265   248 in the twelve arms + 9 head + 7 draw + 1
//                       epilogue.  Per arm it is 20, EXCEPT planes 1 and
//                       6 which are 22 -- their two extra are the integer
//                       copy of the plane constant into the pinned z
//                       (`mov 0x1008b908,%ecx ; mov %ecx,-0x20c(%ebp)` at
//                       0x1002b2e0/0x1002b2ec and five siblings).  Planes
//                       2..5 pin their coordinate through the FPU
//                       instead, so they have neither.  20*8 + 22*4 = 248.
//    flds         181   12 plane tests + 64 inline arms + 87 sunk arms +
//                       18 screen transform.  The inline arms are 11 each
//                       for planes 1..5 and 9 for plane 6 (plane 6 needs
//                       no z reload afterwards, being last); the sunk arms
//                       are 12 for plane 6 and 15 for planes 1..5.  The
//                       sunk arms are +3 or +4 on the inline ones for one
//                       reason: their destination IS their base, so
//                       v[0].y, v[0].x and v[0].tu have to be PRE-LOADED
//                       (0x1002c190, 0x1002c1a2, 0x1002c1b4) where the
//                       inline arms can fold the base into an `fadds`.
//    movzbl       120   12 arms x 5 bytes x 2 endpoints.  All ten
//                       displacements per arm dumped: v[1] at -0x202,
//                       -0x203, -0x204, -0x201, -0x1fd and v[0] at -0x222,
//                       -0x223, -0x224, -0x221, -0x21d.  That is
//                       diffuse+2, +1, +0, +3 then specular+3, i.e. the
//                       field order r, g, b, a, fog -- byte for byte the
//                       order big051a0.cpp, big40e10.cpp and big44ff4.cpp
//                       all read independently.
//    fildl        120   the same 120, one per `movzbl`, EVERY ONE from
//                       -0x100(%ebp).
//    fstps        114   12 arms x 9 (5 colour-byte float temps + 4 of the
//                       position/uv stores) + 6 in the screen transform
//                       (v[0].x/.y/.z, v[1].x/.y/.z).  108 + 6 = 114.
//    fxch          96   1 + 42 (inline arms: 6,8,7,7,8,6) + 19 (screen
//                       transform, which runs the x87 stack EIGHT deep at
//                       0x1002c043) + 34 (sunk arms: 5,6,6,6,6,5).
//    faddp         84   30 (inline arms, 5 each) + 48 (sunk arms, 8 each)
//                       + 6 (screen transform).  The sunk arms' extra
//                       three are the pre-loaded bases being added back
//                       instead of folded into `fadds`.
//    sub           61   60 + 1.  The 60 are `sub %edx,%eax` -- the colour
//                       difference taken in the INTEGER unit, once per
//                       byte lerp (12 x 5).  The 1 is `sub $0x428,%esp`.
//    fistpl        60   12 arms x 5.  All sixty destinations dumped above;
//                       they are sixty DISTINCT slots.
//    fsubs         48   12 arms x 4 -- the four float differences
//                       (two positions, tu, tv).
//    fmull         48   12 arms x 4 -- colour bytes 2..5 reload the
//                       interpolant from its double spill.  Byte 1 has it
//                       still in %st and uses `fmulp` instead, which is
//                       why this row is 48 and not 60.
//    fmul          48   12 arms x 4 -- the four float lerps, in register
//                       form (`fmul %st(4),%st`, `fmul %st,%st(3)`,
//                       `fmul %st,%st(2)`, `fmul %st(2),%st`).
//    fldl          41   11 (plane tests: 1,2,2,2,2,2) + 24 (inline arms,
//                       2 each) + 18 (sunk arms, 3 each -- the extra one
//                       is the fresh v[1] spill their own test made).
//    fstpl         36   22 + 14.  The 22 are the eleven `fstpl -0x10 ;
//                       fstpl -0x8` pairs that re-spill v[0].z and v[1].z:
//                       once in the head (0x1002b238/0x1002b24f) and once
//                       at the end of each of the TEN arms that are
//                       followed by another plane test -- planes 1..5
//                       inline and planes 1..5 sunk.  Plane 6's two arms
//                       have no such pair because nothing tests z again.
//                       The 14 are the single comparand spills -0x20,
//                       -0x30, -0x28, -0x48, -0x38, -0x60, -0x50, -0x70,
//                       -0x40, -0x98, -0xa0, -0xa8, -0xb8, -0xc8.
//    fadds         30   24 (inline arms, 4 each -- every base folded into
//                       the add) + 6 (sunk arms, 1 each -- only tv's base
//                       can be folded, the other three are pre-loaded).
//    fsubl         20   10 + 10.  Per arm: 1 for planes 1 and 6, 2 for
//                       planes 2..5, inline and sunk alike.  These are the
//                       denominator's and numerator's double-slot terms.
//    fmulp         18   12 (colour byte 1 in each arm) + 6 (the screen
//                       transform's six `* rhw`).
//    fchs          18   8 numerator negations (planes 2..5, both arms) +
//                       4 pinned-coordinate negations (planes 2 and 5,
//                       both arms) + 6 in the tests (planes 2 and 5 have
//                       `fldl ; fchs ; fcomps` twice each = 4, and their
//                       sunk tests once each = 2).
//    fnstsw        18   one per float compare.
//    sahf          18   ditto.
//    fcomp          9 |  the SAME 18 compares, by operand form:
//    fcomps         7 |  12 in the six plane tests (2 each) and 6 in the
//    fcompl         2 |  six sunk arms' own second test.  THERE ARE NO
//                       OTHERS -- in particular no |fDen| > 0.00001f
//                       epsilon guard, which the polygon clippers
//                       (big051a0, big5a960, big16324) all have and this
//                       family (big40e10, big44ff4) all lack.  18 compares
//                       is the row that proves the absence.
//    fstl          13   12 interpolant spills, one per arm, + 1 for
//                       (double)g_f1008b908 at 0x1002b231.
//    faddl         12   6 + 6.  Planes 2 and 5 use 2 each (denominator and
//                       numerator), planes 3 and 4 use 1 each, planes 1
//                       and 6 none -- inline and sunk alike.
//    fsts          10   8 + 2.  The 8 are the FIRST of the two stores of
//                       fVal in planes 2..5, both arms (a non-popping
//                       store because the value is used twice); planes 1
//                       and 6 pin z from a global by integer move and have
//                       none.  The 2 are v[0].rhw and v[1].rhw at
//                       0x1002c059 and 0x1002c0c9.
//    fmuls         10   the screen transform, 5 per vertex: g_f1008bb9c,
//                       g_f1008bba0, g_f1008bba4, g_f1008bba8,
//                       g_f1008bbac.
//    fdivp          8 |  FOURTEEN divides, and no more:
//    fdivrp         4 |  12 register-form, exactly one per clip arm, and 2
//    fdivs          2 |  `fdivs` for the two reciprocals in the screen
//                       transform.  (Mnemonics as objdump prints them --
//                       see the SWAPPED-MNEMONIC note below.)
//    fsubrl         8   4 + 4.  Planes 1, 3, 4 and 6 only, both arms:
//                       `fsubrl` computes mem - st0, which is how the
//                       denominator (v[1].z - v[0].z) and the numerator
//                       (v[0].y - v[0].z, v[0].x - v[0].z) come out.
//                       Planes 2 and 5 use `faddl` there instead.
//    jbe            6 |  EIGHTEEN conditional branches: 12 for the six
//    ja             6 |  planes' two tests each and 6 for the sunk arms'
//    jae            4 |  own second test.  Their senses are the six plane
//    jb             2 |  predicates and are tabulated below.
//    jmp            6   the six sunk arms rejoining the body.
//    movl           6   4 head (two matrix addresses, two 0xffffffff
//                       speculars) + 2 draw (the 0 flags and the FVF).
//    lea            5   4 head (&v[0], &vStart, &v[1], &vEnd) + 1 draw
//                       (&v[0] as the vertex pointer).
//    add            3   the two argument-block adjustments and the
//                       `add $0x10,%esp` release.
//    call           3   two direct, one indirect.
//    fstp           2   `fstp %st(0)` twice -- MatVMul_H's return value
//                       DISCARDED at 0x1002b1d9 and 0x1002b20b.  Its
//                       sibling big40e10.cpp stores that same return into
//                       v[i].m_fRHW; this function throws it away and
//                       recomputes 1.0f/z in the screen transform, which
//                       is why the pool has two 1.0f literals and not
//                       zero.
//    push           1 | the prologue and the epilogue.
//    pop            1 |
//    ret            1 |
//    ---------  -----
//    TOTAL      1,575
//
//  WHAT THE CENSUS FORBIDS.  A seventh plane would move fistpl (+5),
//  fildl (+10), movzbl (+10), sub (+5), fstl (+2), the compare rows (+3)
//  and the divide rows (+2) at once.  A missing arm would move the same
//  seven the other way.  A second uv pair would put fsubs at 72 and
//  fstps at 138 (that is precisely how big051a0.cpp's TEX2 vertex differs
//  from this TEX1 one).  A sixth colour byte would put movzbl at 144 and
//  fistpl at 72.  An epsilon guard on fDen would put the compare rows at
//  30 and add twelve sunk `fT = 0.0f` arms.  Not one of those is
//  compatible with the numbers above.
//
//  A NOTE ON THE SWAPPED MNEMONICS, VERIFIED RATHER THAN ASSUMED.
//  objdump prints binutils' AT&T-swapped names for the x87 DE E0/E8/F0/F8
//  register forms, and this function's twelve divides all use them.  The
//  encodings were checked by EXECUTING them with st(1)=6.0, st(0)=2.0:
//      de f9 -> 3.0      = st(1)/st(0)   -- Intel FDIVP;  objdump "fdivrp"
//      de f1 -> 0.333    = st(0)/st(1)   -- Intel FDIVRP; objdump "fdivp"
//      de e9 -> 4.0      = st(1)-st(0)   -- Intel FSUBP;  objdump "fsubrp"
//      de e1 -> -4.0     = st(0)-st(1)   -- Intel FSUBRP; objdump "fsubp"
//  So the four `fdivrp`-printed sites (0x1002b265, 0x1002be03, 0x1002c182,
//  0x1002cd16 -- planes 1 and 6, both arms) divide st(1) BY st(0), i.e.
//  numerator-first; and the eight `fdivp`-printed sites (0x1002b4c0,
//  0x1002b70f, 0x1002b95a, 0x1002bbb1, 0x1002c3cc, 0x1002c620, 0x1002c872,
//  0x1002caca -- planes 2..5) divide st(0) BY st(1), i.e.
//  denominator-first.  Both give num/den; getting this backwards would
//  invert all twelve interpolants.  big40e10.cpp records the same reading
//  for the same two encodings.
//
// ----------------------------------------------------------------------- //
//  INSTRUMENT 3 -- THE LITERAL POOL, AND IT DECOMPOSES TO THE BYTE
// ----------------------------------------------------------------------- //
//  This function references EXACTLY TWO .rdata constants in 7,568 bytes:
//
//      1002c00d  flds 0x100a617c
//      1002c037  flds 0x100a6184
//
//  Everything else it reads is a runtime global in .data (0x1008b908,
//  0x1008b910, 0x1008b9dc, 0x1008bb9c..0x1008bbb0, 0x1008d988) -- all of
//  which `Pe.va2off` cannot resolve, i.e. they are uninitialised, so they
//  hold no compile-time value and cannot be dumped.
//
//  BOUNDING THE RUN.  Sweeping all of .text for references into
//  0x100a6000..0x100a61ff: the last address the PREVIOUS function touches
//  is 0x100a615c (from 0x1002b161, which is inside 0x1002b1b0's
//  predecessor), and the lowest the NEXT function touches is 0x100a61a0
//  (from 0x1002d32c).  **This function's pool run is therefore
//  0x100a6160..0x100a619c -- sixteen dwords -- and every one of them is
//  0x3f800000, 1.0f**, dumped:
//
//      0x100a6160 0000803f    0x100a6180 0000803f
//      0x100a6164 0000803f    0x100a6184 0000803f  <- LOADED (2nd divide)
//      0x100a6168 0000803f    0x100a6188 0000803f
//      0x100a616c 0000803f    0x100a618c 0000803f
//      0x100a6170 0000803f    0x100a6190 0000803f
//      0x100a6174 0000803f    0x100a6194 0000803f
//      0x100a6178 0000803f    0x100a6198 0000803f
//      0x100a617c 0000803f    0x100a619c 0000803f
//                 ^ LOADED (1st divide)
//
//  (For the record: 0x100a615c above the run is 255.0f and 0x100a61a0
//  below it is 0.5f, so the run really is bounded by two different
//  values, not merely by two different functions.)
//
//  AND IT READS AS A TRANSCRIPT.  The dead-1.0f
//  anomaly is settled: icl emits one dead 1.0f per float DIVISION, immediately
//  before the live literal when the division has one.  This function has
//  fourteen divisions and two literal `1.0f /` numerators, so the
//  prediction is 14 + 2 = 16 slots -- which is what is there -- and the
//  two live slots must sit at the positions the two literal divisions
//  occupy in EMISSION order.  Numbering the run 1..16:
//
//      slots  1.. 6   dead, the SIX inline clip arms' divides, in
//                     emission order 0x1002b265, 0x1002b4c0, 0x1002b70f,
//                     0x1002b95a, 0x1002bbb1, 0x1002be03
//      slot   7       dead partner of the first reciprocal
//      slot   8       0x100a617c  -- LIVE, `1.0f / v[0].z` at 0x1002c00d
//      slot   9       dead partner of the second reciprocal
//      slot  10       0x100a6184  -- LIVE, `1.0f / v[1].z` at 0x1002c037
//      slots 11..16   dead, the SIX sunk clip arms' divides, in emission
//                     order 0x1002c182, 0x1002c3cc, 0x1002c620,
//                     0x1002c872, 0x1002caca, 0x1002cd16
//
//  The two live slots ARE the 8th and 10th of sixteen.  That is only
//  possible if exactly six divisions precede the screen transform and
//  exactly six follow it in emission order -- so the pool independently
//  proves (a) twelve clip arms, (b) one division each, (c) the two
//  reciprocals sitting between the two groups of six, and (d) the two
//  reciprocals being SEPARATE statements rather than one loop, because a
//  single source `1.0f` would have emitted one pair, not two.  Nothing
//  else in this function recovers any of that, and no frame slot or call
//  could have.
//
//  Note also what the pool does NOT contain: no -0.00001f and no 0.00001f
//  anywhere in the run.  That is the second, independent proof (beside
//  the 18-compare census row) that there is no epsilon guard on fDen.
//
//  ONE HONEST QUALIFICATION.  The probe behind that rule establishes the mechanism
//  for divisions whose numerator IS a 1.0f literal (three of those gave
//  six slots).  The reading above extends it to twelve divisions that
//  have no literal at all, on the arithmetic 14 + 2 = 16 and on the two
//  live slots landing at 8 and 10.  The fit is exact and there is no
//  competing account of sixteen 1.0f in a function whose only float
//  literal use is two reciprocals -- but the extension itself is an
//  inference from this function's numbers, not something the probe
//  measured.
//
// ----------------------------------------------------------------------- //
//  INSTRUMENT 4 -- EXPRESSION BY EXPRESSION AGAINST THREE DECODED SIBLINGS
// ----------------------------------------------------------------------- //
//  The same six-plane clipper has been decoded three times before, and
//  the plane ORDER agrees with all three:
//
//      z >= g_f1008b908 | -z < x | y < z | x < z | -z < y | z <= g_f1008b910
//        NEAR              LEFT     TOP     RIGHT   BOTTOM   FAR
//
//  (big051a0.cpp lines 321-322 for the polygon form; big44ff4.cpp line
//  ~155 and big40e10.cpp for the two-endpoint line form.)  Read off THIS
//  function's own bytes, plane by plane, with the branch that leaves the
//  inside path:
//
//    P1 0x1002b223  flds 0x1008b908 ; flds -0x22c ; fxch ; fcomp %st(1)
//                   ; ja 0x1002cced          -> K1 > v[0].z leaves
//                   => inside is  v[0].z >= g_f1008b908       NEAR
//    P2 0x1002b472  fldl -0x8 ; fchs ; fcomps -0x234 ; jae 0x1002ca93
//                   => inside is  -v[0].z <  v[0].x           LEFT
//    P3 0x1002b6cd  flds -0x230 ; fldl -0x8 ; fcomp %st(1) ; jbe 0x1002c841
//                   => inside is   v[0].y <  v[0].z           TOP
//    P4 0x1002b918  flds -0x234 ; fldl -0x8 ; fcomp %st(1) ; jbe 0x1002c5ef
//                   => inside is   v[0].x <  v[0].z           RIGHT
//    P5 0x1002bb63  fldl -0x8 ; fchs ; fcomps -0x230 ; jae 0x1002c395
//                   => inside is  -v[0].z <  v[0].y           BOTTOM
//    P6 0x1002bdcd  fldl -0x8 ; fcomps 0x1008b910 ; ja 0x1002c16b
//                   => inside is   v[0].z <= g_f1008b910      FAR
//
//  and the second test of each plane is the same predicate on v[1]
//  (0x1002b242, 0x1002b48f, 0x1002b6e4, 0x1002b92f, 0x1002bb80,
//  0x1002bde8), with the sunk arms' third test the same predicate on v[1]
//  again (0x1002cced, 0x1002ca93, 0x1002c841, 0x1002c5ef, 0x1002c395,
//  0x1002c16b).
//
//  THE TWELVE INTERPOLANTS, OPERAND FOR OPERAND.  Both arms of a plane
//  compute the SAME expression -- only the destination differs -- which
//  is itself a finding: the sunk arm never swaps the roles of v[0] and
//  v[1].  Denominator then numerator, with the double slots resolved
//  (-0x8 = v[0].z, -0x10 = v[1].z, -0x18 = K1, -0x20/-0x38 = v[0].x,
//  -0x28/-0x50 = v[0].y, -0x30/-0x60/-0xa0/-0xb8 = v[1].x,
//  -0x40 = K2, -0x48/-0x70/-0xa8/-0xc8 = v[1].y, -0x98 = v[1].z):
//
//    P1 1002b259  fldl K1 ; fsubl v0z          num = K1 - v0z
//                 fldl v0z ; fsubrl v1z        den = v1z - v0z
//                 de f9 = FDIVP                fT  = num / den
//    P2 1002b4ac  fldl v1x ; fsubl v0x ; faddl v1z ; fsubl v0z
//                                              den = (v1x-v0x) + v1z - v0z
//                 fldl v0z ; faddl v0x ; fchs  num = -(v0x + v0z)
//                 de f1 = FDIVRP               fT  = num / den
//    P3 1002b6fb  fldl v1y ; fsubl v0y ; fsubl v1z ; faddl v0z
//                                              den = (v1y-v0y) - v1z + v0z
//                 fldl v0z ; fsubrl v0y ; fchs num = -(v0y - v0z)
//    P4 1002b946  fldl v1x ; fsubl v0x ; fsubl v1z ; faddl v0z
//                                              den = (v1x-v0x) - v1z + v0z
//                 fldl v0z ; fsubrl v0x ; fchs num = -(v0x - v0z)
//    P5 1002bb9d  fldl v1y ; fsubl v0y ; faddl v1z ; fsubl v0z
//                                              den = (v1y-v0y) + v1z - v0z
//                 fldl v0z ; faddl v0y ; fchs  num = -(v0y + v0z)
//    P6 1002bdf7  fldl K2 ; fsubl v0z          num = K2 - v0z
//                 fldl v0z ; fsubrl v1z        den = v1z - v0z
//
//  and the six sunk arms (0x1002cd07, 0x1002cab3, 0x1002c85b, 0x1002c609,
//  0x1002c3b5, 0x1002c176) are instruction-for-instruction the same six,
//  differing only in which slot holds v[1]'s coordinate.  **All twelve
//  match big44ff4.cpp's twelve and big40e10.cpp's twelve exactly**, and
//  all six match big051a0.cpp's polygon numerators with `pPrev`->`v[0]`
//  and `pWalk`->`v[1]`.
//
//  THE PINNED COORDINATE, per plane, read off the store form:
//    P1, P6  an INTEGER move of the global: `mov 0x1008b908,%ecx ; mov
//            %ecx,-0x20c(%ebp)` (0x1002b2e0/0x1002b2ec) -- a plain float
//            lvalue copy, not an x87 store.  Same shape big051a0.cpp
//            records at 0x10005375.
//    P2      `fsts` z at 0x1002b51d, `fchs`, `fstps` x at 0x1002b52f:
//            z = fVal, x = -fVal.
//    P3      `fsts` z at 0x1002b78a, `fstps` y at 0x1002b790:
//            z = fVal, y = fVal.
//    P4      `fsts` z at 0x1002b9d5, `fstps` x at 0x1002b9db.
//    P5      `fsts` z at 0x1002bc11, `fchs`, `fstps` y at 0x1002bc23:
//            z = fVal, y = -fVal.
//  -- which is the LEFT/TOP/RIGHT/BOTTOM family of big44ff4.cpp coin-flip
//  (b) exactly, including that the shipped store order is z-first for all
//  four while the family source spelling puts the free component first.
//
//  THE LERP BODY.  Field order tu(+0x18), tv(+0x1c), then the bytes +0x12,
//  +0x11, +0x10, +0x13, +0x17 -- identical to big40e10.cpp's
//  CLIP40E10_LERP, big44ff4.cpp's LERP44FF4 and big051a0.cpp's
//  CLIP051A0_LERP with its second uv pair removed.  The colour difference
//  is taken in the INTEGER unit and only then widened
//  (`movzbl v1 ; movzbl v0 ; sub %edx,%eax ; mov -0x100 ; fildl`), and the
//  result goes back through a float temp and a bare `fistpl` with no
//  fnstcw/fldcw dance and no helper call -- at all sixty sites.
//
//  THE SCREEN TRANSFORM.  0x1002c00d..0x1002c137 is big5a960.cpp's
//  0x1005ca38 body and big40e10.cpp's CLIP40E10_SCREEN, unrolled over the
//  two vertices.  All six operands appear in the same roles:
//      x' = (x * g_f1008bb9c + z * g_f1008bba0) * rhw
//      y' = (y * g_f1008bba4 + z * g_f1008bba8) * rhw
//      z' = (z * g_f1008bbac +     g_f1008bbb0) * rhw
//  with `rhw = 1.0f / z` written to the vertex field and RELOADED --
//  v[0]: stored 0x1002c059, reloaded 0x1002c071 and 0x1002c07b; v[1]:
//  stored 0x1002c0c9, reloaded 0x1002c0d3 and 0x1002c0ef.  That is the
//  same store-once/reload-twice pattern big40e10.cpp documents at
//  0x10041e08/0x10041e20/0x10041e2a, so the source really does write the
//  field and read it back rather than keeping a local.
//
//  THE TRANSFORM CALL.  0x10023230 was disassembled in full
//  (0x10023230..0x100232af) and is ltmatrix.h:222 `MatVMul_H(pDest, pMat,
//  pSrc)`: `w = m[3][0]*x + m[3][1]*y + m[3][2]*z + m[3][3]`, then
//  `fdivrs 0x100a5c64` (dumped: 0x3f800000) for one_over_w, then rows 0,
//  1 and 2 each scaled by it and stored to (%ecx)/0x4/0x8, and it LEAVES
//  one_over_w in st(0) at the `ret` -- i.e. it returns the rhw.
//  big40e10.cpp declares that same address with that same signature and
//  stores its return into m_fRHW; this function discards it.
//
// ----------------------------------------------------------------------- //
//  REGION MAP.  Every one of the 7,568 bytes is in exactly one row, and
//  the instruction counts sum to 1,575.
// ----------------------------------------------------------------------- //
//
//   0x1002b1b0..0x1002b223   115 B   25 i  HEAD.  Prologue, the 16-byte
//       argument block, MatVMul_H(&v[0], &g_Mat1008b9dc, &vStart), the
//       v[0] colour stores, MatVMul_H(&v[1], ..., &vEnd), the v[1] colour
//       stores, the block release.  BOTH direct calls.
//   0x1002b223..0x1002b259    54 B   16 i  PLANE 1 TEST (near).
//   0x1002b259..0x1002b472   537 B  108 i  PLANE 1 INLINE ARM  (clip v[1])
//   0x1002b472..0x1002b4ac    58 B   16 i  PLANE 2 TEST (left).
//   0x1002b4ac..0x1002b6cd   545 B  113 i  PLANE 2 INLINE ARM
//   0x1002b6cd..0x1002b6fb    46 B   14 i  PLANE 3 TEST (top).
//   0x1002b6fb..0x1002b918   541 B  111 i  PLANE 3 INLINE ARM
//   0x1002b918..0x1002b946    46 B   14 i  PLANE 4 TEST (right).
//   0x1002b946..0x1002bb63   541 B  111 i  PLANE 4 INLINE ARM
//   0x1002bb63..0x1002bb9d    58 B   16 i  PLANE 5 TEST (bottom).
//   0x1002bb9d..0x1002bdcd   560 B  113 i  PLANE 5 INLINE ARM
//   0x1002bdcd..0x1002bdf7    42 B   12 i  PLANE 6 TEST (far).
//   0x1002bdf7..0x1002c00d   534 B  104 i  PLANE 6 INLINE ARM
//   0x1002c00d..0x1002c167   346 B   81 i  SCREEN TRANSFORM AND THE DRAW.
//       Two reciprocals, six components, the 24-byte argument block and
//       DrawPrimitive(g_pD3DDevice, 2, 0x1c4, &v[0], 2, 0) through
//       vtable +0x64.  `mov $0x2,%eax` at 0x1002c04e serves BOTH the
//       primitive type (0x4(%esp)) and the vertex count (0x10(%esp)).
//   0x1002c167..0x1002c16b     4 B    3 i  THE ONE EPILOGUE.
//  -- everything below is sunk cold code, all of it `else` arms --
//   0x1002c16b..0x1002c395   554 B  112 i  PLANE 6 SUNK ARM  (clip v[0]),
//                                          jmp 0x1002c00d
//   0x1002c395..0x1002c5ef   602 B  123 i  PLANE 5 SUNK ARM, jmp 0x1002bdcd
//   0x1002c5ef..0x1002c841   594 B  121 i  PLANE 4 SUNK ARM, jmp 0x1002bb63
//   0x1002c841..0x1002ca93   594 B  121 i  PLANE 3 SUNK ARM, jmp 0x1002b918
//   0x1002ca93..0x1002cced   602 B  123 i  PLANE 2 SUNK ARM, jmp 0x1002b6cd
//   0x1002cced..0x1002cf3b   590 B  118 i  PLANE 1 SUNK ARM, jmp 0x1002b472
//   0x1002cf3b..0x1002cf40     5 B    2 i  alignment padding (not code).
//
//  Note the emission order of the sunk arms: SIX FIRST, i.e. plane 6's
//  arm is nearest the epilogue and plane 1's is furthest.  That is the
//  standard /O2 reversal, and it is what makes the literal pool's
//  slot-11..16 ordering (plane 6 first) the mirror of the frame's
//  slot ordering (plane 1 first).
//
// ----------------------------------------------------------------------- //
//  COIN-FLIPS -- both spellings stated, so one build settles each
// ----------------------------------------------------------------------- //
//
//  (a) THE OUTER IF/ELSE ARM ORDER IS *NOT* A COIN-FLIP, and it is the
//      one place the arm-order warning bites.  Written
//
//          if (INSIDE(v[0])) { if (!INSIDE(v[1])) clip v[1]; }
//          else              { if (!INSIDE(v[1])) return; clip v[0]; }
//
//      the else arm is the cold one and /O2 sinks it below the epilogue,
//      which is what shipped.  Written with the test negated
//
//          if (!INSIDE(v[0])) { ... } else { ... }
//
//      the two arms swap and the clip-v[1] arm becomes the sunk one.
//      Neither arm makes a call, so the call check cannot tell them apart --
//      but the sunk-block ADDRESSES can, and they are recorded above:
//      the six sunk arms write v[0]'s fields (-0x234, -0x230, -0x22c,
//      -0x21c, -0x218, -0x222, -0x223, -0x224, -0x221, -0x21d) and the
//      six inline arms write v[1]'s (-0x214, -0x210, -0x20c, -0x1fc,
//      -0x1f8, -0x202, -0x203, -0x204, -0x201, -0x1fd).  The spelling
//      below is the one that reproduces that.  This is also exactly
//      big40e10.cpp's spelling, whose sunk arms are its `else` arms too.
//
//  (b) `Vert2b1b0 v[2]` VERSUS TWO NAMED LOCALS.  Written as an array
//      because DrawPrimitive is handed `&v[0]` with a count of 2.  Two
//      adjacent locals `Vert2b1b0 v0, v1;` with `DrawPrimitive(..., &v0,
//      2, 0)` put them at the same two offsets and fold every subscript.
//      Source shape only.  (Same flip as big44ff4.cpp coin-flip (a).)
//
//  (c) THE LEFT AND BOTTOM NUMERATORS.  The bytes load the z spill first:
//      `fldl -0x8 ; faddl -0x20 ; fchs` at 0x1002b4b8 and `fldl -0x8 ;
//      faddl -0x50 ; fchs` at 0x1002bba9, i.e. the emitted order is
//      z then x (and z then y).  The rule that a sum emits its
//      terms backwards makes the source `-(v[0].x + v[0].z)` and
//      `-(v[0].y + v[0].z)`, which is also what big44ff4.cpp and
//      big051a0.cpp spell, and that is what is written below.  If those
//      two arms come out short, swap the operands.
//
//  (d) THE fVAL STORE ORDER IN PLANES 2..5.  The shipped order is z
//      first in all four (`fsts` on z, then `fstps` on x or y); the
//      family spelling below puts the free component first and then
//      fVal, which for LEFT and BOTTOM agrees with the shipped z-then-
//      other order and for TOP and RIGHT does not.  big44ff4.cpp hit the
//      identical mismatch on the identical planes and kept the family
//      order; kept here too, for comparability.  The alternative is to
//      write `v[1].z = (float)fVal; v[1].y = (float)fVal;` for TOP and
//      `v[1].z = (float)fVal; v[1].x = (float)fVal;` for RIGHT.
//
//  (e) fT AND fVAL DECLARED `double`.  Every one of the twelve
//      interpolants is spilled with `fstl` to an EIGHT-byte slot and
//      reloaded with `fmull` (0x1002b2b1/0x1002b350 and eleven
//      siblings), never rounded through a 4-byte slot, and fVal is
//      stored to the vertex with a non-popping `fsts` and negated at
//      full width.  A `float` local that never leaves an x87 register
//      emits the same instructions, though, so this is not decidable
//      from the bytes -- big40e10.cpp spells the same slots `float`.
//      Written `double` here, matching big44ff4.cpp coin-flip (d).
//      Likewise the twenty-nine 8-byte slots are icl allocating one temp
//      per site (as it does for the sixty `fistpl` destinations and the
//      sixty float temps) and are NOT twenty-nine source variables.
//
//  (f) THE COLOUR/SPECULAR STORE ORDER IN THE HEAD.  v[0] stores diffuse
//      (0x1002b1e1) then specular (0x1002b1ed); v[1] stores specular
//      (0x1002b213) then diffuse (0x1002b21d).  Written diffuse-then-
//      specular for both, because the second pair's inversion is the
//      scheduler hoisting `mov 0x24(%ebp),%eax` above the `add
//      $0x10,%esp`.  What is NOT a flip is that both pairs sit AFTER
//      their own MatVMul_H call: MatVMul_H takes `&v[i]` and may alias,
//      so icl could not have moved a store from before the call to after
//      it.  The head therefore really does alternate transform, colours,
//      transform, colours.
//
//  (g) THE CLIP BODY AS A MACRO.  The twelve arms are one macro expanded
//      twelve times, as in all three siblings.  A `static inline` helper
//      would risk an out-of-line copy and a call the shipped code does
//      not make; with only three calls that is not a risk worth taking.
//
// ----------------------------------------------------------------------- //
//  NOT PINNED
// ----------------------------------------------------------------------- //
//
//  (1) THE FUNCTION'S NAME AND THE SUBSYSTEM.  All twelve callers are in
//      one function, 0x1002dd60, which is not decoded; its shape (four
//      turns of three edges, 12 calls) says wireframe box, exactly like
//      big44ff4.cpp's 0x10044ff4, but nothing names either.  Left as
//      `sub_1002b1b0`.
//
//  (2) THE VALUES OF THE .data GLOBALS.  0x1008b908 (near), 0x1008b910
//      (far), 0x1008b9dc (the 4x4) and 0x1008bb9c..0x1008bbb0 (the six
//      screen-transform coefficients) are all uninitialised at link time
//      and hold no dumpable constant.  Their ROLES are pinned by the
//      instructions that read them and by five earlier decodes; their
//      values are runtime state.
//
//  (3) v[i].rhw, v[i].tu AND v[i].tv ARE NEVER INITIALISED before the
//      clipper reads them.  MatVMul_H writes only x/y/z, the head writes
//      only the two colour dwords, and the clip arms interpolate tu and
//      tv from whatever the frame happened to hold.  This is not a hole
//      in the decode -- it is what the shipped code does, and
//      big44ff4.cpp records the same thing at 0x1004526c ("+0x0c (rhw),
//      +0x14 (specular) and +0x18/+0x1c (u,v) are never initialised and
//      are read by the clipper anyway").  Reproduced faithfully; the
//      reconstruction deliberately does not add the initialisation a
//      careful programmer would.
//
//  (4) THE SIXTY `fistpl` SITES WILL DIFFER IN BYTES.  They are bare
//      `fstps temp ; flds temp ; fistpl n ; mov n,%cl` with no
//      fnstcw/fldcw pair, which is icl's `/Qrcd` (icl
//      4.0 has no /QIfist, the real option is /Qrcd,
//      and even with it a `fistpll` can survive where the shipped code
//      has a 32-bit `fistpl`).  `build/engine.sh` does not pass /Qrcd to
//      any d3d TU.  These sixty sites contain no call, so the acceptance
//      test is blind to them either way; the structure above is what is
//      being claimed, not the bytes.
//
//  UNDECODED: NONE.  Every one of the 7,568 bytes is in the region map
//  and every instruction is in the census.
//
// *********************************************************************** //

#include "ltbasedefs.h"


// ---------------------------------------------------------------------- //
//  The twelve-byte world-space point.  Declared locally rather than as
//  the SDK LTVector because the only thing done with it is having its
//  address handed to MatVMul_H; the SDK class would drag its inline
//  members in.
// ---------------------------------------------------------------------- //
struct LTVec2b1b0
{
	float	x;                                          // +0x00
	float	y;                                          // +0x04
	float	z;                                          // +0x08
};


// ---------------------------------------------------------------------- //
//  ltmatrix.h's LTMatrix, declared locally: only the sixteen floats
//  matter, and MatVMul_H reads them at +0x00..+0x0c (row 0), +0x10..+0x1c
//  (row 1), +0x20..+0x2c (row 2) and +0x30..+0x3c (row 3).
// ---------------------------------------------------------------------- //
struct LTMat2b1b0
{
	float	m[4][4];
};


// ---------------------------------------------------------------------- //
//  The 32-byte draw vertex.  FVF 0x1c4 = XYZRHW|DIFFUSE|SPECULAR|TEX1,
//  read off the DrawPrimitive at 0x1002c15a, and every offset below is
//  touched by this function.  Field-for-field identical to
//  big44ff4.cpp's Vert44ff4 and big40e10.cpp's Vert40e10.
// ---------------------------------------------------------------------- //
union VColor2b1b0
{
	uint32	m_n;
	uint8	m_b[4];
};

struct Vert2b1b0                                            // 0x20 bytes
{
	float		x;                                          // +0x00
	float		y;                                          // +0x04
	float		z;                                          // +0x08
	float		m_fRHW;                                     // +0x0c
	VColor2b1b0	m_Diffuse10;                                // +0x10
	VColor2b1b0	m_Specular14;                               // +0x14
	float		tu;                                         // +0x18
	float		tv;                                         // +0x1c
};


// ---------------------------------------------------------------------- //
//  IDirect3DDevice7 -- ONE slot used, +0x64 / 4 == 25 == DrawPrimitive.
//  big0ae20.cpp / big124a4.cpp / big44ff4.cpp all model the same slice.
// ---------------------------------------------------------------------- //
struct D3DDevice7;

struct D3DDevice7Vtbl
{
	uint8	m_pad_00[0x64];
	long (__stdcall* DrawPrimitive)(D3DDevice7*, uint32 dwType, uint32 dwFVF,
	                                void* lpVerts, uint32 dwCount,
	                                uint32 dwFlags);        // +0x64
};

struct D3DDevice7
{
	D3DDevice7Vtbl*	lpVtbl;
};

#define D3DPT_LINELIST                          2
#define D3DFVF_CLIPVERTEX                   0x1c4  // XYZRHW|DIFFUSE|SPEC|TEX1


// ---------------------------------------------------------------------- //
//  The .data globals.  All uninitialised in the image, so only their
//  roles are pinned -- see NOT PINNED (2).
// ---------------------------------------------------------------------- //
extern float		g_f1008b908;                    // 0x1008b908  near
extern float		g_f1008b910;                    // 0x1008b910  far
extern LTMat2b1b0	g_Mat1008b9dc;                  // 0x1008b9dc  clip 4x4

extern float		g_f1008bb9c;                    // 0x1008bb9c
extern float		g_f1008bba0;                    // 0x1008bba0
extern float		g_f1008bba4;                    // 0x1008bba4
extern float		g_f1008bba8;                    // 0x1008bba8
extern float		g_f1008bbac;                    // 0x1008bbac
extern float		g_f1008bbb0;                    // 0x1008bbb0

extern D3DDevice7*	g_pD3DDevice;                   // 0x1008d988


// ---------------------------------------------------------------------- //
//  0x10023230 -- ltmatrix.h:222 `MatVMul_H`, OUT OF LINE.  Disassembled
//  in full: it computes w from row 3, `fdivrs 0x100a5c64` (dumped
//  0x3f800000 = 1.0f) for one_over_w, scales rows 0..2 by it into
//  (%ecx)/+4/+8, and leaves one_over_w in st(0) at the `ret`, i.e. it
//  RETURNS the rhw.  big40e10.cpp declares the same address the same way.
//
//  It MUST have its own declaration here and MUST NOT be defined in this
//  TU: our SDK's ltmatrix.h defines the body inline, and an expansion
//  would cost BOTH of this function's direct calls -- which is two thirds
//  of the entire call sequence.  This is the rule "a tiny accessor
//  spelled as a field read makes no call at all", one step out.
// ---------------------------------------------------------------------- //
//  Named `sub_10023230` rather than `MatVMul_H` so that the call check can
//  resolve the target by name, and declared `extern "C"` with the same
//  linkage name big40e10.cpp uses for the same address (C linkage, so the
//  two declarations' differing struct types cannot clash at link).
// ---------------------------------------------------------------------- //
extern "C" float sub_10023230(LTVec2b1b0* pDest, LTMat2b1b0* pMat,
                              LTVec2b1b0* pSrc);            // 0x10023230


// ---------------------------------------------------------------------- //
//  The part of a clip step that is identical in all TWELVE arms: one
//  texture pair and five colour bytes.  A macro rather than a function so
//  that no out-of-line copy and no call can be emitted -- there is no
//  `call` anywhere in the 7,406 call-free bytes.
//
//  Field order tu(+0x18), tv(+0x1c), then +0x12, +0x11, +0x10, +0x13,
//  +0x17, read off the sixty `movzbl <v1 byte> ; movzbl <v0 byte> ; sub`
//  triples (plane 1's inline arm at 0x1002b297/0x1002b29e,
//  0x1002b328/0x1002b32f, 0x1002b373/0x1002b37a,
//  0x1002b3be/0x1002b3c5, 0x1002b409/0x1002b410) and the same in all
//  twelve.  The difference is taken in the INTEGER unit and only then
//  widened; the result goes back through a float temp and a bare
//  `fistpl` (`fstps -0x410(%ebp) ; flds -0x410(%ebp) ;
//  fistpl -0x1f4(%ebp) ; mov -0x1f4(%ebp),%cl ; mov %cl,-0x202(%ebp)` at
//  0x1002b316..0x1002b33e).
//
//  OUT may alias PREV: in the six sunk arms it is v[0] on both sides,
//  which is exactly what the shipped code does -- it reads v[0]'s byte
//  and then writes it.
// ---------------------------------------------------------------------- //
// Round to nearest in the current FPU mode (w1_10001000.cpp's helper;
// 0x10015190 out of line): icl inlines a function holding __asm as
// `fstps f ; flds f ; fistpl n ; mov n,%cl`.
inline int32 RoundF(float f)
{
	int32 n;
	__asm fld f
	__asm fistp n
	return n;
}

#define CLIP2B1B0_LERP(OUT, PREV, WALK)                                      \
	(OUT).tu = ((WALK).tu - (PREV).tu) * fT + (PREV).tu;                     \
	(OUT).tv = ((WALK).tv - (PREV).tv) * fT + (PREV).tv;                     \
	(OUT).m_Diffuse10.m_b[2] = (uint8)RoundF(                                \
	    ((WALK).m_Diffuse10.m_b[2] - (PREV).m_Diffuse10.m_b[2]) * fT         \
	    + (PREV).m_Diffuse10.m_b[2]);                                        \
	(OUT).m_Diffuse10.m_b[1] = (uint8)RoundF(                                \
	    ((WALK).m_Diffuse10.m_b[1] - (PREV).m_Diffuse10.m_b[1]) * fT         \
	    + (PREV).m_Diffuse10.m_b[1]);                                        \
	(OUT).m_Diffuse10.m_b[0] = (uint8)RoundF(                                \
	    ((WALK).m_Diffuse10.m_b[0] - (PREV).m_Diffuse10.m_b[0]) * fT         \
	    + (PREV).m_Diffuse10.m_b[0]);                                        \
	(OUT).m_Diffuse10.m_b[3] = (uint8)RoundF(                                \
	    ((WALK).m_Diffuse10.m_b[3] - (PREV).m_Diffuse10.m_b[3]) * fT         \
	    + (PREV).m_Diffuse10.m_b[3]);                                        \
	(OUT).m_Specular14.m_b[3] = (uint8)RoundF(                               \
	    ((WALK).m_Specular14.m_b[3] - (PREV).m_Specular14.m_b[3]) * fT       \
	    + (PREV).m_Specular14.m_b[3])


// ---------------------------------------------------------------------- //
//  The five float temporaries every arm needs.  Declared per arm, in an
//  inner block, so no live range spans two planes -- which is what the
//  frame says happened: icl gave every one of the sixty sites its own
//  slot rather than reusing five.
// ---------------------------------------------------------------------- //
#define CLIP2B1B0_TEMPS                                                      \
	float	fT


// ---------------------------------------------------------------------- //
//  The camera-space -> screen-space step, once per vertex.  Written in
//  big5a960.cpp's / big40e10.cpp's exact spelling.  `m_fRHW` is a genuine
//  memory round-trip here: stored with a non-popping `fsts` and reloaded
//  twice (v[0] 0x1002c059 / 0x1002c071 / 0x1002c07b; v[1] 0x1002c0c9 /
//  0x1002c0d3 / 0x1002c0ef), the third use coming from the register copy.
// ---------------------------------------------------------------------- //
#define CLIP2B1B0_SCREEN(V)                                                  \
	(V).m_fRHW = 1.0f / (V).z;                                               \
	(V).x      = ((V).x * g_f1008bb9c + (V).z * g_f1008bba0) * (V).m_fRHW;   \
	(V).y      = ((V).y * g_f1008bba4 + (V).z * g_f1008bba8) * (V).m_fRHW;   \
	(V).z      = ((V).z * g_f1008bbac +       g_f1008bbb0)   * (V).m_fRHW


// ======================================================================= //
//  0x1002b1b0 .. 0x1002cf40   (7,568 bytes, 3 calls, 97.9% call-free)
//
//  __cdecl, four stack arguments, void.  See THE SIGNATURE above.
// ======================================================================= //
void sub_1002b1b0(LTVec2b1b0 vStart, LTVec2b1b0 vEnd,
                  uint32 nColor0, uint32 nColor1)
{
	Vert2b1b0	v[2];                                   // -0x234, -0x214

	// ------------------------------------------------------------------ //
	//  0x1002b1b0..0x1002b222 -- THE HEAD.
	//
	//    1002b1bc  lea -0x234(%ebp),%eax        ; &v[0]
	//    1002b1c2  lea 0x8(%ebp),%edx           ; &vStart
	//    1002b1cc  movl $0x1008b9dc,0x4(%esp)   ; &g_Mat1008b9dc
	//    1002b1d4  call 0x10023230
	//    1002b1d9  fstp %st(0)                  ; the rhw, DISCARDED
	//    1002b1e1  mov %eax,-0x224(%ebp)        ; v[0].m_Diffuse10.m_n
	//    1002b1ed  movl $0xffffffff,-0x220(%ebp); v[0].m_Specular14.m_n
	//    ... and the same four for v[1] at 0x1002b1e7..0x1002b21d.
	//
	//  Neither call's return value is used, and nothing initialises
	//  m_fRHW, tu or tv -- see NOT PINNED (3).
	// ------------------------------------------------------------------ //
	sub_10023230((LTVec2b1b0*)&v[0], &g_Mat1008b9dc, &vStart);
	v[0].m_Diffuse10.m_n  = nColor0;
	v[0].m_Specular14.m_n = 0xffffffff;

	sub_10023230((LTVec2b1b0*)&v[1], &g_Mat1008b9dc, &vEnd);
	v[1].m_Diffuse10.m_n  = nColor1;
	v[1].m_Specular14.m_n = 0xffffffff;

	// ================================================================== //
	//  PLANE 1, NEAR:  z >= g_f1008b908
	//  test 0x1002b223 (fcomp/ja 0x1002cced, then fcomp/jbe 0x1002b472)
	//  inline arm 0x1002b259 | sunk test 0x1002cced (fcomp/ja 0x1002c167)
	//  sunk arm 0x1002cd07 (jmp 0x1002b472).
	//  Interpolant 0x1002b259..0x1002b265, `de f9` = Intel FDIVP.
	//  Pinned z by INTEGER move -- `mov 0x1008b908,%ecx ; mov %ecx,
	//  -0x20c(%ebp)` at 0x1002b2e0/0x1002b2ec in the inline arm and
	//  `mov %ecx,-0x22c(%ebp)` at 0x1002cd96/0x1002cda2 in the sunk one.
	// ================================================================== //
	if (g_f1008b908 <= v[0].z)
	{
		if (v[1].z < g_f1008b908)
		{
			CLIP2B1B0_TEMPS;

			fT = (g_f1008b908 - v[0].z) / (v[1].z - v[0].z);

			v[1].x = (float)((v[1].x - v[0].x) * fT + v[0].x);
			v[1].y = (float)((v[1].y - v[0].y) * fT + v[0].y);
			v[1].z = g_f1008b908;

			CLIP2B1B0_LERP(v[1], v[0], v[1]);
		}
	}
	else
	{
		CLIP2B1B0_TEMPS;

		if (!(g_f1008b908 <= v[1].z))
			goto done;

		fT = (g_f1008b908 - v[0].z) / (v[1].z - v[0].z);

		v[0].x = (float)((v[1].x - v[0].x) * fT + v[0].x);
		v[0].y = (float)((v[1].y - v[0].y) * fT + v[0].y);
		v[0].z = g_f1008b908;

		CLIP2B1B0_LERP(v[0], v[0], v[1]);
	}

	// ================================================================== //
	//  PLANE 2, LEFT:  -z < x
	//  test 0x1002b472 (fchs/fcomps/jae 0x1002ca93, then fchs/fcomps/jb
	//  0x1002b6cd) | inline arm 0x1002b4ac | sunk test 0x1002ca93
	//  (jae 0x1002c167) | sunk arm 0x1002cab3 (jmp 0x1002b6cd).
	//  Interpolant 0x1002b4ac..0x1002b4c0, `de f1` = Intel FDIVRP.
	//  `fsts` z at 0x1002b51d, `fchs`, `fstps` x at 0x1002b52f -- z is
	//  written first and x gets -fVal.
	// ================================================================== //
	if (-v[0].z < v[0].x)
	{
		if (-v[1].z >= v[1].x)
		{
			CLIP2B1B0_TEMPS;
			float	fVal;

			fT = -(v[0].x + v[0].z)
			   / ((v[1].x - v[0].x) + v[1].z - v[0].z);

			v[1].y = (float)((v[1].y - v[0].y) * fT + v[0].y);
			fVal   = (v[1].z - v[0].z) * fT + v[0].z;
			v[1].z = (float)fVal;
			v[1].x = (float)-fVal;

			CLIP2B1B0_LERP(v[1], v[0], v[1]);
		}
	}
	else
	{
		CLIP2B1B0_TEMPS;
		float	fVal;

		if (!(-v[1].z < v[1].x))
			goto done;

		fT = -(v[0].x + v[0].z)
		   / ((v[1].x - v[0].x) + v[1].z - v[0].z);

		v[0].y = (float)((v[1].y - v[0].y) * fT + v[0].y);
		fVal   = (v[1].z - v[0].z) * fT + v[0].z;
		v[0].z = (float)fVal;
		v[0].x = (float)-fVal;

		CLIP2B1B0_LERP(v[0], v[0], v[1]);
	}

	// ================================================================== //
	//  PLANE 3, TOP:  y < z
	//  test 0x1002b6cd (fcomp/jbe 0x1002c841, then fcomp/ja 0x1002b918)
	//  inline arm 0x1002b6fb | sunk test 0x1002c841 (jbe 0x1002c167)
	//  sunk arm 0x1002c85b (jmp 0x1002b918).
	//  Interpolant 0x1002b6fb..0x1002b70f, `de f1` = Intel FDIVRP, with
	//  `fsubrl -0x28` + `fchs` giving -(v[0].y - v[0].z).
	//  `fsts` z at 0x1002b78a, `fstps` y at 0x1002b790.
	// ================================================================== //
	if (v[0].y < v[0].z)
	{
		if (v[1].y >= v[1].z)
		{
			CLIP2B1B0_TEMPS;
			float	fVal;

			fT = -(v[0].y - v[0].z)
			   / ((v[1].y - v[0].y) - v[1].z + v[0].z);

			v[1].x = (float)((v[1].x - v[0].x) * fT + v[0].x);
			fVal   = (v[1].z - v[0].z) * fT + v[0].z;
			v[1].y = (float)fVal;
			v[1].z = (float)fVal;

			CLIP2B1B0_LERP(v[1], v[0], v[1]);
		}
	}
	else
	{
		CLIP2B1B0_TEMPS;
		float	fVal;

		if (!(v[1].y < v[1].z))
			goto done;

		fT = -(v[0].y - v[0].z)
		   / ((v[1].y - v[0].y) - v[1].z + v[0].z);

		v[0].x = (float)((v[1].x - v[0].x) * fT + v[0].x);
		fVal   = (v[1].z - v[0].z) * fT + v[0].z;
		v[0].y = (float)fVal;
		v[0].z = (float)fVal;

		CLIP2B1B0_LERP(v[0], v[0], v[1]);
	}

	// ================================================================== //
	//  PLANE 4, RIGHT:  x < z
	//  test 0x1002b918 (fcomp/jbe 0x1002c5ef, then fcomp/ja 0x1002bb63)
	//  inline arm 0x1002b946 | sunk test 0x1002c5ef (jbe 0x1002c167)
	//  sunk arm 0x1002c609 (jmp 0x1002bb63).
	//  Interpolant 0x1002b946..0x1002b95a, `de f1` = Intel FDIVRP.
	//  `fsts` z at 0x1002b9d5, `fstps` x at 0x1002b9db.
	// ================================================================== //
	if (v[0].x < v[0].z)
	{
		if (v[1].x >= v[1].z)
		{
			CLIP2B1B0_TEMPS;
			float	fVal;

			fT = -(v[0].x - v[0].z)
			   / ((v[1].x - v[0].x) - v[1].z + v[0].z);

			v[1].y = (float)((v[1].y - v[0].y) * fT + v[0].y);
			fVal   = (v[1].z - v[0].z) * fT + v[0].z;
			v[1].x = (float)fVal;
			v[1].z = (float)fVal;

			CLIP2B1B0_LERP(v[1], v[0], v[1]);
		}
	}
	else
	{
		CLIP2B1B0_TEMPS;
		float	fVal;

		if (!(v[1].x < v[1].z))
			goto done;

		fT = -(v[0].x - v[0].z)
		   / ((v[1].x - v[0].x) - v[1].z + v[0].z);

		v[0].y = (float)((v[1].y - v[0].y) * fT + v[0].y);
		fVal   = (v[1].z - v[0].z) * fT + v[0].z;
		v[0].x = (float)fVal;
		v[0].z = (float)fVal;

		CLIP2B1B0_LERP(v[0], v[0], v[1]);
	}

	// ================================================================== //
	//  PLANE 5, BOTTOM:  -z < y
	//  test 0x1002bb63 (fchs/fcomps/jae 0x1002c395, then fchs/fcomps/jb
	//  0x1002bdcd) | inline arm 0x1002bb9d | sunk test 0x1002c395
	//  (jae 0x1002c167) | sunk arm 0x1002c3b5 (jmp 0x1002bdcd).
	//  Interpolant 0x1002bb9d..0x1002bbb1, `de f1` = Intel FDIVRP.
	//  `fsts` z at 0x1002bc11, `fchs`, `fstps` y at 0x1002bc23.
	// ================================================================== //
	if (-v[0].z < v[0].y)
	{
		if (-v[1].z >= v[1].y)
		{
			CLIP2B1B0_TEMPS;
			float	fVal;

			fT = -(v[0].y + v[0].z)
			   / ((v[1].y - v[0].y) + v[1].z - v[0].z);

			v[1].x = (float)((v[1].x - v[0].x) * fT + v[0].x);
			fVal   = (v[1].z - v[0].z) * fT + v[0].z;
			v[1].z = (float)fVal;
			v[1].y = (float)-fVal;

			CLIP2B1B0_LERP(v[1], v[0], v[1]);
		}
	}
	else
	{
		CLIP2B1B0_TEMPS;
		float	fVal;

		if (!(-v[1].z < v[1].y))
			goto done;

		fT = -(v[0].y + v[0].z)
		   / ((v[1].y - v[0].y) + v[1].z - v[0].z);

		v[0].x = (float)((v[1].x - v[0].x) * fT + v[0].x);
		fVal   = (v[1].z - v[0].z) * fT + v[0].z;
		v[0].z = (float)fVal;
		v[0].y = (float)-fVal;

		CLIP2B1B0_LERP(v[0], v[0], v[1]);
	}

	// ================================================================== //
	//  PLANE 6, FAR:  z <= g_f1008b910
	//  test 0x1002bdcd (fcomps/ja 0x1002c16b, then fcompl/jbe 0x1002c00d)
	//  inline arm 0x1002bdf7 | sunk test 0x1002c16b (fcompl/ja
	//  0x1002c167) | sunk arm 0x1002c176 (jmp 0x1002c00d).
	//  Interpolant 0x1002bdf7..0x1002be03, `de f9` = Intel FDIVP.
	//  Pinned z by INTEGER move -- `mov 0x1008b910,%ecx ; mov %ecx,
	//  -0x20c(%ebp)` at 0x1002be81/0x1002be8d in the inline arm and
	//  `mov %ecx,-0x22c(%ebp)` at 0x1002c202/0x1002c20e in the sunk one.
	//  All FOUR integer plane-constant moves in the function are those,
	//  which is the row that says only planes 1 and 6 pin z to a global.
	//  Neither of plane 6's arms re-spills v[0].z / v[1].z afterwards --
	//  nothing tests z again -- which is why the `fstpl` row is 22 and
	//  not 26.
	// ================================================================== //
	if (v[0].z <= g_f1008b910)
	{
		if (v[1].z > g_f1008b910)
		{
			CLIP2B1B0_TEMPS;

			fT = (g_f1008b910 - v[0].z) / (v[1].z - v[0].z);

			v[1].x = (float)((v[1].x - v[0].x) * fT + v[0].x);
			v[1].y = (float)((v[1].y - v[0].y) * fT + v[0].y);
			v[1].z = g_f1008b910;

			CLIP2B1B0_LERP(v[1], v[0], v[1]);
		}
	}
	else
	{
		CLIP2B1B0_TEMPS;

		if (!(v[1].z <= g_f1008b910))
			goto done;

		fT = (g_f1008b910 - v[0].z) / (v[1].z - v[0].z);

		v[0].x = (float)((v[1].x - v[0].x) * fT + v[0].x);
		v[0].y = (float)((v[1].y - v[0].y) * fT + v[0].y);
		v[0].z = g_f1008b910;

		CLIP2B1B0_LERP(v[0], v[0], v[1]);
	}

	// ------------------------------------------------------------------ //
	//  0x1002c00d..0x1002c137 -- CAMERA SPACE TO SCREEN, both vertices.
	//  The two `1.0f` are the pool's slots 8 and 10 (0x100a617c and
	//  0x100a6184); two separate literals for two separate statements.
	// ------------------------------------------------------------------ //
	CLIP2B1B0_SCREEN(v[0]);
	CLIP2B1B0_SCREEN(v[1]);

	// ------------------------------------------------------------------ //
	//  0x1002c13d..0x1002c166 -- ONE DrawPrimitive, vtable +0x64, six
	//  dwords in a 24-byte block (`add $0xffffffe8,%esp` at 0x1002c04b).
	//  `mov $0x2,%eax` at 0x1002c04e feeds BOTH the type at 0x4(%esp) and
	//  the count at 0x10(%esp); `lea -0x234(%ebp),%edx` at 0x1002c053 is
	//  &v[0]; `movl $0x1c4,0x8(%esp)` is the FVF; `movl $0x0,0x14(%esp)`
	//  the flags.
	// ------------------------------------------------------------------ //
	g_pD3DDevice->lpVtbl->DrawPrimitive(g_pD3DDevice, D3DPT_LINELIST,
	                                    D3DFVF_CLIPVERTEX, &v[0], 2, 0);

done:
	;
}
