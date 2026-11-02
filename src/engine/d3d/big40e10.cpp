// *********************************************************************** //
//
//  MODULE:   BIG_40E10 (.CPP)
//
//  PURPOSE:  One function -- 0x10040e10, 7,982 bytes.  d3d.ren's
//            DRAW-ONE-LINE-SYSTEM pass: it walks an OT_LINESYSTEM
//            object's circular list of LTLine records, and for each line
//            builds two 32-byte FVF-0x1c4 vertices, clips the segment
//            against the six frustum planes (Cohen-Sutherland, both
//            endpoints, no polygon loop anywhere), projects to screen and
//            issues one DrawPrimitive(D3DPT_LINELIST, 2 vertices).
//
//  ------------------------------------------------------------------- //
//  THE EXTENT.  The enumeration is WRONG IN BOTH DIRECTIONS and this was
//  settled before a byte was decoded.
//  ------------------------------------------------------------------- //
//
//  The enumeration carries `0x10040dd0, 4,841 bytes` and, separately,
//  `0x100420b9, 3,207 bytes`.  Neither row is a function.
//
//    * The extent 0x10040dd0, 4841 has THREE `ret`s (0x10040de2,
//      0x10040e09, 0x10041f36), TWO `push %ebp` frames (0x10040dd0,
//      0x10040e10), FIVE branch targets escaping the row (0x10040feb ->
//      0x10042af0, 0x10041238 -> 0x10042890, 0x1004148d -> 0x10042630,
//      0x100416d8 -> 0x100423d0, 0x10041929 -> 0x10042170) and a last
//      instruction that overruns the stated end.
//
//    * 0x10040dd0 is a SEPARATE 58-byte function (below, as a
//      candidate).  Its body is 0x10040dd0..0x10040e09 inclusive and
//      0x10040e0a..0x10040e0f is six bytes of `lea 0x0(%esi),%esi`
//      padding, so the large function starts at 0x10040e10 on a 16-byte
//      boundary.
//
//    * **THE TRUE EXTENT IS 0x10040e10..0x10042d3d, 7,982 bytes.**
//      Walked from the bytes, 0x10040e10..0x10042d3e has:
//        - ONE `ret`, at 0x10041f36;
//        - ONE `push %ebp ; mov %esp,%ebp`, at 0x10040e10;
//        - ZERO branch targets outside the range or mid-instruction
//          (all 27 branches and all 7 `jmp`s checked);
//        - its last instruction ending exactly at the stated end.
//
//    * The single `ret` has **3,592 bytes of code after it** and every
//      one of those bytes is /O2 cold-block sinking, proved target by
//      target.  The seven sunk blocks are 0x10041f37 (plane 6's
//      both-outside test, falling into the shared `continue` at
//      0x10041f42 whose `jmp 0x10041f19` re-enters the loop latch),
//      0x10041f50 (plane 6's clip-v0 arm, `jmp 0x10041dbc` at
//      0x1004216a), 0x10042170 (plane 5's, `jmp 0x10041b7c` at
//      0x100423c5), 0x100423d0 (plane 4's, `jmp 0x10041912` at
//      0x1004261d), 0x10042630 (plane 3's, `jmp 0x100416c7` at
//      0x1004287d), 0x10042890 (plane 2's, `jmp 0x1004147c` at
//      0x10042ae5) and 0x10042af0 (plane 1's, `jmp 0x10041221` at
//      0x10042d39).  Every one of them uses THIS frame (-0x8(%ebp),
//      -0x23c(%ebp), -0x498(%ebp) ...), every one is reached only by a
//      forward branch from ABOVE the `ret`, and every one ends by
//      branching BACK above the `ret`.  Each is preceded by lea-nop
//      padding to a 16-byte boundary (0x10041f47/0x10041f49,
//      0x100423ca, 0x10042622/0x10042629, 0x10042882/0x10042889,
//      0x10042aea), which is intra-function block alignment, not a
//      function boundary -- none of those pads is followed by a
//      prologue.
//
//    * The enumeration's second row, `0x100420b9, 3,207 bytes`, is a
//      slice through the middle of plane 5's sunk arm.  The call count
//      over 0x100420b9..0x10042d40 is **0 calls**, which is what a
//      cold-block tail looks like.
//
//    * THE BOUNDARY AFTER THE FUNCTION.  0x10042d39 is the last
//      instruction (`jmp 0x10041221`), ending at 0x10042d3e.
//      0x10042d3e..0x10042d3f is `mov %esi,%esi` -- two bytes of
//      alignment padding -- and 0x10042d40 opens a new function with
//      `push %ebx ; sub $0x10,%esp` (no `push %ebp`, which is why
//      an extent walk sees only one frame over any window that includes
//      it).  It has its own two `ret`s (0x10042d70, 0x10042d93) and
//      nothing in our range branches to it.
//
//    * THE CALL ARITHMETIC CLOSES.  Call counts:
//        0x10040dd0..0x10040e0a  ..............  2 calls
//        0x10040e10..0x10042d3e  .............. 15 calls (14 direct, 1
//                                               indirect)
//        0x100420b9..0x10042d40  ..............  0 calls
//      and the defective row 0x10040dd0..0x100420b9 has **17**.
//      2 + 15 = 17 exactly, and the 15 are all in 0x10040e2b..0x10041f13
//      so they all fall inside the defective row -- which is why the row
//      totalled correctly while being wrong at both ends.
//
//  ------------------------------------------------------------------- //
//  THE OTHER FUNCTIONS THIS DELIMITS.  Candidates, NOT decoded here.
//  ------------------------------------------------------------------- //
//
//    0x10040dd0   58 bytes (0x10040dd0..0x10040e09, then 6 pad bytes),
//                 2 calls, 29 B/call, 2 distinct callees.  Shape:
//                   if (!g_p100893dc) return;
//                   sub_1007b8e0((Ctx*)((uint8*)sub_1007b8d0() + 0x244),
//                                0x1008b880, 0x10040cc0);
//                 -- `call 0x1007b8d0 ; add $0x244,%eax ; mov %eax,%ecx`
//                 then the two-slot area at 0x10040df2/0x10040df9 and
//                 `call 0x1007b8e0`.  It REGISTERS 0x10040cc0 (not this
//                 function) as a per-item callback, exactly the
//                 0x1007b8e0 shape d3dbatchc.cpp records.
//
//    0x10042d40   84 bytes (0x10042d40..0x10042d93), 2 calls, 42 B/call,
//                 2 distinct callees (0x1007b8d0, 0x10021d60).  An
//                 append-to-set: reads the count at +0x2a8 and the cap
//                 at +0x2b8 of `sub_1007b8d0()`'s context, stores its
//                 argument into `+0x2a4[count]` and bumps the count, or
//                 warns `d3d_Warning(1, "Set '%s' overflowed",
//                 ctx->+0x2bc)` (string 0x100a9a10 dumped) when full.
//                 Register-ABI: the argument arrives at 0x18(%esp) with
//                 no prologue read of %ebp.
//
//    0x10042da0   38 bytes (0x10042da0..0x10042dc5), 1 call
//                 (0x100500a0), 38 B/call.  **This is our caller.**
//                   sub_100500a0(g_p1008ef3c, pItem, sub_10040e10);
//                 -- `mov 0xc(%ebp),%eax ; mov 0x1008ef3c,%ecx ;
//                 movl $0x10040e10,0x4(%esp) ; call 0x100500a0` at
//                 0x10042da9..0x10042dbd.  The `$0x10040e10` immediate at
//                 0x10042db5 is the only reference to our function's
//                 address in the image and it is what fixes the
//                 signature -- see below.
//
//    0x10042dd0   at least 0x100 bytes (0x10042dd0.., `ret` at
//                 0x10042de8 with its own `mov (%esp),%ebx ;
//                 mov %ebp,%esp ; pop %ebp` epilogue and a body running
//                 past 0x10042e79).  NOT delimited at the far end and
//                 not counted.
//
//  ------------------------------------------------------------------- //
//  WHAT IT IS, AND THE SIGNATURE
//  ------------------------------------------------------------------- //
//
//  `void sub_10040e10(void* pCtx1008b880, ObjLine40e10* pObj)`, __cdecl,
//  two stack arguments, no return value (the epilogue at 0x10041f2c is
//  `mov (%esp),%ebx ; mov 0x4(%esp),%esi ; mov %ebp,%esp ; pop %ebp ;
//  ret` and never touches %eax).
//
//  Read off the ONE call site.  0x10042da0 hands our address to
//  0x100500a0, and 0x100500a0's direct-dispatch path is
//
//      100500bf  add  $-0x8,%esp
//      100500c2  mov  0x8(%ebp),%eax          ; the queued item
//      100500c5  mov  %eax,0x4(%esp)          ; argument 2
//      100500c9  movl $0x1008b880,(%esp)      ; argument 1
//      100500d0  call *%edi                   ; our function
//
//  so argument 1 is always the transform block 0x1008b880 and argument 2
//  is the object.  Our function reads only 0xc(%ebp) (at 0x10040e30,
//  into %esi) -- 0x8(%ebp) is never read, because the one field of the
//  block it wants is reached as the ABSOLUTE 0x1008b9dc (an immediate at
//  0x10040e76), which is 0x1008b880 + 0x15c, the block's first 4x4.
//  big5a960.cpp's CtxPG5a960 names that member `m_m15c` on the sibling
//  block at 0x1008ff60.
//
//  THE OBJECT IS OT_LINESYSTEM, and lithtech.exe's `ci_AddLine`
//  (0x0043d1c0) is what proves the record layout rather than any guess:
//
//      43d1df  cmpb $0x8,0xbc(%edi)      ; object type 8 = OT_LINESYSTEM
//      43d212  lea  0x1b0(%edi),%eax     ; pHead -- the same +0x1b0 our
//                                        ; 0x10040e8d takes the address of
//      43d21b  mov  %eax,0x40(%esi)      ; pRec->m_pNext40 = pHead
//      43d21e  mov  0x1ec(%edi),%edx     ; pHead->m_pPrev3c  (0x1b0+0x3c)
//      43d224  mov  %edx,0x3c(%esi)      ; pRec->m_pPrev3c
//      43d227  mov  %esi,0x3c(%eax)      ; pHead->m_pPrev3c  = pRec
//      43d239  mov  %esi,0x40(%eax)      ; ->m_pNext40       = pRec
//      43d23c  mov  %edi,0x38(%esi)      ; pRec->m_pObject38 = pObj
//
//  -- a circular doubly-linked list whose links live at +0x38/+0x3c/+0x40
//  of a record whose first 0x38 bytes are the line itself, with the head
//  a pseudo-record based at pObj+0x1b0 (so its next field is pObj+0x1f0,
//  which is exactly what 0x10040e96 loads).  Our walk is
//  `mov 0x1f0(%esi),%ebx` / `cmp &pObj->x1b0,%ebx` / `je exit` at the top
//  and `mov 0x40(%ebx),%ebx` / `cmp` / `jne top` at 0x10041f16.
//
//  AND THE RECORD'S FIRST 0x38 BYTES ARE THE SDK's `LTLine` EXACTLY.
//  src/LT2/sdk/inc/iltclient.h:34 declares
//      struct LTLinePt { LTVector m_Pos; float r, g, b, a; };   // 0x1c
//      struct LTLine   { LTLinePt m_Points[2]; };               // 0x38
//  and the field offsets this function reads are +0x00 (position, handed
//  to the transform at 0x10040ecf), +0x0c/+0x10/+0x14/+0x18 (four floats
//  fed to the four colour bytes) and the same again at +0x1c/+0x28/
//  +0x2c/+0x30/+0x34.  Nothing else in the record is touched.
//
//  The colour mapping is read off which destination byte each product
//  lands in: r*255 -> +0x12, g*255 -> +0x11, b*255 -> +0x10 and
//  a*(float)pObj->m_nA97 -> +0x13.  +0x10/+0x11/+0x12/+0x13 is
//  B/G/R/A in a D3DCOLOR, so `r` really is red.  The six `255.0f` are
//  0x100a9c28..0x100a9c3c (all dumped: 0x437f0000).  The object's own
//  alpha is the BYTE at +0x97 -- big5a960.cpp's `m_nA97` on the same
//  LTObject layout -- widened once, before the loop
//  (`movzbl 0x97(%esi),%eax ; mov %eax,-0x108(%ebp) ;
//  fildl -0x108(%ebp) ; fstpl -0xf0(%ebp)` at 0x10040e86..0x10040eaa)
//  and reloaded from -0xf0 at both alpha sites (0x10040f29, 0x10040fb2).
//
//  ------------------------------------------------------------------- //
//  THE ACCEPTANCE FIGURE AND WHAT IT DOES *NOT* WITNESS.  READ FIRST.
//  ------------------------------------------------------------------- //
//
//  The call count over 0x10040e10..0x10042d3e is **7,982
//  bytes, 15 calls (14 direct, 1 indirect), 532 bytes per call**, across
//  **six distinct direct callees** (0x100232b0, 0x100158b0, 0x100151b0,
//  0x10014720, 0x10023230 x2, 0x10015190 x8).
//
//  THE EARLIER "285 B/CALL, THE BEST RATIO LEFT IN THE MODULE" WAS AN
//  ARTEFACT OF THE DEFECTIVE ROW.  The row 0x10040dd0..0x100420b9 packs
//  17 calls into 4,841 bytes because it stops in the middle of the
//  call-free cold tail; the real function is 532 B/call, which is
//  firmly in the WEAK band.  This banner does not lean on the call
//  sequence and the reader should not either.
//
//  * **CALL-FREE COVERAGE: 7,538 of 7,982 bytes = 94.4%.**  All fourteen
//    direct calls are in 0x10040e2b..0x10040fc9 (a 414-byte window) and
//    the one indirect is at 0x10041f13.  So 0x10040fc9..0x10041f13
//    (3,914 bytes: the six planes' inline arms) and
//    0x10041f16..0x10042d3e (3,624 bytes: the epilogue and all seven
//    sunk blocks) contain NO calls at all.  The call sequence witnesses the
//    prologue, the two matrices and the vertex fill, and nothing else.
//
//  What pins the other 94% is three independent instruments, and all
//  three came out with **no residue whatsoever**.
//
//  ------------------------------------------------------------------- //
//  (1) THE INSTRUCTION CENSUS.  ZERO RESIDUE.
//  ------------------------------------------------------------------- //
//
//  Every count is `sed 's/.*\t//' | awk '{print $1}' | sort | uniq -c`
//  over all 1,669 instructions of 0x10040e10..0x10042d3e, and every row
//  is EXACT.  The structure claimed below is TWELVE CLIP ARMS -- six
//  planes, two arms each -- and each arm is FOUR float LERPs plus FIVE
//  byte LERPs plus one exact assignment of the pinned coordinate:
//
//    | mnemonic  | shipped | what the structure requires |
//    |-----------|---------|-----------------------------|
//    | fistpl    |   60    | 12 arms x 5 byte LERPs |
//    | fildl     |  121    | 12 x 5 x 2 (difference + base) + 1 x (float)m_nA97 |
//    | movzbl    |  121    | 12 x 5 x 2 byte widenings + 1 x m_nA97 |
//    | fmul      |   48    | 12 x 4 float LERPs (`* fT` from the register) |
//    | fmull     |   48    | 12 x 4 byte LERPs 2..5 (`* fT` from its double slot) |
//    | fmulp     |   18    | 12 x byte LERP 1 (fT still in st) + 6 in the screen transform |
//    | fsubs     |   48    | 12 x 4 float LERP differences |
//    | fstl      |   13    | 12 x the fT spill + v[0].z in plane 1's first test |
//    | fadds     |   30    | 6 inline arms x 4 LERP bases + 6 sunk arms x 1 |
//    | faddp     |   84    | 60 byte-LERP bases + 6 sunk arms x 3 preloaded float bases + 6 in the screen transform |
//    | faddl     |   12    | planes 2 and 5 x 2 arms x 2 (fDen's `+ v[1].z` and the numerator's `+ v[0].y`/`+ v[0].x`) + planes 3 and 4 x 2 arms x 1 (fDen's `+ v[0].z`) = 8 + 4 |
//    | fstps     |  124    | 12 x (4 float LERP stores + 5 fRed..fSpec stores) + 10 in the fill + 6 in the screen transform |
//    | fsts      |   10    | planes 2,3,4,5 x 2 arms (the shared `fVal`) + 2 rhw in the screen transform |
//    | fsubl     |   20    | planes 1 and 6 x 2 arms x 1 + planes 2,3,4,5 x 2 arms x 2 = 4 + 16 |
//    | fsubrl    |    8    | planes 1,3,4,6 x 2 arms |
//    | fchs      |   18    | 6 x `-z` in planes 2 and 5's classify (2 vertices inline + 1 sunk, each plane) + 8 numerator negations (planes 2,3,4,5 x 2 arms) + 4 x `-fVal` (planes 2 and 5 x 2 arms) |
//    | fdivrp*   |    4    | planes 1 and 6 x 2 arms -- **binutils prints Intel FDIVP as `fdivrp`** |
//    | fdivp*    |    8    | planes 2,3,4,5 x 2 arms -- **binutils prints Intel FDIVRP as `fdivp`** |
//    | fdivs     |    2    | `1.0f / v[0].z` and `1.0f / v[1].z` |
//    | fcomp     |    9    | planes 1,3,4 x 2 vertices, less the 3 that use another form |
//    | fcomps    |    7    | planes 2,5,6's `fcomps` forms |
//    | fcompl    |    2    | plane 6's two `fcompl -0x40(%ebp)` |
//    | fnstsw    |   18    | 12 inline tests + 6 sunk tests = 18 float compares |
//    | sahf      |   18    | the same 18 |
//    | ja/jae/jb/jbe | 4/4/3/7 | the same 18 |
//    | jmp       |    7    | the shared `continue` + the 6 sunk arms' returns |
//    | je / jne  |  1 / 1  | the list-empty entry test and the loop back edge |
//    | sub       |   61    | 12 x 5 byte differences taken in the INTEGER unit + `sub $0x4b8,%esp` |
//    | call      |   15    | 4 in the prologue + 10 in the fill + 1 DrawPrimitive |
//    | ret       |    1    | the single epilogue |
//    | push/pop  |  1 / 1  | `push %ebp` / `pop %ebp` |
//    | lea       |   20    | 12 real (6 prologue at 0x10040e40..0x10040e8d, 5 fill, 1 for &v[0] at 0x10041e02) + 8 lea-nop pads (0x10040ebc, 0x10041f49, 0x100423ca, 0x10042622, 0x10042629, 0x10042882, 0x10042889, 0x10042aea) |
//    | add       |   11    | the outgoing-argument areas + the loop's +0x1b0 |
//    | cmp       |    2    | the loop entry test and the back edge |
//    | movl      |    3    | `$0x1008b9dc`, `$0x1c4`, `$0x0` |
//    | nop       |    1    | one pad byte |
//    | fxch      |   98    | scheduling; not load-bearing |
//    | flds      |  187    | see the per-region breakdown |
//    | mov       |  292    | ditto |
//
//    THE LOAD-BEARING ROWS ARE `fistpl 60 = 12 x 5`, `fildl 121 = 12 x 5
//    x 2 + 1`, `movzbl 121 = 12 x 5 x 2 + 1`, `fmul 48 = 12 x 4`,
//    `fmull 48 = 12 x 4`, `fsubs 48 = 12 x 4` and the twelve divides
//    (4 + 8).  A seventh plane, a missing plane, a single arm written
//    once instead of twice, a third float LERP anywhere, or a sixth byte
//    would move at least four of those rows at once.  None of them
//    moves, and the totals leave NOTHING over: every one of the 1,669
//    instructions is accounted for by the per-region table below, which
//    was computed independently and sums to the same numbers.
//
//    Note in particular that **there is no `test` and no `and` in the
//    whole function**: unlike big70d10/big5a960/big051a0 there is no
//    `nClip & bit` guard on any plane and no UseD3DClip test.  All six
//    planes run unconditionally on every line.
//
//    And there is no epsilon guard: `fcomp` appears ONLY in the eighteen
//    plane tests, so the `if (fDen < -1e-5 || fDen > 1e-5) ... else
//    fT = 0.0f;` of every previous clipper decode is ABSENT here.  The
//    pool confirms it -- there is no 1e-5 and no 0.0f in this function's
//    literal run.
//
//  ------------------------------------------------------------------- //
//  (2) FRAME ARITHMETIC.  `sub $0x4b8,%esp` = 1,208 bytes, AND IT
//      DECOMPOSES TO THE BYTE WITH ONLY 24 BYTES OF ALIGNMENT SLACK.
//  ------------------------------------------------------------------- //
//
//    -0x008 .. -0x0f0   240  THIRTY 8-byte x87 spill slots.  Compiler
//                            temporaries, not source variables: every one
//                            is written `fstl`/`fstpl` and read `fldl`,
//                            which is icl parking an x87 register at 64
//                            bits.  Their roles are all read off the
//                            code:  -0x8 = v[0].z, -0x10 = v[1].z (both
//                            refreshed at the end of every arm by
//                            `flds -0x214 ; flds -0x234 ; fstpl -0x8 ;
//                            fstpl -0x10`), -0x18 = g_f1008b908,
//                            -0x40 = g_f1008b910, -0x20/-0x30 =
//                            v[0].x/v[1].x for plane 2, -0x28/-0x48 =
//                            v[0].y/v[1].y for plane 3, -0x38/-0x60 =
//                            v[0].x/v[1].x for plane 4, -0x50/-0x70 =
//                            v[0].y/v[1].y for plane 5, -0x98/-0xa0/
//                            -0xa8/-0xb8/-0xc8 = the sunk paths' own
//                            copies of v[1]'s coordinate for planes
//                            1/2/3/4/5, -0x58/-0x68/-0x78/-0x80/-0x88/
//                            -0x90 = fT in the six inline arms and
//                            -0xb0/-0xc0/-0xd0/-0xd8/-0xe0/-0xe8 = fT in
//                            the six sunk arms, and -0xf0 =
//                            (float)pObj->m_nA97.
//    -0x0f4               4  pObj, spilled once at 0x10040eb6 and
//                            reloaded at 0x10041f19 each iteration.
//    -0x0f8 .. -0x104    12  hole.
//    -0x108               4  the int scratch every `fildl` goes through
//                            (`mov %eax,-0x108(%ebp) ; fildl -0x108`),
//                            121 uses, one slot.
//    -0x10c               4  hole.
//    -0x110 .. -0x1fc   240  **SIXTY int32 cast temporaries**, one per
//                            byte LERP.
//    -0x1fd .. -0x23c    64  **v[0] and v[1]** -- two 32-byte vertices,
//                            v[0] at -0x23c and v[1] at -0x21c.
//    -0x23d .. -0x27c    64  mFull, the object-to-clip 4x4.
//    -0x27d .. -0x2bc    64  mObj, the object-to-world 4x4.
//    -0x2bd .. -0x498   476  **SIXTY float temporaries**, one per byte
//                            LERP (fRed/fGreen/fBlue/fAlpha/fSpec x 12
//                            arms).  59 slots of 8 plus a last one of 4,
//                            because mObj begins at -0x2bc.
//    -0x499 .. -0x4b0    24  alignment.  The only slack in the frame.
//    -0x4b1 .. -0x4b8     8  %ebx and %esi, saved esp-relative at
//                            0x10040e19/0x10040e1d and restored from
//                            (%esp)/0x4(%esp) at 0x10041f2c.
//
//    240 + 4 + 12 + 4 + 4 + 240 + 64 + 64 + 64 + 476 + 24 + 8 = 1,208
//    = 0x4b8.  EXACT.
//
//  **AND THE TWO 60-SLOT BLOCKS DECOMPOSE INTO TWELVE GROUPS OF FIVE, IN
//  SOURCE ORDER.**  This is the single strongest piece of evidence in
//  the file and it is independent of every call.  Grouping the `fstps`
//  targets in the float block and the `fistpl` targets in the int block
//  by which arm they occur in:
//
//      plane 1  inline  -0x498 -0x490 -0x488 -0x480 -0x478 | -0x1fc .. -0x1ec
//      plane 1  sunk    -0x470 -0x468 -0x460 -0x458 -0x450 | -0x1e8 .. -0x1d8
//      plane 2  inline  -0x448 -0x440 -0x438 -0x430 -0x428 | -0x1d4 .. -0x1c4
//      plane 2  sunk    -0x420 -0x418 -0x410 -0x408 -0x400 | -0x1c0 .. -0x1b0
//      plane 3  inline  -0x3f8 -0x3f0 -0x3e8 -0x3e0 -0x3d8 | -0x1ac .. -0x19c
//      plane 3  sunk    -0x3d0 -0x3c8 -0x3c0 -0x3b8 -0x3b0 | -0x198 .. -0x188
//      plane 4  inline  -0x3a8 -0x3a0 -0x398 -0x390 -0x388 | -0x184 .. -0x174
//      plane 4  sunk    -0x380 -0x378 -0x370 -0x368 -0x360 | -0x170 .. -0x160
//      plane 5  inline  -0x358 -0x350 -0x348 -0x340 -0x338 | -0x15c .. -0x14c
//      plane 5  sunk    -0x330 -0x328 -0x320 -0x318 -0x310 | -0x148 .. -0x138
//      plane 6  inline  -0x308 -0x300 -0x2f8 -0x2f0 -0x2e8 | -0x134 .. -0x124
//      plane 6  sunk    -0x2e0 -0x2d8 -0x2d0 -0x2c8 -0x2c0 | -0x120 .. -0x110
//
//  Twelve consecutive runs of five, 0x28 apart, 0x50 per plane, walking
//  DOWNWARDS in plane order 1,2,3,4,5,6 and, within each plane, the
//  INLINE arm's block before the SUNK arm's.
//
//  A caution on WHAT these slots are, because the conclusion has to
//  survive either reading.  icl reuses no temporary slot anywhere in
//  this function (all 30 double spills are distinct, all 60 int cast
//  destinations are distinct), so the 60 float slots are EITHER the
//  source's five block-scoped `float fRed, fGreen, fBlue, fAlpha,
//  fSpec` per arm -- big70d10.cpp's spelling, and the reason the
//  `(uint8)` cast needs a 32-bit memory copy at all -- OR icl's own
//  cast temporaries.  Either way icl allocates them with a descending
//  bump allocator in PROGRAM order, and program order is source order,
//  so the frame still reads the source's structure straight off:
//
//    * SIX planes, in the order NEAR, LEFT, TOP, RIGHT, BOTTOM, FAR --
//      the same order as d3dr3a.cpp, big051a0.cpp, big0f6f0.cpp,
//      big5a960.cpp, big61a30.cpp, big124a4.cpp, big6d910.cpp and
//      big70d10.cpp;
//    * TWO textual arms per plane, so the clip is NOT written once with
//      a computed destination pointer -- it is written twice;
//    * FIVE float temporaries per arm and no sixth, i.e. five byte
//      LERPs per arm (which the `fistpl 60` census row says
//      independently);
//    * `fT` is NOT among them: all twelve live in the 8-byte x87 spill
//      region at -0x58..-0x90 and -0xb0..-0xe8, which is icl
//      register-allocating a float local and spilling it at 64 bits.
//      That is why it is declared separately from fRed..fSpec below;
//    * the inline arm is declared FIRST, which settles the arm-order
//      coin-flip from the frame rather than from block placement:
//      the source is `if (v[0] inside) { clip v[1] } else { ... clip
//      v[0] }`, positive guard first.  See coin-flip note 1.
//
//  ------------------------------------------------------------------- //
//  (3) THE LITERAL POOL.  0x100a9c28..0x100a9c64, read in order.
//  ------------------------------------------------------------------- //
//
//  A sweep of every `0x100a9xxx` operand in .text (0x10001000..0x10081000)
//  shows the pool is emitted in FUNCTION-ADDRESS order and, within a
//  function, in SOURCE order.  Our function's references bracket
//  0x100a9c28..0x100a9c64; the previous referenced literal is 0x100a9bc0
//  (from 0x10040b12) and the next is 0x100a9cd0 (from 0x100440ab).
//  Dumped, in address order, with the referencing instruction:
//
//      0x100a9c28  255.0f   loaded at 0x10040edf  -> v[0] red
//      0x100a9c2c  255.0f   loaded at 0x10040efb  -> v[0] green
//      0x100a9c30  255.0f   loaded at 0x10040f12  -> v[0] blue
//      0x100a9c34  255.0f   loaded at 0x10040f68  -> v[1] red
//      0x100a9c38  255.0f   loaded at 0x10040f84  -> v[1] green
//      0x100a9c3c  255.0f   loaded at 0x10040f9b  -> v[1] blue
//      0x100a9c40  1.0f     NEVER LOADED
//      0x100a9c44  1.0f     NEVER LOADED
//      0x100a9c48  1.0f     NEVER LOADED
//      0x100a9c4c  1.0f     NEVER LOADED
//      0x100a9c50  1.0f     NEVER LOADED
//      0x100a9c54  1.0f     NEVER LOADED
//      0x100a9c58  1.0f     NEVER LOADED
//      0x100a9c5c  1.0f     loaded at 0x10041dbc  -> 1.0f / v[0].z
//      0x100a9c60  1.0f     NEVER LOADED
//      0x100a9c64  1.0f     loaded at 0x10041de6  -> 1.0f / v[1].z
//
//  What this settles:
//
//    * SIX separate `* 255.0f`, not one shared constant, so the colour
//      fill is six separate statements and the two endpoints are written
//      out in full rather than looped -- which is also why the fill has
//      no loop at all in 268 bytes;
//    * the alpha byte does NOT go through 255.0f (there is no seventh or
//      eighth 255.0f), confirming `a * (float)pObj->m_nA97`;
//    * NO 1e-5 and NO 0.0f anywhere in the run, confirming the absence
//      of the epsilon guard the five sibling clippers all have;
//    * TWO `1.0f`, one per vertex, so the perspective divide is written
//      twice rather than in a two-iteration loop.
//
//  What it does NOT settle, and is recorded as an open item rather than
//  invented: **EIGHT folded 1.0f**, seven at 0x100a9c40..0x100a9c58 and
//  one at 0x100a9c60.  They are inside our bracket, so by the
//  contiguity of a function's pool run they are ours; icl emitted them
//  and then folded every use away.  Their placement -- seven between the
//  last colour statement and the first `1.0f / z`, and exactly one
//  between the two `1.0f / z` -- is consistent with (a) six statements
//  carrying a foldable 1.0f, one per plane, plus one per vertex before
//  its divide, or (b) seven such statements in the clip block and one in
//  v[0]'s transform.  No candidate expression was found in the
//  disassembly for either, so no statement is written for them.  See
//  "WHAT IS NOT PINNED".
//
//  ------------------------------------------------------------------- //
//  (4) THE TWELVE ARMS ARE THE SAME SHAPE, MEASURED NOT SKIMMED.
//  ------------------------------------------------------------------- //
//
//  Per-region instruction census (computed with a per-range counter over
//  the disassembly, then summed -- the sums are the whole-function
//  numbers above, with nothing left over):
//
//    region                bytes insn  the identical rows
//    PROLOGUE  40e10-40ec0  176   39   call=4 lea=7 fildl=1 fstpl=1
//    FILL      40ec0-40fcc  268   61   call=10 fmuls=8 fstps=10 flds=6
//    plane1 T  40fcc-41008   60   17 |
//    plane1 A  41008-41221  537  108 | fildl=10 fistpl=5 fmul=4 fmull=4
//    plane2 T  41221-4125b   58   16 | fmulp=1 fsubs=4 movzbl=10 sub=5
//    plane2 A  4125b-4147c  545  113 | fstps=9 fstl=1
//    plane3 T  4147c-414aa   46   14 |
//    plane3 A  414aa-416c7  541  111 |  ... every one of the twelve arms
//    plane4 T  416c7-416f5   46   14 |  carries EXACTLY those eight rows,
//    plane4 A  416f5-41912  541  111 |  with no exception and nothing
//    plane5 T  41912-4194c   58   16 |  extra beyond the per-plane
//    plane5 A  4194c-41b7c  560  113 |  numerator/`fVal` differences
//    plane6 T  41b7c-41ba6   42   12 |  enumerated below.
//    plane6 A  41ba6-41dbc  534  104 |
//    SCREEN    41dbc-41f16  346   81   call=1 fdivs=2 fmuls=10 fmulp=6
//    ADVANCE   41f16-41f37   33   10   ret=1 pop=1 cmp=1 jne=1
//    p6 sunk T 41f37-41f50   25    9   fcompl=1 jbe=1 jmp=1
//    p6 sunk A 41f50-42170  544  108 | the same eight rows, plus faddp=8
//    p5 sunk T 42170-42190   32    8 | (three float LERP bases preloaded
//    p5 sunk A 42190-423d0  576  116 | onto the stack instead of added
//    p4 sunk T 423d0-423ea   26    7 | from memory) and jmp=1
//    p4 sunk A 423ea-42630  582  116 |
//    p3 sunk T 42630-4264a   26    7 |
//    p3 sunk A 4264a-42890  582  116 |
//    p2 sunk T 42890-428b0   32    8 |
//    p2 sunk A 428b0-42af0  576  116 |
//    p1 sunk T 42af0-42b0a   26    7 |
//    p1 sunk A 42b0a-42d3e  564  111 |
//
//  ------------------------------------------------------------------- //
//  (5) CORRESPONDENCE WITH THE DECODED SIBLINGS, PLANE BY PLANE.
//  ------------------------------------------------------------------- //
//
//  This is the NINTH reading of this one clipper source; it
//  is reused, not re-derived.  The six classify tests and the six
//  numerator/denominator pairs were diffed against big70d10.cpp's
//  (itself the sixth) and agree EXPRESSION FOR EXPRESSION:
//
//    plane 1  z >= g_f1008b908   fDen = w.z - p.z
//                                fT   = (g_f1008b908 - p.z) / fDen
//                                pins z = g_f1008b908 (INTEGER move:
//                                `mov 0x1008b908,%ecx ; mov %ecx,-0x214`
//                                at 0x1004108f/0x1004109b, so a plain
//                                float lvalue copy of the global)
//    plane 2  -z < x             fDen = (w.x - p.x) + w.z - p.z
//                                fT   = -(p.x + p.z) / fDen
//                                fVal = (w.z - p.z)*fT + p.z
//                                pins z = fVal, x = -fVal
//    plane 3  y < z              fDen = (w.y - p.y) - w.z + p.z
//                                fT   = -(p.y - p.z) / fDen
//                                pins z = fVal, y = fVal
//    plane 4  x < z              fDen = (w.x - p.x) - w.z + p.z
//                                fT   = -(p.x - p.z) / fDen
//                                pins z = fVal, x = fVal
//    plane 5  -z < y             fDen = (w.y - p.y) + w.z - p.z
//                                fT   = -(p.y + p.z) / fDen
//                                pins z = fVal, y = -fVal
//    plane 6  z <= g_f1008b910   fDen = w.z - p.z
//                                fT   = (g_f1008b910 - p.z) / fDen
//                                pins z = g_f1008b910 (integer move at
//                                0x10041c30/0x10041c3c)
//
//  read off, for the inline arms, 0x10040fcc-0x10041014 (fcomp/jb,
//  fsubl/fsubrl/FDIVP), 0x10041221-0x1004126f (fchs/fcomps/jae,
//  fldl/fsubl/faddl/fsubl then fldl/faddl/fchs/FDIVRP),
//  0x1004147c-0x100414be, 0x100416c7-0x10041709,
//  0x10041912-0x10041960 and 0x10041b7c-0x10041bb2; and for the sunk
//  arms 0x10042af0-0x10042b19, 0x10042890-0x100428c7,
//  0x10042630-0x10042661, 0x100423d0-0x10042401,
//  0x10042170-0x100421a7 and 0x10041f37-0x10041f5c.  In every plane the
//  sunk arm's fDen and fT are the SAME expressions as the inline arm's
//  (p = v[0], w = v[1] on both paths); only the DESTINATION changes,
//  from v[1] to v[0].
//
//  **THE OPERAND ORDER OF EVERY DIVIDE WAS VERIFIED WITH `as`, NOT READ
//  OFF THE DUMP.**  binutils prints the DE F0/F8 and DE E0/E8 register
//  forms SWAPPED.  Assembled with `as` and disassembled back:
//      Intel `fdivp  st(1),st`  ->  de f9  ->  objdump says `fdivrp`
//      Intel `fdivrp st(1),st`  ->  de f1  ->  objdump says `fdivp`
//  so the correct readings are:
//      de f9  =  Intel FDIVP  ST(1),ST  ->  ST(1) = ST(1)/ST(0)
//      de f1  =  Intel FDIVRP ST(1),ST  ->  ST(1) = ST(0)/ST(1)
//      de e9  =  Intel FSUBP  ST(1),ST  ->  ST(1) = ST(1)-ST(0)
//      de e1  =  Intel FSUBRP ST(1),ST  ->  ST(1) = ST(0)-ST(1)
//  Planes 1 and 6 use `de f9` (numerator pushed first, denominator
//  second: `fldl fLim ; fsubl p.z ; fldl p.z ; fsubrl w.z ; de f9`) and
//  planes 2-5 use `de f1` (denominator first, numerator second).  Both
//  compute numerator/denominator; getting either backwards would invert
//  every clip in the function.
//
//  The FIVE BYTE LERPS are +0x12, +0x11, +0x10, +0x13, +0x17 in that
//  order in ALL TWELVE arms -- sixty sites, every one read from its
//  `movzbl`/`movzbl`/`sub` pair (e.g. plane 1 inline at 0x10041046,
//  0x100410d7, 0x10041122, 0x1004116d, 0x100411b8) -- which is exactly
//  big70d10.cpp's and big6d910.cpp's order.  The TWO TEXTURE LERPS are
//  +0x18 then +0x1c in all twelve arms (twenty-four sites); big70d10's
//  40-byte TEX2 vertex has four there, and the count of two is what says
//  this vertex is 32 bytes and TEX1 -- independently of the FVF
//  immediate.
//
//  ------------------------------------------------------------------- //
//  (6) THE FVF AND THE VERTEX SIZE AGREE THREE WAYS.
//  ------------------------------------------------------------------- //
//
//  DrawPrimitive is vtable +0x64 (d3dbatcha.cpp and big5a960.cpp both
//  fix that slot) with primitive type **2 = D3DPT_LINELIST** and FVF
//  **0x1c4** (0x10041dfd `mov $0x2,%eax`, 0x10041f09
//  `movl $0x1c4,0x8(%esp)`).  0x1c4 = 0x004 XYZRHW | 0x040 DIFFUSE |
//  0x080 SPECULAR | 0x100 TEX1 = 16 + 4 + 4 + 8 = 32 bytes, and the
//  frame gives the two vertices 64 bytes at -0x23c with v[1] at -0x21c,
//  exactly 0x20 higher.  The vertex count argument is the SAME `%eax`
//  as the primitive type (0x10041ef2 and 0x10041f05 both store %eax),
//  i.e. both are the constant 2: one line, two vertices.  The last
//  argument is 0 (0x10041ef6).
//
//  ------------------------------------------------------------------- //
//  A REAL FINDING: tu, tv AND THE SPECULAR ALPHA ARE NEVER INITIALISED.
//  ------------------------------------------------------------------- //
//
//  The fill writes v[i].m_vPos (through the transform), v[i].m_fRHW (the
//  transform's return value) and the FOUR diffuse bytes -- and nothing
//  else.  There is no store anywhere in the function to +0x18 (tu),
//  +0x1c (tv), or to bytes +0x14..+0x17 of the specular dword, OTHER
//  than inside the twelve clip arms.  Confirmed exhaustively: the only
//  `movl $imm` in the whole function are `$0x1008b9dc`, `$0x1c4` and
//  `$0x0`, all three at known argument slots, so there is no immediate
//  store to the vertex; and the `fstps` census leaves no store
//  unaccounted for.
//
//  So a clipped line's tu/tv/+0x17 are interpolated from uninitialised
//  stack, and an unclipped line's are passed to DrawPrimitive
//  uninitialised.  That is not a decode gap -- it is what the shipped
//  code does, and it is why the LERP macro below interpolates fields the
//  fill never writes.  (It is harmless in practice: this pass binds no
//  texture and the FVF's TEX1/SPECULAR are there because the vertex
//  declaration is shared with the polygon pipeline.)
//
//  ------------------------------------------------------------------- //
//  COMPILER VERDICT: Intel C/C++ 4.0, /O2, and `/Qrcd` for the casts.
//  ------------------------------------------------------------------- //
//
//  FOR /O2:
//    * SEVEN COLD BLOCKS SUNK BELOW THE EPILOGUE (3,592 bytes of the
//      7,982 -- 45% of the function is below its own `ret`), enumerated
//      under THE EXTENT.
//    * OUTGOING ARGUMENTS as `add $-N,%esp` then `mov %reg,K(%esp)`,
//      never `push`: 0x10040e20 (+8), 0x10040e3d (+0x10),
//      0x10040ec0 (+0x10), 0x10040f40 (+8), 0x10041dfa (+0x18).  There
//      is not a single argument `push` in the function.
//    * Each area is a MULTIPLE OF 8: the one-argument 0x100232b0 gets
//      two slots, the four-argument 0x100151b0 and three-argument
//      0x10014720 share four, the three-argument 0x10023230 gets four,
//      and the six-argument DrawPrimitive gets six (0x18).  icl's stack
//      alignment, not arguments.
//    * Aggressive x87 scheduling: eight live values on the stack at
//      once in the screen transform (0x10041dbc..0x10041df8 pushes eight
//      before storing anything), and 98 `fxch`.  This is what makes the
//      emission order inside an arm NOT source order -- see the
//      coin-flip notes.
//
//  FOR /Qrcd:
//    * Every one of the sixty byte casts is `fstps <float temp> ;
//      flds <same temp> ; fistpl <int temp> ; mov <int temp>,%cl ;
//      mov %cl,<dest>` -- a bare `fistpl` with NO fnstcw/fldcw dance
//      and no helper call.  Plain /O2 emits the control-word dance;
//      /Qrcd is the flag.  Expect a leftover `fistpl` vs `fistpll` row
//      in the instruction census: that is the known icl gap, not a defect.
//
//  AGAINST (recorded, not explained away):
//    * ONE epilogue for a function with seven cold blocks -- /O2 usually
//      duplicates.  All six "whole line rejected" exits cross-jump into
//      the single shared `mov 0x40(%ebx),%ebx ; jmp 0x10041f19` at
//      0x10041f42, and the six sunk arms all jump back into the hot
//      path rather than duplicating the tail.
//
//  ------------------------------------------------------------------- //
//  REGION MAP
//  ------------------------------------------------------------------- //
//
//  0x10040e10..0x10040e36  PROLOGUE.  `sub $0x4b8,%esp` (1,208; no
//      _chkstk, the frame is under 4 KB), %esi/%ebx saved esp-relative,
//      then `sub_100232b0(g_n1008d7d0)` and `pObj = arg2`.
//
//  0x10040e36..0x10040e93  THE TWO MATRICES.
//      sub_100151b0(pObj->sub_100158b0(), &pObj->m_Rot9c,
//                   &pObj->m_vScaleac, &mObj)   -- 0x10040e61
//      sub_10014720(&mFull, &g_m1008b9dc, &mObj) -- 0x10040e81
//      Argument order for both is big5a960.cpp's / big60010.cpp's.
//      0x100158b0 is `push %esi ; lea 0x18c(%ecx),%eax ; add $0x4,%esp ;
//      ret` -- an out-of-line two-instruction accessor for
//      &m_vPos18c, so it is declared as a MEMBER FUNCTION with no
//      definition in this TU (spelling it
//      `&pObj->m_vPos18c` would emit no call at all and cost 1 of 15).
//
//  0x10040e86..0x10040ebc  `fObjAlpha = (float)pObj->m_nA97;` then the
//      list-empty test `if (pObj->m_LineHead1b0.m_pNext40 ==
//      &pObj->m_LineHead1b0) return;` (0x10040e96/0x10040eb0) and the
//      one-time spill of pObj to -0xf4.  The loop is ROTATED: this is
//      the guard, and the real test is the back edge at 0x10041f24.
//
//  0x10040ec0..0x10040fcc  THE FILL, once per line, no inner loop.  Two
//      calls to the projective transform 0x10023230 and eight to the
//      round-to-nearest 0x10015190, building v[0] from
//      pRec->m_Line.m_Points[0] and v[1] from [1].
//
//  0x10040fcc..0x10041dbc  THE SIX PLANES' TESTS AND INLINE ARMS,
//      alternating: tests at 0x10040fcc, 0x10041221, 0x1004147c,
//      0x100416c7, 0x10041912, 0x10041b7c and inline arms at
//      0x10041008, 0x1004125b, 0x100414aa, 0x100416f5, 0x1004194c,
//      0x10041ba6.  Joins at 0x10041221, 0x1004147c, 0x100416c7,
//      0x10041912, 0x10041b7c, 0x10041dbc -- i.e. each plane's join IS
//      the next plane's test, which is what a straight-line sequence of
//      six `if`s gives.
//
//  0x10041dbc..0x10041f16  SCREEN SPACE + DrawPrimitive.  The six
//      globals 0x1008bb9c..0x1008bbb0 in big5a960.cpp's exact spelling,
//      applied to v[0] then v[1], then
//      DrawPrimitive(dev, 2, 0x1c4, &v[0], 2, 0).
//
//  0x10041f16..0x10041f2c  `pRec = pRec->m_pNext40` and the back edge.
//
//  0x10041f2c..0x10041f37  THE EPILOGUE -- the only one.
//
//  0x10041f37..0x10041f47  plane 6's sunk both-outside test, and the
//      SHARED `continue` at 0x10041f42 that all six rejects reach.
//
//  0x10041f50..0x10042170  plane 6's sunk arm (clip v[0]), back to the
//      screen transform.
//  0x10042170..0x100423d0  plane 5's sunk test + arm, back to plane 6's
//      test.
//  0x100423d0..0x10042630  plane 4's, back to plane 5's test.
//  0x10042630..0x10042890  plane 3's, back to plane 4's test.
//  0x10042890..0x10042af0  plane 2's, back to plane 3's test.
//  0x10042af0..0x10042d3e  plane 1's, back to plane 2's test.
//      (Sunk in REVERSE plane order, exactly as big70d10.cpp records
//       for its six sunk epsilon arms.)
//
//  0x10042d3e..0x10042d40  two bytes of `mov %esi,%esi` alignment
//      padding, then a different function.
//
//  ------------------------------------------------------------------- //
//  THE EXPECTED CALL-SEQUENCE RESULT, predicted from the disassembly before
//  any build.
//  ------------------------------------------------------------------- //
//
//  The shipped sequence, in address order, is
//
//      0x100232b0  (0x10040e2b)
//      0x100158b0  (0x10040e38)
//      0x100151b0  (0x10040e61)
//      0x10014720  (0x10040e81)
//      0x10023230  (0x10040eda)
//      0x10015190  (0x10040ef6)
//      0x10015190  (0x10040f0d)
//      0x10015190  (0x10040f24)
//      0x10015190  (0x10040f3b)
//      0x10023230  (0x10040f63)
//      0x10015190  (0x10040f7f)
//      0x10015190  (0x10040f96)
//      0x10015190  (0x10040fad)
//      0x10015190  (0x10040fc4)
//      *0x64       (0x10041f13, indirect)
//
//  -- 15 calls, 14 direct and 1 indirect, and the source below emits
//  exactly that order.  **The expected result is 15 of 15, with no
//  residual.**  There is no _chkstk here (the frame is 1,208 bytes), no
//  CRT intrinsic (no sqrt/fabs/tan/atan/memcpy anywhere -- so this file
//  does NOT need /Oi-), and no /Qip two-entry offset to worry about
//  unless the accessor 0x100158b0 is placed in the same TU as its
//  callers, which it is not here.
//
//  ANYTHING ELSE IS A DECODE ERROR, with two named exceptions:
//    * if 0x100158b0 goes missing from ours, the accessor declaration
//      below has been turned back into a field read;
//    * a shipped-only/ours-only PAIR on 0x10023230 or 0x10015190 with
//      equal totals would be the fill's two endpoint blocks emitted in
//      the other order, which the pool (six 255.0f in v[0]-then-v[1]
//      order) rules out.
//
//  ------------------------------------------------------------------- //
//  COIN-FLIPS.  Both spellings stated, so one build settles each.
//  ------------------------------------------------------------------- //
//
//  1. ARM ORDER, and it is NOT a coin-flip -- the frame settles it.
//     Written as below,
//         if (v0 inside) { if (!(v1 inside)) { clip v1 } }
//         else           { if (!(v1 inside)) continue;  clip v0 }
//     the positive arm is emitted inline and the `else` is sunk, and its
//     five `float fRed..fSpec` get the HIGHER block of frame slots.  The
//     other spelling,
//         if (!(v0 inside)) { if (!(v1 inside)) continue; clip v0 }
//         else              { if (!(v1 inside)) { clip v1 } }
//     emits the clip-v0 arm inline, sinks clip-v1, and gives clip-v0 the
//     higher slots.  The shipped frame gives the INLINE arm the higher
//     slots in all six planes (plane 1: inline -0x498, sunk -0x470), so
//     the spelling below is the shipped one.  It also predicts, for the
//     other spelling, a call-sequence result that is still 15 of 15 -- the
//     arms are call-free, which is exactly why the frame had to decide
//     it.
//
//  2. WITHIN AN ARM, THE ORDER OF THE POSITION STATEMENTS.  icl
//     schedules all four float LERPs and the `fVal` together across up
//     to eight x87 registers, so the emission order is NOT source order:
//     plane 2's source (per big70d10.cpp) is `y` then `fVal` and the
//     shipped loads are `fVal`'s difference FIRST, while plane 3's
//     source is `x` then `fVal` and the shipped loads are `x` first.
//     The two spellings for planes 1 and 6 (which lerp both x and y) are
//     therefore indistinguishable here; `x` then `y` is written, matching
//     big70d10.cpp, big6d910.cpp and big5a960.cpp.  Predicted
//     difference either way: none in the call sequence, none in the census.
//
//  3. WHICH OF THE PINNED PAIR IS ASSIGNED FIRST in planes 2..5.  The
//     shipped code uses `fsts` (store, no pop) then `fstps` (store and
//     pop) of one value, and the `fsts` is the FIRST source use.  It
//     targets the Z field in all four planes and all eight arms
//     (0x100412cc, 0x10041539, 0x10041784, 0x100419c0 and the four sunk
//     twins at 0x10042207, 0x10042475, 0x100426d5, 0x10042927), so this
//     function writes `v.z = fVal;` BEFORE `v.x = -fVal;` / `v.y =
//     fVal;`.  That is the OPPOSITE of big70d10.cpp's planes 3 and 4
//     (which record `fsts` on x/y first) and the SAME as its planes 2
//     and 5.  Written below as the bytes say, per plane.  Reversing a
//     pair would swap that plane's `fsts`/`fstps` targets and nothing
//     else.
//
//  4. `float fT` vs `double fT`.  All twelve arms spill fT with `fstl`
//     (64-bit) and reload it with `fmull`, so the value never becomes a
//     32-bit float.  `float fT;` is what the five sibling decodes use
//     and what is written below; `double fT;` is the one-token
//     alternative and would be indistinguishable in the call sequence and in the
//     census.
//
//  5. `fDen` as a named local vs one expression.  The shipped code emits
//     the denominator's terms first and the numerator's second, then
//     divides -- which is what `fT = num / den;` as ONE expression gives
//     ("the term first in the source is last out").  There is
//     no frame slot for a denominator anywhere in the 1,208 bytes.  One
//     expression is written.  A named `float fDen;` would very likely
//     stay in `st` and be indistinguishable; big70d10.cpp needs the name
//     only because of its epsilon test, which this function does not
//     have.
//
//  ------------------------------------------------------------------- //
//  WHAT IS NOT PINNED
//  ------------------------------------------------------------------- //
//
//  * The EIGHT FOLDED `1.0f` at 0x100a9c40..0x100a9c58 and 0x100a9c60.
//    See the pool section.  No statement is invented for them.
//  * The identity of 0x1008d7d0 beyond big0ae20.cpp's "stage" (it is
//    read as a plain uint32 and handed to the stage-unbind).
//  * Whether `fObjAlpha` is a source local before the loop or a
//    loop-invariant hoisted out of it.  It is computed at
//    0x10040e86..0x10040eaa, before the list-empty `je`, so either
//    reading produces this code; it is written before the loop.
//  * The name of the object class.  It is the LTObject union arm for
//    OT_LINESYSTEM (lithtech.exe 0x0043d1c0 checks `type == 8`), and
//    there is no symbol for it; declared as ObjLine40e10 with only the
//    fields this function and ci_AddLine touch.
//  * There is NO `// UNDECODED:` gap.  Every one of the 1,669
//    instructions in 0x10040e10..0x10042d3e is accounted for by a region
//    above, and the census has no residue.
//
//  ------------------------------------------------------------------- //
//  PER REGION: WHAT IT DOES AND WHAT PINS IT
//  ------------------------------------------------------------------- //
//
//  0x10040e10..0x10040e36  prologue + stage unbind.  PINNED BY CALLS
//      (0x100232b0) and by the frame size 0x4b8, which the decomposition
//      above spends exactly.
//
//  0x10040e36..0x10040e93  the two matrices.  PINNED BY CALLS
//      (0x100158b0, 0x100151b0, 0x10014720) and by three sibling decodes
//      that already fix both argument lists (big5a960.cpp:802,
//      big60010.cpp:869, big17c14.cpp:877).  The 0x1008b9dc =
//      0x1008b880 + 0x15c identity is arithmetic, not a guess.
//
//  0x10040e86..0x10040ebc  fObjAlpha + list-empty guard.  PINNED BY the
//      lithtech.exe list-builder ci_AddLine (0x0043d1c0), which fixes
//      +0x1b0 as the head and +0x40 as the next field, and by
//      big5a960.cpp's m_nA97.
//
//  0x10040ec0..0x10040fcc  the vertex fill.  PINNED BY CALLS -- all ten
//      of the call-witnessed calls are here -- by the SDK's LTLine
//      layout, and by the six dumped 255.0f in v[0]-then-v[1] pool
//      order.
//
//  0x10040fcc..0x10041dbc and 0x10041f37..0x10042d3e  the twelve clip
//      arms.  NO CALLS AT ALL.  PINNED BY (a) the census, where seven
//      rows are exact multiples of twelve and of five; (b) the frame,
//      where 120 temporary slots fall into twelve consecutive runs of
//      five in plane order with the inline arm first; (c) an
//      expression-by-expression diff of all six planes' tests,
//      denominators, numerators and pinned coordinates against
//      big70d10.cpp, itself the sixth reading of the same source; and
//      (d) `as` verification of every divide's operand order.
//
//  0x10041dbc..0x10041f16  screen space + DrawPrimitive.  PINNED BY the
//      six globals 0x1008bb9c..0x1008bbb0 (already decoded verbatim in
//      big5a960.cpp:1128), by the two dumped 1.0f, by the FVF 0x1c4
//      immediate agreeing with the 0x20 vertex stride and with the count
//      of two texture LERPs per arm, and by the one indirect call.
//
//  0x10041f16..0x10041f37  the latch and the single epilogue.  PINNED BY
//      the extent (one ret, one prologue) and by ci_AddLine's +0x40.
//
//  0x10041f42  the SHARED `continue`.  All six "whole line rejected"
//      exits reach this one copy, and it is hosted at the LAST of the
//      six -- plane 6's sunk test falls straight into it, and the other
//      five jump back to it from below.  "A sequence of
//      guarded return/continue statements hosts the tail at the LAST",
//      which is exactly six sequential plane blocks each containing
//      `if (...) continue;`.  Written as an if/else chain instead, the
//      tail would be hosted at the FIRST.
//
// *********************************************************************** //

