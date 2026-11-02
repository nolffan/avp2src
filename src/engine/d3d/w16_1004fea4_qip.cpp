// *********************************************************************** //
//
//  w16_1004fea4_qip.cpp -- d3d.ren, written for the link.
//  /O2 /Qip (the padding after `ret $0x4` is the /O2 multi-byte nop).
//
//  THE SHIPPED FUNCTION IS A /Qip TWO-ENTRY PAIR.  0x1004fea0 is
//  `mov 0x4(%esp),%eax` -- the stack entry icl emits in front of an
//  EXTERNAL __thiscall member under /Qip (a dead load: the body still
//  reads its argument from the stack) -- and 0x1004fea4 is the `$`
//  register entry, which the one caller (0x1004fbc0, `push %edi ;
//  mov $0x1008ef20,%ecx ; movl $0x1008b880,(%esp) ; call 0x1004fea4`)
//  enters.  Measured with a probe TU holding this body plus a same-file
//  caller at /O2 /Qip /Qip_no_inlining: icl emits exactly that pair
//  (`?sub_1004fea4@...` = the 4-byte shim, `?sub_1004fea4$@...` = the body)
//  and the register body is instruction-for-instruction the shipped one
//  (one differing block, our trailing padding only).  Here, with no caller
//  in the file, there is no shim, the body starts 16-aligned instead of
//  at 4 mod 16, and its three loop-head alignment runs come out different
//  sizes -- the ONLY difference (5 differing blocks, all nop runs).  Byte
//  identity needs this body in one /Qip TU with 0x1004fbc0
//  (w7_100500a0_qip.cpp).  NEAR.
//
//  THAT MOVE WAS MEASURED AND NOT KEPT.  One file with
//  0x1004fbc0, this body (named sub_1004fea0, after its stack entry) and
//  0x100500a0 at /O2 /Qip /Qip_no_inlining: the shim 0x1004fea0, the body
//  0x1004fea4 and 0x100500a0 all byte-identical, 0x1004fbc0 unchanged --
//  once the gap is read before the shift (below: the other order swaps
//  their stack slots 0x10/0x14, the last 4 bytes) and the fields are read
//  through a view of `this` (CMoArray's m_pArray is private; GetArray()
//  reloads it every iteration).  But the static s_Queue's CMoArray
//  vtable is instantiated in that TU, and under /Qip_no_inlining its
//  GenBegin no longer expands GenListPos's constructor: 0x10050770,
//  byte-identical from w7_100500a0_qip.obj today, is lost.  At /O2 /Qip
//  (inlining on) this body is inlined into 0x1004fbc0 (1085 bytes) --
//  also with the sort defined after its caller, and with a local timer
//  object for the compiled-out tick count.  So the shipped TU inlined
//  in-class members but not this sort, and what stopped it is not found.
//
//  0x1004fea4 -- 503 bytes (0x1004fea0 with the shim).  C100500a0::sub_1004fea4, the deferred-draw
//  queue's sort (called once, from the end-of-frame flush 0x1004fbc0 in
//  w7_100500a0_qip.cpp, with the draw context 0x1008b880).
//
//  Two passes over the queue's twelve-byte {item, draw callback, key}
//  records (m_pArray at +0x04, the count in use at +0x14):
//
//    1. the key: the squared distance from the context's position
//       (+0x4b4) to the item's position (+0x18c) -- or, for an item with
//       bit 0x40 of its +0x88 flags set, the squared length of its
//       position alone (already camera-relative);
//    2. a shell sort over the gap tables 0x10089560 (the shift,
//       {9..0}) and 0x10089588 (the gap, {512..1}): each pass compares
//       a[j << shift] against a[(j << shift) + gap] for j below
//       (count - 1) >> shift, swaps them when they are out of order, and
//       backs up one (`if (j) j -= 2`).  Out of order: the 0x40-flagged
//       item last -- `(fb - fa) >> 31`, unsigned, when the two flags
//       differ (the `sub ; shr $0x1f` in the bytes) -- else by key, the
//       nearer one moved back: far to near.  The swap test is materialised
//       into a register (`mov $1 / jb / mov $0`), so it is a variable.
//       fb is read before fa and the test is `fa == fb` (measured: either
//       other order swaps the flag registers or the cmp operands).
//
//  and then the "Sort: %d ticks" report through the console print slot
//  (+0x14 of 0x1008cbcc) when SortProfile (0x1008ef60) is set.  The tick
//  count is a literal 0 in the shipped bytes (`movl $0x0,0x4(%esp)`): the
//  timer it printed was compiled out of the release build.
//
// *********************************************************************** //

