// *********************************************************************** //
//
//  MODULE:   BIG_24024 (.CPP)
//
//  ------------------------------------------------------------------- //
//  THE CODE HAS MOVED.  0x10024090 reaches BOTH functions
//  decoded here at their /Qip REGISTER entries (0x10025523 -> 0x10024024,
//  0x100254cf -> 0x1002564c), and icl emits a register entry, and a
//  caller that uses it, only inside one /Qip translation unit.  So the
//  two bodies are now compiled in big24090.cpp (/O2 /Qip
//  /Qip_no_inlining), where all four entries are byte-identical; this
//  file keeps the decode and the audit.  Two findings from the move that
//  correct the notes below: 0x10024020's pool pop is StdLith's inline
//  sb_Allocate stored through untested (0x1002407f's `xor %eax,%eax` is
//  the allocator's NULL return), and 0x10025640 is ONE flat alternating
//  chain -- icl does re-associate the parenthesised three-pair form.
//  ------------------------------------------------------------------- //
//
//  PURPOSE:  The enumeration row "0x10024024, 5,672 bytes" is a DOUBLE
//            EXTENT DEFECT.  There is no 5,672-byte function at
//            0x10024024.  The row is
//
//              0x10024024 .. 0x10024084   97 B  the TAIL of a 101-byte
//                                               function whose real start
//                                               is 0x10024020
//              0x10024085 .. 0x1002408f   11 B  lea-nop alignment padding
//              0x10024090 .. 0x1002563f 5552 B  a WHOLE, SEPARATE function
//                                               (already reconstructed as
//                                               src/engine/d3d/big24090.cpp)
//              0x10025640 .. 0x1002564b   12 B  the /Qip STACK entry of
//                                               the NEXT function, whose
//                                               register entry is at
//                                               0x1002564c -- the row's
//                                               own stated end
//                                               ------
//                                        5672 B
//
//            97 + 11 + 5552 + 12 = 5,672 exactly.  The row starts four
//            bytes inside one function and stops twelve bytes inside
//            another.
//
//            THE FUNCTION AT 0x10024024 IS THEREFORE THE 101-BYTE
//            0x10024020, and it is the only genuinely undecoded body in
//            the row.  It is written out in full below, every byte of it.
//            What it does: it is d3d.ren's DEFER-THIS-OBJECT draw slot --
//            it takes a renderable object, pops a 0x18-byte record off
//            the shared element pool at 0x1008d4e4, stamps the object and
//            the CURRENT clip-flag word (0x1008b874) into it, and pushes
//            it on the singly-linked list headed at 0x1008d4bc so that
//            the end-of-scene flush at 0x10025820 can draw it later with
//            those flags restored.
//
//            The 5,552-byte body at 0x10024090 (the overdraw meter) is
//            NOT re-written here.  It is already reconstructed at
//            src/engine/d3d/big24090.cpp to this same standard; this file
//            carries an INDEPENDENT AUDIT of that decode against the
//            shipped bytes instead, at the bottom, as five independent
//            instruments.
//
//  Compiler: Intel C/C++ 4.0, /O2 /Qip.  0x10024020 is the textbook
//            "external linkage + /Qip" two-entry pair:
//
//              10024020  mov 0x4(%esp),%eax      <- the STACK entry,
//                                                   one argument load
//              10024024  sub $0xc,%esp           <- the REGISTER entry:
//                                                   arg 1 live-in in %eax
//
//            and the two entries have disjoint caller sets, which is the
//            proof rather than the pattern-match (e8/e9 scan over the
//            whole of .text, see below): three cross-TU callers reach
//            0x10024020 and exactly one same-TU caller reaches
//            0x10024024.
//
// ----------------------------------------------------------------------- //
//  THE EXTENT -- CONFIRMED AT BOTH ENDS, MYSELF, FROM THE BYTES
// ----------------------------------------------------------------------- //
//
//      the extent 0x10024024, 5672, walked from the bytes:
//        0x10024024..0x1002564c  5,672 bytes  1604 instructions
//        ret               2        0x10024058   0x10025539
//        push %ebp frames  1        0x10024090   (lea-nop padding precedes it)
//        branch targets    0 outside the range or mid-instruction
//        last instruction  ends exactly at the stated end
//
//  The question: are the two `ret`s one /O2
//  duplicated epilogue or two functions?  THEY ARE TWO FUNCTIONS, and the
//  extent walk already says so in a line it does not flag: a
//  `push %ebp` prologue at 0x10024090 *with lea-nop padding in front of
//  it*.  A duplicated epilogue does not get an aligned, padded prologue
//  after it.  Read off the bytes:
//
//    10024014  ret                           <- previous function's exit
//    10024015  lea 0x0(%esi,%eiz,1),%esi     <- 4 bytes padding
//    10024019  lea 0x0(%edi,%eiz,1),%edi     <- 7 more, 11 in all
//    10024020  mov 0x4(%esp),%eax            <- FUNCTION A: stack entry
//    10024024  sub $0xc,%esp                 <- FUNCTION A: register entry
//      ...
//    10024055  add $0xc,%esp                 <- A's epilogue
//    10024058  ret                           <- `ret` #1: A's ONLY exit
//    10024059  ...                           <- A's cold refill block
//    10024083  jmp 0x1002403c                <- A's last instruction,
//                                                ends at 0x10024085
//    10024085  lea 0x0(%esi,%eiz,1),%esi     <- 4 bytes padding
//    10024089  lea 0x0(%edi,%eiz,1),%edi     <- 7 more, 11 in all
//    10024090  push %ebp                     <- FUNCTION B, on a 16-byte
//    10024091  mov %esp,%ebp                    boundary
//    10024093  sub $0xa90,%esp
//      ...
//    10025539  ret                           <- `ret` #2: B's only exit
//      ...
//    1002563b  jmp 0x10025276                <- B's last instruction,
//                                                ends at 0x10025640
//    10025640  mov 0x4(%esp),%eax            <- FUNCTION C: stack entry,
//    10025644  mov 0x8(%esp),%edx               THREE argument loads
//    10025648  mov 0xc(%esp),%ecx
//    1002564c  fld 0x4(%edx)                 <- FUNCTION C: register entry
//                                                = the row's stated END
//
//  So each of the two `ret`s belongs to a different function, each has
//  exactly one prologue, and A's cold block and B's sunk arms both branch
//  only within their own body -- which is why the extent walk finds zero
//  stray branch targets even though the row spans three functions.  Zero
//  stray targets is NOT evidence of one function when the constituent
//  functions are each self-contained; that is the trap this row sets.
//
//  A's own extent, checked on its own:
//
//      the extent 0x10024020, 101:
//        ret 1 (0x10024058); push %ebp frames 0 (frameless: normal for
//        icl); branch targets 0 outside the range; last instruction ends
//        exactly at the stated end
//
//      the call count over 0x10024020..0x10024085:
//        101 bytes, 1 calls (1 direct, 0 indirect), 101 bytes per call
//
//  And 0x10024020 .. 0x10024084 inclusive is 0x65 = 101 bytes.
//
// ----------------------------------------------------------------------- //
//  THE 0x10024090 CONFLICT, RESOLVED
// ----------------------------------------------------------------------- //
//
//  An earlier note on this row records
//
//      "0x10024090's row begins at 0x10024024, which is a different
//       function's register entry"
//
//  and it looks possibly wrong because a prologue-based extent check does
//  not report a register-entry defect on 0x10024024.  **The note is
//  RIGHT and that check has a blind spot.**
//
//  A /Qip test that only walks back over the argument loads when the row's
//  FIRST instruction is `push %ebp` never fires here.
//
//  0x10024024's first instruction is `sub $0xc,%esp`, because function A
//  is FRAMELESS -- so the test never runs and the row comes back clean.
//  A /Qip register entry does not have to begin with a frame pointer; a
//  frameless one begins with `sub $N,%esp` (or with any instruction at
//  all).  That gap is why this row's defect survived a sweep that
//  looked only for frame-pointer prologues.
//
//  The positive evidence that 0x10024024 is a register entry, and that
//  0x10024020 and 0x10024090 are two DISTINCT functions with NO overlap:
//
//   1. THE ARGUMENT LOAD.  0x10024020 is `mov 0x4(%esp),%eax` and nothing
//      else; it falls straight into 0x10024024, which uses %eax live-in
//      (`mov %eax,%ecx` at 0x1002402f) and never reloads it from the
//      frame.  One argument load, so the corrected start is 4 bytes
//      earlier, not 8.
//
//   2. DISJOINT CALLER SETS (e8/e9 scan of all of .text):
//        0x10024020  <- 0x1001428e, 0x1006f16c, 0x1006f527   (3, cross-TU)
//        0x10024024  <- 0x10025523                           (1, same-TU:
//                       it is INSIDE function B)
//        0x10024090  <- nothing at all
//      That is exactly the /Qip signature: outside callers at +0, the
//      same translation unit's caller at +4.  It also proves the two
//      bodies are not one function, because a single function would not
//      have a caller landing 0x6c bytes into it from outside.
//
//   3. B IS REACHED ONLY THROUGH THE DRAW-SLOT TABLE.  0x10024090 has no
//      direct caller; 0x1002571f loads it into %edx and stores it into
//      all four slots 0x1008ce94/98/9c/a0.  A's address goes into the
//      same table -- `movl $0x10024020,0x1008ce98` at 0x100256e2 (the
//      default) and the four stores at 0x10025748 -- so A and B are
//      PEERS in that table, alternative implementations of one draw
//      slot, not a function and its interior.
//
//   4. NOTHING BRANCHES ACROSS.  No branch in 0x10024020..0x10024084
//      targets >= 0x10024090 and no branch in 0x10024090..0x1002563f
//      targets < 0x10024090; the eleven bytes between them are lea-nop.
//
//  DO THE RANGES OVERLAP?  Yes -- and the overlap is a defect worth
//  stating precisely, because the enumeration has two rows here with two
//  different numbers:
//
//    * `0x10024020,4` -- a
//      four-byte "function", which is just A's stack entry.  A is 101
//      bytes.  UNDER by 97.
//    * `0x10024024,5672`, which
//      overlaps A's own row (4 bytes of it), swallows B whole, and runs
//      12 bytes into C.
//    * B's correct extent is 0x10024090, 5552.
//    * An earlier diagnosis of this row was right, but its
//      claim that the 101-byte function is "already decoded in
//      big124a4.cpp:505 and big6d910.cpp:666" is WRONG: both of those
//      lines are `extern` DECLARATIONS of sub_10024020, not definitions.
//      The body had never been written.  It is written below.
//
//    The corrected enumeration is three rows:
//      0x10024020    101   (A, this file)
//      0x10024090   5552   (B, src/engine/d3d/big24090.cpp)
//      0x10025640     53   (C, body ends 0x10025675; stack entry
//                           0x10025640, register entry 0x1002564c,
//                           `ret` 0x10025674, then 11 bytes of padding
//                           to 0x10025680.  Written out below.)
//
// ----------------------------------------------------------------------- //
//  WHAT THE CHECK WITNESSES
// ----------------------------------------------------------------------- //
//
//  The earlier figure -- 5 calls in 5,672 bytes, 1,134 B/call -- belongs
//  to the bogus row.  Split correctly:
//
//    A  0x10024020..0x10024085   101 B   1 call   101 B/call
//    B  0x10024090..0x10025640  5552 B   4 calls 1388 B/call
//
//  (call counts on each range; 1 + 4 = 5, which is the row's own
//  5 -- arithmetic worth doing before decoding anything large.)
//
//  For A the check is strong in the only sense that matters at this size:
//  one call, and it is 77% of the way through the body, so a wrong
//  decode of the refill path shows up as a missing or extra call.  For A
//  the CALL-FREE STRETCHES are
//
//      0x10024020..0x1002406d   78 bytes   (77.2% of the body)
//      0x10024073..0x10024085   18 bytes   (17.8%)
//
//  i.e. the call witnesses one statement out of five and the census below
//  carries the rest.  A is small enough that the census is exhaustive:
//  every one of its 28 instructions is named.
//
//  For B, see the audit at the bottom of this file.
//
// *********************************************************************** //


