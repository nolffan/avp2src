// ----------------------------------------------------------------------- //
//
// MODULE  : m15_uiscale.h
//
// PURPOSE : The modern build's menus and HUD scaled to the screen
//           (docs/MODERN.md, M-15 the menus, M-16 the HUD).
//
//           Retail draws its interface in screen pixels: the menus are a
//           640x480 page centred on the screen (CInterfaceResMgr's
//           offsets) and the HUD is laid out from the screen's corners,
//           edges and centre (CHudElement::SetRects, CMotionDetector::
//           SetOffsets, ...) at its art's size, so on a large screen
//           both are small (the page a third of a 1920x1080 screen's
//           height).
//
//           Here each is drawn on a smaller "virtual screen" that is
//           scaled up onto the real one: a space.  While a space is
//           entered, ILTClient answers GetSurfaceDims for the screen
//           surface with the space's size, and every blit onto the screen
//           surface (Draw*, Scale*, Warp*, Transform*, FillRect) has its
//           destination scaled by the space's factor (a Draw becomes a
//           Scale).  The game's own layout code then runs unchanged -- a
//           HUD element anchored to the bottom-right corner lands in the
//           bottom-right corner, bigger -- and so does its text, which is
//           bitmap fonts drawn a glyph at a time (lithbitmapfont.cpp).
//           The wrappers are put into the engine's ILTClient table once
//           (M15_Install) and pass every call through untouched when no
//           space is entered, when the destination is not the screen, or
//           when the space's scale is 1 (at 640x480 always: retail's
//           exact output).
//
//             menu space (M-15): the 640x480 page fills the screen's
//               height (MenuScale 100), centred, the sides beside it as
//               retail leaves them; never wider than the screen.
//             HUD space (M-16): the HUD scaled by HUDScale percent of the
//               screen's height over 480 (75 by default: 1.69 at 1080
//               lines), never below retail's size; the HUD keeps its
//               anchors (corners, edges, centre) on any shape of screen.
//
//           The scale is smooth, not a whole number: the art is painted
//           16-bit bitmaps and antialiased fonts, not pixel art, and the
//           page must fill the height exactly (2.25 at 1080 lines).  The
//           renderer draws the interface in its optimized 2D mode, as
//           textured quads, filtered as its "Bilinear" variable says
//           (bilinear by default).
//
//           What is drawn in which space is chosen where it is drawn
//           (CM15Space, a scope): the folders, the message box, the
//           loading, failure and splash screens in the menu space; the
//           HUD, crosshair, objectives, weapon chooser, messages,
//           subtitles, teletype and story items in the HUD space; the
//           screen-sized things (cinematic borders, the 4:3 bars, the
//           overlays, the Predator's target lock and vision wipe, the
//           multiplayer scoreboard, which picks its fonts by the screen's
//           width) stay in screen pixels.  A space entered for input
//           (M15_SPACE_INPUT) changes only CInterfaceResMgr's page offsets
//           and ratios, not the screen's size, so a folder command that
//           sets a camera's rectangle from the screen's size still gets
//           the real one.
//
//           The current space is per thread (the loading screen draws
//           from a thread of its own while the main one loads).
//
//           Retail files include this inside #ifdef AVP2SRC_MODERN; one
//           of them (InterfaceMgr.cpp) defines M15_UISCALE_IMPLEMENTATION
//           first and so holds the code.
//
//           Modern build only (AVP2SRC_MODERN).
//
// ----------------------------------------------------------------------- //

#ifndef __M15_UISCALE_H__
#define __M15_UISCALE_H__

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

// The console variables (percent; saved in autoexec.cfg)
#define M15_VAR_MENUSCALE		"MenuScale"
#define M15_MENUSCALE_DEFAULT	100
#define M16_VAR_HUDSCALE		"HUDScale"
#define M16_HUDSCALE_DEFAULT	75

// The menu's choices (percent; 0 is retail's size, unscaled).
static const int g_nM15MenuChoices[] = { 0, 50, 60, 70, 80, 90, 100 };
#define M15_NUM_MENU_CHOICES	(sizeof(g_nM15MenuChoices) / sizeof(g_nM15MenuChoices[0]))
static const int g_nM16HudChoices[] = { 0, 50, 60, 70, 75, 80, 90, 100 };
#define M16_NUM_HUD_CHOICES		(sizeof(g_nM16HudChoices) / sizeof(g_nM16HudChoices[0]))

