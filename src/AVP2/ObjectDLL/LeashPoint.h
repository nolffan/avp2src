// ----------------------------------------------------------------------- //
//
// MODULE  : LeashPoint.h
//
// PURPOSE : LeashPoint - a sphere a leashed AI may roam, and CLeashPoint,
//           its node in the leash manager
//
//           Primal Hunt (Third Law). No source shipped: written from
//           PHObject.lto, 0x10151930..0x10152063 (LeashPoint's vtable
//           0x1028affc, CLeashPoint's 0x1028b048).
//
// ----------------------------------------------------------------------- //

#ifndef __LEASH_POINT_H__
#define __LEASH_POINT_H__

#ifdef PRIMAL_HUNT

#include "GameBase.h"
#include "LeashMgr.h"

class CLeashPoint : public CLeashNode
{
	public :

		CLeashPoint();												// 0x10151ca0
		virtual ~CLeashPoint();										// 0x10151d40

		virtual LTVector	GetPos();										// 0x10151ce0
		virtual void		SetPos(const LTVector& vPos);					// 0x10151d00
		virtual LTBOOL		Contains(const LTVector& vPos, LTFLOAT fEpsilon);	// 0x10151e10
		virtual LTBOOL		GetExitPos(CLeashNode* pTo, const LTVector& vPos, LTVector* pvExit, LTFLOAT fRadius);	// 0x10151e50
		virtual LTBOOL		GetEntryPos(CLeashNode* pTo, const LTVector& vPos, LTVector* pvEntry, LTFLOAT fRadius);	// 0x10151f60

		LTBOOL	Overlaps(const LTVector& vPos, LTFLOAT fRadius);			// 0x10151d50
		LTBOOL	Overlaps(CLeashNode* pNode);								// 0x10151dd0

		LTVector	m_vPos;		// +0x30
		LTFLOAT		m_fRadius;	// +0x3c
};

class LeashPoint : public GameBase
{
	public :

		LeashPoint();												// 0x10151ad0
		~LeashPoint() {}											// 0x10151b30

	protected :

		virtual void	PostStartWorld(uint8 nLoadGameFlags) {}		// 0x10151c90 (folded)
		virtual uint32	OnPrecreate(void *pData, uint32 nInfo);		// 0x10151b40
		virtual uint32	OnInitialUpdate(uint32 nInfo);				// 0x10151bb0

	private :

		CLeashPoint*	m_pNode;			// +0x50
		char			m_szGroupName[32];	// +0x54
		LTFLOAT			m_fRadius;			// +0x74
		HLEASHGROUP		m_hGroup;			// +0x78
};

#endif // PRIMAL_HUNT

#endif // __LEASH_POINT_H__
