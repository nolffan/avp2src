// ----------------------------------------------------------------------- //
//
// MODULE  : Marker.cpp
//
// PURPOSE : CMarker - Implementation
//
//           Primal Hunt (Third Law). No source shipped: written from
//           PHObject.lto, 0x10153c00..0x10153e36.
//
// ----------------------------------------------------------------------- //

#include "stdafx.h"

#ifdef PRIMAL_HUNT

#include "Marker.h"

BEGIN_CLASS(CMarker)
END_CLASS_DEFAULT(CMarker, GameBase, LTNULL, LTNULL)

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CMarker::OnPrecreate (0x10153d50)
//
// ----------------------------------------------------------------------- //

uint32 CMarker::OnPrecreate(void *pData, uint32 nInfo)
{
	MarkerCreateStruct* pStruct = (MarkerCreateStruct*)pData;
	pStruct->m_Flags |= FLAG_VISIBLE;

	if ((LTFLOAT)nInfo == PRECREATE_NORMAL)
	{
		m_nType = pStruct->m_nType;
		m_fLifeTime = pStruct->m_fLifeTime;
	}

	return LTFALSE;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CMarker::OnInitialUpdate (0x10153da0)
//
// ----------------------------------------------------------------------- //

uint32 CMarker::OnInitialUpdate(uint32 nInfo)
{
	SetNextUpdate(m_fLifeTime);
	return LTFALSE;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CMarker::OnUpdate (0x10153db0)
//
// ----------------------------------------------------------------------- //

uint32 CMarker::OnUpdate(LTFLOAT fFrameTime)
{
	g_pLTServer->RemoveObject(m_hObject);
	return LTFALSE;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CMarker::Save (0x10153dd0)
//
// ----------------------------------------------------------------------- //

void CMarker::Save(HMESSAGEWRITE hWrite)
{
	hWrite->WriteByte((uint8)m_nType);
	hWrite->WriteFloat(m_fLifeTime);
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CMarker::Load (0x10153e00)
//
// ----------------------------------------------------------------------- //

void CMarker::Load(HMESSAGEREAD hRead)
{
	m_nType = hRead->ReadByte();
	m_fLifeTime = hRead->ReadFloat();
}

#endif // PRIMAL_HUNT
