// *********************************************************************** //
//
//  w17_10020e80_osoi.cpp -- 0x10020e80, the lens from a field of view.
//  /Os /Oi- (see build/engine.sh): 0x90 padding round it (the /Os pad)
//  and cos/sin CALLED (0x1007fe30 is libcmt's `fldl ; fcos`, 0x1007fdd0
//  its `fldl ; fsin`), which is /Oi-.  The double arguments of those calls
//  are what give it icl's 14-byte stack-aligning entry.
//
//  w4_1001e4a0_os.cpp's 0x10020ce0 builds its lens with it: near and far
//  as given, a unit focal length, no offset, and the half extents
//  cot(pi/2 - fov/2) = tan(fov/2) for each axis.
// *********************************************************************** //

#include "ltbasedefs.h"
#include <math.h>

// w4_1001e4a0_os.cpp's lens block.
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

void sub_10020e80(D3DLens* pLens, float fNear, float fFar, float fFovX,
                  float fFovY)
{
	float	fAngleX, fAngleY;

	fAngleX = MATH_HALFPI - fFovX * 0.5f;
	fAngleY = MATH_HALFPI - fFovY * 0.5f;

	pLens->m_fNear = fNear;
	pLens->m_fFar = fFar;

	pLens->m_fHalfW = (float)(cos(fAngleX) / sin(fAngleX));
	pLens->m_fHalfH = (float)(cos(fAngleY) / sin(fAngleY));

	pLens->m_fFocal = 1.0f;
	pLens->m_fOffX = 0.0f;
	pLens->m_fOffY = 0.0f;
}
