// ----------------------------------------------------------------------- //
//
// MODULE  : LTGUIFont.cpp
//
// PURPOSE : Font wrapper around LithFont for the GUI controls
//
// AvP2's build of it (the release's prebuilt ltguimgr_LT2.lib): a menu
// font of three faces (normal, selected, disabled) beside the plain one,
// DrawMenu and DrawMenuFormat, and the extents of a char string.
//
// ----------------------------------------------------------------------- //

#include "ltguimgr.h"
#include "ltguifont.h"

LTBOOL CLTGUIFont::s_bWrapUseSpaces = LTTRUE;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CLTGUIFont::CLTGUIFont()
{
	m_nType = LGFT_NONE;

	for (int i = 0; i < LGF_MAX_FONTS; i++)
		m_pLithFonts[i] = LTNULL;
}

CLTGUIFont::~CLTGUIFont()
{
	Term();
}

// Initialize a bitmap font
LTBOOL CLTGUIFont::Init(ILTClient *pClientDE, LITHFONTCREATESTRUCT *lfCS)
{
	m_pClientDE = pClientDE;
	if (!pClientDE)
		return LTFALSE;

	m_pLithFonts[LGFI_NORMAL] = CreateLithFont(pClientDE, lfCS, LTTRUE);
	if (!m_pLithFonts[LGFI_NORMAL])
		return LTFALSE;

	m_nType = lfCS->szFontBitmap ? LGFT_BITMAP : LGFT_ENGINE;
	return LTTRUE;
}

// Termination
void CLTGUIFont::Term()
{
	for (int i = 0; i < LGF_MAX_FONTS; i++)
	{
		if (m_pLithFonts[i])
		{
			FreeLithFont(m_pLithFonts[i]);
			m_pLithFonts[i] = LTNULL;
		}
	}

	m_nType = LGFT_NONE;
}

void CLTGUIFont::Draw(char *szString, HSURFACE hDest, int x, int y, int justify, HLTCOLOR color)
{
	if (!m_pClientDE || !szString || !m_pLithFonts[LGFI_NORMAL])
		return;

	LITHFONTDRAWDATA dd;

	if (m_pLithFonts[LGFI_NORMAL]->IsSystemFont() && color)
		dd.dwFlags = LTF_DRAW_SOLID;
	else
		dd.dwFlags = LTF_DRAW_NORMAL;

	dd.hColor = color;
	dd.bForceFormat = LTTRUE;

	switch (justify)
	{
	case 0:
		dd.byJustify = LTF_JUSTIFY_LEFT;
		break;
	case 1:
		dd.byJustify = LTF_JUSTIFY_RIGHT;
		break;
	case 2:
		dd.byJustify = LTF_JUSTIFY_CENTER;
		break;
	}

	m_pLithFonts[LGFI_NORMAL]->Draw(hDest, szString, &dd, x, y);
}

void CLTGUIFont::Draw(HSTRING hString, HSURFACE hDest, int x, int y, int justify, HLTCOLOR color)
{
	if (!m_pClientDE || !hString)
		return;

	char *pString = m_pClientDE->GetStringData(hString);
	Draw(pString, hDest, x, y, justify, color);
}

void CLTGUIFont::DrawFormat(char *szString, HSURFACE hDest, int x, int y, int nWidth, HLTCOLOR color)
{
	if (!m_pClientDE || !szString || !m_pLithFonts[LGFI_NORMAL])
		return;

	LITHFONTDRAWDATA dd;

	dd.dwFlags = ((m_pLithFonts[LGFI_NORMAL]->IsSystemFont() && color) ? LTF_DRAW_SOLID : LTF_DRAW_NORMAL) | LTF_DRAW_FORMATTED;
	dd.dwFormatWidth = nWidth;
	dd.byJustify = LTF_JUSTIFY_LEFT;
	dd.bForceFormat = LTTRUE;
	dd.hColor = color;

	m_pLithFonts[LGFI_NORMAL]->Draw(hDest, szString, &dd, x, y);
}

void CLTGUIFont::DrawFormat(HSTRING hString, HSURFACE hDest, int x, int y, int nWidth, HLTCOLOR color)
{
	if (!m_pClientDE || !hString)
		return;

	char *pString = m_pClientDE->GetStringData(hString);
	DrawFormat(pString, hDest, x, y, nWidth, color);
}

