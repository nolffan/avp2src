// *********************************************************************** //
//
//  big7cc90.cpp -- d3d.ren 0x1007cc90 .. 0x1007d620, 2,448 bytes.
//
//  Reconstructed to the "structurally decoded" standard.  Not byte-exact; every field offset,
//  constant, bound and branch below is read off the shipped bytes and
//  cited by address.
//
//  Built /Os /Qip (build/engine.sh D3D_QIP_OS_TUS), which gives the
//  shipped two-entry pair: `sub_1007cc90`, the 4-byte stack entry,
//  byte-identical at 0x1007cc90, and `sub_1007cc90$`, the register entry
//  and body, NEAR at 0x1007cc94 -- 2,424 bytes against the shipped
//  2,439 (0x1007cc94..0x1007d61b), frame 0x50 as shipped, all 6 calls in
//  agreement (tools/callseq.py), the first 272 bytes and the whole
//  descent from 0x1007d546 on byte-exact.  What is left is x87 scheduling
//  inside the two MatMuls and one register swap in REGION 5 -- see
//  MEASURED at the bottom.
//
//  ===================================================================== //
//  THE EXTENT -- CONFIRMED AT BOTH ENDS
//  ===================================================================== //
//
//  The function enumeration carries `0x1007cc94, 2448`, and that row
//  is wrong at BOTH ends.  The extent 0x1007cc90, 2448, walked from the bytes:
//
//      0x1007cc90..0x1007d620  2,448 bytes  652 instructions
//      ret               1                 0x1007d5b1
//      push %ebp frames  1                 0x1007cc94
//      /Qip              start is the STACK entry; register entry at
//                        0x1007cc94
//      branch targets    0 outside the range or mid-instruction
//      last instruction  ends flush at the stated end, BUT the final 5
//                        bytes are alignment padding -- the body ends at
//                        0x1007d61b
//
//  One ret, one prologue, no strays, no overlap -- exactly as predicted.
//
//    * BOTTOM.  0x1007cc90 is the /Qip STACK entry (`mov 0x4(%esp),%eax`,
//      four bytes) and it falls into the REGISTER entry at 0x1007cc94.
//      The eight bytes before it (0x1007cc88..0x1007cc8f) are `nop`
//      padding and 0x1007cc86 is the `mov %esi,%esi` tail of the previous
//      function, so nothing is being annexed.
//    * TOP.  The last four bytes of the enumerated row, 0x1007d61c..
//      0x1007d61f, are `nop` padding; 0x1007d620 is the NEXT function's
//      /Qip stack entry (`mov 0x4(%esp),%eax`) falling into its prologue
//      at 0x1007d624 (`push %ebp ; mov %esp,%ebp ; sub $0x70,%esp`).
//
//  So the extent is 0x1007cc90..0x1007d620 -- the same 2,448 bytes,
//  shifted four bytes down.  The body proper ends at 0x1007d61b and the
//  five bytes 0x1007d61b..0x1007d61f are `nop` pads.
//
//  Note: the enumeration's 0x1007d624 /
//  591-byte row for the NEXT function is short at ITS bottom for the same
//  reason -- that function starts at 0x1007d620.
//
//  ===================================================================== //
//  WHAT THE FUNCTION IS
//  ===================================================================== //
//
//  `void __thiscall D3DRenderInfo::sub_1007cc90(uint32 nNodeIndex,
//                                               LTMat *pParentXForm)`
//
//  The model-node hierarchy transform pass: for one skeleton node it
//  blends the active animations into a local rotation and position, turns
//  that into a matrix, concatenates it with the parent's matrix and with
//  the node's base-pose matrix, writes the result into the model's node
//  matrix array, fires the per-node callback, and then descends into the
//  node's children -- recursing on all but the last and tail-iterating on
//  the last.
//
//  It is NOT container or CRT code despite sitting high in .text next to
//  them; it is graphics.  0x10091d40 is the only global it touches and
//  every one of the 62 references to 0x10091d40..0x10091d7f in the whole
//  of d3d.ren's .text is inside this function, so that global is a
//  file-scope scratch matrix private to this TU.
//
//  THE SIGNATURE, read off the two call sites (both same-TU):
//
//    * 0x1007ca9e, in sub_1007ca60 (84 bytes, already reconstructed in
//      src/engine/d3d/d3dr4a.cpp:1001 where it is written up as
//      `sub_1007cc94(m_p1a4->m_p2c0->m_nWord10, m_nValuee4)`):
//
//          mov    0x1a4(%ebx),%eax     ; the model
//          mov    0x2c0(%eax),%eax     ; ... its root node
//          mov    0xe4(%ebx),%edx      ; m_pRootXForme4
//          movzwl 0x10(%eax),%eax      ; ... the root node's WORD index
//          mov    %eax,(%esp)          ; arg1 = node index
//          mov    %edx,0x4(%esp)       ; arg2 = the root transform
//          mov    %ebx,%ecx            ; this
//          call   0x1007cc94
//
//      d3dr4a.cpp types arg2 as a bare `uint32 m_nValuee4`; the present
//      function proves it is an `LTMat*` -- it is the left operand of the
//      first 4x4 concatenation.  big17c14.cpp:479 already had it right
//      (`LTMat* m_pMate4;  // +0x0e4`).
//
//    * 0x1007d5ee, this function's own recursive call (see REGION 7).
//
//  `ret $0x8` with `this` in %ecx: __thiscall, two dword arguments, void
//  return (no epilogue path sets %eax).  arg1 arrives from a `uint16`
//  field zero-extended by the caller, so the parameter itself is uint32
//  (it is used as an index scaled by 4, 0x1c and 0x40).
//
//  ===================================================================== //
//  WHAT PINS THE DECODE -- THE CENSUS, AND IT HAS NO RESIDUE
//  ===================================================================== //
//
//  6 calls in 2,448 bytes is 408 bytes per call, which puts the call
//  sequence in the WEAK band and something else has to pin it.
//  The instruction census does, completely.  All 652 instructions:
//
//      mov    176    flds  129    fmuls 127    faddp 96    fstps 32
//      fxch    20    add    12    lea     7    call   6    test   5
//      push     5    nop     5    je      5    pop    4    jmp    3
//      inc      3    cmp     3    movzwl  2    movl   2    jbe    2
//      jb       2    xor     1    sub     1    shl    1    ret    1
//      imul     1    fmulp   1
//
//  The floating-point half decomposes EXACTLY, with nothing spare:
//
//      fmuls 127 + fmulp 1  = 128 = 2 MatMuls x 16 elements x 4 products
//      faddp            96  =  96 = 2 x 16 x 3 adds per element
//      fstps            32  =  32 = 2 x 16 stores
//      flds            129  = 128 = 2 x 16 x 4 loads, PLUS ONE:
//                                   the single element that used `fmulp`
//                                   instead of `fmuls mem` needed a fifth
//                                   `flds` for the operand it could not
//                                   fold (0x1007d250 / 0x1007d25f, the
//                                   dest[0][1] element of MatMul 2).
//
//  There is not one fdiv, fsub, fcom, fistp, fabs, fsqrt, fstpl, fildl or
//  `rep movs` in the range.  So: two 4x4 concatenations and NOTHING ELSE
//  floating-point.  A third matrix product, a missing element, a vector
//  normalise, a homogeneous divide, a lerp or a single extra float
//  constant would move at least three of those five rows.
//
//  The integer half decomposes just as exactly, which is what pins the
//  control flow in the call-free stretches:
//
//      test 5  = the five conditions: `flags12 & 2` (0x1007cd5a),
//                m_pfnNodeCallbackec (0x1007d54c), m_pIndexStackf8
//                (0x1007d57f), m_nIndexStackCountfc (0x1007d589),
//                pNode->m_nNumChildren20 (0x1007d5ba)
//      cmp  3  = `cmp $1,%edx` (the anim-loop guard, 0x1007ccf2) + the two
//                loop back-edge compares (0x1007cd1a, 0x1007d5f7)
//      jb   2  = the two loop back edges, and no others -- so there are
//                exactly TWO loops in the function
//      je   5  = one per `test` except the m_nfc one, which is `jbe`
//      jbe  2  = the anim-loop guard and `m_nfc > 0`
//      jmp  3  = over the flags12 else-arm (0x1007cd7c) and the two
//                continues (0x1007d5a6, 0x1007d619)
//      inc  3  = the `1` materialised at 0x1007cce8 + the two loop bumps
//      shl  1  = nNodeIndex * 0x40   (sizeof(LTMat), 0x1007ccc6)
//      imul 1  = nNodeIndex * 0x1c   (sizeof the base-pose record,
//                0x1007d0a0)
//      movzwl 2 = the two `pChild->m_nIndex10` word reads (0x1007d5e1,
//                0x1007d612) -- TWO, which is exactly the peeled-last-child
//                shape and not a single loop
//      movl 2  = `movl $0x0,(%esp)` (the literal 0 handed to sub_1007da80)
//                and `movl $0x1,0xfc(%edi)` (0x1007d601)
//      lea  7  = &m_Rot188 twice (0x1007ccb5, 0x1007cd36), &m_Vec198,
//                &m_Mat148, &m_Mat104, &pXForm->m_Rot0c, and `lea -1`
//      xor  1  = the zero that `inc` turns into the anim loop's `i = 1`
//      add 12  = six argument-area adjustments (four `add $-0x10/-0x8` for
//                the call frames plus `add $8` twice and `add $0x10`
//                once), plus `add %esi,%eax`, `add %ebx,%eax`,
//                `add %esi,%eax` (the nChildren-1)
//      sub 1 / push 5 / pop 4 / ret 1 / nop 5 = frame and padding
//
//  Nothing is left over in either half.  That, not the six calls, is the
//  evidence for this reconstruction.
//
//  CALL-FREE COVERAGE:
//      2,416 of 2,448 bytes (98.7%) lie in call-free stretches.  Only the
//      27 bytes of the six call instructions and the 5 bytes of padding
//      are outside one.  The stretches, by address:
//
//          0x1007cc90..0x1007cce1     81 B  prologue + arg setup
//          0x1007cce6..0x1007cd0e     40 B  anim-loop head
//          0x1007cd13..0x1007cd4c     57 B  loop bump + node fetch
//          0x1007cd51..0x1007d0ce    893 B  flags12 + MatMul 1   <-- big
//          0x1007d0d3..0x1007d574  1,185 B  MatMul 2 + callback   <-- big
//          0x1007d576..0x1007d5ee    120 B  the tail's branches
//          0x1007d5f3..0x1007d61b     40 B  loop bump + last child
//
//      The two 893- and 1,185-byte runs are the two MatMuls, and the
//      census above is what stands in for the call test there.
//
//  CALL SEQUENCE: 6 shipped, 6 ours, 6 in agreement -- the self-call at
//  0x1007d5ee goes to the REGISTER entry 0x1007cc94 on both sides now
//  that the file is built /Qip (it was the one predicted /Qip +4 pair
//  before).  See the RESIDUALS banner at the bottom.
//
//  /Qrcd: NOT NEEDED.  The census has zero float-to-int conversions --
//  no fistp of any width, no cvt, no byte lerps.  There is no colour
//  interpolation in this function.
//  /Oi-: NOT NEEDED.  The census has no CRT-intrinsic-shaped call and no
//  `rep movs`; the six calls are all accounted for.  The judgement is
//  based on what the census shows, not on the family's habit.
//  _chkstk: no residual.  The frame is `sub $0x50,%esp` = 80 bytes, well
//  under the probe threshold, and the shipped code emits no stack probe.
//
// *********************************************************************** //

