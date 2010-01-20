/*---------------------------------------------------------------------------
* Copyright (C) 2009, Freescale Semiconductor, Inc. All Rights Reserved.
* THIS SOURCE CODE, AND ITS USE AND DISTRIBUTION, IS SUBJECT TO THE TERMS
* AND CONDITIONS OF THE APPLICABLE LICENSE AGREEMENT 
*--------------------------------------------------------------------------*/    


#include <DriverSpecs.h>
__user_code  
       
#include <windows.h>

#include <conio.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <time.h>

#include "devioctl.h"
#include "public.h"
#include "strsafe.h"


#pragma warning(disable:4200)  // 
#pragma warning(disable:4201)  // nameless struct/union
#pragma warning(disable:4214)  // bit field types other than int

#include <setupapi.h>
#include <basetyps.h>
#include "usbdi.h"

#pragma warning(default:4200)
#pragma warning(default:4201)
#pragma warning(default:4214)
  

#define NOISY(_x_) printf _x_ ;
#define MAX_LENGTH 256
#define MAX_DEVICE 8

#define IMX51_VID	0x15A2
#define	IMX51_PID	0x0041

//#define IMX35_VIP   0x15A2
#define IMX35_PID   0x0030

//
// i.MX type
//
typedef enum 
{
	IMX_UNKNOW = -1,
	IMX_MX25_TO1 = 0,
	IMX_MX25_TO11,
	IMX_MX27_TO1,
	IMX_MX27_TO2,
	IMX_MX31_TO1,
	IMX_MX31_TO2,
	IMX_MX31_TO201,
	IMX_MX32,
	IMX_MX35_TO1,
	IMX_MX35_TO2,
	IMX_MX37,
	IMX_MX51_TO1,
	IMX_MX51_TO2,
	IMX_MAX,
} IMX_T;

char inPipe[MAX_LENGTH] = "PIPE01";     // pipe name for bulk input pipe on our test board
char outPipe[MAX_LENGTH] = "PIPE00";    // pipe name for bulk output pipe on our test board
char completeDeviceName[MAX_LENGTH] = "";  //complete device path name
char completeInPipeName[MAX_LENGTH] = "";  //complete in pipe name for read operation
char completeOutPipeName[MAX_LENGTH] = "";  //complete out pipe name for write operation
int NumIMXDevices = 0;

BOOL fDumpUsbConfig = FALSE;    // flags set in response to console command line switches
BOOL fDumpReadData = FALSE;
BOOL fRead = FALSE;
BOOL fWrite = FALSE;
HANDLE hIMXDev[MAX_DEVICE];
HANDLE hRead[MAX_DEVICE];
HANDLE hWrite[MAX_DEVICE];

int gDebugLevel = 1;      // higher == more verbose, default is 1, 0 turns off all

ULONG IterationCount = 1; //count of iterations of the test we are to perform
int WriteLen = 0;         // #bytes to write
int ReadLen = 0;          // #bytes to read


// functions

HANDLE
WINAPI
OpenOneIMXDevice (
    __in  HDEVINFO                    HardwareDeviceInfo,
    __in  PSP_DEVICE_INTERFACE_DATA   DeviceInfoData,
    __in  PSTR                        devName
    )
/*++
Routine Description:

    Given the HardwareDeviceInfo, representing a handle to the plug and
    play information, and deviceInfoData, representing a specific usb device,
    open that device and fill in the device path in the given devName parameter.

Arguments:

    HardwareDeviceInfo:  handle to info obtained from Pnp mgr via SetupDiGetClassDevs()
    DeviceInfoData:      ptr to info obtained via SetupDiEnumDeviceInterfaces()

Return Value:

    return HANDLE if the open and initialization was successfull,
    else INVLAID_HANDLE_VALUE.

--*/
{
    PSP_DEVICE_INTERFACE_DETAIL_DATA     functionClassDeviceData = NULL;
    ULONG                                predictedLength = 0;
    ULONG                                requiredLength = 0;
    HANDLE                               hOut = INVALID_HANDLE_VALUE;

    //
    // allocate a function class device data structure to receive the
    // goods about this particular device.
    //
    SetupDiGetDeviceInterfaceDetail (
            HardwareDeviceInfo,
            DeviceInfoData,
            NULL, // probing so no output buffer yet
            0, // probing so output buffer length of zero
            &requiredLength,
            NULL); // not interested in the specific dev-node


    predictedLength = requiredLength;
    // sizeof (SP_FNCLASS_DEVICE_DATA) + 512;

    functionClassDeviceData = (PSP_DEVICE_INTERFACE_DETAIL_DATA)malloc (predictedLength);
    if(NULL == functionClassDeviceData) {
        return INVALID_HANDLE_VALUE;
    }
    functionClassDeviceData->cbSize = sizeof (SP_DEVICE_INTERFACE_DETAIL_DATA);

    //
    // Retrieve the information from Plug and Play.
    //
    if (! SetupDiGetDeviceInterfaceDetail (
               HardwareDeviceInfo,
               DeviceInfoData,
               functionClassDeviceData,
               predictedLength,
               &requiredLength,
               NULL)) {
        free( functionClassDeviceData );
        return INVALID_HANDLE_VALUE;
    }

    (void)StringCchCopy( devName, MAX_LENGTH, functionClassDeviceData->DevicePath) ;
    printf( "Attempting to open %s\n", devName );

    hOut = CreateFile (
                  functionClassDeviceData->DevicePath,
                  GENERIC_READ | GENERIC_WRITE,
                  FILE_SHARE_READ | FILE_SHARE_WRITE,
                  NULL, // no SECURITY_ATTRIBUTES structure
                  OPEN_EXISTING, // No special create flags
                  0, // No special attributes
                  NULL); // No template file

    if (INVALID_HANDLE_VALUE == hOut) {
                printf( "FAILED to open %s\n", devName );
    }
        
    free( functionClassDeviceData );
    return hOut;
}


