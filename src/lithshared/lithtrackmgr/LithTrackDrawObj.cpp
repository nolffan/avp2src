//*********************************************************************************
// LithTrackDrawObj.cpp
//
// LithTrackMgr, as AvP2's client links it (LithTrackMgrClient.lib), written
// from the release's prebuilt build of it.
//*********************************************************************************

#include "lithtrackdrawobj.h"

//*********************************************************************************

LithTrackDrawObj::LithTrackDrawObj()
{
	Init(DNULL);
}

//*********************************************************************************

LithTrackDrawObj::~LithTrackDrawObj()
{
	Term();
}

//*********************************************************************************

DBOOL LithTrackDrawObj::Init(INTERFACE *pInterface)
{
	m_pInterface = pInterface;

	m_hObj = DNULL;
	m_bVisible = DFALSE;
	m_bSelected = DFALSE;

	m_fPitch = 0.0f;
	m_fYaw = 0.0f;
	m_fRoll = 0.0f;

	m_pTrack = DNULL;
	m_nNumKeys = 0;

	m_pLines = DNULL;
	m_nNumLines = 0;
	m_nNumKeyLines = 0;
	m_nNumPathLines = 0;

	m_vKeyColor.Init();
	m_vKeyHColor.Init();
	m_vPathColor.Init();
	m_vMarkerColor.Init();
	m_vFlexColor.Init();
	m_vGreyColor.Init();

	m_nLastNumKeys = 0;
	m_nPathDetail = TRACKMGR_DEFAULT_PATH_DETAIL;

	if(m_pInterface)
	{
		VEC_SET(m_vKeyColor, 1.0f, 0.0f, 0.0f);
		VEC_SET(m_vKeyHColor, 1.0f, 1.0f, 0.0f);
		VEC_SET(m_vPathColor, 0.75f, 0.0f, 0.0f);
		VEC_SET(m_vMarkerColor, 0.0f, 0.0f, 1.0f);
		VEC_SET(m_vFlexColor, 0.0f, 1.0f, 0.0f);
		VEC_SET(m_vGreyColor, 0.5f, 0.5f, 0.5f);

		// Create the line system that draws the track
		ObjectCreateStruct ocs;
		ocs.Clear();

		ocs.m_ObjectType = OT_LINESYSTEM;
		ocs.m_Flags = 0;
		VEC_INIT(ocs.m_Pos);
		ROT_INIT(ocs.m_Rotation);

		m_hObj = m_pInterface->CreateObject(&ocs);
	}

	// RETAIL BUG: Init reports failure even when the line system was created
	return DFALSE;
}

//*********************************************************************************

void LithTrackDrawObj::Term()
{
	if(m_pInterface)
	{
		Track(DNULL);

		if(m_hObj)
		{
			m_pInterface->DeleteObject(m_hObj);
			m_hObj = DNULL;
		}
	}

	m_pInterface = DNULL;
}

//*********************************************************************************

void LithTrackDrawObj::Track(LithTrack *pTrack)
{
	m_pTrack = pTrack;
	Reset();
}

//*********************************************************************************

void LithTrackDrawObj::Reset()
{
	if(!m_hObj || !m_pInterface) return;

	DDWORD i;

	// Remove the old lines
	for(i = 0; i < m_nNumLines; i++)
	{
		if(m_pLines[i])
		{
			m_pInterface->RemoveLine(m_hObj, m_pLines[i]);
			m_pLines[i] = DNULL;
		}
	}

	delete m_pLines;
	m_pLines = DNULL;

	m_nNumLines = 0;
	m_nNumKeyLines = 0;
	m_nNumPathLines = 0;

	if(!m_pTrack) return;

	// Make room for the lines of the new track
	m_nNumKeys = m_pTrack->NumKeys();
	m_nNumKeyLines = m_nNumKeys * TRACKMGR_DRAW_KEY_LINES;
	m_nNumPathLines = (m_nNumKeys - 1) * m_nPathDetail;
	m_nNumLines = m_nNumKeys + m_nNumPathLines + m_nNumKeyLines + TRACKMGR_DRAW_MARKER_LINES;

	m_pLines = new HDELINE[m_nNumLines];
	memset(m_pLines, 0, m_nNumLines * sizeof(HDELINE));

	LTLine line;
	line.m_Points[0].m_Pos.Init();
	line.m_Points[0].r = line.m_Points[0].g = line.m_Points[0].b = 0.0f;
	line.m_Points[0].a = 1.0f;
	line.m_Points[1].m_Pos.Init();
	line.m_Points[1].r = line.m_Points[1].g = line.m_Points[1].b = 0.0f;
	line.m_Points[1].a = 1.0f;

	for(i = 0; i < m_nNumLines; i++)
		m_pLines[i] = m_pInterface->AddLine(m_hObj, &line);

	Select(DTRUE);
	Update();
}

