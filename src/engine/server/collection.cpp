// server.dll — stdlith's growable array, instantiated. Build /O1, cl 8447.
//
// The template itself is in collection.h, and it is a transcription of the
// period source (src/LT2/lithshared/stdlith/{dynarray,genlist,l_allocator}.h)
// rather than a reconstruction. This file is only the instantiation list:
// the element types, recovered one at a time from the element size baked
// into each allocator's `imul`/`shl` and from what the walk does with an
// element, and one explicit instantiation per (element type, cache policy)
// pair that the image actually holds.
//
//
// ---- WHAT THE FAMILY IS ------------------------------------------------ //
//
// One class template, `CMoArray<T, C>`, deriving from the abstract
// `GenList<T>`, over a cache policy `C` that is either `DefaultCache` (two
// extra words) or `NoCache` (none). That is the whole family: there is no
// second array class, no "fixed" variant and no separate pointer array.
// What made it look like three classes is that the policy's accessors fold
// to nothing:
//
//   CMoArray<T, DefaultCache>   0x14   vptr, m_pArray, m_nElements,
//                                      m_CacheSize (0x0c),
//                                      m_WantedCache (0x10)
//   CMoArray<T, NoCache>        0x10   vptr, m_pArray, m_nElements,
//                                      and one byte of `NoCache` itself
//                                      at 0x0c, plus three of padding —
//                                      `C m_Cache;` is a member and an
//                                      empty class member still costs a
//                                      byte. See CCollElem24Rec at the
//                                      foot of this file, whose copy
//                                      constructor is the only thing in
//                                      the image that reads it.
//
// and every visible difference between the two follows from that one fact:
//
//   * `Init(size, cacheSize)` accepts its second argument and never reads
//     it in the NoCache instantiations (0x10019903), because
//     `NoCache::SetWantedCache` has an empty body.
//   * vtable slot 11, `GenSetCacheSize`, is `mov [ecx+0x10],arg; ret $4`
//     for DefaultCache (0x1000b152) and a bare `ret $4` for NoCache
//     (0x1000bd91). That slot is the cheapest way to tell the two apart in
//     the image, and it is what says 0x1004dbf0 is cached while
//     0x1004da34 is not.
//   * `SetSize2` and `InternalNiceSetSize` lose every m_CacheSize and
//     m_WantedCache term (0x1000c470 -> 0x1001a03d, 0x1000c3c1 -> the
//     0x10019633 family) — including the `nItems > m_nItems &&
//     m_nItems >= nItems` guard that VC6 cannot satisfy and will not
//     delete, which is the four-branches-off-one-compare shape.
//   * `_DeleteAndDestroyArray` loses its trailing `SetCacheSize(0)` store
//     (0x1000d020, 32 bytes -> 0x1001aad4, 28 bytes).
//   * the constructor loses two of `Clear()`'s four stores, and — because
//     what is left is small enough — VC6 then inlines `Init` into it as
//     well, which is the whole difference between the 34-byte cached
//     constructor and the 52-byte uncached one.
//
// The twelve virtuals are `GenList<T>`'s, in declaration order, and that
// order is what names every collection vtable in .rdata (the previous
// reconstruction counted thirty of them):
//
//   0 GenBegin        4 GenAppend      8  GenCopyList
//   1 GenIsValid      5 GenRemoveAt    9  GenAppendList
//   2 GenGetNext      6 GenRemoveAll   10 GenFindElement
//   3 GenGetAt        7 GenGetSize     11 GenSetCacheSize
//
// The allocator is `LAlloc` (l_allocator.h), whose vtable is
// { ~LAlloc, Alloc, Free } — which is why every allocation in the image is
// `call *0x4(%eax)` and every release `call *0x8(%edx)`. The one instance
// is `g_DefAlloc` at 0x10058c7c; earlier reconstructions called it
// `CMoAllocator`/`g_MoAlloc`.
//
// Allocation does NOT go straight to the allocator. `_AllocateTArray` is a
// one-line wrapper over the `LNew_Array` macro, which is `BaseNew<T>` — a
// free function template taking (LAlloc*, T* typeCarrier, count). That is
// the answer to two long-standing puzzles:
//
//   * the middle argument every three-argument allocator in the image
//     pushes as a literal zero and never reads is the macro's `(T*)0`
//     type carrier;
//   * the image holds allocators in two shapes for the same element size —
//     a two-argument thiscall member (0x100243de) and a three-argument
//     cdecl free function (0x1001b8fa) — because VC6 inlines exactly one
//     of the two layers. Which one is the per-function inline
//     budget, not two functions in the source.
//
// A destructive read of the same wrapper pair: `_DeleteAndDestroyArray`
// over `LDelete_Array` = `BaseDelete<T>`, which is why the release helper
// takes an element count (stdlith deliberately stores no array cookie) and
// why the count is dead for element types with no destructor.
//
// Two more shapes now have names rather than guesses. `GenFindElement`
// forwards to `MoArray_FindElementMemcmp` (0x1003d4c0, and the period
// header explains in a comment why it is a separate non-template
// function); and the walk's "scratch buffer that is never constructed but
// is destroyed" is not a scratch buffer at all — `GenGetNext` returns `T`
// **by value**, so what the shipped `lea`/thiscall pair after the copy-out
// destroys is the copy's own temporary at the end of the full expression.
// For a four-byte class element that copy is the element's copy
// constructor called on the caller's return buffer (0x1000ba3e); for a
// scalar T it is `mov eax` (0x100240b9). Both are in the image, so four
// bytes is at least three distinct element types.
//
//
// ---- WHAT IS STILL OPEN ----------------------------------------------- //
//
// A later pass closed twenty-two more shipped addresses from here, and
// what it closed says where the rest of the residual is. Three findings
// generalise:
//
//   * A shipped address that "does not match" may only be miscounted.
//     A byte match credits one address per SYMBOL, so where several shipped
//     instantiations are byte-identical (SetSize2 and InternalNiceSetSize
//     embed no element size at all, so a whole cache policy shares one
//     body) only the first is reported. Eight of the thirty addresses in
//     that range were already covered by an existing symbol
//     before a line was written. Count distinct covered ADDRESSES, not
//     matched symbols, before deciding anything here is missing.
//   * The element types are not all opaque. Four more of them earned
//     their place below, each recovered from the one thing about it that
//     is visible — a constructor its BaseNew calls, an assignment its
//     Insert2 calls, a cache policy its SetSize2 implies — and each
//     brought its own array functions with it: a four-byte element with
//     a float-zeroing constructor (CCollElem04F) closed four addresses,
//     an 0x20 element with an out-of-line assignment (CCollElem20A) two
//     plus eleven of its own vtable slots and walks.
//   * Two element types are fully reconstructed at the foot of the file
//     rather than modelled as opaque padding, because their own special
//     members are in the image and those members name every offset.
//
// What is left, and why:
//   * `CopyArray2` and `_AllocateTArray` where VC6 chose the other side
//     of the inline-one-layer coin, plus 0x1001aaf0 (Insert2, 185 bytes
//     against 189), 0x1001a6dd (338) and 0x1001a82f (286). In all of
//     them the shipped build expanded one inline layer that ours leaves
//     as a call or the reverse. 0x1001a6dd is the clearest statement of
//     it: the SAME element assignment is expanded at three call sites in
//     that one function and called at the other two, and the out-of-line
//     copy of it (0x100195b8, matched below) has its own inner
//     assignment expanded because that COMDAT is small.
//
//     0x1001a82f is the same finding on the same element, and needs no
//     separate hunt: it is `CMoArray<CCollElem1CS, DefaultCache>::Remove2`,
//     232 bytes against 286, it has THREE element-copy sites, the shipped
//     build expands two of them (`movsl` triples into
//     CCollInner10::operator= at 0x10038e1f, at 0x1001a897 and 0x1001a8e0)
//     and calls the third (0x100195b8 at 0x1001a932), and the 54-byte
//     residual is exactly those two expansions at 27 bytes each. Everything
//     else in it is instruction-for-instruction, down to the same
//     `xor %ebx,%ebx` zero register the partial spend forces on us. Same
//     PARTIAL spend, same three revisions, same answer -- do not re-derive.
//
//     0x1001a6dd is now measured out, and it is the inline BUDGET,
//     not a source spelling. Insert2 is verbatim the stdlith
//     original (src/LT2/lithshared/stdlith/dynarray.h:536), so its five
//     element-copy sites are not in question; the whole 63-byte residual
//     is those three expansions, at 21 bytes each. The site is expandable
//     here -- cutting CCollElem1CS::operator= down to its twelve-byte head
//     alone makes VC6 expand ALL FIVE and the function comes out at 252
//     bytes with two relocations -- so what our build cannot reach is the
//     PARTIAL spend, three of five. With the true two-member body every
//     revision in toolchains/ expands none of them: 8168, 8447 and 8966 all
//     give the same 275 bytes and seven relocations. Neither is it the
//     helper's spelling. Three forms were measured and all three give the
//     identical 275/7: the in-class operator= below, the compiler's
//     IMPLICIT memberwise operator= (delete ours -- CCollInner10's own
//     user-declared assignment keeps the synthesised one memberwise, and
//     it still comes out as the same 44-byte 0x100195b8), and a void
//     operator= that drops the `return *this`. This is the Model::Model
//     finding again: the compiler that built the shipped object had one
//     more unit of this budget than any of ours.
//   * 0x10016922 and 0x1000bcdc are size-exact with the identical
//     instructions in a different order — scheduling, documented at each.
//   * the 0xa4 walk, which does one thing more than the template's.
// Two of those InternalNiceSetSize instantiations are the only REGRESSION
// against the previous reconstruction, and they are worth being explicit
// about: shipped 0x10019a06 and 0x1001a0d8, 148 bytes each, were matched
// by the old hand-written `CFixedCollection<T>::Reserve`, which called the
// three-argument allocator directly and so had no `_AllocateTArray` layer
// to place. With the real two-layer source they come out size-exact with
// one instruction more than shipped: the shipped build inlined
// `_AllocateTArray` and left `BaseNew` as the call, ours calls
// `_AllocateTArray`. Ours also materialises a zero into a register that
// the shipped build does without (`cmp %edx,mem` where shipped has
// `cmpl $0,mem`), which is the downstream effect of the same choice. No
// source change reaches it — it is the per-function inline budget, and it
// costs two functions against a hundred and fourteen gained.
//
//
// ---- WHAT THE ALLOCATOR LAYER ADDED (a later pass) -------------------- //
//
// A later pass closed nineteen more shipped addresses without touching the
// container at all, by noticing that l_allocator.h and dynarray.h carry
// three more templates than this file was instantiating:
//
//   * `BaseNew_1P` and `BaseNew_2P`, l_allocator.h's one- and
//     two-pointer-argument siblings of `BaseNew`. Four of the image's
//     allocators push an argument inside the construction loop
//     (`push 0x1c(%esp); mov %edi,%ecx; call <ctor>`) and one pushes two,
//     which is the only thing that separates a `T()` element from a
//     `T(P1*)` one. 0x1001cc25, 0x1001cc67, 0x1001cce3, 0x1001cd21 and
//     0x1001cd98.
//   * `DeleteAndClearArray2`, the free function at the very foot of
//     dynarray.h. Its template parameter is the ARRAY type, not the
//     element type, and the image holds six instantiations that differ
//     only in the BaseDelete they reach. 0x1000d071, 0x1001a1a1,
//     0x1001a1dd, 0x1001a215, 0x1001b5c9 and 0x1001b605.
//   * six more `BaseDelete` instantiations, for the element types that
//     turned out to carry a destructor: 0x1000d22c, 0x1001a16c,
//     0x1001a251, 0x1001a283, 0x1001b641, 0x1001b672, 0x1001ba94.
//
// What that cost was one line per element type, because it is the
// allocator pair -- not the container -- that records an element type's
// constructor and destructor profile. Three findings generalise:
//
//   * `pTemp->~T()` inside BaseDelete distinguishes a virtual destructor
//     from a non-virtual one at a glance: virtual is
//     `mov (%esi),%eax; push $0x0; mov %esi,%ecx; call *(%eax)` -- slot
//     zero with a literal deleting flag -- and non-virtual is a direct
//     `call`. Three of the seven element types with a destructor are
//     polymorphic, and for those the vptr eats the first four bytes of
//     the size.
//   * a class can be polymorphic and still have a NON-virtual destructor,
//     and the image says which: 0x100164e3 (the 0x30 destructor called
//     directly from 0x1001a283) restores a vptr, while 0x1001ba94 calls
//     a 0x30 destructor through the vtable. Those are two different
//     classes of the same size, not one class read two ways.
//   * the guard in DeleteAndClearArray2 -- `if(theArray[i])` -- makes
//     BaseDelete's own `if(!ptr) return;` provably dead, and VC6 folds
//     the two `test`s into one *only* when BaseDelete inlines, i.e. only
//     for an element type with no destructor. So the 56-byte
//     instantiation and the 60-byte ones differ by exactly that fold, and
//     the byte count says whether the pointee has a destructor.
//
//
// ---- A NOTE ON THE UNMATCHED COUNT ------------------------------------ //
//
// About two hundred of the symbols this file emits are thin wrappers —
// Term, SetSize, Remove, Insert, NiceSetSize, CopyArray, Get, operator[],
// Clear, _InitArray, and the DefaultCache `CMoArray(cacheSize)` — that the
// shipped image does not contain at all. That is not missing work. The
// shipped build instantiated the template *implicitly*, so only the
// members something still referenced after inlining were emitted; we have
// to instantiate *explicitly*, and an explicit instantiation emits every
// member that has a definition. They are counted as unmatched by
// the byte comparison and should be read as "not in the image", not as "wrong".
// VC6 cannot explicitly instantiate a constructor (C2909), so there is no
// way to ask for the vtable and the constructor without also getting the
// wrappers.
//
//
// ---- WHAT THE CLIENT CLASSES NEED (a follow-up pass, NOT done here) --- //
//
// Seven 34-byte constructors in the image are `CMoArray<T, DefaultCache>`
// built by seven *different* classes: 0x1000b15c, 0x1000b295, 0x1000b4db
// and 0x1000b591 in worldbsp.cpp, 0x10019104 in commonmodel.cpp,
// 0x10023fe7 and 0x10024097 in netmgr.cpp. They are byte-identical to each
// other and to the constructor this file emits — the only differences are
// the vftable immediate and the `Init` call target, both relocations. Each
// of those classes therefore either *is* an instantiation of this template
// or *derives* from one; 0x1004dd9c overrides vtable slot 9 with its own
// GenAppendList (0x10024009), whose body is in turn byte-identical to the
// template's, so at least that one is a derivation.
//
// What a follow-up pass has to change, file by file:
//
//   netmgr.cpp / servermgr.h — DONE, and the prescription that used to
//     stand here was wrong on both counts. It said to DERIVE from
//     `CMoArray<void*>` and to add a `GenAppendList` override. The shipped
//     bytes say plain TYPED INSTANTIATIONS and no override:
//
//       typedef CMoArray<NetConnection*> CConnList;    // vtable 0x1004dd9c
//       typedef CMoArray<CBaseDriver*>   CDriverList;  // vtable 0x1004ddcc
//
//     Not a derivation: `~CNetMgr` (0x100213ee) expands each list as ONE
//     vftable store plus one `SetSize2(0,&g_DefAlloc)`, and a derived class
//     over a destructor-bearing base emits TWO vptr stores — VC6 does not
//     fold them, as the very same function proves by storing 0x1004dd8c
//     then 0x1004d260 for `CNetBank : CBankBase`. The constructors say the
//     same: one vftable at the TOP, then Clear()'s four zeroes, then Init.
//     A base ctor would put its store first and the derived store LAST.
//     Deriving would have cost ??0CNetMgr (131b) and ??1CNetMgr (116b),
//     both matched in server.dll.
//
//     No `GenAppendList` override either: in lithtech.exe 0x004a2b00 and
//     0x004a2ad0 name the SAME twelve addresses while staying two distinct
//     tables — identical code folded, identical data not. server.dll's pair
//     differs in slot 9 only, where the second copy at 0x10024009 is
//     byte-identical to 0x1000b207 apart from two call displacements. That
//     is a link artefact, not source.
//
//     Measured: lithtech +2, server.dll +2, nothing lost.
//
//   worldbsp.cpp / worldbsp.h — the four `CWorldArray50` / `CWorldArray54`
//     / `CWorldArray68` / `CWorldArray138` classes are the same case.
//     Their `~CWorldArrayNN() { SetSize(0, &g_MoAlloc); }` is the
//     template's `~CMoArray() { Term(); }`; their declared
//     `SetSize(nItems, pAlloc)` is `SetSize2`; and the addresses already
//     written down there — 0x1000c470, 0x1000bf8d, 0x1000c0e7 — are
//     `SetSize2` instantiations, so they belong to whichever element type
//     each array holds, not to the class. All three of those SetSize2
//     addresses are matched from here: 0x1000c470 (95 bytes, four-byte
//     element), 0x1000bf8d (98, the shape where BaseNew is the call) and
//     0x1000c0e7 (112, the 0xc0 element whose BaseNew *and* BaseDelete
//     both take a constructor/destructor loop).
//
//   smgr.cpp / servermgr.h — `CUnknown8FC` (constructor 0x1002cbe3, its
//     SetSize 0x1002ccee, its AllocItems 0x1002cecf) is the same pattern
//     once more, and all three are matched from here: the 34-byte
//     `CMoArray()`, the 95-byte DefaultCache `SetSize2` and a 24-byte
//     `_AllocateTArray`.
//
//   commonmodel.cpp / commonmodel.h — delete the local `CMoPtrArray`
//     class (it is `CMoArray<void*, NoCache>`; its SetSize 0x1001a03d,
//     FreeItems 0x1001aad4 and AllocItems 0x100243de are all matched from
//     here now). The thirteen sub-object constructors the `Model`
//     constructor runs are collection constructors: 0x1001831d,
//     0x10018b5e, 0x10018c1b, 0x10018c92, 0x10018e8f, 0x1001927c and
//     0x10019364 are `CMoArray<T, NoCache>::CMoArray()` (52 bytes) and
//     0x10019104 is `CMoArray<T, DefaultCache>::CMoArray()` (34 bytes),
//     so `Model`'s members at 0x24, 0x34, 0x44, 0x54, 0x70, 0x8c, 0x154
//     and 0x164 are `CMoArray` objects of 0x10 or 0x14 bytes rather than
//     four-byte holes. WARNING, and it changes the arithmetic in this
//     paragraph: `CMoArray<T, NoCache>` is 0x10 bytes, not the 0x0c this
//     comment used to claim (see CCollElem24Rec at the foot of the file
//     for the proof). So the two temporaries in Model's constructor
//     cannot be two whole `CMoArray<T, NoCache>` locals in a 0x10-byte
//     frame — one of them fills it on its own. Work on
//     commonmodel.cpp has to re-derive that from the frame; the member
//     offsets above are unaffected, since they come from the constructor
//     call sites rather than from a size. `Assign` there is `CopyArray2(other, pAlloc)`
//     (0x1001b76a, 0x1001b9b5 — both matched from here) and the
//     "destroyed inline" tail is `~CMoArray()`, i.e. the vptr re-store
//     plus `Term()`.
//
//   packet.cpp / servermsg.h — `CPacketData` is `CMoArray<uint8>`, i.e.
//     dynarray.h's `CMoByteArray`. Its `m_AllocSize` is `m_CacheSize` at
//     0x0c and really is an element count.
//
// None of those files is changed here; the point of the
// modelling was to make them reachable.
#include <string.h>
#include "collection.h"


