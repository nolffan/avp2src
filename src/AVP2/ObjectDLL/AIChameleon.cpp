// ----------------------------------------------------------------------- //
//
// MODULE  : AIChameleon.cpp
//
// PURPOSE : Primal Hunt's chameleon: a CAIBase that takes on the texture
//           of the surface under it, stalks, and leaps at its target
//
// ----------------------------------------------------------------------- //

// PH: Third Law's class, PHObject.lto 0x10148380..0x1014b60f (the name of
// the file is ours: retail does not record it).

#include "stdafx.h"

#ifdef PRIMAL_HUNT

#include "AIChameleon.h"
#include "ServerUtilities.h"
#include "SurfaceFunctions.h"
#include "CharacterMgr.h"
#include "PlayerObj.h"
#include "CharacterHitBox.h"

// PH: the chameleon's sounds (0x102e39dc)
static const char* s_aszSounds[] =
{
	"sounds/exp/characters/chameleon/sight.wav",
	"sounds/exp/characters/chameleon/death.wav",
	"sounds/exp/characters/chameleon/step1.wav",
	"sounds/exp/characters/chameleon/step2.wav",
	"sounds/exp/characters/chameleon/step3.wav",
	"sounds/exp/characters/chameleon/pain.wav",
	"sounds/exp/characters/chameleon/idle1.wav",
	"sounds/exp/characters/chameleon/idle1.wav",
	"sounds/exp/characters/chameleon/attack.wav",
};

enum ChameleonSound
{
	eSoundSight = 0,
	eSoundDeath,
	eSoundStep1,
	eSoundStep2,
	eSoundStep3,
	eSoundPain,
	eSoundIdle1,
	eSoundIdle2,
	eSoundAttack,
};

// PH: the chameleon's skins for each vision mode (0x102e3a00), as
// CAIBase::SetSkins takes them
static const char* s_aszSkins[][4] =
{
	{ "skins/exp/characters/chameleonhead.dtx", "skins/exp/characters/chameleontorso.dtx", "skins/exp/characters/chameleonwound.dtx", LTNULL },
	{ "skins/exp/characters/chameleonhead.dtx", "skins/exp/characters/chameleontorso.dtx", "skins/exp/characters/chameleonwound.dtx", LTNULL },
	{ "skins/exp/characters/heatvision/chameleonhead.spr", "skins/exp/characters/heatvision/chameleonbody.spr", "skins/exp/characters/heatvision/chameleonwound.spr", LTNULL },
	{ "skins/exp/characters/electrovision/chameleonhead.dtx", "skins/exp/characters/electrovision/chameleonbody.dtx", "skins/exp/characters/electrovision/chameleonwound.dtx", LTNULL },
	{ "skins/exp/characters/navvision/chameleonhead.spr", "skins/exp/characters/navvision/chameleonbody.spr", "skins/exp/characters/navvision/chameleonwound.spr", LTNULL },
};

// PH: the head's piece and its socket (0x102e3a50)
static const char* s_aszHead[] = { "Head", "Skull" };

// PH: the property table and the class's registration, 0x10148390 and
// 0x10148580; the default constructor, 0x10148530
BEGIN_CLASS(CAIChameleon)
	ADD_VECTORPROP_VAL_FLAG(Dims, 64.0f, 80.0f, 64.0f, PF_HIDDEN | PF_DIMS)
	ADD_BOOLPROP_FLAG_HELP(Wait_To_Attack, LTTRUE, 0, "If TRUE, this AI will wait until the enemy is within Wait_Attack_Dist units to attack.")
	ADD_REALPROP_FLAG_HELP(Wait_Attack_Dist, 256.0f, 0, "How close an enemy must be before this AI will attack if Wait_To_Attack is TRUE.")
END_CLASS_DEFAULT(CAIChameleon, CAIBase, NULL, NULL)

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CAIChameleon::CAIChameleon()
//
//	PURPOSE:	Initialize object
//
// ----------------------------------------------------------------------- //

// PH: 0x101485a0
CAIChameleon::CAIChameleon() : CAIBase(OT_MODEL)
{
	m_fNextSurfaceCheck	= GetRandom(1.0f, 3.0f);
	m_fMaxHitPoints		= 667.0f;
	m_fHitPoints		= 667.0f;
	m_eCamoState		= eCamoOn;
	m_fCamoTime			= 0.0f;
	m_bCamoDone			= LTTRUE;
	m_szTexture[0]		= '\0';
	m_bResetHeadSkin	= LTFALSE;
	m_bWaitToAttack		= LTTRUE;
	m_fWaitAttackDist	= 256.0f;
	m_fSwipeRange		= 192.0f;
	m_eAttackState		= eAttackAlert;
	m_bCrouchIdle		= LTFALSE;
	m_nPainChance		= 50;
	m_fLeapMinDist		= 384.0f;
	m_fLeapMaxDist		= 1024.0f;
	m_fHeadDist			= 80.0f;
}

