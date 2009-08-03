#pragma once
#include "afxwin.h"

#include "../../Libs/WinSupport/visvaledit.h"
#include "StCfgResOptions.h"

// CTargetOptionsDlg dialog

class CTargetOptionsDlg : public CPropertyPage
{
	DECLARE_DYNAMIC(CTargetOptionsDlg)

public:
	CTargetOptionsDlg();   // standard constructor
	virtual ~CTargetOptionsDlg();

// Dialog Data
	enum { IDD = IDD_TARGET_OPTIONS };

	virtual void OnOK();
	virtual void OnCancel();

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()


public:
	virtual	BOOL OnInitDialog();

	CButton m_AutoRecoveryBtn;
	CButton m_ForceRecvBtn;
	CButton m_FormatDataBtn;
	CButton m_EraseMediaBtn;
	CButton m_MinDlgBtn;
	CButton m_StdDlgBtn;
	CButton m_AdvDlgBtn;
	CButton m_UseLocalFilesBtn;
	CButton m_WinCEDownload;
	CComboBox m_DefaultFileSystem;
	USER_DLG_TYPE m_dlgType;
	CButton m_AutoStartBtn;
	CButton m_AutoCloseBtn;
	CVisValidEdit m_DriveLabel;

	afx_msg void OnBnClickedTargetOptionsFormatData();
	afx_msg void OnBnClickedTargetOptionsEraseMedia();
	afx_msg void OnBnClickedMinDlg();
	afx_msg void OnBnClickedStdDlg();
	afx_msg void OnBnClickedAdvDlg();
	afx_msg void OnEnChangeTargetOptionsLabel();
};
