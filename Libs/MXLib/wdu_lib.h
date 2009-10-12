#ifndef _WD_USB_H_
#define _WD_USB_H_

#include "windrvr.h"

#if defined(__cplusplus)
    extern "C" {
#endif

typedef PVOID WDU_DEVICE_HANDLE;
typedef PVOID WDU_DRIVER_HANDLE; 

// User Callbacks

// Function typedef: WDU_ATTACH_CALLBACK()
//   WinDriver calls this function with any new device that is attached, 
//   matches the given criteria, and if WD_ACKNOWLEDGE was passed to WDU_Init()
//   in dwOptions - not controlled yet by another driver.
//   This callback is called once for each matching interface.
// Parameters:
//   [in] hDevice: a unique identifier for the device/interface
//   [in] pDeviceInfo: ptr to device configuration details, this ptr is valid 
//   until the end of the function
//   [in] pUserData: ptr that was passed to WDU_Init
// Return Value:
//   if WD_ACKNOWLEDGE was passed to WDU_Init(), the implementor should check & 
//   return if he wants to control the device
typedef BOOL (WINAPI *WDU_ATTACH_CALLBACK)(WDU_DEVICE_HANDLE hDevice, 
    WDU_DEVICE *pDeviceInfo, PVOID pUserData);

// Function typedef: WDU_DETACH_CALLBACK()
//   WinDriver calls this function when a controlled device has been detached 
//   from the system.
// Parameters:
//   [in] hDevice: a unique identifier for the device/interface
//   [in] pUserData: ptr that was passed to WDU_Init (in the event table)
// Return Value:
//   None.
typedef void (WINAPI *WDU_DETACH_CALLBACK)(WDU_DEVICE_HANDLE hDevice, PVOID pUserData);

// Function typedef: WDU_POWER_CHANGE_CALLBACK()
//   (currently (CE.Net 4.10) pRegisterNotificationRoutine will pass only 
//   USB_CLOSE_DEVICE event (see microsoft documentation))
// Parameters:
//   [in] hDevice: a unique identifier for the device/interface
//   [in] dwPowerState: number of the power state selected
//   [in] pUserData: ptr that was passed to WDU_Init (in the event table)
// Return Value:
//   TRUE/FALSE; Currently there is no significance to the return value.
typedef BOOL (WINAPI *WDU_POWER_CHANGE_CALLBACK)(WDU_DEVICE_HANDLE hDevice, DWORD dwPowerState, PVOID pUserData);

// API Functions

typedef struct
{
    WDU_ATTACH_CALLBACK pfDeviceAttach;
    WDU_DETACH_CALLBACK pfDeviceDetach;
    WDU_POWER_CHANGE_CALLBACK pfPowerChange;
    PVOID pUserData; // pointer to pass in each callback
} WDU_EVENT_TABLE;

// Function: WDU_Init()
//   Starts listening to devices matching a criteria, and registers
//   notification callbacks for those devices
// Parameters:
//   [out] phDriver: handle to this registration of events & criteria
//   [in] pMatchTables: array of match tables defining the devices-criteria
//   [in] dwNumMatchTables: number of elements in pMatchTables
//   [in] pEventTable: notification callbackss when the device's status changes
//   [in] sLicense: WinDriver's license string
//   [in] dwOptions: can be 0 or:
//          WD_ACKNOWLEDGE - The user can seize control over the device in 
//                           WDU_ATTACH_CALLBACK return value
// Return Value:
//   WinDriver Error Code
DWORD DLLCALLCONV WDU_Init(WDU_DRIVER_HANDLE *phDriver,
    WDU_MATCH_TABLE *pMatchTables, DWORD dwNumMatchTables,
    WDU_EVENT_TABLE *pEventTable, const char *sLicense, DWORD dwOptions);

// Function: WDU_Uninit()
//   Stops listening to devices matching the criteria, and unregisters the 
//   notification callbacks for those devices
// Parameters:
//   [in] hDriver: handle to the registration, received from WDU_Init
// Return Value:
//   None.
void DLLCALLCONV WDU_Uninit(WDU_DRIVER_HANDLE hDriver);

// Function: WDU_GetDeviceInfo()
//   Gets configuration information from the device including all the descriptors
//   in an WDU_DEVICE struct. The caller should free ppDeviceInfo after the use.
// Parameters:
//   [in] hDevice: a unique identifier for the device/interface
//   [out] ppDeviceInfo: ptr to ptr to a buffer containing the device information
// Return Value:
//   WinDriver Error Code
DWORD DLLCALLCONV WDU_GetDeviceInfo(WDU_DEVICE_HANDLE hDevice, WDU_DEVICE **ppDeviceInfo);

// Function: WDU_SetInterface()
//   Sets the alternate setting for the specified interface
// Parameters:
//   [in] hDevice: a unique identifier for the device/interface
//   [in] dwInterfaceNum: the interface's number
//   [in] dwAlternateSetting: the desired alternate setting value
// Return Value:
//   WinDriver Error Code
DWORD DLLCALLCONV WDU_SetInterface(WDU_DEVICE_HANDLE hDevice, DWORD dwInterfaceNum, DWORD dwAlternateSetting);

// NOT IMPLEMENTED YET
DWORD DLLCALLCONV WDU_SetConfig(WDU_DEVICE_HANDLE hDevice, DWORD dwConfigNum);
// NOT IMPLEMENTED YET

// Function: WDU_ResetPipe()
//   Resets a pipe
// Parameters:
//   [in] hDevice: a unique identifier for the device/interface
//   [in] dwPipeNum: pipe number
// Return Value:
//   WinDriver Error Code
DWORD DLLCALLCONV WDU_ResetPipe(WDU_DEVICE_HANDLE hDevice, DWORD dwPipeNum);

// Function: WDU_ResetDevice()
//   Resets a device
// Parameters:
//   [in] hDevice: a unique identifier for the device/interface
// Return Value:
//   WinDriver Error Code
DWORD DLLCALLCONV WDU_ResetDevice(WDU_DEVICE_HANDLE hDevice);

// Function: WDU_Transfer()
//   Transfers data to/from a device
// Parameters:
//   [in] hDevice: a unique identifier for the device/interface
//   [in] dwPipeNum: number of the pipe through which the data is transferred
//   [in] fRead: TRUE for read, FALSE for write
//   [in] dwOptions: same like old API
//   [in] pBuffer: location of the data buffer
//   [in] dwBufferSize: number of the bytes to transfer
//   [out] pdwBytesTransferred: number of bytes actually transferred
//   [in] pSetupPacket: 8-bytes packet to transfer to control pipes
//   [in] dwTimeout: time in miliseconds to complete the transfer
// Return Value:
//   WinDriver Error Code
DWORD DLLCALLCONV WDU_Transfer(WDU_DEVICE_HANDLE hDevice, DWORD dwPipeNum, DWORD fRead, 
    DWORD dwOptions, PVOID pBuffer, DWORD dwBufferSize, PDWORD pdwBytesTransferred, 
    PBYTE pSetupPacket, DWORD dwTimeout);

// Function: WDU_HaltTransfer()
//   Halts the transfer on the specified pipe (only one simultaneous transfer
//   per-pipe is allowed by WinDriver)
// Parameters:
//   [in] hDevice: a unique identifier for the device/interface
//   [in] dwPipeNum: pipe number
// Return Value:
//   WinDriver Error Code
DWORD DLLCALLCONV WDU_HaltTransfer(WDU_DEVICE_HANDLE hDevice, DWORD dwPipeNum);

//
// low level functions
//

// Function: WDU_GetDeviceData()
//   Gets configuration information from the device including all the descriptors
//   in an WDU_DEVICE struct and to a user-allocated buffer. Passing 0 in pBuf
//   will do nothing but return in pdwBytes the needed size of the buffer for the data.
// Parameters:
//   [in] hDevice: a unique identifier for the device/interface
//   [in] pBuf: location of the data buffer
//   [in/out] pdwBytes: if pBuf is 0 - returns the data size, else pBuf's size
// Return Value:
//   WinDriver Error Code
DWORD DLLCALLCONV WDU_GetDeviceData(WDU_DEVICE_HANDLE hDevice, PVOID pBuf, DWORD *pdwBytes);

//
// simplified transfers
//

DWORD DLLCALLCONV WDU_TransferDefaultPipe(WDU_DEVICE_HANDLE hDevice, DWORD fRead, 
    DWORD dwOptions, PVOID pBuffer, DWORD dwBufferSize, PDWORD pdwBytesTransferred, 
    PBYTE pSetupPacket, DWORD dwTimeout);

DWORD DLLCALLCONV WDU_TransferBulk(WDU_DEVICE_HANDLE hDevice, DWORD dwPipeNum, DWORD fRead, 
    DWORD dwOptions, PVOID pBuffer, DWORD dwBufferSize, PDWORD pdwBytesTransferred, 
    DWORD dwTimeout);

DWORD DLLCALLCONV WDU_TransferIsoch(WDU_DEVICE_HANDLE hDevice, DWORD dwPipeNum, DWORD fRead, 
    DWORD dwOptions, PVOID pBuffer, DWORD dwBufferSize, PDWORD pdwBytesTransferred, 
    DWORD dwTimeout);

DWORD DLLCALLCONV WDU_TransferInterrupt(WDU_DEVICE_HANDLE hDevice, DWORD dwPipeNum, DWORD fRead, 
    DWORD dwOptions, PVOID pBuffer, DWORD dwBufferSize, PDWORD pdwBytesTransferred, 
    DWORD dwTimeout);

#ifdef __cplusplus
}
#endif

#endif
