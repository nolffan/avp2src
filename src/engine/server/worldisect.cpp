// *********************************************************************** //
//
//  MODULE:   WORLDISECT (.CPP)  -- file name ours
//
//  PURPOSE:  The world's segment query, w_IntersectSegment, and its
//            per-polygon and per-object tests.
//
//  Primal Hunt (and AvP2 1.0.9.6) build this module UNOPTIMISED, in both
//  images: every function opens `push %ebp; mov %esp,%ebp`, keeps its
//  arguments and locals in the frame, and calls the SDK's vector inlines
//  out of line (server.dll 0x10012de7..0x10013ebf, lithtech.exe
//  0x0043b750..; AvP2 1.0.9.6 server.dll 0x100123f2, lithtech.exe
//  0x00437c10).  NOLF built the same source inside objbank.cpp at /O1 and
//  /O2 (server.dll 0x1000edd7), which is where it was reconstructed from and
//  what most of the notes below were measured against; the code moved here
//  unchanged except where a Primal Hunt address is cited. Bare addresses in
//  the older notes (0x1000edd7..0x1000f74e, the globals at 0x10058540..)
//  are NOLF server.dll's.
//
//  The module starts with three global constructors (the three LTVectors
//  below, server.dll 0x10012de7 / 0x10012df6 / 0x10012e05, each an `_$E`
//  pair whose body is empty) and, in Primal Hunt only, ends with
//  std::ctype<unsigned short>::id's guarded initializer from <string>
//  (0x10013e8b / 0x10013eaf).
//
// *********************************************************************** //

#include <new.h>
#include <math.h>
#include "servermgr.h"
#include "objtypes.h"
#include "commonmodel.h"
#include "de_error.h"
#include <string.h>

// PH: <string>'s std::ctype<unsigned short>::id initializer closes the module
// (server.dll 0x10013e8b: guard 0x1008ec84 bit 0, atexit of 0x1002eaeb).
#ifdef PRIMAL_HUNT
#include <string>
#endif


// *********************************************************************** //
//  0x1000edd7 — the world's segment query (servermgr.h names it
//  w_IntersectSegment). It is here rather than in a world file because this
//  file already has servermgr.h; nothing about it belongs to the banks.
//
//  Every intermediate lives in a run of file-scope globals at 0x10058540 and
//  up rather than in the frame — the same style as worldsim.cpp's box
//  machinery — so the walk's two per-poly tests and its per-object callback
//  can read them without being handed anything. All the names below are
//  ours, from what each is used for; only their sizes and types are read off
//  the bytes.
// *********************************************************************** //

// They are DEFINED here, this file being the one that uses them, and with no
// initialiser: the whole run lies in .data's uninitialised tail (the section's
// raw data stops at 0x10057000), so every one of them starts at zero.
//
// 0x10058540 — the segment's length, and 0x10058544 the projection of the
// start onto the (normalised) direction divided by its squared length.
float g_fIsectLength;
float g_fIsectStartFrac;
// 0x10058548 — the direction scaled by the reciprocal of its squared length,
// and 0x10058558 the normalised direction itself.
LTVector g_vIsectScaledDir;
LTVector g_vIsectDir;
// 0x10058564 — the polygon handle the walk found, seeded to -1 (no poly).
int g_hIsectPoly;
// 0x10058568 and 0x10058578 — where and on what the segment stopped.
LTVector g_vIsectPoint;
LTPlane  g_IsectPlane;
// 0x10058588 — the squared length plus one, which is what the per-poly tests
// reject against.
float g_fIsectRangeSqr;
// 0x1005858c and 0x10058590 — what was hit: the object always, and the
// world-polygon record only when it was world geometry.
class LTObject* g_pIsectObject;
class CIsectPolyRef* g_pIsectPolyRef;
// 0x10058594 and 0x10058595 — the two INTERSECT_ bits, unpacked to bytes so
// the walk can test them without the mask.
uint8 g_bIsectFlag0;
uint8 g_bIsectNotFlag1;
// 0x10058598 — the query itself, for everything the walk needs that is not
// unpacked above.
IntersectQuery* g_pIsectQuery;

void (*g_pIsectPolyTest)();                  // 0x1005859c

