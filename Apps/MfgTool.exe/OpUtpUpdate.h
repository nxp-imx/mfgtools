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
#include "UpdateCommandList.h"
#include "../../Libs/DevSupport/Volume.h"
#include "../../Libs/DevSupport/UpdateTransportProtocol.h"
#include "../../Libs/DevSupport/DeviceClass.h"

#define OP_UPDATE_INCOMPLETE	-1L

class COpUtpUpdate : public COperation //, public CStProgress
{
	DECLARE_DYNCREATE(COpUtpUpdate)
public:
	COpUtpUpdate(CPortMgrDlg *pPortMgrDlg = NULL, usb::Port *pUSBPort = NULL, COpInfo* pOpInfo = NULL);
	virtual ~COpUtpUpdate(void);

	virtual afx_msg void OnMsgStateChange(WPARAM nEventType, LPARAM dwData);
	
	static void SetTimedOut(void);
protected:
	DECLARE_MESSAGE_MAP()

	enum HAB_type
	{
		HabUnknown	= -1,
		HabDisable  = 0,
		HabEnable   = 1  
	};
	enum HAB_status
	{
        HabUnknownStatus = -1,
        HabChecked = 0,
        NoHabCheck = 1,
        HabDismatch = 2,
        HabMatch =3
	};
	enum OpState_t{OP_INVALID = 0, WAITING_FOR_DEVICE, /*WAITING_FOR_HID_MODE, */WAITING_FOR_RECOVERY_MODE, OP_RECOVERING, WAITING_FOR_UPDATER_MODE, WAITING_FOR_MFG_MSC_MODE, WAITING_FOR_MSC_MODE, WAITING_FOR_MTP_MODE, OP_FLASHING, OP_COMPLETE, WAITING_FOR_SECOND_UPDATER_MODE};
	OpState_t m_OpState;

	CPlayerProfile *m_pProfile;
	
	BOOL InitInstance(void);
//	DWORD ResetToRecovery();
//	DWORD RecoverDevice();
//	DWORD InitOtpRegs();
//	DWORD RecoverHidDevice();
//	DWORD UpdateMscDevice(CString& _logText);
//	DWORD CompleteUpdate();
//	DWORD EraseLogicalMedia(Volume* pMscDevice, HANDLE hVolume, Device::NotifyStruct& nsInfo);
	CString GetProjectVersion(void);
	CString GetOpStateString(OpState_t _state);
    void OnDownloadProgress(const Device::NotifyStruct& nsInfo);
	void HandleError(DWORD _error, LPCTSTR _errorMsg, OpState_t _nextState, LPCTSTR _logStr = NULL);
//	DWORD PerformMscDeviceUpdate(CString& _logText); // handles both OP_EVENT_START and OP_EVENT_VOL_ARRIVAL
//	DWORD COpUtpUpdate::VerifyNand(Volume* pMscDevice);
	CString m_sVersion;
	DWORD	m_dwStartTime;
	BOOL WaitForDeviceChange(int seconds);
	DWORD CheckHabType(CString strHabType);

	const UCL::DeviceState::DeviceState_t GetDeviceState();

	UpdateTransportProtocol* m_pUTP;
	UCL m_UclNode;
	UCL::CommandList* m_pCmdList;
	UCL::DeviceState::DeviceState_t m_CurrentDeviceState;
	std::map<UCL::DeviceState::DeviceState_t, UCL::DeviceDesc*> m_DeviceStates;
    HANDLE m_hChangeEvent;
	BOOL m_bProcessingList;
	HAB_status m_habStatus;

	CWinThread* m_p_do_list_thread;
	friend UINT DoListThreadProc(LPVOID pParam);
	
	DWORD DoCommand(UCL::Command* pCmd);
	DWORD DoDrop(UCL::Command* pCmd);
	DWORD DoFind(UCL::Command* pCmd);
	DWORD DoBoot(UCL::Command* pCmd);
	DWORD DoResetToRecovery();
	DWORD DoBurn(UCL::Command* pCmd);
	DWORD DoHidLoad(UCL::Command* pCmd);//for ST hid device    
	DWORD DoMxHidLoad(UCL::Command* pCmd);//for MX hid device,such as mx508
	DWORD DoInit(UCL::Command* pCmd);
	DWORD DoMxRomLoad(UCL::Command* pCmd);
	DWORD DoShow(UCL::Command* pCmd);
	DWORD DoPlugin(UCL::Command* pCmd);

};
