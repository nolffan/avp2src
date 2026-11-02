// *********************************************************************** //
//
//  MODULE:   W53_10070D10_OS (.CPP)  -- was BIG_70D10 (.CPP)
//
//  Attempted -> NEAR, in two steps.
//    1. The thirty byte lerps go through RoundF (the w1_10001000.cpp
//       fld/fistp helper, as big6d910.cpp's twin 0x1006d910 already
//       does): each inlined copy keeps its float argument and its int
//       result in frame slots of their own -- the shipped `fstps t ;
//       flds t ; fistpl n` -- and that is exactly the 0x158 bytes the
//       frame was short.  Frame 0xfb0 -> 0x1108 (the shipped `mov
//       $0x1108,%eax ; call _chkstk` entry), 7683 -> 6962 bytes against
//       7331, the first 21 bytes identical: it pairs.
//    2. The clip path is 0x1000a8a0's body INLINE -- XFormClip70d10 over
//       the inline 0x1000f4b0 driver and the six inline plane bodies,
//       w23_100525b0_os.cpp's set on this file's vertex -- taking
//       &pVerts / &nVerts.  With the old CLIP70D10_HEAD/TAIL macros icl
//       knew pVerts was still aVerts after the lighting call and indexed
//       the array off %ebp; the shipped code re-reads both from their
//       slots (0x10070e19, 0x10070f1c), which is what an address-taken
//       pair gives.  Normalised instruction match 0.301 -> 0.843 (the
//       six plane blocks now line up 1:1), 6962 -> 7174 bytes; the
//       positional count moves 6111 -> 6295 because the lengths now
//       differ in different places; pairing prefix 21 -> 18.
//  What is left: the shipped vertex fill keeps pDest / pSrc / the count in
//  frame slots (-0x78 / -0x68 / -0xb4, shared with the driver's pVerts and
//  nVerts) and the source position pointer in %edi; ours walks registers.
//  The fill as an inline taking (pDest, pSrc, nCount) breaks the prefix
//  (pairing prefix 4, 0.837).  In the plane bodies the shipped code keeps
//  pEnd / pFlag / nInside in frame slots (-0x88 / -0x80 / -0x90); ours
//  keeps them in registers.  `pV = &(*ppVerts)[0]` in the planes: 0.841.
//
//  Renamed /Os.  Measured on this file: /O2 (and /O2 /Oi-)
//  9,296 bytes against the shipped 7,331; /Os (and /Os /Oi-, /Os /Qip,
//  /Os /Qip /Qip_no_inlining) 7,683.  The verdict below weighed /O2's
//  cold-block sinking against the pushed saves and 0x90 padding; the
//  twin big6d910.cpp already measured to /Os /Qip /Qip_no_inlining.  The
//  frame is still 0xfb0 against the shipped 0x1108 (/O2 gives 0x1190).
//
//  PURPOSE:  One function -- 0x10070d10, 7,331 bytes.  ONE caller,
//            0x10072c9d, which publishes the item's clip mask into
//            0x1008b874 first (`mov 0x10(%ebx),%eax ;
//            mov %eax,0x1008b874` at 0x10072c8e/0x10072c94) and then
//            recycles the item onto the free list at 0x1008d4fc -- the
//            same caller shape big6d910.cpp records for 0x1006d910.
//
//            It is the SAME SOURCE FUNCTION as big6d910.cpp's 0x1006d910
//            with three localised differences (a lit/ramped vertex fill
//            instead of a white one, a generated second texture pair, and
//            a different geometry pre-pass).  Everything between
//            0x10070e0c and 0x10072641 is that file's body, and this
//            decode is a DIFF against it rather than an independent
//            reading.  Where a region is quoted as "identical to
//            big6d910" the two disassemblies were compared instruction by
//            instruction, not skimmed.
//
//  ------------------------------------------------------------------- //
//  THE EXTENT.  Verified before a byte was decoded.
//  ------------------------------------------------------------------- //
//
//  The stated range 0x10070d10..0x100729b3 is CORRECT.
//
//    * ONE `ret` in the whole range, at 0x10072641, and ONE
//      `push %ebp ; mov %esp,%ebp`, at 0x10070d10.  No second prologue,
//      no second epilogue.
//    * The single `ret` has **882 bytes of code after it** and every one
//      of those bytes is /O2 cold-block sinking, not a second function.
//      Proved target by target: the eleven blocks below the `ret` are
//      0x10072642 (L_Tex, reached by `je 0x10072642` from 0x10070e2d,
//      `jmp 0x10072642` from 0x10070f17 and 0x10072637, and
//      `je 0x10072642` from 0x10072619 -- all four ABOVE the ret),
//      0x100727b9 (L_Draw), 0x10072838 / 0x1007285e / 0x10072886 /
//      0x100728ae / 0x100728d6 / 0x100728fb (the six clip-epsilon arms,
//      each jumping BACK to 0x100710cd / 0x100714a0 / 0x10071875 /
//      0x10071c3e / 0x1007200f / 0x100723ae), 0x1007291e (L_ElseLOD),
//      0x10072954 / 0x1007295e (the two record-search `break`
//      trampolines, both `jmp 0x100726a9`) and 0x10072968 (L_MakeRec,
//      `jmp 0x100726c3`).  Every block uses THIS frame (0x8(%ebp),
//      -0x604(%ebp), -0x98(%ebp), ...) and every one ends by branching
//      back into 0x10070d10..0x10072641.
//    * EVERY branch target in the range is inside the range.  A pass over
//      all 100-odd jump/branch targets in the disassembly found nothing
//      below 0x10070d10 and nothing above 0x100729ae.
//    * 0x100729b3..0x100729bf is thirteen `nop` bytes and 0x100729c0 is
//      `push %ebp ; mov %esp,%ebp ; sub $0x38,%esp` -- the next function.
//
//  ------------------------------------------------------------------- //
//  WHAT IT IS
//  ------------------------------------------------------------------- //
//
//  d3d.ren's DRAW-ONE-LIT-POLYGON pass.  The 40-byte/two-stage vertex,
//  the six-plane clipper and the whole texture-bind + DrawPrimitive tail
//  are big6d910.cpp's; what is different is the front:
//
//    * the vertex COLOUR is not white.  0x10070d60..0x10070d8f is
//      0x100598a0's body INLINED -- three adjacent source bytes at +0x16,
//      +0x15, +0x14 through three 256-entry ramps at 0x1008d1bc,
//      0x1008d2bc, 0x1008d3bc into the destination's +0x12, +0x11, +0x10,
//      then the alpha byte from 0x1008be60 into +0x13.  d3dhigh.cpp
//      decoded that helper (0x100598a0) and records exactly this order,
//      highest destination byte first; big2ef34.cpp records the same
//      +0x13 <- 0x1008be60 store in its own fill.  There is no
//      `call 0x100598a0` anywhere in the shipped 7.3 K.
//    * the SECOND texture pair is GENERATED, not copied.  +0x20 and +0x24
//      are `(pos->x + g_f1008ad9c) * g_f1008ad94` and
//      `(pos->z + g_f1008ada0) * g_f1008ad98` -- a world-space planar
//      projection off x and z with the y axis dropped, which is a
//      ground-plane detail/lightmap coordinate.  big6d910 copies its
//      +0x20/+0x24 straight out of the source vertex; this one computes
//      them.
//    * the geometry pre-pass is `sub_10064800(pObj, pVerts, nVerts)`
//      (0x10070e04) where big6d910 has 0x1000a740 + 0x1006d8e0 +
//      0x10039d70.  0x10064800 is a per-vertex lighting routine -- it
//      reads pObj->+0x2c and, through pObj->+0x28, +0x34; three light
//      colour bytes at +0x94/+0x95/+0x96; a radius at +0x1a8; and it
//      bumps the counter 0x1008be48.  It branches as far as 0x10064b3c,
//      so it is at least 0x33c bytes.  It is NOT decoded here and does
//      not need to be -- only its argument list matters, and that is
//      read off the three outgoing slots at
//      0x10070df9/0x10070dfd/0x10070e01.
//    * there is NO overflow guard and NO `pVerts != aVerts` rebuild here:
//      nothing can move the array, so the 64-vertex bound is enforced
//      only by the frame.
//    * DrawPrimitive is vtable +0x64 with primitive type **6**
//      (D3DPT_TRIANGLEFAN) and FVF **0x2c4** = XYZRHW | DIFFUSE |
//      SPECULAR | TEX2 (0x100727fc/0x10072807), and every vertex stride
//      in the function is 0x28 = 40 bytes, which is exactly that FVF.
//    * unlike big6d910 there is NO `g_n1008be5c++` after the
//      DrawPrimitive.  See the census: only SEVEN `incl` in the whole
//      function and all seven are accounted for.
//
//  ------------------------------------------------------------------- //
//  COMPILER VERDICT: Intel C/C++ 4.0, **/O2**.  Same split as big6d910.
//  ------------------------------------------------------------------- //
//
//  FOR /O2:
//    * COLD-BLOCK SINKING BELOW THE EPILOGUE, eleven-fold -- the 882-byte
//      tail enumerated under THE EXTENT above.
//    * OUTGOING ARGUMENTS as `add $-N,%esp` then `mov %reg,K(%esp)` at
//      0x10070d4b, 0x10070df0, 0x1007272c, 0x10072759, 0x1007279a,
//      0x100727f4 and 0x1007292a.
//    * `&p->m` as `mov $K,%reg ; add %p,%reg` -- 0x10071074, 0x100712de,
//      0x10071440, 0x10071812, 0x10071bdd, 0x10071fac, 0x10072357 all do
//      `mov $0x28,%eax ; add %edi,%eax` (or the %ecx twin) for `pOut++`,
//      never `lea 0x28(%edi),%eax`.
//
//  AGAINST (recorded, not explained away):
//    * ONE epilogue.  `return 1` at 0x10072830 is
//      `xor %eax,%eax ; inc %eax ; jmp 0x1007263b`, a cross-jump INTO the
//      middle of the `return 0` epilogue at 0x10072639.  /O2 usually
//      DUPLICATES, and it uses the /Os `xor ; inc` idiom for the 1.
//    * six outgoing arguments ARE pushed (0x10070f3e, 0x10070f3f,
//      0x10072627, 0x10072628, and the four-push groups at
//      0x1007296c..0x10072973 and 0x10072994..0x1007299b).  Exactly as in
//      big6d910: the `add/mov` form appears where a slot has to be
//      computed, the `push` form where the value is already in a register
//      or an immediate.  Both forms in one function is unexplained there
//      and unexplained here.
//
//  There is only one function in the file, so it wants ONE flag.  Try
//  /O2 first, /Os second.
//
//  Outgoing argument areas are again always a MULTIPLE OF 8: 0x10064800
//  takes three arguments and gets four slots (`add $-0x10,%esp` ...
//  `add $0x10,%esp`), 0x10039bd0 takes three and gets four pushes,
//  0x1000d9b0 and 0x1000d940 take two and get two.  That is icl's stack
//  alignment, not arguments.
//
//  ------------------------------------------------------------------- //
//  THE ACCEPTANCE FIGURE AND WHAT IT DOES *NOT* WITNESS.  READ FIRST.
//  ------------------------------------------------------------------- //
//
//  The call count over the range: 7,331 bytes, **12 calls (7 direct, 5
//  indirect), 611 bytes per call**, across **five distinct direct
//  callees** (0x100805a0, 0x1000d9d0, 0x10064800, 0x1000d9b0,
//  0x1000d940, 0x10039bd0 -- six symbols, of which 0x10039bd0 is called
//  twice).  Anything over 300 B/call is in the weak band; at
//  611 the sequence agreeing is worth almost nothing on its own and this
//  banner does not lean on it.
//
//  * **CALL-FREE COVERAGE: 5,860 of 7,331 bytes = 79.9%.**  The last call
//    before the gap is 0x1000d9b0 at 0x10070f40 and the next is
//    0x1000d940 at 0x10072629; 0x10070f45..0x10072628 has ZERO calls in
//    it.  A second, smaller call-free stretch is 0x10070e09..0x10070f3f
//    (311 bytes, the whole inline projective transform).  Together
//    **6,171 of 7,331 bytes -- 84.2% -- are invisible to the call-sequence comparison.**
//
//  What pins that 84% instead is an INSTRUCTION CENSUS that leaves no
//  room for anything but the structure claimed.  Every count below is
//  from `sed 's/.*\t//' dump.txt | awk '{print $1}' | sort | uniq -c`
//  over the whole 7,331 bytes, and every one is EXACT -- there is no
//  residue anywhere:
//
//    | mnemonic | shipped | what the structure below requires |
//    |----------|---------|-----------------------------------|
//    | fmull    |  66     | 6 passes x 11 uses of fT (2 position lerps + 4 texture lerps + 5 byte lerps) |
//    | fildl    |  60     | 6 x 5 byte-lerps x 2 (the difference and the base) |
//    | fistpl   |  30     | 6 x 5 byte-lerps x 1 |
//    | fstpl    |  12     | 6 x `fT = num/den` + 6 x `fT = 0.0f` in the sunk arms |
//    | fcomp    |  12     | 6 x `fDen < -1e-5` + 6 x `fDen > 1e-5` |
//    | fcomps   |   6     | the six classify loops, one per plane |
//    | fdivp    |   6     | one divide per clip pass |
//    | idiv     |   6     | six x `nIn = (pOut - pOutStart) / 40` |
//    | cltd     |   6     | the sign extension for those six |
//    | rep movsl|   7     | 6 x `*pOut = *pPrev` ($0xa = 40 bytes) + 1 x `pWalk->m_vPos = vScreen` ($0x3) |
//    | fsubs    |  40     | 6 x 6 lerp differences + 4 fDen tails (passes 2,3,4,5) |
//    | fadds    |  46     | 6 x 6 lerp bases + 4 fDen tails + 4 in the inline transform + 2 in the fill |
//    | faddp    |  40     | 30 byte-lerp adds + 8 in the inline transform + 2 numerators (passes 2,5) |
//    | fstps    |  78     | 68 in the six passes + 4 in the inline transform + 2 in the fill + 4 in the rotated scale loop |
//    | fmuls    |  16     | 12 in the inline transform + 2 in the fill + 2 in the scale loop |
//    | fmul/fmulp| 2+1    | the three `* fOOW` in the inline transform |
//    | fsubrs   |   2     | `g_f1008b908 - pPrev->z` and `g_f1008b910 - pPrev->z` |
//    | fsubrp   |   2     | passes 3 and 4's `pPrev->y - pPrev->z` / `pPrev->x - pPrev->z` (DE E8+i is Intel FSUBP) |
//    | fsub     |   6     | the six `pWalk->? - pPrev->?` that open each fDen |
//    | fchs     |   8     | 2 classify negations (passes 2,5) + 4 numerator negations (2,3,4,5) + 2 result negations (2,5) |
//    | fxch     |  12     | 8 in the six passes + 3 in the inline transform + 1 in the scale loop |
//    | fsts     |   2     | pass 3's `y = fVal` and pass 4's `x = fVal` |
//    | fld      |   2     | passes 2 and 5 duplicating fVal before negating it |
//    | fstl/fldl|  1+1    | the fOOW double spill/reload in the inline transform |
//    | fdivrs   |   1     | `1.0f / w` (0x100ab7c0 dumped: 0x3f800000) |
//    | incl     |   7     | 6 x `g_n1008be44++` (one per pass) + 1 x `g_n1008bd80++` |
//    | movzbl   |  65     | 60 byte-lerp widenings + 3 ramp indices + 2 uint8 casts in the texture block |
//    | movzwl   |  14     | 2 x `m_nCount50` + g_w1008b870 + 2 x the unrolled `m_nStage42` search + 4 more `m_nStage42` + 2 x `m_nStamp14` + g_w1008bea8 + m_nAlphaRef16 + 2 x `m_nDirty44` |
//    | jae      |   1     | pass 1's classify (`z >= near`) |
//    | jb       |  10     | 4 classify (passes 2,3,4,5) + 6 sunk-arm second tests |
//    | jbe       |  8     | 6 sunk-arm first tests + pass 6's classify (`z <= far`) + the fill loop's guard |
//    | jl / jle  |  6 / 6 | the six edge loops' `i < nIn` and the six `nInside != nIn` guards |
//    | and       |  1     | `nClip &= 1` under UseD3DClip |
//    | ret       |  1     | the single epilogue |
//
//    66 = 6 x 11 and 60 = 6 x 5 x 2 and 30 = 6 x 5 are the load-bearing
//    rows: **six clip arms, each with two position LERPs, four texture
//    LERPs and five byte LERPs, and nothing else.**  A seventh plane, a
//    missing plane, a third position LERP anywhere or a sixth byte would
//    all move at least three of those counts, and none of them moves.
//
//  * **FRAME ARITHMETIC.  `mov $0x1108,%eax ; call 0x100805a0` is
//    _chkstk, so the frame is 4,360 bytes, and it decomposes EXACTLY
//    into the locals declared below -- no slack anywhere except 24 bytes
//    of alignment at the bottom:**
//
//      -0x1108 .. -0x10f1   24  alignment (nothing addresses it)
//      -0x10f0 .. -0x1005  236  THIRTY 8-byte-aligned float temps, used
//                               `fstps -0x10f0(%ebp) ; flds -0x10f0(%ebp)
//                               ; fistpl -0x528(%ebp)`.  6 passes x 5
//                               byte-lerps, stepping by 8 from -0x10f0 to
//                               -0x1008; the last one is 4 bytes because
//                               aVerts begins at -0x1004.
//      -0x1004 .. -0x0605 2560  aVerts -- 0xa00 / 0x28 = **64** vertices.
//                               The bound is read off the frame, not
//                               guessed, and there is no overflow-guard
//                               callee here to cross-check it against.
//      -0x0604              4   pVerts
//      -0x0600              4   nVerts
//      -0x05fc .. -0x05f1  12   vScreen, the three-float local the
//                               `rep movsl $0x3` at 0x10070eff copies
//      -0x05f0 .. -0x00c9 1320  SIX x 0xdc = the six clip passes' state:
//                               0xc8 = 200 bytes = **50 int32** of flags
//                               plus 0x14 = five int slots.  The arrays
//                               are -0x5f0, -0x514, -0x438, -0x35c,
//                               -0x280, -0x1a4 (exactly 0xdc apart, all
//                               six indexed as `-0xNNN(%ebp,%eax,4)`) and
//                               pass 1's five int slots are -0x528,
//                               -0x524, -0x520, -0x51c, -0x518, which is
//                               precisely -0x5f0 + 0xc8 onwards.  Same
//                               finding as big5a960.cpp and big6d910.cpp.
//      -0x00c8, -0x00b4 .. -0x00a4   the integer scratch
//      -0x00a0, -0x98, -0x90, -0x88, -0x80, -0x78   the six 8-byte fT
//      -0x0070              8   the fOOW double spill
//      -0x0068              4   icl scratch
//      -0x0008              4   the pRec spill at 0x1007293d in L_ElseLOD
//
//    The 0xdc x 6 block landing exactly between vScreen and the fT
//    doubles, and the 30 x 8 temp block landing exactly against aVerts,
//    is the strongest single piece of evidence in this file: it is
//    independent of the call sequence and it pins BOTH the clip-array
//    bound (50) and the vertex-array bound (64).
//
//  * **CORRESPONDENCE WITH big6d910.cpp, EXPRESSION BY EXPRESSION.**  The
//    six clip passes were diffed against that file's, not re-derived.
//    Everything agreed:
//      - the six mask bits are **0x01, 0x04, 0x08, 0x10, 0x20, 0x02 IN
//        THAT ORDER** (0x10070f8c `test $0x1,%cl`, 0x1007134d `test
//        $0x4,%al`, 0x10071721 `test $0x8,%al`, 0x10071aed `test
//        $0x10,%al`, 0x10071eb9 `test $0x20,%al`, 0x10072269 `test
//        $0x2,%al`);
//      - the six classify tests are
//            z >= g_f1008b908 | -z < x | y < z | x < z | -z < y
//            | z <= g_f1008b910
//        read off 0x10070fdf (fcomps 0x1008b908 / jae), 0x1007139f
//        (fchs ; fcomps (%edx) / jb), 0x10071773 (flds 0x4 ; fcomps 0x8 /
//        jb), 0x10071b3e (flds (%edx) ; fcomps 0x8 / jb), 0x10071f0b
//        (fchs ; fcomps 0x4 / jb), 0x100722bb (fcomps 0x1008b910 / jbe);
//      - the epsilon triple is the same and the constants were DUMPED:
//        0x100ab7c4/cc/d4/dc/e4/ec are all -9.999999747e-06,
//        0x100ab7f4/fc/804/80c/814/81c are all +9.999999747e-06 and
//        0x100ab7f8/800/808/810/818/820 are all 0.0f.  The sunk arms read
//        them in reverse pass order (pass 1 uses the highest pair);
//      - the six `pIn = pOutStart ; nIn = (pOut-pOutStart)/40 ;
//        pOut += nIn` tails are all visible as `cltd ; idiv $0x28` at
//        0x10071336, 0x1007170a, 0x10071ad6, 0x10071ea2, 0x10072252,
//        0x100725e8;
//      - the FIVE BYTE LERPS are +0x12, +0x11, +0x10, +0x13, +0x17 in
//        that order in ALL SIX passes -- thirty sites, all read;
//      - the FOUR TEXTURE LERPS are +0x18, +0x1c, +0x20, +0x24 in that
//        order in all six passes -- twenty-four sites, all read.
//    This is the SIXTH independent reading of this one clipper source
//    (d3dr3a.cpp, big5a960.cpp, big61a30.cpp, big124a4.cpp, big6d910.cpp
//    and here); it is reused, not re-derived.
//
//  * **THE INLINE PROJECTIVE TRANSFORM, 0x10070e19..0x10070f17 (310
//    bytes, no calls).**  Pinned outright: it reads sixteen floats at
//    0x1008ba5c..0x1008ba98, and 0x1008ba5c = **0x1008b880 + 0x1dc**, so
//    the block is the +0x1dc..+0x218 field set of the transform object
//    0x1008b880 -- the SAME sixteen fields in the same operand order that
//    big124a4.cpp decoded at 0x10012a65, that big6d910.cpp decoded at
//    0x1006da68 and that are the body of 0x1000a8a0 at 0x1000a8d0.
//    `fdivrs 0x100ab7c0` is `1.0f /` (dumped: 0x3f800000).  Written in
//    big124a4.cpp's / big6d910.cpp's exact spelling.
//
//  * **THE TEXTURE-BIND BLOCK, 0x10072642..0x100727b9, and the two sunk
//    arms 0x1007291e / 0x10072968.**  big61a30.cpp's and big6d910.cpp's
//    block field for field: g_a1008e684[stage], the LRU move-to-front of
//    the +0x1c LTLink through 0x1008e634, `g_1008cbcc->+0x48 +=`,
//    SetTexture at +0x8c, ALPHAREF (state 0x18) at +0x50, the pair
//    published into g_a1008e664[stage], `g_n1008bd80++`, SetLOD at +0xbc,
//    and the two sub_10039bd0 create arms with the third argument 1 when
//    the list is non-empty and 0 when it is empty.  Only three calls in
//    170 bytes, but every offset is a repeat of an already-decoded block.
//
//  * **THE TEXTURE-SCALE LOOP, 0x100727b9..0x100727f4 (59 bytes, no
//    calls).**  Pinned by the two absolute addresses 0x1008e664 and
//    0x1008e668, which are element ZERO of the per-stage pair that
//    0x10072785 writes with an index -- the same fold big61a30.cpp
//    records at 0x10061f29/0x10061f35 and big6d910.cpp at 0x1006f2ff.
//
//  * **THE FILL LOOP, 0x10070d49..0x10070ddd (148 bytes, one call).**
//    Witnessed by the 0x1000d9d0 inside it.  What pins the rest: the
//    three ramp bases 0x1008d1bc / 0x1008d2bc / 0x1008d3bc are exactly
//    0x100 apart and d3dhigh.cpp already decoded the out-of-line version
//    of this exact four-statement block (0x100598a0), destination byte
//    order and all; the strides are read directly (`add $0x18,%esi`,
//    `add $0x28,%ebx`); the source's +0x00 is a POINTER, dereferenced for
//    three floats at 0x10070d4e/0x10070d54/0x10070d5a and again for the
//    generated texture pair at 0x10070da1/0x10070dbb.
//
//  * **NO `// UNDECODED:` GAPS.**  Every byte of
//    0x10070d10..0x100729b2 is accounted for by a region below, and the
//    census above has no residue.
//
//  * **THE EXPECTED CALL-SEQUENCE RESULT, predicted from the disassembly
//    before any build.**  The shipped sequence, in address order, is
//
//      0x100805a0 (_chkstk), 0x1000d9d0, 0x10064800, 0x1000d9b0,
//      0x1000d940, *0x8c, *0x50, *0xbc, *0x64, *0xbc, 0x10039bd0,
//      0x10039bd0
//
//    -- 12 calls, 7 direct and 5 indirect.  The source below emits
//    exactly that order, with ONE expected difference: icl generates its
//    own `__chkstk` for the 4,360-byte frame and the comparison cannot resolve
//    that symbol to 0x100805a0, which big124a4.cpp and big6d910.cpp both
//    already recorded for the same reason.  So the expected result is
//    **11 of 12 agreeing, with one by-position-only or one shipped-only
//    row, and that row is _chkstk**.  Anything else is a decode error or
//    a block-placement difference; if it is the latter it will show as
//    equal counts on both sides with the odd rows all being the
//    hand-placed cold blocks named in coin-flip 3.
//
//  ------------------------------------------------------------------- //
//  REGION MAP
//  ------------------------------------------------------------------- //
//
//  0x10070d10..0x10070d45  PROLOGUE.  `mov $0x1108,%eax ;
//      call 0x100805a0` is _chkstk (frame 4,360); the four pushes that
//      follow are icl's dead slot plus %edi/%esi/%ebx (three `pop`s at
//      0x1007263b..0x1007263d against four pushes, `mov %ebp,%esp`
//      covering the difference).  aVerts and pObj->m_aSrc54 go to their
//      frame slots and the count is tested.
//
//  0x10070d49..0x10070ddd  THE FILL.  aVerts[i] built from
//      pObj->m_aSrc54[i]: three floats through the source's +0x00
//      POINTER, the three colour bytes through the three ramps, the alpha
//      byte from g_1008be60, the first texture pair through the thiscall
//      setter 0x1000d9d0, and the second pair GENERATED from world x and
//      z.
//
//  0x10070ddd..0x10070e0c  `pVerts = aVerts ; nVerts = pObj->m_nCount50`
//      then `sub_10064800(pObj, pVerts, nVerts)` -- the lighting pass.
//      No return value is tested (`add $0x10,%esp` then straight on).
//
//  0x10070e0c..0x10070f1c  `if (!g_n1008b874)` THE UNCLIPPED PATH: one
//      projective transform per vertex through 0x1008b880's
//      +0x1dc..+0x218, the three results assembled in a named three-float
//      local (`rep movsl $0x3` at 0x10070eff) and 1/w into +0x0c.  Then
//      straight to L_Tex.
//
//  0x10070f1c..0x10070f56  THE CLIPPED PATH's pre-transform: nVerts calls
//      to 0x1000d9b0(pVert, g_1008b880), then g_1008b874 is RE-READ
//      (0x10070f4e).  0x1000a8a0 re-reads it in exactly the same place
//      (0x1000a9e1) before handing it to its clipper, which is why the
//      second read is in the source and not a missed CSE; note that the
//      nVerts==0 path (`je 0x10070f53`) reaches the join with the FIRST
//      load still live in %eax, which is icl CSEing the two reads across
//      a call-free path.
//
//  0x10070f56..0x10070f6c  UseD3DClip: `if (g_p1008d7a0) { nClip &= 1;
//      if (!nClip) goto L_Project; }` -- big5a960.cpp's test.  The `goto`
//      skips the `pVerts = pIn ; nVerts = nIn` epilogue at 0x100725f9
//      exactly as big124a4.cpp's early `goto L_Draw` does.
//
//  0x10070f6c..0x100725f9  THE SIX CLIP PASSES.  See the accounting
//      above.  Heads at 0x10070f9e, 0x10071361, 0x10071735, 0x10071b01,
//      0x10071ecd, 0x1007227d, each opening with `incl 0x1008be44`.
//
//  0x100725f9..0x10072605  `pVerts = pIn ; nVerts = nIn`.
//
//  0x10072605..0x10072639  L_Project: nVerts calls to
//      0x1000d940(pVert, g_1008b880), then `jmp L_Tex`.
//
//  0x10072639..0x10072642  THE EPILOGUE -- the only one.  Entered at
//      0x10072639 for `return 0` and at 0x1007263b for `return 1`.
//
//  0x10072642..0x100726c3  L_Tex, SUNK past the epilogue: read the stage,
//      stamp the block, walk the record list (icl unrolled the walk x2 at
//      0x10072681/0x10072694), and dispatch to L_MakeRec / L_ElseLOD /
//      L_Bind.
//
//  0x100726c3..0x100727b9  L_Bind: the LRU move-to-front, SetTexture,
//      ALPHAREF, the published scale pair, `g_n1008bd80++`, SetLOD.
//
//  0x100727b9..0x10072833  L_Draw: tu0/tv0 scaled by g_a1008e664[0],
//      DrawPrimitive(TRIANGLEFAN, 0x2c4, pVerts, nVerts, 0), `return 1`.
//
//  0x10072838..0x1007291e  THE SIX SUNK CLIP EPSILON ARMS, one per pass,
//      each `flds +1e-5 ; fcomp ; jb <back into the divide> ; fstp ; fstp
//      [; fstp] ; flds 0.0f ; fstpl <fT> ; jmp <back past the divide>`.
//      That is the `else fT = 0.0f;` half of `fDen < -eps || fDen > eps`.
//
//  0x1007291e..0x10072954  L_ElseLOD: the record was already bound to
//      this stage, so only the SetLOD survives.
//
//  0x10072954..0x10072968  The two `break` trampolines out of the record
//      search, `mov %eax,%edx ; jmp 0x100726a9`.
//
//  0x10072968..0x100729b3  L_MakeRec: the two sub_10039bd0 arms.
//
//  ------------------------------------------------------------------- //
//  COIN-FLIPS, both spellings stated so one build settles each.
//  ------------------------------------------------------------------- //
//
//  1. THE FILL LOOP'S GUARD IS `jbe`, NOT `je`.  0x10070d37 is
//     `test %ecx,%ecx ; ... ; jbe 0x10070ddd` while every other
//     count-guarded loop in the function (0x10070e2b, 0x10070f2e,
//     0x100727c8, 0x10072617) is `test ; je`.  On a `test reg,reg` the
//     two are the same branch, so icl chose `jbe` for a reason, and the
//     reason is most likely an UNSIGNED RELATIONAL in the source rather
//     than the `while (nLeft--)` written below.  The alternative
//     spelling, which is the one to try if the fill loop's head does not
//     come out:
//
//         uint32 i;
//         for (i = 0; i < pObj->m_nCount50; i++)
//         {
//             ... body indexed by i ...
//         }
//
//     Both produce a `dec ; mov ; jne` back edge (the shipped body
//     decrements -0xb4(%ebp) at 0x10070dd0), so only the guard's mnemonic
//     separates them.  Written `while (nLeft--)` below for consistency
//     with the four sibling loops and with big6d910.cpp.
//
//  2. `float fT` vs `double fT`.  All six passes spill fT with `fstpl`
//     and reload it with `fmull` (0x100710d7/0x100710e1 and the five
//     twins).  That is an 8-byte slot, which a `double` would give
//     directly -- but so does an icl x87 register spill of a `float`
//     expression, and big5a960.cpp / big124a4.cpp / big6d910.cpp all
//     wrote `float` and got the same bytes.  Written `float` here for
//     consistency; `double fT;` is the one-token alternative.
//
//  3. THE HAND-PLACED COLD BLOCKS.  L_Tex (0x10072642), L_Bind
//     (0x100726c3), L_Draw (0x100727b9), L_ElseLOD (0x1007291e) and
//     L_MakeRec (0x10072968) are written below as `goto` targets laid out
//     at the BOTTOM of the function in the SHIPPED ADDRESS ORDER.  This
//     is big5a960.cpp's and big6d910.cpp's technique and it is here for
//     one reason: it makes the EMITTED call sequence match without
//     depending on icl re-deriving the same sinking decisions.  The
//     structured spelling of the same logic is
//
//         if (!pRec)                        { ...sub_10039bd0... }
//         if (pRec != g_a1008e684[nStage])  { ...bind... }
//         else                              { ...LOD only... }
//
//     and if a build shows icl sinking those by itself into the shipped
//     order, that spelling is the better one and the labels below should
//     go.  The logic is identical either way; only block placement
//     differs.
//
//  4. THE TEXTURE-SCALE MULTIPLY ORDER.  0x100727d4 loads tu0 then
//     `fmuls g_1008e664`, but 0x100727dd loads g_1008e668 then
//     `fmuls tv0` -- the two are spelled with opposite operand order in
//     the bytes, exactly as in big6d910.cpp.  Written as `*=` for both
//     below.  The alternative that reproduces the asymmetry literally is
//         pV->m_fU0 = pV->m_fU0 * g_a1008e664[0].m_fInvW00;
//         pV->m_fV0 = g_a1008e664[0].m_fInvH04 * pV->m_fV0;
//     (icl emits the source's LEFT operand as the load).
//
//  5. THE FOUR GENERATED-COORDINATE GLOBALS.  0x1008ad94, 0x1008ad98,
//     0x1008ad9c, 0x1008ada0 are four adjacent floats used as
//     (scaleU, scaleV, offsetU, offsetV) -- the scales at the low pair
//     and the offsets at the high pair, which is what makes them look
//     like TWO two-float structures rather than one four-float array.
//     Declared as four separate floats below because nothing in the bytes
//     distinguishes the groupings; if a sibling function is ever found
//     that indexes them, this should become `g_a1008ad94[2]` and
//     `g_a1008ad9c[2]`.  No bytes depend on the choice.
//
//  ------------------------------------------------------------------- //
//  WHAT IS *NOT* PINNED
//  ------------------------------------------------------------------- //
//
//  * 0x10064800 IS NOT DECODED.  Only its call shape is: three dword
//    arguments in the first three outgoing slots, no return value tested.
//    Its own body (900+ bytes of per-vertex lighting) is out of scope
//    here and it is declared, not written.
//  * THE SECOND ARGUMENT.  0xc(%ebp) is never read (grep over the whole
//    disassembly: zero hits).  It is declared because the ONE caller
//    pushes it: 0x10072c9b is `push %edi ; push %eax ;
//    call 0x10070d10 ; add $0x8,%esp`.  Strictly that second push could
//    be icl's outgoing-area alignment rather than an argument -- the
//    same ambiguity big6d910.cpp records for its own `pCtx` -- and
//    nothing inside these 7.3 K settles it.  Declared as a parameter to
//    match big6d910.cpp, which is the same source function.
//  * THE FIVE DEAD 1.0f CONSTANTS.  0x100ab7c0 is the `1.0f /` of the
//    inline transform; 0x100ab7c8, 0x100ab7d0, 0x100ab7d8, 0x100ab7e0 and
//    0x100ab7e8 are also 1.0f, interleaved one-for-one with the six
//    -1e-5 epsilons, and NONE of them is loaded anywhere in the function.
//    big5a960.cpp and big61a30.cpp found the same dead 1.0f slots in
//    their copies of this clipper.  Reproduced by writing the same
//    source; not otherwise explained.
//  * THE STORE ORDER inside the inline projective loop (x, then rhw as a
//    double spill, then y, then z, software-pipelined) is icl's schedule,
//    not a source order.
//  * THE FRAME SLOT ORDER.  icl overlaps -0x70/-0x78/-0x80/-0x88/-0x90/
//    -0x98/-0xa0 between the pointer/counter roles and the six `fT`
//    doubles, and reuses -0xb4 for both the fill loop's counter and the
//    clipper's `nIn`.  Nothing in the source controls that.
//  * THE UNROLLED RECORD SEARCH.  0x10072681..0x100726a5 is one loop body
//    emitted twice with two separate `break` trampolines (0x1007295e for
//    the first copy, 0x10072954 for the second).  That is icl unrolling a
//    `while (pCur)` walk x2; the source has one copy.  big6d910.cpp's
//    copy of the same source is unrolled the same way and records the
//    same pair of trampolines (0x1006f4af / 0x1006f4b6).
//  * THE VERTEX'S SPECULAR DWORD (+0x14) IS NEVER WRITTEN by this
//    function -- not by the fill loop, not anywhere -- yet every one of
//    the six clip passes INTERPOLATES its top byte (+0x17).  Confirmed by
//    grep: the only `0x14(%reg)` sites in the whole 7.3 K are the texture
//    record's +0x14 stamp, and the only `0x17(%reg)` sites are the thirty
//    clipper byte-lerps.  So the specular the FVF advertises comes in
//    with the buffer, and the reconstruction leaves it alone too.  Same
//    finding as big6d910.cpp.
//  * +0x0c (RHW) is likewise written only on the unclipped path (the
//    inline transform) and by 0x1000d940 on the clipped one; the fill
//    loop leaves it.
//  * THE SOURCE VERTEX'S +0x0c..+0x13 are never read.  Its stride is
//    0x18 (`add $0x18,%esi`) and its live fields are +0x00 (pointer),
//    +0x04, +0x08, +0x14, +0x15, +0x16.  big6d910's twin reads +0x0c and
//    +0x10 as a second texture pair; this one generates that pair
//    instead, so those two slots are padding HERE and named as such.
//
// *********************************************************************** //