// The element types, by the only thing about them that is visible here:
// their size, and which of a constructor, an assignment and a destructor
// each carries. The engine's names for them are not recoverable; the sizes
// come from the element indexing and from the size GenFindElement hands
// MoArray_FindElementMemcmp.
#define DECLARE_COLLELEM(name, size)                   \
	class name                                         \
	{                                                  \
	public:                                            \
		name() {}                                      \
		name(const name& other);                       \
		name& operator=(const name& other);            \
		uint8 m_pad[size];                             \
	}

// Three of them carry an assignment of their own and the rest are plain
// data: the walk calls out for the first three and copies the rest inline,
// which is what separates them.
#define DECLARE_COLLPOD(name, size)                    \
	class name                                         \
	{                                                  \
	public:                                            \
		uint8 m_pad[size];                             \
	}

// And seven carry a constructor, which BaseNew settles on its own: an
// element with one gets a loop running it over every slot, an element
// without gets nothing but the dangling null test.
#define DECLARE_COLLCTOR(name, size)                   \
	class name                                         \
	{                                                  \
	public:                                            \
		name();                                        \
		uint8 m_pad[size];                             \
	}

// One of the seven takes an argument, defaulted: its BaseNew pushes a
// literal zero before the constructor call where the other six push
// nothing.
#define DECLARE_COLLCTORARG(name, size)                \
	class name                                         \
	{                                                  \
	public:                                            \
		name(int nInit = 0);                           \
		uint8 m_pad[size];                             \
	}

