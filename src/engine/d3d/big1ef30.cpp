// *********************************************************************** //
//
//  0x1001ef30 5954 -> 4688 bytes differing (agreement 0.139
//  -> 0.313), frame 0x7a8 -> 0x6a0 against the shipped 0x6b0; still
//  attempted.  Two changes, each measured:
//    * 0x1001e140 is LTMatrix::Init with SIXTEEN FLOAT arguments (d3dlow's
//      C1001e140, as w3_10014580_os.cpp and w29_10027f70.cpp call it), not
//      a by-value LTMat: the shipped code builds the six matrices (mTrans,
//      mScale, the shear, mFov, mInvFar, mZ) straight into the outgoing
//      argument area, with no local `mat` and no `rep movsl` into it.
//      That alone: 5954 -> 4541, frame 0x7a8 -> 0x668.
//    * the near/far cap points are the SDK LTVector's `vDir * fDist`
//      (operator* returning a temporary, copied with `rep movsl` %ecx = 3
//      into vPoint, as shipped at 0x1002094f / 0x10020a98), then m_vPos
//      added in place: the shipped -0x1d8 / -0x1c0 temporaries and the
//      thirteen `rep movsl`.  Frame 0x668 -> 0x6a0 (the fistp scratch
//      lands on the shipped -0x1b0), bytes 4541 -> 4688.
//  With the frame right, the first 0x25+ bytes would agree (every slot
//  above -0x1b0 already lines up).  The 0x10 left is vDir: the shipped
//  code copies mRot's row 2 into a MEMORY vDir at -0x1e4 (three integer
//  `mov`s, z/y/x) and reads it back; icl here scalar-replaces it into x87
//  doubles.  Not moved by: vDir.Init(...), the LTVector constructor, a
//  constructed temporary, vDir at function scope, `m_vPos + vDir * f`
//  through operator+, or the cap block as an inline taking vDir by value
//  (all 0x6a0); `vDir = *(LTVector*)&mRot.m[2][0]` puts it in memory but
//  as a `rep movsl` copy and the frame drops to 0x698.
//
//  THIS FILE IS NOW BUILT /Os /Oi- /Qip (D3D_QIP_OS_OI_TUS),
//  not /O2 /Oi-, and holds 0x1001e600 and 0x1001e8d0/d4, the two
//  register-ABI helpers 0x1001ef30 calls (moved in from w4_1001e4a0_os.cpp).
//  Measured on the combined file:
//      /O2 /Oi- /Qip /Qip_no_inlining   helpers 3 of 3, 0x1001ef30 8117 differing
//      /Os /Qip /Qip_no_inlining        helpers 3 of 3, 0x1001ef30 6612
//      /Os /Oi- /Qip                    helpers 3 of 3, 0x1001ef30 5954
//  against 8202 for 0x1001ef30 alone at /O2 /Oi-.  The "/O2" verdicts
//  below were reached before the /Os /Qip shapes were understood; the
//  byte counts above are the measurement.
//
// *********************************************************************** //
// *********************************************************************** //
//
//  MODULE:   BIG1EF30 (.CPP)
//
//  PURPOSE:  0x1001ef30 .. 0x10020bf9 -- d3d.ren's CAMERA / VIEWPORT
//            SET-UP.  Given a screen rectangle, a camera matrix, a 28-byte
//            lens block and a scale vector it fills the whole front half of
//            the 0x4f8-byte transform object at 0x1008b880 (and its twin at
//            0x1008ff60): the integer and float viewport rectangles, the
//            two half-angle FOVs, the eleven derived screen-mapping floats,
//            eight 4x4 matrices, the six frustum planes and the two
//            transformed clip corners.
//
//            Reconstructed instruction by instruction from the retail d3d.ren
//            under a "structurally right, not byte-exact" bar:
//            same control flow, same callees in the same order, same field
//            offsets, same constants.  Nothing here is guessed; anything
//            that could not be read off the bytes is named by its address
//            and every coin-flip is at the bottom with both spellings.
//
//  ===================================================================== //
//  1.  THE EXTENT.  THE ROW IS RIGHT.
//  ===================================================================== //
//
//  Dispatched as "0x1001ef30, 7,372 bytes" -- i.e. 0x1001ef30..0x10020bfb
//  inclusive, end 0x10020bfc.  Every one of the six checks passes:
//
//    * ONE `ret` in the range, at 0x10020bf9, and it is the last
//      instruction of the only epilogue:
//          10020bf0  33 c0     xor  %eax,%eax
//          10020bf2  40        inc  %eax          <- `return 1;`
//          10020bf3  5b 5e 5f  pop  %ebx/%esi/%edi
//          10020bf6  8b e5     mov  %ebp,%esp
//          10020bf8  5d        pop  %ebp
//          10020bf9  c3        ret
//      No duplicated /O2 epilogue, no second function.
//
//    * ONE `push %ebp ; mov %esp,%ebp` prologue, at 0x1001ef30 itself.
//      There is no lea-nop padding anywhere inside the range.
//
//    * THE FIRST INSTRUCTION IS THE PROLOGUE, not a /Qip stack entry.
//      0x1001ef30 is `55 8b ec 81 ec b0 06 00 00` -- push %ebp, mov, sub
//      $0x6b0 -- with no `mov 0x4(%esp),%eax` lead-in.  And it is not
//      itself a register entry with a stack entry above it: 0x1001ef2d,
//      0x1001ef2e and 0x1001ef2f are three `90` pad bytes and 0x1001ef28
//      is the previous function's `jmp 0x1001ee29`.
//
//    * THE END IS NOT MID-INSTRUCTION.  0x10020bfa..0x10020bfb is `8b f6`
//      (the two-byte `mov %esi,%esi` pad), 0x10020bfc..0x10020bff are four
//      `90`, and 0x10020c00 is the next function (`sub $0x84,%esp`).  So
//      the FUNCTION is 0x1001ef30..0x10020bf9, 7,370 bytes, and the row's
//      two extra bytes are its alignment pad.  Harmless either way -- the
//      call count is identical over both ranges.
//
//    * EVERY BRANCH TARGET LANDS INSIDE THE RANGE AND ON AN INSTRUCTION
//      BOUNDARY.  There are only ELEVEN jumps in 7,370 bytes (this is
//      almost pure straight-line code); all eleven were checked against
//      the instruction-address set of the linear sweep and all eleven
//      pass.  The sweep itself never desynchronises: objdump emits no
//      `.byte` and no `(bad)` anywhere in the range.
//
//    * CALL COUNT.
//          0x1001ef30..0x10020bfc:
//          7,372 bytes, 39 calls (39 direct, 0 indirect), 189 bytes/call
//          0x1001ef30..0x10020bfa:
//          7,370 bytes, 39 calls (39 direct, 0 indirect), 189 bytes/call
//      The 39, all direct, over SEVENTEEN distinct
//      callees.
//
//  ===================================================================== //
//  2.  THE ACCEPTANCE FIGURE AND WHAT IT DOES *NOT* WITNESS.  READ FIRST.
//  ===================================================================== //
//
//  189 bytes per call over 17 distinct callees is the strong band for
//  the call check, and it is the right instrument for the FIRST 1,900
//  BYTES.  It is blind to most of the rest, and that has to be said with
//  the figure and not after it:
//
//      0x1001ef30 .. 0x1001f6a8   1,912 B   33 of the 39 calls
//      0x1001f6a8 .. 0x100205dc   3,892 B   ZERO calls   <-- 53% of the
//                                                            function
//      0x100205dc .. 0x1002073b     351 B   4 calls (all four are the CRT
//                                                    sqrt, 0x1007fc98)
//      0x1002073b .. 0x10020bc5   1,162 B   ZERO calls
//      0x10020bc5 .. 0x10020bf9      52 B   2 calls
//
//  So 5,054 bytes -- 69% -- carry no call at all, and they are the three
//  inlined 4x4 MatMuls and the six-plane extraction.  What pins THOSE is
//  three other instruments, and all three are used below:
//
//    (a) THE LITERAL POOL AS A STATEMENT-ORDER WITNESS.  This function
//        owns 0x100a5120..0x100a5374 outright -- 0x100a511c is the last
//        slot the PREVIOUS function reads (at 0x1001ee0e) and 0x100a5378
//        the first the NEXT one reads (at 0x10020dc3).  149 slots, and
//        they are a transcript: see section 4.  Four separate matrix
//        literals match their pool runs element for element, which is
//        what fixes row-major source order, and the two `atan` statements,
//        the two 0.5f statements, the four `1.0f/x` statements and the two
//        1e-4f statements are all ordered by it.
//
//    (b) FRAME ARITHMETIC THAT DECOMPOSES EXACTLY.  `sub $0x6b0` = 1,712
//        bytes and every byte is named in section 3.
//
//    (c) AN INSTRUCTION CENSUS WITH NO RESIDUE for the MatMul region --
//        section 5.
//
//  ===================================================================== //
//  3.  THE SIGNATURE, READ OFF THE THREE CALL SITES
//  ===================================================================== //
//
//  Exactly three `call 0x1001ef30` exist in .text (found by scanning every
//  E8 in the section): 0x10020d6f, 0x10027cd7 and 0x1005f71b.  All three
//  build TEN outgoing dwords at (%esp)..0x24(%esp) and none of them
//  restores %esp for the callee, so it is __cdecl with a 40-byte argument
//  list.  The callee reads 0x8(%ebp)..0x2c(%ebp) and returns 1.
//
//      arg0  0x08(%ebp)  the transform object.  0x10027cd7 pushes the
//                        immediate 0x1008b880 (`movl $0x1008b880,(%esp)`)
//                        and 0x1005f71b pushes 0x1008ff60.
//      arg1  0x0c(%ebp)  a 28-byte lens block, by pointer.  At 0x10020d6f
//                        it is `lea 0x28(%esp)`, the local that
//                        0x10020e80 filled two instructions earlier; at
//                        0x10027cd7 it is `lea -0x244(%ebp)`, which
//                        big26728.cpp's frame map calls "the 28-byte block
//                        0x10020d80 fills".  memcpy'd wholesale into
//                        arg0 at 0x1001efc9 with a literal 0x1c.
//      arg2  0x10(%ebp)  float  fMinX      0x1005f6b8 pushes 0x1008ff3c
//      arg3  0x14(%ebp)  float  fMinY      0x1005f6c8 pushes 0x1008ff38
//      arg4  0x18(%ebp)  float  fMaxX      0x1005f6df pushes 0x1008ff34
//      arg5  0x1c(%ebp)  float  fMaxY      0x1005f6ec pushes 0x1008ff30
//                        -- the four sky-portal screen-extent accumulators
//                        big5dca0.cpp names, in that order.  big26728.cpp
//                        pushes -0x25c/-0x258/-0x250/-0x24c out of its
//                        six-float bounding box, i.e. min.x/min.y/max.x/
//                        max.y, which agrees.
//      arg6  0x20(%ebp)  const LTMatrix*   `lea -0x2a4(%ebp)` at
//                        0x10027c7a, which big26728.cpp's frame map calls
//                        `mFull`.  Proved a 4x4 from the inside: the very
//                        first thing done with it is
//                        `rep movsl` with %ecx = 0x10 into arg0 + 0x9c.
//      arg7..9  0x24/0x28/0x2c(%ebp)  an LTVector BY VALUE.  Both
//                        0x10027cd7 and 0x1005f71b build it with the
//                        three-`mov` 12-byte struct copy into 0x1c(%esp);
//                        0x10020d6f uses `rep movsl` %ecx = 3.  At two of
//                        the three sites it is (1,1,1).
//
//  RETURN: `xor %eax,%eax ; inc %eax` -- 1.  0x1005f720 ignores it,
//  0x10020d74 ignores it and returns 1 of its own.
//
//  ===================================================================== //
//  4.  THE LITERAL POOL, 0x100a5120 .. 0x100a5374, READ IN ORDER
//  ===================================================================== //
//
//  icl gives every float literal OCCURRENCE its own dword, in source
//  order, including the ones it then folds into a `movl $imm` and never
//  loads.  Slots this function actually loads are marked (*) with the
//  loading address.  The runs marked "== matrix" matched a predicted
//  row-major literal sequence element for element BEFORE the source below
//  was written; that is the single strongest piece of evidence in this
//  file for the call-free regions.
//
//    5120  3.0f       (*) 0x1001f004  fcomps -- the low clamp's test
//    5124  3.0f           folded to `movl $0x40400000` at 0x1001f024
//    5128  500000.0f  (*) 0x1001f034  fcomps -- the high clamp's test
//    512c  500000.0f      folded to `movl $0x48f42400` at 0x1001f03f
//    5130  1.0f       (*) 0x1001f0c6  m_f30 = (float)m_n24 - 1.0f
//    5134  1.0f       (*) 0x1001f0b1  m_f38 = (float)m_n28 - 1.0f
//        ^ the USE order is 5134 then 5130, the POOL order 5130 then 5134,
//          so the SOURCE order is m_f30's statement then m_f38's and the
//          scheduler swapped them.  That is what fixes the four
//          m_f2c/m_f30/m_f34/m_f38 assignments as ascending.
//    5138 513c 5140   0,0,0        SetTranslation(0.0f, 0.0f, 0.0f)
//    5144 .. 5174     1,0,0,0,1,0,0,0,1,0,0,0,1     == mTrans   (13)
//    5178 .. 51a8     0 x12 then 1                  == mScale   (13)
//    51ac  1.0f       (*) 0x1001f275  1.0f / pIn->m_fFocal
//    51b0 .. 51e4     1,0,0,0,1,0,0,0,1,0,0,0,0,1   == mSkew    (14)
//    51e8  1.0f           unloaded -- see the note on divisions below
//    51ec  1.0f           unloaded
//    51f0  3.14159274f(*) 0x1001f316   m_fFovX = MATH_PI - 2.0f*atan(..)
//    51f4  2.0f       (*) 0x1001f310
//    51f8  3.14159274f(*) 0x1001f335   m_fFovY = MATH_PI - 2.0f*atan(..)
//    51fc  2.0f       (*) 0x1001f32f
//    5200 .. 5234     0 x8, 1, 0 x4, 1              == mFov     (14)
//    5238  0.5f       (*) 0x1001f49c   m_fHalfW = 0.5f * m_fW
//    523c  0.5f       (*) 0x1001f466   m_fHalfH = 0.5f * m_fH
//    5240  1.0f           unloaded
//    5244  1.0f       (*) 0x1001f4da   m_fInvHalfW = 1.0f / m_fHalfW
//    5248  1.0f           unloaded
//    524c  1.0f       (*) 0x1001f4d1   m_fInvHalfH = 1.0f / m_fHalfH
//    5250  1.0f           unloaded
//    5254  1.0f       (*) 0x1001f4b8   m_fInvSx    = 1.0f / m_fSx
//    5258  1.0f           unloaded
//    525c  1.0f       (*) 0x1001f482   m_fInvSy    = 1.0f / m_fSy
//        ^ the four USED slots ascend 5244 < 524c < 5254 < 525c in exactly
//          the order the four members ascend, 0x64 0x68 0x7c 0x80, which
//          is what fixes the eleven screen-mapping assignments as one
//          ascending run even though the scheduler emits them interleaved.
//    5260  1e-4f      (*) 0x1001f523   mScreen.m[0][0]
//    5264  1e-4f      (*) 0x1001f529   mScreen.m[1][1]
//    5268  1.0f           unloaded
//    526c  1.0f       (*) 0x1001f5cf   1.0f / pIn->m_fFar
//    5270 .. 52a0     0 x12 then 1                  == mInvFar  (13)
//    52a4  1.0f       (*) 0x1001f645   the numerator of fQ
//    52a8  1.0f       (*) 0x1001f63d   the `1.0f -` inside fQ
//    52ac .. 52e4     1,0,0,0,0,1,0,0,0,0,[1],0,0,1,0   == mZ   (15)
//        ^ ONE literal falls between m[2][1] and m[3][0]: 52d4, loaded at
//          0x1001f649.  So m[2][2] carries exactly one 1.0f and m[2][3]
//          carries none.  See COIN-FLIP (E).
//    52e8  1.0f       (*) 0x10020347   1.0f / pLens->m_fFocal, the
//                                       corner table's reciprocal
//    52ec 52f0 52f4   0,0,0        m_avCorners[0] = (0,0,0)
//    52f8 .. 5330     THE SIX-PLANE TABLE, fifteen literals, matched
//                     value for value:
//                       52f8 52fc 5300 5304   0, 0,  1.0,  1.0   plane[0]
//                       5308 530c 5310        0, 0, -1.0        plane[1]
//                                             (its .d is -m_fFar)
//                       5314 5318             plane[2].y, plane[2].d
//                       531c 5320             plane[3].x, plane[3].d
//                       5324 5328             plane[4].y, plane[4].d
//                       532c 5330             plane[5].x, plane[5].d
//                     -- and the LAST FOUR of those, all 0.0f and all
//                     folded into `mov %eax` stores that never load them,
//                     are exactly the slots the four normalisation
//                     guards' `flds 0.0f` point at (0x100205e3,
//                     0x10020658, 0x100206cd, 0x10020742).  icl reused
//                     the dead plane-distance constants for the
//                     comparisons rather than emitting four more.
//    5334 .. 5340     0,0,0,0
//    5344  0.01f          folded to `movl $0x3c23d70a,0x43c(%ebx)` at
//                         0x10020bcd -- the 0.01f big5dca0.cpp records
//                         the sky-portal caller handing over
//    5348 .. 5374     twelve 1.0f, of which four are loaded, by the four
//                     `1.0f / fMag` normalisations, in REVERSE program
//                     order: 5368 by the first (0x100205f3), 5360 by the
//                     second, 5358 by the third, 5350 by the fourth.
//                     One of the twelve is `m_f4d0 = 1.0f`.  This tail is
//                     NOT in program order -- the 0.01f at 5344 belongs
//                     to the very last statement in the function -- and
//                     it is the one run of the pool that is not used as
//                     evidence below.
//
//  ONE HONEST GAP IN THE ACCOUNTING.  Every `1.0f / <memory operand>` in
//  this function occupies TWO consecutive pool slots and loads the second
//  (5240/5244, 5248/524c, 5250/5254, 5258/525c, 5268/526c) -- but the two
//  at 0x1001f275 and inside fQ occupy one each, and 51e8/51ec are two
//  spare 1.0f with no statement to attribute them to other than the two
//  divisions `m_fSx = f8/fc` and `m_fSy = f8/f10` that sit exactly there.
//  The most economical rule that fits is "a float division emits one extra
//  1.0f", with the two exceptions unexplained.  It does not affect any
//  ORDER conclusion above -- every one of those rests on which slot is
//  loaded and in what sequence -- so it is recorded and left open rather
//  than smoothed over.
//
//  ===================================================================== //
//  5.  THE FRAME.  `sub $0x6b0` = 1,712 bytes, and it decomposes exactly
//  ===================================================================== //
//
//  The prologue is `push %ebp ; mov %esp,%ebp ; sub $0x6b0,%esp ;
//  push %edi ; push %edi ; push %esi ; push %ebx` -- the duplicated
//  `push %edi` is icl's, not the source's.  %ebx holds arg0 for the whole
//  body.
//
//    -0x670  64 B  mRot     -- arg6 copied in (`rep movsl` %ecx=0x10),
//                              its translation cleared by 0x1001e4a0 and
//                              its 3x3 transposed by 0x1001e420.  Still
//                              live at 0x100208f6, 5.6 KB later, where
//                              row 2 is read out as the view direction.
//    -0x630  64 B  mTrans   -- translate(-m_vPos), built element by
//                              element and handed to 0x1001e140
//    -0x5f0  64 B  mView    = mRot * mTrans      (0x10014720)
//    -0x5b0  64 B  mScale   -- diag(vScale.x, vScale.y, vScale.z, 1)
//    -0x570  64 B  mFov     -- diag(m_fAspX, m_fAspY, 1, 1)
//    -0x530  64 B  operator* return temp -> m_Mat29c
//    -0x4f0  64 B  operator* return temp -> m_Mat15c
//    -0x4b0  64 B  operator* return temp -> m_Mat2dc
//    -0x470  64 B  mDepth   -- built by 0x1001e600
//    -0x430  64 B  mScreen
//    -0x3f0  64 B  operator* return temp -> m_Mat19c
//    -0x3b0  64 B  operator* return temp -> m_Mat1dc
//    -0x370  64 B  mInvFar  -- diag(k, k, k, 1), k = 1/pIn->m_fFar
//    -0x330  64 B  mZ       -- the z-range row
//    -0x2f0  64 B  mA = mZ * mInvFar        (inlined MatMul #1)
//    -0x2b0  64 B  mB = mA * mFov           (inlined MatMul #2)
//    -0x270  64 B  mC = mB * m_Mat25c       (inlined MatMul #3)
//    -0x230  12 B  the corner-transform temp
//    -0x224  64 B  mT -- m_MatWorld copied and 3x3-transposed IN LINE
//    -0x1e4  12 B  vDir   (mRot row 2)
//    -0x1d8  12 B  the scaled-direction temp
//    -0x1cc  12 B  vPoint
//    -0x1c0  12 B  the second scaled-direction temp
//    -0x1b0   4 B  the `fistpl` scratch 0x10015190's result goes through
//    -0x19c .. -0x181   seven dwords: icl's copies of the incoming
//                       pointers and floats
//    -0x180 .. -0x161   FOUR DOUBLES: fMaxX, fMaxY, fMinX, fMinY widened
//                       at entry (0x1001ef60..0x1001ef7b) because they are
//                       needed after eight calls
//    -0x160 .. -0x008   forty-four 8-byte x87 double spill slots inside
//                       the three MatMuls.  Every one is written `fstpl`
//                       and read `fldl`, i.e. in DOUBLE precision, which
//                       icl only does for unnamed temporaries.
//
//  ===================================================================== //
//  6.  INSTRUCTION CENSUS OF THE CALL-FREE MATMUL REGION
//      0x1001f6a8 .. 0x100205dc, 3,892 bytes, zero calls
//  ===================================================================== //
//
//  Counted with objdump over exactly that range; nothing is left over.
//
//    MULTIPLIES.  fmuls 144 + fmull 40 + fmul %st 23 + fmulp 1 = 208.
//        3 MatMuls x 16 elements x 4 terms                        192
//        the four corner products at 0x10020398..0x100203ae         4
//        the corner loop body, 3 components x 3 terms               9
//        the three `fmul %st(0),%st` squares of plane[2]            3
//                                                          total  208
//
//    ADDS.  faddp 148 + fadds 5 + faddl 4 = 157.
//        3 MatMuls x 16 elements x 3 adds                          144
//        the corner loop body, 3 components x 2 adds                 6
//        the corner loop's three `m_vPos.x + v.x`                    3
//        the two `halfH + offY` / `halfW + offX` plane terms          2
//        plane[2]'s two `faddp` of the squares                        2
//                                                          total   157
//
//    FLOAT STORES.  fstps 48 + fsts 21 = 69.
//        3 MatMuls x 16 elements                                    48
//        the corner loop body: 3 components + 3 offset stores         6
//        the eight corner-component stores at 0x100203b2..0x100203ee  8
//        the seven plane words that are not integer `mov`s            7
//                                                          total    69
//
//    AND THE REST, each with exactly one home:
//        rep movsl  2   the mC -> m_Mat11c copy (%ecx = 0x10) and the
//                       corner loop's `m_avCorners[i] = v` (%ecx = 3)
//        fdivrs     1   1.0f / pIn->m_fFocal at 0x10020345
//        fsubs      2   `offX - halfW` and `offY - halfH`
//        fchs       6   two in the corner table, four in the plane table
//        cmp/ja     1/1 the corner loop's single bound test
//        fstpl 44 / fldl 58   the double spill traffic, all inside the
//                             three MatMuls
//
//  A fourth MatMul, a fifth or seventh corner, or a plane built with a
//  different sign would move at least three of those rows.
//
//  ===================================================================== //
//  7.  WHAT THE FUNCTION IS, AND HOW EACH PIECE WAS READ
//  ===================================================================== //
//
//  (a) 0x1001ef30..0x1001effe  m_n4d4 = 0, m_Mat21c.Identity(), the
//      camera matrix copied to +0x9c, the lens block memcpy'd to +0x00,
//      m_vPos = arg6's translation, and m_vPos handed to 0x1001e360 by
//      value.  The `movl $0x0,0x4d4` sits BETWEEN the `lea 0x21c(%ebx)`
//      and the `call 0x100156e0`, and a store cannot cross a call in
//      either direction, so its position is FIRM, not a schedule.
//
//  (b) 0x1001effe..0x1001f05f  the far-plane clamp.
//          m_fFar = pIn->m_fFar;
//          if (3.0f > m_fFar)        m_fFar = 3.0f;
//          if (m_fFar > 500000.0f)   m_fFar = 500000.0f;
//      Two literals per bound in the pool (5120/5124 and 5128/512c) is
//      what says each bound is written TWICE -- once in the test, once in
//      the assignment -- rather than as big17c14's D3D_CLAMPF, whose low
//      arm skips the high test.  Here the low arm FALLS THROUGH into the
//      high test (0x1001f02e reloads m_fFar and drops into 0x1001f034),
//      which only two separate `if`s produce.
//
//  (c) 0x1001f060..0x1001f10a  eight calls to 0x10015190, the
//      `flds ; fistpl` round-to-nearest big17c14.cpp names.  The first
//      four round the four incoming rectangle floats into +0x1c..+0x28;
//      the second four round +0x2c..+0x38 into +0x3c..+0x48.  The
//      -1.0f on two of the four float forms is the pixel-inclusive
//      right/bottom edge.
//
//  (d) 0x1001f10a..0x1001f16b  mRot: a copy of arg6, SetTranslation(0,0,0)
//      (0x1001e4a0, `ret $0xc`, writes +0xc/+0x1c/+0x2c), then
//      0x1001e450 -> 0x1001e4d0 which fills three vectors from the three
//      COLUMNS, and then the 3x3 transpose 0x1001e420.  The argument
//      order at 0x1001f14d..0x1001f156 puts +0x49c first, +0x490 second
//      and +0x4a8 third, and 0x1001e4d0 fills its first argument from
//      column 0, so +0x49c is column 0, +0x490 column 1 and +0x4a8
//      column 2.
//
//  (e) 0x1001f16b..0x1001f1f2  mTrans = translate(-m_vPos), mView =
//      mRot * mTrans.  0x10014720's argument order is big5a960.cpp's
//      (pDest, pMat1, pMat2) and the pushes are (dest, mRot, mTrans).
//
//  (f) 0x1001f1f5..0x1001f269  mScale = diag(vScale), m_MatWorld =
//      mScale * mView.
//
//  (g) 0x1001f269..0x1001f2d8  m_Mat25c: the off-centre shear, with
//      m[0][2] = -pIn->m_fOffX * fInvFocal and m[1][2] = -pIn->m_fOffY *
//      fInvFocal.  The pool run 51b0..51e4 matches its fourteen literals
//      exactly and 51ac is the reciprocal's own 1.0f, which is what says
//      the reciprocal is a STATEMENT of its own and not two divisions.
//
//  (h) 0x1001f2dd..0x1001f33e  m_fAspX = focal/halfW, m_fAspY =
//      focal/halfH, then the two half-angle FOVs through 0x1007fe1c --
//      `fldl 0x4(%esp) ; fld1 ; fpatan ; ret`, i.e. the CRT's `atan` on a
//      double.  `fmuls 2.0f ; fsubrs PI` is `PI - 2.0f * atan(x)`, and
//      the pool order pi-then-2.0 puts PI first in the source.
//
//  (i) 0x1001f341..0x1001f438  mFov = diag(m_fAspX, m_fAspY, 1, 1) and
//      three `operator*` calls (0x10015800, hidden return slot at
//      (%esp), `other` at 0x4(%esp), `this` in %ecx):
//          m_Mat29c = m_Mat25c * m_MatWorld
//          m_Mat15c = mFov     * m_Mat29c
//          m_Mat2dc = mFov     * m_Mat25c
//      each followed by a 16-dword `rep movsl` out of the return temp.
//
//  (j) 0x1001f438..0x1001f453  0x1001e600.  A /Qip REGISTER ENTRY:
//      d3dr2a.cpp records that its destination matrix arrives in %eax and
//      that it opens `fsub %st(1),%st` against an x87 value THE CALLER
//      LEFT ON THE STACK.  Here the caller opens 16 bytes, writes only
//      0x8(%esp) with g_f1008b90c, loads pIn->m_fNear onto the x87 stack
//      and puts &mDepth in %eax.  Such a call is decoded as
//      an ordinary one and move on; that is what is done below, and the
//      x87 live-in is NOT spellable in C.
//
//  (k) 0x1001f458..0x1001f518  the eleven screen-mapping floats.  Order
//      fixed by the pool, not by the emission -- see section 4.
//
//  (l) 0x1001f519..0x1001f5bc  mScreen = Identity (0x1001e410, the
//      __cdecl wrapper round 0x100156e0) with four elements overwritten,
//      then m_Mat19c = mScreen * mDepth and m_Mat1dc = m_Mat19c *
//      m_Mat15c, then 0x1001e8d4 -- a register-ABI body taking %eax that
//      copies m_Mat19c's [0][0], [0][2], [1][1], [1][2], [2][2], [2][3]
//      into the six floats at +0x31c.
//
//  (m) 0x1001f5c1..0x1001f6a8  mInvFar = diag(k,k,k,1) with
//      k = 1.0f/pIn->m_fFar, then mZ, the projection's z row:
//          fZ = pIn->m_fNear * k;
//          fQ = 1.0f / (1.0f - fZ);
//          mZ.m[2][2] = 1.0f * fQ;   mZ.m[2][3] = -fZ * fQ;
//          mZ.m[3][2] = 1.0f;        everything else identity-with-no-w.
//      The `fstl -0x108(%ebp)` at 0x1001f613 is k kept as a double across
//      the 0x1001e140 call.
//
//  (n) 0x1001f6a8..0x10020334  THREE INLINED 4x4 MatMuls, column-store
//      order, terms last-first, exactly the ltmatrix.h:172 body
//      big19d64.cpp / big17c14.cpp / big66f08.cpp each derived
//      independently:  mA = mZ*mInvFar, mB = mA*mFov, mC = mB*m_Mat25c,
//      and m_Mat11c = mC.  Read off the FIRST element of each: 0x1001f6a8
//      pairs mZ[0][3] with mInvFar[3][0]; 0x1001fb24 pairs mA[0][0] with
//      mFov[0][0]; 0x1001ff47 pairs m_Mat25c[3][0] with mB[0][3].
//
//  (o) 0x10020336..0x100203f4  the five frustum corner points at +0x334:
//      (0,0,0) and the four far corners
//      (+-far/m_fAspX, +-far/m_fAspY, far).  The four `m_fFar` stores go
//      to +0x348/+0x354/+0x360/+0x36c, i.e. the .z of corners 1..4.
//
//  (p) 0x100203f4..0x100204f9  the five corners rotated by m_MatWorld's
//      3x3 TRANSPOSE (`out.x = v.x*m[0][0] + v.y*m[1][0] + v.z*m[2][0]`)
//      and then offset by m_vPos.  Five iterations: the induction pointer
//      starts at pXForm and the bound is `pXForm + 0x3c` = 5 x 12.
//
//  (q) 0x100204f9..0x100205d6  the six camera-space frustum planes at
//      +0x370, sixteen bytes each:
//          [0] ( 0,  0,  1)         d =  1.0f
//          [1] ( 0,  0, -1)         d = -m_fFar
//          [2] ( focal, 0, halfW - offX)   d = 0
//          [3] ( 0, -focal, halfH + offY)  d = 0
//          [4] (-focal, 0, halfW + offX)   d = 0
//          [5] ( 0,  focal, halfH - offY)  d = 0
//      (`fsubs ; fchs` is what makes the two `halfW - offX` terms, and
//      the pool's lone -1.0f at 0x100a5310 is [1]'s .z.)
//
//  (r) 0x100205d6..0x10020788  planes [2]..[5] normalised.  Each is
//      `mag = sqrt(x*x + y*y + z*z); if (0.0f != mag) { inv = 1.0f/mag;
//      x *= inv; y *= inv; z *= inv; }`.  The 0.0f comparands 5324..5330
//      are in POOL order = program order; the four 1.0f are in REVERSE
//      pool order (5368 first, 5350 last), which is icl's inline-expansion
//      order and is recorded here rather than explained.
//
//  (s) 0x10020788..0x100208f6  the four side planes carried into world
//      space by m_MatWorld's 3x3 transpose, `d = Dot(n, m_vPos)`, into
//      the second plane array at +0x3d0.  The transpose is written OUT OF
//      LINE at 0x1001f166 (`call 0x1001e420`) and IN LINE here -- the same
//      twelve `mov`s in the same order.  The inlining-budget rule
//      ("spent from the END backwards, so later sites get the richer
//      expansion") is the reading; the source below reproduces the two
//      shapes deliberately, because the call sequence depends on it.
//
//  (t) 0x100208f6..0x10020bb0  planes [0] and [1] -- the near and far
//      caps -- whose distances come from a POINT rather than the origin:
//      vDir = mRot row 2, vPoint = m_vPos + pIn->m_fNear * vDir for [0]
//      and m_vPos + m_fFar * vDir for [1].
//
//  (u) 0x10020bb0..0x10020bf9  `memcpy(&m_aClip[0], &m_aPlanes[0], 0x60)`
//      -- six 16-byte planes -- then m_aClip[0].m_fDist = 0.01f (the
//      literal big5dca0.cpp records the sky-portal caller handing over),
//      m_n4cc = 0, m_f4d0 = 1.0f, 0x1001e660, `return 1`.
//
// *********************************************************************** //

