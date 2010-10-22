/*
 * Copyright (C) 2010, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */
#pragma once
#include "afxwin.h"
#include "Libs/DevSupport/StApi.h"
#include "OverwriteEdit.h"

// CCmdDataDlg dialog

class CCmdDataDlg : public CDialog
{
	DECLARE_DYNAMIC(CCmdDataDlg)

public:
	CCmdDataDlg(StApi* pApi, CWnd* pParent = NULL);   // standard constructor
	virtual ~CCmdDataDlg();

// Dialog Data
	enum { IDD = IDD_DATA_DIALOG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	CEdit _cmdDataCtrl;
private:
	StApi *_pApi;
protected:
	virtual void OnOK();
public:
	virtual BOOL OnInitDialog();
};
