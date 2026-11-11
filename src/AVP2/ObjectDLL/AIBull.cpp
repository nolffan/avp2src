// ----------------------------------------------------------------------- //
//
// MODULE  : AIBull.cpp
//
// PURPOSE : CAIBull - Primal Hunt's charging beast, the buhlagh
//
// PH: no source of this survives. Written from PHObject.lto, where its
// object is 0x10146370..0x101486e0; functions are in retail order, with
// their addresses. Those past 0x10148000 are n2's.
//
// ----------------------------------------------------------------------- //

#include "stdafx.h"

#ifdef PRIMAL_HUNT

#include "AIBull.h"
#include "ServerUtilities.h"
#include "CharacterHitBox.h"
#include "PlayerObj.h"
#include "MsgIDs.h"
#include "CommonUtilities.h"

// PH: 0x102e3414
static const char* s_aszSounds[] =
{
	"sounds/exp/characters/buhlagh/sight1.wav",
	"sounds/exp/characters/buhlagh/sight2.wav",
	"sounds/exp/characters/buhlagh/death1.wav",
	"sounds/exp/characters/buhlagh/death2.wav",
	"sounds/exp/characters/buhlagh/step1.wav",
	"sounds/exp/characters/buhlagh/step2.wav",
	"sounds/exp/characters/buhlagh/step3.wav",
	"sounds/exp/characters/buhlagh/step4.wav",
	"sounds/exp/characters/buhlagh/pain1.wav",
	"sounds/exp/characters/buhlagh/pain2.wav",
	"sounds/exp/characters/buhlagh/idle1.wav",
	"sounds/exp/characters/buhlagh/idle2.wav",
	"sounds/exp/characters/buhlagh/snort1.wav",
	"sounds/exp/characters/buhlagh/snort2.wav",
	"sounds/exp/characters/buhlagh/impact1.wav",
	"sounds/exp/characters/buhlagh/run.wav",
	"sounds/exp/characters/buhlagh/breathing1.wav",
	"sounds/exp/characters/buhlagh/breathing2.wav",
};

// PH: 0x102e345c, one set of skins for each vision mode
static const char* s_aszSkins[][4] =
{
	{ "skins/exp/characters/buhlaghhead.dtx", "skins/exp/characters/buhlaghbody.dtx", LTNULL, LTNULL },
	{ "skins/exp/characters/buhlaghhead.dtx", "skins/exp/characters/buhlaghbody.dtx", LTNULL, LTNULL },
	{ "skins/exp/characters/heatvision/buhlaghhead.spr", "skins/exp/characters/heatvision/buhlaghbody.spr", LTNULL, LTNULL },
	{ "skins/exp/characters/electrovision/buhlaghhead.dtx", "skins/exp/characters/electrovision/buhlaghbody.dtx", LTNULL, LTNULL },
	{ "skins/exp/characters/navvision/buhlaghhead.dtx", "skins/exp/characters/navvision/buhlaghbody.dtx", LTNULL, LTNULL },
};

// PH: 0x102e34ac, 0x102e34b0
static const char* s_szHeadPiece	= "Head";
static const char* s_szHeadSocket	= "Skull";

BEGIN_CLASS(CAIBull)
	ADD_VECTORPROP_VAL_FLAG(Dims, 48.0f, 64.0f, 48.0f, PF_HIDDEN | PF_DIMS)
END_CLASS_DEFAULT(CAIBull, CAIBase, NULL, NULL)

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CAIBull::CAIBull (0x101464d0)
//
// ----------------------------------------------------------------------- //