// 0x100585a0 — a float in this run that only smgr.cpp's Update touches: it
// zeroes it each frame beside the two counters below (0x10032ebb, the one
// reference in the image). servermgr.h names it (name ours).
float g_Unknown585a0;

// 0x100585a4 and 0x100585a8 — how many segment queries have run and how long
// they have taken.
// smgr.cpp's Update zeroes the same two each frame (servermgr.h externs).
uint32 g_nIsectQueries;
uint32 g_nIsectTime;

// 0x1000f00c and 0x1000f0a3 — the two per-polygon tests, chosen by
// INTERSECT_ bit 2; 0x1005859c is the slot the walk calls through, and
// 0x1000f0e8 is the per-object callback the container walk is handed
// (names ours).
//
// Both take the same eight arguments, and the eighth is read by neither.
// Both are defined below.
void w_IsectPolyTestFull(class IWorldBsp* pWorldBsp,
                         class CIsectPolyRef** ppPolyRef, LTVector* pPoint,
                         float* pDist, int* pHPoly, LTVector* pFrom,
                         LTVector* pTo, int nUnused);   // 0x1000f00c
void w_IsectPolyTestCheap(class IWorldBsp* pWorldBsp,
                          class CIsectPolyRef** ppPolyRef, LTVector* pPoint,
                          float* pDist, int* pHPoly, LTVector* pFrom,
                          LTVector* pTo, int nUnused);  // 0x1000f0a3

// The three halves of the per-object test that are not reconstructed. All
// three are cdecl and all three answer in `al`, so their return type is a
// byte rather than LTBOOL (names ours, from the call sites):
//
// 0x1000f650 — the cheap reject: does the segment come near this object's
// bounding sphere at all (name ours). Defined below, and `inline` — see the
// note on the definition.
uint8 w_IsectObjectBounds(LTVector* pFrom, LTVector* pTo,
                          class LTObject* pObj);
// 0x1000f74e — the whole test for an object that carries its own geometry,
// which is why it takes nothing but the object: everything else it needs is
// in the query globals above, and it records its own hit. Defined below
// (name ours).
uint8 w_IsectObjectGeometry(class LTObject* pObj);
// 0x1000f1f7 — the test for everything else, which reports where and on what
// rather than recording it. Defined below (name ours).
uint8 w_IsectObjectShape(LTVector* pFrom, LTVector* pTo, class LTObject* pObj,
                         LTVector* pPoint, LTPlane* pPlane);
// The per-object callback the container walk is handed, and the test it
// runs (server.dll 0x10013206 and 0x1001326e; defined below).
LTBOOL w_IsectObjectCB(HOBJECT hObj);
uint8  w_IsectObject(LTVector* pFrom, LTVector* pTo, class LTObject* pObj);

// The LTObject virtual at slot 0x34 answers whether the object takes part in
// the test at all, and ltobject_internal.h declares that slot `void` (see the
// same view in servercommon.cpp). Redeclared here on a layout-identical view
// whose only purpose is to give the slot a return value; the view's vptr is
// the object's own, at offset 0, which is where the shipped `mov (%esi),%eax`
// reads it (names ours).
class IObjIsectTestView
{
public:
	virtual void _p00()=0; virtual void _p04()=0; virtual void _p08()=0;
	virtual void _p0c()=0; virtual void _p10()=0; virtual void _p14()=0;
	virtual void _p18()=0; virtual void _p1c()=0; virtual void _p20()=0;
	virtual void _p24()=0; virtual void _p28()=0; virtual void _p2c()=0;
	virtual void _p30()=0;
	virtual LTBOOL TakesPartInIsect()=0;     // 0x34
};

// The polygon record the BSP walk hands back. AVP2: its polygon is
// ltobject_internal.h's WorldPolyOwner -- the plane is the polygon's own at
// +0x28 behind the surface test at +0x2c, and the flags the query reports
// are the surface's word at +0x34 (server.dll 0x100130dd/0x100130e0, AvP2
// 1.0.9.6 0x100126ec/0x100126ef); NOLF reached a plane pointer at
// surface+0x34 and the flags at +0x3c.
class CIsectPolyRef
{
public:
	// The plane through the reference: null without a polygon or a surface
	// (server.dll 0x10013cf9..0x10013d22, once per use). The plane passes
	// through a local of its own on the way out (0x10013d14 then
	// 0x10013d1a), which a direct return does not give.
	LTPlane* GetPlane()
	{
		if (m_pPoly && m_pPoly->m_pSurface)
		{
			LTPlane* pPlane = m_pPoly->m_pPlane;
			return pPlane;
		}

		return LTNULL;
	}

