/*
 * Copyright (C) 2010, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */
// LoadProfileDlg.cpp : implementation file
//

#include "stdafx.h"
#include "StBinder.h"
#include "StBinderProfile.h"
#include "TargetCfgData.h"
#include "LoadProfileDlg.h"

extern TARGET_CFG_DATA g_ResCfgData;

// CLoadProfileDlg dialog

IMPLEMENT_DYNAMIC(CLoadProfileDlg, CDialog)

CLoadProfileDlg::CLoadProfileDlg(CStBinderDlg *_binderDlg, CWnd* pParent /*=NULL*/)
	: CDialog(CLoadProfileDlg::IDD, pParent)
{
	m_pBinderDlg = _binderDlg;
}

CLoadProfileDlg::~CLoadProfileDlg()
{
}

void CLoadProfileDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_CFG_LOAD_PROFILES, m_ProfileListCtl);
}


BEGIN_MESSAGE_MAP(CLoadProfileDlg, CDialog)
	ON_BN_CLICKED(IDOK, &CLoadProfileDlg::OnBnClickedOk)
	ON_BN_CLICKED(IDCANCEL, &CLoadProfileDlg::OnBnClickedCancel)
END_MESSAGE_MAP()


BOOL CLoadProfileDlg::OnInitDialog()
{
	CString resStr;
	LONG lStatus = -1;
	HKEY hProfilesKey = NULL;

	CDialog::OnInitDialog();

	((CStBinderApp*)AfxGetApp())->GetString(IDS_PROFILE_LOAD_TEXT, resStr);
	SetDlgItemText(IDC_CFG_LOAD_TEXT, resStr );


	lStatus = RegCreateKeyEx(HKEY_CURRENT_USER, _T("Software\\Freescale\\StBinder\\Profiles"), 0, NULL,
					REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hProfilesKey, NULL);

	DWORD dwIndex = 0;
	while (lStatus == ERROR_SUCCESS)
	{
		TCHAR nameStr[256];
		DWORD dwSize = 256;

		lStatus = RegEnumKeyEx(hProfilesKey, dwIndex, nameStr, &dwSize, NULL, NULL, NULL, NULL );

		if (lStatus == ERROR_SUCCESS)
		{
			++dwIndex;
			m_ProfileListCtl.InsertString(-1, nameStr);
		}
	}
	m_ProfileListCtl.UpdateData(FALSE);

    return TRUE;  // return TRUE  unless you set the focus to a control
}

// CLoadProfileDlg message handlers

void CLoadProfileDlg::OnBnClickedOk()
{
	int index = m_ProfileListCtl.GetCurSel();

	if (index != LB_ERR)
	{
		CString csProfile;
		m_ProfileListCtl.GetText(index, csProfile);
		StBinderProfile *pProfile = new StBinderProfile(csProfile);
		//pProfile->ReadProfileIniData();
		pProfile->ReadProfileRegData(m_pBinderDlg);
		delete pProfile;
		g_ResCfgData.ProfileName = csProfile;
	}
	OnOK();
}

void CLoadProfileDlg::OnBnClickedCancel()
{
	OnCancel();
}
