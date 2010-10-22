/*
 * Copyright (C) 2010, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */
#pragma once

#include "UsbHub.h"

namespace usb
{
	class Controller : public Device
	{
	public:
		Controller(DeviceClass * deviceClass, DEVINST devInst, CStdString path);
		virtual ~Controller(void);
		Hub * GetRootHub();

	private:
		HANDLE Open();
		int32_t Initialize();

		StringProperty _rootHubFilename;
	};
} // namespace usb
