// *********************************************************************** //
//
//  MODULE:   W4_1001E4A0_OS (.CPP)  -- /Os
//
//  PURPOSE:  Functions the d3d.ren link referenced by address and nobody
//            had written, 0x1001e4a0..0x10020d80: the camera module's
//            small helpers around big1ef30.cpp's 0x1001ef30.
//
//  Flag set: /Os -- each shipped body is followed by 0x90 padding (after
//  a 2/3-byte `mov %esi,%esi` / `lea 0x0(%esi),%esi` to a 4-byte
//  boundary), the /Os shape.
//
// *********************************************************************** //

#include "ltbasedefs.h"

struct LTVec
{
	LTVec() {}
	LTVec(float mx, float my, float mz) { x = mx; y = my; z = mz; }
	float x, y, z;
};

struct LTMat
{
	float m[4][4];

	void sub_1001e4a0(float x, float y, float z);
};

// big1ef30.cpp's lens block: the first 0x1c bytes of the transform object,
// and the 28-byte block 0x10020d80 / 0x10020e80 fill.
struct D3DLens
{
	float	m_fOffX;                                    // +0x00
	float	m_fOffY;                                    // +0x04
	float	m_fFocal;                                   // +0x08
	float	m_fHalfW;                                   // +0x0c
	float	m_fHalfH;                                   // +0x10
	float	m_fNear;                                    // +0x14
	float	m_fFar;                                     // +0x18
};

// big1ef30.cpp's transform object, only as far as these functions read it.
struct D3DXForm
{
	D3DLens	m_Lens;                                     // +0x000
	int32	m_nLeft;                                    // +0x01c
	int32	m_nTop;                                     // +0x020
	int32	m_nRight;                                   // +0x024
	int32	m_nBottom;                                  // +0x028
	uint8	m_Pad02c[0x19c - 0x2c];                     // +0x02c
	LTMat	m_Mat19c;                                   // +0x19c
	uint8	m_Pad1dc[0x31c - 0x1dc];                    // +0x1dc
	float	m_a31c[6];                                  // +0x31c
};


// ======================================================================= //
//  0x1001e4a0 -- LTMat::SetTranslation(x, y, z): m[0][3], m[1][3], m[2][3].
//  __thiscall, `ret $0xc`.  The stores come out z, y, x.
// ======================================================================= //
void LTMat::sub_1001e4a0(float x, float y, float z)
{
	m[0][3] = x;
	m[1][3] = y;
	m[2][3] = z;
}


// ======================================================================= //
//  0x1001e600 and 0x1001e8d0/0x1001e8d4 now live in big1ef30.cpp, the TU
//  of their only caller 0x1001ef30, which reaches both through /Qip
//  register entries (all three identical there).
// ======================================================================= //


// ======================================================================= //
//  0x1001e920 -- terminate the three struct banks d3dr2b.cpp's 0x1001e8a0
//  initialises (sb_Term, 0x1007fbe0), in the reverse order.
// ======================================================================= //
struct StructBank
{
	uint32 m_StructSize;                                // +0x00
	uint32 m_AlignedStructSize;                         // +0x04
	uint32 m_CacheSize;                                 // +0x08
	uint32 m_nPages;                                    // +0x0c
	uint32 m_nTotalObjects;                             // +0x10
	void*  m_PageHead;                                  // +0x14
	void*  m_FreeListHead;                              // +0x18
};

extern StructBank g_1008bebc;                           // 0x1008bebc
extern StructBank g_1008bed8;                           // 0x1008bed8
extern StructBank g_1008bef4;                           // 0x1008bef4

void sub_1007fbe0(void* pBank);                         // 0x1007fbe0 sb_Term

void sub_1001e920()
{
	sub_1007fbe0(&g_1008bef4);
	sub_1007fbe0(&g_1008bed8);
	sub_1007fbe0(&g_1008bebc);
}


