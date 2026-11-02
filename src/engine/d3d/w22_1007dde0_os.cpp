// *********************************************************************** //
//
//  w22_1007dde0_os.cpp -- d3d.ren functions reached only through vtables,
//  written for the whole-DLL link.  /Os group (w<N>_*_os.cpp, see
//  build/engine.sh).
//
//  Two slots of the vertex-buffer pool's base vtable 0x10089a20 (both
//  also appear in the derived table 0x10089a40, at +0x08 and +0x10):
//
//  0x1007dde0  slot +0x08: unlock the current buffer and draw it INDEXED
//              (IDirect3DDevice7::DrawIndexedPrimitiveVB), then wrap --
//              the indexed twin of d3dr5c.cpp's 0x1007dd24.  NEAR.
//  0x1007e8b0  slot +0x10: Init(pD3D, nVertsPerVB, nNumVBs, a, b).  NEAR
//              (one argument load scheduled differently, below).
//
//  Why /Os: both pad with icl's short `lea 0x0(%esi),%esi` / 0x90 run and
//  the /Os build of each is the closer one by far (0x1007dde0: /O2
//  duplicates every epilogue, 240 bytes against 213; 0x1007e8b0: /O2 saves
//  with `mov` into the frame, 368 bytes against 314).
//
//  THE COLD-NAME LEVER, used twice here: icl 4.0 lays out a call to a
//  function whose decorated name contains "fault" (or warn, err, fail,
//  ...; see d3dr5c_o2.cpp at 0x100038e0) as a cold block, hosted past the
//  epilogue with its live values spilled around it.  The shipped code
//  hosts 0x1007dde0's wrap call and 0x1007e8b0's allocator calls exactly
//  that way, and the class names below carry "Default" so the same
//  layout comes out.  The names are otherwise arbitrary.
//
// *********************************************************************** //

#include "ltbasedefs.h"


// ======================================================================= //
//  0x1007dde0 -- 213 bytes, `ret $0x14`, `this` in %ecx.
//
//  WHAT PINS IT: the same two guards over +0x30 (initialised) and +0x34
//  (locked) sharing one exit as 0x1007dd24, the same Unlock (COM slot
//  +0x10) of m_pVBs[m_iCurVB] and the same zero stores to +0x38/+0x34; a
//  zero third argument returns 0; then `call *0x80(%eax)` with an
//  eight-dword argument area -- IDirect3DDevice7::DrawIndexedPrimitiveVB
//  (type, buffer, start vertex, vertex count, indices, index count,
//  flags = 0) -- from arguments 2..5 and m_nCurVertex (+0x1c).  Then
//  m_nCurVertex += count and, at or past +0x20, the wrap 0x1007dec4
//  (d3dr2d_os.cpp), called on the register entry past its /Qip shim.
//
//  NEAR.  Everything but the allocation matches: the shipped function
//  saves NO callee-saved register at entry -- `this` stays in %ecx and is
//  spilled to -0x1c(%ebp) around the two COM calls, and %edi/%esi/%ebx
//  are each saved and restored in the frame around their one use in the
//  argument set-up -- where icl here puts `this` in %ebx and pushes
//  %ebx/%esi.  Not moved by nesting the guards, a `this` copy, a buffer
//  local, C++ COM interfaces, cold-named interface types, or any of
//  /O1 /Os /O2 /Qip /Oi- /Op /Ob0 /G5 /G6.  The cold wrap call (the
//  `mov %edx,-0x14(%ebp); call; mov -0x14(%ebp),%edx; jmp` tail) does
//  match, through the class name.
//  +0x34 is an integer (uint32), not a pointer: the shipped
//  code puts its zero case on the fall-through (184 -> 178 bytes).  The
//  allocation is unchanged by it, by C++ COM interfaces deriving from an
//  IUnknown-like base, by cold-named Unlock /
//  DrawIndexedPrimitiveVB methods or fields, by /O2, or by renaming the
//  class to a non-cold name (only the wrap call moves back inline).
// ======================================================================= //
//  0x1007dde0 MOVED to w59_1007dd24_osoi.cpp, beside its
//  non-indexed twin 0x1007dd24, where both are BYTE-IDENTICAL: m_VBs is the
//  real dynarray.h CMoArray (its inlined operator[] is what gives the
//  shipped frame-save allocation described above), the buffer and
//  m_nCurVertex are read into block locals before the draw, and the wrap
//  is a `switch` whose default arm icl sinks (with the switch the class
//  name no longer matters: renamed without "Default" it stays identical).