// ----------------------------------------------------------------------- //
//  Measured: this function merged into
//  w10_1007cab4_os.cpp (one class, w10's LTRotation/LTVector types) and
//  built /Os /Qip /Qip_no_inlining, /Os /Oi- /Qip and /Os /Oi- /Qip
//  /Qip_no_inlining: 0x1007cab4 stays identical, the 0x1007cc90 stack entry
//  comes out identical, but the body's frame is STILL 0x58 (shipped 0x50)
//  -- having 0x1007d624 / 0x1007da80 in the TU does not keep m_pNodeMatse8
//  in %esi -- and /Qip gives 0x1007d880 and 0x1007da80 stack entries the
//  shipped image does not have (only 0x1007cab0/0x1007cc90/0x1007d620
//  exist; their callers 0x1007ca60 and 0x1007cc90 are the register-entry
//  users).  So the shipped TU is 0x1007ca60 + 0x1007cab4 + 0x1007cc90 +
//  0x1007d624 at /Qip, with 0x1007d880/0x1007da80 outside it.  Not kept.
// ----------------------------------------------------------------------- //
#include "ltbasedefs.h"


// ======================================================================= //
//  TYPES
//
//  LTMat and d3d_MatMul are ltmatrix.h:172's MatMul written out, because
//  BOTH sites here carry no call.  Copied VERBATIM from
//  src/engine/d3d/big66f08.cpp:100, which took it unchanged from
//  big19d64.cpp / big17c14.cpp.  It is not re-derived here -- but it IS
//  re-verified against these bytes:
//
//    * store order.  The 16 stores of MatMul 1 land at 0x10091d40 in the
//      address order  d40 d50 d60 d70 | d44 d54 d64 d74 | d48 d58 d68 d78 |
//      d4c d5c d6c d7c  (0x1007cdcb, 0x1007cdfb, 0x1007ce2b, 0x1007ce5b,
//      0x1007ce8a, ...).  With offset(r,c) = r*0x10 + c*4 that is
//      [0][0] [1][0] [2][0] [3][0] [0][1] ... -- the header's column-by-
//      column order.  MatMul 2's stores (0x1007d156 +0x00, 0x1007d192
//      +0x10, 0x1007d1d7 +0x20, 0x1007d222 +0x30, 0x1007d263 +0x04, ...)
//      are in the identical order, which is why both sites are ONE helper.
//    * term order.  MatMul 1's dest[0][0] at 0x1007cda2 emits
//      pMat1[0][3]*pMat2[3][0] first, then [0][2]*[2][0], then [0][1]*[1][0],
//      then [0][0]*[0][0] -- k = 3,2,1,0.  An FP sum emits its
//      terms backwards: a flat four-term sum written k = 0,1,2,3 emits
//      exactly that, so the helper's flat-sum spelling is confirmed rather
//      than assumed.
//    * operand roles.  dest[r][c] = sum_k pMat1[r][k] * pMat2[k][c].
//      MatMul 1 indexes %ebx (the argument) as [0][0..3] and 0x148(%edi)
//      as [0..3][0], so pMat1 = pParentXForm and pMat2 = &m_Mat148.
// ======================================================================= //
struct LTMat
{
	float  m[4][4];
};

struct LTVec
{
	float  x, y, z;
};

//  +0x188 .. +0x197.  Sixteen bytes handed to sub_1007da80 as its third
//  argument and to sub_10073400 as its first; 0x10073400 is already
//  established across four files (d3dbatchd.cpp:714, bige0e0.cpp:488,
//  d3dr3b.cpp:562) as quaternion-to-matrix, __cdecl, (rotation, matrix).
struct LTQuat
{
	float  x, y, z, w;
};

#define M1 pMat1->m
#define M2 pMat2->m

inline void d3d_MatMul(LTMat *pDest, LTMat *pMat1, LTMat *pMat2)
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

//  ltmatrix.h's Mat_SetTranslation(mat, vec) -> LTMatrix::SetTranslation(
//  LTVector vec), which takes the vector BY VALUE.  The copy is what the
//  bytes show: REGION 3's then-arm loads all three of pNode's dwords
//  (0x1007cd61..0x1007cd67) before the first store (0x1007cd6a), which
//  three plain assignments through a pointer that may alias m_Mat148
//  cannot do -- they came out load/store/load/store, two more frame
//  slots were needed and the frame was 0x58 against the shipped 0x50.
//  With the by-value copy the frame is 0x50 and the arm is byte-exact.
inline void d3d_SetTranslation(LTMat &mat, LTVec vec)
{
	mat.m[0][3] = vec.x;
	mat.m[1][3] = vec.y;
	mat.m[2][3] = vec.z;
}


// ======================================================================= //
//  THE SKELETON NODE, reached as m_pModel1a4->m_ppNodes28[nNodeIndex]
//  (0x1007cd24..0x1007cd33).  Four fields are read here and every one is
//  cited:
//
//      +0x04 +0x08 +0x0c   three consecutive dwords moved as INTEGERS
//                          into m_Mat148[0][3] / [1][3] / [2][3]
//                          (0x1007cd61..0x1007cd76).  A float triple
//                          copied componentwise off a kept base register,
//                          so three float assignments, not a vector copy.
//      +0x10               `movzwl 0x10(%eax),%eax` at 0x1007d5e1 and
//                          0x1007d612 -- a uint16 node index, and the
//                          same field sub_1007ca60 reads off the root
//                          node at 0x1007ca9a.
//      +0x12               `mov 0x12(%eax),%al ; test $0x2,%al`
//                          (0x1007cd57).  A byte-wide flag load.  Both a
//                          `uint8` and a `uint16` field narrow to
//                          `mov mem,%al` under either compiler
//                          ("Two movzx passes", case A), so
//                          the width is NOT pinned by these bytes;
//                          declared uint8 with the next byte padded.
//      +0x1c               `mov 0x1c(%esi),%eax ; mov (%eax,%ebx,4),%eax`
//                          (0x1007d5d5) -- an array of child NODE
//                          POINTERS, not of indices: the loaded dword is
//                          immediately dereferenced at +0x10.
//      +0x20               `mov 0x20(%eax),%esi` (0x1007d5b7), compared
//                          with `test`/`je` and used as an unsigned bound
//                          (`add $-1` then `jb`), so uint32.
// ======================================================================= //
struct D3DModelNode
{
	uint8           m_pad_00[0x04];                 // +0x00
	LTVec           m_vPos04;                       // +0x04
	uint16          m_nIndex10;                     // +0x10
	uint8           m_nFlags12;                     // +0x12
	uint8           m_pad_13[0x1c - 0x13];          // +0x13
	D3DModelNode**  m_ppChildren1c;                 // +0x1c
	uint32          m_nNumChildren20;               // +0x20
};


// ======================================================================= //
//  THE BASE-POSE TRANSFORM RECORD -- 0x1c bytes, and the size is read off
//  `imul $0x1c,%eax,%ebx` at 0x1007d0a0 with nNodeIndex in %eax.  The two
//  fields:
//
//      +0x00 +0x04 +0x08   `mov (%eax,%ebx,1),%esi` / `0x4(...)` /
//                          `0x8(...)` at 0x1007d0db..0x1007d100, stored
//                          into m_Mat104[0][3] / [1][3] / [2][3] -- an
//                          LTVec.
//      +0x0c               `lea 0xc(%ebx,%eax,1),%ecx` at 0x1007d0bb,
//                          handed to sub_10073400 as the rotation -- so
//                          16 bytes of quaternion.
//
//  12 + 16 = 0x1c exactly, with no hole.  The same 0x1c stride and the
//  same +0x00/+0x0c split appear inside sub_1007da80 at 0x1007dacc /
//  0x1007dacf, which is the independent confirmation.
// ======================================================================= //
struct D3DNodeXForm
{
	LTVec   m_vPos00;                               // +0x00
	LTQuat  m_Rot0c;                                // +0x0c
};


