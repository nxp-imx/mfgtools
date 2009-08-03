#include "UsbControllerMgr.h"
#include "UsbController.h"

usb::ControllerMgr::ControllerMgr(void)
: DeviceClass(&GUID_DEVINTERFACE_USB_HOST_CONTROLLER, &GUID_DEVCLASS_USB, _T("PCI"), DeviceTypeUsbController)
{
}

usb::ControllerMgr::~ControllerMgr(void)
{
}

Device* usb::ControllerMgr::CreateDevice(DeviceClass* deviceClass, SP_DEVINFO_DATA deviceInfoData, CStdString path)
{
	return new usb::Controller(deviceClass, deviceInfoData.DevInst, path);
}
