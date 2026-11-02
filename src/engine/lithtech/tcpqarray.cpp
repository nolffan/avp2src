// *********************************************************************** //
//
//  MODULE:   CLU4870F0 (.CPP)  -- scratch reconstruction, lithtech.exe only
//
//            0x004870f0 .. 0x0048822a, fifteen functions, 4,055 bytes.
//
//
//  ---- WHAT THIS BLOCK IS, AND HOW IT WAS IDENTIFIED FROM DATA --------- //
//
//  It is the TAIL of the client's copy of src/engine/server/tcpdriver.cpp:
//  the last two CTCPDriverBase vtable slots, followed by the whole of the
//  `CMoArray<TCPQueryResponse, DefaultCache>` instantiation that the driver
//  carries as its SECOND BASE at +0x50.
//
//  The identification is from .rdata, not from code.  Nine of the fifteen
//  are stored as dwords in two ADJACENT vtables:
//
//      0x004a37a4   27 slots   CTCPDriverBase   slot 21 (0x54) = 0x004870f0
//                                               slot 22 (0x58) = 0x00487160
//      0x004a3810   12 slots   GenList<T>       slots 2..10:
//                              2 GenGetNext      0x00487200
//                              3 GenGetAt        0x00487290
//                              4 GenAppend       0x00487310
//                              5 GenRemoveAt     0x004874f0
//                              6 GenRemoveAll    0x004876e0
//                              7 GenGetSize      0x00491480  (folded away)
//                              8 GenCopyList     0x00487740
//                              9 GenAppendList   0x004878e0
//                             10 GenFindElement  0x00487a90  (just outside)
//                             11 GenSetCacheSize 0x004915f0  (folded away)
//
//  server.dll holds the same pair -- 0x1004e94c (27) and 0x1004e9bc (12) --
//  slot for slot, and src/engine/server/tcpdriver.cpp already names the
//  first.  Lining the two up column by column names every function here:
//  GenList<T>'s twelve pure virtuals are declared in exactly the order the
//  run of pointers has them (collection.h, transcribed from
//  src/LT2/lithshared/stdlith/genlist.h).  The server twins, which are the
//  Rosetta stone for every body below, are:
//
//      lithtech    server        what
//      0x004870f0  0x10037b7c    ConnectTCP            (slot 0x54)
//      0x00487160  0x10037bda    SendTo                (slot 0x58)
//      0x00487200  0x10037c6e    GenGetNext
//      0x00487290  0x10037cf5    GenGetAt
//      0x00487310  0x10037d1e    GenAppend
//      0x004874f0  0x10037d33    GenRemoveAt
//      0x004876e0  0x10037d44    GenRemoveAll
//      0x00487740  0x10037d55    GenCopyList
//      0x004878e0  0x10037e1a    GenAppendList
//      0x00487ac0  0x10037f49    SetSize2
//      0x00487be0  0x10037fb9    InternalNiceSetSize
//      0x00487e50  0x100380c9    Insert2
//      0x004880d0  --            _DeleteAndDestroyArray
//      0x00488170  0x10038329    BaseDelete<T>
//      0x004881c0  0x10038366    BaseNew<T>
//
//  The one string in the block, "ConnectTCP" at 0x004a9b7c, confirms slot
//  0x54: 0x004870f0's RETURN_ERROR pushes it together with "80"
//  (0x004a9b78) -- LT_CANTBINDTOPORT, the code that function returns.
//
//
//  ---- NOT ALREADY RECONSTRUCTED --------------------------------------- //
//
//  src/engine/server/tcpdriver.cpp IS compiled into lithtech.exe
//  (build/engine.sh lists ..\server\tcpdriver.cpp) and matches 21 functions
//  there, including ??0TCPQueryResponse at 0x00487df0, which sits INSIDE
//  this address range and is deliberately left out of the fifteen.  But:
//
//    * slots 0x54 / 0x58 are pure `_p54()` / `SendTo()` declarations in its
//      CTCPDriverBase and have no bodies anywhere;
//    * its `TCPQueryArray` is a hand-written stand-in whose members are
//      "declared, not defined" (tcpdriver.cpp lines 79-130), on the belief
//      that the bodies live above server.dll's app-region cut.
//
//  None of the fifteen addresses below is matched by any of our code in
//  either binary.  src/engine/server/collection.cpp knows about this array
//  as well and says so, in its "what is still open" list, as
//  "the 0xa4 walk, which does one thing more than the template's" -- the
//  "one thing more" being TCPQueryResponse::operator=, which strdups the
//  pointer at +0x98 instead of copying it.
//
//
//  ---- WHY THIS IS A SEPARATE FILE ------------------------------------- //
//
//  Making TCPQueryArray a real `CMoArray<TCPQueryResponse>` changes
//  tcpdriver.cpp's own constructor and destructor codegen, and that file is
//  shared with server.dll.  Everything here therefore carries a `C4` suffix
//  so it can be compiled beside src/engine/lithtech/* without colliding
//  with tcpdriver.obj's TCPDriver / TCPQueryResponse / TCPQueryArray.
//
//  BUILD (the line build/engine.sh gives every other lithtech TU):
//    cl /nologo /MT /W3 /O2 /D WIN32 /D NDEBUG /D _WINDOWS /D NO_PRAGMA_LIBS
//       /I LT2\sdk\inc /I src/engine\server /I src/engine\lithtech /c
//
// *********************************************************************** //

#include <winsock2.h>
#include <string.h>

#include "ltbasetypes.h"
#include "ltcodes.h"
#include "de_error.h"        // src/engine/server -- RETURN_ERROR
#include "collection.h"      // src/engine/server -- LAlloc, GenList, CMoArray

// ltbasetypes.h is the only SDK header collection.h reaches, so LTTRUE and
// LTFALSE are in scope and LTNULL is not (ltbasedefs.h has it).  Null is
// spelled 0 below for that reason, not by preference -- the same position
// src/engine/lithtech/stdlithalloc.cpp compiles from.


