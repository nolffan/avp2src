// *********************************************************************** //
//
//  MODULE:   W26_100371E0_OSOI (.CPP)  --  FlipScreen and the dirty-rect blit
//
//  0x100371e0 (FlipScreen, RenderStruct slot +0xbc, 651 bytes) and
//  0x1003b240 (its flag-4 path, 339 bytes) of the retail d3d.ren, written for
//  the d3d-link.
//
//  Flag set /Os /Oi-: both pad with 0x90 (0x1003746b, 0x1003b393) and push
//  their callee-saved registers after the `sub` (the /Os frame), and
//  FlipScreen CALLS memset (0x100809a0) twice where the /Os default would
//  expand it.
//
//  FLIPSCREEN (0x100371e0).  Nothing without a primary (0x1008bf6c).
//    * flag 4: blit only the dirty rectangles (0x1003b240) and return --
//      without the dirty-list reset the other paths end with.
//    * a windowed primary (0x1008bf70): find the client area's screen
//      origin (GetClientRect + ClientToScreen on 0x1008cbc0 when windowed,
//      else the desktop window's GetWindowRect), make the clipper the first
//      time (IDirectDraw7 +0x10 CreateClipper into 0x1008d97c, clipper
//      +0x20 SetHWnd, surface +0x70 SetClipper), and Blt the primary onto
//      the windowed primary, DDBLT_WAIT.
//    * else flag 2: Blt the back buffer (0x1008bf68) onto the primary;
//      otherwise primary->Flip(NULL, DDFLIP_WAIT).
//    * then 0x1003b3a0 empties the dirty-rect list.
//
//  THE RECTANGLE COMPUTED TWICE (d3dblit.cpp left this function a decode
//  over it): the bytes build the destination as
//  {x, y, x + W*0x1008cb78, y + H*0x1008cb74}, zero-and-size a DDBLTFX,
//  then build it again as {x, y, x + W, y + H} and zero-and-size the
//  DDBLTFX again (memset + dwSize both times: 0x10037300/0x10037331 and
//  0x10037346/0x1003735a).  There is no control flow between the two;
//  the source says it twice, and so does this.  0x1008cb78/0x1008cb74 are
//  the screen divisors Init sets to 1 (w26_10020fd0_osoiqip.cpp), so the
//  two agree at run time.
//
//  0x1003B240.  Blit each rectangle of the dirty list (count 0x1008e730,
//  LTRect[100] at 0x1008e734) from the back buffer: onto the windowed
//  primary offset by the client area's screen position (both corners of
//  the client rect through ClientToScreen), or onto the primary in place;
//  then empty the list.  The windowed arm opens with a second, dead
//  `je` on the same flags (0x1003b264) that skips the arm's body AND its
//  list reset: the pointer is tested twice, nested, in the source.
//
// *********************************************************************** //

#include "ltbasedefs.h"
#include <string.h>


struct W26Rect
{
	long    left;                                       // +0x00
	long    top;                                        // +0x04
	long    right;                                      // +0x08
	long    bottom;                                     // +0x0c
};

struct W26Point
{
	long    x;                                          // +0x00
	long    y;                                          // +0x04
};

struct DDBltFx26
{
	uint32  dwSize;                                     // +0x00
	uint32  m_Reserved04[0x18];                         // +0x04..+0x63
};                                                      // 0x64

struct DDClipper
{
	virtual long __stdcall Slot00() = 0;  virtual long __stdcall Slot04() = 0;
	virtual long __stdcall Slot08() = 0;  virtual long __stdcall Slot0c() = 0;
	virtual long __stdcall Slot10() = 0;  virtual long __stdcall Slot14() = 0;
	virtual long __stdcall Slot18() = 0;  virtual long __stdcall Slot1c() = 0;
	virtual long __stdcall SetHWnd(uint32 dwFlags, void* hWnd) = 0;            // +0x20
};

