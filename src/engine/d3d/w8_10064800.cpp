// *********************************************************************** //
//
//  w8_10064800.cpp -- 0x10064800, the dynamic-light vertex pass for a
//  40-byte vertex array.  /O2: the double -> int casts are the plain /O2
//  twelve-instruction `shld` truncation, sqrt is the inline
//  `fsqrt`, the loop heads are nop-aligned, the `< 0` clamp arms sunk.
//
//  __cdecl, three stack dwords, plain `ret`; big70d10.cpp's 0x10070d10
//  calls it as `sub_10064800(pObj, pVerts, nVerts)`.  The third argument
//  is never read: the count is the object's own +0x50 word, exactly as
//  big3e620.cpp and big64cc0.cpp observe of their copies.
//
//  WHAT IT DOES -- big3e620.cpp's light loop (0x1004029c..0x10040721),
//  statement for statement.  For each light on the object's list (+0x2c):
//  bump the light counter 0x1008be48; the light's distance from the
//  object's plane (+0x28 -> +0x34), made positive; the colour delta
//  c - (255 - c) per channel; skip the light when the distance is not
//  inside its radius (+0x1a8 of the light object); then for every vertex
//  within the radius add delta * (1 - dist / radius) to the diffuse bytes
//  (+0x12 red, +0x11 green, +0x10 blue), doubled when Saturate
//  (0x1008cb98) is on, clamped to 0..255.
//
//  The radius test as an `if` block around the vertex loop
//  (the shipped code sinks the miss to the loop bottom), the loop as
//  `for (i = 0; i < nLeft; i++)` over `pWalk = &pVerts[i]`, and the light
//  position copied before the counter bump: 879 bytes = shipped, 546 of
//  843 bytes positionally equal (was 104) -- pairs as near.  LEFT: frame
//  0xb4 against 0xbc and the down-counter the shipped loop keeps.
//
//  Then: the plane distance from the fLightX/Y/Z copies (the shipped
//  code loads each position float TWICE at the top, one copy spilled and
//  one fed to the dot product before the counter store -- read back from
//  the light after the store, ours reloaded it), and the vertex loop as a
//  walking pointer under an unsigned `while (nLeft > 0)` down-counter
//  (`test ; jbe` in front, `dec ; jne` at the bottom; `while (nLeft)`
//  gives `je`): 879 bytes, 6 instructions differ.  NEAR: the frame is
//  0xb4 against 0xbc (every slot the same offset; the shipped frame just
//  has 12 unused bytes at the top to our 4), and the loop guard's `test`
//  sits one store later.  Unchanged by: the five radius/count statements
//  in seven orders, fAtten / fDistSqr / fDist / fRadius / the light
//  colours or position as doubles, `sqrt` without the (float), the light
//  position as a struct local (copied whole or by member), a dead local,
//  float arrays for the colours or the position (worse).
//
//  (Earlier:) NEAR: 876 bytes against 879, the same blocks and calls.  The shipped
//  code loads the three colour bytes and the six light-position floats at
//  the top of the light loop and spills the position as doubles before the
//  plane dot product; ours schedules them later (tried: the colour
//  statements first, last, and the loop reading pRef->m_vPos directly --
//  892 bytes, worse).
// *********************************************************************** //

#include "ltbasedefs.h"
#include <math.h>

struct Vec10064800
{
	float	x;                                          // +0x00
	float	y;                                          // +0x04
	float	z;                                          // +0x08
};

struct Plane10064800
{
	Vec10064800		m_vNormal;                          // +0x00
	float			m_fDist;                            // +0x0c
};

struct Surf10064800
{
	uint8			m_pad_00[0x34];
	Plane10064800*	m_pPlane34;                         // +0x34
};

struct LightObj10064800
{
	uint8			m_pad_00[0x94];
	uint8			m_nR94;                             // +0x94
	uint8			m_nG95;                             // +0x95
	uint8			m_nB96;                             // +0x96
	uint8			m_pad_97[0x1a8 - 0x97];
	float			m_fRadius1a8;                       // +0x1a8
};

struct LightRef10064800
{
	LightRef10064800*	m_pNext;                        // +0x00
	LightObj10064800*	m_pObject;                      // +0x04
	Vec10064800			m_vPos;                         // +0x08
};