CAIBull::CAIBull() : CAIBase(OT_MODEL)
{
	m_fNextSightSound	= -1.0f;
	m_fNextIdleSound	= -1.0f;
	m_fNextImpactTime	= -1.0f;

	m_fMaxHitPoints		= 400.0f;
	m_fHitPoints		= 400.0f;

	m_eBullState		= eBullWalk;
	m_fChargeSpeed		= 640.0f;
	m_fChargeTurnRate	= MATH_PI / 4.0f;
	m_fWalkSpeed		= 240.0f;
	m_fTurnRate			= MATH_PI;

	m_MoveMgr.m_fMinSpeed		= 640.0f;
	m_OrientMgr.SetTurnRate(m_fTurnRate);

	m_nHeadTracker		= (uint32)-1;
	m_fNextChargeTime	= 0.0f;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CAIBull::~CAIBull (0x101465b0)
//
// ----------------------------------------------------------------------- //

CAIBull::~CAIBull()
{
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CAIBull::OnPrecreate (0x101465c0)
//
// ----------------------------------------------------------------------- //

uint32 CAIBull::OnPrecreate(void *pData, uint32 nInfo)
{
	CAIBase::OnPrecreate(pData, nInfo);

	if (!pData) return 0;

	if (nInfo == PRECREATE_WORLDFILE)
	{
		ObjectCreateStruct* pStruct = (ObjectCreateStruct*)pData;

		SAFE_STRCPY(pStruct->m_Filename, "models/exp/characters/buhlagh.abc");

		SetSkins(0, pStruct, s_aszSkins);

		pStruct->m_Flags |= FLAG_STAIRSTEP;
	}

	return 1;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CAIBull::OnInitialUpdate (0x10146650)
//
// ----------------------------------------------------------------------- //

uint32 CAIBull::OnInitialUpdate(uint32 nInfo)
{
	uint32 hWeightSet;

	CAIBase::OnInitialUpdate(nInfo);

	g_pLTServer->SetObjectMass(m_hObject, 4000.0f);

	m_nHeadTracker = m_Animator.AddTracker();
	hWeightSet = m_Animator.FindWeightSet("Full");
	m_Animator.SetWeightSet(hWeightSet, 0);
	hWeightSet = m_Animator.FindWeightSet("Empty");
	m_Animator.SetWeightSet(hWeightSet, m_nHeadTracker);
	m_Animator.SetAllowTransition(0, LTTRUE);

	m_szHeadPiece	= s_szHeadPiece;
	m_szHeadSocket	= s_szHeadSocket;

	if (nInfo != INITIALUPDATE_SAVEGAME)
	{
		SetNextUpdate(0.001f);

		m_Animator.PlayAnim("Idle0", LTTRUE, 0);
		m_Animator.PlayAnim("Idle0", LTTRUE, m_nHeadTracker);

		m_MoveMgr.m_bInstant	= LTFALSE;
		m_MoveMgr.m_fSpeed	= 1280.0f;

		m_fNextImpactTime = g_pLTServer->GetTime() + 0.001f;
	}

	return 1;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CAIBull::OnLoad (0x10146770)
//
// ----------------------------------------------------------------------- //

void CAIBull::OnLoad(HMESSAGEREAD hRead)
{
	CAIBase::OnLoad(hRead);

	m_eBullState		= hRead->ReadByte();
	m_Dist.m_fMin			= hRead->ReadFloat();
	m_Dist.m_fMax			= hRead->ReadFloat();
	m_Dist.m_fDist				= hRead->ReadFloat();
	m_Dist.m_nAxes			= hRead->ReadDWord();
	m_bHeadDown			= hRead->ReadByte();
	m_fNextChargeTime	= hRead->ReadFloat();
	m_fNextSightSound	= hRead->ReadFloat();
	m_fNextIdleSound	= hRead->ReadFloat();
	m_fNextImpactTime	= hRead->ReadFloat();
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CAIBull::OnSave (0x10146870)
//
// ----------------------------------------------------------------------- //

void CAIBull::OnSave(HMESSAGEWRITE hWrite)
{
	CAIBase::OnSave(hWrite);

	hWrite->WriteByte(m_eBullState);
	*hWrite << m_Dist.m_fMin;
	*hWrite << m_Dist.m_fMax;
	*hWrite << m_Dist.m_fDist;
	hWrite->WriteDWord(m_Dist.m_nAxes);
	hWrite->WriteByte(m_bHeadDown);
	*hWrite << m_fNextChargeTime;
	*hWrite << m_fNextSightSound;
	*hWrite << m_fNextIdleSound;
	*hWrite << m_fNextImpactTime;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CAIBull::UpdateIdle (0x10146910)
//
// ----------------------------------------------------------------------- //

void CAIBull::UpdateIdle(LTFLOAT fFrameTime)
{
	CheckForPlayer();

	if (ChooseTarget() == LTTRUE)
	{
		m_Sounds.PlayAlert(s_aszSounds[GetRandom(0, 1)]);

		AIBaseStateInfo info(eStateAlert, eStateAlert);
		SetState(&info);
	}

	if (g_pLTServer->GetTime() >= m_fNextSightSound)
	{
		LTFLOAT fTime = g_pLTServer->GetTime();
		LTFLOAT fDelay = GetRandom(2.0f, 8.0f);
		m_fNextSightSound = fTime + fDelay;

		m_Sounds.PlayVoice(s_aszSounds[GetRandom(10, 11)]);
	}
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CAIBull::UpdateAlert (0x10146a20)
//
// ----------------------------------------------------------------------- //

void CAIBull::UpdateAlert(LTFLOAT fFrameTime)
{
	if (m_bHeadDown == LTTRUE && (m_Animator.GetPlaybackState(m_nHeadTracker) & MS_PLAYDONE))
	{
		RaiseHead();
	}

	switch (m_eBullState)
	{
		case eBullWalk:		UpdateWalk(fFrameTime);		break;
		case eBullCharge:	UpdateCharge(fFrameTime);	break;
		case eBullRam:		UpdateRam(fFrameTime);		break;
	}

	if (g_pLTServer->GetTime() >= m_fNextIdleSound)
	{
		LTFLOAT fTime = g_pLTServer->GetTime();
		LTFLOAT fDelay = GetRandom(1.0f, 4.0f);
		m_fNextIdleSound = fTime + fDelay;

		m_Sounds.PlayVoice(s_aszSounds[GetRandom(12, 13)]);
	}
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CAIBull::SetStateIdle (0x10146b10)
//
// ----------------------------------------------------------------------- //

LTBOOL CAIBull::SetStateIdle(const AIBaseStateInfo* pInfo)
{
	CAIBase::SetStateIdle(pInfo);

	m_Animator.PlayAnim("Idle0", LTTRUE, 0);
	m_OrientMgr.SetFacePos(LTFALSE);
	m_MoveMgr.SetMoveType(1);

	LTFLOAT fTime = g_pLTServer->GetTime();
	LTFLOAT fDelay = GetRandom(2.0f, 8.0f);
	m_fNextSightSound = fTime + fDelay;

	m_hTarget = LTNULL;

	return LTTRUE;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CAIBull::SetStateAlert (0x10146bd0)
//
// ----------------------------------------------------------------------- //

LTBOOL CAIBull::SetStateAlert(const AIBaseStateInfo* pInfo)
{
	CAIBase::SetStateAlert(pInfo);

	m_eBullState = eBullWalk;

	LTFLOAT fTime = g_pLTServer->GetTime();
	LTFLOAT fDelay = GetRandom(1.0f, 4.0f);
	m_fNextIdleSound = fTime + fDelay;

	return LTTRUE;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CAIBull::SetStateDying (0x10146c30)
//
// ----------------------------------------------------------------------- //

LTBOOL CAIBull::SetStateDying(const AIBaseStateInfo* pInfo)
{
	int eOldState = m_eState;

	CAIBase::SetStateDying(pInfo);

	if (eOldState == eStateAlert)
	{
		LTVector vVel;
		g_pLTServer->Physics()->GetVelocity(m_hObject, &vVel);

		m_MoveMgr.SetMoveType(eOldState);

		LTFLOAT fSpeed = vVel.Mag();
		if (fSpeed > m_fWalkSpeed)
		{
			m_Animator.PlayAnim("Death1", LTFALSE, 0);
			g_pLTServer->Physics()->SetFrictionCoefficient(m_hObject, 0.0f);
			m_MoveMgr.m_fSpeed = fSpeed * 0.5f;
		}
	}

	return LTTRUE;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CAIBull::SetStateMoving (0x10146cf0)
//
// ----------------------------------------------------------------------- //

LTBOOL CAIBull::SetStateMoving(const AIBaseStateInfo* pInfo)
{
	if (!CAIBase::SetStateMoving(pInfo)) return LTFALSE;

	m_MoveMgr.SetMinSpeed(m_fChargeSpeed);
	m_OrientMgr.SetTurnRate(m_fTurnRate);

	return LTTRUE;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CAIBull::UpdateWalk (0x10146d30)
//
// ----------------------------------------------------------------------- //

void CAIBull::UpdateWalk(LTFLOAT fFrameTime)
{
	AIBaseStateInfo info(eStateIdle);

	if (IsTargetGone() == LTTRUE)
	{
		SetState(&info);
		return;
	}

	LTVector vTargetPos;
	g_pLTServer->GetObjectPos(m_hTarget, &vTargetPos);

	m_MoveMgr.SetDest(vTargetPos);
	m_StuckCheck.m_bEnabled = LTFALSE;
	m_OrientMgr.SetFacePos(LTTRUE);
	m_OrientMgr.SetFaceTarget(vTargetPos);
	m_MoveMgr.SetMoveType(1);

	LTVector vDir = vTargetPos - m_MoveMgr.GetPos(m_hObject);
	LTFLOAT fDist = vDir.Mag();

	if (ReturnToLeash() == LTTRUE) return;

	CLeashNode* pNode = m_Leash.FindNode(vTargetPos);
	if (!pNode)
	{
		m_Leash.m_bOutside = LTFALSE;

		AIBaseStateInfo home(eStateMoving);
		home.vPos = m_Leash.m_vHomePos;
		if (!SetState(&home))
		{
			AIBaseStateInfo idle(eStateIdle);
			SetState(&idle);
		}
		return;
	}

	if (pNode->m_nID != m_Leash.m_nCurNode)
	{
		AIBaseStateInfo move(eStateMoving, eStateAlert);
		move.vPos = vTargetPos;
		if (!SetState(&move))
		{
			AIBaseStateInfo idle(eStateIdle);
			SetState(&idle);
		}
		return;
	}

	if (fDist <= 512.0f)
	{
		if (g_pLTServer->GetTime() < m_fNextChargeTime) return;

		vDir *= 1.0f / fDist;

		// Ram unless we're already facing within 15 degrees of the target;
		// then charge (0x10147020: retail's test, and the way round it is)
		LTVector vForward = m_OrientMgr.m_vForward;
		if (vDir.Dot(vForward) < cos(MATH_PI / 12.0f))
		{
			StartRam();
			return;
		}
	}

	StartCharge();
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CAIBull::UpdateCharge (0x10147050)
//
// ----------------------------------------------------------------------- //

void CAIBull::UpdateCharge(LTFLOAT fFrameTime)
{
	AIBaseStateInfo info(eStateIdle);

	if (IsTargetGone() == LTTRUE)
	{
		SetState(&info);
		return;
	}

	LTVector vForward = m_OrientMgr.m_vForward;
	m_MoveMgr.SetDest(m_MoveMgr.GetPos(m_hObject) + vForward * 128.0f);

	LTVector vTargetPos;
	g_pLTServer->GetObjectPos(m_hTarget, &vTargetPos);
	m_OrientMgr.SetFacePos(LTTRUE);
	m_OrientMgr.SetFaceTarget(vTargetPos);

	if (ReturnToLeash() == LTTRUE) return;

	LTVector vDiff = vTargetPos - m_MoveMgr.GetPos(m_hObject);
	if (!(m_Dist.m_nAxes & 1)) vDiff.x = 0.0f;
	if (!(m_Dist.m_nAxes & 2)) vDiff.y = 0.0f;
	if (!(m_Dist.m_nAxes & 4)) vDiff.z = 0.0f;

	LTFLOAT fDist = vDiff.Mag();
	m_Dist.m_fDist = fDist;
	if (fDist < m_Dist.m_fMin) m_Dist.m_fMin = fDist;
	if (fDist > m_Dist.m_fMax) m_Dist.m_fMax = fDist;

	if (m_Dist.m_fDist > m_Dist.m_fMin && m_Dist.m_fDist > 128.0f && m_Dist.m_fDist - m_Dist.m_fMin > 256.0f)
	{
		StartRam();
		return;
	}

	if (m_Dist.m_fDist > 256.0f)
	{
		LTVector vDir = vTargetPos - m_MoveMgr.GetPos(m_hObject);
		vDir.Norm();

		if (vDir.Dot(m_OrientMgr.m_vForward) < 0.5f)
		{
			StartRam();
			return;
		}
	}

	if (m_Dist.m_fDist < 128.0f)
	{
		LowerHead();
	}
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CAIBull::UpdateRam (0x101473d0)
//
// ----------------------------------------------------------------------- //

void CAIBull::UpdateRam(LTFLOAT fFrameTime)
{
	AIBaseStateInfo info(eStateIdle);

	if (IsTargetGone() == LTTRUE)
	{
		SetState(&info);
		return;
	}

	LTVector vForward = m_OrientMgr.m_vForward;
	m_MoveMgr.SetDest(m_MoveMgr.GetPos(m_hObject) + vForward * 128.0f);

	LTVector vTargetPos;
	g_pLTServer->GetObjectPos(m_hTarget, &vTargetPos);
	m_OrientMgr.SetFacePos(LTTRUE);
	m_OrientMgr.SetFaceTarget(vTargetPos);

	if (ReturnToLeash() == LTTRUE) return;

	CLeashNode* pNode = m_Leash.FindNode(vTargetPos);
	if (!pNode)
	{
		m_Leash.m_bOutside = LTFALSE;

		AIBaseStateInfo home(eStateMoving);
		home.vPos = m_Leash.m_vHomePos;
		if (!SetState(&home))
		{
			AIBaseStateInfo idle(eStateIdle);
			SetState(&idle);
		}
		return;
	}

	if (pNode->m_nID != m_Leash.m_nCurNode)
	{
		AIBaseStateInfo move(eStateMoving, eStateAlert);
		move.vPos = vTargetPos;
		if (!SetState(&move))
		{
			AIBaseStateInfo idle(eStateIdle);
			SetState(&idle);
		}
		return;
	}

	LTVector vDir = vTargetPos - m_MoveMgr.GetPos(m_hObject);
	vDir.Norm();

	if (vDir.Dot(m_OrientMgr.m_vForward) > (LTFLOAT)cos(MATH_DEGREES_TO_RADIANS(15.0f)))
	{
		StartCharge();
	}
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CAIBull::StartWalk (0x10147740)
//
// ----------------------------------------------------------------------- //

void CAIBull::StartWalk()
{
	m_eBullState = eBullWalk;

	LTVector vTargetPos;
	g_pLTServer->GetObjectPos(m_hTarget, &vTargetPos);

	m_MoveMgr.SetDest(vTargetPos);
	m_MoveMgr.SetMoveType(1);
	m_OrientMgr.SetTurnRate(m_fTurnRate);
	m_OrientMgr.SetFacePos(LTTRUE);
	m_OrientMgr.SetFaceTarget(vTargetPos);

	m_Animator.PlayAnim("Idle0", LTTRUE, 0);

	m_StuckCheck.m_bEnabled = LTFALSE;
	m_fNextChargeTime = g_pLTServer->GetTime() + 1.0f;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CAIBull::StartCharge (0x10147800)
//
// ----------------------------------------------------------------------- //

void CAIBull::StartCharge()
{
	m_eBullState = eBullCharge;

	LTVector vTargetPos;
	g_pLTServer->GetObjectPos(m_hTarget, &vTargetPos);

	m_MoveMgr.SetDest(vTargetPos);
	m_MoveMgr.SetMoveType(2);
	m_MoveMgr.SetMinSpeed(m_fChargeSpeed);
	m_OrientMgr.SetTurnRate(m_fChargeTurnRate);
	m_OrientMgr.SetFacePos(LTTRUE);

	m_Animator.PlayAnim("Run", LTTRUE, 0);

	m_Dist.m_fMin	= 9999999.0f;
	m_Dist.m_fMax	= -9999999.0f;
	m_Dist.m_fDist		= 0.0f;
	m_Dist.m_nAxes	= 7;

	m_StuckCheck.m_bEnabled = LTTRUE;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CAIBull::StartRam (0x101478d0)
//
// ----------------------------------------------------------------------- //

void CAIBull::StartRam()
{
	m_eBullState = eBullRam;

	LTVector vTargetPos;
	g_pLTServer->GetObjectPos(m_hTarget, &vTargetPos);

	m_MoveMgr.SetDest(vTargetPos);
	m_MoveMgr.SetMoveType(eBullRam);
	m_MoveMgr.SetMinSpeed(m_fWalkSpeed);
	m_OrientMgr.SetTurnRate(m_fTurnRate * 0.75f);
	m_OrientMgr.SetFacePos(LTTRUE);
	m_StuckCheck.m_bEnabled = LTTRUE;

	HMODELANIM hRun = m_Animator.FindAnim("Run");
	if (m_Animator.GetAnim(0) != hRun)
	{
		m_Animator.PlayAnim("Run", LTTRUE, 0);
	}

	if (ReturnToLeash() == LTTRUE) return;

	CLeashNode* pNode = m_Leash.FindNode(vTargetPos);
	if (!pNode)
	{
		m_Leash.m_bOutside = LTFALSE;

		AIBaseStateInfo home(eStateMoving);
		home.vPos = m_Leash.m_vHomePos;
		if (!SetState(&home))
		{
			AIBaseStateInfo idle(eStateIdle);
			SetState(&idle);
		}
		return;
	}

	if (pNode->m_nID != m_Leash.m_nCurNode)
	{
		AIBaseStateInfo move(eStateMoving, eStateAlert);
		move.vPos = vTargetPos;
		if (!SetState(&move))
		{
			AIBaseStateInfo idle(eStateIdle);
			SetState(&idle);
		}
	}
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CAIBull::LowerHead (0x10147ac0)
//
// ----------------------------------------------------------------------- //

void CAIBull::LowerHead()
{
	uint32 hWeightSet;

	if (m_bHeadDown == LTTRUE) return;

	m_bHeadDown = LTTRUE;

	hWeightSet = m_Animator.FindWeightSet("Full");
	m_Animator.SetWeightSet(hWeightSet, 0);
	hWeightSet = m_Animator.FindWeightSet("ChargeHead");
	m_Animator.SetWeightSet(hWeightSet, m_nHeadTracker);
	HMODELANIM hAnim = m_Animator.FindAnim("Attack");
	m_Animator.PlayAnim(hAnim, LTFALSE, m_nHeadTracker);
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CAIBull::RaiseHead (0x10147b30)
//
// ----------------------------------------------------------------------- //

void CAIBull::RaiseHead()
{
	uint32 hWeightSet;

	m_bHeadDown = LTFALSE;

	hWeightSet = m_Animator.FindWeightSet("Full");
	m_Animator.SetWeightSet(hWeightSet, 0);
	hWeightSet = m_Animator.FindWeightSet("Empty");
	m_Animator.SetWeightSet(hWeightSet, m_nHeadTracker);
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CAIBull::OnTouchNotify (0x10147b80)
//
// ----------------------------------------------------------------------- //

uint32 CAIBull::OnTouchNotify(void *pData, LTFLOAT fData)
{
	HOBJECT hObj = (HOBJECT)pData;

	char szClass[256];
	g_pLTServer->GetClassName(g_pLTServer->GetObjectClass(hObj), szClass, 256);

	if (IsPlayer(hObj) != LTTRUE)
	{
		if (IsCharacterHitBox(hObj) == LTTRUE)
		{
			CCharacterHitBox* pHitBox = (CCharacterHitBox*)g_pLTServer->HandleToObject(hObj);
			if (!pHitBox) return 0;

			hObj = pHitBox->GetModelObject();
		}
		else if (IsAIBaseOrTentacle(hObj) != LTTRUE)
		{
			if (hObj)
			{
				g_pLTServer->Physics()->IsWorldObject(hObj);
			}
			return 0;
		}
	}

	if (!hObj) return 0;

	HandleFootstep(hObj, 2, 1001.0f);

	LTVector vVel;
	g_pLTServer->Physics()->GetVelocity(m_hObject, &vVel);
	if (vVel.Mag() <= m_fWalkSpeed) return 0;

	LTVector vMyPos, vPos;
	g_pLTServer->GetObjectPos(m_hObject, &vMyPos);
	g_pLTServer->GetObjectPos(hObj, &vPos);

	LTVector vDir(vPos.x - vMyPos.x, 0.0f, vPos.z - vMyPos.z);
	vDir.Norm();

	LTVector vForward = m_OrientMgr.m_vForward;
	if (vDir.Dot(vForward) <= 0.0f) return 0;

	LTFLOAT fDamage = ((LTFLOAT)rand() / RAND_MAX + 1.0f) * 20.0f;

	if (g_pLTServer->GetTime() >= m_fNextImpactTime)
	{
		DamageObject(hObj, &vDir, fDamage, 0.0f, DT_CRUSH);

		m_fNextImpactTime = g_pLTServer->GetTime() + 1.0f;
		m_Sounds.PlaySound(s_aszSounds[14], vMyPos, 100, 0, 128, 256.0f, 3072.0f, 58);
	}

	LTBOOL bPlayer = IsPlayer(hObj);
	LTBOOL bBody = IsBodyProp(hObj);

	CPlayerObj* pPlayer = LTNULL;
	if (bPlayer == LTTRUE)
		pPlayer = (CPlayerObj*)g_pLTServer->HandleToObject(hObj);

	CCharacter* pBody = LTNULL;
	if (bBody == LTTRUE)
		pBody = (CCharacter*)g_pLTServer->HandleToObject(hObj);

	CAIBase* pAI = GetAIBase(hObj);

	if (!(pBody && !pBody->IsDead()))
	{
		if (!pAI) return 0;
		if (pAI->IsDead()) return 0;
	}

	if (pPlayer)
	{
		LTVector vKnock(vDir.x * fDamage * 30.0f, 600.0f, vDir.z * fDamage * 30.0f);

		HMESSAGEWRITE hMsg = g_pLTServer->StartMessage(pPlayer->GetClient(), MID_SERVERFORCEVELOCITY);
		g_pLTServer->WriteToMessageVector(hMsg, &vKnock);
		g_pLTServer->EndMessage(hMsg);
	}
	else
	{
		LTVector vKnock(vDir.x * fDamage * 10.0f, 200.0f, vDir.z * fDamage * 10.0f);
		g_pLTServer->Physics()->SetVelocity(hObj, &vKnock);
	}

	if (m_eState == eStateAlert)
	{
		StartWalk();
	}

	CollisionInfo info;
	g_pLTServer->GetLastCollision(&info);

	g_pLTServer->Physics()->GetVelocity(m_hObject, &vVel);
	vVel += info.m_vStopVel;
	g_pLTServer->Physics()->SetVelocity(m_hObject, &vVel);

	return 1;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CAIBull::HandleDamage (0x10147fc0)
//
// ----------------------------------------------------------------------- //

void CAIBull::HandleDamage(const DamageStruct& damage)
{
	CAIBase::HandleDamage(damage);

	if (m_eState == eStateDying || m_eState == eStateDead) return;

	m_Sounds.PlayPain(s_aszSounds[GetRandom(8, 9)]);

	if (m_hTarget) return;

	if (!CanReact(damage)) return;

	if (IsPlayer(damage.hDamager) == LTTRUE || IsAI(damage.hDamager) == LTTRUE ||
		IsAIBaseOrTentacle(damage.hDamager) == LTTRUE)
	{
		SetTarget(damage.hDamager);

		AIBaseStateInfo info(eStateAlert, eStateAlert);
		SetState(&info);
	}
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CAIBull::OnModelStringKey (0x10148100)
//
// ----------------------------------------------------------------------- //

uint32 CAIBull::OnModelStringKey(void *pData)
{
	if (CAIBase::OnModelStringKey(pData) == LTTRUE) return LTTRUE;

	ArgList* pArgList = (ArgList*)pData;

	LTVector vPos;
	g_pLTServer->GetObjectPos(m_hObject, &vPos);

	if (_stricmp(pArgList->argv[0], "run_sound") == 0)
	{
		m_Sounds.PlaySound(s_aszSounds[15], vPos, 100, 0.0f, 128, 256.0f, 3072.0f, 58);
		m_Sounds.PlaySound(s_aszSounds[GetRandom(16, 17)], vPos, 100, 0.0f, 128, 256.0f, 3072.0f, 58);
	}
	else if (_stricmp(pArgList->argv[0], "step_sound") == 0)
	{
		int nStep = atoi(pArgList->argv[1]) - 1;
		m_Sounds.PlaySound(s_aszSounds[4 + nStep], vPos, 100, 0.0f, 128, 256.0f, 3072.0f, 58);
	}
	else if (_stricmp(pArgList->argv[0], "death_sound") == 0)
	{
		int nSound = atoi(pArgList->argv[1]) - 1;
		m_Sounds.PlayVoice(s_aszSounds[2 + nSound]);
	}

	return 0;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CAIBull::SetVisionMode (0x10148260)
//
//	PURPOSE:	Put on the skins for the player's vision mode
//
// ----------------------------------------------------------------------- //

void CAIBull::SetVisionMode(int nMode)
{
	if (nMode == m_nSoundSet) return;

	ObjectCreateStruct ocs;
	g_pLTServer->GetModelFilenames2(m_hObject, &ocs);
	SetSkins(nMode, &ocs, s_aszSkins);
	g_pLTServer->Common()->SetObjectFilenames(m_hObject, &ocs);
}

// PH: these three are CAIChameleon's too, and folded into its copies
// (0x10148920, 0x1014b400, 0x1014b410)
uint32 CAIBull::OnUpdate(LTFLOAT fFrameTime)
{
	CAIBase::OnUpdate(fFrameTime);
	return 1;
}

LTBOOL CAIBull::HasMoveTarget()
{
	return m_NextState.eState != eStateAlert;
}

LTBOOL CAIBull::ReturnToLeash()
{
	if (m_Leash.m_bOutside == LTTRUE)
	{
		m_Leash.m_bOutside = LTFALSE;

		AIBaseStateInfo home(eStateMoving);
		home.vPos	= m_Leash.m_vHomePos;
		if (!SetState(&home))
		{
			AIBaseStateInfo idle(eStateIdle);
			SetState(&idle);
		}

		return LTTRUE;
	}

	return LTFALSE;
}

#endif // PRIMAL_HUNT
