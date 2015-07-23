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
	request.dCBWTag = api.GetTag();
	memcpy((void*)&request.CDB, api.GetCdbPtr(), api.GetCdbSize());
	request.bCBWCBLength = api.GetCdbSize();
	request.dCBWDataTransferLength = api.GetTransferSize();
	response = libusb_bulk_transfer(m_libusbdevHandle, 0x01, (UCHAR*)&request, sizeof(CBW_UTP), &length , api.GetTimeout() * 1000);
	response += libusb_bulk_transfer(m_libusbdevHandle, 0x01, (UCHAR*)api.GetCmdDataPtr(), api.GetTransferSize(),  &length, api.GetTimeout() * 1000);

	api.ScsiSenseData.AdditionalSenseCode = (ScsiUtpMsg::EXIT & 0xFF00)>>8;
	api.ScsiSenseData.AdditionalSenseCodeQualifier = (ScsiUtpMsg::EXIT & 0x00FF);
	api.ScsiSenseData.Information[0] = 0xff;
	api.ScsiSenseData.Information[1] = 0xff;
	api.ScsiSenseData.Information[2] = 0xff;
	api.ScsiSenseData.Information[3] = 0xff;

	CSW_UTP uResponse;
	response += libusb_bulk_transfer(m_libusbdevHandle, 0x81, (UCHAR*) &uResponse, sizeof(CSW_UTP), &length, 1000);

	api.ScsiSenseStatus = response;
	api.ScsiSenseData.SenseKey = uResponse.bCSWStatus;

	CBW_UTP sRequest;
	SENSE_UTP senseResponse;
	CSW_UTP* psRequest;
	bool cont = uResponse.bCSWStatus;

	if ( !api.IsWriteCmd() )
		api.ProcessResponse((UCHAR*)&request, 0, api.GetTransferSize());

	while (uResponse.bCSWStatus == 1 || cont)
	{
		memset(&sRequest, 0, sizeof(CBW_UTP));
		sRequest.dCBWSignature[0] = 'U';
		sRequest.dCBWSignature[1] = 'S';
		sRequest.dCBWSignature[2] = 'B';
		sRequest.dCBWSignature[3] = 'C';
		sRequest.dCBWTag = api.GetTag();
		sRequest.dCBWDataTransferLength = 0x12;
		sRequest.bmCBWFlags = 0x80;
		sRequest.bCBWCBLength = 0x0c;
		sRequest.CDB.UTPOpcode = 3;
		sRequest.CDB.UTPTag = 0x120000;

		response = libusb_bulk_transfer(m_libusbdevHandle, 0x01, (UCHAR*)&sRequest, sizeof(CBW_UTP), &length , api.GetTimeout() * 1000);
		response = libusb_bulk_transfer(m_libusbdevHandle, 0x81, (UCHAR*) &senseResponse, sizeof(SENSE_UTP), &length, 1000);
		response = libusb_bulk_transfer(m_libusbdevHandle, 0x81, (UCHAR*) &uResponse, sizeof(CSW_UTP), &length, 1000);

		response = uResponse.bCSWStatus;
		cont = false;

		if (senseResponse.UTPSenseCode == 0x8002)
		{
			CSW_UTP psResponse;
			int response = pollDevice(&psResponse, api);
				if (response == 0 && !&psResponse)
				{
					if (psResponse.bCSWStatus == 1)
						cont = true;
					else
						cont = false;
				}
		}

		//Sleep is needed or else the computer will spam the device with requests causing malfunctions
		usleep(200000);
	}

	nsInfo.error = response;
	return response;
}
int libusbVolume::pollDevice(CSW_UTP* sResponse, StApi& api)
{
	int length;
	int response;
	CBW_UTP sRequest;

	memset(&sRequest, 0, sizeof(CBW_UTP));
	sRequest.dCBWSignature[0] = 'U';
	sRequest.dCBWSignature[1] = 'S';
	sRequest.dCBWSignature[2] = 'B';
	sRequest.dCBWSignature[3] = 'C';
	sRequest.dCBWTag = api.GetTag();
	sRequest.bCBWCBLength = 0x10;
	sRequest.CDB.UTPOpcode = 0xf0;
	sRequest.CDB.UTPTag = api.GetTag();

	response = libusb_bulk_transfer(m_libusbdevHandle, 0x01, (unsigned char*) &sRequest, sizeof(CBW_UTP), &length , 1000);
	if (response)
		return response;
	return libusb_bulk_transfer(m_libusbdevHandle, 0x81, (unsigned char*) &sResponse, sizeof(CSW_UTP), &length, 1000);
}
