// *********************************************************************** //
//
//  MODULE:   W4_10023C40_OS (.CPP)  -- /Os
//
//  PURPOSE:  Functions the d3d.ren link referenced by address and nobody
//            had written: 0x10023c40 (texture-stage setup) and 0x100230d4
//            (the script tokeniser's advance).
//
// *********************************************************************** //

#include "ltbasedefs.h"


// ======================================================================= //
//  0x10023c40 -- the default single-texture blend.  With the DX6 command
//  path (UseDX6Commands) it programs stage 0 through
//  IDirect3DDevice7::SetTextureStageState (+0x94): COLOROP = MODULATE,
//  COLORARG1 = TEXTURE, COLORARG2 = DIFFUSE, ALPHAOP = MODULATE,
//  ALPHAARG1 = TEXTURE, ALPHAARG2 = DIFFUSE; without it, the legacy
//  SetRenderState (+0x50) TEXTUREMAPBLEND (0x15) = MODULATE (4).  The
//  device global is reloaded for every call.
// ======================================================================= //
struct D3DDevice7;

struct D3DDevice7Vtbl
{
	void*   m_pad_00[20];                               // +0x00 .. +0x4c
	long (__stdcall* SetRenderState)(D3DDevice7*, uint32 nState,
	                                 uint32 nValue);      // +0x50
	void*   m_pad_54[16];                               // +0x54 .. +0x90
	long (__stdcall* SetTextureStageState)(D3DDevice7*, uint32 nStage,
	                                       uint32 nState,
	                                       uint32 nValue);  // +0x94
};

struct D3DDevice7
{
	D3DDevice7Vtbl* lpVtbl;
};

extern D3DDevice7* g_pD3DDevice;                        // 0x1008d988
extern int32 g_CV_UseDX6Commands;                       // 0x100893b4   UseDX6Commands

#define D3DTSS_COLOROP                         1
#define D3DTSS_COLORARG1                       2
#define D3DTSS_COLORARG2                       3
#define D3DTSS_ALPHAOP                         4
#define D3DTSS_ALPHAARG1                       5
#define D3DTSS_ALPHAARG2                       6
#define D3DTOP_MODULATE                        4
#define D3DTA_DIFFUSE                          0
#define D3DTA_TEXTURE                          2
#define D3DRENDERSTATE_TEXTUREMAPBLEND      0x15
#define D3DTBLEND_MODULATE                     4

void sub_10023c40()
{
	if (g_CV_UseDX6Commands)
	{
		g_pD3DDevice->lpVtbl->SetTextureStageState(g_pD3DDevice, 0, D3DTSS_COLOROP,   D3DTOP_MODULATE);
		g_pD3DDevice->lpVtbl->SetTextureStageState(g_pD3DDevice, 0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
		g_pD3DDevice->lpVtbl->SetTextureStageState(g_pD3DDevice, 0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
		g_pD3DDevice->lpVtbl->SetTextureStageState(g_pD3DDevice, 0, D3DTSS_ALPHAOP,   D3DTOP_MODULATE);
		g_pD3DDevice->lpVtbl->SetTextureStageState(g_pD3DDevice, 0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
		g_pD3DDevice->lpVtbl->SetTextureStageState(g_pD3DDevice, 0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);
	}
	else
	{
		g_pD3DDevice->lpVtbl->SetRenderState(g_pD3DDevice, D3DRENDERSTATE_TEXTUREMAPBLEND, D3DTBLEND_MODULATE);
	}
}


// ======================================================================= //
//  0x100230d4 -- C10023130's advance (d3dqip3.cpp's 0x10023130 calls it
//  first): if there is text left at +0x104, run the tokeniser 0x10022e9c
//  over it, clearing the cursor when it reports the end.  __thiscall, no
//  arguments; FALSE only when there was nothing left to begin with.
//
//  NEAR at best: the tokeniser is entered at its /Qip REGISTER entry
//  0x10022e9c (%eax = the cursor, %edx = &cursor, %ecx = &+0x108, the
//  other two at 0xc/0x10(%esp) over the register holes).  Nothing has
//  written that function yet (d3dqip3.cpp holds only a stand-in for it);
//  it is declared here by its stack entry 0x10022e90, the one a C call can
//  name, and called that way.
// ======================================================================= //
// 0x100230d4 moved to w15_10022e90_osoiqip.cpp (as sub_100230d0, the
// stack entry of its /Qip pair): it calls the tokeniser through its /Qip
// register entry 0x10022e9c, which needs the two in one /Qip translation
// unit.  Byte-identical there.
