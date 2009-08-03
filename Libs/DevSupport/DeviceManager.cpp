#include "DeviceManager.h"
#include "HidDeviceClass.h"
#include "RecoveryDeviceClass.h"
#include "VolumeDeviceClass.h"
#include "MtpDeviceClass.h"
#include "UsbControllerMgr.h"
#include "UsbHubMgr.h"
//#include "UsbDeviceMgr.h"

#include <mswmdm_i.c>

//////////////////////////////////////////////////////////////////////
//
// DeviceManager class implementation
//
//////////////////////////////////////////////////////////////////////
IMPLEMENT_DYNCREATE(DeviceManager, CWinThread)

DeviceManager::DeviceManager()
: _pWMDevMgr3(NULL)
, _hUsbDev(NULL)
, _hUsbHub(NULL)
, _dwWMDMNotificationCookie(-1)
, _bStopped(true)
, _hChangeEvent(NULL)
, _hStartEvent(NULL)

{
	ATLTRACE(_T("  +DeviceManager::DeviceManager()\n"));
	// SingletonHolder likes it better if this thing is around
	// when it calls delete.
	m_bAutoDelete = FALSE;

	// Create the DeviceClasses
	_devClasses[DeviceClass::DeviceTypeRecovery]      = new RecoveryDeviceClass;
	_devClasses[DeviceClass::DeviceTypeMsc]		      = new VolumeDeviceClass;
	_devClasses[DeviceClass::DeviceTypeMtp]		      = new MtpDeviceClass;
	_devClasses[DeviceClass::DeviceTypeHid]		      = new HidDeviceClass;
	_devClasses[DeviceClass::DeviceTypeUsbController] = new usb::ControllerMgr;
	_devClasses[DeviceClass::DeviceTypeUsbHub]        = new usb::HubMgr;
//	_devClasses[DeviceClass::DeviceTypeUsbDevice]     = new usb::DeviceMgr;
	if ( _devClasses.size() != 6/*7*/ )
	{
		ATLTRACE(" *** FAILED TO CREATE ALL DEVICECLASSES.\n");
	}
}

DeviceManager::~DeviceManager()
{
	ATLTRACE(_T("  +DeviceManager::~DeviceManager()\n"));

	// Client has to call gDeviceManager::Instance().Close() before it exits;
	ASSERT ( _bStopped == true );

	ATLTRACE(_T("  -DeviceManager::~DeviceManager()\n"));
}

// Runs in the context of the Client thread.
DeviceClass* DeviceManager::operator[](DeviceClass::DeviceType devType)
{
	ASSERT ( (size_t)devType < _devClasses.size() );
	return _devClasses[devType];
}

// Runs in the context of the Client thread.
uint32_t DeviceManager::Open()
{
	ATLTRACE(_T("+DeviceManager::Open()\n"));
	// Client can not call gDeviceManager::Instance().Open() more than once.
	ASSERT ( _bStopped == true );
	
	// Create an event that we can wait on so we will know when
    // the DeviceManager thread has completed InitInstance()
    _hStartEvent = ::CreateEvent(NULL, TRUE, FALSE, NULL);
	
    // Create the user-interface thread supporting messaging
	uint32_t error;
    if ( CreateThread() != 0 ) // success
    {
	    // Hold the client thread until the DeviceManager thread is running
		VERIFY(::WaitForSingleObject(_hStartEvent, INFINITE) == WAIT_OBJECT_0);

        // Set flag to running. Flag set to stopped in constructor and in Close()
	    _bStopped = false;

        error = ERROR_SUCCESS;
        ATLTRACE(" Created thread(%#x, %d).\n", m_nThreadID, m_nThreadID);
    }
	else
    {
        error = GetLastError();
        ATLTRACE(" *** FAILED TO CREATE THREAD.\n");
    }

	// clean up
    ::CloseHandle(_hStartEvent);
    _hStartEvent = NULL;

	ATLTRACE(_T("-DeviceManager::Open()\n"));
	return error;
}

// Runs in the context of the Client thread
void DeviceManager::Close()
{
	ATLTRACE(_T("+DeviceManager::Close()\n"));
	// Client has to call gDeviceManager::Instance().Open() before it 
    // can call gDeviceManager::Instance().Close();
	ASSERT ( _bStopped == false );

    // Post a KILL event to the DeviceManager thread
	PostThreadMessage(WM_MSG_DEV_EVENT, EVENT_KILL, 0);
	// Wait for the DeviceManager thread to die before returning
	WaitForSingleObject(m_hThread, INFINITE);

	// Bad things happen in ~DeviceManager() if ExitInstance() has not been called,
	// so the destuctor asserts if the client did not call DeviceManager::Close().
	_bStopped = true;
	m_hThread = NULL;
    ATLTRACE(_T("-DeviceManager::Close()\n"));
}

