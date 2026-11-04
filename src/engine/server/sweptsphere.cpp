// *********************************************************************** //
//  MODULE:   SWEPTSPHERE (.CPP) -- reconstruction
//  PURPOSE:  The sphere physics' collision against world polygons: a sphere
//            swept along a segment against a polygon's face, its edges and
//            its corners, the push-out of a resting sphere, and the
//            re-orientation of an object to a new up vector
//            ("SweptSphereOrient"). Names ours throughout; Jupiter's
//            intersectsweptsphere.cpp is a later rewrite of the same tests
//            and shares only the idea.
//
//  server.dll 0x10008bf3..0x10009988 (/O1), lithtech.exe
//  0x00427110..0x0042847b (/O2), right after servermath.cpp's code in both
//  (its PH <string> initializer ends at 0x10008bf3 / 0x00427110). AvP2
//  1.0.9.6 carries the same module (the Orient string at server.dll
//  0x10070950, lithtech.exe 0x004d180c).
//
//  Every caller is in worldsim.cpp's sphere physics, which is not
//  reconstructed (server.dll 0x1000353d, 0x100035a1, 0x10003eaa,
//  0x10003ef7, 0x10003fe6, 0x100040ae, 0x10004181), so the entry points
//  carry this file's suffix.
// *********************************************************************** //

#include <math.h>
#include <float.h>
#include "ltbasedefs.h"
#include "iltmath.h"
#include "ltobject_internal.h"
#include "de_error.h"

// PH: the C++ library's <string>, whose guarded std::ctype<unsigned
// short>::id initializer ends the module in Primal Hunt (server.dll
// 0x10009967: guard 0x1008ec84; lithtech.exe 0x00428450).
#ifdef PRIMAL_HUNT
#include <string>
#endif


// What the last face test hit: -1 nothing, 0 the face, 1 an edge, 2 a
// corner (0x1007da2c, initialised -1; written only by sp_SweptSphereToPoly).
static int g_nSweptSphereHit = -1;

static LTBOOL sp_SweptSphereToEdge(LTVector* pStart, LTVector* pEnd, float fRadius,
	LTVector* pEdge0, LTVector* pEdge1, float* pT, LTVector* pNormal);
static LTBOOL sp_SweptSphereToPoint(LTVector* pStart, LTVector* pEnd, float fRadius,
	LTVector* pPoint, float* pT, LTVector* pNormal);


// server.dll 0x100098ce (an out-of-line COMDAT there; lithtech.exe expands
// it into both callers): the two roots of a*t^2 + b*t + c, false for a
// degenerate or a negative discriminant.
inline bool sp_SolveQuadratic(float a, float b, float c, float* pRoot1, float* pRoot2)
{
	if (a == 0.0f)
		return false;

	float fDisc = b * b - a * c * 4.0f;

	a = 1.0f / (a * 2.0f);

	if (fDisc == 0.0f)
	{
		*pRoot1 = *pRoot2 = -(a * b);
	}
	else if (fDisc > 0.0f)
	{
		fDisc = (float)sqrt(fDisc);

		*pRoot1 = (fDisc - b) * a;
		*pRoot2 = (-b - fDisc) * a;
	}
	else
	{
		return false;
	}

	return true;
}


