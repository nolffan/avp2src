// *********************************************************************** //
//
//  w16_10064720.cpp -- d3d.ren, written for the link.  /O2.
//
//  0x10064720 -- 158 bytes (`ret`, then `mov %esi,%esi` to 0x100647c0: the
//  /O2 multi-byte-nop padding).  Called from 0x100500a0's flush (w7).
//
//  The DrawSprites pass: behind g_CV_DrawSprites (0x100893d4) it takes the
//  sprite list at +0x1c4 of the 0x1007b8d0 singleton and, if it is not
//  empty, draws it through the 0x1007b8e0 registrar with 0x10061a30 as the
//  callback, with Z testing off for the duration (SetRenderState(ZENABLE,
//  0) ... SetRenderState(ZENABLE, g_1008d984)).  Then the same trailing
//  load-and-store-back of *g_1008beb0->m_p0c that closes 0x100646d0
//  (d3dr2c.cpp) -- written the same way, as a copy between two union
//  members.
//
//  Pinned by the bytes:
//    * `test %eax,%eax ; jbe` on +0x1c8 -- an unsigned count, `if (n > 0)`;
//    * the second SetRenderState reloads g_pD3DDevice and reads 0x1008d984
//      after the registrar call: two separate calls, no locals.
//
// *********************************************************************** //

#include "ltbasedefs.h"

struct D3DDevice7;

struct D3DDevice7Vtbl
{
	uint8	m_pad_00[0x50];
	long (__stdcall* SetRenderState)(D3DDevice7*, uint32 dwState,
	                                 uint32 dwValue);       // +0x50
};

struct D3DDevice7
{
	D3DDevice7Vtbl*	lpVtbl;
};

#define D3DRENDERSTATE_ZENABLE                  7

extern D3DDevice7*	g_pD3DDevice;                           // 0x1008d988
extern uint32		g_nZEnable1008d984;                     // 0x1008d984
extern uint32       g_100893d4;                             // 0x100893d4  DrawSprites

struct Ctx1007b8e0;
struct Elem1007b8e0;
typedef void (*Fn1007b8e0)(Ctx1007b8e0* pContext, Elem1007b8e0* pElem);

class C1007b8e0
{
public:
	void sub_1007b8e0(Ctx1007b8e0* pContext, Fn1007b8e0 pfn);   // 0x1007b8e0

	Elem1007b8e0** m_pElements;                         // +0x00
	uint32         m_nCount;                            // +0x04
};

struct C1007b8d0Thing
{
	uint8       m_pad_000[0x1c4];
	C1007b8e0   m_1c4;                                  // +0x1c4
};

C1007b8d0Thing *sub_1007b8d0();                         // 0x1007b8d0

struct C1008b880;
extern C1008b880    g_1008b880;                         // 0x1008b880

struct Ctx61a30;
class  Obj61a30;
void sub_10061a30(Ctx61a30* pCtx, Obj61a30* pObj);      // 0x10061a30

struct C1008beb0Node
{
	union
	{
		C1008beb0Node  *m_pA;                           // +0x00
		C1008beb0Node  *m_pB;                           // +0x00
	};
};

struct C1008beb0Thing
{
	uint8           m_pad_00[0x0c];
	C1008beb0Node  *m_p0c;                              // +0x0c
};

extern C1008beb0Thing *g_1008beb0;                      // 0x1008beb0

void sub_10064720()
{
	C1007b8e0* pList;

	if (!g_100893d4)
		return;

	pList = &sub_1007b8d0()->m_1c4;

	if (pList->m_nCount > 0)
	{
		g_pD3DDevice->lpVtbl->SetRenderState(g_pD3DDevice,
		                                     D3DRENDERSTATE_ZENABLE, 0);
		pList->sub_1007b8e0((Ctx1007b8e0*)&g_1008b880,
		                    (Fn1007b8e0)sub_10061a30);
		g_pD3DDevice->lpVtbl->SetRenderState(g_pD3DDevice,
		                                     D3DRENDERSTATE_ZENABLE,
		                                     g_nZEnable1008d984);
	}

	g_1008beb0->m_p0c->m_pA = g_1008beb0->m_p0c->m_pB;
}
