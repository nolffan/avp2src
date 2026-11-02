// *********************************************************************** //
//
//  w54_1000ae20_osoiqip.cpp -- big0ae20.cpp (0x1000ae20, and the out-of-
//  line LTMatrix::Inverse that is 0x1000ac10), w2_1000b6c0_os.cpp
//  (0x1000b6c0) and bigb910.cpp (0x1000b910) as ONE /Os /Oi- /Qip
//  translation unit.  Shipped, the four are contiguous
//  (0x1000ac10..0x1000d934) and chained by /Qip register entries:
//
//    * 0x1000b6c0 calls 0x1000b910 (`call 0x1000b910` at 0x1000b860) with
//      the object in %eax -- 0x1000b910 has NO stack entry: it opens
//      `push %ebp ; mov %esp,%ebp ; push %eax ; mov $0x1b78,%eax ; call
//      _chkstk ; mov 0x1b78(%esp),%eax`, the register argument saved
//      across _chkstk;
//    * 0x1000b910 calls 0x1000ae20 (`call 0x1000ae20` at 0x1000d337) with
//      %eax/%edx/%ecx -- 0x1000ae20 is register-only too.
//
//  A register-only callee exists only as a static of its caller's TU, so
//  0x1000ae20 and 0x1000b910 are `static` here and the three files'
//  bodies follow in address order (b6c0's code sits after bigb910.cpp's
//  declarations, which it needs).  0x1000b910's shape is /Os (pushes after
//  _chkstk, one pad in 8228 bytes); /Oi- for the CRT sqrt calls.
//  Measured (difflib over normalised instructions, matched ratio):
//                       before (own files)       merged /Os /Oi- /Qip
//      0x1000ae20       0.875  (/Os /Oi- /Qip)   0.886  attempted -> near
//      0x1000b6c0       0.719  (/Os)             0.726  near, .07 -> .11
//      0x1000b910       0.096  (/O2 /Oi-)        0.379  (8771 bytes, ship
//                                                       8228; was 10240)
//      0x1000ac10       identical                identical
//  /Qip_no_inlining and /Qsfalign16 change nothing here; /O2 /Oi- /Qip
//  /Qip_no_inlining is worse for all three (0.730 / 0.638 / 0.098).
//
//  Declarations the three files shared are kept once (big0ae20.cpp's):
//  bigb910.cpp's D3DDev7 view of g_pD3DDevice gives way to big0ae20.cpp's
//  D3DDevice (the same two slots, +0x50 and +0x64), and w2_1000b6c0's copy
//  of big24020.cpp's queue element to bigb910.cpp's identical Elemb910.
//  The three original headers follow unchanged.
//
// *********************************************************************** //

