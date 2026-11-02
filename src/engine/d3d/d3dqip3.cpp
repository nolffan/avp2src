// Split out of d3dqip2.cpp: these functions need the
// OTHER optimisation flag from the rest of that file.  d3d.ren is a
// mixed build and our file boundaries are invented.
// Everything that is not a function body is
// carried over verbatim so no declaration is lost.
// ********************************************************* //
// *********************************************************************** //
//
//  MODULE:   D3DQIP2 (.CPP)  --  d3d.ren, the tier-Y seam
//
//  PURPOSE:  Tier-Y CALLERS, decoded instruction
//            by instruction from the retail d3d.ren.
//
//  THIS FILE MUST BE COMPILED /O2 /Qip.  Add it to D3D_QIP_TUS in
//  build/engine.sh next to d3dqip.cpp.
//
//  WHAT THIS FILE WAS.  (Historical -- no placeholder is left here or in
//  d3dqip.cpp, whose `qip_Callee10040938` was the last.)  Everything named `qip_Callee<addr>` was a
//  PLACEHOLDER with the right signature and a deliberately wrong body; only
//  the functions named `sub_<addr>` / `C<addr>::sub_<addr>` were real
//  reconstructions.  That worked because the byte comparison masks all four bytes of a
//  call's relocation, so a tier-Y caller reproduces without its callee
//  being written.
//
//  THE PLACEHOLDER RECIPE, and why each part of it is load-bearing
//  (d3dqip.cpp measured all three failures):
//    * it must have a SIDE EFFECT -- a pure callee gets the call deleted
//      outright and the caller comes back as a bare `ret`.  Every stand-in
//      here ends `g_qip_nSink = v;`.
//    * it must be BIG and STRAIGHT-LINE -- a small body, or a loop, gets
//      inlined into the caller by /Qip and the caller is destroyed.
//      QIP_MIX x10 is ~80 statements of straight-line integer code.
//    * it must CONSUME EVERY PARAMETER, or /Qip drops the argument set-up
//      in the caller and the `lea`/`mov $K` in front of the call vanishes.
//    * its RETURN TYPE must match: a float return makes the caller emit
//      `fstp %st(0)` after the call and shifts every later displacement.
//      None of the shipped callers below has a `dd d8` after its register
//      call, so every void stand-in here is void on purpose.
//
//  HOW THE REGISTER ABI WAS READ OFF THE BYTES.  Argument 1 arrives in
//  %eax, 2 in %edx, 3 in %ecx; arguments 4 and up stay on the stack AT
//  THEIR ORIGINAL SLOTS, i.e. the register arguments leave HOLES.  The
//  caller therefore reserves the whole stack argument area and writes only
//  the tail of it.  Two sites nail that:
//
//      0x100231b5  add $0xffffffe8,%esp        ; 24 bytes for FIVE arguments
//                  mov %ebx,0xc(%esp)          ; argument 4
//                  mov %edx,0x10(%esp)         ; argument 5
//                  call 0x10022e9c             ; 1,2,3 in eax/edx/ecx
//      0x10022e9c  mov 0x24(%esp),%edi         ; = entry+0x14, argument 5
//                  mov 0x54(%esp),%ebx         ; = entry+0x10, argument 4
//
//  and the reservation size is the compiler's own alignment padding of
//  4*nargs up to the next multiple that leaves %esp congruent at the call:
//  8 bytes for one argument, 16 for three, 24 for five.  That is how the
//  arity of every stand-in below was counted.
//
//  LINKAGE.  A shipped callee with a stack shim in front of its register
//  entry (`mov 0x4(%esp),%eax ...`) had EXTERNAL linkage; one with no shim
//  had internal linkage.  0x10039210 is the proof that it does not change
//  the CALLER: it has no shim (0x1003920f is a `nop` and 0x1003920e is the
//  previous function's `ret`), so it is static, and its caller still writes
//  argument 4 at 0xc(%esp) with the three register holes left empty.  So
//  every stand-in here is `static`, which is also the one form d3dqip.cpp
//  proved end to end.
//
//  FLAGS.  Four of the five reconstructions are /O2 -- duplicated
//  epilogues and `add $-N,%esp` + `mov` for outgoing arguments.
//  C10023130::sub_10023130 is /Os and is the odd one out; its banner says
//  so and it wants a separate /Os + /Qip translation unit
//  It is left here so the address is not lost.
//
//  Naming: `sub_<address>` so the address can be read out of
//  the symbol.  `Obj<hex>` / `Rec<hex>` types are invented here.
//
// *********************************************************************** //

#include "ltbasedefs.h"

#include <math.h>
#include <string.h>


// ======================================================================= //
//  THE PLACEHOLDER MACHINERY
// ======================================================================= //

// The placeholder machinery (g_qip_nSink, QIP_MIX, QIP_BODY) is gone:
// every qip_Callee stand-in in this file now calls the real function (see
// each declaration), so nothing here runs a wrong body any more.


// ======================================================================= //
//  SHARED DECLARATIONS
// ======================================================================= //

