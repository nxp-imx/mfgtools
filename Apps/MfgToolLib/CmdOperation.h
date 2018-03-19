/*
 * Copyright 2009-2013, Freescale Semiconductor, Inc. All Rights Reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 *
 * Redistributions in binary form must reproduce the above copyright notice, this
 * list of conditions and the following disclaimer in the documentation and/or
 * other materials provided with the distribution.
 *
 * Neither the name of the Freescale Semiconductor nor the names of its
 * contributors may be used to endorse or promote products derived from this
 * software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 */

#pragma once

#include "MfgToolLib_Export.h"
#include "CallbackDef.h"
#include "DeviceClass.h"
#include "UsbPort.h"
#include "UpdateTransportProtocol.h"
#include "UpdateUIInfo.h"


#define MAX_CNT_KEYWORD         10

class CCmdOpreation : public CWinThread
{
public:
	//DECLARE_DYNCREATE(CCmdOpreation)
	CCmdOpreation(INSTANCE_HANDLE handle, int WndIndex = 0);
	virtual ~CCmdOpreation();

	typedef struct _KeyWord
    {
        CString KeyWordName;
        DWORD RangeBegin;
        DWORD RangeEnd;//End of the range, please notice the value in end border is not contained. Say [0, 100), means 0 ~99 is valid.
        DWORD CurrentValue;
        DWORD NextValue;
    }KeyWord, *PKeyWord;

	typedef struct _UniqueID
    {
        CString Section;
        KeyWord keyWord[MAX_CNT_KEYWORD];
    }UniqueID, *PUniqueID;  

    UniqueID m_UniqueID;

	DWORD Open();
	void Close();
	virtual BOOL InitInstance();
	BOOL OnStart();
	BOOL OnStop();
	BOOL OnDeviceArrive();
	BOOL OnDeviceRemove();

	usb::Port* FindPort();
	void SetUsbPort(usb::Port* _port);

	MX_DEVICE_STATE GetDeviceState();

	CWinThread* m_pThread;;
	int m_WndIndex;
	HANDLE m_hKillEvent;
	HANDLE m_hDeviceArriveEvent;
	HANDLE m_hDeviceRemoveEvent;
	HANDLE m_hThreadStartEvent;
	HANDLE m_hRunEvent;
	HANDLE m_hStopEvent;
	HANDLE m_hOneCmdCompleteEvent;
	BOOL m_bKilled;
	BOOL m_bRun;
	BOOL m_bDeviceOn;
	CString m_usb_hub_name;
	DWORD m_usb_port_index;
	usb::Port * m_p_usb_port;
	UpdateTransportProtocol* m_pUTP;
	StApi *m_pStApi;

	HANDLE_CALLBACK m_hDeviceChangeCallback;

	HANDLE m_hDevCanDeleteEvent;
    OPERATE_RESULT m_uiInfo;
	MX_DEVICE_STATE m_currentState;
	DWORD m_dwCmdIndex;
	INSTANCE_HANDLE m_pLibHandle;

	virtual int ExitInstance();
public:
	HANDLE m_hMutex_cb;
	HANDLE m_hMutex_cb2;
	std::map<int, DeviceChangeCallbackStruct*> m_callbacks;
	std::map<int, OperateResultUpdateStruct*> m_callbacks2;
	void mRegisterUIDevChangeCallback(DeviceChangeCallbackStruct *pCB);
	void mUnregisterUIDevChangeCallback();
	void mRegisterUIInfoUpdateCallback(OperateResultUpdateStruct *pCB);
	void mUnregisterUIInfoUpdateCallback();
	void OnDeviceChangeNotify(DeviceClass::NotifyStruct *pnsinfo);
	DEVICE_CHANGE_NOTIFY m_ni;
	void ExecuteUIUpdate(UI_UPDATE_INFORMATION *pInfo);
	DWORD WaitforEvents(DWORD dwTimeout);
	BOOL CanRun(void);
	DWORD UpdateUI(UI_UPDATE_INFORMATION* _uiInfo,DWORD dwStateIndex);
};

