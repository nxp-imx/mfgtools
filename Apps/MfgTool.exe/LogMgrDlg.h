/*
 * Copyright (C) 2009-2011, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */
#pragma once

#include "resource.h"		// main symbols

// CLogMgrDlg dialog

class CLogMgrDlg : public CDialog
{

public:
	CLogMgrDlg();   // standard constructor
	virtual ~CLogMgrDlg();

	BOOL Create();

// Dialog Data
	enum { IDD = IDD_LOGMGR_DIALOG };

	void LogIt(CString Data);
    virtual BOOL OnInitDialog();

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	CString m_sLogDataStr;
	CWnd* m_pParent; 
	
	// Generated message map functions 
	//{{AFX_MSG(CLogMgrDlg) 
	virtual void OnCancel(); 
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
public:
};
