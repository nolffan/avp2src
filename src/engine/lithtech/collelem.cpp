// *********************************************************************** //
//
//  lithtech.exe — 0x00428000..0x0042c000, the COLLECTION half of the run.
//
//  WHAT THIS RANGE TURNED OUT TO BE.  The indented string pool suggests "a formatted report
//  or console/status dump".  The indented
//  strings are real and they are all in ONE function (0x0042bd30, the last
//  row in the range) — a counter-table fill, written up at the foot of this
//  file as a COMMENT ONLY, not as code.  Everything else in the six `ptr`
//  rows and a dozen of the `call` rows is stdlith's growable array
//  `CMoArray<T, C>`, instantiated over two client-only element types.
//
//  THE EVIDENCE, in the order it was read:
//
//    * The six `ptr` rows are not in the ILTClient vtable at all.  They are
//      slots of two TWELVE-slot tables, 0x004a2160 and 0x004a21c0, and
//      twelve slots in that order is genlist.h's `GenList<T>` — the same
//      twelve that src/engine/server/collection.h already documents:
//
//        0 GenBegin      4 GenAppend    8  GenCopyList
//        1 GenIsValid    5 GenRemoveAt  9  GenAppendList
//        2 GenGetNext    6 GenRemoveAll 10 GenFindElement
//        3 GenGetAt      7 GenGetSize   11 GenSetCacheSize
//
//      Both tables carry 0x004915f0 in slot 11 (`mov arg,0x10(%ecx); ret
//      $4`), which is `DefaultCache`'s GenSetCacheSize — so both arrays are
//      `CMoArray<T, DefaultCache>`, 0x14 bytes.  The abstract base's own
//      vtable is 0x004a21f0 (eleven `_purecall` at 0x004928e7 and the empty
//      GenSetCacheSize body at 0x00459f70); both element copy sequences
//      store it, which is what says these are GenList derivations and not
//      some other twelve-method interface.
//
//    * ELEMENT SIZES come off GenFindElement, which hands
//      MoArray_FindElementMemcmp a literal sizeof(T):
//          0x004915c0  push $0x8   -> vtable 0x004a21c0 holds an 8-byte T
//          0x0042afc0  push $0x1c  -> vtable 0x004a2160 holds a 0x1c-byte T
//      and is confirmed by the indexing (`lea (%ecx,%eax,8)` at 0x004126f0,
//      `add $0x1c,%esi` at 0x0042bbae).
//
//    * THE 8-BYTE ELEMENT HAS A CONSTRUCTOR, and the constructor is fully
//      visible.  BaseNew<T> for it is 0x0042bb20, and the placement-new
//      loop that a constructor-less element folds away to nothing is still
//      there:
//          0042bb4a:  movl $0x3f800000,(%ecx)     ; +0x00 = 1.0f
//          0042bb50:  mov  %edx,0x4(%ecx)         ; +0x04 = 0  (edx is the
//                                                  ; zero the null test
//                                                  ; already materialised)
//      So the element is two words, the first initialised to 1.0f and the
//      second to zero, in that store order.  `movl $0x3f800000` rather than
//      `fld1/fstp` is VC6 storing a float CONSTANT as an integer immediate;
//      it means the second member could equally be `float m_f04 = 0.0f`,
//      and nothing in the image separates the two readings.  The names
//      below are ours; only the two initialisers are read off bytes.
//
//    * THE 0x1c ELEMENT CONTAINS THE 8-BYTE ARRAY.  BaseDelete for it
//      (0x0042bb60) walks the block at stride 0x1c and, at each element,
//      inlines a `CMoArray` destructor whose vptr store lands at +0x08:
//          0042bb75:  add  $0xc,%esi                 ; esi = elem + 0x0c
//          0042bb7c:  movl $0x4a21c0,-0x4(%esi)      ; vptr   at elem+0x08
//          0042bb7a:  mov  (%esi),%eax               ; m_pArray  at +0x0c
//          0042bb87:  mov  0x4(%esi),%ecx            ; m_nElements at +0x10
//          0042bb8a:  mov  0x8(%esi),%edx            ; m_CacheSize at +0x14
//      0x08 + 0x14 = 0x1c exactly, so the element is two leading dwords and
//      then a `CMoArray<the 8-byte T, DefaultCache>`.  The same three
//      offsets appear in 0x0042b9b0 and in the scalar deleting destructor
//      at 0x0042bcc0.
//
//      NOTE: the decoded symbol map names 0x0042bcc0
//      `??_GModelWeightSet@@QAEPAXI@Z` on a bridged call site.  That name is
//      WRONG for this image.  commonmodel.h's ModelWeightSet is 0x28 bytes
//      with its collection at +0x10 and the model pointer at +0x24; the
//      shipped 0x0042bcc0 touches +0x08/+0x0c/+0x10/+0x14 and nothing else.
//      It is the 0x1c element's destructor, and the 94-byte agreement with
//      ModelWeightSet's is a shape coincidence between two classes that
//      both end in one `CMoArray<..., DefaultCache>` member.
//
//    * WHO OWNS THEM.  The constructor at 0x00429a80 builds an object whose
//      +0x04 is a `CMoArray<4-byte T, DefaultCache>` (vtable 0x004a2190)
//      and whose +0x18 is the 0x1c-element array (vtable 0x004a2160), then
//      sets +0x48 = 0x9e, +0x4c = 0x4e, +0x50 = 2 and zeroes +0x38..+0x44
//      and +0x54..+0x60.  Its destructor is 0x00429b40.  The owning class
//      is NOT identified here; the constants 158 / 78 / 2 are recorded so
//      whoever names it has them.
//
//  WHY THIS MATTERS MORE THAN THE SIX ROWS.  The lithtech build already
//  compiles ..\server\collection.cpp, so the template itself is proven
//  source.  What was missing was these two ELEMENT TYPES.  Adding them
//  reaches, on the byte evidence above:
//
//      0x0042a160  298  GenAppend<T08>        0x0042a760  457  GenAppend<T1C>
//      0x0042a290  271  GenRemoveAt<T08>      0x0042a930  588  GenRemoveAt<T1C>
//      0x0042a3a0  ~304 GenCopyList<T08>      0x0042abe0  478  GenCopyList<T1C>
//      0x0042a4d0  ~224 GenAppendList<T08>    0x0042adc0  501  GenAppendList<T1C>
//      0x0042a6b0   82  GenGetNext<T1C>       0x0042a710   76  GenGetAt<T1C>
//      0x0042ab80   88  GenRemoveAll<T1C>     0x0042afc0   ~40 GenFindElement<T1C>
//      0x0042b270  259  SetSize2<T1C>         0x0042b9b0  109  the T1C array's
//                                                              destroy-and-free
//      0x0042ba20  241  CopyArray2<T08>       0x0042bb20   59  BaseNew<T08>
//      0x0042bb60  102  BaseDelete<T1C>       0x0042bbd0  238  BaseNew<T1C>
//
//  CALIBRATION, HONESTLY.  The equivalent already-instantiated 8-byte
//  element in the lithtech build (`CCollElem08`, a plain POD) produces a
//  GenRemoveAt of 288 bytes against the shipped 271, and a byte diff shows
//  the whole residual is one inline-layer decision: shipped keeps BaseNew
//  as a call (`push count; push $0; push &g_DefAlloc; call 0x0042bb20`),
//  ours expands it to `pAlloc->Alloc(n*8)` in line.  Giving the element a
//  CONSTRUCTOR is exactly the lever that should flip that — BaseNew grows a
//  construction loop and stops fitting VC6's inline budget, which is why
//  the shipped 0x0042bb20 exists as a separate function at all.  That is a
//  hypothesis the byte evidence supports, not a result: it was not built.
//
//  WHAT IS NOT WRITTEN.  Thirteen `call` rows in the range are ordinary
//  client code and are untouched: 0x004281c0, 0x00428520, 0x004285e0,
//  0x00428810, 0x00428cc0, 0x00429250, 0x00429440, 0x004296a0, 0x00429860,
//  0x00429bc0, 0x0042a050.  (0x004296a0 is obj_LinkIntoWorldTree, already
//  in src/engine/server/ltobject.cpp.  0x00429980, 0x00429ca0, 0x00429d30,
//  0x00429d40 and 0x00429e00 are now written below, in THE SURFACE CACHE.)  0x0042bd30 is written
//  up at the foot of this file as a comment, with its table decoded, but
//  not as code.
//
//  Every symbol introduced here carries the Q428 suffix so this object can
//  be compiled alongside the existing TUs without colliding.
//
// *********************************************************************** //
#include <string.h>

