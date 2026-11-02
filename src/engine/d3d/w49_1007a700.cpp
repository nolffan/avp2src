// *********************************************************************** //
//
//  w49_1007a700.cpp -- 0x1007a700, moved out of d3dr5c.cpp (/Os /Oi-)
//  into a file of its own at /O2 (the default for w<N>_*.cpp): see the
//  CLOSED note below.
//
// *********************************************************************** //

#include "ltbasedefs.h"
#include "ltlink.h"


// ======================================================================= //
//  0x1007a700 -- 164 bytes.  Its address is planted as an immediate at
//  0x1007b6b8 (a callback).  FILTER A LINKED LIST INTO A CALLER'S ARRAY.
//
//  WHAT PINS IT:
//    * a bare `ret` and `mov 0x10(%esp),%esi` before any frame: __cdecl
//      with three arguments, read at 0x10/0x14/0x18(%esp) after three
//      pushes.
//    * the walk is `mov (%esi),%ecx` then `cmp %esi,%ecx` -- ltlink.h puts
//      m_pPrev at +0x00 and m_pNext at +0x04, so this walks BACKWARDS from
//      the head, and `mov 0x8(%ecx),%edx` is m_pData at +0x08.
//    * `lea (%eax,%eax,4),%ebx ; mov %ebx,%ebp ; add %ebp,%ebp ;
//      mov 0x100895c0(%ebp,%ebx,2),%ebp` is 0x100895c0 + 20*index --
//      a table of TWENTY-byte records, and the index is
//      `movsbl 0xbc(%edx)`, a SIGNED char field.
//    * the predicate from +0x88 / +0x8c / 0x1008bd54 is the same one
//      d3dr3a.cpp reconstructed inside 0x1005d9c0, and 0x1008bd54 is
//      0x1008b880 + 0x4d4, the member 0x1007b8e0 reads through its own
//      parameter.  Here the ELSE arm is the other way round from
//      0x1005d9c0's -- `mov 0x1008bd54,%eax ; test ; je <continue>` means a
//      clear flag SKIPS -- so it is `if(!g || !(flags & 0x400)) continue;`.
//    * `test %ebx,%ebx ; jne <continue>` at 0x1007a765 re-tests `flags & 1`
//      on a path that has just proved it (see below for the source shape).
//    * the early `if (m_188) return;` really does leave the whole loop --
//      `pop %ebx ; pop %ebp ; pop %esi ; ret`, the function's own epilogue,
//      not the loop exit.
//    * the append reads the count, writes count+1 with
//      `mov $0x1,%esi ; add %ebp,%esi` (a NEW value, not an update),
//      and only then loads the array and stores through it.
// ======================================================================= //
struct Item1007a700
{
	uint8	m_pad_000[0x88];
	uint32	m_88;                                       // +0x88
	uint32	m_8c;                                       // +0x8c
	uint8	m_pad_90[0xbc - 0x90];
	char	m_bc;                                       // +0xbc
	uint8	m_pad_bd[0x188 - 0xbd];
	void*	m_188;                                       // +0x188 (see below)
};

struct Desc1007a700
{
	void*	m_p00;                                      // +0x00
	uint8	m_pad_04[0x14 - 0x04];                      // 20-byte stride
};

extern Desc1007a700	g_100895c0[];                       // 0x100895c0
extern uint32		g_1008bd54;                         // 0x1008bd54

//  NEAR (was 153 bytes against 176, now 171).  Two source facts:
//    * the first argument is the list's HEAD LINK, not the LTList: the
//      walk starts `mov (%esi),%ecx ; cmp %esi,%ecx` with no +0x04, and the
//      loop test compares against the argument itself.
//    * the flag test is NOT an if/else.  The shipped `mov %ebp,%ebx ; and
//      $0x1,%ebx` keeps `m_88 & 1` as a value and re-tests it (`test
//      %ebx,%ebx ; jne continue`) in a block entered both when the
//      then-condition held and when the bit is clear -- which is exactly
//      `if (!(f & 1) || (g && (m_8c & 1))) { if (f & 1) continue; if (!g ||
//      !(f & 0x400)) continue; }`, the same predicate; the if/else form
//      tests the bit once and branches straight to `continue`.  Written so,
//      that block is instruction for instruction the shipped one.
//  CLOSED: IDENTICAL, by two more facts.
//    * +0x188 is a POINTER: icl predicts a pointer non-null, so the
//      `if (m_188) return;` return is the fall-through with its own
//      epilogue copy, as shipped (as uint32 the return jumps out).
//    * the function is /O2, not d3dr5c.cpp's /Os /Oi-: the shipped count
//      store is `mov $0x1,%esi ; add %ebp,%esi` (/Os: `xor ; inc`), the
//      loop exit duplicates the epilogue and the pad is `lea` nops.
//      Measured: the pointer at /Os /Oi- 36 bytes off; the uint32 at /O2
//      192 bytes long; both together identical (at /O2, /O2 /Oi- and
//      /O2 /Qip alike).
//  (Before: register assignment and the inline return, under /Os.)
void sub_1007a700(LTLink* pHead, Item1007a700*** ppArray, uint32* pnCount)
{
	LTLink*				pCur;
	Item1007a700*		pItem;
	uint32				nIndex;

	for (pCur = pHead->m_pPrev; pCur != pHead; pCur = pCur->m_pPrev)
	{
		pItem = (Item1007a700*)pCur->m_pData;

		if (pItem->m_188)
			return;

		if (!g_100895c0[pItem->m_bc].m_p00)
			continue;

		if (pItem->m_bc == 7)
			continue;

		if (pItem->m_8c & 0x40)
			continue;

		if (!(pItem->m_88 & 1) || (g_1008bd54 && (pItem->m_8c & 1)))
		{
			if (pItem->m_88 & 1)
				continue;

			if (!g_1008bd54 || !(pItem->m_88 & 0x400))
				continue;
		}

		nIndex = *pnCount;
		*pnCount = nIndex + 1;
		(*ppArray)[nIndex] = pItem;
	}
}