// The engine allocator front ends, as src/engine/lithtech/stdlithalloc.cpp
// already names them for this image.  Both are one-argument cdecl
// forwarders and both are relocations here, so the choice between spelling
// them `dalloc`/`dfree` and `::operator new`/`::operator delete` is invisible
// to the byte oracle; this is the spelling tcpdriver.cpp's own
// TCPQueryResponse uses.
void* dalloc(unsigned long size);        // 0x004290b0
void  dfree(void* ptr);                  // 0x004290c0


// ======================================================================= //
//  THE ELEMENT TYPE
//
//  0xa4 bytes.  The layout is src/engine/server/tcpdriver.cpp's
//  TCPQueryResponse verbatim -- that class's constructor is already
//  byte-identical at 0x00487df0 -- and the three special members below are
//  read out of this block's inline expansions.
//
//  EVERY ONE OF THEM IS DEFINED IN-CLASS, and that is a finding, not a
//  style choice.  The shipped build inlines all three at some call sites
//  and calls an out-of-line copy at others, in the same object:
//
//      constructor   inlined  0x004881c0 (BaseNew), 0x00487b76 (SetSize2)
//                    called   0x00487c76 (InternalNiceSetSize -> 0x00487df0)
//      operator=     expanded (anchors: the `rep movsl $0x29` of each)
//                             0x00487230, 0x004872bd, 0x00487488, 0x00487655,
//                             0x00487808, 0x004879b3, 0x00487d0e
//                    called   0x00487361, 0x0048739a, 0x004873cd, 0x00487446,
//                             0x00487548, 0x0048757d, 0x00487eaf ...
//                             (all to 0x004853c0)
//      destructor    inlined  0x00488192 (BaseDelete), 0x004880f4
//                             (_DeleteAndDestroyArray), 0x0048785d
//                             (GenCopyList's by-value temp)
//                    called   0x00487d94 (InternalNiceSetSize -> 0x00487e30)
//                             0x004873f1 0x004875b4 0x00487705 0x00487af7
//                             0x00487f3f 0x00485c78 (-> 0x00488230)
//
//  A member that is only DECLARED cannot be inlined at all, so all three
//  are in-class here and VC6's per-function inline budget is left to make
//  the same split it made in 2000.
//
//  THE ONE THING I COULD NOT PIN.  The destructor has TWO out-of-line
//  entry points in the shipped image and the same source line reaches both:
//
//      0x00487e30   18 bytes   `mov 0x98(%ecx),%eax; test; je; push; call
//                               dfree; pop %ecx; ret`         -- plain ??1
//      0x00488230   44 bytes   the same, then `testb $1,0x8(%esp); je;
//                               push %esi; call dfree; ...; mov %esi,%eax;
//                               ret $4`                       -- ??_G shape
//
//  InternalNiceSetSize's `pTemp->~T()` calls the first with nothing pushed;
//  SetSize2's, GenAppend's, GenRemoveAt's, GenRemoveAll's and Insert2's all
//  call the second with a literal 0 pushed.  Both sites are the SAME line of
//  l_allocator.h's BaseDelete, inlined at the same depth, in one object.
//  src/engine/server/collection.cpp's rule ("non-virtual is a direct call,
//  virtual is push $0 / call *(%eax)") does not cover a direct call WITH a
//  push, and no third spelling of an explicit destructor call is known here.
//  It costs two bytes at each of the five `push $0` sites; nothing else in
//  any of the fifteen depends on it, and every call still lands in the same
//  position, so the call sequence is unaffected either way.
// ======================================================================= //

class TCPQueryResponseC4
{
public:

	// 0x00487df0 -- 52 bytes, already byte-identical from tcpdriver.cpp.
	// The store order is the shipped one: +0x98 and +0x9c go down FIRST,
	// then +0x80, then the 0x80-byte memset, then +0xa0 and +0x84.
	TCPQueryResponseC4()
	{
		m_pData = 0;
		m_Unknown9c = 0.0f;
		m_Unknown80 = 0;
		memset(m_Text, 0, sizeof(m_Text));
		m_UnknownA0 = 0.0f;
		m_Time = 0.0f;
	}

	// GenGetNext (0x00487200) and GenGetAt (0x00487290) open by storing a
	// literal zero to the return object's +0x98 and then run operator= on
	// it.  That is this copy constructor expanded: the member that owns the
	// heap block has to be null before the assignment can free it.
	TCPQueryResponseC4(const TCPQueryResponseC4& other)
	{
		m_pData = 0;
		*this = other;
	}

	// 0x00487e30 / 0x00488230.
	//
	// The named local is measured, not style.  Spelled `if (m_pData)
	// dfree(m_pData);` the body is cheap enough that /Ob1 expands it inside
	// InternalNiceSetSize's _DeleteAndDestroyArray -> BaseDelete, where the
	// shipped build CALLS 0x00487e30 (0x00487d94); through a local it is
	// called there, InternalNiceSetSize (0x00487be0, 513) goes identical and
	// the out-of-line ??1 is emitted and pairs.  Both out-of-line bodies are
	// unchanged byte for byte.  A `m_pData = 0` after the free does the
	// same to InternalNiceSetSize but adds a store the shipped 18 bytes
	// do not have.
	~TCPQueryResponseC4()
	{
		char* pData = m_pData;
		if (pData)
			dfree(pData);
	}

