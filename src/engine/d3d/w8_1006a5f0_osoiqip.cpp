// *********************************************************************** //
//
//  w8_1006a5f0_osoiqip.cpp -- the segment-intersection entry points
//  0x1006a5f0 and 0x1006a630.  /Os /Oi- /Qip (see build/engine.sh).
//
//  WHY /Qip, AND WHY ONE FILE.  0x1006a5f0 is /Qip's external two-entry
//  pair: eight bytes of `mov 0x4(%esp),%eax ; mov 0x8(%esp),%edx` falling
//  into the register body at 0x1006a5f8.  0x1006a630 calls that register
//  body (`lea 0x8(%esp),%edx ; call 0x1006a5f8` with %eax = its first
//  argument), which only a caller in the same /Qip translation unit does.
//  0x1006a630 itself has no in-file caller and so no stack shim; its
//  outside caller is d3dr5b.cpp's 0x10069110.  Both pad the /Os way (a
//  2-byte `mov %esi,%esi`, then 0x90s).
//
//  THE QUERY (24 bytes, a local of 0x1006a630): the two segment ends
//  (+0x00, +0x04), where the impact point goes (+0x08), a filter object
//  (+0x0c: its +0x2c is a callback taking +0x30 as user data) and a second
//  object (+0x10: vtable +0x08 called on the node) that 0x1006a630 leaves
//  NULL, and the node that was hit (+0x14).  The roles are 0x1006a1b0's:
//  it copies the impact point through +0x08 with `rep movsl` and stores
//  the node into +0x14 (0x1006a53c..0x1006a54a), and 0x1006a5f8 copies the
//  two vectors behind +0x00 and +0x04 into its by-value arguments.
//
//  0x1006a1b0, the recursive walk they reach, is written here too (its
//  padding is /Os and its sqrt is the CRT call, so the flags fit): NEAR,
//  991 bytes against 1076 -- the shipped code builds each vector
//  difference and cross product in a temporary and copies it with `rep
//  movsl`, ours computes in place, and the plane distances are scheduled
//  differently.  0x1006a630 is NEAR by nine bytes of store order (below).
//
//  d3dr5b.cpp declared 0x1006a630's third parameter `uint32 nFlags`; the
//  bytes store it into the query's +0x04, the second segment end, so it is
//  a vector pointer and is spelled as one here.  link/aliases.rsp maps
//  d3dr5b.cpp's spelling onto this one.
// *********************************************************************** //

#include "ltbasedefs.h"

struct D3DVec3
{
	float	x;                                          // +0x00
	float	y;                                          // +0x04
	float	z;                                          // +0x08
};

// d3dr5b.cpp's sixteen-byte plane buffer: a normal and a distance.
struct Plane10069110
{
	D3DVec3		m_Normal;                               // +0x00
	float		m_Dist;                                 // +0x0c
};

struct Surf1006a5f0
{
	uint8			m_pad_00[0x34];
	Plane10069110*	m_pPlane34;                         // +0x34
	uint32			m_nFlags38;                         // +0x38 (1 = solid)
};

struct Edge1006a5f0
{
	LTVector*		m_pVert;                            // +0x00
	uint8			m_pad_04[0x18 - 0x04];
};

struct Poly1006a5f0
{
	uint8			m_pad_00[0x18];
	LTVector		m_vCenter18;                        // +0x18
	float			m_fRadius24;                        // +0x24
	Surf1006a5f0*	m_pSurf28;                          // +0x28
	uint8			m_pad_2c[0x50 - 0x2c];
	uint16			m_nVerts50;                         // +0x50
	uint16			m_pad_52;
	Edge1006a5f0	m_Edges54[1];                       // +0x54
};

// A BSP node: its splitting polygon, the back (0) and front (1) children,
// and at +0x16 the byte whose low two bits mark an empty / solid leaf.
struct Hit1006a5f0
{
	Poly1006a5f0*	m_pPoly00;                          // +0x00
	Hit1006a5f0*	m_pSides04[2];                      // +0x04
	uint8			m_pad_0c[0x16 - 0x0c];
	uint8			m_nFlags16;                         // +0x16

	Plane10069110*	GetPlane()	{ return m_pPoly00->m_pSurf28->m_pPlane34; }
};

// The query's filter (+0x0c): a callback at +0x2c taking the user data at
// +0x30; and the object at +0x10 whose vtable slot +0x08 maps the node to
// what the callback is given.
struct Filter1006a5f0
{
	uint8			m_pad_00[0x2c];
	LTBOOL			(*m_pfnFilter2c)(void* pObj, void* pUser);  // +0x2c
	void*			m_pUser30;                          // +0x30
};

class Mapper1006a5f0
{
public:
	virtual void	_v00() = 0;
	virtual void	_v04() = 0;
	virtual void*	Slot08(Hit1006a5f0* pNode) = 0;     // +0x08
};