#include "ltbasedefs.h"


// ======================================================================= //
//  THE CRT ENTRY POINTS THIS FUNCTION REACHES.
//
//  Declared here rather than through <math.h>/<string.h> so that the
//  symbol the call carries is the one the shipped bytes carry.  `sqrt` and
//  `atan` both take a DOUBLE: the argument goes out as `fstpl (%esp)` at
//  0x1001f306, 0x1001f325, 0x100205d9, 0x1002064e, 0x100206c3 and
//  0x10020738, and 0x1007fe1c's body is `fldl 0x4(%esp) ; fld1 ; fpatan`.
//
//  0x1007fc98 is a known toolchain gap: icl expands it to
//  `fsqrt` where the shipped build calls it.  Four calls are expected to
//  come back shipped-only for that reason and no other.
// ======================================================================= //
extern "C" void*  memcpy(void* pDest, const void* pSrc, unsigned int nLen);
extern "C" double sqrt(double x);                       // 0x1007fc98
extern "C" double atan(double x);                       // 0x1007fe1c

#define D3D_PI  3.1415927410125732f                     // 0x100a51f0


// ======================================================================= //
//  VECTOR, PLANE AND MATRIX.
//
//  Declared LOCALLY rather than by including ltmatrix.h, for the reason
//  d3dr3b.cpp and big26728.cpp both give: under /Ob1 icl inlines exactly
//  what the source marks inline, and the shipped code CALLS Identity,
//  GetTranslation, SetTranslation, GetBasisVectors, operator*, MatMul and
//  the transpose out of line at the sites below while INLINING MatMul and
//  the transpose at others.  ltmatrix.h marks all of them `inline`, so it
//  cannot produce these bytes.
//
//  EVERY member below is DECLARED AND NOT DEFINED.  The first
//  "thing that costs calls" is a tiny accessor spelled as a field read;
//  0x1001e480 (three `mov`s) and 0x1001e4a0 (three `mov`s) are exactly
//  that shape and would emit nothing at all if written inline.
// ======================================================================= //
struct LTVec
{
	float	x;                                          // +0x00
	float	y;                                          // +0x04
	float	z;                                          // +0x08
};