	WorldPolyOwner* m_pPoly;                 // 0x00
};

// The container at CServerMgr+0x210 once more (servercommon.h declares its
// slot 3, the point query); this is slot 4, the segment walk (name ours).
class ISegmentQueryWorld
{
public:
	virtual void _v00() = 0;
	virtual void _v04() = 0;
	virtual void _v08() = 0;
	virtual void _v0c() = 0;
	virtual void FindObjectsOnSegment(LTVector* pFrom, LTVector* pTo,
	                                  void (*pfn)(), int a, int b) = 0;   // 0x10
};

// worldsim.h's CLineQuery, declared here rather than included so this file
// stays free of the world headers (packet.cpp keeps its own view of the same
// class for the same reason). The two dwords worldsim.h leaves unnamed are
// what the walk's polygon filter needs: +0x0c is the SDK's IntersectQuery,
// whose m_PolyFilterFn and m_pUserData it reaches, and +0x10 is the world
// whose MakeHPoly turns the node it stopped on into the handle that filter
// takes. Both are filled in here and nowhere else, which is what makes this
// the FULL test — the cheap one at 0x1000f0a3 goes through
// bsp_IntersectLine, which constructs its query without them and so accepts
// every polygon.
//
// The constructor (0x1000f08d) zeroes all six fields, in the order below.
// It is `inline`: /O1 declines to expand it and emits the out-of-line
// COMDAT server.dll calls, while /O2 expands it in lithtech.exe's copy
// (0x004308f0), where the only store that survives is m_pPoly's
// `movl $0x0` -- the call there was a 214-vs-213-byte near.
class CLineQuery
{
public:
	CLineQuery();                            // 0x1000f08d

	LTVector*       m_pStart;                // 0x00
	LTVector*       m_pEnd;                  // 0x04
	LTVector*       m_pHit;                  // 0x08
	IntersectQuery* m_pQuery;                // 0x0c
	IWorldBsp*      m_pWorldBsp;             // 0x10
	WorldPoly*      m_pPoly;                 // 0x14  what the walk found
};

inline CLineQuery::CLineQuery()
{
	m_pStart    = LTNULL;
	m_pEnd      = LTNULL;
	m_pHit      = LTNULL;
	m_pPoly     = LTNULL;
	m_pQuery    = LTNULL;
	m_pWorldBsp = LTNULL;
}

// 0x10013f28 — the walk's entry point (worldsim.h's bspq_IntersectLine).
int bspq_IntersectLine(class BSPNode* pRoot, CLineQuery* pQuery);

// 0x10013f53 — worldsim.h's line-versus-BSP query: the first polygon the
// segment crosses, where it crossed it and the plane it crossed.
WorldPoly* bsp_IntersectLine(class BSPNode* pRoot, LTVector* pStart,
                             LTVector* pEnd, LTVector* pHit,
                             LTPlane* pPlane);



// ======================================================================= //
//  The functions, in the shipped order (server.dll 0x10012e14 on).
// ======================================================================= //