// ---------------------------------------------------------------------- //
//  The Direct3D 7 device at 0x1008d988, repeated from
//  src/engine/d3d/d3dr2d.cpp (separate translation units, so it is
//  repeated rather than shared).  Slot numbering, all already pinned in
//  this repo: SetRenderState is index 20 (+0x50), GetRenderState is index
//  21 (+0x54), SetTexture is index 35 (+0x8c).  The
//  `mov (%eax),%edx ; call *0xNN(%edx)` with the object written as the
//  first stack argument is the function-pointer-struct model the rest of
//  src/engine/d3d uses for COM.
// ---------------------------------------------------------------------- //
struct D3DDevice7;

struct D3DDevice7Vtbl
{
	uint8 m_pad_00[0x50];
	long (__stdcall* SetRenderState)(D3DDevice7*, uint32 dwState,
	                                 uint32 dwValue);                 // +0x50
	long (__stdcall* GetRenderState)(D3DDevice7*, uint32 dwState,
	                                 uint32* lpdwValue);              // +0x54
	uint8 m_pad_58[0x34];
	long (__stdcall* SetTexture)(D3DDevice7*, uint32 dwStage,
	                             void* lpTexture);                    // +0x8c
};

struct D3DDevice7
{
	D3DDevice7Vtbl* lpVtbl;
};

extern D3DDevice7* g_pD3DDevice;                        // 0x1008d988

extern uint32 g_1008d7d0;                               // current stage
extern void*  g_a1008e684[];                            // bound textures


// ======================================================================= //
//  0x10026480   128 bytes (0x10026480..0x100264ff)      2 callers
//
//  DECODED, every instruction.  A mode switch: it retargets four function
//  pointers through its callee, sets three flags from the mode, and then
//  runs one of two fixups.
//
//      10026480  mov 0x4(%esp),%eax             <- the STACK entry
//      10026484  push %ebx / sub $0x8,%esp      <- the REGISTER entry
//                mov %eax,%ebx / mov %ebx,%eax
//                call 0x100256d0                ; one argument, in %eax
//                cmp $0x1,%ebx / je 0x100264ef
//                movl $0x0,0x1008ce7c / movl $0x1,0x1008ce80
//      100264aa  mov 0x1008d970,%eax / test / je 0x100264c4
//                cmp $0x1,%ebx / je 0x100264eb
//                mov $0x1,%eax
//      100264bd  mov %eax,0x1008ceac / jmp 0x100264ce
//      100264c4  movl $0x0,0x1008ceac
//      100264ce  mov 0x1008d96c,%eax / test / je 0x100264e1
//                call 0x10073210 / add $0x8,%esp / pop %ebx / ret
//      100264e1  call 0x100145f0 / add $0x8,%esp / pop %ebx / ret
//      100264eb  xor %eax,%eax / jmp 0x100264bd
//      100264ef  mov $0x1,%eax / mov %eax,0x1008ce7c
//                mov %eax,0x1008ce80 / jmp 0x100264aa
//
//  WHAT PINS IT:
//    * the two-entry pair: 0x10026480 is four bytes of `mov 0x4(%esp),%eax`
//      in front of 0x10026484, and 0x10029890 below calls 0x10026484
//      DIRECTLY.  So this is an external-linkage /Qip function of ONE
//      argument, and 0x10029890 is in its translation unit.
//    * 0x100256d0 has no shim of its own (0x100256cc is zero padding), so
//      it is static; it reads its argument at 0x1002579d (`cmp $0x1,%eax`)
//      and returns nothing.  The 8 bytes `sub $0x8,%esp` reserves in the
//      prologue ARE that call's outgoing area -- 4 bytes for one argument,
//      padded to 8 -- which is why the epilogue frees 8 after two calls
//      that push nothing.
//    * 0x100264bd is ONE store reached from both arms through %eax, so the
//      inner if/else is a value-producing expression -- a `?:`, not two
//      separate `movl $K,0x1008ceac`.  The outer arm at 0x100264c4 IS a
//      separate `movl $0x0`, so that one is a statement.
//    * `mov $0x1,%eax` (b8 01 00 00 00) not `xor;inc`, `add $-N,%esp` for
//      outgoing arguments and the DUPLICATED epilogue at 0x100264dc and
//      0x100264e6: /O2.
//    * 0x100145f0 is a bare `c3`, an empty function; 0x10073210 is a real
//      body.  Neither takes an argument.
//
//  COIN-FLIP: the `nMode == 1` arm is placed at the END of the function
//  and jumped back from, which is icl sinking a cold block (the same
//  transform d3dr3c.cpp records on 0x1000eef0's `if (!pBucket)`).  If the
//  sink does not reproduce, the alternative spelling is the negation --
//  `if (nMode != 1) { g_1008ce7c = 0; g_1008ce80 = 1; } else { ... 1, 1 }`.
// ======================================================================= //

extern uint32 g_1008ce7c;                               // 0x1008ce7c
extern uint32 g_1008ce80;                               // 0x1008ce80
extern uint32 g_1008ceac;                               // 0x1008ceac
extern uint32 g_1008d970;                               // 0x1008d970
extern uint32 g_1008d96c;                               // 0x1008d96c

extern void sub_10073210();                             // 0x10073210
extern void sub_100145f0();                             // 0x100145f0

// 0x100256d0 -- static, one argument in %eax, returns nothing.



