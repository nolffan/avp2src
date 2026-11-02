// *********************************************************************** //
//  READ THIS FIRST.  The clipper is now written as the
//  shipped code is built: d3dr3c_os.cpp's driver 0x1000f4b0 over the six
//  32-byte plane bodies 0x10001000 .. 0x10001fe0, all inlined (copied from
//  w55_10016324_os.cpp), with RoundF on the colour bytes, and the
//  vertex spelled with byte fields.  Measured (difflib over normalised
//  instructions): 1493 of 1660 shipped instructions matched (was 561 as
//  the macro-expanded passes).  LEFT: the frame is 0xc68 against 0xc78 --
//  the shipped build has 8 more bytes above the spill slots and 8 more in
//  the -0x140 region -- so the row still does not pair.  The long decode
//  below describes the earlier macro spelling; its readings still stand.
// *********************************************************************** //
// *********************************************************************** //
//
//  MODULE:   BIG_5DCA0 (.CPP)
//
//  PURPOSE:  ONE function -- 0x1005dca0 .. 0x1005f577, 6,359 bytes of
//            body plus 9 bytes of alignment padding to 0x1005f580.
//
//            d3d.ren's SKY-PORTAL SCREEN-EXTENT pass: it walks the frame's
//            sky-portal polygon list, projects every polygon into screen
//            space, runs the SAME six-plane homogeneous clipper that
//            big051a0.cpp / big5a960.cpp / big61a30.cpp / big124a4.cpp /
//            big6d910.cpp / big70d10.cpp all carry, and accumulates the
//            surviving vertices' screen x/y into the four globals
//            0x1008ff3c / 0x1008ff38 / 0x1008ff34 / 0x1008ff30.  It
//            returns LTTRUE when the accumulated rectangle is wider AND
//            taller than 0.9 pixels.
//
//            The clipper here is the 32-BYTE / ONE-TEXTURE-PAIR variant
//            -- big051a0.cpp section 3 predicted this shape exactly when
//            it described 0x10003c40, and every one of its predictions
//            (shl $0x5 / add $0x20 / sar $0x5 instead of the x40 lea
//            chain and the idiv; two texture lerps instead of four; the
//            SAME five byte lerps and six planes) is confirmed here by
//            the census below.
//
//  ------------------------------------------------------------------- //
//  1. THE EXTENT.  VERIFIED BEFORE A BYTE WAS DECODED.
//  ------------------------------------------------------------------- //
//
//  The listing row reads `0x1005dca0 .. 0x1005f580, 6,368 bytes,
//  4 direct calls / 3 distinct`.  It is CORRECT (it includes the
//  alignment padding); the function body ends at 0x1005f577.
//
//    * ONE `ret` in 0x1005dca0..0x1005f580, at 0x1005f409.  The next two
//      `ret`s in the file are 0x1005f5a8 and 0x1005f5b8, both ABOVE
//      0x1005f580 and both inside the next function.
//    * ONE `push %ebp ; mov %esp,%ebp`, at 0x1005dca0/0x1005dca1.  There
//      is no second prologue anywhere in the range -- a scan of the whole
//      disassembly for `push %ebp` found exactly one hit.
//    * The 365 bytes after the `ret` (0x1005f40a..0x1005f577) are /O2
//      cold-block sinking, not a second function, and every block was
//      proved individually.  All of them use THIS frame and all of them
//      branch BACK into 0x1005dca0..0x1005f409:
//        0x1005f40a  the `AllSkyPortals` arm of the opening if/else
//                    (`jmp 0x1005dd0d`);
//        0x1005f43b / 0x1005f460 / 0x1005f490 / 0x1005f4c0 / 0x1005f4f0 /
//        0x1005f520  the six clip-epsilon arms, in REVERSE pass order,
//                    each `jb` back to its pass's divide (0x1005dfb5,
//                    0x1005e30c, 0x1005e65e, 0x1005e9ac, 0x1005ecfe,
//                    0x1005f035) or `jmp` to its `fT = 0.0f`
//                    continuation (0x1005dfbf, 0x1005e314, 0x1005e666,
//                    0x1005e9b4, 0x1005ed06, 0x1005f03f);
//        0x1005f540  `xor %edx,%edx ; jmp 0x1005f3f9` -- the shared
//                    `return LTFALSE`, reached by BOTH final tests;
//        0x1005f547 / 0x1005f550 / 0x1005f560 / 0x1005f570  the four
//                    min/max "take the candidate" arms (`fstp %st(1)`
//                    then `jmp` to 0x1005f388 / 0x1005f369 / 0x1005f34a /
//                    0x1005f32b).
//    * EVERY branch target in 0x1005dca0..0x1005f580 lands INSIDE the
//      range and ON an instruction boundary.  Checked mechanically over
//      every jump in the range: 0 out-of-range, 0 mid-instruction.  (The
//      only out-of-range branches in the dump -- 0x1005f58d, 0x1005f5a0,
//      0x1005f5b0 -- are all at or above 0x1005f580, i.e. in the next
//      function.)
//    * 0x1005f572 is the last real instruction (`jmp 0x1005f32b`, ending
//      at 0x1005f577).  0x1005f577..0x1005f57f is `mov %esi,%esi` plus
//      `lea 0x0(%edi,%eiz,1),%edi` -- nine bytes of alignment padding.
//      0x1005f580 is the NEXT function and it has NO `push %ebp`: it opens
//      `sub $0x94,%esp` and is entirely %esp-relative, ending
//      `add $0x94,%esp ; ret`.  So 0x1005f580 is a true function start,
//      not a /Qip register entry (a register entry would sit 4 bytes into
//      a function whose stack entry we would see just above; there is no
//      such entry here).
//    * the call count over 0x1005dca0..0x1005f580
//      -> **6,368 bytes, 4 calls (4 direct, 0 indirect), 1592
//      bytes per call**, which is the row's own 4.  Over the trimmed
//      0x1005dca0..0x1005f577 it is 6,359 bytes and the SAME 4 calls, so
//      no call is lost to the padding and none belongs to a neighbour.
//      THE ROW DOES NOT SPLIT.
//
//  ------------------------------------------------------------------- //
//  2. THE SIGNATURE.
//  ------------------------------------------------------------------- //
//
//      LTBOOL sub_1005dca0(void);        /* __cdecl, no arguments */
//
//  Read off the ONE call site, 0x1005f5a9 inside the next function:
//
//      1005f593  mov  0x1008beb0,%eax
//      1005f598  mov  0xa0(%eax),%eax
//      1005f59e  test %eax,%eax
//      1005f5a0  jg   0x1005f5a9
//      1005f5a2  add  $0x94,%esp ; ret          ; nothing to draw
//      1005f5a9  call 0x1005dca0                ; NO push, NO %ecx, NO %edx
//      1005f5ae  test %eax,%eax
//      1005f5b0  jne  0x1005f5b9                ; ...so the result is an
//      1005f5b2  add  $0x94,%esp ; ret          ;    LTBOOL in %eax
//
//  and confirmed from the inside: the function's `ret` is a bare `ret`
//  (no `ret $N`), and there is NOT ONE positive `%ebp` displacement in
//  the whole 6,359 bytes -- every frame reference is negative.  %ecx and
//  %edx are both written before they are read (0x1005dcbe, 0x1005dcb9),
//  so it is not one of icl's register-ABI entries either.
//
//  WHAT IT IS, pinned by the console variable.  0x1005f730 -- twelve
//  bytes past this function's padding -- is
//
//      1005f75f  movl $0x100aab80,(%esp)      ; "AllSkyPortals"
//      1005f766  mov  $0x1008ff40,%ecx
//      1005f76b  call 0x10021f40              ; the console-variable registrar
//
//  so 0x1008ff40, the flag this function tests at 0x1005dccf, IS the
//  console variable **AllSkyPortals**.  The four accumulators
//  0x1008ff3c/0x1008ff38/0x1008ff34/0x1008ff30 are handed, in that order,
//  to 0x10020d80 and 0x1001ef30 by the caller (0x1005f5e0..0x1005f604 and
//  0x1005f6b8..0x1005f6f5) together with 0x1008b880 -- the screen
//  transform object every other d3d.ren decode names -- and a literal
//  0x3c23d70a = 0.01f.  That is the sky-portal screen rectangle.
//
//  ------------------------------------------------------------------- //
//  3. THE ACCEPTANCE FIGURE AND WHAT IT DOES *NOT* WITNESS.  READ FIRST.
//  ------------------------------------------------------------------- //
//
//  **1,592 BYTES PER CALL.  THE CALL CHECK IS VERY NEARLY BLIND HERE.**
//  Four calls, three distinct callees, and all four are in the first 90
//  bytes plus one sunk block:
//
//      0x1005dcb4  call 0x1007b8d0     the 0x10091a10 singleton accessor
//      0x1005dcf7  call 0x1000a670     `return *(this+4)`
//      0x1005f416  call 0x1000a670     (the sunk AllSkyPortals arm)
//      0x1005f429  call 0x1005f780     `return *(this+8)`
//
//  6,269 of the 6,359 bytes -- 98.6% -- are call-free.  The whole
//  transform, the whole six-plane clipper and the whole min/max
//  accumulation could be wrong and the call sequence would still agree 4 of 4.
//  Quote the figure only WITH this paragraph.
//
//  What pins the decode instead is the other three instruments;
//  all three are set out in sections 3a/3b/3c and all three come out
//  clean.
//
//  ------------------------------------------------------------------- //
//  3a. AN INSTRUCTION CENSUS WITH NO RESIDUE.
//  ------------------------------------------------------------------- //
//
//  Counted over 0x1005dca0..0x1005f577 with
//  `awk -F'\t' 'NF>2{print $3}' fn.txt | awk '{print $1}' | sort |
//   uniq -c`, then every row over five DECOMPOSED BY OPERAND with
//  `awk '$1==m' | sort | uniq -c`.  EVERY mnemonic in the function is in
//  the table and EVERY row closes exactly.  There is no residue.
//
//   | mnemonic | shipped | what the structure below requires |
//   |----------|---------|-----------------------------------|
//   | movzbl   |  60     | 6 passes x 5 byte lerps x 2 widenings.  NOTHING else in the function widens a byte |
//   | fildl    |  60     | the same 30 byte lerps x 2 (the integer difference and the base) |
//   | fistpl   |  30     | the same 30 byte lerps x 1 |
//   | fdivp    |   6     | one divide per clip pass |
//   | fdivrs   |   2     | the TWO `1.0f / w` reciprocals, 0x100aab04 and 0x100aab0c -- two separate 1.0f literals, i.e. TWO separate inline expansions of ltmatrix.h's MatVMul_H |
//   | fcomp    |  12     | 6 x `fDen < -1e-5` in line + 6 x `fDen > 1e-5` in the sunk arms |
//   | fcomps   |   8     | 6 classify tests + the 2 final `> 0.9f` tests |
//   | fcom     |   4     | the FOUR min/max tests of the accumulator |
//   | sar      |   6     | 6 x `nIn = (pOut - pOutStart) / 32`.  A **shift** -- big051a0's 40-byte vertex needs `cltd ; idiv`, and there is NO `idiv` and NO `cltd` anywhere in this function |
//   | dec      |   6     | 6 x `j = nIn - 1` |
//   | fsub     |   6     | the six `pWalk->? - pPrev->?` that open each fDen |
//   | fsubrs   |   2     | `g_f1008b908 - pPrev->z` (pass 1, 0x1005dfb7) and `g_f1008b910 - pPrev->z` (pass 6, 0x1005f037) |
//   | fsubrp   |   2     | passes 3 and 4's `pPrev->y - pPrev->z` / `pPrev->x - pPrev->z` (0x1005e65e, 0x1005e9ac) |
//   | fsts     |   2     | pass 3's `y = fVal` (0x1005e67d) and pass 4's `x = fVal` (0x1005e9cf) |
//   | fld      |   2     | passes 2 and 5 duplicating fVal before negating it (0x1005e32f, 0x1005ed1d) |
//   | fchs     |   8     | 2 classify negations (2,5) + 4 numerator negations (2,3,4,5) + 2 result negations (2,5) |
//   | movzwl   |   3     | pPoly->m_u50 read THREE times -- see note 5(e) |
//   | incl     |   1     | `g_n1008be3c++` at 0x1005f39b |
//   | and      |   1     | `nClip &= 1` under g_n1008d7a0 |
//   | call     |   4     | section 3 |
//   | ret/push/pop | 1/1/1 | the single epilogue |
//   | nop      |   4     | alignment inside the function |
//
//  THE ROWS THAT MATTER, decomposed by operand.  Each of these was
//  produced by the tool, not by eye.
//
//  `fsubs` 30 = 6 x `0x8(%edx)` (4 z-lerp differences, passes 2-5, plus
//      2 fDen tails, passes 2 and 5) + 6 x `0x18(%ebx)` + 6 x `0x1c(%ebx)`
//      (the TWELVE texture-lerp differences) + 4 x `0x4(%edx)` (y-lerps,
//      passes 1,2,4,6) + 4 x `(%edx)` (x-lerps, passes 1,3,5,6)
//      + 2 x `0x8(%esi)` (fDen tails, passes 3 and 4)
//      + `fsubs 0x1008ff3c` + `fsubs 0x1008ff38` (the two final tests).
//      **big051a0's 40-byte twin has 40 here, six per pass, because it
//      has FOUR texture lerps.  This row alone rules out a second
//      texture pair.**
//
//  `fadds` 36 = 6 x `0x8(%edx)` + 12 x `0x18/0x1c(%ebx)` + 4 x `0x4(%edx)`
//      + 4 x `(%edx)` + 2 x `0x8(%esi)` -- the same 28 lerp/fDen sites --
//      + EIGHT matrix `m[i][3]` adds: 0x1008b9e8, 0x1008b9f8, 0x1008ba08,
//      0x1008ba18 (transform 1) and 0x1008ba28, 0x1008ba38, 0x1008ba48,
//      0x1008ba58 (transform 2).  Four rows each, two transforms.
//
//  `fstps` 66 = 12 texture stores (6 x `0x18(%eax)` + 6 x `0x1c(%eax)`)
//      + 5 x `0x4(%ecx)` (y: passes 1,2,4,5,6 -- pass 3 uses `fsts`)
//      + 5 x `(%ecx)`    (x: passes 1,2,3,5,6 -- pass 4 uses `fsts`)
//      + 4 x `0x8(%ecx)` (z: passes 2,3,4,5 -- passes 1 and 6 store the
//        plane constant with an INTEGER `mov`)
//      + 30 byte-lerp float temps (-0xc68 .. -0xb80, all thirty distinct)
//      + 3 (-0xb7c/-0xb78/-0xb74(%edx), transform 1's destination)
//      + 3 (-0x14c/-0x148/-0x144(%ebp), transform 2's destination)
//      + 4 (the four accumulator globals).
//
//  `fmuls` 23 = transform 1's twelve (0x1008ba14/ba10/ba0c for w,
//      0x1008b9e4/b9e0/b9dc for row 0, then `0x4(%eax)`/`0x8(%eax)`/
//      `(%eax)` twice over for rows 1 and 2) + transform 2's eleven
//      (`0x8(%edx)`/`0x4(%edx)`/`(%edx)` three times over for w, row 1
//      and row 2, plus 0x1008ba24 and 0x1008ba20 for row 0).
//
//  `fmul` 53 = 51 x `fmul %st(1),%st` (6 passes x 8 uses of fT -- 2
//      position + 2 texture + 4 of the 5 byte lerps = 48 -- plus
//      transform 1's rows 0 and 1 and transform 2's row 1)
//      + 1 x `fmul %st,%st(2)` + 1 x `fmul %st(3),%st` (transform 2's
//      row 0, scheduled through the hoisted m[0][0]).
//
//  `fmulp` 8 = the 5th byte lerp of each pass (6, the operand order
//      forcing the pop form exactly as big051a0 records) + transform 1's
//      row 2 + transform 2's row 2.
//
//  `faddp` 48 = 30 byte-lerp adds + 2 numerators (passes 2 and 5)
//      + 8 (transform 1: two per row, four rows)
//      + 8 (transform 2: two per row, four rows).
//
//  `flds` 129, by operand, with nothing over:
//      0x8(%edx) x11  4 classify (1,2,5,6) + 2 fDen opens (1,6)
//                     + 4 numerator pPrev->z (2,3,4,5)
//                     + 1 transform 2's row-0 z (0x1005f27a)
//      0x4(%edx) x4   1 classify (3) + 2 fDen opens (3,5) + 1 transform 2
//      (%edx)    x4   1 classify (4) + 2 fDen opens (2,4) + 1 transform 2
//      0x8(%esi) x6   2 fDen opens (1,6) + 4 z-lerps (2,3,4,5)
//      (%esi)    x6   4 x-lerps (1,3,5,6) + 2 fDen opens (2,4)
//      0x4(%esi) x6   4 y-lerps (1,2,4,6) + 2 fDen opens (3,5)
//      0x18(%esi) x6, 0x1c(%esi) x6      the TWELVE texture lerps
//                                        (big051a0 has 24: two pairs)
//      0x8/0x4/(%eax) x2 each = 6        transform 1's w row and row 0
//      -0xc68..-0xb80 x30                the thirty byte-lerp temps
//      0x100aab18 + 8k x6                the six in-line -1e-5 epsilons
//      0x100aab48 + 8k x6                the six sunk +1e-5 epsilons
//      0x100aab4c + 8k x6                the six sunk 0.0f
//      0x1008b9f0/b9f4/b9ec/ba00/ba04/b9fc  transform 1's rows 1 and 2
//      0x1008ba1c/ba54/ba50/ba4c/ba30/ba34/ba2c/ba40/ba44/ba3c  x10
//                                        transform 2
//      -0x14c x2, -0x148 x2              the accumulator's re-reads of
//                                        the projected x and y
//      0x1008ff3c x1, 0x1008ff38 x1, 0x1008ff34 x2, 0x1008ff30 x2
//                                        the accumulator + the two final
//                                        tests (which reach ff3c/ff38
//                                        with `fsubs`, hence x1 each)
//                                                        ---- 129 exactly.
//
//  `fstp` 25 = the six sunk epsilon arms' x87 cleanup (2+3+3+3+3+2 = 16)
//      + 4 x `fstp %st(0)` on the min/max fall-through
//      + 4 x `fstp %st(1)` in the four sunk min/max arms
//      + 1 x `fstp %st(0)` at 0x1005f399, dropping the LOOP-INVARIANT
//        m[0][0] the accumulator hoisted at 0x1005f24a.
//
//  `fxch` 26 = 6 (the 5th byte lerp) + 8 (the numerator/result juggling,
//      1+2+1+1+2+1 across the six passes, exactly big051a0's 8)
//      + 12 in transform 2's scheduled interleave.
//
//  `shl` 18 = 6 x `pEnd = pIn + nIn` + 6 x `pPrev = pIn + (nIn - 1)`
//      + **FIVE** x `pOut = pOut + nIn` + 1 in the accumulator's pEnd.
//      FIVE, not six: pass 6 has no `pOut += nIn` because pOut is dead
//      after it -- INDEPENDENT confirmation of big051a0's note on the
//      same statement, arrived at here by counting shifts.
//
//  `lea` 29 = 20 lea-nop pads (`lea 0x0(%esi,%eiz,1),%esi` x8,
//      `lea 0x0(%edi,%eiz,1),%edi` x8, `lea 0x0(%esi),%esi` x3,
//      `lea 0x0(%edi),%edi` x1) plus NINE real ones:
//        6 x `lea -0xNNN(%ebp),%esi|%ecx`   the six flag arrays
//        1 x `lea -0xb7c(%ebp),%eax`        `pVerts = aVerts`
//        1 x `lea 0x28(%ebx),%ecx`          the singleton's list
//        1 x `lea (%ebx,%eax,4),%ebx`       `ppEnd = ppList + nPolys`
//      Note what is ABSENT: big051a0's forty `lea (%eax,%eax,4)` /
//      `lea (%ecx,%eax,8)` x40 pairs are gone, because a 32-byte stride
//      is a shift.  That absence is the fact `sar` reports, counted a
//      second way.
//
//  `add` 60 = 6 `add $0x4,%ecx` (pFlag++) + 6 `add %esi,%ebx`
//      (nInside += nFlag) + 8 `add $0x20,%edx` (6 classify pWalk++ + the
//      two transform loops) + 6 `add $0x20,%esi` (edge-loop pWalk++)
//      + 6 `add $0x20,%eax` (pOut++ after the copy) + 6 `add %eax,%ecx`
//      from `mov $0x20,%ecx ; add %eax,%ecx` (pOut++ after the lerp -- the
//      /O2 form, never `lea`) + 5 `add %eax,%ecx` + 1 `add %eax,%edx`
//      (the six pEnd) + 5 `add %eax,%ecx` (the five `pOut += nIn`)
//      + 5 `add %eax,%esi` + 1 `add %eax,%ebx` (the six pPrev)
//      + 1 `add %edx,%ecx` (the accumulator's pEnd) + 1 `add $0x4,%eax`
//      (ppWalk++) + 1 `add $0x18,%ecx` (transform 1's SPolyVert walk)
//      + 2 `add $0x19c,%ecx` (the sunk arm).
//
//  `sub` 37 = 24 `sub %edx,%eax` + 6 `sub %ebx,%eax` (the THIRTY
//      byte-lerp integer differences; the 5th of each pass uses %ebx)
//      + 5 `sub %edx,%ecx` + 1 `sub %edx,%eax` (the six
//      `pOut - pOutStart`) + `sub $0xc78,%esp`.
//
//  `cmp` 34 = 6 classify back-edge + 6 `abInside[j] != abInside[i]`
//      + 6 `i < nIn` + 6 `pIn == pEnd` + 6 `nInside != nIn`
//      + 1 transform-1 loop + 1 accumulator guard + 1 accumulator
//      back-edge + 1 outer loop.
//
//  `xor` 14 = 6 `nInside = 0` + 6 `i = 0` + 1 transform-1 `j = 0`
//      + 1 `return LTFALSE`.
//
//  `inc` 13 = 6 `g_n1008be44++` + 6 edge-loop `i++` + 1 transform-1 `j++`.
//
//  `test` 29 = 1 g_n1008ff40 + 1 g_p1008beac + 1 nPolys + 1 transform-1
//      guard + 1 g_n1008d7a0 + 6 `nClip & BIT` + 6 `nInside == 0`
//      + 6 `nIn <= 0` + 6 `abInside[j]`.
//
//  `je` 40 = 1 (g_p1008beac test at 0x1005dce5) + 1 (g_n1008d7a0 at
//      0x1005de2d) + 1 (`nClip == 0` at 0x1005de3f) + 6 mask-bit skips
//      + 6 `pIn == pEnd` continues + 6 `nInside == 0` continues
//      + 6 `nInside == nIn` skips + 6 `abInside[j] == 0` copy skips
//      + 6 `abInside[j] == abInside[i]` lerp skips
//      + 1 accumulator empty guard.
//  `jne` 8 = 6 classify back-edges + 1 accumulator back-edge
//      + 1 (`g_p1008beac != NULL` at 0x1005dcee).
//  `jle` 7 = 6 `nIn <= 0` + 1 transform-1 guard.
//  `jl`  7 = 6 edge loops + 1 transform-1 loop.
//  `jbe` 11 = 6 in-line epsilon low tests + 1 pass-6 classify
//      + 2 final `> 0.9f` tests + 1 empty-list guard + 1 min/max.
//  `jb`  11 = 4 classify (passes 2,3,4,5) + 6 sunk-arm high tests
//      + 1 outer-loop back-edge.
//  `jae` 4 = 1 pass-1 classify + 3 min/max.
//  `jmp` 12 = 6 sunk `fT = 0.0f` arms + 4 sunk min/max arms
//      + 1 sunk AllSkyPortals arm + 1 sunk `return LTFALSE`.
//
//  `mov` 600 is not decomposed row by row; its load-bearing sub-rows are
//  the 30 x 5 byte-lerp staging movs and the 6 x 8 dword `*pOut = *pPrev`
//  copies, both counted above by other means.
//
//  60 = 6x5x2, 30 = 6x5, TWELVE texture lerps = 6x2, `sar` 6 with `idiv`
//  0, and `shl` 18 = 6+6+5+1 are the load-bearing rows: **six clip
//  planes, each with two position lerps, TWO texture lerps and five byte
//  lerps, on a 32-BYTE vertex, with pass 6's `pOut += nIn` dead.**  A
//  seventh plane, a missing plane, a second texture pair, a sixth colour
//  byte or a 40-byte vertex would each move at least three of those rows,
//  and none of them moves.
//
//  ------------------------------------------------------------------- //
//  3b. FRAME ARITHMETIC THAT DECOMPOSES EXACTLY.
//  ------------------------------------------------------------------- //
//
//  `sub $0xc78,%esp` = 3,192 bytes.  Every `%ebp`-relative displacement
//  in the function was DUMPED and sorted; the list is contiguous exactly
//  where the objects below say it should be.
//
//    -0xc78 .. -0xc6d   12  the three callee-saved spills, written
//                           `mov %ebx,(%esp)` / `%esi,0x4(%esp)` /
//                           `%edi,0x8(%esp)` at 0x1005dca9..0x1005dcb1
//                           and reloaded at 0x1005f3fb..0x1005f402.
//    -0xc6c .. -0xc69    4  alignment; nothing addresses it.
//    -0xc68 .. -0xb7d  236  THIRTY 8-byte float temps, stepping by 8 from
//                           -0xc68 to -0xb80 -- DUMPED, all thirty
//                           present, none repeated, 29*8 + 4 = 236
//                           EXACTLY, and -0xb7c is the next object.  Each
//                           is used once as `fstps -0xNNN(%ebp) ;
//                           flds -0xNNN(%ebp) ; fistpl <int slot>` --
//                           6 passes x 5 byte lerps.  Same shape as
//                           big051a0's -0x700..-0x618 and big70d10's
//                           -0x10f0..-0x1005.
//    -0xb7c .. -0x67d 1280  **RVert aVerts[40]** -- 0x500 bytes at a
//                           32-byte stride.  The bound FORTY is read off
//                           the frame here, not assumed: the transform
//                           loop writes -0xb7c/-0xb78/-0xb74 with
//                           `add $0x20,%edx` per vertex (0x1005ddf9) and
//                           the next named object is at -0x67c.
//    -0x67c, -0x678      8  `pVerts` and `nVerts`, the clipper's in/out
//                           pair.  Both are STORED and then RELOADED
//                           across nothing that could alias
//                           (0x1005de10/0x1005de6a, 0x1005de1f/
//                           0x1005de4a, 0x1005f225/0x1005f237) -- see
//                           COIN-FLIP (3).
//    -0x674 .. -0x14d 1320  SIX x 0xdc, the six clip passes' state.  The
//                           flag arrays are -0x674, -0x598, -0x4bc,
//                           -0x3e0, -0x304 and -0x228, EXACTLY 0xdc
//                           apart, and all six are indexed as
//                           `-0xNNN(%ebp,%edx,4)` / `(%ebp,%edi,4)`.
//                           0xdc = 0xc8 + 0x14 = **50 int32** of flags
//                           plus five int slots.  Each pass's five int
//                           slots were DUMPED and land exactly at
//                           (array + 0xc8):
//                             -0x674 -> -0x5ac -0x5a8 -0x5a4 -0x5a0 -0x59c
//                             -0x598 -> -0x4d0 -0x4cc -0x4c8 -0x4c4 -0x4c0
//                             -0x4bc -> -0x3f4 -0x3f0 -0x3ec -0x3e8 -0x3e4
//                             -0x3e0 -> -0x318 -0x314 -0x310 -0x30c -0x308
//                             -0x304 -> -0x23c -0x238 -0x234 -0x230 -0x22c
//                             -0x228 -> -0x160 -0x15c -0x158 -0x154 -0x150
//                           The bound FIFTY and the count FIVE are both
//                           read off the frame, not guessed -- the same
//                           finding big051a0, big5a960, big6d910 and
//                           big70d10 all record.
//    -0x14c, -0x148,
//    -0x144             12  the second MatVMul_H's destination LTVector
//                           (x, y, z).  -0x144 (z) is WRITTEN and NEVER
//                           READ -- which is why this is the header's
//                           three-component MatVMul_H and not a
//                           hand-written x/y-only projection.
//    -0x140              4  the integer scratch every byte lerp stages
//                           its `movzbl ; movzbl ; sub` through.
//    -0x13c .. -0x12d   16  dead; nothing addresses it.
//    -0x12c .. -0x110   28  the clipper's bookkeeping, in frame order:
//                           nIn (-0x12c), pOut (-0x128), nClipFlags
//                           (-0x124, a BYTE), ppEnd (-0x120), pOutStart
//                           (-0x11c), and the two slots -0x118 / -0x114
//                           the six passes swap between `i` and `pIn`,
//                           and nClip (-0x110, a BYTE).
//    -0x10c .. -0xe9    36  dead; nothing addresses it.
//    -0xe8               4  pIn's spill across each classify loop.
//    -0xe4               4  dead.
//    -0xe0               4  `ppWalk`, the outer polygon-list cursor.
//    -0xdc .. -0x9     212  dead; nothing addresses it.
//    -0x8                4  %edi's spill across the 32-byte vertex copy.
//    -0x4                4  dead.
//                     -----
//                      3192 = 0xc78 EXACTLY.
//
//  The 30x8 temp block landing EXACTLY against the 40-vertex array, and
//  that array landing EXACTLY against the six 0xdc clip blocks whose five
//  int slots sit at +0xc8 with the next array at +0xdc, is the strongest
//  single piece of evidence in this file.  It pins the vertex-array bound
//  (40), the flag-array bound (50) and the number of byte lerps (five per
//  pass, thirty in all) independently of every call.
//
//  ------------------------------------------------------------------- //
//  3c. EXPRESSION-BY-EXPRESSION CORRESPONDENCE WITH A DECODED SIBLING.
//  ------------------------------------------------------------------- //
//
//  The six clip passes were DIFFED against big051a0.cpp's -- itself the
//  seventh independent reading of this source -- address by address, not
//  skimmed.  Everything agreed:
//
//    - the six mask bits are **0x01, 0x04, 0x08, 0x10, 0x20, 0x02 IN THAT
//      ORDER**: 0x1005de68 `test $0x1,%al`, 0x1005e1b2 `test $0x4,%dl`,
//      0x1005e50a `test $0x8,%dl`, 0x1005e853 `test $0x10,%dl`,
//      0x1005eba4 `test $0x20,%dl`, 0x1005eef9 `test $0x2,%dl`.  There
//      are SIX `test $imm` in the whole function and these are they;
//      every other `test` is a register-against-itself;
//    - the six classify tests are
//          z >= g_f1008b908 | -z < x | y < z | x < z | -z < y
//          | z <= g_f1008b910
//      read off 0x1005deb0 (`flds 0x8(%edx) ; fcomps 0x1008b908` / jae),
//      0x1005e200 (`flds 0x8 ; fchs ; fcomps (%edx)` / jb), 0x1005e550
//      (`flds 0x4(%edx) ; fcomps 0x8(%edx)` / jb), 0x1005e8a0 (`flds (%edx) ;
//      fcomps 0x8` / jb), 0x1005ebf0 (`flds 0x8 ; fchs ; fcomps 0x4` /
//      jb), 0x1005ef40 (`flds 0x8 ; fcomps 0x1008b910` / jbe);
//    - the six denominators and numerators are big051a0's, term for term
//      -- re-read here at 0x1005df9c, 0x1005e2ec, 0x1005e63c, 0x1005e98c,
//      0x1005ecdc, 0x1005f01c, including the two `fsubrs` against the
//      plane globals (0x1005dfb7, 0x1005f037), the two `faddp`
//      numerators (0x1005e30c, 0x1005ecfe) and the two `fsubrp`
//      numerators (0x1005e65e, 0x1005e9ac);
//    - the epsilon triple is the same and the constants were DUMPED out
//      of the image: 0x100aab18, 0x100aab20, 0x100aab28, 0x100aab30,
//      0x100aab38 and 0x100aab40 are all `ac c5 27 b7` =
//      -9.999999747e-06; 0x100aab48, 0x100aab50, 0x100aab58, 0x100aab60,
//      0x100aab68 and 0x100aab70 are all `ac c5 27 37` =
//      +9.999999747e-06; 0x100aab4c, 0x100aab54, 0x100aab5c, 0x100aab64,
//      0x100aab6c and 0x100aab74 are all `00 00 00 00` = 0.0f.  The sunk
//      arms read them in REVERSE pass order (pass 1 uses the highest
//      pair, 0x100aab70/0x100aab74);
//    - **the SIX DEAD 1.0f CONSTANTS are here too**: 0x100aab1c,
//      0x100aab24, 0x100aab2c, 0x100aab34, 0x100aab3c and 0x100aab44 are
//      all `00 00 80 3f` = 1.0f, interleaved with the six -1e-5 epsilons
//      at 0x100aab18 + 8k, and referenced from NOWHERE in this function.
//      big5a960.cpp and big051a0.cpp both record the same six; this is
//      the third independent sighting.  See NOT PINNED (1);
//    - the six `pIn = pOutStart ; nIn = (pOut - pOutStart)/32` tails are
//      whose `sar $0x5` sits at 0x1005e196, 0x1005e4ee, 0x1005e837,
//      0x1005eb88, 0x1005eedd and 0x1005f214, each a SHIFT where big051a0
//      has `mov $0x28,%ebx ; cltd ; idiv %ebx`;
//    - the FIVE BYTE LERPS are +0x12, +0x11, +0x10, +0x13, +0x17 in that
//      order in ALL SIX passes -- thirty sites, all read;
//    - the TWO TEXTURE LERPS are +0x18 then +0x1c in all six passes --
//      twelve sites, all read.  big051a0's +0x20/+0x24 pair is ABSENT,
//      which is what makes this the 32-byte vertex.
//
//  This file therefore REUSES that decode rather than re-deriving it;
//  what is new here is everything OUTSIDE the clipper --
//  the polygon walk, the two transforms and the min/max accumulator --
//  and that is set out region by region below.
//
//  ------------------------------------------------------------------- //
//  4. COMPILER VERDICT: Intel C/C++ 4.0, **/O2**, and **/QIfist**.
//  ------------------------------------------------------------------- //
//
//  FOR /O2:
//    * COLD-BLOCK SINKING BELOW THE EPILOGUE, twelve-fold -- the
//      365-byte tail enumerated under THE EXTENT.
//    * `&p->m` as `mov $K,%reg ; add %p,%reg`: all six `pOut++` at the
//      end of a lerp are `mov $0x20,%ecx ; add %eax,%ecx` (0x1005e160,
//      0x1005e4ba, 0x1005e804, 0x1005eb55, 0x1005eea9, 0x1005f1e0),
//      never `lea 0x20(%eax),%ecx`.  Likewise `add $0x19c,%ecx` at
//      0x1005f410/0x1005f423 rather than `lea 0x19c(%ecx),%ecx`.
//    * `mov $0x1,%esi ; mov $0x0,%eax` for the classify flag at every one
//      of the six sites, never the /Os `xor ; inc` pair.
//    * LOOP-INVARIANT HOISTING with an x87 register: the accumulator's
//      `flds 0x1008ba1c` (m[0][0]) sits OUTSIDE the loop at 0x1005f24a
//      and is dropped with a bare `fstp %st(0)` at 0x1005f399 when the
//      loop ends.
//
//  AGAINST (recorded, not explained away):
//    * ONE epilogue.  Both `return LTFALSE` arms and the `return LTTRUE`
//      converge on `mov %edx,%eax` at 0x1005f3f9.  /O2 usually
//      DUPLICATES.  Exactly the anomaly big051a0.cpp and big70d10.cpp
//      record for the same source family.
//    * There are only two call sites with arguments in the whole
//      function and BOTH are thiscall with no stack argument, so the
//      `add $-N,%esp` vs `push` test has nothing to look at.
//
//  /QIfist: all thirty float->byte conversions are
//  `fstps <float temp> ; flds <float temp> ; fistpl <int slot> ;
//   mov <int slot>,%dl` -- a bare `fistp` in the current rounding mode,
//  with NO fnstcw/fldcw dance and NO helper call at any of the thirty
//  sites.  Same finding as big051a0, big5a960, big61a30, big44ff4 and
//  d3dr3c.  Under plain /O2 those thirty sites will differ in bytes.
//
//  ------------------------------------------------------------------- //
//  5. FIVE READINGS THAT ARE THIS FUNCTION'S OWN.
//  ------------------------------------------------------------------- //
//
//  (a) THE REJECT IS `continue`, NOT `return`.  big051a0's six clip
//      passes each own a private `xor %edx,%edx ; jmp <epilogue>` block.
//      Here there are TWELVE `je 0x1005f3a1` (0x1005de9a, 0x1005dee2,
//      0x1005e1e3, 0x1005e230, 0x1005e53b, 0x1005e57f, 0x1005e884,
//      0x1005e8ce, 0x1005ebd5, 0x1005ec21, 0x1005ef26, 0x1005ef70) --
//      two per pass, the empty-input guard and the `nInside == 0` test --
//      and 0x1005f3a1 is the OUTER POLYGON LOOP'S INCREMENT, not the
//      epilogue.  A rejected polygon is skipped; the walk goes on.  Note
//      that a reject also skips `g_n1008be3c++` at 0x1005f39b, so that
//      counter counts polygons that SURVIVED.
//
//  (b) THE `UseD3DClip` EARLY-OUT IS A SKIP, NOT A RETURN.  big051a0's
//      `if (g_n1008d7a0) { nClip &= 1; if (!nClip) return LTTRUE; }`
//      returns; here the same test at 0x1005de25..0x1005de3f jumps to
//      0x1005f231, which is the ACCUMULATOR.  The unclipped vertices are
//      still projected and still accumulated.
//
//  (c) TWO SEPARATE 4x4 MATRICES, both at file scope.  0x1008b9dc (rows
//      at +0x00/+0x10/+0x20/+0x30) drives the FIRST transform and
//      0x1008ba1c (rows at +0x00/+0x10/+0x20/+0x30) the SECOND.  They are
//      adjacent and non-overlapping: 0x1008b9dc + 0x40 == 0x1008ba1c.
//      big44ff4.cpp already named the first one `g_Mat1008b9dc` and read
//      its row layout off exactly these operands.  Each transform has its
//      OWN 1.0f literal (0x100aab04 and 0x100aab0c), which is what says
//      they are two separate expansions of one inline header function
//      rather than one expansion in a loop.
//
//  (d) THE ACCUMULATOR ALWAYS STORES.  Each of the four min/max sites
//      ends in an UNCONDITIONAL `fstps <global>`; the branch chooses only
//      WHICH x87 register is discarded (`fstp %st(0)` on the fall-through,
//      `fstp %st(1)` in the sunk arm).  That is a ternary / LTMIN-LTMAX
//      macro, not an `if (a < b) b = a;`.  See COIN-FLIP (2).
//
//  (e) THE POLYGON'S VERTEX COUNT IS READ THREE TIMES.  `movzwl
//      0x50(%esi)` appears at 0x1005dd2e (the loop guard), 0x1005ddf5
//      (INSIDE the transform loop, re-read every iteration) and
//      0x1005de16 (`nVerts` for the clipper).  Three reads means the
//      member is written in the loop condition, not cached in a local --
//      the same shape big64cc0.cpp records for its light pass.
//
//  ------------------------------------------------------------------- //
//  6. WHAT IS *NOT* PINNED -- read before trusting any byte.
//  ------------------------------------------------------------------- //
//
//  (1) SIX DEAD 1.0f CONSTANTS (0x100aab1c + 8k).  Each clip pass's
//      source mentions a 1.0f that icl folded away and the spelling below
//      does not.  Unresolved in big5a960.cpp, unresolved in big051a0.cpp,
//      unresolved here.
//
//  (2) THE THIRTY `(uint8)` CASTS.  /QIfist, as section 4 says; under
//      plain /O2 they emit the control-word dance instead.  No call
//      witnesses them.
//
//  (3) THE SINGLETON'S LIST.  `lea 0x28(%ebx) ; call 0x1000a670` gives
//      the array base from *(pRoot+0x2c); the count comes from
//      `mov 0x3c(%ebx),%ebx`, i.e. *(pRoot+0x3c) = base+0x14.  The
//      COLD arm uses a container at g_1008beac+0x19c whose base is
//      *(+0x1a0) (the same 0x1000a670) and whose count is *(+0x1a4)
//      through 0x1005f780 (`return *(this+8)`).  So the two containers
//      put their count at DIFFERENT offsets (+0x14 vs +0x08) and only
//      one of the two size accessors was emitted out of line.  Either
//      they are two different container classes and the +0x28 one's
//      GetSize was inlined because it has one caller, or +0x3c is a
//      plain member of the singleton that happens to hold the count.
//      Nothing in the bytes separates those, and both are written up
//      under COIN-FLIP (4).
//
//  (4) THE ORDER OF THE FOUR INITIALISING STORES.  The shipped order is
//      0x1008ff3c, 0x1008ff38, 0x1008ff30, 0x1008ff34 -- min, min, max,
//      max but with the two maxes transposed relative to the two mins.
//      Both constants live in a register (%edx = +10000, %ecx = -10000)
//      across all four, so nothing orders the pair.  Written below in
//      the un-transposed order.
//
//  (5) THE 212 DEAD FRAME BYTES at -0xdc..-0x9 (and the smaller holes at
//      -0x13c..-0x12d and -0x10c..-0xe9).  268 bytes of the 3,192-byte
//      frame are addressed by nothing.  They are consistent with icl
//      reserving slots for temporaries that later died, but no source
//      construct is claimed for them.
//
//  ------------------------------------------------------------------- //
//  7. COIN-FLIPS -- both spellings given, one build settles each.
//  ------------------------------------------------------------------- //
//
//  (1) THE OPENING IF/ELSE.  *** SETTLED BY THE BUILD, not a coin-flip. ***
//      Written below as
//          if (!g_nAllSkyPortals || !g_p1008beac) { <hot> }
//          else                                  { <cold> }
//      The other spelling,
//          if (g_nAllSkyPortals && g_p1008beac) { <cold> } else { <hot> }
//      compiles to the same instructions in a DIFFERENT ORDER: icl emits
//      the cold arm inline instead of sinking it, so the two accessor
//      calls in that arm come out ahead of the hot one and the call sequence gives
//      3 of 4 with an equal shipped-only/ours-only pair.  Inverting the
//      test sinks the arm and gives 4 of 4.  Both were built and measured;
//      this is not a reading.
//
//      UPDATE: the shipped arm is sunk as a whole -- `je` to the default
//      list on AllSkyPortals, then `jne` to the sunk arm on the grid -- and
//      each arm stores its own clip mask (`mov $0x3f,%dl ; mov %dl,
//      -0x124(%ebp)`).  Both come out only as a `switch` on the `&&` whose
//      `case 0` is the default list and whose `default` is the sky arm
//      (the lever of w26_100262f0.cpp / w12_3b3d0_os.cpp), with the mask a
//      uint32: the first 0x84 bytes are then the shipped ones and the
//      instruction count is the shipped 1,660 (was 1,654; 47 -> 42 blocks).
//      As if/else in either polarity, or with the mask uint8 / int32, the
//      mask is stored once after the join.
//
//      NOTE the general lesson, because it cost this function three of its
//      four calls twice over.  An arm-order "coin-flip" is usually NOT a
//      coin-flip when the function makes calls in either arm -- the call
//      ORDER decides it, and one build settles it in minutes.
//
//  (2) THE FOUR ACCUMULATIONS.  Written below as
//          g_fMinX = LTMIN(g_fMinX, vT.x);
//      i.e. `((a) < (b)) ? (a) : (b)` with the GLOBAL as `a`.  That is
//      what the bytes say: the "take the candidate" arm is the INCLUSIVE
//      comparison in all four cases (jae at 0x1005f323 with the global in
//      %st(0); jbe at 0x1005f342, jae at 0x1005f361 and jae at 0x1005f380
//      with the candidate in %st(0)), and only `cand` sitting in the
//      ternary's `b` position produces an inclusive test.  THE
//      ALTERNATIVE, spelled out longhand:
//          g_fMinX = (g_fMinX < vT.x) ? g_fMinX : vT.x;
//      -- identical text after macro expansion; the flip is only whether
//      the original used the SDK macro or wrote the conditional out.
//      What is NOT a live alternative is `if (vT.x < g_fMinX) g_fMinX =
//      vT.x;`: that would branch around the store, and the store here is
//      unconditional.
//
//  (3) THE CLIPPER'S IN/OUT PAIR.  `pVerts` (-0x67c) and `nVerts`
//      (-0x678) are stored and immediately reloaded three times over,
//      across nothing that could alias them.  That is the signature of
//      ADDRESS-TAKEN locals, i.e. the original called a clipper
//      `f(&pVerts, &nVerts, ...)` -- big124a4.cpp's
//      `sub_10011420(&pSrcVerts, &nVerts, g_1008b880)` is exactly that
//      shape -- and icl inlined it.  The body below expands the clipper
//      as a MACRO over two ordinary locals, which is the spelling that
//      guarantees no call is emitted; if the store/reload pairs matter,
//      the alternative is a `static inline` helper taking
//      `(RVert** ppVerts, int32* pnVerts, uint8 nClip)` and returning
//      LTBOOL, called once here.  Both produce the same call sequence
//      (none).
//
//  (4) THE SINGLETON'S COUNT.  Written below as a member of the
//      container: `pRoot->m_List28.m_nCount`, the container being
//      {+0x00 ?, +0x04 data, +0x14 count}.  THE ALTERNATIVE:
//          ppList = pRoot->m_List28.sub_1000a670();
//          nPolys = pRoot->m_nCount3c;          /* a plain member */
//      See NOT PINNED (3).
//
//  (5) `nClip &= 1` VS `nClip = nClipFlags & 1`.  0x1005de2f re-reads
//      -0x124 (nClipFlags) rather than -0x110 (nClip, stored two
//      instructions earlier at 0x1005de27), which slightly favours the
//      second spelling.  Written below as `nClip &= 1` to match
//      big051a0.cpp; the alternative is one token away.
//
//  (6) THE FINAL TEST.  Written below as one `return A && B;`.  The
//      alternative, which produces the same two `jbe` to one shared
//      block:
//          if (g_fMaxX - g_fMinX <= 0.9f) return LTFALSE;
//          if (g_fMaxY - g_fMinY <= 0.9f) return LTFALSE;
//          return LTTRUE;
//
//  ------------------------------------------------------------------- //
//  8. REGION MAP.  Every byte of 0x1005dca0..0x1005f577 is here.
//  ------------------------------------------------------------------- //
//
//   0x1005dca0..0x1005dcf3  prologue, the three spills, the four
//                           accumulator resets, the AllSkyPortals test.
//   0x1005dcf4..0x1005dd23  the default list + `nClipFlags = 0x3f`, the
//                           empty-list guard and the outer loop head.
//   0x1005dd24..0x1005de03  THE FIRST TRANSFORM -- MatVMul_H of every
//                           polygon vertex through g_Mat1008b9dc into
//                           aVerts[].
//   0x1005de04..0x1005de75  pVerts/nVerts/nClip, the g_n1008d7a0
//                           early-skip, pOut/pOutStart from g_p1008b87c.
//   0x1005de76..0x1005e1ab  CLIP PASS 1, bit 0x01, NEAR   (z >= near).
//   0x1005e1ac..0x1005e503  CLIP PASS 2, bit 0x04, LEFT   (-z < x).
//   0x1005e504..0x1005e84c  CLIP PASS 3, bit 0x08, TOP    (y < z).
//   0x1005e84d..0x1005eb9d  CLIP PASS 4, bit 0x10, RIGHT  (x < z).
//   0x1005eb9e..0x1005eef2  CLIP PASS 5, bit 0x20, BOTTOM (-z < y).
//   0x1005eef3..0x1005f230  CLIP PASS 6, bit 0x02, FAR    (z <= far),
//                           and the writeback of pVerts/nVerts.
//   0x1005f231..0x1005f39a  THE SECOND TRANSFORM AND THE ACCUMULATOR --
//                           MatVMul_H through g_Mat1008ba1c and the four
//                           LTMIN/LTMAX stores.
//   0x1005f39b..0x1005f3bd  `g_n1008be3c++` and the outer loop increment.
//   0x1005f3be..0x1005f409  the two `> 0.9f` tests and the ONE epilogue.
//   0x1005f40a..0x1005f43a  the sunk AllSkyPortals arm.
//   0x1005f43b..0x1005f53f  the six sunk clip-epsilon arms, reverse pass
//                           order.
//   0x1005f540..0x1005f546  the sunk shared `return LTFALSE`.
//   0x1005f547..0x1005f577  the four sunk min/max arms.
//   0x1005f577..0x1005f57f  nine bytes of alignment padding.
//
//  UNDECODED: NONE.  Every byte of 0x1005dca0..0x1005f577 is covered by
//  a region above and by a statement below.
//
// *********************************************************************** //

