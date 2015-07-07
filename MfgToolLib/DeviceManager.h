/*
 * Copyright (C) 2009-2013, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */

#pragma once

#include <map>
#include <list>
#include "DeviceClass.h"
#include "MfgToolLib.h"


#define WM_MSG_DEV_EVENT	(WM_USER+36)


int DevChange_callback(struct libusb_context *ctx, struct libusb_device *dev, libusb_hotplug_event event, void *user_data);

class CMyExceptionHandler;

class DeviceManager//:CWinThread
{

	public:
		DeviceManager(INSTANCE_HANDLE handle=NULL);
		~DeviceManager();

		typedef enum DevChangeEvent
		{
			UNKNOWN_EVT = 100,
			EVENT_KILL=-1,
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
#ifdef __linux__

#else
		class DevChangeWnd
		{
			pthread_t	m_wnd;
			HINSTANCE  hModule;          //Injected Modules Handle
			public:
			void create();
			void DeviceChangeProc();
			BOOL OnDeviceChange(UINT nEventType,DWORD_PTR dwData);
			CString DrivesFromMask(ULONG UnitMask);
			LRESULT CALLBACK DLLWindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
			void destroy();
			static void MessageProc();

		};
		DevChangeWnd _DevChangeWnd;
#endif
		void OnMsgDeviceEvent(WPARAM eventType, LPARAM desc);
		pthread_t m_hThread;
		sem_t msgs;
		std::queue<thread_msg> DevMgrMsgs;
		// Used by ~DeviceManager() to tell if DeviceManager::ExitInstance() was called.
		BOOL _bStopped;
		// Used by Open() to syncronize DeviceManager thread start.
		myevent * _hStartEvent;
		myevent * _hKillEvent;
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

		pthread_mutex_t * m_hMutex_cb;
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
