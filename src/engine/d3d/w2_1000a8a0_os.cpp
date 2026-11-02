// *********************************************************************** //
//
//  w2_1000a8a0_os.cpp -- sub_1000a8a0 (written for the d3d.ren link).
//
//  Transform a run of 0x28-byte vertices through the transform object
//  (g_1008b880, handed over as the third argument) and project them:
//
//    * g_1008b874 == 0 (no clipping): one projective transform per vertex
//      through the object's +0x1dc..+0x218 block, the three results
//      assembled in a local vector (`rep movsl` x3) and 1/w into +0x0c --
//      the same expression big124a4.cpp (0x10012a65) and big6d910.cpp
//      (0x1006da68) carry inline, written in their spelling;
//    * otherwise: each position through the 3x4 at +0x15c
//      (sub_1000a770), the clipper sub_1000aa90 on g_1008b874 (RE-READ,
//      as big6d910.cpp notes for its inline copy), and a FALSE from it
//      is returned; then the perspective divide through +0x31c..+0x330.
//
//  The fourth argument is never read.  d3dr2d.cpp's 0x1006d8e0 passes
//  (&pVerts, &nVerts, g_1008b880, 0).
//
//  /Os: frameless with pushes, `xor %eax,%eax; inc %eax` for TRUE, 0x90
//  padding.
//
//  BYTE-IDENTICAL now: the last 4 bytes (the z row's product order) closed
//  with MatVMulH_a8a0 below.  History, when it was a near --
//  NEAR (131 bytes, same size and instruction count): the 1/w row's
//  `(float)(...)` cast with matrix-first products is what loads 0x214
//  first as shipped (measured); what remains is the scheduling of the z
//  row's three products (shipped z, x, y; no term order or double
//  promotion of that row reproduces it) and %ecx vs %edx for the count of
//  the last loop.
//  The %ecx/%edx residual and the layout of the last loop
//  close with that loop written as a `for` whose step is
//  `nLeft--, pWalk++` (the pointer bump in the loop header, not the
//  body): the count is then loaded straight into %ecx and the loop is
//  entered with `jmp` to its body, as shipped.  Measured 131 -> 6 bytes;
//  `while (nLeft--)`, a do/while, a plain `for` with the bump in the body
//  and an int32 counter all stay at 131.  The 6 left are the z row's
//  x and y products (shipped z, x, y; ours z, y, x).
//
//  Measured again, all byte-for-byte unchanged unless noted:
//  the z row as `(float)(...)`, matrix-first, the +0x208 term first, the
//  reciprocal on the left, and regrouped (icl canonicalises the sum --
//  every term order of that row compiles to the same bytes); the three
//  rows in the other five statement orders (worse, 139..148 bytes); the
//  last loop with its own counter, as a `for`, and with its two loads
//  swapped; and the function in a TU with its shipped neighbours
//  (0x1000a8a0 with 0x1000a7d0 and d3dr3b.cpp's 0x1000aa90 around it;
//  0x1000f2c0 with d3dr3c_os.cpp's 0x1000f4b0, and inside the /Os /Qip
//  lightmap TU w30_1000f020_osqipni.cpp).  The same class as 0x10023230
//  (w4_10023fb0.cpp): icl's sum order follows TU history.
//
//  The +0x1dc..+0x218 block is an LTMatrix (the real SDK
//  type) and the first loop is ltmatrix.h's MatVMul_H(&vScreen, &matrix,
//  &pos) -- one_over_w first, matrix-first products, `one_over_w * (...)`
//  -- returning 1/w into the double spill as shipped: 6 -> 4 bytes.  The
//  z row now comes out y, x, z (shipped z, x, y; the hand-written float
//  fields gave z, y, x).  Measured at 4 and not moving: MatVMul_InPlace_H
//  (same bytes), all six term orders of the z row written out over
//  pMat->m[2][*], LTMatrix::Apply4x4's vertex-first spelling, the vertex
//  passed as (LTVector*)pWalk, the matrix through a local pointer,
//  vScreen declared in the loop, the 3x4 at +0x15c as float[3][4], the
//  object as a class, an int32 count; the first loop as a `for` with
//  `nLeft--, pWalk++` is 16.  The order is icl's per-TU canonicalisation,
//  so the residual is the same class as 0x1005a550.
//
// *********************************************************************** //

#include "ltbasedefs.h"


extern unsigned long g_1008b874;                            // 0x1008b874

