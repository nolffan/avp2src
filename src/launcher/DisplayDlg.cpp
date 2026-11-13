/*******************************************************************************
;
;	MODULE:			DISPLAYDLG (.CPP)
;
;	PURPOSE:		Display settings dialog class
;
;	HISTORY:		11/15/2000  [kml]  This file was created
;
;	COMMENT:		Copyright (c) 2000, Monolith Inc.
;
********************************************************************************/

#include "stdafx.h"
#include "Launcher.h"
#include "DisplayDlg.h"
#include "DisplayMgr.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////
// Button and image coordinates
/////////////////////////////////////////////

#define DD_CLOSE_X					579
#define DD_CLOSE_Y					1
#define DD_OK_X						194
#define DD_OK_Y						349
#define DD_CANCEL_X					306
#define DD_CANCEL_Y					349

#define DD_RESOLUTION_LIST_X		16
#define DD_RESOLUTION_LIST_Y		73
#define DD_RESOLUTION_LIST_WIDTH	133
#define DD_RESOLUTION_LIST_HEIGHT	248
#define DD_RENDERER_LIST_X			181
#define DD_RENDERER_LIST_Y			73
#define DD_RENDERER_LIST_WIDTH		405
#define DD_RENDERER_LIST_HEIGHT		106
#define DD_DISPLAY_LIST_X			181
#define DD_DISPLAY_LIST_Y			205
#define DD_DISPLAY_LIST_WIDTH		405
#define DD_DISPLAY_LIST_HEIGHT		106

// AVP2.exe 0x42de30
CDisplayModes* g_pDisplayModes = NULL;

/////////////////////////////////////////////////////////////////////////////
// CDisplayDlg dialog


