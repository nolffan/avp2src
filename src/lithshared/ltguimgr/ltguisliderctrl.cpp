// LTGUISliderCtrl.cpp: implementation of the CLTGUISliderCtrl class.
//
//////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include "ltguimgr.h"
#include "ltguisliderctrl.h"
#include "ltguifont.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CLTGUISliderCtrl::CLTGUISliderCtrl()
{
	m_bNumDisplay	= LTFALSE;
	m_pFont			= LTNULL;
	m_dwWidth		= 0;
	m_dwHeight		= 0;
	m_hBarSurf		= LTNULL;
	m_hTabSurf		= LTNULL;
	m_nMinSlider	= 0;
	m_nSliderPos	= 0;
	m_hText			= LTNULL;
	m_nSliderOffset	= 0;
	m_nAlignment	= LTF_JUSTIFY_LEFT;
	m_nMaxSlider	= 100;
	m_nSliderIncrement	= 10;
	m_nSlideEdgeSpace	= 5;
	m_nNumFonts		= 1;
}

CLTGUISliderCtrl::~CLTGUISliderCtrl()
{
	Destroy();
}

// Create the control
// The statement order is load-bearing: VC6 reorders these stores, and only
// this source order yields the shipped store order (pnValue, pClientDE,
// bNumDisplay, ...) and the ebx zero register. Likewise the width is assigned
// before the height.
LTBOOL CLTGUISliderCtrl::Create(ILTClient *pClientDE, HSTRING hText, CLTGUIFont *pFont, int nSliderOffset,
								HSURFACE hBarSurf, HSURFACE hTabSurf, LTBOOL bNumDisplay, int *pnValue)
{
	m_pClientDE = pClientDE;
	m_hBarSurf = hBarSurf;
	m_hTabSurf = hTabSurf;
	m_nSliderOffset = nSliderOffset;
	m_pnValue = pnValue;
	m_bNumDisplay = bNumDisplay;

	if (!pClientDE || !hBarSurf || !hTabSurf)
		return LTFALSE;

	uint32 nWidth = 0, nHeight = 0;
	m_pClientDE->GetSurfaceDims(hBarSurf, &nWidth, &nHeight);

	m_dwWidth = nWidth + nSliderOffset;
	m_dwHeight = nHeight;

	SetFont(pFont);
	SetText(hText);

	return LTTRUE;
}

// Destroys the control
void CLTGUISliderCtrl::Destroy()
{
	if (m_hText && m_pClientDE)
	{
		m_pClientDE->FreeString(m_hText);
		m_hText = LTNULL;
	}
}

// Update data
void CLTGUISliderCtrl::UpdateData(LTBOOL bSaveAndValidate)
{
	if (!m_pnValue)
		return;

	if (bSaveAndValidate)
		*m_pnValue = m_nSliderPos;
	else
		SetSliderPos(*m_pnValue);
}

// Sets the min and max of the slider bar
void CLTGUISliderCtrl::SetSliderRange(int nMin, int nMax)
{
	m_nMinSlider = nMin;
	m_nMaxSlider = nMax;

	SetSliderPos(m_nSliderPos);
}

// Sets the position of the slider bar
void CLTGUISliderCtrl::SetSliderPos(int nPos)
{
	m_nSliderPos = nPos;

	if (m_nSliderPos > m_nMaxSlider)
		m_nSliderPos = m_nMaxSlider;
	if (m_nSliderPos < m_nMinSlider)
		m_nSliderPos = m_nMinSlider;
}

// Sets the text for the slider bar
void CLTGUISliderCtrl::SetText(HSTRING hString)
{
	if (m_hText && m_pClientDE)
	{
		m_pClientDE->FreeString(m_hText);
		m_hText = LTNULL;
	}

	if (hString && m_pClientDE)
	{
		m_hText = m_pClientDE->CopyString(hString);
	}
}

void CLTGUISliderCtrl::SetText(int messageCode)
{
	HSTRING hString = m_pClientDE->FormatString(messageCode);
	SetText(hString);
	m_pClientDE->FreeString(hString);
}

// Render the control
void CLTGUISliderCtrl::Render(HSURFACE hDestSurf)
{
	RenderSliderText(hDestSurf);
	RenderSliderBar(hDestSurf);
}

// Render the slider text
void CLTGUISliderCtrl::RenderSliderText(HSURFACE hDestSurf)
{
	if (!m_pFont)
		return;

	if (!m_hText)
		return;

	int x = m_pos.x;
	int y = m_pos.y;

	if (m_nAlignment == LTF_JUSTIFY_RIGHT)
		x = m_nSliderOffset + m_pos.x - 25;

	HLTCOLOR color = GetCurrentColor();

	m_pFont->Draw(m_hText, hDestSurf, x, y, m_nAlignment, color);

	if (m_bNumDisplay)
		DrawNumDisplay(hDestSurf);
}

