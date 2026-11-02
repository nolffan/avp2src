// *********************************************************************** //
//
//  w24_10059980_osoi.cpp -- written for the d3d.ren link.
//
//  0x10059980: slot +0x4 of the three-entry table at 0x1008977c
//  (0x10059c70, THIS, 0x10059c70 -- d3dr4c.cpp's one-argument __thiscall
//  no-op either side).  747 bytes: icl's 14-byte stack-aligning entry
//  (0x10059980..0x1005998d) falling into the body at 0x1005998e.  The
//  enumerator's 14-byte row is that entry alone, and it scored
//  "identical" only because big2b1b0.obj's 0x1002b1b0 opens with the same
//  generic 14 bytes (the entry has no address-specific byte; its `jmp`
//  is always `eb 03`).
//
//  Flag set /Os /Oi-: sqrt is CALLED (0x1007fc98, the CRT's double sqrt),
//  the float-to-int casts are the full `fnstcw / or $0xc00 / fldcw /
//  fistpll / fldcw` sequence and the light position is a `rep movsl`
//  struct copy -- all /Os, as w5_1002ec60_osoi.cpp measured on the same
//  body -- and the padding after it is 0x90s.
//
// *********************************************************************** //

#include "ltbasedefs.h"
#include <math.h>


// ======================================================================= //
//  0x10059980 -- per-vertex dynamic lighting of a 40-byte vertex run.
//
//  w5_1002ec60_osoi.cpp's 0x1002ec60 and big3e620.cpp's inlined light pass
//  (0x1004026d) landmark for landmark: the light list at pObj->+0x2c, the
//  plane pObj->+0x28->+0x34 and its |distance| against the light's radius,
//  `c - (255.0f - c)` per colour byte, `1 - sqrt(d2)/r` per vertex added
//  to the vertex's own diffuse bytes, doubled under Saturate and clamped.
//  Here the owner is the argument's +0x00 and the vertices are an array
//  INSIDE the argument, starting at its +0x18, 0x28 bytes apart (`add
//  $0x28,%ebp`); the count is the owner's +0x50 word, as there.
//
//  __thiscall, `ret $0x4`: one stack argument (read at 0x8(%ebx) behind
//  the aligning entry), `this` never read -- the table's other two slots
//  are d3dr4c.cpp's one-argument __thiscall 0x10059c70.
//
//  w5_1002ec60_osoi.cpp's spelling (FLOAT variables; the `fstpl` double
//  slots are icl's x87 spills) gives the aligning entry exactly as
//  shipped -- big3e620.cpp's DOUBLE-local spelling loses it -- and the
//  radius test written POSITIVELY with the vertex loop inside it and a
//  `> 0` unsigned count (`test %esi,%esi ; ja`) puts the next-light block
//  after the loop as shipped (the `continue`/`goto` spelling hosts it at
//  the radius test).
//
//  NEAR: the entry is identical and the body (0x1005998e, 733 bytes) is
//  size-exact with 7 instructions differing -- one scheduling window
//  before the vertex loop: the shipped code loads the radius and squares
//  it (`fldl 0x88(%esp) ; fmul %st(0),%st`) BEFORE reloading pObj, the
//  vertex pointer and the count; ours reloads those three first.
//  Unmoved by: statement order of the radius terms / the pointer and
//  count (four placements), for/while/do forms, double for any subset of
//  the radius variables, dropping the fRadius local.  Also:
//  `while (nLeft)` (27), pVert from pPoly->m_Verts after the count (629),
//  fRadiusSq before fInvRadius (26, unchanged).
// ======================================================================= //

extern int32 g_nLightTests;                             // 0x1008be48
extern int32 g_CV_Saturate;                             // 0x1008cb98

struct Vec59980
{
	float   x;                                          // +0x00
	float   y;                                          // +0x04
	float   z;                                          // +0x08
};

struct Plane59980
{
	Vec59980    m_vNormal;                              // +0x00
	float       m_fDist;                                // +0x0c
};

struct Surf59980
{
	uint8       m_pad_00[0x34];
	Plane59980* m_pPlane34;                             // +0x34
};

