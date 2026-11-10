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
//  ONE OF TWO TRANSLATION UNITS (AvP2 1.0.9.6 addresses):
//    A. 0x00489630..0x004898c0  CLTTransform -- the client's ILTTransform:
//       this file.
//    B. 0x00489a60..0x0048a4ab  the model-node transform builder:
//       transformmaker.cpp.
//  They are separated by the 0x00489a5a padding run and share no call edge;
//  in Primal Hunt each ends in its own <string> guard pair (0x004a6990 here,
//  0x004a73c0 there).
// *********************************************************************** //

#if defined(PRIMAL_HUNT) && !defined(LT_SERVER_BUILD)
// PH: Primal Hunt's engine saw the STL's <string> here, through a header
// not yet identified (AvP2 1.0.9.6's did not): its translation unit ends
// in <string>'s guarded initializer of std::ctype<unsigned short>::id
// (guard 0x004f8014, atexit of the empty 0x00401790; 0x4a6990).
#include <string>
#endif
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
