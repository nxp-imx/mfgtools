/*
 * Copyright (C) 2010, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */
// TargetMisc.cpp : implementation file
//

#include "stdafx.h"
#include "StBinder.h"
#include "TargetMiscDlg.h"

extern TARGET_CFG_DATA g_ResCfgData;

// CTargetMisc dialog

IMPLEMENT_DYNAMIC(CTargetMiscDlg, CPropertyPage)

CTargetMiscDlg::CTargetMiscDlg(/*CWnd* pParent =NULL*/)
	: CPropertyPage(CTargetMiscDlg::IDD)
{

}

CTargetMiscDlg::~CTargetMiscDlg()
{
}

void CTargetMiscDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_TARGET_OPTIONS_REBOOT2PLAYER, m_ShowBootToPlayerMsg);
	DDX_Control(pDX, IDC_LOWNAND, m_LowNandBtn);
	DDX_Control(pDX, IDC_TARGET_OPTIONS_FMT_WARN, m_MinDlgFmtMsg);
}


BEGIN_MESSAGE_MAP(CTargetMiscDlg, CDialog)
END_MESSAGE_MAP()


BOOL CTargetMiscDlg::OnInitDialog()
{
	CString resStr;

	CPropertyPage::OnInitDialog();

	((CStBinderApp*)AfxGetApp())->GetString(IDS_TARGET_OPTIONS_REBOOT2PLAYER_TEXT, resStr);
	SetDlgItemText(IDC_TARGET_OPTIONS_REBOOT2PLAYER, resStr );

	m_ShowBootToPlayerMsg.SetCheck(g_ResCfgData.Options.RebootToPlayerMsg);

	resStr = g_ResCfgData.Options.BaseSDK;
	if (resStr.Find(_T("SDK3")) == -1)
		m_ShowBootToPlayerMsg.EnableWindow(FALSE);  // Disable for all but SDK3xx

	((CStBinderApp*)AfxGetApp())->GetString(IDS_TARGET_OPTIONS_LOWNAND_TEXT, resStr);
	SetDlgItemText(IDC_TARGET_OPTIONS_LOWNAND, resStr );

	m_LowNandBtn.SetCheck(g_ResCfgData.Options.LowNANDSolution);

	((CStBinderApp*)AfxGetApp())->GetString(IDS_TARGET_OPTIONS_FMT_WARN, resStr);
	SetDlgItemText(IDC_TARGET_OPTIONS_FMT_WARN, resStr );

	m_LowNandBtn.SetCheck(g_ResCfgData.Options.MinDlgFmtMsg);


    return TRUE;  // return TRUE  unless you set the focus to a control
}

// CTargetMisc message handlers
void CTargetMiscDlg::OnOK()
{
	g_ResCfgData.Options.RebootToPlayerMsg = m_ShowBootToPlayerMsg.GetCheck();
	g_ResCfgData.Options.LowNANDSolution = m_LowNandBtn.GetCheck();

	CPropertyPage::OnOK();
}

void CTargetMiscDlg::OnCancel()
{
	CPropertyPage::OnCancel();
}
