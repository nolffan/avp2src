// Split out of d3dr5d.cpp: these functions need the
// OTHER optimisation flag from the rest of that file.  d3d.ren is a
// mixed build and our file boundaries are invented.
// Everything that is not a function body is
// carried over verbatim so no declaration is lost.
// ********************************************************* //
// *********************************************************************** //
//
//  MODULE:   D3DR5D (.CPP)  --  d3d.ren "batch4"
//
//  PURPOSE:  the writable rows of batch4.txt (284..513 bytes each) --
//            ten of the twenty-five, plus the two REAL functions that
//            own the two rows which are not functions at all,
//            decoded instruction by instruction from the retail d3d.ren.
//            Intel C/C++ 4.0.  THE FILE IS NOT FLAG-HOMOGENEOUS -- see the
//            table below; it has to be cut.
//
//  ------------------------------------------------------------------ //
//  THE FLAG VERDICTS (read off the bytes, per function)
//  ------------------------------------------------------------------ //
//
//      addr        flag   the idiom that says so
//      0x10021ad0  /Os    every later exit CROSS-JUMPS (`eb a4` at
//                         0x10021b42, `e9` at 0x10021be9) into ONE
//                         `pop %ebx/%esi/%edi; mov %ebp,%esp; pop %ebp;
//                         ret $0xc`, and `return TRUE` is `xor %eax,%eax;
//                         inc %eax`.
//      0x10009d60  /O2    THE SAME SOURCE as 0x10021ad0, and it duplicates
//                         that epilogue THREE times (0x10009d7d,
//                         0x10009ddf, 0x10009ea7) and spells `return TRUE`
//                         `mov $0x1,%eax`.  This pair is the cleanest
//                         flag-split evidence in the batch: one source,
//                         two objects, two flags.
//      0x1007c290  /O2    five duplicated epilogues, `mov $0x1,%eax`.
//      0x10009f10  /O2    five duplicated epilogues, `mov $0x1,%eax`.
//      0x10032f30  /O2    SIX duplicated `mov (%esp),%ebx; mov %ebp,%esp;
//                         pop %ebp; ret` epilogues, and `mov $0x1,%eax`
//                         at 0x10033048.
//      0x100395f0  /O2    two duplicated epilogues; and `&g_1008e640.m_Bank`
//                         is `mov $0x1008e640,%eax; add $0x4,%eax` TWICE
//                         -- the /O2 `&p->m` form, not `lea`.
//      0x10064b70  /O2    outgoing arguments are `add $0xfffffff0,%esp`
//                         then `mov ...,(%esp)`; no `push` anywhere.
//      0x10035c30  /O2    same; single exit, so the epilogue test is mute.
//      0x10068a50  /O2    same; two duplicated epilogues (0x10068aa8 and
//                         0x10068c37).
//      0x1001d810  /Os    all four `return TRUE` sites JUMP into one
//                         block at 0x1001d834 and the FALSE exit
//                         cross-jumps into its tail.  (It covers
//                         listed row 0x1001d834.)
//      0x1007f420  /Os    identical, around 0x1007f444.  (It
//                         covers listed row 0x1007f444.)
//      0x1001e770  /Os    the second exit cross-jumps (`eb ed` at
//                         0x1001e8ce) into the first's `pop %ebx; pop %ebp;
//                         ret`, and `return TRUE` is `xor %eax,%eax;
//                         inc %eax`.  It is also the only frame-pointerless
//                         function here.
//
//  So: 0x10021ad0, 0x1001d810, 0x1007f420 and 0x1001e770 want /Os;
//  the other eight want /O2.  The file has to be cut on that line.
//
//  ------------------------------------------------------------------ //
//  THE BIG FIND: SIX OF THE ROWS ARE stdlith's CMoArray<T,C>
//  ------------------------------------------------------------------ //
//
//  src/LT2/lithshared/stdlith/dynarray.h is in the tree, so for these the
//  source is not a reconstruction at all -- it is the header, transcribed,
//  with the accessors defined out of line (/Ob1).  The identification is
//  not a shape argument; it is the callee set, which is the same three
//  one-line accessors in every instantiation:
//
//      0x1000a670  `push %esi; mov 0x4(%ecx),%eax; add $0x4,%esp; ret`
//                  == DefaultCache::GetWantedCache  (m_WantedCache, +0x04)
//      0x1000a680  `push %esi; mov 0x8(%esp),%eax; mov %eax,(%ecx);
//                   add $0x4,%esp; ret $0x4`
//                  == DefaultCache::SetCacheSize    (m_CacheSize,   +0x00)
//      0x1000a690  `push %esi; mov (%ecx),%eax; add $0x4,%esp; ret`
//                  == DefaultCache::GetCacheSize
//
//  and the two private helpers, one pair per instantiation:
//
//      _AllocateTArray            _DeleteAndDestroy   GetNumAllocElements
//      0x10021aa0 (n*4 bytes)     0x1001cf00          0x10021a80
//      0x10009cc0 (n*4 bytes)     0x10009cf0          0x10009c90
//      0x1000a0e0 (n*16, `shl $0x4`) 0x1002e6c0       0x10009c90
//
//  0x10021a80 and 0x10009c90 are literally `GetNumAllocatedElements()`:
//  `mov $0xc,%ecx; add %ebx,%ecx; call GetCacheSize; add 0x8(%ebx),%eax`
//  == `m_nElements + m_Cache.GetCacheSize()`, and that `mov $0xc,%ecx;
//  add %ebx,%ecx` is the /O2 `&this->m_Cache`.
//
//  THE LAYOUT falls straight out: +0x00 vptr (CMoArray derives GenList<T>),
//  +0x04 m_pArray, +0x08 m_nElements, +0x0c m_Cache (two dwords).
//
//  ELEMENT SIZE is read off the copy loops:
//      0x10021ad0 / 0x10009d60 / 0x1007c290   `(%edx,%eax,4)`      -> 4
//      0x10009f10                             four dword movs per
//                                             element, `shl $0x4` in
//                                             its _AllocateTArray -> 16
//
//  The `Insert2` pair (0x10021ad0, 0x10009d60) is byte-for-byte the same
//  instruction sequence apart from the epilogue idiom, which is what makes
//  the /Os-vs-/O2 reading above safe rather than a guess.
//
//  ------------------------------------------------------------------ //
//  TWO ROWS ARE NOT FUNCTIONS
//  ------------------------------------------------------------------ //
//
//   0x1001d834 is the shared `return TRUE` exit INSIDE
//              CMoArray<WORD,DefaultCache>::InternalNiceSetSize, whose real
//              prologue is at 0x1001d810 (`push %ebp; mov %esp,%ebp;
//              sub $0x18,%esp; push %edi; push %edi; push %esi; push %ebx;
//              mov %ecx,%ebx`).  0x1001d832's `jne 0x1001d840` falls into
//              it and 0x1001d8e5's `jmp 0x1001d834` re-enters it; it has no
//              prologue of its own.  Its 284 "bytes" are the distance to
//              0x1001d950, which is that function's `_AllocateTArray`.
//              (d3dr4c.cpp already records 0x1001d810 as a real, unwritten,
//              200-400 byte function -- this is what it is.)
//
//   0x1007f444 is the identical block of the identical function at
//              0x1007f420: the two are the same source emitted into two
//              objects (compare 0x1001d820..0x1001d83d with
//              0x1007f430..0x1007f44d -- same bytes, different call
//              displacements).  Same verdict.
//
//              SO BOTH HOSTS ARE WRITTEN BELOW -- 0x1001d810 (T = WORD:
//              `movzwl (%edx,%eax,2)` / `mov %dx,(%esi,%eax,2)`, one
//              `add %ecx,%ecx` for the byte count) and 0x1007f420 (T = 4
//              bytes: `mov (%ecx,%eax,4),%ecx`, two doublings).  They are
//              the same InternalNiceSetSize source as the two /O2 copies,
//              in /Os objects.  Getting those two right retires these two
//              rows; scoring them as separate functions is the
//              enumeration's business, not the source's.
//
//  ------------------------------------------------------------------ //
//  THIRTEEN ROWS ARE DECODED BUT NOT WRITTEN.  The
//  short version, so they need not be re-decoded:
//
//   0x1006c6e0 (311) two-entry.  `malloc(0x28)` (0x10081222), vptr
//                  0x10089800, nine zero stores + `movl $1,0x20`, then
//                  d3d_Alloc(0x80) into +0x0c, +0x14 = 0x2000, a
//                  DDSURFACEDESC2 built on the stack (dwSize 0x7c,
//                  dwFlags 0x40014, ddsCaps 0x1000, and a 0x20-byte
//                  DDPIXELFORMAT memcpy'd out of 0x1008e6c4+0x0c), then
//                  IDirectDraw7::CreateSurface (vtbl +0x18) into +0x1c.
//                  On success it links itself onto the caller's list
//                  (+0x24 = *pList, ++pList->m_nCount, *pList = this),
//                  adds 0x2000 to g_10091948 and its +0x14 to
//                  g_1008cbcc->+0x50.  A vertex-buffer allocator.
//   0x1007e8b0 (314) `Init(handle, w, h, a, b)` on a class with a vtable:
//                  slot +0x0c is Term, slot +0x1c is a virtual "create"
//                  taking one dword.  Contains TWO `jne` off ONE `test`
//                  (0x1007e91b/0x1007e91d) -- the deleted-arm
//                  pattern -- and reaches g_DefAlloc (0x10091df0) Alloc/
//                  Free directly.  Its buffer grows by `(m_24 + m_14) * 4`.
//   0x10064b70 -- WRITTEN below.
//   0x1007da80 (334) model animation blend: two node lists indexed by
//                  0x208(%ebx,%iAnim,4) and 0x228(...), 0x1c-byte
//                  per-animation records, an interpolation weight at
//                  0x14(%ebx,%iAnim*0x1c), a call to 0x10073500 and a
//                  three-component lerp `a + (b - a) * t` written into the
//                  caller's LTVector, then a second lerp over 0x20-byte
//                  records hanging off 0x168 of 0x1a4(%ebx) ADDED to that
//                  vector.  Two `rep movsl` ecx=3 confirm LTVector.
//   0x1001e770 -- WRITTEN below.
//   0x10069ca0 (360) two-entry.  Opens a file through 0x1007fa80(name,
//                  &pStream, 0), memsets two 0x17c-byte structures and
//                  stamps 0x17c into each (a dwSize-carrying descriptor
//                  pair), calls the stream's vtable +0x2c with them,
//                  d3d_Alloc(0xec) (0x10021c20), copies 0x10 bytes from
//                  the caller's argument into +0x10, sets +0x20 =
//                  this+0x10 (or 0 when that argument is NULL), strncpy's
//                  two 0x64-byte names into +0x24 and +0x88, then links
//                  the record onto the doubly-linked list whose head is
//                  0x1008bf78/0x1008bf7c.  Always closes the stream
//                  (vtable +0x08) and returns TRUE.
//   0x10066d10 (384) two-entry.  Walks a `count`-long array of object
//                  pointers; stamps a global frame stamp (0x1008bea8,
//                  a WORD) into each object's +0x42 and skips the ones
//                  already stamped; then `Dot(pDir, pObj->+0x28->+0x34) -
//                  dist > 0.0001f` (the constant at
//                  0x100aaf44), a `+0x38 & 4` reject, and either records the
//                  object in a 0x20-entry table at 0x140/0x40 of the
//                  manager (0x1007b8d0) or dispatches it through one of
//                  four function pointers at 0x1008ce94/98/9c/a0 chosen by
//                  bits 0x80 and 0x8000 of that +0x38.
//   0x10056ab4 (413) an intrusive-list "find or create" over a
//                  0x8c-stride and a 0x1dc-stride table, with a free list
//                  at +0x7f0 and a bank refill call at 0x1007fbf0; the
//                  found/created record is tied onto two circular lists
//                  (+0x10/+0x14 and +0x1c/+0x20) and onto the caller's
//                  link.  Four `dl_Insert`-shaped tie-offs, no arithmetic.
//   0x1005f580 (428) a 0x94-byte frame that stages ~20 globals into an
//                  argument block, calls 0x10020d80 with a 0x3c23d70a
//                  (0.01f) constant, then 0x1001ef30 and 0x10030a60.  It
//                  is a "draw the debug/wireframe pass" driver; almost
//                  every byte is a global-to-slot move, so it is
//                  reconstructible but needs the two callees' prototypes
//                  first.
//   0x10009ae0 (432) five identical `pObj->vtbl->+0x10(pObj, w, h, 0|1,
//                  g_1008cb14)` calls on five global objects
//                  (0x1008ad4c, 0x1008ad10, 0x1008acd4, 0x1008ac98,
//                  0x10091d80) with w/h clamped to >= 1 and w rounded to
//                  a multiple of 3 (`mov $3,%esi; cltd; idiv; lea
//                  (%eax,%eax,2)`), then g_10091de0 = g_1008abb0.  A
//                  render-target reallocation.
//   0x10019874 (432) the same inline "ensure the array is at least
//                  vtbl->m_64 (resp. m_78) elements" helper written TWICE,
//                  over (0x62c,0x630,0x634,0x638) with a `shl $0x5`
//                  element and (0x640,0x644,0x648,0x64c) with `shl $0x6`.
//                  Each copy has the TWO-`jne`-off-one-`test` deleted-arm
//                  shape.  It wants an `inline BOOL Ensure(...)` in the
//                  source; the two copies must be spelled once.
//   0x10008a44 (462) the renderer's per-frame setup: chooses one of two
//                  (stride, cb, cb, size, bank) quintuples by
//                  m_428 == 1, then walks m_pWorld->+0x38[0..m_pWorld->
//                  +0x3c) doing per-"sky/portal" work.  Long, and its
//                  callee 0x10009ae0 is in this same list.
//   0x10058e64 (477) `DrawPolyMgr::DrawPolyAdditionalPass` -- the string
//                  at 0x100aa7b0 names it.  0x1448-byte frame (`_chkstk`
//                  at 0x100805a0), a 0x1428-byte vertex scratch, and four
//                  dispatch tables at 0x10089738/44/4c/5c indexed by the
//                  pass descriptor's +0x10/+0x14/+0x1c.
//   0x10035c30 -- WRITTEN below.
//   0x10068a50 -- WRITTEN below.
//   0x1004fea4 (503) a shell sort of 12-byte {ptr, ?, float} records:
//                  ten passes over the gap tables 0x10089560
//                  {9,8,7,6,5,4,3,2,1,0} (shift) and 0x10089588
//                  {512,...,1} (gap), each pass comparing
//                  `a[j<<s].key < a[(j<<s)+gap].key` with the flag bit
//                  0x40 of each element's +0x88 taken first (an integer
//                  compare when the two differ, the float compare when
//                  they agree), swapping the three dwords and backing up
//                  by one.  Ends with a `d3d_Warning(0, "Sort: %d ticks")`
//                  gated on 0x1008ef60.
//   0x1007f150 (513) not decoded past its prologue in the time available.
//
// *********************************************************************** //