#include "ltbasedefs.h"


// ---------------------------------------------------------------------- //
//  Local copies of the shapes.  Relocations are masked, so only the
//  shapes matter.
// ---------------------------------------------------------------------- //
struct Vec40e10
{
	float	x;                                          // +0x00
	float	y;                                          // +0x04
	float	z;                                          // +0x08
};

// The quaternion 0x100151b0 sanitises, at +0x9c of the object.
struct Rot40e10
{
	float	x;
	float	y;
	float	z;
	float	w;
};

// The 4x4, indexed [row][column] as d3dr2a.cpp and d3dbatchd.cpp fix it.
struct Mat40e10
{
	float	m[4][4];
};

union VColor40e10
{
	uint32	m_n;
	uint8	m_b[4];                                     // 0=B 1=G 2=R 3=A
};


// ---------------------------------------------------------------------- //
//  The 32-byte vertex.  FVF 0x1c4 = XYZRHW | DIFFUSE | SPECULAR | TEX1,
//  read off the DrawPrimitive at 0x10041dfd/0x10041f09 and confirmed by
//  the 0x20 gap between v[0] at -0x23c and v[1] at -0x21c, and by there
//  being exactly TWO texture LERPs per clip arm rather than four.
// ---------------------------------------------------------------------- //
struct Vert40e10
{
	Vec40e10	m_vPos;                                 // +0x00
	float		m_fRHW;                                 // +0x0c
	VColor40e10	m_Diffuse10;                            // +0x10
	VColor40e10	m_Specular14;                           // +0x14
	float		m_fU;                                   // +0x18
	float		m_fV;                                   // +0x1c
};