// Two carry all three: a constructor, an assignment, and a destructor. The
// destructor is the one BaseDelete runs over the block, and it is also the
// one that runs on GenCopyList's by-value return temporary — visible in the
// shipped walk as a bare `lea` of the frame slot and a thiscall immediately
// after the copy-out.
#define DECLARE_COLLCTORELEM(name, size)               \
	class name                                         \
	{                                                  \
	public:                                            \
		name();                                        \
		name(const name& other);                       \
		~name();                                       \
		name& operator=(const name& other);            \
		uint8 m_pad[size];                             \
	}

DECLARE_COLLPOD (CCollElem04, 0x04);
// A fifth four-byte element type, and the only one that carries a
// constructor. `BaseNew` for it (0x1001ba2b) expands the constructor in
// line instead of calling it, and what it expands to is `fldz; fstps
// (%ecx)` — one float member, zero-initialised — wrapped in the dangling
// placement-new null test. The size is `shl $0x2`.
class CCollElem04F
{
public:
	CCollElem04F() { m_f = 0.0f; }
	float m_f;
};
DECLARE_COLLPOD (CCollElem08, 0x08);
// 0x14 is at least two element types with a constructor: BaseNew for the
// size exists twice over (0x1001b730 and 0x1001b97b, byte-identical but
// for the constructor they call), and the two constructors are
// 0x1001651d and 0x10016478. This is the second. Four floats and a
// trailing dword, all zeroed, and the store order is the reverse of the
// layout — the dword at 0x10 goes first (as an `andl $0x0`, which is how
// /O1 spells an integer zero store) and the floats follow at 0xc, 8, 4, 0
// with a fresh `fldz` each.
// 0x10056ff4 — zero-initialised .data, and the only thing 0x1001651d
// says about it is that the element type's second member is initialised
// to point at it. Named by its address; the object it really is lives in
// whichever translation unit owns the element type.
extern uint32 g_Unknown10056ff4;

// 0x1001651d (23 bytes) — the first of the two 0x14 element constructors.
// A dword, a pointer, a byte and two more dwords; everything but the
// pointer is zeroed out of one `xor %ecx,%ecx` (the byte through `%cl`),
// and the stores run in layout order.
class CCollElem14P
{
public:
	CCollElem14P();

	uint32  m_a;
	uint32* m_pData;
	uint8   m_b;
	uint32  m_c;
	uint32  m_d;
};

CCollElem14P::CCollElem14P()
{
	m_a = 0;
	m_pData = &g_Unknown10056ff4;
	m_b = 0;
	m_c = 0;
	m_d = 0;
}

// The 0x14 array below (CMoArray<CCollElem14, NoCache>) holds THIS element:
// its InternalNiceSetSize (0x1001971a), Insert2 (0x1001a550) and Remove2
// (0x1001a611) all call BaseNew 0x1001b730, and that BaseNew's constructor
// is 0x1001651d. (The other 0x14 BaseNew, 0x1001b97b, runs 0x10016478 --
// CCollElem14F -- and no array of that element is instantiated here.)
typedef CCollElem14P CCollElem14;

class CCollElem14F
{
public:
	CCollElem14F();

	float  m_a, m_b, m_c, m_d;
	uint32 m_n;
};