// Runs in the context of the DeviceManager thread
BOOL DeviceManager::InitInstance()
{
	ATLTRACE(_T("  +DeviceManager::InitInstance()\n"));
    // Create a hidden window and register it to receive WM_DEVICECHANGE messages
	if( _DevChangeWnd.CreateEx(WS_EX_TOPMOST, _T("STATIC"), _T("DeviceChangeWnd"), 0, CRect(0,0,5,5), NULL, 0) )
    {
		DEV_BROADCAST_DEVICEINTERFACE broadcastInterface;
		
		broadcastInterface.dbcc_size = sizeof(DEV_BROADCAST_DEVICEINTERFACE);
		broadcastInterface.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
	    
		memcpy(&(broadcastInterface.dbcc_classguid),&(GUID_DEVINTERFACE_USB_DEVICE),sizeof(struct _GUID));
		
		// register for usb devices
		_hUsbDev=RegisterDeviceNotification(_DevChangeWnd.GetSafeHwnd(),&broadcastInterface,
											DEVICE_NOTIFY_WINDOW_HANDLE);
	    
		memcpy(&(broadcastInterface.dbcc_classguid),&(GUID_DEVINTERFACE_USB_HUB),sizeof(struct _GUID));
		
		// register for usb hubs
		_hUsbHub=RegisterDeviceNotification(_DevChangeWnd.GetSafeHwnd(),&broadcastInterface,
											DEVICE_NOTIFY_WINDOW_HANDLE);
    }
    else
    {
        ATLTRACE(_T(" *** FAILED TO CREATE WINDOW FOR WM_DEVCHANGE NOTIFICATIONS.\n"));
    }
    
	// WMDM Notification
	HRESULT hr = InitMTPDevMgr();
	if ( _pWMDevMgr3 )
	{
		hr = RegisterWMDMNotification();
	}

	// Init all the Device Classes
	Devices();
	// Init all the USB Ports.
	((usb::HubMgr*)_devClasses[DeviceClass::DeviceTypeUsbHub])->RefreshHubs();

	// Let the client thread that called Open() resume.
	VERIFY(::SetEvent(_hStartEvent));

    ATLTRACE(_T("  -DeviceManager::InitInstance()\n"));
	return (TRUE);
}

// Runs in the context of the DeviceManager thread
int DeviceManager::ExitInstance()
{
	ATLTRACE(_T(" +DeviceManager::ExitInstance()\n"));

    std::map<uint32_t, DeviceClass*>::iterator devClass;
	for (devClass=_devClasses.begin(); devClass!=_devClasses.end(); ++devClass)
	{
		if ( (*devClass).second != NULL )
		{
			delete (*devClass).second;
			(*devClass).second = NULL;
		}
	}
	_devClasses.clear();

	// Messaging support
    if ( _hUsbDev != INVALID_HANDLE_VALUE )
        UnregisterDeviceNotification(_hUsbDev);
	if ( _hUsbHub != INVALID_HANDLE_VALUE )
        UnregisterDeviceNotification(_hUsbHub);
    
    // Windows must be Destroyed in the thread in which they were created.
    // _DevChangeWnd was created in the DeviceManager thread in InitInstance().
    BOOL ret = _DevChangeWnd.DestroyWindow();

    if ( _pWMDevMgr3 )
	{
	    UnregisterWMDMNotification();
		_pWMDevMgr3->Release();
		_pWMDevMgr3 = NULL;
	}

	// Clean up the AutoPlay object
	SetCancelAutoPlay(false);

	std::map<HANDLE, Observer*>::iterator callback;
	for ( callback=_callbacks.begin(); callback!=_callbacks.end(); ++callback )
	{
		delete (Observer*)(*callback).first;
	}
	_callbacks.clear();

	ATLTRACE(_T(" -DeviceManager::ExitInstance()\n"));
    return CWinThread::ExitInstance();;
}

// Runs in the context of the Client thread.
HANDLE DeviceManager::Register(DeviceChangeCallback notifyFn, DeviceChangeNotifyType notifyType, LPARAM pData)
{
	Observer* pWatcher = new Observer();
	pWatcher->NotifyType = notifyType;
	pWatcher->NotifyFn = notifyFn;

	switch ( notifyType )
	{
		case ByDeviceClass:
			pWatcher->DeviceType = (DeviceClass::DeviceType)pData;
			break;
		case AnyChange:
		case ByDevice:
		case ByPort:
		case HubChange:
		default:
			pWatcher->pDevice = (Device*)pData;
			break;
	}
	_callbacks[pWatcher] = pWatcher;
	return pWatcher;
};

// Runs in the context of the Client thread.
HANDLE DeviceManager::Register(const Observer& observer)
{
	Observer* pWatcher = new Observer(observer);
	_callbacks[pWatcher] = pWatcher;
	return pWatcher;
}

// Runs in the context of the Client thread.
bool DeviceManager::Unregister(HANDLE hObserver)
{
	delete (Observer*)hObserver;
	return _callbacks.erase(hObserver) == 1;
};

/// Calls DeviceClass::Refresh() for each DeviceClass held by DeviceManager[].
/// Runs in the context of the Client thread.
void DeviceManager::Refresh()
{
	std::map<uint32_t, DeviceClass*>::iterator deviceClass;
	for ( deviceClass = _devClasses.begin(); deviceClass != _devClasses.end(); ++deviceClass )
	{
		(*deviceClass).second->Refresh();
	}
}

// Runs in the context of the Client thread.
void DeviceManager::ClearFilters()
{
	std::map<uint32_t, DeviceClass*>::iterator deviceClass;
	for ( deviceClass = _devClasses.begin(); deviceClass != _devClasses.end(); ++deviceClass )
	{
		(*deviceClass).second->ClearFilters();
	}
}

