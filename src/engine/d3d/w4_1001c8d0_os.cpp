// *********************************************************************** //
//
//  MODULE:   W4_1001C8D0_OS (.CPP)  -- /Os
//
//  PURPOSE:  Functions the d3d.ren link referenced by address and nobody
//            had written, 0x1001c8d0..0x1001dee0: the out-of-line workers
//            of src/LT2/lithshared/stdlith/dynarray.h's CMoArray<T,DefaultCache>
//            (Remove2 / Insert2 / SetSize2) behind the one-line
//            wrappers d3dr2a/b/c/d.cpp already reconstructed.
//
//  Flag set: /Os -- every one of these is followed by a 4-byte-aligning
//  `mov %esi,%esi` / `lea 0x0(%esi),%esi` and then 0x90 bytes up to the
//  next 16-byte boundary, the /Os padding shape.
//
// *********************************************************************** //

#include "ltbasedefs.h"

// stdlith's LAlloc, as an abstract interface (never defined here, so no
// vtable is emitted).  `struct` because that is how the callers' mangled
// references spell it.
struct IAllocator
{
	virtual void* Slot00(uint32 nSize) = 0;             // +0x00
	virtual void* Alloc(uint32 nSize) = 0;              // +0x04
	virtual void  Free(void* pMem) = 0;                 // +0x08
};

// dynarray.h's DefaultCache; its accessors are out of line in the shipped
// build (0x1000a660..0x1000a690, reconstructed in d3dbatchc.cpp).
struct DefaultCache
{
	uint32 m_CacheSize;                                 // +0x00
	uint32 m_WantedCache;                               // +0x04

	uint32 GetCacheSize() const;                        // 0x1000a690
	void   SetCacheSize(uint32 val);                    // 0x1000a680
	uint32 GetWantedCache() const;                      // 0x1000a670
};


// ======================================================================= //
//  CMoArray<T64,DefaultCache>, T a 64-byte POD (element copies are
//  `rep movsl` of 0x10 dwords).  Helpers: _AllocateTArray 0x1001c450,
//  _DeleteAndDestroyArray 0x1001cf00, GetNumAllocatedElements 0x10021a80.
// ======================================================================= //
struct T1001c8d0 { uint32 m_d[16]; };

#define MOARRAY_T64_BODY(NAME)                                              \
	uint32*      m_pVTable;                             /* +0x00 */         \
	T1001c8d0*   m_pArray;                              /* +0x04 */         \
	uint32       m_nElements;                           /* +0x08 */         \
	DefaultCache m_Cache;                               /* +0x0c */         \
	T1001c8d0* _AllocateTArray(uint32 nElements, IAllocator* pAlloc);       \
	void       _DeleteAndDestroyArray(IAllocator* pAlloc, uint32 n);        \
	uint32     GetNumAllocatedElements();

// ---- 0x1001c8d0 -- CMoArray<T64>::Remove2, verbatim from the header.
//      Referenced by Remove (0x1001ca80, d3dr2b.cpp).
//
//      NEAR (and 0x1001cab0 below with it): the SAME source for T32
//      (0x1001d390 / 0x1001d570) is byte-identical, but for this 64-byte T
//      the shipped code folds `add 0x4(%ebx),%edi` where the element index
//      is plain [i] (ours loads m_pArray into a register first) -- two
//      instructions per copy loop, nothing else.  Measured without effect:
//      T as uint32[16] / float[16] / double[8] / uchar[64] / void*[16] /
//      nested structs / with a constructor; a real template instantiation;
//      a real vptr; /O1, /Os /Oi-, /Os /Qip.  (/O2 is far worse.)
//      Also without effect: T as LTMatrix; the REAL
//      dynarray.h template explicitly instantiated for a 64-byte T (no
//      fold either); the T64 pair alone in a file or after the WORD ones;
//      `int` / `unsigned long` indices; pointer-arithmetic spellings;
//      /Os with /G6 /G7 /Qrcd /Op /Ob0 /Ob2 /Oy- /GX /Oa /Ow /Gy /Za /Qansi
//      /Qsfalign16 /Qip_no_inlining (/G5 /GB break the T32 ones).  An
//      inline memcpy operator= drops the strength reduction altogether.
struct Arr1001ca80
{
	MOARRAY_T64_BODY(Arr1001ca80)
	void sub_1001c8d0(uint32 index, IAllocator* pAlloc);
};

