#pragma once
#include "UpdateTransportProtocol.Api.h"
#include "Volume.h"
#pragma pack (push, 1)
struct CDB_UTP
{
	uint8_t UTPOpcode;
	uint8_t UTPMessage;
	uint32_t UTPTag;
	uint32_t UTPParam1;
	uint32_t UTPParam2;
	uint16_t SCSIExtra;
};
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
	CDB_UTP CDB;
};
struct CSW_UTP
{
	uint8_t dCBWSignature[4];
	uint32_t dCSWTag;
	uint32_t dCSWDataResidue;
	uint8_t bCSWStatus;
};
struct SENSE_UTP
{
	uint8_t UTPResponseCode;
	uint8_t UTPSpec;
	uint8_t UTPSenseKey;
	uint32_t UTPReplyInfoLower;
	uint8_t UTPSenseLength;
	uint32_t UTPReplyInfoUpper;
	uint16_t UTPSenseCode;
	uint32_t SCSIExtra;
};
#pragma pack (pop)
class libusbVolume : public Volume
{
	public:
		libusbVolume(DeviceClass* deviceClass, DEVINST devInst, CString path, INSTANCE_HANDLE handle);
		int pollDevice(CSW_UTP* sResponse, StApi& api);
		UINT SendCommand(StApi& api, UCHAR* additionalInfo);
		UINT SendCommand(HANDLE hDrive, StApi& api, UCHAR* additionalInfo, NotifyStruct& nsInfo);
};