// ======================================================================= //
//  0x1002cf40   268 bytes (0x1002cf40..0x1002d04b)      1 caller
//
//  DECODED, every instruction.  Save one render state, force it to 1, drop
//  the bound texture, do the work, put the state back.
//
//      1002cf40  mov 0x4(%esp),%eax / mov 0x8(%esp),%edx   <- STACK entry
//      1002cf48  push %ebp / mov %esp,%ebp / sub $0x28,%esp <- REGISTER
//                mov %edi,0x8(%esp) / mov %esi,0x4(%esp) / mov %ebx,(%esp)
//                mov $0x7,%ecx / add $0xfffffff4,%esp
//                lea -0x4(%ebp),%ebx / mov 0x1008d988,%esi
//                mov %ecx,-0x8(%ebp)            ; the state id, in a LOCAL
//                mov %ebx,0x8(%esp)             ; &nSaved
//                mov %ecx,0x4(%esp)
//                mov %eax,%edi / mov %edx,%esi
//                call *0x54(%ecx)               ; GetRenderState
//                ... mov -0x8(%ebp),%eax / movl $0x1,0x8(%esp)
//                call *0x50(%eax)               ; SetRenderState(state, 1)
//      1002cfa1  mov 0x1008d7d0,%ebx
//                mov 0x1008e684(,%ebx,4),%eax / test / je 0x1002cfdc
//                ... call *0x8c(%edx)           ; SetTexture(stage, NULL)
//                movl $0x0,0x1008e684(,%ebx,4)
//      1002cfdc  test %esi,%esi / jb 0x1002d01e
//      1002cfe0  lea 0x24(%edi),%eax / lea 0x30(%edi),%edx
//                add $0xfffffff0,%esp / mov $0xffffffff,%ecx
//                call 0x1002dd6c / add $0x10,%esp
//                mov 0x58(%edi),%eax / test / je 0x1002d01e
//                mov %edi,%ebx / add $0x10,%edi
//      1002d002  mov 0x58(%ebx),%eax / add $0xfffffff0,%esp
//                mov $0x1,%edx / mov %esi,%ecx
//                call 0x1002d05c
//                add $0x4,%ebx / add $0x10,%esp
//                cmp %ebx,%edi / ja 0x1002d002
//      1002d01e  ... call *0x50(%edx)           ; SetRenderState(state, nSaved)
//
//  WHAT PINS IT:
//    * the two-entry pair at +0/+8 says TWO arguments, %eax then %edx.
//    * 0x1002dd6c has a three-mov shim at 0x1002dd60 and 0x1002d05c has one
//      at 0x1002d050, so both take THREE arguments; the caller reserves 16
//      bytes for each and writes none of them, which is the register holes.
//    * 0x1002cfa1..0x1002cfdc is byte-for-byte the body of 0x100257d0,
//      which d3dr2d.cpp already reconstructed as
//      `nStage = g_1008d7d0; if (g_a1008e684[nStage]) { SetTexture(...); }`.
//      The stage is held in %ebx ACROSS the COM call and never reloaded,
//      which a global cannot be, so it is a local here too.
//    * the state id is materialised ONCE into %ecx and ALSO stored to
//      -0x8(%ebp), then reloaded from there twice more.  A literal would
//      have been re-materialised as `movl $0x7,0x4(%esp)`; the stack home
//      plus two reloads is a source VARIABLE that icl declined to
//      enregister across the COM calls.
//    * the loop walks %ebx from %edi to %edi+0x10 in steps of 4 reading
//      0x58(%ebx), so +0x58 is an array of FOUR pointers, and
//      `cmp %ebx,%edi ; ja` is an UNSIGNED compare -- an unsigned index.
//      The `mov 0x58(%edi),%eax ; test ; je` in front of it tests element
//      ZERO and is a separate source `if`, not the loop guard.
//    * `add $-N,%esp` + `mov` for every outgoing argument and no
//      cross-jumped exit: /O2.
//
//  THE ALWAYS-TRUE GUARD at 0x1002cfdc is in the shipped bytes and is not
//  a decoding error: `test %esi,%esi` clears CF, so `jb` is never taken and
//  the block always runs.  The inverse of `jb` is `jae`, so the source
//  condition is an UNSIGNED `>= 0`.  icl neither folded it nor warned it
//  away.  COIN-FLIP: written below as `if (nValue >= 0)` on a uint32; the
//  other spelling that gives the same `test`/`jb` is `if (0 <= nValue)`.
// ======================================================================= //

struct Obj1002cf40
{
	uint8   m_pad_00[0x24];                             // +0x00
	uint8   m_a24[0xc];                                 // +0x24
	uint8   m_a30[0x28];                                // +0x30
	void*   m_ap58[4];                                  // +0x58
};

// 0x1002dd6c -- three arguments, %eax/%edx/%ecx, returns nothing.

// 0x1002d05c -- three arguments, %eax/%edx/%ecx, returns nothing.



