#include "stdafx.h"
#include <Assert.h>
#include <cfgmgr32.h>
#include <basetyps.h>
#include <setupapi.h>
#include <initguid.h>
extern "C" {
#include "Libs/WDK/hidsdi.h"
}
#include "Libs/WDK/hidclass.h"
#include "DeviceClass.h"
#include "MxHidDevice.h"
#include "MxHidDeviceClass.h"

MxHidDeviceClass::MxHidDeviceClass(void)
: DeviceClass(&GUID_DEVINTERFACE_HID, &GUID_DEVCLASS_HIDCLASS, _T("MXHID"), DeviceTypeMxHid)
{
    AddFilter(_T("hid"),0x15A2, 0x0052);//MX508
}

MxHidDeviceClass::~MxHidDeviceClass(void)
{
}

size_t MxHidDeviceClass::AddFilter(uint16_t vid, uint16_t pid)
{
	CStdString filter;
	filter.Format(_T("hid#vid_%04x&pid_%04x"), vid, pid);
	_filters.push_back(filter);
	return _filters.size();
}

size_t MxHidDeviceClass::AddFilter(LPCTSTR Prefix, uint16_t vid, uint16_t pid)
{
	CStdString filter;
	filter.Format(_T("%s#vid_%04x&pid_%04x"), Prefix,vid, pid);
	_filters.push_back(filter);
	return _filters.size();
}

size_t MxHidDeviceClass::AddFilter(LPCTSTR vid, LPCTSTR pid, LPCTSTR instance)
{
	CStdString filter;
	filter.Format(_T("hid#vid_%s&pid_%s#%s"), vid, pid, instance);
	_filters.push_back(filter);
	return _filters.size();
}

Device* MxHidDeviceClass::CreateDevice(DeviceClass* deviceClass, SP_DEVINFO_DATA deviceInfoData, CStdString path)
{
	//Check if the path matches our device pid&vid.
    //An example of value of path: "\\?\hid#vid_413c&pid_2011&mi_01&col02#8&2598dfbd&0&0001#{4d1e55b2-f16f-11cf-88cb-001111000030}"

	// add it to our list of devices if there are no filters
	if ( _filters.empty() )
	{
		return new MxHidDevice(deviceClass, deviceInfoData.DevInst, path);
	}
	else
	{
		// if there are filters, don't add it unless it matches
		for (size_t idx=0; idx<_filters.size(); ++idx)
		{
			if ( path.IsEmpty() )
			{
				MxHidDevice * dev = new MxHidDevice(deviceClass, deviceInfoData.DevInst, path);
				if ( dev->_path.get().ToUpper().Find(_filters[idx].ToUpper()) != -1 )
				{
					return dev;
				}
				else
					delete dev;
			}
			else if ( path.ToUpper().Find(_filters[idx].ToUpper()) != -1 )
			{
				return new MxHidDevice(deviceClass, deviceInfoData.DevInst, path);
			}
		}
	}
	
	return NULL;
}
