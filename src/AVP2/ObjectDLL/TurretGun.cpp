// ----------------------------------------------------------------------- //
//
// MODULE  : TurretGun.cpp
//
// PURPOSE : CTurretGun - implementation
//
// CREATED : Primal Hunt (Third Law); no source survives, written from
//           PHObject.lto (0x10158a40..0x1015a54b)
//
// ----------------------------------------------------------------------- //

#include "stdafx.h"

#ifdef PRIMAL_HUNT

#include "TurretGun.h"
#include "PlayerObj.h"
#include "ServerUtilities.h"
#include "CharacterFuncs.h"
#include "GameServerShell.h"
#include "ObjectMsgs.h"
#include "iltsoundmgr.h"
#include "AIUtils.h"
#include "SoundTypes.h"

// The turret's own messages to its user's client (Primal Hunt's 0xe1,
// which Shared/MsgIDs.h does not name yet): a byte saying what follows.
#define MID_TURRETGUN				0xe1
#define TURRETGUN_MSG_ACTIVE		1		// byte: manned or not
#define TURRETGUN_MSG_AMMO			4		// object, word: ammo left

// A manned turret gun can be activated to let go of it (ActivateType 10,
// after AT_TRIPOD_PICKUP; Shared/ClientServerShared.h does not name it).
#define AT_TURRETGUN				((ActivateType)10)

#define TURRETGUN_SPINUP_TIME		0.5f
#define TURRETGUN_SPINDOWN_TIME		1.0f
#define TURRETGUN_FIRE_DELAY		0.1f
#define TURRETGUN_USE_DIST			96.0f
#define TURRETGUN_MAX_PITCH			0.5235988f	// 30 degrees

// PH: the $E initializer 0x10158a40
BEGIN_CLASS(CTurretGun)
	ADD_LONGINTPROP(Ammo, 750)
	ADD_REALPROP(Yaw Range, 45.0f)
	ADD_LONGINTPROP(Ammo_Regen, 0)
	ADD_REALPROP(Ammo_Regen_Time, 15.0f)
END_CLASS_DEFAULT(CTurretGun, GameBase, NULL, NULL)

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	IsTurretGun / GetTurretGun
//
// ----------------------------------------------------------------------- //

// PH: 0x10158c80
LTBOOL IsTurretGun(HOBJECT hObj)
{
	HCLASS hTurretGun = g_pLTServer->GetClass("CTurretGun");
	if (!hTurretGun) return LTFALSE;

	HCLASS hClass = g_pLTServer->GetObjectClass(hObj);
	if (!hClass) return LTFALSE;

	return g_pLTServer->IsKindOf(hClass, hTurretGun) == LTTRUE;
}