// ======================================================================= //
//  THE BASE-POSE SET at +0x144.  sub_1007cab4 (0x1007cbee..0x1007cbf2)
//  sets it from
//      m_pModel1a4->m_p168[ m_Anims[0].m_nAnim04 * 0x20 ]->+0x04
//  and this function reads only its +0x04:
//      mov 0x144(%edi),%eax ; mov 0x4(%eax),%eax   (0x1007d0a9/0x1007d0b5)
//  then indexes that by nNodeIndex * 0x1c.  So +0x04 is a per-node array
//  of D3DNodeXForm, NOT an array of pointers (contrast sub_1007da80's
//  0x208[i]->+0x04, which IS an array of pointers -- a different type at
//  the same offset of a different object).
// ======================================================================= //
struct D3DBasePose
{
	uint8           m_pad_00[0x04];                 // +0x00
	D3DNodeXForm*   m_pNodeXForms04;                // +0x04
};


// ======================================================================= //
//  THE MODEL at +0x1a4.  Only +0x28 is read here; the rest of the offsets
//  in the comment are from sub_1007cab4 and sub_1007da80 and are listed
//  only so the next reader does not have to re-derive them.
//
//      +0x28   D3DModelNode** -- indexed by node index
//              (0x1007cd2d/0x1007cd33)
//      +0x48   array read by sub_1007cab4 at 0x1007cbae
//      +0x4c   a bound compared against m_Anims[i].+0x18 (0x1007cb08)
//      +0x68   ...
//      +0x74   the default node-matrix array, copied into +0x0e8 when the
//              caller left that null (0x1007cc03)
//      +0x168  the 0x20-byte animation table (0x1007cb5d, 0x1007db6c)
//      +0x2c0  the root node, read by sub_1007ca60 (0x1007ca92)
// ======================================================================= //
struct D3DModel
{
	uint8           m_pad_000[0x28];                // +0x000
	D3DModelNode**  m_ppNodes28;                    // +0x028
	uint8           m_pad_02c[0x2c4 - 0x2c];        // +0x02c
};


// ======================================================================= //
//  THE OWNER.  Named D3DRenderInfo to match big17c14.cpp:473, which
//  models the same class from the other side (it constructs one at
//  -0x68c of its frame, writes +0xe4, +0xec and +0xf4 and hands it to
//  sub_1007ca60).  Everything below +0xe0 is the eight 0x1c-byte
//  animation records the constructor 0x10015950 walks
//  (`mov $0x8,%edi ; ... add $0x1c,%ebx ; dec %edi ; jne`) -- 8 * 0x1c =
//  0xe0 exactly, which is why the scalars start at +0xe0.
//
//  Offsets THIS function uses, each cited:
//
//   +0x0e0  uint32 m_nNumAnims.  `cmp $0x1,%edx ; jbe` (0x1007ccf2) and
//           `cmp %eax,%ebx ; jb` (0x1007cd1a).  Unsigned.  Zeroed by the
//           constructor at 0x100159ba; sub_1007cab4 rejects 0 and >8.
//   +0x0e4  LTMat* -- not read here; it is what arg2 IS at the
//           0x1007ca9e call site.
//   +0x0e8  LTMat* m_pNodeMatse8.  `mov 0xe8(%edi),%esi` (0x1007ccac)
//           then `shl $0x6,%eax ; add %esi,%eax` -- a stride of 0x40, so
//           an array of LTMat.  sub_1007cab4 defaults it from the model's
//           +0x74 at 0x1007cc06.
//   +0x0ec  the per-node callback, `call *%eax` (0x1007d574) after a null
//           test.  __cdecl, four dword arguments (the caller cleans up
//           with `add $0x10,%esp` at 0x1007d576).
//   +0x0f0  the callback's FOURTH argument (0x1007d56a -> 0xc(%esp)).
//   +0x0f4  the callback's FIRST argument (0x1007d556 -> (%esp)).
//           big17c14.cpp:483 types it `D3DObject*`.
//   +0x0f8  uint32* -- indexed `-0x4(%ecx,%eax,4)` (0x1007d59c), so an
//           array of dwords used as node indices.  SEMANTICS NOT PINNED;
//           see COIN-FLIP (1).  sub_1007cab4 stores NULL into it
//           unconditionally at 0x1007cb3e, so in every traversal d3d.ren
//           actually performs this arm is dead.
//   +0x0fc  uint32, the count that goes with +0xf8.  Read, decremented
//           and stored (0x1007d583..0x1007d596) and set to 1 at
//           0x1007d601.  NOT initialised by the constructor.
//   +0x100  D3DNodeXForm* -- set to the record this pass used
//           (0x1007d0c4).  A published "current node" pointer.
//   +0x104  LTMat m_Mat104, 0x104..0x143.  All sixteen elements are
//           read by MatMul 2 (0x104/0x114/0x124/0x134, 0x108/0x118/
//           0x128/0x138, 0x10c/0x11c/0x12c/0x13c, 0x110/0x120/0x130/
//           0x140), which is what fixes it at 0x40 bytes and fixes the
//           gap to +0x144 at zero.
//   +0x144  D3DBasePose*.
//   +0x148  LTMat m_Mat148, 0x148..0x187.  All sixteen elements read by
//           MatMul 1.
//   +0x188  LTQuat m_Rot188, 0x188..0x197.  Written by sub_1007da80,
//           consumed by sub_10073400.
//   +0x198  LTVec m_Vec198, 0x198..0x1a3.  Written by sub_1007da80; its
//           three floats are the else-arm translation
//           (0x1007cd7e..0x1007cd9c).
//   +0x1a4  D3DModel*.
//
//  The three declared-but-not-defined members are deliberate: a tiny
//  accessor spelled as a field read makes no call at all.
//  None of them is an accessor here -- they are three real functions in
//  other parts of the image -- but leaving them undefined is what forces
//  icl to emit the calls instead of inlining anything.
// ======================================================================= //
struct D3DAnimRecord                                // 0x1c bytes
{
	uint8   m_pad_00[0x1c];
};

class D3DRenderInfo
{
public:
	//  0x1007da80, 336 bytes.  __thiscall, `ret $0x10`.  Blends
	//  animation iAnim's two keyframes for node nNodeIndex into
	//  *pRot / *pPos.  Its argument roles are read off its own body:
	//  0x50(%esp) indexes +0x208 and the 0x1c anim records, 0x54(%esp)
	//  indexes the per-node pointer array, 0x58(%esp) goes to
	//  0x10073500 as the rotation destination and 0x5c(%esp) receives
	//  the `rep movsl` ecx=3 vector.  Declared only.
	void    sub_1007da80(uint32 iAnim, uint32 nNodeIndex,
	                     LTQuat *pRot, LTVec *pPos);

	//  0x1007d620 (stack entry) / 0x1007d624 (register entry), 591
	//  bytes.  __thiscall, `ret $0x8`.  Blends one further animation
	//  into m_Rot188 / m_Vec198 -- its 0x1007d68e..0x1007d6af reads
	//  +0x188/+0x18c/+0x190 into a local and calls sub_1007da80 into
	//  another.  Argument roles from 0x1007d636 (`0x1e8(%ebx,%ecx,4)`,
	//  so arg1 is the animation) and 0x1007d646 (`(%eax,%edx,4)`, so
	//  arg2 is the node).  Declared only.
	void    sub_1007d624(uint32 iAnim, uint32 nNodeIndex);

	//  THIS function.
	void    sub_1007cc90(uint32 nNodeIndex, LTMat *pParentXForm);

	D3DAnimRecord   m_Anims[8];                     // +0x000 .. +0x0df
	uint32          m_nNumAnims;                    // +0x0e0
	LTMat*          m_pRootXForme4;                 // +0x0e4
	LTMat*          m_pNodeMatse8;                  // +0x0e8
	void            (*m_pfnNodeCallbackec)(void *pObject, uint32 nNodeIndex,
	                                       LTMat *pNodeMat, void *pUserData);
	void*           m_pUserDataf0;                  // +0x0f0
	void*           m_pObjectf4;                    // +0x0f4
	uint32*         m_pIndexStackf8;                // +0x0f8
	uint32          m_nIndexStackCountfc;           // +0x0fc
	D3DNodeXForm*   m_pCurXForm100;                 // +0x100
	LTMat           m_Mat104;                       // +0x104
	D3DBasePose*    m_p144;                         // +0x144
	LTMat           m_Mat148;                       // +0x148
	LTQuat          m_Rot188;                       // +0x188
	LTVec           m_Vec198;                       // +0x198
	D3DModel*       m_pModel1a4;                    // +0x1a4
};


//  0x10073400, 244 bytes, __cdecl (rotation, matrix) -- quaternion to
//  matrix.  Already declared in four of our files (d3dbatchd.cpp:715,
//  bige0e0.cpp:494, d3dr3b.cpp:563, d3dr2a.cpp); the same prototype is
//  reused here.  Both call sites push two pointers and clean up with
//  `add $0x8,%esp` (0x1007cd4c/0x1007cd54 and 0x1007d0ce/0x1007d0df),
//  which is what says __cdecl rather than __thiscall.
extern "C" void sub_10073400(LTQuat *pRot, LTMat *pMat);


