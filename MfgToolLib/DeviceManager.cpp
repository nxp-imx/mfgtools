/*
 * Copyright (C) 2009-2014, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */

#include "stdafx.h"
#include "DeviceManager.h"
#include "ControllerClass.h"
#include "HubClass.h"
#include "MxRomDeviceClass.h"
#include "HidDeviceClass.h"
#include "MxHidDeviceClass.h"
#include "DiskDeviceClass.h"
#include "VolumeDeviceClass.h"

#include "MfgToolLib_Export.h"
#include "MfgToolLib.h"
#include "ExceptionHandle.h"

//#include <Dbt.h>

DeviceManager* g_pDeviceManager;
DEV_CLASS_ARRAY g_devClasses;

//only dor debug
//LARGE_INTEGER g_t1, g_t2, g_t3, g_tc;

//////////////////////////////////////////////////////////////////////
//
// DeviceManager class implementation
//
//////////////////////////////////////////////////////////////////////
//IMPLEMENT_DYNCREATE(DeviceManager, CWinThread)
pthread_t usbThread;
DeviceManager::DeviceManager(INSTANCE_HANDLE handle)
: _hUsbDev(NULL)
, _hUsbHub(NULL)
, _bStopped(TRUE)
, _hStartEvent(NULL)
{	
	m_pLibHandle = handle;
	m_bSelfThreadRunning = FALSE;
	//m_bAutoDelete = FALSE; //Don't destroy the object at thread termination
	m_hMutex_cb=new pthread_mutex_t;
	if(m_hMutex_cb == NULL)
	{
		LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_FATAL_ERROR, _T("create DeviceManager::m_hMutex_cb failed, errcode is %d"), GetLastError());
		throw 1;
	}
	pthread_mutex_init(m_hMutex_cb,NULL);// = ::CreateMutex(NULL, FALSE, NULL);
}

DeviceManager::~DeviceManager()
{
	// Client has to call Close() before it exits;
	ASSERT( _bStopped == TRUE );
//	::CloseHandle(m_hMutex_cb);
	pthread_mutex_destroy(m_hMutex_cb);
	delete m_hMutex_cb;
}

void* DevManagerThreadProc(void* pParam){
	DeviceManager* pDevManage = (DeviceManager*)pParam;
	pDevManage->InitInstance();// init the instance that would normally be called by CreateThread on a CWinThread object
	pDevManage->m_bSelfThreadRunning = true; // set the thread var to running
	printf("before start event \n");
    	SetEvent(pDevManage->_hStartEvent); // post the event so  Open() can complete
	printf("after start event\n");
	while (1){
	    sem_wait( &pDevManage->msgs);
	    if(pDevManage->DevMgrMsgs.front().message==-1){
		printf("Dev thread kill msg received\n");
		pDevManage->DevMgrMsgs.pop();
		pthread_exit(NULL);
	    }
	    else{
		thread_msg temp=pDevManage->DevMgrMsgs.front();
		pDevManage->OnMsgDeviceEvent(temp.message,temp.lParam);
		pDevManage->DevMgrMsgs.pop();
    	    }
	}

}

void * UsbProcEvents(void * pParam){
	printf("usb thread inside\n");
	while(1){
	    libusb_handle_events_completed(NULL, NULL);
	    sleep(1);
	}
    return NULL;
}

	

// Create DeviceManager thread
DWORD DeviceManager::Open(){
	//must make sure the DeviceManager thread  is not running
	if( !_bStopped )
	{
		return MFGLIB_ERROR_DEV_MANAGER_IS_RUNNING;
	}
	
	// Create an event that we can wait on so we will know when
    // the DeviceManager thread has completed InitInstance()
	InitEvent(&_hStartEvent);// = ::CreateEvent(NULL, FALSE, FALSE, NULL); //Auto reset, nosignal
	if( _hStartEvent == NULL )
	{
		LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_FATAL_ERROR, _T("DeviceManager::Open()--Create _hStartEvent failed"));
		return MFGLIB_ERROR_NO_MEMORY;
	}
	InitEvent(&_hKillEvent);// = ::CreateEvent(NULL, FALSE, FALSE, NULL); //Auto reset, nosignal
	if( _hKillEvent == NULL )
	{
		LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_FATAL_ERROR, _T("DeviceManager::Open()--Create _hKillEvent failed"));
		return MFGLIB_ERROR_NO_MEMORY;
	}
	// Create the user-interface thread supporting messaging

	DWORD dwErrCode = ERROR_SUCCESS;
	sem_init(&msgs,0,0);
	libusb_hotplug_callback_handle handle;
	libusb_init(NULL);
	int rc;
	rc = libusb_hotplug_register_callback(NULL,(libusb_hotplug_event)(LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED | LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT), (libusb_hotplug_flag)0,LIBUSB_HOTPLUG_MATCH_ANY,LIBUSB_HOTPLUG_MATCH_ANY,LIBUSB_HOTPLUG_MATCH_ANY,DevChange_callback, this,&handle);
	
	if (LIBUSB_SUCCESS != rc) {
	    printf("Error creating a hotplug callback\n");
	    libusb_exit(NULL);
	    return MFGLIB_ERROR_DEV_MANAGER_RUN_FAILED;
	}	
	
	rc = pthread_create(&usbThread, NULL, UsbProcEvents, this);
	if(rc!=0){
	    printf("usb Thread Fail\n");
	}
	int result = pthread_create(&m_hThread, NULL, DevManagerThreadProc, this);


	if (result==0) //create DeviceManager thread successfully
	{
		printf("created thread successfully\n");
		WaitOnEvent(_hStartEvent);
		// Set flag to running. Flag set to stopped in constructor and in Close()
		if(m_bSelfThreadRunning)
		{	

			_bStopped = FALSE;
		}
		else
		{
			DestroyEvent(_hStartEvent);
			_hStartEvent = NULL;
			return MFGLIB_ERROR_DEV_MANAGER_RUN_FAILED;
		}
	}
	else //create DeviceManager thread failed
	{   
		printf(" broke thread\n");
		dwErrCode = GetLastError();
		LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_FATAL_ERROR, _T("DeviceManager::Open()--Create DeviceManager thread failed(error: %d)"), dwErrCode);
		return MFGLIB_ERROR_THREAD_CREATE_FAILED;
	}
	
	printf("created the thread all ok\n");
	LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_NORMAL_MSG, _T("Device Manager thread is running"));
	
	// clean up
	DestroyEvent(_hStartEvent);
	_hStartEvent = NULL;

	return MFGLIB_ERROR_SUCCESS;
}

