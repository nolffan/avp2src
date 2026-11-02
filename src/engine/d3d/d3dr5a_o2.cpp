// Split out of d3dr5a.cpp: these functions need the
// OTHER optimisation flag (/O2) from the rest of that file.  d3d.ren is a
// mixed build and our file boundaries are invented.
// Everything that is not a function body is
// carried over verbatim so no declaration is lost.
// ********************************************************* //
// *********************************************************************** //
//
//  MODULE:   D3DR5A (.CPP)  --  d3d.ren "batch1"
//
//  PURPOSE:  The twenty-five SMALLEST rows of the d3d.ren
//            listing (28..86 bytes), decoded instruction by instruction
//            from the retail d3d.ren.  Intel C/C++ 4.0.
//
//  WHAT THIS BATCH IS.  It is six families and four singletons, and every
//  family was identified from a callee or a vtable rather than from shape:
//
//   A  GenList<T> vtable slots (five rows).  The four twelve-entry pointer
//      runs d3dr4b.cpp named -- 0x10089260, 0x10089690, 0x100897a0,
//      0x100899f0 -- place every one of them by SLOT, and the slot's body
//      is then the one-liner in src/LT2/lithshared/stdlith/dynarray.h:
//
//        0x1001dbb0  slot +0x10  GenAppend   -> Append 0x1001e040
//        0x1005a2f0  slot +0x10  GenAppend   -> Append 0x1005a3e0
//        0x1001c720  slot +0x08  GenGetNext  -> Get 0x1001c790  (T = 64)
//        0x1001d200  slot +0x08  GenGetNext  -> Get 0x10009ef0  (T = 32)
//        0x10050730  slot +0x08  GenGetNext  -> Get 0x10050790  (T = 12)
//
//      0x10089690 reads slot-for-slot
//        [0x10050770 0x1007c700 0x10050730 0x100506f0 0x100506d0
//         0x100506b0 0x10050690 0x10050670 0x10050560 0x10050450
//         0x10050410 0x100503f0]
//      and 0x100897a0
//        [0x1001d240 0x1001c760 0x1005a350 0x1005a310 0x1005a2f0
//         0x1005a2d0 0x1005a2b0 0x1001db50 0x1005a1d0 0x1005a0f0
//         0x1005a0b0 0x1001cf40],
//      i.e. index 2 (GenGetNext) and index 4 (GenAppend) of runs whose
//      other ten entries d3dr4b.cpp already reconstructed under those
//      names.  That is the whole proof; nothing here is shape-guessed.
//
//   B  Console-variable registration (five rows).  0x10021f40 is the
//      registrar d3dr4c.cpp pinned at 0x10068c50 -- a __thiscall with
//      `ret $0x10` that writes its four arguments to +0x14, +0x08, +0x0c,
//      +0x10 of the bank and links it onto the list head at 0x1008bf90.
//      Three of these rows are literally TWO of d3dr4c's 48-byte function
//      back to back (48 + 38 = 86), one is a sibling constructor followed
//      by one registration, one is four global stores followed by one.
//
//   C  "Set '%s' overflowed" (four rows).  Four copies of one source
//      function over four 0x20-byte records of the singleton at
//      0x10091a10 (0x1007b8d0 is `mov $0x10091a10,%eax ; ret`, already
//      pinned by d3dr2a/d3dr3a/d3dr4c).  The record is
//      +0x00 elements, +0x04 count, +0x14 max, +0x18 name, and the four
//      rows sit at 0x264, 0x2a4, 0x2c4, 0x2e4 -- a 0x20 stride, which is
//      how the layout was read.
//
//   D  Deleting destructors over the hash at +0x04 (three rows).
//      0x1007fb20 is `Init(nBuckets, nSize)` (it clamps arg2 to >= 4,
//      rounds it up to a multiple of 4, stores arg3 at +0x08 and clears
//      +0x0c..+0x18) and 0x1007fba0 is its `Clear` (walks the chain at
//      +0x14 freeing through 0x10021cb0, then clears the same four
//      dwords).  Both are __cdecl with the object as the first STACK
//      argument -- 0x1007fba0 reads 0xc(%esp) after two pushes and never
//      touches %ecx -- so they are free functions, not members.
//
//   E  CMoArray<T,C>::Term() inlined (two rows).  This one is settled by
//      the header, not by shape.  dynarray.h line 103 is
//          void Term(LAlloc *pAlloc=&g_DefAlloc) { SetSize2(0, pAlloc); }
//      SetSize2 (line 657) is
//          if((newSize == m_nElements) && (newSize != 0)) return TRUE;
//          if( m_pArray ) _DeleteAndDestroyArray(pAlloc, ...);
//          m_nElements = newSize;
//          if( newSize > 0 ) { ... }
//      _DeleteAndDestroyArray (line 805) is
//          if(m_pArray) { LDelete_Array(pAlloc, m_pArray, n); m_pArray=NULL; }
//          m_Cache.SetCacheSize(0);
//      and LDelete_Array -> BaseDelete (l_allocator.h line 223) opens with
//          if(!ptr) return;
//      THREE tests of m_pArray, nested, each guarding one more store --
//      which is exactly the three conditional branches the shipped code
//      hangs off ONE `test %eax,%eax`:
//          test ; jne A ; <tail> ; A: jne B ; m_48=0 ; jmp tail
//          B: mov $g_DefAlloc,%ecx ; je C ; Free ; C: m_40=0 ; jmp A+2
//      The same figure appears verbatim at 0x1007def4 (offsets 0x08/0x10)
//      and 0x1007dcc4 (0x08/0x10/0x0c), so it is one inline chain used in
//      four places, and the element offsets say where the subobject is:
//      m_pArray at +0x40 puts the CMoArray at +0x3c -- and 0x1007e850's
//      `movl $0x10089a90,0x3c(%ebx)` is that subobject's own vptr.
//
//   F  Node-list singletons (two rows).  0x100362e0 is a recursive
//      `Find(key)` (`if(key==m_Key) return this; if(!m_pNext) return NULL;
//      return m_pNext->Find(key);`) and 0x10035ab0 is a scalar deleting
//      destructor (`sub_100362a4() ; if(flags&1) operator delete(this)`).
//      0x10036420 inlines ONE level of Find and then deletes the hit;
//      0x10035b40 is the same class's own deleting destructor.
//
//  ADDRESSES WITH A +4 REGISTER ENTRY.  0x1003b920, 0x100362e0, 0x1007dcc0
//  and 0x1007def0 each begin `mov 0x4(%esp),%eax` and fall through into a
//  second entry four bytes on; the shipped calls from inside their own TU
//  go to the +4 entry and calls from outside go to +0.  We cannot choose,
//  and we do not have to: the byte comparison masks the four bytes of every REL32
//  relocation, so a call displacement is free.  Where the body needed is
//  at +4 the DECLARATION is named for +4 so a future reconstruction of
//  it lands on the right address.
//
//  ONE ROW IS THE WRONG SIZE IN THE LISTING.  0x10035b40 is listed at 81
//  bytes; its first `ret $0x4` is at 0x10035b8e but the body runs to the
//  `jmp 0x10035b5b` at 0x10035ba7, i.e. 105 bytes.  81 is the distance to
//  0x10035b91, which the function enumeration took for a start and which is
//  really the `operator delete` arm.  Reconstructed at its true length.
//
//  ALL TWENTY-FIVE ROWS ARE FUNCTIONS.  None of them is an interior branch
//  target or an operand byte.
//
//  FLAG SPLIT -- THIS FILE DISAGREES WITH ITSELF, and the flag-tell table is
//  what says so.  Five addresses want /O2 and the other twenty want /Os:
//
//    /O2   0x10050730  outgoing arg by `mov`, and `mov (%ebx),%edx ;
//                      inc %edx ; mov %edx,(%ebx)` where its two /Os
//                      twins use `incl (%eax)`
//          0x100374d0  DUPLICATES the epilogue at its second exit
//                      (0x10037508..0x10037516) where 0x10050cc0, byte for
//                      byte the same function, cross-jumps (`eb ef`)
//          0x10042d40  duplicates the epilogue where 0x1003b790 /
//          0x1005d960  0x100508a0, the same source, cross-jump (`eb 1e`)
//          0x1007b520  `mov $0x1,%eax` for `return 1`, not `xor/inc`
//
//    /Os   everything else.  The strongest single witnesses:
//          0x1001dbb0 / 0x1005a2f0 are byte-identical to d3dr4b.cpp's
//          0x1001c6c0, and 0x1001c720 / 0x1001d200 are d3dr4c.cpp's
//          0x1001d1c0 plus the post-increment -- both of those files are
//          already on the /Os list and match there.
//          0x10059f70 has `xor %eax,%eax ; inc %eax`.
//          0x10003770, 0x10036420, 0x10035b40, 0x1007e270, 0x1007e850,
//          0x10050cc0 all cross-jump to a single epilogue.
//
//    NOT a flag tell, in this image: `add $-N,%esp` + `mov` for outgoing
//    arguments.  d3dr4c.cpp is /Os and its 0x10068c50 uses it, and so do
//    0x1003d910 / 0x10047b00 / 0x10050330 / 0x10050c60 / 0x1005f730,
//    which are the same function.  Only the epilogue-duplication and the
//    `mov $1` / `xor;inc` levers separated anything here.
//
// *********************************************************************** //

