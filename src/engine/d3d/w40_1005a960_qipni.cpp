// *********************************************************************** //
//
//  MODULE:   W40_1005A960_QIPNI (.CPP)  -- was big5a960.cpp
//
//  BUILD:    /O2 /Qip /Qip_no_inlining (the _qipni convention).
//            The shipped 0x1005a960 calls 0x1005d170, 0x1005d3e0 and
//            0x1005a630 at /Qip REGISTER-ONLY entries, which icl gives only
//            to internal-linkage callees in the caller's TU, so the three
//            are statics defined here (moved in from w7_10040cc0.cpp; all
//            three identical here).
//
//  PURPOSE:  One function -- 0x1005a960, 10256 bytes (0x2810, body ends at
//            0x1005d16f; 0x1005d170 is the next function's entry).  Two
//            callers, 0x10030cce and 0x1005dabc.
//
//  WHAT IT IS: d3d.ren's POLYGRID PASS.  0x10030b85 dispatches on the
//  visible object's type byte at +0xbc -- `cmp $0x2` (worldmodel),
//  `cmp $0x7`, `cmp $0x3` (sprite, which is big61a30.cpp) -- and the
//  `cmp $0x7 ; je 0x10030ca2` arm falls into
//  `push %ebx ; push $0x1008ff60 ; call 0x1005a960`.  7 is OT_POLYGRID in
//  ltbasedefs.h's object-type enum, and 0x1008ff60 is the SAME fixed
//  context global big61a30.cpp's sprite arm is handed.  So the signature is
//  __cdecl (Ctx*, Obj*), arg0 the context at 0x8(%ebp) and arg1 the object
//  at 0xc(%ebp).
//
//  FIVE INDEPENDENT TELLS that this is a polygrid:
//    * the object carries a WIDTH at +0x1f0 and a HEIGHT at +0x1f4 and a
//      byte array at +0x1a8 with row stride +0x1f0 -- a heightfield;
//    * the vertex buffer is sized `+0x1f0 * +0x1f4` entries of 40 bytes
//      (`lea (%esi,%esi,4),%eax ; add %eax,%eax ; add %eax,%eax ;
//      add %eax,%eax` = *40 at 0x1005aecc) -- one vertex per grid cell;
//    * the index buffer at +0x1ac is walked 6 uint16 per quad and
//      `2*(W-1)` triangles per row for `H-1` rows (0x1005b2ee and
//      0x1005cb1e) -- the standard grid triangulation;
//    * the model matrix is built by 0x100151b0 from the object's position,
//      the quaternion at +0x9c and a SCALE of
//      (+0xac/(W-1)*W, +0xb0, +0xb4/(H-1)*H), then multiplied by a
//      translation of (-(W-1)/2, [-128], -(H-1)/2) -- i.e. cell index space
//      centred and scaled into world units;
//    * the DrawPrimitive at 0x1005caf0 and the two DrawIndexedPrimitive at
//      0x1005cc6b / 0x1005ce7b use FVF **0x2c4** =
//      D3DFVF_XYZRHW|DIFFUSE|SPECULAR|**TEX2**, which is exactly the
//      40-byte vertex the whole function manipulates (x,y,z,rhw at +0..+0xc,
//      two colours at +0x10/+0x14, u0,v0 at +0x18/+0x1c and u1,v1 at
//      +0x20/+0x24 -- and the clipper interpolates all FOUR of those
//      floats, which a one-texture vertex would not have).
//
//  Compiler: Intel C/C++ 4.0, **/O2**.  The /O2 tells, both rows:
//    * every outgoing argument is `add $-N,%esp` followed by
//      `mov %reg,K(%esp)` (0x1005a974, 0x1005ab5c, 0x1005ae33, 0x1005afc0,
//      0x1005cab7, 0x1005cc2b ...); there is not one `push` of an argument
//      in the whole 10K;
//    * the epilogue is DUPLICATED at every exit -- five separate
//      `mov (%esp),%ebx ; mov 0x4(%esp),%esi ; mov 0x8(%esp),%edi ;
//      mov %ebp,%esp ; pop %ebp ; ret` at 0x1005cf21, 0x1005cf5c,
//      0x1005cfa1, 0x1005d126 and 0x1005d161, no cross-jump anywhere;
//    * `&x` is `mov $K,%reg ; add %p,%reg` at 0x1005abee, 0x1005b30f,
//      0x1005b868, 0x1005b89a and 0x1005cc79.
//  Cold-block sinking (the six clip-epsilon second halves at
//  0x1005cfb0..0x1005d090, the "triangle wholly inside" arm at 0x1005d0b0
//  and all four early-return destructor blocks) is /O2 behaviour too.
//
//  ------------------------------------------------------------------- //
//  THE ACCEPTANCE FIGURE AND WHAT IT DOES *NOT* WITNESS.  Read this first.
//  ------------------------------------------------------------------- //
//
//  54 calls in 10256 bytes = 190 B/call, 47 direct across 23 DISTINCT
//  callees.  That is mid-strength for a call test, so here is what
//  else pins the decode and where the test is blind.
//
//  * **A 5987-BYTE CALL-FREE STRETCH, 0x1005b392..0x1005caf0**, 58% of the
//    function.  The call-sequence comparison cannot see one byte of it.  What pins it instead:
//    it is SIX near-identical blocks 0x378-0x385 bytes apart
//    (0x1005b541, 0x1005b8c4, 0x1005bc3c, 0x1005bfb5, 0x1005c336,
//    0x1005c6bb) gated by mask bits 0x01, 0x04, 0x08, 0x10, 0x20 and 0x02
//    IN THAT ORDER, testing
//      z >= g_f1008b908 | -z < x | y < z | x < z | -z < y | z <= g_f1008b910
//    -- the SAME six planes in the SAME order, with the same -0.00001f /
//    +0.00001f / 0.0f epsilon triple and the same
//    `pIn = pOutStart ; nIn = (pOut-pOutStart) ; pOut += nIn` tail, that
//    big61a30.cpp reconstructs at 0x10062da6..0x100641bb.  Two independent
//    functions in one module emitting the same clipper is a second reading
//    of one source.  The frame confirms the array bound without guessing:
//    the six flag arrays are at -0x68c, -0x5b0, -0x4d4, -0x3f8, -0x31c and
//    -0x240, exactly 0xdc apart, of which 0xc8 = 200 bytes = 50 int32 is
//    the array and 0x14 is the five int slots the five byte-lerps need.
//    50 is READ OFF THE FRAME.
//
//  * The rest of the decode is pinned by the callee set, which names the
//    subsystems outright: 0x1005a490 (blend/fog chooser, d3dr3c.cpp),
//    0x1000adc0 (scoped render-state saver, d3dbatcha.cpp) with states
//    0x13/0x14/0x1c/0x22, 0x1005a410 (sphere-vs-frustum with an outcode
//    mask, d3dr3d.cpp), 0x100151b0 (pos+quat+scale -> matrix, d3dbatchd),
//    0x10014720 (MatMul), 0x1001e480 (GetTranslation), 0x1001e4d0
//    (GetBasisVectors), 0x1005a550 (in-place homogeneous transform of an
//    array of 40-BYTE vertices, d3dr3b.cpp -- which is what fixes the
//    vertex size independently of the FVF), 0x10015800 (LTMatrix
//    operator*), 0x1004b640 (LTVector +=), 0x1005a600 (~saver).
//
//  * THE OTHER CALL-FREE STRETCHES worth naming: the two projection loops
//    at 0x1005cb66..0x1005cc13 and 0x1005cd76..0x1005ce23 (233 and 173
//    bytes) and the FPU block at 0x1005abe6..0x1005ad8d (423 bytes).  All
//    three are decoded below and none is witnessed by the test.
//
//  ------------------------------------------------------------------- //
//  REGION MAP, with what pins each one.
//  ------------------------------------------------------------------- //
//
//  0x1005a960..0x1005a9a5  PROLOGUE.  `sub $0x9c0,%esp` then the three
//      callee-saved registers into the BOTTOM of that block
//      (0x8/0x4/0x0(%esp)) and `add $-0x18,%esp` for the outgoing area --
//      icl's frame, so the locals run -0x004 .. -0x9b8.
//
//  0x1005a9a5..0x1005aa26  BLEND SETUP, byte for byte big61a30.cpp's:
//      sub_1005a490(pObj, &a,&b,&c,&d) then four C1000adc0 savers at
//      states 0x13, 0x14, 0x1c, 0x22 = SRCBLEND, DESTBLEND, FOGENABLE,
//      FOGCOLOR.  Note it happens BEFORE the first guard, which is why the
//      +0x1a8 test at 0x1005aa2e jumps to a destructing exit.
//
//  0x1005aa26..0x1005ab05  THE BOUNDING SPHERE.  Three half-extents
//      ((float)W*0.5f*+0xac, 128.0f*+0xb0, +0xb4*((float)H*0.5f)) into the
//      three consecutive float slots -0x874/-0x870/-0x86c, then
//      `fsqrt ; fadds 1.0f` -- icl expands sqrt() to fsqrt, so there is no
//      call here and there is none in the shipped bytes either.  The result
//      goes to 0x1005a410 with `this+0x3d0` as the plane array and a stack
//      local as the outcode mask; d3dr3d.cpp pinned that signature from
//      THIS call site (it says "1 caller (0x1005aaf5)").
//      The unsigned->float conversions are `mov %eax,-0x120 ;
//      movl $0,-0x11c ; fildll -0x120`, which is why W and H are uint32.
//
//  0x1005ab05..0x1005abe6  TEXTURE BINDING.  Two calls to 0x10015320
//      (d3dbatcha.cpp's lookup-or-create per stage, `(pTex, nStage,
//      nFlag)`).  The first binds the object's DETAIL/environment texture
//      at stage 0 and only when five things are all non-null; the second
//      binds the polygrid's own texture at stage `bDetail ? 1 : 0`.  If
//      that second bind fails the stage is unbound with 0x100232b0 and
//      both bDetail and the stage collapse to 0.  The resulting stage index
//      is what indexes g_a1008e664 at 0x1005acaa (`0x1008e664(,%eax,8)`),
//      which is how we know it is a stage number and not a bool.
//
//  0x1005abe6..0x1005ad8d  THE PUBLISHED SCALES.  Five floats into
//      0x1008fe88..0x1008fe98 and a three-float scale vector into the
//      consecutive slots -0x864/-0x860/-0x85c.  Every operand is read off
//      the bytes; the constants are 0.5f (0x100aa948/0x94c), 128.0f
//      (0x100aa950), 1.0f (0x100aa954/0x95c/0x964) and 1.0f/255.0f
//      (0x100aa970 = 0x3b808081).  `or $0x40000000` into +0x88 at
//      0x1005acfa and `test $0x3f,%al ; setne` on the outcode mask at
//      0x1005ad0c are in the middle of the FPU block purely as scheduling.
//
//  0x1005ad8d..0x1005ae9a  THE MODEL MATRIX.  0x100151b0(pos, &+0x9c,
//      &vScale, &mObj); then 0x1001e410 (d3dr2b.cpp: the __cdecl wrapper
//      that calls a no-argument __thiscall on its argument -- Identity)
//      on a SECOND matrix whose fourth column is then written at +0x0c,
//      +0x2c and, only when +0x88 & 2, +0x1c = 0xc3000000 = -128.0f;
//      then MatMul(&mWorld, &mObj, &mCentre) and GetTranslation /
//      GetBasisVectors off mWorld.  -0x818+0x0c/-0x1c/-0x2c ARE the
//      fourth column: -0x80c, -0x7fc, -0x7ec.
//
//  0x1005ae9a..0x1005aef9  THE VERTEX CACHE.  `W*H > g_n1008fe9c` grows
//      it: free 0x1008fe60 through 0x10021cb0, allocate W*H*40 through
//      0x10021c20, republish both the pointer and the new capacity.  On
//      failure the capacity is zeroed and the function returns through the
//      destructors (0x1005d0f0).
//
//  0x1005aef9..0x1005b23e  THE TWO VERTEX LOOPS.  Row-major over the
//      heightfield: vCur walks by the matrix's FORWARD vector once per row,
//      vRow walks by RIGHT once per column, and (fU,fV) count 0,1,2,...
//      as doubles and are handed over as floats.  +0x88 & 2 chooses
//      between `movzbl` + 0x1005d170 and `movsbl` + 0x1005d3e0 -- an
//      UNSIGNED and a SIGNED reading of the same height byte, two textual
//      copies of one loop (they use different frame slots for fU, -0xe8
//      against -0xf8, so they are different variables in different
//      scopes).  See the register-ABI note below.
//
//  0x1005b23e..0x1005b281  When the detail texture bound, 0x1005a630 (a
//      register-ABI callee) and 0x10011450 (d3dr5c.cpp's two-entry
//      texture-stage setup; d3dr5c says "0x1005b279 calls 0x10011450",
//      i.e. THIS site) run the second stage's set-up.
//
//  0x1005b281..0x1005cc6e  THE CLIPPED PATH, taken when the frustum test
//      left any outcode bit set.  Grow the index scratch at 0x1008fe64 to
//      +0x1e0 uint16, transform every vertex into camera space with
//      0x1005a550 and `this+0x15c`, then for each of the 2*(W-1)*(H-1)
//      triangles call 0x1005d650 with the outcode mask and the three
//      vertices:
//        0 -> wholly inside, append its three indices to the scratch
//             (the sunk block at 0x1005d0b0);
//        1 -> wholly outside, skip;
//        else -> copy the three vertices into a local 3-vertex array, run
//             the six clip passes, project with the six globals at
//             0x1008bb9c..0x1008bbb0 and draw ONE TRIANGLE FAN.
//      Then project every vertex with the global 4x4 at 0x1008ba1c and
//      draw the collected indices as ONE TRIANGLE LIST.
//
//  0x1005cc73..0x1005ce7b  THE UNCLIPPED PATH.  `this+0x19c * this+0x15c`
//      through 0x10015800, project every vertex with it and draw the
//      object's OWN index buffer (+0x1ac, +0x1e0 indices) as one triangle
//      list.  Same loop body as the clipped path's second projection, with
//      a local matrix instead of the global one -- which is why the two
//      1.0f reciprocal constants are DIFFERENT slots (0x100aa99c and
//      0x100aaa0c): icl gives every textual float literal in a distinct
//      basic block its own .rdata slot, so two slots means two textual
//      copies.
//
//  0x1005ce7e..0x1005cf2f  0x10011500 (d3dr2b.cpp: turn texture stage 1
//      off) when the detail texture was bound, then the four savers
//      unwound in reverse order INLINE as four `call *0x50` SetRenderState,
//      then the epilogue.
//
//  0x1005cf30..0x1005d16f  The four sunk early-return blocks and the six
//      sunk clip-epsilon arms.  See the note on the destructors below.
//
//  ------------------------------------------------------------------- //
//  FIVE THINGS THIS FILE CANNOT REPRODUCE, all stated up front.
//  ------------------------------------------------------------------- //
//
//  1. THREE REGISTER-ABI CALLEES (icl /Qip), which is why this function is
//     rated tier Y.  They are decoded as ordinary calls
//     and noted, so:
//       * 0x1005d170 and 0x1005d3e0 are entered with %eax=arg0, %edx=arg1,
//         %ecx=arg2 and ST(0)=arg4, and the caller still reserves a
//         0x20-byte outgoing area and fills 0xc, 0x14, 0x18 and 0x1c of it.
//         The callees prove the shape: both open `fstpl -0x10(%ebp) ;
//         mov %eax,-0x18(%ebp) ; mov %ecx,%eax` and then read 0x14(%ebp)
//         (a `flds`, so arg3 is a float), 0x1c(%ebp), 0x20(%ebp) and
//         0x24(%ebp) -- and NEVER 0x18(%ebp), which is the slot arg4
//         reserved and did not use because it arrived in ST(0).  So the
//         argument list is (pObj, pVert, nHeight, float fV, float fU,
//         LTVector* pRow, LTVector* pUp, int32 bDetail), eight slots, and
//         it is declared __cdecl below with exactly that list.
//       * 0x1005a630 is entered with %eax=arg0, %edx=arg1, %ecx=arg2 and
//         arg3 at 0xc(%esp); its own `mov 0x20(%esp),%esi` after four
//         pushes reads that fourth slot.  Declared __cdecl with four
//         arguments.
//     SUPERSEDED: /Qip gives exactly that convention to a
//     static in the caller's TU; the three are now statics here (see their
//     banners below), with the parameter order their register slots imply.
//
//  2. THE SHIPPED TU WAS BUILT WITH /QIfist.  All thirty float->byte
//     conversions in the clipper are
//         fstps <float slot> ; flds <float slot> ; fistpl <int slot>
//     -- a bare `fistp` in the current rounding mode with no fnstcw/fldcw
//     dance and no helper call.  big61a30.cpp records the same thing for
//     the sprite pass and d3dbatchd.cpp for 0x10015190.  Under plain /O2 a
//     `(uint8)(int32)f` cast emits the control-word dance instead, so those
//     sites will differ in bytes.  They contain NO calls.
//
//  3. 0x10023380 IS INLINED HERE, six times, and EXTENDED.  d3dbatchd.cpp
//     describes that helper as "two floats at +0x18 and +0x1c interpolated
//     normally, then five bytes (+0x12, +0x11, +0x10, +0x13, +0x17)".  The
//     six clip passes here carry that body inline with FOUR floats
//     (+0x18, +0x1c, +0x20, +0x24) and the same five bytes in the same
//     order, because this vertex has two texture coordinate pairs.  There
//     is no `call 0x10023380` anywhere in the shipped 10K, so it is an
//     `inline` function below.
//
//  4. THE 40-BYTE STRUCT COPIES ARE UNROLLED, NOT `rep movsl`.  Every one
//     of them -- the three triangle vertices at 0x1005b3d0, and
//     `*pOut++ = *pPrev` in all six clip passes -- is ten separate dword
//     load/store pairs.  The rule that struct assignment is always
//     `rep movsl` does not hold at this size in this translation unit;
//     recorded, not worked around.
//
//  5. THE STRUCT COPIES MAY COST A CALL WE DO NOT WANT.  Three places copy
//     an aggregate by value: `aTri[k] = pVerts[...]` and `*pOut++ = *pPrev`
//     (40 bytes each) and `mFull = A * B` (64 bytes, out of the NRVO
//     temporary).  The shipped build inlines all of them.  If icl decides
//     to call `memcpy` for any of them, that shows up as an OURS-ONLY row
//     in the acceptance test and is this and nothing else; the fix is to
//     spell the copy out field by field.
//
//  ------------------------------------------------------------------- //
//  WHAT IS *NOT* PINNED -- read this before trusting any byte of it.
//  ------------------------------------------------------------------- //
//
//  * SIX DEAD 1.0f CONSTANTS, exactly as big61a30.cpp found: 0x100aa9ac,
//    0x100aa9b4, 0x100aa9bc, 0x100aa9c4, 0x100aa9cc and 0x100aa9d4 are
//    1.0f slots interleaved with the six -0.00001f clip epsilons
//    (0x100aa9a8 + 8k) and referenced from nowhere.  Each clip pass's
//    source mentions a 1.0f that icl folded away and the spelling below
//    does not.
//
//  * THE THREE `* 1.0f` FACTORS at 0x1005ad0e, 0x1005ad2a and in the
//    reciprocal at 0x1005cb8a/0x1005cd9a.  0x100aa95c and 0x100aa964 are
//    literal 1.0f multiplied INTO the two published scales -- a `fmuls`
//    that cannot be anything else and cannot be explained either.  Written
//    as a literal `* 1.0f` so the bytes are reproduced; the macro it came
//    from is not recoverable.
//
//  * THE FOUR ROWS' FLOAT ASSOCIATION in the two projection loops.  Rows
//    0, 1 and 2 accumulate last-term-backwards (`(m02*z + m01*y) + m00*x`)
//    but the w row does NOT -- it is `(m30*x + m32*z) + m31*y`.  d3dr3b.cpp
//    hit exactly this in 0x1005a550 and said so: "0x100149a0 adds
//    (m32*z + m31*y) + m30*x; 0x1005a550 adds (m30*x + m32*z) + m31*y ...
//    the source is not simply MatVMul_InPlace_H in a loop, and I would be
//    guessing at what it is instead."  This file does not guess either: all four
//    rows are written below in the plain `m0*x + m1*y + m2*z + m3` form,
//    and the w row's association is expected to differ.  No calls.
//
//  * THE STORE ORDER inside the projection loops (x, rhw, z, y, with z and
//    y deferred one iteration by software pipelining) is icl's schedule,
//    not a source order.
//
//  * WHY THE UNCLIPPED PATH HOLDS THE MATRIX TWICE.  The operator* result
//    lands at -0x750 and is then copied dword by dword into -0x164..-0x128
//    before the loop.  Either the source has a named local assigned from
//    the temporary (written that way below) or icl hoisted sixteen
//    loop-invariant loads; it does not do the same for the GLOBAL matrix in
//    the other copy of the loop, which argues for the former.  Not proved.
//
//  * THE INSIDE-CLASSIFICATION LOOPS are emitted pointer-walking
//    (`add $0x28,%edx ; cmp %edi,%edx`) while the EDGE loops are emitted
//    index-walking on the flag array and pointer-walking on the vertices.
//    Both are written index-form below, as big61a30.cpp writes them; the
//    difference is induction-variable strength reduction, not structure.
//
//  COIN-FLIP, stated with both spellings so one build settles it: THE FOUR
//  SAVERS' DESTRUCTORS.  The shipped code restores them two different ways
//  -- INLINE as four `call *0x50(%edx)` at the normal exit
//  (0x1005ceaf/0x1005ced4/0x1005cef9/0x1005cf1e) and OUT OF LINE as four
//  `call 0x1005a600` in each of the four sunk early-return blocks
//  (0x1005cf36.., 0x1005cf7b.., 0x1005d100.., 0x1005d13b..), sixteen calls
//  in all.  Both orders are reverse-of-construction.  Below they are
//  written by hand: the normal exit spells the four SetRenderState calls
//  out and each early exit is a `goto` to one of four labelled blocks
//  placed at the BOTTOM of the function in the shipped address order, so
//  that the emitted call sequence matches.  The alternative -- give
//  C1000adc0 a real `~C1000adc0()` and use plain `return` statements -- is
//  what the shipped source almost certainly said; if a build shows icl
//  reproducing the inline-at-one-exit / out-of-line-at-the-others split by
//  itself, that spelling is the better one and this hand-written tail
//  should be replaced by it.
//
//  ------------------------------------------------------------------- //
//  SECOND PASS: from 10432 bytes / residue 122 / 156 diff blocks to
//  10272 bytes / residue 37 / 68 blocks (shipped 10256).  Each item was
//  measured with tools/fndiff.py and tools/census.py; the four identical
//  functions of this file stayed identical throughout.
//  ------------------------------------------------------------------- //
//
//  * THE NINE COUNTED LOOPS COUNT DOWN IN THE SOURCE.  icl 4.0 does not
//    reverse a count-up loop by itself: `for (i = 0; i < n; i++)` with
//    int32 gives `inc/cmp/jl`, with uint32 `inc/cmp/jb` (measured), never
//    the shipped `dec ; jne`.  `for (n = N; n; n--)` (and w28's `while
//    (n--)`) gives the shipped entry `test ; je` + tail `dec ; jne` for the
//    two vertex loops (both nestings), the triangle row/column loops, the
//    fan projection and the two vertex projections.  This alone was
//    -176 bytes and residue 122 -> 59.  w28's `while (n--)` on the same
//    nine loops is byte-neutral HERE but takes sub_1005a630 out of
//    identity (measured), so the `for` form stays.
//  * sub_10015320 RETURNS AN INTEGER HANDLE, declared `uint32` here.  As
//    `void*` icl predicts the result non-null (guide 09) and sinks both
//    the `bDetail = 0` arm and the Lnotexture block to the end; the
//    shipped keeps both inline, i.e. tests an integer.
//  * `nStage = bDetail ? 1 : 0` ONCE AFTER THE UNBIND JOIN: the shipped
//    unbind block falls through into the ternary's zero arm (0x1005abe4).
//  * `bNeedClip = !!(nClipMask & 0x3f)` is the `setne` at 0x1005ad16;
//    `((x & 0x3f) != 0)` and `(x & 0x3f) ? 1 : 0` both give branches
//    and a BYTE local.
//  * g_f1008fe98 (1/255) IS THE LAST PUBLISHED SCALE: the literal pool
//    0x100aa948..0x100aa984 reads 0.5 0.5 128 1 | 1 1 1 1 1 1 | 1/255 |
//    1 1 | 0.5 0.5 -128, and our pool now reads the same sequence.  The
//    six dead 1.0f are one per float DIVISION (six divisions in the
//    block), the two live ones the literal `* 1.0f`.
//  * THE BOUNDING SPHERE: `(float)sqrt(x*x + y*y + z*z) + 1.0f` -- the
//    `+ 1.0f` is a float add (`fadds`, not `faddl`), so the sum is float,
//    over an LTVector, with the two half extents `(float)W * 0.5f` and
//    `(float)H * 0.5f` computed first in temporaries (the pool opens
//    0.5 0.5 128 1.0).  That spelling gives the shipped stores and
//    reloads of x, y, z (-0x874/-0x870/-0x86c) with no member call.
//    `vDims.Mag()`, `MagSqr()` and a local `static inline` helper all
//    come out as CALLS here while Mag() inlines in sub_1005a630 above:
//    with the clipper removed (probe) MagSqr inlines, so the six inlined
//    clip passes exhaust icl's per-routine inline budget first.
//  * `++g_n1008be44` AFTER the pV/pEnd setup in each clip pass puts the
//    counter in %ecx around the pEnd lea, as all six shipped sites have
//    it (103 -> 71 blocks together with the half extents).
//  * fU IS A FLOAT LITERAL STORED AS A DOUBLE in the shipped (`flds
//    0x100aa98c ; fstpl -0xe8`, and `flds 1.0f ; faddl ; fstpl`); the
//    pool has four float entries 0x100aa988..0x100aa994 and three more at
//    0x100aaa10..0x100aaa18 for the signed copy.  `float fU` (block
//    scoped, one per copy) reproduces all of that: residue 47 -> 40.
//    CAUTION: in one intermediate state (an out-of-line MagSqr instance
//    in the TU) the same change flipped sub_1005a630's x87 stack order at
//    0x1005a8d2 (`fld %st(2) ; fmul %st(5)` -> `fld %st(4) ; fmul
//    %st(3)`); it does not with the file as it is.  Re-measure the
//    sibling after any change to the literal pool of this function.
//  * REGISTER ALLOCATION FOLLOWED: once the texture block matched, icl
//    put pObj in %ebx as the shipped does (it was %esi), which removed
//    most of the rename-only diff blocks.
//
//  WHAT REMAINS (68 blocks, residue 37; mov -15, lea +10, dec -6):
//  * the published-scales FPU block 0x1005abe6..0x1005ad8d.  The pool
//    is the witness: our 0x88..0xc4 is value for value the shipped
//    0x100aa948..0x100aa984, and every float DIVISION emits one dead
//    1.0f, so the six 1.0f between the sphere's and 1/255 read
//    D L D L D D -- two statements of [one division, `* 1.0f`] and then
//    two bare divisions.  That is the per-stage scale in a temporary
//    (`fScaleU = iw / d4 * ac * 1.0f`) and `g_f1008fe90 = fScaleU /
//    (float)(W - 1) * (float)W` -- which also gives the shipped chain
//    `fmuls ac ; fmuls 1.0f ; fdivp ; fmulp`.  Spelled as one expression
//    the live slots were 2nd and 5th (D L D D L D).  What is left is the
//    x87 schedule: the shipped converts W-1, W, H-1, H FIRST (for
//    fe90/fe94), then the byte, then a second W-1/W pair for vScale.x,
//    with fe88/fe8c in the middle; ours divides first.  Probes, all
//    worse or neutral: the four conversions as float locals (residue 44
//    / 73 blocks); `fInvWm1 = 1.0f / (float)(W - 1)` temporaries (size
//    10256, residue 39 / 73, the `fmuls 1.0f` vanish); fe88/fe8c after
//    fe90/fe94 (residue 40 / 67).
//  * the six clip-pass `iPrev = n - 1` as `lea -1(%eax),%edx` where the
//    shipped has `mov %edx,%ecx ; dec %edx` (n copied, then updated in
//    place; pIn lands in %eax there and in %ecx here).  w28_100256d0
//    gets the `dec` with this very spelling, so it is register context,
//    not the statement.
//  * bDetail kept in %esi across the second bind where ours spills it;
//    nH loaded before the `+0x88 & 2` test where ours sinks it into both
//    arms; the saver slots 8 bytes lower than shipped (-0x89c.. against
//    -0x894..).  The `lea` surplus is mostly alignment pads
//    (`lea 0x0(%edi,%eiz,1),%edi` 13 against 8) following from the
//    block sizes, not a source shape.
//
// *********************************************************************** //

