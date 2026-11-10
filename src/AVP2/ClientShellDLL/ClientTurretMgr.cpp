// ----------------------------------------------------------------------- //
//
// MODULE  : ClientTurretMgr.cpp
//
// PURPOSE : The client's side of Primal Hunt's mounted turret guns
//
// CREATED : Primal Hunt (Third Law); no source survives, written from
//           PHCShell.dll (0x10134cd0 to 0x10136221; names ours).
//
// ----------------------------------------------------------------------- //

#include "stdafx.h"

#ifdef PRIMAL_HUNT

#include "ClientTurretMgr.h"
#include "GameClientShell.h"
#include "InterfaceResMgr.h"
#include "CameraMgrFX.h"
#include "MsgIDs.h"
#include "ltquatbase.h"

extern CGameClientShell* g_pGameClientShell;

// ----------------------------------------------------------------------- //

CClientTurretMgr g_ClientTurretMgr;

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CClientTurret::CClientTurret()
//
// ----------------------------------------------------------------------- //

CClientTurret::CClientTurret()
{
	m_hFlash = LTNULL;
	Init(LTNULL);
}

// ----------------------------------------------------------------------- //

CClientTurret::~CClientTurret()
{
	RemoveFlash();
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CClientTurret::Init()
//
//	PURPOSE:	Take a turret model and control its nodes (0x10134d60)
//
// ----------------------------------------------------------------------- //

void CClientTurret::Init(HOBJECT hObject)
{
	m_hObject			= hObject;
	m_hUser				= LTNULL;
	m_bFiring			= LTFALSE;
	m_fPitch			= 0.0f;
	m_fYaw				= 0.0f;
	m_fBarrelAngle		= 0.0f;
	m_fSpinSpeed		= 0.0f;
	m_fPitchRange		= 0.0f;
	m_fYawRange			= 0.0f;

	g_pLTClient->ModelNodeControl(hObject, NodeControlFn, this);

	m_vForward			= LTVector(0.0f, 0.0f, 1.0f);
	m_fUnused44			= 0.0f;
	m_bSpinning			= LTFALSE;
	m_nSpinState		= TURRET_SPIN_STOPPED;

	RemoveFlash();

	m_bSpinControl		= LTFALSE;
	m_bFireControl		= LTFALSE;
	m_fNextShakeTime	= 0.0f;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CClientTurret::Activate()
//
//	PURPOSE:	Someone mans the turret (0x10134de0)
//
// ----------------------------------------------------------------------- //

void CClientTurret::Activate(HOBJECT hUser)
{
	m_hUser			= hUser;
	m_fPitch		= 0.0f;
	m_fYaw			= 0.0f;
	m_fBarrelAngle	= 0.0f;

	LTVector vPos;
	g_pLTClient->GetObjectPos(m_hObject, &vPos);
	m_vPivotPos = vPos;

	if(g_pLTClient->GetClientObject() == hUser)
	{
		m_hAmmoSurface = g_pInterfaceResMgr->GetSharedSurface(
			(g_pGameClientShell->GetPlayerMovement()->GetCharacterButes()->m_eCharacterClass == CORPORATE) ?
			"/Interface/StatusBar/Corporate/turret_gun_ammo2.pcx" :
			"/Interface/StatusBar/Marine/turret_gun_ammo1.pcx");
	}

	m_fNextShakeTime = g_pLTClient->GetTime();

	CreateFlash();

	m_bSpinControl = LTFALSE;
	m_bFireControl = LTFALSE;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CClientTurret::Deactivate()
//
//	PURPOSE:	The turret is let go of (0x10134e90)
//
// ----------------------------------------------------------------------- //

void CClientTurret::Deactivate()
{
	SetFireControl(LTFALSE);

	m_hUser			= LTNULL;
	m_fPitch		= 0.0f;
	m_fYaw			= 0.0f;
	m_fBarrelAngle	= 0.0f;
	m_bSpinning		= LTFALSE;

	SpinDown();
	RemoveFlash();

	CWeaponModel* pWeaponModel = g_pGameClientShell->GetWeaponModel();
	if(pWeaponModel)
	{
		HOBJECT hWeapon = pWeaponModel->GetHandle();
		if(hWeapon)
		{
			uint32 dwFlags;
			g_pLTClient->Common()->GetObjectFlags(hWeapon, OFT_Flags, dwFlags);
			g_pLTClient->Common()->SetObjectFlags(hWeapon, OFT_Flags, dwFlags & ~FLAG_VISIBLE);
		}
	}

	m_bSpinControl = LTFALSE;
	m_bFireControl = LTFALSE;
}

// ----------------------------------------------------------------------- //

void CClientTurret::SpinUp()
{
	if(m_nSpinState != TURRET_SPIN_ON && m_nSpinState != TURRET_SPIN_UP)
	{
		m_fSpinUpTime = g_pLTClient->GetTime() + 0.5f;
		m_nSpinState = TURRET_SPIN_UP;
	}
}

// ----------------------------------------------------------------------- //

void CClientTurret::SpinDown()
{
	if(m_nSpinState != TURRET_SPIN_STOPPED && m_nSpinState != TURRET_SPIN_DOWN)
	{
		m_fSpinDownTime = g_pLTClient->GetTime() + 1.0f;
		m_nSpinState = TURRET_SPIN_DOWN;
	}
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CClientTurret::SetSpinControl()
//
//	PURPOSE:	The spin key: each press toggles the barrels (0x10134f70)
//
// ----------------------------------------------------------------------- //

void CClientTurret::SetSpinControl(LTBOOL bOn)
{
	if(bOn == m_bSpinControl)
		return;

	m_bSpinControl = bOn;

	if(!m_hUser || bOn)
		return;

	if(!m_bSpinning)
	{
		m_bSpinning = LTTRUE;

		HMESSAGEWRITE hMessage = g_pLTClient->StartMessage(MID_TURRET_USE);
		g_pLTClient->WriteToMessageByte(hMessage, TURRET_USE_CONTROLS);
		g_pLTClient->WriteToMessageByte(hMessage, TURRET_CONTROL_SPIN);
		g_pLTClient->EndMessage(hMessage);

		SpinUp();
	}
	else
	{
		m_bSpinning = LTFALSE;

		HMESSAGEWRITE hMessage = g_pLTClient->StartMessage(MID_TURRET_USE);
		g_pLTClient->WriteToMessageByte(hMessage, TURRET_USE_CONTROLS);
		g_pLTClient->WriteToMessageByte(hMessage, TURRET_CONTROL_STOPSPIN);
		g_pLTClient->EndMessage(hMessage);

		if(!m_bFiring)
			SpinDown();
	}
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CClientTurret::SetFireControl()
//
//	PURPOSE:	The fire key (0x10135050)
//
// ----------------------------------------------------------------------- //

void CClientTurret::SetFireControl(LTBOOL bOn)
{
	if(bOn == m_bFireControl)
		return;

	m_bFireControl = bOn;

	if(bOn == LTTRUE)
	{
		if(m_bFiring)
			return;

		SpinUp();

		if(m_nSpinState == TURRET_SPIN_ON && m_nAmmo > 0)
			ShowFlash();

		HMESSAGEWRITE hMessage = g_pLTClient->StartMessage(MID_TURRET_USE);
		g_pLTClient->WriteToMessageByte(hMessage, TURRET_USE_CONTROLS);
		g_pLTClient->WriteToMessageByte(hMessage, TURRET_CONTROL_FIRE);
		g_pLTClient->EndMessage(hMessage);

		m_bFiring = LTTRUE;
	}
	else if(m_bFiring == LTTRUE)
	{
		HideFlash();

		if(!m_bSpinning)
			SpinDown();

		HMESSAGEWRITE hMessage = g_pLTClient->StartMessage(MID_TURRET_USE);
		g_pLTClient->WriteToMessageByte(hMessage, TURRET_USE_CONTROLS);
		g_pLTClient->WriteToMessageByte(hMessage, TURRET_CONTROL_CEASEFIRE);
		g_pLTClient->EndMessage(hMessage);

		m_bFiring = LTFALSE;
	}
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CClientTurret::UpdateSpin()
//
//	PURPOSE:	Turn the barrels (0x10135150)
//
// ----------------------------------------------------------------------- //

void CClientTurret::UpdateSpin(LTFLOAT fFrameTime)
{
	switch(m_nSpinState)
	{
		case TURRET_SPIN_UP:
		{
			m_fSpinSpeed += fFrameTime * 100.0f;
			if(m_fSpinSpeed > 100.0f) m_fSpinSpeed = 100.0f;

			if(g_pLTClient->GetTime() >= m_fSpinUpTime)
				m_nSpinState = TURRET_SPIN_ON;
		}
		break;

		case TURRET_SPIN_ON:
		{
			m_fSpinSpeed += fFrameTime * 100.0f;
			if(m_fSpinSpeed > 100.0f) m_fSpinSpeed = 100.0f;
		}
		break;

		case TURRET_SPIN_DOWN:
		{
			m_fSpinSpeed -= fFrameTime * 50.0f;
			if(m_fSpinSpeed < 0.0f) m_fSpinSpeed = 0.0f;

			if(g_pLTClient->GetTime() >= m_fSpinDownTime)
				m_nSpinState = TURRET_SPIN_STOPPED;
		}
		break;

		default:
		{
			m_fSpinSpeed -= fFrameTime * 50.0f;
			if(m_fSpinSpeed < 0.0f) m_fSpinSpeed = 0.0f;
		}
		break;
	}

	if(m_fSpinSpeed != 0.0f)
	{
		m_fBarrelAngle += fFrameTime * m_fSpinSpeed;

		if(m_fBarrelAngle >= MATH_CIRCLE)
			m_fBarrelAngle -= MATH_CIRCLE;
	}
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CClientTurret::UpdateFire()
//
//	PURPOSE:	The muzzle flash and the recoil (0x10135270)
//
// ----------------------------------------------------------------------- //

void CClientTurret::UpdateFire(LTFLOAT fFrameTime)
{
	if(m_bFiring == LTTRUE)
	{
		if(m_nSpinState == TURRET_SPIN_ON && m_nAmmo > 0)
		{
			ShowFlash();

			if(g_pLTClient->GetTime() >= m_fNextShakeTime)
			{
				g_pGameClientShell->ShakeScreen(LTVector(1.0f, 1.0f, 1.0f), 0.2f, 0.001f, 0.025f);
				m_fNextShakeTime = g_pLTClient->GetTime() + 0.1f;
			}
		}
		else
		{
			HideFlash();
		}
	}
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CClientTurret::Update()
//
//	PURPOSE:	Send the user's aim, spin, fire and place the flash
//				(0x10135330)
//
// ----------------------------------------------------------------------- //

void CClientTurret::Update(LTFLOAT fFrameTime)
{
	if(m_fAimPitch != 0.0f || m_fAimYaw != 0.0f)
	{
		HMESSAGEWRITE hMessage = g_pLTClient->StartMessage(MID_TURRET_USE);
		g_pLTClient->WriteToMessageByte(hMessage, TURRET_USE_AIM);
		g_pLTClient->WriteToMessageFloat(hMessage, m_fAimYaw);
		g_pLTClient->WriteToMessageFloat(hMessage, m_fAimPitch);
		g_pLTClient->EndMessage2(hMessage, 0);
	}

	m_fAimPitch = 0.0f;
	m_fAimYaw = 0.0f;

	UpdateSpin(fFrameTime);
	UpdateFire(fFrameTime);

	if(!m_hFlash)
		return;

	HMODELSOCKET hSocket = INVALID_MODEL_SOCKET;
	g_pLTClient->GetModelLT()->GetSocket(m_hObject, "Flash", hSocket);

	if(hSocket != INVALID_MODEL_SOCKET)
	{
		LTransform tf;
		g_pLTClient->GetModelLT()->GetSocketTransform(m_hObject, hSocket, tf, LTTRUE);
		g_pLTClient->SetObjectPos(m_hFlash, &tf.m_Pos);
	}
}

// ----------------------------------------------------------------------- //

void CClientTurret::SetAim(LTFLOAT fPitch, LTFLOAT fYaw)
{
	m_fAimPitch = fPitch * 0.01f;
	m_fAimYaw = fYaw * 0.5f;
}

// ----------------------------------------------------------------------- //

void CClientTurret::SetRange(LTFLOAT fPitchRange, LTFLOAT fYawRange)
{
	m_fPitchRange = fPitchRange;
	m_fYawRange = fYawRange;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CClientTurret::Rotate()
//
//	PURPOSE:	Turn the turret by its user's aim (0x10135480)
//
// ----------------------------------------------------------------------- //

void CClientTurret::Rotate(LTFLOAT fPitch, LTFLOAT fYaw, LTFLOAT fSpin)
{
	m_fYaw += fYaw;

	if(m_fYawRange >= 6.28f)
	{
		// RETAIL BUG: the turret was meant to wrap all the way around here,
		// but the result is thrown away
		floor(m_fYaw * (1.0f / MATH_CIRCLE));
	}
	else
	{
		LTFLOAT fMin = -m_fYawRange;
		if(m_fYaw < fMin) m_fYaw = fMin;
		if(m_fYaw > m_fYawRange) m_fYaw = m_fYawRange;
	}

	m_fPitch += fPitch;
	if(m_fPitch > 0.5235988f) m_fPitch = 0.5235988f;
	if(m_fPitch < -m_fPitchRange) m_fPitch = -m_fPitchRange;

	m_fBarrelAngle += fSpin;
}

// ----------------------------------------------------------------------- //

void CClientTurret::SetRotation(LTFLOAT fPitch, LTFLOAT fYaw)
{
	m_fYaw = fYaw;
	m_fPitch = fPitch;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CClientTurret::DrawAmmo()
//
//	PURPOSE:	The ammo display, bottom right (0x10135560)
//
// ----------------------------------------------------------------------- //

void CClientTurret::DrawAmmo()
{
	HSURFACE hScreen = g_pLTClient->GetScreenSurface();

	uint32 nScreenWidth, nScreenHeight;
	g_pLTClient->GetSurfaceDims(hScreen, &nScreenWidth, &nScreenHeight);

	uint32 nWidth, nHeight;
	g_pLTClient->GetSurfaceDims(m_hAmmoSurface, &nWidth, &nHeight);

	LTRect rcDest;
	rcDest.left		= nScreenWidth - nWidth - 30;
	rcDest.top		= nScreenHeight - nHeight - 45;
	rcDest.right	= rcDest.left + nWidth;
	rcDest.bottom	= rcDest.top + nHeight;

	g_pLTClient->ScaleSurfaceToSurfaceTransparent(hScreen, m_hAmmoSurface, &rcDest, LTNULL, SETRGB_T(0,0,0));

	CHudMgr* pHudMgr = g_pGameClientShell->GetInterfaceMgr()->GetHudMgr();
	if(pHudMgr)
	{
		LithFont* pFont = pHudMgr->GetFont(1);
		if(pFont)
		{
			char szAmmo[16];
			itoa(m_nAmmo, szAmmo, 10);

			LITHFONTDRAWDATA lfDD;
			lfDD.nLetterSpace	= 1;
			lfDD.bForceFormat	= LTTRUE;
			lfDD.byJustify		= LTF_JUSTIFY_CENTER;

			pFont->Draw(hScreen, szAmmo, &lfDD, rcDest.left + (rcDest.right - rcDest.left) / 2, rcDest.top + 25, LTNULL);
		}
	}
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CClientTurret::NodeControlFn()
//
//	PURPOSE:	Turn the turret's nodes: the yaw, the pitch and the
//				barrels' spin (0x101356a0)
//
// ----------------------------------------------------------------------- //

void CClientTurret::NodeControlFn(HOBJECT hObj, HMODELNODE hNode, LTMatrix *pGlobalMat, void *pUserData)
{
	static LTBOOL s_bPivoted = LTFALSE;

	if(!hObj)
		return;

	CClientTurret* pTurret = (CClientTurret*)pUserData;
	if(!pTurret)
		return;

	LTMatrix mMat = *pGlobalMat;

	LTRotation rRot;
	quat_ConvertFromMatrix((float*)&rRot, mMat.m);

	LTVector vR, vU, vF;
	g_pLTClient->GetRotationVectors(&rRot, &vR, &vU, &vF);

	LTVector vPitchAxis = vR;
	LTVector vYawAxis = vF;
	LTVector vSpinAxis = vU;

	LTVector vPos;
	pGlobalMat->GetTranslation(vPos);

	char szNode[128];
	g_pLTClient->GetModelNodeName(hObj, hNode, szNode, 127);

	if(stricmp("Bone_Horiz", szNode) == 0)
	{
		vYawAxis = LTVector(0.0f, 1.0f, 0.0f);
		g_pLTClient->RotateAroundAxis(&rRot, &vYawAxis, pTurret->GetYaw());

		LTVector vPivot;
		pGlobalMat->GetTranslation(vPivot);
		pTurret->m_vPivotPos = vPivot;

		LTVector vTemp1, vTemp2;
		g_pLTClient->GetRotationVectors(&rRot, &vTemp1, &vTemp2, &pTurret->m_vForward);
	}
	else if(stricmp("Bone_Pivot", szNode) == 0)
	{
		g_pLTClient->RotateAroundAxis(&rRot, &vPitchAxis, pTurret->GetPitch());
		s_bPivoted = LTTRUE;
	}
	else if(stricmp("Bone_Barrel", szNode) == 0 && s_bPivoted)
	{
		g_pLTClient->RotateAroundAxis(&rRot, &vSpinAxis, pTurret->GetBarrelAngle());
		pTurret->m_vBarrelAxis = vSpinAxis;
		s_bPivoted = LTFALSE;
	}

	quat_ConvertToMatrix((float*)&rRot, mMat.m);
	mMat.SetTranslation(vPos.x, vPos.y, vPos.z);

	*pGlobalMat = mMat;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CClientTurret::CreateFlash()
//
//	PURPOSE:	The muzzle flash sprite, hidden (0x10135950)
//
// ----------------------------------------------------------------------- //

void CClientTurret::CreateFlash()
{
	RemoveFlash();

	LTVector vPos;
	g_pLTClient->GetObjectPos(m_hObject, &vPos);

	ObjectCreateStruct createStruct;
	INIT_OBJECTCREATESTRUCT(createStruct);

	createStruct.m_ObjectType = OT_SPRITE;
	createStruct.m_Flags = FLAG_NOLIGHT | FLAG_SPRITEBIAS;
	createStruct.m_Flags2 = FLAG2_ADDITIVE;
	createStruct.m_Pos = vPos;
	strncpy(createStruct.m_Filename, "sprites/exp/SFX/MuzzleFX/TurretFlash.spr", 127);
	createStruct.m_Scale = LTVector(0.125f, 0.125f, 0.125f);

	m_hFlash = g_pLTClient->CreateObject(&createStruct);

	if(m_hFlash)
		g_pLTClient->SetObjectColor(m_hFlash, 1.0f, 1.0f, 1.0f, 1.0f);
}

// ----------------------------------------------------------------------- //

void CClientTurret::RemoveFlash()
{
	if(m_hFlash)
	{
		g_pLTClient->RemoveObject(m_hFlash);
		m_hFlash = LTNULL;
	}
}

// ----------------------------------------------------------------------- //

void CClientTurret::ShowFlash()
{
	if(m_hFlash)
	{
		uint32 dwFlags;
		g_pLTClient->Common()->GetObjectFlags(m_hFlash, OFT_Flags, dwFlags);
		g_pLTClient->Common()->SetObjectFlags(m_hFlash, OFT_Flags, dwFlags | FLAG_VISIBLE);
	}
}

// ----------------------------------------------------------------------- //

void CClientTurret::HideFlash()
{
	if(m_hFlash)
	{
		uint32 dwFlags;
		g_pLTClient->Common()->GetObjectFlags(m_hFlash, OFT_Flags, dwFlags);
		g_pLTClient->Common()->SetObjectFlags(m_hFlash, OFT_Flags, dwFlags & ~FLAG_VISIBLE);
	}
}

// ----------------------------------------------------------------------- //
//
//	CClientTurretList
//
// ----------------------------------------------------------------------- //

CClientTurretList::CClientTurretList()
{
	m_nNumTurrets = 0;
	m_nMaxTurrets = 20;
	m_pTurrets = LTNULL;
}

// ----------------------------------------------------------------------- //

CClientTurretList::~CClientTurretList()
{
	Term();
}

// ----------------------------------------------------------------------- //

void CClientTurretList::Init(int nMaxTurrets)
{
	Term();

	m_nNumTurrets = 0;
	m_nMaxTurrets = nMaxTurrets;
	m_pTurrets = new CClientTurret[nMaxTurrets];
}

// ----------------------------------------------------------------------- //

void CClientTurretList::Term()
{
	if(m_pTurrets)
	{
		delete [] m_pTurrets;
		m_pTurrets = LTNULL;
	}

	m_pTurrets = LTNULL;
	m_nNumTurrets = 0;
}

// ----------------------------------------------------------------------- //

CClientTurret* CClientTurretList::Find(HOBJECT hObject)
{
	for(int i = 0; i < m_nNumTurrets; i++)
	{
		if(m_pTurrets[i].m_hObject == hObject || m_pTurrets[i].m_hUser == hObject)
			return &m_pTurrets[i];
	}

	return LTNULL;
}

// ----------------------------------------------------------------------- //

CClientTurret* CClientTurretList::Add(HOBJECT hObject)
{
	if(m_nNumTurrets >= m_nMaxTurrets)
		return LTNULL;

	CClientTurret* pTurret = &m_pTurrets[m_nNumTurrets];
	pTurret->Init(hObject);
	m_nNumTurrets++;

	return pTurret;
}

// ----------------------------------------------------------------------- //

void CClientTurretList::Update(LTFLOAT fFrameTime)
{
	for(int i = 0; i < m_nNumTurrets; i++)
		m_pTurrets[i].Update(fFrameTime);
}

// ----------------------------------------------------------------------- //

void CClientTurretList::DrawAmmo()
{
	for(int i = 0; i < m_nNumTurrets; i++)
	{
		CClientTurret* pTurret = &m_pTurrets[i];
		HOBJECT hUser = pTurret->m_hUser;

		if(g_pLTClient->GetClientObject() == hUser)
			pTurret->DrawAmmo();
	}
}

// ----------------------------------------------------------------------- //

void InitClientTurrets(int nMaxTurrets)
{
	if(GetClientTurretMgr())
		GetClientTurretMgr()->Init(nMaxTurrets);
}

// ----------------------------------------------------------------------- //

CClientTurretMgr* GetClientTurretMgr()
{
	return &g_ClientTurretMgr;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CClientTurretMgr::SaveCameraFX()
//
//	PURPOSE:	Turn off one of the player camera's effects for the turret
//				view, remembering its state
//
// ----------------------------------------------------------------------- //

inline void CClientTurretMgr::SaveCameraFX(int nID)
{
	CAMERAEFFECTINSTANCE *pEffect = g_pCameraMgr->GetCameraEffectByID(g_pGameClientShell->GetPlayerCamera(), nID, LTNULL);

	if(pEffect)
	{
		CAMERAMGRFX *pData = (CAMERAMGRFX*)pEffect->ceCS.pUserData;

		if(pData)
		{
			m_nCameraFXState[nID - CAMERAMGRFX_ID_POSOFFSET] = pData->nState;
			pData->nState = CAMERAMGRFX_STATE_DISABLED;
		}
	}
	else
	{
		m_nCameraFXState[nID - CAMERAMGRFX_ID_POSOFFSET] = -1;
	}
}

// ----------------------------------------------------------------------- //

inline void CClientTurretMgr::RestoreCameraFX(uint8 nID)
{
	CAMERAEFFECTINSTANCE *pEffect = g_pCameraMgr->GetCameraEffectByID(g_pGameClientShell->GetPlayerCamera(), nID, LTNULL);

	if(pEffect)
	{
		CAMERAMGRFX *pData = (CAMERAMGRFX*)pEffect->ceCS.pUserData;

		if(pData)
			pData->nState = (uint8)m_nCameraFXState[nID - CAMERAMGRFX_ID_POSOFFSET];
	}
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	StartTurretView()
//
//	PURPOSE:	The player mans a turret: the view goes behind it
//				(0x10135dd0)
//
// ----------------------------------------------------------------------- //

void StartTurretView(CClientTurret* pTurret)
{
	if(g_pCameraMgr->GetCameraActive(g_pGameClientShell->GetCinematicCamera()))
		return;

	g_ClientTurretMgr.SaveCameraFX(CAMERAMGRFX_ID_AIMING);
	g_ClientTurretMgr.SaveCameraFX(CAMERAMGRFX_ID_POSOFFSET);
	g_ClientTurretMgr.SaveCameraFX(CAMERAMGRFX_ID_3RDPERSON);
	g_ClientTurretMgr.SaveCameraFX(CAMERAMGRFX_ID_WIDESCREEN);
	g_ClientTurretMgr.SaveCameraFX(CAMERAMGRFX_ID_ZOOM);
	g_ClientTurretMgr.SaveCameraFX(CAMERAMGRFX_ID_QUICKZOOM);
	g_ClientTurretMgr.SaveCameraFX(CAMERAMGRFX_ID_LANDBOB);
	g_ClientTurretMgr.SaveCameraFX(CAMERAMGRFX_ID_NET_TILT);

	LTRotation rRot;
	g_pLTClient->GetObjectRotation(pTurret->m_hObject, &rRot);
	g_pCameraMgr->SetCameraRotation(g_pGameClientShell->GetPlayerCamera(), rRot);

	CameraMgrFX_TurretCreate(g_pGameClientShell->GetPlayerCamera(), pTurret->m_hObject);
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	EndTurretView()
//
//	PURPOSE:	The player lets go of the turret (0x10136040)
//
// ----------------------------------------------------------------------- //

void EndTurretView(CClientTurret* pTurret)
{
	CameraMgrFX_Clear(g_pGameClientShell->GetPlayerCamera(), CAMERAMGRFX_ID_TURRET);

	LTRotation rRot;
	g_pLTClient->GetObjectRotation(pTurret->m_hObject, &rRot);
	g_pCameraMgr->SetCameraRotation(g_pGameClientShell->GetPlayerCamera(), rRot);

	g_ClientTurretMgr.RestoreCameraFX(CAMERAMGRFX_ID_AIMING);
	g_ClientTurretMgr.RestoreCameraFX(CAMERAMGRFX_ID_POSOFFSET);
	g_ClientTurretMgr.RestoreCameraFX(CAMERAMGRFX_ID_3RDPERSON);
	g_ClientTurretMgr.RestoreCameraFX(CAMERAMGRFX_ID_WIDESCREEN);
	g_ClientTurretMgr.RestoreCameraFX(CAMERAMGRFX_ID_ZOOM);
	g_ClientTurretMgr.RestoreCameraFX(CAMERAMGRFX_ID_QUICKZOOM);
	g_ClientTurretMgr.RestoreCameraFX(CAMERAMGRFX_ID_LANDBOB);
	g_ClientTurretMgr.RestoreCameraFX(CAMERAMGRFX_ID_NET_TILT);
}

#endif // PRIMAL_HUNT
