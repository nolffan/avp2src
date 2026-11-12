// server.dll — the physics module (reconstruction). Build /O1.
#include <math.h>
#include <float.h>
#include "worldsim.h"
#include "iltmath.h"

// PH: Primal Hunt's engine TUs see the C++ library's <string>, as ~95 of
// lithtech.exe's do (AvP2 1.0.9.6's none).  Its trace is
// std::ctype<unsigned short>::id's guarded initializer at the end of the TU
// (lithtech.exe 0x00421bb0: guard 0x004f8014 bit 0, atexit of the empty
// 0x00401790), and server.dll's copy of this TU ends in the same one
// (0x10007327, after its out-of-line _CVector members: guard 0x1008ec84).
#ifdef PRIMAL_HUNT
#include <string>
#endif

// 0x10056ff8 (.bss) -- bspq_TestSphere's node stack, declared in worldsim.h.
// 1000 entries: the next global the image references, g_nPhysMoves at
// 0x10057f98, is exactly 0xfa0 bytes on.
BSPNode* g_NodeStack[1000];

// A null polygon has no plane either (name ours).
inline LTPlane* ws_GetPlaneQ506(WorldPolyOwner* pOwner)
{
	return pOwner ? pOwner->GetPlane() : LTNULL;
}

// AVP2: w_IsPolyBlocking's two out-parameters, the clipped polygon's
// bounds, are filled only when both are given (AvP2 0x0041718c..0x00417287);
// bspq_TestSphere passes neither (lithtech 0x0041d5c0).

// Walking the tree for anything inside a sphere. The walk keeps its own stack
// rather than recursing: where the sphere straddles a plane, the near side is
// followed at once and the far side is pushed for later — and only if there
// is something below it, so a leaf is never pushed.
//
// A node whose geometry the sphere clears entirely on one side is followed
// down that side alone, which is the whole point of the tree.
//
// The if/else chain is load-bearing. All three arms end by descending the
// front side, and VC6 emits that step once: written as two guarded
// `continue`s with the descend last it hosts the surviving copy at the *end*
// and jumps forward to it, which is three bytes and seven blocks away from
// the shipped layout. Written as if/else, as here, the copy stays at the
// first arm and the other two jump backwards into it -- what shipped.
int bspq_TestSphere(BSPNode* pNode, CSphereQuery* pQuery)
{
	BSPNode** ppTop = g_NodeStack;
	LTVector  v;
	float     fDist;

	for (;;)
	{
		if (pNode->m_Flags & BSPNODE_LEAF)
		{
			if (ppTop == g_NodeStack)
				return 0;

			ppTop--;
			pNode = *ppTop;
		}

		{
			WorldPolyOwner* pOwner = pNode->m_pOwner;
			LTPlane*        pPlane = ws_GetPlaneQ506(pOwner);

			v = pQuery->m_vPoint;

			fDist = v.x * pPlane->m_Normal.x +
			        v.y * pPlane->m_Normal.y +
			        v.z * pPlane->m_Normal.z - pPlane->m_Dist;

			if (fDist > pQuery->m_fRadius)
			{
				pNode = pNode->m_pChild[1];
			}
			else if (fDist < -pQuery->m_fRadius)
			{
				pNode = pNode->m_pChild[0];
			}
			else
			{
				if (pOwner && w_IsPolyBlocking(pOwner, LTNULL, LTNULL))
					return 1;

				{
					BSPNode* pBack = pNode->m_pChild[0];

					if (!(pBack->m_Flags & BSPNODE_LEAF))
					{
						*ppTop = pBack;
						ppTop++;
					}
				}

				pNode = pNode->m_pChild[1];
			}
		}
	}
}

// Hitting a surface. An object already travelling away from the surface is
// not hitting it at all, and nothing is written; otherwise the impulse is the
// surface normal scaled by how fast the object was closing on it, the
// momentum that impulse against the object's mass, and the impulse is then
// nudged back along the normal so that a resting object does not settle
// inside what it is resting on.
//
// This sat as a four-byte near over the dot product's term order for two
// sessions. What closed it is written up at the expression itself.
//
// `inline` is not a hint here, it is the /O2 shape. ws_HandleImpact is the
// only caller in the image, and the shipped client has no standalone copy of
// this function at all: 0x0041a680 (528 bytes) carries
// the dot product, the `fcoms` against zero and the three vector builds in
// its own body. /Ob1 will only expand a callee marked `inline`, and only one
// defined ABOVE its caller, which this is. /O1 declines a 228-byte expansion
// on size, so server.dll keeps both the standalone body at 0x100051ec and the
// call to it.
inline int ws_Bounce(LTObject* pObj, LTVector* pVel, LTVector* pNormal,
                     LTVector* pImpulse, LTVector* pMomentum)
{
	LTVector v = *pVel;
	float    fDot;

	// The grouping is not decoration. VC6 emits an FP add's operands in
	// reverse, so a flat `x*nx + y*ny + z*nz` can only ever put the x term
	// last -- and the shipped code puts the z term last (PH server.dll
	// 0x1000338c: y, x, z). Only a sum whose right operand is itself an add
	// reaches this order. The normal is the left operand of each product in
	// AvP2; the vector first gives the same server.dll bytes.
	fDot = pNormal->z * v.z + (pNormal->x * v.x + pNormal->y * v.y);

	if (fDot < 0.0f)
	{
		fDot = -fDot;

		*pImpulse = *pNormal * fDot;

		*pMomentum = (*pImpulse * WS_IMPULSE) * pObj->m_Mass;

		// AVP2: only off a floor-like surface (PH server.dll 0x1000341d).
		if (pNormal->y > 0.7071f)
			*pImpulse += *pNormal * WS_SEPARATION;

		return 1;
	}

	return 0;
}

// One coordinate against one axis of the region. The two bounds are tested
// in the order they are stored, so a coordinate below the minimum is
// reported without the maximum being looked at.
//
// The `else if`/`else` is what makes the three constant returns land as they
// shipped: each writes out its own `pop eax; ret` rather than cross-jumping
// onto the next one's. Three separate `if (...) return X;` statements share
// one exit instead.
int CSimContext::ClassifyAxis(float f)
{
	if (f < m_fMin)
		return WS_BELOW;
	else if (f > m_fMax)
		return WS_ABOVE;
	else
		return WS_INSIDE;
}

float g_fStepMinNormalY = 0.1f;
float g_fStepSlopeLimit = 0.7071f;

// The file's class-typed globals, and how they are declared, are read off the
// shipped initialiser table: VC6 gives each DECLARATION (not each declarator)
// a dynamic initialiser, an empty constructor's being a `jmp` to a `ret`.
// Every shipped engine has six of those for this file, then g_BoxPlanes' own
// (Primal Hunt's lithtech.exe 0x0041a4d0-0x0041a590, server.dll
// 0x1000211b-0x1000213f; AvP2's 0x004182b0, 0x10002071), and none after: so
// the three spheres are one declaration, as are the three move vectors, and
// g_BoxPlanes is defined after all of them (below the step-up's two vectors).
CSphereQuery g_BoxSphere, g_SphereAtOffset, g_SphereAtDelta;
float        g_fBoxRadius;

LTVector g_SweepPoints[2][8];

// Turning a box into something the tree walk can test. Only the two faces
// square to Y are built: the walk needs a wound face to clip against and the
// four sides fall out of the six planes, whose distances are the box's own
// corners. Each face is kept twice, so that the copy can be clipped while the
// original stays as it was.
//
// `inline`, and both of the two things that follow from it are in the shipped
// client. ws_GetBoxStatus (0x00417840, 968 bytes against our 432) writes the
// whole of this body into itself -- the run of stores into g_SweepPoints at
// 0x4b2830 is right there ahead of its bspq_TestSphere call -- while
// ws_SetupSweptBox still CALLS it at 0x004183f5, which is what keeps the
// standalone copy at 0x00418410 in the image and matching. That is VC6
// deciding per call site, so the keyword does not cost the out-of-line body.
// /O1 declines a 562-byte expansion outright, so server.dll is untouched.
inline void ws_SetupBoxGeometry(LTVector* pMin, LTVector* pMax)
{
	g_SweepPoints[0][0].Init(pMin->x, pMax->y, pMin->z);
	g_SweepPoints[0][1].Init(pMin->x, pMax->y, pMax->z);
	g_SweepPoints[0][2].Init(pMax->x, pMax->y, pMax->z);
	g_SweepPoints[0][3].Init(pMax->x, pMax->y, pMin->z);

	g_SweepPoints[0][4] = g_SweepPoints[0][0];
	g_SweepPoints[0][5] = g_SweepPoints[0][1];
	g_SweepPoints[0][6] = g_SweepPoints[0][2];
	g_SweepPoints[0][7] = g_SweepPoints[0][3];

	g_SweepPoints[1][0].Init(pMin->x, pMin->y, pMin->z);
	g_SweepPoints[1][1].Init(pMin->x, pMin->y, pMax->z);
	g_SweepPoints[1][2].Init(pMax->x, pMin->y, pMax->z);
	g_SweepPoints[1][3].Init(pMax->x, pMin->y, pMin->z);

	g_SweepPoints[1][4] = g_SweepPoints[1][0];
	g_SweepPoints[1][5] = g_SweepPoints[1][1];
	g_SweepPoints[1][6] = g_SweepPoints[1][2];
	g_SweepPoints[1][7] = g_SweepPoints[1][3];

	g_BoxPlanes[0].m_Dist =  pMin->x;
	g_BoxPlanes[1].m_Dist = -pMax->x;
	g_BoxPlanes[2].m_Dist =  pMin->y;
	g_BoxPlanes[3].m_Dist = -pMax->y;
	g_BoxPlanes[4].m_Dist =  pMin->z;
	g_BoxPlanes[5].m_Dist = -pMax->z;
}

// Will the physics collide with this polygon? Three answers, cheapest first.
// Geometry the world data does not mark solid is not collided with at all;
// geometry whose bounding sphere is further from the box's bounding sphere
// than the two radii together cannot be reached; and everything else is
// settled by clipping the polygon against the box's own six planes and asking
// whether anything survived.
//
// The clip is Sutherland-Hodgman, run six times with the two vertex lists
// swapping roles each round. Only the input list is the polygon's own vertex
// pointers; every list after that is built in the working array, and every
// vertex the clip invents is bump-allocated out of vClipped, which is never
// reset -- so the two arrays have to be big enough for all six rounds at once
// rather than for one.
//
// The clip planes are read one number at a time: an axis-aligned normal is
// the plane's index halved for the axis and g_BoxSigns[i] for the direction,
// so the distance is one multiply rather than a dot product.
//
// Byte-identical on server.dll (0x10001cad). The last instruction was the
// zeroing of nStored: shipped has `and %esi,-0x1c(%ebp)` (the /O1
// `and [x],0` idiom with the zero already in %esi substituted for the
// immediate) ahead of the first `lea`, where a `nStored = 0` written before
// the three list-pointer assignments gave `mov %esi` one instruction later.
// The statement order is pointers, then the vertex count, then nStored. It
// also helps lithtech.exe (0x4152f0, 621 bytes): that build reads m_nVertices
// after the pointer `lea`s, and all that is left there is the order the /O2
// build squares the sphere-reject terms in (z, x, y where ours does x, y, z).
// Changing the term order in the source does not move it on either image.
//
// Two earlier fixes are worth remembering. The four terms
// of the sphere reject are named locals assigned in the order sum, z, y, x --
// the reverse of the order the comparison uses them, which is the order a
// stack machine has to push them in. And `pDist` is a real local: naming
// `&g_BoxPlanes[i].m_Dist` once per round is what puts the plane's induction
// variable in the first frame slot and makes it, rather than g_BoxSigns', the
// one the loop's exit test compares. Writing `g_BoxPlanes[i].m_Dist` at both
// use sites gives the same instructions with two frame slots transposed and
// the exit test on the other induction variable.
// AVP2: byte-identical on both lithtech.exe images (PH 0x00418e80, 1113
// bytes with an exception frame; AvP2 0x00416f10, 929). Besides the polygon
// layout (ltobject_internal.h's WorldPolyOwner), the two bounds
// out-parameters (filled from the surviving vertices only when both are
// given; PH 0x0041ec79 and 0x004207f9 pass them, every other caller passes
// null) and the client's timer (AvP2 0x00416f3e..0x00416f5d, a CountAdder on
// 0x004e0490), three shapes closed it:
//   * the three lists are static locals (AvP2 0x004df018, 0x004df418,
//     0x004e0070): LTVector's empty constructor gives the clipped array the
//     guard (0x004df010) and the atexit of an empty destructor (0x004172c0)
//     the function opens with, ahead of the stat counter;
//   * the copy loop re-reads m_nVertices for its test (the statics can
//     alias the polygon, server.dll 0x10001e1e);
//   * the bounds walk counts nVerts down and steps ppIn (`dec; jne`, and
//     PH's `test; je` entry), each component an LTMIN/LTMAX of the bound
//     and (*ppIn) -- re-read every time, `mov (%edi),%esi`.
// NEAR on both server.dll images, the same 824 / 785 bytes: AvP2 1.0.9.6's
// (0x10001d4e) differs in one instruction, the clip loop's `fmuls
// (%esi,%eax,1)` (0x10001ea3), base and index the other way round from
// ours; Primal Hunt's (0x10001db0) also swaps %ebx and %edi (ppIn, nVerts)
// and the j / ppOut frame slots.
int w_IsPolyBlocking(WorldPolyOwner* pOwner, LTVector* pMin, LTVector* pMax)
{
	static LTVector   vClipped[WS_MAX_CLIP_VERTS];
	static LTVector*  pWork[WS_MAX_CLIP_VERTS];
	static LTVector*  pInList[WS_MAX_POLY_VERTS];
	LTVector** ppIn;
	LTVector** ppOut;
	LTVector** ppOutBase;
	float      fPrevDist;
	float      fCurDist;
	float      fSign;
	int        nStored;
	int        bPrevIn;
	int        bCurIn;
	int        nAxis;
	int        nPrev;
	int        nVerts;
	int        i;
	int        j;
	float*     pDist;
	float      fSum;
	float      fDX, fDY, fDZ;

	g_nPhysStatA8++;

#ifdef LT_SERVER_TIMERS
	CountAdder cntAdder(&g_nPhysStatA4);
#endif

	if (!(pOwner->m_pSurface->m_Flags & 1))
		return 0;

	// All four terms are pushed onto the FPU stack before any of the squaring
	// happens, deepest first, which is the reverse of the order they are used
	// in: so the four assignments below run sum, z, y, x and the comparison
	// then reads them back x, y, z, sum.
	fSum = g_BoxSphere.m_fRadius + pOwner->m_fRadius;
	fDZ  = g_BoxSphere.m_vPoint.z - pOwner->m_vCentre.z;
	fDY  = g_BoxSphere.m_vPoint.y - pOwner->m_vCentre.y;
	fDX  = g_BoxSphere.m_vPoint.x - pOwner->m_vCentre.x;

	if (fDX * fDX + fDY * fDY + fDZ * fDZ > fSum * fSum)
		return 0;

	ppOut     = pWork;
	ppOutBase = pWork;
	ppIn      = pInList;

	nVerts = pOwner->m_nVertices;

	nStored = 0;

#ifdef AVP2SRC_MODERN
	// fixed: w_IsPolyBlocking has no bounds checks (RETAIL_BUGS.md). A
	// polygon too big for the lists, or a clip that would run out of room,
	// is answered as blocking: it has already passed the sphere test, and
	// colliding with it is the safe mistake.
	if (nVerts > WS_MAX_POLY_VERTS)
		return 1;

#endif
	for (j = 0; j < pOwner->m_nVertices; j++)
		pInList[j] = pOwner->m_Vertices[j].m_pPos;

	for (i = 0; i < 6; i++)
	{
		pDist = &g_BoxPlanes[i].m_Dist;
		fSign = g_BoxSigns[i];
		nAxis = i >> 1;

		nPrev = nVerts - 1;

		fPrevDist = fSign * (*ppIn[nPrev])[nAxis] - *pDist;

		if (fPrevDist > WS_CLIP_SLOP)
			bPrevIn = 1;
		else
			bPrevIn = 0;

		for (j = 0; j < nVerts; j++)
		{
#ifdef AVP2SRC_MODERN
			// Room for this step's two pointers and one new vertex.
			if (ppOut + 2 > pWork + WS_MAX_CLIP_VERTS ||
			    nStored >= WS_MAX_CLIP_VERTS)
				return 1;

#endif
			fCurDist = fSign * (*ppIn[j])[nAxis] - *pDist;

			if (fCurDist > WS_CLIP_SLOP)
				bCurIn = 1;
			else
				bCurIn = 0;

			if (bPrevIn)
			{
				*ppOut = ppIn[nPrev];
				ppOut++;
			}

			if (bPrevIn != bCurIn)
			{
				// Both ends are written out long, not through locals: the
				// shipped code re-reads ppIn[nPrev] once per component (two
				// uses each) and keeps only the *address* of ppIn[j], which
				// it already has from the distance above.
				float fT = fPrevDist / (fPrevDist - fCurDist);

				vClipped[nStored].x = (ppIn[j]->x - ppIn[nPrev]->x) * fT +
				                      ppIn[nPrev]->x;
				vClipped[nStored].y = (ppIn[j]->y - ppIn[nPrev]->y) * fT +
				                      ppIn[nPrev]->y;
				vClipped[nStored].z = (ppIn[j]->z - ppIn[nPrev]->z) * fT +
				                      ppIn[nPrev]->z;

				*ppOut = &vClipped[nStored];
				ppOut++;
				nStored++;
			}

			fPrevDist = fCurDist;
			nPrev     = j;
			bPrevIn   = bCurIn;
		}

		// The count is taken before the base moves on: that keeps ppOut live
		// across the subtraction, so the shipped code copies it into a second
		// register to subtract in and stores the original. Assigning the base
		// first instead lets the subtraction happen in place and comes out
		// two bytes shorter.
		ppIn      = ppOutBase;
		nVerts    = ppOut - ppIn;
		ppOutBase = ppOut;

		if (nVerts == 0)
			return 0;
	}


	if (pMin && pMax)
	{
		pMin->x = pMin->y = pMin->z =  1.0e37f;   // 0x7cf0bdc2
		pMax->x = pMax->y = pMax->z = -1.0e37f;   // 0xfcf0bdc2

		while (nVerts--)
		{
			pMin->x = LTMIN(pMin->x, (*ppIn)->x);
			pMin->y = LTMIN(pMin->y, (*ppIn)->y);
			pMin->z = LTMIN(pMin->z, (*ppIn)->z);
			pMax->x = LTMAX(pMax->x, (*ppIn)->x);
			pMax->y = LTMAX(pMax->y, (*ppIn)->y);
			pMax->z = LTMAX(pMax->z, (*ppIn)->z);

			ppIn++;
		}
	}

	return 1;
}

// Everything the walk needs, from where the sweep starts and ends and how big
// the thing being swept is. Squares are kept alongside their roots because
// almost every test the walk makes compares squares — the roots are only
// wanted where a length has to be halved or divided by.
//
// A sweep that does not move has no direction, and gets a zero one rather
// than a division: the tests that use it are all against the midpoint, which
// is still meaningful.
//
// Three things closed it, on both images (server 0x10002637, lithtech
// 0x4169e0):
//   * the zero direction is `m_vDir.Init()`, not three component stores. The
//     inlined call makes VC6 forget which FPU registers held which members,
//     so the shipped build drops m_fLength at the join and re-reads it for
//     m_fReach (`flds 0x4c(%ebx)`), and at /O2 re-reads m_fCornerDist too
//     (`fadds 0x30(%ecx)`). With component stores ours carried both on the
//     FPU stack across the join, which also cost the midpoint's x sum its
//     register.
//   * the reciprocal is written inline, `m_vDelta * (1.0f / m_fLength)`: the
//     shipped `fdiv %st(1),%st; fstp %st(1)` keeps the length alive for the
//     divide and drops it after; a named fInv gave `fdivp`.
//   * m_fHitDepth is cleared before m_pHitPoly: /O2 keeps that store order
//     (0x98 before 0x9c at 0x416b80); /O1 emits the same bytes either way.
//
// The two extents closed on the SDK macros. Two things were wrong before:
// the arms were the wrong way round (the shipped `jae`/`jbe` jump to the
// m_vEnd arm and fall through to m_vStart, which is the false-branch polarity
// of `m_vStart > m_vEnd`, not of `m_vEnd >= m_vStart`), and the operands were
// named in locals, where the shipped code reads the members four times per
// statement. Writing them as members with the condition inverted got the
// polarity but still compiled the compare as one load and an `fcomps` against
// memory. LTMAX/LTMIN evaluate `(a)` twice, and that is what makes VC6 push
// *both* operands and compare with `fcompp` — the shipped shape exactly.
void CSimContext::Setup()
{
	m_fRadiusSqr = m_fRadius * m_fRadius;
	m_fCornerSqr = m_fHeight * m_fHeight + m_fRadiusSqr;
	m_fCornerDist = (float)sqrt(m_fCornerSqr);

	m_vDelta = m_vEnd - m_vStart;

	m_fLengthSqr = m_vDelta.Dot(m_vDelta);
	m_fLength = (float)sqrt(m_fLengthSqr);

	if (m_fLength != 0.0f)
	{
		m_vDir = m_vDelta * (1.0f / m_fLength);
	}
	else
	{
		m_vDir.Init();
	}

	m_vMid = (m_vStart + m_vEnd) * 0.5f;

	m_fReach = m_fLength * 0.5f + m_fCornerDist;
	m_fReachSqr = m_fReach * m_fReach;

	// Both extents read the members, not locals: each comparison pushes
	// m_vStart.y and m_vEnd.y afresh, compares them with `fcompp`, and then
	// each arm loads its own member again. Four loads per statement where a
	// pair of locals would have made two.
	m_fMax = LTMAX(m_vStart.y, m_vEnd.y) + m_fHeight;
	m_fMin = LTMIN(m_vStart.y, m_vEnd.y) - m_fHeight;

	m_fHitDepth = 0.0f;
	m_pHitPoly = NULL;
}

// Which side of a plane the swept volume falls on. The volume is a cylinder,
// so how far it reaches towards a plane depends on how the plane is tilted:
// a plane whose normal is level is cleared by the radius alone, one whose
// normal is straight up or down by the height alone, and anything between by
// a blend of the two. The cheap flag asks for the corner distance instead —
// the largest either could be — which is right whenever a wrong answer only
// costs more work later.
// Every exit is written as one explicit if/else chain whose arms each end in
// their own return: that is what puts the merged `return WS_BEHIND` INLINE at
// its first site as shipped (early guarded returns host it at the last; a goto
// to the first site does not move it). The overlap arms are `if (bWantOverlap)
// m_fOverlap = ...; return WS_STRADDLE;` and the final BEHIND arm keeps its
// single return -- splitting that into two returns cost lithtech.exe eight
// bytes.
//
// The dot product is written as an accumulator for the reason set out at
// ws_ProjectBox: the shipped build emits its terms in source order, which a
// single `a + b + c` expression cannot do.
int CSimContext::ClassifyPlane(LTPlane* pPlane, LTVector* pPoint,
                               int bWantOverlap, int bCheap)
{
	LTVector v = *pPoint;
	float    fDist;
	float    fReach;
	float    fFlat;

	fDist  = v.x * pPlane->m_Normal.x;
	fDist += v.y * pPlane->m_Normal.y;
	fDist += v.z * pPlane->m_Normal.z;
	fDist -= pPlane->m_Dist;

	if (bWantOverlap)
		m_fPlaneDist = fDist;

	if (bCheap)
	{
		if (fDist >= m_fCornerDist)
		{
			return WS_INFRONT;
		}
		else if (-m_fCornerDist >= fDist)
		{
			return WS_BEHIND;
		}
		else if (pPlane->m_Normal.y == 0.0)
		{
			if (fDist >= m_fRadius)
			{
				return WS_INFRONT;
			}
			else if (-m_fRadius >= fDist)
			{
				return WS_BEHIND;
			}
			else
			{
				if (bWantOverlap)
					m_fOverlap = m_fRadius - fDist;

				return WS_STRADDLE;
			}
		}
		else if (pPlane->m_Normal.x == 0.0 && pPlane->m_Normal.z == 0.0)
		{
			if (fDist >= m_fHeight)
			{
				return WS_INFRONT;
			}
			else if (-m_fHeight >= fDist)
			{
				return WS_BEHIND;
			}
			else
			{
				if (bWantOverlap)
					m_fOverlap = m_fHeight - fDist;

				return WS_STRADDLE;
			}
		}
	}

	// One expression, so the two cached normal components stay on the stack
	// until sqrt's argument is stored (0x100028a4: `fstpl (%esp)` before the
	// two `fstp %st(0)`). The (double) on the x square keeps it first; without
	// it VC6 squares z first and lithtech.exe breaks (512 bytes). As two
	// statements the stack is tidied before the store instead.
	fFlat = (float)sqrt((double)(pPlane->m_Normal.x * pPlane->m_Normal.x) +
	                    pPlane->m_Normal.z * pPlane->m_Normal.z);

	fReach = fFlat * m_fRadius;
	fReach = (float)sqrt(1.0f - fFlat * fFlat) * m_fHeight + fReach;

	if (fDist >= fReach)
	{
		return WS_INFRONT;
	}
	else if (-fReach >= fDist)
	{
		if (!bCheap && bWantOverlap)
			m_fOverlap = (fReach + fReach) - fDist;

		return WS_BEHIND;
	}
	else
	{
		if (bWantOverlap)
			m_fOverlap = fReach - fDist;

		return WS_STRADDLE;
	}
}

