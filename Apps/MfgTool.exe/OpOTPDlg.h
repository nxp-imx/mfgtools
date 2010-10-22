/*
 * Copyright (C) 2010, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */
#pragma once
#include "OpInfo.h"
#include "../../Libs/WinSupport/visvaledit.h"

// COpOTPDlg dialog

class COpOTPDlg : public CDialog
{
	DECLARE_DYNAMIC(COpOTPDlg)

public:
	COpOTPDlg(CWnd* pParent = NULL, COpInfo* pInfo = NULL);   // standard constructor
	virtual ~COpOTPDlg();

// Dialog Data
	enum { IDD = IDD_OTPOPDLG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    virtual BOOL OnInitDialog();

	CVisValidEdit m_vve_op_name_ctrl;
	CVisValidEdit m_vve_OTP_register_value;

    COpInfo*	m_pOpInfo;

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedCancel();

private:
	void Localize();
};
