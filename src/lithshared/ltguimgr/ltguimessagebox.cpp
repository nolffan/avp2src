// LTGUIMessageBox.cpp: implementation of the CLTGUIMessageBox class.
//
//////////////////////////////////////////////////////////////////////

#include "ltguimgr.h"
#include "ltguimessagebox.h"
#include "ltguifont.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CLTGUIMessageBox::CLTGUIMessageBox()
{
	m_nTextButtonHorzSpace = 5;
	m_nTextButtonVertSpace = 5;
	m_nMouseDownItemSel = -1;
	m_nSelection = -1;
	m_pFont = LTNULL;
	m_hText = LTNULL;
	m_pClientDE = LTNULL;
	m_pCommandHandler = LTNULL;
	m_hBackgroundSurf = LTNULL;
	m_hTransColor = SETRGB_T(0,0,0);
	m_textColor = SETRGB(255,255,255);
	m_textButtonColor = SETRGB(255,255,0);
	m_textButtonHighlightColor = SETRGB(255,255,145);
	m_screenPos.x = 0;
	m_screenPos.y = 0;
	m_bSolid = LTTRUE;
}

CLTGUIMessageBox::~CLTGUIMessageBox()
{
	Destroy();
}

// Create the message box from a pcx file
LTBOOL CLTGUIMessageBox::Create(ILTClient *pClientDE, char *lpszBackground, CLTGUIFont *pFont,
								HSTRING hText, CLTGUICommandHandler *pCommandHandler, LTBOOL bSolid)
{
	if (!pClientDE || !lpszBackground || !pFont)
		return LTFALSE;

	HSURFACE hSurf = pClientDE->CreateSurfaceFromBitmap(lpszBackground);
	if (!hSurf)
		return LTFALSE;

	return Create(pClientDE, hSurf, pFont, hText, pCommandHandler, bSolid);
}

// Create the message box
LTBOOL CLTGUIMessageBox::Create(ILTClient *pClientDE, HSURFACE hBack, CLTGUIFont *pFont,
								HSTRING hText, CLTGUICommandHandler *pCommandHandler, LTBOOL bSolid)
{
	if (!pClientDE || !hBack || !pFont)
		return LTFALSE;

	m_pFont = pFont;
	m_hBackgroundSurf = hBack;
	m_pClientDE = pClientDE;

	SetText(hText);

	m_pCommandHandler = pCommandHandler;

	return LTTRUE;
}

void CLTGUIMessageBox::Destroy()
{
	if (m_pClientDE && m_hBackgroundSurf)
	{
		m_pClientDE->DeleteSurface(m_hBackgroundSurf);
		m_hBackgroundSurf = LTNULL;
	}

	if (m_hText && m_pClientDE)
	{
		m_pClientDE->FreeString(m_hText);
		m_hText = LTNULL;
	}

	m_pCommandHandler = LTNULL;
	m_pFont = LTNULL;

	m_keyBindings.Term();
	RemoveMessageButtons();

	m_pClientDE = LTNULL;
}

// Render the message box
void CLTGUIMessageBox::Render(HSURFACE hDestSurf)
{
	if (!m_pClientDE)
		return;

	if (!m_hBackgroundSurf)
		return;

	// Draw the background
	m_pClientDE->DrawSurfaceToSurfaceTransparent(hDestSurf, m_hBackgroundSurf, LTNULL,
												 m_screenPos.x, m_screenPos.y, m_hTransColor);

	// Draw the text
	if (m_pFont && m_hText)
	{
		LTIntPt pt;
		pt = m_pFont->GetTextExtentsFormat(m_hText, GetWidth()-40);

		int x = GetWidth()/2 - pt.x/2 + m_screenPos.x;
		int y = GetHeight()/2 - pt.y/2 + m_screenPos.y;

		if (m_textButtonArray.GetSize() > 0)
		{
			y += m_nTextButtonVertSpace*2 - m_pFont->GetHeight();
		}

		y += -10;

		m_pFont->Draw(m_hText, hDestSurf, x, y, LTF_JUSTIFY_LEFT, m_textColor);
	}

	// Draw the text buttons
	if (m_textButtonArray.GetSize() > 0)
	{
		for (uint32 i = 0; i < m_textButtonArray.GetSize(); i++)
		{
			LTRect rect;
			rect = GetTextButtonRect(i);

			int x = m_screenPos.x + rect.left;
			int y = m_screenPos.y + rect.top;

			HLTCOLOR color = m_textButtonColor;
			if ((int)i == m_nSelection)
				color = m_textButtonHighlightColor;

			m_pFont->Draw(m_textButtonArray[i].m_hString, hDestSurf, x, y, LTF_JUSTIFY_LEFT, color);
		}
	}
}

