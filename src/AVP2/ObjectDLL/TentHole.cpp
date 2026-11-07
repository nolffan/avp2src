// ----------------------------------------------------------------------- //
//
// MODULE  : TentHole.cpp
//
// PURPOSE : CTentHole - implementation
//
// CREATED : Primal Hunt (Third Law); no source survives, written from
//           PHObject.lto (0x10157e70..0x101586c6)
//
// ----------------------------------------------------------------------- //

#include "stdafx.h"

#ifdef PRIMAL_HUNT

#include "TentHole.h"

#define TENTHOLE_OPEN_TIME		0.5f
#define TENTHOLE_CLOSE_TIME		3.0f
#define TENTHOLE_FADE_TIME		3.0f
#define TENTHOLE_DEPTH			16.0f

// PH: the $E initializer 0x10157e70
BEGIN_CLASS(CTentHole)
END_CLASS_DEFAULT_FLAGS(CTentHole, GameBase, NULL, NULL, CF_HIDDEN)

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CreateTentHole / IsTentHole / GetTentHole
//
// ----------------------------------------------------------------------- //

// PH: 0x10157f60
CTentHole* CreateTentHole(ObjectCreateStruct* pStruct)
{
	HCLASS hClass = g_pLTServer->GetClass("CTentHole");
	if (!hClass) return LTNULL;

	return (CTentHole*)g_pLTServer->CreateObject(hClass, pStruct);
}

// PH: 0x10157f90
LTBOOL IsTentHole(HOBJECT hObj)
{
	HCLASS hTentHole = g_pLTServer->GetClass("CTentHole");
	if (!hTentHole) return LTFALSE;

	HCLASS hClass = g_pLTServer->GetObjectClass(hObj);
	if (!hClass) return LTFALSE;

	return g_pLTServer->IsKindOf(hClass, hTentHole) == LTTRUE;
}

