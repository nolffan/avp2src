//*********************************************************************************
// LithTrackMgr.cpp
//
// LithTrackMgr, as AvP2's client links it (LithTrackMgrClient.lib), written
// from the release's prebuilt build of it.
//*********************************************************************************

#include "lithtrackmgr.h"
#include "lithfontmgr.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//*********************************************************************************

LithTrackMgr *g_pTrackMgr = DNULL;

//*********************************************************************************

LithTrackMgr::LithTrackMgr()
{
	Init();
}

//---------------------------------------------------------------------------------

LithTrackMgr::~LithTrackMgr()
{
	Term();
}

//---------------------------------------------------------------------------------

DBOOL LithTrackMgr::Init(INTERFACE *pInterface)
{
	m_pInterface = pInterface;
	m_bEditMode = DFALSE;

	m_tList.Init(TRACKMGR_DATA_FILE);

	m_nScreenWidth = 0;
	m_nScreenHeight = 0;

	m_byControlType = GUI_CONTROL_NONE;
	m_byControlDevice = GUI_DEVICE_NONE;
	m_nControlXAxis = 0;
	m_nControlYAxis = 0;

	m_pFont = DNULL;
	m_cTransColor = 0;

	memset(m_pGUIBars, 0, sizeof(m_pGUIBars));

	if(m_pInterface)
	{
		// Get the dimensions of the screen
		HSURFACE hScreen = m_pInterface->GetScreenSurface();
		if(hScreen) m_pInterface->GetSurfaceDims(hScreen, &m_nScreenWidth, &m_nScreenHeight);

		// Create the camera
		ObjectCreateStruct ocs;
		INIT_OBJECTCREATESTRUCT(ocs);
		ocs.m_ObjectType = OT_CAMERA;
		m_dCamera.m_hObj = m_pInterface->CreateObject(&ocs);
		SetupCamera();

		// Create the font
		m_lfCS.szFontName = "Arial";
		m_lfCS.nWidth = TRACKMGR_DEFAULT_FONT_WIDTH;
		m_lfCS.nHeight = TRACKMGR_DEFAULT_FONT_HEIGHT;
		m_pFont = CreateLithFont(m_pInterface, &m_lfCS, DTRUE);

		// Init the GUI bars
		m_EditBar.Init(m_pInterface, m_pFont);
		m_TrackBar.Init(m_pInterface, m_pFont);
		m_KeyBar.Init(m_pInterface, m_pFont);
		m_CameraBar.Init(m_pInterface, m_pFont);
		m_HelpBar.Init(m_pInterface, m_pFont);

		AddGUIBarToList(&m_EditBar);

		m_cTransColor = m_pInterface->SetupColor2(1.0f, 0.0f, 1.0f, DFALSE);

		SetupMouseCursor();
		m_dCursor.m_hColor = m_pInterface->SetupColor2(0.0f, 1.0f, 0.0f, DFALSE);

		m_pInterface->RegisterConsoleProgram(VAR_COMMAND_TRACKEDIT, TrackEditFn);

		m_ltDO.Init(m_pInterface);
	}

	return DTRUE;
}

//---------------------------------------------------------------------------------

void LithTrackMgr::Term()
{
	m_tList.Term();

	m_EditBar.Term();
	m_TrackBar.Term();
	m_KeyBar.Term();
	m_CameraBar.Term();
	m_HelpBar.Term();

	memset(m_pGUIBars, 0, sizeof(m_pGUIBars));

	FreeLithFont(m_pFont);
	m_pFont = DNULL;

	if(m_pInterface)
	{
		if(m_dCamera.m_hObj)
		{
			m_pInterface->DeleteObject(m_dCamera.m_hObj);
			m_dCamera.m_hObj = DNULL;
		}

		if(m_dCursor.m_hSurf)
		{
			m_pInterface->DeleteSurface(m_dCursor.m_hSurf);
			m_dCursor.m_hSurf = DNULL;
		}

		m_pInterface->UnregisterConsoleProgram(VAR_COMMAND_TRACKEDIT);

		m_ltDO.Term();
	}

	m_pInterface = DNULL;
}

//---------------------------------------------------------------------------------

DBOOL LithTrackMgr::Update()
{
	UpdateInput();
	UpdateMouseClicks();
	UpdateGUIBars();

	return DTRUE;
}

//---------------------------------------------------------------------------------

void LithTrackMgr::Rot2Vec(DRotation &rRot, DVector &vAngles)
{
	ILTMath *pMath = m_pInterface->GetMathLT();
	DRotation rTemp = rRot;
	DVector vR, vU, vF;

	// Roll
	pMath->GetRotationVectors(rTemp, vR, vU, vF);

	DVector vTemp = vF.Cross(DVector(0.0f, 1.0f, 0.0f));
	DVector vUp = vF.Cross(vTemp);

	vAngles.z = (vR.Dot(vTemp) + 1.0f) * 0.5f * 180.0f;
	if(vR.Dot(vUp) < 0.0f) vAngles.z = 360.0f - vAngles.z;

	pMath->AlignRotation(rTemp, vF, vUp);

	// Yaw
	pMath->GetRotationVectors(rTemp, vR, vU, vF);

	DVector vForward = vU.Cross(DVector(1.0f, 0.0f, 0.0f));

	vAngles.y = (vF.Dot(vForward) + 1.0f) * 0.5f * 180.0f;
	if(vF.Dot(DVector(1.0f, 0.0f, 0.0f)) < 0.0f) vAngles.y = 360.0f - vAngles.y;

	pMath->AlignRotation(rTemp, vForward, vU);

	// Pitch
	pMath->GetRotationVectors(rTemp, vR, vU, vF);

	vAngles.x = (vF.Dot(DVector(0.0f, 0.0f, 1.0f)) + 1.0f) * 0.5f * 180.0f;
	if(vF.Dot(DVector(0.0f, 1.0f, 0.0f)) < 0.0f) vAngles.x = 360.0f - vAngles.x;
}