// ======================================================================= //
//  0x10029890   280 bytes (0x10029890..0x1002999a)      2 callers
//
//  DECODED, every instruction.  A frame teardown: one render state, three
//  fixed calls, then either a per-object sweep or a pair of mode calls.
//
//      10029890  mov 0x4(%esp),%eax                       <- STACK entry
//      10029894  push %ebp / mov %esp,%ebp / sub $0x20,%esp <- REGISTER
//                mov %eax,%edi
//                mov 0x1008d988,%eax                      ; loaded FIRST
//                mov 0x1008d7a0,%edx / test / je 0x100298bd
//                mov $0x1,%edx / jmp 0x100298bf
//      100298bd  xor %edx,%edx
//      100298bf  add $0xfffffff4,%esp
//                movl $0x88,0x4(%esp) / mov %edx,0x8(%esp)
//                call *0x50(%edx)                         ; SetRenderState
//                call 0x10026670 / call 0x10050390 / call 0x1001eb20
//                cmpl $0x2,(%edi) / je 0x1002993a
//                add $0xfffffff8,%esp / xor %eax,%eax
//                call 0x10026484 / add $0x8,%esp
//                call 0x1007b880
//                add $0xfffffff8,%esp / xor %eax,%eax
//                call 0x10026504 / add $0x8,%esp
//      10029909  call 0x1004fbc0 / call 0x1001eb20
//                lea 0x50(%edi),%eax / add $0xfffffff8,%esp
//                call 0x10025db0
//                add $0x5c,%edi / mov %edi,%eax
//                call 0x10025890 / add $0x8,%esp / <epilogue>
//      1002993a  call 0x1007b8d0 / mov %eax,%ecx / call 0x1007bae0
//                mov 0xe0(%edi),%eax / mov 0xdc(%edi),%edx
//                test %eax,%eax / jle 0x10029909
//                mov %edx,%ebx / lea (%edx,%eax,4),%esi
//      10029960  mov (%ebx),%eax / test / je 0x10029993
//                movsbl 0xbc(%eax),%edx
//                test %edx,%edx / jl 0x10029993
//                cmp $0xb,%edx / jge 0x10029993
//                lea (%edx,%edx,4),%edx / mov %edx,%ecx / add %ecx,%ecx
//                mov 0x100895c0(%ecx,%edx,2),%edx
//                test %edx,%edx / je 0x10029993
//                add $0xfffffff8,%esp / mov %eax,(%esp) / call *%edx
//      10029993  add $0x4,%ebx / cmp %esi,%ebx / jl 0x10029960
//                jmp 0x10029909
//
//  WHAT PINS IT:
//    * the shim at +0 is one `mov 0x4(%esp),%eax`: ONE argument.
//    * 0x10026484 and 0x10026504 are the REGISTER entries of 0x10026480
//      (above) and 0x10026500, both entered four bytes past their shims, so
//      this function shares their translation unit.  Both get 8 bytes of
//      outgoing area and only %eax: one argument each.  0x10025db0 and
//      0x10025890 have NO shim (0x10025dac and 0x1002588c are zero), so
//      they are static one-argument functions, and 0x10025890's argument is
//      formed by `add $0x5c,%edi` -- %edi is dead afterwards, so icl
//      clobbered it rather than lea.
//    * the addressing `0x100895c0(%ecx,%edx,2)` with %edx = n*5 and
//      %ecx = n*10 is base + n*20, i.e. element n of the ELEVEN twenty-byte
//      records at 0x100895bc that d3dr2d.cpp's sub_10050390 already pinned,
//      and the +4 in the base picks the SECOND field of the record.  The
//      bound `cmp $0xb` is that array's 11.
//    * `movsbl 0xbc(%eax)` -- the discriminator is a SIGNED char, and it is
//      range-checked `>= 0` (`jl`) then `< 11` (`jge`).
//    * the sweep's bound is `test %eax,%eax ; jle` and the back edge is
//      `cmp %esi,%ebx ; jl`: SIGNED, so the count at +0xe0 is int32 and the
//      index is int32.  The indirect call reserves 8 and cleans 8, so the
//      slot is a __cdecl `void (*)(T*)`.
//    * `add $-N,%esp` + `mov` throughout and no cross-jumped exit: /O2.
//
//  COIN-FLIP: `cmpl $0x2,(%edi) ; je` sends the LONG arm away and lets the
//  short arm fall through, which is the same cold-block sink d3dr3c.cpp
//  records.  If it does not reproduce, the alternative is the negation --
//  `if (pObj->m_n00 != 2) { the two mode calls } else { the sweep }`.
// ======================================================================= //

struct Elem10029890
{
	uint8   m_pad_00[0xbc];                             // +0x00
	char    m_cbc;                                      // +0xbc
};

struct Rec100895bc
{
	void  (*m_pfnSlot00)();                             // +0x00
	void  (*m_pfnSlot04)(Elem10029890*);                // +0x04
	void*   m_p08;                                      // +0x08
	void*   m_p0c;                                      // +0x0c
	void*   m_p10;                                      // +0x10
};

extern Rec100895bc g_a100895bc[11];                     // 0x100895bc

struct Obj10029890
{
	uint32          m_n00;                              // +0x00
	uint8           m_pad_04[0x4c];                     // +0x04
	uint8           m_a50[0xc];                         // +0x50
	uint8           m_a5c[0x80];                        // +0x5c
	Elem10029890**  m_pdc;                              // +0xdc
	int32           m_ne0;                              // +0xe0
};

struct Mgr1007b8d0
{
	void sub_1007bae0();                                // 0x1007bae0
};

extern Mgr1007b8d0* sub_1007b8d0();                     // 0x1007b8d0

extern uint32 g_1008d7a0;                               // 0x1008d7a0

