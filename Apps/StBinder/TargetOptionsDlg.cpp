// TargetOptionsDlg.cpp : implementation file
//

#include "stdafx.h"
#include "StBinder.h"
#include "TargetOptionsDlg.h"

extern TARGET_CFG_DATA g_ResCfgData;


// CTargetOptionsDlg dialog

IMPLEMENT_DYNAMIC(CTargetOptionsDlg, CPropertyPage)

CTargetOptionsDlg::CTargetOptionsDlg(/*CWnd* pParent =NULL*/)
	: CPropertyPage(CTargetOptionsDlg::IDD)
{
}

CTargetOptionsDlg::~CTargetOptionsDlg()
{
}

void CTargetOptionsDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_TARGET_OPTIONS_AUTORECV, m_AutoRecoveryBtn);
	DDX_Control(pDX, IDC_TARGET_OPTIONS_FORCERECV, m_ForceRecvBtn);
	DDX_Control(pDX, IDC_TARGET_OPTIONS_USELOCALFILES, m_UseLocalFilesBtn);
	DDX_Control(pDX, IDC_TARGET_OPTIONS_MINDLG, m_MinDlgBtn);
	DDX_Control(pDX, IDC_TARGET_OPTIONS_STDDLG, m_StdDlgBtn);
	DDX_Control(pDX, IDC_TARGET_OPTIONS_ADVDLG, m_AdvDlgBtn);
	DDX_Control(pDX, IDC_TARGET_OPTIONS_FORMATDATA, m_FormatDataBtn);
	DDX_Control(pDX, IDC_TARGET_OPTIONS_ERASEMEDIA, m_EraseMediaBtn);
	DDX_Control(pDX, IDC_TARGET_OPTIONS_FILESYSTEM, m_DefaultFileSystem);
	DDX_Control(pDX, IDC_TARGET_OPTIONS_WINCE, m_WinCEDownload);
	DDX_Control(pDX, IDC_TARGET_OPTIONS_AUTOSTART, m_AutoStartBtn);
	DDX_Control(pDX, IDC_TARGET_OPTIONS_AUTOCLOSE, m_AutoCloseBtn);
	DDX_Control(pDX, IDC_TARGET_OPTIONS_LABEL, m_DriveLabel);
}


BEGIN_MESSAGE_MAP(CTargetOptionsDlg, CPropertyPage)
	ON_BN_CLICKED(IDC_TARGET_OPTIONS_FORMATDATA, &CTargetOptionsDlg::OnBnClickedTargetOptionsFormatData)
	ON_BN_CLICKED(IDC_TARGET_OPTIONS_ERASEMEDIA, &CTargetOptionsDlg::OnBnClickedTargetOptionsEraseMedia)
	ON_BN_CLICKED(IDC_TARGET_OPTIONS_MINDLG, &CTargetOptionsDlg::OnBnClickedMinDlg)
	ON_BN_CLICKED(IDC_TARGET_OPTIONS_STDDLG, &CTargetOptionsDlg::OnBnClickedStdDlg)
	ON_BN_CLICKED(IDC_TARGET_OPTIONS_ADVDLG, &CTargetOptionsDlg::OnBnClickedAdvDlg)
END_MESSAGE_MAP()


