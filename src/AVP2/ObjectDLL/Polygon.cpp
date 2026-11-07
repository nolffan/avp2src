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

LTBOOL CPolygon::Clip(LTPlane* pPlane, LTFLOAT fEpsilon)
{
	int anCounts[5];
	anCounts[0] = 0;
	anCounts[1] = 0;
	anCounts[2] = 0;
	anCounts[3] = 0;
	anCounts[4] = 0;

	LTFLOAT afDists[POLYGON_MAX_VERTS + 1];
	int anSides[POLYGON_MAX_VERTS + 1];

	int i;
	for (i = 0; i < m_nVerts; i++)
	{
		LTVector vVert = m_Verts[i];
		LTFLOAT fDist = vVert.y*pPlane->m_Normal.y + vVert.x*pPlane->m_Normal.x + vVert.z*pPlane->m_Normal.z - pPlane->m_Dist;
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

	LTVector aNewVerts[POLYGON_MAX_VERTS];
	int nNewVerts = 0;

	for (i = 0; i < m_nVerts; i++)
	{
		LTVector* pVert = &m_Verts[i];

		if (anSides[i] == SIDE_ON)
		{
			aNewVerts[nNewVerts++] = *pVert;
			continue;
		}

		if (anSides[i] == SIDE_FRONT)
		{
			aNewVerts[nNewVerts++] = *pVert;
		}

		int iNext = (i + 1) % m_nVerts;
		if (anSides[iNext] == SIDE_ON || anSides[iNext] == anSides[i]) continue;

		LTFLOAT fT = afDists[i] / (afDists[i] - afDists[iNext]);
		LTVector vVert = *pVert;
		aNewVerts[nNewVerts++] = vVert + (m_Verts[iNext] - vVert) * fT;
	}

	if (nNewVerts > 0)
	{
		memcpy(m_Verts, aNewVerts, nNewVerts * sizeof(LTVector));
	}

	m_nVerts = nNewVerts;

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
		if (m_Verts[i].x < pvMin->x) pvMin->x = m_Verts[i].x;
		if (m_Verts[i].y < pvMin->y) pvMin->y = m_Verts[i].y;
		if (m_Verts[i].z < pvMin->z) pvMin->z = m_Verts[i].z;

		if (m_Verts[i].x > pvMax->x) pvMax->x = m_Verts[i].x;
		if (m_Verts[i].y > pvMax->y) pvMax->y = m_Verts[i].y;
		if (m_Verts[i].z > pvMax->z) pvMax->z = m_Verts[i].z;
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