struct Query1006a5f0
{
	D3DVec3*		m_pFrom;                            // +0x00
	D3DVec3*		m_pTo;                              // +0x04
	D3DVec3*		m_pIPoint;                          // +0x08
	Filter1006a5f0*	m_pFilter0c;                        // +0x0c
	Mapper1006a5f0*	m_pUser10;                          // +0x10
	Hit1006a5f0*	m_pHit14;                           // +0x14
};

#define EPS_1006a1b0	0.01f

#define PLANE_DIST_1006a1b0(p, v) \
	((p)->m_Normal.x * (v).x + (p)->m_Normal.y * (v).y + \
	 (p)->m_Normal.z * (v).z - (p)->m_Dist)


// ======================================================================= //
//  0x1006a1b0  1076 bytes.  The recursive segment / BSP walk.  __cdecl:
//  the node, the query and the two segment ends BY VALUE (0x1006a5f8 and
//  the recursion at 0x1006a2fe both build them with `rep movsl` of three
//  dwords).
//
//  Down the tree while both ends are on one side of the node's plane
//  (+-0.01f); where the segment crosses, split it at t = -d1 / (d2 - d1),
//  walk the near half first (recursing into the start's side), and on a
//  front-to-back crossing test the node's own polygon: solid surface
//  (flag 1), the split point inside its bounding sphere (+0x18, +0x24)
//  and inside every edge plane (Cross(poly normal, prev - cur),
//  normalised -- the CRT sqrt CALLED, so /Oi-), then the query's filter.
//  A hit stores the node into the query's +0x14 and the point through its
//  +0x08.  Otherwise carry on down the far side from the split point,
//  giving up when a back-side split falls past t = 0.99.
// ======================================================================= //
// ONE temporary vector for the centre difference and the
// edge (the shipped frame reuses -0x60 for both), d1/d2 as
// ((LTPlane*)pNode->GetPlane())->DistTo(v) -- an inline Node::GetPlane, as
// d3dqip.cpp found for 0x10040930 -- the filter object in a local, and
// pEnd computed before pPrev = pEnd - 1: 991 -> 1059 bytes (shipped 1076),
// frame 0x98 exact, four mnemonic blocks left -- the `t > 0.99f` exit
// (ja vs jbe+jmp), the edge Dot's term order (z,y,x against y,x,z), and
// the else-branch's `d1 > EPS` retest the shipped code keeps and icl
// here folds away (tried: `||` forms, a ternary, d1 as double -- the last
// brings back icl's stack-realigning entry).
// 1059 -> 1076 bytes, ZERO mnemonic blocks, all three closed:
//    * the both-sides tests as the server twin writes them
//      (bspq_IntersectLineRec, server packet.cpp): `d1 > EPS && d2 > EPS`
//      first, then `if (d1 < -EPS) {..}` with `else if (d1 > EPS)` as the
//      spanning case's first test -- icl threads the first `&&`'s d1 test
//      straight into `iSide = 1` and KEEPS the `d1 > EPS` retest, as shipped;
//    * each dot as `a = y + x; a += z;` -- icl puts a flat three-term float
//      sum in z,y,x order whatever the source order (six permutations,
//      Dot from either side: all z,y,x), only the split statement keeps y,x;
//    * `t > 0.99f` BREAKs to the loop's `return LTFALSE` (a direct `ja`
//      back to the shared exit) where a `return LTFALSE` there was a
//      separate block icl tail-merged into `jbe +5 ; jmp`.
// NEAR, 34 instructions differ, all frame-slot operands: our spill slots
// for pNode/iSide/pQuery/pPlane/pEnd sit at -0x20..-0x30, shipped at
// -0x1c..-0x2c -- ours has an unreferenced 4-byte slot at -0x1c, shipped
// one at -0x30.  Unchanged by: declaration order (d1.. last, iSide/pPoly/
// pFilter moved), t/pPrev/pEdge/fEdgeDist block-scoped, fDenom folded
// away, pFilter removed (worse), iSide uint32/LTBOOL (worse), Norm(1.0f)
// or written out, pNode/pQuery copied from renamed parameters, a for(;;)
// with the flag test inside (worse), unused functions before it.
LTBOOL sub_1006a1b0(Hit1006a5f0* pNode, Query1006a5f0* pQuery,
                    LTVector vFrom, LTVector vTo)
{
	float			d1, d2, fDenom, t;
	int32			iSide;
	LTVector		vMid;
	LTVector		vTemp;
	LTVector		vNormal;
	Poly1006a5f0*	pPoly;
	Plane10069110*	pPlane;
	Edge1006a5f0*	pEdge;
	Edge1006a5f0*	pPrev;
	Edge1006a5f0*	pEnd;
	float			fEdgeDist, fMidDist;
	Filter1006a5f0*	pFilter;

	while (!(pNode->m_nFlags16 & 3))
	{
		d1 = ((LTPlane*)pNode->GetPlane())->DistTo(vFrom);
		d2 = ((LTPlane*)pNode->GetPlane())->DistTo(vTo);

		if (d1 > EPS_1006a1b0 && d2 > EPS_1006a1b0)
		{
			pNode = pNode->m_pSides04[1];
			continue;
		}

		if (d1 < -EPS_1006a1b0)
		{
			if (d2 < -EPS_1006a1b0)
			{
				pNode = pNode->m_pSides04[0];
				continue;
			}

			iSide = 0;
		}
		else if (d1 > EPS_1006a1b0)
			iSide = 1;
		else if (d2 < 0.0f)
			iSide = 1;
		else
			iSide = 0;

		fDenom = d2 - d1;
		if (fDenom != 0.0f && (d1 < -EPS_1006a1b0 || d1 > EPS_1006a1b0))
		{
			t = -d1 / fDenom;
			VEC_LERP(vMid, vFrom, vTo, t);

			if (sub_1006a1b0(pNode->m_pSides04[iSide], pQuery, vFrom, vMid))
				return LTTRUE;

			if (iSide == 1 && (pPoly = pNode->m_pPoly00) != LTNULL)
			{
				if (!(pPoly->m_pSurf28->m_nFlags38 & 1))
					goto Miss;

				vTemp = pPoly->m_vCenter18 - vMid;
				if (pPoly->m_fRadius24 * pPoly->m_fRadius24 < vTemp.MagSqr())
					goto Miss;

				pPlane = pPoly->m_pSurf28->m_pPlane34;
				pEnd = &pPoly->m_Edges54[pPoly->m_nVerts50];
				pPrev = pEnd - 1;

				for (pEdge = pPoly->m_Edges54; pEdge != pEnd; pEdge++)
				{
					vTemp = *pPrev->m_pVert - *pEdge->m_pVert;
					vNormal = ((LTVector*)&pPlane->m_Normal)->Cross(vTemp);

					vNormal.Norm();

					fEdgeDist  = vNormal.y * pEdge->m_pVert->y + vNormal.x * pEdge->m_pVert->x;
					fEdgeDist += vNormal.z * pEdge->m_pVert->z;

					fMidDist  = vNormal.y * vMid.y + vNormal.x * vMid.x;
					fMidDist += vNormal.z * vMid.z;

					if (fMidDist - fEdgeDist < -EPS_1006a1b0)
						goto Miss;

					pPrev = pEdge;
				}

				pFilter = pQuery->m_pFilter0c;
				if (pFilter && pFilter->m_pfnFilter2c && pQuery->m_pUser10)
				{
					if (!pFilter->m_pfnFilter2c(pQuery->m_pUser10->Slot08(pNode),
					                            pFilter->m_pUser30))
						goto Miss;
				}

				pQuery->m_pHit14 = pNode;
				*(LTVector*)pQuery->m_pIPoint = vMid;
				return LTTRUE;
			}
			else if (t > 0.99f)
			{
				break;		// to the loop's own `return LTFALSE`
			}

		Miss:
			vFrom = vMid;
		}

		pNode = pNode->m_pSides04[iSide == 0];
	}

	return LTFALSE;
}


