// *********************************************************************** //
//
//  w10_1007c250.cpp -- d3d.ren functions written for the whole-DLL link,
//  /O2 group (the w<N>_*.cpp default, see build/engine.sh).
//
//  Why /O2: each function here is followed by /O2's multi-byte lea-nop
//  padding (`8d b4 26 00 00 00 00` / `8d bc 27 00 00 00 00`), and the
//  CMoArray pair duplicates its epilogue at every exit.
//
// *********************************************************************** //

#include "ltbasedefs.h"


// ======================================================================= //
//  0x1007c250, 49 bytes.  __thiscall (pOwner, pszName), `ret $0x8`,
//  returns 1.  1 caller: 0x1007c140 (d3dbatchc_o2.cpp), which passes its
//  own first two arguments straight through and discards the answer.
//
//  Links the object's node at +0x08 onto the TAIL of the circular list
//  whose head node pOwner points at, then records the name at +0x18:
//
//      mov (%eax),%eax          pHead = *pOwner
//      lea 0x8(%ecx),%edx       &m_Link
//      mov %eax,0x8(%ecx)       m_Link.m_pNext = pHead
//      mov 0x4(%eax),%eax       pHead->m_pPrev   (from the register)
//      mov %eax,0xc(%ecx)       m_Link.m_pPrev = it
//      mov %edx,(%eax)          m_Link.m_pPrev->m_pNext = &m_Link
//      mov 0x8(%ecx),%eax       m_Link.m_pNext reloaded (the store above
//      mov %edx,0x4(%eax)         may alias it) ->m_pPrev = &m_Link
//
//  The link is skipped when pOwner is NULL; the name store is not.
// ======================================================================= //
struct Link1007c250
{
	Link1007c250*	m_pNext;                            // +0x00
	Link1007c250*	m_pPrev;                            // +0x04
};

class C1007c140
{
public:
	LTBOOL sub_1007c250(void* pOwner, const char* pszVarName);

	uint32*			m_pTable;                           // +0x00
	uint32			m_n04;                              // +0x04
	Link1007c250	m_Link08;                           // +0x08
	uint32			m_n10;                              // +0x10
	int32			m_nCount14;                         // +0x14
	const char*		m_pszName18;                        // +0x18
};

LTBOOL C1007c140::sub_1007c250(void* pOwner, const char* pszVarName)
{
	if (pOwner)
	{
		Link1007c250* pHead = *(Link1007c250**)pOwner;
		Link1007c250* pLink = &m_Link08;

		pLink->m_pNext = pHead;
		pLink->m_pPrev = pHead->m_pPrev;
		pLink->m_pPrev->m_pNext = pLink;
		pLink->m_pNext->m_pPrev = pLink;
	}

	m_pszName18 = pszVarName;
	return LTTRUE;
}


// ======================================================================= //
//  0x1007c760 (SetSize2, 216 bytes) and 0x1007c890 (Remove2, 307 bytes):
//  stdlith dynarray.h's CMoArray<T*>::SetSize2 and ::Remove2, T four bytes
//  wide (`mov 0x4(%edx,%eax,4)` / `(%edx,%eax,4)` in the copy loops), with
//  every helper CALLED rather than expanded -- the image reaches each one
//  at its own address:
//
//      0x1000a670  m_Cache.GetWantedCache()     (this+0x0c)
//      0x1000a680  m_Cache.SetCacheSize(n)
//      0x1000a690  m_Cache.GetCacheSize()
//      0x10021a80  GetNumAllocatedElements()
//      0x10021aa0  _AllocateTArray(n, pAlloc)   (n*4 bytes through +0x04)
//      0x1001cf00  _DeleteAndDestroyArray(pAlloc, n)
//
//  Layout: +0x00 vptr (GenList), +0x04 m_pArray, +0x08 m_nElements,
//  +0x0c DefaultCache.  The helpers are declared with this class and
//  resolved onto their existing definitions in link/aliases.rsp.
//
//  Callers: SetSize2 from 0x1007c85b and 0x1007ca4b (Term()'s
//  `SetSize2(0, pAlloc)`), Remove2 from 0x1007c9eb (Remove()'s
//  `Remove2(index, &g_DefAlloc)`, d3dr2c.cpp).
//
//  IDENTICAL, both (were 110 and 203 bytes off).  The bodies
//  were already dynarray.h verbatim; what moved them was the SPELLING OF
//  THE HELPERS: the cache class named `DefaultCache` with dynarray.h's
//  GetWantedCache / SetCacheSize / GetCacheSize (const as in the header),
//  and the array's own helpers named _AllocateTArray /
//  _DeleteAndDestroyArray / GetNumAllocatedElements.  Measured, same
//  bodies and types throughout: with the sub_<addr> names both functions
//  stay 110 / 203 off; `DefaultCache` alone closes Remove2; the array
//  helpers' names close SetSize2.  icl's output depends on the callees'
//  NAMES here, not only on their declarations (w2_1000a110.cpp, whose
//  sixteen-byte instantiation matches, spells them the dynarray.h way).
//  Return type (LTBOOL / int32) and IAllocator as a class make no
//  difference.  The real template (dynarray.h's CMoArray<T*>) also
//  compiles to both shipped bodies, but only at /O2 /Ob0.
// ======================================================================= //
struct IAllocator;