#include "ltbasedefs.h"
#include <math.h>


// ---------------------------------------------------------------------- //
//  The D3D7 device.  Only three vtable slots are reached from here:
//  +0x50 SetRenderState, +0x64 DrawPrimitive and +0x68
//  DrawIndexedPrimitive -- the last two distinguished by their argument
//  counts (six dwords at 0x1005caf0, eight at 0x1005cc6b/0x1005ce7b).
//  d3dbatcha.cpp fixes +0x50 and +0x64 already.
// ---------------------------------------------------------------------- //
struct D3DDevice;

struct D3DDeviceVtbl
{
	uint8			m_pad_00[0x50];
	long (__stdcall* SetRenderState)(D3DDevice*, uint32, uint32);         // +0x50
	uint8			m_pad_54[0x64 - 0x54];
	long (__stdcall* DrawPrimitive)(D3DDevice*, uint32, uint32, void*,
	                                uint32, uint32);                      // +0x64
	long (__stdcall* DrawIndexedPrimitive)(D3DDevice*, uint32, uint32,
	                                       void*, uint32, uint16*,
	                                       uint32, uint32);               // +0x68
};

struct D3DDevice
{
	D3DDeviceVtbl*	lpVtbl;
};

extern D3DDevice* g_pD3DDevice;                             // 0x1008d988


