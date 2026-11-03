//*********************************************************************************
//*********************************************************************************
// Project:		Lith Tech Font Manager
// Purpose:		A general font method for bitmap and system fonts
//*********************************************************************************
// File:		LithFontMgr.cpp
//*********************************************************************************
//*********************************************************************************

#define NO_PRAGMA_LIBS
#include "lithfontmgr.h"

//*********************************************************************************

LithFont* CreateLithFont(ILTClient *pClient, LITHFONTCREATESTRUCT *lfCS, LTBOOL bEnglish)
{
	if (!pClient || !lfCS || (!lfCS->szFontBitmap && !lfCS->szFontName))
		return LTNULL;

	LithFont *pFont;

	if (bEnglish && lfCS->szFontBitmap)
	{
		pFont = new LithBitmapFont;
	}
	else
	{
		if (!lfCS->szFontName)
			return LTNULL;

		pFont = new LithSystemFont;
	}

	if (!pFont)
		return LTNULL;

	if (pFont->Init(pClient, lfCS))
		return pFont;

	delete pFont;
	return LTNULL;
}

//*********************************************************************************

void FreeLithFont(LithFont *lf)
{
	if (lf)
		lf->Free();
}

//*********************************************************************************

LithCursor* CreateLithCursor(ILTClient *pClient, LITHCURSORCREATESTRUCT *lcCS)
{
	if (!pClient || !lcCS)
		return LTNULL;

	LithCursor *pCursor = new LithCursor;

	if (!pCursor)
		return LTNULL;

	if (pCursor->Init(pClient, lcCS))
		return pCursor;

	delete pCursor;
	return LTNULL;
}

//*********************************************************************************

void FreeLithCursor(LithCursor *lc)
{
	if (lc)
		lc->Free();
}

//*********************************************************************************
//*********************************************************************************

LithFontMgr::LithFontMgr()
{
	m_pClient = LTNULL;

	// Clear the font and cursor lists (loops, not memset: the second memset
	// expansion schedules its lea differently from the shipped object)
	for (int i = 0; i < LTF_MGR_MAX_FONTS; i++)
		m_pFontList[i] = LTNULL;

	for (i = 0; i < LTF_MGR_MAX_CURSORS; i++)
		m_pCursorList[i] = LTNULL;

	m_bEnglish = LTFALSE;
}

//*********************************************************************************

LithFontMgr::~LithFontMgr()
{
	Free();
}

//*********************************************************************************

void LithFontMgr::Init(ILTClient *pClient, LTBOOL bEnglish)
{
	m_pClient = pClient;
	m_bEnglish = bEnglish;
}

//*********************************************************************************

void LithFontMgr::Free()
{
	m_pClient = LTNULL;

	int i;
	for (i = 0; i < LTF_MGR_MAX_FONTS; i++)
	{
		if (m_pFontList[i])
		{
			m_pFontList[i]->Free();
			delete m_pFontList[i];
			m_pFontList[i] = LTNULL;
		}
	}

	for (i = 0; i < LTF_MGR_MAX_CURSORS; i++)
	{
		if (m_pCursorList[i])
		{
			m_pCursorList[i]->Free();
			delete m_pCursorList[i];
			m_pCursorList[i] = LTNULL;
		}
	}
}

//*********************************************************************************

LithFont* LithFontMgr::AddFont(LITHFONTCREATESTRUCT *lfCS, int nIndex)
{
	if (!m_pClient || (!lfCS->szFontBitmap && !lfCS->szFontName))
		return LTNULL;

	if (nIndex == -1)
	{
		// Find the first available slot
		for (int i = 0; i < LTF_MGR_MAX_FONTS; i++)
		{
			if (!m_pFontList[i])
				return AddFont(lfCS, i);
		}

		return LTNULL;
	}

	if (m_pFontList[nIndex])
		return LTNULL;

	if (m_bEnglish && lfCS->szFontBitmap)
	{
		m_pFontList[nIndex] = new LithBitmapFont;
	}
	else if (lfCS->szFontName)
	{
		m_pFontList[nIndex] = new LithSystemFont;
	}

	if (!m_pFontList[nIndex])
		return LTNULL;

	m_pFontList[nIndex]->Init(m_pClient, lfCS);

	return m_pFontList[nIndex];
}

//*********************************************************************************

LTBOOL LithFontMgr::DeleteFont(int nIndex)
{
	if (nIndex < 0 || nIndex >= LTF_MGR_MAX_FONTS)
		return LTFALSE;

	if (!m_pFontList[nIndex])
		return LTFALSE;

	m_pFontList[nIndex]->Free();
	delete m_pFontList[nIndex];
	m_pFontList[nIndex] = LTNULL;

	return LTTRUE;
}

//*********************************************************************************

LTBOOL LithFontMgr::DeleteFont(char *szName)
{
	if (!szName)
		return LTFALSE;

	strupr(szName);

	for (int i = 0; i < LTF_MGR_MAX_FONTS; i++)
	{
		if (m_pFontList[i])
		{
			if (!strcmp(m_pFontList[i]->GetRefName(), szName))
			{
				m_pFontList[i]->Free();
				delete m_pFontList[i];
				m_pFontList[i] = LTNULL;

				return LTTRUE;
			}
		}
	}

	return LTFALSE;
}

