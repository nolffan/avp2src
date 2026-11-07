// ----------------------------------------------------------------------- //
//
// MODULE  : LeashPoint.cpp
//
// PURPOSE : LeashPoint and CLeashPoint - Implementation
//
//           Primal Hunt (Third Law). No source shipped: written from
//           PHObject.lto, 0x10151930..0x10152063.
//
// ----------------------------------------------------------------------- //

#include "stdafx.h"

#ifdef PRIMAL_HUNT

#include "LeashPoint.h"

BEGIN_CLASS(LeashPoint)
	ADD_STRINGPROP_FLAG(Group_Name, "", 0)
	ADD_REALPROP_FLAG(Radius, 1024.0f, PF_RADIUS)
END_CLASS_DEFAULT(LeashPoint, GameBase, LTNULL, LTNULL)

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	LeashPoint::LeashPoint (0x10151ad0)
//
// ----------------------------------------------------------------------- //

LeashPoint::LeashPoint() : GameBase(OT_NORMAL)
{
	SAFE_STRCPY(m_szGroupName, "");
	m_pNode = LTNULL;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	LeashPoint::OnPrecreate (0x10151b40)
//
// ----------------------------------------------------------------------- //

uint32 LeashPoint::OnPrecreate(void *pData, uint32 nInfo)
{
	ObjectCreateStruct* pStruct = (ObjectCreateStruct*)pData;
	pStruct->m_ObjectType = OT_NORMAL;
	pStruct->m_Flags = 0;

	if ((LTFLOAT)nInfo == PRECREATE_WORLDFILE)
	{
		g_pLTServer->GetPropString("Group_Name", m_szGroupName, sizeof(m_szGroupName));
		g_pLTServer->GetPropReal("Radius", &m_fRadius);
	}

	return LTFALSE;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	LeashPoint::OnInitialUpdate (0x10151bb0)
//
// ----------------------------------------------------------------------- //

uint32 LeashPoint::OnInitialUpdate(uint32 nInfo)
{
	m_pNode = new CLeashPoint;

	LTVector vPos;
	g_pLTServer->GetObjectPos(m_hObject, &vPos);
	m_pNode->SetPos(vPos);
	m_pNode->m_fRadius = m_fRadius;

	m_hGroup = LeashMgr_GetGroup(m_szGroupName);
	if (m_hGroup && !LeashMgr_AddNode(m_hGroup, m_pNode))
	{
		g_pLTServer->RemoveObject(m_hObject);
		return LTTRUE;
	}

	return LTFALSE;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CLeashPoint::CLeashPoint (0x10151ca0)
//
// ----------------------------------------------------------------------- //

CLeashPoint::CLeashPoint() : CLeashNode(LEASHNODE_POINT)
{
	m_vPos.Init();
	m_fRadius = 0.0f;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CLeashPoint::GetPos (0x10151ce0)
//
// ----------------------------------------------------------------------- //

LTVector CLeashPoint::GetPos()
{
	return m_vPos;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CLeashPoint::SetPos (0x10151d00)
//
// ----------------------------------------------------------------------- //

void CLeashPoint::SetPos(const LTVector& vPos)
{
	m_vPos = vPos;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CLeashPoint::~CLeashPoint (0x10151d40)
//
// ----------------------------------------------------------------------- //

CLeashPoint::~CLeashPoint()
{
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CLeashPoint::Overlaps (0x10151d50)
//
// ----------------------------------------------------------------------- //

LTBOOL CLeashPoint::Overlaps(const LTVector& vPos, LTFLOAT fRadius)
{
	LTVector vMyPos = m_vPos;
	LTFLOAT fDist = (vPos - vMyPos).Mag();

	if (fRadius + m_fRadius > fDist) return LTTRUE;

	return LTFALSE;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CLeashPoint::Overlaps (0x10151dd0)
//
// ----------------------------------------------------------------------- //

LTBOOL CLeashPoint::Overlaps(CLeashNode* pNode)
{
	if (pNode->m_nType != LEASHNODE_POINT) return LTFALSE;

	return Overlaps(pNode->GetPos(), ((CLeashPoint*)pNode)->m_fRadius);
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CLeashPoint::Contains (0x10151e10)
//
//	PURPOSE:	Is vPos inside the point's cylinder (in x and z)?
//
// ----------------------------------------------------------------------- //

LTBOOL CLeashPoint::Contains(const LTVector& vPos, LTFLOAT fEpsilon)
{
	LTFLOAT fX = m_vPos.x - vPos.x;
	LTFLOAT fZ = m_vPos.z - vPos.z;

	if (fZ*fZ + fX*fX <= m_fRadius*m_fRadius + fEpsilon) return LTTRUE;

	return LTFALSE;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CLeashPoint::GetExitPos (0x10151e50)
//
// ----------------------------------------------------------------------- //

LTBOOL CLeashPoint::GetExitPos(CLeashNode* pTo, const LTVector& vPos, LTVector* pvExit, LTFLOAT fRadius)
{
	if (pTo->m_nType != LEASHNODE_POINT) return LTFALSE;

	LTVector vMyPos = m_vPos;
	LTVector vDir = pTo->GetPos() - vMyPos;
	vDir.Norm();

	*pvExit = m_vPos + vDir * (m_fRadius - 64.0f);

	return LTTRUE;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CLeashPoint::GetEntryPos (0x10151f60)
//
// ----------------------------------------------------------------------- //

LTBOOL CLeashPoint::GetEntryPos(CLeashNode* pTo, const LTVector& vPos, LTVector* pvEntry, LTFLOAT fRadius)
{
	if (pTo->m_nType != LEASHNODE_POINT) return LTFALSE;

	LTVector vMyPos = m_vPos;
	LTVector vDir = pTo->GetPos() - vMyPos;
	vDir.Norm();

	*pvEntry = m_vPos + vDir * (m_fRadius + 64.0f);

	return LTTRUE;
}

#endif // PRIMAL_HUNT