//*********************************************************************************

void LithTrackDrawObj::Update()
{
	if(!m_hObj || !m_pInterface || !m_pLines || !m_pTrack) return;

	ILTMath *pMath = m_pInterface->GetMathLT();
	LTLine line;

	DVector vPos, vRot, vStart, vEnd;
	DVector vRight, vUp, vForward;
	DRotation rRot;
	int nLine = 0, nFlexLine;
	int i = 0, j = 0;

	// Draw a box at each key
	for(i = 0; i < m_nNumKeys; i++)
	{
		vPos = m_pTrack->Key(i)->m_vPos;
		vRot = m_pTrack->Key(i)->m_vRot;

		pMath->SetupEuler(rRot, MATH_DEGREES_TO_RADIANS(vRot.x), MATH_DEGREES_TO_RADIANS(vRot.y), MATH_DEGREES_TO_RADIANS(vRot.z));
		pMath->GetRotationVectors(rRot, vRight, vUp, vForward);

		vRight *= TRACKMGR_DRAW_KEY_DIM;
		vUp *= TRACKMGR_DRAW_KEY_DIM;
		vForward *= TRACKMGR_DRAW_KEY_DIM;

		for(j = 0; j < TRACKMGR_DRAW_KEY_LINES; j++)
		{
			m_pInterface->GetLineInfo(m_pLines[nLine], &line);

			switch(j)
			{
				case 0:
					vStart = vPos + vRight + vUp + vForward;
					vEnd = vPos - vRight + vUp + vForward;
					break;

				case 1:
					vStart = vPos + vRight + vUp + vForward;
					vEnd = vPos + vRight - vUp + vForward;
					break;

				case 2:
					vStart = vPos + vRight + vUp + vForward;
					vEnd = vPos + vRight + vUp - vForward;
					break;

				case 3:
					vStart = vPos - vRight + vUp - vForward;
					vEnd = vPos - vRight + vUp + vForward;
					break;

				case 4:
					vStart = vPos - vRight + vUp - vForward;
					vEnd = vPos - vRight - vUp - vForward;
					break;

				case 5:
					vStart = vPos - vRight + vUp - vForward;
					vEnd = vPos + vRight + vUp - vForward;
					break;

				case 6:
					vStart = vPos + vRight - vUp - vForward;
					vEnd = vPos + vRight + vUp - vForward;
					break;

				case 7:
					vStart = vPos + vRight - vUp - vForward;
					vEnd = vPos + vRight - vUp + vForward;
					break;

				case 8:
					vStart = vPos + vRight - vUp - vForward;
					vEnd = vPos - vRight - vUp - vForward;
					break;

				case 9:
					vStart = vPos - vRight - vUp + vForward;
					vEnd = vPos - vRight + vUp + vForward;
					break;

				case 10:
					vStart = vPos - vRight - vUp + vForward;
					vEnd = vPos - vRight - vUp - vForward;
					break;

				case 11:
					vStart = vPos - vRight - vUp + vForward;
					vEnd = vPos + vRight - vUp + vForward;
					break;
			}

			line.m_Points[0].m_Pos = vStart;
			line.m_Points[1].m_Pos = vEnd;
			m_pInterface->SetLineInfo(m_pLines[nLine], &line);

			nLine++;
		}
	}

	// Draw the flex lines of each key
	nFlexLine = m_nNumKeyLines + m_nNumPathLines;

	for(i = 0; i < m_nNumKeys; i++)
	{
		m_pTrack->Key(i)->CalcCtrlPts();

		m_pInterface->GetLineInfo(m_pLines[nFlexLine], &line);
		line.m_Points[0].m_Pos = m_pTrack->Key(i)->m_vCtrlPt1;
		line.m_Points[1].m_Pos = m_pTrack->Key(i)->m_vCtrlPt2;
		m_pInterface->SetLineInfo(m_pLines[nFlexLine], &line);

		nFlexLine++;
	}

	// Draw the path curve between the keys
	nLine = m_nNumKeyLines;

	for(i = 0; i < m_nNumKeys - 1; i++)
	{
		m_pTrack->Key(i)->CalcCurve(m_pTrack->Key(i + 1));
		vStart = m_pTrack->Key(i)->m_vPos;

		for(j = 1; j <= m_nPathDetail; j++)
		{
			m_pInterface->GetLineInfo(m_pLines[nLine], &line);

			DFLOAT fTime = (DFLOAT)j / m_nPathDetail;

			if(fTime == 1.0f)
				vEnd = m_pTrack->Key(i + 1)->m_vPos;
			else
				m_pTrack->Key(i)->CalcCurvePt(vEnd, fTime);

			line.m_Points[0].m_Pos = vStart;
			line.m_Points[1].m_Pos = vEnd;
			m_pInterface->SetLineInfo(m_pLines[nLine], &line);

			vStart = vEnd;
			nLine++;
		}
	}
}