//*********************************************************************************

LithFont* LithFontMgr::Font(int nIndex)
{
	if (nIndex < 0 || nIndex >= LTF_MGR_MAX_FONTS)
		return LTNULL;

	return m_pFontList[nIndex];
}

//*********************************************************************************

LithFont* LithFontMgr::Font(char *szName)
{
	if (!szName)
		return LTNULL;

	strupr(szName);

	for (int i = 0; i < LTF_MGR_MAX_FONTS; i++)
	{
		if (m_pFontList[i])
		{
			if (!strcmp(m_pFontList[i]->GetRefName(), szName))
				return m_pFontList[i];
		}
	}

	return LTNULL;
}

//*********************************************************************************

LithCursor* LithFontMgr::AddCursor(LITHCURSORCREATESTRUCT *lcCS, int nIndex)
{
	if (!m_pClient)
		return LTNULL;

	if (nIndex == -1)
	{
		// Find the first available slot
		// NOTE: the shipped code scans only LTF_MGR_MAX_FONTS (32) slots of
		// the 64-entry cursor list here — an original Monolith bug,
		// preserved for byte-identity.
		for (int i = 0; i < LTF_MGR_MAX_FONTS; i++)
		{
			if (!m_pCursorList[i])
				return AddCursor(lcCS, i);
		}

		return LTNULL;
	}

	if (m_pCursorList[nIndex])
		return LTNULL;

	m_pCursorList[nIndex] = new LithCursor;

	if (m_pCursorList[nIndex])
	{
		m_pCursorList[nIndex]->Init(m_pClient, lcCS);
		return m_pCursorList[nIndex];
	}

	return LTNULL;
}

//*********************************************************************************

LTBOOL LithFontMgr::DeleteCursor(int nIndex)
{
	if (nIndex < 0 || nIndex >= LTF_MGR_MAX_CURSORS)
		return LTFALSE;

	if (!m_pCursorList[nIndex])
		return LTFALSE;

	m_pCursorList[nIndex]->Free();
	delete m_pCursorList[nIndex];
	m_pCursorList[nIndex] = LTNULL;

	return LTTRUE;
}

//*********************************************************************************

LTBOOL LithFontMgr::DeleteCursor(char *szName)
{
	if (!szName)
		return LTFALSE;

	strupr(szName);

	for (int i = 0; i < LTF_MGR_MAX_CURSORS; i++)
	{
		if (m_pCursorList[i])
		{
			if (!strcmp(m_pCursorList[i]->GetRefName(), szName))
			{
				m_pCursorList[i]->Free();
				delete m_pCursorList[i];
				m_pCursorList[i] = LTNULL;

				return LTTRUE;
			}
		}
	}

	return LTFALSE;
}

//*********************************************************************************

LithCursor* LithFontMgr::Cursor(int nIndex)
{
	if (nIndex < 0 || nIndex >= LTF_MGR_MAX_CURSORS)
		return LTNULL;

	return m_pCursorList[nIndex];
}

//*********************************************************************************

LithCursor* LithFontMgr::Cursor(char *szName)
{
	if (!szName)
		return LTNULL;

	strupr(szName);

	for (int i = 0; i < LTF_MGR_MAX_CURSORS; i++)
	{
		if (m_pCursorList[i])
		{
			if (!strcmp(m_pCursorList[i]->GetRefName(), szName))
				return m_pCursorList[i];
		}
	}

	return LTNULL;
}

//*********************************************************************************

LTBOOL LithFontMgr::Draw(int nCursor, HSTRING szString)
{
	LithCursor *pCursor = Cursor(nCursor);
	if (pCursor)
	{
		pCursor->Draw(szString, LTNULL);
		return LTTRUE;
	}

	return LTFALSE;
}

//*********************************************************************************

LTBOOL LithFontMgr::Draw(char *szCursor, HSTRING szString)
{
	LithCursor *pCursor = Cursor(szCursor);
	if (pCursor)
	{
		pCursor->Draw(szString, LTNULL);
		return LTTRUE;
	}

	return LTFALSE;
}

//*********************************************************************************

LTBOOL LithFontMgr::Draw(int nCursor, int nFont, HSTRING szString)
{
	LithCursor *pCursor = Cursor(nCursor);
	LithFont *pFont = Font(nFont);

	if (!pCursor || !pFont)
		return LTFALSE;

	pCursor->SetFont(pFont);
	pCursor->Draw(szString, LTNULL);

	return LTTRUE;
}

//*********************************************************************************

LTBOOL LithFontMgr::Draw(char *szCursor, char *szFont, HSTRING szString)
{
	LithCursor *pCursor = Cursor(szCursor);
	LithFont *pFont = Font(szFont);

	if (!pCursor || !pFont)
		return LTFALSE;

	pCursor->SetFont(pFont);
	pCursor->Draw(szString, LTNULL);

	return LTTRUE;
}
