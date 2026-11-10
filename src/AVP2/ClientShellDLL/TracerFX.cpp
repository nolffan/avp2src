// ----------------------------------------------------------------------- //
//
// MODULE  : TracerFX.cpp
//
// PURPOSE : Tracer special FX - Implementation
//
// CREATED : 1/21/98
//
// (c) 1998-2000 Monolith Productions, Inc.  All Rights Reserved
//
// ----------------------------------------------------------------------- //

#include "stdafx.h"
#include "TracerFX.h"
#include "iltclient.h"
#include "ClientUtilities.h"
#include "SFXMsgIds.h"
#include "GameClientShell.h"
#include "VarTrack.h"
#ifdef PRIMAL_HUNT
#include "CameraMgr.h"
#endif

extern CGameClientShell* g_pGameClientShell;

VarTrack	g_cvarTracerSegments;
VarTrack	g_cvarTracerLifetime;
VarTrack	g_cvarTracerVelocity;

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CTracerFX::Init
//
//	PURPOSE:	Init the tracer fx
//
// ----------------------------------------------------------------------- //

LTBOOL CTracerFX::Init(HLOCALOBJ hServObj, HMESSAGEREAD hMessage)
{
    if (!CSpecialFX::Init(hServObj, hMessage)) return LTFALSE;
    if (!hMessage) return LTFALSE;

	// Don't support server-side versions of this fx...

    return LTFALSE;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CTracerFX::Init
//
//	PURPOSE:	Init the tracer fx
//
// ----------------------------------------------------------------------- //

LTBOOL CTracerFX::Init(SFXCREATESTRUCT* psfxCreateStruct)
{
    if (!CSpecialFX::Init(psfxCreateStruct)) return LTFALSE;

	TRCREATESTRUCT* pTR = (TRCREATESTRUCT*)psfxCreateStruct;
	m_cs = *pTR;

    if (!m_cs.pTracerFX) return LTFALSE;

	m_vDir  = m_cs.vEndPos - m_cs.vStartPos;
	m_fDist = m_vDir.Mag();
	m_vDir.Norm();

	if (!g_cvarTracerSegments.IsInitted())
	{
        g_cvarTracerSegments.Init(g_pLTClient, "TracerSegments", NULL, 1.0f);
	}

	if (!g_cvarTracerLifetime.IsInitted())
	{
        g_cvarTracerLifetime.Init(g_pLTClient, "TracerLifetime", NULL, 0.2f);
	}

	if (!g_cvarTracerVelocity.IsInitted())
	{
        g_cvarTracerVelocity.Init(g_pLTClient, "TracerVeloctity", NULL, -1.0f);
	}

	if (g_cvarTracerVelocity.GetFloat() > 0.0f)
	{
		m_fVelocity = g_cvarTracerVelocity.GetFloat();
	}
	else
	{
		m_fVelocity = m_cs.pTracerFX->fVelocity;
	}

#ifdef PRIMAL_HUNT
	m_fHeadVelocity = m_cs.pTracerFX->fHeadVelocity;

	for (int i = 0; i < 3; i++)
		m_hFlares[i] = LTNULL;
#endif


	// Calculate life time...

	if (m_fVelocity > 0.0f)
	{
		m_fLifetime = m_fDist / m_fVelocity;
	}
	else
	{
        return LTFALSE;
	}

    return LTTRUE;
}


// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CTracerFX::Update
//
//	PURPOSE:	Update the tracer
//
// ----------------------------------------------------------------------- //

LTBOOL CTracerFX::Update()
{
    if (!m_pClientDE || !m_cs.pTracerFX) return LTFALSE;

    LTFLOAT fTime = m_pClientDE->GetTime();

	if (m_bFirstUpdate)
	{
        m_bFirstUpdate  = LTFALSE;
		m_fStartTime	= fTime;

		PLFXCREATESTRUCT pls;

#ifdef PRIMAL_HUNT
		if (m_cs.pTracerFX->bSingleSegment == LTTRUE)
		{
			// Start a frame back
			m_cs.vStartPos -= (m_vDir * g_pLTClient->GetFrameTime() * m_fVelocity);
		}
#endif

		m_vTracerPos = m_cs.vStartPos + (m_vDir * g_pLTClient->GetFrameTime() * m_fVelocity);

		pls.pTexture				= m_cs.pTracerFX->szTexture[0] ? m_cs.pTracerFX->szTexture : LTNULL;
		pls.vStartPos				= m_cs.vStartPos;
		pls.vEndPos					= m_vTracerPos;
		pls.vInnerColorStart		= m_cs.pTracerFX->vColor;
		pls.vInnerColorEnd			= m_cs.pTracerFX->vColor;
        pls.vOuterColorStart        = m_cs.pTracerFX->vColor;
        pls.vOuterColorEnd          = m_cs.pTracerFX->vColor;
		pls.fAlphaStart				= m_cs.pTracerFX->fInitialAlpha;
		pls.fAlphaEnd				= m_cs.pTracerFX->fFinalAlpha;
		pls.fMinWidth				= 0;
		pls.fMaxWidth				= m_cs.pTracerFX->fWidth;
		pls.fMinDistMult			= 1.0f;
		pls.fMaxDistMult			= 1.0f;
		pls.fLifeTime				= g_cvarTracerLifetime.GetFloat();
		//pls.fLifeTime				= m_fLifetime;
		pls.fAlphaLifeTime			= pls.fLifeTime;
		pls.fPerturb				= 0.0f;
        pls.bAdditive               = LTTRUE;
        pls.bNoZ                    = LTTRUE;
		pls.nWidthStyle				= PLWS_CONSTANT;
		pls.nNumSegments			= (int)g_cvarTracerSegments.GetFloat();
        pls.bAlignUp                = LTTRUE;

#ifdef PRIMAL_HUNT
		if (m_cs.pTracerFX->bSingleSegment == LTTRUE)
		{
			// The head runs the whole path in the lifetime, the tail half
			// of it
			m_fFlareTime	= 0.0f;
			m_vFlareStart	= pls.vStartPos;
			m_fLifetime		= 0.5f;
			pls.fLifeTime	= 0.5f;
			pls.fAlphaLifeTime = 0.5f;

			m_fHeadVelocity	= m_fDist / m_fLifetime;
			m_fTailVelocity	= m_fVelocity = m_fHeadVelocity * 0.5f;

			pls.vEndPos		= m_vTracerPos;
		}

		for (int i = 0; i < 3; i++)
		{
			char szSprite[128];
			_snprintf(szSprite, 127, m_cs.pTracerFX->szFlare, i + 1);

			m_hFlares[i] = CreateFlare(szSprite, m_vTracerPos, m_cs.pTracerFX->fFlareScale, (i == 0));
		}

		UpdateFlares(m_vTracerPos);
#endif

		if (!m_Tracer.Init(&pls) || !m_Tracer.CreateObject(m_pClientDE))
		{
#ifdef PRIMAL_HUNT
			RemoveFlares();
#endif
            return LTFALSE;
		}
	}
	else if (fTime > m_fStartTime + m_fLifetime)
	{
#ifdef PRIMAL_HUNT
		RemoveFlares();
#endif
        return LTFALSE;
	}


	// Draw the tracer...

	m_Tracer.Update();


	// Update the tracer positions...

#ifdef PRIMAL_HUNT
	if (m_Tracer.HasBeenDrawn() == LTTRUE && m_cs.pTracerFX->bSingleSegment == LTTRUE)
	{
		// Stretch the one segment from the tail to the head
		m_fFlareTime += g_pLTClient->GetFrameTime();

		PolyLine** pLine = m_Tracer.GetLines()->GetItem(TLIT_LAST);
		if (pLine && *pLine)
		{
			LTFLOAT fTail = m_fTailVelocity * m_fFlareTime;
			LTFLOAT fHead = m_fHeadVelocity * m_fFlareTime;

			LTVector vTail = m_vFlareStart;
			LTVector vHead = m_vFlareStart;

			if (fTail < m_fDist - 0.1f)
				vTail += m_vDir * fTail;
			else
				vTail += m_vDir * (m_fDist - 0.1f);

			if (fHead < m_fDist)
				vHead += m_vDir * fHead;
			else
				vHead += m_vDir * m_fDist;

			m_Tracer.ResetLine(*pLine, vTail, vHead);

			UpdateFlares(vHead);
		}
	}
	else if (m_Tracer.HasBeenDrawn())
#else
	if (m_Tracer.HasBeenDrawn())
#endif
	{
 		// Use trail technology, instead of moving the tracer...

		PLFXLINESTRUCT ls;

		ls.vStartPos = m_vTracerPos;
		ls.vEndPos	 = m_vTracerPos + (m_vDir * g_pLTClient->GetFrameTime() * m_fVelocity);
		
		m_vTracerPos = ls.vEndPos;

		// Get the last vert position...

		PolyLineList* pLines = m_Tracer.GetLines();
		if (pLines->GetLength() > 0)
		{
			PolyLine** pLine = pLines->GetItem(TLIT_LAST);
			if (pLine && *pLine)
			{
				PolyVertStruct** pVert = (*pLine)->list.GetItem(TLIT_LAST);
				if (pVert && *pVert)
				{
					ls.vStartPos = m_Tracer.GetVertPos((*pVert));
				}
			}
		}

		ls.vInnerColorStart		= m_cs.pTracerFX->vColor;
		ls.vInnerColorEnd		= m_cs.pTracerFX->vColor;
		ls.vOuterColorStart		= m_cs.pTracerFX->vColor;
		ls.vOuterColorEnd		= m_cs.pTracerFX->vColor;
		ls.fAlphaStart			= m_cs.pTracerFX->fInitialAlpha;
		ls.fAlphaEnd			= m_cs.pTracerFX->fFinalAlpha;
		ls.fLifeTime			= g_cvarTracerLifetime.GetFloat();
        //ls.fLifeTime            = m_fLifetime - (g_pLTClient->GetTime() - m_fStartTime);
		ls.fLifeTime			= ls.fLifeTime < 0.0f ? 0.0f : ls.fLifeTime;
		ls.fAlphaLifeTime		= ls.fLifeTime;

		m_Tracer.AddLine(ls);
	}

    return LTTRUE;
}

//-------------------------------------------------------------------------------------------------
// SFX_TracerFactory
//-------------------------------------------------------------------------------------------------

const SFX_TracerFactory SFX_TracerFactory::registerThis;

CSpecialFX* SFX_TracerFactory::MakeShape() const
{
	return new CTracerFX();
}

#ifdef PRIMAL_HUNT
// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CTracerFX::Term
//
//	PURPOSE:	Remove the tracer and its flares
//
// ----------------------------------------------------------------------- //

void CTracerFX::Term()
{
	CSpecialFX::Term();
	RemoveFlares();
}

// ----------------------------------------------------------------------- //

void CTracerFX::OnObjectRemove(HOBJECT hObj)
{
	RemoveFlares();
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CTracerFX::CreateFlare
//
//	PURPOSE:	Create one of the flare sprites
//
// ----------------------------------------------------------------------- //

HOBJECT CTracerFX::CreateFlare(char* szSprite, LTVector & vPos, LTFLOAT fScale, LTBOOL bGlow)
{
	if (!szSprite || !szSprite[0]) return LTNULL;

	ObjectCreateStruct createStruct;
	INIT_OBJECTCREATESTRUCT(createStruct);

	createStruct.m_ObjectType = OT_SPRITE;
	SAFE_STRCPY(createStruct.m_Filename, szSprite);
	createStruct.m_Scale.Init(fScale, fScale, fScale);
	createStruct.m_Flags |= FLAG_VISIBLE | FLAG_NOLIGHT;
	createStruct.m_Flags2 |= FLAG2_ADDITIVE | FLAG2_ADDITIVEALPHA;

	if (bGlow == LTTRUE)
		createStruct.m_Flags |= FLAG_GLOWSPRITE;

	createStruct.m_Rotation.Init();
	createStruct.m_Pos = vPos;

	return g_pLTClient->CreateObject(&createStruct);
}

// ----------------------------------------------------------------------- //

void CTracerFX::RemoveFlares()
{
	for (int i = 0; i < 3; i++)
	{
		if (m_hFlares[i])
			g_pLTClient->DeleteObject(m_hFlares[i]);

		m_hFlares[i] = LTNULL;
	}
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CTracerFX::UpdateFlares
//
//	PURPOSE:	Move the flares to the tracer's head: the first fades with
//				the tracer, the other two trail it along the camera's
//				view, shown only when the camera looks down the tracer
//
// ----------------------------------------------------------------------- //

void CTracerFX::UpdateFlares(LTVector & vPos)
{
	g_pLTClient->SetObjectPos(m_hFlares[0], &vPos, LTTRUE);

	LTFLOAT fAlpha = 1.0f - m_fFlareTime / m_fLifetime;
	if (fAlpha < 0.0f) fAlpha = 0.0f;

	g_pLTClient->SetObjectColor(m_hFlares[0], 1.0f, 1.0f, 1.0f, fAlpha);

	CameraMgr* pCameraMgr = g_pCameraMgr;
	if (!pCameraMgr) return;

	LTVector vCameraPos;
	pCameraMgr->GetCameraPos(g_pGameClientShell->GetPlayerCamera(), vCameraPos, LTTRUE);

	LTVector vDir = vPos - vCameraPos;
	LTFLOAT fDist = vDir.Mag();
	vDir *= 1.0f / fDist;

	LTRotation rRot;
	pCameraMgr->GetCameraRotation(g_pGameClientShell->GetPlayerCamera(), rRot, LTFALSE);

	LTVector vR, vU, vF;
	g_pLTClient->GetRotationVectors(&rRot, &vR, &vU, &vF);

	LTFLOAT fDot = vF.Dot(vDir);

	if (fDot < 0.7071f || fDist < 4.0f)
	{
		SetFlaresHidden(LTTRUE);
		return;
	}

	SetFlaresHidden(LTFALSE);

	LTFLOAT fFade = (1.0f - (1.0f - fDot) * (1.0f / (1.0f - 0.7071f))) * fAlpha;
	LTFLOAT fScale = (fDist > 1024.0f) ? 1.0f : fDist * (1.0f / 1020.0f);

	LTFLOAT fFlareAlpha = fScale * fFade;
	LTFLOAT fSize = fScale * 0.5f;
	LTVector vScale(fSize, fSize, fSize);

	for (int i = 1; i < 3; i++)
	{
		LTVector vFlarePos = vPos - (vF * 16.0f * (LTFLOAT)i);

		g_pLTClient->SetObjectPos(m_hFlares[i], &vFlarePos);
		g_pLTClient->SetObjectColor(m_hFlares[i], 1.0f, 1.0f, 1.0f, fFlareAlpha);
		g_pLTClient->SetObjectScale(m_hFlares[i], &vScale);
	}
}

// ----------------------------------------------------------------------- //

void CTracerFX::SetFlaresHidden(LTBOOL bHide)
{
	uint32 dwFlags;

	if (bHide == LTTRUE)
	{
		for (int i = 1; i < 3; i++)
		{
			g_pLTClient->Common()->GetObjectFlags(m_hFlares[i], OFT_Flags, dwFlags);
			g_pLTClient->Common()->SetObjectFlags(m_hFlares[i], OFT_Flags, dwFlags & ~FLAG_VISIBLE);
		}
	}
	else
	{
		for (int i = 1; i < 3; i++)
		{
			g_pLTClient->Common()->GetObjectFlags(m_hFlares[i], OFT_Flags, dwFlags);
			g_pLTClient->Common()->SetObjectFlags(m_hFlares[i], OFT_Flags, dwFlags | FLAG_VISIBLE);
		}
	}
}
#endif
