// ----------------------------------------------------------------------- //
//
// MODULE  : Tentacle.cpp
//
// PURPOSE : CTentacle - implementation
//
// CREATED : Primal Hunt (Third Law); no source survives, written from
//           PHObject.lto (0x10156060..0x10157e5a)
//
// ----------------------------------------------------------------------- //

#include "stdafx.h"

#ifdef PRIMAL_HUNT

#include "Tentacle.h"
#include "AITentacle.h"
#include "AIChameleon.h"
#include "TentHole.h"
#include "Destructible.h"
#include "ServerUtilities.h"
#include "CharacterHitBox.h"
#include "SurfaceFunctions.h"
#include "CommonUtilities.h"
#include "ObjectMsgs.h"

#define TENTACLE_MELEE_RANGE		128.0f
#define TENTACLE_GROUND_DIST		128.0f

// The tentacle's sounds, by what it is doing
enum TentacleSound
{
	eSoundAttack1 = 0,
	eSoundAttack2,
	eSoundErupt,
	eSoundErupt2,
	eSoundEruptScream2,
	eSoundEruptScream5,
	eSoundBurrow,
	eSoundBurrow2,
	eSoundPain1,
	eSoundPain2,
	eSoundDeath0,
	eSoundDeath1
};

static char* s_aszSounds[] =
{
	"sounds/exp/characters/tentacle/attack1.wav",
	"sounds/exp/characters/tentacle/attack2.wav",
	"sounds/exp/characters/tentacle/erupt.wav",
	"sounds/exp/characters/tentacle/erupt2.wav",
	"sounds/exp/characters/tentacle/erupt_scream2.wav",
	"sounds/exp/characters/tentacle/erupt_scream5.wav",
	"sounds/exp/characters/tentacle/burrow.wav",
	"sounds/exp/characters/tentacle/burrow2.wav",
	"sounds/exp/characters/tentacle/pain1.wav",
	"sounds/exp/characters/tentacle/pain2.wav",
	"sounds/exp/characters/tentacle/death.wav",
	"sounds/exp/characters/tentacle/death.wav"
};

// PH: the $E initializer 0x10156060
BEGIN_CLASS(CTentacle)
END_CLASS_DEFAULT_FLAGS(CTentacle, GameBase, NULL, NULL, CF_HIDDEN)

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CreateTentacleObject / IsTentacle
//
// ----------------------------------------------------------------------- //

// PH: 0x10156150
CTentacle* CreateTentacleObject(ObjectCreateStruct* pStruct)
{
	HCLASS hClass = g_pLTServer->GetClass("CTentacle");
	if (!hClass) return LTNULL;

	return (CTentacle*)g_pLTServer->CreateObject(hClass, pStruct);
}