// ---------------------------------------------------------------------- //
//  0x1000adc0 / 0x1005a600 -- d3dbatcha.cpp's scoped render-state saver and
//  d3dbatchd.cpp's restorer, declared with the SAME member signatures so
//  the external references resolve to those objects' symbols.  d3dbatchd
//  records 0x1005a600 as "40 bytes, 16 callers.  Pushes one cached render
//  state" -- sixteen of those callers are the four blocks at the bottom of
//  THIS function.
// ---------------------------------------------------------------------- //
class C1000adc0
{
public:
	C1000adc0*	sub_1000adc0(uint32 nState, uint32 nValue);
	void		sub_1005a600();

	uint32		m_nState00;                             // +0x00
	uint32		m_nOld04;                               // +0x04
};


// ---------------------------------------------------------------------- //
//  The vector.  0x1004b640 is d3dbatchc.cpp's in-place `operator+=`, a
//  __thiscall taking the three floats BY VALUE (`ret $0xc`); the four call
//  sites here each copy twelve bytes to (%esp) with three dword moves and
//  put the destination in %ecx.
// ---------------------------------------------------------------------- //
struct Vec5a960
{
	void	sub_1004b640(const Vec5a960 v);             // 0x1004b640

	float	x;
	float	y;
	float	z;
};