// Only ltbasedefs.h, as d3dr4b.cpp has it.  memcpy needs no include of
// its own: ltbasedefs.h already pulls in <string.h> (and <stdlib.h>),
// which is what lets icl treat memcpy as the intrinsic it expands
// inline at 0x1007c290 and calls at 0x1007f420.
#include "ltbasedefs.h"


// ======================================================================= //
//  SHARED DECLARATIONS
// ======================================================================= //

// ---------------------------------------------------------------------- //
//  stdlith's LAlloc (l_allocator.h), reached as an OBJECT at 0x10091df0
//  and as a pointer parameter everywhere else.  Declared exactly as
//  d3dr4b.cpp / d3dbatchb.cpp declare it -- never defined, so no vtable
//  for it is emitted here.  Slot +0x04 is Alloc (one pushed dword, %ecx =
//  the object) and slot +0x08 is Free; slot +0x00 is only present to put
//  them at those offsets.
// ---------------------------------------------------------------------- //
class IAllocator
{
public:
	virtual void* Slot00(uint32 nSize) = 0;             // +0x00
	virtual void* Alloc(uint32 nSize) = 0;              // +0x04
	virtual void  Free(void* pMem) = 0;                 // +0x08
};


// ---------------------------------------------------------------------- //
//  dynarray.h's DefaultCache.  m_CacheSize at +0x00 (0x1000a690 reads
//  `(%ecx)`, 0x1000a680 writes it) and m_WantedCache at +0x04 (0x1000a670
//  reads `0x4(%ecx)`).  All three accessors are OUT OF LINE in the shipped
//  build, which is what /Ob1 does to a member function that is not marked
//  `inline` -- so they are declared here and never defined.
// ---------------------------------------------------------------------- //
struct DefaultCache
{
	uint32 m_CacheSize;                                 // +0x00
	uint32 m_WantedCache;                               // +0x04

