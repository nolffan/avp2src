// *********************************************************************** //
//
//  MODULE:   BIG_8FD0 (.CPP)  --  d3d.ren 0x10008fd0 .. 0x1000985f
//
//  ONE function, 2,191 bytes, 661 instructions, of which 472 are x87.
//
//  PURPOSE:  CD3DModelDraw's SOFTWARE VERTEX PIPELINE for one skinned
//            model piece.  Given a source vertex set, a destination run
//            of 32-byte FVF-0x1c4 vertices, a per-bone matrix palette and
//            a texture-coordinate callback, it walks the vertices and for
//            each one:
//
//              (1) blends it through the matrix palette -- for every
//                  weight record of the vertex, out(4) += M[record's
//                  bone] * record's weighted homogeneous position -- and,
//                  when the object is interpolating between two animation
//                  keys (m_n450 != 0), does the same for the SECOND key's
//                  vertex into four locals and LERPs the two results with
//                  m_fLerp454;
//              (2) projects: fInvW = 1.0f / out.w, out.xyz *= fInvW,
//                  out.rhw = fInvW;
//              (3) lights: the global directional term
//                  dot(vNormal, m_vLightDir674) * m_f680 selects between
//                  the ambient colour m_vAmbient684 and a LERP towards
//                  the sun colour m_v668, then every one of the up-to-16
//                  model-space dynamic lights in m_Lights3c[] that faces
//                  the normal and is within its radius adds
//                  its colour * (1 - d^2/r^2) * dot;
//              (4) scales that by (m_fR65c, m_fG660, m_fB664), biases by
//                  m_v650, clamps each channel at 255.0f, converts to
//                  three bytes into the vertex's diffuse dword and stamps
//                  m_bAlpha6a8 as the alpha;
//              (5) calls this->m_pfnTexCoord42c(pVert, pOut) to fill the
//                  vertex's u/v.
//
//            The whole function is ONE `if (m_n450) { two-key } else
//            { one-key }`, and the two arms are near-identical: regions
//            R13..R17 below are instruction-for-instruction the same code
//            as R4..R8 with a second copy of the literal pool.  That
//            internal duplication is this decode's strongest structural
//            check and it is used as such throughout.
//
//  Compiler: Intel C/C++ 4.0, /O2 /Qip.  The /Qip pair is visible at the
//            entry (see THE EXTENT below).  /Qrcd is REQUIRED -- see the
//            census note on `fistp`.  /Oi- is IRRELEVANT here: there is
//            not one CRT-intrinsic candidate in the function (no fsqrt,
//            no fabs, no fpatan, no fptan, no fscale, no rep movs), so the
//            flag cannot move a byte.  Both `call`s are indirect.
//
// ----------------------------------------------------------------------- //
//  1.  THE EXTENT -- CONFIRMED AT BOTH EDGES BY HAND
// ----------------------------------------------------------------------- //
//
//  The enumeration carries `0x10008fd4, 2,187`, which is the /Qip REGISTER
//  entry.  The real function starts four bytes earlier at its STACK entry,
//  so the extent is 0x10008fd0..0x1000985f, 2,191 bytes.  Verified:
//
//        0x10008fd0..0x1000985f  2,191 bytes  661 instructions
//        ret               1        0x1000981b
//        push %ebp frames  1        0x10008fd4
//        /Qip              start is the STACK entry; register entry at
//                          0x10008fd4
//        branch targets    0 outside the range or mid-instruction
//        last instruction  ends exactly at the stated end
//
//  Both edges were also read by hand:
//
//  (a) THE START.  The previous function's last instruction is a `jmp`,
//      then nine bytes of lea-nop padding to a 16-byte boundary:
//
//        10008fb8  ret                          <- previous function's
//        10008fb9  mov 0x640(%ebx),%eax            epilogue block and its
//        10008fbf  mov %eax,-0x14(%ebp)            sunk arm
//        10008fc2  jmp 0x10008e38                  (5 bytes, ends 0x8fc7)
//        10008fc7  mov %esi,%esi                <- 2 bytes of padding
//        10008fc9  lea 0x0(%edi,%eiz,1),%edi    <- 7 more, 9 in all
//        10008fd0  mov 0x4(%esp),%eax           <- the /Qip STACK entry,
//        10008fd4  push %ebp                       ONE argument load
//        10008fd5  mov %esp,%ebp
//        10008fd7  sub $0xf4,%esp
//
//      One argument load, so ONE register argument -- the walk-back is 4
//      bytes, not 8.  0x10008fd0 is 16-byte aligned and preceded by
//      padding: it is the function start.  Nothing in .text branches into
//      0x10008fc7..0x10008fcf (checked over the whole image).
//
//      A caution worth recording: the register entry does NOT consume the
//      %eax the stack entry loads.  0x10008fe8 is `mov 0x8(%ebp),%eax`,
//      which re-reads the same argument out of its home slot before %eax
//      is read anywhere, and the two callers (below) both leave an
//      unrelated value in %eax.  So the stack entry's load is DEAD; it is
//      still the /Qip stack entry -- the shape and the walk-back are
//      unaffected -- but do not read it as evidence that argument 1
//      travels in a register.
//
//  (b) THE END.  The last instruction is the sunk two-key ambient arm's
//      backward jump, and it ends flush at the stated end:
//
//        1000985a  jmp 0x10009659       <- 5 bytes, ends at 0x1000985f
//        1000985f  nop                  <- 1 byte of padding
//        10009860  push %ebp            <- the NEXT function, on a
//        10009861  mov %esp,%ebp           16-byte boundary
//        10009863  sub $0x10,%esp
//
//      0x1000985f is the padding byte, so 0x1000985f - 0x10008fd0 = 0x88f
//      = 2,191 and the body ends exactly there.  The next function has no
//      /Qip stack entry of its own (it opens `push %ebp` directly), so
//      there is no ambiguity at that edge either.
//
//  ONE `ret` (0x1000981b, `ret $0x14`) and ONE prologue over the whole
//  range.  Sixty-five bytes of code sit AFTER the `ret` -- the two sunk
//  "ambient only" arms at 0x1000981e..0x1000983c and
//  0x10009840..0x1000985e -- and each is reached only by a forward branch
//  from above the `ret` (0x10009160 -> 0x1000981e, 0x10009615 ->
//  0x10009840) and ends in a `jmp` back above it (0x10009838 ->
//  0x100091a4, 0x1000985a -> 0x10009659), using THIS frame.  There is no
//  second prologue and no padded-then-prologue boundary inside.
//
// ----------------------------------------------------------------------- //
//  2.  THE SIGNATURE, AND THE TWO CALL SITES IT WAS READ FROM
// ----------------------------------------------------------------------- //
//
//      void CD3DModelDraw::sub_10008fd0(ModelVertSet* pSet1,
//                                       ModelVertSet* pSet2,
//                                       RVert*        pOut,
//                                       PFNTEXCOORD   pfnTexCoord,
//                                       LTMat*        pMatrices);   // ret $0x14
//
//  `ret $0x14` -- twenty bytes, five dword arguments, callee-pops -- and
//  %ecx is read live-in at 0x10008feb (`mov %ecx,%esi`) and never
//  reloaded.  __thiscall with five stack arguments.  Every argument is
//  read off the frame in the prologue:
//
//      10008fe8  mov 0x8(%ebp),%eax   -> pSet1 ; +0x4 = pVerts, +0x8 = count
//      10008feb  mov %ecx,%esi        -> this
//      10008fed  mov 0x18(%ebp),%edx  -> pMatrices, cached at -0x5c
//      10008ff6  mov 0x14(%ebp),%ebx  -> pfnTexCoord, cached at -0x60
//      1000900c  mov 0x10(%ebp),%edi  -> pOut, the walking destination
//      10009360  mov 0xc(%ebp),%edx   -> pSet2, read ONLY in the two-key
//                                        arm; may legitimately be NULL in
//                                        the one-key arm (the caller at
//                                        0x10008f44 hands over a slot it
//                                        zeroed at 0x10008e43)
//
//  Return type `void`: the three exits (0x10009017, 0x1000936b and the two
//  loop-bottom fallthroughs) all reach 0x1000980d with three different
//  things in %eax and nothing sets it deliberately.
//
//  TWO callers, found by scanning all of .text for `call 0x10008fd0` and
//  `call 0x10008fd4` (there is no dword anywhere in the image holding
//  either address, so there is no table-driven caller):
//
//    * 0x10008f65 -> 0x10008fd4, the /Qip REGISTER entry, from
//      CD3DModelDraw::sub_10008e10 -- the same translation unit.  It sets
//      up all five arguments with `add $0xffffffec,%esp` plus five
//      `mov %reg,K(%esp)` (no pushes -- /O2), and %ecx = %ebx = this:
//
//        10008f3f  add $-0x14,%esp
//        10008f4b  mov %edi,(%esp)             ; pSet1
//        10008f51  mov %edx,0x4(%esp)          ; pSet2   (-0x10(%ebp))
//        10008f47  mov %eax,0x8(%esp)          ; pOut = m_n62c + i*0x20
//        10008f5f  mov %edx,0xc(%esp)          ; this->m_pfnTexCoord42c
//        10008f5b  mov %eax,0x10(%esp)         ; pMatrices
//        10008f63  mov %ebx,%ecx
//        10008f65  call 0x10008fd4
//
//      and 0x10008e32 is where pMatrices comes from: `mov (%ebx),%eax ;
//      mov 0x74(%eax),%eax` -- this->m_pModel00->m_pMatrices74.  The same
//      array is indexed with `imul $0x40` at 0x100188cb in another
//      function, which is the independent confirmation that its element
//      is 64 bytes.
//
//    * 0x1004c3f6 -> 0x10008fd0, the STACK entry, from a different TU.
//      Identical argument block (`add $0xffffffec,%esp` at 0x1004c3cc,
//      five `mov`s, `mov %esi,%ecx`), with pfnTexCoord again taken from
//      `0x42c(%esi)` at 0x1004c3e6.
//
//  Both callers therefore pass this->m_pfnTexCoord42c as argument 4, which
//  is what pins the two indirect calls (section 3).
//
// ----------------------------------------------------------------------- //
//  3.  THE CALL CHECK IS VACUOUS-ADJACENT: 2 CALLS, BOTH INDIRECT
// ----------------------------------------------------------------------- //
//
//        2,191 bytes, 2 calls (0 DIRECT, 2 indirect), 1096 bytes per call
//
//      distinct direct callees: 0 of 0
//
//  PREDICTED call-check result: `shipped calls 2, ours 2, in agreement 2 (0
//  by resolved target, 2 BY POSITION ONLY), shipped-only 0, ours-only 0`.
//  The shipped order is 0x1000933c then 0x100097fa -- the ONE-KEY arm's
//  call first, the TWO-KEY arm's second -- because the one-key arm is the
//  fall-through of `jne 0x10009360` and therefore sits at the lower
//  address, while the literal pool says the source has the TWO-KEY arm
//  first (instrument 3).  A reconstruction written in source order
//  therefore emits its two calls in the opposite order from the shipped
//  bytes.  It makes no difference to the check: both calls are indirect,
//  both resolve to nothing, and the check's LCS alignment sees two
//  positions against two positions and reports agreement either way.  The
//  check literally cannot tell the two arms apart, nor tell either of
//  them from a call to anything else in the image.
//
//  WHAT FRACTION OF THE BYTES THE CALL CHECK WITNESSES:
//
//      the two `call *%eax` instructions are 2 bytes each
//          4 of 2,191 bytes  = 0.18%
//          2 of 661 instructions = 0.30%
//      bytes witnessed BY RESOLVED TARGET:  0 of 2,191  = 0.00%
//      x87 instructions witnessed:          0 of 472    = 0.00%
//
//  This is 1,096 B/call against a "weak" threshold of
//  300, with zero discrimination because zero callees resolve.  It is one
//  notch better than 0x100051a0's outright `NO CALLS IN RANGE` and no
//  more.  Everything that follows -- the census, the frame, the pool and
//  the internal-duplication correspondence -- is not supplementary
//  evidence here, it IS the evidence.
//
//  WHAT PINS THE TWO INDIRECT TARGETS.  Both are the same value:
//
//      1000933c  mov -0x60(%ebp),%eax ; call *%eax     (one-key arm)
//      100097fa  mov -0x60(%ebp),%eax ; call *%eax     (two-key arm)
//
//  -0x60(%ebp) is argument 4, and BOTH callers load argument 4 from
//  `0x42c(this)` (0x10008f55 and 0x1004c3e6).  +0x42c is a function-
//  pointer member of CD3DModelDraw, and it is written in exactly two
//  places in the image:
//
//    * 0x10044f8b, inside the NAMED INSTALLER at 0x10044f70 --
//      `mov 0x8(%ebp),%eax ; mov %eax,0x434(%ebx) ; mov 0xc(%ebp),%edx ;
//      mov %edx,0x42c(%ebx) ; movl $0x1,0x304(%ebx)`.  Its second
//      argument is our slot.  It has four call sites, and the pointer
//      each one installs is a compile-time immediate:
//          100442ad/100442b4  (0x10003aa0, 0x10003b50, 1)
//          10044369/10044371  (0x10003a40, 0x10003ad0, 1)
//          100443e7/100443f2  (0x10003a10, 0x10003ad0, 1)
//          10044bff/10044c06  (0x10003aa0, 0x10003b50, 1)
//    * 0x10043eb6, a direct `movl $0x10003ae0,0x42c(%esi)` immediately
//      before `call 0x10008e10` -- i.e. immediately before the very
//      caller that reaches us.
//
//  So m_pfnTexCoord42c is one of {0x10003ad0, 0x10003ae0, 0x10003b50}, a
//  three-member family of texture-coordinate generators sharing one
//  signature, verified from their bodies:
//
//      0x10003ad0  `sub $0x8,%esp ; add $0x8,%esp ; ret $0x4`   -- the
//                  DO-NOTHING generator.  Fixes the argument size at one
//                  stack dword and the convention at callee-pops.
//      0x10003ae0  `mov 0x4(%esp),%eax` then reads 0x14/0x18/0x1c(%edx)
//                  -- the source vertex's NORMAL -- against
//                  0x3d0..0x3e8(%ecx) (CD3DModelDraw::m_Mat3d0) and
//                  0x45c..0x468(%ecx), and stores two floats to
//                  0x18(%eax) and 0x1c(%eax).  `ret $0x4`.
//      0x10003b50  `push %ebx ; mov 0x8(%esp),%ebx` -- same argument,
//                  reads 0x14/0x18/0x1c(%edx) against 0x674..0x67c(%ecx),
//                  writes 0x18(%ebx)/0x1c(%ebx).  `ret $0x4`.
//
//  which is exactly what the call site hands over:
//
//      10009336  push %edi              ; pOut, the one stack argument
//      10009337  mov %ebx,%edx          ; pVert, in %edx
//      10009339  mov -0x60(%ebp),%eax
//      1000933c  call *%eax             ; %ecx is still `this`
//
//  So: `this` in %ecx, the SOURCE vertex in %edx as a register-only
//  argument with no stack home, the DESTINATION vertex as the single
//  stack argument, callee pops 4.  The +0x18/+0x1c stores are the two
//  texture coordinates of the destination vertex, which is what makes
//  this a texture-coordinate generator and not something else.
//
//  NOT the device vtable.  0x1008d988 -- the device pointer whose slots
//  several decoded files record -- is reached at 0x10044f37/0x10044f5a in
//  the installer's own neighbourhood but plays no part in these two calls.
//
// ----------------------------------------------------------------------- //
//  4.  INSTRUMENT 1 -- THE INSTRUCTION CENSUS, WITH NO RESIDUE
// ----------------------------------------------------------------------- //
//
//  661 instructions.  The region table below partitions the WHOLE range
//  0x10008fd0..0x1000985f -- every byte, padding included -- and its
//  counts sum to 661 exactly, mechanically (a
//  disassembler over `objdump -d -M intel`, bucketed by the region
//  boundaries listed).  No instruction is outside a region and no region
//  overlaps another.
//
//   region                        range               n
//   R0  /Qip stack entry          8fd0..8fd4          1
//   R1  prologue + dispatch       8fd4..901d         22
//   R2a one-key: zero + loop head 901d..9050         16
//   R2b one-key: blend loop body  9050..9105         63
//   R3  one-key: projection       9105..9134         16
//   R4  one-key: sun dot test     9134..9166         14
//   R5  one-key: sun LERP         9166..91a4         13
//   R6a one-key: light setup      91a4..91c2          9
//   R6b one-key: light loop body  91c2..9250         53
//   R6c one-key: light loop join  9250..925f          6
//   R7  one-key: scale/clamp/byte 925f..932b         50
//   R8  one-key: alpha+call+latch 932b..9354         13
//   R9  lea-nop padding           9354..9360          2
//   R10a two-key: head + zero     9360..93b0         26
//   R10b two-key: blend loop 1    93b0..946b         65
//   R11a two-key: zero acc        946b..94a0         13
//   R11b two-key: blend loop 2    94a0..956c         61
//   R12 two-key: LERP + project   956c..95e9         36
//   R13 two-key: sun dot test     95e9..961b         14
//   R14 two-key: sun LERP         961b..9659         13
//   R15a two-key: light setup     9659..9680         11
//   R15b two-key: light loop body 9680..970e         53
//   R15c two-key: light loop join 970e..971d          6
//   R16 two-key: scale/clamp/byte 971d..97e9         50
//   R17 two-key: alpha+call+latch 97e9..980d         12
//   R18 epilogue                  980d..981e          6
//   R19 SUNK one-key ambient arm  981e..9840          9
//   R20 SUNK two-key ambient arm  9840..985f          8
//                                              -------
//                                                    661
//
//  1 + 22 + 16 + 63 + 16 + 14 + 13 + 9 + 53 + 6 + 50 + 13 + 2 + 26 + 65
//  + 13 + 61 + 36 + 14 + 13 + 11 + 53 + 6 + 50 + 12 + 6 + 9 + 8 = 661.
//
//  Whole-function mnemonic totals, which also sum to 661:
//
//    fld    135    fmul    96    mov     91    fstp    55    faddp   54
//    fxch    48    fadd    28    fsub    13    fcomp   12    fnstsw  12
//    sahf    12    add     11    test    10    jbe      8    lea      7
//    jne      6    imul     6    jae      6    fistp    6    je       5
//    dec      5    fsubr    5    movzx    4    fmulp    4    push     3
//    xor      3    jmp      3    fst      2    cmp      2    jb       2
//    call     2    sub      1    fdiv     1    fdivr    1    pop      1
//    ret      1
//
//    x87, counted three ways, because the figure quoted for this
//    function elsewhere is 454 and all three numbers are right:
//      472  every mnemonic beginning `f`
//      460  minus the 12 `fnstsw` (the status-word half of a compare,
//           which is really branch plumbing)
//      454  minus the 6 `fistp` as well (the conversion half, which is
//           an integer store)
//    and 189 of the 661 are integer instructions (91 mov, 12 sahf,
//    11 add, 10 test, 8 jbe, 7 lea, 6 each jne/imul/jae, 5 each je/dec,
//    4 movzx, 3 each push/xor/jmp, 2 each cmp/jb/call, 1 each
//    sub/pop/ret).  189 + 472 = 661.
//
//  ============ EVERY ROW OVER FIVE, DECOMPOSED BY OPERAND ============
//
//  The three blend loops and the two light loops are the function, so
//  each row is broken into "n per iteration x m sites + k setup".
//
//  fmul  96 = 16 + 16 + 16   three blend loops, 4 rows x 4 columns each
//           +  2             one-key projection: x*=invW, y*=invW
//                            (the third is the `fmulp` below)
//           +  6             two-key LERP: 4 x m_fLerp454, plus the two
//                            projection multiplies
//           +  4 + 4         sun dot product, 3 terms + m_f680, x2 arms
//           +  2 + 2         sun LERP: two of the three t-multiplies
//                            (the third is the `fmulp` below), x2 arms
//           + 11 + 11        light loop body, x2 arms:
//                              3  dot(vDir, vNormal)
//                              3  dx*dx, dy*dy, dz*dz
//                              1  m_fInvRadiusSqr1c * fDistSqr
//                              1  attenuation * fDot
//                              3  m_fR20/m_fG24/m_fB28 * that
//           +  3 + 3         m_fR65c/m_fG660/m_fB664, x2 arms
//           = 48+2+6+8+4+22+6 = 96.  ZERO left over.
//
//  fld  135 = 19 + 19 + 16   the three blend loops (see the two shapes
//                            below -- 19 for the two that accumulate into
//                            the DESTINATION vertex, 16 for the one that
//                            accumulates into four frame slots)
//           +  5             one-key projection: 1.0f, z, y, x, and the
//                            hoisted 0.0f of the sun test
//           +  8             two-key LERP+projection: 4 accumulators
//                            re-read (1 `fld`, 3 folded into `fsubr`),
//                            so 1 + 3 preloads of out.y/z/w + z,y,x
//                            + the hoisted 0.0f  = 8
//           +  3 + 3         sun dot: the three normal components, x2
//           +  3 + 3         sun LERP: m_v668.x/.y/.z, x2
//           + 11 + 11        light body, x2:
//                              3  m_vDir0c.x/.y/.z
//                              1  the 0.0f of the dot test
//                              3  vPos.z, vPos.y, vPos.x (the fsub side)
//                              1  m_fRadiusSqr18
//                              1  m_fInvRadiusSqr1c
//                              2  m_fR20 and m_fG24 (m_fB28 folds into
//                                 an `fmul` of the surviving st0)
//           +  3 + 3         light loop join: the three accumulators
//                            reloaded out of their spill slots, x2
//           + 11 + 11        scale/clamp/convert, x2, verified by listing
//                            them (0x1000926b, 92a0, 92a6, 92b4, 92bd,
//                            92cb, 92da, 92ec, 92f8, 930a, 9316):
//                              1  the FIRST 255.0f, hoisted into st0 for
//                                 the R compare
//                              3  the three replacement 255.0f
//                              2  channels G and B re-read out of their
//                                 spill slots for their clamps
//                              2  channels G and B re-read AGAIN for
//                                 their conversions
//                              3  the float reload of each conversion
//                                 round trip
//                            (the G and B COMPARES need no `fld`: they
//                            are `fcomp DWORD PTR ds:0x100a42f8` etc.,
//                            memory-operand compares, which is exactly
//                            what makes their operand order readable and
//                            calibrates the register form -- section 8.)
//           +  3 + 3         the two sunk ambient arms: m_vAmbient684's
//                            three components
//           = 54+5+8+6+6+22+6+22+6 = 135.  ZERO left over.
//
//  faddp 54 = 15 + 15        blend loops 1 and 2 (the ones that write the
//                            destination): row 0 has 3 (the 4-term dot)
//                            and rows 1..3 have 4, because each row
//                            PRELOADS the next component so the
//                            accumulate is a register add rather than
//                            `fadd mem`.  3 + 4 + 4 + 4 = 15.
//           + 12             blend loop 3 (four separate frame slots, so
//                            no preload chain): 3 per row x 4 = 12
//           +  2 + 2         sun dot product, x2 arms
//           +  4 + 4         light body, x2: 2 for the vDir dot and 2 for
//                            dx^2+dy^2+dz^2
//           = 30+12+4+8 = 54.  ZERO left over.
//
//  fstp  55 = 4 + 4 + 4      one store per row, three blend loops
//           +  3             one-key projection (z, y, x; w uses `fst`)
//           +  6             two-key LERP+projection: 3 LERP stores +
//                            3 projection stores (the 4th LERP's store
//                            is DEAD -- see region R12)
//           +  3 + 3         the three light accumulators spilled at the
//                            top of each light loop
//           +  5 + 5         light body, x2: fDistSqr, then R and G into
//                            their spill slots, then B, then the `fstp
//                            %st(0)` that discards fDot at the join
//           +  8 + 8         scale/clamp/convert, x2, listed at
//                            0x10009273, 9298, 929e, 92ba, 92d1, 92d4,
//                            92f2, 9310:
//                              2  channels B and G spilled
//                              1  `fstp %st(0)` -- the R clamp's TRUE
//                                 arm discarding the unclamped value,
//                                 because R is still in st0 and the
//                                 replacement is loaded over it
//                              2  the G and B clamps' replacement stores
//                              3  the float half of each round trip
//           +  1 + 1         the two sunk arms' discard of the dead
//                            product
//           = 12+3+6+6+10+16+2 = 55.  ZERO left over.
//
//  fxch  48 = 3 + 3 + 3      blend loops (the preload/store swap on rows
//                            0..2 of each)
//           +  2             one-key projection
//           +  5             two-key LERP + projection
//           +  1 + 1         sun dot (bringing the hoisted 0.0f to st0)
//           +  1 + 1         sun LERP
//           +  4 + 4         light body, x2
//           +  3 + 3         light loop join, x2 (restoring the R/G/B
//                            stack order the fall-through path has)
//           +  4 + 4         scale/clamp, x2
//           +  3 + 3         the two sunk arms (same 3-deep reorder)
//           = 9+2+5+2+2+8+6+8+6 = 48.  ZERO left over.
//
//  fadd  28 = 1 + 1          blend loops 1 and 2: only ROW 0 reads its
//                            accumulator from memory; rows 1..3 use the
//                            preloaded value.  THIS ROW IS LOAD-BEARING:
//                            a fourth `fadd mem` in either loop would
//                            mean the preload chain does not exist, and
//                            the 15/12 split of `faddp` would move with
//                            it.
//           +  4             blend loop 3: all four rows read their
//                            accumulator from memory, because the four
//                            targets are four separate frame slots
//           +  4             the two-key LERP's four `+ out.c` terms
//           +  3 + 3         sun LERP's `+ m_vAmbient684.c`, x2
//           +  3 + 3         light body's three colour accumulations, x2
//           +  3 + 3         the `+ m_v650.c` bias, x2
//           = 6+4+6+6+6 = 28.  ZERO left over.
//
//  fsub  13 and fsubr 5.  The pair is the whole subtraction census:
//           fsub  3 + 3      sun LERP: m_v668.c - m_vAmbient684.c, x2
//           fsub  3 + 3      light body: vPos.c - m_vPos00.c, x2
//           fsub  1          two-key LERP, component 0 only
//           fsubr 3          two-key LERP, components 1..3 (the operands
//                            arrive the other way round because the
//                            destination component is already in st0
//                            from the previous row's preload)
//           fsubr 1 + 1      light body: 1.0f - fDistSqr*m_fInvRadius, x2
//           = 13 and 5.  The 1/3 split inside the two-key LERP is the
//           tell that the four LERPs are ONE chained shape and not four
//           independent statements; four independent ones would give
//           four `fsub`.
//
//  fcomp 12 = fnstsw 12 = sahf 12.  Twelve float compares, six per arm:
//           1  the sun dot test          (jae -> the sunk ambient arm)
//           2  the light body's dot test and radius test
//           3  the three 255.0f clamps
//           x2 arms.  A seventh compare in either arm -- a lower clamp at
//           0.0f, say, or a w-epsilon guard before the divide -- would
//           show here and does not.  THERE IS NO LOWER CLAMP: the colour
//           channels are clamped at 255.0f ONLY, which is a behavioural
//           fact of the shipped renderer, not an omission in this decode.
//
//  fistp 6 = 3 + 3.  Three float-to-int conversions per arm, one per
//           colour channel, and each is `fstp DWORD; fld DWORD; fistp
//           DWORD; mov BYTE`.  This is icl's conversion
//           idiom and its /Qrcd form: at plain /O2 each of these becomes
//           the twelve-instruction shld/neg/or/shr/and/xor/cmp/test/jge/
//           jle truncation, which would add roughly 66 instructions to a
//           661-instruction function and cannot be reconciled with the
//           census above.  So this TU NEEDS /Qrcd.  The known residual
//           -- shipped `fistpl` with a float round trip versus
//           /Qrcd's 64-bit `fistpll` with none -- is present here and is
//           NOT closed by anything in this file.
//
//  fmulp 4 = 1 + 1  the third t-multiply of each sun LERP
//          + 1      the one-key projection's x*invW
//          + 1      the two-key projection's x*invW
//
//  fdiv 1 + fdivr 1 = TWO divisions, one per arm, and the literal pool
//          independently says two (instrument 3).  `fdiv 0xc(%edi)` in
//          the one-key arm because the accumulated w is in MEMORY there
//          (the blend loop wrote it); `fdivr 0x100a42a4` in the two-key
//          arm because the LERPed w is in st0 and its store was
//          eliminated.  Both compute 1.0f / w.
//
//  fst 2 = 1 + 1.  The non-popping store of fInvW into out.rhw, once per
//          arm; non-popping because the three multiplies still need it.
//
//  imul 6 = 3  x 0x40, the matrix-palette index in the three blend loops
//         + 2  x 0x2c, the light-table walk bound in the two light loops
//         + 1  x 0x20, `pSet2->m_pVerts04[pVert->m_nIndex06]` in the
//              two-key arm.  ONE, not two: the one-key arm never touches
//              the second key.
//
//  movzx 4 = 1  one-key arm: pVert->m_nBlends04
//          + 2  two-key arm: pVert->m_nBlends04 and pVert->m_nIndex06
//          + 1  two-key arm: the SECOND key vertex's m_nBlends04
//          Four uint16 loads.  A fifth would mean the one-key arm reads
//          m_nIndex06, which it does not.
//
//  dec 5 = 2 vertex-loop latches (one per arm)
//        + 3 blend-loop latches (one in the one-key arm, two in the
//          two-key arm).  Explicitly:
//            1000902d  dec %edx     one-key vertex counter
//            100090fe  dec %edx     one-key blend loop
//            1000938c  dec %ecx     two-key vertex counter
//            1000945e  dec %ecx     two-key blend loop 1
//            10009562  dec %ecx     two-key blend loop 2
//          The two light loops do NOT `dec`; they walk a pointer and
//          `cmp` it, which is the `cmp` row of 2.
//
//  add 11 = 2 x 0x20 per arm (pVert++ and pOut++)      = 4
//         + 3 x 0x14, one per blend loop (record++)     = 3
//         + 2 x 0x2c, one per light loop (light++)      = 2
//         + 2 `add %esi,%ecx`, the light loops' end bound
//         = 11.
//
//  lea 7 = 2 (R9) + 1 (0x1000983d) lea-nop padding = 3 pad instructions
//        + 4 more lea-nops used as in-body alignment at 0x10009045,
//          0x10009049, 0x100093a9 and 0x10009679 (loop-head alignment).
//        Every one of the seven is a NOP.  There is not one real `lea` in
//        this function -- no address arithmetic escapes the addressing
//        modes, which is what says there is no aliasing temporary and no
//        named pointer into the middle of a structure.
//
//  mov 91, test 10, je 5, jne 6, jae 6, jbe 8, jb 2, xor 3, push 3,
//  cmp 2, sub 1, pop 1, ret 1: integer plumbing, fully attributed in the
//  per-region table above (R1 has 16 movs, R10a 16, R2a 9, R11a 9, R7 6,
//  R16 6, R8 6, R17 6, R18 4, R10b 3, R15a 3, R11b 2, R6a 2, R2b 1, R3 1,
//  R0 1 = 91).
//
// ----------------------------------------------------------------------- //
//  5.  INSTRUMENT 2 -- THE FRAME, DECOMPOSED EXACTLY, EVERY SLOT DUMPED
// ----------------------------------------------------------------------- //
//
//      10008fd7  sub $0xf4,%esp          244 bytes
//
//  Every ebp-relative and esp-relative slot the function touches, read
//  off the bytes (grep of the disassembly, not assumed):
//
//    slot     size  refs  what it is
//    ------   ----  ----  --------------------------------------------
//    -0xf4     4     1    ebx home           `mov %ebx,(%esp)`
//    -0xf0     4     1    esi home           `mov %esi,0x4(%esp)`
//    -0xec     4     1    edi home           `mov %edi,0x8(%esp)`
//    -0xe8..-0xd9  16     0    UNUSED (alignment)
//    -0xd8     4     2    two-key: float round trip, channel R
//    -0xd0     4     2    two-key: float round trip, channel G
//    -0xc8     4     2    two-key: float round trip, channel B
//    -0xc0     4     2    one-key: float round trip, channel R
//    -0xb8     4     2    one-key: float round trip, channel G
//    -0xb0     4     2    one-key: float round trip, channel B
//    -0xac     4     4    two-key: second-key blend accumulator [0]
//    -0xa8     4     4    two-key: second-key blend accumulator [1]
//    -0xa4     4     4    two-key: second-key blend accumulator [2]
//    -0xa0     4     4    two-key: second-key blend accumulator [3]
//    -0x9c     4     2    two-key: int32 conversion result, R
//    -0x98     4     2    two-key: int32 conversion result, G
//    -0x94     4     2    two-key: int32 conversion result, B
//    -0x90     4     2    one-key: int32 conversion result, R
//    -0x8c     4     2    one-key: int32 conversion result, G
//    -0x88     4     2    one-key: int32 conversion result, B
//    -0x84     4     0    UNUSED
//    -0x80     8     4    one-key: light accumulator R  (QWORD spill)
//    -0x78     8     4    one-key: light accumulator G  (QWORD spill)
//    -0x70     8     4    one-key: light accumulator B  (QWORD spill)
//    -0x68     8     2    one-key: fDistSqr             (QWORD spill)
//    -0x60     4     3    argument 4, pfnTexCoord
//    -0x5c     4     4    argument 5, pMatrices
//    -0x58     4     7    the vertex down-counter (must be in memory:
//                         it is live across the indirect call)
//    -0x54     4     0    UNUSED
//    -0x50     8     4    one-key: colour channel B     (QWORD spill)
//    -0x48     8     4    one-key: colour channel G     (QWORD spill)
//    -0x48     4     2      ...AND, in the two-key arm only,
//                           pSet2->m_pVerts04 (disjoint live range)
//    -0x40     4     6    `this`, parked across the blend loops because
//                         %esi is spent on pMatrices inside them
//    -0x3c     4     0    UNUSED
//    -0x38     8     4    two-key: light accumulator R  (QWORD spill)
//    -0x30     8     4    two-key: light accumulator G  (QWORD spill)
//    -0x28     8     4    two-key: light accumulator B  (QWORD spill)
//    -0x20     8     2    two-key: fDistSqr             (QWORD spill)
//    -0x18     8     4    two-key: colour channel B     (QWORD spill)
//    -0x10     8     4    two-key: colour channel G     (QWORD spill)
//    -0x10     4     2      ...AND, earlier in the same arm,
//                           m_nIndex06 * 0x20 (disjoint live range)
//    -0x08..-0x01   8     0    UNUSED
//
//  THE ARITHMETIC, and it closes to the byte:
//
//      12   ebx/esi/edi homes            -0xf4 -0xf0 -0xec
//      16   unused                       -0xe8 .. -0xd9
//      44   six float round-trip temps at 8-byte stride, -0xd8..-0xad
//           (24 used + 20 in the five 4-byte holes between them)
//      16   two-key blend accumulators   -0xac -0xa8 -0xa4 -0xa0
//      24   six int32 conversion results -0x9c .. -0x85
//       4   unused                       -0x84
//      32   one-key light R/G/B + fDistSqr  -0x80 -0x78 -0x70 -0x68
//      12   pfnTexCoord, pMatrices, counter  -0x60 -0x5c -0x58
//       4   unused                       -0x54
//      16   one-key colour B and G       -0x50 -0x48
//       4   `this`                       -0x40
//       4   unused                       -0x3c
//      32   two-key light R/G/B + fDistSqr  -0x38 -0x30 -0x28 -0x20
//      16   two-key colour B and G       -0x18 -0x10
//       8   unused                       -0x08 .. -0x01
//     ---
//     244 = 0xf4    exactly.
//
//  WHAT THE FRAME PROVES, independently of any call:
//
//  (a) THE TWO ARMS ARE SEPARATE CODE, NOT ONE SHARED TAIL.  Six int32
//      slots, six float round-trip slots, two fDistSqr slots, six QWORD
//      light accumulators and four QWORD colour spills -- everything is
//      DOUBLED, and the two halves never share a slot except where the
//      live ranges are provably disjoint (-0x48 and -0x10).  A
//      reconstruction that factored the lighting into one helper called
//      from both arms would halve this frame.  It is not a helper: it is
//      the same code written twice, which is what /Qip inlining of a
//      static helper ALSO produces -- so this fixes the CODE shape, and
//      section 8 records the one thing it cannot fix.
//
//  (b) THE SECOND KEY'S BLEND TARGET IS FOUR FLOATS, NOT A VECTOR OF
//      THREE.  -0xac..-0xa0 is sixteen contiguous ascending 4-byte slots,
//      all four written by the loop and all four consumed by the LERP.  A
//      three-float target plus a separate w would not be contiguous.
//
//  (c) THERE IS NO VERTEX BUFFER AND NO CLIP ARRAY IN THIS FUNCTION.
//      244 bytes of frame with 188 accounted for leaves no room for one,
//      which is what separates this function from the clipper family
//      (0x100051a0's frame is 4,360 bytes).  Both the source and the
//      destination vertices are the caller's memory, walked in place.
//
//  (d) THE COUNTER IS IN MEMORY AND THE BLEND COUNTERS ARE NOT.  -0x58 is
//      loaded and stored every iteration (7 references) because it is
//      live across `call *%eax`; the three blend counters and the two
//      light-loop bounds never touch the frame.  A counter in
//      memory is one variable spanning a call, one in a register is not.
//      So there is exactly ONE loop in each arm that contains the
//      callback, and the blend loops are nested strictly inside it.
//
//  (e) EVERY x87 FRAME SLOT IS A COMPILER SPILL, NOT A `double` IN THE
//      SOURCE.  The twelve QWORD slots hold values that are `float` on
//      both sides of the spill: -0x80 etc. are written `fstp QWORD`,
//      re-read `fadd QWORD`, and their final consumer is `fmul
//      0x664(%esi)` against a 4-byte member; -0x68's only consumer is
//      `fmul QWORD` against a 4-byte member too.  icl is built without
//      /Op, so an unrounded float temporary spills at 64 bits.  The
//      counter-example is in the same frame: -0xac..-0xa0, which the
//      source really does round every iteration, are DWORD on both sides.
//      So a reconstruction must NOT declare any `double` here.
//
// ----------------------------------------------------------------------- //
//  6.  INSTRUMENT 3 -- THE LITERAL POOL AS A STATEMENT-ORDER WITNESS
// ----------------------------------------------------------------------- //
//
//  THE RUN IS BOUNDED ON BOTH SIDES BY THE NEIGHBOURS' OWN LOADS.  Every
//  reference to 0x100a42xx / 0x100a43xx in the whole image was collected
//  (grep of `ds:0x100a4...` and `$0x100a4...` over all of .text):
//
//      0x100a4278  loaded at 0x10008eea   \  the PREVIOUS function
//      0x100a427c  loaded at 0x10008f1c   /  (0x10008e10, our same-TU
//                                            caller) -- its last two
//      0x100a4280 .. 0x100a4304           OUR RUN, 34 slots
//      0x100a4308  0x3e4ccccd = 0.2f, folded to an IMMEDIATE at
//                  0x1000986b in the NEXT function (0x10009860) -- so the
//                  run ends at 0x100a4304 and not one slot later
//
//  The 34 slots, dumped from the image:
//
//    0x100a4280  00000000  0.0f    DEAD  \
//    0x100a4284  00000000  0.0f    DEAD   |
//    0x100a4288  00000000  0.0f    DEAD   |  EIGHT dead 0.0f
//    0x100a428c  00000000  0.0f    DEAD   |
//    0x100a4290  00000000  0.0f    DEAD   |
//    0x100a4294  00000000  0.0f    DEAD   |
//    0x100a4298  00000000  0.0f    DEAD   |
//    0x100a429c  00000000  0.0f    DEAD  /
//    0x100a42a0  3f800000  1.0f    DEAD  <- the dead twin of the next
//    0x100a42a4  3f800000  1.0f    LIVE  0x100095c0  fdivr  1.0f / w
//    0x100a42a8  00000000  0.0f    LIVE  0x100095ce  the sun dot test
//    0x100a42ac  00000000  0.0f    LIVE  0x10009696  the light dot test
//    0x100a42b0  3f800000  1.0f    LIVE  0x100096d8  1.0f - d2*invr2
//    0x100a42b4  437f0000  255.0f  LIVE  0x10009729  clamp R, compare
//    0x100a42b8  437f0000  255.0f  LIVE  0x1000975e  clamp R, assign
//    0x100a42bc  437f0000  255.0f  LIVE  0x10009767  clamp G, compare
//    0x100a42c0  437f0000  255.0f  LIVE  0x10009772  clamp G, assign
//    0x100a42c4  437f0000  255.0f  LIVE  0x1000977e  clamp B, compare
//    0x100a42c8  437f0000  255.0f  LIVE  0x10009789  clamp B, assign
//    0x100a42cc  00000000  0.0f    DEAD  \
//    0x100a42d0  00000000  0.0f    DEAD   |  FOUR dead 0.0f
//    0x100a42d4  00000000  0.0f    DEAD   |
//    0x100a42d8  00000000  0.0f    DEAD  /
//    0x100a42dc  3f800000  1.0f    DEAD  <- the dead twin of the next
//    0x100a42e0  3f800000  1.0f    LIVE  0x10009108  fld then fdiv, 1.0f/w
//    0x100a42e4  00000000  0.0f    LIVE  0x10009119  the sun dot test
//    0x100a42e8  00000000  0.0f    LIVE  0x100091d8  the light dot test
//    0x100a42ec  3f800000  1.0f    LIVE  0x1000921a  1.0f - d2*invr2
//    0x100a42f0  437f0000  255.0f  LIVE  0x1000926b  clamp R, compare
//    0x100a42f4  437f0000  255.0f  LIVE  0x100092a0  clamp R, assign
//    0x100a42f8  437f0000  255.0f  LIVE  0x100092a9  clamp G, compare
//    0x100a42fc  437f0000  255.0f  LIVE  0x100092b4  clamp G, assign
//    0x100a4300  437f0000  255.0f  LIVE  0x100092c0  clamp B, compare
//    0x100a4304  437f0000  255.0f  LIVE  0x100092cb  clamp B, assign
//
//  Twenty live, fourteen dead, and EVERY ONE OF THE FOURTEEN IS EXPLAINED.
//  Nothing here is recorded as unexplained.
//
//  (i)  THE TWO-KEY ARM COMES FIRST IN THE SOURCE.  Both blocks of live
//       slots are strictly ascending in code order, and the two-key arm's
//       block (0x2a0..0x2c8) precedes the one-key arm's (0x2dc..0x304)
//       even though the two-key arm's CODE is at the higher address.  icl
//       emits literals in source order, so the source is
//
//           if (m_n450) { ...two-key... } else { ...one-key... }
//
//       and the `jne 0x10009360` sinks the two-key arm.  Nothing else in
//       the function recovers this: the arms make one indirect call each,
//       so the call check is blind to it (section 3), and the frame is
//       symmetric.
//
//  (ii) THE DEAD 1.0f RULE CONFIRMS EXACTLY TWO DIVISIONS.  0x100a42a0
//       and 0x100a42dc are each an unreferenced 1.0f sitting immediately
//       before a live one that a divide loads.  A compiler probe fixes
//       this as icl's per-division pair, so counting them counts
//       divisions: TWO, one per arm.  The census agrees independently
//       (`fdiv` 1 + `fdivr` 1 = 2) and no third reciprocal exists
//       anywhere in the function.  In particular there is NO
//       1.0f/255.0f, so the colour conversion really is a raw 0..255
//       scale-and-clamp and not a normalise-then-rescale.
//
//  (iii) THE TWELVE DEAD 0.0f ARE THE TWELVE ZERO ASSIGNMENTS, AND THEY
//       PROVE THEY ARE TWELVE SEPARATE STATEMENTS.  Each arm opens by
//       clearing its blend targets with integer stores off a `xor
//       %eax,%eax` -- 0x1000901d..0x1000902b (four dwords into the
//       destination vertex), 0x10009375..0x10009385 (the same four) and
//       0x1000946b..0x10009482 (the four second-key accumulators).  That
//       is 4 + 4 + 4 = twelve stores, and the pool carries EIGHT dead
//       0.0f ahead of the two-key arm's divide plus FOUR between the two
//       arms:
//           8 = the two-key arm's 4 + 4, both groups ahead of its 1.0f
//           4 = the one-key arm's 4, ahead of its 1.0f
//       exactly the source order the code has.  Two consequences:
//         * the assignments are `= 0.0f`, not `= 0`.  An integer zero
//           emits no pool slot at all, so the pool is what says these are
//           float assignments whose stores icl then folded to `mov`;
//         * they are TWELVE separate statements, not three chained
//           `a = b = c = d = 0.0f`.  A chain occupies ONE slot (0x10019d64
//           has both spellings and big19d64.cpp records the chained one at
//           0x1001b7fb), so a chain here would leave nine slots
//           unexplained.
//
//  (iv) SIX 255.0f PER ARM PROVE THREE `if` CLAMPS, NOT A TERNARY AND NOT
//       AN LTMIN.  Each channel spends TWO literals -- one on the
//       compare, one on the replacement -- which is `if (c > 255.0f) c =
//       255.0f;` written out three times.  A conditional expression
//       (`c = c > 255.0f ? 255.0f : c`) spends two as well but puts BOTH
//       arms' value through st0 and duplicates one store into each arm;
//       the shipped G and B clamps have NO store on the fall-through path
//       (0x10009770 and 0x10009787 jump straight past), so the store is
//       inside a statement-level `if`.  And the ORDER is R, G, B in both
//       arms, which is the order of the three byte stores too but is
//       recovered here independently of the scheduler.
//
//  (v)  THE ORDER WITHIN EACH ARM IS: the four zeroes, [blend], the
//       reciprocal, the sun dot test, the light dot test, the
//       attenuation, then the three clamps.  Read straight off the
//       ascending addresses.  It is the order this reconstruction is
//       written in.
//
// ----------------------------------------------------------------------- //
//  7.  INSTRUMENT 4 -- EXPRESSION-BY-EXPRESSION CORRESPONDENCE
// ----------------------------------------------------------------------- //
//
//  This function is its own best sibling: R13..R17 are the same source as
//  R4..R8.  The two blocks were diffed instruction by instruction, and
//  the correspondence is EXACT -- same mnemonics, same order, same
//  operands modulo the frame slot and the pool slot:
//
//     R4  9134..9165  14 instr  ==  R13 95e9..961a  14 instr
//     R5  9166..91a3  13        ==  R14 961b..9658  13
//     R6b 91c2..924f  53        ==  R15b 9680..970d 53
//     R6c 9250..925e   6        ==  R15c 970e..971c  6
//     R7  925f..932a  50        ==  R16 971d..97e8  50
//     R8  932b..9353  13        ~=  R17 97e9..980c  12  (R8 ends in the
//                                    `jmp 0x1000980d` that R17 reaches by
//                                    falling through -- the one and only
//                                    difference in 149 instructions)
//     R19 981e..983c   9        ~=  R20 9840..985e   8  (the 3-byte
//                                    lea-nop pad, again the only
//                                    difference)
//
//  Operand-for-operand, with the offsets read off the bytes:
//
//     step                       one-key            two-key
//     -------------------------  -----------------  -----------------
//     sun dot, z term            9134 ebx+0x1c      95e9 ebx+0x1c
//                                     esi+0x67c          esi+0x67c
//     sun dot, y term            913d ebx+0x18      95f2 ebx+0x18
//                                     esi+0x678          esi+0x678
//     sun dot, x term            9148 ebx+0x14      95fd ebx+0x14
//                                     esi+0x674          esi+0x674
//     * m_f680                   9153 esi+0x680     9608 esi+0x680
//     compare against            9119 0x100a42e4    95ce 0x100a42a8
//     LERP base / target x       9166 esi+0x668      961b esi+0x668
//                                916c esi+0x684      9621 esi+0x684
//     LERP y                     917a esi+0x66c      962f esi+0x66c
//                                9180 esi+0x688      9635 esi+0x688
//     LERP z                     918e esi+0x670      9643 esi+0x670
//                                9194 esi+0x68c      9649 esi+0x68c
//     light count                91a4 esi+0x2fc     9659 esi+0x2fc
//     light stride               91bb imul 0x2c     9670 imul 0x2c
//     light vDir dot             91c2 edx+0x50/4c/48 9680 edx+0x50/4c/48
//     light vPos delta           91e5 edx+0x44/40/3c 96a3 edx+0x44/40/3c
//     m_fRadiusSqr18             9207 edx+0x54      96c5 edx+0x54
//     m_fInvRadiusSqr1c          9214 edx+0x58      96d2 edx+0x58
//     m_fR20/m_fG24/m_fB28       9222 edx+0x5c      96e0 edx+0x5c
//                                922a edx+0x60      96e8 edx+0x60
//                                923a edx+0x64      96f8 edx+0x64
//     colour scale               925f esi+0x664     971d esi+0x664
//                                9278 esi+0x65c     9736 esi+0x65c
//                                9286 esi+0x660     9744 esi+0x660
//     colour bias                9265 esi+0x658     9723 esi+0x658
//                                927e esi+0x650     973c esi+0x650
//                                928c esi+0x654     974a esi+0x654
//     byte stores                92ef edi+0x12      97ad edi+0x12
//                                930d edi+0x11      97cb edi+0x11
//                                9328 edi+0x10      97e6 edi+0x10
//     alpha                      932b esi+0x6a8     97e9 esi+0x6a8
//                                9333 edi+0x13      97f1 edi+0x13
//     callback                   9336 push %edi     97f4 push %edi
//                                9337 %edx = %ebx   97f5 %edx = %ebx
//                                933c call *%eax    97fa call *%eax
//
//  Not one offset differs.  So any error in the lighting half of this
//  reconstruction is an error made twice in the same place, and any
//  mis-read offset would have had to be mis-read identically in two
//  independently-scheduled 50-instruction blocks.
//
//  THE SECOND SIBLING is the class itself.  Every `this` offset the
//  function touches was already decoded and named by other files, and
//  they agree with this one:
//
//    +0x2fc  m_nLights2fc      big19d64.cpp:511, big17c14.cpp:638,
//                              big4b720.cpp:483 -- and big19d64 proves
//                              the BOUND is 16, because it clamps the
//                              count at 16 and 0x3c + 16*0x2c = 0x2fc,
//                              i.e. the table ends exactly where the
//                              count begins.  Our loop's `imul $0x2c`
//                              and the fields it reads (+0x3c..+0x64 off
//                              a walking pointer, i.e. +0x00..+0x28 of a
//                              0x2c record) are the SAME table.
//    +0x3c   m_Lights3c[16]    big19d64.cpp:490 `ModelLight19d64`
//                              { LTVec vPos00; LTVec vDir0c;
//                                float fRadiusSqr18, fInvRadiusSqr1c,
//                                fR20, fG24, fB28; }  -- seven fields,
//                              0x2c bytes, and this function reads all
//                              seven at exactly those offsets.
//    +0x42c  the tex-coord fn  installed at 0x10043eb6 / 0x10044f8b
//    +0x450  m_n450            big17c14.cpp:649, big4b720.cpp:490
//    +0x454  the LERP factor   unnamed in big17c14 (inside m_pad_454)
//    +0x62c  m_n62c            big17c14.cpp:662 -- the destination
//                              vertex base our caller indexes by 0x20
//    +0x650  m_v650            big19d64.cpp:520, written from
//                              g_p1008beb0->m_v1c
//    +0x65c/660/664  m_fR65c/m_fG660/m_fB664  big19d64.cpp:521-523,
//                              written from the object's byte colour and
//                              then scaled by 1/255 -- which is why THIS
//                              function multiplies by them and then
//                              clamps at 255 rather than at 1
//    +0x668  m_v668            big19d64.cpp:524
//    +0x674  m_vLightDir674    big19d64.cpp:525, the global light
//                              direction rotated into MODEL space and
//                              negated (big19d64.cpp:664-671)
//    +0x680  m_f680            big19d64.cpp:526, a cached intensity
//                              (0x1001bae4's return, or 0.0f)
//    +0x684  m_vAmbient684     big19d64.cpp:527, defaulted to
//                              (255,255,255) at big19d64.cpp:632-634 --
//                              which is what makes it the value the sunk
//                              "ambient only" arms load
//    +0x6a8  m_bAlpha6a8       big17c14.cpp:667, big43e10.cpp:598
//    +0x000  m_pModel00, and +0x74 of that is the matrix palette, from
//            our caller at 0x10008e32 and independently from
//            0x100188bf/0x100188cb (`mov 0xf4(%eax),%edx ; imul $0x40`)
//
//  So the lighting model is not inferred from shape: it is the table
//  0x10019d64 FILLS, read by the function that consumes it, with the
//  producer's field names.
//
// ----------------------------------------------------------------------- //
//  8.  HOW THE x87 OPERAND ORDER WAS SETTLED
// ----------------------------------------------------------------------- //
//
//  FIRST, the hazard cannot bite this function at all, and that was
//  checked before anything else.  The DE/DC/D8 register forms that objdump
//  prints with swapped operands are the SUBTRACT and DIVIDE ones -- reg
//  field 4/5 and 6/7, i.e. the E0..FF quarter of the modrm byte.  A grep
//  of the function's 661 encodings finds:
//
//      de c1  de ca  de cb          (faddp, fmulp, fmulp)
//      dc ca  dc cb                 (fmul st(i),st)
//      d8 c8  d8 c9  d8 ca          (fmul st,st(i))
//      d8 d9  d8 da                 (fcomp st(i))
//      matches for `(de|dc|d8) [ef]?`:  ZERO
//
//  Every register-form x87 operation in the function is FADD, FMUL or
//  FCOM/FCOMP.  The first two are commutative, so no swap can change
//  their meaning; the third produces flags, and its sense was settled by
//  execution below.  There is no `fsubp`, `fsubrp`, `fdivp` or `fdivrp`
//  anywhere in the range.  All thirteen `fsub`, five `fsubr`, one `fdiv`
//  and one `fdivr` are MEMORY forms, where the reg field is unambiguous.
//
//  SECOND, the memory forms and the compare sense were verified BY
//  EXECUTING THE EXACT OPCODE BYTES, not by reading a table.  A 64-bit
//  test program emitted the shipped bytes with `.byte`, using [rax] so
//  that the modrm REG field -- the only field that selects the operation
//  -- is identical to the shipped disp32 forms:
//
//      d8 /5  fsubr [3.0] with st0=10.0  ->  -7      (3 - 10)   CONFIRMED
//      d8 /4  fsub  [3.0] with st0=10.0  ->   7      (10 - 3)   CONFIRMED
//      d8 /7  fdivr [3.0] with st0=12.0  ->   0.25   (3 / 12)   CONFIRMED
//      d8 /6  fdiv  [3.0] with st0=12.0  ->   4      (12 / 3)   CONFIRMED
//      de c1  faddp st(1),st  st1=2 st0=7      ->  9            CONFIRMED
//      de ca  fmulp st(2),st  st2=5 st0=3      ->  15 at st1    CONFIRMED
//      dc cb  fmul  st(3),st  st3=4 st0=6      ->  24 at st3    CONFIRMED
//      d8 d9  fcomp st(1) + fnstsw + sahf + jae:
//                 st0=5 st1=3  -> jae TAKEN
//                 st0=3 st1=5  -> jae NOT taken
//             i.e. `jae` is taken iff st0 >= st(i)             CONFIRMED
//
//  `objdump -M intel` agrees with all of it, and the AT&T listing agrees
//  too for every form present here (`fsubrs 0x100a42ec`, `fdivrs
//  0x100a42a4`, `fsubs (%edi)`, `fdivs 0xc(%edi)`).  So both listings
//  could be used, and both were.
//
//  THIRD, the COMPARE OPERAND ORDER is calibrated inside the function
//  rather than assumed, because the constant is sometimes hoisted into
//  st0 and sometimes left in memory:
//
//    * The G and B clamps are unambiguous: `fld <channel> ; fcomp
//      0x100a42f8` -- the channel is st0, the 255.0f is the memory
//      operand, and `jbe` skips.  Source: `if (c > 255.0f) c = 255.0f;`
//    * The R clamp is the SAME STATEMENT with the 255.0f hoisted:
//      `fxch %st(1) ; fcomp %st(2)` with 255.0f in st0 and the channel in
//      st(2), `jae` skips.  Since the statement is known from G and B,
//      this calibrates the register form: in THIS function, `fcomp st(i)`
//      has st0 holding the RIGHT-hand operand and st(i) the left.
//    * Applying that calibration:
//        - 0x1000915b `fcomp %st(1)` with 0.0f in st0 and the sun dot in
//          st(1), `jae` -> the sunk ambient arm.  Source:
//          `if (fDot > 0.0f)`.
//        - 0x100091de `fld 0x100a42e8 ; fcomp %st(1)`, 0.0f in st0 and
//          the per-light dot in st(1), `jae` -> skip this light.  Source:
//          `if (fDot > 0.0f)`.
//        - 0x1000920a `fld 0x54(%edx) ; fcomp %st(1)`, m_fRadiusSqr18 in
//          st0 and fDistSqr in st(1), `jbe` -> skip.  Source:
//          `if (fDistSqr < pLight->m_fRadiusSqr18)`.
//      All three tests are STRICT and their two spellings are
//      semantically identical, so the calibration decides the spelling
//      and nothing about the behaviour is at risk either way.
//
// ----------------------------------------------------------------------- //
//  9.  REGION MAP -- WHAT EACH REGION DOES AND WHAT PINS IT
// ----------------------------------------------------------------------- //
//
//  R0   8fd0  the /Qip stack entry.  Pinned by the padding above it and by
//             the external caller at 0x1004c3f6.  Its load is dead.
//  R1   8fd4  frame, three register homes, the five arguments, and
//             `mov 0x450(%esi),%ecx ; test %ecx,%ecx ; ... jne 0x10009360`.
//             Pinned: the dispatch member is +0x450 (named in big17c14 and
//             big4b720); the two-key arm is the jump target and the
//             literal pool says it is FIRST in the source.
//  R2a  901d  one-key vertex-loop head: four `= 0.0f` (four dead pool
//             slots), the counter decrement, pBlends/nBlends, and the
//             `je` that skips the blend loop when nBlends == 0.
//  R2b  9050  the blend loop.  Pinned: `imul $0x40` on +0x10 of a 0x14
//             record, four rows of four columns at +0x00/0x10/0x20/0x30 of
//             the matrix, accumulating into +0x00/04/08/0c of the vertex.
//             The `fadd mem` on row 0 only, and the 15/12 `faddp` split
//             against loop 3, are the census rows that prove the preload
//             chain and therefore that all four rows exist.
//  R3   9105  one-key projection.  `fld 1.0f ; fdiv 0xc(%edi)` then three
//             multiplies and `fst 0xc(%edi)`.  Pinned by the dead-1.0f
//             pool pair and by `fst` being non-popping.
//  R4   9134  sun dot test.  Normal FIRST in each product (opposite
//             operand order from the per-light dot -- see the code).
//  R5   9166  sun LERP towards m_v668, three separate assignments in
//             source order x, y, z.
//  R6a  91a4  spill fR/fG/fB, `test`/`jbe` on m_nLights2fc, `imul $0x2c`
//             and `add %esi,%ecx` to form the end bound.
//  R6b  91c2  the per-light body: dot test, distance test, attenuation,
//             three accumulations.  Every offset cross-checked against
//             big19d64.cpp's ModelLight19d64.
//  R6c  9250  reload the three accumulators and restore the st0/st1/st2
//             order the nLights == 0 fall-through leaves -- which is what
//             says the `if (nLights)` guard shares its join with the loop.
//  R7   925f  scale by (m_fR65c, m_fG660, m_fB664), bias by m_v650, three
//             255.0f clamps (six pool slots), three float->int round trips
//             (`fistp`, so /Qrcd), three byte stores at +0x12/+0x11/+0x10.
//  R8   932b  alpha from m_bAlpha6a8 into +0x13, the indirect call, the
//             two `add $0x20` walks, the loop latch, `jmp` to the exit.
//  R9   9354  two lea-nops aligning 0x10009360.
//  R10a 9360  two-key head: pSet2->m_pVerts04 cached at -0x48, the vertex
//             guard, four `= 0.0f`, `imul $0x20` on m_nIndex06.
//  R10b 93b0  two-key blend loop over the FIRST key.  The same shape as
//             R2b instruction for instruction -- R2b is 63 and R10b 65,
//             and the two extra instructions are the `mov -0x10(%ebp),
//             %edx ; mov -0x40(%ebp),%esi` restores at 0x10009465, which
//             fall inside R10b's range where R2b's single restore falls
//             inside R3's.  The x87 rows are identical: fld 19, fmul 16,
//             faddp 15, fadd 1, fxch 3, fstp 4 in both.
//  R11a 946b  zero the four second-key accumulators (four more dead pool
//             slots), fetch pVerts2[m_nIndex06]'s pBlends/nBlends.
//  R11b 94a0  two-key blend loop over the SECOND key, into the four frame
//             slots.  Distinguished from R2b/R10b by exactly the census
//             rows named above: `fadd` 4 not 1, `faddp` 12 not 15.
//  R12  956c  the four-component LERP with m_fLerp454, then the divide.
//             The `fsub` / three `fsubr` split is the tell that all four
//             are one shape.  The fourth component's STORE is dead.
//  R13..R17   the two-key copies of R4..R8.  See section 7.
//  R18  980d  the single epilogue: three register restores, `mov %ebp,%esp
//             ; pop %ebp ; ret $0x14`.
//  R19  981e  SUNK: the one-key sun test's else -- discard the product,
//             load m_vAmbient684's three components, jump back to
//             0x100091a4.
//  R20  9840  SUNK: the two-key copy, jumping back to 0x10009659.
//
//  NOTHING IN THE RANGE IS UNDECODED.  There is no `// UNDECODED:` banner
//  in this file: all 661 instructions are attributed to a statement, and
//  the region table in section 4 accounts for every one of the 2,191
//  bytes including the seven lea-nops.
//
// ----------------------------------------------------------------------- //
//  10.  WHAT COULD NOT BE PINNED, AND THE REMAINING COIN-FLIPS
// ----------------------------------------------------------------------- //
//
//  (1) THE CALLBACK'S REGISTER ARGUMENT.  The call passes the source
//      vertex in %edx with no stack home and the destination as the only
//      stack dword (`ret $0x4` in all three candidate bodies).  That is
//      icl's /Qip register convention on a __thiscall, and the rule is
//      to decode such a call as an ordinary call whose argument arrives
//      in a register -- which is what the code below does.  What is NOT
//      pinned is how the ORIGINAL source declared it, because /Qip is a
//      per-TU flag and this pointer crosses TUs (installed at 0x10043eb6
//      and 0x10044f8b, called here).  Does not move a call: the call is
//      indirect and unresolvable either way.
//
//  (2) THE LOOP SPELLING.  `while (n--)` and `for (i = 0; i < n; i++)`
//      produce the same rotated guard + `dec`/`jne` for every one of the
//      five counted loops here.  The vertex loops are written `while
//      (nVerts--)` below because the shipped counter SHARES the slot the
//      initial value was stored into (-0x58, seven references) rather
//      than getting a fresh induction variable; the blend loops keep
//      their counter in a register and so carry no such evidence.  Both
//      spellings; neither moves a call.
//
//  (3) THE ORDER OF THE FOUR PROJECTION ASSIGNMENTS.  `pOut->m_fRHW =
//      fInvW` is emitted between the z and y multiplies, which is pure
//      FPU-stack scheduling; the source could have it first or last.
//      Written last below.  Does not move a call.
//
//  (4) THE FOURTH LERP'S DEAD STORE (region R12).  Written below as an
//      assignment to `pOut->m_fRHW` whose store icl eliminated because
//      the reciprocal overwrites it.  The alternative -- a named `float
//      fW` local that is never stored -- is byte-identical.  Does not
//      move a call.
//
//  (5) `imul $0x40` VERSUS `shl $6`.  The matrix index is scaled with
//      `imul`, three bytes, where a subscript on a 64-byte struct usually
//      gives `shl`.  0x100188cb does the same on the same array, so this
//      is not a source distinction that this decode can read.  Written as
//      a subscript below.
//
//  (6) THE FIELD AT +0x14 OF THE DESTINATION VERTEX -- the specular
//      D3DCOLOR -- is NEVER WRITTEN by this function, in either arm.  It
//      is named from the FVF (0x1c4, 32 bytes: xyzrhw + diffuse +
//      specular + one uv pair, exactly as big0ae20.cpp:925 records) and
//      from nothing else.  Whoever fills it, fills it elsewhere.
//
//  (7) THE OTHER FIELDS OF ModelVertSet.  Only +0x04 and +0x08 are read.
//      The caller at 0x10008e80 forms these pointers with
//      `lea (%ecx,%ecx,8)` after `lea -0x4(,%eax,4)`, i.e. a stride of
//      36 bytes, so the record is 0x24 bytes and 0x0c..0x23 is unread
//      here.  Padded, not invented.
//
//  (8) THE 16-BYTE HOLE AT -0xe8..-0xd9 and the four smaller ones.  The
//      frame arithmetic closes with them counted as unused; no
//      instruction in the range touches those bytes.  They are icl's
//      allocator, not a local this decode failed to find -- but they are
//      the one part of section 5 that is an absence rather than a
//      positive reading, so they are listed here.
//
// ======================================================================= //
//  THE RECONSTRUCTION
// ======================================================================= //

