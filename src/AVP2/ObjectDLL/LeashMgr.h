// ----------------------------------------------------------------------- //
//
// MODULE  : LeashMgr.h
//
// PURPOSE : The leash manager: named groups of leash nodes (LeashPoints
//           and LeashVolumes) with the routes between them
//
//           Primal Hunt (Third Law). No source shipped: written from
//           PHObject.lto, 0x10150250..0x10151926. CBaseIterator,
//           CHashIterator<CLeashGroup*> and CLeashNode are RTTI names; the
//           rest (the file's name too) are ours.
//
// ----------------------------------------------------------------------- //

#ifndef __LEASH_MGR_H__
#define __LEASH_MGR_H__

#ifdef PRIMAL_HUNT

#include <math.h>
#include "LinkList.h"

class CLeashNode;
class CLeashPoint;
class CLeashVolume;

#define LEASHNODE_NONE		0
#define LEASHNODE_POINT		1
#define LEASHNODE_VOLUME	2

// A group's handle is its CLeashGroup pointer, which the manager's table
// is keyed on.
typedef uint32 HLEASHGROUP;

// ----------------------------------------------------------------------- //
// The hash table of groups

template <class T>
class THashNode
{
	public :

		THashNode() { m_nKey = 0; m_pNext = LTNULL; }

		T				m_Data;
		uint32			m_nKey;
		THashNode<T>*	m_pNext;
};

template <class T>
class CHashTable
{
	public :

		CHashTable(uint32 nBuckets)
		{
			m_nBuckets = nBuckets;
			m_ppBuckets = new THashNode<T>*[m_nBuckets];
			for (uint32 i = 0; i < m_nBuckets; i++)
			{
				m_ppBuckets[i] = LTNULL;
			}
		}

		~CHashTable()
		{
			RemoveAll();
			if (m_ppBuckets)
			{
				delete [] m_ppBuckets;
				m_ppBuckets = LTNULL;
			}
		}

		uint32 Hash(uint32 nKey)
		{
			LTFLOAT fKey = (LTFLOAT)(nKey & 0xFFFF) * 0.618034f;
			LTFLOAT fFrac = fKey - floor(fKey);
			return (uint32)(fFrac * m_nBuckets);
		}

		void Add(T data, uint32 nKey)
		{
			uint32 nBucket = Hash(nKey);
			THashNode<T>* pNode = new THashNode<T>;
			pNode->m_Data = data;
			pNode->m_nKey = nKey;
			pNode->m_pNext = m_ppBuckets[nBucket];
			m_ppBuckets[nBucket] = pNode;
		}

		T Find(uint32 nKey)
		{
			for (THashNode<T>* pNode = m_ppBuckets[Hash(nKey)]; pNode; pNode = pNode->m_pNext)
			{
				if (pNode->m_nKey == nKey) return pNode->m_Data;
			}

			return LTNULL;
		}

		void RemoveAll()
		{
			for (uint32 i = 0; i < m_nBuckets; i++)
			{
				THashNode<T>* pNode = m_ppBuckets[i];
				if (pNode)
				{
					while (pNode)
					{
						THashNode<T>* pNext = pNode->m_pNext;
						delete pNode;
						pNode = pNext;
					}

					m_ppBuckets[i] = LTNULL;
				}
			}
		}

		THashNode<T>**	m_ppBuckets;
		uint32			m_nBuckets;
};

class CBaseIterator
{
	public :

		virtual ~CBaseIterator() {}

		virtual void	SetContainer(void* pContainer) = 0;
		virtual void	First() = 0;
		virtual void	Next() = 0;
		virtual void*	Get() = 0;
};

template <class T>
class CHashIterator : public CBaseIterator
{
	public :

		CHashIterator(CHashTable<T>* pTable)
		{
			m_pTable = pTable;
			First();
		}

		virtual void SetContainer(void* pContainer) { m_pTable = (CHashTable<T>*)pContainer; }	// 0x10151640

		// 0x10151650
		virtual void First()
		{
			m_nBucket = 0;
			while (m_nBucket < m_pTable->m_nBuckets)
			{
				if (m_pTable->m_ppBuckets[m_nBucket])
				{
					m_pNode = m_pTable->m_ppBuckets[m_nBucket];
					return;
				}

				m_nBucket++;
			}

			m_pNode = LTNULL;
		}