// Runs in the context of the Client thread.
void DeviceManager::AddFilter(LPCTSTR vid, LPCTSTR pid, LPCTSTR instance)
{
	std::map<uint32_t, DeviceClass*>::iterator deviceClass;
	for ( deviceClass = _devClasses.begin(); deviceClass != _devClasses.end(); ++deviceClass )
	{
		(*deviceClass).second->AddFilter(vid,pid);
	}
}

// Runs in the context of the Client thread.
void DeviceManager::AddFilter(uint16_t vid, uint16_t pid)
{
	std::map<uint32_t, DeviceClass*>::iterator deviceClass;
	for ( deviceClass = _devClasses.begin(); deviceClass != _devClasses.end(); ++deviceClass )
	{
		(*deviceClass).second->AddFilter(vid,pid);
	}
}

// Runs in the context of the Client thread.
void DeviceManager::UpdateDeviceFilters(LPCTSTR usbVid, LPCTSTR usbPid)
{
	// Remove all device filters from the gDeviceManager.
	ClearFilters();

	// Add device filters to the gDeviceManager.
	// HID
	(*this)[DeviceClass::DeviceTypeHid]->AddFilter(0x066f, 0x3700);
	(*this)[DeviceClass::DeviceTypeHid]->AddFilter(0x066f, 0x3770);
	(*this)[DeviceClass::DeviceTypeHid]->AddFilter(0x066f, 0x3780);
	(*this)[DeviceClass::DeviceTypeHid]->AddFilter(usbVid, usbPid);
	// MTP
	(*this)[DeviceClass::DeviceTypeMtp]->AddFilter(usbVid, usbPid);
	// MSC
	(*this)[DeviceClass::DeviceTypeMsc]->AddFilter(0x066f, 0x0000); // stmfgmsc.sb
	(*this)[DeviceClass::DeviceTypeMsc]->AddFilter(0x066f, 0xA000); // updater.sb
	(*this)[DeviceClass::DeviceTypeMsc]->AddFilter(0x066f, 0x37FF); // updater.sb
	(*this)[DeviceClass::DeviceTypeMsc]->AddFilter(usbVid, usbPid);

	// Rebuild the DeviceManager Device Lists
	Refresh();

}

// Runs in the context of the Client thread.
std::list<Device*> DeviceManager::Devices()
{
	std::list<Device*> masterList;

	std::map<uint32_t, DeviceClass*>::iterator deviceClass;
	for ( deviceClass = _devClasses.begin(); deviceClass != _devClasses.end(); ++deviceClass )
	{
		std::list<Device*> classList = (*deviceClass).second->Devices();
		
		std::list<Device*>::iterator device;
		for ( device = classList.begin(); device != classList.end(); ++device )
		{
			masterList.push_back(*device);
		}
	}

	return masterList;
}

// Runs in the context of the Client thread.
Device* DeviceManager::FindDevice(LPCTSTR paramStr)
{
    ATLTRACE(_T("+DeviceManager::FindDevice()\n"));

    //Parameter=Type:Hid,Vid:0x066F,Pid:0x1234,TimeOut:10
	Parameter::ParamMap localParams;
	ParameterT<uint32_t> deviceType;
	ParameterT<uint16_t> usbVid;
	ParameterT<uint16_t> usbPid;
	ParameterT<int32_t> waitTimeOut;

	//;param Type:Msc, Mtp, Recovery, Hid
	deviceType.ValueList[DeviceClass::DeviceTypeRecovery]		= L"Recovery";
	deviceType.ValueList[DeviceClass::DeviceTypeMsc]			= L"Msc";
	deviceType.ValueList[DeviceClass::DeviceTypeMtp]			= L"Mtp";
	deviceType.ValueList[DeviceClass::DeviceTypeHid]			= L"Hid";

	localParams[L"Type:"]		= &deviceType;
	localParams[L"Vid:"]		= &usbVid;
	localParams[L"Pid:"]		= &usbPid;
	localParams[L"TimeOut:"]	= &waitTimeOut;

	deviceType.Value = deviceType.Default = DeviceClass::DeviceTypeNone;
	usbVid.Value = usbVid.Default = 0;
	usbPid.Value = usbPid.Default = 0;
	waitTimeOut.Value = waitTimeOut.Default = 0;

	int32_t ret = ParseParameterString(paramStr, localParams);
	
    if ( deviceType.Value == deviceType.Default )
    {
        ATLTRACE(_T(" ERROR()%s(%d) : Invalid 'DeviceType'.\n"), __TFILE__, __LINE__);
        ATLTRACE(_T("-DeviceManager::FindDevice()\n"));
		return NULL;
    }

	// get the pointer to the right DeviceClass
	DeviceClass * pDevClass = _devClasses[deviceType.Value];
	if ( pDevClass == NULL )
    {
        ATLTRACE(_T(" ERROR()%s(%d) : Invalid 'DeviceClass'.\n"), __TFILE__, __LINE__);
        ATLTRACE(_T("-DeviceManager::FindDevice()\n"));
		return NULL;
    }

	// Clear the Filter for the DeviceClass
	pDevClass->ClearFilters();

	// Add the Filter to the DeviceClass
	pDevClass->AddFilter(usbVid.Value, usbPid.Value);

	// Refresh the DeviceClass
	pDevClass->Refresh();

	//
    // Return the first Device in the list if there is a list
    //
    if ( !pDevClass->Devices().empty() )
    {
        ATLTRACE(_T("-DeviceManager::FindDevice() : Found device.\n"));
	    return pDevClass->Devices().front();
    }

    //
    // We didn't find a device on our first pass
    //
    // Just return right away if there was no wait period specified
    if ( waitTimeOut.Value == 0 )
    {
        ATLTRACE(_T("-DeviceManager::FindDevice() : No device, not waiting.\n"));
        return NULL;
    }

    //
    // Since a waitPeriod was specified, we need to hang out and see if a device arrives before the 
    // waitPeriod expires.
    //
 	// Create an event that we can wait on so we will know if
    // the DeviceManager got a device arrival
    _hChangeEvent = ::CreateEvent(NULL, TRUE, FALSE, NULL);

    HANDLE hTimer = CreateWaitableTimer(NULL, true, _T("FindDeviceTimer"));
    LARGE_INTEGER waitTime;
    waitTime.QuadPart = waitTimeOut.Value * (-10000000);
    SetWaitableTimer(hTimer, &waitTime, 0, NULL, NULL, false);

    HANDLE waitHandles[2] = { _hChangeEvent, hTimer };
    DWORD waitResult;
    while( pDevClass->Devices().empty() )
    {

        waitResult = MsgWaitForMultipleObjects(2, &waitHandles[0], false, INFINITE, 0);
        if ( waitResult == WAIT_OBJECT_0 )
        {
            // Device Change Event
           	VERIFY(::ResetEvent(_hChangeEvent));
            continue;
        }
        else if ( waitResult == WAIT_OBJECT_0 + 1 )
        {
            // Timeout
            break;
        }
        else
        {
            // unreachable, but catch it just in case.
            ASSERT(0);
        }
    }
    // clean up
    CloseHandle(_hChangeEvent);
    _hChangeEvent = NULL;
    
    ATLTRACE(_T("-DeviceManager::FindDevice() : %s.\n"), pDevClass->Devices().empty() ? _T("No device") : _T("Found device"));
    return pDevClass->Devices().empty() ? NULL : pDevClass->Devices().front();
}

