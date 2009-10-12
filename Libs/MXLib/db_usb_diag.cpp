// use in wizard's device-specific generated code
#include "wdu_lib.h"
#include "status_strings.h"
#include "utils.h"
#include "usb_diag_lib.h"
#include "Platform/UsbInf.h"
#include <stdlib.h>
#include <stdio.h>

#define DEFAULT_INTERFACE_NUM     0
#define DEFAULT_ALTERNATE_SETTING 0
#define DEFAULT_LICENSE_STRING    "6c3cd57876b3a6e415f93fd697134bd97ee1ec50.Motorola Semiconductors"
#define ATTACH_EVENT_TIMEOUT 30 // in seconds
#define TRANSFER_TIMEOUT 30000 // in msecs

#if !defined(TRACE)
#define TRACE printf
#endif
#if !defined(ERR)
#define ERR printf
#endif

static char line[250];
static WORD wVendorId = 0;
static WORD wProductId = 0;
static WDU_DRIVER_HANDLE hDriver;
static DEVICE_CONTEXT DevCtx;
static DWORD dwWritePipeNum = 0x01;
static DWORD dwReadPipeNum = 0x82;

BOOL WINAPI DeviceAttach(WDU_DEVICE_HANDLE hDevice, 
    WDU_DEVICE *pDeviceInfo, PVOID pUserData)
{
    BOOL bAcceptControl;
    DEVICE_CONTEXT *pDevCtx = (DEVICE_CONTEXT *)pUserData;
    DWORD dwInterfaceNum = pDeviceInfo->pActiveInterface->pActiveAltSetting->Descriptor.bInterfaceNumber;

    if (dwInterfaceNum != pDevCtx->dwInterfaceNum)
    {
        // This is not the requested interface
        return FALSE;
    }

    if (pDevCtx->hDev)
    {
        TRACE("DeviceAttach: received another attach - control is rejected "
            "because only one device control is supported\n");
        return FALSE;
    }

    pDevCtx->hDev = hDevice;

    pDevCtx->dwAttachError = WDU_SetInterface(pDevCtx->hDev, pDevCtx->dwInterfaceNum, pDevCtx->dwAlternateSetting);
    if (pDevCtx->dwAttachError)
    {
        ERR("DeviceAttach: WDU_SetInterface failed (num. %d, alternate %d) device 0x%x: error 0x%x (\"%s\")\n",
            pDevCtx->dwInterfaceNum, pDevCtx->dwAlternateSetting, pDevCtx->hDev, 
            pDevCtx->dwAttachError, Stat2Str(pDevCtx->dwAttachError));

        pDevCtx->hDev = 0;
        bAcceptControl = FALSE;
    }
    else
    {
        TRACE("DeviceAttach: received and accepted attach for vendor id 0x%x, "
            "product id 0x%x, device handle 0x%x\n",
            pDeviceInfo->Descriptor.idVendor, pDeviceInfo->Descriptor.idProduct,
            pDevCtx->hDev);
        bAcceptControl = TRUE;
    }

    OsEventSignal(pDevCtx->hEvent);
    // Accept control over this device
    return bAcceptControl;
}
BOOL USB_Trans_Data(BOOL bRead,DWORD dwPipeNum,PVOID pBuffer,DWORD dwSize)
{
	DWORD dwError;
    WDU_DEVICE *pDevice;
    WDU_ALTERNATE_SETTING *pAltSet;
    WDU_PIPE_INFO *pPipes;
    BYTE  SetupPacket[8];
    DWORD i, dwBytesTransferred;
	WDU_DEVICE_HANDLE hDevice = DevCtx.hDev;

    dwError = WDU_GetDeviceInfo(hDevice, &pDevice);
    if (dwError)
    {
        ERR("ReadWritePipesMenu: WDU_GetDeviceInfo() failed: error 0x%x (\"%s\")\n",
            dwError, Stat2Str(dwError));
        return FALSE;
    }

    pAltSet = pDevice->pActiveInterface->pActiveAltSetting;
    pPipes = pAltSet->pPipes;

	// search for the pipe
        if (dwPipeNum)
        {
            for (i=0; i<pAltSet->Descriptor.bNumEndpoints; i++)
                if (pPipes[i].dwNumber==dwPipeNum)
                    break;
            if (i >= pAltSet->Descriptor.bNumEndpoints)
            {
                printf("The pipe number 0x%x does not exist, please try again.\n", dwPipeNum);
                return FALSE;
            }
        }
		
		dwError = WDU_Transfer(hDevice, dwPipeNum, bRead, 0, pBuffer, 
                dwSize, &dwBytesTransferred, SetupPacket, TRANSFER_TIMEOUT);
	
		// free the memory
		if(pDevice)
			free(pDevice);

		if (dwError){
                ERR("ReadWritePipesMenu: WDU_Transfer() failed: error 0x%x (\"%s\")\n",
                    dwError, Stat2Str(dwError));
				return FALSE;
		}
		else{
                printf("Transferred %d bytes\n", dwBytesTransferred);
				return TRUE;
		}
}

VOID WINAPI DeviceDetach(WDU_DEVICE_HANDLE hDevice, PVOID pUserData)
{
    DEVICE_CONTEXT *pDevCtx = (DEVICE_CONTEXT *)pUserData;

    pDevCtx->hDev = 0;
    TRACE("DeviceDetach: received detach for device handle 0x%x\n", hDevice);
}


