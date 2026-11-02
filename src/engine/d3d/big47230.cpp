// *********************************************************************** //
//
//  MODULE:   BIG47230 (.CPP)
//
//  PURPOSE:  0x10047230 -- CD3DModelDraw's SET-THE-PASS-TEXTURES method.
//            Given a pass index (0 or 1) it binds that pass's texture block
//            to up to THREE Direct3D texture stages: the base stage
//            m_n034, the optional second stage m_n038, and stage 1 for the
//            block's detail sub-block.  Where a stage cannot be served it
//            UNBINDS instead.  The whole thing is the texture-record
//            find/create/bind block of big729c0.cpp, big0ae20.cpp,
//            big6d910.cpp and big43e10.cpp, THREE TIMES OVER, with the
//            LOD argument no longer a constant.
//
//  ------------------------------------------------------------------- //
//  THE EXTENT -- 0x10047230..0x10047afb, 2,251 bytes of body.  CONFIRMED
//  AT BOTH EDGES, BY HAND.
//  ------------------------------------------------------------------- //
//
//      0x10047230..0x10047b00  2,256 bytes  604 instructions
//      ret               2      0x100478e9, 0x100478fa
//      push %ebp frames  1      0x10047230
//      branch targets    0 outside the range or mid-instruction
//      last instruction  ends flush, BUT the final 5 bytes are alignment
//                        padding -- the body ends at 0x10047afb
//
//  THE UPPER EDGE.  The preceding function ends
//
//      10047224: call *0x64(%eax)      ; on g_pD3DDevice
//      10047227: mov  %ebp,%esp
//      10047229: pop  %ebp
//      1004722a: ret                   ; plain c3, no `ret $N`
//      1004722b: nop                   ; 90
//      1004722c: lea  0x0(%esi,%eiz,1),%esi   ; 8d 74 26 00
//      10047230: push %ebp             ; <- THIS function
//
//  Five bytes of lea-nop padding sit between the previous `ret` and our
//  `push %ebp`, so 0x10047230 is a real function start and NOT a /Qip
//  register entry: the bytes immediately above it are PADDING, not
//  argument loads.  That was checked by hand.
//
//  THE LOWER EDGE.  0x10047afb is `nop` and 0x10047afc is
//  `lea 0x0(%esi,%eiz,1),%esi` -- the same five-byte pad -- and 0x10047b00
//  opens `push %ebp ; mov %esp,%ebp ; sub $0x10,%esp`, which is
//  d3dr5a.cpp's already-decoded sub_10047b00 (the "ModelSpecular" /
//  "ModelTexture" console-variable registrar).  NOTHING in our range
//  branches across the padding: the highest branch target in the whole
//  function is 0x10047af3, and the two trampolines that live at
//  0x10047aeb and 0x10047af3 both jump BACKWARD to 0x1004733b.
//
//  THE TWO RETS ARE ONE EPILOGUE, DUPLICATED BY /O2.  Both are `c2 04 00`
//  (`ret $0x4`) and both are preceded by the SAME five instructions:
//
//      100478db: mov (%esp),%ebx ; mov 0x4(%esp),%esi ; mov 0x8(%esp),%edi
//                mov %ebp,%esp ; pop %ebp ; ret $0x4
//      100478ec: mov (%esp),%ebx ; mov 0x4(%esp),%esi ; mov 0x8(%esp),%edi
//                mov %ebp,%esp ; pop %ebp ; ret $0x4
//
//  Seventeen bytes each, byte for byte identical.  The SECOND copy has
//  exactly ONE predecessor -- `je 0x100478ec` at 0x1004724b, the
//  `if (!m_n304) return;` at the very top -- and no fall-through into it
//  (0x100478e9 is a `ret`).  The first copy serves everything else.  So
//  this is the "duplicated epilogue", i.e. one early `return`
//  statement given its own exit rather than jumping into the shared one;
//  it is NOT a second function.  One prologue, one frame, and every one of
//  the 604 instructions uses it.
//
//  Call census:
//
//      2,251 bytes, 24 calls (6 direct, 18 indirect), 94 bytes per call
//
//  The 6 direct calls are ALL to 0x10039bd0.  Distinct direct callees: 1.
//  Distinct indirect vtable slots: 5 (+0x50, +0x8c, +0x90, +0x94 on the
//  device, +0xbc on the surface).  A low distinct-
//  callee count makes the check witnessing rather than discriminating, so
//  THE CENSUS AT THE BOTTOM IS NOT DECORATION HERE -- it is the second
//  instrument, and it comes out with no residue in every row.
//
//  ------------------------------------------------------------------- //
//  THE SIGNATURE:  `void __thiscall CD3DModelDraw::sub_10047230(uint32)`
//  ------------------------------------------------------------------- //
//
//    * __thiscall.  `mov %ecx,%ebx` at 0x10047241 with nothing having set
//      %ecx inside the function, and every subsequent %ebx dereference is
//      a CD3DModelDraw field (+0x04, +0x30, +0x34, +0x38, +0x304, +0x308,
//      +0x30c, +0x428, +0x6a4).
//    * ONE dword argument.  A census of every `(%ebp)` operand in the 604
//      instructions gives EXACTLY ONE positive displacement -- `0x8(%ebp)`
//      at 0x1004725f -- against seven negative ones (-0x04, -0x08, -0x0c,
//      -0x10, -0x14, -0x18, -0x1c).  `ret $0x4` at both epilogues confirms
//      the count from the other side: the callee pops four bytes.
//    * uint32, not int32.  `cmp $0x2,%esi ; jae` at 0x10047262 is an
//      UNSIGNED compare, so the source's bound check is on an unsigned.
//    * void.  Neither epilogue sets %eax; the last instruction before the
//      first one is `mov %esi,0x30c(%ebx)`, and the caller does not test
//      the result.
//
//  THE ONE CALLER.  A sweep of all
//  0x86b90 bytes of .text for an `e8`/`e9` displacement resolving to
//  0x10047230 finds exactly ONE hit, 0x10008c18, and a sweep of every
//  section for the literal dword 0x10047230 finds NONE -- so it is not in
//  any vtable or dispatch table either:
//
//      10008c05: mov  0x24(%edi),%eax      ; the pass record's +0x24
//      10008c08: mov  0x30c(%ebx),%edx     ; m_n30c -- OUR OWN CACHE
//      10008c0e: cmp  %eax,%edx
//      10008c10: je   0x10008c2b           ; unchanged: skip
//      10008c12: push %edi                 ; four bytes of room
//      10008c13: mov  %ebx,%ecx            ; this
//      10008c15: mov  %eax,(%esp)          ; the argument, overwriting %edi
//      10008c18: call 0x10047230
//      10008c1d: lea  0x430(%ebx),%eax
//      10008c23: mov  %ebx,%ecx
//      10008c25: push %eax
//      10008c26: call 0x10047b60
//
//  Three things fall out of that.  The `push %edi` is a four-byte stack
//  bump immediately overwritten, so the argument count is one, not two.
//  There is no `add $N,%esp` afterwards, which is `ret $0x4` doing the
//  cleanup -- __thiscall, confirmed from the call site.  And the caller
//  guards the call on `m_n30c != value`, which is exactly the field this
//  function's last statement writes: m_n30c is the "textures currently
//  bound for pass N" cache, and 0x10047230 is its setter.
//
//  ------------------------------------------------------------------- //
//  WHAT PINS THE EIGHTEEN INDIRECT TARGETS
//  ------------------------------------------------------------------- //
//
//  The call check matches indirect calls BY POSITION ONLY, so each slot needs its
//  own evidence.  All five offsets are recorded independently by
//  big729c0.cpp, big0ae20.cpp, big6d910.cpp, big2ef34.cpp and
//  big43e10.cpp, AND each is corroborated here by its own operands:
//
//    +0x8c  SetTexture            6 sites (0x100472b9, 0x100473f5,
//           0x100475bf, 0x1004777a, 0x10047888, 0x100478ca).  Three dwords
//           (`add $-0xc,%esp`).  In the three BIND sites argument 2 is the
//           stage NARROWED WITH `movzbl %al,%eax` out of pRec->m_nStage42
//           and argument 3 is pRec->+0x0c, which every other user of this
//           record treats as an IDirectDrawSurface7.  In the three UNBIND
//           sites argument 3 is a LITERAL 0 (`movl $0x0,0x8(%esp)`) and the
//           store that follows sets g_a1008e684[stage] to 0 -- a device
//           method whose third argument is a surface or NULL and whose
//           second is a small stage index is SetTexture and nothing else.
//    +0x50  SetRenderState        3 sites (0x10047423, 0x100475ed,
//           0x100477a5).  Three dwords, and argument 2 is ALWAYS the
//           LITERAL 0x18 (`movl $0x18,0x4(%esp)`), paired with argument 3
//           = pRec->m_nAlphaRef16.  A literal state id plus a value.
//           0x18 is D3DRENDERSTATE_ALPHAREF; the NUMBER is what is pinned.
//    +0x90  GetTextureStageState  1 site  (0x10047837).  FOUR dwords and
//           the LAST ONE IS AN OUT POINTER -- `movl $0x1008edf0,0xc(%esp)`,
//           the ADDRESS of a global, planted as an immediate.  Only a Get
//           takes one.  What lands in that global is handed straight back
//           to +0x94 at 0x10047680, which fixes the direction and makes
//           this a save/restore pair.
//    +0x94  SetTextureStageState  2 sites (0x1004768e, 0x1004785c).  Four
//           dwords, ALL VALUES.  0x1004768e writes (stage 1, state 1,
//           g_n1008edf0) -- the value +0x90 saved -- and 0x1004785c writes
//           (stage 1, state 1, 1).  Same (stage, state) pair as the Get,
//           opposite direction.
//    +0xbc  IDirectDrawSurface7::SetLOD  6 sites (0x1004747a, 0x10047644,
//           0x100477f5, 0x1004791b, 0x100479c3, 0x10047a69).  NOT ON THE
//           DEVICE.  The vtable comes from the RECORD's own +0x0c
//           (`mov 0xc(%eax),%eax ; mov (%eax),%eax ; call *0xbc(%eax)`), it
//           takes TWO dwords (`add $-0x8,%esp`), and every site is guarded
//           by `pRec->+0x44 != <value>` and followed by
//           `pRec->+0x44 = <value>`.  A surface method taking one integer
//           whose caller caches the last value it passed is SetLOD.
//
//  THE TWO VTABLES MUST NOT BE CONFLATED, and the bytes keep them apart
//  everywhere: every +0x50 / +0x8c / +0x90 / +0x94 site loads its vtable
//  from `0x1008d988` (the device global) and every +0xbc site loads it from
//  a record's +0x0c.  They are declared as two unrelated structs below.
//
//  ------------------------------------------------------------------- //
//  WHAT IS CONFIRMED FROM big729c0.cpp, AND THE ONE THING THAT DIFFERS
//  ------------------------------------------------------------------- //
//
//  CONFIRMED, field for field, at the addresses cited inline:
//    * the record: +0x04/+0x08 the two float scales, +0x0c the surface,
//      +0x10 the byte count, +0x14 the frame stamp, +0x16 the alpha
//      reference, +0x1c/+0x20 the LRU link, +0x30 the next-in-block,
//      +0x42 the stage;
//    * the block: +0x0c the record-list head, +0x30 the frame stamp;
//    * `g_a1008e684[pRec->m_nStage42] = pRec` as the bound-texture table;
//    * the eight-store LRU relink -- dl_Remove, dl_TieOff, then insert
//      AFTER g_Link1008e634.m_pPrev, i.e. at the BACK -- reproduced
//      exactly here at 0x1004737f..0x100473b5, 0x10047549..0x1004757f and
//      0x1004770b..0x10047740.  0x1008e634 is read as `mov 0x1008e634,%eax`
//      at 0x1004739f / 0x10047569 / 0x10047725, which is the head OBJECT's
//      +0x00 field, exactly as big729c0.cpp says;
//    * `g_p1008cbcc->m_nBytes48 += pRec->m_nBytes10` and
//      `pRec->m_nStamp14 = g_w1008bea8` inside the stamp guard;
//    * `g_a1008e664[stage] = { pRec->+0x04, pRec->+0x08 }` then
//      `g_n1008bd80++`;
//    * `pBlock->m_nStamp30 = g_w1008b870` before the search;
//    * the search-then-create shape, with 0x10039bd0's third argument 1
//      when the list is non-empty (and the new record threaded in as the
//      head's successor) and 0 when it is empty, four dwords pushed and
//      `add $0x10,%esp` -- three real arguments plus icl's padding, with
//      `0xc(%esp)` never written at any of the six sites (see the frame
//      census at the bottom);
//    * icl 4.0 declining to fold a redundant null test: 0x10047275 and
//      0x100472db both test the SAME pBlock, and 0x1004765f and 0x10047697
//      both test the SAME pBlock->m_pDetail28.
//
//  WHAT DIFFERS, AND IT SHARPENS big729c0.cpp's READING OF +0x44.
//  big729c0.cpp calls the record's +0x44 `m_nDirty44` and reads its two
//  copies of this tail as
//
//      if (pRec->m_nDirty44) { SetLOD(surface, 0); pRec->m_nDirty44 = 0; }
//
//  Here the same tail appears SIX times and FOUR of them pass a VARIABLE:
//
//      100474 5b: mov  -0x1c(%ebp),%edx      ; nLOD = this->m_n6a4
//      1004745e: cmp  %edx,%eax              ; against pRec->+0x44
//      10047460: je   0x1004748a
//      10047474: mov  %edx,0x4(%esp)         ; SetLOD's argument IS nLOD
//      10047486: mov  %dx,0x44(%eax)         ; and +0x44 becomes nLOD
//
//  (the same at 0x10047625/0x1004763e/0x10047650, 0x100479a4/0x100479b7/
//  0x100479cf and 0x10047a48/0x10047a63/0x10047a75).  The other two --
//  bind C at 0x100477d3 and its L_ElseLOD copy at 0x100478fd -- compare
//  against zero and pass a literal 0.  So +0x44 is the record's CURRENT
//  LOD, the guard is "the LOD I want is not the LOD it has", and
//  big729c0.cpp's two copies are the nLOD == 0 specialisation of the same
//  three statements, not a different idiom.  The field is named
//  `m_nLOD44` below for that reason, with big729c0.cpp's name given beside
//  it so nobody has to guess they are the same offset.
//
//  ALSO DIFFERENT, and worth recording: the SIX SetTexture and SIX SetLOD
//  calls are NOT six stages.  They are THREE binds -- each
//  contributing one SetTexture and one SetLOD in the main path plus one
//  more SetLOD in its sunk already-bound arm -- plus THREE unbinds, which
//  are SetTexture(stage, NULL) and carry no SetLOD.  3 + 3 = 6 SetTexture,
//  3 + 3 = 6 SetLOD, and 3 x 2 = 6 calls to 0x10039bd0.  Every one of the
//  discriminating census rows below is a multiple of three for that
//  reason, which is the arithmetic that proves it is three and not six.
//
//  ------------------------------------------------------------------- //
//  (now BYTE-IDENTICAL -- see THE BIND above
//  CD3DModelDraw::sub_10047230.  The source is three calls of
//  big729c0.cpp's BindBlock inline, not the goto transcription this
//  banner was written against.)
//
//  COMPILER: Intel C/C++ 4.0 /O2, same as the rest of d3d.ren.
//  ------------------------------------------------------------------- //
//
//    * COLD-BLOCK SINKING BELOW THE EPILOGUE, ten-fold -- 510 of 2,251
//      bytes (22.7%).  Six are real source blocks and four are compiler
//      trampolines; both lists are in the COVERAGE table.
//    * REGISTER SAVES STORED, NOT PUSHED: `mov %edi,0x8(%esp)` /
//      `mov %esi,0x4(%esp)` / `mov %ebx,(%esp)` into the bottom of a 0x40
//      frame, with the epilogues loading them back the same way.  There is
//      only ONE `push` and TWO `pop` in the whole function (the frame
//      pointer, once per epilogue).
//    * OUTGOING ARGUMENTS as `add $-N,%esp` then `mov %reg,K(%esp)` at
//      every one of the 24 call sites, never `push`.
//    * THE SEARCH LOOP UNROLLED TWICE in phase A (0x10047310..0x1004731b
//      and 0x10047323..0x10047334) and NOT unrolled in phases B and C.
//      Same source, three times; the unroll is icl's.
//    * `lea 0x0(%esi,%eiz,1),%esi` and `lea 0x0(%esi),%esi` as loop-head
//      alignment padding (0x1004730a, 0x100474ef via `nop`, 0x100476be via
//      `mov %esi,%esi`).
//
//  ------------------------------------------------------------------- //
//  THE ACCEPTANCE FIGURE, AND WHAT IT DOES NOT WITNESS
//  ------------------------------------------------------------------- //
//
//  Predicted: 24 shipped, 24 ours, 24 in agreement -- 6 by resolved target
//  (all 0x10039bd0) and 18 by position only.  The full predicted order and
//  the one thing that can cost a row are at the bottom.
//
//  94 bytes per call is inside the strong band, but ONE distinct
//  direct callee is not, so the figure is reported here as WITNESSING and
//  the discrimination is carried by (a) the interleaving pattern of the
//  sunk tail -- fifteen indirect calls, then `indirect, direct, direct`
//  three times, which is what the six 0x10039bd0 rows actually pin -- and
//  (b) the instruction census, which has no residue in any of its ten
//  discriminating rows.
//
//  The three longest call-free runs are 325 B (14.4%), 316 B (14.0%) and
//  236 B (10.5%), and all three are the same find-the-record-and-relink
//  code, decoded against FIVE independent siblings that reproduce it
//  field for field.  There are NO `// UNDECODED:` gaps.
//
//  /Oi- JUDGEMENT: NOT WANTED, from THIS function's census.  The residual
//  /Oi- fixes is CRT intrinsics -- sqrt, fabs, tan, atan, memcpy.  This
//  function has SEVEN x87 instructions in 602 (`flds` 2, `fmuls` 2,
//  `fstps` 2, `fxch` 1) and they are two plain multiplies; there is no
//  `fsqrt`, no `fabs`, no `fpatan`, no `fcos`/`fsin`, no `fdiv`.  There is
//  also no block copy: zero `rep movs`, and the widest structure store in
//  the function is a `mov %ax,K(%reg)`.  So there is nothing for /Oi- to
//  bring back, and the risk that it can manufacture an ours-only
//  `memcpy` row applies with nothing on the other side of the trade.
//  Census: 0 candidate sites.  DECLINED WITH REASON.
//
//  /Qrcd JUDGEMENT: NOT WANTED.  ZERO float-to-int conversions -- there is
//  no `fistp`, no `fist`, and no integer destination for any x87 value in
//  the range.  The five `movzbl` and thirty-four `movzwl` here are all
//  INTEGER field narrowings and are fully decomposed in the census below;
//  /Qrcd does not touch them.
//
//  DEAD 1.0f SLOTS: NONE, and none is expected.  There is no `fdiv` in the
//  function, so the dead-1.0f-per-division row is empty rather than
//  unexplained -- and in fact this function has NO literal pool entry at
//  all: all four x87 memory operands (0x1008e664, 0x1008e668, 0x1008d7c0
//  twice) and both destinations (0x1008ac90, 0x1008ac94) are .data
//  globals, not .rdata constants.
//
//  `_chkstk`: NOT PRESENT.  The frame is 0x40 = 64 bytes, far under the
//  probe threshold, and there is no `mov $N,%eax ; call 0x100805a0`.
//
//  THE /Qip `f+4` QUESTION does not arise: the only direct callee is
//  0x10039bd0, which is d3dmid.cpp's STACK entry (the register entry is
//  0x10039bdc), it is not in this TU, and the call check accepts the offset
//  anyway.
//
// *********************************************************************** //

