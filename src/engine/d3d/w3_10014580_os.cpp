// *********************************************************************** //
//
//  w3_10014580_os.cpp -- written for the d3d.ren link.
//
//  Functions in 0x10014580 .. 0x10015b6a that the link was missing: the
//  screen projection, the SDK's out-of-line
//  ltmatrix.h bodies (MatVMul, MatVMul_InPlace_H, MatVMul_3x3,
//  Mat_InverseTransformation, LTMatrix::Identity, LTMatrix::Apply), the
//  light-grid sampler, a vector Init, a getter, the texture-stage binder,
//  and the model-vertex transform hooks (plain and "warble") stored into
//  CD3DModelDraw's +0x41c/+0x420 slots by big17c14.cpp.
//
//  Kept in shipped address order on purpose: icl's x87 operand order
//  depends on the other functions in the TU (0x10015aa0 is identical only
//  with 0x10015320 after it -- measured), so the order is part of the
//  source.
//
//  Flag set /Os (the `_os` suffix): the shipped bodies are padded to four
//  with lea-nops and then 0x90, not to sixteen with multi-byte nops.
//
//  Every other spelling the callers use for these addresses is aliased
//  onto the one definition here in link/aliases.rsp (w3_*.cpp block), each with
//  its address evidence.
//
// *********************************************************************** //

#include "ltbasedefs.h"


// ======================================================================= //
//  Types.  Each TU in src/engine/d3d spells its own; only the tag names
//  matter to the linker.
// ======================================================================= //

// The SDK's LTVector / LTMatrix, under the tags the callers use.
struct LTVec
{
	float x, y, z;

	LTVec* sub_10015480(float fX, float fY, float fZ);  // 0x10015480
	LTVec  sub_10015640(const LTVec v) const;           // 0x10015640 operator -
};

struct LTMat
{
	float m[4][4];

	// 0x1001e140 -- LTMatrix::Init, sixteen floats by value, out of line.
	void sub_1001e140(float m00, float m01, float m02, float m03,
	                  float m10, float m11, float m12, float m13,
	                  float m20, float m21, float m22, float m23,
	                  float m30, float m31, float m32, float m33);

	void sub_100156e0();                                // 0x100156e0
	void sub_10015770(const LTVec* pIn, LTVec* pOut);   // 0x10015770
};

class Vec10027eb0
{
public:
	float x, y, z;
};

class Mat10027eb0
{
public:
	float m[4][4];
};

// A transformed vertex: position then rhw (+0x0c).
struct TLVert14580
{
	float x, y, z;                                      // +0x00
	float rhw;                                          // +0x0c
};

// The transform object's screen-projection block (0x1008b880 in every
// caller): scale and offset at +0x31c .. +0x330.
struct XForm14580
{
	uint8   m_pad_000[0x31c];
	float   m_f31c;                                     // +0x31c  x scale
	float   m_f320;                                     // +0x320  x centre
	float   m_f324;                                     // +0x324  y scale
	float   m_f328;                                     // +0x328  y centre
	float   m_f32c;                                     // +0x32c  z scale
	float   m_f330;                                     // +0x330  z offset
};

// The model vertex the +0x420 hooks read: position at +0x08.
struct MVert159d0
{
	uint32  m_n00;                                      // +0x00
	uint32  m_n04;                                      // +0x04
	float   x, y, z;                                    // +0x08
};

// The hooks' output: position and 1/w.
struct HVert159d0
{
	float   x, y, z;                                    // +0x00
	float   rhw;                                        // +0x0c
};

struct SDesc1dc;

struct SModel
{
	uint8       m_pad_000[0x1dc];
	SDesc1dc*   m_p1dc;                                 // +0x1dc

	SDesc1dc* sub_100158c0() const;                     // 0x100158c0
};