// server.dll 0x10012e14 -- the segment query. The last argument is accepted
// and never read.
//
// The scoped timer is the SERVER's only: NOLF's lithtech.exe copy
// (0x004305f0) has no CBaseCounter constructor, cnt_StartCounter or
// cnt_EndCounter, and is guarded with LT_SERVER_BUILD, not LT_SERVER_TIMERS,
// which AvP2's client defines too. The query counter goes up FIRST and the
// counter is constructed after it (0x10012e1f, then 0x10012e2f).
LTBOOL w_IntersectSegment(IntersectQuery* pQuery, IntersectInfo* pInfo,
                          CUnknown210* pObjects, int nFlags)
{
	// Unoptimised code lays the frame out by the locals' NAMES, so these four
	// are named to reproduce it (isectCounter at -0x8, then fInvSqr,
	// fStartDot, fMagSqr down to -0x14); the shipped names are not
	// recoverable.
	float fMagSqr;
	float fStartDot;
	float fInvSqr;

	g_nIsectQueries++;

#ifdef LT_SERVER_BUILD
	CBaseCounter isectCounter(0);

	cnt_StartCounter(&isectCounter);
#endif

	g_pIsectQuery    = pQuery;
	g_pIsectObject   = LTNULL;
	g_pIsectPolyRef  = LTNULL;
	g_hIsectPoly     = -1;

	// `and $2; neg; sbb; inc` and `and $1; neg; sbb; neg` (0x10012e61,
	// 0x10012e74): two logical tests of the flags, not shifts.
	g_bIsectNotFlag1 = !(pQuery->m_Flags & 2);
	g_bIsectFlag0    = (pQuery->m_Flags & 1) != 0;

	g_fIsectRangeSqr = (pQuery->m_From - pQuery->m_To).MagSqr() + 1.0f;

	g_vIsectDir = pQuery->m_To - pQuery->m_From;

	g_fIsectLength = g_vIsectDir.Mag();
	g_vIsectDir /= g_fIsectLength;

	// AVP2: the running total of every query's length, the float smgr.cpp's
	// Update zeroes each frame (0x10012f63).
	g_Unknown585a0 += g_fIsectLength;

	fMagSqr = g_vIsectDir.MagSqr();

	if (fMagSqr < 0.5f || fMagSqr > 2.0f)
		return LTFALSE;

	fStartDot = g_vIsectDir.Dot(pQuery->m_From);

	fInvSqr = 1.0f / g_vIsectDir.MagSqr();

	g_vIsectScaledDir = g_vIsectDir * fInvSqr;
	g_fIsectStartFrac = fStartDot * fInvSqr;

	if (pQuery->m_Flags & 4)
		g_pIsectPolyTest = (void (*)())w_IsectPolyTestFull;
	else
		g_pIsectPolyTest = (void (*)())w_IsectPolyTestCheap;

	((ISegmentQueryWorld*)pObjects)->FindObjectsOnSegment(&pQuery->m_From,
	                                                      &pQuery->m_To,
	                                                      (void (*)())
	                                                      w_IsectObjectCB,
	                                                      0, 0);

#ifdef LT_SERVER_BUILD
	g_nIsectTime += cnt_EndCounter(&isectCounter);
#endif

	// A positive guard: the FALSE return is the last statement (0x100130f8).
	if (g_pIsectObject)
	{
		pInfo->m_Point   = g_vIsectPoint;
		pInfo->m_Plane   = g_IsectPlane;
		pInfo->m_hObject = (HOBJECT)g_pIsectObject;
		pInfo->m_hPoly   = g_hIsectPoly;

		if (g_pIsectPolyRef)
			pInfo->m_SurfaceFlags = g_pIsectPolyRef->m_pPoly->m_pSurface->m_nTextureFlags;
		else
			pInfo->m_SurfaceFlags = 0;

		return LTTRUE;
	}

	return LTFALSE;
}


// 0x1000f00c — the per-polygon test the segment walk calls through when
// INTERSECT_ bit 2 is set. Everything the caller wants comes back through
// pointers, so the function itself answers nothing.
//
// The distance is read back out of the query's own m_pHit rather than out of
// pPoint: the two hold the same pointer, but VC6 does not CSE across the
// virtual call above it, and the shipped code pushes the frame slot.
void w_IsectPolyTestFull(IWorldBsp* pWorldBsp, CIsectPolyRef** ppPolyRef,
                         LTVector* pPoint, float* pDist, int* pHPoly,
                         LTVector* pFrom, LTVector* pTo, int nUnused)
{
	CLineQuery Query;

	Query.m_pStart    = pFrom;
	Query.m_pEnd      = pTo;
	Query.m_pHit      = pPoint;
	Query.m_pQuery    = g_pIsectQuery;
	Query.m_pWorldBsp = pWorldBsp;

	if (bspq_IntersectLine(pWorldBsp->GetRoot(), &Query))
	{
		*pHPoly    = pWorldBsp->MakeHPoly(Query.m_pPoly);
		*ppPolyRef = (CIsectPolyRef*)Query.m_pPoly;
		*pDist     = g_pIsectQuery->m_From.DistSqr(*Query.m_pHit);
	}
	else
	{
		*pHPoly    = -1;
		*ppPolyRef = LTNULL;
	}
}

