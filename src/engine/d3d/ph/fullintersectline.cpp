// *********************************************************************** //
//  ph/fullintersectline.cpp -- Primal Hunt's d3d.ren, TU35, first object
//  (0x10031a30..0x10032af0), /Os.
//
//  The client's segment intersector (i_IntersectSegment): the world tree
//  walked with a per-object callback, each object rejected by a sphere
//  test, the query's filter and then either its world model's BSP or its
//  bounding box.  NOLF2's runtime/world/src/fullintersectline.cpp is this
//  file's descendant, function for function.
//
//  Built without the optimiser's global pass (plain /Os): every local
//  lives in the frame, every function has an ebp frame, and the inline
//  vector members are called out of line -- the shape NOLF2's source notes
//  ("the raycasting is all screwed up") explains.  .bss 0x1007c178..
//  0x1007c1e4.
// *********************************************************************** //

#include "ltbasedefs.h"


#define IGNORE_NONSOLID			2
#define INTERSECT_OBJECTS		1
#define INTERSECT_HPOLY			4

#define FLAG_SOLID_RAYHIT		0x3000

#define OT_WORLDMODEL			2
#define OT_CONTAINER			9


struct Surface
{
	uint8		m_Pad[0x34];
	uint16		m_TextureFlags;                         // +0x34
};

struct WorldPoly
{
	uint8		m_Pad[0x28];
	LTPlane		*m_pPlane;                              // +0x28
	Surface		*m_pSurface;                            // +0x2c

	LTPlane*	GetPlane()		{return m_pPlane;}
	Surface*	GetSurface()	{return m_pSurface;}
};

struct Node
{
	WorldPoly	*m_pPoly;                               // +0x00

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

class LTObject
{
public:
	virtual void	Reserved00();
	virtual void	Reserved04();
	virtual void	Reserved08();
	virtual void	Reserved0c();
	virtual void	Reserved10();
	virtual void	Reserved14();
	virtual void	Reserved18();
	virtual void	Reserved1c();
	virtual void	Reserved20();
	virtual void	Reserved24();
	virtual void	Reserved28();
	virtual void	Reserved2c();
	virtual void	Reserved30();
	virtual LTBOOL	IsMainWorldModel();                 // +0x34

	float		GetRadius()			{return m_Radius;}
	float		GetRadiusSquared()	{return m_Radius * m_Radius;}
	LTVector&	GetPos()			{return m_Pos;}

