//*********************************************************************************
// LithTrackBarKey.cpp
//
// LithTrackMgr, as AvP2's client links it (LithTrackMgrClient.lib), written
// from the release's prebuilt build of it.
//*********************************************************************************

#include "lithtrackdefs.h"
#include "lithtrackmgr.h"

//*********************************************************************************

DBOOL LithTrackBarKey::Init(INTERFACE *pInterface, LithFont *pFont, void *pData)
{
	LithTrackBarBase::Init(pInterface, pFont, pData);
	SetupLines(0, 0);

	m_pLastTrack = DNULL;
	m_nCurrentKey = 0;
	m_bResetOnUpdate = DTRUE;

	return DTRUE;
}

//*********************************************************************************

DBOOL LithTrackBarKey::Update()
{
	if(!LithTrackBarBase::Update())
		return DFALSE;

	LithTrack *pTrack = g_pTrackMgr->DrawObj().Track();

	if(pTrack)
	{
		// Keep the current key in range, and reset the lines whenever the track changes
		if((m_nCurrentKey < 0) || (m_nCurrentKey >= pTrack->NumKeys()))
		{
			m_nCurrentKey = (m_nCurrentKey < 0) ? 0 : pTrack->NumKeys() - 1;
			g_pTrackMgr->DrawObj().Reset();
			g_pTrackMgr->DrawObj().SelectKey(m_nCurrentKey);
		}
		else if(pTrack != m_pLastTrack)
		{
			m_pLastTrack = pTrack;
			g_pTrackMgr->DrawObj().Reset();
			g_pTrackMgr->DrawObj().SelectKey(m_nCurrentKey);
		}
	}

	UpdateControl();

	if(m_bResetOnUpdate)
	{
		ResetSurface();
		g_pTrackMgr->DrawObj().Update();
		m_bResetOnUpdate = DFALSE;
	}

	return DTRUE;
}

//*********************************************************************************

DBOOL LithTrackBarKey::UpdateResolution(DDWORD nWidth, DDWORD nHeight)
{
	SetupLines(nWidth, nHeight);
	return LithTrackBarBase::UpdateResolution(nWidth, nHeight);
}

//*********************************************************************************

DBOOL LithTrackBarKey::UpdateItems()
{
	if(!m_pInterface) return DFALSE;

	LithTrack *pTrack = g_pTrackMgr->DrawObj().Track();
	char szStr[64];

	if(pTrack)
	{
		sprintf(szStr, TEXT_RANGE, m_nCurrentKey + 1, pTrack->NumKeys());
		UpdateTextItemString(KEYBAR_TEXT_KEY_NUM, szStr);

		DVector vPos = pTrack->Key(m_nCurrentKey)->m_vPos;
		DVector vRot = pTrack->Key(m_nCurrentKey)->m_vRot;

		sprintf(szStr, TEXT_POSITION, vPos.x, vPos.y, vPos.z);
		UpdateTextItemString(KEYBAR_TEXT_KEY_POSITION, szStr);

		sprintf(szStr, TEXT_ROTATION, vRot.x, vRot.y, vRot.z);
		UpdateTextItemString(KEYBAR_TEXT_KEY_ROTATION, szStr);

		sprintf(szStr, TEXT_TIME, pTrack->Key(m_nCurrentKey)->m_fTime);
		UpdateTextItemString(KEYBAR_TEXT_TIME, szStr);

		sprintf(szStr, TEXT_FLEX, pTrack->Key(m_nCurrentKey)->m_fFlex);
		UpdateTextItemString(KEYBAR_TEXT_FLEX, szStr);
	}
	else
	{
		sprintf(szStr, TEXT_RANGE, 0, 0);
		UpdateTextItemString(KEYBAR_TEXT_KEY_NUM, szStr);

		sprintf(szStr, TEXT_POSITION, 0.0f, 0.0f, 0.0f);
		UpdateTextItemString(KEYBAR_TEXT_KEY_POSITION, szStr);

		sprintf(szStr, TEXT_ROTATION, 0.0f, 0.0f, 0.0f);
		UpdateTextItemString(KEYBAR_TEXT_KEY_ROTATION, szStr);

		sprintf(szStr, TEXT_TIME, 0.0f);
		UpdateTextItemString(KEYBAR_TEXT_TIME, szStr);

		sprintf(szStr, TEXT_FLEX, 0.0f);
		UpdateTextItemString(KEYBAR_TEXT_FLEX, szStr);
	}

	// Keep the flex text just after the time text
	int nWidth = m_pTextItems[KEYBAR_TEXT_FLEX].m_rRect.right - m_pTextItems[KEYBAR_TEXT_FLEX].m_rRect.left;
	m_pTextItems[KEYBAR_TEXT_FLEX].m_rRect.left = m_pTextItems[KEYBAR_TEXT_TIME].m_rRect.right + (m_pFont->Height() * 2);
	m_pTextItems[KEYBAR_TEXT_FLEX].m_rRect.right = m_pTextItems[KEYBAR_TEXT_FLEX].m_rRect.left + nWidth;

	return DTRUE;
}