void DeviceManager::Close()
{
	// Client has to call DeviceManager::Open() before it can call DeviceManager::Close();
    if(_bStopped)
	{
		return;
	}

	// Post a KILL event to kill DeviceManager thread
	//PostThreadMessage(WM_MSG_DEV_EVENT, EVENT_KILL, 0);
	// Wait for the DeviceManager thread to die before returninga
	SetEvent(_hKillEvent);
	pthread_join(m_hThread,NULL);
	libusb_exit(NULL);
	LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_NORMAL_MSG, _T("Device Manager thread is closed"));

	_bStopped = TRUE;
	//m_hThread = NULL;
}

void DeviceManager::SetSelfThreadRunStatus(BOOL bRunning)
{
	m_bSelfThreadRunning = bRunning;
	SetEvent(_hStartEvent);
}
#ifndef __linux__
void DeviceManager::DevChangeWnd::DeviceChangeProc(){

	MSG messages;
	//wchar_t *pString = reinterpret_cast<wchar_t * > (lpParam);
	WNDCLASSEX wc;
	//wc.hInstance = inj_hModule;
	wc.lpszClassName = (LPCWSTR)L"InjectedDLLWindowClass";
	//wc.lpfnWndProc = DLLWindowProc;
	wc.style = CS_DBLCLKS;
	wc.cbSize = sizeof (WNDCLASSEX);
	wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wc.hIconSm = LoadIcon(NULL, IDI_APPLICATION);
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.lpszMenuName = NULL;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hbrBackground = (HBRUSH)COLOR_BACKGROUND;
	if (!RegisterClassEx(&wc))
		return ;
	//prnt_hWnd = FindWindow(L"Window Injected Into ClassName", L"Window Injected Into Caption");
	//HWND hwnd = CreateWindowEx(0, L"InjectedDLLWindowClass",_T("thisWindow"), WS_EX_PALETTEWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 400, 300, prnt_hWnd, hMenu, inj_hModule, NULL);
	//ShowWindow(hwnd, SW_SHOWNORMAL);
	while (GetMessage(&messages, NULL, 0, 0))
	{
		TranslateMessage(&messages);
		DispatchMessage(&messages);
	}
	return ;
}


#endif
//when CreateThread, this function will be executed
BOOL DeviceManager::InitInstance()
{
	//Reset all DeviceClasses
	g_devClasses[DeviceClass::DeviceTypeDisk] = NULL;
	g_devClasses[DeviceClass::DeviceTypeMsc] = NULL;
	g_devClasses[DeviceClass::DeviceTypeHid] = NULL;
	g_devClasses[DeviceClass::DeviceTypeMxHid] = NULL;
	g_devClasses[DeviceClass::DeviceTypeMxRom] = NULL;
	g_devClasses[DeviceClass::DeviceTypeUsbController] = NULL;
	g_devClasses[DeviceClass::DeviceTypeUsbHub] = NULL;

	// Create a hidden window and register it to receive WM_DEVICECHANGE messages
	/*

	if( _DevChangeWnd.CreateEx(WS_EX_TOPMOST, _T("STATIC"), _T("DeviceChangeWnd"), 0, CRect(0,0,5,5), NULL, 0) )
	{	//Create DevChangeWnd successfully
		DEV_BROADCAST_DEVICEINTERFACE broadcastInterface;
		
		broadcastInterface.dbcc_size = sizeof(DEV_BROADCAST_DEVICEINTERFACE);
        broadcastInterface.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
		
		memcpy( &(broadcastInterface.dbcc_classguid),&(GUID_DEVINTERFACE_USB_DEVICE),sizeof(GUID) );
		// register for usb devices
		_hUsbDev = RegisterDeviceNotification(_DevChangeWnd.GetSafeHwnd(), &broadcastInterface, DEVICE_NOTIFY_WINDOW_HANDLE);
		
		memcpy( &(broadcastInterface.dbcc_classguid),&(GUID_DEVINTERFACE_USB_HUB),sizeof(GUID) );
		// register for usb hubs
		_hUsbHub=RegisterDeviceNotification(_DevChangeWnd.GetSafeHwnd(), &broadcastInterface, DEVICE_NOTIFY_WINDOW_HANDLE);
	}
	else //create window that received WM_DEVICECHANGE message failed
	{
		LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_FATAL_ERROR, _T(" *** FAILED TO CREATE WINDOW FOR WM_DEVCHANGE NOTIFICATIONS."));
		SetSelfThreadRunStatus(FALSE);
		return FALSE;
	}*/

	for(int i=0; i<MAX_BOARD_NUMBERS; i++)
	{
		m_bHasConnected[i] = FALSE;
	}
	printf("initInstance DevMgr\n");
	m_pExpectionHandler = new CMyExceptionHandler;
	if(NULL == m_pExpectionHandler)
	{
		LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_FATAL_ERROR, _T("create CExceptionHandler failed"));
		SetSelfThreadRunStatus(FALSE);
		return FALSE;
	}
	
	if(m_pExpectionHandler->Open() != MFGLIB_ERROR_SUCCESS)
	{
		LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_FATAL_ERROR, _T("open ExceptionHandler thread failed"));
		return FALSE;
	}
	
	printf("initInstance DevMgr after exception\n");
	//init all device classes
	DeviceClass *pDevClass = NULL;
	try
	{
		pDevClass = new usb::ControllerClass(m_pLibHandle);
	}
	catch(...)
	{
		SetSelfThreadRunStatus(FALSE);
		return FALSE;
	}
	if(pDevClass == NULL)
	{
		LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_FATAL_ERROR, _T(" Failed to create usb::ControllerClass"));
		SetSelfThreadRunStatus(FALSE);
		return FALSE;
	}
	g_devClasses[DeviceClass::DeviceTypeUsbController] = pDevClass;
	pDevClass->Devices();
	
	pDevClass = new usb::HubClass(m_pLibHandle);
	if(pDevClass == NULL)
	{
		LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_FATAL_ERROR, _T(" Failed to create usb::HubClass"));
		SetSelfThreadRunStatus(FALSE);
		return FALSE;
	}
	g_devClasses[DeviceClass::DeviceTypeUsbHub] = pDevClass;
	pDevClass->Devices();

	OP_STATE_ARRAY *pCurrentStates = GetOpStates((MFGLIB_VARS *)m_pLibHandle);
	if(pCurrentStates == NULL)
	{
		SetSelfThreadRunStatus(FALSE);
		return FALSE;
	}
	
	printf("initInstance DevMgr right befor state iterator\n");
	std::vector<COpState*>::iterator stateIt = pCurrentStates->begin();
	for(; stateIt!=pCurrentStates->end(); stateIt++)
	{
		switch((*stateIt)->opDeviceType)
		{
		case DEV_HID_MX28:
			pDevClass = new HidDeviceClass(m_pLibHandle);
			if (pDevClass == NULL)
			{
				LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_FATAL_ERROR, _T(" Failed to create HidDeviceClass"));
				SetSelfThreadRunStatus(FALSE);
				return FALSE;
			}
			g_devClasses[DeviceClass::DeviceTypeHid] = pDevClass;
			pDevClass->Devices();
			break;

/*		case DEV_MX23:		//Hid
		case DEV_MX28:
			break;
		case DEV_MX25:
		case DEV_MX35:
		case DEV_MX51:
		case DEV_MX53:		//MxRom
			pDevClass = new MxRomDeviceClass;
			if(pDevClass == NULL)
			{
				LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_FATAL_ERROR, _T(" Failed to create MxRomDeviceClass"));
				SetSelfThreadRunStatus(FALSE);
				return FALSE;
			}
			g_devClasses[DeviceClass::DeviceTypeMxRom] = pDevClass;
			pDevClass->Devices();
			break; */
		case DEV_HID_MX6Q:	//MxHid
		//case DEV_MX50:
		case DEV_HID_MX6D:
		case DEV_HID_MX6SL:
		case DEV_HID_MX6SX:
		//	printf("iterator MX6\n");
			pDevClass = new MxHidDeviceClass(m_pLibHandle);
			if(pDevClass == NULL)
			{
				LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_FATAL_ERROR, _T(" Failed to create MxHidDeviceClass"));
				SetSelfThreadRunStatus(FALSE);
				return FALSE;
			}
			g_devClasses[DeviceClass::DeviceTypeMxHid] = pDevClass;
			pDevClass->Devices();
			break;
		case DEV_MSC_UPDATER:	//VolumeDeviceClass and DiskDeviceClass
			printf("msc update iterate\n");
	    		pDevClass = new DiskDeviceClass(m_pLibHandle);
			if(pDevClass == NULL)
			{
				LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_FATAL_ERROR, _T(" Failed to create DiskDeviceClass"));
				SetSelfThreadRunStatus(FALSE);
				return FALSE;
			}
			printf(" after creation\n");
			g_devClasses[DeviceClass::DeviceTypeDisk] = pDevClass;
			(dynamic_cast<DiskDeviceClass *>(g_devClasses[DeviceClass::DeviceTypeDisk]))->Refresh();
			//don't enum devices

			pDevClass = new VolumeDeviceClass(m_pLibHandle);
			if(pDevClass == NULL)
			{
				LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_FATAL_ERROR, _T(" Failed to create VolumeDeviceClass"));
				SetSelfThreadRunStatus(FALSE);
				return FALSE;
			}
			g_devClasses[DeviceClass::DeviceTypeMsc] = pDevClass;
			g_devClasses[DeviceClass::DeviceTypeMsc]->SetMSCVidPid((*stateIt)->uiVid, (*stateIt)->uiPid);
			pDevClass->Devices();
			break;
		}
	}
	printf("after iterations\n");
	// Init all the USB Ports.
	((usb::HubClass*)(g_devClasses[DeviceClass::DeviceTypeUsbHub]))->RefreshHubs();

	// resume DeviceManager::Open() function
