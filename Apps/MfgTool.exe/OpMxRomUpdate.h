// OpUpdater.h : header file
//

#pragma once

#include "Operation.h"
#include "UpdateCommandList.h"

#define OP_UPDATE_INCOMPLETE	-1L

class COpMxRomUpdate : public COperation //, public CStProgress
{
	DECLARE_DYNCREATE(COpMxRomUpdate)
public:
	COpMxRomUpdate(CPortMgrDlg *pPortMgrDlg = NULL, usb::Port *pUSBPort = NULL, COpInfo* pOpInfo = NULL);
	virtual ~COpMxRomUpdate(void);

	virtual afx_msg void OnMsgStateChange(WPARAM nEventType, LPARAM dwData);
	
	static void SetTimedOut(void);
protected:
	DECLARE_MESSAGE_MAP()

	enum OpState_t{OP_INVALID = 0, WAITING_FOR_DEVICE, WAITING_FOR_RECOVERY_MODE, OP_RECOVERING, WAITING_FOR_RAM_KERNEL_MODE, OP_FLASHING, OP_COMPLETE};
	OpState_t m_OpState;

	CPlayerProfile *m_pProfile;
	
	BOOL InitInstance(void);
	DWORD RecoverDevice();
	CString GetProjectVersion(void);
	CString GetOpStateString(OpState_t _state);
    void OnDownloadProgress(const Device::NotifyStruct& nsInfo);
	void HandleError(DWORD _error, LPCTSTR _errorMsg, OpState_t _nextState, LPCTSTR _logStr = NULL);
	CString m_sVersion;
	DWORD	m_dwStartTime;
	BOOL WaitForDeviceChange(int seconds);

	const UCL::DeviceState::DeviceState_t GetDeviceState();

	UCL m_UclNode;
	UCL::CommandList* m_pCmdList;
	UCL::DeviceState::DeviceState_t m_CurrentDeviceState;
	std::map<UCL::DeviceState::DeviceState_t, UCL::DeviceDesc*> m_DeviceStates;
    HANDLE m_hChangeEvent;
	BOOL m_bProcessingList;

	CWinThread* m_p_do_list_thread;
	friend UINT DoMxListThreadProc(LPVOID pParam);
	
	DWORD DoCommand(UCL::Command* pCmd);
	DWORD DoDrop(UCL::Command* pCmd);
	DWORD DoFind(UCL::Command* pCmd);
	DWORD DoBoot(UCL::Command* pCmd);
//	DWORD DoBurn(UCL::Command* pCmd);
	DWORD DoLoad(UCL::Command* pCmd);
//	DWORD DoMxRomLoad(CString filename, unsigned int RAMKNLAddr, bool bPreload);
	DWORD DoShow(UCL::Command* pCmd);

};