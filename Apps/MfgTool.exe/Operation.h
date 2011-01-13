/*
 * Copyright (C) 2009-2011, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */
// Operation.h : header file
//

#pragma once

#include "../../Libs/DevSupport/UsbPort.h"
#include "../../Libs/DevSupport/Device.h"
#include "stmsg.h"

class CPlayerProfile;
class COpInfo;
class CPortMgrDlg;

#define SIGMATEL_VID _T("VID_066F")
#define SIGMATEL_MFG_MSC_PID _T("PID_0000")
#define SIGMATEL_UPDATER_MSC_PID _T("PID_A000")
#define SIGMATEL_37XX_UPDATER_PID _T("PID_37FF")


#define LOADER_OP_ENABLED

class COperation : public CWinThread
{
public:
	DECLARE_DYNCREATE(COperation)
	// if you change the OpTypes enum you MUST change the OperationStrings[] 
	// definitions at the top of the cpp file
	typedef enum OpTypes {INVALID_OP = -1,
							UPDATE_OP,
							COPY_OP,
							LOADER_OP,
							OTP_OP,
							UTP_UPDATE_OP,
							MX_UPDATE_OP,
							NO_MORE_OPS, // move after loader_op to enable loader op
							ERASE_OP,
							REGISTRY_OP,
							MONITOR_OP};
	typedef enum OpEvents { OPEVENT_START, OPEVENT_STOP, OPEVENT_DEVICE_ARRIVAL, OPEVENT_DEVICE_REMOVAL, OPEVENT_VOLUME_ARRIVAL, OPEVENT_VOLUME_REMOVAL, OPEVENT_WKR_THREAD_COMPLETE, OPEVENT_TIMEOUT, OPEVENT_KILL }; 
	typedef enum OpStatus { OPSTATUS_SUCCESS = 0, OPSTATUS_ERROR, OPSTATUS_ABORTED }; 
//	typedef enum DeviceStates { DISCONNECTED, CONNECTED_UNKNOWN, HID_MODE, RECOVERY_MODE, USBMSC_MODE, USBMSC_PLUS_DRIVE_MODE, RESET_DEVICE_MODE, RESET_WAIT_MODE };
	typedef enum DeviceState_t { DISCONNECTED, CONNECTED_UNKNOWN, HID_MODE, RECOVERY_MODE, UPDATER_MODE, MFG_MSC_MODE, MSC_MODE, MTP_MODE }; //, WAITING_FOR_HID_MODE, WAITING_FOR_RECOVERY_MODE, WAITING_FOR_UPDATER_MODE, WAITING_FOR_MFG_MSC_MODE, WAITING_FOR_MSC_MODE, WAITING_FOR_MTP_MODE };
	static const PTCHAR OperationStrings[];
	COperation(CPortMgrDlg *pPortMgrDlg = NULL, usb::Port *pUsbPort = NULL, COpInfo *pOpInfo = NULL, OpTypes type = INVALID_OP);
	virtual ~COperation(void);
	virtual BOOL InitInstance(){return true;}; // pure virtual; must implement in dervived class
	CString GetDescription(){return m_sDescription;};
	CString GetDeviceStateString(DeviceState_t _state) const;
	CString COperation::GetEventString(WPARAM _event) const;
	VOID CALLBACK TimerProc(HWND hWnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime);

protected:
	CString GetDeviceSerialNumber(Device* pDevice) const;
	usb::Port *m_pUSBPort;
	COpInfo *m_pOpInfo;
	CPlayerProfile *m_pProfile;
//	eDeviceState m_DeviceState;
	int m_iDuration;
	int m_iPercentComplete;
//	INT_PTR m_part;
	bool m_bStart;
	UINT_PTR m_Timer;
	UINT m_TimeOutSeconds;
	OpTypes m_OpType;
//	CString m_sStatus;
//	CString m_sDrive;
//	CString m_sDevicePath;
	CString m_sDescription;
	DECLARE_MESSAGE_MAP()
	virtual afx_msg void OnMsgStateChange(WPARAM nEventType, LPARAM dwData) {}; // pure virtual; must implement in dervived class
	inline int ProgressDelta(int newProgress) {int delta = newProgress - m_iPercentComplete; m_iPercentComplete = newProgress; return delta;};

public:
	CPortMgrDlg *m_pPortMgrDlg;
	const OpTypes GetOpType() const { return m_OpType; };
	const DeviceState_t GetDeviceState() const;
//	CString GetStatus(void){return m_sStatus;};
	INT_PTR GetDuration(void){return m_iDuration;};
	INT_PTR GetPercentComplete(void){return m_iPercentComplete;};
};