// The move in progress. The from-point and the wanted delta are what the
// caller asked for; the offset is what the solver has had to add to keep the
// box out of the world, and every contact found along the way is remembered
// in the list below so the same object cannot push twice.
int       g_nContacts;
WSContact g_Contacts[WS_MAX_CONTACTS];

LTVector    g_vMoveBase, g_vMoveStart, g_vMoveEnd;
CMoveState*    g_pMoveState;
CBounceState*  g_pMoveResult;
CPhysicsBlock* g_pPBlock;

// Statistics, all zeroed at the top of each move and read back by the timing
// console commands (names ours).
uint32 g_nPhysMoves;
uint32 g_nPhysNodes;
uint32 g_nPhysContactTime;
uint32 g_nPhysStatA4;
uint32 g_nPhysStatA8;
uint32 g_nPhysStepTime;
uint32 g_nPhysMoveTime;

// Nothing is this far away, so the first corner always wins the comparison.
#define WS_FAR 1.0e37f

// AVP2: add a displacement to a position so that it always moves it: when
// the sum comes out equal to where it started -- the step is below the
// position's float precision -- the step is scaled up to FLT_EPSILON of the
// position's own length first (PH lithtech 0x0041f960, server.dll's twin
// beside it). A step shorter than FLT_EPSILON is not taken at all. The one
// caller is the box move, which passes the move base as both the result
// and the start (0x0041f597..0x0041f5b9). Names ours.
//
// IDENTICAL on PH lithtech. The magnitude is assigned inside the test
// (`fsts`, 0x0041f99b) and reused for the scale (`flds x; fmul %st(1)`,
// 0x0041fa35); a separate scale local gives `fld %st(0); fmuls x`.
// NEAR on PH server.dll (0x100059a4, 180 of 180): the magnitude and the
// y-sum temporary swap frame slots (shipped -0x8 and the dead pOut slot,
// ours the other way round); with a separate scale server.dll keeps that
// swap too, so it is slot assignment, not the scale.
//
// AVP2: `inline` in both engines. AvP2 1.0.9.6's lithtech.exe box move
// (0x0041cbda) carries the body for the axis push-out and calls the copy
// (0x0041d640) only from ws_HitPoly's, past where /O2 stops inlining;
// Primal Hunt's lithtech.exe expands it in the box move and the new stair
// step (0x00420d6b, 0x00420f0f), and its server.dll's /Ob1 expands it into
// ws_HitPoly, which is then byte-identical (0x10005a58).
inline void ws_NudgeQ516(LTVector* pOut, LTVector vStart, LTVector vStep)
{
	float fMag;
	float fScale;

	*pOut = vStart;

	// AVP2: negated: cl 9044 keeps NaN apart where 8966 does not, and AvP2
	// 1.0.9.6's server.dll (0x100059a9 `test $0x5; jnp`) nudges on an
	// unordered magnitude.
	if (!((fMag = vStep.Mag()) < 1.192092896e-07f))
	{
		*pOut += vStep;

		// The test is `!=`: AvP2 1.0.9.6's box move, past where /O2 stops
		// inlining, CALLS _CVector::operator!= here (0x0041cc7d onto
		// 0x0041f7e0) and branches away on true.
		if (*pOut != vStart)
			return;

		fScale = vStart.Mag() * 1.192092896e-07f;
		fScale /= fMag;

		vStep *= fScale;

		*pOut = vStart + vStep;
	}
}

// Remember a plane the box has hit; the moment there are two, the solver
// has to be run. Above the solver, as shipped (PH 0x0041fa90 / 0x0041fb80):
// /GX keeps the counter's unwind state around a callee VC6 has not yet seen
// (0x0041facf), and has none to keep around one it has already compiled.
//
// AVP2: no depth and no shove -- the contact is only recorded (PH lithtech
// 0x0041fa90, server.dll 0x10005c4c: two arguments, nothing touches
// g_vMoveEnd), and the client times it too.
void ws_AddContact(LTPlane* pPlane, void* pObject)
{
	CountAdder cntAdder(&g_nPhysContactTime);
	int        i;

	if (g_nContacts < WS_MAX_CONTACTS)
	{
		if (pObject)
		{
			for (i = 0; i < g_nContacts; i++)
			{
				if (g_Contacts[i].m_pObject == pObject)
					return;
			}
		}

		g_Contacts[g_nContacts].m_Plane = *pPlane;
		g_Contacts[g_nContacts].m_pObject = pObject;
		g_nContacts++;

		if (g_nContacts > 1)
			ws_PushBoxOutOfContacts();
	}
}

// Byte-identical on both images. `vTest.Init(...)` rather than three
// component stores is what lets the prologue's pushes interleave with the FP
// adds as shipped; `vPush = normal * (BIAS - fMin)` through the operator is
// what keeps fMin on the stack (`flds BIAS; fsub %st(1),%st; fstp %st(1)`)
// instead of a named fPush's `fsubrs`; and with Init in place,
// `for (i = 0, nTries = 0; ...)` orders lithtech's two zeroing xors.
//
// Push the box out of every contact plane it is still inside. One pass is not
// enough: pushing clear of one plane can bury the box in another, so a plane
// that had to push restarts the sweep. Ten pushes is the budget — a box in a
// corner tight enough to need more is stuck, and no amount of shoving will
// free it.
void ws_PushBoxOutOfContacts()
{
	CMoveState* pState;
	WSContact*  pContact;
	LTVector*   pCorner;
	LTVector    vCorners[8];
	LTVector    vTest;
	LTVector    vPush;
	float       fMin, fDist;
	int         i, nTries, n;

	vTest.Init(g_vMoveBase.x + g_vMoveEnd.x,
	           g_vMoveBase.y + g_vMoveEnd.y,
	           g_vMoveBase.z + g_vMoveEnd.z);

	pState = g_pMoveState;

	vCorners[0].x = vTest.x + pState->m_vDims.x;
	vCorners[0].y = vTest.y + pState->m_vDims.y;
	vCorners[0].z = vTest.z + pState->m_vDims.z;
	vCorners[1].x = vTest.x - pState->m_vDims.x;
	vCorners[1].y = vTest.y + pState->m_vDims.y;
	vCorners[1].z = vTest.z + pState->m_vDims.z;
	vCorners[2].x = vTest.x - pState->m_vDims.x;
	vCorners[2].y = vTest.y + pState->m_vDims.y;
	vCorners[2].z = vTest.z - pState->m_vDims.z;
	vCorners[3].x = vTest.x + pState->m_vDims.x;
	vCorners[3].y = vTest.y + pState->m_vDims.y;
	vCorners[3].z = vTest.z - pState->m_vDims.z;
	vCorners[4].x = vTest.x + pState->m_vDims.x;
	vCorners[4].y = vTest.y - pState->m_vDims.y;
	vCorners[4].z = vTest.z + pState->m_vDims.z;
	vCorners[5].x = vTest.x - pState->m_vDims.x;
	vCorners[5].y = vTest.y - pState->m_vDims.y;
	vCorners[5].z = vTest.z + pState->m_vDims.z;
	vCorners[6].x = vTest.x - pState->m_vDims.x;
	vCorners[6].y = vTest.y - pState->m_vDims.y;
	vCorners[6].z = vTest.z - pState->m_vDims.z;
	vCorners[7].x = vTest.x + pState->m_vDims.x;
	vCorners[7].y = vTest.y - pState->m_vDims.y;
	vCorners[7].z = vTest.z - pState->m_vDims.z;

	for (i = 0, nTries = 0; i < g_nContacts; i++)
	{
		if (nTries >= WS_PUSH_TRIES)
			break;

		pContact = &g_Contacts[i];

		fMin = WS_FAR;

		pCorner = vCorners;
		n = 8;

		do
		{
			LTVector v = *pCorner;

			// AVP2: LTPlane::DistTo, whose dot product puts y*ny before x*nx
			// (PH 0x0041fccd) where the four statements gave x first.
			fDist = pContact->m_Plane.DistTo(v);

			if (fDist < fMin)
				fMin = fDist;

			pCorner++;
			n--;
		}
		while (n);

		if (fMin < WS_PUSH_SLOP)
		{
			vPush = pContact->m_Plane.m_Normal * (WS_PUSH_BIAS - fMin);

			g_vMoveEnd.x += vPush.x;
			g_vMoveEnd.y += vPush.y;
			g_vMoveEnd.z += vPush.z;

			pCorner = vCorners;
			n = 8;

			do
			{
				pCorner->x += vPush.x;
				pCorner->y += vPush.y;
				pCorner->z += vPush.z;

				pCorner++;
				n--;
			}
			while (n);

			i = -1;
			nTries++;
		}
	}
}

// Is the swept box clear of this polygon? Every edge of the polygon carries a
// plane that stands on it at right angles to the polygon's own, and the box
// misses the polygon if all sixteen of its points fall outside any one of
// them. The two arrays are the box where the move starts and where it ends.
// Both loops test at the bottom and return from inside: the shipped code ends
// each in `cmp; jae <return>; jmp top` (0x100046da, 0x100046f2; lithtech
// 0x4198d0, 0x4198ed) with `return 1` laid out before `return 0`. A for loop's
// own condition cannot give that -- `for (j...)` is not rotated at all, and
// `for (;;)` or do/while peel the first test (378 bytes). pCur, the edge as a
// vector difference, and the unnamed DistTo/Dot arguments set the register
// choice; lithtech 0x419700 is byte-identical.
// NEAR (server, 340 of 340 bytes, 4 instructions): the m_Dist dot product
// multiplies z, x, y at 0x10004649; ours does y, x, z. Operand order only.
//
// AVP2: in neither engine. Its only caller is NOLF's three-argument
// ws_MoveBox (below, not built); the four-argument move tests the straddle
// with w_IsPolyBlocking's clipped bounds instead, and no function of its
// shape is left between the physics' placed neighbours.
#if 0
int ws_IsBoxOutsidePoly(WorldPolyOwner* pPoly, LTVector* pPoints1,
                        LTVector* pPoints2)
{
	LTVector *pCur;
	uint32   j;
	uint32   i;
	LTVector vEdge;
	LTPlane  edgePlane;
	uint32   nPrev;

	nPrev = pPoly->m_nVertices - 1;

	for (i = 0; i < pPoly->m_nVertices; )
	{
		pCur = pPoly->m_Vertices[i].m_pPos;
		vEdge = *pCur - *pPoly->m_Vertices[nPrev].m_pPos;

		edgePlane.m_Normal = pPoly->GetPlane()->m_Normal.Cross(vEdge);
		edgePlane.m_Normal.Norm();

		edgePlane.m_Dist = edgePlane.m_Normal.Dot(*pPoly->m_Vertices[i].m_pPos);

		j = 0;
		while (1)
		{
			if (edgePlane.DistTo(pPoints1[j]) < 0.0f)
				goto NextEdge;
			if (edgePlane.DistTo(pPoints2[j]) < 0.0f)
				goto NextEdge;
			j++;
			if (j >= 8)
				return 1;
		}

NextEdge:
		nPrev = i;
		i++;
		if (i >= pPoly->m_nVertices)
			return 0;
	}
	return 0;
}
#endif

// One surface has been hit: describe it for whoever asked, work out what the
// hit does to the object, and add that to the running total.
//
// AVP2: the plane is always the polygon's (no bWorld), the bounce is taken
// against a normal the caller passes as the last argument (CSimContext::
// DeflectOffHit, PH 0x0041bdb8, hands it its own normalised push), and
// ws_Bounce only nudges the impulse back along a floor-like normal (y above
// 0.7071, PH server.dll 0x10003420). IDENTICAL on both server.dlls
// (PH 0x1000328d, ws_Bounce 0x10003378).
//
// NEAR on lithtech.exe (PH 0x0041bdd0, 565 of 537): /O2 expands ws_Bounce
// here and the inline budget runs out in a different place -- shipped calls
// `operator*(float)` for the mass and for the nudge and `+=` for the nudge
// (0x0041bf2b, 0x0041bf60, 0x0041bf7e) where ours expands the mass multiply;
// and shipped multiplies the dot product's terms y, z, x (0x0041be74..
// 0x0041be80) where every spelling that keeps server.dll's y, x, z gives
// y, x, z here too.
void ws_HandleImpact(CollisionInfo* pInfo, CBounceState* pState,
                     LTObject* pObj, HOBJECT hObject, IWorldBsp* pWorldBsp,
                     WorldPoly* pPoly, LTVector* pNormal)
{
	LTVector vTotal;
	LTVector vImpulse;
	LTVector vMomentum;

	pInfo->m_Plane = *ws_GetPlaneQ506(pPoly->m_pOwner);

	pInfo->m_hObject = hObject;

	if (pWorldBsp)
		pInfo->m_hPoly = pWorldBsp->MakeHPoly(pPoly);
	else
		pInfo->m_hPoly = INVALID_HPOLY;

	vTotal = pObj->m_Velocity + pState->m_vVelocityAdd;

	if (ws_Bounce(pObj, &vTotal, pNormal, &vImpulse, &vMomentum))
	{
		if (pObj->m_Flags & FLAG_TOUCH_NOTIFY)
		{
			pState->m_vMomentum += vMomentum;
		}

		pObj->m_ChangeFlags |= CHANGEFLAG_MOTION;

		pState->m_vVelocityAdd += vImpulse;
	}
}

// ======================================================================= //
//  AVP2: the sphere mover. Primal Hunt's FLAG2_SPHEREPHYSICS objects are
//  moved by a module of their own that NOLF's engine has not: server.dll
//  0x1000346a..0x100041ca (lithtech.exe 0x0041bff0..0x0041d13f), whose two
//  error strings name its polygon gathers (0x1007d350.., 0x1007d2d4..), and
//  which hands the gathered polygons to the swept-sphere module
//  (sweptsphere.cpp, 0x10009413 / 0x100095f4 / 0x100096a3). Names ours.
// ======================================================================= //

// The objects a sphere move is tested against: a count at +4 and eight-byte
// records from +8, of which only the object is read (server.dll 0x10003ae5
// count, 0x10003b18 record; names ours).
struct SphereObjEntryQ553
{
	LTObject* m_pObject;                     // 0x00
	uint32    m_n04;                         // 0x04
};

struct SphereObjListQ553
{
	uint32             m_n00;                // 0x00
	int                m_nCount;             // 0x04
	SphereObjEntryQ553 m_Entries[1];         // 0x08
};

// A polygon gather's optional second answer: which object each polygon
// came from, and its record in that object's table (0x10003c6a, 0x10003c6f).
struct SphereHitQ553
{
	LTObject*     m_pObject;                 // 0x00
	WorldPolyRef* m_pRec;                    // 0x04
};

// The sphere move's own state, as the mover at 0x1000346a reads and writes it
// (names ours).
// Where a gather's segment comes from when the caller gives none: a pointer
// to the start at +0x1c and the end itself at +0x20 (0x100037f6,
// 0x1000380e; names ours).
class CSphereSrcQ553
{
public:
	uint8     m_pad_00[0x1c];
	LTVector* m_pStart;                      // 0x1c
	LTVector  m_vEnd;                        // 0x20
};

class CSphereMoveQ553
{
public:
	LTObject*          m_pObject;            // 0x00  +0x9c rotation, +0x8c flags
	CSphereSrcQ553*    m_pSrc;               // 0x04
	SphereObjListQ553* m_pObjects;           // 0x08
	LTVector           m_vStart;             // 0x0c
	LTVector           m_vEnd;               // 0x18
	float              m_fRadius;            // 0x24
	float              m_fFraction;          // 0x28
	LTVector           m_vNormal;            // 0x2c
	uint8              m_nIterations;        // 0x38
	uint8              m_pad_39[3];
	int                m_nNormals;           // 0x3c
	LTVector           m_vNormals[10];       // 0x40
	int                m_nContacts;          // 0xb8
	LTVector           m_vContacts[10];      // 0xbc
	uint32             m_bContact;           // 0x134
};

// objtypes.cpp's GetPBlockAt (0x1002eced), as worldsim's three callers reach
// it: a thiscall on the world model taking the point by address.
class CObjWorldModelPBlockQ532
{
public:
	class CPhysicsBlock* GetPBlockAt(LTVector* pPos);
};

extern uint16 g_BlockStackQ534[];

// How far the gather reaches past the sphere's own radius (0x100742ac).
#define WS_SPHERE_SLOP 0.1f

// The sphere move's helpers, in address order below the mover (names ours).
void ws_GetSphereCollideTestPolysQ553(CSphereMoveQ553* pMove,
                                      WorldPolyOwner** pList, int* pnCount,
                                      int nMax, LTVector* pStart,
                                      LTVector* pEnd, SphereHitQ553* pHits);
void ws_GetSpherePosTestPolysQ553(CSphereMoveQ553* pMove,
                                  WorldPolyOwner** pList, int* pnCount,
                                  int nMax, LTVector vPos,
                                  SphereHitQ553* pHits);
// server.dll 0x10003cc0 / lithtech.exe 0x0041ca70: the FLAG2_ORIENTMOVEMENT
// pass after a move (below the position gather).
void ws_SphereOrientMoveQ553(CSphereMoveQ553* pMove);

// sweptsphere.cpp's push-out and polygon sweep (0x10009413, 0x100095f4).
LTBOOL sp_PushSphereOutQ533(LTVector* pPos, float fRadius,
                            WorldPolyOwner** ppPolys, int nPolys);
LTBOOL sp_SweptSphereToPolysQ533(LTVector* pStart, LTVector* pEnd,
                                 float fRadius, WorldPolyOwner** ppPolys,
                                 int nPolys, LTVector* pHitNormal,
                                 LTVector* pPlaneNormal, float* pT,
                                 LTBOOL* pbOrient);

// The gathers' list size: 0x200 polygons (`push $0x200`, 0x10003510).
#define WS_SPHERE_MAX_POLYS 0x200

// Ten sweeps, ten remembered normals and ten contacts at most.
#define WS_SPHERE_MAX_ITER 10

// How far past a hit the remaining move is pulled back, and how close to
// the object's up vector a surface may face before it stops counting as a
// contact (0x1007425c, 0x10074258).
#define WS_SPHERE_PUSHBACK  1.02f
#define WS_SPHERE_UP_LIMIT  0.99f

// server.dll 0x1000346a (870 bytes) / lithtech.exe 0x0041bff0 (1151) -- the
// sphere move itself, called from 0x1002801b. The start is first pushed
// clear of whatever it is resting in (no room to do so abandons the move,
// end = start); then up to ten sweeps from start to end: each hit takes the
// rest of the move back off the end along the hit normal -- or, when the
// normal opposes one already met (a crease), the rest of the move outright
// -- remembers the normal, and records it as a contact unless it faces the
// object's own up vector. A sweep that hits nothing ends it; FLAG2_ORIENT
// MOVEMENT then has its own pass (0x100037b0). Answers whether any sweep
// hit.
//
// Byte-identical on server.dll. Load-bearing: `while (TRUE)` (a `for (;;)`
// duplicates the loop head), the fraction stored before the normal
// (0x1000365a), and sp_PushSphereOutQ533 answering a full register
// (`test %eax,%eax`, 0x10003545; sweptsphere.cpp still declares it uint16).
// NEAR on lithtech.exe (1151 of 1151, 6 blocks): the two `end = start`
// copies load all three dwords before storing (0x0041c101, 0x0041c428)
// where ours interleaves them. ILTMath is declared where it is constructed,
// after the field resets, as lithtech's vptr store shows (0x0041c052).
LTBOOL ws_MoveSphereQ553(CSphereMoveQ553* pMove)
{
	int              nCount;
	WorldPolyOwner*  pList[WS_SPHERE_MAX_POLYS];
	LTVector         vRight;
	LTVector         vUp;
	LTVector         vForward;
	LTVector         vHitNormal;
	LTVector         vPlaneNormal;
	float            fT;
	LTBOOL           bOrient;
	LTVector*        pEnd;
	int              j;
	uint8            bOpposed;

	pMove->m_fFraction = 1.0f;
	pMove->m_vNormal = LTVector(0.0f, 0.0f, 0.0f);
	pMove->m_nIterations = 0;
	pMove->m_nNormals = 0;
	pMove->m_nContacts = 0;
	pMove->m_bContact = 0;

	ILTMath    math;
	LTRotation rRot = pMove->m_pObject->m_Rotation;

	math.GetRotationVectors(rRot, vRight, vUp, vForward);

	nCount = 0;

	LTVector* pStart = &pMove->m_vStart;

	ws_GetSpherePosTestPolysQ553(pMove, pList, &nCount, WS_SPHERE_MAX_POLYS,
	                             *pStart, LTNULL);

	if (!sp_PushSphereOutQ533(pStart, pMove->m_fRadius, pList, nCount))
	{
		pMove->m_vEnd = *pStart;

		return 0;
	}

	pEnd = &pMove->m_vEnd;

	while (TRUE)
	{
		ws_GetSphereCollideTestPolysQ553(pMove, pList, &nCount,
		                                 WS_SPHERE_MAX_POLYS,
		                                 LTNULL, LTNULL, LTNULL);

		if (!sp_SweptSphereToPolysQ533(pStart, pEnd,
		                               pMove->m_fRadius, pList, nCount,
		                               &vHitNormal, &vPlaneNormal, &fT,
		                               &bOrient))
			break;

		// AVP2: negated -- a NaN normal stops the move too. cl 9044 keeps
		// NaN apart where 8966 does not (AvP2 1.0.9.6's server.dll
		// 0x10003581 `test $0x41; jne`).
		if (!(vHitNormal.MagSqr() > 0.001f))
		{
			*pEnd = *pStart;

			return 0;
		}

		LTVector vRemain = (*pEnd - *pStart) * (1.0f - fT);
		LTVector vProj   = vHitNormal * vHitNormal.Dot(vRemain);

		if (bOrient && fT <= pMove->m_fFraction)
		{
			pMove->m_fFraction = fT;
			pMove->m_vNormal   = vPlaneNormal;
		}

		bOpposed = 0;

		for (j = 0; j < pMove->m_nNormals && !bOpposed; j++)
			bOpposed = vHitNormal.Dot(pMove->m_vNormals[j]) < 0.0f;

		if (pMove->m_nNormals < WS_SPHERE_MAX_ITER)
		{
			pMove->m_vNormals[pMove->m_nNormals] = vHitNormal;
			pMove->m_nNormals++;
		}

		if (pMove->m_nIterations >= WS_SPHERE_MAX_ITER)
		{
			*pEnd = *pStart;

			return 0;
		}

		if (!bOpposed)
			*pEnd -= vProj * WS_SPHERE_PUSHBACK;
		else
			*pEnd -= vRemain * WS_SPHERE_PUSHBACK;

		if (bOrient && vUp.Dot(vHitNormal) < WS_SPHERE_UP_LIMIT)
		{
			pMove->m_vContacts[pMove->m_nContacts] = vHitNormal;
			pMove->m_nContacts++;
			pMove->m_bContact = 1;
		}

		pMove->m_nIterations++;
	}

	if (pMove->m_pObject->m_Flags2 & FLAG2_ORIENTMOVEMENT)
		ws_SphereOrientMoveQ553(pMove);

	return pMove->m_nIterations > 0;
}

// How far past the segment's own half-length and the sphere's radius the
// collide gather reaches (0x100742a8).
#define WS_SPHERE_COLLIDE_SLOP 5.0f