typedef unsigned char  uint8;
typedef unsigned short uint16;
typedef unsigned int   uint32;
typedef signed int     int32;


// ----------------------------------------------------------------------- //
//  LTVector / LTMatrix, in the shape the other d3d files use.
// ----------------------------------------------------------------------- //
struct LTVec
{
	float x, y, z;
};

struct LTMat
{
	float m[4][4];
};


// ----------------------------------------------------------------------- //
//  The DESTINATION vertex: FVF 0x1c4, 32 bytes.  Identical to
//  big16324.cpp:210's `RVert`, reused verbatim.  This function writes
//  +0x00..+0x0c and +0x10..+0x13 and hands +0x18/+0x1c to the callback;
//  +0x14..+0x17 it never touches.
// ----------------------------------------------------------------------- //
struct RVert
{
	LTVec  m_vPos;                                       // +0x00
	float  m_fRHW;                                       // +0x0c
	uint8  m_b;                                          // +0x10
	uint8  m_g;                                          // +0x11
	uint8  m_r;                                          // +0x12
	uint8  m_a;                                          // +0x13
	uint8  m_sb;                                         // +0x14  untouched
	uint8  m_sg;                                         // +0x15  untouched
	uint8  m_sr;                                         // +0x16  untouched
	uint8  m_sa;                                         // +0x17  untouched
	float  m_fU;                                         // +0x18  callback
	float  m_fV;                                         // +0x1c  callback
};


