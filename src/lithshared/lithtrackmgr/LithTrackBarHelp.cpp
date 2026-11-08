//*********************************************************************************
// LithTrackBarHelp.cpp
//
// LithTrackMgr, as AvP2's client links it (LithTrackMgrClient.lib), written
// from the release's prebuilt build of it.
//*********************************************************************************

#include "lithtrackbarhelp.h"

//*********************************************************************************

DBOOL LithTrackBarHelp::Init(INTERFACE *pInterface, LithFont *pFont, void *pData)
{
	LithTrackBarBase::Init(pInterface, pFont, pData);

	if(m_pInterface && m_pFont)
		SetupLines(0, 0);

	return DTRUE;
}

//*********************************************************************************

DBOOL LithTrackBarHelp::UpdateResolution(DDWORD nWidth, DDWORD nHeight)
{
	SetupLines(nWidth - m_pTextItems[HELPBAR_TEXT_GENERAL1].m_rRect.left - TRACKMGR_GUI_BAR_EDGE_SPACE, nHeight);
	return LithTrackBarBase::UpdateResolution(nWidth, nHeight);
}

//*********************************************************************************

void LithTrackBarHelp::SetupLines(int nWidth, int nHeight)
{
	if(!m_pFont) return;

	int nFontHeight = m_pFont->Height();
	DRect rect;

	// RETAIL BUG: every line passes nWidth where CreateTextItem takes its
	// justification, so the lines are never word wrapped to the bar's width
	// and a width whose low byte is 1 or 2 centres or right-justifies them.
	rect = CreateTextItem(HELPBAR_TEXT_TITLE, TITLE_TRACK_HELP, m_cTitle, TRACKMGR_GUI_BAR_EDGE_SPACE, TRACKMGR_GUI_BAR_EDGE_SPACE, nWidth);

	int x = rect.right + (nFontHeight * 2);

	rect = CreateTextItem(HELPBAR_TEXT_GENERAL1, HELP_TEXT_GENERAL1, m_cText, x, TRACKMGR_GUI_BAR_EDGE_SPACE, nWidth);
	rect = CreateTextItem(HELPBAR_TEXT_GENERAL2, HELP_TEXT_GENERAL2, m_cText, x, rect.bottom + nFontHeight, nWidth);
	rect = CreateTextItem(HELPBAR_TEXT_COMMANDS, HELP_TEXT_COMMANDS, m_cText, x, rect.bottom + nFontHeight, nWidth);
	rect = CreateTextItem(HELPBAR_TEXT_EXT_COMMANDS, HELP_TEXT_EXT_COMMANDS, m_cText, x, rect.bottom, nWidth);

	m_nNumLines = (m_pTextItems[HELPBAR_TEXT_EXT_COMMANDS].m_rRect.bottom - m_pTextItems[HELPBAR_TEXT_GENERAL1].m_rRect.top) / m_pFont->Height();
}
