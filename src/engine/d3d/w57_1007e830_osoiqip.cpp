// *********************************************************************** //
//  w57_1007e830_osoiqip -- d3d.ren, /Os /Oi- /Qip:
//    0x1007e0a4  104 bytes  the model vertex-buffer CACHE's constructor
//                           (register entry; 0x1007e0a0 its stack shim)
//    0x1007e830   31 bytes  the static initialiser (.data 0x10089210):
//                           ctor(&g_10091d80) ; atexit(0x1007e850)
//    0x1007e850   84 bytes  that atexit destructor
//
//  One file-scope object, g_10091d80 (w2_10009ae0.cpp's "model cache":
//  slot +0x10 Init, the cache delay at +0x60), of a class derived from the
//  vertex-buffer base 0x1007dc54 (w35): the base constructor called out of
//  line (0x1007e0b0 `call 0x1007dc54`, the register entry -- the call is a
//  relocation either way), the class vptr 0x10089a40, a CMoArray at +0x3c
//  (vptr 0x10089a90; Clear, then Init's Term / _InitArray / SetSize(0) --
//  the three zero groups, written with w35's Construct and d3dr5a.cpp's
//  header chain), +0x50..+0x68 zeroed, +0x60 = 4, +0x28 = 1.
//
//  WHAT THE FLAGS ARE FOR.  /Qip: the initialiser calls the constructor's
//  register entry and the constructor's shim shape is /Qip's.  /Os: the
//  initialiser's `push %edi ; push $dtor` (pushed arguments with a dead
//  slot).  /Oi- changes nothing here (no intrinsics); it is the /Os /Qip
//  convention engine.sh has a pattern for.
//
//  THE DESTRUCTOR IS VIRTUAL.  Slot 0 of 0x10089a40 is 0x1007dc00, a
//  scalar deleting destructor, so the base's destructor is declared
//  `virtual`.  That is also what makes icl's atexit thunk keep `this` in
//  %ebx (`mov $0x10091d80,%ebx`, fields as N(%ebx), the base destructor
//  called with an absolute %ecx) -- with a non-virtual destructor every
//  field is addressed absolutely (92 bytes against 84, measured at /Os,
//  /O2, /O2 /Qip, /Os /Qip /Qip_no_inlining, and with an explicit or
//  `inline` destructor).  d3dr5a.cpp's hand-written sub_1007e850, which
//  is the same bytes through a pointer local, stays (the byte comparison maps it).
//
//  The vptr is stored explicitly (d3ddata.cpp's g_vtbl_10089a40 has the
//  real slots; icl's own ??_7 for this class only the destructor); icl
//  drops its own store as dead.  The constructor's class is named for the
//  stack shim so that the byte comparison measures its `$` register entry at
//  0x1007e0a4.  All three IDENTICAL.  New global: 0x10091d80 (defined
//  here).  icl also emits ??_GC1007e0a0 and an out-of-line ~C1007e0a0 for
//  the vtable it never stores; neither is referenced.
// *********************************************************************** //

#include "ltbasedefs.h"

struct IAllocator
{
	virtual void*	Slot00(uint32 nSize);               // vtable +0x00
	virtual void*	Alloc(uint32 nSize);                // vtable +0x04
	virtual void	Free(void* pMem);                   // vtable +0x08
};

extern IAllocator g_10091df0;                           // 0x10091df0

extern void* g_vtbl_10089a40[];                         // 0x10089a40
extern void* g_vtbl_10089a90[];                         // 0x10089a90

template<class T>
inline void BaseDelete1007e0a4(IAllocator* pAlloc, T* ptr, uint32 nElements)
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

struct DefCache1007e0a4
{
	uint32	m_CacheSize;                                // +0x00
	uint32	m_WantedCache;                              // +0x04

	uint32	GetCacheSize() const	{ return m_CacheSize; }
	void	SetCacheSize(uint32 n)	{ m_CacheSize = n; }
	uint32	GetWantedCache() const	{ return m_WantedCache; }
	void	SetWantedCache(uint32 n){ m_WantedCache = n; }
};

struct MoArray1007e0a4
{
	void**			m_pVTable;                      // +0x00
	uint32*			m_pArray;                       // +0x04
	uint32			m_nElements;                    // +0x08
	DefCache1007e0a4	m_Cache;                        // +0x0c

	~MoArray1007e0a4()
	{
		m_pVTable = g_vtbl_10089a90;
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
			BaseDelete1007e0a4(pAlloc, m_pArray, nElements);
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
};

struct C1007dc54
{
	C1007dc54();                                        // 0x1007dc50
	virtual ~C1007dc54();                               // 0x1007dcc0

	uint8		m_pad_04[0x28 - 0x04];                  // +0x04
	uint32		m_n28;                                  // +0x28
	uint8		m_pad_2c[0x3c - 0x2c];                  // +0x2c
};

struct C1007e0a0 : public C1007dc54
{
	C1007e0a0();

	MoArray1007e0a4	m_Array3c;                          // +0x3c
	uint32		m_n50;                                  // +0x50
	uint32		m_n54;                                  // +0x54
	uint32		m_n58;                                  // +0x58
	uint32		m_n5c;                                  // +0x5c
	uint32		m_nCacheDelay60;                        // +0x60
	uint32		m_n64;                                  // +0x64
	uint32		m_n68;                                  // +0x68
};

C1007e0a0::C1007e0a0()
{
	*(void***)this = g_vtbl_10089a40;
	m_Array3c.Construct(g_vtbl_10089a90);

	m_n50 = 0;
	m_n54 = 0;
	m_n58 = 0;
	m_n5c = 0;
	m_n64 = 0;
	m_n68 = 0;

	m_nCacheDelay60 = 4;
	m_n28 = 1;
}

C1007e0a0 g_10091d80;                                   // 0x10091d80