#include "ltbasedefs.h"


// ---------------------------------------------------------------------- //
//  The D3D7 device.  Three vtable slots are reached: +0x50 SetRenderState
//  (0x10072773), +0x64 DrawPrimitive (0x1007282d) and +0x8c SetTexture
//  (0x10072748).  Same declaration big61a30.cpp and big6d910.cpp use.
// ---------------------------------------------------------------------- //
struct D3DDevice;

struct D3DDeviceVtbl
{
	uint8			m_pad_00[0x50];
	long (__stdcall* SetRenderState)(D3DDevice*, uint32, uint32);   // +0x50
	uint8			m_pad_54[0x64 - 0x54];
	long (__stdcall* DrawPrimitive)(D3DDevice*, uint32, uint32, void*,
	                                uint32, uint32);                // +0x64
	uint8			m_pad_68[0x8c - 0x68];
	long (__stdcall* SetTexture)(D3DDevice*, uint32, void*);        // +0x8c
};

struct D3DDevice
{
	D3DDeviceVtbl*	lpVtbl;
};


// IDirectDrawSurface7: only +0xbc, SetLOD (0x100727ad and 0x10072940).
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


// ---------------------------------------------------------------------- //
//  The 40-byte vertex.  FVF 0x2c4 = XYZRHW | DIFFUSE | SPECULAR | TEX2,
//  read straight off the DrawPrimitive at 0x100727fc/0x10072807.  Field
//  for field big6d910.cpp's Vert6d910.  The two texture pairs are both
//  interpolated by every one of the six clip passes here (twenty-four
//  sites), which no 32-byte vertex would have.
// ---------------------------------------------------------------------- //
union VColor70d10
{
	uint32		m_n;
	uint8		m_b[4];
};

