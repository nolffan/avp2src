// Split out of d3dr4a.cpp: these need /O2 where the rest of that file needs
// /Os.  d3d.ren is a mixed build and our file boundaries are invented, so a
// file holding functions from two kinds of shipped TU loses whichever group
// the flag does not suit.  Measured: /Os alone 28 of 38, /O2 alone 23, split
// 32.  The tell is the `return 1` spelling -- `mov $1,%eax` here against
// `xor %eax,%eax; inc %eax` in the /Os group.
// *********************************************************************** //
//
//  MODULE:   SMALL_A (.CPP) -- d3d.ren, "small_a".
//
//  WHAT THESE ROWS TURNED OUT TO BE.  Twenty-two of the forty rows are
//  ONE THING: the out-of-line members of src/LT2/lithshared/stdlith/dynarray.h's
//  `CMoArray<T,C>`, instantiated over six different element types, and in
//  particular the `GenList<T>` VIRTUAL OVERRIDES that dynarray.h spells out
//  in full inside the class body:
//
//      virtual GenListPos GenBegin() const          {return GenListPos((DWORD)0);}
//      virtual BOOL   GenIsValid(const GenListPos &pos) const
//                                                   {return pos.m_Index < GetSize();}
//      virtual T      GenGetNext(GenListPos &pos) const
//                             {T& ret = Get(pos.m_Index); ++pos.m_Index; return ret;}
//      virtual T      GenGetAt(GenListPos &pos) const  {return Get(pos.m_Index);}
//      virtual BOOL   GenAppend(T &toAppend)        {return Append(toAppend);}
//      virtual void   GenRemoveAt(GenListPos pos)   {Remove(pos.m_Index);}
//      virtual void   GenRemoveAll()                {Term();}
//      virtual DWORD  GenGetSize() const            {return GetSize();}
//      virtual BOOL   GenCopyList(const GenList<T> &theList)     { ...loop... }
//      virtual BOOL   GenAppendList(const GenList<T> &theList)   { ...loop... }
//      virtual void   GenSetCacheSize(DWORD size)   {SetCacheSize(size);}
//
//  plus the non-virtual helpers those call:
//
//      BOOL  Append(const T &toAdd)      {return Insert(m_nElements, toAdd);}
//      T&    operator[](const DWORD i) const  {return Get(i);}
//      void  SetCacheSize(DWORD size)    {m_Cache.SetWantedCache(size);}
//
//  HOW THE FAMILY WAS PROVED, not assumed.  Three independent hooks:
//
//    * 0x10091df0 is stdlith's `g_DefAlloc`, already named by d3dr2b.cpp,
//      and it is what every `Term()` / `Remove()` / `SetSize()` call site in
//      these bodies materialises -- i.e. the DEFAULT ARGUMENT of
//      `Term(LAlloc *pAlloc=&g_DefAlloc)` is being formed in the caller,
//      exactly as the header declares it.
//    * `ret $0x8` on a one-value forwarder.  0x1001c6a0 and 0x1007c690 take
//      EIGHT stack bytes and pass only the first dword on.  genlist.h says
//      why: `GenRemoveAt(GenListPos pos)` takes its position BY VALUE, and
//      release-build GenListPos is `union{void*;DWORD;} m_Index` plus a
//      `BaseGenListPos m_SubIndex` -- eight bytes, first dword `m_Index`.
//    * 0x1007c700 is `pos.m_Index < GetSize()` line for line, and
//      0x1001dc00 is `T& ret = Get(pos.m_Index); ++pos.m_Index; return ret;`
//      line for line -- the `incl (%ebx)` sits BETWEEN the Get call and the
//      dereference of its result, which is only true if `ret` is a
//      REFERENCE.  That single instruction ordering is what identifies
//      GenGetNext against GenGetAt.
//
//  Six distinct instantiations are separated here, by the element size their
//  `Get` scales by and by which helper addresses they reach:
//
//      T = 64 bytes  (Get 0x1001c790, `shl $0x6`)      -- "Arr1001c"
//      T = 16 bytes  (Get 0x1007ee40, `shl $0x4`)      -- "Arr1005a"
//      T =  2 bytes  (Get 0x1001dc30)                  -- "Arr1001d"
//      T =  4 bytes  (Get 0x10009d40, `lea (,%eax,4)`) -- "Arr1007c"
//      T = 12 bytes  (Get 0x10050790, `8i + 4i`)       -- "Arr10050"
//      T = 16 bytes, second instantiation (Insert 0x1007f360) -- "Arr1007e"
//
//  A wrapper's bytes do not depend on T except through the element size, so
//  the structs below carry only the fields and the callee declarations each
//  body actually touches.  Callees are declared and never defined, named
//  after their own address; relocations are masked by the checker, so the
//  call displacements do not have to resolve.
//
//  TWO ROWS ARE NOT WRITTEN, and neither is a time problem:
//
//    0x1007c554  IS NOT A FUNCTION ENTRY.  It has no prologue, reads
//                `-0x4(%ebp)` from a frame it never set up, and 0x1007c50a
//                (`jae 0x1007c554`) branches into it: it is the
//                `{Term(); return FALSE;}` arm of the GenCopyList-shaped
//                function that starts earlier.  The enumeration landed
//                mid-body.
//    0x1001c2c0  the listed size (37) is wrong by an order of magnitude.
//                Its first branch is `jb 0x1001c420` and the body runs to
//                0x1001c450 -- roughly 400 bytes of CMoArray insert/copy
//                worker, not a small function.  Left with the large bodies.
//
//  FLAG WARNING for whoever measures this file.  It is not internally
//  consistent about one idiom: 0x1001c4c0 / 0x1001c5a0 / 0x1007ca60 return
//  TRUE as `33 c0 40` (xor/inc) while 0x1003aba0 returns 1 as
//  `b8 01 00 00 00`.  Those came from different shipped translation units.
//  Whichever way the file is compiled, one of the two spellings will lose.
//
// *********************************************************************** //

