// *********************************************************************** //
//  w57_1007bb20_qipni -- d3d.ren, /O2 /Qip /Qip_no_inlining:
//    0x1007b964   27 bytes  the per-list sub-object's constructor (register
//                           entry; also written in d3dbatchb.cpp)
//    0x1007b984  348 bytes  the renderer root's constructor (register
//                           entry; 0x1007b980 its stack shim)
//    0x1007bb20   48 bytes  the static initialiser (.data 0x10089200):
//                           ctor(&g_10091a10) ; atexit(0x1007bb50)
//    0x1007bb50  829 bytes  that atexit destructor
//
//  The object is the renderer root at 0x10091a10 that 0x1007b8d0 answers
//  (d3dqip4.cpp's Root1007aeb4, w26_1007be90.cpp's VisibleSet): an LTLink
//  head at +0x00, flags +0x0c, two polygon queues at +0x10 / +0x28 (a
//  CMoArray<Poly*> with vptr 0x100899f0 and a use count), the 32-entry
//  table at +0x40 with its count at +0x140, and fifteen 0x20-byte object
//  lists at +0x144.. (w26's names).
//
//  CONSTRUCTOR, read off 0x1007b984: the members in declaration order --
//  the two queues' CMoArray constructors (vptr, Clear, Init: the same
//  three zero groups as w35's base), the fifteen lists through 0x1007b964
//  out of line -- then the body: the head tied to itself (+0x04 first),
//  +0x0c = 1, the two queues' wanted cache 0x200 and 0x20, and 0x1007bae0
//  (d3dr4a.cpp, "reset the counters and walk the list") inlined.  The
//  queue constructors had to be MEMBER constructors: called in the body
//  they come after the fifteen calls.  /Qip_no_inlining keeps 0x1007b964
//  out of line (plain /Qip inlines it, 716 bytes); /Qip itself is what the
//  register-entry calls and the shims say.
//
//  DESTRUCTOR 0x1007bb50: the members in reverse -- each list's Term
//  (free the table, +0x04 and +0x14 zeroed; written +0x14 first because
//  icl emits the pair reversed), then the two queues' ~CMoArray (vptr
//  store, the Term chain through g_DefAlloc, m_nElements = 0), every field
//  absolute.  NEAR, 10 bytes: the two queue chains.  Shipped is
//  `je <cache=0> ; mov $g_DefAlloc,%ecx ; je ; mov g_DefAlloc,%edx ; ...`;
//  with dynarray.h's chain verbatim (BaseDelete's `if(!ptr) return` in its
//  own inline) icl lays the cold arm out the other way round (57 bytes);
//  written as `if (m_pArray) pAlloc->Free(m_pArray)` inside
//  _DeleteAndDestroyArray the layout is shipped's and only the
//  `mov $g_DefAlloc,%ecx` sinks below the vtable load (10 bytes, this
//  file).  Measured without effect: BaseDelete without its destruct loop,
//  `if (ptr)` instead of the early return, SetSize2 without its dead first
//  test, Term without SetSize2, the allocator through a local, the cache
//  class named DefaultCache (the cold-name lever), plain /Qip with the
//  list constructor defined after the root's.
//
//  The list's table is released with free(), as d3dbatchc_o2.cpp's
//  0x1007c140 releases it (0x10080479 in both).  The root's and the
//  lists' classes are named for the stack shims so the byte comparison measures the
//  `$` register entries.  /Qip also emits out-of-line copies of the
//  inline CMoArray helpers; they are unreferenced.
//  New global: 0x10091a10 (defined here); g_vtbl_100899f0 referenced.
// *********************************************************************** //

#include "ltbasedefs.h"
#include <stdlib.h>

struct IAllocator
{
	virtual void*	Slot00(uint32 nSize);               // vtable +0x00
	virtual void*	Alloc(uint32 nSize);                // vtable +0x04
	virtual void	Free(void* pMem);                   // vtable +0x08
};

extern IAllocator g_10091df0;                           // 0x10091df0

extern void* g_vtbl_100899f0[];                         // 0x100899f0
extern uint8 g_100a9810[];                              // 0x100a9810

template<class T>
inline void BaseDelete1007bb50(IAllocator* pAlloc, T* ptr, uint32 nElements)
{
	T* pTemp;

	if (!ptr)
		return;

	pTemp = ptr;
	while (nElements)
	{
		--nElements;
		pTemp->~T();
		++pTemp;
	}

	pAlloc->Free(ptr);
}

struct DefCache1007bb50
{
	uint32	m_CacheSize;                                // +0x00
	uint32	m_WantedCache;                              // +0x04

	uint32	GetCacheSize() const	{ return m_CacheSize; }
	void	SetCacheSize(uint32 n)	{ m_CacheSize = n; }
	uint32	GetWantedCache() const	{ return m_WantedCache; }
	void	SetWantedCache(uint32 n){ m_WantedCache = n; }
};

struct MoArray1007bb50
{
	void**			m_pVTable;                      // +0x00
	void**			m_pArray;                       // +0x04
	uint32			m_nElements;                    // +0x08
	DefCache1007bb50	m_Cache;                        // +0x0c

	MoArray1007bb50()
	{
		Construct(g_vtbl_100899f0);
	}

	~MoArray1007bb50()
	{
		m_pVTable = g_vtbl_100899f0;
		Term();
	}

	void	Clear()
	{
		m_pArray = 0;
		m_nElements = 0;
		m_Cache.SetCacheSize(0);
		m_Cache.SetWantedCache(0);
	}

