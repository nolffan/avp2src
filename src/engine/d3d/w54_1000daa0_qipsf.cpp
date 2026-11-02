// *********************************************************************** //
//
//  w54_1000daa0_qipsf.cpp -- bige0e0.cpp (0x1000e0e0, the particle-system
//  draw) and w2_1000daa0.cpp (0x1000daa0, the batch it hands the
//  particles to) as ONE /O2 /Qip /Qsfalign16 translation unit.
//
//  0x1000daa0 has no stack entry: it opens `push %ebp ; mov %esp,%ebp ;
//  push %ecx ; push %edx ; push %eax ; mov $0x4150,%eax ; call _chkstk`,
//  its three register arguments saved across _chkstk, and both calls at
//  0x1000ece3 / 0x1000ed1f load %eax/%edx/%ecx -- a register-only /Qip
//  static of its caller's TU, so it is `static` here.  /Qsfalign16
//  because it holds doubles and would otherwise get icl's realigning
//  entry, which the shipped code lacks.  The two files' Obje0e0 are one
//  object (w2's colour bytes +0x94..+0x97 sit in bige0e0's padding) and
//  their device views one interface (DrawIndexedPrimitive +0x68 added to
//  bige0e0's).  Measured (difflib over normalised instructions):
//      0x1000daa0  0.496 (own file, /O2)  ->  0.537, attempted -> near
//      0x1000e0e0  0.541 (own file, /O2)  ->  0.557
//  The two files' own headers follow.
//
// *********************************************************************** //

