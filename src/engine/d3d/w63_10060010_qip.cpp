// *********************************************************************** //
//
//  MODULE:   W63_10060010_QIP (.CPP) -- w40_10060010_qipni.cpp (itself
//            big60010.cpp) and big61a30.cpp merged.
//
//  BUILD:    /O2 /Qip (the _qip convention).  0x10060010 is a /Qip
//            REGISTER-ONLY function (`push %ecx ; push %edx ; push %eax`
//            homing pCtx/pObj/pPos, fScaleX in ST(0)) whose one caller is
//            0x10061a30 (0x10061b64): icl gives it that entry only with
//            the caller in its TU, so the two files are one.  0x1005f7d0
//            stays a static here, reached at its own register entry.
//
//  MEASUREMENTS -- bytes differing / frame:
//    * the merge itself: 0x10060010 now opens as shipped (register entry,
//      `mov $0x2d5c,%eax ; call _chkstk`, the three homed arguments read
//      back) -- it was a plain stack function with a 0x818 frame;
//      0x1005f7d0 stays near (8 bytes); 0x10061a30 stays near with
//      big61a30.cpp's text unchanged (frame the shipped 0x978, 12247 B).
//    * flags, same source: /O2 /Qip, /O2 /Qip /Qip_no_inlining and
//      /O2 /Qip /Qsfalign16 give the same three bodies; /Qip_no_inlining
//      also emits dead out-of-line RoundF/sub_10023380 copies, so /Qip.
//      /Os /Qip and /Os /Qip /Qip_no_inlining: 0x1005f7d0 353 bytes off
//      (near lost), 0x10061a30 9049.  So /O2.
//    * 0x10060010's clipper as d3dr3c_os.cpp's 0x1000f4b0 driver INLINED
//      over its six plane bodies (w38_10028030_qip.cpp's set over
//      this vertex): 5740 -> 5228 bytes differing, mnemonic agreement
//      0.97, frame 0x2d5c -> 0x2d54 against the shipped 0x2d64.  (The
//      plane bodies are named ClipPlane<Side>60010, not Plane32_<addr>:
//      icl also emits them out of line, unreferenced, and an address
//      name would make the byte comparison list identical addresses as differing;
//      the renaming leaves every byte of the file's code unchanged.)  Every
//      slot below -0xa8 then lines up with the shipped frame; the
//      missing 0x10 is at the top: the shipped code keeps the -20.0f
//      bias in a double slot (`flds -20.0f ; fstpl -0x88` on entry to
//      the m_nFlags88 & 4 arm, `fldl -0x88 ; fadd %st(1)` in the loop)
//      and gives the plane loop's pVerts spill (-0x10) its own slot
//      where ours shares one with a -fTexH temporary.  NOT closed by:
//      the bias as a const float, a float or double local (at the top
//      or assigned in the arm), the literal, a double/float fBias, fZ
//      or none, double fTexW/fTexH/fU0..fV1 (96 combinations measured:
//      frames 0x2d54/0x2d60/0x2d70/0x2d80/0x2d90, never 0x2d64), the
//      arm as an inline function taking the bias as a parameter (icl
//      folds the constant argument).
//    * 0x10061a30 with the same driver inline, the light grid as
//      vSample[8] + four blend vectors in memory (the shipped -0x80c /
//      -0x7ac blocks), int ambient terms, and RoundF everywhere: the
//      frame CANNOT be had -- icl 4.0 stops inlining in this body once
//      the six planes are in (the three ambient RoundF calls stay calls,
//      and w24's BindBlock inline set stays a call), whichever way the
//      inlines are spelled (static/__inline/__forceinline, ClipEdge
//      expanded, the lerp's RoundF written as bare __asm).  A RoundF
//      placed after the clipper IS inlined, one before it is not.  The
//      shipped body has all of it inline, so the shipped TU (or build)
//      inlined differently -- possibly /Qipo cross-file inlining of the
//      out-of-line 0x1000f4b0/0x10001000 family; not measured (it is a
//      toolchain-wide change).  With (int32) casts for the ambient the
//      frame is 0x970 (8 short); big61a30.cpp's text is kept because it
//      is the one that holds the shipped 0x978 and so the pairing.
//    * LATER, with the clipper as the six static inline plane bodies
//      below: RoundF IS inlined at every one of 0x10061a30's 33 sites
//      (the thirty lerp bytes and the three ambient terms, 0x10062bf9..
//      0x10062c69) -- the earlier "stays a call" verdict does not hold
//      for this text.  Measured on 0x10061a30 (tools/census.py,
//      tools/fndiff.py, ship 11280 B / 2719 instructions):
//          (uint8)(int32) casts, /O2 /Qip   12256 B / 3302, residue 1217,
//                                            242 blocks, frame 0x978; the
//                                            33 sites each the 12-instruction
//                                            shld/neg/or/jge truncation
//          the same, /O2 /Qip /Qrcd           9056 B / 2302, residue 675:
//                                            39 fistpll, 0 fistpl
//          RoundF at the 33, /O2 /Qip         9824 B / 2437, residue 506,
//                                            146 blocks, frame 0x940:
//                                            fistpl 33 = shipped, and the
//                                            six plain casts (nX/nY/nZ and
//                                            the three LTCLAMPs) the shld
//                                            idiom = shipped (6)
//          RoundF, /Os /Qip                  10102 B / 2347, residue 774
//                                            (0x10060010 6351, 0x1005f7d0
//                                            448: both worse)
//          RoundF, /O2 /Qip /G5               9927 B / 2634, residue 675
//      and icl's inlining knobs, passed through as
//      /Qoption,c,-mIPOPT_inline_max_total_stats=100000,
//      -mIPOPT_inline_max_stats=100000 and -mIPOPT_inline_limit=100000
//      (the names are in mcpcom.exe; the driver itself emits
//      -mIPOPT_inline_limit=0 for /Qip_no_inlining), each give the same
//      9824-byte body: nothing was left un-inlined for them to inline.
//      The RoundF text is the one kept: every conversion is now the
//      shipped idiom, the call sequence is unchanged (29/29, the four
//      indirect block-placement rows), 0x10060010 and 0x1005f7d0 are
//      byte-for-byte what they were, and the body stays within the 15%
//      the near pairing allows (9824/11280 = 0.871).  What remains is
//      the ship keeping 163 more `mov`, 59 `fstps` and 43 `flds` --
//      values held in frame slots (the missing 0x38 of frame) where
//      icl keeps ours in registers, and the ship's cold arms sunk where
//      ours are inline -- not the conversions.
//    * A LEAD, measured but not closed: the shipped front half loads
//      EVERY literal as a float (`flds 0x100aad5c` 2.0, `fsubs 0x100aad54`
//      2.0, `fsubs 0x100aad48` 10.0, `fsubrs 0x100aad44` 500.0 ...) and
//      computes the fade's `500 - 10` and `2.0 - 0.1` at run time
//      (0x10061f6b..0x10061fa9: `fsubs 10 ; flds 10 ; fsubrs 500 ;
//      fdivrp`, then `flds 2.0 ; fsubs 0.1 ; fmulp ; fadds 0.1`), where
//      ours, with fZ/fFade/fTexW... declared double, folds both and
//      multiplies by 1/490 (`fmull` 0.00204, `fmull` 1.9, `faddl` 0.1)
//      and holds 2.0/7.0/0.01/1.0/-20.0 as DOUBLE pool entries (26 `fmull`
//      against the shipped 4, 23 `fmuls` short).  The shipped locals are
//      still doubles (`fldl -0x78(%ebp)` is fZ), so this is the literal's
//      type, not the local's: fFade as a float changes nothing (same 9824
//      bytes), /Op and /Qprec make everything worse (13088 / 10320 bytes,
//      0x10060010 8048 / 7040), so it is a source shape.  The shipped
//      pool runs 2.0, 0.1, 500, 10, 0.01, 2.0, 2.0, 2.0, 2.0 (0x100aad3c..
//      0x100aad5c), i.e. the fade's four literals are emitted BEFORE
//      fMinZ's 0.01f and the four 2.0f of the uv inset, in the order
//      (2.0 - 0.1) then (500 - 10): the fade was written earlier in the
//      source than its use here (an inline helper or a macro defined
//      above the function) with `(2.0f - 0.1f)` before `(500.0f -
//      10.0f)`.  The literal pool is the statement-order witness
//      (guide 11).  Two spellings of that were then measured and are NOT
//      it: a `static inline float Fade61a30(float fZ)` defined above the
//      function with the literals inside it (the pool entries become
//      floats, residue 506 -> 494, but icl still folds: `fmuls` 1/490,
//      `fmuls` 1.9), and the same helper taking (double z, float near,
//      float far, float min, float max) called with (fZ, 10.0f, 500.0f,
//      0.1f, 2.0f) -- the pool order read as right-to-left argument
//      evaluation -- which icl propagates and folds just the same
//      (9808 bytes, residue 507, `fmull` 1/490 again).  icl 4.0 at
//      /O2 folds a literal subtraction in float and in double context,
//      before and after inlining; the shipped divisor was therefore not
//      two literals to its compiler, and the kept text is the plain
//      statement.
//
//  PURPOSE:  One function -- 0x10060010, 6,688 bytes (0x1a20), so the
//            claimed extent 0x10060010..0x10061a30 is exactly right; see
//            THE EXTENT below for the five checks.  ONE caller in the whole
//            image, 0x10061b64, which is the `m_nFlags88 & 0x8` arm of the
//            sprite pass big61a30.cpp reconstructs.
//
//  WHAT IT IS:  d3d.ren's WORLD-ORIENTED SPRITE PASS -- the arm of the
//            sprite draw that builds the quad in the OBJECT's own rotated
//            frame instead of facing the camera.  big61a30.cpp's
//            0x10061a30 is the camera-facing twin and shares the tail
//            (six-plane clipper, projection, DrawPrimitive) almost
//            instruction for instruction; the whole front half is
//            different and is decoded here from the bytes.
//
//            Six tells, none of them guessed:
//              * the ONE caller is `if (pObj->m_nFlags88 & 0x8)` inside
//                the OT_SPRITE arm (big61a30.cpp:637), and it `goto`s
//                straight to that function's render-state unwind
//                afterwards -- so this IS the whole draw for that flag;
//              * the quad is built from a matrix that 0x100151b0 makes out
//                of (position, quaternion at +0x9c, scale at +0xac)
//                (0x10060177) -- an object-space orientation, where
//                big61a30 uses the camera basis;
//              * the facing normal is (0, 0, -1) rotated by the 3x3 of
//                that matrix (0x10060196 + 0x100601b1) and dotted against
//                (pCtx->m_v4b4 - *pPos) (0x1006022c); the sign of the dot
//                picks between two four-entry INDEX tables, 0x100897e0 =
//                {3,2,1,0} and 0x100897f0 = {0,1,2,3} -- a winding flip,
//                which is only meaningful for a two-sided oriented quad;
//              * the four vertices are (-w,+h), (+w,+h), (+w,-h), (-w,-h)
//                with the standard two-texel uv inset, exactly big61a30's
//                corner set;
//              * DrawPrimitive at 0x100618d8 is
//                (device, 6, 0x1c4, pVerts, nVerts, 0) -- D3DPT_TRIANGLEFAN
//                and D3DFVF_XYZRHW|DIFFUSE|SPECULAR|TEX1, which is the
//                32-byte vertex every stride in the function agrees with
//                (`shl $0x5`, `add $0x20`, `imul $0x20` throughout);
//              * the six clip passes test z >= g_f1008b908, -z < x, y < z,
//                x < z, -z < y and z <= g_f1008b910 under mask bits 0x01,
//                0x04, 0x08, 0x10, 0x20, 0x02 IN THAT ORDER -- the same
//                six planes in the same non-numeric order that
//                d3dr3c.cpp's clip driver and big61a30.cpp both record.
//
//  ------------------------------------------------------------------- //
//  THE EXTENT.  Verified before a byte was decoded.
//  ------------------------------------------------------------------- //
//
//  0x10060010 .. 0x10061a30 -- 6,688 bytes -- is CORRECT as claimed.
//
//    * THE START IS A REAL FUNCTION START, not a /Qip register entry
//      reached from an earlier stack entry.  0x10060002..0x1006000f is
//      fourteen bytes of lea-nop padding
//      (`lea 0x0(%esi,%eiz,1),%esi ; lea 0x0(%edi,%eiz,1),%edi`) and the
//      function before it ends with a `ret` at 0x10060001.  There is no
//      `mov 0x4(%esp),%eax` stack entry anywhere in front of it.
//      0x10060010 IS the register entry: the prologue is
//      `push %ebp ; mov %esp,%ebp ; push %ecx ; push %edx ; push %eax`,
//      which SAVES the three incoming register arguments, and the three
//      are read straight back at 0x10060020/0x10060027/0x1006002e after
//      _chkstk moves %esp.  A function with a stack entry would not need
//      to.
//    * ONE `push %ebp ; mov %esp,%ebp` in the whole range, at 0x10060010.
//      Nothing else in 6,688 bytes.
//    * THREE `ret`s -- 0x10060097, 0x100618e9, 0x100618f8 -- and all three
//      are the SAME five-instruction epilogue
//      (`mov (%esp),%ebx ; mov 0x4(%esp),%esi ; mov 0x8(%esp),%edi ;
//        mov %ebp,%esp ; pop %ebp ; ret`) restoring THIS frame's three
//      spill slots.  0x10060097 is the `sub_10015320` early return,
//      0x100618db..0x100618e9 the normal exit and 0x100618ea..0x100618f8
//      /O2's duplicate of it for the `sub_1005fb70` early return.  No
//      `ret N` anywhere, so no second calling convention.
//    * THE LAST INSTRUCTION ENDS ON THE STATED END.  0x10061a17 is
//      `mov $0x100897f0,%esi` and 0x10061a1c `jmp 0x10060245`, ending at
//      0x10061a21; 0x10061a21 is one `nop` and 0x10061a22..0x10061a2f is
//      fourteen more bytes of lea-nop padding.  The end is not
//      mid-instruction, not an interior label and not another function's
//      entry.
//    * EVERY BRANCH TARGET IS INSIDE THE RANGE AND ON AN INSTRUCTION
//      BOUNDARY.  All 97 jump/branch targets in the range were collected
//      and checked against the instruction-address set: 97 of 97 inside,
//      0 misaligned.
//    * the call count over 0x10060010..0x10061a30 gives 30 calls
//      (26 direct, 4 indirect), 223 bytes per call -- the claimed figure,
//      unchanged by the extent check.
//
//    The 443 bytes from 0x10061875 to the end are /O2 cold-block sinking.
//    TEN blocks, every one using THIS frame and every one branching back
//    above 0x10061875: the `else` of `m_nFlags88 & 4` (0x10061875, which
//    falls into the shared draw tail at 0x1006189b and so sits between
//    the two duplicated epilogues), then, below the LAST `ret`, the six
//    `> +0.00001f` halves of the clip epsilon tests (0x100618f9,
//    0x10061920, 0x10061950, 0x10061980, 0x100619b0, 0x100619e0, each
//    with its `fT = 0.0f` partner at 0x1006190e / 0x10061937 /
//    0x10061967 / 0x10061997 / 0x100619c7 / 0x100619f5), the
//    `fBias = -20.0f` arm (0x10061a00), the `nDraw = 0` reject
//    (0x10061a10) and the `pIdx = g_a100897f0` arm (0x10061a17).
//
//  ------------------------------------------------------------------- //
//  THE SIGNATURE, read off the ONE call site at 0x10061b47..0x10061b69.
//  ------------------------------------------------------------------- //
//
//      add   $0xffffffe8,%esp            ; 0x18-byte outgoing area
//      mov   0xb0(%edx),%ecx             ; pObj->m_fScaleYb0, as a dword
//      flds  0xac(%edx)                  ; pObj->m_fScaleXac, into ST(0)
//      mov   %ecx,0x10(%esp)             ; -> callee's 0x18(%ebp)
//      mov   (%edi),%edi                 ; *pObj->m_ppBlock1b0
//      mov   %eax,%ecx                   ; pPos (from sub_100158b0)
//      mov   %edi,0x14(%esp)             ; -> callee's 0x1c(%ebp)
//      mov   %esi,%eax                   ; the caller's own first argument
//      call  0x10060010
//      add   $0x18,%esp
//
//  Six arguments in six dword slots at 0x8(%ebp)..0x1c(%ebp), of which
//  icl /Qip puts arg0 in %eax, arg1 in %edx, arg2 in %ecx and the first
//  FLOAT (arg3) in ST(0), leaving arg4 and arg5 on the stack.  The callee
//  confirms every part of that: it opens `push %ecx ; push %edx ;
//  push %eax`, reloads all three, does `fstp %st(0)` at 0x10060040 to drop
//  the incoming ST(0), and the ONLY positive %ebp displacement it ever
//  touches is 0x1c(%ebp) (0x10060065).  So:
//
//      void sub_10060010(Ctx60010* pCtx, Obj60010* pObj, LTVec* pPos,
//                        float fScaleX, float fScaleY, TexBlock60010*);
//
//  which is byte for byte the declaration big61a30.cpp already carries at
//  its line 525.  Written __cdecl here: there is no way to
//  ask icl for the register convention from C, and it is still ONE call in
//  the same position.
//
//  fScaleX AND fScaleY ARE DEAD.  0x18(%ebp) is never read and ST(0) is
//  popped and discarded at 0x10060040.  That is not a decode gap: the
//  function re-derives the same two numbers from the object itself, by
//  handing &pObj->m_vScaleac (+0xac, whose first two floats ARE
//  m_fScaleXac / m_fScaleYb0) to 0x100151b0 at 0x1006015c.  pPos is
//  likewise passed AND re-fetched -- pObj->sub_100158b0() is called three
//  times (0x1006004a, 0x100600f2, 0x1006014e, 0x10060247) for the very
//  address the caller passed in %ecx -- and the parameter itself is used
//  only once, as the by-value operand of the subtraction at 0x100601cf.
//
//  ------------------------------------------------------------------- //
//  COMPILER VERDICT: Intel C/C++ 4.0, **/O2 /Qip**, the module default.
//  ------------------------------------------------------------------- //
//
//  FOR /O2:
//    * OUTGOING ARGUMENTS as `add $-N,%esp` + `mov %reg,K(%esp)` at
//      0x10060062, 0x10060153, 0x1006019b, 0x100601bc, 0x100601ff,
//      0x1006024c, 0x100603fc, 0x10060457, 0x10061850 and 0x10061875 --
//      not one `push` of a real argument in the function (`push %edi` at
//      0x1006004f is `sub $4,%esp`, the value is dead).
//    * COLD-BLOCK SINKING BELOW THE EPILOGUE, ten-fold; enumerated
//      under THE EXTENT.
//    * `&p->m` as `mov $K,%reg ; add %p,%reg` at 0x10060850 and its five
//      twins (`mov $0x20,%ecx ; add %eax,%ecx` for `pOut++`), never
//      `lea 0x20(%eax)`.
//    * `mov $0x1,%edi` / `mov $0x0,%eax` as FIVE-byte immediates in the
//      six inside-flag loops rather than `push/pop` -- the /O2 form.
//
//  AGAINST (recorded, not explained away):
//    * the shared draw test at 0x1006189b is reached from BOTH arms of
//      `m_nFlags88 & 4` rather than being duplicated.
//
// *********************************************************************** //
//
// *********************************************************************** //
//  REGION MAP, with what pins each one.
// *********************************************************************** //
//
//  0x10060010..0x10060062  PROLOGUE + THE DISCARDED DISTANCE.  _chkstk
//      0x2d64, the three register arguments spilled and reloaded, the three
//      callee-saved registers parked at the BOTTOM of the frame
//      ((%esp), 0x4(%esp), 0x8(%esp) -- so they move with every outgoing
//      area), `fstp %st(0)` dropping fScaleX, and then
//          pObj->sub_100158b0()->sub_100154b0(pCtx->m_v4b4);
//      whose float result is popped and thrown away by the `fstp %st(0)`
//      at 0x10060060.  There is no store and no branch after it: the
//      distance is computed and discarded.  0x100154b0 is d3dr2d_os.cpp's
//      _CVector::Dist (`ret $0x4`, one reference argument, %ecx = this),
//      and 0x100158b0 is `lea 0x18c(%ecx),%eax` out of line -- both
//      already pinned elsewhere in src/engine/d3d.
//
//  0x10060062..0x10060098  THE TEXTURE GATE.
//          if (!sub_10015320(pBlock, g_n1008d7d0, 0)) return;
//      A 0x10-byte outgoing area with three slots filled, `add $0x10,%esp`,
//      `test %eax,%eax ; jne` over an inline epilogue -- the pattern
//      "error falls through, success is jumped over".  0x10015320's
//      (pointer, stage, flag) shape is big5a960.cpp's.
//
//  0x10060098..0x10060106  THE TEXTURE SIZE.  Two INDIRECT calls, BOTH
//      through vtable slot +0x0c, both on g_a1008e684[g_n1008d7d0], each
//      widened to double by `mov %eax,-0xa8 ; movl $0,-0xa4 ; fildll`
//      (an UNSIGNED 32-bit result).  big61a30.cpp's twin reads +0x0c for
//      the width and +0x10 for the HEIGHT; this function reads +0x0c
//      twice, and the second result is used exactly where a height is used
//      (the y extent and the v coordinate).  The two `8b 40 0c` bytes at
//      0x100600a6 and 0x100600d2 were checked individually.  Written below
//      as two calls to the same accessor with a note; see COIN-FLIPS.
//
//  0x10060106..0x1006014c  THE UV CORNERS, and the CONSTANT POOL settles
//      their source order.  The four 2.0f the block uses are FOUR DISTINCT
//      pool slots -- 0x100aaca4, 0x100aaca8, 0x100aacac, 0x100aacb0 -- and
//      the code reaches them in the order aca8, acb0, aca4, acac.  Pool
//      order is SOURCE order, so the four statements are
//          fU0 = invW * 2.0f;                (aca4, used at 0x10060132)
//          fU1 = (fTexW - 2.0f) * invW;      (aca8, used at 0x10060109)
//          fV0 = invH * 2.0f;                (acac, used at 0x1006013e)
//          fV1 = (fTexH - 2.0f) * invH;      (acb0, used at 0x10060118)
//      and NOT the emitted order.  invW/invH are g_a1008e664[0].m_fInvW04
//      and .m_fInvH08 -- index 0, not the stage: `fmuls 0x1008e664` and
//      `fmuls 0x1008e668` with no index register, at 0x1006010f, 0x1006011e,
//      0x10060132, 0x1006013e.  All four results are stored with `fstpl`,
//      so all four are DOUBLE (-0x70, -0x68, -0x60, -0x78).
//
//  0x100600f2..0x10060106  THE SPECULAR.  `g_pfn_1008ce8c(pos, &nSpec)` --
//      the __fastcall(%ecx,%edx) global d3dr2d_os.cpp pinned across
//      fourteen sites.  Its out-parameter -0x2c54 is the dword every one
//      of the four vertices stores into +0x14.
//
//  0x1006014c..0x100601b6  THE OBJECT FRAME.
//          sub_100151b0(pObj->sub_100158b0(), &pObj->m_Rot9c,
//                       &pObj->m_vScaleac, &mat);
//          vDir.Init(0.0f, 0.0f, -1.0f);
//          sub_1005f790(&mat, &vDir);
//      The argument list is read off the four slots at 0x10060162..
//      0x10060174; big17c14.cpp and d3dbatchd.cpp both record 0x100151b0 as
//      (position, quaternion, scale, matrix).  0x1005f790 is
//      ltmatrix.h:274 MatVMul_InPlace_3x3, decoded verbatim in d3dr2d.cpp.
//      The THREE literals of the Init are 0.0f, 0.0f, -1.0f folded to
//      immediates (`xor %eax,%eax` reused twice plus
//      `movl $0xbf800000,0x8(%esp)`) -- and the pool has 0.0f at
//      0x100aacb4, 0.0f at 0x100aacb8 and -1.0f at 0x100aacbc, in that
//      order and referenced from nowhere.  That is the pool confirming
//      both the statement's position and its argument order.
//
//  0x100601b6..0x1006023a  THE WINDING TEST.
//          vDelta = pCtx->m_v4b4 - *pPos;            (0x10015640, ret $0x10)
//          if (vDir.Dot(vDelta) < 0.0f)              (0x10027f50, by value)
//              pIdx = g_a100897e0;                   /* {3,2,1,0} */
//          else
//              pIdx = g_a100897f0;                   /* {0,1,2,3} */
//      The operator's return temp (-0x2c04), the copy out into the named
//      local (-0x2bf8) and the by-value copy into Dot's slot are all three
//      present, which is the known pattern "a vector operator leaves a temp plus a
//      whole-vector copy".  `fcomps 0x100aacc0` is against 0.0f (dumped),
//      `jae` sends the >= 0 case to the SUNK block at 0x10061a17, so the
//      `< 0` arm is the inline one.  Both tables were dumped from the image.
//
//  0x1006023a..0x1006027e  THE CENTRE AND THE COLOUR.
//          sub_10014940(&vXfm, &g_1008b880_m15c_of_pCtx, pos);   -- NO:
//      read exactly, `add $0x15c,%edi` on pCtx, so it is
//          sub_10014940(&vXfm, pCtx->m_m15c, pObj->sub_100158b0());
//      (d3dr2c_os.cpp: MatVMul, three __cdecl arguments, called not
//      inlined), then the register-ABI
//          sub_1005f7d0(pObj, &nColor, &vXfm);
//      whose ONLY caller in the image is this site.  Its body (checked)
//      writes pObj->+0x97 into out[3] and either pObj->+0x94/95/96 or a
//      light-grid sample into out[2]/[1]/[0]: a BGRA dword.  It never reads
//      %ecx, so the third argument is dead in the callee but live at the
//      call site -- recorded, not resolved.
//
//  0x1006027e..0x1006043c  THE QUAD.  Four vertices written to
//      aQuad[pIdx[0..3]] -- the index is loaded from (%esi), 0x4(%esi),
//      0x8(%esi), 0xc(%esi) and multiplied by 0x20 with `imul $0x20`, so
//      the table is int32[4] and the vertex is 32 bytes.  Per vertex:
//          pV->Init(x, y, 0.0f);            0x1000eec0, thiscall, ret $0xc
//          pV->sub_1000d9d0(u, v);          0x1000d9d0, +0x18/+0x1c setter
//          pV->m_Diffuse10.m_n  = nColor;   +0x10
//          pV->m_Specular14.m_n = nSpecular;+0x14
//      in that order, and the corners are
//          0: (-fTexW,  fTexH)  (fU0, fV0)
//          1: ( fTexW,  fTexH)  (fU1, fV0)
//          2: ( fTexW, -fTexH)  (fU1, fV1)
//          3: (-fTexW, -fTexH)  (fU0, fV1)
//      -fTexW is CSEd into -0x58 by an `fstl` at 0x1006029b (used by
//      vertices 0 and 3) and -fTexH into -0x40 at 0x1006035c (vertices 2
//      and 3).  0x1000d9d0's body was read: `mov %eax,0x18(%ecx) ;
//      mov %edx,0x1c(%ecx) ; ret $0x8`.
//
//  0x1006043c..0x1006048f  THE PER-OBJECT PASS.
//          pVerts = aQuad;  nVerts = 4;
//          if (pObj->m_n1bc != -1)
//              if (!sub_1005fb70(pObj, pObj->m_n1bc, &pVerts, &nVerts,
//                                &nUnknown2b54))
//                  return;
//      preceded by sub_1005fac0(aQuad, 4, &mat) -- d3dr3c.cpp decoded that
//      one as "transform an array of vertices in place by a 4x4 with the
//      homogeneous divide", stride 0x20, and this is its ONLY caller.
//      0x1005fb70's five __cdecl slots are read off 0x1006046c..0x1006047c;
//      it too has exactly one caller.  Its fifth argument is the address
//      of a frame slot (-0x2b54) that THIS function never reads and never
//      writes; the callee copies the pointer straight into its own -0x50
//      at 0x1005fb84, so it is an out-parameter this caller discards.
//      That is as far as the bytes go -- what the callee puts there is not
//      claimed here.
//
//  0x1006048f..0x10060543  THE WORLD TRANSFORM (`m_nFlags88 & 4` arm).
//      A counted loop over nVerts applying the 3x4 at 0x1008b9dc, which is
//      g_1008b880 + 0x15c -- the transform object's view matrix that
//      d3dlow.cpp's sub_1000d9b0 forwards to 0x1000a770 for.  Here it is
//      SPELLED OUT, not called: there is no call in the loop, and
//      0x1000d9b0 lives in another translation unit so /Qip could not have
//      inlined it.  Each row associates from the last term backwards
//      (m[i][2]*z, then m[i][1]*y, then m[i][0]*x, then + m[i][3]), this
//      compiler's habit.  The three stores are software-pipelined: z at
//      0x10060534 in the body, x and y at 0x100604c0/0x100604c3 in the NEXT
//      iteration's head, with the last iteration's pair peeled to
//      0x1006053d.  -20.0f is materialised into a double slot (-0x88) at
//      the TOP of this arm (0x1006049e) although its only use is the
//      projection loop 5 KB later: icl hoisting a loop-invariant constant.
//      It is also the FIRST literal in this function's pool run
//      (0x100aaca0), which is what says the statement that mentions it is
//      not where the load is.
//
//  0x10060543..0x1006181e  THE CLIPPER.  Six Sutherland-Hodgman passes
//      against bits 0x01, 0x04, 0x08, 0x10, 0x20 and 0x02 of g_n1008b874
//      in that order, i.e. near / left / top / right / bottom / far, with
//      the tests
//          z >= g_f1008b908 | -z < x | y < z | x < z | -z < y
//          | z <= g_f1008b910
//      read off 0x100605c3, 0x100608d2, 0x10060bf0, 0x10060f03, 0x10061220
//      and 0x10061540.  This is big61a30.cpp's clipper with the SAME
//      geometry and the same 0xdc-per-pass frame stride: each pass's
//      inside-flag array is at -0x5d4, -0x4f8, -0x41c, -0x340, -0x264 and
//      -0x188, 0xdc = 220 apart, of which 0xc8 = 200 = FIFTY int32 is the
//      array and the remaining 0x14 the five int slots the five byte-lerps
//      need (-0x50c..-0x4fc for pass 1 and so on).  Fifty is read off the
//      frame, not assumed.  The five float temps of each pass live in a
//      SEPARATE run at -0x2d40 stepping 8, thirty of them, ending exactly
//      at -0x2c58 where the next named local (-0x2c54, nSpecular) begins.
//      Every pass bumps g_n1008be44.  The interpolation parameter is
//          t = (d < -0.00001f || d > 0.00001f) ? num / d : 0.0f
//      with the `> 0.00001f` half sunk below the epilogue in all six cases
//      (0x100618f9, 0x10061920, 0x10061950, 0x10061980, 0x100619b0,
//      0x100619e0); the epsilons were dumped from 0x100aacdc + 8k and
//      0x100aad0c + 8k.  The vertex lerp is 0x10023380's body INLINE (no
//      call to it anywhere in the range), field order +0x18, +0x1c, then
//      the bytes +0x12, +0x11, +0x10, +0x13, +0x17, each through
//      `fstps <float> ; flds <float> ; fistpl <int> ; mov <int>,%dl` --
//      the /QIfist bare conversion big61a30.cpp records for the same code.
//
//  0x1006181e..0x1006186c  THE PROJECTION.  One loop over nVerts:
//          fZ    = pVerts[i].z;
//          fBias = (-20.0 + fZ >= 7.0f) ? -20.0 : 7.0f - fZ;
//          sub_1005f9a0(&pVerts[i], &pVerts[i], fBias);
//      -20.0 comes from the double slot (fldl -0x88) and 7.0f from
//      0x100aacd4 / 0x100aacd8 (two DISTINCT pool slots, so two textual
//      7.0f).  The `>=` arm is the SUNK one at 0x10061a00, so the compare
//      is written in the shipped sense.  This differs from big61a30, which
//      computes the bias ONCE outside its loop from the sprite centre; here
//      `flds 0x8(%ebx)` is inside the loop and %ebx advances by 0x20.
//      0x1005f9a0's (dest, src, bias) is big61a30.cpp's and was re-read.
//
//  0x1006186c..0x100618f8  THE DRAW.
//          nDraw = 1;                                (0x1006186c)
//      or, for the `!(m_nFlags88 & 4)` arm sunk to 0x10061875,
//          nDraw = sub_10014550(&pVerts, &nVerts, g_1008b880);
//      then `if (nDraw)` DrawPrimitive(6, 0x1c4, pVerts, nVerts, 0) and the
//      two duplicated epilogues.  The reject path from any clip pass
//      (0x10061a10) is `nDraw = 0` joining the same test, which is why the
//      value is funnelled through %edx at 0x10061871.
//
//  ------------------------------------------------------------------- //
//  WHAT IS *NOT* PINNED -- read this before trusting any byte of it.
//  ------------------------------------------------------------------- //
//
//  * A 9,600-BYTE FRAME HOLE.  Every %ebp displacement the function
//    touches was collected: they run -0x8 .. -0x5d4 and then -0x2b54 ..
//    -0x2d40, with NOTHING between -0x2b54 and -0x5d4.  That is
//    0x2580 = 9,600 bytes of frame the code never addresses.  9,600 is
//    exactly 6 x 50 x 32 -- six clip passes, the fifty the flag arrays
//    prove, and the 32-byte vertex -- which would make it a per-pass output
//    buffer the shipped body does not use because it clips into the global
//    g_p1008b87c instead.  That arithmetic is an OBSERVATION, not a
//    decode; the hole is recorded as UNDECODED below and the
//    reconstruction does not reproduce the frame size.
//  * WHICH ACCESSOR THE SECOND +0x0c CALL IS.  See COIN-FLIPS.
//  * THE FPU SCHEDULE of the four uv corners and of the three transform
//    rows.  Statement order is pinned (pool, and the store order); the
//    interleaving is icl's.
//  * `(uint8)(int32)expr` rather than `(uint8)expr` at the thirty
//    float->byte sites, and the /QIfist bare `fistpl`: exactly as
//    big61a30.cpp records.  Those sites contain no calls.
//
//  ------------------------------------------------------------------- //
//  COIN-FLIPS, both stated with both spellings.
//  ------------------------------------------------------------------- //
//
//  1. THE TWO +0x0c CALLS.  Written below as GetWidth0c() twice, because
//     that is literally what the two `mov 0xc(%eax),%eax` bytes say.  The
//     alternative -- that the class in the original header has GetWidth at
//     +0x0c and GetHeight at +0x10 and this call site is a copy-paste bug
//     that reads the width for both -- is indistinguishable from the
//     bytes and does not change the emitted code.  If it matters: writing
//     GetHeight10() for the second call moves ONE indirect call's target,
//     which the call-sequence comparison matches by position only, so the acceptance test
//     cannot separate them either.  NOT a call-order flip.
//
//  2. THE `m_nFlags88 & 4` ARM ORDER -- and this one is NOT a coin flip,
//     because BOTH arms make calls.  The shipped sequence is
//     `call 0x1005f9a0` (the projection loop, 0x1006185e) BEFORE
//     `call 0x10014550` (0x10061893).  The spelling chosen below is
//         if (pObj->m_nFlags88 & 0x4) { ...projection... }
//         else                        { nDraw = sub_10014550(...); }
//     which emits the `&4` arm inline and sinks the `else`, giving
//     0x1005f9a0 then 0x10014550.  The other spelling,
//         if (!(pObj->m_nFlags88 & 0x4)) { nDraw = sub_10014550(...); }
//         else                          { ...projection... }
//     would emit 0x10014550 FIRST and predict one shipped-only /
//     one ours-only pair on those two direct calls.  Stated so one build
//     settles it.
//
// *********************************************************************** //
//
//  ------------------------------------------------------------------- //
//  THE ACCEPTANCE TEST, and the residual predicted in advance.
//  ------------------------------------------------------------------- //
//
//  The shipped side is 30 calls -- 26 direct and 4 indirect -- at 223
//  bytes per call, and the 26 direct reach SIXTEEN DISTINCT callees
//  (counted mechanically, not by eye)
//  (0x100805a0, 0x100158b0 x4, 0x100154b0, 0x10015320, 0x100151b0,
//  0x1000eec0 x5, 0x1005f790, 0x10015640, 0x10027f50, 0x10014940,
//  0x1005f7d0, 0x1000d9d0 x4, 0x1005fac0, 0x1005fb70, 0x1005f9a0,
//  0x10014550), which is the discrimination worth quoting
//  rather than the raw ratio.
//
//  EXPECTED: 30 shipped, 29 ours, 29 in agreement (25 by resolved target
//  plus 4 indirect by position), ONE shipped-only and ZERO ours-only.
//
//  The one shipped-only is 0x100805a0, _chkstk -- `mov $0x2d64,%eax ;
//  call 0x100805a0` at 0x10060016.  This reconstruction does not carry
//  the 9,600-byte frame hole (see WHAT IS NOT PINNED), so its frame is
//  under a page and no probe is emitted at all; and even with one, icl
//  emits its OWN `__chkstk` symbol, which the call-sequence comparison cannot resolve to
//  0x100805a0.  big124a4.cpp records the same residual for the same
//  reason.  It is the known toolchain gap, not a decode error.
//
//  NOT expected here, and worth saying so: no CRT-intrinsic gap (the
//  function calls neither sqrt nor fabs nor tan directly -- 0x100154b0
//  calls sqrt, but that is inside the callee), and no /Qip f+4 offset
//  (every direct target is a plain function entry that this file names by
//  its own address).
//
//  WHAT THE TEST CANNOT SEE, stated with the figure:
//    * the 4,827 bytes from 0x10060543 to 0x1006181e -- the six clip
//      passes -- contain NOT ONE call, direct or indirect.  That is 72%
//      of the function, and its correctness rests on the frame
//      decomposition (0xdc per pass, 0xc8 = fifty int32 plus five int
//      slots, and thirty float temps at -0x2d40 step 8 ending exactly at
//      -0x2c58), on the six plane tests read one at a time, and on the
//      instruction-for-instruction agreement with big61a30.cpp's already
//      decoded twin -- not on the call sequence.
//    * the 165 bytes from 0x1006049e to 0x10060543 -- the world transform
//      loop -- are likewise call-free.
//    * the ONE genuinely undecoded thing is the 0x2580-byte frame hole,
//      which contains no code at all.
//
//  ------------------------------------------------------------------- //
//  THE CONSTANT POOL, dumped and read in order.  0x100aaca0..0x100aad38 is
//  this function's run: the function before it (0x1005fb70) touches only
//  0x100aac14, 0x100aac18 and 0x100aac20, and the function after it
//  (0x10061a30) starts at 0x100aad3c.  In pool order:
//
//    100aaca0  -20.0f    the projection bias  (see the note below)
//    100aaca4    2.0f    fU0 = invW * 2.0f            <- used at 0x10060132
//    100aaca8    2.0f    fU1 = (fTexW - 2.0f) * invW  <- used at 0x10060109
//    100aacac    2.0f    fV0 = invH * 2.0f            <- used at 0x1006013e
//    100aacb0    2.0f    fV1 = (fTexH - 2.0f) * invH  <- used at 0x10060118
//    100aacb4    0.0f  \
//    100aacb8    0.0f   > vDir.Init(0.0f, 0.0f, -1.0f), all three FOLDED
//    100aacbc   -1.0f  /  to immediates at 0x1006017f..0x10060192
//    100aacc0    0.0f    the Dot compare                 (loaded)
//    100aacc4    0.0f  \
//    100aacc8    0.0f   \ the z argument of the FOUR vertex Inits, folded
//    100aaccc    0.0f   / to `movl $0x0,0x8(%esp)` at 0x100602a5,
//    100aacd0    0.0f  /  0x100602fe, 0x1006036e, 0x100603c8
//    100aacd4    7.0f    the projection compare          (loaded)
//    100aacd8    7.0f    the projection subtract         (loaded)
//    100aacdc..  (-0.00001f, 1.0f) x 6, then (+0.00001f, 0.0f) x 6
//    100aad38            -- the six clip passes, all twelve 1.0f dead
//
//  Two readings come out of that and both are used below:
//    * the FOUR 2.0f are four distinct slots in the order u0, u1, v0, v1
//      while the code emits u1, v1, u0, v0 -- so the pool, not the
//      schedule, gives the statement order;
//    * the FOUR 0.0f at 100aacc4..100aacd0 are the four vertex Inits'
//      third argument, never loaded, which independently confirms that the
//      quad is built with four separate Init(x, y, 0.0f) calls and not
//      with one loop or with direct field stores.
//
//  Two things the pool does NOT explain, recorded rather than argued away:
//    * -20.0f is FIRST in the run although its only load is 5 KB into the
//      function.  It is also the one constant icl parks in a spill slot
//      (`flds 0x100aaca0 ; fstpl -0x88(%ebp)` at 0x1006049e) rather than
//      re-loading, which is presumably why it is allocated early.  The
//      SECOND textual -20.0f of the ternary needs no pool slot because the
//      sunk arm at 0x10061a02 loads that same spill.
//    * the twelve clip-pass epsilons come AFTER the projection's two 7.0f
//      even though the clipper runs first.  big61a30.cpp's pool run has
//      exactly the same shape (its three 7.0f at 0x100aad70..0x100aad78
//      precede its epsilon runs at 0x100aad7c), so this is how icl orders
//      a clip macro's constants in this family and not a fact about this
//      function's source order.
//
// *********************************************************************** //

