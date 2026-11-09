// ----------------------------------------------------------------------- //
//
// MODULE  : TurretInfoMgr.cpp
//
// PURPOSE : Primal Hunt: what the client knows of the turrets about it
//
// CREATED : Primal Hunt (Third Law); no source survives, written from
//           PHCShell.dll (0x10136bb0 to 0x10136e5a; names ours).
//
// ----------------------------------------------------------------------- //

#include "stdafx.h"

#ifdef PRIMAL_HUNT

#include "TurretInfoMgr.h"

// ----------------------------------------------------------------------- //

CTurretInfoMgr g_TurretInfoMgr;

// ----------------------------------------------------------------------- //

CTurretInfoMgr::CTurretInfoMgr()
{
	m_nNumTurrets	= 0;
	m_nMaxTurrets	= 20;
	m_hTarget		= LTNULL;
	m_nCarried		= 0;
	m_pTurrets		= LTNULL;
}

// ----------------------------------------------------------------------- //

CTurretInfoMgr::~CTurretInfoMgr()
{
	if(m_pTurrets)
		delete [] m_pTurrets;

	m_pTurrets = LTNULL;
}

// ----------------------------------------------------------------------- //

void CTurretInfoMgr::Init(int nMaxTurrets)
{
	Term();

	m_nMaxTurrets	= nMaxTurrets;
	m_nNumTurrets	= 0;
	m_hTarget		= LTNULL;
	m_nCarried		= 0;
	m_pTurrets		= new TURRETINFO[nMaxTurrets];
}

// ----------------------------------------------------------------------- //

void CTurretInfoMgr::Term()
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
//
//	ROUTINE:	CTurretInfoMgr::Set()
//
//	PURPOSE:	A turret's ammo and heat (MID_TURRET_AMMO)
//
// ----------------------------------------------------------------------- //

void CTurretInfoMgr::Set(HOBJECT hTurret, int nAmmo, int nHeat)
{
	TURRETINFO* pInfo = Find(hTurret);

	if(pInfo)
	{
		g_pLTClient->SetObjectClientFlags(hTurret, CF_NOTIFYREMOVE);
		pInfo->nAmmo = nAmmo;
		pInfo->nHeat = nHeat;
		return;
	}

	if(m_nNumTurrets < m_nMaxTurrets)
	{
		g_pLTClient->SetObjectClientFlags(hTurret, CF_NOTIFYREMOVE);
		m_pTurrets[m_nNumTurrets].hTurret = hTurret;
		m_pTurrets[m_nNumTurrets].nAmmo = nAmmo;
		m_pTurrets[m_nNumTurrets].nHeat = nHeat;
		m_nNumTurrets++;
	}
}

// ----------------------------------------------------------------------- //

TURRETINFO* CTurretInfoMgr::Find(HOBJECT hTurret)
{
	if(!hTurret)
		return LTNULL;

	for(int i = 0; i < m_nNumTurrets; i++)
	{
		if(m_pTurrets[i].hTurret == hTurret)
			return &m_pTurrets[i];
	}

	return LTNULL;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CTurretInfoMgr::Remove()
//
//	PURPOSE:	The turret is gone (CGameClientShell::OnObjectRemove)
//
// ----------------------------------------------------------------------- //

void CTurretInfoMgr::Remove(HOBJECT hTurret)
{
	int nIndex = -1;

	for(int i = 0; i < m_nNumTurrets; i++)
	{
		if(m_pTurrets[i].hTurret == hTurret)
		{
			nIndex = i;
			break;
		}
	}

	if(nIndex < 0)
		return;

	m_nNumTurrets--;
	memcpy(&m_pTurrets[nIndex], &m_pTurrets[nIndex + 1], (m_nNumTurrets - nIndex) * sizeof(TURRETINFO));
}

// ----------------------------------------------------------------------- //

void CTurretInfoMgr::GetInfo(HOBJECT hTurret, int &nAmmo, int &nHeat)
{
	TURRETINFO* pInfo = Find(hTurret);

	if(pInfo)
	{
		nAmmo = pInfo->nAmmo;
		nHeat = pInfo->nHeat;
	}
}

// ----------------------------------------------------------------------- //

void CTurretInfoMgr::GetTargetInfo(int &nAmmo, int &nHeat)
{
	GetInfo(m_hTarget, nAmmo, nHeat);
}

// ----------------------------------------------------------------------- //

void InitTurretInfo(int nMaxTurrets)
{
	if(GetTurretInfoMgr())
		GetTurretInfoMgr()->Init(nMaxTurrets);
}

// ----------------------------------------------------------------------- //

CTurretInfoMgr* GetTurretInfoMgr()
{
	return &g_TurretInfoMgr;
}

// ----------------------------------------------------------------------- //

void RemoveTurretInfo(HOBJECT hTurret)
{
	if(GetTurretInfoMgr())
		GetTurretInfoMgr()->Remove(hTurret);
}

#endif // PRIMAL_HUNT