#include "collection.h"
#include "clientinterface.h"


// ======================================================================= //
//  The 8-byte element (vtable 0x004a21c0)
// ======================================================================= //
//
//  Read entirely off BaseNew's construction loop at 0x0042bb4a..0x0042bb50.
//  Two words; the first is 1.0f, the second is zero, and the stores run in
//  layout order.  The names are ours — a 1.0f-initialised leading float in
//  an array that is itself a member of a per-something record is most
//  likely a weight or a scale, but nothing in the image says so.
class CElem08Q428
{
public:
	CElem08Q428()
	{
		m_f00 = 1.0f;
		m_n04 = 0;
	}

	float  m_f00;                            // +0x00  = 1.0f
	uint32 m_n04;                            // +0x04  = 0
};


// ======================================================================= //
//  The 0x1c element (vtable 0x004a2160)
// ======================================================================= //
//
//  LAYOUT.  Two leading dwords and then the 8-byte-element array, proved by
//  the three offsets BaseDelete<T1C> (0x0042bb60) and the destructor
//  (0x0042bcc0) touch: vptr at +0x08, m_pArray at +0x0c, m_nElements at
//  +0x10, m_CacheSize at +0x14.  8 + 0x14 = 0x1c, the stride the walk uses.
//  The two leading dwords are copied as two plain `mov`s by every copy in
//  the image (0x0042a6b0 GenGetNext, 0x0042a7bf inside GenAppend), so they
//  are scalars; nothing says which scalars.
//
//  BOTH CONSTRUCTORS ARE IN THE IMAGE AND BOTH DECOMPOSE EXACTLY.
//
//  0x0042b580 (171 bytes) is the two-argument one, `__thiscall` returning
//  `this` with `ret $0x8`.  Reading it against collection.h:
//
//    0042b592  mov %eax,(%ebx)          m_n00 = arg1
//    0042b594  mov %ecx,0x4(%ebx)       m_n04 = arg2
//    0042b59c  movl $0x4a21c0,(%esi)    ] the member's implicit
//    0042b5a2..0042b5ae  four zeros     ] CMoArray() -> Clear()
//    0042b5b1  call 0x0042b0a0          ]   Init()'s Term()  = SetSize2(0,&g_DefAlloc)
//    0042b5b6..0042b5c4  four zeros     ]   Init()'s _InitArray(0)
//    0042b5c7..0042b5de  if(m_pArray)   ]   Init()'s SetSize(0), inlined
//                        _DeleteAndDestroyArray; m_nElements = 0
//    0042b5ec  call 0x0042b0a0          ] the BODY: Init(0,0x400)'s Term()
//    0042b5f1  m_pArray = 0             ]   _InitArray(0x400)
//    0042b5f4  m_nElements = 0          ]
//    0042b5f7  movl $0x400,0x10(%esi)   ]   SetWantedCache(0x400)
//    0042b603  m_CacheSize = 0          ]
//    0042b5fe..0042b620  if(m_pArray)   ]   SetSize(0), inlined
//                        _DeleteAndDestroyArray; m_nElements = 0
//
//  so the body is exactly one statement, `m_Array08.Init(0, 0x400)`, and
//  0x400 is read straight off 0x0042b5f7.  The SAME tail, instruction for
//  instruction, is expanded inside BaseNew<T1C> at 0x0042bc19..0x0042bca8
//  with the two leading stores zeroed instead of copied, which is the
//  default constructor.  Two constructors are written below rather than one
//  with defaulted arguments: 0x0042b580 is an out-of-line body taking two
//  arguments, while BaseNew<T1C> expands a zeroing body in line, which
//  needs an inline default constructor of its own.
//
//  NOTE the store order in both: the two scalars go down BEFORE the
//  member's vptr (0x0042b592, 0x0042bc19).  That is SOURCE order, not
//  scheduling: they are mem-initialisers, and m_n00/m_n04 are declared
//  ahead of m_Array08, so they run before its constructor.  Written as
//  body assignments /O2 kept them after the member construction (both
//  functions near, same size).  The default constructor must also be
//  INLINE: out of line, BaseNew<T1C> called it and was 67 bytes.
class CElem1CQ428
{
public:
	CElem1CQ428();                           // inlined; see BaseNew 0x0042bbd0
	CElem1CQ428(uint32 n00, uint32 n04);     // 0x0042b580

