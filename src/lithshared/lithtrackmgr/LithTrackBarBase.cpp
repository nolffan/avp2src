//*********************************************************************************
// LithTrackBarBase.cpp
//
// LithTrackMgr, as AvP2's client links it (LithTrackMgrClient.lib), written
// from the release's prebuilt build of it.
//*********************************************************************************

#include "lithtrackbarbase.h"

//*********************************************************************************

LithTrackBarBase::LithTrackBarBase()
{
	Init();
}

//*********************************************************************************

LithTrackBarBase::~LithTrackBarBase()
{
	Term();
}

//*********************************************************************************

DBOOL LithTrackBarBase::Init(INTERFACE *pInterface, LithFont *pFont, void *pData)
{
	m_pInterface = pInterface;
	m_pData = pData;

	m_hSurface = DNULL;
	m_pFont = pFont;

	m_bOn = DFALSE;
	m_bFocus = DFALSE;

	m_nNumLines = 1;
	m_nSurfWidth = 0;
	m_nSurfHeight = 0;

	m_nPosX = 0;
	m_nPosY = 0;

	if(pInterface)
	{
		m_cTitle		= pInterface->SetupColor2(0.0f, 0.0f, 0.5f, DFALSE);
		m_cNormal		= pInterface->SetupColor2(1.0f, 1.0f, 1.0f, DFALSE);
		m_cHighlight	= pInterface->SetupColor2(1.0f, 1.0f, 0.0f, DFALSE);
		m_cSelected		= pInterface->SetupColor2(0.5f, 0.0f, 0.0f, DFALSE);
		m_cHSelected	= pInterface->SetupColor2(1.0f, 0.0f, 0.0f, DFALSE);
		m_cDisabled		= pInterface->SetupColor2(0.3f, 0.3f, 0.3f, DFALSE);
		m_cText			= pInterface->SetupColor2(0.8f, 0.8f, 0.8f, DFALSE);
		m_cShadow		= pInterface->SetupColor2(0.0f, 0.0f, 0.0f, DFALSE);

		if(m_pFont)
		{
			LITHCURSORCREATESTRUCT lcCS;
			lcCS.szRefName = "GUIBar_Cursor";
			lcCS.pFont = m_pFont;
			lcCS.pDest = DNULL;
			lcCS.lfDD.dwFlags = LTF_DRAW_SOLID;

			m_Cursor.Init(pInterface, &lcCS);
		}
	}

	m_nButtonFocus = -1;

	return DTRUE;
}

//*********************************************************************************

void LithTrackBarBase::Term()
{
	if(m_pInterface)
	{
		for(int i = 0; i < TRACKMGR_GUI_BAR_MAX_ITEMS; i++)
		{
			if(m_pTextItems[i].m_szText)
			{
				m_pInterface->FreeString(m_pTextItems[i].m_szText);
				m_pTextItems[i].m_szText = DNULL;
			}

			if(m_pButtonItems[i].m_szText)
			{
				m_pInterface->FreeString(m_pButtonItems[i].m_szText);
				m_pButtonItems[i].m_szText = DNULL;
			}
		}

		if(m_hSurface)
		{
			m_pInterface->DeleteSurface(m_hSurface);
			m_hSurface = DNULL;
		}

		m_Cursor.Free();
	}

	m_pInterface = DNULL;
	m_pData = DNULL;
	m_pFont = DNULL;
}

//*********************************************************************************

DBOOL LithTrackBarBase::Update()
{
	return DTRUE;
}

//*********************************************************************************

DBOOL LithTrackBarBase::UpdateResolution(DDWORD nWidth, DDWORD nHeight)
{
	if(!m_pInterface || !m_pFont) return DFALSE;

	if(m_hSurface)
	{
		m_pInterface->DeleteSurface(m_hSurface);
		m_hSurface = DNULL;
	}

	m_nSurfWidth = nWidth;
	m_nSurfHeight = (m_nNumLines * m_pFont->Height()) + (TRACKMGR_GUI_BAR_EDGE_SPACE * 2);

	m_hSurface = m_pInterface->CreateSurface(m_nSurfWidth, m_nSurfHeight);
	if(!m_hSurface) return DFALSE;

	ResetSurface();
	return DTRUE;
}

//*********************************************************************************