#include "ltbasedefs.h"

// Round to nearest in the current FPU mode: w1_10001000.cpp's helper
// (0x10015190 out of line), the clipper family's colour-byte conversion.
// Inlined, it is the shipped `fstps t ; flds t ; fistpl n ; mov n,%dl`.
inline int32 RoundF(float f)
{
	int32 n;
	__asm fld f
	__asm fistp n
	return n;
}


// ======================================================================= //
//  DECLARATIONS
//
//  Relocations are masked, so only the SHAPES matter here; every type is
//  named by the address the shipped code reads it from, and each is
//  already named the same way by an earlier decode where one exists.
// ======================================================================= //

// ---------------------------------------------------------------------- //
//  The vector and the 4x4 matrix.  big44ff4.cpp already read
//  0x1008b9dc's row layout off exactly these operands: 0x1008b9dc..
//  0x1008b9e8 is m[0][*], 0x1008b9ec.. m[1][*], 0x1008b9fc.. m[2][*],
//  0x1008ba0c.. m[3][*].  0x1008ba1c is the SECOND matrix, immediately
//  after it (0x1008b9dc + 0x40 == 0x1008ba1c) and with the same layout.
// ---------------------------------------------------------------------- //
struct LTVec5dca0
{
	float			x;                                  // +0x00
	float			y;                                  // +0x04
	float			z;                                  // +0x08
};