//---------------------------------------------------------------------------------

void LithTrackMgr::Send_To_Console(char *szStr, DBOOL bInvalid)
{
	if(bInvalid)
		m_pInterface->CPrint(MSG_INVALID_PARAMS);

	m_pInterface->CPrint(szStr);
}

//---------------------------------------------------------------------------------

DBOOL LithTrackMgr::Draw()
{
	HSURFACE hScreen = m_pInterface->GetScreenSurface();
	DDWORD nWidth = 0, nHeight = 0;

	if(!hScreen || !m_bEditMode) return DFALSE;

	// Check for a change in resolution
	m_pInterface->GetSurfaceDims(hScreen, &nWidth, &nHeight);

	if((nWidth != m_nScreenWidth) || (nHeight != m_nScreenHeight))
	{
		m_nScreenWidth = nWidth;
		m_nScreenHeight = nHeight;

		SetupCamera();

		for(int i = 0; i < TRACKMGR_MAX_GUIBARS; i++)
			if(m_pGUIBars[i]) m_pGUIBars[i]->UpdateResolution(nWidth, nHeight);
	}

	// Draw the GUI bars
	for(int i = 0; i < TRACKMGR_MAX_GUIBARS; i++)
		if(m_pGUIBars[i]) m_pGUIBars[i]->Draw(hScreen);

	// Draw the mouse cursor
	if(m_dCursor.m_hSurf && !m_dCtrl.m_bMouse1)
	{
		m_pInterface->DrawSurfaceSolidColor(hScreen, m_dCursor.m_hSurf, DNULL,
			m_dCursor.m_nX - (m_dCursor.m_nSize >> 1), m_dCursor.m_nY - (m_dCursor.m_nSize >> 1),
			m_cTransColor, m_dCursor.m_hColor);
	}

	return DTRUE;
}

//---------------------------------------------------------------------------------

void LithTrackMgr::TrackCommands(int argc, char **argv)
{
	if(!m_pInterface) return;

	if(argc > 0)
	{
		if(TrackCommandEqual(argv[0], COM_VAL_HELP))
		{
			Send_To_Console(HELP_COM_TRACKEDIT);
			return;
		}
		else if(TrackCommandEqual(argv[0], COM_VAL_ON))
		{
			Command_EditModeOn();
			return;
		}
		else if(TrackCommandEqual(argv[0], COM_VAL_OFF))
		{
			Command_EditModeOff();
			return;
		}
		else if(m_bEditMode)
		{
			if(TrackCommandEqual(argv[0], COM_GUI_CURSOR))
			{
				Command_Cursor(argc - 1, argv + 1);
				return;
			}
			else if(TrackCommandEqual(argv[0], COM_GUI_CAMERA))
			{
				Command_Camera(argc - 1, argv + 1);
				return;
			}
			else if(TrackCommandEqual(argv[0], COM_GUI_FONT))
			{
				Command_Font(argc - 1, argv + 1);
				return;
			}
			else if(TrackCommandEqual(argv[0], COM_GUI_TRACK))
			{
				Command_Track(argc - 1, argv + 1);
				return;
			}
			else if(TrackCommandEqual(argv[0], COM_GUI_KEY))
			{
				Command_Key(argc - 1, argv + 1);
				return;
			}
		}
	}

	Send_To_Console(HELP_COM_TRACKEDIT, DTRUE);
}

//---------------------------------------------------------------------------------

void LithTrackMgr::GUIBarState(int nBar, DBOOL bOn)
{
	LithTrackBarBase *pBar = DNULL;

	switch(nBar)
	{
		case GUI_BAR_EDIT:		pBar = &m_EditBar;		break;
		case GUI_BAR_CAMERA:	pBar = &m_CameraBar;	break;
		case GUI_BAR_TRACK:		pBar = &m_TrackBar;		break;
		case GUI_BAR_KEY:		pBar = &m_KeyBar;		break;
		case GUI_BAR_HELP:		pBar = &m_HelpBar;		break;
	}

	if(pBar)
	{
		if(bOn)
			AddGUIBarToList(pBar);
		else
			RemoveGUIBarFromList(pBar);
	}
}

//---------------------------------------------------------------------------------

void LithTrackMgr::ClearControlType()
{
	m_EditBar.OnControlChange();
	m_CameraBar.OnControlChange();
	m_TrackBar.OnControlChange();
	m_KeyBar.OnControlChange();
	m_HelpBar.OnControlChange();

	m_byControlType = GUI_CONTROL_NONE;
}

//---------------------------------------------------------------------------------

void LithTrackMgr::ControlValues(DBYTE &nDevice, int &x, int &y)
{
	nDevice = m_byControlDevice;
	x = m_nControlXAxis;
	y = m_nControlYAxis;
}

//---------------------------------------------------------------------------------

DBOOL LithTrackMgr::TrackCommandEqual(char *szTok, const char *szStr)
{
	if(!szTok || !szStr) return DFALSE;
	return !stricmp(szTok, szStr);
}

//---------------------------------------------------------------------------------

