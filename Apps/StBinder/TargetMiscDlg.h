/*
 * Copyright (C) 2010, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */
#pragma once
#include "afxwin.h"
#include "TargetCfgData.h"


// CTargetMisc dialog

class CTargetMiscDlg : public CPropertyPage
{
	DECLARE_DYNAMIC(CTargetMiscDlg)

public:
	CTargetMiscDlg(/*CWnd* pParent =NULL*/);   // standard constructor
	virtual ~CTargetMiscDlg();

	virtual	BOOL OnInitDialog();
	virtual void OnOK();
	virtual void OnCancel();

// Dialog Data
	enum { IDD = IDD_TARGET_MISC };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	CButton m_ShowBootToPlayerMsg;
	CButton m_LowNandBtn;
	CButton m_MinDlgFmtMsg;
};
