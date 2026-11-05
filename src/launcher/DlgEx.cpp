/*******************************************************************************
;
;	MODULE:			DLGEX (.CPP)
;
;	PURPOSE:		Extended dialog class (derived from CDialog) that has
;					a bitmap as a background
;
;	HISTORY:		11/15/2000  [kml]  This file was created
;
;	COMMENT:		Copyright (c) 2000, Monolith Inc.
;
********************************************************************************/

#include "stdafx.h"
#include "DlgEx.h"
#include "BitmapCheckButton.h"
#include "ButtonEx.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDialogEx dialog


// AVP2.exe 0x4043e0
CDialogEx::CDialogEx(int IDD, CWnd* pParent)
	: CDialog(IDD, pParent)
{
	//{{AFX_DATA_INIT(CDlgEx)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT

	m_pFocusButton = NULL;
}


// AVP2.exe 0x404430
void CDialogEx::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDlgEx)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDialogEx, CDialog)
	//{{AFX_MSG_MAP(CDialogEx)
	ON_WM_MOUSEMOVE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDialogEx message handlers

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CDialogEx::OnInitDialog (AVP2.exe 0x404450)
//
//	PURPOSE:	Dialog initialization
//
// ----------------------------------------------------------------------- //
BOOL CDialogEx::OnInitDialog() 
{
	CDialog::OnInitDialog();

	// IMPORTANT: SetBackGround() should have been called before we get here!!
	// Call SetBackGround() to set the background bitmap before calling CDialogEx::OnInitDialog()
	// from your implmentation of OnInitDialog.

	// Get background bitmap size
	BITMAP bmpInfo;
	m_Background.GetBitmap(&bmpInfo);

	// Resize the dialog to fit the bitmap and then center it
	SetWindowPos(NULL, 0, 0, bmpInfo.bmWidth, bmpInfo.bmHeight, SWP_NOZORDER|SWP_NOMOVE);
	CenterWindow();

	return TRUE;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CDialogEx::SetBackGround (AVP2.exe 0x4044a0)
//
//	PURPOSE:	Sets the bitmap background image for the dialog
//
// ----------------------------------------------------------------------- //
BOOL CDialogEx::SetBackGround(int nID)
{
	// We could also use CBitmap::LoadBitmap but that has code that screws with the palette
	LPCTSTR lpszResourceName = (LPCTSTR)nID;
	HBITMAP hBmp = (HBITMAP)::LoadImage(AfxGetInstanceHandle(), lpszResourceName, IMAGE_BITMAP, 0,0, LR_CREATEDIBSECTION);

	if (hBmp == NULL) return FALSE;

	m_Background.Attach(hBmp);

	return TRUE;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CDialogEx::PaintBackGround (AVP2.exe 0x4044e0)
//
//	PURPOSE:	Draws the background image
//
//  IMPORTANT:	This function should be called from your OnPaint()
//				implementation BEFORE anything is painted on the dialog, 
//				to make sure the background gets drawn first.
//
// ----------------------------------------------------------------------- //
void CDialogEx::PaintBackGround(CPaintDC * dc)
{
	BITMAP bmpInfo;
	m_Background.GetBitmap(&bmpInfo);

	CDC dcTmp;
	dcTmp.CreateCompatibleDC(dc);
	dcTmp.SelectObject(&m_Background);
	
	// Draw bitmap regardless of dialog size.
	dc->BitBlt(0, 0, bmpInfo.bmWidth, bmpInfo.bmHeight, &dcTmp, 0,0, SRCCOPY);
	dcTmp.DeleteDC();
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CDialogEx::FreeAllButtons (AVP2.exe 0x4045b0)
//
//	PURPOSE:	Frees all the buttons
//
// ----------------------------------------------------------------------- //
void CDialogEx::FreeAllButtons()
{
	CButton* pButton;

	for(int i=0;i<m_collButtons.GetSize();i++)
	{
		pButton = m_collButtons[i];
		if(pButton)
			delete pButton;
	}
	m_collButtons.RemoveAll();
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CDialogEx::AddButton (AVP2.exe 0x404600)
//
//	PURPOSE:	Adds a bitmap button to the dialog
//
// ----------------------------------------------------------------------- //
CButton* CDialogEx::AddButton(UINT nID, int x, int y, CStaticEx* pHelpText, DWORD dwHelpTextID)
{
	// Create the button
	CButtonEx* pButton = new CButtonEx;

	// Load the bitmaps
	if(!pButton->AutoLoad(nID,this))
	{
		delete pButton;
		return NULL;
	}

	// Set the help text if applicable
	if(pHelpText)
	{
		CString csHelpText;
		if(dwHelpTextID)
			csHelpText.LoadString(dwHelpTextID);
		pButton->SetHelpTextCtrl(pHelpText,csHelpText);
	}

	// Set the position
	pButton->SetWindowPos(NULL, x, y, 0, 0, SWP_NOZORDER|SWP_NOSIZE);

	// Add us to our array
	m_collButtons.Add(pButton);

	// All done!
	return pButton;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CDialogEx::AddCheckBox (AVP2.exe 0x404810)
//
//	PURPOSE:	Adds a bitmap check box to the dialog
//
// ----------------------------------------------------------------------- //
CButton* CDialogEx::AddCheckBox(UINT nID, int x, int y, CStaticEx* pHelpText, DWORD dwHelpTextID)
{
	// Create the button
	CBitmapCheckButton* pButton = new CBitmapCheckButton;

	// Load the bitmaps
	if(!pButton->AutoLoad(nID,this))
	{
		delete pButton;
		return NULL;
	}

	// Set the help text if applicable
	if(pHelpText)
	{
		CString csHelpText;
		if(dwHelpTextID)
			csHelpText.LoadString(dwHelpTextID);
		pButton->SetHelpTextCtrl(pHelpText,csHelpText);
	}

	// Set the position
	pButton->SetWindowPos(NULL, x, y, 0, 0, SWP_NOZORDER|SWP_NOSIZE);

	// Add us to our array
	m_collButtons.Add(pButton);

	// All done!
	return pButton;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CDialogEx::OnMouseMove (AVP2.exe 0x404a20)
//
//	PURPOSE:	Mouse movement handler
//
// ----------------------------------------------------------------------- //
void CDialogEx::OnMouseMove(UINT nFlags, CPoint point) 
{
	SetFocusButton(NULL);
	CDialog::OnMouseMove(nFlags, point);
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CDialogEx::SetFocusButton (AVP2.exe 0x404a40)
//
//	PURPOSE:	Sets the focus button
//
// ----------------------------------------------------------------------- //
void CDialogEx::SetFocusButton(CButton* pButton)
{
	// Force a draw on the button that's losing focus
	if(m_pFocusButton)
	{
		m_pFocusButton->InvalidateRect(NULL);
	}

	m_pFocusButton = pButton;

	// Force a draw on the button that's gaining focus
	if(m_pFocusButton)
	{
		m_pFocusButton->InvalidateRect(NULL);
	}
}
