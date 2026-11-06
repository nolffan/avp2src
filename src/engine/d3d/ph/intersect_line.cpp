// *********************************************************************** //
//  ph/intersect_line.cpp -- Primal Hunt's d3d.ren, TU35, fourth object
//  (0x10032e09..0x100331d1), /O1.
//
//  Line segment intersects in the BSP (NOLF2's runtime/world/src/
//  intersect_line.cpp is its descendant): a recursive walk down the
//  nodes the segment crosses, testing the front-facing polygon of each
//  crossed plane.  This older version takes the crossing only when the
//  start point is off the plane, and its polygon test rejects non-solid
//  surfaces first.
//
//  InsideConvex (0x10033013) is a near: two terms of the edge plane's
//  distance are added in the other order.
// *********************************************************************** //

#include "ltbasedefs.h"


#define INTERSECT_EPSILON	0.01f

#define NF_IN				1
#define NF_OUT				2

#define FrontSide			1
#define BackSide			0



struct Surface
{
	uint8		m_Pad[0x30];
	uint8		m_Flags;                                // +0x30
};

struct SPolyVertex
{
	LTVector	*m_Vec;                                 // +0x00
	uint8		m_Pad[0x18 - 0x4];
};

struct WorldPoly
{
	uint8		m_Pad[0x18];
	LTVector	m_Center;                               // +0x18
	float		m_Radius;                               // +0x24
	LTPlane		*m_pPlane;                              // +0x28
	Surface		*m_pSurface;                            // +0x2c
	uint8		m_Pad30[0x54 - 0x30];
	uint16		m_nVertices;                            // +0x54
	SPolyVertex	m_Vertices[1];                          // +0x58

	LTVector&	GetCenter()		{return m_Center;}
	float		GetRadius()		{return m_Radius;}
	LTPlane*	GetPlane()		{return m_pPlane;}
	Surface*	GetSurface()	{return m_pSurface;}
};

struct Node
{
	WorldPoly	*m_pPoly;                               // +0x00
	Node		*m_Sides[2];                            // +0x04
	uint8		m_Pad0c[0x16 - 0xc];
	uint8		m_Flags;                                // +0x16

	LTPlane*	GetPlane() const
	{
		if (m_pPoly && m_pPoly->m_pSurface)
			return m_pPoly->GetPlane();
		else
			return LTNULL;
	}
};

class WorldBsp
{
public:
	virtual void	Reserved();
	virtual Node*	GetRootNode();                      // +0x04
	virtual HPOLY	MakeHPoly(const Node *pNode);       // +0x08
};

struct IntersectRequest
{
	IntersectRequest()
	{
		m_pPoints[0] = LTNULL;
		m_pPoints[1] = LTNULL;
		m_pIPos = LTNULL;
		m_pNodeHit = LTNULL;
		m_pQuery = LTNULL;
		m_pWorldBsp = LTNULL;
	}

	LTVector			*m_pPoints[2];                  // +0x00
	LTVector			*m_pIPos;                       // +0x08
	IntersectQuery		*m_pQuery;                      // +0x0c
	WorldBsp			*m_pWorldBsp;                   // +0x10
	Node				*m_pNodeHit;                    // +0x14
};


// Tells if pPt is inside the convex poly.
inline LTBOOL InsideConvex(WorldPoly *pPoly, LTVector *pPt)
{
	LTPlane edgePlane;
	float edgeDot;
	LTVector *pNormal, vTemp;
	SPolyVertex *pCurVert, *pPrevVert, *pEndVert;
	float radius;

	if (!(pPoly->GetSurface()->m_Flags & SURF_SOLID))
		return LTFALSE;

	// Reject it if it's outside the radius of the poly
	vTemp = pPoly->GetCenter() - *pPt;
	radius = pPoly->GetRadius();
	if (vTemp.MagSqr() > (radius * radius))
		return LTFALSE;

	pNormal = &pPoly->GetPlane()->m_Normal;
	pEndVert = &pPoly->m_Vertices[pPoly->m_nVertices];
	pPrevVert = pEndVert - 1;

	for(pCurVert=pPoly->m_Vertices; pCurVert != pEndVert; )
	{
		vTemp = *pPrevVert->m_Vec - *pCurVert->m_Vec;
		edgePlane.m_Normal = pNormal->Cross(vTemp);
		edgePlane.m_Normal.Norm();
		edgePlane.m_Dist = edgePlane.m_Normal.Dot(*pCurVert->m_Vec);

		edgeDot = edgePlane.DistTo(*pPt);
		if(edgeDot < -INTERSECT_EPSILON)
			return LTFALSE;

		pPrevVert = pCurVert;
		++pCurVert;
	}

	return LTTRUE;
}


