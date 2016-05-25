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


// UsbPort.h : header file
//
#ifndef _USBPORT_H
#define _USBPORT_H

#pragma once

//#pragma warning( disable : 4200 )
//#include <usbioctl.h>

//#pragma warning( default : 4200 )

#include "Property.h"
#include "Device.h"

class Device;

namespace usb
{
    class Hub;

    // usb::Port
    class Port : public Property
    {
    public:
        Port(Hub* pHub, int index);
        virtual ~Port();

        DWORD Refresh();
        void Clear();
        CString GetUsbDevicePath();
        CString GetDriverKeyName();
        CString GetDeviceDescription();
        CString GetDriveLetters();
        DWORD GetDeviceType();
        int GetIndex();
        int IsHub();
        int Connected();
        Device* GetDevice();
		Hub* GetParentHub();
        CString GetName();
		void SetWndIndex(DWORD dwIndex);

		//HANDLE m_Mtx;

    private:
        Hub* _parentHub;
        Int32Property _connected;
        Int32Property _isHub;
        Int32Property _index;
        Device* _device;
		DWORD _WndIndex;

        Hub* FindHub(LPCTSTR driverName);
        Device* FindDevice(LPCTSTR driverName);

        //USB_NODE_CONNECTION_INFORMATION_EX _connectionInfo;      ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /*
        // PROPERTIES
        class volumeName : public StringProperty { public: CString get(); }_volumeName;
        class logicalDrive : public StringProperty { public: CString get(); }_logicalDrive;

    //  int GetDriveCount(void);
        CString GetDevicePath(void);
        CString GetDriveLetters(void);
        CString GetDriverKeyName(void);
        CString GetDeviceDescription(void);

        DWORD Refresh(bool DriveLetterRefreshOnly = false);
        void GetPortData(HANDLE HubHandle);
    //  NODE_CONNECTION_INFORMATION GetConnectionInformation(void);

    protected:
        int m_iDriveCount;
        HANDLE m_hHubHandle;

        CString m_sDevicePath;
        CString m_sDriveLetter;
        CString m_sDriverKeyName;
        CString m_sDeviceDescription;

        HDEVINFO m_HardwareDeviceInfo;
        SP_DEVINFO_DATA m_DeviceInfoData;
    //  NODE_CONNECTION_INFORMATION m_ConnectionInformation;

        DWORD GetDeviceDescAndPath(void);
        USHORT GetDeviceDescriptor(USHORT LanguageID, PUCHAR BufferPtr);
        USHORT GetConfigurationDescriptor(USHORT LanguageID);
        USHORT GetStringDescriptor(USHORT LanguageID, UCHAR Index);
        void GetDriveLetters(PSP_DEVINFO_DATA pDeviceInfoData);
        CString GetDeviceRegistryProperty(DWORD Property);
    */
    };
}

#endif
