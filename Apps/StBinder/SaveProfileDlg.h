#pragma once
#include "afxwin.h"


// CSaveProfileDlg dialog

class CSaveProfileDlg : public CDialog
{
	DECLARE_DYNAMIC(CSaveProfileDlg)

public:
	CSaveProfileDlg(CStBinderDlg *_binderDlg, CWnd* pParent = NULL);   // standard constructor
	virtual ~CSaveProfileDlg();

	virtual BOOL OnInitDialog();

// Dialog Data
	enum { IDD = IDD_CFG_SAVE_PROFILE };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	CStBinderDlg *m_pBinderDlg;
	CEdit m_profileName;
	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedCancel();
};