// 0x1000f0a3 — the per-polygon test the segment walk calls through when
// INTERSECT_ bit 2 is clear: the plain world query, which has no polygon
// filter to consult and hands back no handle, so the handle is always -1.
// The plane it reports is not wanted (the frame is exactly one LTPlane), and
// the distance is measured from the segment's own start rather than from the
// query's.
void w_IsectPolyTestCheap(IWorldBsp* pWorldBsp, CIsectPolyRef** ppPolyRef,
                          LTVector* pPoint, float* pDist, int* pHPoly,
                          LTVector* pFrom, LTVector* pTo, int nUnused)
{
	LTPlane Plane;

	*pHPoly = -1;

	*ppPolyRef = (CIsectPolyRef*)bsp_IntersectLine(pWorldBsp->GetRoot(),
	                                               pFrom, pTo, pPoint, &Plane);
	if (*ppPolyRef)
		*pDist = pFrom->DistSqr(*pPoint);
}

// 0x1000f0e8 — the per-object callback the container walk is handed, and the
// only thing in front of the test above (name ours).
//
// Two gates. An object that is neither ray-hittable nor solid is skipped
// unless the query's second bit says to consider it anyway; and with
// INTERSECT_OBJECTS clear the object is asked through its own slot 0x34
// whether it wants to be tested at all.
// server.dll 0x10013206: the handle is copied into a local before anything
// reads it (0x1001320a), and the test that lets an object through is the
// positive one, with the shared FALSE last (0x1001326a).
LTBOOL w_IsectObjectCB(HOBJECT hObj)
{
	LTObject* pObj = hObj;

	if ((pObj->m_Flags & (FLAG_RAYHIT | FLAG_SOLID)) || g_bIsectNotFlag1)
	{
		if (!g_bIsectFlag0)
		{
			if (!((IObjIsectTestView*)pObj)->TakesPartInIsect())
				return LTFALSE;
		}

		return w_IsectObject(&g_pIsectQuery->m_From,
		                     &g_pIsectQuery->m_To, pObj);
	}

	return LTFALSE;
}

// server.dll 0x1001326e — one object against the segment, and the closest
// hit so far kept in the query globals (name ours).
//
// The object type chooses the path: a WorldModel or a Container carries its
// own geometry and records its own hit, so the answer is whatever that test
// says. Everything else reports a point and a plane back here, and this is
// what decides whether they are nearer than what is already recorded.
//
// The type test is an inline predicate: its `||` lands in a frame temporary
// that is then tested (0x100132d9..0x100132e6). The record-the-hit block is
// written twice, the "nearer" arm first (0x1001332f).
inline LTBOOL w_IsGeometryObjectQ(LTObject* pObj)
{
	return pObj->m_ObjectType == OT_WORLDMODEL ||
	       pObj->m_ObjectType == OT_CONTAINER;
}

uint8 w_IsectObject(LTVector* pFrom, LTVector* pTo, LTObject* pObj)
{
	LTVector vPoint;
	LTPlane  Plane;
	float    fDistSqr;

	if (w_IsectObjectBounds(pFrom, pTo, pObj))
	{
		// A refusal by the filter skips the rest rather than returning: the
		// `jne; jmp` at 0x100132b4 goes to the shared FALSE (0x100133ae).
		if (g_pIsectQuery->m_FilterFn &&
		    !g_pIsectQuery->m_FilterFn((HOBJECT)pObj, g_pIsectQuery->m_pUserData))
		{
		}
		else
		{
			if (w_IsGeometryObjectQ(pObj))
				return w_IsectObjectGeometry(pObj);

			if (w_IsectObjectShape(pFrom, pTo, pObj, &vPoint, &Plane))
			{
				fDistSqr = vPoint.DistSqr(g_pIsectQuery->m_From);

				if (g_pIsectObject)
				{
					if (fDistSqr < g_fIsectRangeSqr)
					{
						g_fIsectRangeSqr = fDistSqr;
						g_pIsectObject   = pObj;
						g_IsectPlane     = Plane;
						g_vIsectPoint    = vPoint;
						g_hIsectPoly     = -1;

						return 1;
					}
				}
				else
				{
					g_fIsectRangeSqr = fDistSqr;
					g_pIsectObject   = pObj;
					g_IsectPlane     = Plane;
					g_vIsectPoint    = vPoint;
					g_hIsectPoly     = -1;

					return 1;
				}
			}
		}
	}

	return 0;
}

