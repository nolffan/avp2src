// ======================================================================= //
//  w7_100500a0_qip.cpp -- d3d.ren functions written for the whole-DLL link
//: the deferred-draw queue at 0x1008ef20.  /O2 /Qip (multi-byte
//  lea padding).
//
//  /O2 /Qip, not /O2: the queue's CMoArray<W7Pending500a0> is instantiated
//  here, and at /O2 its virtuals came out with Insert2/Remove2/SetSize2
//  INLINED (GenAppend 424 bytes, GenRemoveAt 392, GenCopyList 417 ...):
//  nine COMDATs the shipped d3d.ren does not have.  At /O2 /Qip the stock
//  dynarray.h bodies are the shipped vtable 0x100896c0's twelve slots,
//  every one byte-identical (0x100503f0 GenSetCacheSize .. 0x10050770
//  GenBegin, and 0x1007c700 GenIsValid), and 0x1004fbc0 and 0x100500a0
//  stay identical.  (The sort 0x1004fea4 still lives apart, in
//  w16_1004fea4_qip.cpp: in this TU /O2 /Qip inlines it, see there.)
// ======================================================================= //
#include "ltbasedefs.h"
#include "../../LT2/lithshared/stdlith/dynarray.h"


// One queued draw: an item and the callback that draws it; the third
// dword is always written zero.
struct W7Pending500a0
{
	void*	m_pItem;                                    // +0x00
	void	(*m_pfnDraw)(void* pCtx, void* pItem);      // +0x04
	uint32	m_n08;                                      // +0x08
};

// 0x1008ef20: a CMoArray of those (vptr, m_pArray, m_nElements, the two
// DefaultCache dwords) plus the count in use at +0x14.
class C100500a0;
extern C100500a0*		g_p1008ef3c;                    // 0x1008ef3c  the queue

class C100500a0 : public CMoArray<W7Pending500a0>
{
public:
	// The constructor 0x1004fbc0 expands for its function-local static:
	// the count in use zeroed and the global pointer at 0x1008ef3c set.
	C100500a0()
	{
		m_nUsed = 0;
		g_p1008ef3c = this;
	}

	void	sub_100500a0(void* pItem, void (*pfnDraw)(void* pCtx, void* pItem));
	void	sub_10050284(uint32 nSize);                 // 0x10050284  (d3dr5c.cpp)
	void	sub_1004fea4(void* pCtx);                   // 0x1004fea4  the sort

	uint32	m_nUsed;                                    // +0x14
};

extern uint32			g_1008ef40;                     // 0x1008ef40  queueing enabled
extern unsigned char	g_1008b880[28];                 // 0x1008b880  the draw context