// *********************************************************************** //
//
//  MODULE:   BIG_E0E0 (.CPP)
//
//  PURPOSE:  One function -- 0x1000e0e0.  d3d.ren's PARTICLE-SYSTEM DRAW:
//            bind the system's texture to the current stage, publish the
//            sprite-quad UV extents and the 1/255 colour scales into the
//            0x1008adb0 global block, build the system's world transform
//            from its quaternion / scale / position, compose it with the
//            view matrix, look the ambient colour up at the system's
//            origin, and hand the particles to 0x1000daa0 in batches of
//            128.
//
//            Reconstructed to the "structurally decoded" standard.
//            Byte identity is NOT claimed.
//
//  ------------------------------------------------------------------- //
//  THE EXTENT.  THE ROW IS WRONG.  Verified before a byte was decoded.
//  ------------------------------------------------------------------- //
//
//  The row claims 0x1000e0e0..0x1000eeb0, 3,536 bytes.  The function
//  really ends at 0x1000eea7 and is **3,527 bytes** (0xdc7).  The row's
//  last nine bytes are alignment padding plus a SEPARATE one-byte
//  function.
//
//    * TWO `ret` in the row, at 0x1000ed35 and at 0x1000eeb0.
//    * ONE `push %ebp ; mov %esp,%ebp`, at 0x1000e0e0.  There is no
//      second prologue anywhere in the row.
//    * The `ret` at 0x1000ed35 is THIS function's only epilogue
//      (0x1000ed27 `mov (%esp),%ebx` / `mov 0x4(%esp),%esi` /
//      `mov 0x8(%esp),%edi` / `mov %ebp,%esp` / `pop %ebp`).  The 370
//      bytes after it, 0x1000ed36..0x1000eea6, are /O2 cold-block
//      sinking: nine blocks, each of which uses THIS frame and ends by
//      branching BACK above the epilogue --
//          0x1000ed36 -> 0x1000e38f / 0x1000e396   (quaternion w clamp)
//          0x1000ed4d -> 0x1000e371 / 0x1000e378   (quaternion z clamp)
//          0x1000ed64 -> 0x1000e353 / 0x1000e35a   (quaternion y clamp)
//          0x1000ed7b -> 0x1000e336 / 0x1000e33c   (quaternion x clamp)
//          0x1000ed92 -> 0x1000e272                (L_ElseLOD)
//          0x1000edc8 -> 0x1000e173 / 0x1000ee01 / 0x1000ee67 (L_MakeRec)
//          0x1000ee01 -> 0x1000e30e                (L_NoTexture)
//          0x1000ee67 -> 0x1000e173 / 0x1000ee01   (L_MakeRec, empty list)
//          0x1000ee93, 0x1000ee9d -> 0x1000e159    (the two unrolled
//                                                   search `break`s)
//    * THE LAST INSTRUCTION IN THE FUNCTION IS 0x1000eea2
//      `jmp 0x1000e159`, which ends at 0x1000eea7.  0x1000eea7 is
//      `mov %esi,%esi` and 0x1000eea9 is `lea 0x0(%edi,%eiz,1),%edi`:
//      nine bytes of the two-instruction lea-nop pad that aligns the next
//      entry to 0x1000eeb0.
//    * 0x1000eeb0 is `c3`, a ONE-BYTE FUNCTION -- an empty virtual.  It
//      is padded to 0x1000eec0 (`nop` + two lea-nops) and its address is
//      stored in .data TWICE, at 0x10089020 and 0x10089190, which are
//      vtable slots.  0x1000eec0 is the next real function
//      (`push %ebx ; sub $0x10,%esp`, a __thiscall three-dword setter).
//    * EVERY branch target in 0x1000e0e0..0x1000eea7 lands INSIDE that
//      range and on an instruction boundary.  Checked mechanically over
//      all 40 jump/branch operands and all 5 direct-call operands: zero
//      outside, zero mid-instruction.
//
//  CORRECTED EXTENT:      0x1000e0e0 .. 0x1000eea7   (3,527 bytes)
//  PAD:                   0x1000eea7 .. 0x1000eeb0   (9 bytes)
//  SPLIT OFF, NOT DECODED HERE:
//      0x1000eeb0, 1 byte, 0 calls -- a bare `ret`.  An empty virtual
//      function; two vtable references in .data (0x10089020, 0x10089190).
//      Padded to 0x1000eec0.
//
//  ------------------------------------------------------------------- //
//  THE CALL-GRAPH CHECK IS WEAK HERE, AND THAT IS SAID UP FRONT.
//  ------------------------------------------------------------------- //
//
//    0x1000e0e0..0x1000eea7:
//      3,527 bytes, 13 calls (5 direct, 8 indirect), 271 bytes per call
//
//  271 B/call is inside the weak band and there are only THREE
//  distinct direct callees (0x1000daa0, 0x10039bd0, 0x10073400).  Worse,
//  0x1000e30e..0x1000ec58 is 2,379 bytes holding exactly ONE call, the
//  `call 0x10073400` at 0x1000e3a6, and inside it 0x1000e45a..0x1000ec58
//  -- 2,047 bytes, 58% of the function -- has no call of any kind, direct
//  or indirect.  So the sequence is a sanity check, not the evidence.  The
//  evidence is the other three instruments, and all three are
//  supplied below: the INSTRUCTION CENSUS (no residue), the FRAME
//  ARITHMETIC (exact), and EXPRESSION-BY-EXPRESSION CORRESPONDENCE with
//  four already-decoded siblings.
//
//  ------------------------------------------------------------------- //
//  INSTRUCTION CENSUS -- 874 instructions, NO RESIDUE
//  ------------------------------------------------------------------- //
//
//  | opcode  | count | decomposition                                    |
//  |---------|-------|--------------------------------------------------|
//  | fmuls   |  109  | of the 141 total multiplies below                |
//  | fmul    |   22  |   "                                              |
//  | fmull   |    6  |   "                                              |
//  | fmulp   |    4  |   "                                              |
//  | ALL MUL |  141  | = 2 x 64 (the two inlined 4x4 MatMul4 arms)       |
//  |         |       | + 9 (the scale multiply, 3 columns x 3 rows)      |
//  |         |       | + 4 (2 x `* m_fInvW00`, 2 x `* m_fInvH04`).      |
//  |         |       | 128+9+4 = 141 EXACTLY.                           |
//  | faddp   |   94  | of the 96 total adds                             |
//  | faddl   |    2  |   "                                              |
//  | ALL ADD |   96  | = 2 x 48.  A 4x4 multiply is 16 elements x 3      |
//  |         |       | adds = 48; two arms = 96, and NOTHING else in     |
//  |         |       | the function adds a float.  EXACTLY.             |
//  | fstps   |   45  | = 32 (2 arms x 16 matrix stores)                  |
//  |         |       | +  9 (the scale multiply)                        |
//  |         |       | +  4 (0x1008adb4/b8/bc/c0).  EXACTLY.            |
//  | flds    |  149  | = 28 (g_Mat1008bb5c, element by element)          |
//  |         |       | + 28 (g_Mat1008b9dc -- the SAME 28, which is why  |
//  |         |       |       the two arms are one source construct)      |
//  |         |       | + 72 (mLocal's slots inside the two arms)         |
//  |         |       | +  9 (the scale multiply)                        |
//  |         |       | +  8 (4 quaternion components + 4 x 100000.0f)    |
//  |         |       | +  2 (the two `2.0f` that are NOT subtracted)     |
//  |         |       | +  2 (the two 0.0f passed to 0x1000daa0).        |
//  |         |       | 28+28+72+9+8+2+2 = 149 EXACTLY.                  |
//  | fsubs   |    2  | the two `- 2.0f` (width and height).              |
//  | fcomp   |    4  | the four `> 100000.0f`.                           |
//  | fcomps  |    4  | the four `< -100000.0f`.                          |
//  | fnstsw  |    8  | one per compare.  8 = 4 + 4, no other compare.    |
//  | sahf    |    8  |   "                                              |
//  | fildll  |    2  | the unsigned width and the unsigned height.       |
//  | fldl    |   12  | 4 quaternion reloads + 8 x87 spill reloads.       |
//  | fstpl   |   12  | 4 quaternion spills   + 8 x87 spill stores.       |
//  | fxch    |   96  | scheduling only.  No source construct.            |
//  | fld     |    2  | `fld %st(5)`, scheduling only.                    |
//  | movzwl  |   12  | g_w1008b870 + 2 (the unrolled m_nStage42 search)   |
//  |         |       | + 3 more m_nStage42 + m_nStamp14 + m_nAlphaRef16  |
//  |         |       | + 2 x g_w1008bea8 + 2 x m_nDirty44.  EXACTLY.    |
//  | lea     |   10  | 2 x `&mFinal` (the two 0x1000daa0 calls)          |
//  |         |       | + 2 x `&pRec->m_Link1c` (dl_TieOff/dl_Insert)     |
//  |         |       | + `&m_qRot9c` + `&m_vScaleac`                     |
//  |         |       | + 2 x `&m_vPos18c` (the matrix and the pfn)       |
//  |         |       | + `&mLocal` + ONE lea-nop pad (0x1000ecbc).      |
//  | call    |   13  | 3 x `*%eax` (g_pfn1008ce8c, GetWidth0c,           |
//  |         |       |   GetHeight10) + 2 x SetLOD (+0xbc) + 2 x         |
//  |         |       |   0x1000daa0 + 2 x 0x10039bd0 + 2 x SetTexture    |
//  |         |       |   (+0x8c) + 1 x SetRenderState (+0x50) + 1 x      |
//  |         |       |   0x10073400.  EXACTLY.                          |
//  | inc     |    2  | `g_n1008bd80++` and the block loop's `i++`.       |
//  |         |       | There is NO frame-counter bump in this function.  |
//  | sar/shr |    3  | the signed `/128`.                                |
//  | shl     |    1  | the `% 128` (count - (count/128)*128).            |
//  | idiv    |    0  | -- which is why the batch size is a power of two. |
//
//  A fifth matrix, a third MatMul, a missing scale axis, a fifth
//  quaternion component or a second SetRenderState would each move at
//  least three of those rows.
//
//  ------------------------------------------------------------------- //
//  FRAME ARITHMETIC -- `sub $0x130,%esp` = 304, and it decomposes exactly
//  ------------------------------------------------------------------- //
//
//      -0x108 .. -0xc9   0x40   LTMat mLocal   (16 dwords, all 16 seen)
//      -0xc8  .. -0x89   0x40   LTMat mFinal   (16 dwords, each written
//                               EXACTLY TWICE -- once per MatMul arm)
//      -0x88  .. -0x81   0x08   the int64 the two `fildll` read
//      -0x80  .. -0x75   0x0c   never referenced
//      -0x74, -0x70, -0x6c      three int spill slots
//      -0x68                    one more int slot (a dead `(uint8)nStage`
//                               spill at 0x1000e12e, read nowhere)
//      -0x64                    pad to the 8-byte double alignment
//      -0x60 .. -0x09    0x58   ELEVEN 8-byte slots, -0x60 -0x58 -0x50
//                               -0x48 -0x40 -0x38 -0x30 -0x28 -0x20
//                               -0x18 -0x10
//      -0x08  .. -0x01   0x08   the twelfth 8-byte slot
//
//    locals            0x108 = 264
//    saved ebx/esi/edi   0xc =  12   (at the frame BASE, (%esp)/4/8)
//    max outgoing block 0x18 =  24   (`add $0xffffffe8,%esp`, the two
//                                     0x1000daa0 calls)
//                             = 300, rounded up to 16 = 304 = 0x130.
//
//  Of the twelve 8-byte slots, FOUR are source-visible: -0x28 -0x30
//  -0x38 -0x40 hold the four quaternion components across the two halves
//  of each `||`.  The other eight are x87-stack spills inside the two
//  MatMul arms, four per arm (read back by the 8 `fldl` + 6 `fmull` +
//  2 `faddl`).  The 0x40 / 0x40 pair is the load-bearing part: it says
//  there are exactly TWO 4x4 matrices on this frame and no third.
//
//  ------------------------------------------------------------------- //
//  EXPRESSION-BY-EXPRESSION CORRESPONDENCE WITH DECODED SIBLINGS
//  ------------------------------------------------------------------- //
//
//    * 0x1000e0f4..0x1000e272 and the sunk 0x1000ed92 / 0x1000edc8 /
//      0x1000ee67 are big43e10.cpp's texture-bind block, compared
//      instruction by instruction, not skimmed.  Same block stamp at
//      +0x30, same unrolled-by-two m_nStage42 search with both `break`s
//      sunk, same `pRec == g_a1008e684[nStage]` early-out, same
//      dl_Remove/dl_TieOff/dl_Insert through g_p1008e634, same
//      `g_p1008cbcc->+0x48 += pRec->m_nBytes10`, same SetTexture with the
//      stage TRUNCATED TO A BYTE, same ALPHAREF guard, same
//      counter-before-the-pair order (`mov 0x1008bd80,%edx ; inc %edx ;
//      mov %edx,0x1008bd80` at 0x1000e22a/e233/e237 AHEAD of the two
//      stores at 0x1000e23d/0x1000e244 -- big43e10 records exactly this
//      and contrasts it with big2ef34/big6d910's `incl` after), same
//      duplicated SetLOD tail, same two-arm 0x10039bd0 create.
//      THE ONE DIFFERENCE: where big43e10's create-failure goes to its
//      shared L_Done, ours goes to the NO-TEXTURE arm (0x1000edef and
//      0x1000ee88 both `je 0x1000ee01`).
//    * 0x1000e272..0x1000e30e is big61a30.cpp's quad-extent block
//      (0x10061e7c) with the four results written to globals instead of
//      locals: same `GetWidth0c()` / `GetHeight10()` vtable slots +0x0c
//      and +0x10, same `fildll` of a zero-extended dword, same
//      `(w - 2.0f) * m_fInvW00` and `m_fInvW00 * 2.0f` pair.
//    * the `pObj->m_nFlags88 & 0x40 ? g_Mat1008bb5c : g_Mat1008b9dc`
//      view choice is big17c14.cpp's, at 0x10018233, verbatim -- same
//      flag, same bit, same two matrices, same arm order.
//    * `(*g_pfn1008ce8c)(&pObj->m_vPos18c, &g_n1008adb0)` is
//      big17c14.cpp's 0x10017d29 and big0f6f0.cpp's 0x1000f85e: both
//      arguments in registers, %ecx then %edx, nothing pushed.
//    * the two 4x4 multiplies are big48180.cpp's `MatMul4`, reused
//      VERBATIM below.  Its store order (column by column, [0][0] [1][0]
//      [2][0] [3][0] [0][1] ...) and its last-term-first accumulation
//      (M1[i][3]*M2[3][j] emitted before M1[i][0]*M2[0][j]) are both
//      reproduced here with no exception in either arm.
//
//  ------------------------------------------------------------------- //
//  THE SIGNATURE, AND WHERE IT WAS READ
//  ------------------------------------------------------------------- //
//
//  ONE call site: 0x10050b41, inside 0x10050900.
//
//      10050b3f  push %edi
//      10050b40  push %ebx          <- 0x8(%ebp) here
//      10050b41  call 0x1000e0e0
//      10050b46  push %edi
//
//  `mov 0xc(%ebp),%ebx` at 0x1005090a makes %ebx that caller's second
//  argument, and the object it reaches has +0x88 flags, +0xac..+0xb4
//  floats and +0x208..+0x214 -- the same object this function reads.
//  The function is __cdecl (plain `ret`) and returns nothing (no `xor
//  %eax,%eax` or `mov ...,%eax` reaches the epilogue at 0x1000ed32).
//
//  COIN-FLIP: the call site pushes TWO
//  dwords and this function reads only 0x8(%ebp) -- 0xc(%ebp) is never
//  touched (verified by a mechanical sweep of every `(%ebp)` operand in
//  the range: the only non-negative one is `0x8`).  So it is either
//
//      void sub_1000e0e0(Obje0e0* pObj, uint32 nUnused0c);   <- written
//      void sub_1000e0e0(Obje0e0* pObj);
//
//  and the stack arithmetic cannot separate them: that caller uses
//  `push <reg>` as a 4-byte esp reservation elsewhere (`push %edi ;
//  push %edi` at 0x10050906, `push %eax ; push %esp` at 0x10050ae0), and
//  under either reading exactly 12 bytes are outstanding when the
//  __stdcall COM call at 0x10050b60 pops them.  The two-parameter form is
//  written because two pushes is the literal reading; %edi there is the
//  D3DRS_DESTBLEND value the caller installed at 0x10050ab8.
//
//  ------------------------------------------------------------------- //
//  COMPILER VERDICT: Intel C/C++ 4.0, /O2.
//  ------------------------------------------------------------------- //
//    * COLD-BLOCK SINKING BELOW THE EPILOGUE, nine-fold (enumerated
//      under THE EXTENT).
//    * OUTGOING ARGUMENTS as `add $-N,%esp` then `mov %reg,K(%esp)` at
//      0x1000e1dc, 0x1000e209, 0x1000e253, 0x1000e396, 0x1000ecc0,
//      0x1000ecfc, 0x1000ed9e, 0x1000edd0, 0x1000ee32 and 0x1000ee67.
//      NOT ONE `push` in the whole function except the prologue's.
//    * the __stdcall COM calls leave esp low and the next `add $-N,%esp`
//      re-lowers it; only the __cdecl calls (0x10073400, 0x10039bd0,
//      0x1000daa0) are followed by an `add $N,%esp`.
//    * the /O2 x87 scheduler: 96 `fxch` and 8 double spill slots.
//
// *********************************************************************** //

#include "ltbasedefs.h"


// ======================================================================= //
//  TYPES.  Every offset below is cited against the instruction that
//  reaches it.  Nothing here is inferred from a header.
// ======================================================================= //

struct Vece0e0
{
	float	x;                                          // +0x00
	float	y;                                          // +0x04
	float	z;                                          // +0x08
};

// 0x1000e31a / 0x1000e33c / 0x1000e35a / 0x1000e378 read +0x00 +0x04
// +0x08 +0x0c as floats and 0x1000e3a6 hands the base to 0x10073400,
// which d3dr2a.cpp/d3dr3b.cpp name (rotation, matrix).  Four floats.
struct Quate0e0
{
	float	x;                                          // +0x00
	float	y;                                          // +0x04
	float	z;                                          // +0x08
	float	w;                                          // +0x0c
};