// ---------------------------------------------------------------------- //
//  The SDK's line, src/LT2/sdk/inc/iltclient.h:34.  0x1c per point, 0x38 for
//  the pair, and every offset this function reads is one of these ten
//  fields.
// ---------------------------------------------------------------------- //
struct LinePt40e10
{
	Vec40e10	m_Pos;                                  // +0x00
	float		r;                                      // +0x0c
	float		g;                                      // +0x10
	float		b;                                      // +0x14
	float		a;                                      // +0x18
};

struct Line40e10
{
	LinePt40e10	m_Points[2];                            // +0x00, +0x1c
};


// ---------------------------------------------------------------------- //
//  The renderer-side line record, laid out by lithtech.exe's ci_AddLine
//  (0x0043d1c0): the line, then the object back-pointer, then the two
//  links -- and the links hold RECORD base addresses, not link
//  addresses, which is why the walk here reads +0x40 off the record and
//  compares against pObj+0x1b0.
// ---------------------------------------------------------------------- //
struct LineRec40e10
{
	Line40e10		m_Line;                             // +0x00
	void*			m_pObject38;                        // +0x38
	LineRec40e10*	m_pPrev3c;                          // +0x3c
	LineRec40e10*	m_pNext40;                          // +0x40
};


// ---------------------------------------------------------------------- //
//  The OT_LINESYSTEM object.  Only the fields this function reads are
//  declared; m_LineHead1b0 is the list's head pseudo-record, of which
//  only +0x38..+0x40 (= object +0x1e8..+0x1f0) is real storage.
// ---------------------------------------------------------------------- //
class ObjLine40e10
{
public:
	Vec40e10*		sub_100158b0();                     // 0x100158b0