#include "ltbasedefs.h"


// ======================================================================= //
//  TYPES
//
//  Every offset below is one this function's bytes touch, with the
//  touching address given.  The record, the block, the per-stage scale
//  pair and the two vtables are big729c0.cpp's and big43e10.cpp's; the
//  three fields those files leave inside padding (the block's +0x28, the
//  object's +0x1b8 array, the class's +0x308/+0x30c/+0x6a4) are spelled
//  out here, each against the instruction that reaches it.
// ======================================================================= //

struct Tex47230;
struct TexBlock47230;
struct DDSurface47230;


// ----------------------------------------------------------------------- //
//  ltlink.h's CheapLTLink, as big729c0.cpp and big6d910.cpp declare it.
// ----------------------------------------------------------------------- //
struct Link47230
{
	Link47230*      m_pPrev;                            // +0x00
	Link47230*      m_pNext;                            // +0x04
	void*           m_pData;                            // +0x08

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

	inline void AddAfter(Link47230* pLink)
	{
		pLink->m_pPrev = this;
		pLink->m_pNext = m_pNext;
		pLink->m_pPrev->m_pNext = pLink->m_pNext->m_pPrev = pLink;
	}
};

inline void dl_Insert(Link47230* pAfter, Link47230* pLink)
{
	pAfter->AddAfter(pLink);
}

