// *********************************************************************** //
//
//  MODULE:   W18_1001DC70_OS (.CPP)  -- /Os
//
//  PURPOSE:  0x1001dc70, CMoArray<WORD>::NiceSetSize, which the vtable-only
//            GenAppendList 0x1001d9c0 (w18_1001d180_os.cpp) calls.  A file of
//            its own because icl inlines it into a same-file caller and the
//            shipped 0x1001d9c0 calls it (measured: 216 bytes against 195).
//            /Os: it pads with `8b f6`.
//
// *********************************************************************** //

#include "ltbasedefs.h"

struct IAllocator
{
	virtual void* Slot00(uint32 nSize);                 // vtable +0x00
	virtual void* Alloc(uint32 nSize);                  // vtable +0x04
	virtual void  Free(void* pMem);                     // vtable +0x08
};

extern IAllocator g_10091df0;                           // 0x10091df0

struct Arr1001d
{
	int32		InternalNiceSetSize(uint32 newSize, int32 bFast,
				                    IAllocator* pAlloc);      // 0x1001d810
	int32		sub_1001dc70(uint32 nNewSize);            // NiceSetSize
};


// ---------------------------------------------------------------------- //
//  0x1001dc70 -- 44 bytes.  NiceSetSize(DWORD n)
//  { return InternalNiceSetSize(n, FALSE, &g_DefAlloc); } -- the literal
//  zero in the middle outgoing dword and the allocator address in the
//  third, exactly as d3dr2b.cpp's 0x100507b0 over another T.  The worker
//  0x1001d810 is d3dr5d_os.cpp's Arr1001d810::InternalNiceSetSize.
// ---------------------------------------------------------------------- //
int32 Arr1001d::sub_1001dc70(uint32 nNewSize)
{
	return InternalNiceSetSize(nNewSize, LTFALSE, &g_10091df0);
}
