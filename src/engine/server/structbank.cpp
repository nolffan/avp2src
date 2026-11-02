// server.dll — the struct bank's allocate (reconstruction). Build /O1.
//
// A bank hands out fixed-size records from chunks it grows on demand. Only
// the allocate lives inside the app region; the init, grow and teardown are
// in the statically linked half of the image. The file name is ours.
#include <string.h>
#include "servermgr.h"

// 0x1003d240 — add another chunk's worth of records to the free list (name
// ours; it lives outside the reconstructed region).
void sb_Grow(StructBank* pBank, uint32 nCount);

// 0x1003e280 is the CRT's memset: its normal-path return reads the
// destination (lithtech/ccobj.cpp), and /O1's lack of /Oi keeps it a call.

// sb_Allocate moved to servermgr.h as `inline` -- see the note there.
// It has to be visible to every TU, not just this one, because the
// shipped /O2 build expands it at every call site.


// 0x10028933 — the same allocate, CLEARED. Kept beside its twin even though
// its address falls in the manager's run rather than near sb_Allocate, since
// it pushes no string literal and so is not sensitive to which TU it compiles
// in (name ours).
//
// The length it clears is the bank's 0x00, not the 0x04 that sb_Grow
// multiplies by, which is the only thing in the reconstruction that
// distinguishes the two fields. And the fill is a memset call rather than the
// intrinsic, so a record of any size costs a call.
void* sb_AllocateCleared(StructBank* pBank)
{
	void* pStruct = pBank->m_pFreeList;

	if (!pStruct)
	{
		sb_Grow(pBank, pBank->m_Unknown08);

		pStruct = pBank->m_pFreeList;
		if (!pStruct)
			return LTNULL;
	}

	pBank->m_pFreeList = *(void**)pStruct;

	memset(pStruct, 0, pBank->m_StructSize);

	return pStruct;
}


// The three halves of creating an object (names ours): allocate the game-side
// BaseClass for a class, put a precreated one into the world, and hand one
// back when that fails.
// 0x10001c54 — hand out one instance of a registered class.
//
// The instance bank is not on the manager: it is found through the class
// definition's own internal pointer table. `ClassDef::m_pInternal[nIndex]`,
// indexed by the class manager's module slot (CServerMgr+0x834 =
// CClassMgr::m_nIndex), is this server's CClassStats for that class, and the
// StructBank at its +8 is where the instances come from. That is the same
// slot CClassMgr::Term clears on the way out.
//
// Two things the bytes say that a careful reconstruction would not write of
// its own accord. The allocation result is *not* null-checked — a bank that
// cannot grow crashes the caller. And only two of BaseClass's four members
// are cleared, m_hObject before m_pFirstAggregate (descending offsets, so
// this is statement order and not the compiler's), leaving m_nType and the
// vptr to the class's own ConstructFn.
LPBASECLASS sm_AllocateObject(CServerMgr* pMgr, struct ClassDef* pClassDef)
{
	CClassStats* pStats;
	LPBASECLASS  pObject;

	pStats = (CClassStats*)pClassDef->m_pInternal[pMgr->m_ClassMgr.m_nIndex];

	pObject = (LPBASECLASS)sb_Allocate(&pStats->m_Bank);

	pObject->m_hObject = LTNULL;
	pObject->m_pFirstAggregate = LTNULL;

	pClassDef->m_ConstructFn(pObject);

	return pObject;
}
