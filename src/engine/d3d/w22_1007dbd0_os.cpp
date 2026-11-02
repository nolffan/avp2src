// *********************************************************************** //
//
//  w22_1007dbd0_os.cpp -- d3d.ren functions reached only through vtables,
//  written for the whole-DLL link.  /Os group (w<N>_*_os.cpp, see
//  build/engine.sh).
//
//  0x1007dbd0          slot 0 of the vertex-buffer pool's base vtable
//                      0x10089a20: the scalar deleting destructor.
//  0x1007f5a0,         slots of the CMoArray<void*> vtable 0x10089a60
//  0x1007f730..f790    (genlist.h order: +0x0c GenGetAt, +0x10 GenAppend,
//                      +0x14 GenRemoveAt, +0x18 GenRemoveAll, +0x24
//                      GenAppendList).  0x1007f790 is ALSO slot +0x0c of
//                      the other pointer-array vtable 0x100899f0: one
//                      body, folded.
//  0x1007fa20          CMoArray<void*>::Append, the callee of GenAppend
//                      (not a table entry; written so 0x1007f770 links).
//
//  Why /Os: the siblings of every one of these in the same vtables --
//  0x1007f670 (GenCopyList), 0x1007c670/0x1007c6b0/0x1007c690 and the
//  deleting destructor 0x1007dc00 -- are byte-identical at /Os in
//  d3dr4b.cpp / d3dr4a.cpp / d3dr4c.cpp, and 0x1007dbd0's delete arm
//  jumps back into the one epilogue (the /Os shape; /O2 duplicates it,
//  see d3dr4b_o2.cpp).
//
// *********************************************************************** //

#include "ltbasedefs.h"


// ---------------------------------------------------------------------- //
//  0x10091df0 -- stdlith's g_DefAlloc, declared as d3dr4b.cpp declares it.
// ---------------------------------------------------------------------- //
struct IAllocator
{
	virtual void* Slot00(uint32 nSize);                 // vtable +0x00
	virtual void* Alloc(uint32 nSize);                  // vtable +0x04
	virtual void  Free(void* pMem);                     // vtable +0x08
};

extern IAllocator g_10091df0;                           // 0x10091df0


// genlist.h's GenListPos (two dwords, POD), as d3dr4b.cpp declares it.
struct GenListPos
{
	uint32 m_Index;                                     // +0x00
	uint32 m_SubIndex;                                  // +0x04
};


// ======================================================================= //
//  0x1007dbd0 -- 43 bytes.  Slot 0 of 0x10089a20, `ret $0x4`.
//
//      55 8b ec / 83 ec 10 / 57 / 53      frame, dead slot, %ebx saved
//      8b d9 / 8b cb / e8 <0x1007dcc4>    the destructor body
//      8b 45 08 / a8 01 / 75 07           test $0x1 on the flags
//      5b / 8b e5 / 5d / c2 04 00         ret $0x4
//      57 / 53 / e8 <0x10080479>          operator delete(this)
//      83 c4 08 / eb ed                   back into the epilogue
//
//  The base-class twin of d3dr4c.cpp's 0x1007dc00 (slot 0 of the derived
//  table at 0x10089a40), minus the member destructor at +0x3c: run the
//  destructor body 0x1007dcc4 (d3dbatchb_os.cpp), then honour the delete
//  flag.
// ======================================================================= //
struct Obj1007dcc4
{
	void	sub_1007dcc4();                             // 0x1007dcc4
	void	sub_1007dbd0(uint32 nFlags);
};

void Obj1007dcc4::sub_1007dbd0(uint32 nFlags)
{
	sub_1007dcc4();

	if ((nFlags & 1) == 0)
		return;

	::operator delete(this);
}


// ======================================================================= //
//  CMoArray<void*> -- the array whose GenList vtable is 0x10089a60.
//
//  Its members, by the callees these slots reach:
//      0x1007ee60 GetSize, 0x1007f7e0 NiceSetSize, 0x1007c870 operator[],
//      0x1007fa50 Term(LAlloc*), 0x1007f9f0 Remove, 0x10009d40 Get,
//      0x10009ec0 Insert(index, toAdd) -- the wrapper that appends
//      &g_DefAlloc.
//  ArrP is the spelling d3dr4b.cpp gives this instantiation (its
//  0x1007f670 is slot +0x20, GenCopyList, of the same table), so the
//  callee spellings resolve through the aliases already in
//  link/aliases.rsp.
// ======================================================================= //
struct GenListP
{
	virtual GenListPos GenBegin() const;
	virtual int32      GenIsValid(const GenListPos& pos) const;
	virtual void*      GenGetNext(GenListPos& pos) const;
	virtual void*      GenGetAt(GenListPos& pos) const;
	virtual int32      GenAppend(void*& toAppend);
	virtual void       GenRemoveAt(GenListPos pos);
	virtual void       GenRemoveAll();
	virtual uint32     GenGetSize() const;
	virtual int32      GenCopyList(const GenListP& theList);
	virtual int32      GenAppendList(const GenListP& theList);
	virtual int32      GenFindElement(void* const& toFind,
	                                  GenListPos& thePos) const;
	virtual void       GenSetCacheSize(uint32 nSize);
};

struct ArrP
{
	uint32 sub_1007ee60() const;                        // GetSize
	int32  sub_1007f7e0(uint32 nNewSize);               // NiceSetSize
	void*& sub_1007c870(uint32 nIndex) const;           // operator[]
	void   sub_1007fa50(IAllocator* pAlloc = &g_10091df0);      // Term

	void   sub_1007f730();                              // GenRemoveAll
	int32  sub_1007f5a0(const GenListP& theList);       // GenAppendList
};