// ======================================================================= //
//  0x1007e8b0 -- 314 bytes, `ret $0x14`, `this` in %ecx.
//
//      if (m_bInit && nVertsPerVB == m_nVertsPerVB && nNumVBs == m_nNumVBs)
//          return TRUE;                  // already set up this way
//      Term();                           // virtual +0x0c (0x1007def0)
//      m_nVertsPerVB = nVertsPerVB > 0 ? nVertsPerVB : m_nVertsPerVB;
//      m_nNumVBs     = nNumVBs > 0     ? nNumVBs     : m_nNumVBs;
//      m_28 = a;  m_2c = b;
//      m_VBs.Init(nNumVBs);              // dynarray.h, inlined, result dropped
//      if (!CreateBuffers(pD3D))         // virtual +0x1c (pure here;
//          return FALSE;                 //  0x1007e2c0 in the derived table)
//      m_bInit = TRUE;  return TRUE;
//
//  WHAT PINS IT:
//    * the already-initialised test is hosted past the epilogue (0x1007e9d0)
//      -- the `switch` default-arm shape, as at 0x1007e1c0;
//    * `test; jbe` then an unconditional store: `> 0 ? :`, not an `if`;
//    * the CMoArray<IDirect3DVertexBuffer7*> at +0x04 (vptr 0x10089a60,
//      m_pArray +0x08, m_nElements +0x0c, cache +0x10/+0x14) goes through
//      dynarray.h's Init -> Term -> SetSize2(0) -> _DeleteAndDestroyArray ->
//      BaseDelete, _InitArray(0) (the four zero stores), then SetSize2(n):
//      the re-tested `jne; jne` of `(n == m_nElements) && (n != 0)` with
//      m_nElements known 0, the second delete chain, and _AllocateTArray ->
//      BaseNew (`(n + wanted) * 4` as two `add`s, g_DefAlloc through a
//      pointer).  Init's BOOL is dropped: the allocation-failure arm joins
//      the CreateBuffers call.  The allocator and cache types are named so
//      icl hosts the delete and allocate arms cold, as shipped.
//
//  NEAR, one difference: the shipped code loads pD3D (0x8(%ebp)) into
//  %edi early, beside the loads for +0x28/+0x2c, and keeps it across the
//  array code; icl here reloads it from the argument slot just before the
//  call (9 bytes).  Not moved by a local copy (at the top or after the
//  stores), by folding the tail into an inline helper, or by store order.
//  Also unchanged: dynarray.h's REAL CMoArray (with LAlloc
//  spelled IAllocator, as w10_1007a280_os.cpp includes it -- the layout
//  stays identical), a local copy just before Init, the result through a
//  BOOL local, `== LTFALSE`, `this->`, and a `switch` on the result.
// ======================================================================= //
// The allocator as the array code sees it: l_allocator.h's LAlloc
// (vtable: destructor, Alloc +0x04, Free +0x08), under a cold name.  The
// object is g_DefAlloc, 0x10091df0, declared in the spelling
// d3dr5c_o2.cpp already uses and viewed through this type.
class LAllocDefault
{
public:
	virtual				~LAllocDefault();
	virtual void*		Alloc(uint32 nSize);
	virtual void		Free(void* pMem);
};
class LAllocR5C
{
public:
	virtual				~LAllocR5C();
	virtual void*		Alloc(uint32 nSize);
	virtual void		Free(void* pMem);
};
extern LAllocR5C g_10091df0;

struct VB7e8b0;                                         // IDirect3DVertexBuffer7
struct D3D7e8b0;                                        // IDirect3D7

// l_allocator.h's placement new and BaseNew/BaseDelete, verbatim but for
// the names: T is a pointer, so neither loop emits code, but they are what
// give the shipped nesting of the delete chain (three tests of m_pArray,
// each skipping to a different store).
class Dummy1007e8b0 { public: int asdf; };
inline void* operator new(size_t size, void* ptr, Dummy1007e8b0* pBlah) { return ptr; }
inline void operator delete(void* pDataPtr, void* ptr, Dummy1007e8b0* pBlah) {}

template<class T>
inline T* BaseNew1007e8b0(LAllocDefault* pAlloc, T* dummy, uint32 nElements)
{
	T *ptr, *pTemp;

	ptr = (T*)pAlloc->Alloc(sizeof(T) * nElements);
	if (!ptr)
		return LTNULL;

	pTemp = ptr;
	while (nElements)
	{
		--nElements;
		::new(pTemp, (Dummy1007e8b0*)0) T;
		++pTemp;
	}

	return ptr;
}

template<class T>
inline void BaseDelete1007e8b0(LAllocDefault* pAlloc, T* ptr, uint32 nElements)
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