	uint32 m_n00;                            // +0x00
	uint32 m_n04;                            // +0x04
	CMoArray<CElem08Q428, DefaultCache> m_Array08;   // +0x08 .. +0x1c
};

// 0x0042b580.  `m_Array08` is default-constructed by the implicit member
// initialisation first — that is the 0x0042b59c..0x0042b5de block — and the
// one statement of the body is the re-Init with the 0x400 cache.
CElem1CQ428::CElem1CQ428(uint32 n00, uint32 n04)
	: m_n00(n00), m_n04(n04)
{
	m_Array08.Init(0, 0x400);
}

// Expanded inside BaseNew<T1C> (0x0042bbd0) and nowhere else in the image.
inline CElem1CQ428::CElem1CQ428()
	: m_n00(0), m_n04(0)
{
	m_Array08.Init(0, 0x400);
}


// ======================================================================= //
//  The instantiations
// ======================================================================= //
//
//  Same form as src/engine/server/collection.cpp's list.  VC6 cannot
//  explicitly instantiate a constructor (C2909), so the container's
//  wrappers come along for the ride and will read as unmatched; that is
//  the documented cost of asking for the vtable at all.

template class CMoArray<CElem08Q428, DefaultCache>;
template class CMoArray<CElem1CQ428, DefaultCache>;

// 0x0042bb20 (59 bytes) — the 8-byte element's allocator, the one whose
// construction loop named the element.  0x0042bbd0 (238) is the 0x1c
// element's, and it is large because each element's CMoArray member has to
// be Clear()ed, Init()ed and re-Init()ed with the 0x400 cache in the loop.
template CElem08Q428* BaseNew(LAlloc*, CElem08Q428*, uint32);
template CElem1CQ428* BaseNew(LAlloc*, CElem1CQ428*, uint32);

// 0x0042bb60 (102 bytes) — the 0x1c element's releaser.  The 8-byte
// element has no destructor, so its BaseDelete folds into every caller and
// there is no separate symbol for it in the image.
template void BaseDelete(LAlloc*, CElem1CQ428*, uint32);


// ======================================================================= //
//  THE SURFACE CACHE THAT OWNS BOTH ARRAYS -- 0x00429810 .. 0x0042a14f
// ======================================================================= //
//
//  This file is the natural home of that run: the two CMoArray
//  instantiations above are its members (outer +0x04 and +0x18), and their
//  template COMDATs start at 0x0042a160, immediately after its last method.
//  Other methods of the same two classes are already written under view
//  names elsewhere and are only DECLARED here:
//      0x00429820 IsReady, 0x00429830 Free    clientworld.cpp (CHandleWrapperView)
//      0x00429a20 item SetRect, 0x00429fd0 GetCellRect, 0x0042a050,
//      0x0042a100                             cmgsurf.cpp PART C
//  The layouts are cmgsurf.cpp's CSub11F0ItemQ428 / CSub11F0Q428.  The
//  0x1c element's +0x04 dword (CElem1CQ428::m_n04) is the item pointer:
//  0x00429d6a loads it and 0x00429dcc calls SetRect on it.

