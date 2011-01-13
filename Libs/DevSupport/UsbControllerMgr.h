/*
 * Copyright (C) 2009-2011, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */
#pragma once
#include "DeviceClass.h"

namespace usb
{
	class ControllerMgr : public DeviceClass
	{
		friend class DeviceManager;
	private:
		ControllerMgr(void);
		virtual ~ControllerMgr(void);

	private:
	   Device* CreateDevice(DeviceClass* deviceClass, SP_DEVINFO_DATA deviceInfoData, CStdString path);

	public:
		CStdString ToString() { return _T("ControllerMgr"); }
	};
} // namespace usb