//	m_bSelfThreadRunning = TRUE;
//	SetEvent(_hStartEvent);
	//SetSelfThreadRunStatus(TRUE);

	return TRUE;
}

int DeviceManager::ExitInstance()
{
	m_pExpectionHandler->Close();
	delete m_pExpectionHandler;

	//delete DeviceClass that have been newed
	std::map<DWORD, DeviceClass*>::iterator deviceClassIt;
	for ( deviceClassIt = g_devClasses.begin(); deviceClassIt != g_devClasses.end(); ++deviceClassIt )
	{
		if((*deviceClassIt).second != NULL)
		{
			delete (*deviceClassIt).second;
			(*deviceClassIt).second = NULL;
		}
	}
	g_devClasses.clear();

	// Messaging support
    if ( _hUsbDev != (long)INVALID_HANDLE_VALUE )
	    _hUsbDev=_hUsbDev;
       // UnregisterDeviceNotification(_hUsbDev);
    if ( _hUsbHub != (long)INVALID_HANDLE_VALUE )
	    _hUsbHub=_hUsbHub;
       // UnregisterDeviceNotification(_hUsbHub);

	// Windows must be Destroyed in the thread in which they were created.
    // _DevChangeWnd was created in the DeviceManager thread in InitInstance().
    //BOOL ret = _DevChangeWnd.DestroyWindow();

	std::map<HANDLE_CALLBACK, CBStruct*>::iterator cbIt = m_callbacks.begin();
	for(; cbIt!=m_callbacks.end(); cbIt++)
	{
		CBStruct *pCb;
		pCb = (CBStruct *)((*cbIt).first);
		if(pCb != NULL)
		{
			delete pCb;
		}
	}
	m_callbacks.clear();

	return 0;
}

//BEGIN_MESSAGE_MAP(DeviceManager::DevChangeWnd, CWnd)
    //ON_WM_DEVICECHANGE()
//END_MESSAGE_MAP()
#ifdef __linux__

int DevChange_callback(struct libusb_context *ctx, struct libusb_device *dev, libusb_hotplug_event event, void *user_data) {
	DeviceManager* pDevManage = (DeviceManager*)user_data;
	static libusb_device_handle *handle = NULL;
        struct libusb_device_descriptor desc;
        int rc;
        (void)libusb_get_device_descriptor(dev, &desc);
	switch((int)event){
	    case LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED: {  // a device matching our PID/VID pairs has shown up
		    printf(" Dev arrival in libusb callback\n");
		    thread_msg temp;
		    CString strDesc;
		    strDesc.Format(_T("vid_%04x&pid_%04x"), desc.idVendor, desc.idProduct);
		    temp.message=DeviceManager::DEVICE_ARRIVAL_EVT;
		    temp.lParam=(unsigned long)&strDesc;
		    temp.wParam=NULL;
		    pDevManage->DevMgrMsgs.push(temp);
		    sem_post(&pDevManage->msgs);		  
		    break;
		}
	    case LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT: {  //a device matching our PID/VID pair has left
		if (handle) {
		    printf("closed already open device\n");
		    libusb_close(handle);
		    handle = NULL;
		}
		else{

		    printf("unplugged unopend device \n");
		}
		break;
	    }
        default:// error for event
            printf("Unhandled event %d\n", event);
	    printf("evnt arrived %d\n",LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED);
	    printf("evnt left %d\n",LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT);
	    return -1;
        }
        return 0;
}