#include "ltbasedefs.h"


// ======================================================================= //
//  SHARED DECLARATIONS
// ======================================================================= //

// ---------------------------------------------------------------------- //
//  0x10091df0 -- stdlith's `g_DefAlloc` (l_allocator.h), declared the way
//  d3dbatchb.cpp / d3dr2b.cpp / d3dr4b.cpp declare it and never defined,
//  so no vtable for it is emitted here.  The shipped free is
//      mov $0x10091df0,%ecx ; mov (%ecx),%edx ; push <ptr>
//      mov 0x8(%edx),%eax   ; call *%eax
//  -- slot +0x08, reached through the object and NOT devirtualised.
// ---------------------------------------------------------------------- //
struct IAllocator
{
	virtual void*	Slot00(uint32 nSize);               // vtable +0x00
	virtual void*	Alloc(uint32 nSize);                // vtable +0x04
	virtual void	Free(void* pMem);                   // vtable +0x08
};

extern IAllocator g_10091df0;


// ---------------------------------------------------------------------- //
//  The CRT free reached as `operator delete` -- 0x10080479, identified in
//  d3dbatchc.cpp and used as such by d3dr4a_o2.cpp / d3dr4c.cpp.
// ---------------------------------------------------------------------- //
extern void sub_10080479(void* pMem);


// ---------------------------------------------------------------------- //
//  genlist.h's GenListPos.  Two dwords and plain old data, exactly as
//  d3dr4b.cpp establishes it.
// ---------------------------------------------------------------------- //
struct GenListPos
{
	uint32	m_Index;                                    // +0x00
	uint32	m_SubIndex;                                 // +0x04
};


// ---------------------------------------------------------------------- //
//  0x10021f40 -- the console-variable registrar, pinned by d3dr4c.cpp's
//  0x10068c50.  Declared with the identical signature so the two files
//  agree about it.
// ---------------------------------------------------------------------- //
struct Bank10021f40
{
	void	sub_10021f40(const char* pszName, float fValue,
	                     uint32 nA, uint32 nB);         // 0x10021f40
};


// ---------------------------------------------------------------------- //
//  0x10021d60 -- the renderer's two-argument-plus warning.  d3dr3b.cpp
//  names it `d3d_Warning`; its body is
//      if (nLevel > g_1008caac) return;
//      vsprintf-into-0x100 buffer ; (*0x1008cbcc)->Slot14(buf)
//  and the "%s" it is handed here makes the site varargs.
// ---------------------------------------------------------------------- //
extern void sub_10021d60(uint32 nLevel, const char* pszMsg, ...);


// ======================================================================= //
//  FAMILY A1 -- GenList<T>::GenAppend, slot +0x10
//
//      virtual BOOL GenAppend(T &toAppend) { return Append(toAppend); }
//
//  Twenty-eight bytes each:
//
//      55 8b ec / 83 ec 10          frame
//      57                           the dead four-byte slot
//      8b 45 08 / 89 04 24          the reference forwarded as one dword
//      8b c1 / 8b c8                the %ecx round trip
//      e8 <Append>
//      8b e5 / 5d / c2 04 00
//
//  WHAT PINS IT: the slot (index 4 of 0x100897a0 for 0x1005a2f0), and the
//  callee being the two-argument-Insert wrapper `Append` in each case --
//  0x1005a3e0 is `mov 0x8(%eax),%edx ; <arg> ; call 0x1000a600`, i.e.
//  `Insert(m_nElements, toAdd)`, which is the same body d3dr4b.cpp wrote
//  for 0x1001cce0 / 0x1001e040 / 0x1007ca00.  0x1001dbb0's callee IS
//  0x1001e040, which d3dr4b.cpp has already reconstructed.
// ======================================================================= //

// ---- 0x1001dbb0 -> Append 0x1001e040.
struct Arr1001dbb0
{
	int32	sub_1001e040(const void* pToAdd);           // Append
	int32	sub_1001dbb0(void* pToAppend);              // GenAppend
};



// ---- 0x1005a2f0 -> Append 0x1005a3e0.
struct Arr1005a2f0
{
	int32	sub_1005a3e0(const void* pToAdd);           // Append
	int32	sub_1005a2f0(void* pToAppend);              // GenAppend
};



// ======================================================================= //
//  FAMILY A2 -- GenList<T>::GenGetNext, slot +0x08
//
//  dynarray.h line 253, verbatim:
//
//      virtual T GenGetNext(GenListPos &pos) const
//      {
//          T& ret = Get(pos.m_Index);
//          ++pos.m_Index;
//          return ret;
//      }
//
//  and the shipped code is that in source order:
//
//      8b 5d 0c / 8b 03 / 50        push pos.m_Index
//      8b c1 / 8b c8 / e8 <Get>
//      8b f0                        %esi = the returned T*
//      b9 <sizeof(T)/4> 
//      8b c3                        %eax = &pos, freeing %ebx
//      8b 5d 08                     %ebx = the hidden return pointer
//      ff 00                        incl (%eax)      <- ++pos.m_Index
//      8b fb / f3 a5                copy the element out
//      8b c3 / 5b 5e 5f / 8b e5 / 5d / c2 08 00
//
//  WHAT PINS IT AGAINST GenGetAt: the `incl`.  d3dr4c.cpp's 0x1001d1c0 is
//  this function's GenGetAt twin over the same 32-byte element and the
//  same Get (0x10009ef0) and is fifty bytes; 0x1001d200 is fifty-four, and
//  the four extra bytes are `mov %ebx,%eax` + `incl (%eax)`.  The `ret $8`
//  is the hidden return-value pointer at 0x8(%ebp) plus `pos` by reference
//  at 0xc(%ebp), and sizeof(T) is read straight off the copy: %ecx = 0x10
//  (64 bytes), %ecx = 8 (32 bytes), three movs (12 bytes -- below icl's
//  return-copy threshold, as d3dr4b.cpp measured at 0x100506f0).
// ======================================================================= //