struct LTMat5dca0
{
	float			m[4][4];
};


// ---------------------------------------------------------------------- //
//  THE VERTEX.  Stride 0x20 = 32, from `add $0x20,%edx` in every classify
//  loop, `add $0x20,%esi` in every edge loop, `mov $0x20,%ecx ;
//  add %eax,%ecx` after every lerp, `shl $0x5` x18 / `sar $0x5` x6 in the
//  pointer arithmetic, and the 0x500-byte (= 40 x 32) frame array.
//
//  It is big051a0.cpp's Vert051a0 WITHOUT the second texture pair -- the
//  shape big051a0 section 3 predicted for 0x10003c40.  Every offset below
//  is a load or a store in these bytes:
//    +0x00..+0x08  the position -- written by the transform, interpolated
//                  by all six clip passes
//    +0x0c         never named; copied by the eight-dword `*pOut = *pPrev`
//                  (`mov 0xc(%ebx),%eax ; mov %eax,0xc(%edi)`, six sites),
//                  so it must be declared for the copy to be 32 bytes
//    +0x10..+0x13  b, g, r, a -- interpolated in the order +0x12, +0x11,
//                  +0x10, +0x13 in all six passes
//    +0x14..+0x17  the specular dword; only its top byte +0x17 is
//                  interpolated, and it is the FIFTH byte lerp
//    +0x18/+0x1c   the ONE texture pair
// ---------------------------------------------------------------------- //
union VColor5dca0
{
	uint32			m_n;
	uint8			m_b[4];
};

