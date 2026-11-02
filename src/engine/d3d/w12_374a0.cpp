// *********************************************************************** //
//
//  w12_374a0.cpp -- 0x100374a0 (37 bytes): the scalar deleting destructor
//  in slot 0 of BOTH vtables 0x100894d0 (an interface: slots 1-4 are
//  _purecall, 0x10080905) and 0x100894f0 (C1003abe4, d3dbatchd.cpp: slots
//  1-4 are 0x1003aba0/abb0/abc0/abd0). The derived class adds nothing to
//  destroy, so both slots hold the base's.
//
//  IDENTICAL (37 bytes, /O2) -- written as an early return: with
//  `if (nFlags & 1) delete` icl keeps the delete inline; the shipped hosts
//  it after a duplicated epilogue.
//
//  Shipped: store the base vtable 0x100894d0 into this (the base
//  destructor's own vptr store), and if bit 0 of the flag argument is set,
//  operator delete (0x10080479) the object. thiscall, one argument
//  (`ret $4`).
//
//  Written as an ordinary member named for its address, storing the
//  vtable through d3ddata's spelling g_vtbl_100894d0, rather than as a
//  C++ virtual destructor -- which would make icl emit a second copy of
//  the vtable under a compiler name no table of ours points at.
//
// *********************************************************************** //

typedef unsigned long uint32;

extern void* g_vtbl_100894d0[];                         // 0x100894d0

struct C100374a0
{
	void**	m_pVTable;                                  // +0x00

	void	sub_100374a0(uint32 nFlags);
};

void C100374a0::sub_100374a0(uint32 nFlags)
{
	m_pVTable = g_vtbl_100894d0;
	if (!(nFlags & 1))
		return;
	operator delete(this);
}