// 0x10032e09
LTBOOL InternalIntersectLineNode(Node *pRoot, IntersectRequest *pRequest,
	LTVector *pPoint1, LTVector *pPoint2)
{
	LTVector point1, iPoint;
	float intersection_t;
	float dot1, dot2;
	int side1;

	point1 = *pPoint1;

	while((pRoot->m_Flags & (NF_IN|NF_OUT)) == 0)
	{
		// Go into the correct side.
		dot1 = pRoot->GetPlane()->DistTo(point1);
		dot2 = pRoot->GetPlane()->DistTo(*pPoint2);

		// Handle the segment being entirely on one side of the plane
		if(dot1 > INTERSECT_EPSILON && dot2 > INTERSECT_EPSILON)
		{
			pRoot = pRoot->m_Sides[FrontSide];
		}
		else if(dot1 < -INTERSECT_EPSILON && dot2 < -INTERSECT_EPSILON)
		{
			pRoot = pRoot->m_Sides[BackSide];
		}
		else
		{
			// Ok, it crosses this plane.. go into the side that pFrom is on first.
			if ((dot1 < -INTERSECT_EPSILON) || (dot1 > INTERSECT_EPSILON))
				side1 = (int)(dot1 > 0.0f);
			else
				side1 = (int)(dot2 < 0.0f);

			// Get the difference between the distances
			intersection_t = dot2 - dot1;
			if (intersection_t != 0.0f)
			{
				if ((dot1 < -INTERSECT_EPSILON) || (dot1 > INTERSECT_EPSILON))
				{
					// Find the point of intersection
					intersection_t = -dot1 / intersection_t;
					VEC_LERP(iPoint, point1, *pPoint2, intersection_t);

					// Test the side the starting point is on.
					if (InternalIntersectLineNode(pRoot->m_Sides[side1], pRequest, &point1, &iPoint))
						return LTTRUE;

					// Check for a polygon intersection
					if((side1 == FrontSide) && (pRoot->m_pPoly))
					{
						if(InsideConvex(pRoot->m_pPoly, &iPoint))
						{
							LTBOOL bDone = LTTRUE;

							IntersectQuery* pQuery = pRequest->m_pQuery;
							if (pQuery && pQuery->m_PolyFilterFn && pRequest->m_pWorldBsp)
							{
								if (!pQuery->m_PolyFilterFn(pRequest->m_pWorldBsp->MakeHPoly(pRoot), pQuery->m_pUserData))
								{
									bDone = LTFALSE;
								}
							}

							if (bDone)
							{
								// Congratulations, we have a winner!
								pRequest->m_pNodeHit = pRoot;
								*pRequest->m_pIPos = iPoint;
								return LTTRUE;
							}
						}
					}
					// Jump out if the ray doesn't go to the "other" side
					else if (intersection_t > (1.0f - INTERSECT_EPSILON))
						return LTFALSE;

					// Clip the segment to the plane
					point1 = iPoint;
				}
			}

			// Go into the other side.
			pRoot = pRoot->m_Sides[!side1];
		}
	}

	return LTFALSE;
}


// 0x1003315c
LTBOOL IntersectLineNode(Node *pRoot, IntersectRequest *pRequest)
{
	return InternalIntersectLineNode(pRoot, pRequest, pRequest->m_pPoints[0], pRequest->m_pPoints[1]);
}


// 0x10033173
Node* IntersectLine(Node *pRoot, LTVector *pPoint1, LTVector *pPoint2, LTVector *pIPos, LTPlane *pIPlane)
{
	IntersectRequest req;

	req.m_pPoints[0] = pPoint1;
	req.m_pPoints[1] = pPoint2;
	req.m_pIPos = pIPos;

	if(InternalIntersectLineNode(pRoot, &req, pPoint1, pPoint2))
	{
		*pIPlane = *req.m_pNodeHit->GetPlane();
		return req.m_pNodeHit;
	}

	return LTNULL;
}