// ---------------------------------------------------------------------- //
//  0x1007f5a0 -- 194 bytes.  Slot +0x24, GenAppendList, `ret $0x4`.
//
//  dynarray.h's GenAppendList verbatim: GetSize (0x1007ee60) FIRST into
//  %esi, then theList.GenGetSize() (+0x1c) added to it for NiceSetSize
//  (0x1007f7e0); `pos = theList.GenBegin()` is the `rep movsl` with
//  %ecx = 2 (GenListPos is POD); the loop's `iCurOut >= GetSize()` arm
//  plants &g_DefAlloc and calls Term (0x1007fa50), and the store is
//  `mov %edi,(%eax)` into operator[] (0x1007c870) AFTER GenGetNext --
//  a pointer T comes back in %eax.  The twin of d3dr4b.cpp's 0x1007f670
//  (GenCopyList) and of d3dr5c_o2.cpp's 0x1007c480 (the other pointer
//  array's GenAppendList).
// ---------------------------------------------------------------------- //
int32 ArrP::sub_1007f5a0(const GenListP& theList)
{
	GenListPos pos;
	uint32     iCurOut, prevSize;

	prevSize = sub_1007ee60();

	if (!sub_1007f7e0(prevSize + theList.GenGetSize()))
		return LTFALSE;

	iCurOut = prevSize;
	for (pos = theList.GenBegin(); theList.GenIsValid(pos); )
	{
		// BAD bug in theList!
		if (iCurOut >= sub_1007ee60())
		{
			sub_1007fa50();
			return LTFALSE;
		}

		sub_1007c870(iCurOut) = theList.GenGetNext(pos);
		iCurOut++;
	}

	return LTTRUE;
}


// ---------------------------------------------------------------------- //
//  0x1007f730 -- 27 bytes.  Slot +0x18, GenRemoveAll, bare `ret`.
//
//      virtual void GenRemoveAll() { Term(); }
//
//  The one dword planted is 0x10091df0: Term's default argument
//  materialised by the caller (d3dr4b.cpp FAMILY D, 0x1007c670).
// ---------------------------------------------------------------------- //
void ArrP::sub_1007f730()
{
	sub_1007fa50();
}


// ---------------------------------------------------------------------- //
//  0x1007f750 -- 28 bytes.  Slot +0x14, GenRemoveAt, `ret $0x8`.
//
//      virtual void GenRemoveAt(GenListPos pos) { Remove(pos.m_Index); }
//
//  GenListPos by value (the `ret $0x8`), only its first dword read, and
//  forwarded to Remove 0x1007f9f0 (`Remove2(index, &g_DefAlloc)`,
//  d3dr2b.cpp).  Written on d3dr2b.cpp's class so the callee's spelling
//  is its definition's.
// ---------------------------------------------------------------------- //
struct Arr1007f9f0
{
	void sub_1007f9f0(uint32 nIndex);                   // Remove
	void sub_1007f750(GenListPos pos);                  // GenRemoveAt
};

void Arr1007f9f0::sub_1007f750(GenListPos pos)
{
	sub_1007f9f0(pos.m_Index);
}


// ---------------------------------------------------------------------- //
//  0x1007fa20 -- 37 bytes.  Append, not a table entry: the callee of
//  GenAppend 0x1007f770.
//
//      BOOL Append(const T &toAdd) { return Insert(m_nElements, toAdd); }
//
//  d3dr4b.cpp's FAMILY A shape exactly (`mov 0x8(%eax),%edx` is
//  m_nElements read directly), over the Insert wrapper at 0x10009ec0
//  (d3dr2c.cpp).
// ---------------------------------------------------------------------- //
struct Arr1007fa20
{
	uint8  m_pad_00[0x08];                              // vptr, m_pArray
	uint32 m_nElements;                                 // +0x08

	int32  sub_10009ec0(uint32 nIndex, const void* pToInsert);   // Insert
	int32  sub_1007fa20(const void* pToAdd);                     // Append
	int32  sub_1007f770(void* pToAppend);                        // GenAppend
};

int32 Arr1007fa20::sub_1007fa20(const void* pToAdd)
{
	return sub_10009ec0(m_nElements, pToAdd);
}


// ---------------------------------------------------------------------- //
//  0x1007f770 -- 28 bytes.  Slot +0x10, GenAppend, `ret $0x4`.
//
//      virtual BOOL GenAppend(T &toAppend) { return Append(toAppend); }
//
//  The reference is forwarded as one dword to Append (0x1007fa20 above);
//  d3dr4b.cpp FAMILY B (0x1007c6b0) is the other pointer array's copy.
// ---------------------------------------------------------------------- //
int32 Arr1007fa20::sub_1007f770(void* pToAppend)
{
	return sub_1007fa20(pToAppend);
}


// ---------------------------------------------------------------------- //
//  0x1007f790 -- 30 bytes.  Slot +0x0c, GenGetAt, `ret $0x4`, in BOTH
//  pointer-array vtables (0x10089a60 and 0x100899f0).
//
//      virtual T GenGetAt(GenListPos &pos) const { return Get(pos.m_Index); }
//
//  `mov 0x8(%ebp),%eax ; ... mov (%eax),%eax` plants pos.m_Index, calls
//  Get (0x10009d40, `&m_pArray[i]` over dwords, d3dbatcha.cpp) and loads
//  the element through the returned address.
// ---------------------------------------------------------------------- //
struct C10009d40
{
	uint32* sub_10009d40(uint32 nIndex);                // Get
	void*   sub_1007f790(GenListPos& pos);              // GenGetAt
};

void* C10009d40::sub_1007f790(GenListPos& pos)
{
	return (void*)*sub_10009d40(pos.m_Index);
}
