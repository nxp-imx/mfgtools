/*
 * Copyright (C) 2009-2013, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */

#pragma once

#include "DeviceClass.h"

/// <summary>
/// The device class for disk devices.
/// </summary>
class DiskDeviceClass : public DeviceClass
{
public:
	DiskDeviceClass(INSTANCE_HANDLE handle);
	virtual ~DiskDeviceClass(void);
	virtual std::list<Device*>& Refresh();
	Device* CreateDevice(DeviceClass* deviceClass, SP_DEVINFO_DATA deviceInfoData, CString path);

public:
	CString ToString() { return _T("DiskDeviceClass"); }
};