// Sets the message box position
void CLTGUIMessageBox::SetPos(int xPos, int yPos, HSURFACE hDestSurf)
{
	if (hDestSurf)
	{
		if (!m_pClientDE)
			return;

		uint32 nWidth = 0, nHeight = 0;
		m_pClientDE->GetSurfaceDims(hDestSurf, &nWidth, &nHeight);

		if (xPos == LTGUI_MB_CENTER)
			m_screenPos.x = nWidth/2 - GetWidth()/2;
		else
			m_screenPos.x = xPos;

		if (yPos == LTGUI_MB_CENTER)
			m_screenPos.y = nHeight/2 - GetHeight()/2;
		else
			m_screenPos.y = yPos;
	}
	else
	{
		if (xPos != LTGUI_MB_CENTER && yPos != LTGUI_MB_CENTER)
		{
			m_screenPos.x = xPos;
			m_screenPos.y = yPos;
		}
	}
}

// Returns the bounding rectangle for a text button at a specific index
// Spelled as the game's copy (src/game/ClientShellDLL/MessageBoxCtrl.cpp): LTIntPt
// copy-init locals, nLeft += split in two, and a named LTRect whose right and
// bottom are read back from its own left and top.  All of these together fix
// the register choices; none of them alone did.
LTRect CLTGUIMessageBox::GetTextButtonRect(int nIndex)
{
	// The top of the bounding box
	int nTop=GetHeight()-m_nTextButtonVertSpace-m_pFont->GetHeight();

	// Get the total width of all of the buttons to be used for centering
	int nTotalWidth=0;
	int i;
	for (i=0; i < (int)m_textButtonArray.GetSize(); i++)
	{
		LTIntPt size=m_pFont->GetTextExtents(m_textButtonArray[i].m_hString);
		nTotalWidth+=size.x;

		// If there are more buttons, add the spacing
		if (i < (int)m_textButtonArray.GetSize()-1)
		{
			nTotalWidth+=m_nTextButtonHorzSpace;
		}
	}

	// Set the starting position for the left edge of the bounding box
	int nLeft=(GetWidth()/2)-(nTotalWidth/2);

	// Add each button's width to the left edge until we get to the index that we are looking for
	for (i=0; i < nIndex; i++)
	{
		LTIntPt size=m_pFont->GetTextExtents(m_textButtonArray[i].m_hString);
		nLeft+=size.x;
		nLeft+=m_nTextButtonHorzSpace;
	}

	LTIntPt size=m_pFont->GetTextExtents(m_textButtonArray[nIndex].m_hString);

	// Build the bounding box
	LTRect rcBounding;
	rcBounding.top=nTop;
	rcBounding.left=nLeft;
	rcBounding.right=rcBounding.left+size.x;
	rcBounding.bottom=rcBounding.top+size.y;

	return rcBounding;
}

// Set the text for the message box
void CLTGUIMessageBox::SetText(HSTRING hString)
{
	if (!m_pClientDE)
		return;

	if (m_hText)
	{
		m_pClientDE->FreeString(m_hText);
		m_hText = LTNULL;
	}

	if (hString)
	{
		m_hText = m_pClientDE->CopyString(hString);
	}
}

