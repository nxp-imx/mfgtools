#pragma once
#include "stdafx.h"
#include "MxRomDevice.h"
#include "DeviceManager.h"

#include "../MxLib/status_strings.h"
#include "../MxLib/Platform/MXDefine.h"
#include "../MxLib/AtkHostApiClass.h"

MxRomDevice::MxRomDevice(DeviceClass * deviceClass, DEVINST devInst, CStdString path)
	: Device(deviceClass, devInst, path)
{
    strcpy_s(&DefaultLicenseString[0], 128, "6c3cd57876b3a6e415f93fd697134bd97ee1ec50.Motorola Semiconductors");

	if (IsUsb())
    {
		AtkHostApiClass apiClass;
		apiClass.OpenUSB(MX_MX25_TO1);

		USB_DEVICE_DESCRIPTOR Descriptor;
		apiClass.GetDeviceDescriptor(&Descriptor);

		apiClass.CloseUSB();
/*		
		WDU_DRIVER_HANDLE hDriver = NULL;
        WDU_MATCH_TABLE matchTable;
		WDU_DEVICE * pDeviceInfo = NULL;
		memset(&matchTable, 0, sizeof(matchTable));
		matchTable.wVendorId = 0x15a2;
		matchTable.wProductId = 0x003a;

		WDU_EVENT_TABLE eventTable;

        DWORD dwError = WDU_Init(&hDriver, &matchTable, 1, &eventTable, &DefaultLicenseString[0], 0);
        CStringA errStr = Stat2Str(dwError);

		dwError = WDU_GetDeviceInfo(hDriver, &pDeviceInfo);
        errStr = Stat2Str(dwError);
*/
    }
}
