/*******************************************************************************
;
;	MODULE:			MOVEDLG (.H)
;
;	PURPOSE:		Dialog class that lets the user drag the dialog around
;					(derived from CDialogEx)
;
;	HISTORY:		11/15/2000  [kml]  This file was created
;
;	COMMENT:		Copyright (c) 2000, Monolith Inc.
;
********************************************************************************/

#if !defined(AFX_MOVEDLG_H__4A0D1C01_B4C0_11D4_A2BA_00010229388A__INCLUDED_)
#define AFX_MOVEDLG_H__4A0D1C01_B4C0_11D4_A2BA_00010229388A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "DlgEx.h"

/////////////////////////////////////////////////////////////////////////////
// CMoveDialog dialog (AVP2.exe: vtable 0x423160, 0x88 bytes)

class CMoveDialog : public CDialogEx
{
// Construction
public:
	CMoveDialog(int IDD, CWnd* pParent = NULL);   // standard constructor

	BOOL isMoving;				// 0x7c
	CPoint m_MovePoint;			// 0x80

// Dialog Data
	//{{AFX_DATA(CMoveDialog)
	enum { IDD = 0 };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMoveDialog)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CMoveDialog)
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MOVEDLG_H__4A0D1C01_B4C0_11D4_A2BA_00010229388A__INCLUDED_)
