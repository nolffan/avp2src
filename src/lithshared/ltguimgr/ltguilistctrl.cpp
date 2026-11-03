// LTGUIListCtrl.cpp: implementation of the CLTGUIListCtrl class.
//
// NOTE: Render, GetWidth, GetLastDisplayedIndex and GetControlUnderPoint
// follow the shape of the game's own CListCtrl (src/game/ClientShellDLL/
// ListCtrl.cpp), which was derived from this code: an `LTIntPt size` local
// for the control extents, `unsigned int i` declared before the loop, and a
// positive fit test with the break/return in the else. Written with plain
// int locals and an early-out the four differ in register allocation.
//
//////////////////////////////////////////////////////////////////////

#include "ltguimgr.h"
#include "ltguilistctrl.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CLTGUIListCtrl::CLTGUIListCtrl()
{
	m_nCurrentIndex			= 0;
	m_nFirstDisplayedItem	= 0;
	m_bScrollWrap			= LTTRUE;
	m_bEnableBoxFormat		= LTTRUE;
	m_bEnableMouseMoveSelect	= LTFALSE;
	m_nAlign				= LTF_JUSTIFY_LEFT;
	m_nHeight				= 100;
	m_nItemSpacing			= 10;
	m_nMouseDownItemSel		= -1;
}

CLTGUIListCtrl::~CLTGUIListCtrl()
{
	Destroy();
}

// Creation
LTBOOL CLTGUIListCtrl::Create(int nHeight)
{
	m_nHeight = nHeight;
	m_bCreated = LTTRUE;

	return LTTRUE;
}

// Destroy the control
void CLTGUIListCtrl::Destroy()
{
	m_controlArray.Term();
	m_bCreated = LTFALSE;
}

// Render the control
void CLTGUIListCtrl::Render(HSURFACE hDestSurf)
{
	if (m_bEnableBoxFormat)
	{
		LTIntPt size;
		int y = m_pos.y;

		unsigned int i;
		for (i = m_nFirstDisplayedItem; i < m_controlArray.GetSize(); i++)
		{
			size.x = m_controlArray[i]->GetWidth();
			size.y = m_controlArray[i]->GetHeight();

			if (y + size.y < m_pos.y + m_nHeight)
			{
				int x = m_pos.x;
				switch (m_nAlign)
				{
				case LTF_JUSTIFY_RIGHT:
					x -= size.x;
					break;
				case LTF_JUSTIFY_CENTER:
					x += -(size.x/2);
					break;
				}

				m_controlArray[i]->SetPos(x + m_controlArray[i]->GetBoxDrawAdjustX(), y);

				m_controlArray[i]->Render(hDestSurf);

				y += size.y + m_nItemSpacing;
			}
			else
			{
				break;
			}
		}
	}
	else
	{
		for (uint32 i = 0; i < m_controlArray.GetSize(); i++)
		{
			m_controlArray[i]->Render(hDestSurf);
		}
	}
}

// Width/Height calculations
int CLTGUIListCtrl::GetWidth()
{
	int x = 0;
	int y = 0;

	LTIntPt size;

	unsigned int i;
	for (i = 0; i < m_controlArray.GetSize(); i++)
	{
		size.x = m_controlArray[i]->GetWidth();
		size.y = m_controlArray[i]->GetHeight();

		if (y + size.y < m_nHeight)
		{
			if (size.x > x)
			{
				x = size.x;
			}
			y += size.y + m_nItemSpacing;
		}
		else
		{
			return x;
		}
	}
	return x;
}

int CLTGUIListCtrl::GetHeight()
{
	return m_nHeight;
}

// Call this if you wish to enable highlighting the item that is under the mouse cursor
void CLTGUIListCtrl::EnableMouseMoveSelect(LTBOOL bEnable)
{
	m_bEnableMouseMoveSelect = bEnable;
}

// Handle a keypress
LTBOOL CLTGUIListCtrl::HandleKeyDown(int key, int rep)
{
	if (CLTGUICtrl::HandleKeyDown(key, rep))
		return LTTRUE;

	if (m_nCurrentIndex < (int)m_controlArray.GetSize())
		return m_controlArray[m_nCurrentIndex]->HandleKeyDown(key, rep);

	return LTFALSE;
}

