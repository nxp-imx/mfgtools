#include "stdafx.h"
#include "RecoveryDeviceClass.h"
#include "RecoveryDevice.h"
#include "RecoveryDeviceGuid.h"

RecoveryDeviceClass::RecoveryDeviceClass(void)
: DeviceClass(NULL, &GUID_CLASS_STMP3XXX_USB_BULK_DEVICE, NULL, DeviceTypeRecovery)
{
}

RecoveryDeviceClass::~RecoveryDeviceClass(void)
{
}

Device* RecoveryDeviceClass::CreateDevice(DeviceClass* deviceClass, SP_DEVINFO_DATA deviceInfoData, CStdString path)
{
	RecoveryDevice * dev;

	// add it to our list of devices if there are no filters
	if ( _filters.empty() )
	{
		dev = new RecoveryDevice(deviceClass, deviceInfoData.DevInst, path);
		Sleep(1000);
		return dev;
	}
	else
	{
		// if there are filters, don't add it unless it matches
		for (size_t idx=0; idx<_filters.size(); ++idx)
		{
			if ( path.IsEmpty() )
			{
				dev = new RecoveryDevice(deviceClass, deviceInfoData.DevInst, path);
				if ( dev->_path.get().ToUpper().Find(_filters[idx].ToUpper()) != -1 )
				{
					Sleep(1000);
					return dev;
				}
				else
					delete dev;
			}
			else if ( path.ToUpper().Find(_filters[idx].ToUpper()) != -1 )
			{
				dev = new RecoveryDevice(deviceClass, deviceInfoData.DevInst, path);
				Sleep(1000);
				return dev;
			}
		}
	}
	
	return NULL;
}