struct Vec70d10
{
	float	x;                                          // +0x00
	float	y;                                          // +0x04
	float	z;                                          // +0x08
};

struct Vert70d10
{
	// 0x1000d9d0 -- d3dbatchb.cpp's 21-byte two-field __thiscall setter
	// (`push %esi ; mov 0x8(%esp),%eax ; mov 0xc(%esp),%edx ;
	//   mov %eax,0x18(%ecx) ; mov %edx,0x1c(%ecx) ; add $0x4,%esp ;
	//   ret $0x8`), called at 0x10070d9c with the vertex in %ecx.  The
	// `add $-0x8,%esp` at the head of the fill loop body (0x10070d4b) is
	// its outgoing area and the callee's `ret $0x8` reclaims it, which is
	// why there is no matching `add $0x8,%esp` inside the loop.
	void			sub_1000d9d0(float fU, float fV);

	Vec70d10		m_vPos;                             // +0x00
	float			m_fRHW;                             // +0x0c
	VColor70d10		m_Diffuse10;                        // +0x10
	VColor70d10		m_Specular14;                       // +0x14
	float			m_fU0;                              // +0x18
	float			m_fV0;                              // +0x1c
	float			m_fU1;                              // +0x20
	float			m_fV1;                              // +0x24
};


// ---------------------------------------------------------------------- //
//  The SOURCE vertex, pObj+0x54.  Stride 0x18 from `add $0x18,%esi` at
//  0x10070daf.  +0x00 is a POINTER: 0x10070d49 does `mov (%esi),%edi` and
//  then reads (%edi), 0x4(%edi) and 0x8(%edi) (0x10070d4e..0x10070d5a)
//  and again at 0x10070da1/0x10070dbb.  +0x04/+0x08 go to the vertex's
//  +0x18/+0x1c through the thiscall setter.  +0x14/+0x15/+0x16 are three
//  adjacent colour bytes, each indexing its own ramp.  +0x0c..+0x13 and
//  +0x17 are never read and exist only because the stride says 24.
// ---------------------------------------------------------------------- //
struct SrcVert70d10
{
	Vec70d10*	m_pPos00;                               // +0x00
	float		m_fU004;                                // +0x04
	float		m_fV008;                                // +0x08
	uint8		m_pad_0c[0x14 - 0x0c];                  // +0x0c
	uint8		m_nBlue14;                              // +0x14
	uint8		m_nGreen15;                             // +0x15
	uint8		m_nRed16;                               // +0x16
	uint8		m_pad_17;                               // +0x17
};


