/*
 * Copyright (C) 2010, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */
#pragma once

#include "PlayerProfile.h"

// CRegScrubDlg dialog

class CRegScrubDlg : public CDialog
{
	DECLARE_DYNAMIC(CRegScrubDlg)

public:
	CRegScrubDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CRegScrubDlg();
	virtual BOOL OnInitDialog();

// Dialog Data
	enum { IDD = IDD_REGSCRUB_DLG };

protected:	
	CComboBox m_ProfileComboBox;
	
	BOOL m_RemoveStaticIDEntryCkCtrl;
	BOOL m_RemoveRecoveryEntries;

	CString m_ProfileNameStr;
	CString m_ConfigMgrProfileNameStr;

	CString m_SCSIMfgStr;
	CString m_SCSIProductStr;
	CString m_SCSIVenderIDStr;
	CString m_SCSIProductIDStr;	
	
	CString m_DeletingKeyStr;
	CString m_ProcessingKeyStr;

	CString m_HWDevInstStr;
	CString m_USBStorageStr;
	
	bool m_ValidProfile;
	UINT m_DeviceCount;

	HDEVINFO		m_HWUSBDevInfo;
	SP_DEVINFO_DATA m_HWUSBDevInfoData;
	HDEVINFO		m_HWHIDDevInfo;

	BOOL IsPlatformNT();
	DWORD RemoveUSBRegEntries();
	DWORD RemoveUSBStorRegEntries();
	DWORD RemoveRemovableMediaRegEntries();
    DWORD RemoveHIDRegEntries();

	BOOL UpdateData(BOOL bSaveAndValidate=TRUE);
	CString GetDeviceRegistryProperty(DWORD Property);
	void Localize();
	
	void InitProfileComboBox(void);
	afx_msg void OnBnClickedRemoveStaticIds();
	afx_msg void OnCbnSelchangeProductDescCombo(void);
	
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	// Generated message map functions
	//{{AFX_MSG(CRegScrubDlg)
	afx_msg void OnScrub();
	virtual void OnCancel();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedRemoveRecoveryEntries();
};
