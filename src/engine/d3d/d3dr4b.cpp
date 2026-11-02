// *********************************************************************** //
//
//  MODULE:   SMALL_B (.CPP)  --  d3d.ren "small_b"
//
//  PURPOSE:  35 of the 40 rows of small_b.txt, decoded instruction by
//            instruction from the retail d3d.ren.  Intel C/C++ 4.0, /O2 or /Os.
//
//  WHAT THIS BATCH IS.  Nineteen of the forty rows are ONE family and the
//  family already has a name in this repo: they are the out-of-line members
//  of src/LT2/lithshared/stdlith/dynarray.h's `CMoArray<T,C>` and of the
//  `GenList<T>` interface it implements (src/LT2/lithshared/stdlith/genlist.h),
//  instantiated over five different element types.  The proof is not a
//  shape argument -- it is the VTABLES.  Four twelve-entry pointer runs sit
//  in .data at 0x10089260, 0x10089690, 0x100897a0 and 0x100899f0, and every
//  one of them is genlist.h's virtual list in declaration order:
//
//      +0x00 GenBegin        +0x18 GenRemoveAll
//      +0x04 GenIsValid      +0x1c GenGetSize
//      +0x08 GenGetNext      +0x20 GenCopyList
//      +0x0c GenGetAt        +0x24 GenAppendList
//      +0x10 GenAppend       +0x28 GenFindElement
//      +0x14 GenRemoveAt     +0x2c GenSetCacheSize
//
//  Three of those slots -- GenBegin (0x1001d240), GenIsValid (0x1001c760),
//  GenGetSize (0x1001db50) and GenSetCacheSize (0x1001cf40) -- are the SAME
//  address in several of the runs, which is what you would expect of a
//  template member that does not depend on T, and is why the layout reading
//  can be trusted.  Every listed row that lands in one of those runs is
//  therefore named by its slot, and the body then reads back exactly as the
//  header's one-liner:
//
//      row         slot           header text
//      0x1001c6c0  +0x10 (4)      GenAppend(T&)      { return Append(x); }
//      0x1001c6e0  +0x0c (3)      GenGetAt(pos)      { return Get(pos.m_Index); }
//      0x1001cfa0  +0x24 (9)      GenAppendList(list)
//      0x1001d080  +0x20 (8)      GenCopyList(list)
//      0x1001db70  +0x18 (6)      GenRemoveAll()     { Term(); }
//      0x1001db90  +0x14 (5)      GenRemoveAt(pos)   { Remove(pos.m_Index); }
//      0x100506b0  +0x14 (5)      GenRemoveAt
//      0x100506f0  +0x0c (3)      GenGetAt
//      0x10050670  +0x1c (7)      GenGetSize()       { return GetSize(); }
//      0x1005a2d0  +0x14 (5)      GenRemoveAt
//      0x1007c670  +0x18 (6)      GenRemoveAll
//      0x1007c6b0  +0x10 (4)      GenAppend
//      0x1007ec20  +0x24 (9)      GenAppendList
//      0x1007f670  +0x20 (8)      GenCopyList
//
//  and the six that are NOT in a vtable are the non-virtual members those
//  fourteen call:
//
//      0x1001cce0 / 0x1001e040 / 0x1007ca00   Append(const T&)
//                                    { return Insert(m_nElements, toAdd); }
//      0x1001dc50   SetCacheSize(DWORD)  { m_Cache.SetWantedCache(size); }
//      0x1001dd70   operator[](DWORD) const { return Get(index); }
//
//  The class layout is nailed by the callees, not guessed:
//    +0x00 vptr, +0x04 m_pArray, +0x08 m_nElements, +0x0c m_Cache.
//    * 0x1007ee60 and 0x1005f780 are `GetSize` -- `mov 0x8(%ecx),%eax`.
//    * 0x1001cce0 loads `0x8(%eax)` and passes it as Insert's index, which
//      is `Insert(m_nElements, toAdd)` verbatim.
//    * 0x1001dc50 forms `this+0x0c` and calls 0x1000a660, which is
//      `mov 0x8(%esp),%eax ; mov %eax,0x4(%ecx)` -- DefaultCache's
//      SetWantedCache writing m_WantedCache at +0x04 of the two-dword cache.
//    * 0x1001dc30 is `Get` for a WORD array (`lea (%ecx,%eax,2)`),
//      0x1001c790 for a 64-byte T (`shl $0x6`), 0x10050790 for a 12-byte T
//      (`lea (,%eax,8)` + `lea (%edx,%eax,4)`).
//    * 0x1001d7e0 / 0x1001e070 / 0x1007ca30 are `Term(LAlloc*)`, defined
//      out of line and reached through the header's DEFAULT ARGUMENT --
//      each caller plants 0x10091df0 (g_DefAlloc) itself.
//
//  THE ELEMENT SIZE OF EACH INSTANTIATION IS READ OFF THE COPY, and icl is
//  consistent about which copy it uses:
//    * struct ASSIGNMENT is always `rep movsl` -- ecx=8 at 0x1001d080 (T is
//      32 bytes), ecx=4 at 0x1007ec20 (16), ecx=2 for the 8-byte GenListPos;
//    * a RETURN-VALUE copy is field-by-field below the threshold and
//      `rep movsl` above -- 0x100506f0 emits three movs for a 12-byte T,
//      0x1001c6e0 emits `rep movsl` ecx=0x10 for a 64-byte one.
//  0x1007f670's T is a bare pointer: GenGetNext comes back in %eax and the
//  store is a single `mov %edi,(%eax)`.
//
//  FIVE ROWS ARE NOT WRITTEN, and four of them are not functions:
//
//    0x1005053c  is the out-of-line failure tail of the function at
//                0x10050450 -- `push %edi ; mov -0x8(%ebp),%ecx` with %ebp
//                already set up by that function's prologue.  0x100504da
//                (`jae 0x1005053c`) is the only way in.  It is that
//                function's `{ Term(); return FALSE; }` arm.
//    0x1007c644  the same thing for the function at 0x1007f5a0's twin at
//                0x1007c580: reached only by `jae 0x1007c644` at 0x1007c5fa.
//    0x1006d500  (since written: w57_1006d500_os.cpp, the initialiser of a
//                file-scope object; the call is a relocation either way.)
//                A REGISTER-ABI CALLER.  It calls 0x1006d3a4, and
//                0x1006d3a0 is the matching stack shim (`mov 0x4(%esp),%eax`)
//                that falls through into it, so the call is to icl's
//                register entry -- and %eax is never even set, so the
//                argument is passed as garbage.  Blocked.
//    0x1007e430  is NOT 35 bytes.  Its first `ret $0x10` is at 0x1007e48a
//                but the body runs to 0x1007e7c0 -- 912 bytes, with a
//                three-deep loop nest.  Wrong size band for this file.
//
// *********************************************************************** //

#include "ltbasedefs.h"


// ======================================================================= //
//  SHARED DECLARATIONS
// ======================================================================= //