#include "ltbasedefs.h"

struct Item1004fea4
{
	uint8		m_pad_000[0x88];
	uint32		m_dwFlags88;                            // +0x88
	uint8		m_pad_08c[0x18c - 0x8c];
	LTVector	m_vPos;                                 // +0x18c
};

struct Ctx1004fea4
{
	uint8		m_pad_000[0x4b4];
	LTVector	m_vPos;                                 // +0x4b4
};

struct Rec1004fea4
{
	Item1004fea4*	m_pItem;                            // +0x00
	void*			m_pfnDraw;                          // +0x04
	float			m_fKey;                             // +0x08
};

// The queue (w7_100500a0_qip.cpp: a CMoArray<W7Pending500a0> plus the count in
// use); only the fields this function reads.
class C100500a0
{
public:
	void	sub_1004fea4(void* pCtx);                   // 0x1004fea4

	void*			m_pVtbl;                            // +0x00
	Rec1004fea4*	m_pArray;                           // +0x04
	uint8			m_pad_08[0x14 - 0x08];
	uint32			m_nUsed;                            // +0x14
};

struct Counters10059590
{
	uint8	m_pad_00[0x14];                             // +0x00
	void	(*m_pfnPrint14)(const char* pszFmt, ...);   // +0x14
};

// The "SortProfile" console variable (d3dr5a.cpp's tag for it); its
// value is the first dword.
struct Bank10021f40
{
	int32       m_nValue00;                             // +0x00
};

extern Counters10059590*	g_1008cbcc;                 // 0x1008cbcc
extern Bank10021f40			g_1008ef60;                 // 0x1008ef60  "SortProfile"
extern uint32				g_10089560[10];             // 0x10089560  shell sort shifts {9..0}
extern uint32				g_10089588[10];             // 0x10089588  shell sort gaps {512..1}

void C100500a0::sub_1004fea4(void* pCtx)
{
	uint32			i;
	uint32			k;
	int32			j;
	int32			n;
	int32			a;
	uint32			nShift;
	int32			nGap;
	uint32			fa;
	uint32			fb;
	uint32			bSwap;
	Item1004fea4*	pItem;
	Rec1004fea4*	pArr;
	Rec1004fea4*	pA;
	Rec1004fea4*	pB;
	Rec1004fea4		tmp;
	float			dx;
	float			dy;
	float			dz;

	for (i = 0; i < m_nUsed; i++)
	{
		pItem = m_pArray[i].m_pItem;

		if (!(pItem->m_dwFlags88 & 0x40))
		{
			dx = pItem->m_vPos.x - ((Ctx1004fea4*)pCtx)->m_vPos.x;
			dy = pItem->m_vPos.y - ((Ctx1004fea4*)pCtx)->m_vPos.y;
			dz = pItem->m_vPos.z - ((Ctx1004fea4*)pCtx)->m_vPos.z;
			m_pArray[i].m_fKey = dx * dx + dy * dy + dz * dz;
		}
		else
		{
			m_pArray[i].m_fKey = pItem->m_vPos.x * pItem->m_vPos.x +
			                     pItem->m_vPos.y * pItem->m_vPos.y +
			                     pItem->m_vPos.z * pItem->m_vPos.z;
		}
	}

	pArr = m_pArray;

	for (k = 0; k < 10; k++)
	{
		nGap   = g_10089588[k];
		nShift = g_10089560[k];
		n      = (m_nUsed - 1) >> nShift;

		for (j = 0; j < n; j++)
		{
			a  = j << nShift;
			pA = &pArr[a];
			pB = &pArr[a + nGap];

			fb = pB->m_pItem->m_dwFlags88 & 0x40;
			fa = pA->m_pItem->m_dwFlags88 & 0x40;

			if (fa == fb)
				bSwap = pA->m_fKey < pB->m_fKey;
			else
				bSwap = (fb - fa) >> 31;

			if (bSwap)
			{
				tmp = *pA;
				*pA = *pB;
				*pB = tmp;

				if (j)
					j -= 2;
			}
		}
	}

	if (g_1008ef60.m_nValue00)
		g_1008cbcc->m_pfnPrint14("Sort: %d ticks", 0);
}