BOOL
WINAPI 
USB_OpenLocateDevice( 
    int    *iMXNum, 
    int    *iMXType
    )
{
    ULONG    NumberDevices;
    HDEVINFO    hardwareDeviceInfo;
    SP_DEVICE_INTERFACE_DATA    deviceInfoData;
    PUSB_DEVICE_DESCRIPTOR    pUsbDeviceDesc;
    char buf[256];
    int siz, nBytes;
    UINT success;
    ULONG    i;
    
	*iMXType = (int)IMX_UNKNOW;
	*iMXNum = 0;
	
    siz = sizeof(buf);

    // Open a handle to the i.MX device node with USB connection available.
    // The handle is get by the i.MX device interface GUID 
    // GUID_CLASS_IMX_USB with WDF API SetupDiGetClassDevs()

    hardwareDeviceInfo = SetupDiGetClassDevs (
                            (LPGUID)& GUID_CLASS_IMX_USB,
                             NULL,
                             NULL,
                            ( DIGCF_PRESENT | DIGCF_DEVICEINTERFACE));
                            
    if (hardwareDeviceInfo == INVALID_HANDLE_VALUE) {
        return FALSE;
    }


    // Suppose we have 8 i.MX devices with USB connection available. Will
    // update the value according to actual number of i.MX devices connected
    // by USB cable
    NumberDevices = 8;
	
	for (i = 0; i < MAX_DEVICE; i++) {
	    hIMXDev[i] = INVALID_HANDLE_VALUE;
		hRead[i] = INVALID_HANDLE_VALUE;
		hWrite[i] = INVALID_HANDLE_VALUE;
	}

    // Open i.MX devices with USB connection and get the device handles
    
    deviceInfoData.cbSize = sizeof (SP_DEVICE_INTERFACE_DATA);
                
    for (i = 0; i < NumberDevices; i++)  {
        if (SetupDiEnumDeviceInterfaces (hardwareDeviceInfo,
                                         0, // We don't care about specific PDOs
                                         (LPGUID)& GUID_CLASS_IMX_USB,
                                         i,
                                         &deviceInfoData)) {

            hIMXDev[i] = OpenOneIMXDevice (hardwareDeviceInfo, &deviceInfoData, completeDeviceName);
            
            if ( hIMXDev[i] != INVALID_HANDLE_VALUE ) {
               
               // Get device descriptor the get the i.MX type 
               // (i.MX51, i.MX35, etc) according to VID&PID
               success = DeviceIoControl(hIMXDev[i],
                    IOCTL_IMXDEVICE_GET_DEVICE_DESCRIPTOR,
                    buf,
                    siz,
                    buf,
                    siz,
                    (PULONG) &nBytes,
                    NULL);
                    
               if (success) {
                   pUsbDeviceDesc = (PUSB_DEVICE_DESCRIPTOR)buf;
                   if((pUsbDeviceDesc->idVendor == IMX51_VID) &&
                              (pUsbDeviceDesc->idProduct == IMX51_PID) )
                       *iMXType = (int)IMX_MX51_TO2;
                   else if(pUsbDeviceDesc->idProduct == IMX35_PID)
					   *iMXType = (int)IMX_MX35_TO2;
				   else
                       *iMXType = (int)IMX_UNKNOW;                          
               } else {
                   *iMXType = (int)IMX_UNKNOW;
                   *iMXNum = 0;                   
                   return FALSE;                   
               }
               
               // Get handles for read operation
               (void)StringCchCopy(completeInPipeName, MAX_LENGTH, completeDeviceName) ;
               (void) StringCchCat (completeInPipeName, MAX_LENGTH, "\\" );                      

               if(FAILED(StringCchCat (completeInPipeName, MAX_LENGTH, inPipe))) {
                   NOISY(("Failed to open handle - possibly long filename\n"));
                   *iMXType = (int)IMX_UNKNOW;
                   *iMXNum = 0;
                   return FALSE;
               }

               printf("completeInPipeName = (%s)\n", completeInPipeName);

               hRead[i] = CreateFile(completeInPipeName,
                              GENERIC_WRITE | GENERIC_READ,
                              FILE_SHARE_WRITE | FILE_SHARE_READ,
                              NULL,
                              OPEN_EXISTING,
                              0,
                              NULL);

               if (hRead[i] == INVALID_HANDLE_VALUE) {
                   NOISY(("Failed to open (%s) = %d", completeInPipeName, GetLastError()));
                   *iMXType = (int)IMX_UNKNOW;
                   *iMXNum = 0;
                   return FALSE;
               } else {
                   NOISY(("Opened successfully.\n"));
               }
               
               // Get handles for write operation
               (void)StringCchCopy(completeOutPipeName, MAX_LENGTH, completeDeviceName) ;
               (void) StringCchCat (completeOutPipeName, MAX_LENGTH, "\\" );                      

               if(FAILED(StringCchCat (completeOutPipeName, MAX_LENGTH, outPipe))) {
                   NOISY(("Failed to open handle - possibly long filename\n"));
                   *iMXType = (int)IMX_UNKNOW;
                   *iMXNum = 0;
                   return FALSE;
               }

               printf("completeOutPipeName = (%s)\n", completeOutPipeName);

               hWrite[i] = CreateFile(completeOutPipeName,
                              GENERIC_WRITE | GENERIC_READ,
                              FILE_SHARE_WRITE | FILE_SHARE_READ,
                              NULL,
                              OPEN_EXISTING,
                              0,
                              NULL);

               if (hWrite[i] == INVALID_HANDLE_VALUE) {
                   NOISY(("Failed to open (%s) = %d", completeOutPipeName, GetLastError()));
                   *iMXType = (int)IMX_UNKNOW;
                   *iMXNum = 0;
                   return FALSE;
               } else {
                   NOISY(("Opened successfully.\n"));
               }
               
                                     
            } else {
               *iMXType = (int)IMX_UNKNOW;
               *iMXNum = 0;
               return FALSE;                              
            }
            
            *iMXNum = i + 1;
            NumIMXDevices = i + 1;            
         } else {
            if (ERROR_NO_MORE_ITEMS == GetLastError()) {
               break;
            }
         }
    }    
                            
    // SetupDiDestroyDeviceInfoList() destroys a device information set
    // and frees all associated memory.

    SetupDiDestroyDeviceInfoList (hardwareDeviceInfo); 
    
    return TRUE;                                                                             
}

