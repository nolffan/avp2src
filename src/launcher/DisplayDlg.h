/*******************************************************************************
;
;	MODULE:			DISPLAYDLG (.H)
;
;	PURPOSE:		Display settings dialog class
;
;	HISTORY:		11/15/2000  [kml]  This file was created
;
;	COMMENT:		Copyright (c) 2000, Monolith Inc.
;
********************************************************************************/

#if !defined(AFX_DISPLAYDLG_H__2C0BDE3D_2C9B_4C9E_9C2C_9C5D2ECC6F0B__INCLUDED_)
#define AFX_DISPLAYDLG_H__2C0BDE3D_2C9B_4C9E_9C2C_9C5D2ECC6F0B__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "MoveDlg.h"

/////////////////////////////////////////////////////////////////////////////
// CDisplayDlg dialog (AVP2.exe: vtable 0x422ac8)

class CDisplayDlg : public CMoveDialog
{
// Construction
public:
	CDisplayDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CDisplayDlg)
	enum { IDD = IDD_DISPLAY_DIALOG };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDisplayDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CDisplayDlg)
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg void OnOK();
	afx_msg void OnCancel();
	afx_msg void OnSelchangeDisplayList();
	afx_msg void OnSelchangeRendererList();
	afx_msg void OnSelchangeResolutionList();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DISPLAYDLG_H__2C0BDE3D_2C9B_4C9E_9C2C_9C5D2ECC6F0B__INCLUDED_)