extern void sub_10026670();                             // 0x10026670
extern void sub_10050390();                             // 0x10050390
extern void sub_1001eb20();                             // 0x1001eb20
extern void sub_1007b880();                             // 0x1007b880
extern void sub_1004fbc0();                             // 0x1004fbc0

// 0x10026484 -- the register entry of 0x10026480; one argument, void.

// 0x10026504 -- the register entry of 0x10026500; one argument, void.

// 0x10025db0 -- static, one argument in %eax, void.

// 0x10025890 -- static, one argument in %eax, void.



// ======================================================================= //
//  0x10039420   462 bytes (0x10039420..0x100395ed)      1 caller
//
//  DECODED, every instruction.  Rebuild a 64x64 gamma ramp surface when the
//  gamma actually changed.
//
//      10039420  push %ebp / mov %esp,%ebp
//                mov $0x4060,%eax / call 0x100805a0        ; _chkstk
//                mov %edi,0x8(%esp) / mov %esi,0x4(%esp) / mov %ebx,(%esp)
//                flds 0x1008da6c / fcomps 0x8(%ebp)
//                fnstsw %ax / mov 0x8(%ebp),%edx / sahf
//                jne 0x10039458 / <epilogue>               ; equal -> return
//      10039458  mov 0x1008da74,%eax
//                mov %edx,0x1008da6c                       ; INTEGER store
//                test %eax,%eax / je 0x1003947c
//                mov 0x1008da74,%eax                       ; RELOADED
//                mov (%eax),%edx / push %eax / call *0x8(%edx)
//                movl $0x0,0x1008da74
//      1003947c  lea -0x4040(%ebp),%edi / xor %eax,%eax
//                mov $0x1000,%ecx / rep stos %eax,%es:(%edi)
//                flds 0x8(%ebp) / fstpl -0x10(%ebp)
//                xor %ebx,%ebx / xor %edi,%edi / xor %esi,%esi
//                mov $-4,-0x1c(%ebp) / mov $-0x100,-0x18(%ebp)
//      100394b0  mov %ebx,-0x40(%ebp) / movl $0,-0x3c(%ebp)
//                fildll -0x40(%ebp)                        ; (double)(unsigned)i
//                fmuls 0x100a9410                          ; * 0.015625f
//                fldl -0x10(%ebp) / fxch %st(1)
//                fstpl (%esp) / fstpl 0x8(%esp) / call 0x1007fe54   ; pow
//                fmuls 0x100a9414                          ; * 255.0f
//                fstpl -0x28(%ebp)  ... inline double->int ...
//      10039532  lea (%esi,%edi,1),%edx
//                mov %eax,%ecx / shl $0x8,%ecx / or %eax,%ecx
//                mov %eax,%edi / shl $0x10,%edi / or %edi,%ecx
//                test %ebx,%ebx / mov %ecx,-0x4040(%ebp,%edx,1)
//                jbe 0x1003958f
//                ... the SAME or-chain twice more, at -0x18+%edi and
//                    -0x1c+%esi ...
//      1003958f  -0x1c += 4 / -0x18 += 0x100 / inc %ebx
//                %esi += 0x100 / %edi += 4
//                cmp $0x40,%ebx / jb 0x100394b0
//                add $0xfffffff0,%esp / lea -0x4040(%ebp),%eax
//                mov $0x40,%edx / mov $0x40,%ecx
//                movl $0x100,0xc(%esp) / call 0x10039210
//                mov %eax,0x1008da74
//
//  WHAT PINS IT:
//    * `mov $0x4060,%eax ; call 0x100805a0` is _chkstk, so the frame is
//      0x4060 and the array at -0x4040 is 0x4000 bytes = 64*64 dwords.  The
//      `rep stos` of 0x1000 dwords zeroes exactly that array.
//    * the constants ARE the ramp: 0x100a9410 is 0x3c800000 = 0.015625f =
//      1/64 and 0x100a9414 is 0x437f0000 = 255.0f, and 0x1007fe54 opens
//      `mov 0x8(%esp),%ecx ; and $0x7ff00000,%ecx` on the SECOND double --
//      the exponent of the exponent -- so it is pow(x, y), not a one-
//      argument libm entry.  Both multiplies are `fmuls`, i.e. FLOAT
//      literals promoted in the x87, so the source writes them as floats.
//    * `fildll` of {i, 0} is icl's UNSIGNED int-to-double, so the loop
//      counter is unsigned; the back edge `cmp $0x40,%ebx ; jb` agrees.
//    * the three index expressions are %esi+%edi = i*0x100 + i*4,
//      (-0x100 + i*0x100) + i*4 and (-4 + i*4) + i*0x100 -- i.e. [i][i],
//      [i-1][i] and [i][i-1] of a uint32[64][64].
//    * the guard on the second pair is `test %ebx,%ebx ; jbe`, an UNSIGNED
//      `<=`, so the source wrote `if (i > 0)` and not `if (i != 0)`.
//    * the or-chain is emitted THREE times, once per store: icl did not CSE
//      it, so the expression really is written out three times.
//    * `mov 0x1008da74,%eax` appears TWICE around the release, so the
//      global is read twice -- there is no local copy.
//    * `mov %edx,0x1008da6c` stores the float with an INTEGER mov, which is
//      what this compiler does for float lvalue copies.
//    * `fcomps 0x1008da6c`-first means the global is the source's LEFT
//      operand.
//    * 0x10039210 has no shim (0x1003920f is a `nop`), so it is static; the
//      caller reserves 16 and writes argument 4 at 0xc(%esp), leaving the
//      three register holes -- FOUR arguments, returning a pointer.
//    * DUPLICATED epilogue (0x10039449 and 0x100395df) and `add $-N,%esp` +
//      `mov`: /O2.
//
//  RISK, stated plainly: the double->int32 at 0x100394e1..0x1003951f is
//  icl's INLINE truncating conversion (exponent extract, `shld $0xb`,
//  variable `shr`, `neg` for the sign), not a call to _ftol.  It is emitted
//  from a plain `(uint32)` cast; if this build's icl calls _ftol instead,
//  the whole tail shifts and the body below is a near, not a match.
//  COIN-FLIP on the frame: `nValue` is homed at -0x40(%ebp), the same slot
//  the fildll temporary uses on the next iteration; if the slot colouring
//  does not reproduce, drop the variable and write the or-chain against the
//  cast expression directly.
//  COIN-FLIP on pow: the shipped call passes TWO doubles (16 bytes of
//  outgoing area, `fstpl (%esp)` and `fstpl 0x8(%esp)`), so it is
//  pow(double,double) and not C++ math.h's `float pow(float,float)`
//  overload, which is what unadorned `pow(i * (1.0f/64.0f), fGamma)` would
//  select.  The explicit `(double)` casts below force the right overload
//  and should cost nothing, because a float-to-double widening of a value
//  already in %st(0) is a no-op with no /Op.  If they DO cost an `fstps`/
//  `flds` round trip, the alternative is to drop <math.h> and declare
//  `extern "C" double pow(double, double);` in this file, then write the
//  call with no casts at all.
// ======================================================================= //