// ----------------------------------------------------------------------- //
//  One matrix-palette weight record, 0x14 bytes.  Read off the stride
//  (`add $0x14,%eax` at 0x100090f8) and the five field offsets: four
//  floats multiplied against the four COLUMNS of a matrix row, and a
//  dword scaled by 0x40 to index the palette.
//
//  The four floats are the vertex's position in the bone's space already
//  multiplied by the bone's weight, in homogeneous form -- (w*x, w*y,
//  w*z, w) -- which is the only reading under which summing M[i] * v[i]
//  over the records produces a single blended homogeneous position, and
//  the only reading under which the fourth column of every row (the
//  translation column) participates.  That is an inference from the
//  arithmetic, not a name read off the bytes.
// ----------------------------------------------------------------------- //
struct ModelBlend
{
	float   m_fW0;                                      // +0x00
	float   m_fW1;                                      // +0x04
	float   m_fW2;                                      // +0x08
	float   m_fW3;                                      // +0x0c
	uint32  m_nMatrix10;                                // +0x10
};


// ----------------------------------------------------------------------- //
//  One SOURCE vertex, 0x20 bytes (`add $0x20,%ebx`, and the second key is
//  indexed with `imul $0x20` at 0x10009382).
//
//    +0x00  the weight-record list          (mov (%ebx),%eax)
//    +0x04  how many                        (movzwl 0x4(%ebx))
//    +0x06  index of the matching vertex in the OTHER animation key
//                                           (movzwl 0x6(%ebx), x0x20)
//    +0x08  model-space position, used for the point-light distance
//    +0x14  model-space normal, used for both dot products and handed to
//           the texture-coordinate callback (0x10003ae0 reads exactly
//           these three offsets off its %edx argument)
// ----------------------------------------------------------------------- //
struct ModelVert
{
	ModelBlend* m_pBlends00;                            // +0x00
	uint16      m_nBlends04;                            // +0x04
	uint16      m_nIndex06;                             // +0x06
	LTVec       m_vPos08;                               // +0x08
	LTVec       m_vNormal14;                            // +0x14
};