struct RVert5dca0
{
	LTVec5dca0		m_vPos;                             // +0x00
	float			m_fRHW;                             // +0x0c
	uint8			m_b;                                // +0x10
	uint8			m_g;                                // +0x11
	uint8			m_r;                                // +0x12
	uint8			m_a;                                // +0x13
	uint8			m_sb;                               // +0x14
	uint8			m_sg;                               // +0x15
	uint8			m_sr;                               // +0x16
	uint8			m_sa;                               // +0x17
	float			m_fU;                               // +0x18
	float			m_fV;                               // +0x1c
};


// ---------------------------------------------------------------------- //
//  THE WORLD POLYGON.  big64cc0.cpp's SPoly / SPolyVert, cut to the two
//  members this function reads: the uint16 count at +0x50 (`movzwl
//  0x50(%esi)` at 0x1005dd2e, 0x1005ddf5 and 0x1005de16) and the EMBEDDED
//  0x18-byte vertex array at +0x54 (`mov 0x54(%ecx),%eax` with
//  `add $0x18,%ecx` at 0x1005ddec).  Only +0x00 of the SPolyVert, the
//  pointer to the world position, is touched here.
// ---------------------------------------------------------------------- //
struct SPolyVert5dca0
{
	LTVec5dca0*		m_pPos;                             // +0x00
	uint8			m_pad_04[0x14];                     // +0x04 .. +0x17
};