// server.dll 0x10008bf3 (1077 bytes), lithtech.exe 0x00427110 (1633): the
// sphere swept from pStart to pEnd against one polygon. The face first: the
// polygon's own normal from its first three vertices, the point where the
// sphere first touches the plane, and an inside test against every edge.
// Failing that every edge and every corner, keeping the nearest.
//
// NEAR in both images. The shipped code CALLS LTVector's three-float
// constructor inside most of the vector operators here (server.dll
// 0x100071e2, lithtech.exe 0x004140a0) while expanding the same operators
// elsewhere in the function, and lithtech.exe calls Dot, operator-,
// operator* and Norm out of line as well; the SDK header we build against
// expands them all. Shape, frame and flow are as shipped.
static LTBOOL sp_SweptSphereToPoly(LTVector* pStart, LTVector* pEnd, float fRadius,
	WorldPolyOwner* pPoly, float* pT, LTVector* pNormal)
{
	g_nSweptSphereHit = -1;

	LTVector vNormal = -((*pPoly->m_Vertices[1].m_pPos - *pPoly->m_Vertices[0].m_pPos).Cross(
		*pPoly->m_Vertices[2].m_pPos - *pPoly->m_Vertices[0].m_pPos));
	vNormal.Norm();

	float fDistStart = (*pStart - *pPoly->m_Vertices[0].m_pPos).Dot(vNormal);
	float fDistEnd = (*pEnd - *pPoly->m_Vertices[0].m_pPos).Dot(vNormal);

	float fT;

	if (fDistStart >= fRadius && fDistEnd <= fRadius && fDistStart - fDistEnd != 0.0f)
	{
		fT = (fDistStart - fRadius) / (fDistStart - fDistEnd);

		LTVector vDir = *pEnd - *pStart;
		LTVector vPtInPlane = vDir * fT + *pStart - vNormal * fRadius;

		uint32 nVerts = pPoly->m_nVertices;
		LTVector* pPrev = pPoly->m_Vertices[nVerts - 1].m_pPos;
		uint32 i;

		for (i = 0; i < nVerts; i++)
		{
			LTVector* pCur = pPoly->m_Vertices[i].m_pPos;

			LTVector vEdge = *pCur - *pPrev;

			if (vEdge.Cross(vPtInPlane - *pPrev).Dot(vNormal) > 0.0f)
				break;

			pPrev = pCur;
		}

		if (i >= nVerts)
		{
			*pT = fT;
			*pNormal = vNormal;
			g_nSweptSphereHit = 0;
			return LTTRUE;
		}
	}

	if (0.0f > fDistStart)
		return LTFALSE;

	uint32 nVerts = pPoly->m_nVertices;
	LTVector* pPrev = pPoly->m_Vertices[nVerts - 1].m_pPos;
	LTBOOL bHit = LTFALSE;

	*pT = 1.0f;

	for (uint32 i = 0; i < nVerts; i++)
	{
		LTVector* pCur = pPoly->m_Vertices[i].m_pPos;

		if (sp_SweptSphereToEdge(pStart, pEnd, fRadius, pPrev, pCur, &fT, &vNormal))
		{
			if (fT < *pT)
			{
				*pT = fT;
				*pNormal = vNormal;
				bHit = LTTRUE;
				g_nSweptSphereHit = 1;
			}
		}

		if (sp_SweptSphereToPoint(pStart, pEnd, fRadius, pPrev, &fT, &vNormal))
		{
			if (fT < *pT)
			{
				*pT = fT;
				*pNormal = vNormal;
				bHit = LTTRUE;
				g_nSweptSphereHit = 2;
			}
		}

		pPrev = pCur;
	}

	if (bHit)
		return LTTRUE;

	return LTFALSE;
}


// server.dll 0x10009028 (679 bytes), lithtech.exe 0x00427780 (914): the
// sphere against the segment pEdge0..pEdge1 -- the infinite line first,
// solved as a quadratic in t, then whether the touch lands inside the
// segment.
//
// NEAR in both images, for the reason sp_SweptSphereToPoly gives: the
// shipped edge direction and start offset are constructor CALLS (0x100090af,
// 0x100090dd), ours expand. A CVectorInit-style stand-in for those two moves
// server.dll from 16 mnemonic blocks to 13 and was not kept.
static LTBOOL sp_SweptSphereToEdge(LTVector* pStart, LTVector* pEnd, float fRadius,
	LTVector* pEdge0, LTVector* pEdge1, float* pT, LTVector* pNormal)
{
	LTVector vDir = *pEnd - *pStart;
	LTVector vEdge = *pEdge1 - *pEdge0;
	float fEdgeLen = vEdge.Mag();
	LTVector vEdgeDir = vEdge * (1.0f / fEdgeLen);
	LTVector vRel = *pStart - *pEdge0;

	float fDirDot = vEdgeDir.Dot(vDir);
	float fRelDot = vEdgeDir.Dot(vRel);

	float fT1, fT2;

	if (!sp_SolveQuadratic(vDir.Dot(vDir) - fDirDot * fDirDot,
		(vRel.Dot(vDir) - fRelDot * fDirDot) * 2.0f,
		vRel.Dot(vRel) - fRelDot * fRelDot - fRadius * fRadius,
		&fT1, &fT2))
	{
		return LTFALSE;
	}

	if (fT1 == fT2)
		return LTFALSE;

	float fT = (fT1 < fT2) ? fT1 : fT2;
	*pT = fT;

	if (0.0f > fT || fT > 1.0f)
		return LTFALSE;

	float fDistOnLine = fT * fDirDot + fRelDot;

	if (0.0f > fDistOnLine || fDistOnLine > fEdgeLen)
		return LTFALSE;

	LTVector vCenter = vDir * fT + vRel;
	*pNormal = vCenter - vEdgeDir * fDistOnLine;
	pNormal->Norm();

	return LTTRUE;
}