	// 0x004853c0 (outside this block; the out-of-line copy of this body).
	//
	// Read off the expansion at 0x0048763b, which is the clearest one:
	//   * the free of the destination's own block comes FIRST -- and in the
	//     copy constructor above, where the destination was just nulled,
	//     VC6 folds the test away entirely (0x0048721c has the store and no
	//     test at all);
	//   * `rep movsl` with $0x29 in ecx and no byte tail: the whole 0xa4
	//     object, sizeof() spelled out, not a member-by-member copy;
	//   * the length is computed from `other.m_pData` and not from the
	//     just-copied `m_pData` -- the shipped code re-reads the SOURCE
	//     pointer at 0x0048765b after the block copy;
	//   * `strlen(..)+1` is the `or $-1,%ecx; repnz scasb; not %ecx` idiom,
	//     and the strlen is computed TWICE, once for the size and once
	//     inside the intrinsic strcpy.  Written as two calls, not cached.
	//   * the return type is void: every call site ignores eax, and the
	//     server twin at 0x10037c9c leaves it undefined.
	void operator=(const TCPQueryResponseC4& other)
	{
		if (m_pData)
			dfree(m_pData);

		memcpy(this, &other, sizeof(TCPQueryResponseC4));

		if (other.m_pData)
		{
			m_pData = (char*)dalloc(strlen(other.m_pData) + 1);
			strcpy(m_pData, other.m_pData);
		}
	}

	// STAND-IN -- see "WHERE THE SHIPPED BUILD CALLS operator=" below.
	// AssignCopy is the out-of-line copy of operator= at 0x004853c0, i.e.
	// tcpdriver.cpp's TCPQueryResponse::operator=, which link/aliases.rsp
	// maps this name onto; nothing here defines it.  AssignOutOfLine is the
	// site spelling: one inline level that emits no code of its own, whose
	// expansion is charged to the caller's /Ob1 budget the way the refused
	// operator= sites evidently were (without it standalone Insert2 has
	// budget left and expands BaseNew, which 0x00487e86 calls: 661 bytes
	// against 634).
	void AssignCopy(const TCPQueryResponseC4& other);
	void AssignOutOfLine(const TCPQueryResponseC4& other) { AssignCopy(other); }

	char               m_Text[0x80];         // 0x00
	uint32             m_Unknown80;          // 0x80  0xff for a broadcast sweep
	float              m_Time;               // 0x84  when the request went out
	struct sockaddr_in m_Addr;               // 0x88
	char*              m_pData;              // 0x98
	float              m_Unknown9c;          // 0x9c
	float              m_UnknownA0;          // 0xa0
};


// The driver's second base.  DefaultCache, so 0x14 bytes: vptr, m_pArray,
// m_nElements, m_CacheSize, m_WantedCache -- and every function below reads
// them at exactly those offsets (0x04, 0x08, 0x0c, 0x10).
typedef CMoArray<TCPQueryResponseC4, DefaultCache> TCPQueryArrayC4;


