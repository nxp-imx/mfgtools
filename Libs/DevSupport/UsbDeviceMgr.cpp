/*
 * Copyright (C) 2010, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */
#include "UsbDeviceMgr.h"

/// <summary>
/// Initializes a new instance of the usb::DeviceMgr class.
/// </summary>
usb::DeviceMgr::DeviceMgr()
: DeviceClass(&GUID_DEVINTERFACE_USB_DEVICE, &GUID_DEVCLASS_USB, _T("USB"), DeviceTypeUsbDevice)
{
}

usb::DeviceMgr::~DeviceMgr(void)
{
}

//Device* usb::DeviceMgr::CreateDevice(DeviceClass* deviceClass, SP_DEVINFO_DATA deviceInfoData, CStdString path)
//{
//	return new usb::Hub(deviceClass, deviceInfoData.DevInst, path);
//}