// PH: 0x10148700
CAIChameleon::~CAIChameleon()
{
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CAIChameleon::OnPrecreate()
//
//	PURPOSE:	Set up the model
//
// ----------------------------------------------------------------------- //

// PH: 0x10148710
uint32 CAIChameleon::OnPrecreate(void *pData, uint32 nInfo)
{
	CAIBase::OnPrecreate(pData, nInfo);

	ObjectCreateStruct* pStruct = (ObjectCreateStruct*)pData;
	if (!pStruct) return 0;

	if (nInfo == PRECREATE_WORLDFILE)
	{
		SAFE_STRCPY(pStruct->m_Filename, "models/exp/characters/chameleon.abc");
		SetSkins(0, pStruct, s_aszSkins);
		SAFE_STRCPY(pStruct->m_ExtraTextureName, "worldtextures/exp/Predator/prock5.dtx");

		pStruct->m_Flags |= FLAG_STAIRSTEP;
		pStruct->m_Flags2 |= FLAG2_CHAMELEON2;
		pStruct->m_Scale.Init(0.8f, 0.8f, 0.8f);

		g_pLTServer->GetPropBool("Wait_To_Attack", &m_bWaitToAttack);
		g_pLTServer->GetPropReal("Wait_Attack_Dist", &m_fWaitAttackDist);
	}

	return 1;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CAIChameleon::OnInitialUpdate()
//
//	PURPOSE:	First update
//
// ----------------------------------------------------------------------- //

// PH: 0x10148800
uint32 CAIChameleon::OnInitialUpdate(uint32 nInfo)
{
	CAIBase::OnInitialUpdate(nInfo);

	m_Animator.SetAllowTransition(0, LTTRUE);

	m_szHeadPiece	= s_aszHead[0];
	m_szHeadSocket	= s_aszHead[1];

	if (nInfo != INITIALUPDATE_SAVEGAME)
	{
		g_pLTServer->SetObjectColor(m_hObject, 1.0f, 1.0f, 1.0f, 0.0f);
		SetNextUpdate(0.001f);

		StartToCrouch();

		m_MoveMgr.m_bInstant	= LTFALSE;
		m_MoveMgr.m_fSpeed	= 5000.0f;

		HMODELPIECE hPiece;
		g_pLTServer->GetModelLT()->GetPiece(m_hObject, "HeadNub", hPiece);
		g_pLTServer->GetModelLT()->SetPieceHideStatus(m_hObject, hPiece, LTTRUE);
		g_pLTServer->GetModelLT()->GetPiece(m_hObject, "NeckNub", hPiece);
		g_pLTServer->GetModelLT()->SetPieceHideStatus(m_hObject, hPiece, LTTRUE);
	}

	return 1;
}

// PH: 0x10148920 (CAIBull's is the same, folded into this one)
uint32 CAIChameleon::OnUpdate(LTFLOAT fFrameTime)
{
	CAIBase::OnUpdate(fFrameTime);
	return 1;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CAIChameleon::OnLoad()
//
//	PURPOSE:	Restore the object
//
// ----------------------------------------------------------------------- //

// PH: 0x10148940
void CAIChameleon::OnLoad(HMESSAGEREAD hRead)
{
	CAIBase::OnLoad(hRead);

	m_fNextSurfaceCheck	= hRead->ReadFloat();
	m_eCamoState		= (CamoState)hRead->ReadByte();
	m_fCamoTime			= hRead->ReadFloat();
	m_bCamoDone			= hRead->ReadByte();
	hRead->ReadStringFL(m_szTexture, sizeof(m_szTexture));
	m_bResetHeadSkin	= hRead->ReadByte();
	m_TargetDist.m_fMin	= hRead->ReadFloat();
	m_TargetDist.m_fMax	= hRead->ReadFloat();
	m_TargetDist.m_fDist = hRead->ReadFloat();
	m_TargetDist.m_nAxes = hRead->ReadDWord();
	m_bWaitToAttack		= hRead->ReadByte();
	m_fWaitAttackDist	= hRead->ReadFloat();
	m_eAttackState		= (AttackState)hRead->ReadByte();
	m_fSwipeRange		= hRead->ReadFloat();
	m_tmrSwipe.m_fTime = hRead->ReadFloat();
	m_tmrAlert.m_fTime = hRead->ReadFloat();
	m_Stare.m_fMaxTime	= hRead->ReadFloat();
	m_Stare.m_fFOV	= hRead->ReadFloat();
	m_Stare.m_fTime		= hRead->ReadFloat();
	m_bCrouchIdle		= hRead->ReadByte();
	m_tmrToCrouch.m_fTime = hRead->ReadFloat();
	m_tmrIdleSound.m_fTime = hRead->ReadFloat();
	m_fLeapMinDist		= hRead->ReadFloat();
	m_fLeapMaxDist		= hRead->ReadFloat();
	m_tmrLeap.m_fTime = hRead->ReadFloat();
	m_tmrNextLeap.m_fTime = hRead->ReadFloat();
	m_tmrPounce.m_fTime = hRead->ReadFloat();
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CAIChameleon::OnSave()
//
//	PURPOSE:	Save the object
//
// ----------------------------------------------------------------------- //

// PH: 0x10148bd0
void CAIChameleon::OnSave(HMESSAGEWRITE hWrite)
{
	CAIBase::OnSave(hWrite);

	hWrite->WriteFloat(m_fNextSurfaceCheck);
	hWrite->WriteByte(m_eCamoState);
	hWrite->WriteFloat(m_fCamoTime);
	hWrite->WriteByte(m_bCamoDone);
	hWrite->WriteString(m_szTexture);
	hWrite->WriteByte(m_bResetHeadSkin);
	hWrite->WriteFloat(m_TargetDist.m_fMin);
	hWrite->WriteFloat(m_TargetDist.m_fMax);
	hWrite->WriteFloat(m_TargetDist.m_fDist);
	hWrite->WriteDWord(m_TargetDist.m_nAxes);
	hWrite->WriteByte(m_bWaitToAttack);
	hWrite->WriteFloat(m_fWaitAttackDist);
	hWrite->WriteByte(m_eAttackState);
	hWrite->WriteFloat(m_fSwipeRange);
	hWrite->WriteFloat(m_tmrSwipe.m_fTime);
	hWrite->WriteFloat(m_tmrAlert.m_fTime);
	hWrite->WriteFloat(m_Stare.m_fMaxTime);
	hWrite->WriteFloat(m_Stare.m_fFOV);
	hWrite->WriteFloat(m_Stare.m_fTime);
	hWrite->WriteByte(m_bCrouchIdle);
	hWrite->WriteFloat(m_tmrToCrouch.m_fTime);
	hWrite->WriteFloat(m_tmrIdleSound.m_fTime);
	hWrite->WriteFloat(m_fLeapMinDist);
	hWrite->WriteFloat(m_fLeapMaxDist);
	hWrite->WriteFloat(m_tmrLeap.m_fTime);
	hWrite->WriteFloat(m_tmrNextLeap.m_fTime);
	hWrite->WriteFloat(m_tmrPounce.m_fTime);
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CAIChameleon::OnModelStringKey()
//
//	PURPOSE:	Handle the model's string keys
//
// ----------------------------------------------------------------------- //

// PH: 0x10148d60
uint32 CAIChameleon::OnModelStringKey(void *pData)
{
	ArgList* pArgList = (ArgList*)pData;
	if (!pArgList->argc) return 0;

	LTVector vPos;
	g_pLTServer->GetObjectPos(m_hObject, &vPos);

	if (_stricmp(pArgList->argv[0], "damage_melee") == 0)
	{
		Swipe();
	}
	else if (_stricmp(pArgList->argv[0], "step_sound") == 0)
	{
		int nStep = atoi(pArgList->argv[1]) - 1;
		m_Sounds.PlaySound(s_aszSounds[eSoundStep1 + nStep], vPos, 100, 0.0f, 128, 256.0f, 3072.0f, 0x3a);
	}
	else if (_stricmp(pArgList->argv[0], "death_sound") == 0)
	{
		int nSound = atoi(pArgList->argv[1]) - 1;
		m_Sounds.PlayVoice(s_aszSounds[eSoundDeath + nSound]);
	}

	return 0;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	ChameleonSurfaceFilterFn()
//
//	PURPOSE:	Let only the world and world models stop the surface ray
//
// ----------------------------------------------------------------------- //

// PH: 0x10148e50
static LTBOOL ChameleonSurfaceFilterFn(HOBJECT hObj, void *pUserData)
{
	ILTPhysics* pPhysics = g_pLTServer->Physics();
	if (!pPhysics || !hObj) return LTFALSE;

	if (pPhysics->IsWorldObject(hObj) == LTTRUE) return LTTRUE;

	uint32 dwType;
	g_pLTServer->Common()->GetObjectType(hObj, &dwType);
	return dwType == OT_WORLDMODEL;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CAIChameleon::FindSurface()
//
//	PURPOSE:	Cast a ray for a surface
//
// ----------------------------------------------------------------------- //

// PH: 0x10148ea0
LTBOOL CAIChameleon::FindSurface(const LTVector& vFrom, const LTVector& vDir, LTFLOAT fDist, IntersectInfo* pInfo)
{
	IntersectQuery iq;
	iq.m_From		= vFrom;
	iq.m_To			= iq.m_From + vDir * fDist;
	iq.m_Direction	= vDir;
	iq.m_Flags		= INTERSECT_OBJECTS | IGNORE_NONSOLID | INTERSECT_HPOLY;
	iq.m_FilterFn	= ChameleonSurfaceFilterFn;
	iq.m_PolyFilterFn = LTNULL;
	iq.m_pUserData	= m_hObject;

	return g_pLTServer->IntersectSegment(&iq, pInfo);
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CAIChameleon::CanBlendWith()
//
//	PURPOSE:	Whether the chameleon can take on a surface's texture
//
// ----------------------------------------------------------------------- //

// PH: 0x10148f70
LTBOOL CAIChameleon::CanBlendWith(SurfaceType eSurface)
{
	// PH: surface ids from the surface butes (surface.txt), which no enum
	// names
	switch (eSurface)
	{
		case 10:
		case 11:
		case 70:
		case 71:
		case 72:
		case 73:
			return LTTRUE;
	}

	return LTFALSE;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CAIChameleon::UpdateSurface()
//
//	PURPOSE:	Every half second, when the player is near, take on the
//				texture of the surface below (or failing that, behind)
//
// ----------------------------------------------------------------------- //

// PH: 0x10148fe0
void CAIChameleon::UpdateSurface(LTFLOAT fFrameTime)
{
	if (m_eCamoState == eCamoFading) return;
	if (g_pLTServer->GetTime() < m_fNextSurfaceCheck) return;

	m_fNextSurfaceCheck = g_pLTServer->GetTime() + 0.5f;

	CCharacterMgr::PlayerIterator iter = g_pCharacterMgr->BeginPlayers();
	if (iter == g_pCharacterMgr->EndPlayers()) return;

	CPlayerObj* pPlayer = *iter;
	if (!pPlayer) return;

	LTVector vPlayerPos, vPos;
	g_pLTServer->GetObjectPos(pPlayer->m_hObject, &vPlayerPos);
	g_pLTServer->GetObjectPos(m_hObject, &vPos);

	LTVector vDir = vPos - vPlayerPos;
	if (vDir.MagSqr() > 2048.0f * 2048.0f) return;

	IntersectInfo ii;
	vDir.Norm();

	if (FindSurface(vPos, vDir, 2048.0f, &ii) != LTTRUE) return;
	if (ii.m_hPoly == INVALID_HPOLY) return;

	if (!CanBlendWith(GetSurfaceType(ii.m_hPoly)))
	{
		LTVector vDown(0.0f, -1.0f, 0.0f);
		if (!FindSurface(vPos, vDown, 256.0f, &ii)) return;
		if (!CanBlendWith(GetSurfaceType(ii.m_hPoly))) return;
	}

	char szTexture[MAX_CS_FILENAME_LEN + 1 + 132];
	if (g_pLTServer->GetPolyTextureName(ii.m_hPoly, szTexture, 0x104) != LT_OK) return;
	if (!szTexture[0]) return;

	TakeTexture(szTexture);
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CAIChameleon::TakeTexture()
//
//	PURPOSE:	Take on a texture
//
// ----------------------------------------------------------------------- //

// PH: 0x10149240
void CAIChameleon::TakeTexture(const char* szTexture)
{
	if (!szTexture) return;
	if (_stricmp(m_szTexture, szTexture) == 0) return;

	SAFE_STRCPY(m_szTexture, szTexture);

	ObjectCreateStruct ocs;
	g_pLTServer->GetModelFilenames2(m_hObject, &ocs);

	if (m_eCamoState == eCamoOff)
	{
		SAFE_STRCPY(ocs.m_ExtraTextureName, szTexture);
		m_eCamoState = eCamoOn;
	}
	else if (m_eCamoState == eCamoOn)
	{
		SAFE_STRCPY(ocs.m_SkinNames[0], szTexture);
		SAFE_STRCPY(ocs.m_SkinNames[1], szTexture);
		m_eCamoState = eCamoOff;
	}

	g_pLTServer->Common()->SetObjectFilenames(m_hObject, &ocs);

	m_bCamoDone	= LTFALSE;
	m_fCamoTime	= 0.0f;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CAIChameleon::FinishCamo()
//
//	PURPOSE:	The blend to the new texture is done
//
// ----------------------------------------------------------------------- //

// PH: 0x10149340
void CAIChameleon::FinishCamo()
{
	m_bCamoDone = LTTRUE;

	if (m_eCamoState == eCamoFading)
	{
		uint32 dwFlags, dwFlags2;
		g_pLTServer->Common()->GetObjectFlags(m_hObject, OFT_Flags, dwFlags);
		g_pLTServer->Common()->GetObjectFlags(m_hObject, OFT_Flags2, dwFlags2);
		g_pLTServer->Common()->SetObjectFlags(m_hObject, OFT_Flags, dwFlags & ~FLAG_ENVIRONMENTMAP);
		g_pLTServer->Common()->SetObjectFlags(m_hObject, OFT_Flags2, dwFlags2 & ~FLAG2_CAMOBLEND);
	}
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CAIChameleon::UpdateCamo()
//
//	PURPOSE:	Blend between the two textures over half a second
//
// ----------------------------------------------------------------------- //

// PH: 0x101493d0
void CAIChameleon::UpdateCamo(LTFLOAT fFrameTime)
{
	if (m_bCamoDone == LTTRUE) return;

	LTFLOAT fBlend;
	m_fCamoTime += fFrameTime;
	if (m_fCamoTime >= 0.5f)
	{
		fFrameTime = 1.0f;
		fBlend = 0.0f;
		FinishCamo();
	}
	else
	{
		fFrameTime = m_fCamoTime * 2.0f;
		fBlend = 1.0f - fFrameTime;
	}

	LTFLOAT r, g, b, a;
	g_pLTServer->GetObjectColor(m_hObject, &r, &g, &b, &a);

	// The blend between the two skins goes in red and alpha
	if (m_eCamoState == eCamoOn)
	{
		g_pLTServer->SetObjectColor(m_hObject, fFrameTime, g, b, fBlend);
	}
	else
	{
		g_pLTServer->SetObjectColor(m_hObject, fBlend, g, b, fFrameTime);
	}
}

// PH: 0x101494b0
void CAIChameleon::ResetHeadSkin()
{
	m_bResetHeadSkin = LTTRUE;
	SAFE_STRCPY(m_szTexture, "skins/exp/characters/chameleonhead.dtx");
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CAIChameleon::UpdateUncamo()
//
//	PURPOSE:	Drop the camouflage and go back to its own skin
//
// ----------------------------------------------------------------------- //

// PH: 0x101494e0
void CAIChameleon::UpdateUncamo()
{
	if (!m_bResetHeadSkin) return;
	if (!m_bCamoDone) return;
	if (m_eCamoState == eCamoFading) return;

	m_bResetHeadSkin = LTFALSE;

	ObjectCreateStruct ocs;
	g_pLTServer->GetModelFilenames2(m_hObject, &ocs);

	if (m_eCamoState == eCamoOff)
	{
		SAFE_STRCPY(ocs.m_ExtraTextureName, ocs.m_SkinNames[0]);
	}

	SAFE_STRCPY(ocs.m_SkinNames[0], "skins/exp/characters/chameleonhead.dtx");
	SAFE_STRCPY(ocs.m_SkinNames[1], "skins/exp/characters/chameleontorso.dtx");

	g_pLTServer->Common()->SetObjectFilenames(m_hObject, &ocs);
	g_pLTServer->SetObjectColor(m_hObject, 1.0f, 1.0f, 1.0f, 0.0f);

	uint32 dwFlags2;
	g_pLTServer->Common()->GetObjectFlags(m_hObject, OFT_Flags2, dwFlags2);
	g_pLTServer->Common()->SetObjectFlags(m_hObject, OFT_Flags2, dwFlags2 & ~FLAG2_CAMOBLEND);

	m_eCamoState	= eCamoFading;
	m_bCamoDone		= LTFALSE;
	m_fCamoTime		= 0.0f;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CAIChameleon::UpdateIdle()
//
//	PURPOSE:	Crouch, mutter, and blend in
//
// ----------------------------------------------------------------------- //

// PH: 0x10149630
void CAIChameleon::UpdateIdle(LTFLOAT fFrameTime)
{
	CAIBase::UpdateIdle(fFrameTime);

	switch (m_bCrouchIdle)
	{
		case LTFALSE:
			UpdateToCrouch(fFrameTime);
			break;

		case LTTRUE:
			UpdateCrouchIdle(fFrameTime);
			break;
	}

	if (g_pLTServer->GetTime() >= m_tmrIdleSound.m_fTime)
	{
		LTFLOAT fNow = g_pLTServer->GetTime();
		LTFLOAT fDelay = GetRandom(4.0f, 16.0f);
		m_tmrIdleSound.m_fTime = fNow + fDelay;
		m_Sounds.PlayVoice(s_aszSounds[GetRandom(eSoundIdle1, eSoundIdle2)]);
	}

	UpdateCamo(fFrameTime);
	UpdateUncamo();
	UpdateSurface(fFrameTime);
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CAIChameleon::UpdateAlert()
//
//	PURPOSE:	Stalk and attack the target
//
// ----------------------------------------------------------------------- //

// PH: 0x10149700
void CAIChameleon::UpdateAlert(LTFLOAT fFrameTime)
{
	CAIBase::UpdateAlert(fFrameTime);

	switch (m_eAttackState)
	{
		case eAttackAlert:
			UpdateAttackAlert(fFrameTime);
			break;

		case eAttackChase:
			UpdateChase(fFrameTime);
			break;

		case eAttackSwipe:
			UpdateSwipe(fFrameTime);
			break;

		case eAttackLeap:
			UpdateLeap(fFrameTime);
			break;

		case eAttackPounce:
			UpdatePounce(fFrameTime);
			break;
	}

	if (m_eState != eStateAlert) return;
	if (m_eAttackState == eAttackPounce || m_eAttackState == eAttackLeap) return;

	LTVector vTargetPos;
	g_pLTServer->GetObjectPos(m_hTarget, &vTargetPos);

	m_MoveMgr.m_vDest		= vTargetPos;
	m_OrientMgr.m_bFacePos	= LTTRUE;
	m_OrientMgr.m_vFacePos	= vTargetPos;

	LTVector vDir = vTargetPos - m_MoveMgr.GetPos(m_hObject);
	if (!(m_TargetDist.m_nAxes & 1)) vDir.x = 0.0f;
	if (!(m_TargetDist.m_nAxes & 2)) vDir.y = 0.0f;
	if (!(m_TargetDist.m_nAxes & 4)) vDir.z = 0.0f;

	m_TargetDist.m_fDist = vDir.Mag();
	if (m_TargetDist.m_fDist < m_TargetDist.m_fMin)
	{
		m_TargetDist.m_fMin = m_TargetDist.m_fDist;
	}
	if (m_TargetDist.m_fDist > m_TargetDist.m_fMax)
	{
		m_TargetDist.m_fMax = m_TargetDist.m_fDist;
	}

	UpdateCamo(fFrameTime);
	UpdateUncamo();
	UpdateSurface(fFrameTime);

	if (m_eAttackState == eAttackAlert) return;
	if (ReturnToLeash() == LTTRUE) return;

	CLeashNode* pNode = m_Leash.FindNode(vTargetPos);
	if (!pNode)
	{
		m_Leash.m_bOutside = LTFALSE;

		AIBaseStateInfo home(eStateMoving, 0);
		home.vPos = m_Leash.m_vHomePos;
		if (!SetState(&home))
		{
			AIBaseStateInfo idle(eStateIdle);
			SetState(&idle);
		}
	}
	else if (pNode->m_nID != m_Leash.m_nCurNode)
	{
		AIBaseStateInfo follow(eStateIdle);
		follow.eState	= eStateMoving;
		follow.vPos		= vTargetPos;
		follow.nParam	= 1;
		if (!SetState(&follow))
		{
			AIBaseStateInfo idle(eStateIdle);
			SetState(&idle);
		}
	}
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CAIChameleon::SetStateIdle()
//
//	PURPOSE:	Go idle
//
// ----------------------------------------------------------------------- //

// PH: 0x101499f0
LTBOOL CAIChameleon::SetStateIdle(const AIBaseStateInfo* pInfo)
{
	CAIBase::SetStateIdle(pInfo);

	if (m_Animator.GetAnim(0) != m_Animator.FindAnim("Crouch"))
	{
		StartToCrouch();
	}
	else
	{
		StartCrouchIdle();
	}

	m_OrientMgr.m_bFacePos	= LTFALSE;
	m_MoveMgr.m_eMoveType	= 1;

	m_Stare.m_fMaxTime	= 10.0f;
	m_Stare.m_fFOV	= 0.5183628f;
	m_Stare.m_fTime		= 0.0f;

	LTFLOAT fNow = g_pLTServer->GetTime();
	LTFLOAT fDelay = GetRandom(4.0f, 16.0f);
	m_tmrIdleSound.m_fTime = fNow + fDelay;

	m_hTarget = LTNULL;

	return LTTRUE;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CAIChameleon::SetStateAlert()
//
//	PURPOSE:	Go after the target
//
// ----------------------------------------------------------------------- //

// PH: 0x10149af0
LTBOOL CAIChameleon::SetStateAlert(const AIBaseStateInfo* pInfo)
{
	CAIBase::SetStateAlert(pInfo);

	if (IsTargetGone() == LTTRUE) return LTFALSE;

	LTVector vTargetPos;
	g_pLTServer->GetObjectPos(m_hTarget, &vTargetPos);

	m_MoveMgr.m_vDest		= vTargetPos;
	m_MoveMgr.m_fMinSpeed	= 380.0f;
	m_OrientMgr.m_vFacePos	= vTargetPos;
	m_OrientMgr.m_bFacePos	= LTTRUE;

	m_TargetDist.m_fMin		= 9999999.0f;
	m_TargetDist.m_fMax		= -9999999.0f;
	m_TargetDist.m_fDist	= 0.0f;
	m_TargetDist.m_nAxes	= 7;

	m_tmrLeap.m_fTime = g_pLTServer->GetTime();

	if (m_Animator.GetAnim(0) == m_Animator.FindAnim("Crouch"))
	{
		StartAlert();
	}
	else if (InSwipeRange() == LTTRUE)
	{
		StartSwipe();
	}
	else if (CanLeap() == LTTRUE)
	{
		StartPounce();
	}
	else
	{
		StartChase();
	}

	if (m_eCamoState != eCamoFading)
	{
		ResetHeadSkin();
	}

	return LTTRUE;
}

// PH: 0x10149c30
LTBOOL CAIChameleon::SetStatePain(const AIBaseStateInfo* pInfo)
{
	if (m_eState == eStateAlert && (m_eAttackState == eAttackLeap || m_eAttackState == eAttackPounce))
	{
		return LTFALSE;
	}

	if (m_eState == eStateIdle) return LTFALSE;
	if (!m_MoveMgr.m_bMoving) return LTFALSE;

	return CAIBase::SetStatePain(pInfo);
}

// PH: 0x10149c80
LTBOOL CAIChameleon::SetStateDead(const AIBaseStateInfo* pInfo)
{
	LTVector vDims;
	g_pLTServer->Physics()->GetObjectDims(m_hObject, &vDims);
	vDims.x += 24.0f;
	vDims.z += 24.0f;
	g_pLTServer->Physics()->SetObjectDims(m_hObject, &vDims, SETDIMS_PUSHOBJECTS);

	return CAIBase::SetStateDead(pInfo);
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CAIChameleon::Start...()
//
//	PURPOSE:	Start each part of the attack
//
// ----------------------------------------------------------------------- //

// PH: 0x10149cf0
LTBOOL CAIChameleon::StartAlert()
{
	m_eAttackState = eAttackAlert;

	m_Animator.PlayAnim("Alert", LTTRUE, LTFALSE);
	m_tmrAlert.Start(m_Animator.GetAnimLength(0));

	m_MoveMgr.m_eMoveType = 1;
	m_Sounds.PlayAlert(s_aszSounds[eSoundSight]);
	m_StuckCheck.m_bEnabled = LTFALSE;

	return LTTRUE;
}

// PH: 0x10149d70
LTBOOL CAIChameleon::StartChase()
{
	m_eAttackState = eAttackChase;

	LTVector vTargetPos;
	g_pLTServer->GetObjectPos(m_hTarget, &vTargetPos);

	CLeashNode* pNode = m_Leash.FindNode(vTargetPos);
	if (pNode && pNode->m_nID == m_Leash.m_nCurNode)
	{
		m_Animator.PlayAnim("Run", LTTRUE, LTFALSE);
		m_MoveMgr.m_eMoveType = 2;
		m_StuckCheck.m_bEnabled = LTTRUE;
		return LTTRUE;
	}

	AIBaseStateInfo follow(eStateIdle);
	follow.eState	= eStateMoving;
	follow.vPos		= vTargetPos;
	follow.nParam	= 1;
	if (!SetState(&follow))
	{
		m_Leash.m_bOutside = LTFALSE;

		AIBaseStateInfo home(eStateMoving, 0);
		home.vPos = m_Leash.m_vHomePos;
		if (!SetState(&home))
		{
			AIBaseStateInfo idle(eStateIdle);
			SetState(&idle);
		}
	}

	return LTFALSE;
}

// PH: 0x10149ec0
LTBOOL CAIChameleon::StartSwipe()
{
	m_eAttackState = eAttackSwipe;

	m_Animator.PlayAnim("Attack0", LTTRUE, LTFALSE);
	m_tmrSwipe.Start(m_Animator.GetAnimLength(0));

	m_MoveMgr.m_eMoveType = 1;
	m_Sounds.PlayVoice(s_aszSounds[eSoundAttack]);
	m_StuckCheck.m_bEnabled = LTFALSE;

	return LTTRUE;
}

// PH: 0x10149f40
LTBOOL CAIChameleon::StartPounce()
{
	m_eAttackState = eAttackPounce;

	m_Animator.PlayAnim("Attack1", LTFALSE, LTFALSE);
	m_Animator.GetAnimLength(0);	// PH: unused (0x10149f68)
	m_tmrPounce.Start(0.7f);

	m_MoveMgr.m_eMoveType = 1;
	m_StuckCheck.m_bEnabled = LTFALSE;
	m_Sounds.PlayVoice(s_aszSounds[eSoundAttack]);

	return LTTRUE;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CAIChameleon::StartLeap()
//
//	PURPOSE:	Leap at the target
//
// ----------------------------------------------------------------------- //

// PH: 0x10149fc0
LTBOOL CAIChameleon::StartLeap()
{
	if (!m_hTarget) return LTFALSE;

	m_eAttackState = eAttackLeap;
	m_MoveMgr.m_eMoveType = 8;

	LTVector vTargetPos;
	g_pLTServer->GetObjectPos(m_hTarget, &vTargetPos);

	if (!m_Leash.FindNode(vTargetPos))
	{
		LTFLOAT fNow = g_pLTServer->GetTime();
		LTFLOAT fDelay = GetRandom(2.0f, 8.0f);
		m_tmrNextLeap.m_fTime = fNow + fDelay;
		return LTFALSE;
	}

	LTVector vTargetVel;
	g_pLTServer->Physics()->GetVelocity(m_hTarget, &vTargetVel);

	LTVector vDir = vTargetPos - m_MoveMgr.GetPos(m_hObject);
	LTFLOAT fDist = vDir.Mag();
	vDir *= 1.0f / fDist;

	LTVector vVel = vDir * ((fDist - 192.0f) * 2.0f);

	LTVector vGravity;
	g_pLTServer->Physics()->GetGlobalForce(vGravity);
	vVel.y = (96.0f - vGravity.y * 0.03125f) * 4.0f;

	g_pLTServer->Physics()->SetVelocity(m_hObject, &vVel);

	vTargetPos += vDir * -192.0f;
	m_MoveMgr.m_vDest		= vTargetPos;
	m_OrientMgr.m_vFacePos	= vTargetPos;

	m_tmrLeap.Start(0.5f);
	m_StuckCheck.m_bEnabled = LTFALSE;

	return LTTRUE;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CAIChameleon::Update...()
//
//	PURPOSE:	Update each part of the attack
//
// ----------------------------------------------------------------------- //

// PH: 0x1014a240
void CAIChameleon::UpdateAttackAlert(LTFLOAT fFrameTime)
{
	m_MoveMgr.m_eMoveType = 1;

	if (m_Animator.GetAnim(0) == m_Animator.FindAnim("Alert"))
	{
		if (g_pLTServer->GetTime() < m_tmrAlert.m_fTime && !(m_Animator.GetPlaybackState(0) & MS_PLAYDONE))
		{
			return;
		}
	}

	if (InSwipeRange() == LTTRUE)
	{
		StartSwipe();
	}
	else if (CanLeap() == LTTRUE)
	{
		StartPounce();
	}
	else
	{
		StartChase();
	}
}

// PH: 0x1014a2e0
void CAIChameleon::UpdateChase(LTFLOAT fFrameTime)
{
	m_MoveMgr.m_eMoveType = 2;

	if (InSwipeRange() == LTTRUE)
	{
		StartSwipe();
		return;
	}

	if (CanLeap() == LTTRUE)
	{
		StartPounce();
		return;
	}

	LTVector vTargetPos;
	g_pLTServer->GetObjectPos(m_hTarget, &vTargetPos);

	CLeashNode* pNode = m_Leash.FindNode(vTargetPos);
	if (pNode && pNode->m_nID == m_Leash.m_nCurNode) return;

	AIBaseStateInfo follow(eStateIdle);
	follow.eState	= eStateMoving;
	follow.vPos		= vTargetPos;
	follow.nParam	= 1;
	if (!SetState(&follow))
	{
		m_Leash.m_bOutside = LTFALSE;

		AIBaseStateInfo home(eStateMoving, 0);
		home.vPos = m_Leash.m_vHomePos;
		if (!SetState(&home))
		{
			AIBaseStateInfo idle(eStateIdle);
			SetState(&idle);
		}
	}
}

// PH: 0x1014a430
void CAIChameleon::UpdateSwipe(LTFLOAT fFrameTime)
{
	m_MoveMgr.m_eMoveType = 1;

	AIBaseStateInfo idle;

	if (g_pLTServer->GetTime() < m_tmrSwipe.m_fTime && !(m_Animator.GetPlaybackState(0) & MS_PLAYDONE))
	{
		return;
	}

	if (IsTargetGone() == LTTRUE)
	{
		SetState(&idle);
	}
	else if (InSwipeRange() == LTTRUE)
	{
		StartSwipe();
	}
	else if (CanLeap() == LTTRUE)
	{
		StartPounce();
	}
	else
	{
		StartChase();
	}
}

// PH: 0x1014a510
void CAIChameleon::UpdatePounce(LTFLOAT fFrameTime)
{
	AIBaseStateInfo idle(eStateIdle);

	if (IsTargetGone() == LTTRUE)
	{
		SetState(&idle);
		return;
	}

	if (g_pLTServer->GetTime() < m_tmrPounce.m_fTime) return;
	if (StartLeap()) return;
	if (StartChase()) return;

	AIBaseStateInfo home(eStateIdle);
	home.eState	= eStateMoving;
	home.vPos	= m_Leash.m_vHomePos;
	home.nParam	= 0;
	SetState(&home);
}

// PH: 0x1014a5f0
void CAIChameleon::UpdateLeap(LTFLOAT fFrameTime)
{
	m_MoveMgr.m_eMoveType = 8;

	AIBaseStateInfo idle;

	LTVector vTargetPos;
	g_pLTServer->GetObjectPos(m_hTarget, &vTargetPos);
	m_MoveMgr.m_vDest		= vTargetPos;
	m_OrientMgr.m_vFacePos	= vTargetPos;

	if (g_pLTServer->GetTime() < m_tmrLeap.m_fTime && !(m_Animator.GetPlaybackState(0) & MS_PLAYDONE))
	{
		if (m_tmrLeap.GetTimeLeft() >= 0.25f) return;
		if (m_MoveMgr.m_bMoving != LTTRUE) return;
	}

	if (IsTargetGone() == LTTRUE)
	{
		SetState(&idle);
		return;
	}

	LTFLOAT fNow = g_pLTServer->GetTime();
	LTFLOAT fDelay = GetRandom(2.0f, 8.0f);
	m_tmrNextLeap.m_fTime = fNow + fDelay;

	if (InSwipeRange() == LTTRUE)
	{
		StartSwipe();
	}
	else if (CanLeap() == LTTRUE)
	{
		StartPounce();
	}
	else
	{
		StartChase();
	}
}

// PH: 0x1014a7a0
LTBOOL CAIChameleon::InSwipeRange()
{
	LTVector vTargetPos;
	g_pLTServer->GetObjectPos(m_hTarget, &vTargetPos);

	LTVector vDir = vTargetPos - m_MoveMgr.GetPos(m_hObject);
	if (vDir.Mag() < m_fSwipeRange)
	{
		return LTTRUE;
	}

	return LTFALSE;
}

// PH: 0x1014a850
LTBOOL CAIChameleon::CanLeap()
{
	if (g_pLTServer->GetTime() < m_tmrNextLeap.m_fTime || !m_MoveMgr.m_bMoving) return LTFALSE;

	LTVector vTargetPos;
	g_pLTServer->GetObjectPos(m_hTarget, &vTargetPos);

	LTVector vDir = vTargetPos - m_MoveMgr.GetPos(m_hObject);
	LTFLOAT fDist = vDir.Mag();
	if (fDist < m_fLeapMinDist || fDist > m_fLeapMaxDist) return LTFALSE;

	if (!m_Leash.FindNode(vTargetPos))
	{
		LTFLOAT fNow = g_pLTServer->GetTime();
		LTFLOAT fDelay = GetRandom(2.0f, 8.0f);
		m_tmrNextLeap.m_fTime = fNow + fDelay;
		return LTFALSE;
	}

	return LTTRUE;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CAIChameleon::HandleDamage()
//
//	PURPOSE:	Turn on whoever hurt it
//
// ----------------------------------------------------------------------- //

// PH: 0x1014a990
void CAIChameleon::HandleDamage(const DamageStruct& damage)
{
	CAIBase::HandleDamage(damage);

	if (m_eState == eStateDying || m_eState == eStateDead) return;

	GetRandom();
	m_Sounds.PlayPain(s_aszSounds[eSoundPain]);

	if (m_hTarget && m_eState != eStateIdle && m_eState != eStateMoving) return;
	if (!CanReact(damage)) return;

	if (IsPlayer(damage.hDamager) == LTTRUE ||
		IsAI(damage.hDamager) == LTTRUE ||
		IsAIBaseOrTentacle(damage.hDamager) == LTTRUE)
	{
		SetTarget(damage.hDamager);

		AIBaseStateInfo alert(eStateAlert, 1);
		SetState(&alert);
	}
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	ChameleonSwipeFilterFn()
//
//	PURPOSE:	Let the swipe's ray stop at anything but the world and the
//				chameleon itself
//
// ----------------------------------------------------------------------- //

// PH: 0x1014aad0
LTBOOL ChameleonSwipeFilterFn(HOBJECT hObj, void *pUserData)
{
	ILTPhysics* pPhysics = g_pLTServer->Physics();
	if (!pPhysics || !hObj) return LTFALSE;

	if (pPhysics->IsWorldObject(hObj) == LTTRUE) return LTFALSE;

	uint32 dwType;
	g_pLTServer->Common()->GetObjectType(hObj, &dwType);
	if (dwType == OT_WORLDMODEL) return LTFALSE;

	return hObj != (HOBJECT)pUserData;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CAIChameleon::Swipe()
//
//	PURPOSE:	The melee attack's damage
//
// ----------------------------------------------------------------------- //

// PH: 0x1014ab20
LTBOOL CAIChameleon::Swipe()
{
	LTBOOL bInFront = LTFALSE;

	LTVector vTargetPos(0.0f, 0.0f, 0.0f);
	LTVector vForward = m_OrientMgr.m_vForward;

	if (m_hTarget)
	{
		g_pLTServer->GetObjectPos(m_hTarget, &vTargetPos);

		LTVector vDir = vTargetPos - m_MoveMgr.GetPos(m_hObject);
		LTFLOAT fMag = vDir.Mag();
		if (fMag != 0.0f)
		{
			vDir *= 1.0f / fMag;
		}

		if (vForward.Dot(vDir) > 0.866f)
		{
			vForward = vDir;
		}
		else
		{
			LTVector vDims, vPos;
			g_pLTServer->Physics()->GetObjectDims(m_hObject, &vDims);
			g_pLTServer->GetObjectPos(m_hObject, &vPos);

			LTVector vMin = vPos - vDims;
			LTVector vMax = vPos + vDims;

			LTVector vTargetDims;
			g_pLTServer->Physics()->GetObjectDims(m_hTarget, &vTargetDims);

			LTVector vTargetMin = vTargetPos - vTargetDims;
			LTVector vTargetMax = vTargetPos + vTargetDims;

			// RETAIL BUG: no absolute value is below zero, so the target is
			// never taken as standing on the chameleon, and the swipe
			// always casts its ray (0x1014ad61)
			if (vTargetMax.x > vMin.x && vTargetMax.z > vMin.z &&
				vTargetMin.x < vMax.x && vTargetMin.z < vMax.z &&
				(LTFLOAT)fabs(vTargetMin.y - vMax.y) < 0.0)
			{
				bInFront = LTTRUE;
			}
		}
	}

	IntersectQuery iq;
	IntersectInfo ii;

	iq.m_From		= m_MoveMgr.GetPos(m_hObject);
	iq.m_To			= iq.m_From + vForward * m_fSwipeRange;
	iq.m_FilterFn	= ChameleonSwipeFilterFn;
	iq.m_Flags		= INTERSECT_OBJECTS | IGNORE_NONSOLID;
	iq.m_PolyFilterFn = LTNULL;
	iq.m_pUserData	= m_hObject;

	if (bInFront == LTTRUE)
	{
		ii.m_Point		= vTargetPos;
		ii.m_hObject	= m_hTarget;
		ii.m_Plane.m_Normal = LTVector(0.0f, -1.0f, 0.0f);
		ii.m_Plane.m_Dist = 0.0f;
		ii.m_hPoly		= INVALID_HPOLY;
	}
	else if (g_pLTServer->IntersectSegment(&iq, &ii) != LTTRUE || !ii.m_hObject)
	{
		return LTFALSE;
	}

	HOBJECT hHit;
	if (IsGameBase(ii.m_hObject) == LTTRUE)
	{
		CCharacterHitBox* pHitBox = (CCharacterHitBox*)g_pLTServer->HandleToObject(ii.m_hObject);
		if (!pHitBox) return LTFALSE;

		hHit = pHitBox->GetModelObject();
	}
	else
	{
		hHit = ii.m_hObject;
	}

	LTFLOAT fDamage = GetRandom(1.0f, 2.0f) * 20.0f;
	DamageObject(hHit, &vForward, fDamage, 0.0f, DT_SLICE);

	return LTTRUE;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CAIChameleon::StartToCrouch()...
//
//	PURPOSE:	The idle's crouch
//
// ----------------------------------------------------------------------- //

// PH: 0x1014b050
void CAIChameleon::StartToCrouch()
{
	m_bCrouchIdle = LTFALSE;

	m_Animator.PlayAnim("ToCrouch", LTTRUE, LTFALSE);
	m_tmrToCrouch.Start(m_Animator.GetAnimLength(0));

	m_eCamoState = eCamoOff;

	{
		uint32 dwFlags;
		g_pLTServer->Common()->GetObjectFlags(m_hObject, OFT_Flags, dwFlags);
		g_pLTServer->Common()->SetObjectFlags(m_hObject, OFT_Flags, dwFlags | FLAG_ENVIRONMENTMAP);
	}
}

// PH: 0x1014b0e0
void CAIChameleon::StartCrouchIdle()
{
	m_bCrouchIdle = LTTRUE;
	m_Animator.PlayAnim("Crouch", LTTRUE, LTFALSE);
	m_OrientMgr.m_bFacePos = LTFALSE;
}

// PH: 0x1014b110
void CAIChameleon::UpdateToCrouch(LTFLOAT fFrameTime)
{
	if (g_pLTServer->GetTime() < m_tmrToCrouch.m_fTime && !(m_Animator.GetPlaybackState(0) & MS_PLAYDONE))
	{
		return;
	}

	StartCrouchIdle();
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CAIChameleon::UpdateCrouchIdle()
//
//	PURPOSE:	Wait, crouched, until the player comes close, or has
//				stared at it too long
//
// ----------------------------------------------------------------------- //

// PH: 0x1014b150
void CAIChameleon::UpdateCrouchIdle(LTFLOAT fFrameTime)
{
	CPlayerObj* pPlayer = LTNULL;

	CCharacterMgr::PlayerIterator iter = g_pCharacterMgr->BeginPlayers();
	if (iter != g_pCharacterMgr->EndPlayers())
	{
		pPlayer = *iter;
		if (pPlayer)
		{
			LTVector vPlayerPos = pPlayer->GetPosition();
			LTVector vAim = pPlayer->GetAimForward();

			LTVector vDir = m_MoveMgr.GetPos(m_hObject) - vPlayerPos;
			vDir.Norm();

			if ((LTFLOAT)acos(vAim.Dot(vDir)) > m_Stare.m_fFOV)
			{
				m_Stare.m_fTime = 0.0f;
			}
			else
			{
				m_Stare.m_fTime += fFrameTime;
			}
		}
	}

	CheckForPlayer();
	if (ChooseTarget() != LTTRUE) return;

	LTVector vTargetPos;
	g_pLTServer->GetObjectPos(m_hTarget, &vTargetPos);

	LTVector vDir = vTargetPos - m_MoveMgr.GetPos(m_hObject);
	LTFLOAT fDist = vDir.Mag();

	AIBaseStateInfo alert(eStateAlert, 1);

	if (m_bWaitToAttack && fDist >= m_fWaitAttackDist)
	{
		if (!pPlayer) return;

		if (m_fWaitAttackDist * 2.0f > fDist &&
			((m_Stare.m_fTime == 0.0f) ? -1.0f : m_Stare.m_fTime) >= m_Stare.m_fMaxTime * 0.5f)
		{
			SetState(&alert);
			return;
		}

		if (!(m_Sight.m_fRange > fDist)) return;
		if (((m_Stare.m_fTime == 0.0f) ? -1.0f : m_Stare.m_fTime) < m_Stare.m_fMaxTime) return;
	}

	SetState(&alert);
}

// PH: 0x1014b400 (CAIBull's is the same, folded into this one)
LTBOOL CAIChameleon::HasMoveTarget()
{
	return m_NextState.eState != eStateAlert;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CAIChameleon::ReturnToLeash()
//
//	PURPOSE:	When the leash has left the chameleon outside, head home
//
// ----------------------------------------------------------------------- //

// PH: 0x1014b410 (CAIBull's is the same, folded into this one)
LTBOOL CAIChameleon::ReturnToLeash()
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

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CAIChameleon::SetVisionMode()
//
//	PURPOSE:	Put on the skins for the player's vision mode
//
// ----------------------------------------------------------------------- //

// PH: 0x1014b4b0
void CAIChameleon::SetVisionMode(int nMode)
{
	if (nMode == m_nSoundSet) return;

	ObjectCreateStruct ocs;
	g_pLTServer->GetModelFilenames2(m_hObject, &ocs);
	SetSkins(nMode, &ocs, s_aszSkins);
	g_pLTServer->Common()->SetObjectFilenames(m_hObject, &ocs);
}

// PH: 0x1014b5d0
void CAIChameleon::HideHead()
{
	CAIBase::HideHead();

	HMODELPIECE hPiece;
	g_pLTServer->GetModelLT()->GetPiece(m_hObject, "NeckNub", hPiece);
	g_pLTServer->GetModelLT()->SetPieceHideStatus(m_hObject, hPiece, LTFALSE);
}

#endif // PRIMAL_HUNT