// ---- 0x1001c720 -- T is 64 bytes; Get is 0x1001c790 (`shl $0x6`).
struct T1001c720 { uint32 m_v[16]; };

struct Arr1001c720
{
	T1001c720&	sub_1001c790(uint32 nIndex) const;  // Get
	T1001c720	sub_1001c720(GenListPos& pos) const;
};



// ---- 0x1001d200 -- T is 32 bytes; Get is 0x10009ef0 (`shl $0x5`).
struct T1001d200 { uint32 m_v[8]; };

struct Arr1001d200
{
	T1001d200&	sub_10009ef0(uint32 nIndex) const;  // Get
	T1001d200	sub_1001d200(GenListPos& pos) const;
};



// ---- 0x10050730 -- T is 12 bytes; Get is 0x10050790
//      (`lea 0x0(,%eax,8) ; lea (%edx,%eax,4) ; add 0x4(%ecx)`).
//      THIS ROW WANTS /O2: the argument goes out by `mov %eax,(%esp)` and
//      the increment is a register read-modify-write.
struct T10050730 { uint32 m_v[3]; };

struct Arr10050730
{
	T10050730&	sub_10050790(uint32 nIndex) const;  // Get
	T10050730	sub_10050730(GenListPos& pos) const;
};

T10050730 Arr10050730::sub_10050730(GenListPos& pos) const
{
	T10050730& ret = sub_10050790(pos.m_Index);

	++pos.m_Index;

	return ret;
}


// ======================================================================= //
//  FAMILY B -- console-variable registration
//
//  d3dr4c.cpp's 0x10068c50 is the 48-byte unit:
//
//      55 8b ec / 83 ec 10          frame
//      83 c4 f0                     a 16-byte outgoing area
//      33 c0                        the shared integer zero
//      c7 44 24 04 <bits>           argument 2, a float literal, as its
//                                   own immediate -- NOT the zero register
//                                   even when it is 0.0f, which is what
//                                   proves argument 2 is a float and
//                                   arguments 3 and 4 are integers
//      c7 04 24 <name>              argument 1
//      b9 <bank>                    `this`
//      89 44 24 08 / 89 44 24 0c    arguments 3 and 4 = 0
//      e8 <0x10021f40>
//
//  Three of these rows are exactly two of those (86 = 48 + 38, the second
//  copy sharing the frame), so the only thing to read off them is which
//  bank, which string and which float.
// ======================================================================= //

extern Bank10021f40 g_1008ee24;                         // "ModelSpecular"
extern Bank10021f40 g_1008ee04;                         // "ModelTexture"
extern Bank10021f40 g_1008ef60;                         // "SortProfile"
extern Bank10021f40 g_1008ef40;                         // "DrawSorted"
extern Bank10021f40 g_1008efb0;                         // "PSSrcBlend"
extern Bank10021f40 g_1008ef90;                         // "PSDestBlend"
extern Bank10021f40 g_1008ed80;                         // "DrawCanvases"
extern Bank10021f40 g_1008ff40;                         // "AllSkyPortals"


// ----------------------------------------------------------------------- //
//  0x10047b00   86 bytes.  1.0f (0x3f800000) twice.
// ----------------------------------------------------------------------- //


// ----------------------------------------------------------------------- //
//  0x10050330   86 bytes.  The first float is 0x00000000 and it is still
//  written with its own `movl $0x0,0x4(%esp)` while arguments 3 and 4 come
//  out of the already-zeroed %eax -- the float/integer split above.
// ----------------------------------------------------------------------- //


// ----------------------------------------------------------------------- //
//  0x10050c60   86 bytes.  0xbf800000 twice, i.e. -1.0f.
// ----------------------------------------------------------------------- //


// ----------------------------------------------------------------------- //
//  0x1003d910   58 bytes.  One no-argument __thiscall on a second bank
//  first, then the registration.
//
//      b9 <0x1008eda0> / e8 <0x1003b924>
//      83 c4 f0 / 33 c0 / ... / e8 <0x10021f40>
//
//  0x1003b920 is a sibling constructor -- `movl $0x10089530,(%ebx)` then
//  `memset(this+0x2c, 0, 0x20)` through 0x100809a0 -- so the call takes no
//  arguments of its own, which is all this row needs to know about it.
//  DECLARED AT +4 because that is the entry the shipped call reaches; the
//  displacement is a masked relocation either way.
// ----------------------------------------------------------------------- //
struct Bank1003b920
{
	void	sub_1003b924();                             // 0x1003b920 + 4
};

extern Bank1003b920 g_1008eda0;



// ----------------------------------------------------------------------- //
//  0x1005f730   68 bytes.  Four dword stores of the SHARED zero register
//  before the registration:
//
//      33 c0 / 83 c4 f0
//      a3 88 ff 08 10 / a3 84 ff 08 10 / a3 80 ff 08 10 / a3 7c ff 08 10
//      c7 44 24 04 00000000    <- and the float argument, separately
//      ...
//
//  The four use %eax and the float does not, so the four are integers.
//  Store order is descending, 0x1008ff88 down to 0x1008ff7c, and that is
//  the order written here; see the COIN-FLIP note at the end of the file.
// ----------------------------------------------------------------------- //
extern uint32 g_1008ff88;
extern uint32 g_1008ff84;
extern uint32 g_1008ff80;
extern uint32 g_1008ff7c;



// ======================================================================= //
//  FAMILY C -- "Set '%s' overflowed"
//
//      53 / 83 ec 10                %ebx, then a 16-byte outgoing area
//      e8 <0x1007b8d0>              the singleton accessor
//      8b 90 <count> / 8b 88 <max>  both fields
//      3b d1                        cmp count,max  -> `count < max`
//      73 <overflow>
//      8b 88 <elements>
//      8b 5c 24 18                  the parameter
//      89 1c 91                     pElements[count] = p
//      ff 80 <count>                count++
//      <exit>
//    overflow:
//      c7 44 24 04 <"Set '%s' overflowed">
//      8b 80 <name>
//      c7 04 24 01000000            level 1
//      89 44 24 08
//      e8 <0x10021d60>
//      <exit>
//
//  WHAT PINS IT: `jae`, not `jge`, so both fields are unsigned; the
//  destination of the `cmp` is the count, so the count is the source's
//  LEFT operand and the guard reads `m_nCount < m_nMax`; and the success
//  arm falls through with the warning jumped to, which is the if/else the
//  arm-order rule describes.  The record layout comes from the
//  four rows agreeing on a 0x20 stride:
//      0x264/0x268/0x278/0x27c   0x2a4/0x2a8/0x2b8/0x2bc
//      0x2c4/0x2c8/0x2d8/0x2dc   0x2e4/0x2e8/0x2f8/0x2fc
//  i.e. +0x00 elements, +0x04 count, +0x14 max, +0x18 name.
//
//  AND IT IS THE FLAG WITNESS OF THIS BATCH.  0x1003b790 and 0x100508a0
//  cross-jump out of the success arm (`eb 1e`) and are 81 bytes;
//  0x10042d40 and 0x1005d960 duplicate `add $0x10,%esp ; pop %ebx ; ret`
//  instead and are 84.  Same source, two flags, three bytes apart.
// ======================================================================= //
struct Set1007b8d0                                      // 0x20 bytes
{
	void**	m_pElements;                                // +0x00
	uint32	m_nCount;                                   // +0x04
	uint8	m_Reserved08[0x0c];                         // +0x08..+0x13
	uint32	m_nMax;                                     // +0x14
	char*	m_pszName;                                  // +0x18
	uint32	m_n1c;                                      // +0x1c
};