void LithTrackMgr::Command_EditModeOn()
{
	m_bEditMode = DTRUE;

	ILTMath *pMath = m_pInterface->GetMathLT();

	m_pInterface->StartDeviceTrack(DEVICETYPE_KEYBOARD | DEVICETYPE_MOUSE, MAX_INPUT_BUFFER_SIZE);

	// Start the camera out where the player is
	HLOCALOBJ hPlayer = m_pInterface->GetClientObject();

	if(m_dCamera.m_hObj && hPlayer)
	{
		DVector vPos, vAngles;
		DRotation rRot;

		m_pInterface->GetObjectPos(hPlayer, &vPos);
		m_pInterface->GetObjectRotation(hPlayer, &rRot);
		m_pInterface->SetObjectPosAndRotation(m_dCamera.m_hObj, &vPos, &rRot);

		pMath->GetEulerAngles(rRot, vAngles);

		if(vAngles.x < 0.0f) vAngles.x += MATH_CIRCLE;
		if(vAngles.y < 0.0f) vAngles.y += MATH_CIRCLE;
		if(vAngles.z < 0.0f) vAngles.z += MATH_CIRCLE;

		m_dCamera.m_vPos = vPos;

		vAngles *= (180.0f / MATH_PI);

		m_dCamera.m_fPitch = vAngles.x;
		m_dCamera.m_fYaw = vAngles.y;
		m_dCamera.m_fRoll = vAngles.z;
	}

	// Put the mouse cursor in the middle of the screen
	HSURFACE hScreen = m_pInterface->GetScreenSurface();
	DDWORD nWidth = 0, nHeight = 0;

	if(hScreen) m_pInterface->GetSurfaceDims(hScreen, &nWidth, &nHeight);

	m_dCursor.m_nX = nWidth / 2;
	m_dCursor.m_nY = nHeight / 2;

	ClearControlType();

	m_EditBar.OnEditModeOn();
	m_CameraBar.OnEditModeOn();
	m_TrackBar.OnEditModeOn();
	m_KeyBar.OnEditModeOn();
	m_HelpBar.OnEditModeOn();
}

//---------------------------------------------------------------------------------

void LithTrackMgr::Command_EditModeOff()
{
	m_EditBar.OnEditModeOff();
	m_CameraBar.OnEditModeOff();
	m_TrackBar.OnEditModeOff();
	m_KeyBar.OnEditModeOff();
	m_HelpBar.OnEditModeOff();

	m_bEditMode = DFALSE;
	m_pInterface->EndDeviceTrack();
}

//---------------------------------------------------------------------------------

void LithTrackMgr::Command_Cursor(int nToks, char **szToks)
{
	if(nToks > 0)
	{
		if(TrackCommandEqual(szToks[0], COM_VAL_HELP))
		{
			Send_To_Console(HELP_COM_CURSOR);
			return;
		}
		else if(TrackCommandEqual(szToks[0], COM_EXT_COLOR))
		{
			Ext_Color(nToks - 1, szToks + 1, m_dCursor.m_hColor);
			return;
		}
		else if(TrackCommandEqual(szToks[0], COM_EXT_SIZE))
		{
			if(Ext_Size(nToks - 1, szToks + 1, m_dCursor.m_nSize))
				SetupMouseCursor();
			return;
		}
		else if(TrackCommandEqual(szToks[0], COM_EXT_THICKNESS))
		{
			if(Ext_Thickness(nToks - 1, szToks + 1, m_dCursor.m_nThickness))
				SetupMouseCursor();
			return;
		}
		else if(TrackCommandEqual(szToks[0], COM_EXT_SPEED))
		{
			Ext_Speed(nToks - 1, szToks + 1, m_dCursor.m_fSpeed);
			return;
		}
	}

	Send_To_Console(HELP_COM_CURSOR, DTRUE);
}

//---------------------------------------------------------------------------------

void LithTrackMgr::Command_Camera(int nToks, char **szToks)
{
	if(nToks > 0)
	{
		if(TrackCommandEqual(szToks[0], COM_VAL_HELP))
		{
			Send_To_Console(HELP_COM_CAMERA);
			return;
		}
		else if(TrackCommandEqual(szToks[0], COM_EXT_SCALE))
		{
			Ext_Scale(nToks - 1, szToks + 1, m_dCamera.m_fRotSpeed);
			return;
		}
		else if(TrackCommandEqual(szToks[0], COM_EXT_SPEED))
		{
			Ext_Speed(nToks - 1, szToks + 1, m_dCamera.m_fMoveSpeed);
			return;
		}
		else if(TrackCommandEqual(szToks[0], COM_EXT_FOV))
		{
			if(Ext_FOV(nToks - 1, szToks + 1, m_dCamera.m_fFOVX, m_dCamera.m_fFOVY))
				SetupCamera();
			return;
		}
	}

	Send_To_Console(HELP_COM_CAMERA, DTRUE);
}

//---------------------------------------------------------------------------------

void LithTrackMgr::Command_Font(int nToks, char **szToks)
{
	if(nToks > 0)
	{
		if(TrackCommandEqual(szToks[0], COM_VAL_HELP))
		{
			Send_To_Console(HELP_COM_FONT);
			return;
		}
		else if(TrackCommandEqual(szToks[0], COM_EXT_TYPE))
		{
			if(Ext_Type(nToks - 1, szToks + 1, m_lfCS.szFontName))
				SetupFont();
			return;
		}
		else if(TrackCommandEqual(szToks[0], COM_EXT_WIDTH))
		{
			if(Ext_Width(nToks - 1, szToks + 1, m_lfCS.nWidth))
				SetupFont();
			return;
		}
		else if(TrackCommandEqual(szToks[0], COM_EXT_HEIGHT))
		{
			if(Ext_Height(nToks - 1, szToks + 1, m_lfCS.nHeight))
				SetupFont();
			return;
		}
	}

	Send_To_Console(HELP_COM_FONT, DTRUE);
}