// ======================================================================= //
//  0x100123a0 (the deferred lightmap flush) moved to w30_1000f020_osqipni.cpp:
//  it enters 0x100124a4 at its /Qip register entry, so the two share a TU.
//
//  Its element type stays declared here, unused, BY MEASUREMENT: with the
//  flush and all its declarations gone, icl schedules the x87 code of
//  0x100149a0, 0x100159d0 and 0x10015aa0 differently (12 -> 9 identical
//  in this file).  Bisected: the flush's body, its externs, D3DDev7 and
//  Bucket123a0 can go; this one struct definition cannot (a forward
//  declaration alone is not enough).  The same TU-context sensitivity as
//  the address-order note at the top of the file.
// ======================================================================= //

struct Obj124a4;

struct Elem0f6f0                                        // 0x18 bytes
{
	Obj124a4*   m_pOwner;                               // +0x00  (free link)
	uint32      m_n04;                                  // +0x04
	uint32      m_n08;                                  // +0x08
	uint32      m_n0c;                                  // +0x0c
	uint32      m_nClip10;                              // +0x10
	Elem0f6f0*  m_pNext14;                              // +0x14
};


// ======================================================================= //
//  0x10014580 -- the screen projection (103 bytes).  __cdecl
//  (vertex, transform object).
// ======================================================================= //
extern "C" void sub_10014580(TLVert14580* pVert, XForm14580* pXForm)
{
	float rhw = 1.0f / pVert->z;

	pVert->rhw = rhw;
	pVert->x = (pVert->x * pXForm->m_f31c + pVert->z * pXForm->m_f320) * rhw;
	pVert->y = (pVert->y * pXForm->m_f324 + pVert->z * pXForm->m_f328) * pVert->rhw;
	pVert->z = (pVert->z * pXForm->m_f32c + pXForm->m_f330) * pVert->rhw;
}


// ======================================================================= //
//  0x10014940 -- ltmatrix.h's MatVMul (92 bytes), out of line.
// ======================================================================= //
extern "C" void sub_10014940(LTVec* pDest, LTMat* pMat, LTVec* pSrc)
{
	pDest->x = pMat->m[0][0]*pSrc->x + pMat->m[0][1]*pSrc->y + pMat->m[0][2]*pSrc->z + pMat->m[0][3];
	pDest->y = pMat->m[1][0]*pSrc->x + pMat->m[1][1]*pSrc->y + pMat->m[1][2]*pSrc->z + pMat->m[1][3];
	pDest->z = pMat->m[2][0]*pSrc->x + pMat->m[2][1]*pSrc->y + pMat->m[2][2]*pSrc->z + pMat->m[2][3];
}


// ======================================================================= //
//  0x100149a0 -- ltmatrix.h's MatVMul_InPlace_H (164 bytes): the temp
//  vector is copied back with `rep movsl`, and 1/w is returned.
// ======================================================================= //
float sub_100149a0(Mat10027eb0* pMat, Vec10027eb0* pSrc)
{
	float one_over_w = 1.0f / (pMat->m[3][0]*pSrc->x + pMat->m[3][1]*pSrc->y + pMat->m[3][2]*pSrc->z + pMat->m[3][3]);
	Vec10027eb0 temp;

	temp.x = one_over_w * (pMat->m[0][0]*pSrc->x + pMat->m[0][1]*pSrc->y + pMat->m[0][2]*pSrc->z + pMat->m[0][3]);
	temp.y = one_over_w * (pMat->m[1][0]*pSrc->x + pMat->m[1][1]*pSrc->y + pMat->m[1][2]*pSrc->z + pMat->m[1][3]);
	temp.z = one_over_w * (pMat->m[2][0]*pSrc->x + pMat->m[2][1]*pSrc->y + pMat->m[2][2]*pSrc->z + pMat->m[2][3]);

	*pSrc = temp;
	return one_over_w;
}