// The registrar at +0x184 (0x10043a30 below reads its count and calls its
// member 0x1007b8e0; d3dr2b.cpp's twins reach it through the same object).
struct Reg10043a30
{
	void	sub_1007b8e0(void* pObj, void (*pfnCallback)());

	void*	m_pElements;                                // +0x00
	uint32	m_nCount;                                   // +0x04
};

struct Root1007b8d0
{
	uint8		m_Reserved000[0x184];                   // +0x000..+0x183
	Reg10043a30	m_Reg184;                               // +0x184
	uint8		m_Reserved18c[0x264 - 0x18c];           // +0x18c..+0x263
	Set1007b8d0	m_Set264;                               // +0x264
	Set1007b8d0	m_Set284;                               // +0x284
	Set1007b8d0	m_Set2a4;                               // +0x2a4
	Set1007b8d0	m_Set2c4;                               // +0x2c4
	Set1007b8d0	m_Set2e4;                               // +0x2e4
};

extern Root1007b8d0* sub_1007b8d0();                    // 0x1007b8d0


// ---- 0x1005d960 -- the set at +0x264.  /O2 (duplicated epilogue).
void sub_1005d960(void* pItem)
{
	Root1007b8d0* pRoot = sub_1007b8d0();

	if (pRoot->m_Set264.m_nCount < pRoot->m_Set264.m_nMax)
	{
		pRoot->m_Set264.m_pElements[pRoot->m_Set264.m_nCount] = pItem;
		pRoot->m_Set264.m_nCount++;
	}
	else
	{
		sub_10021d60(1, "Set '%s' overflowed", pRoot->m_Set264.m_pszName);
	}
}


// ---- 0x10042d40 -- the set at +0x2a4.  /O2 (duplicated epilogue).
void sub_10042d40(void* pItem)
{
	Root1007b8d0* pRoot = sub_1007b8d0();

	if (pRoot->m_Set2a4.m_nCount < pRoot->m_Set2a4.m_nMax)
	{
		pRoot->m_Set2a4.m_pElements[pRoot->m_Set2a4.m_nCount] = pItem;
		pRoot->m_Set2a4.m_nCount++;
	}
	else
	{
		sub_10021d60(1, "Set '%s' overflowed", pRoot->m_Set2a4.m_pszName);
	}
}


// ---- 0x100508a0 -- the set at +0x2c4.  /Os (cross-jumped exit).


// ---- 0x1003b790 -- the set at +0x2e4.  /Os (cross-jumped exit).


// ======================================================================= //
//  FAMILY D -- the hash at +0x04, its Init and its two destructors
//
//  0x1007fb20 and 0x1007fba0 are __cdecl free functions whose first STACK
//  argument is the object:
//
//    0x1007fb20  mov 0x8(%esp),%ecx ; mov 0x4(%esp),%eax
//                cmp $0x4,%ecx ; mov %ecx,(%eax) ; mov %ecx,0x4(%eax)
//                jae .. ; movl $0x4,0x4(%eax)          clamp to >= 4
//                <round 0x4(%eax) up to a multiple of 4>
//                mov 0xc(%esp),%edx ; xor %ecx,%ecx
//                mov %ecx,0xc(%eax) ; mov %ecx,0x10(%eax)
//                mov %edx,0x8(%eax) ; mov %ecx,0x14(%eax)
//                mov %ecx,0x18(%eax)
//    0x1007fba0  push %ebx ; push %edi ; mov 0xc(%esp),%edi
//                <walk the chain at 0x14(%edi), freeing through 0x10021cb0>
//                clear 0x14, 0x18, 0x0c, 0x10
//
//  -- neither reads %ecx, so neither is a member.  Seven dwords are
//  touched (+0x00..+0x18), which is the size given to the member here.
// ======================================================================= //
struct Hash1007fba0                                     // 0x1c bytes
{
	uint32	m_v[7];
};

extern void sub_1007fba0(Hash1007fba0* pHash, uint32 nA);
extern void sub_1007fb20(Hash1007fba0* pHash, uint32 nA, uint32 nB);

extern void* g_vtbl_10089510[];                         // 0x10089510
extern void* g_vtbl_100897d0[];                         // 0x100897d0
extern void* g_vtbl_100894c0[];                         // 0x100894c0


// ----------------------------------------------------------------------- //
//  0x100375b0   61 bytes.  A constructor: no flag argument, a bare `ret`,
//  and no vtable store of its own.
//
//      53 / 83 ec 18
//      c7 44 24 04 01000000         argument 2 of the first call
//      8b d9                        %ebx = this
//      b8 04 000000 / 03 c3         %eax = this + 4       <- first &m_Hash04
//      89 04 24 / e8 <0x1007fba0>
//      83 c3 04                     %ebx = this + 4       <- second one
//      c7 44 24 04 4c000000 / 89 1c 24 / c7 44 24 08 20000000
//      e8 <0x1007fb20>
//      83 c4 18 / 5b / c3
//
//  WHAT PINS IT: the address of the member is formed TWICE, once with
//  `mov $4 ; add` and once with `add $4,%ebx` -- the catalogue's "writing
//  the same member-address expression twice makes it recompute" -- so the
//  source names `&m_Hash04` at both call sites rather than keeping a local
//  pointer.  0x4c and 0x20 are Init's two literals.
// ----------------------------------------------------------------------- //
struct C100375b0
{
	void**		m_pVTable;                              // +0x00
	Hash1007fba0	m_Hash04;                           // +0x04

	void	sub_100375b0();
};



// ----------------------------------------------------------------------- //
//  0x100374d0   71 bytes, and 0x10050cc0   58 bytes + 2 of padding.
//
//  The same scalar deleting destructor over two derived classes:
//
//      8b d9                        %ebx = this
//      b8 04 000000 / 03 c3         &m_Hash04
//      c7 03 <derived vtable>       the derived destructor's vptr store
//      <sub_1007fba0(&m_Hash04, 1)>
//      8b 44 24 18                  the flag word
//      c7 03 <0x100894c0>           the BASE destructor's vptr store
//      a8 01                        test $0x1,%al -- byte test, bit 0 only
//      jne free ; <exit> ; free: mov %ebx,(%esp) ; call 0x10080479
//
//  WHAT PINS IT: two vptr stores with only the member destruction between
//  them is a derived destructor whose base body is empty, and the flag
//  word at the single stack argument with `test $0x1,%al` is MSVC's
//  scalar-deleting-destructor thunk, written here as the ordinary function
//  that emits those bytes (as d3dr4a_o2.cpp does at 0x1006c690).
//
//  AND THEY ARE THE SECOND FLAG WITNESS: 0x100374d0 duplicates
//  `add $0x10,%esp ; pop %ebx ; ret $0x4` at 0x10037508, 0x10050cc0
//  cross-jumps to it with `eb ef`.
// ----------------------------------------------------------------------- //
struct C100374d0
{
	void**		m_pVTable;                              // +0x00
	Hash1007fba0	m_Hash04;                           // +0x04

