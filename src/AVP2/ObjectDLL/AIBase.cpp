// ----------------------------------------------------------------------- //
//
// MODULE  : AIBase.cpp
//
// PURPOSE : CAIBase - Third Law's lightweight AI for Primal Hunt
//
// PH: no source of this survives. Written from PHObject.lto, where its
// object is 0x10141bd0..0x10146370 (between WorldProperties.obj and the
// bull's object); functions are in retail order, with their addresses.
//
// ----------------------------------------------------------------------- //

#include "stdafx.h"

#ifdef PRIMAL_HUNT

#include "AIBase.h"
#include "ObjectMsgs.h"
#include "MsgIDs.h"
#include "ServerUtilities.h"
#include "CharacterMgr.h"
#include "PlayerObj.h"
#include "Character.h"
#include "CharacterFuncs.h"
#include "ServerSoundMgr.h"
#include "SurfaceFunctions.h"
#include "GameServerShell.h"
#include "Spawner.h"
#include "LTString.h"
#include "ParseUtils.h"
#include "ClientServerShared.h"
#include "Tokenizer.h"

// PH: 0x10316310, set by a cheat in CGameServerShell
extern LTBOOL g_bGodMode;

// PH: 0x102e321c, 0x102e3220: the head the aliens can take
static const char* s_szHeadPiece	= "Head";
static const char* s_szHeadSocket	= "Skull";

// PH: the axes, 0x10337130..0x103371e0 ($E initializers 0x10141bd0..0x10141d30)
LTVector g_vAIBaseZero(0.0f, 0.0f, 0.0f);
LTVector g_vAIBaseZero2(0.0f, 0.0f, 0.0f);
LTVector g_vAIBaseUp(0.0f, 1.0f, 0.0f);
LTVector g_vAIBaseDown(0.0f, -1.0f, 0.0f);
LTVector g_vAIBaseRight(1.0f, 0.0f, 0.0f);
LTVector g_vAIBaseUp2(0.0f, 1.0f, 0.0f);
LTVector g_vAIBaseForward(0.0f, 0.0f, 1.0f);
LTVector g_avAIBaseAxes[6] =
{
	LTVector(1.0f, 0.0f, 0.0f),
	LTVector(0.0f, 1.0f, 0.0f),
	LTVector(0.0f, 0.0f, 1.0f),
	LTVector(-1.0f, 0.0f, 0.0f),
	LTVector(0.0f, -1.0f, 0.0f),
	LTVector(0.0f, 0.0f, -1.0f),
};

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	GetPerpendicular (0x10141df0)
//
//	PURPOSE:	A horizontal vector perpendicular to vDir
//
// ----------------------------------------------------------------------- //