// ======================================================================= //
//  0x1004fbc0 -- 639 bytes (then `nop`).  The renderer's end-of-frame
//  flush.  The six guarded calls through the render struct's +0x24 hook
//  (g_1008cbcc, called with no arguments) bracket the set-building passes
//  0x10068840, 0x100438f0, 0x1005d9c0 and 0x1003b7f0; then 0x10023eb0,
//  0x10043a30, 0x10068890, 0x10023f40 and the optional hook at 0x1008ef80.
//
//  THE QUEUE IS A FUNCTION-LOCAL STATIC.  0x1008ef38 bit 0 is its
//  once-flag; the guarded block is CMoArray's constructor expanded
//  (Clear(), vptr 0x10089690, Init()), then this class's own (m_nUsed = 0,
//  vptr 0x100896c0 -- the same twelve slots again: a derived class that
//  overrides nothing -- and g_p1008ef3c = this), then the atexit
//  registration of the destructor thunk 0x1004fe40.
//
//  The queue is reserved for the sum of seven set counts of the context
//  (sub_1007b8d0() read seven times), 0x10023d70(0), 0x10050c20, the
//  five hook/pass pairs 0x1005db10, 0x10042dd0, 0x10068a50, 0x10043d70,
//  0x1003b8e0 and 0x100646d0 fill it, and then -- if anything was queued
//  -- it is sorted (0x1004fea4) and drawn in order through each entry's
//  callback, with the object filter d3dr3a.cpp's 0x1005d9c0 also uses
//  (g_1008bd54 = 0x1008b880+0x4d4).  m_nUsed = 0, 0x10064720, and
//  0x10023e50(1) close the frame.
//
//  BYTE-IDENTICAL (was 656 against 639, 122 differ).  The
//  draw loop is what closed it, three changes together:
//    * g_1008bd54 is a uint32 (as d3dqip4.cpp / d3dr5c.cpp declare it),
//      not a long: with the signed type icl sinks the filter-off arm out
//      of line; with the unsigned one it keeps the arms in source order;
//    * the filter is its own `if (g) { if (item->+0x8c & 1) continue; }`
//      ahead of the draw test, so the filter-ON arm comes first and the
//      filter-off arm loads the item behind a `jmp`, as shipped;
//    * the item is read through a pointer to the entry at every use, not
//      a pObj local, so the load is made in each arm (shipped `mov
//      (%esi,%ebx,1),%ecx` twice) rather than once before the test.
//  (The same two-`if` spelling with a pObj local and a long cost
//  0x100500a0 its match -- 430 bytes; this one keeps it identical.)
//  Written in address order, 0x1004fbc0 before 0x100500a0, as the shipped
//  TU has them.
//
//  The merged TU re-measured with this loop: 0x1004fbc0, the sort
//  0x1004fea0/0x1004fea4 and 0x100500a0 in one /O2 /Qip /Qip_no_inlining
//  file are ALL byte-identical -- but 0x10050770 (CMoArray<>::GenBegin,
//  instantiated only here) still loses its expanded GenListPos
//  constructor under /Qip_no_inlining, and at /O2 /Qip the sort is
//  inlined into 0x1004fbc0 (1104 bytes; `__declspec(noinline)` is
//  silently ignored by icl 4.0).  Not taken: the rules forbid the loss.
//
//  The queue object is a real function-local static, so the object and
//  its once-flag are the compiler's (not 0x1008ef20 / 0x1008ef38), and
//  CMoArray<W7Pending500a0>'s header members and vtables are instantiated
//  here from src/LT2/lithshared/stdlith/dynarray.h.
// ======================================================================= //
struct Counters10059590
{
	uint8	m_pad_00[0x24];                             // +0x00
	void	(*m_pfnHook24)();                           // +0x24
};

struct W7Set4fbc0                                       // count at +0x04
{
	void*	m_pArray;
	uint32	m_nCount;
};

struct W7Ctx4fbc0
{
	uint8		m_pad_000[0x164];
	W7Set4fbc0	m_Set164;                               // +0x164
	uint8		m_pad_16c[0x1a4 - 0x16c];
	W7Set4fbc0	m_Set1a4;                               // +0x1a4
	uint8		m_pad_1ac[0x204 - 0x1ac];
	W7Set4fbc0	m_Set204;                               // +0x204
	uint8		m_pad_20c[0x284 - 0x20c];
	W7Set4fbc0	m_Set284;                               // +0x284
	uint8		m_pad_28c[0x2a4 - 0x28c];
	W7Set4fbc0	m_Set2a4;                               // +0x2a4
	uint8		m_pad_2ac[0x2c4 - 0x2ac];
	W7Set4fbc0	m_Set2c4;                               // +0x2c4
	uint8		m_pad_2cc[0x304 - 0x2cc];
	W7Set4fbc0	m_Set304;                               // +0x304
};

struct W7Obj4fbc0
{
	uint8	m_pad_00[0x88];
	uint32	m_dwFlags88;                                // +0x88
	uint32	m_dwFlags8c;                                // +0x8c
};

extern Counters10059590*	g_1008cbcc;                 // 0x1008cbcc
extern unsigned long		g_1008ef80;                 // 0x1008ef80  optional hook
extern uint32			g_1008bd54;                 // 0x1008bd54

extern void  sub_10068840();                            // 0x10068840
extern void  sub_100438f0();                            // 0x100438f0
extern void  sub_1005d9c0();                            // 0x1005d9c0
extern void  sub_1003b7f0();                            // 0x1003b7f0
extern void  sub_10023eb0();                            // 0x10023eb0
extern void  sub_10043a30();                            // 0x10043a30
extern void  sub_10068890();                            // 0x10068890
extern void  sub_10023f40();                            // 0x10023f40
extern void* sub_1007b8d0();                            // 0x1007b8d0
extern void  sub_10023d70(unsigned long bAdditive);     // 0x10023d70
extern void  sub_10050c20();                            // 0x10050c20
extern void  sub_1005db10();                            // 0x1005db10
extern void  sub_10042dd0();                            // 0x10042dd0
extern void  sub_10068a50();                            // 0x10068a50
extern void  sub_10043d70();                            // 0x10043d70
extern void  sub_1003b8e0();                            // 0x1003b8e0
extern void  sub_100646d0();                            // 0x100646d0
extern void  sub_10064720();                            // 0x10064720
extern void  sub_10023e50(unsigned long bZWriteEnable); // 0x10023e50

