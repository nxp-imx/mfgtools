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

	const UCL::DeviceDesc::DeviceMode GetDeviceMode();

	UpdateTransportProtocol* m_pUTP;
	UCL m_UclNode;
	UCL::CommandList* m_pCmdList;
	UCL::DeviceDesc::DeviceMode m_CurrentDeviceMode;
	DeviceClass::DeviceType m_CurrentDeviceType;
	std::map<UCL::DeviceDesc::DeviceMode, UCL::DeviceDesc*> m_DeviceDescs;
    HANDLE m_hChangeEvent;
	BOOL m_bProcessingList;

	CWinThread* m_p_do_list_thread;
	friend UINT DoListThreadProc(LPVOID pParam);
	
	DWORD DoCommand(UCL::Command* pCmd);
	DWORD DoDrop(UCL::Command* pCmd);
	DWORD DoFind(UCL::Command* pCmd);
	DWORD DoBoot(UCL::Command* pCmd);
	DWORD DoResetToRecovery();
	DWORD DoBurn(UCL::Command* pCmd);
	DWORD DoLoad(CString filename);
	DWORD DoMxRomLoad(CString filename);
	DWORD DoShow(UCL::Command* pCmd);

};