enum
{
	M15_SPACE_SCREEN	= 0,	// screen pixels (retail)
	M15_SPACE_MENU		= 1,	// the menus' page (M-15)
	M15_SPACE_HUD		= 2,	// the HUD (M-16)
	M15_NUM_SPACES		= 3,

	M15_SPACE_INPUT		= 0x10	// or'd in: the page's offsets only, not the screen's size
};

void	M15_Install();				// CGameClientShell::OnEngineInitialized: the wrappers
void	M15_Uninstall();			// CGameClientShell::OnEngineTerm: the engine's own back
LTBOOL	M15_Refresh();				// the spaces from the screen and the variables (main thread);
									// TRUE if the HUD's space changed (it is then laid out again)
int		M15_Enter(int nSpace);		// returns the space it replaces
void	M15_Leave(int nPrevious);

// The current space, as CInterfaceResMgr reports the page in it (when
// M15_PageScaled; otherwise it reports retail's own, the screen's).
LTBOOL	M15_PageScaled();
uint32	M15_Width();
uint32	M15_Height();
int		M15_PageXOffset();
int		M15_PageYOffset();
float	M15_XRatio();
float	M15_YRatio();

// A point of the screen in a space (the mouse's, for the menus).
void	M15_ScreenToSpace(int nSpace, int& x, int& y);

// The menus' 3D parts (the interface camera's sprites and models) are
// pushed back by this, retail's X ratio, so they line up with the page as
// it is scaled (1 when it fills the height).
float	M15_MenuDepth();

// The variables, kept to 0..100.
int		M15_GetPercent(const char* pName, int nDefault);
void	M15_SetPercent(const char* pName, int nValue);
int		M15_Choice(const int* pChoices, int nChoices, int nValue);

class CM15Space
{
public:
	CM15Space(int nSpace)	{ m_nPrevious = M15_Enter(nSpace); }
	~CM15Space()			{ M15_Leave(m_nPrevious); }

private:
	int		m_nPrevious;
};

#ifdef M15_UISCALE_IMPLEMENTATION

namespace
{
	struct M15SpaceDims
	{
		float	fSX, fSY;		// space -> screen
		uint32	nW, nH;			// the space's size
		LTBOOL	bScaled;
	};

	M15SpaceDims	s_aM15Space[M15_NUM_SPACES];
	uint32			s_nM15ScreenW = 640;
	uint32			s_nM15ScreenH = 480;
	DWORD			s_dwM15Tls = 0xFFFFFFFF;	// TLS_OUT_OF_INDEXES
	HSURFACE		s_hM15Screen = LTNULL;
	ILTClient*		s_pM15Client = LTNULL;

	// The engine's own entries
	void		(*s_pfnGetSurfaceDims)(HSURFACE, uint32*, uint32*);
	LTRESULT	(*s_pfnDrawSurfaceSolidColor)(HSURFACE, HSURFACE, LTRect*, int, int, HLTCOLOR, HLTCOLOR);
	LTRESULT	(*s_pfnDrawSurfaceToSurface)(HSURFACE, HSURFACE, LTRect*, int, int);
	LTRESULT	(*s_pfnDrawSurfaceToSurfaceTransparent)(HSURFACE, HSURFACE, LTRect*, int, int, HLTCOLOR);
	LTRESULT	(*s_pfnScaleSurfaceToSurface)(HSURFACE, HSURFACE, LTRect*, LTRect*);
	LTRESULT	(*s_pfnScaleSurfaceToSurfaceTransparent)(HSURFACE, HSURFACE, LTRect*, LTRect*, HLTCOLOR);
	LTRESULT	(*s_pfnScaleSurfaceToSurfaceSolidColor)(HSURFACE, HSURFACE, LTRect*, LTRect*, HLTCOLOR, HLTCOLOR);
	LTRESULT	(*s_pfnWarpSurfaceToSurface)(HSURFACE, HSURFACE, LTWarpPt*, int);
	LTRESULT	(*s_pfnWarpSurfaceToSurfaceTransparent)(HSURFACE, HSURFACE, LTWarpPt*, int, HLTCOLOR);
	LTRESULT	(*s_pfnWarpSurfaceToSurfaceSolidColor)(HSURFACE, HSURFACE, LTWarpPt*, int, HLTCOLOR, HLTCOLOR);
	LTRESULT	(*s_pfnTransformSurfaceToSurface)(HSURFACE, HSURFACE, LTFloatPt*, int, int, float, float, float);
	LTRESULT	(*s_pfnTransformSurfaceToSurfaceTransparent)(HSURFACE, HSURFACE, LTFloatPt*, int, int, float, float, float, HLTCOLOR);
	LTRESULT	(*s_pfnFillRect)(HSURFACE, LTRect*, HLTCOLOR);

