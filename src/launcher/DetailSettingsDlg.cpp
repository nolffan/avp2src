/*******************************************************************************
;
;	MODULE:			DETAILSETTINGSDLG (.CPP)
;
;	PURPOSE:		Detail settings dialog class
;
;	HISTORY:		11/15/2000  [kml]  This file was created
;
;	COMMENT:		Copyright (c) 2000, Monolith Inc.
;
********************************************************************************/

#include "stdafx.h"
#include "Launcher.h"
#include "DetailSettingsDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////
// Button and image coordinates
/////////////////////////////////////////////

#define DSD_CLOSE_X				435
#define DSD_CLOSE_Y				1

#define DSD_LOWDETAIL_X			123
#define DSD_LOWDETAIL_Y			179
#define DSD_MEDIUMDETAIL_X		123
#define DSD_MEDIUMDETAIL_Y		288
#define DSD_HIGHDETAIL_X		123
#define DSD_HIGHDETAIL_Y		397
#define DSD_CANCEL_X			178
#define DSD_CANCEL_Y			440

#ifdef PRIMAL_HUNT
// PH: its art's text boxes sit 8 pixels lower, in a smaller font
#define DSD_HEADERTEXT_Y		46
#define DSD_LOWTEXT_Y			130
#define DSD_MEDIUMTEXT_Y		239
#define DSD_HIGHTEXT_Y			349
#define DSD_FONTSIZE			12
#else
#define DSD_HEADERTEXT_Y		38
#define DSD_LOWTEXT_Y			122
#define DSD_MEDIUMTEXT_Y		231
#define DSD_HIGHTEXT_Y			341
#define DSD_FONTSIZE			14
#endif
#define DSD_HEADERTEXT_HEIGHT	58
#define DSD_LOWTEXT_HEIGHT		49
#define DSD_MEDIUMTEXT_HEIGHT	49
#define DSD_HIGHTEXT_HEIGHT		49


/////////////////////////////////////////////////////////////////////////////
// CDetailSettingsDlg dialog


// AVP2.exe 0x401ed0
CDetailSettingsDlg::CDetailSettingsDlg(CWnd* pParent /*=NULL*/)
	: CMoveDialog(CDetailSettingsDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CDetailSettingsDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


// AVP2.exe 0x4020e0
void CDetailSettingsDlg::DoDataExchange(CDataExchange* pDX)
{
	CMoveDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDetailSettingsDlg)
	DDX_Control(pDX, IDC_MEDIUMTEXT, m_MediumText);
	DDX_Control(pDX, IDC_LOWTEXT, m_LowText);
	DDX_Control(pDX, IDC_HIGHTEXT, m_HighText);
	DDX_Control(pDX, IDC_HEADERTEXT, m_HeaderText);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDetailSettingsDlg, CMoveDialog)
	//{{AFX_MSG_MAP(CDetailSettingsDlg)
	ON_BN_CLICKED(IDC_DSD_CLOSE, OnClose)
	ON_BN_CLICKED(IDC_DSD_HIGH, OnHighDetail)
	ON_BN_CLICKED(IDC_DSD_LOW, OnLowDetail)
	ON_BN_CLICKED(IDC_DSD_MEDIUM, OnMediumDetail)
	ON_WM_PAINT()
	ON_BN_CLICKED(IDC_DSD_CANCEL, OnCancel)
	ON_WM_CTLCOLOR()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDetailSettingsDlg message handlers

// AVP2.exe 0x407ae0 (folded with OnClose)
void CDetailSettingsDlg::OnCancel() 
{
	CMoveDialog::OnCancel();
}

// AVP2.exe 0x407ae0
void CDetailSettingsDlg::OnClose() 
{
	CMoveDialog::OnCancel();
}

// AVP2.exe 0x402150
void CDetailSettingsDlg::OnHighDetail() 
{
	theApp.m_dwDetailSetting = DETAIL_HIGH;
	OnOK();
}