//---------------------------------------------------------------------------------

void LithTrackMgr::Command_Track(int nToks, char **szToks)
{
	if(nToks <= 0)
	{
		Send_To_Console(HELP_COM_TRACK, DTRUE);
		return;
	}

	if(TrackCommandEqual(szToks[0], COM_VAL_HELP))
		Send_To_Console(HELP_COM_TRACK);
	else if(TrackCommandEqual(szToks[0], COM_EXT_SCALE))
		Ext_Scale(nToks - 1, szToks + 1, m_dTrack.m_fRotSpeed);
	else if(TrackCommandEqual(szToks[0], COM_EXT_SPEED))
		Ext_Speed(nToks - 1, szToks + 1, m_dTrack.m_fMoveSpeed);
}

//---------------------------------------------------------------------------------

void LithTrackMgr::Command_Key(int nToks, char **szToks)
{
	if(nToks <= 0)
	{
		Send_To_Console(HELP_COM_KEY, DTRUE);
		return;
	}

	if(TrackCommandEqual(szToks[0], COM_VAL_HELP))
		Send_To_Console(HELP_COM_KEY);
	else if(TrackCommandEqual(szToks[0], COM_EXT_SCALE))
		Ext_Scale(nToks - 1, szToks + 1, m_dKey.m_fRotSpeed);
	else if(TrackCommandEqual(szToks[0], COM_EXT_SPEED))
		Ext_Speed(nToks - 1, szToks + 1, m_dKey.m_fMoveSpeed);
}

//---------------------------------------------------------------------------------

DBOOL LithTrackMgr::Ext_Color(int nToks, char **szToks, HDECOLOR &cColor)
{
	if(nToks <= 0)
	{
		char szStr[64];
		sprintf(szStr, "%s = %d %d %d", COM_EXT_COLOR, GETR(cColor), GETG(cColor), GETB(cColor));
		Send_To_Console(szStr);
		return DFALSE;
	}

	if(TrackCommandEqual(szToks[0], COM_VAL_HELP))
	{
		Send_To_Console(HELP_EXT_COLOR);
		return DFALSE;
	}

	if(nToks != 3)
	{
		Send_To_Console(HELP_EXT_COLOR, DTRUE);
		return DFALSE;
	}

	cColor = m_pInterface->SetupColor2((float)atof(szToks[0]), (float)atof(szToks[1]), (float)atof(szToks[2]), DFALSE);
	return DTRUE;
}

//---------------------------------------------------------------------------------

DBOOL LithTrackMgr::Ext_Size(int nToks, char **szToks, int &nSize)
{
	if(nToks <= 0)
	{
		char szStr[64];
		sprintf(szStr, "%s = %d", COM_EXT_SIZE, nSize);
		Send_To_Console(szStr);
		return DFALSE;
	}

	if(TrackCommandEqual(szToks[0], COM_VAL_HELP))
	{
		Send_To_Console(HELP_EXT_SIZE);
		return DFALSE;
	}

	if(nToks != 1)
	{
		Send_To_Console(HELP_EXT_SIZE, DTRUE);
		return DFALSE;
	}

	nSize = atoi(szToks[0]);
	return DTRUE;
}

//---------------------------------------------------------------------------------

DBOOL LithTrackMgr::Ext_Scale(int nToks, char **szToks, float &fScale)
{
	if(nToks <= 0)
	{
		char szStr[64];
		sprintf(szStr, "%s = %f", COM_EXT_SCALE, fScale);
		Send_To_Console(szStr);
		return DFALSE;
	}

	if(TrackCommandEqual(szToks[0], COM_VAL_HELP))
	{
		Send_To_Console(HELP_EXT_SCALE);
		return DFALSE;
	}

	if(nToks != 1)
	{
		Send_To_Console(HELP_EXT_SCALE, DTRUE);
		return DFALSE;
	}

	fScale = (float)atof(szToks[0]);
	return DTRUE;
}

//---------------------------------------------------------------------------------

DBOOL LithTrackMgr::Ext_Thickness(int nToks, char **szToks, int &nThickness)
{
	if(nToks <= 0)
	{
		char szStr[64];
		sprintf(szStr, "%s = %d", COM_EXT_THICKNESS, nThickness);
		Send_To_Console(szStr);
		return DFALSE;
	}

	if(TrackCommandEqual(szToks[0], COM_VAL_HELP))
	{
		Send_To_Console(HELP_EXT_THICKNESS);
		return DFALSE;
	}

	if(nToks != 1)
	{
		Send_To_Console(HELP_EXT_THICKNESS, DTRUE);
		return DFALSE;
	}

	nThickness = atoi(szToks[0]);
	return DTRUE;
}

//---------------------------------------------------------------------------------

DBOOL LithTrackMgr::Ext_Speed(int nToks, char **szToks, float &fSpeed)
{
	if(nToks <= 0)
	{
		char szStr[64];
		sprintf(szStr, "%s = %f", COM_EXT_SPEED, fSpeed);
		Send_To_Console(szStr);
		return DFALSE;
	}

	if(TrackCommandEqual(szToks[0], COM_VAL_HELP))
	{
		Send_To_Console(HELP_EXT_SPEED);
		return DFALSE;
	}

	if(nToks != 1)
	{
		Send_To_Console(HELP_EXT_SPEED, DTRUE);
		return DFALSE;
	}

	fSpeed = (float)atof(szToks[0]);
	return DTRUE;
}

