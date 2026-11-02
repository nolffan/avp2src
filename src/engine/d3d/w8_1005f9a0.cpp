// *********************************************************************** //
//
//  w8_1005f9a0.cpp -- 0x1005f9a0, project one sprite vertex.
//
//  __cdecl (dest, src, fBias), frameless, plain `ret`.  Callers: big60010.cpp
//  (0x1006185e, dest == src, a per-vertex bias) and big61a30.cpp
//  (0x10064234 / 0x1006428e, dest == src, a bias or 7.0f).
//
//  WHAT IT DOES, read off the x87 stack: the vertex goes through the global
//  view-projection 4x4 at 0x1008ba1c with the homogeneous divide -- but
//  twice over for the depth.  x and y are divided by the w of the vertex
//  AS GIVEN; z is first pushed along by fBias, and z and rhw use the w of
//  the biased point:
//      oow  = 1 / (m[3] . (x, y, z)  + m[3][3])
//      x'   = (m[0] . (x, y, z)  + m[0][3]) * oow
//      y'   = (m[1] . (x, y, z)  + m[1][3]) * oow
//      zb   = z + fBias
//      oowb = 1 / (m[3] . (x, y, zb) + m[3][3])
//      z'   = (m[2] . (x, y, zb) + m[2][3]) * oowb,   rhw' = oowb
//  -- a sprite drawn at its own screen position with its depth pulled
//  towards the camera.  Two divisions, and the pool holds the two dead
//  1.0f slots that go with them (0x100aabfc, 0x100aac04).  All
//  loads come before the first store, so dest may alias src.
// *********************************************************************** //

#include "ltbasedefs.h"

struct LTMat
{
	float	m[4][4];
};

extern LTMat g_Mat1008ba1c;                             // 0x1008ba1c

struct Vert1005f9a0
{
	float	x;                                          // +0x00
	float	y;                                          // +0x04
	float	z;                                          // +0x08
	float	rhw;                                        // +0x0c
};


// ======================================================================= //
//  0x1005f9a0  224 bytes
// ======================================================================= //
extern "C" void sub_1005f9a0(Vert1005f9a0* pDest, Vert1005f9a0* pSrc,
                             float fBias)
{
	float	oow, oowBias, fX, fY, fZ;
	LTMat*	pMat = &g_Mat1008ba1c;

	oow = 1.0f / (pMat->m[3][0] * pSrc->x + pMat->m[3][1] * pSrc->y +
	              pMat->m[3][2] * pSrc->z + pMat->m[3][3]);
	fX = (pMat->m[0][0] * pSrc->x + pMat->m[0][1] * pSrc->y +
	      pMat->m[0][2] * pSrc->z + pMat->m[0][3]) * oow;
	fY = (pMat->m[1][0] * pSrc->x + pMat->m[1][1] * pSrc->y +
	      pMat->m[1][2] * pSrc->z + pMat->m[1][3]) * oow;

	fZ = pSrc->z + fBias;
	oowBias = 1.0f / (pMat->m[3][0] * pSrc->x + pMat->m[3][1] * pSrc->y +
	                  pMat->m[3][2] * fZ + pMat->m[3][3]);
	fZ = (pMat->m[2][0] * pSrc->x + pMat->m[2][1] * pSrc->y +
	      pMat->m[2][2] * fZ + pMat->m[2][3]) * oowBias;

	pDest->x = fX;
	pDest->y = fY;
	pDest->z = fZ;
	pDest->rhw = oowBias;
}