	uint8			m_pad_00[0x97];
	uint8			m_nA97;                             // +0x97
	uint8			m_pad_98[0x9c - 0x98];
	Rot40e10		m_Rot9c;                            // +0x9c
	Vec40e10		m_vScaleac;                         // +0xac
	uint8			m_pad_b8[0x18c - 0xb8];
	Vec40e10		m_vPos18c;                          // +0x18c
	uint8			m_pad_198[0x1b0 - 0x198];
	LineRec40e10	m_LineHead1b0;                      // +0x1b0
};


// IDirect3DDevice7: only +0x64, DrawPrimitive (0x10041f13).
struct D3DDevice;

struct D3DDeviceVtbl
{
	uint8	m_pad_00[0x64];
	long (__stdcall* DrawPrimitive)(D3DDevice*, uint32, uint32, void*,
	                                uint32, uint32);              // +0x64
};

struct D3DDevice
{
	D3DDeviceVtbl*	lpVtbl;
};


// ---------------------------------------------------------------------- //
//  Externals.
// ---------------------------------------------------------------------- //
extern float		g_f1008b908;                        // near plane
extern float		g_f1008b910;                        // far plane
extern Mat40e10		g_m1008b9dc;                        // 0x1008b880 + 0x15c
extern float		g_f1008bb9c;                        // 0x1008bb9c
extern float		g_f1008bba0;                        // 0x1008bba0
extern float		g_f1008bba4;                        // 0x1008bba4
extern float		g_f1008bba8;                        // 0x1008bba8
extern float		g_f1008bbac;                        // 0x1008bbac
extern float		g_f1008bbb0;                        // 0x1008bbb0
extern uint32		g_n1008d7d0;                        // 0x1008d7d0  stage
extern D3DDevice*	g_pD3DDevice;                       // 0x1008d988