#include "ltbasedefs.h"
#include <math.h>


// ---------------------------------------------------------------------- //
//  The D3D7 device, exactly as d3dbatcha.cpp declares it.
// ---------------------------------------------------------------------- //
struct D3DDevice;

struct D3DDeviceVtbl
{
	uint8			m_pad_00[0x50];
	long (__stdcall* SetRenderState)(D3DDevice*, uint32, uint32);         // +0x50
	long (__stdcall* GetRenderState)(D3DDevice*, uint32, uint32*);        // +0x54
	uint8			m_pad_58[0x64 - 0x58];
	long (__stdcall* DrawPrimitive)(D3DDevice*, uint32, uint32, void*,
	                                uint32, uint32);                      // +0x64
	uint8			m_pad_68[0x8c - 0x68];
	long (__stdcall* SetTexture)(D3DDevice*, uint32, void*);              // +0x8c
};

struct D3DDevice
{
	D3DDeviceVtbl*	lpVtbl;
};

extern D3DDevice* g_pD3DDevice;                             // 0x1008d988


// IDirectDrawSurface7: only +0xbc, SetLOD, is reached from here.
struct DDSurface;

struct DDSurfaceVtbl
{
	uint8			m_pad_00[0xbc];
	long (__stdcall* SetLOD)(DDSurface*, uint32);                         // +0xbc
};

