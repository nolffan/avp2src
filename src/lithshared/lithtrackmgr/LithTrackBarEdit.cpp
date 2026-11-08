//*********************************************************************************
// LithTrackBarEdit.cpp
//
// LithTrackMgr, as AvP2's client links it (LithTrackMgrClient.lib), written
// from the release's prebuilt build of it.
//*********************************************************************************

#include "lithtrackdefs.h"
#include "lithtrackmgr.h"

//*********************************************************************************

DBOOL LithTrackBarEdit::Init(INTERFACE *pInterface, LithFont *pFont, void *pData)
{
	LithTrackBarBase::Init(pInterface, pFont, pData);
	SetupLines(0, 0);
	return DTRUE;
}

//*********************************************************************************

DBOOL LithTrackBarEdit::Update()
{
	if(!LithTrackBarBase::Update())
		return DFALSE;

	if(!memcmp(&g_pTrackMgr->DataAlign(), &m_dAlign, sizeof(DATA_ALIGN)))
		return DFALSE;

	ResetSurface();
	m_dAlign = g_pTrackMgr->DataAlign();

	return DTRUE;
}

//*********************************************************************************

DBOOL LithTrackBarEdit::UpdateResolution(DDWORD nWidth, DDWORD nHeight)
{
	SetupLines(nWidth, nHeight);
	return LithTrackBarBase::UpdateResolution(nWidth, nHeight);
}

//*********************************************************************************

DBOOL LithTrackBarEdit::UpdateItems()
{
	if(!m_pInterface) return DFALSE;

	char szStr[64];
	DATA_ALIGN dAlign = g_pTrackMgr->DataAlign();

	if(dAlign.m_bCamera)
		sprintf(szStr, "");
	else
		sprintf(szStr, TEXT_REFERENCE_POS, dAlign.m_vPos.x, dAlign.m_vPos.y, dAlign.m_vPos.z);

	UpdateTextItemString(EDITBAR_TEXT_REFERENCE_POS, szStr, GUI_JUSTIFY_RIGHT);

	sprintf(szStr, TEXT_REFERENCE_TYPE, dAlign.m_bCamera ? "Camera" : (dAlign.m_hObj ? "Object" : "Point"));
	UpdateTextItemString(EDITBAR_TEXT_REFERENCE_TYPE, szStr);

	// Keep the reference type text and button lined up against the position text
	int nWidth = m_pTextItems[EDITBAR_TEXT_REFERENCE_TYPE].m_rRect.right - m_pTextItems[EDITBAR_TEXT_REFERENCE_TYPE].m_rRect.left;
	m_pTextItems[EDITBAR_TEXT_REFERENCE_TYPE].m_rRect.right = m_pTextItems[EDITBAR_TEXT_REFERENCE_POS].m_rRect.left;
	m_pTextItems[EDITBAR_TEXT_REFERENCE_TYPE].m_rRect.left = m_pTextItems[EDITBAR_TEXT_REFERENCE_TYPE].m_rRect.right - nWidth;

	nWidth = m_pButtonItems[EDITBAR_BUTTON_REFERENCE].m_rRect.right - m_pButtonItems[EDITBAR_BUTTON_REFERENCE].m_rRect.left;
	m_pButtonItems[EDITBAR_BUTTON_REFERENCE].m_rRect.right = m_pTextItems[EDITBAR_TEXT_REFERENCE_TYPE].m_rRect.left;
	m_pButtonItems[EDITBAR_BUTTON_REFERENCE].m_rRect.left = m_pButtonItems[EDITBAR_BUTTON_REFERENCE].m_rRect.right - nWidth;

	return DTRUE;
}

//*********************************************************************************

void LithTrackBarEdit::OnMouseClick()
{
	if(!g_pTrackMgr) return;

	int nBar = GUI_BAR_NONE;

	switch(m_nButtonFocus)
	{
		case EDITBAR_BUTTON_MANAGER:	nBar = GUI_BAR_MANAGER;		break;
		case EDITBAR_BUTTON_CAMERA:		nBar = GUI_BAR_CAMERA;		break;
		case EDITBAR_BUTTON_TRACK:		nBar = GUI_BAR_TRACK;		break;
		case EDITBAR_BUTTON_KEY:		nBar = GUI_BAR_KEY;			break;
		case EDITBAR_BUTTON_HELP:		nBar = GUI_BAR_HELP;		break;

		case EDITBAR_BUTTON_REFERENCE:
			g_pTrackMgr->DataAlign().m_bCamera = DTRUE;
			break;
	}

	if(nBar != GUI_BAR_NONE)
	{
		m_pButtonItems[m_nButtonFocus].m_bSelected = !m_pButtonItems[m_nButtonFocus].m_bSelected;
		g_pTrackMgr->GUIBarState(nBar, ButtonSelected(m_nButtonFocus));
	}

	ResetSurface();
}

//*********************************************************************************

void LithTrackBarEdit::SetupLines(int nWidth, int nHeight)
{
	if(!m_pFont) return;

	int nSpace = m_pFont->Height() * 2;
	DRect rect;

	rect = CreateTextItem(EDITBAR_TEXT_TITLE, TITLE_TRACK_EDIT, m_cTitle, TRACKMGR_GUI_BAR_EDGE_SPACE, TRACKMGR_GUI_BAR_EDGE_SPACE);
	rect = CreateButtonItem(EDITBAR_BUTTON_MANAGER, BUTTON_MANAGER, m_cNormal, rect.right + nSpace, TRACKMGR_GUI_BAR_EDGE_SPACE);
	rect = CreateButtonItem(EDITBAR_BUTTON_CAMERA, BUTTON_CAMERA, m_cNormal, rect.right + nSpace, TRACKMGR_GUI_BAR_EDGE_SPACE);
	rect = CreateButtonItem(EDITBAR_BUTTON_TRACK, BUTTON_TRACK, m_cNormal, rect.right + nSpace, TRACKMGR_GUI_BAR_EDGE_SPACE);
	rect = CreateButtonItem(EDITBAR_BUTTON_KEY, BUTTON_KEY, m_cNormal, rect.right + nSpace, TRACKMGR_GUI_BAR_EDGE_SPACE);
	rect = CreateButtonItem(EDITBAR_BUTTON_HELP, BUTTON_HELP, m_cNormal, rect.right + nSpace, TRACKMGR_GUI_BAR_EDGE_SPACE);

	rect = CreateTextItem(EDITBAR_TEXT_REFERENCE_POS, TEXT_REFERENCE_POS, m_cText, nWidth - TRACKMGR_GUI_BAR_EDGE_SPACE, TRACKMGR_GUI_BAR_EDGE_SPACE, GUI_JUSTIFY_RIGHT);
	rect = CreateTextItem(EDITBAR_TEXT_REFERENCE_TYPE, TEXT_REFERENCE_TYPE, m_cText, rect.left, TRACKMGR_GUI_BAR_EDGE_SPACE, GUI_JUSTIFY_RIGHT);
	rect = CreateButtonItem(EDITBAR_BUTTON_REFERENCE, BUTTON_REFERENCE, m_cNormal, rect.left, TRACKMGR_GUI_BAR_EDGE_SPACE, GUI_JUSTIFY_RIGHT);
}
