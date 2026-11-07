// ----------------------------------------------------------------------- //
//
// MODULE  : ArtifactField.cpp
//
// PURPOSE : Primal Hunt's artifact field: a world model that spins and
//           pulses its alpha, faded on and off by trigger messages
//
// ----------------------------------------------------------------------- //

// PH: Third Law's class, PHObject.lto 0x1014de00..0x1014e8cb (the name of
// the file is ours: retail does not record it).

#include "stdafx.h"

#ifdef PRIMAL_HUNT

#include "ArtifactField.h"
#include <math.h>

// PH: the property table and the class's registration, 0x1014de10 and
// 0x1014e100; the default constructor, 0x1014e0b0
BEGIN_CLASS(CArtifactField)
	ADD_BOOLPROP(Additive, LTTRUE)
	ADD_BOOLPROP(AdditiveAlpha, LTTRUE)
	ADD_REALPROP(AlphaCycleTime, 5.0f)
	ADD_REALPROP(MinAlpha, 0.0f)
	ADD_REALPROP(MaxAlpha, 1.0f)
	ADD_BOOLPROP(Surge, LTFALSE)
	ADD_BOOLPROP(StartOn, LTTRUE)
END_CLASS_DEFAULT(CArtifactField, GameBase, NULL, NULL)

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CArtifactField::CArtifactField()
//
//	PURPOSE:	Initialize object
//
// ----------------------------------------------------------------------- //

// PH: 0x1014e120
CArtifactField::CArtifactField() : GameBase(OT_WORLDMODEL)
{
	m_fTime				= 0.0f;
	m_fMinAlpha			= 0.0f;
	m_fAlpha			= 1.0f;
	m_fAlphaCycleTime	= 5.0f;
	m_fMaxAlpha			= 1.0f;
	m_eState			= eStateOn;
}

