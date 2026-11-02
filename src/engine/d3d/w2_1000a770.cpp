// *********************************************************************** //
//
//  w2_1000a770.cpp -- sub_1000a770 (written for the d3d.ren link).
//
//  Transform a vector in place by a row-major 3x4 matrix (ltmatrix.h's
//  MatVMul rows), with the matrix handed over as a float pointer: the
//  caller, d3dlow.cpp's 0x1000d9b0, passes an object's +0x15c.
//
//  The shipped x87 order is z, x, y with the z result left deepest: x and
//  y are held in temporaries while z is stored straight back (it reads
//  v->z before overwriting it).  The z row's first product is evaluated
//  in double -- the same icl operand-order lever as w2_10003a10.cpp's
//  half-texel sums: `flds 0x8(%edx); fmuls 0x28(%eax)` (vector operand
//  loaded first) is what that promotion gives, and no float spelling or
//  term order does (all 36 permutations measured).
//
// *********************************************************************** //

#include "ltbasedefs.h"


void sub_1000a770(LTVector* pVec, const float* pMat34)
{
	LTMatrix* pMat = (LTMatrix*)pMat34;

	float x = pMat->m[0][0] * pVec->x + pMat->m[0][1] * pVec->y + pMat->m[0][2] * pVec->z + pMat->m[0][3];
	float y = pMat->m[1][0] * pVec->x + pMat->m[1][1] * pVec->y + pMat->m[1][2] * pVec->z + pMat->m[1][3];
	pVec->z = (float)((double)pVec->z * pMat->m[2][2] + pMat->m[2][0] * pVec->x
	                  + pMat->m[2][1] * pVec->y + pMat->m[2][3]);
	pVec->x = x;
	pVec->y = y;
}
