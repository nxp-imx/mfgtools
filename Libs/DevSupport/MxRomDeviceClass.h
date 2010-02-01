#pragma once
#include "DeviceClass.h"
#include "MxRomDevice.h"

#include <initguid.h>
// Device Class GUID for the Jungo/WinDriver iMX ROM Driver
// Used in mx*.inf
// {C671678C-82C1-43F3-D700-0049433E9A4B}
// DEFINE_GUID(GUID_DEVCLASS_MX_ROM_USB_DEVICE, 0xC671678C, 0x82C1, 0x43F3, 0xD7, 0x00, 0x00, 0x49, 0x43, 0x3E, 0x9A, 0x4B);

// Device Interface GUID for the MX ROM WDF USB Bulk Recovery Driver
// Used by imxusb.inf, imxusb.sys
// {00873FDF-61A8-11D1-AA5E-00C04FB1728B}
DEFINE_GUID(GUID_DEVINTERFACE_MX_ROM_WDF_USB_BULK_DEVICE, 0x00873FDF, 0x61A8, 0x11D1, 0xAA, 0x5E, 0x00, 0xC0, 0x4F, 0xB1, 0x72, 0x8B);

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

protected:
	virtual Device* FindDeviceByUsbPath(CStdString pathToFind, const DeviceListType devListType, const DeviceListAction devListAction );
};


