/*
 * Copyright (C) 2010, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */
#pragma once
#include "operation.h"
#include "../../Libs/OtpAccessPitc/StOtpAccessPitc.h"

class COpOTP : public COperation //, public CStProgress
{
	DECLARE_DYNCREATE(COpOTP)
public:
	COpOTP(CPortMgrDlg *pPortMgrDlg = NULL, usb::Port *pUSBPort = NULL, COpInfo* pOpInfo = NULL);
	~COpOTP(void);

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
    void OnDownloadProgress(const Device::NotifyStruct& nsInfo);
	DWORD UpdateSettingsField(void);
	DWORD ReadCustomerOtpField(StOtpAccessPitc * const pOtpPitc, const uint8_t _fieldNum, uint8_t& _fieldValue);
	DWORD WriteCustomerOtpField(StOtpAccessPitc * const pOtpPitc, const uint8_t _fieldNum, uint8_t _fieldValue);
	void HandleError(DWORD _error, LPCTSTR _errorMsg, OpState_t _nextState, LPCTSTR _logStr = NULL);
	CString GetOpStateString(OpState_t _state);
};