	uint32 GetCacheSize() const;                        // 0x1000a690
	void   SetCacheSize(uint32 val);                    // 0x1000a680
	uint32 GetWantedCache() const;                      // 0x1000a670
};


// ======================================================================= //
//  FAMILY A -- CMoArray<T,DefaultCache>::Insert2(DWORD, const T&, LAlloc*)
//
//      BOOL Insert2( DWORD index, const T &toInsert, LAlloc *pAlloc )
//      {
//          T *pNewArray;  DWORD newSize, i;
//
//          if(index > m_nElements) return FALSE;
//          newSize = m_nElements + 1;
//          if( m_Cache.GetCacheSize() == 0 ) { ...grow... }
//          else                              { ...slide... }
//          ++m_nElements;
//          return TRUE;
//      }
//
//  WHAT PINS IT, instruction by instruction (0x10021ad0):
//   * `mov 0x8(%ebx),%edx; cmp %edx,%esi; jbe` is `index > m_nElements`
//     with the SOURCE's operand order preserved -- index is the cmp's
//     destination because the source wrote `index > m_nElements`.
//   * `lea 0xc(%ebx),%ecx; call 0x1000a690` then `test %eax,%eax; je` is
//     `m_Cache.GetCacheSize() == 0`, and the je goes to the GROW arm --
//     so the slide arm is the fall-through, i.e. the source's `if` tests
//     the cache being zero and the grow arm is the `if` body.  (The `je`
//     jumping FORWARD past the slide code is icl sinking the larger arm.)
//   * `lea 0x1(%eax,%edx,1),%eax` after GetWantedCache is
//     `newSize + m_Cache.GetWantedCache()` with newSize = m_nElements + 1
//     folded into the lea's displacement -- which is why `newSize` is a
//     variable in the source and yet nothing ever adds one.
//   * the two copy loops are `pNewArray[i] = m_pArray[i]` for i<index and
//     `pNewArray[i+1] = m_pArray[i]` for index<=i<m_nElements: the second
//     stores at `0x4(%edi,%eax,4)` and RELOADS `0x8(%ebx)` every trip,
//     which is the header's un-cached `i < m_nElements` bound.
//   * `mov -0x8(%ebp),%eax; mov (%eax),%eax; mov %eax,(%edx,%esi,4)` is
//     `pNewArray[index] = toInsert` through the reference.
//   * `call 0x10021a80` NESTED inside the `_DeleteAndDestroyArray` call's
//     argument block is `_DeleteAndDestroyArray(pAlloc,
//     GetNumAllocatedElements())` -- pAlloc at (%esp), the count at
//     0x4(%esp), matching that function's declared parameter order.
//   * `incl 0x8(%ebx)` reached by BOTH arms is the single `++m_nElements`
//     after the if/else, and the `xor %eax,%eax; inc %eax` next to it is
//     the single `return TRUE`.
// ======================================================================= //

// ---- 0x10021ad0 -- T is 4 bytes; _AllocateTArray 0x10021aa0 doubles its
//      count twice (`add %eax,%eax` twice) for the byte size.   /Os.
struct Arr10021ad0
{
	uint32*      m_pVTable;                             // +0x00
	uint32*      m_pArray;                              // +0x04
	uint32       m_nElements;                           // +0x08
	DefaultCache m_Cache;                               // +0x0c

	uint32* _AllocateTArray(uint32 nElements, IAllocator* pAlloc);   // 0x10021aa0
	void    _DeleteAndDestroyArray(IAllocator* pAlloc, uint32 n);    // 0x1001cf00
	uint32  GetNumAllocatedElements();                               // 0x10021a80

	int32   Insert2(uint32 index, const uint32& toInsert,
	                IAllocator* pAlloc);                             // 0x10021ad0
};

int32 Arr10021ad0::Insert2(uint32 index, const uint32& toInsert,
                           IAllocator* pAlloc)
{
	uint32* pNewArray;
	uint32  newSize, i;

	if (index > m_nElements)
		return 0;

	newSize = m_nElements + 1;

	if (m_Cache.GetCacheSize() == 0)
	{
		pNewArray = _AllocateTArray(newSize + m_Cache.GetWantedCache(), pAlloc);
		if (!pNewArray)
			return 0;

		for (i = 0; i < index; i++)
			pNewArray[i] = m_pArray[i];

		for (i = index; i < m_nElements; i++)
			pNewArray[i + 1] = m_pArray[i];

		pNewArray[index] = toInsert;

		if (m_pArray)
		{
			_DeleteAndDestroyArray(pAlloc, GetNumAllocatedElements());
		}

		m_Cache.SetCacheSize(m_Cache.GetWantedCache());
		m_pArray = pNewArray;
	}
	else
	{
		for (i = m_nElements; i > index; i--)
			m_pArray[i] = m_pArray[i - 1];

		m_Cache.SetCacheSize(m_Cache.GetCacheSize() - 1);

		m_pArray[index] = toInsert;
	}

	++m_nElements;

	return 1;
}


// ---- 0x10009d60 -- the SAME source, T also 4 bytes, in an /O2 object.
//      Its helpers are 0x10009cc0 / 0x10009cf0 / 0x10009c90.
struct Arr10009d60
{
	uint32*      m_pVTable;                             // +0x00
	uint32*      m_pArray;                              // +0x04
	uint32       m_nElements;                           // +0x08
	DefaultCache m_Cache;                               // +0x0c

	uint32* _AllocateTArray(uint32 nElements, IAllocator* pAlloc);   // 0x10009cc0
	void    _DeleteAndDestroyArray(IAllocator* pAlloc, uint32 n);    // 0x10009cf0
	uint32  GetNumAllocatedElements();                               // 0x10009c90

	int32   Insert2(uint32 index, const uint32& toInsert,
	                IAllocator* pAlloc);                             // 0x10009d60
};



