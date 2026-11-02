// *********************************************************************** //
//  MODULE:   TRANSFORM (.CPP) -- lithtech.exe, cluster 0x00489630..0x0048a4ab
//  BUILD:    MSVC 6 (cl 8447), /O2  (frameless, %esp-relative)
//  GROUND TRUTH: the retail lithtech.exe
//
//  NOT A PREBUILT LIBRARY.  Evidence:
//    * every 20-byte window at stride 8 over all eleven enumerated rows was
//      searched in the retail server.dll.  Ten of the eleven produced ZERO
//      hits.  The only full-body match in the whole 0x00488000..0x0048c000
//      sweep is 0x0048a080 (136 bytes) at server 0x10035468 -- and that is
//      quat_Mul, an SDK *inline* out of src/LT2/sdk/inc/ltquatbase.h that both
//      images carry out of line.  A straight-line x87 leaf with no branches,
//      no relocations and no frame is the one shape /O1 and /O2 agree on, so
//      it is not evidence of a library; src/engine/server/servermath.cpp had
//      already identified and scored the server copy under exactly that
//      reading.  Nothing here belongs in src/lithshared/.
//
//  TWO TRANSLATION UNITS ARE PRESENT, kept in one file only because the task
//  asks for one:
//    A. 0x00489630..0x004898c0  CLTTransform -- the client's ILTTransform.
//    B. 0x00489a60..0x0048a4ab  the model-node transform builder (section B).
//  They are separated by the 0x00489a5a padding run and share no call edge.
// *********************************************************************** //

#include "ltbasedefs.h"
#include "ltmatrix.h"
#include "ltquatbase.h"
#include "ilttransform.h"


// ======================================================================= //
//  SECTION A -- CLTTransform, the client's ILTTransform implementation.
//
//  WHAT PINS THE CLASS
//    * vtable 0x004a16c8.  ltnew2.cpp had
//      already anchored it: CLTClient's ctor writes 0x004a16c8 to this+0x3a4
//      at 0x00404c10 and stores &this[0x3a4] into m_pTransformLT at this+0x14
//      (0x00404c9c), and iltcsbase.h puts m_pTransformLT at +0x14.
//    * the enumerator SPLIT this vtable: it lists 0x004a16c8 with eight slots
//      and then a one-slot "table" at 0x004a16e8.  0x004a16c8 + 8*4 = 0x4a16e8,
//      so it is ONE nine-slot table -- exactly ilttransform.h's nine virtuals,
//      and the per-slot arg counts line up one for one:
//
//        slot 0  0x004a16c8 -> 0x00489630  3 args  Get
//        slot 1  0x004a16cc -> 0x00489670  3       Set
//        slot 2  0x004a16d0 -> 0x004896b0  2       GetPos        [already done]
//        slot 3  0x004a16d4 -> 0x004896d0  2       GetRot        [already done]
//        slot 4  0x004a16d8 -> 0x00489700  2       ToMatrix
//        slot 5  0x004a16dc -> 0x00489750  2       FromMatrix    [already done]
//        slot 6  0x004a16e0 -> 0x00489780  2       Inverse
//        slot 7  0x004a16e4 -> 0x004897f0  3       Multiply
//        slot 8  0x004a16e8 -> 0x004898c0  3       Difference
//
//      The three marked [already done] are reconstructed and scored
//      `identical` (ltnew2.cpp, lt2new2.cpp,
//      lt2new4.cpp).  They are repeated here because the six NEW ones only
//      become readable as a class alongside them, and because the "gap" rows
//      0x00489630/0x00489670/0x00489700 sit between the enumerated rows and
//      belong to the same class.
//
//  CALLEES, all already `identical`:
//    0x00444880  quat_ConvertToMatrix(const float*, float[4][4])
//                -- opens `flds 0xc;flds 8;flds 4;flds 0` then
//                   x*x+y*y+z*z+w*w and `fdivrs 0x004a1a68` (= 2.0f), which
//                   is ltquatbase.cpp's normalising quat->matrix and nothing
//                   else.
//    0x004449b0  quat_ConvertFromMatrix(float*, const float[4][4])
//                -- opens with the trace m[1][1]+m[0][0]+m[2][2]
//                   (already pinned in lt2new4.cpp).
//    0x00434580  MatMul(LTMatrix*, LTMatrix*, LTMatrix*)  (ltmatrix.h:172)
//    0x00431650  MatVMul_3x3(LTVector*, LTMatrix*, LTVector*) (ltmatrix.h:267)
//
//  THE FRAME SIZES ARE THE PROOF.  Every local in this class is either a
//  64-byte LTMatrix or the 12-byte LTVector that `SetTranslation(LTVector)`
//  -- note: BY VALUE, ltmatrix.h:57 -- makes on every call.  The four bodies
//  that have a frame account for it to the byte:
//    ToMatrix    0x0c  = 12            = 1 vector
//    Inverse     0x4c  = 12 + 64       = 1 vector + 1 matrix
//    Multiply    0xcc  = 12 + 3*64     = 1 vector + 3 matrices
//    Difference  0x118 = 12 + 12 + 4*64= 2 vectors + 4 matrices
//  Nothing is left over and nothing is missing, which is what rules out a
//  hidden extra temporary in any of them.
// ======================================================================= //

// Declared standalone rather than `: public ILTTransform`, and NOT `virtual`,
// which is how ltnew2.cpp / lt2new2.cpp / lt2new4.cpp already spell their
// three slots.  It costs nothing: `this` is dead in all nine bodies, so the
// vptr never shows in the code, and thiscall is thiscall either way.  The
// class really is ILTTransform's implementation -- the nine addresses ARE
// 0x004a16c8's nine slots -- but deriving from the SDK class would drag in
// ILTTransform's own vtable, whose nine declared-not-defined virtuals have no
// bodies anywhere in this tree.
class CLTTransform
{
public:
	LTRESULT Get(LTransform &transform, LTVector &pos, LTRotation &rot);
	LTRESULT Set(LTransform &transform, LTVector &pos, LTRotation &rot);
	LTRESULT GetPos(LTransform &transform, LTVector &pos);
	LTRESULT GetRot(LTransform &transform, LTRotation &rot);
	LTRESULT ToMatrix(LTransform &transform, LTMatrix &mat);
	LTRESULT FromMatrix(LTransform &transform, LTMatrix &mat);
	LTRESULT Inverse(LTransform &inverse, LTransform &transform);
	LTRESULT Multiply(LTransform &out, LTransform &t1, LTransform &t2);
	LTRESULT Difference(LTransform &diff, LTransform &t1, LTransform &t2);
};


// ----------------------------------------------------------------------- //
//  0x00489630 -- slot 0, Get.  60 bytes, `ret $0xc`.
//
//  WHAT PINS IT
//    * `ret $0xc` with %ecx dead on the third instruction -> member, three
//      reference arguments, `this` unused.
//    * 0x00489638 `mov %eax,%ecx` parks arg1 in a second register so %eax
//      stays live for the second copy; that is allocation, not source.
//    * three dword mov/mov pairs at +0/+4/+8 of arg1 into arg2 -- the
//      compiler-generated bitwise LTVector operator= (ltvector.h declares
//      none), i.e. `pos = transform.m_Pos;`.  Then FOUR pairs at
//      +0xc/+0x10/+0x14/+0x18 into arg3: LTRotation is four floats and
//      m_Rot is at +0x0c of an LTransform (ltbasetypes.h).
//    * order: m_Pos copy first, then m_Rot -- the declaration order of the
//      two out-parameters, so the statements are in that order too.
//    * `xor %eax,%eax` immediately before the ret -> `return LT_OK;`.
// ----------------------------------------------------------------------- //
LTRESULT CLTTransform::Get(LTransform &transform, LTVector &pos, LTRotation &rot)
{
	pos = transform.m_Pos;
	rot = transform.m_Rot;

	return LT_OK;
}