// ---------------------------------------------------------------------- //
//  The 4x4, indexed [row][column] exactly as d3dr2a.cpp and d3dbatchd.cpp
//  fix it: a translation lives in m[0][3], m[1][3], m[2][3] = +0x0c, +0x1c,
//  +0x2c, which is precisely where 0x1005adf8, 0x1005ae29 and 0x1005ae19
//  write.
// ---------------------------------------------------------------------- //
class Mat5a960
{
public:
	Mat5a960	sub_10015800(Mat5a960& other);          // operator *
	void		sub_1001e480(Vec5a960& vec);            // GetTranslation
	void		sub_1001e4d0(Vec5a960* pRight, Vec5a960* pUp,
	                         Vec5a960* pForward);       // GetBasisVectors

	float		m[4][4];
};


// The quaternion 0x100151b0 sanitises, at +0x9c of the object.
struct Rot5a960
{
	float	x;
	float	y;
	float	z;
	float	w;
};


// ---------------------------------------------------------------------- //
//  The 40-byte vertex.  FVF 0x2c4 = XYZRHW | DIFFUSE | SPECULAR | TEX2,
//  read straight off the three draw calls; the two extra floats at +0x20
//  and +0x24 are interpolated by every one of the six clip passes, which no
//  32-byte vertex would have.
// ---------------------------------------------------------------------- //
union VColor5a960
{
	uint32		m_n;
	uint8		m_b[4];
};

struct Vert5a960
{
	float		x;                                      // +0x00
	float		y;                                      // +0x04
	float		z;                                      // +0x08
	float		rhw;                                    // +0x0c
	VColor5a960	m_Diffuse10;                            // +0x10
	VColor5a960	m_Specular14;                           // +0x14
	float		tu0;                                    // +0x18
	float		tv0;                                    // +0x1c
	float		tu1;                                    // +0x20
	float		tv1;                                    // +0x24
};


// ---------------------------------------------------------------------- //
//  The thing 0x10015320 hands back and the object points at through +0x1bc.
//  Only +0x28 and +0x2c are read here, and both are only tested and passed
//  on, so they are void*.
// ---------------------------------------------------------------------- //
struct Tex5a960
{
	uint8		m_pad_00[0x28];
	void*		m_p28;                                  // +0x28
	uint32		m_n2c;                                  // +0x2c
};

// +0x1b4 of the object: a three-dword record whose +0x08 is a POINTER TO a
// texture pointer.  `lea 0x1b4(%ebx),%eax ; mov %eax,-0x1c(%ebp)` at
// 0x1005ab13 takes its address and keeps it, and both 0x1005ab1c and
// 0x1005b26e read `0x8(%eax)` off that saved address.
struct TexRef5a960
{
	void*		m_p00;                                  // +0x00
	void*		m_p04;                                  // +0x04
	Tex5a960**	m_pp08;                                 // +0x08
};


// The pair published per stage at 0x1008e664, indexed
// `0x1008e664(,%eax,8)` / `0x1008e668(,%eax,8)` -- the same eight-byte
// record big61a30.cpp names.
struct TexScale5a960
{
	float		m_fInvW00;                              // +0x00
	float		m_fInvH04;                              // +0x04
};


// ---------------------------------------------------------------------- //
//  The polygrid object.
// ---------------------------------------------------------------------- //
class ObjPG5a960
{
public:
	Vec5a960*	sub_100158b0();                         // 0x100158b0

	uint8		m_pad_00[0x88];
	uint32		m_nFlags88;                             // +0x88
	uint8		m_pad_8c[0x97 - 0x8c];
	uint8		m_nA97;                                 // +0x97
	uint8		m_pad_98[0x9c - 0x98];
	Rot5a960	m_Rot9c;                                // +0x9c
	float		m_fDimXac;                              // +0xac
	float		m_fDimYb0;                              // +0xb0
	float		m_fDimZb4;                              // +0xb4
	uint8		m_pad_b8[0x1a8 - 0xb8];
	uint8*		m_pData1a8;                             // +0x1a8
	uint16*		m_pIndices1ac;                          // +0x1ac
	void*		m_p1b0;                                 // +0x1b0
	TexRef5a960	m_sTex1b4;                              // +0x1b4
	uint8		m_pad_1c0[0x1cc - 0x1c0];
	float		m_f1cc;                                 // +0x1cc
	float		m_f1d0;                                 // +0x1d0
	float		m_f1d4;                                 // +0x1d4
	float		m_f1d8;                                 // +0x1d8
	uint8		m_pad_1dc[0x1e0 - 0x1dc];
	uint32		m_nNumIndices1e0;                       // +0x1e0
	uint8		m_pad_1e4[0x1f0 - 0x1e4];
	uint32		m_nWidth1f0;                            // +0x1f0
	uint32		m_nHeight1f4;                           // +0x1f4
};


// ---------------------------------------------------------------------- //
//  The context the callers pass: the fixed global at 0x1008ff60.
//  +0x15c and +0x19c are two 4x4s and +0x3d0 is the six-plane frustum
//  0x1005a410 walks.
// ---------------------------------------------------------------------- //
struct CtxPG5a960
{
	uint8		m_pad_00[0x15c];
	Mat5a960	m_m15c;                                 // +0x15c
	Mat5a960	m_m19c;                                 // +0x19c
	uint8		m_pad_1dc[0x3d0 - 0x1dc];
	LTPlane		m_aPlanes3d0[6];                        // +0x3d0
};


// ---------------------------------------------------------------------- //
//  Externals.  Relocations are masked, so only the shapes matter.
// ---------------------------------------------------------------------- //
extern Vert5a960*		g_p1008b87c;                    // 0x1008b87c
extern float			g_f1008b908;                    // 0x1008b908
extern float			g_f1008b910;                    // 0x1008b910
extern Mat5a960			g_m1008ba1c;                    // 0x1008ba1c
extern float			g_f1008bb9c;                    // 0x1008bb9c
extern float			g_f1008bba0;                    // 0x1008bba0
extern float			g_f1008bba4;                    // 0x1008bba4
extern float			g_f1008bba8;                    // 0x1008bba8
extern float			g_f1008bbac;                    // 0x1008bbac
extern float			g_f1008bbb0;                    // 0x1008bbb0
extern uint32			g_n1008be44;                    // 0x1008be44
extern void*			g_p1008d7a0;                    // 0x1008d7a0
extern uint32			g_n1008d7d0;                    // 0x1008d7d0
extern void*			g_p1008d950;                    // 0x1008d950
extern TexScale5a960	g_a1008e664[];                  // 0x1008e664
extern void*			g_a1008e684[];                  // 0x1008e684
extern Vert5a960*		g_p1008fe60;                    // 0x1008fe60
extern uint16*			g_p1008fe64;                    // 0x1008fe64
extern void*			g_p1008fe68;                    // 0x1008fe68
extern float			g_f1008fe88;                    // 0x1008fe88
extern float			g_f1008fe8c;                    // 0x1008fe8c
extern float			g_f1008fe90;                    // 0x1008fe90
extern float			g_f1008fe94;                    // 0x1008fe94
extern float			g_f1008fe98;                    // 0x1008fe98
extern uint32			g_n1008fe9c;                    // 0x1008fe9c
extern uint32			g_n1008fea0;                    // 0x1008fea0

extern "C" void  sub_1005a490(ObjPG5a960* pObj, uint32* pSrcBlend,
                              uint32* pDestBlend, uint32* pFogEnable,
                              uint32* pFogColor);              // 0x1005a490
extern "C" int32 sub_1005a410(Vec5a960* pPos, float fRadius,
                              LTPlane* pPlanes, uint32* pClipMask);
                                                               // 0x1005a410
extern "C" uint32 sub_10015320(void* pTex, uint32 nStage, uint32 nFlag);
                                                               // 0x10015320
extern "C" void  sub_100232b0(uint32 nStage);                  // 0x100232b0
extern "C" void  sub_100151b0(Vec5a960* pPos, Rot5a960* pRot,
                              Vec5a960* pScale, Mat5a960* pMat);
                                                               // 0x100151b0
extern "C" void  sub_1001e410(Mat5a960* pMat);                 // 0x1001e410
extern "C" void  sub_10014720(Mat5a960* pDest, Mat5a960* pMat1,
                              Mat5a960* pMat2);                // 0x10014720
extern "C" void  sub_10021cb0(void* p);                        // 0x10021cb0
extern "C" void* sub_10021c20(uint32 nBytes);                  // 0x10021c20
extern "C" void  sub_1005a550(Vert5a960* pVerts, uint32 nCount,
                              Mat5a960* pMat);                 // 0x1005a550
extern "C" void  sub_10011450(uint32 nMode);                   // 0x10011450
extern "C" void  sub_10011500();                               // 0x10011500