inline void dl_Remove(Link47230* pLink)
{
	pLink->Remove();
}


// ----------------------------------------------------------------------- //
//  THE PER-STAGE TEXTURE RECORD.  big729c0.cpp's Tex729c0 and
//  big43e10.cpp's Tex440e3, field for field.  This function reads +0x04,
//  +0x08, +0x0c, +0x10, +0x14, +0x16, +0x1c, +0x20, +0x30, +0x42 and +0x44
//  and writes +0x14, +0x1c, +0x20, +0x30 and +0x44 -- every field either
//  file names.  +0x00 is the surface-ish first dword nothing here touches.
//
//  +0x44 IS THE CURRENT LOD, not a dirty flag; see the banner.  The name
//  big729c0.cpp / big43e10.cpp use for it (`m_nDirty44`) is kept in the
//  comment so the two readings can be lined up.
// ----------------------------------------------------------------------- //
struct TexBase47230
{
	uint32          m_n00;                              // +0x00
	float           m_fInvW04;                          // +0x04  0x1004743d
	float           m_fInvH08;                          // +0x08  0x10047440
	DDSurface47230* m_pSurface0c;                       // +0x0c  0x1004746e
	uint32          m_nBytes10;                         // +0x10  0x100473bb
	uint16          m_nStamp14;                         // +0x14  0x10047371
	uint16          m_nAlphaRef16;                      // +0x16  0x100473fe
	uint8           m_pad_18[0x1c - 0x18];
	Link47230       m_Link1c;                           // +0x1c  0x1004737f
	uint8           m_pad_28[0x30 - 0x28];
};

struct Tex47230 : public TexBase47230
{
	Tex47230*       m_pNext30;                          // +0x30  0x1004731c
	uint8           m_pad_34[0x42 - 0x34];
	uint16          m_nStage42;                         // +0x42  0x10047310
	uint16          m_nLOD44;                           // +0x44  0x10047457
	                                                    //   (= m_nDirty44)
};


// ----------------------------------------------------------------------- //
//  THE TEXTURE BLOCK.  big729c0.cpp's TexBlock729c0 / big43e10.cpp's
//  TexBlock440e3 -- +0x0c the record-list head (0x100472ec, 0x100474d4,
//  0x100476a6) and +0x30 the sixteen-bit frame stamp (0x100472f4,
//  0x100474dc, 0x100476ae).
//
//  +0x28 IS NEW HERE and both files leave it inside padding.  It is
//  another block of the SAME type: `mov 0x28(%edi),%eax` at 0x1004765f is
//  null-tested, and the value re-read at 0x10047694 is then used as a
//  block in every way the outer one is -- `mov %ax,0x30(%edx)` (the stamp,
//  0x100476ae), `mov 0xc(%edx),%ecx` (the head, 0x100476a6) and as
//  0x10039bd0's first argument at 0x10047946 / 0x1004797f, which is where
//  every sibling passes a block.  Named m_pDetail28: big43e10.cpp's
//  stage-1 arm, gated on the same m_n030, is its "DETAIL-TEXTURE STAGE-1
//  PATH".
// ----------------------------------------------------------------------- //
struct TexBlock47230
{
	uint8           m_pad_00[0x0c];
	Tex47230*       m_pHead0c;                          // +0x0c
	uint8           m_pad_10[0x28 - 0x10];
	TexBlock47230*  m_pDetail28;                        // +0x28  0x1004765f
	uint8           m_pad_2c[0x30 - 0x2c];
	uint16          m_nStamp30;                         // +0x30
};


// ----------------------------------------------------------------------- //
//  THE PAIR PUBLISHED PER STAGE at 0x1008e664 --
//  `mov %edi,0x1008e668(,%ecx,8)` and `mov %esi,0x1008e664(,%ecx,8)` at
//  0x10047449/0x10047450 (and 0x10047613/0x1004761a, 0x100477c5/
//  0x100477cc).  Both are INTEGER moves of floats, as in every sibling.
//  Element size 8 is read off the `,8` scale.
// ----------------------------------------------------------------------- //
struct TexScale47230
{
	float           m_fInvW00;                          // +0x00
	float           m_fInvH04;                          // +0x04
};


// 0x1008cbcc, d3dmid.cpp's RenderStructCallbacks / big43e10.cpp's
// Stats440e3.  Only +0x48 is touched (`add %ecx,0x48(%eax)` at 0x100473be
// and 0x10047588, `add %edx,0x48(%eax)` at 0x10047749) -- the same field
// big0ae20.cpp, big6d910.cpp, big43e10.cpp and big729c0.cpp record.
struct Stats47230
{
	uint8           m_pad_00[0x48];
	uint32          m_nBytes48;                         // +0x48
};


// ======================================================================= //
//  THE DEVICE AND THE SURFACE.  The five slots are pinned in the banner;
//  the shapes are big729c0.cpp's and big43e10.cpp's, unchanged.  Note that
//  NO indirect call here is followed by an `add $N,%esp`, so all five are
//  __stdcall.
// ======================================================================= //
struct D3DDevice47230;

struct D3DDevice47230Vtbl
{
	uint8   m_pad_00[0x50];
	long (__stdcall* SetRenderState)(D3DDevice47230*, uint32, uint32);
	                                                    // +0x50
	uint8   m_pad_54[0x8c - 0x54];
	long (__stdcall* SetTexture)(D3DDevice47230*, uint32, void*);
	                                                    // +0x8c
	long (__stdcall* GetTextureStageState)(D3DDevice47230*, uint32, uint32,
	                                       uint32*);    // +0x90
	long (__stdcall* SetTextureStageState)(D3DDevice47230*, uint32, uint32,
	                                       uint32);     // +0x94
};

struct D3DDevice47230
{
	D3DDevice47230Vtbl* lpVtbl;
};

struct DDSurface47230Vtbl
{
	uint8   m_pad_00[0xbc];
	long (__stdcall* SetLOD)(DDSurface47230*, uint32);   // +0xbc
};

struct DDSurface47230
{
	DDSurface47230Vtbl* lpVtbl;
};


// ======================================================================= //
//  GLOBALS.  Relocations are masked, so only the shapes matter; the names
//  are the ones already in the tree.
// ======================================================================= //

extern D3DDevice47230*  g_pD3DDevice;                   // 0x1008d988

