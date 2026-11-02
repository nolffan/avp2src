// *********************************************************************** //
//
//  w8_1005fb70.cpp -- 0x1005fb70, clip a sprite's screen polygon to the
//  edges of the world polygon it sits on.  /O2 (callee saves as frame-slot
//  stores, lea-nop padding, the loop head aligned with `nop ; lea`, four
//  duplicated epilogues).
//
//  __cdecl, five stack dwords, plain `ret`; the only caller is
//  big60010.cpp's sprite draw (0x1006047f), which declares it
//      extern "C" int32 sub_1005fb70(pObj, nIndex, ppVerts, pnVerts, pOut)
//  The first argument is never read.
//
//  WHAT IT DOES.
//    * no light grid / world (0x1008beac): 0.  The polygon is fetched by
//      index through that object (0x10015910, __thiscall, `ret $0x4`);
//      none: 0.  The camera (0x1008bd34, passed BY VALUE) must be in front
//      of its plane by more than 0.01f (d3dbatchb.cpp's plane distance,
//      0x10048150); else 0.
//    * then, for each edge (0x18-byte records from +0x54, count the word
//      at +0x50; the first edge is paired with the last), the edge plane:
//      normal = Cross(poly normal, cur - prev) normalised with no zero test
//      (VEC_NORM: `fsqrt ; fdivrs 1.0`), distance = Dot(normal, cur); the
//      clip counter 0x1008be44 is bumped.  Vertices with Dot - dist > 0 are
//      inside (a 0/1 flag per vertex, at most 50 -- the -0x144 array).
//      None inside: 0.  All inside: next edge.  Otherwise a
//      Sutherland-Hodgman pass into the output buffer, the new vertex at
//      t = -dPrev / (dCur - dPrev), lerping xyz, uv and the five colour
//      bytes +0x12, +0x11, +0x10, +0x13, +0x17 (big61a30.cpp's 0x10023380
//      order).  The polygon becomes the output, and the output pointer
//      moves on by the new count beyond its end.
//    * *ppVerts / *pnVerts get the result; 1.
//
//  NEAR, ONE BYTE (was 1387 of 1618): the frame.  Closed:
//    * the five byte lerps go through RoundF, the clipper family's
//      inlined __asm helper (w1_10001000.cpp) -- the shipped `fstps ;
//      flds ; fistpl`;
//    * the edge normal is VEC_CROSS(vNormal, vEdge, plane normal): the
//      earlier (plane normal, vEdge) order computed the NEGATED normal
//      (VEC_CROSS(d, a, b) is b x a), which the shipped product order
//      gave away;
//    * the inside flags are written and read back through a pointer
//      (`*pFlag = 1/0; nInside += *pFlag;`, pV and then pFlag stepped),
//      as shipped -- `add (%ecx),%ebx`;
//    * the clip loop takes pV = &pVerts[i] in its body.
//  What is left: `sub $0x18c,%esp` shipped against our 0x188.  Every
//  slot the shipped code touches is ours too (the lowest, -0x168, is the
//  first RoundF float); the extra four bytes are never addressed.  Not
//  moved by an inline lerp function in place of the macro, a float local
//  for the camera distance or the inside test, a camera copy, `<=`, or
//  locals for the two crossing distances; aInside[51] moves the array.
// *********************************************************************** //

#include "ltbasedefs.h"

struct Vec3
{
	float	x;                                          // +0x00
	float	y;                                          // +0x04
	float	z;                                          // +0x08
};

// d3dbatchb.cpp's plane: the distance of a point by value.
struct Plane10048150
{
	Vec3	m_Normal;                                   // +0x00
	float	m_fDist;                                    // +0x0c

	float sub_10048150(Vec3 v) const;                   // 0x10048150
};

struct Surf1005fb70
{
	uint8			m_pad_00[0x34];
	Plane10048150*	m_pPlane34;                         // +0x34
};

struct Edge1005fb70
{
	Vec3*			m_pVert;                            // +0x00
	uint8			m_pad_04[0x18 - 0x04];
};

struct Poly1005fb70
{
	uint8			m_pad_00[0x28];
	Surf1005fb70*	m_pSurf28;                          // +0x28
	uint8			m_pad_2c[0x50 - 0x2c];
	uint16			m_nEdges50;                         // +0x50
	uint16			m_pad_52;
	Edge1005fb70	m_Edges54[1];                       // +0x54
};

// The 32-byte vertex, FVF 0x1c4.
struct Vert1005fb70
{
	float	x;                                          // +0x00
	float	y;                                          // +0x04
	float	z;                                          // +0x08
	float	rhw;                                        // +0x0c
	uint8	m_Diffuse[4];                               // +0x10
	uint8	m_Specular[4];                              // +0x14
	float	tu;                                         // +0x18
	float	tv;                                         // +0x1c
};

class C10015910
{
public:
	uint32	sub_10015910(uint32 nArg);                  // 0x10015910
};

struct S1001e520Root;

extern S1001e520Root*	g_1008beac;                     // 0x1008beac
// The camera position, under the spelling big0f6f0.cpp / big16324.cpp /
// big17c14.cpp already give it (not yet defined by d3ddata.cpp).
struct LTVec
{
	float	x, y, z;
};

extern LTVec			g_v1008bd34;                    // 0x1008bd34 camera
extern uint32			g_1008be44;                     // clipped-poly count