// ---------------------------------------------------------------------- //
//  0x10091df0 -- stdlith's `g_DefAlloc` (l_allocator.h).  Declared exactly
//  as d3dbatchb.cpp / d3dr2b.cpp declare it, and never defined, so no
//  vtable for it is emitted here.
// ---------------------------------------------------------------------- //
struct IAllocator
{
	virtual void* Slot00(uint32 nSize);                 // vtable +0x00
	virtual void* Alloc(uint32 nSize);                  // vtable +0x04
	virtual void  Free(void* pMem);                     // vtable +0x08
};

extern IAllocator g_10091df0;


// ---------------------------------------------------------------------- //
//  genlist.h's GenListPos.  Two dwords, and PLAIN OLD DATA: the assignment
//  `pos = theList.GenBegin()` in GenCopyList compiles to `rep movsl` with
//  %ecx = 2, not to a call, so whatever operator= the header we have shows
//  is not in the shipped one.
// ---------------------------------------------------------------------- //
struct GenListPos
{
	uint32 m_Index;                                     // +0x00
	uint32 m_SubIndex;                                  // +0x04
};


// ---------------------------------------------------------------------- //
//  DefaultCache -- two dwords, m_CacheSize at +0x00 and m_WantedCache at
//  +0x04.  Only the setter is needed here; it is 0x1000a660, already
//  reconstructed by an earlier batch, so it is declared and not defined.
// ---------------------------------------------------------------------- //
struct DefaultCache
{
	void sub_1000a660(uint32 val);                      // SetWantedCache
};


// ======================================================================= //
//  FAMILY A -- CMoArray<T,C>::Append(const T &toAdd)
//
//      BOOL Append(const T &toAdd) { return Insert(m_nElements, toAdd); }
//
//  Thirty-seven bytes, identical in all three but the call displacement:
//
//      55 8b ec / 83 ec 10          frame
//      8b c1                        %eax = this
//      83 c4 f8                     an 8-byte outgoing area
//      8b 50 08                     %edx = this->m_nElements
//      8b 4d 08                     %ecx = toAdd
//      89 14 24 / 89 4c 24 04       plant them
//      8b c8                        this back into %ecx
//      e8 <Insert>                  call
//      8b e5 / 5d / c2 04 00        ret $0x4
//
//  WHAT PINS IT: the `mov 0x8(%eax),%edx` is m_nElements read DIRECTLY --
//  not through GetSize() -- which is what the header's Append says, and the
//  callee is in every case the two-argument `Insert` wrapper that appends
//  &g_DefAlloc (0x1001ccb0, 0x1001e010, 0x10021bf0 all have that shape).
// ======================================================================= //

// ---- 0x1001cce0 -- T is 64 bytes (its Get, 0x1001c790, is `shl $0x6`).
struct Arr1001cce0
{
	uint8  m_pad_00[0x08];                              // vptr, m_pArray
	uint32 m_nElements;                                 // +0x08

	int32  sub_1001ccb0(uint32 nIndex, const void* pToInsert);   // Insert
	int32  sub_1001cce0(const void* pToAdd);                     // Append
};

int32 Arr1001cce0::sub_1001cce0(const void* pToAdd)
{
	return sub_1001ccb0(m_nElements, pToAdd);
}


// ---- 0x1001e040 -- same wrapper over the Insert at 0x1001e010.
struct Arr1001e040
{
	uint8  m_pad_00[0x08];
	uint32 m_nElements;                                 // +0x08

	int32  sub_1001e010(uint32 nIndex, const void* pToInsert);
	int32  sub_1001e040(const void* pToAdd);
};

int32 Arr1001e040::sub_1001e040(const void* pToAdd)
{
	return sub_1001e010(m_nElements, pToAdd);
}


// ---- 0x1007ca00 -- same wrapper over the Insert at 0x10021bf0.
struct Arr1007ca00
{
	uint8  m_pad_00[0x08];
	uint32 m_nElements;                                 // +0x08

	int32  sub_10021bf0(uint32 nIndex, const void* pToInsert);
	int32  sub_1007ca00(const void* pToAdd);
};

int32 Arr1007ca00::sub_1007ca00(const void* pToAdd)
{
	return sub_10021bf0(m_nElements, pToAdd);
}


// ======================================================================= //
//  FAMILY B -- GenList<T>::GenAppend, slot +0x10
//
//      virtual BOOL GenAppend(T &toAppend) { return Append(toAppend); }
//
//  Twenty-eight bytes.  The reference is forwarded as one dword and the
//  callee is the Append above, which is what makes the slot reading and the
//  Append reading corroborate each other.
// ======================================================================= //

// ---- 0x1001c6c0 -> Append 0x1001cce0 (vtable 0x10089260 group 3, slot 4).
struct Arr1001c6c0
{
	int32 sub_1001cce0(const void* pToAdd);             // Append
	int32 sub_1001c6c0(void* pToAppend);                // GenAppend
};

int32 Arr1001c6c0::sub_1001c6c0(void* pToAppend)
{
	return sub_1001cce0(pToAppend);
}


// ---- 0x1007c6b0 -> Append 0x1007ca00 (vtable 0x100899f0 group 1, slot 4).
struct Arr1007c6b0
{
	int32 sub_1007ca00(const void* pToAdd);
	int32 sub_1007c6b0(void* pToAppend);
};

int32 Arr1007c6b0::sub_1007c6b0(void* pToAppend)
{
	return sub_1007ca00(pToAppend);
}


// ======================================================================= //
//  FAMILY C -- GenList<T>::GenRemoveAt, slot +0x14
//
//      virtual void GenRemoveAt(GenListPos pos) { Remove(pos.m_Index); }
//
//  Twenty-eight bytes and `ret $0x8`.  The `ret $0x8` with only 0x8(%ebp)
//  ever read is the pin: GenListPos goes by VALUE (two dwords) and only its
//  first is used.  The callee in each case is the one-argument `Remove`
//  wrapper that appends &g_DefAlloc (0x1001deb0, 0x1002e9d0, 0x1000a3c0 --
//  all three are `f(n) { g(n, &g_DefAlloc); }`).
// ======================================================================= //

// ---- 0x1001db90 -> Remove 0x1001deb0.
struct Arr1001db90
{
	void sub_1001deb0(uint32 nIndex);                   // Remove
	void sub_1001db90(GenListPos pos);                  // GenRemoveAt
};

void Arr1001db90::sub_1001db90(GenListPos pos)
{
	sub_1001deb0(pos.m_Index);
}


// ---- 0x100506b0 -> Remove 0x1002e9d0.
struct Arr100506b0
{
	void sub_1002e9d0(uint32 nIndex);
	void sub_100506b0(GenListPos pos);
};

void Arr100506b0::sub_100506b0(GenListPos pos)
{
	sub_1002e9d0(pos.m_Index);
}