#include "ltbasedefs.h"


// ======================================================================= //
//  SHARED DECLARATIONS
// ======================================================================= //

// ---------------------------------------------------------------------- //
//  0x10091df0 -- stdlith's `g_DefAlloc` (l_allocator.h).  Same declaration
//  d3dr2b.cpp uses; repeated because this is a separate translation unit.
//  Declared, never defined, so no vtable is emitted for it.
// ---------------------------------------------------------------------- //
struct IAllocator
{
	virtual void* Slot00(uint32 nSize);                 // vtable +0x00
	virtual void* Alloc(uint32 nSize);                  // vtable +0x04
	virtual void  Free(void* pMem);                     // vtable +0x08
};

extern IAllocator g_10091df0;


// ---------------------------------------------------------------------- //
//  genlist.h's GenListPos, release layout.
//
//  WHAT PINS IT: eight bytes (`ret $0x8` on GenRemoveAt, and `mov $0x2,%ecx
//  ; rep movsl` for `pos = theList.GenBegin()`), with `m_Index` first --
//  every reader in this file loads `(%eax)` from it.  The user-declared
//  constructors are load-bearing too: they are what makes GenBegin's return
//  value travel through a hidden result pointer (`lea -0x5c(%ebp),%eax ;
//  push %eax ; call *(%eax)`) instead of coming back in %edx:%eax.
// ---------------------------------------------------------------------- //
struct BaseGenListPos
{
	uint32	m_Index;
};

struct GenListPos : public BaseGenListPos
{
	GenListPos()				{}
	GenListPos(uint32 nIndex)	{ m_Index = nIndex; }

	BaseGenListPos	m_SubIndex;
};


// ======================================================================= //
//  Arr1001c -- CMoArray<T,C> with a 64-byte T.
//
//  Get is 0x1001c790: `mov 0x8(%esp),%eax ; shl $0x6,%eax ;
//  add 0x4(%ecx),%eax`, i.e. `m_pArray[index]` with sizeof(T)==64, which is
//  also the `rep movsl` count (0x10 dwords) in the two list bodies below.
// ======================================================================= //
struct T1001c
{
	uint8	m_Data[64];
};

struct GenList1001c
{
	virtual GenListPos	GenBegin() const = 0;                          // +0x00
	virtual uint32		GenIsValid(const GenListPos& pos) const = 0;   // +0x04
	virtual T1001c		GenGetNext(GenListPos& pos) const = 0;         // +0x08
	virtual T1001c		GenGetAt(GenListPos& pos) const = 0;           // +0x0c
	virtual uint32		GenAppend(T1001c& toAppend) = 0;               // +0x10
	virtual void		GenRemoveAt(GenListPos pos) = 0;               // +0x14
	virtual void		GenRemoveAll() = 0;                            // +0x18
	virtual uint32		GenGetSize() const = 0;                        // +0x1c
	virtual uint32		GenCopyList(const GenList1001c& other) = 0;    // +0x20
	virtual uint32		GenAppendList(const GenList1001c& other) = 0;  // +0x24
	virtual uint32		GenFindElement(const T1001c& toFind,
	                                   GenListPos& thePos) const = 0;  // +0x28
	virtual void		GenSetCacheSize(uint32 nSize) = 0;             // +0x2c
};

struct Arr1001c
{
	// --- declared only; these are the helpers the bodies below call. ---
	uint32		sub_1007ee60() const;                     // GetSize
	uint32		sub_1001c880(uint32 nNewSize);            // SetSize
	uint32		sub_1001c7b0(uint32 nNewSize);            // NiceSetSize
	void		sub_1001cd10(IAllocator* pAlloc = &g_10091df0);  // Term
	void		sub_1001ca80(uint32 nIndex);              // Remove
	T1001c&		sub_1001c790(uint32 nIndex) const;        // Get

	// --- written here ---
	T1001c&		sub_1001c8b0(uint32 nIndex) const;        // operator[]
	void		sub_1001c6a0(GenListPos pos);             // GenRemoveAt
	uint32		sub_1001c5a0(const GenList1001c& theList);      // GenCopyList
	uint32		sub_1001c4c0(const GenList1001c& theList);      // GenAppendList
};


// ---------------------------------------------------------------------- //
//  0x1001c8b0 -- 28 bytes.  `T& operator[](const DWORD index) const
//  {return Get(index);}`
//
//  WHAT PINS IT: it is the function 0x1001c5a0 and 0x1001c4c0 call to get
//  the destination of `(*this)[iCurOut] = theList.GenGetNext(pos)` -- the
//  `rep movsl` immediately after it writes THROUGH the returned %eax, so the
//  return is a reference.  Its own body is a bare forward of the one dword
//  argument to Get (0x1001c790).  The four-byte outgoing area is reserved
//  with `push %edi`, which is icl's dead-slot spelling of `sub $4,%esp`.
// ---------------------------------------------------------------------- //


// ---------------------------------------------------------------------- //
//  0x1001c6a0 -- 28 bytes.  `virtual void GenRemoveAt(GenListPos pos)
//  {Remove(pos.m_Index);}`
//
//  WHAT PINS IT: `ret $0x8` -- eight stack bytes -- while only ONE dword,
//  the first, is forwarded.  That is GenListPos by value.  The callee
//  0x1001ca80 is d3dr2b.cpp's already-identified `Remove(DWORD)`.
// ---------------------------------------------------------------------- //


