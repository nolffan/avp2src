// ----------------------------------------------------------------------- //
//
// MODULE  : LeashMgr.cpp
//
// PURPOSE : The leash manager - Implementation
//
//           Primal Hunt (Third Law). No source shipped: written from
//           PHObject.lto, 0x10150250..0x10151926.
//
// ----------------------------------------------------------------------- //

#include "stdafx.h"

#ifdef PRIMAL_HUNT

#include "LeashMgr.h"
#include "LeashPoint.h"
#include "LeashVolume.h"

CLeashMgr g_LeashMgr;	// 0x10337788; $E 0x10150250

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CLeashGroup::CLeashGroup (0x10150290)
//
// ----------------------------------------------------------------------- //

CLeashGroup::CLeashGroup()
{
	m_szName[0] = '\0';
	m_nNextID = 0;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CLeashGroup::~CLeashGroup (0x101502c0)
//
// ----------------------------------------------------------------------- //

CLeashGroup::~CLeashGroup()
{
	Clear();
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CLeashGroup::AddNode (0x10150390)
//
// ----------------------------------------------------------------------- //

LTBOOL CLeashGroup::AddNode(CLeashNode* pNode)
{
	switch (pNode->m_nType)
	{
		case LEASHNODE_NONE:	return LTFALSE;
		case LEASHNODE_POINT:	return AddPoint(pNode);
		case LEASHNODE_VOLUME:	return AddVolume(pNode);
	}

	return LTFALSE;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CLeashGroup::AddPoint (0x101503c0)
//
// ----------------------------------------------------------------------- //

LTBOOL CLeashGroup::AddPoint(CLeashNode* pNode)
{
	if (!pNode) return LTFALSE;

	int nID = m_nNextID++;
	pNode->m_nID = nID;
	m_Points.AddHead((CLeashPoint*)pNode);

	return LTTRUE;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CLeashGroup::AddVolume (0x101503f0)
//
// ----------------------------------------------------------------------- //

LTBOOL CLeashGroup::AddVolume(CLeashNode* pNode)
{
	if (!pNode) return LTFALSE;

	int nID = m_nNextID++;
	pNode->m_nID = nID;
	m_Volumes.AddTail((CLeashVolume*)pNode);

	return LTTRUE;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CLeashGroup::Init (0x10150420)
//
// ----------------------------------------------------------------------- //

LTBOOL CLeashGroup::Init()
{
	BuildNeighbors();
	BuildRoutes();

	return LTTRUE;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CLeashGroup::Clear (0x10150440)
//
// ----------------------------------------------------------------------- //

void CLeashGroup::Clear()
{
	m_Points.DeleteAll();
	m_Volumes.DeleteAll();
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CLeashGroup::BuildRoutes (0x101504d0)
//
// ----------------------------------------------------------------------- //

LTBOOL CLeashGroup::BuildRoutes()
{
	TLinkNode<CLeashPoint*>* pPoint;
	for (pPoint = m_Points.GetHead(); pPoint != m_Points.GetEnd(); pPoint = pPoint->m_pNext)
	{
		BuildPointRoutes(pPoint->m_Data);
	}

	TLinkNode<CLeashVolume*>* pVolume;
	for (pVolume = m_Volumes.GetHead(); pVolume != m_Volumes.GetEnd(); pVolume = pVolume->m_pNext)
	{
		BuildVolumeRoutes(pVolume->m_Data);
	}

	return LTFALSE;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CLeashGroup::GetPoint (0x10150510)
//
// ----------------------------------------------------------------------- //

CLeashNode* CLeashGroup::GetPoint(int nID)
{
	for (TLinkNode<CLeashPoint*>* pNode = m_Points.GetHead(); pNode != m_Points.GetEnd(); pNode = pNode->m_pNext)
	{
		if (pNode->m_Data->m_nID == nID) return pNode->m_Data;
	}

	return LTNULL;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CLeashGroup::GetVolume (0x10150540)
//
// ----------------------------------------------------------------------- //

CLeashNode* CLeashGroup::GetVolume(int nID)
{
	for (TLinkNode<CLeashVolume*>* pNode = m_Volumes.GetHead(); pNode != m_Volumes.GetEnd(); pNode = pNode->m_pNext)
	{
		if (pNode->m_Data->m_nID == nID) return pNode->m_Data;
	}

	return LTNULL;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CLeashGroup::BuildPointRoutes (0x10150570)
//
//	PURPOSE:	Dijkstra out from pDest: leaves each point's next step
//				toward pDest in pDest's route table
//
// ----------------------------------------------------------------------- //

LTBOOL CLeashGroup::BuildPointRoutes(CLeashNode* pDest)
{
	TLinkNode<CLeashPoint*>* pNode;
	for (pNode = m_Points.GetHead(); pNode != m_Points.GetEnd(); pNode = pNode->m_pNext)
	{
		pNode->m_Data->m_bVisited = LTFALSE;
		pNode->m_Data->m_fDist = 1000000000.0f;
		pNode->m_Data->m_nPrev = -1;
	}

	pDest->m_fDist = 0.0f;

	while (m_Points.GetCount() > 0)
	{
		int nVisited = 0;
		TLinkNode<CLeashPoint*>* pBest = LTNULL;

		for (pNode = m_Points.GetHead(); pNode != m_Points.GetEnd(); pNode = pNode->m_pNext)
		{
			if (pNode->m_Data->m_bVisited)
			{
				nVisited++;
			}
			else if (!pBest || (pNode->m_Data->m_fDist <= pBest->m_Data->m_fDist &&
				pNode->m_Data->m_fDist != 1000000000.0f))
			{
				pBest = pNode;
			}
		}

		if (nVisited >= m_Points.GetCount() || !pBest) break;

		pBest->m_Data->m_bVisited = LTTRUE;

		CLeashNode* pBestNode = pBest->m_Data;
		for (TLinkNode<CLeashNode*>* pNeighbor = pBestNode->m_Neighbors.GetHead();
			pNeighbor != pBestNode->m_Neighbors.GetEnd(); pNeighbor = pNeighbor->m_pNext)
		{
			if (pNeighbor->m_Data->m_bVisited) continue;

			LTVector vNeighborPos = pNeighbor->m_Data->GetPos();
			LTFLOAT fDist = (pBestNode->GetPos() - vNeighborPos).Mag() + pBest->m_Data->m_fDist;

			if (pNeighbor->m_Data->m_fDist > fDist)
			{
				pNeighbor->m_Data->m_fDist = fDist;
				pNeighbor->m_Data->m_nPrev = pBest->m_Data->m_nID;
			}
		}
	}

	pDest->AllocRoutes(m_Points.GetCount());

	int i = 0;
	for (pNode = m_Points.GetHead(); pNode != m_Points.GetEnd() && i < m_Points.GetCount(); pNode = pNode->m_pNext, i++)
	{
		if (i == pDest->m_nID)
		{
			pDest->SetRoute(i, -1, 0);
			continue;
		}

		int nNext = i;
		int nHops = 0;

		CLeashNode* pPoint = GetPoint(i);
		if (pPoint)
		{
			int nPrev = pPoint->m_nPrev;
			while (nPrev != pDest->m_nID)
			{
				nNext = nPrev;
				if (nNext == -1) break;

				nPrev = GetPoint(nNext)->m_nPrev;
				nHops++;
			}
		}

		pDest->SetRoute(i, nNext, nHops);
	}

	return LTFALSE;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CLeashGroup::BuildVolumeRoutes (0x101507f0)
//
// ----------------------------------------------------------------------- //

LTBOOL CLeashGroup::BuildVolumeRoutes(CLeashNode* pDest)
{
	TLinkNode<CLeashVolume*>* pNode;
	for (pNode = m_Volumes.GetHead(); pNode != m_Volumes.GetEnd(); pNode = pNode->m_pNext)
	{
		pNode->m_Data->m_bVisited = LTFALSE;
		pNode->m_Data->m_fDist = 1000000000.0f;
		pNode->m_Data->m_nPrev = -1;
	}

	pDest->m_fDist = 0.0f;

	while (m_Volumes.GetCount() > 0)
	{
		int nVisited = 0;
		TLinkNode<CLeashVolume*>* pBest = LTNULL;

		for (pNode = m_Volumes.GetHead(); pNode != m_Volumes.GetEnd(); pNode = pNode->m_pNext)
		{
			if (pNode->m_Data->m_bVisited)
			{
				nVisited++;
			}
			else if (!pBest || (pNode->m_Data->m_fDist <= pBest->m_Data->m_fDist &&
				pNode->m_Data->m_fDist != 1000000000.0f))
			{
				pBest = pNode;
			}
		}

		if (nVisited >= m_Volumes.GetCount() || !pBest) break;

		pBest->m_Data->m_bVisited = LTTRUE;

		CLeashNode* pBestNode = pBest->m_Data;
		for (TLinkNode<CLeashNode*>* pNeighbor = pBestNode->m_Neighbors.GetHead();
			pNeighbor != pBestNode->m_Neighbors.GetEnd(); pNeighbor = pNeighbor->m_pNext)
		{
			if (pNeighbor->m_Data->m_bVisited) continue;

			LTVector vNeighborPos = pNeighbor->m_Data->GetPos();
			LTFLOAT fDist = pBestNode->GetPos().Dist(vNeighborPos) + pBest->m_Data->m_fDist;

			if (pNeighbor->m_Data->m_fDist > fDist)
			{
				pNeighbor->m_Data->m_fDist = fDist;
				pNeighbor->m_Data->m_nPrev = pBest->m_Data->m_nID;
			}
		}
	}

	pDest->AllocRoutes(m_Volumes.GetCount());

	int i = 0;
	for (pNode = m_Volumes.GetHead(); pNode != m_Volumes.GetEnd() && i < m_Volumes.GetCount(); pNode = pNode->m_pNext, i++)
	{
		if (i == pDest->m_nID)
		{
			pDest->SetRoute(i, -1, 0);
			continue;
		}

		int nNext = i;
		int nHops = 0;

		CLeashNode* pVolume = GetVolume(i);
		if (pVolume)
		{
			int nPrev = pVolume->m_nPrev;
			while (nPrev != pDest->m_nID)
			{
				nNext = nPrev;
				if (nNext == -1) break;

				nPrev = GetVolume(nNext)->m_nPrev;
				nHops++;
			}
		}

		pDest->SetRoute(i, nNext, nHops);
	}

	return LTFALSE;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CLeashGroup::BuildNeighbors (0x10150a30)
//
// ----------------------------------------------------------------------- //

void CLeashGroup::BuildNeighbors()
{
	BuildPointNeighbors();
	BuildVolumeNeighbors();
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CLeashGroup::BuildPointNeighbors (0x10150a50)
//
// ----------------------------------------------------------------------- //

void CLeashGroup::BuildPointNeighbors()
{
	for (TLinkNode<CLeashPoint*>* pA = m_Points.GetHead(); pA != m_Points.GetEnd(); pA = pA->m_pNext)
	{
		for (TLinkNode<CLeashPoint*>* pB = m_Points.GetHead(); pB != m_Points.GetEnd(); pB = pB->m_pNext)
		{
			if (pA == pB) continue;

			CLeashPoint* pPoint = pA->m_Data;
			CLeashPoint* pOther = pB->m_Data;
			if (pPoint->Overlaps(pOther) == LTTRUE)
			{
				pPoint->AddNeighbor(pOther);
			}
		}
	}
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CLeashGroup::BuildVolumeNeighbors (0x10150aa0)
//
// ----------------------------------------------------------------------- //

void CLeashGroup::BuildVolumeNeighbors()
{
	for (TLinkNode<CLeashVolume*>* pA = m_Volumes.GetHead(); pA != m_Volumes.GetEnd(); pA = pA->m_pNext)
	{
		for (TLinkNode<CLeashVolume*>* pB = m_Volumes.GetHead(); pB != m_Volumes.GetEnd(); pB = pB->m_pNext)
		{
			if (pB == pA) continue;
			if (pA->m_Data->IsNeighbor(pB->m_Data) == LTTRUE) continue;

			LeashBox boxB = pB->m_Data->m_Box;
			LeashBox boxA = pA->m_Data->m_Box;

			if (boxB.m_vMin.x <= boxA.m_vMax.x && boxB.m_vMin.y <= boxA.m_vMax.y && boxB.m_vMin.z <= boxA.m_vMax.z &&
				boxB.m_vMax.x >= boxA.m_vMin.x && boxB.m_vMax.y >= boxA.m_vMin.y && boxB.m_vMax.z >= boxA.m_vMin.z)
			{
				pA->m_Data->FindPortals(pB->m_Data);
			}
		}
	}
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CLeashGroup::FindPoint (0x10150b90)
//
// ----------------------------------------------------------------------- //

CLeashNode* CLeashGroup::FindPoint(const LTVector& vPos)
{
	CLeashNode* pFound = LTNULL;

	for (TLinkNode<CLeashPoint*>* pNode = m_Points.GetHead(); pNode != m_Points.GetEnd(); pNode = pNode->m_pNext)
	{
		LTFLOAT fX = pNode->m_Data->GetPos().x - vPos.x;
		LTFLOAT fZ = pNode->m_Data->GetPos().z - vPos.z;

		CLeashPoint* pPoint = pNode->m_Data;
		if (fZ*fZ + fX*fX <= pPoint->m_fRadius*pPoint->m_fRadius)
		{
			if (pPoint->GetPos().y - vPos.y <= pPoint->m_fRadius)
			{
				pFound = pPoint;
			}
		}
	}

	return pFound;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CLeashGroup::FindVolume (0x10150c40)
//
// ----------------------------------------------------------------------- //

CLeashNode* CLeashGroup::FindVolume(const LTVector& vPos)
{
	for (TLinkNode<CLeashVolume*>* pNode = m_Volumes.GetHead(); pNode != m_Volumes.GetEnd(); pNode = pNode->m_pNext)
	{
		if (pNode->m_Data->Contains(vPos, 0.1f)) return pNode->m_Data;
	}

	return LTNULL;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CLeashGroup::GetNextNode (0x10150c80)
//
//	PURPOSE:	The next node on the way from vFrom's node to vTo's
//
// ----------------------------------------------------------------------- //

LTBOOL CLeashGroup::GetNextNode(const LTVector& vFrom, const LTVector& vTo, int* pnNext, int* pnStart, int* pnDest)
{
	*pnNext = -1;

	CLeashNode* pDest = FindNode(vTo);
	if (!pDest) return LTFALSE;

	*pnDest = pDest->m_nID;

	CLeashNode* pStart = FindNode(vFrom);
	if (!pStart) return LTFALSE;

	*pnStart = pStart->m_nID;

	if (pStart->m_nID == *pnDest)
	{
		*pnNext = pStart->m_nID;
		return LTTRUE;
	}

	LeashRoute* pRoute = pStart->GetRoute(pDest->m_nID);
	if (!pRoute) return LTFALSE;

	*pnNext = pRoute->m_nNext;
	return (pRoute->m_nNext >= 0);
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CLeashGroup::GetExitPos (0x10150d40)
//
// ----------------------------------------------------------------------- //

LTBOOL CLeashGroup::GetExitPos(int nFrom, int nTo, const LTVector& vPos, LTVector* pvExit, LTFLOAT fRadius)
{
	CLeashNode* pFrom = GetNode(nFrom);
	if (!pFrom) return LTFALSE;

	CLeashNode* pTo = GetNode(nTo);
	if (!pTo) return LTFALSE;

	return pFrom->GetExitPos(pTo, vPos, pvExit, fRadius);
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CLeashGroup::GetEntryPos (0x10150db0)
//
// ----------------------------------------------------------------------- //

LTBOOL CLeashGroup::GetEntryPos(int nFrom, int nTo, const LTVector& vPos, LTVector* pvEntry, LTFLOAT fRadius)
{
	CLeashNode* pFrom = GetNode(nFrom);
	if (!pFrom) return LTFALSE;

	CLeashNode* pTo = GetNode(nTo);
	if (!pTo) return LTFALSE;

	return pFrom->GetEntryPos(pTo, vPos, pvEntry, fRadius);
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CLeashMgr::CLeashMgr (0x10150e20)
//
// ----------------------------------------------------------------------- //

CLeashMgr::CLeashMgr()
{
	m_pGroups = new CHashTable<CLeashGroup*>(32);
	m_bActive = LTTRUE;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CLeashMgr::~CLeashMgr (0x10150eb0)
//
// ----------------------------------------------------------------------- //

CLeashMgr::~CLeashMgr()
{
	Clear();

	if (m_pGroups)
	{
		delete m_pGroups;
		m_pGroups = LTNULL;
	}
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CLeashMgr::GetGroup (0x10150f20)
//
//	PURPOSE:	The named group's handle, making the group if need be
//
// ----------------------------------------------------------------------- //

HLEASHGROUP CLeashMgr::GetGroup(const char* szName)
{
	CLeashGroup* pGroup = FindGroup(szName);
	if (pGroup) return pGroup->m_hGroup;

	pGroup = new CLeashGroup;
	if (!pGroup) return LTNULL;

	SAFE_STRCPY(pGroup->m_szName, szName);
	pGroup->m_hGroup = (HLEASHGROUP)pGroup;

	m_pGroups->Add(pGroup, (uint32)pGroup);

	return pGroup->m_hGroup;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CLeashMgr::Init (0x10151010)
//
// ----------------------------------------------------------------------- //

LTBOOL CLeashMgr::Init()
{
	CHashIterator<CLeashGroup*> iter(m_pGroups);

	CLeashGroup* pGroup;
	while ((pGroup = (CLeashGroup*)iter.Get()) != LTNULL)
	{
		pGroup->Init();
		iter.Next();
	}

	return LTTRUE;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CLeashMgr::AddNode (0x10151110)
//
// ----------------------------------------------------------------------- //

LTBOOL CLeashMgr::AddNode(HLEASHGROUP hGroup, CLeashNode* pNode)
{
	CLeashGroup* pGroup = GetGroup(hGroup);
	if (!pGroup) return LTFALSE;

	return pGroup->AddNode(pNode);
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CLeashMgr::Clear (0x10151130)
//
// ----------------------------------------------------------------------- //

void CLeashMgr::Clear()
{
	CHashIterator<CLeashGroup*> iter(m_pGroups);

	CLeashGroup* pGroup;
	while ((pGroup = (CLeashGroup*)iter.Get()) != LTNULL)
	{
		delete pGroup;
		iter.Next();
	}

	m_pGroups->RemoveAll();
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CLeashMgr::GetGroup (0x10151240)
//
// ----------------------------------------------------------------------- //

CLeashGroup* CLeashMgr::GetGroup(HLEASHGROUP hGroup)
{
	return m_pGroups->Find(hGroup);
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CLeashMgr::FindGroup (0x101512d0)
//
// ----------------------------------------------------------------------- //

CLeashGroup* CLeashMgr::FindGroup(const char* szName)
{
	CHashIterator<CLeashGroup*> iter(m_pGroups);

	CLeashGroup* pGroup;
	while ((pGroup = (CLeashGroup*)iter.Get()) != LTNULL)
	{
		if (!_stricmp(pGroup->m_szName, szName)) return pGroup;
		iter.Next();
	}

	return LTNULL;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CLeashMgr::GetNode (0x10151360)
//
// ----------------------------------------------------------------------- //

CLeashNode* CLeashMgr::GetNode(HLEASHGROUP hGroup, int nID)
{
	CLeashGroup* pGroup = GetGroup(hGroup);
	if (!pGroup) return LTNULL;

	CLeashNode* pNode = pGroup->GetPoint(nID);
	if (!pNode) pNode = pGroup->GetVolume(nID);

	return pNode;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CLeashMgr::FindNode (0x101513a0)
//
// ----------------------------------------------------------------------- //

CLeashNode* CLeashMgr::FindNode(HLEASHGROUP hGroup, const LTVector& vPos)
{
	CLeashGroup* pGroup = GetGroup(hGroup);
	if (!pGroup) return LTNULL;

	CLeashNode* pNode = pGroup->FindPoint(vPos);
	if (!pNode) pNode = pGroup->FindVolume(vPos);

	return pNode;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CLeashMgr::GetNextNode (0x101513e0)
//
// ----------------------------------------------------------------------- //

LTBOOL CLeashMgr::GetNextNode(HLEASHGROUP hGroup, const LTVector& vFrom, const LTVector& vTo, int* pnNext, int* pnStart, int* pnDest)
{
	CLeashGroup* pGroup = GetGroup(hGroup);
	if (!pGroup) return LTFALSE;

	return pGroup->GetNextNode(vFrom, vTo, pnNext, pnStart, pnDest);
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CLeashMgr::GetExitPos (0x10151420)
//
// ----------------------------------------------------------------------- //

LTBOOL CLeashMgr::GetExitPos(HLEASHGROUP hGroup, int nFrom, int nTo, const LTVector& vPos, LTVector* pvExit, LTFLOAT fRadius)
{
	CLeashGroup* pGroup = GetGroup(hGroup);
	if (!pGroup) return LTFALSE;

	return pGroup->GetExitPos(nFrom, nTo, vPos, pvExit, fRadius);
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CLeashMgr::GetEntryPos (0x10151460)
//
// ----------------------------------------------------------------------- //

LTBOOL CLeashMgr::GetEntryPos(HLEASHGROUP hGroup, int nFrom, int nTo, const LTVector& vPos, LTVector* pvEntry, LTFLOAT fRadius)
{
	CLeashGroup* pGroup = GetGroup(hGroup);
	if (!pGroup) return LTFALSE;

	return pGroup->GetEntryPos(nFrom, nTo, vPos, pvEntry, fRadius);
}

// ----------------------------------------------------------------------- //
// The manager's entry points (0x101514a0..0x101515b8)

HLEASHGROUP LeashMgr_GetGroup(const char* szName)
{
	return g_LeashMgr.GetGroup(szName);
}

LTBOOL LeashMgr_Init()
{
	return g_LeashMgr.Init();
}

LTBOOL LeashMgr_AddNode(HLEASHGROUP hGroup, CLeashNode* pNode)
{
	return g_LeashMgr.AddNode(hGroup, pNode);
}

void LeashMgr_Clear()
{
	g_LeashMgr.Clear();
}

CLeashNode* LeashMgr_GetNode(HLEASHGROUP hGroup, int nID)
{
	return g_LeashMgr.GetNode(hGroup, nID);
}

CLeashNode* LeashMgr_FindNode(HLEASHGROUP hGroup, const LTVector& vPos)
{
	return g_LeashMgr.FindNode(hGroup, vPos);
}

LTBOOL LeashMgr_GetNextNode(HLEASHGROUP hGroup, const LTVector& vFrom, const LTVector& vTo, int* pnNext, int* pnStart, int* pnDest)
{
	return g_LeashMgr.GetNextNode(hGroup, vFrom, vTo, pnNext, pnStart, pnDest);
}

LTBOOL LeashMgr_GetExitPos(HLEASHGROUP hGroup, int nFrom, int nTo, const LTVector& vPos, LTVector* pvExit, LTFLOAT fRadius)
{
	return g_LeashMgr.GetExitPos(hGroup, nFrom, nTo, vPos, pvExit, fRadius);
}

LTBOOL LeashMgr_GetEntryPos(HLEASHGROUP hGroup, int nFrom, int nTo, const LTVector& vPos, LTVector* pvEntry, LTFLOAT fRadius)
{
	return g_LeashMgr.GetEntryPos(hGroup, nFrom, nTo, vPos, pvEntry, fRadius);
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CLeashNode::~CLeashNode (0x10151750)
//
// ----------------------------------------------------------------------- //

CLeashNode::~CLeashNode()
{
	CLeashNode::Term();
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CLeashNode::Term (0x10151800)
//
// ----------------------------------------------------------------------- //

void CLeashNode::Term()
{
	ClearNeighbors();

	if (m_pRoutes)
	{
		delete [] m_pRoutes;
		m_pRoutes = LTNULL;
	}
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CLeashNode::IsNeighbor (0x10151830)
//
// ----------------------------------------------------------------------- //

LTBOOL CLeashNode::IsNeighbor(CLeashNode* pNode)
{
	for (TLinkNode<CLeashNode*>* pNeighbor = m_Neighbors.GetHead(); pNeighbor != m_Neighbors.GetEnd(); pNeighbor = pNeighbor->m_pNext)
	{
		if (pNeighbor->m_Data == pNode) return LTTRUE;
	}

	return LTFALSE;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CLeashNode::AddNeighbor (0x10151860)
//
// ----------------------------------------------------------------------- //

void CLeashNode::AddNeighbor(CLeashNode* pNode)
{
	m_Neighbors.AddHead(pNode);
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CLeashNode::ClearNeighbors (0x10151870)
//
// ----------------------------------------------------------------------- //

void CLeashNode::ClearNeighbors()
{
	m_Neighbors.RemoveAll();
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CLeashNode::Contains (0x101518b0)
//
// ----------------------------------------------------------------------- //

LTBOOL CLeashNode::Contains(const LTVector& vPos, LTFLOAT fEpsilon)
{
	return LTFALSE;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CLeashNode::FindNeighbor (0x101518c0)
//
// ----------------------------------------------------------------------- //

CLeashNode* CLeashNode::FindNeighbor(const LTVector& vPos, LTFLOAT fEpsilon)
{
	for (TLinkNode<CLeashNode*>* pNeighbor = m_Neighbors.GetHead(); pNeighbor != m_Neighbors.GetEnd(); pNeighbor = pNeighbor->m_pNext)
	{
		if (pNeighbor->m_Data->Contains(vPos, fEpsilon) == LTTRUE) return pNeighbor->m_Data;
	}

	return LTNULL;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CLeashNode::GetNeighbor (0x10151900)
//
// ----------------------------------------------------------------------- //

CLeashNode* CLeashNode::GetNeighbor(int nID)
{
	for (TLinkNode<CLeashNode*>* pNeighbor = m_Neighbors.GetHead(); pNeighbor != m_Neighbors.GetEnd(); pNeighbor = pNeighbor->m_pNext)
	{
		if (pNeighbor->m_Data->m_nID == nID) return pNeighbor->m_Data;
	}

	return LTNULL;
}

#endif // PRIMAL_HUNT
