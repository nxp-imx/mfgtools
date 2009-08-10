// OpUpdater.h : header file
//

#pragma once

#include "Operation.h"

class COpLoader : public COperation //, public CStProgress
{
	DECLARE_DYNCREATE(COpLoader)
public:
	COpLoader(CPortMgrDlg *pPortMgrDlg = NULL, usb::Port *pUSBPort = NULL, COpInfo* pOpInfo = NULL);
	virtual ~COpLoader(void);
	virtual afx_msg void OnMsgStateChange(WPARAM nEventType, LPARAM dwData);

protected:
	DECLARE_MESSAGE_MAP()

	enum OpState_t{OP_INVALID = 0, WAITING_FOR_DEVICE, WAITING_FOR_HID_MODE, OP_LOADING, WAITING_FOR_MTP_MODE, OP_COMPLETE};
	OpState_t m_OpState;

	BOOL InitInstance(void);
	CString GetProjectVersion(void);
   	CString GetOpStateString(OpState_t _state) const;
	CString MakeLogString() const;

	DWORD ResetToRecovery();
	DWORD LoadHidDevice();
	DWORD LoadRecoveryDevice();
	void HandleError(DWORD _error, LPCTSTR _errorMsg, OpState_t _nextState, LPCTSTR _logStr = NULL);
	void OnDownloadProgress(const Device::NotifyStruct& nsInfo);

	CString m_sVersion;
	DWORD	m_dwStartTime;

};