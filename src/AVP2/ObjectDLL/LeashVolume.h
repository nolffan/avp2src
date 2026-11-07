// ----------------------------------------------------------------------- //
//
// MODULE  : LeashVolume.h
//
// PURPOSE : LeashVolume - a convex brush a leashed AI may roam, and
//           CLeashVolume, its node in the leash manager
//
//           Primal Hunt (Third Law). No source shipped: written from
//           PHObject.lto, 0x10152070..0x10153b3c (LeashVolume's vtable
//           0x1028b080, CLeashVolume's 0x1028b0cc).
//
// ----------------------------------------------------------------------- //

#ifndef __LEASH_VOLUME_H__
#define __LEASH_VOLUME_H__

#ifdef PRIMAL_HUNT

#include "GameBase.h"
#include "LeashMgr.h"
#include "LeashFace.h"

struct LeashBox
{
	LeashBox() {}

	// 0x10153a40
	LeashBox(const LTVector& vCenter, const LTVector& vExtents)
	{
		m_vMin = vCenter - vExtents;
		m_vMax = vCenter + vExtents;
	}

	LTVector GetExtents() const
	{
		return (m_vMax - m_vMin) * 0.5f;
	}

	LTVector GetCenter() const
	{
		return (m_vMin + m_vMax) * 0.5f;
	}

	LTVector	m_vMin;
	LTVector	m_vMax;
};

// Where two volumes' faces meet
struct CLeashPortal
{
	CLeashNode*		m_pNeighbor;	// +0x00
	LTVector		m_vStart;		// +0x04
	LTVector		m_vDir;			// +0x10
	LTFLOAT			m_fLength;		// +0x1c
	LTFLOAT			m_fHeight;		// +0x20
	LTVector		m_vMin;			// +0x24
	LTVector		m_vMax;			// +0x30
	LTPlane			m_Plane;		// +0x3c, the face's: out of this volume
};

class CLeashVolume : public CLeashNode
{
	public :

		CLeashVolume();												// 0x101527d0
		virtual ~CLeashVolume();									// 0x10152a10

		virtual void		Term();									// 0x10152b00
		virtual LTVector	GetPos();								// 0x101528d0
		virtual void		SetPos(const LTVector& vPos);			// 0x10152940
		virtual LTBOOL		Contains(const LTVector& vPos, LTFLOAT fEpsilon);			// 0x10152cd0
		virtual CLeashNode*	FindNeighbor(const LTVector& vPos, LTFLOAT fEpsilon);		// 0x10152d90
		virtual LTBOOL		GetExitPos(CLeashNode* pTo, const LTVector& vPos, LTVector* pvExit, LTFLOAT fRadius);	// 0x101531e0
		virtual LTBOOL		GetEntryPos(CLeashNode* pTo, const LTVector& vPos, LTVector* pvEntry, LTFLOAT fRadius);	// 0x101534f0

		void		ClearFaces();											// 0x10152b20
		void		ClearPortals();											// 0x10152b70
		LTBOOL		MergeFace(CLeashFace* pFace);							// 0x10152bc0
		LTBOOL		IsConvex(LTFLOAT fEpsilon);								// 0x10152c20
		void		FindPortals(CLeashVolume* pOther);						// 0x10152de0
		LTBOOL		AddPortal(CLeashFace* pFace, CLeashFace* pOtherFace, CLeashVolume* pOther);	// 0x10152ec0
		LTVector	GetExitOffset(const LTVector& vPos, CLeashPortal* pPortal, LTFLOAT fRadius);	// 0x10153120
		LTVector	GetEntryOffset(const LTVector& vPos, CLeashPortal* pPortal, LTFLOAT fRadius);	// 0x10153190
		void		ExtendBounds(const LTVector& vMin, const LTVector& vMax);	// 0x10153800

		CLeashPortal* GetPortal(CLeashNode* pTo)
		{
			for (TLinkNode<CLeashPortal*>* pNode = m_Portals.GetHead(); pNode != m_Portals.GetEnd(); pNode = pNode->m_pNext)
			{
				if (pNode->m_Data->m_pNeighbor == pTo) return pNode->m_Data;
			}

			return LTNULL;
		}

		TLinkList<CLeashPortal*>	m_Portals;		// +0x30
		TLinkList<CLeashFace*>		m_Faces;		// +0x40
		LeashBox					m_Box;			// +0x50
		LTBOOL						m_bShowPortals;	// +0x68
};

class LeashVolume : public GameBase
{
	public :

		LeashVolume();												// 0x10152260
		~LeashVolume();												// 0x101522a0

	protected :

		virtual void	PostStartWorld(uint8 nLoadGameFlags);		// 0x10152480
		virtual uint32	OnPrecreate(void *pData, uint32 nInfo);		// 0x101522f0
		virtual uint32	OnInitialUpdate(uint32 nInfo);				// 0x10152390
		virtual uint32	OnUpdate(LTFLOAT fFrameTime) { return LTFALSE; }	// 0x101524a0 (folded)

		void			Term();										// 0x101524b0 (folded)
		LTBOOL			BuildHull();								// 0x101524c0

	private :

		CLeashVolume*	m_pNode;			// +0x50
		LTBOOL			m_bShowVolume;		// +0x54
		LTBOOL			m_bShowPortals;		// +0x58
		char			m_szGroupName[128];	// +0x5c
};

#endif // PRIMAL_HUNT

#endif // __LEASH_VOLUME_H__