// d3dr3b.cpp's clipper, spelled as that file defines it.
LTBOOL sub_1000aa90(uint32 nFlags, void** ppVerts, uint32* pnVerts);
// w2_1000a770.cpp: a vector through a row-major 3x4.
void sub_1000a770(LTVector* pVec, const float* pMat34);


// The 0x28-byte vertex: position, 1/w.
struct TVert1000a8a0
{
	LTVector m_vPos;                                        // +0x00
	float    m_fRHW;                                        // +0x0c
	uint8    m_pad_10[0x28 - 0x10];                         // +0x10
};

// The transform object: the 3x4 at +0x15c, the projective 4x4 block at
// +0x1dc..+0x218, the perspective terms at +0x31c..+0x330.
struct XForm1000a8a0
{
	uint8   m_pad_000[0x15c];                               // +0x000
	float   m_Mat15c[12];                                   // +0x15c
	uint8   m_pad_18c[0x1dc - 0x18c];                       // +0x18c
	LTMatrix m_Mat1dc;                                      // +0x1dc
	uint8   m_pad_21c[0x31c - 0x21c];                       // +0x21c
	float   m_f31c;                                         // +0x31c
	float   m_f320;                                         // +0x320
	float   m_f324;                                         // +0x324
	float   m_f328;                                         // +0x328
	float   m_f32c;                                         // +0x32c
	float   m_f330;                                         // +0x330
};


// ltmatrix.h's MatVMul_H, with the z row's first and third matrix terms
// read through a float pointer to the matrix.  icl canonicalises the
// term order of a sum, but which product it emits first follows the
// operands' address forms: this spelling gives the shipped z, x, y
// (the header's gives y, x, z).  Found by sweeping m[r][c] / f[k] per
// column x member / v[c] per component (128 spellings, 4 identical).
inline float MatVMulH_a8a0(LTVector *pDest, LTMatrix *pMat, LTVector *pSrc)
{
	const float* f = &pMat->m[0][0];
	float one_over_w = 1.0f / (pMat->m[3][0]*pSrc->x + pMat->m[3][1]*pSrc->y + pMat->m[3][2]*pSrc->z + pMat->m[3][3]);

	pDest->x = one_over_w * (pMat->m[0][0]*pSrc->x + pMat->m[0][1]*pSrc->y + pMat->m[0][2]*pSrc->z + pMat->m[0][3]);
	pDest->y = one_over_w * (pMat->m[1][0]*pSrc->x + pMat->m[1][1]*pSrc->y + pMat->m[1][2]*pSrc->z + pMat->m[1][3]);
	pDest->z = one_over_w * (f[8]*pSrc->x + pMat->m[2][1]*pSrc->y + f[10]*pSrc->z + pMat->m[2][3]);

	return one_over_w;
}


LTBOOL sub_1000a8a0(TVert1000a8a0** ppVerts, uint32* pnVerts,
                    XForm1000a8a0* pM, uint32 nUnused)
{
	TVert1000a8a0* pWalk;
	uint32 nLeft;
	float fOOW;
	LTVector vScreen;

	if (!g_1008b874)
	{
		nLeft = *pnVerts;
		pWalk = *ppVerts;
		while (nLeft--)
		{
			fOOW = MatVMulH_a8a0(&vScreen, &pM->m_Mat1dc, &pWalk->m_vPos);
			pWalk->m_vPos = vScreen;
			pWalk->m_fRHW = fOOW;
			pWalk++;
		}
	}
	else
	{
	pWalk = *ppVerts;
	nLeft = *pnVerts;
	while (nLeft--)
	{
		sub_1000a770(&pWalk->m_vPos, pM->m_Mat15c);
		pWalk++;
	}

	if (!sub_1000aa90(g_1008b874, (void**)ppVerts, pnVerts))
		return 0;

	pWalk = *ppVerts;
	for (nLeft = *pnVerts; nLeft; nLeft--, pWalk++)
	{
		fOOW = 1.0f / pWalk->m_vPos.z;
		pWalk->m_fRHW = fOOW;
		pWalk->m_vPos.x = (pWalk->m_vPos.x * pM->m_f31c + pWalk->m_vPos.z * pM->m_f320) * fOOW;
		pWalk->m_vPos.y = (pWalk->m_vPos.y * pM->m_f324 + pWalk->m_vPos.z * pM->m_f328) * pWalk->m_fRHW;
		pWalk->m_vPos.z = (pWalk->m_vPos.z * pM->m_f32c + pM->m_f330) * pWalk->m_fRHW;
	}
	}
	return 1;
}