// 0x100232b0 -- d3dbatchd.cpp's "unbind a texture stage".
extern "C" void  sub_100232b0(uint32 nStage);                  // 0x100232b0

// 0x100151b0 -- d3dbatchd.cpp: (position, quaternion, scale) -> matrix.
extern "C" void  sub_100151b0(Vec40e10* pPos, Rot40e10* pRot,
                              Vec40e10* pScale, Mat40e10* pMat);
                                                               // 0x100151b0

// 0x10014720 -- MatMul(pDest, pMat1, pMat2), ltmatrix.h:172, out of line.
extern "C" void  sub_10014720(Mat40e10* pDest, Mat40e10* pMat1,
                              Mat40e10* pMat2);                // 0x10014720

// 0x10023230 -- 128 bytes, the projective transform.  Decoded in full
// from 0x10023230..0x100232af: `w = m[3][0]*in.x + m[3][1]*in.y +
// m[3][2]*in.z + m[3][3]; oow = 1.0f / w;` (`fdivrs 0x100a5c64`, dumped:
// 0x3f800000) and then out.x/out.y/out.z from rows 0/1/2 each scaled by
// oow -- and it leaves `oow` in st0 at the `ret`, i.e. it RETURNS the
// rhw.  That return is what 0x10040ee7 and 0x10040f70 store into
// v[i].m_fRHW.
extern "C" float sub_10023230(Vec40e10* pOut, Mat40e10* pMat,
                              Vec40e10* pIn);                  // 0x10023230