// ----------------------------------------------------------------------- //
//  A source vertex RUN.  0x24 bytes from the caller's stride; only +0x04
//  and +0x08 are read here.
// ----------------------------------------------------------------------- //
struct ModelVertSet
{
	uint32      m_n00;                                  // +0x00  unread
	ModelVert*  m_pVerts04;                             // +0x04
	uint32      m_nVerts08;                             // +0x08
	uint8       m_pad_0c[0x24 - 0x0c];                  // +0x0c  unread
};


// ----------------------------------------------------------------------- //
//  One entry of CD3DModelDraw's 16-slot model-space light table at
//  +0x3c, stride 0x2c.  This is big19d64.cpp:490's `ModelLight19d64`
//  unchanged -- that file WRITES all seven fields, this one READS all
//  seven, and every offset agrees.
// ----------------------------------------------------------------------- //
struct ModelLight
{
	LTVec       m_vPos00;                               // +0x00
	LTVec       m_vDir0c;                               // +0x0c
	float       m_fRadiusSqr18;                         // +0x18
	float       m_fInvRadiusSqr1c;                      // +0x1c
	float       m_fR20;                                 // +0x20
	float       m_fG24;                                 // +0x24
	float       m_fB28;                                 // +0x28
};


// ----------------------------------------------------------------------- //
//  CD3DModelDraw -- the global object at 0x1008b090.  Only the members
//  THIS function touches are named; everything between them is padding
//  written as an arithmetic difference so it cannot drift.  Each name and
//  offset is the one an already-decoded file uses (section 7).
// ----------------------------------------------------------------------- //
struct D3DModel
{
	uint8       m_pad_00[0x74 - 0x00];                  // +0x00
	LTMat*      m_pMatrices74;                          // +0x74
};