// ----------------------------------------------------------------------- //
//  0x00489670 -- slot 1, Set.  60 bytes, `ret $0xc`.
//
//  The mirror image of Get, and the byte pattern mirrors it exactly: the
//  source of the vector copy is arg2 and the destination arg1+0, the source
//  of the rotation copy is arg3 and the destination arg1+0xc.  `mov %eax,%edx`
//  at 0x00489679 is the same parking trick with the roles swapped (arg1 is
//  now the DESTINATION, so it is the one that must stay live).
// ----------------------------------------------------------------------- //
LTRESULT CLTTransform::Set(LTransform &transform, LTVector &pos, LTRotation &rot)
{
	transform.m_Pos = pos;
	transform.m_Rot = rot;

	return LT_OK;
}


// ----------------------------------------------------------------------- //
//  0x004896b0 -- slot 2, GetPos.  ALREADY `identical` (ltnew2.cpp).
// ----------------------------------------------------------------------- //
LTRESULT CLTTransform::GetPos(LTransform &transform, LTVector &pos)
{
	pos = transform.m_Pos;

	return LT_OK;
}


// ----------------------------------------------------------------------- //
//  0x004896d0 -- slot 3, GetRot.  ALREADY `identical` (lt2new2.cpp).
// ----------------------------------------------------------------------- //
LTRESULT CLTTransform::GetRot(LTransform &transform, LTRotation &rot)
{
	rot = transform.m_Rot;

	return LT_OK;
}


// ----------------------------------------------------------------------- //
//  0x00489700 -- slot 4, ToMatrix.  65 bytes, `ret $8`, frame 0x0c.
//
//  WHAT PINS IT
//    * one __cdecl call to 0x00444880 with (arg1+0xc, arg2) -- the quaternion
//      of the transform and the matrix -- so `quat_ConvertToMatrix(
//      transform.m_Rot.m_Quat, mat.m)` and the argument order is fixed by the
//      push order (`push %esi` = mat, `push %eax` = &m_Rot, %eax last, so it
//      is argument 1).
//    * the three dwords read from arg1+0/+4/+8 land at 0xc, 0x1c and 0x2c of
//      arg2 -- m[0][3], m[1][3], m[2][3] of a row-major float[4][4], the
//      translation column.  That is LTMatrix::SetTranslation(float,float,float)
//      inlined (ltmatrix.h:781), stores in declaration order.
//    * AND the same three dwords are ALSO written to the whole 0x0c-byte
//      frame at 0x04/0x08/0x10(%esp).  Nothing ever reads that copy.  It is
//      the by-value parameter of `SetTranslation(LTVector vec)`
//      (ltmatrix.h:776) -- MSVC materialises the copy even after forwarding
//      the values.  A `SetTranslation(const LTVector&)` overload does not
//      exist and a hand-written three-line store would not allocate 12 bytes
//      of frame, so the call spelling is forced.
//    * the frame is EXACTLY 0x0c: no other local exists.
// ----------------------------------------------------------------------- //
LTRESULT CLTTransform::ToMatrix(LTransform &transform, LTMatrix &mat)
{
	quat_ConvertToMatrix(transform.m_Rot.m_Quat, mat.m);
	mat.SetTranslation(transform.m_Pos);

	return LT_OK;
}


// ----------------------------------------------------------------------- //
//  0x00489750 -- slot 5, FromMatrix.  ALREADY `identical` (lt2new4.cpp).
// ----------------------------------------------------------------------- //
LTRESULT CLTTransform::FromMatrix(LTransform &transform, LTMatrix &mat)
{
	transform.m_Pos.x = mat.m[0][3];
	transform.m_Pos.y = mat.m[1][3];
	transform.m_Pos.z = mat.m[2][3];

	quat_ConvertFromMatrix(transform.m_Rot.m_Quat, mat.m);

	return LT_OK;
}


// ----------------------------------------------------------------------- //
//  0x00489780 -- slot 6, Inverse.  100 bytes, `ret $8`, frame 0x4c.
//
//  THIS FUNCTION DOES NOT INVERT ANYTHING.  Read the bytes: two calls, the
//  quaternion->matrix at 0x00489791 and the matrix->quaternion at 0x004897d3,
//  and between them nothing but the translation stores.  No `fchs`, no
//  transpose, no second matrix -- and the frame is 0x4c = 12 + 64, room for
//  ONE LTVector and ONE LTMatrix, so there is nowhere for an inverse to live.
//  (Compare Difference at 0x004898c0, which DOES invert: it has the transpose
//  copies, three `fchs` and the 0/0/0/1 bottom row inline, and pays 0x118 of
//  frame for the extra matrix.)  So the shipped ILTTransform::Inverse is a
//  transform-to-matrix-and-back round trip -- a Monolith bug.  Reconstructing
//  it means reproducing the bug.
//
//  WHAT PINS THE BODY
//    * the matrix local is at E-0x40 (`lea 0x10(%esp)` at 0x00489788 and
//      `lea 0x18(%esp)` at 0x004897c2 with the two intervening pushes name
//      the same address), the vector local at E-0x4c.
//    * the SetTranslation by-value copy is at E-0x4c..E-0x40 and the three
//      matrix stores at E-0x34/E-0x24/E-0x14 = mat+0xc/+0x1c/+0x2c: the same
//      ToMatrix shape.
//    * the three loads that build the output come from the VECTOR copy
//      (`flds 0xc(%esp)` = E-0x4c, `mov 0x10(%esp)` = E-0x48) and from %ecx,
//      not from the matrix -- CSE, because SetTranslation has just stored the
//      identical values into m[0][3]/m[1][3]/m[2][3].  Source-level this is
//      FromMatrix's three reads; the compiler forwards them.
//    * x goes out through `flds`/`fstps` while y and z go out as integer
//      `mov`s: at that point %eax holds the out pointer, %ecx holds z and
//      %edx is needed for the `lea`, so one of the three has to route through
//      the x87 stack.  Register pressure, not source (Multiply and Difference
//      do the identical thing at the identical place).
//    * `xor %eax,%eax` before the ret -> `return LT_OK;`.
// ----------------------------------------------------------------------- //
LTRESULT CLTTransform::Inverse(LTransform &inverse, LTransform &transform)
{
	LTMatrix mat;

	quat_ConvertToMatrix(transform.m_Rot.m_Quat, mat.m);
	mat.SetTranslation(transform.m_Pos);

	inverse.m_Pos.x = mat.m[0][3];
	inverse.m_Pos.y = mat.m[1][3];
	inverse.m_Pos.z = mat.m[2][3];

	quat_ConvertFromMatrix(inverse.m_Rot.m_Quat, mat.m);

	return LT_OK;
}


// ----------------------------------------------------------------------- //
//  0x004897f0 -- slot 7, Multiply.  207 bytes, `ret $0xc`, frame 0xcc.
//
//  FRAME MAP (E = %esp on entry), and it is exact:
//    E-0xcc  LTVector   the SetTranslation by-value copy, used twice
//    E-0xc0  LTMatrix   mat2   (built from t2)
//    E-0x80  LTMatrix   mat1   (built from t1)
//    E-0x40  LTMatrix   matOut (MatMul's destination)
//    12 + 3*64 = 204 = 0xcc.
//
//  WHAT PINS IT
//    * two calls to 0x00444880: the first on arg2+0xc into E-0x80, the second
//      on arg3+0xc into E-0xc0.  Each is followed by the same three-dword
//      translation store at +0xc/+0x1c/+0x2c of its matrix plus the by-value
//      vector copy -- ToMatrix twice, on t1 then t2, in that order.
//    * the single __cdecl call to 0x00434580 pushes E-0xc0 (mat2), then
//      E-0x80 (mat1), then E-0x40 (matOut); last push is argument 1, so it is
//      MatMul(&matOut, &mat1, &mat2), i.e. out = t1 * t2 -- NOT t2 * t1.
//    * the tail is FromMatrix on matOut into arg1: `flds` of matOut+0xc into
//      arg1+0, integer movs of +0x1c and +0x2c into arg1+4/+8, then
//      quat_ConvertFromMatrix(arg1+0xc, matOut).
//    * `add $0x24,%esp` at the end cleans 2+2+3+2 = 9 dwords: four __cdecl
//      calls' arguments, deferred and cleaned once.  That count is what
//      confirms there are exactly four calls and no fifth.
// ----------------------------------------------------------------------- //
LTRESULT CLTTransform::Multiply(LTransform &out, LTransform &t1, LTransform &t2)
{
	LTMatrix mat1, mat2, matOut;

	quat_ConvertToMatrix(t1.m_Rot.m_Quat, mat1.m);
	mat1.SetTranslation(t1.m_Pos);

	quat_ConvertToMatrix(t2.m_Rot.m_Quat, mat2.m);
	mat2.SetTranslation(t2.m_Pos);

	MatMul(&matOut, &mat1, &mat2);

	out.m_Pos.x = matOut.m[0][3];
	out.m_Pos.y = matOut.m[1][3];
	out.m_Pos.z = matOut.m[2][3];

	quat_ConvertFromMatrix(out.m_Rot.m_Quat, matOut.m);

	return LT_OK;
}