// The three register-ABI callees -- see note 1 in the banner.  They are
// /Qip statics of this TU (moved in from w7_10040cc0.cpp): the
// shipped 0x1005a960 hands them their first arguments in %eax/%edx/%ecx,
// which icl only does for an internal-linkage callee in the caller's TU.
// ======================================================================= //
//  0x1005d170 / 0x1005d3e0 -- 611 bytes each (then lea padding).  The two
//  per-vertex fills of the polygrid pass (0x1005a960 below), one for UNSIGNED
//  height bytes (`movzbl`) and one for SIGNED (`movsbl`):
//
//      pos   = pRow + pUp * (float)nHeight   (0x10015690, then 0x100155f0)
//      colour from the object's 256-entry {r,g,b,a} float table: +0x1f8
//            indexed by the byte (unsigned), or +0x9f8 = +0x1f8 + 128*16
//            indexed by the signed byte, i.e. entry h + 128;
//            r, g, b through the gamma ramps at 0x1008d1bc / d2bc / d3bc,
//            a scaled by g_f1008fe98 (no ramp)
//      (signed only) specular = 0xffffffff
//      the texture pair (fU + fe88) * fe90, (fV + fe8c) * fe94 goes to
//            +0x20/+0x24 for the detail pass, else +0x18/+0x1c.
//
//  Each float->int is the full /O2 truncation (`fstpl ; shr $0x14 ; and
//  $0x7ff ; cmp $0x3fe ; shld ...`), so the int result is a named local
//  (stored back to -0x70 after each).
//
//  IDENTICAL (re-measured).  Both are /Qip
//  REGISTER-ABI callees (%eax = pObj, %edx = pVert, %ecx = the height
//  byte, ST(0) = fU, the rest on the stack -- note 1), which only a /Qip
//  static in their caller's translation unit produces: they moved here
//  from w7_10040cc0.cpp.  Three things made them match:
//    * the parameter order (pObj, pVert, fU, fV, nHeight, ...): icl puts
//      the first float parameter in ST(0) and the third int in %ecx;
//    * the colour row taken as a pointer before the calls (pCol);
//    * nHeight a uint8 / int8 parameter, and the tail assigning the
//      float parameters (fV first) and storing U before V in each arm.
// ======================================================================= //
struct LTVec
{
	float x, y, z;

	LTVec  sub_100155f0(const LTVec v) const;   // 0x100155f0  operator +
	LTVec  sub_10015690(float v) const;         // 0x10015690  operator * (T)
};

struct W7Colour5d170
{
	float	r, g, b, a;
};

struct W7Obj5d170
{
	uint8			m_pad_000[0x1f8];                   // +0x000
	W7Colour5d170	m_Colours1f8[256];                  // +0x1f8
};

struct W7Vert5d170                                      // 40 bytes
{
	LTVec	m_Pos;                                      // +0x00
	float	m_0c;                                       // +0x0c
	uint8	m_10, m_11, m_12, m_13;                     // +0x10
	uint32	m_nSpecular14;                              // +0x14
	float	m_fU18, m_fV1c;                             // +0x18
	float	m_fU20, m_fV24;                             // +0x20
};

extern unsigned char	g_ColorRamp2[256];              // 0x1008d1bc
extern unsigned char	g_ColorRamp1[256];              // 0x1008d2bc
extern unsigned char	g_ColorRamp0[256];              // 0x1008d3bc
extern float			g_f1008fe88;                    // 0x1008fe88
extern float			g_f1008fe8c;                    // 0x1008fe8c
extern float			g_f1008fe90;                    // 0x1008fe90
extern float			g_f1008fe94;                    // 0x1008fe94
extern float			g_f1008fe98;                    // 0x1008fe98


static void sub_1005d170(W7Obj5d170* pObj, W7Vert5d170* pVert,
                        float fU, float fV, uint8 nHeight,
                        LTVec* pRow, LTVec* pUp, int32 bDetail);   // 0x1005d170
static void sub_1005d3e0(W7Obj5d170* pObj, W7Vert5d170* pVert,
                        float fU, float fV, int8 nHeight,
                        LTVec* pRow, LTVec* pUp, int32 bDetail);    // 0x1005d3e0

// ======================================================================= //
//  0x1005a630 -- 811 bytes (then `nop` and a lea-nop).  The polygrid's
//  per-vertex normals and environment-map coordinates, called by
//  0x1005a960 when the detail texture is bound.  For each vertex of the
//  W x H grid the height bytes (signed, +0x1a8) give two central
//  differences -- along a row (+-1) and down a column (+-W), one-sided on
//  the edges -- which tilt pUp by pRight and pForward:
//
//      sF = 0.5 * (1/|pForward|) * |pUp|^2/128     (hoisted, doubles)
//      sR = 0.5 * (1/|pRight|)   * |pUp|^2/128
//      N  = pForward * (dCol * sF) + pRight * (dRow * sR) + pUp ; N.Norm()
//      sub_1001e950(&eye, &vert, &N, &vert.u, &vert.v)
//
//  The edge differences are exactly as shipped: interior rows take
//  h[-W] - h[W], the row with y == 0 takes h[-W] - h[0] and the first row
//  (y == H-1) takes h[W] - h[0]; likewise along the row with -1/+1.  Both
//  counters count DOWN from the count (`lea -1(%eax)` then `test %eax`),
//  which is `while (n--)`, and the row pointers advance by W at the end of
//  each row.
//
//  IDENTICAL.  A /Qip register-only static (%eax =
//  pObj, %edx = pUp, %ecx = pRight, pForward on the stack), so it lives
//  in its caller's TU.  What closed it, in order:
//    * LTVector operators for the three products and the sum, and float
//      (not double) scale locals -- icl spills them as doubles (628 -> 23);
//    * fUp computed FIRST in the source, although icl schedules it last:
//      the shipped literal pool opens with its 1/128 ahead of the two
//      inverses' 1.0s, and the order fixes the three spill slots (-> 6);
//    * sF takes 1/|pForward| and sR 1/|pRight| (the two 0.5f products
//      were the wrong way round, which also made icl CSE 0.5f*fUp; the
//      shipped pool has two 0.5 entries) (-> 0).
// ======================================================================= //
struct W7Obj5a630
{
	uint8		m_pad_000[0x1a8];                       // +0x000
	int8*		m_pHeights1a8;                          // +0x1a8
	uint8		m_pad_1ac[0x1f0 - 0x1ac];               // +0x1ac
	uint32		m_nWidth1f0;                            // +0x1f0
	uint32		m_nHeight1f4;                           // +0x1f4
};

struct Vert5a960;
struct SPlane;
extern Vert5a960*	g_p1008fe60;                        // 0x1008fe60  the grid's vertices
extern LTVec		g_v1008bd34;                        // 0x1008bd34  the eye point
extern void sub_1001e950(const LTVec* pEye, const LTVec* pPos,
                         const SPlane* pNormal, float* pU, float* pV);  // 0x1001e950


static void sub_1005a630(W7Obj5a630* pObj, LTVector* pUp,
                             LTVector* pRight, LTVector* pForward)
{
	W7Vert5d170*	pRowV;
	int8*			pRowH;
	W7Vert5d170*	pV;
	int8*			pH;
	uint32			x, y;
	int32			d;
	float			fInvFwd, fInvRight, fUp, sF, sR;
	LTVector		vF, vR, vN;

	pRowV = (W7Vert5d170*)g_p1008fe60;
	pRowH = pObj->m_pHeights1a8;

	fUp       = pUp->MagSqr() / 128.0f;
	fInvFwd   = 1.0f / pForward->Mag();
	fInvRight = 1.0f / pRight->Mag();

	y = pObj->m_nHeight1f4;
	while (y--)
	{
		pV = pRowV;
		pH = pRowH;

		x = pObj->m_nWidth1f0;
		while (x--)
		{
			sF = 0.5f * fInvFwd * fUp;
			sR = 0.5f * fInvRight * fUp;

			if (y < pObj->m_nHeight1f4 - 1)
			{
				if (y > 0)
					vF = *pForward * ((pH[-(int32)pObj->m_nWidth1f0] - pH[pObj->m_nWidth1f0]) * sF);
				else
					vF = *pForward * ((pH[-(int32)pObj->m_nWidth1f0] - pH[0]) * sF);
			}
			else
			{
				vF = *pForward * ((pH[pObj->m_nWidth1f0] - pH[0]) * sF);
			}

			if (x < pObj->m_nWidth1f0 - 1)
			{
				if (x > 0)
					vR = *pRight * ((pH[-1] - pH[1]) * sR);
				else
					vR = *pRight * ((pH[-1] - pH[0]) * sR);
			}
			else
			{
				vR = *pRight * ((pH[1] - pH[0]) * sR);
			}

			vN = vF + (vR + *pUp);
			vN.Norm();

			sub_1001e950(&g_v1008bd34, &pV->m_Pos, (SPlane*)&vN,
			             &pV->m_fU18, &pV->m_fV1c);

			pV++;
			pH++;
		}

		pRowV += pObj->m_nWidth1f0;
		pRowH += pObj->m_nWidth1f0;
	}
}


extern "C" int32 sub_1005d650(uint32 nClipMask, uint32* pOutMask,
                              Vert5a960* pV0, Vert5a960* pV1,
                              Vert5a960* pV2);                 // 0x1005d650


// Round to nearest in the current FPU mode (w1_10001000.cpp's helper;
// 0x10015190 out of line): icl inlines a function holding __asm as
// `fstps f ; flds f ; fistpl n` -- the clip passes' thirty bare `fistpl`.
inline int32 RoundF(float f)
{
	int32 n;
	__asm fld f
	__asm fistp n
	return n;
}

// ======================================================================= //
//  THE CLIPPER for the 40-byte vertex -- d3dr3b.cpp's driver 0x1000aa90
//  over the six plane bodies w1_10002310.cpp / w44_10002d40.cpp compile
//  byte-identical out of line, all INLINED into the per-triangle
//  path of 0x1005a960 (0x1005b3c0 .. 0x1005ca20).  Named by plane, not by address: in this
//  /Qip TU icl also emits each helper out of line (unreferenced; the link
//  drops them), and an address in the name would make the byte comparison report the
//  copies as the shipped rows.  Planes in order: 0x10002310 near,
//  0x10002670 left, 0x100029e0 top, 0x10002d40 right, 0x100030a0 bottom,
//  0x10003410 far; ClipPolyA is 0x1000aa90.
// ======================================================================= //
#define CLIP_EPSILON    0.00001f
#define CLIP_MAX_VERTS  50