// The 4x4.  Row-major with the translation in COLUMN 3: 0x1000e440
// stores the position into +0x0c, +0x1c and +0x2c (= m[0][3], m[1][3],
// m[2][3]), and 0x10073400 -- which fills this same buffer -- writes
// 1.0f to +0x3c and zero to +0x2c/+0x30/+0x34/+0x38 (0x100734d3 ..
// 0x100734e3), i.e. a (0,0,0,1) bottom row.
struct Mate0e0
{
	float	m[4][4];
};

// ltlink.h's CheapLTLink as the texture record's +0x1c member.  The
// dl_Remove / dl_TieOff / dl_Insert run at 0x1000e18f..0x1000e1c1 reads
// +0x00 as prev and +0x04 as next.
struct Linke0e0
{
	Linke0e0*	m_pPrev;                                // +0x00
	Linke0e0*	m_pNext;                                // +0x04
	void*		m_pData;                                // +0x08
};

struct DDSurfe0e0;

struct DDSurfe0e0Vtbl
{
	uint8	m_pad_00[0xbc];
	long (__stdcall* SetLOD)(DDSurfe0e0*, uint32 dwLOD);         // +0xbc
};

struct DDSurfe0e0
{
	DDSurfe0e0Vtbl*	lpVtbl;
};

// The per-stage texture record.  Field for field big43e10.cpp's
// Tex440e3, big2ef34.cpp's Tex2ef34 and big61a30.cpp's Tex61a30; the
// two vtable slots are big61a30.cpp's, called here at 0x1000e29a and
// 0x1000e2e4 with %ecx = this and nothing pushed, both answers fed to a
// `fildll` of a dword zero-extended to 64 bits -- so both are UNSIGNED.
class Texe0e0
{
public:
	virtual void	Slot00();
	virtual void	Slot04();
	virtual void	Slot08();
	virtual uint32	GetWidth0c();                       // vtable +0x0c
	virtual uint32	GetHeight10();                      // vtable +0x10

	float		m_fInvW04;                              // +0x04  0x1000e230
	float		m_fInvH08;                              // +0x08  0x1000e234
	DDSurfe0e0*	m_pSurface0c;                           // +0x0c  0x1000e1ef
	uint32		m_nBytes10;                             // +0x10  0x1000e1c7
	uint16		m_nStamp14;                             // +0x14  0x1000e187
	uint16		m_nAlphaRef16;                          // +0x16  0x1000e1fe
	uint8		m_pad_18[0x1c - 0x18];
	Linke0e0	m_Link1c;                               // +0x1c  0x1000e18f
	uint8		m_pad_28[0x30 - 0x28];
	Texe0e0*	m_pNext30;                              // +0x30  0x1000e13d
	uint8		m_pad_34[0x42 - 0x34];
	uint16		m_nStage42;                             // +0x42  0x1000e131
	uint16		m_nDirty44;                             // +0x44  0x1000e24b
};

// The block reached through pObj->m_pBlock1e8: a list head at +0x0c
// (0x1000e112) and a sixteen-bit stamp at +0x30 (`mov %ax,0x30(%ecx)`
// at 0x1000e11a).
struct TexBlocke0e0
{
	uint8		m_pad_00[0x0c];
	Texe0e0*	m_pHead0c;                              // +0x0c
	uint8		m_pad_10[0x30 - 0x10];
	uint16		m_nStamp30;                             // +0x30
};

// The pair published per stage at 0x1008e664, eight bytes apart
// (`mov %edi,0x1008e668(,%eax,8)` / `mov %ecx,0x1008e664(,%eax,8)` at
// 0x1000e23d / 0x1000e244).
struct TexScalee0e0
{
	float		m_fInvW00;                              // +0x00
	float		m_fInvH04;                              // +0x04
};

// 0x1008cbcc.  Only +0x48 is touched here (`add %edx,0x48(%eax)` at
// 0x1000e1ca).
struct Statse0e0
{
	uint8		m_pad_00[0x48];
	uint32		m_nBytes48;                             // +0x48
};

struct D3DDeve0e0;

struct D3DDeve0e0Vtbl
{
	uint8	m_pad_00[0x50];
	long (__stdcall* SetRenderState)(D3DDeve0e0*, uint32 dwState,
	                                 uint32 dwValue);            // +0x50
	uint8	m_pad_54[0x68 - 0x54];
	long (__stdcall* DrawIndexedPrimitive)(D3DDeve0e0*, uint32 nPrimType,
	                                       uint32 nFVF, void* pVerts,
	                                       uint32 nVerts, uint16* pIndices,
	                                       uint32 nIndices, uint32 nFlags); // +0x68 (0x1000daa0)
	uint8	m_pad_6c[0x8c - 0x6c];
	long (__stdcall* SetTexture)(D3DDeve0e0*, uint32 dwStage,
	                             DDSurfe0e0* lpTex);             // +0x8c
};

struct D3DDeve0e0
{
	D3DDeve0e0Vtbl*	lpVtbl;
};


// ----------------------------------------------------------------------- //
//  THE OBJECT -- 0x8(%ebp).  Only the seven members this function reads
//  are spelled out; everything else is padding.  Each is cited.
// ----------------------------------------------------------------------- //
struct Obje0e0
{
	uint8			m_pad_000[0x88];
	uint32			m_nFlags88;                     // +0x088  0x1000e45a
	uint8			m_pad_08c[0x94 - 0x8c];
	uint8			m_nR94;                         // +0x094  0x1000daa0's colour
	uint8			m_nG95;                         // +0x095
	uint8			m_nB96;                         // +0x096
	uint8			m_nA97;                         // +0x097
	uint8			m_pad_098[0x9c - 0x98];
	Quate0e0		m_qRot9c;                       // +0x09c  0x1000e30e
	Vece0e0			m_vScaleac;                     // +0x0ac  0x1000e314
	uint8			m_pad_0b8[0x18c - 0xb8];
	Vece0e0			m_vPos18c;                      // +0x18c  0x1000e329
	uint8			m_pad_198[0x1c8 - 0x198];
	void*			m_pParticles1c8;                // +0x1c8  0x1000ec8d
	uint8			m_pad_1cc[0x1e8 - 0x1cc];
	TexBlocke0e0*	m_pBlock1e8;                    // +0x1e8  0x1000e0f7
	uint8			m_pad_1ec[0x228 - 0x1ec];
	int32			m_nCount228;                    // +0x228  0x1000ec87
};


// ======================================================================= //
//  GLOBALS.  Every one is an address these bytes touch.
// ======================================================================= //

extern uint16			g_w1008b870;                    // 0x1008b870
extern uint32			g_n1008bd80;                    // 0x1008bd80
extern uint16			g_w1008bea8;                    // 0x1008bea8
extern Statse0e0*		g_p1008cbcc;                    // 0x1008cbcc
extern uint32			g_n1008d7d0;                    // 0x1008d7d0  stage
extern D3DDeve0e0*		g_pD3DDevice;                   // 0x1008d988
extern Linke0e0*		g_p1008e634;                    // 0x1008e634
extern TexScalee0e0		g_a1008e664[];                  // 0x1008e664
extern Texe0e0*			g_a1008e684[];                  // 0x1008e684

// The two view matrices big17c14.cpp names, chosen by the SAME
// `m_nFlags88 & 0x40` test it records at 0x10018233.
extern Mate0e0			g_Mat1008b9dc;                  // 0x1008b9dc
extern Mate0e0			g_Mat1008bb5c;                  // 0x1008bb5c

// ----------------------------------------------------------------------- //
//  The eight-dword block at 0x1008adb0..0x1008adcf that this function
//  fills and 0x1000daa0 consumes.  They are written as eight separate
//  globals and not as one struct because nothing in the shipped bytes
//  addresses them off a common base -- every access is an absolute.
//
//    0x1008adb0  filled by g_pfn1008ce8c (0x1000ec6c); read by
//                0x1000daa0 at 0x1000dfb3/0x1000dfde/0x1000e00b/
//                0x1000e02b as an integer.
//    0x1008adb4  v1 = (height - 2.0f) * m_fInvH04
//    0x1008adb8  u1 = (width  - 2.0f) * m_fInvW00
//    0x1008adbc  v0 = m_fInvH04 * 2.0f
//    0x1008adc0  u0 = m_fInvW00 * 2.0f
//    0x1008adc4/c8/cc  1.0f/255.0f each.  The immediate is 0x3b808081,
//                dumped from the instruction, not inferred; 0x1000daa0
//                multiplies the object's +0x94/+0x95/+0x96 bytes by them
//                at 0x1000daf1/0x1000db10/0x1000db2e, so they are the
//                red/green/blue byte-to-float scales.
// ----------------------------------------------------------------------- //
extern uint32			g_n1008adb0;                    // 0x1008adb0
extern float			g_f1008adb4;                    // 0x1008adb4
extern float			g_f1008adb8;                    // 0x1008adb8
extern float			g_f1008adbc;                    // 0x1008adbc
extern float			g_f1008adc0;                    // 0x1008adc0
extern float			g_f1008adc4;                    // 0x1008adc4
extern float			g_f1008adc8;                    // 0x1008adc8
extern float			g_f1008adcc;                    // 0x1008adcc

// 0x1008ce8c -- big17c14.cpp's and big0f6f0.cpp's declaration, verbatim.
// The call at 0x1000ec6c takes BOTH arguments in registers, %ecx =
// &pObj->m_vPos18c and %edx = $0x1008adb0, and nothing is pushed;
// __fastcall is the only spelling with that register pair.
typedef void (__fastcall* PFN1008ce8c)(Vece0e0* pPos, uint32* pOut);
extern PFN1008ce8c		g_pfn1008ce8c;                  // 0x1008ce8c


// ======================================================================= //
//  THE FREE FUNCTIONS THIS ONE CALLS.  Declared and never defined, so no
//  byte here is claimed at any of their addresses.
// ======================================================================= //

// 0x10039bd0 -- d3dmid.cpp's stack-ABI entry (body at 0x10039bdc); the
// same three arguments big43e10.cpp, big2ef34.cpp and big6d910.cpp hand
// it.  Returns the record, or null.
extern "C" Texe0e0* sub_10039bd0(TexBlocke0e0* pBlock, uint32 nStage,
                                 uint32 bLink);