// ======================================================================= //
//  THE FILE-SCOPE SCRATCH MATRIX at 0x10091d40 (.data+0x8d40).
//
//  0x40 bytes, and it is REALLY 0x40: the sixteen stores of MatMul 1 fill
//  0x10091d40..0x10091d7f and nothing outside this function refers to any
//  address in that range -- all 62 references to 0x10091d40..0x10091d7f in
//  the whole of d3d.ren's .text lie between 0x1007cdcb and 0x1007d4ee.
//
//  It is a static, not a local.  The 42 `mov $0x10091d40,%reg` in the
//  range (0x1007d11e, 0x1007d123, 0x1007d130, ...) are icl
//  re-materialising the global's ABSOLUTE ADDRESS into whichever register
//  is free, then spilling those registers to -0x04 .. -0x28 of the frame
//  when it runs out -- MatMul 2 needs a base register for every `fmuls
//  disp(%reg)` against it.  A frame local would be addressed off %ebp
//  instead and would need no immediate at all, so the immediate is the
//  proof that this is a file-scope object.
// ======================================================================= //
static LTMat    g_Mat10091d40;


// ======================================================================= //
//  THE FRAME -- `sub $0x50,%esp` plus one filler `push %edi`.
//
//      1007cc90  mov 0x4(%esp),%eax      <- the /Qip stack entry; %eax is
//                                           dead, the body reloads arg1
//                                           from 0x8(%ebp) at 0x1007cc9e
//      1007cc94  push %ebp
//      1007cc95  mov %esp,%ebp
//      1007cc97  sub $0x50,%esp
//      1007cc9a  push %edi               <- 4 bytes of filler, not a save
//      1007cc9b  push %edi
//      1007cc9c  push %esi
//      1007cc9d  push %ebx
//
//  0x54 bytes of locals, of which only four slots are source variables:
//
//      -0x30   uint32 nNodeIndex   the mutable copy of the parameter; the
//                                  loop head reloads it (0x1007cca9) and
//                                  both continues write it (0x1007d5a0,
//                                  0x1007d616)
//      -0x34   D3DModelNode* pNode                        (0x1007cd3c)
//      -0x38   LTMat*        pNodeMat                     (0x1007ccda)
//      -0x3c   scratch: the base-pose array base (0x1007d0b8) and the
//              child loop's bound (0x1007d5ca)
//
//  -0x2c holds nNodeIndex * 0x40 (0x1007cccf) -- icl's strength reduction
//  of the LTMat subscript, not a variable -- and -0x04 .. -0x28 are ten
//  spill slots for the re-materialised 0x10091d40 immediates and for %esi
//  and %ebx across the two loops.  The remaining 24 bytes of the 0x54 are
//  never touched.
//
//  So the frame witnesses FOUR source locals and no aggregate, which is
//  consistent with the two MatMuls writing straight to the global and to
//  the node array with no temporary matrix anywhere.  A staging matrix
//  would be 0x40 more bytes of frame and there is no room for one.
// ======================================================================= //