// ---------------------------------------------------------------------- //
//  0x1001c5a0 -- 215 bytes (0x1001c5a0..0x1001c676), NOT the 38 the
//  listing says.  dynarray.h's `GenCopyList`, verbatim.
//
//  WHAT PINS IT, in order down the body:
//    call *0x1c(vtbl)            theList.GenGetSize()
//    call 0x1001c880             SetSize(...)          -- the g_DefAlloc wrapper
//    test/jne + xor %eax,%eax    if(!...) return FALSE
//    xor %edx,%edx -> -0x8(%ebp) iCurOut = 0
//    call *0x00(vtbl) + rep movsl $2   pos = theList.GenBegin()
//    call *0x04(vtbl)            theList.GenIsValid(pos)
//    call 0x1007ee60 / cmp / jae GetSize() bound check -> Term(); return FALSE
//    call 0x1001c8b0             (*this)[iCurOut]
//    call *0x08(vtbl) + rep movsl $0x10   = theList.GenGetNext(pos)
//    inc + store to -0x8(%ebp)   iCurOut++
//    33 c0 40                    return TRUE
//  The failure arm at 0x1001c662 plants $0x10091df0 and calls 0x1001cd10 --
//  `Term()` with its default allocator argument formed in the caller.
// ---------------------------------------------------------------------- //


// ---------------------------------------------------------------------- //
//  0x1001c4c0 -- 222 bytes (0x1001c4c0..0x1001c59d), NOT the 47 listed.
//  dynarray.h's `GenAppendList`, verbatim, and the same loop as above.
//
//  WHAT SEPARATES IT FROM GenCopyList: the head.  `call 0x1007ee60` FIRST
//  (prevSize = GetSize()), then `add %esi,%eax` on theList.GenGetSize(),
//  then 0x1001c7b0 -- and 0x1001c7b0 is the NiceSetSize wrapper (it plants a
//  literal 0 for bFast alongside g_DefAlloc), not the SetSize wrapper.  The
//  loop counter then starts at prevSize instead of 0, which is why there is
//  no `xor %edx,%edx` before the loop here.
// ---------------------------------------------------------------------- //


// ======================================================================= //
//  Arr1005a -- CMoArray<T,C> with a 16-byte T.
//
//  Get is 0x1007ee40: `mov 0x8(%esp),%eax ; shl $0x4,%eax ;
//  add 0x4(%ecx),%eax`.  The two list bodies use `rep movsl $4` and the
//  frame is `sub $0x38` where the 64-byte instantiation used `sub $0x68`;
//  the difference is exactly 48 bytes, i.e. sizeof(T)'s difference.
// ======================================================================= //
struct T1005a
{
	uint8	m_Data[16];
};

struct GenList1005a
{
	virtual GenListPos	GenBegin() const = 0;                          // +0x00
	virtual uint32		GenIsValid(const GenListPos& pos) const = 0;   // +0x04
	virtual T1005a		GenGetNext(GenListPos& pos) const = 0;         // +0x08
	virtual T1005a		GenGetAt(GenListPos& pos) const = 0;           // +0x0c
	virtual uint32		GenAppend(T1005a& toAppend) = 0;               // +0x10
	virtual void		GenRemoveAt(GenListPos pos) = 0;               // +0x14
	virtual void		GenRemoveAll() = 0;                            // +0x18
	virtual uint32		GenGetSize() const = 0;                        // +0x1c
	virtual uint32		GenCopyList(const GenList1005a& other) = 0;    // +0x20
	virtual uint32		GenAppendList(const GenList1005a& other) = 0;  // +0x24
	virtual uint32		GenFindElement(const T1005a& toFind,
	                                   GenListPos& thePos) const = 0;  // +0x28
	virtual void		GenSetCacheSize(uint32 nSize) = 0;             // +0x2c
};

struct Arr1005a
{
	void*		m_pVTable;			// +0x00
	T1005a*		m_pArray;			// +0x04
	uint32		m_nElements;		// +0x08

	// --- declared only ---
	uint32		sub_1007ee60() const;                     // GetSize
	uint32		sub_1007ef40(uint32 nNewSize);            // SetSize
	uint32		sub_1005a390(uint32 nNewSize);            // NiceSetSize
	void		sub_1000a630(IAllocator* pAlloc = &g_10091df0);  // Term
	T1005a&		sub_1007ee40(uint32 nIndex) const;        // Get
	T1005a&		sub_1005a3c0(uint32 nIndex) const;        // operator[]
	uint32		sub_1000a600(uint32 nIndex, const T1005a& toInsert);  // Insert

	// --- written here ---
	uint32		sub_1005a3e0(const T1005a& toAdd);        // Append
	void		sub_1005a2b0();                           // GenRemoveAll
	T1005a		sub_1005a310(GenListPos& pos) const;      // GenGetAt
	uint32		sub_1005a0f0(const GenList1005a& theList);     // GenAppendList
	uint32		sub_1007ed00(const GenList1005a& theList);     // GenCopyList
};


// ---------------------------------------------------------------------- //
//  0x1005a3e0 -- 35 bytes.  `BOOL Append(const T &toAdd)
//  {return Insert(m_nElements, toAdd);}`
//
//  WHAT PINS IT: `mov 0x8(%eax),%edx` -- the FIRST outgoing dword is read
//  straight out of the object at +0x08, which the whole family agrees is
//  `m_nElements` -- and the second is the one incoming argument.  The callee
//  0x1000a600 is d3dr2b.cpp's already-identified `Insert(DWORD, const T&)`.
//  0x1001d7b0 and 0x1007f390 are the same 35 bytes over other T.
// ---------------------------------------------------------------------- //


// ---------------------------------------------------------------------- //
//  0x1005a2b0 -- 27 bytes.  `virtual void GenRemoveAll() {Term();}`
//
//  WHAT PINS IT: the ONLY outgoing dword is the literal $0x10091df0, i.e.
//  the call takes no source-level argument at all and the allocator is the
//  default argument being materialised here.  A bare `ret` (no count) is a
//  __thiscall member with no parameters.  0x1001d160 is the same 27 bytes.
// ---------------------------------------------------------------------- //