extern uint16           g_w1008b870;                    // 0x1008b870
extern uint16           g_w1008bea8;                    // 0x1008bea8
extern uint32           g_n1008bd80;                    // 0x1008bd80
extern Stats47230*      g_p1008cbcc;                    // 0x1008cbcc
extern uint32           g_n1008d7d0;                    // 0x1008d7d0  stage
extern uint32           g_n1008edf0;                    // 0x1008edf0

// 0x1008d7c0 -- big331e0.cpp names it and records that 0x100331e0 sets it
// to 0.5f on success.  Read TWICE by the x87 tail (0x10047490, 0x10047496).
extern float            g_f1008d7c0;                    // 0x1008d7c0

// 0x1008ac90 / 0x1008ac94 -- the half-texel offsets the three texture-
// coordinate generators big43e10.cpp installs into m_pfn434 / m_pfn42c
// ADD to every uv pair: 0x10003a19 is `fadds 0x1008ac90` and 0x10003a1f
// `flds 0x1008ac94`, which is what makes them floats rather than the
// dwords 0x10047b71/0x10047b77 zero them as.  Only 0x10047230 (here) and
// 0x10047b60 write them; only 0x10003a10, 0x10003a40 and 0x10003a80 read
// them.
extern float            g_f1008ac90;                    // 0x1008ac90
extern float            g_f1008ac94;                    // 0x1008ac94

// 0x1008ee04 -- d3dr5a.cpp registers this address as the "ModelTexture"
// console variable (`g_1008ee04.sub_10021f40("ModelTexture", 1.0f, 0, 0)`
// at 0x10047b41, inside sub_10047b00).  It is read here as a single dword
// and `test`ed (0x1004727d), exactly as big43e10.cpp reads its sibling
// 0x1008ee24 and declares it `int32`; the same spelling is used so the two
// files agree.  ONE instruction either way, so the two declarations of the
// address cannot be told apart in the bytes.
extern int32            g_n1008ee04;                    // 0x1008ee04

// 0x1008e634 is the 12-byte list HEAD OBJECT, not a pointer variable --
// big0ae20.cpp's correction, settled there by 0x10038e3d planting the
// IMMEDIATE $0x1008e634 into a link's m_pPrev, and repeated by
// big729c0.cpp.  This function reads the head's +0x00 field
// (`mov 0x1008e634,%eax`), which is the instruction a pointer load would
// also emit, so the bytes cannot separate the two spellings and the object
// is the right one.
extern Link47230        g_Link1008e634;                 // 0x1008e634

extern TexScale47230    g_a1008e664[];                  // 0x1008e664
extern Tex47230*        g_a1008e684[];                  // 0x1008e684
                                                        // [1] = 0x1008e688


// ======================================================================= //
//  THE NEIGHBOUR.  The only direct callee, six times.
// ======================================================================= //

// 0x10039bd0 -- d3dmid.cpp's stack-ABI entry (the register entry is
// 0x10039bdc).  Three arguments; big0ae20.cpp, big2ef34.cpp, big6d910.cpp,
// big43e10.cpp and big729c0.cpp all reach it with the same
// (block, stage, bLink) triple, and all SIX call sites here build a
// 16-byte outgoing area and pop 0x10 -- three real arguments plus icl's
// eight-byte-rounding padding, with `0xc(%esp)` never written at any of
// them (see the frame census at the bottom).
extern "C" Tex47230* sub_10039bd0(TexBlock47230* pBlock, uint32 nStage,
                                  uint32 bLink);


// ======================================================================= //
//  `this` -- big17c14.cpp's and big43e10.cpp's CD3DModelDraw, the global
//  object at 0x1008b090.  The class TAG NAME and every field name those
//  two files fixed are copied VERBATIM; the three offsets they leave
//  inside padding (+0x308, +0x30c, +0x6a4) are spelled out here, each
//  against the instruction that reaches it.  The two files already
//  disagree about the padding between +0x034 and +0x304 -- big17c14.cpp
//  pads it and big43e10.cpp names +0x034/+0x038 inside it -- so a third
//  view is precedented rather than novel.
// ======================================================================= //

struct D3DObject47230
{
	uint8           m_pad_000[0x1b8];
	// +0x1b8 -- `mov 0x1b8(%eax,%esi,4),%edi` at 0x1004726e with %esi
	// bounded by `cmp $0x2,%esi ; jae` at 0x10047262.  TWO entries, and
	// big17c14.cpp's D3DObject has EXACTLY EIGHT BYTES of padding at
	// +0x1b8 (`uint8 m_pad_1b8[0x1c0 - 0x1b8]`) before its m_Sub1c0 -- so
	// the array length the bound check gives and the hole the sibling
	// leaves agree to the byte, from two independent directions.
	TexBlock47230*  m_apTex1b8[2];                      // +0x1b8
};

class CD3DModelDraw
{
public:
	void            sub_10047230(uint32 nIndex);        // 0x10047230  THIS

	void*           m_pModel00;                         // +0x000
	D3DObject47230* m_pObject04;                        // +0x004  0x1004726b
	uint8           m_pad_008[0x030 - 0x008];
	uint32          m_n030;                             // +0x030  0x10047654
	uint32          m_n034;                             // +0x034  0x100472d2
	uint32          m_n038;                             // +0x038  0x100474b0
	uint8           m_pad_03c[0x304 - 0x03c];
	uint32          m_n304;                             // +0x304  0x10047243
	uint32          m_n308;                             // +0x308  0x10047251
	uint32          m_n30c;                             // +0x30c  0x10047898
	uint8           m_pad_310[0x428 - 0x310];
	uint32          m_n428;                             // +0x428  0x10047806
	uint8           m_pad_42c[0x6a4 - 0x42c];
	uint32          m_n6a4;                             // +0x6a4  0x100472d5
};


// ======================================================================= //
//  0x10047230
//
//  THE FRAME.  `sub $0x40,%esp` -- 64 bytes -- with the three callee-saved
//  registers STORED into the bottom three dwords (`mov %edi,0x8(%esp)`,
//  `mov %esi,0x4(%esp)`, `mov %ebx,(%esp)` at 0x10047236..0x10047240) and
//  loaded back the same way by both epilogues.  Seven ebp-relative dword
//  slots are used, -0x04 down to -0x1c; nothing touches -0x20..-0x34, so
//  24 bytes of the frame are unaccounted for (see WHAT COULD NOT BE PINNED).
//
//  The frame CANNOT be inverted to a declaration order -- the frame-order rule
//  is not usable here and is not claimed -- because five of the seven slots
//  hold unrelated values in different phases:
//    -0x0c  the mid-unbind's stage (0x10047292), then phase A's pRec
//           (0x1004733b), then phase B's pRec (0x1004750b), then phase C's
//           pHead (0x100476a9) and finally phase C's pRec (0x100476f9)
//    -0x10  phase A's pHead (0x100472ef), then phase A's pRec spilled
//           across the bind (0x10047367), then the pRec spilled across the
//           SetLOD in both sunk already-bound arms (0x10047918, 0x100479c0)
//    -0x14  phase A's pRec (0x10047338), then phase B's pHead (0x100474d7)
//    -0x18  phase A's stage (0x100472e0), then phases A and B's alpha
//           reference (0x10047402, 0x100475cc)
//    -0x04 and -0x08 hold nothing but %esi/%edi spills across the two
//           register-hungry blocks and are not source locals at all.
//  Only -0x1c (the LOD) holds one value for its whole life.
// ======================================================================= //
// ======================================================================= //
//  THE BIND: big729c0.cpp's BindBlock729c0 -- w24's
//  identical BindBlock59380, w3_10014580_os.cpp's 0x10015320 as an inline
//  -- over this file's types, called THREE times: phase A with the base
//  stage m_n034, phase B with the optional second stage m_n038, phase C
//  with stage 1 on the block's detail sub-block and a LOD of 0.  The
//  three copies of the find / create / relink / SetLOD walk in the
//  shipped body are this one inline expanded three times; the shared
//  "unbind the current stage" block is a second inline, Unbind47230.
//
//  BYTE-IDENTICAL.  Was attempted: the goto-for-goto
//  transcription below the banner's region map compiled to a 0x30 frame
//  and 2112 bytes (agreement 0.065).  With the three inline binds the
//  frame is the shipped 0x40 and the shipped 0x10047230 comes out 2256
//  bytes, near; the last step is the gates: `pBlock` defaulted to NULL
//  and assigned under `if (m_n308 && nIndex < 2)`, then `if (pBlock)`.
//  Folding the load into the condition (`&& (pBlock = ...) != NULL`) let
//  icl drop BindBlock's own `if (!pBlock)` in phase A; the shipped body
//  keeps it (0x100472db) -- that one spelling took 0.140 -> identical.
//  The banner's region / slot commentary describes the shipped bytes and
//  still holds; its "written out rather than factored into a helper"
//  reading does not.
// ======================================================================= //
static inline Tex47230* FindRec47230(Tex47230* pHead, uint32 nStage)
{
	Tex47230* pCur;

	for (pCur = pHead; pCur; pCur = pCur->m_pNext30)
	{
		if (pCur->m_nStage42 == (uint8)nStage)
			return pCur;
	}

	return LTNULL;
}

static inline uint32 StageOf47230(Tex47230* pRec)
{
	return pRec->m_nStage42;
}

