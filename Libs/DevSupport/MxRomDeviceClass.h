#pragma once
#include "DeviceClass.h"
#include "MxRomDevice.h"
#include "../MxLib/Platform/usbinf.h"

#include <initguid.h>
// Device Class GUID for the Jungo/WinDriver iMX ROM Driver
// Used in mx*.inf
// {C671678C-82C1-43F3-D700-0049433E9A4B}
DEFINE_GUID(GUID_DEVCLASS_MX_ROM_USB_DEVICE, 0xC671678C, 0x82C1, 0x43F3, 0xD7, 0x00, 0x00, 0x49, 0x43, 0x3E, 0x9A, 0x4B);

class MxRomDeviceClass : public DeviceClass
{
friend class DeviceManager;
private:
    /// <summary>
    /// Initializes a new instance of the MxRomDeviceClass class.
    /// </summary>
    MxRomDeviceClass();
	virtual ~MxRomDeviceClass(void) {}

    Device* CreateDevice(DeviceClass* deviceClass, SP_DEVINFO_DATA deviceInfoData, CStdString path);

public:
	CStdString ToString() { return _T("MxRomDeviceClass"); }

};


