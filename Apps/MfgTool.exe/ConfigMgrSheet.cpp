/*
 * Copyright (C) 2009-2011, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */
// ConfigMgrSheet.cpp : implementation file
//

#include "stdafx.h"
#include "StMfgTool.h"
#include "ConfigMgrSheet.h"


// CConfigMgrSheet

IMPLEMENT_DYNAMIC(CConfigMgrSheet, CPropertySheet)
CConfigMgrSheet::CConfigMgrSheet(UINT nIDCaption, CWnd* pParentWnd, UINT iSelectPage)
	:CPropertySheet(nIDCaption, pParentWnd, iSelectPage)
{
	AddControlPages();
}

CConfigMgrSheet::CConfigMgrSheet(LPCTSTR pszCaption, CWnd* pParentWnd, UINT iSelectPage)
	:CPropertySheet(pszCaption, pParentWnd, iSelectPage)
{
	AddControlPages();
}

CConfigMgrSheet::~CConfigMgrSheet()
{
	RemovePage(&m_player_profile_page);
	RemovePage(&m_usb_port_page);
	RemovePage(&m_general_page);
}

#ifdef _DEBUG
void CConfigMgrSheet::Dump(CDumpContext& dc) const
{
	CPropertySheet::Dump(dc);
	dc << "CConfigMgrSheet = " << this;
}
#endif

void CConfigMgrSheet::AddControlPages()
{
	m_psh.dwFlags |= PSH_NOAPPLYNOW;    // Lose the Apply Now button
	m_psh.dwFlags &= ~PSH_HASHELP;      // Lose the Help button

	AddPage(&m_player_profile_page);
	AddPage(&m_usb_port_page);
	AddPage(&m_general_page);
}


BEGIN_MESSAGE_MAP(CConfigMgrSheet, CPropertySheet)
END_MESSAGE_MAP()

DWORD CConfigMgrSheet::InitConfigMgr(void)
{
	m_player_profile_page.InitProfileList();

	// add Filters to DeviceManager
//clw	if ( GetPlayerProfile() && GetPlayerProfile()->IsValid() )
//clw	{
//clw		gDeviceManager::Instance().UpdateDeviceFilters(GetPlayerProfile()->GetUsbVid(), GetPlayerProfile()->GetUsbPid());
//clw	}

	return 0;
}

BOOL CConfigMgrSheet::OnInitDialog()
{
    BOOL bResult = CPropertySheet::OnInitDialog();
    CString resStr;

    CRect rectBtn, rectClient;
    GetDlgItem(IDOK)->GetWindowRect(rectBtn);
    ScreenToClient (&rectBtn);
    GetClientRect(&rectClient);
    rectBtn.MoveToX(rectClient.Width()/2-(rectBtn.Width())-10);
    GetDlgItem (IDOK)->MoveWindow(rectBtn);

    resStr.LoadString(IDS_OK);
	SetDlgItemText(IDOK, resStr );
	resStr.LoadString(IDS_CANCEL);
	SetDlgItemText(IDCANCEL, resStr );

    GetDlgItem(IDCANCEL)->GetWindowRect(rectBtn);
    ScreenToClient (&rectBtn);
    GetClientRect(&rectClient);
    rectBtn.MoveToX((rectClient.Width()/2)+10);
    GetDlgItem (IDCANCEL)->MoveWindow(rectBtn);

    // move entire dialog so we can see the main app behind it
    CRect rect;
    GetWindowRect(&rect);
    rect.OffsetRect(75,75);
    MoveWindow(&rect, TRUE);

    return bResult;
}

void CConfigMgrSheet::SelectPlayerProfile(LPCTSTR _szProfile)
{
	m_player_profile_page.SelectProfile( _szProfile );
}
