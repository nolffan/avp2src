// ----------------------------------------------------------------------- //
//
// MODULE  : TurretSpawner.h
//
// PURPOSE : CTurretSpawner - keeps a carryable turret at its position,
//           spawning a new one once the last has been picked up or
//           destroyed
//
// CREATED : Primal Hunt (Third Law); no source survives, written from
//           PHObject.lto (vtable 0x1028b46c, ClassDef 0x102e4e08)
//
// ----------------------------------------------------------------------- //

#ifndef __TURRET_SPAWNER_H__
#define __TURRET_SPAWNER_H__

#ifdef PRIMAL_HUNT

#include "GameBase.h"
#include "ltsmartlink.h"

class CTurretSpawner : public GameBase
{
	public :

		CTurretSpawner();
		virtual ~CTurretSpawner();

		// The spawned turrets alive in the world (Turret::Destroyed lets
		// go of its own, PHObject.lto 0x1008a9d5).
		static void	DecTurretCount();
		static void	IncTurretCount();
		static int	GetTurretCount();
		static void	ResetTurretCount();		// a new world (CGameServerShell::PreStartWorld)

	protected :

		virtual uint32	OnPrecreate(void *pData, uint32 nInfo);
		virtual uint32	OnInitialUpdate(uint32 nInfo);
		virtual uint32	OnUpdate(LTFLOAT fFrameTime);
		virtual uint32	OnTrigger(HOBJECT hSender, const char* szMsg);
		virtual void	OnSave(HMESSAGEWRITE hWrite);
		virtual void	OnLoad(HMESSAGEREAD hRead);

	private :

		void	Spawn();
		LTBOOL	NeedsTurret();

		LTSmartLink	m_hTurret;			// 0x50
		LTFLOAT		m_fSpawnInterval;	// 0x58

		static int	s_nTurretCount;
};

#endif // PRIMAL_HUNT

#endif // __TURRET_SPAWNER_H__