// ---------------------------------------------------------------------- //
//  0x1005a310 -- 50 bytes.  `virtual T GenGetAt(GenListPos &pos) const
//  {return Get(pos.m_Index);}`
//
//  WHAT PINS IT: `ret $0x8` where the SECOND dword (0xc(%ebp)) is the one
//  dereferenced for the index and the FIRST (0x8(%ebp)) is the destination
//  of a 16-byte `rep movsl` and is returned in %eax -- that is the hidden
//  result pointer of a by-value T return, not a second parameter.  It calls
//  Get (0x1007ee40) DIRECTLY, not operator[], which is what distinguishes
//  GenGetAt from the list bodies below; the header says the same.
// ---------------------------------------------------------------------- //


// ---------------------------------------------------------------------- //
//  0x1005a0f0 -- 220-odd bytes, NOT the 47 listed.  GenAppendList over the
//  16-byte T: identical instruction for instruction to 0x1001c4c0 except
//  `sub $0x38` for the frame, `rep movsl $4` for the element copy, and the
//  0x1005a3c0 / 0x1005a390 helpers.
// ---------------------------------------------------------------------- //


// ---------------------------------------------------------------------- //
//  0x1007ed00 -- 210-odd bytes, NOT the 38 listed.  GenCopyList over the
//  same 16-byte T -- it reaches the SAME operator[] (0x1005a3c0) and the
//  same GetSize (0x1007ee60) as 0x1005a0f0, which is what puts the two in
//  one instantiation despite the address gap.  Its head is the GenCopyList
//  head: SetSize (0x1007ef40) and `xor %edx,%edx` for iCurOut = 0.
// ---------------------------------------------------------------------- //


// ======================================================================= //
//  Arr1001d -- CMoArray<T,C> with a 2-byte T (`unsigned short`).
//
//  WHAT PINS THE ELEMENT TYPE: 0x1001dbd0 and 0x1001dc00 both close with
//  `movzwl (%eax),%eax` on the address Get returned -- a 16-bit load widened
//  into a 32-bit return, i.e. T is `unsigned short` and it is being returned
//  BY VALUE.  Everything else in this group hangs off that: Get 0x1001dc30,
//  Insert 0x1001d780, SetSize2 0x1001d2a0 behind Term 0x1001d7e0, and
//  SetCacheSize 0x1001dc50 (which is `m_Cache.SetWantedCache(size)` -- it
//  adds 0xc to `this` and calls 0x1000a660, the DefaultCache setter
//  d3dbatchc.cpp reconstructed).
// ======================================================================= //
struct Arr1001d
{
	void*		m_pVTable;			// +0x00
	uint16*		m_pArray;			// +0x04
	uint32		m_nElements;		// +0x08

	// --- declared only ---
	uint32		sub_1007ee60() const;                     // GetSize
	void		sub_1001d7e0(IAllocator* pAlloc = &g_10091df0);  // Term
	uint16&		sub_1001dc30(uint32 nIndex) const;        // Get
	uint32		sub_1001d780(uint32 nIndex, const uint16& toInsert);  // Insert
	void		sub_1001dc50(uint32 nSize);               // SetCacheSize

	// --- written here ---
	uint32		sub_1001d7b0(const uint16& toAdd);        // Append
	uint32		sub_1001d1a0(uint16& toAppend);           // GenAppend
	void		sub_1001d160();                           // GenRemoveAll
	uint32		sub_1001db50() const;                     // GenGetSize
	uint16		sub_1001dbd0(GenListPos& pos) const;      // GenGetAt
	uint16		sub_1001dc00(GenListPos& pos) const;      // GenGetNext
	void		sub_100503f0(uint32 nSize);               // GenSetCacheSize
};


// 0x1001d7b0 -- 35 bytes.  Append; see 0x1005a3e0 for what pins the shape.


// ---------------------------------------------------------------------- //
//  0x1001d1a0 -- 28 bytes.  `virtual BOOL GenAppend(T &toAppend)
//  {return Append(toAppend);}`  One dword in, one dword straight out to
//  0x1001d7b0 (Append), `ret $0x4`.
// ---------------------------------------------------------------------- //


// 0x1001d160 -- 27 bytes.  GenRemoveAll; see 0x1005a2b0.


// ---------------------------------------------------------------------- //
//  0x1001db50 -- 19 bytes.  `virtual DWORD GenGetSize() const
//  {return GetSize();}`
//
//  WHAT PINS IT: no arguments in, no arguments out, a bare `ret`, and the
//  callee is 0x1007ee60 -- `push %esi ; mov 0x8(%ecx),%eax ; add $0x4,%esp ;
//  ret`, which is `return m_nElements`, i.e. GetSize.  The frame's
//  `sub $0x8` reserves nothing that is used; icl emits it anyway.
// ---------------------------------------------------------------------- //


// ---------------------------------------------------------------------- //
//  0x1001dbd0 -- 33 bytes.  `virtual T GenGetAt(GenListPos &pos) const
//  {return Get(pos.m_Index);}` over the 2-byte T: the argument is
//  dereferenced once (`mov (%eax),%eax`) to get m_Index, and the result of
//  Get is dereferenced once (`movzwl (%eax),%eax`) to get the value.
// ---------------------------------------------------------------------- //


// ---------------------------------------------------------------------- //
//  0x1001dc00 -- 38 bytes.  `virtual T GenGetNext(GenListPos &pos) const
//  {T& ret = Get(pos.m_Index); ++pos.m_Index; return ret;}`
//
//  WHAT PINS IT AGAINST GenGetAt: `ff 03` -- `incl (%ebx)` -- lands BETWEEN
//  the call to Get and the `movzwl (%eax),%eax` that reads its result.  The
//  increment can only be hoisted above the dereference if the source held a
//  REFERENCE across it, which is exactly how the header writes it.
// ---------------------------------------------------------------------- //