	void	sub_100374d0(uint32 nFlags);
};

void C100374d0::sub_100374d0(uint32 nFlags)
{
	m_pVTable = g_vtbl_10089510;

	sub_1007fba0(&m_Hash04, 1);

	m_pVTable = g_vtbl_100894c0;

	if ((nFlags & 1) == 0)
		return;

	sub_10080479(this);
}


struct C10050cc0
{
	void**		m_pVTable;                              // +0x00
	Hash1007fba0	m_Hash04;                           // +0x04

	void	sub_10050cc0(uint32 nFlags);
};



// ----------------------------------------------------------------------- //
//  0x10003770   64 bytes.  The same thunk with NO vtable store, so the
//  destructor body it forwards to does all the work:
//
//      55 8b ec / 83 ec 18 / 89 1c 24    %ebx saved INTO the frame
//      8b d9 / 8b cb / e8 <0x1007dcc0>
//      8b 45 08 / a8 01 / 75 ..
//      8b 1c 24 / 8b e5 / 5d / c2 04 00
//      free: 83 c4 f8 / 89 1c 24 / e8 <0x10080479> / 83 c4 08 / eb e7
//
//  WHAT PINS IT: it is slot 0 of the pointer run at 0x10089260, whose
//  other entries (0x1007dd20, 0x1007dde0, 0x1007def0, 0x1007dfa0) are the
//  same object's members, and 0x1007dcc0 is a destructor -- it stores
//  0x10089a20 at +0x00 and 0x10089a60 at +0x04 and Terms the array at
//  +0x08.  `this` has to survive the call for the free, which is the %ebx
//  round trip.  The free's eight-byte outgoing area for one argument is
//  icl's alignment, as at 0x10035b91 where it is spelt `push`/`push`.
//
//  NEAR, AND WHY (measured): as written, 57 bytes + pad against 64, 8
//  differing -- icl at /O2 DUPLICATES the four-instruction epilogue into
//  the free arm where the shipped arm does `jmp` back (`eb e7`). None of
//  `if (f&1) free;`, `if (!(f&1)) return; free; return;`, `if (f&1) free;
//  else return;` or `if (f&1) { free; return; }` removes the duplication,
//  and no icl flag does either (swept /Ob0-2, /Og, /Oy-, /Ot, /Qunroll0,
//  /G5, /G6, /Qip, /Gy, /Oa, /Ow, /Qrcd, /Oi-, /Op; /Os cross-jumps but
//  saves %ebx with push/pop, which is not the shipped frame).
//
//  What DOES produce these 64 bytes exactly, padding included, is icl's
//  OWN scalar deleting destructor: a class with `virtual ~C();` whose
//  destructor is DEFINED in the translation unit makes icl emit
//  `??_GC@@UAEPAXI@Z` next to the vtable, and at /O2 that thunk is this
//  function byte for byte (call ??1, `test $0x1,%al`, `jne` to an
//  out-of-line `operator delete` arm that jumps BACK to the one
//  epilogue). So 0x10003770 is compiler-generated, emitted by the TU that
//  defines the destructor 0x1007dcc0 -- a two-entry /Qip body not yet
//  written. It closes when that destructor is written as a real virtual
//  ~C in a /O2 /Qip file and this hand-written stand-in is deleted.
//  Writing a stub destructor here just to get the thunk emitted would be
//  an invented function, so it is not done. The same explanation very
//  likely covers this file's "flag witnesses" (0x100374d0 duplicating
//  where 0x10050cc0 cross-jumps): cross-jump = compiler-generated.
// ----------------------------------------------------------------------- //
//  CLOSED: w57_10009860.cpp defines the four model vertex-buffer
//  banks at 0x1008ad4c.. as file-scope objects of a class whose base
//  destructor (0x1007dcc0) is `virtual`, and icl's ??_GI100039d0@@UAEPAXI@Z
//  from that TU is byte-identical here. The stand-in that stood here was
//  deleted with it.



// ======================================================================= //
//  FAMILY E -- CMoArray<T,C>::Term() inlined over the subobject at +0x3c
//
//  See the header note.  The three nested tests of m_pArray are
//  dynarray.h's `SetSize2` -> `_DeleteAndDestroyArray` -> BaseDelete's
//  `if(!ptr) return;`, and the store each one guards is, from the inside
//  out: `pAlloc->Free(m_pArray)`, `m_pArray = NULL`,
//  `m_Cache.SetCacheSize(0)`, `m_nElements = newSize`.
//
//  SetSize2's first statement -- `if((newSize == m_nElements) &&
//  (newSize != 0)) return TRUE;` -- and its trailing `if(newSize > 0)`
//  block are both provably dead at newSize == 0 and emit nothing, so they
//  are left out rather than written and relied on to fold.
// ======================================================================= //
struct DefCache1007e270
{
	uint32	m_CacheSize;                                // +0x00
	uint32	m_WantedCache;                              // +0x04

	void	SetCacheSize(uint32 n)	{ m_CacheSize = n; }
};

struct MoArray1007e270
{
	void**			m_pVTable;                      // +0x00
	void*			m_pArray;                       // +0x04
	uint32			m_nElements;                    // +0x08
	DefCache1007e270	m_Cache;                        // +0x0c

	// _DeleteAndDestroyArray, with LDelete_Array -> BaseDelete inlined
	// into it.  T has no destructor here, so BaseDelete's destruct loop
	// emits nothing and only its null test survives.
	void	_DeleteAndDestroyArray(IAllocator* pAlloc)
	{
		if (m_pArray)
		{
			if (m_pArray)
				pAlloc->Free(m_pArray);

			m_pArray = LTNULL;
		}

		m_Cache.SetCacheSize(0);
	}

	void	Term(IAllocator* pAlloc = &g_10091df0)
	{
		if (m_pArray)
			_DeleteAndDestroyArray(pAlloc);

		m_nElements = 0;
	}
};


// ----------------------------------------------------------------------- //
//  0x1007e270   75 bytes.  A __thiscall with no arguments and a bare
//  `ret`, one sibling call and then the Term:
//
//      8b d9 / 8b cb / e8 <0x1007def4>
//      8b 43 40 / 85 c0 / 75 ..            the array pointer, tested
//      33 c0 / 89 43 5c / 89 43 50 / 89 43 44
//      5b / 8b e5 / 5d / c3
//      .. jne .. ; movl $0x0,0x48(%ebx) ; jmp
//      .. mov $0x10091df0,%ecx ; je .. ; <Free> ; movl $0x0,0x40(%ebx)
//
//  WHAT PINS IT: three branches off one `test %eax,%eax`, which
//  is three tests in the source and which the header supplies; the
//  0x40/0x44/0x48 triple being m_pArray/m_nElements/m_CacheSize, which
//  puts the subobject's vptr at +0x3c -- and 0x1007e850 stores a vtable
//  there.  0x1007def0 is the object's other Term (its own copy of the same
//  three-test chain, over the array at +0x08); declared at +4, which is
//  the entry this call reaches.
// ----------------------------------------------------------------------- //
struct C1007e270
{
	uint8			m_Reserved000[0x3c];            // +0x00..+0x3b
	MoArray1007e270		m_Array3c;                      // +0x3c
	uint32			m_n50;                          // +0x50
	uint8			m_Reserved054[0x08];            // +0x54..+0x5b
	uint32			m_n5c;                          // +0x5c