// *********************************************************************** //
//
//  MODULE:   BIG_0AE20 (.CPP)
//
//  PURPOSE:  One function -- 0x1000ae20, 2,196 bytes, 30 calls
//            (6 direct to 5 distinct callees, 24 indirect), 73 bytes per
//            call.  ONE caller, 0x1000d337.
//
//  ------------------------------------------------------------------- //
//  THE EXTENT.  Verified against the bytes before anything was decoded.
//  ------------------------------------------------------------------- //
//
//  The stated range 0x1000ae20..0x1000b6b4 is CORRECT; 0x1000b6b1 is the
//  first byte of padding, so the body is 0x1000ae20..0x1000b6b0.  Five
//  checks, every one of them run:
//
//    * ONE `push %ebp ; mov %esp,%ebp` in the range, at 0x1000ae20, and
//      ONE `ret`, at 0x1000ae45.  There is no second prologue and no
//      second epilogue.  Every other exit reaches 0x1000ae3f
//      (`pop %ebx ; pop %esi ; pop %edi ; mov %ebp,%esp ; pop %ebp ; ret`)
//      either by falling into the destructor tail at 0x1000b589 or by the
//      single `jmp 0x1000ae3f` at 0x1000b61d.
//    * The 1,643 bytes AFTER that `ret` are /O2 cold-block sinking, not a
//      second function.  Every block below it uses THIS frame and jumps
//      BACKWARDS into it: 0x1000b622 (L_ElseLOD) -> 0x1000b589,
//      0x1000b658 and 0x1000b65f (the two search-hit trampolines) ->
//      0x1000b479, 0x1000b666 (L_MakeRec, non-empty arm) -> 0x1000b493 /
//      0x1000b589, 0x1000b692 (L_MakeRec, empty arm) -> 0x1000b493 /
//      0x1000b589.  The last instruction of the body, 0x1000b6ac, is
//      `jmp 0x1000b493`.
//    * EVERY branch target in the range is inside the range.  Counted:
//      33 distinct targets, the lowest 0x1000ae3f and the highest
//      0x1000b692 -- nothing below the entry, nothing above the last
//      instruction, and nothing in the padding 0x1000b6b1..0x1000b6bf.
//    * 0x1000b6b1 is `lea 0x0(%esi),%esi` and 0x1000b6b4..0x1000b6bf is
//      twelve `nop` -- the compiler's multi-byte padding.  The next
//      function starts at 0x1000b6c0 with `push %ebp ; mov %esp,%ebp ;
//      sub $0x18,%esp ; push %edi ; push %edi ; push %esi ; push %ebx`.
//    * The function BEFORE it, 0x1000adc0, is d3dbatcha.cpp's decoded
//      C1000adc0 render-state-saver constructor: 85 bytes ending
//      `mov %ebx,%eax ; pop %ebx ; mov %ebp,%esp ; pop %ebp ; ret $0x8`
//      at 0x1000ae12, then padding.  So 0x1000ae20 is a genuine function
//      start -- and its immediate neighbour is the very class this
//      function inlines four times.
//
//    * ONE caller, and it is the entry proof.  0x1000d322 is
//      `mov -0xc0(%ebp),%eax ; mov -0x90(%ebp),%edx ;
//       mov -0x1a70(%ebp),%ecx ; add $0xfffffff0,%esp ; call 0x1000ae20 ;
//       add $0x10,%esp`.  Three arguments in %eax/%edx/%ecx and sixteen
//      DEAD bytes of outgoing area -- tier Z, "register-only ABI, no
//      stack entry".  That is
//      decoded as an ordinary function whose arguments arrive in
//      registers; it is written __cdecl below and the ABI is noted.  The
//      three spills at 0x1000ae2d..0x1000ae33 put %eax at -0x8, %edx at
//      -0x18 and %ecx at -0x14, and -0x18/-0x14 are the pointer and the
//      count handed to the DrawPrimitive at 0x1000b3b5.  AND THE CALLER
//      PROVES ALL THREE: at 0x1000d312 it issues its OWN vtable-+0x64
//      DrawPrimitive with type 6, FVF 0x1c4, `mov %eax,0xc(%esp)` from
//      -0x90(%ebp) and `mov %eax,0x10(%esp)` from -0x1a70(%ebp) -- the
//      same pointer and the same count it then hands over in %edx and
//      %ecx -- and it guards the call with `mov -0xc0(%ebp),%eax ;
//      mov 0x2c(%eax),%eax ; test %eax,%eax ; je`, which is the very
//      light list this function walks at +0x2c of argument 0.
//
//  ------------------------------------------------------------------- //
//  WHAT IT IS -- the LMDynamic (dynamic-lightmap) additive second pass
//  ------------------------------------------------------------------- //
//
//  Draw one already-transformed screen-space triangle fan again, once per
//  dynamic light on the surface, generating the light's projected texture
//  coordinates per vertex.  Six tells, all read off the bytes:
//
//    * 0x10091200, 0x100911c0 and 0x100911e0 are the three console
//      variables d3dr5b.cpp decoded at 0x1006aca0:
//          g_10091200.sub_10021f40("LMDynamic",      1.0f,  0, 0);
//          g_100911e0.sub_10021f40("LMDynamicScale", 2.5f,  0, 0);
//          g_100911c0.sub_10021f40("LMDynamicSize",  16.0f, 0, 0);
//      big21f90.cpp pinned the record: +0x00 is the value as int32 and
//      +0x04 the value as float.  This function's FIRST act is
//      `if (!g_varLMDynamic.m_nValue00) return;` (0x1000ae36); its second
//      is to stamp (uint8)LMDynamicSize into the surface's +0x48 and
//      +0x49 (0x1000ae52 / 0x1000ae5e); and the value it gets back from
//      the light object is multiplied by 0x100911e4, which is
//      LMDynamicScale's record + 4 -- m_fValue04 (0x1000b278).
//    * the blend state it installs is 0x1b ALPHABLENDENABLE := 1,
//      0x13 SRCBLEND := 2 (D3DBLEND_ONE), 0x14 DESTBLEND := 2 and
//      0x22 FOGCOLOR := 0 -- black fog on an additive pass, over
//      geometry that has already been drawn.
//    * 0x1006a860, the lightmap builder it calls, opens with the SAME
//      `mov 0x10091200,%eax ; test %eax,%eax ; je` guard.
//    * the DrawPrimitive at 0x1000b3b5 is vtable +0x64 with primitive
//      type 6 (D3DPT_TRIANGLEFAN) and FVF 0x1c4 = XYZRHW | DIFFUSE |
//      SPECULAR | TEX1; 0x20 is the vertex stride of the uv loop
//      (`add $0x20,%eax` at 0x1000b372) -- exactly that FVF.  Only +0x18
//      and +0x1c (u and v) are written; +0x00/+0x04/+0x08 are read.  The
//      caller drew the SAME array with the same type and the same count.
//    * the uv generator inverts the 4x4 block at 0x1008ba5c
//      (= 0x1008b880 + 0x1dc, the transform object big124a4.cpp,
//      big6d910.cpp and big70d10.cpp all read) and uses it to take each
//      screen-space vertex back to world space, subtract the light's
//      +0x08/+0x0c/+0x10 and project onto the two basis vectors the
//      surface hands back through its m_p28 vtable slot 0.  That is a
//      projected light map, and both texture coordinates land at
//      `dot * fScale + 0.5f` (0x100a44e4 and 0x100a44e8 were dumped and
//      are both 0x3f000000).
//    * the tail binds a texture record through the block / LRU /
//      SetTexture machinery big2ef34.cpp, big6d910.cpp, big61a30.cpp and
//      big43e10.cpp have each decoded.
//
//  ------------------------------------------------------------------- //
//  THE SIBLING -- 0x1003dd50, 2,260 bytes, also tier Z, one caller.
//  0x1003dd50 IS THE SAME SOURCE COMPILED IN A DIFFERENT TU, and it is
//  the strongest single piece of evidence in this file.
//  ------------------------------------------------------------------- //
//
//  A mnemonic diff of 0x1000ae20..0x1000b6b0 against
//  0x1003dd50..0x1003e612, with addresses masked, aligns 600 instructions
//  against 616 and comes back with exactly SEVEN differing hunks.
//  Everything else -- the four render-state savers, the two
//  texture-stage states, the whole 460-byte Gauss-Jordan inverse, the
//  per-vertex projective transform and dot products, the DrawPrimitive,
//  the entire texture-bind block and its four sunk arms -- is instruction
//  for instruction identical.  The seven, verbatim from the diff:
//
//    1. 0x1000ae49..0x1000ae5e: this function does two
//       `mov 0x100911c0,%dl ; mov %dl,0x48/0x49(%eax)` stores that the
//       sibling does not have.
//    2. 0x1000b215 `xor %eax,%eax` against 0x1003e133
//       `mov 0x100911c0,%eax` -- this function hands 0x1006d550 (0,0) as
//       its third and fourth arguments, the sibling hands it
//       LMDynamicSize twice.  0x1006d550's own body settles that these
//       are the SAME request: `test %eax,%eax ; ja` on argument 2 falls
//       through to `movzbl 0x48(%esi)` when it is zero, and `test ; jbe`
//       on argument 3 falls through to `movzbl 0x49(%esi)` when it is
//       zero.  So (1) and (2) are one difference and each confirms the
//       other's field.
//    3. 0x1000af07 `movl $0x2,0x8(%esp)` against 0x1003de25
//       `movl $0x9,0x8(%esp)`: SRCBLEND is 2 (D3DBLEND_ONE) here and
//       9 (D3DBLEND_DESTCOLOR) there.
//    4. 0x1000b243/0x1000b227 against 0x1003e144/0x1003e14b -- a
//       one-instruction scheduling rotation of `mov %edi,(%esp)`, not a
//       source difference.
//    5. 0x1000b2c1..0x1000b2cb against 0x1003e1e2..0x1003e1e8 -- the y
//       and x terms of ONE dot product are emitted in the other order.
//       Same three products, same sum; a scheduler choice.
//    6. the sibling repeats its whole DrawPrimitive block under a
//       `mov <global>,%eax ; test ; je` at 0x1003e2d9 that this function
//       does not have.
//    7. 0x1000b41a `mov 0x1008beb8,%eax ; mov (%eax),%edi` against
//       0x1003e37a `mov -0x8(%ebp),%eax ; mov 0x28(%eax),%eax ;
//       mov 0x30(%eax),%edi` -- the texture block comes from the global
//       here and from `pThis->m_p28->+0x30` there.  big2ef34.cpp reads
//       the same +0x28 -> +0x30 chain.
//
//  The four float constants also sit in different pools (0x100a44cc,
//  0x100a44d8, 0x100a44e0, 0x100a44e4, 0x100a44e8, 0x100a44f0 here;
//  0x100a9aec, 0x100a9af8, 0x100a9b00, 0x100a9b04, 0x100a9b08,
//  0x100a9b10 there) -- two translation units of one source.  The
//  sibling's fourth argument to 0x1006a860 is 0x40000000 (2.0f) where
//  this one's is 0x3f800000 (1.0f).
//
//  Everything this file claims about the shape of the arithmetic is
//  therefore claimed about two independent compilations of it.
//
//  ------------------------------------------------------------------- //
//  SUPERSEDED: the TU is built /Os /Oi- /Qip (moved in
//  build/engine.sh from D3D_OI_TUS, /O2 /Oi-, to D3D_QIP_OS_OI_TUS).
//  The proof is 0x1000ac10, the function two before this one: it is
//  icl's /Qip out-of-line copy of the SDK's inline LTMatrix::Inverse()
//  (recorded below at the matrix), and THIS file, with the
//  real LTMatrix and MakeInverse inlined here, emits that copy
//  byte-identical at /Os /Oi- /Qip -- and at /Os /Oi- /Qip /Qsfalign16
//  and /Os /Oi- /Qip /Qip_no_inlining (0x1000ae20 1917 at all three),
//  while /Os /Qip /Qip_no_inlining is 378 bytes off it and /O2 /Oi-
//  emits no copy.  The same change takes 0x1000ae20 itself from
//  2491 to 1917 bytes differing (2210 bytes against the old 2839).
//  The readings below are kept for the record.
//  ------------------------------------------------------------------- //
//  WHICH FLAG:  /O2.  Two of the /O2 tells, none against.
//  ------------------------------------------------------------------- //
//
//    * OUTGOING ARGUMENTS are `add $-N,%esp` then `mov %reg,K(%esp)` at
//      every COM call site -- 0x1000ae5b, 0x1000ae82, 0x1000aea7,
//      0x1000aecf, 0x1000aef4, ... 0x1000b5f8.  The four `push` at
//      0x1000b23d..0x1000b244 and the pairs at 0x1000b66a / 0x1000b692
//      are the __cdecl calls to 0x1006a860 and 0x10039bd0 -- three real
//      arguments plus icl's dead 4-byte reserve, with `add $0x10,%esp`
//      after (and, at 0x1000b24a, `add $0xc,%esp` because the fourth
//      slot is immediately RE-USED as 0x1006d210's argument).
//    * COLD BLOCKS ARE SUNK PAST THE SINGLE EPILOGUE -- the 1,643 bytes
//      after 0x1000ae45.  /Os cross-jumps instead of sinking.
//    * THE EPILOGUE IS NOT DUPLICATED, but there is only one exit, so
//      that signal is silent rather than against.
//    * `&local` is `lea -0x184(%ebp),%eax`, but every site is a frame
//      local and none is a `&p->m`, so the table's third row is silent.
//
//  ------------------------------------------------------------------- //
//  THE FRAME -- `sub $0x1a8`, 424 bytes, and it decomposes exactly
//  ------------------------------------------------------------------- //
//
//      -0x1a8 .. -0x191   24 B  never addressed.  0x18 is exactly the
//                       largest outgoing area the body uses (the six
//                       dwords of the DrawPrimitive at 0x1000b382);
//                       icl's reserve.
//      -0x190 .. -0x185   12 B  vBasis0   (vtable-slot-0 out-param 0)
//      -0x184 .. -0x179   12 B  vBasis1   (vtable-slot-0 out-param 1)
//      -0x178 .. -0x171    8 B  CStateSave #1  state 0x1b
//      -0x170 .. -0x169    8 B  CStateSave #2  state 0x13
//      -0x168 .. -0x161    8 B  CStateSave #3  state 0x14
//      -0x160 .. -0x159    8 B  CStateSave #4  state 0x22
//      -0x158              4 B  nOldAddress (GetTextureStageState 0x0c)
//      -0x154 .. -0x115   64 B  MakeInverse's NRVO return slot, 4x4
//      -0x114 .. -0x95   128 B  Inverse()'s `float mat[4][8]`
//      -0x94  .. -0x85    16 B  Inverse()'s `int rowMap[4]`
//      -0x84  .. -0x45    64 B  mInv, the inverse
//      -0x44  .. -0x29    28 B  the descriptor 0x1006d550 fills and
//                       0x1006d210 consumes
//      -0x28               4 B  compiler temp (`iRow*2`, 0x1000b176)
//      -0x24               4 B  never addressed
//      -0x20 .. -0x19      8 B  Inverse()'s fLargest, spilled `fstpl`
//      -0x18               4 B  pVerts    (argument 1, from %edx)
//      -0x14               4 B  nVerts    (argument 2, from %ecx)
//      -0x10 .. -0x9       8 B  a SHARED compiler temp with four disjoint
//                       live ranges: the vtable function pointer at
//                       0x1000ae78, fScale as a `fstpl` double at
//                       0x1000b28c/0x1000b295/0x1000b37f, nStage at
//                       0x1000b427/0x1000b483/0x1000b66b/0x1000b693, and
//                       the alpha reference at 0x1000b522.  Because the
//                       slot is SHARED none of the four is a named local
//                       -- see the coin-flip note on nStage below.
//      -0x8                4 B  pThis     (argument 0, from %eax)
//      -0x4                4 B  compiler temp (%edi save, 0x1000b062)
//
//    424 = 24 + 12 + 12 + 8*4 + 4 + 64 + 128 + 16 + 64 + 28 + 4 + 4 + 8
//        + 4 + 4 + 8 + 4 + 4.  Nothing is left over and nothing is short.
//    THE ONE INFERENCE IN THAT TABLE is the descriptor's 28 bytes.  Its
//    base is read off `lea -0x44(%ebp),%ecx`; its top is bounded above by
//    -0x28, the first slot this function itself addresses, and below by
//    +0x18, the highest field either callee touches (0x1006d550 writes
//    +0x0c, +0x10, +0x14 and +0x18; 0x1006d210 reads +0x0c and +0x10).
//    0x1c is the only size consistent with both, and it is used below.
//
//    DECLARATION ORDER is lowest slot first, the convention big2ef34.cpp,
//    big6d910.cpp and big70d10.cpp all record.  The named locals below
//    are declared in exactly that order; the three MakeInverse/Inverse
//    slots between nOldAddress and mInv are the inline expansion's own
//    temps and have no declaration in this function.
//
//  ------------------------------------------------------------------- //
//  REGION MAP, with what pins each region
//  ------------------------------------------------------------------- //
//
//  0x1000ae20..0x1000ae45  PROLOGUE and `if (!g_varLMDynamic) return;`.
//      The three register arguments are spilled to -0x8/-0x18/-0x14 in
//      that order.  `push %edi ; push %edi` is icl's dead 4-byte slot in
//      front of the three real register saves.
//
//  0x1000ae46..0x1000ae7c  m_b48 = m_b49 = (uint8)LMDynamicSize -- TWO
//      separate `mov 0x100911c0,%dl`, so two source statements, not one
//      value stored twice -- and then
//      `pThis->m_p28->lpVtbl->Slot00(&vBasis0, &vBasis1)`.  __thiscall:
//      the object stays in %ecx and is NOT pushed, and nothing cleans the
//      8-byte outgoing area, so the callee is `ret $0x8`.
//
//  0x1000ae7d..0x1000afb0  FOUR RENDER-STATE SAVERS.  Each is
//      `Get(nState, &m_nOld04)` through vtable +0x54 followed by
//      `Set(m_nState00, value)` through +0x50, with the Get handed the
//      PARAMETER (%eax, still live) and the Set handed the MEMBER
//      RELOADED FROM THE FRAME -- the exact split d3dbatcha.cpp decoded
//      on the OUT-OF-LINE copy of this same constructor at 0x1000adc0,
//      the function immediately before this one.
//      States 0x1b := 1, 0x13 := 2, 0x14 := 2, 0x22 := 0.
//      THEY ARE OBJECTS AND NOT OPEN-CODED STATEMENTS because the four
//      restores at 0x1000b589..0x1000b61a replay them in EXACT REVERSE
//      ORDER -- destructor order for block-scope objects and nothing
//      else -- and because their four 8-byte frame slots ascend in
//      construction order.  Neither half is a `call`, so both bodies are
//      defined in-class here (implicitly inline, which is what /Ob1
//      needs); d3dbatcha.cpp's TU kept them out of line.
//
//  0x1000afb1..0x1000b034  THE TWO TEXTURE-STAGE STATES, stage 0.
//      Get(0, 0x0c, &nOldAddress) through +0x90, Set(0, 0x0c, 3) and
//      Set(0, 1, 2) through +0x94.  The restores at 0x1000b3c2 and
//      0x1000b3ef are in the SAME order, not reversed, and the second
//      restores a CONSTANT 4 rather than a saved value -- so these are
//      plain statements, NOT the CStageStateSet class of big43e10.cpp.
//      0x0c = D3DTSS_ADDRESS := 3 (D3DTADDRESS_CLAMP): a projected map
//      must not tile.  1 = D3DTSS_COLOROP := 2 (D3DTOP_SELECTARG1),
//      restored to 4 (D3DTOP_MODULATE).
//
//  0x1000b035..0x1000b201  LTMatrix::MakeInverse() (ltmatrix.h line 514)
//      with LTMatrix::Inverse() (line 404) inlined inside it.  Reused,
//      not re-derived: d3dr4b.cpp decoded the pair at 0x10015740 /
//      0x1000ac10.  Every landmark of the header body is present and in
//      order -- see the banner on the class below.
//
//  0x1000b202..0x1000b3c1  THE PER-LIGHT LOOP.  Guarded do-while over
//      pThis->m_pLights2c chained through +0x00 (`mov (%ebx),%ebx` at
//      0x1000b3b8); three failure tests, all to the loop tail.  Body:
//      fill the descriptor (0x1006d550, __thiscall, `ret $0x10`); build
//      the lightmap (0x1006a860, __cdecl, four dwords, the fourth a raw
//      float immediate); commit it (0x1006d210, __thiscall, `ret $0x4`);
//      ask the light for its size through its +0x04 interface's vtable
//      slot +0x38; then the per-vertex uv loop and the DrawPrimitive.
//
//  0x1000b3c2..0x1000b419  RESTORE the two texture-stage states.
//
//  0x1000b41a..0x1000b588  FIND-OR-MAKE AND BIND THE TEXTURE RECORD.
//      big2ef34.cpp's block, field for field; see its banner and the
//      notes inline below.
//
//  0x1000b589..0x1000b61c  THE FOUR DESTRUCTORS, in reverse order.
//
//  0x1000b61d..0x1000b6b0  SUNK COLD BLOCKS: L_ElseLOD, the two
//      search-hit trampolines, and L_MakeRec's two arms.
//
//  ------------------------------------------------------------------- //
//  CALL-FREE COVERAGE -- what the call-sequence test cannot witness here
//  ------------------------------------------------------------------- //
//
//  73 bytes per call overall, which is in the strong band, but
//  the calls are not evenly spread.  Two regions carry real code with no
//  call in them at all and must be quoted alongside any figure:
//
//    * 0x1000b0fa..0x1000b201, 264 bytes -- the tail of the pivot search
//      and ALL of the row-reduction and copy-out of LTMatrix::Inverse.
//      Witnessed only by the `fabs` inside the pivot search above it.
//      What pins it instead: it is a line-for-line correspondence with
//      src/LT2/sdk/inc/ltmatrix.h's Inverse(), including the two "preserve
//      accuracy" stores (0x1000b154 `movl $0x3f800000,(%ebx,%edi,4)` =
//      pRow[iRow] = 1.0f, 0x1000b19f `movl $0x0,(%edx,%ecx,2)` =
//      pScaleRow[iRow] = 0.0f), the 0.001f seed (0x100a44cc, dumped) and
//      the -1 iLargest sentinel.
//    * 0x1000b278..0x1000b381, 267 bytes -- the whole per-vertex uv
//      loop.  Zero calls.  What pins it instead: the four constants were
//      dumped (1.0f, 1.0f, 0.5f, 0.5f), the sixteen matrix operands are
//      exactly the sixteen slots of the 64-byte block at -0x84, the
//      three light operands are exactly +0x08/+0x0c/+0x10, the two basis
//      operands are exactly the two 12-byte out-params of the slot-0
//      call, the stride is 0x20 and the only stores are +0x18 and +0x1c
//      -- and the SIBLING at 0x1003dd50 emits the identical sequence bar
//      one scheduling rotation (difference 5 above).
//
//  ------------------------------------------------------------------- //
//  THE EXPECTED CALL-SEQUENCE RESULT, predicted from the disassembly before
//  any build.  The shipped sequence, in address order, is
//
//     1 *0x00 (m_p28 slot 0)   0x1000ae7b
//     2 *0x54  0x1000aea4      3 *0x50  0x1000aec7    state 0x1b
//     4 *0x54  0x1000aef1      5 *0x50  0x1000af14    state 0x13
//     6 *0x54  0x1000af3e      7 *0x50  0x1000af61    state 0x14
//     8 *0x54  0x1000af8b      9 *0x50  0x1000afae    state 0x22
//    10 *0x90  0x1000afd9     11 *0x94  0x1000b004   12 *0x94  0x1000b02f
//    13 0x1007fc98/0x1007fc90 fabs      0x1000b0e1
//    14 0x1006d550  0x1000b22d
//    15 0x1006a860  0x1000b245
//    16 0x1006d210  0x1000b255
//    17 *0x38 (light iface)    0x1000b276
//    18 *0x64 DrawPrimitive    0x1000b3b5
//    19 *0x94  0x1000b3e9     20 *0x94  0x1000b414
//    21 *0x8c  0x1000b518     22 *0x50  0x1000b543   23 *0xbc  0x1000b57d
//    24 *0x50  0x1000b5ab     25 *0x50  0x1000b5d0
//    26 *0x50  0x1000b5f5     27 *0x50  0x1000b61a   the four destructors
//    28 *0xbc  0x1000b644     L_ElseLOD, sunk
//    29 0x10039bd0  0x1000b672    30 0x10039bd0  0x1000b69a  sunk
//
//  The source below issues those thirty in exactly that order: the six
//  direct ones resolve by target, the twenty-four indirect ones by
//  position.  Two things could move the count and both are recognisable:
//
//    * `fabs` (13).  icl holds it as an intrinsic
//      and expands to the `fabs` instruction where the shipped build
//      calls 0x1007fc90.  `#pragma function(fabs)` is set at the top for
//      that reason, but if it does not take, ONE shipped-only row at
//      0x1007fc90 is the known toolchain gap, not a decode error.
//    * the four destructors (24-27).  All exits funnel through the single
//      `L_Done: return;` so icl should emit them once, as shipped.  If it
//      instead duplicates them per exit the extra rows are ours-only and
//      all `*0x50`.
//
//  ------------------------------------------------------------------- //
//  COIN-FLIPS, each with both spellings so one build settles it
//  ------------------------------------------------------------------- //
//
//   1. `nStage`.  Written as a named local because that is the only way
//      to say it in C, but its frame slot (-0x10) is SHARED with three
//      other live ranges, which named locals are not.  The alternative
//      is no local at all -- `g_n1008d7d0` written out at all four uses
//      (0x1000b427's compare source, 0x1000b483's index, and the second
//      argument of the two 0x10039bd0 calls).  Neither changes a call.
//
//   2. `fScale`, `fW`, `fWX/fWY/fWZ`, `fDX/fDY/fDZ`.  None of the eight
//      has a frame slot -- all live on the x87 stack, which peaks at six
//      -- so the source may have been one nested expression per texture
//      coordinate instead of eight named floats.  Written as floats
//      here because /O2 keeps an address-free float local in a register
//      and the result is the same code.
//
//   3. `mInv = g_mat1008ba5c.MakeInverse();` (declare, then assign) vs
//      `LTMatrix mInv = g_mat1008ba5c.MakeInverse();` (copy-init).  The
//      declare-then-assign form is written because the shipped code has
//      BOTH `rep movsl`s -- the NRVO copy into -0x154 at 0x1000b045 and
//      the copy out to -0x84 at 0x1000b200.  Copy-init would let icl
//      elide the second.
//
//   4. `Basis0ae20::Slot00` as a C++ pure virtual (written) vs a
//      __thiscall function-pointer table reached through an explicit
//      `lpVtbl` member.  Both give `mov (%ecx),%eax ; mov (%eax),%eax ;
//      call *%eax` with the object in %ecx and two stack arguments.  The
//      virtual spelling is the one the class deserves; the COM spelling
//      is what the device needs (the light interface is __thiscall
//      with `this` also pushed, see LightIface0ae20), and both are
//      used in this file for that reason.
//
//   5. `g_mat1008ba5c` declared as a 4x4 matrix at 0x1008ba5c (written)
//      vs `((XForm0ae20*)g_1008b880)->m_Mat1dc` in big6d910.cpp's
//      spelling.  Both fold to the absolute address 0x1008ba5c as the
//      `rep movsl` source; the first is chosen because the sixteen
//      floats are consumed here as ONE object and never individually.
//
//   6. The descriptor's 0x1c bytes.  Only the bound is proved (see the
//      frame table); the alternative is a struct naming +0x0c, +0x10,
//      +0x14 and +0x18 with four bytes of head padding.  Nothing in
//      this function reads it, so no call moves either way.
//
//  ------------------------------------------------------------------- //
//  NO `// UNDECODED:` GAPS.  Every byte of 0x1000ae20..0x1000b6b0 is
//  accounted for by a region above and is written below.
//  ------------------------------------------------------------------- //
//
// *********************************************************************** //

#include "ltbasedefs.h"
#include <math.h>

// 0x1007fc90 is the CRT's fabs, and Inverse() calls it at 0x1000b0e1.
// icl 4.0 holds fabs as an intrinsic and would expand it to `fabs`;
// big4b720.cpp uses `#pragma function(sqrt, fabs)` for exactly this
// reason and it is repeated here.  The pragma has
// not been measured to move bytes, so if this one call goes missing it is
// the known toolchain gap, not a decode error.
#pragma function(fabs)