class CSurfItemM5
{
public:
	~CSurfItemM5();                          // 0x00429810 (`jmp Free`)

	LTRESULT IsReady();                      // 0x00429820
	LTRESULT Free();                         // 0x00429830
	LTRESULT Plot(CElem08Q428* pSample, LTBOOL bNoScroll);   // 0x00429860 (name ours)
	LTRESULT Draw();                         // 0x00429980
	LTRESULT SetRect(LTRect* pRect);         // 0x00429a20

public:
	CLTClient* m_pClientDE;                  // 0x00
	HSURFACE   m_hSurface;                   // 0x04
	HSURFACE   m_hSurface2;                  // 0x08
	uint32     m_Unknown0c;                  // 0x0c
	int        m_nTopInset;                  // 0x10
	LTRect     m_rcRect;                     // 0x14
};

class CSurfCacheM5
{
public:
	LTRESULT IsReady();                      // 0x00429820 (same body, +0x00)
	LTRESULT Init(CLTClient* pClientDE, LTRect* pRect);   // 0x00429bc0
	LTRESULT Term();                         // 0x00429ca0
	void     ResetScroll();                  // 0x00429d30 (name ours)
	void     Remove(uint32 iIndex);          // 0x00429d40
	void     RemoveAll();                    // 0x00429e00
	LTRESULT Update();                       // 0x00429e60 (name ours)
	LTBOOL   GetCellRect(LTRect* pRect, uint32 nIndex);   // 0x00429fd0
	LTRESULT SetCellSize(uint32 nWidth, uint32 nHeight);  // 0x0042a050
	LTRESULT SetRect(LTRect* pRect);         // 0x0042a100 (cmgsurf.cpp)

public:
	CLTClient*                          m_pClientDE;      // 0x00
	CMoArray<uint32, DefaultCache>      m_Array04;        // 0x04  vptr 0x004a2190
	CMoArray<CElem1CQ428, DefaultCache> m_Array18;        // 0x18  vptr 0x004a2160
	uint32   m_Unknown2c;                                 // 0x2c
	uint32   m_Unknown30;                                 // 0x30
	uint32   m_Unknown34;                                 // 0x34
	LTRect   m_rcArea;                                    // 0x38
	uint32   m_nCellWidth;                                // 0x48
	uint32   m_nCellHeight;                               // 0x4c
	uint32   m_nSpacing;                                  // 0x50
	uint32   m_nCols;                                     // 0x54
	uint32   m_nRows;                                     // 0x58
	uint32   m_nExtraX;                                   // 0x5c
	uint32   m_nExtraY;                                   // 0x60
};

// 0x00429810, 5 bytes -- `jmp 0x00429830`: the destructor is Free(), its
// answer dropped, as a tail call.  Remove (0x00429d40) calls it before
// operator delete.
CSurfItemM5::~CSurfItemM5()
{
	Free();
}

// 0x00429860, 280 bytes.  Plot one sample into the item's surface as a
// scrolling strip chart (name ours): unless bNoScroll, scroll the surface
// one pixel left (DrawSurfaceToSurface onto itself, slot 0x1a4, source
// rect (2,1)-(w,h-1) to (1,1)) and clear the right-hand column (FillRect,
// slot 0x1cc, colour 0); then fill the column from the sample's height
// fraction (+0x00, `fmuls (%ecx)` then _ftol) down, in its colour (+0x04).
// The 8-byte sample is the CElem08Q428 of each 0x1c slot's inner array
// (0x00429ec8 passes &m_pArray[j], and `j != 0` as bNoScroll).
LTRESULT CSurfItemM5::Plot(CElem08Q428* pSample, LTBOOL bNoScroll)
{
	LTRESULT dResult;
	LTRect rcSrc;
	LTRect rcBar;
	int nWidth, nBar;

	if (IsReady() != LT_OK)
		return LT_NOTINITIALIZED;

	nWidth = m_rcRect.right - m_rcRect.left;
	rcBar.left  = nWidth - 2;
	rcBar.right = rcBar.left + 1;

	if (!bNoScroll)
	{
		rcSrc.left   = 2;
		rcSrc.top    = 1;
		rcSrc.right  = nWidth;
		rcSrc.bottom = m_rcRect.bottom - m_rcRect.top - 1;

		dResult = m_pClientDE->DrawSurfaceToSurface(m_hSurface, m_hSurface, &rcSrc, 1, 1);
		if (dResult != LT_OK)
			return dResult;

		rcBar.top    = 1;
		rcBar.bottom = m_rcRect.bottom - m_rcRect.top - 1;

		dResult = m_pClientDE->FillRect(m_hSurface, &rcBar, 0);
		if (dResult != LT_OK)
			return dResult;
	}

	// Written through m_rcRect, not a height local: a shared height local
	// swaps %ebx/%ebp against 0x00429924..0x00429961.
	nBar = (int)((float)(m_rcRect.bottom - m_rcRect.top) * pSample->m_f00);
	rcBar.top    = m_rcRect.bottom - nBar - m_rcRect.top - 2;
	rcBar.bottom = m_rcRect.bottom - m_rcRect.top - 1;

	return m_pClientDE->FillRect(m_hSurface, &rcBar, pSample->m_n04);
}


