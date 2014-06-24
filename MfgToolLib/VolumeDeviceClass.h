/*
 * Copyright (C) 2009-2013, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */

#pragma once

#include "DeviceClass.h"
#include <map>

/// <summary>
/// The device class for volume devices.
/// </summary>
class VolumeDeviceClass : public DeviceClass
{
public:
	VolumeDeviceClass(INSTANCE_HANDLE handle);
	virtual ~VolumeDeviceClass(void);
	virtual NotifyStruct AddUsbDevice(LPCTSTR path);
	virtual NotifyStruct RemoveUsbDevice(LPCTSTR path);
	virtual std::list<Device*>& Refresh();
	// Maps Volume Name to Drive Letter. ( ex. "\\?\Volume{1e7c5ed6-1b59-11dc-b682-0013721e2a55}\" , "A:" )
	std::map<CString, CString> _logicalDrives;

private:
	UINT InitDriveLettersMap();
	Device* CreateDevice(DeviceClass* deviceClass, SP_DEVINFO_DATA deviceInfoData, CString path);

public:
	CString ToString() { return _T("VolumeDeviceClass"); }
};
