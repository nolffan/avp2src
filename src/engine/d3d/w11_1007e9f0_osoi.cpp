// *********************************************************************** //
//
//  MODULE:   W11_1007E9F0_OSOI (.CPP)  --  d3d.ren, CMoArray<T> members the
//            link was missing (0x1007e9f0 .. 0x1007fa50)
//
//  PURPOSE:  Six instantiations of stdlith's CMoArray<T,DefaultCache>
//            (src/LT2/lithshared/stdlith/dynarray.h), written from the shipped
//            bytes.  The bodies are the header's own, statement for
//            statement; what the bytes add is the element type of each
//            instantiation and the helpers it reaches.
//
//  LAYOUT (every body below): +0x00 vptr (CMoArray derives GenList<T>),
//  +0x04 m_pArray, +0x08 m_nElements, +0x0c m_Cache (DefaultCache, two
//  dwords).  The DefaultCache accessors and GetNumAllocatedElements are
//  CALLED in the shipped code (0x1000a670/a680/a690, 0x10021a80,
//  0x10009c90), so here they are declared and defined elsewhere, never
//  in-class (icl's /Ob1 would expand an in-class body).
//
//  TWO INSTANTIATIONS:
//
//    T = 16 bytes, BLOCK-copied: every element assignment is
//        `mov $0x4,%ecx ; rep movsl`, not four named dword moves (compare
//        0x10009f10 in d3dr5d.cpp, whose T copies field by field), so T is
//        a 16-byte aggregate whose implicit copy is the block form.  Its
//        helpers: _AllocateTArray 0x1007eb80 (`shl $0x4`), the
//        T-independent _DeleteAndDestroyArray 0x1007ebe0 and
//        GetNumAllocatedElements 0x10021a80.
//            0x1007e9f0  InternalNiceSetSize  (400)
//            0x1007eea0  SetSize2             (160)
//            0x1007ef70  Remove2              (432)
//            0x1007f150  Insert2              (513)
//
//    T = 4 bytes (a pointer -- the callers name the class ArrP): copies
//        are `mov (%edx,%eax,4)`.  Its helpers: _AllocateTArray 0x10009cc0,
//        _DeleteAndDestroyArray 0x10009cf0, GetNumAllocatedElements
//        0x10009c90, SetSize2 0x1007f810.
//            0x1007f8e0  Remove2              (268)
//            0x1007fa50  Term                 (40)
//
//  FLAGS: /Os /Oi- (the file name selects it).  InternalNiceSetSize's
//  bFast arm CALLS memcpy (0x100805d0, cdecl, `add $0x10,%esp`) where the
//  /O2 copies at 0x10009f10/0x1007c290 expand it, and the saves are
//  pushes, not the /O2 `mov %reg,(%esp)` form.  Measured on this file:
//      /O2        1 of 6 identical     /Os        5 of 6 (not e9f0: memcpy)
//      /O2 /Oi-   1 of 6               /Os /Oi-   6 of 6
//
//  The referencing TUs spell these members under their own class names
//  (Obj1007ee70, C1007eea0, C1007f3c0, Obj1007f120, C1007f360,
//  Arr1007f9f0, ArrP); link/aliases.rsp maps each onto the definition
//  here, and maps this file's helper spellings onto their definitions.
//
// *********************************************************************** //

#include "ltbasedefs.h"
#include <string.h>


// stdlith's LAlloc as the d3d TUs model it: Alloc at +0x04, Free at +0x08.
// Only passed through here.
class IAllocator
{
public:
	virtual void* Slot00(uint32 nSize) = 0;             // +0x00
	virtual void* Alloc(uint32 nSize) = 0;              // +0x04
	virtual void  Free(void* pMem) = 0;                 // +0x08
};

struct DefaultCache
{
	uint32 m_CacheSize;                                 // +0x00
	uint32 m_WantedCache;                               // +0x04

	uint32 GetCacheSize() const;                        // 0x1000a690
	void   SetCacheSize(uint32 val);                    // 0x1000a680
	uint32 GetWantedCache() const;                      // 0x1000a670
};


// ======================================================================= //
//  T = 16 bytes
// ======================================================================= //

struct Elem1007e9f0
{
	uint32 m_n[4];                                      // +0x00, block-copied
};

struct Arr1007e9f0
{
	uint32*       m_pVTable;                            // +0x00
	Elem1007e9f0* m_pArray;                             // +0x04
	uint32        m_nElements;                          // +0x08
	DefaultCache  m_Cache;                              // +0x0c

	Elem1007e9f0* _AllocateTArray(uint32 nElements, IAllocator* pAlloc); // 0x1007eb80
	void    _DeleteAndDestroyArray(IAllocator* pAlloc, uint32 n);        // 0x1007ebe0
	uint32  GetNumAllocatedElements();                                   // 0x10021a80