// ======================================================================= //
//  THE D3D7 DEVICE AT 0x1008d988.  Six vtable slots are reached:
//  +0x50 SetRenderState        (0x1000aec7, 0x1000af14, 0x1000af61,
//                               0x1000afae, 0x1000b543, 0x1000b5ab,
//                               0x1000b5d0, 0x1000b5f5, 0x1000b61a)
//  +0x54 GetRenderState        (0x1000aea4, 0x1000aef1, 0x1000af3e,
//                               0x1000af8b)
//  +0x64 DrawPrimitive         (0x1000b3b5)
//  +0x8c SetTexture            (0x1000b518)
//  +0x90 GetTextureStageState  (0x1000afd9)
//  +0x94 SetTextureStageState  (0x1000b004, 0x1000b02f, 0x1000b3e9,
//                               0x1000b414)
//  Same declaration big2ef34.cpp, big61a30.cpp and big6d910.cpp use.
// ======================================================================= //
struct D3DDevice;

struct D3DDeviceVtbl
{
	uint8			m_pad_00[0x50];
	long (__stdcall* SetRenderState)(D3DDevice*, uint32, uint32);   // +0x50
	long (__stdcall* GetRenderState)(D3DDevice*, uint32, uint32*);  // +0x54
	uint8			m_pad_58[0x64 - 0x58];
	long (__stdcall* DrawPrimitive)(D3DDevice*, uint32, uint32, void*,
	                                uint32, uint32);                // +0x64
	uint8			m_pad_68[0x8c - 0x68];
	long (__stdcall* SetTexture)(D3DDevice*, uint32, void*);        // +0x8c
	long (__stdcall* GetTextureStageState)(D3DDevice*, uint32, uint32,
	                                       uint32*);                // +0x90
	long (__stdcall* SetTextureStageState)(D3DDevice*, uint32, uint32,
	                                       uint32);                 // +0x94
};

struct D3DDevice
{
	D3DDeviceVtbl*	lpVtbl;
};


// IDirectDrawSurface7: only +0xbc, SetLOD (0x1000b57d and 0x1000b644).
struct DDSurface;

struct DDSurfaceVtbl
{
	uint8			m_pad_00[0xbc];
	long (__stdcall* SetLOD)(DDSurface*, uint32);                   // +0xbc
};

struct DDSurface
{
	DDSurfaceVtbl*	lpVtbl;
};


extern D3DDevice*	g_pD3DDevice;                       // 0x1008d988


// ======================================================================= //
//  THE CONSOLE-VARIABLE RECORD, big21f90.cpp's CD3DConVar.  Only the two
//  value members are reached from here: +0x00 as an int32 (0x1000ae36
//  `mov 0x10091200,%eax ; test`, 0x1000ae49 and 0x1000ae55
//  `mov 0x100911c0,%dl`) and +0x04 as a float (0x1000b278
//  `fmuls 0x100911e4`, which is 0x100911e0 + 4).  Declared with the same
//  layout big21f90.cpp pinned; the rest is padding here.
// ======================================================================= //
struct CD3DConVar
{
	int32		m_nValue00;                             // +0x00
	float		m_fValue04;                             // +0x04
	uint8		m_pad_08[0x20 - 0x08];
};

extern CD3DConVar	g_varLMDynamic;                     // 0x10091200
extern CD3DConVar	g_varLMDynamicSize;                 // 0x100911c0
extern CD3DConVar	g_varLMDynamicScale;                // 0x100911e0


// ======================================================================= //
//  THE 4x4 MATRIX, and MakeInverse / Inverse from src/LT2/sdk/inc/ltmatrix.h.
//
//  d3dr4b.cpp decoded the out-of-line pair at 0x10015740 (MakeInverse)
//  and 0x1000ac10 (Inverse).  Here both are INLINE -- there is no call in
//  0x1000b035..0x1000b201 except the `fabs` at 0x1000b0e1 -- so the
//  bodies have to be written in-class, and they are transcribed from the
//  header rather than re-derived.  Every landmark of the header body is
//  present, in order:
//
//    0x1000b035  `mov $0x1008ba5c,%esi ; lea -0x154(%ebp),%edi ;
//                 mov $0x10,%ecx ; rep movsl`        ret = *this
//    0x1000b04d  the i loop: four dwords copied per row (the inner
//                `mov 0x10(%edx,%eax,4)` / `mov %edi,0x10(%ecx,%eax,4)`
//                with %eax running -4..-1), then FOUR zero stores at
//                -0xf8/-0xfc/-0x100/-0x104 (pOut[7]..pOut[4], emitted
//                high to low), then `movl $0x3f800000,0x10(%ecx,%esi,4)`
//                = pOut[i+4] = 1.0f, then `mov %esi,-0x94(%ebp,%esi,4)`
//                = rowMap[i] = i.  Row stride 0x20 = eight floats, m
//                stride 0x10 = four.  That is the header's setup loop
//                exactly.
//    0x1000b0ae  `flds 0x100a44cc` -- dumped, 0x3a83126f = 0.001f, the
//                header's fLargest seed -- and `mov $0xffffffff,%esi`,
//                iLargest = -1.
//    0x1000b0c7  the pivot search: rowMap[iTest] * 0x20, + iRow*4,
//                `call 0x1007fc90` (fabs), `fcomp` against fLargest,
//                `jae` past the two updates.
//    0x1000b102  `cmp $0xffffffff,%esi ; je` -- return LTFALSE.  It
//                lands on the SAME copy-out as the normal exit, which is
//                MakeInverse ignoring Inverse's result and returning ret
//                either way.
//    0x1000b10b  the three-way row swap through iTemp.
//    0x1000b11f  `flds 0x100a44f0` (dumped, 1.0f) then
//                `fdivs (%ebx,%edi,4)` -- mul = 1.0f / pRow[iRow]; the
//                j loop runs pRow to pRow+0x20, eight floats.
//    0x1000b154  `movl $0x3f800000,(%ebx,%edi,4)` -- pRow[iRow] = 1.0f.
//    0x1000b15b  the elimination loop, `cmp %edi,%esi ; je` for
//                `if (i == iRow) continue;`, `fchs` for the negation,
//                %eax running -8..-1 over the eight columns, and
//                `movl $0x0,(%edx,%ecx,2)` with %ecx = iRow*2 --
//                pScaleRow[iRow] = 0.0f.
//    0x1000b1b6  the copy-out: pIn = mat[rowMap[i]] + 4 is
//                `lea -0x104(%ebp,%eax,1)` = -0x114 + 0x10, four dwords
//                per row.
//    0x1000b1ef  `lea -0x84(%ebp),%edi ; lea -0x154(%ebp),%esi ;
//                 mov $0x10,%ecx ; rep movsl`        mInv = ret
//
//  The return type is written LTBOOL and discarded, as the header has it.
//
//  The bodies are no longer transcribed -- this is the
//  SDK's own LTMatrix (ltbasedefs.h -> ltmatrix.h), MakeInverse called
//  as the header has it.  Byte-for-byte the same 0x1000ae20 as the
//  transcribed Mat0ae20 copy (measured: 1917 both ways), and it is what
//  places 0x1000ac10.  0x1000ac10 = icl's /Qip IPO out-of-line copy of
//  the inline Inverse, emitted because the inline MakeInverse calls it;
//  that copy is the one with sixteen never-touched bytes at the top of
//  its frame (`sub $0xbc` where a plain /Os /Oi- function is `sub $0xac`
//  -- probed).  It used to be written by hand in
//  w2_1000ac10_osoi.cpp as a separate function and stayed 3 bytes off
//  for exactly that reason (all of what was measured there: double
//  locals, dead arrays, /Qip, /Qrcd, /O1, /Qpc64, /QIfist, /G6, /Ob0..2);
//  it now comes out of this file BYTE-IDENTICAL as ?Inverse@LTMatrix@@,
//  sitting just below this TU's other function, as in the image.
//  d3dr4b.cpp's out-of-line MakeInverse (0x10015740) still calls it under
//  its old LTMatrix10015740::sub_1000ac10 spelling, which the link
//  aliases onto this definition by address.
// ======================================================================= //


// 0x1008ba5c = 0x1008b880 + 0x1dc -- the 4x4 projective block of the
// transform object every other user of 0x1008b880 in the project takes as
// an opaque `uint8[]`.  Copied WHOLE here (`mov $0x1008ba5c,%esi ;
// mov $0x10,%ecx ; rep movsl` at 0x1000b035), so it is one matrix-shaped
// object at this site; big6d910.cpp's XForm6d910 names the same sixteen
// floats m_f1dc..m_f218.
extern LTMatrix		g_mat1008ba5c;                      // 0x1008ba5c


// ======================================================================= //
//  THE RENDER-STATE SAVER.  Eight bytes, state id at +0x00 and the saved
//  value at +0x04, exactly as d3dbatcha.cpp's C1000adc0 -- which is this
//  same constructor compiled OUT OF LINE at 0x1000adc0, the function
//  immediately before this one.  Here it is inlined at all four sites, so
//  both bodies are written in-class.
//
//  Note that the constructor's Get is handed the PARAMETER and its Set is
//  handed the MEMBER reloaded from the frame (`mov -0x178(%ebp),%eax` at
//  0x1000aeaa and its three copies) -- so the source names the member the
//  second time, not the argument.  d3dbatcha.cpp records the same split.
// ======================================================================= //
class CStateSave
{
public:
	CStateSave(uint32 nState, uint32 nValue)
	{
		m_nState00 = nState;

		g_pD3DDevice->lpVtbl->GetRenderState(g_pD3DDevice, nState,
		                                     &m_nOld04);
		g_pD3DDevice->lpVtbl->SetRenderState(g_pD3DDevice, m_nState00,
		                                     nValue);
	}

	~CStateSave()
	{
		g_pD3DDevice->lpVtbl->SetRenderState(g_pD3DDevice, m_nState00,
		                                     m_nOld04);
	}

	uint32	m_nState00;                                 // +0x00
	uint32	m_nOld04;                                   // +0x04
};


// ======================================================================= //
//  THE TEXTURE RECORD AND ITS BLOCK.  Field for field big2ef34.cpp's
//  Tex2ef34 / TexBlock2ef34 (and big61a30.cpp's and big6d910.cpp's): the
//  bind block here reads exactly the same offsets in the same order.
//    +0x04/+0x08 the inverse-size pair    (0x1000b54f, 0x1000b552)
//    +0x0c       the surface              (0x1000b50f, 0x1000b56d)
//    +0x10       the byte count           (0x1000b4e7)
//    +0x14       the frame stamp          (0x1000b4a7, 0x1000b4f4)
//    +0x16       the alpha reference      (0x1000b51e)
//    +0x1c       the CheapLTLink          (0x1000b4af..0x1000b4df)
//    +0x30       the next-in-block chain  (0x1000b45d, 0x1000b470,
//                                          0x1000b682, 0x1000b68a)
//    +0x42       the stage                (0x1000b451, 0x1000b493, ...)
//    +0x44       the dirty flag           (0x1000b562, 0x1000b622)
// ======================================================================= //
struct Link0ae20                                        // ltlink.h CheapLTLink
{
	Link0ae20*	m_pPrev;                                // +0x00
	Link0ae20*	m_pNext;                                // +0x04
	void*		m_pData;                                // +0x08

	inline void Remove()
	{
		m_pPrev->m_pNext = m_pNext;
		m_pNext->m_pPrev = m_pPrev;
		TieOff();
	}

	inline void TieOff()
	{
		m_pPrev = m_pNext = this;
	}

	inline void AddAfter(Link0ae20* pLink)
	{
		pLink->m_pPrev = this;
		pLink->m_pNext = m_pNext;
		pLink->m_pPrev->m_pNext = pLink->m_pNext->m_pPrev = pLink;
	}
};

inline void dl_Insert0ae20(Link0ae20* pAfter, Link0ae20* pLink)
{
	pAfter->AddAfter(pLink);
}

inline void dl_Remove0ae20(Link0ae20* pLink)
{
	pLink->Remove();
}

// The texture's base part (big729c0.cpp's TexBase729c0): the bind goes
// through a base-class pointer.
struct TexBase0ae20
{
	void*		m_pVtbl00;                              // +0x00
	float		m_fInvW04;                              // +0x04
	float		m_fInvH08;                              // +0x08
	DDSurface*	m_pSurface0c;                           // +0x0c
	uint32		m_nBytes10;                             // +0x10
	uint16		m_nStamp14;                             // +0x14
	uint16		m_nAlphaRef16;                          // +0x16
	uint8		m_pad_18[0x1c - 0x18];
	Link0ae20	m_Link1c;                               // +0x1c
	uint8		m_pad_28[0x30 - 0x28];
};

struct Tex0ae20 : public TexBase0ae20
{
	Tex0ae20*	m_pNext30;                              // +0x30
	uint8		m_pad_34[0x42 - 0x34];
	uint16		m_nStage42;                             // +0x42
	uint16		m_nDirty44;                             // +0x44
};

// The block: a list head at +0x0c (0x1000b439) and a sixteen-bit stamp at
// +0x30 (`mov %ax,0x30(%edi)` at 0x1000b43e).
struct TexBlock0ae20
{
	uint8		m_pad_00[0x0c];
	Tex0ae20*	m_pHead0c;                              // +0x0c
	uint8		m_pad_10[0x30 - 0x10];
	uint16		m_nStamp30;                             // +0x30
};

// The pair published per stage at 0x1008e664 -- `mov %ecx,(%edx,%eax,8)`
// and `mov %esi,0x4(%edx,%eax,8)` at 0x1000b555 / 0x1000b558.  Both moves
// are INTEGER moves of floats, as everywhere else in the project.
struct TexScale0ae20
{
	float		m_fInvW00;                              // +0x00
	float		m_fInvH04;                              // +0x04
};

// 0x1008cbcc: only +0x48 is touched here (`add %edx,0x48(%eax)` at
// 0x1000b4ea) -- the same field big2ef34.cpp and big6d910.cpp record.
struct Stats0ae20
{
	uint8		m_pad_00[0x48];
	uint32		m_nBytes48;                             // +0x48
};


// ======================================================================= //
//  THE SURFACE (argument 0) AND ITS PIECES
//
//  pThis is only touched at four offsets:
//    +0x28  the object whose vtable slot 0 hands back the two basis
//           vectors (0x1000ae4f, 0x1000ae74) -- and the same +0x28 that
//           0x1006a860 reads out of its second argument at 0x1006a884
//    +0x2c  the head of the light list (0x1000b205)
//    +0x48  \  the two bytes 0x1006d550 falls back on when its third and
//    +0x49  /  fourth arguments are zero (0x1006d569, 0x1006d580)
// ======================================================================= //
struct Vec0ae20
{
	float	x;                                          // +0x00
	float	y;                                          // +0x04
	float	z;                                          // +0x08
};

// The object at pThis->+0x28.  0x1000ae74 is `mov (%ecx),%eax ;
// mov (%eax),%eax ; call *%eax` with the object in %ecx and NOT on the
// stack, and only the two out-params in the 8-byte outgoing area which
// nothing cleans -- so this is an ORDINARY C++ VIRTUAL CALL on vtable
// slot 0 (__thiscall, `ret $0x8`), not the COM `lpVtbl->F(p)` spelling
// the device below uses.  Written as an abstract
// class so icl emits exactly that shape; the class has no other member
// this function touches.
class Basis0ae20
{
public:
	virtual void Slot00(Vec0ae20* pOut0, Vec0ae20* pOut1) = 0;      // +0x00
};

// The light's +0x04 interface.  Only vtable slot +0x38 is called
// (0x1000b26a..0x1000b276): the interface is loaded TWICE, once into
// %ecx as `this` and once pushed as the only stack argument, and it
// answers a float in st(0).  The callee is lithtech.exe's 0x0045b550
// (`flds 0x1a8(%ecx) ; ret $0x4`) -- __thiscall with one dword -- so
// the slot is a pointer to member taking the interface again, called
// `(p->*p->lpVtbl->Slot38)(p)`.  (It was written __stdcall, which
// pushed the pointer but left %ecx stale: M09S02 faulted in the exe
// reading 0x1a8 off a NULL `this`.)
struct LightIface0ae20Vtbl;