// ======================================================================= //
//  THE INSTANTIATION
//
//  Thirteen of the fifteen functions are template bodies out of
//  collection.h, so the only thing this file contributes for them is the
//  element type above and the instantiation below.  Every body was checked
//  instruction by instruction against the shipped bytes; what follows is
//  that audit, one function per block, with the addresses it was read at.
//
//  The instantiation is EXPLICIT, as src/engine/server/collection.cpp's is.
//  The shipped build instantiated implicitly -- the vtable pulls in all
//  twelve virtuals, GenAppend pulls Append/Insert/Insert2, GenRemoveAt
//  pulls Remove/Remove2, GenRemoveAll pulls Term/SetSize2, GenAppendList
//  pulls NiceSetSize/InternalNiceSetSize, GenFindElement pulls
//  MoArray_FindElementMemcmp -- which is exactly the set the image holds.
//  An explicit instantiation adds Init, CopyArray2, SetSizeInit* and the
//  other thin wrappers that the image does NOT contain; those are "not in
//  the image", not wrong, and they stay unmatched.
//
//
//  0x00487200  GenGetNext(GenListPos& pos) const      ret $0x8, 1 call
//  ----------------------------------------------------------------------
//    `T& ret = Get(pos.m_Index); ++pos.m_Index; return ret;`
//    The return is BY VALUE, so [esp+4] is the hidden return object and
//    [esp+8] is &pos -- 0x00487200 reads 0x8(%esp) for pos and 0xc(%esp)
//    (after two pushes) for the buffer.  0x0048720b picks the index up,
//    0x00487219 `inc %eax` / `mov %eax,(%edx)` is the increment, and the
//    element address is `lea (%eax,%eax,4)` / `lea (%eax,%esi,8)` /
//    `lea (%ecx,%esi,4)` -- x*41*4 = x*0xa4, the element size, reached
//    without a multiply.  Then the copy constructor: `movl $0x0,0x98(%ebp)`
//    at 0x0048721c and operator= expanded with its leading free FOLDED
//    AWAY (VC6 can see the store it just made).
//    calls: dalloc                                            [1 direct]
//
//  0x00487290  GenGetAt(GenListPos& pos) const        ret $0x8, 1 call
//  ----------------------------------------------------------------------
//    The same function without the `++`; 0x00487299 reads (%eax) and never
//    writes it back.  Identical copy-constructor expansion.
//    calls: dalloc                                            [1 direct]
//
//  0x00487310  GenAppend(T& toAppend)                 ret $0x4, 9 calls
//  ----------------------------------------------------------------------
//    `Append(toAppend)` -> `Insert(m_nElements, toAdd)` ->
//    `Insert2(index, toInsert, &g_DefAlloc)`, all three expanded, so the
//    body IS Insert2 with `index == m_nElements` propagated: 0x00487428's
//    `cmp %esi,%esi` is the cached branch's `i > index` test with both
//    sides known equal, and the `for(i=index; i<m_nElements; i++)` middle
//    loop is still emitted (0x0048737e) because VC6 does not prove it
//    empty.  0x004b4e88 is g_DefAlloc.
//    calls: BaseNew, op=, op=, op=, ~T, [Free], op=, dfree, dalloc
//                                              [8 direct + 1 indirect]
//
//  0x004874f0  GenRemoveAt(GenListPos pos)            ret $0x8, 7 calls
//  ----------------------------------------------------------------------
//    GenListPos goes BY VALUE and is 8 bytes (BaseGenListPos's union plus
//    m_SubIndex), hence `ret $0x8` for a one-argument call; only the first
//    dword is read (0x0048751c reads 0x1c(%esp)).  Body is
//    `Remove(pos.m_Index)` -> `Remove2(index, &g_DefAlloc)` expanded:
//    0x004874ff `lea (%eax,%eax,1)` is `m_WantedCache*2`, the shrink test.
//    calls: BaseNew, op=, op=, ~T, [Free], dfree, dalloc
//                                              [6 direct + 1 indirect]
//
//  0x004876e0  GenRemoveAll()                         ret,      2 calls
//  ----------------------------------------------------------------------
//    `Term()` -> `SetSize2(0, &g_DefAlloc)` expanded with newSize == 0, so
//    the `(newSize == m_nElements) && (newSize != 0)` guard folds away
//    entirely and the `if(newSize > 0)` tail with it.  What is left is the
//    teardown, `m_nElements = newSize` (0x0048772b / 0x00487731) and
//    nothing else.  Note the two `test %ebp,%ebp` at 0x004876e7 and
//    0x004876f3 -- SetSize2's `if(m_pArray)` and BaseDelete's `if(!ptr)`,
//    the guarded-inline duplication.
//    calls: ~T, [Free]                         [1 direct + 1 indirect]
//
//  0x00487740  GenCopyList(const GenList<T>& theList) ret $0x4, 10 calls
//  ----------------------------------------------------------------------
//    genlist.h's body unchanged.  The frame is 0xbc bytes and 0xa4 of it
//    (B+0x28..B+0xcc) is the by-value temporary GenGetNext returns into --
//    which is what the `mov 0xc0(%esp),%eax; test; call dfree` at
//    0x00487851 destroys: the temp's destructor at the end of the full
//    expression, inlined.  iCurOut lives at B+0x10 and its strength-reduced
//    byte offset at B+0x14; `pos` is at B+0x1c/B+0x20.
//    calls: [GenGetSize], SetSize2, [GenBegin], [GenIsValid],
//           [GenGetNext], dfree, dalloc, dfree, [GenIsValid], BaseDelete
//                                              [5 direct + 5 indirect]
//
//  0x004878e0  GenAppendList(const GenList<T>& theList) ret $0x4, 10 calls
//  ----------------------------------------------------------------------
//    The same walk over `NiceSetSize(prevSize + theList.GenGetSize())`.
//    0x00487904 pushes g_DefAlloc, 0 and the size in that order, which is
//    `InternalNiceSetSize(newSize, LTFALSE, &g_DefAlloc)` -- the LTFALSE
//    is NiceSetSize's, not Fast_NiceSetSize's.
//    calls: [GenGetSize], InternalNiceSetSize, [GenBegin], [GenIsValid],
//           [GenGetNext], dfree, dalloc, dfree, [GenIsValid], BaseDelete
//                                              [5 direct + 5 indirect]
//
//  0x00487ac0  SetSize2(uint32 newSize, LAlloc* pAlloc)  ret $0x8, 3 calls
//  ----------------------------------------------------------------------
//    0x00487acc/0x00487ad1 are the two halves of
//    `(newSize == m_nElements) && (newSize != 0)`; 0x00487ae0 `add %ecx,%eax`
//    is GetNumAllocatedElements().  _AllocateTArray AND its BaseNew are
//    both expanded here (unlike GenAppend, where BaseNew survives as a
//    call), which is why the element constructor appears inline at
//    0x00487b76 -- including the placement-new null test
//    (`lea -0x9c(%edx),%eax; test %eax,%eax; je`) that
//    `::new(pTemp,(DummyClassBlahBlah*)0) T` always emits.
//    calls: ~T, [Free], [Alloc]                [1 direct + 2 indirect]
//
//  0x00487be0  InternalNiceSetSize(uint32,LTBOOL,LAlloc*) ret $0xc, 6 calls
//  ----------------------------------------------------------------------
//    All three trivial rejects are present and in source order: `<`
//    (0x00487bf5), `>` with room in the cache (0x00487c12..0x00487c1b),
//    `==` (0x00487c33).  bFast is a runtime argument here (0x00487c95
//    tests 0x24(%esp)), so BOTH arms of `if(bFast)` are emitted -- the
//    `rep movsl` block at 0x00487c99 and the element-by-element loop at
//    0x00487ce8.  This is the ONE function in the block where the element
//    constructor and destructor survive as ordinary direct calls.
//    calls: [Alloc], ctor, dfree, dalloc, ~T, [Free]
//                                              [4 direct + 2 indirect]
//
//  0x00487e50  Insert2(uint32, const T&, LAlloc*)     ret $0xc, 10 calls
//  ----------------------------------------------------------------------
//    0x00487e5d `cmp %eax,%esi; jbe` is `if(index > m_nElements) return
//    LTFALSE` with the error arm falling through.  Fresh-array branch
//    first (m_CacheSize == 0, 0x00487e6f), cached branch at 0x00487f80.
//    The cached branch expands operator= twice (0x00487fb9/0x00487fe1 and
//    0x00488054/0x0048807f are the two dfree/dalloc pairs) where the
//    fresh branch calls it three times.
//    calls: BaseNew, op=, op=, op=, ~T, [Free], dfree, dalloc,
//           dfree, dalloc                      [9 direct + 1 indirect]
//
//  0x004880d0  _DeleteAndDestroyArray(LAlloc*, uint32) ret $0x8, 2 calls
//  ----------------------------------------------------------------------
//    `if(m_pArray) { LDelete_Array(...); m_pArray = 0; }
//     m_Cache.SetCacheSize(0);` -- and the shipped tail proves the
//    ordering: 0x00488113 stores 0 to +0x04 and 0x00488116 to +0x0c on the
//    taken path, while the not-taken path (0x0048811e) stores only +0x0c.
//    BaseDelete is expanded WITH the element destructor expanded inside it.
//    calls: dfree, [Free]                      [1 direct + 1 indirect]
//
//  0x00488170  BaseDelete<T>(LAlloc*, T*, uint32)     ret,      2 calls
//  ----------------------------------------------------------------------
//    l_allocator.h's free function, out of line.  `if(!ptr) return;` at
//    0x00488175, `while(nElements)` pre-tested at 0x0048817d, the
//    destructor expanded in the loop, `pAlloc->Free(ptr)` at 0x004881ac.
//    The element pointer is carried as `&pTemp->m_pData` (0x00488183 leas
//    +0x98 once and strides 0xa4), which is the destructor's only member.
//    calls: dfree, [Free]                      [1 direct + 1 indirect]
//
//  0x004881c0  BaseNew<T>(LAlloc*, T*, uint32)        ret,      1 call
//  ----------------------------------------------------------------------
//    The sibling.  `pAlloc->Alloc(sizeof(T)*nElements)` with the size
//    formed as x*41*4 (0x004881cd), `if(!ptr) return NULL`, then the
//    construction loop with the element constructor expanded and the
//    placement-new null test in front of it.  The middle argument, the
//    `(T*)0` type carrier that LNew_Array passes, is accepted and never
//    read -- it is the literal zero every caller pushes.
//    calls: [Alloc]                            [0 direct + 1 indirect]
// ======================================================================= //

