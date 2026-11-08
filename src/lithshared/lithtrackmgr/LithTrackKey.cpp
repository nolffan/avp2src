//*********************************************************************************
// LithTrackKey.cpp
//
// LithTrackMgr, as AvP2's client links it (LithTrackMgrClient.lib), written
// from the release's prebuilt build of it.
//*********************************************************************************

#include "lithtrackmgr.h"
#include "lithtrackkey.h"

//*********************************************************************************

LithTrackKey::LithTrackKey()
{
	Init();
}

//---------------------------------------------------------------------------------

LithTrackKey::~LithTrackKey()
{
	Term();
}

//---------------------------------------------------------------------------------

LTBOOL LithTrackKey::Init()
{
	m_vPos.Init();
	m_vRot.Init();

	m_fFlex = 0.0f;
	m_vCtrlPt1.Init();
	m_vCtrlPt2.Init();

	m_fXA = m_fXB = m_fXC = 0.0f;
	m_fYA = m_fYB = m_fYC = 0.0f;
	m_fZA = m_fZB = m_fZC = 0.0f;

	m_fTime = 0.0f;

	return LTTRUE;
}

//---------------------------------------------------------------------------------

LTBOOL LithTrackKey::Load(FILE *file)
{
	return LTFALSE;
}

//---------------------------------------------------------------------------------

void LithTrackKey::Term()
{
}

//---------------------------------------------------------------------------------

LTBOOL LithTrackKey::Copy(LithTrackKey *pKey)
{
	m_vPos = pKey->m_vPos;
	m_vRot = pKey->m_vRot;
	m_fFlex = pKey->m_fFlex;

	CalcCtrlPts();

	return LTTRUE;
}

//---------------------------------------------------------------------------------

void LithTrackKey::CalcCtrlPts()
{
	LTRotation rRot;
	LTVector vR, vU, vF;
	ILTMath *pMath = g_pTrackMgr->Interface()->GetMathLT();

	pMath->SetupEuler(rRot, MATH_DEGREES_TO_RADIANS(m_vRot.x), MATH_DEGREES_TO_RADIANS(m_vRot.y), MATH_DEGREES_TO_RADIANS(m_vRot.z));
	pMath->GetRotationVectors(rRot, vR, vU, vF);

	vF *= m_fFlex;

	m_vCtrlPt1 = m_vPos + vF;
	m_vCtrlPt2 = m_vPos - vF;
}

//---------------------------------------------------------------------------------

void LithTrackKey::CalcCurve(LithTrackKey *pKey)
{
	if(!pKey)
	{
		m_fXA = m_fXB = m_fXC = 0.0f;
		m_fYA = m_fYB = m_fYC = 0.0f;
		m_fZA = m_fZB = m_fZC = 0.0f;
		return;
	}

	LTFLOAT x0 = m_vPos.x, x1 = m_vCtrlPt1.x, x2 = pKey->m_vCtrlPt2.x, x3 = pKey->m_vPos.x;
	LTFLOAT y0 = m_vPos.y, y1 = m_vCtrlPt1.y, y2 = pKey->m_vCtrlPt2.y, y3 = pKey->m_vPos.y;
	LTFLOAT z0 = m_vPos.z, z1 = m_vCtrlPt1.z, z2 = pKey->m_vCtrlPt2.z, z3 = pKey->m_vPos.z;

	m_fXC = 3.0f * (x1 - x0);
	m_fYC = 3.0f * (y1 - y0);
	m_fZC = 3.0f * (z1 - z0);

	m_fXB = 3.0f * (x2 - x1) - m_fXC;
	m_fYB = 3.0f * (y2 - y1) - m_fYC;
	m_fZB = 3.0f * (z2 - z1) - m_fZC;

	m_fXA = x3 - x0 - m_fXC - m_fXB;
	m_fYA = y3 - y0 - m_fYC - m_fYB;
	m_fZA = z3 - z0 - m_fZC - m_fZB;
}

//---------------------------------------------------------------------------------

void LithTrackKey::CalcCurvePt(LTVector &vPt, LTFLOAT fTime)
{
	LTFLOAT fTime2 = fTime * fTime;
	LTFLOAT fTime3 = fTime2 * fTime;

	vPt.x = (m_fXA * fTime3) + (m_fXB * fTime2) + (m_fXC * fTime) + m_vPos.x;
	vPt.y = (m_fYA * fTime3) + (m_fYB * fTime2) + (m_fYC * fTime) + m_vPos.y;
	vPt.z = (m_fZA * fTime3) + (m_fZB * fTime2) + (m_fZC * fTime) + m_vPos.z;
}