// ----------------------------------------------------------------------- //
//  0x004898c0 -- slot 8, Difference.  409 bytes, `ret $0xc`, frame 0x118.
//
//  FRAME MAP (E = %esp on entry), exact to the byte:
//    E-0x118 LTVector  SetTranslation's by-value copy, later newTrans
//    E-0x10c LTVector  Mat_InverseTransformation's `trans`
//    E-0x100 LTMatrix  mat2Inv
//    E-0xc0  LTMatrix  mat2
//    E-0x80  LTMatrix  mat1
//    E-0x40  LTMatrix  matDiff
//    12 + 12 + 4*64 = 280 = 0x118.
//
//  WHAT PINS THE INVERSE.  0x004899b5 calls 0x00431650 = MatVMul_3x3, and
//  everything around it is Mat_InverseTransformation (ltmatrix.h:304)
//  inlined, line for line:
//    * `trans` at E-0x10c is loaded from t2's position -- CSE of
//      pSrc->m[0][3]/m[1][3]/m[2][3], which SetTranslation has just written.
//    * the nine dword copies at 0x0048992d..0x004899a6 move mat2's upper 3x3
//      TRANSPOSED into mat2Inv: dest +0/+4/+8 <- src +0/+0x10/+0x20, dest
//      +0x10/+0x14/+0x18 <- src +0x4/+0x14/+0x24, dest +0x20/+0x24/+0x28 <-
//      src +0x8/+0x18/+0x28.  Exactly the header's three lines.
//    * MatVMul_3x3(&newTrans, pDest, &trans) -- push order (trans, mat2Inv,
//      newTrans) makes newTrans argument 1; newTrans reuses E-0x118.
//    * three `flds`/`fchs`/`fstps` into mat2Inv+0xc/+0x1c/+0x2c: the three
//      `= -newTrans.x/y/z` lines.
//    * the bottom row is stored m[3][2] FIRST, then m[3][1], then m[3][0],
//      then m[3][3]=0x3f800000.  That reversed order is the chained
//      assignment `pDest->m[3][0] = pDest->m[3][1] = pDest->m[3][2] = 0.0f;`
//      evaluating right to left -- a three-statement version would emit in
//      the other order.  It is the single strongest byte-level tell that the
//      header's own Mat_InverseTransformation is what was compiled.
//  MatVMul_3x3 stays OUT of line (0x00431650) while its caller is inlined.
//  That is VC6's per-function inline budget, and what spends it is the
//  source's shape: the body goes through two small inline helpers (ToMatrix
//  and FromMatrix bodies, tr_ToMatrix/tr_FromMatrix below).  Written flat
//  -- the same statements inline in Difference -- VC6 expands MatVMul_3x3
//  too (474 bytes against 409).  Through the helpers: identical, and the
//  COMDAT copy of MatVMul_3x3 is emitted and matches 0x00431650.
//
//  WHAT PINS THE MULTIPLY ORDER.  0x00489a10 pushes E-0x80 (mat1), then
//  E-0x100 (mat2Inv), then E-0x40 (matDiff): MatMul(&matDiff, &mat2Inv,
//  &mat1) -- the inverse on the LEFT.  That is the correct reading of the
//  header comment "diff = t1 - t2 so t2 + diff = t1".
//
//  `add $0x30,%esp` cleans 2+2+3+3+2 = 12 dwords -- five __cdecl calls, which
//  is the count in the body.
// ----------------------------------------------------------------------- //
inline void tr_ToMatrix(LTransform &transform, LTMatrix &mat)
{
	quat_ConvertToMatrix(transform.m_Rot.m_Quat, mat.m);
	mat.SetTranslation(transform.m_Pos);
}

inline void tr_FromMatrix(LTransform &transform, LTMatrix &mat)
{
	transform.m_Pos.x = mat.m[0][3];
	transform.m_Pos.y = mat.m[1][3];
	transform.m_Pos.z = mat.m[2][3];

	quat_ConvertFromMatrix(transform.m_Rot.m_Quat, mat.m);
}

LTRESULT CLTTransform::Difference(LTransform &diff, LTransform &t1, LTransform &t2)
{
	LTMatrix mat1, mat2, mat2Inv, matDiff;

	tr_ToMatrix(t1, mat1);
	tr_ToMatrix(t2, mat2);

	Mat_InverseTransformation(&mat2, &mat2Inv);
	MatMul(&matDiff, &mat2Inv, &mat1);

	tr_FromMatrix(diff, matDiff);

	return LT_OK;
}


// ####################################################################### //
//  SECTION B -- 0x00489a60..0x0048a4ab, the model transform builder.
//
//  WHAT THE MODULE IS.  `this` opens with EIGHT 0x1c-byte records and carries
//  their count at +0xe0; 8 * 0x1c = 0xe0 exactly, so the count sits directly
//  behind the array.  The record is ltanimtracker.h's AnimTimeRef field for
//  field, and every access in the module agrees:
//      +0x00 Model* m_pModel        (0x00489a8a compares every record's +0
//                                    against the first one's)
//      +0x04 m_Prev.m_iAnim  uint16 (0x00489c15 `mov (%ecx),%bx`, then <<5
//                                    to index Model::m_AnimArray)
//      +0x06 m_Prev.m_iFrame uint16 (0x00489d1e, then *0x1c to index the
//                                    keyframe transforms)
//      +0x0c m_Cur.m_iAnim   uint16 (0x00489c35)
//      +0x0e m_Cur.m_iFrame  uint16 (0x00489d30)
//      +0x14 float m_Percent        (0x00489d12 `flds 0x14(%esi)`, used as
//                                    quat_Slerp's t and VEC_LERP's t)
//      +0x18 uint32 m_iWeightSet    (0x00489c00, indexes Model+0x48)
//  So this class blends up to eight AnimTimeRefs onto a model's node tree and
//  writes one 4x4 per node.  In the shipped engine that is the model
//  transform maker; the name below is ours.
//
//  THE MODEL SIDE IS ALREADY RECONSTRUCTED -- in src/engine/server/
//  commonmodel.h, off server.dll, and every offset this module uses matches
//  it: AnimInfo is 0x20 bytes with m_pAnim at +0 (here `shl $5` then a load
//  at +0 or +4), Model::m_AnimArray is at 0x164 with its array pointer at
//  0x168 (here `mov 0x168(%ebp),%ebp`), ModelWeightSet::m_Weights is at 0x10
//  with its array pointer at 0x14 (here `mov 0x14(%eax),%eax`), and
//  ModelNode's 4x4 lives at +0x2c.  Two facts are NEW and belong back in that
//  header when someone touches it:
//      * AnimInfo carries an LTVector at +0x14 (inside its current
//        m_pad_14[0x20-0x14]) -- the animation's root translation, which
//        0x00489ce0 adds onto node 0 and nothing else reads.
//      * ModelNode carries a uint16 index at +0x12 flags / +0x10 index, a
//        parent index at +0x28 (-1 at the root) and a child array at
//        +0x1c with its count at +0x20.
//
//  NOT ALREADY WRITTEN: none of 0x00489a60..0x0048a4ab appears anywhere in
//  src/engine/ or src/lithshared/, and the cite scan cannot have seen it --
//  it has no server twin at all (the server has no per-node transform pass).
//
//  THE TU BOUNDARY.  0x0048a4b0 and 0x0048a510 (the "%lu.%lu" version
//  formatter and the FindResource/LoadResource pair for RCDATA 12346) are a
//  DIFFERENT module: no call edge to or from anything above, and their
//  callers are 0x00403462 / 0x0040facf / 0x0042e7bc.  They are excluded.
//  0x0048a510's stated 36 bytes are also WRONG -- the
//  branch at 0x0048a529 targets 0x0048a534, which is the next enumerated row;
//  the real function runs 0x0048a510..0x0048a588 (121 bytes) and swallows the
//  two "gap" rows 0x0048a534 (23) and 0x0048a54b (62).  Every other row in
//  the cluster has a clean extent.
// ####################################################################### //