LTBOOL CLTGUIListCtrl::OnUp()
{
	int nNewSel = m_nCurrentIndex;

	int nSel = m_nCurrentIndex-1;
	while (nSel != m_nCurrentIndex)
	{
		if (nSel < 0)
		{
			if (!m_bScrollWrap)
				return LTFALSE;

			if (m_controlArray.GetSize() <= 2)
				return LTFALSE;

			nSel = m_controlArray.GetSize()-1;
		}

		if (m_controlArray[nSel]->IsEnabled())
		{
			nNewSel = nSel;
			break;
		}

		nSel--;
	}

	if (nNewSel != m_nCurrentIndex)
	{
		SelectItem(nNewSel);
		return LTTRUE;
	}

	return LTFALSE;
}

LTBOOL CLTGUIListCtrl::OnDown()
{
	int nNewSel = m_nCurrentIndex;

	int nSel = m_nCurrentIndex+1;
	while (nSel != m_nCurrentIndex)
	{
		if (nSel >= (int)m_controlArray.GetSize())
		{
			if (!m_bScrollWrap)
				return LTFALSE;

			if (m_controlArray.GetSize() <= 2)
				return LTFALSE;

			nSel = 0;
		}

		if (m_controlArray[nSel]->IsEnabled())
		{
			nNewSel = nSel;
			break;
		}

		nSel++;
	}

	if (nNewSel != m_nCurrentIndex)
	{
		SelectItem(nNewSel);
		return LTTRUE;
	}

	return LTFALSE;
}

LTBOOL CLTGUIListCtrl::OnLeft()
{
	if (m_nCurrentIndex < (int)m_controlArray.GetSize())
		return m_controlArray[m_nCurrentIndex]->OnLeft();

	return LTFALSE;
}

LTBOOL CLTGUIListCtrl::OnRight()
{
	if (m_nCurrentIndex < (int)m_controlArray.GetSize())
		return m_controlArray[m_nCurrentIndex]->OnRight();

	return LTFALSE;
}

LTBOOL CLTGUIListCtrl::OnEnter()
{
	if (m_nCurrentIndex < (int)m_controlArray.GetSize())
		return m_controlArray[m_nCurrentIndex]->OnEnter();

	return LTFALSE;
}

LTBOOL CLTGUIListCtrl::OnLButtonDown(int x, int y)
{
	int nControlIndex = 0;
	CLTGUICtrl *pCtrl = GetControlUnderPoint(x, y, &nControlIndex);
	if (pCtrl)
	{
		if (pCtrl->IsEnabled())
		{
			SelectItem(nControlIndex);

			m_nMouseDownItemSel = nControlIndex;

			m_controlArray[nControlIndex]->OnLButtonDown(x, y);

			return LTTRUE;
		}
	}
	else
	{
		m_nMouseDownItemSel = -1;
	}

	return LTFALSE;
}

LTBOOL CLTGUIListCtrl::OnLButtonUp(int x, int y)
{
	int nControlIndex = 0;
	CLTGUICtrl *pCtrl = GetControlUnderPoint(x, y, &nControlIndex);
	if (pCtrl)
	{
		if (pCtrl->IsEnabled())
		{
			if (nControlIndex == m_nMouseDownItemSel)
			{
				return m_controlArray[nControlIndex]->OnLButtonUp(x, y);
			}
		}
	}
	else
	{
		m_nMouseDownItemSel = -1;
	}

	return LTFALSE;
}

LTBOOL CLTGUIListCtrl::OnMouseMove(int x, int y)
{
	if (m_bEnableMouseMoveSelect)
	{
		int nControlIndex = 0;
		CLTGUICtrl *pCtrl = GetControlUnderPoint(x, y, &nControlIndex);
		if (pCtrl && pCtrl->IsEnabled())
		{
			if (m_nCurrentIndex != nControlIndex)
			{
				SelectItem(nControlIndex);
				return LTTRUE;
			}
		}
	}

	return LTFALSE;
}