//---------------------------------------------------------------------------------

DBOOL LithTrackMgr::Ext_FOV(int nToks, char **szToks, float &fX, float &fY)
{
	if(nToks <= 0)
	{
		char szStr[64];
		sprintf(szStr, "%s = %f %f", COM_EXT_FOV, fX, fY);
		Send_To_Console(szStr);
		return DFALSE;
	}

	if(TrackCommandEqual(szToks[0], COM_VAL_HELP))
	{
		Send_To_Console(HELP_EXT_FOV);
		return DFALSE;
	}

	if(nToks != 2)
	{
		Send_To_Console(HELP_EXT_FOV, DTRUE);
		return DFALSE;
	}

	fX = (float)atof(szToks[0]);
	fY = (float)atof(szToks[1]);
	return DTRUE;
}

//---------------------------------------------------------------------------------

DBOOL LithTrackMgr::Ext_Type(int nToks, char **szToks, char *szType)
{
	if(nToks <= 0)
	{
		char szStr[64];
		sprintf(szStr, "%s = %s", COM_EXT_TYPE, szType);
		Send_To_Console(szStr);
		return DFALSE;
	}

	if(TrackCommandEqual(szToks[0], COM_VAL_HELP))
	{
		Send_To_Console(HELP_EXT_TYPE);
		return DFALSE;
	}

	if(nToks != 1)
	{
		Send_To_Console(HELP_EXT_TYPE, DTRUE);
		return DFALSE;
	}

	// RETAIL BUG: this only changes the local copy of the pointer, so the
	// font type is never changed ('TrackEdit Font Type <x>' just recreates
	// the same font).
#ifdef AVP2SRC_MODERN
	// M-B44 (RETAIL_BUGS.md): the setting itself takes the new type (its
	// only caller passes m_lfCS.szFontName), from a copy that outlives the
	// command's tokens
	static char s_szType[128];
	strncpy(s_szType, szToks[0], sizeof(s_szType) - 1);
	s_szType[sizeof(s_szType) - 1] = '\0';
	m_lfCS.szFontName = s_szType;
#else
	szType = szToks[0];
#endif
	return DTRUE;
}

//---------------------------------------------------------------------------------

DBOOL LithTrackMgr::Ext_Width(int nToks, char **szToks, int &nWidth)
{
	if(nToks <= 0)
	{
		char szStr[64];
		sprintf(szStr, "%s = %d", COM_EXT_WIDTH, nWidth);
		Send_To_Console(szStr);
		return DFALSE;
	}

	if(TrackCommandEqual(szToks[0], COM_VAL_HELP))
	{
		Send_To_Console(HELP_EXT_WIDTH);
		return DFALSE;
	}

	if(nToks != 1)
	{
		Send_To_Console(HELP_EXT_WIDTH, DTRUE);
		return DFALSE;
	}

	nWidth = atoi(szToks[0]);
	return DTRUE;
}

//---------------------------------------------------------------------------------

DBOOL LithTrackMgr::Ext_Height(int nToks, char **szToks, int &nHeight)
{
	if(nToks <= 0)
	{
		char szStr[64];
		sprintf(szStr, "%s = %d", COM_EXT_HEIGHT, nHeight);
		Send_To_Console(szStr);
		return DFALSE;
	}

	if(TrackCommandEqual(szToks[0], COM_VAL_HELP))
	{
		Send_To_Console(HELP_EXT_HEIGHT);
		return DFALSE;
	}

	if(nToks != 1)
	{
		Send_To_Console(HELP_EXT_HEIGHT, DTRUE);
		return DFALSE;
	}

	nHeight = atoi(szToks[0]);
	return DTRUE;
}

//---------------------------------------------------------------------------------

DBOOL LithTrackMgr::SetupMouseCursor()
{
	if(!m_pInterface) return DFALSE;

	if(m_dCursor.m_hSurf)
	{
		m_pInterface->DeleteSurface(m_dCursor.m_hSurf);
		m_dCursor.m_hSurf = DNULL;
	}

	if(m_dCursor.m_nSize < 1) m_dCursor.m_nSize = 1;
	if(m_dCursor.m_nSize > TRACKMGR_MAX_MOUSE_CURSOR_SIZE) m_dCursor.m_nSize = TRACKMGR_MAX_MOUSE_CURSOR_SIZE;

	m_dCursor.m_hSurf = m_pInterface->CreateSurface(m_dCursor.m_nSize, m_dCursor.m_nSize);
	if(!m_dCursor.m_hSurf) return DFALSE;

	m_pInterface->FillRect(m_dCursor.m_hSurf, DNULL, m_pInterface->SetupColor2(1.0f, 1.0f, 1.0f, DFALSE));

	if(m_dCursor.m_nThickness < 1) m_dCursor.m_nThickness = 1;
	if(m_dCursor.m_nThickness >= m_dCursor.m_nSize) m_dCursor.m_nThickness = m_dCursor.m_nSize - 2;

	// Cut the crosshair out of the square
	int nPos = (m_dCursor.m_nSize >> 1) - (m_dCursor.m_nThickness >> 1);
	DRect rect;
	rect.left = 0;
	rect.top = 0;
	rect.right = nPos;
	rect.bottom = nPos;
	m_pInterface->FillRect(m_dCursor.m_hSurf, &rect, m_cTransColor);

	rect.top = rect.bottom + m_dCursor.m_nThickness;
	rect.bottom = m_dCursor.m_nSize;
	m_pInterface->FillRect(m_dCursor.m_hSurf, &rect, m_cTransColor);

	rect.left = rect.right + m_dCursor.m_nThickness;
	rect.right = m_dCursor.m_nSize;
	m_pInterface->FillRect(m_dCursor.m_hSurf, &rect, m_cTransColor);

	rect.bottom = rect.top - m_dCursor.m_nThickness;
	rect.top = 0;
	m_pInterface->FillRect(m_dCursor.m_hSurf, &rect, m_cTransColor);

	rect.right = rect.left;
	rect.left = rect.left - m_dCursor.m_nThickness;
	rect.top = rect.bottom;
	rect.bottom = rect.bottom + m_dCursor.m_nThickness;
	m_pInterface->FillRect(m_dCursor.m_hSurf, &rect, m_cTransColor);

	return DTRUE;
}