// 0x1000f1f7 — the segment against the object's world-space bounding box,
// reporting where it crossed and on which face. This is what every object
// that does not carry its own geometry gets (name ours).
//
// Six faces written out, not a loop over three axes: each one tests the near
// side, solves for the crossing parameter, and rejects unless BOTH other
// components land inside their slab. An inner rejection falls through to the
// next axis rather than answering, which is why the six blocks are three
// if/else pairs in a row and not a chain.
//
// The three POSITIVE faces report the wrong plane distance -- the box's
// minimum on that axis where the point they just reported is the maximum.
// Written here as the bytes have it.
uint8 w_IsectObjectShape(LTVector* pFrom, LTVector* pTo, LTObject* pObj,
                         LTVector* pPoint, LTPlane* pPlane)
{
	// The two components that are not the face's own axis, in the order the
	// tests take them. An array rather than a pair of named floats, and that
	// is the frame talking: the shipped build reserves eight bytes of locals
	// and writes only the first, while a pair of floats lets VC6 park the one
	// that needs memory in the dead pObj parameter slot and reserve nothing.
	float fCross[2];
	float t;

	if (pFrom->x < pObj->m_BBoxMin.x)
	{
		if (pTo->x < pObj->m_BBoxMin.x)
			return 0;

		t = (pObj->m_BBoxMin.x - pFrom->x) / (pTo->x - pFrom->x);

		fCross[0] = (pTo->y - pFrom->y) * t + pFrom->y;

		if (fCross[0] > pObj->m_BBoxMin.y && fCross[0] < pObj->m_BBoxMax.y)
		{
			fCross[1] = (pTo->z - pFrom->z) * t + pFrom->z;

			if (fCross[1] > pObj->m_BBoxMin.z && fCross[1] < pObj->m_BBoxMax.z)
			{
				pPoint->x = pObj->m_BBoxMin.x;
				pPoint->y = fCross[0];
				pPoint->z = fCross[1];

				pPlane->m_Normal.x = -1.0f;
				pPlane->m_Normal.y = 0.0f;
				pPlane->m_Normal.z = 0.0f;
				pPlane->m_Dist = pObj->m_BBoxMin.x * -1.0f;

				return 1;
			}
		}
	}
	else if (pFrom->x > pObj->m_BBoxMax.x)
	{
		if (pTo->x > pObj->m_BBoxMax.x)
			return 0;

		t = (pObj->m_BBoxMax.x - pFrom->x) / (pTo->x - pFrom->x);

		fCross[0] = (pTo->y - pFrom->y) * t + pFrom->y;

		if (fCross[0] > pObj->m_BBoxMin.y && fCross[0] < pObj->m_BBoxMax.y)
		{
			fCross[1] = (pTo->z - pFrom->z) * t + pFrom->z;

			if (fCross[1] > pObj->m_BBoxMin.z && fCross[1] < pObj->m_BBoxMax.z)
			{
				pPoint->x = pObj->m_BBoxMax.x;
				pPoint->y = fCross[0];
				pPoint->z = fCross[1];

				pPlane->m_Normal.x = 1.0f;
				pPlane->m_Normal.y = 0.0f;
				pPlane->m_Normal.z = 0.0f;
#ifdef AVP2_MODERN
				// fixed: the box-intersect test reports the wrong plane on three
				// of six faces (RETAIL_BUGS.md). The positive face is at the maximum.
				pPlane->m_Dist = pObj->m_BBoxMax.x * 1.0f;
#else
				pPlane->m_Dist = pObj->m_BBoxMin.x * 1.0f;
#endif

				return 1;
			}
		}
	}

	if (pFrom->y < pObj->m_BBoxMin.y)
	{
		if (pTo->y < pObj->m_BBoxMin.y)
			return 0;

		t = (pObj->m_BBoxMin.y - pFrom->y) / (pTo->y - pFrom->y);

		fCross[0] = (pTo->x - pFrom->x) * t + pFrom->x;

		if (fCross[0] > pObj->m_BBoxMin.x && fCross[0] < pObj->m_BBoxMax.x)
		{
			fCross[1] = (pTo->z - pFrom->z) * t + pFrom->z;

			if (fCross[1] > pObj->m_BBoxMin.z && fCross[1] < pObj->m_BBoxMax.z)
			{
				pPoint->y = pObj->m_BBoxMin.y;
				pPoint->x = fCross[0];
				pPoint->z = fCross[1];

				pPlane->m_Normal.x = 0.0f;
				pPlane->m_Normal.y = -1.0f;
				pPlane->m_Normal.z = 0.0f;
				pPlane->m_Dist = pObj->m_BBoxMin.y * -1.0f;

				return 1;
			}
		}
	}
	else if (pFrom->y > pObj->m_BBoxMax.y)
	{
		if (pTo->y > pObj->m_BBoxMax.y)
			return 0;

		t = (pObj->m_BBoxMax.y - pFrom->y) / (pTo->y - pFrom->y);

		fCross[0] = (pTo->x - pFrom->x) * t + pFrom->x;

		if (fCross[0] > pObj->m_BBoxMin.x && fCross[0] < pObj->m_BBoxMax.x)
		{
			fCross[1] = (pTo->z - pFrom->z) * t + pFrom->z;

			if (fCross[1] > pObj->m_BBoxMin.z && fCross[1] < pObj->m_BBoxMax.z)
			{
				pPoint->y = pObj->m_BBoxMax.y;
				pPoint->x = fCross[0];
				pPoint->z = fCross[1];

				pPlane->m_Normal.x = 0.0f;
				pPlane->m_Normal.y = 1.0f;
				pPlane->m_Normal.z = 0.0f;
#ifdef AVP2_MODERN
				pPlane->m_Dist = pObj->m_BBoxMax.y * 1.0f;
#else
				pPlane->m_Dist = pObj->m_BBoxMin.y * 1.0f;
#endif

				return 1;
			}
		}
	}

	if (pFrom->z < pObj->m_BBoxMin.z)
	{
		if (pTo->z < pObj->m_BBoxMin.z)
			return 0;

		t = (pObj->m_BBoxMin.z - pFrom->z) / (pTo->z - pFrom->z);

		fCross[0] = (pTo->x - pFrom->x) * t + pFrom->x;

		if (fCross[0] > pObj->m_BBoxMin.x && fCross[0] < pObj->m_BBoxMax.x)
		{
			fCross[1] = (pTo->y - pFrom->y) * t + pFrom->y;

			if (fCross[1] > pObj->m_BBoxMin.y && fCross[1] < pObj->m_BBoxMax.y)
			{
				pPoint->z = pObj->m_BBoxMin.z;
				pPoint->x = fCross[0];
				pPoint->y = fCross[1];

				pPlane->m_Normal.x = 0.0f;
				pPlane->m_Normal.y = 0.0f;
				pPlane->m_Normal.z = -1.0f;
				pPlane->m_Dist = pObj->m_BBoxMin.z * -1.0f;

				return 1;
			}
		}
	}
	else if (pFrom->z > pObj->m_BBoxMax.z)
	{
		if (pTo->z > pObj->m_BBoxMax.z)
			return 0;

		t = (pObj->m_BBoxMax.z - pFrom->z) / (pTo->z - pFrom->z);

		fCross[0] = (pTo->x - pFrom->x) * t + pFrom->x;

		if (fCross[0] > pObj->m_BBoxMin.x && fCross[0] < pObj->m_BBoxMax.x)
		{
			fCross[1] = (pTo->y - pFrom->y) * t + pFrom->y;

			if (fCross[1] > pObj->m_BBoxMin.y && fCross[1] < pObj->m_BBoxMax.y)
			{
				pPoint->z = pObj->m_BBoxMax.z;
				pPoint->x = fCross[0];
				pPoint->y = fCross[1];

				pPlane->m_Normal.x = 0.0f;
				pPlane->m_Normal.y = 0.0f;
				pPlane->m_Normal.z = 1.0f;
#ifdef AVP2_MODERN
				pPlane->m_Dist = pObj->m_BBoxMax.z * 1.0f;
#else
				pPlane->m_Dist = pObj->m_BBoxMin.z * 1.0f;
#endif

				return 1;
			}
		}
	}

	return 0;
}

