/*
 * Copyright (C) 2010, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */
#pragma once
#include "afxwin.h"
#include "StBinderDlg.h"


// CLoadProfileDlg dialog

class CLoadProfileDlg : public CDialog
{
	DECLARE_DYNAMIC(CLoadProfileDlg)

public:
	CLoadProfileDlg(CStBinderDlg *_binderDlg, CWnd* pParent = NULL);   // standard constructor
	virtual ~CLoadProfileDlg();

// Dialog Data
	enum { IDD = IDD_CFG_LOAD_PROFILE };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	CStBinderDlg *m_pBinderDlg;
	CListBox m_ProfileListCtl;

	virtual BOOL OnInitDialog();

	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedCancel();
};
