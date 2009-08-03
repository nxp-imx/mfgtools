#pragma once
#include "DeviceClass.h"

class RecoveryDeviceClass : public DeviceClass
{
friend class DeviceManager;
private:
	RecoveryDeviceClass(void);
	virtual ~RecoveryDeviceClass(void);

private:
	Device* CreateDevice(DeviceClass* deviceClass, SP_DEVINFO_DATA deviceInfoData, CStdString path);

public:
	CStdString ToString() { return _T("RecoveryDeviceClass"); }
};
