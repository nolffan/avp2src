// *********************************************************************** //
//
//  MODULE:   WC1_R5C (.CPP)  --  functions of d3dr5c.cpp's range that
//            need /O2 (the default group) where d3dr5c.cpp is /Os /Oi-.
//            Both ship with /O2's sixteen-byte padding, and 0x10050284
//            with /O2's register saves INTO the frame (`mov %esi,0x4(%esp)`)
//            rather than pushes.
//
//  Measured on 0x1003ac30: the shipped function is followed by fifteen
//  bytes of /O2 padding (`nop` and two seven-byte `lea`s up to the next
//  16-byte boundary; /Os pads to four), and at /O2 it is byte-identical,
//  padding included.  At /Os and /Os /Oi- the only difference is the
//  atexit argument, `push`ed where the shipped code stores it into the
//  outgoing area.  d3dr5c_o2.cpp is the existing /O2 half of d3dr5c.cpp;
//  this is a separate new file.
//
// *********************************************************************** //

#include "ltbasedefs.h"
#include "ltlink.h"
#include "../../LT2/lithshared/stdlith/dynarray.h"
#include <stdlib.h>

// 0x10091df0 -- stdlith's g_DefAlloc, declared as d3dr5c.cpp declares it.
class LAllocR5C
{
public:
	virtual				~LAllocR5C();                   // vtable +0x00
	virtual void*		Alloc(uint32 nSize);            // vtable +0x04
	virtual void		Free(void* pMem);               // vtable +0x08
};

extern LAllocR5C g_10091df0;

struct CVarR5C
{
	CVarR5C*	sub_10021f40(const char* pName, float fVal,
							 uint32 nA, uint32 nB);     // 0x10021f40
};


// ======================================================================= //
//  0x1003ac30 -- 177 bytes.  In .data at 0x100890c0.  A MODULE INIT:
//  two list heads, a struct bank, an atexit, one sub-object init and one
//  console variable.
//
//  WHAT PINS EACH PIECE:
//    * 0x1008e694 gets a literal 0 and 0x1008e698/0x1008e69c both get the
//      address 0x1008e698.  ltlink.h's LTList is `{DWORD m_nElements;
//      LTLink m_Head;}` and CheapLTLink is `{m_pPrev, m_pNext}`, so that is
//      m_nElements at +0x00 and m_Head tied off at +0x04 -- dl_InitList,
//      exactly.
//    * 0x1008e6a4/0x1008e6a8 both get 0x1008e6a4 and there is no count
//      beside them: a bare LTLink tied off.
//    * 0x1007fb20(0x1008e644, 0x4c, 0x20) is the struct bank init
//      d3dr4b.cpp already writes as `sub_1007fb20(&m_Pool, 0x28, 0x20)` --
//      element size then grow-by.  0x1008e640 is set to 0x10089510 first,
//      which is the bank's `this` slot ahead of it.
//    * 0x10080502 is atexit (`push 0x4(%esp) ; call _atexit ;
//      neg ; sbb ; neg ; dec`), so 0x1003acf0 is this module's terminator.
//    * 0x1007a0d0 is reached with `mov $0x1008daa0,%ecx` and nothing
//      pushed -- a thiscall no-argument member on a global.
//
//  MY COIN-FLIP HERE, and it is worth one build.  The two tie-offs are NOT
//  spelled the same way in the shipped code: 0x1008e698 is planted as an
//  IMMEDIATE twice (`c7 05 9c e6 08 10 98 e6 08 10` and
//  `c7 05 98 e6 08 10 98 e6 08 10`, two relocations) while 0x1008e6a4 is
//  planted from a SHARED register (`mov $0x1008e6a4,%eax` once, stored
//  twice).  09-codegen-patterns.md's "same function pointer in two slots"
//  entry reads that difference as two source expressions against one, so
//  the first list is written out by hand -- two separate `&m_Head`s -- and
//  the second uses TieOff(), which is `m_pPrev = m_pNext = this`.  IF THE
//  FIRST LIST COMES OUT WITH A SHARED REGISTER, the answer is that both are
//  dl_InitList(&g_1008e694) / dl_TieOff(&g_1008e6a4) and the immediate
//  duplication is icl's, not the source's.
//
//  IDENTICAL (was 38 bytes off).  The coin-flip held.  What was missing was
//  a THIRD tie-off: after the atexit the shipped code plants $0x1008e634
//  from a register into 0x1008e634 and 0x1008e638 -- the texture list head
//  big0ae20.cpp and big2ef34.cpp walk, defined in d3ddata.cpp as
//  `RTexLink g_1008e634` (declared here under that type so it links).  The
//  rest was the flag set: see the file banner.
// ======================================================================= //

extern LTList g_1008e694;                               // 0x1008e694
extern LTLink g_1008e6a4;                               // 0x1008e6a4
extern void*  g_1008e640;                               // 0x1008e640
extern uint8  g_10089510[];                             // 0x10089510
extern uint8  g_1008e644[];                             // 0x1008e644, the bank

extern CVarR5C g_1008da80;                              // S3TCEnable

struct Obj1007a0d0
{
	void	sub_1007a0d0();                             // 0x1007a0d0
};

extern Obj1007a0d0 g_1008daa0;                          // 0x1008daa0

//  0x1008e634 is the texture list head big0ae20.cpp and big2ef34.cpp walk;
//  big38a90.cpp names its type RTexLink (ltlink.h's LTLink).
struct RTexLink
{
	RTexLink*	m_pPrev;                                // +0x00
	RTexLink*	m_pNext;                                // +0x04
	void*		m_pData;                                // +0x08

	void		TieOff()	{ m_pPrev = m_pNext = this; }
};

extern RTexLink g_1008e634;                             // 0x1008e634

