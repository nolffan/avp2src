// ----------------------------------------------------------------------- //
//
// MODULE  : MiniTurret.cpp
//
// PURPOSE : CMiniTurret - Implementation
//
//           Primal Hunt (Third Law). No source shipped: written from
//           PHObject.lto, 0x10153e50..0x1015412a.
//
// ----------------------------------------------------------------------- //

#include "stdafx.h"

#ifdef PRIMAL_HUNT

#include "MiniTurret.h"

BEGIN_CLASS(CMiniTurret)
	ADD_LONGINTPROP_FLAG(Ammo, 750, 0)
	ADD_REALPROP_FLAG(ArcLength, MATH_HALFPI, 0)
END_CLASS_DEFAULT(CMiniTurret, CTurretGun, LTNULL, LTNULL)

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CMiniTurret::CMiniTurret (0x10154000)
//
// ----------------------------------------------------------------------- //

CMiniTurret::CMiniTurret()
{
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CMiniTurret::~CMiniTurret (0x10154040)
//
// ----------------------------------------------------------------------- //

CMiniTurret::~CMiniTurret()
{
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CMiniTurret::OnPrecreate (0x10154050)
//
// ----------------------------------------------------------------------- //

uint32 CMiniTurret::OnPrecreate(void *pData, uint32 nInfo)
{
	CTurretGun::OnPrecreate(pData, nInfo);

	if ((LTFLOAT)nInfo == PRECREATE_WORLDFILE)
	{
		ObjectCreateStruct* pStruct = (ObjectCreateStruct*)pData;
		SAFE_STRCPY(pStruct->m_Filename, "models/exp/weapons/turret/miniturret.abc");
		SAFE_STRCPY(pStruct->m_SkinName, "skins/exp/weapons/turret.dtx");
	}

	return LTTRUE;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CMiniTurret::OnInitialUpdate (0x101540c0)
//
// ----------------------------------------------------------------------- //

uint32 CMiniTurret::OnInitialUpdate(uint32 nInfo)
{
	CTurretGun::OnInitialUpdate(nInfo);
	return LTTRUE;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CMiniTurret::OnUpdate (0x101540e0)
//
// ----------------------------------------------------------------------- //

uint32 CMiniTurret::OnUpdate(LTFLOAT fFrameTime)
{
	CTurretGun::OnUpdate(fFrameTime);
	SetNextUpdate(0.001f);
	return LTTRUE;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CMiniTurret::OnLoad (0x10154110)
//
// ----------------------------------------------------------------------- //

void CMiniTurret::OnLoad(HMESSAGEREAD hRead)
{
	CTurretGun::OnLoad(hRead);
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CMiniTurret::OnSave (0x10154120)
//
// ----------------------------------------------------------------------- //

void CMiniTurret::OnSave(HMESSAGEWRITE hWrite)
{
	CTurretGun::OnSave(hWrite);
}

#endif // PRIMAL_HUNT
