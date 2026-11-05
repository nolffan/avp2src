/*******************************************************************************
;
;	MODULE:			MOVEDLG (.CPP)
;
;	PURPOSE:		Dialog class that lets the user drag the dialog around
;					(derived from CDialogEx)
;
;	HISTORY:		11/15/2000  [kml]  This file was created
;
;	COMMENT:		Copyright (c) 2000, Monolith Inc.
;
********************************************************************************/

#include "stdafx.h"
#include "MoveDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CMoveDialog dialog


// AVP2.exe 0x407510
CMoveDialog::CMoveDialog(int IDD, CWnd* pParent)
	: CDialogEx(IDD, pParent)
{
	//{{AFX_DATA_INIT(CMoveDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	isMoving = FALSE;
}


// AVP2.exe 0x407560
void CMoveDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CMoveDlg)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CMoveDialog, CDialogEx)
	//{{AFX_MSG_MAP(CMoveDlg)
	ON_WM_MOUSEMOVE()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMoveDialog message handlers

// AVP2.exe 0x407580
void CMoveDialog::OnLButtonDown(UINT nFlags, CPoint point)
{
	if (!isMoving)
	{
		// Store current mousepointer position
		GetCursorPos(&m_MovePoint);

		// Capture all mouse input
		SetCapture();

		isMoving = TRUE;
	}
}

// AVP2.exe 0x4075c0
void CMoveDialog::OnLButtonUp(UINT nFlags, CPoint point)
{
	if (isMoving)
	{
		// Store last mousepointer position
		GetCursorPos(&m_MovePoint);

		// Release mouse
		ReleaseCapture();

		isMoving = FALSE;
	}
}

// AVP2.exe 0x4075f0
void CMoveDialog::OnMouseMove(UINT nFlags, CPoint point)
{
	if (isMoving)
	{
		// Get current mousepointer position
		CPoint npoint;
		GetCursorPos(&npoint);

		if ((m_MovePoint.x != npoint.x) || (m_MovePoint.y != npoint.y))
		{
			// Move the dialog according to the new pos of the mousepointer
			CRect wrect;
			GetWindowRect(&wrect);
			int divx = npoint.x - m_MovePoint.x;
			int divy = npoint.y - m_MovePoint.y;
			SetWindowPos(NULL, wrect.left+divx, wrect.top+divy, 0, 0, SWP_NOZORDER|SWP_NOSIZE);

			// Store new mousepointer position
			m_MovePoint = npoint;
		}
	}

	CDialogEx::OnMouseMove(nFlags, point);
}