	inline int M15_CurrentRaw()
	{
		if (s_dwM15Tls == 0xFFFFFFFF)
			return M15_SPACE_SCREEN;
		return (int)(DWORD)TlsGetValue(s_dwM15Tls);
	}

	// The space the page is reported in.
	inline const M15SpaceDims& M15_PageSpace()
	{
		return s_aM15Space[M15_CurrentRaw() & 0x0F];
	}

	// The space a blit onto hDest is scaled by, or none.
	inline const M15SpaceDims* M15_BlitSpace(HSURFACE hDest)
	{
		if (hDest != s_hM15Screen || !s_hM15Screen)
			return LTNULL;

		int nSpace = M15_CurrentRaw();
		if (nSpace & M15_SPACE_INPUT)
			return LTNULL;

		const M15SpaceDims* pSpace = &s_aM15Space[nSpace & 0x0F];
		return pSpace->bScaled ? pSpace : LTNULL;
	}

	inline int M15_Round(float f)
	{
		return (f >= 0.0f) ? (int)(f + 0.5f) : -(int)(0.5f - f);
	}

	inline int M15_X(const M15SpaceDims* p, int x)	{ return M15_Round((float)x * p->fSX); }
	inline int M15_Y(const M15SpaceDims* p, int y)	{ return M15_Round((float)y * p->fSY); }

	inline void M15_MapRect(const M15SpaceDims* p, const LTRect& rcIn, LTRect& rcOut)
	{
		rcOut.left		= M15_X(p, rcIn.left);
		rcOut.top		= M15_Y(p, rcIn.top);
		rcOut.right		= M15_X(p, rcIn.right);
		rcOut.bottom	= M15_Y(p, rcIn.bottom);
	}

	// A Draw's source rectangle, clipped to the source as Draw clips it,
	// and the destination rectangle it covers, scaled.  FALSE: nothing
	// to draw.
	LTBOOL M15_DrawRects(const M15SpaceDims* p, HSURFACE hSrc, LTRect* pSrcRect,
	                     int x, int y, LTRect& rcSrc, LTRect& rcDest)
	{
		uint32 nW = 0, nH = 0;
		s_pfnGetSurfaceDims(hSrc, &nW, &nH);

		if (pSrcRect)
			rcSrc = *pSrcRect;
		else
			rcSrc.Init(0, 0, (int)nW, (int)nH);

		if (rcSrc.left < 0)		{ x -= rcSrc.left; rcSrc.left = 0; }
		if (rcSrc.top < 0)		{ y -= rcSrc.top; rcSrc.top = 0; }
		if (rcSrc.right > (int)nW)	rcSrc.right = (int)nW;
		if (rcSrc.bottom > (int)nH)	rcSrc.bottom = (int)nH;

		if (rcSrc.right <= rcSrc.left || rcSrc.bottom <= rcSrc.top)
			return LTFALSE;

		LTRect rcSpace(x, y, x + (rcSrc.right - rcSrc.left), y + (rcSrc.bottom - rcSrc.top));
		M15_MapRect(p, rcSpace, rcDest);

		return (rcDest.right > rcDest.left && rcDest.bottom > rcDest.top);
	}

	// ------------------------------------------------------------------ //
	// The wrappers

