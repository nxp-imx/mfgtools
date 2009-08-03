#include "HidDeviceClass.h"
#include "HidDevice.h"
#include <hidclass.h>

HidDeviceClass::HidDeviceClass(void)
: DeviceClass(&GUID_DEVINTERFACE_HID, &GUID_DEVCLASS_HIDCLASS, _T("HID"), DeviceTypeHid)
{
	// CLW just get the 3700 HID Recovery-devices
//clw	AddFilter(0x066f, 0x3700);
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

size_t HidDeviceClass::AddFilter(LPCTSTR vid, LPCTSTR pid, LPCTSTR instance)
{
	CStdString filter;
	filter.Format(_T("hid#vid_%s&pid_%s#%s"), vid, pid, instance);
	_filters.push_back(filter);
	return _filters.size();
}

Device* HidDeviceClass::CreateDevice(DeviceClass* deviceClass, SP_DEVINFO_DATA deviceInfoData, CStdString path)
{
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