void sub_1004fbc0()
{
	uint32 i;

	g_1008cbcc->m_pfnHook24();
	sub_10068840();
	g_1008cbcc->m_pfnHook24();
	sub_100438f0();
	g_1008cbcc->m_pfnHook24();
	sub_1005d9c0();
	g_1008cbcc->m_pfnHook24();
	sub_1003b7f0();
	sub_10023eb0();
	sub_10043a30();
	sub_10068890();
	sub_10023f40();

	if (g_1008ef80)
		((void (*)())g_1008ef80)();

	static C100500a0 s_Queue;

	s_Queue.sub_10050284(((W7Ctx4fbc0*)sub_1007b8d0())->m_Set2c4.m_nCount +
	                     ((W7Ctx4fbc0*)sub_1007b8d0())->m_Set284.m_nCount +
	                     ((W7Ctx4fbc0*)sub_1007b8d0())->m_Set2a4.m_nCount +
	                     ((W7Ctx4fbc0*)sub_1007b8d0())->m_Set204.m_nCount +
	                     ((W7Ctx4fbc0*)sub_1007b8d0())->m_Set164.m_nCount +
	                     ((W7Ctx4fbc0*)sub_1007b8d0())->m_Set304.m_nCount +
	                     ((W7Ctx4fbc0*)sub_1007b8d0())->m_Set1a4.m_nCount);

	sub_10023d70(0);
	sub_10050c20();
	g_1008cbcc->m_pfnHook24();
	sub_1005db10();
	g_1008cbcc->m_pfnHook24();
	sub_10042dd0();
	g_1008cbcc->m_pfnHook24();
	sub_10068a50();
	g_1008cbcc->m_pfnHook24();
	sub_10043d70();
	g_1008cbcc->m_pfnHook24();
	sub_1003b8e0();
	sub_100646d0();

	if (s_Queue.m_nUsed)
	{
		s_Queue.sub_1004fea4(g_1008b880);

		for (i = 0; i < s_Queue.m_nUsed; i++)
		{
			W7Pending500a0* pEntry = &s_Queue[i];

			if (g_1008bd54)
			{
				if (((W7Obj4fbc0*)pEntry->m_pItem)->m_dwFlags8c & 1)
					continue;
			}

			if ((((W7Obj4fbc0*)pEntry->m_pItem)->m_dwFlags88 & 1) ||
			    (g_1008bd54 && (((W7Obj4fbc0*)pEntry->m_pItem)->m_dwFlags88 & 0x400)))
				pEntry->m_pfnDraw(g_1008b880, pEntry->m_pItem);
		}
	}

	s_Queue.m_nUsed = 0;

	sub_10064720();
	sub_10023e50(1);
}


// ======================================================================= //
//  0x100500a0 -- 478 bytes (`ret $8`, then `mov %esi,%esi`).  Queue one
//  draw, or do it now when queueing is off:
//
//      if (!g_1008ef40) { pfnDraw(&g_1008b880, pItem); return; }
//      reuse slot m_nUsed if the array already has one, else Append
//      {pItem, pfnDraw, 0} -- dynarray.h's Insert2 at m_nElements,
//      expanded in place (the cache path, then the allocate / copy /
//      free path through g_DefAlloc = 0x10091df0);
//      m_nUsed++
// ======================================================================= //
void C100500a0::sub_100500a0(void* pItem, void (*pfnDraw)(void* pCtx, void* pItem))
{
	if (!g_1008ef40)
	{
		pfnDraw(g_1008b880, pItem);
		return;
	}

	if (GetSize() > m_nUsed)
	{
		W7Pending500a0& slot = (*this)[m_nUsed];

		slot.m_pfnDraw = pfnDraw;
		slot.m_pItem   = pItem;
	}
	else
	{
		W7Pending500a0 pending;

		pending.m_pItem   = pItem;
		pending.m_pfnDraw = pfnDraw;
		pending.m_n08     = 0;
		Append(pending);
	}

	m_nUsed++;
}