// server.dll 0x100037d0 (764 bytes) / lithtech.exe 0x0041c470 (934) -- the
// polygons a sphere swept from pStart to pEnd may hit: "GetSphereCollide
// TestPolys" (0x1007d314). The segment defaults to the move's own
// (0x100037f0, 0x1000380b). The walk is GetSpherePosTestPolys' over the
// segment: a plane both ends clear by the radius on one side is descended on
// that side, and a straddler is collected when its bounding sphere comes
// within reach of the segment's midpoint; the far child is pushed by the
// start's side of the plane (0x10003a67), as ws_MoveCylinder does.
// Byte-identical on server.dll: vDelta and vMid are initialised where they
// are declared (assigned into function-scope vectors, VC6 copies vDelta for
// Mag), and vFrom is the per-object copy the start-side distance reads
// (0x100038dd).
void ws_GetSphereCollideTestPolysQ553(CSphereMoveQ553* pMove,
                                      WorldPolyOwner** pList, int* pnCount,
                                      int nMax, LTVector* pStart,
                                      LTVector* pEnd, SphereHitQ553* pHits)
{
	LTVector       vStart;
	LTVector       vEnd;
	float          fReach;
	int            i;
	LTObject*      pObj;
	IWorldBsp*     pBsp;
	CPhysicsBlock* pBlock;
	WorldPolyRef*  pRecs;
	WorldPolyRef*  pRec;
	WSNode*        pNode;
	LTPlane*       pPlane;
	uint16*        pTop;
	uint16         nNode;
	float          fStartDist;
	float          fEndDist;
	float          fPolyReach;
	int            nSide;

	if (pList)
	{
		*pnCount = 0;

		vStart = *(pStart ? pStart : pMove->m_pSrc->m_pStart);
		vEnd   = *(pEnd ? pEnd : &pMove->m_pSrc->m_vEnd);

		LTVector vDelta = vEnd - vStart;
		LTVector vMid   = vStart + vDelta * 0.5f;

		fReach = vDelta.Mag() + pMove->m_fRadius + WS_SPHERE_COLLIDE_SLOP;

		for (i = pMove->m_pObjects->m_nCount - 1; i >= 0; i--)
		{
			pObj = pMove->m_pObjects->m_Entries[i].m_pObject;

			if (pObj->m_ObjectType == OT_WORLDMODEL ||
			    pObj->m_ObjectType == OT_CONTAINER)
			{
				pBsp   = (IWorldBsp*)pObj->m_pWMIface;
				pBlock = ((CObjWorldModelPBlockQ532*)pObj)->GetPBlockAt(&vStart);

				if (pBlock)
				{
					LTVector vFrom = vStart;

					nNode = pBlock->m_nIndex;
					pTop  = g_BlockStackQ534;
					pRecs = pBsp->GetPolyRefs();

					while (TRUE)
					{
						if (pTop - g_BlockStackQ534 >= WS_NODE_STACK)
						{
							dsi_ServerError("GetSphereCollideTestPolys!!  World caused stack overflow!");
							break;
						}

						if (nNode == WS_NODE_EMPTY || nNode == WS_NODE_SOLID)
						{
							if (pTop == g_BlockStackQ534)
								break;

							pTop--;
							nNode = *pTop;
						}

						pNode = &pBlock->m_pNodes[nNode];

						if (pNode->m_nPoly == 0xffff)
						{
							nNode = pNode->m_nChild[1];
							continue;
						}

						pRec   = &pRecs[pNode->m_nPoly];
						pPlane = ws_GetPlaneQ506(pRec->m_pOwner);

						fStartDist = vFrom.z * pPlane->m_Normal.z +
						             vFrom.y * pPlane->m_Normal.y +
						             vFrom.x * pPlane->m_Normal.x - pPlane->m_Dist;

						pPlane = ws_GetPlaneQ506(pRec->m_pOwner);

						fEndDist = vEnd.y * pPlane->m_Normal.y +
						           vEnd.z * pPlane->m_Normal.z +
						           vEnd.x * pPlane->m_Normal.x - pPlane->m_Dist;

						if (fStartDist > pMove->m_fRadius &&
						    fEndDist > pMove->m_fRadius)
						{
							nNode = pNode->m_nChild[1];
							continue;
						}

						if (fStartDist < -pMove->m_fRadius &&
						    fEndDist < -pMove->m_fRadius)
						{
							nNode = pNode->m_nChild[0];
							continue;
						}

						if (pRec->m_pOwner->m_pSurface->m_Flags & 1)
						{
							LTVector vD = pRec->m_pOwner->m_vCentre - vMid;

							fPolyReach = fReach + pRec->m_pOwner->m_fRadius;

							if (vD.MagSqr() <= fPolyReach * fPolyReach)
							{
								if (*pnCount >= nMax)
								{
									dsi_ServerError("GetSphereCollideTestPolys!!  World caused max collision polys!");
									break;
								}

								pList[*pnCount] = pRec->m_pOwner;

								if (pHits)
								{
									pHits[*pnCount].m_pObject = pObj;
									pHits[*pnCount].m_pRec    = pRec;
								}

								(*pnCount)++;
							}
						}

						if (fStartDist > WS_PLANE_SLOP)
							nSide = 1;
						else
							nSide = 0;

						if (pNode->m_nChild[nSide == 0] != WS_NODE_EMPTY &&
						    pNode->m_nChild[nSide == 0] != WS_NODE_SOLID)
						{
							*pTop = pNode->m_nChild[nSide == 0];
							pTop++;
						}

						nNode = pNode->m_nChild[nSide];
					}
				}
			}
		}
	}
}

// server.dll 0x10003acc (500 bytes) / lithtech.exe 0x0041c820 (581) -- every
// polygon of every world model or container in the move's object list whose
// bounding sphere comes within the sphere's reach of vPos: the error string
// at 0x1007d38c names it GetSpherePosTestPolys. Each object's physics block
// under the point is walked through the shared node stack; a plane further
// in front than the reach is descended in front, one behind the slop
// behind, and a straddling polygon on a solid surface (bit 0 of +0x30) is
// collected when the two spheres overlap (`fcompp` against the squared sum,
// 0x10003c45). Both errors leave that object's walk (0x10003cae).
//
// NEAR on server.dll (500 of 500, one instruction): the object load is
// `mov (%ecx,%eax,1),%ebx` (0x10003b18) where ours names the two registers
// the other way round; the collide gather above, which reads the list the
// same way, is identical. A pointer-arithmetic spelling of the entry and a
// loop-scoped pObj give the same. Two shapes it needed: `while (TRUE)` (a
// `for (;;)` peels the stack check into the entry, constant-folded) and
// guarded `continue`s for the three descends (an if/else chain hosts the
// shared descend at the plane test instead of the 0xffff test). NEAR on
// lithtech.exe (565 of 581, 4 blocks): shipped keeps the squared distance's
// z and the reach in frame slots (0x0041c9a8, `sub $0x1c`) and sums x, y, z;
// ours keeps all five values in st and sums z, y, x. The collide gather's
// lithtech copy (0x0041c470, 918 of 934) has the same residual.
void ws_GetSpherePosTestPolysQ553(CSphereMoveQ553* pMove,
                                  WorldPolyOwner** pList, int* pnCount,
                                  int nMax, LTVector vPos,
                                  SphereHitQ553* pHits)
{
	float          fRadius;
	int            i;
	LTObject*      pObj;
	IWorldBsp*     pBsp;
	CPhysicsBlock* pBlock;
	WorldPolyRef*  pRecs;
	WorldPolyRef*  pRec;
	WSNode*        pNode;
	LTPlane*       pPlane;
	uint16*        pTop;
	uint16         nNode;
	float          fDist;
	float          fReach;
#ifndef PRIMAL_HUNT
	int            nSide;
#endif

	if (pList)
	{
		*pnCount = 0;

		fRadius = pMove->m_fRadius + WS_SPHERE_SLOP;

		for (i = pMove->m_pObjects->m_nCount - 1; i >= 0; i--)
		{
			pObj = pMove->m_pObjects->m_Entries[i].m_pObject;

			if (pObj->m_ObjectType == OT_WORLDMODEL ||
			    pObj->m_ObjectType == OT_CONTAINER)
			{
				pBsp   = (IWorldBsp*)pObj->m_pWMIface;
				pBlock = ((CObjWorldModelPBlockQ532*)pObj)->GetPBlockAt(&vPos);

				if (pBlock)
				{
					nNode = pBlock->m_nIndex;
					pTop  = g_BlockStackQ534;
					pRecs = pBsp->GetPolyRefs();

					while (TRUE)
					{
						if (pTop - g_BlockStackQ534 >= WS_NODE_STACK)
						{
							dsi_ServerError("GetSpherePosTestPolys!!  World caused stack overflow!");
							break;
						}

						if (nNode == WS_NODE_EMPTY || nNode == WS_NODE_SOLID)
						{
							if (pTop == g_BlockStackQ534)
								break;

							pTop--;
							nNode = *pTop;
						}

						pNode = &pBlock->m_pNodes[nNode];

						if (pNode->m_nPoly == 0xffff)
						{
							nNode = pNode->m_nChild[1];
							continue;
						}

						pRec   = &pRecs[pNode->m_nPoly];
						pPlane = ws_GetPlaneQ506(pRec->m_pOwner);

						fDist = vPos.z * pPlane->m_Normal.z +
						        vPos.y * pPlane->m_Normal.y +
						        vPos.x * pPlane->m_Normal.x - pPlane->m_Dist;

						if (fDist > fRadius)
						{
							nNode = pNode->m_nChild[1];
							continue;
						}

						if (fDist < WS_PLANE_SLOP)
						{
							nNode = pNode->m_nChild[0];
							continue;
						}

						if (pRec->m_pOwner->m_pSurface->m_Flags & 1)
						{
							LTVector vDelta = pRec->m_pOwner->m_vCentre - vPos;

							fReach = fRadius + pRec->m_pOwner->m_fRadius;

							if (vDelta.MagSqr() <= fReach * fReach)
							{
								if (*pnCount >= nMax)
								{
									dsi_ServerError("GetSpherePosTestPolys!!  World caused max collision polys!");
									break;
								}

								pList[*pnCount] = pRec->m_pOwner;

								if (pHits)
								{
									pHits[*pnCount].m_pObject = pObj;
									pHits[*pnCount].m_pRec    = pRec;
								}

								(*pnCount)++;
							}
						}

#ifdef PRIMAL_HUNT
						if (pNode->m_nChild[0] != WS_NODE_EMPTY &&
						    pNode->m_nChild[0] != WS_NODE_SOLID)
						{
							*pTop = pNode->m_nChild[0];
							pTop++;
						}

						nNode = pNode->m_nChild[1];
#else
						// AVP2: a straddler is descended on the point's side
						// of the plane and the far side pushed, as the
						// collide gather does (AvP2 1.0.9.6's lithtech.exe
						// 0x0041a75d `and $0x100`: an unordered distance
						// takes the back). IDENTICAL on both AvP2 images
						// (lithtech.exe 0x0041a570, server.dll 0x10003a9a).
						if (fDist >= WS_PLANE_SLOP)
							nSide = 1;
						else
							nSide = 0;

						if (pNode->m_nChild[nSide == 0] != WS_NODE_EMPTY &&
						    pNode->m_nChild[nSide == 0] != WS_NODE_SOLID)
						{
							*pTop = pNode->m_nChild[nSide == 0];
							pTop++;
						}

						nNode = pNode->m_nChild[nSide];
#endif
					}
				}
			}
		}
	}
}

// The list the orient pass gathers into: a .bss array of its own rather than
// the mover's frame (server.dll 0x1008bbb8, 0x200 entries, which run up to
// the next global the image references at 0x1008c3b8; name ours).
WorldPolyOwner* g_SphereOrientPolysQ563[WS_SPHERE_MAX_POLYS];

// sweptsphere.cpp's turn to a new up vector (0x100096a3).
LTBOOL sp_SweptSphereOrientQ533(LTVector* pUp, LTObject* pObj);

// How far below the sphere the settle probe reaches, as a fraction of the
// radius (0x100742b4), how far each probe's hit is pulled back (0x100742ac),
// how far along a contact normal the move is pushed (0x100742b0), and how
// closely a surface must face the object's up vector to need no turn
// (0x100742b8, a double).
#define WS_ORIENT_PROBE     0.2f
#define WS_ORIENT_BACKOFF   0.1f
#define WS_ORIENT_PUSH      2.0f
#define WS_ORIENT_ALIGNED   0.99

// server.dll 0x10003cc0 (1291 bytes) / lithtech.exe 0x0041ca70 (1743) -- the
// FLAG2_ORIENTMOVEMENT pass, run by the sphere move once its sweeps are done
// (0x100037bc). The move is walked in steps of just under the sphere's
// radius, and at each step a sweep from one up-vector above the path down
// through two radii looks for a surface the sphere could stand on; the
// first that faces somewhere other than up turns the object to it, and the
// end is moved to where the sweep stopped, a little short. Then: a move that
// made contacts turns the object to their average normal and pushes it off
// along that normal; one that found nothing but had a hit turns it to the
// best hit's plane; and otherwise a short probe below the new end turns it
// to whatever it is about to settle on, or drops it there.
//
// The step count is (int)(length / (radius * 0.99)) + 2 -- 0.99 is a double
// (0x100742b8), and the length is a named float (the shipped `fld %st(0)`
// that keeps it across the divide, 0x10003d58). Load-bearing, all seen on
// server.dll: bFound and nCount zeroed BEFORE the delta, which gives the
// shipped `and $0,mem` stores (0x10003d24, 0x10003d34) -- after it they
// share a zero register with i; the delta read off m_vEnd with pEnd taken
// after it (pEnd in %esi, 0x10003d21); fT at function scope (it shares the
// dead pMove slot, ebp+8, and the step-count temporary shares i's); and
// both rotations ASSIGNED and the contacts' sum Init()ed, which with the
// rest leaves the contact push's `operator*` expanded as shipped.
//
// NEAR in all three images, one residual and the same one: the lerp onto
// the hit, `vStart + (vEnd - vStart) * fT`. Every image expands the
// difference and the scale and calls the vector constructor for the sum
// only (server.dll 0x10003f56..0x10003f9d, lithtech.exe 0x0041cdd3..
// 0x0041ce16, AvP2 server.dll 0x10003f5c..0x10003fa9); ours spends VC6's
// inline budget elsewhere first and calls the constructor for all three
// (server 1341 of 1291, 4 blocks; lithtech 1788 of 1743, 1 block; AvP2
// 1363 of 1315). Spellings that move the budget without changing the code (Dot
// against VEC_DOT, Init() against the 3-float constructor, rotation copy
// against assignment, member against pEnd) never give exactly that site.
// A named `vDir = vEnd - vStart; vDir *= fT;` gets the calls right on
// server.dll and lithtech.exe but stores and copies vDir (server 1297,
// lithtech 1767), which the shipped code does not, so the single
// expression stays.
void ws_SphereOrientMoveQ553(CSphereMoveQ553* pMove)
{
	LTVector   vRight;
	LTVector   vUp;
	LTVector   vForward;
	LTVector   vStart;
	LTVector   vEnd;
	LTVector   vHitNormal;
	LTVector   vPlaneNormal;
	LTBOOL     bOrient;
	float      fT;
	int        nCount;
	int        nSteps;
	int        i;
	LTBOOL     bFound;
	LTVector*  pEnd;

	ILTMath    math;
	LTRotation rRot;
	rRot = pMove->m_pObject->m_Rotation;

	math.GetRotationVectors(rRot, vRight, vUp, vForward);

	bFound = 0;
	nCount = 0;

	LTVector vDelta = pMove->m_vEnd - pMove->m_vStart;

	pEnd = &pMove->m_vEnd;

	float fDist = vDelta.Mag();

	nSteps = (int)(fDist / (pMove->m_fRadius * WS_ORIENT_ALIGNED)) + 2;

	LTVector vStep = vDelta / (float)(nSteps - 1);

	for (i = 0; i < nSteps; i++)
	{
		vStart = vUp + (pMove->m_vStart + vStep * (float)i);
		vEnd   = vStart - vUp * (pMove->m_fRadius * 2.0f);

		ws_GetSphereCollideTestPolysQ553(pMove, g_SphereOrientPolysQ563,
		                                 &nCount, WS_SPHERE_MAX_POLYS,
		                                 &vStart, &vEnd, LTNULL);

		if (sp_SweptSphereToPolysQ533(&vStart, &vEnd, pMove->m_fRadius,
		                              g_SphereOrientPolysQ563, nCount,
		                              &vHitNormal, &vPlaneNormal, &fT,
		                              &bOrient) &&
		    bOrient && vHitNormal.Dot(vUp) <= WS_ORIENT_ALIGNED)
		{
			sp_SweptSphereOrientQ533(&vHitNormal, pMove->m_pObject);

			rRot = pMove->m_pObject->m_Rotation;
			math.GetRotationVectors(rRot, vRight, vUp, vForward);

			fT -= WS_ORIENT_BACKOFF;
			if (fT < 0.0f)
				fT = 0.0f;

			*pEnd = vStart + (vEnd - vStart) * fT;

			bFound = 1;
			break;
		}
	}

	if (pMove->m_nIterations > 0 && !pMove->m_bContact && !bFound)
	{
		if (pMove->m_fFraction < 1.0f)
			sp_SweptSphereOrientQ533(&pMove->m_vNormal, pMove->m_pObject);
	}
	else if (!pMove->m_bContact)
	{
		vStart = *pEnd;
		vEnd   = *pEnd - vUp * (pMove->m_fRadius * WS_ORIENT_PROBE);

		ws_GetSphereCollideTestPolysQ553(pMove, g_SphereOrientPolysQ563,
		                                 &nCount, WS_SPHERE_MAX_POLYS,
		                                 &vStart, &vEnd, LTNULL);

		if (!sp_SweptSphereToPolysQ533(&vStart, &vEnd, pMove->m_fRadius,
		                               g_SphereOrientPolysQ563, nCount,
		                               &vHitNormal, &vPlaneNormal, &fT,
		                               &bOrient))
		{
			*pEnd = vEnd;
		}
		else if (fT < 1.0f && vHitNormal.Dot(vUp) <= WS_ORIENT_ALIGNED)
		{
			sp_SweptSphereOrientQ533(&vPlaneNormal, pMove->m_pObject);
		}
	}
	else
	{
		LTVector vAvg;
		vAvg.Init();

		for (i = 0; i < pMove->m_nContacts; i++)
			vAvg += pMove->m_vContacts[i];

		vAvg /= (float)pMove->m_nContacts;
		vAvg.Norm();

		sp_SweptSphereOrientQ533(&vAvg, pMove->m_pObject);

		*pEnd += vAvg * WS_ORIENT_PUSH;
	}
}

// The other way to answer a contact: instead of shoving the box out along the
// plane's normal, wind the move back to the fraction of it that reaches the
// plane and no further. That is what a slide wants -- the box keeps the part
// of its travel that was legal and loses the part that was not.
// NEAR, and paired again: server 259/259 (same 89 instructions, 16 differ at
// byte level), lithtech 308 of 312. The delta is ONE `vDelta.Init(...)` call,
// not three component stores: through Init's by-value arguments VC6 no longer
// CSEs g_vMoveStart.x from the delta into the `+=` tail, so the tail reloads it
// (`fadds 0x10058290` at 0x1000452d) as shipped does. The component spelling
// kept it in st(3) for the whole function and came out 255 bytes with a 12-byte
// prefix, which the pairing could not place. What Init costs: VC6 now promotes
// vDelta and shares vScaled's slot, so the frame is 0xc against the shipped
// 0x18 (0x100044b7), and lithtech, which was 312/312 with four operand-order
// residuals in the scale, drops to 308. Tried and no better on server.dll:
// `vDelta = g_vMoveEnd - g_vMoveStart` (copies g_vMoveStart, 261), vDelta's
// ctor (= Init), `vDelta -= g_vMoveStart` (262), VEC_SUB, a pointer to
// g_vMoveStart or g_vMoveEnd, VEC_DOT or vDelta.Dot, vScaled.Init / LTVector(...) /
// VEC_MULSCALAR for the scale, `g_vMoveEnd = vDelta * fRatio`, and VEC_ADD in
// the tail. Declaring g_vMoveBase/Start/End as one struct kills the CSE with
// the component spelling too (vDelta at -0x18 is kept), but it costs
// ws_MoveObject its match, so the globals really are separate.
//
// AVP2: in neither engine. NOLF's ws_HitPoly calls it; AvP2's and Primal
// Hunt's ws_HitPoly do not, nothing else does, and /OPT:REF drops it: the
// retail images have no function of its shape between ws_MovePoint and
// ws_AddContact. NOLF's, kept for reference.
#if 0
void ws_AddClippedContact(LTPlane* pPlane, void* pObject, float fDist)
{
	LTVector vDelta;
	LTVector vScaled;
	float    fDot, fNew, fRatio;
	int      i;

	vDelta.Init(g_vMoveEnd.x - g_vMoveStart.x,
	            g_vMoveEnd.y - g_vMoveStart.y,
	            g_vMoveEnd.z - g_vMoveStart.z);

	fDot = pPlane->m_Normal.Dot(vDelta);

	fNew = fDist - fDot;

#ifdef AVP2SRC_MODERN
	// fixed: ws_AddClippedContact divides by a dot product that can be zero
	// (RETAIL_BUGS.md). The divisor, fDist - fNew, is fDot: a move parallel
	// to the plane never reaches it, so the move is left as it is.
	if (fDot != 0.0f)
	{
		fRatio = -(fNew / (fDist - fNew));

		vScaled = vDelta * fRatio;

		g_vMoveEnd = vScaled;

		g_vMoveEnd.x += g_vMoveStart.x;
		g_vMoveEnd.y += g_vMoveStart.y;
		g_vMoveEnd.z += g_vMoveStart.z;
	}
#else
	fRatio = -(fNew / (fDist - fNew));

	vScaled = vDelta * fRatio;

	g_vMoveEnd = vScaled;

	g_vMoveEnd.x += g_vMoveStart.x;
	g_vMoveEnd.y += g_vMoveStart.y;
	g_vMoveEnd.z += g_vMoveStart.z;
#endif

	if (g_nContacts < WS_MAX_CONTACTS)
	{
		if (pObject)
		{
			for (i = 0; i < g_nContacts; i++)
			{
				if (g_Contacts[i].m_pObject == pObject)
					return;
			}
		}

		g_Contacts[g_nContacts].m_Plane = *pPlane;
		g_Contacts[g_nContacts].m_pObject = pObject;
		g_nContacts++;

		if (g_nContacts > 1)
			ws_PushBoxOutOfContacts();
	}
}
#endif

// Project all eight corners of the swept box onto a plane and keep the
// nearest and furthest. Answering which side the box is on then costs two
// comparisons, and the interval it leaves behind is what tells a caller how
// deeply the box has crossed.
//
// **The rule that closed this one, and three others in this file.** VC6
// emits an FP add's two operands in REVERSE order, so a single-expression
// sum `a + b + c` comes out c, b, a -- and no permutation of the source
// terms reaches every emission order, because the reversal is applied to
// whatever order you write. An ACCUMULATOR does not reverse: written as
//
//     f  = a;
//     f += b;
//     f += c;
//
// the terms come out a, b, c, in source order, and the value never leaves
// the FPU stack, so the two forms are the same instructions in a different
// order and exactly the same size. The shipped build uses the flat sum in
// ws_ProjectBoxCheap (which emits z, y, x, and matched with `x + y + z`
// written out) and the accumulator here, three lines away. Both spellings
// are in this file for that reason, and neither is a stylistic choice.
// AVP2: the corners are counted up rather than down, so each extreme can
// remember which corner it came from; the two tests are an if/else (a new
// minimum is never also a new maximum); and the projected flag is set before
// the answer (PH 0x0041a725).
//
// The distance is an accumulator in z, x, y order, as shipped multiplies
// (0x0041a6c1..0x0041a6e8): identical in all four engines. DistTo, Dot and
// every flat sum start on x before the z load is stored. The corner pointer
// is taken AFTER the two extreme stores: server.dll 0x10002216..0x10002224
// stores m_fMin/m_fMax before parking it in the dead parameter slot.
int ws_ProjectBox(CBoxProject* pProject)
{
	CBoxProject*     pBox   = pProject;
	LTPlane*         pPlane = pBox->m_pPlane;
	LTVector*        pPoint;
	float            fDist;
	int              i;

	pBox->m_fMin =  WS_FAR;
	pBox->m_fMax = -WS_FAR;

	pPoint = pBox->m_pPoints;

	for (i = 0; i < 8; i++)
	{
		LTVector v = *pPoint;

		fDist  = v.z * pPlane->m_Normal.z;
		fDist += v.x * pPlane->m_Normal.x;
		fDist += v.y * pPlane->m_Normal.y;
		fDist -= pPlane->m_Dist;

		if (fDist < pBox->m_fMin)
		{
			pBox->m_fMin = fDist;
			pBox->m_iMinCorner = i;
		}
		else if (fDist > pBox->m_fMax)
		{
			pBox->m_fMax = fDist;
			pBox->m_iMaxCorner = i;
		}

		pPoint++;
	}

	*pBox->m_pProjected = 1;

	// Member first, constant second: /O2 reloads per comparison and honours
	// source order (`flds 0xc(%ecx); fcomps <slop>` three times).
	if (pBox->m_fMin < WS_PLANE_SLOP && pBox->m_fMax > WS_PLANE_SLOP)
		return WS_STRADDLE;
	else if (pBox->m_fMin > WS_PLANE_SLOP)
		return WS_INFRONT;
	else
		return WS_BEHIND;
}

// The same answer for a plane the box is nowhere near, without touching the
// corners: the box's bounding sphere clears the plane by its radius or it
// does not, and only the undecided case pays for the eight projections.
int ws_ProjectBoxCheap(CBoxProject* pProject)
{
	CSphereQuery* pSphere;
	LTPlane*      pPlane;
	float         fDist;

	g_nPhysMoves++;

	pSphere = pProject->m_pSphere;
	pPlane  = pProject->m_pPlane;

	LTVector v = pSphere->m_vPoint;

	fDist = v.x * pPlane->m_Normal.x + v.y * pPlane->m_Normal.y +
	        v.z * pPlane->m_Normal.z - pPlane->m_Dist;

	if (fDist > pSphere->m_fRadius)
	{
		*pProject->m_pProjected = 0;

		return WS_INFRONT;
	}

	if (fDist < -pSphere->m_fRadius)
	{
		*pProject->m_pProjected = 0;

		return WS_BEHIND;
	}

	return ws_ProjectBox(pProject);
}