// ======================================================================= //
//  THE 0x18-BYTE POOL ELEMENT.  This is d3dr3c.cpp's Elem1000eef0 /
//  big124a4.cpp's Elem124a4, unchanged -- the same pool at 0x1008d4e4
//  serves both lists, so the record layout is not a guess here, it is the
//  one already read off 0x1000eef0 and 0x100142e6.  The three fields THIS
//  function writes are +0x00, +0x10 and +0x14; the drain at 0x10025820
//  reads exactly those three back (0x1002584b `mov (%ebx),%eax`,
//  0x10025840 `mov 0x10(%ebx),%eax`, 0x10025843 `mov 0x14(%ebx),%ebp`),
//  and the free-list link the pool itself uses is +0x00 (0x1000a6ae
//  `mov (%eax),%edx`), which is why the drain's 0x10025865
//  `mov %eax,(%ebx)` can overwrite the owner on the way back to the pool.
// ======================================================================= //

// The pool header.  +0x08 is the grow count and +0x18 the free-list head;
// both are read as ABSOLUTE addresses here (0x1008d4ec and 0x1008d4fc)
// because icl folded the base, and 0x1008d4e4+0x08 = 0x1008d4ec,
// 0x1008d4e4+0x18 = 0x1008d4fc.  0x1000a6a0's out-of-line copy of the
// same allocator reaches them as `0x8(%ecx)` and `0x18(%ecx)` off the
// pool pointer, which is what fixes the two offsets.


// ======================================================================= //
//  GLOBALS.  Every one is an address these 101 bytes touch.
// ======================================================================= //

// 0x1008b874 -- the clip-flag word.  Named by big572b4.cpp ("clip
// flags"), big124a4.cpp and big16324.cpp; 0x10057391 sets it to 0x3f (six
// planes) and big124a4.cpp stores it into the SAME element field +0x10
// at 0x10014329.  The drain at 0x10025846 stores it back before drawing.

// 0x1008d4e4 -- the 0x18-byte element pool, and 0x1008d4fc its free-list
// head.  Declared exactly as big124a4.cpp declares them (both the pool
// object and the head separately), because the shipped code reaches the
// head as a folded absolute and the pool only as `$0x1008d4e4` pushed to
// the refill helper.

// 0x1008d4bc -- THE DEFERRED-DRAW LIST HEAD.  Read off three sites:
//   * this function pushes onto it (0x1002403e read, 0x10024050 store);
//   * 0x10025820 walks it through +0x14, restores each element's +0x10
//     into g_1008b874, draws through 0x1002803c, returns the element to
//     the pool and finally clears the head (0x10025871 `movl $0x0`);
//   * 0x1002d61b zeroes it at the top of every frame alongside
//     0x1008d4c0, 0x1008d4c4 and 0x1008d520.
// big2d1c0.cpp / d3dr5b.cpp declare it `uint32` because they only ever
// zero it; the two functions that USE it use it as an Elem* list head, so
// that is how it is declared here.  See the coin-flip note at the bottom.


// ======================================================================= //
//  NEIGHBOURS
// ======================================================================= //

// 0x1007fbf0 -- the pool refill.  d3dbatchc.cpp already decoded it and
// records the prototype `(pool, pool->m_nGrowCount08)`, __cdecl, called
// with a batched `add $0x8,%esp` cleanup at its other sites.  THIS call
// site (0x1002406e) writes its two arguments into a pre-subtracted frame
// (`sub $0xc,%esp` in the prologue, `movl $0x1008d4e4,(%esp)`,
// `mov %edx,0x4(%esp)`) and the third slot 0x8(%esp) holds the spilled
// object pointer -- so the 0xc is 8 bytes of outgoing arguments plus one
// spill slot, not 12 bytes of arguments.