void D3DRenderInfo::sub_1007cc90(uint32 nNodeIndex, LTMat *pParentXForm)
{
	//  The back edge at 0x1007d5a6 and 0x1007d619 is an UNCONDITIONAL
	//  `jmp 0x1007cca9`, and 0x1007cca9 is the top of the body (it
	//  reloads nNodeIndex out of -0x30).  So the whole body is one
	//  unrotated loop with the exits inside it -- `for (;;)` with
	//  `return`s.  The two parameters are what the loop carries.
	for (;;)
	{
		D3DModelNode*   pNode;
		LTMat*          pNodeMat;

		// ------------------------------------------------------------- //
		//  REGION 1 -- 0x1007cca9 .. 0x1007cce6, the loop head.
		//
		//      1007cca9  mov -0x30(%ebp),%eax        nNodeIndex
		//      1007ccac  mov 0xe8(%edi),%esi         m_pNodeMatse8
		//      1007ccb2  add $0xfffffff0,%esp        4 argument slots
		//      1007ccb5  lea 0x188(%edi),%edx        &m_Rot188
		//      1007ccbb  movl $0x0,(%esp)            arg1 = 0
		//      1007ccc2  mov %eax,0x4(%esp)          arg2 = nNodeIndex
		//      1007ccc6  shl $0x6,%eax               * sizeof(LTMat)
		//      1007ccc9  lea 0x198(%edi),%ecx        &m_Vec198
		//      1007cccf  mov %eax,-0x2c(%ebp)
		//      1007ccd2  mov %ecx,0xc(%esp)          arg4
		//      1007ccd6  add %esi,%eax
		//      1007ccd8  mov %edi,%ecx               this
		//      1007ccda  mov %eax,-0x38(%ebp)        pNodeMat
		//      1007ccdd  mov %edx,0x8(%esp)          arg3
		//      1007cce1  call 0x1007da80
		//
		//  WHAT PINS IT: the `shl $0x6` is sizeof(LTMat), so
		//  m_pNodeMatse8 is an LTMat array and pNodeMat is
		//  &m_pNodeMatse8[nNodeIndex].  It is computed HERE, ~900 bytes
		//  before its first use, and spilled to a frame slot it keeps
		//  for the rest of the body -- a named local ("name a repeated
		//  address in a local"); it has four uses
		//  (MatMul 2's destination, the callback's third argument, the
		//  child recursion's second argument, and the next iteration's
		//  parent).
		//
		//  The literal `0` in arg1 is `movl $0x0,(%esp)` -- one of the
		//  census's two `movl`s -- so animation 0 is blended by this
		//  call and animations 1..n-1 by REGION 2's loop.
		// ------------------------------------------------------------- //
		pNodeMat = &m_pNodeMatse8[nNodeIndex];

		sub_1007da80(0, nNodeIndex, &m_Rot188, &m_Vec198);

		// ------------------------------------------------------------- //
		//  REGION 2 -- 0x1007cce6 .. 0x1007cd24, the extra animations.
		//
		//      1007cce6  xor %eax,%eax
		//      1007cce8  inc %eax                    i = 1
		//      1007cce9  mov 0xe0(%edi),%edx         m_nNumAnims
		//      1007ccef  mov %eax,-0x3c(%ebp)        (a dead spill of 1)
		//      1007ccf2  cmp $0x1,%edx
		//      1007ccf5  jbe 0x1007cd24              n <= 1: skip
		//      1007ccf7  mov %esi,-0x24(%ebp)        save m_pNodeMatse8
		//      1007ccfa  mov -0x30(%ebp),%esi        nNodeIndex
		//      1007ccfd  mov %ebx,-0x28(%ebp)        save pParentXForm
		//      1007cd00  mov %eax,%ebx               i
		//    L 1007cd02  add $0xfffffff8,%esp
		//      1007cd05  mov %edi,%ecx
		//      1007cd07  mov %esi,0x4(%esp)          arg2 = nNodeIndex
		//      1007cd0b  mov %ebx,(%esp)             arg1 = i
		//      1007cd0e  call 0x1007d624
		//      1007cd13  inc %ebx
		//      1007cd14  mov 0xe0(%edi),%eax         m_nNumAnims again
		//      1007cd1a  cmp %eax,%ebx
		//      1007cd1c  jb  0x1007cd02
		//      1007cd1e  mov -0x28(%ebp),%ebx
		//      1007cd21  mov -0x24(%ebp),%esi
		//
		//  WHAT PINS IT: `cmp $0x1` + `jbe` is the hoisted first-
		//  iteration test of `for (i = 1; i < m_nNumAnims; i++)` on an
		//  UNSIGNED bound (`jbe`/`jb`, never `jle`/`jl`), and the bound
		//  is RELOADED from +0xe0 on every trip (0x1007cd14) because the
		//  callee could have changed it -- so the source writes the
		//  member in the condition rather than caching it in a local.
		//  One `cmp` and one `jb` of the census's three and two.
		//
		//  The `mov %eax,-0x3c(%ebp)` at 0x1007ccef is a DEAD store of
		//  the 1: -0x3c is next written at 0x1007d0b8 and never read
		//  before that.  It is icl spilling the induction variable's
		//  initial value on the path where the loop is skipped.  Do not
		//  read it as a source statement.
		// ------------------------------------------------------------- //
		for (uint32 iAnim = 1; iAnim < m_nNumAnims; iAnim++)
			sub_1007d624(iAnim, nNodeIndex);

		// ------------------------------------------------------------- //
		//  REGION 3 -- 0x1007cd24 .. 0x1007cda2, the node and the local
		//  transform.
		//
		//      1007cd24  mov 0x1a4(%edi),%eax        m_pModel1a4
		//      1007cd2a  mov -0x30(%ebp),%edx        nNodeIndex
		//      1007cd2d  mov 0x28(%eax),%eax         m_ppNodes28
		//      1007cd30  add $0xfffffff8,%esp
		//      1007cd33  mov (%eax,%edx,4),%eax      [nNodeIndex]
		//      1007cd36  lea 0x188(%edi),%edx        &m_Rot188
		//      1007cd3c  mov %eax,-0x34(%ebp)        pNode
		//      1007cd3f  lea 0x148(%edi),%eax        &m_Mat148
		//      1007cd45  mov %edx,(%esp)
		//      1007cd48  mov %eax,0x4(%esp)
		//      1007cd4c  call 0x10073400             quat -> matrix
		//      1007cd54  add $0x8,%esp               __cdecl cleanup
		//      1007cd57  mov 0x12(%eax),%al          pNode->m_nFlags12
		//      1007cd5a  test $0x2,%al
		//      1007cd5c  je  0x1007cd7e
		//      ; then-arm, FALLS THROUGH:
		//      1007cd5e  mov -0x34(%ebp),%eax
		//      1007cd61  mov 0x4(%eax),%edx          pNode->m_vPos04.x
		//      1007cd64  mov 0x8(%eax),%ecx                       .y
		//      1007cd67  mov 0xc(%eax),%eax                       .z
		//      1007cd6a  mov %edx,0x154(%edi)        m_Mat148[0][3]
		//      1007cd70  mov %ecx,0x164(%edi)        m_Mat148[1][3]
		//      1007cd76  mov %eax,0x174(%edi)        m_Mat148[2][3]
		//      1007cd7c  jmp 0x1007cda2
		//      ; else-arm:
		//      1007cd7e  mov 0x198(%edi),%eax        m_Vec198.x
		//      1007cd84  mov 0x19c(%edi),%edx                 .y
		//      1007cd8a  mov 0x1a0(%edi),%ecx                 .z
		//      1007cd90  mov %edx,0x164(%edi)
		//      1007cd96  mov %ecx,0x174(%edi)
		//      1007cd9c  mov %eax,0x154(%edi)
		//
		//  WHAT PINS IT:
		//    * 0x154 / 0x164 / 0x174 are m_Mat148[0][3], [1][3] and
		//      [2][3] -- the translation COLUMN of a row-major 4x4, so
		//      this is the column-vector convention and `A * B` means
		//      "apply B, then A".  That is what fixes the operand order
		//      of both MatMuls below.
		//    * sub_10073400 runs BEFORE the translation stores, so it
		//      writes the whole matrix (bige0e0.cpp:917 records that it
		//      also writes row 3 as (0,0,0,1)) and the source then
		//      overwrites three elements of it.
		//    * the `test $0x2,%al` arm order.  The then-arm FALLS
		//      THROUGH and the else-arm is the branch target reached by
		//      `je`, so the source guard is POSITIVE:
		//      `if (pNode->m_nFlags12 & 2) {...} else {...}`.
		//      Neither arm makes a call, so this cannot move the call
		//      sequence; it is settled by the fall-through instead.
		//    * both arms store the same three matrix elements, so the
		//      `if` is over the SOURCE of the translation and not over
		//      the destination.  The element order differs between the
		//      arms (x,y,z against y,z,x) which is scheduling, not
		//      source: the else-arm's %eax is loaded first and spent
		//      last.
		//    * the then-arm does all three LOADS before the first store.
		//      Through pNode, which may alias m_Mat148, three plain
		//      assignments cannot be reordered that way; a by-value LTVec
		//      copy can.  So both arms are ltmatrix.h's
		//      Mat_SetTranslation, written d3d_SetTranslation (see TYPES).
		//      Measured: the three assignments gave load/store pairs and a
		//      0x58 frame; the helper gives this arm byte-exact and 0x50.
		// ------------------------------------------------------------- //
		pNode = m_pModel1a4->m_ppNodes28[nNodeIndex];

		sub_10073400(&m_Rot188, &m_Mat148);

		if (pNode->m_nFlags12 & 2)
		{
			d3d_SetTranslation(m_Mat148, pNode->m_vPos04);
		}
		else
		{
			d3d_SetTranslation(m_Mat148, m_Vec198);
		}

		// ------------------------------------------------------------- //
		//  REGION 4 -- 0x1007cda2 .. 0x1007d0a9, MatMul 1.  893 bytes,
		//  call-free, and the census is what pins it.
		//
		//  16 stores into 0x10091d40..0x10091d7c, in this address order:
		//
		//      1007cdcb d40   1007cdfb d50   1007ce2b d60   1007ce5b d70
		//      1007ce8a d44   1007ceba d54   1007ceea d64   1007cf1a d74
		//      1007cf49 d48   1007cf79 d58   1007cfa9 d68   1007cfd9 d78
		//      1007d010 d4c   1007d040 d5c   1007d070 d6c   1007d0a3 d7c
		//
		//  which is [0][0] [1][0] [2][0] [3][0] [0][1] ... -- exactly
		//  d3d_MatMul's store order.
		//
		//  The dest[0][0] element, decoded term by term (0x1007cda2):
		//
		//      flds  0xc(%ebx)      pParentXForm->m[0][3]
		//      fmuls 0x178(%edi)    * m_Mat148.m[3][0]
		//      flds  0x8(%ebx)      pParentXForm->m[0][2]
		//      fmuls 0x168(%edi)    * m_Mat148.m[2][0]
		//      faddp
		//      flds  0x158(%edi)    m_Mat148.m[1][0]
		//      fmuls 0x4(%ebx)      * pParentXForm->m[0][1]
		//      faddp
		//      flds  0x148(%edi)    m_Mat148.m[0][0]
		//      fmuls (%ebx)         * pParentXForm->m[0][0]
		//      faddp
		//      fstps 0x10091d40
		//
		//  so dest[r][c] = sum_k pParentXForm[r][k] * m_Mat148[k][c],
		//  emitted k = 3,2,1,0 -- the predicted flat-sum order.
		//  pMat1 is the ARGUMENT and pMat2 is m_Mat148.
		//
		//  %ebx is pParentXForm throughout: it is loaded from 0xc(%ebp)
		//  at 0x1007cca6, saved to -0x28 and restored around REGION 2's
		//  loop only, and reassigned only at the loop back edge
		//  (0x1007d5a3).
		//
		//  All sixteen elements of m_Mat148 are read (0x148/0x158/0x168/
		//  0x178, 0x14c/0x15c/0x16c/0x17c, 0x150/0x160/0x170/0x180,
		//  0x154/0x164/0x174/0x184) and all sixteen of pParentXForm
		//  (0x00..0x3c), so both operands are full 4x4s and neither is
		//  a 4x3.
		// ------------------------------------------------------------- //
		d3d_MatMul(&g_Mat10091d40, pParentXForm, &m_Mat148);

		// ------------------------------------------------------------- //
		//  REGION 5 -- 0x1007d0a0 .. 0x1007d116, the base-pose matrix.
		//
		//      1007d00a  mov -0x30(%ebp),%eax        nNodeIndex, hoisted
		//                                            into MatMul 1's tail
		//      1007d0a0  imul $0x1c,%eax,%ebx        * sizeof(D3DNodeXForm)
		//      1007d0a9  mov 0x144(%edi),%eax        m_p144
		//      1007d0af  lea 0x104(%edi),%edx        &m_Mat104
		//      1007d0b5  mov 0x4(%eax),%eax          m_pNodeXForms04
		//      1007d0b8  mov %eax,-0x3c(%ebp)
		//      1007d0bb  lea 0xc(%ebx,%eax,1),%ecx   &pXForm->m_Rot0c
		//      1007d0bf  add %ebx,%eax               pXForm
		//      1007d0c1  mov %ecx,(%esp)
		//      1007d0c4  mov %eax,0x100(%edi)        m_pCurXForm100 =
		//      1007d0ca  mov %edx,0x4(%esp)
		//      1007d0ce  call 0x10073400             quat -> matrix
		//      1007d0df  add $0x8,%esp
		//      1007d0db  mov 0x8(%eax,%ebx,1),%ecx   pXForm->m_vPos00.z
		//      1007d0fd  mov (%eax,%ebx,1),%esi                     .x
		//      1007d100  mov 0x4(%eax,%ebx,1),%eax                  .y
		//      1007d104  mov %ecx,0x130(%edi)        m_Mat104[2][3]
		//      1007d10a  mov %eax,0x120(%edi)        m_Mat104[1][3]
		//      1007d110  mov %esi,0x110(%edi)        m_Mat104[0][3]
		//
		//  WHAT PINS IT: the 0x1c multiplier fixes the record size, the
		//  +0xc offset handed to the quaternion converter fixes the
		//  rotation's place in it, and 0x110/0x120/0x130 are the
		//  translation column of m_Mat104 -- the same shape as REGION 3
		//  but on the other matrix.  The `imul` is the census's only
		//  one, so there is exactly one 0x1c-strided array in the
		//  function.
		//
		//  The store to +0x100 sits BEFORE the call in the emission
		//  (0x1007d0c4 against 0x1007d0ce), so it is a statement before
		//  the call and not a by-product of one.
		//
		//  The three float moves come out z, y, x in the emission
		//  because %ecx is loaded before the `add $0x8,%esp` and %esi /
		//  %eax after it; written x, y, z here.  Nothing distinguishes
		//  the source order -- see COIN-FLIP (3).
		// ------------------------------------------------------------- //
		{
			D3DNodeXForm*   pXForm = &m_p144->m_pNodeXForms04[nNodeIndex];

			m_pCurXForm100 = pXForm;

			sub_10073400(&pXForm->m_Rot0c, &m_Mat104);

			d3d_SetTranslation(m_Mat104, pXForm->m_vPos00);
		}

		// ------------------------------------------------------------- //
		//  REGION 6 -- 0x1007d116 .. 0x1007d546, MatMul 2.  The other
		//  half of the 1,185-byte call-free run, and the census's other
		//  64 multiplies / 48 adds / 16 stores.
		//
		//  16 stores into (%esi,%edx,1) where %esi is m_pNodeMatse8
		//  (restored from -0x20 at 0x1007d153) and %edx is
		//  nNodeIndex * 0x40 (-0x2c), in this address order:
		//
		//      1007d156 +0x00   1007d192 +0x10   1007d1d7 +0x20
		//      1007d222 +0x30   1007d263 +0x04   1007d2ad +0x14
		//      1007d301 +0x24   1007d345 +0x34   1007d380 +0x08
		//      1007d3c0 +0x18   1007d3fb +0x28   1007d443 +0x38
		//      1007d48f +0x0c   1007d4d1 +0x1c   1007d509 +0x2c
		//      1007d542 +0x3c
		//
		//  identical to MatMul 1's order, which is why the two sites are
		//  one helper and not two hand-written blocks.
		//
		//  The dest[0][0] element, from an x87 stack simulation of
		//  0x1007d0e2..0x1007d156:
		//
		//      flds 0x134(%edi) 0x124(%edi) 0x114(%edi) 0x104(%edi)
		//                              m_Mat104[3][0] [2][0] [1][0] [0][0]
		//      fmuls 0x10091d40        st0 = m104[0][0] * g[0][0]   = P0
		//      fxch %st(3)
		//      fmuls 0xc(%edx)         st0 = m104[3][0] * g[0][3]   = P3
		//      fxch %st(2)
		//      fmuls 0x8(%eax)         st0 = m104[2][0] * g[0][2]   = P2
		//      faddp %st,%st(2)        P3 += P2
		//      fmuls 0x4(%ecx)         st0 = m104[1][0] * g[0][1]   = P1
		//      faddp %st,%st(1)        P1 += (P3+P2)
		//      faddp %st,%st(1)        P0 += that
		//      fstps (%esi,%edx,1)
		//
		//  = sum_k g_Mat10091d40[0][k] * m_Mat104[k][0], so
		//  pMat1 = &g_Mat10091d40 and pMat2 = &m_Mat104.  All sixteen
		//  elements of m_Mat104 are read (0x104/0x114/0x124/0x134,
		//  0x108/0x118/0x128/0x138, 0x10c/0x11c/0x12c/0x13c,
		//  0x110/0x120/0x130/0x140) and all sixteen of the global.
		//
		//  So the whole arithmetic of the function is
		//
		//      m_pNodeMatse8[nNodeIndex]
		//          = pParentXForm * m_Mat148 * m_Mat104
		//
		//  with the column-3 translation convention REGION 3 pinned:
		//  the node's base-pose transform is applied first, then the
		//  blended animation transform, then the parent's.
		//
		//  All 42 `mov $0x10091d40,%reg` immediates in the function are
		//  in this region (0x1007d0d6 .. 0x1007d4ee), and so are most of
		//  the nineteen spill stores to -0x04..-0x28.  They are icl
		//  running out of registers to hold the global's address in --
		//  MatMul 1 needed none because its left operand arrived in %ebx
		//  and it could address the global with a direct `fstps
		//  0x10091dNN`, while MatMul 2 has to READ the global sixteen
		//  times four and every read wants a base register.  They carry no
		//  source content.  The one `fmulp` of the census is at
		//  0x1007d25f, in the dest[0][1] element, where the extra
		//  `flds 0x10091d40` at 0x1007d250 was needed because no
		//  register was left to address the global with.
		// ------------------------------------------------------------- //
		d3d_MatMul(pNodeMat, &g_Mat10091d40, &m_Mat104);

		// ------------------------------------------------------------- //
		//  REGION 7a -- 0x1007d546 .. 0x1007d579, the per-node callback.
		//
		//      1007d546  mov 0xec(%edi),%eax
		//      1007d54c  test %eax,%eax
		//      1007d54e  je  0x1007d579
		//      1007d550  add $0xfffffff0,%esp        4 argument slots
		//      1007d553  mov -0x38(%ebp),%edx        pNodeMat
		//      1007d556  mov 0xf4(%edi),%ecx         m_pObjectf4
		//      1007d55c  mov %edx,0x8(%esp)          arg3
		//      1007d560  mov -0x30(%ebp),%edx        nNodeIndex
		//      1007d563  mov %ecx,(%esp)             arg1
		//      1007d566  mov %edx,0x4(%esp)          arg2
		//      1007d56a  mov 0xf0(%edi),%edx         m_pUserDataf0
		//      1007d570  mov %edx,0xc(%esp)          arg4
		//      1007d574  call *%eax
		//      1007d576  add $0x10,%esp              __cdecl cleanup
		//
		//  WHAT PINS IT: four dword slots written and a caller-side
		//  `add $0x10,%esp`, so __cdecl with four arguments; the slot
		//  each value lands in fixes the order unambiguously.  This is
		//  the census's only indirect call and its only `add $0x10`.
		// ------------------------------------------------------------- //
		if (m_pfnNodeCallbackec)
			m_pfnNodeCallbackec(m_pObjectf4, nNodeIndex, pNodeMat,
			                    m_pUserDataf0);

		// ------------------------------------------------------------- //
		//  REGION 7b -- 0x1007d579 .. 0x1007d61b, the descent.
		//
		//      1007d579  mov 0xf8(%edi),%eax         m_pIndexStackf8
		//      1007d57f  test %eax,%eax
		//      1007d581  je  0x1007d5b4              null: the children
		//      ; the +0xf8 arm:
		//      1007d583  mov 0xfc(%edi),%eax         m_nIndexStackCountfc
		//      1007d589  test %eax,%eax
		//      1007d58b  jbe 0x1007d5ab              <- JBE, not JE
		//      1007d58d  lea -0x1(%eax),%edx
		//      1007d590  mov 0xf8(%edi),%ecx
		//      1007d596  mov %edx,0xfc(%edi)         count = count - 1
		//      1007d59c  mov -0x4(%ecx,%eax,4),%ecx  stack[count - 1]
		//      1007d5a0  mov %ecx,-0x30(%ebp)        nNodeIndex =
		//    C 1007d5a3  mov -0x38(%ebp),%ebx        pParentXForm =
		//      1007d5a6  jmp 0x1007cca9                          pNodeMat
		//    R 1007d5ab  pop %ebx ; pop %esi ; pop %edi
		//      1007d5ae  mov %ebp,%esp ; pop %ebp ; ret $0x8
		//      ; the children arm:
		//      1007d5b4  mov -0x34(%ebp),%eax        pNode
		//      1007d5b7  mov 0x20(%eax),%esi         m_nNumChildren20
		//      1007d5ba  test %esi,%esi
		//      1007d5bc  je  0x1007d5ab              <- JE, not JBE
		//      1007d5be  mov $0xffffffff,%eax
		//      1007d5c3  add %esi,%eax               nChildren - 1
		//      1007d5c5  mov $0x0,%ebx               i = 0
		//      1007d5ca  mov %eax,-0x3c(%ebp)
		//      1007d5cd  je  0x1007d5fe              nChildren-1 == 0
		//      1007d5cf  mov %esi,-0x28(%ebp) ; mov -0x34(%ebp),%esi
		//    L 1007d5d5  mov 0x1c(%esi),%eax         m_ppChildren1c
		//      1007d5d8  add $0xfffffff8,%esp
		//      1007d5db  mov (%eax,%ebx,4),%eax      [i]
		//      1007d5de  mov -0x38(%ebp),%edx        pNodeMat
		//      1007d5e1  movzwl 0x10(%eax),%eax      ->m_nIndex10
		//      1007d5e5  mov %edx,0x4(%esp)          arg2
		//      1007d5e9  mov %edi,%ecx               this
		//      1007d5eb  mov %eax,(%esp)             arg1
		//      1007d5ee  call 0x1007cc94             RECURSE
		//      1007d5f3  inc %ebx
		//      1007d5f4  mov -0x3c(%ebp),%eax
		//      1007d5f7  cmp %eax,%ebx
		//      1007d5f9  jb  0x1007d5d5
		//      1007d5fb  mov -0x28(%ebp),%esi
		//      1007d5fe  mov -0x34(%ebp),%eax        pNode
		//      1007d601  movl $0x1,0xfc(%edi)        count = 1
		//      1007d60b  mov 0x1c(%eax),%eax         m_ppChildren1c
		//      1007d60e  mov -0x4(%eax,%esi,4),%eax  [nChildren - 1]
		//      1007d612  movzwl 0x10(%eax),%eax      ->m_nIndex10
		//      1007d616  mov %eax,-0x30(%ebp)        nNodeIndex =
		//      1007d619  jmp 0x1007d5a3              -> C
		//
		//  WHAT PINS IT:
		//
		//    * TWO exits, TWO comparison spellings, and they differ in
		//      the bytes.  0x1007d58b is `jbe` and 0x1007d5bc is `je`
		//      off identical `test %eax,%eax` / `test %esi,%esi`
		//      instructions.  `while (n > 0)` and
		//      `while (n)` are different on an unsigned -- the first
		//      gives `jbe`, the second `je`.  Not interchangeable.  So
		//      the count test is written with a RELATIONAL operator and
		//      the child-count test with an EQUALITY one, and that is
		//      read off the bytes rather than chosen.  Writing both the
		//      same way loses one of the two.
		//
		//    * the recursion is over children 0..nChildren-2 and the
		//      LAST child is tail-iterated.  Three independent things
		//      say so and none of them is an inference about what icl
		//      might have done:
		//        - the loop bound in -0x3c is `nChildren - 1`
		//          (0x1007d5c3/0x1007d5ca), not nChildren;
		//        - there are TWO `movzwl 0x10(%eax)` in the whole
		//          function (0x1007d5e1 and 0x1007d612) and the census
		//          has exactly two, so the last child's index is read
		//          by its own statement and not by a loop iteration;
		//        - 0x1007d60e indexes `-0x4(%eax,%esi,4)` with %esi
		//          still holding nChildren, i.e. `m_ppChildren1c[
		//          nChildren - 1]` written out.
		//      An icl-invented tail-recursion elimination would explain
		//      the jump but not the `movl $0x1,0xfc(%edi)` store that
		//      sits between the loop and it -- a member write cannot be
		//      invented by the compiler.
		//
		//    * the shared epilogue at 0x1007d5ab is hosted BETWEEN the
		//      two arms, reached forward from 0x1007d58b and backward
		//      from 0x1007d5bc.  Measured: both exits written `return`
		//      put the epilogue at the END, after the last-child `jmp`,
		//      with the child-count exit going through a second `jmp`
		//      to it; written `break` out of the `for (;;)`, icl hosts
		//      it between the arms exactly as shipped.
		//
		//    * `nLast` is a named local.  Shipped zeroes the counter
		//      with `mov $0x0,%ebx` AFTER `add %esi,%eax` (0x1007d5c5,
		//      flag-preserving, so the `je` still tests the add);
		//      `iChild < nChildren - 1` in the loop condition gave
		//      `xor %ebx,%ebx` before the add.  The last-child read
		//      still indexes `-0x4(%eax,%esi,4)` either way.
		//
		//    * the two continues share the tail at 0x1007d5a3, which
		//      assigns pParentXForm = pNodeMat.  So the assignment is
		//      written ONCE, after the if/else, and not duplicated into
		//      each arm.
		// ------------------------------------------------------------- //
		if (m_pIndexStackf8)
		{
			if (m_nIndexStackCountfc <= 0)
				break;

			nNodeIndex = m_pIndexStackf8[--m_nIndexStackCountfc];
		}
		else
		{
			uint32  nChildren = pNode->m_nNumChildren20;

			if (nChildren == 0)
				break;

			uint32  nLast = nChildren - 1;

			for (uint32 iChild = 0; iChild < nLast; iChild++)
				sub_1007cc90(pNode->m_ppChildren1c[iChild]->m_nIndex10,
				             pNodeMat);

			m_nIndexStackCountfc = 1;

			nNodeIndex = pNode->m_ppChildren1c[nLast]->m_nIndex10;
		}

		pParentXForm = pNodeMat;
	}
}


