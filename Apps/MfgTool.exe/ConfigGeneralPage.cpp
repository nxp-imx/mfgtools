/*
 * Copyright (C) 2009-2011, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */
// ConfigGeneralPage.cpp : implementation file
//

#include "stdafx.h"
#include "StMfgTool.h"
#include "ConfigGeneralPage.h"

// CConfigGeneralPage dialog

IMPLEMENT_DYNAMIC(CConfigGeneralPage, CPropertyPage)
CConfigGeneralPage::CConfigGeneralPage(/*CSBPortMgr* pPortMgr*/)
	: CPropertyPage(CConfigGeneralPage::IDD)
//	, m_language(_T("English"))
	, m_supress_autoplay(TRUE)
	, m_autoplay_sel_drv_list(_T(""))
{
	m_ops_restricted = theApp.GetProfileInt(_T("Options"), _T("Restrict Operations to Profile Devices"), 1);
	m_event_log_enable = theApp.GetProfileInt(_T("Options"), _T("Enable Event Logging"), 1);
	m_use_comport_msgs = theApp.GetProfileInt(_T("Options"), _T("Enable COM port status msgs"), 0);
}

CConfigGeneralPage::~CConfigGeneralPage()
{
}

#ifdef _DEBUG
void CConfigGeneralPage::Dump(CDumpContext& dc) const
{
	CPropertyPage::Dump(dc);
	dc << "CConfigGeneralPage = " << this;
}
#endif

void CConfigGeneralPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	DDX_Check(pDX, IDC_RESTRICT_DEVICE_CHECK, m_ops_restricted);
//	DDX_Control(pDX, IDC_LANGUAGE_COMBO, m_language_ctrl);
//	DDX_CBString(pDX, IDC_LANGUAGE_COMBO, m_language);
	DDX_Check(pDX, IDC_SUPRESS_AUTOPLAY, m_supress_autoplay);
	DDX_Control(pDX, IDC_AUTOPLAY_ALL_DRVS_RADIO, m_all_drvs_ctrl);
	DDX_Control(pDX, IDC_AUTOPLAY_SEL_DRVS_RADIO, m_sel_drvs_ctrl);
	DDX_Control(pDX, IDC_IDC_AUTOPLAY_DRV_LIST, m_autoplay_drv_list_ctrl);
	DDX_Check(pDX, IDC_EVENT_LOG_ENABLE_CHECK, m_event_log_enable);
	DDX_Check(pDX, IDC_CFG_COM_PORT_MSGS, m_use_comport_msgs);
}

BEGIN_MESSAGE_MAP(CConfigGeneralPage, CPropertyPage)
	ON_BN_CLICKED(IDC_RESTRICT_DEVICE_CHECK, OnBnClickedRestrictOps)
	ON_BN_CLICKED(IDC_SUPRESS_AUTOPLAY, OnBnClickedSupressAutoplay)
	ON_BN_CLICKED(IDC_AUTOPLAY_ALL_DRVS_RADIO, OnBnClickedAutoplayAllDrvsRadio)
	ON_BN_CLICKED(IDC_AUTOPLAY_SEL_DRVS_RADIO, OnBnClickedAutoplaySelDrvsRadio)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_IDC_AUTOPLAY_DRV_LIST, OnLvnItemchangedIdcAutoplayDrvList)
	ON_WM_DESTROY()
    ON_BN_CLICKED(IDC_EVENT_LOG_ENABLE_CHECK, OnBnClickedEventLogEnableCheck)
    ON_BN_CLICKED(IDC_CFG_COM_PORT_MSGS, OnBnClickedEventUseComPortMsgs)
END_MESSAGE_MAP()

// CConfigGeneralPage message handlers

