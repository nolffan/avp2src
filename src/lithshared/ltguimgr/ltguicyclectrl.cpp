// LTGUICycleCtrl.cpp: implementation of the CLTGUICycleCtrl class.
//
//////////////////////////////////////////////////////////////////////

#include "ltguimgr.h"
#include "ltguicyclectrl.h"
#include "ltguifont.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CLTGUICycleCtrl::CLTGUICycleCtrl()
{
	m_hHeaderText	= LTNULL;
	m_nHeaderWidth	= 0;
	m_bInUpdate		= LTFALSE;
}

CLTGUICycleCtrl::~CLTGUICycleCtrl()
{
	if (m_pClientDE && m_hHeaderText)
		m_pClientDE->FreeString(m_hHeaderText);
}

// Create the control
LTBOOL CLTGUICycleCtrl::Create(ILTClient *pClientDE, HSTRING hText, CLTGUIFont *pFont,
							   int nHeaderWidth, int nSpacerWidth, int *pnValue, int alignment)
{
	if (hText && pClientDE)
		m_hHeaderText = pClientDE->CopyString(hText);

	m_nHeaderWidth = nHeaderWidth;
	m_nSpacerWidth = nSpacerWidth;
	m_nAlign = alignment;

	return CLTGUITextItemCtrl::Create(pClientDE, 0, LTNULL, pFont, LTNULL, pnValue, 0, 0);
}

// This function sets up a notification when the control's value changes
void CLTGUICycleCtrl::NotifyOnChange(uint32 dwCommandID, CLTGUICommandHandler *pCommandHandler, uint32 dwParam1, uint32 dwParam2)
{
	m_pCommandHandler = pCommandHandler;
	m_dwCommandID = dwCommandID;
	m_dwParam1 = dwParam1;
	m_dwParam2 = dwParam2;
}

// Sets the currently selected index
void CLTGUICycleCtrl::SetSelIndex(int nIndex)
{
	int nOldIndex = m_nSelIndex;

	CLTGUITextItemCtrl::SetSelIndex(nIndex);

	if (!m_bInUpdate && m_pCommandHandler && m_nSelIndex != nOldIndex)
	{
		m_pCommandHandler->SendCommand(m_dwCommandID, m_dwParam1, m_dwParam2);
	}
}

// Update data
void CLTGUICycleCtrl::UpdateData(LTBOOL bSaveAndValidate)
{
	m_bInUpdate = LTTRUE;
	CLTGUITextItemCtrl::UpdateData(bSaveAndValidate);
	m_bInUpdate = LTFALSE;
}

// Render the control
void CLTGUICycleCtrl::Render(HSURFACE hDestSurf)
{
	if (!m_pFont)
		return;

	if (!m_stringArray.GetSize())
		return;

	int nHeaderX;
	int nStringX;
	int nHeaderJustify = m_nAlign;
	int nStringJustify = m_nAlign;

	switch (m_nAlign)
	{
	case LTF_JUSTIFY_LEFT:
		nHeaderX = m_pos.x;
		nStringX = m_nSpacerWidth + m_nHeaderWidth + m_pos.x;
		break;
	case LTF_JUSTIFY_RIGHT:
		nHeaderX = m_pos.x;
		nStringX = m_dwWidth + m_pos.x;
		nHeaderJustify = LTF_JUSTIFY_LEFT;
		break;
	case LTF_JUSTIFY_CENTER:
		nHeaderX = m_nHeaderWidth + m_pos.x;
		nHeaderJustify = LTF_JUSTIFY_RIGHT;
		nStringJustify = LTF_JUSTIFY_LEFT;
		nStringX = m_nSpacerWidth + nHeaderX;
		break;
	}

	HLTCOLOR color = GetCurrentColor();

	m_pFont->Draw(m_hHeaderText, hDestSurf, nHeaderX, m_pos.y, nHeaderJustify, color);
	m_pFont->Draw(m_stringArray[m_nSelIndex], hDestSurf, nStringX, m_pos.y, nStringJustify, color);
}

// Width/Height calculations
void CLTGUICycleCtrl::CalculateSize()
{
	if (!m_pFont)
		return;

	if (m_stringArray.GetSize() > 0)
	{
		LTIntPt headerExtents = m_pFont->GetTextExtents(m_hHeaderText);
		LTIntPt stringExtents = m_pFont->GetTextExtents(m_stringArray[m_nSelIndex]);

		// Both extents are initialised at their declarations, not assigned
		// afterwards: with `LTIntPt x; x = ...;` VC6 orders the RIGHT-justify
		// sum as spacer+string+header and schedules the pops differently.
		switch (m_nAlign)
		{
		case LTF_JUSTIFY_LEFT:
			m_dwWidth = m_nSpacerWidth + m_nHeaderWidth + stringExtents.x;
			break;
		case LTF_JUSTIFY_RIGHT:
			m_dwWidth = m_nSpacerWidth + headerExtents.x + stringExtents.x;
			break;
		case LTF_JUSTIFY_CENTER:
			m_dwWidth = m_nHeaderWidth*2 + m_nSpacerWidth;
			break;
		}

		m_dwHeight = headerExtents.y;
	}
	else
	{
		LTIntPt pt;
		pt = m_pFont->GetTextExtents(m_hHeaderText);
		m_dwWidth = pt.x;
		m_dwHeight = pt.y;
	}
}