void WINAPI USB_CloseLocateDevice(void)
{
    int numDevices;
    int i;
    
    numDevices = NumIMXDevices;
    
    for (i = 0; i < numDevices; i++)
    {
        // IOCTL IOCTL_IMXDEVICE_RESET_DEVICE will stop all pipes on target
        // devices
		if (hIMXDev[i] != INVALID_HANDLE_VALUE)
            DeviceIoControl(hIMXDev[i],
                        IOCTL_IMXDEVICE_RESET_DEVICE,
                        NULL,
                        0,
                        NULL,
                        0,
                        NULL,
                        NULL);
    }
}

BOOL
WINAPI
ReadFromUsb (
    DWORD    size,
    int    id,
    PVOID    pBuffer,
    int    timeout
)
/*++

Routine Description:

    This function calls for Win API ReadFile() to read data from i.MX devices
    by USB connection.

Arguments:

    size - The data size required to read
    id - i.MX device id
    Lenght - Length of the data buffer associated with the request.
    timeout - Time limitation for the read operation     
Return Value:
    TRUE for success, FALSE for failure.

--*/
{
    int    nBytesRead; // for bytes actually read
    UINT   success = FALSE;
    
	if(hRead[id] != INVALID_HANDLE_VALUE) 
        success = ReadFile(hRead[id], pBuffer, size, (PULONG) &nBytesRead, NULL);
    
    if(success)
        return TRUE;
    else 
        return FALSE;    
    
}

BOOL WINAPI WriteToUsb (
    DWORD dwSize, 
    PVOID pBuffer,
    int id)
  /*++

Routine Description:

    This function calls for Win API WriteFile() to write data to i.MX devices
    by USB connection.

Arguments:

    dwsize - The data size required to write
    id - i.MX device id
    pBuffer - Points to buffer to be written
        
Return Value:
    TRUE for success, FALSE for failure.

--*/
{
    int    nBytesWrite; // for bytes actually read
    UINT   success;
    
	if(hWrite[id] != INVALID_HANDLE_VALUE)
        success = WriteFile(hWrite[id], pBuffer, dwSize, (PULONG) &nBytesWrite, NULL);
    
    if(success)
        return TRUE;
    else 
        return FALSE;    
}
