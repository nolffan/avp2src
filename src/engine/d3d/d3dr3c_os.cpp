// *********************************************************************** //
//
//  MODULE:   D3DR3C_OS (.CPP) -- split out of d3dr3c.cpp
//
//  These four are d3dr3c.cpp functions whose shipped bytes say /Os, where
//  the rest of that file is /O2.  The tell is the tail: /O2 pads every
//  function to 16 inside its own COMDAT (`nop` + `lea` fill), /Os pads to
//  4 at most and emits nothing after a trailing `jmp`, and the shipped
//  image has the linker's 0x90 fill after each of these:
//
//    0x1000eef0  108 bytes, then 4 x 0x90   /O2: 112 (pad differs), /Os: identical
//    0x1000f4b0  374 bytes, then 10 x 0x90  /O2: 384 and 327 bytes off,
//                                           /Os: size-exact, identical once
//                                           its far-plane test is respelled
//    0x10040850  110 + `mov %esi,%esi`      /O2 adds a loop-alignment nop
//                (since moved to w34_1003e620_osoiqipsf.cpp: identical)
//    0x1003b7f0  233 + `lea 0(%esi)` + 0x90 /O2: 271 bytes, /Os: 235
//
//  Everything below is carried over from d3dr3c.cpp with its decode notes;
//  only the spellings noted in place changed.  Built /Os (D3D_OS_TUS).
//
// *********************************************************************** //

#include "ltbasedefs.h"


extern void sub_10021d60(uint32 nLevel, const char* pszMsg, ...);  // 0x10021d60 AddDebugMessage (was d3d_Warning*; icl treats a callee named *Warn*/*Error* as cold)

// The two pools 0x1002e2d0 creates (d3dr3c.cpp).
extern uint8  g_1008d4c8[];                             // 0x1008d4c8
extern uint8  g_1008d4e4[];                             // 0x1008d4e4


// ======================================================================= //
//  0x1000eef0 -- 108 bytes.  DECODED, every instruction.
//
//  Add one element to a per-index bucket, creating the bucket on demand.
//
//  FOUR parameters, and the fourth is dead: the one caller (0x10012766)
//  pushes four and cleans 0x10 off the stack, but the body reads only
//  0x4(%esp), 0x8(%esp) and 0xc(%esp).  A dead trailing parameter is
//  normal for this compiler and `add $0x10,%esp` proves it is there.
//
//  The two pools are the ones 0x1002e2d0 creates: 0x1008d4c8 with element
//  size 0xc (this function writes +0x00, +0x04, +0x08 of it and nothing
//  else) and 0x1008d4e4 with element size 0x18 (this function writes +0x00
//  and +0x14, the caller writes +0x04, +0x08 and +0x0c).
//
//  The bucket array lives at +0x1c of the object reached through
//  pOwner->+0x28->+0x30, indexed by the third argument, four bytes per
//  entry.  When a bucket is created it is ALSO pushed onto the singly
//  linked list whose head the second argument points at.
//
//  The `if (!pBucket)` block is placed after the return and jumps back
//  into the common tail -- icl sinking a cold block, not a source `goto`.
// ======================================================================= //