struct SPoly5dca0
{
	uint8			m_pad_00[0x50];                     // +0x00 .. +0x4f
	uint16			m_u50;                              // +0x50
	uint16			m_u52;                              // +0x52
	SPolyVert5dca0	m_aVerts[1];                        // +0x54
};


// ---------------------------------------------------------------------- //
//  THE TWO CONTAINERS.
//
//  0x1000a670 is `push %esi ; mov 0x4(%ecx),%eax ; add $0x4,%esp ; ret`
//  -- d3dbatchb_os.cpp's accessor for the field at +0x04; a scan of every
//  resolving `e8` in .text counts SEVENTY-FOUR direct callers, which is
//  why icl never inlines it.  0x1005f780 is `push %esi ;
//  mov 0x8(%ecx),%eax ; add $0x4,%esp ; ret` -- d3dr4a.cpp names it
//  GetSize; it has NINE callers.  Together they say the container puts
//  its data pointer at +0x04 and its count at +0x08, which is exactly
//  what the neighbouring functions read off the singleton's other
//  containers (0x1005d964 reads data at +0x264 and count at +0x268 for
//  the container based at +0x260).
//
//  The singleton's container at +0x28 uses the SAME 0x1000a670 for its
//  data pointer (*(pRoot+0x2c)) but its count is read INLINE as
//  `mov 0x3c(%ebx),%ebx` = *(pRoot+0x3c) = base + 0x14, not base + 0x08.
//  See NOT PINNED (3) and COIN-FLIP (4): the two readings are a second
//  container class whose one-caller GetSize icl inlined, or a plain
//  member of the singleton.  It is written here as the former.  (The
//  count asymmetry is at least consistent with the caller counts: a
//  GetSize with one caller is exactly what icl WOULD inline.)
// ---------------------------------------------------------------------- //
struct List1000a670                                     // count at +0x08
{
	SPoly5dca0**	sub_1000a670() const;               // 0x1000a670
	uint32			sub_1005f780() const;               // 0x1005f780