BOOL CConfigGeneralPage::OnInitDialog()
{
	CPropertyPage::OnInitDialog();

	Localize();
	UpdateData(FALSE);
//	m_language_ctrl.SetCurSel(0);
//	m_language_ctrl.EnableWindow(FALSE);
	m_supress_autoplay = theApp.GetProfileInt(_T("AutoPlay"), _T("Enable AutoPlay Rejection"), 1);
	m_all_drvs_ctrl.SetCheck(theApp.GetProfileInt(_T("AutoPlay"), _T("Reject All Drives"), 1));
	m_sel_drvs_ctrl.SetCheck(!theApp.GetProfileInt(_T("AutoPlay"), _T("Reject All Drives"), 1));
    UpdateData(FALSE);
	OnBnClickedSupressAutoplay();
	m_autoplay_sel_drv_list = theApp.GetProfileString(_T("AutoPlay"), _T("Selected Drives"), _T(""));
	InitAutoPlayListCtrl();

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CConfigGeneralPage::OnOK()
{
	CPropertyPage::OnOK();

	theApp.WriteProfileInt(_T("Options"), _T("Restrict Operations to Profile Devices"), m_ops_restricted);

	theApp.WriteProfileInt(_T("AutoPlay"), _T("Enable AutoPlay Rejection"), m_supress_autoplay);
	// turn on/off the AutoPlay rejection mechanism
	gDeviceManager::Instance().SetCancelAutoPlay(m_supress_autoplay == TRUE);

	theApp.WriteProfileInt(_T("AutoPlay"), _T("Reject All Drives"), m_all_drvs_ctrl.GetCheck());
	// turn on/off the AutoPlay rejection mechanism
	gDeviceManager::Instance().SetCancelAutoPlay(m_supress_autoplay == TRUE, _T("DEFGHIJKLMNOPQRSTUVWXYZ"));

	theApp.WriteProfileInt(_T("AutoPlay"), _T("Reject All Drives"), !m_sel_drvs_ctrl.GetCheck());
	// turn on/off the AutoPlay rejection mechanism
	gDeviceManager::Instance().SetCancelAutoPlay(m_supress_autoplay == TRUE, m_autoplay_sel_drv_list);

	theApp.WriteProfileString(_T("AutoPlay"), _T("Selected Drives"), m_autoplay_sel_drv_list);
	// turn on/off the AutoPlay rejection mechanism
	gDeviceManager::Instance().SetCancelAutoPlay(m_supress_autoplay == TRUE, m_autoplay_sel_drv_list);

	theApp.WriteProfileInt(_T("AutoPlay"), _T("Scroll Position"), m_autoplay_drv_list_ctrl.GetTopIndex());

	theApp.WriteProfileInt(_T("Options"), _T("Enable Event Logging"), m_event_log_enable);

	theApp.WriteProfileInt(_T("Options"), _T("Enable COM port status msgs"), m_use_comport_msgs);
}

void CConfigGeneralPage::OnCancel()
{
	CPropertyPage::OnCancel();
}

void CConfigGeneralPage::OnBnClickedRestrictOps()
{
	UpdateData(TRUE);
}

void CConfigGeneralPage::OnBnClickedSupressAutoplay()
{
	UpdateData(TRUE);

	if ( m_supress_autoplay ) {
		m_all_drvs_ctrl.EnableWindow(true);
		m_sel_drvs_ctrl.EnableWindow(true);
		if ( m_all_drvs_ctrl.GetCheck() ) {
			m_autoplay_drv_list_ctrl.EnableWindow(false);
		}
		else {
			m_autoplay_drv_list_ctrl.EnableWindow(true);
		}
	}
	else {
		m_all_drvs_ctrl.EnableWindow(false);
		m_sel_drvs_ctrl.EnableWindow(false);
		m_autoplay_drv_list_ctrl.EnableWindow(false);
	}
}

void CConfigGeneralPage::OnBnClickedAutoplayAllDrvsRadio()
{

	if ( m_all_drvs_ctrl.GetCheck() )
	{
		m_autoplay_drv_list_ctrl.EnableWindow(false);
	}
}

void CConfigGeneralPage::OnBnClickedAutoplaySelDrvsRadio()
{
	if ( m_sel_drvs_ctrl.GetCheck() )
	{
		m_autoplay_drv_list_ctrl.EnableWindow(true);
	}
}

void CConfigGeneralPage::InitAutoPlayListCtrl(void)
{
	CRect rect;
	m_autoplay_drv_list_ctrl.GetClientRect(&rect);
	m_autoplay_drv_list_ctrl.InsertColumn(0,_T("Drive"),LVCFMT_LEFT, rect.Width()-GetSystemMetrics(SM_CXVSCROLL));
	m_autoplay_drv_list_ctrl.SetExtendedStyle(LVS_EX_CHECKBOXES /*| LVS_EX_GRIDLINES*/);
	TCHAR _char; int item; _char=_T('D'); CString str;
	for ( int i=0; _char<_T('Z'); ++i ) {
		_char=_T('A') + i; 
		str.Format(_T("%c:"), _char);
		item = m_autoplay_drv_list_ctrl.InsertItem(i, str);
		if ( CString(_char).FindOneOf(m_autoplay_sel_drv_list) != -1 ) {
			m_autoplay_sel_drv_list.Remove(_char);
			m_autoplay_drv_list_ctrl.SetCheck(item);
		}		
	}
	m_autoplay_drv_list_ctrl.EnsureVisible(theApp.GetProfileInt(_T("AutoPlay"), _T("Scroll Position"), 0)+4,FALSE);
}

void CConfigGeneralPage::OnLvnItemchangedIdcAutoplayDrvList(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	*pResult = 0;

	if (pNMLV->uOldState == 0 && pNMLV->uNewState == 0)
		return;	// No change

	BOOL bPrevState = (BOOL)(((pNMLV->uOldState & 
				LVIS_STATEIMAGEMASK)>>12)-1);   // Old check box state
	if (bPrevState < 0)	// On startup there's no previous state 
		bPrevState = 0; // so assign as false (unchecked)

	// New check box state
	BOOL bChecked=(BOOL)(((pNMLV->uNewState & LVIS_STATEIMAGEMASK)>>12)-1);   
	if (bChecked < 0) // On non-checkbox notifications assume false
		bChecked = 0; 

	if (bPrevState == bChecked) // No change in check box
		return;
	
	// Now bChecked holds the new check box state
	if ( bChecked ) {
		m_autoplay_sel_drv_list.AppendChar(m_autoplay_drv_list_ctrl.GetItemText(pNMLV->iItem, 0)[0]);
	}
	else {
		m_autoplay_sel_drv_list.Remove(m_autoplay_drv_list_ctrl.GetItemText(pNMLV->iItem, 0)[0]);
	}
}

void CConfigGeneralPage::OnDestroy()
{
	CPropertyPage::OnDestroy();
}

CString CConfigGeneralPage::GetAutoPlayDrvList(void) {
	CString drv_list;
	if ( theApp.GetProfileInt(_T("AutoPlay"), _T("Enable AutoPlay Rejection"), 1) )
		if ( theApp.GetProfileInt(_T("AutoPlay"), _T("Reject All Drives"), 1) )
			drv_list = _T("ABCDEFGHIJKLMNOPQRSTUVWXYZ");
		else
			drv_list = theApp.GetProfileString(_T("AutoPlay"), _T("Selected Drives"), _T(""));
	else 
		drv_list.Empty();
	
	return drv_list;
}
BOOL CConfigGeneralPage::GetAutoPlayEnabled(void)
{
	return theApp.GetProfileInt(_T("AutoPlay"), _T("Enable AutoPlay Rejection"), 1);
}

void CConfigGeneralPage::OnBnClickedEventLogEnableCheck()
{
	UpdateData(TRUE);
}

void CConfigGeneralPage::OnBnClickedEventUseComPortMsgs()
{
	UpdateData(TRUE);
}

void CConfigGeneralPage::Localize()
{
	CString resStr;

	resStr.LoadString(IDS_CFG_GENERAL_TITLE);
	SetWindowText(resStr);

	resStr.LoadString(IDS_CFG_GENERAL_RESTRICT_DEVICE);
	SetDlgItemText(IDC_RESTRICT_DEVICE_CHECK, resStr );

	resStr.LoadString(IDS_CFG_GENERAL_SUPRESS_AUTOPLAY);
	SetDlgItemText(IDC_SUPRESS_AUTOPLAY, resStr );

	resStr.LoadString(IDS_CFG_GENERAL_AUTOPLAY_ALL_DRVS);
	SetDlgItemText(IDC_AUTOPLAY_ALL_DRVS_RADIO, resStr );

	resStr.LoadString(IDS_CFG_GENERAL_AUTOPLAY_SELECT_DRIVE);
	SetDlgItemText(IDC_AUTOPLAY_SEL_DRVS_RADIO, resStr );

//	resStr.LoadString(IDS_CFG_GENERAL_LANGUAGE_GROUP);
//	SetDlgItemText(IDC_CFG_GENERAL_LANGUAGE_GROUP_TEXT, resStr );

	resStr.LoadString(IDS_CFG_GENERAL_OPTIONS_GROUP);
	SetDlgItemText(IDC_CFG_GENERAL_OPTIONS_GROUP_TEXT, resStr );

//	resStr.LoadString(IDS_CFG_GENERAL_DEFAULT_LANGUAGE);
//	SetDlgItemText(IDC_CFG_GENERAL_DEFAULT_LANGUAGE_TEXT, resStr );

	resStr.LoadString(IDS_CFG_GENERAL_AUTOPLAY_GROUP);
	SetDlgItemText(IDC_CFG_GENERAL_AUTOPLAY_GROUP_TEXT, resStr );

	resStr.LoadString(IDS_CFG_GENERAL_ENABLE_LOGGING);
	SetDlgItemText(IDC_EVENT_LOG_ENABLE_CHECK, resStr );
}
