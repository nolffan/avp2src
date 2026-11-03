// LTGUIFadeColorCtrl.cpp: implementation of the CLTGUIFadeColorCtrl class.
//
//////////////////////////////////////////////////////////////////////

#include "ltguimgr.h"
#include "ltguifadecolorctrl.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CLTGUIFadeColorCtrl::CLTGUIFadeColorCtrl()
{
	m_dwFadeTime	= 0;
	m_selColor		= SETRGB(255,255,255);
	m_nonSelColor	= 0;
	m_disabledColor	= SETRGB(96,96,96);
	m_fStartTime	= 0.0f;
}

CLTGUIFadeColorCtrl::~CLTGUIFadeColorCtrl()
{
}

// Sets the colors to fade to and from
void CLTGUIFadeColorCtrl::SetColor(HLTCOLOR selColor, HLTCOLOR nonSelColor, HLTCOLOR disabledColor)
{
	m_selColor		= selColor;
	m_nonSelColor	= nonSelColor;
	m_disabledColor	= disabledColor;
}

// Starts the fade animation
void CLTGUIFadeColorCtrl::StartFade()
{
	if (m_pClientDE)
	{
		m_fStartTime = m_pClientDE->GetTime();
	}
}

// Stops the fade animation
void CLTGUIFadeColorCtrl::StopFade()
{
	m_fStartTime = 0.0f;
}

// Returns the current fade percentage.  0.0f is selected and 1.0f is non-selected
float CLTGUIFadeColorCtrl::GetCurrentFadePercentage()
{
	float fPercentage = 0.0f;

	if (m_dwFadeTime == 0)
	{
		if (m_bSelected)
			return 0.0f;
		return 1.0f;
	}

	if (m_bSelected)
		return 0.0f;

	if (m_pClientDE)
	{
		// NOTE: the shipped build (VC6 SP3) wrote this literal as double 0.001
		// and demoted it to a single-precision constant (truncating to
		// 0x3A83126E); our compiler revision does not perform the demotion,
		// so we use 0.001f (0x3A83126F) — the code bytes are identical, the
		// constant pool differs by one ulp.
		fPercentage = (m_pClientDE->GetTime() - m_fStartTime) / (m_dwFadeTime * 0.001f);
		if (fPercentage > 1.0f)
			return 1.0f;
	}

	return fPercentage;
}

// Returns the current color as it is fading
HLTCOLOR CLTGUIFadeColorCtrl::GetCurrentColor()
{
	if (!m_bEnabled)
		return m_disabledColor;

	float fPercentage = GetCurrentFadePercentage();
	HLTCOLOR sel = m_selColor;
	HLTCOLOR nonSel = m_nonSelColor;

	// Each channel is an accumulator (f = a; f += b;), not a flat sum: a flat
	// `a + b` is canonicalised by VC6 and always emitted sel-term first, while
	// the shipped object emits the nonSel term first.
	float fInvPercentage = 1.0f - fPercentage;
	float f;
	f  = (int)GETR(nonSel)*fPercentage;
	f += (int)GETR(sel)*fInvPercentage;
	int r = (int)f;
	f  = (int)GETG(nonSel)*fPercentage;
	f += (int)GETG(sel)*fInvPercentage;
	int g = (int)f;
	f  = (int)GETB(nonSel)*fPercentage;
	f += (int)GETB(sel)*fInvPercentage;
	int b = (int)f;

	return SETRGB(r,g,b);
}
