// *********************************************************************** //
//
//  MODULE:   W6_10035AB0_OS (.CPP)  --  the payload-list node's deleting
//                                        destructor, 0x10035ab0 (31 bytes)
//
//  Written for the d3d-link.  The node is d3dqip3.cpp's Node10036420
//  (d3dr5c.cpp spells the same class Node10036380): three dwords built by
//  0x10036254 -- +0x00 the owned payload, +0x04 prev, +0x08 next.
//
//  0x10035ab0 is the node's SCALAR DELETING DESTRUCTOR: the destructor
//  body (0x100362a4, d3dqip3.cpp, `this` in %ecx), then operator delete
//  (0x10080479) when bit 0 of the flag is set.  It is referenced by NAME
//  from d3dqip3.cpp, d3dr5a*.cpp and d3dr5c*.cpp (every caller passes 1),
//  so it is written as the ordinary member those references name.
//
//  WHY THIS FILE AND /Os.  The shipped function pads with 0x90 (/Os) and
//  pushes its outgoing argument (`push %edi ; push %ebx`); built /O2 /Qip
//  it comes out 48 bytes with a mov-based argument area.  It belonged to
//  the same /Qip TU as 0x100362a4 (it calls that register entry, four
//  bytes past the vestigial stack entry at 0x100362a0), but compiled into
//  d3dqip3.cpp /Qip inlines it into 0x10036420, which the shipped code
//  does not (measured: 0x10036420 stops matching).  Here the call goes to
//  the symbol sub_100362a4, which IS the register entry's address; the
//  instruction is the same `mov %ebx,%ecx ; call rel32`.
// *********************************************************************** //

#include "ltbasedefs.h"


// operator delete, __cdecl (void*).
void sub_10080479(void* p);                                 // 0x10080479

struct Elem10035bb4V;

struct Node10036420
{
	Elem10035bb4V*	m_pKey;                                 // +0x00
	Node10036420*	m_pPrev04;                              // +0x04
	Node10036420*	m_pNext;                                // +0x08

	void	sub_100362a4();                             // 0x100362a4, d3dqip3.cpp
	void	sub_10035ab0(uint32 nFlags);                // 0x10035ab0
};


void Node10036420::sub_10035ab0(uint32 nFlags)
{
	sub_100362a4();

	if ((nFlags & 1) == 0)
		return;

	sub_10080479(this);
}