// PH: 0x1014e180
CArtifactField::~CArtifactField()
{
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CArtifactField::OnPrecreate()
//
//	PURPOSE:	Read the properties
//
// ----------------------------------------------------------------------- //

// PH: 0x1014e190
uint32 CArtifactField::OnPrecreate(void *pData, uint32 nInfo)
{
	if (nInfo == PRECREATE_WORLDFILE)
	{
		ObjectCreateStruct* pStruct = (ObjectCreateStruct*)pData;

		SAFE_STRCPY(pStruct->m_Filename, pStruct->m_Name);

		g_pLTServer->GetPropBool("Additive", &m_Blend.m_bAdditive);
		g_pLTServer->GetPropBool("AdditiveAlpha", &m_Blend.m_bAdditiveAlpha);
		g_pLTServer->GetPropReal("AlphaCycleTime", &m_fAlphaCycleTime);
		g_pLTServer->GetPropReal("MinAlpha", &m_fMinAlpha);
		g_pLTServer->GetPropReal("MaxAlpha", &m_fMaxAlpha);
		g_pLTServer->GetPropBool("Surge", &m_bSurge);
		g_pLTServer->GetPropBool("StartOn", &m_bStartOn);

		m_fSpinRate = MATH_CIRCLE / m_fAlphaCycleTime;

		pStruct->m_Flags = FLAG_VISIBLE | FLAG_FULLPOSITIONRES | FLAG_GOTHRUWORLD;

		if (m_Blend.m_bAdditiveAlpha == LTTRUE)
		{
			pStruct->m_Flags2 |= FLAG2_ADDITIVEALPHA;
		}

		if (m_Blend.m_bAdditive == LTTRUE)
		{
			pStruct->m_Flags2 |= FLAG2_ADDITIVE;
		}
	}

	return 0;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CArtifactField::OnInitialUpdate()
//
//	PURPOSE:	Start on or off
//
// ----------------------------------------------------------------------- //

// PH: 0x1014e2b0
uint32 CArtifactField::OnInitialUpdate(uint32 nInfo)
{
	if (nInfo != INITIALUPDATE_SAVEGAME)
	{
		m_fTime = 0.0f;

		if (m_bStartOn == LTTRUE)
		{
			SetOn();
		}
		else
		{
			SetOff();
		}

		SetNextUpdate(0.001f);
	}

	return 0;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CArtifactField::OnUpdate()
//
//	PURPOSE:	Update the field
//
// ----------------------------------------------------------------------- //

// PH: 0x1014e310
uint32 CArtifactField::OnUpdate(LTFLOAT fFrameTime)
{
	switch (m_eState)
	{
		case eStateOn:
			UpdateField(fFrameTime);
			break;

		case eStateOff:
			UpdateOff(fFrameTime);
			break;

		case eStateTurningOn:
			UpdateTurningOn(fFrameTime);
			break;

		case eStateTurningOff:
			UpdateTurningOff(fFrameTime);
			break;
	}

	return 0;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CArtifactField::OnTrigger()
//
//	PURPOSE:	Handle the ON and OFF messages, each with an optional
//				fade time
//
// ----------------------------------------------------------------------- //

// PH: 0x1014e370
uint32 CArtifactField::OnTrigger(HOBJECT hSender, const char* szMsg)
{
	GameBase::OnTrigger(hSender, szMsg);

	ILTCommon* pCommon = g_pLTServer->Common();
	if (!pCommon) return 1;

	ConParse parse;
	parse.Init((char*)szMsg);

	while (pCommon->Parse(&parse) == LT_OK)
	{
		if (parse.m_nArgs > 0 && parse.m_Args[0])
		{
			if (_stricmp(parse.m_Args[0], "ON") == 0)
			{
				LTFLOAT fTime = 2.0f;
				if (parse.m_nArgs > 1)
				{
					fTime = (LTFLOAT)atof(parse.m_Args[1]);
				}

				TurnOn(fTime);
			}
			else if (_stricmp(parse.m_Args[0], "OFF") == 0)
			{
				LTFLOAT fTime = 2.0f;
				if (parse.m_nArgs > 1)
				{
					fTime = (LTFLOAT)atof(parse.m_Args[1]);
				}

				TurnOff(fTime);
			}
		}
	}

	return 0;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CArtifactField::TurnOn()
//
//	PURPOSE:	Fade the field in over fTime seconds
//
// ----------------------------------------------------------------------- //

// PH: 0x1014e4a0
void CArtifactField::TurnOn(LTFLOAT fTime)
{
	if (m_eState == eStateOn || m_eState == eStateTurningOn) return;

	m_eState			= eStateTurningOn;
	m_fTransitionTime	= fTime;
	m_fTransitionEnd	= g_pLTServer->GetTime() + fTime;

	{
		uint32 dwFlags;
		g_pLTServer->Common()->GetObjectFlags(m_hObject, OFT_Flags, dwFlags);
		g_pLTServer->Common()->SetObjectFlags(m_hObject, OFT_Flags, dwFlags | FLAG_VISIBLE);
	}

	g_pLTServer->SetObjectState(m_hObject, OBJSTATE_ACTIVE);
	SetNextUpdate(0.001f);

	LTFLOAT fFrameTime = g_pLTServer->GetFrameTime();
	UpdateField(fFrameTime);
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CArtifactField::TurnOff()
//
//	PURPOSE:	Fade the field out over fTime seconds
//
// ----------------------------------------------------------------------- //

// PH: 0x1014e560
void CArtifactField::TurnOff(LTFLOAT fTime)
{
	if (m_eState == eStateOff || m_eState == eStateTurningOff) return;

	m_eState			= eStateTurningOff;
	m_fTransitionTime	= fTime;
	LTFLOAT fNow		= g_pLTServer->GetTime();
	m_fTransitionEnd	= fNow + fTime;

	g_pLTServer->SetObjectState(m_hObject, OBJSTATE_ACTIVE);
	SetNextUpdate(0.001f);

	LTFLOAT fFrameTime = g_pLTServer->GetFrameTime();
	UpdateField(fFrameTime);
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CArtifactField::UpdateField()
//
//	PURPOSE:	Pulse the alpha and spin the field around its Y axis
//
// ----------------------------------------------------------------------- //

// PH: 0x1014e5e0
void CArtifactField::UpdateField(LTFLOAT fFrameTime)
{
	m_fTime += fFrameTime;

	LTFLOAT fCycleTime = m_fTime - (LTFLOAT)floor(m_fTime / m_fAlphaCycleTime) * m_fAlphaCycleTime;

	LTFLOAT fAlpha = fCycleTime / m_fAlphaCycleTime;
	if (!m_bSurge)
	{
		fAlpha *= MATH_PI;
	}
	else
	{
		fAlpha *= MATH_CIRCLE;
	}

	fAlpha = (LTFLOAT)sin(fAlpha) * (m_fMaxAlpha - m_fMinAlpha) + m_fMinAlpha;

	if (fAlpha < 0.0f)
	{
		fAlpha = 0.0f;
	}
	else if (fAlpha > 1.0f)
	{
		fAlpha = 1.0f;
	}

	fAlpha *= m_fFade;

	LTFLOAT r, g, b, a;
	g_pLTServer->GetObjectColor(m_hObject, &r, &g, &b, &a);
	g_pLTServer->SetObjectColor(m_hObject, r, g, b, fAlpha);

	LTFLOAT fAngle = fCycleTime * m_fSpinRate;
	LTRotation rRot(0.0f, 0.0f, 0.0f, 1.0f);
	LTVector vUp(0.0f, 1.0f, 0.0f);
	g_pLTServer->RotateAroundAxis(&rRot, &vUp, fAngle);
	g_pLTServer->SetObjectRotation(m_hObject, &rRot);

	SetNextUpdate(0.001f);
}

// PH: 0x1014e740
void CArtifactField::UpdateOff(LTFLOAT fFrameTime)
{
	SetOff();
}

// PH: 0x1014e750
void CArtifactField::UpdateTurningOn(LTFLOAT fFrameTime)
{
	if (g_pLTServer->GetTime() >= m_fTransitionEnd)
	{
		SetOn();
		m_fFade = 1.0f;
		UpdateField(fFrameTime);
	}
	else
	{
		LTFLOAT fTime = g_pLTServer->GetTime();
		m_fFade = 1.0f - (m_fTransitionEnd - fTime) / m_fTransitionTime;
		UpdateField(fFrameTime);
	}
}

// PH: 0x1014e7c0
void CArtifactField::UpdateTurningOff(LTFLOAT fFrameTime)
{
	if (g_pLTServer->GetTime() >= m_fTransitionEnd)
	{
		SetOff();
		UpdateField(fFrameTime);
	}
	else
	{
		LTFLOAT fTime = g_pLTServer->GetTime();
		m_fFade = (m_fTransitionEnd - fTime) / m_fTransitionTime;
		UpdateField(fFrameTime);
	}
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CArtifactField::SetOff()
//
//	PURPOSE:	Hide the field and let it sleep
//
// ----------------------------------------------------------------------- //

// PH: 0x1014e820
void CArtifactField::SetOff()
{
	m_eState	= eStateOff;
	m_fFade		= 0.0f;

	uint32 dwFlags;
	g_pLTServer->Common()->GetObjectFlags(m_hObject, OFT_Flags, dwFlags);
	g_pLTServer->Common()->SetObjectFlags(m_hObject, OFT_Flags, dwFlags & ~FLAG_VISIBLE);

	g_pLTServer->SetObjectState(m_hObject, OBJSTATE_INACTIVE);
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CArtifactField::SetOn()
//
//	PURPOSE:	Show the field
//
// ----------------------------------------------------------------------- //

// PH: 0x1014e880
void CArtifactField::SetOn()
{
	m_eState	= eStateOn;
	m_fFade		= 1.0f;

	uint32 dwFlags;
	g_pLTServer->Common()->GetObjectFlags(m_hObject, OFT_Flags, dwFlags);
	g_pLTServer->Common()->SetObjectFlags(m_hObject, OFT_Flags, dwFlags | FLAG_VISIBLE);
}

#endif // PRIMAL_HUNT