#else
BOOL DeviceManager::DevChangeWnd::OnDeviceChange(UINT nEventType, DWORD_PTR dwData)
{
	PDEV_BROADCAST_HDR lpdb = (PDEV_BROADCAST_HDR)dwData;
	CString strMsg;
	DWORD event = UNKNOWN_EVT;
	
	if( lpdb != NULL )
	{
		switch(nEventType) 
		{
		case DBT_DEVICEARRIVAL:	//A device has been inserted and is now available.
			//check its dbch_devicetype member to determine the device type
			if( lpdb->dbch_devicetype == DBT_DEVTYP_DEVICEINTERFACE )  //Class of devices
			{	//Class of devices, dwData is a DEV_BROADCAST_DEVICEINTERFACE structure
				if( ((PDEV_BROADCAST_DEVICEINTERFACE)lpdb)->dbcc_classguid == GUID_DEVINTERFACE_USB_DEVICE )
				{	//USB device
					strMsg = (LPCTSTR)((PDEV_BROADCAST_DEVICEINTERFACE)lpdb)->dbcc_name;
					//strMsg looks like the following: "\\?\USB#Vid_15a2&Pid_0054#6&28e0277e&0&3#{a5dcbf10-6530-11d2-901f-00c04fb951ed}", can get Vid&Pid
					//when MassStorage device arrived, first DEVICE_ARRIVAL_EVT, it's DiskDeviceClass, and the strMsg looks like "\\?\USB#Vid_066f&Pid_37ff#6&28e0277e&0&3#{a5dcbf10-6530-11d2-901f-00c04fb951ed}"
					//can get the second stage(MassStorage)device's vid&pid(Vid_066f, Pid_37ff)[the pid&vid is specified in ucl.xml]
				//	CString strDebug;
				//	strDebug.Format(_T("vid_%04x&pid_%04x"), 0x066f, 0x37ff);
				//	strMsg.MakeUpper();
				//	strDebug.MakeUpper();
				//	if( strMsg.Find(strDebug) != -1 )
				//	{	//find updater DeviceArrival
				//		QueryPerformanceFrequency(&g_tc);
				//		QueryPerformanceCounter(&g_t1);
				//		break;
				//	}

					event = DEVICE_ARRIVAL_EVT;
					LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_NORMAL_MSG, _T("DeviceManager::DevChangeWnd::OnDeviceChange() - DEVICE_ARRIVAL_EVT(%s)"),strMsg);
				}
				else if( ((PDEV_BROADCAST_DEVICEINTERFACE)lpdb)->dbcc_classguid == GUID_DEVINTERFACE_USB_HUB )
				{	//USB Hub
					strMsg = (LPCTSTR)((PDEV_BROADCAST_DEVICEINTERFACE)lpdb)->dbcc_name;
					event = HUB_ARRIVAL_EVT;
					LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_NORMAL_MSG, _T("DeviceManager::DevChangeWnd::OnDeviceChange() - HUB_ARRIVAL_EVT(%s)"),strMsg);
				}
			}
			else if(lpdb->dbch_devicetype == DBT_DEVTYP_VOLUME)  //Logical volume
			{
				strMsg = DrivesFromMask( ((PDEV_BROADCAST_VOLUME)lpdb)->dbcv_unitmask );
				//strMsg looks like as "G"
				//the second stage(Updater) first DEVICE Arrive(DiskDeviceClass), then Volume Arrive(VolumeDeviceClass)

			//	QueryPerformanceCounter(&g_t2);
			//	double dTotalTime = (double)(g_t2.QuadPart-g_t1.QuadPart) / (double)g_tc.QuadPart;
			//	LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_NORMAL_MSG, _T("DEVICE_ARRIVAL_EVT to VOLUME_ARRIVAL_EVT-Time: %f(S)"), dTotalTime);

				event = VOLUME_ARRIVAL_EVT;
				LogMsg(LOG_MODULE_MFGTOOL_LIB,LOG_LEVEL_NORMAL_MSG,  _T("DeviceManager::DevChangeWnd::OnDeviceChange() - VOLUME_ARRIVAL_EVT(%s)"), strMsg);
			}
			break;
		case DBT_DEVICEREMOVECOMPLETE:	//Device has been removed
			//check its dbch_devicetype member to determine the device type
			if( lpdb->dbch_devicetype == DBT_DEVTYP_DEVICEINTERFACE )
			{	//Class of devices, dwData is a DEV_BROADCAST_DEVICEINTERFACE structure
				if( ((PDEV_BROADCAST_DEVICEINTERFACE)lpdb)->dbcc_classguid == GUID_DEVINTERFACE_USB_DEVICE )
				{	//USB device
					strMsg = (LPCTSTR)((PDEV_BROADCAST_DEVICEINTERFACE)lpdb)->dbcc_name;
					//strMsg looks like the following: "\\?\USB#Vid_15a2&Pid_0054#6&28e0277e&0&3#{a5dcbf10-6530-11d2-901f-00c04fb951ed}", can get Vid&Pid
					event = DEVICE_REMOVAL_EVT;
					LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_NORMAL_MSG, _T("DeviceManager::DevChangeWnd::OnDeviceChange() - DEVICE_REMOVAL_EVT(%s)"),strMsg);
				}
				else if( ((PDEV_BROADCAST_DEVICEINTERFACE)lpdb)->dbcc_classguid == GUID_DEVINTERFACE_USB_HUB )
				{	//USB Hub
					strMsg = (LPCTSTR)((PDEV_BROADCAST_DEVICEINTERFACE)lpdb)->dbcc_name;
					event = HUB_REMOVAL_EVT;
					LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_NORMAL_MSG, _T("DeviceManager::DevChangeWnd::OnDeviceChange() - HUB_REMOVAL_EVT(%s)"),strMsg);
				}
			}
			else if(lpdb->dbch_devicetype == DBT_DEVTYP_VOLUME) 
            {
                strMsg = DrivesFromMask( ((PDEV_BROADCAST_VOLUME)lpdb)->dbcv_unitmask );
				//when update is complete, remove the usb cable, first VOLUME Remove, then DEVICE Remove
				//strMsg looks like as "G"
                event = VOLUME_REMOVAL_EVT;
                LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_NORMAL_MSG, _T("DeviceManager::DevChangeWnd::OnDeviceChange() - VOLUME_REMOVAL_EVT(%s)"), strMsg);
            }
			break;
		default:
			return FALSE;
		}

		//Don't send unknown event
		if ( UNKNOWN_EVT == event ) 
		{
			return TRUE;
		}
		
		// let's figure out what to do with the WM_DEVICECHANGE message
        // after we get out of this loop so we don't miss any messages.
		//BSTR bstr_msg = strMsg.AllocSysString();
		if(g_pDeviceManager != NULL)
		{
			//g_pDeviceManager->PostThreadMessage(WM_MSG_DEV_EVENT, (WPARAM)event, (LPARAM)bstr_msg);
		}

		LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_NORMAL_MSG, _T("DeviceManager::DevChangeWnd::OnDeviceChange() - end"));
	}

	return TRUE;
}
// worker function for OnDeviceChange() to get drive letters from the bitmask
// Runs in the context of the DeviceManager thread
CString DeviceManager::DevChangeWnd::DrivesFromMask(ULONG UnitMask)
{
    CString Drive;
    TCHAR Char;

    for (Char = 0; Char < 26; ++Char) 
    {
        if (UnitMask & 0x1)
            Drive.AppendFormat(_T("%c"), Char + _T('A'));
        UnitMask = UnitMask >> 1;
    }

    return Drive;
}