	uint32			m_n00;                              // +0x00
	SPoly5dca0**	m_pData;                            // +0x04
	uint32			m_nCount;                           // +0x08
};

struct List1005dcf4                                     // count at +0x14
{
	SPoly5dca0**	sub_1000a670() const;               // 0x1000a670

	uint32			m_n00;                              // +0x00
	SPoly5dca0**	m_pData;                            // +0x04
	uint8			m_pad_08[0xc];                      // +0x08 .. +0x13
	uint32			m_nCount;                           // +0x14
};

// 0x1007b8d0 -- `mov $0x10091a10,%eax ; ret`, the singleton accessor
// d3dr2a.cpp, d3dr5a_o2.cpp, d3dqip3.cpp and big64cc0.cpp all name.
// Only the container at +0x28 (and its count at +0x3c) is reached here.
struct Root1007b8d0
{
	uint8			m_pad_00[0x28];                     // +0x00 .. +0x27
	List1005dcf4	m_List28;                           // +0x28 .. +0x3f
};

// 0x1008beac -- d3dr3a.cpp names it `g_1008beac` and big124a4.cpp calls
// it "the light grid".  Only the container at +0x19c is reached here.
struct Grid1008beac
{
	uint8			m_pad_00[0x19c];                    // +0x00 .. +0x19b
	List1000a670	m_List19c;                          // +0x19c .. +0x1a7
};

extern "C" Root1007b8d0*	sub_1007b8d0();             // 0x1007b8d0


// ---------------------------------------------------------------------- //
//  Globals.  Every one is an address this function touches.
// ---------------------------------------------------------------------- //

// 0x1008ff40 -- the console variable **AllSkyPortals**, registered at
// 0x1005f766 (`mov $0x1008ff40,%ecx` with the name string 0x100aab80
// pushed one instruction earlier).  Read once, at 0x1005dccf.
extern uint32			g_nAllSkyPortals;                   // 0x1008ff40

// 0x1008beac -- tested for NULL at 0x1005dcec and dereferenced only in
// the sunk arm at 0x1005f40a/0x1005f41b.
extern Grid1008beac*	g_p1008beac;                        // 0x1008beac

// 0x1008d7a0 -- UseD3DClip.  `mov 0x1008d7a0,%eax ; test %eax,%eax` at
// 0x1005de1a/0x1005de25, the only site.  big0f6f0.cpp, big124a4.cpp,
// big24090.cpp and big051a0.cpp all name it uint32.
extern uint32			g_n1008d7a0;                        // 0x1008d7a0

// 0x1008b87c -- the clipper's output bank.  Read ONCE at 0x1005de45 and
// stored to BOTH pOut (-0x128) and pOutStart (-0x11c).
extern RVert5dca0*		g_p1008b87c;                        // 0x1008b87c

// 0x1008b908 / 0x1008b910 -- the near and far plane constants.  Each is
// used three ways: the classify comparand (`fcomps`), the numerator
// (`fsubrs`) and an INTEGER MOVE into pOut->m_vPos.z (0x1005dfd9 and
// 0x1005f059), which is icl copying a float lvalue with `mov`s.
extern float			g_f1008b908;                        // 0x1008b908
extern float			g_f1008b910;                        // 0x1008b910

// 0x1008b9dc and 0x1008ba1c -- the two 4x4 matrices, adjacent.
extern LTMat5dca0		g_Mat1008b9dc;                      // 0x1008b9dc
extern LTMat5dca0		g_Mat1008ba1c;                      // 0x1008ba1c

// 0x1008ff3c/0x1008ff38/0x1008ff34/0x1008ff30 -- the accumulated screen
// rectangle.  The CALLER hands them to 0x10020d80 and 0x1001ef30 in the
// order (ff3c, ff38, ff34, ff30) at 0x1005f5e0..0x1005f604, which is what
// names them min-x, min-y, max-x, max-y.  Declared as four floats rather
// than one struct because nothing in either function addresses them as a
// block; relocations are masked, so the layout is free.
extern float			g_fSkyMinX;                         // 0x1008ff3c
extern float			g_fSkyMinY;                         // 0x1008ff38
extern float			g_fSkyMaxX;                         // 0x1008ff34
extern float			g_fSkyMaxY;                         // 0x1008ff30

// 0x1008be44 -- incremented once per EXECUTED clip pass:
// `mov 0x1008be44,%edx ; inc %edx ; mov %edx,0x1008be44`, six sites, each
// inside its pass's `if (nClip & BIT)` and before the classify loop.
// big051a0.cpp names it the same way.
extern uint32			g_n1008be44;                        // 0x1008be44

// 0x1008be3c -- `incl` once per SURVIVING polygon, at 0x1005f39b, which
// a rejected polygon skips (note 5(a)).
extern uint32			g_n1008be3c;                        // 0x1008be3c


// The frame says fifty: each pass's flag array is 0xc8 = 200 bytes and
// the six arrays are exactly 0xdc apart (-0x674, -0x598, -0x4bc, -0x3e0,
// -0x304, -0x228).  The remaining 0x14 of each 0xdc is the five int slots
// the five byte lerps need -- pass 1's are -0x5ac..-0x59c, which is
// precisely -0x674 + 0xc8 onwards.
#define CLIP5DCA0_MAX	50

// The frame says forty: the destination array runs -0xb7c..-0x67d, which
// is 0x500 = 40 x 32 bytes, and the next named object is at -0x67c.
// There is NO overflow guard anywhere in the function -- the bound is
// enforced only by the frame, exactly as big70d10.cpp records for its 64.
#define VERTS5DCA0_MAX	40


// ---------------------------------------------------------------------- //
//  THE CLIPPER: d3dr3c_os.cpp's 0x1000f4b0 over the six
//  32-byte plane bodies 0x10001000 .. 0x10001fe0, all INLINED, exactly as
//  w55_10016324_os.cpp writes them -- the shipped code keeps pVerts /
//  nVerts / pOut in frame slots (-0x67c / -0x678 / -0x128) and reloads them
//  around every plane because their addresses are handed to the plane
//  bodies.  The earlier macro-expanded passes are in git history.
// ---------------------------------------------------------------------- //
#define CLIP_EPSILON    0.00001f
#define CLIP_MAX_VERTS  50

static inline void LerpRVert5dca0(RVert5dca0* a, RVert5dca0* b, RVert5dca0* dst, float t)
{
	dst->m_fU = (b->m_fU - a->m_fU) * t + a->m_fU;
	dst->m_fV = (b->m_fV - a->m_fV) * t + a->m_fV;
	dst->m_r  = (uint8)RoundF((b->m_r  - a->m_r)  * t + a->m_r);
	dst->m_g  = (uint8)RoundF((b->m_g  - a->m_g)  * t + a->m_g);
	dst->m_b  = (uint8)RoundF((b->m_b  - a->m_b)  * t + a->m_b);
	dst->m_a  = (uint8)RoundF((b->m_a  - a->m_a)  * t + a->m_a);
	dst->m_sa = (uint8)RoundF((b->m_sa - a->m_sa) * t + a->m_sa);
}


static inline float ClipEdgeNear(LTVec5dca0* a, LTVec5dca0* b, LTVec5dca0* dst)
{
	float fDenom = b->z - a->z;
	float t;

	if (fDenom < -CLIP_EPSILON || fDenom > CLIP_EPSILON)
		t = (g_f1008b908 - a->z) / fDenom;
	else
		t = 0.0f;

	dst->x = (b->x - a->x) * t + a->x;
	dst->y = (b->y - a->y) * t + a->y;
	dst->z = g_f1008b908;

	return t;
}


static inline float ClipEdgeLeft(LTVec5dca0* a, LTVec5dca0* b, LTVec5dca0* dst)
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


static inline float ClipEdgeTop(LTVec5dca0* a, LTVec5dca0* b, LTVec5dca0* dst)
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


static inline float ClipEdgeRight(LTVec5dca0* a, LTVec5dca0* b, LTVec5dca0* dst)
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


static inline float ClipEdgeBottom(LTVec5dca0* a, LTVec5dca0* b, LTVec5dca0* dst)
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


static inline float ClipEdgeFar(LTVec5dca0* a, LTVec5dca0* b, LTVec5dca0* dst)
{
	float fDenom = b->z - a->z;
	float t;

	if (fDenom < -CLIP_EPSILON || fDenom > CLIP_EPSILON)
		t = (g_f1008b910 - a->z) / fDenom;
	else
		t = 0.0f;

	dst->x = (b->x - a->x) * t + a->x;
	dst->y = (b->y - a->y) * t + a->y;
	dst->z = g_f1008b910;

	return t;
}


