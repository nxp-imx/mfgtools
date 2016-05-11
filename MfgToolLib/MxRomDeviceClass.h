/*
 * Copyright (c) 2016, Freescale Semiconductor, Inc.
 * All rights reserved.
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

#include "DeviceClass.h"
//#include "MxRomDevice.h"

//#include <initguid.h>
// Device Class GUID for the Jungo/WinDriver iMX ROM Driver
// Used in mx*.inf
// {C671678C-82C1-43F3-D700-0049433E9A4B}
// DEFINE_GUID(GUID_DEVCLASS_MX_ROM_USB_DEVICE, 0xC671678C, 0x82C1, 0x43F3, 0xD7, 0x00, 0x00, 0x49, 0x43, 0x3E, 0x9A, 0x4B);

// Device Interface GUID for the MX ROM WDF USB Bulk Recovery Driver
// Used by imxusb.inf, imxusb.sys
// {00873FDF-61A8-11D1-AA5E-00C04FB1728B}
#if 0
DEFINE_GUID(GUID_DEVINTERFACE_MX_ROM_WDF_USB_BULK_DEVICE, 0x00873FDF, 0x61A8, 0x11D1, 0xAA, 0x5E, 0x00, 0xC0, 0x4F, 0xB1, 0x72, 0x8B);
#endif
class MxRomDeviceClass : public DeviceClass
{
public:
    /// <summary>
    /// Initializes a new instance of the MxRomDeviceClass class.
    /// </summary>
    MxRomDeviceClass(INSTANCE_HANDLE handle);
    virtual ~MxRomDeviceClass();

    Device* CreateDevice(DeviceClass* deviceClass, SP_DEVINFO_DATA deviceInfoData, CString path);

public:
    CString ToString() { return _T("MxRomDeviceClass"); }
};
