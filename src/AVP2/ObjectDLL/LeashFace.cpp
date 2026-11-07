// ----------------------------------------------------------------------- //
//
// MODULE  : LeashFace.cpp
//
// PURPOSE : CLeashFace - Implementation
//
//           Primal Hunt (Third Law). No source shipped: written from
//           PHObject.lto, 0x1014f6b0..0x1015024c.
//
// ----------------------------------------------------------------------- //

#include "stdafx.h"

#ifdef PRIMAL_HUNT

#include "LeashFace.h"

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CLeashFace::~CLeashFace (0x1014f6b0)
//
// ----------------------------------------------------------------------- //

CLeashFace::~CLeashFace()
{
	m_Verts.DeleteAll();
	m_EdgePlanes.DeleteAll();
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CLeashFace::CalcBounds (0x1014f7a0)
//
// ----------------------------------------------------------------------- //

void CLeashFace::CalcBounds()
{
	m_vMin.Init(999999.0f, 999999.0f, 999999.0f);
	m_vMax.Init(-999999.0f, -999999.0f, -999999.0f);

	for (TLinkNode<LTVector*>* pNode = m_Verts.GetHead(); pNode != m_Verts.GetEnd(); pNode = pNode->m_pNext)
	{
		LTVector* pVert = pNode->m_Data;

		if (pVert->x < m_vMin.x) m_vMin.x = pVert->x;
		if (pVert->y < m_vMin.y) m_vMin.y = pVert->y;
		if (pVert->z < m_vMin.z) m_vMin.z = pVert->z;

		if (pVert->x > m_vMax.x) m_vMax.x = pVert->x;
		if (pVert->y > m_vMax.y) m_vMax.y = pVert->y;
		if (pVert->z > m_vMax.z) m_vMax.z = pVert->z;
	}
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CLeashFace::IsFacingNormal (0x1014f840)
//
//	PURPOSE:	Does the winding of the first three vertices agree with
//				the face's plane?
//
// ----------------------------------------------------------------------- //

LTBOOL CLeashFace::IsFacingNormal()
{
	TLinkNode<LTVector*>* pNode = m_Verts.GetHead();
	LTVector v0 = *pNode->m_Data;
	pNode = pNode->m_pNext;
	LTVector v1 = *pNode->m_Data;
	LTVector v2 = *pNode->m_pNext->m_Data;

	LTVector vA = v0 - v1;
	vA.Norm();

	LTVector vB = v2 - v1;
	vB.Norm();

	LTVector vCross = vA.Cross(vB);
	if (vCross.Dot(m_Plane.m_Normal) < 0.999f) return LTFALSE;

	return LTTRUE;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CLeashFace::BuildEdgePlanes (0x1014f9d0)
//
// ----------------------------------------------------------------------- //

void CLeashFace::BuildEdgePlanes()
{
	m_EdgePlanes.DeleteAll();

	TLinkNode<LTVector*>* pNode = m_Verts.GetHead();
	TLinkNode<LTVector*>* pNext = pNode->m_pNext;

	LTBOOL bFlip = !IsFacingNormal();

	while (pNode != m_Verts.GetEnd())
	{
		LTPlane* pPlane = new LTPlane;

		LTVector vA = *pNode->m_Data;
		LTVector vB = *pNext->m_Data;

		LTVector vEdge = vB - vA;
		vEdge.Norm();

		pPlane->m_Normal = vEdge.Cross(m_Plane.m_Normal);
		pPlane->m_Dist = pPlane->m_Normal.Dot(vA);

		if (bFlip == LTTRUE)
		{
			pPlane->m_Normal *= -1.0f;
			pPlane->m_Dist = -pPlane->m_Dist;
		}

		m_EdgePlanes.AddHead(pPlane);

		pNext = pNext->m_pNext;
		if (pNext == m_Verts.GetEnd())
		{
			pNext = m_Verts.GetHead();
		}

		pNode = pNode->m_pNext;
	}
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CLeashFace::Merge (0x1014fbe0)
//
//	PURPOSE:	Joins pOther into this face across an edge they share
//
// ----------------------------------------------------------------------- //

LTBOOL CLeashFace::Merge(CLeashFace* pOther)
{
	LTVector aVerts[LEASHFACE_MAX_VERTS];
	LTVector aOtherVerts[LEASHFACE_MAX_VERTS];

	int nVerts = 0;
	TLinkNode<LTVector*>* pNode;
	for (pNode = m_Verts.GetHead(); pNode != m_Verts.GetEnd(); pNode = pNode->m_pNext)
	{
		aVerts[nVerts++] = *pNode->m_Data;
	}

	int nOtherVerts = 0;
	for (pNode = pOther->m_Verts.GetHead(); pNode != pOther->m_Verts.GetEnd(); pNode = pNode->m_pNext)
	{
		aOtherVerts[nOtherVerts++] = *pNode->m_Data;
	}

	LTBOOL bReverse = LTFALSE;
	int nStart, nEnd;

	for (int i = 0; i < nVerts; i++)
	{
		int iNext = (i + 1) % nVerts;

		for (int j = 0; j < nOtherVerts; j++)
		{
			if (aVerts[i] == aOtherVerts[j])
			{
				int jPrev = j ? j - 1 : nOtherVerts - 1;
				int jNext = (j + 1) % nOtherVerts;

				if (aVerts[iNext] == aOtherVerts[jNext])
				{
					bReverse = LTTRUE;
					nStart = i;
					nEnd = (j + 1) % nOtherVerts;
				}
				else if (aVerts[iNext] == aOtherVerts[jPrev])
				{
					nStart = i;
					nEnd = jPrev;
				}
				else
				{
					continue;
				}

				m_Verts.DeleteAll();

				int k;
				for (k = iNext; k != nStart; k = (k + 1) % nVerts)
				{
					m_Verts.AddHead(new LTVector(aVerts[k]));
				}

				if (!bReverse)
				{
					for (k = j; k != nEnd; k = (k + 1) % nOtherVerts)
					{
						m_Verts.AddHead(new LTVector(aOtherVerts[k]));
					}
				}
				else
				{
					for (k = j; k != nEnd; k = k ? k - 1 : nOtherVerts - 1)
					{
						m_Verts.AddHead(new LTVector(aOtherVerts[k]));
					}
				}

				RemoveCollinearVerts();
				CalcBounds();
				BuildEdgePlanes();

				return LTTRUE;
			}
		}
	}

	return LTFALSE;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CLeashFace::RemoveCollinearVerts (0x1014ff80)
//
// ----------------------------------------------------------------------- //

int CLeashFace::RemoveCollinearVerts()
{
	LTBOOL abRemove[LEASHFACE_MAX_VERTS];
	memset(abRemove, 0, sizeof(abRemove));

	LTVector aVerts[LEASHFACE_MAX_VERTS];

	int nVerts = 0;
	for (TLinkNode<LTVector*>* pNode = m_Verts.GetHead(); pNode != m_Verts.GetEnd(); pNode = pNode->m_pNext)
	{
		aVerts[nVerts++] = *pNode->m_Data;
	}

	int nRemoved = 0;

	int i;
	for (i = 0; i < nVerts; i++)
	{
		LTVector vStart = aVerts[i];
		LTVector vDir = aVerts[(i + 1) % nVerts] - vStart;

		if (vDir.x == 0.0f && vDir.y == 0.0f && vDir.z == 0.0f)
		{
			abRemove[(i + 1) % nVerts] = LTTRUE;
			nRemoved++;
		}

		vDir.Norm();

		for (int j = 1; j < nVerts - 1; j++)
		{
			LTVector vFrom = aVerts[i];
			LTVector vNextDir = aVerts[(i + j + 1) % nVerts] - vFrom;
			vNextDir.Norm();

			if (vNextDir.Dot(vDir) < 0.999999f) break;

			abRemove[(i + j) % nVerts] = LTTRUE;
			nRemoved++;
		}
	}

	if (!nRemoved) return 0;

	m_Verts.DeleteAll();

	for (i = 0; i < nVerts; i++)
	{
		if (abRemove[i] != LTTRUE)
		{
			m_Verts.AddTail(new LTVector(aVerts[i]));
		}
	}

	return nRemoved;
}

#endif // PRIMAL_HUNT
