/*
 * Copyright (C) 2009-2011, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */
#include "UsbControllerMgr.h"
#include "UsbController.h"

usb::ControllerMgr::ControllerMgr(void)
: DeviceClass(&GUID_DEVINTERFACE_USB_HOST_CONTROLLER, &GUID_DEVCLASS_USB, _T("PCI"), DeviceTypeUsbController)
{
}

usb::ControllerMgr::~ControllerMgr(void)
{
}

Device* usb::ControllerMgr::CreateDevice(DeviceClass* deviceClass, SP_DEVINFO_DATA deviceInfoData, CStdString path)
{
	return new usb::Controller(deviceClass, deviceInfoData.DevInst, path);
}