		// 0x10151690
		virtual void Next()
		{
			if (!m_pNode) return;

			if (m_pNode)
			{
				m_pNode = m_pNode->m_pNext;
				if (m_pNode) return;
			}

			for (m_nBucket++; m_nBucket < m_pTable->m_nBuckets; m_nBucket++)
			{
				if (m_pTable->m_ppBuckets[m_nBucket])
				{
					m_pNode = m_pTable->m_ppBuckets[m_nBucket];
					return;
				}
			}

			m_pNode = LTNULL;
		}

		// 0x101516e0
		virtual void* Get()
		{
			if (!m_pNode) return LTNULL;
			return m_pNode->m_Data;
		}

	protected :

		CHashTable<T>*	m_pTable;	// +0x04
		THashNode<T>*	m_pNode;	// +0x08
		uint32			m_nBucket;	// +0x0c
};

// ----------------------------------------------------------------------- //
// A leash node: a LeashPoint's sphere or a LeashVolume's hull

struct LeashRoute
{
	LeashRoute() { m_nNext = -1; m_nHops = -1; }

	int		m_nNext;	// the next node toward the route's owner
	int		m_nHops;
};

class CLeashNode
{
	public :

		CLeashNode(int nType)
		{
			m_nType = nType;
			m_bVisited = LTFALSE;
			m_nID = -1;
			m_nRoutes = 0;
			m_pRoutes = LTNULL;
		}

		virtual ~CLeashNode();												// 0x10151750

		virtual void		Term();											// 0x10151800
		virtual void		SetPos(const LTVector& vPos) = 0;
		virtual LTVector	GetPos() = 0;
		virtual LTBOOL		IsNeighbor(CLeashNode* pNode);					// 0x10151830
		virtual void		AddNeighbor(CLeashNode* pNode);					// 0x10151860
		virtual void		ClearNeighbors();								// 0x10151870
		virtual LTBOOL		Contains(const LTVector& vPos, LTFLOAT fEpsilon);	// 0x101518b0
		virtual CLeashNode*	FindNeighbor(const LTVector& vPos, LTFLOAT fEpsilon);	// 0x101518c0
		virtual LTBOOL		GetExitPos(CLeashNode* pTo, const LTVector& vPos, LTVector* pvExit, LTFLOAT fRadius) = 0;
		virtual LTBOOL		GetEntryPos(CLeashNode* pTo, const LTVector& vPos, LTVector* pvEntry, LTFLOAT fRadius) = 0;
		virtual CLeashNode*	GetNeighbor(int nID);							// 0x10151900

		void AllocRoutes(int nRoutes)
		{
			m_nRoutes = nRoutes;
			m_pRoutes = new LeashRoute[nRoutes];
		}

		void SetRoute(int nID, int nNext, int nHops)
		{
			if (m_pRoutes && nID < m_nRoutes)
			{
				m_pRoutes[nID].m_nNext = nNext;
				m_pRoutes[nID].m_nHops = nHops;
			}
		}

		LeashRoute* GetRoute(int nID)
		{
			if (!m_pRoutes || nID >= m_nRoutes) return LTNULL;
			return &m_pRoutes[nID];
		}

		int							m_nType;		// +0x04
		int							m_nID;			// +0x08
		LeashRoute*					m_pRoutes;		// +0x0c, indexed by the start node's id
		int							m_nRoutes;		// +0x10
		int							m_nPrev;		// +0x14, route building
		LTBOOL						m_bVisited;		// +0x18, route building
		LTFLOAT						m_fDist;		// +0x1c, route building
		TLinkList<CLeashNode*>		m_Neighbors;	// +0x20
};

// ----------------------------------------------------------------------- //
// A named group of leash nodes

class CLeashGroup
{
	public :

		CLeashGroup();												// 0x10150290
		~CLeashGroup();												// 0x101502c0