#include "ltanimtracker.h"
#include "ltvector.h"
#include "ltrotation.h"


// ----------------------------------------------------------------------- //
//  The model-side records this module reaches.  Offsets are all cited from
//  lithtech.exe; the names follow src/engine/server/commonmodel.h wherever
//  that header already has the field, so the two can be merged later.
// ----------------------------------------------------------------------- //

// 0x1c bytes: the per-keyframe transform.  Named as in commonmodel.h.
// 0x00489d25 multiplies the frame index by 0x1c (`lea (,%eax,8)` minus %eax,
// then `lea (%edx,%edi,4)`), and 0x00489d4d/0x00489d50 pass base+0xc to
// quat_Slerp -- so m_Pos is at +0 and m_Rot at +0xc.
class CModelXform
{
public:
	LTVector   m_Pos;                        // 0x00
	LTRotation m_Rot;                        // 0x0c
};

// The per-node animation channel.  0x00489cf3 walks
// ModelAnim -> +0x04 (the array pointer of a collection at +0x00) -> [iNode]
// -> +0x0c, and +0x0c is the CModelXform table.
class AnimNode
{
public:
	uint8        m_pad_00[0x0c];
	CModelXform* m_pXforms;                  // 0x0c
};

class ModelAnimK
{
public:
	uint32     m_nAllocedNodes;              // 0x00
	AnimNode** m_pAnimNodes;                 // 0x04
	uint8      m_pad_08[0x20 - 0x08];
};

// commonmodel.h's AnimInfo, plus the LTVector at +0x14 this module uses.
class AnimInfoK
{
public:
	ModelAnimK* m_pAnim;                     // 0x00
	void*       m_pChildModel;               // 0x04
	LTVector    m_vDims;                     // 0x08
	LTVector    m_vTranslation;              // 0x14  NEW (see above)
};

class ModelWeightSetK
{
public:
	char    m_Name[0x10];                    // 0x00
	uint32  m_nAllocedWeights;               // 0x10
	float*  m_pWeights;                      // 0x14
	uint8   m_pad_18[0x28 - 0x18];
};

class ModelNodeK
{
public:
	uint8    m_pad_00[0x04];
	LTVector m_vOffset;                      // 0x04  read when m_Flags&2
	// 0x10 -- the node's own index, uint16.  Every walker widens it with
	// `xor %exx,%exx` before `mov 0x10(..),%xx` (0x00489ae5, 0x0048a471,
	// 0x0048a498), so it is a uint16 field read as a uint32 value.
	uint16   m_iNode;                        // 0x10
	uint16   m_Flags;                        // 0x12  bit 1 tested at 0x0048a357
	uint8    m_pad_14[0x1c - 0x14];
	ModelNodeK** m_pChildren;                // 0x1c
	uint32   m_nChildren;                    // 0x20
	uint8    m_pad_24[0x28 - 0x24];
	uint32   m_iParent;                      // 0x28  0xffffffff at the root
	uint8    m_pad_2c[0xb4 - 0x2c];
};

class ModelK
{
public:
	uint8             m_pad_00[0x28];
	// 0x028 -- indexed by node index at 0x0048a33a and at 0x00489b58, which
	// then reads the node's parent out of +0x28 of the node.  In
	// commonmodel.h this is the array pointer of the collection at 0x24.
	ModelNodeK**      m_pNodes;              // 0x028
	uint8             m_pad_2c[0x48 - 0x2c];
	// 0x048 -- indexed by AnimTimeRef::m_iWeightSet at 0x00489c06.  The
	// array pointer of commonmodel.h's collection at 0x44.
	ModelWeightSetK** m_pWeightSets;         // 0x048
	uint8             m_pad_4c[0x74 - 0x4c];
	// 0x074 -- the destination transform table.  0x00489bae copies it into
	// the builder's own +0xe8, and 0x0048a2e9 indexes it `shl $6` = 64 bytes
	// a node, i.e. one LTMatrix each.
	LTMatrix*         m_pTransforms;         // 0x074
	uint8             m_pad_78[0x168 - 0x78];
	// 0x168 -- m_AnimArray's array pointer; every use indexes it `shl $5`,
	// which is commonmodel.h's 0x20-byte AnimInfo.
	AnimInfoK*        m_pAnims;              // 0x168
	uint8             m_pad_16c[0x2c0 - 0x16c];
	// 0x2c0 -- the root node.  0x00489ad7 reads its +0x10 as a uint16 and
	// hands that to the walker as the node to start from.
	ModelNodeK*       m_pRootNode;           // 0x2c0
};


// ----------------------------------------------------------------------- //
//  Callees outside this TU, all cited by address.
// ----------------------------------------------------------------------- //

// 0x00445370 -- AnimTimeRef::IsValid(), declared in ltanimtracker.h and
// already enumerated (`near`, 123 bytes).  Proof it is that and not something
// else: it null-checks m_pModel at +0, then range-checks the uint16 at +4 and
// the uint16 at +0xc against pModel->[0x16c] -- Model::m_AnimArray's count --
// and then reads the uint16 at +6.  That is m_Prev.m_iAnim, m_Cur.m_iAnim,
// m_Prev.m_iFrame of an AnimTimeRef and nothing else.
//   (declared by ltanimtracker.h; nothing to write here)

// 0x00411a30 -- LTRotation::LTRotation(float,float,float,float), the
// four-float constructor (thiscall, `ret $0x10`, four dword stores at
// +0/+4/+8/+0xc).  Still `todo`; it is an SDK
// inline the shipped build kept out of line, exactly like quat_Mul.
// 0x0047c460 -- LTRotation::operator=(const LTRotation&), already
// `identical` (thiscall, `ret $4`, four dword copies, returns `this`).

#define MAX_TM_TIMEREFS  8
#define MAX_NODE_PATH    0x40


