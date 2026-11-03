// LTGUIFadeItemCtrl.cpp: implementation of the CLTGUIFadeItemCtrl class.
//
//////////////////////////////////////////////////////////////////////

#include "ltguimgr.h"
#include "ltguifadeitemctrl.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CLTGUIFadeItemCtrl::CLTGUIFadeItemCtrl()
{
	m_pSurfArray	= LTNULL;
	m_hDisabledSurf	= LTNULL;
	m_nSurfaces		= 0;
	m_fLastSelTime	= 0.0f;
	m_dwWidth		= 0;
	m_dwHeight		= 0;
	m_pCommandHandler = LTNULL;
	m_dwFadeTime	= 300;
}

CLTGUIFadeItemCtrl::~CLTGUIFadeItemCtrl()
{
	Destroy();
}

// Create the control
LTBOOL CLTGUIFadeItemCtrl::Create(ILTClient *pClientDE, uint32 dwCommandID, HSURFACE *pSurfArray, int nSurfaces,
								  HSURFACE hDisabledSurf, LTBOOL bOwnSurfaces, CLTGUICommandHandler *pCommandHandler,
								  uint32 dwParam1, uint32 dwParam2)
{
	m_pClientDE = pClientDE;
	m_hDisabledSurf = hDisabledSurf;

	m_pSurfArray = new HSURFACE[nSurfaces];
	if (!m_pSurfArray)
		return LTFALSE;

	memcpy(m_pSurfArray, pSurfArray, nSurfaces*sizeof(HSURFACE));
	m_nSurfaces = nSurfaces;

	// Calculate the width and height from the surfaces
	uint32 nWidth = 0, nHeight = 0;
	for (int i = 0; i < m_nSurfaces; i++)
	{
		m_pClientDE->GetSurfaceDims(m_pSurfArray[i], &nWidth, &nHeight);

		m_dwWidth = (m_dwWidth > nWidth) ? m_dwWidth : nWidth;
		m_dwHeight = (m_dwHeight > nHeight) ? m_dwHeight : nHeight;
	}

	if (m_hDisabledSurf)
	{
		m_pClientDE->GetSurfaceDims(m_hDisabledSurf, &nWidth, &nHeight);

		m_dwWidth = (m_dwWidth > nWidth) ? m_dwWidth : nWidth;
		m_dwHeight = (m_dwHeight > nHeight) ? m_dwHeight : nHeight;
	}

	m_pCommandHandler = pCommandHandler;
	m_dwCommandID = dwCommandID;
	m_bCreated = LTTRUE;
	m_dwParam1 = dwParam1;
	m_dwParam2 = dwParam2;

	return LTTRUE;
}

// Destroys the control
void CLTGUIFadeItemCtrl::Destroy()
{
	if (m_pSurfArray)
	{
		// Delete the surfaces
		if (m_bOwnSurfaces)
		{
			for (int i = 0; i < m_nSurfaces; i++)
			{
				m_pClientDE->DeleteSurface(m_pSurfArray[i]);
				m_pSurfArray[i] = LTNULL;
			}

			if (m_hDisabledSurf)
			{
				m_pClientDE->DeleteSurface(m_hDisabledSurf);
				m_hDisabledSurf = LTNULL;
			}
		}

		delete [] m_pSurfArray;
		m_pSurfArray = LTNULL;
		m_nSurfaces = 0;
	}

	m_bCreated = LTFALSE;
}

// Render the control
void CLTGUIFadeItemCtrl::Render(HSURFACE hDestSurf)
{
	HSURFACE hSurface;

	if (!m_bEnabled)
	{
		if (m_hDisabledSurf)
			hSurface = m_hDisabledSurf;
		else
			hSurface = m_pSurfArray[m_nSurfaces-1];
	}
	else if (m_bSelected)
	{
		hSurface = m_pSurfArray[0];
	}
	else
	{
		// NOTE: the shipped build (VC6 SP3) wrote the 0.001 literal as a
		// double and demoted it to a single-precision constant; see the
		// matching note in ltguifadecolorctrl.cpp.
		float fPercent = (m_pClientDE->GetTime() - m_fLastSelTime) / (m_dwFadeTime * 0.001f);

		if (fPercent > 1.0f)
			hSurface = m_pSurfArray[m_nSurfaces-1];
		else
			hSurface = m_pSurfArray[(int)((m_nSurfaces-1) * fPercent)];
	}

	// GetPos() twice: each inlined call copies m_pos into a frame temporary,
	// which is where the shipped code's dead stores and second read of
	// m_pos.x come from.
	m_pClientDE->DrawSurfaceToSurfaceTransparent(hDestSurf, hSurface, LTNULL, GetPos().x, GetPos().y, m_hTransColor);
}

// Handle the enter key
LTBOOL CLTGUIFadeItemCtrl::OnEnter()
{
	if (m_pCommandHandler)
	{
		if (m_pCommandHandler->SendCommand(m_dwCommandID, m_dwParam1, m_dwParam2))
			return LTTRUE;
	}

	return LTFALSE;
}

// The selection for this control has changed
void CLTGUIFadeItemCtrl::OnSelChange()
{
	if (!m_bSelected)
	{
		m_fLastSelTime = m_pClientDE->GetTime();
	}
}
