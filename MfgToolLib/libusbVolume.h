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
#pragma pack (push, 1)
struct CBW_UTP
{
	//CBW
	uint8_t dCBWSignature[4];
	uint32_t dCBWTag;
	uint32_t dCBWDataTransferLength;
	uint8_t bmCBWFlags;
	uint8_t bCBWLUN;
	uint8_t bCBWCBLength;
	//CDB
	uint8_t UTPOpcode;
	uint8_t UTPMessage;
	uint32_t UTPTag;
	uint32_t UTPParam1;
	uint32_t UTPParam2;
	uint16_t SCSIExtra;
};