struct Plane1ef30
{
	LTVec	m_vNormal;                                  // +0x00
	float	m_fDist;                                    // +0x0c
};

struct LTMat
{
	// 0x100156e0 -- the identity Init, __thiscall (d3dr2b.cpp/d3dlow.cpp).
	void	sub_100156e0();

	// 0x1001e480 -- GetTranslation, __thiscall, `ret $0x4`.  Reads
	// m[0][3], m[1][3], m[2][3] (0xc/0x1c/0x2c) into the argument.
	void	sub_1001e480(LTVec* pOut);

	// 0x1001e4a0 -- SetTranslation(x,y,z), __thiscall, `ret $0xc`.
	// Writes the same three slots.
	void	sub_1001e4a0(float x, float y, float z);

	// 0x1001e140 -- LTMatrix::Init, SIXTEEN FLOATS by value, `ret $0x40`
	// (d3dlow.cpp's C1001e140; w3_10014580_os.cpp / w29_10027f70.cpp).
	void	sub_1001e140(float m00, float m01, float m02, float m03,
	                     float m10, float m11, float m12, float m13,
	                     float m20, float m21, float m22, float m23,
	                     float m30, float m31, float m32, float m33);

	// 0x10015800 -- operator*, __thiscall: hidden return slot at (%esp),
	// `other` at 0x4(%esp), `this` in %ecx, `ret $8` (d3dbatchd.cpp).
	LTMat	sub_10015800(LTMat& other);

