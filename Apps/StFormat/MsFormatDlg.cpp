/*
 * Copyright (C) 2010, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */
// MsFormatDlg.cpp : implementation file
//

#include "stdafx.h"
#include "StFormat.h"
#include "MsFormatDlg.h"


// MsFormatDlg dialog

IMPLEMENT_DYNAMIC(MsFormatDlg, CDialog)
MsFormatDlg::MsFormatDlg(CWnd* pParent /*=NULL*/)
	: CDialog(MsFormatDlg::IDD, pParent)
{
}

MsFormatDlg::~MsFormatDlg()
{
}

void MsFormatDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(MsFormatDlg, CDialog)
END_MESSAGE_MAP()


// MsFormatDlg message handlers