// server.dll 0x100092cf (324 bytes), lithtech.exe 0x00427b20 (607): the
// sphere against one point.
//
// NEAR: server.dll 320 of 324 bytes, 4 mnemonic blocks -- the FPU order of
// the hit-centre product (shipped multiplies y and z by t from memory and
// pops t after) and one faddp scheduled before its push. VEC_DOT for the
// middle coefficient is what gives the shipped z, y, x term order (Dot
// gives y, z, x there); every VEC_DOT / VEC_MAGSQR / Dot combination of
// the three coefficients was compiled and this one is closest.
static LTBOOL sp_SweptSphereToPoint(LTVector* pStart, LTVector* pEnd, float fRadius,
	LTVector* pPoint, float* pT, LTVector* pNormal)
{
	LTVector vDir = *pEnd - *pStart;
	LTVector vRel = *pStart - *pPoint;

	float fT1, fT2;

	if (!sp_SolveQuadratic(vDir.Dot(vDir), VEC_DOT(vDir, vRel) * 2.0f,
		vRel.Dot(vRel) - fRadius * fRadius, &fT1, &fT2))
	{
		return LTFALSE;
	}

	if (fT1 == fT2)
		return LTFALSE;

	float fT = (fT1 < fT2) ? fT1 : fT2;
	*pT = fT;

	if (0.0f > fT || fT > 1.0f)
		return LTFALSE;

	*pNormal = vDir * fT + vRel;
	pNormal->Norm();

	return LTTRUE;
}


// server.dll 0x10009413 (481 bytes), lithtech.exe 0x00427d80 (650): push a
// resting sphere out of every solid polygon it sinks into, a little past
// the surface, up to ten times; answers how many pushes were left.
//
// NEAR in both images. The shipped loop tests the count at the top on the
// first pass too (0x10009426, a `cmp %ax` against the constant 10 VC6 did
// not fold), and its frame is 0x18 bytes deeper (0x8c: a copy of the edge
// at -0x80 before the cross product). The `for` with the decrement as its
// increment and the plane read through the polygon at every use are what
// the bytes show; while/goto shapes were further off.
LTBOOL sp_PushSphereOutQ533(LTVector* pPos, float fRadius, WorldPolyOwner** ppPolys, int nPolys)
{
	uint16 nTries;
	int i;

	for (nTries = 10; nTries > 0; nTries--)
	{
		for (i = 0; i < nPolys; i++)
		{
			WorldPolyOwner* pPoly = ppPolys[i];

			if (!(pPoly->m_pSurface->m_Flags & SURF_SOLID))
				continue;

			float fDist = pPoly->m_pPlane->m_Normal.Dot(*pPos) - pPoly->m_pPlane->m_Dist;

			if (fRadius + 0.1f > fDist)
			{
				LTVector vPt = *pPos - pPoly->m_pPlane->m_Normal * fDist;
				uint32 j;

				for (j = 0; j < pPoly->m_nVertices; j++)
				{
					LTVector vPrev = *pPoly->m_Vertices[(j ? j : pPoly->m_nVertices) - 1].m_pPos;
					LTVector vEdge = *pPoly->m_Vertices[j].m_pPos - vPrev;
					LTVector vToPt = vPt - vPrev;
					LTVector vEdgeNormal = vEdge.Cross(pPoly->m_pPlane->m_Normal);

					if (vEdgeNormal.Dot(vToPt) < -0.001f)
						break;
				}

				if (j >= pPoly->m_nVertices)
				{
					*pPos += pPoly->m_pPlane->m_Normal * (fRadius - fDist + 0.2f);
					break;
				}
			}
		}

		if (i >= nPolys)
			break;
	}

	return nTries;
}