// 0x10073400 -- quaternion to matrix, __cdecl, (rotation, matrix).
// d3dr2a.cpp / d3dr3b.cpp / d3dbatchd.cpp all declare it this way.  Its
// body writes ALL SIXTEEN elements (nine rotation terms at +0x00..+0x28,
// zeros at +0x0c/+0x1c/+0x2c/+0x30/+0x34/+0x38 and 1.0f at +0x3c,
// 0x10073490..0x100734ea), which is why this function initialises only
// the translation column afterwards.
extern "C" void sub_10073400(Quate0e0* pRot, Mate0e0* pMat);

// ----------------------------------------------------------------------- //
//  0x1000daa0 -- THE PARTICLE BATCH DRAW, and a /Qip register entry.
//
//  Its prologue is the spill form:  `push %ebp ; mov %esp,%ebp ;
//  push %ecx ; push %edx ; push %eax ; mov $0x4150,%eax ;
//  call 0x100805a0` and then it reads the three back at 0x4150(%esp),
//  0x4154(%esp), 0x4158(%esp) (0x1000dab0..0x1000dabe).  So arguments
//  one to three arrive in %eax, %edx, %ecx.  It also consumes an
//  INCOMING st(0): the caller's `flds` value survives four `fildl` /
//  `fstpl` pairs and is stored as a double at -0xb0(%ebp) by
//  `fstl -0xb0(%ebp)` at 0x1000db6f, then multiplied at 0x1000db75.
//  The two remaining arguments are ordinary stack slots, read at
//  0x14(%ebp) (0x1000dbd9, used as a 4x4 matrix -- `flds 0x38(%ebx)`,
//  `0x34`, `0x30`, `0x3c` at 0x1000dbe0..0x1000dbf6, the bottom row of a
//  projective transform) and 0x18(%ebp) (0x1000dbbf, compared with 1).
//  %ecx is the item count (`test %ecx,%ecx ; je` at 0x1000dbd1, then
//  `mov %ecx,%edi`) and %edx the item cursor (+0x24/+0x28/+0x2c is the
//  item position).  %eax is the object (+0x94..+0x97 are its four colour
//  bytes).  It returns the advanced cursor in %eax.
//
//  NOTE for the reader: the float parameter takes NO stack slot -- the
//  matrix lands at 0x14(%ebp) and the mode at 0x18(%ebp), i.e. the
//  fourth and fifth dwords, with the first three reserved for the
//  register arguments.  That is what fixes the parameter ORDER written
//  below; a float occupying a slot would have pushed the matrix to
//  0x18(%ebp).
// ----------------------------------------------------------------------- //
// (defined below, static: the /Qip register-only body this TU calls)


// ======================================================================= //
//  ltmatrix.h's MatMul, big48180.cpp's `MatMul4` reused VERBATIM.  There
//  is no `call` for it in the shipped bytes: 0x1000e468..0x1000e858 and
//  0x1000e863..0x1000ec58 are TWO inlined copies back to back, one per
//  arm of the `& 0x40` test.  The proof is the store order -- the
//  sixteen results of each copy land at -0xc8, -0xb8, -0xa8, -0x98,
//  -0xc4, -0xb4, ... which is m[0][0] m[1][0] m[2][0] m[3][0] m[0][1]
//  m[1][1] ..., the header's own column-by-column order, with no
//  exception in either copy -- and the accumulation inside each element
//  runs last-term-first (`flds 0x1008bb68 ; fmuls -0xd8(%ebp)`, i.e.
//  M1[0][3]*M2[3][0], is the FIRST product emitted for m[0][0] at
//  0x1000e468).
// ======================================================================= //
#define M1 pMat1->m
#define M2 pMat2->m

inline void MatMul4(Mate0e0* pDest, Mate0e0* pMat1, Mate0e0* pMat2)
{
	pDest->m[0][0] = M1[0][0]*M2[0][0] + M1[0][1]*M2[1][0] + M1[0][2]*M2[2][0] + M1[0][3]*M2[3][0];
	pDest->m[1][0] = M1[1][0]*M2[0][0] + M1[1][1]*M2[1][0] + M1[1][2]*M2[2][0] + M1[1][3]*M2[3][0];
	pDest->m[2][0] = M1[2][0]*M2[0][0] + M1[2][1]*M2[1][0] + M1[2][2]*M2[2][0] + M1[2][3]*M2[3][0];
	pDest->m[3][0] = M1[3][0]*M2[0][0] + M1[3][1]*M2[1][0] + M1[3][2]*M2[2][0] + M1[3][3]*M2[3][0];

	pDest->m[0][1] = M1[0][0]*M2[0][1] + M1[0][1]*M2[1][1] + M1[0][2]*M2[2][1] + M1[0][3]*M2[3][1];
	pDest->m[1][1] = M1[1][0]*M2[0][1] + M1[1][1]*M2[1][1] + M1[1][2]*M2[2][1] + M1[1][3]*M2[3][1];
	pDest->m[2][1] = M1[2][0]*M2[0][1] + M1[2][1]*M2[1][1] + M1[2][2]*M2[2][1] + M1[2][3]*M2[3][1];
	pDest->m[3][1] = M1[3][0]*M2[0][1] + M1[3][1]*M2[1][1] + M1[3][2]*M2[2][1] + M1[3][3]*M2[3][1];

	pDest->m[0][2] = M1[0][0]*M2[0][2] + M1[0][1]*M2[1][2] + M1[0][2]*M2[2][2] + M1[0][3]*M2[3][2];
	pDest->m[1][2] = M1[1][0]*M2[0][2] + M1[1][1]*M2[1][2] + M1[1][2]*M2[2][2] + M1[1][3]*M2[3][2];
	pDest->m[2][2] = M1[2][0]*M2[0][2] + M1[2][1]*M2[1][2] + M1[2][2]*M2[2][2] + M1[2][3]*M2[3][2];
	pDest->m[3][2] = M1[3][0]*M2[0][2] + M1[3][1]*M2[1][2] + M1[3][2]*M2[2][2] + M1[3][3]*M2[3][2];

	pDest->m[0][3] = M1[0][0]*M2[0][3] + M1[0][1]*M2[1][3] + M1[0][2]*M2[2][3] + M1[0][3]*M2[3][3];
	pDest->m[1][3] = M1[1][0]*M2[0][3] + M1[1][1]*M2[1][3] + M1[1][2]*M2[2][3] + M1[1][3]*M2[3][3];
	pDest->m[2][3] = M1[2][0]*M2[0][3] + M1[2][1]*M2[1][3] + M1[2][2]*M2[2][3] + M1[2][3]*M2[3][3];
	pDest->m[3][3] = M1[3][0]*M2[0][3] + M1[3][1]*M2[1][3] + M1[3][2]*M2[2][3] + M1[3][3]*M2[3][3];
}

#undef M1
#undef M2




// ==== w2_1000daa0.cpp ====


// ---- the particle and its owner.
struct Particle1000daa0
{
	uint8               m_pad_00[0x0c];
	float               m_fR0c;                         // +0x0c
	float               m_fG10;                         // +0x10
	float               m_fB14;                         // +0x14
	float               m_fA18;                         // +0x18
	float               m_fSize1c;                      // +0x1c
	Particle1000daa0*   m_pNext20;                      // +0x20
	LTVector            m_vPos24;                       // +0x24
};

// (Obje0e0 and Mate0e0: bige0e0.cpp's, above)

// The screen vertex, FVF 0x1c4.
struct PVert1000daa0
{
	float           m_fX;                               // +0x00
	float           m_fY;                               // +0x04
	float           m_fZ;                               // +0x08
	float           m_fRHW;                             // +0x0c
	uint32          m_nColor;                           // +0x10
	uint32          m_nSpecular;                        // +0x14
	float           m_fU;                               // +0x18
	float           m_fV;                               // +0x1c
};


// ---- globals.
extern long         g_nViewLeft;                        // 0x1008b89c
extern long         g_nViewRight;                       // 0x1008b8a4
// The view rectangle as floats: left, right, top, bottom.
extern float        g_f1008b8ac;                        // 0x1008b8ac
extern float        g_f1008b8b0;                        // 0x1008b8b0
extern float        g_f1008b8b4;                        // 0x1008b8b4
extern float        g_f1008b8b8;                        // 0x1008b8b8
extern float        g_1008b908;                         // 0x1008b908  near z
extern float        g_f1008b90c;                        // 0x1008b90c  far z
// The particle size scale per pixel of view width.
extern float        g_f1008b914;                        // 0x1008b914
extern Mate0e0      g_Mat1008ba1c;                      // 0x1008ba1c
extern long         g_nParticlesDrawn;                  // 0x1008be4c
extern uint16       g_1008efd4[768];                    // 0x1008efd4

// bige0e0.cpp's eight-dword block, verbatim.
extern uint32       g_n1008adb0;                        // 0x1008adb0
extern float        g_f1008adb4;                        // 0x1008adb4
extern float        g_f1008adb8;                        // 0x1008adb8
extern float        g_f1008adbc;                        // 0x1008adbc
extern float        g_f1008adc0;                        // 0x1008adc0
extern float        g_f1008adc4;                        // 0x1008adc4
extern float        g_f1008adc8;                        // 0x1008adc8
extern float        g_f1008adcc;                        // 0x1008adcc


