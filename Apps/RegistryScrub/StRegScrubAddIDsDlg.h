/*
 * Copyright (C) 2010, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */
#pragma once


// StRegScrubAddIDsDlg dialog

class CStRegScrubAddIDsDlg : public CDialog
{
	DECLARE_DYNAMIC(CStRegScrubAddIDsDlg)

public:
	CStRegScrubAddIDsDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CStRegScrubAddIDsDlg();

	BOOL ValidateHexString(CString hexStr);
	void FixUpString(CString& str);

// Dialog Data
	enum { IDD = IDD_ADD_NEW_IDS };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    virtual BOOL OnInitDialog();


	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedOk();
};
