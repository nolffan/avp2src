// *********************************************************************** //
//
//  MODULE:   BIG73140 (.CPP)
//
//  PURPOSE:  0x10073140 -- add one object to its texture's render bucket,
//            creating the bucket if the texture has none yet.
//
//  EXTENT:   0x10073140..0x10073210, 208 bytes, 3 calls (all 0x1007fbf0).
//            Body ends 0x1007320d; the last 3 bytes are a lea-nop pad.
//
//  WHY IT WAS NOT FOUND BEFORE:  the enumeration merges it into the row
//  `0x100729c0, 2,128`, whose own function is the bucket FLUSH at
//  0x100729c0..0x1007313c.  Nothing branches across the four `nop`s at
//  0x1007313c, and this function opens `push %ebx` rather than
//  `push %ebp` -- so a `push %ebp` prologue count
//  could not see it.  That is the same frameless blind spot that hid
//  0x10024024's register entry, in a different guise.  The checker now
//  also flags "alignment padding whose successor nothing in the range
//  branches to", which is what catches this one.
//
//  SIGNATURE, off the bytes:  `push %ebx ; mov 0x8(%esp),%ebx` reads one
//  argument past the saved %ebx, `add $0x10,%esp ; pop %ebx ; ret` is a
//  plain `ret`, and no exit sets %eax -- so `void __cdecl f(Obj*)`.
//
//  RELATIONSHIP TO 0x10024020, already reconstructed in big24020.cpp:
//  that function pushes an element onto the DEFERRED list at 0x1008d4bc
//  with the same 0x18-byte element type; this one pushes onto a
//  per-texture bucket.  Both take their element from the same free list
//  (0x1008d4fc) and refill it the same way, and both STORE THROUGH THE
//  NULL the refill can return -- see REGION 2.
//
// *********************************************************************** //

#include "ltbasedefs.h"


// ----------------------------------------------------------------------- //
//  Types.  Reused verbatim from big124a4.cpp, which modelled the bucket
//  and element from the consumer side before this producer was read; every
//  offset this function touches lands on one of its fields.
// ----------------------------------------------------------------------- //

struct Elem73140                                        // 0x18 bytes
{
	void*           m_pOwner;                           // +0x00
	uint32          m_n04;                              // +0x04
	uint32          m_n08;                              // +0x08
	uint32          m_n0c;                              // +0x0c
	uint32          m_nClipFlags10;                     // +0x10
	Elem73140*      m_pNext;                            // +0x14
};

struct Bucket73140                                      // 0x0c bytes
{
	void*           m_pTexture;                         // +0x00
	Elem73140*      m_pElements;                        // +0x04
	Bucket73140*    m_pNextInList;                      // +0x08
};

// The texture record. Only +0x1c is reached here: the bucket this texture
// is currently accumulating into. 0x100729c0 clears it (`movl $0x0,0x1c`)
// as it frees each bucket, which is what makes the null test below the
// "has this texture been touched this frame" test.
struct Tex73140
{
	uint8           m_pad_00[0x1c];                     // +0x00 .. +0x1b
	Bucket73140*    m_pBucket1c;                        // +0x1c
};

struct Obj73140
{
	uint8           m_pad_00[0x28];                     // +0x00 .. +0x27
	struct Sub28*   m_p28;                              // +0x28
};

struct Sub28
{
	uint8           m_pad_00[0x30];                     // +0x00 .. +0x2f
	Tex73140*       m_pTex30;                           // +0x30
};

// StdLith's StructBank (struct_bank.h), as big24090.cpp has it: the two
// pools are banks of 0x18-byte elements (0x1008d4e4, free list +0x18 =
// 0x1008d4fc) and 0x0c-byte buckets (0x1008d4c8, free list 0x1008d4e0).
struct StructLink73140
{
	StructLink73140*  m_pSLNext;
};

struct Pool73140
{
	uint32            m_StructSize;                     // +0x00
	uint32            m_AlignedStructSize;              // +0x04
	uint32            m_CacheSize;                      // +0x08
	uint32            m_nPages;                         // +0x0c
	uint32            m_nTotalObjects;                  // +0x10
	void*             m_PageHead;                       // +0x14
	StructLink73140*  m_FreeListHead;                   // +0x18
};

// 0x1007fbf0 -- StdLith's sb_AllocateNewStructPage.
extern "C" int      sub_1007fbf0(Pool73140* pBank, uint32 nAllocations);

extern Pool73140    g_Pool1008d4e4;                     // 0x1008d4e4  elements
extern Pool73140    g_Pool1008d4c8;                     // 0x1008d4c8  buckets
extern Bucket73140* g_pBucketList1008d4c4;              // 0x1008d4c4
extern uint32       g_nClipFlags1008b874;               // 0x1008b874

// struct_bank.h's inline sb_Allocate, line for line (release build).
inline void* sb_Allocate73140(Pool73140* pBank)
{
	StructLink73140* pRet;

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
//  0x10073140 -- IDENTICAL (was 142 bytes off).  Both free-list
//  pops are struct_bank.h's sb_Allocate, inlined: the "refill can return
//  NULL and the NULL is stored through" shape is sb_Allocate's own
//  `return 0` with no test in this caller (as 0x10024020 in big24090.cpp).
// ======================================================================= //
void sub_10073140(Obj73140* pObj)
{
	Tex73140*		pTex = pObj->m_p28->m_pTex30;
	Bucket73140*	pBucket = pTex->m_pBucket1c;
	Elem73140*		pElem;

	if (!pBucket)
	{
		pBucket = (Bucket73140*)sb_Allocate73140(&g_Pool1008d4c8);
		pBucket->m_pTexture    = pTex;
		pTex->m_pBucket1c      = pBucket;
		pBucket->m_pElements   = 0;
		pBucket->m_pNextInList = g_pBucketList1008d4c4;
		g_pBucketList1008d4c4  = pBucket;
	}

	pElem = (Elem73140*)sb_Allocate73140(&g_Pool1008d4e4);
	pElem->m_pNext        = pBucket->m_pElements;
	pBucket->m_pElements  = pElem;
	pElem->m_pOwner       = pObj;
	pElem->m_nClipFlags10 = g_nClipFlags1008b874;
}


// ======================================================================= //
//  COVERAGE
//
//    0x10073140..0x10073154   21 B  prologue, pTex/pBucket, the null test
//    0x10073155..0x10073167   19 B  element pop
//    0x10073168..0x10073180   25 B  the four stores and the epilogue
//    0x10073181..0x100731ac   44 B  element-pool refill (sunk)
//    0x100731ad..0x100731e0   52 B  bucket create
//    0x100731e1..0x1007320c   44 B  bucket-pool refill (sunk)
//    0x1007320d..0x1007320f    3 B  lea-nop pad
//                            ----
//                             208 B
//
//  No `// UNDECODED:` gaps.
//
//  MEASURED call sequence: 2 shipped, 2 ours, 2 agreeing, no residual.  (A first
//  count of 3 included a 0x1007fbf0 call that belongs to the FLUSH
//  function sharing this row -- the split is exactly what that mistake
//  was about.)  Both
//  three go to 0x1007fbf0 and the two refill blocks are sunk in the
//  shipped build; if ours emits them inline the order is unchanged,
//  because both sunk blocks return to the same place they left.
//
//  COIN-FLIP, and it does not move a call: whether the free-list links are
//  spelled as a cast of +0x00 (written) or as a union with a dedicated
//  `m_pNextFree`.  The bytes reuse +0x00 for both roles and cannot
//  distinguish the two spellings.
// ======================================================================= //