//---------------------------------------------------------------------------------

void LithTrackMgr::UpdateInput()
{
	if(!m_bEditMode) return;

	DDWORD nInOut = MAX_INPUT_BUFFER_SIZE;
	m_pInterface->TrackDevice(m_diArray, &nInOut);

	DBOOL bMouse0 = m_dCtrl.m_bMouse0;
	DBOOL bMouse1 = m_dCtrl.m_bMouse1;

	m_nControlXAxis = 0;
	m_nControlYAxis = 0;

	for(DDWORD i = 0; i < nInOut; i++)
	{
		if(!stricmp(m_diArray[i].m_ControlName, "Up Arrow") || !stricmp(m_diArray[i].m_ControlName, "Numpad 8"))
			m_dCtrl.m_bUpArrow = m_diArray[i].m_InputValue;
		else if(!stricmp(m_diArray[i].m_ControlName, "Down Arrow") || !stricmp(m_diArray[i].m_ControlName, "Numpad 2"))
			m_dCtrl.m_bDownArrow = m_diArray[i].m_InputValue;
		else if(!stricmp(m_diArray[i].m_ControlName, "Left Arrow") || !stricmp(m_diArray[i].m_ControlName, "Numpad 4"))
			m_dCtrl.m_bLeftArrow = m_diArray[i].m_InputValue;
		else if(!stricmp(m_diArray[i].m_ControlName, "Right Arrow") || !stricmp(m_diArray[i].m_ControlName, "Numpad 6"))
			m_dCtrl.m_bRightArrow = m_diArray[i].m_InputValue;
		else if(!stricmp(m_diArray[i].m_ControlName, "Left Shift") || !stricmp(m_diArray[i].m_ControlName, "Right Shift"))
			m_dCtrl.m_bShiftKey = m_diArray[i].m_InputValue;
		else if(!stricmp(m_diArray[i].m_ControlName, "Left Ctrl") || !stricmp(m_diArray[i].m_ControlName, "Right Ctrl"))
			m_dCtrl.m_bCtrlKey = m_diArray[i].m_InputValue;
		else if(!stricmp(m_diArray[i].m_ControlName, "Button 0"))
			m_dCtrl.m_bMouse0 = m_diArray[i].m_InputValue;
		else if(!stricmp(m_diArray[i].m_ControlName, "Button 1"))
			m_dCtrl.m_bMouse1 = m_diArray[i].m_InputValue;
		else if(!stricmp(m_diArray[i].m_ControlName, "X-Axis"))
			m_nControlXAxis += m_diArray[i].m_InputValue;
		else if(!stricmp(m_diArray[i].m_ControlName, "Y-Axis"))
			m_nControlYAxis += m_diArray[i].m_InputValue;
	}

	// A click happens when the button is let go
	if(bMouse0 && !m_dCtrl.m_bMouse0) m_dCtrl.m_bMouse0Clicked = DTRUE;
	if(bMouse1 && !m_dCtrl.m_bMouse1) m_dCtrl.m_bMouse1Clicked = DTRUE;

	UpdateControl();
}

//---------------------------------------------------------------------------------

void LithTrackMgr::UpdateControl()
{
	m_byControlDevice = GUI_DEVICE_KEYBOARD;

	if(m_nControlXAxis || m_nControlYAxis)
	{
		if(m_dCtrl.m_bMouse1)
		{
			m_byControlDevice = GUI_DEVICE_KEYBOARD | GUI_DEVICE_MOUSE;
		}
		else
		{
			m_dCursor.m_nX += (int)(m_nControlXAxis * m_dCursor.m_fSpeed);
			m_dCursor.m_nY += (int)(m_nControlYAxis * m_dCursor.m_fSpeed);

			if(m_dCursor.m_nX < 0) m_dCursor.m_nX = 0;
			else if(m_dCursor.m_nX > (int)m_nScreenWidth) m_dCursor.m_nX = m_nScreenWidth;

			if(m_dCursor.m_nY < 0) m_dCursor.m_nY = 0;
			else if(m_dCursor.m_nY > (int)m_nScreenHeight) m_dCursor.m_nY = m_nScreenHeight;
		}
	}

	if(m_byControlType & GUI_CONTROL_CAMERA)
		UpdateCamera();
}

//---------------------------------------------------------------------------------

