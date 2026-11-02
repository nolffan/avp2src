// *********************************************************************** //
//
//  MODULE:   W6_1003B9C4_OS (.CPP)  --  the draw-callback state bracket
//
//  0x1003d950 (811 bytes) and 0x1003b9c4 (328 bytes), written for the
//  d3d-link: the two members d3dr2b.cpp's Obj1003b964::sub_1003b964 calls
//  around a context's draw callback --
//
//      if (!pCtx->m_pfnCallback) return;
//      sub_1003d950(pCtx);                 <- save + set the device state
//      pCtx->m_pfnCallback(this, pCtx, pCtx->m_n1ac);
//      sub_1003b9c4();                     <- restore it
//
//  /Os (both pad to 4 then 0x90).  In the shipped TU 0x1003b9c4 also has
//  /Qip's vestigial stack entry (`mov 0x4(%esp),%eax` at 0x1003b9c0) and
//  0x1003b964 calls the register entry; nothing in THIS TU calls it, so
//  icl emits the body alone -- which is the shipped function from
//  0x1003b9c4 on.  0x1003d950 has no entry in front of it in the shipped
//  code either.
//
//  THE OBJECT.  +0x04..+0x24 hold the saved device values (Get*State
//  into them, Set*State back out of them), +0x28..+0x48 the values the
//  bracket set, in the same order:
//
//      render state            saved   set to  tracked
//      0x1b ALPHABLENDENABLE   +0x04   0       +0x2c
//      0x07 ZENABLE            +0x08   1       +0x30
//      0x0e ZWRITEENABLE       +0x0c   1       +0x34
//      0x13 SRCBLEND           +0x10   5       +0x38   (SRCALPHA)
//      0x14 DESTBLEND          +0x14   6       +0x3c   (INVSRCALPHA)
//      0x1c FOGENABLE          +0x24   0 if the context's +0x88 has 0x80
//      stage 0:
//      0x0c ADDRESS            +0x18   1       +0x40   (WRAP)
//      0x01 COLOROP            +0x1c   4       +0x44 = 3   (MODULATE;
//      0x04 ALPHAOP            +0x20   2       +0x48 = 2    renderer enum)
//
//  and +0x28 = the context's +0x88 & 0x40 as a boolean.  The bracket also
//  unbinds the texture current on stage g_1008d7d0.
// *********************************************************************** //

#include "ltbasedefs.h"


// IDirect3DDevice7, C-style: +0x50 SetRenderState, +0x54 GetRenderState,
// +0x8c SetTexture, +0x90 GetTextureStageState, +0x94 SetTextureStageState.
struct D3DDevice7;

struct D3DDevice7Vtbl
{
	void*	m_pad_00[20];                                   // +0x00..+0x4c
	long	(__stdcall* SetRenderState)(D3DDevice7*, uint32 nState,
	                                    uint32 nValue);     // +0x50
	long	(__stdcall* GetRenderState)(D3DDevice7*, uint32 nState,
	                                    uint32* pValue);    // +0x54
	void*	m_pad_58[13];                                   // +0x58..+0x88
	long	(__stdcall* SetTexture)(D3DDevice7*, uint32 nStage,
	                                void* pTexture);        // +0x8c
	long	(__stdcall* GetTextureStageState)(D3DDevice7*, uint32 nStage,
	                                          uint32 nState,
	                                          uint32* pValue);  // +0x90
	long	(__stdcall* SetTextureStageState)(D3DDevice7*, uint32 nStage,
	                                          uint32 nState,
	                                          uint32 nValue);   // +0x94
};

struct D3DDevice7 { D3DDevice7Vtbl* lpVtbl; };

extern D3DDevice7* g_pD3DDevice;                            // 0x1008d988

extern uint32 g_1008d7d0;                                   // 0x1008d7d0, the stage
extern void*  g_1008e684[];                                 // 0x1008e684, current RTexture per stage

#define W6_D3DRS_ZENABLE            7
#define W6_D3DRS_ZWRITEENABLE       14
#define W6_D3DRS_SRCBLEND           19
#define W6_D3DRS_DESTBLEND          20
#define W6_D3DRS_ALPHABLENDENABLE   27
#define W6_D3DRS_FOGENABLE          28
#define W6_D3DTSS_COLOROP           1
#define W6_D3DTSS_ALPHAOP           4
#define W6_D3DTSS_ADDRESS           12

// The context (d3dr2b.cpp's Ctx1003b964): only +0x88 is read here.
struct Ctx1003b964
{
	uint8	m_pad_000[0x88];                                // +0x000
	uint32	m_nFlags88;                                     // +0x088
};

#define CTX_FLAG_40                 0x40        // -> +0x28
#define CTX_FLAG_NOFOG              0x80        // fog off for the callback

struct Obj1003b964
{
	uint32	m_n00;                                          // +0x00
	uint32	m_nAlphaBlend04;                                // +0x04
	uint32	m_nZEnable08;                                   // +0x08
	uint32	m_nZWrite0c;                                    // +0x0c
	uint32	m_nSrcBlend10;                                  // +0x10
	uint32	m_nDestBlend14;                                 // +0x14
	uint32	m_nAddress18;                                   // +0x18
	uint32	m_nColorOp1c;                                   // +0x1c
	uint32	m_nAlphaOp20;                                   // +0x20
	uint32	m_nFog24;                                       // +0x24
	uint32	m_b28;                                          // +0x28
	uint32	m_nAlphaBlend2c;                                // +0x2c
	uint32	m_nZEnable30;                                   // +0x30
	uint32	m_nZWrite34;                                    // +0x34
	uint32	m_nSrcBlend38;                                  // +0x38
	uint32	m_nDestBlend3c;                                 // +0x3c
	uint32	m_nAddress40;                                   // +0x40
	uint32	m_nColorOp44;                                   // +0x44
	uint32	m_nAlphaOp48;                                   // +0x48