static void* sub_1000daa0(Obje0e0* pObj, void* pCursor, uint32 nCount,
                          float fFrac, Mate0e0* pMat, uint32 nMode)
{
	PVert1000daa0       aVerts[128 * 4];
	PVert1000daa0*      pOut;
	Particle1000daa0*   pCur;
	double              fScaleR, fScaleG, fScaleB, fScaleA;
	double              fSizeScale, fSize;
	double              fU0, fV0, fU1, fV1;
	double              fX, fY, fZ, fW;
	double              fSX, fSY, fSZ, fSW, fHalf;
	double              x0, y0, x1, y1, u0, v0, u1, v1, t;
	int32               nR, nG, nB, nA;
	uint32              nAlpha, nColor;
	uint32              nVerts, nQuads;

	pCur = (Particle1000daa0*)pCursor;

	fScaleR = (double)pObj->m_nR94 * g_f1008adc4;
	fScaleG = (double)pObj->m_nG95 * g_f1008adc8;
	fScaleB = (double)pObj->m_nB96 * g_f1008adcc;
	fScaleA = (double)pObj->m_nA97;

	fSizeScale = (double)(g_nViewRight - g_nViewLeft) * g_f1008b914;
	fSize = fFrac * fSizeScale;

	fU0 = g_f1008adc0;
	fV0 = g_f1008adbc;
	fU1 = g_f1008adb8;
	fV1 = g_f1008adb4;

	nAlpha = 0;
	pOut = aVerts;

	if (nMode != 1)
		return pCur;

	for (; nCount; nCount--, pCur = pCur->m_pNext20)
	{
		// Into the caller's space, with the homogeneous divide.
		fW = 1.0f / (pMat->m[3][0] * pCur->m_vPos24.x
		           + pMat->m[3][1] * pCur->m_vPos24.y
		           + pMat->m[3][2] * pCur->m_vPos24.z + pMat->m[3][3]);
		fZ = (pMat->m[2][0] * pCur->m_vPos24.x
		    + pMat->m[2][1] * pCur->m_vPos24.y
		    + pMat->m[2][2] * pCur->m_vPos24.z + pMat->m[2][3]) * fW;
		fX = (pMat->m[0][0] * pCur->m_vPos24.x
		    + pMat->m[0][1] * pCur->m_vPos24.y
		    + pMat->m[0][2] * pCur->m_vPos24.z + pMat->m[0][3]) * fW;
		fY = (pMat->m[1][0] * pCur->m_vPos24.x
		    + pMat->m[1][1] * pCur->m_vPos24.y
		    + pMat->m[1][2] * pCur->m_vPos24.z + pMat->m[1][3]) * fW;

		if (!(fZ > g_1008b908) || !(fZ < g_f1008b90c))
			continue;
		if (!(fX > -fZ) || !(fX < fZ) || !(fY < fZ) || !(fY > -fZ))
			continue;

		// Onto the screen.
		fSW = 1.0f / (g_Mat1008ba1c.m[3][0] * fX + g_Mat1008ba1c.m[3][1] * fY
		            + g_Mat1008ba1c.m[3][2] * fZ + g_Mat1008ba1c.m[3][3]);
		fSX = (g_Mat1008ba1c.m[0][0] * fX + g_Mat1008ba1c.m[0][1] * fY
		     + g_Mat1008ba1c.m[0][2] * fZ + g_Mat1008ba1c.m[0][3]) * fSW;
		fSY = (g_Mat1008ba1c.m[1][0] * fX + g_Mat1008ba1c.m[1][1] * fY
		     + g_Mat1008ba1c.m[1][2] * fZ + g_Mat1008ba1c.m[1][3]) * fSW;
		fSZ = (g_Mat1008ba1c.m[2][0] * fX + g_Mat1008ba1c.m[2][1] * fY
		     + g_Mat1008ba1c.m[2][2] * fZ + g_Mat1008ba1c.m[2][3]) * fSW;

		if (fFrac == 0.0f)
			fSize = pCur->m_fSize1c * fSizeScale;

		fHalf = fSize * fSW;

		// The colour: alpha kept in the top byte across particles.
		nA = (int32)(float)(pCur->m_fA18 * fScaleA);
		nAlpha = (nAlpha & 0xffffff) | ((uint32)(uint8)nA << 24);
		nR = (int32)(float)(pCur->m_fR0c * fScaleR);
		nG = (int32)(float)(pCur->m_fG10 * fScaleG);
		nB = (int32)(float)(pCur->m_fB14 * fScaleB);
		nColor = (nR << 16) | nB | (nG << 8) | nAlpha;

		// The square, clipped to the view rectangle with the uvs
		// interpolated to match.
		x0 = fSX - fHalf;
		y0 = fSY - fHalf;
		x1 = fHalf + fSX;
		y1 = fHalf + fSY;
		u0 = fU0;
		v0 = fV0;
		u1 = fU1;
		v1 = fV1;

		if (g_f1008b8ac > x0)
		{
			t = (g_f1008b8ac - x0) / (x1 - x0);
			u0 = (u1 - u0) * t + u0;
			x0 = g_f1008b8ac;
		}

		if (g_f1008b8b4 > y0)
		{
			t = (g_f1008b8b4 - y0) / (y1 - y0);
			v0 = (v1 - v0) * t + v0;
			y0 = g_f1008b8b4;
		}

		if (g_f1008b8b0 < x1)
		{
			t = (g_f1008b8b0 - x0) / (x1 - x0);
			u1 = (u1 - u0) * t + u0;
			x1 = g_f1008b8b0;
		}

		if (g_f1008b8b8 < y1)
		{
			t = (g_f1008b8b8 - y0) / (y1 - y0);
			v1 = (v1 - v0) * t + v0;
			y1 = g_f1008b8b8;
		}

		pOut[0].m_fX = (float)x0;
		pOut[0].m_fY = (float)y0;
		pOut[0].m_fZ = (float)fSZ;
		pOut[0].m_fRHW = (float)fSW;
		pOut[0].m_nColor = nColor;
		pOut[0].m_nSpecular = g_n1008adb0;
		pOut[0].m_fU = (float)u0;
		pOut[0].m_fV = (float)v0;

		pOut[1].m_fX = (float)x1;
		pOut[1].m_fY = (float)y0;
		pOut[1].m_fZ = (float)fSZ;
		pOut[1].m_fRHW = (float)fSW;
		pOut[1].m_nColor = nColor;
		pOut[1].m_nSpecular = g_n1008adb0;
		pOut[1].m_fU = (float)u1;
		pOut[1].m_fV = (float)v0;

		pOut[2].m_fX = (float)x1;
		pOut[2].m_fY = (float)y1;
		pOut[2].m_fZ = (float)fSZ;
		pOut[2].m_fRHW = (float)fSW;
		pOut[2].m_nColor = nColor;
		pOut[2].m_nSpecular = g_n1008adb0;
		pOut[2].m_fU = (float)u1;
		pOut[2].m_fV = (float)v1;

		pOut[3].m_fX = (float)x0;
		pOut[3].m_fY = (float)y1;
		pOut[3].m_fZ = (float)fSZ;
		pOut[3].m_fRHW = (float)fSW;
		pOut[3].m_nColor = nColor;
		pOut[3].m_nSpecular = g_n1008adb0;
		pOut[3].m_fU = (float)u0;
		pOut[3].m_fV = (float)v1;

		pOut += 4;
	}

	if (pOut != aVerts)
	{
		nVerts = pOut - aVerts;
		nQuads = nVerts >> 2;
		g_nParticlesDrawn += nQuads;
		g_pD3DDevice->lpVtbl->DrawIndexedPrimitive(g_pD3DDevice, 4, 0x1c4,
			aVerts, nVerts, g_1008efd4, nQuads * 6, 0);
	}

	return pCur;
}


