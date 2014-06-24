/*
 * Copyright (C) 2009-2013, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */

#pragma once

#include "DeviceClass.h"
#include "UsbHub.h"

namespace usb
{
	class HubClass : public DeviceClass
	{
	public:
		HubClass(INSTANCE_HANDLE handle);
		virtual ~HubClass();
		
		Device* CreateDevice(DeviceClass* deviceClass, SP_DEVINFO_DATA deviceInfoData, CString path);

		Hub* FindHubByDriver(LPCTSTR driverName);
		Hub* FindHubByPath(LPCTSTR pathName);

		void RefreshHubs();
		
		CString ToString() { return _T("HubClass"); }
	};
}	// namespace usb

