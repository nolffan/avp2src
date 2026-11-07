// ----------------------------------------------------------------------- //
//
// MODULE  : AITentacle.cpp
//
// PURPOSE : Primal Hunt's tentacle beast: a hidden CAIBase that sends its
//           tentacles (CTentacle) erupting out of the ground at its target
//
// ----------------------------------------------------------------------- //

// PH: Third Law's class, PHObject.lto 0x1014bb20..0x1014d65b (the name of
// the file is ours: retail does not record it).

#include "stdafx.h"

#ifdef PRIMAL_HUNT

#include "AITentacle.h"
#include "Tentacle.h"
#include "ServerUtilities.h"
#include "CharacterMgr.h"
#include "PlayerObj.h"
#include "BodyProp.h"
#include "MsgIDs.h"

// PH: the beast's sounds (0x102e3fcc)
static const char* s_aszSounds[] =
{
	"sounds/exp/characters/tentacle/rumble1.wav",
	"sounds/exp/characters/tentacle/rumble2.wav",
	"sounds/exp/characters/tentacle/final_death1.wav",
	"sounds/exp/characters/tentacle/final_death2.wav",
};

enum TentacleSound
{
	eSoundRumble1 = 0,
	eSoundRumble2,
	eSoundDeath1,
	eSoundDeath2,
};

// PH: the tentacles' skins for each vision mode (0x102e3fdc), as
// CAIBase::SetSkins takes them
static const char* s_aszSkins[][4] =
{
	{ "skins/exp/characters/tentacle.dtx", LTNULL, LTNULL, LTNULL },
	{ "skins/exp/characters/tentacle.dtx", LTNULL, LTNULL, LTNULL },
	{ "skins/exp/characters/heatvision/tentacle.spr", LTNULL, LTNULL, LTNULL },
	{ "skins/exp/characters/electrovision/tentacle.dtx", LTNULL, LTNULL, LTNULL },
	{ "skins/exp/characters/navvision/tentacle.dtx", LTNULL, LTNULL, LTNULL },
};

// PH: the property table and the class's registration, 0x1014bb30 and
// 0x1014bef0; the default constructor, 0x1014bea0
BEGIN_CLASS(CAITentacle)
	ADD_VECTORPROP_VAL_FLAG(Dims, 32.0f, 48.0f, 32.0f, PF_HIDDEN | PF_DIMS)
	ADD_LONGINTPROP_FLAG_HELP(Num_Tentacles, 8, 0, "Number of tentacles total.")
	ADD_LONGINTPROP_FLAG_HELP(Max_Attacking_Tentacles, 2, 0, "Number of tentacles that can attack at once.")
	ADD_REALPROP_FLAG_HELP(Min_Erupt_Time, 0.25f, 0, "Minimum time between tentacles erupting.")
	ADD_REALPROP_FLAG_HELP(Max_Erupt_Time, 4.0f, 0, "Maximum time between tentacles erupting.")
	ADD_REALPROP_FLAG_HELP(Tentacle_Health, 250.0f, 0, "Health of each tentacle.")
	ADD_REALPROP_FLAG_HELP(Tentacle_Damage, 20.0f, 0, "Maximum amount of damage a tentacle attack can do.")
	ADD_REALPROP_FLAG_HELP(Tentacle_Scale, 1.0f, 0, "The relative size of the tentacles.  1.0 = normal size, 2.0 = twice normal size.")
	ADD_BOOLPROP_FLAG_HELP(First_Erupt_In_Front, LTTRUE, 0, "If TRUE the first tentacle will always erupt directly in front of the player.")
END_CLASS_DEFAULT(CAITentacle, CAIBase, NULL, NULL)

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	IsAITentacle() / GetAITentacle()
//
// ----------------------------------------------------------------------- //

// PH: 0x1014bf10
LTBOOL IsAITentacle(HOBJECT hObj)
{
	HCLASS hClass = g_pLTServer->GetClass("CAITentacle");
	if (!hClass) return LTFALSE;

	HCLASS hObjClass = g_pLTServer->GetObjectClass(hObj);
	if (!hObjClass) return LTFALSE;

	return g_pLTServer->IsKindOf(hObjClass, hClass) == LTTRUE;
}

