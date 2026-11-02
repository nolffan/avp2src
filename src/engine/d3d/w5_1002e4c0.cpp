// *********************************************************************** //
//
//  MODULE:   W5_1002E4C0 (.CPP)  --  CMoArray<T,DefaultCache> over a
//            TWELVE-byte T: InternalNiceSetSize, SetSize2, Remove2, Insert2.
//
//  All four are src/LT2/lithshared/stdlith/dynarray.h verbatim, instantiated for
//  a 12-byte element.  The element size is read off every copy loop: three
//  dword moves per element at +0/+4/+8 and a byte index built as
//  `lea (,%n,8)` + `lea (%x,%n,4)` == n*12, and off the helpers the four
//  share -- _AllocateTArray 0x1002e690 (`lea (,%eax,8)` + `lea
//  (%edx,%eax,4)`, i.e. n*12 bytes), _DeleteAndDestroyArray 0x1002e6c0,
//  GetNumAllocatedElements 0x10009c90 and DefaultCache's out-of-line
//  accessors 0x1000a670/80/90 at `this+0xc`.
//
//  /O2: every one pads to 16 with multi-byte lea-nops and duplicates its
//  epilogue at each return, and the one-argument wrappers next to them
//  (0x1002e7f0, 0x1002e9d0, 0x1002ec00, 0x1002ec30) use the /O2
//  `add $-N,%esp` outgoing area.
//
// *********************************************************************** //

#include "ltbasedefs.h"


// stdlith's LAlloc; slot +0x04 Alloc, +0x08 Free.  Declared as the callers'
// mangled names spell it (`struct IAllocator`), never defined.
struct IAllocator
{
	virtual void* Slot00(uint32 nSize) = 0;             // +0x00
	virtual void* Alloc(uint32 nSize) = 0;              // +0x04
	virtual void  Free(void* pMem) = 0;                 // +0x08
};

// dynarray.h's DefaultCache; its accessors are out of line in the image.
struct DefaultCache
{
	uint32 m_CacheSize;                                 // +0x00
	uint32 m_WantedCache;                               // +0x04

	uint32 GetCacheSize() const;                        // 0x1000a690
	void   SetCacheSize(uint32 val);                    // 0x1000a680
	uint32 GetWantedCache() const;                      // 0x1000a670
};

// The element: twelve bytes copied field by field (three dword movs), not
// by `rep movsl`, so three named dwords rather than an array member.
struct Elem1002e4c0
{
	uint32 m_n00;                                       // +0x00
	uint32 m_n04;                                       // +0x04
	uint32 m_n08;                                       // +0x08
};

// The same object the callers name four ways (their class names are their
// own; aliases.rsp maps the spellings onto these definitions).
struct Arr1002e4c0
{
	uint32*       m_pVTable;                            // +0x00
	Elem1002e4c0* m_pArray;                             // +0x04
	uint32        m_nElements;                          // +0x08
	DefaultCache  m_Cache;                              // +0x0c

	Elem1002e4c0* _AllocateTArray(uint32 nElements, IAllocator* pAlloc); // 0x1002e690
	void    _DeleteAndDestroyArray(IAllocator* pAlloc, uint32 n);        // 0x1002e6c0
	uint32  GetNumAllocatedElements();                                   // 0x10009c90

	uint32  sub_1002e4c0(uint32 newSize, uint32 bFast, IAllocator* pAlloc);  // InternalNiceSetSize
	uint32  sub_1002e710(uint32 newSize, IAllocator* pAlloc);                // SetSize2
	void    sub_1002e820(uint32 index, IAllocator* pAlloc);                  // Remove2
	uint32  sub_1002ea00(uint32 index, const void* pToInsert, IAllocator* pAlloc); // Insert2
};