struct DDSurface
{
	DDSurfaceVtbl*	lpVtbl;
};


// ---------------------------------------------------------------------- //
//  0x1000adc0 -- d3dbatcha.cpp's scoped render-state saver, declared here
//  with the SAME class name and the SAME member signature so the external
//  reference resolves to that object's symbol.  The destructor is written
//  out by hand at the tail of the function instead of being a real one:
//  see the banner.
// ---------------------------------------------------------------------- //
class C1000adc0
{
public:
	C1000adc0*	sub_1000adc0(uint32 nState, uint32 nValue);

	uint32		m_nState00;                             // +0x00
	uint32		m_nOld04;                               // +0x04
};


// ---------------------------------------------------------------------- //
//  The 32-byte vertex.  FVF 0x1c4 = XYZRHW | DIFFUSE | SPECULAR | TEX1,
//  read straight off the DrawPrimitive at 0x10064307.
// ---------------------------------------------------------------------- //
union VColor61a30
{
	uint32		m_n;
	uint8		m_b[4];
};

struct Vert61a30
{
	float		x;                                      // +0x00
	float		y;                                      // +0x04
	float		z;                                      // +0x08
	float		rhw;                                    // +0x0c
	VColor61a30	m_Diffuse10;                            // +0x10
	VColor61a30	m_Specular14;                           // +0x14
	float		tu;                                     // +0x18
	float		tv;                                     // +0x1c
};


// ---------------------------------------------------------------------- //
//  ltlink.h's CheapLTLink, as the record's +0x1c member.
// ---------------------------------------------------------------------- //
struct Link61a30
{
	Link61a30*	m_pPrev;                                // +0x00
	Link61a30*	m_pNext;                                // +0x04
	void*		m_pData;                                // +0x08
};


// ---------------------------------------------------------------------- //
//  The per-stage texture record.  Slots +0x0c and +0x10 of its vtable are
//  called with %ecx = this and nothing pushed at 0x10061e93 / 0x10061ec8,
//  and both results feed `fildll` of a 64-bit temp whose high half is
//  zeroed -- an UNSIGNED width and height.
// ---------------------------------------------------------------------- //
class Tex61a30
{
public:
	virtual void	Slot00();
	virtual void	Slot04();
	virtual void	Slot08();
	virtual uint32	GetWidth0c();                       // vtable +0x0c
	virtual uint32	GetHeight10();                      // vtable +0x10

	float		m_fInvW04;                              // +0x04
	float		m_fInvH08;                              // +0x08
	DDSurface*	m_pSurface0c;                           // +0x0c
	uint32		m_nBytes10;                             // +0x10
	uint16		m_nStamp14;                             // +0x14
	uint16		m_nAlphaRef16;                          // +0x16
	uint8		m_pad_18[0x1c - 0x18];
	Link61a30	m_Link1c;                               // +0x1c
	uint8		m_pad_28[0x30 - 0x28];
	Tex61a30*	m_pNext30;                              // +0x30
	uint8		m_pad_34[0x42 - 0x34];
	uint16		m_nStage42;                             // +0x42
	uint16		m_nDirty44;                             // +0x44
};

// The block the sprite's +0x1b0 points at: a list head at +0x0c and a
// sixteen-bit stamp at +0x30.
struct TexBlock61a30
{
	uint8		m_pad_00[0x0c];
	Tex61a30*	m_pHead0c;                              // +0x0c
	uint8		m_pad_10[0x30 - 0x10];
	uint16		m_nStamp30;                             // +0x30
};


// ---------------------------------------------------------------------- //
//  The sprite object.  +0x88, +0x94..+0x97, +0x18c and +0x1a8 are the SAME
//  class d3dr3d.cpp's sub_1001e210 takes, which is why that call can be
//  handed `pObj` unchanged.
// ---------------------------------------------------------------------- //
class Obj61a30
{
public:
	LTVector*	sub_100158b0();                         // 0x100158b0

	uint8		m_pad_00[0x88];
	uint32		m_nFlags88;                             // +0x88
	uint8		m_pad_8c[0x94 - 0x8c];
	uint8		m_nR94;                                 // +0x94
	uint8		m_nG95;                                 // +0x95
	uint8		m_nB96;                                 // +0x96
	uint8		m_nA97;                                 // +0x97
	uint8		m_pad_98[0xac - 0x98];
	float		m_fScaleXac;                            // +0xac
	float		m_fScaleYb0;                            // +0xb0
	uint8		m_pad_b4[0x18c - 0xb4];
	LTVector	m_vPos18c;                              // +0x18c
	uint8		m_pad_198[0x1ac - 0x198];
	void*		m_p1ac;                                 // +0x1ac
	TexBlock61a30** m_ppBlock1b0;                       // +0x1b0
};


// ---------------------------------------------------------------------- //
//  The context the ONE caller passes: the fixed global at 0x1008ff60.
//  Two 3x4 row-major transforms and two scale factors.
// ---------------------------------------------------------------------- //
struct Ctx61a30
{
	uint8		m_pad_00[0x94];
	float		m_fScaleX94;                            // +0x94
	float		m_fScaleY98;                            // +0x98
	uint8		m_pad_9c[0x15c - 0x9c];
	float		m_m15c[3][4];                           // +0x15c
	uint8		m_pad_18c[0x2dc - 0x18c];
	float		m_m2dc[3][4];                           // +0x2dc
};


// ---------------------------------------------------------------------- //
//  The world light grid, field for field big19d64.cpp's Src19d64Grid.
// ---------------------------------------------------------------------- //
struct GridCell61a30
{
	uint8		m_b0;
	uint8		m_b1;
	uint8		m_b2;
	uint8		m_b3;
};

struct Grid61a30
{
	uint8			m_pad_00[0xfc];                     // +0x00
	GridCell61a30*	m_pCellsfc;                         // +0xfc
	uint8			m_pad_100[0x104 - 0x100];           // +0x100
	int32			m_nStrideY104;                      // +0x104
	uint8			m_pad_108[0x110 - 0x108];           // +0x108
	int32			m_nMaxX110;                         // +0x110
	int32			m_nMaxY114;                         // +0x114
	int32			m_nMaxZ118;                         // +0x118
	int32			m_nStrideZ11c;                      // +0x11c
	uint8			m_pad_120[0x12c - 0x120];           // +0x120
	float			m_fScale12c;                        // +0x12c
	float			m_fScale130;                        // +0x130
	float			m_fScale134;                        // +0x134
	LTVector		m_vMin138;                          // +0x138
};


// The pair published per stage at 0x1008e664: `mov %edx,0x1008e668(,%ecx,8)`
// and `mov %edi,0x1008e664(,%ecx,8)` write it, and 0x10061f29/0x10061f35
// read element ZERO of it with the index folded away.
struct TexScale61a30
{
	float		m_fInvW00;                              // +0x00
	float		m_fInvH04;                              // +0x04
};


// 0x1008cbcc: only +0x48 is touched here (`add %edx,0x48(%eax)`).
struct Stats61a30
{
	uint8		m_pad_00[0x48];
	uint32		m_nBytes48;                             // +0x48
};


// ---------------------------------------------------------------------- //
//  Externals.  Relocations are masked, so only the shapes matter.
// ---------------------------------------------------------------------- //
typedef void (__fastcall* PFN_1008ce8c)(void* pA, void* pB);

extern uint16			g_w1008b870;                    // 0x1008b870
extern uint32			g_n1008b874;                    // 0x1008b874
extern Vert61a30*		g_p1008b87c;                    // 0x1008b87c
extern float			g_f1008b908;                    // 0x1008b908
extern float			g_f1008b910;                    // 0x1008b910
extern uint32			g_n1008bd80;                    // 0x1008bd80
extern uint32			g_n1008be44;                    // 0x1008be44
extern uint16			g_w1008bea8;                    // 0x1008bea8
extern Grid61a30*		g_p1008beac;                    // 0x1008beac
extern Stats61a30*		g_p1008cbcc;                    // 0x1008cbcc
extern PFN_1008ce8c		g_pfn_1008ce8c;                 // 0x1008ce8c
extern void*			g_p1008d7a0;                    // 0x1008d7a0
extern uint32			g_n1008d7d0;                    // 0x1008d7d0
extern Link61a30*		g_p1008e634;                    // 0x1008e634
extern TexScale61a30	g_a1008e664[];                  // 0x1008e664
extern Tex61a30*		g_a1008e684[];                  // 0x1008e684

extern "C" void  sub_1005a490(Obj61a30* pObj, uint32* pSrcBlend,
                              uint32* pDestBlend, uint32* pFogEnable,
                              uint32* pFogColor);              // 0x1005a490
extern "C" Tex61a30* sub_10039bd0(TexBlock61a30* pBlock, uint32 nStage,
                                  uint32 bLink);               // 0x10039bd0
extern "C" void  sub_1001e210(Obj61a30* pObject, LTVector* pColor);
                                                               // 0x1001e210