struct LightIface0ae20
{
	LightIface0ae20Vtbl*	lpVtbl;
};

struct LightIface0ae20Vtbl
{
	uint8	m_pad_00[0x38];
	float (LightIface0ae20::* Slot38)(LightIface0ae20*);             // +0x38
};

// The light record.  +0x00 chains the list (0x1000b3b8), +0x04 is the
// interface above, and +0x08/+0x0c/+0x10 are the three floats subtracted
// from the world-space vertex at 0x1000b338 / 0x1000b307 / 0x1000b2e8 --
// so they are read as a position, x then y then z.
struct Light0ae20
{
	Light0ae20*			m_pNext00;                      // +0x00
	LightIface0ae20*	m_pIface04;                     // +0x04
	float				m_fX08;                         // +0x08
	float				m_fY0c;                         // +0x0c
	float				m_fZ10;                         // +0x10
};

struct Surf0ae20
{
	uint8			m_pad_00[0x28];
	Basis0ae20*		m_p28;                              // +0x28
	Light0ae20*		m_pLights2c;                        // +0x2c
	uint8			m_pad_30[0x48 - 0x30];
	uint8			m_b48;                              // +0x48
	uint8			m_b49;                              // +0x49
};


// ======================================================================= //
//  THE DESCRIPTOR, 0x1c bytes at -0x44.  0x1006d550 fills +0x0c, +0x10,
//  +0x14 and +0x18 (`mov %edx,0xc(%ebx)` at 0x1006d575, `0x10(%ebx)` at
//  0x1006d584, `movl $0x0,0x14(%ebx)` at 0x1006d589 and `0x18(%ebx)` at
//  0x1006d5d2) and 0x1006d210 reads +0x0c and +0x10 back (0x1006d222,
//  0x1006d22b).  Nothing in THIS function ever reads it, so no member is
//  named; the size is the frame's, bounded above by -0x28.
//
//  Both members are callee-pop __thiscall: 0x1006d550 is `ret $0x10`
//  (four stack dwords) and 0x1006d210 is `ret $0x4` (one).
// ======================================================================= //
class CLightDesc0ae20
{
public:
	int32	sub_1006d550(Surf0ae20* pSurf, uint32 nA, uint32 nB,
	                     uint32 nC);                    // 0x1006d550
	int32	sub_1006d210(void* pMap);                   // 0x1006d210

	uint8	m_pad_00[0x1c];
};


// ======================================================================= //
//  EXTERNALS.  Relocations are masked, so only the shapes matter.
// ======================================================================= //

// 0x1008beb8 -- d3dr2c.cpp's C1000da50Data*.  Only its +0x00 is read here
// (`mov 0x1008beb8,%eax ; mov (%eax),%edi` at 0x1000b41a/0x1000b425), and
// what comes out is used as a texture block: +0x0c is walked as a list
// head and +0x30 is stamped.  The sibling 0x1003dd50 reaches THE SAME
// KIND of object through pThis->m_p28->+0x30 instead (difference 7).
struct Cur0ae20
{
	TexBlock0ae20*	m_pBlock00;                         // +0x00
};

extern Cur0ae20*		g_p1008beb8;                    // 0x1008beb8

extern uint16			g_w1008b870;                    // 0x1008b870
extern uint16			g_w1008bea8;                    // 0x1008bea8
extern uint32			g_n1008bd80;                    // 0x1008bd80
extern Stats0ae20*		g_p1008cbcc;                    // 0x1008cbcc
extern uint32			g_n1008d7d0;                    // 0x1008d7d0  stage
// 0x1008e634 is the 12-byte list HEAD OBJECT, not a pointer variable.
// 0x10038a90 settles it: at 0x10038e3d it plants the IMMEDIATE $0x1008e634
// into the new link's m_pPrev and then loads 0x1008e638 -- i.e. &head and
// head.m_pNext.  This function reads the head's +0 field instead, which is
// the same instruction a pointer load would emit, so the old spelling was
// byte-correct here and wrong about what the object is.  The two functions
// also insert at opposite ends: 0x10038a90 at the front (after the head),
// this one after head.m_pPrev, i.e. at the back.
extern Link0ae20		g_Link1008e634;                 // 0x1008e634
extern TexScale0ae20	g_a1008e664[];                  // 0x1008e664
extern Tex0ae20*		g_a1008e684[];                  // 0x1008e684

// 0x1006a860 -- __cdecl, four stack dwords, plain `ret`; its own body
// opens with the SAME `mov 0x10091200,%eax ; test ; je` guard this
// function opens with, reads argument 1's +0x28 -> +0x34 and argument
// 2's +0x04 and +0x10, and answers a pointer in %eax.
extern "C" void* sub_1006a860(CLightDesc0ae20* pDesc, Surf0ae20* pSurf,
                              Light0ae20* pLight, float fVal);

// 0x10039bd0 -- d3dmid.cpp's stack-ABI entry; the body is at 0x10039bdc.
// Three arguments; big2ef34.cpp, big6d910.cpp and big43e10.cpp all reach
// it with the same (block, stage, bLink) triple.
extern "C" Tex0ae20* sub_10039bd0(TexBlock0ae20* pBlock, uint32 nStage,
                                  uint32 bLink);


// ======================================================================= //
//  THE BIND: big729c0.cpp's BindBlock729c0 (w24_10059380_os.cpp's
//  BindBlock59380, w3_10014580_os.cpp's 0x10015320 as an inline; also
//  big47230.cpp's three) over this file's types.  Both 0x1000ae20 and
//  0x1000b6c0 end in one copy of it with a LOD of 0.
//
//  FindRec is the one departure: big729c0.cpp's `return pCur` inside the
//  loop / `return LTNULL` after it gives the shipped search exactly (the
//  `xor` on the exhausted path) in BOTH functions, but in this TU it also
//  makes icl give 0x1000b6c0 the aligned-stack shim (`push %ebx ; mov
//  %esp,%ebx ; and $-16,%esp`) for its register call to 0x1000b910, which
//  the shipped 0x1000b6c0 does not have (with 0x1000b910 out of the TU the
//  shim goes away; `for`+`break`+`return pCur` and a `while` with the test
//  in the condition shim too).  The pRet spelling below avoids the shim and
//  leaves `mov $0` for `xor` plus the break trampolines' order:
//  0x1000b6c0 598 of 592 bytes, 4 blocks (was 609, 31); 0x1000ae20 2212 of
//  2196, 11 blocks (was 2208, 27; with the return spelling its bind is
//  exact and 0x1000b6c0 shims).
// ======================================================================= //
static inline Tex0ae20* FindRec0ae20(Tex0ae20* pHead, uint32 nStage)
{
	Tex0ae20* pCur;

	Tex0ae20* pRet = LTNULL;
	for (pCur = pHead; pCur; pCur = pCur->m_pNext30)
	{
		if (pCur->m_nStage42 == (uint8)nStage)
		{
			pRet = pCur;
			break;
		}
	}

	return pRet;
}

static inline uint32 StageOf0ae20(Tex0ae20* pRec)
{
	return pRec->m_nStage42;
}

static inline void SetScale0ae20(TexScale0ae20* pScales, uint32 nStage, float fInvW, float fInvH)
{
	pScales[nStage].m_fInvW00 = fInvW;
	pScales[nStage].m_fInvH04 = fInvH;
}

static inline void BindRec0ae20(Tex0ae20* pRec)
{
	TexBase0ae20* pTex = pRec;

	g_a1008e684[StageOf0ae20(pRec)] = (Tex0ae20*)pTex;

	if (pTex->m_nStamp14 != g_w1008bea8)
	{
		dl_Remove0ae20(&pRec->m_Link1c);
		dl_Insert0ae20(g_Link1008e634.m_pPrev, &pRec->m_Link1c);

		g_p1008cbcc->m_nBytes48 += pTex->m_nBytes10;

		pTex->m_nStamp14 = g_w1008bea8;
	}

	g_pD3DDevice->lpVtbl->SetTexture(g_pD3DDevice, (uint8)StageOf0ae20(pRec),
	                                 pTex->m_pSurface0c);

	if (pTex->m_nAlphaRef16)
		g_pD3DDevice->lpVtbl->SetRenderState(g_pD3DDevice, 0x18,
		                                     pTex->m_nAlphaRef16);

	SetScale0ae20(g_a1008e664, StageOf0ae20(pRec), pTex->m_fInvW04, pTex->m_fInvH08);

	g_n1008bd80++;
}

static inline int32 BindBlock0ae20(TexBlock0ae20* pBlock, uint32 nStage, uint32 nLOD)
{
	Tex0ae20* pRec;
	Tex0ae20* pHead;
	Tex0ae20* pNew;

	if (!pBlock)
		return 0;

	pBlock->m_nStamp30 = g_w1008b870;

	pHead = pBlock->m_pHead0c;
	pRec  = LTNULL;

	if (pHead)
		pRec = FindRec0ae20(pHead, nStage);

	if (!pRec)
		goto L_MakeRec;

	if (pRec == g_a1008e684[nStage])
		goto L_ElseLOD;

L_Bind:
	BindRec0ae20(pRec);

	if (pRec->m_nDirty44 != nLOD)
	{
		pRec->m_pSurface0c->lpVtbl->SetLOD(pRec->m_pSurface0c, nLOD);
		pRec->m_nDirty44 = (uint16)nLOD;
	}

	return 1;

L_ElseLOD:
	if (pRec->m_nDirty44 != nLOD)
	{
		pRec->m_pSurface0c->lpVtbl->SetLOD(pRec->m_pSurface0c, nLOD);
		pRec->m_nDirty44 = (uint16)nLOD;
	}

	return 1;

L_MakeRec:
	if (pBlock->m_pHead0c)
	{
		pNew = sub_10039bd0(pBlock, nStage, 1);

		if (!pNew)
			return 0;

		pNew->m_pNext30  = pHead->m_pNext30;
		pHead->m_pNext30 = pNew;
		pRec = pNew;

		goto L_Bind;
	}

	pRec = sub_10039bd0(pBlock, nStage, 0);

	if (!pRec)
		return 0;

	goto L_Bind;
}


// ======================================================================= //
//  THE VERTEX.  Stride 0x20 (`add $0x20,%eax` at 0x1000b372) and FVF
//  0x1c4 = D3DFVF_XYZRHW | DIFFUSE | SPECULAR | TEX1 (0x1000b39b), which
//  is x, y, z, rhw, diffuse, specular, u, v = 32 bytes exactly.  This
//  function READS +0x00/+0x04/+0x08 and WRITES +0x18/+0x1c; the four
//  middle dwords it never touches are named from the FVF and nothing
//  else.
// ======================================================================= //
struct Vert0ae20
{
	float	m_fX00;                                     // +0x00
	float	m_fY04;                                     // +0x04
	float	m_fZ08;                                     // +0x08
	float	m_fRHW0c;                                   // +0x0c
	uint32	m_nDiffuse10;                               // +0x10
	uint32	m_nSpecular14;                              // +0x14
	float	m_fU18;                                     // +0x18
	float	m_fV1c;                                     // +0x1c
};