	void M15W_GetSurfaceDims(HSURFACE hSurf, uint32* pWidth, uint32* pHeight)
	{
		if (hSurf == s_hM15Screen && hSurf)
		{
			int nSpace = M15_CurrentRaw();
			const M15SpaceDims& space = s_aM15Space[nSpace & 0x0F];
			if (!(nSpace & M15_SPACE_INPUT) && space.bScaled)
			{
				if (pWidth)		*pWidth = space.nW;
				if (pHeight)	*pHeight = space.nH;
				return;
			}
		}

		s_pfnGetSurfaceDims(hSurf, pWidth, pHeight);
	}

	LTRESULT M15W_DrawSurfaceToSurface(HSURFACE hDest, HSURFACE hSrc, LTRect* pSrcRect, int x, int y)
	{
		const M15SpaceDims* p = M15_BlitSpace(hDest);
		if (!p || !hSrc)
			return s_pfnDrawSurfaceToSurface(hDest, hSrc, pSrcRect, x, y);

		LTRect rcSrc, rcDest;
		if (!M15_DrawRects(p, hSrc, pSrcRect, x, y, rcSrc, rcDest))
			return LT_OK;
		return s_pfnScaleSurfaceToSurface(hDest, hSrc, &rcDest, &rcSrc);
	}

	LTRESULT M15W_DrawSurfaceToSurfaceTransparent(HSURFACE hDest, HSURFACE hSrc, LTRect* pSrcRect,
	                                              int x, int y, HLTCOLOR hColor)
	{
		const M15SpaceDims* p = M15_BlitSpace(hDest);
		if (!p || !hSrc)
			return s_pfnDrawSurfaceToSurfaceTransparent(hDest, hSrc, pSrcRect, x, y, hColor);

		LTRect rcSrc, rcDest;
		if (!M15_DrawRects(p, hSrc, pSrcRect, x, y, rcSrc, rcDest))
			return LT_OK;
		return s_pfnScaleSurfaceToSurfaceTransparent(hDest, hSrc, &rcDest, &rcSrc, hColor);
	}

	LTRESULT M15W_DrawSurfaceSolidColor(HSURFACE hDest, HSURFACE hSrc, LTRect* pSrcRect,
	                                    int x, int y, HLTCOLOR hTransColor, HLTCOLOR hFillColor)
	{
		const M15SpaceDims* p = M15_BlitSpace(hDest);
		if (!p || !hSrc)
			return s_pfnDrawSurfaceSolidColor(hDest, hSrc, pSrcRect, x, y, hTransColor, hFillColor);

		LTRect rcSrc, rcDest;
		if (!M15_DrawRects(p, hSrc, pSrcRect, x, y, rcSrc, rcDest))
			return LT_OK;
		return s_pfnScaleSurfaceToSurfaceSolidColor(hDest, hSrc, &rcDest, &rcSrc, hTransColor, hFillColor);
	}

	LTRESULT M15W_ScaleSurfaceToSurface(HSURFACE hDest, HSURFACE hSrc, LTRect* pDestRect, LTRect* pSrcRect)
	{
		const M15SpaceDims* p = M15_BlitSpace(hDest);
		if (!p || !pDestRect)
			return s_pfnScaleSurfaceToSurface(hDest, hSrc, pDestRect, pSrcRect);

		LTRect rcDest;
		M15_MapRect(p, *pDestRect, rcDest);
		return s_pfnScaleSurfaceToSurface(hDest, hSrc, &rcDest, pSrcRect);
	}

	LTRESULT M15W_ScaleSurfaceToSurfaceTransparent(HSURFACE hDest, HSURFACE hSrc, LTRect* pDestRect,
	                                               LTRect* pSrcRect, HLTCOLOR hColor)
	{
		const M15SpaceDims* p = M15_BlitSpace(hDest);
		if (!p || !pDestRect)
			return s_pfnScaleSurfaceToSurfaceTransparent(hDest, hSrc, pDestRect, pSrcRect, hColor);

		LTRect rcDest;
		M15_MapRect(p, *pDestRect, rcDest);
		return s_pfnScaleSurfaceToSurfaceTransparent(hDest, hSrc, &rcDest, pSrcRect, hColor);
	}