//*********************************************************************************

void LithTrackDrawObj::Select(DBOOL bSelect)
{
	if(!m_pInterface || !m_pLines) return;

	m_bSelected = bSelect;

	LTLine line;
	DDWORD i;

	if(!bSelect)
	{
		for(i = 0; i < m_nNumLines; i++)
		{
			m_pInterface->GetLineInfo(m_pLines[i], &line);
			line.m_Points[0].r = line.m_Points[1].r = m_vGreyColor.x;
			line.m_Points[0].g = line.m_Points[1].g = m_vGreyColor.y;
			line.m_Points[0].b = line.m_Points[1].b = m_vGreyColor.z;
			line.m_Points[0].a = line.m_Points[1].a = 1.0f;
			m_pInterface->SetLineInfo(m_pLines[i], &line);
		}

		return;
	}

	for(i = 0; i < m_nNumKeyLines; i++)
	{
		m_pInterface->GetLineInfo(m_pLines[i], &line);
		line.m_Points[0].r = line.m_Points[1].r = m_vKeyColor.x;
		line.m_Points[0].g = line.m_Points[1].g = m_vKeyColor.y;
		line.m_Points[0].b = line.m_Points[1].b = m_vKeyColor.z;
		m_pInterface->SetLineInfo(m_pLines[i], &line);
	}

	for(; i < m_nNumKeyLines + m_nNumPathLines; i++)
	{
		m_pInterface->GetLineInfo(m_pLines[i], &line);
		line.m_Points[0].r = line.m_Points[1].r = m_vPathColor.x;
		line.m_Points[0].g = line.m_Points[1].g = m_vPathColor.y;
		line.m_Points[0].b = line.m_Points[1].b = m_vPathColor.z;
		m_pInterface->SetLineInfo(m_pLines[i], &line);
	}

	for(; i < m_nNumKeyLines + m_nNumPathLines + m_nNumKeys; i++)
	{
		m_pInterface->GetLineInfo(m_pLines[i], &line);
		line.m_Points[0].r = line.m_Points[1].r = m_vFlexColor.x;
		line.m_Points[0].g = line.m_Points[1].g = m_vFlexColor.y;
		line.m_Points[0].b = line.m_Points[1].b = m_vFlexColor.z;
		m_pInterface->SetLineInfo(m_pLines[i], &line);
	}

	for(; i < m_nNumKeyLines + m_nNumPathLines + m_nNumKeys + TRACKMGR_DRAW_MARKER_LINES; i++)
	{
		m_pInterface->GetLineInfo(m_pLines[i], &line);
		line.m_Points[0].r = line.m_Points[1].r = m_vMarkerColor.x;
		line.m_Points[0].g = line.m_Points[1].g = m_vMarkerColor.y;
		line.m_Points[0].b = line.m_Points[1].b = m_vMarkerColor.z;
		m_pInterface->SetLineInfo(m_pLines[i], &line);
	}
}

//*********************************************************************************