// ---------------------------------------------------------------------- //
//  ltlink.h's CheapLTLink, as the texture record's +0x1c member.  The
//  dl_Remove / dl_TieOff / dl_Insert sequence at 0x100726df..0x1007270f
//  reads +0x00 as prev and +0x04 as next.
// ---------------------------------------------------------------------- //
struct Link70d10
{
	Link70d10*	m_pPrev;                                // +0x00
	Link70d10*	m_pNext;                                // +0x04
	void*		m_pData;                                // +0x08
};


// ---------------------------------------------------------------------- //
//  The per-stage texture record, field for field big61a30.cpp's Tex61a30
//  and big6d910.cpp's Tex6d910.  Its vtable is never called from here, so
//  slot 0 is a plain pointer.
// ---------------------------------------------------------------------- //
struct Tex70d10
{
	void*		m_pVtbl00;                              // +0x00
	float		m_fInvW04;                              // +0x04
	float		m_fInvH08;                              // +0x08
	DDSurface*	m_pSurface0c;                           // +0x0c
	uint32		m_nBytes10;                             // +0x10
	uint16		m_nStamp14;                             // +0x14
	uint16		m_nAlphaRef16;                          // +0x16
	uint8		m_pad_18[0x1c - 0x18];
	Link70d10	m_Link1c;                               // +0x1c
	uint8		m_pad_28[0x30 - 0x28];
	Tex70d10*	m_pNext30;                              // +0x30
	uint8		m_pad_34[0x42 - 0x34];
	uint16		m_nStage42;                             // +0x42
	uint16		m_nDirty44;                             // +0x44
};