struct DDSurface
{
	virtual long __stdcall Slot00() = 0;  virtual long __stdcall Slot04() = 0;
	virtual long __stdcall Slot08() = 0;  virtual long __stdcall Slot0c() = 0;
	virtual long __stdcall Slot10() = 0;
	virtual long __stdcall Blt(void* pDestRect, DDSurface* pSrc, void* pSrcRect,
	                           uint32 dwFlags, DDBltFx26* pFx) = 0;            // +0x14
	virtual long __stdcall Slot18() = 0;  virtual long __stdcall Slot1c() = 0;
	virtual long __stdcall Slot20() = 0;  virtual long __stdcall Slot24() = 0;
	virtual long __stdcall Slot28() = 0;
	virtual long __stdcall Flip(DDSurface* pOverride, uint32 dwFlags) = 0;     // +0x2c
	virtual long __stdcall Slot30() = 0;  virtual long __stdcall Slot34() = 0;
	virtual long __stdcall Slot38() = 0;  virtual long __stdcall Slot3c() = 0;
	virtual long __stdcall Slot40() = 0;  virtual long __stdcall Slot44() = 0;
	virtual long __stdcall Slot48() = 0;  virtual long __stdcall Slot4c() = 0;
	virtual long __stdcall Slot50() = 0;  virtual long __stdcall Slot54() = 0;
	virtual long __stdcall Slot58() = 0;  virtual long __stdcall Slot5c() = 0;
	virtual long __stdcall Slot60() = 0;  virtual long __stdcall Slot64() = 0;
	virtual long __stdcall Slot68() = 0;  virtual long __stdcall Slot6c() = 0;
	virtual long __stdcall SetClipper(DDClipper* pClipper) = 0;                // +0x70
};

struct DDraw
{
	virtual long __stdcall Slot00() = 0;  virtual long __stdcall Slot04() = 0;
	virtual long __stdcall Slot08() = 0;  virtual long __stdcall Slot0c() = 0;
	virtual long __stdcall CreateClipper(uint32 dwFlags, DDClipper** ppClipper,
	                                     void* pUnkOuter) = 0;                 // +0x10
};

extern "C"
{
	__declspec(dllimport) int   __stdcall GetClientRect(void* hWnd, W26Rect* pRect);   // *0x10088100
	__declspec(dllimport) int   __stdcall ClientToScreen(void* hWnd, W26Point* pPt);   // *0x10088104
	__declspec(dllimport) int   __stdcall GetWindowRect(void* hWnd, W26Rect* pRect);   // *0x1008810c
	__declspec(dllimport) void* __stdcall GetDesktopWindow();                          // *0x10088114
}

extern DDSurface*   g_pPrimary;                         // 0x1008bf6c
extern DDSurface*   g_pWindowedPrimary;                 // 0x1008bf70
extern DDSurface*   g_pOffscreen;                       // 0x1008bf68
extern DDraw*       g_pDirectDraw;                      // 0x1008bf74
extern DDClipper*   g_pClipper;                         // 0x1008d97c
extern LTBOOL       g_bWindowed;                        // 0x1008cbbc
extern void*        g_hWnd;                             // 0x1008cbc0
extern uint32       g_nWindowWidth;                     // 0x1008cbb4
extern uint32       g_nWindowHeight;                    // 0x1008cbb0
// The two scale factors are SIGNED here: declared uint32,
// icl folds each into `imul mem,reg`; as int32 it loads both into
// registers and multiplies `imul reg,reg`, as shipped -- 0x100371e0
// identical.  (w26_10020fd0_osoiqip.cpp still reaches them as uint32.)
extern int32        g_1008cb78;                         // 0x1008cb78 (see Init)
extern int32        g_1008cb74;                         // 0x1008cb74
extern uint32       g_1008e730;                         // 0x1008e730 dirty-rect count
extern LTRect       g_DirtyRects[100];                  // 0x1008e734

void    sub_1003b3a0();                                 // empty the dirty-rect list

#define DDBLT_WAIT_26           0x01000000
#define DDFLIP_WAIT_26          0x00000001


