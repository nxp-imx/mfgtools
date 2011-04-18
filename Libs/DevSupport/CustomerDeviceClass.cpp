/*
 * Copyright (C) 2011, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */
#include "stdafx.h"
#include "CustomerDeviceClass.h"

CustomerDeviceClass::CustomerDeviceClass(void)
: DeviceClass(&GUID_DEVINTERFACE_HUAWEI_E587, NULL, NULL, DeviceTypeCst)
{
}

CustomerDeviceClass::~CustomerDeviceClass(void)
{
}

size_t CustomerDeviceClass::AddFilter(LPCTSTR Prefix, uint16_t vid, uint16_t pid)
{
	CStdString filter;
	filter.Format(_T("%s#vid_%04x&pid_%04x"), Prefix,vid, pid);
	_filters.push_back(filter);
	return _filters.size();
}

size_t CustomerDeviceClass::AddFilter(LPCTSTR vid, LPCTSTR pid, LPCTSTR instance)
{
	CStdString filter;
	filter.Format(_T("usb#vid_%s&pid_%s#%s"), vid, pid, instance);
	_filters.push_back(filter);
	return _filters.size();
}

Device* CustomerDeviceClass::CreateDevice(DeviceClass* deviceClass, SP_DEVINFO_DATA deviceInfoData, CStdString path)
{
    //Check if the path matches our device pid&vid.
    //An example of value of path: "\\?\USB#Vid_12d1&Pid_1c22#0123456789ABCDEF#{a5dcbf10-6530-11d2-901f-00c04fb951ed} "

	// add it to our list of devices if there are no filters
	if ( _filters.empty() )
	{
		return new Device(deviceClass, deviceInfoData.DevInst, path);
	}
	else
	{
		// if there are filters, don't add it unless it matches
		for (size_t idx=0; idx<_filters.size(); ++idx)
		{
			if ( path.IsEmpty() )
			{
				Device * dev = new Device(deviceClass, deviceInfoData.DevInst, path);
				if ( dev->_path.get().ToUpper().Find(_filters[idx].ToUpper()) != -1 )
				{
					return dev;
				}
				else
					delete dev;
			}
			else if ( path.ToUpper().Find(_filters[idx].ToUpper()) != -1 )
			{
				return new Device(deviceClass, deviceInfoData.DevInst, path);
			}
		}
	}
	
	return NULL;
}