// *********************************************************************** //
//  THE CALL SEQUENCE, AND WHAT TO EXPECT FROM IT
// *********************************************************************** //
//
//  Shipped, in order (decoded call sites, not a byte scan for 0xe8):
//
//      0x1007cce1  0x1007da80          blend animation 0
//      0x1007cd0e  0x1007d624          blend animations 1..n-1
//      0x1007cd4c  0x10073400          m_Rot188  -> m_Mat148
//      0x1007d0ce  0x10073400          pXForm->m_Rot0c -> m_Mat104
//      0x1007d574  INDIRECT            *m_pfnNodeCallbackec
//      0x1007d5ee  0x1007cc94          recurse into a child
//
//  4 distinct direct callees of 5 direct calls -- so
//  this is a discriminating sequence, but a very short one at 408 bytes
//  per call.  The census banner at the top of this file, not this list, is
//  the evidence.
//
//  PREDICTED RESULT: 6 shipped, 6 ours, 5 in agreement, one shipped-only
//  and one ours-only, both at 0x1007d5ee.
//
//  That pair is the `/Qip` +4 pattern and nothing else.  The
//  shipped self-call reaches the REGISTER entry at 0x1007cc94 (its two
//  callers are both in this same shipped TU: 0x1007ca9e in sub_1007ca60
//  and this one); ours reaches the symbol, which resolves to
//  0x1007cc90 from the `sub_1007cc90` name.  Three ways to close it, in
//  increasing cost:
//
//    (a) rename the method `sub_1007cc94`.  Cheap, and wrong in the sense
//        that the function does not start there -- it would put the
//        enumeration's original error back into our own source.  Not done
//        here.
//    (b) compile this TU with `/Qip` and give the method external
//        linkage.  This has been measured to produce
//        the two-symbol pair (`?f@@...` and `?f$@@...`) with same-TU
//        callers entering at the register one, which is exactly the
//        shipped shape.  DONE: the file is in D3D_QIP_OS_TUS (/Os
//        /Qip), the self-call reaches `sub_1007cc90$`, and callseq
//        reports 6 of 6.  The scorer places `sub_1007cc90$` at
//        0x1007cc90 + the stack entry's 4 bytes, and the stack entry
//        itself at 0x1007cc90 (progress/extent-splits.csv).
//    (c) accept it and say so: it is recognisable by the small constant
//        offset between the two addresses, and it costs one shipped-only
//        and one ours-only row.
//
//  The other four direct rows should resolve by name: sub_1007da80,
//  sub_1007d624 and sub_10073400 are all declared with `sub_<address>`
//  names, so the call check places them without needing our objects to define
//  them.  Note that sub_1007d624 is ALSO a /Qip register entry (its stack
//  entry is 0x1007d620) and the declared name here is deliberately
//  `sub_1007d624`, the address the shipped call actually reaches.
//
//  The indirect row can only be compared by position, and its position is
//  the same on both sides (fifth of six).  It is not sunk below the
//  epilogue in the shipped code -- 0x1007d550..0x1007d576 sits inline
//  between MatMul 2 and the descent -- so the benign
//  block-placement pattern does not apply and no extra pair is expected
//  from it.
//
//  /Qrcd -- NOT NEEDED, and this is measured off the census rather than
//  assumed from the family: the range contains no `fistp` of any width,
//  no float-to-int conversion of any shape, and no colour byte lerp.  The
//  whole floating-point content is 128 multiplies, 96 adds, 32 stores and
//  129 loads.
//
//  /Oi- -- NOT NEEDED here.  All six calls are accounted for above and
//  none of them is a CRT intrinsic (no sqrt, fabs, tan, atan, memcpy),
//  and there is no `rep movs` in the range for a struct copy to have
//  become.  The flag would change nothing in this TU.  (This is a
//  judgement about THIS function; the D3D_OI_TUS list stands for the
//  files where it was measured.)
//
//  _chkstk -- no residual.  `sub $0x50,%esp` is 80 bytes and the shipped
//  prologue emits no probe.
//
//  THE LITERAL POOL -- this function contributes NOTHING to it.  There is
//  not one `.rdata`/`.data1` float operand in the whole 2,448 bytes; the
//  only non-code address it touches is 0x10091d40 in `.data`.  So there is
//  no division here and therefore none of the dead-1.0f pairs to
//  account for, and the pool cannot be used as a statement-order witness
//  for this function.  The store ORDER of the two MatMuls does that job
//  instead.
//
// *********************************************************************** //
//  WHAT COULD NOT BE PINNED
// *********************************************************************** //
//
//  There are NO `// UNDECODED:` regions.  All 652 instructions in
//  0x1007cc90..0x1007d61b are accounted for by the census banner at the
//  top, and every branch, bound, offset and stride above is cited by
//  address.  What is left is naming and meaning, not structure:
//
//  (1) THE MEANING of +0x0f8 and +0x0fc.  The STRUCTURE is fixed: +0xf8
//      is a dword array tested for null, +0xfc is its unsigned count,
//      the count is decremented and the array read at [count-1], and the
//      other arm sets the count to 1.  What that is FOR is not
//      recoverable from this function, and it is not recoverable from the
//      rest of d3d.ren either:
//        - the constructor 0x10015950 does NOT initialise either field
//          (it zeroes only +0xe0..+0xf4, at 0x1001599c..0x100159ba);
//        - sub_1007cab4 stores NULL into +0xf8 unconditionally
//          (0x1007cb3e) and never writes +0xfc;
//        - a whole-.text scan for the two offsets finds no other
//          reader or writer of either.
//      So in every traversal d3d.ren actually performs, +0xf8 is NULL,
//      the +0xf8 arm is dead code, and the `m_nIndexStackCountfc = 1`
//      store at 0x1007d601 has no observable effect.  The names
//      `m_pIndexStackf8` / `m_nIndexStackCountfc` are a DESCRIPTION of
//      the code's shape, not an identification; treat them as
//      provisional.  They are written out because they are in the bytes,
//      and a reconstruction that dropped the dead arm would be 60 bytes
//      and two branches short of the shipped function.
//
//  (2) The types of +0x0f0 and +0x0f4 (the callback's fourth and first
//      arguments).  Both are opaque dwords here; big17c14.cpp:483 types
//      +0xf4 as `D3DObject*` from its own call site, which is the better
//      evidence, and +0xf0 is only ever copied.  Declared `void*`.
//
//  (3) The width of D3DModelNode::m_nFlags12.  `mov 0x12(%eax),%al`
//      narrows the same way from a uint8 or a uint16 field under both
//      compilers ("Two movzx passes", case A), so these bytes
//      do not decide it.  uint8 with the next byte padded.
//
//  (4) The 24 unused bytes of the frame, -0x40 .. -0x54.  `sub $0x50` is
//      80 bytes and the deepest slot referenced is -0x3c; nothing in the
//      range touches -0x40 or below.  icl's own rounding, most likely,
//      but not demonstrated -- so it is listed rather than explained
//      away.  A 0x40-byte staging matrix would fit there, which is why it
//      is worth saying explicitly that no instruction addresses it.
//
//  (5) Whether the two matrix concatenations are really d3d_MatMul or a
//      macro with the same body.  The bytes cannot tell an inline
//      function from a macro; what they DO tell is that both sites have
//      the identical 16-element store order, which is why one helper is
//      written rather than two blocks.
//
// *********************************************************************** //
//  COIN-FLIPS -- both spellings given, so one build settles each
// *********************************************************************** //
//
//  Every one of these is call-neutral: the function has 6 calls, and none
//  of the alternatives below adds, removes or reorders any of them.  Said
//  explicitly because the one place
//  where an arm order WOULD have moved a call -- the `flags12 & 2` if/else
//  in REGION 3 -- is not a coin-flip at all: neither arm calls anything,
//  and the fall-through fixes the polarity.
//
//  (0) TWO LOOP COUNTERS, NOT ONE.  Each loop declares its own, which is
//      not a style choice.  Do not reuse one `i` across every
//      loop: a counter live across the whole function claims a
//      callee-saved register for its whole length, and %ebx cannot be
//      that register here: it holds `i` in the animation loop
//      (0x1007cd00), pParentXForm through MatMul 1 (0x1007cda2 onward,
//      with `i`'s copy of it spilled to -0x28 around the loop) and `i`
//      again in the child loop (0x1007d5c5).  Neither counter is ever
//      given a frame slot, so both live in registers only -- which is
//      exactly the shape two separately-scoped counters produce and one
//      shared counter cannot.  No call either way.
//
//  (1) THE PEELED LAST CHILD.  Written above as
//
//          for (i = 0; i < nChildren - 1; i++)
//              sub_1007cc90(pNode->m_ppChildren1c[i]->m_nIndex10, pNodeMat);
//          m_nIndexStackCountfc = 1;
//          nNodeIndex = pNode->m_ppChildren1c[nChildren - 1]->m_nIndex10;
//
//      The alternative is the plain recursion
//
//          for (iChild = 0; iChild < nChildren; iChild++)
//              sub_1007cc90(pNode->m_ppChildren1c[iChild]->m_nIndex10,
//                           pNodeMat);
//          return;
//
//      with icl having peeled the last iteration into a tail jump.  The
//      form written is the evidenced one on three counts (the -0x3c bound
//      is nChildren-1, the census has exactly TWO `movzwl` of +0x10, and
//      0x1007d60e indexes with %esi = nChildren) and the alternative
//      leaves the `movl $0x1,0xfc(%edi)` store with nothing to come from.
//      Both spellings give ONE static call site to 0x1007cc94, so
//      the call sequence cannot separate them -- which is exactly why the census
//      had to.
//
//  (2) `m_nIndexStackCountfc = 1;` BEFORE or AFTER the last child's
//      index read.  Emitted first (0x1007d601 precedes the 0x1c(%eax)
//      load at 0x1007d60b), and written first above; but %eax is reloaded
//      from -0x34 at 0x1007d5fe either way, so the store could equally
//      be the second statement and have been scheduled up.  No call.
//
//  (3) THE THREE TRANSLATION MOVES, in both REGION 3 and REGION 5.
//      Written x, y, z at all three sites.  The shipped emission order is
//      x,y,z at 0x1007cd6a, y,z,x at 0x1007cd90 and z,y,x at 0x1007d104 --
//      three different orders for the same three statements in one
//      function, which is what says the order is the scheduler's.  Built:
//      all three are d3d_SetTranslation (by-value LTVec, see REGION 3),
//      and REGION 3's two arms and REGION 5's moves come out as shipped.
//      No call.
//
//  (4) THE POP EXPRESSION.  Written
//          nNodeIndex = m_pIndexStackf8[--m_nIndexStackCountfc];
//      The two-statement form
//          m_nIndexStackCountfc--;
//          nNodeIndex = m_pIndexStackf8[m_nIndexStackCountfc];
//      also loads +0xfc once and also indexes at count-1; the shipped
//      code keeps the OLD value in %eax and reaches the element with
//      `-0x4(%ecx,%eax,4)` (0x1007d59c), which very slightly favours the
//      pre-decrement-in-subscript spelling written.  No call.
//
//  (5) THE COUNT TEST.  Written `if (m_nIndexStackCountfc <= 0) return;`.
//      `if (m_nIndexStackCountfc > 0) { ... } else return;` gives the
//      same `test`/`jbe`.  What is NOT a coin-flip is using `== 0` here:
//      that spelling emits `je`, and the shipped byte is
//      `jbe` (0x1007d58b) where the child-count test four instructions
//      later really is `je` (0x1007d5bc).  No call.
//
//  (6) `pNodeMat` AS A NAMED LOCAL.  Written as one local computed at the
//      top of the loop body.  The alternative is
//      `&m_pNodeMatse8[nNodeIndex]` written out at its four uses.  The
//      local is strongly favoured: the shipped code computes it at
//      0x1007ccda, 900 bytes before its first use, and keeps it in -0x38
//      for the whole body ("Name a repeated address in a
//      local").  MatMul 2's stores nonetheless go through
//      base+index rather than the pointer, so if a build shows the
//      pointer where the shipped code indexes, pass
//      `&m_pNodeMatse8[nNodeIndex]` to d3d_MatMul while keeping the local
//      for the other three uses.  No call.
//
//  (7) `for (;;)` WITH RETURNS versus `while (1)` with breaks, and versus
//      writing the two continues as `goto`.  The back edge at 0x1007d5a6
//      is an unconditional `jmp` to the top of the body, so the loop is
//      unrotated and all three spellings lower the same way.  No call.
//
//  (8) THE CLASS NAME.  `D3DRenderInfo`, taken from big17c14.cpp:473 so
//      the two files agree; d3dr4a.cpp calls the same class `C1007ca60`
//      and types +0xe4 as `uint32 m_nValuee4`, which this function
//      corrects to `LTMat*`.  Cosmetic, but worth unifying when these
//      files are merged.
//
// *********************************************************************** //