// server.dll 0x100095f4 (175 bytes), lithtech.exe 0x00428010 (250): the
// sphere swept against a list of polygons, keeping the nearest hit and the
// plane normal of the polygon it was on (twelve bytes, 0x10009673); *pbOrient
// is cleared when that polygon's surface refuses sphere orientation.
LTBOOL sp_SweptSphereToPolysQ533(LTVector* pStart, LTVector* pEnd, float fRadius,
	WorldPolyOwner** ppPolys, int nPolys, LTVector* pHitNormal, LTVector* pPlaneNormal,
	float* pT, LTBOOL* pbOrient)
{
	LTBOOL bHit = LTFALSE;
	float fT;
	LTVector vNormal;

	*pT = 1.0f;
	*pbOrient = LTTRUE;

	for (int i = 0; i < nPolys; i++)
	{
		WorldPolyOwner* pPoly = ppPolys[i];

		if (!(pPoly->m_pSurface->m_Flags & SURF_SOLID))
			continue;

		if (sp_SweptSphereToPoly(pStart, pEnd, fRadius, pPoly, &fT, &vNormal))
		{
			if (fT < *pT)
			{
				*pT = fT;
				*pHitNormal = vNormal;
				*pPlaneNormal = pPoly->m_pPlane->m_Normal;
				bHit = LTTRUE;

				if (*pbOrient && (pPoly->m_pSurface->m_Flags & SURF_NOWALLWALK))
					*pbOrient = LTFALSE;
			}
		}
	}

	return bHit;
}


// server.dll 0x100096a3 (555 bytes), lithtech.exe 0x00428110 (829): turn
// the object so that its up vector is *pUp, carrying its velocity and
// acceleration round with it.
//
// Byte-identical in server.dll. The two scratch rotations are ONE array:
// as two variables the second constructor spends lithtech.exe's /O2
// inline budget and the Mag inside the axis' Norm becomes a call. NEAR in
// lithtech.exe (829 of 829, 0 mnemonic blocks): the shipped frame swaps
// the two rotations and the first right/forward pair (0x00428121,
// 0x0042824e) against our layout; declaration order and names do not move
// VC6's slots.
LTBOOL sp_SweptSphereOrientQ533(LTVector* pUp, LTObject* pObj)
{
	ILTMath math;
	LTVector vRight, vUp, vForward;
	LTRotation* pRot = &pObj->m_Rotation;

	math.GetRotationVectors(*pRot, vRight, vUp, vForward);

	LTVector vAxis = vUp.Cross(-*pUp);
	vAxis.Norm();

	float fTheta = (float)acos(vUp.Dot(*pUp));

	if (_isnan(fTheta))
		return LTFALSE;

	LTRotation rTemp[2];
	LTVector vR, vU, vF;
	float fMag;

	math.AlignRotation(rTemp[0], pObj->m_Velocity, vUp);
	math.RotateAroundAxis(rTemp[0], vAxis, fTheta);
	fMag = pObj->m_Velocity.Mag();
	math.GetRotationVectors(rTemp[0], vR, vU, vF);
	pObj->m_Velocity = vF * fMag;

	math.AlignRotation(rTemp[1], pObj->m_Acceleration, vUp);
	math.RotateAroundAxis(rTemp[1], vAxis, fTheta);
	fMag = pObj->m_Acceleration.Mag();
	math.GetRotationVectors(rTemp[1], vR, vU, vF);
	pObj->m_Acceleration = vF * fMag;

	math.RotateAroundAxis(*pRot, vAxis, fTheta);
	math.GetRotationVectors(*pRot, vR, vU, vF);

	if (_isnan(vU.x) || _isnan(vU.y) || _isnan(vU.z))
	{
		dsi_ServerError("SweptSphereOrient Rotation Invalid!!  Axis = %f %f %f, Theta",
			vAxis.x, vAxis.y, vAxis.z, fTheta);
	}

	return LTTRUE;
}