struct DefaultCache
{
	uint32	GetWantedCache() const;                     // 0x1000a670
	void	SetCacheSize(uint32 val);                   // 0x1000a680
	uint32	GetCacheSize() const;                       // 0x1000a690

	uint32	m_CacheSize;                                // +0x00
	uint32	m_WantedCache;                              // +0x04
};

struct MoArray1007c760
{
	void*			m_pVtbl00;                          // +0x00
	void**			m_pArray;                           // +0x04
	uint32			m_nElements;                        // +0x08
	DefaultCache	m_Cache;                            // +0x0c

	uint32	GetNumAllocatedElements();                             // GetNumAllocatedElements
	void**	_AllocateTArray(uint32 nElements, IAllocator* pAlloc);
	void	_DeleteAndDestroyArray(IAllocator* pAlloc, uint32 nElements);

	LTBOOL	sub_1007c760(uint32 newSize, IAllocator* pAlloc);
	void	sub_1007c890(uint32 index, IAllocator* pAlloc);
};


// 0x1007c760 -- SetSize2, dynarray.h verbatim.
LTBOOL MoArray1007c760::sub_1007c760(uint32 newSize, IAllocator* pAlloc)
{
	if ((newSize == m_nElements) && (newSize != 0))
	{
		return LTTRUE;
	}

	if (m_pArray)
	{
		_DeleteAndDestroyArray(pAlloc, GetNumAllocatedElements());
	}

	m_nElements = newSize;
	if (newSize > 0)
	{
		m_pArray = _AllocateTArray(newSize + m_Cache.GetWantedCache(), pAlloc);
		if (!m_pArray)
		{
			m_nElements = 0;
			m_Cache.SetCacheSize(0);
			return LTFALSE;
		}

		m_Cache.SetCacheSize(m_Cache.GetWantedCache());
	}

	return LTTRUE;
}


// 0x1007c890 -- Remove2, dynarray.h verbatim.
void MoArray1007c760::sub_1007c890(uint32 index, IAllocator* pAlloc)
{
	uint32 i, newSize, newAllocSize;
	void** pNewArray;
	LTBOOL bSlideDown;

	bSlideDown = LTTRUE;
	if (m_Cache.GetCacheSize() >= (m_Cache.GetWantedCache() * 2))
	{
		newSize = m_nElements - 1;
		newAllocSize = newSize + m_Cache.GetWantedCache();
		pNewArray = _AllocateTArray(newAllocSize, pAlloc);

		if (pNewArray || newAllocSize == 0)
		{
			for (i = 0; i < index; i++)
				pNewArray[i] = m_pArray[i];

			for (i = index; i < m_nElements - 1; i++)
				pNewArray[i] = m_pArray[i + 1];

			_DeleteAndDestroyArray(pAlloc, GetNumAllocatedElements());
			m_pArray = pNewArray;

			m_Cache.SetCacheSize(m_Cache.GetWantedCache());
			bSlideDown = LTFALSE;
		}
	}

	if (bSlideDown)
	{
		m_Cache.SetCacheSize(m_Cache.GetCacheSize() + 1);

		for (i = index; i < m_nElements - 1; i++)
			m_pArray[i] = m_pArray[i + 1];
	}

	--m_nElements;
}