class CD3DModelDraw
{
public:
	//  The texture-coordinate slot at +0x42c, as the type this function's
	//  fourth PARAMETER has.  `this` in %ecx, the source vertex in %edx,
	//  the destination vertex as the single stack dword, callee pops 4 --
	//  see section 3 and coin-flip (1).  Its three known values are
	//  0x10003ad0 (do nothing), 0x10003ae0 and 0x10003b50; all three write
	//  only m_fU/m_fV.  The typedef is nested so that the pointer's
	//  representation is fixed inside the class body (VC6/icl need the
	//  inheritance known, and it is, here).
	typedef void (CD3DModelDraw::*PFNTEXCOORD)(ModelVert* pVert,
	                                           RVert* pOut);

	//  How the slot is actually CALLED.  The three bodies (0x10003ad0,
	//  0x10003ae0, 0x10003b50; w2_10003a10.cpp) take `this` in %ecx, the
	//  source vertex in %edx and the destination as the one stack dword,
	//  `ret $0x4` -- __fastcall, which is how w2_10003a10.cpp declares
	//  them.  Called through the member-pointer type above, icl passes
	//  BOTH arguments on the stack with nothing in %edx: the callee then
	//  read the normal through a stale %edx, wrote U/V into the SOURCE
	//  vertex, and left 4 bytes on the stack (a run-time corruption of
	//  CD3DModelDraw+0x00 and a page fault in 0x10008e10 on the first
	//  model drawn).  The parameter keeps its member-pointer type so this
	//  function's name is unchanged (w2_10008da0.cpp, link/aliases.rsp);
	//  the call goes through this type.
	typedef void (__fastcall* PFNTEXCOORDCALL)(CD3DModelDraw* pThis,
	                                           ModelVert* pVert,
	                                           RVert* pOut);