extern "C" void  sub_1005f9a0(Vert61a30* pDest, Vert61a30* pSrc, float fBias);
                                                               // 0x1005f9a0
extern "C" void  sub_1005fa80(Vert61a30* pVerts, uint32 nCount);
                                                               // 0x1005fa80






// ---------------------------------------------------------------------- //
//  The SDK vector, with the four out-of-line helpers this function
//  reaches.  All four are DECLARED AND NOT DEFINED here on purpose: they
//  are two- to twenty-instruction bodies that the shipped build did not
//  inline, and writing them as field arithmetic would emit no call at all.
//
//    0x100158b0  `lea 0x18c(%ecx),%eax`      -- &Obj::m_vPos18c
//    0x100154b0  Dist, ltvector.h:36         -- `ret $0x4`, %ecx = this
//    0x10015640  operator-, by-value arg     -- `ret $0x10`
//    0x10027f50  Dot, by-value arg           -- 31 bytes
//    0x1000eec0  Init(x,y,z) out of line     -- `ret $0xc`
// ---------------------------------------------------------------------- //
struct LTVec60010
{
	float	x;
	float	y;
	float	z;

	void		sub_1000eec0(float fX, float fY, float fZ);   // 0x1000eec0
	float		sub_100154b0(const LTVec60010& v) const;      // 0x100154b0
	LTVec60010	sub_10015640(const LTVec60010 v) const;       // 0x10015640
	float		sub_10027f50(LTVec60010 v) const;             // 0x10027f50
};


// ---------------------------------------------------------------------- //
//  The 32-byte vertex.  FVF 0x1c4 = XYZRHW | DIFFUSE | SPECULAR | TEX1,
//  read straight off the DrawPrimitive at 0x100618d8; every stride in the
//  function is 0x20 (`shl $0x5` at 0x1006059c, `imul $0x20` at 0x10060286,
//  `add $0x20` at 0x100605d0, `sar $0x5` at 0x10060883).
//
//  sub_1000eec0 is declared a SECOND time here, on the vertex, because
//  0x100602ad and its three twins call it with %ecx = &aQuad[k], i.e. on
//  the vertex's first twelve bytes.  Same address, two mangled names.
//  sub_1000d9d0 is 0x1000d9d0, whose whole body is
//  `mov %eax,0x18(%ecx) ; mov %edx,0x1c(%ecx) ; ret $0x8` -- the tu/tv
//  setter, and d3dbatchb_os.cpp already records it as the two-field
//  thiscall setter with thirteen callers.
// ---------------------------------------------------------------------- //
union VColor60010
{
	uint32	m_n;
	uint8	m_b[4];
};

struct Vert60010
{
	float		x;                                  // +0x00
	float		y;                                  // +0x04
	float		z;                                  // +0x08
	float		rhw;                                // +0x0c
	VColor60010	m_Diffuse10;                        // +0x10
	VColor60010	m_Specular14;                       // +0x14
	float		tu;                                 // +0x18
	float		tv;                                 // +0x1c

	void	sub_1000eec0(float fX, float fY, float fZ); // 0x1000eec0
	void	sub_1000d9d0(float fU, float fV);           // 0x1000d9d0
};


// ---------------------------------------------------------------------- //
//  The per-stage texture record.  Only vtable slot +0x0c is reached from
//  here, and it is reached TWICE (0x100600a6, 0x100600d2 -- the two
//  `8b 40 0c` were checked one at a time).  Both results go through
//  `mov %eax,-0xa8 ; movl $0x0,-0xa4 ; fildll`, the unsigned-32-to-double
//  sequence, so the slot returns uint32.  big61a30.cpp's twin of this
//  class has GetWidth at +0x0c and GetHeight at +0x10.
// ---------------------------------------------------------------------- //
class Tex60010
{
public:
	virtual void	Slot00();
	virtual void	Slot04();
	virtual void	Slot08();
	virtual uint32	GetWidth0c();                       // vtable +0x0c
	virtual uint32	GetHeight10();                      // vtable +0x10
};


// The pair of reciprocals published per stage; this function reads index
// ZERO of it -- `fmuls 0x1008e664` / `fmuls 0x1008e668` with no index
// register, four times.
struct TexScale60010
{
	float	m_fInvW;                                    // +0x00
	float	m_fInvH;                                    // +0x04
};


// ---------------------------------------------------------------------- //
//  The object.  Offsets read off this function only:
//    +0x88  the flag word            (0x1006048f, `test $0x4,%bl`)
//    +0x94..+0x97  the colour bytes  (used by the callee 0x1005f7d0)
//    +0x9c  a quaternion             (0x1006016a, 0x100151b0's arg1)
//    +0xac  a scale vector           (0x1006015c, 0x100151b0's arg2);
//           its .x and .y are the caller's dead fScaleX/fScaleY
//    +0x18c the position             (0x100158b0 = `lea 0x18c(%ecx)`)
//    +0x1bc an int compared with -1  (0x1006044c)
//  Every one of them is also in big61a30.cpp's Obj61a30, at the same
//  offset, which is the cross-check.
// ---------------------------------------------------------------------- //
class Obj60010
{
public:
	LTVec60010*	sub_100158b0();                         // 0x100158b0

	uint8		m_pad_00[0x88];
	uint32		m_nFlags88;                             // +0x88
	uint8		m_pad_8c[0x9c - 0x8c];
	float		m_Rot9c[4];                             // +0x9c
	LTVec60010	m_vScaleac;                             // +0xac
	uint8		m_pad_b8[0x18c - 0xb8];
	LTVec60010	m_vPos18c;                              // +0x18c
	uint8		m_pad_198[0x1bc - 0x198];
	int32		m_n1bc;                                 // +0x1bc
};


// ---------------------------------------------------------------------- //
//  The context, the same 0x1008ff60 big61a30.cpp's caller passes through.
//  Two offsets are used here: the 3x4 at +0x15c (`add $0x15c,%edi` at
//  0x1006024f) and a position at +0x4b4 (`lea 0x4b4(%edi)` at 0x10060050
//  and 0x100601b6).  +0x15c is the same 3x4 big61a30.cpp names m_m15c.
// ---------------------------------------------------------------------- //
struct Ctx60010
{
	uint8		m_pad_00[0x15c];
	float		m_m15c[3][4];                           // +0x15c
	uint8		m_pad_18c[0x4b4 - 0x18c];
	LTVec60010	m_v4b4;                                 // +0x4b4
};


// The block reached only as sub_10015320's first argument.
struct TexBlock60010;


// ---------------------------------------------------------------------- //
//  The transform object at 0x1008b880.  Declared as a struct here rather
//  than big124a4.cpp's `uint8[]` because this function needs BOTH ends of
//  it: the whole object goes to sub_10014550 (0x10061884 pushes the bare
//  address) and the 3x4 at +0x15c is read field by field by the world
//  transform loop -- 0x1008b9dc is 0x1008b880 + 0x15c, and the twelve
//  floats 0x1008b9dc..0x1008ba08 are exactly m[0][0]..m[2][3].
// ---------------------------------------------------------------------- //
struct XForm60010
{
	uint8		m_pad_00[0x15c];
	LTMatrix	m_Mat15c;                               // +0x15c
	uint8		m_pad_19c[0x400 - 0x19c];
};


// ---------------------------------------------------------------------- //
//  The D3D7 device, exactly as d3dbatcha.cpp and big61a30.cpp declare it.
// ---------------------------------------------------------------------- //
struct D3DDevice60010;

struct D3DDeviceVtbl60010
{
	uint8	m_pad_00[0x64];
	long (__stdcall* DrawPrimitive)(D3DDevice60010*, uint32 nType,
	                                uint32 nFVF, void* pVerts,
	                                uint32 nCount, uint32 nFlags); // +0x64
};

struct D3DDevice60010
{
	D3DDeviceVtbl60010*	lpVtbl;
};


// ---------------------------------------------------------------------- //
//  Globals.  Relocations are masked, so only the shapes matter.
// ---------------------------------------------------------------------- //
typedef void (__fastcall* PFN_1008ce8c_60010)(const LTVec60010* pPos,
                                              uint32* pnOut);

extern uint32			g_n1008b874;                    // 0x1008b874
extern float			g_f1008b908;                    // 0x1008b908
extern float			g_f1008b910;                    // 0x1008b910
extern uint32			g_n1008be44;                    // 0x1008be44
extern void*			g_p1008d7a0;                    // 0x1008d7a0
extern uint32			g_n1008d7d0;                    // 0x1008d7d0
extern XForm60010		g_1008b880;                     // 0x1008b880

// The two winding tables, dumped from the image: {3,2,1,0} and {0,1,2,3}.
extern const int32		g_a100897e0[4];                 // 0x100897e0
extern const int32		g_a100897f0[4];                 // 0x100897f0


// ---------------------------------------------------------------------- //
//  Callees.  All __cdecl except where noted; the three that take their
//  arguments in registers under /Qip are declared __cdecl --
//  they are still one call each, in the same place.
// ---------------------------------------------------------------------- //

// 0x10015320 -- big5a960.cpp: (pointer, stage, flag), returns the bound
// record or NULL.
extern "C" void*  sub_10015320(TexBlock60010* pBlock, uint32 nStage,
                               uint32 nFlags);

// 0x100151b0 -- d3dbatchd.cpp / big17c14.cpp: (position, quaternion,
// scale, matrix).  Four __cdecl slots, read off 0x10060162..0x10060174.
extern "C" void   sub_100151b0(LTVec60010* pPos, float* pRot,
                               LTVec60010* pScale, float* pMat);

// 0x1005f790 -- d3dr2d.cpp: ltmatrix.h:274 MatVMul_InPlace_3x3(mat, vec).
extern "C" void   sub_1005f790(float* pMat, LTVec60010* pVec);

// 0x10014940 -- d3dr2c_os.cpp: MatVMul(dest, mat, src), CALLED not inlined.
extern "C" void   sub_10014940(LTVec60010* pDest, const float* pMat,
                               const LTVec60010* pSrc);

// 0x1005f7d0 -- a sprite's vertex colour (moved in from
// w8_1005f7d0.cpp).  This call site is its ONLY caller in the image, and it hands
// it %eax = the object, %edx = a BGRA out-parameter, %ecx a third pointer
// its body never reads: the /Qip register entry of an internal-linkage
// callee, which icl gives only to a static in its caller's translation
// unit.  Defined below.
struct LTVec;
struct C1001e210;

class Obj1005f7d0
{
public:
	uint8		m_pad_00[0x88];
	uint32		m_nFlags88;                             // +0x88
	uint8		m_pad_8c[0x94 - 0x8c];
	uint8		m_nR94;                                 // +0x94
	uint8		m_nG95;                                 // +0x95
	uint8		m_nB96;                                 // +0x96
	uint8		m_nA97;                                 // +0x97
	uint8		m_pad_98[0x18c - 0x98];
	float		m_vPos18c[3];                           // +0x18c
};


extern void sub_10014b60(void* pGrid, LTVec* pPos, uint8* pOutRGB);

// ======================================================================= //
//  0x1005f7d0  432 bytes.  Writes a BGRA dword: alpha is the object's
//  +0x97.  When the object's flags (+0x88) have 0x200 set, or there is no
//  light grid (0x1008beac), the colour is the object's own +0x94/+0x95/
//  +0x96.  Otherwise each channel is object colour + dynamic light +
//  light-grid sample, clamped to 0..255: the grid is sampled at the
//  object's position (+0x18c) by 0x10014b60 into a local dword, the
//  dynamic lights are summed by 0x1001e210 into a local vector.
//
//  The three float-to-byte conversions are `fstps tmp ; flds tmp ;
//  fistpl n`: RoundF, the __asm `fld ; fistp` helper icl inlines with its
//  float argument materialised in its own slot (w1_10001000.cpp's
//  precedent, out of line at 0x10015190).
//
//  NEAR, 8 bytes (586 -> 8; re-measured under the new file name).
//  The early return is an if/else so the shared epilogue is jumped to as
//  shipped, and g_p1008beac is read a second time (a one-site volatile)
//  as the shipped code does.  Residual: the two double spill slots are
//  swapped -- shipped spills clamped G to -0x10 and clamped B to -0x8,
//  ours the reverse (R stays on the x87 stack in both).  No effect:
//  declaration order (fB,fG,fR / fG,fB,fR / fB,fR,fG), separate
//  clamped locals, the clamp as one expression around the sum, a ClampF
//  inline, an LTVector or array, scope.  Worse: the clamps inside the
//  RoundF calls (239), the stores in B,G,R order (171), if/else chains
//  instead of LTCLAMP (183), the inverted condition with the plain
//  colour in the else (348), double locals (26).
//  Still 8 with fR/fG/fB declared one per line, the RoundF
//  results named (int32 nR/nG/nB), G and B in an inner block, and the
//  definition moved after 0x10060010 (a static prototype here).  Worse:
//  all three sums before the three clamps (260; clamps B,G,R 178), the
//  channels computed B,G,R (141), B's clamp as an if/else (137).  The
//  RoundF argument and result slots are allocated in program order
//  (R lowest); the two double spills are not, in ours.
// ======================================================================= //
inline int32 RoundF(float f)
{
	int32 n;
	__asm fld f
	__asm fistp n
	return n;
}

static void sub_1005f7d0(Obj1005f7d0* pObj, uint32* pnColor,
                         const void* pXfmPos)
{
	uint8*		pOut = (uint8*)pnColor;
	uint8		sample[4];
	LTVector	vLight;
	float		fR, fG, fB;

	pOut[3] = pObj->m_nA97;

	if ((pObj->m_nFlags88 & 0x200) || !g_p1008beac)
	{
		pOut[2] = pObj->m_nR94;
		pOut[1] = pObj->m_nG95;
		pOut[0] = pObj->m_nB96;
	}
	else
	{
		sub_10014b60(*(uint8* volatile*)&g_p1008beac + 0xfc, (LTVec*)pObj->m_vPos18c, sample);
		sub_1001e210((Obj61a30*)pObj, &vLight);

		fR = (float)pObj->m_nR94 + vLight.x + (float)sample[2];
		fR = LTCLAMP(fR, 0.0f, 255.0f);
		fG = (float)pObj->m_nG95 + vLight.y + (float)sample[1];
		fG = LTCLAMP(fG, 0.0f, 255.0f);
		fB = (float)pObj->m_nB96 + vLight.z + (float)sample[0];
		fB = LTCLAMP(fB, 0.0f, 255.0f);

		pOut[2] = (uint8)RoundF(fR);
		pOut[1] = (uint8)RoundF(fG);
		pOut[0] = (uint8)RoundF(fB);
	}
}

// 0x1005fac0 -- d3dr3c.cpp: transform nCount 32-byte vertices in place by
// a 4x4 WITH the homogeneous divide.  This call site is its only caller.
extern "C" void   sub_1005fac0(Vert60010* pVerts, uint32 nCount,
                               float* pMat);

// 0x1005fb70 -- 1,184 bytes, only caller.  Five __cdecl slots read off
// 0x1006046c..0x1006047c; returns zero to abandon the draw.
extern "C" int32  sub_1005fb70(Obj60010* pObj, int32 nIndex,
                               Vert60010** ppVerts, int32* pnVerts,
                               Vert60010* pOut);

// 0x1005f9a0 -- big61a30.cpp: (dest, src, bias), the screen projection.

// 0x10014550 -- d3dr2c.cpp, via big124a4.cpp: a __cdecl forwarder to
// 0x1000f2c0 with a fourth argument of 0.
extern "C" int32  sub_10014550(Vert60010** ppVerts, int32* pnVerts,
                               XForm60010* pXForm);


// ---------------------------------------------------------------------- //
//  0x10023380 -- the vertex LERP, INLINE here: there is no call to it
//  anywhere in the 6,688 bytes, only six copies of its body.  Field order
//  is the shipped one, +0x18, +0x1c, then +0x12, +0x11, +0x10, +0x13,
//  +0x17, each byte through `fstps <float slot> ; flds <float slot> ;
//  fistpl <int slot> ; mov <int slot>,%dl` -- the /QIfist bare conversion,
//  identical to big61a30.cpp's twelve copies.
// ---------------------------------------------------------------------- //
// (the five bytes through RoundF, as the helper above says
// they should be -- 8,976 -> 6,880 bytes against the shipped 6,688.  The
// shipped 0x10060010 is a register-only /Qip function (`push %ecx ; push
// %edx ; push %eax` save the three register arguments) whose one caller,
// 0x10061b64, is big61a30.cpp's 0x10061a30: the two belong in one TU.)
inline void sub_10023380(Vert60010* a, Vert60010* b, Vert60010* dst, float t)
{
	dst->tu = (b->tu - a->tu) * t + a->tu;
	dst->tv = (b->tv - a->tv) * t + a->tv;

	dst->m_Diffuse10.m_b[2]  = (uint8)RoundF((b->m_Diffuse10.m_b[2]  - a->m_Diffuse10.m_b[2])  * t + a->m_Diffuse10.m_b[2]);
	dst->m_Diffuse10.m_b[1]  = (uint8)RoundF((b->m_Diffuse10.m_b[1]  - a->m_Diffuse10.m_b[1])  * t + a->m_Diffuse10.m_b[1]);
	dst->m_Diffuse10.m_b[0]  = (uint8)RoundF((b->m_Diffuse10.m_b[0]  - a->m_Diffuse10.m_b[0])  * t + a->m_Diffuse10.m_b[0]);
	dst->m_Diffuse10.m_b[3]  = (uint8)RoundF((b->m_Diffuse10.m_b[3]  - a->m_Diffuse10.m_b[3])  * t + a->m_Diffuse10.m_b[3]);
	dst->m_Specular14.m_b[3] = (uint8)RoundF((b->m_Specular14.m_b[3] - a->m_Specular14.m_b[3]) * t + a->m_Specular14.m_b[3]);
}