// AVP2.exe 0x402170
void CDetailSettingsDlg::OnLowDetail() 
{
	theApp.m_dwDetailSetting = DETAIL_LOW;
	OnOK();
}

// AVP2.exe 0x402190
void CDetailSettingsDlg::OnMediumDetail() 
{
	theApp.m_dwDetailSetting = DETAIL_MEDIUM;
	OnOK();
}

// AVP2.exe 0x4021b0
BOOL CDetailSettingsDlg::OnInitDialog() 
{
	SetBackGround(IDB_DSD_BACKGROUND);

	CMoveDialog::OnInitDialog();

	// Here's where we define what buttons go in our dialog
	AddButton(IDC_DSD_CLOSE,DSD_CLOSE_X,DSD_CLOSE_Y);
	AddButton(IDC_DSD_HIGH,DSD_HIGHDETAIL_X,DSD_HIGHDETAIL_Y);
	AddButton(IDC_DSD_MEDIUM,DSD_MEDIUMDETAIL_X,DSD_MEDIUMDETAIL_Y);
	AddButton(IDC_DSD_LOW,DSD_LOWDETAIL_X,DSD_LOWDETAIL_Y);
	AddButton(IDC_DSD_CANCEL,DSD_CANCEL_X,DSD_CANCEL_Y);

	CRect rcDlg;
	GetWindowRect(&rcDlg);

	CString csText;
	csText.LoadString(IDS_DETAIL_HEADER);
	m_HeaderText.SetFontName("Arial Narrow");
	m_HeaderText.SetFontSize(DSD_FONTSIZE);
	m_HeaderText.SetTextColor(RGB(255,255,255));
	m_HeaderText.SetWindowText(csText);
	m_HeaderText.SetWindowPos(NULL,15,DSD_HEADERTEXT_Y,rcDlg.Width()-30,DSD_HEADERTEXT_HEIGHT,SWP_NOZORDER);
	csText.LoadString(IDS_DETAIL_LOW);
	m_LowText.SetFontName("Arial Narrow");
	m_LowText.SetFontSize(DSD_FONTSIZE);
	m_LowText.SetTextColor(RGB(255,255,255));
	m_LowText.SetWindowText(csText);
	m_LowText.SetWindowPos(NULL,15,DSD_LOWTEXT_Y,rcDlg.Width()-30,DSD_LOWTEXT_HEIGHT,SWP_NOZORDER);
	csText.LoadString(IDS_DETAIL_MEDIUM);
	m_MediumText.SetFontName("Arial Narrow");
	m_MediumText.SetFontSize(DSD_FONTSIZE);
	m_MediumText.SetTextColor(RGB(255,255,255));
	m_MediumText.SetWindowText(csText);
	m_MediumText.SetWindowPos(NULL,15,DSD_MEDIUMTEXT_Y,rcDlg.Width()-30,DSD_MEDIUMTEXT_HEIGHT,SWP_NOZORDER);
	csText.LoadString(IDS_DETAIL_HIGH);
	m_HighText.SetFontName("Arial Narrow");
	m_HighText.SetFontSize(DSD_FONTSIZE);
	m_HighText.SetTextColor(RGB(255,255,255));
	m_HighText.SetWindowText(csText);
	m_HighText.SetWindowPos(NULL,15,DSD_HIGHTEXT_Y,rcDlg.Width()-30,DSD_HIGHTEXT_HEIGHT,SWP_NOZORDER);
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

// AVP2.exe 0x402490
void CDetailSettingsDlg::OnPaint() 
{
	CPaintDC dc(this); // device context for painting

	PaintBackGround(&dc);
	// Do not call CDialog::OnPaint() for painting messages
}

// AVP2.exe 0x4024f0
HBRUSH CDetailSettingsDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor) 
{
	switch(nCtlColor)
	{
		case CTLCOLOR_DLG:
		case CTLCOLOR_BTN:
		{
			return (HBRUSH)GetStockObject(NULL_BRUSH);
		}
		default:
		return CMoveDialog::OnCtlColor(pDC, pWnd, nCtlColor);
	}
}