	void        sub_10008fd0(ModelVertSet* pSet1, ModelVertSet* pSet2,
	                         RVert* pOut, PFNTEXCOORD pfnTexCoord,
	                         LTMat* pMatrices);          // 0x10008fd0

	D3DModel*   m_pModel00;                             // +0x000
	uint8       m_pad_004[0x03c - 0x004];               // +0x004
	ModelLight  m_Lights3c[16];                         // +0x03c
	uint32      m_nLights2fc;                           // +0x2fc
	uint8       m_pad_300[0x42c - 0x300];               // +0x300
	//  +0x42c holds a PFNTEXCOORD; declared `void*` in big17c14.cpp's
	//  style because THIS function never reads it -- both callers load it
	//  and hand it over as argument 4 (0x10008f55, 0x1004c3e6).
	void*       m_pfnTexCoord42c;                       // +0x42c
	uint8       m_pad_430[0x434 - 0x430];               // +0x430
	//  +0x434 is the OTHER pointer the installer at 0x10044f70 writes
	//  (0x10044f85, from its first argument); unread here, named only so
	//  the padding either side is honest about where it is.
	void*       m_pfn434;                               // +0x434
	uint8       m_pad_438[0x450 - 0x438];               // +0x438
	uint32      m_n450;                                 // +0x450
	float       m_fLerp454;                             // +0x454
	uint8       m_pad_458[0x650 - 0x458];               // +0x458
	LTVec       m_v650;                                 // +0x650
	float       m_fR65c;                                // +0x65c
	float       m_fG660;                                // +0x660
	float       m_fB664;                                // +0x664
	LTVec       m_v668;                                 // +0x668
	LTVec       m_vLightDir674;                         // +0x674
	float       m_f680;                                 // +0x680
	LTVec       m_vAmbient684;                          // +0x684
	uint8       m_pad_690[0x6a8 - 0x690];               // +0x690
	uint8       m_bAlpha6a8;                            // +0x6a8
};