// The frame says fifty: 0xdc between consecutive passes, of which 0xc8 =
// 200 bytes = 50 int32 is the flag array and 0x14 the five int slots.
#define CLIP60010_MAX	50


// ---------------------------------------------------------------------- //
//  THE CLIPPER IS d3dr3c_os.cpp's 0x1000f4b0 INLINED over
//  its six plane bodies, as w38_10028030_qip.cpp has it for 0x1002803c
//, over this function's 32-byte vertex: the driver's nClipFlags is
//  the memory-resident -0x7c, its pOut -0x90/-0x80, its pVerts/nVerts
//  copies; the caller's pVerts/nVerts are -0x2b5c/-0x2b58.
// ---------------------------------------------------------------------- //
struct Vec60010c
{
	float		x, y, z;
};

#define CLIP60010_EPSILON	0.00001f

static inline float ClipEdgeNear_60010(Vec60010c* a, Vec60010c* b, Vec60010c* dst)
{
	float fDenom = b->z - a->z;
	float t;

	if (fDenom < -CLIP60010_EPSILON || fDenom > CLIP60010_EPSILON)
		t = (g_f1008b908 - a->z) / fDenom;
	else
		t = 0.0f;

	dst->x = (b->x - a->x) * t + a->x;
	dst->y = (b->y - a->y) * t + a->y;
	dst->z = g_f1008b908;

	return t;
}


static inline float ClipEdgeLeft_60010(Vec60010c* a, Vec60010c* b, Vec60010c* dst)
{
	float fDenom = (b->x - a->x) + b->z - a->z;
	float t;
	float fZ;

	if (fDenom < -CLIP60010_EPSILON || fDenom > CLIP60010_EPSILON)
		t = -(a->x + a->z) / fDenom;
	else
		t = 0.0f;

	dst->y = (b->y - a->y) * t + a->y;
	fZ = (b->z - a->z) * t + a->z;
	dst->z = fZ;
	dst->x = -fZ;

	return t;
}


static inline float ClipEdgeTop_60010(Vec60010c* a, Vec60010c* b, Vec60010c* dst)
{
	float fDenom = (b->y - a->y) - b->z + a->z;
	float t;
	float fZ;

	if (fDenom < -CLIP60010_EPSILON || fDenom > CLIP60010_EPSILON)
		t = -(a->y - a->z) / fDenom;
	else
		t = 0.0f;

	dst->x = (b->x - a->x) * t + a->x;
	fZ = (b->z - a->z) * t + a->z;
	dst->z = fZ;
	dst->y = fZ;

	return t;
}


static inline float ClipEdgeRight_60010(Vec60010c* a, Vec60010c* b, Vec60010c* dst)
{
	float fDenom = (b->x - a->x) - b->z + a->z;
	float t;
	float fZ;

	if (fDenom < -CLIP60010_EPSILON || fDenom > CLIP60010_EPSILON)
		t = -(a->x - a->z) / fDenom;
	else
		t = 0.0f;

	dst->y = (b->y - a->y) * t + a->y;
	fZ = (b->z - a->z) * t + a->z;
	dst->z = fZ;
	dst->x = fZ;

	return t;
}


static inline float ClipEdgeBottom_60010(Vec60010c* a, Vec60010c* b, Vec60010c* dst)
{
	float fDenom = (b->y - a->y) + b->z - a->z;
	float t;
	float fZ;

	if (fDenom < -CLIP60010_EPSILON || fDenom > CLIP60010_EPSILON)
		t = -(a->y + a->z) / fDenom;
	else
		t = 0.0f;

	dst->x = (b->x - a->x) * t + a->x;
	fZ = (b->z - a->z) * t + a->z;
	dst->z = fZ;
	dst->y = -fZ;

	return t;
}


static inline float ClipEdgeFar_60010(Vec60010c* a, Vec60010c* b, Vec60010c* dst)
{
	float fDenom = b->z - a->z;
	float t;

	if (fDenom < -CLIP60010_EPSILON || fDenom > CLIP60010_EPSILON)
		t = (g_f1008b910 - a->z) / fDenom;
	else
		t = 0.0f;

	dst->x = (b->x - a->x) * t + a->x;
	dst->y = (b->y - a->y) * t + a->y;
	dst->z = g_f1008b910;

	return t;
}