// ======================================================================= //
//  0x1002e4c0 -- InternalNiceSetSize(DWORD newSize, BOOL bFast, LAlloc*).
//  The three `cmp %eax,%edx` off one register pair are the header's
//  three-way else-if; the bFast arm is the inline memcpy expansion
//  (`push %ecx; shr $2; rep movsl; pop %ecx; and $3; rep movsb`).
//  Same source as d3dr5d.cpp's 0x1007c290 / 0x10009f10.
// ======================================================================= //
uint32 Arr1002e4c0::sub_1002e4c0(uint32 newSize, uint32 bFast, IAllocator* pAlloc)
{
	Elem1002e4c0* pNewArray;
	uint32        i, nToCopy;

	if (newSize < m_nElements)
	{
		m_Cache.SetCacheSize(m_Cache.GetCacheSize() + (m_nElements - newSize));
		m_nElements = newSize;
		return 1;
	}
	else if (newSize > m_nElements && (m_nElements + m_Cache.GetCacheSize()) >= newSize)
	{
		m_Cache.SetCacheSize(m_Cache.GetCacheSize() - (newSize - m_nElements));
		m_nElements = newSize;
		return 1;
	}
	else if (newSize == m_nElements)
	{
		return 1;
	}

	pNewArray = _AllocateTArray(newSize + m_Cache.GetWantedCache(), pAlloc);
	if (!pNewArray)
		return 0;

	nToCopy = m_nElements;
	if (nToCopy > newSize)
		nToCopy = newSize;

	if (bFast)
	{
		memcpy(pNewArray, m_pArray, sizeof(Elem1002e4c0) * nToCopy);
	}
	else
	{
		for (i = 0; i < nToCopy; i++)
		{
			pNewArray[i] = m_pArray[i];
		}
	}

	_DeleteAndDestroyArray(pAlloc, GetNumAllocatedElements());
	m_pArray = pNewArray;
	m_nElements = newSize;
	m_Cache.SetCacheSize(m_Cache.GetWantedCache());

	return 1;
}


// ======================================================================= //
//  0x1002e710 -- SetSize2(DWORD newSize, LAlloc*).  `cmp 0x8(%ebx),%esi;
//  jne; test %esi,%esi; je` is `(newSize == m_nElements) && (newSize != 0)`;
//  the delete is SUNK past the body (0x1002e7cb) and jumps back to the
//  `m_nElements = newSize` store.  Same source as 0x1007f810 (T = 4).
// ======================================================================= //
uint32 Arr1002e4c0::sub_1002e710(uint32 newSize, IAllocator* pAlloc)
{
	if ((newSize == m_nElements) && (newSize != 0))
	{
		return 1;
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
			return 0;
		}

		m_Cache.SetCacheSize(m_Cache.GetWantedCache());
	}

	return 1;
}


// ======================================================================= //
//  0x1002e820 -- Remove2(DWORD index, LAlloc*).  `GetCacheSize() >=
//  GetWantedCache()*2` is `add %eax,%eax; cmp %eax,%esi; jae`; the
//  `pNewArray || newAllocSize == 0` test is the sunk block at 0x1002e9b1
//  (the allocation failing with a non-zero size falls into the slide).
// ======================================================================= //
void Arr1002e4c0::sub_1002e820(uint32 index, IAllocator* pAlloc)
{
	uint32        i, newSize, newAllocSize;
	Elem1002e4c0* pNewArray;
	int32         bSlideDown;

	bSlideDown = 1;
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
			bSlideDown = 0;
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


// ======================================================================= //
//  0x1002ea00 -- Insert2(DWORD index, const T&, LAlloc*).  Same source as
//  d3dr5d.cpp's 0x10009d60 (T = 4): `cmp %edx,%edi; jbe` is
//  `index > m_nElements`, the grow arm is sunk behind `je 0x1002eada`,
//  and the single `incl 0x8(%ebx)` is the `++m_nElements` both arms reach.
//  The callers pass the element as `const void*`; it is a `const T&`.
// ======================================================================= //
uint32 Arr1002e4c0::sub_1002ea00(uint32 index, const void* pToInsert, IAllocator* pAlloc)
{
	const Elem1002e4c0& toInsert = *(const Elem1002e4c0*)pToInsert;
	Elem1002e4c0* pNewArray;
	uint32        newSize, i;

	if (index > m_nElements)
		return 0;

	newSize = m_nElements + 1;

	if (m_Cache.GetCacheSize() == 0)
	{
		pNewArray = _AllocateTArray(newSize + m_Cache.GetWantedCache(), pAlloc);
		if (!pNewArray)
			return 0;

		for (i = 0; i < index; i++)
			pNewArray[i] = m_pArray[i];

		for (i = index; i < m_nElements; i++)
			pNewArray[i + 1] = m_pArray[i];

		pNewArray[index] = toInsert;

		if (m_pArray)
		{
			_DeleteAndDestroyArray(pAlloc, GetNumAllocatedElements());
		}

		m_Cache.SetCacheSize(m_Cache.GetWantedCache());
		m_pArray = pNewArray;
	}
	else
	{
		for (i = m_nElements; i > index; i--)
			m_pArray[i] = m_pArray[i - 1];

		m_Cache.SetCacheSize(m_Cache.GetCacheSize() - 1);

		m_pArray[index] = toInsert;
	}

	++m_nElements;

	return 1;
}