// ==== bige0e0.cpp: 0x1000e0e0 ====
// ======================================================================= //
//  0x1000e0e0 .. 0x1000eea7
//
//  The body is written FLAT with the shipped labels because the shipped
//  control flow is flat: the no-texture block at 0x1000ee01 is reached
//  from THREE places (0x1000e0ff, 0x1000edef and 0x1000ee88) and the
//  bind block at 0x1000e173 from THREE (0x1000e16d's fallthrough,
//  0x1000edfc and 0x1000ee8e).  big43e10.cpp uses the same goto spelling
//  for the same block.
// ======================================================================= //
void sub_1000e0e0(Obje0e0* pObj, uint32 nUnused0c)
{
	TexBlocke0e0*	pBlock;
	Texe0e0*		pHead;
	Texe0e0*		pCur;
	Texe0e0*		pRec;
	Texe0e0*		pTex;
	uint32			nStage;

	Quate0e0*		pRot;
	Vece0e0*		pScale;
	Vece0e0*		pPos;

	Mate0e0			mLocal;                             // -0x108
	Mate0e0			mFinal;                             // -0xc8

	int32			nBlocks;
	int32			nRem;
	int32			i;
	void*			pCursor;

	// ------------------------------------------------------------------ //
	//  REGION 1 -- 0x1000e0f4 .. 0x1000e172, and the sunk 0x1000edc8 /
	//  0x1000ee67 / 0x1000ee93 / 0x1000ee9d.
	//
	//  FIND THIS STAGE'S TEXTURE RECORD.  big43e10.cpp's block with the
	//  block pointer taken from the OBJECT (`mov 0x1e8(%esi),%ecx` at
	//  0x1000e0f7) instead of from g_p1008cbcc->+0x104.  %ecx holds
	//  pBlock live from there all the way down to the two 0x10039bd0
	//  calls at 0x1000ede2 / 0x1000ee79, which is what says one local
	//  reaches both.
	// ------------------------------------------------------------------ //
	pBlock = pObj->m_pBlock1e8;

	if (!pBlock)
		goto L_NoTexture;                           // 0x1000e0ff

	// -- 0x1000e105/0x1000e11a: `movzwl 0x1008b870,%eax` then
	//    `mov %ax,0x30(%ecx)`.
	pBlock->m_nStamp30 = g_w1008b870;

	// -- 0x1000e10c: read ONCE into -0x70 and reused at 0x1000e163,
	//    0x1000edd3 and 0x1000ee6a.  L_NoTexture below re-reads it,
	//    which is why that arm gets its own statement.
	nStage = g_n1008d7d0;

	// -- 0x1000e112: loaded ONCE into %edi and still live at L_MakeRec
	//    (0x1000edc8, 0x1000edf1).
	pHead = pBlock->m_pHead0c;

	pRec = LTNULL;

	// The list test is emitted TWICE, `je 0x1000edc8` at 0x1000e11e and
	// `je 0x1000e157` at 0x1000e127 -- the first jumping straight to
	// L_MakeRec because icl has folded "the loop never ran, so pRec is
	// still null".  big43e10.cpp, big2ef34.cpp and big61a30.cpp all
	// record the same pair.
	if (pHead)
	{
		pCur = pHead;

		// UNROLLED BY TWO in the shipped bytes: 0x1000e131..0x1000e13d
		// and 0x1000e144..0x1000e155, with BOTH `break` trampolines sunk
		// (0x1000ee93, 0x1000ee9d).  `cmp %edx,%ebx` with the RECORD's
		// field in %ebx makes the record the left operand.
		// `movzbl %dl,%edx` at 0x1000e12b is the cast on the stage.
		while (pCur)
		{
			if (pCur->m_nStage42 == (uint8)nStage)
			{
				pRec = pCur;
				break;
			}

			pCur = pCur->m_pNext30;
		}
	}

	if (!pRec)
		goto L_MakeRec;                             // 0x1000e15d

	// -- 0x1000e166: `cmp 0x1008e684(,%eax,4),%edx`.
	if (pRec == g_a1008e684[nStage])
		goto L_ElseLOD;                             // 0x1000e16d

L_Bind:
	// ------------------------------------------------------------------ //
	//  REGION 2 -- 0x1000e173 .. 0x1000e271.  BIND.  Instruction for
	//  instruction big43e10.cpp's L_Bind.
	// ------------------------------------------------------------------ //
	g_a1008e684[pRec->m_nStage42] = pRec;

	if (pRec->m_nStamp14 != g_w1008bea8)
	{
		// ltlink.h:100..145 dl_Remove / dl_TieOff / dl_Insert,
		// 0x1000e18f..0x1000e1c1, prev at +0x00 and next at +0x04.
		pRec->m_Link1c.m_pPrev->m_pNext = pRec->m_Link1c.m_pNext;
		pRec->m_Link1c.m_pNext->m_pPrev = pRec->m_Link1c.m_pPrev;
		pRec->m_Link1c.m_pPrev = &pRec->m_Link1c;
		pRec->m_Link1c.m_pNext = &pRec->m_Link1c;

		pRec->m_Link1c.m_pPrev = g_p1008e634;
		pRec->m_Link1c.m_pNext = g_p1008e634->m_pNext;
		pRec->m_Link1c.m_pNext->m_pPrev = &pRec->m_Link1c;
		pRec->m_Link1c.m_pPrev->m_pNext = &pRec->m_Link1c;

		// 0x1000e1c2: `mov 0x1008cbcc,%eax ; mov 0x10(%edi),%edx ;
		// add %edx,0x48(%eax)`.
		g_p1008cbcc->m_nBytes48 += pRec->m_nBytes10;

		// 0x1000e1cd -- g_w1008bea8 is read a SECOND time here, not
		// reused from the compare's register.
		pRec->m_nStamp14 = g_w1008bea8;
	}

	// 0x1000e1f8 -- vtable +0x8c, the stage TRUNCATED TO A BYTE
	// (`movzwl 0x42(%ebx),%eax` then `movzbl %al,%eax` at
	// 0x1000e1d8 / 0x1000e1e8).
	g_pD3DDevice->lpVtbl->SetTexture(g_pD3DDevice,
		(uint8)pRec->m_nStage42, pRec->m_pSurface0c);

	if (pRec->m_nAlphaRef16)
	{
		// 0x1000e223 -- vtable +0x50, state 0x18 =
		// D3DRENDERSTATE_ALPHAREF (`movl $0x18,0x4(%esp)` at
		// 0x1000e216).
		g_pD3DDevice->lpVtbl->SetRenderState(g_pD3DDevice, 0x18,
			pRec->m_nAlphaRef16);
	}

	// 0x1000e22a/0x1000e233/0x1000e237 -- loaded, incremented and STORED
	// BACK BEFORE the pair goes out at 0x1000e23d/0x1000e244.
	// big43e10.cpp records exactly this ordering and contrasts it with
	// big2ef34.cpp's and big6d910.cpp's `incl 0x1008bd80` in memory
	// AFTER the pair.  So the counter statement comes first.
	g_n1008bd80++;

	// COIN-FLIP: the two stores are emitted +0x08 first (0x1008e668 =
	// m_fInvH04 at 0x1000e23d, then 0x1008e664 = m_fInvW00 at
	// 0x1000e244) while the two LOADS are +0x04 first (0x1000e230 /
	// 0x1000e234).  big61a30.cpp writes H then W for that emitted order;
	// big43e10.cpp writes W then H for it.  Both spellings are on the
	// record; the H-first one is written here.
	g_a1008e664[pRec->m_nStage42].m_fInvH04 = pRec->m_fInvH08;
	g_a1008e664[pRec->m_nStage42].m_fInvW00 = pRec->m_fInvW04;

	if (pRec->m_nDirty44)
	{
		// 0x1000e266 -- the surface's own vtable +0xbc.
		pRec->m_pSurface0c->lpVtbl->SetLOD(pRec->m_pSurface0c, 0);
		pRec->m_nDirty44 = 0;
	}

	goto L_HaveTexture;                             // fall through

L_ElseLOD:
	// -- 0x1000ed92 (SUNK): already bound to this stage, so only the LOD
	//    reset survives.  The same three statements as the tail of
	//    L_Bind; big43e10, big2ef34, big6d910 and big61a30 all record the
	//    same duplication.
	if (pRec->m_nDirty44)
	{
		pRec->m_pSurface0c->lpVtbl->SetLOD(pRec->m_pSurface0c, 0);
		pRec->m_nDirty44 = 0;
	}

	goto L_HaveTexture;                             // 0x1000edc3

L_MakeRec:
	// -- 0x1000edc8 (SUNK): a THIRD test of the same list head, and the
	//    third argument is 1 when the list is non-empty and 0 when it is
	//    empty.  THE DIFFERENCE FROM big43e10: both failure arms here go
	//    to the NO-TEXTURE block (`je 0x1000ee01` at 0x1000edef and at
	//    0x1000ee88), not to a shared done.
	if (pHead)
	{
		pRec = sub_10039bd0(pBlock, nStage, 1);

		if (!pRec)
			goto L_NoTexture;                       // 0x1000edef

		// 0x1000edf1: `mov 0x30(%edi),%edx ; mov %edx,0x30(%eax) ;
		// mov %eax,0x30(%edi)` -- inserted AFTER the head.
		pRec->m_pNext30 = pHead->m_pNext30;
		pHead->m_pNext30 = pRec;

		goto L_Bind;                                // 0x1000edfc
	}

	// -- 0x1000ee67: the empty-list arm.
	pRec = sub_10039bd0(pBlock, nStage, 0);

	if (!pRec)
		goto L_NoTexture;                           // 0x1000ee88

	goto L_Bind;                                    // 0x1000ee8e

L_HaveTexture:
	// ------------------------------------------------------------------ //
	//  REGION 3 -- 0x1000e272 .. 0x1000e30d.  THE QUAD'S UV EXTENTS,
	//  published into the 0x1008adb0 block.  This is big61a30.cpp's
	//  0x10061e7c block with globals for destinations.
	//
	//  THE STATEMENT ORDER IS PINNED BY THE LITERAL POOL, not by the
	//  schedule.  The four `2.0f` are FOUR SEPARATE pool entries,
	//  0x100a45e4, 0x100a45e8, 0x100a45ec and 0x100a45f0 (all
	//  0x40000000, dumped from the image, not inferred), consumed in
	//  exactly that order: 0x1000e272 uses e4, 0x1000e2b4 uses e8,
	//  0x1000e2c0 uses ec, 0x1000e2fc uses f0.  So the source really is
	//  u0, u1, v0, v1 and no other permutation.  The four DESTINATIONS
	//  come out 0x1008adc0, 0x1008adb8, 0x1008adbc, 0x1008adb4 in the
	//  same order (0x1000e283, e2ce, e2d4, e308).
	//
	//  pTex is a LOCAL, not the array expression written twice:
	//  `mov 0x1008d7d0,%eax` appears ONCE (0x1000e27e) and
	//  `mov 0x1008e684(,%eax,4),%ebx` ONCE (0x1000e289), and %ecx
	//  carries it across BOTH indirect calls.  icl 4.0 cannot hoist a
	//  global across an indirect call, so the
	//  twice-written form would have reloaded.
	//
	//  The two `fildll -0x88(%ebp)` at 0x1000e2ae and 0x1000e2f6 read a
	//  64-bit temp whose high half was just zeroed (`movl $0x0,-0x84`),
	//  so both accessors return UNSIGNED and the cast is from uint32.
	// ------------------------------------------------------------------ //
	pTex = g_a1008e684[g_n1008d7d0];

	g_f1008adc0 = g_a1008e664[0].m_fInvW00 * 2.0f;
	g_f1008adb8 = ((float)pTex->GetWidth0c() - 2.0f)
	            * g_a1008e664[0].m_fInvW00;

	g_f1008adbc = g_a1008e664[0].m_fInvH04 * 2.0f;
	g_f1008adb4 = ((float)pTex->GetHeight10() - 2.0f)
	            * g_a1008e664[0].m_fInvH04;

	goto L_Transform;

L_NoTexture:
	// ------------------------------------------------------------------ //
	//  REGION 4 -- 0x1000ee01 .. 0x1000ee66 (SUNK).  No texture: zero the
	//  four extents and unbind the stage.
	//
	//  g_n1008d7d0 is read AGAIN here (0x1000ee01) rather than reused
	//  from -0x70, which is why this arm carries its own statement.  The
	//  four zero stores go out in ASCENDING address order -- 0x1008adb4,
	//  b8, bc, c0 at 0x1000ee14/e1a/e20/e26 -- against the other arm's
	//  u0-first order; zero-init stores follow declaration
	//  order, so the two arms genuinely differ and the order written
	//  below is the emitted one.  All four are `mov %edx,<abs>` with
	//  %edx zeroed once at 0x1000ee06, the "zero register".
	// ------------------------------------------------------------------ //
	nStage = g_n1008d7d0;
	pTex   = g_a1008e684[nStage];

	g_f1008adb4 = 0.0f;
	g_f1008adb8 = 0.0f;
	g_f1008adbc = 0.0f;
	g_f1008adc0 = 0.0f;

	if (pTex)
	{
		// 0x1000ee4e -- vtable +0x8c with the stage as a FULL DWORD
		// (`mov -0x74(%ebp),%edx`), not truncated as it is in L_Bind.
		g_pD3DDevice->lpVtbl->SetTexture(g_pD3DDevice, nStage, LTNULL);

		g_a1008e684[nStage] = LTNULL;
	}

L_Transform:
	;

	// ------------------------------------------------------------------ //
	//  REGION 5 -- 0x1000e30e .. 0x1000e3aa, and the four sunk second
	//  halves at 0x1000ed36 / 0x1000ed4d / 0x1000ed64 / 0x1000ed7b.
	//
	//  THE QUATERNION SANITY CLAMP.  Four two-sided range tests on
	//  pObj->m_qRot9c, then quaternion-to-matrix into mLocal.
	//
	//  Each test is one `||` with the second half SUNK, which is icl's
	//  `||` shape exactly:
	//      flds (%edx)            ; the component, KEPT
	//      flds 0x100a45f4        ; 100000.0f
	//      fcomp %st(1)           ; compares the CONSTANT with it, so
	//      fnstsw / sahf / jae    ; `jae` = 100000.0f >= q.x = NOT >
	//      fstpl -0x28(%ebp)      ; the component spilled as a double for
	//                             ; the second half
	//  and the sunk half reloads that double and does
	//      fcomps 0x100a4634 / jb <the store>  /  jmp <the next test>.
	//
	//  THE CONSTANTS ARE DUMPED, NOT INFERRED.  0x100a45f4 = 0x47c35000
	//  = 100000.0f, and the four negatives 0x100a4628..0x100a4634 are all
	//  0xc7c35000 = -100000.0f.  The replacement values are folded to
	//  immediates in the code (`movl $0x0,(%edx)` and
	//  `movl $0x3f800000,0xc(%edx)`) but still hold their pool slots --
	//  see the literal-pool note under REGION 8.
	//
	//  COIN-FLIP: `q.x > 100000.0f` and `100000.0f < q.x` compile to the
	//  same `fcomp %st(1)` here and cannot be told apart.
	// ------------------------------------------------------------------ //

	// 0x1000e30e / 0x1000e314 / 0x1000e329 -- all three addresses are
	// materialised HERE with `lea`, before any of them is used; the
	// position's is materialised a SECOND time at 0x1000ec5e for the
	// g_pfn1008ce8c call, which is why it is not one hoisted local.
	// COIN-FLIP: three named pointers and three `&pObj->m_...`
	// expressions produce the same three `lea`s.
	pRot   = &pObj->m_qRot9c;
	pScale = &pObj->m_vScaleac;
	pPos   = &pObj->m_vPos18c;

	if (pRot->x > 100000.0f || pRot->x < -100000.0f)
		pRot->x = 0.0f;                             // 0x1000e336

	if (pRot->y > 100000.0f || pRot->y < -100000.0f)
		pRot->y = 0.0f;                             // 0x1000e353

	if (pRot->z > 100000.0f || pRot->z < -100000.0f)
		pRot->z = 0.0f;                             // 0x1000e371

	if (pRot->w > 100000.0f || pRot->w < -100000.0f)
		pRot->w = 1.0f;                             // 0x1000e38f

	// 0x1000e3a6 -- __cdecl, `add $0xfffffff8,%esp` then the quaternion
	// at (%esp) and &mLocal at 0x4(%esp), `add $0x8,%esp` after.
	sub_10073400(pRot, &mLocal);

	// ------------------------------------------------------------------ //
	//  REGION 6 -- 0x1000e3ab .. 0x1000e459.  SCALE THE 3x3 AND PLANT THE
	//  TRANSLATION COLUMN.
	//
	//  Nine `flds` / `fmuls` / `fstps` triples, COLUMN BY COLUMN: column 0
	//  by x (0x1000e3ab, e3bc, e3c4), column 1 by y (e3d4, e3e5, e3f6),
	//  column 2 by z (e407, e418, e429).  Eight of the nine load the
	//  matrix element first and the scale second, which is the compound
	//  assignment (an FP sum emits its terms backwards; a
	//  compound assignment does not); only the very first is scheduled
	//  the other way round.
	//
	//  Row 3 is NOT touched: 0x10073400 already wrote (0,0,0,1) there
	//  (0x100734d3..0x100734e3), and the two MatMul4 arms below read
	//  -0xd8/-0xd4/-0xd0/-0xcc eight times each without any store to
	//  them anywhere in this function.
	//
	//  The three translation stores at 0x1000e440/e448/e451 are INTEGER
	//  moves (`mov (%edi),%eax ; mov %eax,-0xfc(%ebp)`), not fld/fstp.
	//  d3dmid.cpp records the same trap for icl 4.0; the destinations are
	//  float here because the MatMul4 arms multiply them.
	// ------------------------------------------------------------------ //
	mLocal.m[0][0] *= pScale->x;
	mLocal.m[1][0] *= pScale->x;
	mLocal.m[2][0] *= pScale->x;

	mLocal.m[0][1] *= pScale->y;
	mLocal.m[1][1] *= pScale->y;
	mLocal.m[2][1] *= pScale->y;

	mLocal.m[0][2] *= pScale->z;
	mLocal.m[1][2] *= pScale->z;
	mLocal.m[2][2] *= pScale->z;

	mLocal.m[0][3] = pPos->x;                       // -0xfc
	mLocal.m[1][3] = pPos->y;                       // -0xec
	mLocal.m[2][3] = pPos->z;                       // -0xdc

	// ------------------------------------------------------------------ //
	//  REGION 7 -- 0x1000e45a .. 0x1000ec58.  COMPOSE WITH THE VIEW
	//  MATRIX.  2,047 bytes, NO CALLS -- the call-graph check is blind
	//  here and the census above is what carries it.
	//
	//  `mov 0x88(%esi),%eax ; test $0x40,%al ; je 0x1000e863` at
	//  0x1000e45a is big17c14.cpp's 0x10018233 test, same flag word, same
	//  bit, same two matrices, same arm order (0x40 set takes
	//  g_Mat1008bb5c).
	//
	//  The two arms are two INLINED copies of MatMul4 with the global
	//  matrix as M1 and mLocal as M2.  Pinned three ways:
	//    * the store order is column by column, -0xc8 -0xb8 -0xa8 -0x98
	//      -0xc4 -0xb4 ... in both copies with no exception;
	//    * every element's accumulation is emitted last-term-first, e.g.
	//      m[0][0] starts `flds 0x1008bb68 ; fmuls -0xd8(%ebp)` =
	//      M1[0][3]*M2[3][0] (0x1000e468) and ends with
	//      `flds 0x1008bb5c ; fmuls -0x108(%ebp)` = M1[0][0]*M2[0][0]
	//      (0x1000e490);
	//    * within a term the GLOBAL is loaded first and the local
	//      multiplied in, so the global is M1 and mLocal is M2.
	//  The two arms use identical instruction sequences with the base
	//  address changed -- the 28 `flds` off 0x1008bb5c in the first arm
	//  are matched one for one by 28 off 0x1008b9dc in the second.
	// ------------------------------------------------------------------ //
	if (pObj->m_nFlags88 & 0x40)
		MatMul4(&mFinal, &g_Mat1008bb5c, &mLocal);  // 0x1000e468
	else
		MatMul4(&mFinal, &g_Mat1008b9dc, &mLocal);  // 0x1000e863

	// ------------------------------------------------------------------ //
	//  REGION 8 -- 0x1000ec59 .. 0x1000ed35.  THE AMBIENT LOOKUP, THE
	//  THREE COLOUR SCALES, AND THE BATCHED DRAW.
	//
	//  THE LITERAL POOL IS THE STATEMENT-ORDER WITNESS for the whole
	//  function.  Every float literal this function uses occupies its own
	//  slot in .data1 in source order, even the ones icl then folded into
	//  immediates, and the slots run without a gap:
	//
	//      0x100a45e4  2.0f        u0            (REGION 3)
	//      0x100a45e8  2.0f        u1
	//      0x100a45ec  2.0f        v0
	//      0x100a45f0  2.0f        v1
	//      0x100a45f4  100000.0f   x clamp       (REGION 5)
	//      0x100a45f8  0.0f        x replacement (folded to `movl $0x0`)
	//      0x100a45fc  100000.0f   y clamp
	//      0x100a4600  0.0f        y replacement (folded)
	//      0x100a4604  100000.0f   z clamp
	//      0x100a4608  0.0f        z replacement (folded)
	//      0x100a460c  100000.0f   w clamp
	//      0x100a4610  1.0f        w replacement (folded to
	//                              `movl $0x3f800000`)
	//      0x100a4614  1/255       g_f1008adcc   (REGION 8, below)
	//      0x100a4618  1/255       g_f1008adc8   (all three folded into
	//      0x100a461c  1/255       g_f1008adc4    one `mov $0x3b808081`)
	//      0x100a4620  0.0f        the loop's 0x1000daa0 argument
	//      0x100a4624  0.0f        the remainder's 0x1000daa0 argument
	//      0x100a4628  -100000.0f  w clamp, second half   \  allocated in
	//      0x100a462c  -100000.0f  z clamp, second half    | the SUNK
	//      0x100a4630  -100000.0f  y clamp, second half    | blocks'
	//      0x100a4634  -100000.0f  x clamp, second half   /  emission
	//                                                        order, which
	//                                                        is w,z,y,x
	//
	//  Three separate 1/255 slots say three separate assignment
	//  statements and not one temp reused; two separate 0.0f slots say
	//  two separate calls and not one loop.  No slot in the run is
	//  unaccounted for and no statement below lacks one.
	// ------------------------------------------------------------------ //

	// 0x1000ec6c -- register-ABI, %ecx = &pObj->m_vPos18c (`lea
	// 0x18c(%esi),%ecx`) and %edx = $0x1008adb0, nothing pushed.
	// big17c14.cpp's 0x10017d29 and big0f6f0.cpp's 0x1000f85e are the
	// same call through the same pointer.
	(*g_pfn1008ce8c)(&pObj->m_vPos18c, &g_n1008adb0);

	// 0x1000ec6e/e75/e7d/e82 -- one `mov $0x3b808081,%eax` and three
	// stores, in DESCENDING address order.  0x3b808081 is 1.0f/255.0f;
	// the immediate is read off the instruction, not inferred.
	g_f1008adcc = 1.0f / 255.0f;
	g_f1008adc8 = 1.0f / 255.0f;
	g_f1008adc4 = 1.0f / 255.0f;

	// COIN-FLIP: `mov %edx,-0x6c(%ebp)` with %edx already zeroed sits at
	// 0x1000ec7a, between the first and second colour-scale stores and
	// well above the loop.  That is either a separately declared
	// `int i = 0;` here, or the `for`'s own init hoisted by the "zero
	// register" reuse.  The `for` spelling is written.

	// 0x1000ec87/0x1000ec8d.
	nBlocks = pObj->m_nCount228;
	pCursor = pObj->m_pParticles1c8;

	// 0x1000ec96..0x1000ecac -- `sar $0x6 ; shr $0x19 ; add ; sar $0x7`
	// is SIGNED division by 128, and `shl $0x7 ; sub` is the matching
	// remainder.  No `idiv`, which is why the batch size has to be a
	// power of two; 0x80 is also the literal handed to 0x1000daa0 as the
	// count (`mov $0x80,%ecx` at 0x1000ecde).
	nRem    = nBlocks % 128;
	nBlocks = nBlocks / 128;

	// 0x1000ecae `test %edi,%edi ; jle` and 0x1000ecee `cmp %edi,%ebx ;
	// jl` -- a SIGNED loop, so m_nCount228 is int32.
	for (i = 0; i < nBlocks; i++)
	{
		// 0x1000ecc0..0x1000ece7.  %eax = pObj, %edx = pCursor,
		// %ecx = 0x80, st(0) = 0.0f from 0x100a4620, and the two stack
		// slots 0xc(%esp) = &mFinal, 0x10(%esp) = 1.  `add $0x18,%esp`
		// after, so __cdecl cleanup on a 0x18 outgoing block.
		pCursor = sub_1000daa0(pObj, pCursor, 128, 0.0f, &mFinal, 1);
	}

	// 0x1000ecf5 `test %eax,%eax ; je` -- `!= 0`, not `> 0`.
	if (nRem)
	{
		// 0x1000ecfc..0x1000ed23.  The same call with %ecx = nRem and the
		// float from the SECOND pool slot, 0x100a4624.  The answer is
		// discarded: no `mov %eax,...` follows.
		sub_1000daa0(pObj, pCursor, nRem, 0.0f, &mFinal, 1);
	}

	// 0x1000ed27 -- the epilogue.  `void`: nothing reaches %eax.
}


