// *********************************************************************** //
//
//  w48_1006d3e0_os.cpp -- d3d.ren 0x1006d3e0, moved out of
//  d3dr3a.cpp (/O2) into an /Os TU of its own: the shipped body pads with
//  `mov %esi,%esi` and 0x90s and has no loop-head alignment, the /Os
//  shape.  Byte-identical here.
//
// *********************************************************************** //

#include "ltbasedefs.h"
#include "ltlink.h"


// ----------------------------------------------------------------------- //
//  0x1006d3e0 -- 122 bytes + padding, 1 caller (0x10039a28).
//
//  THIS IS ltlink.h.  Five iterations (%ebx walks 0..0x14 by 4 over a dword
//  array at 0x10091954, %ebp walks 0..0x3c by 12 over a 12-byte-element
//  array at 0x10091968 -- 12 bytes is sizeof(LTLink): m_pPrev, m_pNext,
//  m_pData).  Two induction variables over two arrays, which is what two
//  differently-typed arrays subscripted by the same i produce.
//
//  WHAT PINS THE HEADER CALLS, instruction for instruction:
//
//    CheapLTLink::TieOff  -- `m_pPrev = m_pNext = (LTLink*)this;`
//        stores +0x04 FIRST and +0x00 second.  The shipped code writes
//        0x10(%esp) then 0x0c(%esp).  Declaration order would be the other
//        way round; the chained assignment is what reverses it.
//
//    CheapLTLink::AddAfter --
//        pLink->m_pPrev = this;                       -> mov %eax,0x0c(%esp)
//        pLink->m_pNext = m_pNext;                    -> mov 0x4(%eax),%eax
//                                                        mov %eax,0x10(%esp)
//        pLink->m_pPrev->m_pNext = pLink->m_pNext->m_pPrev = pLink;
//                                                     -> mov %ecx,(%eax)
//                                                        mov 0x0c(%esp),%eax
//                                                        mov %edx,0x4(%eax)
//        That RELOAD of pLink->m_pPrev out of the local, three instructions
//        after storing it, is the header's third line written longhand --
//        icl 4.0 does not forward it.
//
//  The dword stored to 0x14(%esp) -- the third word of the LTLink, i.e.
//  m_pData -- is never read back, so it is the m_pData assignment and the
//  local link is only ever used as a splice.
// ----------------------------------------------------------------------- //

extern void   *g_10091954[5];                           // 0x10091954
extern LTLink  g_10091968[5];                           // 0x10091968

void sub_10039730(LTLink *pList);

//  __thiscall: the only caller (0x10039a2c) loads %ecx =
//  0x10091950, the object d3dr3d.cpp's 0x1006d460 is a member of too (its
//  +0x04 is the array at 0x10091954, its +0x18 the heads at 0x10091968);
//  the body never reads %ecx.
//
//  IDENTICAL here, at /Os, with the one-site volatile read
//  moved from the null test to the m_pData read: shipped RELOADS
//  g_10091954[i] for m_pData after the test, and a volatile TEST (the
//  d3dr3a.cpp spelling) also stops /Os strength-reducing the index into
//  the byte-offset induction variable shipped has.  The history, from
//  d3dr3a.cpp (/O2): a volatile null test, 83 -> 81 bytes differ.  /O2 aligns the loop head with a four-byte nop the shipped code
//  does not have, and reads head->m_pNext as `0x10091968+4(%ebp)` where
//  shipped reuses the `mov $0x10091968 ; add %ebp` pointer (`0x4(%eax)`).
//  /Os drops the nop but moves the link local and reorders the stores
//  (6-8 blocks); /O1, /Ox, /Oi-, /Qip, /Qpf_loop, a head-pointer local,
//  AddAfter directly, and an int index do not help.  (The whole file
//  at /Os re-measured -- 92 bytes differ, 124 bytes, the index is no
//  longer strength-reduced into the two induction variables the shipped
//  loop has.)
class C10091950
{
public:
	void	sub_1006d3e0();
};

void C10091950::sub_1006d3e0()
{
	LTLink	link;
	uint32	i;

	for (i = 0; i < 5; i++)
	{
		dl_TieOff(&link);

		if (g_10091954[i])
		{
			link.m_pData = *(void* volatile*)&g_10091954[i];
			dl_Insert(&g_10091968[i], &link);
			g_10091954[i] = LTNULL;
		}

		sub_10039730(&g_10091968[i]);
	}
}