//*********************************************************************************

void LithTrackBarKey::OnMouseClick()
{
	if(!g_pTrackMgr) return;

	LithTrack *pTrack = g_pTrackMgr->DrawObj().Track();
	if(!pTrack) return;

	DBYTE byLock = 0;

	switch(m_nButtonFocus)
	{
		case KEYBAR_BUTTON_INSERT_KEY:
		{
			pTrack->InsertKey(m_nCurrentKey);
			g_pTrackMgr->DrawObj().Reset();
			m_nCurrentKey++;
			g_pTrackMgr->DrawObj().SelectKey(m_nCurrentKey);
			break;
		}

		case KEYBAR_BUTTON_DELETE_KEY:
		{
			pTrack->DeleteKey(m_nCurrentKey);
			g_pTrackMgr->DrawObj().Reset();

			if(m_nCurrentKey >= pTrack->NumKeys())
				m_nCurrentKey = pTrack->NumKeys() - 1;

			g_pTrackMgr->DrawObj().SelectKey(m_nCurrentKey);
			break;
		}

		case KEYBAR_BUTTON_RESET_KEY:
		{
			if(!(g_pTrackMgr->DataKey().m_byLockControls & GUI_CONTROL_LOCK_ROT_X)) pTrack->Key(m_nCurrentKey)->m_vRot.x = 0.0f;
			if(!(g_pTrackMgr->DataKey().m_byLockControls & GUI_CONTROL_LOCK_ROT_Y)) pTrack->Key(m_nCurrentKey)->m_vRot.y = 0.0f;
			if(!(g_pTrackMgr->DataKey().m_byLockControls & GUI_CONTROL_LOCK_ROT_Z)) pTrack->Key(m_nCurrentKey)->m_vRot.z = 0.0f;

			g_pTrackMgr->DrawObj().Update();
			break;
		}

		case KEYBAR_BUTTON_LOCK_POS_X:	byLock = GUI_CONTROL_LOCK_POS_X;	break;
		case KEYBAR_BUTTON_LOCK_POS_Y:	byLock = GUI_CONTROL_LOCK_POS_Y;	break;
		case KEYBAR_BUTTON_LOCK_POS_Z:	byLock = GUI_CONTROL_LOCK_POS_Z;	break;
		case KEYBAR_BUTTON_LOCK_ROT_X:	byLock = GUI_CONTROL_LOCK_ROT_X;	break;
		case KEYBAR_BUTTON_LOCK_ROT_Y:	byLock = GUI_CONTROL_LOCK_ROT_Y;	break;
		case KEYBAR_BUTTON_LOCK_ROT_Z:	byLock = GUI_CONTROL_LOCK_ROT_Z;	break;

		case KEYBAR_BUTTON_PREV_KEY_FAST:
		{
			m_nCurrentKey -= TRACKMGR_DEFAULT_KEY_SELECT_FAST;
			if(m_nCurrentKey < 0) m_nCurrentKey = 0;

			g_pTrackMgr->DrawObj().SelectKey(m_nCurrentKey);
			break;
		}

		case KEYBAR_BUTTON_PREV_KEY:
		{
			m_nCurrentKey--;
			if(m_nCurrentKey < 0) m_nCurrentKey = 0;

			g_pTrackMgr->DrawObj().SelectKey(m_nCurrentKey);
			break;
		}

		case KEYBAR_BUTTON_NEXT_KEY:
		{
			m_nCurrentKey++;
			if(m_nCurrentKey >= pTrack->NumKeys()) m_nCurrentKey = pTrack->NumKeys() - 1;

			g_pTrackMgr->DrawObj().SelectKey(m_nCurrentKey);
			break;
		}

		case KEYBAR_BUTTON_NEXT_KEY_FAST:
		{
			m_nCurrentKey += TRACKMGR_DEFAULT_KEY_SELECT_FAST;
			if(m_nCurrentKey >= pTrack->NumKeys()) m_nCurrentKey = pTrack->NumKeys() - 1;

			g_pTrackMgr->DrawObj().SelectKey(m_nCurrentKey);
			break;
		}

		case KEYBAR_BUTTON_CONTROL_KEY:
		{
			g_pTrackMgr->ClearControlType();
			m_pButtonItems[m_nButtonFocus].m_bSelected = DTRUE;
			g_pTrackMgr->ControlType() |= GUI_CONTROL_KEY;
			break;
		}
	}

	if(byLock)
	{
		m_pButtonItems[m_nButtonFocus].m_bSelected = !m_pButtonItems[m_nButtonFocus].m_bSelected;

		if(m_pButtonItems[m_nButtonFocus].m_bSelected)
			g_pTrackMgr->DataKey().m_byLockControls |= byLock;
		else
			g_pTrackMgr->DataKey().m_byLockControls &= ~byLock;
	}

	ResetSurface();
}