	LTRESULT M15W_ScaleSurfaceToSurfaceSolidColor(HSURFACE hDest, HSURFACE hSrc, LTRect* pDestRect,
	                                              LTRect* pSrcRect, HLTCOLOR hTransColor, HLTCOLOR hFillColor)
	{
		const M15SpaceDims* p = M15_BlitSpace(hDest);
		if (!p || !pDestRect)
			return s_pfnScaleSurfaceToSurfaceSolidColor(hDest, hSrc, pDestRect, pSrcRect, hTransColor, hFillColor);

		LTRect rcDest;
		M15_MapRect(p, *pDestRect, rcDest);
		return s_pfnScaleSurfaceToSurfaceSolidColor(hDest, hSrc, &rcDest, pSrcRect, hTransColor, hFillColor);
	}

	// The engine takes 3 to 10 points.
	LTBOOL M15_MapWarp(const M15SpaceDims* p, LTWarpPt* pIn, int nCoords, LTWarpPt* pOut)
	{
		if (!pIn || nCoords < 1 || nCoords > 10)
			return LTFALSE;

		for (int i = 0; i < nCoords; i++)
		{
			pOut[i] = pIn[i];
			pOut[i].dest_x = pIn[i].dest_x * p->fSX;
			pOut[i].dest_y = pIn[i].dest_y * p->fSY;
		}

		return LTTRUE;
	}

	LTRESULT M15W_WarpSurfaceToSurface(HSURFACE hDest, HSURFACE hSrc, LTWarpPt* pCoords, int nCoords)
	{
		LTWarpPt aPts[10];
		const M15SpaceDims* p = M15_BlitSpace(hDest);
		if (!p || !M15_MapWarp(p, pCoords, nCoords, aPts))
			return s_pfnWarpSurfaceToSurface(hDest, hSrc, pCoords, nCoords);
		return s_pfnWarpSurfaceToSurface(hDest, hSrc, aPts, nCoords);
	}

	LTRESULT M15W_WarpSurfaceToSurfaceTransparent(HSURFACE hDest, HSURFACE hSrc, LTWarpPt* pCoords,
	                                              int nCoords, HLTCOLOR hColor)
	{
		LTWarpPt aPts[10];
		const M15SpaceDims* p = M15_BlitSpace(hDest);
		if (!p || !M15_MapWarp(p, pCoords, nCoords, aPts))
			return s_pfnWarpSurfaceToSurfaceTransparent(hDest, hSrc, pCoords, nCoords, hColor);
		return s_pfnWarpSurfaceToSurfaceTransparent(hDest, hSrc, aPts, nCoords, hColor);
	}

	LTRESULT M15W_WarpSurfaceToSurfaceSolidColor(HSURFACE hDest, HSURFACE hSrc, LTWarpPt* pCoords,
	                                             int nCoords, HLTCOLOR hTransColor, HLTCOLOR hFillColor)
	{
		LTWarpPt aPts[10];
		const M15SpaceDims* p = M15_BlitSpace(hDest);
		if (!p || !M15_MapWarp(p, pCoords, nCoords, aPts))
			return s_pfnWarpSurfaceToSurfaceSolidColor(hDest, hSrc, pCoords, nCoords, hTransColor, hFillColor);
		return s_pfnWarpSurfaceToSurfaceSolidColor(hDest, hSrc, aPts, nCoords, hTransColor, hFillColor);
	}

	// The engine places the source's corners about the origin (the
	// destination's coordinates; the source's centre at (x, y) when there
	// is none), scales and rotates them about it.  In a space the origin
	// is scaled onto the screen and so are the scales; the destination
	// point keeps its offset from the origin (it is scaled with the
	// corners).
	void M15_MapTransform(const M15SpaceDims* p, HSURFACE hSrc, LTFloatPt* pOrigin,
	                      int& x, int& y, float& fScaleX, float& fScaleY, LTFloatPt& origin)
	{
		if (pOrigin)
		{
			origin = *pOrigin;
		}
		else
		{
			uint32 nW = 0, nH = 0;
			s_pfnGetSurfaceDims(hSrc, &nW, &nH);
			origin.x = (float)nW * 0.5f + (float)x;
			origin.y = (float)nH * 0.5f + (float)y;
		}

		float fOffX = (float)x - origin.x;
		float fOffY = (float)y - origin.y;

		x = M15_Round(origin.x * p->fSX + fOffX);
		y = M15_Round(origin.y * p->fSY + fOffY);
		origin.x = (float)x - fOffX;
		origin.y = (float)y - fOffY;

		fScaleX *= p->fSX;
		fScaleY *= p->fSY;
	}

