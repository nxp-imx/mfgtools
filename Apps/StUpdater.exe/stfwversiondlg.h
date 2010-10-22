/*
 * Copyright (C) 2010, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */
#if !defined(AFX_STFWVERSIONDLG_H__274253B6_3C9B_4589_B8CC_D268DE6B91CC__INCLUDED_)
#define AFX_STFWVERSIONDLG_H__274253B6_3C9B_4589_B8CC_D268DE6B91CC__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// StFwVersionDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CStFwVersionDlg dialog

class CStFwVersionDlg : public CDialog
{
// Construction
public:
	CStFwVersionDlg(CStVersionInfoPtrArray* p_arr_current_component_vers,
		CStVersionInfoPtrArray* p_arr_upgrade_component_vers,
		CStConfigInfo* p_config_info,
		CWnd* pParent = NULL);   // standard constructor

	void SetupDisplay();
// Dialog Data
	//{{AFX_DATA(CStFwVersionDlg)
	enum { IDD = IDD_STFWVERSIONDLG };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CStFwVersionDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

	CListCtrl	m_CurrentVersionListCtrl;
	CListCtrl	m_UpgradeVersionListCtrl;

// Implementation
protected:

	CStVersionInfoPtrArray*		m_p_arr_current_component_vers;
	CStVersionInfoPtrArray*		m_p_arr_upgrade_component_vers;
	CStConfigInfo*				m_p_config_info;

	void SetupDriveDetails(UCHAR drive_num);

	// Generated message map functions
	//{{AFX_MSG(CStFwVersionDlg)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STFWVERSIONDLG_H__274253B6_3C9B_4589_B8CC_D268DE6B91CC__INCLUDED_)