// 0x00429980, 152 bytes.  Blit the item's surface to the screen at its
// rect's top-left, and the optional second surface directly above it.
//
//   * the IsReady refusal is the constant 0x49 (`mov $0x49,%eax`), not
//     IsReady's own answer.
//   * each blit pushes 0x80000000, top, left, 0, the surface, then calls
//     GetScreenSurface (slot 0x168) and pushes its result: one nested call,
//     DrawSurfaceToSurfaceTransparent (slot 0x1a8), per surface.
//   * the second surface's height comes from GetSurfaceDims (slot 0x194),
//     and only the HEIGHT is read back (0x004299e1): top - height.
LTRESULT CSurfItemM5::Draw()
{
	LTRESULT dResult;
	uint32 nWidth, nHeight;

	if (IsReady() != LT_OK)
		return LT_NOTINITIALIZED;

	dResult = m_pClientDE->DrawSurfaceToSurfaceTransparent(
		m_pClientDE->GetScreenSurface(), m_hSurface, LTNULL,
		m_rcRect.left, m_rcRect.top, 0x80000000);

	if (dResult != LT_OK)
		return dResult;

	if (m_hSurface2)
	{
		m_pClientDE->GetSurfaceDims(m_hSurface2, &nWidth, &nHeight);

		dResult = m_pClientDE->DrawSurfaceToSurfaceTransparent(
			m_pClientDE->GetScreenSurface(), m_hSurface2, LTNULL,
			m_rcRect.left, m_rcRect.top - nHeight, 0x80000000);

		if (dResult != LT_OK)
			return dResult;
	}

	return LT_OK;
}


// 0x00429bc0, 215 bytes.  thiscall, `ret $0x8`; the one caller is the
// renderer/video start-up at 0x0040fc61.  No interface is LT_ERROR (the
// `mov $1,%eax` arm); SetRect's own failure is handed straight back (the
// `jne` to the epilogue with %eax untouched); a live cache is torn down
// first (IsReady read as a boolean, then Term); then both arrays are
// re-Init()ed with a zero cache -- the +0x04 array's Term() stays a call
// to SetSize2 (0x00414fa0), the +0x18 array's is expanded -- and the
// interface pointer is stored last.
LTRESULT CSurfCacheM5::Init(CLTClient* pClientDE, LTRect* pRect)
{
	LTRESULT dResult;

	if (!pClientDE)
		return LT_ERROR;

	dResult = SetRect(pRect);
	if (dResult != LT_OK)
		return dResult;

	if (IsReady())
		Term();

	m_Array04.Init(0, 0);
	m_Array18.Init(0, 0);

	m_pClientDE = pClientDE;

	return LT_OK;
}


// 0x00429ca0, 144 bytes (the enumeration cuts it at the early `ret` at
// 0x00429cad and lists the rest as the "gap" 0x00429cae -- not a function).
// Tear down: every slot, both arrays (both Terms expanded inline), then the
// interface pointer.  IsReady's LTRESULT is again read as a boolean, as
// Init (0x00429bc0) does before calling this.
LTRESULT CSurfCacheM5::Term()
{
	if (!IsReady())
		return LT_OK;

	RemoveAll();

	m_Array04.Term();
	m_Array18.Term();

	m_pClientDE = LTNULL;

	return LT_OK;
}


// 0x00429d30, 12 bytes.  thiscall, zeroes +0x2c/+0x30/+0x34.
void CSurfCacheM5::ResetScroll()
{
	m_Unknown2c = 0;
	m_Unknown30 = 0;
	m_Unknown34 = 0;
}


// 0x00429d40, 192 bytes (the enumeration's 174 is short: the real tail runs to
// 0x00429dfd).  Remove slot iIndex from both arrays, delete its item, and
// re-lay-out every later item into its new cell.
//
//   * `delete` of the item: `call 0x00429810` (its destructor) then
//     `call 0x004290c0` (operator delete) behind one null test.
//   * `cmp 0x34(%edi),%esi; ja` -- ResetScroll when iIndex <= +0x34.
//   * the LTRect is zeroed AFTER that (0x00429d9c), so it is declared there.
//   * SetRect's answer is tested with `je` into the recursive Remove: the
//     original treats SetRect's LTRESULT as a boolean, so a SUCCESSFUL
//     SetRect (LT_OK == 0) is what removes the slot.  Reproduced as found.
void CSurfCacheM5::Remove(uint32 iIndex)
{
	CSurfItemM5* pItem;

	m_Array04.Remove(iIndex);

	pItem = (CSurfItemM5*)m_Array18[iIndex].m_n04;
	delete pItem;

	m_Array18.Remove(iIndex);

	if (iIndex <= m_Unknown34)
		ResetScroll();

	LTRect rcCell;

	for (; iIndex < m_Array18.GetSize(); iIndex++)
	{
		if (!GetCellRect(&rcCell, iIndex))
			return;

		if (!((CSurfItemM5*)m_Array18[iIndex].m_n04)->SetRect(&rcCell))
		{
			Remove(iIndex);
			return;
		}
	}
}