// ----------------------------------------------------------------------- //
//  The builder itself.  Every offset below is cited from an instruction.
//  Name ours -- nothing in the image names it, and it has no server twin.
// ----------------------------------------------------------------------- //
class CTransformMakerK
{
public:
	LTBOOL IsValid();                                           // 0x00489a60
	LTBOOL SetupTransforms();                                   // 0x00489ac0
	LTBOOL SetupTransformsForNode(uint32 iNode);                // 0x00489b00
	LTBOOL Prepare();                                           // 0x00489bc0
	void   GetNodeTransform(uint32 iTimeRef, uint32 iNode,
	                        LTRotation *pRot, LTVector *pPos);  // 0x00489ce0
	void   GetNodeTransformRel(uint32 iTimeRef, uint32 iNode,
	                           LTRotation *pRot, LTVector *pPos);// 0x00489eb0
	void   BlendNode(uint32 iTimeRef, uint32 iNode);            // 0x0048a110
	void   ProcessNode(uint32 iNode, LTMatrix *pParent);        // 0x0048a2d0

public:
	// 0x000 -- eight of them, and 8*0x1c = 0xe0 is exactly where the count
	// sits.  0x00489a7d starts the walk at `this` itself and strides 0x1c.
	AnimTimeRef      m_TimeRefs[MAX_TM_TIMEREFS];    // 0x000
	uint32           m_nTimeRefs;                    // 0x0e0
	// 0x0e4 -- the transform the root node hangs off.  Null on entry to
	// Prepare means "use identity", and Prepare points it at m_matIdentity.
	LTMatrix*        m_pBaseTransform;               // 0x0e4
	// 0x0e8 -- where the finished 4x4s go, one per node; 0x0048a2e9 indexes
	// it `shl $6`.  Null means "the model's own table".
	LTMatrix*        m_pTransforms;                  // 0x0e8
	// 0x0ec..0x0f4 -- ltbasedefs.h's NodeControlFn and its two carried
	// values.  0x0048a425 calls it __cdecl with, in argument order,
	// [0x0f4], the node index, the node's finished matrix and [0x0f0] --
	// which is (HOBJECT, HMODELNODE, LTMatrix*, void*) exactly.
	NodeControlFn    m_NodeControlFn;                // 0x0ec
	void*            m_pNodeControlData;             // 0x0f0
	HOBJECT          m_hNodeControlObject;           // 0x0f4
	// 0x0f8/0x0fc -- the optional node path.  SetupTransformsForNode fills
	// it from a 0x40-entry stack array and ProcessNode walks it downwards.
	uint32*          m_pNodePath;                    // 0x0f8
	uint32           m_nNodePath;                    // 0x0fc
	CModelXform*     m_pCurXform;                    // 0x100
	LTMatrix         m_matXform;                     // 0x104
	void*            m_pAnimOwner;                   // 0x144
	LTMatrix         m_matNode;                      // 0x148
	// 0x188/0x198 -- the blend accumulators ProcessNode seeds from time-ref
	// zero and BlendNode folds the other seven into.
	LTRotation       m_rBlend;                       // 0x188
	LTVector         m_vBlend;                       // 0x198
	ModelK*          m_pModel;                       // 0x1a4
	LTMatrix         m_matIdentity;                  // 0x1a8
	ModelWeightSetK* m_pWeightSets[MAX_TM_TIMEREFS]; // 0x1e8
	ModelAnimK*      m_pPrevAnims[MAX_TM_TIMEREFS];  // 0x208
	ModelAnimK*      m_pCurAnims[MAX_TM_TIMEREFS];   // 0x228
};

// 0x004b4d30 -- a file-scope LTMatrix in .data that ProcessNode uses as the
// destination of its first MatMul and the left operand of its second.  It is
// written unconditionally before it is read, so it needs no initialiser; that
// it lives in .data rather than .bss says the original had one anyway, and a
// zeroed one is the only initialiser consistent with a matrix that is always
// overwritten.  (Name ours.)
static LTMatrix g_matTMTemp;


// ----------------------------------------------------------------------- //
//  0x00489a60 -- IsValid.  91 bytes, thiscall, no arguments (`ret`).
//
//  WHAT PINS IT
//    * two guards on m_nTimeRefs to the SAME return-0 tail: `test %eax,%eax;
//      je` and `cmp $8,%eax; ja` -- one `||` condition, and the 8 is the
//      array bound.
//    * the walk starts at `this` (0x00489a7d `mov %ebx,%edi`) and strides
//      0x1c, so it indexes m_TimeRefs from element 0.
//    * 0x00489a81 calls 0x00445370 thiscall on the element -- AnimTimeRef's
//      own IsValid.
//    * 0x00489a8a compares the element's +0 with the FIRST element's +0
//      (%ebp, loaded once before the loop): every tracker must be on the
//      same model.
//    * 0x00489a8e `test %esi,%esi; je` skips the weight-set range check on
//      element ZERO -- the test is written for the case that CONTINUES, so
//      the source guard is `i != 0 &&`.  0x4c is the weight-set count beside
//      the array at 0x48.
//    * m_nTimeRefs is re-loaded at the bottom of every iteration
//      (0x00489a9c), which is what a member read in the loop condition gives.
//  CONFIDENCE: high on shape, high on calls.
// ----------------------------------------------------------------------- //
LTBOOL CTransformMakerK::IsValid()
{
	ModelK *pModel;
	uint32 i;

	if (m_nTimeRefs == 0 || m_nTimeRefs > MAX_TM_TIMEREFS)
		return LTFALSE;

	pModel = (ModelK*)m_TimeRefs[0].m_pModel;

	for (i = 0; i < m_nTimeRefs; i++)
	{
		if (!m_TimeRefs[i].IsValid())
			return LTFALSE;

		if ((ModelK*)m_TimeRefs[i].m_pModel != pModel)
			return LTFALSE;

		if (i != 0 && m_TimeRefs[i].m_iWeightSet >= *(uint32*)((uint8*)pModel + 0x4c))
			return LTFALSE;
	}

	return LTTRUE;
}
// UNDECODED: the 0x4c read is spelled as a raw offset because ModelK above
// flattens the collection at 0x44 into a bare array pointer; with
// commonmodel.h's Model it is `pModel->m_WeightSets.m_nElements`.


// ----------------------------------------------------------------------- //
//  0x00489ac0 -- SetupTransforms.  54 bytes, thiscall, `ret`.
//
//  WHAT PINS IT
//    * `call 0x489bc0` (Prepare) first, then `test %eax,%eax; jne` over a
//      bare `pop %esi; ret`: the failure path returns the ZERO already in
//      %eax, which is what `if (!Prepare()) return LTFALSE;` gives.
//    * `xor %eax,%eax; mov 0x10(%edx),%ax` on m_pModel->m_pRootNode is a
//      uint16 field widened to uint32 -- the root node's index.
//    * two pushes then `call 0x48a2d0` with %ecx = this: ProcessNode(index,
//      m_pBaseTransform), the base transform pushed first so it is argument 2.
//    * `mov $1,%eax` before the ret -> `return LTTRUE;`.
//  CONFIDENCE: high; the call sequence is two calls and both are pinned.
// ----------------------------------------------------------------------- //
LTBOOL CTransformMakerK::SetupTransforms()
{
	if (!Prepare())
		return LTFALSE;

	ProcessNode(m_pModel->m_pRootNode->m_iNode, m_pBaseTransform);

	return LTTRUE;
}


// ----------------------------------------------------------------------- //
//  0x00489b00 -- SetupTransformsForNode.  180 bytes, thiscall, `ret $4`.
//
//  WHAT PINS IT
//    * frame 0x100 = 0x40 dwords, and the loop's own bound is `cmp $0x40`.
//      One local array, nothing else.
//    * the node index is range-checked against m_pModel->[0x78] -- the count
//      beside the transform array at 0x74, i.e. one transform per node.
//    * the counter IS the member: 0x00489b47 re-reads 0xfc(%esi) at the top
//      of every iteration and 0x00489b52 writes it back after the increment.
//      A local counter copied out at the end would not do that.
//    * the loop exits on the parent index being 0xffffffff (`je`) and only
//      THEN tests the 0x40 bound (`jb` back), so the overflow return is the
//      fall-through of the second test -- the bound check is the LAST
//      statement of the body, not the loop condition.
//    * after the loop `lea 0x8(%esp),%eax` (the array) goes to m_pNodePath
//      and the count is decremented once before being stored back.
//    * the tail is SetupTransforms' tail verbatim.
//  CONFIDENCE: high on shape and calls.
// ----------------------------------------------------------------------- //
LTBOOL CTransformMakerK::SetupTransformsForNode(uint32 iNode)
{
	uint32 nodePath[MAX_NODE_PATH];

	if (!Prepare())
		return LTFALSE;

	if (iNode >= *(uint32*)((uint8*)m_pModel + 0x78))
		return LTFALSE;

	m_nNodePath = 0;
	while (1)
	{
		nodePath[m_nNodePath] = iNode;
		m_nNodePath++;

		iNode = m_pModel->m_pNodes[iNode]->m_iParent;
		if (iNode == 0xffffffff)
			break;

		if (m_nNodePath >= MAX_NODE_PATH)
			return LTFALSE;
	}

	m_pNodePath = nodePath;
	m_nNodePath--;

	ProcessNode(m_pModel->m_pRootNode->m_iNode, m_pBaseTransform);

	return LTTRUE;
}


