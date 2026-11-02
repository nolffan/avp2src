// *********************************************************************** //
//
//  w10_1007a280_os.cpp -- d3d.ren functions written for the whole-DLL link,
//  /Os group (w<N>_*_os.cpp, see build/engine.sh).
//
//  0x1007a280, 0x1007def4, 0x1007e430.  Why /Os: all three end with icl's /Os padding -- a short
//  `lea 0x0(%esi),%esi` / `mov %esi,%esi` up to a 4-byte boundary and then
//  single 0x90s -- and not /O2's multi-byte `8d bc 27 ..` runs.
//
// *********************************************************************** //

#include "ltbasedefs.h"
// dynarray.h with its allocator spelled IAllocator and g_DefAlloc spelled
// by address (0x10091df0): see 0x1007e430 for why the name matters.
#define LAlloc		IAllocator
#define g_DefAlloc	g_10091df0
#include "../../LT2/lithshared/stdlith/dynarray.h"
#undef LAlloc
#undef g_DefAlloc


// ======================================================================= //
//  0x1007a280, 292 bytes.  __thiscall, no arguments; 1 caller, the
//  constructor 0x1007a0d0 (d3dr4c.cpp), right after the pixel-format
//  setup.  Builds the colour-channel scaling tables for the nine channel
//  widths 0..8 bits:
//
//    * +0x970: nine pointers into one byte block at +0x994, table k
//      holding 1<<k entries (block offsets 0, 1, 3, 7, ..., 255), each
//      entry scaling a k-bit value UP to 0..255: `i*255 / ((1<<k)-1)`
//      (0x1007a32c `div %esi`, the numerator stepping by 0xff), and 0
//      when the channel has no bits (0x1007a31d, the max == 0 test kept
//      INSIDE the loop);
//    * +0x70: nine 256-byte tables scaling 0..255 DOWN to k bits:
//      `j*((1<<k)-1) / 255` (0x1007a370 `div` by 0xff, the numerator
//      stepping by (1<<k)-1).
// ======================================================================= //
class C1007a0d0
{
public:
	void		sub_1007a280();

	uint8		m_Reserved00[0x70];                     // +0x000
	uint8		m_Down70[9][256];                       // +0x070..+0x96f
	uint8*		m_pUp970[9];                            // +0x970..+0x993
	uint8		m_UpBlock994[511];                      // +0x994..+0xb92
};

// (k is an int32.  The shipped down loop indexes
// m_Down70[k][i] afresh each pass (`imul $0x100` of k reloaded from its
// slot) where an unsigned k let icl strength-reduce it to a pointer:
// 311 -> 292 bytes, the shipped length.  Left: the shipped k lives in
// %edx through the up loop, ours in its slot.)
void C1007a0d0::sub_1007a280()
{
	int32 k;
	uint32 i, nMax;

	m_pUp970[0] = &m_UpBlock994[0];
	m_pUp970[1] = &m_UpBlock994[1];
	m_pUp970[2] = &m_UpBlock994[3];
	m_pUp970[3] = &m_UpBlock994[7];
	m_pUp970[4] = &m_UpBlock994[15];
	m_pUp970[5] = &m_UpBlock994[31];
	m_pUp970[6] = &m_UpBlock994[63];
	m_pUp970[7] = &m_UpBlock994[127];
	m_pUp970[8] = &m_UpBlock994[255];

	for (k = 0; k < 9; k++)
	{
		nMax = (1 << k) - 1;

		for (i = 0; i <= nMax; i++)
		{
			if (nMax == 0)
				m_pUp970[k][i] = 0;
			else
				m_pUp970[k][i] = (uint8)((i * 255) / nMax);
		}

		for (i = 0; i < 256; i++)
		{
			m_Down70[k][i] = (uint8)((i * nMax) / 255);
		}
	}
}