// The six axis-aligned faces. A plane whose normal is an axis direction needs
// no projection at all: the interval is the box's own bounds along that axis,
// and for a negative normal the two ends swap and change sign.
//
// All six are an `else if`/`else` chain rather than three guarded returns,
// and that is the whole difference between a match and a size-exact near.
// Three separate `if (...) return X;` statements let our build cross-jump the
// straddle exit's `pop eax; ret` onto the one the front exit already has; the
// shipped build writes both out. Written as one chain, VC6 emits each exit's
// own copy. Nothing else moved it -- both compiler revisions, /Os without
// /Og, /O2, /Og /Ot and reordering the three exits were all tried and all
// left the cross-jump in place.
int ws_ProjectBoxXPos(CBoxProject* pProject)
{
	pProject->m_fMin = pProject->m_vMin.x - pProject->m_pPlane->m_Dist;
	pProject->m_fMax = pProject->m_vMax.x - pProject->m_pPlane->m_Dist;

	if (pProject->m_fMin <= WS_PLANE_SLOP && pProject->m_fMax > WS_PLANE_SLOP)
		return WS_STRADDLE;
	else if (pProject->m_fMin > WS_PLANE_SLOP)
		return WS_INFRONT;
	else
		return WS_BEHIND;
}

int ws_ProjectBoxXNeg(CBoxProject* pProject)
{
	pProject->m_fMin = -pProject->m_vMax.x - pProject->m_pPlane->m_Dist;
	pProject->m_fMax = -pProject->m_vMin.x - pProject->m_pPlane->m_Dist;

	if (pProject->m_fMin <= WS_PLANE_SLOP && pProject->m_fMax > WS_PLANE_SLOP)
		return WS_STRADDLE;
	else if (pProject->m_fMin > WS_PLANE_SLOP)
		return WS_INFRONT;
	else
		return WS_BEHIND;
}

int ws_ProjectBoxYPos(CBoxProject* pProject)
{
	pProject->m_fMin = pProject->m_vMin.y - pProject->m_pPlane->m_Dist;
	pProject->m_fMax = pProject->m_vMax.y - pProject->m_pPlane->m_Dist;

	if (pProject->m_fMin <= WS_PLANE_SLOP && pProject->m_fMax > WS_PLANE_SLOP)
		return WS_STRADDLE;
	else if (pProject->m_fMin > WS_PLANE_SLOP)
		return WS_INFRONT;
	else
		return WS_BEHIND;
}

int ws_ProjectBoxYNeg(CBoxProject* pProject)
{
	pProject->m_fMin = -pProject->m_vMax.y - pProject->m_pPlane->m_Dist;
	pProject->m_fMax = -pProject->m_vMin.y - pProject->m_pPlane->m_Dist;

	if (pProject->m_fMin <= WS_PLANE_SLOP && pProject->m_fMax > WS_PLANE_SLOP)
		return WS_STRADDLE;
	else if (pProject->m_fMin > WS_PLANE_SLOP)
		return WS_INFRONT;
	else
		return WS_BEHIND;
}

int ws_ProjectBoxZPos(CBoxProject* pProject)
{
	pProject->m_fMin = pProject->m_vMin.z - pProject->m_pPlane->m_Dist;
	pProject->m_fMax = pProject->m_vMax.z - pProject->m_pPlane->m_Dist;

	if (pProject->m_fMin <= WS_PLANE_SLOP && pProject->m_fMax > WS_PLANE_SLOP)
		return WS_STRADDLE;
	else if (pProject->m_fMin > WS_PLANE_SLOP)
		return WS_INFRONT;
	else
		return WS_BEHIND;
}

int ws_ProjectBoxZNeg(CBoxProject* pProject)
{
	pProject->m_fMin = -pProject->m_vMax.z - pProject->m_pPlane->m_Dist;
	pProject->m_fMax = -pProject->m_vMin.z - pProject->m_pPlane->m_Dist;

	if (pProject->m_fMin <= WS_PLANE_SLOP && pProject->m_fMax > WS_PLANE_SLOP)
		return WS_STRADDLE;
	else if (pProject->m_fMin > WS_PLANE_SLOP)
		return WS_INFRONT;
	else
		return WS_BEHIND;
}

float g_BoxSigns[6] = { 1.0f, -1.0f, 1.0f, -1.0f, 1.0f, -1.0f };

WSProjectFn g_BoxProjectFns[7] = {
	ws_ProjectBoxXPos, ws_ProjectBoxXNeg,
	ws_ProjectBoxYPos, ws_ProjectBoxYNeg,
	ws_ProjectBoxZPos, ws_ProjectBoxZNeg,
	ws_ProjectBoxCheap
};

// The box has hit a world polygon. Remember it if it is the most upward-
// facing floor seen so far -- that is what the object ends up standing on --
// tell the caller about the impact, then answer the contact either by pushing
// the box clear of the plane or by stopping the move at it, and shift the box
// to wherever that leaves it.
//
// `inline`, defined above its one caller: lithtech.exe's /O2 ws_MoveBox
// (0x00418650, 3189 bytes) carries this body and the client has no copy of
// its own; server.dll's /Os declines it and ws_MoveBox calls 0x10004167.
inline int ws_HitPoly(WorldPoly* pPoly, CBounceState* pState, CPolyTest* pTest)
{
	// AVP2 (PH server.dll 0x10005a58, 500 bytes): the impact is reported
	// against a copy of the plane's normal, which a push-out move with
	// m_n38 set flattens onto the horizontal when the surface is a slope
	// steeper than 45 degrees (0.0 < y < 0.7071, then Norm(1.0)); the
	// contact is always ws_AddContact's, and what moves is the end of the
	// move, by ws_NudgeQ516, before the box is shifted: back along the
	// normal by the end projection's distance for a push-out, otherwise
	// back along the move by the fraction that distance is of the move's
	// approach to the plane (nothing for an approach within FLT_EPSILON).
	//
	// NEAR on server.dll (500 of 500, 2 mnemonic blocks): the dot product's
	// first two terms. Shipped multiplies the spilled y difference first
	// (`flds -0x18(%ebp); fmuls 4(%ebx)`, 0x10005ba6) and then z from the
	// stack; ours takes z first. Every term order and grouping of a written-
	// out sum and both Dot spellings were tried: /O1 canonicalises the sum
	// (all orders give the same code), and vDelta.Dot(N) copies the normal.
	// What is settled: bPushOut is read before the normal is copied (that
	// puts it in the frame, -0x4, rather than %edi), the difference is one
	// Init (three assignments schedule the else arm's multiplies
	// differently), and the plane lookups are the inline owner test, not
	// WorldPoly::GetPlane (which /O1 calls).
	LTPlane* pPlane;
	LTVector vNormal;
	LTBOOL   bPushOut;
	LTVector vPush;

	pPlane   = ws_GetPlaneQ506(pPoly->m_pOwner);
	bPushOut = g_pMoveState->m_bPushOut;
	vNormal  = pPlane->m_Normal;

	if (bPushOut && vNormal.y > 0.0f && vNormal.y < 0.7071f &&
	    g_pMoveState->m_n38)
	{
		vNormal.y = 0.0f;
		vNormal.Norm(1.0f);
	}

	if (vNormal.y > WS_FLOOR_NORMAL_Y)
	{
		if (!g_pMoveResult->m_pStandingPoly ||
		    ws_GetPlaneQ506(g_pMoveResult->m_pStandingPoly->m_pOwner)->m_Normal.y <
		        pPlane->m_Normal.y)
		{
			g_pMoveResult->m_pStandingPoly = pPoly;
		}
	}

	ws_HandleImpact(g_pMoveState->m_pInfo, pState, g_pMoveState->m_pObject,
	                g_pMoveState->m_hObject, g_pMoveState->m_pWorldBsp,
	                pPoly, &vNormal);

	pState->m_nHits++;

	if (*pTest->m_End.m_pProjected == 0)
		ws_ProjectBox(&pTest->m_End);

	if (bPushOut)
	{
		vPush = -pPlane->m_Normal * pTest->m_End.m_fMin;
	}
	else
	{
		LTVector vDelta;
		float    fDenom;

		vDelta.Init(g_vMoveEnd.x - g_vMoveStart.x,
		            g_vMoveEnd.y - g_vMoveStart.y,
		            g_vMoveEnd.z - g_vMoveStart.z);

		fDenom = (pTest->m_End.m_fMin - pPlane->m_Normal.Dot(vDelta)) -
		         pTest->m_End.m_fMin;

		if (fDenom < FLT_EPSILON && fDenom > -FLT_EPSILON)
			vPush.Init(0.0f, 0.0f, 0.0f);
		else
			vPush = vDelta * (pTest->m_End.m_fMin / fDenom);
	}

	ws_NudgeQ516(&g_vMoveEnd, g_vMoveEnd, vPush);

	ws_AddContact(pPlane, pPlane);

	return ws_ShiftBox();
}

// bsp_PointInside and bsp_SphereInside (worldsim.h) are serverlightanim.cpp's:
// both images place them after that file's code (server.dll 0x10017db3 /
// 0x10017e31, lithtech.exe 0x00441570 / 0x00441620).

// 0x100192ec (lithtech.exe: expanded in line at 0x004480d0's polygon test)
// -- the polygon test the line query stops on: the crossing point is on the
// polygon's plane, and this asks whether it is inside the polygon itself.
// NOLF's was packet.cpp's (0x10013ddc); in both AvP2 1.0.9.6 and Primal Hunt
// it is here, with the recursion, ahead of bspq_IntersectLine.
//
// Two rejections before the edge walk: bit 0 of the polygon's surface flags,
// and the bounding sphere at +0x18/+0x24 (the squared radius against the
// squared distance, so nothing takes a root).
//
// The walk itself is the standard convex test. Each edge runs from the
// previous vertex to the current one -- the cursor starts at the *last*
// vertex -- and the plane's normal crossed with the edge gives the inward
// normal of the edge's own plane. The point has to be on the inside of every
// one of them, to within a hundredth of a unit.
//
// `Cross`, `Norm` and `MagSqr` are calls and `Dot` inlines (ltvector.h's,
// by value). The edge plane is a real LTPlane local, and the plane test
// re-reads m_Dist from memory (`fstp -0x50` / `fsub -0x50`): the read goes
// through a volatile float, which is what forces the reload.
//
// INLINE, and defined above the recursion: lithtech.exe (/O2) expands it
// there; server.dll (/O1) declines and keeps the out-of-line copy after it.
inline int w_PointInPoly(WorldPolyOwner* pOwner, LTVector* pPoint)
{
	WorldPolyVertex* pCur;
	WorldPolyVertex* pPrev;
	WorldPolyVertex* pEnd;
	LTPlane*         pPlane;
	LTPlane          edgePlane;
	LTVector         vTemp;

	if (!(pOwner->m_pSurface->m_Flags & 1))
		return 0;

	vTemp = pOwner->m_vCentre - *pPoint;

	if (vTemp.MagSqr() > pOwner->m_fRadius * pOwner->m_fRadius)
	{
		return 0;
	}

	pPlane = pOwner->m_pPlane;

	pEnd  = &pOwner->m_Vertices[pOwner->m_nVertices];
	pCur  = pOwner->m_Vertices;
	pPrev = pEnd - 1;

	while (pCur != pEnd)
	{
		vTemp = *pPrev->m_pPos - *pCur->m_pPos;

		edgePlane.m_Normal = pPlane->m_Normal.Cross(vTemp);

		edgePlane.m_Normal.Norm();

		edgePlane.m_Dist = edgePlane.m_Normal.Dot(*pCur->m_pPos);

		if (edgePlane.m_Normal.Dot(*pPoint) - *(volatile float*)&edgePlane.m_Dist < -0.01f)
			return 0;

		pPrev = pCur;
		pCur++;
	}

	return 1;
}

// The plane-thickness the walk works to, and the fraction past which a miss
// is not worth splitting again (both read out of .rdata: 0x3c23d70a and
// 0x3f7d70a4).
#define BSP_LINE_EPSILON 0.01f
#define BSP_LINE_MAXFRAC 0.99f

// 0x100190e2 (lithtech.exe 0x004480d0; AvP2 server.dll 0x1001854e) -- the
// line query's recursion: the classic BSP segment walk. A segment entirely
// on one side of the node's plane descends that side without splitting, and
// one that straddles it is cut at the crossing and the near half recursed
// into first; the far half against the other child is the loop.
//
// AVP2: the ends come by pointer (NOLF's took them by value, packet.cpp):
// the start is copied into a local of its own at entry, the end is read
// where it is. The node's plane is fetched for each of the two distances
// (a node without a polygon or surface has none), and DistTo's by-value Dot
// is what copies the end onto the frame.
//
// Read off the bytes: the side to take first comes from where the start is,
// except when the start is within the plane's thickness, where it comes from
// the end instead, inverted; the split is skipped when the two distances are
// equal and when the start is on the plane; the polygon test only runs on
// the front-first side (nSide == 1), and the fraction cut-off only when it
// does not.
int bspq_IntersectLineRecQ544(BSPNode* pNode, CLineQuery* pQuery,
                              LTVector* pStart, LTVector* pEnd)
{
	LTVector vStart = *pStart;
	LTVector vMid;
	float    t;
	int      nSide;

	for (;;)
	{
		if (pNode->m_Flags & BSPNODE_LEAF)
			return 0;

		float fStart = ws_GetPlaneQ506(pNode->m_pOwner)->DistTo(vStart);
		float fEnd   = ws_GetPlaneQ506(pNode->m_pOwner)->DistTo(*pEnd);

		if (fStart > BSP_LINE_EPSILON && fEnd > BSP_LINE_EPSILON)
		{
			pNode = pNode->m_pChild[1];

			continue;
		}

		if (fStart < -BSP_LINE_EPSILON && fEnd < -BSP_LINE_EPSILON)
		{
			pNode = pNode->m_pChild[0];

			continue;
		}

		// AvP2 1.0.9.6's server.dll tests the start against -EPSILON a
		// second time here (0x1001861e, the constant still on the FP
		// stack from the test above): the two tests are separate
		// statements, not one nested `if`.
		if (fStart < -BSP_LINE_EPSILON || fStart > BSP_LINE_EPSILON)
		{
			if (fStart > 0.0f)
				nSide = 1;
			else
				nSide = 0;
		}
		else
		{
			if (fEnd < 0.0f)
				nSide = 1;
			else
				nSide = 0;
		}

		float fDelta = fEnd - fStart;

		if (fDelta != 0.0f &&
		    (fStart < -BSP_LINE_EPSILON || fStart > BSP_LINE_EPSILON))
		{
			t = -fStart / fDelta;

			vMid.x = (pEnd->x - vStart.x) * t + vStart.x;
			vMid.y = (pEnd->y - vStart.y) * t + vStart.y;
			vMid.z = (pEnd->z - vStart.z) * t + vStart.z;

			if (bspq_IntersectLineRecQ544(pNode->m_pChild[nSide], pQuery,
			                              &vStart, &vMid))
				return 1;

			if (nSide == 1 && pNode->m_pOwner)
			{
				if (w_PointInPoly(pNode->m_pOwner, &vMid))
				{
					// +0x0c is the caller's IntersectQuery (its filter at
					// 0x2c, its user data at 0x30) and +0x10 the world
					// whose MakeHPoly makes the handle the filter takes;
					// the node stands for its polygon. A null filter, a
					// null function or a null world all mean "accept".
					IntersectQuery* pIQuery = (IntersectQuery*)pQuery->m_Unknown0c;

					if (!pIQuery || !pIQuery->m_PolyFilterFn ||
					    !pQuery->m_Unknown10 ||
					    pIQuery->m_PolyFilterFn(
					        ((IWorldBsp*)pQuery->m_Unknown10)->MakeHPoly(
					            (WorldPoly*)pNode),
					        pIQuery->m_pUserData))
					{
						pQuery->m_pPoly = (WorldPoly*)pNode;

						*pQuery->m_pHit = vMid;

						return 1;
					}
				}
			}
			else if (t > BSP_LINE_MAXFRAC)
			{
				return 0;
			}

			vStart = vMid;
		}

		pNode = pNode->m_pChild[nSide == 0];
	}
}

// The walk's entry point, whose whole job is to unpack the query: the two
// ends and the query go to the recursion as they are.
int bspq_IntersectLine(BSPNode* pRoot, CLineQuery* pQuery)
{
	return bspq_IntersectLineRecQ544(pRoot, pQuery, pQuery->m_pStart,
	                                 pQuery->m_pEnd);
}

// The constructor is `inline`, defined here beside its only user in this
// file (objbank.cpp keeps its own copy of the class and the same body). /O1
// declines to expand it and server.dll calls the COMDAT at 0x1000f08d; /O2
// expands it in lithtech.exe (0x0043c360), where the dead stores to the three
// pointers go and m_pPoly, +0x0c, +0x10 are zeroed in that order.
inline CLineQuery::CLineQuery()
{
	m_pStart    = LTNULL;
	m_pEnd      = LTNULL;
	m_pHit      = LTNULL;
	m_pPoly     = LTNULL;
	m_Unknown0c = 0;
	m_Unknown10 = 0;
}

// The line query, from the outside: build the query object, hand it to the
// walk, and if the walk found something copy the polygon's plane out for the
// caller and answer the polygon. The crossing point itself is written by the
// walk, through the pointer the query carries — this function never touches
// it.
//
// The query is constructed rather than cleared here: the shipped code calls
// the six-store zeroing constructor at 0x1000f08d and only then assigns the
// three pointers, so the two unknown dwords and the result are left at the
// zero the constructor put there.
WorldPoly* bsp_IntersectLine(BSPNode* pRoot, LTVector* pStart, LTVector* pEnd,
                             LTVector* pHit, LTPlane* pPlane)
{
	CLineQuery query;

	query.m_pStart = pStart;
	query.m_pEnd   = pEnd;
	query.m_pHit   = pHit;

	// The success path falls through and the failure return sits at the end
	// with its own `xor eax,eax`: written as `if (!walk) return NULL;` the
	// zero the walk already returned is reused instead, which is two bytes
	// shorter and not what shipped.
	if (bspq_IntersectLine(pRoot, &query))
	{
		// AVP2: the owner and surface tests expanded in place
		// (server.dll 0x1001947e..0x10019492).
		*pPlane = *ws_GetPlaneQ506(query.m_pPoly->m_pOwner);

		return query.m_pPoly;
	}

	return NULL;
}

// Move a point rather than a box. There are no contacts and no box geometry
// to rebuild: the segment is handed to the world, and whatever it hits first
// bounces it and becomes the new end of the segment.
//
// AVP2: one test, not a loop of forty (PH lithtech 0x0041d600): a hit that
// moves the point by more than 0.001 bounces off the polygon's own normal,
// and a floor-like polygon (y above 0.01) becomes what the move stands on
// unless what it already stands on is flatter.
void ws_MovePoint(CMoveState* pMove, CBounceState* pState)
{
	LTVector   vHit;
	LTPlane    planeHit;
	WorldPoly* pPoly;

	pState->m_vPoint = pMove->m_vEnd;

	pPoly = bsp_IntersectLine(pMove->m_pWorldBsp->GetRoot(),
	                          &pMove->m_vStart, &pState->m_vPoint,
	                          &vHit, &planeHit);

	if (pPoly && pState->m_vPoint.DistSqr(vHit) > 0.001f)
	{
		ws_HandleImpact(g_pMoveState->m_pInfo, pState, pMove->m_pObject,
		                g_pMoveState->m_hObject, g_pMoveState->m_pWorldBsp,
		                pPoly, &ws_GetPlaneQ506(pPoly->m_pOwner)->m_Normal);

		pState->m_vPoint = vHit;
		pState->m_nHits++;

		if (ws_GetPlaneQ506(pPoly->m_pOwner)->m_Normal.y > WS_FLOOR_NORMAL_Y)
		{
			if (!pState->m_pStandingPoly ||
			    ws_GetPlaneQ506(pState->m_pStandingPoly->m_pOwner)->m_Normal.y <
			    ws_GetPlaneQ506(pPoly->m_pOwner)->m_Normal.y)
			{
				g_pMoveResult->m_pStandingPoly = pPoly;
			}
		}
	}
}

// Does this box touch the world? Three point tests and one sphere test settle
// almost every case without building any box geometry at all; only a sphere
// that straddles is worth the full walk. The offset is zeroed first because
// the geometry ws_SetupBoxGeometry lays out is absolute, not relative to a
// move in progress.
int ws_GetBoxStatus(BSPNode* pRoot, LTVector* pMin, LTVector* pMax)
{
	int nStatus;

	g_BoxSphere.m_vPoint = *pMax - *pMin;
	g_BoxSphere.m_vPoint *= 0.5f;

	g_BoxSphere.m_fRadius = g_BoxSphere.m_vPoint.Mag() + 0.1f;

	g_BoxSphere.m_vPoint += *pMin;

	if (bsp_PointInside(pRoot, &g_BoxSphere.m_vPoint) &&
	    bsp_PointInside(pRoot, pMin) &&
	    bsp_PointInside(pRoot, pMax))
	{
		nStatus = bsp_SphereInside(&pRoot, &g_BoxSphere.m_vPoint,
		                           g_BoxSphere.m_fRadius);

		if (nStatus == 1)
			return 0;

		if (nStatus)
		{
			// Init, not three stores: the same three `movl $0` either
			// way, but the Init is one more inline expansion, and that is
			// what spends lithtech.exe's /O2 budget exactly where shipped
			// does -- the last two Inits of ws_SetupBoxGeometry below
			// become calls to 0x0041ab50 (0x00417b0c, 0x00417b21), not one.
			g_vMoveBase.Init(0.0f, 0.0f, 0.0f);

			ws_SetupBoxGeometry(pMin, pMax);

			return bspq_TestSphere(pRoot, &g_BoxSphere);
		}
	}

	return 1;
}

// The sweep ran into something. Back its destination out along the surface's
// normal by a little more than it went in, and if the surface is a wall --
// its normal dead level -- reflect the travel along the wall as well, so the
// volume slides rather than stopping dead. Then re-derive the sweep from the
// destination that leaves. A surface too steep to stand on is reported to the
// game; a floor is not, because walking up a slope is not a collision.
void CSimContext::DeflectOffHit(WorldPoly* pPoly, CBounceState* pState)
{
	// AVP2: initialised, not assigned -- the product is built straight into
	// the offset (PH 0x0041bc0c..0x0041bc22), with no temporary copied over.
	LTVector vOffset = m_vHitNormal * -(m_fHitDepth * WS_PUSHOUT_SCALE);

	if (m_vHitNormal.y == 0.0f)
	{
		// Initialised too: server.dll's Cross writes it in place (PH
		// 0x10003190, its result pointer is the side vector's own slot).
		LTVector vSide = m_vDir.Cross(LTVector(0.0f, 1.0f, 0.0f));

		vOffset += vSide * (vSide.Dot(m_vHitNormal) * -2.0f);
	}

	m_vEnd += vOffset;

	// AVP2: the standing-poly report and the impact are separate: any hit
	// polygon is reported to the impact, against the push direction itself
	// normalised (PH 0x0041bd5f..0x0041bdb8).
	if (m_bReport && m_vHitNormal.y < WS_SLOPE_LIMIT)
		g_pMoveResult->m_pStandingPoly = m_pHitPoly;

	if (pPoly)
	{
		LTVector vNormal = vOffset;

		vNormal.Norm();

		ws_HandleImpact(g_pMoveState->m_pInfo, pState,
		                g_pMoveState->m_pObject, g_pMoveState->m_hObject,
		                g_pMoveState->m_pWorldBsp, pPoly, &vNormal);
	}

	Setup();
}

// Two objects have run into each other. Each one's share of the collision is
// the part of its velocity that was heading into the other, turned around;
// the strength the game is told about is the difference between the two
// shares, weighted by the pair's mass. An object only hears about it if it
// asked to and the hit was hard enough to be worth hearing about, and only
// the object with the lower blocking priority actually gets moved -- which is
// what makes a lift push a player rather than the other way round.
//
// Byte-identical on lithtech.exe (0x41a890, 689 bytes). The two impulses and
// the two dot products are each a two-element ARRAY, not a pair of scalars.
// That is what the frames were saying: shipped server (0x100052d0) puts the
// compiler temporaries at the top of the frame and the impulses lowest
// (vImpulse[0] at -0x4c, [1] at -0x40), and its four dwords run pInfo,
// pVel2, fDot[1], fDot[0] from -0x4 down -- the arrays, being the larger
// objects, sit below the scalars. With separate locals ours was the mirror
// image, and lithtech (frame 0x38 against our 0x34) homed fDot1/fDot2 in the
// dead pObj1/pObj2 parameter slots where shipped keeps them in the frame and
// puts pVel2 in pObj1's slot instead; an array element cannot be homed in a
// parameter slot. Both arrays are needed -- either alone leaves lithtech off.
// Naming the first object's velocity in a pointer local *inside* the if that
// adds to it is also part of the shape (shipped forms that address with a
// `lea` there and only there; the dot product at the top reads the member).
// NEAR on server.dll (562 of 562, one instruction scheduled differently): the
// loads of pInfo and fDist for the three pInfo stores come after ONE movsl of
// the vImpulse[1] copy in shipped, after two in ours. Store order of those
// three statements, pVel2's position, declaration order and Dot(*pVel2) do
// not move it.
void ws_ObjectsCollided(IPhysicsSim* pSim, LTObject* pObj1, LTObject* pObj2,
                        LTVector* pNormal, float fDist)
{
	LTVector       vImpulse[2];
	LTVector       vRel;
	LTVector       vNormal2;
	CollisionInfo* pInfo;
	LTVector*      pVel2;
	float          fDot[2];

	pInfo = pSim->GetOwner()->m_pInfo;

	fDot[0] = pObj1->m_Velocity.x * pNormal->x +
	          pObj1->m_Velocity.y * pNormal->y +
	          pObj1->m_Velocity.z * pNormal->z;

	vImpulse[0] = *pNormal * -fDot[0];

	vNormal2 = -(*pNormal);

	pVel2 = &pObj2->m_Velocity;

	fDot[1] = vNormal2.Dot(pObj2->m_Velocity);

	vImpulse[1] = vNormal2 * -fDot[1];

	pInfo->m_Plane.m_Normal = *pNormal;
	pInfo->m_hPoly = 0;
	pInfo->m_Plane.m_Dist = fDist;

	vRel = (vImpulse[0] - vImpulse[1]) *
	       ((pObj1->m_Mass + pObj2->m_Mass) * WS_IMPULSE);

	{
		float fMagSqr = vRel.MagSqr();
		float fMag    = (float)sqrt(fMagSqr);

		if ((pObj1->m_Flags & FLAG_TOUCH_NOTIFY) &&
		    fMagSqr >= pObj1->m_ForceIgnoreLimitSqr)
		{
			pSim->OnBounce(pObj1, pObj2, &vImpulse[0], fMag);
		}

		if ((pObj2->m_Flags & FLAG_TOUCH_NOTIFY) &&
		    fMagSqr >= pObj2->m_ForceIgnoreLimitSqr)
		{
			pSim->OnBounce(pObj2, pObj1, &vImpulse[1], -fMag);
		}
	}

	if (fDot[0] < 0.0f &&
	    pObj1->m_BlockingPriority <= pObj2->m_BlockingPriority)
	{
		LTVector* pVel1 = &pObj1->m_Velocity;

		pObj1->m_ChangeFlags |= CHANGEFLAG_MOTION;

		pVel1->x += vImpulse[0].x;
		pVel1->y += vImpulse[0].y;
		pVel1->z += vImpulse[0].z;
	}

	if (fDot[1] < 0.0f &&
	    pObj2->m_BlockingPriority <= pObj1->m_BlockingPriority)
	{
		pObj2->m_ChangeFlags |= CHANGEFLAG_MOTION;

		pVel2->x += vImpulse[1].x;
		pVel2->y += vImpulse[1].y;
		pVel2->z += vImpulse[1].z;
	}
}