	void	sub_1007def4();                             // 0x1007def0 + 4
	void	sub_1007e270();
};



// ----------------------------------------------------------------------- //
//  0x1007e850   84 bytes.  The same Term, on the fixed object at
//  0x10091d80, preceded by the subobject's vptr store and followed by one
//  __thiscall on the object itself:
//
//      bb 80 1d 09 10               %ebx = &g_10091d80
//      8b 43 40                     the array pointer
//      c7 43 3c <0x10089a90>        the CMoArray subobject's vptr
//      85 c0 / 75 ..                <the three-test chain, as above>
//      b9 80 1d 09 10               %ecx = &g_10091d80
//      c7 43 44 00000000            m_nElements = 0
//      e8 <0x1007dcc4>
//
//  WHAT PINS THE OFFSET: a vtable stored at +0x3c immediately above an
//  array pointer at +0x40 is a CMoArray subobject at +0x3c, which is the
//  same layout 0x1007e270 gives from the other side.  0x1007dcc0 is the
//  destructor named at 0x10003770; declared at +4, the entry reached here.
// ----------------------------------------------------------------------- //
extern void* g_vtbl_10089a90[];                         // 0x10089a90

struct C1007e850
{
	uint8			m_Reserved000[0x3c];            // +0x00..+0x3b
	MoArray1007e270		m_Array3c;                      // +0x3c

	void	sub_1007dcc4();                             // 0x1007dcc0 + 4
};

extern C1007e850 g_10091d80;



// ======================================================================= //
//  FAMILY F -- the node list at 0x100911b8
//
//  0x100362e0 (+4 = 0x100362e4) is a recursive lookup:
//
//      8b 01 / 8b d0 / 8b 45 08 / 3b c2   cmp key,m_Key
//      74 ..                              -> mov %ecx,%eax ; ret
//      8b 49 08 / 85 c9 / 74 ..           -> xor %eax,%eax ; ret
//      57 / 89 04 24 / e8 <itself>        the tail recursion
//
//  i.e. `if(key==m_Key) return this; if(!m_pNext) return NULL;
//  return m_pNext->Find(key);`, so the node is +0x00 key, +0x08 next.
//  0x10035ab0 is the node's scalar deleting destructor
//  (`sub_100362a4() ; if(flags&1) operator delete(this)`).
// ======================================================================= //
struct Node10036420
{
	void*		m_pKey;                                 // +0x00
	uint32		m_n04;                                  // +0x04
	Node10036420*	m_pNext;                                // +0x08

	// 0x100362e0 + 4.  Defined in-class so it is source-marked inline:
	// 0x10036420 carries ONE expansion of it and reaches this
	// out-of-line copy for the recursion, which is what the shipped code
	// does at 0x10036438.
	Node10036420*	sub_100362e4(void* pKey)
	{
		if (pKey == m_pKey)
			return this;

		if (!m_pNext)
			return LTNULL;

		return m_pNext->sub_100362e4(pKey);
	}

	void	sub_10035ab0(uint32 nFlags);                // 0x10035ab0
};


// ----------------------------------------------------------------------- //
//  0x10036420   62 bytes + 2 of padding.
//
//      8b 41 04                     %eax = m_pHead
//      8b 55 08                     %edx = the key
//      3b 10                        cmp %edx,(%eax)
//      74 2a                        -> mov %eax,%ecx  (the hit is the head)
//      8b 48 08 / 85 c9 / 74 1f     -> xor %ecx,%ecx
//      52 / e8 <0x100362e4>         the recursive Find on m_pNext
//      8b c8
//      85 c9 / 74 ..                the delete's own null test
//      57 / c7 04 24 01000000 / e8 <0x10035ab0>
//
//  WHAT PINS IT: one inlined level of Find with the recursion left as a
//  call, and the `3b 10` destination being %edx -- so the KEY is the
//  source's left operand, `if (pKey == m_pKey)`, matching the out-of-line
//  copy's `3b c2` (destination %eax, also the key).  The two arms both
//  jump forward into one `test %ecx,%ecx`, which is the
//  "if/else through a single result variable" shape.
// ----------------------------------------------------------------------- //
struct C10036420
{
	void**		m_pVTable;                              // +0x00
	Node10036420*	m_pHead;                                // +0x04

	void	sub_10036420(void* pKey);
};



// ----------------------------------------------------------------------- //
//  0x10035b40   105 bytes -- NOT the 81 listed; see the header
//  note.  The same class's own deleting destructor:
//
//      8b f1 / c7 06 <0x100894a0>          the derived vptr store
//      a1 <0x100911b8> / 3b c6 / 74 42     -> movl $0x0,0x100911b8 ; back
//      8b 4e 04 / 85 c9 / 74 19            the list, tested for entry
//      33 ff / 47                          the constant 1, hoisted
//   L: 8b 59 08                            pNext
//      85 c9 / 74 06                       the delete's own null test
//      57 / e8 <0x10035ab0>
//      8b cb / 89 5e 04                    pCur = pNext ; m_pHead = pNext
//      85 db / 75 ea                       rotated loop test
//      8b 45 08 / c7 06 <0x10089470>       the flag word, the base vptr
//      a8 01 / 75 ..
//      <exit> / free: 57 / 56 / e8 <0x10080479> / 83 c4 08 / eb eb
//
//  WHAT PINS IT: `cmp %esi,%eax` with %eax = g_100911b8 makes the GLOBAL
//  the left operand (`if (g_100911b8 == this)`), and its body is sunk past
//  the loop to 0x10035b9d with a `jmp` back -- one cold block, not a
//  second exit.  The redundant `test %ecx,%ecx` inside a loop whose
//  condition already proved %ecx non-zero is the un-folded null
//  test, and it is `delete pCur`'s.  Two vptr stores again, and the loop
//  writes m_pHead every iteration, which is a source fact: `mov %ebx,%ecx`
//  then `mov %ebx,0x4(%esi)`.
// ----------------------------------------------------------------------- //
extern void* g_vtbl_100894a0[];                         // 0x100894a0
extern void* g_vtbl_10089470[];                         // 0x10089470

struct C10035b40
{
	void**		m_pVTable;                              // +0x00
	Node10036420*	m_pHead;                                // +0x04

	void	sub_10035b40(uint32 nFlags);
};

extern C10035b40* g_100911b8;                           // 0x100911b8



