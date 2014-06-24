/*
 * Copyright (C) 2009-2013, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */

#include "stdafx.h"
#include "ControllerClass.h"
#include "UsbController.h"
#include "MfgToolLib_Export.h"

usb::ControllerClass::ControllerClass(INSTANCE_HANDLE handle)
: DeviceClass(&GUID_DEVINTERFACE_USB_HOST_CONTROLLER, &GUID_DEVCLASS_USB, _T("PCI"), DeviceTypeUsbController, handle)
{
	//LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_NORMAL_MSG, _T("new ControllerClass"));
}

usb::ControllerClass::~ControllerClass()
{
	//LogMsg(LOG_MODULE_MFGTOOL_LIB, LOG_LEVEL_NORMAL_MSG, _T("delete ControllerClass"));
}

Device* usb::ControllerClass::CreateDevice(DeviceClass* deviceClass, SP_DEVINFO_DATA deviceInfoData, CString path)
{
    return new usb::Controller(deviceClass, deviceInfoData.DevInst, path, m_pLibHandle);
}

