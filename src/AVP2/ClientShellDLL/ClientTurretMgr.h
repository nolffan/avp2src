// ----------------------------------------------------------------------- //
//
// MODULE  : ClientTurretMgr.h
//
// PURPOSE : The client's side of Primal Hunt's mounted turret guns (the
//           server's CTurretGun): each turret's aim, barrel spin and muzzle
//           flash, the ammo display and the view of the player manning one.
//
// CREATED : Primal Hunt (Third Law); no source survives, written from
//           PHCShell.dll (0x10134cd0 to 0x10136221; names ours).
//
// ----------------------------------------------------------------------- //

#ifndef __CLIENT_TURRET_MGR_H__
#define __CLIENT_TURRET_MGR_H__

#ifdef PRIMAL_HUNT

#include "ltbasedefs.h"

// What follows MID_TURRET_USE's first byte (names ours)
#define TURRET_USE_ACTIVE			1		// byte on, object turret [, object user, float pitch range, float yaw range, word ammo]
#define TURRET_USE_AIM				2		// object user, float yaw, float pitch
#define TURRET_USE_CONTROLS			3		// object user, byte controls
#define TURRET_USE_AMMO				4		// object turret, word ammo
#define TURRET_USE_ROTATION			5		// object user, byte yaw, float pitch

// The controls (TURRET_USE_CONTROLS)
#define TURRET_CONTROL_FIRE			0x01
#define TURRET_CONTROL_CEASEFIRE	0x02
#define TURRET_CONTROL_SPIN			0x04
#define TURRET_CONTROL_STOPSPIN		0x08

// The spin states
#define TURRET_SPIN_STOPPED			0
#define TURRET_SPIN_UP				1
#define TURRET_SPIN_ON				2
#define TURRET_SPIN_DOWN			3

// ----------------------------------------------------------------------- //

class CClientTurret
{
	public :

		CClientTurret();
		~CClientTurret();

		void		Init(HOBJECT hObject);

		void		Activate(HOBJECT hUser);
		void		Deactivate();

		void		SetSpinControl(LTBOOL bOn);
		void		SetFireControl(LTBOOL bOn);

		void		Update(LTFLOAT fFrameTime);

		void		SetAim(LTFLOAT fPitch, LTFLOAT fYaw);
		void		SetRange(LTFLOAT fPitchRange, LTFLOAT fYawRange);
		void		Rotate(LTFLOAT fPitch, LTFLOAT fYaw, LTFLOAT fSpin);
		void		SetRotation(LTFLOAT fPitch, LTFLOAT fYaw);

		void		DrawAmmo();

		LTFLOAT		GetPitch()			const	{ return m_fPitch; }
		LTFLOAT		GetYaw()			const	{ return m_fYaw; }
		LTFLOAT		GetBarrelAngle()	const	{ return m_fBarrelAngle; }

		static void	NodeControlFn(HOBJECT hObj, HMODELNODE hNode, LTMatrix *pGlobalMat, void *pUserData);

	protected :

		void		SpinUp();
		void		SpinDown();

		void		UpdateSpin(LTFLOAT fFrameTime);
		void		UpdateFire(LTFLOAT fFrameTime);

		void		CreateFlash();
		void		RemoveFlash();
		void		ShowFlash();
		void		HideFlash();

	public :

		LTVector	m_vForward;			// 0x00 the turret's aim
		HOBJECT		m_hObject;			// 0x0c the turret's model
		HOBJECT		m_hUser;			// 0x10 who mans it
		LTBOOL		m_bFiring;			// 0x14
		LTFLOAT		m_fPitch;			// 0x18
		LTFLOAT		m_fYaw;				// 0x1c
		LTFLOAT		m_fBarrelAngle;		// 0x20
		LTFLOAT		m_fSpinSpeed;		// 0x24
		LTFLOAT		m_fAimPitch;		// 0x28 the user's aim, sent to the server
		LTFLOAT		m_fAimYaw;			// 0x2c
		LTFLOAT		m_fPitchRange;		// 0x30
		LTFLOAT		m_fYawRange;		// 0x34
		LTVector	m_vPivotPos;		// 0x38
		LTFLOAT		m_fUnused44;		// 0x44
		LTFLOAT		m_fSpinUpTime;		// 0x48
		LTFLOAT		m_fSpinDownTime;	// 0x4c
		int			m_nAmmo;			// 0x50
		HSURFACE	m_hAmmoSurface;		// 0x54
		LTVector	m_vBarrelAxis;		// 0x58
		HOBJECT		m_hFlash;			// 0x64
		int			m_nSpinState;		// 0x68
		LTBOOL		m_bSpinning;		// 0x6c
		LTBOOL		m_bSpinControl;		// 0x70
		LTBOOL		m_bFireControl;		// 0x74
		LTFLOAT		m_fNextShakeTime;	// 0x78
};

// ----------------------------------------------------------------------- //

class CClientTurretList
{
	public :

		CClientTurretList();
		~CClientTurretList();

		void			Init(int nMaxTurrets);
		void			Term();

		CClientTurret*	Find(HOBJECT hObject);
		CClientTurret*	Add(HOBJECT hObject);

		void			Update(LTFLOAT fFrameTime);
		void			DrawAmmo();

	protected :

		CClientTurret*	m_pTurrets;
		int				m_nNumTurrets;
		int				m_nMaxTurrets;
};

// ----------------------------------------------------------------------- //

class CClientTurretMgr : public CClientTurretList
{
	public :

		CClientTurretMgr() {}
		~CClientTurretMgr() {}

		void			SaveCameraFX(int nID);
		void			RestoreCameraFX(uint8 nID);

	protected :

		int				m_nCameraFXState[19];	// what the turret view turned off, CAMERAMGRFX_ID_POSOFFSET on
};

void				InitClientTurrets(int nMaxTurrets);
CClientTurretMgr*	GetClientTurretMgr();

void				StartTurretView(CClientTurret* pTurret);
void				EndTurretView(CClientTurret* pTurret);

#endif // PRIMAL_HUNT

#endif // __CLIENT_TURRET_MGR_H__
