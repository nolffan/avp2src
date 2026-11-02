// *********************************************************************** //
//
//  w2_1000a110.cpp -- stdlith's CMoArray<T,DefaultCache> with a sixteen-
//  byte T: SetSize2 (0x1000a110), Remove2 (0x1000a220), Insert2
//  (0x1000a3f0), and DefaultCache::SetWantedCache (0x1000a660).  Written
//  for the d3d.ren link.
//
//  The same instantiation as d3dr5d.cpp's 0x10009f10 (InternalNiceSetSize):
//  its _AllocateTArray is 0x1000a0e0 (`shl $0x4`), its
//  _DeleteAndDestroyArray 0x1002e6c0 and GetNumAllocatedElements
//  0x10009c90, and every element copy is four dword moves.  The bodies are
//  src/LT2/lithshared/stdlith/dynarray.h transcribed; the class, element and
//  helper spellings are d3dr5d.cpp's, so the callees resolve through the
//  aliases that file already has.  The functions keep sub_<addr> names so
//  the byte comparison places them (the class name carries 0x10009f10).
//
//  The DefaultCache accessors are OUT OF LINE in the shipped build (/Ob1
//  and not `inline`): 0x1000a670 GetWantedCache, 0x1000a680 SetCacheSize,
//  0x1000a690 GetCacheSize, and 0x1000a660 SetWantedCache, which d3dr4b.cpp
//  calls as DefaultCache::sub_1000a660 and is defined here.
//
//  The forwarders that supply g_DefAlloc (0x10091df0) or a zero are
//  already written: 0x1000a1f0 / 0x1000a630 -> SetSize2 (Term() is
//  SetSize2(0, pAlloc)), 0x1000a3c0 -> Remove2, 0x1000a600 -> Insert2;
//  their spellings of these three are aliased in link/aliases.rsp.
//
//  /O2: duplicated epilogues, `mov $0x1,%eax` for TRUE, lea-nop padding.
//
// *********************************************************************** //

#include "ltbasedefs.h"


// stdlith's LAlloc, as d3dr5d.cpp declares it (never defined here).
class IAllocator
{
public:
	virtual void* Slot00(uint32 nSize) = 0;             // +0x00
	virtual void* Alloc(uint32 nSize) = 0;              // +0x04
	virtual void  Free(void* pMem) = 0;                 // +0x08
};


// dynarray.h's DefaultCache.
struct DefaultCache
{
	uint32 m_CacheSize;                                 // +0x00
	uint32 m_WantedCache;                               // +0x04

	uint32 GetCacheSize() const;                        // 0x1000a690
	void   SetCacheSize(uint32 val);                    // 0x1000a680
	uint32 GetWantedCache() const;                      // 0x1000a670
	void   sub_1000a660(uint32 val);                    // 0x1000a660  SetWantedCache
};


// d3dr5d.cpp's sixteen-byte element and array.
struct Elem10009f10
{
	uint32 m_n00;                                       // +0x00
	uint32 m_n04;                                       // +0x04
	uint32 m_n08;                                       // +0x08
	uint32 m_n0c;                                       // +0x0c
};

struct Arr10009f10
{
	uint32*      m_pVTable;                             // +0x00
	Elem10009f10* m_pArray;                             // +0x04
	uint32       m_nElements;                           // +0x08
	DefaultCache m_Cache;                               // +0x0c

	Elem10009f10* _AllocateTArray(uint32 nElements, IAllocator* pAlloc); // 0x1000a0e0
	void    _DeleteAndDestroyArray(IAllocator* pAlloc, uint32 n);        // 0x1002e6c0
	uint32  GetNumAllocatedElements();                                   // 0x10009c90

	int32   sub_1000a110(uint32 newSize, IAllocator* pAlloc);          // SetSize2
	void    sub_1000a220(uint32 index, IAllocator* pAlloc);            // Remove2
	int32   sub_1000a3f0(uint32 index, const Elem10009f10& toInsert,
	                     IAllocator* pAlloc);                          // Insert2
};


// ======================================================================= //
//  0x1000a110 -- CMoArray::SetSize2.
// ======================================================================= //
int32 Arr10009f10::sub_1000a110(uint32 newSize, IAllocator* pAlloc)
{
	// Uhhh ok...
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
//  0x1000a220 -- CMoArray::Remove2.
// ======================================================================= //
void Arr10009f10::sub_1000a220(uint32 index, IAllocator* pAlloc)
{
	uint32 i, newSize, newAllocSize;
	Elem10009f10* pNewArray;
	int32 bSlideDown;

	bSlideDown = 1;
	if (m_Cache.GetCacheSize() >= (m_Cache.GetWantedCache() * 2))
	{
		newSize = m_nElements - 1;
		newAllocSize = newSize + m_Cache.GetWantedCache();
		pNewArray = _AllocateTArray(newAllocSize, pAlloc);

		// Make sure it allocated the array .. if it didn't, just have
		// it slide all the elements down (this guarantees that Remove()
		// won't fail.)
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
		// Slide them all down one.
		m_Cache.SetCacheSize(m_Cache.GetCacheSize() + 1);

		for (i = index; i < m_nElements - 1; i++)
			m_pArray[i] = m_pArray[i + 1];
	}

	--m_nElements;
}


// ======================================================================= //
//  0x1000a3f0 -- CMoArray::Insert2.
// ======================================================================= //
int32 Arr10009f10::sub_1000a3f0(uint32 index, const Elem10009f10& toInsert,
                                IAllocator* pAlloc)
{
	Elem10009f10* pNewArray;
	uint32 newSize, i;

	if (index > m_nElements)
		return 0;

	// Create a new array (possibly).
	newSize = m_nElements + 1;

	if (m_Cache.GetCacheSize() == 0)
	{
		pNewArray = _AllocateTArray(newSize + m_Cache.GetWantedCache(), pAlloc);
		if (!pNewArray)
			return 0;

		// Copy the old array into the new one, start inserting at index.
		for (i = 0; i < index; i++)
			pNewArray[i] = m_pArray[i];

		for (i = index; i < m_nElements; i++)
			pNewArray[i + 1] = m_pArray[i];

		// Insert the new item into the array
		pNewArray[index] = toInsert;

		// Free the old array and set our pointer to the new one
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


// ======================================================================= //
//  0x1000a660 -- DefaultCache::SetWantedCache.
// ======================================================================= //
void DefaultCache::sub_1000a660(uint32 val)
{
	m_WantedCache = val;
}
