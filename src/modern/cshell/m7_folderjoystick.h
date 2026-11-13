// ----------------------------------------------------------------------- //
//
// MODULE  : m7_folderjoystick.h
//
// PURPOSE : The modern build's controller (docs/MODERN.md, M-7): Options,
//           Controls, "Controller", the folder that was Joystick.
//           FolderJoystick.cpp is this in the modern build.
//
//           Retail's Joystick page set up DirectInput joystick axes (use
//           the joystick, then each of four axes: which, invert, dead zone,
//           analog, sensitivity, centring).  There is no DirectInput
//           joystick now, and binding the pad's sticks as axes would move
//           the player a second time (the client shell reads them:
//           m7_pad.h), so the page is the pad's own settings instead:
//
//             Look speed      PadLookSpeed, 25..300%
//             Invert look     PadInvertY
//             Look dead zone  PadLookDeadZone, 0..50%
//             Move dead zone  PadMoveDeadZone, 0..50%
//             Restore defaults   those four, and the pad's default
//                                bindings (m7_padbindings.h) in place of
//                                its column of the profile
//
//           Its buttons are bound in Customize Controls like keys.  The
//           same page, the same place in the menus, and the same layout
//           (LAYOUT.txt's [FolderJoystick]: ColumnWidth, SliderWidth; the
//           Keyboard page's RestorePos, as retail's).
//
//           Modern build only (AVP2SRC_MODERN).
//
// ----------------------------------------------------------------------- //

#ifndef __M7_FOLDERJOYSTICK_H__
#define __M7_FOLDERJOYSTICK_H__

#include "m7_pad.h"
#include "m7_padbindings.h"

namespace
{
	int		g_nM7Width = 0;
	int		g_nM7Slider = 0;

	// The controls' values: look speed in 25% steps (1..12), dead zones in
	// 5% steps (0..10)
	int		g_nM7LookSpeed = 4;
	LTBOOL	g_bM7InvertLook = LTFALSE;
	int		g_nM7LookDeadZone = 3;
	int		g_nM7MoveDeadZone = 4;

	int M7Steps(LTFLOAT fValue, LTFLOAT fStep, int nMin, int nMax)
	{
		int n = (int)(fValue / fStep + 0.5f);
		return n < nMin ? nMin : n > nMax ? nMax : n;
	}

	void M7ReadVars()
	{
		g_nM7LookSpeed    = M7Steps(M7Pad_GetVar(M7_VAR_LOOKSPEED, M7_DEF_LOOKSPEED), 0.25f, 1, 12);
		g_bM7InvertLook   = M7Pad_GetVar(M7_VAR_INVERTY, M7_DEF_INVERTY) != 0.0f;
		g_nM7LookDeadZone = M7Steps(M7Pad_GetVar(M7_VAR_LOOKDEADZONE, M7_DEF_LOOKDEADZONE), 0.05f, 0, 10);
		g_nM7MoveDeadZone = M7Steps(M7Pad_GetVar(M7_VAR_MOVEDEADZONE, M7_DEF_MOVEDEADZONE), 0.05f, 0, 10);
	}

	void M7WriteVars()
	{
		M7Pad_SetVar(M7_VAR_LOOKSPEED, g_nM7LookSpeed * 0.25f);
		M7Pad_SetVar(M7_VAR_INVERTY, g_bM7InvertLook ? 1.0f : 0.0f);
		M7Pad_SetVar(M7_VAR_LOOKDEADZONE, g_nM7LookDeadZone * 0.05f);
		M7Pad_SetVar(M7_VAR_MOVEDEADZONE, g_nM7MoveDeadZone * 0.05f);
	}
}

CFolderJoystick::CFolderJoystick()
{
	m_bUseJoystick = 0;
	m_pAxisCycle = LTNULL;
	m_pInvertToggle = LTNULL;
	m_pDeadZoneSlider = LTNULL;
	m_pAnalogToggle = LTNULL;
	m_pSensitivitySlider = LTNULL;
	m_pCenterToggle = LTNULL;
	m_pFixedToggle = LTNULL;
	m_bFixedPositionLook = LTFALSE;
	m_dwCurrentAxis = 0;
}

CFolderJoystick::~CFolderJoystick()
{
}