// ---------------------------------------------------------------------- //
//  0x100503f0 -- 28 bytes.  `virtual void GenSetCacheSize(DWORD size)
//  {SetCacheSize(size);}`
//
//  WHAT PINS IT: the callee 0x1001dc50 is `this+0xc` -> 0x1000a660, and
//  +0x0c is where dynarray.h puts `C m_Cache` and 0x1000a660 is
//  DefaultCache's `SetWantedCache`.  So 0x1001dc50 is `SetCacheSize` and
//  this is the virtual that forwards to it.
// ---------------------------------------------------------------------- //


// ======================================================================= //
//  Arr1007c -- CMoArray<T,C> with a 4-byte T.
//
//  Get is 0x10009d40: `mov 0x4(%ecx),%ecx ; mov 0x8(%esp),%eax ;
//  lea (%ecx,%eax,4),%eax`.  GetSize for this instantiation is 0x1005f780
//  (the same `mov 0x8(%ecx),%eax` body at a different address -- the linker
//  did not fold every copy).
// ======================================================================= //
struct Arr1007c
{
	// --- declared only ---
	uint32		sub_1005f780() const;                     // GetSize
	void		sub_1007c9d0(uint32 nIndex);              // Remove
	uint32&		sub_10009d40(uint32 nIndex) const;        // Get

	// --- written here ---
	void		sub_1007c690(GenListPos pos);             // GenRemoveAt
	uint32		sub_1007c700(const GenListPos& pos) const;// GenIsValid
	uint32		sub_1007c6d0(GenListPos& pos) const;      // GenGetNext
};


// 0x1007c690 -- 28 bytes.  GenRemoveAt; `ret $0x8`, one dword forwarded.


// ---------------------------------------------------------------------- //
//  0x1007c700 -- 35 bytes.  `virtual BOOL GenIsValid(const GenListPos &pos)
//  const {return pos.m_Index < GetSize();}`
//
//  WHAT PINS IT: GetSize is called FIRST, into %eax, and only then is the
//  argument dereferenced -- `mov (%edx),%edx ; cmp %eax,%edx ; setb %cl`.
//  `setb` (unsigned below) says both sides are unsigned, and the result is
//  widened through a zeroed %ecx into %eax, so the return is a 32-bit BOOL
//  and not a `bool`.
// ---------------------------------------------------------------------- //


// ---------------------------------------------------------------------- //
//  0x1007c6d0 -- 40 bytes.  GenGetNext over the 4-byte T; the same
//  `incl (%ebx)` between the Get call and `mov (%eax),%eax` that pins
//  0x1001dc00.  It differs from 0x1001dc00 only in spilling %ebx into the
//  frame (`mov %ebx,(%esp)`) instead of pushing it, and in the width of the
//  final load.
// ---------------------------------------------------------------------- //
uint32 Arr1007c::sub_1007c6d0(GenListPos& pos) const
{
	uint32& ret = sub_10009d40(pos.m_Index);
	++pos.m_Index;
	return ret;
}


// ======================================================================= //
//  Arr1007e -- a SECOND CMoArray instantiation over a 16-byte T.  Its
//  Append (0x1007f390) forwards to Insert 0x1007f360, not to 0x1000a600, so
//  it is not Arr1005a however similar the element size.
// ======================================================================= //
struct T1007e
{
	uint8	m_Data[16];
};

struct Arr1007e
{
	void*		m_pVTable;			// +0x00
	T1007e*		m_pArray;			// +0x04
	uint32		m_nElements;		// +0x08

	uint32		sub_1007f360(uint32 nIndex, const T1007e& toInsert);  // Insert

	uint32		sub_1007f390(const T1007e& toAdd);        // Append
	uint32		sub_1007ee20(T1007e& toAppend);           // GenAppend
};

// 0x1007f390 -- 35 bytes.  Append.

// 0x1007ee20 -- 28 bytes.  GenAppend, forwarding to the Append above.


// ======================================================================= //
//  Arr10050 -- CMoArray<T,C> with a 12-byte T.
//
//  Get is 0x10050790: `lea 0x0(,%eax,8),%edx ; lea (%edx,%eax,4),%eax ;
//  add 0x4(%ecx),%eax` -- 8i + 4i = 12i.
// ======================================================================= //
struct T10050
{
	uint8	m_Data[12];
};

struct Arr10050
{
	T10050&		sub_10050790(uint32 nIndex) const;        // Get
	T10050&		sub_100507e0(uint32 nIndex) const;        // operator[]
};

// 0x100507e0 -- 28 bytes.  operator[]; the same 28-byte forward as
// 0x1001c8b0, differing only in which Get it reaches.


// ======================================================================= //
//  0x10050dc0 -- 28 bytes.  ONE MORE of the 28-byte member forwarders, but
//  this one is not a dynarray member: 0x10050d60 is a real body that takes a
//  pointer, calls 0x1000a6e0 twice on `arg+0x10` and `arg+0x1c`, and copies
//  `this->m_1c` into `*arg`.  All that is decidable from 0x10050dc0's own
//  bytes is the shape: __thiscall, one dword in, forwarded unchanged, and
//  the callee's answer left in %eax.
// ======================================================================= //
struct C10050dc0
{
	void	sub_10050d60(void* pParam);                   // declared only
	void	sub_10050dc0(void* pParam);
};



// ======================================================================= //
//  THE PLAIN ACCESSORS
// ======================================================================= //

// ---------------------------------------------------------------------- //
//  0x100158a0 -- 8 bytes: `56 8b 41 1c 83 c4 04 c3`.
//  __thiscall, no arguments (bare `ret`), returns the dword at +0x1c.  The
//  `push %esi` / `add $0x4,%esp` pair is icl's dead four-byte slot, not a
//  register save -- %esi is never restored.  Its immediate neighbours are
//  `lea 0x18c(%ecx),%eax` and `mov 0x1dc(%ecx),%eax`, i.e. this is one of a
//  run of accessors on a large object.
// ---------------------------------------------------------------------- //
struct C100158a0
{
	uint8		m_pad_00[0x1c];
	uint32		m_nValue1c;			// +0x1c