// Returns the height and width of a text string
LTIntPt CLTGUIFont::GetTextExtents(HSTRING hString)
{
	LTIntPt pt(0, 0);

	if (!m_pClientDE || !hString)
		return pt;

	if (m_pLithFonts[LGFI_NORMAL])
	{
		LITHFONTDRAWDATA dd;
		dd.dwFlags = LTF_DRAW_NORMAL;
		dd.byJustify = LTF_JUSTIFY_LEFT;

		uint32 x = 0, y = 0;
		m_pLithFonts[LGFI_NORMAL]->GetTextExtents(hString, &dd, x, y);

		pt.x = x;
		pt.y = y;
	}

	return pt;
}

LTIntPt CLTGUIFont::GetTextExtents(char *szString)
{
	LTIntPt pt(0, 0);

	if (!m_pClientDE || !szString)
		return pt;

	if (m_pLithFonts[LGFI_NORMAL])
	{
		LITHFONTDRAWDATA dd;
		dd.dwFlags = LTF_DRAW_NORMAL;
		dd.byJustify = LTF_JUSTIFY_LEFT;

		uint32 x = 0, y = 0;
		m_pLithFonts[LGFI_NORMAL]->GetTextExtents(szString, &dd, x, y);

		pt.x = x;
		pt.y = y;
	}

	return pt;
}

LTIntPt CLTGUIFont::GetTextExtentsFormat(HSTRING hString, int nWidth)
{
	LTIntPt pt(0, 0);

	if (!m_pClientDE || !hString)
		return pt;

	if (m_pLithFonts[LGFI_NORMAL])
	{
		LITHFONTDRAWDATA dd;
		dd.dwFlags = LTF_DRAW_FORMATTED;
		dd.dwFormatWidth = nWidth;
		dd.byJustify = LTF_JUSTIFY_LEFT;
		dd.bForceFormat = LTTRUE;

		uint32 x = 0, y = 0;
		m_pLithFonts[LGFI_NORMAL]->GetTextExtents(hString, &dd, x, y);

		pt.x = x;
		pt.y = y;
	}

	return pt;
}

LTIntPt CLTGUIFont::GetTextExtentsFormat(char *szString, int nWidth)
{
	LTIntPt pt(0, 0);

	if (!m_pClientDE || !szString)
		return pt;

	if (m_pLithFonts[LGFI_NORMAL])
	{
		LITHFONTDRAWDATA dd;
		dd.dwFlags = LTF_DRAW_FORMATTED;
		dd.dwFormatWidth = nWidth;
		dd.byJustify = LTF_JUSTIFY_LEFT;
		dd.bForceFormat = LTTRUE;

		uint32 x = 0, y = 0;
		m_pLithFonts[LGFI_NORMAL]->GetTextExtents(szString, &dd, x, y);

		pt.x = x;
		pt.y = y;
	}

	return pt;
}

LTIntPt CLTGUIFont::GetPositionFromIndex(HSTRING hString, int nIndex, int nWidth)
{
	LTIntPt pt(0, 0);

	if (!m_pClientDE || !hString)
		return pt;

	if (m_pLithFonts[LGFI_NORMAL])
	{
		LITHFONTDRAWDATA dd;
		if (nWidth > 0)
		{
			dd.dwFlags = LTF_DRAW_FORMATTED;
			dd.dwFormatWidth = nWidth;
		}
		else
		{
			dd.dwFlags = LTF_DRAW_NORMAL;
		}
		dd.byJustify = LTF_JUSTIFY_LEFT;

		uint32 x = 0, y = 0;
		m_pLithFonts[LGFI_NORMAL]->GetPositionFromIndex(hString, &dd, nIndex, x, y);

		pt.x = x;
		pt.y = y;
	}

	return pt;
}

// Returns the height of the font
int CLTGUIFont::GetHeight()
{
	if (m_pLithFonts[LGFI_NORMAL])
		return m_pLithFonts[LGFI_NORMAL]->Height();
	return 0;
}

void CLTGUIFont::Draw(HSTRING hString, HSURFACE hDest, LITHFONTDRAWDATA *lfDD, int x, int y, LITHFONTSAVEDATA *lfSD)
{
	if (!m_pClientDE || !hString)
		return;

	char *pString = m_pClientDE->GetStringData(hString);
	Draw(pString, hDest, lfDD, x, y, lfSD);
}

void CLTGUIFont::Draw(char *szString, HSURFACE hDest, LITHFONTDRAWDATA *lfDD, int x, int y, LITHFONTSAVEDATA *lfSD)
{
	if (!m_pClientDE || !szString || !m_pLithFonts[LGFI_NORMAL])
		return;

	m_pLithFonts[LGFI_NORMAL]->Draw(hDest, szString, lfDD, x, y, lfSD);
}

