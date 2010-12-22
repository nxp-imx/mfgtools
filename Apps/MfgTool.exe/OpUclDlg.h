/*
 * Copyright (C) 2010, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */
#pragma once
#include "OpInfo.h"
#include "UpdateCommandList.h"

// COpUclDlg dialog

class COpUclDlg : public CDialog
{
	DECLARE_DYNAMIC(COpUclDlg)

public:
	COpUclDlg(CWnd* pParent = NULL, COpInfo* pInfo = NULL);   // standard constructor
	virtual ~COpUclDlg();

// Dialog Data
	enum { IDD = IDD_UTPUPDATEOPDLG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    virtual BOOL OnInitDialog();

    CComboBox     m_combo_ucl_lists;
	UCL			  m_UclNode;

    COpInfo*	m_pOpInfo;

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedCancel();

private:
	void Localize();
	void LoadCommandLists(LPCTSTR filename);
};
