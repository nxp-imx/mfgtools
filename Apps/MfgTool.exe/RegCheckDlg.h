#pragma once

#include "PlayerProfile.h"

#define ID_TESTTIMER			6000
#define REGCHK_PROG_RANGE		30
#define REGCHK_WARNING_LIMIT	600



#define WM_MSG_UPDATE_DATA WM_USER+1

// CRegCheckDlg dialog


class CRegCheckDlg : public CDialog
{
	DECLARE_DYNAMIC(CRegCheckDlg)

public:
	CRegCheckDlg(CConfigMgrSheet *_p_config_mgr, CWnd* pParent = NULL);   // standard constructor
	virtual ~CRegCheckDlg();

	virtual BOOL OnInitDialog();

// Dialog Data
	enum { IDD = IDD_REGCHECK_DLG };


protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	void Localize();
	DWORD CountUSBRegEntries();
	DWORD CountRecoveryRegEntries();
	DWORD CountUSBStorRegEntries();
	DWORD CountRemovableMediaRegEntries();
	void UpdateDlgStatus();

	CString GetDeviceRegistryProperty(DWORD Property);

	CConfigMgrSheet *m_pCfgMgr;

	UINT_PTR		m_DelayTimer;
	CProgressCtrl	m_ProgressCtrl;
	USHORT			m_prog_pos;

	BOOL			m_FirstPass;

	UINT			m_DeviceCount;
	UINT			m_OtherDeviceCount;
	CString			m_SCSIMfgStr;
	CString			m_SCSIProductStr;
	CString			m_SCSIVenderIDStr;
	CString			m_SCSIProductIDStr;	
	CString			m_HWDevInstStr;
	CString			m_USBStorageStr;

	HDEVINFO		m_HWUSBDevInfo;
	SP_DEVINFO_DATA m_HWUSBDevInfoData;
	HDEVINFO		m_HWHIDDevInfo;

public:



	DECLARE_MESSAGE_MAP()
	afx_msg void OnTimer(UINT_PTR nIDEvent);

};