CCollElem14F::CCollElem14F()
{
	m_n = 0;
	m_d = 0.0f;
	m_c = 0.0f;
	m_b = 0.0f;
	m_a = 0.0f;
}
// 0x10009250 (37 bytes) is the constructor BaseNew<CCollElem18> (0x1000d145)
// calls, and the reason that BaseNew pushes a literal zero before the call
// where the other six push nothing: the constructor takes one defaulted
// argument, and `mov 0x4(%esp),%al` says it is a byte. The 0x18 element is
// therefore three dwords, a self-linked list head at 0x0c (both links are
// set to the address of the head itself, which is stdlith's circular
// list), a 16-bit -1 at 0x14 (`orw $0xffff`), the argument at 0x16 and a
// last byte at 0x17.
class CCollElem18Link
{
public:
	CCollElem18Link* m_pNext;             // +0x0c
	CCollElem18Link* m_pPrev;             // +0x10
};

class CCollElem18L
{
public:
	CCollElem18L(uint8 bInit = 0) { Init(bInit); }

	void            Init(uint8 bInit);

	uint32          m_a, m_b, m_c;        // +0x00
	CCollElem18Link m_head;               // +0x0c
	uint16          m_x;                  // +0x14
	uint8           m_flag;               // +0x16
	uint8           m_y;                  // +0x17
};

// The shipped function is NOT the constructor: it leaves eax holding
// `&m_head` rather than `this`, and a VC6 constructor always hands `this`
// back. What BaseNew's placement new calls is a void member function, so
// the constructor itself is one line — `Init(bInit)` — expanded at the
// placement-new site, which is also why the defaulted argument shows up
// as BaseNew's `push $0x0`.
void CCollElem18L::Init(uint8 bInit)
{
	CCollElem18Link* pHead;

	m_x = 0xffff;
	m_flag = bInit;
	m_a = 0;
	m_y = 0;
	m_c = 0;
	m_b = 0;
	pHead = &m_head;
	pHead->m_pPrev = pHead;
	pHead->m_pNext = pHead;
}

// The 0x18 array and BaseNew below hold THIS element: BaseNew<0x18>
// (0x1000d145) pushes the defaulted zero and calls 0x10009250, which is
// Init above -- the constructor's one statement.
typedef CCollElem18L CCollElem18;
DECLARE_COLLELEM(CCollElem1C, 0x1c);
// The 0x1c element CCollElem1CS (and its two sub-objects, CCollInner10
// and CCollInner0C) is in collelem1cs.h, where a lithtech-only TU can
// reach its assignment operator too; the notes on it went with it.
#include "collelem1cs.h"


// 0x10016723 (42 bytes) is the same shape once more — twelve-byte head,
// sixteen-byte sub-object, the sub-object's assignment expanded — for a
// third 0x1c element type whose assignment operator returns VOID rather
// than a reference. That is not a codegen variant: the shipped function
// leaves `this` nowhere and ends `ret $0x4` with eax holding the last
// dword it read. stdlith spells assignments that way elsewhere too —
// `BaseGenListPos::operator=` in genlist.h above is void.
class CCollElem1CV
{
public:
	void operator=(const CCollElem1CV& other);

	CCollInner0C m_head;
	CCollInner10 m_sub;
};

void CCollElem1CV::operator=(const CCollElem1CV& other)
{
	m_head = other.m_head;
	m_sub = other.m_sub;
}
DECLARE_COLLPOD (CCollElem20, 0x20);
// The 0x20 element with an assignment operator of its own (CCollElem20A,
// with its CCollInner08) and the one with a constructor (CCollElem20F) are
// in collelem20.h, where modelnew.cpp's piece data record reaches them too;
// the notes on them went with them.
#include "collelem20.h"

CCollElem20F::CCollElem20F()
{
	m_a = 0;
	m_b = 0;
	m_c = 0.0f;
	m_d = 0.0f;
	m_e = 0.0f;
	m_f = 0.0f;
	m_g = 0.0f;
	m_h = 0.0f;
}

// The 0x20 array with a constructor (CMoArray<CCollElem20C, NoCache>) holds
// THIS element: its InternalNiceSetSize (0x10019a06), Insert2 (0x1001a94d)
// and Remove2 (0x1001aa09) call BaseNew 0x1001b84c, whose constructor is
// 0x10016492, and its CopyArray2 (0x1001b7d4) calls 0x10016492 directly.
typedef CCollElem20F CCollElem20C;

// 0x10016922 (44 bytes) is the other. Two dwords, then six floats whose
// initialisers are the only numeric constants this file recovers: 128.0f
// at 0x08 (stored as the integer immediate 0x43000000), 16.0f at 0x0c and
// 0x10 (one `flds` of 0x1004da20 = 0x41800000, stored twice — `fsts` then
// `fstps`), and zero for the last three.
// RESIDUAL: size-exact at 44 bytes with the same fourteen instructions,
// but two of them land in a different FPU latency gap. Shipped fills the
// `flds` gap with the two integer zero stores and the first `fldz` gap
// with the 128.0f immediate; ours leaves the `flds` gap empty and pushes
// both fillers one gap later. Source statement order does not reach it —
// all six orderings of the eight assignments compile to the identical
// fourteen instructions in our order, as does moving the two dwords into
// an inlined base class — so this is the FP store scheduling near class,
// not a source difference.
class CCollElem20K
{
public:
	CCollElem20K();

	uint32 m_a, m_b;
	float  m_c, m_d, m_e, m_f, m_g, m_h;
};

CCollElem20K::CCollElem20K()
{
	m_a = 0;
	m_b = 0;
	m_c = 128.0f;
	m_d = 16.0f;
	m_e = 16.0f;
	m_f = 0.0f;
	m_g = 0.0f;
	m_h = 0.0f;
}
DECLARE_COLLCTORELEM(CCollElem24, 0x24);
DECLARE_COLLPOD (CCollElem40, 0x40);
DECLARE_COLLCTOR    (CCollElem60, 0x60);
// 0x10007e85 (40 bytes) is the constructor BaseNew<CCollElem60> (0x1000d1f2)
// calls, so this is the 0x60 element's own shape as far as its constructor
// touches it: a byte at 0x00, five dwords at 0x20..0x30, two -1 words at
// 0x34 and 0x38 (`orl $0xffffffff`, /O1's spelling of a -1 store), 0.0f at
// 0x3c and 1.0f at 0x40. Everything from 0x01 to 0x20 and from 0x44 up is
// left uninitialised and so unnamed.
class CCollElem60C
{
public:
	CCollElem60C();

	uint8  m_flag;                 // +0x00
	uint8  m_pad0[0x1f];
	uint32 m_a;                    // +0x20
	uint32 m_b, m_c, m_d, m_e;     // +0x24
	int32  m_f;                    // +0x34
	int32  m_g;                    // +0x38
	float  m_h;                    // +0x3c
	float  m_i;                    // +0x40
	uint8  m_pad1[0x1c];
};

// Store order is lithtech.exe's (0x00422240): /O2 keeps these ten stores in
// source order, while server.dll's /O1 hoists the -1s and the x87 pair ahead
// of the byte and the zeroes -- so server.dll matches under either order and
// only this one pairs both images.
CCollElem60C::CCollElem60C()
{
	m_flag = 0;
	m_b = 0;
	m_c = 0;
	m_d = 0;
	m_e = 0;
	m_g = -1;
	m_f = -1;
	m_h = 0.0f;
	m_i = 1.0f;
	m_a = 0;
}
DECLARE_COLLPOD (CCollElemA4, 0xa4);   // its walk does one thing more
DECLARE_COLLCTORELEM(CCollElemC0, 0xc0);
// Seven more sizes with no walk anywhere in the image, and — see the
// instantiation list below — no array function of their own either. All
// that is really there for them is `BaseNew<T>`, i.e. LNew_Array on a raw
// block of T from somewhere else in the engine. They are kept here because
// that is where their element size and constructor/no-constructor profile
// is recorded, not because a CMoArray ever held them.
// Six of the seven now have more of a profile than "a size", because the
// image holds LNew_Array / LDelete_Array over them and those two name a
// constructor and a destructor apiece. What each one carries:
//
//   size  BaseNew                        BaseDelete
//   0x28  0x1001cce3 BaseNew_1P, T(P1*)  0x1001b672, ~T() non-virtual
//   0x30  0x1001cd5e BaseNew,    T()     0x1001a283, ~T() non-virtual
//   0x30  --                             0x1001ba94, ~T() VIRTUAL
//   0x34  0x1001ccaa BaseNew,    T()     --
//   0x5c  0x1001cd98 BaseNew_1P, T(P1*)  0x1001a251, ~T() VIRTUAL
//   0x6c  0x1001cd21 BaseNew_1P, T(P1*)  0x1001b641, ~T() non-virtual
//   0xb4  --                             0x1001a16c, ~T() VIRTUAL
//
// "VIRTUAL" is not an inference: `pTemp->~T()` in BaseDelete comes out as
// `mov (%esi),%eax; push $0x0; mov %esi,%ecx; call *(%eax)` -- vtable slot
// zero with a literal deleting flag -- for those three and as a direct
// `call` for the rest. Where a class is polymorphic the vptr eats the
// first four bytes, so the padding below is the size less four for those
// three and the whole size for the others.
//
// The P1 of the three BaseNew_1P instantiations is `CCollElem24Owner*`,
// the same "owner that holds an LAlloc* at +0x174" that CCollElem24Rec
// points at: the 0x6c constructor (0x10016bdb) stores its argument at
// +0x68 and the matching destructor body (0x10016c3a) reads
// `0x174(0x68(this))` back out of it to Term an array member.
class CCollElem24Owner;