// ---- 0x1005a2d0 -> Remove 0x1000a3c0.
struct Arr1005a2d0
{
	void sub_1000a3c0(uint32 nIndex);
	void sub_1005a2d0(GenListPos pos);
};

void Arr1005a2d0::sub_1005a2d0(GenListPos pos)
{
	sub_1000a3c0(pos.m_Index);
}


// ======================================================================= //
//  FAMILY D -- GenList<T>::GenRemoveAll, slot +0x18
//
//      virtual void GenRemoveAll() { Term(); }
//
//  Twenty-seven bytes and a bare `ret`: no argument of its own, and the one
//  dword it plants is 0x10091df0.  That is the header's
//  `Term(LAlloc *pAlloc = &g_DefAlloc)` default argument being materialised
//  by the caller, with the body itself living out of line at 0x1001e070 /
//  0x1007ca30 (both `SetSize2(0, pAlloc)`).
// ======================================================================= //

// ---- 0x1001db70 -> Term 0x1001e070.
struct Arr1001db70
{
	void sub_1001e070(IAllocator* pAlloc = &g_10091df0);    // Term
	void sub_1001db70();                                    // GenRemoveAll
};

void Arr1001db70::sub_1001db70()
{
	sub_1001e070();
}


// ---- 0x1007c670 -> Term 0x1007ca30.
struct Arr1007c670
{
	void sub_1007ca30(IAllocator* pAlloc = &g_10091df0);
	void sub_1007c670();
};

void Arr1007c670::sub_1007c670()
{
	sub_1007ca30();
}


// ======================================================================= //
//  0x10050670 -- GenList<T>::GenGetSize, slot +0x1c
//
//      virtual DWORD GenGetSize() const { return GetSize(); }
//
//      55 8b ec / 83 ec 08 / 8b c1 / 8b c8 / e8 <0x1005f780> / 8b e5 5d c3
//
//  Nineteen bytes.  0x1005f780 is `push %esi ; mov 0x8(%ecx),%eax ;
//  add $0x4,%esp ; ret` -- GetSize reading m_nElements at +0x08, out of
//  line.  There is no argument at all, hence the bare `ret`.
// ======================================================================= //
struct Arr10050670
{
	uint32 sub_1005f780() const;                        // GetSize
	uint32 sub_10050670() const;                        // GenGetSize
};

uint32 Arr10050670::sub_10050670() const
{
	return sub_1005f780();
}


// ======================================================================= //
//  FAMILY E -- GenList<T>::GenGetAt, slot +0x0c
//
//      virtual T GenGetAt(GenListPos &pos) const { return Get(pos.m_Index); }
//
//  `ret $0x8` = the hidden return pointer at 0x8(%ebp) plus `pos` by
//  reference at 0xc(%ebp); the `mov (%eax),%eax` off that reference is
//  pos.m_Index.  The two rows differ only in sizeof(T), and that difference
//  is visible: 64 bytes copies with `rep movsl` (%ecx = 0x10), 12 bytes
//  copies with three movs.
// ======================================================================= //

// ---- 0x1001c6e0 -- T is 64 bytes; Get is 0x1001c790 (`shl $0x6`).
struct T1001c6e0 { uint32 m_v[16]; };

struct Arr1001c6e0
{
	T1001c6e0& sub_1001c790(uint32 nIndex) const;       // Get
	T1001c6e0  sub_1001c6e0(GenListPos& pos) const;     // GenGetAt
};

T1001c6e0 Arr1001c6e0::sub_1001c6e0(GenListPos& pos) const
{
	return sub_1001c790(pos.m_Index);
}


// ---- 0x100506f0 -> d3dr4b_o2.cpp (it needs /O2; see there).


// ======================================================================= //
//  0x1001dc50 -- CMoArray<T,C>::SetCacheSize(DWORD size)
//
//      void SetCacheSize(DWORD size) { m_Cache.SetWantedCache(size); }
//
//      55 8b ec / 83 ec 10 / 8b c1 / 57 / 83 c0 0c   %eax = this + 0x0c
//      8b 55 08 / 8b c8 / 89 14 24 / e8 <0x1000a660> / 8b e5 5d c2 04 00
//
//  WHAT PINS IT: `this + 0x0c` is m_Cache, and 0x1000a660 writes +0x04 of
//  whatever it is handed -- DefaultCache::m_WantedCache.  It is shared by
//  every instantiation (0x1001cf40, the GenSetCacheSize that calls it, is
//  the same address in all four vtables), which is exactly right for a
//  member whose offsets do not depend on T.
// ======================================================================= //
struct Arr1001dc50
{
	uint8        m_pad_00[0x0c];                        // vptr/m_pArray/m_nElements
	DefaultCache m_Cache;                               // +0x0c

	void sub_1001dc50(uint32 nSize);
};

void Arr1001dc50::sub_1001dc50(uint32 nSize)
{
	m_Cache.sub_1000a660(nSize);
}


// ======================================================================= //
//  0x1001dd70 -- CMoArray<WORD,C>::operator[](DWORD) const
//
//      T& operator[](const DWORD index) const { return Get(index); }
//
//  Twenty-eight bytes, the plain one-argument forwarder.  The element type
//  comes from the callee: 0x1001dc30 is
//      push %esi ; mov 0x4(%ecx),%ecx ; mov 0x8(%esp),%eax ;
//      lea (%ecx,%eax,2),%eax ; add $0x4,%esp ; ret $0x4
//  -- `&m_pArray[index]` with a stride of TWO, so T is a WORD.
// ======================================================================= //
struct Arr1001dd70
{
	uint16& sub_1001dc30(uint32 nIndex) const;          // Get
	uint16& sub_1001dd70(uint32 nIndex) const;          // operator[]
};

uint16& Arr1001dd70::sub_1001dd70(uint32 nIndex) const
{
	return sub_1001dc30(nIndex);
}


// ======================================================================= //
//  THE TWO BIG TEMPLATE MEMBERS
//
//  0x1001cfa0 / 0x1007ec20 are GenAppendList and 0x1001d080 / 0x1007f670
//  are GenCopyList, both straight out of dynarray.h.  They are the reason
//  the four rows were mis-sized in the listing: the "47" and "38" are the
//  distance to the shared `return FALSE` exit, not to the end of the body.
//  0x1001cfa0 runs to 0x1001d080 (222 bytes), 0x1001d080 to 0x1001d160
//  (215), 0x1007ec20 to 0x1007ed00 (222), 0x1007f670 to 0x1007f730 (187).
//
//  Everything in them is the header, line for line:
//
//      virtual BOOL GenCopyList(const GenList<T> &theList)
//      {
//          GenListPos pos;  DWORD iCurOut;
//          if(!SetSize(theList.GenGetSize())) return FALSE;
//          iCurOut = 0;
//          for(pos=theList.GenBegin(); theList.GenIsValid(pos); )
//          {
//              if(iCurOut >= GetSize()) { Term(); return FALSE; }
//              (*this)[iCurOut] = theList.GenGetNext(pos);
//              iCurOut++;
//          }
//          return TRUE;
//      }
//
//  and GenAppendList is the same with `prevSize = GetSize()` up front,
//  `NiceSetSize(prevSize + theList.GenGetSize())` for the resize and
//  `iCurOut = prevSize` for the start.  The `mov %eax,%esi` right after the
//  first `call 0x1007ee60` and before theList's own GenGetSize is what says
//  prevSize is taken FIRST, in source order.
//
//  THE VIRTUAL CALLS ARE THE C++ KIND, not the function-pointer-struct
//  kind: `mov (%ebx),%eax ; mov %ebx,%ecx ; mov 0x1c(%eax),%eax ;
//  call *%eax`, three instructions with `this` loaded separately.  So
//  GenList<T> below is a real class with real virtual members, declared in
//  genlist.h's order so that GenBegin lands on +0x00, GenIsValid on +0x04,
//  GenGetNext on +0x08 and GenGetSize on +0x1c -- the four slots these
//  bodies reach through.  Nothing here constructs one, so no vtable for it
//  is emitted.
// ======================================================================= //