// PH: 0x10156180 (CProjectile's touch, PHObject.lto 0x101456a6)
LTBOOL IsTentacle(HOBJECT hObj)
{
	HCLASS hTentacle = g_pLTServer->GetClass("CTentacle");
	if (!hTentacle) return LTFALSE;

	HCLASS hClass = g_pLTServer->GetObjectClass(hObj);
	if (!hClass) return LTFALSE;

	return g_pLTServer->IsKindOf(hClass, hTentacle) == LTTRUE;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CTentacle::CTentacle()
//
// ----------------------------------------------------------------------- //

// PH: 0x101561d0
CTentacle::CTentacle() : GameBase(OT_MODEL)
{
	m_hOwner = LTNULL;
	m_eState = eHidden;
	m_hTarget = LTNULL;
	m_bJoinOwner = LTFALSE;
	m_hHole = LTNULL;
	m_fHitPoints = 250.0f;
	m_fDamage = 20.0f;
	m_fScale = 1.0f;
	m_vNormal.Init(0.0f, 1.0f, 0.0f);
	m_bDeath1 = LTFALSE;
}

// PH: 0x101563e0
CTentacle::~CTentacle()
{
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CreateTentacle()
//
// ----------------------------------------------------------------------- //

// PH: 0x101564c0 (CAITentacle, PHObject.lto 0x1014c2de)
CTentacle* CreateTentacle(HOBJECT hOwner, const LTVector & vPos, LTFLOAT fHitPoints,
						  LTFLOAT fDamage, LTFLOAT fScale)
{
	if (!hOwner) return LTNULL;

	ObjectCreateStruct theStruct;

	theStruct.m_Pos = vPos;
	theStruct.m_Scale.Init(fScale, fScale, fScale);

	CTentacle* pTentacle = CreateTentacleObject(&theStruct);
	if (!pTentacle) return LTNULL;

	pTentacle->m_fHitPoints = fHitPoints;
	pTentacle->m_fDamage = fDamage;
	pTentacle->m_hOwner = hOwner;
	pTentacle->m_fScale = fScale;

	return pTentacle;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CTentacle::OnPrecreate()
//
// ----------------------------------------------------------------------- //

// PH: 0x101565c0
uint32 CTentacle::OnPrecreate(void *pData, uint32 nInfo)
{
	if (nInfo == PRECREATE_WORLDFILE || nInfo == PRECREATE_NORMAL)
	{
		ObjectCreateStruct* pStruct = (ObjectCreateStruct*)pData;

		SAFE_STRCPY(pStruct->m_Filename, "models/exp/characters/tentacle.abc");
		SAFE_STRCPY(pStruct->m_SkinName, "skins/exp/characters/tentacle.dtx");
		pStruct->m_Flags |= FLAG_ANIMTRANSITION | FLAG_RAYHIT | FLAG_MODELKEYS;
		pStruct->m_Flags2 |= FLAG2_DYNAMICDIRLIGHT;

		m_fScale = pStruct->m_Scale.x;
	}

	return 0;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CTentacle::OnInitialUpdate()
//
// ----------------------------------------------------------------------- //

// PH: 0x10156660
uint32 CTentacle::OnInitialUpdate(uint32 nInfo)
{
	GameBase::OnInitialUpdate(nInfo);

	LTVector vDims;
	g_pLTServer->Common()->GetModelAnimUserDims(m_hObject, &vDims, 0);
	vDims *= m_fScale;
	g_pLTServer->Physics()->SetObjectDims(m_hObject, &vDims, 0);

	g_pLTServer->SetForceIgnoreLimit(m_hObject, 0.0f);
	g_pLTServer->SetFrictionCoefficient(m_hObject, 0.0f);

	m_Anim.Init(m_hObject);
	uint32 hWeightSet = m_Anim.FindWeightSet("Full");
	m_Anim.SetWeightSet(hWeightSet, 0);

	m_Sounds.m_hObject = m_hObject;

	g_pLTServer->SetBlockingPriority(m_hObject, 255);
	uint32 dwUsrFlags = SurfaceToUserFlag(ST_FLESH_HUMAN);
	g_pLTServer->SetObjectUserFlags(m_hObject, dwUsrFlags);

	SetNextUpdate(0.001f);

	if (nInfo != INITIALUPDATE_SAVEGAME)
	{
		Hide();
	}

	return 0;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CTentacle::OnUpdate()
//
// ----------------------------------------------------------------------- //

// PH: 0x10156790
uint32 CTentacle::OnUpdate(LTFLOAT fFrameTime)
{
	if (m_bJoinOwner == LTTRUE)
	{
		CAITentacle* pOwner = GetAITentacle(m_hOwner);
		if (pOwner)
		{
			pOwner->AddTentacle(this);
		}

		m_bJoinOwner = LTFALSE;
	}

	m_Orientation.Update(m_hObject, fFrameTime);

	// RETAIL BUG: StartAttack() puts us in eAttacking and Burrow() in
	// eBurrowing, but this switch has their updates the other way about:
	// an attack ends as soon as it starts (the burrow timer is long
	// done), and a burrow restarts every frame once the attack timer is
	switch (m_eState)
	{
		case eHidden:		UpdateHidden(fFrameTime);		break;
		case eWaiting:		UpdateWaiting(fFrameTime);		break;
		case eErupting:		UpdateErupting(fFrameTime);		break;
#ifdef AVP2SRC_MODERN
		// M-B82 (RETAIL_BUGS.md): each state its own update
		case eBurrowing:	UpdateBurrowing(fFrameTime);	break;
		case eAttacking:	UpdateAttacking(fFrameTime);	break;
#else
		case eBurrowing:	UpdateAttacking(fFrameTime);	break;
		case eAttacking:	UpdateBurrowing(fFrameTime);	break;
#endif
		case eUnused:		UpdateUnused(fFrameTime);		break;
		case eDying:		UpdateDying(fFrameTime);		break;
		case eDead:			UpdateDead(fFrameTime);			break;
		case ePain:			UpdatePain(fFrameTime);			break;
	}

	SetNextUpdate(0.001f);

	return 0;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CTentacle::OnSave()
//
// ----------------------------------------------------------------------- //

// PH: 0x10156890
void CTentacle::OnSave(HMESSAGEWRITE hWrite)
{
	GameBase::OnSave(hWrite);

	hWrite->WriteObject(m_hOwner);
	hWrite->WriteByte(m_eState);
	hWrite->WriteFloat(m_tmrErupt.m_fTime);
	hWrite->WriteFloat(m_tmrBurrow.m_fTime);
	hWrite->WriteFloat(m_tmrAttack.m_fTime);
	hWrite->WriteFloat(m_tmrWait.m_fTime);
	hWrite->WriteFloat(m_tmrPain.m_fTime);
	m_Anim.Save(hWrite);
	hWrite->WriteObject(m_hTarget);
	m_Orientation.Save(hWrite);
	hWrite->WriteFloat(m_Sounds.m_fSoundEnd);
	hWrite->WriteFloat(m_Sounds.m_fNextPain);
	hWrite->WriteFloat(m_Sounds.m_fNextAlert);
	hWrite->WriteFloat(m_Sounds.m_fVolume);
	hWrite->WriteFloat(m_Sounds.m_fRadius);
	hWrite->WriteDWord(m_Sounds.m_dwFlags);
	hWrite->WriteDWord(m_Sounds.m_nVolume);
	hWrite->WriteFloat(m_Sounds.m_fPitch);
	hWrite->WriteByte(m_Sounds.m_nPriority);
	hWrite->WriteFloat(m_Sounds.m_fInner);
	hWrite->WriteFloat(m_Sounds.m_fOuter);
	hWrite->WriteObject(m_Sounds.m_hObject);
	hWrite->WriteFloat(m_fHitPoints);
	hWrite->WriteFloat(m_fDamage);
	hWrite->WriteByte(m_ePainState);
	hWrite->WriteFloat(m_fScale);
	hWrite->WriteVector(m_vNormal);
	hWrite->WriteByte(m_bDeath1);
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CTentacle::OnLoad()
//
// ----------------------------------------------------------------------- //

// PH: 0x10156a30
void CTentacle::OnLoad(HMESSAGEREAD hRead)
{
	GameBase::OnLoad(hRead);

	m_hOwner						= hRead->ReadObject();
	m_eState						= hRead->ReadByte();
	m_tmrErupt.m_fTime				= hRead->ReadFloat();
	m_tmrBurrow.m_fTime				= hRead->ReadFloat();
	m_tmrAttack.m_fTime				= hRead->ReadFloat();
	m_tmrWait.m_fTime				= hRead->ReadFloat();
	m_tmrPain.m_fTime				= hRead->ReadFloat();
	m_Anim.Load(hRead);
	m_hTarget						= hRead->ReadObject();
	m_Orientation.Load(hRead);
	m_Sounds.m_fSoundEnd		= hRead->ReadFloat();
	m_Sounds.m_fNextPain		= hRead->ReadFloat();
	m_Sounds.m_fNextAlert		= hRead->ReadFloat();
	m_Sounds.m_fVolume			= hRead->ReadFloat();
	m_Sounds.m_fRadius			= hRead->ReadFloat();
	m_Sounds.m_dwFlags			= hRead->ReadDWord();
	m_Sounds.m_nVolume				= hRead->ReadDWord();
	m_Sounds.m_fPitch				= hRead->ReadFloat();
	m_Sounds.m_nPriority			= hRead->ReadByte();
	m_Sounds.m_fInner			= hRead->ReadFloat();
	m_Sounds.m_fOuter			= hRead->ReadFloat();
	m_Sounds.m_hObject				= hRead->ReadObject();
	m_fHitPoints					= hRead->ReadFloat();
	m_fDamage						= hRead->ReadFloat();
	m_ePainState					= hRead->ReadByte();
	m_fScale						= hRead->ReadFloat();
	hRead->ReadVectorFL(m_vNormal);
	m_bDeath1						= hRead->ReadByte();

	m_hHole = LTNULL;
	m_bJoinOwner = LTTRUE;

	SetNextUpdate(0.001f);
	g_pLTServer->SetObjectState(m_hObject, OBJSTATE_ACTIVE);
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CTentacle::ObjectMessageFn()
//
// ----------------------------------------------------------------------- //

// PH: 0x10156d40
uint32 CTentacle::ObjectMessageFn(HOBJECT hSender, uint32 messageID, HMESSAGEREAD hRead)
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

		default : break;
	}

	return GameBase::ObjectMessageFn(hSender, messageID, hRead);
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CTentacle::OnModelStringKey()
//
// ----------------------------------------------------------------------- //

// PH: 0x10156e80
uint32 CTentacle::OnModelStringKey(void *pData)
{
	ArgList* pArgList = (ArgList*)pData;

	if (pArgList->argc)
	{
		LTVector vPos;
		g_pLTServer->GetObjectPos(m_hObject, &vPos);

		if (_stricmp(pArgList->argv[0], "damage_melee") == 0)
		{
			DoMeleeAttack();
		}
	}

	return 0;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CTentacle::Erupt()
//
// ----------------------------------------------------------------------- //

// PH: 0x10156ed0 (CAITentacle, PHObject.lto 0x1014d363)
void CTentacle::Erupt(HOBJECT hTarget)
{
	if (m_eState != eHidden) return;

	LTVector vPos, vNormal;
	if (FindGround(vPos, vNormal))
	{
		m_hTarget = hTarget;
		Erupt(vPos, vNormal);
	}
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CTentacle state updates
//
// ----------------------------------------------------------------------- //

// PH: 0x10156f80
void CTentacle::UpdateWaiting(LTFLOAT fFrameTime)
{
	if (m_tmrWait.IsDone())
	{
		StartAttack();
	}
}

// PH: 0x10156fb0
void CTentacle::UpdateErupting(LTFLOAT fFrameTime)
{
	LTVector vTargetPos;
	g_pLTServer->GetObjectPos(m_hTarget, &vTargetPos);

	m_Orientation.m_bFacePos = LTTRUE;
	m_Orientation.m_vFacePos = vTargetPos;

	if (m_tmrErupt.IsDone())
	{
		StartAttack();
	}
}

// PH: 0x10157030
void CTentacle::UpdateBurrowing(LTFLOAT fFrameTime)
{
	if (m_tmrBurrow.IsDone())
	{
		Hide();
	}
}

// PH: 0x10157060
void CTentacle::UpdateAttacking(LTFLOAT fFrameTime)
{
	uint32 dwFlags;
	g_pLTServer->Common()->GetObjectFlags(m_hObject, OFT_Flags, dwFlags);

	LTVector vTargetPos;
	g_pLTServer->GetObjectPos(m_hTarget, &vTargetPos);

	m_Orientation.m_bFacePos = LTTRUE;
	m_Orientation.m_vFacePos = vTargetPos;

	if (m_tmrAttack.IsDone())
	{
		Burrow();
	}
}

// PH: 0x101570f0
void CTentacle::UpdateDying(LTFLOAT fFrameTime)
{
	if (m_Anim.GetPlaybackState(0) & MS_PLAYDONE)
	{
		Dead();
	}
}

// PH: 0x10157110
void CTentacle::UpdateHidden(LTFLOAT fFrameTime)
{
	g_pLTServer->SetObjectState(m_hObject, OBJSTATE_INACTIVE);
}

// PH: 0x10157110 (the linker's one copy of UpdateHidden's body)
void CTentacle::UpdateDead(LTFLOAT fFrameTime)
{
	g_pLTServer->SetObjectState(m_hObject, OBJSTATE_INACTIVE);
}

// PH: 0x10157130
void CTentacle::UpdatePain(LTFLOAT fFrameTime)
{
	if (!m_tmrPain.IsDone()) return;

	switch (m_ePainState)
	{
		case eWaiting:
		case eErupting:
			StartAttack();
		break;

		case eBurrowing:
		case eAttacking:
			Burrow();
		break;
	}
}

// PH: 0x10151c90 (the linker's one copy of an empty member)
void CTentacle::UpdateUnused(LTFLOAT fFrameTime)
{
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CTentacle::Hide()
//
// ----------------------------------------------------------------------- //

// PH: 0x10157190
void CTentacle::Hide()
{
	HideModel();

	m_Anim.PlayAnim("erupt_Idle", LTTRUE, 0);
	m_hTarget = LTNULL;
	m_eState = eHidden;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CTentacle::Erupt()
//
// ----------------------------------------------------------------------- //

// PH: 0x101571f0
void CTentacle::Erupt(const LTVector & vPos, const LTVector & vNormal)
{
	ShowModel();

	if (rand() % 100 < 50)
	{
		m_Anim.PlayAnim("Erupt", LTFALSE, 0);
		m_tmrErupt.Start(0.9f);
	}
	else
	{
		m_Anim.PlayAnim("Erupt2", LTFALSE, 0);
		m_tmrErupt.Start(0.533f);
	}

	LTVector vEruptPos = vPos;
	vEruptPos.y += m_fScale * 48.0f;
	g_pLTServer->SetObjectPos(m_hObject, &vEruptPos);

	m_eState = eErupting;

	m_vNormal = vNormal;
	m_vNormal.Norm();

	CTentHole* pHole = CTentHole::Spawn(vPos, m_vNormal, m_fScale);
	if (pHole)
	{
		m_hHole = pHole->m_hObject;
	}
	else
	{
		m_hHole = LTNULL;
	}

	m_Sounds.PlaySound(s_aszSounds[GetRandom(eSoundErupt, eSoundErupt2)], vEruptPos, 100, 0, 0x80, 256.0f, 3072.0f, 0x3a);

	if (rand() % 100 < 50)
	{
		m_Sounds.PlayVoice(s_aszSounds[GetRandom(eSoundEruptScream2, eSoundEruptScream5)]);
	}

	if (m_hOwner)
	{
		CAITentacle* pOwner = GetAITentacle(m_hOwner);
		if (pOwner)
		{
			LTFLOAT fShake = m_fScale * 6.0f;
			LTVector vShake(fShake, fShake, fShake);
			pOwner->ShakePlayer(vEruptPos, vShake, m_fScale * 512.0f, 1.0f, 0.01f, 0.05f);
		}
	}

	LTVector vFacePos = vPos;
	vFacePos.y += 8.0f;
	FaceTarget(vFacePos);
}

// PH: 0x10151c90 (the linker's one copy of an empty member)
void CTentacle::FaceTarget(const LTVector & vPos)
{
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CTentacle::Burrow()
//
// ----------------------------------------------------------------------- //

// PH: 0x101574b0
void CTentacle::Burrow()
{
	if (rand() % 100 < 50)
	{
		m_Anim.PlayAnim("Burrow", LTFALSE, 0);
		m_tmrBurrow.Start(0.9f);
	}
	else
	{
		m_Anim.PlayAnim("Burrow2", LTFALSE, 0);
		m_tmrBurrow.Start(0.8f);
	}

	m_eState = eBurrowing;

	if (m_hHole)
	{
		CTentHole* pHole = GetTentHole(m_hHole);
		if (pHole)
		{
			pHole->Close();
		}
	}

	LTVector vPos;
	g_pLTServer->GetObjectPos(m_hObject, &vPos);

	m_Sounds.PlaySound(s_aszSounds[GetRandom(eSoundBurrow, eSoundBurrow2)], vPos, 100, 0, 0x80, 256.0f, 3072.0f, 0x3a);
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CTentacle::StartAttack()
//
// ----------------------------------------------------------------------- //

// PH: 0x101575b0
void CTentacle::StartAttack()
{
	const char* szSound;

	if (rand() % 100 < 50)
	{
		m_Anim.PlayAnim("Attack0", LTFALSE, 0);
		m_tmrAttack.Start(1.0f);
		szSound = s_aszSounds[eSoundAttack1];
	}
	else
	{
		m_Anim.PlayAnim("Attack1", LTFALSE, 0);
		m_tmrAttack.Start(1.0f);
		szSound = s_aszSounds[eSoundAttack2];
	}

	m_Sounds.PlayVoice(szSound);
	m_eState = eAttacking;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CTentacle::Die()
//
// ----------------------------------------------------------------------- //

// PH: 0x10157640
void CTentacle::Die()
{
	if (m_eState == eHidden || m_vNormal.y < 0.999f || rand() % 100 < 50)
	{
		m_bDeath1 = LTTRUE;
	}

	int nSound;
	if (!m_bDeath1)
	{
		m_Anim.PlayAnim("Death0", LTFALSE, 0);
		nSound = eSoundDeath0;
	}
	else
	{
		m_Anim.PlayAnim("Death1", LTFALSE, 0);
		nSound = eSoundDeath1;
	}

	m_eState = eDying;
	m_Sounds.PlayVoice(s_aszSounds[nSound]);

	if (m_hHole)
	{
		CTentHole* pHole = GetTentHole(m_hHole);
		if (pHole)
		{
			pHole->Hide();
		}
	}
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CTentacle::Dead()
//
// ----------------------------------------------------------------------- //

// PH: 0x10157700
void CTentacle::Dead()
{
	m_eState = eDead;

	uint32 dwFlags;
	g_pLTServer->Common()->GetObjectFlags(m_hObject, OFT_Flags, dwFlags);
	g_pLTServer->Common()->SetObjectFlags(m_hObject, OFT_Flags, dwFlags & ~(FLAG_RAYHIT | FLAG_SOLID));

	if (m_bDeath1 == LTTRUE)
	{
		HideModel();
	}
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CTentacle::Pain()
//
// ----------------------------------------------------------------------- //

// PH: 0x10157760
void CTentacle::Pain()
{
	m_ePainState = m_eState;
	m_eState = ePain;

	m_Anim.PlayAnim("Pain0", LTFALSE, 0);
	m_tmrPain.Start(0.534f);
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CTentacle::ShowModel() / HideModel()
//
// ----------------------------------------------------------------------- //

// PH: 0x101577a0
void CTentacle::ShowModel()
{
	uint32 dwFlags;
	g_pLTServer->Common()->GetObjectFlags(m_hObject, OFT_Flags, dwFlags);
	g_pLTServer->Common()->SetObjectFlags(m_hObject, OFT_Flags, dwFlags | FLAG_VISIBLE | FLAG_SOLID);

	g_pLTServer->SetObjectState(m_hObject, OBJSTATE_ACTIVE);
	uint32 dwUsrFlags = g_pLTServer->GetObjectUserFlags(m_hObject);
	g_pLTServer->SetObjectUserFlags(m_hObject, dwUsrFlags & ~USRFLG_IGNORE_PROJECTILES);
}

// PH: 0x10157820
void CTentacle::HideModel()
{
	uint32 dwFlags;
	g_pLTServer->Common()->GetObjectFlags(m_hObject, OFT_Flags, dwFlags);
	g_pLTServer->Common()->SetObjectFlags(m_hObject, OFT_Flags, dwFlags & ~(FLAG_VISIBLE | FLAG_SOLID));

	g_pLTServer->SetObjectState(m_hObject, OBJSTATE_INACTIVE);
	m_Orientation.m_bFacePos = LTFALSE;
	uint32 dwUsrFlags = g_pLTServer->GetObjectUserFlags(m_hObject);
	g_pLTServer->SetObjectUserFlags(m_hObject, dwUsrFlags | USRFLG_IGNORE_PROJECTILES);
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CTentacle::FindGround()
//
//	PURPOSE:	The floor under us, if it is flat enough to erupt from
//
// ----------------------------------------------------------------------- //

// PH: 0x101578a0
static LTBOOL GroundFilterFn(HOBJECT hObj, void *pUserData)
{
	uint32 dwType;
	g_pLTServer->Common()->GetObjectType(hObj, &dwType);

	return dwType == OT_WORLDMODEL || dwType == OT_NORMAL;
}

// PH: 0x101578d0
LTBOOL CTentacle::FindGround(LTVector & vPos, LTVector & vNormal)
{
	LTVector vMyPos;
	g_pLTServer->GetObjectPos(m_hObject, &vMyPos);

	IntersectQuery iQuery;
	IntersectInfo iInfo;

	iQuery.m_From = vMyPos;
	iQuery.m_To = LTVector(vMyPos.x, vMyPos.y - TENTACLE_GROUND_DIST, vMyPos.z);
	iQuery.m_Flags = INTERSECT_HPOLY | INTERSECT_OBJECTS | IGNORE_NONSOLID;
	iQuery.m_FilterFn = GroundFilterFn;

	if (!g_pLTServer->IntersectSegment(&iQuery, &iInfo)) return LTFALSE;
	if (iInfo.m_Plane.m_Normal.y < 0.7071f) return LTFALSE;

	vPos = iInfo.m_Point;
	vNormal = iInfo.m_Plane.m_Normal;

	return LTTRUE;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CTentacle::HandleDamage()
//
// ----------------------------------------------------------------------- //

// PH: 0x10157a20
void CTentacle::HandleDamage(const DamageStruct & damage)
{
	if (m_eState == eDying || m_eState == eDead || m_eState == eHidden) return;
	if (m_eState == eBurrowing && m_tmrBurrow.GetTimeLeft() < 0.2f) return;

	m_fHitPoints -= damage.fDamage;
	if (m_fHitPoints <= 0.0f)
	{
		Die();
		return;
	}

	m_Sounds.PlayPain(s_aszSounds[GetRandom(eSoundPain1, eSoundPain2)]);

	if (m_eState != ePain && rand() % 100 < 25)
	{
		Pain();
	}
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CTentacle::DoMeleeAttack()
//
//	PURPOSE:	The "damage_melee" key of an attack: lash out along our
//				facing, or straight at the target if it is near enough
//				in front
//
// ----------------------------------------------------------------------- //

// PH: 0x10157ae0
LTBOOL CTentacle::DoMeleeAttack()
{
	LTVector vDir = m_Orientation.m_vForward;

	LTVector vPos;
	g_pLTServer->GetObjectPos(m_hObject, &vPos);

	if (m_hTarget)
	{
		LTVector vTargetPos;
		g_pLTServer->GetObjectPos(m_hTarget, &vTargetPos);

		LTVector vToTarget = vTargetPos - vPos;
		vToTarget.Norm();

		if (vToTarget.Dot(vDir) > 0.866f)
		{
			vDir = vToTarget;
		}
	}

	IntersectQuery iQuery;
	IntersectInfo iInfo;

	iQuery.m_From = vPos;
	iQuery.m_To = vPos + vDir * (m_fScale * TENTACLE_MELEE_RANGE);
	iQuery.m_FilterFn = ChameleonSwipeFilterFn;
	iQuery.m_Flags = INTERSECT_OBJECTS | IGNORE_NONSOLID;
	iQuery.m_PolyFilterFn = LTNULL;
	iQuery.m_pUserData = m_hObject;

	if (g_pLTServer->IntersectSegment(&iQuery, &iInfo) != LTTRUE || !iInfo.m_hObject) return LTFALSE;

	HOBJECT hHit;
	if (IsCharacterHitBox(iInfo.m_hObject) == LTTRUE)
	{
		CCharacterHitBox* pHitBox = (CCharacterHitBox*)g_pLTServer->HandleToObject(iInfo.m_hObject);
		if (!pHitBox) return LTFALSE;

		hHit = pHitBox->GetModelObject();
	}
	else
	{
		hHit = iInfo.m_hObject;
	}

	LTFLOAT fDamage = GetRandom(m_fDamage * 0.5f, m_fDamage);
	((CAIBase*)this)->DamageObject(hHit, &vDir, fDamage, 0.0f, DT_SLICE);

	return LTTRUE;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CTentacle::IsHidden()
//
//	PURPOSE:	Is the tentacle down (not shown)? Folded with
//				Turret::CanShootThrough (0x10092d30); called directly by
//				CAITentacle::IsPosTaken.
//
// ----------------------------------------------------------------------- //

LTBOOL CTentacle::IsHidden()
{
	uint32 dwFlags;
	g_pLTServer->Common()->GetObjectFlags(m_hObject, OFT_Flags, dwFlags);

	return !(dwFlags & FLAG_VISIBLE);
}

#endif // PRIMAL_HUNT
