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

#include "stdafx.h"
#include "MxRomDeviceClass.h"
#include "MxRomDevice.h"
#include "MfgToolLib_Export.h"
#include "MfgToolLib.h"

/// <summary>
/// Initializes a new instance of the MxRomDeviceClass class.
/// </summary>
MxRomDeviceClass::MxRomDeviceClass(INSTANCE_HANDLE handle)
: DeviceClass(&GUID_DEVINTERFACE_MX_ROM_WDF_USB_BULK_DEVICE, NULL, NULL, DeviceTypeMxRom, handle)
{
	LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_FATAL_ERROR, _T("new MxRomDeviceClass"));
}

MxRomDeviceClass::~MxRomDeviceClass() 
{
	LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_FATAL_ERROR, _T("delete MxRomDeviceClass"));
}

Device* MxRomDeviceClass::CreateDevice(DeviceClass* deviceClass, SP_DEVINFO_DATA deviceInfoData, CString path)
{
    MxRomDevice* pDev = new MxRomDevice(deviceClass, deviceInfoData.DevInst, path, m_pLibHandle);

    return pDev;
}


