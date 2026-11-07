// ----------------------------------------------------------------------- //
//
// MODULE  : AttachedSpears.cpp
//
// PURPOSE : The spears stuck in a CAIBase (SpearInfo, CSpearList)
//
// ----------------------------------------------------------------------- //

// PH: Third Law's code, PHObject.lto 0x1014e8d0..0x1014edba, linked
// between CArtifactField's file and ChameleonTest's. The file's name is a
// guess (retail does not record it; the new files link in the order of
// their names); the classes are declared in AIBase.h.

#include "stdafx.h"

#ifdef PRIMAL_HUNT

#include "AIBase.h"

// PH: 0x1014e8d0
void SpearInfo::Save(HMESSAGEWRITE hWrite)
{
	hWrite->WriteObject(hSpear);
	hWrite->WriteFloat(rRot.m_Quat[0]);
	hWrite->WriteFloat(rRot.m_Quat[1]);
	hWrite->WriteFloat(rRot.m_Quat[2]);
	hWrite->WriteFloat(rRot.m_Quat[3]);
	hWrite->WriteVector(vPos);
	hWrite->WriteDWord(nNode);
}

// PH: 0x1014e940
void SpearInfo::Load(HMESSAGEREAD hRead)
{
	hSpear		= hRead->ReadObject();
	rRot.m_Quat[0] = hRead->ReadFloat();
	rRot.m_Quat[1] = hRead->ReadFloat();
	rRot.m_Quat[2] = hRead->ReadFloat();
	rRot.m_Quat[3] = hRead->ReadFloat();
	hRead->ReadVectorFL(vPos);
	nNode		= hRead->ReadDWord();
}

// PH: folded into CAIAnimator's (0x1014d920)
CSpearList::CSpearList()
{
}

// PH: 0x1014ea00
CSpearList::~CSpearList()
{
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CSpearList::Add()
//
//	PURPOSE:	Keep a copy of a spear
//
// ----------------------------------------------------------------------- //

// PH: 0x1014eaa0
LTBOOL CSpearList::Add(SpearInfo* pSpear)
{
	SpearInfo* pNew = new SpearInfo;
	if (!pNew) return LTFALSE;

	*pNew = *pSpear;
	AddHead(pNew);

	return LTTRUE;
}

// PH: 0x1014ebe0
void CSpearList::Save(HMESSAGEWRITE hWrite)
{
	RemoveDead();

	hWrite->WriteDWord(GetCount());

	for (Node* pNode = GetHead(); pNode != GetEnd(); pNode = pNode->m_pNext)
	{
		pNode->m_Data->Save(hWrite);
	}
}

// PH: 0x1014ec20
void CSpearList::Load(HMESSAGEREAD hRead)
{
	DeleteAll();

	int nCount = hRead->ReadDWord();
	for (int i = 0; i < nCount; i++)
	{
		SpearInfo* pSpear = new SpearInfo;
		pSpear->Load(hRead);
		AddHead(pSpear);
	}
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CSpearList::RemoveDead()
//
//	PURPOSE:	Drop the spears that are gone
//
// ----------------------------------------------------------------------- //

// PH: 0x1014ed50. RETAIL BUG: Remove() frees only the node, so each spear
// dropped here leaks its SpearInfo.
void CSpearList::RemoveDead()
{
	Node* pNode = GetHead();
	while (pNode != GetEnd())
	{
		Node* pNext = pNode->m_pNext;

		SpearInfo* pSpear = pNode->m_Data;
		if (pSpear && !pSpear->hSpear)
		{
			Remove(pNode);
		}

		pNode = pNext;
	}
}

#endif // PRIMAL_HUNT
