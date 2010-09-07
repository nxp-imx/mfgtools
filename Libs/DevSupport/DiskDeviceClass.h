#pragma once
#include "DeviceClass.h"

/// <summary>
/// The device class for disk devices.
/// </summary>
class DiskDeviceClass : public DeviceClass
{
public:
	DiskDeviceClass(void);
	virtual ~DiskDeviceClass(void);
	virtual std::list<Device*>& Refresh();
//	virtual NotifyStruct AddUsbDevice(LPCTSTR path);
//	virtual NotifyStruct RemoveUsbDevice(LPCTSTR path);

private:
   Device* CreateDevice(DeviceClass* deviceClass, SP_DEVINFO_DATA deviceInfoData, CStdString path);

public:
	CStdString ToString() { return _T("DiskDeviceClass"); }
};