#endif
//BEGIN_MESSAGE_MAP(DeviceManager, CWinThread)
 //   ON_THREAD_MESSAGE(WM_MSG_DEV_EVENT, OnMsgDeviceEvent)
//END_MESSAGE_MAP()

void DeviceManager::OnMsgDeviceEvent(WPARAM eventType, LPARAM desc)
{

	CString msg = (LPCTSTR)desc;
   // SysFreeString((BSTR)desc);
	OP_STATE_ARRAY *pOpStates = NULL;
#if 0
	if ( eventType == EVENT_KILL ) 
    {
        //PostQuitMessage(0);	//send WM_QUIT to DeviceManager thread message queue, and then end DeviceManager thread, the exit code is 0
		LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_NORMAL_MSG, _T("DeviceManager::OnMsgDeviceEvent() - EVENT_KILL"));
        return;
    }
#endif
	//Skip MSC device message if there is no MSC device required.
	if(VOLUME_ARRIVAL_EVT == eventType || VOLUME_REMOVAL_EVT == eventType)
	{
		if(g_devClasses[DeviceClass::DeviceTypeMsc]->m_msc_vid == 0x00 && 
			g_devClasses[DeviceClass::DeviceTypeMsc]->m_msc_pid == 0x00)
		{
			LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_NORMAL_MSG, _T("DeviceManager::OnMsgDeviceEvent() - VOLUME_ARRIVAL_EVT or VOLUME_REMOVAL_EVT, but Msc VID&PID not initialized"));
			return;
		}
	}

	switch( eventType )
	{
		case DEVICE_ARRIVAL_EVT:
		{	printf("detected an event arrival in msg handler\n");
			TRACE(_T("Device manager device arrive\r\n"));
			LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_NORMAL_MSG, _T("DeviceManager::OnMsgDeviceEvent() - DEVICE_ARRIVAL_EVT(%s)"), msg.c_str());
			pOpStates = GetOpStates((MFGLIB_VARS *)m_pLibHandle);
			if(pOpStates == NULL)
			{
				LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_FATAL_ERROR, _T("DeviceManager::OnMsgDeviceEvent() - DEVICE_ARRIVAL_EVT, no OpStates"));
				return;
			}
			CString filter;
			BOOL isRightDevice = FALSE;
			OP_STATE_ARRAY::iterator it = pOpStates->begin();
			COpState *pCurrentState = NULL;
			for(; it!=pOpStates->end(); it++)
			{
				filter.Format(_T("vid_%04x&pid_%04x"), (*it)->uiVid, (*it)->uiPid);
				msg.MakeUpper();
				filter.MakeUpper();
				if( msg.Find(filter) != -1 )
				{	//find
					isRightDevice = TRUE;
					pCurrentState = (*it);
					break;
				}
			}
			if(isRightDevice) //OK, find the device
			{
				DeviceClass::NotifyStruct nsInfo = {0};
				switch(pCurrentState->opDeviceType)
				{
				case DEV_HID_MX28:	//Hid
					nsInfo = g_devClasses[DeviceClass::DeviceTypeHid]->AddUsbDevice(msg);
					LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_NORMAL_MSG, _T("DeviceManager::OnMsgDeviceEvent() - DEVICE_ARRIVAL_EVT,[HidDeviceClass] vid_%04x&pid_%04x, Hub:%d-Port:%d"), pCurrentState->uiVid, pCurrentState->uiPid, nsInfo.HubIndex, nsInfo.PortIndex);
					break;

			/*	case DEV_MX23:		//Hid
				case DEV_MX28:
					break;
				case DEV_MX25:
				case DEV_MX35:
				case DEV_MX51:
				case DEV_MX53:		//MxRom
					LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_NORMAL_MSG, _T("DeviceManager::OnMsgDeviceEvent() - DEVICE_ARRIVAL_EVT,[MxRomDeviceClass] vid_%04x&pid_%04x"), pCurrentState->uiVid, pCurrentState->uiPid);
					nsInfo = g_devClasses[DeviceClass::DeviceTypeMxRom]->AddUsbDevice(msg);
					break; 
				case DEV_MX50: */
				case DEV_HID_MX6D:
		        case DEV_HID_MX6SL:
				case DEV_HID_MX6SX:
				case DEV_HID_MX6Q:	//MxHid
					nsInfo = g_devClasses[DeviceClass::DeviceTypeMxHid]->AddUsbDevice(msg);
					LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_NORMAL_MSG, _T("DeviceManager::OnMsgDeviceEvent() - DEVICE_ARRIVAL_EVT,[MxHidDeviceClass] vid_%04x&pid_%04x, Hub:%d-Port:%d"), pCurrentState->uiVid, pCurrentState->uiPid, nsInfo.HubIndex, nsInfo.PortIndex);
					break;
				case DEV_MSC_UPDATER:
					// don't look for USB arrival in MSC class, wait for Volume arrival.
					LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_NORMAL_MSG, _T("DeviceManager::OnMsgDeviceEvent() - DEVICE_ARRIVAL_EVT,[Msc,DiskDeviceClass] vid_%04x&pid_%04x, not handled"), pCurrentState->uiVid, pCurrentState->uiPid);
					nsInfo.pDevice = NULL;
					//QueryPerformanceFrequency(&g_tc);
					//QueryPerformanceCounter(&g_t1);
					break;
				}
				if(nsInfo.pDevice)
				{
					nsInfo.Event = DEVICE_ARRIVAL_EVT;

					//Loop for Volume device as the same HubIndex and PortIndex;
					std::list<Device*>::iterator deviceIt;
					int portIndex;
					CString hubPath;
					BOOL bExceptionExist = FALSE;
					
					for(deviceIt=g_devClasses[DeviceClass::DeviceTypeMsc]->_devices.begin(); deviceIt!=g_devClasses[DeviceClass::DeviceTypeMsc]->_devices.end(); ++deviceIt)
					{
						portIndex = (*deviceIt)->_hubIndex.get();
						hubPath = (*deviceIt)->_hub.get();
						if( (hubPath.CompareNoCase(nsInfo.Hub)==0) && (portIndex==nsInfo.PortIndex ) ) //the same Hub-Port has two device
						{
							bExceptionExist = TRUE;
							break;
						}
					}
					if(bExceptionExist)
					{
						LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_NORMAL_MSG, _T("DeviceArriveBeforVolumeRemove Exception occurs"));
						pthread_mutex_lock(m_pExpectionHandler->m_hMapMsgMutex);
						mapMsg[msg] = 1;
						pthread_mutex_unlock(m_pExpectionHandler->m_hMapMsgMutex);
						//BSTR bstr_msg = msg.AllocSysString();
						//m_pExpectionHandler->PostThreadMessage(WM_MSG_EXCEPTION_EVENT, (WPARAM)(CMyExceptionHandler::DeviceArriveBeforVolumeRemove), (LPARAM)bstr_msg);
						return;
					}

					LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_NORMAL_MSG, _T("DeviceManager::OnMsgDeviceEvent() - DEVICE_ARRIVAL_EVT, Notify"));
					Notify(&nsInfo);
				}
				else
				{
					switch(pCurrentState->opDeviceType)
					{
					//case DEV_MX50:
					case DEV_HID_MX28:
					case DEV_HID_MX6D:
					case DEV_HID_MX6SL:
					case DEV_HID_MX6SX:
					case DEV_HID_MX6Q:	//MxHid 
						{
							LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_NORMAL_MSG, _T("DeviceArriveButEnumFailed Exception occurs"));
							pthread_mutex_lock(m_pExpectionHandler->m_hMapMsgMutex);
							mapMsg[msg] = 1;
							pthread_mutex_unlock(m_pExpectionHandler->m_hMapMsgMutex);
							//BSTR bstr_msg = msg.AllocSysString();
							//m_pExpectionHandler->PostThreadMessage(WM_MSG_EXCEPTION_EVENT, (WPARAM)(CMyExceptionHandler::DeviceArriveButEnumFailed), (LPARAM)bstr_msg);
							return;
						}
						break;
					}
				}
			}
			break;
		} //end case DEVICE_ARRIVAL_EVT
		case DEVICE_REMOVAL_EVT:
		{
			TRACE(_T("Device manager device remove\r\n"));
			LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_NORMAL_MSG, _T("DeviceManager::OnMsgDeviceEvent() - DEVICE_REMOVAL_EVT(%s)"), msg.c_str());
			pOpStates = GetOpStates((MFGLIB_VARS *)m_pLibHandle);
			if(NULL == pOpStates)
			{
				return;
			}
			CString filter;
			BOOL isRightDevice = FALSE;
			OP_STATE_ARRAY::iterator it = pOpStates->begin();
			COpState *pCurrentState = NULL;
			for(; it!=pOpStates->end(); it++)
			{
				filter.Format(_T("vid_%04x&pid_%04x"), (*it)->uiVid, (*it)->uiPid);
				msg.MakeUpper();
				filter.MakeUpper();
				if( msg.Find(filter) != -1 )
				{	//find
					isRightDevice = TRUE;
					pCurrentState = (*it);
					break;
				}
			}
			if(isRightDevice) //OK, find the device
			{
				//check exception handle queue
				pthread_mutex_lock(m_pExpectionHandler->m_hMapMsgMutex);
				std::map<CString, int>::iterator it = mapMsg.find(msg);
				if(it != mapMsg.end())
				{
					LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_NORMAL_MSG, _T("DeviceManager::OnMsgDeviceEvent() - DEVICE_REMOVAL_EVT, find not handled exception event, so don't to need handle"));
					mapMsg.erase(it);
					pthread_mutex_unlock(m_pExpectionHandler->m_hMapMsgMutex);
					return;
				}
				pthread_mutex_unlock(m_pExpectionHandler->m_hMapMsgMutex);

				DeviceClass::NotifyStruct nsInfo = {0};
				DeviceClass::DEV_CLASS_TYPE class_type;
				switch(pCurrentState->opDeviceType)
				{
				case DEV_HID_MX28:
					nsInfo = g_devClasses[DeviceClass::DeviceTypeHid]->RemoveUsbDevice(msg);
					class_type = DeviceClass::DeviceTypeHid;
					LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_NORMAL_MSG, _T("DeviceManager::OnMsgDeviceEvent() - DEVICE_REMOVAL_EVT,[HidDeviceClass] vid_%04x&pid_%04x, Hub:%d-Port:%d"), pCurrentState->uiVid, pCurrentState->uiPid, nsInfo.HubIndex, nsInfo.PortIndex);
					break;
			/*	case DEV_MX23:		//Hid
				case DEV_MX28:
					break;
				case DEV_MX25:
				case DEV_MX35:
				case DEV_MX51:
				case DEV_MX53:		//MxRom
					LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_NORMAL_MSG, _T("DeviceManager::OnMsgDeviceEvent() - DEVICE_REMOVAL_EVT,[MxRomDeviceClass] vid_%04x&pid_%04x"), pCurrentState->uiVid, pCurrentState->uiPid);
					nsInfo = g_devClasses[DeviceClass::DeviceTypeMxRom]->RemoveUsbDevice(msg);
					break;
				case DEV_MX50: */
				case DEV_HID_MX6Q:	//MxHid
				case DEV_HID_MX6D:
		        case DEV_HID_MX6SL:
				case DEV_HID_MX6SX:
					nsInfo = g_devClasses[DeviceClass::DeviceTypeMxHid]->RemoveUsbDevice(msg);
					class_type = DeviceClass::DeviceTypeMxHid;
					LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_NORMAL_MSG, _T("DeviceManager::OnMsgDeviceEvent() - DEVICE_REMOVAL_EVT,[MxHidDeviceClass] vid_%04x&pid_%04x, Hub:%d-Port:%d"), pCurrentState->uiVid, pCurrentState->uiPid, nsInfo.HubIndex, nsInfo.PortIndex);
					break;
				case DEV_MSC_UPDATER:
					LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_NORMAL_MSG, _T("DeviceManager::OnMsgDeviceEvent() - DEVICE_REMOVAL_EVT,[Msc,DiskDeviceClass] vid_%04x&pid_%04x, not handled"), pCurrentState->uiVid, pCurrentState->uiPid);
					nsInfo.pDevice = NULL;
					break;
				}
				if(nsInfo.pDevice)
				{
					nsInfo.Event = DEVICE_REMOVAL_EVT;
					LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_NORMAL_MSG, _T("DeviceManager::OnMsgDeviceEvent() - DEVICE_REMOVAL_EVT, Notify"));
					Notify(&nsInfo);

					//delete device
					std::list<Device*>::iterator deviceIt;
					for (deviceIt = g_devClasses[class_type]->_devices.begin(); deviceIt != g_devClasses[class_type]->_devices.end(); ++deviceIt)
					{
						if((*deviceIt) == nsInfo.pDevice)
						{
							break;
						}
					}

					//DWORD dwResult = WaitForMultipleObjects(MAX_BOARD_NUMBERS, g_hDevCanDeleteEvts, FALSE, INFINITE);
					TRACE(_T("Device manager wait for begin\r\n"));
					WaitOnEvent(((MFGLIB_VARS *)m_pLibHandle)->g_hDevCanDeleteEvts[nsInfo.pDevice->GetDeviceWndIndex()]);
					TRACE(_T("Device manager wait for end\r\n"));
					//int index = dwResult - WAIT_OBJECT_0;
					LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_NORMAL_MSG, _T("DeviceManager::OnMsgDeviceEvent()-DEVICE_REMOVAL_EVT, hDevCanDeleteEvent has been set"));
					pthread_mutex_lock(g_devClasses[class_type]->devicesMutex);// , INFINITE);
					delete (*deviceIt);
					g_devClasses[class_type]->_devices.erase(deviceIt);
					//LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_NORMAL_MSG, _T("DeviceManager::OnMsgDeviceEvent() - DEVICE_REMOVAL_EVT Device Object[0x%X] has been delete"), (*deviceIt));
					pthread_mutex_unlock(g_devClasses[class_type]->devicesMutex);
				}
			}
			break;
		} //end case DEVICE_REMOVAL_EVT
		case VOLUME_ARRIVAL_EVT:
		{
			int msgLetterIndex;
			CString driveLetterStr;
	            TRACE(_T("Device manager volume arrive %s\r\n"), msg.c_str());
			LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_NORMAL_MSG, _T("DeviceManager::OnMsgDeviceEvent() - VOLUME_ARRIVAL_EVT(%s)"), msg.c_str());
			//QueryPerformanceCounter(&g_t2);
			//double dTotalTime = (double)(g_t2.QuadPart-g_t1.QuadPart) / (double)g_tc.QuadPart;
			//LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_NORMAL_MSG, _T("DEVICE_ARRIVAL_EVT to VOLUME_ARRIVAL_EVT-Time: %f"), dTotalTime);

			g_devClasses[DeviceClass::DeviceTypeDisk]->Refresh();

			for ( msgLetterIndex = 0; msgLetterIndex < msg.GetLength(); ++msgLetterIndex )
			{
				driveLetterStr = msg.substr(msgLetterIndex,1);
				DeviceClass::NotifyStruct nsInfo = g_devClasses[DeviceClass::DeviceTypeMsc]->AddUsbDevice(driveLetterStr);
				LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_NORMAL_MSG, _T("DeviceManager::OnMsgDeviceEvent() - VOLUME_ARRIVAL_EVT-Disk(%s), Hub:%d-Port:%d"), driveLetterStr.c_str(), nsInfo.HubIndex, nsInfo.PortIndex);
				if(nsInfo.pDevice)
				{
					//QueryPerformanceFrequency(&g_tc);
					//QueryPerformanceCounter(&g_t1);

					nsInfo.Event = VOLUME_ARRIVAL_EVT;
					LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_NORMAL_MSG, _T("DeviceManager::OnMsgDeviceEvent() - VOLUME_ARRIVAL_EVT, Notify"));
					Notify(&nsInfo);
				}
			}
			break;
		} //end case VOLUME_ARRIVAL_EVT
		case VOLUME_REMOVAL_EVT:
		{
			int msgLetterIndex;
			CString driveLetterStr;
			TRACE(_T("Device manager volume remove %s\r\n"), msg.c_str());
			LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_NORMAL_MSG, _T("DeviceManager::OnMsgDeviceEvent() - VOLUME_REMOVAL_EVT(%s)"), msg.c_str());
			for ( msgLetterIndex = 0; msgLetterIndex < msg.GetLength(); ++msgLetterIndex )
			{
				driveLetterStr = msg.substr(msgLetterIndex,1);
				DeviceClass::NotifyStruct nsInfo = g_devClasses[DeviceClass::DeviceTypeMsc]->RemoveUsbDevice(driveLetterStr);
				LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_NORMAL_MSG, _T("DeviceManager::OnMsgDeviceEvent() - VOLUME_REMOVAL_EVT-Disk(%s), Hub:%d-Port:%d"), driveLetterStr.c_str(), nsInfo.HubIndex, nsInfo.PortIndex);
				if(nsInfo.pDevice)
				{
					nsInfo.Event = VOLUME_REMOVAL_EVT;
					LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_NORMAL_MSG, _T("DeviceManager::OnMsgDeviceEvent() - VOLUME_REMOVAL_EVT, Notify"));
					Notify(&nsInfo);
				}
				else
				{
					return;
				}

				//delete device
				std::list<Device*>::iterator deviceIt;
				for(deviceIt=g_devClasses[DeviceClass::DeviceTypeMsc]->_devices.begin(); deviceIt!=g_devClasses[DeviceClass::DeviceTypeMsc]->_devices.end(); ++deviceIt)
				{
					if((*deviceIt) == nsInfo.pDevice)
					{
						break;
					}
				}
				
				if(nsInfo.PortIndex == 0)
				{
					// at such case, the cmdoperation thread can't get the volume remove event at all
					// we need to call cmdoperation's call back
					nsInfo.Event = VOLUME_REMOVAL_EVT;
					Notify(&nsInfo, nsInfo.pDevice->GetDeviceWndIndex());
				}

				TRACE(_T("Device manager wait for begin %d\r\n"),nsInfo.pDevice->GetDeviceWndIndex());
				LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_NORMAL_MSG, _T("DeviceManager::OnMsgDeviceEvent()-VOLUME_REMOVAL_EVT, wait hDevCanDeleteEvent"));
				WaitOnEvent(((MFGLIB_VARS *)m_pLibHandle)->g_hDevCanDeleteEvts[nsInfo.pDevice->GetDeviceWndIndex()]);// , INFINITE);
				TRACE(_T("Device manager wait for end\r\n"));
				LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_NORMAL_MSG, _T("DeviceManager::OnMsgDeviceEvent()-VOLUME_REMOVAL_EVT, hDevCanDeleteEvent has been set"));
				pthread_mutex_lock(g_devClasses[DeviceClass::DeviceTypeMsc]->devicesMutex);// , INFINITE);
				//find the corresponding disk device
				std::list<Device*>::iterator diskIt;
				for(diskIt=g_devClasses[DeviceClass::DeviceTypeDisk]->_devices.begin(); diskIt!=g_devClasses[DeviceClass::DeviceTypeDisk]->_devices.end(); ++diskIt)
				{
					if((dynamic_cast<Volume*>(*deviceIt))->StorageDisk() == (*diskIt))
					{
						break;
					}
				}
				LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_NORMAL_MSG, _T("DeviceManager::OnMsgDeviceEvent()-VOLUME_REMOVAL_EVT, delete Disk:%p, Volume:%p"), *diskIt, *deviceIt);
				delete (*diskIt);
				g_devClasses[DeviceClass::DeviceTypeDisk]->_devices.erase(diskIt);
				delete (*deviceIt);
				g_devClasses[DeviceClass::DeviceTypeMsc]->_devices.erase(deviceIt);
				pthread_mutex_unlock(g_devClasses[DeviceClass::DeviceTypeMsc]->devicesMutex);
			}
			break;
		} //end case VOLUME_REMOVAL_EVT
		case HUB_ARRIVAL_EVT:
			{
				DeviceClass::NotifyStruct nsInfo = g_devClasses[DeviceClass::DeviceTypeUsbHub]->AddUsbDevice(msg);
				LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_NORMAL_MSG, _T("DeviceManager::OnMsgDeviceEvent() - HUB_ARRIVAL_EVT"));
				if ( nsInfo.pDevice )
				{
					nsInfo.Event = HUB_ARRIVAL_EVT;
					Notify(&nsInfo);
				}
			}
			break;
		case HUB_REMOVAL_EVT:
			{
				DeviceClass::NotifyStruct nsInfo = g_devClasses[DeviceClass::DeviceTypeUsbHub]->RemoveUsbDevice(msg);
				LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_NORMAL_MSG, _T("DeviceManager::OnMsgDeviceEvent() - HUB_REMOVAL_EVT"));
				if ( nsInfo.pDevice )
				{
					nsInfo.Event = HUB_REMOVAL_EVT;
					Notify(&nsInfo);
				}
			}
			break;
		default:
			ASSERT(0);
			break;
	}	//end switch( eventType )