// PH: 0x10158cd0
CTurretGun* GetTurretGun(HOBJECT hObj)
{
	if (!IsTurretGun(hObj)) return LTNULL;

	return (CTurretGun*)g_pLTServer->HandleToObject(hObj);
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CTurretGun::CTurretGun()
//
// ----------------------------------------------------------------------- //

// PH: 0x10158d00
CTurretGun::CTurretGun() : GameBase(OT_MODEL)
{
	m_nAmmo = 0;
	m_fYawRange = MATH_PI / 4.0f;
	m_fPitchRange = 40.0f * MATH_PI / 180.0f;
	m_hUser = LTNULL;
	m_fYaw = 0.0f;
	m_fPitch = 0.0f;

	AddAggregate(&m_Weapons);

	m_bShooting = LTFALSE;
	m_hLoopFireSound = LTNULL;
	m_hSpinSound = LTNULL;
	m_hStartFireSound = LTNULL;
	m_bSpinning = LTFALSE;
	m_eSpinState = eSpinStopped;
}

// PH: 0x10158e20
CTurretGun::~CTurretGun()
{
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CTurretGun::OnPrecreate()
//
// ----------------------------------------------------------------------- //

// PH: 0x10158ea0
uint32 CTurretGun::OnPrecreate(void *pData, uint32 nInfo)
{
	GameBase::OnPrecreate(pData, nInfo);

	if (nInfo == PRECREATE_WORLDFILE)
	{
		ObjectCreateStruct* pStruct = (ObjectCreateStruct*)pData;

		pStruct->m_ObjectType = OT_MODEL;
		pStruct->m_Flags = FLAG_VISIBLE | FLAG_RAYHIT;

		GenericProp genProp;

		if (g_pLTServer->GetPropGeneric("Yaw Range", &genProp) == LT_OK)
		{
			m_fYawRange = (genProp.m_Float * MATH_PI) * 0.0055555557f;	// degrees to radians
		}

		if (g_pLTServer->GetPropGeneric("Ammo", &genProp) == LT_OK)
		{
			m_nAmmo = genProp.m_Long;
		}

		if (g_pLTServer->GetPropGeneric("Ammo_Regen", &genProp) == LT_OK)
		{
			m_nAmmoRegen = genProp.m_Long;
		}

		if (g_pLTServer->GetPropGeneric("Ammo_Regen_Time", &genProp) == LT_OK)
		{
			m_fAmmoRegenTime = genProp.m_Float;
		}
	}

	return 1;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CTurretGun::OnInitialUpdate()
//
// ----------------------------------------------------------------------- //

// PH: 0x10158fb0
uint32 CTurretGun::OnInitialUpdate(uint32 nInfo)
{
	GameBase::OnInitialUpdate(nInfo);

	LTVector vDims;

	if (nInfo != INITIALUPDATE_SAVEGAME)
	{
		g_pLTServer->GetModelAnimUserDims(m_hObject, &vDims, 0);
		g_pLTServer->SetObjectDims(m_hObject, &vDims);
		g_pLTServer->Physics()->SetObjectMass(m_hObject, 100000.0f);

		SetActivateType(m_hObject, AT_TURRETGUN);

		m_bFiring = LTFALSE;

		WEAPON* pWeapon = g_pWeaponMgr->GetWeapon("Minigun");
		if (pWeapon)
		{
			m_nWeaponId = pWeapon->nId;
		}

		m_Weapons.Init(m_hObject);
		m_Weapons.ObtainWeapon((uint8)m_nWeaponId, BARREL_DEFAULT_ID, AMMO_DEFAULT_ID, m_nAmmo);
		m_Weapons.ChangeWeapon((uint8)m_nWeaponId);

		SetNextUpdate(0.001f);

		m_bMultiplayer = g_pGameServerShell->GetGameType().IsMultiplayer() ? LTTRUE : LTFALSE;

		if (!m_nAmmoRegen)
		{
			m_fNextAmmoRegenTime = m_fAmmoRegenTime = 9999999.0f;
			return 1;
		}

		m_fNextAmmoRegenTime = g_pLTServer->GetTime() + m_fAmmoRegenTime;
	}

	return 1;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CTurretGun::StartSpin()
//
// ----------------------------------------------------------------------- //

// PH: 0x10159120
void CTurretGun::StartSpin()
{
	if (m_eSpinState == eSpinning || m_eSpinState == eSpinningUp) return;

	m_tmrSpinUp.Start(TURRETGUN_SPINUP_TIME);
	m_hStartFireSound = PlaySound("sounds/exp/weapons/marine/turret/startfire.wav", LTFALSE, LTTRUE);
	m_eSpinState = eSpinningUp;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CTurretGun::StopSpin()
//
// ----------------------------------------------------------------------- //

// PH: 0x10159170
void CTurretGun::StopSpin()
{
	if (m_eSpinState == eSpinStopped || m_eSpinState == eSpinningDown) return;

	m_tmrSpinDown.Start(TURRETGUN_SPINDOWN_TIME);

	if (m_hStartFireSound)
	{
		g_pLTServer->KillSound(m_hStartFireSound);
		m_hStartFireSound = LTNULL;
	}

	if (m_hSpinSound)
	{
		g_pLTServer->KillSound(m_hSpinSound);
		m_hSpinSound = LTNULL;
	}

	PlaySound("sounds/exp/weapons/marine/turret/stopfire.wav", LTFALSE, LTFALSE);
	m_eSpinState = eSpinningDown;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CTurretGun::UpdateSpin()
//
// ----------------------------------------------------------------------- //

// PH: 0x10159210
void CTurretGun::UpdateSpin(LTFLOAT fFrameTime)
{
	switch (m_eSpinState)
	{
		case eSpinningUp:
		{
			if (m_tmrSpinUp.IsDone())
			{
				m_eSpinState = eSpinning;
				m_hSpinSound = PlaySound("sounds/exp/weapons/marine/turret/spin.wav", LTTRUE, LTTRUE);
			}
		}
		break;

		case eSpinningDown:
		{
			if (m_hSpinSound)
			{
				g_pLTServer->SoundMgr()->KillSound(m_hSpinSound);
				m_hSpinSound = LTNULL;
			}

			if (m_tmrSpinDown.IsDone())
			{
				m_eSpinState = eSpinStopped;
			}
		}
		break;
	}
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CTurretGun::OnUpdate()
//
// ----------------------------------------------------------------------- //

// PH: 0x101592b0
uint32 CTurretGun::OnUpdate(LTFLOAT fFrameTime)
{
	GameBase::OnUpdate(fFrameTime);

	SetNextUpdate(0.001f);

	CWeapon* pWeapon = m_Weapons.GetCurWeapon();
	if (!pWeapon) return 1;

	// Regenerate ammo...

	if (m_nAmmoRegen > 0 && g_pLTServer->GetTime() >= m_fNextAmmoRegenTime)
	{
		const AMMO* pAmmo = pWeapon->GetAmmoData();
		if (pAmmo)
		{
			if (m_Weapons.GetAmmoCount(pWeapon->GetAmmoId()) < ((AMMO*)pAmmo)->GetMaxPoolAmount(m_hObject))
			{
				m_Weapons.AddAmmo(pWeapon->GetAmmoId(), m_nAmmoRegen);
			}
		}

		m_fNextAmmoRegenTime = g_pLTServer->GetTime() + m_fAmmoRegenTime;
	}

	// Keep our user at the gun...

	if (m_hUser)
	{
		LTVector vUserPos;
		g_pLTServer->GetObjectPos(m_hUser, &vUserPos);

		LTVector vDiff = vUserPos - m_vUserPos;
		if (vDiff.Mag() > 2.0f)
		{
			CPlayerObj* pPlayer = (CPlayerObj*)g_pLTServer->HandleToObject(m_hUser);
			if (pPlayer)
			{
				pPlayer->Teleport(m_vUserPos);
			}
		}
	}

	UpdateSpin(fFrameTime);

	if (m_bFiring)
	{
		if (!m_hLoopFireSound && m_tmrFireStart.IsDone() &&
			m_Weapons.GetAmmoCount(pWeapon->GetAmmoId()) > 0)
		{
			m_hLoopFireSound = PlaySound("sounds/exp/weapons/marine/turret/loopfire.wav", LTTRUE, LTTRUE);
		}

		m_fFireDelay -= fFrameTime;
		if (m_fFireDelay <= 0.0f)
		{
			if (m_Weapons.GetAmmoCount(pWeapon->GetAmmoId()) <= 0)
			{
				// Out of ammo, just spin...

				if (m_hLoopFireSound)
				{
					g_pLTServer->SoundMgr()->KillSound(m_hLoopFireSound);
					m_hLoopFireSound = LTNULL;
				}

				if (!m_hSpinSound)
				{
					m_hSpinSound = PlaySound("sounds/exp/weapons/marine/turret/spin.wav", LTTRUE, LTTRUE);
				}
			}
			else
			{
				LTVector vPos;
				g_pLTServer->GetObjectPos(m_hObject, &vPos);

				LTRotation rRot;
				g_pLTServer->GetObjectRotation(m_hObject, &rRot);

				LTVector vU, vR, vF;
				g_pLTServer->GetRotationVectors(&rRot, &vU, &vR, &vF);
				g_pLTServer->RotateAroundAxis(&rRot, &vR, m_fPitch);

				vU = LTVector(0.0f, 1.0f, 0.0f);
				g_pLTServer->RotateAroundAxis(&rRot, &vU, m_fYaw);
				g_pLTServer->GetRotationVectors(&rRot, &vU, &vR, &vF);

				vPos += vU * 30.0f + vF * 60.0f;

				WFireInfo fireInfo;
				fireInfo.hIgnoreObj		= m_hUser;
				fireInfo.hFiredFrom		= m_hObject;
				fireInfo.vPath			= vF;
				fireInfo.vFirePos		= vPos;
				fireInfo.vFlashPos		= vPos;
				fireInfo.nFlashSocket	= 0;
				fireInfo.nSeed			= GetRandom(2, 255);
				fireInfo.bAltFire		= LTFALSE;
				fireInfo.fPerturbR		= 2.0f;
				fireInfo.fPerturbU		= 2.0f;
				fireInfo.fDamageMultiplier = GetDifficultyFactor() * 10.0f;

				pWeapon->Fire(fireInfo);

				m_fFireDelay = TURRETGUN_FIRE_DELAY;
			}
		}
	}

	// Tell the clients our ammo...

	m_fAmmoMsgDelay -= fFrameTime;
	if (m_fAmmoMsgDelay <= 0.0f)
	{
		int nAmmo = m_Weapons.GetAmmoCount(pWeapon->GetAmmoId());
		if (nAmmo != m_nLastAmmo)
		{
			HMESSAGEWRITE hMessage = g_pLTServer->StartMessage(LTNULL, MID_TURRETGUN);
			g_pLTServer->WriteToMessageByte(hMessage, TURRETGUN_MSG_AMMO);
			g_pLTServer->WriteToMessageObject(hMessage, m_hObject);
			g_pLTServer->WriteToMessageWord(hMessage, nAmmo);
			g_pLTServer->EndMessage(hMessage);

			m_nLastAmmo = nAmmo;
		}

		if (m_bMultiplayer)
		{
			m_fAmmoMsgDelay = 0.25f;
			return 1;
		}

		m_fAmmoMsgDelay = 0.001f;
	}

	return 1;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CTurretGun::OnTrigger()
//
// ----------------------------------------------------------------------- //

// PH: 0x101598e0
uint32 CTurretGun::OnTrigger(HOBJECT hSender, const char* szMsg)
{
	GameBase::OnTrigger(hSender, szMsg);

	if (_stricmp(szMsg, "ACTIVATE") == 0 && IsPlayer(hSender) == LTTRUE)
	{
		if (m_hUser)
		{
			if (m_hUser == hSender)
			{
				Deactivate();
				return 1;
			}
		}
		else
		{
			Activate(hSender);
		}
	}

	return 1;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CTurretGun::OnLoad()
//
// ----------------------------------------------------------------------- //

// PH: 0x10159960
void CTurretGun::OnLoad(HMESSAGEREAD hRead)
{
	GameBase::OnLoad(hRead);

	m_nAmmo				= hRead->ReadDWord();
	m_nLastAmmo			= hRead->ReadDWord();
	m_hUser				= hRead->ReadObject();
	m_bFiring			= hRead->ReadByte();
	m_fFireDelay		= hRead->ReadFloat();
	m_nWeaponId			= hRead->ReadDWord();
	hRead->ReadVectorFL(m_vUserPos);
	m_tmrSpinUp.m_fTime		= hRead->ReadFloat();
	m_fYawRange			= hRead->ReadFloat();
	m_fPitchRange		= hRead->ReadFloat();
	m_fYaw				= hRead->ReadFloat();
	m_fPitch			= hRead->ReadFloat();
	m_nAmmoRegen		= hRead->ReadDWord();
	m_fAmmoRegenTime	= hRead->ReadFloat();
	m_fNextAmmoRegenTime = hRead->ReadFloat();
	m_bSpinning			= hRead->ReadByte();
	m_eSpinState		= hRead->ReadByte();
	m_tmrSpinDown.m_fTime		= hRead->ReadFloat();
	m_tmrFireStart.m_fTime	= hRead->ReadFloat();

	m_hLoopFireSound	= LTNULL;
	m_hSpinSound		= LTNULL;
	m_hStartFireSound	= LTNULL;
	m_bShooting			= LTFALSE;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CTurretGun::OnSave()
//
// ----------------------------------------------------------------------- //

// PH: 0x10159b50
void CTurretGun::OnSave(HMESSAGEWRITE hWrite)
{
	GameBase::OnSave(hWrite);

	hWrite->WriteDWord(m_nAmmo);
	hWrite->WriteDWord(m_nLastAmmo);
	hWrite->WriteObject(m_hUser);
	hWrite->WriteByte(m_bFiring);
	hWrite->WriteFloat(m_fFireDelay);
	hWrite->WriteDWord(m_nWeaponId);
	hWrite->WriteVector(m_vUserPos);
	hWrite->WriteFloat(m_tmrSpinUp.m_fTime);
	hWrite->WriteFloat(m_fYawRange);
	hWrite->WriteFloat(m_fPitchRange);
	hWrite->WriteFloat(m_fYaw);
	hWrite->WriteFloat(m_fPitch);
	hWrite->WriteDWord(m_nAmmoRegen);
	hWrite->WriteFloat(m_fAmmoRegenTime);
	hWrite->WriteFloat(m_fNextAmmoRegenTime);
	hWrite->WriteByte(m_bSpinning);
	hWrite->WriteByte(m_eSpinState);
	hWrite->WriteFloat(m_tmrSpinDown.m_fTime);
	hWrite->WriteFloat(m_tmrFireStart.m_fTime);
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CTurretGun::SetFiring()
//
// ----------------------------------------------------------------------- //

// PH: 0x10159c70
void CTurretGun::SetFiring(LTBOOL bFiring)
{
	m_bFiring = bFiring;

	CWeapon* pWeapon = m_Weapons.GetCurWeapon();
	if (!pWeapon) return;

	if (bFiring == LTTRUE)
	{
		if (m_bShooting) return;

		if (m_eSpinState == eSpinning)
		{
			if (!m_hLoopFireSound && m_Weapons.GetAmmoCount(pWeapon->GetAmmoId()) > 0)
			{
				m_hLoopFireSound = PlaySound("sounds/exp/weapons/marine/turret/loopfire.wav", LTTRUE, LTTRUE);
			}

			m_tmrFireStart.Start();
			m_fFireDelay = 0.0f;
			m_bShooting = LTTRUE;
		}
		else
		{
			StartSpin();

			m_tmrFireStart.Start(TURRETGUN_SPINUP_TIME);
			m_fFireDelay = TURRETGUN_SPINUP_TIME;
			m_bShooting = LTTRUE;
		}
	}
	else if (m_bShooting == LTTRUE)
	{
		if (m_hLoopFireSound)
		{
			g_pLTServer->SoundMgr()->KillSound(m_hLoopFireSound);
			m_hLoopFireSound = LTNULL;
		}

		m_bShooting = LTFALSE;

		if (!m_bSpinning)
		{
			StopSpin();
		}
	}
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CTurretGun::SetSpinning()
//
// ----------------------------------------------------------------------- //

// PH: 0x10159dd0
void CTurretGun::SetSpinning(LTBOOL bSpinning)
{
	m_bSpinning = bSpinning;

	if (bSpinning == LTTRUE)
	{
		StartSpin();
	}
	else if (!m_bFiring)
	{
		StopSpin();
	}
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CTurretGun::Activate()
//
//	PURPOSE:	A player takes the gun: he must be a human, close
//				behind it and facing the way it does
//
// ----------------------------------------------------------------------- //

// PH: 0x10159e00
void CTurretGun::Activate(HOBJECT hPlayer)
{
	if (m_hUser) return;
	if (!IsPlayer(hPlayer)) return;
	if (!IsHuman(hPlayer)) return;

	CPlayerObj* pPlayer = (CPlayerObj*)g_pLTServer->HandleToObject(hPlayer);
	if (!pPlayer) return;

	LTRotation rRot;
	g_pLTServer->GetObjectRotation(m_hObject, &rRot);

	LTVector vU, vR, vF;
	g_pLTServer->GetRotationVectors(&rRot, &vU, &vR, &vF);

	LTVector vPos, vPlayerPos;
	g_pLTServer->GetObjectPos(m_hObject, &vPos);
	g_pLTServer->GetObjectPos(hPlayer, &vPlayerPos);

	LTVector vDir = vPos - vPlayerPos;
	LTFLOAT fDist = vDir.Mag();
	if (fDist > TURRETGUN_USE_DIST) return;

	vDir *= 1.0f / fDist;
	if (vDir.Dot(vF) < 0.866f) return;

	m_hUser = hPlayer;

	m_vUserPos = vPos - vF * 64.0f;
	pPlayer->Teleport(m_vUserPos);

	g_pLTServer->AlignRotation(&rRot, &vF, &LTVector(0.0f, 1.0f, 0.0f));
	pPlayer->GetMovement()->SetRot(rRot);

	HMESSAGEWRITE hMessage = g_pLTServer->StartMessageToObject(this, m_hUser, MID_TURRETGUN);
	g_pLTServer->WriteToMessageByte(hMessage, TURRETGUN_MSG_ACTIVE);
	g_pLTServer->WriteToMessageByte(hMessage, LTTRUE);
	g_pLTServer->EndMessage(hMessage);

	SetSpinning(LTFALSE);
	SetFiring(LTFALSE);

	g_pLTServer->SetVelocity(hPlayer, &LTVector(0.0f, 0.0f, 0.0f));

	m_fAmmoMsgDelay = 0.001f;
	m_bFiring = LTFALSE;
	m_bShooting = LTFALSE;
	m_hLoopFireSound = LTNULL;
	m_hSpinSound = LTNULL;

	SetActivateType(m_hObject, AT_NOT_ACTIVATABLE);

	uint32 dwFlags;
	g_pLTServer->Common()->GetObjectFlags(m_hObject, OFT_Flags, dwFlags);
	g_pLTServer->Common()->SetObjectFlags(m_hObject, OFT_Flags, dwFlags & ~FLAG_RAYHIT);

	m_fPitch = 0.0f;
	m_fYaw = 0.0f;
	m_tmrNextUse.Start(0.25f);
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CTurretGun::Deactivate()
//
// ----------------------------------------------------------------------- //

// PH: 0x1015a1f0
void CTurretGun::Deactivate()
{
	HMESSAGEWRITE hMessage = g_pLTServer->StartMessageToObject(this, m_hUser, MID_TURRETGUN);
	g_pLTServer->WriteToMessageByte(hMessage, TURRETGUN_MSG_ACTIVE);
	g_pLTServer->WriteToMessageByte(hMessage, LTFALSE);
	g_pLTServer->EndMessage(hMessage);

	CPlayerObj* pPlayer = (CPlayerObj*)g_pLTServer->HandleToObject(m_hUser);
	if (pPlayer)
	{
		LTRotation rRot;
		g_pLTServer->GetObjectRotation(m_hObject, &rRot);
		pPlayer->GetMovement()->SetRot(rRot);
	}

	SetSpinning(LTFALSE);
	SetFiring(LTFALSE);

	m_hUser = LTNULL;
	m_bSpinning = LTFALSE;
	m_bFiring = LTFALSE;
	m_bShooting = LTFALSE;
	m_fYaw = 0.0f;
	m_fPitch = 0.0f;

	SetActivateType(m_hObject, AT_TURRETGUN);

	uint32 dwFlags;
	g_pLTServer->Common()->GetObjectFlags(m_hObject, OFT_Flags, dwFlags);
	g_pLTServer->Common()->SetObjectFlags(m_hObject, OFT_Flags, dwFlags | FLAG_RAYHIT);
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CTurretGun::Rotate()
//
// ----------------------------------------------------------------------- //

// PH: 0x1015a340
void CTurretGun::Rotate(LTFLOAT fYaw, LTFLOAT fPitch)
{
	m_fYaw += fYaw;

	if (m_fYawRange >= 6.28f)
	{
		// RETAIL BUG: meant to wrap the yaw into one turn, but the result
		// is thrown away
#ifdef AVP2SRC_MODERN
		// M-B83 (RETAIL_BUGS.md): whole turns taken off (only a rotation
		// is made of it, so where it wraps does not show)
		m_fYaw = (LTFLOAT)fmod(m_fYaw, MATH_CIRCLE);
#else
		ceil(m_fYaw * (1.0f / MATH_CIRCLE));
#endif
	}
	else
	{
		if (m_fYaw > m_fYawRange)
		{
			m_fYaw = m_fYawRange;
		}

		if (m_fYaw < -m_fYawRange)
		{
			m_fYaw = -m_fYawRange;
		}
	}

	m_fPitch += fPitch;

	LTFLOAT fMinPitch = -m_fPitchRange;
	if (m_fPitch < fMinPitch)
	{
		m_fPitch = fMinPitch;
	}

	if (m_fPitch > TURRETGUN_MAX_PITCH)
	{
		m_fPitch = TURRETGUN_MAX_PITCH;
	}
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CTurretGun::GetAmmoCount()
//
// ----------------------------------------------------------------------- //

// PH: 0x1015a410
int CTurretGun::GetAmmoCount()
{
	CWeapon* pWeapon = m_Weapons.GetCurWeapon();
	if (!pWeapon) return 0;

	return m_Weapons.GetAmmoCount(pWeapon->GetAmmoId());
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CTurretGun::PlaySound()
//
// ----------------------------------------------------------------------- //

// PH: 0x1015a470
HLTSOUND CTurretGun::PlaySound(const char* szSound, LTBOOL bLoop, LTBOOL bGetHandle)
{
	PlaySoundInfo playSoundInfo;

	playSoundInfo.m_dwFlags = PLAYSOUND_3D;
	if (bLoop == LTTRUE)
	{
		playSoundInfo.m_dwFlags |= PLAYSOUND_LOOP;
	}
	if (bGetHandle == LTTRUE)
	{
		playSoundInfo.m_dwFlags |= PLAYSOUND_GETHANDLE;
	}

	playSoundInfo.m_fOuterRadius = 2048.0f;
	playSoundInfo.m_fInnerRadius = 512.0f;
	playSoundInfo.m_fPitchShift = 0.0f;
	playSoundInfo.m_hObject = LTNULL;
	playSoundInfo.m_nPriority = SOUNDPRIORITY_PLAYER_LOW;
	playSoundInfo.m_nVolume = 100;

	g_pLTServer->GetObjectPos(m_hObject, &playSoundInfo.m_vPosition);
	SAFE_STRCPY(playSoundInfo.m_szSoundName, szSound);

	HLTSOUND hSound = LTNULL;
	g_pLTServer->SoundMgr()->PlaySound(&playSoundInfo, hSound);

	return hSound;
}

#endif // PRIMAL_HUNT
