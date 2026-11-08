//*********************************************************************************
// LithTrackBarCamera.cpp
//
// LithTrackMgr, as AvP2's client links it (LithTrackMgrClient.lib), written
// from the release's prebuilt build of it.
//*********************************************************************************

#include "lithtrackdefs.h"
#include "lithtrackmgr.h"

//*********************************************************************************

DBOOL LithTrackBarCamera::Init(INTERFACE *pInterface, LithFont *pFont, void *pData)
{
	LithTrackBarBase::Init(pInterface, pFont, pData);

	m_nNumLines = 4;
	SetupLines(0, 0);

	if(g_pTrackMgr)
	{
		g_pTrackMgr->ControlType() |= GUI_CONTROL_CAMERA;
		m_pButtonItems[CAMERABAR_BUTTON_CONTROL].m_bSelected = DTRUE;
	}

	return DTRUE;
}

//*********************************************************************************

DBOOL LithTrackBarCamera::Update()
{
	if(!LithTrackBarBase::Update())
		return DFALSE;

	if(!memcmp(&g_pTrackMgr->DataCamera(), &m_dCamera, sizeof(DATA_CAMERA)))
		return DFALSE;

	ResetSurface();
	memcpy(&m_dCamera, &g_pTrackMgr->DataCamera(), sizeof(DATA_CAMERA));

	return DTRUE;
}

//*********************************************************************************

DBOOL LithTrackBarCamera::UpdateResolution(DDWORD nWidth, DDWORD nHeight)
{
	SetupLines(nWidth, nHeight);
	return LithTrackBarBase::UpdateResolution(nWidth, nHeight);
}

//*********************************************************************************

DBOOL LithTrackBarCamera::UpdateItems()
{
	if(!m_pInterface) return DFALSE;

	DATA_CAMERA dCamera = g_pTrackMgr->DataCamera();
	char szStr[64];

	sprintf(szStr, TEXT_POSITION, dCamera.m_vPos.x, dCamera.m_vPos.y, dCamera.m_vPos.z);
	UpdateTextItemString(CAMERABAR_TEXT_POSITION, szStr);

	sprintf(szStr, TEXT_ROTATION, dCamera.m_fPitch, dCamera.m_fYaw, dCamera.m_fRoll);
	UpdateTextItemString(CAMERABAR_TEXT_ROTATION, szStr);

	sprintf(szStr, TEXT_FOV, dCamera.m_fFOVX, dCamera.m_fFOVY);
	UpdateTextItemString(CAMERABAR_TEXT_FOV, szStr);

	return DTRUE;
}

//*********************************************************************************

void LithTrackBarCamera::OnMouseClick()
{
	DBYTE byLock = 0;

	switch(m_nButtonFocus)
	{
		case CAMERABAR_BUTTON_CONTROL:
		{
			g_pTrackMgr->ClearControlType();
			m_pButtonItems[m_nButtonFocus].m_bSelected = DTRUE;
			g_pTrackMgr->ControlType() |= GUI_CONTROL_CAMERA;
			break;
		}

		case CAMERABAR_BUTTON_RESET:
		{
			ILTMath *pMathLT = m_pInterface->GetMathLT();
			DRotation rRot;

			if(!(g_pTrackMgr->DataCamera().m_byLockControls & GUI_CONTROL_LOCK_ROT_X))
				g_pTrackMgr->DataCamera().m_fPitch = 0.0f;
			if(!(g_pTrackMgr->DataCamera().m_byLockControls & GUI_CONTROL_LOCK_ROT_Y))
				g_pTrackMgr->DataCamera().m_fYaw = 0.0f;
			if(!(g_pTrackMgr->DataCamera().m_byLockControls & GUI_CONTROL_LOCK_ROT_Z))
				g_pTrackMgr->DataCamera().m_fRoll = 0.0f;

			pMathLT->SetupEuler(rRot, g_pTrackMgr->DataCamera().m_fPitch * (MATH_PI / 180.0f),
				g_pTrackMgr->DataCamera().m_fYaw * (MATH_PI / 180.0f), g_pTrackMgr->DataCamera().m_fRoll * (MATH_PI / 180.0f));
			m_pInterface->SetObjectRotation(g_pTrackMgr->DataCamera().m_hObj, &rRot);
			break;
		}

		case CAMERABAR_BUTTON_LOCK_POS_X:	byLock = GUI_CONTROL_LOCK_POS_X;	break;
		case CAMERABAR_BUTTON_LOCK_POS_Y:	byLock = GUI_CONTROL_LOCK_POS_Y;	break;
		case CAMERABAR_BUTTON_LOCK_POS_Z:	byLock = GUI_CONTROL_LOCK_POS_Z;	break;
		case CAMERABAR_BUTTON_LOCK_ROT_X:	byLock = GUI_CONTROL_LOCK_ROT_X;	break;
		case CAMERABAR_BUTTON_LOCK_ROT_Y:	byLock = GUI_CONTROL_LOCK_ROT_Y;	break;
		case CAMERABAR_BUTTON_LOCK_ROT_Z:	byLock = GUI_CONTROL_LOCK_ROT_Z;	break;
	}

	if(byLock)
	{
		m_pButtonItems[m_nButtonFocus].m_bSelected = !m_pButtonItems[m_nButtonFocus].m_bSelected;

		if(m_pButtonItems[m_nButtonFocus].m_bSelected)
			g_pTrackMgr->DataCamera().m_byLockControls |= byLock;
		else
			g_pTrackMgr->DataCamera().m_byLockControls &= ~byLock;
	}

	ResetSurface();
}

