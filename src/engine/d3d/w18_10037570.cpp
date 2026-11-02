// *********************************************************************** //
//
//  MODULE:   W18_10037570 (.CPP)  -- /O2
//
//  PURPOSE:  0x10037570 and 0x10037590, slots +0x04 and +0x08 of the vtable
//            at 0x10089510 (slot +0x00 0x100374d0, +0x0c 0x100375b0).  The
//            object is d3dr2c.cpp's node bank C10037520Bank, whose +0x00 is
//            this vptr: slot +0x04 is Alloc -- a straight forward to
//            0x10037520 (pop the pool, run the node initialiser) -- and
//            slot +0x08 is Free(pNode), a straight forward to 0x10037550.
//            Reached only through the table; written for the d3d-link.
//
//  FLAGS: both pad with multi-byte nops to 16 (`8d b6 ..`, `8d bc 27 ..`,
//  `8d 74 26 00`), which is /O2.
//
// *********************************************************************** //

#include "ltbasedefs.h"

struct C1003abe4;

class C10037520Bank
{
public:
	C1003abe4  *sub_10037520();                         // d3dr2c.cpp
	void        sub_10037550(C1003abe4 *pNode);         // d3dr2c.cpp

	C1003abe4  *sub_10037570();                         // slot +0x04
	void        sub_10037590(C1003abe4 *pNode);         // slot +0x08
};

// 0x10037570 -- 19 bytes.  `this` rounded through %eax, bare `ret`.
C1003abe4 *C10037520Bank::sub_10037570()
{
	return sub_10037520();
}

// 0x10037590 -- 26 bytes.  One dword in, forwarded, `ret $0x4`.
void C10037520Bank::sub_10037590(C1003abe4 *pNode)
{
	sub_10037550(pNode);
}
