// *********************************************************************** //
//
//  MODULE:   W15_1000D9F0_OS (.CPP)  -- /Os
//
//  PURPOSE:  Two of the polygon draw-slot routines 0x100256d0
//            (d3dqip2.cpp) installs in 0x1008ce94..0x1008cea0, each
//            `void (*)(Obj*)` with one stack argument.  Neither draws:
//            each queues the polygon for a later batched pass.
//
//              0x1000d9f0  push the polygon, with the current clip flags,
//                          on the deferred list 0x1008ad90 (drained by
//                          0x1000b6c0, w2_1000b6c0_os.cpp);
//              0x1003dc80  queue it under its texture's second bucket
//                          (table +0x20), creating the bucket on the live
//                          list 0x1008d4c0 first if the texture has none --
//                          the same insertion 0x1000eef0 (d3dr3c_os.cpp)
//                          does for an indexed bucket.
//
//  Both use the 0x18-byte element pool 0x1008d4e4 (free list 0x1008d4fc)
//  and 0x1003dc80 also the 0x0c-byte bucket pool 0x1008d4c8 (free list
//  0x1008d4e0), through StdLith's inline sb_Allocate (out of line at
//  0x1000a6a0; big24020.cpp documents the inlined shape), the bank's own
//  fields folded to absolutes.
//  0x1000d9f0 tests the element it gets; 0x1003dc80, like 0x10024020,
//  stores through it untested.
//
//  /Os, not /O2 (measured): 0x1000d9f0's refill call is icl's /Os
//  push form -- `add $0x8,%esp ; push %edx ; push $0x1008d4e4 ; call`
//  inside the 0xc frame -- where /O2 stores the two arguments with `mov`
//  into the outgoing area (18 bytes off).  Built /Os both functions are
//  byte-identical (0x1003dc80 was already identical at /O2 and stays so);
//  /Os /Oi- and /O1 give the same bytes.  The padding after both is the
//  /Os `lea 0x0(%esi),%esi` to a 4-byte boundary.
//
// *********************************************************************** //

#include "ltbasedefs.h"

struct Elem1000eef0                                     // 0x18 bytes
{
	void*             m_pOwner;                         // +0x00
	void*             m_p04;                            // +0x04
	void*             m_p08;                            // +0x08
	void*             m_p0c;                            // +0x0c
	uint32            m_n10;                            // +0x10  clip flags
	Elem1000eef0*     m_pNext;                          // +0x14
};

struct Bucket1000eef0                                   // 0x0c bytes
{
	void*             m_pTable;                         // +0x00
	Elem1000eef0*     m_pElements;                      // +0x04
	Bucket1000eef0*   m_pNextInList;                    // +0x08
};

struct Table1000eef0
{
	uint8             m_pad_00[0x1c];                   // +0x00..+0x1b
	Bucket1000eef0*   m_pBuckets[2];                    // +0x1c
};

struct Holder1000eef0
{
	uint8             m_pad_00[0x30];                   // +0x00..+0x2f
	Table1000eef0*    m_pTable;                         // +0x30
};

struct Owner1000eef0
{
	uint8             m_pad_00[0x28];                   // +0x00..+0x27
	Holder1000eef0*   m_pHolder;                        // +0x28
};

struct Elem24024;

// StdLith's StructBank (struct_bank.h), under the name the d3d objects
// already link the two pools by.
struct StructLink124a4
{
	StructLink124a4*  m_pSLNext;
};

struct Pool124a4
{
	uint32            m_StructSize;                     // +0x00
	uint32            m_AlignedStructSize;              // +0x04
	uint32            m_CacheSize;                      // +0x08
	uint32            m_nPages;                         // +0x0c
	uint32            m_nTotalObjects;                  // +0x10
	void*             m_PageHead;                       // +0x14
	StructLink124a4*  m_FreeListHead;                   // +0x18
};

extern Pool124a4          g_1008d4e4;                   // 0x1008d4e4  element pool
extern Pool124a4          g_1008d4c8;                   // 0x1008d4c8  bucket pool
extern Bucket1000eef0*    g_1008d4c0;                   // 0x1008d4c0  live bucket list
extern Elem24024*         g_1008ad90;                   // 0x1008ad90  deferred list
extern uint32             g_1008b874;                   // 0x1008b874  clip flags

// 0x1007fbf0 -- StdLith's sb_AllocateNewStructPage.
extern "C" int  sub_1007fbf0(Pool124a4* pBank, uint32 nAllocations);

// struct_bank.h's sb_Allocate (the out-of-line copy is 0x1000a6a0).
inline void* sb_Allocate124a4(Pool124a4* pBank)
{
	StructLink124a4* pRet;

	pRet = pBank->m_FreeListHead;
	if (!pRet)
	{
		sub_1007fbf0(pBank, pBank->m_CacheSize);
		pRet = pBank->m_FreeListHead;
		if (!pRet)
			return 0;
	}

	pBank->m_FreeListHead = pRet->m_pSLNext;
	return pRet;
}


// ======================================================================= //
//  0x1000d9f0 -- 93 bytes.
// ======================================================================= //
void sub_1000d9f0(void* pObj)
{
	Elem1000eef0* pElem;

	pElem = (Elem1000eef0*)sb_Allocate124a4(&g_1008d4e4);

	if (pElem)
	{
		Elem1000eef0* pNext;

		pElem->m_n10    = g_1008b874;
		pNext           = (Elem1000eef0*)g_1008ad90;
		g_1008ad90      = (Elem24024*)pElem;
		pElem->m_pNext  = pNext;
		pElem->m_pOwner = pObj;
	}
}


// ======================================================================= //
//  0x1003dc80 -- 205 bytes.
// ======================================================================= //
void sub_1003dc80(void* pvPoly)
{
	Owner1000eef0*   pPoly = (Owner1000eef0*)pvPoly;
	Table1000eef0*   pTable = pPoly->m_pHolder->m_pTable;
	Bucket1000eef0*  pBucket = pTable->m_pBuckets[1];
	Elem1000eef0*    pElem;

	if (!pBucket)
	{
		pBucket = (Bucket1000eef0*)sb_Allocate124a4(&g_1008d4c8);

		pBucket->m_pTable = pTable;
		pTable->m_pBuckets[1] = pBucket;
		pBucket->m_pElements = LTNULL;
		pBucket->m_pNextInList = g_1008d4c0;
		g_1008d4c0 = pBucket;
	}

	pElem = (Elem1000eef0*)sb_Allocate124a4(&g_1008d4e4);

	pElem->m_pNext = pBucket->m_pElements;
	pBucket->m_pElements = pElem;
	pElem->m_pOwner = pPoly;
	pElem->m_n10 = g_1008b874;
}