	LTRESULT M15W_TransformSurfaceToSurface(HSURFACE hDest, HSURFACE hSrc, LTFloatPt* pOrigin,
	                                        int x, int y, float fAngle, float fScaleX, float fScaleY)
	{
		const M15SpaceDims* p = M15_BlitSpace(hDest);
		if (!p || !hSrc)
			return s_pfnTransformSurfaceToSurface(hDest, hSrc, pOrigin, x, y, fAngle, fScaleX, fScaleY);

		LTFloatPt origin;
		M15_MapTransform(p, hSrc, pOrigin, x, y, fScaleX, fScaleY, origin);
		return s_pfnTransformSurfaceToSurface(hDest, hSrc, &origin, x, y, fAngle, fScaleX, fScaleY);
	}

	LTRESULT M15W_TransformSurfaceToSurfaceTransparent(HSURFACE hDest, HSURFACE hSrc, LTFloatPt* pOrigin,
	                                                   int x, int y, float fAngle, float fScaleX,
	                                                   float fScaleY, HLTCOLOR hColor)
	{
		const M15SpaceDims* p = M15_BlitSpace(hDest);
		if (!p || !hSrc)
			return s_pfnTransformSurfaceToSurfaceTransparent(hDest, hSrc, pOrigin, x, y, fAngle, fScaleX, fScaleY, hColor);

		LTFloatPt origin;
		M15_MapTransform(p, hSrc, pOrigin, x, y, fScaleX, fScaleY, origin);
		return s_pfnTransformSurfaceToSurfaceTransparent(hDest, hSrc, &origin, x, y, fAngle, fScaleX, fScaleY, hColor);
	}

	LTRESULT M15W_FillRect(HSURFACE hDest, LTRect* pRect, HLTCOLOR hColor)
	{
		const M15SpaceDims* p = M15_BlitSpace(hDest);
		if (!p || !pRect)
			return s_pfnFillRect(hDest, pRect, hColor);

		LTRect rc;
		M15_MapRect(p, *pRect, rc);
		return s_pfnFillRect(hDest, &rc, hColor);
	}

	// A space scaled by fScale (at least 1) onto a screen of nW x nH.
	void M15_SetSpace(M15SpaceDims& space, float fScale, uint32 nW, uint32 nH)
	{
		if (fScale <= 1.0f || nW == 0 || nH == 0)
		{
			space.fSX = space.fSY = 1.0f;
			space.nW = nW;
			space.nH = nH;
			space.bScaled = LTFALSE;
			return;
		}

		space.nW = (uint32)((float)nW / fScale + 0.5f);
		space.nH = (uint32)((float)nH / fScale + 0.5f);
		if (space.nW < 1) space.nW = 1;
		if (space.nH < 1) space.nH = 1;

		// the space's edges are the screen's
		space.fSX = (float)nW / (float)space.nW;
		space.fSY = (float)nH / (float)space.nH;
		space.bScaled = (space.nW != nW || space.nH != nH);
	}

	// A percent of the screen's height over 480, never wider than
	// the screen over 640, never below 1.
	float M15_ScaleFor(int nPercent, uint32 nW, uint32 nH)
	{
		float fScale = ((float)nPercent / 100.0f) * ((float)nH / 480.0f);
		float fWidest = (float)nW / 640.0f;
		if (fScale > fWidest) fScale = fWidest;
		return (fScale < 1.0f) ? 1.0f : fScale;
	}
}

int M15_GetPercent(const char* pName, int nDefault)
{
	HCONSOLEVAR hVar = g_pLTClient->GetConsoleVar((char*)pName);
	int nValue = hVar ? (int)g_pLTClient->GetVarValueFloat(hVar) : nDefault;
	if (nValue < 0) nValue = 0;
	if (nValue > 100) nValue = 100;
	return nValue;
}