// 0x00429e00, 81 bytes.  Remove from the back until empty, then the inline
// Term of the +0x04 array (SetSize2(0) expanded: free through g_DefAlloc's
// slot 2, zero pointer and cache, then the count).
void CSurfCacheM5::RemoveAll()
{
	while (m_Array18.GetSize())
		Remove(m_Array18.GetSize() - 1);

	m_Array04.Term();
}


// 0x00429e60, 365 bytes (the enumeration's "0x00429e7a gap" is this body past
// the entry guard; 0x00429e60's own row says 26).  Called once per frame
// from CClientMgr::FlipScreen (clientblit.cpp's CSub11F0::Update view).
// For every slot: plot the samples queued in its inner array into the
// item (the first one scrolls), empty the queue with NiceSetSize(0) --
// its whole reallocation arm is expanded at 0x00429ef9 although newSize
// is 0 and it cannot run -- and then draw the item.  Stops at the first
// failed Draw and returns its answer.
LTRESULT CSurfCacheM5::Update()
{
	LTRESULT dResult;
	CElem1CQ428* pElem;
	CSurfItemM5* pItem;
	uint32 i, j;

	if (!m_Array18.GetSize())
		return LT_OK;

	dResult = LT_OK;
	for (i = 0; i < m_Array18.GetSize() && dResult == LT_OK; i++)
	{
		pElem = &m_Array18[i];
		pItem = (CSurfItemM5*)pElem->m_n04;

		if (pElem->m_Array08.GetSize())
		{
			for (j = 0; j < pElem->m_Array08.GetSize(); j++)
				pItem->Plot(&pElem->m_Array08[j], j != 0);

			pElem->m_Array08.NiceSetSize(0);
		}

		dResult = pItem->Draw();
	}

	return dResult;
}


// 0x0042a050, 175 bytes (declared in cmgsurf.cpp PART C as
// CSub11F0Q428::SetCellSize, not written there).  Positive guards with the
// one LT_ERROR tail at 0x0042a0f3: the cell must be at least 0x20 x 0x10
// (`jb`) and, with the gutter, fit the area (`ja`).  Then RemoveAll, store
// the size, and tile: columns/rows by (cell + spacing), the leftovers from
// the PARAMETERS (0x0042a0c9 / 0x0042a0db re-read them off the stack).
LTRESULT CSurfCacheM5::SetCellSize(uint32 nWidth, uint32 nHeight)
{
	if (nWidth >= 0x20 && nHeight >= 0x10 &&
	    m_nSpacing + nWidth <= (uint32)(m_rcArea.right - m_rcArea.left) &&
	    m_nSpacing + nHeight <= (uint32)(m_rcArea.bottom - m_rcArea.top))
	{
		RemoveAll();

		m_nCellWidth  = nWidth;
		m_nCellHeight = nHeight;

		m_nCols = (m_rcArea.right - m_rcArea.left) / (nWidth + m_nSpacing);
		m_nRows = (m_rcArea.bottom - m_rcArea.top) / (m_nSpacing + nHeight);

		m_nExtraX = m_rcArea.right - m_nCols * nWidth - m_rcArea.left;
		m_nExtraY = m_rcArea.bottom - m_nRows * nHeight - m_rcArea.top;

		return LT_OK;
	}

	return LT_ERROR;
}