// Draw the numeric display
void CLTGUISliderCtrl::DrawNumDisplay(HSURFACE hDestSurf)
{
	int x = m_pos.x + GetWidth() + 4;
	int y = m_pos.y;

	char szStr[8] = "";
	sprintf(szStr, "%d", m_nSliderPos);

	HLTCOLOR color = GetCurrentColor();

	m_pFont->Draw(szStr, hDestSurf, x, y, LTF_JUSTIFY_LEFT, color);
}

// Render the slider bar
void CLTGUISliderCtrl::RenderSliderBar(HSURFACE hDestSurf)
{
	if (!m_hBarSurf)
		return;

	if (!m_hTabSurf)
		return;

	// Draw the bar
	m_pClientDE->DrawSurfaceToSurfaceTransparent(hDestSurf, m_hBarSurf, LTNULL,
												 m_pos.x + m_nSliderOffset, m_pos.y, m_hTransColor);

	// Draw the tab
	uint32 nBarWidth = 0, nBarHeight = 0;
	uint32 nTabWidth = 0, nTabHeight = 0;
	m_pClientDE->GetSurfaceDims(m_hBarSurf, &nBarWidth, &nBarHeight);
	m_pClientDE->GetSurfaceDims(m_hTabSurf, &nTabWidth, &nTabHeight);

	int x = (int)(((LTFLOAT)(m_nSliderPos - m_nMinSlider) / (m_nMaxSlider - m_nMinSlider)) * (int)(nBarWidth - m_nSlideEdgeSpace*2 - nTabWidth)) + m_pos.x + m_nSliderOffset + m_nSlideEdgeSpace;

	m_pClientDE->DrawSurfaceToSurfaceTransparent(hDestSurf, m_hTabSurf, LTNULL, x, m_pos.y, m_hTransColor);
}

// Set the font
void CLTGUISliderCtrl::SetFont(CLTGUIFont *pFont)
{
	m_pFont = pFont;
}

// Handle left and right presses
LTBOOL CLTGUISliderCtrl::OnLeft()
{
	int nPos = m_nSliderPos - m_nSliderIncrement;
	if (nPos < m_nMinSlider)
		nPos = m_nMinSlider;

	if (nPos != m_nSliderPos)
	{
		SetSliderPos(nPos);
		return LTTRUE;
	}

	return LTFALSE;
}

LTBOOL CLTGUISliderCtrl::OnRight()
{
	int nPos = m_nSliderPos + m_nSliderIncrement;
	if (nPos > m_nMaxSlider)
		nPos = m_nMaxSlider;

	if (nPos != m_nSliderPos)
	{
		SetSliderPos(nPos);
		return LTTRUE;
	}

	return LTFALSE;
}

// The selection for this control has changed
void CLTGUISliderCtrl::OnSelChange()
{
	if (!m_bSelected)
		StartFade();
}

// The shipped code reads m_pos.x and m_pos.y into registers before the
// virtual GetWidth/GetHeight calls and re-reads m_pos.x after GetWidth, so the
// left/top edges are copies taken up front and the right edge reads m_pos.x.
LTBOOL CLTGUISliderCtrl::OnLButtonUp(int x, int y)
{
	int nLeft = m_pos.x;
	int nTop = m_pos.y;
	int nRight = m_pos.x + GetWidth();
	int nBottom = nTop + GetHeight();

	if (x < nLeft || x > nRight || y < nTop || y > nBottom)
		return LTFALSE;

	if (x < nLeft + m_nSliderOffset)
		return LTTRUE;

	if (x < CalculateTabPos())
	{
		return OnLeft();
	}

	return OnRight();
}

LTBOOL CLTGUISliderCtrl::OnRButtonUp(int x, int y)
{
	int nLeft = m_pos.x;
	int nTop = m_pos.y;
	int nRight = m_pos.x + GetWidth();
	int nBottom = nTop + GetHeight();

	if (x < nLeft || x > nRight || y < nTop || y > nBottom)
		return LTFALSE;

	return LTTRUE;
}

// Calculate the x position of the tab
int CLTGUISliderCtrl::CalculateTabPos()
{
	uint32 nBarWidth = 0, nBarHeight = 0;
	uint32 nTabWidth = 0, nTabHeight = 0;
	m_pClientDE->GetSurfaceDims(m_hBarSurf, &nBarWidth, &nBarHeight);
	m_pClientDE->GetSurfaceDims(m_hTabSurf, &nTabWidth, &nTabHeight);

	return (int)(((LTFLOAT)(m_nSliderPos - m_nMinSlider) / (m_nMaxSlider - m_nMinSlider)) * (int)(nBarWidth - m_nSlideEdgeSpace*2 - nTabWidth)) + m_nSliderOffset + m_pos.x + m_nSlideEdgeSpace;
}