// ======================================================================= //
//  0x1003b240  339 bytes.
// ======================================================================= //
void sub_1003b240()
{
	W26Rect     rcClient;
	W26Rect     rcDest;
	uint32      i;

	if (!g_1008e730)
		return;

	if (g_pWindowedPrimary)
	{
		if (g_pWindowedPrimary)
		{
			GetClientRect(g_hWnd, &rcClient);
			ClientToScreen(g_hWnd, (W26Point*)&rcClient.left);
			ClientToScreen(g_hWnd, (W26Point*)&rcClient.right);

			for (i = 0; i < g_1008e730; i++)
			{
				rcDest.left   = g_DirtyRects[i].left   + rcClient.left;
				rcDest.top    = g_DirtyRects[i].top    + rcClient.top;
				rcDest.right  = g_DirtyRects[i].right  + rcClient.left;
				rcDest.bottom = g_DirtyRects[i].bottom + rcClient.top;

				g_pWindowedPrimary->Blt(&rcDest, g_pOffscreen, &g_DirtyRects[i],
				                        DDBLT_WAIT_26, LTNULL);
			}

			g_1008e730 = 0;
		}
	}
	else
	{
		for (i = 0; i < g_1008e730; i++)
		{
			g_pPrimary->Blt(&g_DirtyRects[i], g_pOffscreen, &g_DirtyRects[i],
			                DDBLT_WAIT_26, LTNULL);
		}

		g_1008e730 = 0;
	}
}


// ======================================================================= //
//  0x100371e0  651 bytes.  FlipScreen(dwFlags).
// ======================================================================= //
void sub_100371e0(uint32 dwFlags)
{
	W26Rect     rc;
	W26Point    pt;
	W26Rect     rcSrc;
	W26Rect     rcDest;
	DDBltFx26   fx;
	void*       hWnd;

	if (!g_pPrimary)
		return;

	if (dwFlags & 4)
	{
		sub_1003b240();
		return;
	}

	if (g_pWindowedPrimary)
	{
		if (g_bWindowed)
		{
			GetClientRect(g_hWnd, &rc);
			pt.x = rc.left;
			pt.y = rc.top;
			ClientToScreen(g_hWnd, &pt);
			hWnd = g_hWnd;
		}
		else
		{
			hWnd = GetDesktopWindow();
			GetWindowRect(hWnd, &rc);
			pt.x = rc.left;
			pt.y = rc.top;
		}

		if (!g_pClipper && g_pDirectDraw)
		{
			if (g_pDirectDraw->CreateClipper(0, &g_pClipper, LTNULL) == 0 && g_pClipper)
			{
				g_pClipper->SetHWnd(0, hWnd);
				g_pWindowedPrimary->SetClipper(g_pClipper);
			}
		}

		rcSrc.left   = 0;
		rcSrc.top    = 0;
		rcSrc.right  = g_nWindowWidth;
		rcSrc.bottom = g_nWindowHeight;

		rcDest.left   = pt.x;
		rcDest.top    = pt.y;
		rcDest.right  = pt.x + g_nWindowWidth * g_1008cb78;
		rcDest.bottom = pt.y + g_nWindowHeight * g_1008cb74;

		memset(&fx, 0, sizeof(fx));
		fx.dwSize = sizeof(fx);

		rcDest.left   = pt.x;
		rcDest.top    = pt.y;
		rcDest.right  = pt.x + g_nWindowWidth;
		rcDest.bottom = pt.y + g_nWindowHeight;

		memset(&fx, 0, sizeof(fx));
		fx.dwSize = sizeof(fx);

		g_pWindowedPrimary->Blt(&rcDest, g_pPrimary, &rcSrc, DDBLT_WAIT_26, &fx);
	}
	else if (dwFlags & 2)
	{
		g_pPrimary->Blt(LTNULL, g_pOffscreen, LTNULL, DDBLT_WAIT_26, LTNULL);
	}
	else
	{
		g_pPrimary->Flip(LTNULL, DDFLIP_WAIT_26);
	}

	sub_1003b3a0();
}
