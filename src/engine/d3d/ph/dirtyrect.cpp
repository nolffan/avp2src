// *********************************************************************** //
//  ph/dirtyrect.cpp -- Primal Hunt's d3d.ren, TU22
//  (0x10022dd1..0x10023104), /O1.
//
//  Dirty-rectangle flipping (NOLF2's render_a dirtyrect.cpp is its
//  descendant): the invalid-rectangle list, and the swap that blits it
//  from the back buffer to the screen. Ahead of it, two global objects
//  of a small list class, constructed with 1 and 2, whose only reference
//  is the pointer table after them in .data (0x1004d96c).
// *********************************************************************** //

#include <windows.h>
#include "ltbasedefs.h"
#include "ltrect.h"


// IDirectDrawSurface7, through Blt (index 5).
class CDDSurface
{
public:
	virtual long __stdcall QueryInterface(void*, void**) = 0;
	virtual uint32 __stdcall AddRef() = 0;
	virtual uint32 __stdcall Release() = 0;
	virtual long __stdcall AddAttachedSurface(CDDSurface*) = 0;
	virtual long __stdcall AddOverlayDirtyRect(RECT*) = 0;
	virtual long __stdcall Blt(RECT* pDestRect, CDDSurface* pSrc, RECT* pSrcRect,
	                           uint32 dwFlags, void* pFX) = 0;
};

#define DDBLT_WAIT		0x01000000L

extern CDDSurface*	g_pWindowedPrimary;                 // 0x1005d2ac
extern CDDSurface*	g_pFrontBuffer;                     // 0x1005d2b0
extern CDDSurface*	g_pBackBuffer;                      // 0x1005d2b4
extern HWND			g_hWnd;                             // 0x1005d430
extern uint32		g_ScreenWidth;                      // 0x1005d8cc
extern uint32		g_ScreenHeight;                     // 0x1005d9c0


// ---------------------------------------------------------------------- //
//  The list class of the two objects at 0x10069740 and 0x10069758.
// ---------------------------------------------------------------------- //
struct CListHead
{
	void		TieOff()	{ m_pNext = m_pPrev = this; }

	CListHead*	m_pNext;
	CListHead*	m_pPrev;
};

class CRenderList
{
public:
	CRenderList(uint8 nType)	{ Init(nType); }

	void		Init(uint8 nType);

	uint32		m_Data0;                                // +0x00
	uint32		m_Data4;                                // +0x04
	uint32		m_Data8;                                // +0x08
	CListHead	m_Head;                                 // +0x0c
	uint16		m_Index;                                // +0x14
	uint8		m_Type;                                 // +0x16
	uint8		m_bFlag;                                // +0x17
};

CRenderList	g_RenderList1(1);                           // 0x10069740  $E 0x10022dd1
CRenderList	g_RenderList2(2);                           // 0x10069758  $E 0x10022e03

// ---------------------------------------------------------------------- //
//  0x10022dde
// ---------------------------------------------------------------------- //
void CRenderList::Init(uint8 nType)
{
	m_Index = 0xFFFF;
	m_Type = nType;
	m_Data0 = 0;
	m_bFlag = 0;
	m_Data8 = 0;
	m_Data4 = 0;
	m_Head.TieOff();
}

CRenderList* g_RenderLists[2] = { &g_RenderList1, &g_RenderList2 };   // 0x1004d96c


#define	MAX_INVALID_RECTS		100

LTRect	g_invalidRect[MAX_INVALID_RECTS];                // 0x100697a0  $E 0x10022e10
uint32	g_invalidRectCount = 0;                         // 0x10069de0

inline bool RectangleContains(LTRect *outer, LTRect *inner)
{
	return (outer->left <= inner->left && outer->top <= inner->top && outer->right >= inner->right && outer->bottom >= inner->bottom);
}

// 0x10022fa0
LTRect RectangleCombine(LTRect *r1, LTRect *r2)
{
	LTRect r;
	r.left   = (r1->left   < r2->left  ) ? r1->left   : r2->left;
	r.top    = (r1->top    < r2->top   ) ? r1->top    : r2->top;
	r.right  = (r1->right  > r2->right ) ? r1->right  : r2->right;
	r.bottom = (r1->bottom > r2->bottom) ? r1->bottom : r2->bottom;
	return r;
}