// A sixteen-byte element with a destructor, and the only one of the family
// whose array is DefaultCache rather than NoCache: BaseDelete for it is
// 0x1000d22c (stride 0x10, direct `call 0x10007eb9`) and the
// DeleteAndClearArray2 above it at 0x1000d071 Terms through 0x1000c470,
// the four-byte *DefaultCache* SetSize2, where all five of its siblings
// Term through 0x1001a03d. That is worth spelling out because the two are
// byte-identical under relocation masking and a byte match reports the
// DefaultCache one first: only the call target separates them, so this is
// a call-target-only distinction.
class CCollElem10
{
public:
	    ~CCollElem10();

	uint8 m_pad[0x10];
};

class CCollElem28
{
public:
	    CCollElem28(CCollElem24Owner* pOwner);
	    ~CCollElem28();

	uint8 m_pad[0x28];
};

// The 0x30 element carries three constructors' worth of evidence: a
// default one (0x100164bb, reached by the plain BaseNew at 0x1001cd5e), a
// TWO-pointer one (0x10016578, reached by BaseNew_2P at 0x1001cc25, which
// stores its first argument at +0x2c and its second at +0x18), and a
// destructor (0x100164e3).
class CCollElem30
{
public:
	    CCollElem30();
	    CCollElem30(CCollElem24Owner* p1, CCollElem24Owner* p2);
	    ~CCollElem30();

	uint8 m_pad[0x30];
};

// The second 0x30 element type: same size, but its destructor is virtual
// (0x1001ba94 against 0x1001a283). Not the same class as the one above --
// the non-virtual 0x30 destructor at 0x100164e3 restores a vptr of its
// own, so that class is polymorphic *without* a virtual destructor, and
// one class cannot be both.
class CCollElem30V
{
public:
	    virtual ~CCollElem30V();

	uint8 m_pad[0x2c];
};

DECLARE_COLLPOD (CCollElem2E, 0x2e);

class CCollElem34
{
public:
	    CCollElem34();

	uint8 m_pad[0x34];
};

class CCollElem5C
{
public:
	    CCollElem5C(CCollElem24Owner* pOwner);
	    virtual ~CCollElem5C();

	uint8 m_pad[0x58];
};

class CCollElem6C
{
public:
	    CCollElem6C(CCollElem24Owner* pOwner);
	    ~CCollElem6C();

	uint8 m_pad[0x6c];
};

class CCollElemB4
{
public:
	    CCollElemB4(CCollElem24Owner* pOwner);
	    virtual ~CCollElemB4();

	uint8 m_pad[0xb0];
};


// ---- LNew_1P ---------------------------------------------------------- //
//
// Period source, and it stays here: nothing outside this translation unit
// instantiates it. (`LNew`/`LDelete` and the `DeleteAndClearArray` pair
// that used to sit below it have moved into collection.h, where the period
// headers carry them -- l_allocator.h and the foot of dynarray.h -- because
// the callers in commonmodel.cpp need to reach the wrapper.)
//
// `BaseNew_1P` is l_allocator.h's one-pointer-argument sibling of
// `BaseNew` (the header carries BaseNew, BaseNew_1P and BaseNew_2P side by
// side). The image holds three instantiations of it and they are what
// separates a `T()` element from a `T(P1*)` one: the placement new pushes
// the argument inside the construction loop, so the loop body is
// `push 0x1c(%esp); mov %edi,%ecx; call <ctor>` rather than a bare
// `mov %esi,%ecx; call <ctor>`.
#define LNew_1P(pAllocator, type, p1) \
	BaseNew_1P(pAllocator, (type*)0, 1, p1);

template<class T, class P1>
inline T* BaseNew_1P(LAlloc* pAlloc, T* dummy, uint32 nElements, P1* p1)
{
	T *ptr, *pTemp;

	ptr = (T*)pAlloc->Alloc(sizeof(T) * nElements);
	if (!ptr)
		return 0;

	// Construct elements.
	pTemp = ptr;
	while (nElements)
	{
		--nElements;
		::new(pTemp, (DummyClassBlahBlah*)0) T(p1);
		++pTemp;
	}

	return ptr;
}


#define LNew_2P(pAllocator, type, p1, p2) \
	BaseNew_2P(pAllocator, (type*)0, 1, p1, p2);

template<class T, class P1, class P2>
inline T* BaseNew_2P(LAlloc* pAlloc, T* dummy, uint32 nElements, P1* p1, P2* p2)
{
	T *ptr, *pTemp;

	ptr = (T*)pAlloc->Alloc(sizeof(T) * nElements);
	if (!ptr)
		return 0;

	// Construct elements.
	pTemp = ptr;
	while (nElements)
	{
		--nElements;
		::new(pTemp, (DummyClassBlahBlah*)0) T(p1, p2);
		++pTemp;
	}

	return ptr;
}


// `DeleteAndClearArray2` and its one-line wrapper `DeleteAndClearArray` are
// now at the foot of collection.h, where dynarray.h carries them. The five
// shipped copies and what they prove are written up there.


// ---- the instantiation list ------------------------------------------- //
//
// Pruned to what the image actually holds. The test is mechanical: an
// instantiation earns its place only if it matches a shipped function that
// no other instantiation matches. SetSize2 embeds no element size at all
// (the size lives in BaseNew, one call further down), so it is a single
// shared function for a whole cache policy; what separates one element type
// from another is the element *indexing* in Insert2, Remove2,
// InternalNiceSetSize, CopyArray2 and the walk. An element type with none
// of those in the image is an element type this container never had.
//
// What that pruning found, and it is worth writing down:
//
//   * Four bytes is three element types, not one, and the walk is what
//     separates them. 0x100240b9 hands the element back in eax and
//     0x1000ba3e copies it into a buffer the caller passed, which is the
//     difference between `T` being a scalar — dynarray.h's own
//     `CMoPtrArray` is `CMoArray<void*>` and `CMoDWordArray` is
//     `CMoArray<uint32>` — and being a four-byte class. The image holds
//     both, plus `CMoArray<uint8>` (returned in al) and a four-byte float
//     one (returned on the FPU stack), so four of the instantiations below
//     differ only in the type of a four-byte element.
//   * The two cache policies are not interchangeable across element types.
//     0x14, 0x24 and 0x40 appear only uncached; 0x08, 0x18, 0x60, 0xa4 and
//     0xc0 only cached; 0x1c, 0x20 and four-byte both ways.
//   * One size can be several element types, and the allocator is what
//     says so: 0x20 has a BaseNew with no constructor loop (0x1001b8fa)
//     and two with one (0x1001b84c, 0x1001ba5b, calling two different
//     element constructors). Adding the constructor-carrying 0x20 element
//     recovered nine more shipped functions, two of which no previous
//     reconstruction had — so the remaining misses in this family are
//     probably more of the same rather than codegen problems.
//   * Seven sizes the previous reconstruction modelled — 0x28, 0x2e, 0x30,
//     0x34, 0x5c, 0x6c, 0xb4 — have no array function of their own in the
//     image at all. What is really there for them is LNew_Array on its
//     own: `BaseNew<T>` out of line, from callers elsewhere in the engine
//     that allocate a raw block of T. Those are instantiated below as free
//     functions, which is what they are.

