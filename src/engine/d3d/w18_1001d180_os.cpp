// *********************************************************************** //
//
//  MODULE:   W18_1001D180_OS (.CPP)  -- /Os (build/engine.sh w<N>_*_os.cpp)
//
//  PURPOSE:  Functions d3d.ren reaches only through a vtable, written for
//            the d3d-link: a vtable cannot be emitted until every
//            slot's target exists.  Nobody calls these by name.
//
//    0x1001d180, 0x1001d9c0, 0x1001da90 -- slots of the two GenList<WORD>
//      vtables of CMoArray<WORD> (0x10089310 slot +0x14; 0x100892e0 slots
//      +0x24 and +0x20).  The slot order is genlist.h's:
//        +00 GenBegin 0x1001d240    +04 GenIsValid 0x1001c760
//        +08 GenGetNext             +0c GenGetAt
//        +10 GenAppend              +14 GenRemoveAt
//        +18 GenRemoveAll           +1c GenGetSize 0x1001db50
//        +20 GenCopyList            +24 GenAppendList
//        +28 GenFindElement         +2c GenSetCacheSize
//      so 0x1001da90 is GenCopyList and 0x1001d9c0 GenAppendList -- the
//      d3dr4b.cpp template bodies (0x1001d080 / 0x1001cfa0) over a 2-byte T
//      (`mov %di,(%eax)` stores GenGetNext's answer).
//      0x1001d9c0 calls NiceSetSize 0x1001dc70, which is in its own file
//      (w18_1001dc70_os.cpp): defined here, icl inlines it into the caller
//      (measured, 216 bytes against 195).
//    0x10015d30 -- one of the eleven 20-byte init-hook records at
//      0x100895bc (0x100895d0's first dword; d3dr2d_os.cpp decodes the
//      table and the loop 0x10050390 that calls every hook).
//    0x10035c00 -- slot +0x08 of the DirectDraw surface wrapper's vtable
//      0x10089480 (d3dr2c.cpp's C10035bb4: +0x04 width, +0x08 height,
//      +0x0c IDirectDrawSurface7*).  Slots +0x00 and +0x1c are the /Qip
//      pair in w18_10035a70_osoiqip.cpp.
//
//  FLAGS: every one pads the /Os way (`8d 76 00` / `8b f6` to 4, then
//  0x90), and none calls a CRT intrinsic.
//
// *********************************************************************** //

#include "ltbasedefs.h"


// stdlith's g_DefAlloc, spelled as d3dr4b.cpp spells it.
struct IAllocator
{
	virtual void* Slot00(uint32 nSize);                 // vtable +0x00
	virtual void* Alloc(uint32 nSize);                  // vtable +0x04
	virtual void  Free(void* pMem);                     // vtable +0x08
};

extern IAllocator g_10091df0;                           // 0x10091df0

// genlist.h's GenListPos: two dwords (d3dr4b.cpp, d3dr5a.cpp).
struct GenListPos
{
	uint32 m_Index;                                     // +0x00
	uint32 m_SubIndex;                                  // +0x04
};


// ======================================================================= //
//  CMoArray<WORD> -- its GenList<WORD> virtuals.
// ======================================================================= //
struct GenListW
{
	virtual GenListPos GenBegin() const;                        // +0x00
	virtual int32      GenIsValid(const GenListPos& pos) const; // +0x04
	virtual uint16     GenGetNext(GenListPos& pos) const;       // +0x08
	virtual uint16     GenGetAt(GenListPos& pos) const;         // +0x0c
	virtual int32      GenAppend(uint16& toAppend);             // +0x10
	virtual void       GenRemoveAt(GenListPos pos);             // +0x14
	virtual void       GenRemoveAll();                          // +0x18
	virtual uint32     GenGetSize() const;                      // +0x1c
	virtual int32      GenCopyList(const GenListW& theList);    // +0x20
	virtual int32      GenAppendList(const GenListW& theList);  // +0x24
	virtual int32      GenFindElement(const uint16& toFind,
	                                  GenListPos& thePos) const;// +0x28
	virtual void       GenSetCacheSize(uint32 nSize);           // +0x2c
};

// d3dr4a.cpp's name for this instantiation.
struct Arr1001d
{
	void*		m_pVTable;			// +0x00
	uint16*		m_pArray;			// +0x04
	uint32		m_nElements;		// +0x08

