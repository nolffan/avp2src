// ----------------------------------------------------------------------- //
//
// MODULE  : AISight.cpp
//
// PURPOSE : CAvoidance and CAISight - how a CAIBase steps around what
//           blocks it, and what it can see
//
// CREATED : Primal Hunt (Third Law); no source survives, written from
//           PHObject.lto (0x1015abd0..0x1015b520, between TurretSpawner's
//           code and VolumetricFog's). The file's name is ours.
//
// ----------------------------------------------------------------------- //

#include "stdafx.h"

#ifdef PRIMAL_HUNT

#include "AIBase.h"
#include "ServerUtilities.h"
#include "Character.h"
#include "CharacterHitBox.h"
#include "CharacterFuncs.h"

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CAvoidance::Update (0x1015abd0)
//
//	PURPOSE:	Start the next step of going around: which way to move
//
// ----------------------------------------------------------------------- //

void CAvoidance::Update()
{
	m_fTime = g_pLTServer->GetTime();

	switch (m_nState)
	{
		// Step to the right
		case 1:
		{
			m_vRight.Init(m_vDir.z, 0.0f, -m_vDir.x);
		}
		break;

		// Step to the left
		case 2:
		{
			m_vRight.Init(m_vDir.z, 0.0f, -m_vDir.x);
			m_vRight *= -1.0f;
		}
		break;

		// Back off
		case 5:
		{
			m_vRight = m_vDir * -1.0f;
		}
		break;

		// Jump forward
		case 3:
		{
			m_bActive = LTTRUE;
			m_vRight = m_vDir;
		}
		break;
	}
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CAvoidance::CheckBlocked (0x1015acd0)
//
//	PURPOSE:	Keep going around, if we are
//
// ----------------------------------------------------------------------- //

LTBOOL CAvoidance::CheckBlocked(HOBJECT hObj, CMoveMgr* pMoveMgr)
{
	if (!m_nState) return LTFALSE;

	Steer(hObj, pMoveMgr, g_pLTServer->GetTime() - m_fTime);

	return LTTRUE;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CAvoidance::Steer (0x1015ad10)
//
//	PURPOSE:	Move the AI a step the way we're going around
//
// ----------------------------------------------------------------------- //

void CAvoidance::Steer(HOBJECT hObj, CMoveMgr* pMoveMgr, LTFLOAT fTime)
{
	LTVector vPos;
	g_pLTServer->GetObjectPos(hObj, &vPos);

	LTFLOAT fOldSpeed = pMoveMgr->m_fMinSpeed;

	switch (m_nState)
	{
		case 1:
		case 2:
		case 5:
		{
			m_vPos = m_vRight;

			LTVector vDir = m_vRight;
			pMoveMgr->SetDest(vPos + vDir * 128.0f);
			pMoveMgr->m_dw14 = 1;
			pMoveMgr->SetMoveType(CMoveMgr::eMoveApproach);
			pMoveMgr->SetMinSpeed(m_fCheckDist);

			pMoveMgr->Update(hObj, g_pLTServer->GetFrameTime());
		}
		break;

		case 3:
		{
			m_vPos = m_vRight;

			LTVector vDir = m_vRight;
			pMoveMgr->SetMoveType(CMoveMgr::eMoveApproach);
			pMoveMgr->SetDest(vPos + vDir * 128.0f);
			pMoveMgr->m_dw14 = 1;
			pMoveMgr->SetMinSpeed(m_fCheckDist);

			if (m_bActive == LTTRUE)
			{
				LTVector vVel;
				g_pLTServer->Physics()->GetVelocity(hObj, &vVel);
				vVel.y += m_fAvoidDist;
				g_pLTServer->Physics()->SetVelocity(hObj, &vVel);

				m_bActive = LTFALSE;
			}

			pMoveMgr->Update(hObj, g_pLTServer->GetFrameTime());
		}
		break;
	}

	pMoveMgr->SetMinSpeed(fOldSpeed);
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CAISight::CanSee (0x1015af50)
//
//	PURPOSE:	Can we see hObj from vPos, looking along vForward? If so,
//				*pfDist is how well (1 near, falling off to a quarter at
//				the edge of our range, less for a cloaked predator)
//
// ----------------------------------------------------------------------- //

LTBOOL CAISight::CanSee(HOBJECT hObj, const LTVector& vPos, const LTVector& vForward, int nFlags,
						ObjectFilterFn fnFilter, PolyFilterFn fnPolyFilter, void* pUserData, LTFLOAT* pfDist)
{
	*pfDist = 0.0f;

	m_fNextUpdate = g_pLTServer->GetTime() + m_fUpdateRate;

	if (!hObj) return LTFALSE;

	LTVector vTargetPos;
	g_pLTServer->GetObjectPos(hObj, &vTargetPos);

	CCharacter* pChar = LTNULL;
	if (IsCharacter(hObj) == LTTRUE)
	{
		pChar = (CCharacter*)g_pLTServer->HandleToObject(hObj);
		if (pChar)
		{
			vTargetPos += pChar->GetHeadOffset();
		}
	}

	LTVector vDir = vTargetPos - vPos;
	LTFLOAT fDist = vDir.Mag();
	if (fDist > m_fRange) return LTFALSE;

	vDir *= 1.0f / fDist;
	vDir.Norm();

	if (vDir.Dot(vForward) < m_fCosHalfFOV) return LTFALSE;

	IntersectQuery iq;
	iq.m_Flags			= INTERSECT_OBJECTS | IGNORE_NONSOLID;
	iq.m_From			= vPos;
	iq.m_To				= vTargetPos;
	iq.m_FilterFn		= fnFilter;
	iq.m_PolyFilterFn	= fnPolyFilter;
	iq.m_pUserData		= pUserData;

	if (!g_pLTServer->IntersectSegment(&iq, &m_Target)) return LTTRUE;

	LTBOOL bHit = LTFALSE;
	if (m_Target.m_hObject == hObj)
	{
		bHit = LTTRUE;
	}

	if (IsCharacterHitBox(m_Target.m_hObject) == LTTRUE)
	{
		CCharacterHitBox* pHitBox = (CCharacterHitBox*)g_pLTServer->HandleToObject(m_Target.m_hObject);
		if (!pHitBox) return LTFALSE;

		if (pHitBox->GetModelObject() == hObj)
		{
			bHit = LTTRUE;
		}
	}

	if (bHit != LTTRUE) return LTFALSE;

	fDist = (m_Target.m_Point - vPos).Mag();

	*pfDist = fDist / m_fRange;
	if (*pfDist > 1.0f)
	{
		*pfDist = 1.0f;
	}

	*pfDist = 1.0f - *pfDist + 0.25f;
	if (*pfDist > 1.0f)
	{
		*pfDist = 1.0f;
	}

	if (pChar && IsPredator(hObj) == LTTRUE && pChar->GetCloakState())
	{
		*pfDist *= m_fFudge;
	}

	return LTTRUE;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CAISight::Save (0x1015b390)
//
// ----------------------------------------------------------------------- //

void CAISight::Save(HMESSAGEWRITE hWrite)
{
	hWrite->WriteFloat(m_fUpdateRate);
	hWrite->WriteFloat(m_fNextUpdate);
	hWrite->WriteFloat(m_fFOV);
	hWrite->WriteFloat(m_fCosHalfFOV);
	hWrite->WriteByte(m_bEnabled);
	hWrite->WriteFloat(m_fRange);
	hWrite->WriteVector(m_Target.m_Point);
	hWrite->WriteVector(m_Target.m_Plane.m_Normal);
	hWrite->WriteFloat(m_Target.m_Plane.m_Dist);
	hWrite->WriteObject(m_Target.m_hObject);
	hWrite->WriteDWord(m_Target.m_hPoly);
	hWrite->WriteDWord(m_Target.m_SurfaceFlags);
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CAISight::Load (0x1015b430)
//
// ----------------------------------------------------------------------- //

void CAISight::Load(HMESSAGEREAD hRead)
{
	m_fUpdateRate				= hRead->ReadFloat();
	m_fNextUpdate				= hRead->ReadFloat();
	m_fFOV						= hRead->ReadFloat();
	m_fCosHalfFOV				= hRead->ReadFloat();
	m_bEnabled					= hRead->ReadByte();
	m_fRange					= hRead->ReadFloat();
	hRead->ReadVectorFL(m_Target.m_Point);
	hRead->ReadVectorFL(m_Target.m_Plane.m_Normal);
	m_Target.m_Plane.m_Dist		= hRead->ReadFloat();
	m_Target.m_hObject			= hRead->ReadObject();
	m_Target.m_hPoly			= hRead->ReadDWord();
	m_Target.m_SurfaceFlags		= hRead->ReadDWord();
}

#endif // PRIMAL_HUNT
