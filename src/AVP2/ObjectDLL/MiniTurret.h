// ----------------------------------------------------------------------- //
//
// MODULE  : MiniTurret.h
//
// PURPOSE : CMiniTurret - the small turret gun model
//
//           Primal Hunt (Third Law). No source shipped: written from
//           PHObject.lto, 0x10153e50..0x1015412a (vtable 0x1028b160).
//           Needs CTurretGun (TurretGun.h, written in parallel).
//
// ----------------------------------------------------------------------- //

#ifndef __MINI_TURRET_H__
#define __MINI_TURRET_H__

#ifdef PRIMAL_HUNT

#include "TurretGun.h"

class CMiniTurret : public CTurretGun
{
	public :

		CMiniTurret();												// 0x10154000
		~CMiniTurret();												// 0x10154040

	protected :

		virtual uint32	OnPrecreate(void *pData, uint32 nInfo);		// 0x10154050
		virtual uint32	OnInitialUpdate(uint32 nInfo);				// 0x101540c0
		virtual uint32	OnUpdate(LTFLOAT fFrameTime);				// 0x101540e0
		virtual void	OnSave(HMESSAGEWRITE hWrite);				// 0x10154120
		virtual void	OnLoad(HMESSAGEREAD hRead);					// 0x10154110
};

#endif // PRIMAL_HUNT

#endif // __MINI_TURRET_H__