	void	_InitArray(uint32 wantedCache)
	{
		m_pArray = LTNULL;
		m_nElements = 0;

		m_Cache.SetWantedCache(wantedCache);
		m_Cache.SetCacheSize(0);
	}

	uint32	GetNumAllocatedElements() const
	{
		return m_nElements + m_Cache.GetCacheSize();
	}

	void	_DeleteAndDestroyArray(IAllocator* pAlloc, uint32 nElements)
	{
		if (m_pArray)
		{
			if (m_pArray != LTNULL)
				pAlloc->Free(m_pArray);
			m_pArray = LTNULL;
		}

		m_Cache.SetCacheSize(0);
	}

	LTBOOL	SetSize2(uint32 newSize, IAllocator* pAlloc)
	{
		if ((newSize == m_nElements) && (newSize != 0))
			return LTTRUE;

		if (m_pArray)
			_DeleteAndDestroyArray(pAlloc, GetNumAllocatedElements());

		m_nElements = newSize;
		if (newSize > 0)
			m_Cache.SetCacheSize(m_Cache.GetWantedCache());

		return LTTRUE;
	}

	void	Term(IAllocator* pAlloc = &g_10091df0)	{ SetSize2(0, pAlloc); }

	LTBOOL	Init(uint32 size = 0, uint32 cacheSize = 0)
	{
		Term();
		_InitArray(cacheSize);

		return SetSize2(size, &g_10091df0);
	}

	void	Construct(void** pVTable)
	{
		m_pVTable = pVTable;
		Clear();
		Init();
	}

	void	SetCacheSize(uint32 size)	{ m_Cache.SetWantedCache(size); }
};

struct Queue1007bb50
{
	MoArray1007bb50	m_Array;                            // +0x00
	uint32			m_nUsed;                            // +0x14
};

struct Link1007bb50
{
	Link1007bb50*	m_pPrev;                            // +0x00
	Link1007bb50*	m_pNext;                            // +0x04
	void*			m_pData;                            // +0x08
};

struct Owner1007bb50
{
	void*			m_p00;                              // +0x00
	void*			m_pOwner04;                         // +0x04
};

class C1007b960
{
public:
	C1007b960();
	~C1007b960()
	{
		Term();
	}

	void	Term()
	{
		if (m_pTable)
		{
			free(m_pTable);
			m_pTable = LTNULL;
		}

		m_nCount14 = 0;
		m_n04 = 0;
	}

	uint32*			m_pTable;                           // +0x00
	uint32			m_n04;                              // +0x04
	Link1007bb50*	m_pNext08;                          // +0x08
	Link1007bb50*	m_pPrev0c;                          // +0x0c
	void*			m_p10;                              // +0x10
	int32			m_nCount14;                         // +0x14
	const void*		m_pszName18;                        // +0x18
	uint32			m_n1c;                              // +0x1c
};

C1007b960::C1007b960()
{
	m_pszName18 = g_100a9810;
	m_p10 = this;
	m_n04 = 0;
	m_pTable = LTNULL;
	m_nCount14 = 0;
}

struct LMEntry1007bb50
{
	void*			m_pPoly;                            // +0x00
	void*			m_pMat;                             // +0x04
};

class C1007b980
{
public:
	C1007b980();

	void	Reset()
	{
		Link1007bb50* pCur;

		m_Opaque.m_nUsed = 0;
		m_Translucent.m_nUsed = 0;
		m_nEntries = 0;

		for (pCur = m_Head.m_pNext; pCur != (Link1007bb50*)this; pCur = pCur->m_pNext)
			((Owner1007bb50*)pCur->m_pData)->m_pOwner04 = LTNULL;
	}

	Link1007bb50	m_Head;                             // +0x000
	uint32			m_nFlags0c;                         // +0x00c
	Queue1007bb50	m_Opaque;                           // +0x010
	Queue1007bb50	m_Translucent;                      // +0x028
	LMEntry1007bb50	m_Entries[0x20];                    // +0x040
	uint32			m_nEntries;                         // +0x140
	C1007b960		m_Models;                           // +0x144
	C1007b960		m_ModelsTranslucent;                // +0x164
	C1007b960		m_ModelsChromakey;                  // +0x184
	C1007b960		m_Sprites;                          // +0x1a4
	C1007b960		m_SpritesNoZ;                       // +0x1c4
	C1007b960		m_WorldModels;                      // +0x1e4
	C1007b960		m_WorldModelsTranslucent;           // +0x204
	C1007b960		m_WorldModelsChromakey;             // +0x224
	C1007b960		m_Lights;                           // +0x244
	C1007b960		m_PolyGrids;                        // +0x264
	C1007b960		m_PolyGridsTranslucent;             // +0x284
	C1007b960		m_LineSystems;                      // +0x2a4
	C1007b960		m_ParticleSystems;                  // +0x2c4
	C1007b960		m_Canvases;                         // +0x2e4
	C1007b960		m_CanvasesTranslucent;              // +0x304
};

C1007b980::C1007b980()
{
	m_Head.m_pPrev = m_Head.m_pNext = (Link1007bb50*)this;
	m_nFlags0c = 1;
	m_Opaque.m_Array.SetCacheSize(0x200);
	m_Translucent.m_Array.SetCacheSize(0x20);

	Reset();
}

C1007b980 g_10091a10;                                   // 0x10091a10
