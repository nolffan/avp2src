// Split out of d3dmid.cpp: these functions need the
// OTHER optimisation flag from the rest of that file.  d3d.ren is a
// mixed build and our file boundaries are invented.
// Everything that is not a function body is
// carried over verbatim so no declaration is lost.
// ********************************************************* //
// *********************************************************************** //
//
//  MODULE:   MID (.CPP)   -- scratch, d3d.ren batch "mid"
//
//  Seven rows between 0x10021c20 and 0x1003b690.  SIX functions; the
//  seventh row is not a function at all (see the note on 0x100399bf at the
//  bottom of this file).
//
//  READ THIS BEFORE MEASURING.
//
//  The listing was produced by a tier-A filter, which
//  rejects a body containing any of `55 8b ec`, `33 c0 40`, `83 c4 04`,
//  `89 04 24`, `c7 04 24`, the gas lea-nops, `83 c4 f8/f4/ec`, x87 opcodes,
//  or a first instruction that reads an unloaded register.  FIVE of these
//  six slipped through that filter but are NOT actually free of the unknown
//  compiler, because the filter's patterns are narrower than the idioms:
//
//    * `89 04 24` catches `mov %eax,(%esp)` but NOT `mov %edx,(%esp)`
//      (`89 14 24`), which is how 0x10021c20 and 0x10021ce0 pass their one
//      argument.  Same accumulate-outgoing-args idiom, different register.
//      cl has no such mode: it pushes call arguments, always.
//    * `83 c4 04` catches a 4-byte dummy slot but not an 8- or 12-byte one
//      (`83 c4 08`, `83 c4 0c`), which is what 0x1003b690 opens and closes
//      without ever writing to it.
//    * The "argument in a register" test looks only at the CALLEE's first
//      instruction.  It cannot see a CALLER that skips a callee's stack
//      shim -- 0x10025680 calls 0x1002803c and 0x10039f90 calls 0x10039bdc,
//      both of which are twelve bytes INSIDE a `mov N(%esp),{eax,edx,ecx}`
//      adapter (0x10028030 and 0x10039bd0).  No C expresses that call.
//
//  The expected outcome:
//
//      sub_1003b3b0    likely to reproduce; the only clean one.  A
//                      call-free counted loop over a global array.
//      sub_10021c20    right source, WILL NOT match: cl pushes.
//      sub_10021ce0    right source, WILL NOT match: cl pushes.
//      sub_1003b690    right source, WILL NOT match: dead 8-byte slot and
//                      a tails-at-the-end block layout cl inverts.
//      sub_10025680    right source, CANNOT match: calls a register-ABI
//                      entry point twelve bytes into a shim.
//      sub_10039f90    right source, CANNOT match: same, plus it saves
//                      %edi with a `mov` into its own frame.
//
//  Everything below is DECODED -- every instruction read -- and the two
//  INFERRED points are flagged where they occur.
//
// *********************************************************************** //

#include "ltbasedefs.h"

#include <stdlib.h>


// ----------------------------------------------------------------------- //
//  The engine's side of the RenderStruct, at 0x1008cbcc.
//
//  RenderDLLSetup stores its argument there (d3dneutral.cpp records the one
//  store), and d3d.cpp's map names the first block: +0x10 ConsolePrint,
//  +0x14 ConsolePrintf, +0x34 dalloc, +0x38 dfree.  Only the two allocator
//  slots are needed here.  Both are __cdecl -- each call site restores the
//  outgoing area itself.
// ----------------------------------------------------------------------- //

struct RenderStructCallbacks
{
	uint8   m_Reserved00[0x34];                         // +0x00..+0x33
	void* (*dalloc)(uint32 nSize);                      // +0x34
	void  (*dfree)(void* pMem);                         // +0x38
};

extern RenderStructCallbacks* g_pStruct;                // 0x1008cbcc