static inline void SetScale47230(TexScale47230* pScales, uint32 nStage, float fInvW, float fInvH)
{
	pScales[nStage].m_fInvW00 = fInvW;
	pScales[nStage].m_fInvH04 = fInvH;
}

static inline void BindRec47230(Tex47230* pRec)
{
	TexBase47230* pTex = pRec;

	g_a1008e684[StageOf47230(pRec)] = (Tex47230*)pTex;

	if (pTex->m_nStamp14 != g_w1008bea8)
	{
		dl_Remove(&pRec->m_Link1c);
		dl_Insert(g_Link1008e634.m_pPrev, &pRec->m_Link1c);

		g_p1008cbcc->m_nBytes48 += pTex->m_nBytes10;

		pTex->m_nStamp14 = g_w1008bea8;
	}

	g_pD3DDevice->lpVtbl->SetTexture(g_pD3DDevice, (uint8)StageOf47230(pRec),
	                                 pTex->m_pSurface0c);

	if (pTex->m_nAlphaRef16)
		g_pD3DDevice->lpVtbl->SetRenderState(g_pD3DDevice, 0x18,
		                                     pTex->m_nAlphaRef16);

	SetScale47230(g_a1008e664, StageOf47230(pRec), pTex->m_fInvW04, pTex->m_fInvH08);

	g_n1008bd80++;
}