// ======================================================================= //
//  FAMILY B -- CMoArray<T,DefaultCache>::InternalNiceSetSize
//
//      BOOL InternalNiceSetSize( DWORD newSize, BOOL bFast, LAlloc *pAlloc )
//
//  WHAT PINS IT: the header's else-if chain compares newSize against
//  m_nElements THREE TIMES and icl emits THREE `cmp %eax,%edx` off the
//  same two registers --
//
//      3b d0  cmp %eax,%edx ; 0f 82  jb  shrink      newSize <  m_nElements
//      3b d0  cmp %eax,%edx ; 0f 87  ja  grow        newSize >  m_nElements
//      3b d0  cmp %eax,%edx ; 75     jne alloc       newSize == m_nElements
//
//  which is the "several branches off one compare" signature and
//  is only produced by three separate source comparisons, not by a
//  switch or by a cached result.  The `grow` arm's own
//  `(m_nElements + GetCacheSize()) >= newSize` failure jumps BACK to the
//  third compare (`jb 0x1007c2bb`), which is icl routing the else-if
//  chain -- more proof that the chain is written exactly as the header
//  writes it.
//
//  Other pins:
//   * TWO GetCacheSize calls in the grow arm == two in the source
//     (the guard `(m_nElements + GetCacheSize()) >= newSize` and then
//     `SetCacheSize(GetCacheSize() - (newSize - m_nElements))`).
//   * the shrink arm computes `m_nElements - newSize` FIRST and adds the
//     cache size to it (`sub %ecx,%edx; add %eax,%edx`) -- the source's
//     own parenthesisation, `GetCacheSize() + (m_nElements - newSize)`.
//   * `mov 0x8(%ebx),%edx; mov 0x8(%ebp),%eax; cmp %eax,%edx; jbe; mov
//     %eax,%edx` is `nToCopy = m_nElements; if(nToCopy > newSize) nToCopy
//     = newSize;` -- two statements, not a ?: (a ?: puts both arms
//     through one register and loses the `jbe` over a single `mov`).
//   * `mov 0xc(%ebp),%eax; test %eax,%eax; jne` is `if(bFast)`, with the
//     memcpy arm SUNK past the element loop -- the fall-through is the
//     `else`.
// ======================================================================= //

// ---- 0x1007c290 -- T is 4 bytes.  memcpy is expanded inline here
//      (`push %ecx; shr $0x2,%ecx; rep movsl; pop %ecx; and $0x3,%ecx;
//      rep movsb`) with the byte count built by two `add %edx,%edx`, so
//      the source really does say memcpy(dst, src, sizeof(T) * nToCopy).
//      /O2.
struct Arr1007c290
{
	uint32*      m_pVTable;                             // +0x00
	uint32*      m_pArray;                              // +0x04
	uint32       m_nElements;                           // +0x08
	DefaultCache m_Cache;                               // +0x0c

	uint32* _AllocateTArray(uint32 nElements, IAllocator* pAlloc);   // 0x10021aa0
	void    _DeleteAndDestroyArray(IAllocator* pAlloc, uint32 n);    // 0x1001cf00
	uint32  GetNumAllocatedElements();                               // 0x10021a80

	int32   InternalNiceSetSize(uint32 newSize, int32 bFast,
	                            IAllocator* pAlloc);                 // 0x1007c290
};



// ---- 0x10009f10 -- the SAME source with a SIXTEEN-byte T.  Two
//      independent witnesses to the element size: its _AllocateTArray
//      (0x1000a0e0) is `shl $0x4,%eax`, and the non-fast loop copies four
//      dwords per element with the index stepping by 0x10 while the bound
//      is `nToCopy << 4`.  The fast arm is `shl $0x2,%edx; mov %edx,%ecx;
//      rep movsl` -- the same memcpy call, expanded knowing 16*n is a
//      whole number of dwords.  /O2.
// The element type: sixteen bytes copied FIELD BY FIELD (four dword movs
// at 0x10009fa5..0x10009fc5), not by `rep movsl`.  Four NAMED dwords are
// written here rather than a uint32[4] because an array member's implicit
// copy is the block form; whether the four are integers or floats the
// shipped bytes cannot say (icl copies float lvalues with integer movs),
// so they are named by offset.
struct Elem10009f10
{
	uint32 m_n00;                                       // +0x00
	uint32 m_n04;                                       // +0x04
	uint32 m_n08;                                       // +0x08
	uint32 m_n0c;                                       // +0x0c
};

struct Arr10009f10
{
	uint32*      m_pVTable;                             // +0x00
	Elem10009f10* m_pArray;                             // +0x04
	uint32       m_nElements;                           // +0x08
	DefaultCache m_Cache;                               // +0x0c

	Elem10009f10* _AllocateTArray(uint32 nElements, IAllocator* pAlloc); // 0x1000a0e0
	void    _DeleteAndDestroyArray(IAllocator* pAlloc, uint32 n);        // 0x1002e6c0
	uint32  GetNumAllocatedElements();                                   // 0x10009c90

	int32   InternalNiceSetSize(uint32 newSize, int32 bFast,
	                            IAllocator* pAlloc);                     // 0x10009f10
};


// (0x1001d810 and 0x1007f420, the two /Os InternalNiceSetSize copies, moved
// to w45_1001d810_osoi.cpp: they CALL memcpy, so /Os /Oi- -- identical there.)


// ======================================================================= //
//  0x10032f30 / 0x10032f34   310 bytes.  LOAD A GAMMA RAMP FROM A FILE.
//
//  TWO ENTRIES: 0x10032f30 is a four-byte stack adapter
//  (`mov 0x4(%esp),%eax`) that falls into the register entry at
//  0x10032f34, and BOTH callers (0x10032dc2, 0x10033a61) call the
//  register entry.  It is written here as one ordinary one-argument
//  __cdecl function named after the adapter, which is what d3dbatchc.cpp
//  did for 0x10032a00; if it comes back shifted by four bytes the fix is
//  to rename it sub_10032f34 and the body does not change.
//
//  WHAT IT IS, and none of it is guessed:
//   * 0x1008107b is fopen -- `push $0x40; push mode; push name; call
//     _fsopen` with 0x40 == _SH_DENYNO.  The mode string at 0x100a9050 is
//     "r".
//   * 0x1008108e is fread -- it locks the stream (0x10083ba5), calls
//     _fread_lk (0x100810bd, which starts `imul 0x10(%ebp),%edi`) and
//     unlocks.  The pushed argument block is (buffer, 0x600, 1, file), so
//     the record size is 0x600 and the count is 1, which is why the
//     result is compared against 1.
//   * 0x100811a5 is fclose (`testb $0x40,0xc(%esi)` on the _iobuf flags,
//     then _fclose_lk).
//   * 0x10088188 is the sixteen bytes
//     3E 1C C1 69 6B B4 D1 11 AD 7A 00 C0 4F C2 9B 4E ==
//     {69C11C3E-B46B-11D1-AD7A-00C04FC29B4E} == IID_IDirectDrawGammaControl,
//     handed to slot +0x00 (QueryInterface) of a surface with an out
//     pointer -- and slot +0x10 of the RESULT (the fifth vtable entry:
//     QI, AddRef, Release, GetGammaRamp, SetGammaRamp) is then called with
//     (0, buffer).  That fixes the buffer as a DDGAMMARAMP, whose size is
//     exactly 3 * 256 * sizeof(WORD) == 0x600.
//   * 0x1008cbbc, 0x1008bf6c and 0x1008bf70 are g_bWindowed, g_pPrimary
//     and g_pWindowedPrimary -- already named by d3dblit.cpp, and the
//     `test; je` picks g_pPrimary when the flag is clear, so the
//     fall-through (windowed) surface is 0x1008bf70.
//
//  WHAT PINS THE SHAPE:
//   * TWO fclose call sites, one in each arm of the fread test, mean two
//     source statements, not one hoisted close.
//   * TWO QueryInterface call sites, each with its own `test %eax,%eax;
//     je 0x1003301b` and its own `xor %eax,%eax` + epilogue, mean the
//     `if (g_bWindowed)` arms each contain their own guarded return --
//     writing one call through a selected pointer gives one call site.
//   * every failure is `xor %eax,%eax` and the success at 0x10033048 is
//     `mov $0x1,%eax`, so the return type is BOOL and the flag is /O2.
//   * the frame is 0x620: 0x600 of ramp at -0x608, the interface pointer
//     at -0x8, icl's dead dword at -0x4, and the %ebx save at (%esp).
// ======================================================================= //

extern "C"
{
	struct _iobuf;
	typedef struct _iobuf FILE;

	FILE*        __cdecl fopen(const char* pszName, const char* pszMode);
	unsigned int __cdecl fread(void* pBuf, unsigned int nSize,
	                           unsigned int nCount, FILE* pFile);
	int          __cdecl fclose(FILE* pFile);
}

