// ----------------------------------------------------------------------- //
//
// MODULE  : MoveMgr.cpp
//
// PURPOSE : CMoveMgr - Implementation
//
//           Primal Hunt (Third Law). No source shipped: written from
//           PHObject.lto, 0x10154130..0x10154958.
//
// ----------------------------------------------------------------------- //

#include "stdafx.h"

#ifdef PRIMAL_HUNT

#include "AIBase.h"

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CMoveMgr::CMoveMgr (0x10154130)
//
// ----------------------------------------------------------------------- //

CMoveMgr::CMoveMgr()
{
	m_dw14 = 0;
	m_eMoveType = eMoveStop;
	m_dw18 = 0;
	m_fMinSpeed = 320.0f;
	m_fSpeed = 1280.0f;
	m_vDest = LTVector(0, 0, 0);
	m_bMoving = LTFALSE;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CMoveMgr::~CMoveMgr (0x101541a0)
//
// ----------------------------------------------------------------------- //

CMoveMgr::~CMoveMgr()
{
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CMoveMgr::Update (0x101541b0)
//
// ----------------------------------------------------------------------- //

void CMoveMgr::Update(HOBJECT hObj, LTFLOAT fTime)
{
	switch (m_dw18)
	{
		case 0:	UpdateWalk(hObj, fTime);	break;
		case 1:	UpdateFly(hObj, fTime);		break;
		case 2:	UpdateSwim(hObj, fTime);	break;
	}
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CMoveMgr::Accelerate (0x10154200)
//
// ----------------------------------------------------------------------- //

void CMoveMgr::Accelerate(const LTVector& vDesired, const LTVector& vVel, LTVector* pvNewVel,
						  LTFLOAT fSpeed, LTFLOAT fAccel, LTFLOAT fTime)
{
	if (m_bInstant == LTTRUE)
	{
		*pvNewVel = vDesired;
		if (!m_dw18) pvNewVel->y = 0.0f;

		pvNewVel->Norm();
		*pvNewVel *= fSpeed;

		if (!m_dw18) pvNewVel->y = vVel.y;
		return;
	}

	LTVector vDelta = vDesired - vVel;
	if (!m_dw18) vDelta.y = 0.0f;

	LTFLOAT fMag = vDelta.Mag();
	LTFLOAT fInv = 1.0f / fMag;
	LTVector vDir(vDelta.x * fInv, vDelta.y * fInv, vDelta.z * fInv);

	LTFLOAT fStep = fAccel * fTime;
	if (fStep > fMag) fStep = fMag;

	pvNewVel->x = vDir.x * fStep + vVel.x;
	pvNewVel->y = vDir.y * fStep + vVel.y;
	pvNewVel->z = vDir.z * fStep + vVel.z;

	if (!m_dw18) pvNewVel->y = vVel.y;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CMoveMgr::Decelerate (0x101543b0)
//
// ----------------------------------------------------------------------- //

void CMoveMgr::Decelerate(const LTVector& vVel, LTVector* pvNewVel, LTFLOAT fAccel, LTFLOAT fTime)
{
	if (m_bInstant == LTTRUE || !m_eMoveType)
	{
		pvNewVel->Init(0.0f, 0.0f, 0.0f);
		return;
	}

	LTFLOAT fSpeed = vVel.Mag() - fAccel * fTime;
	if (fSpeed <= 0.0f)
	{
		pvNewVel->Init(0.0f, 0.0f, 0.0f);
		return;
	}

	*pvNewVel = vVel;
	pvNewVel->Norm();
	*pvNewVel *= fSpeed;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CMoveMgr::UpdateWalk (0x10154490)
//
// ----------------------------------------------------------------------- //

void CMoveMgr::UpdateWalk(HOBJECT hObj, LTFLOAT fTime)
{
	LTVector vPos;
	g_pLTServer->GetObjectPos(hObj, &vPos);

	LTVector vVel;
	g_pLTServer->Physics()->GetVelocity(hObj, &vVel);

	LTVector vDir = m_vDest - vPos;

	CollisionInfo info;
	g_pLTServer->Physics()->GetStandingOn(hObj, &info);
	if (info.m_hObject && info.m_hPoly != INVALID_HPOLY)
	{
		m_bMoving = LTTRUE;
	}
	else
	{
		m_bMoving = LTFALSE;
	}

	vDir.y = 0.0f;
	vDir.Norm();

	LTVector vNewVel;

	switch (m_eMoveType)
	{
		case eMoveStop:
		{
			LTVector vZero(0.0f, 0.0f, 0.0f);
			g_pLTServer->Physics()->SetVelocity(hObj, &vZero);

			// RETAIL BUG: this falls through to setting the velocity
			// again, to vNewVel, which nothing has set.
		}
		break;

		case eMoveSlowDown:
		{
			Decelerate(vVel, &vNewVel, m_fSpeed, fTime);
		}
		break;

		case eMoveApproach:
		{
			Accelerate(vDir * m_fMinSpeed, vVel, &vNewVel, m_fMinSpeed, m_fSpeed, fTime);
		}
		break;

		case eMoveFlee:
		{
			Accelerate(vDir * m_fMinSpeed * -1.0f, vVel, &vNewVel, m_fMinSpeed, m_fSpeed, fTime);
		}
		break;

		case eMoveStrafeLeft:
		{
			LTVector vSide(vDir.z, 0.0f, -vDir.x);
			Accelerate(vSide * m_fMinSpeed * -1.0f, vVel, &vNewVel, m_fMinSpeed, m_fSpeed, fTime);
		}
		break;

		case eMoveStrafeRight:
		{
			LTVector vSide(vDir.z, 0.0f, -vDir.x);
			Accelerate(vSide * m_fMinSpeed, vVel, &vNewVel, m_fMinSpeed, m_fSpeed, fTime);
		}
		break;

		case eMoveCoast:
		{
			vNewVel = vVel;
		}
		break;
	}

	if (!m_bMoving)
	{
		vNewVel.y = vVel.y;
	}

	g_pLTServer->Physics()->SetVelocity(hObj, &vNewVel);
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CMoveMgr::UpdateFly / UpdateSwim (0x101547e0, folded)
//
//	PURPOSE:	Neither is written: the AI just stops standing on anything
//
// ----------------------------------------------------------------------- //

void CMoveMgr::UpdateFly(HOBJECT hObj, LTFLOAT fFrameTime)
{
	m_bMoving = LTFALSE;
}

void CMoveMgr::UpdateSwim(HOBJECT hObj, LTFLOAT fFrameTime)
{
	m_bMoving = LTFALSE;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CMoveMgr::GetPos (0x101547f0)
//
// ----------------------------------------------------------------------- //

LTVector CMoveMgr::GetPos(HOBJECT hObj)
{
	LTVector vPos;
	g_pLTServer->GetObjectPos(hObj, &vPos);
	return vPos;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CMoveMgr::Save (0x10154830)
//
// ----------------------------------------------------------------------- //

void CMoveMgr::Save(HMESSAGEWRITE hWrite)
{
	hWrite->WriteVector(m_vDest);
	hWrite->WriteByte((uint8)m_eMoveType);
	hWrite->WriteByte((uint8)m_dw14);
	hWrite->WriteByte((uint8)m_dw18);
	hWrite->WriteFloat(m_fMinSpeed);
	hWrite->WriteFloat(m_fSpeed);
	hWrite->WriteByte((uint8)m_bInstant);
	hWrite->WriteByte((uint8)m_bMoving);
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CMoveMgr::Load (0x101548a0)
//
// ----------------------------------------------------------------------- //

void CMoveMgr::Load(HMESSAGEREAD hRead)
{
	hRead->ReadVectorFL(m_vDest);
	m_eMoveType = hRead->ReadByte();
	m_dw14 = hRead->ReadByte();
	m_dw18 = hRead->ReadByte();
	m_fMinSpeed = hRead->ReadFloat();
	m_fSpeed = hRead->ReadFloat();
	m_bInstant = hRead->ReadByte();
	m_bMoving = hRead->ReadByte();
}

#endif // PRIMAL_HUNT