// ======================================================================= //
//  0x1006a5f0  8-byte stack entry + 56-byte register body at 0x1006a5f8.
// ======================================================================= //
LTBOOL sub_1006a5f0(void* pNode, Query1006a5f0* pQuery)
{
	return sub_1006a1b0((Hit1006a5f0*)pNode, pQuery,
	                    *(LTVector*)pQuery->m_pFrom, *(LTVector*)pQuery->m_pTo);
}


// ======================================================================= //
//  0x1006a630  104 bytes.  Answers the node hit (or NULL) and copies the
//  plane of the polygon it hit into the caller's buffer.
// ======================================================================= //
void* sub_1006a630(void* pWorld, D3DVec3* pFrom, D3DVec3* pTo,
                   D3DVec3* pIPoint, Plane10069110* pPlane)
{
	Query1006a5f0	query;

	D3DVec3**		ppTo;

	// IDENTICAL: the shipped code makes the three
	// zero stores (+0x14, +0x0c, +0x10) before the three argument stores
	// (+0x04, +0x00, +0x08).  Written as plain member stores, icl
	// schedules +0x04/+0x00 ahead of the zeros in every statement order
	// tried (9 bytes differ); the +0x04 store made through a pointer to
	// the field keeps all three argument stores after the zeros, as
	// shipped.  (All three through pointers: +0x00 goes before +0x04.)
	query.m_pHit14 = LTNULL;
	query.m_pFilter0c = LTNULL;
	query.m_pUser10 = LTNULL;
	ppTo = &query.m_pTo;
	*ppTo = pTo;
	query.m_pFrom = pFrom;
	query.m_pIPoint = pIPoint;

	if (!sub_1006a5f0(pWorld, &query))
		return LTNULL;

	*pPlane = *query.m_pHit14->m_pPoly00->m_pSurf28->m_pPlane34;
	return query.m_pHit14;
}