		LTBOOL			AddNode(CLeashNode* pNode);					// 0x10150390
		LTBOOL			AddPoint(CLeashNode* pNode);				// 0x101503c0
		LTBOOL			AddVolume(CLeashNode* pNode);				// 0x101503f0
		LTBOOL			Init();										// 0x10150420
		void			Clear();									// 0x10150440
		LTBOOL			BuildRoutes();								// 0x101504d0
		CLeashNode*		GetPoint(int nID);							// 0x10150510
		CLeashNode*		GetVolume(int nID);							// 0x10150540
		LTBOOL			BuildPointRoutes(CLeashNode* pDest);		// 0x10150570
		LTBOOL			BuildVolumeRoutes(CLeashNode* pDest);		// 0x101507f0
		void			BuildNeighbors();							// 0x10150a30
		void			BuildPointNeighbors();						// 0x10150a50
		void			BuildVolumeNeighbors();						// 0x10150aa0
		CLeashNode*		FindPoint(const LTVector& vPos);			// 0x10150b90
		CLeashNode*		FindVolume(const LTVector& vPos);			// 0x10150c40
		LTBOOL			GetNextNode(const LTVector& vFrom, const LTVector& vTo,
							int* pnNext, int* pnDest, int* pnStart);	// 0x10150c80
		LTBOOL			GetExitPos(int nFrom, int nTo, const LTVector& vPos,
							LTVector* pvExit, LTFLOAT fRadius);			// 0x10150d40
		LTBOOL			GetEntryPos(int nFrom, int nTo, const LTVector& vPos,
							LTVector* pvEntry, LTFLOAT fRadius);		// 0x10150db0

		TLinkList<CLeashPoint*>		m_Points;		// +0x00
		TLinkList<CLeashVolume*>	m_Volumes;		// +0x10
		char						m_szName[32];	// +0x20
		HLEASHGROUP					m_hGroup;		// +0x40
		int							m_nNextID;		// +0x44
};

// ----------------------------------------------------------------------- //
// The manager (one, g_LeashMgr at 0x10337788)

class CLeashMgr
{
	public :

		CLeashMgr();												// 0x10150e20
		~CLeashMgr();												// 0x10150eb0

		HLEASHGROUP		GetGroup(const char* szName);				// 0x10150f20
		LTBOOL			Init();										// 0x10151010
		LTBOOL			AddNode(HLEASHGROUP hGroup, CLeashNode* pNode);	// 0x10151110
		void			Clear();									// 0x10151130
		CLeashGroup*	GetGroup(HLEASHGROUP hGroup);				// 0x10151240
		CLeashGroup*	FindGroup(const char* szName);				// 0x101512d0
		CLeashNode*		GetNode(HLEASHGROUP hGroup, int nID);		// 0x10151360
		CLeashNode*		FindNode(HLEASHGROUP hGroup, const LTVector& vPos);	// 0x101513a0
		LTBOOL			GetNextNode(HLEASHGROUP hGroup, const LTVector& vFrom, const LTVector& vTo,
							int* pnNext, int* pnDest, int* pnStart);	// 0x101513e0
		LTBOOL			GetExitPos(HLEASHGROUP hGroup, int nFrom, int nTo, const LTVector& vPos,
							LTVector* pvExit, LTFLOAT fRadius);			// 0x10151420
		LTBOOL			GetEntryPos(HLEASHGROUP hGroup, int nFrom, int nTo, const LTVector& vPos,
							LTVector* pvEntry, LTFLOAT fRadius);		// 0x10151460

	private :

		CHashTable<CLeashGroup*>*	m_pGroups;		// +0x00
		LTBOOL						m_bActive;		// +0x04 (name ours; set, never read)
};

// The rest of the game reaches the manager through these.
HLEASHGROUP		LeashMgr_GetGroup(const char* szName);									// 0x101514a0
LTBOOL			LeashMgr_Init();														// 0x101514b0
LTBOOL			LeashMgr_AddNode(HLEASHGROUP hGroup, CLeashNode* pNode);				// 0x101514c0
void			LeashMgr_Clear();														// 0x101514e0
CLeashNode*		LeashMgr_GetNode(HLEASHGROUP hGroup, int nID);							// 0x101514f0
CLeashNode*		LeashMgr_FindNode(HLEASHGROUP hGroup, const LTVector& vPos);			// 0x10151510
LTBOOL			LeashMgr_GetNextNode(HLEASHGROUP hGroup, const LTVector& vFrom, const LTVector& vTo,
					int* pnNext, int* pnDest, int* pnStart);							// 0x10151530
LTBOOL			LeashMgr_GetExitPos(HLEASHGROUP hGroup, int nFrom, int nTo, const LTVector& vPos,
					LTVector* pvExit, LTFLOAT fRadius);									// 0x10151560
LTBOOL			LeashMgr_GetEntryPos(HLEASHGROUP hGroup, int nFrom, int nTo, const LTVector& vPos,
					LTVector* pvEntry, LTFLOAT fRadius);								// 0x10151590

#endif // PRIMAL_HUNT

#endif // __LEASH_MGR_H__
