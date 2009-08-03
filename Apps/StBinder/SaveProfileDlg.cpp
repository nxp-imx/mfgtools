// SaveProfileDlg.cpp : implementation file
//

#include "stdafx.h"
#include "StBinder.h"
#include "StBinderDlg.h"
#include "TargetCfgData.h"
#include "StBinderProfile.h"
#include "SaveProfileDlg.h"

extern TARGET_CFG_DATA g_ResCfgData;

// SaveProfileDlg dialog

IMPLEMENT_DYNAMIC(CSaveProfileDlg, CDialog)

CSaveProfileDlg::CSaveProfileDlg(CStBinderDlg *_binderDlg, CWnd* pParent /*=NULL*/)
	: CDialog(CSaveProfileDlg::IDD, pParent)
{
	m_pBinderDlg = _binderDlg;
}

CSaveProfileDlg::~CSaveProfileDlg()
{
}

void CSaveProfileDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_CFG_SAVE_PROFILE_AS, m_profileName);
}


BEGIN_MESSAGE_MAP(CSaveProfileDlg, CDialog)
	ON_BN_CLICKED(IDOK, &CSaveProfileDlg::OnBnClickedOk)
	ON_BN_CLICKED(IDCANCEL, &CSaveProfileDlg::OnBnClickedCancel)
END_MESSAGE_MAP()


BOOL CSaveProfileDlg::OnInitDialog()
{
	CString resStr;

	CDialog::OnInitDialog();

	((CStBinderApp*)AfxGetApp())->GetString(IDS_PROFILE_SAVE_TEXT, resStr);
	SetDlgItemText(IDC_CFG_SAVE_PROFILE_TEXT, resStr );

	if (!g_ResCfgData.ProfileName.IsEmpty())
		m_profileName.SetWindowTextW(g_ResCfgData.ProfileName);
	else
		m_profileName.SetWindowTextW(g_ResCfgData.Options.ProductName);

	return TRUE;
}

// SaveProfileDlg message handlers

void CSaveProfileDlg::OnBnClickedOk()
{
	CString profileName;

	m_profileName.GetWindowTextW(profileName);
	StBinderProfile *pProfile = new StBinderProfile(profileName);
	//pProfile->WriteProfileIniData();
	pProfile->WriteProfileRegData(m_pBinderDlg);
	g_ResCfgData.ProfileName = profileName;
	delete pProfile;

	OnOK();
}

void CSaveProfileDlg::OnBnClickedCancel()
{
	OnCancel();
}
