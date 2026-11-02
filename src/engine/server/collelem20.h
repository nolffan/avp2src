// *********************************************************************** //
//
//  MODULE:   COLLELEM20 (.H)
//
//  PURPOSE:  Two of collection.cpp's 0x20 collection elements -- the one
//            with an assignment operator of its own (CCollElem20A) and the
//            one with a constructor (CCollElem20F). Moved out of
//            collection.cpp, unchanged, so that modelnew.cpp's piece data
//            record can hold its two arrays as the SAME types: the shipped
//            image has one CMoArray instance per element type, and the
//            stand-ins modelnew.cpp used to declare for them (two opaque
//            0x20 PODs) compiled a second, wrongly shaped copy of every
//            virtual. Nothing here emits code by being included.
//
// *********************************************************************** //

#ifndef __COLLELEM20_H__
#define __COLLELEM20_H__

#include "ltbasetypes.h"

// And a fourth 0x20 element: no constructor (its BaseNew is the same
// 22-byte no-loop 0x1001b8fa as CCollElem20's) but an assignment operator
// of its own at 0x10018679, so Insert2 and Remove2 call out for every
// element move instead of copying eight dwords in line.
//
// 0x10018679 (46 bytes) is that assignment, and what it copies names the
// type rather than leaving it opaque. It is one three-trip loop that walks
// TWO arrays at once: a uint16 stepping by two from the object's base
// (0x00, 0x02, 0x04) and an eight-byte record stepping by eight from
// +0x08 (0x08, 0x10, 0x18). So the element is `uint16 [3]` followed by a
// two-byte hole and an eight-byte record `[3]`, which is exactly 0x20
// bytes, and the hole at 0x06 is the only part of the element the
// assignment never touches. The record's own copy is two dword moves,
// expanded inline.
//
// Void, like CCollElem1CV's: the shipped function leaves `this` nowhere
// (eax ends holding the walking pointer) and returns `ret $0x4`.
class CCollInner08
{
public:
	uint32 m_a, m_b;
};

// DEFINED IN CLASS, and that is load-bearing rather than cosmetic. Both
// engine builds are /Ob1, so the ONLY functions VC6 will ever expand are the
// ones marked `inline` or written inside the class body; an element operator=
// defined out of line can never be expanded, at any /O level. server.dll is
// /O1 (= /Os) and declines the expansion anyway, so it cannot tell the two
// spellings apart — but lithtech.exe is /O2 (= /Ot) and takes it, and the
// shipped /O2 `Append` for this element type (0x449030) carries this loop
// EXPANDED, walking a uint16 by two and an eight-byte record by eight, three
// trips, with no call to the out-of-line copy at 0x449a10 anywhere in it.
// Out of line here, ours calls; in class, it can be expanded. The out-of-line
// COMDAT is still emitted for the sites VC6 declines, which is what keeps
// 0x10018679 / 0x449a10 matched.
class CCollElem20A
{
public:
	void operator=(const CCollElem20A& other)
	{
		int i;

		for (i = 0; i < 3; i++)
		{
			m_Words[i] = other.m_Words[i];
			m_Recs[i]  = other.m_Recs[i];
		}
	}

	uint16       m_Words[3];
	CCollInner08 m_Recs[3];
};
// A second element type of the same size, with a constructor: BaseNew for
// 0x20 exists both without a loop (0x1001b8fa) and with one (0x1001b84c,
// 0x1001ba5b, two different element constructors), so 0x20 is at least
// three element types.
// The two constructors the 0x20 allocators call, and so the two 0x20
// element types that carry one. 0x10016492 (41 bytes) is a dword, a
// sixteen-bit field (`andw $0x0`, which is the only thing in the file
// that pins a member at two bytes) and six floats, all zeroed in layout
// order.
class CCollElem20F
{
public:
	CCollElem20F();

	uint32 m_a;
	uint16 m_b;
	float  m_c, m_d, m_e, m_f, m_g, m_h;
};

#endif  // __COLLELEM20_H__
