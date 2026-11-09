// FolderHostOptions.h: interface for the CFolderHostOptions class.
//
//////////////////////////////////////////////////////////////////////

#ifndef _FOLDER_HOST_OPTS_H_
#define _FOLDER_HOST_OPTS_H_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "BaseFolder.h"
#include "ServerOptionMgr.h"

#ifdef PRIMAL_HUNT
// PH: a check box (a bitmap button and its label) for the class and weapon
// restriction pages
class CHostCheckBox
{
public:

	CHostCheckBox()
	{
		m_bChecked	= LTTRUE;
		m_pBitmap	= new CBitmapCtrl;		// RETAIL BUG: never deleted
		m_pLabel	= LTNULL;
	}
	~CHostCheckBox() {}

	void	Create(CBaseFolder *pFolder, uint32 dwCommand, int x, int y, int nStringID, LTBOOL bChecked);
	void	Create(CBaseFolder *pFolder, uint32 dwCommand, int x, int y, char *szText, LTBOOL bChecked);

	LTBOOL	SetCheck(LTBOOL bChecked, uint32 dwCommand);
	void	Destroy();

	LTBOOL				m_bChecked;
	CBitmapCtrl			*m_pBitmap;
	CLTGUITextItemCtrl	*m_pLabel;
	CBaseFolder			*m_pFolder;
};

#define MAX_HOST_CLASS_BOXES	24
#define MAX_HOST_WEAPON_BOXES	64
#endif

class CFolderHostOptions : public CBaseFolder
{
public:

	CFolderHostOptions();
	virtual ~CFolderHostOptions();

	// Build the folder
    LTBOOL  Build();

	void	Escape();
	void	OnFocus(LTBOOL bFocus);
	void	FocusSetup(LTBOOL bFocus);

	LTBOOL	OnEnter();

	LTBOOL	OnRButtonUp(int x, int y);
	LTBOOL	OnLButtonUp(int x, int y);

#ifdef PRIMAL_HUNT
	LTBOOL	Render(HSURFACE hDestSurf);
#endif

protected:

    uint32  OnCommand(uint32 dwCommand, uint32 dwParam1, uint32 dwParam2);
	void	BuildControlList();

	void	UpdateBandwidth();
	LTBOOL	IsValidBandwidth(uint32 nBandwidth);

#ifdef PRIMAL_HUNT
	void	AddClassBox(uint32 dwCommand, int x, int y, int nStringID, LTBOOL bChecked);
	void	AddWeaponBox(uint32 dwCommand, int x, int y, char *szText, LTBOOL bChecked);
	void	RemoveBoxes();
#endif

protected:

	CLTGUITextItemCtrl *m_pPreferences;
	CLTGUITextItemCtrl *m_pScoring;
#ifdef PRIMAL_HUNT
	CLTGUITextItemCtrl *m_pClasses;
	CLTGUITextItemCtrl *m_pWeapons;

	CHostCheckBox		m_aClassBoxes[MAX_HOST_CLASS_BOXES];
	CHostCheckBox		m_aWeaponBoxes[MAX_HOST_WEAPON_BOXES];

	CLTGUITextItemCtrl	*m_pRaceLabels[4];

	StringSet			m_Unused;
	CLTGUICtrl			*m_pUnusedCtrl;
	int					m_nUnused;
#endif

	CServerOptions		m_Options;

	CGroupCtrl			*m_pPortGroup;
	CLTGUIEditCtrl		*m_pPortEdit;
	CLTGUITextItemCtrl	*m_pPortLabel;

	char				m_szPort[8];
	char				m_szOldPort[8];

	CCycleCtrl			*m_pBandwidthCycle;
	CGroupCtrl			*m_pBandwidthGroup;
	CLTGUIEditCtrl		*m_pBandwidthEdit;
	CLTGUITextItemCtrl	*m_pBandwidthLabel;

	int					m_nBandwidth;
	char				m_szBandwidth[16];
	char				m_szOldBandwidth[16];

	int	m_nSpeed;
	int	m_nRespawn;
	int	m_nDamage;
	int	m_nExosuit;
	int	m_nQueenMolt;

	int m_nScoring[MAX_MP_SCORING_CLASSES];
};

inline LTBOOL CFolderHostOptions::IsValidBandwidth(uint32 nBandwidth) 
{
	return (nBandwidth >= 1000 && nBandwidth <= 1000000);
}

#endif // _FOLDER_HOST_OPTS_H_