/*
 * Copyright (C) 2010, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */
// OpUpdater.h : header file
//

#pragma once

#include "Operation.h"
#include "../../Libs/DevSupport/Volume.h"

#define OP_UPDATE_INCOMPLETE	-1L

class COpUpdater : public COperation //, public CStProgress
{
	DECLARE_DYNCREATE(COpUpdater)
public:
	COpUpdater(CPortMgrDlg *pPortMgrDlg = NULL, usb::Port *pUSBPort = NULL, COpInfo* pOpInfo = NULL);
	virtual ~COpUpdater(void);

	virtual afx_msg void OnMsgStateChange(WPARAM nEventType, LPARAM dwData);
	
	static void SetTimedOut(void);
protected:
	DECLARE_MESSAGE_MAP()

	enum OpState_t{OP_INVALID = 0, WAITING_FOR_DEVICE, /*WAITING_FOR_HID_MODE, */WAITING_FOR_RECOVERY_MODE, OP_RECOVERING, WAITING_FOR_UPDATER_MODE, WAITING_FOR_MFG_MSC_MODE, WAITING_FOR_MSC_MODE, WAITING_FOR_MTP_MODE, OP_FLASHING, OP_COMPLETE, WAITING_FOR_SECOND_UPDATER_MODE};
	OpState_t m_OpState;

	CPlayerProfile *m_pProfile;
	

BOOL m_bOwnMutex;
BOOL GetResetLockMutex(BOOL _bOpComplete);
BOOL ReleaseResetLockMutex(void);

	BOOL InitInstance(void);
	DWORD ResetToRecovery();
	DWORD RecoverDevice();
	DWORD InitOtpRegs();
	DWORD RecoverHidDevice();
	DWORD UpdateMscDevice(CString& _logText);
	DWORD CompleteUpdate();
	DWORD EraseLogicalMedia(Volume* pMscDevice, HANDLE hVolume, Device::NotifyStruct& nsInfo);
	CString GetProjectVersion(void);
	CString GetOpStateString(OpState_t _state);
    void OnDownloadProgress(const Device::NotifyStruct& nsInfo);
	void HandleError(DWORD _error, LPCTSTR _errorMsg, OpState_t _nextState, LPCTSTR _logStr = NULL);
	DWORD PerformMscDeviceUpdate(CString& _logText); // handles both OP_EVENT_START and OP_EVENT_VOL_ARRIVAL
	DWORD COpUpdater::VerifyNand(Volume* pMscDevice);
	CString m_sVersion;
	DWORD	m_dwStartTime;

};