// ======================================================================= //
//
//  0x10024020 .. 0x10024084   (101 bytes, 1 call)
//
//  THE DEFERRED-DRAW SLOT.  Signature read off the call sites, not
//  guessed:
//
//    * 0x1001428e (big124a4.cpp's TU) -- `push %esi ; call 0x10024020 ;
//      ... add $0x4,%esp`-shaped: ONE stack argument, and big124a4.cpp
//      already writes it as `sub_10024020(pObj)` where pObj is that
//      function's renderable object.
//    * 0x1006f16c and 0x1006f527 (big6d910.cpp's TU) -- big6d910.cpp
//      writes both as `sub_10024020(pObj)` and records at its line 110
//      that "0x10024020 takes one" pushed argument.
//    * 0x10025523 (inside 0x10024090, same TU, so the REGISTER entry) --
//      big24090.cpp reads it as "push the object on the deferred list"
//      with the object in %eax.
//    * and its address is stored into the draw-slot table 0x1008ce94..a0,
//      whose other occupants are 0x1003dc80, 0x1000d9f0, 0x10073140,
//      0x1000f650 and 0x10024090, all `void (*)(Obj*)`.
//
//  Return value: none.  %eax holds the element pointer at the `ret`
//  (0x10024058) but the object pointer is what every caller has, and
//  0x1001428e / 0x1006f16c / 0x1006f527 all discard %eax; the slot table
//  it is installed in is a void slot.
//
//  NOTHING IS UNDECODED.  All 28 instructions are accounted for.
//
// ======================================================================= //

	// ------------------------------------------------------------------ //
	//  0x10024027..0x1002403a and 0x10024059..0x10024083 -- 0x1000a6a0
	//  INLINED.  This is the pool allocator, and it is not a guess: the
	//  out-of-line copy at 0x1000a6a0 is
	//
	//    1000a6a?  mov 0x18(%ecx),%eax     ; p = pPool->m_pFree18
	//    1000a6ac  je  0x1000a6b7
	//    1000a6ae  mov (%eax),%edx         ; pop
	//    1000a6b3  mov %edx,0x18(%ecx)
	//    1000a6b6  ret                     ; returns %eax
	//    1000a6b7  mov %ecx,(%esp)         ; refill
	//    1000a6ba  mov 0x8(%ecx),%ecx
	//    1000a6bd  mov %ecx,0x4(%esp)
	//    1000a6c1  call 0x1007fbf0
	//    1000a6c6  mov 0x10(%esp),%ecx     ; reload the pool pointer
	//    1000a6ca  mov 0x18(%ecx),%eax
	//    1000a6cd  test %eax,%eax
	//    1000a6cf  jne 0x1000a6ae          ; back to the pop
	//    1000a6d1  add $0xc,%esp
	//    1000a6d4  xor %eax,%eax           ; and NULL if it is still empty
	//    1000a6d6  ret
	//
	//  and these bytes are that, instruction for instruction, with the
	//  pool folded to its absolute address and the caller's own stores
	//  reached through the merge at 0x1002403c rather than a `ret`:
	//
	//    10024027  mov 0x1008d4fc,%edx     ; p = g_1008d4fc
	//    1002402d  test %edx,%edx
	//    1002402f  mov %eax,%ecx           ; keep pObj across the call
	//    10024031  je  0x10024059          ; -> refill
	//    10024033  mov (%edx),%eax         ; pop
	//    10024035  mov %eax,0x1008d4fc
	//    1002403a  mov %edx,%eax           ; pElem = p
	//    1002403c  ...the stores...
	//   refill:
	//    10024059  mov 0x1008d4ec,%edx     ; g_1008d4e4.m_nGrow08
	//    1002405f  movl $0x1008d4e4,(%esp)
	//    10024066  mov %edx,0x4(%esp)
	//    1002406a  mov %ecx,0x8(%esp)      ; spill pObj
	//    1002406e  call 0x1007fbf0         ; THE ONE CALL
	//    10024073  mov 0x1008d4fc,%edx
	//    10024079  mov 0x8(%esp),%ecx      ; reload pObj
	//    1002407d  test %edx,%edx
	//    1002407f  jne 0x10024033          ; -> the pop
	//    10024081  xor %eax,%eax           ; still empty: pElem = NULL
	//    10024083  jmp 0x1002403c          ; and store through it anyway
	//
	//  Two things to read off that, both of them source facts:
	//
	//  (a) THE POP IS SKIPPED ON THE NULL PATH, so the "if (p)" round the
	//      pop is real and is not folded into the refill's own test --
	//      exactly the shape big124a4.cpp writes for the same inline at
	//      0x100142e6/0x100144a5 (`jne 0x100142f4` into the pop,
	//      `xor %eax,%eax` then a jump past it).
	//
	//  (b) THE NULL ARM FALLS INTO THE STORES.  0x10024083 jumps to
	//      0x1002403c, which is `mov %ecx,(%eax)` with %eax = 0.  The
	//      shipped code stores through NULL when the pool cannot grow.
	//      This is the "new with a constructor" shape: a
	//      null arm that materialises zero and is then written through is
	//      an allocator whose result the source does not test.  So there
	//      is no `if (!pElem) return;` in the original, and adding one
	//      would cost a branch the bytes do not have.
	//
	//  The refill's arguments are `(&g_1008d4e4, g_1008d4e4.m_nGrow08)`,
	//  which is d3dbatchc.cpp's recorded prototype for 0x1007fbf0 and
	//  the same pair big124a4.cpp passes at 0x100144a5.
	// ------------------------------------------------------------------ //


	// ------------------------------------------------------------------ //
	//  0x1002403c..0x10024054 -- THE PUSH.  Four stores, in emission
	//  order, and the order IS source order: each of the two loads goes
	//  into %edx and is consumed before the next, so nothing here is the
	//  scheduler's choice.
	//
	//    1002403c  mov %ecx,(%eax)         ; pElem->m_pOwner = pObj
	//    1002403e  mov 0x1008d4bc,%edx
	//    10024044  mov %edx,0x14(%eax)     ; pElem->m_pNext14 = g_1008d4bc
	//    10024047  mov 0x1008b874,%edx
	//    1002404d  mov %edx,0x10(%eax)     ; pElem->m_nClipFlags10
	//                                      ;             = g_1008b874
	//    10024050  mov %eax,0x1008d4bc     ; g_1008d4bc = pElem
	//
	//  +0x04, +0x08 and +0x0c are NOT written -- big124a4.cpp's copy of
	//  the same push writes all six fields (0x10014314 +0x04,
	//  0x10014320 +0x08, 0x1001431d +0x0c, 0x10014329 +0x10), so the
	//  three missing stores here are a source fact about THIS list, not
	//  an oversight: the deferred-draw consumer at 0x10025820 reads only
	//  +0x00, +0x10 and +0x14.
	// ------------------------------------------------------------------ //

	// 0x10024055 `add $0xc,%esp` / 0x10024058 `ret`.  One exit.


