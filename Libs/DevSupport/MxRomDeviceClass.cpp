#include "stdafx.h"
#include "MxRomDeviceClass.h"
#include "MxRomDevice.h"

/// <summary>
/// Initializes a new instance of the MxRomDeviceClass class.
/// </summary>
MxRomDeviceClass::MxRomDeviceClass()
: DeviceClass(&GUID_DEVINTERFACE_MX_ROM_WDF_USB_BULK_DEVICE, NULL, NULL, DeviceTypeMxRom)
{
}

Device* MxRomDeviceClass::CreateDevice(DeviceClass* deviceClass, SP_DEVINFO_DATA deviceInfoData, CStdString path)
{
	TRACE("DeviceClass::Devices:Init i.mx device object.\r\n");

	MxRomDevice* pDev = new MxRomDevice(deviceClass, deviceInfoData.DevInst, path);

	return pDev;
}