// ======================================================================= //
//  0x1000ae20 .. 0x1000b6b0   (2,196 bytes)   1 caller, 0x1000d337
//
//  The three arguments arrive in %eax, %edx and %ecx with a dead 16-byte
//  outgoing area (tier Z).  They are written as ordinary
//  parameters.
// ======================================================================= //
static void sub_1000ae20(Surf0ae20* pThis, Vert0ae20* pVerts, uint32 nVerts)
{
	// -- 0x1000ae36 ----------------------------------------------------- //
	//  The whole body is inside one positive test, and this is the ONLY
	//  path that reaches 0x1000ae3f without running a destructor -- which
	//  is why the four savers below have to be declared AFTER it.
	// ------------------------------------------------------------------- //
	if (!g_varLMDynamic.m_nValue00)
		return;

	// The frame order, lowest slot first.  vBasis0 at -0x190 and vBasis1
	// at -0x184 are the two 12-byte out-params of the slot-0 call.
	Vec0ae20			vBasis0;                        // -0x190
	Vec0ae20			vBasis1;                        // -0x184

	// -- 0x1000ae49 / 0x1000ae55 ---------------------------------------- //
	//  TWO separate `mov 0x100911c0,%dl`, so two source statements.  The
	//  bytes are what 0x1006d550 falls back on for its third and fourth
	//  arguments -- see difference 2 in the banner.
	// ------------------------------------------------------------------- //
	pThis->m_b48 = (uint8)g_varLMDynamicSize.m_nValue00;
	pThis->m_b49 = (uint8)g_varLMDynamicSize.m_nValue00;

	// -- 0x1000ae74: __thiscall, two stack out-params, `ret $0x8`.
	pThis->m_p28->Slot00(&vBasis0, &vBasis1);

	// -- 0x1000ae7d..0x1000afb0 ----------------------------------------- //
	//  0x1b ALPHABLENDENABLE := 1, 0x13 SRCBLEND := 2 (D3DBLEND_ONE),
	//  0x14 DESTBLEND := 2, 0x22 FOGCOLOR := 0.  Additive, with black
	//  read-only.  The four slots ascend in construction order and the
	//  four restores at 0x1000b589..0x1000b61a run in exact reverse.
	// ------------------------------------------------------------------- //
	CStateSave			sAlphaBlend(0x1b, 1);           // -0x178
	CStateSave			sSrcBlend(0x13, 2);             // -0x170
	CStateSave			sDestBlend(0x14, 2);            // -0x168
	// 0x22 is D3DRENDERSTATE_FOGCOLOR (34).  It was labelled ZWRITEENABLE
	// here, which is 0x0e -- big26728.cpp defines that constant, and
	// 0x1000b910's decode of the same state save reached FOGCOLOR
	// independently.  Comment only; the emitted code is unchanged.
	CStateSave			sFogColor(0x22, 0);             // -0x160

	uint32				nOldAddress;                    // -0x158

	// -- 0x1000afb1..0x1000b034 ----------------------------------------- //
	//  Stage 0.  0x0c = D3DTSS_ADDRESS := 3 (D3DTADDRESS_CLAMP) -- a
	//  projected map must not tile -- and 1 = D3DTSS_COLOROP := 2
	//  (D3DTOP_SELECTARG1).  Plain statements, not a scoped class: the
	//  restores below are in the SAME order, and the second one restores
	//  a CONSTANT 4 rather than anything saved.
	// ------------------------------------------------------------------- //
	g_pD3DDevice->lpVtbl->GetTextureStageState(g_pD3DDevice, 0, 0x0c,
	                                           &nOldAddress);
	g_pD3DDevice->lpVtbl->SetTextureStageState(g_pD3DDevice, 0, 0x0c, 3);
	g_pD3DDevice->lpVtbl->SetTextureStageState(g_pD3DDevice, 0, 1, 2);

	// -- 0x1000b035..0x1000b201: the inlined MakeInverse; see the class.
	LTMatrix			mInv;                           // -0x84

	mInv = g_mat1008ba5c.MakeInverse();

	CLightDesc0ae20		desc;                           // -0x44

	Light0ae20*			pLight;
	void*				pMap;
	Vert0ae20*			pWalk;
	uint32				nLeft;
	float				fScale;
	float				fW, fWX, fWY, fWZ, fDX, fDY, fDZ;

	TexBlock0ae20*		pBlock;
	Tex0ae20*			pRec;
	Tex0ae20*			pCur;
	uint32				nStage;

	// =================================================================== //
	//  0x1000b202..0x1000b3c1 -- THE PER-LIGHT LOOP.
	//
	//  `mov 0x2c(%eax),%ebx ; test %ebx,%ebx ; je 0x1000b3c2` then a body
	//  ending `mov (%ebx),%ebx ; test %ebx,%ebx ; jne 0x1000b212` -- the
	//  guarded do-while /O2 makes of a while loop over a +0x00 chain.
	//  All THREE failure tests inside jump to 0x1000b3b8, the tail, not
	//  out of the loop.
	// =================================================================== //
	pLight = pThis->m_pLights2c;

	while (pLight)
	{
		// -- 0x1000b22d: __thiscall on `desc`, four stack dwords,
		//    `ret $0x10`.  The two zeroes make 0x1006d550 read the two
		//    bytes stamped at the top of this function -- see the banner.
		if (desc.sub_1006d550(pThis, 1, 0, 0))
		{
			// -- 0x1000b245: __cdecl, four dwords, the fourth pushed as
			//    the raw immediate 0x3f800000.  Only 0xc of the 0x10 is
			//    popped because the top slot is re-used one instruction
			//    later as 0x1006d210's argument.
			pMap = sub_1006a860(&desc, pThis, pLight, 1.0f);

			// -- 0x1000b255: __thiscall, one dword, `ret $0x4`.
			if (desc.sub_1006d210(pMap))
			{
				// -- 0x1000b262: the THIRD test, on the pointer that
				//    came back from 0x1006a860.
				if (pMap)
				{
					// -- 0x1000b26a: the light's +0x04 interface is
					//    loaded TWICE, into %ecx as `this` and pushed as
					//    the only argument to its own vtable slot +0x38.
					//    The float that comes back is scaled by
					//    LMDynamicScale (0x100911e4 = the record + 4) and
					//    reciprocated; 0x100a44d8 was dumped and is 1.0f.
					fScale = 1.0f
					       / ((pLight->m_pIface04->*pLight->m_pIface04
					              ->lpVtbl->Slot38)(pLight->m_pIface04)
					          * g_varLMDynamicScale.m_fValue04);

					// ---------------------------------------------- //
					//  0x1000b284..0x1000b381 -- THE UV LOOP.  267
					//  bytes and NOT ONE CALL; see the call-free note
					//  in the banner.
					//
					//  Per vertex: undo the projection with mInv, take
					//  the result to world space, subtract the light
					//  position, and project the difference onto the
					//  two basis vectors the surface handed back.
					//
					//  icl emits the four rows in the order w, z, y, x
					//  because the dot products below associate
					//  last-term-backwards and the z term is wanted
					//  first; the divide is `fdivrs 0x100a44e0` (dumped,
					//  1.0f) and both adds are `fadds` of 0x100a44e4 /
					//  0x100a44e8 (dumped, both 0.5f).  The three
					//  subtractions are 0x1000b338 (x, `fsubs 0x8(%ebx)`),
					//  0x1000b307 and 0x1000b2e8 -- world minus light in
					//  all three, which for the two DE-form pops means
					//  `de e2` FSUBRP (st0 - st2) and `de eb` FSUBP
					//  (st3 - st0) respectively.
					// ---------------------------------------------- //
					pWalk = pVerts;
					nLeft = nVerts;

					while (nLeft--)
					{
						fW = 1.0f / (pWalk->m_fX00 * mInv.m[3][0]
						           + pWalk->m_fY04 * mInv.m[3][1]
						           + pWalk->m_fZ08 * mInv.m[3][2]
						           + mInv.m[3][3]);

						fWX = (pWalk->m_fX00 * mInv.m[0][0]
						     + pWalk->m_fY04 * mInv.m[0][1]
						     + pWalk->m_fZ08 * mInv.m[0][2]
						     + mInv.m[0][3]) * fW;

						fWY = (pWalk->m_fX00 * mInv.m[1][0]
						     + pWalk->m_fY04 * mInv.m[1][1]
						     + pWalk->m_fZ08 * mInv.m[1][2]
						     + mInv.m[1][3]) * fW;

						fWZ = (pWalk->m_fX00 * mInv.m[2][0]
						     + pWalk->m_fY04 * mInv.m[2][1]
						     + pWalk->m_fZ08 * mInv.m[2][2]
						     + mInv.m[2][3]) * fW;

						fDX = fWX - pLight->m_fX08;
						fDY = fWY - pLight->m_fY0c;
						fDZ = fWZ - pLight->m_fZ10;

						pWalk->m_fU18 = (vBasis0.x * fDX
						               + vBasis0.y * fDY
						               + vBasis0.z * fDZ) * fScale + 0.5f;

						pWalk->m_fV1c = (vBasis1.x * fDX
						               + vBasis1.y * fDY
						               + vBasis1.z * fDZ) * fScale + 0.5f;

						pWalk++;
					}

					// -- 0x1000b3b5: vtable +0x64, six dwords.  6 =
					//    D3DPT_TRIANGLEFAN, 0x1c4 = XYZRHW | DIFFUSE |
					//    SPECULAR | TEX1 -- the same call the caller
					//    made on the same array six instructions before
					//    it called this function.
					g_pD3DDevice->lpVtbl->DrawPrimitive(g_pD3DDevice, 6,
					                                    0x1c4, pVerts,
					                                    nVerts, 0);
				}
			}
		}

		pLight = pLight->m_pNext00;
	}

	// -- 0x1000b3c2 / 0x1000b3ef: the two stage states, restored in the
	//    SAME order they were set.  The second goes back to the constant
	//    4 (D3DTOP_MODULATE), not to anything that was saved.
	g_pD3DDevice->lpVtbl->SetTextureStageState(g_pD3DDevice, 0, 0x0c,
	                                           nOldAddress);
	g_pD3DDevice->lpVtbl->SetTextureStageState(g_pD3DDevice, 0, 1, 4);

	// =================================================================== //
	//  0x1000b41a..0x1000b588 -- FIND-OR-MAKE AND BIND THE RECORD.
	//  big2ef34.cpp's block with its two unbind copies removed: here
	//  every failure simply leaves the function, so all of them land on
	//  the destructor tail at 0x1000b589.
	// =================================================================== //
	BindBlock0ae20(g_p1008beb8->m_pBlock00, g_n1008d7d0, 0);

	// -- 0x1000b589..0x1000b61c: the four destructors, in reverse
	//    construction order, then the single `jmp 0x1000ae3f` to the one
	//    epilogue.
}


// ==== bigb910.cpp: declarations ====
// Round to nearest in the current FPU mode: w1_10001000.cpp's helper
// (0x10015190 out of line), as big49bb0.cpp's clipper uses it.  Inlined,
// it is the shipped `fstps t ; flds t ; fistpl n ; mov n,%dl` that each of
// the thirty clip-vertex colour bytes below shows (0x1000bdfa ...).
inline int32 RoundF(float f)
{
	int32 n;
	__asm fld f
	__asm fistp n
	return n;
}



// ======================================================================= //
//  THE 40-BYTE CLIP/DRAW VERTEX.  d3dr3a.cpp's SClipVertA, unchanged.
//  Every offset is a load or a store in these bytes: +0x00..+0x08 the
//  position, +0x0c the reciprocal w (0x1000bba9), +0x10..+0x13 the four
//  diffuse bytes in the order the clipper interpolates them -- b, g, r, a
//  at +0x10, +0x11, +0x12, +0x13 -- +0x14..+0x17 the specular dword the
//  __fastcall hook writes (only +0x17 is interpolated), +0x18/+0x1c the
//  GENERATED planar texture pair and +0x20/+0x24 the COPIED one.
//  Stride 0x28 is read off nine independent places: 0x1000ba7f,
//  0x1000bacb (via `lea (%eax,%eax,4)` then `,8`), 0x1000bbe5, 0x1000bc73,
//  0x1000bcf2, 0x1000bf7b, 0x1000d1cb, 0x1000d23f and 0x1000d3d0.
// ======================================================================= //
struct LTVec
{
	float	x;                                          // +0x00
	float	y;                                          // +0x04
	float	z;                                          // +0x08
};

struct RVert                                            // 0x28 bytes
{
	// 0x1000d9d0 -- d3dbatchb_os.cpp's 21-byte __thiscall two-field
	// setter, 13 callers.  It writes m_fU18 and m_fV1c.  DECLARED AND NOT
	// DEFINED here so the call is emitted (a tiny accessor
	// spelled as a field read makes no call at all).
	void	sub_1000d9d0(float fU, float fV);

	LTVec	m_vPos;                                     // +0x00
	float	m_fRHW0c;                                   // +0x0c
	uint8	m_nBlue10;                                  // +0x10
	uint8	m_nGreen11;                                 // +0x11
	uint8	m_nRed12;                                   // +0x12
	uint8	m_nAlpha13;                                 // +0x13
	uint8	m_nSpec14;                                  // +0x14
	uint8	m_nSpec15;                                  // +0x15
	uint8	m_nSpec16;                                  // +0x16
	uint8	m_nSpec17;                                  // +0x17
	float	m_fU18;                                     // +0x18
	float	m_fV1c;                                     // +0x1c
	float	m_fU20;                                     // +0x20
	float	m_fV24;                                     // +0x24
};


// ======================================================================= //
//  THE 32-BYTE DRAW VERTEX.  The shared buffer at g_1008d524 is indexed
//  `g_1008d520 << 5` (0x1000d28a) and walked `add $0x20` (0x1000d2c6 and
//  0x1000d3d6), and the copy in is `rep movsl $8`.  FVF 0x1c4 =
//  XYZRHW | DIFFUSE | SPECULAR | TEX1 is exactly 32 bytes.
// ======================================================================= //
struct DVert                                            // 0x20 bytes
{
	uint8	m_Head00[0x18];                             // +0x00..+0x17
	float	m_fU18;                                     // +0x18
	float	m_fV1c;                                     // +0x1c
};


// ======================================================================= //
//  THE 0x18-BYTE SOURCE ENTRY that 0x1000a700 hands back.  Stride 0x18
//  from `lea 0x18(%eax),%eax` at 0x1000ba82; the six members below are
//  every byte of it this function reads.
// ======================================================================= //
struct SrcVert                                          // 0x18 bytes
{
	LTVec*	m_pPos00;                                   // +0x00
	float	m_fU04;                                     // +0x04
	float	m_fV08;                                     // +0x08
	uint8	m_Pad0c[0x08];                              // +0x0c..+0x13
	uint8	m_nBlue14;                                  // +0x14
	uint8	m_nGreen15;                                 // +0x15
	uint8	m_nRed16;                                   // +0x16
	uint8	m_Pad17;                                    // +0x17
};


// ======================================================================= //
//  THE OBJECT, THE DYNAMIC-LIGHT LIST AND THE POLYGON PLANE.
//  +0x28 -> +0x30 -> +0x1c is the bucket chain the tail registers on;
//  +0x28 -> +0x34 is the polygon plane the light pass measures against
//  (0x1000d4ae/0x1000d4b7), the same +0x28/+0x34 big64cc0.cpp reads;
//  +0x2c is the dynamic-light list AND the gate on sub_1000ae20
//  (0x1000d31b, and big0ae20.cpp records the same +0x2c);
//  +0x50 is the uint16 vertex count the light loop uses (0x1000d595) --
//  read off the bytes, and NOT the same slot as nVerts.
// ======================================================================= //
struct SPlane
{
	LTVec	m_vNormal;                                  // +0x00
	float	m_fDist;                                    // +0x0c
};

struct LightObj
{
	uint8	m_Pad00[0x94];                              // +0x00..+0x93
	uint8	m_nRed94;                                   // +0x94
	uint8	m_nGreen95;                                 // +0x95
	uint8	m_nBlue96;                                  // +0x96
	uint8	m_Pad97[0x111];                             // +0x97..+0x1a7
	float	m_fRadius1a8;                               // +0x1a8
};

struct LightRec
{
	LightRec*	m_pNext00;                              // +0x00
	LightObj*	m_pObject04;                            // +0x04
	LTVec		m_vPos08;                               // +0x08
};

struct Elemb910                                         // 0x18 bytes
{
	void*		m_pOwner00;                             // +0x00
	uint32		m_n04;                                  // +0x04
	uint32		m_n08;                                  // +0x08
	uint32		m_n0c;                                  // +0x0c
	uint32		m_n10;                                  // +0x10
	Elemb910*	m_pNext14;                              // +0x14
};

struct Bucketb910                                       // 0x0c bytes
{
	void*		m_pTable00;                             // +0x00
	Elemb910*	m_pElements04;                          // +0x04
	Bucketb910*	m_pNextInList08;                        // +0x08
};

struct Tableb910
{
	uint8		m_Pad00[0x1c];                          // +0x00..+0x1b
	Bucketb910*	m_pBuckets1c[1];                        // +0x1c
};

struct Ctxb910
{
	uint8		m_Pad00[0x30];                          // +0x00..+0x2f
	Tableb910*	m_pTable30;                             // +0x30
	SPlane*		m_pPlane34;                             // +0x34
};

struct Objb910
{
	uint8		m_Pad00[0x28];                          // +0x00..+0x27
	Ctxb910*	m_pCtx28;                               // +0x28
	LightRec*	m_pLights2c;                            // +0x2c
	uint8		m_Pad30[0x20];                          // +0x30..+0x4f
	uint16		m_nCount50;                             // +0x50
};


// ======================================================================= //
//  THE POOL HEADER -- 0x1000a6a0's view of it.  +0x08 is the grow count
//  and +0x18 the free-list head, which is what makes 0x1008d4fc a member
//  of 0x1008d4e4 (0x1008d4e4 + 0x18) and 0x1008d4ec its grow count
//  (0x1008d4e4 + 0x08); likewise 0x1008d4e0 and 0x1008d4d0 for 0x1008d4c8.
// ======================================================================= //
struct Poolb910
{
	uint8	m_Pad00[0x08];                              // +0x00..+0x07
	uint32	m_nGrow08;                                  // +0x08
	uint8	m_Pad0c[0x0c];                              // +0x0c..+0x17
	void*	m_pFree18;                                  // +0x18
};


// ======================================================================= //
//  THE PROJECTIVE TRANSFORM BLOCK.  0x1008ba5c = 0x1008b880 + 0x1dc, and
//  the twelve `fmuls` operands 0x1008ba5c..0x1008ba94 plus the four
//  `fadds` operands 0x1008ba68/78/88/98 are +0x1dc..+0x218 of the object
//  at 0x1008b880 -- the same 4x3+w block big124a4.cpp, big6d910.cpp,
//  big70d10.cpp and 0x1000a8a0 all read.
// ======================================================================= //
struct XFormb910
{
	uint8	m_Pad00[0x1dc];                             // +0x000..+0x1db
	float	m_f1dc, m_f1e0, m_f1e4, m_f1e8;              // +0x1dc  row x
	float	m_f1ec, m_f1f0, m_f1f4, m_f1f8;              // +0x1ec  row y
	float	m_f1fc, m_f200, m_f204, m_f208;             // +0x1fc  row z
	float	m_f20c, m_f210, m_f214, m_f218;             // +0x20c  row w
};


// ======================================================================= //
//  THE DIRECT3D 7 DEVICE AT 0x1008d988.  Only +0x50 SetRenderState and
//  +0x64 DrawPrimitive are used here.
// ======================================================================= //
struct D3DDev7;

struct D3DDev7Vtbl
{
	void*	m_Slots00[20];                              // +0x00..+0x4c
	long (__stdcall* SetRenderState)(D3DDev7*, uint32, uint32);   // +0x50
	void*	m_Slots54[5];                               // +0x54..+0x60
	long (__stdcall* DrawPrimitive)(D3DDev7*, uint32 nType, uint32 nFVF,
	                                void* pVerts, uint32 nCount,
	                                uint32 nFlags);     // +0x64
};

struct D3DDev7
{
	D3DDev7Vtbl* lpVtbl;
};

#define D3DPT_TRIANGLEFAN                        6
#define D3DFVF_DRAWVERTEX                    0x1c4  // XYZRHW|DIFFUSE|SPEC|TEX1
#define D3DRENDERSTATE_FOGCOLOR                 34  // 0x22