// *********************************************************************** //
//
//  FUNCTION A -- THE FIVE INSTRUMENTS
//
// ----------------------------------------------------------------------- //
//  (0) THE CALL CHECK
// ----------------------------------------------------------------------- //
//
//  The call sequence over 0x10024020..0x10024085, shipped against
//  sub_10024020 in our big24024.obj.
//
//  EXPECTED: shipped 1, ours 1, in agreement 1 (by resolved target,
//  because 0x1007fbf0 is declared and reached by name).  Predicted
//  residuals: NONE.  There is no `_chkstk` (the frame is 12 bytes), no
//  CRT intrinsic and no indirect call.
//
//  /Oi- JUDGEMENT: IRRELEVANT HERE.  A calls nothing icl holds as an
//  intrinsic -- no sqrt, fabs, tan, atan or memcpy, and no `rep movs`
//  anywhere in the 101 bytes (census below: zero string instructions).
//  Adding the flag would change nothing either way, so this TU does not
//  need to be on D3D_OI_TUS on A's account.
//
//  /Qrcd JUDGEMENT: IRRELEVANT HERE.  A contains no floating point at
//  all -- zero x87 instructions, zero float-to-int conversions, zero
//  `fistpl` and no .rdata constant of any kind.  The twelve-instruction
//  truncation idiom cannot appear.
//
//  /Qip IS REQUIRED, and it is the one flag that matters.  The two-entry
//  pair 0x10024020/0x10024024 only exists at external linkage under
//  /Qip, and the caller at 0x10025523 reaches the REGISTER entry, which
//  means 0x10024020 and 0x10024090 must be in ONE of our
//  translation units.  Without /Qip we emit one stack-ABI entry, and B's
//  own call sequence then shows one shipped-only 0x10024024 against one
//  ours-only 0x10024020 -- which is exactly the residual
//  src/engine/d3d/big24090.cpp already predicts for itself at its lines
//  978-981.
//
// ----------------------------------------------------------------------- //
//  (1) FRAME ARITHMETIC, DECOMPOSED EXACTLY, SLOTS DUMPED
// ----------------------------------------------------------------------- //
//
//  `sub $0xc,%esp` at 0x10024024 = 12 bytes, and every one of them is
//  named.  A is FRAMELESS (no `push %ebp`), so the slots are %esp-
//  relative and there are exactly three:
//
//      (%esp)      4   outgoing argument 0 of 0x1007fbf0
//                      0x1002405f  movl $0x1008d4e4,(%esp)
//      0x4(%esp)   4   outgoing argument 1 of 0x1007fbf0
//                      0x10024066  mov %edx,0x4(%esp)
//      0x8(%esp)   4   the SPILL of pObj across the call
//                      0x1002406a  mov %ecx,0x8(%esp)   (store)
//                      0x10024079  mov 0x8(%esp),%ecx   (reload)
//                      ------
//                     12 = 0xc.  No remainder, no unused slot.
//
//  Two facts fall straight out of that and out of nothing else:
//
//   * pObj IS THE ONLY VALUE LIVE ACROSS THE CALL.  One spill slot, used
//     once each way.  So the source cannot be holding the element
//     pointer, the grow count or the list head in a local across the
//     refill -- they are all recomputed after it (0x10024073 reloads
//     0x1008d4fc; 0x1002403e and 0x10024047 load the two globals only in
//     the store block).
//   * THE ARGUMENT BLOCK IS PRE-SUBTRACTED, NOT PUSHED.  Eight bytes
//     reserved in the prologue and written with `mov`, which is the /O2
//     tell and confirms two arguments, not three -- so
//     0x1007fbf0's prototype here is d3dbatchc.cpp's `(pool, count)` and
//     the third slot is a spill, not a third argument.  A three-argument
//     reading would need 0xc of outgoing block and leave no spill slot,
//     and the reload at 0x10024079 proves the slot is a spill.
//
//  Sibling check: 0x1000a6a0, the OUT-OF-LINE copy of the same
//  allocator, also does `sub $0xc,%esp` -- (%esp) and 0x4(%esp) for the
//  same two arguments, and it reloads its POOL pointer from 0x10(%esp),
//  its own parameter home slot, because it has a pool pointer to keep
//  and no object.  Same 12 bytes, different third user.  That is the
//  strongest single confirmation that these 101 bytes are that function
//  inlined with the pool folded to a constant.
//
// ----------------------------------------------------------------------- //
//  (2) THE INSTRUCTION CENSUS -- 28 OF 28, DECOMPOSED BY OPERAND
// ----------------------------------------------------------------------- //
//
//  There are 28 instructions in 0x10024020..0x10024085.  Every
//  one is named, and the `mov` row -- the one row most easily waved
//  through -- is broken out per operand.
//
//    mov     18
//        1  mov 0x4(%esp),%eax          the /Qip stack entry
//        1  mov %eax,%ecx               pObj -> the callee-saved-free reg
//        2  mov 0x1008d4fc,%edx         the TWO `pElem = g_1008d4fc`
//                                       (0x10024027 and 0x10024073) --
//                                       two source reads, one each side
//                                       of the refill.  A single read
//                                       hoisted into a local would give
//                                       one, and a `while (!pElem)` loop
//                                       would give one plus a back edge.
//        1  mov (%edx),%eax             the pop: the free link at +0x00
//        1  mov %eax,0x1008d4fc         the pop's store
//        1  mov %edx,%eax               pElem = p
//        1  mov %ecx,(%eax)             m_pOwner          = pObj
//        1  mov 0x1008d4bc,%edx         read the list head
//        1  mov %edx,0x14(%eax)         m_pNext14         = head
//        1  mov 0x1008b874,%edx         read the clip flags
//        1  mov %edx,0x10(%eax)         m_nClipFlags10    = flags
//        1  mov %eax,0x1008d4bc         head              = pElem
//        1  mov 0x1008d4ec,%edx         g_1008d4e4.m_nGrow08
//        1  movl $0x1008d4e4,(%esp)     &g_1008d4e4, AS AN IMMEDIATE --
//                                       so the source names the pool by
//                                       address at the call, not through
//                                       a `Pool* p = &g_...` local
//        1  mov %edx,0x4(%esp)          argument 1
//        1  mov %ecx,0x8(%esp)          spill pObj
//        1  mov 0x8(%esp),%ecx          reload pObj
//                                       -----
//                                        18
//    test     2   the two source tests: `if (!pElem)` at 0x1002402d and
//                 the post-refill `if (pElem)` at 0x1002407d.  THERE ARE
//                 ONLY TWO for THREE source predicates, because the
//                 first `if (pElem)` guarding the pop is proved by the
//                 fall-through of the first test and icl deletes it --
//                 the same range propagation VC6 shows in its
//                 "four branches off one cmp".  A third test would mean
//                 the pop is guarded independently, i.e. a different
//                 source shape.
//    je       1   0x10024031, the `if (!pElem)` branch to the refill
//    jne      1   0x1002407f, back INTO the pop -- so the pop is shared
//                 by both paths and is not duplicated
//    jmp      1   0x10024083, the null arm merging into the store block
//    xor      1   0x10024081, `pElem = NULL`
//    call     1   0x1002406e, 0x1007fbf0
//    sub      1   0x10024024, the frame
//    add      1   0x10024055, the frame
//    ret      1   0x10024058, the ONE exit
//              -----
//               28
//
//  RESIDUE: NONE.  And the row that carries the structure is the branch
//  count, three:
//
//   * a FOURTH branch would be `if (!pElem) return;` -- the shipped code
//     has no such test, which is why the null arm stores through NULL;
//   * a SECOND `ret` (or a second `add $0xc,%esp`) would be a duplicated
//     epilogue, i.e. an early return;
//   * three more `mov`s into +0x04/+0x08/+0x0c would be the three
//     element fields this list does not use, and big124a4.cpp's copy of
//     the same push has exactly those three extra stores. Their absence
//     here is what makes this a DIFFERENT list, not the bucket list.
//
// ----------------------------------------------------------------------- //
//  (3) THE LITERAL POOL
// ----------------------------------------------------------------------- //
//
//  A REFERENCES NO CONSTANT POOL AT ALL.  Its only immediates are
//  $0x1008d4e4 (an address) and $0 (as `xor`).  So the pool instrument
//  is vacuous for A -- and that is itself a reading: no float literal
//  means no float statement, and in particular NO DEAD `1.0f`, so by
//  the dead-1.0f division rule A performs zero divisions.  Stated
//  explicitly so that nobody later reads the absence as an omission.
//
// ----------------------------------------------------------------------- //
//  (4) SIBLING CORRESPONDENCE
// ----------------------------------------------------------------------- //
//
//  Three siblings, and the correspondence was done instruction for
//  instruction, not by eye:
//
//   * 0x1000a6a0 (the allocator OUT OF LINE, 55 bytes).  Quoted in full
//     in the body comment above.  Identical control flow, identical
//     twelve-byte frame, identical `xor %eax,%eax` null answer, and the
//     pool reached as `0x8(%ecx)`/`0x18(%ecx)` where A has the folded
//     absolutes 0x1008d4ec/0x1008d4fc -- which is what FIXES the two
//     member offsets rather than assuming them.
//   * 0x100142e6 + 0x100144a5 (the same allocator INLINED, in
//     big124a4.cpp).  Same `je` to a sunk refill block, same
//     `movl $0x1008d4e4,(%esp)` + `mov <grow>,0x4(%esp)` + `call
//     0x1007fbf0`, same reload-test-`jne` back into the pop, same
//     `xor %eax,%eax` falling into the caller's first store.  A's
//     0x10024083 -> 0x1002403c is that file's 0x100144d7 -> 0x100142fd.
//   * 0x10025820 (the CONSUMER, 97 bytes).  It is the proof that A's
//     three stores are the three fields that matter: it reads +0x10 into
//     g_1008b874 (0x10025846), +0x14 as the next link (0x10025843),
//     +0x00 as the object (0x1002584b), draws through 0x1002803c, then
//     splices the whole chain back onto the pool free list and clears
//     the head.  Nothing reads +0x04, +0x08 or +0x0c off THIS list, and
//     A writes none of them.
//
// ----------------------------------------------------------------------- //
//  WHAT A's CHECK CANNOT SEE, AND A's COIN-FLIPS
// ----------------------------------------------------------------------- //
//
//  Call-free stretches (78 of 101 bytes, 77.2%, before the call and 18
//  bytes, 17.8%, after it) are carried entirely by the census above; at
//  28 instructions the census is exhaustive, so nothing in A rests on
//  the call.
//
//  COIN-FLIP 1 -- THE POP'S GUARD.  Both of these give the shipped
//  bytes, because icl proves the first form's inner test redundant:
//
//      pElem = g_1008d4fc;
//      if (!pElem) { refill(); pElem = g_1008d4fc; }
//      if (pElem) g_1008d4fc = *(Elem**)pElem;          <- written
//
//      pElem = g_1008d4fc;
//      if (pElem) { g_1008d4fc = *(Elem**)pElem; }
//      else { refill(); pElem = g_1008d4fc;
//             if (pElem) g_1008d4fc = *(Elem**)pElem; } <- duplicates
//                                                          the pop
//  The second duplicates the pop and the shipped code shares it
//  (0x1002407f jumps back INTO 0x10024033), so the first is what is
//  written.  DOES NOT MOVE A CALL.
//
//  COIN-FLIP 2 -- WHETHER THE ALLOCATOR IS SPELLED AS A HELPER.  Written
//  out inline above, matching big124a4.cpp's spelling of the same
//  inline.  Spelling it as `pElem = d3d_PoolAlloc(&g_1008d4e4);` with a
//  helper declared-but-defined-in-header would be identical IF icl
//  inlines it and a WRONG EXTRA CALL if it does not -- so this one DOES
//  MOVE A CALL, and the inline spelling is the safe one.  (0x1000a6a0
//  exists as an out-of-line function and three sites in .text call it,
//  so the original source certainly had a helper; but the four sites
//  that inline it -- 0x1000b865, 0x1000d356, 0x100142e6, 0x10024027 --
//  cannot be told apart from a hand-inlined body.)
//
//  COIN-FLIP 3 -- THE TYPE OF 0x1008d4bc.  Declared `Elem24024*` here
//  because A and 0x10025820 both use it as a list head.  d3dr5b.cpp and
//  big2d1c0.cpp declare it `uint32` because those functions only zero
//  it.  Under relocation masking the two spellings emit identical bytes
//  (relocations are masked), so this is free -- but it must
//  be reconciled before the two declarations meet in one link.  DOES
//  NOT MOVE A CALL.
//
//  NOT A COIN-FLIP, RECORDED BECAUSE IT LOOKS LIKE ONE: the store order
//  +0x00, +0x14, +0x10, head.  Each of the two global loads goes into
//  %edx and is consumed by the very next instruction, so the scheduler
//  had no freedom; and putting the clip-flag store before the next-link
//  store would swap the two `mov 0x1008xxxx,%edx` loads, which is
//  visible.  Source order is emission order here.


