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

#include <map>
#include <list>
#include "DeviceClass.h"
#include "MfgToolLib.h"


#define WM_MSG_DEV_EVENT	(WM_USER+36)

class CMyExceptionHandler;

class DeviceManager : public CWinThread
{
	DECLARE_DYNCREATE(DeviceManager)
	
public:
	DeviceManager(INSTANCE_HANDLE handle=NULL);
	~DeviceManager();
	
	enum DevChangeEvent 
    {
		UNKNOWN_EVT = 100, 
		EVENT_KILL,
		HUB_ARRIVAL_EVT, 
		HUB_REMOVAL_EVT,
		DEVICE_ARRIVAL_EVT, 
		DEVICE_REMOVAL_EVT, 
		VOLUME_ARRIVAL_EVT, 
		VOLUME_REMOVAL_EVT,
    };

	DWORD Open();
	void Close();
	virtual BOOL InitInstance();
	virtual int ExitInstance();
	
	class DevChangeWnd : public CWnd
    {
    public:
        afx_msg BOOL OnDeviceChange(UINT nEventType,DWORD_PTR dwData);
		CString DrivesFromMask(ULONG UnitMask);
        DECLARE_MESSAGE_MAP()
    };
	DevChangeWnd _DevChangeWnd;
	
	virtual afx_msg void OnMsgDeviceEvent(WPARAM eventType, LPARAM desc);
	DECLARE_MESSAGE_MAP()
	
	// Used by ~DeviceManager() to tell if DeviceManager::ExitInstance() was called.
	BOOL _bStopped;
	// Used by Open() to syncronize DeviceManager thread start.
	HANDLE _hStartEvent;
	// Message Support
	HDEVNOTIFY _hUsbDev;
	HDEVNOTIFY _hUsbHub;
	DeviceClass::NotifyStruct m_nsInfo;

public:
	BOOL m_bSelfThreadRunning;
	void SetSelfThreadRunStatus(BOOL bRunning);
	typedef struct
	{
		PINTERNALCALLBACK pfunc;  // the pointer to point to callback function
		CString Hub;
		DWORD HubIndex;
		int cmdOpIndex;
	}CBStruct;

	BOOL m_bHasConnected[MAX_BOARD_NUMBERS];

	HANDLE m_hMutex_cb;
	std::map<HANDLE_CALLBACK, CBStruct*> m_callbacks;
	HANDLE_CALLBACK RegisterCallback(CBStruct *pCB);
	void UnregisterCallback(HANDLE_CALLBACK hCallback);
	void Notify(DeviceClass::NotifyStruct* pnsInfo);
	void Notify(DeviceClass::NotifyStruct* pnsInfo, int index);

	CMyExceptionHandler *m_pExpectionHandler;
	std::map<CString, int> mapMsg;

	INSTANCE_HANDLE m_pLibHandle;
};

/*------------------------------
external global variables
-------------------------------*/
extern DEV_CLASS_ARRAY g_devClasses;
extern DeviceManager* g_pDeviceManager;


