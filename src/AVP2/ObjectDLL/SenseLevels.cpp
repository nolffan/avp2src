// ----------------------------------------------------------------------- //
//
// MODULE  : SenseLevels.cpp
//
// PURPOSE : CSenseLevels - Implementation
//
//           Primal Hunt (Third Law). No source shipped: written from
//           PHObject.lto, 0x10155ec0..0x1015604b (the end of an object
//           whose earlier code, 0x10155b50.., is not written).
//
// ----------------------------------------------------------------------- //

#include "stdafx.h"

#ifdef PRIMAL_HUNT

#include "SenseLevels.h"

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CSenseLevels::CSenseLevels (0x10155ec0)
//
// ----------------------------------------------------------------------- //

CSenseLevels::CSenseLevels()
{
	for (int i = 0; i < NUM_SENSE_LEVELS; i++)
	{
		m_Senses[i].m_fRange = 1000.0f;
		m_Senses[i].m_fLevel = 0.0f;
		m_Senses[i].m_fDecay = 100.0f;
	}

	Clear();
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CSenseLevels::Decay (0x10155ef0)
//
// ----------------------------------------------------------------------- //

void CSenseLevels::Decay(LTFLOAT fTime)
{
	for (int i = 0; i < NUM_SENSE_LEVELS; i++)
	{
		m_Senses[i].m_fLevel -= fTime * m_Senses[i].m_fDecay;
		if (m_Senses[i].m_fLevel < 0.0f)
		{
			m_Senses[i].m_fLevel = 0.0f;
		}
	}
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CSenseLevels::Add (0x10155f20)
//
// ----------------------------------------------------------------------- //

void CSenseLevels::Add(int nSense, LTFLOAT fAmount)
{
	m_Senses[nSense].m_fLevel += fAmount;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CSenseLevels::IsAware (0x10155f40)
//
// ----------------------------------------------------------------------- //

LTBOOL CSenseLevels::IsAware(int nSense)
{
	if (m_Senses[nSense].m_fLevel >= m_Senses[nSense].m_fRange) return LTTRUE;

	return LTFALSE;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CSenseLevels::Clear (0x10155f70)
//
// ----------------------------------------------------------------------- //

void CSenseLevels::Clear()
{
	for (int i = 0; i < NUM_SENSE_LEVELS; i++)
	{
		m_Senses[i].m_fLevel = 0.0f;
	}
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CSenseLevels::Save (0x10155f90)
//
// ----------------------------------------------------------------------- //

void CSenseLevels::Save(HMESSAGEWRITE hWrite)
{
	for (int i = 0; i < NUM_SENSE_LEVELS; i++)
	{
		hWrite->WriteFloat(m_Senses[i].m_fLevel);
		hWrite->WriteFloat(m_Senses[i].m_fRange);
		hWrite->WriteFloat(m_Senses[i].m_fDecay);
	}
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CSenseLevels::HasAny (0x10155fd0)
//
// ----------------------------------------------------------------------- //

LTBOOL CSenseLevels::HasAny()
{
	for (int i = 0; i < NUM_SENSE_LEVELS; i++)
	{
		if (m_Senses[i].m_fLevel > 0.0f) return LTTRUE;
	}

	return LTFALSE;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CSenseLevels::SetRange (0x10156000)
//
// ----------------------------------------------------------------------- //

void CSenseLevels::SetRange(int nSense, LTFLOAT fRange)
{
	m_Senses[nSense].m_fRange = fRange;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CSenseLevels::SetDecay (0x10156020)
//
// ----------------------------------------------------------------------- //

void CSenseLevels::SetDecay(int nSense, LTFLOAT fDecay)
{
	m_Senses[nSense].m_fDecay = fDecay;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CSenseLevels::GetRange (0x10156040)
//
// ----------------------------------------------------------------------- //

LTFLOAT CSenseLevels::GetRange(int nSense)
{
	return m_Senses[nSense].m_fRange;
}

#endif // PRIMAL_HUNT