struct IUnk10039420;

struct IUnk10039420Vtbl
{
	uint8 m_pad_00[0x8];
	long (__stdcall* Release)(IUnk10039420*);           // +0x08
};

struct IUnk10039420
{
	IUnk10039420Vtbl* lpVtbl;
};

extern float          g_1008da6c;                       // 0x1008da6c
extern IUnk10039420*  g_1008da74;                       // 0x1008da74

// 0x10039210 -- static, four arguments (three in %eax/%edx/%ecx, the fourth
// at 0xc(%esp)), returns the interface pointer.



// ======================================================================= //
//  0x10023130   198 bytes (0x10023130..0x100231f5)      2 callers
//
//  *** THIS ONE IS /Os, NOT /O2.  IT DOES NOT BELONG IN THIS FILE. ***
//  It is here so the address is not lost; it belongs
//  in an /Os + /Qip translation unit of its own.  Three independent
//  markers, all three of them /Os tells:
//      * the second exit CROSS-JUMPS -- 0x100231f2 is `xor %eax,%eax ;
//        eb ac`, jumping back into the one epilogue, instead of duplicating
//        it the way 0x10026480 and 0x10039420 above both do;
//      * outgoing arguments are PUSHED -- `push %eax ; push %edx` in front
//        of both string compares;
//      * `&this->m_n100` is `lea 0x100(%ebx),%edx`, not `mov $K ; add`.
//  `xor %edi,%edi ; inc %edi` and `xor %eax,%eax ; inc %eax` for the
//  constant 1 are the fourth.  (The one `add $0xffffffe8,%esp` + `mov` is
//  NOT an /O2 marker: the register ABI forces it, because arguments 4 and 5
//  have to land at 0xc(%esp) and 0x10(%esp) over the register holes and
//  pushes cannot skip slots.)
//
//  DECODED, every instruction.  __thiscall, three arguments, `ret $0xc`.
//  A linear search of a token stream for a name.
//
//      10023130  push %ebp / mov %esp,%ebp / sub $0x20,%esp
//                push %edi / push %edi / push %esi / push %ebx
//                mov 0x10(%ebp),%edi / mov %ecx,%ebx
//                test %edi,%edi / jne 0x10023146
//                xor %edi,%edi / inc %edi
//      10023146  mov %ebx,%ecx / call 0x100230d4
//                test %eax,%eax / mov 0xc(%ebp),%esi
//                je 0x100231f2
//                xor %eax,%eax / mov %eax,-0x4(%ebp)
//      1002315d  cmp 0x100(%ebx),%edi / jg 0x100231ab
//                test %esi,%esi / je 0x10023183
//                mov 0x8(%ebp),%eax / mov (%ebx),%edx
//                push %eax / push %edx / call 0x10080910
//                xor %edx,%edx / add $0x8,%esp
//                test %eax,%eax / sete %dl / mov %edx,%eax / jmp 0x1002319b
//      10023183  ... the same, calling 0x100874b0 ...
//      1002319b  test %eax,%eax / je 0x100231ab
//                xor %eax,%eax / inc %eax
//      100231a2  pop %ebx / pop %esi / pop %edi
//                mov %ebp,%esp / pop %ebp / ret $0xc
//      100231ab  mov 0x104(%ebx),%eax / test %eax,%eax / je 0x100231f2
//                add $0xffffffe8,%esp
//                lea 0x100(%ebx),%edx / lea 0x104(%ebx),%ecx
//                mov %ebx,0xc(%esp) / mov %edx,0x10(%esp)
//                mov %ecx,%edx / lea 0x108(%ebx),%ecx
//                call 0x10022e9c / add $0x18,%esp
//                test %eax,%eax / jne 0x1002315d
//                mov -0x4(%ebp),%eax / mov %eax,0x104(%ebx)
//                jmp 0x1002315d
//      100231f2  xor %eax,%eax / jmp 0x100231a2
//
//  WHAT PINS IT:
//    * `ret $0xc` with %ecx live-in at 0x1002313d: __thiscall, three
//      arguments.  The extra `push %edi` at 0x10023136 is the dead 4-byte
//      stack slot this compiler emits.
//    * 0x10080910 is the CRT strcmp (`test $0x3,%edx` alignment probe, then
//      `mov (%edx),%eax ; cmp (%ecx),%al ; or %al,%al`) and 0x100874b0 is
//      _stricmp (it opens on the locale table at 0x10091ed4, the same one
//      d3dr2d.cpp's toupper sits behind).  Argument 2 non-zero picks the
//      case-SENSITIVE one, so it is the case-sensitivity flag.
//    * both arms end `sete %dl ; mov %edx,%eax` and then the join RETESTS
//      with `test %eax,%eax ; je`.  The value is materialised and tested
//      again, which is an if/else through a single result VARIABLE, not a
//      `?:` folded into the branch.
//    * `cmp 0x100(%ebx),%edi` puts the argument on the left, so the source
//      is `nIndex <= m_n100`; `jg` is SIGNED, so both are int32.
//    * 0x10022e9c has a THREE-mov shim at 0x10022e90, so it is external and
//      takes at least three arguments; it reads 0x24(%esp) before its own
//      `sub` (= entry+0x14, argument 5) and 0x54(%esp) after (= entry+0x10,
//      argument 4), so it takes FIVE, and the caller's 24-byte reservation
//      with writes only at 0xc and 0x10 matches exactly.  Its first
//      argument is dereferenced as `cmpb $0x20,0x0(%ebp)`, so +0x104 is a
//      char* and the function is a tokeniser.
//    * 0x100230d4 is `push %ebx ; sub $0x20,%esp ; mov %ecx,%ebx` and then
//      reads the SAME +0x100/+0x104/+0x108 triple: a sibling member of this
//      class, __thiscall, no arguments.
//
//  COIN-FLIP: -0x4(%ebp) is written once with zero and read once, into
//  +0x104.  A direct `m_p104 = LTNULL;` would have been `movl $0x0` at /Os;
//  the stack home says a source LOCAL, spelled below as `pNull`.  If that
//  does not reproduce, the alternative is `m_p104 = LTNULL;` with no local
//  and the declaration deleted.
// ======================================================================= //