	float	m[4][4];
};

// 0x1001e410 -- 15 bytes, the __cdecl wrapper round 0x100156e0
// (d3dr2b.cpp).  Called at 0x1001f519 where 0x100156e0 itself is called at
// 0x1001ef50; the two are different symbols and the call check sees the
// difference, so both spellings are kept.
extern "C" void  sub_1001e410(LTMat* pMat);

// 0x1001e420 -- transpose the 3x3 rotation part in place, __cdecl
// (d3dmisc.cpp).  Its twelve `mov`s are reproduced IN LINE at 0x10020788;
// see the note at that site.
extern "C" void  sub_1001e420(LTMat* pMat);

// 0x1001e450 -- __cdecl, forwards to the __thiscall GetBasisVectors at
// 0x1001e4d0, which fills its three arguments from COLUMNS 0, 1 and 2.
extern "C" void  sub_1001e450(LTMat* pMat, LTVec* pCol0, LTVec* pCol1,
                              LTVec* pCol2);

// 0x10014720 -- MatMul(pDest, pMat1, pMat2), __cdecl (big5a960.cpp).
extern "C" void  sub_10014720(LTMat* pDest, LTMat* pMat1, LTMat* pMat2);

// 0x10015190 -- round-to-nearest float->int32, __cdecl (big17c14.cpp:
// `flds 0x8(%ebp) ; fistpl -0x4(%ebp)`).
extern "C" int32 sub_10015190(float fVal);


// ======================================================================= //
//  ltmatrix.h:172 MatMul, written out for the THREE sites at 0x1001f6a8,
//  0x1001fb24 and 0x1001ff47 that carry NO CALL.  Store order is the
//  header's: column by column, [0][0] [1][0] [2][0] [3][0] [0][1] ...
//  This is byte-for-byte the helper big19d64.cpp, big17c14.cpp and
//  big66f08.cpp each derived independently; it is reused here unchanged.
// ======================================================================= //
#define M1 pMat1->m
#define M2 pMat2->m

inline void d3d_MatMul(LTMat *pDest, LTMat *pMat1, LTMat *pMat2)
{
	pDest->m[0][0] = M1[0][0]*M2[0][0] + M1[0][1]*M2[1][0] + M1[0][2]*M2[2][0] + M1[0][3]*M2[3][0];
	pDest->m[1][0] = M1[1][0]*M2[0][0] + M1[1][1]*M2[1][0] + M1[1][2]*M2[2][0] + M1[1][3]*M2[3][0];
	pDest->m[2][0] = M1[2][0]*M2[0][0] + M1[2][1]*M2[1][0] + M1[2][2]*M2[2][0] + M1[2][3]*M2[3][0];
	pDest->m[3][0] = M1[3][0]*M2[0][0] + M1[3][1]*M2[1][0] + M1[3][2]*M2[2][0] + M1[3][3]*M2[3][0];

	pDest->m[0][1] = M1[0][0]*M2[0][1] + M1[0][1]*M2[1][1] + M1[0][2]*M2[2][1] + M1[0][3]*M2[3][1];
	pDest->m[1][1] = M1[1][0]*M2[0][1] + M1[1][1]*M2[1][1] + M1[1][2]*M2[2][1] + M1[1][3]*M2[3][1];
	pDest->m[2][1] = M1[2][0]*M2[0][1] + M1[2][1]*M2[1][1] + M1[2][2]*M2[2][1] + M1[2][3]*M2[3][1];
	pDest->m[3][1] = M1[3][0]*M2[0][1] + M1[3][1]*M2[1][1] + M1[3][2]*M2[2][1] + M1[3][3]*M2[3][1];

	pDest->m[0][2] = M1[0][0]*M2[0][2] + M1[0][1]*M2[1][2] + M1[0][2]*M2[2][2] + M1[0][3]*M2[3][2];
	pDest->m[1][2] = M1[1][0]*M2[0][2] + M1[1][1]*M2[1][2] + M1[1][2]*M2[2][2] + M1[1][3]*M2[3][2];
	pDest->m[2][2] = M1[2][0]*M2[0][2] + M1[2][1]*M2[1][2] + M1[2][2]*M2[2][2] + M1[2][3]*M2[3][2];
	pDest->m[3][2] = M1[3][0]*M2[0][2] + M1[3][1]*M2[1][2] + M1[3][2]*M2[2][2] + M1[3][3]*M2[3][2];

	pDest->m[0][3] = M1[0][0]*M2[0][3] + M1[0][1]*M2[1][3] + M1[0][2]*M2[2][3] + M1[0][3]*M2[3][3];
	pDest->m[1][3] = M1[1][0]*M2[0][3] + M1[1][1]*M2[1][3] + M1[1][2]*M2[2][3] + M1[1][3]*M2[3][3];
	pDest->m[2][3] = M1[2][0]*M2[0][3] + M1[2][1]*M2[1][3] + M1[2][2]*M2[2][3] + M1[2][3]*M2[3][3];
	pDest->m[3][3] = M1[3][0]*M2[0][3] + M1[3][1]*M2[1][3] + M1[3][2]*M2[2][3] + M1[3][3]*M2[3][3];
}

#undef M1
#undef M2


// ======================================================================= //
//  THE 28-BYTE LENS BLOCK -- arg1.  Seven floats, every one of them read
//  below and none of them written.  0x10020d80 and 0x10020e80 are the two
//  functions that FILL it (d3dr3a.cpp and big26728.cpp both record that);
//  the names here come from what this function does with each field.
// ======================================================================= //
struct D3DLens
{
	float	m_fOffX;                                    // +0x00
	float	m_fOffY;                                    // +0x04
	float	m_fFocal;                                   // +0x08
	float	m_fHalfW;                                   // +0x0c
	float	m_fHalfH;                                   // +0x10
	float	m_fNear;                                    // +0x14
	float	m_fFar;                                     // +0x18
};


// ======================================================================= //
//  THE TRANSFORM OBJECT -- arg0.  0x4f8 bytes, which is the size
//  big26728.cpp measures for the global at 0x1008b880 (`rep movsl` with
//  %ecx = 0x13e).  Every offset below is a load or a store in these bytes.
// ======================================================================= //
struct D3DXForm
{
	// 0x1001e360 -- __thiscall, an LTVec BY VALUE, `ret $0xc`
	// (d3dbatcha.cpp / big66f08.cpp).  It stores the vector at +0x4e4 and
	// then picks a detail level into +0x4f0/+0x4f4 off two globals.
	void	sub_1001e360(LTVec vPos);

	D3DLens	m_Lens;                                     // +0x000 (memcpy'd)

	int32	m_nLeft;                                    // +0x01c
	int32	m_nTop;                                     // +0x020
	int32	m_nRight;                                   // +0x024
	int32	m_nBottom;                                  // +0x028

	float	m_fLeft;                                    // +0x02c
	float	m_fRight;                                   // +0x030
	float	m_fTop;                                     // +0x034
	float	m_fBottom;                                  // +0x038

	int32	m_nLeft2;                                   // +0x03c
	int32	m_nRight2;                                  // +0x040
	int32	m_nTop2;                                    // +0x044
	int32	m_nBottom2;                                 // +0x048

	float	m_fFovX;                                    // +0x04c
	float	m_fFovY;                                    // +0x050

	float	m_fW;                                       // +0x054
	float	m_fH;                                       // +0x058
	float	m_fHalfW;                                   // +0x05c
	float	m_fHalfH;                                   // +0x060
	float	m_fInvHalfW;                                // +0x064
	float	m_fInvHalfH;                                // +0x068
	float	m_fCentreX;                                 // +0x06c
	float	m_fCentreY;                                 // +0x070
	float	m_fSx;                                      // +0x074
	float	m_fSy;                                      // +0x078
	float	m_fInvSx;                                   // +0x07c
	float	m_fInvSy;                                   // +0x080
	float	m_fSxSy;                                    // +0x084

	float	m_fNear;                                    // +0x088
	float	m_fFar;                                     // +0x08c
	float	m_fFar2;                                    // +0x090
	float	m_fAspX;                                    // +0x094
	float	m_fAspY;                                    // +0x098