// ----------------------------------------------------------------------- //
//  0x10021c20 -- 38 bytes of body (40 with the two-byte pad).
//
//  WHAT IT IS: the renderer's raw allocator -- ask the engine if it is
//  there, otherwise fall through to the CRT.
//
//  WHAT PINS IT:
//    * `mov 0x4(%esp),%edx` and a plain `ret`: one argument, __cdecl.
//    * `mov 0x1008cbcc,%eax; test %eax,%eax; je` -- the whole body is a
//      null test on the RenderStruct pointer.
//    * the taken arm calls `*0x34(%eax)`, which d3d.cpp's map names dalloc;
//      the other arm calls 0x100809f8, which is the CRT malloc (it pushes
//      _crtheap at 0x10091e60 and tail-calls _nh_malloc at 0x10080a0a).
//    * %eax is not touched after either call, so the callee's answer IS the
//      answer.  Confirmed from the far side: 0x10023fc3 takes the result
//      into %ebx and null-tests it.
//    * one argument confirmed at every call site (0x10023fc3 passes
//      `nCount << 5`).
//
//  This is d3d_Alloc (0x10021c50) MINUS the zero-fill and minus the
//  out-of-memory message -- 0x10021c50 is this same pair of arms followed
//  by memset(p, 0, n) and a `dlogprint` of "d3drender.dll: out of memory".
//  0x10021c20 is the uncleared form and has five callers.
//
//  WHY IT WILL NOT MATCH: `sub $0xc,%esp` / `mov %edx,(%esp)` /
//  `add $0xc,%esp` is accumulate-outgoing-args.  cl emits
//  `push edx; call ...; add esp,4`.  Expect roughly 30 bytes from cl
//  against 38 shipped, with the control flow identical.
// ----------------------------------------------------------------------- //


// ----------------------------------------------------------------------- //
//  0x10021ce0 -- 36 bytes.
//
//  Instruction for instruction the SAME FUNCTION as 0x10021c20: same load
//  of 0x1008cbcc, same `*0x34(%eax)`, same malloc.  The only difference is
//  block layout -- 0x10021c20 duplicates its `add $0xc,%esp; ret` tail into
//  both arms, 0x10021ce0 shares one tail and reaches it with `jmp` (which
//  is also how the neighbouring d3d_Free at 0x10021cb0 is laid out).  Two
//  copies of one source function, emitted into two objects.
//
//  WHAT PINS THE PAIRING: 0x10021ce0's only two callers are 0x1007fae5 and
//  0x1007fc08, both ABOVE 0x1007fa10 -- i.e. inside the linked-in MSVC CRT.
//  0x1007fae0 is `mov 0x4(%esp),%eax; push %eax; call 0x10021ce0;
//  add $0x4,%esp; ret $0x4` and 0x1007faf0 is the same shape around
//  d3d_Free (0x10021cb0).  That is the operator new / operator delete pair
//  the module supplies, so this copy is the one `new` reaches.  0x10021c20
//  is the copy the renderer's own code reaches.
//
//  WHY IT WILL NOT MATCH: identical to 0x10021c20's reason.
// ----------------------------------------------------------------------- //
void* sub_10021ce0(uint32 nSize)
{
	void* pRet;

	if (g_pStruct)
		pRet = g_pStruct->dalloc(nSize);
	else
		pRet = malloc(nSize);

	return pRet;
}