// ======================================================================= //
//  WHERE THE SHIPPED BUILD CALLS operator= -- STAND-IN SPECIALIZATIONS
//
//  0x00487e50 Insert2, 0x00487310 GenAppend and 0x004874f0 GenRemoveAt are
//  collection.h's bodies with one difference: WHICH operator= sites /Ob1
//  expanded.  In every one of them the shipped build expands the LAST
//  site(s) and calls 0x004853c0 at the earlier ones:
//      Insert2      BN op op op G [Free] | op* op*       (* = expanded)
//      GenAppend    BN op op op G [Free] | op  op*
//      GenRemoveAt  BN op op G [Free]    | op*
//  and in Insert2 it also calls BaseNew (depth 2, 0x00487e86) while it
//  expands BaseDelete (depth 2, later in the source).  All of that is one
//  rule: the per-function budget spent from the END of the function
//  backwards, level by level.
//
//  Our cl 8447 does not spend it that way.  Measured with throwaway probes
//  (a heavy inline at 11..20 sites, straight-line and in loops, in plain
//  functions, template members, virtual members, if/else arms; /O2, /Ox,
//  /Ob2, /Zi, /Z7, /GX, /G6, /Oa, /Ow, /Op, cl 8168/8447/8966): it spends
//  it FORWARD within a level, and a site one inline level deeper is
//  considered only after every shallower one (wrap the first sites in an
//  inline forwarder and they become the calls).  So with collection.h's
//  bodies ours expands all five sites of Insert2 (996 bytes against 634),
//  and making operator= costlier cuts the LAST sites first -- the mirror
//  image of the shipped split (see STILL NEAR at the foot, now closed).
//
//  No source order reaches the shipped split with this compiler, so the
//  three bodies are written out here as explicit specializations for this
//  element, line for line collection.h's, with the called sites spelled
//  AssignOutOfLine.  `inline` keeps them expandable (GenAppend reaches
//  Insert2's body through Append -> Insert, GenRemoveAt Remove2's through
//  Remove) and VC6 still emits Insert2 out of line for StartQuery's call
//  (0x00485185).  Insert is specialized rather than reusing Insert2
//  because GenAppend's expansion calls operator= in the cached-branch
//  loop too (0x00487446) where standalone Insert2 expands it -- the same
//  line, two budgets.  With all three: Insert2 634, GenAppend 475,
//  GenRemoveAt 486, byte-identical.
// ======================================================================= //

template<> inline
LTBOOL CMoArray<TCPQueryResponseC4, DefaultCache>::Insert2(uint32 index,
	const TCPQueryResponseC4& toInsert, LAlloc* pAlloc)
{
	TCPQueryResponseC4* pNewArray;
	uint32 newSize, i;

	if(index > m_nElements)
		return LTFALSE;

	newSize = m_nElements + 1;

	if( m_Cache.GetCacheSize() == 0 )
	{
		pNewArray = _AllocateTArray( newSize + m_Cache.GetWantedCache(), pAlloc );
		if( !pNewArray )
			return LTFALSE;

		for( i=0; i < index; i++ )
			pNewArray[i].AssignOutOfLine(m_pArray[i]);

		for( i=index; i < m_nElements; i++ )
			pNewArray[i+1].AssignOutOfLine(m_pArray[i]);

		pNewArray[index].AssignOutOfLine(toInsert);

		if( m_pArray )
		{
			_DeleteAndDestroyArray(pAlloc, GetNumAllocatedElements());
		}

		m_Cache.SetCacheSize(m_Cache.GetWantedCache());
		m_pArray = pNewArray;
	}
	else
	{
		for( i=m_nElements; i > index; i-- )
			m_pArray[i] = m_pArray[i-1];

		m_Cache.SetCacheSize(m_Cache.GetCacheSize() - 1);

		m_pArray[index] = toInsert;
	}

	++m_nElements;

	return LTTRUE;
}