struct LightObj59980
{
	uint8   m_pad_00[0x94];
	uint8   m_nR94;                                     // +0x94
	uint8   m_nG95;                                     // +0x95
	uint8   m_nB96;                                     // +0x96
	uint8   m_pad_97[0x1a8 - 0x97];
	float   m_fRadius1a8;                               // +0x1a8
};

struct LightRef59980
{
	LightRef59980*  m_pNext;                            // +0x00
	LightObj59980*  m_pObject;                          // +0x04
	Vec59980        m_vPos;                             // +0x08
};

struct Obj59980
{
	uint8           m_pad_00[0x28];
	Surf59980*      m_p28;                              // +0x28
	LightRef59980*  m_pLights2c;                        // +0x2c
	uint8           m_pad_30[0x50 - 0x30];
	uint16          m_nCount50;                         // +0x50
};

struct Vert59980
{
	Vec59980    m_vPos;                                 // +0x00
	uint32      m_n0c;                                  // +0x0c
	uint8       m_nB10;                                 // +0x10
	uint8       m_nG11;                                 // +0x11
	uint8       m_nR12;                                 // +0x12
	uint8       m_nA13;                                 // +0x13
	uint8       m_pad_14[0x28 - 0x14];
};

struct Poly59980
{
	Obj59980*   m_pObj;                                 // +0x00
	uint8       m_pad_04[0x18 - 0x04];
	Vert59980   m_Verts[1];                             // +0x18
};

struct C10059980
{
	void sub_10059980(Poly59980* pPoly);
};

void C10059980::sub_10059980(Poly59980* pPoly)
{
	Obj59980*       pObj;
	Vert59980*      pVerts;
	Vert59980*      pVert;
	LightRef59980*  pNode;
	LightObj59980*  pLight;
	Plane59980*     pPlane;
	Vec59980        vPos;
	uint32          nLeft;
	int32           r, g, b;
	float           fR, fG, fB;
	float           fDist, fRadius, fInvRadius, fRadiusSq;
	float           dx, dy, dz, fDistSq, fScale;

	pVerts = pPoly->m_Verts;
	pObj   = pPoly->m_pObj;

	pNode = pObj->m_pLights2c;

	while (pNode)
	{
		pLight = pNode->m_pObject;
		vPos = pNode->m_vPos;

		r = pLight->m_nR94;
		g = pLight->m_nG95;
		b = pLight->m_nB96;

		g_nLightTests++;

		pPlane = pObj->m_p28->m_pPlane34;
		fDist = pPlane->m_vNormal.x * vPos.x + pPlane->m_vNormal.y * vPos.y
		      + pPlane->m_vNormal.z * vPos.z - pPlane->m_fDist;

		fR = r - (255.0f - r);
		fG = g - (255.0f - g);
		fB = b - (255.0f - b);

		if (fDist < 0.0f)
			fDist = -fDist;

		if (fDist < pLight->m_fRadius1a8)
		{
			fRadius = pLight->m_fRadius1a8;
			fInvRadius = 1.0f / fRadius;
			fRadiusSq = fRadius * fRadius;

			pVert = pVerts;
			nLeft = pObj->m_nCount50;

			while (nLeft > 0)
			{
				dx = pVert->m_vPos.x - vPos.x;
				dy = pVert->m_vPos.y - vPos.y;
				dz = pVert->m_vPos.z - vPos.z;
				fDistSq = dx * dx + dy * dy + dz * dz;

				if (fDistSq < fRadiusSq)
				{
					fScale = 1.0f - (float)sqrt(fDistSq) * fInvRadius;

					r = pVert->m_nR12 + (int32)(fR * fScale);
					g = pVert->m_nG11 + (int32)(fG * fScale);
					b = pVert->m_nB10 + (int32)(fB * fScale);

					if (g_CV_Saturate)
					{
						r += r;
						g += g;
						b += b;
					}

					if (r > 255) r = 255; else if (r < 0) r = 0;
					if (g > 255) g = 255; else if (g < 0) g = 0;
					if (b > 255) b = 255; else if (b < 0) b = 0;

					pVert->m_nR12 = (uint8)r;
					pVert->m_nG11 = (uint8)g;
					pVert->m_nB10 = (uint8)b;
				}

				pVert++;
				nLeft--;
			}
		}

L_NextLight:
		pNode = pNode->m_pNext;
	}
}