// server.dll 0x10013a8c (NOLF 0x1000f650) — the object's bounding sphere
// against the segment, and the first thing every object gets. Neither of the
// two vector arguments is read: the segment is entirely in the query globals
// by the time this runs, and the pair survives because the test below it
// takes them. The object's radius is read through an accessor (name ours).
inline float obj_GetRadius(LTObject* pObj) { return pObj->m_Radius; }

uint8 w_IsectObjectBounds(LTVector* pFrom, LTVector* pTo, LTObject* pObj)
{
	LTVector vOn;
	float    t;

	t = g_vIsectScaledDir.Dot(pObj->m_Pos) - g_fIsectStartFrac;

	// Both bounds read the radius through the accessor: each lands in a
	// temporary first (0x10013ad7, 0x10013aed), the global on the left of
	// the upper one's sum.
	if (t < -obj_GetRadius(pObj) || t > g_fIsectLength + obj_GetRadius(pObj))
		return 0;

	// One expression: the subtraction reads the sum's temporary, not vOn
	// (0x10013b90), and the difference is what is assigned (0x10013bc6).
	vOn = g_pIsectQuery->m_From + g_vIsectDir * t - pObj->m_Pos;

	return pObj->m_Radius * pObj->m_Radius > vOn.MagSqr();
}


