#pragma once
#include "afxwin.h"


// AddFilterDlg dialog

class AddFilterDlg : public CDialog
{
	DECLARE_DYNAMIC(AddFilterDlg)

public:
	AddFilterDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~AddFilterDlg();

// Dialog Data
	enum { IDD = IDD_ADD_FILTER_DIALOG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	CString m_usb_vid;
	CString m_usb_pid;
	afx_msg void OnBnClickedAdd();
	virtual BOOL OnInitDialog();
};