// 0x10001000
// POutStart is read from *ppOut ABOVE the classify loop in
// this inline-only copy of plane 1 (the lead: the shipped plane 1 takes
// pOutStart from the driver's pOut register; this TU's planes are its own,
// so w1_10002310.cpp's identical out-of-line plane is untouched).  0x10060010
// 5228 -> 5198 bytes differing, normalised instruction match 0.9633 ->
// 0.9639; the frame is unchanged (0x2d54 against 0x2d64).  The
// `pV = &(*ppVerts)[0]` spelling in the six planes was measured here too
// and is worse (0.9462): the shipped classify loops keep pVerts in a
// register.
static inline LTBOOL ClipPlaneNear60010(uint32* pUnused, Vert60010** ppVerts, int32* pnVerts, Vert60010** ppOut)
{
	uint32     anInside[CLIP60010_MAX];
	Vert60010*  pOutStart;
	Vert60010*  pV;
	Vert60010*  pEnd;
	Vert60010*  pPrev;
	Vert60010*  pCur;
	uint32*    pFlag;
	int32      nInside;
	int32      i;
	int32      iPrev;

	++g_n1008be44;

	pOutStart = *ppOut;
	nInside = 0;
	pFlag   = anInside;
	pV      = *ppVerts;
	pEnd    = pV + *pnVerts;

	for (; pV != pEnd; ++pV)
	{
		uint32 bIn = (pV->z >= g_f1008b908);

		nInside += bIn;
		*pFlag++ = bIn;
	}

	if (nInside == 0)
		return LTFALSE;

	if (nInside != *pnVerts)
	{
		iPrev     = *pnVerts - 1;
		pPrev     = *ppVerts + iPrev;

		for (i = 0; i < *pnVerts; ++i)
		{
			pCur = *ppVerts + i;

			if (anInside[iPrev])
				*(*ppOut)++ = *pPrev;

			if (anInside[iPrev] != anInside[i])
			{
				float t = ClipEdgeNear_60010((Vec60010c*)&pPrev->x, (Vec60010c*)&pCur->x, (Vec60010c*)&(*ppOut)->x);

				sub_10023380(pPrev, pCur, *ppOut, t);

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
static inline LTBOOL ClipPlaneLeft60010(uint32* pUnused, Vert60010** ppVerts, int32* pnVerts, Vert60010** ppOut)
{
	uint32     anInside[CLIP60010_MAX];
	Vert60010*  pOutStart;
	Vert60010*  pV;
	Vert60010*  pEnd;
	Vert60010*  pPrev;
	Vert60010*  pCur;
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
		uint32 bIn = (-pV->z < pV->x);

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
				float t = ClipEdgeLeft_60010((Vec60010c*)&pPrev->x, (Vec60010c*)&pCur->x, (Vec60010c*)&(*ppOut)->x);

				sub_10023380(pPrev, pCur, *ppOut, t);

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
static inline LTBOOL ClipPlaneTop60010(uint32* pUnused, Vert60010** ppVerts, int32* pnVerts, Vert60010** ppOut)
{
	uint32     anInside[CLIP60010_MAX];
	Vert60010*  pOutStart;
	Vert60010*  pV;
	Vert60010*  pEnd;
	Vert60010*  pPrev;
	Vert60010*  pCur;
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
		uint32 bIn = (pV->y < pV->z);

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
				float t = ClipEdgeTop_60010((Vec60010c*)&pPrev->x, (Vec60010c*)&pCur->x, (Vec60010c*)&(*ppOut)->x);

				sub_10023380(pPrev, pCur, *ppOut, t);

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
static inline LTBOOL ClipPlaneRight60010(uint32* pUnused, Vert60010** ppVerts, int32* pnVerts, Vert60010** ppOut)
{
	uint32     anInside[CLIP60010_MAX];
	Vert60010*  pOutStart;
	Vert60010*  pV;
	Vert60010*  pEnd;
	Vert60010*  pPrev;
	Vert60010*  pCur;
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
		uint32 bIn = (pV->x < pV->z);

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
				float t = ClipEdgeRight_60010((Vec60010c*)&pPrev->x, (Vec60010c*)&pCur->x, (Vec60010c*)&(*ppOut)->x);

				sub_10023380(pPrev, pCur, *ppOut, t);

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
static inline LTBOOL ClipPlaneBottom60010(uint32* pUnused, Vert60010** ppVerts, int32* pnVerts, Vert60010** ppOut)
{
	uint32     anInside[CLIP60010_MAX];
	Vert60010*  pOutStart;
	Vert60010*  pV;
	Vert60010*  pEnd;
	Vert60010*  pPrev;
	Vert60010*  pCur;
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
		uint32 bIn = (-pV->z < pV->y);

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
				float t = ClipEdgeBottom_60010((Vec60010c*)&pPrev->x, (Vec60010c*)&pCur->x, (Vec60010c*)&(*ppOut)->x);

				sub_10023380(pPrev, pCur, *ppOut, t);

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
static inline LTBOOL ClipPlaneFar60010(uint32* pUnused, Vert60010** ppVerts, int32* pnVerts, Vert60010** ppOut)
{
	uint32     anInside[CLIP60010_MAX];
	Vert60010*  pOutStart;
	Vert60010*  pV;
	Vert60010*  pEnd;
	Vert60010*  pPrev;
	Vert60010*  pCur;
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
		uint32 bIn = (pV->z <= g_f1008b910);

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
				float t = ClipEdgeFar_60010((Vec60010c*)&pPrev->x, (Vec60010c*)&pCur->x, (Vec60010c*)&(*ppOut)->x);

				sub_10023380(pPrev, pCur, *ppOut, t);

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
static inline LTBOOL Clip32_1000f4b0_60010(uint32 nClipFlags, Vert60010** ppVerts, int32* pnVerts)
{
	Vert60010*     pOut;
	Vert60010*     pVerts;
	int32      nVerts;
	uint32     anUnused[6];

	if (g_p1008d7a0)
	{
		nClipFlags &= 1;
		if (!nClipFlags)
			return LTTRUE;
	}

	pOut   = (Vert60010*)g_p1008b87c;
	pVerts = *ppVerts;
	nVerts = *pnVerts;

	if (nClipFlags & 0x01)
	{
		if (!ClipPlaneNear60010(&anUnused[0], &pVerts, &nVerts, &pOut))
			return LTFALSE;
	}

	if (nClipFlags & 0x04)
	{
		if (!ClipPlaneLeft60010(&anUnused[1], &pVerts, &nVerts, &pOut))
			return LTFALSE;
	}

	if (nClipFlags & 0x08)
	{
		if (!ClipPlaneTop60010(&anUnused[2], &pVerts, &nVerts, &pOut))
			return LTFALSE;
	}

	if (nClipFlags & 0x10)
	{
		if (!ClipPlaneRight60010(&anUnused[3], &pVerts, &nVerts, &pOut))
			return LTFALSE;
	}

	if (nClipFlags & 0x20)
	{
		if (!ClipPlaneBottom60010(&anUnused[4], &pVerts, &nVerts, &pOut))
			return LTFALSE;
	}

	if (!(nClipFlags & 0x02) ||
	    ClipPlaneFar60010(&anUnused[5], &pVerts, &nVerts, &pOut))
	{
		*ppVerts = pVerts;
		*pnVerts = nVerts;
		return LTTRUE;
	}

	return LTFALSE;
}




// ======================================================================= //
//  0x10060010 .. 0x10061a30   (6,688 bytes including the fifteen-byte pad)
//
//  The shipped entry takes pCtx in %eax, pObj in %edx, pPos in %ecx and
//  fScaleX in ST(0) (icl /Qip); written __cdecl here.
// ======================================================================= //
static void sub_10060010(Ctx60010* pCtx, Obj60010* pObj, LTVec60010* pPos,
                  float fScaleX, float fScaleY, TexBlock60010* pBlock)
{
	// -0x2c50, 0x40 bytes: the object frame sub_100151b0 builds.
	float			mat[16];

	// -0x2c10, -0x2bf8, -0x2bec: three twelve-byte vectors.  The fourth,
	// -0x2c04, is the temporary the subtraction operator returns into and
	// is not named here -- the known pattern "a vector operator leaves a temp
	// plus a whole-vector copy".
	LTVec60010		vDir;
	LTVec60010		vDelta;
	LTVec60010		vXfm;

	uint32			nSpecular;                  // -0x2c54
	uint32			nColor;                     // -0x2be0
	Vert60010		aQuad[4];                   // -0x2bdc, 0x80 bytes
	Vert60010*		pVerts;                     // -0x2b5c
	int32			nVerts;                     // -0x2b58
	Vert60010		aClip[300];                 // -0x2b54 .. -0x5d5

	// -0x50, -0x48, -0x70, -0x68, -0x60, -0x78: every one stored with
	// `fstpl` and loaded with `fldl`, so every one is a double.
	float			fTexW;
	float			fTexH;
	float			fU0, fU1, fV0, fV1;

	const int32*	pIdx;
	Vert60010*		pV;
	int32			i;

	float			fBias;
	float			fZ;
	int32			nDraw;

	// aClip: 0x2580 bytes of frame, %ebp-0x2b54 down to %ebp-0x5d4, of
	// which only the address is taken (the fifth argument of 0x1005fb70,
	// its output buffer): 9,600 = 300 32-byte vertices.  Declared as that
	// buffer rather than the old lone int at -0x2b54, it
	// puts every slot below it where the shipped frame has it.

	// -- 0x10060010: the distance that is computed and thrown away.
	//    `call 0x100154b0` then `fstp %st(0)` at 0x10060060, no store and
	//    no branch.
	pObj->sub_100158b0()->sub_100154b0(pCtx->m_v4b4);

	// -- 0x10060062 --------------------------------------------------- //
	if (!sub_10015320(pBlock, g_n1008d7d0, 0))
		return;

	// -- 0x10060098: BOTH through vtable slot +0x0c.  See COIN-FLIPS. -- //
	fTexW = g_a1008e684[g_n1008d7d0]->GetWidth0c();
	fTexH = g_a1008e684[g_n1008d7d0]->GetWidth0c();

	// -- 0x100600f2: the __fastcall(%ecx, %edx) through 0x1008ce8c ----- //
	g_pfn_1008ce8c(pObj->sub_100158b0(), &nSpecular);

	// -- 0x10060106: the uv corners.  ORDER FROM THE POOL, not the
	//    schedule: 0x100aaca4, 0x100aaca8, 0x100aacac, 0x100aacb0.
	fU0 = g_a1008e664[0].m_fInvW00 * 2.0f;
	fU1 = (fTexW - 2.0f) * g_a1008e664[0].m_fInvW00;
	fV0 = g_a1008e664[0].m_fInvH04 * 2.0f;
	fV1 = (fTexH - 2.0f) * g_a1008e664[0].m_fInvH04;

	// -- 0x1006014c: the object's own frame, and the facing normal ----- //
	sub_100151b0(pObj->sub_100158b0(), pObj->m_Rot9c, &pObj->m_vScaleac,
	             mat);

	vDir.sub_1000eec0(0.0f, 0.0f, -1.0f);

	sub_1005f790(mat, &vDir);

	// -- 0x100601b6: the winding test.  `fcomps 0x100aacc0` is against
	//    0.0f and the `>= 0` arm is the one icl SANK to 0x10061a17, so
	//    the `< 0` arm is the inline one and is written first.
	vDelta = pCtx->m_v4b4.sub_10015640(*pPos);

	if (vDir.sub_10027f50(vDelta) < 0.0f)
		pIdx = g_a100897e0;                     // {3, 2, 1, 0}
	else
		pIdx = g_a100897f0;                     // {0, 1, 2, 3}

	// -- 0x1006023a: the view-space centre and the vertex colour ------- //
	sub_10014940(&vXfm, &pCtx->m_m15c[0][0], pObj->sub_100158b0());

	sub_1005f7d0((Obj1005f7d0*)pObj, &nColor, &vXfm);

	// -- 0x1006027e: the four corners, in the table's order ------------ //
	pV = &aQuad[pIdx[0]];
	pV->sub_1000eec0((float)-fTexW, (float)fTexH, 0.0f);
	pV->sub_1000d9d0((float)fU0, (float)fV0);
	pV->m_Diffuse10.m_n  = nColor;
	pV->m_Specular14.m_n = nSpecular;

	pV = &aQuad[pIdx[1]];
	pV->sub_1000eec0((float)fTexW, (float)fTexH, 0.0f);
	pV->sub_1000d9d0((float)fU1, (float)fV0);
	pV->m_Diffuse10.m_n  = nColor;
	pV->m_Specular14.m_n = nSpecular;

	pV = &aQuad[pIdx[2]];
	pV->sub_1000eec0((float)fTexW, (float)-fTexH, 0.0f);
	pV->sub_1000d9d0((float)fU1, (float)fV1);
	pV->m_Diffuse10.m_n  = nColor;
	pV->m_Specular14.m_n = nSpecular;

	pV = &aQuad[pIdx[3]];
	pV->sub_1000eec0((float)-fTexW, (float)-fTexH, 0.0f);
	pV->sub_1000d9d0((float)fU0, (float)fV1);
	pV->m_Diffuse10.m_n  = nColor;
	pV->m_Specular14.m_n = nSpecular;

	// -- 0x100603fc ---------------------------------------------------- //
	sub_1005fac0(aQuad, 4, mat);

	pVerts = aQuad;
	nVerts = 4;

	// -- 0x1006044c ---------------------------------------------------- //
	if (pObj->m_n1bc != -1)
	{
		if (!sub_1005fb70(pObj, pObj->m_n1bc, &pVerts, &nVerts,
		                  aClip))
			return;
	}

	// -- 0x1006048f: BOTH ARMS MAKE CALLS.  See COIN-FLIPS note 2 ------ //
	if (pObj->m_nFlags88 & 0x4)
	{
		// -- 0x1006049e: the world transform, sub_1000d9b0's body
		//    (d3dlow.cpp -> 0x1000a770) spelled out against the 3x4 at
		//    g_1008b880 + 0x15c.  Each row associates from the last term
		//    backwards, which is why the m[i][2] load comes first.
		pV = pVerts;
		for (i = nVerts; i; i--)
		{
			float fZ = g_1008b880.m_Mat15c.m[2][0]*pV->x + g_1008b880.m_Mat15c.m[2][1]*pV->y + g_1008b880.m_Mat15c.m[2][2]*pV->z + g_1008b880.m_Mat15c.m[2][3];
			float fX = g_1008b880.m_Mat15c.m[0][0]*pV->x + g_1008b880.m_Mat15c.m[0][1]*pV->y + g_1008b880.m_Mat15c.m[0][2]*pV->z + g_1008b880.m_Mat15c.m[0][3];
			float fY = g_1008b880.m_Mat15c.m[1][0]*pV->x + g_1008b880.m_Mat15c.m[1][1]*pV->y + g_1008b880.m_Mat15c.m[1][2]*pV->z + g_1008b880.m_Mat15c.m[1][3];
			pV->z = fZ;
			pV->x = fX;
			pV->y = fY;
			pV++;
		}

		// -- 0x10060543 ------------------------------------------------ //
		if (g_n1008b874 &&
		    !Clip32_1000f4b0_60010(g_n1008b874, &pVerts, &nVerts))
		{
			nDraw = 0;
			goto Ltest;
		}

	Ldraw:
		// -- 0x1006181e: the projection.  The bias is recomputed PER
		//    VERTEX here (`flds 0x8(%ebx)` is inside the loop at
		//    0x10061830 and %ebx advances by 0x20), unlike big61a30 which
		//    computes it once outside.  The `>= 7.0f` arm is the one icl
		//    sank, to 0x10061a00.
		pV = pVerts;
		for (i = nVerts; i; i--)
		{
			fZ = pV->z;

			if (-20.0f + fZ >= 7.0f)
				fBias = -20.0f;
			else
				fBias = 7.0f - fZ;

			sub_1005f9a0((Vert61a30*)pV, (Vert61a30*)pV, fBias);
			pV++;
		}

		// -- 0x1006186c ------------------------------------------------ //
		nDraw = 1;
	}
	else
	{
		// -- 0x10061875, sunk below the epilogue by /O2 ---------------- //
		nDraw = sub_10014550(&pVerts, &nVerts, &g_1008b880);
	}

Ltest:
	// -- 0x1006189b: D3DPT_TRIANGLEFAN, FVF 0x1c4 --------------------- //
	if (nDraw)
	{
		g_pD3DDevice->lpVtbl->DrawPrimitive(g_pD3DDevice, 6, 0x1c4,
		                                    pVerts, nVerts, 0);
	}
}



// *********************************************************************** //
//
//  MODULE:   BIG_61A30 (.CPP)
//
//  PURPOSE:  One function -- 0x10061a30, 11280 bytes (0x2C10, so the body
//            ends at 0x1006463f and the `call 0x1007b8d0` at 0x10064644
//            belongs to the NEXT function, not to this one).  One caller,
//            0x10030bb3.
//
//  WHAT IT IS: d3d.ren's SPRITE PASS.  The caller walks the visible-object
//  array at g_1008beb0->+0x9c/+0xa0 and dispatches on the object's type byte
//  at +0xbc; this function is the arm for TYPE 3 -- OT_SPRITE.  It builds a
//  camera-facing quad from the sprite's transformed centre and the bound
//  texture's dimensions, colours it either from the object's own RGB or from
//  a trilinear sample of the world light grid, clips it against six frustum
//  planes, projects each surviving vertex and draws it as ONE TRIANGLE FAN.
//
//  FOUR INDEPENDENT TELLS that this is a sprite:
//    * the caller's dispatch is `cmp $0x3,%dl` on +0xbc, and 3 is OT_SPRITE
//      in ltbasedefs.h's object-type enum (2 = worldmodel, 7 = polygrid --
//      the other two arms of the same switch);
//    * the DrawPrimitive at 0x10064307 is
//      `(device, 6, 0x1c4, pVerts, nVerts, 0)`: 6 is D3DPT_TRIANGLEFAN and
//      0x1c4 is D3DFVF_XYZRHW|D3DFVF_DIFFUSE|D3DFVF_SPECULAR|D3DFVF_TEX1,
//      which is exactly the 32-byte vertex the whole function manipulates
//      (float x,y,z,rhw at +0..+0xc, two colours at +0x10/+0x14, u,v at
//      +0x18/+0x1c);
//    * the four vertices are built as (cx-w, cy+h), (cx+w, cy+h),
//      (cx+w, cy-h), (cx-w, cy-h) around ONE transformed point, with the
//      half-extents scaled by the TEXTURE's width and height read through
//      the bound texture's own vtable;
//    * the u/v corners are 2*(1/w), 2*(1/h) .. (texw-2)*(1/w),
//      (texh-2)*(1/h) -- the standard two-texel inset of a sprite quad.
//
//  RUN THE ACCEPTANCE TEST WITH END = 0x10064640, NOT 0x10064660.  The row
//  says 11280 = 0x2C10 bytes and 0x10061a30 + 0x2C10 = 0x10064640, which
//  is the next function's entry; the
//  `call 0x1007b8d0` at 0x10064644 belongs to IT.  Given 0x10064660 the
//  shipped side has 30 calls against our 29 and reports one spurious
//  shipped-only row.
//
//  Compiler: Intel C/C++ 4.0, **/O2**.  The /O2 tells, both rows that
//  apply:
//    * outgoing arguments are `add $-N,%esp` followed by `mov %reg,K(%esp)`
//      everywhere (0x10061a70, 0x10061ad2, 0x10061dd8, 0x10062ad9,
//      0x10064210, 0x100642ce ...) and there is not one `push` of an
//      argument in the whole 11K;
//    * `&pObj->m_vPos18c` at 0x10061cc6 is built as `add $0x18c,%ecx` on a
//      register that already holds the object -- the /O2 `mov $K; add`
//      idiom, not `lea 0x18c(%ecx)`.
//  Cold-block sinking (six clip epsilon arms parked at 0x100643b8..0x100644c0
//  and two allocator arms at 0x1006457b/0x100645b1, all BELOW the `ret`) is
//  /O2 behaviour too.
//
//  ------------------------------------------------------------------- //
//  THREE THINGS THIS FILE CANNOT REPRODUCE, all stated up front.
//  ------------------------------------------------------------------- //
//
//  0. MEASURED: byte identity is out of reach for now.  At /O2 /Qip
//     (the build) the body is 9824 bytes against 11280 (see the
//     MEASUREMENTS at the top of the file for the history: the earlier
//     (uint8)(int32) text was 12263, and /O2 /Qrcd on it 9069 with one
//     64-bit `fistpll` per site).  /Qrcd is not the answer and is not
//     used: the shipped `fstps; flds; fistpl` is an inlined RoundF (note
//     1), which this text now reproduces at all 33 sites.
//
//  1. THE SHIPPED CONVERSIONS ARE AN INLINED `__asm fld f ; fistp n`.
//     Every float->byte conversion in the clipper and the three
//     light-grid ambient terms is
//         fstps <float slot> ; flds <float slot> ; fistpl <int slot>
//     -- the float argument spilled to ITS OWN 8-byte slot (-0x938,
//     -0x930, ... one per inlined call), reloaded by the asm's `fld f`,
//     and a bare `fistp` in the CURRENT rounding mode with no fnstcw/fldcw
//     dance and no helper call.  That is RoundF (defined above
//     0x1005f7d0) inlined, exactly as d3dbatchd.cpp recorded for
//     0x10015190 (a Round() helper) and for 0x10023380, the lerp inlined
//     here (see 3).  Six conversions are NOT RoundF in the shipped bytes
//     and are plain casts here too: the light-grid cell nX/nY/nZ
//     (0x100620ca/0x10062158/0x100621e6) and the three LTCLAMP colour
//     terms (0x10062991/0x10062a1e/0x10062aa9), each the twelve-
//     instruction shld/neg/or/jge truncation; both sides carry six.
//     An earlier text had every site as a (uint8)(int32) cast because
//     RoundF was then measured as staying a call in this body; with the
//     clipper as the six static inline plane bodies it inlines
//     everywhere, 33 `fistpl` for the shipped 33.  (/QIfist is not an
//     icl 4.0 option: it parses as /QIf and is ignored.)
//
//  2. 0x10060010 IS A REGISTER-ABI CALLEE (icl /Qip).  At 0x10061b64 it is
//     entered with %eax = arg0, %edx = arg1, %ecx = arg2, ST(0) = arg3 and
//     the last two arguments at 0x10(%esp)/0x14(%esp) of a 0x18-byte
//     outgoing area.  The callee proves the shape: it opens
//     `push %ecx ; push %edx ; push %eax`, reloads those three from the
//     stack, `fstp %st(0)` to drop the float, and reads its fifth and sixth
//     arguments at 0x18(%ebp)/0x1c(%ebp).  There is no way to ask icl for
//     that convention from C, so it is declared __cdecl below with the same
//     six arguments in the same order.  It is still ONE `call` in the same
//     position, which is all the acceptance test looks at.
//
//  3. 0x10023380 IS INLINED HERE, TWELVE TIMES.  d3dbatchd.cpp describes it
//     exactly: "a vertex LERP: two floats at +0x18 and +0x1c interpolated
//     normally, then five bytes (+0x12, +0x11, +0x10, +0x13, +0x17) each
//     `fstps temp; flds temp; fistpl n; mov n,%al`".  The six clip planes
//     here each carry that body inline in that field order, so it is written
//     as an `inline` function below and NOT as a call -- there is no `call
//     0x10023380` anywhere in the shipped 11K.
//
//  ------------------------------------------------------------------- //
//  REGION MAP, with what pins each one.
//  ------------------------------------------------------------------- //
//
//  0x10061a30..0x10061a70  THE GUARD.  Three tests -- +0x1ac, +0x1b0 and
//      *(+0x1b0) -- each `je 0x100643a9`, which is the BARE epilogue with no
//      render-state restore, so all three precede the scoped savers and are
//      three plain early returns.
//
//  0x10061a70..0x10061b2c  BLEND SETUP.  g_n1008b874 = 0x3f (six clip-plane
//      bits, and every later `test` reads exactly one of those six), then
//      sub_1005a490(pObj, &a,&b,&c,&d) -- d3dr3c.cpp has that one decoded as
//      "pick the alpha-blend mode and, with it, the fog colour", four
//      out-parameters.  The four values are then handed to four
//      C1000adc0 constructors (d3dbatcha.cpp: an eight-byte scoped
//      render-state saver, state at +0x00, old value at +0x04) with states
//      0x13, 0x14, 0x1c, 0x22 = SRCBLEND, DESTBLEND, FOGENABLE, FOGCOLOR --
//      the same four states d3dr3c.cpp says 0x1005a490 chooses.  The
//      matching destructors are the four `call *0x50(%edx)` at
//      0x10064337..0x100643a6, in REVERSE order, in ONE place: written out
//      by hand below through a `goto`, because a real C++ destructor would
//      let /O2 duplicate the epilogue and change the call sequence.
//
//  0x10061b2c..0x10061b71  THE `flags & 8` ARM.  One call to 0x10060010 and
//      an immediate `jmp` to the restore-and-destruct tail: everything else
//      in the function is the `else`.
//
//  0x10061b71..0x10061cde  THE TRANSFORM.  m_n88 bit 0x40 chooses between
//      the +0x2dc and the +0x15c 3x4 matrix of the context object (the
//      caller passes the fixed global 0x1008ff60), applied to the position
//      the accessor 0x100158b0 answers (`lea 0x18c(%ecx)` -- already pinned
//      by big4b720.cpp and d3dbatchc.cpp).  Each row is
//      m[i][2]*z first, then m[i][1]*y, then m[i][0]*x, then + m[i][3],
//      which is the measured last-term-backwards association of the
//      four-term source expression, exactly as in big19d64.cpp's MatVMul.
//      The same bit picks the minimum-z the result is tested against:
//      0.01f (0x100aad4c) or 7.0f (0x100aae20).  Then the __fastcall
//      through g_pfn_1008ce8c -- d3dr2d.cpp pinned that global as
//      __fastcall(%ecx,%edx) across fourteen call sites -- fills the
//      specular dword every vertex gets.  `fldl -0x78 ; fcompl -0x80 ; jbe`
//      is the z-reject.
//
//  0x10061ce3..0x10061e7c  TEXTURE BINDING.  A per-stage record is looked
//      up in the block's singly-linked +0x30 list by its +0x42 stage id
//      (the loop is rotated and unrolled by two, with both `found` arms
//      sunk to 0x10064630/0x10064637); on a miss 0x10039bd0 makes one --
//      with a third argument of 1 when the list is non-empty and the result
//      is linked in AFTER the head, and 0 when it is empty, which is the
//      same shape d3dbatcha.cpp records for that allocator at its other
//      call site.  If the record is not the one already bound
//      (g_1008e684[stage]) it is bound: LRU move-to-front of the +0x1c
//      LTLink through the list head at 0x1008e634, `g_1008cbcc->+0x48 +=
//      rec->+0x10`, SetTexture(stage, rec->+0x0c), an optional
//      SetRenderState(24 = D3DRENDERSTATE_ALPHAREF, rec->+0x16),
//      g_1008bd80++ and the two floats at +0x04/+0x08 published into
//      g_1008e664[stage].  The `rec->+0x44` -> SetLOD(0) block appears
//      TWICE in the shipped bytes -- once at 0x10061e51 and once in the
//      sunk arm at 0x10064587 -- so it is written twice here; see the
//      COIN-FLIP note at the end of this banner.
//      +0xbc on the surface's vtable is IDirectDrawSurface7::SetLOD; +0x8c
//      and +0x50 on the device's are SetTexture and SetRenderState, which
//      d3dbatcha.cpp already fixes.
//
//  0x10061e7c..0x10061fc9  THE QUAD'S EXTENTS.  Two virtual calls on the
//      NOW-BOUND record, vtable slots +0x0c and +0x10, return unsigned
//      dimensions (`mov %eax,-0x1b0 ; movl $0,-0x1ac ; fildll` is the
//      unsigned->double sequence), and the half-extents are
//      texw*ctx->+0x94*obj->+0xac and texh*ctx->+0x98*obj->+0xb0.  The u/v
//      corners are 2*g_1008e664[0].f0 / 2*g_1008e664[0].f4 and
//      (texw-2)*..., (texh-2)*... .  m_n88 bit 0x20 adds a distance fade:
//      LTCLAMP((z-10)/(500-10), 0, 1) * (2.0f-0.1f) + 0.1f scaling both
//      half-extents.  LTCLAMP is ltbasedefs.h:1248 and its shape is proved
//      by the CONSTANT POOL: icl gives every textual float literal in a
//      distinct basic block its own .rdata slot, and 0.0f has two
//      (0x100aad64, 0x100aad68) and 1.0f two (0x100aae18, 0x100aae1c) --
//      exactly the two occurrences each of `min` and `max` in that macro.
//
//  0x10061fc9..0x10062028 and 0x10062c84  THE DIFFUSE COLOUR, taken from the
//      object's own +0x94/95/96 when m_n88 & 0x200 OR when there is no light
//      grid (the `je 0x10061fe7` at 0x1006202f jumps BACKWARDS into the
//      first arm, which is what an `||` merge looks like).
//
//  0x10062028..0x10062c84  THE LIGHT GRID.  Byte for byte the same algorithm
//      big19d64.cpp reconstructs at 0x1001ab1a, against the same global
//      g_1008beac and the same field offsets: position minus +0x138,
//      scaled by +0x12c/+0x130/+0x134, truncated, LTCLAMPed against
//      +0x110/+0x114/+0x118, fractional parts from three `floor` calls
//      (0x1007fd30), cell base +0xfc, strides +0x104 and +0x11c, 24 corner
//      bytes read at +2/+1/+0 and +6/+5/+4 of four cells, twelve
//      intermediate weights, three (uint8) results.  The only difference
//      from big19d64 is that HERE the coordinates and fractions are DOUBLE
//      (`fstpl`/`fldl` throughout, against `fstps`/`flds` there), so they
//      are declared double.  The three bytes are then added to
//      sub_1001e210's dynamic-light colour (d3dr3d.cpp has that one) and to
//      the object's own +0x94/95/96, LTCLAMPed to 0..255 and packed.
//
//  0x10062c84..0x10062da6  THE QUAD.  Four 32-byte vertices at -0x764,
//      -0x744, -0x724, -0x704, all sharing z, the diffuse dword and the
//      specular dword; +0x0c (rhw) is deliberately NOT written here -- the
//      projection at 0x1005f9a0 fills it.
//
//  0x10062da6..0x100641bb  THE CLIPPER.  g_1008b908 (the near plane) is
//      saved, and lowered to 0.01f when m_n88 & 0x40.  Then six copies of
//      one Sutherland-Hodgman pass, gated by bits 0x01, 0x04, 0x08, 0x10,
//      0x20 and 0x02 of the mask IN THAT ORDER, against
//        z >= g_1008b908 | -z < x | y < z | x < z | -z < y | z <= g_1008b910
//      i.e. near, left, top, right, bottom, far.  Each pass has its own
//      50-entry inside-flag array: the arrays start at -0x6dc, -0x600,
//      -0x524, -0x448, -0x36c and -0x290, 0xdc apart, of which 0xc8 = 200
//      bytes = 50 int32 is the array and the remaining 0x14 is the five
//      int slots the five byte-lerps of that pass need.  50 is therefore
//      read off the frame, not guessed.  Each pass bumps g_1008be44.
//      The interpolation parameter is
//        t = (d < -0.00001f || d > 0.00001f) ? (num / d) : 0.0f
//      with the second half of the `||` sunk below the epilogue in all six
//      cases (0x100643d6, 0x10064400, 0x10064430, 0x10064460, 0x10064490,
//      0x100644c0) -- the pattern "a returning error guarded by || gets
//      sunk".  The ping-pong buffer is g_1008b87c and each pass ends
//      `pIn = pOutStart; nIn = pOut - pOutStart; pOut += nIn`.
//
//  0x100641bb..0x1006430a  THE DRAW.  m_n88 bit 0x04 projects every vertex
//      with a z bias of LTMAX(-20.0f, 7.0f - z) -- written as the shipped
//      `if (-20.0f + fZ >= 7.0f)` because that is literally the compare
//      emitted; bit 0x40 projects with a fixed 7.0f (0x40e00000); otherwise
//      the batch goes through 0x1005fa80, whose only caller in the whole
//      image is this site (d3dr2a.cpp).  Then DrawPrimitive.
//
//  0x1006430a..0x100643b7  restore g_1008b908, four destructors, epilogue.
//
//  ------------------------------------------------------------------- //
//  WHAT IS *NOT* PINNED -- read this before trusting any byte of it.
//  ------------------------------------------------------------------- //
//
//  * SIX DEAD 1.0f CONSTANTS.  0x100aad80, 0x100aad88, 0x100aad90,
//    0x100aad98, 0x100aada0 and 0x100aada8 are 1.0f slots that sit
//    interleaved with the six clip epsilons (0x100aad7c + 8k) and are
//    referenced from NOWHERE in the image -- verified by scanning all of
//    .text for their address.  So each clip pass's source mentions a 1.0f
//    that icl folded away, and the spelling below does not.  It costs no
//    call and no branch; it is recorded rather than invented.
//
//  * THE COLOUR PACKING masks.  The shipped chain is
//      c = a<<24 ; c |= (r<<16)&0xff0000 ; c = (c&0xffff00ff)|((g<<8)&0xff00)
//      ; c = (c&0xffffff00)|b
//    -- the clear-mask is present for green and blue and absent for alpha
//    and red, which is what icl does when it can still prove those bits
//    zero.  The uniform spelling below is the natural source for that; the
//    exact macro it came from is not recoverable.
//
//  * THE FPU SCHEDULE of the light grid's twelve weights.  The statement
//    shape is big19d64.cpp's, proved there; which of -0x7ac..-0x780 is
//    which is not separable from icl's scheduling and is not claimed.
//
//  * THE THREE CALL-BEARING COLD BLOCKS.  icl sank three blocks BELOW the
//    epilogue in the shipped function: the already-bound arm at 0x1006457b
//    (one `call *0xbc`) and the allocator arm at 0x100645b1 (the two
//    `call 0x10039bd0`).  Their position in the shipped call sequence is
//    therefore LAST, after the four destructors, even though the source
//    puts them in the middle -- exactly where they are written below.  That
//    is the compiler's cold-block placement, not a source order, and it is
//    the one thing in this file that the acceptance test can disagree with
//    for a reason that is not a decode error.  If it reports three
//    shipped-only and three ours-only calls that are precisely
//    0x1006459d, 0x100645d3 and 0x10064615, that is this and nothing else.
//
//  * `(uint8)RoundF(expr)` at the eight float->byte sites (the five lerp
//    bytes and the three ambient terms): the shipped code converts into a
//    32-BIT slot and takes its low byte (`fistpl -0x614(%ebp) ;
//    mov -0x614(%ebp),%dl`), which is the inlined asm helper's `n` (note 1
//    above).  The three LTCLAMP colour terms stay `(uint8)(int32)` casts:
//    the shipped code truncates those with the shld idiom, not a fistp.
//    big19d64.obj is the control -- it holds `_floor` and no `__ftol`, so
//    integer casts of floats and doubles are inline in this build.
//
//  COIN-FLIP, stated so it can be settled in one build: the SetLOD(0) block
//  is written TWICE below, once in each arm of `pRec != g_1008e684[stage]`,
//  because the shipped code has two `call *0xbc(%eax)` sites for it.  The
//  alternative is that the source wrote it ONCE after the `if` and icl
//  cloned the tail into the empty else; if this comes back with one call too
//  many, that is the spelling to try.
//
// *********************************************************************** //

// ---------------------------------------------------------------------- //
//  0x10023380 -- the vertex LERP, INLINE here (there is no call to it in
//  the shipped 11K, only twelve copies of its body).  Field order is the
//  shipped one: +0x18, +0x1c, then +0x12, +0x11, +0x10, +0x13, +0x17.
// ---------------------------------------------------------------------- //
inline void sub_10023380(Vert61a30* a, Vert61a30* b, Vert61a30* dst, float t)
{
	dst->tu = (b->tu - a->tu) * t + a->tu;
	dst->tv = (b->tv - a->tv) * t + a->tv;

	dst->m_Diffuse10.m_b[2]  = (uint8)RoundF((b->m_Diffuse10.m_b[2]  - a->m_Diffuse10.m_b[2])  * t + a->m_Diffuse10.m_b[2]);
	dst->m_Diffuse10.m_b[1]  = (uint8)RoundF((b->m_Diffuse10.m_b[1]  - a->m_Diffuse10.m_b[1])  * t + a->m_Diffuse10.m_b[1]);
	dst->m_Diffuse10.m_b[0]  = (uint8)RoundF((b->m_Diffuse10.m_b[0]  - a->m_Diffuse10.m_b[0])  * t + a->m_Diffuse10.m_b[0]);
	dst->m_Diffuse10.m_b[3]  = (uint8)RoundF((b->m_Diffuse10.m_b[3]  - a->m_Diffuse10.m_b[3])  * t + a->m_Diffuse10.m_b[3]);
	dst->m_Specular14.m_b[3] = (uint8)RoundF((b->m_Specular14.m_b[3] - a->m_Specular14.m_b[3]) * t + a->m_Specular14.m_b[3]);
}


// The frame says fifty: each pass's flag array is 0xc8 = 200 bytes.
#define CLIP61A30_MAX	50


void sub_10061a30(Ctx61a30* pCtx, Obj61a30* pObj)
{
	uint32			nSrcBlend;
	uint32			nDestBlend;
	uint32			nFogEnable;
	uint32			nFogColor;
	C1000adc0		sSrcBlend;
	C1000adc0		sDestBlend;
	C1000adc0		sFogEnable;
	C1000adc0		sFogColor;

	TexBlock61a30*	pBlock;
	Tex61a30*		pRec;
	Tex61a30*		pCur;
	LTVector*		pPos;

	uint32			nSpecular;
	uint32			nColor;
	double			fTexW;
	double			fTexH;
	double			fCx;
	double			fCy;
	double			fZ;
	double			fMinZ;
	double			fSizeX;
	double			fSizeY;
	double			fHalfW;
	double			fHalfH;
	double			fU0;
	double			fU1;
	double			fV0;
	double			fV1;
	double			fFade;
	double			fBias;

	Vert61a30		aQuad[4];
	Vert61a30*		pVerts;
	int32			nVerts;

	Vert61a30*		pIn;
	Vert61a30*		pOut;
	Vert61a30*		pOutStart;
	Vert61a30*		pPrev;
	Vert61a30*		pWalk;
	int32			nIn;
	int32			nInside;
	int32			i;
	int32			j;
	uint32			nClip;
	float			fSaveNear;
	float			fDen;
	float			fT;
	float			fVal;

	// -- 0x10061a30: the guard.  Three separate early returns; all three
	//    reach the BARE epilogue at 0x100643a9, ahead of the savers.
	if (!pObj->m_p1ac)
		return;

	if (!pObj->m_ppBlock1b0)
		return;

	if (!*pObj->m_ppBlock1b0)
		return;

	// -- 0x10061a70 ----------------------------------------------------- //
	g_n1008b874 = 0x3f;

	sub_1005a490(pObj, &nSrcBlend, &nDestBlend, &nFogEnable, &nFogColor);

	sSrcBlend.sub_1000adc0(0x13, nSrcBlend);        // D3DRENDERSTATE_SRCBLEND
	sDestBlend.sub_1000adc0(0x14, nDestBlend);      // D3DRENDERSTATE_DESTBLEND
	sFogEnable.sub_1000adc0(0x1c, nFogEnable);      // D3DRENDERSTATE_FOGENABLE
	sFogColor.sub_1000adc0(0x22, nFogColor);        // D3DRENDERSTATE_FOGCOLOR

	// -- 0x10061b2c ----------------------------------------------------- //
	if (pObj->m_nFlags88 & 0x8)
	{
		pPos   = pObj->sub_100158b0();
		pBlock = *pObj->m_ppBlock1b0;

		sub_10060010((Ctx60010*)pCtx, (Obj60010*)pObj, (LTVec60010*)pPos,
		             pObj->m_fScaleXac, pObj->m_fScaleYb0,
		             (TexBlock60010*)pBlock);
		goto Lcleanup;
	}

	// -- 0x10061b71: the transform -------------------------------------- //
	//  +0xac and +0xb0 are copied into DOUBLE locals here -- `flds 0xac ;
	//  flds 0xb0 ; fstpl -0x90 ; fstpl -0x88` at 0x10061b7e..0x10061b9c --
	//  which is why the half-extents below multiply with `fmull` and not
	//  `fmuls`.
	pPos   = pObj->sub_100158b0();
	pBlock = *pObj->m_ppBlock1b0;

	fSizeX = pObj->m_fScaleXac;
	fSizeY = pObj->m_fScaleYb0;

	if (pObj->m_nFlags88 & 0x40)
	{
		fCx = pCtx->m_m2dc[0][0] * pPos->x + pCtx->m_m2dc[0][1] * pPos->y +
		      pCtx->m_m2dc[0][2] * pPos->z + pCtx->m_m2dc[0][3];
		fCy = pCtx->m_m2dc[1][0] * pPos->x + pCtx->m_m2dc[1][1] * pPos->y +
		      pCtx->m_m2dc[1][2] * pPos->z + pCtx->m_m2dc[1][3];
		fZ  = pCtx->m_m2dc[2][0] * pPos->x + pCtx->m_m2dc[2][1] * pPos->y +
		      pCtx->m_m2dc[2][2] * pPos->z + pCtx->m_m2dc[2][3];

		fMinZ = 0.01f;
	}
	else
	{
		fCx = pCtx->m_m15c[0][0] * pPos->x + pCtx->m_m15c[0][1] * pPos->y +
		      pCtx->m_m15c[0][2] * pPos->z + pCtx->m_m15c[0][3];
		fCy = pCtx->m_m15c[1][0] * pPos->x + pCtx->m_m15c[1][1] * pPos->y +
		      pCtx->m_m15c[1][2] * pPos->z + pCtx->m_m15c[1][3];
		fZ  = pCtx->m_m15c[2][0] * pPos->x + pCtx->m_m15c[2][1] * pPos->y +
		      pCtx->m_m15c[2][2] * pPos->z + pCtx->m_m15c[2][3];

		fMinZ = 7.0f;
	}

	// -- 0x10061cbe: the fog/specular callback, __fastcall through the
	//    global pointer d3dr2d.cpp pinned.
	g_pfn_1008ce8c(&pObj->m_vPos18c, &nSpecular);

	if (fZ <= fMinZ)
		goto Lcleanup;

	if (!pBlock)
		goto Lcleanup;

	// -- 0x10061cf6: find (or make) this stage's texture record ---------- //
	pBlock->m_nStamp30 = g_w1008b870;

	pRec = LTNULL;

	if (pBlock->m_pHead0c)
	{
		// The list test is emitted twice -- 0x10061d06 and 0x10061d14 --
		// and icl 4.0 does not fold a redundant null test, so the source
		// really does test it twice.
		pCur = pBlock->m_pHead0c;
		while (pCur)
		{
			if (pCur->m_nStage42 == (uint8)g_n1008d7d0)
			{
				pRec = pCur;
				break;
			}

			pCur = pCur->m_pNext30;
		}
	}

	if (!pRec)
	{
		// 0x100645b1 -- sunk below the epilogue.
		if (pBlock->m_pHead0c)
		{
			pRec = sub_10039bd0(pBlock, g_n1008d7d0, 1);
			if (!pRec)
				goto Lcleanup;

			pRec->m_pNext30            = pBlock->m_pHead0c->m_pNext30;
			pBlock->m_pHead0c->m_pNext30 = pRec;
		}
		else
		{
			pRec = sub_10039bd0(pBlock, g_n1008d7d0, 0);
			if (!pRec)
				goto Lcleanup;
		}
	}

	if (pRec != g_a1008e684[g_n1008d7d0])
	{
		g_a1008e684[pRec->m_nStage42] = pRec;

		if (pRec->m_nStamp14 != g_w1008bea8)
		{
			// dl_Remove / dl_TieOff / dl_Insert, ltlink.h:100..145.
			pRec->m_Link1c.m_pPrev->m_pNext = pRec->m_Link1c.m_pNext;
			pRec->m_Link1c.m_pNext->m_pPrev = pRec->m_Link1c.m_pPrev;
			pRec->m_Link1c.m_pPrev = &pRec->m_Link1c;
			pRec->m_Link1c.m_pNext = &pRec->m_Link1c;

			pRec->m_Link1c.m_pPrev = g_p1008e634;
			pRec->m_Link1c.m_pNext = g_p1008e634->m_pNext;
			pRec->m_Link1c.m_pNext->m_pPrev = &pRec->m_Link1c;
			pRec->m_Link1c.m_pPrev->m_pNext = &pRec->m_Link1c;

			g_p1008cbcc->m_nBytes48 += pRec->m_nBytes10;

			pRec->m_nStamp14 = g_w1008bea8;
		}

		g_pD3DDevice->lpVtbl->SetTexture(g_pD3DDevice,
		                                 (uint8)pRec->m_nStage42,
		                                 pRec->m_pSurface0c);

		if (pRec->m_nAlphaRef16)
		{
			// 0x18 = D3DRENDERSTATE_ALPHAREF
			g_pD3DDevice->lpVtbl->SetRenderState(g_pD3DDevice, 0x18,
			                                     pRec->m_nAlphaRef16);
		}

		g_n1008bd80++;

		g_a1008e664[pRec->m_nStage42].m_fInvH04 = pRec->m_fInvH08;
		g_a1008e664[pRec->m_nStage42].m_fInvW00 = pRec->m_fInvW04;

		if (pRec->m_nDirty44)
		{
			pRec->m_pSurface0c->lpVtbl->SetLOD(pRec->m_pSurface0c, 0);
			pRec->m_nDirty44 = 0;
		}
	}
	else
	{
		// 0x1006457b -- the same block again, sunk.  See the COIN-FLIP
		// note in the banner.
		if (pRec->m_nDirty44)
		{
			pRec->m_pSurface0c->lpVtbl->SetLOD(pRec->m_pSurface0c, 0);
			pRec->m_nDirty44 = 0;
		}
	}

	// -- 0x10061e7c: the quad's extents --------------------------------- //
	fTexW = g_a1008e684[g_n1008d7d0]->GetWidth0c();
	fTexH = g_a1008e684[g_n1008d7d0]->GetHeight10();

	fV1 = (fTexH - 2.0f) * g_a1008e664[0].m_fInvH04;
	fU1 = (fTexW - 2.0f) * g_a1008e664[0].m_fInvW00;

	fU0 = g_a1008e664[0].m_fInvW00 * 2.0f;
	fV0 = g_a1008e664[0].m_fInvH04 * 2.0f;

	fHalfW = fTexW * pCtx->m_fScaleX94 * fSizeX;
	fHalfH = fTexH * pCtx->m_fScaleY98 * fSizeY;

	if (pObj->m_nFlags88 & 0x20)
	{
		fFade = LTCLAMP((fZ - 10.0f) / (500.0f - 10.0f), 0.0f, 1.0f) *
		        (2.0f - 0.1f) + 0.1f;

		fHalfW = fHalfW * fFade;
		fHalfH = fHalfH * fFade;
	}

	// -- 0x10061fc9: the diffuse colour --------------------------------- //
	//  The clear-mask on alpha and red is absent in the shipped bytes
	//  because icl could still prove those bits zero; see the banner.
	nColor = ((uint32)pObj->m_nA97 << 24) & 0xff000000;

	if ((pObj->m_nFlags88 & 0x200) || !g_p1008beac)
	{
		nColor = (nColor & 0xff00ffff) | (((uint32)pObj->m_nR94 << 16) & 0x00ff0000);
		nColor = (nColor & 0xffff00ff) | (((uint32)pObj->m_nG95 <<  8) & 0x0000ff00);
		nColor = (nColor & 0xffffff00) | ( (uint32)pObj->m_nB96        & 0x000000ff);
	}
	else
	{
		// -- 0x10062028: the light grid.  Same algorithm, same offsets and
		//    the same statement shape big19d64.cpp proved at 0x1001ab1a;
		//    the coordinates are DOUBLE here (fstpl/fldl), not float.
		Grid61a30*		pGrid;
		GridCell61a30*	pBase;
		GridCell61a30*	pZPlane;
		int32			nX, nY, nZ;
		double			fGX, fGY, fGZ;
		double			fInvX, fInvY, fInvZ;
		float			fC00, fC01, fC02, fC03, fC04, fC05;
		float			fC06, fC07, fC08, fC09, fC10, fC11;
		float			fC12, fC13, fC14, fC15, fC16, fC17;
		float			fC18, fC19, fC20, fC21, fC22, fC23;
		float			fW0, fW1, fW2, fW3, fW4, fW5;
		float			fW6, fW7, fW8, fW9, fW10, fW11;
		uint32			nAmbR, nAmbG, nAmbB;
		LTVector		vDynamic;

		pGrid = g_p1008beac;

		fGX = (pObj->m_vPos18c.x - pGrid->m_vMin138.x) * pGrid->m_fScale12c;
		fGY = (pObj->m_vPos18c.y - pGrid->m_vMin138.y) * pGrid->m_fScale130;
		fGZ = (pObj->m_vPos18c.z - pGrid->m_vMin138.z) * pGrid->m_fScale134;

		nX = (int32)fGX;
		nY = (int32)fGY;
		nZ = (int32)fGZ;

		nX = LTCLAMP(nX, 0, pGrid->m_nMaxX110);
		nY = LTCLAMP(nY, 0, pGrid->m_nMaxY114);
		nZ = LTCLAMP(nZ, 0, pGrid->m_nMaxZ118);

		fGX -= floor(fGX);
		fGY -= floor(fGY);
		fGZ -= floor(fGZ);

		fInvX = 1.0f - fGX;
		fInvY = 1.0f - fGY;
		fInvZ = 1.0f - fGZ;

		pBase = pGrid->m_pCellsfc + (nX + nY * pGrid->m_nStrideY104
		                                + nZ * pGrid->m_nStrideZ11c);

		fC00 = pBase[pGrid->m_nStrideY104].m_b2;
		fC01 = pBase[pGrid->m_nStrideY104].m_b1;
		fC02 = pBase[pGrid->m_nStrideY104].m_b0;
		fC03 = pBase[pGrid->m_nStrideY104 + 1].m_b2;
		fC04 = pBase[pGrid->m_nStrideY104 + 1].m_b1;
		fC05 = pBase[pGrid->m_nStrideY104 + 1].m_b0;
		fC06 = pBase[0].m_b2;
		fC07 = pBase[0].m_b1;
		fC08 = pBase[0].m_b0;
		fC09 = pBase[1].m_b2;
		fC10 = pBase[1].m_b1;
		fC11 = pBase[1].m_b0;

		pZPlane = pBase + pGrid->m_nStrideZ11c;

		fC12 = pZPlane[pGrid->m_nStrideY104].m_b2;
		fC13 = pZPlane[pGrid->m_nStrideY104].m_b1;
		fC14 = pZPlane[pGrid->m_nStrideY104].m_b0;
		fC15 = pZPlane[pGrid->m_nStrideY104 + 1].m_b2;
		fC16 = pZPlane[pGrid->m_nStrideY104 + 1].m_b1;
		fC17 = pZPlane[pGrid->m_nStrideY104 + 1].m_b0;
		fC18 = pZPlane[0].m_b2;
		fC19 = pZPlane[0].m_b1;
		fC20 = pZPlane[0].m_b0;
		fC21 = pZPlane[1].m_b2;
		fC22 = pZPlane[1].m_b1;
		fC23 = pZPlane[1].m_b0;

		fW0 = fC06 * fInvY;
		fW1 = fC07 * fInvY;
		fW2 = fC08 * fInvY;
		fW0 += fC00 * fGY;
		fW1 += fC01 * fGY;
		fW2 += fC02 * fGY;

		fW3 = fC09 * fInvY;
		fW4 = fC10 * fInvY;
		fW5 = fC11 * fInvY;
		fW3 += fC03 * fGY;
		fW4 += fC04 * fGY;
		fW5 += fC05 * fGY;

		fW6 = fW0 * fInvX;
		fW7 = fW1 * fInvX;
		fW8 = fW2 * fInvX;
		fW6 += fW3 * fGX;
		fW7 += fW4 * fGX;
		fW8 += fW5 * fGX;

		fW0 = fC18 * fInvY;
		fW1 = fC19 * fInvY;
		fW2 = fC20 * fInvY;
		fW0 += fC12 * fGY;
		fW1 += fC13 * fGY;
		fW2 += fC14 * fGY;

		fW3 = fC21 * fInvY;
		fW4 = fC22 * fInvY;
		fW5 = fC23 * fInvY;
		fW3 += fC15 * fGY;
		fW4 += fC16 * fGY;
		fW5 += fC17 * fGY;

		fW9  = fW0 * fInvX;
		fW10 = fW1 * fInvX;
		fW11 = fW2 * fInvX;
		fW9  += fW3 * fGX;
		fW10 += fW4 * fGX;
		fW11 += fW5 * fGX;

		nAmbR = (uint8)RoundF(fW6 * fInvZ + fW9  * fGZ);
		nAmbG = (uint8)RoundF(fW7 * fInvZ + fW10 * fGZ);
		nAmbB = (uint8)RoundF(fW8 * fInvZ + fW11 * fGZ);

		// -- 0x10062ad9 -- d3dr3d.cpp's dynamic-light accumulator.
		sub_1001e210(pObj, &vDynamic);

		nColor = (nColor & 0xff00ffff) |
		         ((((uint32)(uint8)(int32)LTCLAMP((float)pObj->m_nR94 + vDynamic.x + (float)nAmbR, 0.0f, 255.0f)) << 16) & 0x00ff0000);
		nColor = (nColor & 0xffff00ff) |
		         ((((uint32)(uint8)(int32)LTCLAMP((float)pObj->m_nG95 + vDynamic.y + (float)nAmbG, 0.0f, 255.0f)) <<  8) & 0x0000ff00);
		nColor = (nColor & 0xffffff00) |
		         (( (uint32)(uint8)(int32)LTCLAMP((float)pObj->m_nB96 + vDynamic.z + (float)nAmbB, 0.0f, 255.0f))        & 0x000000ff);
	}

	// -- 0x10062c84: the quad ------------------------------------------- //
	aQuad[0].x = fCx - fHalfW;
	aQuad[0].y = fHalfH + fCy;
	aQuad[0].z = fZ;
	aQuad[0].m_Diffuse10.m_n  = nColor;
	aQuad[0].m_Specular14.m_n = nSpecular;
	aQuad[0].tu = fU0;
	aQuad[0].tv = fV0;

	aQuad[1].x = fHalfW + fCx;
	aQuad[1].y = fHalfH + fCy;
	aQuad[1].z = fZ;
	aQuad[1].m_Diffuse10.m_n  = nColor;
	aQuad[1].m_Specular14.m_n = nSpecular;
	aQuad[1].tu = fU1;
	aQuad[1].tv = fV0;

	aQuad[2].x = fHalfW + fCx;
	aQuad[2].y = fCy - fHalfH;
	aQuad[2].z = fZ;
	aQuad[2].m_Diffuse10.m_n  = nColor;
	aQuad[2].m_Specular14.m_n = nSpecular;
	aQuad[2].tu = fU1;
	aQuad[2].tv = fV1;

	aQuad[3].x = fCx - fHalfW;
	aQuad[3].y = fCy - fHalfH;
	aQuad[3].z = fZ;
	aQuad[3].m_Diffuse10.m_n  = nColor;
	aQuad[3].m_Specular14.m_n = nSpecular;
	aQuad[3].tu = fU0;
	aQuad[3].tv = fV1;

	pVerts = aQuad;
	nVerts = 4;

	// -- 0x10062d9b ----------------------------------------------------- //
	fSaveNear = g_f1008b908;

	if (pObj->m_nFlags88 & 0x40)
		g_f1008b908 = 0.01f;

	nClip = g_n1008b874;

	if (g_p1008d7a0)
	{
		nClip &= 1;

		if (!nClip)
			goto Ldraw;
	}

	pOut      = g_p1008b87c;
	pOutStart = g_p1008b87c;
	pIn       = pVerts;
	nIn       = nVerts;

	// ------------------------------------------------------------------ //
	//  The six clip passes.  Everything except the plane test, the
	//  denominator, the numerator and which of x/y/z the plane pins is
	//  identical; the macro below is expanded once per plane, which is
	//  what the six near-identical shipped blocks are.
	// ------------------------------------------------------------------ //

#define CLIP61A30_HEAD(BIT, INSIDE)                                          \
	if (nClip & (BIT))                                                       \
	{                                                                        \
		int32 abInside[CLIP61A30_MAX];                                       \
                                                                             \
		g_n1008be44++;                                                       \
                                                                             \
		nInside = 0;                                                         \
		for (i = 0; i < nIn; i++)                                            \
			nInside += (abInside[i] = (INSIDE) ? 1 : 0);                     \
                                                                             \
		if (nInside == 0)                                                    \
			goto Ldone;                                                      \
                                                                             \
		if (nInside != nIn)                                                  \
		{                                                                    \
			pOutStart = pOut;                                                \
                                                                             \
			j     = nIn - 1;                                                 \
			pPrev = &pIn[nIn - 1];                                           \
			pWalk = &pIn[0];                                                 \
                                                                             \
			for (i = 0; i < nIn; i++)                                        \
			{                                                                \
				if (abInside[j])                                             \
					*pOut++ = *pPrev;                                        \
                                                                             \
				if (abInside[j] != abInside[i])                              \
				{

#define CLIP61A30_TAIL                                                       \
					sub_10023380(pPrev, pWalk, pOut, fT);                    \
					pOut++;                                                  \
				}                                                            \
                                                                             \
				j     = i;                                                   \
				pPrev = pWalk;                                               \
				pWalk++;                                                     \
			}                                                                \
                                                                             \
			pIn  = pOutStart;                                                \
			nIn  = pOut - pOutStart;                                         \
			pOut = pOut + nIn;                                               \
		}                                                                    \
	}

	// ---- 0x10062e2f: the NEAR plane, z >= g_f1008b908 ----------------- //
	CLIP61A30_HEAD(0x01, pIn[i].z >= g_f1008b908)
					fDen = pWalk->z - pPrev->z;

					if (fDen < -0.00001f || fDen > 0.00001f)
						fT = (g_f1008b908 - pPrev->z) / fDen;
					else
						fT = 0.0f;

					pOut->x = (pWalk->x - pPrev->x) * fT + pPrev->x;
					pOut->y = (pWalk->y - pPrev->y) * fT + pPrev->y;
					pOut->z = g_f1008b908;
	CLIP61A30_TAIL

	// ---- 0x1006315e: the LEFT plane, -z < x --------------------------- //
	CLIP61A30_HEAD(0x04, -pIn[i].z < pIn[i].x)
					fDen = (pWalk->x - pPrev->x) + pWalk->z - pPrev->z;

					if (fDen < -0.00001f || fDen > 0.00001f)
						fT = -(pPrev->x + pPrev->z) / fDen;
					else
						fT = 0.0f;

					pOut->y = (pWalk->y - pPrev->y) * fT + pPrev->y;
					fVal    = (pWalk->z - pPrev->z) * fT + pPrev->z;
					pOut->z = fVal;
					pOut->x = -fVal;
	CLIP61A30_TAIL

	// ---- 0x100634a6: the TOP plane, y < z ----------------------------- //
	CLIP61A30_HEAD(0x08, pIn[i].y < pIn[i].z)
					fDen = (pWalk->y - pPrev->y) - pWalk->z + pPrev->z;

					if (fDen < -0.00001f || fDen > 0.00001f)
						fT = -(pPrev->y - pPrev->z) / fDen;
					else
						fT = 0.0f;

					pOut->x = (pWalk->x - pPrev->x) * fT + pPrev->x;
					fVal    = (pWalk->z - pPrev->z) * fT + pPrev->z;
					pOut->y = fVal;
					pOut->z = fVal;
	CLIP61A30_TAIL

	// ---- 0x100637e2: the RIGHT plane, x < z --------------------------- //
	CLIP61A30_HEAD(0x10, pIn[i].x < pIn[i].z)
					fDen = (pWalk->x - pPrev->x) - pWalk->z + pPrev->z;

					if (fDen < -0.00001f || fDen > 0.00001f)
						fT = -(pPrev->x - pPrev->z) / fDen;
					else
						fT = 0.0f;

					pOut->y = (pWalk->y - pPrev->y) * fT + pPrev->y;
					fVal    = (pWalk->z - pPrev->z) * fT + pPrev->z;
					pOut->x = fVal;
					pOut->z = fVal;
	CLIP61A30_TAIL

	// ---- 0x10063b22: the BOTTOM plane, -z < y ------------------------- //
	CLIP61A30_HEAD(0x20, -pIn[i].z < pIn[i].y)
					fDen = (pWalk->y - pPrev->y) + pWalk->z - pPrev->z;

					if (fDen < -0.00001f || fDen > 0.00001f)
						fT = -(pPrev->y + pPrev->z) / fDen;
					else
						fT = 0.0f;

					pOut->x = (pWalk->x - pPrev->x) * fT + pPrev->x;
					fVal    = (pWalk->z - pPrev->z) * fT + pPrev->z;
					pOut->z = fVal;
					pOut->y = -fVal;
	CLIP61A30_TAIL

	// ---- 0x10063e75: the FAR plane, z <= g_f1008b910 ------------------ //
	CLIP61A30_HEAD(0x02, pIn[i].z <= g_f1008b910)
					fDen = pWalk->z - pPrev->z;

					if (fDen < -0.00001f || fDen > 0.00001f)
						fT = (g_f1008b910 - pPrev->z) / fDen;
					else
						fT = 0.0f;

					pOut->x = (pWalk->x - pPrev->x) * fT + pPrev->x;
					pOut->y = (pWalk->y - pPrev->y) * fT + pPrev->y;
					pOut->z = g_f1008b910;
	CLIP61A30_TAIL

	// -- 0x100641a3 ----------------------------------------------------- //
	pVerts = pIn;
	nVerts = nIn;

Ldraw:
	// -- 0x100641bb: project ------------------------------------------- //
	if (pObj->m_nFlags88 & 0x4)
	{
		if (-20.0f + fZ >= 7.0f)
			fBias = -20.0f;
		else
			fBias = 7.0f - fZ;

		for (i = 0; i < nVerts; i++)
			sub_1005f9a0(&pVerts[i], &pVerts[i], fBias);
	}
	else if (pObj->m_nFlags88 & 0x40)
	{
		for (i = 0; i < nVerts; i++)
			sub_1005f9a0(&pVerts[i], &pVerts[i], 7.0f);
	}
	else
	{
		sub_1005fa80(pVerts, nVerts);
	}

	// -- 0x100642ce: D3DPT_TRIANGLEFAN, FVF 0x1c4 ----------------------- //
	g_pD3DDevice->lpVtbl->DrawPrimitive(g_pD3DDevice, 6, 0x1c4,
	                                    pVerts, nVerts, 0);

Ldone:
	// -- 0x1006430a ----------------------------------------------------- //
	g_f1008b908 = fSaveNear;

Lcleanup:
	// -- 0x10064315: the four scoped savers unwound in reverse order.
	//    Written by hand rather than as ~C1000adc0 so that /O2 cannot
	//    duplicate the epilogue and change the call sequence.
	g_pD3DDevice->lpVtbl->SetRenderState(g_pD3DDevice,
	                                     sFogColor.m_nState00,
	                                     sFogColor.m_nOld04);
	g_pD3DDevice->lpVtbl->SetRenderState(g_pD3DDevice,
	                                     sFogEnable.m_nState00,
	                                     sFogEnable.m_nOld04);
	g_pD3DDevice->lpVtbl->SetRenderState(g_pD3DDevice,
	                                     sDestBlend.m_nState00,
	                                     sDestBlend.m_nOld04);
	g_pD3DDevice->lpVtbl->SetRenderState(g_pD3DDevice,
	                                     sSrcBlend.m_nState00,
	                                     sSrcBlend.m_nOld04);
}