// The block reached through pObj->m_p28->m_pBlock30: a list head at +0x0c
// and a sixteen-bit stamp at +0x30 (`mov %ax,0x30(%edi)` at 0x10072668).
struct TexBlock70d10
{
	uint8			m_pad_00[0x0c];
	Tex70d10*		m_pHead0c;                          // +0x0c
	uint8			m_pad_10[0x30 - 0x10];
	uint16			m_nStamp30;                         // +0x30
};


// The pair published per stage at 0x1008e664 -- `mov %ecx,(%edx,%eax,8)`
// and `mov %esi,0x4(%edx,%eax,8)` at 0x10072785/0x10072788.
struct TexScale70d10
{
	float		m_fInvW00;                              // +0x00
	float		m_fInvH04;                              // +0x04
};


// 0x1008cbcc: only +0x48 is touched here (`add %edx,0x48(%eax)` at
// 0x1007271a).
struct Stats70d10
{
	uint8		m_pad_00[0x48];
	uint32		m_nBytes48;                             // +0x48
};


// ---------------------------------------------------------------------- //
//  THE TRANSFORM OBJECT.  Only the projective block at +0x1dc..+0x218 is
//  read by name here -- 0x1008ba5c..0x1008ba98 is 0x1008b880 + 0x1dc
//  .. 0x218 exactly.  Kept apart from the opaque `uint8[]` view every
//  other user of 0x1008b880 in the project takes, and reached through a
//  cast, exactly as big124a4.cpp and big6d910.cpp do.
// ---------------------------------------------------------------------- //
struct XForm70d10
{
	uint8	m_pad_000[0x1dc];                           // +0x000 .. +0x1db
	float	m_f1dc;                                     // +0x1dc
	float	m_f1e0;                                     // +0x1e0
	float	m_f1e4;                                     // +0x1e4
	float	m_f1e8;                                     // +0x1e8
	float	m_f1ec;                                     // +0x1ec
	float	m_f1f0;                                     // +0x1f0
	float	m_f1f4;                                     // +0x1f4
	float	m_f1f8;                                     // +0x1f8
	float	m_f1fc;                                     // +0x1fc
	float	m_f200;                                     // +0x200
	float	m_f204;                                     // +0x204
	float	m_f208;                                     // +0x208
	float	m_f20c;                                     // +0x20c
	float	m_f210;                                     // +0x210
	float	m_f214;                                     // +0x214
	float	m_f218;                                     // +0x218
};


