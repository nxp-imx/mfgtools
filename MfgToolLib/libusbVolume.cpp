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
	std::cout << additionalInfo << std::endl;
	printf("SendCommand working");
	return 0;
}
UINT libusbVolume::SendCommand(HANDLE hDrive, StApi& api, UCHAR* additionalInfo, NotifyStruct& nsInfo)
{
	std::cout << additionalInfo << std::endl;
	printf("SendCommand2 working");
	return 0;
}
