#include "DiskDeviceClass.h"
#include "Disk.h"

/// <summary>
/// Initializes a new instance of the DiskDeviceClass class.
/// </summary>
DiskDeviceClass::DiskDeviceClass(void)
: DeviceClass(&GUID_DEVINTERFACE_DISK, &GUID_DEVCLASS_DISKDRIVE, _T(""), DeviceTypeMsc)
{
}

DiskDeviceClass::~DiskDeviceClass(void)
{
}

Device* DiskDeviceClass::CreateDevice(DeviceClass* deviceClass, SP_DEVINFO_DATA deviceInfoData, CStdString path)
{
	// add it to our list of devices if there are no filters
	if ( _filters.empty() )
	{
		return new Disk(deviceClass, deviceInfoData.DevInst, path);
	}
	else
	{
		// if there are filters, don't add it unless it matches
		for (size_t idx=0; idx<_filters.size(); ++idx)
		{
			if ( path.IsEmpty() )
			{
				Disk * dev = new Disk(deviceClass, deviceInfoData.DevInst, path);
				if ( dev->_path.get().ToUpper().Find(_filters[idx].ToUpper()) != -1 )
				{
					return dev;
				}
				else
					delete dev;
			}
			else if ( path.ToUpper().Find(_filters[idx].ToUpper()) != -1 )
			{
				return new Disk(deviceClass, deviceInfoData.DevInst, path);
			}
		}
	}
	
	return NULL;
}