// *********************************************************************** //
//  VERIFICATION PERFORMED, AND WHAT WAS NOT
// *********************************************************************** //
//
//  Done:
//    * the extent 0x1007cc90, 2448 walked from the bytes --
//      one ret, one prologue, no strays, no overlap, /Qip stack entry.
//    * the call count over 0x1007cc90..0x1007d620
//      -- 6 calls, 5 direct, 1 indirect, 408 B/call; and the
//      ordered target list quoted above, plus 4 distinct direct callees.
//    * a full mnemonic census of all 652 instructions, reconciled row by
//      row against the claimed structure with nothing left over.
//    * an x87 stack simulation of one element of each MatMul, to fix the
//      operand roles rather than assume them.
//    * a whole-.text scan confirming that every reference to
//      0x10091d40..0x10091d7f lies inside this function (62 of 62) and
//      that no other function reads or writes the class's +0xf8/+0xfc.
//    * the class and struct layouts checked with 26 compile-time
//      assertions at 32-bit pointer width -- +0xe0, +0xe4, +0xe8, +0xec,
//      +0xf0, +0xf4, +0xf8, +0xfc, +0x100, +0x104, +0x144, +0x148,
//      +0x188, +0x198, +0x1a4, sizeof == 0x1a8; the node's +0x04, +0x10,
//      +0x12, +0x1c, +0x20; D3DNodeXForm's +0x0c and sizeof == 0x1c;
//      D3DModel's +0x28; D3DBasePose's +0x04; sizeof(LTMat) == 0x40.
//      All 26 hold.
//    * a `-fsyntax-only` pass, and a link attempt that confirmed the
//      three declared-but-not-defined callees really do emit calls
//      (they came back as undefined references, which is the point).
//
//  MEASURED (build/engine.sh d3d-one big7cc90.cpp, tools/fndiff.py,
//  tools/callseq.py):
//    * flags.  /Os /Qip and /Os /Qip /Qip_no_inlining, /Os /Oi- /Qip,
//      /Os /Qip /Qsfalign16 and /O1 /Qip all give the same 2,424 bytes;
//      /O2 /Qip comes out ~80 bytes short with a 0x40 frame, /Os /G5
//      /Qip ~100 long.  /Os /Qip is what the file's TU-mates use
//      (d3dr4a.cpp and w10_1007cab4_os.cpp are /Os), so that is the group.
//    * call sequence 6 of 6, the /Qip self-call included.
//    * byte-exact: the stack entry; 0x1007cc94..0x1007cda2 (prologue,
//      REGIONS 1-3); 0x1007d546..0x1007d61b (callback and descent).
//      Different: the x87 code of both MatMuls, and in REGION 5 which
//      of %eax / %ebx holds the record base and which `nNodeIndex*0x1c`
//      (shipped: base in %eax and spilled to -0x3c, index in %ebx).
//      In MatMul 1 the [0][c] elements start with term 2 where shipped
//      starts with term 3 ([1..3][c] already agree), and in MatMul 2 the
//      choice of which registers hold 0x10091d40 differs from the
//      first element on.  Swapping the product operands, reversing the
//      sums, the SDK's own LTMatrix / MatMul / Mat_SetTranslation, a
//      non-static scratch matrix and three spellings of REGION 5's
//      record pointer were all built: none moves it (or it gets worse),
//      so it reads as the "FPU term ordering" near class rather than a
//      source shape.
//
// *********************************************************************** //