	LTMat	m_MatCam;                                   // +0x09c
	LTMat	m_MatWorld;                                 // +0x0dc
	LTMat	m_Mat11c;                                   // +0x11c
	LTMat	m_Mat15c;                                   // +0x15c
	LTMat	m_Mat19c;                                   // +0x19c
	LTMat	m_Mat1dc;                                   // +0x1dc
	LTMat	m_Mat21c;                                   // +0x21c
	LTMat	m_Mat25c;                                   // +0x25c
	LTMat	m_Mat29c;                                   // +0x29c
	LTMat	m_Mat2dc;                                   // +0x2dc

	float	m_a31c[6];                                  // +0x31c

	LTVec	m_avCorners[5];                             // +0x334
	Plane1ef30	m_aPlanes[6];                               // +0x370
	Plane1ef30	m_aWorld[6];                                // +0x3d0
	Plane1ef30	m_aClip[6];                                 // +0x430

	LTVec	m_v490;                                     // +0x490
	LTVec	m_v49c;                                     // +0x49c
	LTVec	m_v4a8;                                     // +0x4a8
	LTVec	m_vPos;                                     // +0x4b4
	uint8	m_pad_4c0[0x4cc - 0x4c0];                   // +0x4c0
	int32	m_n4cc;                                     // +0x4cc
	float	m_f4d0;                                     // +0x4d0
	int32	m_n4d4;                                     // +0x4d4
	uint8	m_pad_4d8[0x4f8 - 0x4d8];                   // +0x4d8
};


// 0x1001e600 -- a /Qip REGISTER ENTRY.  The destination matrix arrives in
// %eax, the second float is at 0xc(%esp) inside the callee (the caller's
// 0x8(%esp)), and the FIRST float arrives ON THE X87 STACK -- the body
// opens `fsub %st(1),%st` against a value the caller left there.  That is
// what /Qip does with a STATIC whose only caller is in its TU, and it is
// byte-identical here (was w4_1001e4a0_os.cpp's ordinary
// __cdecl copy, 82 bytes differing).  The depth (projection) matrix:
// identity except m[2][2] = far/(far-near), m[2][3] = -far*near/(far-near),
// m[3][2] = 1, m[3][3] = 0.  In this flag set the three 1.0f stores come out
// in source order (the /Os copy reversed them).
static void sub_1001e600(LTMat* pMat, float fNear, float fFar)
{
	float fQ = 1.0f / (fFar - fNear);

	pMat->m[0][0] = 1.0f;
	pMat->m[1][1] = 1.0f;
	pMat->m[3][2] = 1.0f;
	pMat->m[0][1] = 0.0f;
	pMat->m[0][2] = 0.0f;
	pMat->m[0][3] = 0.0f;
	pMat->m[1][0] = 0.0f;
	pMat->m[1][2] = 0.0f;
	pMat->m[1][3] = 0.0f;
	pMat->m[2][0] = 0.0f;
	pMat->m[2][1] = 0.0f;
	pMat->m[3][0] = 0.0f;
	pMat->m[3][1] = 0.0f;
	pMat->m[3][3] = 0.0f;
	pMat->m[2][2] = fFar * fQ;
	pMat->m[2][3] = -fFar * (fQ * fNear);
}

// 0x1001e8d0 / 0x1001e8d4 -- a /Qip EXTERNAL two-entry pair: the 4-byte
// stack entry `mov 0x4(%esp),%eax` at 0x1001e8d0 falls into the register
// body 0x1001e8d4, which 0x1001ef30 calls with the transform in %eax.  So
// it is named for the stack entry.  Copies m_Mat19c's [0][0], [0][2],
// [1][1], [1][2], [2][2] and [2][3] into m_a31c[0..5].  Both entries
// byte-identical here (was w4_1001e4a0_os.cpp, 77 differing).
extern "C" void sub_1001e8d0(D3DXForm* pXForm)
{
	pXForm->m_a31c[0] = pXForm->m_Mat19c.m[0][0];
	pXForm->m_a31c[1] = pXForm->m_Mat19c.m[0][2];
	pXForm->m_a31c[2] = pXForm->m_Mat19c.m[1][1];
	pXForm->m_a31c[3] = pXForm->m_Mat19c.m[1][2];
	pXForm->m_a31c[4] = pXForm->m_Mat19c.m[2][2];
	pXForm->m_a31c[5] = pXForm->m_Mat19c.m[2][3];
}

// 0x1001e660 -- 251 bytes, no arguments, works entirely on globals
// (d3dr3b.cpp).  `mov 0x1008beb0,%eax ; cmpl $0x1,(%eax)` is its opening.
extern "C" void  sub_1001e660();

// 0x1008b90c -- read once, at 0x1001f444, and handed to 0x1001e600.
extern float  g_f1008b90c;                              // 0x1008b90c