// ======================================================================= //
//  0x10014a50 -- ltmatrix.h's MatVMul_3x3 (83 bytes), out of line.
// ======================================================================= //
void sub_10014a50(Vec10027eb0* pDest, Mat10027eb0* pMat, Vec10027eb0* pSrc)
{
	pDest->x = pMat->m[0][0]*pSrc->x + pMat->m[0][1]*pSrc->y + pMat->m[0][2]*pSrc->z;
	pDest->y = pMat->m[1][0]*pSrc->x + pMat->m[1][1]*pSrc->y + pMat->m[1][2]*pSrc->z;
	pDest->z = pMat->m[2][0]*pSrc->x + pMat->m[2][1]*pSrc->y + pMat->m[2][2]*pSrc->z;
}


// ======================================================================= //
//  0x10014ab0 -- ltmatrix.h's Mat_InverseTransformation(pSrc, pDest)
//  (167 bytes).  MatVMul_3x3 is CALLED (0x10014b20 -> 0x10014a50).
// ======================================================================= //
extern "C" void sub_10014ab0(Mat10027eb0* pSrc, Mat10027eb0* pDest)
{
	Vec10027eb0 trans, newTrans;

	trans.x = pSrc->m[0][3];
	trans.y = pSrc->m[1][3];
	trans.z = pSrc->m[2][3];

	// Transpose the upper 3x3.
	pDest->m[0][0] = pSrc->m[0][0];  pDest->m[0][1] = pSrc->m[1][0]; pDest->m[0][2] = pSrc->m[2][0];
	pDest->m[1][0] = pSrc->m[0][1];  pDest->m[1][1] = pSrc->m[1][1]; pDest->m[1][2] = pSrc->m[2][1];
	pDest->m[2][0] = pSrc->m[0][2];  pDest->m[2][1] = pSrc->m[1][2]; pDest->m[2][2] = pSrc->m[2][2];

	sub_10014a50(&newTrans, pDest, &trans);
	pDest->m[0][3] = -newTrans.x;
	pDest->m[1][3] = -newTrans.y;
	pDest->m[2][3] = -newTrans.z;

	// Fill in the bottom row.
	pDest->m[3][0] = pDest->m[3][1] = pDest->m[3][2] = 0.0f;
	pDest->m[3][3] = 1.0f;
}


// ======================================================================= //
//  0x10014b60 -- trilinear sample of the 8-bit light grid (1,572 bytes),
//  out of line: big19d64.cpp's 0x1001ab1a block is the same computation
//  inlined into CD3DModelLight.  The first argument is the grid block at
//  world+0xfc (cells +0x00, strides +0x08/+0x20, clamps +0x14..+0x1c,
//  scale +0x30, origin +0x3c); the answer goes out as three bytes, the
//  +2 channel first.  The three `call 0x1007fd30` are floor(double) and
//  the fnstcw/fistpll sandwiches are plain (int) casts.
//
//  Rewritten after server.dll's twin, w_GetPointShade
//  (src/engine/server/shade.cpp, NEAR there): the eight corners as ONE
//  array, the blends through vLeft/vRight/vNear/vFar reused, the weights a
//  named vector vInv, `weight * sample` products and `v = a - b` as an
//  assignment (the shipped copy -0xe0 -> -0xd4).  The opening sequence is
//  the shipped one (frame aside); 257 of 472 instructions match (was 162).  LEFT: the shipped
//  build keeps every blend component in its float home (`fstps -0x58 ;
//  flds -0x58`), ours scalarises them into the x87 stack and spills
//  doubles -- frame 0x120 against 0xf0; LTVector locals, a blend array,
//  /Op, /Qpc32 and /O2 measured, none closer.
// ======================================================================= //

extern "C" double floor(double);

struct GridCell14b60
{
	uint8   m_b0;
	uint8   m_b1;
	uint8   m_b2;
	uint8   m_b3;
};