// The DirectDraw pieces this function needs.  Spelled out rather than
// included because the toolchain's ddraw.h is the DirectX 5 one and has no
// IDirectDrawGammaControl at all.
struct DDGammaRamp
{
	uint16 red[256];
	uint16 green[256];
	uint16 blue[256];
};                                                      // == 0x600

struct DDGammaControl;

struct DDGammaControlVtbl
{
	uint32 m_pad_00[4];                                 // QI/AddRef/Release/Get
	// +0x10 -- IDirectDrawGammaControl::SetGammaRamp
	long (__stdcall* SetGammaRamp)(DDGammaControl*, uint32 dwFlags,
	                               DDGammaRamp* pRamp);
};

struct DDGammaControl
{
	DDGammaControlVtbl* lpVtbl;
};

struct DDSurface7;

struct DDSurface7Vtbl
{
	// +0x00 -- IUnknown::QueryInterface
	long (__stdcall* QueryInterface)(DDSurface7*, const void* riid,
	                                 void** ppv);
};

struct DDSurface7
{
	DDSurface7Vtbl* lpVtbl;
};

extern DDSurface7* g_pPrimary;                          // 0x1008bf6c
extern DDSurface7* g_pWindowedPrimary;                  // 0x1008bf70
extern LTBOOL      g_bWindowed;                         // 0x1008cbbc

extern "C" const uint8 IID_IDirectDrawGammaControl[16];  // 0x10088188




// ======================================================================= //
//  0x100395f0   317 bytes.  TEAR DOWN THE TEXTURE/SURFACE SUBSYSTEM.
//
//  WHAT PINS IT:
//   * the whole body is guarded by `mov 0x1008da70,%eax; test; jne` with
//     the jump going INTO the work -- so the source's guard is
//     `if (!g_bInitialised) return;` and the last statement of the body is
//     the store that clears the same global.  (The epilogue is duplicated
//     at 0x1003960a and 0x1003971e: /O2.)
//   * 0x1007fba0 walks a singly-linked chain out of its argument's +0x14
//     calling d3d_Free (0x10021cb0) on every node and then zeroes +0x14
//     and +0x18 -- a memory bank's Term.  0x1007fb20 writes an element
//     size at +0x00, rounds it up to a multiple of four into +0x04 and
//     takes a third argument -- the matching Init(size, count).  The
//     arguments here are (0x4c, 0x20).
//   * BOTH bank calls address their object as `mov $0x1008e640,%eax;
//     add $0x4,%eax` -- recomputed, not hoisted, which is exactly what
//     a repeated `&g_X.m_Member` expression does, and what
//     tells you 0x1008e640 is a STRUCT whose bank member sits at +0x04
//     rather than a bank at 0x1008e644.
//   * the three `if (p) { p->lpVtbl->Release(p); p = NULL; }` blocks are
//     three separate statements: each reloads its global AFTER the test
//     (`mov g,%eax; test %eax,%eax; je; mov g,%eax; mov (%eax),%edx`),
//     which is what a member expression written twice does.
//   * the list walk is an LTLink circular list whose head node IS the two
//     dwords at 0x1008e6a4/0x1008e6a8: the loop starts from the head's
//     m_pNext, frees each node's m_pData (+0x08), follows m_pNext (+0x04)
//     and stops when it reaches the head's own address, and the tie-off
//     afterwards stores that address into both dwords.
//   * the tail zeroes SEVEN consecutive dwords off ONE base register
//     (`mov $0x1008e6b0,%edi` then (%edi) .. 0x18(%edi)) -- seven member
//     assignments in one aggregate, not seven separate globals, which
//     would each have been an absolute `movl $0x0,imm32`.
// ======================================================================= //

struct MemBank
{
	uint32 m_nElementSize;                              // +0x00
	uint32 m_nRoundedSize;                              // +0x04
	uint32 m_pad_08[3];                                 // +0x08
	void*  m_pBlocks;                                   // +0x14
	uint32 m_nUnknown18;                                // +0x18
};

extern void sub_1007fba0(MemBank* pBank, uint32 bFlag);      // 0x1007fba0
extern void sub_1007fb20(MemBank* pBank, uint32 nElemSize,
                         uint32 nCount);                     // 0x1007fb20
extern void sub_10039930();                                  // 0x10039930
extern void d3d_Free(void* pMem);                            // 0x10021cb0

// The record 0x1008e640 is the head of; only its bank member is touched
// here, and the bank is at +0x04.
struct Unknown1008e640
{
	uint32  m_nUnknown00;                               // +0x00
	MemBank m_Bank;                                     // +0x04
};

extern Unknown1008e640 g_1008e640;                      // 0x1008e640

// A COM object with nothing but IUnknown reached here.
struct Releasable;
struct ReleasableVtbl
{
	uint32 m_pad_00[2];
	void (__stdcall* Release)(Releasable*);             // +0x08
};
struct Releasable
{
	ReleasableVtbl* lpVtbl;
};

extern Releasable* g_p1008da74;                         // 0x1008da74
extern Releasable* g_p1008da78;                         // 0x1008da78
extern Releasable* g_p1008da7c;                         // 0x1008da7c
extern LTBOOL      g_b1008da70;                         // 0x1008da70

// The seven dwords at 0x1008e6b0, cleared as one aggregate.
struct Unknown1008e6b0
{
	uint32 m_n00, m_n04, m_n08, m_n0c, m_n10, m_n14, m_n18;
};

extern Unknown1008e6b0 g_1008e6b0;                      // 0x1008e6b0

// stdlith's LTLink, the circular list node this module uses.
struct D3DLink
{
	D3DLink* m_pPrev;                                   // +0x00
	D3DLink* m_pNext;                                   // +0x04
	void*    m_pData;                                   // +0x08
};

extern D3DLink g_List1008e6a4;                          // 0x1008e6a4




// ======================================================================= //
//  THE OBJECT-SET FAMILY -- 0x10064b70 and 0x10068a50
//
//  BOTH functions add render objects to a bounded "set", and the set is a
//  0x1c-byte record whose field offsets are established TWICE over,
//  independently:
//
//   * 0x10064b70 uses three of them inside the manager that 0x1007b8d0
//     returns (0x10091a10), at +0x1e4, +0x204 and +0x224.  Within each,
//     the element array is at +0x00, the count at +0x04, the limit at
//     +0x14 and the name at +0x18:
//         0x1e4/0x1e8/0x1f8/0x1fc
//         0x204/0x208/0x218/0x21c
//         0x224/0x228/0x238/0x23c
//   * 0x10068a50 builds one on its own STACK at -0x1020 and uses exactly
//     the same four deltas (-0x1020/-0x101c/-0x100c/-0x1008), and the
//     0x400-entry array it points the set at starts at -0x1004 -- which
//     is -0x1020 + 0x1c, so the record is 0x1c bytes and nothing follows
//     it.
//
//  The add itself is expanded INLINE at all four sites -- three in
//  0x10064b70 and one in 0x10068a50 -- with the count's increment folded
//  onto the owning object (`incl 0x208(%eax)`, `incl -0x101c(%ebp)`), so
//  the source marks the helper `inline` (this build is /Ob1: nothing else
//  gets inlined).  The overflow arm is d3d_Warning(1, "Set '%s'
//  overflowed", pSet->m_pszName) -- the format string is at 0x100a9a10
//  and 0x10021d60 is d3d_Warning, already named by d3dblit.cpp.
//
//  The "which set" test is a flags dword fetched by a SECOND inline
//  helper, and its five-instruction pointer chain is byte-identical in
//  the two functions:
//
//      mov 0x1a8(%ebx),%eax ; test %eax,%eax ; je  zero
//      mov (%ebx),%eax ; mov %ebx,%ecx ; mov 0x38(%eax),%eax ; call *%eax
//      test %eax,%eax ; je zero
//      mov 0x1a8(%ebx),%eax          <- the member RELOADED after the call
//      mov 0xa4(%eax),%edx ; test %edx,%edx ; jbe zero
//      mov 0xa0(%eax),%eax ; mov (%eax),%eax
//      mov 0x28(%eax),%eax ; mov 0x38(%eax),%eax
//
//  Two `xor %eax,%eax` blocks fall together into the join, which is the
//  helper's single `return 0` after tail merging.  0x10064b70 tests bit
//  0x8 of the result and 0x10068a50 tests bit 0x80000, so the helper is
//  shared and the bit is the caller's.
// ======================================================================= //