BOOL CTargetOptionsDlg::OnInitDialog()
{
	CString resStr;

	CPropertyPage::OnInitDialog();
/*
	m_AutoRecoveryBtn.EnableWindow(FALSE);
	m_ForceRecvBtn.EnableWindow(FALSE);
	m_UseLocalFilesBtn.EnableWindow(FALSE);
	m_MinDlgBtn.EnableWindow(FALSE);
	m_StdDlgBtn.EnableWindow(FALSE);
	m_AdvDlgBtn.EnableWindow(FALSE);
*/

	((CStBinderApp*)AfxGetApp())->GetString(IDS_TARGET_OPTIONS_FS_DEFAULT, resStr);
	m_DefaultFileSystem.InsertString(0, resStr);
	m_DefaultFileSystem.InsertString(1, _T("FAT"));
	m_DefaultFileSystem.InsertString(2, _T("FAT32"));

	GetDlgItem(IDC_TARGET_OPTIONS_FILESYSTEM)->EnableWindow(FALSE);

	((CStBinderApp*)AfxGetApp())->GetString(IDS_TARGET_OPTIONS_AUTORECV_TEXT, resStr);
	SetDlgItemText(IDC_TARGET_OPTIONS_AUTORECV, resStr );

	((CStBinderApp*)AfxGetApp())->GetString(IDS_TARGET_OPTIONS_FORCERECV_TEXT, resStr);
	SetDlgItemText(IDC_TARGET_OPTIONS_FORCERECV, resStr );

	((CStBinderApp*)AfxGetApp())->GetString(IDS_TARGET_OPTIONS_USELOCALFILES_TEXT, resStr);
	SetDlgItemText(IDC_TARGET_OPTIONS_USELOCALFILES, resStr );

	((CStBinderApp*)AfxGetApp())->GetString(IDS_TARGET_OPTIONS_DIALOG_TEXT, resStr);
	SetDlgItemText(IDC_TARGET_OPTIONS_DIALOG_TEXT, resStr );

	((CStBinderApp*)AfxGetApp())->GetString(IDS_TARGET_OPTIONS_FORMATDATAAREA_TEXT, resStr);
	SetDlgItemText(IDC_TARGET_OPTIONS_FORMATDATA, resStr );

	((CStBinderApp*)AfxGetApp())->GetString(IDS_TARGET_OPTIONS_ERASEMEDIA_TEXT, resStr);
	SetDlgItemText(IDC_TARGET_OPTIONS_ERASEMEDIA, resStr );

	((CStBinderApp*)AfxGetApp())->GetString(IDS_TARGET_OPTIONS_DEFAULT_FS_TEXT, resStr);
	SetDlgItemText(IDC_TARGET_OPTIONS_FILESYSTEM_TEXT, resStr );
	GetDlgItem(IDC_TARGET_OPTIONS_FILESYSTEM_TEXT)->EnableWindow(FALSE);

	((CStBinderApp*)AfxGetApp())->GetString(IDS_TARGET_OPTIONS_LABEL_TEXT, resStr);
	SetDlgItemText(IDC_TARGET_OPTIONS_LABEL_TEXT, resStr );
	m_DriveLabel.SetValidCharSet(CVisValidEdit::SET_ALLCHAR);
	m_DriveLabel.SetTextLenMinMax(1,DRIVELABEL_MAX);
	m_DriveLabel.SetEnableLengthCheck();
	m_DriveLabel.SetMyControlID(0x01);
	m_DriveLabel.SetWindowTextW(g_ResCfgData.Options.DriveLabel);
	m_DriveLabel.EnableWindow(FALSE);

	m_AutoRecoveryBtn.SetCheck(g_ResCfgData.Options.AllowAutoRecovery);
	m_ForceRecvBtn.SetCheck(g_ResCfgData.Options.ForceRecovery);
	m_FormatDataBtn.SetCheck(g_ResCfgData.Options.FormatDataArea);
	m_EraseMediaBtn.SetCheck(g_ResCfgData.Options.EraseMedia);
	m_UseLocalFilesBtn.SetCheck(g_ResCfgData.Options.UseLocalFileResources);
	m_dlgType = g_ResCfgData.Options.DialogType;
	m_AutoStartBtn.SetCheck(g_ResCfgData.Options.AutoStart);
	m_AutoCloseBtn.SetCheck(g_ResCfgData.Options.AutoClose);

	((CStBinderApp*)AfxGetApp())->GetString(IDS_TARGET_OPTIONS_AUTOSTART_TEXT, resStr);
	SetDlgItemText(IDC_TARGET_OPTIONS_AUTOSTART, resStr );

	((CStBinderApp*)AfxGetApp())->GetString(IDS_TARGET_OPTIONS_AUTOCLOSE_TEXT, resStr);
	SetDlgItemText(IDC_TARGET_OPTIONS_AUTOCLOSE, resStr );

	if (g_ResCfgData.Options.FormatDataArea)
	{
		GetDlgItem(IDC_TARGET_OPTIONS_FILESYSTEM_TEXT)->EnableWindow(TRUE);
		GetDlgItem(IDC_TARGET_OPTIONS_FILESYSTEM)->EnableWindow(TRUE);
		m_DriveLabel.EnableWindow(TRUE);
	}

	if (g_ResCfgData.Options.DialogType == DLG_MINIMAL)
		m_MinDlgBtn.SetCheck(TRUE);
	else
		if (g_ResCfgData.Options.DialogType == DLG_STANDARD)
			m_StdDlgBtn.SetCheck(TRUE);
		else
		if (g_ResCfgData.Options.DialogType == DLG_ADVANCED)
			m_AdvDlgBtn.SetCheck(TRUE);


	m_DefaultFileSystem.SetCurSel(g_ResCfgData.Options.DefaultFS);

    return TRUE;  // return TRUE  unless you set the focus to a control
}
// CTargetOptionsDlg message handlers