// ----------------------------------------------------------------------- //
//  0x10025680 -- 32 bytes.
//
//  WHAT IT IS: a one-argument wrapper that calls 0x10028030 with two
//  throwaway locals and a fixed global.
//
//  WHAT PINS IT:
//    * `mov 0x4(%esp),%eax` then a plain `ret`, and both call sites clean
//      eight bytes while writing only the first: ONE argument, __cdecl.
//      (0x100406da's extra `push %edi` is alignment filler -- %edi is dead
//      there -- and 0x10066a65 does the same job with `add $-8,%esp` and a
//      single store.)
//    * `sub $0x1c,%esp`, then `movl $0x1008ba9c,0xc(%esp)` and nothing else
//      written below 0x10.  0x10(%esp) and 0x14(%esp) are LOCALS whose
//      addresses go out in %edx and %ecx.
//    * the callee: 0x10028030 is `mov 0x4(%esp),%eax; mov 0x8(%esp),%edx;
//      mov 0xc(%esp),%ecx` falling into the real body at 0x1002803c.  That
//      body reads 0x14(%ebp) as well, i.e. a FOURTH argument -- which is
//      exactly the 0xc(%esp) slot this function fills.  So the call is
//      four-argument: (x, &a, &b, &g_1008ba9c).
//    * 0x1002803c's own callers 0x10025859 and 0x10027910 use the same
//      register-plus-one-stack-slot shape, which is the cross-check that
//      the shim is being bypassed on purpose and not a mis-split.
//
//  INFERRED: nothing about the two locals is recoverable -- 0x1002803c
//  stores them into -0x90(%ebp)/-0x8c(%ebp) and passes them on.  They are
//  left as dwords, and the callee keeps its address for a name.
//
//  WHY IT CANNOT MATCH: written as C, cl calls 0x10028030 with four pushes.
//  The shipped code jumps past the shim and hands three of the four
//  arguments over in %eax/%edx/%ecx.  No source shape does that.
// ----------------------------------------------------------------------- //

extern uint32 g_1008ba9c;                               // 0x1008ba9c

// 0x10028030 -- the stack-ABI entry; the body is at 0x1002803c.
extern void sub_10028030(uint32 nArg0, uint32* pOut0, uint32* pOut1,
                         uint32* pArg3);



// ----------------------------------------------------------------------- //
//  0x10039f90 -- 78 bytes of body (80 with the two-byte pad).
//
//  WHAT IT IS: fetch two dwords out of a texture's renderer-side data,
//  creating that data first if it is not there yet.  Answers 1 on success
//  and 0 if the creation failed.
//
//  WHAT PINS IT:
//    * `push %ebx` then `mov 0x8(%esp),%ebx` -- that is 0x4(%esp) before
//      the push, so %ebx is ARGUMENT 1.  After `sub $0x18,%esp` the other
//      three arrive at 0x24, 0x28 and 0x2c(%esp): four arguments, __cdecl.
//    * `mov 0xc(%ebx),%eax; test %eax,%eax; je` -- everything hangs on
//      pTexture->+0x0c being null.
//    * the null arm calls 0x10039bdc with %eax = arg1, %edx = arg2,
//      %ecx = 0, tests the answer, and on success jumps BACK INTO the
//      success arm at 0x10039f9f, which RELOADS 0xc(%ebx).  The reload is
//      the proof that the call is what fills +0x0c.
//    * the success arm stores `pData->+0x04` through arg3 and
//      `pData->+0x08` through arg4, in that order, then `mov $0x1,%eax`.
//      The failure tail is `xor %eax,%eax`.
//    * the two outputs are copied with INTEGER moves (`mov 0x4(%eax),%ecx;
//      mov %ecx,(%edx)`), so they are written here as uint32 even though
//      the caller at 0x1005a010 reads the destination with `flds` and
//      multiplies it.  Writing `float` produces fld/fstp, which is a
//      different function.  (This is the trap d3dmisc.cpp records against
//      d3d_TransposeRotation.)
//    * arg2 is a small index at both call sites (0x1005a007 and 0x1005a098
//      scale it by 4 and by 8 to address a caller-side array), so it is a
//      count/stage, not a pointer.
//
//  CONTEXT, not needed to pin it: 0x10039bd0/0x10039bdc opens by calling
//  g_pStruct->+0x08, which d3d.cpp names GetTextureData, so arg1 is an
//  engine SharedTexture and +0x0c is the renderer's cached form of it.
//  0x10039930's teardown Releases that same +0x0c and subtracts the
//  object's +0x10 from g_pStruct->+0x50, a byte counter.
//
//  WHY IT CANNOT MATCH: the call goes to 0x10039bdc, twelve bytes past the
//  `mov N(%esp),{eax,edx,ecx}` shim at 0x10039bd0, with the arguments
//  already in registers.  Also `mov %edi,0x10(%esp)` saves a register into
//  the frame instead of pushing it.
// ----------------------------------------------------------------------- //

