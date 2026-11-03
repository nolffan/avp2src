// LTGUIEditCtrl.cpp: implementation of the CLTGUIEditCtrl class.
//
//////////////////////////////////////////////////////////////////////

#include <string.h>
#include "ltguimgr.h"
#include "ltguieditctrl.h"
#include "ltguifont.h"
#include "vkdefs.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CLTGUIEditCtrl::CLTGUIEditCtrl()
{
	m_nBufferSize	= 0;
	m_nTextOffset	= 0;
	m_pFont			= LTNULL;
	m_lpszString	= LTNULL;
	m_pStringValue	= LTNULL;
	m_pCommandHandler	= LTNULL;
	m_hDescription	= LTNULL;
	m_nAlignment	= LTF_JUSTIFY_LEFT;
	m_bCursorEnabled	= LTFALSE;
	m_hCursorColor	= 0;
	m_nCursorPos	= 0;
}

CLTGUIEditCtrl::~CLTGUIEditCtrl()
{
	Destroy();
}

// Create the control
LTBOOL CLTGUIEditCtrl::Create(ILTClient *pClientDE, uint32 dwCommandID, HSTRING hDescription, CLTGUIFont *pFont,
							  int nTextOffset, int nBufferSize, CLTGUICommandHandler *pCommandHandler,
							  char *pStringValue, uint32 dwParam1, uint32 dwParam2)
{
	if (!pClientDE || !pFont || nBufferSize < 0)
		return LTFALSE;

	m_nBufferSize = nBufferSize;
	m_pCommandHandler = pCommandHandler;
	m_nTextOffset = nTextOffset;
	m_pFont = pFont;
	m_pClientDE = pClientDE;
	m_pStringValue = pStringValue;

	if (hDescription)
		m_hDescription = pClientDE->CopyString(hDescription);

	// Allocate the edit string
	m_lpszString = new char[m_nBufferSize];
	if (!m_lpszString)
		return LTFALSE;

	memset(m_lpszString, 0, m_nBufferSize);

	m_dwCommandID = dwCommandID;
	m_dwParam1 = dwParam1;
	m_dwParam2 = dwParam2;

	return LTTRUE;
}

// Destroys the control
void CLTGUIEditCtrl::Destroy()
{
	if (m_lpszString)
	{
		delete [] m_lpszString;
		m_lpszString = LTNULL;
	}

	if (m_hDescription)
	{
		m_pClientDE->FreeString(m_hDescription);
		m_hDescription = LTNULL;
	}
}

// Update data
void CLTGUIEditCtrl::UpdateData(LTBOOL bSaveAndValidate)
{
	if (!m_pStringValue)
		return;

	if (bSaveAndValidate)
	{
		if (m_lpszString)
			strcpy(m_pStringValue, m_lpszString);
	}
	else
	{
		SetText(m_pStringValue);
	}
}

// Render the control
// The description's y is its own local (ebx then holds the alignment), the
// right-justify re-reads m_pos.x, and the cursor rect is built as
// LTRect(nX, nY) with right assigned after: bottom's 0 store then lands
// before right's, as shipped.
void CLTGUIEditCtrl::Render(HSURFACE hDestSurf)
{
	if (!m_pFont)
		return;

	if (!m_lpszString)
		return;

	HLTCOLOR color = GetCurrentColor();

	// Draw the description
	if (m_hDescription)
	{
		int x = m_pos.x;
		int y = m_pos.y;
		if (m_nAlignment == LTF_JUSTIFY_RIGHT)
			x = m_nTextOffset + m_pos.x - 25;

		m_pFont->Draw(m_hDescription, hDestSurf, x, y, LTF_JUSTIFY_LEFT, color);
	}

	// Draw the edit string
	if (m_lpszString)
	{
		m_pFont->Draw(m_lpszString, hDestSurf, m_nTextOffset + m_pos.x, m_pos.y, LTF_JUSTIFY_LEFT, color);

		// Draw the cursor
		if (IsCursorOn() && m_bSelected)
		{
			HSTRING hString = m_pClientDE->CreateString(m_lpszString);

			LTIntPt pt;
			pt = m_pFont->GetPositionFromIndex(hString, m_nCursorPos, -1);

			m_pClientDE->FreeString(hString);

			int nX = m_nTextOffset + pt.x + m_pos.x;
			int nY = pt.y + m_pos.y;

			LTRect rect(nX, nY);
			rect.right = nX+2;
			rect.bottom = rect.top + m_pFont->GetHeight();

			m_pClientDE->FillRect(hDestSurf, &rect, m_hCursorColor);
		}
	}
}