//*********************************************************************************

void LithTrackBarKey::OnControlChange()
{
	m_pButtonItems[KEYBAR_BUTTON_CONTROL_KEY].m_bSelected = DFALSE;
	ResetSurface();
}

//*********************************************************************************

void LithTrackBarKey::SetupLines(int nWidth, int nHeight)
{
	if(!m_pFont) return;

	int nFontHeight = m_pFont->Height();
	int nSpace = nFontHeight * 2;
	int nHalfHeight = nFontHeight >> 1;
	int nRight, nX;
	DRect rect;

	rect = CreateTextItem(KEYBAR_TEXT_TITLE, TITLE_TRACK_KEY_DETAILS, m_cTitle, TRACKMGR_GUI_BAR_EDGE_SPACE, TRACKMGR_GUI_BAR_EDGE_SPACE);
	rect = CreateButtonItem(KEYBAR_BUTTON_CONTROL_KEY, BUTTON_CONTROL, m_cNormal, TRACKMGR_GUI_BAR_EDGE_SPACE, rect.bottom + (nFontHeight * 2) + nFontHeight);

	nX = m_pTextItems[KEYBAR_TEXT_TITLE].m_rRect.right + nSpace;
	rect = CreateButtonItem(KEYBAR_BUTTON_INSERT_KEY, BUTTON_INSERT, m_cNormal, nX, rect.top);
	rect = CreateButtonItem(KEYBAR_BUTTON_DELETE_KEY, BUTTON_DELETE, m_cNormal, rect.right + nSpace, rect.top);
	rect = CreateButtonItem(KEYBAR_BUTTON_RESET_KEY, BUTTON_RESET, m_cNormal, rect.right + nSpace, rect.top);

	nRight = nWidth - TRACKMGR_GUI_BAR_EDGE_SPACE;
	rect = CreateButtonItem(KEYBAR_BUTTON_LOCK_ROT_Z, BUTTON_Z, m_cNormal, nRight, rect.top, GUI_JUSTIFY_RIGHT);
	rect = CreateButtonItem(KEYBAR_BUTTON_LOCK_ROT_Y, BUTTON_Y, m_cNormal, rect.left - nHalfHeight, rect.top, GUI_JUSTIFY_RIGHT);
	rect = CreateButtonItem(KEYBAR_BUTTON_LOCK_ROT_X, BUTTON_X, m_cNormal, rect.left - nHalfHeight, rect.top, GUI_JUSTIFY_RIGHT);
	rect = CreateTextItem(KEYBAR_TEXT_LOCK_ROT, TEXT_LOCK_ROT, m_cText, rect.left, rect.top, GUI_JUSTIFY_RIGHT);

	rect = CreateButtonItem(KEYBAR_BUTTON_LOCK_POS_Z, BUTTON_Z, m_cNormal, nRight, rect.top - nFontHeight, GUI_JUSTIFY_RIGHT);
	rect = CreateButtonItem(KEYBAR_BUTTON_LOCK_POS_Y, BUTTON_Y, m_cNormal, rect.left - nHalfHeight, rect.top, GUI_JUSTIFY_RIGHT);
	rect = CreateButtonItem(KEYBAR_BUTTON_LOCK_POS_X, BUTTON_X, m_cNormal, rect.left - nHalfHeight, rect.top, GUI_JUSTIFY_RIGHT);
	rect = CreateTextItem(KEYBAR_TEXT_LOCK_POS, TEXT_LOCK_POS, m_cText, rect.left, rect.top, GUI_JUSTIFY_RIGHT);

	nX = m_pTextItems[KEYBAR_TEXT_TITLE].m_rRect.right + nSpace;
	rect = CreateTextItem(KEYBAR_TEXT_KEY, TEXT_KEY, m_cText, nX, TRACKMGR_GUI_BAR_EDGE_SPACE);
	rect = CreateButtonItem(KEYBAR_BUTTON_PREV_KEY_FAST, BUTTON_PREV_FAST, m_cNormal, rect.right + nFontHeight, TRACKMGR_GUI_BAR_EDGE_SPACE);
	rect = CreateButtonItem(KEYBAR_BUTTON_PREV_KEY, BUTTON_PREV, m_cNormal, rect.right + nFontHeight, TRACKMGR_GUI_BAR_EDGE_SPACE);
	rect = CreateButtonItem(KEYBAR_BUTTON_NEXT_KEY, BUTTON_NEXT, m_cNormal, rect.right + nFontHeight, TRACKMGR_GUI_BAR_EDGE_SPACE);
	rect = CreateButtonItem(KEYBAR_BUTTON_NEXT_KEY_FAST, BUTTON_NEXT_FAST, m_cNormal, rect.right + nFontHeight, TRACKMGR_GUI_BAR_EDGE_SPACE);
	rect = CreateTextItem(KEYBAR_TEXT_KEY_NUM, TEXT_RANGE, m_cText, rect.right + nSpace, TRACKMGR_GUI_BAR_EDGE_SPACE);

	nX = m_pTextItems[KEYBAR_TEXT_TITLE].m_rRect.right + nSpace;
	rect = CreateTextItem(KEYBAR_TEXT_KEY_POSITION, TEXT_POSITION, m_cText, nX, rect.bottom);
	rect = CreateTextItem(KEYBAR_TEXT_KEY_ROTATION, TEXT_ROTATION, m_cText, nX, rect.bottom);
	rect = CreateTextItem(KEYBAR_TEXT_TIME, TEXT_TIME, m_cText, nX, rect.bottom);
	rect = CreateTextItem(KEYBAR_TEXT_FLEX, TEXT_FLEX, m_cText, rect.right + nSpace, rect.top);

	m_nNumLines = (m_pButtonItems[KEYBAR_BUTTON_CONTROL_KEY].m_rRect.bottom - m_pTextItems[KEYBAR_TEXT_TITLE].m_rRect.top) / m_pFont->Height();
}