	// --- declared only ---
	uint32		sub_1007ee60() const;                     // GetSize
	int32		sub_1001dd40(uint32 nNewSize);            // SetSize
	uint16&		sub_1001dd70(uint32 nIndex) const;        // operator[]
	void		sub_1001e070(IAllocator* pAlloc);         // Term(pAlloc)
	void		sub_1001d540(uint32 nIndex);              // Remove
	int32		sub_1001dc70(uint32 nNewSize);            // NiceSetSize, w18_1001dc70_os.cpp

	// --- written here ---
	void		sub_1001d180(GenListPos pos);             // GenRemoveAt
	int32		sub_1001da90(const GenListW& theList);    // GenCopyList
	int32		sub_1001d9c0(const GenListW& theList);    // GenAppendList
};


// ---------------------------------------------------------------------- //
//  0x1001d180 -- 28 bytes.  `virtual void GenRemoveAt(GenListPos pos)
//  {Remove(pos.m_Index);}`: two stack dwords in (`ret $0x8`, the 8-byte
//  GenListPos by value), the first forwarded to Remove (0x1001d540).
// ---------------------------------------------------------------------- //
void Arr1001d::sub_1001d180(GenListPos pos)
{
	sub_1001d540(pos.m_Index);
}


// ---------------------------------------------------------------------- //
//  0x1001da90 -- 188 bytes.  GenCopyList, dynarray.h line for line.
// ---------------------------------------------------------------------- //
int32 Arr1001d::sub_1001da90(const GenListW& theList)
{
	GenListPos pos;
	uint32     iCurOut;

	if (!sub_1001dd40(theList.GenGetSize()))
		return LTFALSE;

	iCurOut = 0;
	for (pos = theList.GenBegin(); theList.GenIsValid(pos); )
	{
		if (iCurOut >= sub_1007ee60())
		{
			sub_1001e070(&g_10091df0);
			return LTFALSE;
		}

		sub_1001dd70(iCurOut) = theList.GenGetNext(pos);
		iCurOut++;
	}

	return LTTRUE;
}


// ---------------------------------------------------------------------- //
//  0x1001d9c0 -- 195 bytes.  GenAppendList.  prevSize is read FIRST
//  (`call 0x1007ee60 ; ... mov %eax,%esi` before theList's GenGetSize).
// ---------------------------------------------------------------------- //
int32 Arr1001d::sub_1001d9c0(const GenListW& theList)
{
	GenListPos pos;
	uint32     iCurOut, prevSize;

	prevSize = sub_1007ee60();

	if (!sub_1001dc70(prevSize + theList.GenGetSize()))
		return LTFALSE;

	iCurOut = prevSize;
	for (pos = theList.GenBegin(); theList.GenIsValid(pos); )
	{
		if (iCurOut >= sub_1007ee60())
		{
			sub_1001e070(&g_10091df0);
			return LTFALSE;
		}

		sub_1001dd70(iCurOut) = theList.GenGetNext(pos);
		iCurOut++;
	}

	return LTTRUE;
}


// ======================================================================= //
//  0x10015d30 -- 33 bytes.  Init hook: tie off two global list heads.
//
//      mov $0x1008b084,%eax ; mov $0x1008b078,%edx
//      mov %eax,0x1008b084 ; mov %eax,0x1008b088     b084.prev = next = &b084
//      mov %edx,0x1008b07c ; mov %edx,0x1008b078     b078.next = prev = &b078
//
//  The same pair d3dr5c.cpp's 0x1001c1d0 ties off; the record's slot is a
//  plain `void (*)()` (the caller pushes nothing and cleans nothing).
// ======================================================================= //
extern LTLink  g_1008b078;                              // 0x1008b078
extern LTLink  g_1008b084;                              // 0x1008b084

void sub_10015d30()
{
	g_1008b084.TieOff();
	g_1008b078.TieOff();
}


// ======================================================================= //
//  The surface wrapper, vtable 0x10089480.
// ======================================================================= //
struct C10035a70
{
	void*  m_pVTable;                                   // +0x00
	uint32 m_nWidth;                                    // +0x04
	uint32 m_nHeight;                                   // +0x08
	void*  m_pSurface;                                  // +0x0c

	void   sub_10035c00(uint32* pWidth, uint32* pHeight); // slot +0x08
};

// ---------------------------------------------------------------------- //
//  0x10035c00 -- 37 bytes.  GetSize(&w, &h): +0x04 into the first
//  pointer, +0x08 into the second, `ret $0x8`.
// ---------------------------------------------------------------------- //
void C10035a70::sub_10035c00(uint32* pWidth, uint32* pHeight)
{
	*pWidth  = m_nWidth;
	*pHeight = m_nHeight;
}