void M15_SetPercent(const char* pName, int nValue)
{
	char szCmd[64];
	sprintf(szCmd, "+%s %d", pName, nValue);
	g_pLTClient->RunConsoleString(szCmd);
}

int M15_Choice(const int* pChoices, int nChoices, int nValue)
{
	int nBest = 0;
	for (int i = 1; i < nChoices; i++)
	{
		if (abs(pChoices[i] - nValue) < abs(pChoices[nBest] - nValue))
			nBest = i;
	}
	return nBest;
}

LTBOOL M15_Refresh()
{
	if (!s_pM15Client || !s_pfnGetSurfaceDims)
		return LTFALSE;

	// the screen surface is the renderer's: none until it is up
	s_hM15Screen = s_pM15Client->GetScreenSurface();
	if (!s_hM15Screen)
		return LTFALSE;

	uint32 nW = 640, nH = 480;
	s_pfnGetSurfaceDims(s_hM15Screen, &nW, &nH);
	s_nM15ScreenW = nW;
	s_nM15ScreenH = nH;

	M15SpaceDims hudBefore = s_aM15Space[M15_SPACE_HUD];

	M15_SetSpace(s_aM15Space[M15_SPACE_SCREEN], 1.0f, nW, nH);
	M15_SetSpace(s_aM15Space[M15_SPACE_MENU],
	             M15_ScaleFor(M15_GetPercent(M15_VAR_MENUSCALE, M15_MENUSCALE_DEFAULT), nW, nH), nW, nH);
	M15_SetSpace(s_aM15Space[M15_SPACE_HUD],
	             M15_ScaleFor(M15_GetPercent(M16_VAR_HUDSCALE, M16_HUDSCALE_DEFAULT), nW, nH), nW, nH);

	return (hudBefore.nW != s_aM15Space[M15_SPACE_HUD].nW ||
	        hudBefore.nH != s_aM15Space[M15_SPACE_HUD].nH);
}

void M15_Install()
{
	ILTClient* pClient = g_pLTClient;
	if (!pClient || s_pM15Client)
		return;

	if (s_dwM15Tls == 0xFFFFFFFF)
		s_dwM15Tls = TlsAlloc();

	s_pM15Client = pClient;
	s_hM15Screen = pClient->GetScreenSurface();

	s_pfnGetSurfaceDims							= pClient->GetSurfaceDims;
	s_pfnDrawSurfaceSolidColor					= pClient->DrawSurfaceSolidColor;
	s_pfnDrawSurfaceToSurface					= pClient->DrawSurfaceToSurface;
	s_pfnDrawSurfaceToSurfaceTransparent		= pClient->DrawSurfaceToSurfaceTransparent;
	s_pfnScaleSurfaceToSurface					= pClient->ScaleSurfaceToSurface;
	s_pfnScaleSurfaceToSurfaceTransparent		= pClient->ScaleSurfaceToSurfaceTransparent;
	s_pfnScaleSurfaceToSurfaceSolidColor		= pClient->ScaleSurfaceToSurfaceSolidColor;
	s_pfnWarpSurfaceToSurface					= pClient->WarpSurfaceToSurface;
	s_pfnWarpSurfaceToSurfaceTransparent		= pClient->WarpSurfaceToSurfaceTransparent;
	s_pfnWarpSurfaceToSurfaceSolidColor			= pClient->WarpSurfaceToSurfaceSolidColor;
	s_pfnTransformSurfaceToSurface				= pClient->TransformSurfaceToSurface;
	s_pfnTransformSurfaceToSurfaceTransparent	= pClient->TransformSurfaceToSurfaceTransparent;
	s_pfnFillRect								= pClient->FillRect;

	pClient->GetSurfaceDims							= M15W_GetSurfaceDims;
	pClient->DrawSurfaceSolidColor					= M15W_DrawSurfaceSolidColor;
	pClient->DrawSurfaceToSurface					= M15W_DrawSurfaceToSurface;
	pClient->DrawSurfaceToSurfaceTransparent		= M15W_DrawSurfaceToSurfaceTransparent;
	pClient->ScaleSurfaceToSurface					= M15W_ScaleSurfaceToSurface;
	pClient->ScaleSurfaceToSurfaceTransparent		= M15W_ScaleSurfaceToSurfaceTransparent;
	pClient->ScaleSurfaceToSurfaceSolidColor		= M15W_ScaleSurfaceToSurfaceSolidColor;
	pClient->WarpSurfaceToSurface					= M15W_WarpSurfaceToSurface;
	pClient->WarpSurfaceToSurfaceTransparent		= M15W_WarpSurfaceToSurfaceTransparent;
	pClient->WarpSurfaceToSurfaceSolidColor			= M15W_WarpSurfaceToSurfaceSolidColor;
	pClient->TransformSurfaceToSurface				= M15W_TransformSurfaceToSurface;
	pClient->TransformSurfaceToSurfaceTransparent	= M15W_TransformSurfaceToSurfaceTransparent;
	pClient->FillRect								= M15W_FillRect;

	M15_Refresh();
}