	int32   sub_1007e9f0(uint32 newSize, int32 bFast, IAllocator* pAlloc);   // InternalNiceSetSize
	int32   sub_1007eea0(uint32 newSize, IAllocator* pAlloc);                // SetSize2
	void    sub_1007ef70(uint32 index, IAllocator* pAlloc);                  // Remove2
	int32   sub_1007f150(uint32 index, const Elem1007e9f0& toInsert,
	                     IAllocator* pAlloc);                                // Insert2
};


// ======================================================================= //
//  0x1007e9f0  400 bytes -- CMoArray<T16>::InternalNiceSetSize
//
//  The header's body: three compares of newSize against m_nElements off
//  one register pair (jb shrink / ja grow / jne alloc), the grow arm's
//  cache test jumping back to the third compare, nToCopy clamped by a
//  `jbe` over one `mov`, `if(bFast)` with the memcpy arm sunk, then
//  _DeleteAndDestroyArray(pAlloc, GetNumAllocatedElements()).
// ======================================================================= //

int32 Arr1007e9f0::sub_1007e9f0(uint32 newSize, int32 bFast, IAllocator* pAlloc)
{
	Elem1007e9f0* pNewArray;
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
		memcpy(pNewArray, m_pArray, sizeof(Elem1007e9f0) * nToCopy);
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
//  0x1007eea0  160 bytes -- CMoArray<T16>::SetSize2
//
//  `cmp 0x8(%ebx),%esi ; jne ; test %esi,%esi ; je` is the header's
//  `(newSize == m_nElements) && (newSize != 0)` early TRUE; the release is
//  guarded by m_pArray; the failed allocation zeroes m_nElements and the
//  cache size and returns FALSE.
// ======================================================================= //

int32 Arr1007e9f0::sub_1007eea0(uint32 newSize, IAllocator* pAlloc)
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
//  0x1007ef70  432 bytes -- CMoArray<T16>::Remove2
//
//  `add %eax,%eax ; cmp %eax,%esi ; jae` is GetCacheSize() >=
//  GetWantedCache()*2 with the shrink-and-copy arm sunk; a failed
//  allocation with a non-zero newAllocSize (`test %edi,%edi`) falls back
//  to the slide, which is bSlideDown folded into control flow.
// ======================================================================= //

void Arr1007e9f0::sub_1007ef70(uint32 index, IAllocator* pAlloc)
{
	uint32        i, newSize, newAllocSize;
	Elem1007e9f0* pNewArray;
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
//  0x1007f150  513 bytes -- CMoArray<T16>::Insert2
//
//  `index > m_nElements` returns FALSE; `GetCacheSize() == 0` sends the
//  grow arm out of line (`je`) and the slide arm falls through; the
//  grow arm's `lea 0x1(%eax,%edx,1)` is newSize + GetWantedCache() with
//  newSize = m_nElements + 1 folded into the displacement; both arms meet
//  at the single `incl 0x8(%ebx)` / `return TRUE`.
// ======================================================================= //

int32 Arr1007e9f0::sub_1007f150(uint32 index, const Elem1007e9f0& toInsert,
                                IAllocator* pAlloc)
{
	Elem1007e9f0* pNewArray;
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


// ======================================================================= //
//  T = 4 bytes (pointer)
// ======================================================================= //

struct Arr1007f8e0
{
	uint32*      m_pVTable;                             // +0x00
	void**       m_pArray;                              // +0x04
	uint32       m_nElements;                           // +0x08
	DefaultCache m_Cache;                               // +0x0c

	void**  _AllocateTArray(uint32 nElements, IAllocator* pAlloc);   // 0x10009cc0
	void    _DeleteAndDestroyArray(IAllocator* pAlloc, uint32 n);    // 0x10009cf0
	uint32  GetNumAllocatedElements();                               // 0x10009c90
	int32   SetSize2(uint32 newSize, IAllocator* pAlloc);            // 0x1007f810

	void    sub_1007f8e0(uint32 index, IAllocator* pAlloc);          // Remove2
	void    sub_1007fa50(IAllocator* pAlloc);                        // Term
};


// ======================================================================= //
//  0x1007f8e0  268 bytes -- CMoArray<void*>::Remove2
//
//  The same source as 0x1007ef70 with a 4-byte T: copies are
//  `mov (%edx,%eax,4)` / `mov 0x4(%edx,%eax,4)`.
// ======================================================================= //

void Arr1007f8e0::sub_1007f8e0(uint32 index, IAllocator* pAlloc)
{
	uint32 i, newSize, newAllocSize;
	void** pNewArray;
	int32  bSlideDown;

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
//  0x1007fa50  40 bytes -- CMoArray<void*>::Term(LAlloc*)
//
//  `movl $0x0,(%esp) ; mov %eax,0x4(%esp) ; call 0x1007f810` with
//  `ret $0x4`: the header's { SetSize2(0, pAlloc); }, result discarded.
// ======================================================================= //

void Arr1007f8e0::sub_1007fa50(IAllocator* pAlloc)
{
	SetSize2(0, pAlloc);
}
