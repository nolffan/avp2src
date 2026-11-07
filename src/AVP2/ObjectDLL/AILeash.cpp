// ----------------------------------------------------------------------- //
//
// MODULE  : AILeash.cpp
//
// PURPOSE : CAIBase's leash (CLeash), and the list of every CAIBase
//
// ----------------------------------------------------------------------- //

// PH: Third Law's code, PHObject.lto 0x1014b610..0x1014bb1d, linked
// between CAIChameleon's file and CAITentacle's. The file's name is a
// guess (retail does not record it; the new files link in the order of
// their names); CLeash is declared in AIBase.h.

#include "stdafx.h"

#ifdef PRIMAL_HUNT

#include "AIBase.h"

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CLeash::CLeash()
//
// ----------------------------------------------------------------------- //

// PH: 0x1014b610
CLeash::CLeash()
{
	m_bOutside		= LTFALSE;
	m_vOutsidePos.x	= 0.0f;
	m_vOutsidePos.y	= 0.0f;
	m_vOutsidePos.z	= 0.0f;
	m_hGroup		= 0;
	m_szGroup[0]	= '\0';
	m_nCurNode		= -1;
	m_nLastNode		= -1;
	m_nHomeNode		= -1;
}

// PH: empty; folded with the other empty functions (CAIBase's destructor
// calls it at 0x10142891)
CLeash::~CLeash()
{
}