struct D3DTextureData
{
	uint32  m_nUnknown00;                               // +0x00
	uint32  m_nUnknown04;                               // +0x04
	uint32  m_nUnknown08;                               // +0x08
};

struct D3DSharedTexture
{
	uint8            m_Reserved00[0x0c];                // +0x00..+0x0b
	D3DTextureData*  m_pData;                           // +0x0c
};

// 0x10039bd0 -- the stack-ABI entry; the body is at 0x10039bdc.
extern uint32 sub_10039bd0(D3DSharedTexture* pTexture, uint32 nIndex,
                           uint32 nFlags);



// ----------------------------------------------------------------------- //
//  0x1003b3b0 -- 30 bytes of body (32 with the pad).  THE ONE TO MEASURE.
//
//  WHAT IT IS: clear the dirty-rectangle list.
//
//  WHAT PINS IT:
//    * no arguments, no calls, no stack frame at all -- the whole body is
//      `mov $0x1008e734,%eax; mov $0x64,%edx; xor %ecx,%ecx`, four stores
//      at (%eax), 0x4, 0x8, 0xc, then `add $0x10,%eax; dec %edx; jne`.
//    * 0x1008e734 IS the dirty-rect array and 0x1008e730 its count.
//      d3dblit.cpp already established that from d3d_UnlockScreen, and
//      0x1003b3d0 (AddDirtyRect) confirms every field of it: it indexes
//      0x1008e734 by `count << 4`, copies four dwords in with
//      `mov $0x4,%ecx; rep movsl`, `incl 0x1008e730`, and REFUSES at
//      `cmp $0x63,%edx; jae` -- so the entry is 16 bytes, the array holds
//      100 of them, and only 99 are ever filled.  0x1003b3a0, the
//      immediate neighbour, is the count-only reset and is already in
//      d3dneutral.cpp.
//    * the four members are compared with `jg`/`jl`/`jge` inside
//      0x1003b3d0's overlap test, so they are SIGNED: a RECT.
//    * the zero is materialised in a register and reused four times, which
//      is the shape d3d_ResetCounters (0x1001eab0, 23/23 byte-identical)
//      already proved cl picks once a constant has more than one use.
//    * `add $0x10,%eax; dec %edx; jne` is cl /O2's own strength-reduced
//      counted loop: the index is gone, the pointer walks, the trip count
//      counts down.
//
//  The array is written as 100 entries because that is what this function
//  clears; AddDirtyRect's own limit of 99 is a separate, smaller bound.
//
//  RESIDUAL RISK, stated honestly: the three setup instructions
//  (pointer, count, zero) have no forced order and no forced register
//  assignment.  If the measurement comes back size-exact with two or three
//  bytes differing, that is register naming and the source is right.
//  If it comes back a different LENGTH, the loop shape is wrong and the
//  next thing to try is the pointer-walking form:
//      D3DDirtyRect* p = g_DirtyRects;
//      uint32 i;
//      for (i = 100; i; --i, ++p) { p->left = 0; ... }
// ----------------------------------------------------------------------- //

struct D3DDirtyRect
{
	int32   left;                                       // +0x00
	int32   top;                                        // +0x04
	int32   right;                                      // +0x08
	int32   bottom;                                     // +0x0c
};

extern D3DDirtyRect g_DirtyRects[100];                  // 0x1008e734, defined in d3dmid.cpp