// server.dll 0x10013c11 (NOLF 0x1000f74e) — a WorldModel or a Container
// against the segment. These carry their own BSP, so the segment is taken
// into the object's space, walked there, and the hit taken back out (name
// ours).
//
// The plane transform is ltmatrix.h's LTMatrix::TransformPlane as the
// shipped build's copy of that header had it, three lines, not four: the
// SDK copy transforms the scaled start point (`MatVMul_InPlace_H(this,
// &startPt)`) before dotting it, and the shipped bytes have no such call.
// Spelled out here rather than calling the header. The two transformed
// endpoints are one two-element array (its constructor loop at 0x10013c1c).
uint8 w_IsectObjectGeometry(LTObject* pObj)
{
	// Unoptimised code lays the frame out by the locals' names; these give
	// the shipped slots (pWorldPoly -0x4, vStart -0x10, fDist -0x14,
	// vSegment -0x2c, plane -0x3c, hPoly -0x40, vHitPt -0x4c).
	float    fDist;
	int      hPoly;
	LTVector vStart;
	LTVector vHitPt;
	LTPlane  plane;
	LTVector vSegment[2];

	CIsectPolyRef* pWorldPoly;

	MatVMul_H(&vSegment[0], &((WorldModelObject*)pObj)->m_Transform230, &g_pIsectQuery->m_From);
	MatVMul_H(&vSegment[1], &((WorldModelObject*)pObj)->m_Transform230, &g_pIsectQuery->m_To);

	((void (*)(IWorldBsp*, CIsectPolyRef**, LTVector*, float*, int*,
	           LTVector*, LTVector*, int))g_pIsectPolyTest)
		(pObj->m_pWorldBsp, &pWorldPoly, &vHitPt, &fDist, &hPoly,
		 &vSegment[0], &vSegment[1], 1);

	if (!pWorldPoly)
		return 0;

	MatVMul_InPlace_H(&((WorldModelObject*)pObj)->m_Transform1f0, &vHitPt);

	fDist = g_pIsectQuery->m_From.DistSqr(vHitPt);

	if (fDist < g_fIsectRangeSqr)
	{
		vStart = pWorldPoly->GetPlane()->m_Normal * pWorldPoly->GetPlane()->m_Dist;

		MatVMul_3x3(&plane.m_Normal,
		            &((WorldModelObject*)pObj)->m_Transform1f0,
		            &pWorldPoly->GetPlane()->m_Normal);

#ifdef AVP2_MODERN
		// fixed: LTMatrix::TransformPlane never transforms the point it dots
		// (RETAIL_BUGS.md). The point goes through the same transform as the
		// normal, as the SDK's ltmatrix.h has it.
		MatVMul_InPlace_H(&((WorldModelObject*)pObj)->m_Transform1f0, &vStart);

#endif
		plane.m_Dist = plane.m_Normal.Dot(vStart);

		// The stores in this order (0x10013e49..0x10013e7c).
		g_pIsectPolyRef  = pWorldPoly;
		g_fIsectRangeSqr = fDist;
		g_pIsectObject   = pObj;
		g_IsectPlane     = plane;
		g_vIsectPoint    = vHitPt;
		g_hIsectPoly     = hPoly;

		return 1;
	}

	return 0;
}