// ======================================================================= //
//  IDENTICAL.  The two reciprocals are applied in their own statements
//  (written as `x * fInvH * (fTop - fBottom)` icl folds them into one
//  `(fTop - fBottom) / H` and never forms 1/H, which the shipped code
//  does: `fild ; fdivrs 1.0f` twice).  The order is Near/Far stores, the
//  lens window, the four pixel offsets (`fMinY - top`), THEN the
//  reciprocals, then the scaling: the offsets formed before the integer
//  differences are what keep `fildl 0x20(%ecx)` / `fildl 0x1c(%ecx)` reading
//  memory (offsets after the reciprocals reuse the integer register through
//  a stack slot and push %esi).  Found with a 300-variant statement-order
//  probe; only this order (with `*=` or `x = x * f`, explicit or implicit
//  conversion) is exact.
// ======================================================================= //
// ======================================================================= //
//  0x10020d80 -- the lens for a sub-rectangle of the viewport: map the
//  pixel rectangle (fMinX,fMinY)-(fMaxX,fMaxY), relative to the view's
//  (m_nLeft,m_nTop)-(m_nRight,m_nBottom), into the view's lens window and
//  answer that window as offset / half-extent, with the given near/far and
//  a unit focal length.
// ======================================================================= //
void sub_10020d80(D3DLens* pOut, float fNear, float fFar, D3DXForm* pXForm,
                  float fMinX, float fMinY, float fMaxX, float fMaxY)
{
	D3DLens* pLens = &pXForm->m_Lens;
	float fInvW, fInvH;
	float fLeft, fRight, fTop, fBottom;
	float fX0, fX1, fY0, fY1;

	pOut->m_fNear = fNear;
	pOut->m_fFar  = fFar;

	fTop    = -pLens->m_fOffY + pLens->m_fHalfH;
	fBottom = -pLens->m_fOffY - pLens->m_fHalfH;
	fRight  = pLens->m_fOffX + pLens->m_fHalfW;
	fLeft   = pLens->m_fOffX - pLens->m_fHalfW;

	fY0 = fMinY - (float)pXForm->m_nTop;
	fY1 = fMaxY - (float)pXForm->m_nTop;
	fX0 = fMinX - (float)pXForm->m_nLeft;
	fX1 = fMaxX - (float)pXForm->m_nLeft;

	fInvW = 1.0f / (float)(pXForm->m_nRight - pXForm->m_nLeft);
	fInvH = 1.0f / (float)(pXForm->m_nBottom - pXForm->m_nTop);

	fY0 *= fInvH;
	fY1 *= fInvH;
	fX0 *= fInvW;
	fX1 *= fInvW;

	fY0 = -(fBottom + fY0 * (fTop - fBottom));
	fY1 = -(fBottom + fY1 * (fTop - fBottom));
	fX0 = fLeft + fX0 * (fRight - fLeft);
	fX1 = fLeft + fX1 * (fRight - fLeft);

	pOut->m_fOffX  = (fX1 + fX0) * 0.5f;
	pOut->m_fOffY  = (fY1 + fY0) * 0.5f;
	pOut->m_fFocal = 1.0f;
	pOut->m_fHalfW = (fX1 - fX0) * 0.5f;
	pOut->m_fHalfH = -(fY1 - fY0) * 0.5f;
}


// ======================================================================= //
//  0x10020ce0 -- set up a transform from a field of view: build the lens
//  with 0x10020e80 (fov and near/far -> cot(fov/2) half extents), then
//  run 0x1001ef30 over it with a unit scale.  Answers TRUE.
// ======================================================================= //
struct Mat10020c00;

void  sub_10020e80(D3DLens* pLens, float fNear, float fFar,
                   float fFovX, float fFovY);           // 0x10020e80
int32 sub_1001ef30(D3DXForm* pXForm, D3DLens* pLens,
                   float fMinX, float fMinY, float fMaxX, float fMaxY,
                   LTMat* pMatCam, LTVec vScale);       // 0x1001ef30

int32 sub_10020ce0(D3DXForm* pXForm, float fFovX, float fFovY, float fNear,
                   float fFar, float fMinX, float fMinY, float fMaxX,
                   float fMaxY, LTMat* pMatCam)
{
	D3DLens lens;

	sub_10020e80(&lens, fNear, fFar, fFovX, fFovY);
	sub_1001ef30(pXForm, &lens, fMinX, fMinY, fMaxX, fMaxY, pMatCam,
	             LTVec(1.0f, 1.0f, 1.0f));
	return 1;
}