// ---------------------------------------------------------------------- //
//  The item.  Only three fields are read HERE: +0x28 (0x1007264b), +0x50
//  (0x10070d2a and 0x10070dec) and +0x54 (0x10070d31).  0x10064800 reads
//  +0x2c and, through +0x28, +0x34; +0x2c is named so the layout is
//  honest about where +0x50 sits.
// ---------------------------------------------------------------------- //
struct TexHolder70d10
{
	uint8			m_pad_00[0x30];
	TexBlock70d10*	m_pBlock30;                         // +0x30
};

struct Obj70d10
{
	uint8			m_pad_00[0x28];
	TexHolder70d10*	m_p28;                              // +0x28
	void*			m_p2c;                              // +0x2c
	uint8			m_pad_30[0x50 - 0x30];
	uint16			m_nCount50;                         // +0x50
	uint8			m_pad_52[0x54 - 0x52];
	SrcVert70d10	m_aSrc54[1];                        // +0x54
};


// ---------------------------------------------------------------------- //
//  Externals.  Relocations are masked, so only the shapes matter.
// ---------------------------------------------------------------------- //
extern D3DDevice*		g_pD3DDevice;                   // 0x1008d988

extern uint16			g_w1008b870;                    // 0x1008b870
extern uint32			g_n1008b874;                    // 0x1008b874
extern Vert70d10*		g_p1008b87c;                    // 0x1008b87c
extern uint8			g_1008b880[];                   // 0x1008b880
extern float			g_f1008b908;                    // 0x1008b908  near
extern float			g_f1008b910;                    // 0x1008b910  far

// 0x1008ad94..0x1008ada0 -- the generated second texture pair's scale and
// offset.  See coin-flip 5.
extern float			g_f1008ad94;                    // 0x1008ad94  scale u
extern float			g_f1008ad98;                    // 0x1008ad98  scale v
extern float			g_f1008ad9c;                    // 0x1008ad9c  offset u
extern float			g_f1008ada0;                    // 0x1008ada0  offset v

extern uint32			g_n1008bd80;                    // 0x1008bd80
extern uint32			g_n1008be44;                    // 0x1008be44
extern uint8			g_b1008be60;                    // 0x1008be60
extern uint16			g_w1008bea8;                    // 0x1008bea8
extern Stats70d10*		g_p1008cbcc;                    // 0x1008cbcc

// 0x1008d1bc, 0x1008d2bc, 0x1008d3bc -- three 256-entry byte ramps in
// BSS, exactly 0x100 apart.  d3dhigh.cpp names them and its 0x100598a0 is
// the out-of-line version of the four-statement block inlined here.
extern uint8			g_ColorRamp2[256];              // 0x1008d1bc
extern uint8			g_ColorRamp1[256];              // 0x1008d2bc
extern uint8			g_ColorRamp0[256];              // 0x1008d3bc

extern void*			g_p1008d7a0;                    // 0x1008d7a0  UseD3DClip
extern uint32			g_n1008d7d0;                    // 0x1008d7d0  stage
extern Link70d10*		g_p1008e634;                    // 0x1008e634
extern TexScale70d10	g_a1008e664[];                  // 0x1008e664
extern Tex70d10*		g_a1008e684[];                  // 0x1008e684


// 0x1000d940 and 0x1000d9b0 -- d3dlow.cpp's two transform helpers, both
// __cdecl (vertex, transform object).  0x1000d9b0 forwards to 0x1000a770
// with the 3x4 at +0x15c; 0x1000d940 is the screen projection through
// +0x31c..+0x330.
extern "C" void  sub_1000d940(Vert70d10* pVert, uint8* pXForm);
extern "C" void  sub_1000d9b0(Vert70d10* pVert, uint8* pXForm);

// 0x10039bd0 -- d3dmid.cpp's stack-ABI entry; the body is at 0x10039bdc.
extern "C" Tex70d10* sub_10039bd0(TexBlock70d10* pBlock, uint32 nStage,
                                  uint32 bLink);

// 0x10064800 -- __cdecl, three dword arguments in the first three
// outgoing slots (0x10070df9/0x10070dfd/0x10070e01), no return value
// tested.  A per-vertex lighting pass; NOT decoded here.
extern "C" void  sub_10064800(Obj70d10* pObj, Vert70d10* pVerts,
                              uint32 nVerts);


// The frame says sixty-four: aVerts runs -0x1004(%ebp)..-0x605(%ebp),
// which is 0xa00 bytes, and 0xa00 / 0x28 = 64.  There is no
// overflow-guard callee in this function to cross-check that against.
#define VERTS70D10_MAX	64


// Round to nearest in the current FPU mode: w1_10001000.cpp's helper
// (0x10015190 out of line); inlined it is the shipped `fstps t ; flds t ;
// fistpl n` with t and n in frame slots of their own.
inline int32 RoundF(float f)
{
	int32 n;
	__asm fld f
	__asm fistp n
	return n;
}

// ======================================================================= //
//  THE CLIP PATH -- 0x1000a8a0's body INLINE (XFormClip,
//  w23_100525b0_os.cpp's 40-byte version): the pre-transform, the inline
//  0x1000f4b0 driver over the six inline plane bodies 0x10002310..
//  0x10003410, the screen projection.  It takes &pVerts / &nVerts, which
//  is why the shipped code re-reads both from their frame slots after the
//  lighting call (0x10070e19 / 0x10070f1c) instead of knowing pVerts is
//  aVerts.
// ======================================================================= //
#define CLIP70D10_MAX	50

static inline void Lerp70d10(Vert70d10* a, Vert70d10* b, Vert70d10* dst, float t)
{
	dst->m_fU0 = (b->m_fU0 - a->m_fU0) * t + a->m_fU0;
	dst->m_fV0 = (b->m_fV0 - a->m_fV0) * t + a->m_fV0;
	dst->m_fU1 = (b->m_fU1 - a->m_fU1) * t + a->m_fU1;
	dst->m_fV1 = (b->m_fV1 - a->m_fV1) * t + a->m_fV1;
	dst->m_Diffuse10.m_b[2]  = (uint8)RoundF((b->m_Diffuse10.m_b[2]  - a->m_Diffuse10.m_b[2])  * t + a->m_Diffuse10.m_b[2]);
	dst->m_Diffuse10.m_b[1]  = (uint8)RoundF((b->m_Diffuse10.m_b[1]  - a->m_Diffuse10.m_b[1])  * t + a->m_Diffuse10.m_b[1]);
	dst->m_Diffuse10.m_b[0]  = (uint8)RoundF((b->m_Diffuse10.m_b[0]  - a->m_Diffuse10.m_b[0])  * t + a->m_Diffuse10.m_b[0]);
	dst->m_Diffuse10.m_b[3]  = (uint8)RoundF((b->m_Diffuse10.m_b[3]  - a->m_Diffuse10.m_b[3])  * t + a->m_Diffuse10.m_b[3]);
	dst->m_Specular14.m_b[3] = (uint8)RoundF((b->m_Specular14.m_b[3] - a->m_Specular14.m_b[3]) * t + a->m_Specular14.m_b[3]);
}


static inline float ClipEdgeNear70d10(Vec70d10* a, Vec70d10* b, Vec70d10* dst)
{
	float fDenom = b->z - a->z;
	float t;

	if (fDenom < -0.00001f || fDenom > 0.00001f)
		t = (g_f1008b908 - a->z) / fDenom;
	else
		t = 0.0f;

	dst->x = (b->x - a->x) * t + a->x;
	dst->y = (b->y - a->y) * t + a->y;
	dst->z = g_f1008b908;

	return t;
}


static inline float ClipEdgeLeft70d10(Vec70d10* a, Vec70d10* b, Vec70d10* dst)
{
	float fDenom = (b->x - a->x) + b->z - a->z;
	float t;
	float fZ;

	if (fDenom < -0.00001f || fDenom > 0.00001f)
		t = -(a->x + a->z) / fDenom;
	else
		t = 0.0f;

	dst->y = (b->y - a->y) * t + a->y;
	fZ = (b->z - a->z) * t + a->z;
	dst->z = fZ;
	dst->x = -fZ;

	return t;
}


static inline float ClipEdgeTop70d10(Vec70d10* a, Vec70d10* b, Vec70d10* dst)
{
	float fDenom = (b->y - a->y) - b->z + a->z;
	float t;
	float fZ;

	if (fDenom < -0.00001f || fDenom > 0.00001f)
		t = -(a->y - a->z) / fDenom;
	else
		t = 0.0f;

	dst->x = (b->x - a->x) * t + a->x;
	fZ = (b->z - a->z) * t + a->z;
	dst->y = fZ;
	dst->z = fZ;

	return t;
}


static inline float ClipEdgeRight70d10(Vec70d10* a, Vec70d10* b, Vec70d10* dst)
{
	float fDenom = (b->x - a->x) - b->z + a->z;
	float t;
	float fZ;

	if (fDenom < -0.00001f || fDenom > 0.00001f)
		t = -(a->x - a->z) / fDenom;
	else
		t = 0.0f;

	dst->y = (b->y - a->y) * t + a->y;
	fZ = (b->z - a->z) * t + a->z;
	dst->x = fZ;
	dst->z = fZ;

	return t;
}