extern void sub_10021d60(uint32 nLevel, const char* pszMsg, ...);  // 0x10021d60 AddDebugMessage (was d3d_Warning*; icl treats a callee named *Warn*/*Error* as cold)

struct D3DRenderStyle
{
	uint8  m_pad_00[0x38];
	uint32 m_nFlags;                                    // +0x38
};

struct D3DModelPiece
{
	uint8           m_pad_00[0x28];
	D3DRenderStyle* m_pRenderStyle;                     // +0x28
};

struct D3DModelInstance
{
	uint8           m_pad_00[0xa0];
	D3DModelPiece** m_pPieces;                          // +0xa0
	uint32          m_nPieces;                          // +0xa4
};

class D3DObject
{
public:
	// Fourteen slots ahead of the one this family calls; the call is
	// `mov %ebx,%ecx; call *0x38(%eax)` with nothing pushed and a
	// dword answer, i.e. an ordinary virtual __thiscall taking no
	// arguments.
	virtual void  _Slot00();  virtual void  _Slot04();
	virtual void  _Slot08();  virtual void  _Slot0c();
	virtual void  _Slot10();  virtual void  _Slot14();
	virtual void  _Slot18();  virtual void  _Slot1c();
	virtual void  _Slot20();  virtual void  _Slot24();
	virtual void  _Slot28();  virtual void  _Slot2c();
	virtual void  _Slot30();  virtual void  _Slot34();
	virtual int32 Slot38();                             // vtable +0x38

public:
	uint8             m_pad_04[0x84];                   // +0x04
	uint32            m_nFlags;                         // +0x88
	uint32            m_nFlags2;                        // +0x8c
	uint8             m_pad_90[0x118];                  // +0x90
	D3DModelInstance* m_pInstance;                       // +0x1a8
};

// The bounded set.  0x1c bytes; the three dwords at +0x08..+0x13 are
// never touched by either function and are named by offset.
struct D3DObjectSet
{
	D3DObject** m_pElements;                            // +0x00
	uint32      m_nElements;                            // +0x04
	uint32      m_nUnknown08;                           // +0x08
	uint32      m_nUnknown0c;                           // +0x0c
	void*       m_pUnknown10;                           // +0x10
	uint32      m_nMaxElements;                         // +0x14
	const char* m_pszName;                              // +0x18
};

// 0x1007b8d0 is `mov $0x10091a10,%eax; ret` -- it hands back the address
// of the manager, so it is an accessor, not a lookup.
struct D3DObjectMgr
{
	uint8        m_pad_000[0x1e4];                      // +0x000
	D3DObjectSet m_Set1e4;                              // +0x1e4 (0x1c)
	uint32       m_nUnknown200;                         // +0x200
	D3DObjectSet m_Set204;                              // +0x204 (0x1c)
	uint32       m_nUnknown220;                         // +0x220
	D3DObjectSet m_Set224;                              // +0x224 (0x1c)
};

extern D3DObjectMgr* sub_1007b8d0();                    // 0x1007b8d0





// ---------------------------------------------------------------------- //
//  0x10064b70   330 bytes.  FILE THE OBJECT INTO ONE OF THREE SETS.
//
//  __cdecl, one argument (`mov 0x8(%ebp),%ebx`, plain `ret`); its address
//  is in two dispatch tables (.data 0x100895e8 and 0x10089674), which is
//  what makes it __cdecl rather than the register entry its neighbours
//  use.
//
//  ARM ORDER: `test $0x8,%al; jne` sinks the first arm to the very end of
//  the function and `test $0x2,%al; je` sinks the third past the second,
//  so the second arm is the fall-through.  That is the plain
//  if / else-if / else order written below -- icl lays the arms out
//  backwards.  All three arms end `jmp 0x10064cb3`, one shared epilogue.
// ---------------------------------------------------------------------- //


// ---------------------------------------------------------------------- //
//  0x10068a50   502 bytes.  SWEEP THE OBJECT LIST INTO A LOCAL SET.
//
//  __cdecl, NO arguments (plain `ret`, nothing read above %ebp).  The
//  0x1040-byte frame goes through _chkstk (0x100805a0), which is what
//  `mov $0x1040,%eax; call` is.
//
//  WHAT PINS IT:
//   * the local set's five initialising stores are emitted in the order
//     +0x10, +0x00, +0x04, +0x18, +0x14, and the +0x10 one is
//     `lea -0x1020(%ebp),%eax` -- the set's OWN address.  That store is
//     written first below because a statement's position relative to the
//     initialised declarations is source-visible ("a statement
//     written before the initialised declarations stays before them").
//     THIS IS THE FILE'S MAIN COIN-FLIP.
//   * `mov 0x1009110c,%ecx; test %ecx,%ecx; jne` with the whole body
//     jumped to is the leading `if (!g) return;`.
//   * `movl $0x0,-0x101c(%ebp)` a SECOND time at 0x10068ac0, after
//     0x1007b8d0, is a second source statement -- the set is cleared
//     again once the manager is in hand.
//   * the two-way flag test is
//         if (pObj->m_nFlags & 1) { if (g && (pObj->m_nFlags2 & 1)) continue; }
//         else                    { if (g && !(pObj->m_nFlags & 0x400)) continue; }
//     read off `test $0x1,%al / je 0x10068b10`, then in the taken arm
//     `test $0x1,%al` on +0x8c with `jne skip`, and in the fall-through
//     arm `test $0x4,%ah` (bit 0x400 of the SAME +0x88 dword, still in
//     %eax) with `je skip`.  Both arms then reach 0x10068b23.
//   * `mov 0x208(%edi),%edx` is reloaded on EVERY path out of the body,
//     which is the loop's un-cached `i < pMgr->m_Set204.m_nElements`
//     bound.
//   * the second pass hoists `mov $0x10068910,%esi` out of the loop and
//     reloads `mov 0x1008ef3c,%ecx` inside it: a function address is
//     loop-invariant, a global pointer member expression is re-read.
// ---------------------------------------------------------------------- //
extern LTBOOL g_b1009110c;                              // 0x1009110c
extern LTBOOL g_b1008bd54;                              // 0x1008bd54

extern void sub_100688e0(void* pUnknown, D3DObject* pObject);   // 0x100688e0
extern uint32 g_1008b880;                               // 0x1008b880

// 0x100500a0 -- a __thiscall member (this stays in %ecx, two pushed
// dwords) of whatever 0x1008ef3c points at; the second argument is the
// address of the function at 0x10068910.
class C1008ef3c
{
public:
	void sub_100500a0(D3DObject* pObject, void (*pfn)(void*));
};

extern C1008ef3c* g_p1008ef3c;                          // 0x1008ef3c
extern void       sub_10068910(void* pUnknown);          // 0x10068910