// ======================================================================= //
//  0x1007def4, 168 bytes (register entry; the /Qip stack entry 0x1007def0
//  is `mov 0x4(%esp),%eax` and the body never reads %eax).  __thiscall,
//  no arguments.  2 callers: the destructor body 0x1007dcc4
//  (d3dbatchb_os.cpp) and 0x1007e270 (d3dr5a.cpp).
//
//  Releases a pool of vertex buffers held in a CMoArray at +0x04:
//
//    * nothing at all unless +0x30 (initialised) is set;
//    * if +0x34 (locked) is set, Unlock the current buffer --
//      m_pArray[+0x18], COM slot +0x10 (IDirect3DVertexBuffer7::Unlock) --
//      and clear +0x38 (the lock pointer) and +0x34;
//    * Release (slot +0x08) every non-NULL element;
//    * Term() the array, expanded: SetSize2(0, &g_DefAlloc) ->
//      _DeleteAndDestroyArray -> BaseDelete, which is the doubled
//      `jne`/`jne` and `je` on one flags value at 0x1007df2f/0x1007df46/
//      0x1007df56 -- the header tests m_pArray three times;
//    * clear +0x1c, +0x18 and +0x30.
//
//  The free is a virtual call through a pointer to 0x10091df0 (g_DefAlloc),
//  as in 0x1007dcc4.
//
//  IDENTICAL.  The loop and Term are 0x1007e030's body
//  (d3dr5b.cpp) instruction for instruction, so they are written as it is:
//  the CMoArray as MoArray<T, DefaultCache> with GetSize()/operator[]/Term,
//  allocator IAllocator, the vtable read into a local before the Release.
//  Then: the trailing stores come out in REVERSE source order (shipped
//  +0x1c, +0x18, +0x30 from source +0x30, +0x18, +0x1c; the unlock arm's
//  +0x38, +0x34 from +0x34, +0x38), and the initialised test is an early
//  `return` -- that gives the shipped `je` to a `jmp` back to the
//  epilogue; the enclosing `if` jumps straight to it.
// ======================================================================= //
struct VBRel1007def4;
struct VBRel1007def4Vtbl
{
	void*	m_pad_00[2];                                // +0x00 .. +0x04
	uint32	(__stdcall* Release)(VBRel1007def4*);       // +0x08
	void*	m_pad_0c;                                   // +0x0c  Lock
	long	(__stdcall* Unlock)(VBRel1007def4*);        // +0x10
};
struct VBRel1007def4 { VBRel1007def4Vtbl* lpVtbl; };

// d3dr5b.cpp's teardown chain for 0x1007e030 (whose body 0x1007def4
// carries inlined): dynarray.h's Term -> SetSize2 -> _DeleteAndDestroyArray
// -> BaseDelete as a two-parameter template, allocator IAllocator, cache
// DefaultCache -- the names lay the arms out as shipped.
template<class T, class C>
struct MoArray
{
	void	Term(IAllocator* pAlloc = &g_10091df0)	{ SetSize2(0, pAlloc); }
	T&		operator[](const uint32 index) const	{ return Get(index); }
	LTBOOL	SetSize2(uint32 newSize, IAllocator* pAlloc);
	uint32	GetSize() const							{ return m_nElements; }
	T&		Get(uint32 index) const					{ return m_pArray[index]; }
	void	_DeleteAndDestroyArray(IAllocator* pAlloc, uint32 nElements);
	uint32	GetNumAllocatedElements() const			{ return m_nElements + m_Cache.GetCacheSize(); }

	void**	m_pVTable;                                  // +0x00
	T*		m_pArray;                                   // +0x04
	uint32	m_nElements;                                // +0x08
	C		m_Cache;                                    // +0x0c
};

template<class T, class C>
inline void MoArray<T,C>::_DeleteAndDestroyArray(IAllocator* pAlloc, uint32 nElements)
{
	if (m_pArray)
	{
		BaseDelete(pAlloc, m_pArray, nElements);
		m_pArray = LTNULL;
	}

	m_Cache.SetCacheSize(0);
}

template<class T, class C>
inline LTBOOL MoArray<T,C>::SetSize2(uint32 newSize, IAllocator* pAlloc)
{
	if ((newSize == m_nElements) && (newSize != 0))
	{
		return LTTRUE;
	}

	if (m_pArray)
	{
		_DeleteAndDestroyArray(pAlloc, GetNumAllocatedElements());
	}

	m_nElements = newSize;
	if (newSize > 0)
	{
		m_Cache.SetCacheSize(m_Cache.GetWantedCache());
	}

	return LTTRUE;
}

struct Obj1007dcc4
{
	void*							m_pVtbl00;          // +0x00
	MoArray<VBRel1007def4*, DefaultCache>	m_Array;    // +0x04 (array +0x08)
	uint32			m_iCur18;                           // +0x18
	uint32			m_n1c;                              // +0x1c
	uint8			m_pad_20[0x10];                     // +0x20
	uint32			m_bInit30;                          // +0x30
	uint32			m_bLocked34;                        // +0x34
	void*			m_pLock38;                          // +0x38