// ----------------------------------------------------------------------- //
//  0x1003b690 -- 95 bytes.
//
//  WHAT IT IS: translate the renderer's internal texture-blend enum into a
//  D3DTEXTUREOP, falling back to a second enum value when the first is not
//  one it knows.
//
//  WHAT PINS IT:
//    * `mov 0x4(%esp),%eax` and a plain `ret`: __cdecl.  0x10(%esp) after
//      `sub $0x8,%esp` is 0x8(%esp) at entry, so there is a SECOND
//      argument, and both call sites clean eight bytes.
//    * five `cmp $N,%eax; je` in ascending order 1,2,3,4,5, each reaching
//      a `mov $M,%eax; ret` tail: 1->2, 2->3, 3->4, 4->7, 5->8.
//    * the fall-through loads the SECOND argument and calls 0x1003b690 --
//      ITSELF -- with that value pushed TWICE, then returns what came
//      back.  So the default is `return sub_1003b690(nFallback,
//      nFallback);`, and the recursion terminates because the second call
//      cannot take the default arm twice with a value the caller vouches
//      for.
//    * WHAT THE NUMBERS ARE.  Both callers (0x1003d4bf and 0x1003d524)
//      feed the answer straight into `call *0x94(%edx)` on the device at
//      0x1008d988, as the fourth of four arguments -- (this, 0, 1, value)
//      at one site and (this, 0, 4, value) at the other.  Slot 0x94 is
//      index 37 of IDirect3DDevice7, SetTextureStageState, and its states
//      1 and 4 are D3DTSS_COLOROP and D3DTSS_ALPHAOP.  The outputs are
//      therefore D3DTEXTUREOPs, and 2/3/4/7/8 are SELECTARG1, SELECTARG2,
//      MODULATE, ADD and ADDSIGNED.  That also explains the two constants
//      the callers pass as the fallback -- 3 (-> MODULATE) at 0x1003d4bd
//      and 1 (-> SELECTARG1) at 0x1003d51f.
//    * both callers open with the SAME five-test chain on the same enum
//      before giving up and calling here, which is the cross-check that
//      the domain really is 1..5.
//
//  INFERRED: the input enum has no name here.  Its neighbour 0x1003b6f0
//  translates the same kind of value into a D3DBLEND (1->1, 3->3, 4->4,
//  5->5, 6->6, default 2 = D3DBLEND_ONE), so 1..6 is the renderer's own
//  blend-mode enumeration, but nothing in this module spells it.
//
//  WHY IT WILL NOT MATCH: `sub $0x8,%esp` at entry is DEAD -- it is undone
//  by an `add $0x8,%esp` in front of every one of the six returns, and the
//  recursive call pushes its arguments rather than storing into it.  That
//  is the dummy-slot fingerprint at eight bytes instead of four.  On top of
//  that the five `mov $M,%eax; ret` tails are laid out AFTER the default
//  arm and in the order 4,3,2,1,5, which is the block-layout difference
//  d3dneutral.cpp already records against the 0x10030f30 error-string
//  chain; cl lays if-chains out the other way.
// ----------------------------------------------------------------------- //


// ----------------------------------------------------------------------- //
//  0x100399bf -- NOT A FUNCTION.  Nothing is written for it.
//
//  The row is an enumeration artefact.  0x10039930 is 143 bytes and
//  0x10039930 + 143 = 0x100399bf exactly, so the enumerator ended one
//  function and started another at the same address -- but 0x100399bf is
//  not even an instruction boundary: 0x100399bb holds `0f 84 7b 00 00 00`
//  (je 0x10039a3c), which runs through 0x100399c0.
//
//  What is really there is ONE function that starts at 0x10039930 and ends
//  at 0x10039a3b with `mov (%esp),%ebx; mov 0x4(%esp),%esi;
//  mov 0x8(%esp),%edi; mov %ebp,%esp; pop %ebp; ret`, plus two out-of-line
//  arms at 0x10039a3c and 0x10039a50 that jump back into it.  It is the
//  0x10039930 that d3ddraw.cpp already declares as d3d_OnSurfacesRestored,
//  it carries a frame pointer, `add $-8,%esp` and `mov`-based register
//  saves, and it runs to 0x10039a5e -- 303 bytes, not 143.
//
//  ACTION: drop the row, and treat the size column for 0x10039930 as wrong
//  too.
// ----------------------------------------------------------------------- //