// ======================================================================= //
//  0x1007b520 -- 81 bytes, no frame, no calls, one x87 loop
//
//      8b 4c 24 04              the single __cdecl argument
//      d9 41 40 / d9 e0         flds 0x40(%ecx) ; fchs      <- HOISTED
//      33 d2                    the byte offset
//      90 / 8d 74 26 00         alignment INSIDE the function, to 0x30
//   L: d9 82 <+0x08> / d8 49 38     plane.z * m_f38
//      d9 82 <+0x04> / d8 49 34     plane.y * m_f34
//      de c1                        faddp
//      d9 82 <+0x00> / d8 49 30     plane.x * m_f30
//      de c1                        faddp
//      d8 a2 <+0x0c>                fsubs plane.w
//      d8 d9 / df e0                fcomp %st(1) ; fnstsw %ax
//      83 c2 10                     the offset step
//      9e / 73 05                   sahf ; jae -> continue
//      dd d8 / 33 c0 / c3           fstp ; return 0
//      83 fa 60 / 72 c7             cmp $0x60 ; jb L
//      dd d8 / b8 01000000 / c3     fstp ; return 1
//
//  WHAT PINS IT:
//    * 0x60 / 0x10 = SIX sixteen-byte records at 0x1008bc50, addressed as
//      base + offset with the base folded into the displacement, which the
//      catalogue says is an INDEXED source loop, not a walked pointer;
//    * the three components multiply +0x30, +0x34, +0x38 of the argument
//      and the fourth is SUBTRACTED, so the record is a plane and the
//      argument carries a point;
//    * the sum is built last-term-first (z, then y, then x), which is
//      icl's own association for `a*x + b*y + c*z` and not a source order;
//    * `fchs` sits ABOVE the loop and `st(1)` is live across every
//      iteration, so `-m_f40` is a register-allocated local -- written as
//      one here rather than left inside the loop for icl to hoist;
//    * `fcomp %st(1)` compares the sum against it and `jae` CONTINUES, so
//      the source tests `sum < -m_f40` with the sum on the left;
//    * `mov $0x1,%eax` for `return 1` -- this row wants /O2.
//
//  THE TEST IS THE SDK'S LTPlane::DistTo (ltplane.h: `m_Normal.Dot(vec) -
//  m_Dist`, vector BY VALUE). Measured: with the dot product written out
//  inline icl hoisted the three point loads out of the loop (106 bytes,
//  83 differing); through an inline helper it keeps them in the loop, and
//  of the 48 term/operand orders of a hand-written helper only those with
//  the z product written normal-first match -- which is exactly what the
//  by-value LTVector::Dot inside DistTo gives. The record array keeps the
//  Plane1008bc50 spelling the other files declare it with (the symbol's
//  type is in its mangled name), so it is viewed as LTPlane here.
// ======================================================================= //
struct Plane1008bc50                                    // 0x10 bytes
{
	float	m_fA;                                       // +0x00
	float	m_fB;                                       // +0x04
	float	m_fC;                                       // +0x08
	float	m_fD;                                       // +0x0c
};

extern Plane1008bc50 g_1008bc50[6];                     // 0x1008bc50

struct C1007b520
{
	uint8		m_Reserved00[0x30];                     // +0x00..+0x2f
	LTVector	m_Pos;                                  // +0x30
	uint32		m_n3c;                                  // +0x3c
	float		m_f40;                                  // +0x40
};

int32 sub_1007b520(const C1007b520* pObj)
{
	float	fLimit = -pObj->m_f40;
	uint32	i;

	for (i = 0; i < 6; i++)
	{
		if (((LTPlane*)g_1008bc50)[i].DistTo(pObj->m_Pos) < fLimit)
		{
			return 0;
		}
	}

	return 1;
}


// ======================================================================= //
//  0x10059f70 -- 73 bytes.  A COM release through slot +0x8c
//
//      8b 5d 0c                     the SECOND argument
//      8b 04 9d <0x1008e684>        g_1008e684[n]
//      85 c0 / 74 2a
//      83 c4 f4                     twelve bytes of outgoing area
//      a1 <0x1008d988>              the interface pointer, loaded ONCE
//      c7 44 24 08 00000000         argument 3 = 0
//      89 5c 24 04                  argument 2 = n
//      8b 10                        the vtable
//      89 04 24                     argument 1 = the interface itself
//      ff 92 8c 00 00 00            call *0x8c(%edx)
//      c7 04 9d <0x1008e684> 0      g_1008e684[n] = NULL
//      33 c0 / 40                   return 1
//      5b / 8b e5 / 5d / c2 08 00
//
//  WHAT PINS IT: `this` is written to the LOWEST outgoing slot and %ecx is
//  never loaded, which is a virtual __stdcall -- a COM method -- not a
//  __thiscall; 0x8c/4 = 35, so the method is the thirty-sixth slot.  The
//  interface pointer is materialised once and serves both the vtable load
//  and the hidden argument, which is the rule "a COM call may need a
//  local for the VTABLE, not just for the object" resolved the other way:
//  one load suffices here.  `ret $0x8` on a free function is __stdcall,
//  and the first of its two arguments is never read.
// ======================================================================= //
struct IUnk10059f70
{
	virtual int32 __stdcall Slot_00();
	virtual int32 __stdcall Slot_04();
	virtual int32 __stdcall Slot_08();
	virtual int32 __stdcall Slot_0c();
	virtual int32 __stdcall Slot_10();
	virtual int32 __stdcall Slot_14();
	virtual int32 __stdcall Slot_18();
	virtual int32 __stdcall Slot_1c();
	virtual int32 __stdcall Slot_20();
	virtual int32 __stdcall Slot_24();
	virtual int32 __stdcall Slot_28();
	virtual int32 __stdcall Slot_2c();
	virtual int32 __stdcall Slot_30();
	virtual int32 __stdcall Slot_34();
	virtual int32 __stdcall Slot_38();
	virtual int32 __stdcall Slot_3c();
	virtual int32 __stdcall Slot_40();
	virtual int32 __stdcall Slot_44();
	virtual int32 __stdcall Slot_48();
	virtual int32 __stdcall Slot_4c();
	virtual int32 __stdcall Slot_50();
	virtual int32 __stdcall Slot_54();
	virtual int32 __stdcall Slot_58();
	virtual int32 __stdcall Slot_5c();
	virtual int32 __stdcall Slot_60();
	virtual int32 __stdcall Slot_64();
	virtual int32 __stdcall Slot_68();
	virtual int32 __stdcall Slot_6c();
	virtual int32 __stdcall Slot_70();
	virtual int32 __stdcall Slot_74();
	virtual int32 __stdcall Slot_78();
	virtual int32 __stdcall Slot_7c();
	virtual int32 __stdcall Slot_80();
	virtual int32 __stdcall Slot_84();
	virtual int32 __stdcall Slot_88();
	virtual int32 __stdcall Slot_8c(uint32 nIndex, uint32 nFlags);
};

extern IUnk10059f70*	g_1008d988;                     // 0x1008d988
extern void*		g_1008e684[];                   // 0x1008e684