// ======================================================================= //
//  UNDECODED GAPS
// ======================================================================= //
//
//  NONE.  Every byte of 0x1000e0e0..0x1000eea7 is accounted for by a
//  region above:
//
//      0x1000e0e0..0x1000e0f3   prologue
//      0x1000e0f4..0x1000e172   REGION 1
//      0x1000e173..0x1000e271   REGION 2
//      0x1000e272..0x1000e30d   REGION 3
//      0x1000e30e..0x1000e3aa   REGION 5
//      0x1000e3ab..0x1000e459   REGION 6
//      0x1000e45a..0x1000ec58   REGION 7   (2,047 bytes, NO CALLS)
//      0x1000ec59..0x1000ed35   REGION 8 + epilogue
//      0x1000ed36..0x1000ed91   REGION 5, sunk (the four `||` halves)
//      0x1000ed92..0x1000edc7   REGION 2, sunk (L_ElseLOD)
//      0x1000edc8..0x1000ee00   REGION 1, sunk (L_MakeRec, list)
//      0x1000ee01..0x1000ee66   REGION 4, sunk (L_NoTexture)
//      0x1000ee67..0x1000ee92   REGION 1, sunk (L_MakeRec, empty list)
//      0x1000ee93..0x1000eea6   REGION 1, sunk (the two `break`s)
//
//  What is NOT decoded, and does not need to be, is the CALLEES:
//  0x1000daa0 (1,600 bytes, the per-particle projection and fill),
//  0x10073400 (244 bytes, quaternion to matrix) and 0x10039bd0
//  (d3dmid.cpp's shim onto 0x10039bdc).  Only their argument lists are
//  claimed here, and each was read off the callee's own prologue.
//
// ======================================================================= //
//  EVERY COIN-FLIP, WITH BOTH SPELLINGS
// ======================================================================= //
//
//  1. THE PARAMETER COUNT.  The one call site pushes two dwords and the
//     body reads only 0x8(%ebp):
//         void sub_1000e0e0(Obje0e0* pObj, uint32 nUnused0c);   <- written
//         void sub_1000e0e0(Obje0e0* pObj);
//     The caller uses `push <reg>` as a 4-byte esp reservation elsewhere
//     (0x10050906, 0x10050ae0), so the stack arithmetic does not decide.
//
//  2. THE UV-SCALE STORE ORDER at 0x1000e23d/0x1000e244:
//         g_a1008e664[s].m_fInvH04 = pRec->m_fInvH08;    <- written
//         g_a1008e664[s].m_fInvW00 = pRec->m_fInvW04;
//     against big43e10.cpp's W-then-H for the identical emitted order.
//
//  3. THE THREE `lea`s at 0x1000e30e/0x1000e314/0x1000e329:
//         Quate0e0* pRot = &pObj->m_qRot9c;  ... pRot->x ...   <- written
//         ... pObj->m_qRot9c.x ...
//     Identical code either way.
//
//  4. THE RANGE TESTS: `pRot->x > 100000.0f` and `100000.0f < pRot->x`
//     both give `flds ; flds ; fcomp %st(1) ; jae`.
//
//  5. THE 2.0f PRODUCTS at 0x1000e272 and 0x1000e2c0:
//         g_a1008e664[0].m_fInvW00 * 2.0f          <- written
//         2.0f * g_a1008e664[0].m_fInvW00
//     `flds 2.0f ; fmuls <invW>` fits both; big61a30.cpp writes the
//     first form for the same bytes.
//
//  6. THE LOOP COUNTER'S INIT: `for (i = 0; ...)` as written, or a
//     separate `int i = 0;` sitting where the store is, at 0x1000ec7a.
//
//  7. THE 1/255 CONSTANT: `1.0f / 255.0f` as written, or a folded
//     `0.00392156862f` literal -- the pool holds three copies of
//     0x3b808081 either way and the code holds one `mov $0x3b808081`.
//
//  8. sub_1000daa0's DECLARED PARAMETER ORDER.  The three register
//     arguments and the two stack arguments are fixed by the callee's
//     prologue and by 0x14(%ebp)/0x18(%ebp); what is NOT fixed is where
//     the x87 argument sits in the declaration, because under /Qip it
//     consumes no stack slot.  Written as the fourth parameter; first,
//     second or third would produce the same call site.
//
// ======================================================================= //
//  THE EXPECTED CALL-SEQUENCE FIGURE, AND THE RESIDUALS PREDICTED IN ADVANCE
// ======================================================================= //
//
//  SHIPPED, in order (counted mechanically, not by eye):
//
//      0x1000e1f8  indirect   SetTexture          REGION 2
//      0x1000e223  indirect   SetRenderState      REGION 2
//      0x1000e266  indirect   SetLOD              REGION 2
//      0x1000e29a  indirect   GetWidth0c          REGION 3
//      0x1000e2e4  indirect   GetHeight10         REGION 3
//      0x1000e3a6  0x10073400                     REGION 5
//      0x1000ec6c  indirect   g_pfn1008ce8c       REGION 8
//      0x1000ece3  0x1000daa0                     REGION 8, the loop
//      0x1000ed1f  0x1000daa0                     REGION 8, the remainder
//      0x1000edb4  indirect   SetLOD              REGION 2, sunk
//      0x1000ede5  0x10039bd0                     REGION 1, sunk
//      0x1000ee4e  indirect   SetTexture          REGION 4, sunk
//      0x1000ee7c  0x10039bd0                     REGION 1, sunk
//
//  13 calls, 5 direct over 3 distinct targets, 8 indirect.
//
//  EXPECT 13 shipped against 13 ours.  Predicted residuals, all of them
//  known benign classes:
//
//   a. BLOCK PLACEMENT.  Four of the thirteen sit in blocks the shipped
//      /O2 build SANK below the epilogue -- 0x1000edb4 (L_ElseLOD),
//      0x1000ede5 and 0x1000ee7c (L_MakeRec) and 0x1000ee4e
//      (L_NoTexture).  In the source above those blocks are written
//      where the labels are, which is textually BEFORE REGION 3 and
//      REGION 5.  If our build does not sink them the alignment will
//      report an equal number of shipped-only and ours-only rows with
//      the totals matching -- up to four each, of which two would be the
//      DIRECT 0x10039bd0 pair.  That is the pattern 0x10061a30 recorded
//      (29 against 29, all odd rows placement), and it is not a decode
//      error.  If our build does sink them, expect 13 of 13.
//
//   b. NO CRT-INTRINSIC GAP HERE.  Nothing in this function reaches
//      0x1007fc90, 0x1007fc98 or 0x1007fdf4, so the sqrt/fabs/tan
//      residual cannot appear.
//
//   c. NO /Qip TWO-ENTRY OFFSET on the direct calls.  The shipped call
//      goes to 0x10039bd0, which IS the stack entry (the body is at
//      0x10039bdc), and to 0x1000daa0, which is a register-only entry
//      with no separate stack front.  Both are the addresses we name.
//
//   d. WHAT WILL NOT MATCH AND CANNOT: the ARGUMENT SETUP of the two
//      0x1000daa0 calls.  That callee takes three arguments in
//      %eax/%edx/%ecx and one in st(0); a __cdecl declaration pushes six
//      slots instead.  The rule is to decode it as an
//      ordinary call and note it, which is what is done.  The call
//      itself is in the right place with the right target.
//
// ======================================================================= //