// *********************************************************************** //
//
//  FUNCTION C -- 0x10025640 .. 0x10025674   (53 bytes, 0 calls)
//
//  IN SCOPE because the row's last twelve bytes ARE its /Qip stack
//  entry, and because it is the callee whose value the overdraw meter
//  accumulates -- so decoding it is what makes B's meter arithmetic
//  checkable rather than asserted.  It is enumerated as
//  "0x1002564c, 52", which is again the
//  REGISTER entry: the real function starts twelve bytes earlier at
//  0x10025640 (three argument loads, so 12 and not 4), its body ends at
//  0x10025675 and the 11 bytes to 0x10025680 are lea-nop padding.  The
//  enumerated 52 = 41 body bytes from the register entry + 11 padding.
//
//  0x10025640's STACK entry is DEAD in this image: the only caller is
//  0x100254cf, inside B, which reaches the register entry.
//
//  Extent: 1 ret (0x10025674), 0 prologues (frameless), 0 stray
//  branch targets -- it contains no branch at all.
//
//  TWICE THE SIGNED AREA of the 2-D triangle (A, B, C), fields +0x00 = x
//  and +0x04 = y (the first two floats of the 32-byte draw vertex).  The
//  five x87 pairs, with the DE forms read as `objdump -M intel` gives
//  them (`de e9` is modrm reg=5 = plain FSUBP ST(1),ST -> ST(1) =
//  ST(1) - ST(0); `de c1` is reg=0 = FADDP; the reg field, not the rm
//  field, picks the operation, which is the classic trap):
//
//    1002564c  fld   0x4(%edx)   ; B.y
//    1002564f  fmul  (%eax)      ; * A.x            -> acc
//    10025651  fld   0x4(%eax)   ; A.y
//    10025654  fmul  (%edx)      ; * B.x
//    10025656  fsubp %st,%st(1)  ; acc -= A.y*B.x
//    10025658  fld   0x4(%eax)   ; A.y
//    1002565b  fmul  (%ecx)      ; * C.x
//    1002565d  faddp %st,%st(1)  ; acc += A.y*C.x
//    1002565f  fld   (%eax)      ; A.x
//    10025661  fmul  0x4(%ecx)   ; * C.y
//    10025664  fsubp %st,%st(1)  ; acc -= A.x*C.y
//    10025666  fld   0x4(%ecx)   ; C.y
//    10025669  fmul  (%edx)      ; * B.x
//    1002566b  faddp %st,%st(1)  ; acc += C.y*B.x
//    1002566d  fld   (%ecx)      ; C.x
//    1002566f  fmul  0x4(%edx)   ; * B.y
//    10025672  fsubp %st,%st(1)  ; acc -= C.x*B.y
//    10025674  ret               ; the answer in %st(0)
//
//  Expanded that is
//      A.x*B.y - A.y*B.x + A.y*C.x - A.x*C.y + B.x*C.y - B.y*C.x
//  which is the shoelace 2*Area((A,B,C)) term for term:
//      (B.x-A.x)(C.y-A.y) - (C.x-A.x)(B.y-A.y)
//  expands to exactly those six products and nothing else.  So the
//  return value is 2*signed area, positive for one winding -- and B's
//  0x10025508 `fld 0.5f ; fmull fTotal` is the halving that turns the
//  sum of fan triangles into the polygon's area.  B's meter arithmetic
//  is therefore CHECKED, not assumed.
//
//  Emission order is SOURCE order here, and that is a real statement:
//  the rule "an FP sum emits its terms backwards" applies to a chain
//  of ADDS, which icl reassociates freely.  This chain alternates minus
//  and plus, so it is not associative and icl must evaluate it left to
//  right -- which is why the six products come out in the order the
//  three parenthesised pairs are written.  A different grouping of the
//  shoelace (the cyclic (A,B) + (B,C) + (C,A) form, say) would emit the
//  B/C pair second and the C/A pair third; the shipped code emits the
//  A/C pair second.  That fixes the grouping without a single call.
//
//  CENSUS, 21 of 21, no residue (21 instructions in
//  0x10025640..0x10025675): 3 mov (the three /Qip argument loads),
//  6 fld, 6 fmul, 3 fsubp, 2 faddp, 1 ret.  That is
//  6 products, 5 combining operations and one exit -- a seventh product
//  or a temporary spilled to the frame would move at least two rows, and
//  there is no `sub %esp` at all, so there is no frame and no temporary.
//
// *********************************************************************** //