void LithTrackMgr::UpdateMouseClicks()
{
	if(m_dCtrl.m_bMouse0Clicked || m_dCtrl.m_bMouse1Clicked)
	{
		if(m_dCtrl.m_bMouse0Clicked)
		{
			int i;

			DBOOL bHandled = DFALSE;

			for(i = 0; i < TRACKMGR_MAX_GUIBARS; i++)
			{
				if(m_pGUIBars[i] && m_pGUIBars[i]->HasFocus())
				{
					m_pGUIBars[i]->OnMouseClick();
					bHandled = DTRUE;
					break;
				}
			}

			if(!bHandled)
				HandleScreenClick();
		}

		m_dCtrl.m_bMouse0Clicked = DFALSE;
		m_dCtrl.m_bMouse1Clicked = DFALSE;
	}
}

//---------------------------------------------------------------------------------

void LithTrackMgr::HandleScreenClick()
{
	if(!m_pInterface || !m_dCamera.m_hObj || !m_dCtrl.m_bMouse0Clicked) return;

	// Find the point in the world under the cursor
	DVector vPt;
	if(m_pInterface->Get3DCameraPt(m_dCamera.m_hObj, m_dCursor.m_nX, m_dCursor.m_nY, &vPt) == LT_OUTSIDE)
		return;

	DVector vPos;
	ClientIntersectQuery iq;
	ClientIntersectInfo ii;

	m_pInterface->GetObjectPos(m_dCamera.m_hObj, &vPos);

	DVector vDir = vPt - vPos;
	iq.m_From = vPos;
	vDir *= 1.0f / vDir.Mag();
	vDir *= 3000.0f;
	iq.m_To = vPos + vDir;
	iq.m_Flags = INTERSECT_OBJECTS | IGNORE_NONSOLID;

	if(m_pInterface->IntersectSegment(&iq, &ii) && ii.m_hObject)
	{
		uint32 nType;
		m_pInterface->Common()->GetObjectType(ii.m_hObject, &nType);

		m_dAlign.m_bCamera = DFALSE;

		if(nType == OT_WORLDMODEL)
		{
			// Align to the plane that was hit
			m_dAlign.m_hObj = DNULL;
			m_dAlign.m_vPos = ii.m_Point;

			DVector vUp(0.0f, 1.0f, 0.0f);
			m_pInterface->GetMathLT()->AlignRotation(m_dAlign.m_rRot, ii.m_Plane.m_Normal, vUp);
		}
		else
		{
			// Align to the object that was hit
			m_dAlign.m_hObj = ii.m_hObject;
			m_pInterface->GetObjectPos(m_dAlign.m_hObj, &m_dAlign.m_vPos);
			m_pInterface->GetObjectRotation(m_dAlign.m_hObj, &m_dAlign.m_rRot);
		}

		return;
	}

	m_dAlign.m_bCamera = DTRUE;
}

//---------------------------------------------------------------------------------

DBOOL LithTrackMgr::SetupCamera()
{
	if(!m_dCamera.m_hObj) return DFALSE;

	m_pInterface->SetCameraRect(m_dCamera.m_hObj, DFALSE, 0, 0, m_nScreenWidth, m_nScreenHeight);
	m_pInterface->SetCameraFOV(m_dCamera.m_hObj, MATH_DEGREES_TO_RADIANS(m_dCamera.m_fFOVX), MATH_DEGREES_TO_RADIANS(m_dCamera.m_fFOVY));

	return DTRUE;
}

//---------------------------------------------------------------------------------

void LithTrackMgr::UpdateCamera()
{
	if(!m_dCamera.m_hObj || !m_bEditMode) return;

	ILTMath *pMath = m_pInterface->GetMathLT();
	DVector vPos, vR, vU, vF;
	DRotation rRot;

	m_pInterface->GetObjectPos(m_dCamera.m_hObj, &vPos);
	m_pInterface->GetObjectRotation(m_dCamera.m_hObj, &rRot);
	pMath->GetRotationVectors(rRot, vR, vU, vF);

	DFLOAT fFrameTime = m_pInterface->GetFrameTime();

	// Move the camera with the keyboard
	if(m_byControlDevice & GUI_DEVICE_KEYBOARD)
	{
		VEC_MULSCALAR(vR, vR, fFrameTime * m_dCamera.m_fMoveSpeed);
		VEC_MULSCALAR(vU, vU, fFrameTime * m_dCamera.m_fMoveSpeed);
		VEC_MULSCALAR(vF, vF, fFrameTime * m_dCamera.m_fMoveSpeed);

		if(m_dCamera.m_byLockControls & GUI_CONTROL_LOCK_POS_X)	vR.x = vU.x = vF.x = 0.0f;
		if(m_dCamera.m_byLockControls & GUI_CONTROL_LOCK_POS_Y)	vR.y = vU.y = vF.y = 0.0f;
		if(m_dCamera.m_byLockControls & GUI_CONTROL_LOCK_POS_Z)	vR.z = vU.z = vF.z = 0.0f;

		if(m_dCtrl.m_bUpArrow)		vPos += (m_dCtrl.m_bCtrlKey ? vU : vF);
		if(m_dCtrl.m_bDownArrow)	vPos -= (m_dCtrl.m_bCtrlKey ? vU : vF);
		if(m_dCtrl.m_bRightArrow)	vPos += vR;
		if(m_dCtrl.m_bLeftArrow)	vPos -= vR;

		m_dCamera.m_vPos = vPos;
	}

	// Turn the camera with the mouse
	if(m_byControlDevice & GUI_DEVICE_MOUSE)
	{
		m_dCamera.m_fPitch += (m_dCamera.m_byLockControls & GUI_CONTROL_LOCK_ROT_X) ? 0.0f : (m_nControlYAxis * m_dCamera.m_fRotSpeed);
		m_dCamera.m_fYaw += m_dCtrl.m_bCtrlKey ? 0.0f : ((m_dCamera.m_byLockControls & GUI_CONTROL_LOCK_ROT_Y) ? 0.0f : (m_nControlXAxis * m_dCamera.m_fRotSpeed));
		m_dCamera.m_fRoll += (m_dCtrl.m_bCtrlKey && !(m_dCamera.m_byLockControls & GUI_CONTROL_LOCK_ROT_Z)) ? (m_nControlXAxis * m_dCamera.m_fRotSpeed) : 0.0f;

		if(m_dCamera.m_fPitch < 0.0f) m_dCamera.m_fPitch += 360.0f;
		else if(m_dCamera.m_fPitch >= 360.0f) m_dCamera.m_fPitch -= 360.0f;

		if(m_dCamera.m_fYaw < 0.0f) m_dCamera.m_fYaw += 360.0f;
		else if(m_dCamera.m_fYaw >= 360.0f) m_dCamera.m_fYaw -= 360.0f;

		if(m_dCamera.m_fRoll < 0.0f) m_dCamera.m_fRoll += 360.0f;
		else if(m_dCamera.m_fRoll >= 360.0f) m_dCamera.m_fRoll -= 360.0f;

		pMath->SetupEuler(rRot, MATH_DEGREES_TO_RADIANS(m_dCamera.m_fPitch), MATH_DEGREES_TO_RADIANS(m_dCamera.m_fYaw), MATH_DEGREES_TO_RADIANS(m_dCamera.m_fRoll));
	}

	m_pInterface->SetObjectPosAndRotation(m_dCamera.m_hObj, &vPos, &rRot);
}