struct Elem1000eef0                                     // 0x18 bytes
{
	void*             m_pOwner;                         // +0x00
	void*             m_p04;                            // +0x04  caller sets
	void*             m_p08;                            // +0x08  caller sets
	void*             m_p0c;                            // +0x0c  caller sets
	uint32            m_n10;                            // +0x10
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
	Bucket1000eef0*   m_pBuckets[1];                    // +0x1c
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

extern void* sub_1000a6a0(void* pPool);                 // 0x1000a6a0

Elem1000eef0* sub_1000eef0(Owner1000eef0* pOwner, Bucket1000eef0** ppList,
                           uint32 nIndex, uint32 nUnused)
{
	Table1000eef0*   pTable = pOwner->m_pHolder->m_pTable;
	Bucket1000eef0*  pBucket = pTable->m_pBuckets[nIndex];
	Elem1000eef0*    pElem;

	if (!pBucket)
	{
		pBucket = (Bucket1000eef0*)sub_1000a6a0(g_1008d4c8);

		pBucket->m_pTable = pTable;
		pTable->m_pBuckets[nIndex] = pBucket;
		pBucket->m_pElements = LTNULL;
		pBucket->m_pNextInList = *ppList;
		*ppList = pBucket;
	}

	pElem = (Elem1000eef0*)sub_1000a6a0(g_1008d4e4);

	pElem->m_pNext = pBucket->m_pElements;
	pBucket->m_pElements = pElem;
	pElem->m_pOwner = pOwner;

	return pElem;
}


// 0x10040850 -- moved to w34_1003e620_osoiqipsf.cpp (beside its
// /Qip register callee 0x1003e620).



// ======================================================================= //
//  0x1003b7f0 -- 232 bytes (the `ret` is at 0x1003b805, reached from
//  0x1003b8d4).  DECODED, every instruction.
//
//  Rebuild a "set" from the master list at +0x2e4/+0x2e8: for every object
//  in the list, decide whether it belongs, and either append it to the
//  +0x304 array (bounded by +0x318, with "Set '%s' overflowed" naming the
//  set from +0x31c when it is full) or hand it to 0x1003b740.
//
//  The admission test is the awkward part and it is read literally:
//
//     test $0x1,%al   je  -> the else arm at 0x1003b84f
//     ...             je  -> the KEEP path, when the global is null
//     ...             jne -> the SKIP path
//
//  so the null test on 0x1008bd54 appears in BOTH arms, and the second
//  arm reads bit 0x400 out of %ah of the SAME +0x88 word it already
//  loaded.  icl 4.0 does not fold the duplicated null test, so it is
//  written out twice.
// ======================================================================= //

struct Obj1003b7f0
{
	uint8  m_pad_00[0x88];                              // +0x00..+0x87
	uint32 m_nFlags88;                                  // +0x88
	uint32 m_nFlags8c;                                  // +0x8c
	uint8  m_pad_90[0xf0];                              // +0x90..+0x17f
	uint16 m_nFlags180;                                 // +0x180
};

// The 0x20-byte "set" record d3dr5a.cpp reconstructs from 0x1003b790 and
// its three siblings (+0x00 elements, +0x04 count, +0x14 max, +0x18 name).
// The master list at +0x2e4 and the target at +0x304 are two of them.
struct Set1003b7f0
{
	void**	m_pElements;                                // +0x00
	uint32	m_nCount;                                   // +0x04
	uint8	m_Reserved08[0x0c];                         // +0x08..+0x13
	uint32	m_nMax;                                     // +0x14
	char*	m_pszName;                                  // +0x18
	uint32	m_n1c;                                      // +0x1c
};

struct Ctx1003b7f0
{
	uint8		m_Reserved000[0x2e4];                   // +0x000..+0x2e3
	Set1003b7f0	m_Set2e4;                               // +0x2e4
	Set1003b7f0	m_Set304;                               // +0x304
};

extern Ctx1003b7f0* sub_1007b8d0();                     // 0x1007b8d0
extern uint8  g_1008b880[];                             // 0x1008b880
extern uint32 g_1008ed80;                               // 0x1008ed80
extern uint32 g_1008bd54;                               // 0x1008bd54

extern void sub_1003b740(uint8* pUnused, Obj1003b7f0* pObj);   // 0x1003b740

// The add-to-set body of 0x1003b790 (d3dr5a.cpp), expanded here.
inline void Set_Add(Set1003b7f0* pSet, void* pItem)
{
	if (pSet->m_nCount < pSet->m_nMax)
	{
		pSet->m_pElements[pSet->m_nCount] = pItem;
		pSet->m_nCount++;
	}
	else
	{
		sub_10021d60(1, "Set '%s' overflowed", pSet->m_pszName);
	}
}

//  IDENTICAL.  Four things closed it, each measured against the
//  previous near (124 bytes differ):
//    * the warning's callee is spelled sub_10021d60, not d3d_Warning.
//      icl 4.0 treats a call to a function it takes for a diagnostic
//      (d3d_Warning, d3d_WarningA, Warning...) as a cold block and sinks it
//      past the loop exit; under a neutral name the warning stays between
//      the append and the 0x1003b740 call, as shipped (124 -> 72);
//    * the master list is a local pointer (pSrc).  With every access
//      through pCtx the count reload after the element store is scheduled
//      apart from its increment and the `incl 0x308(%ebp)` is lost;
//    * the append is the THEN arm (the 0x1003b740 call is laid out last);
//    * the flag is taken into a local first.  `if (!(f & 0x10))` directly
//      is canonicalised back to `if (f & 0x10)` with the arms swapped
//      (call first), and `!= 0x10` / `== 0` forms emit and+cmp.
void sub_1003b7f0()
{
	Ctx1003b7f0* pCtx;
	uint32       i;
	Set1003b7f0* pSrc;
	uint32       nFlag;

	if (!g_1008ed80)
		return;

	pCtx = sub_1007b8d0();
	pSrc = &pCtx->m_Set2e4;

	pCtx->m_Set304.m_nCount = 0;

	for (i = 0; i < pSrc->m_nCount; ++i)
	{
		Obj1003b7f0* pObj = (Obj1003b7f0*)pSrc->m_pElements[i];

		if (pObj->m_nFlags88 & 0x01)
		{
			if (g_1008bd54)
			{
				if (pObj->m_nFlags8c & 0x01)
					continue;
			}
		}
		else
		{
			if (g_1008bd54)
			{
				if (!(pObj->m_nFlags88 & 0x400))
					continue;
			}
		}

		nFlag = pObj->m_nFlags180 & 0x10;

		if (!nFlag)
			Set_Add(&pCtx->m_Set304, pObj);
		else
			sub_1003b740(g_1008b880, pObj);
	}
}


// ======================================================================= //
//  THE FRUSTUM CLIPPER
//
//  0x1000f4b0 -- 374 bytes.  DECODED, every instruction.
//
//  This is the driver for the twelve-body clip family, and 0x1000aa90 is
//  the SAME FUNCTION for the 40-byte vertex: diffing 0x1000f4b0 against
//  0x1000aa90 instruction for instruction produces only six call
//  displacements and the branch displacements, nothing else.  So this
//  decode is worth two rows of the image.
//
//  The six planes and their flag bits, from the six guarded call sites:
//
//     bit 0x01  ->  0x10001000 / 0x10002310    near   (z >= g_1008b908)
//     bit 0x04  ->  0x10001330 / 0x10002670    left   (x > -z)
//     bit 0x08  ->  0x10001670 / 0x100029e0    top    (y <  z)
//     bit 0x10  ->  0x10001990 / 0x10002d40    right  (x <  z)
//     bit 0x20  ->  0x10001cb0 / 0x100030a0    bottom (y > -z)
//     bit 0x02  ->  0x10001fe0 / 0x10003410    far    (z <= g_1008b910)
//
//  and the order the driver applies them in is near, left, top, right,
//  bottom, far -- NOT the numeric order of the bits, which is what makes
//  the far plane's 0x02 test come last.
//
//  Each call passes FOUR arguments: a scratch dword that the callee never
//  reads, &pVerts, &nVerts, &pOut.  The scratch dwords are six DISTINCT
//  consecutive stack slots (0x1c, 0x20, 0x24, 0x28, 0x2c, 0x30), one per
//  plane, so they are written here as an array indexed by the plane and
//  not as one reused local.
//
//  `and $0x1,%ebx` in the guard MUTATES the flag word -- after it, only
//  bit 0 can survive -- so 0x1008d7a0 is a "clip against the near plane
//  only" switch and the source really does assign back into the parameter.
// ======================================================================= //

struct ClipVert                                         // 32 bytes
{
	float  x;                                           // +0x00
	float  y;                                           // +0x04
	float  z;                                           // +0x08
	float  rhw;                                         // +0x0c
	uint8  b, g, r, a;                                  // +0x10..+0x13
	uint8  sb, sg, sr, sa;                              // +0x14..+0x17
	float  tu;                                          // +0x18
	float  tv;                                          // +0x1c
};

extern LTBOOL sub_10001000(uint32* pUnused, ClipVert** ppVerts,
                           int32* pnVerts, ClipVert** ppOut);
extern LTBOOL sub_10001330(uint32* pUnused, ClipVert** ppVerts,
                           int32* pnVerts, ClipVert** ppOut);
extern LTBOOL sub_10001670(uint32* pUnused, ClipVert** ppVerts,
                           int32* pnVerts, ClipVert** ppOut);
extern LTBOOL sub_10001990(uint32* pUnused, ClipVert** ppVerts,
                           int32* pnVerts, ClipVert** ppOut);
extern LTBOOL sub_10001cb0(uint32* pUnused, ClipVert** ppVerts,
                           int32* pnVerts, ClipVert** ppOut);
extern LTBOOL sub_10001fe0(uint32* pUnused, ClipVert** ppVerts,
                           int32* pnVerts, ClipVert** ppOut);

extern uint32     g_1008d7a0;                           // 0x1008d7a0
extern ClipVert*  g_1008b87c;                           // 0x1008b87c

LTBOOL sub_1000f4b0(uint32 nClipFlags, ClipVert** ppVerts, int32* pnVerts)
{
	ClipVert*  pOut;
	ClipVert*  pVerts;
	int32      nVerts;
	uint32     anUnused[6];

	if (g_1008d7a0)
	{
		nClipFlags &= 1;
		if (!nClipFlags)
			return LTTRUE;
	}

	pOut   = g_1008b87c;
	pVerts = *ppVerts;
	nVerts = *pnVerts;

	if (nClipFlags & 0x01)
	{
		if (!sub_10001000(&anUnused[0], &pVerts, &nVerts, &pOut))
			return LTFALSE;
	}

	if (nClipFlags & 0x04)
	{
		if (!sub_10001330(&anUnused[1], &pVerts, &nVerts, &pOut))
			return LTFALSE;
	}

	if (nClipFlags & 0x08)
	{
		if (!sub_10001670(&anUnused[2], &pVerts, &nVerts, &pOut))
			return LTFALSE;
	}

	if (nClipFlags & 0x10)
	{
		if (!sub_10001990(&anUnused[3], &pVerts, &nVerts, &pOut))
			return LTFALSE;
	}

	if (nClipFlags & 0x20)
	{
		if (!sub_10001cb0(&anUnused[4], &pVerts, &nVerts, &pOut))
			return LTFALSE;
	}

	// The far plane is written the other way round from the five above:
	// the shipped tail FALLS THROUGH into the write-back and jumps to the
	// shared `return 0` block (`je` to it), where the five-guards spelling
	// inverts the last test and lays the failure block first.  Measured:
	// this spelling is byte-identical, the other differs in the last 32.
	if (!(nClipFlags & 0x02) ||
	    sub_10001fe0(&anUnused[5], &pVerts, &nVerts, &pOut))
	{
		*ppVerts = pVerts;
		*pnVerts = nVerts;
		return LTTRUE;
	}

	return LTFALSE;
}