// Insert2's body as GenAppend expands it (Append -> Insert -> Insert2 in
// collection.h): the cached-branch loop is a call here, 0x00487446.
template<> inline
LTBOOL CMoArray<TCPQueryResponseC4, DefaultCache>::Insert(uint32 index,
	const TCPQueryResponseC4& toInsert)
{
	TCPQueryResponseC4* pNewArray;
	uint32 newSize, i;

	if(index > m_nElements)
		return LTFALSE;

	newSize = m_nElements + 1;

	if( m_Cache.GetCacheSize() == 0 )
	{
		pNewArray = _AllocateTArray( newSize + m_Cache.GetWantedCache(), &g_DefAlloc );
		if( !pNewArray )
			return LTFALSE;

		for( i=0; i < index; i++ )
			pNewArray[i].AssignOutOfLine(m_pArray[i]);

		for( i=index; i < m_nElements; i++ )
			pNewArray[i+1].AssignOutOfLine(m_pArray[i]);

		pNewArray[index].AssignOutOfLine(toInsert);

		if( m_pArray )
		{
			_DeleteAndDestroyArray(&g_DefAlloc, GetNumAllocatedElements());
		}

		m_Cache.SetCacheSize(m_Cache.GetWantedCache());
		m_pArray = pNewArray;
	}
	else
	{
		for( i=m_nElements; i > index; i-- )
			m_pArray[i].AssignOutOfLine(m_pArray[i-1]);

		m_Cache.SetCacheSize(m_Cache.GetCacheSize() - 1);

		m_pArray[index] = toInsert;
	}

	++m_nElements;

	return LTTRUE;
}

// Remove2's body as GenRemoveAt expands it: both fresh-array loops call
// (0x00487548, 0x0048757d), the slide-down loop expands (0x0048764e).
template<> inline
void CMoArray<TCPQueryResponseC4, DefaultCache>::Remove2(uint32 index,
	LAlloc* pAlloc)
{
	uint32 i, newSize, newAllocSize;
	TCPQueryResponseC4* pNewArray;
	LTBOOL bSlideDown;

	bSlideDown = LTTRUE;
	if( m_Cache.GetCacheSize() >= (m_Cache.GetWantedCache()*2) )
	{
		newSize = m_nElements - 1;
		newAllocSize = newSize + m_Cache.GetWantedCache();
		pNewArray = _AllocateTArray(newAllocSize, pAlloc);

		if( pNewArray || newAllocSize == 0 )
		{
			for( i=0; i < index; i++ )
				pNewArray[i].AssignOutOfLine(m_pArray[i]);

			for( i=index; i < m_nElements-1; i++ )
				pNewArray[i].AssignOutOfLine(m_pArray[i+1]);

			_DeleteAndDestroyArray(pAlloc, GetNumAllocatedElements());
			m_pArray = pNewArray;

			m_Cache.SetCacheSize(m_Cache.GetWantedCache());
			bSlideDown = LTFALSE;
		}
	}

	if( bSlideDown )
	{
		m_Cache.SetCacheSize(m_Cache.GetCacheSize() + 1);

		for( i=index; i < m_nElements-1; i++ )
			m_pArray[i] = m_pArray[i+1];
	}

	--m_nElements;
}

template class CMoArray<TCPQueryResponseC4, DefaultCache>;


// ======================================================================= //
//  THE TWO DRIVER SLOTS
//
//  Only enough of the driver to put its members at the right offsets and
//  give the two bodies somewhere to live.  The layout is
//  src/engine/server/tcpdriver.cpp's, unchanged: CTCPDriverBase is 0x50
//  bytes (vptr, m_Name[0x40], m_DriverFlags, m_pNetMgr, m_Timeout), the
//  query array is the second base at +0x50 and is 0x14 bytes, and the
//  driver's own fields start at 0x64.  The only one either function below
//  touches is m_Socket at +0x90, which both read straight off `this` --
//  0x00487166 and 0x004871bb.
//
//  Both bodies are TCPDriver's, not the base's: they read TCPDriver
//  members and call TCPDriver's own JoinSession and SendTo directly.  The
//  class stays ABSTRACT (25 of the 27 slots left pure), so no vtable, no
//  constructor and no destructor are emitted from this file; the two
//  bodies are all it contributes.
// ======================================================================= //

class CNetMgrC4;
class NetConnectionC4;
class NetServiceC4;
class NetSessionC4;
class NetHostC4;
class ServerMessageC4;

// 0x00484a80 -- parse "a.b.c.d[:port]" into an address (server 0x10035efb).
// tcpdriver.cpp defines it; its own error string names it.
int tcp_BuildSockaddrFromString(char* pText, struct sockaddr_in* pAddr);


class CTCPDriverBaseC4
{
public:
	virtual ~CTCPDriverBaseC4();
	virtual LTBOOL   Init() = 0;                                    // 0x04
	virtual void     Term() = 0;                                    // 0x08
	virtual void     Update() = 0;                                  // 0x0c
	virtual void     _p10() = 0;                                    // 0x10
	virtual LTRESULT GetServiceList(NetServiceC4** ppList) = 0;     // 0x14
	virtual int      SelectService(void* hService) = 0;             // 0x18
	virtual LTRESULT GetSessionList(NetSessionC4** ppList,
	                                char* pAddresses) = 0;          // 0x1c
	virtual LTRESULT StartQuery(char* pAddresses) = 0;              // 0x20
	virtual LTRESULT UpdateQuery() = 0;                             // 0x24
	virtual LTRESULT GetQueryResults(NetSessionC4** ppList) = 0;    // 0x28
	virtual LTRESULT EndQuery() = 0;                                // 0x2c
	virtual LTRESULT HostSession(NetHostC4* pHostInfo) = 0;         // 0x30
	virtual LTRESULT JoinSession(NetSessionC4* pSession) = 0;       // 0x34
	virtual LTRESULT UpdateSessionName(const char* sName) = 0;      // 0x38
	virtual LTRESULT GetSessionName(char* sName, uint32 nLen) = 0;  // 0x3c
	virtual LTBOOL   IsLobbyLaunched() = 0;                         // 0x40
	virtual LTBOOL   GetLobbyLaunchInfo(void** ppData) = 0;         // 0x44
	virtual void     _p48() = 0;                                    // 0x48
	virtual void     _p4c() = 0;                                    // 0x4c
	virtual LTBOOL   GetTcpIpAddress(char* sAddress, uint32 dwBufferSize,
	                                 unsigned short& hostPort) = 0; // 0x50
	// 0x00487200-block's two: slot 0x54 is the only one in this file that
	// names itself, through the "ConnectTCP" literal its RETURN_ERROR
	// pushes.  tcpdriver.cpp calls them `_p54` and `SendTo`.
	virtual LTRESULT ConnectTCP(char* pAddress) = 0;                // 0x54
	virtual LTRESULT SendToAddr(void* pData, uint32 nLen,
	                            char* sAddr, uint32 port) = 0;      // 0x58
	virtual void     Disconnect(NetConnectionC4* pConn, uint32 nReason,
	                            uint32 flags) = 0;                  // 0x5c
	virtual LTBOOL   _p60(void* pData, uint32 nLen, uint32 nUnused,
	                      NetConnectionC4* pConn) = 0;              // 0x60
	virtual LTBOOL   ReceivePacket(ServerMessageC4* pMsg) = 0;      // 0x64
	virtual uint32   GetRecvSize() = 0;                             // 0x68