// Add a key and tie it to a message
void CLTGUIMessageBox::AddKey(int nVKeyCode, uint32 dwMessageID)
{
	LTGUIKeyMessage key;
	key.m_nVKeyCode = nVKeyCode;
	key.m_dwMessageID = dwMessageID;

	m_keyBindings.Append(key);
}

// Adds a "text button" to the bottom row of the message box
void CLTGUIMessageBox::AddMessageButton(HSTRING hString, uint32 dwMessageID)
{
	LTGUITextButton button;
	button.m_hString = m_pClientDE->CopyString(hString);
	button.m_dwMessageID = dwMessageID;

	m_textButtonArray.Append(button);
}

// Removes all text buttons
void CLTGUIMessageBox::RemoveMessageButtons()
{
	if (m_pClientDE)
	{
		for (uint32 i = 0; i < m_textButtonArray.GetSize(); i++)
		{
			m_pClientDE->FreeString(m_textButtonArray[i].m_hString);
		}
	}

	m_textButtonArray.Term();
}

// Sets the "text buttons" color and highlight color
void CLTGUIMessageBox::SetTextButtonColor(HLTCOLOR color, HLTCOLOR highlightColor)
{
	m_textButtonColor = color;
	m_textButtonHighlightColor = highlightColor;
}

// Height/Width
int CLTGUIMessageBox::GetWidth()
{
	uint32 nWidth = 0, nHeight = 0;

	if (m_hBackgroundSurf)
	{
		m_pClientDE->GetSurfaceDims(m_hBackgroundSurf, &nWidth, &nHeight);
		return nWidth;
	}

	return 0;
}

int CLTGUIMessageBox::GetHeight()
{
	uint32 nWidth = 0, nHeight = 0;

	if (m_hBackgroundSurf)
	{
		m_pClientDE->GetSurfaceDims(m_hBackgroundSurf, &nWidth, &nHeight);
		return nHeight;
	}

	return 0;
}

// Process a key message
LTBOOL CLTGUIMessageBox::HandleKeyDown(int key, int rep)
{
	for (uint32 i = 0; i < m_keyBindings.GetSize(); i++)
	{
		if (m_keyBindings[i].m_nVKeyCode == key && m_pCommandHandler)
		{
			if (m_pCommandHandler->SendCommand(m_keyBindings[i].m_dwMessageID, 0, 0))
				return LTTRUE;
		}
	}

	return LTFALSE;
}

// Mouse messages
LTBOOL CLTGUIMessageBox::OnLButtonDown(int x, int y)
{
	if (m_textButtonArray.GetSize() <= 0)
		return LTFALSE;

	m_nMouseDownItemSel = GetButtonFromPoint(x, y);

	return (m_nMouseDownItemSel != -1);
}

LTBOOL CLTGUIMessageBox::OnLButtonUp(int x, int y)
{
	if (m_textButtonArray.GetSize() > 0)
	{
		int nButton = GetButtonFromPoint(x, y);
		if (nButton != -1 && m_nMouseDownItemSel == nButton)
		{
			if (m_pCommandHandler->SendCommand(m_textButtonArray[nButton].m_dwMessageID, 0, 0))
			{
				m_nMouseDownItemSel = -1;
				return LTTRUE;
			}
		}

		m_nMouseDownItemSel = -1;
	}

	return LTFALSE;
}

LTBOOL CLTGUIMessageBox::OnMouseMove(int x, int y)
{
	m_nSelection = GetButtonFromPoint(x, y);

	return LTFALSE;
}

// Returns the button index that is under the point
int CLTGUIMessageBox::GetButtonFromPoint(int x, int y)
{
	for (int i = 0; i < (int)m_textButtonArray.GetSize(); i++)
	{
		LTRect rect;
		rect = GetTextButtonRect(i);

		if (x >= m_screenPos.x + rect.left && y >= m_screenPos.y + rect.top &&
			x <= m_screenPos.x + rect.right && y <= m_screenPos.y + rect.bottom)
		{
			return i;
		}
	}

	return -1;
}
