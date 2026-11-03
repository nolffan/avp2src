// LTGUIOnOffCtrl.cpp: implementation of the CLTGUIOnOffCtrl class.
//
//////////////////////////////////////////////////////////////////////

#include "ltguimgr.h"
#include "ltguionoffctrl.h"
#include "ltguifont.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CLTGUIOnOffCtrl::CLTGUIOnOffCtrl()
{
	m_bOn		= LTFALSE;
	m_hStrOn	= LTNULL;
	m_hStrOff	= LTNULL;
}

CLTGUIOnOffCtrl::~CLTGUIOnOffCtrl()
{
	if (m_pClientDE)
	{
		if (m_hStrOn)
			m_pClientDE->FreeString(m_hStrOn);
		if (m_hStrOff)
			m_pClientDE->FreeString(m_hStrOff);
	}

	Destroy();
}

// Create the control
LTBOOL CLTGUIOnOffCtrl::Create(ILTClient *pClientDE, HSTRING hString, CLTGUIFont *pFont,
							   int nRightColumnOffset, LTBOOL *pbValue, int alignment)
{
	if (!CLTGUICycleCtrl::Create(pClientDE, hString, pFont, nRightColumnOffset-25, 25, LTNULL, alignment))
		return LTFALSE;

	HSTRING hStrOn = pClientDE->CreateString("ON");
	HSTRING hStrOff = pClientDE->CreateString("OFF");

	AddString(hStrOn);
	AddString(hStrOff);

	m_pClientDE->FreeString(hStrOn);
	m_pClientDE->FreeString(hStrOff);

	m_pbValue = pbValue;

	SetOn(LTFALSE);

	return LTTRUE;
}

// Set the on string
void CLTGUIOnOffCtrl::SetOnString(HSTRING hStrOn)
{
	if (!hStrOn)
		return;

	if (!m_pClientDE)
		return;

	RemoveString(0);
	m_stringArray.Insert(0, m_pClientDE->CopyString(hStrOn));
	m_hStrOn = m_pClientDE->CopyString(hStrOn);
}

// Set the off string
void CLTGUIOnOffCtrl::SetOffString(HSTRING hStrOff)
{
	if (!hStrOff)
		return;

	if (!m_pClientDE)
		return;

	RemoveString(1);
	m_stringArray.Insert(1, m_pClientDE->CopyString(hStrOff));
	m_hStrOff = m_pClientDE->CopyString(hStrOff);
}

// Set the on string from a string ID
void CLTGUIOnOffCtrl::SetOnString(int nStrOnID)
{
	if (!m_pClientDE)
		return;

	HSTRING hString = m_pClientDE->FormatString(nStrOnID);
	SetOnString(hString);
	m_pClientDE->FreeString(hString);
}

// Set the off string from a string ID
void CLTGUIOnOffCtrl::SetOffString(int nStrOffID)
{
	if (!m_pClientDE)
		return;

	HSTRING hString = m_pClientDE->FormatString(nStrOffID);
	SetOffString(hString);
	m_pClientDE->FreeString(hString);
}

// Update data
void CLTGUIOnOffCtrl::UpdateData(LTBOOL bSaveAndValidate)
{
	if (!m_pbValue)
		return;

	if (bSaveAndValidate)
		*m_pbValue = m_bOn;
	else
		SetOn(*m_pbValue);
}

// Sets the on/off status
void CLTGUIOnOffCtrl::SetOn(LTBOOL bOn)
{
	m_bOn = bOn;

	if (bOn)
		SetSelIndex(0);
	else
		SetSelIndex(1);
}