DBOOL LithTrackBarBase::ResetSurface()
{
	if(!m_hSurface || !m_pInterface) return DFALSE;

	DDWORD nWidth = 0, nHeight = 0;
	m_pInterface->GetSurfaceDims(m_hSurface, &nWidth, &nHeight);

	HDECOLOR cBack		= m_pInterface->SetupColor2(0.5f, 0.5f, 0.5f, DFALSE);
	HDECOLOR cLight		= m_pInterface->SetupColor2(1.0f, 1.0f, 1.0f, DFALSE);
	HDECOLOR cMedLight	= m_pInterface->SetupColor2(0.8f, 0.8f, 0.8f, DFALSE);
	HDECOLOR cDark		= m_pInterface->SetupColor2(0.0f, 0.0f, 0.0f, DFALSE);
	HDECOLOR cMedDark	= m_pInterface->SetupColor2(0.2f, 0.2f, 0.2f, DFALSE);

	m_pInterface->FillRect(m_hSurface, DNULL, cBack);

	m_rRect.top = 0;
	m_rRect.left = 0;
	m_rRect.bottom = TRACKMGR_GUI_BAR_EDGE;
	m_rRect.right = nWidth - TRACKMGR_GUI_BAR_EDGE;
	m_pInterface->FillRect(m_hSurface, &m_rRect, cLight);

	m_rRect.top = TRACKMGR_GUI_BAR_EDGE;
	m_rRect.left = 0;
	m_rRect.bottom = nHeight;
	m_rRect.right = TRACKMGR_GUI_BAR_EDGE;
	m_pInterface->FillRect(m_hSurface, &m_rRect, cMedLight);

	m_rRect.left = TRACKMGR_GUI_BAR_EDGE;
	m_rRect.top = nHeight - TRACKMGR_GUI_BAR_EDGE;
	m_rRect.right = nWidth;
	m_rRect.bottom = nHeight;
	m_pInterface->FillRect(m_hSurface, &m_rRect, cDark);

	m_rRect.left = nWidth - TRACKMGR_GUI_BAR_EDGE;
	m_rRect.top = 0;
	m_rRect.right = nWidth;
	m_rRect.bottom = nHeight - TRACKMGR_GUI_BAR_EDGE;
	m_pInterface->FillRect(m_hSurface, &m_rRect, cMedDark);

	DrawItems();

	return DTRUE;
}

//*********************************************************************************

DBOOL LithTrackBarBase::Draw(HSURFACE hScreen)
{
	if(m_bOn && m_pInterface && hScreen)
		m_pInterface->DrawSurfaceToSurface(hScreen, m_hSurface, DNULL, m_nPosX, m_nPosY);

	return DTRUE;
}

//*********************************************************************************

DBOOL LithTrackBarBase::UpdateItems()
{
	return DTRUE;
}

//*********************************************************************************

DBOOL LithTrackBarBase::DrawItems()
{
	if(!m_pInterface || !m_hSurface) return DFALSE;

	UpdateItems();

	m_Cursor.SetDest(m_hSurface);

	for(int i = 0; i < TRACKMGR_GUI_BAR_MAX_ITEMS; i++)
	{
		if(m_pTextItems[i].m_szText)
		{
			if(m_pTextItems[i].m_nWidth)
			{
				m_Cursor.DrawData().dwFlags = LTF_DRAW_SOLID | LTF_DRAW_FORMATTED;
				m_Cursor.DrawData().dwFormatWidth = m_pTextItems[i].m_nWidth;
			}
			else
				m_Cursor.DrawData().dwFlags = LTF_DRAW_SOLID;

			m_Cursor.DrawData().hColor = m_pTextItems[i].m_cColor;
			m_Cursor.SetLoc(m_pTextItems[i].m_rRect.left, m_pTextItems[i].m_rRect.top);
			m_Cursor.Draw(m_pTextItems[i].m_szText);
		}

		if(m_pButtonItems[i].m_szText)
		{
			if(m_pButtonItems[i].m_nWidth)
			{
				m_Cursor.DrawData().dwFlags = LTF_DRAW_SOLID | LTF_DRAW_FORMATTED;
				m_Cursor.DrawData().dwFormatWidth = m_pButtonItems[i].m_nWidth;
			}
			else
				m_Cursor.DrawData().dwFlags = LTF_DRAW_SOLID;

			if(m_pButtonItems[i].m_bSelected)
			{
				if(m_nButtonFocus == i)
					m_Cursor.DrawData().hColor = m_cHSelected;
				else
					m_Cursor.DrawData().hColor = m_cSelected;
			}
			else if(!m_pButtonItems[i].m_bEnabled)
				m_Cursor.DrawData().hColor = m_cDisabled;
			else if(m_nButtonFocus == i)
				m_Cursor.DrawData().hColor = m_cHighlight;
			else
				m_Cursor.DrawData().hColor = m_pButtonItems[i].m_cColor;

			m_Cursor.SetLoc(m_pButtonItems[i].m_rRect.left, m_pButtonItems[i].m_rRect.top);
			m_Cursor.Draw(m_pButtonItems[i].m_szText);
		}
	}

	return DTRUE;
}