// ======================================================================= //
//  WHAT ELSE THE TWO INSTANTIATIONS SHOULD COVER
// ======================================================================= //
//
//  Read off call targets, not guessed.  Each of these is reached from a
//  function already identified above, with the argument shape that names it:
//
//    0x0042b0a0  161  CMoArray<T08,DefaultCache>::SetSize2(uint32, LAlloc*)
//                     — the target of all three Init layers in both 0x1c
//                     constructors.  Its `this` is the INNER array:
//                     0x0042bc38 and 0x0042b5b1 both call it on `elem+8`.
//    0x0042b270  259  CMoArray<T1C,DefaultCache>::SetSize2 — 0x00429adf
//                     calls it on the owner's +0x18 array in the owner's
//                     constructor, right after that array's Clear().
//    0x0042b9b0  109  CMoArray<T1C,DefaultCache>::_DeleteAndDestroyArray —
//                     thiscall (LAlloc*, uint32), walks at stride 0x1c and
//                     inlines each element's destructor.
//    0x0042ba20  241  CMoArray<T08,DefaultCache>::CopyArray2(const&, LAlloc*)
//                     — what the 0x1c element's implicit operator= reaches
//                     (0x0042a7d0, inside GenAppend<T1C>'s shift loop).
//    0x0042b150  285  and 0x0042b3f0  399  are the same `if(newSize <
//                     m_nElements) { m_nElements = newSize; ... }` head as
//                     InternalNiceSetSize, one per element type.
//    0x0042b740  614  the `m_WantedCache*2 <= m_CacheSize` head shared with
//                     GenRemoveAt, plus a third argument — Insert2/Remove2
//                     for the 0x1c element.
//
//  None of those needed a line of their own: an explicit instantiation of
//  the two templates emits all of them.
//
//
// ======================================================================= //
//  0x0042bd30 — the counter table.  (Now written: see the code below.)
// ======================================================================= //
//
//  This is the function the indented string pool belongs to, and it is the
//  ONLY one in the range that touches those strings.  It is not a report
//  and prints nothing: it fills seventeen 16-byte records at 0x004b3098
//  .. 0x004b31a8, each `{ void* pCounter; char* pName; int -1; int 0; }`,
//  in this literal order —
//
//      0x004b2fc4 "FOpen"              0x004b17f4 "  Server"
//      0x004b2fe8 "Malloc"             0x004b17f8 "  ProcessPackets"
//      0x004b2fec "Free"               0x004b17fc "  GameClientShell"
//      0x004b17cc "Music"              0x004b17dc "    Render"
//      0x004b17d0 "Sound"              0x004b43b0 "      TagVisibleLeaves"
//      0x004b17d4 "Input"              0x004b43b4 "      FlushObjectQueues"
//      0x004b17d8 "ClientShell"        0x004b43b8 "        Models"
//      0x004b17f0 "  Net"              0x004b43bc "        WorldModels"
//                                      0x004b43c0 "        Translucent"
//
//  — so the indentation is a TREE the dump code walks later, and the source
//  order here is the emission order.  The third word is `or $-1,%edx`
//  (0x0042bd3b and fifteen siblings) and the fourth is `xor %esi,%esi`
//  sunk into two batches (0x0042be77..0x0042beb9 for the first ten records,
//  0x0042bf62..0x0042bf80 for the rest), which is scheduling, not source.
//
//  It is left unwritten because the seventeen counter OBJECTS it points at
//  are in three different .data clusters and none of them is identified,
//  and writing the table without them would only move the guesswork into
//  this file.  Whoever picks it up: 0x004b2fc4/0x004b2fe8/0x004b2fec sit
//  together, 0x004b17cc..0x004b17fc are a contiguous run of dwords four
//  bytes apart, and 0x004b43b0..0x004b43c0 are another; that spacing says
//  the counters are four bytes each, i.e. the record's first word is a
//  pointer to a plain counter variable and not to a struct.


// ======================================================================= //
//  0x0042bd30 (617 bytes) -- the counter table's dynamic initialiser
// ======================================================================= //
//
//  REAL HOME: the demo-manager TU (0x0042bd00..0x0042ca50; views in
//  clientworld.cpp and clientmgr3.cpp).  No file for it exists, so it is
//  written here.  0x0042bd20 (`jmp 0x0042bd30`, the `ptr` row reached from
//  the CRT initialiser table at 0x004a7004) is VC6's outer _$E thunk and
//  0x0042bd30 the inner one: the table is a file-scope array of a class
//  with an inline constructor, so each element's construction is code.
//  The walk at 0x0042c7cd (dmgrow.cpp) reads +0x08/+0x0c as a min/max
//  pair; the counters are plain 4-byte variables in three .data clusters
//  (0x004b2fc4.., 0x004b17cc.., and the renderer struct's +0x58..+0x68),
//  none of them identified, so they are declared here under our names.
//  The `xor %esi,%esi` zero stores batched at 0x0042be77 and 0x0042bf62
//  are scheduling of each record's fourth word.

//  The first twelve are .bss dwords referenced by nothing but this table (a
//  scan of the whole image finds each address exactly once, here), so they
//  are DEFINED here, zero like the shipped .bss:
//      0x004b2fc4 FOpen, 0x004b2fe8 Malloc, 0x004b2fec Free,
//      0x004b17cc Music, 0x004b17d0 Sound, 0x004b17d4 Input,
//      0x004b17d8 ClientShell, 0x004b17dc Render, 0x004b17f0 Net,
//      0x004b17f4 Server, 0x004b17f8 ProcessPackets, 0x004b17fc GameClientShell.
//  The last five are NOT separate globals: 0x004b43b0..0x004b43c0 is
//  g_RenderStruct (0x004b4358) +0x58..+0x68, clientrender.cpp's
//  m_nTagVisibleLeaves .. m_nTranslucent, which the renderer counts into.
//  The view below carries only those five; its name is the real class's so
//  the reference is to the one g_RenderStruct.
uint32 g_nCntFOpenQ428, g_nCntMallocQ428, g_nCntFreeQ428;
uint32 g_nCntMusicQ428, g_nCntSoundQ428, g_nCntInputQ428;
uint32 g_nCntClientShellQ428, g_nCntRenderQ428;
uint32 g_nCntNetQ428, g_nCntServerQ428, g_nCntProcessPacketsQ428;
uint32 g_nCntGameClientShellQ428;

class RenderStruct
{
public:
	uint8  m_pad_000[0x58];
	uint32 m_nTagVisibleLeaves;              // 0x058
	uint32 m_nFlushObjectQueues;             // 0x05c
	uint32 m_nModels;                        // 0x060
	uint32 m_nWorldModels;                   // 0x064
	uint32 m_nTranslucent;                   // 0x068
};

extern RenderStruct g_RenderStruct;          // 0x004b4358

class CCounterEntryQ428
{
public:
	CCounterEntryQ428(uint32* pCounter, const char* pName)
	{
		m_pCounter = pCounter;
		m_pName    = pName;
		m_nMin     = (uint32)-1;
		m_nMax     = 0;
	}

