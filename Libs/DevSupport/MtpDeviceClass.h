#pragma once
#include "DeviceClass.h"

#include <initguid.h>
// Device Class GUID for the MTP Devices
// {EEC5AD98-8080-425F-922A-DABF3DE3F69A}
DEFINE_GUID(GUID_DEVCLASS_MTP, 0xEEC5AD98L, 0x8080, 0x425F, 0x92, 0x2A, 0xDA, 0xBF, 0x3D, 0xE3, 0xF6, 0x9A);

class MtpDeviceClass : public DeviceClass
{
friend class DeviceManager;
private:
	MtpDeviceClass(void);
	virtual ~MtpDeviceClass(void);

private:
	Device* CreateDevice(DeviceClass* deviceClass, SP_DEVINFO_DATA deviceInfoData, CStdString path);

public:
	CStdString ToString() { return _T("MtpDeviceClass"); }
};
