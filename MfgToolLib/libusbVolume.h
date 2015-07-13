#pragma once
#include "UpdateTransportProtocol.Api.h"
#include "Volume.h"
class libusbVolume : public Volume
{
	public:
		libusb_device_handle* handle;
		libusbVolume(DeviceClass * deviceClass, DEVINST devInst, CString path, INSTANCE_HANDLE handle, libusb_device_handle *dev_handle);
		int SendCommand(const char* CMDName, uint32_t USBTag, uint32_t UTPTag);
		int SendCommand(ScsiUtpMsg scsimessage, uint32_t USBTag, uint32_t UTPTag);
};
