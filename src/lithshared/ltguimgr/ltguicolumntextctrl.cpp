// LTGUIColumnTextCtrl.cpp: implementation of the CLTGUIColumnTextCtrl class.
//
//////////////////////////////////////////////////////////////////////

#include "ltguimgr.h"
#include "ltguicolumntextctrl.h"
#include "ltguifont.h"

// A column's justification when it word-wraps its text to its width
// (AddWordWrapColumn)
#define LTGUI_COLUMN_WORDWRAP	0xFF

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CLTGUIColumnTextCtrl::CLTGUIColumnTextCtrl()
{
	m_pFont		= LTNULL;
	m_dwWidth	= 0;
	m_dwHeight	= 0;
	m_pCommandHandler = LTNULL;
}

CLTGUIColumnTextCtrl::~CLTGUIColumnTextCtrl()
{
	Destroy();
}

// Create the control
LTBOOL CLTGUIColumnTextCtrl::Create(ILTClient *pClientDE, uint32 dwCommandID, CLTGUIFont *pFont,
									CLTGUICommandHandler *pCommandHandler, uint32 dwParam1, uint32 dwParam2)
{
	m_pClientDE = pClientDE;
	m_pCommandHandler = pCommandHandler;

	SetFont(pFont);

	m_dwCommandID = dwCommandID;
	m_dwParam1 = dwParam1;
	m_dwParam2 = dwParam2;

	return LTTRUE;
}

// Destroys the control
void CLTGUIColumnTextCtrl::Destroy()
{
	RemoveAllColumns();
}

// Adds a column to the control
LTBOOL CLTGUIColumnTextCtrl::AddColumn(HSTRING hString, int nWidth, short nJustify)
{
	if (!hString)
		return LTFALSE;

	LTGUIColumn *pColumn = new LTGUIColumn;

	pColumn->m_nWidth = nWidth;
	pColumn->m_hString = m_pClientDE->CopyString(hString);
	pColumn->m_nJustify = nJustify;
	pColumn->m_nHeight = m_pFont->GetHeight();

	m_dwWidth += nWidth;
	m_dwHeight = LTMAX((int)m_dwHeight, pColumn->m_nHeight);

	m_columnArray.Append(pColumn);

	return LTTRUE;
}

LTBOOL CLTGUIColumnTextCtrl::AddColumn(int messageCode, int nWidth, short nJustify)
{
	if (!messageCode)
		return LTFALSE;

	HSTRING hString = m_pClientDE->FormatString(messageCode);
	LTBOOL bResult = AddColumn(hString, nWidth, nJustify);
	m_pClientDE->FreeString(hString);

	return bResult;
}

// Adds a column whose text is word-wrapped to its width
LTBOOL CLTGUIColumnTextCtrl::AddWordWrapColumn(HSTRING hString, int nWidth)
{
	if (!hString)
		return LTFALSE;

	LTGUIColumn *pColumn = new LTGUIColumn;

	m_dwWidth += nWidth;

	pColumn->m_nWidth = nWidth;
	pColumn->m_hString = m_pClientDE->CopyString(hString);
	pColumn->m_nJustify = LTGUI_COLUMN_WORDWRAP;

	LTIntPt pt = m_pFont->GetTextExtentsFormat(hString, nWidth);
	pColumn->m_nHeight = pt.y;

	m_dwHeight = LTMAX((int)m_dwHeight, pt.y);

	m_columnArray.Append(pColumn);

	return LTTRUE;
}

LTBOOL CLTGUIColumnTextCtrl::AddWordWrapColumn(int messageCode, int nWidth)
{
	if (!messageCode)
		return LTFALSE;

	HSTRING hString = m_pClientDE->FormatString(messageCode);
	LTBOOL bResult = AddWordWrapColumn(hString, nWidth);
	m_pClientDE->FreeString(hString);

	return bResult;
}

// Gets a string at a specific column index.  This returns a copy (new handle).
HSTRING CLTGUIColumnTextCtrl::GetString(int nColumnIndex)
{
	return m_pClientDE->CopyString(m_columnArray[nColumnIndex]->m_hString);
}