inline uint32 RectangleArea(LTRect *r)
{
	return (r->right - r->left) * (r->bottom - r->top);
}


// ---------------------------------------------------------------------- //
//  0x10022e31
// ---------------------------------------------------------------------- //
void InvalidateRect(LTRect *pRect)
{
	if (pRect==NULL) {						// Invalidate the whole screen...
		g_invalidRect[0].left	= 0;
		g_invalidRect[0].top	= 0;
		g_invalidRect[0].right	= g_ScreenWidth;
		g_invalidRect[0].bottom	= g_ScreenHeight;
		g_invalidRectCount		= 1;
		return; }

	if (g_invalidRectCount==0) {
		g_invalidRect[g_invalidRectCount] = *pRect;
		++g_invalidRectCount; }
	else {									// Attempt to combine this invalid area with one we already have...
		uint32 i;
		LTRect comb;
		int area1, area2, areac, areaBest;
		int bestRef = -1;

		if (g_invalidRectCount>=MAX_INVALID_RECTS-1) {
			InvalidateRect(NULL); return; }

		for (i=0; i<g_invalidRectCount; i++) {
			if (RectangleContains(&g_invalidRect[i], pRect)) return; }
		for (i=0; i<g_invalidRectCount; i++) {
			comb = RectangleCombine(&g_invalidRect[i], pRect);
			area1 = RectangleArea(&g_invalidRect[i]);
			area2 = RectangleArea(pRect);
			areac = RectangleArea(&comb);
			if (areac < area1 + area2) {
				if (bestRef < 0 || area1 + area2 - areac > areaBest) {
					areaBest = area1 + area2 - areac;
					bestRef = i; } } }
		if (bestRef >= 0)
			g_invalidRect[bestRef] = RectangleCombine(&g_invalidRect[bestRef], pRect);
		else
			g_invalidRect[g_invalidRectCount++] = *pRect; }
}


// ---------------------------------------------------------------------- //
//  0x10022ffc -- windowed: blit each rectangle to the window's place on
//  the primary surface.
// ---------------------------------------------------------------------- //
void DirtyRectSwapWindowed()
{
	RECT rcClient, rcDest;
	uint32 i;

	if (!g_pWindowedPrimary)
		return;

	GetClientRect(g_hWnd, &rcClient);
	ClientToScreen(g_hWnd, (POINT*)&rcClient.left);
	ClientToScreen(g_hWnd, (POINT*)&rcClient.right);

	for (i=0; i < g_invalidRectCount; i++)
	{
		rcDest.left		= g_invalidRect[i].left + rcClient.left;
		rcDest.top		= g_invalidRect[i].top + rcClient.top;
		rcDest.right	= g_invalidRect[i].right + rcClient.left;
		rcDest.bottom	= g_invalidRect[i].bottom + rcClient.top;

		g_pWindowedPrimary->Blt(&rcDest, g_pBackBuffer, (RECT*)&g_invalidRect[i], DDBLT_WAIT, NULL);
	}

	g_invalidRectCount = 0;
}


// ---------------------------------------------------------------------- //
//  0x100230a8
// ---------------------------------------------------------------------- //
void DirtyRectSwap()
{
	uint32 i;

	if (g_invalidRectCount==0) return;	// nothing to draw!

	if (g_pWindowedPrimary)
	{
		DirtyRectSwapWindowed();
		return;
	}

	for (i=0; i < g_invalidRectCount; i++)
		g_pFrontBuffer->Blt((RECT*)&g_invalidRect[i], g_pBackBuffer, (RECT*)&g_invalidRect[i], DDBLT_WAIT, NULL);

	g_invalidRectCount = 0;
}


// ---------------------------------------------------------------------- //
//  0x100230fc
// ---------------------------------------------------------------------- //
void ClearDirtyRects()
{
	g_invalidRectCount	= 0;
}