//*********************************************************************************

DRect LithTrackBarBase::CreateTextItem(int nItem, char *szStr, HDECOLOR cColor, int x, int y, DBYTE byJustify, DDWORD nWidth)
{
	DRect rRect(0, 0, 0, 0);
	DDWORD nTextWidth, nTextHeight;

	if(!m_pInterface) return rRect;

	if(m_pTextItems[nItem].m_szText)
	{
		m_pInterface->FreeString(m_pTextItems[nItem].m_szText);
		m_pTextItems[nItem].m_szText = DNULL;
	}

	m_pTextItems[nItem].m_szText = m_pInterface->CreateString(szStr);
	m_pTextItems[nItem].m_cColor = cColor;
	m_pTextItems[nItem].m_nWidth = nWidth;

	m_Cursor.DrawData().dwFlags = LTF_DRAW_SOLID;

	if(nWidth)
	{
		m_Cursor.DrawData().dwFlags = LTF_DRAW_SOLID | LTF_DRAW_FORMATTED;
		m_Cursor.DrawData().dwFormatWidth = nWidth;
	}

	m_Cursor.GetTextExtents(m_pTextItems[nItem].m_szText, DNULL, nTextWidth, nTextHeight);

	switch(byJustify)
	{
		case GUI_JUSTIFY_CENTER:	m_pTextItems[nItem].m_rRect.left = x - (nTextWidth / 2);	break;
		case GUI_JUSTIFY_RIGHT:		m_pTextItems[nItem].m_rRect.left = x - nTextWidth;			break;
		default:					m_pTextItems[nItem].m_rRect.left = x;						break;
	}

	m_pTextItems[nItem].m_rRect.top = y;
	m_pTextItems[nItem].m_rRect.right = m_pTextItems[nItem].m_rRect.left + nTextWidth;
	m_pTextItems[nItem].m_rRect.bottom = m_pTextItems[nItem].m_rRect.top + nTextHeight;

	return m_pTextItems[nItem].m_rRect;
}

//*********************************************************************************

DRect LithTrackBarBase::CreateButtonItem(int nItem, char *szStr, HDECOLOR cColor, int x, int y, DBYTE byJustify, DDWORD nWidth, DBOOL bEnabled)
{
	DRect rRect(0, 0, 0, 0);
	DDWORD nTextWidth, nTextHeight;

	if(!m_pInterface) return rRect;

	if(m_pButtonItems[nItem].m_szText)
	{
		m_pInterface->FreeString(m_pButtonItems[nItem].m_szText);
		m_pButtonItems[nItem].m_szText = DNULL;
	}

	m_pButtonItems[nItem].m_szText = m_pInterface->CreateString(szStr);
	m_pButtonItems[nItem].m_cColor = cColor;
	m_pButtonItems[nItem].m_nWidth = nWidth;
	m_pButtonItems[nItem].m_bEnabled = bEnabled;

	m_Cursor.DrawData().dwFlags = LTF_DRAW_SOLID;

	if(nWidth)
	{
		m_Cursor.DrawData().dwFlags = LTF_DRAW_SOLID | LTF_DRAW_FORMATTED;
		m_Cursor.DrawData().dwFormatWidth = nWidth;
	}

	m_Cursor.GetTextExtents(m_pButtonItems[nItem].m_szText, DNULL, nTextWidth, nTextHeight);

	switch(byJustify)
	{
		case GUI_JUSTIFY_CENTER:	m_pButtonItems[nItem].m_rRect.left = x - (nTextWidth / 2);	break;
		case GUI_JUSTIFY_RIGHT:		m_pButtonItems[nItem].m_rRect.left = x - nTextWidth;		break;
		default:					m_pButtonItems[nItem].m_rRect.left = x;						break;
	}

	m_pButtonItems[nItem].m_rRect.top = y;
	m_pButtonItems[nItem].m_rRect.right = m_pButtonItems[nItem].m_rRect.left + nTextWidth;
	m_pButtonItems[nItem].m_rRect.bottom = m_pButtonItems[nItem].m_rRect.top + nTextHeight;

	return m_pButtonItems[nItem].m_rRect;
}

//*********************************************************************************