// ---------------------------------------------------------------------- //
//  Instantiation 1 -- T is 32 bytes.  vtable 0x10089260 group 2:
//  GenCopyList = 0x1001d080, GenAppendList = 0x1001cfa0.
// ---------------------------------------------------------------------- //
struct T32 { uint32 m_v[8]; };

struct GenList32
{
	virtual GenListPos GenBegin() const;                        // +0x00
	virtual int32      GenIsValid(const GenListPos& pos) const; // +0x04
	virtual T32        GenGetNext(GenListPos& pos) const;       // +0x08
	virtual T32        GenGetAt(GenListPos& pos) const;         // +0x0c
	virtual int32      GenAppend(T32& toAppend);                // +0x10
	virtual void       GenRemoveAt(GenListPos pos);             // +0x14
	virtual void       GenRemoveAll();                          // +0x18
	virtual uint32     GenGetSize() const;                      // +0x1c
	virtual int32      GenCopyList(const GenList32& theList);   // +0x20
	virtual int32      GenAppendList(const GenList32& theList); // +0x24
	virtual int32      GenFindElement(const T32& toFind,
	                                  GenListPos& thePos) const;// +0x28
	virtual void       GenSetCacheSize(uint32 nSize);           // +0x2c
};

struct Arr32
{
	uint32 sub_1007ee60() const;                        // GetSize
	int32  sub_1001d340(uint32 nNewSize);               // SetSize
	int32  sub_1001d270(uint32 nNewSize);               // NiceSetSize
	T32&   sub_1001d370(uint32 nIndex) const;           // operator[]
	void   sub_1001d7e0(IAllocator* pAlloc = &g_10091df0);      // Term

	int32  sub_1001d080(const GenList32& theList);      // GenCopyList
	int32  sub_1001cfa0(const GenList32& theList);      // GenAppendList
};

int32 Arr32::sub_1001d080(const GenList32& theList)
{
	GenListPos pos;
	uint32     iCurOut;

	if (!sub_1001d340(theList.GenGetSize()))
		return LTFALSE;

	iCurOut = 0;
	for (pos = theList.GenBegin(); theList.GenIsValid(pos); )
	{
		if (iCurOut >= sub_1007ee60())
		{
			sub_1001d7e0();
			return LTFALSE;
		}

		sub_1001d370(iCurOut) = theList.GenGetNext(pos);
		iCurOut++;
	}

	return LTTRUE;
}

int32 Arr32::sub_1001cfa0(const GenList32& theList)
{
	GenListPos pos;
	uint32     iCurOut, prevSize;

	prevSize = sub_1007ee60();

	if (!sub_1001d270(prevSize + theList.GenGetSize()))
		return LTFALSE;

	iCurOut = prevSize;
	for (pos = theList.GenBegin(); theList.GenIsValid(pos); )
	{
		if (iCurOut >= sub_1007ee60())
		{
			sub_1001d7e0();
			return LTFALSE;
		}

		sub_1001d370(iCurOut) = theList.GenGetNext(pos);
		iCurOut++;
	}

	return LTTRUE;
}


// ---------------------------------------------------------------------- //
//  Instantiation 2 -- T is 16 bytes (`rep movsl` with %ecx = 4).
//  0x1007ec20 = GenAppendList; GetSize 0x1007ee60, NiceSetSize 0x1007ee70,
//  operator[] 0x1005a3c0, Term 0x1007f3c0.
// ---------------------------------------------------------------------- //
struct T16 { uint32 m_v[4]; };

struct GenList16
{
	virtual GenListPos GenBegin() const;
	virtual int32      GenIsValid(const GenListPos& pos) const;
	virtual T16        GenGetNext(GenListPos& pos) const;
	virtual T16        GenGetAt(GenListPos& pos) const;
	virtual int32      GenAppend(T16& toAppend);
	virtual void       GenRemoveAt(GenListPos pos);
	virtual void       GenRemoveAll();
	virtual uint32     GenGetSize() const;
	virtual int32      GenCopyList(const GenList16& theList);
	virtual int32      GenAppendList(const GenList16& theList);
	virtual int32      GenFindElement(const T16& toFind,
	                                  GenListPos& thePos) const;
	virtual void       GenSetCacheSize(uint32 nSize);
};

struct Arr16
{
	uint32 sub_1007ee60() const;                        // GetSize
	int32  sub_1007ee70(uint32 nNewSize);               // NiceSetSize
	T16&   sub_1005a3c0(uint32 nIndex) const;           // operator[]
	void   sub_1007f3c0(IAllocator* pAlloc = &g_10091df0);      // Term

	int32  sub_1007ec20(const GenList16& theList);      // GenAppendList
};

int32 Arr16::sub_1007ec20(const GenList16& theList)
{
	GenListPos pos;
	uint32     iCurOut, prevSize;

	prevSize = sub_1007ee60();

	if (!sub_1007ee70(prevSize + theList.GenGetSize()))
		return LTFALSE;

	iCurOut = prevSize;
	for (pos = theList.GenBegin(); theList.GenIsValid(pos); )
	{
		if (iCurOut >= sub_1007ee60())
		{
			sub_1007f3c0();
			return LTFALSE;
		}

		sub_1005a3c0(iCurOut) = theList.GenGetNext(pos);
		iCurOut++;
	}

	return LTTRUE;
}


// ---------------------------------------------------------------------- //
//  Instantiation 3 -- T is a POINTER.  0x1007f670 = GenCopyList, and the
//  difference shows: GenGetNext comes back in %eax with no hidden return
//  slot, and the store is one `mov %edi,(%eax)` into the operator[]
//  result, evaluated AFTER the call rather than before it.
//  SetSize 0x1007f8b0, GetSize 0x1007ee60, operator[] 0x1007c870,
//  Term 0x1007fa50.
// ---------------------------------------------------------------------- //
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
	int32  sub_1007f8b0(uint32 nNewSize);               // SetSize
	void*& sub_1007c870(uint32 nIndex) const;           // operator[]
	void   sub_1007fa50(IAllocator* pAlloc = &g_10091df0);      // Term

	int32  sub_1007f670(const GenListP& theList);       // GenCopyList
};

