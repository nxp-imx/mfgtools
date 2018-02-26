/*
 * Copyright 2009-2014, Freescale Semiconductor, Inc. All Rights Reserved.
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

#include "stdafx.h"
#include "DeviceManager.h"
#include "ControllerClass.h"
#include "HubClass.h"
#include "MxRomDeviceClass.h"
#include "HidDeviceClass.h"
#include "MxHidDeviceClass.h"
#include "KblHidDeviceClass.h"
#include "DiskDeviceClass.h"
#include "VolumeDeviceClass.h"
#include "CdcDeviceClass.h"

#include "MfgToolLib_Export.h"
#include "MfgToolLib.h"
#include "ExceptionHandle.h"

#include <Dbt.h>

DeviceManager* g_pDeviceManager;
DEV_CLASS_ARRAY g_devClasses;

//only dor debug
LARGE_INTEGER g_t1, g_t2, g_t3, g_tc;

//////////////////////////////////////////////////////////////////////
//
// DeviceManager class implementation
//
//////////////////////////////////////////////////////////////////////
IMPLEMENT_DYNCREATE(DeviceManager, CWinThread)

DeviceManager::DeviceManager(INSTANCE_HANDLE handle)
: _hUsbDev(NULL)
, _hUsbHub(NULL)
, _bStopped(TRUE)
, _hStartEvent(NULL)
{	
	m_pLibHandle = handle;
	m_bSelfThreadRunning = FALSE;
	m_bAutoDelete = FALSE; //Don't destroy the object at thread termination
	m_hMutex_cb = ::CreateMutex(NULL, FALSE, NULL);
	if(m_hMutex_cb == NULL)
	{
		LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_FATAL_ERROR, _T("create DeviceManager::m_hMutex_cb failed, errcode is %d"), GetLastError());
		throw 1;
	}
}

DeviceManager::~DeviceManager()
{
	// Client has to call Close() before it exits;
	ASSERT( _bStopped == TRUE );
	::CloseHandle(m_hMutex_cb);
}

// Create DeviceManager thread
DWORD DeviceManager::Open()
{
	//must make sure the DeviceManager thread  is not running
	if( !_bStopped )
	{
		return MFGLIB_ERROR_DEV_MANAGER_IS_RUNNING;
	}
	
	// Create an event that we can wait on so we will know when
    // the DeviceManager thread has completed InitInstance()
	_hStartEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL); //Auto reset, nosignal
	if( _hStartEvent == NULL )
	{
		LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_FATAL_ERROR, _T("DeviceManager::Open()--Create _hStartEvent failed"));
		return MFGLIB_ERROR_NO_MEMORY;
	}
	
	// Create the user-interface thread supporting messaging
	DWORD dwErrCode = ERROR_SUCCESS;
	if( CreateThread() != 0 ) //create DeviceManager thread successfully
	{
		::WaitForSingleObject(_hStartEvent, INFINITE);
		// Set flag to running. Flag set to stopped in constructor and in Close()
		if(m_bSelfThreadRunning)
		{
			_bStopped = FALSE;
		}
		else
		{
			::CloseHandle(_hStartEvent);
			_hStartEvent = NULL;
			return MFGLIB_ERROR_DEV_MANAGER_RUN_FAILED;
		}
	}
	else //create DeviceManager thread failed
	{
		dwErrCode = GetLastError();
		LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_FATAL_ERROR, _T("DeviceManager::Open()--Create DeviceManager thread failed(error: %d)"), dwErrCode);
		return MFGLIB_ERROR_THREAD_CREATE_FAILED;
	}

	LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_NORMAL_MSG, _T("Device Manager thread is running"));
	
	// clean up
	::CloseHandle(_hStartEvent);
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
	PostThreadMessage(WM_MSG_DEV_EVENT, EVENT_KILL, 0);
	// Wait for the DeviceManager thread to die before returning
	WaitForSingleObject(m_hThread, INFINITE);

	LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_NORMAL_MSG, _T("Device Manager thread is closed"));

	_bStopped = TRUE;
	m_hThread = NULL;
}

void DeviceManager::SetSelfThreadRunStatus(BOOL bRunning)
{
	m_bSelfThreadRunning = bRunning;
	SetEvent(_hStartEvent);
}

//when CreateThread, this function will be executed
BOOL DeviceManager::InitInstance()
{
	//Reset all DeviceClasses
	/*
	g_devClasses[DeviceClass::DeviceTypeDisk] = NULL;
	g_devClasses[DeviceClass::DeviceTypeMsc] = NULL;
	g_devClasses[DeviceClass::DeviceTypeHid] = NULL;
	g_devClasses[DeviceClass::DeviceTypeMxHid] = NULL;
	g_devClasses[DeviceClass::DeviceTypeMxRom] = NULL;
	g_devClasses[DeviceClass::DeviceTypeUsbController] = NULL;
	g_devClasses[DeviceClass::DeviceTypeUsbHub] = NULL;
	g_devClasses[DeviceClass::DeviceTypeKBLCDC] = NULL;
	g_devClasses[DeviceClass::DeviceTypeKBLHID] = NULL;
	*/

	// Create a hidden window and register it to receive WM_DEVICECHANGE messages
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
	}

	for(int i=0; i<MAX_BOARD_NUMBERS; i++)
	{
		m_bHasConnected[i] = FALSE;
	}

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
	
	
	static usb::ControllerClass contClass(m_pLibHandle);
	static usb::HubClass hubclass(m_pLibHandle);
	static DiskDeviceClass disk(m_pLibHandle);
	static VolumeDeviceClass volClass(m_pLibHandle);

	g_devClasses[DeviceClass::DeviceTypeUsbController] = &contClass;
	contClass.Devices();
	
	
	g_devClasses[DeviceClass::DeviceTypeUsbHub] = &hubclass;
	hubclass.Devices();

	OP_STATE_ARRAY *pCurrentStates = GetOpStates((MFGLIB_VARS *)m_pLibHandle);
	if(pCurrentStates == NULL)
	{
		SetSelfThreadRunStatus(FALSE);
		return FALSE;
	}
	std::vector<COpState*>::iterator stateIt = pCurrentStates->begin();
	for(; stateIt!=pCurrentStates->end(); stateIt++)
	{
		switch ((*stateIt)->opState)
		{
		case MX_BOOTSTRAP:
			if ((*stateIt)->romInfo.pDeviceClass)
			{
				(*stateIt)->romInfo.pDeviceClass->m_pLibHandle = m_pLibHandle;
				g_devClasses[(*stateIt)->romInfo.pDeviceClass->_deviceClassType] = (*stateIt)->romInfo.pDeviceClass;
				(*stateIt)->romInfo.pDeviceClass->Devices();
			}
			break;
		case MX_UPDATER:
			
			g_devClasses[DeviceClass::DeviceTypeDisk] = &disk;
			(dynamic_cast<DiskDeviceClass *>(g_devClasses[DeviceClass::DeviceTypeDisk]))->Refresh();
			//don't enum devices

			g_devClasses[DeviceClass::DeviceTypeMsc] = &volClass;
			g_devClasses[DeviceClass::DeviceTypeMsc]->SetMSCVidPid((*stateIt)->uiVid, (*stateIt)->uiPid);
			volClass.Devices();
			break;
		}
		
		/*
		switch((*stateIt)->opDeviceType)
		{
		case DEV_HID_KBL:
			pDevClass = new KblHidDeviceClass(m_pLibHandle);
			if (pDevClass == NULL)
			{
				LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_FATAL_ERROR, _T(" Failed to create MxHidDeviceClass"));
				SetSelfThreadRunStatus(FALSE);
				return FALSE;
			}
			g_devClasses[DeviceClass::DeviceTypeKBLHID] = pDevClass;
			pDevClass->Devices();
			break;
		case DEV_CDC_KBL:
			pDevClass = new CDCDeviceClass(m_pLibHandle);
			if (pDevClass == NULL)
			{
				LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_FATAL_ERROR, _T(" Failed to create CDCDeviceClass"));
				SetSelfThreadRunStatus(FALSE);
				return FALSE;
			}
			g_devClasses[DeviceClass::DeviceTypeKBLCDC] = pDevClass;
			pDevClass->Devices();
			break;
		case DEV_MSC_UPDATER:	//VolumeDeviceClass and DiskDeviceClass
			pDevClass = new DiskDeviceClass(m_pLibHandle);
			if(pDevClass == NULL)
			{
				LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_FATAL_ERROR, _T(" Failed to create DiskDeviceClass"));
				SetSelfThreadRunStatus(FALSE);
				return FALSE;
			}
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
		*/
	}

	// Init all the USB Ports.
	((usb::HubClass*)(g_devClasses[DeviceClass::DeviceTypeUsbHub]))->RefreshHubs();

	// resume DeviceManager::Open() function
