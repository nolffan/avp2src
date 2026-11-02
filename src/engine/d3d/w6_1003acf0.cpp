// *********************************************************************** //
//
//  MODULE:   W6_1003ACF0 (.CPP)  --  the RTexture bank's atexit destructor
//
//  0x1003acf0 (51 bytes + lea-nop pad: /O2), written for the d3d-link.
//  d3dr5c.cpp's module init 0x1003ac30 constructs the object at
//  0x1008e640 by hand (vptr 0x10089510, then 0x1007fb20(&bank, 0x4c,
//  0x20) on the bank at +0x04) and registers THIS with atexit.  It is the
//  matching destructor, inlined: the derived vptr store, the bank's
//  teardown 0x1007fba0(&bank, 1), then the base vptr 0x100894c0 -- the
//  same body d3dr5a.cpp's 0x100374d0 (the virtual deleting destructor of
//  this class: vtable 0x10089510 slot 0) has, minus the delete.
//
//      83 ec 0c                     sub $0xc,%esp
//      b8 <0x1008e640> / 83 c0 04   %eax = &obj + 4   <- the member address
//      c7 05 <0x1008e640> <0x10089510>
//      c7 44 24 04 01000000 / 89 04 24 / e8 <0x1007fba0>
//      c7 05 <0x1008e640> <0x100894c0>
//      83 c4 0c / c3
//
//  IDENTICAL: the pointer is declared, THEN assigned.  Written as
//  an initialised declaration (`CBank1008e640* pObj = &g_1008e640;`) the
//  same three statements came out 10 bytes off -- the two outgoing
//  arguments stored the other way round, `mov %eax,(%esp)` before
//  `movl $0x1,0x4(%esp)` -- and so did a separate member-pointer local
//  (before or after the vptr store), the count in a local, and `(*pObj).`
//  spellings.  Tried before that and no better: the global named
//  directly or through an in-class/inline helper (the +4 then folds into
//  the immediate, 18 bytes off), byte arithmetic for the member, /Os,
//  /O2 /Qip.
//
//  The size-0 row the enumerator gave this address is wrong, not a
//  phantom: d3dr5c.cpp's `atexit(sub_1003acf0)` pushes it (0x1003acb3).
// *********************************************************************** //

#include "ltbasedefs.h"


// d3dr5a.cpp's 0x1c-byte bank member and its teardown.
struct Hash1007fba0
{
	uint32	m_v[7];
};

extern void sub_1007fba0(Hash1007fba0* pHash, uint32 nA);  // 0x1007fba0

extern void* g_vtbl_10089510[];                             // 0x10089510
extern void* g_vtbl_100894c0[];                             // 0x100894c0

// The object: d3dr5a.cpp's C100374d0 shape, a vptr and the bank.
struct CBank1008e640
{
	void**			m_pVTable;                              // +0x00
	Hash1007fba0	m_Hash04;                               // +0x04
};

extern CBank1008e640 g_1008e640;                            // 0x1008e640


void sub_1003acf0()
{
	CBank1008e640* pObj;

	pObj = &g_1008e640;
	pObj->m_pVTable = g_vtbl_10089510;
	sub_1007fba0(&pObj->m_Hash04, 1);
	pObj->m_pVTable = g_vtbl_100894c0;
}