// The four four-byte-element instantiations, which differ only in what
// `GenGetNext` does with the element: a pointer or a DWORD comes back in
// eax (`CMoPtrArray` is `CMoArray<void*>` and `CMoDWordArray` is
// `CMoArray<uint32>` — the two are the same code, so one stands for both),
// a byte in al, a float on the FPU stack, and a four-byte *class* through
// a buffer the caller passes.
template class CMoArray<uint32, DefaultCache>;
template class CMoArray<uint8, DefaultCache>;
template class CMoArray<float, DefaultCache>;
template class CMoArray<CCollElem04, DefaultCache>;
// No NoCache array of the four-byte CLASS: every shipped NoCache four-byte
// vtable (0x004a26d8, 0x004a2784, 0x004a28a8, 0x004a2968, 0x004a2998 in
// lithtech.exe) holds a pointer, and shares GenCopyList 0x00447f90 (250)
// and GenAppendList 0x0044a1b0 (197) -- which the class's own copies miss by
// seven bytes each (the class element is copied through a buffer). Every
// other member it emitted is placed by the pointer instantiations too.
template class CMoArray<CCollElem04F, DefaultCache>;

template class CMoArray<CCollElem08, DefaultCache>;
template class CMoArray<CCollElem14, NoCache>;
template class CMoArray<CCollElem18, DefaultCache>;
// The 0x1c element is CCollElem1CS, not the opaque stand-in: see the note on
// the class. Only the /O2 image can tell, because the difference is whether
// the element's copy constructor and assignment are expansion candidates at
// all, and /O1 declines the expansion either way.
template class CMoArray<CCollElem1CS, DefaultCache>;
// No NoCache array of it: the shipped NoCache 0x1c array is commonmodel.h's
// CModelXform's (vtable 0x004a2708), whose void assignment server.dll's
// element loops call (0x10016723). This file's CMoArray<CCollElem1CS,
// NoCache> was a look-alike that stood in for it in lithtech.exe.
// NO array holds the plain 0x20 POD. Both of its instantiations used to
// stand here and neither existed in either shipped image; every "match"
// they ever scored was a byte collision with a sibling.
//
// The evidence, taken from both images at once:
//
//   * Not one shipped address in server.dll or lithtech.exe was covered
//     ONLY by a CMoArray<CCollElem20, *> symbol. Every address they
//     claimed was claimed simultaneously by CCollElem20A, CCollElem20C,
//     CCollElem40 or a DefaultCache sibling.
//   * The element-size-sensitive members are the discriminator, because a
//     0x20 POD, a 0x20 with an out-of-line assignment (CCollElem20A) and a
//     0x20 with a constructor (CCollElem20C) compile to DIFFERENT bytes
//     there. In lithtech.exe the 20A and 20C families place —
//     GenRemoveAt at 0x004491b0 / 0x00448cd0, InternalNiceSetSize at
//     0x0044bbe0 / 0x0044ba80, CopyArray2 at 0x0044c9b0 / 0x0044c8b0,
//     Append at 0x00449030 / 0x00448bd0 — and the plain POD's MISS every
//     one of them. Only SetSize2 places, at 0x0044bb70, and SetSize2 does
//     not touch the element size, so it is 20A's function.
//   * CMoArray<CCollElem20, DefaultCache> misses in lithtech.exe outright
//     and in server.dll places only on Init and SetSize2, both of which
//     every DefaultCache instantiation shares byte for byte.
//
// Deleting them is therefore score-neutral in both images, in the exact
// bucket and in the below-the-floor shape bucket alike: each tiny COMDAT
// they contributed (GenRemoveAll 13b, GetSize 4b, SetCacheSize 3b, …) has
// between twelve and sixty-seven identically-shaped siblings left against
// at most eighteen shipped addresses of that shape.
//
// `BaseNew<CCollElem20>` below is a collision too (0x1001b8fa / 0x0044ca90
// are shared with AnimInfo's and CCollElem20A's), but it is left in place:
// it is a free function template, not an array, and the list it sits in is
// about LNew_Array call sites elsewhere in the engine.
template class CMoArray<CCollElem20, DefaultCache>;
template class CMoArray<CCollElem20, NoCache>;
template class CMoArray<CCollElem20A, NoCache>;
template class CMoArray<CCollElem20C, NoCache>;
// CMoArray<CCollElem24Rec, NoCache> — instantiated at the foot of the file,
// where the element class is complete.
template class CMoArray<CCollElem40, NoCache>;
template class CMoArray<CCollElem60, DefaultCache>;
template class CMoArray<CCollElemA4, DefaultCache>;
// CMoArray<CCollElemC0Rec, DefaultCache> — instantiated at the foot of the
// file, where the element class is complete.


// ---- LNew_Array / LDelete_Array on their own -------------------------- //
//
// `BaseNew<T>` and `BaseDelete<T>` are free function templates, so an
// out-of-line copy exists for every element type whose LNew_Array VC6
// declined to expand at some call site — including element types no
// CMoArray in the image ever held. Eleven survive; these are they. The
// element size is the `shl`/`imul` and the constructor loop is present
// exactly for the element types that have a constructor to run.
template CCollElem08* BaseNew(LAlloc*, CCollElem08*, uint32);
template CCollElem04F* BaseNew(LAlloc*, CCollElem04F*, uint32);
template CCollElem14* BaseNew(LAlloc*, CCollElem14*, uint32);
template CCollElem18* BaseNew(LAlloc*, CCollElem18*, uint32);
template CCollElem1C* BaseNew(LAlloc*, CCollElem1C*, uint32);
template CCollElem20* BaseNew(LAlloc*, CCollElem20*, uint32);
template CCollElem24* BaseNew(LAlloc*, CCollElem24*, uint32);
#ifdef LT_SERVER_BUILD
template CCollElem30* BaseNew(LAlloc*, CCollElem30*, uint32);   // server.dll only
#endif
template CCollElem60* BaseNew(LAlloc*, CCollElem60*, uint32);
template CCollElemC0* BaseNew(LAlloc*, CCollElemC0*, uint32);

template void BaseDelete(LAlloc*, CCollElem24*, uint32);
template void BaseDelete(LAlloc*, CCollElemC0*, uint32);

// The six BaseDelete instantiations whose element type carries a
// destructor. Each is the same eleven-instruction shape and differs only
// in the element stride and in whether the destructor call is virtual.
//
// lithtech.exe has only two of them (0x0044c770 for the 0x28 element,
// 0x0044c740 for the 0x6c one) and none of the BaseNew_1P / BaseNew_2P /
// DeleteAndClearArray2 instantiations below: the client's linker dropped
// them, so they are compiled for server.dll only (their constructors and
// destructors are server-side code the client link could not resolve).
#ifdef LT_SERVER_BUILD
template void BaseDelete(LAlloc*, CCollElem10*, uint32);      // 0x1000d22c
#endif
template void BaseDelete(LAlloc*, CCollElem28*, uint32);      // 0x1001b672
#ifdef LT_SERVER_BUILD
template void BaseDelete(LAlloc*, CCollElem30*, uint32);      // 0x1001a283
template void BaseDelete(LAlloc*, CCollElem30V*, uint32);     // 0x1001ba94
template void BaseDelete(LAlloc*, CCollElem5C*, uint32);      // 0x1001a251
#endif
template void BaseDelete(LAlloc*, CCollElem6C*, uint32);      // 0x1001b641
#ifdef LT_SERVER_BUILD
template void BaseDelete(LAlloc*, CCollElemB4*, uint32);      // 0x1001a16c

// One more plain BaseNew, and the three one-argument ones.
template CCollElem34* BaseNew(LAlloc*, CCollElem34*, uint32);            // 0x1001ccaa
template CCollElem28* BaseNew_1P(LAlloc*, CCollElem28*, uint32, CCollElem24Owner*);   // 0x1001cce3
template CCollElem5C* BaseNew_1P(LAlloc*, CCollElem5C*, uint32, CCollElem24Owner*);   // 0x1001cd98
template CCollElem6C* BaseNew_1P(LAlloc*, CCollElem6C*, uint32, CCollElem24Owner*);   // 0x1001cd21
template CCollElemB4* BaseNew_1P(LAlloc*, CCollElemB4*, uint32, CCollElem24Owner*);   // 0x1001cc67
template CCollElem30* BaseNew_2P(LAlloc*, CCollElem30*, uint32, CCollElem24Owner*, CCollElem24Owner*);  // 0x1001cc25
#endif