struct Grid14b60
{
	GridCell14b60*  m_pCells00;                         // +0x00
	uint32          m_n04;                              // +0x04
	uint32          m_nStrideY08;                       // +0x08
	uint32          m_n0c;                              // +0x0c
	uint32          m_n10;                              // +0x10
	int32           m_nMaxX14;                          // +0x14
	int32           m_nMaxY18;                          // +0x18
	int32           m_nMaxZ1c;                          // +0x1c
	uint32          m_nStrideZ20;                       // +0x20
	uint8           m_pad_24[0x30 - 0x24];              // +0x24
	float           m_fScale30;                         // +0x30
	float           m_fScale34;                         // +0x34
	float           m_fScale38;                         // +0x38
	LTVec           m_vMin3c;                           // +0x3c
};

void sub_10014b60(void* pGridBlock, LTVec* pPos, uint8* pOut)
{
	Grid14b60*      pGrid = (Grid14b60*)pGridBlock;
	LTVec           vSample[8];
	LTVec           vNear, vFar;
	LTVec           vLeft, vRight;
	LTVec           vInv;
	LTVec           v;
	GridCell14b60*  pCell;
	int32           ix, iy, iz;

	v = pPos->sub_10015640(pGrid->m_vMin3c);
	v.x *= pGrid->m_fScale30;
	v.y *= pGrid->m_fScale34;
	v.z *= pGrid->m_fScale38;

	ix = (int32)v.x;
	iy = (int32)v.y;
	iz = (int32)v.z;

	if (ix < 0)
		ix = 0;
	else if (ix > pGrid->m_nMaxX14)
		ix = pGrid->m_nMaxX14;

	if (iy < 0)
		iy = 0;
	else if (iy > pGrid->m_nMaxY18)
		iy = pGrid->m_nMaxY18;

	if (iz < 0)
		iz = 0;
	else if (iz > pGrid->m_nMaxZ1c)
		iz = pGrid->m_nMaxZ1c;

	v.x -= (float)floor(v.x);
	v.y -= (float)floor(v.y);
	v.z -= (float)floor(v.z);

	vInv.x = 1.0f - v.x;
	vInv.y = 1.0f - v.y;
	vInv.z = 1.0f - v.z;

	pCell = pGrid->m_pCells00 + (ix + pGrid->m_nStrideY08 * iy + pGrid->m_nStrideZ20 * iz);

	vSample[0].x = pCell[pGrid->m_nStrideY08].m_b2;
	vSample[0].y = pCell[pGrid->m_nStrideY08].m_b1;
	vSample[0].z = pCell[pGrid->m_nStrideY08].m_b0;
	vSample[1].x = pCell[pGrid->m_nStrideY08 + 1].m_b2;
	vSample[1].y = pCell[pGrid->m_nStrideY08 + 1].m_b1;
	vSample[1].z = pCell[pGrid->m_nStrideY08 + 1].m_b0;
	vSample[2].x = pCell[0].m_b2;
	vSample[2].y = pCell[0].m_b1;
	vSample[2].z = pCell[0].m_b0;
	vSample[3].x = pCell[1].m_b2;
	vSample[3].y = pCell[1].m_b1;
	vSample[3].z = pCell[1].m_b0;

	pCell += pGrid->m_nStrideZ20;

	vSample[4].x = pCell[pGrid->m_nStrideY08].m_b2;
	vSample[4].y = pCell[pGrid->m_nStrideY08].m_b1;
	vSample[4].z = pCell[pGrid->m_nStrideY08].m_b0;
	vSample[5].x = pCell[pGrid->m_nStrideY08 + 1].m_b2;
	vSample[5].y = pCell[pGrid->m_nStrideY08 + 1].m_b1;
	vSample[5].z = pCell[pGrid->m_nStrideY08 + 1].m_b0;
	vSample[6].x = pCell[0].m_b2;
	vSample[6].y = pCell[0].m_b1;
	vSample[6].z = pCell[0].m_b0;
	vSample[7].x = pCell[1].m_b2;
	vSample[7].y = pCell[1].m_b1;
	vSample[7].z = pCell[1].m_b0;

	vLeft.x  = vInv.y * vSample[2].x;
	vLeft.y  = vInv.y * vSample[2].y;
	vLeft.z  = vInv.y * vSample[2].z;
	vLeft.x += v.y * vSample[0].x;
	vLeft.y += v.y * vSample[0].y;
	vLeft.z += v.y * vSample[0].z;

	vRight.x  = vInv.y * vSample[3].x;
	vRight.y  = vInv.y * vSample[3].y;
	vRight.z  = vInv.y * vSample[3].z;
	vRight.x += v.y * vSample[1].x;
	vRight.y += v.y * vSample[1].y;
	vRight.z += v.y * vSample[1].z;

	vNear.x  = vInv.x * vLeft.x;
	vNear.y  = vInv.x * vLeft.y;
	vNear.z  = vInv.x * vLeft.z;
	vNear.x += v.x * vRight.x;
	vNear.y += v.x * vRight.y;
	vNear.z += v.x * vRight.z;

	vLeft.x  = vInv.y * vSample[6].x;
	vLeft.y  = vInv.y * vSample[6].y;
	vLeft.z  = vInv.y * vSample[6].z;
	vLeft.x += v.y * vSample[4].x;
	vLeft.y += v.y * vSample[4].y;
	vLeft.z += v.y * vSample[4].z;

	vRight.x  = vInv.y * vSample[7].x;
	vRight.y  = vInv.y * vSample[7].y;
	vRight.z  = vInv.y * vSample[7].z;
	vRight.x += v.y * vSample[5].x;
	vRight.y += v.y * vSample[5].y;
	vRight.z += v.y * vSample[5].z;

	vFar.x  = vInv.x * vLeft.x;
	vFar.y  = vInv.x * vLeft.y;
	vFar.z  = vInv.x * vLeft.z;
	vFar.x += v.x * vRight.x;
	vFar.y += v.x * vRight.y;
	vFar.z += v.x * vRight.z;

	vRight.x  = vInv.z * vNear.x;
	vRight.y  = vInv.z * vNear.y;
	vRight.z  = vInv.z * vNear.z;
	vRight.x += v.z * vFar.x;
	vRight.y += v.z * vFar.y;
	vRight.z += v.z * vFar.z;

	pOut[2] = (uint8)vRight.x;
	pOut[1] = (uint8)vRight.y;
	pOut[0] = (uint8)vRight.z;
}