return;
}

HANDLE_CALLBACK DeviceManager::RegisterCallback(CBStruct *pCB)
{
	CBStruct *pCallback = new CBStruct;
	if(pCallback == NULL)
	{
		LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_NORMAL_MSG, _T("DeviceManager::RegisterCallback() failed"));
		return NULL;
	}
	pCallback->pfunc = pCB->pfunc;
	pCallback->Hub = pCB->Hub;
	pCallback->HubIndex = pCB->HubIndex;
	pCallback->cmdOpIndex = pCB->cmdOpIndex;
	pthread_mutex_lock(m_hMutex_cb);// , INFINITE);
	m_callbacks[pCallback] = pCallback;
	pthread_mutex_unlock(m_hMutex_cb);
	return pCallback;
}

void DeviceManager::UnregisterCallback(HANDLE_CALLBACK hCallback)
{
	if(hCallback)
	{
		pthread_mutex_lock(m_hMutex_cb);// , INFINITE);
		delete (CBStruct *)hCallback;
		m_callbacks.erase(hCallback);
		pthread_mutex_unlock(m_hMutex_cb);
	}
}

void DeviceManager::Notify(DeviceClass::NotifyStruct* pnsInfo)
{
	std::map<HANDLE_CALLBACK, CBStruct*>::iterator cbIt;
	CBStruct* pCallback;

	if( (pnsInfo->Event == HUB_ARRIVAL_EVT) || (pnsInfo->Event == HUB_REMOVAL_EVT) )
	{
		return;
	}

	BOOL doNotify = FALSE;
	int i = 0;
	for(i=0; i<MAX_BOARD_NUMBERS; i++)
	{
		if(((MFGLIB_VARS *)m_pLibHandle)->g_PortDevInfoArray[i].m_bUsed)
		{
			if( (((MFGLIB_VARS *)m_pLibHandle)->g_PortDevInfoArray[i].hubPath.CompareNoCase(pnsInfo->Hub) == 0)
				&& (((MFGLIB_VARS *)m_pLibHandle)->g_PortDevInfoArray[i].portIndex == pnsInfo->PortIndex) )
			{
				doNotify = TRUE;
				break;
			}
		}
	}
	if(i >= MAX_BOARD_NUMBERS)
	{
		for(i=0; i<MAX_BOARD_NUMBERS; i++)
		{
			if(!((MFGLIB_VARS *)m_pLibHandle)->g_PortDevInfoArray[i].m_bUsed)
			{
				((MFGLIB_VARS *)m_pLibHandle)->g_PortDevInfoArray[i].hubPath = pnsInfo->Hub;
				((MFGLIB_VARS *)m_pLibHandle)->g_PortDevInfoArray[i].portIndex = pnsInfo->PortIndex;
				((MFGLIB_VARS *)m_pLibHandle)->g_PortDevInfoArray[i].m_bUsed = TRUE;
				((MFGLIB_VARS *)m_pLibHandle)->g_PortDevInfoArray[i].hubIndex = pnsInfo->HubIndex;
				doNotify = TRUE;
				break;
			}
		}
	}

	if(doNotify)
	{
		pthread_mutex_lock(m_hMutex_cb);
		if( !m_callbacks.empty() )
		{
			for(cbIt=m_callbacks.begin(); cbIt!=m_callbacks.end(); cbIt++)
			{
				pCallback = (*cbIt).second;
				if(pCallback->cmdOpIndex == i)
				{
					pnsInfo->cmdOpIndex = pCallback->cmdOpIndex;
					pCallback->pfunc(m_pLibHandle, pnsInfo);
				}
			}
		}
		pthread_mutex_unlock(m_hMutex_cb);
	}
}

void DeviceManager::Notify(DeviceClass::NotifyStruct* pnsInfo, int WndIndex)
{
	std::map<HANDLE_CALLBACK, CBStruct*>::iterator cbIt;
	CBStruct* pCallback;


	for(cbIt=m_callbacks.begin(); cbIt!=m_callbacks.end(); cbIt++)
	{
		BOOL doNotify = FALSE;
		pCallback = (*cbIt).second;

        if(pCallback->cmdOpIndex == WndIndex)
        {
            pnsInfo->cmdOpIndex = pCallback->cmdOpIndex;
            pCallback->pfunc(m_pLibHandle, pnsInfo);
			return;
        }
	}
}


