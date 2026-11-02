#include "ltbasedefs.h"

// *********************************************************************** //
//  w29_10027f70.cpp -- 0x10027f70, 148 bytes, 1 caller (0x100268a7).
//  BUILD A 4x4 MATRIX FROM THREE BASIS VECTORS (moved out of
//  d3dr5c.cpp).
//
//  IDENTICAL, two corrections to the decode below:
//    * FLAGS /O2, not d3dr5c.cpp's /Os: the callee-saved registers are
//      mov-saved into the frame (`mov %edi,0x8(%esp)`), not pushed, and the
//      padding is the /O2 `lea` pair.  Measured: /Os 123 bytes off.
//    * 0x1001e140 does not take a matrix by value: it is LTMatrix::Init's
//      SIXTEEN floats (d3dlow.cpp's C1001e140, whose own decode says so),
//      and this function is an Init call on `this` with the vectors'
//      components as arguments, built straight into the argument area.  A
//      local matrix passed by value is copied with `rep movsl` (160 bytes).
//      The unused 0x40 of frame is icl's, not a local.
//
//  WHAT PINS IT:
//    * `ret $0xc` with %ecx live across the body: thiscall, three stack
//      arguments.  The caller sets them with `movl $0x1008bd1c,(%esp)`,
//      `movl $0x1008bd10,0x4(%esp)`, `movl $0x1008bd28,0x8(%esp)` and
//      `lea -0x4b0(%ebp),%ecx`, so all three are pointers to globals.
//    * `add $0xffffffc0,%esp` opens a SIXTY-FOUR byte outgoing area, and
//      0x1001e140 reads its arguments from 0x40(%esp) to 0x7c(%esp) after
//      four pushes and `sub $0x2c` -- sixteen consecutive dwords.  So
//      0x1001e140 takes sixteen dwords, plus `this` in %ecx,
//      and it pops the 0x40 itself (the caller restores nothing).
//    * every element move is an integer `mov`, which is icl copying float
//      lvalues, not integer data.
//    * the three vectors are read at +0x00/+0x04/+0x08 and land in the
//      matrix transposed -- A,B,C across a row, one component per row --
//      so the three arguments are the COLUMNS of the rotation.
//    * `sub $0x58,%esp` is 0x40 of matrix plus 0xc of saved registers plus
//      0xc more; 0x100156e0 is the same function with an identity matrix
//      and reserves 0x48 for the same 0x40 with no saved registers.  So
//      the matrix IS a named local whose copy into the argument area icl
//      elided, and the frame reservation for it survives.
//    * the store order is 0,4,8,c then 0x10,0x14,0x1c,0x18 then
//      0x20,0x24,0x2c,0x28 then 0x30,0x34,0x38,0x3c: row-major, with the
//      row's zero hoisted one slot early twice.  That is a scheduler
//      choice inside a row, not a source order.
//    * `mov %ecx,%edx ; mov %edx,%ecx` round-trips `this` for the call and
//      is icl's spelling for it -- 0x100156e0 has the identical pair.
// *********************************************************************** //


struct Vec10027f70
{
	float	x;
	float	y;
	float	z;
};

// `this` is the matrix: 0x1001e140 is d3dlow.cpp's C1001e140::sub_1001e140
// (LTMatrix::Init), reached through link/aliases.rsp.
struct Obj10027f70
{
	void	sub_1001e140(float m00, float m01, float m02, float m03,
						 float m10, float m11, float m12, float m13,
						 float m20, float m21, float m22, float m23,
						 float m30, float m31, float m32, float m33);
	void	sub_10027f70(const Vec10027f70* pRight,
						 const Vec10027f70* pUp,
						 const Vec10027f70* pForward);
};

void Obj10027f70::sub_10027f70(const Vec10027f70* pRight,
							   const Vec10027f70* pUp,
							   const Vec10027f70* pForward)
{
	sub_1001e140(pRight->x, pUp->x, pForward->x, 0.0f,
				 pRight->y, pUp->y, pForward->y, 0.0f,
				 pRight->z, pUp->z, pForward->z, 0.0f,
				 0.0f, 0.0f, 0.0f, 1.0f);
}
