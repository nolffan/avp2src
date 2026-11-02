// *********************************************************************** //
//
//  MODULE:   D3DBATCHA_O2 (.CPP)
//
//  PURPOSE:  Split out of d3dbatcha.cpp: the functions from that batch that
//            need the OTHER optimisation flag.  d3dbatcha.cpp is /Os (it is
//            in build/engine.sh's D3D_OS_TUS); a file in no list builds at
//            /O2, which is what these are.  d3d.ren is a mixed build and our
//            file boundaries are invented, so the split is measured per
//            function: each body here is byte-identical at /O2 and not at
//            /Os.
//
// *********************************************************************** //

#include "ltbasedefs.h"


// The Direct3D 7 device at 0x1008d988 -- only SetRenderState (+0x50) is
// reached from here.  Same layout as d3dbatcha.cpp's.
struct D3DDevice;

struct D3DDeviceVtbl
{
	void* _pad_00[20];                                  // +0x00 .. +0x4c
	long (__stdcall* SetRenderState)(D3DDevice*, uint32, uint32);        // +0x50
};

struct D3DDevice
{
	D3DDeviceVtbl* lpVtbl;
};

extern D3DDevice* g_pD3DDevice;                         // 0x1008d988

extern uint32 g_n1008ee00;                              // 0x1008ee00


// ======================================================================= //
//  0x10044f74   121 bytes (listing says 128; and note the FOUR BYTES
//               BEFORE it, 0x10044f70 `mov 0x4(%esp),%eax`, are a separate
//               entry that falls through and are not part of this body)
//               4 callers
//
//  DECODED.  __thiscall, three arguments, `ret $0xc`:
//
//      mov %ebx,(%esp)                    <- icl saves %ebx with a store
//      mov %ecx,%ebx
//      mov 0x8(%ebp),%eax / mov 0xc(%ebp),%edx
//      mov %eax,0x434(%ebx)               <- argument 0
//      mov %edx,0x42c(%ebx)               <- argument 1
//      movl $0x1,0x304(%ebx)
//      lea -0x4(%ebp),%eax / push %eax / call 0x10047b60
//      mov 0x10(%ebp),%eax / test %eax,%eax / je +
//        call 0x10008e10
//      + mov -0x4(%ebp),%eax / call 0x10008da0
//      (device, 8, 0x1008ee00) -> call *0x50(%eax)
//
//  WHAT PINS IT: the store ORDER is +0x434, +0x42c, +0x304 -- higher offset
//  first, so it is source order and not declaration order.  0x10047b60 is
//  handed the address of a four-byte local which 0x10008da0 then consumes by
//  value, so the local is an out-parameter threaded between two members of
//  the same class (all three callees take `this` in %ecx and are reached
//  with %ebx moved into %ecx).  The third argument is a flag and is only
//  ever tested.  The tail sets render state 8 (D3DRENDERSTATE_FILLMODE) from
//  the global at 0x1008ee00.
// ======================================================================= //

class C10044f74
{
public:
	void sub_10044f74(uint32 nA, uint32 nB, uint32 bFlag);

	// same class, defined elsewhere in the module -- declared and not
	// defined so that /Ob1 has nothing it could fold and so that this file
	// claims no bytes at their addresses.
	void sub_10047b60(uint32* pnOut);                   // 0x10047b60
	void sub_10008e10();                                // 0x10008e10
	void sub_10008da0(uint32 nValue);                   // 0x10008da0

	uint8  m_Unknown000[0x304];                         // +0x000
	uint32 m_n304;                                      // +0x304
	uint8  m_Unknown308[0x42c - 0x308];                 // +0x308
	uint32 m_n42c;                                      // +0x42c
	uint32 m_nUnknown430;                               // +0x430
	uint32 m_n434;                                      // +0x434
};

void C10044f74::sub_10044f74(uint32 nA, uint32 nB, uint32 bFlag)
{
	uint32 nLocal;

	m_n434 = nA;
	m_n42c = nB;
	m_n304 = 1;

	sub_10047b60(&nLocal);

	if (bFlag)
		sub_10008e10();

	sub_10008da0(nLocal);

	g_pD3DDevice->lpVtbl->SetRenderState(g_pD3DDevice, 8, g_n1008ee00);
}
