// ----------------------------------------------------------------------- //
//
// MODULE  : OrientationMgr.cpp
//
// PURPOSE : COrientationMgr - Implementation
//
//           Primal Hunt (Third Law). No source shipped: written from
//           PHObject.lto, 0x101552e0..0x10155605.
//
// ----------------------------------------------------------------------- //

#include "stdafx.h"

#ifdef PRIMAL_HUNT

#include "AIBase.h"

// AIBase.cpp's (0x10141ea0, and the up axis at 0x10337188)
void TurnToward(const LTVector& vCur, const LTVector& vTarget, const LTVector& vUp,
				LTFLOAT fRate, LTFLOAT fTime, LTVector& vOut);
extern LTVector g_vAIBaseUp2;

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	COrientationMgr::COrientationMgr (0x101552e0)
//
// ----------------------------------------------------------------------- //

COrientationMgr::COrientationMgr()
{
	m_bFacePos = eOrientForward;
	m_vFacePos = LTVector(0.0f, 0.0f, 0.0f);
	m_bEnabled = LTTRUE;
	m_fTurnRate = MATH_CIRCLE;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	COrientationMgr::~COrientationMgr (0x10155340)
//
// ----------------------------------------------------------------------- //

COrientationMgr::~COrientationMgr()
{
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	COrientationMgr::Update (0x10155350)
//
// ----------------------------------------------------------------------- //

void COrientationMgr::Update(HOBJECT hObj, LTFLOAT fFrameTime)
{
	LTRotation rRot;
	g_pLTServer->GetObjectRotation(hObj, &rRot);

	LTVector vUp, vRight, vForward;
	g_pLTServer->GetRotationVectors(&rRot, &vUp, &vRight, &vForward);

	LTVector vDir;

	switch (m_bFacePos)
	{
		case eOrientForward:
		{
			if (m_bEnabled == LTTRUE) vForward.y = 0.0f;
			vForward.Norm();
			m_vForward = vForward;
		}
		return;

		case eOrientFacePos:
		{
			LTVector vPos;
			g_pLTServer->GetObjectPos(hObj, &vPos);
			vDir = m_vFacePos - vPos;
		}
		break;

		case eOrientVelocity:
		{
			g_pLTServer->Physics()->GetVelocity(hObj, &vDir);
		}
		break;
	}

	if (m_bEnabled == LTTRUE) vDir.y = 0.0f;
	vDir.Norm();

	TurnToward(vForward, vDir, g_vAIBaseUp2, m_fTurnRate, fFrameTime, m_vForward);

	g_pLTServer->AlignRotation(&rRot, &m_vForward, &g_vAIBaseUp2);
	g_pLTServer->SetObjectRotation(hObj, &rRot);
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	COrientationMgr::Save (0x10155550)
//
// ----------------------------------------------------------------------- //

void COrientationMgr::Save(HMESSAGEWRITE hWrite)
{
	hWrite->WriteByte((uint8)m_bFacePos);
	hWrite->WriteVector(m_vForward);
	hWrite->WriteVector(m_vFacePos);
	hWrite->WriteByte((uint8)m_bEnabled);
	hWrite->WriteFloat(m_fTurnRate);
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	COrientationMgr::Load (0x101555a0)
//
// ----------------------------------------------------------------------- //

void COrientationMgr::Load(HMESSAGEREAD hRead)
{
	m_bFacePos = hRead->ReadByte();
	hRead->ReadVectorFL(m_vForward);
	hRead->ReadVectorFL(m_vFacePos);
	m_bEnabled = hRead->ReadByte();
	m_fTurnRate = hRead->ReadFloat();
}

#endif // PRIMAL_HUNT
