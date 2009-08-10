#pragma once

#include "resource.h"		// main symbols

// CLogMgrDlg dialog

class CLogMgrDlg : public CDialog
{

public:
	CLogMgrDlg();   // standard constructor
	virtual ~CLogMgrDlg();

	BOOL Create();

// Dialog Data
	enum { IDD = IDD_LOGMGR_DIALOG };

	void LogIt(CString Data);
    virtual BOOL OnInitDialog();

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	CString m_sLogDataStr;
	CWnd* m_pParent; 
	
	// Generated message map functions 
	//{{AFX_MSG(CLogMgrDlg) 
	virtual void OnCancel(); 
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
public:
};
