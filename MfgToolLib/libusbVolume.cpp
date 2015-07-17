#include "UpdateTransportProtocol.Api.h"
#include "libusbVolume.h"
#include "Volume.h"

	libusbVolume::libusbVolume(DeviceClass * deviceClass, DEVINST devInst, CString path, INSTANCE_HANDLE ihandle)
: Volume(deviceClass, devInst, path, ihandle)
{
	//TODO
}
UINT libusbVolume::SendCommand(StApi& api, UCHAR* additionalInfo)
{
	//TODO
	return 0;
}
UINT libusbVolume::SendCommand(HANDLE hDrive, StApi& api, UCHAR* additionalInfo, NotifyStruct& nsInfo)
{
	// init parameter if it is used
	if (additionalInfo)
		*additionalInfo = SCSISTAT_GOOD;
	int length;
	int response;
	// Allocate the SCSI request
	DWORD totalSize = sizeof(CBW_UTP) + api.GetTransferSize();
	CBW_UTP request;
	memset(&request, 0, sizeof(CBW_UTP));

	if ( &request == NULL )
	{
		nsInfo.inProgress = false;
		return ERROR_NOT_ENOUGH_MEMORY;
	}

	// CDB Information
	request.dCBWSignature[0] = 'U';
	request.dCBWSignature[1] = 'S';
	request.dCBWSignature[2] = 'B';
	request.dCBWSignature[3] = 'C';
	memcpy((void*)&request.CDB, api.GetCdbPtr(), api.GetCdbSize());
	request.bCBWCBLength = api.GetCdbSize();
	request.dCBWDataTransferLength = api.GetTransferSize();
	response = libusb_bulk_transfer(m_libusbdevHandle, 0x01, (unsigned char*)&request, sizeof(CBW_UTP), &length , api.GetTimeout() * 1000);
	response += libusb_bulk_transfer(m_libusbdevHandle, 0x01, (unsigned char*)api.GetCmdDataPtr(), api.GetTransferSize(),  &length, api.GetTimeout() * 1000);
	nsInfo.error = response;
	return response;
}