// ----------------------------------------------------------------------- //
//  0x00489bc0 -- Prepare.  285 bytes, thiscall, `ret`.
//
//  WHAT PINS IT
//    * `call 0x489a60` first, same `if (!IsValid()) return LTFALSE;` shape.
//    * `mov (%esi),%eax; mov %eax,0x1a4(%esi)` -- m_pModel is cached from
//      m_TimeRefs[0].m_pModel, which is why IsValid insisted every record
//      carried the same one.
//    * m_pNodePath is cleared (0x00489be3) BEFORE the loop; %edi stays zero
//      for the whole function and doubles as the loop's lower bound and as
//      the 0.0f the identity matrix is filled with.
//    * the loop caches three pointers per time-ref:
//        m_pWeightSets[i] = pModel->m_pWeightSets[m_TimeRefs[i].m_iWeightSet]
//          -- guarded by `if (i > 0)` (0x00489bf6, again the continue case),
//             so slot 0 is deliberately left alone: the first weight set is
//             ignored, exactly as ltanimtracker.h says of m_iWeightSet.
//        m_pPrevAnims[i] = pModel->m_pAnims[m_TimeRefs[i].m_Prev.m_iAnim].m_pAnim
//        m_pCurAnims[i]  = pModel->m_pAnims[m_TimeRefs[i].m_Cur.m_iAnim].m_pAnim
//      The three destinations are walked off ONE induction pointer (%eax,
//      started at this+0x208 and bumped by 4) at -0x20/-0x4/+0x1c, which is
//      what fixes the three arrays at 0x1e8, 0x208 and 0x228 and says they
//      are adjacent and parallel.
//    * m_pModel is re-loaded from the member three times inside the loop --
//      the member is read afresh in each statement, not cached in a local.
//    * 0x00489c63..0x00489c94 writes 1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1 in
//      strict offset order: LTMatrix::Identity() -> Init(...) inlined.
//    * the last three statements read m_TimeRefs[0].m_Prev.m_iAnim again and
//      take AnimInfo's +4 (m_pChildModel) into m_pAnimOwner.
//  CONFIDENCE: high on shape and calls; the two raw-offset reads below are
//  the only places the flattened ModelK shows through.
// ----------------------------------------------------------------------- //
LTBOOL CTransformMakerK::Prepare()
{
	uint32 i;

	if (!IsValid())
		return LTFALSE;

	m_pModel = (ModelK*)m_TimeRefs[0].m_pModel;
	m_pNodePath = LTNULL;

	for (i = 0; i < m_nTimeRefs; i++)
	{
		if (i > 0)
			m_pWeightSets[i] = m_pModel->m_pWeightSets[m_TimeRefs[i].m_iWeightSet];

		m_pPrevAnims[i] = m_pModel->m_pAnims[m_TimeRefs[i].m_Prev.m_iAnim].m_pAnim;
		m_pCurAnims[i]  = m_pModel->m_pAnims[m_TimeRefs[i].m_Cur.m_iAnim].m_pAnim;
	}

	if (!m_pBaseTransform)
	{
		m_matIdentity.Identity();
		m_pBaseTransform = &m_matIdentity;
	}

	if (!m_pTransforms)
		m_pTransforms = m_pModel->m_pTransforms;

	m_pAnimOwner = m_pModel->m_pAnims[m_TimeRefs[0].m_Prev.m_iAnim].m_pChildModel;

	return LTTRUE;
}


// ----------------------------------------------------------------------- //
//  0x00489ce0 -- GetNodeTransform.  460 bytes, thiscall, `ret $0x10`.
//
//  FOUR ARGUMENTS, and the stack arithmetic is what proves the third and
//  fourth are different: 0x00489d49 reads 0x44(%esp) while %esp is E-0x38
//  (one reserve push has happened) -> E+0xc, but 0x00489dcf reads the SAME
//  0x44(%esp) while %esp is E-0x34 -> E+0x10.  Miss the reserve push and the
//  quaternion and the position look like the same out-parameter.
//
//  WHAT PINS IT
//    * `push %ecx` at 0x00489d1d followed by `fstps (%esp)` at 0x00489d43 is
//      MSVC's push-a-float idiom: the %ecx value is thrown away, the slot is
//      the argument.  With the three pointer pushes that makes FOUR __cdecl
//      arguments to 0x00444b20 -- quat_Slerp(float*, const float*,
//      const float*, float), already `identical` -- and `add $0x10,%esp`
//      confirms the count.
//    * the two source pointers are built identically from m_pPrevAnims and
//      m_pCurAnims: ->+4 (the node-array pointer), [iNode], ->+0xc (the
//      transform table), then + m_iFrame * 0x1c.  +0xc of that record is
//      what goes to quat_Slerp, +0 is what gets lerped: CModelXform.
//    * the position is `a + (b-a)*t` through _CVector's OPERATORS, not
//      VEC_LERP: `b - a` takes its right operand by value, and that 12-byte
//      copy of `a` is the three integer movs to 0x10/0x14/0x18(%esp) at
//      0x00489d62; `* t` copies t the same way (0x00489d68 -> 0x38(%esp));
//      the sum lands in a temp that `*pPos =` copies out with three integer
//      movs through a re-materialised pointer (`mov %eax,%ecx`).
//    * the float t reaches quat_Slerp through ltrotation.h's inline
//      LTRotation::Slerp: its parameter is what makes VC6 push t with
//      `flds 0x14(%esi)` ... `push %ecx; fstps (%esp)` instead of a plain
//      dword push (a direct quat_Slerp call gives `mov; push`, 3 bytes
//      short).
//    * `test %ecx,%ecx; jne <epilogue>` on the NODE INDEX: the extra block
//      runs only for node 0, and the test is written for the case that
//      SKIPS, so the source guard is `if (iNode == 0)`.
//    * that block lerps AnimInfo+0x14 of the two animations -- an LTVector
//      the current commonmodel.h has inside a pad -- the same operator way
//      (the by-value copy of `a` at 0x00489e21), and `+=`s it onto *pPos
//      (`flds temp; fadds (%eax); fstps (%eax)` x3).
//  STATUS: size-exact (460/460), every instruction after the quat_Slerp
//  call identical.  The residual is the head's register naming only: the
//  shipped code loads iTimeRef into %eax before the callee-save pushes and
//  interleaves the prev/cur pointer chains with iNode in %ecx; ours has
//  iTimeRef in %ecx and iNode in %edx.  Named node / anim locals, no pRef,
//  pRef declared last, assignments split from declarations and an extra
//  local all leave it unchanged -- allocation.
// ----------------------------------------------------------------------- //
void CTransformMakerK::GetNodeTransform(uint32 iTimeRef, uint32 iNode,
                                        LTRotation *pRot, LTVector *pPos)
{
	AnimTimeRef  *pRef  = &m_TimeRefs[iTimeRef];
	CModelXform  *pPrev = &m_pPrevAnims[iTimeRef]->m_pAnimNodes[iNode]
	                       ->m_pXforms[pRef->m_Prev.m_iFrame];
	CModelXform  *pCur  = &m_pCurAnims[iTimeRef]->m_pAnimNodes[iNode]
	                       ->m_pXforms[pRef->m_Cur.m_iFrame];

	pRot->Slerp(pPrev->m_Rot, pCur->m_Rot, pRef->m_Percent);

	*pPos = pPrev->m_Pos + (pCur->m_Pos - pPrev->m_Pos) * pRef->m_Percent;

	if (iNode == 0)
	{
		AnimInfoK *pAnims = m_pModel->m_pAnims;
		LTVector  &vPrev  = pAnims[pRef->m_Prev.m_iAnim].m_vTranslation;
		LTVector  &vCur   = pAnims[pRef->m_Cur.m_iAnim].m_vTranslation;

		*pPos += vPrev + (vCur - vPrev) * pRef->m_Percent;
	}
}