struct SClipVertA                                       // 0x28 bytes
{
	LTVector v;                                         // +0x00  x, y, z
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

static inline void LerpClipVertA(SClipVertA* a, SClipVertA* b, SClipVertA* dst, float t)
{
	dst->m_18 = (b->m_18 - a->m_18) * t + a->m_18;
	dst->m_1c = (b->m_1c - a->m_1c) * t + a->m_1c;
	dst->m_20 = (b->m_20 - a->m_20) * t + a->m_20;
	dst->m_24 = (b->m_24 - a->m_24) * t + a->m_24;
	dst->m_12 = (uint8)RoundF((b->m_12 - a->m_12) * t + a->m_12);
	dst->m_11 = (uint8)RoundF((b->m_11 - a->m_11) * t + a->m_11);
	dst->m_10 = (uint8)RoundF((b->m_10 - a->m_10) * t + a->m_10);
	dst->m_13 = (uint8)RoundF((b->m_13 - a->m_13) * t + a->m_13);
	dst->m_17 = (uint8)RoundF((b->m_17 - a->m_17) * t + a->m_17);
}

static inline float ClipEdgeNearA(LTVector* a, LTVector* b, LTVector* dst)
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

static inline float ClipEdgeLeftA(LTVector* a, LTVector* b, LTVector* dst)
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

static inline float ClipEdgeTopA(LTVector* a, LTVector* b, LTVector* dst)
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

static inline float ClipEdgeRightA(LTVector* a, LTVector* b, LTVector* dst)
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

static inline float ClipEdgeBottomA(LTVector* a, LTVector* b, LTVector* dst)
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

static inline float ClipEdgeFarA(LTVector* a, LTVector* b, LTVector* dst)
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

// 0x10002310
static inline int32 ClipPlaneNearA(uint32* pUnused, void** ppVertsArg, uint32* pnVertsArg, void** ppOutArg)
{
	SClipVertA** ppVerts = (SClipVertA**)ppVertsArg;
	SClipVertA** ppOut   = (SClipVertA**)ppOutArg;
	int32*       pnVerts = (int32*)pnVertsArg;
	uint32     anInside[CLIP_MAX_VERTS];
	SClipVertA*  pOutStart;
	SClipVertA*  pV;
	SClipVertA*  pEnd;
	SClipVertA*  pPrev;
	SClipVertA*  pCur;
	uint32*    pFlag;
	int32      nInside;
	int32      i;
	int32      iPrev;

	nInside = 0;
	pFlag   = anInside;
	pV      = *ppVerts;
	pEnd    = pV + *pnVerts;

	++g_n1008be44;

	for (; pV != pEnd; ++pV)
	{
		uint32 bIn = (pV->v.z >= g_f1008b908);

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
				float t = ClipEdgeNearA(&pPrev->v, &pCur->v, &(*ppOut)->v);

				LerpClipVertA(pPrev, pCur, *ppOut, t);

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

// 0x10002670
static inline int32 ClipPlaneLeftA(uint32* pUnused, void** ppVertsArg, uint32* pnVertsArg, void** ppOutArg)
{
	SClipVertA** ppVerts = (SClipVertA**)ppVertsArg;
	SClipVertA** ppOut   = (SClipVertA**)ppOutArg;
	int32*       pnVerts = (int32*)pnVertsArg;
	uint32     anInside[CLIP_MAX_VERTS];
	SClipVertA*  pOutStart;
	SClipVertA*  pV;
	SClipVertA*  pEnd;
	SClipVertA*  pPrev;
	SClipVertA*  pCur;
	uint32*    pFlag;
	int32      nInside;
	int32      i;
	int32      iPrev;

	nInside = 0;
	pFlag   = anInside;
	pV      = *ppVerts;
	pEnd    = pV + *pnVerts;

	++g_n1008be44;

	for (; pV != pEnd; ++pV)
	{
		uint32 bIn = (-pV->v.z < pV->v.x);

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
				float t = ClipEdgeLeftA(&pPrev->v, &pCur->v, &(*ppOut)->v);

				LerpClipVertA(pPrev, pCur, *ppOut, t);

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

// 0x100029e0
static inline int32 ClipPlaneTopA(uint32* pUnused, void** ppVertsArg, uint32* pnVertsArg, void** ppOutArg)
{
	SClipVertA** ppVerts = (SClipVertA**)ppVertsArg;
	SClipVertA** ppOut   = (SClipVertA**)ppOutArg;
	int32*       pnVerts = (int32*)pnVertsArg;
	uint32     anInside[CLIP_MAX_VERTS];
	SClipVertA*  pOutStart;
	SClipVertA*  pV;
	SClipVertA*  pEnd;
	SClipVertA*  pPrev;
	SClipVertA*  pCur;
	uint32*    pFlag;
	int32      nInside;
	int32      i;
	int32      iPrev;

	nInside = 0;
	pFlag   = anInside;
	pV      = *ppVerts;
	pEnd    = pV + *pnVerts;

	++g_n1008be44;

	for (; pV != pEnd; ++pV)
	{
		uint32 bIn = (pV->v.y < pV->v.z);

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
				float t = ClipEdgeTopA(&pPrev->v, &pCur->v, &(*ppOut)->v);

				LerpClipVertA(pPrev, pCur, *ppOut, t);

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

// 0x10002d40
static inline int32 ClipPlaneRightA(uint32* pUnused, void** ppVertsArg, uint32* pnVertsArg, void** ppOutArg)
{
	SClipVertA** ppVerts = (SClipVertA**)ppVertsArg;
	SClipVertA** ppOut   = (SClipVertA**)ppOutArg;
	int32*       pnVerts = (int32*)pnVertsArg;
	uint32     anInside[CLIP_MAX_VERTS];
	SClipVertA*  pOutStart;
	SClipVertA*  pV;
	SClipVertA*  pEnd;
	SClipVertA*  pPrev;
	SClipVertA*  pCur;
	uint32*    pFlag;
	int32      nInside;
	int32      i;
	int32      iPrev;

	nInside = 0;
	pFlag   = anInside;
	pV      = *ppVerts;
	pEnd    = pV + *pnVerts;

	++g_n1008be44;

	for (; pV != pEnd; ++pV)
	{
		uint32 bIn = (pV->v.x < pV->v.z);

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
				float t = ClipEdgeRightA(&pPrev->v, &pCur->v, &(*ppOut)->v);

				LerpClipVertA(pPrev, pCur, *ppOut, t);

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

// 0x100030a0
static inline int32 ClipPlaneBottomA(uint32* pUnused, void** ppVertsArg, uint32* pnVertsArg, void** ppOutArg)
{
	SClipVertA** ppVerts = (SClipVertA**)ppVertsArg;
	SClipVertA** ppOut   = (SClipVertA**)ppOutArg;
	int32*       pnVerts = (int32*)pnVertsArg;
	uint32     anInside[CLIP_MAX_VERTS];
	SClipVertA*  pOutStart;
	SClipVertA*  pV;
	SClipVertA*  pEnd;
	SClipVertA*  pPrev;
	SClipVertA*  pCur;
	uint32*    pFlag;
	int32      nInside;
	int32      i;
	int32      iPrev;

	nInside = 0;
	pFlag   = anInside;
	pV      = *ppVerts;
	pEnd    = pV + *pnVerts;

	++g_n1008be44;

	for (; pV != pEnd; ++pV)
	{
		uint32 bIn = (-pV->v.z < pV->v.y);

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
				float t = ClipEdgeBottomA(&pPrev->v, &pCur->v, &(*ppOut)->v);

				LerpClipVertA(pPrev, pCur, *ppOut, t);

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

// 0x10003410
static inline int32 ClipPlaneFarA(uint32* pUnused, void** ppVertsArg, uint32* pnVertsArg, void** ppOutArg)
{
	SClipVertA** ppVerts = (SClipVertA**)ppVertsArg;
	SClipVertA** ppOut   = (SClipVertA**)ppOutArg;
	int32*       pnVerts = (int32*)pnVertsArg;
	uint32     anInside[CLIP_MAX_VERTS];
	SClipVertA*  pOutStart;
	SClipVertA*  pV;
	SClipVertA*  pEnd;
	SClipVertA*  pPrev;
	SClipVertA*  pCur;
	uint32*    pFlag;
	int32      nInside;
	int32      i;
	int32      iPrev;

	nInside = 0;
	pFlag   = anInside;
	pV      = *ppVerts;
	pEnd    = pV + *pnVerts;

	++g_n1008be44;

	for (; pV != pEnd; ++pV)
	{
		uint32 bIn = (pV->v.z <= g_f1008b910);

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
				float t = ClipEdgeFarA(&pPrev->v, &pCur->v, &(*ppOut)->v);

				LerpClipVertA(pPrev, pCur, *ppOut, t);

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

// 0x1000aa90 -- d3dr3b.cpp's driver for this 40-byte vertex.
static inline LTBOOL ClipPolyA(uint32 nFlags, void** ppVerts, uint32* pnVerts)
{
	void*  pScratch;
	void*  pVerts;
	uint32 nVerts;
	uint32 nOut0;
	uint32 nOut1;
	uint32 nOut2;
	uint32 nOut3;
	uint32 nOut4;
	uint32 nOut5;

	if (g_p1008d7a0)
	{
		nFlags &= 0x01;
		if (!nFlags)
			return LTTRUE;
	}

	pScratch = g_p1008b87c;
	pVerts   = *ppVerts;
	nVerts   = *pnVerts;

	if (nFlags & 0x01)
	{
		if (!ClipPlaneNearA(&nOut0, &pVerts, &nVerts, &pScratch))
			return LTFALSE;
	}

	if (nFlags & 0x04)
	{
		if (!ClipPlaneLeftA(&nOut1, &pVerts, &nVerts, &pScratch))
			return LTFALSE;
	}

	if (nFlags & 0x08)
	{
		if (!ClipPlaneTopA(&nOut2, &pVerts, &nVerts, &pScratch))
			return LTFALSE;
	}

	if (nFlags & 0x10)
	{
		if (!ClipPlaneRightA(&nOut3, &pVerts, &nVerts, &pScratch))
			return LTFALSE;
	}

	if (nFlags & 0x20)
	{
		if (!ClipPlaneBottomA(&nOut4, &pVerts, &nVerts, &pScratch))
			return LTFALSE;
	}

	if (nFlags & 0x02)
	{
		if (!ClipPlaneFarA(&nOut5, &pVerts, &nVerts, &pScratch))
			return LTFALSE;
	}

	*ppVerts = pVerts;
	*pnVerts = nVerts;
	return LTTRUE;
}




void sub_1005a960(CtxPG5a960* pCtx, ObjPG5a960* pObj)
{
	uint32			nSrcBlend;
	uint32			nDestBlend;
	uint32			nFogEnable;
	uint32			nFogColor;
	C1000adc0		sSrcBlend;
	C1000adc0		sDestBlend;
	C1000adc0		sFogEnable;
	C1000adc0		sFogColor;

	LTVector		vDims;
	Vec5a960		vScale;
	Vec5a960		vOrigin;
	Vec5a960		vRight;
	Vec5a960		vUp;
	Vec5a960		vForward;
	Vec5a960		vCur;
	Vec5a960		vRow;

	Mat5a960		mObj;
	Mat5a960		mCentre;
	Mat5a960		mWorld;
	Mat5a960		mFull;

	TexRef5a960*	pRef;
	Tex5a960*		pTex;

	Vert5a960*		pVerts;
	Vert5a960*		pVert;
	Vert5a960*		pVerts2;
	Vert5a960*		pV;
	Vert5a960		aTri[3];

	uint16*			pRowIdx;
	uint16*			pIdx;
	uint16*			pOutIdx;

	uint8*			pRow;
	int8*			pRowS;
	uint8*			pCell;
	int8*			pCellS;

	uint32			nClipMask;
	uint32			nOutMask;
	uint32			nStage;
	int32			bDetail;
	int32			bNeedClip;
	int32			nResult;
	uint32			nVerts;
	uint32			nVerts2;
	int32			nIndices;
	uint32			nTrisPerRow;
	uint32			nIdxPerRow;
	uint32			nH;
	uint32			i;
	uint32			x;
	uint32			y;
	uint32			t;

	float			fRadius;
	// float, not double: as a double it takes an 8-byte
	// slot of its own and the frame is 0x9d0; as a float the frame is the
	// shipped 0x9c0 exactly.
	float			fV;
	float			fHalfW;
	float			fScaleU;
	float			fScaleV;
	float			fHalfH;
	float			fPx;
	float			fPy;
	float			fPz;
	float			fPw;

	// -- 0x1005a9a5: the blend and fog state, saved BEFORE any guard ---- //
	sub_1005a490(pObj, &nSrcBlend, &nDestBlend, &nFogEnable, &nFogColor);

	sSrcBlend.sub_1000adc0(0x13, nSrcBlend);        // D3DRENDERSTATE_SRCBLEND
	sDestBlend.sub_1000adc0(0x14, nDestBlend);      // D3DRENDERSTATE_DESTBLEND
	sFogEnable.sub_1000adc0(0x1c, nFogEnable);      // D3DRENDERSTATE_FOGENABLE
	sFogColor.sub_1000adc0(0x22, nFogColor);        // D3DRENDERSTATE_FOGCOLOR

	// -- 0x1005aa26: no heightfield, nothing to draw ------------------- //
	if (!pObj->m_pData1a8)
		goto Lexit_d;

	// -- 0x1005aa34: the bounding sphere.  Three half-extents into three
	//    consecutive float slots, then sqrt + 1.0f.  icl expands sqrt() to
	//    `fsqrt` (0x1005aabc), which is why there is no call here.
	fHalfW  = (float)pObj->m_nWidth1f0  * 0.5f;
	fHalfH  = (float)pObj->m_nHeight1f4 * 0.5f;
	vDims.x = fHalfW * pObj->m_fDimXac;
	vDims.y = 128.0f * pObj->m_fDimYb0;
	vDims.z = pObj->m_fDimZb4 * fHalfH;

	fRadius = (float)sqrt(vDims.x * vDims.x + vDims.y * vDims.y +
	                      vDims.z * vDims.z) + 1.0f;

	if (!sub_1005a410(pObj->sub_100158b0(), fRadius,
	                  pCtx->m_aPlanes3d0, &nClipMask))
		goto Lexit_a;

	// -- 0x1005ab05: bind the textures --------------------------------- //
	//    Five null tests in one `&&` chain, then the detail texture at
	//    stage 0; the grid's own texture then goes to stage 1 if that
	//    worked and stage 0 if it did not.
	if (!pObj->m_p1b0)
		goto Lnotexture;

	pRef = &pObj->m_sTex1b4;

	if (!pRef->m_pp08)
		goto Lnotexture;

	pTex = *pRef->m_pp08;

	if (!pTex)
		goto Lnotexture;

	if (g_p1008d950 && g_a1008e684[0] && pTex->m_p28 && pTex->m_n2c &&
	    g_p1008fe68)
		bDetail = sub_10015320(pTex->m_p28, 0, 0) ? 1 : 0;
	else
		bDetail = 0;

	if (!sub_10015320(*pRef->m_pp08, bDetail ? 1 : 0, 0))
	{
Lnotexture:
		sub_100232b0(g_n1008d7d0);
		bDetail = 0;
	}

	nStage = bDetail ? 1 : 0;

	// -- 0x1005abe6: publish the five scales the vertex builder reads --- //
	//    W and H convert through `mov ; movl $0 ; fildll`, i.e. UNSIGNED.
	//    The two `* 1.0f` are literal `fmuls 0x100aa95c` / `0x100aa964`;
	//    see the banner.
	g_f1008fe88 = g_a1008e664[nStage].m_fInvW00 * pObj->m_f1cc;
	g_f1008fe8c = g_a1008e664[nStage].m_fInvH04 * pObj->m_f1d0;

	fScaleU = g_a1008e664[nStage].m_fInvW00 / pObj->m_f1d4 *
	          pObj->m_fDimXac * 1.0f;
	fScaleV = g_a1008e664[nStage].m_fInvH04 / pObj->m_f1d8 *
	          pObj->m_fDimZb4 * 1.0f;

	g_f1008fe90 = fScaleU / (float)(pObj->m_nWidth1f0 - 1) *
	              (float)pObj->m_nWidth1f0;
	g_f1008fe94 = fScaleV / (float)(pObj->m_nHeight1f4 - 1) *
	              (float)pObj->m_nHeight1f4;

	g_f1008fe98 = (float)pObj->m_nA97 * (1.0f / 255.0f);

	pObj->m_nFlags88 |= 0x40000000;

	bNeedClip = !!(nClipMask & 0x3f);

	// -- 0x1005ad24: cell index space -> world units ------------------- //
	vScale.x = pObj->m_fDimXac / (float)(pObj->m_nWidth1f0  - 1) *
	           (float)pObj->m_nWidth1f0;
	vScale.y = pObj->m_fDimYb0;
	vScale.z = pObj->m_fDimZb4 / (float)(pObj->m_nHeight1f4 - 1) *
	           (float)pObj->m_nHeight1f4;

	// -- 0x1005ad8d: the model matrix ---------------------------------- //
	sub_100151b0(pObj->sub_100158b0(), &pObj->m_Rot9c, &vScale, &mObj);

	sub_1001e410(&mCentre);

	mCentre.m[0][3] = -((float)(pObj->m_nWidth1f0  - 1) * 0.5f);
	mCentre.m[2][3] = -((float)(pObj->m_nHeight1f4 - 1) * 0.5f);

	if (pObj->m_nFlags88 & 0x2)
		mCentre.m[1][3] = -128.0f;

	sub_10014720(&mWorld, &mObj, &mCentre);

	mWorld.sub_1001e480(vOrigin);
	mWorld.sub_1001e4d0(&vRight, &vUp, &vForward);

	// -- 0x1005ae9a: grow the vertex cache ----------------------------- //
	nVerts = pObj->m_nWidth1f0 * pObj->m_nHeight1f4;

	if ((uint32)nVerts > g_n1008fe9c)
	{
		sub_10021cb0(g_p1008fe60);

		g_p1008fe60 = (Vert5a960*)sub_10021c20(nVerts * 40);

		if (!g_p1008fe60)
			goto Lexit_c;

		g_n1008fe9c = nVerts;
	}

	pVerts = g_p1008fe60;

	// -- 0x1005aef9: build one vertex per cell ------------------------- //
	fV     = 0.0f;
	vCur   = vOrigin;
	pVert  = pVerts;
	nH     = pObj->m_nHeight1f4;

	if (pObj->m_nFlags88 & 0x2)
	{
		// 0x1005af4a: UNSIGNED height bytes (`movzbl`), 0x1005d170.
		float fU;

		pRow = pObj->m_pData1a8;

		for (y = nH; y; y--)
		{
			fU    = 0.0f;
			vRow  = vCur;
			pCell = pRow;

			for (x = pObj->m_nWidth1f0; x; x--)
			{
				sub_1005d170((W7Obj5d170*)pObj, (W7Vert5d170*)pVert, (float)fU, (float)fV, *pCell,
				             (LTVec*)&vRow, (LTVec*)&vUp, bDetail);

				pCell++;
				pVert++;
				vRow.sub_1004b640(vRight);
				fU = 1.0f + fU;
			}

			vCur.sub_1004b640(vForward);
			fV   = 1.0f + fV;
			pRow = pRow + pObj->m_nWidth1f0;
		}
	}
	else
	{
		// 0x1005b0c3: SIGNED height bytes (`movsbl`), 0x1005d3e0.
		float fU;

		pRowS = (int8*)pObj->m_pData1a8;

		for (y = nH; y; y--)
		{
			fU     = 0.0f;
			vRow   = vCur;
			pCellS = pRowS;

			for (x = pObj->m_nWidth1f0; x; x--)
			{
				sub_1005d3e0((W7Obj5d170*)pObj, (W7Vert5d170*)pVert, (float)fU, (float)fV, *pCellS,
				             (LTVec*)&vRow, (LTVec*)&vUp, bDetail);

				pCellS++;
				pVert++;
				vRow.sub_1004b640(vRight);
				fU = 1.0f + fU;
			}

			vCur.sub_1004b640(vForward);
			fV    = 1.0f + fV;
			pRowS = pRowS + pObj->m_nWidth1f0;
		}
	}

	// -- 0x1005b23e: the detail stage --------------------------------- //
	if (bDetail)
	{
		sub_1005a630((W7Obj5a630*)pObj, (LTVector*)&vUp, (LTVector*)&vRight, (LTVector*)&vForward);
		sub_10011450((*pRef->m_pp08)->m_n2c);
	}

	// ------------------------------------------------------------------ //
	//  0x1005b281.  The frustum test left outcode bits set, so every
	//  triangle has to be checked and some of them clipped.
	// ------------------------------------------------------------------ //
	if (bNeedClip)
	{
		// -- 0x1005b28c: grow the index scratch ----------------------- //
		if (pObj->m_nNumIndices1e0 > g_n1008fea0)
		{
			g_p1008fe64 = (uint16*)sub_10021c20(pObj->m_nNumIndices1e0 * 2);

			if (!g_p1008fe64)
				goto Lexit_b;

			g_n1008fea0 = pObj->m_nNumIndices1e0;
		}

		// -- 0x1005b2e0: every vertex into camera space --------------- //
		sub_1005a550(pVerts, nVerts, &pCtx->m_m15c);

		nTrisPerRow = pObj->m_nWidth1f0 * 2 - 2;
		pOutIdx     = g_p1008fe64;
		nIdxPerRow  = nTrisPerRow * 3;
		pRowIdx     = pObj->m_pIndices1ac;

		for (y = pObj->m_nHeight1f4 - 1; y; y--)
		{
			pIdx = pRowIdx;

			for (t = nTrisPerRow; t; t--)
			{
				// -- 0x1005b38d: trivial accept / reject, and a reduced
				//    outcode mask for whatever is left.
				nResult = sub_1005d650(nClipMask, &nOutMask,
				                       &pVerts[pIdx[0]],
				                       &pVerts[pIdx[1]],
				                       &pVerts[pIdx[2]]);

				if (nResult == 0)
				{
					// 0x1005d0b0, sunk: wholly inside, keep the indices.
					pOutIdx[0] = pIdx[0];
					pOutIdx[1] = pIdx[1];
					pOutIdx[2] = pIdx[2];
					pOutIdx    = pOutIdx + 3;
				}
				else if (nResult != 1)
				{
					// -- 0x1005b3a6 ------------------------------------ //
					aTri[0] = pVerts[pIdx[0]];
					aTri[1] = pVerts[pIdx[1]];
					aTri[2] = pVerts[pIdx[2]];

					pVerts2 = aTri;
					nVerts2 = 3;

					// 0x1000aa90 inlined (ClipPolyA above).
					if (!ClipPolyA(nOutMask, (void**)&pVerts2, (uint32*)&nVerts2))
						goto Lnexttri;

Ldraw:
					// -- 0x1005ca38: camera space -> screen, with the six
					//    globals at 0x1008bb9c..0x1008bbb0.
					pV = pVerts2;

					for (i = nVerts2; i; i--)
					{

						pV->rhw = 1.0f / pV->z;

						pV->x = (pV->x * g_f1008bb9c +
						         pV->z * g_f1008bba0) * pV->rhw;
						pV->y = (pV->y * g_f1008bba4 +
						         pV->z * g_f1008bba8) * pV->rhw;
						pV->z = (pV->z * g_f1008bbac +
						         g_f1008bbb0) * pV->rhw;

						pV++;
					}

					// -- 0x1005cab7: 6 = D3DPT_TRIANGLEFAN, FVF 0x2c4 -- //
					g_pD3DDevice->lpVtbl->DrawPrimitive(g_pD3DDevice, 6,
					                                    0x2c4, pVerts2,
					                                    nVerts2, 0);
				}

Lnexttri:
				pIdx = pIdx + 3;
			}

			pRowIdx = pRowIdx + nIdxPerRow;
		}

		// -- 0x1005cb33: whatever survived whole, in one indexed draw -- //
		nIndices = (int32)(pOutIdx - g_p1008fe64);

		if (nIndices > 0)
		{
			pV = pVerts;

			for (i = nVerts; i; i--)
			{

				fPw = 1.0f / (g_m1008ba1c.m[3][0] * pV->x +
				              g_m1008ba1c.m[3][1] * pV->y +
				              g_m1008ba1c.m[3][2] * pV->z +
				              g_m1008ba1c.m[3][3]);

				fPx = (g_m1008ba1c.m[0][0] * pV->x +
				       g_m1008ba1c.m[0][1] * pV->y +
				       g_m1008ba1c.m[0][2] * pV->z +
				       g_m1008ba1c.m[0][3]) * fPw;
				fPy = (g_m1008ba1c.m[1][0] * pV->x +
				       g_m1008ba1c.m[1][1] * pV->y +
				       g_m1008ba1c.m[1][2] * pV->z +
				       g_m1008ba1c.m[1][3]) * fPw;
				fPz = (g_m1008ba1c.m[2][0] * pV->x +
				       g_m1008ba1c.m[2][1] * pV->y +
				       g_m1008ba1c.m[2][2] * pV->z +
				       g_m1008ba1c.m[2][3]) * fPw;

				pV->x   = fPx;
				pV->y   = fPy;
				pV->z   = fPz;
				pV->rhw = fPw;

				pV++;
			}

			// -- 0x1005cc2b: 4 = D3DPT_TRIANGLELIST ------------------- //
			g_pD3DDevice->lpVtbl->DrawIndexedPrimitive(g_pD3DDevice, 4,
			                                           0x2c4, pVerts,
			                                           nVerts,
			                                           g_p1008fe64,
			                                           (uint32)(pOutIdx -
                                                    g_p1008fe64),
			                                           0);
		}
	}
	else
	{
		// -------------------------------------------------------------- //
		//  0x1005cc73.  Nothing is clipped, so the whole grid goes in one
		//  indexed draw through the object's own index buffer.
		// -------------------------------------------------------------- //
		mFull = pCtx->m_m19c.sub_10015800(pCtx->m_m15c);

		pV = pVerts;

		for (i = nVerts; i; i--)
		{

			fPw = 1.0f / (mFull.m[3][0] * pV->x +
			              mFull.m[3][1] * pV->y +
			              mFull.m[3][2] * pV->z +
			              mFull.m[3][3]);

			fPx = (mFull.m[0][0] * pV->x +
			       mFull.m[0][1] * pV->y +
			       mFull.m[0][2] * pV->z +
			       mFull.m[0][3]) * fPw;
			fPy = (mFull.m[1][0] * pV->x +
			       mFull.m[1][1] * pV->y +
			       mFull.m[1][2] * pV->z +
			       mFull.m[1][3]) * fPw;
			fPz = (mFull.m[2][0] * pV->x +
			       mFull.m[2][1] * pV->y +
			       mFull.m[2][2] * pV->z +
			       mFull.m[2][3]) * fPw;

			pV->x   = fPx;
			pV->y   = fPy;
			pV->z   = fPz;
			pV->rhw = fPw;

			pV++;
		}

		// -- 0x1005ce2f ------------------------------------------------ //
		if (pObj->m_nNumIndices1e0)
		{
			g_pD3DDevice->lpVtbl->DrawIndexedPrimitive(g_pD3DDevice, 4,
			                                           0x2c4, pVerts,
			                                           nVerts,
			                                           pObj->m_pIndices1ac,
			                                           pObj->m_nNumIndices1e0,
			                                           0);
		}
	}

	// -- 0x1005ce7e ---------------------------------------------------- //
	if (bDetail)
		sub_10011500();

	// -- 0x1005ce8d: the four savers unwound in reverse order, INLINE.
	//    See the COIN-FLIP note in the banner.
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
	return;

	// ------------------------------------------------------------------ //
	//  The four early returns, written out here in the SHIPPED ADDRESS
	//  ORDER (0x1005cf30, 0x1005cf6b, 0x1005d0f0, 0x1005d135) so that the
	//  sixteen `call 0x1005a600` land in the shipped sequence.
	// ------------------------------------------------------------------ //
Lexit_a:
	// 0x1005cf30 -- the bounding sphere missed the frustum.
	sFogColor.sub_1005a600();
	sFogEnable.sub_1005a600();
	sDestBlend.sub_1005a600();
	sSrcBlend.sub_1005a600();
	return;

Lexit_b:
	// 0x1005cf6b -- the index scratch could not be grown.
	g_n1008fea0 = 0;
	sFogColor.sub_1005a600();
	sFogEnable.sub_1005a600();
	sDestBlend.sub_1005a600();
	sSrcBlend.sub_1005a600();
	return;

Lexit_c:
	// 0x1005d0f0 -- the vertex cache could not be grown.
	g_n1008fe9c = 0;
	sFogColor.sub_1005a600();
	sFogEnable.sub_1005a600();
	sDestBlend.sub_1005a600();
	sSrcBlend.sub_1005a600();
	return;

Lexit_d:
	// 0x1005d135 -- no heightfield.
	sFogColor.sub_1005a600();
	sFogEnable.sub_1005a600();
	sDestBlend.sub_1005a600();
	sSrcBlend.sub_1005a600();
}


static void sub_1005d170(W7Obj5d170* pObj, W7Vert5d170* pVert,
                             float fU, float fV, uint8 nHeight,
                             LTVec* pRow, LTVec* pUp, int32 bDetail)
{
	int32 n;
	W7Colour5d170* pCol = &pObj->m_Colours1f8[nHeight];

	pVert->m_Pos = pRow->sub_100155f0(pUp->sub_10015690((float)nHeight));

	n = (int32)pCol->r;
	pVert->m_12 = g_ColorRamp2[(uint8)n];
	n = (int32)pCol->g;
	pVert->m_11 = g_ColorRamp1[(uint8)n];
	n = (int32)pCol->b;
	pVert->m_10 = g_ColorRamp0[(uint8)n];
	n = (int32)(pCol->a * g_f1008fe98);
	pVert->m_13 = (uint8)n;

	if (bDetail)
	{
		fV = (fV + g_f1008fe8c) * g_f1008fe94;
		fU = (fU + g_f1008fe88) * g_f1008fe90;
		pVert->m_fU20 = fU;
		pVert->m_fV24 = fV;
	}
	else
	{
		fV = (fV + g_f1008fe8c) * g_f1008fe94;
		fU = (fU + g_f1008fe88) * g_f1008fe90;
		pVert->m_fU18 = fU;
		pVert->m_fV1c = fV;
	}
}

static void sub_1005d3e0(W7Obj5d170* pObj, W7Vert5d170* pVert,
                             float fU, float fV, int8 nHeight,
                             LTVec* pRow, LTVec* pUp, int32 bDetail)
{
	int32 n;
	W7Colour5d170* pCol = &pObj->m_Colours1f8[nHeight + 128];

	pVert->m_Pos = pRow->sub_100155f0(pUp->sub_10015690((float)nHeight));

	n = (int32)pCol->r;
	pVert->m_12 = g_ColorRamp2[(uint8)n];
	n = (int32)pCol->g;
	pVert->m_11 = g_ColorRamp1[(uint8)n];
	n = (int32)pCol->b;
	pVert->m_10 = g_ColorRamp0[(uint8)n];
	n = (int32)(pCol->a * g_f1008fe98);
	pVert->m_13 = (uint8)n;
	pVert->m_nSpecular14 = 0xffffffff;

	if (bDetail)
	{
		fV = (fV + g_f1008fe8c) * g_f1008fe94;
		fU = (fU + g_f1008fe88) * g_f1008fe90;
		pVert->m_fU20 = fU;
		pVert->m_fV24 = fV;
	}
	else
	{
		fV = (fV + g_f1008fe8c) * g_f1008fe94;
		fU = (fU + g_f1008fe88) * g_f1008fe90;
		pVert->m_fU18 = fU;
		pVert->m_fV1c = fV;
	}
}
