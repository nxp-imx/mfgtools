#pragma once
#include "DeviceClass.h"

/// <summary>
/// The device class for disk devices.
/// </summary>
class DiskDeviceClass :
	public DeviceClass
{
public:
	DiskDeviceClass(void);
	virtual ~DiskDeviceClass(void);

private:
   Device* CreateDevice(DeviceClass* deviceClass, SP_DEVINFO_DATA deviceInfoData, CStdString path);
};