//NewFunctionCall :
BOOL WriteToUsb(DWORD dwSize,PUINT pCommand,int id)
{
	BOOL bStatus = FALSE;
	bStatus = USB_Trans_Data(FALSE,dwWritePipeNum,pCommand,dwSize);
	return bStatus;
}


//NewFunctionCall
BOOL ReadFromUsb(DWORD dwSize, int id, PVOID pBuffer)
{
	BOOL bStatus = FALSE;
	bStatus = USB_Trans_Data(TRUE,dwReadPipeNum,pBuffer,dwSize);
	return bStatus;
}

BOOL USB_OpenLocateDevice(int id)
{
	DWORD dwError;
    WD_LICENSE lic;
    WDU_MATCH_TABLE matchTable;
    WDU_EVENT_TABLE eventTable;

    BZERO(DevCtx);

	// set the interface
    DevCtx.dwInterfaceNum = DEFAULT_INTERFACE_NUM;

	// set the alt
    DevCtx.dwAlternateSetting = DEFAULT_ALTERNATE_SETTING;

	wVendorId  = (WORD)mxusb_inf[id].vendor;
	wProductId = (WORD)mxusb_inf[id].product;
	
	// Create Event
    dwError = OsEventCreate(&DevCtx.hEvent);
    if (dwError)
    {
        ERR("main: OsEventCreate() failed on event 0x%x: error 0x%x (\"%s\")\n",
            DevCtx.hEvent, dwError, Stat2Str(dwError));
        goto Exit;
    }

    BZERO(matchTable);
    matchTable.wVendorId = wVendorId;
    matchTable.wProductId = wProductId;

    BZERO(eventTable);
    eventTable.pfDeviceAttach = DeviceAttach;
    eventTable.pfDeviceDetach = DeviceDetach;
    eventTable.pUserData = &DevCtx;

    strcpy_s(lic.cLicense, sizeof(lic.cLicense), DEFAULT_LICENSE_STRING);
    dwError = WDU_Init(&hDriver, &matchTable, 1, &eventTable, lic.cLicense, WD_ACKNOWLEDGE);
    if (dwError)
    {
        ERR("main: failed to initialize USB driver: error 0x%x (\"%s\")\n", 
                dwError, Stat2Str(dwError));
        goto Exit;
    }
   
    printf("Please make sure the device is attached:\n");

    // Wait for the device to be attached
    dwError = OsEventWait(DevCtx.hEvent, ATTACH_EVENT_TIMEOUT);
    if (dwError)
    {
        if (dwError==WD_TIME_OUT_EXPIRED)
            ERR("Timeout expired for connection with the device.\n" 
                "Check that the device is connected and try again.\n");
        else
            ERR("main: OsEventWait() failed on event 0x%x: error 0x%x (\"%s\")\n",
                DevCtx.hEvent, dwError, Stat2Str(dwError));
        goto Exit;
    }
    if (DevCtx.dwAttachError)
        goto Exit;
	
	return TRUE;
Exit:
    OsEventClose(DevCtx.hEvent);
    WDU_Uninit(hDriver);
	return FALSE;

}

void USB_CloseLocateDevice(void)
{
	OsEventClose(DevCtx.hEvent);
	WDU_Uninit(hDriver);
}

BOOL USB_GetDeviceDescriptor(USB_DEVICE_DESCRIPTOR *pDescriptor)
{
	DWORD dwError;
	WDU_DEVICE *pDevice;
	WDU_DEVICE_HANDLE hDevice = DevCtx.hDev;

	dwError = WDU_GetDeviceInfo(hDevice, &pDevice);
	if (dwError)
	{
		ERR("ReadWritePipesMenu: WDU_GetDeviceInfo() failed: error 0x%x (\"%s\")\n",
			dwError, Stat2Str(dwError));
		return FALSE;
	}

	pDescriptor->bcdDevice			= pDevice->Descriptor.bcdDevice;
	pDescriptor->bcdUSB				= pDevice->Descriptor.bcdUSB;
	pDescriptor->bDescriptorType	= pDevice->Descriptor.bDescriptorType;
	pDescriptor->bDeviceClass		= pDevice->Descriptor.bDeviceClass;
	pDescriptor->bDeviceProtocol	= pDevice->Descriptor.bDeviceProtocol;
	pDescriptor->bDeviceSubClass	= pDevice->Descriptor.bDeviceSubClass;
	pDescriptor->bLength			= pDevice->Descriptor.bLength;
	pDescriptor->bMaxPacketSize0	= pDevice->Descriptor.bMaxPacketSize0;
	pDescriptor->bNumConfigurations = pDevice->Descriptor.bNumConfigurations;
	pDescriptor->idProduct			= pDevice->Descriptor.idProduct;
	pDescriptor->idVendor			= pDevice->Descriptor.idVendor;
	pDescriptor->iManufacturer		= pDevice->Descriptor.iManufacturer;
	pDescriptor->iProduct			= pDevice->Descriptor.iProduct;
	pDescriptor->iSerialNumber		= pDevice->Descriptor.iSerialNumber;

	return TRUE;
}