// ======================================================================= //
//  0x10015480 -- vector Init(x, y, z), returns this (36 bytes, `ret $0xc`).
// ======================================================================= //
LTVec* LTVec::sub_10015480(float fX, float fY, float fZ)
{
	x = fX;
	y = fY;
	z = fZ;
	return this;
}


// ======================================================================= //
//  0x100156e0 -- ltmatrix.h's LTMatrix::Identity (92 bytes): the sixteen
//  values go to Init (0x1001e140) by value.
// ======================================================================= //
void LTMat::sub_100156e0()
{
	sub_1001e140(
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f);
}


// ======================================================================= //
//  0x10015770 -- ltmatrix.h's LTMatrix::Apply(in, out) (104 bytes).
//
//  BYTE-IDENTICAL.  The lever is the ADDRESS FORM, not the term order:
//  icl canonicalises every term order and commutation of a row to the
//  same bytes (48 per form, measured), but which operand of each product
//  it loads first follows how the operand is spelled.  With z read as
//  v[2] off a float pointer to the vector (x and y as members, the matrix
//  as m[r][c]) the z term loads s->z first and the x/y terms the matrix
//  first, as shipped.  Swept: s->c / v[c] per component x m[r][c] / f[k]
//  per column (30 spellings); only z-through-the-pointer matches
//  (v[1] as well is also identical).
// ======================================================================= //
void LTMat::sub_10015770(const LTVec* s, LTVec* d)
{
	const float* v = &s->x;

	d->x = m[0][0]*s->x + m[0][1]*s->y + v[2]*m[0][2] + m[0][3];
	d->y = m[1][0]*s->x + m[1][1]*s->y + v[2]*m[1][2] + m[1][3];
	d->z = m[2][0]*s->x + m[2][1]*s->y + v[2]*m[2][2] + m[2][3];
}