// ----------------------------------------------------------------------- //
//  0x00489eb0 -- GetNodeTransformRel.  462 bytes, thiscall, `ret $0x10`.
//
//  The same four arguments and the same slerp/lerp head as 0x00489ce0 --
//  identical instruction for instruction down to the register roles -- and
//  then, instead of the node-0 block, it takes the result back into the
//  animation's own frame-zero pose.
//
//  WHAT PINS THE TAIL
//    * %esi still holds the PREV animation's transform TABLE BASE (0x00489ef6
//      `mov 0xc(%esi),%esi`), i.e. element ZERO, not the current frame.  The
//      tail reads +0xc/+0x10/+0x14/+0x18 of it: that element's m_Rot.
//    * 0x00489fe1 calls 0x00411a30 -- LTRotation's four-float constructor --
//      with (-m_Rot[0], -m_Rot[1], -m_Rot[2], +m_Rot[3]).  ltrotation.h's
//      Conjugate() is `return LTRotation(-m_Quat[QX], -m_Quat[QY],
//      -m_Quat[QZ], m_Quat[QW]);` and operator~ is `return Conjugate();`.
//    * TWO calls to 0x0047c460 (LTRotation::operator=) follow the
//      constructor, which is exactly the two by-value returns that `~x`
//      costs: Conjugate's return into operator~'s slot, and operator~'s
//      return into the destination.  One `operator~` in the source, two
//      copies in the object code -- that count is the tell, and a direct
//      `LTRotation r(-x,-y,-z,w)` would produce neither.
//    * 0x0048a009 calls quat_Mul (0x0048a080) into a temp with the conjugate
//      on the LEFT and *pRot on the right, then copies the four dwords back
//      over *pRot: `*pRot = rBase * *pRot;` through ltrotation.h's
//      out-of-line operator*.
//    * the position tail is three `flds (%edi); fsubs <spill>` pairs against
//      element zero's m_Pos: `*pPos -= pBase->m_Pos;`.
//  CONFIDENCE: high on the call sequence (quat_Slerp, LTRotation ctor,
//  operator= twice, quat_Mul -- five calls, in that order).
//
//  MEASURED: the t pushed through `flds; fstps (%esp)` (0x00489f07/
//  0x00489f1e) is LTRotation::Slerp's float parameter -- a direct
//  quat_Slerp call pushes the member with an integer mov.  The position is
//  LTVector operators (as BlendNode), which spends the inline budget so the
//  LTRotation ctor and quat_Mul stay calls.  The two 0x0047c460 calls are
//  the COPY CONSTRUCTOR (Conjugate's and operator~'s returns) and there is
//  no named rBase: with one, a third 16-byte copy appears.  Size-exact
//  (462/146); the residual is the head's register allocation (iTimeRef in
//  %eax vs %edx, pCur spilled to a local vs the iNode slot).  Declaration
//  order, no pRef local, and assignment-form locals all give the same.
// ----------------------------------------------------------------------- //
void CTransformMakerK::GetNodeTransformRel(uint32 iTimeRef, uint32 iNode,
                                           LTRotation *pRot, LTVector *pPos)
{
	AnimTimeRef  *pRef  = &m_TimeRefs[iTimeRef];
	CModelXform  *pBase = m_pPrevAnims[iTimeRef]->m_pAnimNodes[iNode]->m_pXforms;
	CModelXform  *pPrev = &pBase[pRef->m_Prev.m_iFrame];
	CModelXform  *pCur  = &m_pCurAnims[iTimeRef]->m_pAnimNodes[iNode]
	                       ->m_pXforms[pRef->m_Cur.m_iFrame];

	pRot->Slerp(pPrev->m_Rot, pCur->m_Rot, pRef->m_Percent);

	*pPos = pPrev->m_Pos + (pCur->m_Pos - pPrev->m_Pos) * pRef->m_Percent;

	*pRot = ~pBase->m_Rot * *pRot;

	*pPos = *pPos - pBase->m_Pos;
}


// ----------------------------------------------------------------------- //
//  0x0048a110 -- BlendNode.  433 bytes, thiscall, `ret $8`.
//
//  WHAT PINS IT
//    * the weight is `m_pWeightSets[iTimeRef]->m_pWeights[iNode]`, loaded
//      with `flds` -- ModelWeightSet's weight array holds FLOATS, which
//      corrects commonmodel.h's `CMoArray<uint32,...> m_Weights`.
//    * two `fcomps`/`test $0x40,%ah` pairs, so two EQUALITY tests and no
//      ordering test.  The constants are read out of .rdata:
//        0x004a14d8 = 0x00000000 = 0.0f   -> `if (fWeight == 0.0f) return;`
//        0x004a1a68 = 0x40000000 = 2.0f   -> `if (fWeight == 2.0f) {...}`
//      2.0f, not 1.0f.  The dword was read rather than assumed, because a
//      weight compared against two is the surprising half of this function;
//      0x004a1a68 is the same pool constant quat_ConvertToMatrix divides by,
//      which is what makes it easy to misread as a scale factor.
//    * the 2.0f arm calls GetNodeTransformRel and then
//      `m_rBlend = m_rBlend * rot; m_vBlend += vPos;` -- an ACCUMULATE.
//    * the other arm calls GetNodeTransform and then blends by weight:
//      quat_Slerp(m_rBlend, <a copy of m_rBlend>, rot, fWeight) -- the copy
//      is four dwords onto the stack at 0x0048a1f2, because quat_Slerp's
//      destination aliases its first source and the source-level spelling
//      passes the old value; and VEC_LERP(m_vBlend, m_vBlend, vPos,
//      fWeight).
//    * the weight is spilled into the INCOMING iTimeRef slot (0x0048a12b
//      `fsts 0x44(%esp)`) and pushed from there as quat_Slerp's t.  MSVC
//      reuses the caller-provided argument area as scratch; reading that as
//      "the index is passed as t" is the trap here.
//    * the vector halves are LTVector OPERATORS, not VEC_LERP / component
//      adds: `m_vBlend + vPos` and `m_vBlend + (vPos - m_vBlend) * t`
//      (the right operand of `-` is the by-value copy at 0x0048a23e).
//      Those inline expansions spend the /O2 inline budget, which is why
//      operator*'s quat_Mul is CALLED (0x0048a17a) rather than expanded.
//  CONFIDENCE: byte-identical (lithtech.exe).
// ----------------------------------------------------------------------- //
void CTransformMakerK::BlendNode(uint32 iTimeRef, uint32 iNode)
{
	float      fWeight;
	LTRotation rot, rOld;
	LTVector   vPos;

	fWeight = m_pWeightSets[iTimeRef]->m_pWeights[iNode];
	if (fWeight == 0.0f)
		return;

	if (fWeight == 2.0f)
	{
		GetNodeTransformRel(iTimeRef, iNode, &rot, &vPos);

		m_rBlend = m_rBlend * rot;
		m_vBlend = m_vBlend + vPos;
	}
	else
	{
		GetNodeTransform(iTimeRef, iNode, &rot, &vPos);

		rOld = m_rBlend;
		quat_Slerp(m_rBlend.m_Quat, rOld.m_Quat, rot.m_Quat, fWeight);

		m_vBlend = m_vBlend + (vPos - m_vBlend) * fWeight;
	}
}


