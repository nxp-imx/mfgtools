/*
 * Copyright (C) 2009-2013, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
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