	char       m_Name[0x44 - 0x04];          // 0x04
	uint32     m_DriverFlags;                // 0x44
	CNetMgrC4* m_pNetMgr;                    // 0x48
	float      m_Timeout;                    // 0x4c
};


class TCPDriverC4 : public CTCPDriverBaseC4, public TCPQueryArrayC4
{
public:
	virtual LTRESULT ConnectTCP(char* pAddress);                    // 0x54
	virtual LTRESULT SendToAddr(void* pData, uint32 nLen,
	                            char* sAddr, uint32 port);          // 0x58

	// 0x004869a0 -- the connect handshake.  Declared, not defined: it is
	// the driver's own 1,752-byte JoinSession, outside this block, and it
	// is reached by a DIRECT call (0x00487151), so it is not the vtable's
	// JoinSession(NetSession*) at slot 0x34.
	LTRESULT JoinSession(struct sockaddr_in* pHostAddr);

	// 0x00484ee0 -- one datagram to one address, already byte-identical
	// from tcpdriver.cpp.  Declared, not defined; also a direct call.
	LTBOOL   SendTo(SOCKET hSocket, void* pData, uint32 nLen,
	                struct sockaddr_in* pTo);

	uint32             m_QuerySocket;        // 0x064
	float              m_QueryEndTime;       // 0x068
	struct sockaddr_in m_QueryAddr;          // 0x06c
	uint32             m_bHosting;           // 0x07c
	struct sockaddr_in m_HostAddr;           // 0x080
	SOCKET             m_Socket;             // 0x090
	uint32             m_MaxPlayers;         // 0x094
	char*              m_pSessionName;       // 0x098
	uint32             m_ConnTable[2];       // 0x09c
	uint32             m_bWinsockUp;         // 0x0a4
	uint32             m_Service[2];         // 0x0a8
};


// ----------------------------------------------------------------------- //
//  0x004870f0  ConnectTCP -- 109 bytes, 4 calls, `ret $0x4`
//
//  `sub $0x10,%esp` at the head is the sockaddr_in; `lea 0x0(%esp),%eax`
//  and later `lea 0x4(%esp),%eax` are the same local at two stack depths.
//  The failure arm is the FALL-THROUGH (0x0048710a `jne` jumps to the
//  success), which is `if (!ok) { report; return code; }`.
//
//  The report is de_error.h's plain RETURN_ERROR and not the _NAMED or
//  _SYM form:  0x00487130 pushes the name as an IMMEDIATE (a string
//  literal, not a `char*` variable holding it), and 0x0048712b pushes
//  "80" -- the NUMBER stringized, which is LT_ERRSTR's double expansion of
//  LT_CANTBINDTOPORT, not `#code`.  The level is 1: `cmp $0x1,%eax; jl`
//  at 0x0048711b, against g_DebugLevel at 0x004b0ab0.  The description is
//  the lone space at 0x004a7164, i.e. LT_ERRDESC.
//
//  calls: tcp_BuildSockaddrFromString, dsi_OnReturnError, dsi_ServerError, JoinSession
// ----------------------------------------------------------------------- //
LTRESULT TCPDriverC4::ConnectTCP(char* pAddress)
{
	struct sockaddr_in addr;

	if (!tcp_BuildSockaddrFromString(pAddress, &addr))
		RETURN_ERROR(1, "ConnectTCP", LT_CANTBINDTOPORT)

	return JoinSession(&addr);
}


// ----------------------------------------------------------------------- //
//  0x00487160  SendToAddr -- 116 bytes, 3 calls, `ret $0x10`
//
//  Four dword arguments; the sockaddr_in local is the same `sub $0x10`.
//  m_Socket is read TWICE -- 0x00487166 for the guard and 0x004871bb for
//  the send -- with a call in between, so it is written twice rather than
//  cached in a local.
//
//  No RETURN_ERROR anywhere: both failures are bare returns, 0x49 =
//  LT_NOTINITIALIZED at 0x00487170 and 0x01 = LT_ERROR at 0x00487192.
//  0x004871a3 is WSOCK32 ordinal 9 = htons, and its 16-bit result goes to
//  0x6(%esp) -- sin_port, the local at 0x4(%esp) plus 2.  The send is the
//  driver's own four-argument SendTo (0x00484ee0), a DIRECT call whose
//  result is discarded, and the function then returns a literal 0.
//
//  calls: tcp_BuildSockaddrFromString, [htons], SendTo
// ----------------------------------------------------------------------- //
LTRESULT TCPDriverC4::SendToAddr(void* pData, uint32 nLen,
                                 char* sAddr, uint32 port)
{
	struct sockaddr_in addr;

	if (!m_Socket)
		return LT_NOTINITIALIZED;

	if (!tcp_BuildSockaddrFromString(sAddr, &addr))
		return LT_ERROR;

	addr.sin_port = htons((unsigned short)port);

	SendTo(m_Socket, pData, nLen, &addr);
	return LT_OK;
}