// ----------------------------------------------------------------------- //
//  0x0048a2d0 -- ProcessNode.  475 bytes, thiscall, `ret $8`.
//
//  THE RECURSION IS AN EXPLICIT LOOP.  Two `jmp 0x0048a2e1` (at 0x0048a44a
//  and 0x0048a49c) jump back to the instruction AFTER the prologue with %edi
//  and %ebp reloaded.  Written as two tail recursions VC6 still folds them
//  (477 bytes, &m_rBlend CSE'd into %ebp); written as the loop -- the form
//  server.dll's twin, src/engine/server/worldtree.cpp 0x10038c4d, is
//  byte-identical with -- it is byte-identical here too.  One `ret` for
//  four exits.
//
//  FRAME PROOF: 0x24 = 36 bytes = THREE 12-byte LTVector by-value copies,
//  and there are exactly three SetTranslation(LTVector) calls below.  Nothing
//  else is on the frame; the two matrices are members and the scratch is the
//  .data one.
//
//  WHAT PINS THE BODY
//    * seed: `push $0` then ProcessNode's own iNode -> GetNodeTransform(0,
//      iNode, &m_rBlend, &m_vBlend).  Time-ref ZERO is the base pose and is
//      written straight into the accumulators -- which is why IsValid skips
//      element 0's weight-set check and Prepare skips its m_pWeightSets slot.
//    * blend loop from index ONE (`mov $1,%ebx`) to m_nTimeRefs, re-reading
//      the member each turn.
//    * `quat_ConvertToMatrix(m_rBlend.m_Quat, m_matNode.m)`, then
//      `testb $2,0x12(%eax)` on the node record picks WHICH vector the
//      translation comes from.  Both arms store m[0][3] and m[1][3]
//      themselves and the m[2][3] store is hoisted past the join
//      (0x0048a3a7), which is the compiler merging one call written twice
//      with different arguments -- so it is two SetTranslation calls in two
//      arms, not one call on a selected pointer.
//    * MatMul(&g_matTMTemp, pParent, &m_matNode) -- the parent on the LEFT.
//    * then m_pCurXform = &((CModelXform*)(m_pAnimOwner+4))[iNode], its
//      quaternion to m_matXform, its position as m_matXform's translation,
//      and MatMul(&m_pTransforms[iNode], &g_matTMTemp, &m_matXform).
//      `add $0x20,%esp` = 8 dwords = 3+2+3 for those two MatMuls and the
//      quat_ConvertToMatrix, which is the whole call count of the block.
//    * the node-control callback is __cdecl through the member pointer with
//      (m_hNodeControlObject, iNode, &m_pTransforms[iNode],
//      m_pNodeControlData) -- ltbasedefs.h's NodeControlFn signature.
//    * recursion: if m_pNodePath is set, walk it DOWNWARDS one step per
//      level; otherwise recurse on every child, the last one tail-folded.
//  CONFIDENCE: medium.  The call sequence is verified instruction by
//  instruction; the loop-folded control flow is the part most likely to
//  need a compile-and-diff to settle.
//
//  BYTE-IDENTICAL, 475 bytes: the server twin's loop shape --
//  m_pCurXform assigned first and pXform read back from it, the path arm
//  as `if (m_nNodePath > 0) {..} else return;`, the children walk with its
//  own counter, and the literal `m_nNodePath = 1` store (0x0048a484) on the
//  children path before the last child is taken as the next iNode.
// ----------------------------------------------------------------------- //
void CTransformMakerK::ProcessNode(uint32 iNode, LTMatrix *pParent)
{
	LTMatrix    *pTransform;
	ModelNodeK  *pNode;
	CModelXform *pXform;
	uint32       i, iChild, nLast;

	for (;;)
	{
		pTransform = &m_pTransforms[iNode];

		GetNodeTransform(0, iNode, &m_rBlend, &m_vBlend);

		for (i = 1; i < m_nTimeRefs; i++)
			BlendNode(i, iNode);

		pNode = m_pModel->m_pNodes[iNode];

		quat_ConvertToMatrix(m_rBlend.m_Quat, m_matNode.m);

		if (pNode->m_Flags & 2)
			m_matNode.SetTranslation(pNode->m_vOffset);
		else
			m_matNode.SetTranslation(m_vBlend);

		MatMul(&g_matTMTemp, pParent, &m_matNode);

		m_pCurXform = &((CModelXform*)(*(uint8**)((uint8*)m_pAnimOwner + 4)))[iNode];
		pXform = m_pCurXform;

		quat_ConvertToMatrix(pXform->m_Rot.m_Quat, m_matXform.m);
		m_matXform.SetTranslation(pXform->m_Pos);

		MatMul(pTransform, &g_matTMTemp, &m_matXform);

		if (m_NodeControlFn)
			m_NodeControlFn(m_hNodeControlObject, iNode, pTransform,
			                m_pNodeControlData);

		if (m_pNodePath)
		{
			if (m_nNodePath > 0)
			{
				m_nNodePath--;
				pParent = pTransform;
				iNode = m_pNodePath[m_nNodePath];
			}
			else
				return;
		}
		else
		{
			nLast = pNode->m_nChildren;
			if (!nLast)
				return;
			nLast--;

			for (iChild = 0; iChild < nLast; iChild++)
				ProcessNode(pNode->m_pChildren[iChild]->m_iNode, pTransform);

			// 0x0048a484 `movl $0x1,0xfc(%esi)`: on this path m_pNodePath
			// is null and the value is never read, but the store is in the
			// bytes of both images (server 0x10038df6).
			m_nNodePath = 1;
			iNode = pNode->m_pChildren[nLast]->m_iNode;
			pParent = pTransform;
		}
	}
}


// ####################################################################### //
//  SECTION C -- 0x0048a080, quat_Mul.  136 bytes, __cdecl, `ret`.
//
//  THE ONE ROW THAT MATCHES server.dll.  All 136 bytes of it appear verbatim
//  at 0x10035468 in the retail server.dll, and src/engine/server/
//  servermath.cpp has already identified that copy as ltquatbase.h's
//  `quat_Mul` inline, emitted out of line and scored `identical`.  Nothing
//  needs writing: the COMDAT VC6 emits for the header's own inline IS the
//  shipped body, term order, operand order and `fsubrp` polarities included.
//
//  Reading it as a library would be an error.  It is
//  one straight-line x87 leaf -- no branches, no relocations, no frame, no
//  call -- and that is precisely the shape /O1 and /O2 agree on.  The other
//  ten rows in this cluster, searched the same way, hit ZERO windows in
//  server.dll, and 0x00489630..0x00489a5a's whole ILTTransform class is
//  absent from server.dll because the server implements ILTCSBase's math
//  through CLTServerMath instead.  This is client code that happens to pull
//  the same SDK header.
//
//  Its real call sites are 0x0048a009 (GetNodeTransformRel's
//  `*pRot = rBase * *pRot`) and 0x0048a17a (BlendNode's
//  `m_rBlend = m_rBlend * rot`), both of which are ltrotation.h's
//  `operator*` -- so the two calls above already force this COMDAT and the
//  driver below is only for building section C standalone.  Like
//  servermath.cpp's servermath_ForceSDKInlines and packet.cpp's
//  packet_ForceInstantiation, it exists in no shipped binary and is not
//  itself reconstruction.
// ####################################################################### //
void transform_ForceSDKInlines(LTRotation *pOut, LTRotation *pA, LTRotation *pB)
{
	quat_Mul(pOut->m_Quat, pA->m_Quat, pB->m_Quat);
}


// ####################################################################### //
//  NOT IN THIS FILE, and why.
//
//  0x0048a4b0 (85 bytes) and 0x0048a510 (real extent 0x0048a510..0x0048a588,
//  121 bytes -- the enumerated 36 is wrong, see the note at the top of
//  section B) are a THIRD module: a version-number pair.  0x0048a510 does
//  FindResourceA(hModule, MAKEINTRESOURCE(0x303a), RT_RCDATA), LoadResource,
//  SizeofResource (which it requires to be exactly 8), LockResource, and
//  copies the two dwords out; 0x0048a4b0 sprintfs them with "%lu.%lu"
//  (0x004af4cc) into a 0x200-byte buffer and strncpy-truncates the result
//  into the caller's.  Neither has a call edge to anything in sections A, B
//  or C, and their callers (0x00403462, 0x0040facf, 0x0042e7bc) are
//  elsewhere entirely.  They are a separate reconstruction.
//
//  0x0048a534 (23) and 0x0048a54b (62) are NOT functions: both are branch
//  targets inside 0x0048a510's chain of early-outs (`jne 0x48a534` at
//  0x0048a529, `jne 0x48a54b` at 0x0048a540).  They are the enumeration
//  defect "the fall-through of an entry test", and they must
//  not be claimed as rows.
// ####################################################################### //
