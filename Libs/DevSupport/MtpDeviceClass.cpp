#include "MtpDeviceClass.h"
#include "MtpDevice.h"

MtpDeviceClass::MtpDeviceClass(void)
: DeviceClass(NULL, &GUID_DEVCLASS_MTP, NULL, DeviceTypeMtp)
{
}

MtpDeviceClass::~MtpDeviceClass(void)
{
}

Device* MtpDeviceClass::CreateDevice(DeviceClass* deviceClass, SP_DEVINFO_DATA deviceInfoData, CStdString path)
{
	// add it to our list of devices if there are no filters
	if ( _filters.empty() )
	{
		return new MtpDevice(deviceClass, deviceInfoData.DevInst, path);
	}
	else
	{
		// if there are filters, don't add it unless it matches
		for (size_t idx=0; idx<_filters.size(); ++idx)
		{
			if ( path.IsEmpty() )
			{
				MtpDevice * dev = new MtpDevice(deviceClass, deviceInfoData.DevInst, path);
				if ( dev->_path.get().ToUpper().Find(_filters[idx].ToUpper()) != -1 )
				{
					return dev;
				}
				else
					delete dev;
			}
			else if ( path.ToUpper().Find(_filters[idx].ToUpper()) != -1 )
			{
				return new MtpDevice(deviceClass, deviceInfoData.DevInst, path);
			}
		}
	}
	
	return NULL;
}
