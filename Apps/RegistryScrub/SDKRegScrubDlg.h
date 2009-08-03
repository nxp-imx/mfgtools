// SDKRegScrubDlg.h : header file
// Copyright (c) 2005 SigmaTel, Inc.

#pragma once

#include "RegScrub.h"
#include "afxwin.h"

// CSDKRegScrubDlg dialog
class CSDKRegScrubDlg : public CDialog
{
// Construction
public:
	CSDKRegScrubDlg(CWnd* pParent = NULL);	// standard constructor
	virtual ~CSDKRegScrubDlg();

// Dialog Data
	enum { IDD = IDD_SDKREGSCRUB_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support

// Implementation
protected:
	HICON m_hIcon;

    bool m_Scrubbing;

    UINT m_LineCount;
	UINT m_EntryRemovalCount;

    CString m_RegData;
   //CEdit m_RegData;
    CEdit m_RegEditCtrl;
    
	CRegScrubList* m_pOtherList;
	CRegScrub* m_pRegScrub;

    CString m_DeletingKeyStr;
	CString m_ProcessingKeyStr;
    
//	CString m_SCSIMfgStr;
//	CString m_SCSIProductStr;
//	CString m_SCSIVenderIDStr;
//	CString m_SCSIProductIDStr;	

//    CString m_SCSIUpdaterMfgStr;
//	CString m_SCSIUpdaterProductStr;
//  CString m_SCSIUpdaterVenderIDStr;
//	CString m_SCSIUpdaterProductIDStr;		

//    CString m_SCSIRecoveryProductIDStr;

   	afx_msg LRESULT OnMsgUpdateUI(WPARAM _event_type, LPARAM _msg_string);

	// Generated message map functions
	afx_msg void OnBnClickedScrub();
	afx_msg void OnBnClickedCancel();
	
    virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()

	BOOL GetParameters ();
	BOOL SaveParameters ();
public:
	afx_msg void OnBnClickedAddButton();
	afx_msg void OnBnClickedOtherCheck();
	afx_msg void OnBnClickedOtherRemove();
};