LTBOOL CLTGUIFont::IsChromaKeyed()
{
	if (!m_pLithFonts[LGFI_NORMAL])
		return LTFALSE;
	return m_pLithFonts[LGFI_NORMAL]->IsChromaKeyed();
}

// Initialize a menu font
LTBOOL CLTGUIFont::Init(ILTClient *pClientDE, LITHFONTCREATESTRUCT *sNormalFont, LITHFONTCREATESTRUCT *sSelectedFont, LITHFONTCREATESTRUCT *sDisabledFont)
{
	if (!pClientDE || !sNormalFont || !sSelectedFont || !sDisabledFont)
		return LTFALSE;

	m_pClientDE = pClientDE;
	m_nType = LGFT_MENU;

	LithFont *pFont = CreateLithFont(pClientDE, sNormalFont, LTTRUE);
	if (!pFont)
		return LTFALSE;
	m_pLithFonts[LGFI_NORMAL] = pFont;

	pFont = CreateLithFont(pClientDE, sSelectedFont, LTTRUE);
	if (!pFont)
		return LTFALSE;
	m_pLithFonts[LGFI_SELECTED] = pFont;

	pFont = CreateLithFont(pClientDE, sDisabledFont, LTTRUE);
	if (!pFont)
		return LTFALSE;
	m_pLithFonts[LGFI_DISABLED] = pFont;

	return LTTRUE;
}

void CLTGUIFont::DrawMenu(HSTRING hString, HSURFACE hDest, int x, int y, int justify, int state)
{
	if (!m_pClientDE || !hString || !hDest)
		return;

	char *pString = m_pClientDE->GetStringData(hString);
	if (!pString[0])
		return;

	DrawMenu(pString, hDest, x, y, justify, state);
}

void CLTGUIFont::DrawMenu(char *szString, HSURFACE hDest, int x, int y, int justify, int state)
{
	if (!m_pClientDE || !szString || !hDest)
		return;

	if (m_nType == LGFT_MENU)
	{
		LithFont *pFont;
		switch (state)
		{
		case LGFI_NORMAL:
			pFont = m_pLithFonts[LGFI_NORMAL];
			break;
		case LGFI_SELECTED:
			pFont = m_pLithFonts[LGFI_SELECTED];
			break;
		case LGFI_DISABLED:
			pFont = m_pLithFonts[LGFI_DISABLED];
			break;
		default:
			return;
		}

		if (!pFont)
			return;

		LITHFONTDRAWDATA dd;
		dd.dwFlags = LTF_DRAW_NORMAL;

		switch (justify)
		{
		case 0:
			dd.byJustify = LTF_JUSTIFY_LEFT;
			break;
		case 1:
			dd.byJustify = LTF_JUSTIFY_RIGHT;
			break;
		case 2:
			dd.byJustify = LTF_JUSTIFY_CENTER;
			break;
		}

		pFont->Draw(hDest, szString, &dd, x, y);
	}
	else
	{
		Draw(szString, hDest, x, y, justify);
	}
}

void CLTGUIFont::DrawMenuFormat(HSTRING hString, HSURFACE hDest, int x, int y, int width, int state)
{
	if (!m_pClientDE || !hString || !hDest)
		return;

	char *pString = m_pClientDE->GetStringData(hString);
	DrawMenuFormat(pString, hDest, x, y, width, state);
}

void CLTGUIFont::DrawMenuFormat(char *szString, HSURFACE hDest, int x, int y, int width, int state)
{
	if (!m_pClientDE || !szString || !hDest)
		return;

	if (m_nType == LGFT_MENU)
	{
		LithFont *pFont;
		switch (state)
		{
		case LGFI_NORMAL:
			pFont = m_pLithFonts[LGFI_NORMAL];
			break;
		case LGFI_SELECTED:
			pFont = m_pLithFonts[LGFI_SELECTED];
			break;
		case LGFI_DISABLED:
			pFont = m_pLithFonts[LGFI_DISABLED];
			break;
		default:
			return;
		}

		if (!pFont)
			return;

		LITHFONTDRAWDATA dd;
		dd.dwFormatWidth = width;
		dd.dwFlags = LTF_DRAW_FORMATTED;
		dd.byJustify = LTF_JUSTIFY_LEFT;

		pFont->Draw(hDest, szString, &dd, x, y);
	}
	else
	{
		DrawFormat(szString, hDest, x, y, width);
	}
}