void CTargetOptionsDlg::OnOK()
{
	g_ResCfgData.Options.AllowAutoRecovery = m_AutoRecoveryBtn.GetCheck();
	g_ResCfgData.Options.ForceRecovery = m_ForceRecvBtn.GetCheck();
	g_ResCfgData.Options.UseLocalFileResources = m_UseLocalFilesBtn.GetCheck();
	g_ResCfgData.Options.DialogType = m_dlgType;
	g_ResCfgData.Options.FormatDataArea = m_FormatDataBtn.GetCheck();
	g_ResCfgData.Options.EraseMedia = m_EraseMediaBtn.GetCheck();
	g_ResCfgData.Options.DefaultFS = m_DefaultFileSystem.GetCurSel(); //Default = 0, FAT = 1, FAT32 = 2
	g_ResCfgData.Options.AutoStart = m_AutoStartBtn.GetCheck();
	g_ResCfgData.Options.AutoClose = m_AutoCloseBtn.GetCheck();
	if (g_ResCfgData.Options.FormatDataArea)
		m_DriveLabel.GetWindowTextW(g_ResCfgData.Options.DriveLabel, DRIVELABEL_MAX+1);
	CPropertyPage::OnOK();
}

void CTargetOptionsDlg::OnCancel()
{
	CPropertyPage::OnCancel();
}

void CTargetOptionsDlg::OnBnClickedTargetOptionsFormatData()
{
	if ( m_FormatDataBtn.GetCheck() )
	{
		GetDlgItem(IDC_TARGET_OPTIONS_FILESYSTEM_TEXT)->EnableWindow(TRUE);
		GetDlgItem(IDC_TARGET_OPTIONS_FILESYSTEM)->EnableWindow(TRUE);
		m_DriveLabel.EnableWindow(TRUE);
	}
	else
	{
		GetDlgItem(IDC_TARGET_OPTIONS_FILESYSTEM_TEXT)->EnableWindow(FALSE);
		GetDlgItem(IDC_TARGET_OPTIONS_FILESYSTEM)->EnableWindow(FALSE);
		m_DriveLabel.EnableWindow(FALSE);

		if( m_EraseMediaBtn.GetCheck() )
			m_EraseMediaBtn.SetCheck(BST_UNCHECKED);
	}
	
}

void CTargetOptionsDlg::OnBnClickedTargetOptionsEraseMedia()
{
	m_FormatDataBtn.SetCheck(BST_CHECKED);
	OnBnClickedTargetOptionsFormatData();
}

void CTargetOptionsDlg::OnBnClickedMinDlg()
{
	m_dlgType = DLG_MINIMAL;
}

void CTargetOptionsDlg::OnBnClickedStdDlg()
{
	m_dlgType = DLG_STANDARD;
}

void CTargetOptionsDlg::OnBnClickedAdvDlg()
{
	m_dlgType = DLG_ADVANCED;
}