	void sub_1007def4();
};

void Obj1007dcc4::sub_1007def4()
{
	uint32 i;

	if (!m_bInit30)
		return;

	switch (m_bLocked34)
	{
		case 0: break;
		default:
		{
			VBRel1007def4* pVB = m_Array[m_iCur18];

			pVB->lpVtbl->Unlock(pVB);
			m_bLocked34 = 0;
			m_pLock38 = LTNULL;
		}
	}

	for (i = 0; i < m_Array.GetSize(); i++)
	{
		if (m_Array[i])
		{
			VBRel1007def4*     p     = m_Array[i];
			VBRel1007def4Vtbl* pVtbl = p->lpVtbl;

			pVtbl->Release(p);
		}
	}

	m_Array.Term();

	m_bInit30 = 0;
	m_iCur18 = 0;
	m_n1c = 0;
}


// ======================================================================= //
//  0x1007e430, 904 bytes.  __thiscall (pObject, nColor, nTris, bForce),
//  `ret $0x10`, returns 1 when a slot was assigned and 0 when not.  1
//  caller: big17c14.cpp's 0x100194a4, on the global model cache
//  0x10091d80, after 0x1007e3c0 found no existing slot for the object.
//
//  The cache is a ring of vertex-buffer slots: a CMoArray of COM buffer
//  pointers at +0x04 and, parallel to it, a CMoArray of 16-byte slot
//  records at +0x3c.  Assigning a slot for pObject:
//
//    * a pending flush runs first (0x1007e7f4, while +0x5c is set);
//    * scan the ring from +0x50 for a record whose +0x0c is 0 (free);
//      take it, Release and clear any buffer it still holds, and store
//      the object and colour in it;
//    * with no free record, fail unless bForce -- else GROW the ring by
//      one: Append a NULL buffer pointer and a new record {pObject,
//      nColor, nTris, +0x60}, both dynarray.h's Insert2 expanded in full
//      (the cached arm, the reallocating arm with its copy loops, and
//      _DeleteAndDestroyArray's triple NULL test); a failed record append
//      is ignored, as the header's Append result is;
//    * then: the ring position moves past the slot, the cache is marked
//      live, +0x1c is cleared, +0x20 takes nTris when non-zero, and the
//      slot's +0x08 takes nTris.
//
//  The ring size at +0x24 grows through `n = size + 1; if (n == 0) n =
//  size;` (0x1007e4a6..0x1007e4a9), a saturating increment.
//
//  NEAR, 10 bytes (was 788 of 883).  What closed the rest:
//    * the two arrays are dynarray.h's REAL CMoArray (Append -> Insert ->
//      Insert2 -> _AllocateTArray / _DeleteAndDestroyArray, BaseNew,
//      BaseDelete), not a hand copy: the hand copy branched straight on
//      the result where the header's inlined BOOL comes back as the
//      shipped `xor %al,%al ; inc %al` flag;
//    * included with LAlloc spelled IAllocator (see the #define at the
//      top, and 0x1007e030 in d3dr5b.cpp): the real name lays each
//      _DeleteAndDestroyArray out with the free inline, IAllocator puts
//      the cache-size store first and the free behind a `jne`, as shipped;
//    * the slot record's class name carries "Default", so icl reads its
//      constructor as cold and hosts the four zero stores of BaseNew's
//      placement-new loop out of line, as shipped;
//    * the flush is a plain `if`, called on a cold-named base
//      (DefaultFlush1007e7f4): the call goes out of line and the argument
//      loads stay at entry (the `switch` sink moved them);
//    * the flush test is +0x5c NONZERO (the earlier `!` was inverted);
//    * the ring size is read through an inline GetSize() on a subobject
//      at +0x20 (as d3dr5b.cpp's 0x1007e3c0 on this same class): that is
//      the shipped re-read of +0x24 in the saturating increment and the
//      `jne ; jmp` around the failed buffer append;
//    * +0x5c is stored before +0x1c.
//  What is left: in the record array's BaseNew the shipped code keeps the
//  element count in %esi (the `while (nElements)` test and the size)
//  and counts down a copy in %edx, zero in %esi after; ours swaps %esi
//  and %edx.  Not moved by the slot type's name (13 names), the cache's
//  class name (7), the flush base's name (10), the constructor's form
//  (initialiser list, chained or reversed stores), a constructor taking
//  the four fields, Insert in place of Append, g_DefAlloc's spelling,
//  or deleting the two functions above it from the TU.
// ======================================================================= //
struct D3DObject;
struct VB1007e430;

