//*********************************************************************************
// LithTrackBarTrack.cpp
//
// LithTrackMgr, as AvP2's client links it (LithTrackMgrClient.lib), written
// from the release's prebuilt build of it.
//*********************************************************************************

#include "lithtrackdefs.h"
#include "lithtrackmgr.h"

//*********************************************************************************

DBOOL LithTrackBarTrack::Init(INTERFACE *pInterface, LithFont *pFont, void *pData)
{
	LithTrackBarBase::Init(pInterface, pFont, pData);
	SetupLines(0, 0);

	m_nCurrentTrack = -1;
	m_bResetOnUpdate = DTRUE;
	m_bFirstDisplay = DTRUE;

	return DTRUE;
}

//*********************************************************************************

DBOOL LithTrackBarTrack::Update()
{
	if(!LithTrackBarBase::Update())
		return DFALSE;

	// Select the first track as soon as there is one
	if((m_nCurrentTrack < 0) && g_pTrackMgr->TrackList().NumTracks())
	{
		m_nCurrentTrack = 0;
		g_pTrackMgr->DrawObj().Track(g_pTrackMgr->TrackList().Track(m_nCurrentTrack));
		m_bResetOnUpdate = DTRUE;
	}

	UpdateControl();

	if(m_bResetOnUpdate)
	{
		ResetSurface();
		m_bResetOnUpdate = DFALSE;
	}

	return DTRUE;
}

//*********************************************************************************

DBOOL LithTrackBarTrack::UpdateResolution(DDWORD nWidth, DDWORD nHeight)
{
	SetupLines(nWidth, nHeight);
	return LithTrackBarBase::UpdateResolution(nWidth, nHeight);
}

//*********************************************************************************

DBOOL LithTrackBarTrack::UpdateItems()
{
	if(!m_pInterface) return DFALSE;

	char szStr[64];

	sprintf(szStr, TEXT_RANGE, m_nCurrentTrack + 1, g_pTrackMgr->TrackList().NumTracks());
	UpdateTextItemString(TRACKBAR_TEXT_TRACK_NUM, szStr);

	if(m_nCurrentTrack >= 0)
	{
		sprintf(szStr, TEXT_NAME, g_pTrackMgr->TrackList().Track(m_nCurrentTrack)->Name());
		UpdateTextItemString(TRACKBAR_TEXT_TRACK_NAME, szStr);

		DVector vPos = g_pTrackMgr->DrawObj().Pos();
		DVector vRot = g_pTrackMgr->DrawObj().Rot();

		sprintf(szStr, TEXT_POSITION, vPos.x, vPos.y, vPos.z);
		UpdateTextItemString(TRACKBAR_TEXT_TRACK_POSITION, szStr);

		sprintf(szStr, TEXT_ROTATION, vRot.x, vRot.y, vRot.z);
		UpdateTextItemString(TRACKBAR_TEXT_TRACK_ROTATION, szStr);
	}
	else
	{
		sprintf(szStr, TEXT_NAME, " ");
		UpdateTextItemString(TRACKBAR_TEXT_TRACK_NAME, szStr);

		sprintf(szStr, TEXT_POSITION, 0.0f, 0.0f, 0.0f);
		UpdateTextItemString(TRACKBAR_TEXT_TRACK_POSITION, szStr);

		sprintf(szStr, TEXT_ROTATION, 0.0f, 0.0f, 0.0f);
		UpdateTextItemString(TRACKBAR_TEXT_TRACK_ROTATION, szStr);
	}

	return DTRUE;
}

//*********************************************************************************