// AVP2.exe 0x402530
CDisplayDlg::CDisplayDlg(CWnd* pParent /*=NULL*/)
	: CMoveDialog(CDisplayDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CDisplayDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


// AVP2.exe 0x402600
void CDisplayDlg::DoDataExchange(CDataExchange* pDX)
{
	CMoveDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDisplayDlg)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


// RETAIL BUG: the dialog's close button (IDC_DD_CLOSE) has no handler here,
// so clicking it does nothing (AVP2.exe's map, 0x422a00, has none; NOLF2's
// launcher added OnClose).
BEGIN_MESSAGE_MAP(CDisplayDlg, CMoveDialog)
	//{{AFX_MSG_MAP(CDisplayDlg)
	ON_WM_CTLCOLOR()
	ON_WM_PAINT()
	ON_LBN_SELCHANGE(IDC_DISPLAY_LIST, OnSelchangeDisplayList)
	ON_LBN_SELCHANGE(IDC_RENDERER_LIST, OnSelchangeRendererList)
	ON_LBN_SELCHANGE(IDC_RESOLUTION_LIST, OnSelchangeResolutionList)
	ON_BN_CLICKED(IDC_DD_OK, OnOK)
	ON_BN_CLICKED(IDC_DD_CANCEL, OnCancel)
	//}}AFX_MSG_MAP
#ifdef AVP2SRC_MODERN
	// RETAIL BUG fixed (MODERN.md M-B104): the close button cancels, as
	// the launcher's own close button does
	ON_BN_CLICKED(IDC_DD_CLOSE, OnCancel)
#endif
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDisplayDlg message handlers

// AVP2.exe 0x402620
BOOL CDisplayDlg::OnInitDialog() 
{
	SetBackGround(IDB_DD_BACKGROUND);

	if (!g_pDisplayModes)
	{
		g_pDisplayModes = new CDisplayModes();

		if (!g_pDisplayModes->Init("", "autoexec.cfg"))
		{
			OnDestroy();
			return TRUE;
		}
	}

#ifndef PRIMAL_HUNT
	if (g_pDisplayModes->m_renderers.GetCount() <= 0)
	{
		theApp.MessageBox( IDS_NORENS, IDS_APPNAME, MB_OK | MB_ICONEXCLAMATION );
		return(0);
	}
#endif

	g_pDisplayModes->FillLists(GetDlgItem(IDC_RENDERER_LIST)->GetSafeHwnd(), GetDlgItem(IDC_DISPLAY_LIST)->GetSafeHwnd(), GetDlgItem(IDC_RESOLUTION_LIST)->GetSafeHwnd());
	g_pDisplayModes->UpdateLists(GetDlgItem(IDC_RENDERER_LIST)->GetSafeHwnd(), GetDlgItem(IDC_DISPLAY_LIST)->GetSafeHwnd(), GetDlgItem(IDC_RESOLUTION_LIST)->GetSafeHwnd());

	CMoveDialog::OnInitDialog();

	AddButton(IDC_DD_CLOSE,DD_CLOSE_X,DD_CLOSE_Y);
	AddButton(IDC_DD_OK,DD_OK_X,DD_OK_Y);
	AddButton(IDC_DD_CANCEL,DD_CANCEL_X,DD_CANCEL_Y);

	CWnd* pWnd = GetDlgItem(IDC_RENDERER_LIST);
	if(pWnd)
	{
		pWnd->SetWindowPos(NULL,DD_RENDERER_LIST_X,DD_RENDERER_LIST_Y,DD_RENDERER_LIST_WIDTH,DD_RENDERER_LIST_HEIGHT,SWP_NOZORDER);
	}

	pWnd = GetDlgItem(IDC_DISPLAY_LIST);
	if(pWnd)
	{
		pWnd->SetWindowPos(NULL,DD_DISPLAY_LIST_X,DD_DISPLAY_LIST_Y,DD_DISPLAY_LIST_WIDTH,DD_DISPLAY_LIST_HEIGHT,SWP_NOZORDER);
	}

	pWnd = GetDlgItem(IDC_RESOLUTION_LIST);
	if(pWnd)
	{
		pWnd->SetWindowPos(NULL,DD_RESOLUTION_LIST_X,DD_RESOLUTION_LIST_Y,DD_RESOLUTION_LIST_WIDTH,DD_RESOLUTION_LIST_HEIGHT,SWP_NOZORDER);
	}

#ifdef PRIMAL_HUNT
	// PH: checked last (PrimalHunt.exe 0x40298a), the lists filled first
	if (g_pDisplayModes->m_renderers.GetCount() <= 0)
	{
		theApp.MessageBox( IDS_NORENS, IDS_APPNAME, MB_OK | MB_ICONEXCLAMATION );
		return(0);
	}
#endif

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

// AVP2.exe 0x402870
HBRUSH CDisplayDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor) 
{
	switch(nCtlColor)
	{
		case CTLCOLOR_LISTBOX:
		case CTLCOLOR_MSGBOX:
		{
			// Set color to green on black and return the background brush.
			pDC->SetTextColor(RGB(0, 255, 0));
			pDC->SetBkColor(RGB(0, 0, 0));
			return (HBRUSH)(theApp.m_BkBrush.GetSafeHandle());
		}

		default:
		return CMoveDialog::OnCtlColor(pDC, pWnd, nCtlColor);
	}
}

// AVP2.exe 0x4028b0
void CDisplayDlg::OnPaint() 
{
	CPaintDC dc(this); // device context for painting
	
	PaintBackGround(&dc);
	
	// Do not call CMoveDialog::OnPaint() for painting messages
}

// AVP2.exe 0x402910
void CDisplayDlg::OnOK() 
{
	if (g_pDisplayModes)
	{
		g_pDisplayModes->UpdateLists(GetDlgItem(IDC_RENDERER_LIST)->GetSafeHwnd(), GetDlgItem(IDC_DISPLAY_LIST)->GetSafeHwnd(), GetDlgItem(IDC_RESOLUTION_LIST)->GetSafeHwnd());
	}
	
	CMoveDialog::OnOK();
}

// AVP2.exe 0x402980
void CDisplayDlg::OnCancel() 
{
	if (g_pDisplayModes)
		g_pDisplayModes->m_pCurRenderer = NULL;
	
	CMoveDialog::OnCancel();	
}

// AVP2.exe 0x4029a0 (the three are one function after folding)
void CDisplayDlg::OnSelchangeDisplayList() 
{
	g_pDisplayModes->UpdateLists(GetDlgItem(IDC_RENDERER_LIST)->GetSafeHwnd(), GetDlgItem(IDC_DISPLAY_LIST)->GetSafeHwnd(), GetDlgItem(IDC_RESOLUTION_LIST)->GetSafeHwnd());
}

void CDisplayDlg::OnSelchangeRendererList() 
{
	g_pDisplayModes->UpdateLists(GetDlgItem(IDC_RENDERER_LIST)->GetSafeHwnd(), GetDlgItem(IDC_DISPLAY_LIST)->GetSafeHwnd(), GetDlgItem(IDC_RESOLUTION_LIST)->GetSafeHwnd());
}

void CDisplayDlg::OnSelchangeResolutionList() 
{
	g_pDisplayModes->UpdateLists(GetDlgItem(IDC_RENDERER_LIST)->GetSafeHwnd(), GetDlgItem(IDC_DISPLAY_LIST)->GetSafeHwnd(), GetDlgItem(IDC_RESOLUTION_LIST)->GetSafeHwnd());
}