// ======================================================================= //
//  0x10008fd0 -- __thiscall, five dword arguments, `ret $0x14`, void.
//  The /Qip stack entry at 0x10008fd0 falls into the register entry at
//  0x10008fd4; the same-TU caller 0x10008e10 enters at +4 and the
//  cross-TU caller 0x1004c3f6 at +0.  Building this beside
//  sub_10008e10 under /Qip is what reproduces the pair.
// ======================================================================= //

void CD3DModelDraw::sub_10008fd0(ModelVertSet* pSet1, ModelVertSet* pSet2,
                                 RVert* pOut, PFNTEXCOORD pfnTexCoord,
                                 LTMat* pMatrices)
{
	ModelVert*  pVert  = pSet1->m_pVerts04;         // 0x10009003
	uint32      nVerts = pSet1->m_nVerts08;         // 0x10009000, -0x58
	//  The callback, copied to -0x60 at 0x10008ff9 and called from there.
	PFNTEXCOORDCALL pfnCall = *(PFNTEXCOORDCALL*)&pfnTexCoord;

	// -- 0x10008ff0 / 0x1000900f ---------------------------------------- //
	//  `mov 0x450(%esi),%ecx ; test %ecx,%ecx ; jne 0x10009360`.  The
	//  literal pool (section 6(i)) puts the TWO-KEY arm first in the
	//  source, so the `if` is written positively and its body is the one
	//  the compiler sinks.
	if (m_n450)
	{
		// -- 0x10009360 ------------------------------------------------- //
		//  The second animation key's vertex run.  Hoisted out of the
		//  loop into -0x48 before the guard, which is why the two loads
		//  (`mov 0xc(%ebp),%edx ; mov 0x4(%edx),%edx`) sit above the
		//  `je 0x1000980d`.
		ModelVert* pVerts2 = pSet2->m_pVerts04;

		// -- 0x10009363 / 0x1000936b ------------------------------------ //
		while (nVerts--)
		{
			//  Four separate `= 0.0f` assignments.  Four dead 0.0f pool
			//  slots; icl folds all four stores to `mov` off one
			//  `xor %eax,%eax` (0x10009375..0x10009385).
			pOut->m_vPos.x = 0.0f;
			pOut->m_vPos.y = 0.0f;
			pOut->m_vPos.z = 0.0f;
			pOut->m_fRHW   = 0.0f;

			// -- 0x10009371 / 0x10009382 -------------------------------- //
			//  pVert->m_nIndex06 * 0x20 is formed HERE, before the blend
			//  loop, and parked at -0x10 across it.
			ModelVert*  pVert2   = pVerts2 + pVert->m_nIndex06;
			ModelBlend* pBlend   = pVert->m_pBlends00;
			uint32      nBlends  = pVert->m_nBlends04;

			// -- 0x100093b0 .. 0x1000945f ------------------------------- //
			//  KEY 1: out(4) += M[record's bone] * record's weighted
			//  homogeneous position, summed over the records.  Row r of
			//  the matrix is +r*0x10; column c is +c*4.
			while (nBlends--)
			{
				LTMat* pMat = &pMatrices[pBlend->m_nMatrix10];

				pOut->m_vPos.x += pMat->m[0][0] * pBlend->m_fW0
				                + pMat->m[0][1] * pBlend->m_fW1
				                + pMat->m[0][2] * pBlend->m_fW2
				                + pMat->m[0][3] * pBlend->m_fW3;
				pOut->m_vPos.y += pMat->m[1][0] * pBlend->m_fW0
				                + pMat->m[1][1] * pBlend->m_fW1
				                + pMat->m[1][2] * pBlend->m_fW2
				                + pMat->m[1][3] * pBlend->m_fW3;
				pOut->m_vPos.z += pMat->m[2][0] * pBlend->m_fW0
				                + pMat->m[2][1] * pBlend->m_fW1
				                + pMat->m[2][2] * pBlend->m_fW2
				                + pMat->m[2][3] * pBlend->m_fW3;
				pOut->m_fRHW   += pMat->m[3][0] * pBlend->m_fW0
				                + pMat->m[3][1] * pBlend->m_fW1
				                + pMat->m[3][2] * pBlend->m_fW2
				                + pMat->m[3][3] * pBlend->m_fW3;

				pBlend++;                       // 0x10009458  add $0x14
			}

			// -- 0x1000946b .. 0x10009482 ------------------------------- //
			//  Four more separate `= 0.0f` (four more dead pool slots).
			//  These four slots are DWORD on both sides of every spill,
			//  so they are real `float` locals, not compiler temporaries
			//  (section 5(e)).
			float aBlend2[4];

			aBlend2[0] = 0.0f;
			aBlend2[1] = 0.0f;
			aBlend2[2] = 0.0f;
			aBlend2[3] = 0.0f;

			// -- 0x10009488 .. 0x10009563 ------------------------------- //
			//  KEY 2: the same blend through the SAME palette, but for
			//  the matching vertex of the other key and into the four
			//  locals.  Census: `fadd` 4 rather than 1 and `faddp` 12
			//  rather than 15, because four separate slots cannot carry
			//  the preload chain the vertex's four adjacent floats do.
			pBlend  = pVert2->m_pBlends00;
			nBlends = pVert2->m_nBlends04;

			while (nBlends--)
			{
				LTMat* pMat = &pMatrices[pBlend->m_nMatrix10];

				aBlend2[0] += pMat->m[0][0] * pBlend->m_fW0
				            + pMat->m[0][1] * pBlend->m_fW1
				            + pMat->m[0][2] * pBlend->m_fW2
				            + pMat->m[0][3] * pBlend->m_fW3;
				aBlend2[1] += pMat->m[1][0] * pBlend->m_fW0
				            + pMat->m[1][1] * pBlend->m_fW1
				            + pMat->m[1][2] * pBlend->m_fW2
				            + pMat->m[1][3] * pBlend->m_fW3;
				aBlend2[2] += pMat->m[2][0] * pBlend->m_fW0
				            + pMat->m[2][1] * pBlend->m_fW1
				            + pMat->m[2][2] * pBlend->m_fW2
				            + pMat->m[2][3] * pBlend->m_fW3;
				aBlend2[3] += pMat->m[3][0] * pBlend->m_fW0
				            + pMat->m[3][1] * pBlend->m_fW1
				            + pMat->m[3][2] * pBlend->m_fW2
				            + pMat->m[3][3] * pBlend->m_fW3;

				pBlend++;                       // 0x10009559  add $0x14
			}

			// -- 0x1000956c .. 0x100095bf ------------------------------- //
			//  Interpolate the two keys.  One `fsub` and three `fsubr`:
			//  component 0 loads the accumulator and subtracts the
			//  destination, and 1..3 already have the destination in st0
			//  from the previous component's preload, so the operands
			//  arrive reversed.  The store of the FOURTH is dead --
			//  the reciprocal below overwrites m_fRHW immediately.
			pOut->m_vPos.x += (aBlend2[0] - pOut->m_vPos.x) * m_fLerp454;
			pOut->m_vPos.y += (aBlend2[1] - pOut->m_vPos.y) * m_fLerp454;
			pOut->m_vPos.z += (aBlend2[2] - pOut->m_vPos.z) * m_fLerp454;
			pOut->m_fRHW   += (aBlend2[3] - pOut->m_fRHW)   * m_fLerp454;

			// -- 0x100095c0 .. 0x100095e8 ------------------------------- //
			//  `fdivr 0x100a42a4` -- 1.0f over the value still in st0.
			//  One of the function's two divisions; the pool's dead 1.0f
			//  at 0x100a42a0 is its twin.
			{
				float fInvW = 1.0f / pOut->m_fRHW;

				pOut->m_vPos.x *= fInvW;
				pOut->m_vPos.y *= fInvW;
				pOut->m_vPos.z *= fInvW;
				pOut->m_fRHW    = fInvW;        // 0x100095d8  fst
			}

			// -- 0x100095e9 .. 0x10009618 ------------------------------- //
			//  The global directional term.  Note the operand order:
			//  here the NORMAL is loaded first in every product
			//  (`fld 0x1c(%ebx) ; fmuls 0x67c(%esi)`), the opposite of
			//  the per-light dot below.  m_vLightDir674 is already
			//  rotated into model space and negated by 0x10019d64.
			{
				float fR, fG, fB;
				float fDot = (pVert->m_vNormal14.x * m_vLightDir674.x
				            + pVert->m_vNormal14.y * m_vLightDir674.y
				            + pVert->m_vNormal14.z * m_vLightDir674.z)
				           * m_f680;

				// -- 0x10009610 ; jae -> the SUNK arm at 0x10009840 ---- //
				if (fDot > 0.0f)
				{
					// -- 0x1000961b .. 0x10009658 ------------------- //
					fR = m_vAmbient684.x
					   + (m_v668.x - m_vAmbient684.x) * fDot;
					fG = m_vAmbient684.y
					   + (m_v668.y - m_vAmbient684.y) * fDot;
					fB = m_vAmbient684.z
					   + (m_v668.z - m_vAmbient684.z) * fDot;
				}
				else
				{
					// -- 0x10009840, sunk below the epilogue -------- //
					fR = m_vAmbient684.x;
					fG = m_vAmbient684.y;
					fB = m_vAmbient684.z;
				}

				// -- 0x10009659 .. 0x1000970d ---------------------- //
				//  The dynamic lights, already in model space in
				//  m_Lights3c[0..m_nLights2fc-1].  The bound is 16 and
				//  it comes off the class, not off this function:
				//  0x3c + 16*0x2c == 0x2fc, which is where the count
				//  lives (big19d64.cpp).
				{
					uint32 i;

					for (i = 0; i < m_nLights2fc; i++)
					{
						ModelLight* pLight = &m_Lights3c[i];

						//  Operand order reversed relative to the sun
						//  dot: the LIGHT's direction is loaded first.
						float fLDot = pLight->m_vDir0c.x * pVert->m_vNormal14.x
						            + pLight->m_vDir0c.y * pVert->m_vNormal14.y
						            + pLight->m_vDir0c.z * pVert->m_vNormal14.z;

						if (fLDot > 0.0f)       // 0x1000969c  fcomp/jae
						{
							float dx = pVert->m_vPos08.x - pLight->m_vPos00.x;
							float dy = pVert->m_vPos08.y - pLight->m_vPos00.y;
							float dz = pVert->m_vPos08.z - pLight->m_vPos00.z;
							float fDistSqr = dx * dx + dy * dy + dz * dz;

							// 0x100096c8  fcomp/jbe
							if (fDistSqr < pLight->m_fRadiusSqr18)
							{
								float fAtten =
								    (1.0f - pLight->m_fInvRadiusSqr1c
								            * fDistSqr) * fLDot;

								fR += pLight->m_fR20 * fAtten;
								fG += pLight->m_fG24 * fAtten;
								fB += pLight->m_fB28 * fAtten;
							}
						}
					}
				}

				// -- 0x1000971d .. 0x1000974f ---------------------- //
				//  m_fR65c/m_fG660/m_fB664 are the object's byte colour
				//  already divided by 255 (big19d64.cpp:617-619), which
				//  is why the clamp below is at 255 and not at 1.
				fR = fR * m_fR65c + m_v650.x;
				fG = fG * m_fG660 + m_v650.y;
				fB = fB * m_fB664 + m_v650.z;

				// -- 0x10009750 .. 0x10009791 ---------------------- //
				//  Three `if` clamps, six 255.0f pool slots, in the
				//  order R, G, B.  There is NO lower clamp: the census
				//  allows exactly six float compares per arm and all six
				//  are accounted for.
				if (fR > 255.0f) fR = 255.0f;
				if (fG > 255.0f) fG = 255.0f;
				if (fB > 255.0f) fB = 255.0f;

				// -- 0x10009792 .. 0x100097e6 ---------------------- //
				//  Three float->int32->byte conversions, each `fstp
				//  DWORD ; fld DWORD ; fistp DWORD ; mov BYTE`.  /Qrcd.
				pOut->m_r = (uint8)fR;
				pOut->m_g = (uint8)fG;
				pOut->m_b = (uint8)fB;
			}

			// -- 0x100097e9 .. 0x100097fa ----------------------------- //
			pOut->m_a = m_bAlpha6a8;

			pfnCall(this, pVert, pOut);

			// -- 0x100097fc / 0x100097ff ------------------------------ //
			pVert++;
			pOut++;
		}
	}
	else
	{
		// -- 0x10009015 / 0x10009017 ------------------------------------ //
		while (nVerts--)
		{
			//  Four separate `= 0.0f` (the last four dead pool slots).
			pOut->m_vPos.x = 0.0f;
			pOut->m_vPos.y = 0.0f;
			pOut->m_vPos.z = 0.0f;
			pOut->m_fRHW   = 0.0f;

			// -- 0x1000902e / 0x10009033 ------------------------------- //
			ModelBlend* pBlend  = pVert->m_pBlends00;
			uint32      nBlends = pVert->m_nBlends04;

			// -- 0x10009050 .. 0x100090ff ------------------------------ //
			//  Identical to KEY 1 above, operand for operand.
			while (nBlends--)
			{
				LTMat* pMat = &pMatrices[pBlend->m_nMatrix10];

				pOut->m_vPos.x += pMat->m[0][0] * pBlend->m_fW0
				                + pMat->m[0][1] * pBlend->m_fW1
				                + pMat->m[0][2] * pBlend->m_fW2
				                + pMat->m[0][3] * pBlend->m_fW3;
				pOut->m_vPos.y += pMat->m[1][0] * pBlend->m_fW0
				                + pMat->m[1][1] * pBlend->m_fW1
				                + pMat->m[1][2] * pBlend->m_fW2
				                + pMat->m[1][3] * pBlend->m_fW3;
				pOut->m_vPos.z += pMat->m[2][0] * pBlend->m_fW0
				                + pMat->m[2][1] * pBlend->m_fW1
				                + pMat->m[2][2] * pBlend->m_fW2
				                + pMat->m[2][3] * pBlend->m_fW3;
				pOut->m_fRHW   += pMat->m[3][0] * pBlend->m_fW0
				                + pMat->m[3][1] * pBlend->m_fW1
				                + pMat->m[3][2] * pBlend->m_fW2
				                + pMat->m[3][3] * pBlend->m_fW3;

				pBlend++;                       // 0x100090f8  add $0x14
			}

			// -- 0x10009108 .. 0x10009133 ------------------------------ //
			//  `fld 0x100a42e0 ; fdivs 0xc(%edi)`.  The accumulated w is
			//  in MEMORY here (the blend loop stored it), which is why
			//  this arm divides with `fdiv` and the two-key arm with
			//  `fdivr`.  Dead 1.0f twin at 0x100a42dc.
			{
				float fInvW = 1.0f / pOut->m_fRHW;

				pOut->m_vPos.x *= fInvW;
				pOut->m_vPos.y *= fInvW;
				pOut->m_vPos.z *= fInvW;
				pOut->m_fRHW    = fInvW;        // 0x10009123  fst
			}

			// -- 0x10009134 .. 0x10009163 ------------------------------ //
			{
				float fR, fG, fB;
				float fDot = (pVert->m_vNormal14.x * m_vLightDir674.x
				            + pVert->m_vNormal14.y * m_vLightDir674.y
				            + pVert->m_vNormal14.z * m_vLightDir674.z)
				           * m_f680;

				// -- 0x1000915b ; jae -> the SUNK arm at 0x1000981e ---- //
				if (fDot > 0.0f)
				{
					// -- 0x10009166 .. 0x100091a3 ------------------- //
					fR = m_vAmbient684.x
					   + (m_v668.x - m_vAmbient684.x) * fDot;
					fG = m_vAmbient684.y
					   + (m_v668.y - m_vAmbient684.y) * fDot;
					fB = m_vAmbient684.z
					   + (m_v668.z - m_vAmbient684.z) * fDot;
				}
				else
				{
					// -- 0x1000981e, sunk below the epilogue -------- //
					fR = m_vAmbient684.x;
					fG = m_vAmbient684.y;
					fB = m_vAmbient684.z;
				}

				// -- 0x100091a4 .. 0x1000924f ---------------------- //
				{
					uint32 i;

					for (i = 0; i < m_nLights2fc; i++)
					{
						ModelLight* pLight = &m_Lights3c[i];

						float fLDot = pLight->m_vDir0c.x * pVert->m_vNormal14.x
						            + pLight->m_vDir0c.y * pVert->m_vNormal14.y
						            + pLight->m_vDir0c.z * pVert->m_vNormal14.z;

						if (fLDot > 0.0f)       // 0x100091de  fcomp/jae
						{
							float dx = pVert->m_vPos08.x - pLight->m_vPos00.x;
							float dy = pVert->m_vPos08.y - pLight->m_vPos00.y;
							float dz = pVert->m_vPos08.z - pLight->m_vPos00.z;
							float fDistSqr = dx * dx + dy * dy + dz * dz;

							// 0x1000920a  fcomp/jbe
							if (fDistSqr < pLight->m_fRadiusSqr18)
							{
								float fAtten =
								    (1.0f - pLight->m_fInvRadiusSqr1c
								            * fDistSqr) * fLDot;

								fR += pLight->m_fR20 * fAtten;
								fG += pLight->m_fG24 * fAtten;
								fB += pLight->m_fB28 * fAtten;
							}
						}
					}
				}

				// -- 0x1000925f .. 0x10009291 ---------------------- //
				fR = fR * m_fR65c + m_v650.x;
				fG = fG * m_fG660 + m_v650.y;
				fB = fB * m_fB664 + m_v650.z;

				// -- 0x10009292 .. 0x100092d3 ---------------------- //
				if (fR > 255.0f) fR = 255.0f;
				if (fG > 255.0f) fG = 255.0f;
				if (fB > 255.0f) fB = 255.0f;

				// -- 0x100092d4 .. 0x10009328 ---------------------- //
				pOut->m_r = (uint8)fR;
				pOut->m_g = (uint8)fG;
				pOut->m_b = (uint8)fB;
			}

			// -- 0x1000932b .. 0x1000933c ----------------------------- //
			pOut->m_a = m_bAlpha6a8;

			pfnCall(this, pVert, pOut);

			// -- 0x1000933e / 0x10009341 ------------------------------ //
			pVert++;
			pOut++;
		}
	}

	// -- 0x1000980d ---------------------------------------------------- //
	//  One epilogue, reached by both loop exits and by both empty-set
	//  guards (0x10009017 and 0x1000936b).
}