// server.dll 0x1000464b (537 bytes) -- AVP2: every physics block the swept
// box overlaps, for ws_MoveObject (its caller, 0x10004e3b). The box is
// taken into the world model's space through the moving-against object's
// second transform (+0x238) at both corners (MatVMul_H, 0x10004864, its
// result dropped), and the world's block grid (cell size and origin, slots
// 0x14 and 0x18) is walked y, x, z from the cell holding the low corner;
// _ftol truncates (0x1000473e). Stops when nMax blocks are found. A box that
// found none asks for the block at the grid's origin (0x10004837).
// Byte-identical in both images (lithtech.exe 0x0041d750, 1106 bytes). Two
// shapes are load-bearing: the low corner is its own vector (vLo, whose
// slot the walk's vPos then shares) and the cell starts are a vector too
// (only their y stays in st); and the world model is a named pointer
// (`lea 0x238(%eax),%esi`, 0x10004695). Name ours.
void ws_GetPBlocksQ553(CPhysicsBlock** ppBlocks, int* pnBlocks, int nMax)
{
	LTVector  vA;
	LTVector  vB;
	LTVector  vLo;
	LTVector  vPos;
	LTVector  vHi;
	LTVector  vCellSize;
	LTVector  vOrigin;
	LTVector  vStart;
	LTVector  vMin;
	LTVector  vMax;
	WorldModelObject* pObj;
	LTBOOL    bFull;

	ws_SetupSweptBox(&vMin, &vMax);

	vCellSize = *g_pMoveState->m_pWorldBsp->GetCellSize();
	vOrigin   = *g_pMoveState->m_pWorldBsp->GetCellOrigin();

	pObj = (WorldModelObject*)g_pMoveState->m_hObject;

	MatVMul_H(&vA, &pObj->m_Transform230, &vMin);
	MatVMul_H(&vB, &pObj->m_Transform230, &vMax);

	VEC_MIN(vLo, vA, vB);
	VEC_MAX(vHi, vA, vB);

	vStart.x = (float)(int)((vLo.x - vOrigin.x) / vCellSize.x) * vCellSize.x + vOrigin.x;
	vStart.y = (float)(int)((vLo.y - vOrigin.y) / vCellSize.y) * vCellSize.y + vOrigin.y;
	vStart.z = (float)(int)((vLo.z - vOrigin.z) / vCellSize.z) * vCellSize.z + vOrigin.z;

	*pnBlocks = 0;
	bFull = LTFALSE;

	for (vPos.y = vStart.y; vPos.y <= vHi.y && !bFull; vPos.y += vCellSize.y)
	{
		for (vPos.x = vStart.x; vPos.x <= vHi.x && !bFull; vPos.x += vCellSize.x)
		{
			for (vPos.z = vStart.z; vPos.z <= vHi.z && !bFull; vPos.z += vCellSize.z)
			{
				*ppBlocks = g_pMoveState->m_pWorldBsp->GetPBlock(vPos);

				if (*ppBlocks)
				{
					(*pnBlocks)++;
					ppBlocks++;

					if (*pnBlocks >= nMax)
						bFull = LTTRUE;
				}
			}
		}
	}

	if (*pnBlocks == 0)
	{
		*ppBlocks = g_pMoveState->m_pWorldBsp->GetPBlock(vOrigin);

		if (*ppBlocks)
			(*pnBlocks)++;
	}
}

// Everything the move could possibly touch, as one box: the span between
// where the move wanted to go and where it has been pushed to, widened by the
// object's own half-extents at both ends. The bounding sphere gets a metre of
// slack on top, because it is only ever used to reject, never to accept.
// NEAR (server 484 of 485, lithtech 624 of 626): the shipped server spills
// pMax->z to the dead pMax slot (`fsts 0xc(%ebp)`, 0x100037ed) before the
// `+=` and adds max-first; lithtech 0x4182b6 adds max-first too. Changing
// vCentre to `*pMin + vHalf` (or Init, or straight into g_BoxSphere) fixes
// the server's second half and leaves only that spill, but moves lithtech's
// SetupBoxGeometry pushes above the half multiplies (630 bytes), so it is
// not taken. Several spellings of the `+=` make lithtech inline
// SetupBoxGeometry, which shipped does not.
void ws_SetupSweptBox(LTVector* pMin, LTVector* pMax)
{
	LTVector vCentre;
	LTVector vHalf;

	VEC_MIN(*pMin, g_vMoveStart, g_vMoveEnd);
	VEC_MAX(*pMax, g_vMoveStart, g_vMoveEnd);

	*pMax += g_vMoveBase + g_pMoveState->m_vDims;
	*pMin += g_vMoveBase - g_pMoveState->m_vDims;

	vHalf = (*pMax - *pMin) * 0.5f;

	vCentre.x = pMin->x + vHalf.x;
	vCentre.y = pMin->y + vHalf.y;
	vCentre.z = pMin->z + vHalf.z;

	g_BoxSphere.m_vPoint = vCentre;
	g_BoxSphere.m_fRadius = vHalf.Mag() + 1.0f;

	ws_SetupBoxGeometry(pMin, pMax);
}

// The solver has pushed the box somewhere new, so everything derived from
// where it was has to be rebuilt: the sixteen points that bound the sweep,
// the three bounding spheres, and the six clip-plane distances. Answers zero
// and touches nothing when the push came to nothing, which is what stops the
// caller going round again.
//
// NEAR, size-exact on both images: server 0x100022ac (907 bytes, 308
// instructions) and lithtech 0x4165c0 (1049 bytes, 302). Three shapes got it
// there:
//   * `vHalf = vLen * 0.5f; vMid = vP1 + vHalf;` as operators. Written as
//     component stores, VC6 kept vP1.x and vP1.z on the FPU stack across the
//     loop (`fsts`, then `fadd %st(3)`), which also swapped vP1 and vP2's
//     frame slots. Shipped keeps vHalf.x on the stack and never stores it.
//   * the early-out tests vLen, not vDiff: shipped reads x through the copy
//     it has just made (-0x30) and y and z from vLen (-0x44, -0x40).
//   * the loop writes through `pOut = g_SweepPoints[i]`. That anchors the
//     induction pointer at [i][1].z and moves `add $0x60` into the middle of
//     the body, as shipped has it. Reading vBase[i] directly rather than
//     through a pointer local takes lithtech from 20 differing instructions
//     to 12.
// PH: Primal Hunt builds the three sums with Init() (z, y, x; server.dll
// 0x1000251a) and keeps the swept bounds in the step sweep's globals as
// AvP2 does (0x100027ea). With dims through a pointer local in the corner
// loop its server.dll is byte-identical (0x100024f1, 1026 bytes); the
// lithtech builds stay near either way (PH 1260 of 1261, AvP2 1193 of
// 1200, against 1261 / 1200 size-exact with the member form).
// What is left (AvP2): the operand order of the commutative `fadds` in the corner
// sums (shipped loads dims first for x and y on server, and for y and z on
// lithtech), plus one instruction scheduled differently on each image
// (server's `mov $g_SweepPoints+0x14,%edx`, lithtech's `sub $0x3c,%esp`).
// Swapping terms in the source does nothing. `LTVector* pD =
// &g_pMoveState->m_vDims` declared before the vBase pointer fixes the server
// order (4 instructions left) but costs lithtech 5 bytes: shipped lithtech
// addresses dims.y/z off g_pMoveState and dims.x through `lea 0x2c`, which is
// the CSE of the member form used here.
extern LTVector g_vStepSweepMinQ553;
extern LTVector g_vStepSweepMaxQ553;

int ws_ShiftBox()
{
#ifdef LT_SERVER_TIMERS
	CountAdder  cntAdder(&g_nPhysMoveTime);
#endif
	CMoveState* pState;
	LTVector    vBase[2];
	LTVector    vP2;
	LTVector    vP1;
	LTVector    vDiff;
	LTVector    vLen;
	LTVector    vMid;
	LTVector    vHalf;
	uint32      i;

#ifdef PRIMAL_HUNT
	// PH: the three sums are Init()s: each is built z, y, x (the by-value
	// arguments, right to left) and stored x, y, z (server.dll 0x1000251a,
	// 0x100025ab, 0x100025f4). The SDK operators copy the right-hand global
	// first, which shipped does not.
	vDiff.Init(g_vMoveEnd.x - g_vMoveStart.x,
	           g_vMoveEnd.y - g_vMoveStart.y,
	           g_vMoveEnd.z - g_vMoveStart.z);
#else
	vDiff.x = g_vMoveEnd.x - g_vMoveStart.x;
	vDiff.y = g_vMoveEnd.y - g_vMoveStart.y;
	vDiff.z = g_vMoveEnd.z - g_vMoveStart.z;
#endif

	vLen = vDiff;

	if (fabs(vLen.x) < WS_MOVE_EPSILON &&
	    fabs(vLen.y) < WS_MOVE_EPSILON &&
	    fabs(vLen.z) < WS_MOVE_EPSILON)
	{
		return 0;
	}

	pState = g_pMoveState;

#ifdef PRIMAL_HUNT
	vP1.Init(g_vMoveBase.x + g_vMoveStart.x,
	         g_vMoveBase.y + g_vMoveStart.y,
	         g_vMoveBase.z + g_vMoveStart.z);

	vBase[0] = vP1;

	vP2.Init(g_vMoveBase.x + g_vMoveEnd.x,
	         g_vMoveBase.y + g_vMoveEnd.y,
	         g_vMoveBase.z + g_vMoveEnd.z);

	vBase[1] = vP2;
#else
	vP1.x = g_vMoveBase.x + g_vMoveStart.x;
	vP1.y = g_vMoveBase.y + g_vMoveStart.y;
	vP1.z = g_vMoveBase.z + g_vMoveStart.z;

	vBase[0] = vP1;

	vP2.x = g_vMoveBase.x + g_vMoveEnd.x;
	vP2.y = g_vMoveBase.y + g_vMoveEnd.y;
	vP2.z = g_vMoveBase.z + g_vMoveEnd.z;

	vBase[1] = vP2;
#endif

	for (i = 0; i < 2; i++)
	{
		LTVector* pOut  = g_SweepPoints[i];
		LTVector* pDims = &g_pMoveState->m_vDims;

		pOut[0].Init(vBase[i].x - pDims->x,
		             vBase[i].y + pDims->y,
		             vBase[i].z - pDims->z);
		pOut[1].Init(vBase[i].x - pDims->x,
		             vBase[i].y + pDims->y,
		             vBase[i].z + pDims->z);
		pOut[2].Init(vBase[i].x + pDims->x,
		             vBase[i].y + pDims->y,
		             vBase[i].z + pDims->z);
		pOut[3].Init(vBase[i].x + pDims->x,
		             vBase[i].y + pDims->y,
		             vBase[i].z - pDims->z);
		pOut[4].Init(vBase[i].x - pDims->x,
		             vBase[i].y - pDims->y,
		             vBase[i].z - pDims->z);
		pOut[5].Init(vBase[i].x - pDims->x,
		             vBase[i].y - pDims->y,
		             vBase[i].z + pDims->z);
		pOut[6].Init(vBase[i].x + pDims->x,
		             vBase[i].y - pDims->y,
		             vBase[i].z + pDims->z);
		pOut[7].Init(vBase[i].x + pDims->x,
		             vBase[i].y - pDims->y,
		             vBase[i].z - pDims->z);
	}

	g_SphereAtDelta.m_vPoint = vP1;
	g_SphereAtOffset.m_vPoint = vP2;
	g_SphereAtOffset.m_fRadius = g_fBoxRadius;
	g_SphereAtDelta.m_fRadius = g_fBoxRadius;

	vHalf = vLen * 0.5f;

	vMid = vP1 + vHalf;

	g_BoxSphere.m_vPoint = vMid;
	g_BoxSphere.m_fRadius = vLen.Mag() + g_fBoxRadius;

	// AVP2: the swept box's bounds are kept, in the step sweep's globals,
	// for the box move to measure its push-out against (AvP2 1.0.9.6's
	// lithtech.exe 0x00418b1e..0x00418cc5; Primal Hunt's server.dll too,
	// 0x100027ea / 0x10002842). With them both AvP2 images are size-exact
	// (lithtech.exe 1200, server.dll 991), the corner sums' operand order
	// left as above.
	VEC_MIN(g_vStepSweepMinQ553, vP1, vP2);
	VEC_MAX(g_vStepSweepMaxQ553, vP1, vP2);

	g_vStepSweepMinQ553 -= g_pMoveState->m_vDims;
	g_vStepSweepMaxQ553 += g_pMoveState->m_vDims;

	g_BoxPlanes[0].m_Dist =  g_vStepSweepMinQ553.x;
	g_BoxPlanes[1].m_Dist = -g_vStepSweepMaxQ553.x;
	g_BoxPlanes[2].m_Dist =  g_vStepSweepMinQ553.y;
	g_BoxPlanes[3].m_Dist = -g_vStepSweepMaxQ553.y;
	g_BoxPlanes[4].m_Dist =  g_vStepSweepMinQ553.z;
	g_BoxPlanes[5].m_Dist = -g_vStepSweepMaxQ553.z;

	return 1;
}

// AVP2: the move state is 0x4c bytes (`rep movsl` of 0x13 dwords,
// server.dll 0x10004ad5..0x10004b0f): past NOLF's +0x44 it carries a
// pointer to the mover's step offset, which the new stair step reads and
// adjusts (0x10004c63, 0x10004cdb). worldsim.h's CMoveState stops at +0x44
// and two other files include it, so the extra member is a view here (name
// ours).
class CMoveStateQ553 : public CMoveState
{
public:
	float* m_pStepOffset;                    // 0x48
};

// AVP2: the console's "NewCollision" and "NewStairStep" (dsi.cpp's table,
// 0x1007df04 / 0x1007df08): the first sends cylinder movers through the box
// sweep as well, the second picks the stair step below.
extern int g_CV_NewCollisionQ537;
extern int g_CV_NewStairStepQ537;

// AVP2: the swept box the step-up works in, kept in .data rather than the
// mover's frame (0x1008bb68, 0x1008bb90; names ours).
LTVector g_vStepSweepMinQ553;
LTVector g_vStepSweepMaxQ553;

// The box's own six faces as planes. Only the distances ever change --
// ws_SetupBoxGeometry touches nothing but m_Dist -- so the six axis normals
// are written once, by this initialiser (the shipped one: lithtech.exe
// 0x0041a590, server.dll 0x1000213f, the last of this file's). The array is
// at 0x10057fc8 in .data's uninitialised tail, where every dynamically
// initialised global is.
LTPlane  g_BoxPlanes[6] = {
	LTPlane( 1.0f,  0.0f,  0.0f, 0.0f),
	LTPlane(-1.0f,  0.0f,  0.0f, 0.0f),
	LTPlane( 0.0f,  1.0f,  0.0f, 0.0f),
	LTPlane( 0.0f, -1.0f,  0.0f, 0.0f),
	LTPlane( 0.0f,  0.0f,  1.0f, 0.0f),
	LTPlane( 0.0f,  0.0f, -1.0f, 0.0f)
};

// AVP2: every physics block the move overlaps (0x10089b88, 0x200 entries,
// which run exactly up to g_pPBlock at 0x1008a388; name ours).
#define WS_MAX_PBLOCKS 0x200
CPhysicsBlock* g_PBlockListQ553[WS_MAX_PBLOCKS];

void ws_GetPBlocksQ553(CPhysicsBlock** ppBlocks, int* pnBlocks, int nMax);
int  ws_MoveBoxPassesQ544(uint16 nBlock, CBounceState* pState);    // 0x10004f9e

// server.dll 0x100064fa: the "NewStairStep" step-up, which answers through
// its third argument (defined after ws_MoveCylinder, as shipped; name ours).
void ws_StairStepNewQ553(uint16 nBlock, CBounceState* pState, int* pbNoStep);

// The move, from the outside. Everything else in the module runs under this.
// The state it is given is copied, because the module works on one global move
// and a move can start another one before it finishes -- the old pointers are
// put back on the way out, which is what makes the recursion safe.
//
// AVP2: server.dll 0x10004ac9 (1237 bytes) / lithtech.exe 0x0041e070 (1654).
// The result starts at the move's start (0x10004b5d). An object flagged for
// point collision skips all of it. A step-up is tried first when the move
// asks for one, the "NewStairStep" way (the step height clamped between half
// the object's height and its height, half of it taken off the box, and
// ws_StairStepNewQ553 at 0x10004cc1) or NOLF's (ws_StairStep, 0x10004dca).
// Then the box is moved through every physics block it overlaps (0x10004e3b;
// none is an error at debug level 1), up to forty attempts in each, each one
// earned by the last having hit something new -- except a cylinder mover
// under the old collision, which gets one: the hit count is re-read right
// after ws_MoveCylinder (0x10004ef2), so the loop's test always ends it.
//
// Byte-identical on lithtech.exe. NEAR on server.dll (1233 of 1237, 3
// blocks): shipped does not CSE &g_vMoveStart between its copy and the
// GetPBlockAt push (0x10004b7a, 0x10004bb9), and multiplies the re-read step
// offset as `flds; fmulp` (0x10004cf1) where ours folds it into `fmuls`.
// fStepHeight is scoped to the step-up so that the block walk's induction
// pointer can take its slot, pState's dead one (0x10004ea5), as shipped.
void ws_MoveObject(CMoveState* pIn, CBounceState* pState)
{
	CMoveStateQ553  state;
	CMoveStateQ553* pMove = (CMoveStateQ553*)pIn;
	CBounceState*   pOldResult;
	CMoveState*     pOldState;
	int             bStepped = 0;
	int             nTries;
	uint32          nHits;
	uint32          nOldHits;
	int             nBlocks;
	int             iBlock;
	float           fHalfStep;

	g_nPhysStatA8 = 0;
	g_nPhysMoveTime = 0;
	g_nPhysStatA4 = 0;
	g_nPhysContactTime = 0;
	g_nPhysStepTime = 0;
	g_nPhysMoves = 0;
	g_nPhysNodes = 0;

	state = *pMove;

	pOldState     = g_pMoveState;
	g_pMoveState  = &state;
	pOldResult    = g_pMoveResult;
	g_pMoveResult = pState;

	pState->m_pStandingPoly = NULL;
	pState->m_vMomentum.Init();
	pState->m_vVelocityAdd.Init();
	pState->m_nHits = 0;

	pState->m_vPoint = pIn->m_vStart;

	if (pIn->m_pObject->m_Flags & FLAG_POINTCOLLIDE)
	{
		ws_MovePoint(pIn, pState);

		g_pMoveState  = pOldState;
		g_pMoveResult = pOldResult;

		return;
	}

	g_nContacts = 0;

	g_vMoveStart = pIn->m_vStart;
	g_vMoveEnd   = pIn->m_vEnd;

	g_vMoveBase.x = 0.0f;
	g_vMoveBase.y = 0.0f;
	g_vMoveBase.z = 0.0f;

	if (pIn->m_n38)
	{
		float fStepHeight;

		g_pPBlock = ((CObjWorldModelPBlockQ532*)g_pMoveState->m_hObject)->
		    GetPBlockAt(&g_vMoveStart);

		if (g_CV_NewStairStepQ537)
		{
			LTVector vStart;
			int      bStepUp;
			float    fOffset;

			pIn->m_pHost->GetStepper()->GetStepHeight(&fStepHeight);

			fStepHeight = LTMIN(pIn->m_vDims.y,
			                    LTMAX(pIn->m_vDims.y * 0.5f, fStepHeight));

			fHalfStep = fStepHeight * 0.5f;

			g_vMoveBase.y = -(pIn->m_vDims.y - fHalfStep);

			g_pMoveState->m_vDims = pIn->m_vDims;
			g_pMoveState->m_vDims.y = fHalfStep;

			fOffset = *pMove->m_pStepOffset * 0.5f;

			g_vMoveBase.y += fOffset;
			g_pMoveState->m_vDims.y -= fOffset;

			bStepUp = 0;

			vStart = g_vMoveStart;

			if (g_pMoveState->m_vDims.y > 0.0f)
			{
				ws_SetupSweptBox(&g_vStepSweepMinQ553, &g_vStepSweepMaxQ553);

				ws_StairStepNewQ553(g_pPBlock->m_nIndex, pState, &bStepUp);
			}

			*pMove->m_pStepOffset += g_vMoveStart.y - vStart.y;

			fOffset = *pMove->m_pStepOffset * 0.5f;

			g_vMoveStart = vStart;

			if (bStepUp)
			{
				g_vMoveBase.y = fOffset;

				g_pMoveState->m_vDims = pIn->m_vDims;
				g_pMoveState->m_vDims.y -= fOffset;
			}
			else
			{
				g_vMoveBase.y = fHalfStep;

				g_pMoveState->m_vDims = pIn->m_vDims;
				g_pMoveState->m_vDims.y -= fHalfStep;
			}
		}
		else
		{
			pIn->m_pHost->GetStepper()->GetStepHeight(&fStepHeight);

			if (fStepHeight < 0.0f)
				fStepHeight = pIn->m_vDims.y * 0.5f;

			fStepHeight = (fStepHeight + 0.5f) * 0.5f;

			g_vMoveBase.y = -(pIn->m_vDims.y - fStepHeight);

			g_pMoveState->m_vDims = pIn->m_vDims;
			g_pMoveState->m_vDims.y = fStepHeight;

			nOldHits = pState->m_nHits;

			ws_SetupSweptBox(&g_vStepSweepMinQ553, &g_vStepSweepMaxQ553);

			bStepped = ws_StairStep(g_pPBlock->m_nIndex, pState);

			g_pMoveState->m_vDims = pIn->m_vDims;

			if (bStepped)
			{
				g_vMoveBase.y = 0.0f;
			}
			else
			{
				g_vMoveBase.y = fStepHeight;

				g_pMoveState->m_vDims.y -= fStepHeight;
			}

			if (pState->m_nHits != nOldHits && !g_pMoveState->m_n44)
				bStepped = 1;
			else
				bStepped = 0;

			pState->m_nHits = nOldHits;
		}
	}

	ws_GetPBlocksQ553(g_PBlockListQ553, &nBlocks, WS_MAX_PBLOCKS);

	if (nBlocks == 0)
	{
		if (g_DebugLevel >= 1)
		{
			dsi_ServerError("Did not intersect with WorldModel %s PBlocks!",
			                g_pMoveState->m_pWorldBsp->m_Name);
		}

		g_pMoveState  = pOldState;
		g_pMoveResult = pOldResult;

		return;
	}

	g_fBoxRadius = g_pMoveState->m_vDims.Mag() + 0.01f;

	for (iBlock = 0; iBlock < nBlocks; iBlock++)
	{
		g_pPBlock = g_PBlockListQ553[iBlock];

		for (nTries = 0; nTries < WS_MAX_MOVE_STEPS; nTries++)
		{
			nHits = pState->m_nHits;

			if (ws_ShiftBox())
			{
				// AVP2: NOLF's FLAG2_PLAYERCOLLIDE is AvP2's
				// FLAG2_CYLINDERPHYSICS, the same bit (1<<4).
				if (!g_CV_NewCollisionQ537 &&
				    (g_pMoveState->m_pObject->m_Flags2 & FLAG2_CYLINDERPHYSICS))
				{
					ws_MoveCylinder(g_pPBlock->m_nIndex, pState, nTries > 0);
					nHits = pState->m_nHits;
				}
				else
					ws_MoveBoxPassesQ544(g_pPBlock->m_nIndex, pState);
			}

			if (nHits == pState->m_nHits)
				break;
		}

		if (nTries == WS_MAX_MOVE_STEPS)
		{
			if (g_DebugLevel > 0)
			{
				dsi_ServerError("Error: physics recursed infinitely on a %s",
				                g_pMoveState->m_pHost->GetObjectName(
				                    pIn->m_pObject));
			}

			g_vMoveEnd = g_vMoveStart;
		}
	}

	pState->m_vPoint = g_vMoveEnd;

	g_pMoveState  = pOldState;
	g_pMoveResult = pOldResult;

	if (!g_CV_NewStairStepQ537 && pState->m_nHits == 0 && bStepped)
		pState->m_nHits = 1;
}