// ======================================================================= //
//  0x00488130  TCPConnTable::InsertAfter -- 52 bytes, no calls, `ret $0xc`
//
//  REAL HOME: src/engine/server/tcpdriver.cpp, next
//  to TCPConnTable::Add.  Written here, against C4 copies of the two
//  classes, for the same reason as everything else in this file.
//
//  The driver's connection table (+0x9c): a count and the tail of a
//  circular list of three-word nodes (next, prev, carried pointer).  In
//  server.dll Add (0x100261eb) is one /O1 function with the splice written
//  in its non-empty arm.  In lithtech.exe Add is expanded into
//  HandleConnPacket and its non-empty arm is THIS call: 0x004862b0 tests
//  m_pTail, and when it is set pushes the node, the connection and the
//  tail and calls here (0x004862be); the empty arm stays inline
//  (0x004862c5).  So the splice is a helper of its own that /Ob1 did not
//  expand, with the carried pointer between the position and the node --
//  the order the pushes have.  Every store is server Add's, in the same
//  order, down to the reload of pAt->m_pNext between the first two lines.
// ======================================================================= //

class TCPConnNodeC4
{
public:
	TCPConnNodeC4* m_pNext;                  // 0x00
	TCPConnNodeC4* m_pPrev;                  // 0x04
	void*          m_pData;                  // 0x08
};

class TCPConnTableC4
{
public:
	void InsertAfter(TCPConnNodeC4* pAt, void* pData, TCPConnNodeC4* pNode);

	uint32         m_nItems;                 // 0x00
	TCPConnNodeC4* m_pTail;                  // 0x04
};

void TCPConnTableC4::InsertAfter(TCPConnNodeC4* pAt, void* pData,
                                 TCPConnNodeC4* pNode)
{
	pAt->m_pNext->m_pPrev = pNode;
	pNode->m_pNext = pAt->m_pNext;
	pNode->m_pPrev = pAt;
	pAt->m_pNext = pNode;
	pNode->m_pData = pData;

	m_nItems++;

	if (pNode->m_pPrev == m_pTail)
		m_pTail = pNode;
}


// ======================================================================= //
//  ADDRESS MAP -- what this file claims, and what it does not
// ======================================================================= //
//
//  CLAIMED (the fifteen):
//    0x004870f0  109  TCPDriver::ConnectTCP                    slot 0x54
//    0x00487160  116  TCPDriver::SendToAddr                    slot 0x58
//    0x00487200  130  CMoArray<T,DefaultCache>::GenGetNext     slot 2
//    0x00487290  127  CMoArray<T,DefaultCache>::GenGetAt       slot 3
//    0x00487310  475  CMoArray<T,DefaultCache>::GenAppend      slot 4
//    0x004874f0  486  CMoArray<T,DefaultCache>::GenRemoveAt    slot 5
//    0x004876e0   91  CMoArray<T,DefaultCache>::GenRemoveAll   slot 6
//    0x00487740  413  CMoArray<T,DefaultCache>::GenCopyList    slot 8
//    0x004878e0  426  CMoArray<T,DefaultCache>::GenAppendList  slot 9
//    0x00487ac0  274  CMoArray<T,DefaultCache>::SetSize2
//    0x00487be0  513  CMoArray<T,DefaultCache>::InternalNiceSetSize
//    0x00487e50  634  CMoArray<T,DefaultCache>::Insert2
//    0x004880d0   90  CMoArray<T,DefaultCache>::_DeleteAndDestroyArray
//    0x00488170   65  BaseDelete<TCPQueryResponse>
//    0x004881c0  106  BaseNew<TCPQueryResponse>
//
//  ALSO EMITTED BY THIS FILE, and already covered elsewhere or just
//  outside the fifteen -- listed so nobody counts them twice:
//    0x004853c0       TCPQueryResponse::operator=      (see GenAppend note)
//    0x00487a90   45  CMoArray<T,DefaultCache>::GenFindElement  slot 10
//    0x00487df0   52  TCPQueryResponse::TCPQueryResponse
//                     ALREADY BYTE-IDENTICAL from tcpdriver.cpp; this
//                     file's copy is the same COMDAT and must not be
//                     linked beside it.
//    0x00487e30   18  TCPQueryResponse::~TCPQueryResponse
//    0x00488230   44  the flagged destructor thunk (see the note above)
//    0x00491480       GenGetSize   -- folded with every other four-byte
//    0x004915f0       GenSetCacheSize   instantiation; not emitted here
//                     in a distinguishable form
//
//  NOT THE QUERY ARRAY, though the enumeration puts it in the range:
//    0x00488130   52  TCPConnTable::InsertAfter, the connection table's
//                     splice (written above; home tcpdriver.cpp).
//
//  CLOSED -- identical via the stand-in specializations above.  The note
//  as it stood (lithtech 0x00487310 GenAppend 475, 0x004874f0
//  GenRemoveAt 486, 0x00487e50 Insert2 634): every call and every block
//  outside operator= is already the shipped one; what differs is WHICH
//  operator= sites /Ob1 expands.  Shipped expands only the LAST site(s) in
//  source order and calls 0x004853c0 at the earlier ones:
//      Insert2      BN op op op G [Free] | op* op*       (* = expanded)
//      GenAppend    BN op op op G [Free] | op  op*
//      GenRemoveAt  BN op op G [Free]    | op*
//  Ours, with this operator=, expands all of them (959/741/996 bytes).
//  Making operator= costlier (dead padding, a named local for the old
//  pointer or the length, a FreeData() helper) does move the split, but
//  our cl 8447 always degrades from the END -- the late sites become calls
//  first and the early ones stay expanded, the mirror image of the
//  shipped split.  Out-of-class `inline` placed before/after the
//  instantiation, a FreeData() helper (an extra inline level) and implicit
//  instantiation change nothing.  Inline-budget ordering, not reached from
//  source.
// ======================================================================= //
