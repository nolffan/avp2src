// ----------------------------------------------------------------------- //
//
// MODULE  : Polygon.cpp
//
// PURPOSE : CPolygon - Implementation
//
//           Primal Hunt (Third Law). No source shipped: written from
//           PHObject.lto, 0x10155610..0x10155b45.
//
// ----------------------------------------------------------------------- //

#include "stdafx.h"

#ifdef PRIMAL_HUNT

#include "Polygon.h"

#define SIDE_FRONT	1
#define SIDE_BACK	2
#define SIDE_ON		4

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CPolygon::Clip (0x10155650)
//
//	PURPOSE:	Keeps the part of the polygon in front of pPlane. False
//				when none of it is.
//
// ----------------------------------------------------------------------- //

// NEAR (637 of 675 bytes): the shipped loop over the new polygon keeps
// j + 1 and j * 4 as two induction variables and spills 'this' and
// nVerts. Ours keeps j and nVerts in registers. The frame is 4 bytes
// smaller (the nVerts slot). The dot product order, the local CPolygon,
// the interpolation and the copy are as shipped. Tried: one 'i' for both
// loops, m_nVerts in the modulo.
LTBOOL CPolygon::Clip(LTPlane* pPlane, LTFLOAT fEpsilon)
{
	int anCounts[5];
	anCounts[0] = 0;
	anCounts[1] = 0;
	anCounts[2] = 0;
	anCounts[3] = 0;
	anCounts[4] = 0;

	LTFLOAT afDists[POLYGON_MAX_VERTS];
	int anSides[POLYGON_MAX_VERTS];

	int nVerts = m_nVerts;

	int i;
	for (i = 0; i < nVerts; i++)
	{
		LTVector vVert = m_Verts[i];
		LTFLOAT fDist = vVert.y*pPlane->m_Normal.y;
		fDist += vVert.x*pPlane->m_Normal.x;
		fDist += vVert.z*pPlane->m_Normal.z;
		fDist -= pPlane->m_Dist;
		afDists[i] = fDist;

		if (fDist > fEpsilon)
		{
			anSides[i] = SIDE_FRONT;
		}
		else if (fDist < -fEpsilon)
		{
			anSides[i] = SIDE_BACK;
		}
		else
		{
			anSides[i] = SIDE_ON;
		}

		anCounts[anSides[i]]++;
	}

	afDists[i] = afDists[0];
	anSides[i] = anSides[0];

	if (!anCounts[SIDE_FRONT]) return LTFALSE;
	if (!anCounts[SIDE_BACK]) return LTTRUE;

	CPolygon newPoly;
	newPoly.m_nVerts = 0;

	for (int j = 0; j < nVerts; j++)
	{
		LTVector* pVert = &m_Verts[j];

		if (anSides[j] == SIDE_ON)
		{
			newPoly.m_Verts[newPoly.m_nVerts++] = *pVert;
			continue;
		}

		if (anSides[j] == SIDE_FRONT)
		{
			newPoly.m_Verts[newPoly.m_nVerts++] = *pVert;
		}

		int iNext = (j + 1) % nVerts;
		if (anSides[iNext] == SIDE_ON || anSides[iNext] == anSides[j]) continue;

		LTFLOAT fDist = afDists[j];
		LTFLOAT fT = fDist / (fDist - afDists[iNext]);
		newPoly.m_Verts[newPoly.m_nVerts++] = *pVert + (m_Verts[iNext] - *pVert) * fT;
	}

	if (newPoly.m_nVerts > 0)
	{
		// (the mask is in the shipped code: a dword copy with no byte tail)
		memcpy(m_Verts, newPoly.m_Verts, (newPoly.m_nVerts * sizeof(LTVector)) & ~3);
	}

	m_nVerts = newPoly.m_nVerts;

	return LTTRUE;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CPolygon::GetBounds (0x10155900)
//
// ----------------------------------------------------------------------- //

void CPolygon::GetBounds(LTVector* pvMin, LTVector* pvMax)
{
	pvMin->Init(999999.0f, 999999.0f, 999999.0f);
	pvMax->Init(-999999.0f, -999999.0f, -999999.0f);

	for (int i = 0; i < m_nVerts; i++)
	{
		LTVector* pVert = &m_Verts[i];

		if (pVert->x < pvMin->x) pvMin->x = pVert->x;
		if (pVert->y < pvMin->y) pvMin->y = pVert->y;
		if (pVert->z < pvMin->z) pvMin->z = pVert->z;

		if (pVert->x > pvMax->x) pvMax->x = pVert->x;
		if (pVert->y > pvMax->y) pvMax->y = pVert->y;
		if (pVert->z > pvMax->z) pvMax->z = pVert->z;
	}
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CPolygon::GetSpan (0x101559b0)
//
//	PURPOSE:	The polygon's widest pair of vertices (in x and z), both
//				put at its middle height, and that height
//
// ----------------------------------------------------------------------- //

LTBOOL CPolygon::GetSpan(LTVector* pvStart, LTVector* pvEnd, LTFLOAT* pfHeight)
{
	int iStart = -1;
	int iEnd = -1;
	LTFLOAT fBest = -1.0f;

	int i, j;
	for (i = 0; i < m_nVerts; i++)
	{
		for (j = 0; j < m_nVerts; j++)
		{
			LTFLOAT fX = m_Verts[i].x - m_Verts[j].x;
			LTFLOAT fZ = m_Verts[i].z - m_Verts[j].z;
			LTFLOAT fDistSqr = fZ*fZ + fX*fX;

			if (fDistSqr > fBest)
			{
				fBest = fDistSqr;
				iStart = i;
				iEnd = j;
			}
		}
	}

	if (iStart == -1 || iEnd == -1) return LTFALSE;

	fBest = -1.0f;
	*pvStart = m_Verts[iStart];
	*pvEnd = m_Verts[iEnd];

	int iLow = -1;
	int iHigh = -1;
	for (i = 0; i < m_nVerts; i++)
	{
		for (j = 0; j < m_nVerts; j++)
		{
			LTFLOAT fHeight = (LTFLOAT)fabs(m_Verts[i].y - m_Verts[j].y);
			if (fHeight > fBest)
			{
				fBest = fHeight;
				iLow = i;
				iHigh = j;
			}
		}
	}

	LTFLOAT fMiddle;
	if (m_Verts[iLow].y < m_Verts[iHigh].y)
	{
		fMiddle = fBest * 0.5f + m_Verts[iLow].y;
	}
	else
	{
		fMiddle = fBest * 0.5f + m_Verts[iHigh].y;
	}

	pvStart->y = fMiddle;
	pvEnd->y = fMiddle;
	*pfHeight = fMiddle;

	return LTTRUE;
}

#endif // PRIMAL_HUNT