// Does the swept cylinder reach this polygon? The cheap rejections come
// first: a plane the sweep is moving away from cannot be hit, and neither can
// a polygon whose bounding sphere is out of reach. What follows is the
// expensive part -- walk the polygon's edges, find the point on its boundary
// closest to where the cylinder ends up, and decide whether that point is
// inside the polygon or on one of its edges. A face hit is answered with the
// plane's own normal; an edge hit with the direction from the edge back to
// the cylinder's axis, which is what makes a cylinder slide round a corner
// instead of catching on it.
//
// NEAR (1378 of 1388 bytes, 509 instructions against 512, 3 mnemonic
// blocks; see the final changes at the end of this comment). Things settled and
// worth not re-deriving:
//
//   * the first comparison really is against a *double*. 0x1004d228 reads
//     0x3f847ae147ae147b, which is 0.01 exactly, and the shipped instruction
//     is `fcompl`; so `m_vDir.Dot(...) > 0.01` with no `f` suffix is right,
//     and "fixing" it to 0.01f gives `fcomps` and is wrong.
//   * both `fabs` comparisons are the other way round: the shipped code
//     calls fabs and then compares with `fcomps` against a float constant,
//     which C's own promotion rules cannot give (fabs returns double, so the
//     float would widen). They need an explicit `(float)` on the fabs.
//
// Four more source facts read off the shipped bytes:
//
//   * the vertical clamp of the polygon centre is NOT `LTCLAMP`. The shipped
//     build reloads the centre for each comparison (`flds; fcomps m_fMin`,
//     then again against m_fMax) and moves the selected float into the
//     destination as *integer* bits through %eax with one shared store —
//     which is three separate assignments in an if/else-if/else, not one
//     assignment whose value is a conditional expression. The macro form put
//     both operands on the FPU stack (`flds; fcompp`) and inverted every
//     branch with them. Note this is the opposite way round from
//     CSimContext::Setup's extents, which did want the macro.
//   * the bounding-sphere reject names its radius sum. `m_fReach +
//     pPoly->m_fRadius` written twice inside the comparison leaves the load
//     after the m_vMid copy; a named local puts the `flds` before it, where
//     the shipped code has it. (The order of the three squared differences
//     is not a source fact: writing the sum x,y,z and y,z,x emits the
//     identical instructions.)
//   * one `Mag()` call, not two. The shipped `fcoms` against fClosest does
//     not pop, because the taken arm stores the same value it just compared;
//     `if (vDelta.Mag() < fClosest) fClosest = vDelta.Mag();` is two calls
//     and a `fcomps`.
//   * the two edge slopes are locals, and the drop is scoped to its clamp.
//     `vEdge.x / vEdge.y` and `vEdge.z / vEdge.y` are computed once above
//     both clamps and stay on the FPU stack across them, which is what the
//     two `fmul %st(3),%st` at the same stack index mean; and the drop is a
//     block-local of each clamp, so it lives at st(0) (`fld %st(0)` to
//     duplicate it) instead of being spilled to the function-scope fDrop.
//
// The two early rejects are nested positive guards around the
// whole body, with one `return 0` after it. lithtech.exe's copy (0x00416de0,
// really 0x00416de0..0x004176b5, 2261 bytes; the enumerated 143 is an extent
// error) takes both `je` straight to one shared `xor %eax,%eax` epilogue at
// 0x004176a6, and ours had copied the epilogue into each reject. Measured:
// lithtech 2174 bytes / 608 instructions -> 2270 / 629 (shipped 2261 / 631);
// server 23 mnemonic blocks -> 20 (1377 / 504 against 1388 / 512 before and
// after). What remains on lithtech is register allocation: shipped pushes
// %ebp on entry and keeps pPoly in it, where ours shrink-wraps %ebp and uses
// %ebx. There is also FP scheduling through the vertex loop.
//
// The final changes (server 1392/506, 18 blocks -> 1378/509, 3 blocks; lithtech
// 62 blocks -> 24). Each item is read off the shipped bytes:
//   * the edge test is an LTPlane. `fsts -0x80(%ebp)` at 0x10002d2f
//     stores fEdgeDist although nothing reads it back, which a float local
//     would not do. -0x8c..-0x80 is one 16-byte object, so the frame is 0xb0.
//   * the sphere reject is `vD = pPoly->m_vCentre - m_vMid`. The by-value
//     copy of m_vMid is the movsl at 0x1000299b, and the differences load
//     x,y,z.
//   * the vertex loop indexes `m_Vertices[i]` and has no pointer local. The
//     pointer at -0x34 is VC6's strength-reduced induction variable. It is
//     set up after the guard (0x10002af7) and stepped after `i++`
//     (0x10002d75).
//   * three fld/fstp copies are Init(): vFlat (0x10002b67), vOn = vCur
//     (0x10002bf4), and both m_vHitNormal arms (0x10002e0f, whose right-to-left
//     arguments compute .z first, and 0x10002e38).
// Left: `fsubs 0x1c(%esi)` (fDrop) is scheduled one compare earlier in the
// shipped build. The shipped build also keeps two copies of the slope pops
// after the second clamp (0x10002c8f/0x10002c95) where ours merges them.
// Plus frame-slot rotation: vPrev/vFlat, i/bStepUp/the induction pointer,
// and the cross temp against vFlatDelta. bFlatPlane-first did not move the
// fsubs, and neither did function-scope slopes or an empty else.
int CSimContext::TestPoly(WorldPolyOwner* pPoly, WorldPoly* pHandle)
{
	float     fClosest;
	LTVector  vOn;
	LTVector  vClosest;
	LTPlane*  pPlane;
	int       bInsidePoly;
	LTVector  vCur;
	LTVector  vEdge;
	int       bStepUp;
	uint32    i;
	LTVector  vPrev;
	LTVector  vOnPlane;
	int       bFlatPlane;
	LTVector  vDelta;
	LTPlane   edgePlane;
	float     fFlatDist;
	int       nPrev, nCur;
	float     fT, fMag, fInv, fDrop;
	LTVector  vFlatDelta;

	pPlane = pPoly->m_pPlane;

	if (!(m_vDir.Dot(pPlane->m_Normal) > 0.01))
	{
		float    fReach = m_fReach + pPoly->m_fRadius;
		LTVector vD     = pPoly->m_vCentre - m_vMid;

		if (!(vD.x * vD.x + vD.y * vD.y + vD.z * vD.z > fReach * fReach))
		{

	{
		LTVector vPoint;
		LTVector vScaled = pPlane->m_Normal * -m_fPlaneDist;

		vPoint.x = vScaled.x + m_vEnd.x;
		vPoint.y = vScaled.y + m_vEnd.y;
		vPoint.z = vScaled.z + m_vEnd.z;

		vOnPlane = vPoint;
	}

	if ((float)fabs(pPlane->m_Normal.y) < 0.01f)
	{
		if (pPoly->m_vCentre.y < m_fMin)
			vOnPlane.y = m_fMin;
		else if (pPoly->m_vCentre.y > m_fMax)
			vOnPlane.y = m_fMax;
		else
			vOnPlane.y = pPoly->m_vCentre.y;
	}

	bInsidePoly = 1;
	bFlatPlane = 0;
	bStepUp = 0;

	if ((float)fabs(pPlane->m_Normal.y) >= 0.9f)
	{
		fDrop = m_vEnd.y - pPoly->m_vCentre.y;

		bFlatPlane = 1;

		if (m_bReport && pPlane->m_Normal.y > 0.0f && fDrop >= 0.0f &&
		    m_fHeight - fDrop < m_fHeight * 0.5f)
		{
			bStepUp = 1;
		}
		else
		{
			bStepUp = 0;
		}
	}

	vPrev    = *pPoly->m_Vertices[pPoly->m_nVertices - 1].m_pPos;
	vClosest = *pPoly->m_Vertices[pPoly->m_nVertices - 1].m_pPos;

	fClosest = m_fCornerDist;

	for (i = 0; i < pPoly->m_nVertices; i++)
	{
		vCur = *pPoly->m_Vertices[i].m_pPos;

		{
			LTVector vTmp;

			vTmp.x = vCur.x - vPrev.x;
			vTmp.y = vCur.y - vPrev.y;
			vTmp.z = vCur.z - vPrev.z;

			vEdge = vTmp;
		}

		nPrev = ClassifyAxis(vPrev.y);
		nCur  = ClassifyAxis(vCur.y);

		if (nPrev != nCur || nPrev == WS_INSIDE)
		{
			LTVector vFlat;

			vFlat.Init(vEdge.x, 0.0f, vEdge.z);

			fMag = vFlat.Mag();

			if (fMag > 0.01f)
			{
				fInv = 1.0f / fMag;

				fT = ((m_vEnd.z - vPrev.z) * (vFlat.z * fInv) +
				      (m_vEnd.x - vPrev.x) * (vFlat.x * fInv)) * fInv;
			}
			else
			{
				fT = 1.01f;
			}

			if (fT >= 0.0f && fT <= 1.0f)
			{
				vOn.x = fT * vEdge.x + vPrev.x;
				vOn.y = fT * vEdge.y + vPrev.y;
				vOn.z = fT * vFlat.z + vPrev.z;
			}
			else
			{
				vOn.Init(vCur.x, vCur.y, vCur.z);
			}

			if (vEdge.y != 0.0f && (nPrev || nCur))
			{
				float fSlopeX, fSlopeZ;

				vEdge.Norm();

				fSlopeX = vEdge.x / vEdge.y;
				fSlopeZ = vEdge.z / vEdge.y;

				if (vOn.y < m_fMin)
				{
					float fLift = m_fMin - vOn.y;

					vOn.y = m_fMin;

					vOn.x += fLift * fSlopeX;
					vOn.z += fLift * fSlopeZ;
				}

				if (vOn.y > m_fHeight + m_vEnd.y)
				{
					float fLift = m_fMax - vOn.y;

					vOn.y = m_fMax;

					vOn.x += fLift * fSlopeX;
					vOn.z += fLift * fSlopeZ;
				}
			}

			vDelta.x = vOn.x - m_vEnd.x;
			vDelta.y = 0.0f;
			vDelta.z = vOn.z - m_vEnd.z;

			{
				float fDist = vDelta.Mag();

				if (fDist < fClosest)
				{
					fClosest = fDist;
					vClosest = vOn;
				}
			}
		}

		if (bInsidePoly)
		{
			edgePlane.m_Normal = vEdge.Cross(pPlane->m_Normal);
			edgePlane.m_Normal.Norm();

			edgePlane.m_Dist = edgePlane.m_Normal.Dot(vCur);

			// `< -0.01f`, NOT `>= -0.01f`. 0x10002d5e is `jae 0x10002d64`
			// jumping OVER the `and [ebp-0x24],0x0` that clears the flag,
			// so the clear happens on the BELOW branch. This is a semantic
			// fault in the reconstruction, not a codegen shape -- the point
			// was being rejected as outside the polygon in exactly the cases
			// the shipped engine accepts it.
			if (edgePlane.m_Normal.Dot(vOnPlane) - edgePlane.m_Dist < -0.01f)
				bInsidePoly = 0;
		}

		vPrev = vCur;
	}

	vDelta.x = vOnPlane.x - m_vEnd.x;
	vDelta.y = 0.0f;
	vDelta.z = vOnPlane.z - m_vEnd.z;

	vFlatDelta = vDelta;

	fFlatDist = vFlatDelta.Mag();

	// AVP2: the flat-distance test is spelled positively: a NaN distance
	// takes the plane arm (AvP2 1.0.9.6's lithtech.exe 0x00419758 `and
	// $0x100; jne`), which cl 9044 keeps apart from `!(... >)`.
	if (fClosest < m_fRadius &&
	    (!bInsidePoly || bFlatPlane || fClosest <= fFlatDist) && !bStepUp)
	{
		// AVP2: negated, as the overlap test below -- a NaN depth answers a
		// hit. cl 9044 keeps NaN apart where 8966 does not (AvP2 1.0.9.6's
		// lithtech.exe 0x0041977c `and $0x4100; jne`).
		if (!(m_fRadius - fClosest > m_fHitDepth))
			return 1;

		m_vHitPoint = vClosest;

		if (fClosest != 0.0f)
		{
			m_vHitNormal.Init(vClosest.x - m_vEnd.x, 0.0f,
			                  vClosest.z - m_vEnd.z);

			m_vHitNormal.Norm();
		}
		else
		{
			m_vHitNormal.Init(0.0f, 0.0f, 0.0f);
		}

		m_fHitDepth = m_fRadius - fClosest;
	}
	else
	{
		if (!bInsidePoly)
			return 0;

		if (!(m_fOverlap > m_fHitDepth))
			return 1;

		m_vHitPoint = vOnPlane;

		m_vHitNormal = -(pPlane->m_Normal);

		m_fHitDepth = m_fOverlap;
	}

	m_pHitPoly = pHandle;

	return 1;
		}
	}

	return 0;
}

// AVP2: the physics-block walks share one node stack in .bss rather than
// each keeping 0x800 bytes of frame: server.dll 0x1008a3a0 (ws_StairStep
// 0x10006b73, ws_MoveCylinder 0x100061f3, ws_MoveBox 0x1000517e, and the
// sphere walk's overflow test at 0x10003b6b, which bounds it at 0x800
// bytes), lithtech.exe 0x004f1e20 (0x004205e7). AvP2 1.0.9.6 agrees
// (server.dll 0x1007d1d8). (name ours)
uint16 g_BlockStackQ534[WS_NODE_STACK];

// March the cylinder along its path a step at a time, testing the world's
// tree at each step. The step length is the cylinder's own size along the
// direction of travel, so it can never pass through anything thinner than
// itself; when the move is shorter than one step it is done in one. Hitting
// something spends one of three deflections: the destination is backed out of
// whatever was hit and the same step is tried again. Run out of deflections
// with the object still stuck and the whole move is abandoned back to where
// it started.
//
// AVP2: Primal Hunt's copy is server.dll 0x10005ebc (1598
// bytes, called from ws_MoveObject at 0x10004eed) and lithtech.exe
// 0x0041fdb0 (its enumeration splits it at 0x00420062). 0x100064fa, which
// progress used to pair this with, is the "NewStairStep" step-up below
// (ws_StairStepNewQ553). AvP2 1.0.9.6's copy is server.dll 0x10005ea0
// (1589 bytes) / lithtech.exe 0x0041da50 (2110).
//
// NEAR on all four images, closer this round (AvP2 server.dll 1498 -> 1575
// of 1589 and 19 -> 4 blocks, PH server.dll 1535 -> 1590 of 1598, PH
// lithtech.exe 2035 -> 2049 of 2071, AvP2 lithtech.exe 2067 -> 2079 of
// 2110). Read off AvP2's server.dll:
//  * the first ClassifyPlane looks the plane up again (0x10006279) rather
//    than taking the sphere test's pPlane;
//  * the two descend copies spell their dot products differently -- copy
//    #2 reads z before y (0x10006357), copy #1 y first (0x10006306) -- so
//    VC6 keeps both TestPoly calls and shares only the x term;
//  * vFrom, vTo, vDelta and the three step scales are Init()s, and the
//    step ends are `vAt + vDir * f`: AvP2's cl 9044 then rereads the
//    globals for each component (0x10005f4b..) and keeps the scale's x and
//    y on the FPU stack for the add (0x10006120..).
// Left on AvP2's server.dll: the short step's add loads vAt first
// (`flds; fadd %st(2)`, 0x10006138) where ours loads the scale. PH's builds
// (cl 8966) also compute fFlat / radius before fDiag (PH server.dll
// 0x1000609c), and
// rebuild the start on a refused deflect z first.
// The notes below are NOLF's, measured against its own copy.
//
// NEAR on both images: server 1472 of 1472 bytes with 2 mnemonic blocks
// left, lithtech 1987 of 1983 bytes with the same 502 instructions and 17
// blocks left. Each fix below is something the shipped bytes show:
//  * LTMIN(x, z), not (z, x): x is loaded first (0x10004768, 0x00419972).
//  * the CountAdder is built after the three counter stores (0x10004735).
//  * the tree walk reuses nBlock as its node, as ws_StairStep does. The
//    copy into nRoot (0x1000471c, 0x0041993e) then leaves the parameter
//    dead, so VC6 puts fRemaining in its slot (`fsts 0x8(%ebp)`).
//  * vFlat.Init(...): its components go through the FPU (0x10004851).
//  * fDiag = height*2 is set before the test and kept in st across it. The
//    radius arm is the fall-through (0x100048d0, 0x00419b3d).
//  * the second ClassifyPlane's plane is a statement of its own, loaded
//    into a register before the flag (0x00419ddb).
//  * the STRADDLE arm is the `if` and the child arm the `else`
//    (0x00419e10 jne to 0x00419e9f).
//  * the dot products are written z,y,x. VC6 evaluates the first two terms
//    swapped, and the bytes read y first (0x00419d6f, 0x10004a92).
//  * each step size has its own scaled vector, and VC6 packs them into dead
//    slots (-0x5c, -0x40, -0x88 on the server image).
//  * the destination goes into sim.m_vDest, CSimContext +0x18
//    (0x10004954), not into a local.
//  * `vAt = vTmp` comes before the fRemaining update. Otherwise its three
//    movsl cross-jump with the pre-loop copy.
// Left: on the server, VC6 keeps g_vMoveBase.x in st from vFrom.x to vTo.x
// where shipped reloads it. Six spellings of that code (term order, Init,
// VEC_ADD, operator+, statement order) give the same output. On lithtech,
// the frame slot sharing is different (shipped: fRemaining 0x8, fAbsY/
// fStepShort 0xc, fFlat/fStepLong 0x10, fSide 0x50) and fDiag gets a slot
// where shipped keeps it in st. That is FP scheduling and slot assignment
// through the whole frame.
int ws_MoveCylinder(uint16 nBlock, CBounceState* pState, int bRetry)
{
	CSimContext   sim;
	LTVector      vNext;
	LTVector      vStep;
	LTVector      vScaled;
	LTVector      vScaledA;
	LTVector      vScaledB;
	LTVector      vFlat;
	uint16        nRoot;
	WorldPolyRef* pRecs;
	LTVector      vDelta;
	uint32        nHitsAtStart;
	LTVector      vTo;
	LTVector      vFrom;
	uint32        nHitsLast;
	LTVector      vTmp;
	LTVector      vAt;
	int           nDeflects;
	LTVector      vDir;
	float         fStepShort;
	float         fStepLong;
	float         fSide;

	uint16*       pTop;
	WSNode*       pNode;
	WorldPolyRef* pRec;
	LTPlane*      pPlane;
	int           nStart, nEnd, nSide;
	float         fDist, fRemaining, fFlat, fAbsY, fDiag;

	nRoot = nBlock;

	nHitsAtStart = pState->m_nHits;
	nHitsLast    = pState->m_nHits;

	nDeflects = 3;

#ifdef LT_SERVER_TIMERS
	CountAdder    cntAdder(&g_nPhysStepTime);
#endif

	pRecs = g_pMoveState->m_pWorldBsp->GetPolyRefs();

	sim.m_bReport = 0;

	sim.m_fRadius = LTMIN(g_pMoveState->m_vDims.x, g_pMoveState->m_vDims.z);
	sim.m_fHeight = g_pMoveState->m_vDims.y;

	// FIX (a): four nested positive guards, one shared `return 1` below.
	// 0x004199ac / 0x004199c4 / 0x00419aab / 0x00419bc1 all jne 0x0041a0d1.
	// AVP2: each guard is the negated failure test. cl 8966 compiles the two
	// alike; 9044 keeps NaN apart, and AvP2 1.0.9.6's server.dll takes the
	// guarded path on an unordered compare (0x10005f31 / 0x10005f42 `test
	// $0x5; jnp`, 0x100060ed `test $0x41; jnp`, and the loop's 0x1000643d
	// `test $0x41; je`, as the size test at 0x10006061).
	if (!(sim.m_fRadius < 0.01f))
	{
	if (!(sim.m_fHeight < 0.01f))
	{
		vFrom.Init(g_vMoveBase.x + g_vMoveStart.x,
		           g_vMoveBase.y + g_vMoveStart.y,
		           g_vMoveBase.z + g_vMoveStart.z);

		vAt = vFrom;

		vTo.Init(g_vMoveBase.x + g_vMoveEnd.x,
		         g_vMoveBase.y + g_vMoveEnd.y,
		         g_vMoveBase.z + g_vMoveEnd.z);

		vDelta.Init(vTo.x - vFrom.x, vTo.y - vFrom.y, vTo.z - vFrom.z);

		vDir = vDelta;

		fRemaining = vDir.Mag();

	if (fRemaining != 0.0f)
	{
		vFlat.Init(vDir.x, 0.0f, vDir.z);

		fFlat = vFlat.Mag();

		fAbsY = (float)fabs(vDir.y);

		fSide = 1.0f / fRemaining;

		vDir.x = fSide * vDir.x;
		vDir.y = fSide * vDir.y;
		vDir.z = fSide * vDir.z;

		if (!(fAbsY > sim.m_fHeight * 1.8f) && !(sim.m_fRadius * 0.9f < fFlat))
		{
			fStepShort = fRemaining;
			fStepLong  = fRemaining;
		}
		else
		{
			fDiag = sim.m_fHeight * 2.0f;

			if (fFlat / sim.m_fRadius > fAbsY / fDiag)
			{
				fSide = sim.m_fRadius;
				fDiag = (sim.m_fRadius / fFlat) * fAbsY;
			}
			else
			{
				fSide = (fDiag / fAbsY) * fFlat;
			}

			fDiag = (float)sqrt(fDiag * fDiag + fSide * fSide);

			fStepShort = fDiag * 0.75f;
			fStepLong  = fDiag * 0.9f;
		}

	if (!(fStepShort <= 0.001f))
	{
		sim.m_vDest = vTo;

		for (;;)
		{
			if (!(fRemaining > 0.0f))
				break;

			if (nHitsAtStart == pState->m_nHits)
			{
				if (fRemaining < fStepShort)
				{
					vStep = vAt + vDir * fRemaining;

					vNext = vStep;
				}
				else
				{
					vTo = vAt + vDir * fStepLong;

					vNext = vTo;
				}

				sim.m_vStart = vAt;
				sim.m_vEnd   = vNext;

				sim.Setup();
			}

			nBlock = nRoot;
			pTop  = g_BlockStackQ534;

			for (;;)
			{
				if (nBlock == WS_NODE_EMPTY || nBlock == WS_NODE_SOLID)
				{
					if (pTop == g_BlockStackQ534)
						break;

					pTop--;
					nBlock = *pTop;
				}

				g_nPhysNodes++;

				pNode  = &g_pPBlock->m_pNodes[nBlock];
				pRec   = &pRecs[pNode->m_nPoly];
				pPlane = ws_GetPlaneQ506(pRec->m_pOwner);

				fDist = sim.m_vMid.z * pPlane->m_Normal.z +
				        sim.m_vMid.y * pPlane->m_Normal.y +
				        sim.m_vMid.x * pPlane->m_Normal.x - pPlane->m_Dist;

				// Written as an if/else chain rather than as guarded
				// `continue`s: all three arms end by descending, and the
				// shipped build keeps the descend at the *first* arm and
				// jumps back into it.  See bsp_SphereInside for the same
				// choice in a smaller function.
				if (fDist > sim.m_fReach)
				{
					nBlock = pNode->m_nChild[1];
				}
				else if (fDist < -sim.m_fReach)
				{
					nBlock = pNode->m_nChild[0];
				}
				else
				{
					// The plane is looked up afresh for each classification,
					// not reused from the sphere test (0x10006279,
					// 0x100062a1).
					nStart = sim.ClassifyPlane(ws_GetPlaneQ506(pRec->m_pOwner),
					                           &sim.m_vStart, 0, 1);

					pPlane = ws_GetPlaneQ506(pRec->m_pOwner);

					nEnd = sim.ClassifyPlane(pPlane, &sim.m_vEnd, 1,
					                         nStart != 1);

					// FIX (b): TWO arms, each with its own TestPoly call and
					// its own written-out descend.  0x00419e05 cmp/jne to
					// 0x00419ea9; 0x00419e0d cmp 0x2; 0x00419ea9 cmp 0x1;
					// TestPoly at 0x00419e21 AND 0x00419eb9.
					if (nStart == nEnd)
					{
						if (nStart == WS_STRADDLE)
						{
							if (sim.TestPoly(pRec->m_pOwner,
							                 (WorldPoly*)pRec))
							{
								pState->m_nHits++;
							}

							// descend copy #1 -- 0x00419e34..0x00419e9a
							pPlane = ws_GetPlaneQ506(pRec->m_pOwner);

							fDist = sim.m_vStart.z * pPlane->m_Normal.z +
							        sim.m_vStart.y * pPlane->m_Normal.y +
							        sim.m_vStart.x * pPlane->m_Normal.x -
							        pPlane->m_Dist;

							if (fDist > WS_PLANE_SLOP)
								nSide = 1;
							else
								nSide = 0;

							if (pNode->m_nChild[nSide == 0] !=
							        WS_NODE_EMPTY &&
							    pNode->m_nChild[nSide == 0] !=
							        WS_NODE_SOLID)
							{
								*pTop = pNode->m_nChild[nSide == 0];
								pTop++;
							}

							nBlock = pNode->m_nChild[nSide];
						}
						else
						{
							// 0x00419e9f mov ax,[esi+ebx*2+0x2]
							nBlock = pNode->m_nChild[nStart];
						}
					}
					else
					{
						if (nStart == WS_INFRONT)
						{
							if (sim.TestPoly(pRec->m_pOwner,
							                 (WorldPoly*)pRec))
							{
								pState->m_nHits++;
							}
						}

						// descend copy #2 -- 0x00419ecc..0x00419f3a. Its dot
						// product reads z before y where copy #1 reads y first
						// (AvP2 server.dll 0x10006357 against 0x10006306), so
						// the two are spelled differently and share only
						// their tails.
						pPlane = ws_GetPlaneQ506(pRec->m_pOwner);

						fDist = sim.m_vStart.x * pPlane->m_Normal.x +
						        (sim.m_vStart.y * pPlane->m_Normal.y +
						         sim.m_vStart.z * pPlane->m_Normal.z) -
						        pPlane->m_Dist;

						if (fDist > WS_PLANE_SLOP)
							nSide = 1;
						else
							nSide = 0;

						if (pNode->m_nChild[nSide == 0] != WS_NODE_EMPTY &&
						    pNode->m_nChild[nSide == 0] != WS_NODE_SOLID)
						{
							*pTop = pNode->m_nChild[nSide == 0];
							pTop++;
						}

						nBlock = pNode->m_nChild[nSide];
					}
				}
			}

			if (nHitsAtStart != pState->m_nHits)
			{
				if (nDeflects != 0 && nHitsLast != pState->m_nHits)
				{
					nDeflects--;

					sim.DeflectOffHit(sim.m_pHitPoly, pState);

					nHitsLast = pState->m_nHits;

					continue;
				}

				if (nHitsLast != pState->m_nHits)
				{
					vTmp.x = g_vMoveBase.x + g_vMoveStart.x;
					vTmp.y = g_vMoveBase.y + g_vMoveStart.y;
					vTmp.z = g_vMoveBase.z + g_vMoveStart.z;

					sim.m_vEnd = vTmp;
				}

				break;
			}

			vScaled.Init(fStepShort * vDir.x,
			             fStepShort * vDir.y,
			             fStepShort * vDir.z);

			vTmp.Init(vAt.x + vScaled.x,
			          vAt.y + vScaled.y,
			          vAt.z + vScaled.z);

			vAt = vTmp;

			fRemaining = fRemaining - fStepShort;
		}

		vTmp.x = sim.m_vEnd.x - g_vMoveBase.x;
		vTmp.y = sim.m_vEnd.y - g_vMoveBase.y;
		vTmp.z = sim.m_vEnd.z - g_vMoveBase.z;

		g_vMoveEnd = vTmp;
	}
	}
	}
	}

	return 1;
}

