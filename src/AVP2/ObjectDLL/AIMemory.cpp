// ----------------------------------------------------------------------- //
//
// MODULE  : AIMemory.cpp
//
// PURPOSE : CAIMemory - Implementation
//
//           Primal Hunt (Third Law). No source shipped: written from
//           PHObject.lto, 0x1014f080..0x1014f6a1.
//
// ----------------------------------------------------------------------- //

#include "stdafx.h"

#ifdef PRIMAL_HUNT

#include "AIBase.h"

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CAIMemory::CAIMemory (0x1014f080)
//
// ----------------------------------------------------------------------- //

CAIMemory::CAIMemory()
{
	m_nCount = 0;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CAIMemory::~CAIMemory (0x1014f100)
//
// ----------------------------------------------------------------------- //

CAIMemory::~CAIMemory()
{
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CAIMemory::Add (0x1014f110)
//
// ----------------------------------------------------------------------- //

LTBOOL CAIMemory::Add(HOBJECT hObj)
{
	if (m_nCount >= 4) return LTFALSE;
	if (IsAware(hObj) == LTTRUE) return LTTRUE;

	m_Memories[m_nCount].m_hObject = hObj;
	m_Memories[m_nCount].m_Senses.Clear();
	m_nCount++;

	return LTTRUE;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CAIMemory::Forget (0x1014f1d0)
//
// ----------------------------------------------------------------------- //

LTBOOL CAIMemory::Forget(HOBJECT hObj)
{
	int nIndex = -1;

	int i;
	for (i = 0; i < m_nCount; i++)
	{
		if (m_Memories[i].m_hObject == hObj)
		{
			nIndex = i;
			break;
		}
	}

	if (nIndex == -1) return LTFALSE;

	for (i = nIndex + 1; i < 4; i++)
	{
		if (i >= m_nCount) break;

		m_Memories[i - 1] = m_Memories[i];
	}

	m_nCount--;

	for (i = m_nCount; i < 4; i++)
	{
		m_Memories[i].m_hObject = LTNULL;
	}

	return LTTRUE;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CAIMemory::Save (0x1014f2f0)
//
// ----------------------------------------------------------------------- //

void CAIMemory::Save(HMESSAGEWRITE hWrite)
{
	hWrite->WriteDWord(m_nCount);

	for (int i = 0; i < m_nCount; i++)
	{
		hWrite->WriteObject(m_Memories[i].m_hObject);
		m_Memories[i].m_Senses.Save(hWrite);
	}
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CAIMemory::Load (0x1014f350)
//
// ----------------------------------------------------------------------- //

void CAIMemory::Load(HMESSAGEREAD hRead)
{
	m_nCount = hRead->ReadDWord();

	for (int i = 0; i < m_nCount; i++)
	{
		m_Memories[i].m_hObject = hRead->ReadObject();

		// RETAIL BUG: the senses are written into the save game's read
		// message instead of being read out of it (CSenseLevels has no
		// Load).
		m_Memories[i].m_Senses.Save(hRead);
	}
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CAIMemory::IsAware (0x1014f400)
//
// ----------------------------------------------------------------------- //

LTBOOL CAIMemory::IsAware(HOBJECT hObj)
{
	for (int i = 0; i < m_nCount; i++)
	{
		if (m_Memories[i].m_hObject == hObj) return LTTRUE;
	}

	return LTFALSE;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CAIMemory::AddStimulus (0x1014f440)
//
// ----------------------------------------------------------------------- //

void CAIMemory::AddStimulus(HOBJECT hObj, int nSense, LTFLOAT fAmount)
{
	for (int i = 0; i < m_nCount; i++)
	{
		if (m_Memories[i].m_hObject == hObj)
		{
			m_Memories[i].m_Senses.Add(nSense, fAmount);
			return;
		}
	}
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CAIMemory::Update (0x1014f4a0)
//
// ----------------------------------------------------------------------- //

void CAIMemory::Update(LTFLOAT fFrameTime)
{
	for (int i = 0; i < m_nCount; i++)
	{
		m_Memories[i].m_Senses.Decay(fFrameTime);

		if (!m_Memories[i].m_Senses.HasAny())
		{
			Forget(m_Memories[i].m_hObject);
			i--;
		}
	}
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CAIMemory::GetBest (0x1014f510)
//
// ----------------------------------------------------------------------- //

HOBJECT CAIMemory::GetBest(int nSense)
{
	for (int i = 0; i < m_nCount; i++)
	{
		if (m_Memories[i].m_Senses.IsAware(nSense) == LTTRUE)
		{
			return m_Memories[i].m_hObject;
		}
	}

	return LTNULL;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CAIMemory::ClearAll (0x1014f570)
//
// ----------------------------------------------------------------------- //

void CAIMemory::ClearAll()
{
	for (int i = 0; i < m_nCount; i++)
	{
		m_Memories[i].m_Senses.Clear();
	}
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CAIMemory::Clear (0x1014f5a0)
//
// ----------------------------------------------------------------------- //

void CAIMemory::Clear(HOBJECT hObj)
{
	for (int i = 0; i < m_nCount; i++)
	{
		if (m_Memories[i].m_hObject == hObj)
		{
			m_Memories[i].m_Senses.Clear();
			return;
		}
	}
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CAIMemory::SetRange (0x1014f5f0)
//
// ----------------------------------------------------------------------- //

void CAIMemory::SetRange(int nSense, LTFLOAT fRange)
{
	for (int i = 0; i < 4; i++)
	{
		m_Memories[i].m_Senses.SetRange(nSense, fRange);
	}
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CAIMemory::SetDecay (0x1014f620)
//
// ----------------------------------------------------------------------- //

void CAIMemory::SetDecay(int nSense, LTFLOAT fDecay)
{
	for (int i = 0; i < 4; i++)
	{
		m_Memories[i].m_Senses.SetDecay(nSense, fDecay);
	}
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CAIMemory::GetStimulus (0x1014f650)
//
// ----------------------------------------------------------------------- //

LTFLOAT CAIMemory::GetStimulus(HOBJECT hObj, int nSense)
{
	for (int i = 0; i < m_nCount; i++)
	{
		if (m_Memories[i].m_hObject == hObj)
		{
			return m_Memories[i].m_Senses.GetRange(nSense);
		}
	}

	return 0.0f;
}

#endif // PRIMAL_HUNT