//---------------------------------------------------------------------------------

DBOOL LithTrackMgr::SetupFont()
{
	m_EditBar.Term();
	m_TrackBar.Term();
	m_KeyBar.Term();
	m_CameraBar.Term();
	m_HelpBar.Term();

	if(m_pFont)
	{
		FreeLithFont(m_pFont);
		m_pFont = DNULL;
	}

	if(m_pInterface)
		m_pFont = CreateLithFont(m_pInterface, &m_lfCS, DTRUE);

	if(m_pFont)
	{
		m_EditBar.Init(m_pInterface, m_pFont);
		m_TrackBar.Init(m_pInterface, m_pFont);
		m_KeyBar.Init(m_pInterface, m_pFont);
		m_CameraBar.Init(m_pInterface, m_pFont);
		m_HelpBar.Init(m_pInterface, m_pFont);

		for(int i = 0; i < TRACKMGR_MAX_GUIBARS; i++)
			m_pGUIBars[i] = DNULL;

		AddGUIBarToList(&m_EditBar);
	}

	return DTRUE;
}

//---------------------------------------------------------------------------------

void LithTrackMgr::AddGUIBarToList(LithTrackBarBase *ltGUIBar)
{
	for(int i = 0; i < TRACKMGR_MAX_GUIBARS; i++)
	{
		if(!m_pGUIBars[i])
		{
			m_pGUIBars[i] = ltGUIBar;
			ltGUIBar->On();
			m_pGUIBars[i]->UpdateResolution(m_nScreenWidth, m_nScreenHeight);
			return;
		}
	}
}

//---------------------------------------------------------------------------------

void LithTrackMgr::RemoveGUIBarFromList(LithTrackBarBase *ltGUIBar)
{
	for(int i = 0; i < TRACKMGR_MAX_GUIBARS; i++)
	{
		if(m_pGUIBars[i] && (m_pGUIBars[i] == ltGUIBar))
		{
			m_pGUIBars[i]->Off();

			for(int j = i; j < TRACKMGR_MAX_GUIBARS - 1; j++)
				m_pGUIBars[j] = m_pGUIBars[j + 1];
		}
	}
}

//---------------------------------------------------------------------------------

void LithTrackMgr::UpdateGUIBars()
{
	if(!m_bEditMode) return;

	int nOldFocus = -1, nNewFocus = -1;
	int nY = m_nScreenHeight;
	int i;

	for(i = 0; i < TRACKMGR_MAX_GUIBARS; i++)
	{
		if(m_pGUIBars[i] && m_pGUIBars[i]->HasFocus())
		{
			nOldFocus = i;
			break;
		}
	}

	// Stack the bars up from the bottom of the screen
	for(i = 0; i < TRACKMGR_MAX_GUIBARS; i++)
	{
		if(m_pGUIBars[i])
		{
			m_pGUIBars[i]->SetPos(0, nY - m_pGUIBars[i]->GetHeight());
			nY = m_pGUIBars[i]->GetPosY();

			int x = m_dCursor.m_nX - m_pGUIBars[i]->GetPosX();
			int y = m_dCursor.m_nY - m_pGUIBars[i]->GetPosY();

			if((x >= 0) && (x <= (int)m_pGUIBars[i]->GetWidth()) && (y >= 0) && (y <= (int)m_pGUIBars[i]->GetHeight()))
			{
				m_pGUIBars[i]->SetFocus(DTRUE);
				m_pGUIBars[i]->OnMouseMove(x, y);
				nNewFocus = i;
			}

			m_pGUIBars[i]->Update();
		}
	}

	if((nNewFocus != nOldFocus) && (nOldFocus != -1))
		m_pGUIBars[nOldFocus]->SetFocus(DFALSE);
}

//*********************************************************************************

void TrackEditFn(int argc, char **argv)
{
	if(g_pTrackMgr && (argc > 0))
		g_pTrackMgr->TrackCommands(argc, argv);
}
