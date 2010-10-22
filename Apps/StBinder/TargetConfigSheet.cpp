/*
 * Copyright (C) 2010, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */
// ConfigMgrSheet.cpp : implementation file
//

#include "stdafx.h"
#include "stdafx.h"
#include "resource.h"
#include "TargetConfigSheet.h"


// CConfigMgrSheet

IMPLEMENT_DYNAMIC(CTargetConfigSheet, CPropertySheet)
CTargetConfigSheet::CTargetConfigSheet(UINT nIDCaption, CWnd* pParentWnd, UINT iSelectPage)
	:CPropertySheet(nIDCaption, pParentWnd, iSelectPage)
{
	AddControlPages();
}

CTargetConfigSheet::CTargetConfigSheet(LPCTSTR pszCaption, CWnd* pParentWnd, UINT iSelectPage)
	:CPropertySheet(pszCaption, pParentWnd, iSelectPage)
{
	AddControlPages();
}

CTargetConfigSheet::~CTargetConfigSheet()
{
	RemovePage(&m_customer_page);
	RemovePage(&m_deviceids_page);
	RemovePage(&m_options_page);
	RemovePage(&m_drivearray_page);
	RemovePage(&m_misc_page);

}


void CTargetConfigSheet::AddControlPages()
{
	m_psh.dwFlags |= PSH_NOAPPLYNOW;    // Lose the Apply Now button
	m_psh.dwFlags &= ~PSH_HASHELP;      // Lose the Help button

	AddPage(&m_customer_page);
	AddPage(&m_deviceids_page);
	AddPage(&m_options_page);
	AddPage(&m_drivearray_page);
	AddPage(&m_misc_page);
}


BEGIN_MESSAGE_MAP(CTargetConfigSheet, CPropertySheet)
END_MESSAGE_MAP()



BOOL CTargetConfigSheet::OnInitDialog()
{
    BOOL bResult = CPropertySheet::OnInitDialog();
    CString resStr;

    CRect rectBtn, rectClient;
    GetDlgItem(IDOK)->GetWindowRect(rectBtn);
    ScreenToClient (&rectBtn);
    GetClientRect(&rectClient);
    rectBtn.MoveToX(rectClient.Width()/2-(rectBtn.Width())-10);
    GetDlgItem (IDOK)->MoveWindow(rectBtn);

    bResult = resStr.LoadString(IDS_OK);
	SetDlgItemText(IDOK, resStr );
	bResult = resStr.LoadString(IDS_CANCEL);
	SetDlgItemText(IDCANCEL, resStr );

    GetDlgItem(IDCANCEL)->GetWindowRect(rectBtn);
    ScreenToClient (&rectBtn);
    GetClientRect(&rectClient);
    rectBtn.MoveToX((rectClient.Width()/2)+10);
    GetDlgItem (IDCANCEL)->MoveWindow(rectBtn);

	// removing the help bit in the flags doesn't seem to work so let's hide it
	GetDlgItem(IDHELP)->ShowWindow(SW_HIDE);
	GetDlgItem(IDHELP)->EnableWindow(FALSE);

    // move entire dialog so we can see the main app behind it
    CRect rect;
    GetWindowRect(&rect);
    rect.OffsetRect(75,75);
    MoveWindow(&rect, TRUE);

    return bResult;
}