	uint32		sub_100158a0() const;
};



// ---------------------------------------------------------------------- //
//  0x1004e340 -- 17 bytes.  `mov 0x38(%ecx),%ecx ; mov 0x8(%esp),%eax ;
//  mov (%ecx,%eax,4),%eax ; ret $0x4` -- index a dword array whose base
//  lives at +0x38.  The base is loaded BEFORE the index, which is the order
//  `return m_pTable[nIndex];` gives.
// ---------------------------------------------------------------------- //
struct C1004e340
{
	uint8		m_pad_00[0x38];
	uint32*		m_pTable;			// +0x38

	uint32		sub_1004e340(uint32 nIndex) const;
};



// ---------------------------------------------------------------------- //
//  0x1004e3a0 -- 34 bytes.  A bit test on the dword at +0x204.
//
//      xor %edx,%edx ; inc %edx        edx = 1
//      mov %ecx,%eax                   this out of the way
//      mov 0x8(%esp),%ecx              the shift count, into %cl
//      mov 0x204(%eax),%eax
//      shl %cl,%edx
//      test %eax,%edx
//      mov $0x0,%eax ; setne %al       <- the FIVE-byte `mov $0`, not `xor`,
//                                         because the flags are live
//
//  The full 32 bits of %eax are cleared before `setne`, so the result is a
//  32-bit 0/1 -- an LTBOOL, not a `bool`.
// ---------------------------------------------------------------------- //
struct C1004e3a0
{
	uint8		m_pad_00[0x204];
	uint32		m_nFlags;			// +0x204

	LTBOOL		sub_1004e3a0(uint32 nBit) const;
};



// ---------------------------------------------------------------------- //
//  0x1006c6c0 -- 7 bytes: `56 33 c0 83 c4 04 c3`.  __thiscall, no
//  arguments, `return 0`.  (Its neighbour at 0x1006c6d0 is the same body
//  returning 0x40, so this is a run of constant-returning virtuals.)
// ---------------------------------------------------------------------- //
struct C1006c6c0
{
	uint32		sub_1006c6c0() const;
};



// ---------------------------------------------------------------------- //
//  0x1003aba0 -- 10 bytes: `56 b8 01 00 00 00 83 c4 04 c3`.  `return 1`,
//  spelled with the five-byte `mov $0x1,%eax` rather than `xor/inc` -- see
//  the flag warning in the file header.
// ---------------------------------------------------------------------- //
struct C1003aba0
{
	LTBOOL		sub_1003aba0() const;
};

LTBOOL C1003aba0::sub_1003aba0() const
{
	return 1;
}


// ---------------------------------------------------------------------- //
//  0x1003abb0 -- 12 bytes.  `mov 0x18(%ecx),%ecx ; movzwl 0x40(%ecx),%eax`
//  -- one pointer hop then a 16-bit field, ZERO-EXTENDED into 32 bits, so
//  the return type is wider than the field.  Its two neighbours at
//  0x1003abc0 and 0x1003abd0 are the identical body over +0x28 and +0x2a,
//  and the constructor at 0x1003abe0 initialises exactly +0x28, +0x2a,
//  +0x40, +0x42 and +0x44 as words -- which is what confirms the widths.
// ---------------------------------------------------------------------- //
struct Sub1003abb0
{
	uint8		m_pad_00[0x40];
	uint16		m_nValue40;			// +0x40
};

struct C1003abb0
{
	uint8			m_pad_00[0x18];
	Sub1003abb0*	m_pSub;			// +0x18

	uint32			sub_1003abb0() const;
};



// ---------------------------------------------------------------------- //
//  0x100037b0 -- 24 bytes.  A two-way constant select on a pointer:
//
//      mov 0x28(%ecx),%ecx ; test %ecx,%ecx
//      je  L ; mov $0x1c,%eax ; jmp E
//      L:    mov $0x20,%eax
//      E:    add $0x4,%esp ; ret
//
//  ONE shared epilogue reached by a `jmp` from the first arm -- the
//  conditional-expression shape, not two returns.  Its neighbour at
//  0x100037d0 is the identical body yielding 0x24 / 0x28, and 0x100037f0
//  builds a descriptor whose +0x0c is likewise 0x1c2 or 0x1c4 depending on
//  the same +0x28 pointer; so these are "size of the vertex format, with or
//  without the extra field".
// ---------------------------------------------------------------------- //
struct C100037b0
{
	uint8		m_pad_00[0x28];
	void*		m_pExtra;			// +0x28

	uint32		sub_100037b0() const;
};



// ======================================================================= //
//  THE REST
// ======================================================================= //

// ---------------------------------------------------------------------- //
//  0x1001c1b0 -- 17 bytes.  A __cdecl free function with no parameters that
//  invokes a __thiscall member on a fixed global:
//
//      push %ebp ; mov %esp,%ebp
//      mov $0x1008b090,%ecx        <- `this`, an address, not a load
//      call 0x10016234
//      mov %ebp,%esp ; pop %ebp ; ret
//
//  WHAT PINS IT: nothing is pushed and no outgoing area is reserved, so the
//  callee takes no stack arguments; the bare `ret` says the caller takes
//  none either.  (0x10016234 is four bytes past 0x10016230's
//  `mov 0x4(%esp),%eax` front; the real body reads `this` from %ecx.)
// ---------------------------------------------------------------------- //
struct C1008b090
{
	void	sub_10016234();                               // declared only
};

extern C1008b090 g_1008b090;



