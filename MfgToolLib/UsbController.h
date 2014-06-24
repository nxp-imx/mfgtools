/*
 * Copyright (C) 2009-2013, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */

#pragma once

#include "UsbHub.h"
#include "Device.h"

namespace usb
{
	class Controller : public Device
	{
	public:
		Controller(DeviceClass * deviceClass, DEVINST devInst, CString path, INSTANCE_HANDLE handle);
		virtual ~Controller();
		Hub* GetRootHub();
		HANDLE Open();
		DWORD Initialize();

		StringProperty _rootHubFilename; //the root hub name, also the root hub path
	};
}	// namespace usb