BEGIN_MESSAGE_MAP(DeviceManager::DevChangeWnd, CWnd)
    ON_WM_DEVICECHANGE()
END_MESSAGE_MAP()

// Runs in the context of the DeviceManager thread
BOOL DeviceManager::DevChangeWnd::OnDeviceChange(UINT nEventType,DWORD_PTR dwData)
{
	CStdString MsgStr;
	uint32_t event = UNKNOWN_EVT;
    PDEV_BROADCAST_HDR lpdb = (PDEV_BROADCAST_HDR)dwData;

	if (lpdb) 
	{
		switch(nEventType) 
		{
			case DBT_DEVICEARRIVAL: 
				if (lpdb->dbch_devicetype == DBT_DEVTYP_DEVICEINTERFACE) 
				{
					if(((PDEV_BROADCAST_DEVICEINTERFACE)lpdb)->dbcc_classguid == GUID_DEVINTERFACE_USB_DEVICE) 
					{
						MsgStr = (LPCTSTR)((PDEV_BROADCAST_DEVICEINTERFACE)lpdb)->dbcc_name;
						event = DEVICE_ARRIVAL_EVT;
					}
					else if(((PDEV_BROADCAST_DEVICEINTERFACE)lpdb)->dbcc_classguid == GUID_DEVINTERFACE_USB_HUB) 
					{
						MsgStr = (LPCTSTR)((PDEV_BROADCAST_DEVICEINTERFACE)lpdb)->dbcc_name;
						event = HUB_ARRIVAL_EVT;
					}
				}
				else if(lpdb->dbch_devicetype == DBT_DEVTYP_VOLUME) 
				{
					MsgStr = DrivesFromMask( ((PDEV_BROADCAST_VOLUME)lpdb)->dbcv_unitmask );
					event = VOLUME_ARRIVAL_EVT;
					ATLTRACE(_T("DeviceManager::DevChangeWnd::OnDeviceChange() - VOLUME_ARRIVAL_EVT(%s)\n"), MsgStr.c_str());
				}
				break;
			case DBT_DEVICEREMOVECOMPLETE:
				if(lpdb->dbch_devicetype == DBT_DEVTYP_DEVICEINTERFACE) 
				{
					if(((PDEV_BROADCAST_DEVICEINTERFACE)lpdb)->dbcc_classguid == GUID_DEVINTERFACE_USB_DEVICE) 
					{
						MsgStr = (LPCTSTR)((PDEV_BROADCAST_DEVICEINTERFACE)lpdb)->dbcc_name;
						event = DEVICE_REMOVAL_EVT;
					}
					else if(((PDEV_BROADCAST_DEVICEINTERFACE)lpdb)->dbcc_classguid == GUID_DEVINTERFACE_USB_HUB) 
					{
						MsgStr = (LPCTSTR)((PDEV_BROADCAST_DEVICEINTERFACE)lpdb)->dbcc_name;
						event = HUB_REMOVAL_EVT;
					}
				}
				else if(lpdb->dbch_devicetype == DBT_DEVTYP_VOLUME) 
				{
					MsgStr = DrivesFromMask( ((PDEV_BROADCAST_VOLUME)lpdb)->dbcv_unitmask );
					event = VOLUME_REMOVAL_EVT;
					ATLTRACE(_T("DeviceManager::DevChangeWnd::OnDeviceChange() - VOLUME_REMOVAL_EVT(%s)\n"), MsgStr.c_str());
				}
				break;
			default:
				ASSERT(0);
		} // end switch (nEventType)

		// let's figure out what to do with the WM_DEVICECHANGE message
		// after we get out of this loop so we don't miss any messages.
		BSTR bstr_msg = MsgStr.AllocSysString();
		gDeviceManager::Instance().PostThreadMessage(WM_MSG_DEV_EVENT, (WPARAM)event, (LPARAM)bstr_msg);

	} // end if (lpdb)

	return TRUE;
}