LTVector GetPerpendicular(const LTVector& vDir)
{
	if (vDir.y < -0.9999f)
		return LTVector(-1.0f, 0.0f, 0.0f);

	if (vDir.y > 0.9999f)
		return LTVector(1.0f, 0.0f, 0.0f);

	return LTVector(vDir.z, 0.0f, -vDir.x);
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	RotateAroundY (0x10141e60)
//
// ----------------------------------------------------------------------- //

void RotateAroundY(const LTVector& vIn, LTVector& vOut, LTFLOAT fAngle)
{
	LTFLOAT fSin = (LTFLOAT)sin(fAngle);
	vOut.y = vIn.y;
	LTFLOAT fCos = (LTFLOAT)cos(fAngle);
	vOut.x = fCos * vIn.x - fSin * vIn.z;
	vOut.z = fSin * vIn.x + fCos * vIn.z;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	TurnToward (0x10141ea0)
//
//	PURPOSE:	Turn vCur toward vTarget around vUp, at most fRate * fTime
//
// ----------------------------------------------------------------------- //

void TurnToward(const LTVector& vCur, const LTVector& vTarget, const LTVector& vUp,
				LTFLOAT fRate, LTFLOAT fTime, LTVector& vOut)
{
	LTFLOAT fDot = vCur.Dot(vTarget);

	if (fDot > 1.0f)
		fDot = 1.0f;
	else if (fDot < -1.0f)
		fDot = -1.0f;

	fRate *= fTime;

	if ((LTFLOAT)acos(fDot) <= fRate)
	{
		vOut = vTarget;
		return;
	}

	LTVector vRight;
	vRight = GetPerpendicular(vCur);
	vRight.Norm();

	if (vRight.Dot(vTarget) < 0.0f)
		fRate *= -1.0f;

	LTRotation rRot;
	g_pLTServer->AlignRotation(&rRot, (LTVector*)&vCur, (LTVector*)&vUp);
	g_pLTServer->RotateAroundAxis(&rRot, (LTVector*)&vUp, fRate);

	LTVector vR, vU;
	g_pLTServer->GetRotationVectors(&rRot, &vR, &vU, &vOut);
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	PlanesEqual (0x10142070)
//
// ----------------------------------------------------------------------- //

LTBOOL PlanesEqual(const LTPlane& a, const LTPlane& b, LTFLOAT fNormalTol, LTFLOAT fDistTol)
{
	if (fabs(b.m_Normal.x - a.m_Normal.x) < fNormalTol &&
		fabs(b.m_Normal.y - a.m_Normal.y) < fNormalTol &&
		fabs(b.m_Normal.z - a.m_Normal.z) < fNormalTol &&
		fabs(b.m_Dist - a.m_Dist) < fDistTol)
	{
		return LTTRUE;
	}

	return LTFALSE;
}

// ----------------------------------------------------------------------- //

BEGIN_CLASS(CAIBase)
	ADD_STRINGPROP(Leash_Group, "")
	ADD_VECTORPROP(Home_Leash_Point)
	ADD_REALPROP_FLAG_HELP(Footstep_Hearing_Range, 2048.0f, PF_RADIUS, "This is the max range from this AI at which it can \"hear\" the player's footsteps.")
	ADD_REALPROP_FLAG_HELP(Footstep_Sensitivity, 1.0f, 0, "How sensitive this AI is to footstep sounds. 1.0 is normal, 0.5 is half of normal, 2.0 is twice normal.")
	ADD_REALPROP_FLAG_HELP(Sight_Range, 2048.0f, PF_DIMS, "How far away this AI can sight an enemy.")
END_CLASS_DEFAULT(CAIBase, GameBase, NULL, NULL)

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	IsAIBase (0x10142390)
//
// ----------------------------------------------------------------------- //

LTBOOL IsAIBase(HOBJECT hObj)
{
	HCLASS hAIBase = g_pLTServer->GetClass("CAIBase");
	if (!hAIBase) return LTFALSE;

	HCLASS hClass = g_pLTServer->GetObjectClass(hObj);
	if (!hClass) return LTFALSE;

	return (g_pLTServer->IsKindOf(hClass, hAIBase) == LTTRUE);
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	GetAIBase (0x101423e0)
//
// ----------------------------------------------------------------------- //

CAIBase* GetAIBase(HOBJECT hObj)
{
	if (!IsAIBase(hObj)) return LTNULL;

	return (CAIBase*)g_pLTServer->HandleToObject(hObj);
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CAIBase::CAIBase (0x10142410)
//
// ----------------------------------------------------------------------- //

CAIBase::CAIBase(uint8 nType) : GameBase(nType)
{
	m_eState = eStateIdle;
	m_hTarget = LTNULL;

	m_bFirstUpdate		= LTFALSE;
	m_fMaxHitPoints		= 100.0f;
	m_fHitPoints		= 100.0f;
	m_szName[0]			= '\0';
	m_vLastPos.Init();

	m_Sounds.m_fInner	= 256.0f;
	m_Sounds.m_fOuter	= 2048.0f;
	m_nSoundSet			= 5;

	m_StuckCheck.m_fInterval	= 0.25f;
	m_StuckCheck.m_fMinDist		= 16.0f;

	m_fHeadDist		= 48.0f;
	m_szHeadPiece	= LTNULL;
	m_szHeadSocket	= LTNULL;

	m_fFootstepHearingRange	= 512.0f;
	m_fFootstepSensitivity	= 1.0f;

	m_Memory.SetRange(0, 1000.0f);
	m_Memory.SetDecay(0, 100.0f);
	m_Memory.SetRange(1, 1000.0f);
	m_Memory.SetDecay(1, 100.0f);
	m_Memory.SetRange(2, 1.0f);
	m_Memory.SetDecay(2, 0.0f);

	m_hPainAnim			= INVALID_ANI;
	m_fNextPainTime		= 0.0f;
	m_fPainDelayMin		= 0.5f;
	m_fPainDelayMax		= 2.0f;
	m_ePainState		= eStateIdle;
	m_nPainMoveType		= 1;
	m_bPainFacePos		= LTFALSE;
	m_hPainLastAnim		= (uint32)-1;
	m_bPainLastLoop		= 1;
	m_nPainChance		= 25;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CAIBase::IsDead (0x10142800)
//
// ----------------------------------------------------------------------- //

LTBOOL CAIBase::IsDead()
{
	return (m_eState == eStateDying || m_eState == eStateDead);
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CAIBase::~CAIBase (0x10142840)
//
// ----------------------------------------------------------------------- //

CAIBase::~CAIBase()
{
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CAIBase::OnPrecreate (0x10142910)
//
// ----------------------------------------------------------------------- //

uint32 CAIBase::OnPrecreate(void *pData, uint32 nInfo)
{
	GameBase::OnPrecreate(pData, nInfo);

	if (nInfo == PRECREATE_WORLDFILE)
	{
		char szGroup[32];
		g_pLTServer->GetPropString("Leash_Group", szGroup, 32);
		SAFE_STRCPY(m_Leash.m_szGroup, szGroup);

		g_pLTServer->GetPropReal("Footstep_Hearing_Range", &m_fFootstepHearingRange);
		g_pLTServer->GetPropReal("Footstep_Sensitivity", &m_fFootstepSensitivity);

		LTFLOAT fSightRange = 2048.0f;
		g_pLTServer->GetPropReal("Sight_Range", &fSightRange);
		m_Sight.SetRange(fSightRange);

		ObjectCreateStruct* pStruct = (ObjectCreateStruct*)pData;
		pStruct->m_Flags	= FLAG_VISIBLE | FLAG_ANIMTRANSITION | FLAG_RAYHIT | FLAG_SOLID | FLAG_TOUCH_NOTIFY | FLAG_GRAVITY | FLAG_MODELKEYS;
		pStruct->m_Flags2	= FLAG2_CYLINDERPHYSICS | FLAG2_SEMISOLID;

		LTVector vHome(0.0f, 0.0f, 0.0f);
		g_pLTServer->GetPropVector("Home_Leash_Point", &vHome);
		m_Leash.m_nHomeNode = -1;
		m_Leash.m_vHomePos = vHome;
	}

	return 0;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CAIBase::OnInitialUpdate (0x10142a40)
//
// ----------------------------------------------------------------------- //

uint32 CAIBase::OnInitialUpdate(uint32 nInfo)
{
	GameBase::OnInitialUpdate(nInfo);

	AIBaseList_Add(this);

	g_pLTServer->SetNetFlags(m_hObject, NETFLAG_POSUNGUARANTEED | NETFLAG_ROTUNGUARANTEED | NETFLAG_ANIMUNGUARANTEED);

	LTVector vScale;
	g_pLTServer->GetObjectScale(m_hObject, &vScale);

	LTVector vDims(32.0f, 48.0f, 32.0f);

	uint32 nType;
	g_pLTServer->Common()->GetObjectType(m_hObject, &nType);
	if (nType == OT_MODEL)
	{
		g_pLTServer->Common()->GetModelAnimUserDims(m_hObject, &vDims, 0);
		vDims.x *= vScale.x;
		vDims.y *= vScale.y;
		vDims.z *= vScale.z;
	}

	g_pLTServer->Physics()->SetObjectDims(m_hObject, &vDims, 0);
	g_pLTServer->SetForceIgnoreLimit(m_hObject, 0.0f);
	g_pLTServer->SetFrictionCoefficient(m_hObject, 0.0f);

	m_Animator.Init(m_hObject);

	m_bFirstUpdate		= LTTRUE;
	m_Sounds.m_hObject	= m_hObject;

	g_pLTServer->SetObjectUserFlags(m_hObject, SurfaceToUserFlag(ST_FLESH_HUMAN));

	m_hPainAnim		= FindPainAnim();
	m_szHeadPiece	= s_szHeadPiece;
	m_szHeadSocket	= s_szHeadSocket;

	if (nInfo != INITIALUPDATE_SAVEGAME)
	{
		AIBaseStateInfo info(eStateIdle);
		SetState(&info);

		g_pLTServer->SetBlockingPriority(m_hObject, 0);

		m_fMaxHitPoints *= GetDifficultyHealthFactor();
		m_fHitPoints *= GetDifficultyHealthFactor();

		SetNextUpdate(0.001f);
	}

	return 1;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CAIBase::OnUpdate (0x10142c60)
//
// ----------------------------------------------------------------------- //

uint32 CAIBase::OnUpdate(LTFLOAT fFrameTime)
{
	LTBOOL bChecked = LTFALSE;

	if (m_bFirstUpdate == LTTRUE)
	{
		if (!m_Leash.IsValid())
		{
			char szName[64];
			g_pLTServer->GetObjectName(m_hObject, szName, 63);
			g_pLTServer->CPrint("%s could not find its leash system!", szName);
		}

		m_bFirstUpdate = LTFALSE;

		LTVector vHome = m_Leash.m_vHomePos;
		if (ABS(vHome.x) <= 0.0f && ABS(vHome.y) <= 0.0f && ABS(vHome.z) <= 0.0f)
		{
			m_Leash.m_nHomeNode = -1;
		}
		else
		{
			CLeashNode* pNode = m_Leash.FindNode(m_Leash.m_vHomePos);
			if (pNode)
				m_Leash.m_nHomeNode = pNode->m_nID;
			else
				m_Leash.m_nHomeNode = -1;
		}
	}

	m_Memory.Update(fFrameTime);

	LTVector vPos;
	g_pLTServer->GetObjectPos(m_hObject, &vPos);

	// Are we stuck?

	LTBOOL bMoving = m_MoveMgr.m_bMoving;
	LTFLOAT fTime = g_pLTServer->GetTime();

	if (m_StuckCheck.m_bEnabled)
	{
		if (fTime - m_StuckCheck.m_fTime >= m_StuckCheck.m_fInterval)
		{
			if (!bMoving)
			{
				m_StuckCheck.m_vPos = vPos;
				m_StuckCheck.m_fTime = fTime;
				m_StuckCheck.m_bStuck = bChecked;
			}
			else
			{
				LTVector vDiff = m_StuckCheck.m_vPos - vPos;
				LTFLOAT x = (m_StuckCheck.m_nAxes & 1) ? vDiff.x : 0.0f;
				if (!(m_StuckCheck.m_nAxes & 2)) vDiff.y = 0.0f;
				LTFLOAT z = (m_StuckCheck.m_nAxes & 4) ? vDiff.z : 0.0f;

				if ((LTFLOAT)sqrt(x*x + vDiff.y*vDiff.y + z*z) > m_StuckCheck.m_fMinDist)
				{
					m_StuckCheck.m_bStuck = LTFALSE;
					bChecked = LTTRUE;
					m_StuckCheck.m_vPos = vPos;
					m_StuckCheck.m_fTime = fTime;
				}
				else
				{
					bChecked = LTTRUE;
					m_StuckCheck.m_bStuck = bChecked;
				}
			}
		}
	}
	else
	{
		m_StuckCheck.m_vPos = vPos;
		m_StuckCheck.m_fTime = fTime;
		m_StuckCheck.m_bStuck = bChecked;
	}

	LTBOOL bStuck = LTFALSE;
	if (m_StuckCheck.m_bEnabled)
	{
		bStuck = m_StuckCheck.m_bStuck;
		if (bStuck == LTTRUE && bChecked == bStuck)
		{
			m_StuckCheck.m_bStuck = LTFALSE;
			m_StuckCheck.m_fTime = g_pLTServer->GetTime();
			m_StuckCheck.m_vPos = vPos;

			LTVector vDir = m_MoveMgr.m_vDest - vPos;
			vDir.Norm();

			if (m_Avoid.m_nState == 0)
			{
				m_Avoid.m_vDir = vDir;
				m_Avoid.m_vDir.y = 0.0f;
				m_Avoid.m_vDir.Norm();
				m_Avoid.m_nState = m_Avoid.m_nNextState;
				m_Avoid.Update();
			}
			else if (++m_Avoid.m_nState == 4)
			{
				m_Avoid.Reset();

				if (m_Avoid.m_nState == 0)
				{
					m_Avoid.m_vDir = m_Avoid.m_vDir;
					m_Avoid.m_vDir.y = 0.0f;
					m_Avoid.m_vDir.Norm();
					m_Avoid.m_nState = m_Avoid.m_nNextState;
					m_Avoid.Update();
				}
				else if (++m_Avoid.m_nState != 4)
				{
					m_Avoid.Update();
				}
			}
			else
			{
				m_Avoid.Update();
			}
		}
	}

	m_OrientMgr.Update(m_hObject, fFrameTime);

	if (m_Avoid.CheckBlocked(m_hObject, &m_MoveMgr) == LTTRUE)
	{
		if ((!bStuck && bChecked == LTTRUE) || !m_StuckCheck.m_bEnabled)
			m_Avoid.Reset();
	}
	else
	{
		m_MoveMgr.Update(m_hObject, fFrameTime);
	}

	m_Leash.Update(vPos);

	if (m_Leash.m_bOutside == LTTRUE)
	{
		if (!m_MoveMgr.m_bMoving)
		{
			m_Leash.m_bOutside = LTFALSE;
		}
		else
		{
			g_pLTServer->SetObjectPos(m_hObject, &m_vLastPos);
			vPos = m_vLastPos;
			m_Leash.Update(vPos);
			m_MoveMgr.m_eMoveType = 0;
			m_MoveMgr.Update(m_hObject, fFrameTime);
		}
	}
	else if (m_MoveMgr.m_bMoving == LTTRUE)
	{
		m_vLastPos = vPos;
	}

	UpdateState(fFrameTime);
	SetNextUpdate(0.001f);

	return 1;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CAIBase::OnLoad (0x101431e0)
//
// ----------------------------------------------------------------------- //

void CAIBase::OnLoad(HMESSAGEREAD hRead)
{
	GameBase::OnLoad(hRead);

	m_Sight.Load(hRead);

	m_eState = hRead->ReadByte();
	*hRead >> m_hTarget;

	m_Animator.Load(hRead);
	m_MoveMgr.Load(hRead);
	m_OrientMgr.Load(hRead);

	m_fMaxHitPoints = hRead->ReadFloat();
	m_fHitPoints = hRead->ReadFloat();

	m_Leash.Load(hRead);

	m_Move.m_eMode = hRead->ReadByte();
	*hRead >> m_Move.m_vTarget;
	*hRead >> m_Move.m_vPos;
	m_Move.m_nFrom = hRead->ReadDWord();
	m_Move.m_nNode = hRead->ReadDWord();
	m_Move.m_nTo = hRead->ReadDWord();
	m_Move.m_nLastNode = hRead->ReadDWord();
	m_MoveDist.m_fMin = hRead->ReadFloat();
	m_MoveDist.m_fMax = hRead->ReadFloat();
	m_MoveDist.m_fDist = hRead->ReadFloat();
	m_MoveDist.m_nAxes = hRead->ReadDWord();

	m_NextState.eState = hRead->ReadByte();
	*hRead >> m_NextState.vPos;
	m_NextState.nParam = hRead->ReadByte();
	m_MoveState.eState = hRead->ReadByte();
	*hRead >> m_MoveState.vPos;
	m_MoveState.nParam = hRead->ReadByte();

	hRead->ReadStringFL(m_szName, sizeof(m_szName));
	*hRead >> m_vLastPos;

	m_nSoundSet = hRead->ReadByte();

	m_StuckCheck.Load(hRead);

	// RETAIL BUG: the avoidance state is written into the load message
	// instead of being read from it (OnSave's lines, pasted and never
	// turned around): the 51 bytes OnSave wrote for it are never read, so
	// m_fHeadDist and everything after it load from the wrong place
	hRead->WriteByte(m_Avoid.m_nState);
	hRead->WriteVector(m_Avoid.m_vDir);
	hRead->WriteVector(m_Avoid.m_vRight);
	hRead->WriteVector(m_Avoid.m_vPos);
	hRead->WriteFloat(m_Avoid.m_fTime);
	hRead->WriteByte(m_Avoid.m_nNextState);
	hRead->WriteByte(m_Avoid.m_bActive);
	hRead->WriteFloat(m_Avoid.m_fCheckDist);
	hRead->WriteFloat(m_Avoid.m_fAvoidDist);
	m_fHeadDist = hRead->ReadFloat();

	m_Memory.Load(hRead);

	m_fFootstepHearingRange = hRead->ReadFloat();
	m_fFootstepSensitivity = hRead->ReadFloat();
	m_hPainAnim = hRead->ReadDWord();
	m_fNextPainTime = hRead->ReadFloat();
	m_fPainDelayMin = hRead->ReadFloat();
	m_fPainDelayMax = hRead->ReadFloat();
	m_ePainState = hRead->ReadByte();
	m_nPainMoveType = hRead->ReadByte();
	m_bPainFacePos = hRead->ReadByte();
	m_hPainLastAnim = hRead->ReadDWord();
	m_bPainLastLoop = hRead->ReadByte();
	m_nPainChance = hRead->ReadDWord();

	m_Spears.Load(hRead);

	m_bFirstUpdate = LTTRUE;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CAIBase::OnSave (0x101436c0)
//
// ----------------------------------------------------------------------- //

void CAIBase::OnSave(HMESSAGEWRITE hWrite)
{
	GameBase::OnSave(hWrite);

	m_Sight.Save(hWrite);

	hWrite->WriteByte(m_eState);
	hWrite->WriteObject(m_hTarget);

	m_Animator.Save(hWrite);
	m_MoveMgr.Save(hWrite);
	m_OrientMgr.Save(hWrite);

	*hWrite << m_fMaxHitPoints;
	*hWrite << m_fHitPoints;

	m_Leash.Save(hWrite);

	hWrite->WriteByte(m_Move.m_eMode);
	*hWrite << m_Move.m_vTarget;
	*hWrite << m_Move.m_vPos;
	hWrite->WriteDWord(m_Move.m_nFrom);
	hWrite->WriteDWord(m_Move.m_nNode);
	hWrite->WriteDWord(m_Move.m_nTo);
	hWrite->WriteDWord(m_Move.m_nLastNode);
	*hWrite << m_MoveDist.m_fMin;
	*hWrite << m_MoveDist.m_fMax;
	*hWrite << m_MoveDist.m_fDist;
	hWrite->WriteDWord(m_MoveDist.m_nAxes);

	hWrite->WriteByte(m_NextState.eState);
	*hWrite << m_NextState.vPos;
	hWrite->WriteByte(m_NextState.nParam);
	hWrite->WriteByte(m_MoveState.eState);
	*hWrite << m_MoveState.vPos;
	hWrite->WriteByte(m_MoveState.nParam);

	hWrite->WriteString(m_szName);
	*hWrite << m_vLastPos;

	hWrite->WriteByte(m_nSoundSet);

	m_StuckCheck.Save(hWrite);

	hWrite->WriteByte(m_Avoid.m_nState);
	*hWrite << m_Avoid.m_vDir;
	*hWrite << m_Avoid.m_vRight;
	*hWrite << m_Avoid.m_vPos;
	*hWrite << m_Avoid.m_fTime;
	hWrite->WriteByte(m_Avoid.m_nNextState);
	hWrite->WriteByte(m_Avoid.m_bActive);
	*hWrite << m_Avoid.m_fCheckDist;
	*hWrite << m_Avoid.m_fAvoidDist;
	*hWrite << m_fHeadDist;

	m_Memory.Save(hWrite);

	*hWrite << m_fFootstepHearingRange;
	*hWrite << m_fFootstepSensitivity;
	hWrite->WriteDWord(m_hPainAnim);
	*hWrite << m_fNextPainTime;
	*hWrite << m_fPainDelayMin;
	*hWrite << m_fPainDelayMax;
	hWrite->WriteByte(m_ePainState);
	hWrite->WriteByte(m_nPainMoveType);
	hWrite->WriteByte(m_bPainFacePos);
	hWrite->WriteDWord(m_hPainLastAnim);
	hWrite->WriteByte(m_bPainLastLoop);
	hWrite->WriteDWord(m_nPainChance);

	m_Spears.Save(hWrite);
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CAIBase::ObjectMessageFn (0x10143a10)
//
// ----------------------------------------------------------------------- //

uint32 CAIBase::ObjectMessageFn(HOBJECT hSender, uint32 messageID, HMESSAGEREAD hRead)
{
	switch (messageID)
	{
		case MID_DAMAGE:
		{
			DamageStruct damage;
			damage.InitFromMessage(hRead);
			HandleDamage(damage);
		}
		break;

		case MID_TRIGGER:
		{
			LTString hMsg = g_pLTServer->ReadFromMessageHString(hRead);
			const char* szMsg = hMsg ? g_pLTServer->GetStringData(hMsg) : LTNULL;
			HandleTrigger(szMsg);
		}
		break;
	}

	return GameBase::ObjectMessageFn(hSender, messageID, hRead);
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CAIBase::CheckForPlayer (0x10143b70)
//
// ----------------------------------------------------------------------- //

LTBOOL CAIBase::CheckForPlayer()
{
	if (g_pLTServer->GetTime() < m_Sight.m_fNextUpdate)
		return LTFALSE;

	CCharacterMgr::PlayerIterator iter = g_pCharacterMgr->BeginPlayers();
	if (iter == g_pCharacterMgr->EndPlayers())
		return LTFALSE;

	CPlayerObj* pPlayer = *iter;
	if (!pPlayer || !pPlayer->m_hObject || pPlayer->IsDead() == LTTRUE)
		return LTFALSE;

	LTVector vPos;
	g_pLTServer->GetObjectPos(m_hObject, &vPos);

	LTFLOAT fDist;
	if (!m_Sight.CanSee(pPlayer->m_hObject, vPos, m_OrientMgr.GetForward(), 3, 0, 0, 0, &fDist))
		return LTFALSE;

	if (m_Memory.Add(pPlayer->m_hObject) == LTTRUE)
	{
		m_Memory.AddStimulus(pPlayer->m_hObject, 0, fDist * 500.0f);
	}

	return LTTRUE;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CAIBase::ChooseTarget (0x10143c70)
//
// ----------------------------------------------------------------------- //

LTBOOL CAIBase::ChooseTarget()
{
	HOBJECT hTarget = m_Memory.GetBest(0);
	if (!hTarget)
	{
		hTarget = m_Memory.GetBest(1);
		if (hTarget)
		{
			LTVector vPos;
			g_pLTServer->GetObjectPos(hTarget, &vPos);

			CLeashNode* pNode = m_Leash.FindNode(vPos);
			if (pNode && pNode->m_nID == m_Leash.m_nCurNode)
				goto found;
		}

		hTarget = m_Memory.GetBest(2);
		if (!hTarget)
			return LTFALSE;
	}

found:
	if (IsBodyProp(hTarget) == LTTRUE)
	{
		CCharacter* pChar = (CCharacter*)g_pLTServer->HandleToObject(hTarget);
		if (!pChar || pChar->IsDead() == LTTRUE)
		{
			m_Memory.Forget(hTarget);
			return LTFALSE;
		}
	}

	SetTarget(hTarget);
	m_Sight.SetFOV(360.0f);

	return LTTRUE;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CAIBase::SetTarget (0x10143d50)
//
// ----------------------------------------------------------------------- //

void CAIBase::SetTarget(HOBJECT hTarget)
{
	if (m_hTarget == hTarget) return;

	if (m_hTarget)
	{
		OnTargetChanged(hTarget, m_hTarget);
	}

	m_hTarget = hTarget;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CAIBase::OnTargetChanged (0x10143df0)
//
// ----------------------------------------------------------------------- //

void CAIBase::OnTargetChanged(HOBJECT hNew, HOBJECT hOld)
{
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CAIBase::UpdateState (0x10143e00)
//
// ----------------------------------------------------------------------- //

void CAIBase::UpdateState(LTFLOAT fFrameTime)
{
	switch (m_eState)
	{
		case eStateIdle:	UpdateIdle(fFrameTime);		break;
		case eStateAlert:	UpdateAlert(fFrameTime);	break;
		case eStateDying:	UpdateDying(fFrameTime);	break;
		case eStateDead:	UpdateDead(fFrameTime);		break;
		case eStateMoving:	UpdateMoving(fFrameTime);	break;
		case eStatePain:	UpdatePain(fFrameTime);		break;
	}
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CAIBase::UpdateDying (0x10143e80)
//
// ----------------------------------------------------------------------- //

void CAIBase::UpdateDying(LTFLOAT fFrameTime)
{
	if (m_Animator.GetPlaybackState(0) & MS_PLAYDONE)
	{
		AIBaseStateInfo info(eStateDead, eStateDead);
		SetState(&info);
	}
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CAIBase::UpdatePain (0x10143ef0)
//
// ----------------------------------------------------------------------- //

void CAIBase::UpdatePain(LTFLOAT fFrameTime)
{
	m_StuckCheck.m_bEnabled = LTFALSE;

	if (m_Animator.GetPlaybackState(0) & MS_PLAYDONE)
	{
		m_MoveMgr.SetMoveType(m_nPainMoveType);
		m_OrientMgr.SetFacePos(m_bPainFacePos);
		m_eState = m_ePainState;

		LTFLOAT fTime = g_pLTServer->GetTime();
		LTFLOAT fMin = m_fPainDelayMin;
		LTFLOAT fMax = m_fPainDelayMax;
		m_fNextPainTime = ((LTFLOAT)rand() / RAND_MAX) * (fMax - fMin) + fMin + fTime;

		m_Animator.PlayAnim(m_hPainLastAnim, m_bPainLastLoop, LTFALSE);
	}
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CAIBase::SetStateIdle (0x10143fb0)
//
// ----------------------------------------------------------------------- //

LTBOOL CAIBase::SetStateIdle(const AIBaseStateInfo* pInfo)
{
	m_eState = eStateIdle;
	m_StuckCheck.m_bEnabled = LTFALSE;
	return LTTRUE;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CAIBase::SetStateAlert (0x10143fd0)
//
// ----------------------------------------------------------------------- //

LTBOOL CAIBase::SetStateAlert(const AIBaseStateInfo* pInfo)
{
	m_eState = eStateAlert;
	m_StuckCheck.m_bEnabled = LTTRUE;
	return LTTRUE;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CAIBase::SetStateDying (0x10143ff0)
//
// ----------------------------------------------------------------------- //

LTBOOL CAIBase::SetStateDying(const AIBaseStateInfo* pInfo)
{
	m_eState = eStateDying;
	m_OrientMgr.SetFacePos(LTFALSE);
	m_MoveMgr.SetMoveType(1);
	m_Animator.PlayAnim("Death0", LTFALSE, LTFALSE);
	m_StuckCheck.m_bEnabled = LTFALSE;
	return LTTRUE;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CAIBase::SetStateDead (0x10144040)
//
// ----------------------------------------------------------------------- //

LTBOOL CAIBase::SetStateDead(const AIBaseStateInfo* pInfo)
{
	m_eState = eStateDead;
	m_OrientMgr.m_bFacePos = LTFALSE;
	m_MoveMgr.m_eMoveType = 1;

	uint32 dwFlags;
	g_pLTServer->Common()->GetObjectFlags(m_hObject, OFT_Flags, dwFlags);
	g_pLTServer->Common()->SetObjectFlags(m_hObject, OFT_Flags, dwFlags & ~(FLAG_SOLID | FLAG_GRAVITY));

	m_StuckCheck.m_bEnabled = LTFALSE;
	return LTTRUE;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CAIBase::SetStatePain (0x101440b0)
//
// ----------------------------------------------------------------------- //

LTBOOL CAIBase::SetStatePain(const AIBaseStateInfo* pInfo)
{
	if (m_hPainAnim == INVALID_ANI) return LTFALSE;
	if (m_eState == eStateDying || m_eState == eStateDead || m_eState == eStatePain) return LTFALSE;
	if (g_pLTServer->GetTime() < m_fNextPainTime) return LTFALSE;
	if (rand() % 100 >= m_nPainChance) return LTFALSE;

	m_ePainState = m_eState;
	m_eState = eStatePain;

	m_nPainMoveType = m_MoveMgr.GetMoveType();
	m_bPainLastLoop = m_Animator.IsLooping(0);
	m_bPainFacePos = m_OrientMgr.GetFacePos();
	m_hPainLastAnim = m_Animator.GetAnim(0);

	m_MoveMgr.SetMoveType(1);
	m_OrientMgr.SetFacePos(LTFALSE);

	m_Animator.PlayAnim(m_hPainAnim, LTFALSE, LTFALSE);
	m_fNextPainTime = m_Animator.GetAnimLength(0) + g_pLTServer->GetTime();

	return LTTRUE;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CAIBase::SetState (0x101441d0)
//
// ----------------------------------------------------------------------- //

LTBOOL CAIBase::SetState(const AIBaseStateInfo* pInfo)
{
	switch (pInfo->eState)
	{
		case eStateIdle:	return SetStateIdle(pInfo);
		case eStateAlert:	return SetStateAlert(pInfo);
		case eStateDying:	return SetStateDying(pInfo);
		case eStateDead:	return SetStateDead(pInfo);
		case eStateMoving:	return SetStateMoving(pInfo);
		case eStatePain:	return SetStatePain(pInfo);
	}

	return LTFALSE;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CAIBase::HandleDamage (0x10144250)
//
// ----------------------------------------------------------------------- //

void CAIBase::HandleDamage(const DamageStruct& damage)
{
	if (m_eState == eStateDying) return;

	if (m_eState == eStateDead)
	{
		if (IsHeadShot(damage) != LTTRUE) return;

		HideHead();

		g_pServerSoundMgr->PlaySoundFromObject(damage.hDamager, "pred_headgib");

		{
		CPlayerObj* pPlayer = (CPlayerObj*)g_pServerDE->HandleToObject(damage.hDamager);

		if(pPlayer)
		{
			HMESSAGEWRITE hMsg = g_pInterface->StartMessage(pPlayer->GetClient(), MID_ADD_SKULL);
			g_pInterface->EndMessage(hMsg);
		}
		}

		{
		ObjArray<HOBJECT,1> objArray;
		if( LT_OK == g_pServerDE->FindNamedObjects( "TrophyCounter", objArray ) )
		{
			if( objArray.NumObjects() > 0 )
			{
				SendTriggerMsgToObject(this, objArray.GetObject(0), "INC");
			}
		}
		}
		return;
	}

	m_fHitPoints -= damage.fDamage;

	if (m_fHitPoints <= 0.0f)
	{
		AIBaseStateInfo info(eStateDying, eStateDying);
		SetState(&info);
		return;
	}

	AIBaseStateInfo info(eStatePain, m_eState);
	SetState(&info);
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CAIBase::CanReact (0x10144430)
//
// ----------------------------------------------------------------------- //

LTBOOL CAIBase::CanReact(const DamageStruct& damage)
{
	return LTTRUE;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CAIBase::DamageObject (0x10144440)
//
// ----------------------------------------------------------------------- //

void CAIBase::DamageObject(HOBJECT hObj, LTVector* pvDir, LTFLOAT fDamage, LTFLOAT fDuration, uint8 eType)
{
	LTFLOAT fFactor = GetDifficultyDamageFactor();

	HMESSAGEWRITE hMsg = g_pLTServer->StartMessageToObject(this, hObj, MID_DAMAGE);
	g_pLTServer->WriteToMessageVector(hMsg, pvDir);
	g_pLTServer->WriteToMessageFloat(hMsg, fDuration);
	g_pLTServer->WriteToMessageFloat(hMsg, fFactor * fDamage);
	g_pLTServer->WriteToMessageByte(hMsg, eType);
	g_pLTServer->WriteToMessageObject(hMsg, m_hObject);
	g_pLTServer->WriteToMessageObject(hMsg, LTNULL);
	g_pLTServer->EndMessage(hMsg);
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CAIBase::OnModelStringKey (0x101444f0)
//
// ----------------------------------------------------------------------- //

uint32 CAIBase::OnModelStringKey(void *pData)
{
	ArgList* pArgList = (ArgList*)pData;
	if (!pArgList->argc) return 1;

	if (m_eState == eStateDead) return 1;

	if (stricmp(pArgList->argv[0], "SOUND") == 0)
	{
		LTVector vPos;
		g_pLTServer->GetObjectPos(m_hObject, &vPos);
		m_Sounds.PlaySound(pArgList->argv[1], vPos, 100, 0, 128, 256.0f, 3072.0f, 58);
	}

	return 0;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CAIBase::SetSkins (0x10144590)
//
// ----------------------------------------------------------------------- //

void CAIBase::SetSkins(int nSet, ObjectCreateStruct* pStruct, const char* aszSkins[][4])
{
	for (int i = 0; i < 4; i++)
	{
		if (!aszSkins[nSet][i])
		{
			m_nSoundSet = nSet;
			return;
		}

		SAFE_STRCPY(pStruct->m_SkinNames[i], aszSkins[nSet][i]);
	}

	m_nSoundSet = nSet;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CAIBase::HandleFootstep (0x10144600)
//
// ----------------------------------------------------------------------- //

void CAIBase::HandleFootstep(HOBJECT hObj, LTBOOL bLoud, LTFLOAT fVolume)
{
	if (IsPlayer(hObj) && m_Memory.Add(hObj))
	{
		m_Memory.AddStimulus(hObj, bLoud, fVolume);
	}
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CAIBase::HandleSound (0x10144640)
//
// ----------------------------------------------------------------------- //

void CAIBase::HandleSound(HOBJECT hObj, const LTVector& vPos, LTFLOAT fVolume, uint32 nSurfaceType, LTFLOAT fTime)
{
	if (!IsPlayer(hObj)) return;

	LTVector vMyPos;
	g_pLTServer->GetObjectPos(m_hObject, &vMyPos);

	LTFLOAT fDist = (vMyPos - vPos).Mag();
	if (fDist > m_fFootstepHearingRange)
		fDist = m_fFootstepHearingRange;

	LTFLOAT fAmount = (1.0f - fDist / m_fFootstepHearingRange) * fVolume * 1000.0f;
	fAmount *= m_fFootstepSensitivity;

	if (IsAlien(hObj) == LTTRUE)
	{
		CCharacter* pChar = (CCharacter*)g_pLTServer->HandleToObject(hObj);
		if (pChar && pChar->GetCloakState())
			fAmount *= 0.25f;
	}

	if (fAmount > 0.0f)
	{
		HandleFootstep(hObj, LTTRUE, fAmount);
	}
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CAIBase::IsPlayerInBox (0x10144810)
//
// ----------------------------------------------------------------------- //

LTBOOL CAIBase::IsPlayerInBox(const LTVector& vBoxPos, const LTVector& vBoxDims)
{
	CCharacterMgr::PlayerIterator iter = g_pCharacterMgr->BeginPlayers();
	if (iter == g_pCharacterMgr->EndPlayers()) return LTFALSE;

	CPlayerObj* pPlayer = *iter;
	if (!pPlayer || !pPlayer->m_hObject) return LTFALSE;

	LTVector vPos, vDims;
	g_pLTServer->GetObjectPos(pPlayer->m_hObject, &vPos);
	g_pLTServer->Physics()->GetObjectDims(pPlayer->m_hObject, &vDims);

	vDims += LTVector(0.1f, 0.1f, 0.1f);

	LTVector vPlayerMin = vPos - vDims;
	LTVector vPlayerMax = vPos + vDims;

	LTVector vBoxMin = vBoxPos - vBoxDims;
	LTVector vBoxMax = vBoxPos + vBoxDims;

	if (vBoxMin.x <= vPlayerMax.x && vBoxMin.y <= vPlayerMax.y && vBoxMin.z <= vPlayerMax.z &&
		vBoxMax.x >= vPlayerMin.x && vBoxMax.y >= vPlayerMin.y && vBoxMax.z >= vPlayerMin.z)
	{
		return LTTRUE;
	}

	return LTFALSE;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CAIBase::CanShootThrough (0x101449b0)
//
// ----------------------------------------------------------------------- //

LTBOOL CAIBase::CanShootThrough()
{
	return (m_eState == eStateDead);
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CAIBase::FindPainAnim (0x101449c0)
//
// ----------------------------------------------------------------------- //

HMODELANIM CAIBase::FindPainAnim()
{
	HMODELANIM hAnim = g_pLTServer->GetAnimIndex(m_hObject, "Pain");
	if (hAnim != INVALID_ANI) return hAnim;

	hAnim = g_pLTServer->GetAnimIndex(m_hObject, "Pain0");
	if (hAnim != INVALID_ANI) return hAnim;

	hAnim = g_pLTServer->GetAnimIndex(m_hObject, "Pain1");
	if (hAnim != INVALID_ANI) return hAnim;

	return INVALID_ANI;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CAIBase::IsHeadShot (0x10144a20)
//
// ----------------------------------------------------------------------- //

LTBOOL CAIBase::IsHeadShot(const DamageStruct& damage)
{
	if (IsAlien(damage.hDamager))
	{
		if (damage.eType == DT_SLICE || damage.eType == DT_HIGHCALIBER)
		{
			HMODELPIECE hPiece = (HMODELPIECE)-1;
			g_pLTServer->GetModelLT()->GetPiece(m_hObject, (char*)m_szHeadPiece, hPiece);
			if (hPiece != (HMODELPIECE)-1)
			{
				LTBOOL bHidden;
				g_pLTServer->GetModelLT()->GetPieceHideStatus(m_hObject, hPiece, bHidden);
				if (bHidden != LTTRUE)
				{
					HMODELSOCKET hSocket = (HMODELSOCKET)-1;
					g_pLTServer->GetModelLT()->GetSocket(m_hObject, (char*)m_szHeadSocket, hSocket);
					if (hSocket != (HMODELSOCKET)-1)
					{
						LTransform tf;
						g_pLTServer->GetModelLT()->GetSocketTransform(m_hObject, hSocket, tf, LTTRUE);

						if ((tf.m_Pos - damage.vPos).Mag() <= m_fHeadDist)
							return LTTRUE;
					}
				}
			}
		}
	}

	return LTFALSE;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CAIBase::HideHead (0x10144b70)
//
// ----------------------------------------------------------------------- //

void CAIBase::HideHead()
{
	HMODELPIECE hPiece = (HMODELPIECE)-1;
	g_pLTServer->GetModelLT()->GetPiece(m_hObject, (char*)m_szHeadPiece, hPiece);
	if (hPiece != (HMODELPIECE)-1)
	{
		g_pLTServer->GetModelLT()->SetPieceHideStatus(m_hObject, hPiece, LTTRUE);
	}
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CAIBase::IsTargetGone (0x10144bc0)
//
// ----------------------------------------------------------------------- //

LTBOOL CAIBase::IsTargetGone()
{
	if (!m_hTarget) return LTTRUE;

	if (IsBodyProp(m_hTarget) == LTTRUE)
	{
		CCharacter* pChar = (CCharacter*)g_pLTServer->HandleToObject(m_hTarget);
		if (pChar && pChar->IsDead() == LTTRUE)
		{
			m_hTarget = LTNULL;
		}
	}
	else if (IsAIBaseOrTentacle(m_hTarget) == LTTRUE)
	{
		CAIBase* pAI = GetAIBase(m_hTarget);
		if (pAI && pAI->IsDead() == LTTRUE)
		{
			m_hTarget = LTNULL;
		}
	}

	if (!m_hTarget) return LTTRUE;

	return LTFALSE;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CAIBase::FindClosestNode (0x10144ce0)
//
// ----------------------------------------------------------------------- //

int CAIBase::FindClosestNode(HOBJECT hObj, const LTVector& vPos)
{
	int nClosest = -1;
	HMODELNODE hNode = INVALID_MODEL_NODE;
	LTFLOAT fClosest = 9999999.0f;

	while (g_pLTServer->GetNextModelNode(m_hObject, hNode, &hNode) == LT_OK)
	{
		LTransform tf;
		g_pLTServer->GetModelLT()->GetNodeTransform(m_hObject, hNode, tf, LTTRUE);

		LTFLOAT fDist = (tf.m_Pos - vPos).Mag();
		if (fDist < fClosest)
		{
			fClosest = fDist;
			nClosest = hNode;
		}
	}

	return nClosest;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CAIBase::AddSpear (0x10144dd0)
//
// ----------------------------------------------------------------------- //

LTBOOL CAIBase::AddSpear(HOBJECT hSpear, const LTVector& vPos, const LTVector& vDir, const LTVector& vScale)
{
	CCharacterMgr::PlayerIterator iter = g_pCharacterMgr->BeginPlayers();
	if (iter == g_pCharacterMgr->EndPlayers()) return LTFALSE;

	CPlayerObj* pPlayer = *iter;
	if (!pPlayer || !pPlayer->m_hObject) return LTFALSE;

	// Did the spear really hit us?

	LTVector vEye = pPlayer->GetHeadOffset() + pPlayer->GetPosition();

	ILTModel::LTRayResult ray;
	ray.m_vOrigin	= vPos;
	ray.m_vDir		= vDir;
	ray.m_fMinDist	= 0.1f;
	ray.m_fMaxDist	= 128.0f;

	g_pLTServer->GetModelLT()->IntersectRays(m_hObject, vEye, 0, LTNULL, 0, &ray, 1);
	if (!ray.m_bIntersect) return LTFALSE;

	LTRotation rRot;
	g_pLTServer->GetObjectRotation(hSpear, &rRot);

	LTVector vSpearPos = vPos - vDir * (vScale.z * 0.5f);

	char szSpawn[255];
	sprintf(szSpawn, "PickupObject Pickup Ammo_Spears_Stuck_Character");

	LTVector vMyPos;
	g_pLTServer->GetObjectPos(m_hObject, &vMyPos);

	BaseClass* pClass = SpawnObject(szSpawn, vMyPos, rRot);
	if (!pClass) return LTFALSE;

	LTVector vSpearScale = vScale;
	g_pLTServer->ScaleObject(pClass->m_hObject, &vSpearScale);

	LTVector vDims;
	g_pLTServer->Physics()->GetObjectDims(pClass->m_hObject, &vDims);
	vDims.x *= vScale.x;
	vDims.y *= vScale.y;
	vDims.z *= vScale.z;
	g_pLTServer->Physics()->SetObjectDims(pClass->m_hObject, &vDims, 0);

	uint32 dwFlags = g_pLTServer->GetObjectFlags(pClass->m_hObject);
	dwFlags |= FLAG_SOLID;
	dwFlags &= ~FLAG_REMOVEIFOUTSIDE;
	g_pLTServer->SetObjectFlags(pClass->m_hObject, dwFlags);

	g_pLTServer->SetVelocity(pClass->m_hObject, &LTVector(0, 0, 0));

	int nNode = FindClosestNode(m_hObject, vSpearPos);
	if (nNode == -1)
	{
		g_pLTServer->RemoveObject(pClass->m_hObject);
		return LTFALSE;
	}

	LTransform tf;
	g_pLTServer->GetModelLT()->GetNodeTransform(m_hObject, nNode, tf, LTTRUE);

	LTVector vNodePos;
	LTRotation rNode;
	g_pLTServer->GetTransformLT()->GetPos(tf, vNodePos);
	g_pLTServer->GetTransformLT()->GetRot(tf, rNode);

	char szNode[256];
	g_pLTServer->GetModelNodeName(m_hObject, nNode, szNode, 255);

	LTRotation rAttachment = ~rNode * rRot;
	LTVector vAttachment, vNull;
	g_pMathLT->GetRotationVectors(rAttachment, vNull, vNull, vAttachment);
	vAttachment *= -10.0f;

	HATTACHMENT hAttachment;
	if (g_pLTServer->CreateAttachment(m_hObject, pClass->m_hObject, szNode, &vAttachment, &rAttachment, &hAttachment) != LT_OK)
	{
		g_pLTServer->RemoveObject(pClass->m_hObject);
		return LTFALSE;
	}

	SpearInfo info;
	info.hSpear	= pClass->m_hObject;
	info.nNode	= nNode;
	info.rRot	= rRot;
	info.vPos	= vSpearPos;

	if (!m_Spears.Add(&info))
	{
		g_pLTServer->RemoveObject(pClass->m_hObject);
		return LTFALSE;
	}

	// now that we are attached it's okay to be non-solid
	g_pLTServer->SetObjectFlags(pClass->m_hObject, g_pLTServer->GetObjectFlags(pClass->m_hObject) & ~FLAG_SOLID);

	// make sure we show up...
	g_pLTServer->SetObjectUserFlags(pClass->m_hObject, g_pLTServer->GetObjectUserFlags(pClass->m_hObject) | USRFLG_SPECIAL_ATTACHMENT);

	return LTTRUE;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CAIBase::HandleTrigger (0x10145480)
//
// ----------------------------------------------------------------------- //

LTBOOL CAIBase::HandleTrigger(const char* szMsg)
{
	if (!szMsg) return LTFALSE;

	CTokenizer parser;
	parser.Parse(szMsg, ";=");

	if (!parser.m_nNumTokens) return LTFALSE;

	if (stricmp(parser.m_szTokens[0], "DAMAGE") == 0)
	{
		if (parser.m_nNumTokens < 2) return LTFALSE;

		DamageStruct damage;
		damage.hDamager = LTNULL;
		damage.fDamage = (LTFLOAT)atof(parser.m_szTokens[1]);
		damage.vDir.Init();
		g_pLTServer->GetObjectPos(m_hObject, &damage.vPos);

		HandleDamage(damage);
		return LTTRUE;
	}
	else if (stricmp(parser.m_szTokens[0], "ATTACK") == 0)
	{
		if (parser.m_nNumTokens < 2) return LTFALSE;

		ObjArray<HOBJECT, 1> objArray;
		g_pLTServer->FindNamedObjects(parser.m_szTokens[1], objArray);
		if (!objArray.NumObjects()) return LTFALSE;

		HOBJECT hObj = objArray.GetObject(0);
		if (hObj && m_Memory.IsAware(hObj) == LTTRUE)
		{
			m_Memory.AddStimulus(hObj, 0, m_Memory.GetStimulus(hObj, 0) * 2.0f);
		}

		return LTTRUE;
	}

	return LTFALSE;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	IsAIBaseOrTentacle (0x10145690)
//
// ----------------------------------------------------------------------- //

LTBOOL IsAIBaseOrTentacle(HOBJECT hObj)
{
	if (IsAIBase(hObj) == LTTRUE) return LTTRUE;

	return (IsTentacle(hObj) == LTTRUE);
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	GetDifficultyDamageFactor (0x101456c0)
//
// ----------------------------------------------------------------------- //

LTFLOAT GetDifficultyDamageFactor()
{
	if (g_bGodMode == LTTRUE) return 3.0f;

	switch (g_pGameServerShell->GetGameType().GetDifficulty())
	{
		case DIFFICULTY_PC_EASY:	return 0.25f;
		case DIFFICULTY_PC_MEDIUM:	return 0.5f;
		case DIFFICULTY_PC_HARD:	return 1.0f;
		case DIFFICULTY_PC_INSANE:	return 1.25f;
	}

	return 1.0f;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	GetDifficultyHealthFactor (0x10145730)
//
// ----------------------------------------------------------------------- //

LTFLOAT GetDifficultyHealthFactor()
{
	if (g_bGodMode == LTTRUE) return 2.0f;

	switch (g_pGameServerShell->GetGameType().GetDifficulty())
	{
		case DIFFICULTY_PC_EASY:	return 0.3f;
		case DIFFICULTY_PC_MEDIUM:	return 0.75f;
		case DIFFICULTY_PC_HARD:	return 1.0f;
		case DIFFICULTY_PC_INSANE:	return 1.25f;
	}

	return 1.0f;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CAIBase::UpdateMoving (0x101457a0)
//
// ----------------------------------------------------------------------- //

void CAIBase::UpdateMoving(LTFLOAT fFrameTime)
{
	AIBaseStateInfo info(eStateIdle);

	if (ReturnToLeash() == LTTRUE) return;

	if (m_Move.m_nNode == -1 || m_Move.m_nTo == -1)
	{
		SetState(&info);
		return;
	}

	if (m_Leash.m_nCurNode == -1)
	{
		SetState(&info);
		return;
	}

	if (!m_Leash.GetNode(m_Move.m_nNode))
	{
		SetState(&info);
		return;
	}

	LTVector vPos = m_Move.m_vPos;
	LTVector vDiff = vPos - m_MoveMgr.GetPos(m_hObject);
	if (!(m_MoveDist.m_nAxes & 1)) vDiff.x = 0.0f;
	if (!(m_MoveDist.m_nAxes & 2)) vDiff.y = 0.0f;
	if (!(m_MoveDist.m_nAxes & 4)) vDiff.z = 0.0f;

	LTFLOAT fDist = vDiff.Mag();
	m_MoveDist.m_fDist = fDist;
	if (fDist < m_MoveDist.m_fMin) m_MoveDist.m_fMin = fDist;
	if (fDist > m_MoveDist.m_fMax) m_MoveDist.m_fMax = fDist;

	switch (m_Move.m_eMode)
	{
		case 0: SetState(&info); return;
		case 1: UpdateMoveToward(fFrameTime); return;
		case 2: UpdateMoveAway(fFrameTime); return;
		case 3: UpdateMoveTo(fFrameTime); return;
	}

	SetState(&info);
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CAIBase::SetStateMoving (0x101459a0)
//
// ----------------------------------------------------------------------- //

LTBOOL CAIBase::SetStateMoving(const AIBaseStateInfo* pInfo)
{
	int nNode = -1;
	int nNext = -1;
	int nDest = -1;

	if (!m_Leash.FindPath(m_MoveMgr.GetPos(m_hObject), pInfo->vPos, &nNext, &nNode, &nDest))
		return LTFALSE;

	if (!MoveToNode(nNode, nNext))
		return LTFALSE;

	m_StuckCheck.m_bEnabled = LTTRUE;
	m_Move.m_nFrom = nNode;
	m_Move.m_nTo = nDest;
	m_Move.m_vTarget = pInfo->vPos;

	m_Animator.PlayAnim("Run", LTTRUE, LTFALSE);

	m_MoveMgr.m_eMoveType = 2;
	m_OrientMgr.m_bFacePos = LTTRUE;
	m_MoveMgr.SetDest(m_Move.m_vPos);
	m_OrientMgr.SetFaceTarget(m_Move.m_vPos);

	m_eState = eStateMoving;
	m_NextState.eState = pInfo->nParam;
	m_NextState.vPos = LTVector(0.0f, 0.0f, 0.0f);
	m_NextState.nParam = 0;
	m_MoveState = *pInfo;

	return LTTRUE;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CAIBase::MoveToNode (0x10145b20)
//
// ----------------------------------------------------------------------- //

LTBOOL CAIBase::MoveToNode(int nFrom, int nTo)
{
	LTVector vDims;
	g_pLTServer->Physics()->GetObjectDims(m_hObject, &vDims);

	LTVector vPos;
	if (nFrom == nTo)
	{
		vPos = m_MoveMgr.GetPos(m_hObject);
	}
	else if (!m_Leash.GetPosToward(nFrom, nTo, m_MoveMgr.GetPos(m_hObject), vPos, vDims.x))
	{
		return LTFALSE;
	}

	AIDebugPoint(3, vPos, AIDebugColor(0.0f, 0.0f, 0.0f, 1.0f), 2.0f);

	m_Move.m_eMode		= 1;
	m_Move.m_vPos		= vPos;
	m_Move.m_nLastNode	= m_Move.m_nNode;
	m_Move.m_nNode		= nTo;
	m_MoveDist.Reset();

	return LTTRUE;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CAIBase::MoveAway (0x10145c60)
//
// ----------------------------------------------------------------------- //

LTBOOL CAIBase::MoveAway(int nFrom, int nTo)
{
	if (m_Move.m_eMode != 1) return LTFALSE;

	LTVector vDims;
	g_pLTServer->Physics()->GetObjectDims(m_hObject, &vDims);

	LTVector vPos;
	if (nFrom == nTo)
	{
		vPos = m_MoveMgr.GetPos(m_hObject);
	}
	else if (!m_Leash.GetPosAway(nFrom, nTo, m_MoveMgr.GetPos(m_hObject), vPos, vDims.x))
	{
		return LTFALSE;
	}

	AIDebugPoint(2, vPos, AIDebugColor(0.0f, 0.0f, 0.0f, 1.0f), 2.0f);

	m_Move.m_eMode		= 2;
	m_Move.m_nLastNode	= m_Move.m_nNode;
	m_Move.m_nNode		= nTo;
	m_Move.m_vPos		= vPos;
	m_MoveDist.Reset();

	return LTTRUE;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CAIBase::MoveToPos (0x10145db0)
//
// ----------------------------------------------------------------------- //

LTBOOL CAIBase::MoveToPos(int nNode, const LTVector& vPos)
{
	m_Move.m_eMode		= 3;
	m_Move.m_vPos		= vPos;
	m_Move.m_nLastNode	= m_Move.m_nNode;
	m_Move.m_nNode		= nNode;
	m_MoveDist.Reset();

	AIDebugPoint(4, vPos, AIDebugColor(0.0f, 0.0f, 0.0f, 1.0f), 2.0f);

	return LTTRUE;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CAIBase::UpdateMoveToward (0x10145e60)
//
// ----------------------------------------------------------------------- //

void CAIBase::UpdateMoveToward(LTFLOAT fFrameTime)
{
	AIBaseStateInfo info(eStateIdle);

	if ((m_MoveDist.m_fMin < m_MoveDist.m_fDist && m_MoveDist.m_fMin < 128.0f) || m_MoveDist.m_fDist < 32.0f)
	{
		if (m_Leash.m_nCurNode == m_Move.m_nNode)
		{
			if (m_Leash.m_nCurNode == m_Move.m_nTo)
			{
				if (HasMoveTarget() == LTTRUE)
				{
					if (!MoveToPos(m_Move.m_nTo, GetMoveTarget()))
						SetState(&info);
				}
				else
				{
					SetState(&m_NextState);
				}
				return;
			}

			int nStart = -1, nNext = -1, nDest = -1;
			LTVector vTarget = m_Move.m_vTarget;
			if (!m_Leash.FindPath(m_MoveMgr.GetPos(m_hObject), vTarget, &nNext, &nStart, &nDest) ||
				!MoveToNode(nStart, nNext))
			{
				SetState(&info);
				return;
			}

			m_Move.m_nTo = nDest;
			m_Move.m_nFrom = nStart;
			return;
		}

		if (!MoveAway(m_Leash.m_nCurNode, m_Move.m_nNode))
		{
			SetState(&info);
			return;
		}
	}

	m_MoveMgr.SetDest(m_Move.m_vPos);
	m_OrientMgr.SetFaceTarget(m_Move.m_vPos);
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CAIBase::UpdateMoveAway (0x10146060)
//
// ----------------------------------------------------------------------- //

void CAIBase::UpdateMoveAway(LTFLOAT fFrameTime)
{
	AIBaseStateInfo info(eStateIdle);

	if (m_MoveDist.m_fMin < m_MoveDist.m_fDist && m_MoveDist.m_fMin < 128.0f)
	{
		if (m_Leash.m_nCurNode != m_Move.m_nNode)
		{
			if (SetStateMoving(&m_MoveState) == LTTRUE) return;

			AIBaseStateInfo home(eStateIdle);
			home.eState = eStateMoving;
			home.vPos = m_Leash.m_vHomePos;
			home.nParam = 0;
			if (SetStateMoving(&home)) return;

			SetState(&info);
			return;
		}

		int nNext = -1, nStart = -1, nDest = -1;
		LTVector vTarget = m_Move.m_vTarget;
		if (!m_Leash.FindPath(m_MoveMgr.GetPos(m_hObject), vTarget, &nNext, &nStart, &nDest))
		{
			SetState(&info);
			return;
		}

		if (nNext != m_Move.m_nTo)
		{
			MoveToNode(nStart, nNext);
		}
		else if (HasMoveTarget() == LTTRUE)
		{
			MoveToPos(nDest, GetMoveTarget());
		}
		else
		{
			SetState(&m_NextState);
			return;
		}
	}

	m_MoveMgr.SetDest(m_Move.m_vPos);
	m_OrientMgr.SetFaceTarget(m_Move.m_vPos);
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CAIBase::UpdateMoveTo (0x10146250)
//
// ----------------------------------------------------------------------- //

void CAIBase::UpdateMoveTo(LTFLOAT fFrameTime)
{
	AIBaseStateInfo info(eStateIdle);

	if (m_MoveDist.m_fMin < m_MoveDist.m_fDist && m_MoveDist.m_fMin < 128.0f)
	{
		if (m_Leash.m_nCurNode != m_Move.m_nTo)
			SetState(&info);
		else
			SetState(&m_NextState);
		return;
	}

	m_MoveMgr.SetDest(m_Move.m_vPos);
	m_OrientMgr.SetFaceTarget(m_Move.m_vPos);
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CAIBase::HasMoveTarget (0x10146330)
//
// ----------------------------------------------------------------------- //

LTBOOL CAIBase::HasMoveTarget()
{
	return LTTRUE;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CAIBase::GetMoveTarget (0x10146340)
//
// ----------------------------------------------------------------------- //

LTVector CAIBase::GetMoveTarget()
{
	return m_Move.m_vTarget;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CAIBase::UpdateIdle / UpdateAlert (slots 28 and 29)
//
//	PURPOSE:	Nothing. Not inline: CAIChameleon calls them (0x1014963a,
//				0x1014970e), and the call lands on an empty function
//				folded with others (0x10151c90).
//
// ----------------------------------------------------------------------- //

void CAIBase::UpdateIdle(LTFLOAT fFrameTime)
{
}

void CAIBase::UpdateAlert(LTFLOAT fFrameTime)
{
}

#endif // PRIMAL_HUNT
