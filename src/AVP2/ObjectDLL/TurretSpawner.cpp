// ----------------------------------------------------------------------- //
//
// MODULE  : TurretSpawner.cpp
//
// PURPOSE : CTurretSpawner - implementation
//
// CREATED : Primal Hunt (Third Law); no source survives, written from
//           PHObject.lto (0x1015a560..0x1015abd0)
//
// ----------------------------------------------------------------------- //

#include "stdafx.h"

#ifdef PRIMAL_HUNT

#include "TurretSpawner.h"

// No more than this many spawned turrets in the world at once.
#define MAX_SPAWNED_TURRETS		6

// turret.cpp's: creates a carryable turret for its spawner
// (PHObject.lto 0x10091e40).
HOBJECT SpawnTurret(HOBJECT hSpawner, const LTVector & vPos, const LTRotation & rRot);

// PH: the $E initializer 0x1015a560
BEGIN_CLASS(CTurretSpawner)
	ADD_VECTORPROP_VAL_FLAG(Dims, 28.0f, 41.0f, 28.0f, PF_HIDDEN | PF_DIMS | PF_LOCALDIMS)
	ADD_REALPROP_FLAG_HELP(Spawn_Interval, 30.0f, 0, "Time between respawns.  Will only spawn a new turret if the current turret was picked up.")
END_CLASS_DEFAULT(CTurretSpawner, GameBase, NULL, NULL)

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CTurretSpawner::CTurretSpawner()
//
// ----------------------------------------------------------------------- //

// PH: 0x1015a710
CTurretSpawner::CTurretSpawner() : GameBase(OT_NORMAL)
{
	m_fSpawnInterval = 30.0f;
	m_hTurret = LTNULL;
}

// PH: 0x1015a7c0
CTurretSpawner::~CTurretSpawner()
{
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CTurretSpawner::OnPrecreate()
//
// ----------------------------------------------------------------------- //

// PH: 0x1015a830
uint32 CTurretSpawner::OnPrecreate(void *pData, uint32 nInfo)
{
	ObjectCreateStruct* pStruct = (ObjectCreateStruct*)pData;

	GameBase::OnPrecreate(pData, nInfo);

	if (nInfo == PRECREATE_WORLDFILE)
	{
		g_pLTServer->GetPropReal("Spawn_Interval", &m_fSpawnInterval);
	}

	pStruct->m_ObjectType = OT_NORMAL;
	pStruct->m_Flags = 0;

	return 0;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CTurretSpawner::OnInitialUpdate()
//
// ----------------------------------------------------------------------- //

// PH: 0x1015a890
uint32 CTurretSpawner::OnInitialUpdate(uint32 nInfo)
{
	GameBase::OnInitialUpdate(nInfo);

	SetNextUpdate(m_fSpawnInterval);

	return 0;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CTurretSpawner::OnUpdate()
//
// ----------------------------------------------------------------------- //

// PH: 0x1015a8b0
uint32 CTurretSpawner::OnUpdate(LTFLOAT fFrameTime)
{
	SetNextUpdate(m_fSpawnInterval);

	if (NeedsTurret() && GetTurretCount() < MAX_SPAWNED_TURRETS)
	{
		Spawn();
	}

	return 0;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CTurretSpawner::OnTrigger()
//
//	PURPOSE:	Our turret tells us when it is picked up or destroyed
//
// ----------------------------------------------------------------------- //

// PH: 0x1015a8f0
uint32 CTurretSpawner::OnTrigger(HOBJECT hSender, const char* szMsg)
{
	if (!szMsg) return 0;
	if (hSender != m_hTurret) return 0;

	if (_stricmp(szMsg, "PICKED_UP") == 0)
	{
		m_hTurret = LTNULL;
	}
	else if (_stricmp(szMsg, "DESTROYED") == 0)
	{
		m_hTurret = LTNULL;
	}

	return GameBase::OnTrigger(hSender, szMsg);
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CTurretSpawner::OnSave()
//
// ----------------------------------------------------------------------- //

// PH: 0x1015a9c0
void CTurretSpawner::OnSave(HMESSAGEWRITE hWrite)
{
	GameBase::OnSave(hWrite);

	*hWrite << m_hTurret;
	*hWrite << m_fSpawnInterval;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CTurretSpawner::OnLoad()
//
// ----------------------------------------------------------------------- //

// PH: 0x1015aa00
void CTurretSpawner::OnLoad(HMESSAGEREAD hRead)
{
	GameBase::OnLoad(hRead);

	m_hTurret = hRead->ReadObject();
	m_fSpawnInterval = hRead->ReadFloat();
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CTurretSpawner::Spawn()
//
// ----------------------------------------------------------------------- //

// PH: 0x1015aaa0
void CTurretSpawner::Spawn()
{
	LTVector vPos;
	g_pLTServer->GetObjectPos(m_hObject, &vPos);

	LTRotation rRot;
	g_pLTServer->GetObjectRotation(m_hObject, &rRot);

	m_hTurret = SpawnTurret(m_hObject, vPos, rRot);

	if (m_hTurret)
	{
		IncTurretCount();
	}
}

// PH: 0x1015ab60
LTBOOL CTurretSpawner::NeedsTurret()
{
	if (!m_hTurret) return LTTRUE;

	return LTFALSE;
}

// ----------------------------------------------------------------------- //
//
//	The spawned-turret count (0x1015ab80..0x1015abcb)
//
// ----------------------------------------------------------------------- //

// PH: 0x1015ab80
void CTurretSpawner::DecTurretCount()
{
	if (--s_nTurretCount < 0)
	{
		s_nTurretCount = 0;
	}
}

// PH: 0x1015aba0
void CTurretSpawner::IncTurretCount()
{
	++s_nTurretCount;
}

// PH: 0x1015abb0
int CTurretSpawner::GetTurretCount()
{
	return s_nTurretCount;
}

// PH: 0x1015abc0
void CTurretSpawner::ResetTurretCount()
{
	s_nTurretCount = 0;
}

int CTurretSpawner::s_nTurretCount = 0;

#endif // PRIMAL_HUNT