void LithTrackBarTrack::OnMouseClick()
{
	ILTMath *pMathLT = m_pInterface->GetMathLT();
	DBYTE byLock = 0;

	if(!g_pTrackMgr) return;

	switch(m_nButtonFocus)
	{
		case TRACKBAR_BUTTON_NEW_TRACK:
		{
			if(g_pTrackMgr->TrackList().New())
			{
				m_nCurrentTrack = g_pTrackMgr->TrackList().NumTracks() - 1;
				g_pTrackMgr->DrawObj().Track(g_pTrackMgr->TrackList().Track(m_nCurrentTrack));
			}
			break;
		}

		case TRACKBAR_BUTTON_COPY_TRACK:
		{
			if(g_pTrackMgr->TrackList().Copy(m_nCurrentTrack))
			{
				m_nCurrentTrack = g_pTrackMgr->TrackList().NumTracks() - 1;
				g_pTrackMgr->DrawObj().Track(g_pTrackMgr->TrackList().Track(m_nCurrentTrack));
			}
			break;
		}

		case TRACKBAR_BUTTON_DELETE_TRACK:
		{
			g_pTrackMgr->TrackList().Delete(m_nCurrentTrack);

			if(m_nCurrentTrack >= g_pTrackMgr->TrackList().NumTracks())
				m_nCurrentTrack = g_pTrackMgr->TrackList().NumTracks() - 1;

			g_pTrackMgr->DrawObj().Track(g_pTrackMgr->TrackList().Track(m_nCurrentTrack));
			break;
		}

		case TRACKBAR_BUTTON_ALIGN_TRACK:
		{
			if(g_pTrackMgr->DataAlign().m_bCamera)
			{
				g_pTrackMgr->DrawObj().Pos(g_pTrackMgr->DataCamera().m_vPos);
				g_pTrackMgr->DrawObj().Rot(g_pTrackMgr->DataCamera().m_fPitch, g_pTrackMgr->DataCamera().m_fYaw, g_pTrackMgr->DataCamera().m_fRoll);
			}
			else if(g_pTrackMgr->DataAlign().m_hObj)
			{
				DVector vAngles, vPos;
				DRotation rRot;

				m_pInterface->GetObjectPos(g_pTrackMgr->DataAlign().m_hObj, &vPos);
				m_pInterface->GetObjectRotation(g_pTrackMgr->DataAlign().m_hObj, &rRot);
				pMathLT->GetEulerAngles(rRot, vAngles);

				if(vAngles.x < 0.0f) vAngles.x += MATH_CIRCLE;
				if(vAngles.y < 0.0f) vAngles.y += MATH_CIRCLE;
				if(vAngles.z < 0.0f) vAngles.z += MATH_CIRCLE;

				vAngles.x = MATH_RADIANS_TO_DEGREES(vAngles.x);
				vAngles.y = MATH_RADIANS_TO_DEGREES(vAngles.y);
				vAngles.z = MATH_RADIANS_TO_DEGREES(vAngles.z);

				g_pTrackMgr->DrawObj().Pos(vPos);
				g_pTrackMgr->DrawObj().Rot(vAngles);
			}
			else
			{
				DVector vAngles;

				pMathLT->GetEulerAngles(g_pTrackMgr->DataAlign().m_rRot, vAngles);

				if(vAngles.x < 0.0f) vAngles.x += MATH_CIRCLE;
				if(vAngles.y < 0.0f) vAngles.y += MATH_CIRCLE;
				if(vAngles.z < 0.0f) vAngles.z += MATH_CIRCLE;

				vAngles.x = MATH_RADIANS_TO_DEGREES(vAngles.x);
				vAngles.y = MATH_RADIANS_TO_DEGREES(vAngles.y);
				vAngles.z = MATH_RADIANS_TO_DEGREES(vAngles.z);

				g_pTrackMgr->DrawObj().Pos(g_pTrackMgr->DataAlign().m_vPos);
				g_pTrackMgr->DrawObj().Rot(vAngles);
			}
			break;
		}

		case TRACKBAR_BUTTON_RESET:
		{
			DVector vRot = g_pTrackMgr->DrawObj().Rot();

			if(!(g_pTrackMgr->DataTrack().m_byLockControls & GUI_CONTROL_LOCK_ROT_X)) vRot.x = 0.0f;
			if(!(g_pTrackMgr->DataTrack().m_byLockControls & GUI_CONTROL_LOCK_ROT_Y)) vRot.y = 0.0f;
			if(!(g_pTrackMgr->DataTrack().m_byLockControls & GUI_CONTROL_LOCK_ROT_Z)) vRot.z = 0.0f;

			g_pTrackMgr->DrawObj().Rot(vRot);
			break;
		}

		case TRACKBAR_BUTTON_LOCK_POS_X:	byLock = GUI_CONTROL_LOCK_POS_X;	break;
		case TRACKBAR_BUTTON_LOCK_POS_Y:	byLock = GUI_CONTROL_LOCK_POS_Y;	break;
		case TRACKBAR_BUTTON_LOCK_POS_Z:	byLock = GUI_CONTROL_LOCK_POS_Z;	break;
		case TRACKBAR_BUTTON_LOCK_ROT_X:	byLock = GUI_CONTROL_LOCK_ROT_X;	break;
		case TRACKBAR_BUTTON_LOCK_ROT_Y:	byLock = GUI_CONTROL_LOCK_ROT_Y;	break;
		case TRACKBAR_BUTTON_LOCK_ROT_Z:	byLock = GUI_CONTROL_LOCK_ROT_Z;	break;

		case TRACKBAR_BUTTON_PREV_TRACK_FAST:
		{
			if(!g_pTrackMgr->TrackList().NumTracks())
			{
				m_nCurrentTrack = -1;
				break;
			}

			if(m_nCurrentTrack >= TRACKMGR_DEFAULT_TRACK_SELECT_FAST)
				m_nCurrentTrack -= TRACKMGR_DEFAULT_TRACK_SELECT_FAST;
			else
				m_nCurrentTrack = 0;

			g_pTrackMgr->DrawObj().Track(g_pTrackMgr->TrackList().Track(m_nCurrentTrack));
			break;
		}

		case TRACKBAR_BUTTON_PREV_TRACK:
		{
			if(!g_pTrackMgr->TrackList().NumTracks())
			{
				m_nCurrentTrack = -1;
				break;
			}

			if(m_nCurrentTrack > 0)
				m_nCurrentTrack--;
			else
				m_nCurrentTrack = 0;

			g_pTrackMgr->DrawObj().Track(g_pTrackMgr->TrackList().Track(m_nCurrentTrack));
			break;
		}

		case TRACKBAR_BUTTON_NEXT_TRACK:
		{
			if(m_nCurrentTrack < g_pTrackMgr->TrackList().NumTracks() - 1)
				m_nCurrentTrack++;
			else
				m_nCurrentTrack = g_pTrackMgr->TrackList().NumTracks() - 1;

			g_pTrackMgr->DrawObj().Track(g_pTrackMgr->TrackList().Track(m_nCurrentTrack));
			break;
		}

		case TRACKBAR_BUTTON_NEXT_TRACK_FAST:
		{
			if(m_nCurrentTrack < g_pTrackMgr->TrackList().NumTracks() - 1 - TRACKMGR_DEFAULT_TRACK_SELECT_FAST)
				m_nCurrentTrack += TRACKMGR_DEFAULT_TRACK_SELECT_FAST;
			else
				m_nCurrentTrack = g_pTrackMgr->TrackList().NumTracks() - 1;

			g_pTrackMgr->DrawObj().Track(g_pTrackMgr->TrackList().Track(m_nCurrentTrack));
			break;
		}

		case TRACKBAR_BUTTON_DISPLAY_TRACK:
		{
			m_pButtonItems[m_nButtonFocus].m_bSelected = !m_pButtonItems[m_nButtonFocus].m_bSelected;

			// The first time the track is displayed, bring it to the camera
			if(m_pButtonItems[m_nButtonFocus].m_bSelected && m_bFirstDisplay)
			{
				g_pTrackMgr->DrawObj().Pos(g_pTrackMgr->DataCamera().m_vPos);
				g_pTrackMgr->DrawObj().Rot(g_pTrackMgr->DataCamera().m_fPitch, g_pTrackMgr->DataCamera().m_fYaw, g_pTrackMgr->DataCamera().m_fRoll);
				m_bFirstDisplay = DFALSE;
			}

			g_pTrackMgr->DrawObj().Visible(m_pButtonItems[m_nButtonFocus].m_bSelected);
			break;
		}

		case TRACKBAR_BUTTON_CONTROL_TRACK:
		{
			g_pTrackMgr->ClearControlType();
			m_pButtonItems[m_nButtonFocus].m_bSelected = DTRUE;
			g_pTrackMgr->ControlType() |= GUI_CONTROL_EDIT_TRACK;
			break;
		}
	}

	if(byLock)
	{
		m_pButtonItems[m_nButtonFocus].m_bSelected = !m_pButtonItems[m_nButtonFocus].m_bSelected;

		if(m_pButtonItems[m_nButtonFocus].m_bSelected)
			g_pTrackMgr->DataTrack().m_byLockControls |= byLock;
		else
			g_pTrackMgr->DataTrack().m_byLockControls &= ~byLock;
	}

	ResetSurface();
}