void LithTrackBarBase::UpdateTextItemString(int nItem, char *szStr, DBYTE byJustify)
{
	if(!m_pInterface) return;

	if(m_pTextItems[nItem].m_szText)
	{
		m_pInterface->FreeString(m_pTextItems[nItem].m_szText);
		m_pTextItems[nItem].m_szText = DNULL;
	}

	m_pTextItems[nItem].m_szText = m_pInterface->CreateString(szStr);

	DDWORD nTextWidth, nTextHeight;
	m_Cursor.GetTextExtents(m_pTextItems[nItem].m_szText, DNULL, nTextWidth, nTextHeight);

	switch(byJustify)
	{
		case GUI_JUSTIFY_CENTER:
			m_pTextItems[nItem].m_rRect.left = ((m_pTextItems[nItem].m_rRect.left + m_pTextItems[nItem].m_rRect.right) / 2) - (nTextWidth / 2);
			m_pTextItems[nItem].m_rRect.right = m_pTextItems[nItem].m_rRect.left + nTextWidth;
			break;

		case GUI_JUSTIFY_RIGHT:
			m_pTextItems[nItem].m_rRect.left = m_pTextItems[nItem].m_rRect.right - nTextWidth;
			break;

		default:
			m_pTextItems[nItem].m_rRect.right = m_pTextItems[nItem].m_rRect.left + nTextWidth;
			break;
	}

	m_pTextItems[nItem].m_rRect.bottom = m_pTextItems[nItem].m_rRect.top + nTextHeight;
}

//*********************************************************************************

void LithTrackBarBase::UpdateButtonItemString(int nItem, char *szStr, DBYTE byJustify)
{
	if(!m_pInterface) return;

	if(m_pButtonItems[nItem].m_szText)
	{
		m_pInterface->FreeString(m_pButtonItems[nItem].m_szText);
		m_pButtonItems[nItem].m_szText = DNULL;
	}

	m_pButtonItems[nItem].m_szText = m_pInterface->CreateString(szStr);

	DDWORD nTextWidth, nTextHeight;
	m_Cursor.GetTextExtents(m_pButtonItems[nItem].m_szText, DNULL, nTextWidth, nTextHeight);

	switch(byJustify)
	{
		case GUI_JUSTIFY_CENTER:
			m_pButtonItems[nItem].m_rRect.left = ((m_pButtonItems[nItem].m_rRect.left + m_pButtonItems[nItem].m_rRect.right) / 2) - (nTextWidth / 2);
			m_pButtonItems[nItem].m_rRect.right = m_pButtonItems[nItem].m_rRect.left + nTextWidth;
			break;

		case GUI_JUSTIFY_RIGHT:
			m_pButtonItems[nItem].m_rRect.left = m_pButtonItems[nItem].m_rRect.right - nTextWidth;
			break;

		default:
			m_pButtonItems[nItem].m_rRect.right = m_pButtonItems[nItem].m_rRect.left + nTextWidth;
			break;
	}

	m_pButtonItems[nItem].m_rRect.bottom = m_pButtonItems[nItem].m_rRect.top + nTextHeight;
}

//*********************************************************************************

void LithTrackBarBase::OnMouseMove(int x, int y)
{
	int nOldFocus = m_nButtonFocus;
	m_nButtonFocus = -1;

	for(int i = 0; i < TRACKMGR_GUI_BAR_MAX_ITEMS; i++)
	{
		if(m_pButtonItems[i].m_szText && m_pButtonItems[i].m_bEnabled)
		{
			if(PointInRect(x, y, m_pButtonItems[i].m_rRect))
			{
				m_nButtonFocus = i;
				break;
			}
		}
	}

	if(nOldFocus != m_nButtonFocus)
		DrawItems();
}

//*********************************************************************************

void LithTrackBarBase::OnMouseClick()
{

}

//*********************************************************************************

void LithTrackBarBase::OnEditModeOn()
{

}

//*********************************************************************************

void LithTrackBarBase::OnEditModeOff()
{

}

//*********************************************************************************

void LithTrackBarBase::OnControlChange()
{

}

//*********************************************************************************

DBOOL LithTrackBarBase::PointInRect(int x, int y, DRect &rect)
{
	if(x < rect.left) return DFALSE;
	if(x > rect.right) return DFALSE;
	if(y < rect.top) return DFALSE;
	return (y <= rect.bottom);
}

//*********************************************************************************

void LithTrackBarBase::SetFocus(DBOOL bFocus)
{
	if((m_bFocus && !bFocus) || (!m_bFocus && bFocus))
	{
		m_nButtonFocus = -1;
		ResetSurface();
	}

	m_bFocus = bFocus;
}
