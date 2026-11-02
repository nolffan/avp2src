// *********************************************************************** //
//
//  w45_1001d810_osoi.cpp -- d3d.ren: the two /Os copies of stdlith's
//  CMoArray<T,DefaultCache>::InternalNiceSetSize, 0x1001d810 (T = WORD)
//  and 0x1007f420 (T = 4 bytes).  Moved here unchanged from d3dr5d_os.cpp;
//  the notes below are that file's.
//
//  FLAGS: /Os /Oi- (the `_osoi` convention).  Both shipped copies CALL
//  memcpy (0x100805d0) on the bFast arm; at d3dr5d_os.cpp's /Os icl
//  expands it to `rep movsl / rep movsb`, and the expansion also cost the
//  register newSize lives in (shipped %edi, ours the frame).  Measured:
//  d3dr5d_os.cpp at /Os 283 / 284 bytes differ, the same source at
//  /Os /Oi- both byte-identical (320 bytes each), and every other
//  function of that file identical under either flag set.
//
// *********************************************************************** //

#include "ltbasedefs.h"


// stdlith's LAlloc, as d3dr5d_os.cpp declares it (never defined here).
class IAllocator
{
public:
	virtual void* Slot00(uint32 nSize) = 0;             // +0x00
	virtual void* Alloc(uint32 nSize) = 0;              // +0x04
	virtual void  Free(void* pMem) = 0;                 // +0x08
};

// dynarray.h's DefaultCache, accessors out of line (0x1000a670/80/90).
struct DefaultCache
{
	uint32 m_CacheSize;                                 // +0x00
	uint32 m_WantedCache;                               // +0x04

	uint32 GetCacheSize() const;                        // 0x1000a690
	void   SetCacheSize(uint32 val);                    // 0x1000a680
	uint32 GetWantedCache() const;                      // 0x1000a670
};


// ---- 0x1001d810 (316 bytes, /Os) -- the SAME source with T = WORD, and
//      it is what makes rows 0x1001d834 and 0x1007f444 go away:
//      0x1001d834 is this function's shared `return TRUE` block.
//
//      T = WORD is read off the element loop --
//          mov 0x4(%ebx),%edx ; movzwl (%edx,%eax,2),%edx
//          mov %dx,(%esi,%eax,2)
//      -- a genuine 16-bit load and 16-bit store, and off the byte count
//      the fast arm builds, `add %ecx,%ecx` (ONE doubling).  Its
//      _AllocateTArray is 0x1001d950; its _DeleteAndDestroyArray
//      (0x1001cf00) and GetNumAllocatedElements (0x10021a80) are the
//      T-INDEPENDENT ones the T=4 instantiation also uses, which is
//      COMDAT folding, not a different function.
//
//      /Os: every one of the four `return TRUE` sites is a jump into the
//      block at 0x1001d834 (`e9 4a ff ff ff`, `e9 0e ff ff ff`,
//      `e9 eb fe ff ff` and the `jne 0x1001d840` fall-in), and the FALSE
//      exit at 0x1001d949 cross-jumps into its tail.
struct Arr1001d810
{
	uint32*      m_pVTable;                             // +0x00
	uint16*      m_pArray;                              // +0x04
	uint32       m_nElements;                           // +0x08
	DefaultCache m_Cache;                               // +0x0c

	uint16* _AllocateTArray(uint32 nElements, IAllocator* pAlloc);   // 0x1001d950
	void    _DeleteAndDestroyArray(IAllocator* pAlloc, uint32 n);    // 0x1001cf00
	uint32  GetNumAllocatedElements();                               // 0x10021a80

	int32   InternalNiceSetSize(uint32 newSize, int32 bFast,
	                            IAllocator* pAlloc);                 // 0x1001d810
};

int32 Arr1001d810::InternalNiceSetSize(uint32 newSize, int32 bFast,
                                       IAllocator* pAlloc)
{
	uint16* pNewArray;
	uint32  i, nToCopy;

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
		memcpy(pNewArray, m_pArray, sizeof(uint16) * nToCopy);
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


// ---- 0x1007f420 (316 bytes, /Os) -- T = 4 bytes again
//      (`mov (%ecx,%eax,4),%ecx`, `add %edx,%edx` TWICE), helpers
//      0x10009cc0 / 0x10009cf0 / 0x10009c90, i.e. the same instantiation
//      as 0x10009d60's Insert2 but in an /Os object.  0x1007f444 is its
//      shared `return TRUE` block.
//
//      WORTH RECORDING: this copy CALLS memcpy (0x100805d0) where
//      0x1007c290 -- the /O2 object, same T, same source line -- expands
//      it inline.  So the inline-vs-call decision on memcpy is per
//      OBJECT, not per element type, and a call turning up where you
//      expected an expansion is a flag symptom, not a source difference.
struct Arr1007f420
{
	uint32*      m_pVTable;                             // +0x00
	uint32*      m_pArray;                              // +0x04
	uint32       m_nElements;                           // +0x08
	DefaultCache m_Cache;                               // +0x0c

	uint32* _AllocateTArray(uint32 nElements, IAllocator* pAlloc);   // 0x10009cc0
	void    _DeleteAndDestroyArray(IAllocator* pAlloc, uint32 n);    // 0x10009cf0
	uint32  GetNumAllocatedElements();                               // 0x10009c90

	int32   InternalNiceSetSize(uint32 newSize, int32 bFast,
	                            IAllocator* pAlloc);                 // 0x1007f420
};

int32 Arr1007f420::InternalNiceSetSize(uint32 newSize, int32 bFast,
                                       IAllocator* pAlloc)
{
	uint32* pNewArray;
	uint32  i, nToCopy;

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
		memcpy(pNewArray, m_pArray, sizeof(uint32) * nToCopy);
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