	uint32*     m_pCounter;                  // +0x00
	const char* m_pName;                     // +0x04
	uint32      m_nMin;                      // +0x08
	uint32      m_nMax;                      // +0x0c
};

// 0x004b3098 .. 0x004b31a7.
CCounterEntryQ428 g_CounterTableQ428[] =
{
	CCounterEntryQ428(&g_nCntFOpenQ428,             "FOpen"),
	CCounterEntryQ428(&g_nCntMallocQ428,            "Malloc"),
	CCounterEntryQ428(&g_nCntFreeQ428,              "Free"),
	CCounterEntryQ428(&g_nCntMusicQ428,             "Music"),
	CCounterEntryQ428(&g_nCntSoundQ428,             "Sound"),
	CCounterEntryQ428(&g_nCntInputQ428,             "Input"),
	CCounterEntryQ428(&g_nCntClientShellQ428,       "ClientShell"),
	CCounterEntryQ428(&g_nCntNetQ428,               "  Net"),
	CCounterEntryQ428(&g_nCntServerQ428,            "  Server"),
	CCounterEntryQ428(&g_nCntProcessPacketsQ428,    "  ProcessPackets"),
	CCounterEntryQ428(&g_nCntGameClientShellQ428,   "  GameClientShell"),
	CCounterEntryQ428(&g_nCntRenderQ428,            "    Render"),
	CCounterEntryQ428(&g_RenderStruct.m_nTagVisibleLeaves,  "      TagVisibleLeaves"),
	CCounterEntryQ428(&g_RenderStruct.m_nFlushObjectQueues, "      FlushObjectQueues"),
	CCounterEntryQ428(&g_RenderStruct.m_nModels,           "        Models"),
	CCounterEntryQ428(&g_RenderStruct.m_nWorldModels,      "        WorldModels"),
	CCounterEntryQ428(&g_RenderStruct.m_nTranslucent,      "        Translucent"),
};


// ======================================================================= //
//  EXTENT DEFECTS IN THE ENUMERATION — all three `gap` rows are wrong
// ======================================================================= //
//
//  Checked against the shipped image before any
//  code was written, and all three failed in two of the catalogued shapes:
//
//    0x00429cae 128 "gap"  NOT A FUNCTION.  The function is 0x00429ca0 ..
//        0x00429d2e (144 bytes with two of alignment padding).  It has an
//        early `ret` at 0x00429cad, and 0x00429cae is simply the next byte
//        — the fall-through block of the entry test.  Also explains the
//        unaligned address.
//
//    0x00429d40 174 "call"  REAL START, SHORT SIZE.  Branch targets at
//        0x00429dee and 0x00429df6 leave the claimed range.  The function
//        is 0x00429d40 .. 0x00429e00 (192 bytes), with a duplicated
//        epilogue at 0x00429deb and the real tail ending 0x00429dfd.  The
//        separate "0x00429dee 18 gap" row in the enumeration is that tail.
//
//    0x00429e7a 339 "gap"  NOT A FUNCTION.  The function is 0x00429e60 ..
//        0x00429fcd.  0x00429e60 is a `sub $0x14,%esp` prologue whose
//        entry guard is
//            0042 9e6c  mov 0x20(%edx),%ecx
//            0042 9e6f  cmp %ebp,%ecx
//            0042 9e71  jne 0x00429e7a        <- the claimed "start"
//            0042 9e73  xor %eax,%eax ; ret
//        i.e. exactly the `if (!p) return 0;` fall-through of an entry test.
//        Unaligned, as expected.
//
//  Everything else in the enumeration passed: one `ret` or a plausibly
//  duplicated /O2 epilogue, no branch target leaving the range, and the
//  last instruction landing exactly on the stated end.
//
//
// ======================================================================= //
//  CHECK C — none of this is a prebuilt library
// ======================================================================= //
//
//  Every row in the range was searched for byte-identically in
//  the retail server.dll (first 48 bytes of each, exact, unmasked) and
//  none of them occurs.  That is consistent with — and does not by itself
//  prove — client-only code: collection.cpp really is shared source, but
//  the client builds it /O2 and server.dll /O1, so a match was never going
//  to happen.  The identification above rests on the vtable shape and the
//  element evidence, not on this test.


// ======================================================================= //
//  0x0044b4d0 (57 bytes) -- collection.cpp's CCollElem1CS::operator=,
//  out of line.  A driver, not a function of this file.
// ======================================================================= //
//
//  The element's assignment is in class (src/engine/server/collelem1cs.h),
//  and the shipped /O2 `Append` for its DefaultCache array (0x00448540)
//  expands it at one site and calls this copy at the other two
//  (0x00448602, 0x00448632) -- the per-site inline budget.  Our cl 8447
//  expands all three, so nothing of ours emits the COMDAT; server.dll
//  (/O1) declines and keeps 0x100195b8 matched.  Taking the address emits
//  the body, as sdkforce.cpp does for the SDK inlines, and here, in a
//  client-only TU, server.dll does not gain the pointer.  /O2 copies the
//  twelve-byte head with three `mov` pairs where /O1 has `movsl` three
//  times, which is all that separates 57 bytes from 44.
#include "collelem1cs.h"

CCollElem1CS& (CCollElem1CS::*g_pfnCollElem1CSAssign)(const CCollElem1CS&) =
	&CCollElem1CS::operator=;