// 0x10023130 (and the tokeniser 0x10022e90/9c and C10023130's advance
// 0x100230d0/d4 it calls) moved to w15_10022e90_osoiqip.cpp: the shipped
// body enters both through their /Qip register entries, which needs all
// three in one /Qip translation unit.  There it is byte-identical, as the
// ConParse::ParseFind of ltbasedefs.h (uint32 minTokens compared as int).


// ======================================================================= //
//  FAMILY F of d3dr5a.cpp -- the node list's recursive Find and its one
//  inlining caller, moved here from d3dr5a.cpp because they need /Os /Qip.
//
//  0x100362e0   4 + 47 bytes.  /Qip's EXTERNAL-linkage two-entry pair:
//
//      100362e0  8b 44 24 04            the stack entry: key -> %eax
//      100362e4  55 / 8b ec / 83 ec 10  the register entry (the listing's
//                8b 01 / 8b d0          row), %edx = m_pKey
//                8b 45 08 / 3b c2       cmp against the key
//                74 16 / 8b 49 08 / 85 c9 / 74 13
//                57 / 89 04 24 / e8 <0x100362e4>   the recursion, into
//                8b e5 / 5d / c2 04 00             the register entry
//                8b c1 / eb f6 / 33 c0 / eb f2
//
//  WHAT PINS IT: the four-byte `mov 0x4(%esp),%eax` in front of the body
//  and the recursive call landing at +4 are the external /Qip
//  shape, and nothing else emits it.  In d3dr5a.cpp (/Os, no /Qip) the
//  same source, written in-class, came out as an ordinary one-entry
//  function with one level of its own recursion inlined (65 bytes against
//  47).  Compiling ALL of d3dr5a.cpp /Os /Qip measured worse (14 -> 13
//  there: 0x10036420 broke and _DeleteAndDestroyArray went out of line),
//  so only this pair moved.  Built here, the stack entry, the register
//  entry and 0x10036420 are all byte-identical.  Named for the stack
//  entry, like big7cc90's pair: the stack entry `X$` sits above `X`.
//
//  d3dr2b.cpp carries a second reconstruction of the same address,
//  Node10036254::sub_100362e4, one byte off; this is the matching one.
// ======================================================================= //
// The payload the node owns (d3dr2b.cpp's Elem10035bb4, a 0x10-byte
// object whose vtable 0x10089480 has its scalar deleting destructor in
// slot 0); only that virtual destructor is used here, by 0x100362a4.
//
// The payload DERIVES from a base with a virtual destructor
// (Elem10035bb4Base below).  Measured: with the destructor declared only
// on Elem10035bb4V, `delete m_pKey` in 0x100362a4 stores the flag before
// loading the vtable (8 bytes off); with the virtual destructor inherited
// from a base, icl loads the vtable first, as shipped -- byte-identical.
struct Elem10035bb4Base
{
	virtual ~Elem10035bb4Base();
};