// ======================================================================= //
//  d3dbatcha.cpp's scoped render-state saver.  The constructor
//  0x1000adc0 stores the state id at +0x00, reads the old value into
//  +0x04 and installs the new one; the restore is written out in line
//  below, which is what the shipped bytes show at 0x1000d412 and
//  0x1000d43c -- there is no destructor call.  Declared and NOT defined,
//  so the call is emitted.
// ======================================================================= //
class C1000adc0
{
public:
	C1000adc0* sub_1000adc0(uint32 nState, uint32 nValue);

	uint32	m_nState00;                                 // +0x00
	uint32	m_nOld04;                                   // +0x04
};


// ======================================================================= //
//  FILE-SCOPE STATE.  Every address is an operand in these bytes.
// ======================================================================= //

// (g_pD3DDevice: big0ae20.cpp's D3DDevice declaration above, same slots)

// The clip flag word and its scratch cursor -- d3dr3a.cpp / big16324.cpp.
extern uint32		g_1008b874;                         // 0x1008b874
extern RVert*		g_1008b87c;                         // 0x1008b87c
extern uint8		g_1008b880[];                       // 0x1008b880  the transform OBJECT (not a pointer)
extern uint32		g_1008d7a0;                         // UseD3DClip

// The near and far plane constants.  BSS, so the values are not in the
// file; the addresses are read off `fcomps`/`fsubrs` operands at
// 0x1000bc66/0x1000bd4d and 0x1000ceac/0x1000cf9e.
extern float		g_f1008b908;                        // near
extern float		g_f1008b910;                        // far

// 0x1008be44 -- bumped once by every clip plane (six `incl` here).
extern uint32		g_1008be44;
// 0x1008be48 -- bumped once per dynamic light considered (big64cc0.cpp).
extern uint32		g_1008be48;
// 0x1008be5c -- the frame counter in the tail (big124a4.cpp).
extern uint32		g_1008be5c;
// 0x1008be60 -- the alpha byte every built vertex gets (big66f08.cpp,
// d3dhigh.cpp).  BSS.
extern uint8		g_b1008be60;

// 0x1008d1bc / 0x1008d2bc / 0x1008d3bc -- d3dhigh.cpp's three 256-entry
// byte ramps, indexed by the source entry's +0x16 / +0x15 / +0x14.
extern uint8		g_ColorRamp2[256];                  // 0x1008d1bc
extern uint8		g_ColorRamp1[256];                  // 0x1008d2bc
extern uint8		g_ColorRamp0[256];                  // 0x1008d3bc

// 0x1008ad94..0x1008ada0 -- big70d10.cpp's generated-coordinate scale and
// offset pair.  Here they feed the FIRST texture pair, not the second.
extern float		g_f1008ad94;                        // scale u
extern float		g_f1008ad98;                        // scale v
extern float		g_f1008ad9c;                        // offset u
extern float		g_f1008ada0;                        // offset v

// 0x1008caf4 / 0x1008caf8 / 0x1008cafc -- the three components
// 0x10023930 folds into the fog colour.
extern uint32		g_1008caf4;
extern uint32		g_1008caf8;
extern uint32		g_1008cafc;

// 0x10091200 -- big0ae20.cpp's LMDynamic console-variable record; +0x00
// is the int32 value.  Read as a bare dword here (0x1000ba99).
extern uint32		g_nLMDynamic;                       // 0x10091200

// 0x1008cb98 -- big21f90.cpp's g_CV_Saturate, the int32 backing store of
// the "Saturate" console variable.
extern uint32		g_CV_Saturate;                      // 0x1008cb98

// The shared draw-vertex buffer: 0x1008d51c the capacity, 0x1008d520 the
// used count, 0x1008d524 the base.  d3dr2c.cpp's 0x1000f270 / 0x10023fb0.
extern uint32		g_1008d51c;
extern uint32		g_1008d520;
extern uint8*		g_1008d524;

// The two pools and the deferred-pass bucket list -- big124a4.cpp.
extern Poolb910		g_1008d4c8;                         // bucket pool
extern Bucketb910*	g_1008d4e0;                         // = 0x1008d4c8+0x18
extern Poolb910		g_1008d4e4;                         // element pool
extern Elemb910*	g_1008d4fc;                         // = 0x1008d4e4+0x18
extern Bucketb910*	g_1008d4c4;                         // the list head

// 0x1008ce8c -- big17c14.cpp pinned this one: both arguments in
// registers, %ecx then %edx, and it writes the specular dword.
typedef void (__fastcall* PFN1008ce8c)(LTVec* pPos, uint32* pOut);
extern PFN1008ce8c	g_pfn1008ce8c;                      // 0x1008ce8c


// ======================================================================= //
//  THE CALLEES -- 12 distinct direct targets.  Ten are declared here;
//  the other two are _chkstk (0x100805a0) and the CRT sqrt (0x1007fc98),
//  neither of which this source can emit.  All twelve are placed by
//  earlier work.
// ======================================================================= //

// 0x1000a700 -- d3dbatchd.cpp.  (geometry, &data, &count).
extern "C" void		sub_1000a700(void* pGeom, SrcVert** ppOutData,
                                 int32* pnOutCount);

// 0x1000a740 -- d3dbatchd.cpp.  `cmpl $0x80,arg ; jg`, SIGNED.
extern "C" LTBOOL	sub_1000a740(int32 nVerts);

// 0x10023930 -- d3dbatchc.cpp, 784 bytes.  Three components in, one
// render-state value out.
extern "C" uint32	sub_10023930(uint32 a, uint32 b, uint32 c);

// 0x1000d9b0 -- d3dlow.cpp, __cdecl (vertex, transform object): the 3x4
// at +0x15c.
extern "C" void		sub_1000d9b0(RVert* pVert, uint8* pXForm);

// 0x1000d940 -- d3dlow.cpp, __cdecl (vertex, transform object): the
// screen projection through +0x31c..+0x330.
extern "C" void		sub_1000d940(RVert* pVert, uint8* pXForm);

// 0x10023fb0 -- d3dr2c.cpp.  The vertex-buffer overflow report.
extern "C" LTBOOL	sub_10023fb0(uint32 nWanted, uint8* pXForm);

// 0x1000ae20 -- big0ae20.cpp, the LMDynamic additive second pass.  Tier
// Z: arg0 in %eax, arg1 in %edx, arg2 in %ecx, with sixteen DEAD bytes of
// outgoing area.  Written __cdecl.
// (defined above, static: the register-only body)

// 0x1007fbf0 -- d3dbatchc.cpp.  (pool, pool->m_nGrow08).
extern "C" void		sub_1007fbf0(Poolb910* pPool, uint32 nCount);

// The CRT double sqrt at 0x1007fc98.  icl expands it to `fsqrt`; a
// known toolchain gap.
extern "C" double	sqrt(double);



// ==== w2_1000b6c0_os.cpp ====
// ---- the deferred-draw queue.


// big24020.cpp's element, verbatim (bigb910.cpp's Elemb910 is the same
// 0x18 bytes; the free list below is spelled with that one).
struct Elem24024                                        // 0x18 bytes
{
	void*       m_pOwner;                               // +0x00
	uint32      m_n04;                                  // +0x04
	uint32      m_n08;                                  // +0x08
	uint32      m_n0c;                                  // +0x0c
	uint32      m_nClipFlags10;                         // +0x10
	Elem24024*  m_pNext14;                              // +0x14
};

// 0x1008ad90 -- the queue head: tested on entry, walked, then cleared.
extern Elem24024*      g_1008ad90;                     // 0x1008ad90

void sub_1000da50();                                    // d3dr2c.cpp
static void sub_1000b910(Objb910* pObj);                // below: register-only


void sub_1000b6c0()
{
	Elem24024*		pElem;
	Elem24024*		pNext;

	if (!g_1008ad90)
		return;

	BindBlock0ae20(g_p1008beb8->m_pBlock00, g_n1008d7d0, 0);

	sub_1000da50();

	for (pElem = g_1008ad90; pElem; pElem = pNext)
	{
		pNext = pElem->m_pNext14;
		g_1008b874 = pElem->m_nClipFlags10;
		sub_1000b910((Objb910*)pElem->m_pOwner);

		pElem->m_pOwner = g_1008d4fc;
		g_1008d4fc = (Elemb910*)pElem;
	}

	g_1008ad90 = LTNULL;
}


// ==== bigb910.cpp: 0x1000b910 ====
// ======================================================================= //
//  THE 40-BYTE CLIPPER: big051a0.cpp's -- the six plane
//  bodies 0x10002310 .. 0x10003410 and the driver 0x100051a0 is built
//  around -- INLINED here as the shipped 0x1000b910 has them: pVerts /
//  nVerts / pOut stay in frame slots because their addresses go to the
//  plane bodies, and every reject reaches the shared state restore.
// ======================================================================= //
#define CLIP_EPSILON    0.00001f
#define CLIP_MAX_VERTS  50

static inline float ClipEdgeNearA(LTVec* a, LTVec* b, LTVec* dst)
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


static inline float ClipEdgeLeftA(LTVec* a, LTVec* b, LTVec* dst)
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


static inline float ClipEdgeTopA(LTVec* a, LTVec* b, LTVec* dst)
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


static inline float ClipEdgeFarA(LTVec* a, LTVec* b, LTVec* dst)
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


static inline float ClipEdgeRightA(LTVec* a, LTVec* b, LTVec* dst)
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


static inline float ClipEdgeBottomA(LTVec* a, LTVec* b, LTVec* dst)
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


static inline void LerpRVertA(RVert* a, RVert* b, RVert* dst, float t)
{
	dst->m_fU18 = (b->m_fU18 - a->m_fU18) * t + a->m_fU18;
	dst->m_fV1c = (b->m_fV1c - a->m_fV1c) * t + a->m_fV1c;
	dst->m_fU20 = (b->m_fU20 - a->m_fU20) * t + a->m_fU20;
	dst->m_fV24 = (b->m_fV24 - a->m_fV24) * t + a->m_fV24;
	dst->m_nRed12 = (uint8)RoundF((b->m_nRed12 - a->m_nRed12) * t + a->m_nRed12);
	dst->m_nGreen11 = (uint8)RoundF((b->m_nGreen11 - a->m_nGreen11) * t + a->m_nGreen11);
	dst->m_nBlue10 = (uint8)RoundF((b->m_nBlue10 - a->m_nBlue10) * t + a->m_nBlue10);
	dst->m_nAlpha13 = (uint8)RoundF((b->m_nAlpha13 - a->m_nAlpha13) * t + a->m_nAlpha13);
	dst->m_nSpec17 = (uint8)RoundF((b->m_nSpec17 - a->m_nSpec17) * t + a->m_nSpec17);
}