// ======================================================================= //
//  0x100158c0 -- the +0x1dc getter (11 bytes).
// ======================================================================= //
SDesc1dc* SModel::sub_100158c0() const
{
	return m_p1dc;
}


// ======================================================================= //
//  THE +0x420 VERTEX HOOKS.  big17c14.cpp stores 0x100159d0 (plain) or
//  0x10015aa0 (warbled) into CD3DModelDraw+0x420, and 0x10015a60 or
//  0x10069c90 into +0x41c.  The hooks take the vertex in %ecx, the output
//  in %edx and the matrix on the stack, `ret $0x4`: __fastcall.
// ======================================================================= //

extern float    g_fWarbleTime;                          // 0x1008b750
extern float    g_fWarbleFrac;                          // 0x1008b74c
extern float    g_1008b754[32];                         // 0x1008b754  slopes
extern float    g_1008b7d4[32];                         // 0x1008b7d4  values
extern uint32   g_1008b748;                             // 0x1008b748  NEW: the warble step

// 0x100159d0 -- homogeneous transform of the vertex position (204 bytes).
extern "C" void __fastcall sub_100159d0(MVert159d0* pVert, HVert159d0* pOut,
                                        LTMat* pMat)
{
	float one_over_w = 1.0f / (pMat->m[3][0]*pVert->x + pMat->m[3][1]*pVert->y + pMat->m[3][2]*pVert->z + pMat->m[3][3]);

	pOut->x = (pMat->m[0][0]*pVert->x + pMat->m[0][1]*pVert->y + pMat->m[0][2]*pVert->z + pMat->m[0][3]) * one_over_w;
	pOut->y = (pMat->m[1][0]*pVert->x + pMat->m[1][1]*pVert->y + pMat->m[1][2]*pVert->z + pMat->m[1][3]) * one_over_w;
	pOut->z = (pMat->m[2][0]*pVert->x + pMat->m[2][1]*pVert->y + pMat->m[2][2]*pVert->z + pMat->m[2][3]) * one_over_w;
	pOut->rhw = one_over_w;
}

// 0x10015a60 -- restart the warble step from the warble time (60 bytes;
// the enumerator's size-0 row, but a real body: big17c14.cpp's
// 0x1001916b stores its address).  The 64-bit `fistpll` is icl's
// float-to-UNSIGNED conversion.
extern "C" void sub_10015a60()
{
	g_1008b748 = (uint32)g_fWarbleTime;
}



// Identical ONLY with a later function in the TU: alone, or as the last
// function of this file, the same source differs by one reassociated
// `fadds` in the x row (m[0][3] added before the m[0][0] term); with
// 0x10015320 after it, it matches.  Do not reorder.
// 0x10015aa0 -- 0x100159d0 with the position first scaled by the warble
// table, one step per vertex (204 bytes).
extern "C" void __fastcall sub_10015aa0(MVert159d0* pVert, HVert159d0* pOut,
                                        LTMat* pMat)
{
	uint32 i = g_1008b748 & 31;
	float  s = g_1008b754[i] * g_fWarbleFrac + g_1008b7d4[i];
	LTVec  v;
	float  one_over_w;

	g_1008b748++;

	v.x = pVert->x * s;
	v.y = pVert->y * s;
	v.z = pVert->z * s;

	one_over_w = 1.0f / (pMat->m[3][0]*v.x + pMat->m[3][1]*v.y + pMat->m[3][2]*v.z + pMat->m[3][3]);

	pOut->x = (pMat->m[0][0]*v.x + pMat->m[0][1]*v.y + pMat->m[0][2]*v.z + pMat->m[0][3]) * one_over_w;
	pOut->y = (pMat->m[1][0]*v.x + pMat->m[1][1]*v.y + pMat->m[1][2]*v.z + pMat->m[1][3]) * one_over_w;
	pOut->z = (pMat->m[2][0]*v.x + pMat->m[2][1]*v.y + pMat->m[2][2]*v.z + pMat->m[2][3]) * one_over_w;
	pOut->rhw = one_over_w;
}