// And the six DeleteAndClearArray2 instantiations, named by the BaseDelete
// each one reaches -- except the first, whose BaseDelete was inlined and so
// names nothing. For that one CCollElem08 is a stand-in: all the shipped
// bytes say is that the pointee has no destructor, and every such pointee
// gives the same 56 bytes.
template void DeleteAndClearArray2(CMoArray<CCollElem08*, NoCache>&, LAlloc*);   // 0x1001a1dd
#ifdef LT_SERVER_BUILD
template void DeleteAndClearArray2(CMoArray<CCollElem10*, DefaultCache>&, LAlloc*); // 0x1000d071
template void DeleteAndClearArray2(CMoArray<CCollElem28*, NoCache>&, LAlloc*);   // 0x1001a215
template void DeleteAndClearArray2(CMoArray<CCollElem30V*, NoCache>&, LAlloc*);  // 0x1001b5c9
template void DeleteAndClearArray2(CMoArray<CCollElem6C*, NoCache>&, LAlloc*);   // 0x1001a1a1
template void DeleteAndClearArray2(CMoArray<CCollElemB4*, NoCache>&, LAlloc*);   // 0x1001b605
#endif


// ---- one element type read all the way out -------------------------- //
//
// CCollElem24 is not opaque after all: three of its special members are in
// the image, and between them they give its whole layout. It is two
// `CMoArray<T, NoCache>` members and a pointer, and nothing else:
//
//   0x00  CMoArray<CCollElem20C, NoCache>   (SetSize2 0x100199b7 / 79,
//                                            CopyArray2 0x1001b7d4 / 120)
//   0x10  CMoArray<CCollElem20A, NoCache>   (SetSize2 0x10019a9a / 76,
//                                            CopyArray2 0x1001b885 / 117)
//   0x20  pointer to an owner that holds an LAlloc* at +0x174
//                                                       = 0x24 bytes
//
// CORRECTION to the size table at the top of this file: a
// `CMoArray<T, NoCache>` is 0x10 bytes, not 0x0c. `NoCache` is an empty
// class and `C m_Cache;` is a MEMBER, not a base, so it costs one byte at
// 0x0c plus three of padding. Nothing else in the image could show that —
// no member of the array ever reads 0x0c under NoCache — but the copy
// constructor here does, because the implicit memberwise copy of an empty
// class member is a real one-byte move: `mov 0xc(%ecx),%dl` in
// 0x1001953b, and again at 0x1c for the second array. Those two byte
// moves are the whole difference between a 77-byte copy constructor and
// the 89-byte one you get by modelling 0x0c and 0x1c as element members.
//
// The element sizes of the two inner arrays come from the shipped sizes of
// the SetSize2 and CopyArray2 they call: those two pairs are the 0x20
// constructor-carrying and 0x20 assignment-carrying instantiations already
// in the list above, and no other pair of element types produces both
// sizes. What names each member is the destructor (0x10016b86): it Terms
// both arrays through the owner's allocator and then lets the implicit
// member destructors Term them again through g_DefAlloc, which is where
// the two vptr re-stores come from.
//
// CORRECTION, and the /O2 image is what makes it visible: the 0x24 array
// DOES hold a CCollElem24Rec, and the opaque DECLARE_COLLCTORELEM stand-in
// above is not interchangeable with it. 0x4495c0 is lithtech.exe's
// `GenGetNext` for the 0x24 element (stride `lea (%eax,%eax,8)` + `*4`), and
// its element copy is this class's IMPLICIT copy constructor EXPANDED: vptr,
// 0x04, 0x08, the one-byte `NoCache` member at 0x0c, vptr again, then the
// same five for the second array at 0x10..0x1c, then m_pOwner at 0x20 — the
// 77 bytes below, in line. A declared-but-undefined copy constructor is not
// an expansion candidate at any /O level, so the stand-in can only call.
//
// So the copy constructor and the assignment are left IMPLICIT here rather
// than written out: the bodies they generate are the ones this file used to
// spell by hand (0x1001953b / 0x10019588, both byte-identical either way at
// /O1), and being implicit is what makes them expansion candidates at /O2.
// The default constructor and the destructor stay user-declared and defined
// OUT OF LINE, because the shipped /O2 BaseNew (0x44cae0) and BaseDelete
// (0x44cab0) both still `call` them — an in-class or implicit one would have
// been expanded there and those two would have moved.

class CCollElem24Owner
{
public:
	LAlloc* GetAlloc() { return m_pAlloc; }

	uint8   m_pad[0x174];
	LAlloc* m_pAlloc;                 // +0x174
};

class CCollElem24Rec
{
public:
	                CCollElem24Rec(CCollElem24Owner* pOwner);
	                CCollElem24Rec();
	                ~CCollElem24Rec();

	// The copy constructor (0x1001953b / 77) and the assignment
	// (0x10019588 / 48) are deliberately IMPLICIT — see the note above.

	void            SetOwner(CCollElem24Owner* pOwner);

	CMoArray<CCollElem20C, NoCache> m_arr1;      // +0x00
	CMoArray<CCollElem20A, NoCache> m_arr2;      // +0x10
	CCollElem24Owner*               m_pOwner;    // +0x20
};

// 0x10016b3e (33 bytes) — the owner-taking constructor (moved here from
// svnew.cpp). Byte-for-byte the default one below but for the argument: the
// two array members' constructors run first, then the one call. Shipped
// order puts it first (0x10016b3e precedes 0x10016b5f), and COMDAT order in a
// .cpp is source definition order.
CCollElem24Rec::CCollElem24Rec(CCollElem24Owner* pOwner)
{
	SetOwner(pOwner);
}

// 0x10016b5f (29 bytes) — the constructor BaseNew<CCollElem24> calls. Its
// whole body is the setter below: the two `call`s in front of it are the
// two array members' own default constructors (0x10018351 and 0x10018546,
// 52 bytes each, which is the NoCache `CMoArray()`), run in declaration
// order before the body as always.
CCollElem24Rec::CCollElem24Rec()
{
	SetOwner(0);
}

// 0x10016b7c (10 bytes). Out of line, so it is declared in a header and
// defined in a .cpp: /O1 inlines only what is defined inside the class.
void CCollElem24Rec::SetOwner(CCollElem24Owner* pOwner)
{
	m_pOwner = pOwner;
}

// 0x10016b86 (85 bytes). The allocator is read through the owner's
// in-class GetAlloc(), not as a bare member, and only lithtech.exe can tell:
// its /O2 copy (0x004460a0, 155 bytes) CALLS _DeleteAndDestroyArray
// (0x0044c720) for the first Term and expands it for the second and for both
// implicit member destructors. The two accessor expansions are what exhaust
// the per-function inline budget at the first site (earliest sites lose
// first); with `m_pOwner->m_pAlloc` all four sites expand. server.dll's /O1
// copy is unchanged either way.
CCollElem24Rec::~CCollElem24Rec()
{
	m_arr2.Term(m_pOwner->GetAlloc());
	m_arr1.Term(m_pOwner->GetAlloc());
}

// The 0x24 array, over the element type read out above rather than over the
// opaque stand-in. It has to sit here, below the class, rather than in the
// list further up.
template class CMoArray<CCollElem24Rec, NoCache>;


