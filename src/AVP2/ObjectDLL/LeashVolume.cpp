// ----------------------------------------------------------------------- //
//
// MODULE  : LeashVolume.cpp
//
// PURPOSE : LeashVolume and CLeashVolume - Implementation
//
//           Primal Hunt (Third Law). No source shipped: written from
//           PHObject.lto, 0x10152070..0x10153b3c.
//
// ----------------------------------------------------------------------- //

#include "stdafx.h"

#ifdef PRIMAL_HUNT

#include "LeashVolume.h"
#include "Polygon.h"

// AIBase.cpp's (0x10142070)
LTBOOL PlanesEqual(const LTPlane& a, const LTPlane& b, LTFLOAT fNormalTol, LTFLOAT fDistTol);

BEGIN_CLASS(LeashVolume)
	ADD_STRINGPROP_FLAG(Group_Name, "", 0)
	ADD_BOOLPROP_FLAG(Show_Volume, LTFALSE, 0)
	ADD_BOOLPROP_FLAG(Show_Portals, LTFALSE, 0)
END_CLASS_DEFAULT(LeashVolume, GameBase, LTNULL, LTNULL)

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	LeashVolume::LeashVolume (0x10152260)
//
// ----------------------------------------------------------------------- //

LeashVolume::LeashVolume() : GameBase(OT_WORLDMODEL)
{
	m_bShowVolume = LTFALSE;
	m_bShowPortals = LTFALSE;
	m_pNode = LTNULL;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	LeashVolume::~LeashVolume (0x101522a0)
//
// ----------------------------------------------------------------------- //

LeashVolume::~LeashVolume()
{
	Term();
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	LeashVolume::OnPrecreate (0x101522f0)
//
// ----------------------------------------------------------------------- //

uint32 LeashVolume::OnPrecreate(void *pData, uint32 nInfo)
{
	ObjectCreateStruct* pStruct = (ObjectCreateStruct*)pData;

	pStruct->m_Flags = 0;
	pStruct->m_ObjectType = OT_WORLDMODEL;
	SAFE_STRCPY(pStruct->m_Filename, pStruct->m_Name);
	pStruct->m_SkinName[0] = '\0';

	LTBOOL bValue;
	g_pLTServer->GetPropBool("Show_Volume", &bValue);
	m_bShowVolume = bValue;

	g_pLTServer->GetPropBool("Show_Portals", &bValue);
	m_bShowPortals = bValue;

	g_pLTServer->GetPropString("Group_Name", m_szGroupName, 127);

	return LTFALSE;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	LeashVolume::OnInitialUpdate (0x10152390)
//
// ----------------------------------------------------------------------- //

uint32 LeashVolume::OnInitialUpdate(uint32 nInfo)
{
	m_pNode = new CLeashVolume;
	if (!m_pNode)
	{
		g_pLTServer->RemoveObject(m_hObject);
		return LTTRUE;
	}

	m_pNode->m_bShowPortals = m_bShowPortals;

	if (!BuildHull())
	{
		g_pLTServer->RemoveObject(m_hObject);
		return LTTRUE;
	}

	HLEASHGROUP hGroup = LeashMgr_GetGroup(m_szGroupName);
	if (hGroup)
	{
		LeashMgr_AddNode(hGroup, m_pNode);
	}

	return LTFALSE;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	LeashVolume::PostStartWorld (0x10152480)
//
// ----------------------------------------------------------------------- //

void LeashVolume::PostStartWorld(uint8 nLoadGameFlags)
{
	g_pLTServer->RemoveObject(m_hObject);
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	LeashVolume::Term (0x101524b0, folded)
//
// ----------------------------------------------------------------------- //

void LeashVolume::Term()
{
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	LeashVolume::BuildHull (0x101524c0)
//
//	PURPOSE:	Builds the node's faces from the brush's polies
//
// ----------------------------------------------------------------------- //

LTBOOL LeashVolume::BuildHull()
{
	m_pNode->ClearFaces();

	HPOLY hPoly = INVALID_HPOLY;
	while (g_pLTServer->GetNextWMPoly(&hPoly, m_hObject) == LT_OK)
	{
		CLeashFace* pFace = new CLeashFace;

		LTPlane* pPlane = LTNULL;
		LTVector aVerts[LEASHFACE_MAX_VERTS];
		uint32 nVerts;
		g_pLTServer->Common()->GetPolyInfo(hPoly, &pPlane, aVerts, LEASHFACE_MAX_VERTS, &nVerts);

		if (nVerts > LEASHFACE_MAX_VERTS)
		{
			// RETAIL BUG: pFace is leaked.
			g_pLTServer->CPrint("LeashVolume \"%s\" has more than %i vertices in a face.",
				g_pLTServer->GetObjectName(m_hObject), LEASHFACE_MAX_VERTS);
			return LTFALSE;
		}

		for (uint32 i = 0; i < nVerts; i++)
		{
			pFace->m_Verts.AddHead(new LTVector(aVerts[i]));
		}

		pFace->m_Plane = *pPlane;
		pFace->CalcBounds();
		pFace->BuildEdgePlanes();

		m_pNode->ExtendBounds(pFace->m_vMin, pFace->m_vMax);

		if (!m_pNode->MergeFace(pFace))
		{
			m_pNode->m_Faces.AddTail(pFace);
		}
		else if (pFace)
		{
			delete pFace;
		}
	}

	TLinkNode<CLeashFace*>* pNext;
	for (TLinkNode<CLeashFace*>* pNode = m_pNode->m_Faces.GetHead(); pNode != m_pNode->m_Faces.GetEnd(); pNode = pNext)
	{
		pNext = pNode->m_pNext;

		if (m_pNode->MergeFace(pNode->m_Data) == LTTRUE)
		{
			m_pNode->m_Faces.Delete(pNode);
		}
	}

	if (!m_pNode->IsConvex(0.1f))
	{
		g_pLTServer->CPrint("LeashVolume \"%s\" is not convex!", g_pLTServer->GetObjectName(m_hObject));
		return LTFALSE;
	}

	return LTTRUE;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CLeashVolume::CLeashVolume (0x101527d0)
//
// ----------------------------------------------------------------------- //

CLeashVolume::CLeashVolume() : CLeashNode(LEASHNODE_VOLUME)
{
	LTVector vMin(999999.0f, 999999.0f, 999999.0f);
	LTVector vMax;
	vMax.Init(-999999.0f, -999999.0f, -999999.0f);

	m_Box = LeashBox(vMin, vMax);
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CLeashVolume::GetPos (0x101528d0)
//
// ----------------------------------------------------------------------- //

LTVector CLeashVolume::GetPos()
{
	return (m_Box.m_vMin + m_Box.m_vMax) * 0.5f;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CLeashVolume::SetPos (0x10152940)
//
// ----------------------------------------------------------------------- //

void CLeashVolume::SetPos(const LTVector& vPos)
{
	LTVector vExtents = (m_Box.m_vMax - m_Box.m_vMin) * 0.5f;

	LeashBox box;
	box.m_vMin.x = vPos.x - vExtents.x;
	box.m_vMin.y = vPos.y - vExtents.y;
	box.m_vMin.z = vPos.z - vExtents.z;
	box.m_vMax.x = vExtents.x + vPos.x;
	box.m_vMax.y = vExtents.y + vPos.y;
	box.m_vMax.z = vExtents.z + vPos.z;

	m_Box = box;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CLeashVolume::~CLeashVolume (0x10152a10)
//
// ----------------------------------------------------------------------- //

CLeashVolume::~CLeashVolume()
{
	CLeashVolume::Term();
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CLeashVolume::Term (0x10152b00)
//
// ----------------------------------------------------------------------- //

void CLeashVolume::Term()
{
	CLeashNode::Term();
	ClearFaces();
	ClearPortals();
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CLeashVolume::ClearFaces (0x10152b20)
//
// ----------------------------------------------------------------------- //

void CLeashVolume::ClearFaces()
{
	m_Faces.DeleteAll();
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CLeashVolume::ClearPortals (0x10152b70)
//
// ----------------------------------------------------------------------- //

void CLeashVolume::ClearPortals()
{
	m_Portals.DeleteAll();
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CLeashVolume::MergeFace (0x10152bc0)
//
//	PURPOSE:	Merges pFace into a coplanar face it shares an edge with
//
// ----------------------------------------------------------------------- //

LTBOOL CLeashVolume::MergeFace(CLeashFace* pFace)
{
	for (TLinkNode<CLeashFace*>* pNode = m_Faces.GetHead(); pNode != m_Faces.GetEnd(); pNode = pNode->m_pNext)
	{
		if (pNode->m_Data == pFace) continue;

		if (PlanesEqual(pNode->m_Data->m_Plane, pFace->m_Plane, 0.001f, 0.1f) &&
			pNode->m_Data->Merge(pFace) == LTTRUE)
		{
			return LTTRUE;
		}
	}

	return LTFALSE;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CLeashVolume::IsConvex (0x10152c20)
//
// ----------------------------------------------------------------------- //

LTBOOL CLeashVolume::IsConvex(LTFLOAT fEpsilon)
{
	for (TLinkNode<CLeashFace*>* pA = m_Faces.GetHead(); pA != m_Faces.GetEnd(); pA = pA->m_pNext)
	{
		for (TLinkNode<CLeashFace*>* pB = m_Faces.GetHead(); pB != m_Faces.GetEnd(); pB = pB->m_pNext)
		{
			if (pB == pA) continue;

			CLeashFace* pFace = pB->m_Data;
			for (TLinkNode<LTVector*>* pVert = pFace->m_Verts.GetHead(); pVert != pFace->m_Verts.GetEnd(); pVert = pVert->m_pNext)
			{
				LTVector vVert = *pVert->m_Data;
				if (pA->m_Data->m_Plane.DistTo(vVert) > fEpsilon) return LTFALSE;
			}
		}
	}

	return LTTRUE;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CLeashVolume::Contains (0x10152cd0)
//
// ----------------------------------------------------------------------- //

LTBOOL CLeashVolume::Contains(const LTVector& vPos, LTFLOAT fEpsilon)
{
	if (!(vPos >= m_Box.m_vMin && vPos <= m_Box.m_vMax)) return LTFALSE;

	for (TLinkNode<CLeashFace*>* pNode = m_Faces.GetHead(); pNode != m_Faces.GetEnd(); pNode = pNode->m_pNext)
	{
		LTVector vVert = vPos;
		if (pNode->m_Data->m_Plane.DistTo(vVert) >= fEpsilon) return LTFALSE;
	}

	return LTTRUE;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CLeashVolume::FindNeighbor (0x10152d90)
//
// ----------------------------------------------------------------------- //

CLeashNode* CLeashVolume::FindNeighbor(const LTVector& vPos, LTFLOAT fEpsilon)
{
	for (TLinkNode<CLeashNode*>* pNode = m_Neighbors.GetHead(); pNode != m_Neighbors.GetEnd(); pNode = pNode->m_pNext)
	{
		if (pNode->m_Data->m_nType == LEASHNODE_VOLUME && pNode->m_Data->Contains(vPos, fEpsilon) == LTTRUE)
		{
			return pNode->m_Data;
		}
	}

	return LTNULL;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CLeashVolume::FindPortals (0x10152de0)
//
// ----------------------------------------------------------------------- //

void CLeashVolume::FindPortals(CLeashVolume* pOther)
{
	for (TLinkNode<CLeashFace*>* pNode = m_Faces.GetHead(); pNode != m_Faces.GetEnd(); pNode = pNode->m_pNext)
	{
		LTBOOL bNeighbor = LTFALSE;

		for (TLinkNode<CLeashFace*>* pOtherNode = pOther->m_Faces.GetHead(); pOtherNode != pOther->m_Faces.GetEnd(); pOtherNode = pOtherNode->m_pNext)
		{
			CLeashFace* pFace = pNode->m_Data;
			CLeashFace* pOtherFace = pOtherNode->m_Data;

			if ((LTFLOAT)fabs(pFace->m_Plane.m_Dist + pOtherFace->m_Plane.m_Dist) > 0.1) continue;

			LTVector vNormal = pOtherFace->m_Plane.m_Normal;
			if (vNormal.Dot(pFace->m_Plane.m_Normal) > -0.9999f) continue;

			if (AddPortal(pFace, pOtherFace, pOther))
			{
				if (!bNeighbor)
				{
					AddNeighbor(pOther);
				}

				bNeighbor = LTTRUE;
			}
		}
	}
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CLeashVolume::AddPortal (0x10152ec0)
//
//	PURPOSE:	Makes a portal of where pOtherFace meets pFace
//
// ----------------------------------------------------------------------- //

LTBOOL CLeashVolume::AddPortal(CLeashFace* pFace, CLeashFace* pOtherFace, CLeashVolume* pOther)
{
	if (!pFace || !pOtherFace) return LTFALSE;

	LTFLOAT fMin = pFace->m_vMin.x;
	if (fMin < pOtherFace->m_vMin.x) fMin = pOtherFace->m_vMin.x;

	LTFLOAT fMax = pFace->m_vMax.x;
	if (fMax > pOtherFace->m_vMax.x) fMax = pOtherFace->m_vMax.x;

	if (fMax < fMin) return LTFALSE;

	CPolygon poly;
	poly.m_nVerts = 0;
	for (TLinkNode<LTVector*>* pVert = pOtherFace->m_Verts.GetHead(); pVert != pOtherFace->m_Verts.GetEnd(); pVert = pVert->m_pNext)
	{
		poly.m_Verts[poly.m_nVerts++] = *pVert->m_Data;
	}

	for (TLinkNode<LTPlane*>* pPlane = pFace->m_EdgePlanes.GetHead(); pPlane != pFace->m_EdgePlanes.GetEnd(); pPlane = pPlane->m_pNext)
	{
		if (!poly.Clip(pPlane->m_Data, 0.001f)) return LTFALSE;
	}

	LTVector vStart, vEnd;
	LTFLOAT fHeight;
	if (!poly.GetSpan(&vStart, &vEnd, &fHeight)) return LTFALSE;

	CLeashPortal* pPortal = new CLeashPortal;
	if (!pPortal) return LTFALSE;

	pPortal->m_pNeighbor = pOther;
	pPortal->m_Plane = pFace->m_Plane;
	pPortal->m_vStart = vStart;
	pPortal->m_vDir = vEnd - vStart;
	pPortal->m_fLength = pPortal->m_vDir.Mag();
	pPortal->m_fHeight = fHeight;
	pPortal->m_vDir *= 1.0f / pPortal->m_fLength;

	poly.GetBounds(&pPortal->m_vMin, &pPortal->m_vMax);

	m_Portals.AddHead(pPortal);

	return LTTRUE;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CLeashVolume::GetExitOffset (0x10153120)
//
// ----------------------------------------------------------------------- //

LTVector CLeashVolume::GetExitOffset(const LTVector& vPos, CLeashPortal* pPortal, LTFLOAT fRadius)
{
	return vPos + (pPortal->m_Plane.m_Normal * fRadius) * -1.0f;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CLeashVolume::GetEntryOffset (0x10153190)
//
// ----------------------------------------------------------------------- //

LTVector CLeashVolume::GetEntryOffset(const LTVector& vPos, CLeashPortal* pPortal, LTFLOAT fRadius)
{
	return vPos + pPortal->m_Plane.m_Normal * fRadius;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CLeashVolume::GetExitPos (0x101531e0)
//
//	PURPOSE:	The point on the portal to pTo nearest vPos, kept a
//				diameter in from the portal's ends, and fRadius back
//				inside this volume
//
// ----------------------------------------------------------------------- //

LTBOOL CLeashVolume::GetExitPos(CLeashNode* pTo, const LTVector& vPos, LTVector* pvExit, LTFLOAT fRadius)
{
	if (pTo->m_nType != LEASHNODE_VOLUME) return LTFALSE;

	CLeashPortal* pPortal = GetPortal(pTo);
	if (!pPortal) return LTFALSE;

	LTFLOAT fDiameter = fRadius + fRadius;
	if (pPortal->m_fLength - fDiameter < 0.0f) return LTFALSE;

	LTFLOAT fSpan = pPortal->m_fLength - (fDiameter + fDiameter);
	if (fSpan < 0.0f)
	{
		*pvExit = pPortal->m_vStart + pPortal->m_vDir * (pPortal->m_fLength * 0.5f);
	}
	else
	{
		LTVector vA = pPortal->m_vStart + pPortal->m_vDir * fDiameter;
		LTVector vB = pPortal->m_vStart + pPortal->m_vDir * fSpan;

		LTVector vToPos(vPos.x - vA.x, vPos.y - vA.y, vPos.z - vA.z);
		LTVector vSeg(vB.x - vA.x, vB.y - vA.y, vB.z - vA.z);

		LTFLOAT fSegMagSqr = vSeg.MagSqr();
		LTFLOAT fT = vSeg.Dot(vToPos) / fSegMagSqr;

		if (fT < 0.0f)
		{
			*pvExit = vA;
		}
		else if (fT > 1.0f)
		{
			*pvExit = vB;
		}
		else
		{
			LTFLOAT fInv = 1.0f / fSegMagSqr;
			*pvExit = vA + LTVector(vSeg.x * fInv, vSeg.y * fInv, vSeg.z * fInv) * (fT * fSegMagSqr);
		}
	}

	*pvExit = GetExitOffset(*pvExit, pPortal, fRadius);

	return LTTRUE;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CLeashVolume::GetEntryPos (0x101534f0)
//
// ----------------------------------------------------------------------- //

LTBOOL CLeashVolume::GetEntryPos(CLeashNode* pTo, const LTVector& vPos, LTVector* pvEntry, LTFLOAT fRadius)
{
	if (pTo->m_nType != LEASHNODE_VOLUME) return LTFALSE;

	CLeashPortal* pPortal = GetPortal(pTo);
	if (!pPortal) return LTFALSE;

	LTFLOAT fDiameter = fRadius + fRadius;
	if (pPortal->m_fLength - fDiameter < 0.0f) return LTFALSE;

	LTFLOAT fSpan = pPortal->m_fLength - (fDiameter + fDiameter);
	if (fSpan < 0.0f)
	{
		*pvEntry = pPortal->m_vStart + pPortal->m_vDir * (pPortal->m_fLength * 0.5f);
	}
	else
	{
		LTVector vA = pPortal->m_vStart + pPortal->m_vDir * fDiameter;
		LTVector vB = pPortal->m_vStart + pPortal->m_vDir * fSpan;

		LTVector vToPos(vPos.x - vA.x, vPos.y - vA.y, vPos.z - vA.z);
		LTVector vSeg(vB.x - vA.x, vB.y - vA.y, vB.z - vA.z);

		LTFLOAT fSegMagSqr = vSeg.MagSqr();
		LTFLOAT fT = vSeg.Dot(vToPos) / fSegMagSqr;

		if (fT < 0.0f)
		{
			*pvEntry = vA;
		}
		else if (fT > 1.0f)
		{
			*pvEntry = vB;
		}
		else
		{
			LTFLOAT fInv = 1.0f / fSegMagSqr;
			*pvEntry = vA + LTVector(vSeg.x * fInv, vSeg.y * fInv, vSeg.z * fInv) * (fT * fSegMagSqr);
		}
	}

	*pvEntry = GetEntryOffset(*pvEntry, pPortal, fRadius);

	return LTTRUE;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CLeashVolume::ExtendBounds (0x10153800)
//
// ----------------------------------------------------------------------- //

void CLeashVolume::ExtendBounds(const LTVector& vMin, const LTVector& vMax)
{
	LeashBox box(m_Box.GetCenter(), m_Box.GetExtents());

	if (vMin.x < box.m_vMin.x) box.m_vMin.x = vMin.x;
	if (vMin.y < box.m_vMin.y) box.m_vMin.y = vMin.y;
	if (vMin.z < box.m_vMin.z) box.m_vMin.z = vMin.z;

	if (vMax.x > box.m_vMax.x) box.m_vMax.x = vMax.x;
	if (vMax.y > box.m_vMax.y) box.m_vMax.y = vMax.y;
	if (vMax.z > box.m_vMax.z) box.m_vMax.z = vMax.z;

	LTVector vExtents = box.GetExtents();
	LTVector vCenter(box.m_vMin.x + vExtents.x, box.m_vMin.y + vExtents.y, box.m_vMin.z + vExtents.z);

	m_Box = LeashBox(vCenter, vExtents);
}

#endif // PRIMAL_HUNT