void M15_Uninstall()
{
	ILTClient* pClient = s_pM15Client;
	if (!pClient)
		return;

	pClient->GetSurfaceDims							= s_pfnGetSurfaceDims;
	pClient->DrawSurfaceSolidColor					= s_pfnDrawSurfaceSolidColor;
	pClient->DrawSurfaceToSurface					= s_pfnDrawSurfaceToSurface;
	pClient->DrawSurfaceToSurfaceTransparent		= s_pfnDrawSurfaceToSurfaceTransparent;
	pClient->ScaleSurfaceToSurface					= s_pfnScaleSurfaceToSurface;
	pClient->ScaleSurfaceToSurfaceTransparent		= s_pfnScaleSurfaceToSurfaceTransparent;
	pClient->ScaleSurfaceToSurfaceSolidColor		= s_pfnScaleSurfaceToSurfaceSolidColor;
	pClient->WarpSurfaceToSurface					= s_pfnWarpSurfaceToSurface;
	pClient->WarpSurfaceToSurfaceTransparent		= s_pfnWarpSurfaceToSurfaceTransparent;
	pClient->WarpSurfaceToSurfaceSolidColor			= s_pfnWarpSurfaceToSurfaceSolidColor;
	pClient->TransformSurfaceToSurface				= s_pfnTransformSurfaceToSurface;
	pClient->TransformSurfaceToSurfaceTransparent	= s_pfnTransformSurfaceToSurfaceTransparent;
	pClient->FillRect								= s_pfnFillRect;

	s_pM15Client = LTNULL;
}

int M15_Enter(int nSpace)
{
	int nPrevious = M15_CurrentRaw();
	if (s_dwM15Tls != 0xFFFFFFFF)
		TlsSetValue(s_dwM15Tls, (LPVOID)(DWORD)nSpace);
	return nPrevious;
}

void M15_Leave(int nPrevious)
{
	if (s_dwM15Tls != 0xFFFFFFFF)
		TlsSetValue(s_dwM15Tls, (LPVOID)(DWORD)nPrevious);
}

LTBOOL M15_PageScaled()		{ return M15_PageSpace().bScaled; }
uint32 M15_Width()			{ return M15_PageSpace().nW; }
uint32 M15_Height()			{ return M15_PageSpace().nH; }
int M15_PageXOffset()		{ return ((int)M15_PageSpace().nW - 640) / 2; }
int M15_PageYOffset()		{ return ((int)M15_PageSpace().nH - 480) / 2; }
float M15_XRatio()			{ return (float)M15_PageSpace().nW / 640.0f; }
float M15_YRatio()			{ return (float)M15_PageSpace().nH / 480.0f; }

void M15_ScreenToSpace(int nSpace, int& x, int& y)
{
	const M15SpaceDims& space = s_aM15Space[nSpace & 0x0F];
	if (!space.bScaled)
		return;

	x = (int)floor((float)x / space.fSX);
	y = (int)floor((float)y / space.fSY);
}

float M15_MenuDepth()
{
	uint32 nH = s_aM15Space[M15_SPACE_MENU].nH;
	return nH ? (float)nH / 480.0f : 1.0f;
}

#endif // M15_UISCALE_IMPLEMENTATION

#endif // __M15_UISCALE_H__