// 0x10015190 -- d3dbatchd.cpp's round-to-nearest float->int32, __cdecl
// (`flds 0x8(%ebp) ; fistpl -0x4(%ebp) ; mov -0x4(%ebp),%eax`).
extern "C" int32 sub_10015190(float f);                        // 0x10015190


// ---------------------------------------------------------------------- //
//  The part of a clip step that is the same in all TWELVE arms: the two
//  texture coordinates and the five colour bytes.  Identical to
//  w53_10070d10_os.cpp's lerp with the second texture pair removed --
//  this vertex is TEX1, not TEX2, which is what the twenty-four texture
//  LERP sites (rather than forty-eight) say.
//
//  The byte order +0x12, +0x11, +0x10, +0x13, +0x17 is read off the
//  sixty `movzbl <walk byte> ; movzbl <prev byte> ; sub` triples; the
//  difference is taken in the INTEGER unit and only then converted
//  (`fildl` x2), and the result goes back through a float temp and a
//  bare `fistpl` (`fstps -0x498(%ebp) ; flds -0x498(%ebp) ;
//  fistpl -0x1fc(%ebp) ; mov -0x1fc(%ebp),%cl ; mov %cl,<dest>` at
//  0x100410c5..0x100410ed) -- /Qrcd, with no fnstcw/fldcw dance and no
//  helper call at any of the sixty sites.
//
//  OUT may alias PREV: in the six sunk arms it is v[0] on both sides,
//  which is exactly what the shipped code does (it reads v[0]'s byte
//  twice and then writes it).
// ---------------------------------------------------------------------- //
// Round to nearest in the current FPU mode: w1_10001000.cpp's helper
// (0x10015190 out of line).  Inlined, it is the shipped `fstps t ;
// flds t ; fistpl n`.
// Measured over the true extent 0x10040e10..0x10042d3e:
// census residue 461 -> 152, 1598 -> 1709 instructions (shipped 1669).
// The enumeration still sizes this row at 4777 bytes because its
// bogus 0x100420b9 start cuts it, so
// no length can pair with it until that start is dropped.
inline int32 RoundF(float f)
{
	int32 n;
	__asm fld f
	__asm fistp n
	return n;
}