// worker function for OnDeviceChange() to get drive letters from the bitmask
// Runs in the context of the DeviceManager thread
CStdString DeviceManager::DevChangeWnd::DrivesFromMask(ULONG UnitMask)
{
	CStdString Drive;
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
	ON_THREAD_MESSAGE(DeviceManager::WM_MSG_DEV_EVENT, OnMsgDeviceEvent)
END_MESSAGE_MAP()

// Runs in the context of the DeviceManager thread.
void DeviceManager::OnMsgDeviceEvent(WPARAM eventType, LPARAM desc)
{
	CStdString msg = (LPCTSTR)desc;
	SysFreeString((BSTR)desc);
	if ( eventType == EVENT_KILL ) 
	{
		PostQuitMessage(0);
		return;
    }

//	DeviceClass::NotifyStruct nsInfo = {0};
	
	switch ( eventType )
	{
		case DEVICE_ARRIVAL_EVT:
		{
			DeviceClass::NotifyStruct nsInfo = AddUsbDevice(msg);
			if ( nsInfo.Device )
			{
				nsInfo.Event = DEVICE_ARRIVAL_EVT;
				Notify(nsInfo);
			}
			break;
		}
		case DEVICE_REMOVAL_EVT:
		{
			DeviceClass::NotifyStruct nsInfo = RemoveUsbDevice(msg);
			if ( nsInfo.Device )
			{
				nsInfo.Event = DEVICE_REMOVAL_EVT;
				Notify(nsInfo);
			}
			break;
		}
		case VOLUME_ARRIVAL_EVT:
		{
			ATLTRACE(_T("DeviceManager::OnMsgDeviceEvent() - VOLUME_ARRIVAL_EVT(%s)\n"), msg.c_str());
			int msgLetterIndex;
			CStdString driveLetterStr;
			for ( msgLetterIndex = 0; msgLetterIndex < msg.GetLength(); ++msgLetterIndex )
			{
				driveLetterStr = msg.GetAt(msgLetterIndex);
				
				DeviceClass::NotifyStruct nsInfo = (*this)[DeviceClass::DeviceTypeMsc]->AddUsbDevice(driveLetterStr);
				if ( nsInfo.Device )
				{
					nsInfo.Event = VOLUME_ARRIVAL_EVT;
					Notify(nsInfo);
				}
			}
			break;
		}
		case VOLUME_REMOVAL_EVT:
		{
			ATLTRACE(_T("DeviceManager::OnMsgDeviceEvent() - VOLUME_REMOVAL_EVT(%s)\n"), msg.c_str());
			int msgLetterIndex;
			CStdString driveLetterStr;
			for ( msgLetterIndex = 0; msgLetterIndex < msg.GetLength(); ++msgLetterIndex )
			{
				driveLetterStr = msg.GetAt(msgLetterIndex);
				
				DeviceClass::NotifyStruct nsInfo = (*this)[DeviceClass::DeviceTypeMsc]->RemoveUsbDevice(driveLetterStr);
				if ( nsInfo.Device )
				{
					nsInfo.Event = VOLUME_REMOVAL_EVT;
					Notify(nsInfo);
				}
			}
			break;
		}
		case HUB_ARRIVAL_EVT:
		{
			DeviceClass::NotifyStruct nsInfo = (*this)[DeviceClass::DeviceTypeUsbHub]->AddUsbDevice(msg);
			if ( nsInfo.Device )
			{
				nsInfo.Event = HUB_ARRIVAL_EVT;
				Notify(nsInfo);
			}
			break;
		}
		case HUB_REMOVAL_EVT:
		{
			DeviceClass::NotifyStruct nsInfo = (*this)[DeviceClass::DeviceTypeUsbHub]->RemoveUsbDevice(msg);
			if ( nsInfo.Device )
			{
				nsInfo.Event = HUB_REMOVAL_EVT;
				Notify(nsInfo);
			}
			break;
		}
		default:
		{
			assert(0);
		}
	} // end switch (eventType)
}

// Runs in the context of the DeviceManager thread
void DeviceManager::Notify(const DeviceClass::NotifyStruct& nsInfo)
{
	// DO NOT DEREFERENCE nsInfo.Device

	// Signal anyone waiting on a device change to go see what changed. See FindDevice() for example.
    if (_hChangeEvent)
	{
		VERIFY(::SetEvent(_hChangeEvent));
	}

	std::vector<HANDLE> eraseCallbackList;

	Observer* pWatcher;
	std::map<HANDLE, Observer*>::iterator callback;
	for ( callback=_callbacks.begin(); callback!=_callbacks.end(); ++callback )
	{
		bool doNotify = false;
		
		pWatcher = (*callback).second;
		
		switch ( pWatcher->NotifyType )
		{
			case AnyChange:
				doNotify = true;
				break;
			case ByDevice:
				if ( pWatcher->pDevice == nsInfo.Device )
					doNotify = true;
				break;
			case ByDeviceClass:
				if ( pWatcher->DeviceType == nsInfo.Type )
					doNotify = true;
				break;
			case ByPort:
			{
				// if the "watched for Hub" is no longer present then do the notify
				usb::HubMgr* pHubMgr = dynamic_cast<usb::HubMgr*>(gDeviceManager::Instance()[DeviceClass::DeviceTypeUsbHub]);
				if ( pHubMgr->FindHubByPath(pWatcher->Hub) == NULL )
				{
					doNotify = true;
				}
				// else if our port changed, do the notify
				else if ( pWatcher->HubIndex == nsInfo.HubIndex )
				{
					if ( pWatcher->Hub.CompareNoCase(nsInfo.Hub) == 0 )
					{
						 doNotify = true;
					}
				}			
//				int32_t hubIndex = pWatcher->HubIndex;
//				if ( hubIndex && hubIndex == nsInfo.HubIndex )
//				{
//					if ( pWatcher->Hub.CompareNoCase(nsInfo.Hub) == 0 )
//						doNotify = true;
//				}
				break;
			}
			case HubChange:
				//TODO: implement this
				ATLTRACE(_T("*** ERROR: DeviceManager::Notify() - Implement HubChange(%d)\n"), pWatcher->NotifyType);
				doNotify = true;
				break;
			default:
				ATLTRACE(_T("*** ERROR: DeviceManager::Notify() - Unknown NotifyType(%d)\n"), pWatcher->NotifyType);
				break;
		}
		if ( doNotify )
		{
			if ( pWatcher->NotifyFn(nsInfo) == retUnregisterCallback )
			{
				eraseCallbackList.push_back((*callback).first);
			}
		}
	
	} // end for ( all registered callbacks )

	while ( !eraseCallbackList.empty() )
	{
		HANDLE hCallback = eraseCallbackList.back();
		Unregister(hCallback);
		eraseCallbackList.pop_back();
	}
};

// Runs in the context of the DeviceManager thread
DeviceClass::NotifyStruct DeviceManager::AddUsbDevice(LPCTSTR path)
{
	DeviceClass::NotifyStruct nsInfo = {0};

	std::map<uint32_t, DeviceClass*>::iterator deviceClass;
	for ( deviceClass = _devClasses.begin(); deviceClass != _devClasses.end(); ++deviceClass )
	{
		// don't look for USB arrival in MSC class, wait for Volume arrival.
		if ( (*deviceClass).second->GetDeviceType() != DeviceClass::DeviceTypeMsc )
		{
			nsInfo = (*deviceClass).second->AddUsbDevice(path);
			if ( nsInfo.Device )
				break;
		}
	}

	return nsInfo;
}

// Runs in the context of the DeviceManager thread
DeviceClass::NotifyStruct DeviceManager::RemoveUsbDevice(LPCTSTR path)
{
	DeviceClass::NotifyStruct nsInfo = {0};

	std::map<uint32_t, DeviceClass*>::iterator deviceClass;
	for ( deviceClass = _devClasses.begin(); deviceClass != _devClasses.end(); ++deviceClass )
	{
		// don't look for USB removal in MSC class, should have already happened by Volume removal.
		if ( (*deviceClass).second->GetDeviceType() != DeviceClass::DeviceTypeMsc )
		{
			nsInfo = (*deviceClass).second->RemoveUsbDevice(path);
			if ( nsInfo.Device )
				break;
		}
	}

	return nsInfo;
}

STDMETHODIMP DeviceManager::CWMDMNotification::WMDMMessage(DWORD  dwMessageType, LPCWSTR  pwszCanonicalName)
{
	CString MsgStr = pwszCanonicalName;
	DevChangeEvent DevChangeEvent;
    
    switch ( dwMessageType ) {
    case WMDM_MSG_DEVICE_ARRIVAL:
        DevChangeEvent = WMDM_DEVICE_ARRIVAL_EVT;
        break;
    case WMDM_MSG_DEVICE_REMOVAL:
        DevChangeEvent = WMDM_DEVICE_REMOVAL_EVT;
        break;
    case WMDM_MSG_MEDIA_ARRIVAL:
        DevChangeEvent = WMDM_MEDIA_ARRIVAL_EVT;
        break;
    case WMDM_MSG_MEDIA_REMOVAL:
        DevChangeEvent = WMDM_MEDIA_REMOVAL_EVT;
        break;
    default:
        DevChangeEvent = UNKNOWN_EVT;
        break;
    }

    return S_OK;
};

HRESULT DeviceManager::RegisterWMDMNotification()
{
	ATLTRACE(_T("   +DeviceManager::RegisterWMDMNotification() : _pWMDevMgr=0x%x\n"), _pWMDevMgr3);
    ASSERT(_pWMDevMgr3);
    HRESULT hr = S_OK;

	CComPtr<IConnectionPointContainer> spICPC;
    CComPtr<IConnectionPoint> spICP;

    if (SUCCEEDED (hr = _pWMDevMgr3->QueryInterface(IID_IConnectionPointContainer, (void**) & spICPC)))
    {
        if (SUCCEEDED (hr = spICPC->FindConnectionPoint(IID_IWMDMNotification, &spICP)))
        {
            DWORD dwCookie;
			if (SUCCEEDED (hr = spICP->Advise(&_IWMDMCallbackObject, &dwCookie)))
			{
				_dwWMDMNotificationCookie = dwCookie;
			}
		}
    }

	ATLTRACE(_T("   -DeviceManager::RegisterWMDMNotification() : result=0x%x\n"), hr);
    return hr;
}

HRESULT DeviceManager::UnregisterWMDMNotification(void)
{
    ASSERT(_pWMDevMgr3);
    HRESULT hr = E_UNEXPECTED;

    if (-1 != _dwWMDMNotificationCookie)
    {
        CComPtr<IConnectionPointContainer> spICPC;
        CComPtr<IConnectionPoint> spICP;

        if (SUCCEEDED (hr = _pWMDevMgr3->QueryInterface(IID_IConnectionPointContainer, (void**) & spICPC)))
        {
            if (SUCCEEDED (hr = spICPC->FindConnectionPoint(IID_IWMDMNotification, &spICP)))
            {
                if (SUCCEEDED (hr = spICP->Unadvise(_dwWMDMNotificationCookie)))
                {
                    _dwWMDMNotificationCookie = -1;
                    hr = S_OK;
                }
            }
        }
    }

    return hr;
}
#include "stmtpapi.h"
HRESULT DeviceManager::InitMTPDevMgr()
{
	ATLTRACE(_T("   +DeviceManager::InitMTPDevMgr()\n"));
    HRESULT hr;
    DWORD dwCount;
    DWORD* pdwProt;  // This will always be SAC_PROTOCOL_V1.
    BYTE abPVK[] = { 0x00 };
    BYTE abCert[] = { 0x00 };
    CSecureChannelClient SAC;
    CComPtr<IComponentAuthenticate> pAuth = NULL;

    hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);

	hr = CoCreateInstance(CLSID_MediaDevMgr, NULL, CLSCTX_ALL,
                IID_IComponentAuthenticate, (void **)&pAuth);

    // After getting IComponentAuthenticate, the authentication progression follows.
    if SUCCEEDED(hr)
    {
        hr = SAC.SetCertificate(SAC_CERT_V1, (BYTE*) abCert, sizeof(abCert), (BYTE*) abPVK, sizeof(abPVK));
        if SUCCEEDED(hr)
        {
            // Set interface for Secure Authenticated Channel
            // operations. The return value of this function is void.

            SAC.SetInterface(pAuth);
            hr = pAuth->SACGetProtocols(&pdwProt, &dwCount);
            if SUCCEEDED(hr)
                hr = SAC.Authenticate(*pdwProt); //SAC_PROTOCOL_V1
            if SUCCEEDED(hr) 
                ATLTRACE(_T("    CSecureChannelClient.Authenticate succeeded\n"));
        }
        if(pdwProt)
		{
			CoTaskMemFree(pdwProt);
		}

		// After authentication has succeeded, call QueryInterface to 
        // get IID_IWMDeviceManager3.
		hr = pAuth->QueryInterface(IID_IWMDeviceManager3, (void**)&_pWMDevMgr3);
        if ( FAILED(hr) )
		{
            ATLTRACE(_T("    *** No IWMDeviceManager. No WMDM support for MTP devices.\n"));
        }
	}

	ATLTRACE(_T("   -DeviceManager::InitMTPDevMgr() : result=0x%x\n"), hr);
    return hr;
}
STDMETHODIMP DeviceManager::CQueryCancelAutoplay::AllowAutoPlay(LPCTSTR pszPath,
    DWORD dwContentType, LPCWSTR pszLabel, DWORD dwSerialNumber)
{
    HRESULT hr = S_OK;
	CStdString drv_path = pszPath;

	// Is it the drive we want to cancel Autoplay for?
	if(drv_path.FindOneOf(_driveList) != -1 ) {
		ATLTRACE("*** CAutoPlayReject: Rejected AutoPlay Drive: %s ***\n", pszPath);
        hr = S_FALSE;
    }

    return hr;
}