// PH: 0x1014bf60
CAITentacle* GetAITentacle(HOBJECT hObj)
{
	if (!IsAITentacle(hObj)) return LTNULL;

	return (CAITentacle*)g_pLTServer->HandleToObject(hObj);
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CAITentacle::CAITentacle()
//
//	PURPOSE:	Initialize object
//
// ----------------------------------------------------------------------- //

// PH: 0x1014bf90
CAITentacle::CAITentacle() : CAIBase(OT_NORMAL)
{
	m_Memory.SetRange(0, 999999.0f);
	m_Memory.SetDecay(0, 999999.0f);
	m_Memory.SetRange(1, 1000.0f);
	m_Memory.SetDecay(1, 50.0f);
	m_Memory.SetRange(2, 999999.0f);
	m_Memory.SetDecay(2, 999999.0f);

	memset(m_apTentacles, 0, sizeof(m_apTentacles));

	m_nNumTentacles			= 0;
	m_nMaxAttacking			= 0;
	m_bJustLoaded			= LTFALSE;
	m_bGivingUp				= LTFALSE;
	m_fMinEruptTime			= 0.25f;
	m_fMaxEruptTime			= 4.0f;
	m_fTentacleHealth		= 250.0f;
	m_fTentacleDamage		= 40.0f;
	m_fTentacleScale		= 1.0f;
	m_MoveMgr.m_eMoveType	= 0;
	m_bFirstEruptInFront	= LTTRUE;
}

// PH: 0x1014c0d0
CAITentacle::~CAITentacle()
{
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CAITentacle::OnPrecreate()
//
//	PURPOSE:	Read the properties
//
// ----------------------------------------------------------------------- //

// PH: 0x1014c0e0
uint32 CAITentacle::OnPrecreate(void *pData, uint32 nInfo)
{
	CAIBase::OnPrecreate(pData, nInfo);

	if (nInfo == PRECREATE_WORLDFILE)
	{
		ObjectCreateStruct* pStruct = (ObjectCreateStruct*)pData;

		g_pLTServer->GetPropLongInt("Num_Tentacles", (long*)&m_nNumTentacles);
		g_pLTServer->GetPropLongInt("Max_Attacking_Tentacles", (long*)&m_nMaxAttacking);
		g_pLTServer->GetPropReal("Min_Erupt_Time", &m_fMinEruptTime);
		g_pLTServer->GetPropReal("Max_Erupt_Time", &m_fMaxEruptTime);
		g_pLTServer->GetPropReal("Tentacle_Health", &m_fTentacleHealth);
		g_pLTServer->GetPropReal("Tentacle_Damage", &m_fTentacleDamage);
		g_pLTServer->GetPropReal("Tentacle_Scale", &m_fTentacleScale);
		g_pLTServer->GetPropBool("First_Erupt_In_Front", &m_bFirstEruptInFront);

		pStruct->m_Flags	= 0;
		pStruct->m_Flags2	= 0;

		g_pLTServer->GetPropVector("Pos", &m_vPos);
	}

	return 0;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CAITentacle::OnInitialUpdate()
//
//	PURPOSE:	Make the tentacles
//
// ----------------------------------------------------------------------- //

// PH: 0x1014c210
uint32 CAITentacle::OnInitialUpdate(uint32 nInfo)
{
	CAIBase::OnInitialUpdate(nInfo);

	LTVector vDims(32.0f, 48.0f, 32.0f);
	g_pLTServer->SetObjectDims(m_hObject, &vDims);

	SetNextUpdate(0.001f);

	if (nInfo != INITIALUPDATE_SAVEGAME)
	{
		m_tmrRumble.Start(5.0f);

		LTVector vPos;
		g_pLTServer->GetObjectPos(m_hObject, &vPos);

		for (int i = 0; i < m_nNumTentacles; i++)
		{
			m_apTentacles[i] = CreateTentacle(m_hObject, vPos, m_fTentacleHealth, m_fTentacleDamage, m_fTentacleScale);
		}
	}

	return 0;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CAITentacle::OnUpdate()
//
//	PURPOSE:	Stay put
//
// ----------------------------------------------------------------------- //

// PH: 0x1014c310
uint32 CAITentacle::OnUpdate(LTFLOAT fFrameTime)
{
	g_pLTServer->SetObjectPos(m_hObject, &m_vPos);
	m_StuckCheck.m_bEnabled = LTFALSE;

	if (m_bJustLoaded == LTTRUE)
	{
		m_bJustLoaded = LTFALSE;
		SetNextUpdate(0.001f);
		return 0;
	}

	CAIBase::OnUpdate(fFrameTime);
	g_pLTServer->SetObjectPos(m_hObject, &m_vPos);

	return 0;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CAITentacle::OnLoad()
//
//	PURPOSE:	Restore the object (the tentacles add themselves back)
//
// ----------------------------------------------------------------------- //

// PH: 0x1014c390
void CAITentacle::OnLoad(HMESSAGEREAD hRead)
{
	CAIBase::OnLoad(hRead);

	m_tmrRumble.m_fTime		= hRead->ReadFloat();
	m_nMaxAttacking			= hRead->ReadDWord();
	m_tmrErupt.m_fTime		= hRead->ReadFloat();
	m_tmrGiveUp.m_fTime		= hRead->ReadFloat();
	m_bGivingUp				= hRead->ReadByte();
	m_fMinEruptTime			= hRead->ReadFloat();
	m_fMaxEruptTime			= hRead->ReadFloat();
	m_fTentacleHealth		= hRead->ReadFloat();
	m_fTentacleDamage		= hRead->ReadFloat();
	m_fTentacleScale		= hRead->ReadFloat();
	m_bFirstEruptInFront	= hRead->ReadByte();
	hRead->ReadVectorFL(m_vPos);

	m_nNumTentacles	= 0;
	m_bJustLoaded	= LTTRUE;
	memset(m_apTentacles, 0, sizeof(m_apTentacles));
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CAITentacle::OnSave()
//
//	PURPOSE:	Save the object
//
// ----------------------------------------------------------------------- //

// PH: 0x1014c4e0
void CAITentacle::OnSave(HMESSAGEWRITE hWrite)
{
	CAIBase::OnSave(hWrite);

	hWrite->WriteFloat(m_tmrRumble.m_fTime);
	hWrite->WriteDWord(m_nMaxAttacking);
	hWrite->WriteFloat(m_tmrErupt.m_fTime);
	hWrite->WriteFloat(m_tmrGiveUp.m_fTime);
	hWrite->WriteByte(m_bGivingUp);
	hWrite->WriteFloat(m_fMinEruptTime);
	hWrite->WriteFloat(m_fMaxEruptTime);
	hWrite->WriteFloat(m_fTentacleHealth);
	hWrite->WriteFloat(m_fTentacleDamage);
	hWrite->WriteFloat(m_fTentacleScale);
	hWrite->WriteByte(m_bFirstEruptInFront);
	hWrite->WriteVector(m_vPos);
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CAITentacle::AddTentacle()
//
//	PURPOSE:	A tentacle restored from a saved game hands itself back
//
// ----------------------------------------------------------------------- //

// PH: 0x1014c5a0
LTBOOL CAITentacle::AddTentacle(CTentacle* pTentacle)
{
	if (m_nNumTentacles < AITENTACLE_MAX_TENTACLES && pTentacle)
	{
		m_apTentacles[m_nNumTentacles] = pTentacle;
		m_nNumTentacles++;
		return LTTRUE;
	}

	return LTFALSE;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CAITentacle::HandleSound()
//
//	PURPOSE:	Feel the player's footsteps
//
// ----------------------------------------------------------------------- //

// PH: 0x1014c5e0
void CAITentacle::HandleSound(HOBJECT hObj, const LTVector& vPos, LTFLOAT fVolume, uint32 nSurfaceType, LTFLOAT fTime)
{
	if (IsDead() == LTTRUE) return;
	if (!IsPlayer(hObj)) return;

	LTVector vMyPos;
	g_pLTServer->GetObjectPos(m_hObject, &vMyPos);

	LTFLOAT fDist = (vMyPos - vPos).Mag();
	if (fDist > m_fFootstepHearingRange)
	{
		fDist = m_fFootstepHearingRange;
	}

	fVolume = (1.0f - fDist / m_fFootstepHearingRange) * fVolume * 1000.0f * m_fFootstepSensitivity;
	if (fVolume > 0.0f)
	{
		HandleFootstep(hObj, LTTRUE, fVolume);
	}
}

// PH: 0x1014c6d0
LTBOOL CAITentacle::ChooseTarget()
{
	HOBJECT hTarget = m_Memory.GetBest(1);
	if (!hTarget) return LTFALSE;

	LTVector vPos;
	g_pLTServer->GetObjectPos(hTarget, &vPos);
	if (!m_Leash.FindNode(vPos)) return LTFALSE;

	SetTarget(hTarget);
	return LTTRUE;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CAITentacle::UpdateIdle()
//
//	PURPOSE:	Rumble now and then, and wait for a target
//
// ----------------------------------------------------------------------- //

// PH: 0x1014c740
void CAITentacle::UpdateIdle(LTFLOAT fFrameTime)
{
	if (ChooseTarget() == LTTRUE)
	{
		AIBaseStateInfo alert;
		alert.eState = eStateAlert;
		alert.nParam = 1;
		SetState(&alert);
	}

	if (!m_tmrRumble.IsDone()) return;

	LTFLOAT fNow = g_pLTServer->GetTime();
	m_tmrRumble.m_fTime = fNow + GetRandom(1.0f, 2.0f) * 5.0f;

	LTVector vPos;
	g_pLTServer->GetObjectPos(m_hObject, &vPos);

	LTFLOAT fShake = m_fTentacleScale * 4.0f;
	LTVector vShake(fShake, fShake, fShake);

	HLTSOUND hSound = m_Sounds.PlaySound(s_aszSounds[eSoundRumble1 + GetRandom(1)], vPos, 100, 0.0f, 0,
		m_fFootstepHearingRange, m_fFootstepHearingRange * 2.0f, 0x3a);

	LTFLOAT fDuration;
	g_pLTServer->SoundMgr()->GetSoundDuration(hSound, fDuration);

	ShakePlayer(vPos, vShake, m_fFootstepHearingRange, fDuration * 0.75f, 0.05f, 0.1f);
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CAITentacle::UpdateAlert()
//
//	PURPOSE:	Send tentacles at the target, or give up on it
//
// ----------------------------------------------------------------------- //

// PH: 0x1014c8c0
void CAITentacle::UpdateAlert(LTFLOAT fFrameTime)
{
	AIBaseStateInfo idle;

	if (!m_hTarget)
	{
		m_Memory.ClearAll();
		SetState(&idle);
		return;
	}

	if (AllDead() == LTTRUE)
	{
		m_Memory.ClearAll();

		AIBaseStateInfo dying;
		dying.eState = eStateDying;
		dying.nParam = 3;
		SetState(&dying);
		return;
	}

	if (m_bGivingUp == LTTRUE && m_tmrGiveUp.IsDone())
	{
		m_bGivingUp = LTFALSE;
		SetState(&idle);
		m_Memory.Forget(m_hTarget);
		return;
	}

	if (!m_bGivingUp && IsBodyProp(m_hTarget) == LTTRUE)
	{
		BodyProp* pBody = (BodyProp*)g_pLTServer->HandleToObject(m_hTarget);
		if (pBody && pBody->GetCanFade() == LTTRUE)
		{
			m_bGivingUp = LTTRUE;
			m_tmrGiveUp.Start(5.0f);
		}
	}

	LTBOOL bOnLeash = LTFALSE;
	if (!m_bGivingUp)
	{
		bOnLeash = LTTRUE;

		LTVector vPos;
		g_pLTServer->GetObjectPos(m_hTarget, &vPos);
		if (!m_Leash.FindNode(vPos))
		{
			m_bGivingUp = LTTRUE;
			m_tmrGiveUp.Start(15.0f);
		}
	}

	if (!m_tmrErupt.IsDone()) return;

	LTFLOAT fNow = g_pLTServer->GetTime();
	m_tmrErupt.m_fTime = fNow + GetRandom(m_fMinEruptTime, m_fMaxEruptTime);

	if (!bOnLeash)
	{
		LTVector vPos;
		g_pLTServer->GetObjectPos(m_hTarget, &vPos);
		if (!m_Leash.FindNode(vPos)) return;
	}

	if (CountAttacking() < m_nMaxAttacking)
	{
		EruptTentacle(LTFALSE);
	}
}

// PH: 0x1014cbd0
void CAITentacle::UpdateDying(LTFLOAT fFrameTime)
{
	AIBaseStateInfo dead;
	dead.eState = eStateDead;
	dead.nParam = 3;
	SetState(&dead);
}

// PH: 0x1014cc30
LTBOOL CAITentacle::SetStateIdle(const AIBaseStateInfo* pInfo)
{
	return CAIBase::SetStateIdle(pInfo);
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CAITentacle::SetStateAlert()
//
//	PURPOSE:	Go after the target, first tentacle in front of it
//
// ----------------------------------------------------------------------- //

// PH: 0x1014cc40
LTBOOL CAITentacle::SetStateAlert(const AIBaseStateInfo* pInfo)
{
	CAIBase::SetStateAlert(pInfo);

	m_StuckCheck.m_bEnabled = LTFALSE;

	if (!m_hTarget)
	{
		AIBaseStateInfo idle;
		SetState(&idle);
		return LTFALSE;
	}

	m_bGivingUp = LTFALSE;
	LTFLOAT fNow = g_pLTServer->GetTime();
	m_tmrErupt.m_fTime = fNow + GetRandom(m_fMinEruptTime, m_fMaxEruptTime);

	LTVector vPos;
	g_pLTServer->GetObjectPos(m_hTarget, &vPos);

	EruptTentacle(m_bFirstEruptInFront);

	return LTTRUE;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CAITentacle::SetStateDying()
//
//	PURPOSE:	The last tentacle is dead: one last rumble
//
// ----------------------------------------------------------------------- //

// PH: 0x1014cd60
LTBOOL CAITentacle::SetStateDying(const AIBaseStateInfo* pInfo)
{
	m_eState = eStateDying;

	LTVector vPos;
	g_pLTServer->GetObjectPos(m_hObject, &vPos);

	LTFLOAT fShake = m_fTentacleScale * 8.0f;
	LTVector vShake(fShake, fShake, fShake);

	HLTSOUND hSound = m_Sounds.PlaySound(s_aszSounds[eSoundDeath1 + GetRandom(1)], vPos, 100, 0.0f, 0,
		m_fFootstepHearingRange * 2.0f, m_fFootstepHearingRange * 4.0f, 0x3a);

	LTFLOAT fDuration;
	g_pLTServer->SoundMgr()->GetSoundDuration(hSound, fDuration);

	ShakePlayer(vPos, vShake, m_fFootstepHearingRange * m_fTentacleScale * 2.0f, fDuration, 0.025f, 0.05f);

	return LTTRUE;
}

// PH: 0x1014ce50
LTBOOL CAITentacle::SetStateDead(const AIBaseStateInfo* pInfo)
{
	m_eState = eStateDead;
	g_pLTServer->SetObjectState(m_hObject, OBJSTATE_INACTIVE);
	return LTTRUE;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CAITentacle::FindEruptPos()
//
//	PURPOSE:	Find a free place next to the target for a tentacle, trying
//				eight directions around it
//
// ----------------------------------------------------------------------- //

// PH: 0x1014ce80
LTBOOL CAITentacle::FindEruptPos(LTVector* pvPos, LTBOOL bInFront)
{
	LTVector vTargetPos;
	g_pLTServer->GetObjectPos(m_hTarget, &vTargetPos);

	LTRotation rRot;
	g_pLTServer->GetObjectRotation(m_hTarget, &rRot);

	LTVector vUp, vRight, vForward;
	g_pLTServer->GetRotationVectors(&rRot, &vUp, &vRight, &vForward);

	LTVector vDims(32.0f, 48.0f, 32.0f);

	LTVector vTargetDims;
	if (m_hTarget && m_hTarget)
	{
		g_pLTServer->Physics()->GetObjectDims(m_hTarget, &vTargetDims);
	}
	else
	{
		vTargetDims = LTVector(32.0f, 50.0f, 32.0f);
	}

	IsPlayerInBox(vTargetPos, vDims);

	LTVector vDir;
	if (!bInFront)
	{
		RotateAroundY(vForward, vDir, (LTFLOAT)(GetRandom() % 8) * MATH_PI / 4.0f);
	}
	else
	{
		vDir = vForward;
	}

	LTFLOAT fDist = (vDims.x + 32.0f) * m_fTentacleScale * 2.0f + vTargetDims.x;

	for (int i = 0; i < 8; i++)
	{
		LTVector vPos = vTargetPos + vDir * fDist;

		if (!IsPosTaken(vPos, vDims) && !IsPlayerInBox(vPos, vDims) && m_Leash.FindNode(vPos))
		{
			*pvPos = vPos;
			return LTTRUE;
		}

		LTVector vNewDir;
		RotateAroundY(vDir, vNewDir, MATH_PI / 4.0f);
		vDir = vNewDir;
	}

	return LTFALSE;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CAITentacle::IsPosTaken()
//
//	PURPOSE:	Whether a box overlaps a tentacle that is out
//
// ----------------------------------------------------------------------- //

// PH: 0x1014d0e0
LTBOOL CAITentacle::IsPosTaken(const LTVector& vPos, const LTVector& vDims)
{
	LTVector vMin = vPos - vDims;
	LTVector vMax = vPos + vDims;

	for (int i = 0; i < m_nNumTentacles; i++)
	{
		if (!m_apTentacles[i] || m_apTentacles[i]->IsHidden() == LTTRUE) continue;

		LTVector vTentPos, vTentDims;
		g_pLTServer->GetObjectPos(m_apTentacles[i]->m_hObject, &vTentPos);
		g_pLTServer->Physics()->GetObjectDims(m_apTentacles[i]->m_hObject, &vTentDims);

		vTentDims += LTVector(0.1f, 0.1f, 0.1f);

		LTVector vTentMin = vTentPos - vTentDims;
		LTVector vTentMax = vTentPos + vTentDims;

		if (vMin.x <= vTentMax.x && vMin.y <= vTentMax.y && vMin.z <= vTentMax.z &&
			vMax.x >= vTentMin.x && vMax.y >= vTentMin.y && vMax.z >= vTentMin.z)
		{
			return LTTRUE;
		}
	}

	return LTFALSE;
}

// PH: 0x1014d2b0
int CAITentacle::CountAttacking()
{
	int nCount = 0;

	for (int i = 0; i < m_nNumTentacles; i++)
	{
		int eState = m_apTentacles[i]->m_eState;
		if (eState == 4 || eState == 2 || eState == 3)
		{
			nCount++;
		}
	}

	return nCount;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CAITentacle::EruptTentacle()
//
//	PURPOSE:	Send a hidden tentacle up next to the target
//
// ----------------------------------------------------------------------- //

// PH: 0x1014d2e0
LTBOOL CAITentacle::EruptTentacle(LTBOOL bInFront)
{
	for (int i = 0; i < m_nNumTentacles; i++)
	{
		if (m_apTentacles[i]->m_eState == CTentacle::eHidden)
		{
			LTVector vPos;
			if (!FindEruptPos(&vPos, bInFront)) return LTFALSE;

			g_pLTServer->SetObjectPos(m_apTentacles[i]->m_hObject, &vPos);
			m_apTentacles[i]->Erupt(m_hTarget);
			return LTTRUE;
		}
	}

	return LTFALSE;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CAITentacle::ShakePlayer()
//
//	PURPOSE:	Shake the player's screen, less the farther away it is
//
// ----------------------------------------------------------------------- //

// PH: 0x1014d380
void CAITentacle::ShakePlayer(const LTVector& vPos, const LTVector& vShake, LTFLOAT fRadius,
							  LTFLOAT fDuration, LTFLOAT fMin, LTFLOAT fMax)
{
	CCharacterMgr::PlayerIterator iter = g_pCharacterMgr->BeginPlayers();
	if (iter == g_pCharacterMgr->EndPlayers()) return;

	CPlayerObj* pPlayer = *iter;
	if (!pPlayer || !pPlayer->m_hObject) return;

	LTVector vPlayerPos;
	g_pLTServer->GetObjectPos(pPlayer->m_hObject, &vPlayerPos);

	LTFLOAT fDist = (vPlayerPos - vPos).Mag();
	if (fDist >= fRadius) return;

	LTVector vAmount = vShake;
	vAmount *= 1.0f - fDist / fRadius;

	HMESSAGEWRITE hMessage = g_pLTServer->StartMessage(pPlayer->GetClient(), MID_SHAKE_SCREEN);
	g_pLTServer->WriteToMessageByte(hMessage, 0);
	g_pLTServer->WriteToMessageVector(hMessage, &vAmount);
	g_pLTServer->WriteToMessageFloat(hMessage, fDuration);
	g_pLTServer->WriteToMessageFloat(hMessage, fMin);
	g_pLTServer->WriteToMessageFloat(hMessage, fMax);
	g_pLTServer->EndMessage(hMessage);
}

// PH: 0x1014d4f0
LTBOOL CAITentacle::AllDead()
{
	for (int i = 0; i < m_nNumTentacles; i++)
	{
		if (m_apTentacles[i]->m_eState != CTentacle::eDead) return LTFALSE;
	}

	return LTTRUE;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CAITentacle::SetVisionMode()
//
//	PURPOSE:	Put the skins for the player's vision mode on every tentacle
//
// ----------------------------------------------------------------------- //

// PH: 0x1014d520
void CAITentacle::SetVisionMode(int nMode)
{
	if (nMode == m_nSoundSet) return;

	ObjectCreateStruct ocs;

	for (int i = 0; i < m_nNumTentacles; i++)
	{
		g_pLTServer->GetModelFilenames2(m_apTentacles[i]->m_hObject, &ocs);
		SetSkins(nMode, &ocs, s_aszSkins);
		g_pLTServer->Common()->SetObjectFilenames(m_apTentacles[i]->m_hObject, &ocs);
	}
}

#endif // PRIMAL_HUNT