//*********************************************************************************

void LithTrackBarTrack::OnEditModeOn()
{
	if(m_pButtonItems[TRACKBAR_BUTTON_DISPLAY_TRACK].m_bSelected)
		g_pTrackMgr->DrawObj().Visible(DTRUE);
}

//*********************************************************************************

void LithTrackBarTrack::OnEditModeOff()
{
	g_pTrackMgr->DrawObj().Visible(DFALSE);
	ResetSurface();
}

//*********************************************************************************

void LithTrackBarTrack::OnControlChange()
{
	m_pButtonItems[TRACKBAR_BUTTON_CONTROL_TRACK].m_bSelected = DFALSE;
	ResetSurface();
}

//*********************************************************************************

void LithTrackBarTrack::SetupLines(int nWidth, int nHeight)
{
	if(!m_pFont) return;

	int nFontHeight = m_pFont->Height();
	int nSpace = nFontHeight * 2;
	int nHalfHeight = nFontHeight >> 1;
	int nX;
	DRect rect;

	rect = CreateTextItem(TRACKBAR_TEXT_TITLE, TITLE_TRACK_DETAILS, m_cTitle, TRACKMGR_GUI_BAR_EDGE_SPACE, TRACKMGR_GUI_BAR_EDGE_SPACE);
	rect = CreateButtonItem(TRACKBAR_BUTTON_DISPLAY_TRACK, BUTTON_DISPLAY, m_cNormal, TRACKMGR_GUI_BAR_EDGE_SPACE, rect.bottom + nSpace);
	rect = CreateButtonItem(TRACKBAR_BUTTON_CONTROL_TRACK, BUTTON_CONTROL, m_cNormal, TRACKMGR_GUI_BAR_EDGE_SPACE, rect.bottom);

	nX = m_pTextItems[TRACKBAR_TEXT_TITLE].m_rRect.right + nSpace;
	rect = CreateButtonItem(TRACKBAR_BUTTON_NEW_TRACK, BUTTON_NEW, m_cNormal, nX, rect.top);
	rect = CreateButtonItem(TRACKBAR_BUTTON_COPY_TRACK, BUTTON_COPY, m_cNormal, rect.right + nSpace, rect.top);
	rect = CreateButtonItem(TRACKBAR_BUTTON_DELETE_TRACK, BUTTON_DELETE, m_cNormal, rect.right + nSpace, rect.top);
	rect = CreateButtonItem(TRACKBAR_BUTTON_ALIGN_TRACK, BUTTON_ALIGN, m_cNormal, rect.right + nSpace, rect.top);
	rect = CreateButtonItem(TRACKBAR_BUTTON_RESET, BUTTON_RESET, m_cNormal, rect.right + nSpace, rect.top);

	rect = CreateButtonItem(TRACKBAR_BUTTON_LOCK_ROT_Z, BUTTON_Z, m_cNormal, nWidth - TRACKMGR_GUI_BAR_EDGE_SPACE, rect.top, GUI_JUSTIFY_RIGHT);
	rect = CreateButtonItem(TRACKBAR_BUTTON_LOCK_ROT_Y, BUTTON_Y, m_cNormal, rect.left - nHalfHeight, rect.top, GUI_JUSTIFY_RIGHT);
	rect = CreateButtonItem(TRACKBAR_BUTTON_LOCK_ROT_X, BUTTON_X, m_cNormal, rect.left - nHalfHeight, rect.top, GUI_JUSTIFY_RIGHT);
	rect = CreateTextItem(TRACKBAR_TEXT_LOCK_ROT, TEXT_LOCK_ROT, m_cText, rect.left, rect.top, GUI_JUSTIFY_RIGHT);

	rect = CreateButtonItem(TRACKBAR_BUTTON_LOCK_POS_Z, BUTTON_Z, m_cNormal, nWidth - TRACKMGR_GUI_BAR_EDGE_SPACE, rect.top - nFontHeight, GUI_JUSTIFY_RIGHT);
	rect = CreateButtonItem(TRACKBAR_BUTTON_LOCK_POS_Y, BUTTON_Y, m_cNormal, rect.left - nHalfHeight, rect.top, GUI_JUSTIFY_RIGHT);
	rect = CreateButtonItem(TRACKBAR_BUTTON_LOCK_POS_X, BUTTON_X, m_cNormal, rect.left - nHalfHeight, rect.top, GUI_JUSTIFY_RIGHT);
	rect = CreateTextItem(TRACKBAR_TEXT_LOCK_POS, TEXT_LOCK_POS, m_cText, rect.left, rect.top, GUI_JUSTIFY_RIGHT);

	nX = m_pTextItems[TRACKBAR_TEXT_TITLE].m_rRect.right + nSpace;
	rect = CreateTextItem(TRACKBAR_TEXT_TRACK, TEXT_TRACK, m_cText, nX, TRACKMGR_GUI_BAR_EDGE_SPACE);
	rect = CreateButtonItem(TRACKBAR_BUTTON_PREV_TRACK_FAST, BUTTON_PREV_FAST, m_cNormal, rect.right + nFontHeight, TRACKMGR_GUI_BAR_EDGE_SPACE);
	rect = CreateButtonItem(TRACKBAR_BUTTON_PREV_TRACK, BUTTON_PREV, m_cNormal, rect.right + nFontHeight, TRACKMGR_GUI_BAR_EDGE_SPACE);
	rect = CreateButtonItem(TRACKBAR_BUTTON_NEXT_TRACK, BUTTON_NEXT, m_cNormal, rect.right + nFontHeight, TRACKMGR_GUI_BAR_EDGE_SPACE);
	rect = CreateButtonItem(TRACKBAR_BUTTON_NEXT_TRACK_FAST, BUTTON_NEXT_FAST, m_cNormal, rect.right + nFontHeight, TRACKMGR_GUI_BAR_EDGE_SPACE);
	rect = CreateTextItem(TRACKBAR_TEXT_TRACK_NUM, TEXT_RANGE, m_cText, rect.right + nSpace, TRACKMGR_GUI_BAR_EDGE_SPACE);

	nX = m_pTextItems[TRACKBAR_TEXT_TITLE].m_rRect.right + nSpace;
	rect = CreateTextItem(TRACKBAR_TEXT_TRACK_NAME, TEXT_NAME, m_cText, nX, m_pTextItems[TRACKBAR_TEXT_TRACK].m_rRect.bottom);
	rect = CreateTextItem(TRACKBAR_TEXT_TRACK_POSITION, TEXT_POSITION, m_cText, nX, rect.bottom);
	rect = CreateTextItem(TRACKBAR_TEXT_TRACK_ROTATION, TEXT_ROTATION, m_cText, nX, rect.bottom);

	m_nNumLines = (m_pButtonItems[TRACKBAR_BUTTON_CONTROL_TRACK].m_rRect.bottom - m_pTextItems[TRACKBAR_TEXT_TITLE].m_rRect.top) / m_pFont->Height();
}

