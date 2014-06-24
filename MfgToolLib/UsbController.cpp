/*
 * Copyright (C) 2009-2013, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */

#include "stdafx.h"
#include "UsbController.h"
#include "DeviceManager.h"
//#include "UsbHub.h"

#pragma warning( disable : 4200 )
#include <usbioctl.h>
#pragma warning( default : 4200 )

usb::Controller::Controller(DeviceClass * deviceClass, DEVINST devInst, CString path, INSTANCE_HANDLE handle)
: Device(deviceClass, devInst, path, handle)
{
	//LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_NORMAL_MSG, _T("new Controller"));

    Initialize();
}

usb::Controller::~Controller()
{
	//LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_NORMAL_MSG, _T("delete Controller"));
}

HANDLE usb::Controller::Open()
{
    SECURITY_ATTRIBUTES SecurityAttrib; // Needed for Win2000
    SecurityAttrib.bInheritHandle = false;
    SecurityAttrib.lpSecurityDescriptor = NULL;
    SecurityAttrib.nLength = sizeof(SECURITY_ATTRIBUTES);

    CString filePath = _path.get();
    filePath.Replace(_T("\\??"), _T("\\\\."));

    return CreateFile(
				filePath,
				/*GENERIC_WRITE*/0,
				/*FILE_SHARE_READ|FILE_SHARE_WRITE*/0,
				&SecurityAttrib, 
				OPEN_EXISTING, 0, NULL);
}

DWORD usb::Controller::Initialize()
{
    DWORD errorCode = ERROR_SUCCESS;

    // reset the member variable
    _rootHubFilename.put(_T(""));
    
    HANDLE hController = Open();
    if( hController == INVALID_HANDLE_VALUE )
    {
        errorCode = GetLastError();
        return errorCode;
    }

    BOOL success;
    DWORD bytesReturned;
    CString rootHubFilename = _T("\\\\.\\");
    struct 
	{
		DWORD Length; 
		wchar_t Name[MAX_PATH];
	} unicodeName;

    // Get the system name of our root hub for interrogation
    success = DeviceIoControl(hController, IOCTL_USB_GET_ROOT_HUB_NAME, &unicodeName,
                              sizeof(unicodeName),&unicodeName, sizeof(unicodeName), &bytesReturned, NULL); 

    CloseHandle(hController);

    if (!success)
    {
        errorCode=GetLastError();
        return errorCode;
    }

	rootHubFilename.Append(&unicodeName.Name[0]);

    // save the Root Hub Filename to our member variable
    _rootHubFilename.put(rootHubFilename);
    _rootHubFilename.describe(this, _T("Root Hub Filename"), _T("Filename used to talk to the Controller's root hub."));

    return ERROR_SUCCESS;
}


usb::Hub * usb::Controller::GetRootHub()
{
    usb::Hub* pHub = NULL;

    CString pathToFind = (LPCTSTR)_rootHubFilename.get() + 4;

    // Find our Hub in gDeviceManager's list of [Hub].Devices()
    std::list<Device*> HubList = g_devClasses[DeviceClass::DeviceTypeUsbHub]->Devices();
    std::list<Device*>::iterator hub;
    for ( hub = HubList.begin(); hub != HubList.end(); ++hub )
    {
        //if ( pathToFind.CompareNoCase( (*hub)->_usbPath.get() ) == 0 )
		if ( pathToFind.CompareNoCase( (*hub)->_path.get().GetBuffer() + 4 ) == 0 )
        {
            pHub = dynamic_cast<usb::Hub*>(*hub);
            break;
        }
    }

    return pHub;
}