struct Elem10035bb4V : public Elem10035bb4Base
{
	virtual ~Elem10035bb4V();
};

struct Node10036420
{
	Elem10035bb4V*	m_pKey;                                 // +0x00
	Node10036420*	m_pPrev04;                              // +0x04
	Node10036420*	m_pNext;                                // +0x08

	Node10036420*	sub_100362e0(void* pKey);           // 0x100362e0
	void	sub_100362a4();                             // 0x100362a4
	void	sub_10035ab0(uint32 nFlags);                // 0x10035ab0
};

Node10036420* Node10036420::sub_100362e0(void* pKey)
{
	if (pKey == m_pKey)
		return this;

	if (!m_pNext)
		return LTNULL;

	return m_pNext->sub_100362e0(pKey);
}


// ----------------------------------------------------------------------- //
//  0x10036420   62 bytes + 2 of padding.  (From d3dr5a.cpp; see above.)
//
//      8b 41 04                     %eax = m_pHead
//      8b 55 08                     %edx = the key
//      3b 10                        cmp %edx,(%eax)
//      74 2a                        -> mov %eax,%ecx  (the hit is the head)
//      8b 48 08 / 85 c9 / 74 1f     -> xor %ecx,%ecx
//      52 / e8 <0x100362e4>         the recursive Find on m_pNext
//      8b c8
//      85 c9 / 74 ..                the delete's own null test
//      57 / c7 04 24 01000000 / e8 <0x10035ab0>
//
//  WHAT PINS IT: one inlined level of Find with the recursion left as a
//  call, and the `3b 10` destination being %edx -- so the KEY is the
//  source's left operand, `if (pKey == m_pKey)`, matching the out-of-line
//  copy's `3b c2` (destination %eax, also the key).  The two arms both
//  jump forward into one `test %ecx,%ecx`, which is the
//  "if/else through a single result variable" shape.
// ----------------------------------------------------------------------- //
struct C10036420
{
	void**		m_pVTable;                              // +0x00
	Node10036420*	m_pHead;                                // +0x04

	void	sub_10036420(void* pKey);
};

void C10036420::sub_10036420(void* pKey)
{
	Node10036420* pNode = m_pHead->sub_100362e0(pKey);

	if (pNode)
		pNode->sub_10035ab0(1);
}




// ======================================================================= //
//  THE NODE'S DESTRUCTOR BODY, added for the d3d-link: 0x100362a4 (60).
//  In the shipped TU it is /Qip's EXTERNAL __thiscall pair, as
//  0x100362e0/0x100362e4 above: a vestigial `mov 0x4(%esp),%eax` stack
//  entry at 0x100362a0, then this register entry, which 0x10035ab0 calls.
//  Nothing in OUR TU calls it, so icl emits no stack entry and the symbol
//  is the body itself -- named for its own address.  (Called from here it
//  does get the entry: measured in a probe, `mov 0x4(%esp),%eax`
//  byte-identical.)  Needs /Os: at /O2 the body does not reproduce.
//
//      8b 0b / 85 c9 / 74 0e        delete m_pKey -- ONE null test, then
//      57 / 8b 01 / c7 04 24 01..   slot 0 of the payload's vtable (its
//      8b 00 / ff d0                scalar deleting destructor), flag 1
//      8b 43 04 / 85 c0 / 74 06     if (m_pPrev04) m_pPrev04->m_pNext = ..
//      8b 43 08 / 85 c0 / 74 06     if (m_pNext) m_pNext->m_pPrev04 = ..
//
//  CLOSED: byte-identical with the payload's virtual
//  destructor inherited from a base class (see Elem10035bb4V above).
//  The earlier record follows.
//  NEAR, 8 bytes: the shipped delete loads the payload's vtable BEFORE
//  storing the flag (`57 / 8b 01 / c7 04 24 01000000`), ours stores the
//  flag first.  `delete m_pKey` and an explicit `if (p) p->slot0(1)`
//  compile identically here.  Also measured, all the same order: a
//  non-destructor virtual `Destroy(uint32)` called with 1, a local copy of
//  m_pKey (which also brings back a second null test), and the caller
//  0x10035ab0 built into this TU (the stack entry then appears, the body is
//  unchanged).  /G5 and /G6 do not move the same swap at 0x10015910.
//
//  The scalar deleting destructor that calls it, 0x10035ab0, is NOT here:
//  /Qip inlines it into 0x10036420 above (measured, 0x10036420 then stops
//  matching), where the shipped 0x10036420 calls it.  It is
//  w6_10035ab0_os.cpp's.
// ======================================================================= //
void Node10036420::sub_100362a4()
{
	delete m_pKey;

	if (m_pPrev04)
		m_pPrev04->m_pNext = m_pNext;

	if (m_pNext)
		m_pNext->m_pPrev04 = m_pPrev04;
}