//*********************************************************************************

void LithTrackBarTrack::UpdateControl()
{
	if(!g_pTrackMgr->DrawObj().Visible()) return;
	if(!(g_pTrackMgr->ControlType() & GUI_CONTROL_EDIT_TRACK)) return;

	ILTMath *pMathLT = m_pInterface->GetMathLT();
	DFLOAT fFrameTime = m_pInterface->GetFrameTime();

	DBYTE byDevice;
	int nX, nY;
	g_pTrackMgr->ControlValues(byDevice, nX, nY);

	DVector vPos = g_pTrackMgr->DrawObj().Pos();
	DVector vRot = g_pTrackMgr->DrawObj().Rot();

	if(byDevice & GUI_DEVICE_KEYBOARD)
	{
		DVector vNewPos = vPos;
		DVector vRight, vUp, vForward;
		DRotation rRot;

		pMathLT->SetupEuler(rRot, MATH_DEGREES_TO_RADIANS(vRot.x), MATH_DEGREES_TO_RADIANS(vRot.y), MATH_DEGREES_TO_RADIANS(vRot.z));
		pMathLT->GetRotationVectors(rRot, vRight, vUp, vForward);

		vUp *= fFrameTime * g_pTrackMgr->DataTrack().m_fMoveSpeed;
		vRight *= fFrameTime * g_pTrackMgr->DataTrack().m_fMoveSpeed;
		vForward *= fFrameTime * g_pTrackMgr->DataTrack().m_fMoveSpeed;

		if(g_pTrackMgr->DataTrack().m_byLockControls & GUI_CONTROL_LOCK_POS_X)
		{
			vForward.x = 0.0f;
			vUp.x = 0.0f;
			vRight.x = 0.0f;
		}
		if(g_pTrackMgr->DataTrack().m_byLockControls & GUI_CONTROL_LOCK_POS_Y)
		{
			vForward.y = 0.0f;
			vUp.y = 0.0f;
			vRight.y = 0.0f;
		}
		if(g_pTrackMgr->DataTrack().m_byLockControls & GUI_CONTROL_LOCK_POS_Z)
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
			g_pTrackMgr->DrawObj().Pos(vNewPos);
			m_bResetOnUpdate = DTRUE;
		}
	}

	if(byDevice & GUI_DEVICE_MOUSE)
	{
		DVector vNewRot = vRot;
		DFLOAT fRotSpeed = g_pTrackMgr->DataTrack().m_fRotSpeed;

		vNewRot.x += (g_pTrackMgr->DataTrack().m_byLockControls & GUI_CONTROL_LOCK_ROT_X) ? 0.0f : nY * fRotSpeed;
		vNewRot.y += g_pTrackMgr->DataControl().m_bCtrlKey ? 0.0f : ((g_pTrackMgr->DataTrack().m_byLockControls & GUI_CONTROL_LOCK_ROT_Y) ? 0.0f : nX * fRotSpeed);
		vNewRot.z += (g_pTrackMgr->DataControl().m_bCtrlKey && !(g_pTrackMgr->DataTrack().m_byLockControls & GUI_CONTROL_LOCK_ROT_Z)) ? nX * fRotSpeed : 0.0f;

		if(vNewRot.x < 0.0f) vNewRot.x += 360.0f;
		else if(vNewRot.x >= 360.0f) vNewRot.x -= 360.0f;
		if(vNewRot.y < 0.0f) vNewRot.y += 360.0f;
		else if(vNewRot.y >= 360.0f) vNewRot.y -= 360.0f;
		if(vNewRot.z < 0.0f) vNewRot.z += 360.0f;
		else if(vNewRot.z >= 360.0f) vNewRot.z -= 360.0f;

		if(vNewRot != vRot)
		{
			g_pTrackMgr->DrawObj().Rot(vNewRot);
			m_bResetOnUpdate = DTRUE;
		}
	}
}