HRESULT DeviceManager::CQueryCancelAutoplay::SetCancelAutoPlay(bool rejectAutoPlay, LPCTSTR driveList)
{
	if ( rejectAutoPlay == false )
	{
		return Unregister();
	}

	if(driveList)
	{
		CStdString RejectDriveLetters = driveList;
		if ( !RejectDriveLetters.IsEmpty() )
			_driveList = RejectDriveLetters;
	}

	HKEY hKey;
	// add a registry key so our IQueryCancelAutoPlay will work
	if(RegCreateKey(HKEY_LOCAL_MACHINE,
		_T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\AutoplayHandlers\\CancelAutoplay\\CLSID"),
		&hKey) == ERROR_SUCCESS) 
	{
		LPTSTR Str; 
		CString  ClsId;

		StringFromCLSID(CLSID_CANCEL_AUTOPLAY, &Str);
		ClsId = Str;
		CoTaskMemFree(Str);
		ClsId.Remove(_T('{')); ClsId.Remove(_T('}'));

		if(RegSetValueEx( hKey, ClsId, 0, REG_SZ, 0, 0) != ERROR_SUCCESS ) 
			ATLTRACE(_T("*** DeviceManager::CQueryCancelAutoplay: [ERROR] Failed to create IQueryCancelAutoPlay CLSID. ***\n"));
			RegCloseKey(hKey);
	}
	else 
	{
		ATLTRACE(_T("*** DeviceManager::CQueryCancelAutoplay: [ERROR] Failed to create IQueryCancelAutoPlay CLSID.\n ***"));
	}

    // Create the moniker that we'll put in the ROT
    IMoniker* pmoniker;
    HRESULT hr = CreateClassMoniker(CLSID_CANCEL_AUTOPLAY, &pmoniker);
    
	if(SUCCEEDED(hr)) 
	{
        IRunningObjectTable* prot;
        hr = GetRunningObjectTable(0, &prot);
        if(SUCCEEDED(hr)) 
		{
			CQueryCancelAutoplay* pQCA = new CQueryCancelAutoplay();
            if(pQCA) 
			{
	            IUnknown* punk;
                hr = pQCA->QueryInterface(IID_IUnknown, (void**)&punk);
                if (SUCCEEDED(hr)) 
				{
                    // Register...
                    hr = prot->Register(ROTFLAGS_REGISTRATIONKEEPSALIVE, punk, pmoniker, &_ROT);
                    if(SUCCEEDED(hr)) 
					{
						_isRegistered = true;
						ATLTRACE(_T("*** DeviceManager::CQueryCancelAutoplay: Registered drives %s***\n"), _driveList.c_str());
                    }
					else 
						ATLTRACE(_T("*** DeviceManager::CQueryCancelAutoplay: [ERROR] Registration failed ***\n"));
                    punk->Release();
                }
				else 
					ATLTRACE(_T("*** DeviceManager::CQueryCancelAutoplay: [ERROR] Registration failed ***\n"));
                pQCA->Release();
            }
            else 
			{
                hr = E_OUTOFMEMORY;
				ATLTRACE(_T("*** DeviceManager::CQueryCancelAutoplay: [ERROR] Registration failed ***\n"));
            }
            prot->Release();
        }
		else 
			ATLTRACE(_T("*** DeviceManager::CQueryCancelAutoplay: [ERROR] Registration failed ***\n"));    
		pmoniker->Release();
    }
    return hr;
}