// ----------------------------------------------------------------------- //
//  11.  BUILD NOTES, AND WHAT A BUILD WOULD SETTLE
// ----------------------------------------------------------------------- //
//
//  FLAGS, judged from this function's own census and nothing else:
//
//    /Qip      REQUIRED, and it has a structural cost.  The two-entry
//              pair at 0x10008fd0/0x10008fd4 is the external-linkage form
//              of /Qip, so reproducing it needs the flag AND
//              needs this function to share a translation unit with its
//              same-TU caller 0x10008e10 -- which is the one place in this
//              reconstruction where our invented file boundaries have to
//              model a shipped TU's membership.  Note the measured
//              caution: icl emits both entries but routes the SELF-call to
//              the stack entry, the reverse of the shipped build, so the
//              +4 call is expected to show as a `~~` acceptance rather
//              than as an exact match.  Here it is moot -- the two calls
//              this function MAKES are indirect, and the call INTO it is
//              the caller's problem.
//
//    /Qrcd     REQUIRED.  Six `fistp` conversions, three per arm, each
//              `fstp DWORD ; fld DWORD ; fistp DWORD ; mov BYTE`.  At
//              plain /O2 each becomes the twelve-instruction
//              shld/neg/or/shr/and/xor/cmp/test/jge/jle truncation, which
//              would add about 66 instructions to a 661-instruction
//              function -- 10% of the census, impossible to miss.  This
//              TU belongs in build/engine.sh's D3D_RCD_TUS list.  The
//              known residual (shipped 32-bit `fistpl` with a
//              float round trip against /Qrcd's 64-bit `fistpll` with
//              none) IS present here and is not closed by anything in
//              this file.
//
//    /Oi-      IRRELEVANT, and this is a positive finding rather than a
//              shrug.  There is no candidate: the census contains no
//              `fsqrt`, no `fabs`, no `fpatan`, no `fptan`, no `fscale`,
//              no `rep movs` and no direct call at all.  The two square
//              terms are `d8 c8` (`fmul %st,%st(0)`), which is
//              multiplication and not `pow`; the two reciprocals are
//              `fdiv`/`fdivr` against a 1.0f literal, not `_fltused`
//              helpers.  So this TU can go in or stay out of D3D_OI_TUS
//              without moving a byte of THIS function.
//
//    /Op       must stay OFF (the default).  Section 5(e): twelve of the
//              frame's slots are 64-bit spills of `float` values, which
//              is precisely what /Op would suppress.
//
//    #pragma function(...)   does nothing (measured).
//    /Ge                     would add a `_chkstk` this function does not
//                            have and, measured elsewhere, costs
//                            inlining.  Do not add it.  There is no
//                            `_chkstk` residual to close: the frame is
//                            0xf4 bytes and the shipped prologue is a
//                            bare `sub $0xf4,%esp`.
//
//  WHAT A BUILD WOULD SETTLE, in decreasing order of value:
//
//    1. Whether the two arms come out as two inline copies (as written)
//       or whether icl factors the shared lighting tail.  The frame
//       (section 5(a)) says two copies, but a build would confirm the
//       source shape rather than the compiler's.
//    2. The five loop spellings (coin-flip 2).  A build tells you whether
//       `while (n--)` reproduces the shared -0x58 slot or whether
//       `for (i = 0; i < n; i++)` does.
//    3. Whether the fourth LERP's store really is dead-eliminated from an
//       assignment to `pOut->m_fRHW` (coin-flip 4).
//    4. Nothing about the call check: it is 2 of 2 by position either
//       way, and it will be 2 of 2 even if this whole reconstruction is
//       wrong.  Do not quote it without section 3.
//
// ----------------------------------------------------------------------- //
//  12.  SUMMARY OF THE EVIDENCE
// ----------------------------------------------------------------------- //
//
//    call check      2 calls in 2,191 bytes, BOTH INDIRECT, 0 of 0
//                    distinct direct callees, 1,096 B/call.  Predicted
//                    2/2 by POSITION ONLY.  Witnesses 4 of 2,191 bytes
//                    (0.18%) and 0 by resolved target.  Near-vacuous.
//    census          661 instructions, 28 regions summing to 661, every
//                    mnemonic row over five decomposed by operand with
//                    ZERO residue.  Five rows are load-bearing: `fadd`
//                    1+1+4 (proves the preload chain and therefore the
//                    four matrix rows), `faddp` 15/15/12 (same), `fsub`
//                    13 against `fsubr` 5 (proves the four LERPs are one
//                    shape), `fcomp` 12 (proves there is no lower colour
//                    clamp), `fistp` 6 (proves /Qrcd).
//    frame           sub $0xf4 = 244 bytes, decomposed to the byte over
//                    35 distinct dumped slots (37 table rows, two of them
//                    dual-use across disjoint live ranges), five unused
//                    blocks and the five 4-byte gaps inside the
//                    conversion-temp run.  Proves the arms are
//                    duplicated code, that the second key's blend target
//                    is four contiguous floats, that there is no vertex
//                    buffer, that exactly one loop per arm spans the
//                    call, and that no local is a `double`.
//    literal pool    34 slots, 0x100a4280..0x100a4304, bounded by the
//                    neighbours' own loads at 0x100a427c and 0x100a4308.
//                    20 live, 14 dead, ALL FOURTEEN EXPLAINED: two
//                    dead-1.0f division twins (cross-checking `fdiv` 1 +
//                    `fdivr` 1) and twelve dead 0.0f matching the twelve
//                    zero assignments.  Fixes the SOURCE ORDER OF THE TWO
//                    ARMS, which nothing else in the function recovers,
//                    proves the zeroes are twelve separate float
//                    statements rather than three chains, and proves the
//                    three clamps are `if` statements in R/G/B order.
//    correspondence  R13..R17 against R4..R8: 149 instructions, one
//                    difference (a `jmp` versus a fallthrough), and not
//                    one differing operand offset in the 40-row table of
//                    section 7.  Plus every `this` offset cross-checked
//                    against big19d64.cpp (which WRITES the light table
//                    this function READS), big17c14.cpp, big4b720.cpp and
//                    big43e10.cpp, and the destination vertex against
//                    big16324.cpp and big0ae20.cpp.
//    x87 order       no hazardous DE/DC/D8 sub/div register form occurs
//                    at all (grep: zero matches in E0..FF); the memory
//                    forms and the fcomp/jae sense were verified by
//                    EXECUTING the shipped opcode bytes; and the compare
//                    operand order was calibrated inside the function
//                    from the two clamps that use a memory operand.
//    undecoded       NONE.  All 661 instructions and all 2,191 bytes are
//                    attributed.
//
