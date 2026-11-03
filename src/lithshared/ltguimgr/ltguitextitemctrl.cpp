// LTGUITextItemCtrl.cpp: implementation of the CLTGUITextItemCtrl class.
//
//////////////////////////////////////////////////////////////////////

#include "ltguimgr.h"
#include "ltguitextitemctrl.h"
#include "ltguifont.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CLTGUITextItemCtrl::CLTGUITextItemCtrl()
{
	m_pFont				= LTNULL;
	m_pCommandHandler	= LTNULL;
	m_dwWidth			= 0;
	m_dwHeight			= 0;
	m_bFixedWidth		= LTFALSE;
	m_nSelIndex			= -1;
}

CLTGUITextItemCtrl::~CLTGUITextItemCtrl()
{
	Destroy();
}

// Create the control
LTBOOL CLTGUITextItemCtrl::Create(ILTClient *pClientDE, uint32 dwCommandID, HSTRING hText, CLTGUIFont *pFont,
								  CLTGUICommandHandler *pCommandHandler, int *pnValue, uint32 dwParam1, uint32 dwParam2)
{
	m_pClientDE = pClientDE;
	m_pCommandHandler = pCommandHandler;

	if (!SetFont(pFont))
		return LTFALSE;

	if (hText)
		AddString(hText);
	else
		m_nSelIndex = -1;

	m_pnValue = pnValue;
	m_dwCommandID = dwCommandID;
	m_dwParam1 = dwParam1;
	m_dwParam2 = dwParam2;

	return LTTRUE;
}

// Destroys the control
void CLTGUITextItemCtrl::Destroy()
{
	RemoveAll();
}

// Update data
void CLTGUITextItemCtrl::UpdateData(LTBOOL bSaveAndValidate)
{
	if (!m_pnValue)
		return;

	if (bSaveAndValidate)
		*m_pnValue = m_nSelIndex;
	else
		SetSelIndex(*m_pnValue);
}

// Add a string to the control
void CLTGUITextItemCtrl::AddString(HSTRING hString)
{
	if (!hString)
		return;

	if (!m_pClientDE)
		return;

	HSTRING hStringCopy = m_pClientDE->CopyString(hString);
	m_stringArray.Append(hStringCopy);

	if (m_nSelIndex < 0)
	{
		m_nSelIndex = 0;
		CalculateSize();
	}
}

// Add a string to the control from a message code
void CLTGUITextItemCtrl::AddString(int messageCode)
{
	if (!m_pClientDE)
		return;

	HSTRING hString = m_pClientDE->FormatString(messageCode);
	AddString(hString);
	m_pClientDE->FreeString(hString);
}

// Return a string at a specific index
HSTRING CLTGUITextItemCtrl::GetString(int nIndex)
{
	return m_stringArray[nIndex];
}

// Remove a string at a specific index
void CLTGUITextItemCtrl::RemoveString(int nIndex)
{
	if (m_pClientDE)
		m_pClientDE->FreeString(m_stringArray[nIndex]);

	m_stringArray.Remove(nIndex);

	if (m_nSelIndex >= (int)m_stringArray.GetSize())
	{
		m_nSelIndex = m_stringArray.GetSize()-1;
		CalculateSize();
	}
	else if (m_nSelIndex > nIndex)
	{
		m_nSelIndex--;
		CalculateSize();
	}
}

// Remove all strings
void CLTGUITextItemCtrl::RemoveAll()
{
	while (m_stringArray.GetSize() > 0)
	{
		RemoveString(0);
	}
}

// Sets the currently selected index
void CLTGUITextItemCtrl::SetSelIndex(int nIndex)
{
	m_nSelIndex = nIndex;
	CalculateSize();
}

// Set the font
LTBOOL CLTGUITextItemCtrl::SetFont(CLTGUIFont *pFont)
{
	if (!pFont)
		return LTFALSE;

	m_pFont = pFont;
	CalculateSize();

	return LTTRUE;
}

// Width/Height calculations
void CLTGUITextItemCtrl::CalculateSize()
{
	if (!m_pFont || m_nSelIndex < 0 || m_nSelIndex >= (int)m_stringArray.GetSize())
	{
		if (!m_bFixedWidth)
			m_dwWidth = 0;
		m_dwHeight = 0;
		return;
	}

	// One pt and one shared m_dwHeight store after the if/else: two
	// branch-local pts, each storing m_dwHeight itself, allocate the else
	// branch's loads differently from shipped.
	LTIntPt pt;
	if (m_bFixedWidth)
	{
		pt = m_pFont->GetTextExtentsFormat(m_stringArray[m_nSelIndex], m_dwWidth);
	}
	else
	{
		pt = m_pFont->GetTextExtents(m_stringArray[m_nSelIndex]);
		m_dwWidth = pt.x;
	}
	m_dwHeight = pt.y;
}

// Render the control
void CLTGUITextItemCtrl::Render(HSURFACE hDestSurf)
{
	if (!m_pFont)
		return;

	if (m_nSelIndex < 0)
		return;

	if (m_stringArray.GetSize() == 0)
		return;

	HLTCOLOR color = GetCurrentColor();

	if (m_bFixedWidth)
	{
		m_pFont->DrawFormat(m_stringArray[m_nSelIndex], hDestSurf, m_pos.x, m_pos.y, m_dwWidth, color);
	}
	else
	{
		m_pFont->Draw(m_stringArray[m_nSelIndex], hDestSurf, m_pos.x, m_pos.y, LTF_JUSTIFY_LEFT, color);
	}
}

// Handle the enter key
LTBOOL CLTGUITextItemCtrl::OnEnter()
{
	if (m_pCommandHandler)
	{
		if (m_pCommandHandler->SendCommand(m_dwCommandID, m_dwParam1, m_dwParam2))
			return LTTRUE;
	}

	return LTFALSE;
}

// Left was pressed
LTBOOL CLTGUITextItemCtrl::OnLeft()
{
	if (m_stringArray.GetSize() <= 1)
		return LTFALSE;

	int nIndex = m_nSelIndex-1;
	if (nIndex < 0)
		nIndex = m_stringArray.GetSize()-1;

	if (nIndex != m_nSelIndex)
	{
		SetSelIndex(nIndex);
		CalculateSize();
	}

	return LTTRUE;
}

// Right was pressed
LTBOOL CLTGUITextItemCtrl::OnRight()
{
	if (m_stringArray.GetSize() <= 1)
		return LTFALSE;

	int nIndex = m_nSelIndex+1;
	if (nIndex >= (int)m_stringArray.GetSize())
		nIndex = 0;

	if (nIndex != m_nSelIndex)
	{
		SetSelIndex(nIndex);
		CalculateSize();
	}

	return LTTRUE;
}

// The selection for this control has changed
void CLTGUITextItemCtrl::OnSelChange()
{
	if (!m_bSelected)
		StartFade();
}