void LithTrackDrawObj::SelectKey(int nKey)
{
	if(!m_pInterface || !m_pLines || !m_bSelected) return;

	LTLine line;
	DDWORD i;

	for(i = 0; i < m_nNumKeyLines; i++)
	{
		m_pInterface->GetLineInfo(m_pLines[i], &line);
		line.m_Points[0].r = line.m_Points[1].r = m_vKeyColor.x;
		line.m_Points[0].g = line.m_Points[1].g = m_vKeyColor.y;
		line.m_Points[0].b = line.m_Points[1].b = m_vKeyColor.z;
		m_pInterface->SetLineInfo(m_pLines[i], &line);
	}

	for(i = 0; i < TRACKMGR_DRAW_KEY_LINES; i++)
	{
		m_pInterface->GetLineInfo(m_pLines[(nKey * TRACKMGR_DRAW_KEY_LINES) + i], &line);
		line.m_Points[0].r = line.m_Points[1].r = m_vKeyHColor.x;
		line.m_Points[0].g = line.m_Points[1].g = m_vKeyHColor.y;
		line.m_Points[0].b = line.m_Points[1].b = m_vKeyHColor.z;
		m_pInterface->SetLineInfo(m_pLines[(nKey * TRACKMGR_DRAW_KEY_LINES) + i], &line);
	}
}

//*********************************************************************************

void LithTrackDrawObj::SetPathDetail(int nDetail)
{
	if(nDetail < 1) nDetail = 1;
	else if(nDetail > TRACKMGR_MAX_PATH_DETAIL) nDetail = TRACKMGR_MAX_PATH_DETAIL;

	m_nPathDetail = nDetail;
	Reset();
}

//*********************************************************************************

void LithTrackDrawObj::Visible(DBOOL bVisible)
{
	if(!m_pInterface || !m_hObj) return;

	m_bVisible = bVisible;

	DDWORD dwFlags = m_pInterface->GetObjectFlags(m_hObj);

	if(bVisible)
		dwFlags |= FLAG_VISIBLE;
	else
		dwFlags &= ~FLAG_VISIBLE;

	m_pInterface->SetObjectFlags(m_hObj, dwFlags);
}

//*********************************************************************************

DVector LithTrackDrawObj::Pos()
{
	DVector vPos(0.0f, 0.0f, 0.0f);

	if(m_hObj && m_pInterface)
		m_pInterface->GetObjectPos(m_hObj, &vPos);

	return vPos;
}

//*********************************************************************************

void LithTrackDrawObj::Pos(DVector &vPos)
{
	if(m_hObj && m_pInterface)
		m_pInterface->SetObjectPos(m_hObj, &vPos, DTRUE);
}

//*********************************************************************************

void LithTrackDrawObj::Pos(DFLOAT x, DFLOAT y, DFLOAT z)
{
	if(m_hObj && m_pInterface)
	{
		DVector vPos(x, y, z);
		m_pInterface->SetObjectPos(m_hObj, &vPos, DTRUE);
	}
}

//*********************************************************************************

DVector LithTrackDrawObj::Rot()
{
	DVector vRot(m_fPitch, m_fYaw, m_fRoll);
	return vRot;
}

//*********************************************************************************

void LithTrackDrawObj::Rot(DVector &vRot)
{
	if(m_hObj && m_pInterface)
	{
		m_fPitch = vRot.x;
		m_fYaw = vRot.y;
		m_fRoll = vRot.z;

		DRotation rRot;
		m_pInterface->GetMathLT()->SetupEuler(rRot, MATH_DEGREES_TO_RADIANS(m_fPitch), MATH_DEGREES_TO_RADIANS(m_fYaw), MATH_DEGREES_TO_RADIANS(m_fRoll));
		m_pInterface->SetObjectRotation(m_hObj, &rRot);
	}
}

//*********************************************************************************

void LithTrackDrawObj::Rot(DFLOAT x, DFLOAT y, DFLOAT z)
{
	if(m_hObj && m_pInterface)
	{
		m_fPitch = x;
		m_fYaw = y;
		m_fRoll = z;

		DRotation rRot;
		m_pInterface->GetMathLT()->SetupEuler(rRot, MATH_DEGREES_TO_RADIANS(m_fPitch), MATH_DEGREES_TO_RADIANS(m_fYaw), MATH_DEGREES_TO_RADIANS(m_fRoll));
		m_pInterface->SetObjectRotation(m_hObj, &rRot);
	}
}