// Add a control to the array
int CLTGUIListCtrl::AddControl(CLTGUICtrl *pControl)
{
	m_controlArray.Append(pControl);

	int nIndex = m_controlArray.GetSize()-1;

	if (nIndex == m_nCurrentIndex)
		m_controlArray[m_nCurrentIndex]->Select(LTTRUE);

	return nIndex;
}

// Remove a control
void CLTGUIListCtrl::RemoveControl(CLTGUICtrl *pControl)
{
	for (uint32 i = 0; i < m_controlArray.GetSize(); i++)
	{
		if (m_controlArray[i] == pControl)
		{
			m_controlArray.Remove(i);
			return;
		}
	}
}

void CLTGUIListCtrl::RemoveControl(int nIndex)
{
	m_controlArray.Remove(nIndex);
}

void CLTGUIListCtrl::RemoveAllControls()
{
	while (GetNum() > 0)
	{
		RemoveControl(0);
	}
}

CLTGUICtrl *CLTGUIListCtrl::GetControl(int nIndex)
{
	return m_controlArray[nIndex];
}

// Select an item
void CLTGUIListCtrl::SelectItem(int nIndex)
{
	// Deselect the current item
	if (m_nCurrentIndex < (int)m_controlArray.GetSize())
		m_controlArray[m_nCurrentIndex]->Select(LTFALSE);

	if (m_controlArray.GetSize() == 0)
	{
		m_nFirstDisplayedItem = 0;
		m_nCurrentIndex = 0;
		return;
	}

	m_nCurrentIndex = nIndex;
	m_controlArray[nIndex]->Select(LTTRUE);

	// Scroll up if necessary
	if (m_nCurrentIndex < m_nFirstDisplayedItem)
	{
		while (m_nCurrentIndex < m_nFirstDisplayedItem)
			m_nFirstDisplayedItem--;

		return;
	}

	// Scroll down if necessary
	if (GetLastDisplayedIndex(m_nFirstDisplayedItem) < m_nCurrentIndex)
	{
		while (GetLastDisplayedIndex(m_nFirstDisplayedItem) < m_nCurrentIndex)
		{
			if (m_nFirstDisplayedItem >= (int)m_controlArray.GetSize())
				return;

			m_nFirstDisplayedItem++;
		}
	}
}

// Returns the last displayed index while in box format
int CLTGUIListCtrl::GetLastDisplayedIndex(int nStartIndex)
{
	LTIntPt size;
	int y = m_pos.y;

	unsigned int i;
	for (i = nStartIndex; i < m_controlArray.GetSize(); i++)
	{
		size.x = m_controlArray[i]->GetWidth();
		size.y = m_controlArray[i]->GetHeight();

		if (y + size.y < m_pos.y + m_nHeight)
		{
			y += size.y + m_nItemSpacing;
		}
		else
		{
			return i-1;
		}
	}

	return m_controlArray.GetSize()-1;
}

// Gets the index of the control that is under the specific screen point
CLTGUICtrl *CLTGUIListCtrl::GetControlUnderPoint(int xPos, int yPos, int *pnIndex)
{
	// The first and last item to be checked
	int nFirstItem=0;
	int nLastItem=m_controlArray.GetSize()-1;

	if (m_bEnableBoxFormat)
	{
		nFirstItem=m_nFirstDisplayedItem;
		nLastItem=GetLastDisplayedIndex(m_nFirstDisplayedItem);
	}

	// See if the user clicked on any of the controls.
	int i;
	for (i=nFirstItem;i <= nLastItem; i++)
	{
		int nLeft=m_controlArray[i]->GetPos().x;
		int nTop=m_controlArray[i]->GetPos().y;
		int nRight=nLeft+m_controlArray[i]->GetWidth();
		int nBottom=nTop+m_controlArray[i]->GetHeight();

		// Check to see if the click is in the bounding box for the control
		if (xPos >= nLeft && xPos <= nRight && yPos >= nTop && yPos <= nBottom)
		{
			*pnIndex=i;

			return m_controlArray[i];
		}
	}

	return LTNULL;
}