HRESULT DeviceManager::CQueryCancelAutoplay::Unregister()
{
    IRunningObjectTable *prot;

	if(_isRegistered)
	{
		if (SUCCEEDED(GetRunningObjectTable(0, &prot))) 
		{
			// Remove our instance from the ROT
			prot->Revoke(_ROT);
			prot->Release();
			_isRegistered = false;
			ATLTRACE(_T("*** DeviceManager::CQueryCancelAutoplay: Unregistered. ***\n"));
		}
		else
			return S_FALSE;
	}

    return S_OK;
}

///////////////////////////////////////////////////////////////////////////////
// COM boiler plate code...
//
///////////////////////////////////////////////////////////////////////////////
// CWMDMNotification
//
STDMETHODIMP DeviceManager::CWMDMNotification::QueryInterface(REFIID riid, void** ppv)
{
    IUnknown* punk = NULL;
    HRESULT hr = S_OK;

    if (IID_IUnknown == riid)
    {
        punk = static_cast<IUnknown*>(this);
        punk->AddRef();
    }
    else
    {
        if (IID_IWMDMNotification == riid)
        {
            punk = static_cast<IWMDMNotification*>(this);
            punk->AddRef();
        }
        else
        {
            hr = E_NOINTERFACE;
        }
    }

    *ppv = punk;

    return hr;
}

