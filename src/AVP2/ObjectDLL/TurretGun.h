// ----------------------------------------------------------------------- //
//
// MODULE  : TurretGun.h
//
// PURPOSE : CTurretGun - a mounted minigun a marine player can man
//
// CREATED : Primal Hunt (Third Law); no source survives, written from
//           PHObject.lto (vtable 0x1028b404, ClassDef 0x102e4ce0).
//           CMiniTurret (0x1028b160) derives from it.
//
// ----------------------------------------------------------------------- //

#ifndef __TURRET_GUN_H__
#define __TURRET_GUN_H__

#ifdef PRIMAL_HUNT

#include "GameBase.h"
#include "Weapons.h"
#include "ltsmartlink.h"
#include "GameTimer.h"

class CTurretGun : public GameBase
{
	public :

		CTurretGun();
		virtual ~CTurretGun();

		// Primal Hunt's virtuals, vtable slots 18 to 21
		virtual void	SetFiring(LTBOOL bFiring);			// 0x10159c70
		virtual void	SetSpinning(LTBOOL bSpinning);		// 0x10159dd0
		virtual void	Activate(HOBJECT hPlayer);			// 0x10159e00
		virtual void	Deactivate();						// 0x1015a1f0

		// The user's aim (CGameServerShell, PHObject.lto 0x10058976)
		void			Rotate(LTFLOAT fYaw, LTFLOAT fPitch);
		int				GetAmmoCount();

		HOBJECT			GetUser() const { return m_hUser; }
		LTFLOAT			GetYawRange() const { return m_fYawRange; }		// the player's view (CPlayerObj::StartUsingTurret)
		LTFLOAT			GetPitchRange() const { return m_fPitchRange; }

	protected :

		virtual uint32	OnPrecreate(void *pData, uint32 nInfo);
		virtual uint32	OnInitialUpdate(uint32 nInfo);
		virtual uint32	OnUpdate(LTFLOAT fFrameTime);
		virtual uint32	OnTrigger(HOBJECT hSender, const char* szMsg);
		virtual void	OnSave(HMESSAGEWRITE hWrite);
		virtual void	OnLoad(HMESSAGEREAD hRead);

		void			StartSpin();
		void			StopSpin();
		void			UpdateSpin(LTFLOAT fFrameTime);

		HLTSOUND		PlaySound(const char* szSound, LTBOOL bLoop, LTBOOL bGetHandle);

	protected :

		enum SpinState
		{
			eSpinStopped = 0,
			eSpinningUp,
			eSpinning,
			eSpinningDown
		};

		int			m_nAmmo;				// 0x50
		LTSmartLink	m_hUser;				// 0x54
		LTBOOL		m_bFiring;				// 0x5c
		LTFLOAT		m_fFireDelay;			// 0x60
		CWeapons	m_Weapons;				// 0x64
		uint32		m_nWeaponId;			// 0x878
		int			m_nLastAmmo;			// 0x87c
		LTFLOAT		m_fAmmoMsgDelay;		// 0x880
		LTBOOL		m_bMultiplayer;			// 0x884
		HLTSOUND	m_hLoopFireSound;		// 0x888
		HLTSOUND	m_hSpinSound;			// 0x88c
		LTBOOL		m_bShooting;			// 0x890
		CGameTimer	m_tmrSpinUp;			// 0x894
		LTVector	m_vUserPos;				// 0x898
		LTFLOAT		m_fYawRange;			// 0x8a4
		LTFLOAT		m_fPitchRange;			// 0x8a8
		LTFLOAT		m_fYaw;					// 0x8ac
		LTFLOAT		m_fPitch;				// 0x8b0
		int			m_nAmmoRegen;			// 0x8b4
		LTFLOAT		m_fAmmoRegenTime;		// 0x8b8
		LTFLOAT		m_fNextAmmoRegenTime;	// 0x8bc
		LTBOOL		m_bSpinning;			// 0x8c0
		int			m_eSpinState;			// 0x8c4
		CGameTimer	m_tmrSpinDown;			// 0x8c8
		CGameTimer	m_tmrFireStart;			// 0x8cc
		HLTSOUND	m_hStartFireSound;		// 0x8d0
		CGameTimer	m_tmrNextUse;			// 0x8d4
};

LTBOOL		IsTurretGun(HOBJECT hObj);
CTurretGun*	GetTurretGun(HOBJECT hObj);

#endif // PRIMAL_HUNT

#endif // __TURRET_GUN_H__