// Sets a string for a column.  This copies the string from hString to an internal string.
void CLTGUIColumnTextCtrl::SetString(int nColumnIndex, HSTRING hString)
{
	m_pClientDE->FreeString(m_columnArray[nColumnIndex]->m_hString);
	m_columnArray[nColumnIndex]->m_hString = m_pClientDE->CopyString(hString);

	if (m_columnArray[nColumnIndex]->m_nJustify == LTGUI_COLUMN_WORDWRAP)
	{
		LTIntPt pt = m_pFont->GetTextExtentsFormat(hString, m_columnArray[nColumnIndex]->m_nWidth);
		m_columnArray[nColumnIndex]->m_nHeight = pt.y;

		m_dwHeight = LTMAX((int)m_dwHeight, pt.y);
	}
}

void CLTGUIColumnTextCtrl::SetString(int nColumnIndex, int messageCode)
{
	if (!messageCode)
		return;

	HSTRING hString = m_pClientDE->FormatString(messageCode);
	SetString(nColumnIndex, hString);
	m_pClientDE->FreeString(hString);
}

// Removes a column
void CLTGUIColumnTextCtrl::RemoveColumn(int nIndex)
{
	LTGUIColumn *pColumn = m_columnArray[nIndex];
	if (pColumn)
	{
		if (pColumn->m_hString)
		{
			m_pClientDE->FreeString(pColumn->m_hString);
			pColumn->m_hString = LTNULL;
		}

		delete pColumn;
	}

	m_columnArray.Remove(nIndex);
}

// Removes all of the columns
void CLTGUIColumnTextCtrl::RemoveAllColumns()
{
	while (m_columnArray.GetSize() > 0)
	{
		RemoveColumn(0);
	}
}

// Render the control
void CLTGUIColumnTextCtrl::Render(HSURFACE hDestSurf)
{
	if (!m_pFont)
		return;

	HLTCOLOR color = GetCurrentColor();

	int x = m_pos.x;
	for (int i = 0; i < (int)m_columnArray.GetSize(); i++)
	{
		LTGUIColumn *pColumn = m_columnArray[i];
		int y = m_pos.y;

		int nJustify = pColumn->m_nJustify;
		int nX = x;
		if (nJustify == LTF_JUSTIFY_RIGHT)
			nX = pColumn->m_nWidth + x;

		if (nJustify == LTGUI_COLUMN_WORDWRAP)
		{
			if (m_pFont->IsMenuFriendly())
				m_pFont->DrawMenuFormat(m_columnArray[i]->m_hString, hDestSurf, x, y, m_columnArray[i]->m_nWidth, GetState());
			else
				m_pFont->DrawFormat(pColumn->m_hString, hDestSurf, x, y, pColumn->m_nWidth, color);
		}
		else
		{
			if (m_pFont->IsMenuFriendly())
			{
				// RETAIL BUG: the column's width is passed where its
				// justification belongs, so a menu font always draws the
				// column left-justified (from nX)
#ifdef AVP2_MODERN
				m_pFont->DrawMenu(m_columnArray[i]->m_hString, hDestSurf, nX, y, m_columnArray[i]->m_nJustify, GetState());
#else
				m_pFont->DrawMenu(m_columnArray[i]->m_hString, hDestSurf, nX, y, m_columnArray[i]->m_nWidth, GetState());
#endif
			}
			else
				m_pFont->Draw(pColumn->m_hString, hDestSurf, nX, y, nJustify, color);
		}

		x += m_columnArray[i]->m_nWidth;
	}
}

// Set the font
void CLTGUIColumnTextCtrl::SetFont(CLTGUIFont *pFont)
{
	m_pFont = pFont;

	m_dwHeight = 0;
	if (pFont)
	{
		for (int i = 0; i < (int)m_columnArray.GetSize(); i++)
		{
			m_dwHeight = LTMAX((int)m_dwHeight, m_columnArray[i]->m_nHeight);
		}
	}

	m_dwWidth = 0;
	for (int i = 0; i < (int)m_columnArray.GetSize(); i++)
	{
		m_dwWidth += m_columnArray[i]->m_nWidth;
	}
}

// Handle the Enter key being pressed
LTBOOL CLTGUIColumnTextCtrl::OnEnter()
{
	if (m_pCommandHandler)
	{
		if (m_pCommandHandler->SendCommand(m_dwCommandID, m_dwParam1, m_dwParam2))
			return LTTRUE;
	}

	return LTFALSE;
}

// The selection for this control has changed
void CLTGUIColumnTextCtrl::OnSelChange()
{
	if (!m_bSelected)
		StartFade();
}