// Only +0x00 and +0x04 of the vertex are read, so C is declared over the
// two floats it actually touches.  B passes it &aVerts[0], &aVerts[i+1]
// and &aVerts[i+2], which are 32-byte RVerts (big24090.cpp's
// RVert24090), and 0x1002564c's own bytes cannot see past +0x08.


//  C's ONE coin-flip, stated with both spellings,
//  and it moves NO call (C makes none):
//
//    WHICH OPERAND OF EACH PRODUCT IS THE `fld`.  Every product is
//    `fld <mem> ; fmul <mem>`, and multiplication is commutative, so
//    each of the six could be written either way round:
//
//        (pB->y * pA->x  ...)      <- written; fld the LEFT operand
//        (pA->x * pB->y  ...)      <- fld the RIGHT operand
//
//    The six choices are consistent with each other under the
//    "fld the left operand" reading, which is why that spelling is the
//    one written; under the other reading the six products would read
//    A.x*B.y, B.x*A.y, C.x*A.y, C.y*A.x, B.x*C.y, B.y*C.x, which is the
//    same six products with three of the six factor orders flipped and
//    is not internally consistent as a source style.  One build settles
//    it; nothing else in this file depends on it.


// *********************************************************************** //
//
//  FUNCTION B -- 0x10024090 .. 0x1002563f (5,552 bytes, 4 calls)
//  AN INDEPENDENT AUDIT OF src/engine/d3d/big24090.cpp
//
//  B is the large function in this row and it is ALREADY reconstructed.
//  Re-writing it would produce a second copy of the same 1,133 lines and
//  prove nothing, so what is done here instead is what a second decode is
//  actually worth: every quantitative claim in that file was recomputed
//  from the shipped bytes, without reading its numbers first, and the
//  results are below.  Two things came out of it -- a full confirmation,
//  and one instrument that file did not run.
//
// ----------------------------------------------------------------------- //
//  AUDIT 1 -- THE CALL SEQUENCE.  CONFIRMED.
// ----------------------------------------------------------------------- //
//
//      the call count over 0x10024090..0x10025640:
//        5,552 bytes, 4 calls (4 direct, 0 indirect), 1388 bytes per call
//
//      distinct callees: 4 of 4 direct
//        0x100241fc -> 0x1000d9b0
//        0x1002547a -> 0x10014580
//        0x100254cf -> 0x1002564c
//        0x10025523 -> 0x10024024
//
//  Identical to big24090.cpp's list, address for address, in order.  Four
//  distinct callees out of four calls is the best discrimination ratio a
//  call check can give, which is worth saying because a
//  table-driven function maximises witnessing and minimises
//  discrimination -- B is the opposite case, minimal witnessing and
//  maximal discrimination.
//
//  1 + 4 = 5, which is the bogus row's own 5 calls.  That arithmetic --
//  the call count per candidate function must sum to the row's own
//  total -- is the last independent confirmation of the split.
//
//  PREDICTED FIGURE AND RESIDUALS FOR B, which big24090.cpp states and
//  this audit endorses:
//    * 4 shipped, 4 ours, 4 in agreement IF A, B and C are compiled in
//      ONE translation unit with /Qip, because two of B's four calls go
//      to /Qip REGISTER entries (0x1002564c and 0x10024024) and only a
//      same-TU caller reaches those;
//    * 2 of 4, with 2 shipped-only and 2 ours-only, if they are not --
//      the `call f+4` / `call f+12` class, one shipped-only
//      and one ours-only per site, the same function and not a wrong
//      callee.
//  No `_chkstk` residual: the frame is 2,704 bytes and there is still no
//  `mov $imm,%eax ; call 0x100805a0` -- the census has four calls and
//  none of them is 0x100805a0.
//
//  /Qrcd JUDGEMENT FOR B: **NEEDED.**  The census carries `fistpl` 30 and
//  every one of them is a `fstps <float slot> ; flds <same slot> ;
//  fistpl <int slot>` triple -- 6 planes x 5 interpolated colour bytes.
//  That is exactly the /Qrcd family (`big2b1b0 1864 -> 807`),
//  so at plain /O2 B's census should show `shld`/`neg`/`or`/`shr`/`and`/
//  `xor`/`cmp`/`test`/`jge`/`jle` rows all appearing at exactly 30, and
//  the fix is the flag, not the source.  B's TU belongs on
//  build/engine.sh's D3D_RCD_TUS list.
//
//  /Oi- JUDGEMENT FOR B: **DO NOT ADD IT**, and unusually the reason is
//  measured off the shipped side rather than feared.  The list of
//  CRT intrinsics is sqrt 0x1007fc98, fabs 0x1007fc90, tan 0x1007fdf4,
//  atan 0x1007fe1c and memcpy 0x100805d0.  B calls NONE of them: its
//  four callees are 0x1000d9b0, 0x10014580, 0x1002564c and 0x10024024,
//  all module-internal.  So /Oi- has nothing to restore here -- it cannot
//  raise 4 of 4 to anything -- while the census shows ZERO `rep movs` of
//  any width in 5,552 bytes even though the clipper does a whole 32-byte
//  vertex copy (`*pOut = *pPrev`) once per plane: icl compiled all six of
//  those as plain `mov` runs, which is part of the 585-strong `mov` row.
//  Turning intrinsic expansion off is therefore all downside for B: it
//  risks turning six struct copies into six `memcpy` CALLS, which would
//  be six ours-only rows against a benefit of zero.  This is a trade
//  to measure rather than assume, and measured on B it goes
//  the other way from big48180's.
//
// ----------------------------------------------------------------------- //
//  AUDIT 2 -- THE CENSUS.  CONFIRMED, ROW FOR ROW.
// ----------------------------------------------------------------------- //
//
//  Counted mechanically over 0x10024090..0x1002563f (1,572 instructions)
//  before looking at big24090.cpp's table.  Every one of its 30-odd
//  itemised rows agrees exactly:
//
//    movzbl 60   fildl 60   fistpl 30   fstps 61   fsts  2   flds 108
//    faddp  40   fadds 33   fsubs 28    fsub   6   fsubrs 2  fsubrp 2
//    fmul   51   fmuls 12   fmulp  6    fdivp  6   fdivrs 1
//    fchs    8   fcomps 6   fcomp 12    fstp  16   fld    2
//    jbe     7   jb    10   jae    1
//    sar     6   shl   17   imul   2    movzwl 5   and    1
//    fstpl   3   fldl   1   fadd   1    fmull  1   faddl  1
//    call    4   push   1   pop    1    ret    1
//
//  and the rows it does not itemise, which are register/frame/branch
//  noise, are mov 585, add 69, je 42, sub 37, lea 35, test 33, cmp 31,
//  fxch 18, fnstsw 18, sahf 18, inc 13, xor 13, dec 10, jne 10, jle 8,
//  jmp 7, jl 7, nop 2.  Total 1,572.
//
//  The load-bearing rows, restated because they are what makes the
//  six-plane reading unfalsifiable: movzbl 60 = fildl 60 = 6 x 5 x 2 and
//  fistpl 30 = 6 x 5 -- five interpolated bytes per plane, each needing
//  two byte loads and two integer-to-float conversions and producing one
//  conversion back.  A fifth or seventh plane moves all three rows at
//  once by 10, 10 and 5; a fourth or sixth interpolated byte moves them
//  by 12, 12 and 6.  Neither is within the counting error, because there
//  is no counting error.
//
// ----------------------------------------------------------------------- //
//  AUDIT 3 -- THE FRAME.  CONFIRMED, SLOT FOR SLOT.
// ----------------------------------------------------------------------- //
//
//  `sub $0xa90,%esp` = 2,704.  Every distinct frame displacement in the
//  5,552 bytes was extracted mechanically -- 81 of them -- and they land
//  exactly where big24090.cpp says:
//
//    -0xa70,-0xa68,...,-0x988   THIRTY slots at an 8-byte stride, each
//                               referenced exactly twice (the `fstps`
//                               and the `flds` on the next instruction).
//                               (0xa70-0x988)/8 + 1 = 30.  30 = 6 x 5.
//    -0x984                     7 references -- nVerts, tucked into the
//                               unused upper half of the last temp
//    -0x980                     the vertex array base
//    -0x5c0                     8 references -- pVerts
//    -0x5bc,-0x4e0,-0x404,      SIX clip blocks, 4 references each,
//    -0x328,-0x24c,-0x170       stride 0xdc exactly (0x5bc-0x4e0 = 0xdc)
//    -0x4f4..-0x4e4  (5)        block 1's five `fistpl` landing slots
//    -0x418..-0x408  (5)        block 2's
//    -0x33c..-0x32c  (5)        block 3's
//    -0x260..-0x250  (5)        block 4's
//    -0x184..-0x174  (5)        block 5's
//    -0xa8..-0x98    (5)        block 6's -- and -0x170 + 0xc8 = -0xa8,
//                               which is what proves the 0xdc stride
//                               decomposes as 200 flags + 20 slots and
//                               not as 204 + 16
//    -0x90                      120 references -- the shared `fildl` slot
//    -0x7c,-0x78,-0x70,-0x68,   the named/spilled scalars, -0x70 being
//    -0x64,-0x60,-0x58,-0x28,   the DOUBLE fTotal (fstpl/fldl/fadd/
//    -0x8                       fmull/faddl all name it)
//    (%esp),0x4(%esp),0x8(%esp) the ebx/esi/edi spills, and 0x4/0x8 also
//                               serve the two 8-byte outgoing argument
//                               blocks after `add $0xfffffff8,%esp`
//
//    12 + 20 + 240 + 960 + 4 + 1320 + 4 + 144 = 2,704 = 0xa90.  Exact.
//
//  The two array bounds are read off the frame and off nothing else:
//  THIRTY vertices (0x980 - 0x5c0 = 0x3c0 = 960 =
//  30 x 0x20) and FIFTY flags per block (0xc8 = 200 = 50 x 4).
//
// ----------------------------------------------------------------------- //
//  AUDIT 4 -- THE LITERAL POOL.  big24090.cpp DID NOT RUN THIS ONE, AND
//  IT INDEPENDENTLY CONFIRMS THE SIX PLANES AND THE SIX DIVISIONS.
// ----------------------------------------------------------------------- //
//
//  B's constant run is 0x100a5d34..0x100a5da0, dumped dword by dword out
//  of the image and cross-referenced against an `objdump` of the WHOLE of
//  .text to establish which slots are live and which are dead:
//
//    0x100a5d34  0000803f   1.0f     DEAD (0 references in all of .text)
//    0x100a5d38  0000803f   1.0f     live  0x1002413f  fdivrs
//    0x100a5d3c  00000000   0.0f     live  0x10025488  fld  (fTotal = 0)
//    0x100a5d40  0000003f   0.5f     live  0x10025508  fld  (the halving)
//    0x100a5d44  acc527b7  -1e-5f    live  0x1002435b  plane 1 (near)
//    0x100a5d48  0000803f   1.0f     DEAD
//    0x100a5d4c  acc527b7  -1e-5f    live  0x1002465f  plane 2 (left)
//    0x100a5d50  0000803f   1.0f     DEAD
//    0x100a5d54  acc527b7  -1e-5f    live  0x10024971  plane 3 (top)
//    0x100a5d58  0000803f   1.0f     DEAD
//    0x100a5d5c  acc527b7  -1e-5f    live  0x10024c6f  plane 4 (right)
//    0x100a5d60  0000803f   1.0f     DEAD
//    0x100a5d64  acc527b7  -1e-5f    live  0x10024f71  plane 5 (bottom)
//    0x100a5d68  0000803f   1.0f     DEAD
//    0x100a5d6c  acc527b7  -1e-5f    live  0x1002525b  plane 6 (far)
//    0x100a5d70  0000803f   1.0f     DEAD
//    0x100a5d74  acc52737  +1e-5f    live  0x10025620  sunk arm, plane 6
//    0x100a5d78  00000000   0.0f     live  0x10025635  sunk arm, plane 6
//    0x100a5d7c  acc52737  +1e-5f    live  0x100255f0  sunk arm, plane 5
//    0x100a5d80  00000000   0.0f     live  0x10025607  sunk arm, plane 5
//    0x100a5d84 /0x100a5d88          live  0x100255c0 /0x100255d7  plane 4
//    0x100a5d8c /0x100a5d90          live  0x10025590 /0x100255a7  plane 3
//    0x100a5d94 /0x100a5d98          live  0x10025560 /0x10025577  plane 2
//    0x100a5d9c /0x100a5da0          live  0x1002553a /0x1002554f  plane 1
//
//  FOUR readings come out of that, and each of them is independent of
//  every call, frame slot and branch:
//
//  (a) SEVEN DEAD 1.0f, AND B PERFORMS SEVEN DIVISIONS.  The
//      rule -- "every float division emits a DEAD 1.0f next to the live
//      one" -- makes the dead slots a division counter.  There are seven
//      (0x100a5d34, and 0x100a5d48/50/58/60/68/70), and the census has
//      `fdivp` 6 + `fdivrs` 1 = 7.  Exact, and in the right places: the
//      one paired with a LIVE 1.0f is the reciprocal-w division in the
//      inlined camera transform (0x1002413f), and the six paired with
//      the six plane epsilons are the six `fT = fNum / fDen`.  This is
//      the rule used predictively, and B is the seventh
//      function to show it.  Nothing here is unexplained: do NOT record
//      these as stray constants.
//
//  (b) SIX PLANES, COUNTED IN THE POOL.  Six -1e-5f in-body epsilons and
//      six +1e-5f/0.0f sunk pairs.  A seventh plane would add one of
//      each; five would remove one of each.  This is a count of the
//      clip arms that does not follow a single branch, which is exactly
//      what is wanted on a function whose call check is blind.
//
//  (c) THE SUNK ARMS' PAIRS ARE EMITTED IN REVERSE PLANE ORDER, and that
//      MAPS EACH SUNK BLOCK TO ITS PLANE.  The in-body epsilons ascend
//      with code address (plane k at 0x100a5d44 + 8k); the sunk pairs
//      DESCEND (plane k at 0x100a5d9c - 8k), so the sunk block at the
//      LOWEST code address (0x1002553a) belongs to plane ONE and the one
//      at the highest (0x10025620) to plane SIX.  Checked against the
//      branch targets and they agree exactly: 0x1002553a jumps to
//      0x1002436c/0x10024376 (plane 1's body), 0x10025560 to
//      0x10024673/0x1002467b (plane 2), 0x10025590 to 0x10024985/
//      0x1002498d (plane 3), 0x100255c0 to 0x10024c83/0x10024c8b
//      (plane 4), 0x100255f0 to 0x10024f85/0x10024f8d (plane 5),
//      0x10025620 to 0x1002526c/0x10025276 (plane 6).  Two independent
//      derivations of the same six-way mapping.  big2b1b0's four
//      `-100000.0f` sit in the sunk blocks' emission order; here the order is the REVERSE of the blocks'
//      code order, so the rule to carry forward is that the pool records
//      SOURCE order and the block placement is what got reversed.
//
//  (d) THE CLIPPER IS AN INLINED CALLEE, NOT WRITTEN OUT IN B's OWN
//      SOURCE.  0.0f (0x100a5d3c) and 0.5f (0x100a5d40) sit BEFORE all
//      twelve clip constants in the pool, yet they are loaded at
//      0x10025488 and 0x10025508 -- after the whole clipper in code
//      order.  A single source text emitting its literals in order
//      cannot do that.  What can is an inlined body whose literals are
//      appended after the enclosing function's own: B's own source
//      (1.0f/w, then `fTotal = 0.0f`, then `* 0.5f`) first, then the
//      inlined clipper's twelve.  big24090.cpp claims the clipper is
//      0x1000f4b0 inlined on other grounds (six identical plane bodies,
//      the shared epsilon set, correspondence with big572b4.cpp); the
//      pool is a fourth, independent witness for it.
//
//  POOL BOUNDARIES, so that the next reader does not mis-attribute:
//    * BELOW: 0x100a5d20..0x100a5d30 is five 255.0f-family constants
//      belonging to the PREVIOUS function (referenced from 0x10023b5a,
//      0x10023bf5, 0x10023c08, 0x10023c20, 0x10023c33), not to B.
//    * ABOVE: 0x100a5da4 and 0x100a5da8 are TWO 1.0f with ZERO
//      references anywhere in .text, and the next live constant is
//      0x100a5dac (0.001f, referenced from 0x100258ce inside the
//      function at 0x10025890).  Two adjacent dead 1.0f with no live
//      partner between them is the leading half of two division pairs
//      whose live halves are not loaded -- the same pattern as
//      big28030's "trailing 1.0f", which belongs to the NEXT function's
//      transform-divide pair, seen twice over.  They CANNOT be attributed
//      to a statement and none is invented:
//      they are recorded here by address as belonging outside B.
//
// ----------------------------------------------------------------------- //
//  AUDIT 5 -- SIBLING CORRESPONDENCE.  ACCEPTED AS WRITTEN, PLUS ONE.
// ----------------------------------------------------------------------- //
//
//  big24090.cpp's plane-by-plane diff against big572b4.cpp (six classify
//  sites, six epsilon pairs, the byte order r/g/b/a/spec at +0x12/+0x11/
//  +0x10/+0x13/+0x17, the `pIn = pOutStart ; nIn = pOut - pOutStart ;
//  pOut += nIn` tail with the same dead sixth `pOut += nIn`) was not
//  re-derived here; it is a diff against a file, and re-doing it would
//  only re-read that file.  The pool run above corroborates its epsilon
//  set independently, which is the part that mattered.
//
//  ONE ADDITION.  B's meter arithmetic is now checked end to end,
//  because function C above is decoded rather than assumed:
//    0x10025488  fTotal  = 0.0                       (double, -0x70)
//    0x100254cf  fTotal += 2*Area(v0, v[i+1], v[i+2])   <- C
//    0x100254f9  if (pObj->+0x28->+0x38 & 0x80) fTotal += fTotal;
//    0x10025508  g_1008be30 += 0.5f * fTotal;
//    0x10025523  sub_10024024(pObj);                    <- A
//  So the "Tri area drawn"/"Overdraw" meter accumulates HALF the sum of
//  the fan triangles' twice-areas -- i.e. the true polygon area -- and
//  doubles it for the 0x80 two-pass surfaces.  Both ends of that chain
//  are decoded in THIS file and the middle is decoded in big24090.cpp.
//
// ----------------------------------------------------------------------- //
//  WHAT THE CHECK WITNESSES OVER THE WHOLE ROW
// ----------------------------------------------------------------------- //
//
//  Quoted beside the figure and not instead of it.
//  With 5 calls in 5,672 bytes, the call check constrains the ordered 5-tuple of
//  call targets and NOTHING ELSE; 20 of the row's 5,672 bytes are call
//  instructions, so 99.6% of the row is code the check cannot see.  The
//  call-free stretches, by address:
//
//    0x10024020..0x1002406d     78 B   A's alloc/pop, before its call
//    0x10024073..0x10024085     18 B   A's reload and merge
//    0x10024085..0x1002408f     11 B   padding
//    0x10024090..0x100241fb    364 B   B's gather and inlined transform
//    0x10024201..0x10025479  4,729 B   *** B's ENTIRE SIX-PLANE CLIPPER,
//                                      83.4% of the row in ONE
//                                      unwitnessed stretch ***
//    0x1002547f..0x100254ce     80 B   B's projection loop tail + area
//                                      loop head
//    0x100254d4..0x10025522     79 B   B's area accumulation and meter
//    0x10025528..0x1002563f    280 B   B's epilogue and six sunk arms
//    0x10025640..0x10025675     53 B   C, which contains no call at all
//
//  The 4,729-byte stretch is the whole of the evidence problem, and it is
//  carried by: the frame (30 x 8 temps, 30 x 0x20 vertices, 6 x 0xdc clip
//  blocks, all decomposing to 0xa90 with no remainder), the census (six
//  planes x five bytes forced simultaneously on movzbl 60 / fildl 60 /
//  fistpl 30), the pool (six epsilon pairs, six dead 1.0f, the six-way
//  sunk-block mapping) and the diff against big572b4.cpp.  Four
//  instruments, all four clean.  It is NOT carried by the call check, and
//  nothing should say otherwise.
//
// *********************************************************************** //


