// *********************************************************************** //
//
//  MODULE:   W26_100262F0 (.CPP)  --  ClearScreen, RenderStruct slot +0x84
//
//  0x100262f0..0x1002647d of the retail d3d.ren (398 bytes), written for the
//  d3d-link (RenderDLLSetup stores it at +0x84; the client's
//  clientmgr.h has the slot as `void (*ClearScreen)(LTRect*, uint32)`).
//  /O2: `mov %esi,%esi` pads it to 16, the frame is /O2's
//  `mov %edi,0x8(%esp) / %esi,0x4(%esp) / %ebx,(%esp)`, and the DDBLTFX
//  clear is an inline `rep stos` (intrinsics on).  No register calls.
//
//  WHAT PINS IT:
//    * nothing happens without the device (0x1008d988).
//    * a rectangle that is NULL or exactly the whole screen (0, 0,
//      0x1008cbb4 width, 0x1008cbb0 height) becomes (0 rects, NULL);
//      anything else is copied into a local D3DRECT and passed as one.
//    * flag 1 (CLEARSCREEN_SCREEN): with a rectangle it is a D3DCLEAR_TARGET
//      (1) in the Clear; without one the back buffer (0x1008bf68) is
//      colour-filled through IDirectDrawSurface7::Blt (+0x14) with
//      DDBLT_COLORFILL|DDBLT_WAIT (0x1000400) and a zeroed DDBLTFX of
//      0x64 bytes -- so the Clear does not also touch the target.
//    * flag 2 (CLEARSCREEN_RENDER): D3DCLEAR_ZBUFFER (2), and
//      D3DCLEAR_STENCIL (4) when 0x10032990 says there is a stencil.
//    * IDirect3DDevice7::Clear (+0x28) with colour 0xff000000, z 1.0,
//      stencil 0, only if any flag was collected.
//    * then 0x1008ce90 (big2d1c0.cpp's frame counter) is zeroed and the
//      rectangle goes to the dirty-rect list (0x1003b3d0).
//
//  IDENTICAL.  The shipped code puts the CLEARSCREEN_RENDER arm (`or $2 ;
//  call 0x10032990 ; or $4`) out of line, after the epilogue.  As an `if`
//  icl keeps it on the fall-through; written as a `switch` on the flag
//  whose default arm does the work (the lever of w59_1007dd24_osoi.cpp and
//  0x1007da80), icl sinks it exactly as shipped.  Calling the stencil
//  query by a name containing one of mcpcom.exe's cold words (`Error...`)
//  does the same, but 0x10032990's real name is not known.  No effect as
//  an `if`: `!= 0` on the flag test and on the call, `== LTTRUE`, an empty
//  then-arm with the work in the else, renaming a global in the arm.
//
// *********************************************************************** //

#include "ltbasedefs.h"
#include <string.h>


struct D3DRect26
{
	long    x1;                                         // +0x00
	long    y1;                                         // +0x04
	long    x2;                                         // +0x08
	long    y2;                                         // +0x0c
};

struct DDBltFx26
{
	uint32  dwSize;                                     // +0x00
	uint32  m_Reserved04[0x18];                         // +0x04..+0x63
};                                                      // 0x64

struct D3DDevice7
{
	virtual long __stdcall Slot00() = 0;  virtual long __stdcall Slot04() = 0;
	virtual long __stdcall Slot08() = 0;  virtual long __stdcall Slot0c() = 0;
	virtual long __stdcall Slot10() = 0;  virtual long __stdcall Slot14() = 0;
	virtual long __stdcall Slot18() = 0;  virtual long __stdcall Slot1c() = 0;
	virtual long __stdcall Slot20() = 0;  virtual long __stdcall Slot24() = 0;
	virtual long __stdcall Clear(uint32 dwCount, D3DRect26* pRects, uint32 dwFlags,
	                             uint32 dwColor, float fZ, uint32 dwStencil) = 0;  // +0x28
};

struct DDSurface
{
	virtual long __stdcall Slot00() = 0;  virtual long __stdcall Slot04() = 0;
	virtual long __stdcall Slot08() = 0;  virtual long __stdcall Slot0c() = 0;
	virtual long __stdcall Slot10() = 0;
	virtual long __stdcall Blt(void* pDestRect, DDSurface* pSrc, void* pSrcRect,
	                           uint32 dwFlags, DDBltFx26* pFx) = 0;               // +0x14
};

extern D3DDevice7*      g_pD3DDevice;                   // 0x1008d988
extern DDSurface*       g_pOffscreen;                   // 0x1008bf68
extern uint32           g_nWindowWidth;                 // 0x1008cbb4
extern uint32           g_nWindowHeight;                // 0x1008cbb0
extern uint32           g_nFrameCount;                  // 0x1008ce90

LTBOOL  sub_10032990();                                 // is there a stencil buffer
void    sub_1003b3d0(LTRect* pRect);                    // d3d_AddDirtyRect

#define CLEARSCREEN_SCREEN_26       1
#define CLEARSCREEN_RENDER_26       2


// ======================================================================= //
//  0x100262f0  398 bytes.
// ======================================================================= //
void sub_100262f0(LTRect* pRect, uint32 dwFlags)
{
	D3DRect26   rect;
	DDBltFx26   fx;
	D3DRect26*  pRects;
	uint32      nRects;
	uint32      dwClear;

	if (!g_pD3DDevice)
		return;

	if (pRect == LTNULL ||
	    (pRect->left == 0 && pRect->top == 0 &&
	     pRect->right == (int)g_nWindowWidth && pRect->bottom == (int)g_nWindowHeight))
	{
		pRects = LTNULL;
		nRects = 0;
	}
	else
	{
		rect.x1 = pRect->left;
		rect.y1 = pRect->top;
		rect.x2 = pRect->right;
		rect.y2 = pRect->bottom;
		pRects = &rect;
		nRects = 1;
	}

	dwClear = 0;

	if (dwFlags & CLEARSCREEN_SCREEN_26)
	{
		if (pRects)
		{
			dwClear = 1;                                // D3DCLEAR_TARGET
		}
		else
		{
			memset(&fx, 0, sizeof(fx));
			fx.dwSize = sizeof(fx);
			g_pOffscreen->Blt(LTNULL, LTNULL, LTNULL, 0x1000400, &fx);
		}
	}

	switch (dwFlags & CLEARSCREEN_RENDER_26)
	{
		case 0:
			break;
		default:
			dwClear |= 2;                               // D3DCLEAR_ZBUFFER
			if (sub_10032990())
				dwClear |= 4;                           // D3DCLEAR_STENCIL
	}

	if (dwClear)
		g_pD3DDevice->Clear(nRects, pRects, dwClear, 0xff000000, 1.0f, 0);

	g_nFrameCount = 0;
	sub_1003b3d0(pRect);
}
