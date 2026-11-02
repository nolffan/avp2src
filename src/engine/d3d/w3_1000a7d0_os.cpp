// *********************************************************************** //
//
//  w3_1000a7d0_os.cpp -- written for the d3d.ren link.
//
//  0x1000a7d0: bind a texture record to its stage.  Not in w3's list, but
//  w3_10014580_os.cpp's 0x10015320 calls it (0x100153a1) and nothing
//  defined it, so writing 0x10015320 would otherwise have added an
//  unresolved external.  It is the out-of-line copy of the bind block
//  big0ae20.cpp inlines at 0x1000b493 (and big2ef34 / big61a30 /
//  big6d910 likewise), with the list removal CALLED (0x1000a6e0, ltlink.h's
//  dl_Remove, d3dneutral.cpp) instead of inlined.
//
//  Flag set /Os: the pad after the ret is `8d 76 00` then 0x90.
//
//  IDENTICAL through locals, each measured: the list head's
//  prev read once into pAfter and the link reached through pLink (one
//  `lea` of +0x1c where the member spelling made two); the stage read into
//  nStage ahead of the SetTexture call (the stage is then evaluated before
//  the device pointer, as shipped); and the scale pair read into two
//  floats and stored through a local copy of the array base (`mov
//  $0x1008e664,%edx` and one index for both stores).
//  Types and globals are spelled as big0ae20.cpp spells them.
//
// *********************************************************************** //

#include "ltbasedefs.h"


struct Link0ae20
{
	Link0ae20*  m_pPrev;                                // +0x00
	Link0ae20*  m_pNext;                                // +0x04
	void*       m_pData;                                // +0x08
};

struct DDSurface;

struct D3DDevice;

struct D3DDeviceVtbl
{
	uint8   m_pad_00[0x50];
	long (__stdcall* SetRenderState)(D3DDevice*, uint32 nState, uint32 nValue);     // +0x50
	uint8   m_pad_54[0x8c - 0x54];
	long (__stdcall* SetTexture)(D3DDevice*, uint32 nStage, DDSurface* pTex);        // +0x8c
};

struct D3DDevice
{
	D3DDeviceVtbl*  lpVtbl;
};

// The per-stage texture record (big0ae20.cpp's layout).
struct Tex0ae20
{
	void*       m_pVtbl00;                              // +0x00
	float       m_fInvW04;                              // +0x04
	float       m_fInvH08;                              // +0x08
	DDSurface*  m_pSurface0c;                           // +0x0c
	uint32      m_nBytes10;                             // +0x10
	uint16      m_nStamp14;                             // +0x14
	uint16      m_nAlphaRef16;                          // +0x16
	uint8       m_pad_18[0x1c - 0x18];
	Link0ae20   m_Link1c;                               // +0x1c
	uint8       m_pad_28[0x30 - 0x28];
	Tex0ae20*   m_pNext30;                              // +0x30
	uint8       m_pad_34[0x42 - 0x34];
	uint16      m_nStage42;                             // +0x42
	uint16      m_nDirty44;                             // +0x44
};

struct TexScale0ae20
{
	float       m_fInvW00;                              // +0x00
	float       m_fInvH04;                              // +0x04
};

struct Stats0ae20
{
	uint8       m_pad_00[0x48];
	uint32      m_nBytes48;                             // +0x48
};

extern D3DDevice*       g_pD3DDevice;                   // 0x1008d988
extern Tex0ae20*        g_a1008e684[];                  // 0x1008e684  bound per stage
extern TexScale0ae20    g_a1008e664[];                  // 0x1008e664  1/w, 1/h per stage
extern Link0ae20        g_Link1008e634;                 // 0x1008e634  the used list head
extern uint16           g_w1008bea8;                    // 0x1008bea8  frame stamp
extern Stats0ae20*      g_p1008cbcc;                    // 0x1008cbcc
extern uint32           g_n1008bd80;                    // 0x1008bd80  texture changes

// 0x1000a6e0 -- ltlink.h's dl_Remove, out of line (d3dneutral.cpp).
extern void dl_Remove_1000a6e0(Link0ae20* pLink);

extern "C" void sub_1000a7d0(Tex0ae20* pRec)
{
	Link0ae20* pAfter;
	Link0ae20* pLink;
	float      fInvW, fInvH;
	uint32     nStage;
	TexScale0ae20* pScales;

	g_a1008e684[pRec->m_nStage42] = pRec;

	if (pRec->m_nStamp14 != g_w1008bea8)
	{
		// dl_Remove, then dl_Insert at the head of the used list.
		dl_Remove_1000a6e0(&pRec->m_Link1c);

		pAfter = g_Link1008e634.m_pPrev;
		pLink  = &pRec->m_Link1c;
		pLink->m_pPrev = pAfter;
		pLink->m_pNext = pAfter->m_pNext;
		pLink->m_pNext->m_pPrev = pLink;
		pLink->m_pPrev->m_pNext = pLink;

		g_p1008cbcc->m_nBytes48 += pRec->m_nBytes10;

		pRec->m_nStamp14 = g_w1008bea8;
	}

	// vtable +0x8c, the stage narrowed with `movzbl %al,%eax`.
	nStage = pRec->m_nStage42;
	g_pD3DDevice->lpVtbl->SetTexture(g_pD3DDevice, (uint8)nStage,
	                                 pRec->m_pSurface0c);

	if (pRec->m_nAlphaRef16)
	{
		// vtable +0x50, state 0x18 = D3DRENDERSTATE_ALPHAREF.
		g_pD3DDevice->lpVtbl->SetRenderState(g_pD3DDevice, 0x18,
		                                     pRec->m_nAlphaRef16);
	}

	nStage  = pRec->m_nStage42;
	pScales = g_a1008e664;
	fInvW   = pRec->m_fInvW04;
	fInvH   = pRec->m_fInvH08;
	pScales[nStage].m_fInvW00 = fInvW;
	pScales[nStage].m_fInvH04 = fInvH;

	g_n1008bd80++;
}