static inline LTBOOL sub_10002310(uint32* pUnused, RVert** ppVerts, int32* pnVerts, RVert** ppOut)
{
	uint32     anInside[CLIP_MAX_VERTS];
	RVert*  pOutStart;
	RVert*  pV;
	RVert*  pEnd;
	RVert*  pPrev;
	RVert*  pCur;
	uint32*    pFlag;
	int32      nInside;
	int32      i;
	int32      iPrev;

	++g_1008be44;

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
				float t = ClipEdgeNearA(&pPrev->m_vPos, &pCur->m_vPos, &(*ppOut)->m_vPos);

				LerpRVertA(pPrev, pCur, *ppOut, t);

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


static inline LTBOOL sub_10002670(uint32* pUnused, RVert** ppVerts, int32* pnVerts, RVert** ppOut)
{
	uint32     anInside[CLIP_MAX_VERTS];
	RVert*  pOutStart;
	RVert*  pV;
	RVert*  pEnd;
	RVert*  pPrev;
	RVert*  pCur;
	uint32*    pFlag;
	int32      nInside;
	int32      i;
	int32      iPrev;

	++g_1008be44;

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
				float t = ClipEdgeLeftA(&pPrev->m_vPos, &pCur->m_vPos, &(*ppOut)->m_vPos);

				LerpRVertA(pPrev, pCur, *ppOut, t);

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


static inline LTBOOL sub_100029e0(uint32* pUnused, RVert** ppVerts, int32* pnVerts, RVert** ppOut)
{
	uint32     anInside[CLIP_MAX_VERTS];
	RVert*  pOutStart;
	RVert*  pV;
	RVert*  pEnd;
	RVert*  pPrev;
	RVert*  pCur;
	uint32*    pFlag;
	int32      nInside;
	int32      i;
	int32      iPrev;

	++g_1008be44;

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
				float t = ClipEdgeTopA(&pPrev->m_vPos, &pCur->m_vPos, &(*ppOut)->m_vPos);

				LerpRVertA(pPrev, pCur, *ppOut, t);

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


static inline LTBOOL sub_10002d40(uint32* pUnused, RVert** ppVerts, int32* pnVerts, RVert** ppOut)
{
	uint32     anInside[CLIP_MAX_VERTS];
	RVert*  pOutStart;
	RVert*  pV;
	RVert*  pEnd;
	RVert*  pPrev;
	RVert*  pCur;
	uint32*    pFlag;
	int32      nInside;
	int32      i;
	int32      iPrev;

	++g_1008be44;

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
				float t = ClipEdgeRightA(&pPrev->m_vPos, &pCur->m_vPos, &(*ppOut)->m_vPos);

				LerpRVertA(pPrev, pCur, *ppOut, t);

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


static inline LTBOOL sub_100030a0(uint32* pUnused, RVert** ppVerts, int32* pnVerts, RVert** ppOut)
{
	uint32     anInside[CLIP_MAX_VERTS];
	RVert*  pOutStart;
	RVert*  pV;
	RVert*  pEnd;
	RVert*  pPrev;
	RVert*  pCur;
	uint32*    pFlag;
	int32      nInside;
	int32      i;
	int32      iPrev;

	++g_1008be44;

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
				float t = ClipEdgeBottomA(&pPrev->m_vPos, &pCur->m_vPos, &(*ppOut)->m_vPos);

				LerpRVertA(pPrev, pCur, *ppOut, t);

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


static inline LTBOOL sub_10003410(uint32* pUnused, RVert** ppVerts, int32* pnVerts, RVert** ppOut)
{
	uint32     anInside[CLIP_MAX_VERTS];
	RVert*  pOutStart;
	RVert*  pV;
	RVert*  pEnd;
	RVert*  pPrev;
	RVert*  pCur;
	uint32*    pFlag;
	int32      nInside;
	int32      i;
	int32      iPrev;

	++g_1008be44;

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
				float t = ClipEdgeFarA(&pPrev->m_vPos, &pCur->m_vPos, &(*ppOut)->m_vPos);

				LerpRVertA(pPrev, pCur, *ppOut, t);

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


// ======================================================================= //
//  THE CLIPPER, as the shipped code is built: 0x100051a0 is
//  the 40-byte-vertex twin of d3dr3c_os.cpp's 0x1000f4b0 -- the same
//  UseD3DClip test, the same output bank, the same six planes in the same
//  order -- with the six plane bodies 0x10002310 .. 0x10003410 (byte-
//  identical out of line in w1_10002310.cpp / w44_10002d40.cpp) INLINED.
//  The plane bodies take &pVerts / &nVerts / &pOut, so those three keep
//  frame slots and are stored and reloaded around every plane -- which is
//  the shipped `mov -0xd0(%ebp),%eax ; ... ; mov %eax,-0xd0(%ebp)` round
//  trip of pOut in every vertex copy, and the RoundF temporaries give the
//  thirty `fstps t ; flds t ; fistpl n` colour conversions.  The earlier
//  macro-expanded passes (the plane bodies written out in place) are in
//  git history; they kept pOut in a register.
//
//  The driver is itself an INLINE body under the exported function: the
//  shipped code has ONE epilogue (`mov %edx,%eax` at 0x100066f4) that
//  every exit reaches with the answer in %edx -- `mov $0x1,%edx ; jmp`
//  for the UseD3DClip early-out, `xor %edx,%edx ; jmp` for each plane's
//  reject -- which is how icl lowers an inlined callee's several
//  `return`s.  Written with the driver's returns in the exported function
//  itself, /O2 gives seven epilogues (0.966 matched); with a result
//  variable and gotos, the variable goes to the frame (0.980); as an
//  inline driver: 1509 of 1523 instructions matched, same count (0.991).
// ======================================================================= //
static inline LTBOOL ClipPolyb910(uint32 nClipFlags, RVert** ppVerts,
                                     int32* pnVerts)
{
	RVert*  pOut;
	RVert*  pVerts;
	int32       nVerts;
	uint32      anUnused[6];

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
		if (!sub_10002310(&anUnused[0], &pVerts, &nVerts, &pOut))
			return LTFALSE;
	}

	if (nClipFlags & 0x04)
	{
		if (!sub_10002670(&anUnused[1], &pVerts, &nVerts, &pOut))
			return LTFALSE;
	}

	if (nClipFlags & 0x08)
	{
		if (!sub_100029e0(&anUnused[2], &pVerts, &nVerts, &pOut))
			return LTFALSE;
	}

	if (nClipFlags & 0x10)
	{
		if (!sub_10002d40(&anUnused[3], &pVerts, &nVerts, &pOut))
			return LTFALSE;
	}

	if (nClipFlags & 0x20)
	{
		if (!sub_100030a0(&anUnused[4], &pVerts, &nVerts, &pOut))
			return LTFALSE;
	}

	if (!(nClipFlags & 0x02) ||
	    sub_10003410(&anUnused[5], &pVerts, &nVerts, &pOut))
	{
		*ppVerts = pVerts;
		*pnVerts = nVerts;
		return LTTRUE;
	}

	return LTFALSE;
}


// struct_bank.h's sb_Allocate (its out-of-line copy is 0x1000a6a0).
static inline void* sb_Allocateb910(Poolb910* pBank)
{
	void**	pRet;

	pRet = (void**)pBank->m_pFree18;
	if (!pRet)
	{
		sub_1007fbf0(pBank, pBank->m_nGrow08);
		pRet = (void**)pBank->m_pFree18;
		if (!pRet)
			return 0;
	}

	pBank->m_pFree18 = *pRet;
	return pRet;
}

// 0x1000eef0 (d3dr3c_os.cpp, identical there out of line), inlined here.
static inline Elemb910* sub_1000eef0(Objb910* pOwner, Bucketb910** ppList,
                                     uint32 nIndex, uint32 nUnused)
{
	Tableb910*   pTable = pOwner->m_pCtx28->m_pTable30;
	Bucketb910*  pBucket = pTable->m_pBuckets1c[nIndex];
	Elemb910*    pElem;

	if (!pBucket)
	{
		pBucket = (Bucketb910*)sb_Allocateb910(&g_1008d4c8);

		pBucket->m_pTable00 = pTable;
		pTable->m_pBuckets1c[nIndex] = pBucket;
		pBucket->m_pElements04 = LTNULL;
		pBucket->m_pNextInList08 = *ppList;
		*ppList = pBucket;
	}

	pElem = (Elemb910*)sb_Allocateb910(&g_1008d4e4);

	pElem->m_pNext14 = pBucket->m_pElements04;
	pBucket->m_pElements04 = pElem;
	pElem->m_pOwner00 = pOwner;

	return pElem;
}


// ----------------------------------------------------------------------- //
//  0x1000b910.  ONE argument, arriving in %eax (tier Z); written __cdecl.
//  The frame slot each local occupies is in the comment.
// ----------------------------------------------------------------------- //
//  Still attempted on the FRAME, 0x1b68 against the shipped
//  0x1b78.  Every slot from -0x80 down sits exactly 0x10 higher than
//  shipped (the one above, the -0x10 spill, is at -0x8 here); the shipped
//  frame has an UNREFERENCED 0x6c-byte run at -0x7c..-0x14 where ours has
//  0x64.  The driver's anUnused[6] is not in it (as an array, as six
//  scalars, or as two: the frame does not move), so the extra eight bytes
//  are dead temporaries of something inlined that is not reproduced yet.
//  The bucket/element allocation at 0x1000d33f is now the
//  real 0x1000eef0 (d3dr3c_os.cpp, identical out of line) over struct_bank.h's
//  sb_Allocate (0x1000a6a0 out of line), both inlined: the refills' shared
//  `return 0` and the -0x10 spill come out as shipped, normalised
//  instruction diff 311 -> 296 (the header's own sb_Allocate is the same
//  bytes but moves 0x1000ae20 by 3).  Frame still 0x1b68.  Measured
//  without effect on the frame (all worse or equal): the clipper's
//  `pV = &(*ppVerts)[0]` (311 -> 335), plane 1's pOutStart
//  read first (304), the LMDynamic gate as `== 0` or as an empty `if` arm
//  (438: neither sinks the light pass to 0x1000d444), the no-clip block
//  as ltmatrix.h's MatVMul_InPlace_H (421) or as 0x1000a8a0's whole body
//  inlined over MatVMul_H (w30's XFormClip124a4 shape, 450), 0x1000f270
//  as an inline reserve (340), and the C1000adc0 restore as an inline
//  destructor (same bytes as the two written-out restores).
static void sub_1000b910(Objb910* pObj)                        // %eax -> -0xc0
{
	SrcVert*	pData;                                  // -0x1a74
	int32		nVerts;                                 // -0x1a70
	C1000adc0	stateSave;                              // -0x1a6c
	RVert		aVerts[128];                            // -0x1a64
	RVert*		pVerts;                                 // -0x664
	LTVec		vLightPos;                              // -0x660
	LTVec		vScreen;                                // -0x654

	RVert*		pWalk;                                  // -0x10c
	RVert*		pOut;                                   // -0xd0
	RVert*		pOutStart;                              // -0x80
	RVert*		pIn;                                    // -0x88
	int32		nIn;                                    // -0xb8
	int32		nLeft;
	uint32		nFlags;
	uint32		nClip;                                  // -0xe0

	DVert*		pDraw;                                  // -0x90
	DVert*		pDrawWalk;
	Elemb910*	pElem;

	// ------------------------------------------------------------------ //
	//  0x1000b92f .. 0x1000b957.  Three pushes, `add $0xc,%esp`, then one
	//  push and `add $0x8,%esp` -- the extra 4 bytes are the reserve
	//  `push %edi` opened at 0x1000b928 and consumed here.
	// ------------------------------------------------------------------ //
	sub_1000a700(pObj, &pData, &nVerts);

	if (!sub_1000a740(nVerts))
		return;

	// ------------------------------------------------------------------ //
	//  0x1000b960 .. 0x1000b99b.  State 0x22 = 34 = FOGCOLOR, and the
	//  value is the three adjacent globals folded by 0x10023930.  The
	//  argument is evaluated first, which is why 0x10023930 precedes
	//  0x1000adc0 in the shipped call sequence.
	// ------------------------------------------------------------------ //
	stateSave.sub_1000adc0(D3DRENDERSTATE_FOGCOLOR,
	                       sub_10023930(g_1008caf4, g_1008caf8, g_1008cafc));

	// ------------------------------------------------------------------ //
	//  0x1000b99c .. 0x1000ba91 -- BUILD THE VERTICES.
	//
	//  `test %edi,%edi ; jle` at 0x1000b9a8 and `dec ; test ; jg` at
	//  0x1000ba85, so the guard and the test are SIGNED.  pData lives in
	//  the frame and is re-read on every use (seven `mov -0x1a74(%ebp)`
	//  inside the loop) and written back at 0x1000ba87, because its
	//  address was taken above.
	//
	//  The __fastcall hook takes &pWalk->m_vPos in %ecx and
	//  &pWalk->m_nSpec14 in %edx; the shipped code keeps the second one
	//  as its own strength-reduced walker at -0x10c, seeded
	//  `lea 0x14(%ebx),%eax` at 0x1000b9b0 and advanced `add $0x28`.
	// ------------------------------------------------------------------ //
	pWalk = aVerts;
	nLeft = nVerts;

	while (nLeft > 0)
	{
		// 0x1000b9c7 .. 0x1000b9d6 -- three integer `mov` through the
		// source entry's +0x00 pointer.
		pWalk->m_vPos.x = pData->m_pPos00->x;
		pWalk->m_vPos.y = pData->m_pPos00->y;
		pWalk->m_vPos.z = pData->m_pPos00->z;

		// 0x1000b9d9 .. 0x1000ba17 -- d3dhigh.cpp's 0x100598a0 inlined,
		// highest destination byte first.
		pWalk->m_nRed12   = g_ColorRamp2[pData->m_nRed16];
		pWalk->m_nGreen11 = g_ColorRamp1[pData->m_nGreen15];
		pWalk->m_nBlue10  = g_ColorRamp0[pData->m_nBlue14];
		pWalk->m_nAlpha13 = g_b1008be60;

		// 0x1000ba1a -- `mov 0x1008ce8c,%eax ; call *%eax`.
		(*g_pfn1008ce8c)(&pWalk->m_vPos, (uint32*)&pWalk->m_nSpec14);

		// 0x1000ba21 .. 0x1000ba4c -- the two arguments are built into
		// the 8-byte outgoing area opened at 0x1000ba2f, u first.  The u
		// term loads the position first and the v term the constant
		// first; that is the scheduler, see COIN-FLIP (3).
		pWalk->sub_1000d9d0(
			(pData->m_pPos00->x + g_f1008ad9c) * g_f1008ad94,
			(g_f1008ada0 + pData->m_pPos00->z) * g_f1008ad98);

		// 0x1000ba63 / 0x1000ba76 -- integer moves, float lvalues.
		pWalk->m_fU20 = pData->m_fU04;
		pWalk->m_fV24 = pData->m_fV08;

		// 0x1000ba7f / 0x1000ba82.
		pWalk++;
		pData++;
		nLeft--;
	}

	// 0x1000ba93.
	pVerts = aVerts;

	// ------------------------------------------------------------------ //
	//  0x1000ba99 -- the whole block below is SUNK to 0x1000d444.  It is
	//  placed HERE in the source on the evidence of the literal pool: its
	//  three 255.0f, its 0.0f and its two 1.0f all precede 0x100a451c,
	//  the `1.0f / w` of the transform two statements down.  See pool
	//  finding 3 in the banner.  With LMDynamic ON, 0x1000ae20 does this
	//  properly as a projected-lightmap pass instead, so it is skipped.
	// ------------------------------------------------------------------ //
	if (!g_nLMDynamic)
	{
		LightRec*	pLight;
		LightObj*	pLightObj;
		SPlane*		pPlane;
		float		fDist, fRed, fGreen, fBlue;
		float		fRadius, fInvRadius, fRadiusSqr, fDistSqr, fAtten;
		int32		nR, nG, nB;
		uint32		nLightVerts;

		// 0x1000d456 / 0x1000d8de -- `mov (%eax),%eax` on -0xbc.
		for (pLight = pObj->m_pLights2c; pLight != 0;
		     pLight = pLight->m_pNext00)
		{
			// 0x1000d473 -- the object, then `rep movsl $3` from
			// pLight+0x08 into the twelve bytes at -0x660.
			pLightObj = pLight->m_pObject04;
			vLightPos = pLight->m_vPos08;

			// 0x1000d495 .. 0x1000d4a8.
			g_1008be48++;

			// 0x1000d4ae .. 0x1000d510.  The plane is pObj->+0x28->+0x34,
			// the same chain big64cc0.cpp reads; the terms are emitted
			// z, y, x and the `- m_fDist` is `fsubs 0xc(%eax)`.
			pPlane = pObj->m_pCtx28->m_pPlane34;

			fDist = pPlane->m_vNormal.x * vLightPos.x
			      + pPlane->m_vNormal.y * vLightPos.y
			      + pPlane->m_vNormal.z * vLightPos.z
			      - pPlane->m_fDist;

			// 0x1000d51f .. 0x1000d54f.  READ EXACTLY AS WRITTEN:
			// `flds 255.0f ; fsub %st(i),%st ; DE E8+i`.  DE E8+i is
			// Intel's FSUBP ST(i),ST(0) -- ST(i) = ST(i) - ST(0) -- which
			// objdump misprints as `fsubrp`; the swap was confirmed on
			// THIS function's own bytes at 0x1000bd53, where
			// `fdivp %st,%st(1)` (DE F1) has to be Intel's FDIVRP for
			// d3dr3a.cpp's known `fT = (plane - pPrev->z) / fDenom` to
			// come out.  So the value stored is c - (255.0f - c), and
			// big64cc0.cpp reached the same reading independently on
			// 0x100650aa.  The three 255.0f are three separate pool
			// entries in the order red, green, blue.
			fRed   = pLightObj->m_nRed94   - (255.0f - pLightObj->m_nRed94);
			fGreen = pLightObj->m_nGreen95 - (255.0f - pLightObj->m_nGreen95);
			fBlue  = pLightObj->m_nBlue96  - (255.0f - pLightObj->m_nBlue96);

			// 0x1000d4e2 / 0x1000d51b / 0x1000d556 -- the constant is
			// 0.0f (0x100a4508, dumped) and the `fchs` is in the
			// fall-through arm, so this is fabs.
			if (fDist < 0.0f)
				fDist = -fDist;

			// 0x1000d55a -- `fcomps 0x1a8(%ebx) ; jae`, and the member is
			// read a SECOND time at 0x1000d562.
			if (fDist >= pLightObj->m_fRadius1a8)
				continue;

			fRadius = pLightObj->m_fRadius1a8;

			// 0x1000d575 / 0x1000d593.  0x100a4510 is 1.0f.
			fInvRadius = 1.0f / fRadius;
			fRadiusSqr = fRadius * fRadius;

			// 0x1000d595 -- the count is pObj->+0x50 as a uint16, NOT
			// nVerts.  Read off `movzwl 0x50(%eax),%esi` and the
			// UNSIGNED `ja` at 0x1000d5a7.
			nLightVerts = pObj->m_nCount50;

			if (nLightVerts == 0)
				continue;

			pWalk = pVerts;

			do
			{
				// 0x1000d5d4 .. 0x1000d5fc, z first, all three
				// subtractions widened to double (`fsubl`).
				fDistSqr =
				    (pWalk->m_vPos.x - vLightPos.x)
				        * (pWalk->m_vPos.x - vLightPos.x)
				  + (pWalk->m_vPos.y - vLightPos.y)
				        * (pWalk->m_vPos.y - vLightPos.y)
				  + (pWalk->m_vPos.z - vLightPos.z)
				        * (pWalk->m_vPos.z - vLightPos.z);

				// 0x1000d5fe -- `fldl radiusSqr ; fcomp ; jbe`.
				if (fRadiusSqr > fDistSqr)
				{
					// 0x1000d615 .. 0x1000d632.  The shipped build CALLS
					// the CRT double sqrt at 0x1007fc98; icl expands it
					// to `fsqrt`, which is a known toolchain gap -- and
					// big64cc0.cpp's TU already ships it inline.
					// 0x100a4514 is 1.0f.
					// (float): the shipped `fsubrs 0x100a4514` is a FLOAT
					// 1.0f, kept only when the subtraction is float.
					fAtten = 1.0f - (float)(sqrt(fDistSqr) * fInvRadius);

					// 0x1000d63a .. 0x1000d70b.  The three `(int32)` are
					// icl's fnstcw/fldcw/fistpll truncation, three of
					// each in the census.
					nR = pWalk->m_nRed12   + (int32)(fRed   * fAtten);
					nG = pWalk->m_nGreen11 + (int32)(fGreen * fAtten);
					nB = pWalk->m_nBlue10  + (int32)(fBlue  * fAtten);

					// 0x1000d711 .. 0x1000d719 -- `add X,X`.
					if (g_CV_Saturate)
					{
						nR = nR + nR;
						nG = nG + nG;
						nB = nB + nB;
					}

					// 0x1000d71b .. 0x1000d748, with the three `< 0` arms
					// sunk to 0x1000d914 / 0x1000d905 / 0x1000d923.
					if (nR > 255)     nR = 255;
					else if (nR < 0)  nR = 0;

					if (nG > 255)     nG = 255;
					else if (nG < 0)  nG = 0;

					if (nB > 255)     nB = 255;
					else if (nB < 0)  nB = 0;

					// 0x1000d74d .. 0x1000d753.
					pWalk->m_nRed12   = (uint8)nR;
					pWalk->m_nGreen11 = (uint8)nG;
					pWalk->m_nBlue10  = (uint8)nB;
				}

				pWalk++;
			}
			while (--nLightVerts);
		}
	}

	// ------------------------------------------------------------------ //
	//  0x1000bab3 / 0x1000bab9.  Both predecessors of 0x1000bab9 set
	//  -0x10c from -0x664 first, so this assignment is above the flag
	//  read in both arms.
	// ------------------------------------------------------------------ //
	pWalk  = pVerts;
	nFlags = g_1008b874;

	if (!nFlags)
	{
		// -------------------------------------------------------------- //
		//  0x1000bac6 .. 0x1000bbc5 -- NO CLIPPING WANTED.  One
		//  projective transform per vertex through 0x1008b880's
		//  +0x1dc..+0x218 block, `fdivrs 1.0f` (0x100a451c, dumped) for
		//  the reciprocal, the three results assembled in the named
		//  three-float local at -0x654 -- the `rep movsl $3` at
		//  0x1000bba4 copies it into the vertex -- and the reciprocal,
		//  held as a double at -0x80, into +0x0c.  The identical
		//  instruction sequence, operand for operand, is 0x1000a8d0
		//  inside 0x1000a8a0, 0x10012a65 in big124a4.cpp and 0x10070e19
		//  in big70d10.cpp.
		// -------------------------------------------------------------- //
		XFormb910* pM = (XFormb910*)g_1008b880;
		float      fOOW;

		nLeft = nVerts;

		while (nLeft)
		{
			fOOW = 1.0f / (pWalk->m_vPos.x * pM->m_f20c
			             + pWalk->m_vPos.y * pM->m_f210
			             + pWalk->m_vPos.z * pM->m_f214
			             + pM->m_f218);

			vScreen.x = (pWalk->m_vPos.x * pM->m_f1dc
			           + pWalk->m_vPos.y * pM->m_f1e0
			           + pWalk->m_vPos.z * pM->m_f1e4
			           + pM->m_f1e8) * fOOW;

			vScreen.y = (pWalk->m_vPos.x * pM->m_f1ec
			           + pWalk->m_vPos.y * pM->m_f1f0
			           + pWalk->m_vPos.z * pM->m_f1f4
			           + pM->m_f1f8) * fOOW;

			vScreen.z = (pWalk->m_vPos.x * pM->m_f1fc
			           + pWalk->m_vPos.y * pM->m_f200
			           + pWalk->m_vPos.z * pM->m_f204
			           + pM->m_f208) * fOOW;

			pWalk->m_vPos   = vScreen;
			pWalk->m_fRHW0c = fOOW;
			pWalk++;
			nLeft--;
		}

		// 0x1000bbc2 -- straight to the reserve; the screen projection
		// has already happened.
		goto L_Reserve;
	}

	// ------------------------------------------------------------------ //
	//  0x1000bbc7 .. 0x1000bbec -- to camera space, then 0x1000f4b0
	//  INLINED.  g_1008b874 is RE-READ at 0x1000bbee; big70d10.cpp and
	//  0x1000a8a0 both record the second read as source and not a missed
	//  CSE, and the nVerts==0 skip reaching the join with the FIRST load
	//  still in %eax is icl CSEing over the call-free path.
	// ------------------------------------------------------------------ //
	nLeft = nVerts;

	while (nLeft)
	{
		sub_1000d9b0(pWalk, g_1008b880);
		pWalk++;
		nLeft--;
	}

	if (!ClipPolyb910(g_1008b874, &pVerts, &nVerts))
		goto L_Done;

	// ------------------------------------------------------------------ //
	//  0x1000d221 .. 0x1000d246 -- the screen projection, one call per
	//  vertex.  d3dlow.cpp has 0x1000d940 as the +0x31c..+0x330 block.
	// ------------------------------------------------------------------ //
	pWalk = pVerts;
	nLeft = nVerts;

	while (nLeft)
	{
		sub_1000d940(pWalk, g_1008b880);
		pWalk++;
		nLeft--;
	}

L_Reserve:
	// ------------------------------------------------------------------ //
	//  0x1000d248 .. 0x1000d297 -- d3dr2c.cpp's 0x1000f270 INLINED:
	//  "bounds-checks g_1008d520 + n against g_1008d51c, returns
	//  g_1008d524 + (g_1008d520 << 5)".  The compare is UNSIGNED (`jbe`
	//  at 0x1000d262) and the overflow argument is
	//  `g_1008d51c + 0x5dc + n` (`lea 0x5dc(%ecx),%ecx ; add %ebx,%ecx`),
	//  which is exactly what d3dr2c.cpp and d3dr2c_os.cpp record.
	//  g_1008b880 is still in %edi from the loop above, which is why it
	//  is the second argument here.
	// ------------------------------------------------------------------ //
	if (g_1008d520 + (uint32)nVerts > g_1008d51c)
	{
		if (!sub_10023fb0(g_1008d51c + 0x5dc + nVerts, g_1008b880))
		{
			// 0x1000d3f0 -- the C1000adc0 restore, written out; there is
			// no destructor call in the shipped bytes.
			g_pD3DDevice->lpVtbl->SetRenderState(g_pD3DDevice,
				stateSave.m_nState00, stateSave.m_nOld04);
			return;
		}
	}

	// 0x1000d28a -- `shl $0x5` is the 32-byte DRAW vertex, not the
	// 40-byte clip vertex.
	pDraw = (DVert*)(g_1008d524 + (g_1008d520 << 5));

	// ------------------------------------------------------------------ //
	//  0x1000d299 .. 0x1000d2d6 -- `rep movsl $8` per vertex, 0x28 in and
	//  0x20 out.  The second texture pair does not travel: FVF 0x1c4 is
	//  TEX1.
	// ------------------------------------------------------------------ //
	pWalk     = pVerts;
	pDrawWalk = pDraw;
	nLeft     = nVerts;

	while (nLeft)
	{
		*pDrawWalk = *(DVert*)pWalk;
		pWalk++;
		pDrawWalk++;
		nLeft--;
	}

	// 0x1000d2d8 -- vtable +0x64, type 6, FVF 0x1c4, flags 0.
	g_pD3DDevice->lpVtbl->DrawPrimitive(g_pD3DDevice, D3DPT_TRIANGLEFAN,
		D3DFVF_DRAWVERTEX, pDraw, nVerts, 0);

	// ------------------------------------------------------------------ //
	//  0x1000d315 -- the dynamic-lightmap second pass.  big0ae20.cpp reads
	//  this very call site from the other side and pins 0x1000ae20's
	//  three register arguments from it: %eax = pObj, %edx = the draw
	//  vertices, %ecx = the count, with sixteen dead bytes of outgoing
	//  area.  The guard is the SAME +0x2c the light loop walks.
	// ------------------------------------------------------------------ //
	if (pObj->m_pLights2c)
		sub_1000ae20((Surf0ae20*)pObj, (Vert0ae20*)pDraw, nVerts);

	// ------------------------------------------------------------------ //
	//  0x1000d33f .. 0x1000d3b4 -- big124a4.cpp's 0x1000eef0 INLINED with
	//  0x1000a6a0 inlined inside it, which is where both calls to
	//  0x1007fbf0 come from.  The bucket index is the literal 0, so the
	//  slot is a bare `0x1c(%edx)`.  The pool addresses agree:
	//  0x1008d4c8 + 0x18 = 0x1008d4e0, 0x1008d4c8 + 0x08 = 0x1008d4d0,
	//  0x1008d4e4 + 0x18 = 0x1008d4fc, 0x1008d4e4 + 0x08 = 0x1008d4ec.
	//  Both refills are SUNK, to 0x1000d8ae and 0x1000d851, and the whole
	//  bucket-allocation arm to 0x1000d87a.
	// ------------------------------------------------------------------ //
	pElem = sub_1000eef0(pObj, &g_1008d4c4, 0, 0);

	pElem->m_n04 = g_1008d520;
	pElem->m_n08 = nVerts;
	pElem->m_n0c = (pVerts != aVerts);            // 0x1000d3a6 `setne %cl`
	pElem->m_n10 = g_1008b874;

	// ------------------------------------------------------------------ //
	//  0x1000d3b7 .. 0x1000d3da -- the SECOND texture pair is written into
	//  the DRAW vertices' first pair, ready for the deferred pass.  Stride
	//  0x28 on the clip side and 0x20 on the draw side, which is what
	//  makes the two arrays different types.
	// ------------------------------------------------------------------ //
	pWalk     = pVerts;
	pDrawWalk = pDraw;
	nLeft     = nVerts;

	while (nLeft)
	{
		pDrawWalk->m_fU18 = pWalk->m_fU20;
		pDrawWalk->m_fV1c = pWalk->m_fV24;
		pWalk++;
		pDrawWalk++;
		nLeft--;
	}

	// 0x1000d3e2 / 0x1000d3e8, in that order -- the commit that pairs
	// with the reserve, then the frame counter.
	g_1008d520 += nVerts;
	g_1008be5c++;

L_Done:
	// 0x1000d41a -- the second copy of the C1000adc0 restore.  The six
	// `nInside == 0` exits and the fall-through both reach it, and it
	// ends `jmp 0x1000b959`, the function's ONE epilogue.
	g_pD3DDevice->lpVtbl->SetRenderState(g_pD3DDevice,
		stateSave.m_nState00, stateSave.m_nOld04);
}


// *********************************************************************** //
//
//  WHAT THE ACCEPTANCE TEST CANNOT SEE.
//
//  1. THE SIX CLIP PLANES ARE 5.7 KB WITH NO CALLS IN THEM AT ALL --
//     0x1000bbe5..0x1000d239, 69.5% of the function.  The call-sequence test is blind to
//     every instruction in them.  What pins them instead:
//       * they are the SAME six blocks d3dr3a.cpp carries OUT OF LINE for
//         the SAME 40-byte vertex (0x10002d40 is plane 4 here and
//         0x100030a0 is plane 5, both diffed term by term above), and the
//         same six big16324.cpp / big124a4.cpp / big70d10.cpp carry
//         inlined for the 32-byte and 40-byte vertices;
//       * the frame decomposes to exactly six 220-byte blocks and no
//         seventh (banner);
//       * the census gives six of `incl 0x1008be44`, six `idiv`, six
//         `fdivp`, six bInside `lea`, six 40-byte `rep movsl`, 30
//         `fistpl` into 30 distinct slots and 66 `fmull` in six groups of
//         eleven, with NO residue (banner);
//       * the literal pool holds six private -1e-5f and six private
//         +1e-5f/0.0f pairs (banner).
//     Each plane's own test, denominator, numerator and store order was
//     then read off THIS image at the addresses in its comment.
//
//  2. THE UNCLIPPED PROJECTION (0x1000bab9..0x1000bbc5) IS ALSO CALL-FREE,
//     270 bytes.  Its twelve `fmuls` and four `fadds` operands are
//     0x1008ba5c..0x1008ba98 = 0x1008b880 + 0x1dc..+0x218, and the same
//     sequence appears operand for operand at 0x1000a8d0, 0x10012a65 and
//     0x10070e19.
//
//  3. THE DYNAMIC-LIGHT PASS (0x1000d444..0x1000d900) HAS ONE CALL IN
//     1,148 BYTES, and it is the sqrt we cannot emit.  It is pinned by
//     big64cc0.cpp's independent decode of the same pass at
//     0x10065007..0x10065394 -- same list walk, same counter, same
//     +0x08/+0x04 split, same plane chain, same `c - (255.0f - c)`, same
//     radius at +0x1a8, same `1.0f - sqrt(d2) * (1/r)`, same Saturate
//     doubling, same two-sided clamps with the `< 0` arms sunk.
//
//  4. THE ARGUMENT COUNTS of the four register-ABI / indirect calls are
//     read off their outgoing areas and their callees' decodes, not
//     guessed: 0x1000ae20 from big0ae20.cpp (which reads THIS call site),
//     g_pfn1008ce8c from big17c14.cpp, and the two COM slots from the
//     vtable indices.
//
//  ---------------------------------------------------------------------- //
//  COIN-FLIPS -- both spellings given, so one build settles each.
//  ---------------------------------------------------------------------- //
//
//  (1) THE LMDynamic GATE, 0x1000baa7.  Written
//          if (!g_nLMDynamic) { <light pass> }
//      The other spelling is
//          if (g_nLMDynamic) { } else { <light pass> }
//      They differ only in which arm icl sinks.  NEITHER changes the call
//      sequence, because the light pass contains no call we can emit (the
//      sqrt inlines).  Recorded, not resolved.
//
//  (2) THE THREE LIGHT-COLOUR EXPRESSIONS, 0x1000d51f..0x1000d54f.
//      Written  `c - (255.0f - c)`.  The arithmetically equal
//      `2.0f * c - 255.0f` and `c + c - 255.0f` are NOT the same
//      instructions: both need an `fadd`/`fmul` that the shipped bytes do
//      not have, and `c - 255.0f` alone would need only the single
//      `DE E8+i` without the `D8 E0+i` before it.  The shape
//      `push 255 ; D8 E0+i ; DE E8+i` is uniquely `c - (255.0f - c)`.
//      This is therefore NOT a coin-flip; it is recorded because the
//      resulting value (2c - 255) reads oddly for a light colour, and
//      because big64cc0.cpp reached the same reading independently.
//
//  (3) THE TWO GENERATED TEXTURE COORDINATES, 0x1000ba21 / 0x1000ba3f.
//      The u term loads the position and then `fadds` the offset; the v
//      term loads the offset and then `fadds` the position.  Written
//      `(pos->x + off) * scale` and `(off + pos->z) * scale` to match,
//      but VC6/icl reassociate freely and big70d10.cpp writes BOTH as
//      `(pos + off) * scale` from the same instruction shapes.  No call
//      depends on it.
//
//  (4) THE POSITION COPY IN THE BUILD LOOP, 0x1000b9c7.  Written as three
//      component assignments (big70d10.cpp's spelling for the identical
//      three integer `mov`).  big64cc0.cpp writes the same three
//      instructions as one struct assignment `pDst->m_vPos =
//      *pSrc->m_pPos;`.  Both produce three dword moves.
//
//  (5) THE `pWalk = pVerts` BEFORE THE FLAG READ, 0x1000bab3 vs
//      0x1000d8f4.  Both predecessors of the join at 0x1000bab9 perform
//      it, so it is either one source statement the compiler duplicated
//      into the two arms, or the light pass's own `pWalk = pVerts` reused.
//      Written as one statement above the `if`.  No call depends on it.
//
//  (6) THE TWO STATE RESTORES, 0x1000d3f0 and 0x1000d41a, are byte-for-
//      byte identical blocks.  Written as two separate restores (one on
//      the reserve-failure return, one at the end), which is what
//      big124a4.cpp does for the same shape.  A single restore reached by
//      `goto` from both would tail-merge them; the shipped bytes have TWO
//      copies, which is the /O2 duplication.
//
//  ---------------------------------------------------------------------- //
//  NOTHING IS MARKED `// UNDECODED:`.  Every one of the 1,959 instructions
//  in 0x1000b910..0x1000d931 is either a statement above or a row in the
//  banner's census, and the census has no residue.
// *********************************************************************** //
