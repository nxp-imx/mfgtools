#pragma once
#include "UpdateTransportProtocol.Api.h"
#include "Volume.h"
class libusbVolume : public Volume
{
	public:
		libusb_device_handle* handle;
		libusbVolume(DeviceClass * deviceClass, DEVINST devInst, CString path, INSTANCE_HANDLE handle, libusb_device_handle *dev_handle);
		UINT SendCommand(StApi& api, UCHAR* additionalInfo);
		UINT SendCommand(HANDLE hDrive, StApi& api, UCHAR* additionalInfo, NotifyStruct& nsInfo);
};