#define CLIP1005FB70_MAX	50

// Round to nearest in the current FPU mode (0x10015190 out of line): the
// clipper family's __asm helper (w1_10001000.cpp), whose inlined copy is
// the shipped `fstps ; flds ; fistpl`.
inline int32 RoundF(float f)
{
	int32 n;
	__asm fld f
	__asm fistp n
	return n;
}

#define VERT_LERP_1005fb70(d, a, b, t)                                    \
	{                                                                     \
		(d)->x  = ((b)->x  - (a)->x)  * (t) + (a)->x;                     \
		(d)->y  = ((b)->y  - (a)->y)  * (t) + (a)->y;                     \
		(d)->z  = ((b)->z  - (a)->z)  * (t) + (a)->z;                     \
		(d)->tu = ((b)->tu - (a)->tu) * (t) + (a)->tu;                    \
		(d)->tv = ((b)->tv - (a)->tv) * (t) + (a)->tv;                    \
		(d)->m_Diffuse[2]  = (uint8)RoundF(((b)->m_Diffuse[2]  - (a)->m_Diffuse[2])  * (t) + (a)->m_Diffuse[2]);  \
		(d)->m_Diffuse[1]  = (uint8)RoundF(((b)->m_Diffuse[1]  - (a)->m_Diffuse[1])  * (t) + (a)->m_Diffuse[1]);  \
		(d)->m_Diffuse[0]  = (uint8)RoundF(((b)->m_Diffuse[0]  - (a)->m_Diffuse[0])  * (t) + (a)->m_Diffuse[0]);  \
		(d)->m_Diffuse[3]  = (uint8)RoundF(((b)->m_Diffuse[3]  - (a)->m_Diffuse[3])  * (t) + (a)->m_Diffuse[3]);  \
		(d)->m_Specular[3] = (uint8)RoundF(((b)->m_Specular[3] - (a)->m_Specular[3]) * (t) + (a)->m_Specular[3]); \
	}


// ======================================================================= //
//  0x1005fb70  1170 bytes
// ======================================================================= //
extern "C" int32 sub_1005fb70(void* pObj, int32 nIndex,
                              Vert1005fb70** ppVerts, int32* pnVerts,
                              Vert1005fb70* pOut)
{
	Poly1005fb70*	pPoly;
	Plane10048150*	pPlane;
	Edge1005fb70*	pEdge;
	Edge1005fb70*	pEdgeEnd;
	Edge1005fb70*	pPrevEdge;
	Vert1005fb70*	pVerts;
	int32			nVerts;
	Vert1005fb70*	pV;
	Vert1005fb70*	pPrev;
	Vert1005fb70*	pStart;
	Vec3			vEdge;
	Vec3			vNormal;
	float			fDist;
	float			fPrev, fCur, t;
	int32			aInside[CLIP1005FB70_MAX];
	int32			nInside;
	int32*			pFlag;
	int32			i, iPrev;

	if (!g_1008beac)
		return 0;

	pPoly = (Poly1005fb70*)((C10015910*)g_1008beac)->sub_10015910(nIndex);
	if (!pPoly)
		return 0;

	if (!(pPoly->m_pSurf28->m_pPlane34->sub_10048150(*(Vec3*)&g_v1008bd34) > 0.01f))
		return 0;

	pVerts = *ppVerts;
	nVerts = *pnVerts;

	pEdgeEnd = &pPoly->m_Edges54[pPoly->m_nEdges50];
	pPrevEdge = pEdgeEnd - 1;

	for (pEdge = pPoly->m_Edges54; pEdge != pEdgeEnd; pEdge++)
	{
		pPlane = pPoly->m_pSurf28->m_pPlane34;

		VEC_SUB(vEdge, *pEdge->m_pVert, *pPrevEdge->m_pVert);
		VEC_CROSS(vNormal, vEdge, pPlane->m_Normal);
		VEC_NORM(vNormal);
		fDist = VEC_DOT(vNormal, *pEdge->m_pVert);

		g_1008be44++;

		nInside = 0;
		pFlag = aInside;
		for (pV = pVerts; pV != &pVerts[nVerts]; pFlag++, pV++)
		{
			if (VEC_DOT(*pV, vNormal) - fDist > 0.0f)
				*pFlag = 1;
			else
				*pFlag = 0;

			nInside += *pFlag;
		}

		if (!nInside)
			return 0;

		if (nInside != nVerts)
		{
			pStart = pOut;
			iPrev = nVerts - 1;
			pPrev = &pVerts[iPrev];

			for (i = 0; i < nVerts; i++)
			{
				pV = &pVerts[i];

				if (aInside[iPrev])
					*pOut++ = *pPrev;

				if (aInside[iPrev] != aInside[i])
				{
					fPrev = VEC_DOT(vNormal, *pPrev) - fDist;
					fCur = VEC_DOT(vNormal, *pV) - fDist;
					t = -fPrev / (fCur - fPrev);

					VERT_LERP_1005fb70(pOut, pPrev, pV, t);
					pOut++;
				}

				iPrev = i;
				pPrev = pV;
			}

			nVerts = pOut - pStart;
			pVerts = pStart;
			pOut += nVerts;
		}

		pPrevEdge = pEdge;
	}

	*ppVerts = pVerts;
	*pnVerts = nVerts;
	return 1;
}
