/*
 * Copyright (C) 2010, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */
#pragma once
#include "DeviceClass.h"

namespace usb
{

	/// <summary>
	/// The device class for USB devices.
	/// </summary>
	class DeviceMgr :	public DeviceClass
	{
	public:
		DeviceMgr();
		virtual ~DeviceMgr(void);

	private:
//	   Device* CreateDevice(DeviceClass* deviceClass, SP_DEVINFO_DATA deviceInfoData, CStdString path);
	};
} // namespace usb