LTBOOL CFolderJoystick::Build()
{
	CSliderCtrl* pSlider;
	CToggleCtrl* pToggle;

	g_nM7Width  = g_pLayoutMgr->GetFolderCustomInt(FOLDER_ID_JOYSTICK,"ColumnWidth");
	g_nM7Slider = g_pLayoutMgr->GetFolderCustomInt(FOLDER_ID_JOYSTICK,"SliderWidth");
	g_pLayoutMgr->GetFolderCustomString((eFolderID)m_nFolderID,"TabSound",m_szTabSound,sizeof(m_szTabSound));

	CreateTitle(IDS_TITLE_CONTROLS);

	pSlider = AddSlider(IDS_M7_LOOKSPEED, IDS_HELP_M7_LOOKSPEED, g_nM7Width, g_nM7Slider, &g_nM7LookSpeed);
	pSlider->SetSliderRange(1, 12);
	pSlider->SetSliderIncrement(1);

	pToggle = AddToggle(IDS_M7_INVERTLOOK, IDS_HELP_M7_INVERTLOOK, g_nM7Width, &g_bM7InvertLook);
	pToggle->SetOnString(IDS_YES);
	pToggle->SetOffString(IDS_NO);

	pSlider = AddSlider(IDS_M7_LOOKDEADZONE, IDS_HELP_M7_LOOKDEADZONE, g_nM7Width, g_nM7Slider, &g_nM7LookDeadZone);
	pSlider->SetSliderRange(0, 10);
	pSlider->SetSliderIncrement(1);

	pSlider = AddSlider(IDS_M7_MOVEDEADZONE, IDS_HELP_M7_MOVEDEADZONE, g_nM7Width, g_nM7Slider, &g_nM7MoveDeadZone);
	pSlider->SetSliderRange(0, 10);
	pSlider->SetSliderIncrement(1);

	m_nNextLink = 2;
	AddLink(IDS_CUSTOM_CONTROLS, FOLDER_CMD_CONTROLS, IDS_HELP_CUSTOMCONTROLS);
	AddLink(IDS_MOUSE, FOLDER_CMD_MOUSE, IDS_HELP_MOUSE);
	AddLink(IDS_KEYBOARD, FOLDER_CMD_KEYBOARD, IDS_HELP_KEYBOARD);

	CLTGUITextItemCtrl *pCtrl = AddLink(IDS_JOYSTICK, FOLDER_CMD_JOYSTICK, IDS_HELP_JOYSTICK);
	pCtrl->Enable(LTFALSE);

	LTIntPt pos = g_pLayoutMgr->GetFolderCustomPoint(FOLDER_ID_KEYBOARD,"RestorePos");
	pCtrl = CreateTextItem(IDS_RESTOREDEFAULTS, FOLDER_CMD_RESET_DEFAULTS, IDS_HELP_M7_RESTORE, LTFALSE, GetSmallFont());
	AddFixedControl(pCtrl,kNextGroup,pos);

	if (! CBaseFolder::Build()) return LTFALSE;

	return LTTRUE;
}

uint32 CFolderJoystick::OnCommand(uint32 dwCommand, uint32 dwParam1, uint32 dwParam2)
{
	if (dwCommand == FOLDER_CMD_RESET_DEFAULTS)
	{
		CUserProfile *pProfile = g_pProfileMgr->GetCurrentProfile();

		g_nM7LookSpeed    = M7Steps(M7_DEF_LOOKSPEED, 0.25f, 1, 12);
		g_bM7InvertLook   = M7_DEF_INVERTY != 0.0f;
		g_nM7LookDeadZone = M7Steps(M7_DEF_LOOKDEADZONE, 0.05f, 0, 10);
		g_nM7MoveDeadZone = M7Steps(M7_DEF_MOVEDEADZONE, 0.05f, 0, 10);
		M7WriteVars();

		// The pad's bindings: what the engine has now into the profile,
		// its column the defaults, and back
		if (pProfile)
		{
			pProfile->SetBindings();
			M7Pad_DefaultBindings(pProfile, LTTRUE);
			pProfile->ApplyBindings();
			pProfile->Save();
		}

		UpdateData(LTFALSE);
		return 1;
	}

	return CBaseFolder::OnCommand(dwCommand,dwParam1,dwParam2);
}

void CFolderJoystick::OnFocus(LTBOOL bFocus)
{
	if (bFocus)
	{
		M7ReadVars();
		UpdateData(LTFALSE);
	}
	else
	{
		UpdateData();
		M7WriteVars();
		g_pLTClient->WriteConfigFile("autoexec.cfg");
	}

	CBaseFolder::OnFocus(bFocus);
}

void CFolderJoystick::SetCurrentAxis(uint32 dwAxisCmd)
{
}

void CFolderJoystick::Escape()
{
	g_pInterfaceMgr->PlayInterfaceSound(IS_ESCAPE);
	g_pInterfaceMgr->SwitchToFolder(FOLDER_ID_OPTIONS);
}

LTBOOL CFolderJoystick::OnLeft()
{
	return CBaseFolder::OnLeft();
}

LTBOOL CFolderJoystick::OnRight()
{
	return CBaseFolder::OnRight();
}

LTBOOL CFolderJoystick::OnLButtonUp(int x, int y)
{
	return CBaseFolder::OnLButtonUp(x, y);
}

LTBOOL CFolderJoystick::OnRButtonUp(int x, int y)
{
	return CBaseFolder::OnRButtonUp(x, y);
}

char* CFolderJoystick::GetSelectSound()
{
	uint32 dwID = 0;
	CLTGUICtrl *pCtrl = GetSelectedControl();
	if (pCtrl)
		dwID = pCtrl->GetHelpID();
	if (dwID == IDS_HELP_CUSTOMCONTROLS || dwID == IDS_HELP_MOUSE || dwID == IDS_HELP_KEYBOARD || dwID == IDS_HELP_JOYSTICK)
		return m_szTabSound;
	return m_sDefaultSelectSound;
}

#endif // __M7_FOLDERJOYSTICK_H__