STDMETHODIMP_(ULONG) DeviceManager::CWMDMNotification::AddRef()
{
    return ::InterlockedIncrement((LONG*)&_cRef);
}

STDMETHODIMP_(ULONG) DeviceManager::CWMDMNotification::Release()
{
    ULONG cRef = ::InterlockedDecrement((LONG*)&_cRef);

    if(!cRef)
    {
        delete this;
    }

    return cRef;
}

///////////////////////////////////////////////////////////////////////////////
// CQueryCancelAutoplay
//
STDMETHODIMP DeviceManager::CQueryCancelAutoplay::QueryInterface(REFIID riid, void** ppv)
{
    IUnknown* punk = NULL;
    HRESULT hr = S_OK;

    if (IID_IUnknown == riid)
    {
        punk = static_cast<IUnknown*>(this);
        punk->AddRef();
    }
    else
    {
        if (IID_IQueryCancelAutoPlay == riid)
        {
            punk = static_cast<IQueryCancelAutoPlay*>(this);
            punk->AddRef();
        }
        else
        {
            hr = E_NOINTERFACE;
        }
    }

    *ppv = punk;

    return hr;
}

STDMETHODIMP_(ULONG) DeviceManager::CQueryCancelAutoplay::AddRef()
{
    return ::InterlockedIncrement((LONG*)&_cRef);
}

STDMETHODIMP_(ULONG) DeviceManager::CQueryCancelAutoplay::Release()
{
    ULONG cRef = ::InterlockedDecrement((LONG*)&_cRef);

    if(!cRef)
    {
        delete this;
    }

    return cRef;
}