// ======================================================================= //
//  0x10035c30   489 bytes.  RESET THE TEXTURE STAGES AND SET UP A
//                           DIFFUSE-ALPHA BLEND FOR THIS SURFACE.
//
//  IT IS A __thiscall MEMBER, and that is not inferred from the missing
//  argument -- its address is the FOURTH entry of the eleven-pointer run
//  at .data 0x10089480, which is a VTABLE: 0x10035c00 (slot +0x08) is
//  `mov 0x4(%ecx),%ebx; ... ret $0x8` and 0x10035bd0 (slot +0x1c) is
//  `mov 0xc(%ecx),%eax; if(p) p->lpVtbl->Release(p); +0xc = +0x8 = +0x4 =
//  0`, so the whole run is %ecx-taking members of one class and the class
//  is { +0x00 vptr, +0x04 w, +0x08 h, +0x0c texture }.  This function
//  reads only +0x0c.
//
//  THE D3D7 CALLS, all on g_pD3DDevice (0x1008d988):
//    +0x50 SetRenderState        (IDirect3DDevice7 slot 20)
//    +0x8c SetTexture            (slot 35)
//    +0x94 SetTextureStageState  (slot 37)
//  and every literal below is in the disassembly:
//    stage loop 0..7:  SetTexture(i, NULL)
//                      SetTextureStageState(i, 1, 1)   COLOROP  = DISABLE
//                      SetTextureStageState(i, 4, 1)   ALPHAOP  = DISABLE
//    then on stage 0:  SetTexture(0, m_pTexture)
//                      (0, 0x0c, 3)   ADDRESS   = CLAMP
//                      (0, 2,    2)   COLORARG1 = TEXTURE
//                      (0, 3,    0)   COLORARG2 = DIFFUSE
//                      (0, 1,  0x0c)  COLOROP   = BLENDDIFFUSEALPHA
//                      (0, 5,    2)   ALPHAARG1 = TEXTURE
//                      (0, 6,    0)   ALPHAARG2 = DIFFUSE
//                      (0, 4,    3)   ALPHAOP   = SELECTARG2
//                      SetRenderState(0x1b, 1)  ALPHABLENDENABLE = TRUE
//                      SetRenderState(0x13, 1)  SRCBLEND  = ZERO
//                      SetRenderState(0x14, 3)  DESTBLEND = SRCALPHA
//
//  WHAT PINS THE SHAPE:
//   * `mov 0xc(%ecx),%esi` is executed ONCE, before the loop, and parked
//     in -0x4(%ebp) across it -- one member read into a local, not a
//     member expression at the point of use (which would reload, since
//     the intervening indirect calls can alias).
//   * `mov 0x1008d988,%ebx` is likewise executed ONCE and %ebx feeds all
//     FOURTEEN calls, while `mov (%ebx),%eax` (the vtable) is reloaded
//     at every one of them.  icl cannot hoist a global pointer across
//     indirect calls, so the DEVICE is a local in the source and the
//     vtable is not -- writing `g_pD3DDevice->lpVtbl->X(g_pD3DDevice,..)`
//     the way d3dbatchc.cpp does would reload the global fourteen times.
//     The local is read AFTER the member, which is the declaration order
//     below.
//   * `xor %edi,%edi` / `inc %edi` / `cmp $0x8,%edi` / `jl` is a SIGNED
//     `for (i = 0; i < 8; i++)`; `jb` would say unsigned.
//   * the three calls in the loop all pass %esi, which is reloaded from
//     %edi at the top of every iteration -- the loop counter itself is
//     the stage index, no separate variable.
//   * the single exit's `xor %eax,%eax; inc %eax` is `return TRUE`.
// ======================================================================= //

struct D3DDevice7;

struct D3DDevice7Vtbl
{
	uint8 m_pad_00[0x50];
	// +0x50 -- IDirect3DDevice7::SetRenderState
	long (__stdcall* SetRenderState)(D3DDevice7*, uint32 dwState,
	                                 uint32 dwValue);
	uint8 m_pad_54[0x38];
	// +0x8c -- IDirect3DDevice7::SetTexture
	long (__stdcall* SetTexture)(D3DDevice7*, uint32 dwStage,
	                             void* lpTexture);
	uint8 m_pad_90[0x04];
	// +0x94 -- IDirect3DDevice7::SetTextureStageState
	long (__stdcall* SetTextureStageState)(D3DDevice7*, uint32 dwStage,
	                                       uint32 dwState, uint32 dwValue);
};

struct D3DDevice7
{
	D3DDevice7Vtbl* lpVtbl;
};

extern D3DDevice7* g_pD3DDevice;                        // 0x1008d988

#define D3DTSS_COLOROP                        1
#define D3DTSS_COLORARG1                      2
#define D3DTSS_COLORARG2                      3
#define D3DTSS_ALPHAOP                        4
#define D3DTSS_ALPHAARG1                      5
#define D3DTSS_ALPHAARG2                      6
#define D3DTSS_ADDRESS                       12

#define D3DTOP_DISABLE                        1
#define D3DTOP_SELECTARG2                     3
#define D3DTOP_BLENDDIFFUSEALPHA             12

#define D3DTA_DIFFUSE                         0
#define D3DTA_TEXTURE                         2

#define D3DTADDRESS_CLAMP                     3

#define D3DRENDERSTATE_SRCBLEND              19         // 0x13
#define D3DRENDERSTATE_DESTBLEND             20         // 0x14
#define D3DRENDERSTATE_ALPHABLENDENABLE      27         // 0x1b

#define D3DBLEND_ZERO                         1
#define D3DBLEND_SRCALPHA                     3


class C10035c30
{
public:
	uint32 m_pVTable;                                   // +0x00
	uint32 m_nWidth;                                    // +0x04
	uint32 m_nHeight;                                   // +0x08
	void*  m_pTexture;                                  // +0x0c

	int32  sub_10035c30();                              // 0x10035c30
};



// ======================================================================= //
//  0x1001e770   356 bytes.  IS THE OBJECT'S BOX INSIDE THE SIX PLANES?
//
//  __cdecl, ONE argument (`push %ebp; push %ebx; mov 0xc(%esp),%eax`) and
//  NO frame pointer -- the only function in this batch without one.  Its
//  address is planted into a local structure at 0x1007b69c
//  (`movl $0x1001e770,-0x2c(%ebp)`), so it is a callback slot, which is
//  why the object it takes is anonymous.
//
//  WHAT IT DOES: builds the eight corners of the box (centre at +0x0c,
//  half-dimensions at +0x18) and tests them against the six planes at
//  0x1008bc50 (stride 0x10 -- `add $0x10,%ebx; cmp $0x60,%ebx; jl`, i.e.
//  six LTPlanes).  If EVERY corner of the box is on the negative side of
//  ANY one plane the answer is FALSE; otherwise TRUE.
//
//  WHAT PINS IT:
//   * the frame is 0x64 and the twenty-four `fstps` walk 0x00, 0x04 ...
//     0x5c in ASCENDING order in eight groups of three -- the rule
//     "contiguous ascending frame slots are an array", so this is
//     LTVector v[8] and not eight named locals.  The remaining dword of
//     the 0x64 is icl's.
//   * the inner loop walks a POINTER (`mov %esp,%edx` ... `add $0xc,%edx`)
//     against `lea 0x60(%esp),%ebp`, which is the strength reduction of
//     an indexed `v[j]` -- write the indexed form.
//   * the constant at 0x100a50a4 reads as the four bytes 00 00 00 00, so
//     the comparison is against 0.0f, and `fcomps` + `fnstsw`/`sahf` +
//     `jbe` (taken when st0 <= 0) sends the loop on to the NEXT CORNER,
//     so the source's test is `> 0.0f` and its body is the break.
//   * the dot product is emitted z-term first, y, faddp, x, faddp -- the
//     "last-term-backwards" association of a three-term float sum, i.e.
//     `n.x*v.x + n.y*v.y + n.z*v.z` written left to right.
//   * `fsub %st(1),%st` against the plane's +0x0c, which is LTPlane's
//     m_Dist, and %st(1) is loaded ONCE per plane before the corner loop:
//     the plane is the loop invariant, the corner is not.
//   * the plane's SIGNS: corner 0 is (+,+,+) and corner 7 is (-,-,-),
//     with the index's bit 0 selecting y, bit 1 x and bit 2 z -- read off
//     the eight `fadds`/`fsubs` groups in order.  Every `-` is
//     `flds centre; fsubs dims`, i.e. centre - dims, and never the other
//     way round.
//   * the two exits: `add $0x64,%esp; xor %eax,%eax; inc %eax` (TRUE,
//     reached when the plane loop runs out) and `add $0x64,%esp;
//     xor %eax,%eax; eb ed` (FALSE, reached when a corner loop completes
//     without breaking) -- the second CROSS-JUMPS, so this TU is /Os.
//
//  RESOLVED (byte-identical).  Three source facts the first version had
//  wrong, each measured in isolation:
//   * the corners are `LTVector::Init(x, y, z)`, not twenty-four member
//     stores: all three components are computed onto the FPU stack (z, x,
//     y) and THEN stored (`fxch %st(2); fstps +8; fstps +0; fstps +4`).
//     That also flips every `+` to `flds dims; fadds pos` -- coin-flip 6
//     settled, the source order is `m_vPos.c + m_vDims.c` inside Init.
//   * the test is `pPlane->m_Normal.Dot(v) - pPlane->m_Dist`.  With the
//     three-term sum written out, icl hoists all four plane floats onto
//     the FPU stack; through LTVector::Dot it reloads the normal per corner
//     and hoists only m_Dist.  Through a `LTPlane*` local, every plane read
//     uses the one `mov $g_Planes,%ecx` base (indexing g_Planes[i] direct
//     folds the m_Dist read to a disp32 instead).  LTPlane::DistTo is
//     NOT it: that stops m_Dist being hoisted too.
//   * the inner exit is the `goto` form (coin-flip 1): with `if (j == 8)`
//     j stays live and is kept in a register (`inc; cmp $8`); with the
//     goto it is replaced by the `cmp %edx,%ebp; jg` pointer test.
// ======================================================================= //