//*********************************************************************************

void LithTrackBarCamera::OnEditModeOn()
{
	m_pButtonItems[CAMERABAR_BUTTON_CONTROL].m_bSelected = DTRUE;
	g_pTrackMgr->ControlType() |= GUI_CONTROL_CAMERA;
	ResetSurface();
}

//*********************************************************************************

void LithTrackBarCamera::OnControlChange()
{
	m_pButtonItems[CAMERABAR_BUTTON_CONTROL].m_bSelected = DFALSE;
	ResetSurface();
}

//*********************************************************************************

void LithTrackBarCamera::SetupLines(int nWidth, int nHeight)
{
	if(!m_pFont) return;

	int nFontHeight = m_pFont->Height();
	int nHalfHeight = nFontHeight >> 1;
	DRect rect;

	rect = CreateTextItem(CAMERABAR_TEXT_TITLE, TITLE_TRACK_CAMERA, m_cTitle, TRACKMGR_GUI_BAR_EDGE_SPACE, TRACKMGR_GUI_BAR_EDGE_SPACE);
	rect = CreateTextItem(CAMERABAR_TEXT_POSITION, TEXT_POSITION, m_cText, rect.right + (nFontHeight * 2), TRACKMGR_GUI_BAR_EDGE_SPACE);
	rect = CreateTextItem(CAMERABAR_TEXT_ROTATION, TEXT_ROTATION, m_cText, rect.left, rect.bottom);
	rect = CreateTextItem(CAMERABAR_TEXT_FOV, TEXT_FOV, m_cText, rect.left, rect.bottom);
	rect = CreateButtonItem(CAMERABAR_BUTTON_RESET, BUTTON_RESET, m_cNormal, rect.left, rect.bottom);

	rect = CreateButtonItem(CAMERABAR_BUTTON_LOCK_ROT_Z, BUTTON_Z, m_cNormal, nWidth - TRACKMGR_GUI_BAR_EDGE_SPACE, rect.top, GUI_JUSTIFY_RIGHT);
	rect = CreateButtonItem(CAMERABAR_BUTTON_LOCK_ROT_Y, BUTTON_Y, m_cNormal, rect.left - nHalfHeight, rect.top, GUI_JUSTIFY_RIGHT);
	rect = CreateButtonItem(CAMERABAR_BUTTON_LOCK_ROT_X, BUTTON_X, m_cNormal, rect.left - nHalfHeight, rect.top, GUI_JUSTIFY_RIGHT);
	rect = CreateTextItem(CAMERABAR_TEXT_LOCK_ROT, TEXT_LOCK_ROT, m_cText, rect.left, rect.top, GUI_JUSTIFY_RIGHT);

	rect = CreateButtonItem(CAMERABAR_BUTTON_CONTROL, BUTTON_CONTROL, m_cNormal, TRACKMGR_GUI_BAR_EDGE_SPACE, rect.top);

	rect = CreateButtonItem(CAMERABAR_BUTTON_LOCK_POS_Z, BUTTON_Z, m_cNormal, nWidth - TRACKMGR_GUI_BAR_EDGE_SPACE, rect.top - nFontHeight, GUI_JUSTIFY_RIGHT);
	rect = CreateButtonItem(CAMERABAR_BUTTON_LOCK_POS_Y, BUTTON_Y, m_cNormal, rect.left - nHalfHeight, rect.top, GUI_JUSTIFY_RIGHT);
	rect = CreateButtonItem(CAMERABAR_BUTTON_LOCK_POS_X, BUTTON_X, m_cNormal, rect.left - nHalfHeight, rect.top, GUI_JUSTIFY_RIGHT);
	rect = CreateTextItem(CAMERABAR_TEXT_LOCK_POS, TEXT_LOCK_POS, m_cText, rect.left, rect.top, GUI_JUSTIFY_RIGHT);
}
