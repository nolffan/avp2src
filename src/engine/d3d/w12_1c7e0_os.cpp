// *********************************************************************** //
//
//  w12_1c7e0_os.cpp -- 0x1001c7e0 (160 bytes): CMoArray<T,C>::SetSize2 for
//  the instantiation whose helpers are 0x1001c450 (_AllocateTArray),
//  0x10021a80 (GetNumAllocatedElements) and 0x1001cf00
//  (_DeleteAndDestroyArray) -- the shipped calls at 0x1001c82d, 0x1001c865
//  and 0x1001c876. src/LT2/lithshared/stdlith/dynarray.h's SetSize2, the same
//  body d3dr5c.cpp writes for 0x1007f810 (T = DWORD, helpers 0x10009cc0 /
//  0x10009c90 / 0x10009cf0; the two are byte-for-byte the same shape).
//
//  References: d3dbatchb's Obj1001cd10::sub_1001c7e0(0, n) and d3dr2a's
//  C1001c880::sub_1001c7e0(n, &g_DefAlloc) -- both aliased onto this one
//  definition in link/aliases.rsp. The shipped body returns BOOL in %eax
//  (`xor %eax,%eax; inc %eax` / `xor %eax,%eax`); both callers ignore it.
//
//  IDENTICAL.  The residual was block layout -- the shipped
//  code hosts `if (m_pArray) _DeleteAndDestroyArray(...)` past the
//  epilogue and routes `newSize > 0`'s false edge through the early
//  `return TRUE` -- and it is NAMES.  icl 4.0 predicts branches from the
//  identifiers involved: a type whose name contains "fault", "error",
//  "fail", "fatal", "abort", "exit" or "warn" (measured: Fault, Xfault,
//  Default, XError, Fail, Fatal, Abort, Exit, Warn all move the layout;
//  Assert, Panic and forty random names do not) makes the paths through
//  its calls cold.  dynarray.h's real names do exactly that: the cache is
//  `DefaultCache`, and every member of `CMoArray<T, DefaultCache>` carries
//  it in its qualified name.  So the cache is spelled DefaultCache (fixes
//  the `ja`/`jmp` routing) and the array is a template over <T, C>
//  explicitly instantiated with it (sinks the delete).  The aliases for
//  the two callers and the four callees (link/aliases.rsp, "LL" block)
//  follow the new mangled names.
//
//  (History: measured, none kept: m_pArray typed uint32 flips the
//  OTHER test, `if ((uint32)m_pArray)` / `if (!m_pArray) {} else {...}`
//  do not move it.)
//
// *********************************************************************** //

typedef unsigned long uint32;
typedef int int32;

struct IAllocator;

struct DefaultCache
{
	uint32	sub_1000a670() const;                       // 0x1000a670 GetWantedCache
	void	sub_1000a680(uint32 nSize);                 // 0x1000a680 SetCacheSize
};

template<class T, class C>
struct Arr1001c7e0
{
	void*			m_pVTable;                          // +0x00
	T*				m_pArray;                           // +0x04
	uint32			m_nElements;                        // +0x08
	C				m_Cache;                            // +0x0c

	// Declared only -- all three are out of line in the image.
	T*		sub_1001c450(uint32 nElements, IAllocator* pAlloc);  // _AllocateTArray
	uint32	sub_10021a80();                                      // GetNumAllocatedElements
	void	sub_1001cf00(IAllocator* pAlloc, uint32 nElements);  // _DeleteAndDestroyArray

	int32	sub_1001c7e0(uint32 nNewSize, IAllocator* pAlloc);
};

template<class T, class C>
int32 Arr1001c7e0<T, C>::sub_1001c7e0(uint32 nNewSize, IAllocator* pAlloc)
{
	if ((nNewSize == m_nElements) && (nNewSize != 0))
	{
		return 1;
	}

	if (m_pArray)
	{
		sub_1001cf00(pAlloc, sub_10021a80());
	}

	m_nElements = nNewSize;
	if (nNewSize > 0)
	{
		m_pArray = sub_1001c450(nNewSize + m_Cache.sub_1000a670(), pAlloc);
		if (!m_pArray)
		{
			m_nElements = 0;
			m_Cache.sub_1000a680(0);
			return 0;
		}

		m_Cache.sub_1000a680(m_Cache.sub_1000a670());
	}

	return 1;
}

template struct Arr1001c7e0<void, DefaultCache>;
