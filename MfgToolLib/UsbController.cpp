/*
 * Copyright 2009-2013, 2016 Freescale Semiconductor, Inc.
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
#include "UsbController.h"
#include "DeviceManager.h"
//#include "UsbHub.h"

#pragma warning( disable : 4200 )
#ifndef __linux__
#include <usbioctl.h>
#endif
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
#if 0
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
#endif
return NULL;
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
#if 0
    // Get the system name of our root hub for interrogation
    success = DeviceIoControl(hController, IOCTL_USB_GET_ROOT_HUB_NAME, &unicodeName,
                              sizeof(unicodeName),&unicodeName, sizeof(unicodeName), &bytesReturned, NULL);

    CloseHandle(hController);

    if (!success)
    {
        errorCode=GetLastError();
        return errorCode;
    }

	rootHubFilename.append(&unicodeName.Name[0]);

    // save the Root Hub Filename to our member variable
    _rootHubFilename.put(rootHubFilename);
    _rootHubFilename.describe(this, _T("Root Hub Filename"), _T("Filename used to talk to the Controller's root hub."));
#endif
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