//*********************************************************************************

void LithTrackBarKey::UpdateControl()
{
	if(!g_pTrackMgr->DrawObj().Visible()) return;

	LithTrack *pTrack = g_pTrackMgr->DrawObj().Track();
	if(!pTrack) return;

	if(!(g_pTrackMgr->ControlType() & GUI_CONTROL_KEY)) return;

	ILTMath *pMathLT = m_pInterface->GetMathLT();
	DFLOAT fFrameTime = m_pInterface->GetFrameTime();

	DBYTE byDevice;
	int nX, nY;
	g_pTrackMgr->ControlValues(byDevice, nX, nY);

	DVector vPos = pTrack->Key(m_nCurrentKey)->m_vPos;
	DVector vRot = pTrack->Key(m_nCurrentKey)->m_vRot;
	DFLOAT fFlex = pTrack->Key(m_nCurrentKey)->m_fFlex;
	DFLOAT fTime = pTrack->Key(m_nCurrentKey)->m_fTime;

	// The first key can't be moved
	if((byDevice & GUI_DEVICE_KEYBOARD) && m_nCurrentKey)
	{
		DVector vNewPos = vPos;
		DVector vRight, vUp, vForward;
		DRotation rRot;

		pMathLT->SetupEuler(rRot, MATH_DEGREES_TO_RADIANS(vRot.x), MATH_DEGREES_TO_RADIANS(vRot.y), MATH_DEGREES_TO_RADIANS(vRot.z));
		pMathLT->GetRotationVectors(rRot, vRight, vUp, vForward);

		vUp *= fFrameTime * g_pTrackMgr->DataKey().m_fMoveSpeed;
		vRight *= fFrameTime * g_pTrackMgr->DataKey().m_fMoveSpeed;
		vForward *= fFrameTime * g_pTrackMgr->DataKey().m_fMoveSpeed;

		if(g_pTrackMgr->DataKey().m_byLockControls & GUI_CONTROL_LOCK_POS_X)
		{
			vForward.x = 0.0f;
			vUp.x = 0.0f;
			vRight.x = 0.0f;
		}
		if(g_pTrackMgr->DataKey().m_byLockControls & GUI_CONTROL_LOCK_POS_Y)
		{
			vForward.y = 0.0f;
			vUp.y = 0.0f;
			vRight.y = 0.0f;
		}
		if(g_pTrackMgr->DataKey().m_byLockControls & GUI_CONTROL_LOCK_POS_Z)
		{
			vForward.z = 0.0f;
			vUp.z = 0.0f;
			vRight.z = 0.0f;
		}

		if(g_pTrackMgr->DataControl().m_bUpArrow)		vNewPos += (g_pTrackMgr->DataControl().m_bCtrlKey ? vUp : vForward);
		if(g_pTrackMgr->DataControl().m_bDownArrow)		vNewPos -= (g_pTrackMgr->DataControl().m_bCtrlKey ? vUp : vForward);
		if(g_pTrackMgr->DataControl().m_bRightArrow)	vNewPos += vRight;
		if(g_pTrackMgr->DataControl().m_bLeftArrow)		vNewPos -= vRight;

		if(vNewPos != vPos)
		{
			pTrack->Key(m_nCurrentKey)->m_vPos = vNewPos;
			m_bResetOnUpdate = DTRUE;
		}
	}

	if(byDevice & GUI_DEVICE_MOUSE)
	{
		DFLOAT fRotSpeed = g_pTrackMgr->DataKey().m_fRotSpeed;

		if(g_pTrackMgr->DataControl().m_bShiftKey)
		{
			if(g_pTrackMgr->DataControl().m_bCtrlKey)
			{
				// Shift + Ctrl moves the key in time, between its neighbours
				if(!m_nCurrentKey) return;

				DFLOAT fNewTime = fTime - (nY * (fRotSpeed * 0.01f));

				if(fNewTime < pTrack->Key(m_nCurrentKey - 1)->m_fTime)
					fNewTime = pTrack->Key(m_nCurrentKey - 1)->m_fTime;

				if(m_nCurrentKey < pTrack->NumKeys() - 1)
				{
					if(fNewTime > pTrack->Key(m_nCurrentKey + 1)->m_fTime)
						fNewTime = pTrack->Key(m_nCurrentKey + 1)->m_fTime;
				}

				if(fNewTime != fTime)
				{
					pTrack->Key(m_nCurrentKey)->m_fTime = fNewTime;
					m_bResetOnUpdate = DTRUE;
				}
			}
			else
			{
				// Shift changes the flex
				DFLOAT fNewFlex = fFlex - (nY * fRotSpeed);

				if(fNewFlex < 0.0f)
					fNewFlex = 0.0f;

				if(fNewFlex != fFlex)
				{
					pTrack->Key(m_nCurrentKey)->m_fFlex = fNewFlex;
					m_bResetOnUpdate = DTRUE;
				}
			}
		}
		else
		{
			DVector vNewRot = vRot;

			vNewRot.x += (g_pTrackMgr->DataKey().m_byLockControls & GUI_CONTROL_LOCK_ROT_X) ? 0.0f : nY * fRotSpeed;
			vNewRot.y += g_pTrackMgr->DataControl().m_bCtrlKey ? 0.0f : ((g_pTrackMgr->DataKey().m_byLockControls & GUI_CONTROL_LOCK_ROT_Y) ? 0.0f : nX * fRotSpeed);
			vNewRot.z += (g_pTrackMgr->DataControl().m_bCtrlKey && !(g_pTrackMgr->DataKey().m_byLockControls & GUI_CONTROL_LOCK_ROT_Z)) ? nX * fRotSpeed : 0.0f;

			if(vNewRot.x < 0.0f) vNewRot.x += 360.0f;
			else if(vNewRot.x >= 360.0f) vNewRot.x -= 360.0f;
			if(vNewRot.y < 0.0f) vNewRot.y += 360.0f;
			else if(vNewRot.y >= 360.0f) vNewRot.y -= 360.0f;
			if(vNewRot.z < 0.0f) vNewRot.z += 360.0f;
			else if(vNewRot.z >= 360.0f) vNewRot.z -= 360.0f;

			if(vNewRot != vRot)
			{
				pTrack->Key(m_nCurrentKey)->m_vRot = vNewRot;
				m_bResetOnUpdate = DTRUE;
			}
		}
	}
}
