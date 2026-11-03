// ----------------------------------------------------------------------- //
//
// MODULE  : LTGUICtrl.cpp
//
// PURPOSE : Base class for controls
//
// ----------------------------------------------------------------------- //

#include "ltguimgr.h"
#include "ltguictrl.h"

LTBOOL CLTGUICtrl::s_bShiftState = LTFALSE;
LTBOOL CLTGUICtrl::s_bCapsState = LTFALSE;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CLTGUICtrl::CLTGUICtrl()
{
	m_pClientDE = LTNULL;
	m_bSelected = LTFALSE;
	m_bCreated = LTFALSE;
	m_dwCommandID = 0;
	m_dwParam1 = 0;
	m_dwParam2 = 0;
	m_pos.x = 0;
	m_pos.y = 0;
	m_dwHelpID = 0;
	m_xBoxDrawAdj = 0;
	m_bEnabled = LTTRUE;
	m_hTransColor = SETRGB_T(0, 0, 0);
}

CLTGUICtrl::~CLTGUICtrl()
{
}

// Destruction
void CLTGUICtrl::Destroy()
{
}

// Reset the animation of the control
void CLTGUICtrl::ResetAnimation()
{
}

// Handles a key press
LTBOOL CLTGUICtrl::HandleKeyDown(int key, int rep)
{
	return LTFALSE;
}

// Handles a character input
LTBOOL CLTGUICtrl::HandleChar(char c)
{
	return LTFALSE;
}

LTBOOL CLTGUICtrl::OnLeft()
{
	return LTFALSE;
}

LTBOOL CLTGUICtrl::OnRight()
{
	return LTFALSE;
}

LTBOOL CLTGUICtrl::OnUp()
{
	return LTFALSE;
}

LTBOOL CLTGUICtrl::OnDown()
{
	return LTFALSE;
}

LTBOOL CLTGUICtrl::OnEnter()
{
	return LTFALSE;
}

LTBOOL CLTGUICtrl::OnLButtonDown(int x, int y)
{
	return LTFALSE;
}

LTBOOL CLTGUICtrl::OnLButtonUp(int x, int y)
{
	return LTFALSE;
}

LTBOOL CLTGUICtrl::OnLButtonDblClick(int x, int y)
{
	return LTFALSE;
}

LTBOOL CLTGUICtrl::OnRButtonDown(int x, int y)
{
	return LTFALSE;
}

LTBOOL CLTGUICtrl::OnRButtonUp(int x, int y)
{
	return LTFALSE;
}

LTBOOL CLTGUICtrl::OnRButtonDblClick(int x, int y)
{
	return LTFALSE;
}

LTBOOL CLTGUICtrl::OnMouseMove(int x, int y)
{
	return LTFALSE;
}

// Handle a command
void CLTGUICtrl::OnCommand(uint32 dwCommandID)
{
}

// Select a control
void CLTGUICtrl::Select(LTBOOL bSelected)
{
	if (m_bSelected != bSelected)
	{
		m_bSelected = bSelected;
		OnSelChange();
	}
}

// Update data
void CLTGUICtrl::UpdateData(LTBOOL bSaveAndValidate)
{
}