// ---- and the 0xc0 element, from its copy and its assignment ---------- //
//
// The same trick reads CCollElemC0 out. The image holds its implicit copy
// constructor (0x1000bcdc, 181 bytes) and its implicit assignment
// (0x1000bd94, 144 bytes), and between them every member offset is fixed:
//
//   0x00  vptr — and TWO vptr stores in the copy constructor, 0x1004d404
//         then 0x1004d380, so there is a base class with a vtable of its
//         own whose copy constructor is expanded in line
//   0x04  char[0x41], and it belongs to the BASE, not to this class. That
//         is not cosmetic: with the array in the derived class the copy
//         constructor is size-exact but puts the base's vptr store before
//         the block move instead of after it, and moving the array up
//         into the base — so that the base's own inlined copy constructor
//         is {vptr store, block move} — is what closes the last four
//         instruction placements. (65 bytes: `rep movsl $0x10` + `movsb`
//         in the copy, a byte-at-a-time loop counted against 0x41 in the
//         assignment, which is how VC6 spells an array member in an
//         implicit operator= as against an implicit copy constructor.)
//   0x48  uint32
//   0x4c  uint32
//   0x50  CMoArray<uint32, DefaultCache>       (CopyArray2 0x1000c6a4/139)
//   0x64  uint32
//   0x68  CMoArray<CCollElem18, DefaultCache>  (CopyArray2 0x1000d0ad/152)
//   0x7c  uint32
//   0x80  0x30 bytes moved as one block
//   0xb0  0x0c bytes moved as one block
//   0xbc  uint32                                          = 0xc0 bytes
//
// The two inner element types are read off the shipped sizes of the
// CopyArray2 each array assignment reaches, the same way CCollElem24's
// were. The assignment stores no vptr at all, which is what says the base
// class carries one: a base with a vtable contributes a vptr store to
// construction and nothing to assignment.
//
// CORRECTION, from the /O2 image: the 0xc0 array is instantiated over THIS
// class, not over the opaque DECLARE_COLLCTORELEM stand-in, and the two are
// not interchangeable once the optimiser is allowed to expand.
// lithtech.exe's 0x427460 is `GenGetNext` for the 0xc0 element (stride
// `lea (%eax,%eax,2)` + `shl $6`) and 0x427540 is `GenGetAt`; both carry the
// implicit copy constructor above EXPANDED, and what they expand to names
// every offset in the table a second time:
//
//   lea 0x4(%edx),%esi / lea 0x4(%eax),%edi / rep movsl $0x10 / movsb
//                                     the base's char[0x41] at 0x04
//   movl $0x4a2054,(%eax)             the base's vptr
//   0x48, 0x4c                        two dwords
//   movl $0x4a2130,0x50(%eax) ... movl $0x4a1c24,0x50(%eax)
//                                     GenList's vptr then CMoArray's, for
//                                     the inner array at 0x50
//   0x54, 0x58, 0x5c, 0x60            its m_pArray/m_nElements/m_CacheSize/
//                                     m_WantedCache
//   0x64                              a dword
//   0x68 pair of vptrs, 0x6c..0x78    the second inner array
//   0x7c                              a dword
//   rep movsl $0xc from 0x80          m_blk1
//   three dwords from 0xb0            m_blk2
//   0xbc                              a dword
//   movl $0x4a2000,(%eax)             the derived class's own vptr
//
// 216 bytes, against the 41 our stand-in gives (a call). Both builds are
// /Ob1, so only an implicit or in-class member is ever an expansion
// candidate; /O1 (= /Os) declines a 181-byte expansion, which is why
// server.dll matches either spelling and 0x1000bcdc survives out of line
// there. /O2 (= /Ot) takes it.
//
// The default constructor and the destructor, by contrast, are user-declared
// and defined OUT OF LINE, because the shipped BaseNew and BaseDelete still
// `call` them in BOTH builds (0x1000d1b5 / 0x428e70 and 0x1000d181 /
// 0x428e30) — an implicit one would have been expanded at /O2 and those two
// would have moved. The real bodies are 0x1000a7de and 0x1000a849 and do
// more than this (the destructor calls a cleanup member at 0x1000a882 before
// the member destructors run); only their out-of-line-ness matters here.

// The two blocks at 0x80 and 0xb0 are sub-objects, not arrays: the
// assignment moves each as one `rep movsl`, and VC6 only does that for a
// whole-struct assignment — an array member of the enclosing class is
// assigned element by element instead, which is exactly what the char
// array at 0x04 does.
class CCollBlock30
{
public:
	uint32 m_v[0x0c];
};

class CCollBlock0C
{
public:
	uint32 m_v[0x03];
};

class CCollElemC0Base
{
public:
	virtual void    Unknown0();

	char    m_name[0x41];                             // +0x04
};

class CCollElemC0Rec : public CCollElemC0Base
{
public:
	        CCollElemC0Rec();
	        ~CCollElemC0Rec();

	// The copy constructor (0x1000bcdc / 181) and the assignment
	// (0x1000bd94 / 144) are deliberately IMPLICIT — see the note above.

	uint32  m_a;                                      // +0x48
	uint32  m_b;                                      // +0x4c
	CMoArray<uint32, DefaultCache>      m_arr1;       // +0x50
	uint32  m_c;                                      // +0x64
	CMoArray<CCollElem18, DefaultCache> m_arr2;       // +0x68
	uint32  m_d;                                      // +0x7c
	CCollBlock30 m_blk1;                              // +0x80
	CCollBlock0C m_blk2;                              // +0xb0
	uint32  m_e;                                      // +0xbc
};

// Out of line, and that is the whole point of them: BaseNew and BaseDelete
// have to keep calling. The real bodies are elsewhere in the engine.
CCollElemC0Rec::CCollElemC0Rec()
{
}

CCollElemC0Rec::~CCollElemC0Rec()
{
}

// The base's one virtual is declared and never defined in the engine's own
// header; give it a body here so the vtable this translation unit now really
// does emit has something to point at.
void CCollElemC0Base::Unknown0()
{
}

// The 0xc0 array, over the element type read out above rather than over the
// opaque stand-in. It has to sit here, below the class, rather than in the
// list further up. The hand-written `CollElemC0Touch` that used to stand
// here existed only to force the copy constructor and the assignment to be
// emitted; the array itself uses both, so it is gone.
//
// WHERE THIS ARRAY LIVES IN lithtech.exe, and why the enumeration never showed
// it. The /O2 image keeps a run of 0xc0-element array functions between the
// two that already match (GenGetAt at 0x00427540 below it, GenFindElement
// at 0x00428190 above it), and every one of them is UNCLAIMED shipped code
// that this file is one register-allocation decision away from:
//
//   0x00427620  761   Append / GenAppend<C0Rec>     ours 763, 238/238 insns
//   0x00427920  800   GenRemoveAt<C0Rec>           ours 798, 253/257 insns
//   0x00427ca0  656   GenCopyList<C0Rec>           ours 656, size-exact
//   0x00427f30  598   GenAppendList<C0Rec>         ours 598, size-exact
//   0x004281c0  418   CCollElemC0Rec::operator=    ours 416, 153/154 insns
//
// The uncovered-row screen never listed the middle three: it stops at
// 400 bytes and every one of these is bigger, and 0x00427ca0 / 0x004281c0
// are additionally filtered out because an unrelated comment in
// clientworld.cpp and collelem.cpp happens to name them ("skipped",
// "ordinary client code"). They are neither.
//
// The residual in all five is the same thing, and it is NOT a source fact
// this file can express: the copy loop of CMoArray<T,C>::CopyArray2,
// inlined two levels deep through the implicit CCollElemC0Rec::operator=
// into the m_arr2 member, allocates its two `rep movsl` pointers the other
// way round. The shipped standalone CopyArray2 (0x0044cb60, CCollElem40)
// builds BOTH pointers by starting the accumulator at the scaled index and
// adding the base, and OUR standalone matches it byte for byte; at the
// deeper inline site the shipped build starts the destination from the
// base instead, and we start the source from the base. Same instruction
// count, same displacements, same calls. Record these as nears.
//
// One further reading, from the same bytes: at 0x00428073 the shipped
// GenAppendList calls 0x00428dd0, and that BaseNew is 84 bytes with an
// element constructor expanded into it where ours is 63 with a call. So
// m_arr2's element is the constructor-carrying CCollElem18L, not the
// opaque CCollElem18. Changing ONLY this member's type would leave the
// standalone `CMoArray<CCollElem18, DefaultCache>` family (0x00426cb0 …
// 0x004271f0, six functions) alone, unlike switching the instantiation
// itself — but it also rewrites five C0Rec functions that server.dll
// currently matches (Insert2 0x1000c92e, Remove2 0x1000ca72, GenAppendList
// 0x1000b6ec, GenCopyList 0x1000b62b, operator= 0x1000bd94), so it is a
// build experiment, not a change to make blind.
template class CMoArray<CCollElemC0Rec, DefaultCache>;