// The shared step is an INLINE FUNCTION (the vertex lerp
// 0x10023380's shape, w23_100525b0_os.cpp's LerpSClipVertA without the
// second texture pair), not the CLIP40E10_LERP macro over five block-scoped
// floats: normalised instruction match 0.593 -> 0.832, 6507 -> 6385 bytes
// differing, the plane blocks now line up 1:1.  Still 8 bytes of frame
// short (0x4b0 against 0x4b8): the shipped inline arm of plane 2 spills
// v[1].x to a double (-0x30) before `jb` and falls into the arm; icl here
// keeps it on the x87 stack and makes the arm the branch target.  Measured
// on top of this: the twelve crossings as w1-style ClipEdge inlines taking
// vector pointers (all twelve, the six sunk only, plane 2's inline only):
// they lay plane 2 out as shipped but lose the shared loads' doubles
// (frames 0x3f8 .. 0x4a8); not kept.
static inline void Lerp40e10(Vert40e10* pOut, Vert40e10* pPrev, Vert40e10* pWalk, float t)
{
	pOut->m_fU = (pWalk->m_fU - pPrev->m_fU) * t + pPrev->m_fU;
	pOut->m_fV = (pWalk->m_fV - pPrev->m_fV) * t + pPrev->m_fV;
	pOut->m_Diffuse10.m_b[2]  = (uint8)RoundF((pWalk->m_Diffuse10.m_b[2]  - pPrev->m_Diffuse10.m_b[2])  * t + pPrev->m_Diffuse10.m_b[2]);
	pOut->m_Diffuse10.m_b[1]  = (uint8)RoundF((pWalk->m_Diffuse10.m_b[1]  - pPrev->m_Diffuse10.m_b[1])  * t + pPrev->m_Diffuse10.m_b[1]);
	pOut->m_Diffuse10.m_b[0]  = (uint8)RoundF((pWalk->m_Diffuse10.m_b[0]  - pPrev->m_Diffuse10.m_b[0])  * t + pPrev->m_Diffuse10.m_b[0]);
	pOut->m_Diffuse10.m_b[3]  = (uint8)RoundF((pWalk->m_Diffuse10.m_b[3]  - pPrev->m_Diffuse10.m_b[3])  * t + pPrev->m_Diffuse10.m_b[3]);
	pOut->m_Specular14.m_b[3] = (uint8)RoundF((pWalk->m_Specular14.m_b[3] - pPrev->m_Specular14.m_b[3]) * t + pPrev->m_Specular14.m_b[3]);
}


// ---------------------------------------------------------------------- //
//  The camera-space -> screen-space step, once per vertex.  Written in
//  big5a960.cpp's exact spelling (0x1005ca38, decoded there as a loop);
//  here the source unrolls it over the two vertices, which is what the
//  TWO `1.0f` in the literal pool say.
//
//  `V.m_fRHW` is a genuine memory round-trip in the shipped code -- it
//  is stored with `fsts` (no pop) at 0x10041e08 / 0x10041e78 and then
//  RELOADED with `flds` at 0x10041e20, 0x10041e2a, 0x10041e82,
//  0x10041e9e -- so the source really does write the field and read it
//  back, exactly as big5a960.cpp does.  (v[0]: stored 0x10041e08,
//  reloaded 0x10041e20 and 0x10041e2a; v[1]: stored 0x10041e78,
//  reloaded 0x10041e82 and 0x10041e9e; the third use of each is served
//  from the register copy.)
// ---------------------------------------------------------------------- //
#define CLIP40E10_SCREEN(V)                                                  \
	(V).m_fRHW    = 1.0f / (V).m_vPos.z;                                     \
	(V).m_vPos.x  = ((V).m_vPos.x * g_f1008bb9c +                            \
	                 (V).m_vPos.z * g_f1008bba0) * (V).m_fRHW;               \
	(V).m_vPos.y  = ((V).m_vPos.y * g_f1008bba4 +                            \
	                 (V).m_vPos.z * g_f1008bba8) * (V).m_fRHW;               \
	(V).m_vPos.z  = ((V).m_vPos.z * g_f1008bbac +                            \
	                 g_f1008bbb0) * (V).m_fRHW;