//	m_bSelfThreadRunning = TRUE;
//	SetEvent(_hStartEvent);
	SetSelfThreadRunStatus(TRUE);

	return TRUE;
}

int DeviceManager::ExitInstance()
{
	m_pExpectionHandler->Close();
	delete m_pExpectionHandler;

	//delete DeviceClass that have been newed
	std::map<DWORD, DeviceClass*>::iterator deviceClassIt;
	
	g_devClasses.clear();

	// Messaging support
    if ( _hUsbDev != INVALID_HANDLE_VALUE )
        UnregisterDeviceNotification(_hUsbDev);
    if ( _hUsbHub != INVALID_HANDLE_VALUE )
        UnregisterDeviceNotification(_hUsbHub);

	// Windows must be Destroyed in the thread in which they were created.
    // _DevChangeWnd was created in the DeviceManager thread in InitInstance().
    BOOL ret = _DevChangeWnd.DestroyWindow();

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

	return CWinThread::ExitInstance();
}

BEGIN_MESSAGE_MAP(DeviceManager::DevChangeWnd, CWnd)
    ON_WM_DEVICECHANGE()
END_MESSAGE_MAP()

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
		BSTR bstr_msg = strMsg.AllocSysString();
		if(g_pDeviceManager != NULL)
		{
			g_pDeviceManager->PostThreadMessage(WM_MSG_DEV_EVENT, (WPARAM)event, (LPARAM)bstr_msg);
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

BEGIN_MESSAGE_MAP(DeviceManager, CWinThread)
    ON_THREAD_MESSAGE(WM_MSG_DEV_EVENT, OnMsgDeviceEvent)
END_MESSAGE_MAP()

void DeviceManager::OnMsgDeviceEvent(WPARAM eventType, LPARAM desc)
{
	CString msg = (LPCTSTR)desc;
    SysFreeString((BSTR)desc);
	OP_STATE_ARRAY *pOpStates = NULL;

	if ( eventType == EVENT_KILL ) 
    {
        PostQuitMessage(0);	//send WM_QUIT to DeviceManager thread message queue, and then end DeviceManager thread, the exit code is 0
		LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_NORMAL_MSG, _T("DeviceManager::OnMsgDeviceEvent() - EVENT_KILL"));
        return;
    }

	//Skip MSC device message if there is no MSC device required.
	if(VOLUME_ARRIVAL_EVT == eventType || VOLUME_REMOVAL_EVT == eventType)
	{

		if(g_devClasses[DeviceClass::DeviceTypeMsc] && g_devClasses[DeviceClass::DeviceTypeMsc]->m_msc_vid == 0x00 &&
			g_devClasses[DeviceClass::DeviceTypeMsc]->m_msc_pid == 0x00)
		{
			LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_NORMAL_MSG, _T("DeviceManager::OnMsgDeviceEvent() - VOLUME_ARRIVAL_EVT or VOLUME_REMOVAL_EVT, but Msc VID&PID not initialized"));
			return;
		}
	}

	switch( eventType )
	{
		case DEVICE_ARRIVAL_EVT:
		{
			TRACE(_T("Device manager device arrive\r\n"));
			LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_NORMAL_MSG, _T("DeviceManager::OnMsgDeviceEvent() - DEVICE_ARRIVAL_EVT(%s)"), msg);
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
					pCurrentState = (*it);

					DeviceClass::NotifyStruct nsInfo = { 0 };
					if (pCurrentState->romInfo.pDeviceClass)
					{
						nsInfo = pCurrentState->romInfo.pDeviceClass->AddUsbDevice(msg);
						LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_NORMAL_MSG, _T("DeviceManager::OnMsgDeviceEvent() - DEVICE_ARRIVAL_EVT,[HidDeviceClass] vid_%04x&pid_%04x, Hub:%d-Port:%d"), pCurrentState->uiVid, pCurrentState->uiPid, nsInfo.HubIndex, nsInfo.PortIndex);
						if (nsInfo.Device)
						{
							if (!nsInfo.Device->IsCorrectDevice((*it)->uiVid, (*it)->uiPid, (*it)->bcdDevice))
							{
								nsInfo = { 0 };
								pCurrentState->romInfo.pDeviceClass->RemoveUsbDevice(msg);
								continue;
							}
						}
					}

					if (nsInfo.Device)
					{
						nsInfo.Event = DEVICE_ARRIVAL_EVT;

						//Loop for Volume device as the same HubIndex and PortIndex;
						std::list<Device*>::iterator deviceIt;
						int portIndex;
						CString hubPath;
						BOOL bExceptionExist = FALSE;

						if (g_devClasses[DeviceClass::DeviceTypeMsc])
						{
							for (deviceIt = g_devClasses[DeviceClass::DeviceTypeMsc]->_devices.begin(); deviceIt != g_devClasses[DeviceClass::DeviceTypeMsc]->_devices.end(); ++deviceIt)
							{
								portIndex = (*deviceIt)->_hubIndex.get();
								hubPath = (*deviceIt)->_hub.get();
								if ((hubPath.CompareNoCase(nsInfo.Hub) == 0) && (portIndex == nsInfo.PortIndex)) //the same Hub-Port has two device
								{
									bExceptionExist = TRUE;
									break;
								}
							}
						}
						if (bExceptionExist)
						{
							LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_NORMAL_MSG, _T("DeviceArriveBeforVolumeRemove Exception occurs"));
							WaitForSingleObject(m_pExpectionHandler->m_hMapMsgMutex, INFINITE);
							mapMsg[msg] = 1;
							ReleaseMutex(m_pExpectionHandler->m_hMapMsgMutex);
							BSTR bstr_msg = msg.AllocSysString();
							m_pExpectionHandler->PostThreadMessage(WM_MSG_EXCEPTION_EVENT, (WPARAM)(CMyExceptionHandler::DeviceArriveBeforVolumeRemove), (LPARAM)bstr_msg);
							return;
						}

						LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_NORMAL_MSG, _T("DeviceManager::OnMsgDeviceEvent() - DEVICE_ARRIVAL_EVT, Notify"));
						Notify(&nsInfo);
					}
					else
					{
						if (pCurrentState->romInfo.pDeviceClass)
						{
							LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_NORMAL_MSG, _T("DeviceArriveButEnumFailed Exception occurs"));
							WaitForSingleObject(m_pExpectionHandler->m_hMapMsgMutex, INFINITE);
							mapMsg[msg] = 1;
							ReleaseMutex(m_pExpectionHandler->m_hMapMsgMutex);
							BSTR bstr_msg = msg.AllocSysString();
							m_pExpectionHandler->PostThreadMessage(WM_MSG_EXCEPTION_EVENT, (WPARAM)(CMyExceptionHandler::DeviceArriveButEnumFailed), (LPARAM)bstr_msg);
							return;
						}
					}
					break;
				}
			}
			break;
		} //end case DEVICE_ARRIVAL_EVT
		case DEVICE_REMOVAL_EVT:
		{
			TRACE(_T("Device manager device remove\r\n"));
			LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_NORMAL_MSG, _T("DeviceManager::OnMsgDeviceEvent() - DEVICE_REMOVAL_EVT(%s)"), msg);
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
				if ((*it)->romInfo.pDeviceClass && (*it)->romInfo.pDeviceClass->FindDeviceByUsbPath(msg.Mid(4), DeviceClass::DeviceListType_Current))
				{
					isRightDevice = TRUE;
					pCurrentState = (*it);
					break;
				}
			}

			if(isRightDevice) //OK, find the device
			{
				//check exception handle queue
				WaitForSingleObject(m_pExpectionHandler->m_hMapMsgMutex, INFINITE);
				std::map<CString, int>::iterator it = mapMsg.find(msg);
				if(it != mapMsg.end())
				{
					LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_NORMAL_MSG, _T("DeviceManager::OnMsgDeviceEvent() - DEVICE_REMOVAL_EVT, find not handled exception event, so don't to need handle"));
					mapMsg.erase(it);
					ReleaseMutex(m_pExpectionHandler->m_hMapMsgMutex);
					return;
				}
				ReleaseMutex(m_pExpectionHandler->m_hMapMsgMutex);

				DeviceClass::NotifyStruct nsInfo = {0};
			
				if (pCurrentState->romInfo.pDeviceClass)
				{
					nsInfo = pCurrentState->romInfo.pDeviceClass->RemoveUsbDevice(msg);
				}

				if(nsInfo.Device)
				{
					nsInfo.Event = DEVICE_REMOVAL_EVT;
					LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_NORMAL_MSG, _T("DeviceManager::OnMsgDeviceEvent() - DEVICE_REMOVAL_EVT, Notify"));
					Notify(&nsInfo);

					//DWORD dwResult = WaitForMultipleObjects(MAX_BOARD_NUMBERS, g_hDevCanDeleteEvts, FALSE, INFINITE);
					TRACE(_T("Device manager wait for begin\r\n"));
					DWORD dwResult = WaitForSingleObject(((MFGLIB_VARS *)m_pLibHandle)->g_hDevCanDeleteEvts[nsInfo.Device->GetDeviceWndIndex()], INFINITE);
					TRACE(_T("Device manager wait for end\r\n"));
					//int index = dwResult - WAIT_OBJECT_0;
					LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_NORMAL_MSG, _T("DeviceManager::OnMsgDeviceEvent()-DEVICE_REMOVAL_EVT, hDevCanDeleteEvent has been set"));
					pCurrentState->romInfo.pDeviceClass->RemoveDevice(nsInfo.Device);
				}
			}
			break;
		} //end case DEVICE_REMOVAL_EVT
		case VOLUME_ARRIVAL_EVT:
		{
			int msgLetterIndex;
			CString driveLetterStr;
            TRACE(_T("Device manager volume arrive %s\r\n"), msg);
			LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_NORMAL_MSG, _T("DeviceManager::OnMsgDeviceEvent() - VOLUME_ARRIVAL_EVT(%s)"), msg);
			//QueryPerformanceCounter(&g_t2);
			//double dTotalTime = (double)(g_t2.QuadPart-g_t1.QuadPart) / (double)g_tc.QuadPart;
			//LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_NORMAL_MSG, _T("DEVICE_ARRIVAL_EVT to VOLUME_ARRIVAL_EVT-Time: %f"), dTotalTime);
			if (!g_devClasses[DeviceClass::DeviceTypeDisk])
			{
				return;
			}
			g_devClasses[DeviceClass::DeviceTypeDisk]->Refresh();

			for ( msgLetterIndex = 0; msgLetterIndex < msg.GetLength(); ++msgLetterIndex )
			{
				driveLetterStr = msg.GetAt(msgLetterIndex);
				DeviceClass::NotifyStruct nsInfo = g_devClasses[DeviceClass::DeviceTypeMsc]->AddUsbDevice(driveLetterStr);
				LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_NORMAL_MSG, _T("DeviceManager::OnMsgDeviceEvent() - VOLUME_ARRIVAL_EVT-Disk(%s), Hub:%d-Port:%d"), driveLetterStr, nsInfo.HubIndex, nsInfo.PortIndex);
				if(nsInfo.Device)
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
			TRACE(_T("Device manager volume remove %s\r\n"), msg);
			LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_NORMAL_MSG, _T("DeviceManager::OnMsgDeviceEvent() - VOLUME_REMOVAL_EVT(%s)"), msg);

			if (!g_devClasses[DeviceClass::DeviceTypeMsc])
				return;
			for ( msgLetterIndex = 0; msgLetterIndex < msg.GetLength(); ++msgLetterIndex )
			{
				driveLetterStr = msg.GetAt(msgLetterIndex);
				DeviceClass::NotifyStruct nsInfo = g_devClasses[DeviceClass::DeviceTypeMsc]->RemoveUsbDevice(driveLetterStr);
				LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_NORMAL_MSG, _T("DeviceManager::OnMsgDeviceEvent() - VOLUME_REMOVAL_EVT-Disk(%s), Hub:%d-Port:%d"), driveLetterStr, nsInfo.HubIndex, nsInfo.PortIndex);
				if(nsInfo.Device)
				{
					nsInfo.Event = VOLUME_REMOVAL_EVT;
					LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_NORMAL_MSG, _T("DeviceManager::OnMsgDeviceEvent() - VOLUME_REMOVAL_EVT, Notify"));
					Notify(&nsInfo);
				}
				else
				{
					return;
				}

				if(nsInfo.PortIndex == 0)
				{
					// at such case, the cmdoperation thread can't get the volume remove event at all
					// we need to call cmdoperation's call back
					nsInfo.Event = VOLUME_REMOVAL_EVT;
					Notify(&nsInfo, nsInfo.Device->GetDeviceWndIndex());
				}

				TRACE(_T("Device manager wait for begin %d\r\n"),nsInfo.Device->GetDeviceWndIndex());
				LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_NORMAL_MSG, _T("DeviceManager::OnMsgDeviceEvent()-VOLUME_REMOVAL_EVT, wait hDevCanDeleteEvent"));
				DWORD dwResult = WaitForSingleObject(((MFGLIB_VARS *)m_pLibHandle)->g_hDevCanDeleteEvts[nsInfo.Device->GetDeviceWndIndex()], INFINITE);
				TRACE(_T("Device manager wait for end\r\n"));
				LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_NORMAL_MSG, _T("DeviceManager::OnMsgDeviceEvent()-VOLUME_REMOVAL_EVT, hDevCanDeleteEvent has been set"));
				
				g_devClasses[DeviceClass::DeviceTypeDisk]->RemoveDevice((dynamic_cast<Volume*>(nsInfo.Device))->StorageDisk());
				g_devClasses[DeviceClass::DeviceTypeMsc]->RemoveDevice(nsInfo.Device);
			}
			break;
		} //end case VOLUME_REMOVAL_EVT
		case HUB_ARRIVAL_EVT:
			{
				DeviceClass::NotifyStruct nsInfo = g_devClasses[DeviceClass::DeviceTypeUsbHub]->AddUsbDevice(msg);
				LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_NORMAL_MSG, _T("DeviceManager::OnMsgDeviceEvent() - HUB_ARRIVAL_EVT"));
				if ( nsInfo.Device )
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
				if ( nsInfo.Device )
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
	WaitForSingleObject(m_hMutex_cb, INFINITE);
	m_callbacks[pCallback] = pCallback;
	ReleaseMutex(m_hMutex_cb);
	return pCallback;
}

void DeviceManager::UnregisterCallback(HANDLE_CALLBACK hCallback)
{
	if(hCallback)
	{
		WaitForSingleObject(m_hMutex_cb, INFINITE);
		delete (CBStruct *)hCallback;
		m_callbacks.erase(hCallback);
		ReleaseMutex(m_hMutex_cb);
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
		WaitForSingleObject(m_hMutex_cb, INFINITE);
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
		ReleaseMutex(m_hMutex_cb);
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