static inline float ClipEdgeBottom70d10(Vec70d10* a, Vec70d10* b, Vec70d10* dst)
{
	float fDenom = (b->y - a->y) + b->z - a->z;
	float t;
	float fZ;

	if (fDenom < -0.00001f || fDenom > 0.00001f)
		t = -(a->y + a->z) / fDenom;
	else
		t = 0.0f;

	dst->x = (b->x - a->x) * t + a->x;
	fZ = (b->z - a->z) * t + a->z;
	dst->z = fZ;
	dst->y = -fZ;

	return t;
}


static inline float ClipEdgeFar70d10(Vec70d10* a, Vec70d10* b, Vec70d10* dst)
{
	float fDenom = b->z - a->z;
	float t;

	if (fDenom < -0.00001f || fDenom > 0.00001f)
		t = (g_f1008b910 - a->z) / fDenom;
	else
		t = 0.0f;

	dst->x = (b->x - a->x) * t + a->x;
	dst->y = (b->y - a->y) * t + a->y;
	dst->z = g_f1008b910;

	return t;
}


// 0x10002310
static inline LTBOOL ClipPlaneNear70d10(uint32* pUnused, Vert70d10** ppVerts, int32* pnVerts, Vert70d10** ppOut)
{
	uint32      anInside[CLIP70D10_MAX];
	Vert70d10*  pOutStart;
	Vert70d10*  pV;
	Vert70d10*  pEnd;
	Vert70d10*  pPrev;
	Vert70d10*  pCur;
	uint32*     pFlag;
	int32       nInside;
	int32       i;
	int32       iPrev;

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
				float t = ClipEdgeNear70d10(&pPrev->m_vPos, &pCur->m_vPos, &(*ppOut)->m_vPos);

				Lerp70d10(pPrev, pCur, *ppOut, t);

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
static inline LTBOOL ClipPlaneLeft70d10(uint32* pUnused, Vert70d10** ppVerts, int32* pnVerts, Vert70d10** ppOut)
{
	uint32      anInside[CLIP70D10_MAX];
	Vert70d10*  pOutStart;
	Vert70d10*  pV;
	Vert70d10*  pEnd;
	Vert70d10*  pPrev;
	Vert70d10*  pCur;
	uint32*     pFlag;
	int32       nInside;
	int32       i;
	int32       iPrev;

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
				float t = ClipEdgeLeft70d10(&pPrev->m_vPos, &pCur->m_vPos, &(*ppOut)->m_vPos);

				Lerp70d10(pPrev, pCur, *ppOut, t);

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
static inline LTBOOL ClipPlaneTop70d10(uint32* pUnused, Vert70d10** ppVerts, int32* pnVerts, Vert70d10** ppOut)
{
	uint32      anInside[CLIP70D10_MAX];
	Vert70d10*  pOutStart;
	Vert70d10*  pV;
	Vert70d10*  pEnd;
	Vert70d10*  pPrev;
	Vert70d10*  pCur;
	uint32*     pFlag;
	int32       nInside;
	int32       i;
	int32       iPrev;

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
				float t = ClipEdgeTop70d10(&pPrev->m_vPos, &pCur->m_vPos, &(*ppOut)->m_vPos);

				Lerp70d10(pPrev, pCur, *ppOut, t);

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
static inline LTBOOL ClipPlaneRight70d10(uint32* pUnused, Vert70d10** ppVerts, int32* pnVerts, Vert70d10** ppOut)
{
	uint32      anInside[CLIP70D10_MAX];
	Vert70d10*  pOutStart;
	Vert70d10*  pV;
	Vert70d10*  pEnd;
	Vert70d10*  pPrev;
	Vert70d10*  pCur;
	uint32*     pFlag;
	int32       nInside;
	int32       i;
	int32       iPrev;

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
				float t = ClipEdgeRight70d10(&pPrev->m_vPos, &pCur->m_vPos, &(*ppOut)->m_vPos);

				Lerp70d10(pPrev, pCur, *ppOut, t);

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
static inline LTBOOL ClipPlaneBottom70d10(uint32* pUnused, Vert70d10** ppVerts, int32* pnVerts, Vert70d10** ppOut)
{
	uint32      anInside[CLIP70D10_MAX];
	Vert70d10*  pOutStart;
	Vert70d10*  pV;
	Vert70d10*  pEnd;
	Vert70d10*  pPrev;
	Vert70d10*  pCur;
	uint32*     pFlag;
	int32       nInside;
	int32       i;
	int32       iPrev;

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
				float t = ClipEdgeBottom70d10(&pPrev->m_vPos, &pCur->m_vPos, &(*ppOut)->m_vPos);

				Lerp70d10(pPrev, pCur, *ppOut, t);

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
static inline LTBOOL ClipPlaneFar70d10(uint32* pUnused, Vert70d10** ppVerts, int32* pnVerts, Vert70d10** ppOut)
{
	uint32      anInside[CLIP70D10_MAX];
	Vert70d10*  pOutStart;
	Vert70d10*  pV;
	Vert70d10*  pEnd;
	Vert70d10*  pPrev;
	Vert70d10*  pCur;
	uint32*     pFlag;
	int32       nInside;
	int32       i;
	int32       iPrev;

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
				float t = ClipEdgeFar70d10(&pPrev->m_vPos, &pCur->m_vPos, &(*ppOut)->m_vPos);

				Lerp70d10(pPrev, pCur, *ppOut, t);

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


// 0x1000f4b0 -- d3dr3c_os.cpp's driver, over the 40-byte vertex.
static inline LTBOOL ClipPoly70d10(uint32 nClipFlags, Vert70d10** ppVerts, int32* pnVerts)
{
	Vert70d10*  pOut;
	Vert70d10*  pVerts;
	int32       nVerts;
	uint32      anUnused[6];

	if (g_p1008d7a0)
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
		if (!ClipPlaneNear70d10(&anUnused[0], &pVerts, &nVerts, &pOut))
			return LTFALSE;
	}

	if (nClipFlags & 0x04)
	{
		if (!ClipPlaneLeft70d10(&anUnused[1], &pVerts, &nVerts, &pOut))
			return LTFALSE;
	}

	if (nClipFlags & 0x08)
	{
		if (!ClipPlaneTop70d10(&anUnused[2], &pVerts, &nVerts, &pOut))
			return LTFALSE;
	}

	if (nClipFlags & 0x10)
	{
		if (!ClipPlaneRight70d10(&anUnused[3], &pVerts, &nVerts, &pOut))
			return LTFALSE;
	}

	if (nClipFlags & 0x20)
	{
		if (!ClipPlaneBottom70d10(&anUnused[4], &pVerts, &nVerts, &pOut))
			return LTFALSE;
	}

	if (!(nClipFlags & 0x02) ||
	    ClipPlaneFar70d10(&anUnused[5], &pVerts, &nVerts, &pOut))
	{
		*ppVerts = pVerts;
		*pnVerts = nVerts;
		return LTTRUE;
	}

	return LTFALSE;
}


// 0x1000a8a0's body, inline: the projective no-clip transform, or the
// camera-space pre-transform, the clipper and the screen projection.
static inline LTBOOL XFormClip70d10(Vert70d10** ppVerts, int32* pnVerts, XForm70d10* pM)
{
	Vert70d10*  pWalk;
	int32       nLeft;
	float       fOOW;
	Vec70d10    vScreen;

	if (!g_n1008b874)
	{
		nLeft = *pnVerts;
		pWalk = *ppVerts;
		while (nLeft--)
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

			pWalk->m_vPos = vScreen;
			pWalk->m_fRHW = fOOW;
			pWalk++;
		}
	}
	else
	{
		pWalk = *ppVerts;
		nLeft = *pnVerts;
		while (nLeft--)
		{
			sub_1000d9b0(pWalk, g_1008b880);
			pWalk++;
		}

		if (!ClipPoly70d10(g_n1008b874, ppVerts, pnVerts))
			return LTFALSE;

		nLeft = *pnVerts;
		pWalk = *ppVerts;
		while (nLeft--)
		{
			sub_1000d940(pWalk, g_1008b880);
			pWalk++;
		}
	}

	return LTTRUE;
}

// ======================================================================= //
//  0x10070d10 .. 0x100729b2   (7,331 bytes)
// ======================================================================= //
int32 sub_10070d10(Obj70d10* pObj, void* pCtx)
{
	// The declaration order is the frame order the shipped code uses,
	// lowest slot first: aVerts at -0x1004, the pVerts/nVerts pair
	// immediately above it at -0x604/-0x600, then XFormClip70d10's
	// vScreen and the six plane bodies' flag blocks.
	Vert70d10		aVerts[VERTS70D10_MAX];
	Vert70d10*		pVerts;
	int32			nVerts;

	Vert70d10*		pWalk;
	SrcVert70d10*	pSrc;
	uint32			nLeft;

	Vert70d10*		pV;

	TexBlock70d10*	pBlock;
	Tex70d10*		pRec;
	Tex70d10*		pCur;
	uint32			nStage;

	// ------------------------------------------------------------------ //
	//  0x10070d13: `mov $0x1108,%eax ; call 0x100805a0` is _chkstk, so
	//  the frame is 4,360 bytes; see the frame arithmetic in the banner.
	//
	//  0x10070d49 -- BUILD THE VERTICES.  Three floats through the source
	//  entry's +0x00 POINTER, the three colour bytes through the three
	//  256-entry ramps into +0x12/+0x11/+0x10 (highest destination byte
	//  first, which is what d3dhigh.cpp's 0x100598a0 records for the same
	//  four statements), the alpha byte from g_1008be60 into +0x13, the
	//  first texture pair through the __thiscall setter, and the second
	//  texture pair GENERATED from the world position's x and z.  Every
	//  float in the position copy moves as an INTEGER `mov`.
	// ------------------------------------------------------------------ //
	pWalk = aVerts;
	pSrc  = pObj->m_aSrc54;
	nLeft = pObj->m_nCount50;

	while (nLeft--)
	{
		pWalk->m_vPos.x = pSrc->m_pPos00->x;
		pWalk->m_vPos.y = pSrc->m_pPos00->y;
		pWalk->m_vPos.z = pSrc->m_pPos00->z;

		pWalk->m_Diffuse10.m_b[2] = g_ColorRamp2[pSrc->m_nRed16];
		pWalk->m_Diffuse10.m_b[1] = g_ColorRamp1[pSrc->m_nGreen15];
		pWalk->m_Diffuse10.m_b[0] = g_ColorRamp0[pSrc->m_nBlue14];
		pWalk->m_Diffuse10.m_b[3] = g_b1008be60;

		pWalk->sub_1000d9d0(pSrc->m_fU004, pSrc->m_fV008);

		// 0x10070da1 / 0x10070dbb -- `flds <pos> ; fadds <off> ;
		// fmuls <scale>`, so the offset is added first and the source's
		// LEFT operand is the position.
		pWalk->m_fU1 = (pSrc->m_pPos00->x + g_f1008ad9c) * g_f1008ad94;
		pWalk->m_fV1 = (pSrc->m_pPos00->z + g_f1008ada0) * g_f1008ad98;

		pSrc++;
		pWalk++;
	}

	// -- 0x10070ddd: both stores are in the shipped bytes (0x10070de6 and
	//    0x10070df3) BEFORE the call, and both slots are re-read from the
	//    frame afterwards, so pVerts/nVerts are real locals and not
	//    register temporaries.  The lighting pass returns nothing that is
	//    tested.
	pVerts = aVerts;
	nVerts = pObj->m_nCount50;

	sub_10064800(pObj, pVerts, nVerts);

	// -- 0x10070e0c: 0x1000a8a0's body, inline (XFormClip70d10 above). -- //
	if (!XFormClip70d10(&pVerts, &nVerts, (XForm70d10*)g_1008b880))
		goto L_Reject;

	goto L_Tex;

L_Reject:
	// -- 0x10072639: the only epilogue in the function.  `return 1` at
	//    0x10072830 cross-jumps INTO it at 0x1007263b.
	return 0;


	// ================================================================== //
	//  THE SUNK TAIL, 0x10072642..0x100729b2, in shipped address order.
	//  Every block below the `ret` is a cold arm of a test above it; icl
	//  put them after the epilogue and they are placed here by hand so
	//  the emitted call sequence matches.  See coin-flip 3.
	// ================================================================== //

L_Tex:
	// ------------------------------------------------------------------ //
	//  0x10072642 -- FIND (OR MAKE) THIS STAGE'S TEXTURE RECORD, then
	//  bind it.  big61a30.cpp's and big6d910.cpp's block field for field.
	// ------------------------------------------------------------------ //
	nStage = g_n1008d7d0;
	pBlock = pObj->m_p28->m_pBlock30;

	if (!pBlock)
		goto L_Draw;

	pBlock->m_nStamp30 = g_w1008b870;

	pRec = LTNULL;

	// The list test is emitted TWICE off ONE `test %esi,%esi` --
	// `je 0x10072968` at 0x1007266c and `je 0x100726a7` at 0x10072675 --
	// and icl 4.0 does not fold a redundant null test, so the source
	// really does test it twice.  big61a30.cpp and big6d910.cpp record the
	// same pair.  Here icl additionally folded the first of them straight
	// to L_MakeRec (empty list => pRec stays null => the `if (!pRec)`
	// below is taken), which is why 0x1007266c's target is 0x10072968 and
	// not 0x100726a7.
	if (pBlock->m_pHead0c)
	{
		pCur = pBlock->m_pHead0c;

		while (pCur)
		{
			// `cmp %ecx,%edx` at 0x10072685 puts the RECORD's field in
			// the destination, so it is the source's left operand.  The
			// walk is UNROLLED x2 in the shipped bytes with two separate
			// `break` trampolines (0x1007295e for the first copy,
			// 0x10072954 for the second); the source has one copy, and
			// big6d910.cpp records the same pair.
			if (pCur->m_nStage42 == (uint8)g_n1008d7d0)
			{
				pRec = pCur;
				break;
			}

			pCur = pCur->m_pNext30;
		}
	}

	// 0x100726ab.  A freshly-made record joins the bind at 0x100726c3
	// WITHOUT going through the comparison below (both create arms
	// `jmp 0x100726c3`), which is why the comparison sits inside the
	// found-it arm and not after the two of them.
	if (!pRec)
		goto L_MakeRec;

	if (pRec == g_a1008e684[nStage])
		goto L_ElseLOD;

L_Bind:
	// -- 0x100726c3 ---------------------------------------------------- //
	g_a1008e684[pRec->m_nStage42] = pRec;

	if (pRec->m_nStamp14 != g_w1008bea8)
	{
		// dl_Remove / dl_TieOff / dl_Insert, ltlink.h:100..145 --
		// 0x100726df..0x1007270f, prev at +0x00 and next at +0x04.
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

	// 0x10072748 -- vtable +0x8c, three dwords.
	g_pD3DDevice->lpVtbl->SetTexture(g_pD3DDevice, (uint8)pRec->m_nStage42,
	                                 pRec->m_pSurface0c);

	if (pRec->m_nAlphaRef16)
	{
		// 0x10072773 -- vtable +0x50, state 0x18 = ALPHAREF.
		g_pD3DDevice->lpVtbl->SetRenderState(g_pD3DDevice, 0x18,
		                                     pRec->m_nAlphaRef16);
	}

	// 0x10072785 -- the pair goes out BEFORE the counter here, the same
	// way round as big6d910.cpp's copy and the opposite way round from
	// big61a30.cpp's.
	g_a1008e664[pRec->m_nStage42].m_fInvW00 = pRec->m_fInvW04;
	g_a1008e664[pRec->m_nStage42].m_fInvH04 = pRec->m_fInvH08;

	g_n1008bd80++;

	if (pRec->m_nDirty44)
	{
		// 0x100727ad -- vtable +0xbc.
		pRec->m_pSurface0c->lpVtbl->SetLOD(pRec->m_pSurface0c, 0);
		pRec->m_nDirty44 = 0;
	}

L_Draw:
	// ------------------------------------------------------------------ //
	//  0x100727b9 -- scale the FIRST texture pair by stage zero's
	//  published inverse size.  0x1008e664 and 0x1008e668 are ELEMENT
	//  ZERO of the per-stage array 0x10072785 wrote with an index; the
	//  fold is the same one big61a30.cpp records at
	//  0x10061f29/0x10061f35.  icl rotated the loop, which is why the two
	//  `fstps` appear twice (0x100727ce and 0x100727ee).
	// ------------------------------------------------------------------ //
	pV    = pVerts;
	nLeft = nVerts;

	while (nLeft--)
	{
		pV->m_fU0 *= g_a1008e664[0].m_fInvW00;
		pV->m_fV0 *= g_a1008e664[0].m_fInvH04;
		pV++;
	}

	// -- 0x100727f4: vtable +0x64, six dwords.  6 = D3DPT_TRIANGLEFAN,
	//    0x2c4 = XYZRHW | DIFFUSE | SPECULAR | TEX2.  Unlike
	//    big6d910.cpp there is NO counter increment after it.
	g_pD3DDevice->lpVtbl->DrawPrimitive(g_pD3DDevice, 6, 0x2c4, pVerts,
	                                    nVerts, 0);

	// -- 0x10072830: `xor %eax,%eax ; inc %eax`, then a cross-jump INTO
	//    the shared epilogue at 0x1007263b.
	return 1;

L_ElseLOD:
	// -- 0x1007291e: the record is already bound to this stage, so only
	//    the LOD reset survives.  The same three statements as the tail of
	//    L_Bind; big61a30.cpp and big6d910.cpp record the same
	//    duplication.  pRec is spilled to -0x8(%ebp) across the call
	//    (0x1007293d/0x10072946).
	if (pRec->m_nDirty44)
	{
		pRec->m_pSurface0c->lpVtbl->SetLOD(pRec->m_pSurface0c, 0);
		pRec->m_nDirty44 = 0;
	}

	goto L_Draw;

L_MakeRec:
	// -- 0x10072968: `test %esi,%esi ; je 0x10072994` on the SAME list
	//    head that was walked above -- a third test of it.  The third
	//    argument is 1 when the list is non-empty and 0 when it is empty.
	if (pBlock->m_pHead0c)
	{
		pRec = sub_10039bd0(pBlock, nStage, 1);

		if (!pRec)
			goto L_Draw;

		pRec->m_pNext30              = pBlock->m_pHead0c->m_pNext30;
		pBlock->m_pHead0c->m_pNext30 = pRec;
	}
	else
	{
		pRec = sub_10039bd0(pBlock, nStage, 0);

		if (!pRec)
			goto L_Draw;
	}

	goto L_Bind;
}