// ======================================================================= //
//  0x100592a0 -- 85 bytes.  Two calls through member-function pointers
//
//      83 c4 f4                     twelve bytes of outgoing area
//      8b 7d 08 / 8b 75 0c          arguments 1 and 2
//      8b d9                        %ebx = this
//      c7 44 24 08 00000000         the third outgoing argument = 0
//      89 74 24 04
//      8b 83 ac 07 00 00            %eax = m_pfn7ac
//      89 3c 24
//      8b cb / ff d0                %ecx = this ; call *%eax
//      83 c4 f4
//      8d 76 08                     %esi += 8
//      c7 44 24 08 01000000         and again with 1
//      89 3c 24 / 8b cb / 89 74 24 04
//      8b 83 b0 07 00 00            %eax = m_pfn7b0
//      ff d0
//      5b 5e 5f / 8b e5 / 5d / c2 0c 00
//
//  WHAT PINS IT: `mov <mem>,%eax ; mov %ecx ; call *%eax` with THREE
//  arguments on the stack and `this` in %ecx is a call through a
//  single-inheritance pointer to member function -- not a virtual call
//  (which would be `mov (%ecx),%edx ; call *N(%edx)`) and not a plain
//  function pointer (%ecx would be dead).  `lea 0x8(%esi),%esi` on the
//  second argument makes it `pB + 8` with pB an unsigned char pointer, and
//  the two literals 0 and 1 are the third argument of each call.
//  `ret $0xc` is three parameters and only two are read; a
//  dead parameter is normal.
//
//  The pointer-to-member type is a NESTED typedef so that the class's
//  inheritance model is already known where it is formed, which is what
//  keeps it four bytes wide.
// ======================================================================= //
struct C100592a0
{
	typedef void (C100592a0::*PFN)(void* pA, void* pB, uint32 nWhich);

	uint8	m_Reserved000[0x7ac];                       // +0x000..+0x7ab
	PFN	m_pfn7ac;                                   // +0x7ac
	PFN	m_pfn7b0;                                   // +0x7b0

	void	sub_100592a0(void* pA, uint8* pB, uint32 nUnused);
};



// *********************************************************************** //
//
//  COIN-FLIPS THE BYTES DO NOT SETTLE
//
//  1. 0x1005f730's four global stores.  The shipped order is DESCENDING --
//     0x1008ff88, 0x1008ff84, 0x1008ff80, 0x1008ff7c -- and all four use
//     the shared zero register, so they are one basic block and icl was
//     free to schedule them.  Written descending above.  The alternative
//     is the ascending source
//
//         g_1008ff7c = 0;
//         g_1008ff80 = 0;
//         g_1008ff84 = 0;
//         g_1008ff88 = 0;
//
//     If both fail, the third possibility is that they are not four
//     globals but one `uint32 g_1008ff7c[4]` written element by element.
//
//  2. Where the two extra stores sit in 0x1007e270.  The shipped join
//     block is `m_5c = 0 ; m_50 = 0 ; m_44 = 0` and m_44 is the one that
//     comes out of Term(), so the two loose stores are emitted BEFORE it.
//     Written as
//
//         m_n5c = 0; m_n50 = 0; m_Array3c.Term();
//
//     which needs icl to sink them past the `Free` call.  The reason to
//     prefer it is 0x1007def4, the same chain on the array at +0x08: its
//     join block is `0x1c, 0x18, 0x30, 0x0c` with 0x0c the Term store, so
//     under this reading BOTH functions come out in exact source order and
//     neither needs any intra-block reordering.  The alternative is
//
//         m_Array3c.Term(); m_n5c = 0; m_n50 = 0;
//
//     which instead needs icl to rotate the three stores inside the block.
//     One build separates them.
//
//  3. 0x10035b40's loop tail.  Written as
//
//         pCur = pNext;
//         m_pHead = pNext;
//
//     because that is the shipped order (`mov %ebx,%ecx` then
//     `mov %ebx,0x4(%esi)`) and it needs no reordering.  The alternative
//     is the more idiomatic form with no cursor variable at all,
//
//         while (m_pHead)
//         {
//             pNext = m_pHead->m_pNext;
//             if (m_pHead) m_pHead->sub_10035ab0(1);
//             m_pHead = pNext;
//         }
//
//     where the `mov %ebx,%ecx` is icl updating its cached copy of
//     m_pHead after the store rather than a source assignment before it.
//
//  4. The index of the store in FAMILY C.  Written as two statements,
//
//         pSet->m_pElements[pSet->m_nCount] = pItem;
//         pSet->m_nCount++;
//
//     which relies on icl commoning the second read of m_nCount with the
//     one it already has in %edx from the compare.  If it reloads instead,
//     the one-statement form
//
//         pSet->m_pElements[pSet->m_nCount++] = pItem;
//
//     is the alternative, and a local for the count
//
//         uint32 n = pSet->m_nCount;
//         if (n < pSet->m_nMax) { pSet->m_pElements[n] = pItem;
//                                 pSet->m_nCount++; }
//
//     is the third.  All three must keep `incl <mem>` for the increment.
//
//  5. 0x1007b520's hoisted negation.  Written as a named local so the
//     `fchs` is unambiguously above the loop.  The alternative leaves it
//     in the loop condition, `... < -pObj->m_f40`, and lets icl hoist it.
//
// *********************************************************************** //


void C100375b0::sub_100375b0()
{
	sub_1007fba0(&m_Hash04, 1);

	sub_1007fb20(&m_Hash04, 0x4c, 0x20);
}
// 0x10003770 -- removed: written as icl's own scalar deleting destructor
// (??_GI100039d0) in w57_10009860.cpp, where it is IDENTICAL; see the note
// above struct C10003770's former place.


// ----------------------------------------------------------------------- //
//  0x10043a30   73 bytes, then a seven-byte lea-nop.  1 caller.  Moved
//  here from d3dr4c.cpp: it is /O2, not /Os -- each guard has its OWN
//  `mov %ebp,%esp ; pop %ebp ; ret` (icl /O2 duplicates the epilogue; at
//  /Os the two guards cross-jumped to one exit and the body jumped back,
//  31 bytes differing) and the padding is the /O2 lea-nop.  Here it is
//  byte-identical.
//
//  d3dr2b.cpp reconstructs this family at 0x10050c20 and 0x1003b8e0 (/Os):
//  a global flag, then 0x1007b8d0 (five bytes -- `mov $0x10091a10,%eax ;
//  ret`, the singleton accessor), then a registrar member of the returned
//  object handed `&g_1008b880` and a .text address.  This row adds a SECOND
//  guard: +0x188 of the singleton, the registrar's own count field (+0x04
//  of the object at +0x184).  `mov %eax,%ecx ; mov 0x188(%ecx),%eax ; add
//  $0x184,%ecx` ahead of the test is a registrar POINTER taken before the
//  guard: through `pRoot->m_Reg184` both times the root stays in %eax, the
//  count goes to %edx and the add sinks below the guard (35 differing).
// ----------------------------------------------------------------------- //
// the callback, 0x10015c70 (`movl $0x10015c70,0x4(%esp)` at 0x10043a61): declared with the
// signature d3dr3c.cpp / w7_10040cc0.cpp reference, so the three agree on one name.
struct Ctx1007b8e0;
extern void sub_10015c70(Ctx1007b8e0* pContext, void* pElem);
extern uint8 g_1008b880[];                              // 0x1008b880
extern uint32 g_100893f8;                               // 0x100893f8

void sub_10043a30()
{
	if (!g_100893f8)
		return;

	Reg10043a30* pReg = &sub_1007b8d0()->m_Reg184;

	if (!pReg->m_nCount)
		return;

	pReg->sub_1007b8e0(g_1008b880, (void (*)())sub_10015c70);
}