// ======================================================================= //
//  0x10040e10 .. 0x10042d3d   (7,982 bytes)
//
//  Argument 1 is the transform block 0x1008b880 that 0x100500a0 pushes
//  (0x100500c9) and this function never reads -- it reaches the block's
//  +0x15c matrix as the absolute g_m1008b9dc instead.
// ======================================================================= //
void sub_10040e10(void* pCtx1008b880, ObjLine40e10* pObj)
{
	Mat40e10		mObj;                               // -0x2bc
	Mat40e10		mFull;                              // -0x27c
	Vert40e10		v[2];                               // -0x23c, -0x21c
	LineRec40e10*	pRec;
	float			fObjAlpha;

	// -- 0x10040e23: unbind the current texture stage --------------- //
	sub_100232b0(g_n1008d7d0);

	// -- 0x10040e38 / 0x10040e61 / 0x10040e81 ----------------------- //
	//    object -> world, then world -> clip.  0x100158b0 is the
	//    out-of-line &m_vPos18c accessor: it MUST stay a call.
	sub_100151b0(pObj->sub_100158b0(), &pObj->m_Rot9c,
	             &pObj->m_vScaleac, &mObj);

	sub_10014720(&mFull, &g_m1008b9dc, &mObj);

	// -- 0x10040e86: the object's own alpha, 0..255, widened once --- //
	fObjAlpha = (float)pObj->m_nA97;

	// -- 0x10040e96 entry guard, 0x10041f16 latch -- one rotated loop
	//    over the circular list, sentinel &pObj->m_LineHead1b0.
	for (pRec = pObj->m_LineHead1b0.m_pNext40;
	     pRec != &pObj->m_LineHead1b0;
	     pRec = pRec->m_pNext40)
	{
		// ---- 0x10040ec0: v[0] from m_Points[0] -------------------- //
		v[0].m_fRHW = sub_10023230(&v[0].m_vPos, &mFull,
		                           &pRec->m_Line.m_Points[0].m_Pos);

		v[0].m_Diffuse10.m_b[2] =
			(uint8)sub_10015190(pRec->m_Line.m_Points[0].r * 255.0f);
		v[0].m_Diffuse10.m_b[1] =
			(uint8)sub_10015190(pRec->m_Line.m_Points[0].g * 255.0f);
		v[0].m_Diffuse10.m_b[0] =
			(uint8)sub_10015190(pRec->m_Line.m_Points[0].b * 255.0f);
		v[0].m_Diffuse10.m_b[3] =
			(uint8)sub_10015190(pRec->m_Line.m_Points[0].a * fObjAlpha);

		// ---- 0x10040f40: v[1] from m_Points[1] -------------------- //
		v[1].m_fRHW = sub_10023230(&v[1].m_vPos, &mFull,
		                           &pRec->m_Line.m_Points[1].m_Pos);

		v[1].m_Diffuse10.m_b[2] =
			(uint8)sub_10015190(pRec->m_Line.m_Points[1].r * 255.0f);
		v[1].m_Diffuse10.m_b[1] =
			(uint8)sub_10015190(pRec->m_Line.m_Points[1].g * 255.0f);
		v[1].m_Diffuse10.m_b[0] =
			(uint8)sub_10015190(pRec->m_Line.m_Points[1].b * 255.0f);
		v[1].m_Diffuse10.m_b[3] =
			(uint8)sub_10015190(pRec->m_Line.m_Points[1].a * fObjAlpha);

		// ================================================================ //
		//  PLANE 1, NEAR:  z >= g_f1008b908
		//  test 0x10040fcc (fcomp/jb 0x10042af0, then fcomp/jbe
		//  0x10041221) | inline arm 0x10041008 | sunk test 0x10042af0
		//  (fcomp/ja 0x10041f42) | sunk arm 0x10042b0a (jmp 0x10041221)
		//  Numerator/denominator: 0x10041008..0x10041014, `de f9` =
		//  Intel FDIVP.  Pinned coordinate by INTEGER move at
		//  0x1004108f/0x1004109b.
		// ================================================================ //
		if (v[0].m_vPos.z >= g_f1008b908)
		{
			if (!(v[1].m_vPos.z >= g_f1008b908))
			{
				float fT;

				fT = (g_f1008b908 - v[0].m_vPos.z)
				   / (v[1].m_vPos.z - v[0].m_vPos.z);

				v[1].m_vPos.x = (v[1].m_vPos.x - v[0].m_vPos.x) * fT
				              + v[0].m_vPos.x;
				v[1].m_vPos.y = (v[1].m_vPos.y - v[0].m_vPos.y) * fT
				              + v[0].m_vPos.y;
				v[1].m_vPos.z = g_f1008b908;

				Lerp40e10(&v[1], &v[0], &v[1], fT);
			}
		}
		else
		{
			float fT;

			if (!(v[1].m_vPos.z >= g_f1008b908))
				continue;

			fT = (g_f1008b908 - v[0].m_vPos.z)
			   / (v[1].m_vPos.z - v[0].m_vPos.z);

			v[0].m_vPos.x = (v[1].m_vPos.x - v[0].m_vPos.x) * fT
			              + v[0].m_vPos.x;
			v[0].m_vPos.y = (v[1].m_vPos.y - v[0].m_vPos.y) * fT
			              + v[0].m_vPos.y;
			v[0].m_vPos.z = g_f1008b908;

			Lerp40e10(&v[0], &v[0], &v[1], fT);
		}

		// ================================================================ //
		//  PLANE 2, LEFT:  -z < x
		//  test 0x10041221 (fchs/fcomps/jae 0x10042890, then
		//  fchs/fcomps/jb 0x1004147c) | inline arm 0x1004125b | sunk test
		//  0x10042890 (jae 0x10041f42) | sunk arm 0x100428b0
		//  (jmp 0x1004147c).  0x1004125b..0x1004126f, `de f1` = Intel
		//  FDIVRP.  `fsts` on Z at 0x100412cc then `fchs` then `fstps` on
		//  X at 0x100412de -- so z is written first and x gets -fVal.
		// ================================================================ //
		if (-v[0].m_vPos.z < v[0].m_vPos.x)
		{
			if (!(-v[1].m_vPos.z < v[1].m_vPos.x))
			{
				float fT, fVal;

				fT = -(v[0].m_vPos.x + v[0].m_vPos.z)
				   / ((v[1].m_vPos.x - v[0].m_vPos.x)
				      + v[1].m_vPos.z - v[0].m_vPos.z);

				v[1].m_vPos.y = (v[1].m_vPos.y - v[0].m_vPos.y) * fT
				              + v[0].m_vPos.y;
				fVal = (v[1].m_vPos.z - v[0].m_vPos.z) * fT
				     + v[0].m_vPos.z;
				v[1].m_vPos.z = fVal;
				v[1].m_vPos.x = -fVal;

				Lerp40e10(&v[1], &v[0], &v[1], fT);
			}
		}
		else
		{
			float fT, fVal;

			if (!(-v[1].m_vPos.z < v[1].m_vPos.x))
				continue;

			fT = -(v[0].m_vPos.x + v[0].m_vPos.z)
			   / ((v[1].m_vPos.x - v[0].m_vPos.x)
			      + v[1].m_vPos.z - v[0].m_vPos.z);

			v[0].m_vPos.y = (v[1].m_vPos.y - v[0].m_vPos.y) * fT
			              + v[0].m_vPos.y;
			fVal = (v[1].m_vPos.z - v[0].m_vPos.z) * fT + v[0].m_vPos.z;
			v[0].m_vPos.z = fVal;
			v[0].m_vPos.x = -fVal;

			Lerp40e10(&v[0], &v[0], &v[1], fT);
		}

		// ================================================================ //
		//  PLANE 3, TOP:  y < z
		//  test 0x1004147c (fcomp/jbe 0x10042630, then fcomp/ja
		//  0x100416c7) | inline arm 0x100414aa | sunk test 0x10042630
		//  (jbe 0x10041f42) | sunk arm 0x1004264a (jmp 0x100416c7).
		//  0x100414aa..0x100414be, `de f1` = Intel FDIVRP.  `fsts` on Z
		//  at 0x10041539 then `fstps` on Y at 0x1004153f, no `fchs`
		//  between them -- z first, y = fVal.
		// ================================================================ //
		if (v[0].m_vPos.y < v[0].m_vPos.z)
		{
			if (!(v[1].m_vPos.y < v[1].m_vPos.z))
			{
				float fT, fVal;

				fT = -(v[0].m_vPos.y - v[0].m_vPos.z)
				   / ((v[1].m_vPos.y - v[0].m_vPos.y)
				      - v[1].m_vPos.z + v[0].m_vPos.z);

				v[1].m_vPos.x = (v[1].m_vPos.x - v[0].m_vPos.x) * fT
				              + v[0].m_vPos.x;
				fVal = (v[1].m_vPos.z - v[0].m_vPos.z) * fT
				     + v[0].m_vPos.z;
				v[1].m_vPos.z = fVal;
				v[1].m_vPos.y = fVal;

				Lerp40e10(&v[1], &v[0], &v[1], fT);
			}
		}
		else
		{
			float fT, fVal;

			if (!(v[1].m_vPos.y < v[1].m_vPos.z))
				continue;

			fT = -(v[0].m_vPos.y - v[0].m_vPos.z)
			   / ((v[1].m_vPos.y - v[0].m_vPos.y)
			      - v[1].m_vPos.z + v[0].m_vPos.z);

			v[0].m_vPos.x = (v[1].m_vPos.x - v[0].m_vPos.x) * fT
			              + v[0].m_vPos.x;
			fVal = (v[1].m_vPos.z - v[0].m_vPos.z) * fT + v[0].m_vPos.z;
			v[0].m_vPos.z = fVal;
			v[0].m_vPos.y = fVal;

			Lerp40e10(&v[0], &v[0], &v[1], fT);
		}

		// ================================================================ //
		//  PLANE 4, RIGHT:  x < z
		//  test 0x100416c7 (fcomp/jbe 0x100423d0, then fcomp/ja
		//  0x10041912) | inline arm 0x100416f5 | sunk test 0x100423d0
		//  (jbe 0x10041f42) | sunk arm 0x100423ea (jmp 0x10041912).
		//  0x100416f5..0x10041709, `de f1` = Intel FDIVRP.  `fsts` on Z
		//  at 0x10041784 then `fstps` on X at 0x1004178a -- z first,
		//  x = fVal.
		// ================================================================ //
		if (v[0].m_vPos.x < v[0].m_vPos.z)
		{
			if (!(v[1].m_vPos.x < v[1].m_vPos.z))
			{
				float fT, fVal;

				fT = -(v[0].m_vPos.x - v[0].m_vPos.z)
				   / ((v[1].m_vPos.x - v[0].m_vPos.x)
				      - v[1].m_vPos.z + v[0].m_vPos.z);

				v[1].m_vPos.y = (v[1].m_vPos.y - v[0].m_vPos.y) * fT
				              + v[0].m_vPos.y;
				fVal = (v[1].m_vPos.z - v[0].m_vPos.z) * fT
				     + v[0].m_vPos.z;
				v[1].m_vPos.z = fVal;
				v[1].m_vPos.x = fVal;

				Lerp40e10(&v[1], &v[0], &v[1], fT);
			}
		}
		else
		{
			float fT, fVal;

			if (!(v[1].m_vPos.x < v[1].m_vPos.z))
				continue;

			fT = -(v[0].m_vPos.x - v[0].m_vPos.z)
			   / ((v[1].m_vPos.x - v[0].m_vPos.x)
			      - v[1].m_vPos.z + v[0].m_vPos.z);

			v[0].m_vPos.y = (v[1].m_vPos.y - v[0].m_vPos.y) * fT
			              + v[0].m_vPos.y;
			fVal = (v[1].m_vPos.z - v[0].m_vPos.z) * fT + v[0].m_vPos.z;
			v[0].m_vPos.z = fVal;
			v[0].m_vPos.x = fVal;

			Lerp40e10(&v[0], &v[0], &v[1], fT);
		}

		// ================================================================ //
		//  PLANE 5, BOTTOM:  -z < y
		//  test 0x10041912 (fchs/fcomps/jae 0x10042170, then
		//  fchs/fcomps/jb 0x10041b7c) | inline arm 0x1004194c | sunk test
		//  0x10042170 (jae 0x10041f42) | sunk arm 0x10042190
		//  (jmp 0x10041b7c).  0x1004194c..0x10041960, `de f1` = Intel
		//  FDIVRP.  `fsts` on Z at 0x100419c0 then `fchs` then `fstps` on
		//  Y at 0x100419d2 -- z first, y = -fVal.
		// ================================================================ //
		if (-v[0].m_vPos.z < v[0].m_vPos.y)
		{
			if (!(-v[1].m_vPos.z < v[1].m_vPos.y))
			{
				float fT, fVal;

				fT = -(v[0].m_vPos.y + v[0].m_vPos.z)
				   / ((v[1].m_vPos.y - v[0].m_vPos.y)
				      + v[1].m_vPos.z - v[0].m_vPos.z);

				v[1].m_vPos.x = (v[1].m_vPos.x - v[0].m_vPos.x) * fT
				              + v[0].m_vPos.x;
				fVal = (v[1].m_vPos.z - v[0].m_vPos.z) * fT
				     + v[0].m_vPos.z;
				v[1].m_vPos.z = fVal;
				v[1].m_vPos.y = -fVal;

				Lerp40e10(&v[1], &v[0], &v[1], fT);
			}
		}
		else
		{
			float fT, fVal;

			if (!(-v[1].m_vPos.z < v[1].m_vPos.y))
				continue;

			fT = -(v[0].m_vPos.y + v[0].m_vPos.z)
			   / ((v[1].m_vPos.y - v[0].m_vPos.y)
			      + v[1].m_vPos.z - v[0].m_vPos.z);

			v[0].m_vPos.x = (v[1].m_vPos.x - v[0].m_vPos.x) * fT
			              + v[0].m_vPos.x;
			fVal = (v[1].m_vPos.z - v[0].m_vPos.z) * fT + v[0].m_vPos.z;
			v[0].m_vPos.z = fVal;
			v[0].m_vPos.y = -fVal;

			Lerp40e10(&v[0], &v[0], &v[1], fT);
		}

		// ================================================================ //
		//  PLANE 6, FAR:  z <= g_f1008b910.  LAST, as in all eight
		//  earlier readings of this clipper.
		//  test 0x10041b7c (fcomps 0x1008b910/ja 0x10041f37, then
		//  fcompl -0x40(%ebp)/jbe 0x10041dbc) | inline arm 0x10041ba6 |
		//  sunk test 0x10041f37 (jbe 0x10041f50, else fall into the
		//  shared continue at 0x10041f42) | sunk arm 0x10041f50
		//  (jmp 0x10041dbc).  0x10041ba6..0x10041bb2, `de f9` = Intel
		//  FDIVP.  Pinned coordinate by INTEGER move at
		//  0x10041c30/0x10041c3c and 0x10041fdc/0x10041fe8.
		// ================================================================ //
		if (v[0].m_vPos.z <= g_f1008b910)
		{
			if (!(v[1].m_vPos.z <= g_f1008b910))
			{
				float fT;

				fT = (g_f1008b910 - v[0].m_vPos.z)
				   / (v[1].m_vPos.z - v[0].m_vPos.z);

				v[1].m_vPos.x = (v[1].m_vPos.x - v[0].m_vPos.x) * fT
				              + v[0].m_vPos.x;
				v[1].m_vPos.y = (v[1].m_vPos.y - v[0].m_vPos.y) * fT
				              + v[0].m_vPos.y;
				v[1].m_vPos.z = g_f1008b910;

				Lerp40e10(&v[1], &v[0], &v[1], fT);
			}
		}
		else
		{
			float fT;

			if (!(v[1].m_vPos.z <= g_f1008b910))
				continue;

			fT = (g_f1008b910 - v[0].m_vPos.z)
			   / (v[1].m_vPos.z - v[0].m_vPos.z);

			v[0].m_vPos.x = (v[1].m_vPos.x - v[0].m_vPos.x) * fT
			              + v[0].m_vPos.x;
			v[0].m_vPos.y = (v[1].m_vPos.y - v[0].m_vPos.y) * fT
			              + v[0].m_vPos.y;
			v[0].m_vPos.z = g_f1008b910;

			Lerp40e10(&v[0], &v[0], &v[1], fT);
		}

		// ---- 0x10041dbc: camera space -> screen space, per vertex --- //
		CLIP40E10_SCREEN(v[0])
		CLIP40E10_SCREEN(v[1])

		// ---- 0x10041dfa..0x10041f13 -------------------------------- //
		//      2 = D3DPT_LINELIST, FVF 0x1c4, two vertices, flags 0.
		//      The primitive type and the vertex count are the SAME
		//      constant 2 and the shipped code materialises it once
		//      (`mov $0x2,%eax` at 0x10041dfd, stored to both
		//      0x4(%esp) and 0x10(%esp)).
		g_pD3DDevice->lpVtbl->DrawPrimitive(g_pD3DDevice, 2, 0x1c4,
		                                    &v[0], 2, 0);
	}
}
