/*
 * Copyright (C) 2010, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */
#pragma once


// MsFormatDlg dialog

class MsFormatDlg : public CDialog
{
	DECLARE_DYNAMIC(MsFormatDlg)

public:
	MsFormatDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~MsFormatDlg();

// Dialog Data
	enum { IDD = IDD_MS_FORMAT_DIALOG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
};