void sub_1007fb20(void* pBank, uint32 nElementSize, uint32 nGrowBy);
void __cdecl sub_1003acf0();                            // the atexit body



void sub_1003ac30()
{
	g_1008e694.m_Head.m_pNext = &g_1008e694.m_Head;
	g_1008e694.m_Head.m_pPrev = &g_1008e694.m_Head;
	g_1008e694.m_nElements = 0;

	g_1008e6a4.TieOff();

	g_1008e640 = g_10089510;

	sub_1007fb20(g_1008e644, 0x4c, 0x20);

	atexit(sub_1003acf0);

	g_1008e634.TieOff();

	g_1008daa0.sub_1007a0d0();

	g_1008da80.sub_10021f40("S3TCEnable", 1.0f, 0, 0);
}


// ---------------------------------------------------------------------- //
//  0x10050284 -- 172 bytes, 1 caller (0x1004fd10, which sums five counts
//  and hands the total in with %ecx = 0x1008ef20).  A GROW-ONLY WRAPPER
//  WHOSE SetSize2 IS FULLY INLINED, over a twelve-byte T.
//
//  WHAT PINS IT AS TWO NESTED CONDITIONS AND NOT ONE:
//  four branches come off the single `cmp 0x8(%ebx),%esi`.
//
//      cmp 0x8(%ebx),%esi
//      ja   0x100502a8       ; newSize > m_nElements -- the wrapper's guard
//      <epilogue>            ; falls straight out, %eax never set
//   0x100502a8:
//      jne  0x100502ae       ; the SAME compare, re-tested
//      test %esi,%esi
//      jne  <epilogue>       ; (newSize==m_nElements)&&(newSize!=0)
//
//  The second pair is SetSize2's own first statement, kept even though the
//  `ja` has just proved it false -- 09-codegen-patterns.md's "four branches
//  off one cmp means a condition the compiler could not satisfy but would
//  not delete".  And %eax is never set on any exit, so the wrapper returns
//  void and throws SetSize2's BOOL away.
//
//  WHAT PINS THE INLINING RATHER THAN A CALL: the allocator is reached
//  directly -- `mov 0x10091df0,%eax ; mov $0x10091df0,%ecx ;
//  mov 0x4(%eax),%eax ; call *%eax` -- with the element count multiplied
//  by twelve (`lea 0x0(,%esi,8),%edx ; lea (%edx,%esi,4),%edx`) in the
//  wrapper's own frame.  Template members are implicitly inline, so /Ob1
//  expands SetSize2, _DeleteAndDestroyArray, _AllocateTArray and
//  l_allocator.h's BaseNew/BaseDelete all the way down; they are written
//  out here in the class body so icl has the same licence.
//
//  WHAT SAYS T HAS NEITHER A CONSTRUCTOR NOR A DESTRUCTOR: BaseNew's
//  construction loop and BaseDelete's destruction loop are BOTH absent, and
//  with them GetNumAllocatedElements -- there is no `add` of the cache size
//  anywhere, because the only consumer of that count was the dead
//  destruction loop.  Compare 0x1007e2c0 below, where the same template
//  over a sixteen-byte T with a zeroing constructor keeps its loop.
//
//  WHAT PINS THE ALLOCATOR AS A POINTER, NOT AN OBJECT: `mov <global>,%eax`
//  for the vptr plus `mov $<global>,%ecx` for `this` plus an indirect
//  `call *%eax` is the compile-time-known POINTER shape.  A virtual called
//  on the object itself devirtualises to a direct call, which is not what
//  ships -- and `&g_DefAlloc` as SetSize2's default argument is a pointer
//  by construction.
//
//  The two dead `je`s at 0x10050307 and 0x1005030e are the same null test
//  BaseDelete and _DeleteAndDestroyArray each write; icl keeps both, which
//  is 09-codegen-patterns.md's "no folding of redundant null tests".
//
//  IDENTICAL (was 122 bytes off as a hand-expanded copy of the
//  template).  The function is a member of w7_100500a0_qip.cpp's queue class
//  C100500a0 -- a CMoArray<W7Pending500a0> with a count in use at +0x14 --
//  and written over the REAL dynarray.h it is
//
//      if (nSize <= GetSize()) return;   SetSize(nSize);
//
//  The early return is what gives the guard its own epilogue copy and
//  leaves the shared exit after SetSize2's `newSize > 0` test, as shipped;
//  `if (nSize > GetSize()) SetSize(nSize);` hosts the shared exit at the
//  guard (86 bytes off), as does `GetSize() < nSize`, and NiceSetSize is
//  a different function (276 bytes).  Measured: identical at /O2 and at
//  /O2 /Qip, alone or beside its register-entry caller 0x1004fbc0 (a
//  w7_100500a0_qip.cpp copy built /O2 /Qip keeps 0x100500a0 identical and
//  0x1004fbc0 at its 122); /Os is 123 off.  Shipped it is the /Qip pair
//  0x10050280/0x10050284 in 0x1004fbc0's TU; the body does not need the
//  pair, so it stays here.
// ---------------------------------------------------------------------- //
struct W7Pending500a0                                   // w7_100500a0_qip.cpp's
{
	void*	m_pItem;                                    // +0x00
	void	(*m_pfnDraw)(void* pCtx, void* pItem);      // +0x04
	uint32	m_n08;                                      // +0x08
};

class C100500a0 : public CMoArray<W7Pending500a0>
{
public:
	void	sub_10050284(uint32 nSize);                 // 0x10050284

	uint32	m_nUsed;                                    // +0x14
};

void C100500a0::sub_10050284(uint32 nSize)
{
	if (nSize <= GetSize())
		return;

	SetSize(nSize);
}
