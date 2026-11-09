// ----------------------------------------------------------------------- //
//
// MODULE  : TurretInfoMgr.h
//
// PURPOSE : Primal Hunt: what the client knows of the turrets about it
//           (MID_TURRET_AMMO: each one's ammo and heat, for the crosshair)
//           and how many the player carries (MID_PLAYER_TURRETS, for the
//           HUD)
//
// CREATED : Primal Hunt (Third Law); no source survives, written from
//           PHCShell.dll (0x10136bb0 to 0x10136e5a; names ours).
//
// ----------------------------------------------------------------------- //

#ifndef __TURRET_INFO_MGR_H__
#define __TURRET_INFO_MGR_H__

#ifdef PRIMAL_HUNT

#include "ltbasedefs.h"

struct TURRETINFO
{
	HOBJECT		hTurret;
	int			nAmmo;			// percent
	int			nHeat;
};

class CTurretInfoMgr
{
	public :

		CTurretInfoMgr();
		~CTurretInfoMgr();

		void		Init(int nMaxTurrets);
		void		Term();

		void		Set(HOBJECT hTurret, int nAmmo, int nHeat);
		TURRETINFO*	Find(HOBJECT hTurret);
		void		Remove(HOBJECT hTurret);

		void		GetInfo(HOBJECT hTurret, int &nAmmo, int &nHeat);
		void		GetTargetInfo(int &nAmmo, int &nHeat);
		void		SetTarget(HOBJECT hTarget) { m_hTarget = hTarget; }

	public :

		TURRETINFO*	m_pTurrets;
		int			m_nNumTurrets;
		int			m_nMaxTurrets;

		HOBJECT		m_hTarget;			// the turret the player looks at (CInterfaceMgr)
		int			m_nCarried;			// the turrets the player carries (MID_PLAYER_TURRETS)
};

void				InitTurretInfo(int nMaxTurrets);
CTurretInfoMgr*		GetTurretInfoMgr();
void				RemoveTurretInfo(HOBJECT hTurret);

#endif // PRIMAL_HUNT

#endif // __TURRET_INFO_MGR_H__