	void	sub_1003d950(Ctx1003b964* pCtx);
	void	sub_1003b9c4();
};


// ======================================================================= //
//  0x1003d950 -- save the device state and set the callback's.
// ======================================================================= //
void Obj1003b964::sub_1003d950(Ctx1003b964* pCtx)
{
	uint32 nStage;

	g_pD3DDevice->lpVtbl->GetRenderState(g_pD3DDevice, W6_D3DRS_ALPHABLENDENABLE, &m_nAlphaBlend04);
	g_pD3DDevice->lpVtbl->SetRenderState(g_pD3DDevice, W6_D3DRS_ALPHABLENDENABLE, 0);
	m_nAlphaBlend2c = 0;

	g_pD3DDevice->lpVtbl->GetRenderState(g_pD3DDevice, W6_D3DRS_ZENABLE, &m_nZEnable08);
	g_pD3DDevice->lpVtbl->SetRenderState(g_pD3DDevice, W6_D3DRS_ZENABLE, 1);
	m_nZEnable30 = 1;

	g_pD3DDevice->lpVtbl->GetRenderState(g_pD3DDevice, W6_D3DRS_ZWRITEENABLE, &m_nZWrite0c);
	g_pD3DDevice->lpVtbl->SetRenderState(g_pD3DDevice, W6_D3DRS_ZWRITEENABLE, 1);
	m_nZWrite34 = 1;

	g_pD3DDevice->lpVtbl->GetRenderState(g_pD3DDevice, W6_D3DRS_SRCBLEND, &m_nSrcBlend10);
	g_pD3DDevice->lpVtbl->SetRenderState(g_pD3DDevice, W6_D3DRS_SRCBLEND, 5);
	m_nSrcBlend38 = 5;

	g_pD3DDevice->lpVtbl->GetRenderState(g_pD3DDevice, W6_D3DRS_DESTBLEND, &m_nDestBlend14);
	g_pD3DDevice->lpVtbl->SetRenderState(g_pD3DDevice, W6_D3DRS_DESTBLEND, 6);
	m_nDestBlend3c = 6;

	g_pD3DDevice->lpVtbl->GetRenderState(g_pD3DDevice, W6_D3DRS_FOGENABLE, &m_nFog24);
	if ((pCtx->m_nFlags88 & CTX_FLAG_NOFOG) && m_nFog24)
		g_pD3DDevice->lpVtbl->SetRenderState(g_pD3DDevice, W6_D3DRS_FOGENABLE, 0);

	g_pD3DDevice->lpVtbl->GetTextureStageState(g_pD3DDevice, 0, W6_D3DTSS_ADDRESS, &m_nAddress18);
	g_pD3DDevice->lpVtbl->SetTextureStageState(g_pD3DDevice, 0, W6_D3DTSS_ADDRESS, 1);
	m_nAddress40 = 1;

	g_pD3DDevice->lpVtbl->GetTextureStageState(g_pD3DDevice, 0, W6_D3DTSS_COLOROP, &m_nColorOp1c);
	g_pD3DDevice->lpVtbl->SetTextureStageState(g_pD3DDevice, 0, W6_D3DTSS_COLOROP, 4);
	m_nColorOp44 = 3;

	g_pD3DDevice->lpVtbl->GetTextureStageState(g_pD3DDevice, 0, W6_D3DTSS_ALPHAOP, &m_nAlphaOp20);
	g_pD3DDevice->lpVtbl->SetTextureStageState(g_pD3DDevice, 0, W6_D3DTSS_ALPHAOP, 2);
	m_nAlphaOp48 = 2;

	m_b28 = (pCtx->m_nFlags88 & CTX_FLAG_40) != 0;

	nStage = g_1008d7d0;
	if (g_1008e684[nStage])
	{
		g_pD3DDevice->lpVtbl->SetTexture(g_pD3DDevice, nStage, LTNULL);
		g_1008e684[nStage] = LTNULL;
	}
}


// ======================================================================= //
//  0x1003b9c4 -- put the saved device state back.
// ======================================================================= //
void Obj1003b964::sub_1003b9c4()
{
	g_pD3DDevice->lpVtbl->SetRenderState(g_pD3DDevice, W6_D3DRS_ALPHABLENDENABLE, m_nAlphaBlend04);
	g_pD3DDevice->lpVtbl->SetRenderState(g_pD3DDevice, W6_D3DRS_ZENABLE, m_nZEnable08);
	g_pD3DDevice->lpVtbl->SetRenderState(g_pD3DDevice, W6_D3DRS_ZWRITEENABLE, m_nZWrite0c);
	g_pD3DDevice->lpVtbl->SetRenderState(g_pD3DDevice, W6_D3DRS_SRCBLEND, m_nSrcBlend10);
	g_pD3DDevice->lpVtbl->SetRenderState(g_pD3DDevice, W6_D3DRS_DESTBLEND, m_nDestBlend14);
	g_pD3DDevice->lpVtbl->SetRenderState(g_pD3DDevice, W6_D3DRS_FOGENABLE, m_nFog24);

	g_pD3DDevice->lpVtbl->SetTextureStageState(g_pD3DDevice, 0, W6_D3DTSS_ADDRESS, m_nAddress18);
	g_pD3DDevice->lpVtbl->SetTextureStageState(g_pD3DDevice, 0, W6_D3DTSS_COLOROP, m_nColorOp1c);
	g_pD3DDevice->lpVtbl->SetTextureStageState(g_pD3DDevice, 0, W6_D3DTSS_ALPHAOP, m_nAlphaOp20);
}