// ======================================================================= //
//  0x10015320 -- bind a shared texture's record for one stage and set its
//  LOD (329 bytes).  __cdecl (block, stage, lod), answers 1 when bound
//  and 0 on failure.  The inline copies of this at 0x1000b439 (big0ae20)
//  and in big2ef34 / big61a30 / big6d910 are the same walk with a LOD of
//  0; this out-of-line one binds through 0x1000a7d0 instead of inlining
//  the bind.
// ======================================================================= //

struct DDSurf15320;

struct DDSurf15320Vtbl
{
	uint8   m_pad_00[0xbc];
	long (__stdcall* SetLOD)(DDSurf15320* pThis, uint32 dwLOD);   // +0xbc
};

struct DDSurf15320
{
	DDSurf15320Vtbl*    lpVtbl;
};

// The per-stage texture record.  The tag is d3ddata.cpp's for
// 0x1008e684's element type.
struct Tex10059590
{
	uint8           m_pad_00[0x0c];
	DDSurf15320*    m_pSurface0c;                       // +0x0c
	uint8           m_pad_10[0x30 - 0x10];
	Tex10059590*    m_pNext30;                          // +0x30
	uint8           m_pad_34[0x42 - 0x34];
	uint16          m_nStage42;                         // +0x42
	uint16          m_nLOD44;                           // +0x44
};

struct TexBlock15320
{
	uint8           m_pad_00[0x0c];
	Tex10059590*    m_pHead0c;                          // +0x0c
	uint8           m_pad_10[0x30 - 0x10];
	uint16          m_nStamp30;                         // +0x30
};

extern uint16       g_w1008b870;                        // 0x1008b870  frame stamp
extern Tex10059590* g_1008e684[];                       // 0x1008e684  bound per stage

// 0x1000a7d0 -- __cdecl, one argument: publishes the record in
// 0x1008e684[m_nStage42] and relinks it on the frame's used list.
extern "C" void sub_1000a7d0(Tex10059590* pRec);

// 0x10039bd0 -- d3dmid.cpp's stack entry (block, stage, bLink).
extern "C" Tex10059590* sub_10039bd0(TexBlock15320* pBlock, uint32 nStage,
                                     uint32 bLink);

// The record walk, inlined: the found record comes back in one register
// (%esi) and the bind works on a copy of it (%edi), which is the shape of
// an inline function's return value.
static inline Tex10059590* FindRec15320(Tex10059590* pHead, uint32 nStage)
{
	Tex10059590* pCur;

	for (pCur = pHead; pCur; pCur = pCur->m_pNext30)
	{
		if (pCur->m_nStage42 == (uint8)nStage)
			return pCur;
	}

	return LTNULL;
}

extern "C" int32 sub_10015320(TexBlock15320* pBlock, uint32 nStage, uint32 nLOD)
{
	Tex10059590* pRec;
	Tex10059590* pHead;
	Tex10059590* pNew;

	if (!pBlock)
		return 0;

	pBlock->m_nStamp30 = g_w1008b870;

	pHead = pBlock->m_pHead0c;
	pRec  = LTNULL;

	if (pHead)
		pRec = FindRec15320(pHead, nStage);

	if (!pRec)
		goto L_MakeRec;

	if (pRec == g_1008e684[nStage])
		goto L_ElseLOD;

L_Bind:
	sub_1000a7d0(pRec);

	if (pRec->m_nLOD44 != nLOD)
	{
		pRec->m_pSurface0c->lpVtbl->SetLOD(pRec->m_pSurface0c, nLOD);
		pRec->m_nLOD44 = (uint16)nLOD;
	}

	return 1;

L_ElseLOD:
	// Already bound to this stage: only the LOD update survives.
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
