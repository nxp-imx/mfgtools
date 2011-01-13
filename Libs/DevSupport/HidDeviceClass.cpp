/*
 * Copyright (C) 2009-2011, Freescale Semiconductor, Inc. All Rights Reserved.
 * THIS SOURCE CODE IS CONFIDENTIAL AND PROPRIETARY AND MAY NOT
 * BE USED OR DISTRIBUTED WITHOUT THE WRITTEN PERMISSION OF
 * Freescale Semiconductor, Inc.
 *
 */
#include "stdafx.h"
#include "HidDeviceClass.h"
#include "HidDevice.h"
#include <hidclass.h>

HidDeviceClass::HidDeviceClass(void)
: DeviceClass(&GUID_DEVINTERFACE_HID, &GUID_DEVCLASS_HIDCLASS, _T("HID"), DeviceTypeHid)
{
    AddFilter(_T("hid"),0x066f, 0x3780);//MX233
    AddFilter(_T("hid"),0x15A2, 0x004F);//MX28
    //AddFilter(_T("usb"),0x066f, 0x3780);//MX233
    //AddFilter(_T("usb"),0x15A2, 0x004F);//MX28
}

HidDeviceClass::~HidDeviceClass(void)
{
}

size_t HidDeviceClass::AddFilter(uint16_t vid, uint16_t pid)
{
	CStdString filter;
	filter.Format(_T("hid#vid_%04x&pid_%04x"), vid, pid);
	_filters.push_back(filter);
	return _filters.size();
}

size_t HidDeviceClass::AddFilter(LPCTSTR Prefix, uint16_t vid, uint16_t pid)
{
	CStdString filter;
	filter.Format(_T("%s#vid_%04x&pid_%04x"), Prefix,vid, pid);
	_filters.push_back(filter);
	return _filters.size();
}

size_t HidDeviceClass::AddFilter(LPCTSTR vid, LPCTSTR pid, LPCTSTR instance)
{
	CStdString filter;
	filter.Format(_T("hid#vid_%s&pid_%s#%s"), vid, pid, instance);
	_filters.push_back(filter);
	return _filters.size();
}

Device* HidDeviceClass::CreateDevice(DeviceClass* deviceClass, SP_DEVINFO_DATA deviceInfoData, CStdString path)
{
    //Check if the path matches our device pid&vid.
    //An example of value of path: "\\?\hid#vid_413c&pid_2011&mi_01&col02#8&2598dfbd&0&0001#{4d1e55b2-f16f-11cf-88cb-001111000030}"

	// add it to our list of devices if there are no filters
	if ( _filters.empty() )
	{
		return new HidDevice(deviceClass, deviceInfoData.DevInst, path);
	}
	else
	{
		// if there are filters, don't add it unless it matches
		for (size_t idx=0; idx<_filters.size(); ++idx)
		{
			if ( path.IsEmpty() )
			{
				HidDevice * dev = new HidDevice(deviceClass, deviceInfoData.DevInst, path);
				if ( dev->_path.get().ToUpper().Find(_filters[idx].ToUpper()) != -1 )
				{
					return dev;
				}
				else
					delete dev;
			}
			else if ( path.ToUpper().Find(_filters[idx].ToUpper()) != -1 )
			{
				return new HidDevice(deviceClass, deviceInfoData.DevInst, path);
			}
		}
	}
	
	return NULL;
}