static inline int32 BindBlock47230(TexBlock47230* pBlock, uint32 nStage, uint32 nLOD)
{
	Tex47230* pRec;
	Tex47230* pHead;
	Tex47230* pNew;

	if (!pBlock)
		return 0;

	pBlock->m_nStamp30 = g_w1008b870;

	pHead = pBlock->m_pHead0c;
	pRec  = LTNULL;

	if (pHead)
		pRec = FindRec47230(pHead, nStage);

	if (!pRec)
		goto L_MakeRec;

	if (pRec == g_a1008e684[nStage])
		goto L_ElseLOD;

L_Bind:
	BindRec47230(pRec);

	if (pRec->m_nLOD44 != nLOD)
	{
		pRec->m_pSurface0c->lpVtbl->SetLOD(pRec->m_pSurface0c, nLOD);
		pRec->m_nLOD44 = (uint16)nLOD;
	}

	return 1;

L_ElseLOD:
	if (pRec->m_nLOD44 != nLOD)
	{
		pRec->m_pSurface0c->lpVtbl->SetLOD(pRec->m_pSurface0c, nLOD);
		pRec->m_nLOD44 = (uint16)nLOD;
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

// Forget whatever is bound to a stage (0x10047286, 0x100478a0, and the
// stage-1 copy at 0x10047862).
static inline void Unbind47230(uint32 nStage)
{
	if (g_a1008e684[nStage])
	{
		g_pD3DDevice->lpVtbl->SetTexture(g_pD3DDevice, nStage, LTNULL);
		g_a1008e684[nStage] = LTNULL;
	}
}

void CD3DModelDraw::sub_10047230(uint32 nIndex)
{
	TexBlock47230* pBlock;

	if (!m_n304)
		return;

	pBlock = LTNULL;
	if (m_n308 && nIndex < 2)
		pBlock = m_pObject04->m_apTex1b8[nIndex];

	if (pBlock)
	{
		// Phase A: the base stage, then the half-texel offsets for stage 0.
		if (g_n1008ee04 && BindBlock47230(pBlock, m_n034, m_n6a4))
		{
			g_f1008ac90 = g_a1008e664[0].m_fInvW00 * g_f1008d7c0;
			g_f1008ac94 = g_f1008d7c0 * g_a1008e664[0].m_fInvH04;
		}
		else
		{
			Unbind47230(g_n1008d7d0);
		}

		// Phase B: the optional second stage (-1 = none).
		if (m_n038 != 0xffffffff)
			BindBlock47230(pBlock, m_n038, m_n6a4);

		// Phase C: the detail texture on stage 1.
		if (m_n030 && pBlock->m_pDetail28)
		{
			g_pD3DDevice->lpVtbl->SetTextureStageState(g_pD3DDevice, 1, 1,
			                                           g_n1008edf0);
			BindBlock47230(pBlock->m_pDetail28, 1, 0);
		}
		else if (m_n428 < 2)
		{
			g_pD3DDevice->lpVtbl->GetTextureStageState(g_pD3DDevice, 1, 1,
			                                           &g_n1008edf0);
			g_pD3DDevice->lpVtbl->SetTextureStageState(g_pD3DDevice, 1, 1, 1);
			Unbind47230(1);
		}

		m_n30c = nIndex;
	}
	else
	{
		Unbind47230(g_n1008d7d0);
	}
}


// ======================================================================= //
//  COVERAGE -- every byte of 0x10047230..0x10047afb accounted for.
//
//    ABOVE THE EPILOGUES
//    0x10047230..0x1004727c    77 B  prologue; the four gates
//    0x1004727d..0x100472d1    85 B  the ModelTexture gate; MID-BODY
//                                    UNBIND of g_n1008d7d0
//    0x100472d2..0x10047358   135 B  phase A: stage + LOD, find the record
//    0x10047359..0x10047489   305 B  phase A: bind it
//    0x1004748a..0x100474af    38 B  phase A: publish the two half-texel
//                                    offsets (the ONLY x87 in the range)
//    0x100474b0..0x10047522   115 B  phase B: the -1 gate, find the record
//    0x10047523..0x10047653   305 B  phase B: bind it
//    0x10047654..0x100476eb   152 B  phase C: the m_n030 / +0x28 gates,
//                                    restore stage 1's COLOROP, find the
//                                    record for stage 1
//    0x100476ec..0x10047805   282 B  phase C: bind it
//    0x10047806..0x10047897   146 B  L_Disable: save + disable stage 1's
//                                    state 1, unbind stage 1
//    0x10047898..0x1004789f     8 B  L_Done: m_n30c = nIndex
//    0x100478a0..0x100478da    59 B  L_UnbindAndReturn
//    0x100478db..0x100478eb    17 B  epilogue, copy 1 (shared)
//    0x100478ec..0x100478fc    17 B  epilogue, copy 2 (the early return)
//                            ------
//                            1,741 B
//
//    SUNK BELOW THE EPILOGUES
//    0x100478fd..0x1004792e    50 B  L_ElseLOD3
//    0x1004792f..0x10047998   106 B  L_MakeRec3, two arms
//    0x10047999..0x1004799f     7 B  phase C's `break` trampoline
//    0x100479a0..0x100479d7    56 B  L_ElseLOD2
//    0x100479d8..0x100479de     7 B  phase B's `break` trampoline
//    0x100479df..0x10047a45   103 B  L_MakeRec2, two arms
//    0x10047a46..0x10047a7d    56 B  L_ElseLOD1
//    0x10047a7e..0x10047aea   109 B  L_MakeRec1, two arms
//    0x10047aeb..0x10047af2     8 B  phase A's `break` trampoline #1
//    0x10047af3..0x10047afa     8 B  phase A's `break` trampoline #2
//                            ------
//                              510 B
//                            ======
//                            2,251 B   + 5 B alignment padding = the
//                                        2,256-byte enumeration row
//
//  There are NO `// UNDECODED:` gaps and NO call-free region that is not
//  decoded against a sibling.
//
// ======================================================================= //
//  WHAT THE CALL CHECK WITNESSES, AND WHAT IT DOES NOT
//
//  The 24 call sites partition 2,251 bytes into 25 runs.  Twenty-two of
//  them are 137 bytes or fewer and the median is 59.  The three long ones
//  are:
//
//    0x1004747a..0x100475bf   325 B  (14.4%)  phase A's LOD tail, the x87
//                                    tail, phase B's gate and search, and
//                                    phase B's LRU relink
//    0x100472b9..0x100473f5   316 B  (14.0%)  the mid-unbind's tail, phase
//                                    A's gates and search, and phase A's
//                                    LRU relink
//    0x1004768e..0x1004777a   236 B  (10.5%)  phase C's search and LRU
//                                    relink
//
//  So 39.0% of the bytes sit in three runs, and all three are the SAME
//  find-the-record-and-relink block, decoded field for field against FIVE
//  independent siblings that reproduce it -- big729c0.cpp, big0ae20.cpp,
//  big6d910.cpp, big2ef34.cpp and big43e10.cpp.  Every `test`/`je` pair,
//  the two-fold unroll, the three list-head tests, the eight link stores,
//  the `+= m_nBytes10` and the stamp compare are cited inline at their own
//  addresses.
//
//  AND THE FIGURE HAS TO BE QUALIFIED HARDER THAN big729c0.cpp's.  There
//  is only ONE distinct direct callee (0x10039bd0, six times), so of the
//  24 rows only 6 are checked BY TARGET and 18 only BY POSITION.  What the
//  sequence really pins is a shape: FIFTEEN indirect calls, then
//  `indirect, direct, direct` three times over.  That is enough to prove
//  three find/create/bind blocks with three sunk already-bound arms and
//  three sunk create blocks of two arms each -- but it is NOT enough on
//  its own, and the census below is the second instrument rather than a
//  garnish.
//
// ======================================================================= //
//  PREDICTED CALL-SEQUENCE FIGURE
//
//      shipped calls  24   ours  24
//      in agreement   24   (6 by resolved target, 18 by position only)
//
//  Our emitted order, lexically, against the shipped addresses:
//
//     1  *0x8c   0x100472b9   L_UnbindCur      SetTexture(stage, NULL)
//     2  *0x8c   0x100473f5   bind A           SetTexture
//     3  *0x50   0x10047423   bind A           SetRenderState(0x18, ref)
//     4  *0xbc   0x1004747a   bind A           SetLOD(nLOD)
//     5  *0x8c   0x100475bf   bind B           SetTexture
//     6  *0x50   0x100475ed   bind B           SetRenderState(0x18, ref)
//     7  *0xbc   0x10047644   bind B           SetLOD(nLOD)
//     8  *0x94   0x1004768e   phase C          SetTextureStageState(1,1,g)
//     9  *0x8c   0x1004777a   bind C           SetTexture
//    10  *0x50   0x100477a5   bind C           SetRenderState(0x18, ref)
//    11  *0xbc   0x100477f5   bind C           SetLOD(0)
//    12  *0x90   0x10047837   L_Disable        GetTextureStageState(1,1,&g)
//    13  *0x94   0x1004785c   L_Disable        SetTextureStageState(1,1,1)
//    14  *0x8c   0x10047888   L_Disable        SetTexture(1, NULL)
//    15  *0x8c   0x100478ca   L_UnbindAndRet   SetTexture(stage, NULL)
//    16  *0xbc   0x1004791b   L_ElseLOD3       SetLOD(0)
//    17  0x10039bd0  0x10047949  L_MakeRec3   (pDetail, 1, 1)
//    18  0x10039bd0  0x10047982  L_MakeRec3   (pDetail, 1, 0)
//    19  *0xbc   0x100479c3   L_ElseLOD2       SetLOD(nLOD)
//    20  0x10039bd0  0x100479f8  L_MakeRec2   (pBlock, m_n038, 1)
//    21  0x10039bd0  0x10047a2e  L_MakeRec2   (pBlock, m_n038, 0)
//    22  *0xbc   0x10047a69   L_ElseLOD1       SetLOD(nLOD)
//    23  0x10039bd0  0x10047a9a  L_MakeRec1   (pBlock, m_n034, 1)
//    24  0x10039bd0  0x10047ad3  L_MakeRec1   (pBlock, m_n034, 0)
//
//  That is the shipped order position for position AND slot for slot, so
//  the prediction is 24 of 24.  PREDICTED RESIDUALS BESIDES THAT: none.
//
//    * `_chkstk` DOES NOT ARISE -- the frame is 0x40 bytes and there is no
//      `mov $N,%eax ; call 0x100805a0` at 0x10047230.
//    * /Oi- IS NOT WANTED, judged from this function's own census: seven
//      x87 instructions and all seven are two plain multiplies (no
//      `fsqrt`, `fabs`, `fpatan`, `fcos`, `fsin`, `fdiv`), and zero
//      multi-dword copies (no `rep movs`; the widest structure store in
//      the range is a `mov %ax,K(%reg)`).  There is nothing for it to
//      bring back and the risk that it can manufacture an
//      ours-only `memcpy` row applies with nothing on the other side.
//      DECLINED WITH REASON, as big729c0.cpp declined it on having zero
//      x87 at all -- here the reason is that the x87 present is not
//      intrinsic-shaped.
//    * /Qrcd IS NOT WANTED.  Zero float-to-int conversions: no `fistp`,
//      no `fist`, and the only two x87 destinations are `fstps` to
//      floats.  The 5 `movzbl` and 34 `movzwl` are integer field
//      narrowings, all 39 accounted for below.
//    * NO DEAD `1.0f` SLOTS, and none is predicted: no `fdiv` anywhere,
//      and no `.rdata` operand at all -- all six x87 memory operands are
//      .data globals.  The dead-1.0f row is EMPTY here rather than
//      unexplained.
//    * The /Qip `f+4` question does not arise (0x10039bd0 is the stack
//      entry, not same-TU, and the call check accepts the offset anyway).
//
// ======================================================================= //
//  COIN-FLIPS, each with both spellings, and whether it moves a call
//
//  1. THE ORDER OF THE SIX SUNK BLOCKS -- MOVES NO CALL, and it is worth
//     stating why rather than leaving it as a risk.  They are written in
//     shipped ADDRESS order: phase C's pair first (0x100478fd, 0x1004792f),
//     then phase B's (0x100479a0, 0x100479df), then phase A's (0x10047a46,
//     0x10047a7e).  That is the REVERSE of body order, the same inversion
//     big729c0.cpp records for its four.  If icl emits them in body order
//     instead, positions 16..24 become `*0xbc, bd0, bd0, *0xbc, bd0, bd0,
//     *0xbc, bd0, bd0` -- token for token what they already are, because
//     the three phases' blocks have the same shape and the call check compares
//     indirect calls by position only.  FREE either way.
//
//  2. WHERE THE MID-BODY UNBIND IS EMITTED -- MOVES NO CALL, for the same
//     reason.  It is written BEFORE phase A (the shipped layout, and the
//     spelling the arm-order table gives for `test ; jne hot ;
//     cold ; jmp end ; hot`).  If icl sinks it instead, positions 1..15
//     permute among themselves, and all fifteen are INDIRECT and
//     therefore the same token.  The alternative spelling is
//         if (!g_n1008ee04) { unbind; }  else { phase A }
//     with a `goto` into the then-arm from three places, which is legal
//     C++ but no clearer and no different in the bytes.
//
//  3. THE ONE THAT COULD MOVE A ROW: WHETHER EACH PHASE'S L_ElseLOD IS
//     EMITTED BEFORE ITS L_MakeRec.  Written ElseLOD-then-MakeRec in all
//     three phases, which is shipped order (0x100478fd < 0x1004792f,
//     0x100479a0 < 0x100479df, 0x10047a46 < 0x10047a7e).  If icl inverts
//     one pair, that phase's tokens read `bd0, bd0, *0xbc` where shipped
//     has `*0xbc, bd0, bd0`, and the LCS charges ONE shipped-only and ONE
//     ours-only row per inverted pair: 23 of 24 for one, 22 for two, 21
//     for all three.  The diagnostic is unmistakable -- the odd rows come
//     in adjacent indirect/0x10039bd0 pairs, and the benign
//     equal-counts pattern would be the right reading, but it still has
//     to be reported as a block-placement question rather than waved
//     through.  Nothing in the source can force it beyond writing them in
//     this order, which is what is done.
//
//  4. THE ALPHA REFERENCE.  Written as the field expression twice
//     (`if (pRec->m_nAlphaRef16) SetRenderState(..., pRec->m_nAlphaRef16)`),
//     which is big729c0.cpp's measured spelling, and which icl commons
//     into the one `movzwl` the shipped bytes have (0x100473fe,
//     0x100475c8, 0x10047783).  The alternative is a named local
//         nAlphaRef = pRec->m_nAlphaRef16;
//         if (nAlphaRef) SetRenderState(g_pD3DDevice, 0x18, nAlphaRef);
//     which is what the DEAD spills to -0x18(%ebp) at 0x10047402 and
//     0x100475cc hint at -- both stores are overwritten before anything
//     reads them, and bind C has no spill at all from the same source.
//     Moves no call.
//
//  5. THE DOUBLED LIST-HEAD TEST.  Each phase tests its head TWICE
//     (0x100472f2 and 0x10047300; 0x100474da and 0x100474e8; 0x100476ac
//     and 0x100476ba), the first jumping straight to L_MakeRec and the
//     second being the `while`'s own entry test.  Written the way
//     big0ae20.cpp and big729c0.cpp write it -- `if (!pHead) goto
//     L_MakeRec;` then `pRec = LTNULL; if (pHead) { pCur = pHead; while
//     (pCur) {...} }` -- which is the form measured there.  Collapsing it
//     to one test emits one `je`.  Moves no call.
//
//  6. PHASE C'S LITERAL STAGE.  Written as the literal 1 in all four
//     places, because the search compare is `cmp $0x1,%ecx` against a
//     narrowed variable in the other two phases, and because there is no
//     `movzbl` in the phase (census: exactly two, one for A and one for
//     B).  A `nStage = 1;` local folds to the identical immediates and
//     the identical absolute `0x1008e688`.  Moves no call.
//
//  7. THE x87 TAIL'S OPERAND ORDER.  Written asymmetrically, matching the
//     shipped `flds 0x1008e664 ; fmuls 0x1008d7c0` / `flds 0x1008d7c0 ;
//     fmuls 0x1008e668` and copying big6d910.cpp's spelling of the same
//     idiom.  Written symmetrically (both `x * g_f1008d7c0`), the `fxch`
//     may go and the two loads may swap.  Moves no call, and the whole
//     region is call-free, so the call check is blind to it -- which is why the
//     0x10003a10 witness is cited inline.
//
//  8. `pHead` LOADED BEFORE THE STAMP STORE.  The shipped bytes load and
//     spill the head first (0x100472ec/0x100472ef) and store the stamp
//     second (0x100472f4); big729c0.cpp writes the stamp first.  Both are
//     call-free scheduling of two independent statements.  Moves no call.
//
//  9. NOT A COIN-FLIP BUT A DECISION WITH A STATED COST: the bind block is
//     written out THREE TIMES rather than factored into a helper.  The
//     shipped bytes have three copies of it (305, 305 and 282 bytes) and a
//     helper icl declined to inline would add three ours-only direct rows
//     and desynchronise the whole sequence -- a far worse failure than any
//     of the flips above.  The same reasoning is why the two unbind blocks
//     are two copies (region 1 and region 11) rather than one label: they
//     have different successors, one falling into phase B and one into the
//     epilogue.
//
// ======================================================================= //
//  WHAT COULD NOT BE PINNED
//
//    * STATUS AS COMPILED (measured): 2,098 bytes of body / 555 instructions
//      against the shipped 2,251 / 604, frame 0x30 against 0x40.  The first
//      divergence is block placement, not a statement: icl hosts
//      L_UnbindAndReturn INLINE right after the `nIndex >= 2` gate (`jb`
//      over it) where the shipped code sends all three gates forward to the
//      end of the body.  Flag sweep, same source: /O2, /O2 /Qip, /O2 /Oi-
//      and /O2 /Qrcd give the identical 2,098 bytes; /Os and /O1 give 1,956
//      and are further off.  /O2 stays.
//
//    * 24 BYTES OF FRAME.  `sub $0x40,%esp` reserves 64.  Twelve go to the
//      three register saves at the bottom ([esp], [esp+4], [esp+8]) and
//      28 to the seven ebp-relative slots at the top (-0x04 .. -0x1c).
//      NOTHING in the 604 instructions touches -0x20 .. -0x34.  It could
//      be alignment, a dead local, or an outgoing area icl reserved and
//      stopped using; the bytes do not say and no member or statement is
//      invented for it.  (big729c0.cpp records the identical 16-byte hole
//      in its own frame.)
//    * Tex47230 +0x00, +0x18..+0x1b, +0x24..+0x2f and +0x34..+0x41 --
//      never touched here; the layout is big729c0.cpp's and
//      big43e10.cpp's.
//    * TexBlock47230 +0x00..+0x0b, +0x10..+0x27 and +0x2c..+0x2f -- ditto.
//      +0x28 IS pinned here (see the type), and it is the one field of
//      this record that no earlier file names.
//    * CD3DModelDraw::m_pModel00 is declared for layout only; this
//      function never reads it.  The padding runs +0x008..+0x02f,
//      +0x03c..+0x303, +0x310..+0x427 and +0x42c..+0x6a3 are exactly that
//      -- padding -- and big17c14.cpp and big43e10.cpp already disagree
//      with each other about what lives inside two of them.
//    * WHAT m_n304, m_n308, m_n030 and m_n428 MEAN.  Their ROLES here are
//      read off the bytes (a hard gate, a second gate, the detail-texture
//      enable, and a stage count that big43e10.cpp sets to 1 or 2), but
//      their names are not recoverable and are not invented.  m_n308 is
//      written by 0x10047b60 (`movl $0x0,0x308(%ebx)` at 0x10047b7b then
//      `movl $0x1,...` at 0x10047b98), which is a second witness for the
//      gate but still not a name.
//    * THE D3D CONSTANT NAMES.  What is pinned is the NUMBERS: render
//      state 0x18, texture stage 1, stage state 1, values 0 and 1, and
//      SetLOD's argument.  ALPHAREF / COLOROP / D3DTOP_DISABLE are the
//      standard D3D7 assignments for those numbers and are a reading, not
//      a measurement.
//    * THE MEANING OF THE ARGUMENT.  It is a uint32 bounded below 2 and
//      it indexes m_apTex1b8, and the caller takes it from a pass
//      record's +0x24 (0x10008c05).  What that record is lives outside
//      these bytes.
//    * "HALF-TEXEL OFFSETS" for 0x1008ac90 / 0x1008ac94 is a READING, from
//      their three readers (0x10003a10, 0x10003a40, 0x10003a80 each ADD
//      them to a uv pair) and from big331e0.cpp's finding that
//      g_f1008d7c0 is 0.5f.  What is MEASURED is that they are floats,
//      that they are written only by this function and 0x10047b60, and
//      that the values written are invW*g_f1008d7c0 and g_f1008d7c0*invH
//      of scale-table element ZERO.
//
// ======================================================================= //
//  THE INSTRUCTION CENSUS -- 602 body instructions, 23 distinct mnemonics,
//  NO RESIDUE IN ANY DISCRIMINATING ROW.  It is wanted where the
//  call check leaves bytes unwitnessed, and it matters more here than in
//  big729c0.cpp because only 6 of the 24 calls are checked by target.  The
//  function is THREE near-identical binds plus THREE unbinds, so almost
//  every row is a multiple of three that the claimed structure has to
//  predict -- and a fourth bind, a missing bind, a fourth unbind or one
//  more search unroll would move at least three rows at once.
//
//      mov    330    je      50    test   39    movzwl 34
//      add     33    call    24    movl   19    cmp    17
//      jmp     16    lea      8    xor     5    movzbl  5
//      jne      4    inc      3    ret     2    pop     2
//      nop      2    jae      2    fstps   2    fmuls   2
//      flds     2    push     1    sub     1
//                                        ---------------
//                                        602  ( + 2 in the 5 bytes of
//                                        alignment padding = the tool's
//                                        604 for the 2,256-byte row)
//
//  SEVEN x87 INSTRUCTIONS, zero `fistp`, zero `fdiv`, zero `rep movs`,
//  zero `idiv`, zero `.rdata` operands.  That is the whole basis for
//  declining /Oi- and /Qrcd above.
//
//  THE TWELVE DISCRIMINATING ROWS, decomposed by operand.  Each one is
//  exact:
//
//  call 24 -- 6 x `0x10039bd0` (three phases x two create arms) + 12 x a
//        DEVICE slot + 6 x the SURFACE slot.  And the device/surface split
//        is independently confirmed by `mov 0x1008d988,%reg`, which occurs
//        EXACTLY TWELVE times -- once per device call and never for a
//        SetLOD, which takes its vtable from the record's +0x0c instead.
//
//  add 33 -- AND THIS ROW ALONE CONFIRMS EVERY CALL'S ARGUMENT COUNT:
//        9  `add $0xfffffff4,%esp`  the nine 12-byte outgoing areas:
//                                   SetTexture x6 + SetRenderState x3
//        9  `add $0xfffffff0,%esp`  the nine 16-byte areas: 0x10039bd0 x6
//                                   + GetTextureStageState + the two
//                                   SetTextureStageState
//        6  `add $0xfffffff8,%esp`  the six 8-byte areas: SetLOD x6
//        6  `add $0x10,%esp`        the six __cdecl cleanups after
//                                   0x10039bd0 -- and there is no other
//                                   `add $N,%esp`, which is what says the
//                                   other eighteen calls are __stdcall
//        3  `add %reg,0x48(%eax)`   the three `m_nBytes48 +=`, one per bind
//
//  THE FRAME/ARGUMENT CENSUS, which is the same statement from the other
//  side and has no residue either:
//        (%esp)      27 = 24 first arguments + 1 prologue `%ebx` store +
//                         2 epilogue `%ebx` loads
//        0x4(%esp)   27 = 24 second arguments + 1 + 2 (`%esi`)
//        0x8(%esp)   21 = 18 third arguments (24 minus the six SetLOD,
//                         which take two) + 1 + 2 (`%edi`)
//        0xc(%esp)    3 = the ONE Get plus the TWO Sets.  The six
//                         0x10039bd0 sites NEVER write it, which is
//                         big729c0.cpp's finding that the fourth dword is
//                         icl's eight-byte-rounding padding and not an
//                         argument.
//        (%ebp)+      1 = `0x8(%ebp)`, the single stack argument
//        (%ebp)-     69 across exactly seven slots, listed in the frame
//                         banner
//
//  movzwl 34 -- every one a 16-bit field load:
//       13  `0x42(...)`   m_nStage42.  FOUR search compares (phase A's two
//                         unrolled copies + phase B + phase C) plus THREE
//                         per bind x 3 binds (the g_a1008e684 index, the
//                         SetTexture stage, the g_a1008e664 index) = 4 + 9
//        6  `0x44(...)`   m_nLOD44: the three main binds + the three sunk
//                         already-bound arms
//        6  `0x1008bea8`  TWO per bind (the stamp compare and the stamp
//                         store) x 3
//        3  `0x16(%eax)`  m_nAlphaRef16, one per bind
//        3  `0x14(...)`   m_nStamp14, one per bind
//        3  `0x1008b870`  the block stamp, one per PHASE
//
//  16-BIT STORES 12, the mirror of the above: 3 x `m_nStamp14`, 3 x
//        `m_nStamp30` (one per phase) and 6 x `m_nLOD44` (one per SetLOD
//        site).  A fourth bind or a fourth SetLOD moves this row.
//
//  movzbl 5 -- 3 x `%al,%eax` (the SetTexture stage cast, one per bind) +
//        `%dl,%edx` (phase A's `(uint8)nStage`) + `%cl,%edx` (phase B's).
//        PHASE C HAS NONE, which is the census proof that its stage is a
//        literal.
//
//  movl 19 -- every immediate-to-memory store:
//        6  `$0x0,0x8(%esp)`   SetTexture's NULL third argument x3 (the
//                              three unbinds) + 0x10039bd0's bLink = 0 x3
//                              (the three empty-list arms)
//        3  `$0x18,0x4(%esp)`  the three SetRenderState state numbers
//        2  `$0x1,0x8(%esp)`   0x10039bd0's bLink = 1 in phases A and B
//                              (phase C's non-empty arm uses the register
//                              that already holds 1)
//        2  `$0x1,0x4(%esp)`   phase C's empty arm's stage, and
//                              L_Disable's SetTexture stage
//        2  `$0x0,0x4(%esp)`   SetLOD's literal 0 in bind C and L_ElseLOD3
//        1  `$0x1008edf0,0xc(%esp)`  THE ONLY OUT-PARAMETER ADDRESS in the
//                              function -- and it is a global's, not a
//                              frame slot's, which is why there is no
//                              `lea -K(%ebp)` beside it
//        3  `$0x0,<bound table>`     the three g_a1008e684 clears: two
//                              indexed (`(,%ebx,4)`, `(,%eax,4)`) and one
//                              ABSOLUTE at 0x1008e688, i.e. index 1
//
//  cmp 17 -- broken out by operand, and every one placed:
//        3  the STAMP compares, one per bind: `%edx,%eax` at 0x10047375
//           and 0x1004753f, `%ecx,%edx` at 0x10047707
//        4  the SEARCH compares: `%edx,%ecx` at 0x10047314 and 0x10047327
//           (phase A's two unrolled copies), `%edx,%edi` at 0x100474f7
//           (phase B), and `$0x1,%ecx` at 0x100476c4 -- phase C's, against
//           an IMMEDIATE, which is the row that says its stage is a literal
//        3  the ALREADY-BOUND tests: `0x1008e684(,%eax,4),%edx` at
//           0x1004734c (A), `0x1008e684(,%ecx,4),%edx` at 0x10047516 (B),
//           `0x1008e688,%ecx` at 0x100476e0 (C -- absolute, index 1 again)
//        5  the LOD compares in their VARIABLE form: `%edx,%eax` at
//           0x1004745e (bind A), 0x10047628 (bind B) and 0x10047a4f
//           (L_ElseLOD1), and `%ecx,%eax` at 0x100479a7 (L_ElseLOD2).
//           That is FOUR, and the fifth `%edx,%eax` is 0x10047375 /
//           0x1004753f already counted above -- so the operand-form total
//           is 5 x `%edx,%eax` = 2 stamp + 3 LOD, exactly as objdump
//           reports.  The other TWO LOD sites (bind C at 0x100477d7 and
//           L_ElseLOD3 at 0x10047901) use `test %eax,%eax` instead,
//           because their constant is 0 -- which is why this row is 4
//           variable-form compares and not 6
//        3  the three remaining immediates: `$0xffffffff,%ecx` at
//           0x100474b3 (the m_n038 sentinel), `$0x2,%esi` at 0x10047262
//           (`nIndex >= 2`) and `$0x2,%eax` at 0x1004780c (`m_n428 < 2`)
//        Total 3 + 4 + 3 + 4 + 3 = 17, nothing left over.
//
//  lea 8 -- 6 x `0x1c(...)`, i.e. TWO per bind (the two link addresses
//        dl_TieOff and dl_Insert need) x 3 binds, + 2 alignment pads
//        (`0x0(%esi,%eiz,1),%esi` at 0x1004730a and `0x0(%esi),%esi`).
//        THERE IS NO `lea -K(%ebp)` AT ALL: no local's address is taken
//        anywhere in this function, which is what says the Get's
//        out-parameter really is the global.
//
//  xor 5 -- 3 x `pRec = LTNULL`, one per phase (`%eax,%eax` in A,
//        `%edx,%edx` in B, `%ecx,%ecx` in C) + 2 x `m_nLOD44 = 0`, in bind
//        C and L_ElseLOD3.  The four variable-form LOD stores do NOT need
//        a zero, which is why this row is 5 and not 9.
//
//  inc 3 -- `inc %edx` for `g_n1008bd80++`, one per bind, and nothing
//        else; the three `mov %edx,0x1008bd80` stores are its mirror.
//
//  test 39 -- ALL THIRTY-NINE placed individually, in address order.
//        The register split is the arithmetic check: 28 x `%eax,%eax`,
//        5 x `%edx,%edx`, 4 x `%edi,%edi`, 2 x `%ecx,%ecx`.
//
//        the four top gates and the first unbind
//          10047249 m_n304          10047257 m_n308
//          10047275 pBlock  (%edi)  10047282 g_n1008ee04
//          10047295 g_a1008e684[g_n1008d7d0]  (%edx)
//        phase A -- SEVEN, which is the shape of one search
//          100472db pBlock AGAIN (%edi)   100472f2 pHead (%edx)
//          10047300 the `while` entry     1004731f back edge, copy 1
//          10047332 back edge, copy 2     1004733e pRec
//          10047405 m_nAlphaRef16
//        phase B -- SIX: the same seven minus one back edge, because the
//        loop is not unrolled
//          100474c2 pBlock (%edi)         100474da pHead (%edx)
//          100474e8 the `while` entry     10047505 the back edge
//          1004750e pRec (%edx)           100475cf m_nAlphaRef16
//        phase C -- EIGHT: B's six, plus the m_n030 gate and the SECOND
//        test of +0x28, and plus the zero-form LOD test bind C has where
//        A and B have a `cmp`
//          10047657 m_n030                10047662 +0x28, first test
//          10047697 pDetail (%edx)        100476ac pHead (%ecx)
//          100476ba the `while` entry     100476d0 the back edge
//          100476d8 pRec (%ecx)           10047787 m_nAlphaRef16
//          100477d7 m_nLOD44 == 0
//        L_Disable and the second unbind
//          10047867 g_a1008e684[1]        100478ad g_a1008e684[stage]
//        the sunk tail -- TEN, and it is the row that proves there are
//        three create blocks of two arms each
//          10047901 m_nLOD44 == 0 (L_ElseLOD3)
//          10047932 pHead   10047951 result   1004798c result (%edi)
//          100479e2 pHead   10047a00 result   10047a39 result
//          10047a81 pHead   10047aa2 result   10047ade result
//
//        5 + 7 + 6 + 9 + 2 + 10 = 39, nothing left over.  A fourth phase
//        would add six or more; a phase with no create block would remove
//        three.
//
//  jae 2 / ret 2 / pop 2 / push 1 / sub 1 -- the two unsigned bounds
//        checks; the two duplicated epilogues, which is why `pop` is 2 and
//        `push` is 1.
//
//  flds 2 / fmuls 2 / fstps 2 / fxch 1 -- the whole of region 4 and the
//        whole of the function's floating point.
//
//  je 50, by target -- this row IS the branch graph, and it is what says
//        there is no fourth phase and no fifth path:
//        6  0x10047898  L_Done              6  0x10047654  L_PhaseC
//        3  0x10047286  L_UnbindCur         2  0x10047a7e  L_MakeRec1
//        2  0x100479df  L_MakeRec2          2  0x1004792f  L_MakeRec3
//        2  0x100478a0  L_UnbindAndReturn   2  0x10047806  L_Disable
//        2  0x1004748a  L_ScaleA            2  0x10047336  A's loop exit
//        1 each: 0x10047af3, 0x10047aeb (A's two trampolines),
//        0x10047abe, 0x10047a1c, 0x1004796c (the three empty-list arms),
//        0x10047a46, 0x100479a0, 0x100478fd (the three L_ElseLOD),
//        0x100479d8, 0x10047999 (B's and C's trampolines),
//        0x100478ec (the early return's own epilogue),
//        0x100478db (L_UnbindAndReturn's fall-through to the shared one),
//        0x100477a8, 0x100475f0, 0x10047426 (the three alpha skips),
//        0x10047757, 0x10047599, 0x100473cf (the three stamp skips),
//        0x10047509, 0x100476d4 (B's and C's loop exits),
//        0x100474b0 (L_PhaseB, from the mid-unbind's table test)
//        = 29 + 21 = 50, with nothing left over.
//
//  jmp 16 -- 2 each to L_Done, L_Bind3, L_Bind2, L_Bind1 and A's join
//        0x1004733b (the two create arms of each phase, and A's two
//        trampolines), plus one each to the shared epilogue, C's join
//        0x100476d6, L_PhaseC, B's join 0x1004750b, L_PhaseB and
//        L_ScaleA.
//
//  WHAT WOULD MOVE THESE ROWS.  A fourth bind moves eight rows at once
//  (movzwl x4, the 16-bit stores, add x2, movl, xor, inc, call, and the
//  three esp rows).  A fourth unbind moves `movl $0x0,0x8(%esp)`,
//  `movl $0x0,<table>` and `add $0xfffffff4,%esp`.  One more or one fewer
//  unroll of phase A's search moves `movzwl 0x42`, `cmp` and `je`.  A
//  fourth create arm moves `add $0x10,%esp` and `add $0xfffffff0,%esp`.
//  A single extra literal argument anywhere moves `movl`.  A local whose
//  address was taken would put a `lea -K(%ebp)` in a row that has none.
//  Nothing in the reconstruction above is free of at least one of them.
//
// ======================================================================= //