int32 ArrP::sub_1007f670(const GenListP& theList)
{
	GenListPos pos;
	uint32     iCurOut;

	if (!sub_1007f8b0(theList.GenGetSize()))
		return LTFALSE;

	iCurOut = 0;
	for (pos = theList.GenBegin(); theList.GenIsValid(pos); )
	{
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


// ======================================================================= //
//  0x10015740 -- LTMatrix::MakeInverse(), src/LT2/sdk/inc/ltmatrix.h line 514
//
//      inline LTMatrix LTMatrix::MakeInverse()
//      {
//          LTMatrix ret;
//          ret = *this;
//          ret.Inverse();
//          return ret;
//      }
//
//      55 8b ec / 83 ec 08 / 57 57 56 53
//      8b 5d 08                 %ebx = the hidden return slot
//      8b c1 / 8b fb / 8b f0
//      b9 10 00 00 00 / f3 a5   64 bytes: *ret = *this   (NRVO -- `ret` IS
//                               the return slot, there is no second copy)
//      8b cb / e8 <0x1000ac10>  ret.Inverse()
//      8b c3                    return ret
//      5b 5e 5f / 8b e5 5d / c2 04 00
//
//  WHAT PINS IT: the callee.  0x1000ac10 is a full 4x4 Gauss-Jordan
//  elimination -- it walks four rows, seeds the right half with 0x3f800000
//  (1.0f), picks a pivot by calling fabs (0x1007fc90) and comparing, and
//  runs to 0x1000ae00.  That is LTMatrix::Inverse() (ltmatrix.h line 404),
//  and the only header line that copies `*this` into a return slot and then
//  calls it is MakeInverse.  Both call sites confirm the by-value return:
//  0x1004c318 passes `lea -0x644(%ebp)` and then copies 64 bytes out of it.
//
//  THE CALLEE: 0x1000ac10 is not a function of its own in
//  the source -- it is icl's /Qip out-of-line copy of ltmatrix.h's inline
//  Inverse, emitted byte-identical by big0ae20.cpp (/Os /Oi- /Qip) as
//  ?Inverse@LTMatrix@@QAEIXZ; the old hand-written w2_1000ac10_osoi.cpp
//  is gone.  The sub_1000ac10 spelling below is aliased onto it by
//  address.  Measured, not it: writing this file
//  with the real LTMatrix is impossible as an out-of-line definition (the
//  header defines MakeInverse inline), and probes found no flag set
//  under which a caller of the inline MakeInverse keeps the call: /Os /Oi-
//  /Qip /Qip_no_inlining inlines it even into the 6.7 KB 0x10017c14
//  (w43_10017c10_osoiqipni.cpp, whose shipped code calls 0x10015740), so
//  where this out-of-line MakeInverse comes from is still open.
// ======================================================================= //
struct LTMatrix10015740
{
	float m_Data[4][4];

	int32             sub_1000ac10();                   // Inverse
	LTMatrix10015740  sub_10015740();                   // MakeInverse
};

LTMatrix10015740 LTMatrix10015740::sub_10015740()
{
	LTMatrix10015740 ret;

	ret = *this;
	ret.sub_1000ac10();
	return ret;
}


// ======================================================================= //
//  0x100736f0 and 0x1007dc40 -- eight-byte out-of-line accessors
//
//      56 / 8b 41 <off> / 83 c4 04 / c3
//
//  `push %esi` + `add $0x4,%esp` is icl's dead 4-byte slot; the body is one
//  load off `this`.  DEFINED OUT OF LINE ON PURPOSE: /Ob1 inlines only what
//  the source marks inline, and the shipped code still CALLS both of these
//  (0x100736f0 from 0x10015cfe and 0x1001940d), so putting either in a
//  class body would silently break its callers.
//
//  0x1007dc40 sits at slot +0x18 of the eight-entry vtable at 0x10089670,
//  whose sibling table at 0x10089270 has 0x100037d0 in the same slot.
// ======================================================================= //
struct C100736f0
{
	uint8  m_pad_00[0x04];
	void*  m_p04;                                       // +0x04

	void*  sub_100736f0();
};

void* C100736f0::sub_100736f0()
{
	return m_p04;
}


struct C1007dc40
{
	uint8  m_pad_00[0x54];
	void*  m_p54;                                       // +0x54

	void*  sub_1007dc40();
};

void* C1007dc40::sub_1007dc40()
{
	return m_p54;
}


// ======================================================================= //
//  0x1003abc0 -- twelve bytes, a two-hop accessor
//
//      56 / 8b 49 18 / 0f b7 41 28 / 83 c4 04 / c3
//
//  `this->m_p18->m_w28`, zero-extended.  Its neighbour 0x1003abd0 is the
//  same twelve bytes with 0x2a instead of 0x28, so the pair is almost
//  certainly a width/height (or x/y) accessor over a WORD-typed record.
//  It is slot +0x0c of the vtable at 0x100894f0.
//
//  The return type is written uint32, not uint16: `movzx eax, word ptr` is
//  the zero-extension of a WORD member into a DWORD result: a uint16 return
//  would leave icl free to emit the 16-bit `66 8b 41 28` instead.
// ======================================================================= //
struct Rec1003abc0
{
	uint8  m_pad_00[0x28];
	uint16 m_w28;                                       // +0x28
	uint16 m_w2a;                                       // +0x2a
};

struct C1003abc0
{
	uint8        m_pad_00[0x18];
	Rec1003abc0* m_p18;                                 // +0x18

	uint32 sub_1003abc0();
};

uint32 C1003abc0::sub_1003abc0()
{
	return m_p18->m_w28;
}


// ======================================================================= //
//  0x100037d0 -- twenty-four bytes, one test and two constants
//
//      56 / 8b 49 28            %ecx = this->m_b28
//      85 c9 / 74 07            if (!m_b28) goto L
//      b8 24 00 00 00 / eb 05       %eax = 0x24 ; goto end
//   L: b8 28 00 00 00           %eax = 0x28
//      83 c4 04 / c3
//
//  Slot +0x18 of the vtable at 0x10089270, i.e. the same slot 0x1007dc40
//  occupies in its sibling.  A vertex-size / stride query is the obvious
//  reading (0x24 = 36, 0x28 = 40, one dword apart) but nothing in the bytes
//  says so, so the name stays numeric.
//
//  WHAT PINS THE SHAPE: the `jmp` over the second constant -- the two arms
//  share the epilogue, and the FIRST one jumps over the second.  That
//  needs +0x28 to be an INTEGER: as a `void*` (the earlier version) icl's
//  static predictor takes the non-null arm as the hot fall-through, hosts
//  the epilogue after it and the null arm jumps back (11 bytes differing).
//  As uint32 it is identical -- the twin 0x100037b0 in d3dr4a.cpp measured
//  the same.
// ======================================================================= //
struct C100037d0
{
	uint8  m_pad_00[0x28];
	uint32 m_b28;                                       // +0x28

	uint32 sub_100037d0();
};

uint32 C100037d0::sub_100037d0()
{
	return m_b28 ? 0x24 : 0x28;
}


// ======================================================================= //
//  0x1006c680 -- four bytes
//
//      33 c0 / 40 / c3          return 1
//
//  No frame, no dummy slot, no arguments.  Nothing calls it directly; its
//  address is planted in a local structure at 0x1006ce33 (alongside
//  0x1006c670 at 0x1006ce3d), so it is a callback, written here as the
//  free function that gives those four bytes.
// ======================================================================= //
int32 sub_1006c680()
{
	return 1;
}


// ======================================================================= //
//  0x1005a040 -- ten bytes
//
//      56 / 33 c0 / 40 / 83 c4 04 / c2 08 00     return 1, with two
//                                                arguments never read
//
//  Its address is in the table at 0x1008971c, one entry away from
//  0x1005a050, so it is a slot implementation.  `ret $0x8` proves the two
//  parameters exist even though the body ignores them, which is
//  normal.  Written as a member (this in %ecx, unused) to
//  match the dummy-slot idiom of the other table entries in this file.
// ======================================================================= //
struct C1005a040
{
	int32 sub_1005a040(void* pA, void* pB);
};

int32 C1005a040::sub_1005a040(void* pA, void* pB)
{
	return 1;
}


// ======================================================================= //
//  0x100234b0 -- twenty-one bytes
//
//      56 / 33 c0
//      89 41 1c / 89 41 20 / 89 41 24 / 89 41 28   four dwords to zero
//      83 c4 04 / 8b c1 / c3                        return this
//
//  `mov %ecx,%eax` on the way out is the return of `this`, and the four
//  stores are in ASCENDING offset order, which is source
//  order and need not be declaration order.  One caller, 0x10026746.
// ======================================================================= //
struct C100234b0
{
	uint8  m_pad_00[0x1c];
	uint32 m_1c;                                        // +0x1c
	uint32 m_20;                                        // +0x20
	uint32 m_24;                                        // +0x24
	uint32 m_28;                                        // +0x28

	C100234b0* sub_100234b0();
};

C100234b0* C100234b0::sub_100234b0()
{
	m_1c = 0;
	m_20 = 0;
	m_24 = 0;
	m_28 = 0;

	return this;
}


// ======================================================================= //
//  0x100039d0 -- fifty-two bytes, three virtual calls on three globals
//
//      55 8b ec
//      a1 <g>  b9 <g>  8b 40 0c  ff d0     x3
//      8b e5 / 5d / c3
//
//  The pair `mov <g>,%eax` then `mov $<g>,%ecx` is the C++ virtual call on
//  a global OBJECT (not a pointer): the first load is the vptr at the
//  object's +0x00, the second is `this`.  Slot +0x0c is index 3 of the
//  eight-entry interface whose tables sit at 0x10089260 and 0x10089670.
//  Two callers, 0x10008b14 and 0x10015e21.
// ======================================================================= //
struct I100039d0
{
	virtual void Slot00();                              // +0x00
	virtual void Slot04();                              // +0x04
	virtual void Slot08();                              // +0x08
	virtual void Slot0c();                              // +0x0c
};

extern I100039d0 g_1008ad4c;                            // 0x1008ad4c
extern I100039d0 g_1008ad10;                            // 0x1008ad10
extern I100039d0 g_10091d80;                            // 0x10091d80

// Called on the object directly, icl devirtualises all three into one
// direct call to I100039d0::Slot0c -- which cannot stand for them: the
// three objects' tables are 0x10089260 and 0x10089670, two different
// implementations. Through a pointer parameter of an inline helper the
// calls stay virtual and the function is byte-identical (52
// bytes; the direct form was 37 bytes, 9 differing, and left
// ?Slot0c@I100039d0@@UAEXXZ unresolved at link).
static inline void Slot0c_100039d0(I100039d0* p)
{
	p->Slot0c();
}

void sub_100039d0()
{
	Slot0c_100039d0(&g_1008ad4c);
	Slot0c_100039d0(&g_1008ad10);
	Slot0c_100039d0(&g_10091d80);
}


// ======================================================================= //
//  0x1003d8f0 -- thirty-one bytes, two globals out through two pointers
//
//      56
//      a1 <0x1008e664>  8b 54 24 08  8b 4c 24 0c  89 02
//      a1 <0x1008e668>  89 01
//      83 c4 04 / 33 c0 / c2 08 00
//
//  Slot +0x10 of the vtable at 0x10089530.  `xor %eax,%eax` after the
//  stores is `return 0`.  The two arguments are out-parameters; the two
//  globals are read, never written.
// ======================================================================= //
extern uint32 g_1008e664;                               // 0x1008e664
extern uint32 g_1008e668;                               // 0x1008e668

struct C1003d8f0
{
	int32 sub_1003d8f0(uint32* pA, uint32* pB);
};

int32 C1003d8f0::sub_1003d8f0(uint32* pA, uint32* pB)
{
	*pA = g_1008e664;
	*pB = g_1008e668;

	return 0;
}


// ======================================================================= //
//  0x100158d0 -- sixty-one bytes, a bounds-checked lookup
//
//      55 8b ec / 83 ec 10 / 56 53
//      0f b7 75 0a              %esi = the HIGH WORD of the one argument
//      8b d9 / b9 88 01 00 00 / 03 cb      %ecx = this + 0x188
//      e8 <0x1007ee60>          GetSize()
//      3b f0 / 72 0a            if (%esi < size) goto body
//      33 c0                    return LTNULL
//      5b 5e / 8b e5 5d / c2 04 00
//  body: 57 / 81 c3 88 01 00 00 / 89 34 24 / 8b cb
//      e8 <0x1001e0c0>          operator[](index)
//      8b 00                    dereference it
//      eb e3                    back to the shared epilogue
//
//  WHAT PINS IT: 0x1007ee60 is CMoArray::GetSize (`mov 0x8(%ecx),%eax`) and
//  0x1001e0c0 is the 28-byte operator[] forwarder, both called on
//  `this + 0x188` -- so there is a CMoArray of pointers embedded at +0x188
//  and the answer is one of its elements.  The `mov (%eax),%eax` after
//  operator[] is the load through the returned T&, so T is a pointer.
//
//  The guard is written FIRST because that is what the layout shows: the
//  `return LTNULL` arm is the fallthrough and hosts the shared epilogue,
//  and the array arm jumps back into it.
//
//  THE ONE COIN-FLIP IN THIS FILE is `>> 16`.  The shipped code reads a WORD
//  at 0xa(%ebp) -- two bytes into a four-byte argument -- which is icl
//  narrowing `(nHandle >> 16)` to a word load, the only form that both
//  zero-extends and needs no shift.  IF IT MISSES, the alternative is that
//  the parameter is a four-byte STRUCT passed by value with a uint16 at
//  +0x02, i.e.
//      struct H1000158d0 { uint16 m_lo; uint16 m_hi; };
//      ... sub_100158d0(H1000158d0 h) { ... h.m_hi ... }
//  which reaches the same word by a member access instead of a shift.
// ======================================================================= //
struct Arr100158d0
{
	uint32  sub_1007ee60() const;                       // GetSize
	void**  sub_1001e0c0(uint32 nIndex) const;          // operator[]
};

struct C100158d0
{
	uint8       m_pad_000[0x188];
	Arr100158d0 m_List;                                 // +0x188

	void* sub_100158d0(uint32 nHandle);
};

void* C100158d0::sub_100158d0(uint32 nHandle)
{
	uint32 nIndex;

	nIndex = nHandle >> 16;

	if (nIndex >= m_List.sub_1007ee60())
		return LTNULL;

	return *m_List.sub_1001e0c0(nIndex);
}


// ======================================================================= //
//  0x10035ae0 and 0x10037470 -- the vptr-restore-then-maybe-free pair
//
//      83 ec 0c                 twelve bytes of outgoing area
//      8b 44 24 10              %eax = the flags argument
//      c7 01 <vtable>           this->vptr = &<vtable>
//      a8 01 / 75 06            if (flags & 1)
//      83 c4 0c / c2 04 00          (the shared exit)
//      89 0c 24 / e8 <0x10080479>   free(this)
//
//  Both write a .data vtable address through %ecx and then conditionally
//  hand `this` to the same cdecl one-argument routine at 0x10080479, which
//  is above the 0x1007fa10 CRT line.
//
//  THE TWO DIFFER IN EXACTLY ONE WAY and it is a flag difference, not a
//  source difference: 0x10035ae0 closes the taken arm with `eb f0`, a jump
//  back into the shared epilogue, while 0x10037470 DUPLICATES the epilogue
//  (`83 c4 0c / c2 04 00` a second time).  That is /Os versus /O2 on the
//  same construct.  MEASURED: 0x10037470 is identical at /O2 and now lives
//  in d3dr4b_o2.cpp; 0x10035ae0 stays here at /Os.
// ======================================================================= //
extern void __cdecl sub_10080479(void* pMem);

extern void* const g_vtbl_10089450[];                   // 0x10089450
extern void* const g_vtbl_100894c0[];                   // 0x100894c0

struct C10035ae0
{
	void* m_pVTable;                                    // +0x00

	void  sub_10035ae0(uint32 nFlags);
};

void C10035ae0::sub_10035ae0(uint32 nFlags)
{
	m_pVTable = (void*)g_vtbl_10089450;

	if ((nFlags & 1) == 0)
		return;

	sub_10080479(this);
}


// C10037470 (0x10037470) -> d3dr4b_o2.cpp: MEASURED, it is the /O2 half of
// this pair (duplicated epilogue) and is byte-identical there.


// ======================================================================= //
//  0x10050de0 -- forty-two bytes, tear down a pool and rebuild it
//
//      53 / 83 ec 10
//      6a 01 / 8b d9 / b8 04 00 00 00 / 03 c3 / 50
//      e8 <0x1007fba0>          f(&m_Pool, 1)
//      83 c4 0c
//      6a 20 / 83 c3 04 / 6a 28 / 53
//      e8 <0x1007fb20>          g(&m_Pool, 0x28, 0x20)
//      83 c4 18 / 5b / c3
//
//  Neither callee is CRT despite sitting above 0x1007fa10.  0x1007fb20 is
//  a fixed-block-pool initialiser: it writes its second argument to +0x00
//  and +0x04 of the pool, clamps +0x04 up to 4, rounds it up to a multiple
//  of 4, stores the third at +0x08 and zeroes +0x0c..+0x18.  0x1007fba0 is
//  the matching teardown: it walks the singly-linked chain at +0x14 freeing
//  each block through 0x10021cb0, then zeroes the same four dwords.  So
//  this is "reset the pool to 0x28-byte elements, 0x20 per block".
//
//  Both calls are cdecl -- the caller pops (`add $0xc`, `add $0x18`).  The
//  pool is at this+0x04.  No frame pointer, which is icl's own choice for a
//  body with no locals.
// ======================================================================= //
struct Pool10050de0
{
	uint8 m_pad_00[0x1c];
};

extern void __cdecl sub_1007fba0(Pool10050de0* pPool, int32 bFreeAll);
extern void __cdecl sub_1007fb20(Pool10050de0* pPool, uint32 nElemSize,
                                 uint32 nPerBlock);

struct C10050de0
{
	uint8        m_pad_00[0x04];
	Pool10050de0 m_Pool;                                // +0x04

	void sub_10050de0();
};



// ======================================================================= //
//  0x1005db90 -- forty-eight bytes, one four-argument construction
//
//      55 8b ec / 83 ec 10 / 83 c4 f0      sixteen bytes of outgoing area
//      33 c0
//      c7 44 24 04 00 00 80 3f             arg1 = 1.0f
//      c7 04 24 <"EnvMapPolyGrids">        arg0 = the name
//      b9 <0x1008fe68>                     this = the global
//      89 44 24 08 / 89 44 24 0c           arg2 = arg3 = 0
//      e8 <0x10021f40> / 8b e5 5d c3
//
//  0x10021f40 is a console-variable node constructor: `ret $0x10`, it
//  stores its four arguments at +0x08..+0x14 of `this`, zeroes +0x00 and
//  +0x04, links itself onto the global chain head at 0x1008bf90 (old head
//  into +0x1c) and returns `this` in %eax.  The 1.0f arrives as a raw
//  immediate because it is a float PARAMETER with a constant argument, not
//  because the source is moving bit patterns.
//
//  This is the dynamic initialiser for a file-scope console variable.  It
//  is written as an explicit call so that it has a name that maps to its address;
//  the shipped source is almost certainly a definition of the form
//  `CConVar g_EnvMapPolyGrids("EnvMapPolyGrids", 1.0f, 0, 0);`.
// ======================================================================= //
struct CVar1005db90
{
	CVar1005db90* sub_10021f40(const char* pName, float fVal,
	                           uint32 nA, uint32 nB);
};

extern CVar1005db90 g_1008fe68;                         // 0x1008fe68

void sub_1005db90()
{
	g_1008fe68.sub_10021f40("EnvMapPolyGrids", 1.0f, 0, 0);
}


// ======================================================================= //
//  0x1003b760 -- thirty-eight bytes, a two-argument cdecl callback
//
//      55 8b ec / 83 ec 08 / 83 c4 f8
//      8b 45 0c                 %eax = the SECOND argument
//      8b 0d <0x1008ef3c>       this = *(void**)0x1008ef3c  (a POINTER
//                               variable -- the value is loaded, not the
//                               address taken)
//      89 04 24
//      c7 44 24 04 <0x1003b740> the other callback
//      e8 <0x100500a0> / 8b e5 5d / c3
//
//  The trailing bare `ret` is __cdecl, and the first argument is never
//  read.  That is exactly the callback signature the registrar imposes:
//  0x100500a0's own `call *%edi` invokes its function-pointer argument as
//  `pfn(0x1008b880, pObj)`, two cdecl dwords.
//
//  NOTE d3dr2b.cpp already declares `extern void sub_1003b760()` for the
//  sole purpose of taking its address; that declaration has the wrong arity
//  (it only ever needs the symbol) and the two TUs are compiled separately,
//  so it does not collide with this definition.
// ======================================================================= //
extern void __cdecl sub_1003b740(void* pA, void* pB);

struct Reg100500a0
{
	void sub_100500a0(void* pObj, void (__cdecl* pfn)(void*, void*));
};

extern Reg100500a0* g_p1008ef3c;                        // 0x1008ef3c

void __cdecl sub_1003b760(void* pA, void* pB)
{
	g_p1008ef3c->sub_100500a0(pB, sub_1003b740);
}


// 0x1001bf34 moved to big17c14.cpp: it is a /Qip two-entry pair (the
// stack lead-in at 0x1001bf30) whose one caller, 0x10019484, enters at the
// register entry, so it must share its caller's TU.


// NEAR, one residual that is the compiler's: ours emits a zero outgoing-area
// adjust, `add $0x0,%esp` (83 c4 00), between `sub $0x10,%esp` and the first
// push; the shipped code has none.  The shipped d3d.ren contains NO `83 c4 00`
// anywhere, while our icl emits it in five objects (d3dr4b, d3dr5a's
// 0x10050cc0, d3dlow's 0x100157e0, d3dr2b's 0x1001ead0, big16324), always
// frameless and always where the first call's pushes exactly fill the area.
// /Os is right (/O2 turns the pushes into movs); /G5 /G6 /G7 /Qip /Oi-
// /Qrcd /O1 /Oy- and int/bool/return-type variants of the callees all
// leave the adjust in.
// NEAR: ours adds `83 c4 00` (add $0x0,%esp) ahead of the first push, the
// icl artefact d3dr5a.cpp records at 0x10050cc0.  Also measured here,
// unchanged: /Qsfalign8, /Qsfalign16, /Qsfalign-, a pPool local, the
// member address spelled as `this + 4`, and 0x1007fba0 declared varargs.
// 0x10050de0 is ObjectBank<Node56ab4>::Term (sb_Term2, then sb_Init with
// 0x28-byte nodes, 0x20 per block), now the template's own: d3dr5b.cpp
// instantiates it (/Os /Qip), with this same near residual.  The stand-in
// C10050de0::sub_10050de0 that was defined here was a second copy of it.


// ----------------------------------------------------------------------- //
//  0x1007e7c0   38 bytes of body, then a 0x90.
//
//      53 / 56                  %ebx saved, dead slot
//      33 db                    i = 0
//      8b 41 44 / 85 c0 / 76 16 if (m_nCount == 0) -> out      [jbe]
//      33 d2                    the byte offset
//   L: 8b 41 40                 the base, RELOADED every iteration
//      8b 44 10 0c              mov  0xc(%eax,%edx,1),%eax
//      85 c0 / 77 0e            if (v > 0) -> the decrement    [ja]
//   M: 43 / 83 c2 10            i++ ; offset += 0x10
//      3b 59 44 / 72 ec         while (i < m_nCount)           [jb]
//      83 c4 04 / 5b / c3       ret
//      8b 41 40 / ff 4c 10 0c   the base again ; decl
//      eb e9                    jmp M
//
//  Every comparison is unsigned, so the count and the field are unsigned.
//  The element stride is 0x10 and the field is at +0x0c.  Both the base and
//  the count are reloaded inside the loop, which is the ordinary
//  `for (i = 0; i < m_nCount; i++)` over `m_pElems[i]` after icl's strength
//  reduction -- exactly the reload pattern d3dbatchc.cpp records for
//  0x1007b8e0.
// ----------------------------------------------------------------------- //
struct Elem1007e7c0                                     // 0x10 bytes
{
	uint32	m_n00;                                      // +0x00
	uint32	m_n04;                                      // +0x04
	uint32	m_n08;                                      // +0x08
	uint32	m_n0c;                                      // +0x0c
};

// The array is reached through INLINE ACCESSORS, not a raw pointer member:
// written `m_pElems[i].m_n0c` icl CSEs the element address and the field
// value (`mov ..,%ebx; dec %ebx; mov %ebx,..` with %esi/%ebp saved); through
// an inline operator[] each access re-forms the address, so the decrement is
// the shipped `mov 0x40(%ecx),%eax; decl 0xc(%eax,%edx,1)` RMW and the frame
// becomes the shipped `push %ebx; push %esi`.  Same mechanism as
// d3dr5d_os.cpp's 0x1001e770 (LTVector::Dot stops the plane being hoisted).
// The pair at +0x40/+0x44 is CMoArray's m_pArray/m_nElements shape
// (operator[] -> Get, GetSize), which would put a CMoArray vptr at +0x3c;
// not modelled, it changes no byte here.  That +0x3c object is d3dr4c's
// C1007f3f0 (destroyed by 0x1007dc00) and d3dr4a's Arr1007e, the CMoArray over
// a 16-byte T -- consistent with the stride here.
struct Arr1007e7c0
{
	Elem1007e7c0&	operator[](uint32 i) const { return m_pArray[i]; }
	uint32			GetSize() const { return m_nElements; }

	Elem1007e7c0*	m_pArray;                           // +0x00
	uint32			m_nElements;                        // +0x04
};

class C1007e7c0
{
public:
	void	sub_1007e7c0();

	uint8			m_Reserved00[0x40];                 // +0x00..+0x3f
	Arr1007e7c0		m_Elems;                            // +0x40
};

// IDENTICAL.  The one residual was block placement: the shipped code puts
// the decrement arm OUT OF LINE (`ja` forward to `mov; decl; jmp` after the
// ret), and every if spelling kept it inline (`if (x)`, `if (x <= 0)
// continue;`, `if (x == 0) {} else`, `?:`, `goto`, `-= 1`, `--x`, `while`,
// a local reference, an inline Elem::Dec(), Get(i) for the store,
// `volatile`, and /O2 /O1 /Os /Qip).  icl sinks the default arm of a
// `switch`: `switch (x > 0) { case 0: break; default: x--; }` is
// byte-identical (`switch (x)` gives `jne` for the `ja`, one byte off).
void C1007e7c0::sub_1007e7c0()
{
	for (uint32 i = 0; i < m_Elems.GetSize(); i++)
	{
		switch (m_Elems[i].m_n0c > 0)
		{
			case 0: break;
			default: m_Elems[i].m_n0c--;
		}
	}
}
