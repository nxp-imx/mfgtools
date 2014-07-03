/*
 * Copyright (C) 2009-2013, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
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





class CCmdOpreation //: public CWinThread
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

	pthread_t * m_pThread;//////////////////////
	int m_WndIndex;
	myevent m_hKillEvent;
	myevent m_hDeviceArriveEvent;
	myevent m_hDeviceRemoveEvent;
	myevent m_hThreadStartEvent;
	myevent m_hRunEvent;
	myevent m_hStopEvent;
	myevent m_hOneCmdCompleteEvent;
	myevent RunFlag;
	sem_t *ev_semaphore;
	BOOL m_bKilled;
	BOOL m_bRun;
	BOOL m_bDeviceOn;
	CString m_usb_hub_name;
	DWORD m_usb_port_index;
	usb::Port * m_p_usb_port;
	UpdateTransportProtocol* m_pUTP;
	HANDLE_CALLBACK m_hDeviceChangeCallback;

	myevent m_hDevCanDeleteEvent;
    OPERATE_RESULT m_uiInfo;
	MX_DEVICE_STATE m_currentState;
	DWORD m_dwCmdIndex;
	INSTANCE_HANDLE m_pLibHandle;

	virtual int ExitInstance();
public:
	pthread_mutex_t m_hMutex_cb;
	pthread_mutex_t m_hMutex_cb2;
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