// AVP2: the "NewStairStep" step-up (server.dll 0x100064fa, lithtech.exe
// 0x004205d0; called from ws_MoveObject at 0x10004cc1). The same tree walk
// as ws_StairStep, but what it steps onto is found from the polygon's own
// clipped extent (w_IsPolyBlocking's bounds) and the swept box's lowest
// corners, and the lift is taken against the move itself: the end is
// nudged by the lift less the part of it along the move, the start by the
// lift whenever that rises, and half the rise comes off the box's height.
// A surface marked no-step is never stepped onto; meeting one is reported
// through the third argument, and the walk goes on. A box whose height the
// steps have used up ends the walk at once. (name ours)
//
// NEAR on server.dll (1574 of 1581, one block): shipped computes the lowest
// corner's address, `lea 0x1008c418(,%eax,4),%eax` at 0x100068a2, right after
// reading its y through the folded address (0x10006898), and never uses it --
// %eax is overwritten by the fnstsw that follows. A pointer or reference to
// the corner, named before or after the read, at block or function scope,
// either folds away entirely or (when it carries the read) becomes the base
// of `fadds 4(%eax)`, which is AvP2 1.0.9.6's shape (0x10006883) and not
// this build's. Read off the shipped bytes, each of these moved it:
//   * the pop of the node stack is its own `while`, re-testing the popped
//     node (0x10006545 jumps back to the test), and the three descends are
//     guarded `continue`s;
//   * the sphere reject is pPlane->DistTo(g_BoxSphere.m_vPoint): its
//     by-value copy is the movsl at 0x1000657a;
//   * the move and its direction are built component-wise from the two
//     globals (no operator- copy of g_vMoveStart, 0x10006626, 0x10006942);
//   * the facing test is written out against a copy of the plane's normal,
//     and the lift's dot product against the move is written out too: the
//     term order and the operand loaded (0x1000666a, 0x10006986) come from
//     these spellings only (48 and 24 spellings measured);
//   * the plane copy is a function-scope local: block-scoped, VC6 hoists the
//     slope load above the copy's last movsl (0x100066c3);
//   * the no-step arm reads the child before the flag store, not after
//     (0x100065fd);
//   * the lift is `vNormal * fLift` with fLift named (`flds; fmul %st(1)`,
//     0x10006867), and the base takes -fHalf away (`fchs; fsubr`, 0x10006aa9).
// NEAR on lithtech.exe (2159 of 2841): shipped expands ws_NudgeQ516 at both
// calls (0x00420d6b.., 0x00420f0f..) and calls operator+ out of line for the
// first one's inner sum (0x00420e3c). With ws_NudgeQ516 `inline` (it is
// defined above) this is 2802 of 2841. Beyond it, shipped reads g_vMoveStart in place in the
// second expansion where ours copies it, keeps the lift's dot in a frame
// slot (0x00420c77), and wants vMove.Dot(pPlane->m_Normal) for the facing
// test -- the spelling server.dll does not take.
void ws_StairStepNewQ553(uint16 nBlock, CBounceState* pState, int* pbNoStep)
{
	uint16*         pTop;
	WorldPolyRef*   pRecs;
	WSNode*         pNode;
	WorldPolyRef*   pRec;
	WorldPolyOwner* pOwner;
	LTPlane*        pPlane;
	float           fDist;
	uint16          nChild;
	LTPlane         plane;

	pRecs = g_pMoveState->m_pWorldBsp->GetPolyRefs();

	pTop = g_BlockStackQ534;

	while (TRUE)
	{
		while (nBlock == WS_NODE_EMPTY || nBlock == WS_NODE_SOLID)
		{
			if (pTop == g_BlockStackQ534)
				return;

			pTop--;
			nBlock = *pTop;
		}

		pNode  = &g_pPBlock->m_pNodes[nBlock];
		pRec   = &pRecs[pNode->m_nPoly];
		pOwner = pRec->m_pOwner;
		pPlane = ws_GetPlaneQ506(pOwner);

		fDist = pPlane->DistTo(g_BoxSphere.m_vPoint);

		if (fDist > g_BoxSphere.m_fRadius)
		{
			nBlock = pNode->m_nChild[1];
			continue;
		}

		if (fDist < -g_BoxSphere.m_fRadius)
		{
			nBlock = pNode->m_nChild[0];
			continue;
		}

		if (pOwner->m_pSurface->m_Flags & WBSP_NOSTEP)
		{
			nChild = pNode->m_nChild[0];

			if (nChild != WS_NODE_EMPTY && nChild != WS_NODE_SOLID)
			{
				*pTop = nChild;
				pTop++;
			}

			nBlock = pNode->m_nChild[1];

			*pbNoStep = 1;
			continue;
		}

		if (pOwner && pPlane->m_Normal.y > g_fStepMinNormalY)
		{
			LTVector vMove(g_vMoveEnd.x - g_vMoveStart.x,
			               g_vMoveEnd.y - g_vMoveStart.y,
			               g_vMoveEnd.z - g_vMoveStart.z);

			vMove.Norm();

			LTVector vUp = pPlane->m_Normal;

			if (vUp.x * vMove.x + vUp.z * vMove.z + vUp.y * vMove.y < 0.5f)
			{
				LTVector vMin;
				LTVector vMax;

				if (w_IsPolyBlocking(pRec->m_pOwner, &vMin, &vMax))
				{
					float   fBest;
					float   fScale;
					int     iBest;
					int     i;

					plane = *pPlane;

					fBest = 5000.0f;

					if (pPlane->m_Normal.y < g_fStepSlopeLimit)
					{
						plane.m_Normal.y = 0.0f;
						plane.m_Normal.Norm();

						fScale = -1.0f / plane.m_Normal.Dot(pPlane->m_Normal);

						for (i = 0; i < 4; i++)
						{
							fDist = (plane.m_Dist -
							         pPlane->m_Normal.Dot(g_SweepPoints[1][i])) *
							        fScale;

							if (fDist < fBest)
							{
								fBest = fDist;
								iBest = i;
							}
						}
					}
					else
					{
						plane.m_Normal.Init(0.0f, 1.0f, 0.0f);

						fScale = -1.0f / pPlane->m_Normal.y;

						for (i = 0; i < 4; i++)
						{
							fDist = (plane.m_Dist -
							         pPlane->m_Normal.Dot(g_SweepPoints[1][i])) *
							        fScale;

							if (fDist < fBest)
							{
								fBest = fDist;
								iBest = i;
							}
						}
					}

					if (fBest < -0.0001f)
					{
						LTVector vNormal;
						LTVector vSave;
						LTVector vPush;
						LTVector vStep;
						float    fTop;
						float    fAlong;
						float    fLift;

						g_pMoveState->m_pInfo->m_Plane = plane;
						g_pMoveState->m_pInfo->m_hObject = g_pMoveState->m_hObject;
						g_pMoveState->m_pInfo->m_hPoly =
						    g_pMoveState->m_pWorldBsp->MakeHPoly((WorldPoly*)pRec);

						vNormal = plane.m_Normal;

						pState->m_nHits++;

						vSave = pState->m_vVelocityAdd;

						ws_HandleImpact(g_pMoveState->m_pInfo, pState,
						                g_pMoveState->m_pObject,
						                g_pMoveState->m_hObject,
						                g_pMoveState->m_pWorldBsp,
						                (WorldPoly*)pRec, &vNormal);

						pState->m_vVelocityAdd = vSave;

						fLift = -fBest;

						vPush = vNormal * fLift;

						vStep = vPush;

						fTop = vPush.y + g_SweepPoints[1][iBest].y;

						if (fTop > vMax.y)
							vStep.y = vPush.y - (fTop - vMax.y);

						if (vStep.x < 0.0f)
						{
							if (vMax.x < g_SweepPoints[1][2].x)
								vStep.x = 0.0f;
						}
						else if (vStep.x > 0.0f)
						{
							if (vMin.x > g_SweepPoints[1][0].x)
								vStep.x = 0.0f;
						}

						if (vStep.z < 0.0f)
						{
							if (vMax.z < g_SweepPoints[1][2].z)
								vStep.z = 0.0f;
						}
						else if (vStep.z > 0.0f)
						{
							if (vMin.z > g_SweepPoints[1][0].z)
								vStep.z = 0.0f;
						}

						LTVector vDir(g_vMoveEnd.x - g_vMoveStart.x,
						              g_vMoveEnd.y - g_vMoveStart.y,
						              g_vMoveEnd.z - g_vMoveStart.z);

						vNormal = vStep;

						vNormal.Norm();

						fAlong = vNormal.x * vDir.x + vDir.y * vNormal.y +
						         vDir.z * vNormal.z;

						if (fAlong > 0.0f)
							vStep -= vNormal * fAlong;

						ws_NudgeQ516(&g_vMoveEnd, g_vMoveEnd, vStep);

						vPush = vStep + vNormal * fAlong;

						if (vPush.y > 0.0f)
						{
							LTVector vStart = g_vMoveStart;

							ws_NudgeQ516(&vStart, g_vMoveStart, vPush);

							g_vMoveStart.y = vStart.y;

							float fHalf = vPush.y * 0.5f;

							g_vMoveBase.y -= -fHalf;
							g_pMoveState->m_vDims.y -= fHalf;
						}

						if (g_pMoveState->m_vDims.y < 0.0f)
							return;

						ws_SetupSweptBox(&g_vStepSweepMinQ553,
						                 &g_vStepSweepMaxQ553);

						g_pMoveResult->m_pStandingPoly = (WorldPoly*)pRec;
					}

					nBlock = pNode->m_nChild[1];
					continue;
				}
			}
		}

		nChild = pNode->m_nChild[0];

		if (nChild != WS_NODE_EMPTY && nChild != WS_NODE_SOLID)
		{
			*pTop = nChild;
			pTop++;
		}

		nBlock = pNode->m_nChild[1];
	}
}

// Can the object step up onto anything in this block? The same tree walk as
// the cylinder sweep, but looking only for upward-facing surfaces the object
// is moving into and is allowed to stand on. What it finds it steps onto
// there and then: the whole move is lifted, the box's height is cut by half
// the lift, and the geometry is rebuilt -- which is why the caller has to put
// the dims back afterwards either way. A surface marked no-step, or one that
// would lift the object further than the move descends, answers one instead
// and leaves everything alone.
//
// PH: against Primal Hunt's server.dll 0x10006b27 (1161 bytes) this is now
// 1124. Two layout changes are in: the shared node stack, and the plane read
// through the polygon's surface test, both at the node (0x10006bc0..
// 0x10006bcd) and again for the step test and vNormal (0x10006c39..
// 0x10006c95), where shipped re-reads pOwner->GetPlane() rather than the
// local. The note below is from an earlier image.
// NEAR (1078 of 1080, the same 361 instructions, 9 blocks; lithtech 1430 of
// 1429). The layout came from three spellings: the radius tests as ONE
// if / else-if / else chain with no `continue`s (that puts the shared
// `nBlock = m_nChild[1]` tail on the FIRST arm, which Descend then jumps back
// to, as shipped 0x10004da3); the step guards as nested positive ifs with the
// descend code after them (not `goto Descend`); and the SDK operators for
// `vAny = m_Velocity + vVel`, `.Dot(vNormal)` and `vFlat * -fBest`. Walking
// the tree in the parameter itself (no nNode copy) put fBest in the dead
// nBlock slot as shipped. Left: shipped holds the node index in %bx, ours in
// %ax (allocation), and shipped's g_vMoveStart copy completes before the
// g_vMoveEnd loads (one store-scheduling block; operator/component spellings
// of both sums and statement order did not move it).
#ifdef AVP2SRC_MODERN
// M-P5-20: the highest point of a polygon over the rectangle x0..x1, z0..z1,
// or 0 when none of it is over it. The polygon is clipped to the rectangle
// seen from above (its height carried along each edge) and the highest of
// what is left is the answer: for a plane over a convex region, it is at one
// of the region's corners.
static int ws_PolyTopOver(WorldPolyOwner* pPoly, float x0, float x1,
                          float z0, float z1, float* pTop)
{
	LTVector vA[40], vB[40];
	LTVector *pIn = vA, *pOut = vB, *pSwap;
	uint32   nIn, nOut, i, nSide;
	float    fLimit, fSign, fCur, fPrev, fT;

	if (pPoly->m_nVertices < 3 || pPoly->m_nVertices > 32)
		return -1;   // not ours to answer: the retail lift

	for (i = 0; i < pPoly->m_nVertices; i++)
		vA[i] = *pPoly->m_Vertices[i].m_pPos;
	nIn = pPoly->m_nVertices;

	for (nSide = 0; nSide < 4 && nIn; nSide++)
	{
		// x >= x0, x <= x1, z >= z0, z <= z1, as fSign * (v - fLimit) >= 0
		fLimit = (nSide == 0) ? x0 : (nSide == 1) ? x1 : (nSide == 2) ? z0 : z1;
		fSign  = (nSide & 1) ? -1.0f : 1.0f;

		nOut  = 0;
		fPrev = fSign * (((nSide < 2) ? pIn[nIn - 1].x : pIn[nIn - 1].z) - fLimit);

		for (i = 0; i < nIn; i++)
		{
			const LTVector& vPrev = pIn[i ? i - 1 : nIn - 1];
			const LTVector& vCur  = pIn[i];

			fCur = fSign * (((nSide < 2) ? vCur.x : vCur.z) - fLimit);

			if ((fCur >= 0.0f) != (fPrev >= 0.0f))
			{
				fT = fPrev / (fPrev - fCur);
				pOut[nOut++] = vPrev + (vCur - vPrev) * fT;
			}
			if (fCur >= 0.0f)
				pOut[nOut++] = vCur;

			fPrev = fCur;
		}

		pSwap = pIn; pIn = pOut; pOut = pSwap;
		nIn = nOut;
	}

	if (!nIn)
		return 0;

	*pTop = pIn[0].y;
	for (i = 1; i < nIn; i++)
	{
		if (pIn[i].y > *pTop)
			*pTop = pIn[i].y;
	}
	return 1;
}
#endif

