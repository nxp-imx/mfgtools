#include "stdafx.h"
#include "MxRomDeviceClass.h"
#include "MxRomDevice.h"

/// <summary>
/// Initializes a new instance of the MxRomDeviceClass class.
/// </summary>
MxRomDeviceClass::MxRomDeviceClass()
: DeviceClass(NULL, &GUID_DEVCLASS_MX_ROM_USB_DEVICE, NULL, DeviceTypeMxRom)
{
}

Device* MxRomDeviceClass::CreateDevice(DeviceClass* deviceClass, SP_DEVINFO_DATA deviceInfoData, CStdString path)
{
	if(deviceInfoData.ClassGuid == GUID_DEVCLASS_MX_ROM_USB_DEVICE && this->_deviceClassType == DeviceTypeMxRom)
		TRACE("DeviceClass::Devices:Init i.mx device object.\r\n");

	MxRomDevice* pDev = new MxRomDevice(deviceClass, deviceInfoData.DevInst, path);
    if ( pDev->IsUsb() )
		return pDev;
	else
	{
		delete pDev;
		return NULL;
	}
}