// 0x10001000
static inline LTBOOL sub_10001000(uint32* pUnused, RVert5dca0** ppVerts, int32* pnVerts, RVert5dca0** ppOut)
{
	uint32     anInside[CLIP_MAX_VERTS];
	RVert5dca0*  pOutStart;
	RVert5dca0*  pV;
	RVert5dca0*  pEnd;
	RVert5dca0*  pPrev;
	RVert5dca0*  pCur;
	uint32*    pFlag;
	int32      nInside;
	int32      i;
	int32      iPrev;

	++g_n1008be44;

	nInside = 0;
	pFlag   = anInside;
	pV      = *ppVerts;
	pEnd    = pV + *pnVerts;

	for (; pV != pEnd; ++pV)
	{
		uint32 bIn = (pV->m_vPos.z >= g_f1008b908);

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

				LerpRVert5dca0(pPrev, pCur, *ppOut, t);

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
static inline LTBOOL sub_10001330(uint32* pUnused, RVert5dca0** ppVerts, int32* pnVerts, RVert5dca0** ppOut)
{
	uint32     anInside[CLIP_MAX_VERTS];
	RVert5dca0*  pOutStart;
	RVert5dca0*  pV;
	RVert5dca0*  pEnd;
	RVert5dca0*  pPrev;
	RVert5dca0*  pCur;
	uint32*    pFlag;
	int32      nInside;
	int32      i;
	int32      iPrev;

	++g_n1008be44;

	nInside = 0;
	pFlag   = anInside;
	pV      = *ppVerts;
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

				LerpRVert5dca0(pPrev, pCur, *ppOut, t);

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
static inline LTBOOL sub_10001670(uint32* pUnused, RVert5dca0** ppVerts, int32* pnVerts, RVert5dca0** ppOut)
{
	uint32     anInside[CLIP_MAX_VERTS];
	RVert5dca0*  pOutStart;
	RVert5dca0*  pV;
	RVert5dca0*  pEnd;
	RVert5dca0*  pPrev;
	RVert5dca0*  pCur;
	uint32*    pFlag;
	int32      nInside;
	int32      i;
	int32      iPrev;

	++g_n1008be44;

	nInside = 0;
	pFlag   = anInside;
	pV      = *ppVerts;
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

				LerpRVert5dca0(pPrev, pCur, *ppOut, t);

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
static inline LTBOOL sub_10001990(uint32* pUnused, RVert5dca0** ppVerts, int32* pnVerts, RVert5dca0** ppOut)
{
	uint32     anInside[CLIP_MAX_VERTS];
	RVert5dca0*  pOutStart;
	RVert5dca0*  pV;
	RVert5dca0*  pEnd;
	RVert5dca0*  pPrev;
	RVert5dca0*  pCur;
	uint32*    pFlag;
	int32      nInside;
	int32      i;
	int32      iPrev;

	++g_n1008be44;

	nInside = 0;
	pFlag   = anInside;
	pV      = *ppVerts;
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

				LerpRVert5dca0(pPrev, pCur, *ppOut, t);

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
static inline LTBOOL sub_10001cb0(uint32* pUnused, RVert5dca0** ppVerts, int32* pnVerts, RVert5dca0** ppOut)
{
	uint32     anInside[CLIP_MAX_VERTS];
	RVert5dca0*  pOutStart;
	RVert5dca0*  pV;
	RVert5dca0*  pEnd;
	RVert5dca0*  pPrev;
	RVert5dca0*  pCur;
	uint32*    pFlag;
	int32      nInside;
	int32      i;
	int32      iPrev;

	++g_n1008be44;

	nInside = 0;
	pFlag   = anInside;
	pV      = *ppVerts;
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

				LerpRVert5dca0(pPrev, pCur, *ppOut, t);

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
static inline LTBOOL sub_10001fe0(uint32* pUnused, RVert5dca0** ppVerts, int32* pnVerts, RVert5dca0** ppOut)
{
	uint32     anInside[CLIP_MAX_VERTS];
	RVert5dca0*  pOutStart;
	RVert5dca0*  pV;
	RVert5dca0*  pEnd;
	RVert5dca0*  pPrev;
	RVert5dca0*  pCur;
	uint32*    pFlag;
	int32      nInside;
	int32      i;
	int32      iPrev;

	++g_n1008be44;

	nInside = 0;
	pFlag   = anInside;
	pV      = *ppVerts;
	pEnd    = pV + *pnVerts;

	for (; pV != pEnd; ++pV)
	{
		uint32 bIn = (pV->m_vPos.z <= g_f1008b910);

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

				LerpRVert5dca0(pPrev, pCur, *ppOut, t);

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
static inline LTBOOL sub_1000f4b0(uint32 nClipFlags, RVert5dca0** ppVerts, int32* pnVerts)
{
	RVert5dca0*     pOut;
	RVert5dca0*     pVerts;
	int32      nVerts;
	uint32     anUnused[6];

	if (g_n1008d7a0)
	{
		nClipFlags &= 1;
		if (!nClipFlags)
			return LTTRUE;
	}

	pOut   = g_p1008b87c;
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



// ======================================================================= //
//  0x1005dca0 .. 0x1005f577   (6,359 bytes + 9 padding, 4 calls)
//
//  __cdecl, no arguments, LTBOOL in %eax.  See section 2.
// ======================================================================= //
LTBOOL sub_1005dca0(void)
{
	Root1007b8d0*	pRoot;
	SPoly5dca0**	ppList;
	uint32			nPolys;
	uint32			i;
	uint32			nClipFlags;

	RVert5dca0		aVerts[VERTS5DCA0_MAX];
	RVert5dca0*		pVerts;
	int32			nVerts;

	// ------------------------------------------------------------------ //
	//  0x1005dca0..0x1005dcf3 -- prologue, the three register spills into
	//  the TOP of the frame, the four accumulator resets and the
	//  AllSkyPortals test.
	//
	//    1005dca3  sub  $0xc78,%esp
	//    1005dca9  mov  %edi,0x8(%esp) / %esi,0x4(%esp) / %ebx,(%esp)
	//    1005dcb4  call 0x1007b8d0            ; %eax = &g_10091a10
	//    1005dcb9  mov  $0x461c4000,%edx      ; DUMPED:  10000.0f
	//    1005dcbe  mov  $0xc61c4000,%ecx      ; DUMPED: -10000.0f
	//    1005dcc3  mov  %edx,0x1008ff3c
	//    1005dcc9  mov  %edx,0x1008ff38
	//    1005dccf  mov  0x1008ff40,%edx       ; AllSkyPortals
	//    1005dcd5  mov  %ecx,0x1008ff30
	//    1005dcdb  test %edx,%edx
	//    1005dcdd  mov  %ecx,0x1008ff34
	//    1005dce3  mov  %eax,%ebx             ; pRoot
	//    1005dce5  je   0x1005dcf4            ; the DEFAULT arm
	//    1005dce7  mov  0x1008beac,%eax
	//    1005dcec  test %eax,%eax
	//    1005dcee  jne  0x1005f40a            ; the SUNK AllSkyPortals arm
	//
	//  All four constants were DUMPED from the immediates, not inferred:
	//  0x461c4000 == 10000.0f and 0xc61c4000 == -10000.0f exactly.  Both
	//  live in a register across all four stores, which is why the two
	//  maxima come out transposed -- see NOT PINNED (4).
	// ------------------------------------------------------------------ //
	pRoot = sub_1007b8d0();

	g_fSkyMinX =  10000.0f;
	g_fSkyMinY =  10000.0f;
	g_fSkyMaxX = -10000.0f;
	g_fSkyMaxY = -10000.0f;

	// ------------------------------------------------------------------ //
	//  0x1005f40a..0x1005f43a (SUNK) and 0x1005dcf4..0x1005dd0c -- the two
	//  arms.  See COIN-FLIP (1) for the arm order and NOT PINNED (3) for
	//  the two containers.
	//
	//    -- sunk arm, AllSkyPortals set AND the grid present --
	//    1005f40a  mov  0x1008beac,%ecx
	//    1005f410  add  $0x19c,%ecx           ; /O2 `&p->m`, not lea
	//    1005f416  call 0x1000a670            ; *(+0x1a0) : the array
	//    1005f41b  mov  0x1008beac,%ecx       ; re-loaded, not kept
	//    1005f421  mov  %eax,%ebx
	//    1005f423  add  $0x19c,%ecx
	//    1005f429  call 0x1005f780            ; *(+0x1a4) : the count
	//    1005f42e  mov  $0x3d,%dl             ; 0x3f & ~0x02 -- NO FAR PLANE
	//    1005f430  mov  %dl,-0x124(%ebp)
	//    1005f436  jmp  0x1005dd0d
	//
	//    -- fall-through arm, the default list --
	//    1005dcf4  lea  0x28(%ebx),%ecx
	//    1005dcf7  call 0x1000a670            ; *(pRoot+0x2c) : the array
	//    1005dcfc  mov  0x3c(%ebx),%ebx       ; *(pRoot+0x3c) : the count
	//    1005dcff  mov  $0x3f,%dl             ; ALL SIX planes
	//    1005dd01  mov  %dl,-0x124(%ebp)
	//
	//  0x3f = 0x01|0x02|0x04|0x08|0x10|0x20, the six mask bits in the
	//  order section 3c lists them; 0x3d is the same with bit 0x02 (FAR)
	//  cleared.  A sky portal seen through the AllSkyPortals debug switch
	//  is not far-clipped.
	// ------------------------------------------------------------------ //
	switch (g_nAllSkyPortals && g_p1008beac)
	{
	case 0:
		ppList     = pRoot->m_List28.sub_1000a670();
		nPolys     = pRoot->m_List28.m_nCount;
		nClipFlags = 0x3f;
		break;
	default:
		ppList     = g_p1008beac->m_List19c.sub_1000a670();
		nPolys     = g_p1008beac->m_List19c.sub_1005f780();
		nClipFlags = 0x3d;
	}

	// ------------------------------------------------------------------ //
	//  0x1005dd0d..0x1005dd23 -- the empty-list guard and the loop head.
	//
	//    1005dd0d  test %eax,%eax
	//    1005dd0f  jbe  0x1005f3be            ; UNSIGNED: `0 < nPolys`
	//    1005dd15  mov  %ebx,-0xe0(%ebp)      ; ppWalk = ppList
	//    1005dd1b  lea  (%ebx,%eax,4),%ebx    ; ppEnd  = ppList + nPolys
	//    1005dd1e  mov  %ebx,-0x120(%ebp)
	//
	//  The `jbe` (not `jl`) is what makes nPolys UNSIGNED; the loop is
	//  fully strength-reduced to a cursor plus an end pointer, with no
	//  surviving `i`, which is what an indexed `for` gives (loops
	//  want the array indexed, not walked).  0x1005f3b0's
	//  `cmp %edx,%eax ; jb` is the matching unsigned pointer test.
	// ------------------------------------------------------------------ //
	for (i = 0; i < nPolys; i++)
	{
		SPoly5dca0*		pPoly;
		int32			j;

		pPoly = ppList[i];

		// -------------------------------------------------------------- //
		//  0x1005dd24..0x1005de03 -- THE FIRST TRANSFORM.
		//
		//    1005dd2c  mov  (%eax),%esi             ; pPoly
		//    1005dd2e  movzwl 0x50(%esi),%eax
		//    1005dd32  test %eax,%eax
		//    1005dd34  jle  0x1005de04              ; SIGNED: `0 < count`
		//    1005dd40  mov  0x54(%ecx),%eax         ; pVert->m_pPos
		//    ...       the four rows, w first
		//    1005dd68  fdivrs 0x100aab04            ; DUMPED: 1.0f
		//    1005dd94  fstps -0xb7c(%edx)           ; aVerts[j].x
		//    1005ddc0  fstps -0xb78(%edx)           ; aVerts[j].y
		//    1005ddec  add  $0x18,%ecx              ; SPolyVert stride
		//    1005ddef  fstps -0xb74(%edx)           ; aVerts[j].z
		//    1005ddf5  movzwl 0x50(%esi),%eax       ; RE-READ every trip
		//    1005ddf9  add  $0x20,%edx              ; RVert stride
		//    1005ddfc  cmp  %eax,%ebx ; jl 0x1005dd40
		//
		//  This is ltmatrix.h:222 `MatVMul_H(pDest, pMat, pSrc)` expanded
		//  inline -- the matrix rows come out at 0x1008b9dc/0x1008b9ec/
		//  0x1008b9fc/0x1008ba0c exactly as big44ff4.cpp read them, and
		//  the 1.0f at 0x100aab04 is its `one_over_w`.  /Ob1 expands
		//  anything the SDK marks `inline`, which is why there is no call.
		//  It is called through the header itself, the local vector and
		//  matrix types cast to the SDK's.
		//
		//  The count is re-read INSIDE the loop, so the member is written
		//  in the loop condition -- note 5(e).
		// -------------------------------------------------------------- //
		// (the header's MatVMul_H itself rather than its
		// body written out -- with the second transform below, 0.865 ->
		// 0.915 normalised instruction match.)
		for (j = 0; j < pPoly->m_u50; j++)
			MatVMul_H((LTVector*)&aVerts[j].m_vPos, (LTMatrix*)&g_Mat1008b9dc,
			          (LTVector*)pPoly->m_aVerts[j].m_pPos);

		// -------------------------------------------------------------- //
		//  0x1005de04..0x1005de44 -- the clipper's in/out pair, the clip
		//  mask, and the UseD3DClip early skip.
		//
		//    1005de04  lea  -0xb7c(%ebp),%eax
		//    1005de0a  mov  -0x124(%ebp),%dl        ; nClipFlags
		//    1005de10  mov  %eax,-0x67c(%ebp)       ; pVerts = aVerts
		//    1005de16  movzwl 0x50(%esi),%esi
		//    1005de1a  mov  0x1008d7a0,%eax
		//    1005de1f  mov  %esi,-0x678(%ebp)       ; nVerts = pPoly->m_u50
		//    1005de25  test %eax,%eax
		//    1005de27  mov  %dl,-0x110(%ebp)        ; nClip = nClipFlags
		//    1005de2d  je   0x1005de45
		//    1005de2f  mov  -0x124(%ebp),%al        ; RE-READ, see (5)
		//    1005de35  and  $0x1,%al
		//    1005de37  mov  %al,%ah
		//    1005de39  mov  %ah,-0x110(%ebp)
		//    1005de3f  je   0x1005f231              ; STRAIGHT to the
		//                                           ; accumulator, NOT a
		//                                           ; return -- note 5(b)
		// -------------------------------------------------------------- //
		pVerts = aVerts;
		nVerts = pPoly->m_u50;

		if (!sub_1000f4b0(nClipFlags, &pVerts, &nVerts))
			continue;


		// -------------------------------------------------------------- //
		//  0x1005f231..0x1005f39a -- THE SECOND TRANSFORM AND THE
		//  ACCUMULATOR.
		//
		//    1005f231  mov  -0x678(%ebp),%ecx        ; nVerts
		//    1005f237  mov  -0x67c(%ebp),%edx        ; pVerts
		//    1005f23d  shl  $0x5,%ecx
		//    1005f240  add  %edx,%ecx                ; pEnd = pVerts+nVerts
		//    1005f242  cmp  %ecx,%edx
		//    1005f244  je   0x1005f39b               ; empty: skip the loop
		//    1005f24a  flds 0x1008ba1c               ; m[0][0], HOISTED --
		//                                            ; the loop back-edge
		//                                            ; is 0x1005f250
		//    1005f274  fdivrs 0x100aab0c             ; DUMPED: 1.0f, the
		//                                            ; SECOND literal
		//    1005f2b2  fstps -0x14c(%ebp)            ; the projected x
		//    1005f2ee  fstps -0x148(%ebp)            ; the projected y
		//    1005f31c  fstps -0x144(%ebp)            ; the projected z --
		//                                            ; WRITTEN, NEVER READ
		//    1005f38e  add  $0x20,%edx ; cmp %ecx,%edx ; jne 0x1005f250
		//    1005f399  fstp %st(0)                   ; drop the hoisted
		//                                            ; m[0][0]
		//
		//  The matrix is the SECOND one, 0x1008ba1c: rows at +0x00
		//  (0x1008ba1c/ba20/ba24/ba28), +0x10 (ba2c/ba30/ba34/ba38), +0x20
		//  (ba3c/ba40/ba44/ba48) and +0x30 (ba4c/ba50/ba54/ba58).  The
		//  dead z is what says this is the header's three-component
		//  MatVMul_H and not a hand-written x/y projection.
		//
		//  THE FOUR ACCUMULATIONS, traced through the x87 stack one
		//  instruction at a time:
		//    0x1005f2f4  flds 0x1008ff3c ; ... ; fcom %st(1) with the
		//                GLOBAL in %st(0) and the candidate in %st(1);
		//                0x1005f323 `jae` -> the sunk arm 0x1005f570
		//                (`fstp %st(1)`, i.e. KEEP the candidate), so the
		//                candidate wins when global >= candidate;
		//    0x1005f331  flds 0x1008ff38 ; flds -0x148 ; fcom %st(1) with
		//                the CANDIDATE in %st(0); 0x1005f342 `jbe` ->
		//                0x1005f560, candidate wins when candidate <=
		//                global;
		//    0x1005f350  the same for 0x1008ff34 with `jae` -> 0x1005f550,
		//                candidate wins when candidate >= global;
		//    0x1005f36f  the same for 0x1008ff30 with `jae` -> 0x1005f547.
		//  In every one of the four the "take the candidate" arm carries
		//  the INCLUSIVE comparison, which is the `else` arm of
		//  `((a) < (b)) ? (a) : (b)` -- so the GLOBAL is `a` and the
		//  candidate is `b` in all four.  See COIN-FLIP (2).
		// -------------------------------------------------------------- //
		{
			RVert5dca0*		pWalk;
			RVert5dca0*		pEnd;

			pEnd = pVerts + nVerts;

			for (pWalk = pVerts; pWalk != pEnd; pWalk++)
			{
				// THE FRAME.  The projected point lives in
				// memory (-0x14c/-0x148/-0x144, z stored and never read):
				// an LTVector there is scalar-replaced by icl (no slots,
				// z's store deleted, frame 0x10 short) whether declared
				// here or at function scope, spelled through operator*
				// or `.x`, or a union; through its operator[] it stays in
				// memory but the frame top shrinks by 0x20.  A float[3]
				// is the one spelling measured that keeps it and the
				// slots exactly where the shipped frame has them.
				float	vT[3];

				MatVMul_H((LTVector*)vT, (LTMatrix*)&g_Mat1008ba1c, (LTVector*)&pWalk->m_vPos);

				g_fSkyMinX = LTMIN(g_fSkyMinX, vT[0]);
				g_fSkyMinY = LTMIN(g_fSkyMinY, vT[1]);
				g_fSkyMaxX = LTMAX(g_fSkyMaxX, vT[0]);
				g_fSkyMaxY = LTMAX(g_fSkyMaxY, vT[1]);
			}
		}

		// -------------------------------------------------------------- //
		//  0x1005f39b..0x1005f3bd -- the survivor counter and the loop
		//  increment.  0x1005f3a1 is the target of all TWELVE clip
		//  rejects, so a rejected polygon does NOT reach the `incl`.
		//
		//    1005f39b  incl 0x1008be3c
		//    1005f3a1  mov  -0xe0(%ebp),%eax
		//    1005f3a7  mov  -0x120(%ebp),%edx
		//    1005f3ad  add  $0x4,%eax
		//    1005f3b0  cmp  %edx,%eax
		//    1005f3b2  mov  %eax,-0xe0(%ebp)
		//    1005f3b8  jb   0x1005dd24
		// -------------------------------------------------------------- //
		g_n1008be3c++;
	}

	// ------------------------------------------------------------------ //
	//  0x1005f3be..0x1005f409 -- the two size tests and the ONE epilogue.
	//
	//    1005f3be  flds 0x1008ff34 ; fsubs 0x1008ff3c
	//    1005f3ca  fcomps 0x100aab10 ; jbe 0x1005f540    ; DUMPED: 0.9f
	//    1005f3d9  flds 0x1008ff30 ; fsubs 0x1008ff38
	//    1005f3e5  fcomps 0x100aab14 ; jbe 0x1005f540    ; DUMPED: 0.9f
	//    1005f3f4  mov  $0x1,%edx
	//    1005f3f9  mov  %edx,%eax                        ; the ONE exit
	//    1005f3fb  mov  (%esp),%ebx / 0x4(%esp),%esi / 0x8(%esp),%edi
	//    1005f406  mov  %ebp,%esp ; pop %ebp ; ret
	//    -- SUNK --
	//    1005f540  xor  %edx,%edx ; jmp 0x1005f3f9
	//
	//  0x100aab10 and 0x100aab14 were DUMPED from the image: both are
	//  `66 66 66 3f` = 0.899999976f, i.e. the float literal 0.9f, and they
	//  are TWO separate constants for two separate comparisons.  The
	//  empty-list guard at 0x1005dd0f jumps straight here, so a frame with
	//  no sky portals at all reaches these tests with the rectangle still
	//  at its initial values: maxX - minX is -10000 - 10000 = -20000,
	//  which fails the first `> 0.9f` and returns LTFALSE.  The
	//  initialisation is chosen so that the empty case falls out of the
	//  same two tests -- there is no separate empty-list return.
	//  See COIN-FLIP (6) for the two-statement spelling.
	// ------------------------------------------------------------------ //
	{
		// The last 8 bytes of the 0xc78 frame.  With the
		// one-expression return the frame is 0xc70 (every slot 8 high);
		// these two locals give 0xc78 and the shipped slots, at the cost
		// of one `fstpl/fldl -0x10` the shipped code does not have (the
		// height is computed before the first test).  The two-`if` form
		// (COIN-FLIP 6), a result variable, and an unused 1/w local all
		// stay at 0xc70.
		float	fWidth, fHeight;

		fWidth  = g_fSkyMaxX - g_fSkyMinX;
		fHeight = g_fSkyMaxY - g_fSkyMinY;

		return (LTBOOL)(fWidth > 0.9f && fHeight > 0.9f);
	}
}


// *********************************************************************** //
//  TWO WARNINGS THIS SOURCE IS EXPECTED TO PRODUCE, both original.
//
//  * `fVal` unused in the pass-1 and pass-6 expansions of
//    CLIP5DCA0_HEAD.  Those two planes pin z to a constant rather than
//    interpolating it, so the macro's `fVal` has no use there.  The same
//    warning is implicit in big051a0.cpp's identical macro.
//  * `fZ` set but not used in the accumulator.  The shipped code
//    COMPUTES and STORES the projected z (`fstps -0x144(%ebp)` at
//    0x1005f31c) and never reads it back; that dead store is what says
//    the source called the three-component MatVMul_H rather than
//    projecting x and y by hand.  Removing it would delete six shipped
//    instructions.
//
//  EXPECTED CALL SEQUENCE:  shipped 4, ours 4, all four direct and in this
//  order -- 0x1007b8d0, 0x1000a670, 0x1000a670, 0x1005f780.  Ours emits
//  them in the source order (0x1007b8d0 first, then the if/else arms), so
//  the two arms' calls may transpose against the shipped order if icl
//  sinks the cold arm and it reads last; that would show as one
//  shipped-only and one ours-only row, both resolvable to the same
//  0x1000a670/0x1005f780 pair.  NO CRT-intrinsic residual is expected:
//  there is no sqrt, fabs or tan anywhere in the function.  And remember
//  section 3 -- 98.6% of the bytes are call-free and this figure
//  witnesses almost none of them.
// *********************************************************************** //
