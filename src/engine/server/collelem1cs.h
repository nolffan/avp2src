// *********************************************************************** //
//
//  MODULE:   COLLELEM1CS (.H)
//
//  PURPOSE:  The 0x1c collection element collection.cpp's two 0x1c arrays
//            hold, and the two sub-objects it is built from.  Moved out of
//            collection.cpp, unchanged, so that a lithtech-only translation
//            unit can name the element's assignment operator (see
//            src/engine/lithtech/collelem.cpp, 0x0044b4d0).  Nothing here
//            emits code by being included.
//
// *********************************************************************** //

#ifndef __COLLELEM1CS_H__
#define __COLLELEM1CS_H__

#include "ltbasetypes.h"

// A second 0x1c element, and the one that explains the two assignment
// shapes the 0x1c arrays show. It is three plain dwords followed by a
// sixteen-byte sub-object with an assignment operator of its own
// (0x10038e1f, four dword moves returning `this`; its neighbours at
// 0x10038e3e negate three floats, so it lives in the math TU). The
// element's OWN assignment is therefore the implicit memberwise one:
// three `movsl` for the leading block, then the sub-object's operator=.
// Emitted out of line that inner operator= is expanded (0x100195b8, 44
// bytes) because the enclosing COMDAT is small; expanded into a large
// caller the inner one is left as a call (0x1001a6dd, 0x1001a82f).
class CCollInner10
{
public:
	CCollInner10& operator=(const CCollInner10& other)
	{
		m_a = other.m_a;
		m_b = other.m_b;
		m_c = other.m_c;
		m_d = other.m_d;
		return *this;
	}

	uint32 m_a, m_b, m_c, m_d;
};

class CCollInner0C
{
public:
	uint32 m_x, m_y, m_z;
};

// THIS is the 0x1c element the two 0x1c arrays in collection.cpp hold; the
// opaque CCollElem1C there is only the stand-in the first pass built them
// on, and the /O2 image says so in two places at once.
//
//   * 0x447980 is the shipped /O2 `GenGetNext` for a 0x1c element, and its
//     element copy is EXPANDED memberwise — three dword moves for the
//     twelve-byte head, then four for the sub-object — so the element's copy
//     constructor is the IMPLICIT one. A declared-but-undefined copy
//     constructor (which is all DECLARE_COLLELEM can give) is never an
//     expansion candidate, so the stand-in can only ever call.
//   * 0x448540, the /O2 `Append` for the DefaultCache 0x1c array, expands
//     THIS operator= at its first copy loop and calls the out-of-line copy
//     (0x44b4d0) at the other two sites — the per-site inline budget.
//     Expanding it at all requires it to be an expansion candidate, and both
//     builds are /Ob1, so in class is the only spelling that makes it one.
//
// /O1 declines both expansions in everything we build -- but NOT in the
// shipped image: 0x1001a6dd expands this operator= at three of its five
// sites at /O1. See the budget note at the head of collection.cpp; it is the
// one thing left in that function.
// server.dll's declined sites still emit the out-of-line COMDAT, so
// 0x100195b8 stays matched.  Our /O2 build expands every site (see the copy
// constructor), so lithtech.exe's copy, 0x44b4d0, is emitted by taking the
// operator's address in src/engine/lithtech/collelem.cpp.
//
// The empty default constructor is not decoration either: BaseNew for this
// element is 33 bytes (0x44c7a0), eleven more than the 22-byte no-loop shape
// (0x44ca90) a POD gets, and those eleven bytes are the placement-new loop an
// element with a user-declared constructor keeps even when the constructor
// itself expands to nothing.
class CCollElem1CS
{
public:
	CCollElem1CS() {}

	// A USER-DECLARED COPY CONSTRUCTOR, and it is load-bearing. Without one
	// the compiler synthesises the implicit copy, which is a flat block move
	// -- our GenGetNext/GenGetAt emitted `rep movsl` of seven dwords. The
	// shipped bytes do not: lithtech.exe copies MEMBERWISE (three dword
	// pairs through one register pair, then four through another) and
	// server.dll CALLS the copy out of line at 0x100195b8. Both are the
	// signature of a declared constructor whose body assigns its members,
	// exactly like the operator= below. /Os declines the expansion, so
	// server.dll keeps its call and cannot regress.
	CCollElem1CS(const CCollElem1CS& other)
	{
		m_head = other.m_head;
		m_sub = other.m_sub;
	}

	// 0x100195b8 (44 bytes) / 0x44b4d0 (57): the twelve-byte head is one
	// `movsl` triple and the sub-object's own operator= expands to its four
	// dword moves.
	CCollElem1CS& operator=(const CCollElem1CS& other)
	{
		m_head = other.m_head;
		m_sub = other.m_sub;

		return *this;
	}

	CCollInner0C m_head;
	CCollInner10 m_sub;
};

#endif  // __COLLELEM1CS_H__