// ======================================================================= //
//  0x1001ef30 -- 7,370 bytes, 39 direct calls, 3 callers.
// ======================================================================= //
int32 sub_1001ef30(D3DXForm* pXForm, D3DLens* pLens,
                   float fMinX, float fMinY, float fMaxX, float fMaxY,
                   LTMat* pMatCam, LTVec vScale)
{
	LTMat	mRot;                                       // -0x670
	LTMat	mTrans;                                     // -0x630
	LTMat	mView;                                      // -0x5f0
	LTMat	mScale;                                     // -0x5b0
	LTMat	mFov;                                       // -0x570
	LTMat	mDepth;                                     // -0x470
	LTMat	mScreen;                                    // -0x430
	LTMat	mInvFar;                                    // -0x370
	LTMat	mZ;                                         // -0x330
	LTMat	mA;                                         // -0x2f0
	LTMat	mB;                                         // -0x2b0
	LTMat	mC;                                         // -0x270
	LTMat	mT;                                         // -0x224
	int32	i;

	// ------------------------------------------------------------------ //
	//  (a) 0x1001ef30 .. 0x1001effe
	//
	//  The `movl $0x0,0x4d4(%ebx)` at 0x1001ef46 is BETWEEN the
	//  `lea 0x21c(%ebx),%ecx` and the `call 0x100156e0`.  A store cannot
	//  be moved across a call in either direction, so this order is read,
	//  not chosen.
	// ------------------------------------------------------------------ //
	pXForm->m_n4d4 = 0;
	pXForm->m_Mat21c.sub_100156e0();

	// 0x1001efbc: `rep movsl` %ecx = 0x10 from arg6 into +0x9c.
	pXForm->m_MatCam = *pMatCam;

	// 0x1001efc9: `push $0x1c` -- an explicit memcpy, not a struct
	// assignment; the 0x1c is an immediate in the bytes.
	memcpy(pXForm, pLens, 0x1c);

	// 0x1001efe0 / 0x1001eff9.
	pMatCam->sub_1001e480(&pXForm->m_vPos);
	pXForm->sub_1001e360(pXForm->m_vPos);

	// ------------------------------------------------------------------ //
	//  (b) 0x1001effe .. 0x1001f05f -- the far-plane clamp.
	//
	//  Both bounds appear TWICE in the literal pool (0x100a5120/5124 =
	//  3.0f, 0x100a5128/512c = 500000.0f), once loaded for the `fcomps`
	//  and once folded into the `movl $imm`, which is what says each is
	//  written twice in the source.  The low arm falls THROUGH into the
	//  high test (0x1001f02e reloads m_fFar), so these are two separate
	//  `if`s and not big17c14.cpp's D3D_CLAMPF.
	//
	//  The operand order is read off which side is loaded first: at
	//  0x1001f004 the BOUND is in %st(0) and the variable is the `fcomps`
	//  memory operand, so 3.0f is on the left; at 0x1001f034 the VARIABLE
	//  is in %st(0), so m_fFar is on the left.
	// ------------------------------------------------------------------ //
	pXForm->m_fFar = pLens->m_fFar;

	if (3.0f > pXForm->m_fFar)
		pXForm->m_fFar = 3.0f;

	if (pXForm->m_fFar > 500000.0f)
		pXForm->m_fFar = 500000.0f;

	pXForm->m_fNear = pLens->m_fNear;
	pXForm->m_fFar2 = pXForm->m_fFar;

	// ------------------------------------------------------------------ //
	//  (c) 0x1001f060 .. 0x1001f10a -- eight rounds.
	//
	//  The pool fixes the four float assignments as one ascending run:
	//  0x100a5130's 1.0f is loaded by m_fRight's statement and 0x100a5134's
	//  by m_fBottom's, and the pool order is the reverse of the emission
	//  order, so the source order is m_fLeft, m_fRight, m_fTop, m_fBottom.
	// ------------------------------------------------------------------ //
	pXForm->m_nLeft   = sub_10015190(fMinX);
	pXForm->m_nTop    = sub_10015190(fMinY);
	pXForm->m_nRight  = sub_10015190(fMaxX);
	pXForm->m_nBottom = sub_10015190(fMaxY);

	pXForm->m_fLeft   = (float)pXForm->m_nLeft;
	pXForm->m_fRight  = (float)pXForm->m_nRight  - 1.0f;
	pXForm->m_fTop    = (float)pXForm->m_nTop;
	pXForm->m_fBottom = (float)pXForm->m_nBottom - 1.0f;

	pXForm->m_nLeft2   = sub_10015190(pXForm->m_fLeft);
	pXForm->m_nRight2  = sub_10015190(pXForm->m_fRight);
	pXForm->m_nTop2    = sub_10015190(pXForm->m_fTop);
	pXForm->m_nBottom2 = sub_10015190(pXForm->m_fBottom);

	// ------------------------------------------------------------------ //
	//  (d) 0x1001f10a .. 0x1001f16b -- the rotation basis.
	//
	//  0x1001e450's arguments go out as (&mRot, &m_v49c, &m_v490,
	//  &m_v4a8) and 0x1001e4d0 fills its first from COLUMN 0, so +0x49c
	//  is column 0, +0x490 column 1 and +0x4a8 column 2.
	// ------------------------------------------------------------------ //
	mRot = *pMatCam;
	mRot.sub_1001e4a0(0.0f, 0.0f, 0.0f);
	sub_1001e450(&mRot, &pXForm->m_v49c, &pXForm->m_v490, &pXForm->m_v4a8);
	sub_1001e420(&mRot);

	// ------------------------------------------------------------------ //
	//  (e) 0x1001f16b .. 0x1001f1f2 -- translate by -m_vPos, then the view
	//  matrix.  The thirteen literals of mTrans are the pool run
	//  0x100a5144..0x100a5174, matched element for element.
	// ------------------------------------------------------------------ //
	{
		mTrans.sub_1001e140(1.0f, 0.0f, 0.0f, -pXForm->m_vPos.x,
			0.0f, 1.0f, 0.0f, -pXForm->m_vPos.y,
			0.0f, 0.0f, 1.0f, -pXForm->m_vPos.z,
			0.0f, 0.0f, 0.0f, 1.0f);
	}

	sub_10014720(&mView, &mRot, &mTrans);

	// ------------------------------------------------------------------ //
	//  (f) 0x1001f1f5 .. 0x1001f269 -- the scale matrix (pool run
	//  0x100a5178..0x100a51a8: twelve zeros then one 1.0f) and the world
	//  matrix.
	// ------------------------------------------------------------------ //
	{
		mScale.sub_1001e140(vScale.x, 0.0f, 0.0f, 0.0f,
			0.0f, vScale.y, 0.0f, 0.0f,
			0.0f, 0.0f, vScale.z, 0.0f,
			0.0f, 0.0f, 0.0f, 1.0f);
	}

	sub_10014720(&pXForm->m_MatWorld, &mScale, &mView);

	// ------------------------------------------------------------------ //
	//  (g) 0x1001f269 .. 0x1001f2d8 -- the off-centre shear.
	//
	//  The reciprocal is a STATEMENT of its own: 0x100a51ac sits between
	//  mScale's run and mSkew's, i.e. before the matrix's first literal,
	//  and the code computes `1.0f/m_fFocal` once and multiplies twice.
	//  mSkew's fourteen literals are 0x100a51b0..0x100a51e4, matched.
	// ------------------------------------------------------------------ //
	{
		float	fInvFocal;

		fInvFocal = 1.0f / pLens->m_fFocal;

		pXForm->m_Mat25c.sub_1001e140(1.0f, 0.0f, -pLens->m_fOffX * fInvFocal, 0.0f,
			0.0f, 1.0f, -pLens->m_fOffY * fInvFocal, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 1.0f);
	}

	// ------------------------------------------------------------------ //
	//  (h) 0x1001f2dd .. 0x1001f33e -- the two half-angle FOVs.
	//
	//  `fmuls 2.0f ; fsubrs PI` with the pool holding PI (0x100a51f0)
	//  BEFORE 2.0f (0x100a51f4) is `PI - 2.0f * atan(x)`, twice.
	// ------------------------------------------------------------------ //
	pXForm->m_fAspX = pLens->m_fFocal / pLens->m_fHalfW;
	pXForm->m_fAspY = pLens->m_fFocal / pLens->m_fHalfH;

	pXForm->m_fFovX = D3D_PI - 2.0f * (float)atan(pXForm->m_fAspX);
	pXForm->m_fFovY = D3D_PI - 2.0f * (float)atan(pXForm->m_fAspY);

	// ------------------------------------------------------------------ //
	//  (i) 0x1001f341 .. 0x1001f438 -- the FOV matrix (pool run
	//  0x100a5200..0x100a5234: eight zeros, 1.0f, four zeros, 1.0f, which
	//  is exactly diag(a,b,1,1) written row by row) and three operator*.
	// ------------------------------------------------------------------ //
	{
		mFov.sub_1001e140(pXForm->m_fAspX, 0.0f, 0.0f, 0.0f,
			0.0f, pXForm->m_fAspY, 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 1.0f);
	}

	pXForm->m_Mat29c = pXForm->m_Mat25c.sub_10015800(pXForm->m_MatWorld);
	pXForm->m_Mat15c = mFov.sub_10015800(pXForm->m_Mat29c);
	pXForm->m_Mat2dc = mFov.sub_10015800(pXForm->m_Mat25c);

	// ------------------------------------------------------------------ //
	//  (j) 0x1001f438 .. 0x1001f453 -- the depth matrix.  See the
	//  declaration of sub_1001e600: this call's first float argument
	//  really arrives on the x87 stack and cannot be spelled.
	// ------------------------------------------------------------------ //
	sub_1001e600(&mDepth, pLens->m_fNear, g_f1008b90c);

	// ------------------------------------------------------------------ //
	//  (k) 0x1001f458 .. 0x1001f518 -- the eleven screen-mapping floats.
	//
	//  The emission is interleaved (Y before X, the reciprocals hoisted)
	//  but the POOL is not: 0x100a5238's 0.5f belongs to m_fHalfW and
	//  0x100a523c's to m_fHalfH, and the four loaded 1.0f ascend
	//  5244 < 524c < 5254 < 525c in the order 0x64, 0x68, 0x7c, 0x80.
	//  That is what makes this one ascending run.
	// ------------------------------------------------------------------ //
	pXForm->m_fW        = fMaxX - fMinX;
	pXForm->m_fH        = fMaxY - fMinY;
	pXForm->m_fHalfW    = 0.5f * pXForm->m_fW;
	pXForm->m_fHalfH    = 0.5f * pXForm->m_fH;
	pXForm->m_fInvHalfW = 1.0f / pXForm->m_fHalfW;
	pXForm->m_fInvHalfH = 1.0f / pXForm->m_fHalfH;
	pXForm->m_fCentreX  = fMinX + pXForm->m_fHalfW;
	pXForm->m_fCentreY  = fMinY + pXForm->m_fHalfH;
	pXForm->m_fSx       = pXForm->m_fAspX * pXForm->m_fHalfW;
	pXForm->m_fSy       = pXForm->m_fAspY * pXForm->m_fHalfH;
	pXForm->m_fInvSx    = 1.0f / pXForm->m_fSx;
	pXForm->m_fInvSy    = 1.0f / pXForm->m_fSy;
	pXForm->m_fSxSy     = pXForm->m_fSy * pXForm->m_fSx;

	// ------------------------------------------------------------------ //
	//  (l) 0x1001f519 .. 0x1001f5bc -- the screen matrix.  1e-4f appears
	//  twice in the pool (0x100a5260 then 0x100a5264) and the m[0][0]
	//  statement loads the first, which is what orders the two.
	// ------------------------------------------------------------------ //
	sub_1001e410(&mScreen);

	mScreen.m[0][0] =   pXForm->m_fHalfW - 0.0001f;
	mScreen.m[0][3] =   pXForm->m_fCentreX;
	mScreen.m[1][1] = -(pXForm->m_fHalfH - 0.0001f);
	mScreen.m[1][3] =   pXForm->m_fCentreY;

	pXForm->m_Mat19c = mScreen.sub_10015800(mDepth);
	pXForm->m_Mat1dc = pXForm->m_Mat19c.sub_10015800(pXForm->m_Mat15c);

	sub_1001e8d0(pXForm);

	// ------------------------------------------------------------------ //
	//  (m) 0x1001f5c1 .. 0x1001f6a8 -- the two z matrices.
	//
	//  mInvFar's thirteen literals are the pool run 0x100a5270..0x100a52a0
	//  and mZ's fifteen are 0x100a52ac..0x100a52e4, with the single 1.0f
	//  at 0x100a52d4 falling between m[2][1] and m[3][0] -- so m[2][2]
	//  carries exactly one literal and m[2][3] none.  See COIN-FLIP (E).
	// ------------------------------------------------------------------ //
	{
		float	fInvFar;
		float	fZ;
		float	fQ;

		fInvFar = 1.0f / pLens->m_fFar;

		mInvFar.sub_1001e140(fInvFar, 0.0f, 0.0f, 0.0f,
			0.0f, fInvFar, 0.0f, 0.0f,
			0.0f, 0.0f, fInvFar, 0.0f,
			0.0f, 0.0f, 0.0f, 1.0f);

		fZ = pLens->m_fNear * fInvFar;
		fQ = 1.0f / (1.0f - fZ);

		mZ.sub_1001e140(1.0f, 0.0f, 0.0f, 0.0f,
			0.0f, 1.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f * fQ, -fZ * fQ,
			0.0f, 0.0f, 1.0f, 0.0f);
	}

	// ------------------------------------------------------------------ //
	//  (n) 0x1001f6a8 .. 0x10020334 -- three inlined MatMuls, 3,724 bytes
	//  with no call in them.  Operand roles read off the first element of
	//  each: 0x1001f6a8 pairs mZ[0][3] with mInvFar[3][0]; 0x1001fb24
	//  pairs mA[0][0] with mFov[0][0]; 0x1001ff47 pairs m_Mat25c[3][0]
	//  with mB[0][3].
	// ------------------------------------------------------------------ //
	d3d_MatMul(&mA, &mZ, &mInvFar);
	d3d_MatMul(&mB, &mA, &mFov);
	d3d_MatMul(&mC, &mB, &pXForm->m_Mat25c);

	pXForm->m_Mat11c = mC;

	// ------------------------------------------------------------------ //
	//  (o) 0x10020336 .. 0x100203f4 -- the five frustum corners in camera
	//  space.  The four `m_fFar` stores land on +0x348/+0x354/+0x360/
	//  +0x36c, i.e. corner[1..4].z, and the two products are computed once
	//  each and negated, which is what makes them two named values rather
	//  than eight expressions.
	// ------------------------------------------------------------------ //
	{
		float	fInvFocal;
		float	fX;
		float	fY;

		fInvFocal = 1.0f / pLens->m_fFocal;

		fX = pLens->m_fHalfW * fInvFocal * pXForm->m_fFar;
		fY = pLens->m_fHalfH * fInvFocal * pXForm->m_fFar;

		pXForm->m_avCorners[0].x =  0.0f;
		pXForm->m_avCorners[0].y =  0.0f;
		pXForm->m_avCorners[0].z =  0.0f;

		pXForm->m_avCorners[1].x = -fX;
		pXForm->m_avCorners[1].y =  fY;
		pXForm->m_avCorners[1].z =  pXForm->m_fFar;

		pXForm->m_avCorners[2].x =  fX;
		pXForm->m_avCorners[2].y =  fY;
		pXForm->m_avCorners[2].z =  pXForm->m_fFar;

		pXForm->m_avCorners[3].x = -fX;
		pXForm->m_avCorners[3].y = -fY;
		pXForm->m_avCorners[3].z =  pXForm->m_fFar;

		pXForm->m_avCorners[4].x =  fX;
		pXForm->m_avCorners[4].y = -fY;
		pXForm->m_avCorners[4].z =  pXForm->m_fFar;
	}

	// ------------------------------------------------------------------ //
	//  (p) 0x100203f4 .. 0x100204f9 -- the corners into world space.
	//  m_MatWorld's 3x3 is applied TRANSPOSED (`out.x = v.x*m[0][0] +
	//  v.y*m[1][0] + v.z*m[2][0]`) and the result is then offset by
	//  m_vPos in a second statement -- the `rep movsl` %ecx = 3 at
	//  0x10020499 is the assignment between the two.
	//  Five iterations: the strength-reduced base runs from pXForm to
	//  pXForm + 0x3c in steps of 12.
	// ------------------------------------------------------------------ //
	for (i = 0; i < 5; i++)
	{
		LTVec	v;

		v.x = pXForm->m_avCorners[i].x * pXForm->m_MatWorld.m[0][0]
		    + pXForm->m_avCorners[i].y * pXForm->m_MatWorld.m[1][0]
		    + pXForm->m_avCorners[i].z * pXForm->m_MatWorld.m[2][0];
		v.y = pXForm->m_avCorners[i].x * pXForm->m_MatWorld.m[0][1]
		    + pXForm->m_avCorners[i].y * pXForm->m_MatWorld.m[1][1]
		    + pXForm->m_avCorners[i].z * pXForm->m_MatWorld.m[2][1];
		v.z = pXForm->m_avCorners[i].x * pXForm->m_MatWorld.m[0][2]
		    + pXForm->m_avCorners[i].y * pXForm->m_MatWorld.m[1][2]
		    + pXForm->m_avCorners[i].z * pXForm->m_MatWorld.m[2][2];

		pXForm->m_avCorners[i] = v;

		pXForm->m_avCorners[i].x = pXForm->m_vPos.x + pXForm->m_avCorners[i].x;
		pXForm->m_avCorners[i].y = pXForm->m_vPos.y + pXForm->m_avCorners[i].y;
		pXForm->m_avCorners[i].z = pXForm->m_vPos.z + pXForm->m_avCorners[i].z;
	}

	// ------------------------------------------------------------------ //
	//  (q) 0x100204f9 .. 0x100205d6 -- the six camera-space planes.
	//
	//  Twelve of the twenty-four words are integer `mov`s of 0 (0x370,
	//  0x374, 0x380, 0x384, 0x394, 0x3a0, 0x3b4, 0x3c0, and the four
	//  distances at 0x39c/0x3ac/0x3bc/0x3cc, which are stored later, at
	//  0x10020790..0x100207a2, from the same zeroed register), which is
	//  what says they are literal 0.0f assignments rather than results.
	//  The two `fsubs ; fchs` pairs are the `halfW - offX` terms; the
	//  pool's lone -1.0f at 0x100a5310 is plane[1].z.
	// ------------------------------------------------------------------ //
	pXForm->m_aPlanes[0].m_vNormal.x = 0.0f;
	pXForm->m_aPlanes[0].m_vNormal.y = 0.0f;
	pXForm->m_aPlanes[0].m_vNormal.z = 1.0f;
	pXForm->m_aPlanes[0].m_fDist     = 1.0f;

	pXForm->m_aPlanes[1].m_vNormal.x = 0.0f;
	pXForm->m_aPlanes[1].m_vNormal.y = 0.0f;
	pXForm->m_aPlanes[1].m_vNormal.z = -1.0f;
	pXForm->m_aPlanes[1].m_fDist     = -pXForm->m_fFar;

	pXForm->m_aPlanes[2].m_vNormal.x = pLens->m_fFocal;
	pXForm->m_aPlanes[2].m_vNormal.y = 0.0f;
	pXForm->m_aPlanes[2].m_vNormal.z = -(pLens->m_fOffX - pLens->m_fHalfW);
	pXForm->m_aPlanes[2].m_fDist     = 0.0f;

	pXForm->m_aPlanes[3].m_vNormal.x = 0.0f;
	pXForm->m_aPlanes[3].m_vNormal.y = -pLens->m_fFocal;
	pXForm->m_aPlanes[3].m_vNormal.z = pLens->m_fHalfH + pLens->m_fOffY;
	pXForm->m_aPlanes[3].m_fDist     = 0.0f;

	pXForm->m_aPlanes[4].m_vNormal.x = -pLens->m_fFocal;
	pXForm->m_aPlanes[4].m_vNormal.y = 0.0f;
	pXForm->m_aPlanes[4].m_vNormal.z = pLens->m_fHalfW + pLens->m_fOffX;
	pXForm->m_aPlanes[4].m_fDist     = 0.0f;

	pXForm->m_aPlanes[5].m_vNormal.x = 0.0f;
	pXForm->m_aPlanes[5].m_vNormal.y = pLens->m_fFocal;
	pXForm->m_aPlanes[5].m_vNormal.z = -(pLens->m_fOffY - pLens->m_fHalfH);
	pXForm->m_aPlanes[5].m_fDist     = 0.0f;

	// ------------------------------------------------------------------ //
	//  (r) 0x100205d6 .. 0x10020788 -- the four side planes normalised.
	//
	//  Written out four times rather than as a loop: the four expansions
	//  are straight-line, at 0x100205d6, 0x1002064b, 0x100206c0 and
	//  0x10020735, with no induction variable and no back edge.
	//  `flds 0.0f ; fcomp %st(1) ; je` puts 0.0f on the LEFT of the test.
	//  The four sqrt calls are 0x1007fc98, the known CRT gap.
	// ------------------------------------------------------------------ //
	{
		float	fMag;
		float	fInv;

		fMag = (float)sqrt(pXForm->m_aPlanes[2].m_vNormal.x * pXForm->m_aPlanes[2].m_vNormal.x
		                 + pXForm->m_aPlanes[2].m_vNormal.y * pXForm->m_aPlanes[2].m_vNormal.y
		                 + pXForm->m_aPlanes[2].m_vNormal.z * pXForm->m_aPlanes[2].m_vNormal.z);
		if (0.0f != fMag)
		{
			fInv = 1.0f / fMag;
			pXForm->m_aPlanes[2].m_vNormal.x *= fInv;
			pXForm->m_aPlanes[2].m_vNormal.y *= fInv;
			pXForm->m_aPlanes[2].m_vNormal.z *= fInv;
		}

		fMag = (float)sqrt(pXForm->m_aPlanes[3].m_vNormal.x * pXForm->m_aPlanes[3].m_vNormal.x
		                 + pXForm->m_aPlanes[3].m_vNormal.y * pXForm->m_aPlanes[3].m_vNormal.y
		                 + pXForm->m_aPlanes[3].m_vNormal.z * pXForm->m_aPlanes[3].m_vNormal.z);
		if (0.0f != fMag)
		{
			fInv = 1.0f / fMag;
			pXForm->m_aPlanes[3].m_vNormal.x *= fInv;
			pXForm->m_aPlanes[3].m_vNormal.y *= fInv;
			pXForm->m_aPlanes[3].m_vNormal.z *= fInv;
		}

		fMag = (float)sqrt(pXForm->m_aPlanes[4].m_vNormal.x * pXForm->m_aPlanes[4].m_vNormal.x
		                 + pXForm->m_aPlanes[4].m_vNormal.y * pXForm->m_aPlanes[4].m_vNormal.y
		                 + pXForm->m_aPlanes[4].m_vNormal.z * pXForm->m_aPlanes[4].m_vNormal.z);
		if (0.0f != fMag)
		{
			fInv = 1.0f / fMag;
			pXForm->m_aPlanes[4].m_vNormal.x *= fInv;
			pXForm->m_aPlanes[4].m_vNormal.y *= fInv;
			pXForm->m_aPlanes[4].m_vNormal.z *= fInv;
		}

		fMag = (float)sqrt(pXForm->m_aPlanes[5].m_vNormal.x * pXForm->m_aPlanes[5].m_vNormal.x
		                 + pXForm->m_aPlanes[5].m_vNormal.y * pXForm->m_aPlanes[5].m_vNormal.y
		                 + pXForm->m_aPlanes[5].m_vNormal.z * pXForm->m_aPlanes[5].m_vNormal.z);
		if (0.0f != fMag)
		{
			fInv = 1.0f / fMag;
			pXForm->m_aPlanes[5].m_vNormal.x *= fInv;
			pXForm->m_aPlanes[5].m_vNormal.y *= fInv;
			pXForm->m_aPlanes[5].m_vNormal.z *= fInv;
		}
	}

	// ------------------------------------------------------------------ //
	//  (s) 0x10020788 .. 0x100208f6 -- the side planes into world space.
	//
	//  THE TRANSPOSE IS WRITTEN OUT HERE AND CALLED AT 0x1001f166.  The
	//  twelve `mov`s at 0x100207b5..0x100207f7 are 0x1001e420's body in
	//  0x1001e420's order, inlined; the rule "inlining budget is spent
	//  from the END backwards" is the reading, and the two spellings are
	//  reproduced deliberately because the CALL SEQUENCE depends on it.
	//
	//  The `test %eax,%eax ; je` for i == 0 is unreachable (icl rotated
	//  the loop so the first test sees i == 1) but is still emitted --
	//  the rule "VC6 keeps both branches of an unsatisfiable guard and
	//  folds only the body".
	// ------------------------------------------------------------------ //
	//  (The four `m_aPlanes[2..5].m_fDist = 0.0f` stores land HERE, at
	//  0x10020790..0x100207a2 in the order 0x39c, 0x3bc, 0x3ac, 0x3cc --
	//  i.e. planes 2, 4, 3, 5 -- from the register icl had just zeroed.
	//  They are written above, with the rest of the plane table, because
	//  the only thing between the two positions is the four `sqrt` calls
	//  and icl knows `sqrt` touches no memory.  Sinking a constant store
	//  past a memory-clean CRT call is the compiler's; nothing in the
	//  bytes distinguishes the two source positions.)

	mT = pXForm->m_MatWorld;

	{
		float	t;

		t = mT.m[0][1];  mT.m[0][1] = mT.m[1][0];  mT.m[1][0] = t;
		t = mT.m[0][2];  mT.m[0][2] = mT.m[2][0];  mT.m[2][0] = t;
		t = mT.m[1][2];  mT.m[1][2] = mT.m[2][1];  mT.m[2][1] = t;
	}

	for (i = 0; i < 6; i++)
	{
		if (i == 0)
			continue;

		if (i == 1)
			continue;

		pXForm->m_aWorld[i].m_vNormal.x =
			  mT.m[0][0] * pXForm->m_aPlanes[i].m_vNormal.x
			+ mT.m[0][1] * pXForm->m_aPlanes[i].m_vNormal.y
			+ mT.m[0][2] * pXForm->m_aPlanes[i].m_vNormal.z;
		pXForm->m_aWorld[i].m_vNormal.y =
			  mT.m[1][0] * pXForm->m_aPlanes[i].m_vNormal.x
			+ mT.m[1][1] * pXForm->m_aPlanes[i].m_vNormal.y
			+ mT.m[1][2] * pXForm->m_aPlanes[i].m_vNormal.z;
		pXForm->m_aWorld[i].m_vNormal.z =
			  mT.m[2][0] * pXForm->m_aPlanes[i].m_vNormal.x
			+ mT.m[2][1] * pXForm->m_aPlanes[i].m_vNormal.y
			+ mT.m[2][2] * pXForm->m_aPlanes[i].m_vNormal.z;

		pXForm->m_aWorld[i].m_fDist =
			  pXForm->m_aWorld[i].m_vNormal.x * pXForm->m_vPos.x
			+ pXForm->m_aWorld[i].m_vNormal.y * pXForm->m_vPos.y
			+ pXForm->m_aWorld[i].m_vNormal.z * pXForm->m_vPos.z;
	}

	// ------------------------------------------------------------------ //
	//  (t) 0x100208f6 .. 0x10020bb0 -- the near and far caps, whose
	//  distances come from a point on the plane rather than the origin.
	//  vDir is read out of mRot's ROW 2 (-0x650/-0x64c/-0x648 = -0x670 +
	//  0x20/0x24/0x28), which after the transpose is the camera matrix's
	//  column 2 -- the same vector GetBasisVectors put in m_v4a8, but the
	//  bytes read the local, not the member.
	// ------------------------------------------------------------------ //
	{
		LTVector	vDir;
		LTVector	vPoint;

		vDir.x = mRot.m[2][0];
		vDir.y = mRot.m[2][1];
		vDir.z = mRot.m[2][2];

		vPoint = vDir * pLens->m_fNear;

		vPoint.x += pXForm->m_vPos.x;
		vPoint.y += pXForm->m_vPos.y;
		vPoint.z += pXForm->m_vPos.z;

		pXForm->m_aWorld[0].m_vNormal.x =
			  mT.m[0][0] * pXForm->m_aPlanes[0].m_vNormal.x
			+ mT.m[0][1] * pXForm->m_aPlanes[0].m_vNormal.y
			+ mT.m[0][2] * pXForm->m_aPlanes[0].m_vNormal.z;
		pXForm->m_aWorld[0].m_vNormal.y =
			  mT.m[1][0] * pXForm->m_aPlanes[0].m_vNormal.x
			+ mT.m[1][1] * pXForm->m_aPlanes[0].m_vNormal.y
			+ mT.m[1][2] * pXForm->m_aPlanes[0].m_vNormal.z;
		pXForm->m_aWorld[0].m_vNormal.z =
			  mT.m[2][0] * pXForm->m_aPlanes[0].m_vNormal.x
			+ mT.m[2][1] * pXForm->m_aPlanes[0].m_vNormal.y
			+ mT.m[2][2] * pXForm->m_aPlanes[0].m_vNormal.z;

		pXForm->m_aWorld[0].m_fDist =
			  pXForm->m_aWorld[0].m_vNormal.x * vPoint.x
			+ pXForm->m_aWorld[0].m_vNormal.y * vPoint.y
			+ pXForm->m_aWorld[0].m_vNormal.z * vPoint.z;

		vPoint = vDir * pXForm->m_fFar;

		vPoint.x += pXForm->m_vPos.x;
		vPoint.y += pXForm->m_vPos.y;
		vPoint.z += pXForm->m_vPos.z;

		pXForm->m_aWorld[1].m_vNormal.x =
			  mT.m[0][0] * pXForm->m_aPlanes[1].m_vNormal.x
			+ mT.m[0][1] * pXForm->m_aPlanes[1].m_vNormal.y
			+ mT.m[0][2] * pXForm->m_aPlanes[1].m_vNormal.z;
		pXForm->m_aWorld[1].m_vNormal.y =
			  mT.m[1][0] * pXForm->m_aPlanes[1].m_vNormal.x
			+ mT.m[1][1] * pXForm->m_aPlanes[1].m_vNormal.y
			+ mT.m[1][2] * pXForm->m_aPlanes[1].m_vNormal.z;
		pXForm->m_aWorld[1].m_vNormal.z =
			  mT.m[2][0] * pXForm->m_aPlanes[1].m_vNormal.x
			+ mT.m[2][1] * pXForm->m_aPlanes[1].m_vNormal.y
			+ mT.m[2][2] * pXForm->m_aPlanes[1].m_vNormal.z;

		pXForm->m_aWorld[1].m_fDist =
			  pXForm->m_aWorld[1].m_vNormal.x * vPoint.x
			+ pXForm->m_aWorld[1].m_vNormal.y * vPoint.y
			+ pXForm->m_aWorld[1].m_vNormal.z * vPoint.z;
	}

	// ------------------------------------------------------------------ //
	//  (u) 0x10020bb0 .. 0x10020bf9 -- the clip copy and the tail.
	//  0x60 = six 16-byte planes, and 0x43c is m_aClip[0].m_fDist.
	// ------------------------------------------------------------------ //
	memcpy(&pXForm->m_aClip[0], &pXForm->m_aPlanes[0], 0x60);

	pXForm->m_aClip[0].m_fDist = 0.01f;

	pXForm->m_n4cc = 0;
	pXForm->m_f4d0 = 1.0f;

	sub_1001e660();

	return 1;
}