// ---------------------------------------------------------------------- //
//  0x10040dd0 -- 58 bytes.  A guarded two-argument call on a sub-object of
//  the module's context block.
//
//      mov 0x100893dc,%eax ; test %eax,%eax
//      jne  L                       <- the BODY is the branch target and the
//      <epilogue> ; ret                early return is the fall-through,
//      L: call 0x1007b8d0              which is the `if(!x) return;` shape
//         add $0x244,%eax
//         movl $0x1008b880,(%esp)
//         movl $0x10040cc0,0x4(%esp)  <- a code address: a callback
//         call 0x1007b8e0
//
//  0x1007b8d0 and 0x1007b8e0 are already pinned by d3dr2d.cpp / d3dr3a.cpp:
//  0x1007b8d0 returns the address of the one big context object and
//  0x1007b8e0 is `sub_1007b8e0(void *pParam, void (*pfn)())` __thiscall on
//  a sub-object of it -- here the one at +0x244.
// ---------------------------------------------------------------------- //
struct Ctx1007b8d0Sub
{
	void	sub_1007b8e0(void* pParam, void (*pfn)());    // declared only
};

struct Ctx1007b8d0
{
	uint8			m_pad_000[0x244];
	Ctx1007b8d0Sub	m_Sub244;			// +0x244
};

extern Ctx1007b8d0* sub_1007b8d0();
extern uint32 g_100893dc;
extern uint8 g_1008b880[];
extern void sub_10040cc0();

void sub_10040dd0()
{
	if(!g_100893dc)
		return;

	sub_1007b8d0()->m_Sub244.sub_1007b8e0(g_1008b880, sub_10040cc0);
}


// ---------------------------------------------------------------------- //
//  0x1007ca60 -- 84 bytes.  A guarded forward whose two arguments are dug
//  out of two different member chains.
//
//      call 0x1007cab4              this->sub_1007cab4()
//      test %eax,%eax ; jne L
//      xor %eax,%eax                return 0   <- shared epilogue lives HERE
//      L: mov 0x1a4(%ebx),%eax
//         mov 0x2c0(%eax),%eax
//         mov 0xe4(%ebx),%edx
//         movzwl 0x10(%eax),%eax    a WORD field, widened
//         call 0x1007cc94 (word, m_e4)
//         xor %eax,%eax ; inc %eax  return 1
//         jmp back to the epilogue
//
//  WHAT PINS THE SHAPE: the `return 0` is the FALL-THROUGH and the body is
//  the branch target, so the source tests the negation first and returns
//  early -- `if(!f()) return LTFALSE;`.  Argument order is settled by the
//  slots: the widened word goes to (%esp) and m_e4 to 0x4(%esp).
// ---------------------------------------------------------------------- //
struct Obj1007ca60_2c0
{
	uint8		m_pad_00[0x10];
	uint16		m_nWord10;			// +0x10
};

struct Obj1007ca60_1a4
{
	uint8				m_pad_000[0x2c0];
	Obj1007ca60_2c0*	m_p2c0;			// +0x2c0
};

struct C1007ca60
{
	uint8				m_pad_000[0xe4];
	uint32				m_nValuee4;			// +0x0e4
	uint8				m_pad_0e8[0x1a4 - 0xe8];
	Obj1007ca60_1a4*	m_p1a4;				// +0x1a4

	uint32		sub_1007cab4();                           // declared only
	void		sub_1007cc94(uint32 nWord, uint32 nValue);// declared only

	LTBOOL		sub_1007ca60();
};



// ---------------------------------------------------------------------- //
//  0x1007bae0 -- 50 bytes.  Reset three counters, then walk a circular
//  doubly-linked list tying off every node's payload.
//
//      mov 0x4(%ecx),%eax          m_Head.m_pNext, hoisted above the stores
//      xor %edx,%edx
//      mov %edx,0x24(%ecx) ; 0x3c(%ecx) ; 0x140(%ecx)
//      cmp %ecx,%eax ; je done     <- the list head IS `this`, so the head
//      xor %ebx,%ebx                  link sits at offset 0
//      loop: mov 0x8(%eax),%edx    pCur->m_pData
//            mov %ebx,0x4(%edx)    pCur->m_pData->m_4 = NULL
//            mov 0x4(%eax),%eax    pCur = pCur->m_pNext
//            cmp %ecx,%eax ; jne loop
//
//  WHAT PINS THE LAYOUT: ltlink.h's CheapLTLink is `LTLink *m_pPrev,
//  *m_pNext;` and LTLink adds `m_pData` -- +0x00, +0x04, +0x08 -- and every
//  offset this body touches on a node matches.  The guarded do/while is the
//  standard lowering of a `for` whose first test can fail.
// ---------------------------------------------------------------------- //
struct Node1007bae0
{
	void*		m_pPad0;			// +0x00
	void*		m_pOwner;			// +0x04  <- tied off
};

struct Link1007bae0
{
	Link1007bae0*	m_pPrev;		// +0x00
	Link1007bae0*	m_pNext;		// +0x04
	Node1007bae0*	m_pData;		// +0x08
};

struct C1007bae0
{
	Link1007bae0	m_Head;			// +0x00
	uint8			m_pad_0c[0x24 - 0x0c];
	uint32			m_nCount24;		// +0x024
	uint8			m_pad_28[0x3c - 0x28];
	uint32			m_nCount3c;		// +0x03c
	uint8			m_pad_40[0x140 - 0x40];
	uint32			m_nCount140;	// +0x140

	void		sub_1007bae0();
};

void C1007bae0::sub_1007bae0()
{
	Link1007bae0* pCur;

	m_nCount24 = 0;
	m_nCount3c = 0;
	m_nCount140 = 0;

	for(pCur=m_Head.m_pNext; pCur != &m_Head; pCur=pCur->m_pNext)
		pCur->m_pData->m_pOwner = LTNULL;
}


