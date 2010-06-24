#pragma once
#include "DeviceClass.h"

//extern "C" {
//#include <hidsdi.h>
//}

class HidDeviceClass : public DeviceClass
{
friend class DeviceManager;
private:
	HidDeviceClass(void);
	virtual ~HidDeviceClass(void);

	size_t AddFilter(uint16_t vid, uint16_t pid);
	size_t AddFilter(LPCTSTR Prefix, uint16_t vid, uint16_t pid);
	size_t AddFilter(LPCTSTR vid, LPCTSTR pid, LPCTSTR instance = _T(""));

private:
   Device* CreateDevice(DeviceClass* deviceClass, SP_DEVINFO_DATA deviceInfoData, CStdString path);

public:
	CStdString ToString() { return _T("HidDeviceClass"); }
};