// *********************************************************************** //
//
//  NO `// UNDECODED:` REGIONS.  Every instruction in 0x1001ef30 ..
//  0x10020bf9 is accounted for by the source above.  Nothing is left as a
//  hole and nothing is named only by an address inside the body -- the
//  three addresses that stay opaque are all CALLEES (0x1001e600,
//  0x1001e8d4, 0x1001e660), each already described by an earlier file.
//
//  ===================================================================== //
//  ARM ORDER:  THERE IS NOTHING FOR IT TO COST HERE
//  ===================================================================== //
//
//  The second "thing that costs calls" is an arm-order coin-flip
//  where either arm makes a call.  This function has ELEVEN branches and
//  NOT ONE of them has a call in either arm:
//
//      0x1001f020 jbe   the low clamp        -- one arm, no call
//      0x1001f03d jbe   the high clamp       -- one arm, no call
//      0x100204f3 ja    the corner loop's back edge
//      0x100205ef je  } the four normalisation guards.  The `sqrt` call
//      0x10020664 je  } is BEFORE each guard, in the condition's own
//      0x100206d9 je  } expression, so it is issued unconditionally on
//      0x1002074e je  } both paths and the guard cannot move it.
//      0x10020803 jae   the plane loop's exit
//      0x1002080b je    `if (i == 0) continue;`   (unreachable, kept)
//      0x10020810 je    `if (i == 1) continue;`
//      0x100208f1 jmp   the plane loop's back edge
//
//  So every arm order below is size- and layout-only, and the call-sequence
//  figure is unaffected by any of them.  They are still stated.
//
//  ===================================================================== //
//  PREDICTED CALL-SEQUENCE RESIDUAL
//  ===================================================================== //
//
//  EXPECTED: shipped 39, ours 35, in agreement 35, shipped-only 4,
//  ours-only 0 -- i.e. 35 OF 39.
//
//  The four shipped-only rows are all 0x1007fc98, the CRT `sqrt`, at
//  0x100205dc / 0x10020651 / 0x100206c6 / 0x1002073b.  That is the
//  documented toolchain gap: icl holds `sqrt` as an intrinsic and expands
//  `fsqrt`.  Do NOT try `#pragma function(sqrt)`; it has been
//  tested and inert on two targets.
//
//  TWO FURTHER RISKS, stated in advance so the first build settles them
//  rather than being explained afterwards:
//
//    * `memcpy` (0x100805d0, two sites).  It is declared here WITHOUT
//      <string.h> exactly as d3dblit.cpp declares it, so it should stay a
//      call; if icl expands it anyway the figure drops to 33 of 39 with
//      two more shipped-only rows at 0x100805d0 and nothing else moves.
//    * `atan` (0x1007fe1c, two sites).  Same shape.  If icl expands
//      `fpatan` the figure drops by two more.
//      Neither is a decode error and both are visible as shipped-only
//      rows on a CRT address with zero ours-only rows opposite.
//
//  If instead `d3d_MatMul` fails to inline, the tell is the opposite:
//  THREE ours-only rows to an unresolvable local symbol, and the fix is
//  the file's, not the decode's.
//
//  ===================================================================== //
//  COIN-FLIPS, both spellings stated so one build settles each.
//  None of them moves a call.
//  ===================================================================== //
//
//   (A) m_fNear before m_fFar2.  The two stores are emitted the other way
//       round -- `mov %edx,0x90(%ebx)` at 0x1001f054 then
//       `mov %eax,0x88(%ebx)` at 0x1001f05a -- with their loads
//       interleaved above them, which is a schedule inside one basic
//       block.  Neither statement has a literal, so the pool is silent.
//       ALTERNATIVE:
//           pXForm->m_fFar2 = pXForm->m_fFar;
//           pXForm->m_fNear = pLens->m_fNear;
//
//   (B) The two clamp guards' operand order.  Written `3.0f > m_fFar` and
//       `m_fFar > 500000.0f` because at 0x1001f004 the BOUND is loaded
//       first (it is %st(0) for the `fcomps`) and at 0x1001f034 the
//       VARIABLE is.  That is "operand order survives" applied
//       to x87, and big17c14.cpp states it in the same words ("icl emits
//       the source's LEFT operand as the `fcomp` destination").
//       ALTERNATIVE:
//           if (pXForm->m_fFar < 3.0f)        pXForm->m_fFar = 3.0f;
//           if (500000.0f < pXForm->m_fFar)   pXForm->m_fFar = 500000.0f;
//       which should swap the two `fcomps` operands and nothing else.
//
//   (C) fX before fY in the corner table.  The emitted order is the other
//       way: `pLens->m_fHalfH * fInvFocal * m_fFar` is computed first
//       (0x10020398) and negated last.  Written ascending here to match
//       the .x stores.  No literal in either, so the pool is silent.
//       ALTERNATIVE: compute fY first.
//
//   (D) `if (i == 0) continue; if (i == 1) continue;` in the plane loop.
//       Two separate `je` to the loop head with the body falling through.
//       ALTERNATIVE, byte-for-byte the same:
//           if (i != 0 && i != 1) { ...body... }
//       The i == 0 arm is unreachable either way -- icl rotated the loop
//       so the increment precedes the first test and i is 1 there -- and
//       it survives because icl keeps an unsatisfiable guard's branches
//       and folds only the body.
//
//   (E) `mZ.m[2][2] = 1.0f * fQ;`.  The pool is unambiguous that m[2][2]
//       carries EXACTLY ONE 1.0f literal (0x100a52d4, loaded at
//       0x1001f649) and that m[2][3] carries none, and the emitted code
//       really does multiply fQ by a loaded 1.0f at 0x1001f69d.  What it
//       cannot say is what that 1.0f MEANT: the shape is the standard
//       z-range row f/(f-n), -n*f/(f-n) with the far plane normalised to
//       1, so it is very likely a named `1.0f` far constant.
//       ALTERNATIVE, identical code: `mZ.m[2][2] = fQ * 1.0f;`
//       WHAT IS NOT AN ALTERNATIVE: writing `mZ.m[2][2] = fQ;`.  That
//       loses the pool slot and the `fmulp`.
//
//   (F) The plane table's statement order (section (q)).  Written plane 0
//       through plane 5, each x/y/z/d.  The STORES are scattered across
//       all six planes (0x3b8 is first and 0x38c last), which is one
//       basic block's schedule -- but the POOL is not scattered: its
//       fifteen literals at 0x100a52f8..0x100a5330 are in exactly the
//       order written here, and any other statement order would permute
//       them.  This one is therefore NOT really a coin-flip; it is
//       listed only because the emission looks like one.
//
//   (G) The transpose at 0x10020788 is written out by hand while the one
//       at 0x1001f166 is `sub_1001e420(&mRot)`.  Both are the same twelve
//       `mov`s in the same order and the shipped build has one of each.
//       The inlining-budget rule says the LATER site gets the
//       richer expansion, which is what the bytes show.  If our compiler
//       makes the same choice from one spelling, the right source is
//       `sub_1001e420(&mT);` at both sites and this file is one
//       ours-only row light.  Written as it is because the CALL SEQUENCE
//       is the acceptance test and this reproduces it exactly.
//
//   (H) `sub_1001e600`'s prototype.  Its first float argument really
//       arrives on the x87 stack (d3dr2a.cpp: "the body opens
//       `fsub %st(1),%st` against an x87 value the CALLER left on the
//       stack") and its second is at 0xc(%esp) inside a function with no
//       prologue.  There is no C spelling.  Declared here as
//       `(LTMat*, float, float)` so that the CALL is emitted at the right
//       point in the sequence; the argument marshalling will not match
//       and is not claimed to.
//
//  ===================================================================== //
//  WHAT COULD NOT BE PINNED
//  ===================================================================== //
//
//   1. The pool's tail, 0x100a5334..0x100a5374 -- four 0.0f, the 0.01f,
//      and twelve 1.0f of which four are loaded in REVERSE program order.
//      The 0.01f belongs to the function's last statement and the twelve
//      1.0f come after it, so this run is not in program order at all.
//      Recorded, not used.
//
//   2. The "extra" 1.0f that accompanies every `1.0f / <memory operand>`
//      (0x100a5240/5248/5250/5258/5268) but not the two reciprocals at
//      0x1001f275 and inside fQ.  See section 4's closing paragraph.  It
//      costs nothing here because every ORDER conclusion rests on which
//      slot is LOADED.
//
//   3. What the object at +0x4c0..+0x4cb is.  Nothing in these 7,370
//      bytes touches it; it is padding in the declaration above purely so
//      that sizeof(D3DXForm) comes out 0x4f8, which is the size
//      big26728.cpp measures for the global at 0x1008b880 (`rep movsl`
//      with %ecx = 0x13e).  That the declaration reaches 0x4f8 with
//      exactly one unexplained twelve-byte hole is itself a check on
//      every offset above it, and it passes.
//
//   4. Which of m_v490 / m_v49c / m_v4a8 is "right", "up" and "forward".
//      The COLUMN each comes from is read off 0x1001e4d0 and is certain;
//      the names are not, so none is given.
//
// *********************************************************************** //