// ---------------------------------------------------------------------- //
//  0x1006c690 -- 36 bytes.  A scalar deleting destructor: set the vtable
//  pointer back, then free `this` if bit 0 of the flag word is set.
//
//      sub $0xc,%esp
//      mov 0x10(%esp),%eax          the flag word
//      movl $0x100894d0,(%ecx)      the destructor's own vtable store
//      test $0x1,%al                <- byte test, so only bit 0 matters
//      jne  free ; add $0xc,%esp ; ret $0x4
//      free: mov %ecx,(%esp) ; call 0x10080479 ; jmp back
//
//  0x10080479 is the CRT free, already identified in d3dbatchc.cpp.  The
//  vtable store with no other member work says the underlying destructor
//  body is empty.  NOTE: this is a compiler-generated thunk in the shipped
//  build; written here as the ordinary function that emits those bytes,
//  because the checker maps symbols to addresses by name.
// ---------------------------------------------------------------------- //
extern void* g_vtbl_100894d0[];
extern void sub_10080479(void* pMem);

struct C1006c690
{
	void*		m_pVTable;			// +0x00

	void		sub_1006c690(uint32 nFlags);
};



// ---------------------------------------------------------------------- //
//  0x1006c4a0 -- 48 bytes.  A four-argument __thiscall on a fixed global,
//  from a __cdecl function with no parameters and no return:
//
//      add $0xfffffff0,%esp                 sixteen bytes of outgoing area
//      movl $"LMAnimStatic",(%esp)
//      movl $0x0,0x4(%esp) ; %eax -> 0x8, 0xc
//      mov  $0x10091220,%ecx
//      call 0x10021f40
//
//  The string is in .rdata at 0x100ab500 and reads "LMAnimStatic", so this
//  is a registration of a named animation/tracker type against the object at
//  0x10091220.  All three trailing arguments are literal zero.
// ---------------------------------------------------------------------- //
struct C10091220
{
	void	sub_10021f40(const char* pName, uint32 a, uint32 b, uint32 c);
};

extern C10091220 g_10091220;



// ---------------------------------------------------------------------- //
//  0x10059270 -- 48 bytes.  Two scaled float copies, no frame pointer.
//
//      mov 0x8(%esp),%eax ; mov 0x4(%esp),%edx     both args before the sub
//      sub $0xc,%esp
//      flds 0x788(%ecx) ; fmuls 0x4(%edx) ; fstps (%eax)
//      mov %edi,0x4(%esp) ; mov %esi,0x8(%esp)     <- dead register slots
//      flds 0x78c(%ecx) ; fmuls 0x8(%edx) ; fstps 0x4(%eax)
//      ret $0xc
//
//  WHAT PINS IT: `ret $0xc` is THREE stack dwords but only two are ever
//  read, so the third parameter is dead -- which is normal.
//  The member scale factors are at +0x788 and +0x78c and are multiplied by
//  the source's SECOND and THIRD floats (+0x04, +0x08) into the
//  destination's FIRST and SECOND (+0x00, +0x04); the scale is the left
//  operand of each multiply because it is the one loaded with `flds`.
// ---------------------------------------------------------------------- //
struct C10059270
{
	uint8		m_pad_000[0x788];
	float		m_fScaleX;			// +0x788
	float		m_fScaleY;			// +0x78c

	void		sub_10059270(const LTVector* pIn, float* pOut, uint32 nUnused);
};



// ---------------------------------------------------------------------- //
//  0x1007dc54 -- 93 bytes.  A constructor: two vtable pointers, then three
//  separate zero-fill groups, then two literals, then `mov %ecx,%eax`.
//
//      movl $0x10089a20,(%ecx)      the object's own vptr
//      movl $0x10089a60,0x4(%ecx)   a second vptr at +0x04
//      xor %eax,%eax ; -> 0x14, 0x10, 0x0c, 0x08
//      xor %eax,%eax ; -> 0x10, 0x14, 0x0c, 0x08
//      xor %eax,%eax ; -> 0x38, 0x34, 0x30, 0x2c, 0x28, 0x1c, 0x18, 0x0c
//      movl $0x300,0x20(%ecx) ; mov %ecx,%eax ; movl $0x1,0x24(%ecx)
//
//  WHAT THE SECOND vptr MEANS: +0x04..+0x14 is a CMoArray subobject laid out
//  exactly as dynarray.h declares it -- vptr, m_pArray, m_nElements,
//  m_Cache.m_CacheSize, m_Cache.m_WantedCache -- and the FIRST zero group is
//  `Clear()` word for word: m_pArray, m_nElements, SetCacheSize(0),
//  SetWantedCache(0).  The zero constant is re-materialised three times,
//  which is what says three separate inlined bodies rather than one run of
//  statements; icl emits each body's stores in reverse of source order (the
//  first group's 0x14,0x10,0x0c,0x08 is Clear()'s ascending order backwards)
//  and the source below is written accordingly.
//
//  THIS IS THE LEAST CERTAIN FUNCTION IN THE FILE.
// ---------------------------------------------------------------------- //
extern void* g_vtbl_10089a20[];
extern void* g_vtbl_10089a60[];

struct C1007dc54
{
	void*		m_pVTable;			// +0x00
	void*		m_pArrVTable;		// +0x04
	void*		m_pArray;			// +0x08
	uint32		m_nElements;		// +0x0c
	uint32		m_nCacheSize;		// +0x10
	uint32		m_nWantedCache;		// +0x14
	uint32		m_n18;				// +0x18
	uint32		m_n1c;				// +0x1c
	uint32		m_n20;				// +0x20
	uint32		m_n24;				// +0x24
	uint32		m_n28;				// +0x28
	uint32		m_n2c;				// +0x2c
	uint32		m_n30;				// +0x30
	uint32		m_n34;				// +0x34
	uint32		m_n38;				// +0x38

	C1007dc54*	sub_1007dc54();
};

