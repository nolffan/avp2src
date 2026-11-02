// *********************************************************************** //
//  w57_1003b1c0 -- d3d.ren: 0x1003b1c0, 116 bytes, /O2.
//
//  A STATIC INITIALISER (init-table entry at .data 0x100890d0), so it is
//  written as what makes icl generate it: two file-scope objects with an
//  inline constructor.  The bytes construct two 0x18-byte records,
//  0x1008e6d0 and 0x1008e6e8 (nothing else in .text names either; the
//  pointer pair at .data 0x10089520 holds their addresses):
//
//      +0x00 +0x04 +0x08  0          +0x0c/+0x10  a list head tied to
//      +0x14 (word) 0xffff           itself, +0x10 stored first
//      +0x16 (byte) 1 / 2            +0x17 (byte) 0
//
//  The 0 and 0xffff are hoisted into %eax/%edx (icl's reused-constant
//  habit) and the two constructions are interleaved by the scheduler.
//  The store order is the source's, read off the bytes: icl emits a run of
//  zero stores last-assigned-first except the first assignment (source
//  m_n00, m_b17, m_n08, m_n04 -> shipped +4, +8, +0x17, +0), and the
//  chained `m_pPrev = m_pNext = &head` stores +0x10 first.  IDENTICAL
//  (with the comparison's pad list taking the 12-byte `8d b6 / 8d bf`
//  pad icl ends this COMDAT with).
//
//  What the records are is not pinned by this function; the fields are
//  named by offset.  The file name carries the address, and so does icl's
//  ___sti__w57_1003b1c0_cpp_* name, which is how the byte comparison maps it.
// *********************************************************************** //

#include "ltbasedefs.h"

struct Link1003b1c0
{
	Link1003b1c0*	m_pPrev;                            // +0x00
	Link1003b1c0*	m_pNext;                            // +0x04
};

class C1003b1c0
{
public:
	C1003b1c0(uint8 nType)
	{
		m_n00 = 0;
		m_b17 = 0;
		m_n08 = 0;
		m_n04 = 0;
		m_w14 = 0xffff;
		m_nType16 = nType;
		m_Head.m_pPrev = m_Head.m_pNext = &m_Head;
	}

	uint32			m_n00;                              // +0x00
	uint32			m_n04;                              // +0x04
	uint32			m_n08;                              // +0x08
	Link1003b1c0	m_Head;                             // +0x0c
	uint16			m_w14;                              // +0x14
	uint8			m_nType16;                          // +0x16
	uint8			m_b17;                              // +0x17
};

C1003b1c0 g_1008e6d0(1);                                // 0x1008e6d0  (new global)
C1003b1c0 g_1008e6e8(2);                                // 0x1008e6e8  (new global)