// dynarray.h's DefaultCache and CMoArray<T,C>, the members Init reaches,
// written in the class body so /Ob1 expands them.
struct DefaultCache
{
	uint32	m_nCacheSize;
	uint32	m_nWantedCache;
	uint32	GetCacheSize() const	{ return m_nCacheSize; }
	void	SetCacheSize(uint32 n)	{ m_nCacheSize = n; }
	uint32	GetWantedCache() const	{ return m_nWantedCache; }
	void	SetWantedCache(uint32 n)	{ m_nWantedCache = n; }
};

template<class T, class C = DefaultCache>
class CMoArray
{
public:
	void*		m_pVtbl;                                // +0x04 in the owner
	T*		m_pArray;
	uint32		m_nElements;
	C		m_Cache;

	uint32	GetNumAllocatedElements() { return m_nElements + m_Cache.GetCacheSize(); }

	T* _AllocateTArray(uint32 nElements, LAllocDefault* pAlloc)
	{
		T* tPtr = BaseNew1007e8b0(pAlloc, (T*)0, nElements);
		return tPtr;
	}

	void _DeleteAndDestroyArray(LAllocDefault* pAlloc, uint32 nElements)
	{
		if (m_pArray)
		{
			BaseDelete1007e8b0(pAlloc, m_pArray, nElements);
			m_pArray = LTNULL;
		}
		m_Cache.SetCacheSize(0);
	}

	void _InitArray(uint32 wantedCache)
	{
		m_pArray = LTNULL;
		m_nElements = 0;
		m_Cache.SetWantedCache(wantedCache);
		m_Cache.SetCacheSize(0);
	}

	LTBOOL SetSize2(uint32 nNewSize, LAllocDefault* pAlloc)
	{
		if ((nNewSize == m_nElements) && (nNewSize != 0))
			return LTTRUE;

		if (m_pArray)
			_DeleteAndDestroyArray(pAlloc, GetNumAllocatedElements());

		m_nElements = nNewSize;
		if (nNewSize > 0)
		{
			m_pArray = _AllocateTArray(nNewSize + m_Cache.GetWantedCache(), pAlloc);
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

	LTBOOL SetSize(uint32 nNewSize) { return SetSize2(nNewSize, (LAllocDefault*)&g_10091df0); }
	void Term(LAllocDefault* pAlloc = (LAllocDefault*)&g_10091df0) { SetSize2(0, pAlloc); }
	LTBOOL Init(uint32 size = 0, uint32 cacheSize = 0)
	{
		Term();
		_InitArray(cacheSize);
		return SetSize(size);
	}
};

class VBBatch1007e8b0
{
public:
	virtual void	Slot00(uint32 nFlags) = 0;
	virtual long	Slot04() = 0;
	virtual long	Slot08() = 0;
	virtual void	Term() = 0;                          // +0x0c  0x1007def0
	virtual LTBOOL	Slot10(D3D7e8b0* pD3D, uint32 nVertsPerVB, uint32 nNumVBs, uint32 n28, uint32 n2c) = 0;   // +0x10  (this function)
	virtual void*	Slot14() = 0;
	virtual long	Slot18() = 0;
	virtual LTBOOL	CreateBuffers(D3D7e8b0* pD3D) = 0;       // +0x1c

	LTBOOL	sub_1007e8b0(D3D7e8b0* pD3D, uint32 nVertsPerVB, uint32 nNumVBs,
	                     uint32 n28, uint32 n2c);

	CMoArray<VB7e8b0*>	m_VBs;                                  // +0x04
	uint32		m_iCurVB;                               // +0x18
	uint32		m_nCurVertex;                           // +0x1c
	uint32		m_nVertsPerVB;                          // +0x20
	uint32		m_nNumVBs;                              // +0x24
	uint32		m_n28;                                  // +0x28
	uint32		m_n2c;                                  // +0x2c
	uint32		m_bInit;                                // +0x30
};

LTBOOL VBBatch1007e8b0::sub_1007e8b0(D3D7e8b0* pD3D, uint32 nVertsPerVB,
                                     uint32 nNumVBs, uint32 n28, uint32 n2c)
{
	switch (m_bInit)
	{
		case 0: break;
		default:
			if (nVertsPerVB == m_nVertsPerVB && nNumVBs == m_nNumVBs)
				return LTTRUE;
	}

	Term();

	m_nVertsPerVB = nVertsPerVB > 0 ? nVertsPerVB : m_nVertsPerVB;
	m_nNumVBs = nNumVBs > 0 ? nNumVBs : m_nNumVBs;
	m_n28 = n28;
	m_n2c = n2c;

	m_VBs.Init(nNumVBs);

	if (!CreateBuffers(pD3D))
		return LTFALSE;

	m_bInit = 1;
	return LTTRUE;
}