struct Obj1001e770
{
	uint8    m_pad_00[0x0c];                            // +0x00
	LTVector m_vPos;                                    // +0x0c
	LTVector m_vDims;                                   // +0x18
};

extern LTPlane g_Planes1008bc50[6];                     // 0x1008bc50


LTBOOL sub_1001e770(Obj1001e770* pObject)
{
	LTVector vCorners[8];
	int32    i, j;

	vCorners[0].Init(pObject->m_vPos.x + pObject->m_vDims.x,
	                 pObject->m_vPos.y + pObject->m_vDims.y,
	                 pObject->m_vPos.z + pObject->m_vDims.z);
	vCorners[1].Init(pObject->m_vPos.x + pObject->m_vDims.x,
	                 pObject->m_vPos.y - pObject->m_vDims.y,
	                 pObject->m_vPos.z + pObject->m_vDims.z);
	vCorners[2].Init(pObject->m_vPos.x - pObject->m_vDims.x,
	                 pObject->m_vPos.y + pObject->m_vDims.y,
	                 pObject->m_vPos.z + pObject->m_vDims.z);
	vCorners[3].Init(pObject->m_vPos.x - pObject->m_vDims.x,
	                 pObject->m_vPos.y - pObject->m_vDims.y,
	                 pObject->m_vPos.z + pObject->m_vDims.z);
	vCorners[4].Init(pObject->m_vPos.x + pObject->m_vDims.x,
	                 pObject->m_vPos.y + pObject->m_vDims.y,
	                 pObject->m_vPos.z - pObject->m_vDims.z);
	vCorners[5].Init(pObject->m_vPos.x + pObject->m_vDims.x,
	                 pObject->m_vPos.y - pObject->m_vDims.y,
	                 pObject->m_vPos.z - pObject->m_vDims.z);
	vCorners[6].Init(pObject->m_vPos.x - pObject->m_vDims.x,
	                 pObject->m_vPos.y + pObject->m_vDims.y,
	                 pObject->m_vPos.z - pObject->m_vDims.z);
	vCorners[7].Init(pObject->m_vPos.x - pObject->m_vDims.x,
	                 pObject->m_vPos.y - pObject->m_vDims.y,
	                 pObject->m_vPos.z - pObject->m_vDims.z);

	for (i = 0; i < 6; i++)
	{
		LTPlane* pPlane = &g_Planes1008bc50[i];

		for (j = 0; j < 8; j++)
		{
			if (pPlane->m_Normal.Dot(vCorners[j]) - pPlane->m_Dist > 0.0f)
				goto NextPlane;
		}

		return LTFALSE;

NextPlane:;
	}

	return LTTRUE;
}


// *********************************************************************** //
//  COIN-FLIPS -- each is ONE build to settle, and the alternative spelling
//  is written out so it can be pasted in.
// *********************************************************************** //
//
//  1. SETTLED: the goto form (see 0x1001e770's header).  Originally
//     0x1001e770, the inner loop's two exits.  Written above as
//
//         for (j = 0; j < 8; j++) { if (... > 0.0f) break; }
//         if (j == 8) return LTFALSE;
//
//     which needs icl to thread the two exit edges and drop the compare
//     (the shipped code has no `cmp $0x8` anywhere).  If a `cmp $0x8,%edx`
//     turns up, the source was the goto form instead:
//
//         for (i = 0; i < 6; i++)
//         {
//             for (j = 0; j < 8; j++)
//             {
//                 if (... > 0.0f)
//                     goto NextPlane;
//             }
//             return LTFALSE;
//         NextPlane:;
//         }
//         return LTTRUE;
//
//  2. 0x10068a50, the `objectSet.m_pUnknown10 = &objectSet;` store.  The
//     shipped order of the five initialising stores is +0x10, +0x00,
//     +0x04, +0x18, +0x14, and +0x10 gets `lea -0x1020(%ebp),%eax`, the
//     record's own address.  Written above as a first statement.  The
//     alternative, if the store lands in the wrong place, is that the
//     record is CONSTRUCTED and the +0x10 store is the constructor's:
//
//         D3DObjectSet objectSet(apObjects, 0x400, "");
//
//     with an inline constructor whose body is
//     `m_pUnknown10 = this; m_pElements = p; m_nElements = 0;
//      m_pszName = s; m_nMaxElements = n;`.
//
//  3. 0x10068a50, the second loop's guard.  `test %eax,%eax; jbe done;
//     mov $0x0,%ebx; jbe done` is TWO jbe off one test (the `mov $0x0`
//     is the five-byte form precisely so it does not disturb the flags).
//     Written above as a plain `for`.  If the duplicate guard does not
//     come back, the source wrapped it:
//
//         if (objectSet.m_nElements > 0)
//             for (i = 0; i < objectSet.m_nElements; i++) ...
//
//  4. 0x10009f10's element type.  Sixteen bytes copied as four dword
//     movs.  Written as four named uint32s.  If the copy comes back as
//     `rep movsl` the type is a single 16-byte aggregate
//     (`struct { LTVector v; float f; };` is the obvious candidate for a
//     renderer) and the assignment has to be spelled as four member
//     assignments instead of one struct assignment.
//
//  5. 0x10021ad0 / 0x10009d60's `newSize`.  The header keeps
//     `newSize = m_nElements + 1;` above the branch and icl folds the +1
//     into `lea 0x1(%eax,%edx,1)`, so what is live across the two
//     accessor calls is m_nElements, not newSize.  If the spill pattern
//     at -0x4(%ebp) comes back wrong, drop the variable and write
//     `_AllocateTArray(m_nElements + 1 + m_Cache.GetWantedCache(), pAlloc)`
//     directly.
//
//  6. SETTLED: LTVector::Init with `pos + dims` (see 0x1001e770's header).
//     0x1001e770's corner adds.  EVERY `+` is emitted `flds <dims>;
//     fadds <pos>` (the half-dimension loaded first) while every `-` is
//     `flds <pos>; fsubs <dims>` (which it has no choice about).  Written
//     above as `m_vPos.c + m_vDims.c` on the rule that FPU add
//     order is the scheduler's.  If the twenty-four groups come back with
//     the two loads transposed, the source wrote the adds the other way
//     round -- `pObject->m_vDims.x + pObject->m_vPos.x` -- and only the
//     twelve `+` lines change.
//
//  7. 0x10032f30's entry point.  Named after the four-byte stack adapter
//     at 0x10032f30; both callers actually call the register entry at
//     0x10032f34.  If the result is short by four bytes at the front,
//     rename it sub_10032f34 -- the body does not change.
//
// *********************************************************************** //