struct VB1007e430Vtbl
{
	long	(__stdcall* QueryInterface)(VB1007e430*, void*, void**);
	uint32	(__stdcall* AddRef)(VB1007e430*);
	uint32	(__stdcall* Release)(VB1007e430*);           // +0x08
};

struct VB1007e430
{
	VB1007e430Vtbl*	lpVtbl;
};

struct SlotDefault1007e430                                     // 0x10 bytes
{
	D3DObject*	m_pObject;                              // +0x00
	uint32		m_nColor;                               // +0x04
	uint32		m_nTris;                                // +0x08
	uint32		m_nStamp;                               // +0x0c  0 = free

	SlotDefault1007e430()
	{
		m_pObject = LTNULL;
		m_nColor = 0;
		m_nTris = 0;
		m_nStamp = 0;
	}
};

struct Cnt1007e430
{
	uint32	GetSize() const { return m_nCount; }

	uint32	m_n00;                                      // +0x00
	uint32	m_nCount;                                   // +0x04
};

// The flush 0x1007e7f4 (d3dr2a_os.cpp), declared on a base whose name icl
// reads as cold (it contains "fault"): that hosts the call past the
// epilogue, as shipped, with the `if` form keeping the argument loads at
// entry.  The `switch` sink gives the same placement but moves the loads.
struct DefaultFlush1007e7f4
{
	void	sub_1007e7f4();                             // 0x1007e7f4, flush
};

class D3DModelCache : public DefaultFlush1007e7f4
{
public:
	int32	sub_1007e430(D3DObject* pObject, uint32 nColor,
	                     uint32 nTris, uint32 bForce);

	void*						m_pVtbl00;              // +0x00
	CMoArray<VB1007e430*>		m_Buffers;              // +0x04
	uint32						m_iCur18;               // +0x18
	uint32						m_n1c;                  // +0x1c
	Cnt1007e430					m_Cnt;                  // +0x20 (+0x24 the ring size)
	uint8						m_pad_28[0x3c - 0x28];  // +0x28
	CMoArray<SlotDefault1007e430>		m_Slots;                // +0x3c
	uint32						m_iPos50;               // +0x50
	uint8						m_pad_54[0x5c - 0x54];  // +0x54
	uint32						m_bDirty5c;             // +0x5c
	uint32						m_nStamp60;             // +0x60
};

int32 D3DModelCache::sub_1007e430(D3DObject* pObject, uint32 nColor,
                                  uint32 nTris, uint32 bForce)
{
	uint32 i, n;

	if (m_bDirty5c)
		sub_1007e7f4();

	i = m_iPos50;
	do
	{
		if (m_Slots[i].m_nStamp == 0)
		{
			m_iCur18 = i;

			if (m_Buffers[i])
			{
				VB1007e430*     p     = m_Buffers[m_iCur18];
				VB1007e430Vtbl* pVtbl = p->lpVtbl;

				pVtbl->Release(p);
				m_Buffers[m_iCur18] = LTNULL;
			}

			m_Slots[i].m_pObject = pObject;
			m_Slots[i].m_nColor = nColor;
			goto UseSlot;
		}

		i++;
		if (i >= m_Cnt.m_nCount)
			i = 0;
	}
	while (i != m_iPos50);

	if (!bForce)
		return 0;

	if (!m_Buffers.Append(LTNULL))
		return 0;

	m_iCur18 = m_Cnt.GetSize();

	n = m_Cnt.GetSize() + 1;
	if (n == 0)
		n = m_Cnt.GetSize();
	m_Cnt.m_nCount = n;

	{
		SlotDefault1007e430 slot;

		slot.m_pObject = pObject;
		slot.m_nColor = nColor;
		slot.m_nTris = nTris;
		slot.m_nStamp = m_nStamp60;
		m_Slots.Append(slot);
	}

UseSlot:
	m_iPos50 = m_iCur18 + 1;
	if (m_iPos50 >= m_Cnt.m_nCount)
		m_iPos50 = 0;

	m_bDirty5c = 1;
	m_n1c = 0;
	m_Cnt.m_n00 = nTris > 0 ? nTris : m_Cnt.m_n00;
	m_Slots[m_iCur18].m_nTris = nTris;
	return 1;
}