int ws_StairStep(uint16 nBlock, CBounceState* pState)
{
	LTVector       vMax;
	LTVector       vMin;
	LTVector       vSave;
	LTVector       vAny;
	LTVector       vPoint;
	LTVector       vOrigNormal;
	LTVector       vPointA;
	LTVector       vVel;
	LTVector       vAtEnd;
	LTVector       vAtStart;
	LTVector       vCentre;
	LTVector       vFlat;
	LTVector       vNormal;
	WorldPolyRef*  pRecs;
	LTPlane*       pPlane;
	WSNode*        pNode;
	int            nResult;
	LTVector       vPush;
	float          fRise;
	WorldPolyRef*  pRec;
	uint16*        pTop;

	uint16          nChild;
	WorldPolyOwner* pOwner;
	CollisionInfo*  pInfo;
	LTObject*       pObj;
	int             nPoint;
	float           fDist, fBest, fScale;

	nResult = 0;

	if (g_vMoveStart.y > g_vMoveEnd.y)
		fRise = g_vMoveStart.y - g_vMoveEnd.y;
	else
		fRise = 0.0f;

	pRecs = g_pMoveState->m_pWorldBsp->GetPolyRefs();

	pTop  = g_BlockStackQ534;

	for (;;)
	{
		if (nBlock == WS_NODE_EMPTY || nBlock == WS_NODE_SOLID)
		{
			if (pTop == g_BlockStackQ534)
				break;

			pTop--;
			nBlock = *pTop;
		}

		pNode  = &g_pPBlock->m_pNodes[nBlock];
		pOwner = pRecs[pNode->m_nPoly].m_pOwner;
		pRec   = &pRecs[pNode->m_nPoly];
		pPlane = ws_GetPlaneQ506(pOwner);

		vCentre = g_BoxSphere.m_vPoint;

		// TERM ORDER IS NOT THE LEVER HERE, and this is worth reading before
		// anyone tries it again, because a normalised byte diff CANNOT SEE the difference:
		// it masks hex operands, so `flds X / fmuls X` compares
		// equal whichever vector component was loaded, and no mnemonic block
		// will ever mention it.
		//
		// The shipped site 0x10003b0b emits `fmuls 0x8(%eax)` (z) then
		// `fmuls 0x4(%eax)` (y). Ours emits 0x4 then 0x8. One reading took that
		// as our build reversing source order for a frame-local vector and
		// proposed swapping these two terms at four sites.
		//
		// MEASURED BY DISASSEMBLING BOTH SPELLINGS: swapping them changes
		// NOTHING. Source `z + y + x` and source `y + z + x` both compile to
		// `fmuls 0x4` then `fmuls 0x8`. VC6 re-associates the sum, so the
		// order written here never reaches the output and neither that
		// reading nor the obvious opposite one is right. Whatever
		// picks the shipped order, it is not this.
		fDist = vCentre.z * pPlane->m_Normal.z +
		        vCentre.y * pPlane->m_Normal.y +
		        vCentre.x * pPlane->m_Normal.x - pPlane->m_Dist;

		if (fDist > g_BoxSphere.m_fRadius)
		{
			nBlock = pNode->m_nChild[1];
		}
		else if (fDist < -g_BoxSphere.m_fRadius)
		{
			nBlock = pNode->m_nChild[0];
		}
		else
		{
			if (pOwner && pOwner->GetPlane()->m_Normal.y > g_fStepMinNormalY)
			{
				vVel = g_pMoveResult->m_vVelocityAdd;

				pObj = g_pMoveState->m_pObject;

				vNormal = pOwner->GetPlane()->m_Normal;

				vAny = pObj->m_Velocity + vVel;

#ifdef PRIMAL_HUNT
				if (!(vAny.Dot(vNormal) > 0.0f) &&
#else
				// AVP2: `<=`, which cl 9044 keeps apart from the negated `>`
				// (AvP2 1.0.9.6's lithtech.exe 0x0041ef96 `test $0x41; jp`:
				// an unordered dot product is no contact).
				if (vAny.Dot(vNormal) <= 0.0f &&
#endif
				    w_IsPolyBlocking(pOwner, LTNULL, LTNULL))
				{
					fBest = WS_STEP_FAR;

					pPlane = ws_GetPlaneQ506(pRec->m_pOwner);

					pInfo = g_pMoveState->m_pInfo;

					pInfo->m_Plane = *pPlane;

					if (pPlane->m_Normal.y < g_fStepSlopeLimit)
					{
						pInfo->m_Plane.m_Normal.y = 0.0f;
						pInfo->m_Plane.m_Normal.Norm();

						vOrigNormal = pPlane->m_Normal;

						fScale = -1.0f /
						         (vOrigNormal.z * pInfo->m_Plane.m_Normal.z +
						          vOrigNormal.y * pInfo->m_Plane.m_Normal.y +
						          vOrigNormal.x * pInfo->m_Plane.m_Normal.x);

						for (nPoint = 0; nPoint < 4; nPoint++)
						{
							vPoint = g_SweepPoints[1][nPoint];

							fDist = (pInfo->m_Plane.m_Dist -
							         (vPoint.y * pPlane->m_Normal.y +
							          vPoint.z * pPlane->m_Normal.z +
							          vPoint.x * pPlane->m_Normal.x)) * fScale;

							if (fDist < fBest)
								fBest = fDist;
						}
					}
					else
					{
#ifdef PRIMAL_HUNT
						for (nPoint = 0; nPoint < 4; nPoint++)
						{
							vPointA = g_SweepPoints[1][nPoint];

							fDist = vPointA.z * pInfo->m_Plane.m_Normal.z +
							        vPointA.y * pInfo->m_Plane.m_Normal.y +
							        vPointA.x * pInfo->m_Plane.m_Normal.x -
							        pInfo->m_Plane.m_Dist;

							if (fDist < fBest)
								fBest = fDist;
						}
#else
						// AVP2: a floor is reported as flat, and each corner's
						// rise is measured straight up to the plane, as the
						// slope arm measures it along the flattened normal
						// (AvP2 1.0.9.6's lithtech.exe 0x0041f0f4..0x0041f17f).
						pInfo->m_Plane.m_Normal = LTVector(0.0f, 1.0f, 0.0f);

						fScale = -1.0f / pPlane->m_Normal.y;

						for (nPoint = 0; nPoint < 4; nPoint++)
						{
							vPointA = g_SweepPoints[1][nPoint];

							fDist = (pInfo->m_Plane.m_Dist -
							         (vPointA.y * pPlane->m_Normal.y +
							          vPointA.z * pPlane->m_Normal.z +
							          vPointA.x * pPlane->m_Normal.x)) * fScale;

							if (fDist < fBest)
								fBest = fDist;
						}
#endif
					}

					g_pMoveState->m_pInfo->m_hObject = g_pMoveState->m_hObject;

					g_pMoveState->m_pInfo->m_hPoly =
					    g_pMoveState->m_pWorldBsp->MakeHPoly((WorldPoly*)pRec);

#ifdef AVP2SRC_MODERN
					// M-P5-20: a floor or a slope one can walk on lifts the
					// feet onto the highest point of it that is under them.
					// The lift was measured from the swept box's corners to
					// the polygon's plane, wherever the corners were, and the
					// plane goes on past the polygon: over the top edge of a
					// ramp, or a ridge between two slopes, the corners beyond
					// it were below that plane carried on, and the player was
					// stepped up onto it in mid-air -- shoved back and up,
					// falling again, over and over (retail does it too). And
					// along the slope's normal, that is backwards, off it.
					// Over the polygon throughout, this is retail's lift,
					// straight up.
					vPush.Init(0.0f, 0.0f, 0.0f);
					if (pPlane->m_Normal.y >= g_fStepSlopeLimit)
					{
						float fTop;
						int   nOver = ws_PolyTopOver(pOwner,
						                            g_SweepPoints[1][0].x, g_SweepPoints[1][2].x,
						                            g_SweepPoints[1][0].z, g_SweepPoints[1][2].z,
						                            &fTop);

						if (nOver >= 0)
						{
							fBest = (nOver && fTop > g_SweepPoints[1][0].y) ?
							        g_SweepPoints[1][0].y - fTop : 0.0f;
							vPush.y = -fBest;
						}
					}
#endif
					if (fBest < 0.0f)
					{
						vFlat = pInfo->m_Plane.m_Normal;

#ifdef AVP2SRC_MODERN
						if (vPush.y == 0.0f)
#endif
						vPush = vFlat * -fBest;

						if (pRec->m_pOwner->m_pSurface->m_Flags & WBSP_NOSTEP)
						{
#ifdef PRIMAL_HUNT
							if (fRise < vPush.y)
#else
							// AVP2: negated (lithtech.exe 0x0041f219 `and
							// $0x100`: an unordered rise is refused too).
							if (!(fRise >= vPush.y))
#endif
							{
								nResult = 1;

								nBlock = pNode->m_nChild[1];

								continue;
							}
						}

						pState->m_nHits++;

						vSave = pState->m_vVelocityAdd;

						ws_HandleImpact(g_pMoveState->m_pInfo, pState,
						                g_pMoveState->m_pObject, g_pMoveState->m_hObject,
						                g_pMoveState->m_pWorldBsp, (WorldPoly*)pRec,
						                &g_pMoveState->m_pInfo->m_Plane.m_Normal);

						pState->m_vVelocityAdd = vSave;

						vAtStart = g_vMoveStart + vPush;

						g_vMoveStart = vAtStart;

						vAtEnd = g_vMoveEnd + vPush;

						g_vMoveEnd = vAtEnd;

						g_vMoveBase.y = g_vMoveBase.y - vPush.y * 0.5f;

						g_pMoveState->m_vDims.y =
						    g_pMoveState->m_vDims.y - vPush.y * 0.5f;

						ws_SetupSweptBox(&vMin, &vMax);

						g_pMoveResult->m_pStandingPoly = (WorldPoly*)pRec;

						fRise = fRise - vPush.y;
					}

					nBlock = pNode->m_nChild[1];

					continue;
				}
			}

			nChild = pNode->m_nChild[0];

			if (nChild != WS_NODE_EMPTY && nChild != WS_NODE_SOLID)
			{
				*pTop = nChild;
				pTop++;
			}

			nBlock = pNode->m_nChild[1];
		}
	}

	return nResult;
}

// AVP2: the box move in two passes (PH server.dll 0x10004f9e, 78 bytes;
// lithtech.exe 0x0041e6f0, 95; AvP2 server.dll 0x10004fcd). Both images
// sweep through the four-argument box move that follows this in the image
// (server.dll 0x10004fec, lithtech.exe 0x0041e750), whose fourth argument,
// a flag cleared here first (`andl $0x0,-0x4(%ebp)`, 0x10004fa5), says the
// first pass wants another; only then is the box shifted (ws_ShiftBox,
// 0x100024f1 / 0x0041aa60) and swept again with the third argument set.
// The four-argument move follows (ws_MoveBoxQ544), built for every engine:
// Primal Hunt's (server.dll 0x10004fec, lithtech.exe 0x0041e750) is the same
// function. (names ours)
int ws_MoveBoxQ544(uint16 nBlock, CBounceState* pState, int bRetry,
                   int* pbAgain);                     // 0x10004fec / 0x0041e750

int ws_MoveBoxPassesQ544(uint16 nBlock, CBounceState* pState)
{
	int bAgain = 0;

	if (!ws_MoveBoxQ544(nBlock, pState, 0, &bAgain))
		return 0;

	if (!bAgain)
		return 1;

	if (!ws_ShiftBox())
		return 0;

	return ws_MoveBoxQ544(nBlock, pState, 1, &bAgain);
}

// AVP2: the four-argument box move (AvP2 1.0.9.6's lithtech.exe 0x0041c460,
// server.dll 0x1000501b). It walks the tree as NOLF's ws_MoveBox does, but a
// polygon both ends straddle is no longer pushed back along the move a step
// at a time. The first pass only asks for another (*pbAgain); the second
// takes the polygon's clipped bounds (w_IsPolyBlocking's out-parameters) and
// pushes the box out along whichever axis of the move it overlaps least --
// none it moves along at all abandons the move -- reporting the impact
// against that axis. A polygon the move faces away from is left alone. One
// the box only comes to lies in front of is ws_HitPoly's, as before, and a
// refused hit there falls through to the descend.
//
// NEAR on server.dll (2400 of 2404). NEAR on lithtech.exe (4652 of 4543):
// /O2's inline budget runs out elsewhere. Shipped expands CPolyTest's
// constructor and calls WorldPoly::GetPlane, Init, operator+ and Dot in the
// ws_HitPoly copy and the descend (0x0041d054, 0x0041d22c, 0x0041d545,
// 0x0041d599); ours calls the CBoxProject constructor and expands those.
// The straddle arm's ws_NudgeQ516 is expanded in both, its operators called.
// The re-setup's `vAt = LTVector(sum, ...)` is shipped's shape (a constructed
// temporary, no by-value copy of g_vMoveStart), but it moves the budget
// further off (94 blocks against 59), so the operator form stays.
#define WS_BOX_AXIS_EPSILON 0.0001f

int ws_MoveBoxQ544(uint16 nBlock, CBounceState* pState, int bRetry,
                   int* pbAgain)
{
	CPolyTest test;
#ifdef LT_SERVER_TIMERS
	CountAdder cntAdder(&g_nPhysStepTime);
#endif
	LTVector   vAt;
	LTVector   vDir;
	LTVector   vPolyMin;
	LTVector   vPolyMax;
	LTVector   vNormal;
	LTVector   vPush;

	WorldPolyRef* pRecs;
	WorldPolyRef* pRec;
	WSNode*       pNode;
	uint32        nProjected;
	uint16*       pTop;
	uint16        nNode;
	uint16        nChild;
	int           nStart, nEnd, nSide;
	int           i, nAxis;
	float         fBest, fDist, fPush;
	LTPlane*      pPlane;

	vAt = g_vMoveBase + g_vMoveStart;

	test.m_Start.m_pPoints = g_SweepPoints[0];
	test.m_End.m_pPoints   = g_SweepPoints[1];

	test.m_Start.m_pSphere = &g_SphereAtDelta;
	test.m_End.m_pSphere   = &g_SphereAtOffset;

	test.m_End.m_pProjected   = &nProjected;
	test.m_Start.m_pProjected = &nProjected;

	test.m_Start.m_vMin = vAt - g_pMoveState->m_vDims;
	test.m_Start.m_vMax = vAt + g_pMoveState->m_vDims;

	vAt = g_vMoveBase + g_vMoveEnd;

	test.m_End.m_vMin = vAt - g_pMoveState->m_vDims;
	test.m_End.m_vMax = vAt + g_pMoveState->m_vDims;

	pRecs = g_pMoveState->m_pWorldBsp->GetPolyRefs();

	pTop  = g_BlockStackQ534;
	nNode = nBlock;

	for (;;)
	{
		if (nNode == WS_NODE_EMPTY || nNode == WS_NODE_SOLID)
		{
			if (pTop == g_BlockStackQ534)
			{
				return 1;
			}

			pTop--;
			nNode = *pTop;
		}

		g_nPhysNodes++;

		pNode  = &g_pPBlock->m_pNodes[nNode];
		pRec   = &pRecs[pNode->m_nPoly];
		pPlane = ws_GetPlaneQ506(pRec->m_pOwner);

		fDist = pPlane->DistTo(g_BoxSphere.m_vPoint);

		if (fDist > g_BoxSphere.m_fRadius)
		{
			nNode = pNode->m_nChild[1];
			continue;
		}

		if (fDist < -g_BoxSphere.m_fRadius)
		{
			nNode = pNode->m_nChild[0];
			continue;
		}

		nProjected = 1;

		test.m_End.m_pPlane   = pPlane;
		test.m_Start.m_pPlane = pPlane;

		nStart = g_BoxProjectFns[pRec->m_nProject](&test.m_Start);
		nEnd   = g_BoxProjectFns[pRec->m_nProject](&test.m_End);

		if (nStart == nEnd)
		{
			if (nStart != WS_STRADDLE)
			{
				nNode = pNode->m_nChild[nStart];
				continue;
			}

			vDir = g_vMoveEnd - g_vMoveStart;
			vDir.Norm();

			if (vDir.Dot(pPlane->m_Normal) < WS_BOX_AXIS_EPSILON &&
			    w_IsPolyBlocking(pRec->m_pOwner, &vPolyMin, &vPolyMax))
			{
				*pbAgain = 1;

				if (bRetry)
				{
					fBest = FLT_MAX;
					nAxis = -1;

					for (i = 0; i < 3; i++)
					{
						if (VEC_INDEX(vDir, i) > WS_BOX_AXIS_EPSILON)
						{
							fDist = VEC_INDEX(g_vStepSweepMaxQ553, i) - VEC_INDEX(vPolyMin, i);

							if (fDist < fBest)
							{
								fBest = fDist;
								fPush = -fDist;
								nAxis = i;
							}
						}
						else if (VEC_INDEX(vDir, i) < -WS_BOX_AXIS_EPSILON)
						{
							fDist = VEC_INDEX(vPolyMax, i) - VEC_INDEX(g_vStepSweepMinQ553, i);

							if (fDist < fBest)
							{
								fBest = fDist;
								fPush = fDist;
								nAxis = i;
							}
						}
					}

					if (nAxis == -1)
						return 0;

					vNormal.Init(0.0f, 0.0f, 0.0f);
					VEC_INDEX(vNormal, nAxis) = 1.0f;

					if (vNormal.y > WS_FLOOR_NORMAL_Y)
					{
						if (!g_pMoveResult->m_pStandingPoly ||
						    ws_GetPlaneQ506(g_pMoveResult->m_pStandingPoly->m_pOwner)->m_Normal.y <
						        pPlane->m_Normal.y)
						{
							g_pMoveResult->m_pStandingPoly = (WorldPoly*)pRec;
						}
					}

					ws_HandleImpact(g_pMoveState->m_pInfo, pState,
					                g_pMoveState->m_pObject, g_pMoveState->m_hObject,
					                g_pMoveState->m_pWorldBsp, (WorldPoly*)pRec,
					                &vNormal);

					pState->m_nHits++;

					if (g_pMoveState->m_bPushOut)
					{
						vPush = vNormal * fPush;
					}
					else
					{
						LTVector vDelta;
						float    fDot, fT;

						vDelta.Init(g_vMoveEnd.x - g_vMoveStart.x,
						            g_vMoveEnd.y - g_vMoveStart.y,
						            g_vMoveEnd.z - g_vMoveStart.z);

						fDot = vNormal.Dot(vDelta);

						if (fDot < FLT_EPSILON && fDot > -FLT_EPSILON)
							fT = 0.0f;
						else
							fT = -fPush / fDot;

						vPush = vDelta * -fT;
					}

					ws_NudgeQ516(&g_vMoveEnd, g_vMoveEnd, vPush);

					if (!ws_ShiftBox())
						goto Failed;

					vAt = g_vMoveBase + g_vMoveStart;

					test.m_Start.m_vMin = vAt - g_pMoveState->m_vDims;
					test.m_Start.m_vMax = vAt + g_pMoveState->m_vDims;

					vAt = g_vMoveBase + g_vMoveEnd;

					test.m_End.m_vMin = vAt - g_pMoveState->m_vDims;
					test.m_End.m_vMax = vAt + g_pMoveState->m_vDims;
				}
			}

			pPlane = ws_GetPlaneQ506(pRec->m_pOwner);

			fDist = g_vMoveStart.z * pPlane->m_Normal.z +
			        g_vMoveStart.y * pPlane->m_Normal.y +
			        g_vMoveStart.x * pPlane->m_Normal.x - pPlane->m_Dist;
		}
		else
		{
			if (nStart == WS_INFRONT &&
			    w_IsPolyBlocking(pRec->m_pOwner, LTNULL, LTNULL) &&
			    ws_HitPoly((WorldPoly*)pRec, pState, &test))
			{
				vAt = g_vMoveBase + g_vMoveStart;

				test.m_Start.m_vMin = vAt - g_pMoveState->m_vDims;
				test.m_Start.m_vMax = vAt + g_pMoveState->m_vDims;

				vAt = g_vMoveBase + g_vMoveEnd;

				test.m_End.m_vMin = vAt - g_pMoveState->m_vDims;
				test.m_End.m_vMax = vAt + g_pMoveState->m_vDims;

				nNode = pNode->m_nChild[1];

				continue;
			}

			pPlane = ws_GetPlaneQ506(pRec->m_pOwner);

			fDist = g_vMoveStart.z * pPlane->m_Normal.z +
			        g_vMoveStart.y * pPlane->m_Normal.y +
			        g_vMoveStart.x * pPlane->m_Normal.x - pPlane->m_Dist;
		}

		if (fDist > WS_PLANE_SLOP)
			nSide = 1;
		else
			nSide = 0;

		nChild = pNode->m_nChild[nSide == 0];

		if (nChild != WS_NODE_EMPTY && nChild != WS_NODE_SOLID)
		{
			*pTop = nChild;
			pTop++;
		}

		nNode = pNode->m_nChild[nSide];
	}

Failed:
	return 0;
}

// The box sweep. Unlike the cylinder there is no marching: the box at each
// end of the move is projected onto every plane the tree offers, and a plane
// both ends straddle is a real candidate. Which of the seven projections is
// used comes off the polygon itself -- a plane lying along an axis needs no
// projection at all.
//
// A polygon the box is genuinely inside is answered one of two ways. On the
// first attempt at a move the contact is simply recorded. On a retry the box
// is pushed back along its own path, one step at a time, until it is outside
// -- fifty steps at most, and failing that the whole move is abandoned.
//
// NEAR, size-exact on server.dll (2134 = 2134 bytes, 617 = 617 instructions,
// no mnemonic difference). Read off the shipped bytes, each one moved it:
//   * the sphere reject is `pPlane->DistTo(g_BoxSphere.m_vPoint)`: the
//     by-value argument is the 12-byte copy at -0xa4 (0x10003ae4), not a
//     named vCentre in the upper frame;
//   * each re-projection has its own pair of position vectors (retry
//     -0x4c/-0x5c, hit -0x1c/-0x10) and the prologue's one is shared, which
//     is what makes the frame 0x9a0;
//   * no nResult: `return 1` / `return 0` carry the value in esi across the
//     one CountAdder destructor at 0x10004134, and `Failed:` has its own at
//     0x10004158 -- which also puts the counter at -0x68;
//   * the straddle arm is `if (nStart == WS_STRADDLE) {..} else {child}` with
//     positive guards round `if (bRetry) {push back} else {record}`, and the
//     front arm is a positive guard with the descend written out after it
//     (0x10003f36, 0x10003be3, 0x10003f45);
//   * the push-back loop is a `for` (one bottom test at 0x10003d0a), and has
//     its own pointer locals: pPts0 of the first test lives in the dead
//     nBlock slot, the loop's copy at -0x50, nTries in the nBlock slot;
//   * pPts1 is assigned before pPts0 (the -0x19c load comes first);
//   * the positions are `Init(g_vMoveBase.x + g_vMoveStart.x, ...)`: three
//     separate stores let VC6 keep g_vMoveBase.x on the stack across the
//     second one (`fld %st(0)`), which the shipped build never does;
//   * m_End.m_pProjected is stored before m_Start's (0x10003953).
// Left: 14 instructions. (a) pRecs sits at -0x50 where shipped has -0x40,
// which moves vRetryStart and the loop's pPts0 by 4. Neither declaration
// order, block scope nor the local's name changes that. (b) Both descend
// dot products emit y,z,x where shipped has z,y,x. No term order changes
// it. It follows the PROLOGUE's spelling: three component stores give z,y,x
// here, but the prologue then loses its shipped scheduling. Allocation, not
// structure.
// AVP2: in neither engine, whose box move is ws_MoveBoxQ544 above (each
// image's ws_MoveBoxPassesQ544 calls the four-argument move; Primal Hunt's
// lithtech.exe has no three-argument one). NOLF's, kept for reference.
#if 0
int ws_MoveBox(uint16 nBlock, CBounceState* pState, int bRetry)
{
	CPolyTest test;
#ifdef LT_SERVER_TIMERS
	CountAdder cntAdder(&g_nPhysStepTime);
#endif
	LTVector   vBack;
	LTVector   vNorm;
	LTVector   vSaveStart;
	LTVector   vAt;
	LTVector   vRetryStart;
	LTVector   vRetryEnd;
	LTVector   vHitStart;
	LTVector   vHitEnd;

	WorldPolyRef* pRecs;
	WorldPolyRef* pRec;
	WSNode*       pNode;
	uint32        nProjected;
	uint16*       pTop;
	uint16        nNode;
	uint16        nChild;
	int           nStart, nEnd, nSide;
	// UNSIGNED: the bound test at 0x10003d0a is `jb`.
	uint32        nTries;
	WorldPolyOwner* pOwner;
	LTVector*     pPts0;
	LTVector*     pPts1;
	WorldPolyOwner* pPushOwner;
	LTVector*     pPushPts0;
	LTVector*     pPushPts1;
	LTPlane*      pPlane;
	float         fDist;

	vAt.Init(g_vMoveBase.x + g_vMoveStart.x, g_vMoveBase.y + g_vMoveStart.y,
	         g_vMoveBase.z + g_vMoveStart.z);

	test.m_Start.m_pPoints = g_SweepPoints[0];
	test.m_End.m_pPoints   = g_SweepPoints[1];

	test.m_Start.m_pSphere = &g_SphereAtDelta;
	test.m_End.m_pSphere   = &g_SphereAtOffset;

	test.m_End.m_pProjected   = &nProjected;
	test.m_Start.m_pProjected = &nProjected;

	test.m_Start.m_vMin = vAt - g_pMoveState->m_vDims;
	test.m_Start.m_vMax = vAt + g_pMoveState->m_vDims;

	vAt.Init(g_vMoveBase.x + g_vMoveEnd.x, g_vMoveBase.y + g_vMoveEnd.y,
	         g_vMoveBase.z + g_vMoveEnd.z);

	test.m_End.m_vMin = vAt - g_pMoveState->m_vDims;
	test.m_End.m_vMax = vAt + g_pMoveState->m_vDims;

	pRecs = g_pMoveState->m_pWorldBsp->GetPolyRefs();

	pTop  = g_BlockStackQ534;
	nNode = nBlock;

	for (;;)
	{
		if (nNode == WS_NODE_EMPTY || nNode == WS_NODE_SOLID)
		{
			if (pTop == g_BlockStackQ534)
			{
				return 1;
			}

			pTop--;
			nNode = *pTop;
		}

		g_nPhysNodes++;

		pNode  = &g_pPBlock->m_pNodes[nNode];
		pRec   = &pRecs[pNode->m_nPoly];
		pPlane = ws_GetPlaneQ506(pRec->m_pOwner);

		fDist = pPlane->DistTo(g_BoxSphere.m_vPoint);

		if (fDist > g_BoxSphere.m_fRadius)
		{
			nNode = pNode->m_nChild[1];
			continue;
		}

		if (fDist < -g_BoxSphere.m_fRadius)
		{
			nNode = pNode->m_nChild[0];
			continue;
		}

		nProjected = 1;

		test.m_End.m_pPlane   = ws_GetPlaneQ506(pRec->m_pOwner);
		test.m_Start.m_pPlane = test.m_End.m_pPlane;

		nStart = g_BoxProjectFns[pRec->m_nProject](&test.m_Start);
		nEnd   = g_BoxProjectFns[pRec->m_nProject](&test.m_End);

		if (nStart == nEnd)
		{
			if (nStart == WS_STRADDLE)
			{
				pOwner = pRec->m_pOwner;
				pPts1  = test.m_End.m_pPoints;
				pPts0  = test.m_Start.m_pPoints;

				if (w_IsPolyBlocking(pOwner, LTNULL, LTNULL) &&
				    !ws_IsBoxOutsidePoly(pOwner, pPts0, pPts1))
				{
					if (bRetry)
					{
						vBack.x = g_vMoveStart.x - g_vMoveEnd.x;
						vBack.y = g_vMoveStart.y - g_vMoveEnd.y;
						vBack.z = g_vMoveStart.z - g_vMoveEnd.z;

						vNorm = vBack;
						vNorm.Norm();

						ws_HandleImpact(g_pMoveState->m_pInfo, pState,
						                g_pMoveState->m_pObject, g_pMoveState->m_hObject,
						                g_pMoveState->m_pWorldBsp, (WorldPoly*)pRec,
						                &g_pMoveState->m_pInfo->m_Plane.m_Normal);

						pState->m_nHits++;

						vSaveStart = g_vMoveStart;

						for (nTries = 0; nTries < WS_MAX_BOX_PUSHES; nTries++)
						{
							pPushOwner = pRec->m_pOwner;
							pPushPts1  = test.m_End.m_pPoints;
							pPushPts0  = test.m_Start.m_pPoints;

							if (!w_IsPolyBlocking(pPushOwner, LTNULL, LTNULL))
								break;

							if (ws_IsBoxOutsidePoly(pPushOwner, pPushPts0, pPushPts1))
								break;

							g_vMoveStart.x = vNorm.x + g_vMoveStart.x;
							g_vMoveStart.y = vNorm.y + g_vMoveStart.y;
							g_vMoveStart.z = vNorm.z + g_vMoveStart.z;

							g_vMoveEnd.x = vNorm.x + g_vMoveEnd.x;
							g_vMoveEnd.y = vNorm.y + g_vMoveEnd.y;
							g_vMoveEnd.z = vNorm.z + g_vMoveEnd.z;

							if (!ws_ShiftBox())
								return 0;

						}

						g_vMoveStart = vSaveStart;

						if (nTries == WS_MAX_BOX_PUSHES)
						{
							g_vMoveEnd.x = vSaveStart.x;
							g_vMoveEnd.y = vSaveStart.y;
							g_vMoveEnd.z = vSaveStart.z;

							goto Failed;
						}

						vRetryStart.Init(g_vMoveBase.x + g_vMoveStart.x, g_vMoveBase.y + g_vMoveStart.y,
						         g_vMoveBase.z + g_vMoveStart.z);

						test.m_Start.m_vMin = vRetryStart - g_pMoveState->m_vDims;
						test.m_Start.m_vMax = vRetryStart + g_pMoveState->m_vDims;

						vRetryEnd.Init(g_vMoveBase.x + g_vMoveEnd.x, g_vMoveBase.y + g_vMoveEnd.y,
						         g_vMoveBase.z + g_vMoveEnd.z);

						test.m_End.m_vMin = vRetryEnd - g_pMoveState->m_vDims;
						test.m_End.m_vMax = vRetryEnd + g_pMoveState->m_vDims;
					}
					else
					{
						g_pMoveState->m_pInfo->m_Plane =
						    *ws_GetPlaneQ506(pRec->m_pOwner);

						g_pMoveState->m_pInfo->m_hObject = g_pMoveState->m_hObject;

						g_pMoveState->m_pInfo->m_hPoly =
						    g_pMoveState->m_pWorldBsp->MakeHPoly((WorldPoly*)pRec);

						pState->m_nHits++;
					}
				}

				pPlane = ws_GetPlaneQ506(pRec->m_pOwner);

				fDist = g_vMoveStart.z * pPlane->m_Normal.z +
				        g_vMoveStart.y * pPlane->m_Normal.y +
				        g_vMoveStart.x * pPlane->m_Normal.x - pPlane->m_Dist;

				if (fDist > WS_PLANE_SLOP)
					nSide = 1;
				else
					nSide = 0;

				nChild = pNode->m_nChild[nSide == 0];

				if (nChild != WS_NODE_EMPTY && nChild != WS_NODE_SOLID)
				{
					*pTop = nChild;
					pTop++;
				}

				nNode = pNode->m_nChild[nSide];
			}
			else
			{
				nNode = pNode->m_nChild[nStart];
			}
		}
		else
		{
			if (nStart == WS_INFRONT && w_IsPolyBlocking(pRec->m_pOwner, LTNULL, LTNULL))
			{
				if (!ws_HitPoly((WorldPoly*)pRec, pState, &test))
					goto Failed;

				vHitStart.Init(g_vMoveBase.x + g_vMoveStart.x, g_vMoveBase.y + g_vMoveStart.y,
				         g_vMoveBase.z + g_vMoveStart.z);

				test.m_Start.m_vMin = vHitStart - g_pMoveState->m_vDims;
				test.m_Start.m_vMax = vHitStart + g_pMoveState->m_vDims;

				vHitEnd.Init(g_vMoveBase.x + g_vMoveEnd.x, g_vMoveBase.y + g_vMoveEnd.y,
				         g_vMoveBase.z + g_vMoveEnd.z);

				test.m_End.m_vMin = vHitEnd - g_pMoveState->m_vDims;
				test.m_End.m_vMax = vHitEnd + g_pMoveState->m_vDims;

				nNode = pNode->m_nChild[1];

				continue;
			}

			pPlane = ws_GetPlaneQ506(pRec->m_pOwner);

			fDist = g_vMoveStart.z * pPlane->m_Normal.z +
			        g_vMoveStart.y * pPlane->m_Normal.y +
			        g_vMoveStart.x * pPlane->m_Normal.x - pPlane->m_Dist;

			if (fDist > WS_PLANE_SLOP)
				nSide = 1;
			else
				nSide = 0;

			nChild = pNode->m_nChild[nSide == 0];

			if (nChild != WS_NODE_EMPTY && nChild != WS_NODE_SOLID)
			{
				*pTop = nChild;
				pTop++;
			}

			nNode = pNode->m_nChild[nSide];
		}
	}

Failed:
	return 0;
}
#endif