// PH: 0x1014b640
LTBOOL CLeash::IsValid()
{
	if (!m_szGroup[0]) return LTFALSE;

	m_bOutside = LTFALSE;
	m_hGroup = LeashMgr_GetGroup(m_szGroup);
	return m_hGroup != 0;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CLeash::Update()
//
//	PURPOSE:	Find the node the AI is in; note when it has left the
//				leash, and the first node it was ever in
//
// ----------------------------------------------------------------------- //

// PH: 0x1014b670
LTBOOL CLeash::Update(const LTVector& vPos)
{
	if (!m_hGroup) return LTFALSE;

	if (m_nCurNode != -1)
	{
		CLeashNode* pNode = LeashMgr_GetNode(m_hGroup, m_nCurNode);
		if (pNode && pNode->Contains(vPos, 0.1f) == LTTRUE) return LTTRUE;
	}

	if (m_nCurNode != -1)
	{
		m_nLastNode = m_nCurNode;
	}

	CLeashNode* pNode = LeashMgr_FindNode(m_hGroup, vPos);
	if (!pNode)
	{
		m_nCurNode = -1;
		m_bOutside = LTTRUE;
		return LTFALSE;
	}

	m_nCurNode = pNode->m_nID;

	if (m_nHomeNode == -1 && m_nLastNode == -1 && m_nCurNode > -1)
	{
		m_nHomeNode = m_nCurNode;
		m_vHomePos = vPos;
	}

	return LTTRUE;
}

// PH: 0x1014b720
void CLeash::Save(HMESSAGEWRITE hWrite)
{
	hWrite->WriteString(m_szGroup);
	hWrite->WriteByte(m_bOutside);
	hWrite->WriteVector(m_vOutsidePos);
	hWrite->WriteDWord(m_nCurNode);
	hWrite->WriteDWord(m_nLastNode);
	hWrite->WriteDWord(m_nHomeNode);
	hWrite->WriteVector(m_vHomePos);
}

// PH: 0x1014b780
void CLeash::Load(HMESSAGEREAD hRead)
{
	hRead->ReadStringFL(m_szGroup, sizeof(m_szGroup));
	m_bOutside	= hRead->ReadByte();
	hRead->ReadVectorFL(m_vOutsidePos);
	m_nCurNode	= hRead->ReadDWord();
	m_nLastNode	= hRead->ReadDWord();
	m_nHomeNode	= hRead->ReadDWord();
	hRead->ReadVectorFL(m_vHomePos);
}

// PH: 0x1014b800
CLeashNode* CLeash::GetNode(int nID)
{
	if (!m_hGroup) return LTNULL;
	return LeashMgr_GetNode(m_hGroup, nID);
}

// PH: 0x1014b820
CLeashNode* CLeash::FindNode(const LTVector& vPos)
{
	if (!m_hGroup) return LTNULL;
	return LeashMgr_FindNode(m_hGroup, vPos);
}

// PH: 0x1014b840
LTBOOL CLeash::FindPath(const LTVector& vFrom, const LTVector& vTo, int* pnNext, int* pnStart, int* pnDest)
{
	return LeashMgr_GetNextNode(m_hGroup, vFrom, vTo, pnNext, pnStart, pnDest);
}

// PH: 0x1014b870
LTBOOL CLeash::GetPosToward(int nFrom, int nTo, const LTVector& vPos, LTVector& vOut, LTFLOAT fRadius)
{
	return LeashMgr_GetExitPos(m_hGroup, nFrom, nTo, vPos, &vOut, fRadius);
}

// PH: 0x1014b8a0
LTBOOL CLeash::GetPosAway(int nFrom, int nTo, const LTVector& vPos, LTVector& vOut, LTFLOAT fRadius)
{
	return LeashMgr_GetEntryPos(m_hGroup, nFrom, nTo, vPos, &vOut, fRadius);
}

// ----------------------------------------------------------------------- //
//
//	The list of every CAIBase (0x10337410; its initialisers are 0x1014b8d0
//	and 0x1014b910)
//
// ----------------------------------------------------------------------- //

class CAIBaseList : public TLinkList<CAIBase*>
{
	public :

		CAIBaseList() { m_pIter = LTNULL; }

		Node*	m_pIter;
};

static CAIBaseList s_AIBaseList;

// PH: 0x1014b9b0, from CGameServerShell's PreStartWorld
void AIBaseList_Clear()
{
	s_AIBaseList.RemoveAll();
	s_AIBaseList.m_pIter = LTNULL;
}

// PH: 0x1014ba10, from CAIBase::OnInitialUpdate
void AIBaseList_Add(CAIBase* pAI)
{
	s_AIBaseList.AddHead(pAI);
}

// PH: 0x1014ba20
CAIBase* AIBaseList_GetFirst()
{
	if (s_AIBaseList.GetHead() == s_AIBaseList.GetEnd()) return LTNULL;

	s_AIBaseList.m_pIter = s_AIBaseList.GetHead();
	return s_AIBaseList.m_pIter->m_Data;
}

// PH: 0x1014ba40
CAIBase* AIBaseList_GetNext()
{
	if (s_AIBaseList.GetHead() != s_AIBaseList.GetEnd() && s_AIBaseList.m_pIter)
	{
		s_AIBaseList.m_pIter = s_AIBaseList.m_pIter->m_pNext;
		if (s_AIBaseList.m_pIter != s_AIBaseList.GetEnd())
		{
			return s_AIBaseList.m_pIter->m_Data;
		}
	}

	return LTNULL;
}

// PH: 0x1014ba70, from CGameServerShell when the player's vision mode
// changes
void AIBaseList_SetVisionMode(int nMode)
{
	for (CAIBase* pAI = AIBaseList_GetFirst(); pAI; pAI = AIBaseList_GetNext())
	{
		pAI->SetVisionMode(nMode);
	}
}

// PH: 0x1014baa0, from CCharacter's footsteps
void AIBaseList_HandleSound(HOBJECT hObj, const LTVector& vPos, LTFLOAT fVolume, uint32 nSurfaceType, LTFLOAT fTime)
{
	for (CAIBase* pAI = AIBaseList_GetFirst(); pAI; pAI = AIBaseList_GetNext())
	{
		pAI->HandleSound(hObj, vPos, fVolume, nSurfaceType, fTime);
	}
}

#endif // PRIMAL_HUNT