struct Obj10064800
{
	uint8				m_pad_00[0x28];
	Surf10064800*		m_p28;                          // +0x28
	LightRef10064800*	m_pLights2c;                    // +0x2c
	uint8				m_pad_30[0x50 - 0x30];
	uint16				m_nCount50;                     // +0x50
};

// The 40-byte vertex: position, then the diffuse BGRA at +0x10.
struct Vert10064800
{
	Vec10064800		m_vPos;                             // +0x00
	float			m_fRHW0c;                           // +0x0c
	uint8			m_Diffuse10[4];                     // +0x10
	uint8			m_pad_14[0x28 - 0x14];
};

extern uint32	g_1008be48;                             // 0x1008be48 lights applied
extern uint32	g_1008cb98;                             // 0x1008cb98 Saturate


// ======================================================================= //
//  0x10064800  879 bytes
// ======================================================================= //
extern "C" void sub_10064800(Obj10064800* pObj, Vert10064800* pVerts,
                             uint32 nVerts)
{
	LightRef10064800*	pRef;
	LightObj10064800*	pLightObj;
	Plane10064800*		pPlane;
	Vert10064800*		pWalk;
	float				fDist;
	float				fLightR, fLightG, fLightB;
	float				fRadius, fInvRadius, fRadiusSqr;
	float				fDistSqr, fAtten;
	float				fLightX, fLightY, fLightZ;
	uint32				nLeft, i;
	int32				nR, nG, nB;

	for (pRef = pObj->m_pLights2c; pRef; pRef = pRef->m_pNext)
	{
		pLightObj = pRef->m_pObject;

		fLightR = pLightObj->m_nR94 - (255.0f - pLightObj->m_nR94);
		fLightG = pLightObj->m_nG95 - (255.0f - pLightObj->m_nG95);
		fLightB = pLightObj->m_nB96 - (255.0f - pLightObj->m_nB96);

		fLightX = pRef->m_vPos.x;
		fLightY = pRef->m_vPos.y;
		fLightZ = pRef->m_vPos.z;

		g_1008be48++;

		pPlane = pObj->m_p28->m_pPlane34;
		fDist  = pPlane->m_vNormal.x * fLightX
		       + pPlane->m_vNormal.y * fLightY
		       + pPlane->m_vNormal.z * fLightZ
		       - pPlane->m_fDist;

		if (fDist < 0.0f)
			fDist = -fDist;

		if (fDist < pLightObj->m_fRadius1a8)
		{
			fRadius = pLightObj->m_fRadius1a8;
			fInvRadius = 1.0f / fRadius;
			fRadiusSqr = fRadius * fRadius;

			nLeft = pObj->m_nCount50;
			pWalk = pVerts;
			while (nLeft > 0)
			{

				fDistSqr =
				    (pWalk->m_vPos.x - fLightX) * (pWalk->m_vPos.x - fLightX)
				  + (pWalk->m_vPos.y - fLightY) * (pWalk->m_vPos.y - fLightY)
				  + (pWalk->m_vPos.z - fLightZ) * (pWalk->m_vPos.z - fLightZ);

				if (fRadiusSqr > fDistSqr)
				{
					fAtten = 1.0f - (float)sqrt(fDistSqr) * fInvRadius;

					nR = pWalk->m_Diffuse10[2] + (int32)(fLightR * fAtten);
					nG = pWalk->m_Diffuse10[1] + (int32)(fLightG * fAtten);
					nB = pWalk->m_Diffuse10[0] + (int32)(fLightB * fAtten);

					if (g_1008cb98)
					{
						nR = nR + nR;
						nG = nG + nG;
						nB = nB + nB;
					}

					if (nR > 255)     nR = 255;
					else if (nR < 0)  nR = 0;

					if (nG > 255)     nG = 255;
					else if (nG < 0)  nG = 0;

					if (nB > 255)     nB = 255;
					else if (nB < 0)  nB = 0;

					pWalk->m_Diffuse10[2] = (uint8)nR;
					pWalk->m_Diffuse10[1] = (uint8)nG;
					pWalk->m_Diffuse10[0] = (uint8)nB;
				}

				pWalk++;
				nLeft--;
			}
		}
	}
}