// PH: 0x10157fe0
CTentHole* GetTentHole(HOBJECT hObj)
{
	if (!IsTentHole(hObj)) return LTNULL;

	return (CTentHole*)g_pLTServer->HandleToObject(hObj);
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CTentHole::CTentHole()
//
// ----------------------------------------------------------------------- //

// PH: 0x10158010
CTentHole::CTentHole() : GameBase(OT_MODEL)
{
	m_eState = eOpening;
}

// PH: 0x10158060
CTentHole::~CTentHole()
{
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CTentHole::Spawn()
//
//	PURPOSE:	Create a hole at the position, facing out along the normal
//
// ----------------------------------------------------------------------- //

// PH: 0x10158070
CTentHole* CTentHole::Spawn(const LTVector & vPos, const LTVector & vNormal, LTFLOAT fScale)
{
	ObjectCreateStruct theStruct;

	theStruct.m_Pos = vPos;

	LTFLOAT fX, fZ;
	if (vNormal.y > 0.999f)
	{
		fX = 1.0f;
		fZ = 0.0f;
	}
	else
	{
		fX = vNormal.x;
		fZ = vNormal.z;
	}

	LTVector vRight(fX, 0.0f, fZ);
	LTVector vForward = vRight.Cross(vNormal);

	LTRotation rRot;
	g_pLTServer->AlignRotation(&rRot, &vForward, (LTVector*)&vNormal);

	theStruct.m_Rotation = rRot;
	theStruct.m_Scale.Init(fScale, fScale, fScale);

	CTentHole* pHole = CreateTentHole(&theStruct);
	if (!pHole) return LTNULL;

	pHole->m_fScale = fScale;

	return pHole;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CTentHole::OnPrecreate()
//
// ----------------------------------------------------------------------- //

// PH: 0x10158180
uint32 CTentHole::OnPrecreate(void *pData, uint32 nInfo)
{
	if (nInfo == PRECREATE_NORMAL)
	{
		ObjectCreateStruct* pStruct = (ObjectCreateStruct*)pData;

		SAFE_STRCPY(pStruct->m_Filename, "models/exp/props/tenthole.abc");
		SAFE_STRCPY(pStruct->m_SkinName, "skins/exp/props/tenthole.dtx");
		pStruct->m_Flags |= FLAG_VISIBLE;
	}

	return 0;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CTentHole::OnInitialUpdate()
//
// ----------------------------------------------------------------------- //

// PH: 0x10158200
uint32 CTentHole::OnInitialUpdate(uint32 nInfo)
{
	LTRotation rRot;
	LTVector vR, vF;

	g_pLTServer->SetNetFlags(m_hObject, NETFLAG_POSUNGUARANTEED | NETFLAG_ROTUNGUARANTEED | NETFLAG_ANIMUNGUARANTEED);

	if (nInfo != INITIALUPDATE_SAVEGAME)
	{
		g_pLTServer->GetObjectPos(m_hObject, &m_vPos);
		g_pLTServer->GetObjectRotation(m_hObject, &rRot);
		g_pLTServer->GetRotationVectors(&rRot, &m_vUp, &vR, &vF);

		Open();
	}

	return 0;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CTentHole::OnUpdate()
//
// ----------------------------------------------------------------------- //

// PH: 0x101582a0
uint32 CTentHole::OnUpdate(LTFLOAT fFrameTime)
{
	switch (m_eState)
	{
		case eOpening:
		{
			if (m_tmrOpen.IsDone())
			{
				LTVector vPos = m_vPos + m_vUp * TENTHOLE_DEPTH * m_fScale;
				g_pLTServer->SetObjectPos(m_hObject, &vPos);

				Hide();
				return 0;
			}

			LTFLOAT fOffset = -(m_tmrOpen.GetTimeLeft() * 2.0f * (m_fScale * TENTHOLE_DEPTH));

			LTVector vPos = m_vPos + m_vUp * fOffset + m_vUp * TENTHOLE_DEPTH * m_fScale;
			g_pLTServer->SetObjectPos(m_hObject, &vPos);
		}
		break;

		case eHidden:
		{
			g_pLTServer->SetObjectState(m_hObject, OBJSTATE_INACTIVE);
		}
		return 0;

		case eOpen:
		{
			if (m_tmrClose.IsDone())
			{
				FadeOut();
				SetNextUpdate(0.001f);
				return 0;
			}

			SetNextUpdate(0.1f);
		}
		return 0;

		case eFading:
		{
			if (m_tmrRemove.IsDone())
			{
				g_pLTServer->RemoveObject(m_hObject);
				return 0;
			}

			LTFLOAT fAlpha = m_tmrRemove.GetTimeLeft() * (1.0f / TENTHOLE_FADE_TIME);

			LTFLOAT r, g, b, a;
			g_pLTServer->GetObjectColor(m_hObject, &r, &g, &b, &a);
			g_pLTServer->SetObjectColor(m_hObject, r, g, b, fAlpha);
		}
		break;

		default:
		return 0;
	}

	SetNextUpdate(0.001f);

	return 0;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CTentHole state changes
//
// ----------------------------------------------------------------------- //

// PH: 0x101585c0
void CTentHole::Hide()
{
	m_eState = eHidden;
	g_pLTServer->SetObjectState(m_hObject, OBJSTATE_INACTIVE);
}

// PH: 0x101585e0
void CTentHole::Open()
{
	m_eState = eOpening;
	m_tmrOpen.Start(TENTHOLE_OPEN_TIME);
	g_pLTServer->SetObjectState(m_hObject, OBJSTATE_ACTIVE);
	SetNextUpdate(0.001f);
}

// PH: 0x10158630
void CTentHole::Close()
{
	m_eState = eOpen;
	m_tmrClose.Start(TENTHOLE_CLOSE_TIME);
	g_pLTServer->SetObjectState(m_hObject, OBJSTATE_ACTIVE);
	SetNextUpdate(0.1f);
}

// PH: 0x10158680
void CTentHole::FadeOut()
{
	m_eState = eFading;
	m_tmrRemove.Start(TENTHOLE_FADE_TIME);
	g_pLTServer->SetObjectState(m_hObject, OBJSTATE_ACTIVE);
	SetNextUpdate(0.001f);
}

#endif // PRIMAL_HUNT