	uint8		m_Pad004[0x88 - 0x4];
	uint32		m_Flags;                                // +0x088
	uint8		m_Pad08c[0xbc - 0x8c];
	char		m_ObjectType;                           // +0x0bc
	uint8		m_Pad0bd[0xf4 - 0xbd];
	LTVector	m_MinBox;                               // +0x0f4
	LTVector	m_MaxBox;                               // +0x100
	uint8		m_Pad10c[0x194 - 0x10c];
	LTVector	m_Pos;                                  // +0x194
	uint8		m_Pad1a0[0x1ac - 0x1a0];
	float		m_Radius;                               // +0x1ac
};

class WorldModelInstance : public LTObject
{
public:
	WorldBsp	*m_pOriginalBsp;                        // +0x1b0
	uint8		m_Pad1b4[0x1f8 - 0x1b4];
	LTMatrix	m_Transform;                            // +0x1f8
	LTMatrix	m_BackTransform;                        // +0x238
};

inline LTBOOL HasWorldModel(LTObject *pObj)
{
	return pObj->m_ObjectType == OT_WORLDMODEL || pObj->m_ObjectType == OT_CONTAINER;
}

typedef LTBOOL (*WorldTreeISFn)(LTObject *pObj, void *pUser);

class WorldTree
{
public:
	virtual void	Reserved00();
	virtual void	Reserved04();
	virtual void	Reserved08();
	virtual void	Reserved0c();
	virtual void	IntersectSegment(LTVector *pPoint1, LTVector *pPoint2,
						WorldTreeISFn fn, void *pUser, void *pUser2);   // +0x10
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

// intersect_line.cpp (TU35's third object).
LTBOOL IntersectLineNode(Node *pRoot, IntersectRequest *pRequest);            // 0x1003315c
Node* IntersectLine(Node *pRoot, LTVector *pPoint1, LTVector *pPoint2,
	LTVector *pIPos, LTPlane *pIPlane);                                         // 0x10033173


uint32 g_nIntersectCalls;                               // 0x1007c1cc
float g_fIntersectLineLen;                              // 0x1007c1d8


static void (*g_FindIntersectionsFn)(WorldBsp *pWorldBsp, Node **pNodeIntersectionPtr,
	LTVector *pIntersectionPosPtr, float *pDistSqrPtr, HPOLY *hWorldPoly,
	LTVector *pPoint1, LTVector *pPoint2, uint8 bWorldModel);              // 0x1007c178


// The current query.
static IntersectQuery *g_pCurQuery;                     // 0x1007c1a4
static uint8 g_bProcessNonSolid;                        // 0x1007c1a0
static uint8 g_bProcessObjects;                         // 0x1007c1d0


// The current best intersection (LTNULL if none).
static Node *g_pWorldIntersection;                      // 0x1007c19c
static LTObject *g_pIntersection;                       // 0x1007c18c
static float g_IntersectionBestDistSqr;                 // 0x1007c1dc
static LTPlane g_IntersectionPlane;                     // 0x1007c1b8
static LTVector g_IntersectionPos;                      // 0x1007c1a8
static HPOLY g_hWorldPoly;                              // 0x1007c1e0

static LTVector g_V, g_VTimesInvVV;                     // 0x1007c180, 0x1007c190
static float g_VPTimesInvVV, g_LineLen;                 // 0x1007c1d4, 0x1007c1b4


#ifdef AVP2SRC_MODERN
// Each face's plane at that face (planeCoord), not at the minimum corner.
// Get the intersection point and see if it's inside the other dimensions.
#define DO_PLANE_TEST_X(planeCoord, coord0, coord1, coord2, normalDirection) \
	t = (planeCoord - Point1.coord0) / (Point2.coord0 - Point1.coord0);\
	testCoords[0] = Point1.coord1 + ((Point2.coord1 - Point1.coord1) * t);\
	if (testCoords[0] > pServerObj->m_MinBox.coord1 && testCoords[0] < pServerObj->m_MaxBox.coord1)\
	{\
		testCoords[1] = Point1.coord2 + ((Point2.coord2 - Point1.coord2) * t);\
		if (testCoords[1] > pServerObj->m_MinBox.coord2 && testCoords[1] < pServerObj->m_MaxBox.coord2)\
		{\
			pIntersectPt->coord0 = planeCoord;\
			pIntersectPt->coord1 = testCoords[0];\
			pIntersectPt->coord2 = testCoords[1];\
			pIntersectPlane->m_Normal.x = normalDirection;\
			pIntersectPlane->m_Normal.y = 0.0f;\
			pIntersectPlane->m_Normal.z = 0.0f;\
			pIntersectPlane->m_Dist = (planeCoord) * normalDirection;\
			return true;\
		}\
	}

#define DO_PLANE_TEST_Y(planeCoord, coord0, coord1, coord2, normalDirection) \
	t = (planeCoord - Point1.coord0) / (Point2.coord0 - Point1.coord0);\
	testCoords[0] = Point1.coord1 + ((Point2.coord1 - Point1.coord1) * t);\
	if (testCoords[0] > pServerObj->m_MinBox.coord1 && testCoords[0] < pServerObj->m_MaxBox.coord1)\
	{\
		testCoords[1] = Point1.coord2 + ((Point2.coord2 - Point1.coord2) * t);\
		if (testCoords[1] > pServerObj->m_MinBox.coord2 && testCoords[1] < pServerObj->m_MaxBox.coord2)\
		{\
			pIntersectPt->coord0 = planeCoord;\
			pIntersectPt->coord1 = testCoords[0];\
			pIntersectPt->coord2 = testCoords[1];\
			pIntersectPlane->m_Normal.x = 0.0f;\
			pIntersectPlane->m_Normal.y = normalDirection;\
			pIntersectPlane->m_Normal.z = 0.0f;\
			pIntersectPlane->m_Dist = (planeCoord) * normalDirection;\
			return true;\
		}\
	}

#define DO_PLANE_TEST_Z(planeCoord, coord0, coord1, coord2, normalDirection) \
	t = (planeCoord - Point1.coord0) / (Point2.coord0 - Point1.coord0);\
	testCoords[0] = Point1.coord1 + ((Point2.coord1 - Point1.coord1) * t);\
	if (testCoords[0] > pServerObj->m_MinBox.coord1 && testCoords[0] < pServerObj->m_MaxBox.coord1)\
	{\
		testCoords[1] = Point1.coord2 + ((Point2.coord2 - Point1.coord2) * t);\
		if (testCoords[1] > pServerObj->m_MinBox.coord2 && testCoords[1] < pServerObj->m_MaxBox.coord2)\
		{\
			pIntersectPt->coord0 = planeCoord;\
			pIntersectPt->coord1 = testCoords[0];\
			pIntersectPt->coord2 = testCoords[1];\
			pIntersectPlane->m_Normal.x = 0.0f;\
			pIntersectPlane->m_Normal.y = 0.0f;\
			pIntersectPlane->m_Normal.z = normalDirection;\
			pIntersectPlane->m_Dist = (planeCoord) * normalDirection;\
			return true;\
		}\
	}
#else
// Get the intersection point and see if it's inside the other dimensions.
#define DO_PLANE_TEST_X(planeCoord, coord0, coord1, coord2, normalDirection) \
	t = (planeCoord - Point1.coord0) / (Point2.coord0 - Point1.coord0);\
	testCoords[0] = Point1.coord1 + ((Point2.coord1 - Point1.coord1) * t);\
	if (testCoords[0] > pServerObj->m_MinBox.coord1 && testCoords[0] < pServerObj->m_MaxBox.coord1)\
	{\
		testCoords[1] = Point1.coord2 + ((Point2.coord2 - Point1.coord2) * t);\
		if (testCoords[1] > pServerObj->m_MinBox.coord2 && testCoords[1] < pServerObj->m_MaxBox.coord2)\
		{\
			pIntersectPt->coord0 = planeCoord;\
			pIntersectPt->coord1 = testCoords[0];\
			pIntersectPt->coord2 = testCoords[1];\
			pIntersectPlane->m_Normal.x = normalDirection;\
			pIntersectPlane->m_Normal.y = 0.0f;\
			pIntersectPlane->m_Normal.z = 0.0f;\
			pIntersectPlane->m_Dist = pServerObj->m_MinBox.x * normalDirection;\
			return true;\
		}\
	}

#define DO_PLANE_TEST_Y(planeCoord, coord0, coord1, coord2, normalDirection) \
	t = (planeCoord - Point1.coord0) / (Point2.coord0 - Point1.coord0);\
	testCoords[0] = Point1.coord1 + ((Point2.coord1 - Point1.coord1) * t);\
	if (testCoords[0] > pServerObj->m_MinBox.coord1 && testCoords[0] < pServerObj->m_MaxBox.coord1)\
	{\
		testCoords[1] = Point1.coord2 + ((Point2.coord2 - Point1.coord2) * t);\
		if (testCoords[1] > pServerObj->m_MinBox.coord2 && testCoords[1] < pServerObj->m_MaxBox.coord2)\
		{\
			pIntersectPt->coord0 = planeCoord;\
			pIntersectPt->coord1 = testCoords[0];\
			pIntersectPt->coord2 = testCoords[1];\
			pIntersectPlane->m_Normal.x = 0.0f;\
			pIntersectPlane->m_Normal.y = normalDirection;\
			pIntersectPlane->m_Normal.z = 0.0f;\
			pIntersectPlane->m_Dist = pServerObj->m_MinBox.y * normalDirection;\
			return true;\
		}\
	}

#define DO_PLANE_TEST_Z(planeCoord, coord0, coord1, coord2, normalDirection) \
	t = (planeCoord - Point1.coord0) / (Point2.coord0 - Point1.coord0);\
	testCoords[0] = Point1.coord1 + ((Point2.coord1 - Point1.coord1) * t);\
	if (testCoords[0] > pServerObj->m_MinBox.coord1 && testCoords[0] < pServerObj->m_MaxBox.coord1)\
	{\
		testCoords[1] = Point1.coord2 + ((Point2.coord2 - Point1.coord2) * t);\
		if (testCoords[1] > pServerObj->m_MinBox.coord2 && testCoords[1] < pServerObj->m_MaxBox.coord2)\
		{\
			pIntersectPt->coord0 = planeCoord;\
			pIntersectPt->coord1 = testCoords[0];\
			pIntersectPt->coord2 = testCoords[1];\
			pIntersectPlane->m_Normal.x = 0.0f;\
			pIntersectPlane->m_Normal.y = 0.0f;\
			pIntersectPlane->m_Normal.z = normalDirection;\
			pIntersectPlane->m_Dist = pServerObj->m_MinBox.z * normalDirection;\
			return true;\
		}\
	}
#endif


// Just sets up the current 'closest object'.
#define USE_THIS_OBJECT(pServerObj, distSqr, plane, intersectionPt, hPoly) \
	g_IntersectionBestDistSqr = distSqr;\
	g_pIntersection = pServerObj;\
	g_IntersectionPlane = plane;\
	g_IntersectionPos = intersectionPt;\
	g_hWorldPoly = hPoly;


static void i_FindIntersectionsHPoly(WorldBsp *pWorldBsp, Node **pNodeIntersectionPtr,
	LTVector *pIntersectionPosPtr, float *pDistSqrPtr, HPOLY *hWorldPoly,
	LTVector *pPoint1, LTVector *pPoint2, uint8 bWorldModel);
static void i_FindIntersections(WorldBsp *pWorldBsp, Node **pNodeIntersectionPtr,
	LTVector *pIntersectionPosPtr, float *pDistSqrPtr, HPOLY *hWorldPoly,
	LTVector *pPoint1, LTVector *pPoint2, uint8 bWorldModel);
static LTBOOL i_ISCallback(LTObject *pObj, void *pCBUser);


// 0x10031a5d -- pUser is unused: the renderer's one caller
// (drawmodel.cpp's d3d_SegmentHitsSky) passes LTNULL for it.
LTBOOL i_IntersectSegment(IntersectQuery *pQuery, IntersectInfo *pInfo, WorldTree *pWorldTree, void *pUser)
{
	float InvVV, VP, testMag;

	++g_nIntersectCalls;

	// Init..
	g_pCurQuery = pQuery;
	g_pIntersection = LTNULL;
	g_pWorldIntersection = LTNULL;
	g_hWorldPoly = INVALID_HPOLY;
	g_bProcessNonSolid = !(pQuery->m_Flags & IGNORE_NONSOLID);
	g_bProcessObjects = !!(pQuery->m_Flags & INTERSECT_OBJECTS);
	g_IntersectionBestDistSqr = (pQuery->m_From - pQuery->m_To).MagSqr() + 1.0f;

	// Precalculate stuff to totally accelerate i_QuickSphereTest.
	g_V = pQuery->m_To - pQuery->m_From;
	g_V /= (g_LineLen = g_V.Mag());
	g_fIntersectLineLen += g_LineLen;

	// Was it too short?
	testMag = g_V.MagSqr();
	if (testMag < 0.5f || testMag > 2.0f)
	{
		return LTFALSE;
	}

	VP = g_V.Dot(pQuery->m_From);
	InvVV = 1.0f / g_V.MagSqr();
	g_VTimesInvVV = g_V * InvVV;
	g_VPTimesInvVV = VP * InvVV;

	if (pQuery->m_Flags & INTERSECT_HPOLY)
	{
		g_FindIntersectionsFn = i_FindIntersectionsHPoly;
	}
	else
	{
		g_FindIntersectionsFn = i_FindIntersections;
	}

	// Start at the world tree.
	pWorldTree->IntersectSegment(&pQuery->m_From, &pQuery->m_To, i_ISCallback, LTNULL, LTNULL);

	// If an object was hit, use it!
	if (g_pIntersection)
	{
		pInfo->m_Point = g_IntersectionPos;
		pInfo->m_Plane = g_IntersectionPlane;
		pInfo->m_hObject = (HOBJECT)g_pIntersection;
		pInfo->m_hPoly = g_hWorldPoly;

		if (g_pWorldIntersection)
		{
			pInfo->m_SurfaceFlags = g_pWorldIntersection->m_pPoly->m_pSurface->m_TextureFlags;
		}
		else
		{
			pInfo->m_SurfaceFlags = 0;
		}

		return LTTRUE;
	}
	else
	{
		return LTFALSE;
	}
}


// 0x10031d0f -- finds intersections a slower way, but fills in hWorldPoly.
static void i_FindIntersectionsHPoly(WorldBsp *pWorldBsp, Node **pNodeIntersectionPtr,
	LTVector *pIntersectionPosPtr, float *pDistSqrPtr, HPOLY *hWorldPoly,
	LTVector *pPoint1, LTVector *pPoint2, uint8 bWorldModel)
{
	IntersectRequest req;

	req.m_pPoints[0] = pPoint1;
	req.m_pPoints[1] = pPoint2;
	req.m_pIPos      = pIntersectionPosPtr;
	req.m_pQuery     = g_pCurQuery;
	req.m_pWorldBsp  = pWorldBsp;

	if (IntersectLineNode(pWorldBsp->GetRootNode(), &req))
	{
		*hWorldPoly = pWorldBsp->MakeHPoly(req.m_pNodeHit);
		*pNodeIntersectionPtr = req.m_pNodeHit;
		*pDistSqrPtr = g_pCurQuery->m_From.DistSqr(*req.m_pIPos);
	}
	else
	{
		*hWorldPoly = INVALID_HPOLY;
		*pNodeIntersectionPtr = LTNULL;
	}
}


// 0x10031dcb
static void i_FindIntersections(WorldBsp *pWorldBsp, Node **pNodeIntersectionPtr,
	LTVector *pIntersectionPosPtr, float *pDistSqrPtr, HPOLY *hWorldPoly,
	LTVector *pPoint1, LTVector *pPoint2, uint8 bWorldModel)
{
	LTPlane iPlane;

	*hWorldPoly = INVALID_HPOLY;
	*pNodeIntersectionPtr = IntersectLine(pWorldBsp->GetRootNode(), pPoint1, pPoint2, pIntersectionPosPtr, &iPlane);
	if (*pNodeIntersectionPtr)
	{
		*pDistSqrPtr = pPoint1->DistSqr(*pIntersectionPosPtr);
	}
}


inline bool i_HandlePossibleIntersection(const LTVector& Point1, const LTVector& Point2, LTObject *pServerObj);

// 0x10031e17 -- called by the WorldTree in IntersectSegment.
static LTBOOL i_ISCallback(LTObject *pObj, void *pCBUser)
{
	LTObject *pObject = pObj;

	if (pObject->m_Flags & FLAG_SOLID_RAYHIT || g_bProcessNonSolid)
	{
		// Honor the INTERSECT_OBJECTS flag (or lack thereof...)
		if (!g_bProcessObjects && !pObject->IsMainWorldModel())
		{
			return LTFALSE;
		}

		return i_HandlePossibleIntersection(g_pCurQuery->m_From, g_pCurQuery->m_To, pObject);
	}

	return LTFALSE;
}


bool i_BoundingBoxTest(const LTVector& Point1, const LTVector& Point2, LTObject *pServerObj,
	LTVector *pIntersectPt, LTPlane *pIntersectPlane);
inline bool i_QuickSphereTest(const LTVector& Point1, const LTVector& Point2, LTObject *pServerObj);
static bool i_TestWorldModel(WorldModelInstance *pObj);

// 0x10031e7f -- tries everything it can think of to reject this object
// intersection.  If it does intersect and is closer than the current best
// world intersection then it replaces the current one.
inline bool i_HandlePossibleIntersection(const LTVector& Point1, const LTVector& Point2, LTObject *pServerObj)
{
	// Quick sphere test.
	if (i_QuickSphereTest(Point1, Point2, pServerObj))
	{
		// Ok, filter if necessary.
		if (g_pCurQuery->m_FilterFn &&
			!g_pCurQuery->m_FilterFn((HOBJECT)pServerObj, g_pCurQuery->m_pUserData))
		{
			// They said to ignore it..
		}
		else
		{
			// If it's a WorldModel, test its geometry: a ray could go right
			// through all of it.
			if (HasWorldModel(pServerObj))
			{
				return i_TestWorldModel((WorldModelInstance*)pServerObj);
			}
			else
			{
				// Bounding box test...
				LTVector testPt;
				float distToIntersectionSqr;
				LTPlane testPlane;
				if (i_BoundingBoxTest(Point1, Point2, pServerObj, &testPt, &testPlane))
				{
					// Is this intersection closer than the current best?
					distToIntersectionSqr = testPt.DistSqr(g_pCurQuery->m_From);

					if (g_pIntersection)
					{
						if (distToIntersectionSqr < g_IntersectionBestDistSqr)
						{
							USE_THIS_OBJECT(pServerObj, distToIntersectionSqr, testPlane, testPt, INVALID_HPOLY);
							return true;
						}
					}
					else
					{
						USE_THIS_OBJECT(pServerObj, distToIntersectionSqr, testPlane, testPt, INVALID_HPOLY);
						return true;
					}
				}
			}
		}
	}

	return false;
}


// 0x10031fc5
// RETAIL BUG: every face's plane distance is taken from the box's minimum
// corner, so the planes of the +X, +Y and +Z faces are wrong (NOLF2 still
// has it).
bool i_BoundingBoxTest(const LTVector& Point1, const LTVector& Point2, LTObject *pServerObj,
	LTVector *pIntersectPt, LTPlane *pIntersectPlane)
{
	float t;
	float testCoords[2];

	// Left/Right.
	if (Point1.x < pServerObj->m_MinBox.x)
	{
		if (Point2.x < pServerObj->m_MinBox.x)
		{
			return false;
		}

		DO_PLANE_TEST_X(pServerObj->m_MinBox.x, x, y, z, -1.0f);
	}
	else if (Point1.x > pServerObj->m_MaxBox.x)
	{
		if (Point2.x > pServerObj->m_MaxBox.x)
		{
			return false;
		}

		DO_PLANE_TEST_X(pServerObj->m_MaxBox.x, x, y, z, 1.0f);
	}

	// Top/Bottom.
	if (Point1.y < pServerObj->m_MinBox.y)
	{
		if (Point2.y < pServerObj->m_MinBox.y)
		{
			return false;
		}

		DO_PLANE_TEST_Y(pServerObj->m_MinBox.y, y, x, z, -1.0f);
	}
	else if (Point1.y > pServerObj->m_MaxBox.y)
	{
		if (Point2.y > pServerObj->m_MaxBox.y)
		{
			return false;
		}

		DO_PLANE_TEST_Y(pServerObj->m_MaxBox.y, y, x, z, 1.0f);
	}

	// Front/Back.
	if (Point1.z < pServerObj->m_MinBox.z)
	{
		if (Point2.z < pServerObj->m_MinBox.z)
		{
			return false;
		}

		DO_PLANE_TEST_Z(pServerObj->m_MinBox.z, z, x, y, -1.0f);
	}
	else if (Point1.z > pServerObj->m_MaxBox.z)
	{
		if (Point2.z > pServerObj->m_MaxBox.z)
		{
			return false;
		}

		DO_PLANE_TEST_Z(pServerObj->m_MaxBox.z, z, x, y, 1.0f);
	}

	return false;
}


// 0x1003269d
inline bool i_QuickSphereTest(const LTVector& Point1, const LTVector& Point2, LTObject *pServerObj)
{
	// Find the closest point to the line.
	// Here's the equation for t:
	// P = point1
	// V = point2 - point1 (ie: direction vector)
	// S = point you're testing
	// t = parametric (P + Vt)
	// t = -(-VS + VP) / VV
	float t = g_VTimesInvVV.Dot(pServerObj->GetPos()) - g_VPTimesInvVV;

	if (t < -pServerObj->GetRadius() || t > (g_LineLen + pServerObj->GetRadius()))
	{
		return false;
	}

	// Now see if it's within range.
	LTVector vecTo;
	vecTo = g_pCurQuery->m_From + g_V * t - pServerObj->GetPos();
	return vecTo.MagSqr() < pServerObj->GetRadiusSquared();
}


// 0x10032822 -- returns true if the segment hits the world model.
static bool i_TestWorldModel(WorldModelInstance *pObj)
{
	Node *pNodeIntersection;
	LTVector intersectionPt;
	LTFLOAT distToIntersectionSqr;
	HPOLY hWorldPoly;
	LTVector points[2], planePt;
	LTPlane tempPlane;


	// Pre-rotate the endpoints for the worldmodel.
	MatVMul_H(&points[0], &pObj->m_BackTransform, &g_pCurQuery->m_From);
	MatVMul_H(&points[1], &pObj->m_BackTransform, &g_pCurQuery->m_To);

	g_FindIntersectionsFn(pObj->m_pOriginalBsp,
		&pNodeIntersection, &intersectionPt, &distToIntersectionSqr, &hWorldPoly,
		&points[0], &points[1], true);

	if (!pNodeIntersection)
	{
		return false;
	}

	MatVMul_InPlace_H(&pObj->m_Transform, &intersectionPt);
	distToIntersectionSqr = g_pCurQuery->m_From.DistSqr(intersectionPt);

	if (distToIntersectionSqr < g_IntersectionBestDistSqr)
	{
		planePt = pNodeIntersection->GetPlane()->m_Normal * pNodeIntersection->GetPlane()->m_Dist;
		MatVMul_3x3(&tempPlane.m_Normal, &pObj->m_Transform, &pNodeIntersection->GetPlane()->m_Normal);
		tempPlane.m_Dist = tempPlane.m_Normal.Dot(planePt);

		g_pWorldIntersection = pNodeIntersection;

		USE_THIS_OBJECT(pObj, distToIntersectionSqr, tempPlane, intersectionPt, hWorldPoly);
		return true;
	}

	return false;
}