// Should the cursor be rendered
LTBOOL CLTGUIEditCtrl::IsCursorOn()
{
	static LTFLOAT fLastTime = 0.0f;
	static LTFLOAT fCursorTimer = 0.0f;
	static LTBOOL bCursorOn = LTFALSE;

	if (!m_bCursorEnabled)
		return LTFALSE;

	LTFLOAT fTime = m_pClientDE->GetTime();

	if (fTime - fLastTime >= fCursorTimer)
	{
		fCursorTimer = m_fCursorTime;
		fLastTime = fTime;
		bCursorOn ^= 1;
		return bCursorOn;
	}

	fCursorTimer -= fTime - fLastTime;
	fLastTime = fTime;

	return bCursorOn;
}

// Set the cursor usage
void CLTGUIEditCtrl::EnableCursor(float fToggleTime, HLTCOLOR hColor)
{
	m_bCursorEnabled = LTTRUE;
	m_fCursorTime = fToggleTime;
	m_hCursorColor = hColor;
}

// Set the font
void CLTGUIEditCtrl::SetFont(CLTGUIFont *pFont)
{
	m_pFont = pFont;
}

// Set the text for the edit string
void CLTGUIEditCtrl::SetText(char *lpszString)
{
	if (!lpszString)
		return;

	strncpy(m_lpszString, lpszString, m_nBufferSize);
	m_nCursorPos = strlen(m_lpszString);
}

// Width/Height calculations
int CLTGUIEditCtrl::GetWidth()
{
	if (!m_pFont || !m_lpszString)
		return 0;

	HSTRING hString = m_pClientDE->CreateString(m_lpszString);

	LTIntPt pt;
	pt = m_pFont->GetTextExtents(hString);

	m_pClientDE->FreeString(hString);

	return pt.x;
}

int CLTGUIEditCtrl::GetHeight()
{
	if (!m_pFont)
		return 0;

	return m_pFont->GetHeight();
}

// Add a character to the end
void CLTGUIEditCtrl::AddCharacter(char c)
{
	if (!m_lpszString)
		return;

	if ((int)strlen(m_lpszString)+1 >= m_nBufferSize-1)
		return;

	int i = (int)strlen(m_lpszString);
	m_lpszString[i+1] = '\0';

	while (i > m_nCursorPos)
	{
		m_lpszString[i] = m_lpszString[i-1];
		i--;
	}

	m_lpszString[i] = c;
	m_nCursorPos++;
}

// Remove a character from the end
void CLTGUIEditCtrl::RemoveCharacter()
{
	if (!m_lpszString)
		return;

	int nLen = (int)strlen(m_lpszString);
	if (m_nCursorPos >= nLen)
		return;

	int i = m_nCursorPos;
	while (i < nLen)
	{
		m_lpszString[i] = m_lpszString[i+1];
		i++;
	}

	m_lpszString[i] = '\0';
}

// Handles a key press
// VK_DELETE sits before VK_RIGHT: the shared `return LTTRUE` tail is hosted
// by the last case in source order, and shipped hosts it after VK_RIGHT.
LTBOOL CLTGUIEditCtrl::HandleKeyDown(int key, int rep)
{
	if (CLTGUICtrl::HandleKeyDown(key, rep))
		return LTTRUE;

	switch (key)
	{
	case VK_BACK:
		if (m_nCursorPos > 0)
		{
			m_nCursorPos--;
			RemoveCharacter();
			return LTTRUE;
		}
		break;
	case VK_HOME:
		m_nCursorPos = 0;
		return LTTRUE;
	case VK_END:
		m_nCursorPos = strlen(m_lpszString);
		return LTTRUE;
	case VK_LEFT:
		if (m_nCursorPos > 0)
		{
			m_nCursorPos--;
			return LTTRUE;
		}
		break;
	case VK_DELETE:
		RemoveCharacter();
		return LTTRUE;
	case VK_RIGHT:
		if (m_nCursorPos < (int)strlen(m_lpszString))
			m_nCursorPos++;
		break;
	}

	return LTTRUE;
}

LTBOOL CLTGUIEditCtrl::HandleChar(char c)
{
	if (CLTGUICtrl::HandleChar(c))
		return LTTRUE;

	if (c < ' ')
		return LTFALSE;

	AddCharacter(c);

	return LTTRUE;
}

// Handle the Enter key being pressed
LTBOOL CLTGUIEditCtrl::OnEnter()
{
	if (m_pCommandHandler)
	{
		if (m_pCommandHandler->SendCommand(m_dwCommandID, m_dwParam1, m_dwParam2))
			return LTTRUE;
	}

	return LTFALSE;
}

// The selection for this control has changed
void CLTGUIEditCtrl::OnSelChange()
{
	if (m_bSelected)
	{
		m_nCursorPos = strlen(m_lpszString);
	}
	else
	{
		StartFade();
	}
}