void Arr1001ca80::sub_1001c8d0(uint32 index, IAllocator* pAlloc)
{
	uint32 i, newSize, newAllocSize;
	T1001c8d0* pNewArray;
	int32 bSlideDown;

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


// ---- 0x1001cab0 -- CMoArray<T64>::Insert2, verbatim from the header.
//      Referenced by Insert (0x1001ccb0, d3dr2d.cpp) under a void/dword
//      spelling; aliased onto this one in link/aliases.rsp.  Returns BOOL
//      (`xor %eax,%eax` / `xor %eax,%eax; inc %eax`).
struct Arr1001cab0
{
	MOARRAY_T64_BODY(Arr1001cab0)
	int32 sub_1001cab0(uint32 index, const T1001c8d0& toInsert, IAllocator* pAlloc);
};

int32 Arr1001cab0::sub_1001cab0(uint32 index, const T1001c8d0& toInsert, IAllocator* pAlloc)
{
	T1001c8d0* pNewArray;
	uint32 newSize, i;

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
//  CMoArray<T32,DefaultCache>, T a 32-byte POD (`rep movsl` of 8 dwords);
//  _AllocateTArray 0x1001ced0.
// ======================================================================= //
struct T1001cd40 { uint32 m_d[8]; };

// ---- 0x1001d2a0 -- CMoArray<T32>::SetSize2, verbatim from the header.
struct Arr1001d2a0
{
	uint32*      m_pVTable;                             // +0x00
	T1001cd40* m_pArray;                              // +0x04
	uint32       m_nElements;                           // +0x08
	DefaultCache m_Cache;                               // +0x0c

	T1001cd40* _AllocateTArray(uint32 nElements, IAllocator* pAlloc);   // 0x1001ced0
	void _DeleteAndDestroyArray(IAllocator* pAlloc, uint32 n);   // 0x1001cf00
	uint32 GetNumAllocatedElements();                            // 0x10021a80

	int32 sub_1001d2a0(uint32 newSize, IAllocator* pAlloc);
};

int32 Arr1001d2a0::sub_1001d2a0(uint32 newSize, IAllocator* pAlloc)
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


// ---- 0x1001d390 -- CMoArray<T32>::Remove2, verbatim from the header.
struct Arr1001d390
{
	uint32*      m_pVTable;                             // +0x00
	T1001cd40* m_pArray;                              // +0x04
	uint32       m_nElements;                           // +0x08
	DefaultCache m_Cache;                               // +0x0c

	T1001cd40* _AllocateTArray(uint32 nElements, IAllocator* pAlloc);   // 0x1001ced0
	void _DeleteAndDestroyArray(IAllocator* pAlloc, uint32 n);   // 0x1001cf00
	uint32 GetNumAllocatedElements();                            // 0x10021a80

	void sub_1001d390(uint32 index, IAllocator* pAlloc);
};

void Arr1001d390::sub_1001d390(uint32 index, IAllocator* pAlloc)
{
	uint32 i, newSize, newAllocSize;
	T1001cd40* pNewArray;
	int32 bSlideDown;

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


// ---- 0x1001d570 -- CMoArray<T32>::Insert2, verbatim from the header.
struct Arr1001d570
{
	uint32*      m_pVTable;                             // +0x00
	T1001cd40* m_pArray;                              // +0x04
	uint32       m_nElements;                           // +0x08
	DefaultCache m_Cache;                               // +0x0c

	T1001cd40* _AllocateTArray(uint32 nElements, IAllocator* pAlloc);   // 0x1001ced0
	void _DeleteAndDestroyArray(IAllocator* pAlloc, uint32 n);   // 0x1001cf00
	uint32 GetNumAllocatedElements();                            // 0x10021a80

	int32 sub_1001d570(uint32 index, const T1001cd40& toInsert, IAllocator* pAlloc);
};

int32 Arr1001d570::sub_1001d570(uint32 index, const T1001cd40& toInsert, IAllocator* pAlloc)
{
	T1001cd40* pNewArray;
	uint32 newSize, i;

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
//  CMoArray<WORD,DefaultCache>; _AllocateTArray 0x1001d950 (`add %eax,%eax`
//  for the byte count).
// ======================================================================= //
// ---- 0x1001dca0 -- CMoArray<WORD>::SetSize2, verbatim from the header.
struct Arr1001dca0
{
	uint32*      m_pVTable;                             // +0x00
	uint16* m_pArray;                              // +0x04
	uint32       m_nElements;                           // +0x08
	DefaultCache m_Cache;                               // +0x0c

	uint16* _AllocateTArray(uint32 nElements, IAllocator* pAlloc);   // 0x1001d950
	void _DeleteAndDestroyArray(IAllocator* pAlloc, uint32 n);   // 0x1001cf00
	uint32 GetNumAllocatedElements();                            // 0x10021a80

	int32 sub_1001dca0(uint32 newSize, IAllocator* pAlloc);
};

int32 Arr1001dca0::sub_1001dca0(uint32 newSize, IAllocator* pAlloc)
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


// ---- 0x1001dd90 -- CMoArray<WORD>::Remove2, verbatim from the header.
struct Arr1001dd90
{
	uint32*      m_pVTable;                             // +0x00
	uint16* m_pArray;                              // +0x04
	uint32       m_nElements;                           // +0x08
	DefaultCache m_Cache;                               // +0x0c

	uint16* _AllocateTArray(uint32 nElements, IAllocator* pAlloc);   // 0x1001d950
	void _DeleteAndDestroyArray(IAllocator* pAlloc, uint32 n);   // 0x1001cf00
	uint32 GetNumAllocatedElements();                            // 0x10021a80

	void sub_1001dd90(uint32 index, IAllocator* pAlloc);
};

void Arr1001dd90::sub_1001dd90(uint32 index, IAllocator* pAlloc)
{
	uint32 i, newSize, newAllocSize;
	uint16* pNewArray;
	int32 bSlideDown;

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


// ---- 0x1001dee0 -- CMoArray<WORD>::Insert2, verbatim from the header.
struct Arr1001dee0
{
	uint32*      m_pVTable;                             // +0x00
	uint16* m_pArray;                              // +0x04
	uint32       m_nElements;                           // +0x08
	DefaultCache m_Cache;                               // +0x0c

	uint16* _AllocateTArray(uint32 nElements, IAllocator* pAlloc);   // 0x1001d950
	void _DeleteAndDestroyArray(IAllocator* pAlloc, uint32 n);   // 0x1001cf00
	uint32 GetNumAllocatedElements();                            // 0x10021a80

	int32 sub_1001dee0(uint32 index, const uint16& toInsert, IAllocator* pAlloc);
};

int32 Arr1001dee0::sub_1001dee0(uint32 index, const uint16& toInsert, IAllocator* pAlloc)
{
	uint16* pNewArray;
	uint32 newSize, i;

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