// *********************************************************************** //
//
//  SUMMARY FOR THE RECORD
//
// ----------------------------------------------------------------------- //
//  UNDECODED GAPS
// ----------------------------------------------------------------------- //
//
//  NONE in this file.  There is no `// UNDECODED:` banner because there
//  is no undecoded byte:
//    * A  0x10024020..0x10024084   28 of 28 instructions named
//    * C  0x10025640..0x10025674   21 of 21 instructions named
//    * B  0x10024090..0x1002563f   decoded in src/engine/d3d/big24090.cpp,
//         audited here on four instruments, no unattributed row in a
//         1,572-instruction census
//
//  THE ONE THING NOT PINNED, recorded by address
//  rather than explained away: the two 1.0f dwords at
//  0x100a5da4 and 0x100a5da8, unreferenced anywhere in .text.  They sit
//  between B's pool run (which ends at 0x100a5da0) and the next
//  function's (which starts at 0x100a5dac), and they have the shape of
//  the dead halves of two division pairs -- but neither the
//  statement nor even the function they belong to can be named, and
//  inventing one is the wrong move.  They are NOT B's.
//
// ----------------------------------------------------------------------- //
//  A STALE NOTE FOUND WHILE DOING THIS
// ----------------------------------------------------------------------- //
//
//     src/engine/d3d/big24090.cpp's compiler line
//     says "/O2 (with /QIfist)" and its sibling section says the
//     conversions are done "with no fnstcw/fldcw dance anywhere (thirty
//     sites, /QIfist)".  It has since been established that icl 4.0
//     has NO /QIfist -- it parses as /QIf and is silently ignored -- and
//     that the real option is /Qrcd.  The OBSERVATION is right (thirty
//     conversions with no control-word dance); only the flag name is
//     stale.
//
// ----------------------------------------------------------------------- //
//  REMAINING GENUINE COIN-FLIPS, AND WHETHER EACH MOVES A CALL
// ----------------------------------------------------------------------- //
//
//   A-1  the pop's guard, two spellings          DOES NOT move a call
//        (settled by the shipped bytes anyway: 0x1002407f jumps back
//        INTO the shared pop, so the pop is not duplicated)
//   A-2  allocator written out inline vs spelled
//        as a helper call                        **MOVES A CALL** -- the
//        inline spelling is written; a helper icl declines to inline
//        would add one wrong ours-only call
//   A-3  0x1008d4bc declared `Elem*` here and
//        `uint32` in big2d1c0.cpp / d3dr5b.cpp   DOES NOT move a call
//        (relocation-masked; must be reconciled before they share a link)
//   C-1  which operand of each of the six
//        products is the `fld`                   DOES NOT move a call
//        (C makes none)
//   B-1  whether A, B and C share ONE translation
//        unit compiled with /Qip                 **MOVES TWO CALLS** --
//        4 of 4 if they do, 2 of 4 with a matched shipped-only/ours-only
//        pair at 0x100254cf and 0x10025523 if they do not.  This is the
//        only coin-flip in the row with a measurable cost, and one build
//        settles it.
//
// *********************************************************************** //
