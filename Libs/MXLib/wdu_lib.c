////////////////////////////////////////////////////////////////
// File - wdu_lib.c
//
// WinDriver USB API Declarations & Implementations
//
// Copyright (c) 2003 Jungo Ltd.  http://www.jungo.com 
////////////////////////////////////////////////////////////////

#include "wdu_lib.h"
#include "windrvr_events.h"
#include "status_strings.h"
#include "utils.h"
#include <stdarg.h>
#include <stdio.h>

// Print Functions

#if !defined(TRACE)
int __cdecl TRACE(const char *fmt, ...) 
{ 
    #if defined(DEBUG)
        va_list argp;
        va_start(argp, fmt);
        vfprintf(stderr, fmt, argp);
        va_end(argp);
    #endif
    return 0;
}
#endif

#if !defined(ERR)
int __cdecl ERR(const char *fmt, ...) 
{ 
    #if defined(DEBUG)
        va_list argp;
        va_start(argp, fmt);
        fprintf(stderr, "WDERROR: ");
        vfprintf(stderr, fmt, argp);
        va_end(argp);
    #endif
    return 0;
}
#endif

// Structures

#define WDU_DEVLIST_TIMEOUT 30 // in seconds

typedef struct
{
    HANDLE hWD; // old API WinDriver handle
    WDU_EVENT_TABLE EventTable;
    HANDLE hEvents;
} DRIVER_CTX;
    
typedef struct
{
    DRIVER_CTX *pDriverCtx;
    WDU_DEVICE *pDevice; // not fixed size => ptr 
    DWORD dwUniqueID;
} DEVICE_CTX;

typedef struct _WDU_DEVICE_LIST_ITEM
{
    struct _WDU_DEVICE_LIST_ITEM *next;
    DEVICE_CTX *pDeviceCtx;
} WDU_DEVICE_LIST_ITEM; 

typedef struct
{
    WDU_DEVICE_LIST_ITEM *pHead;
    HANDLE hEvent;    
    int iRefCount;
} WDU_DEVICE_LIST;

WDU_DEVICE_LIST DevList; // global devices list

// Private Functions Prototypes

DWORD AddDeviceToDevList(DEVICE_CTX *pDeviceCtx);
DWORD RemoveDeviceFromDevList(DWORD dwUniqueID, DEVICE_CTX **ppDeviceCtx);
DWORD RemoveAllDevicesFromDevList(DRIVER_CTX *pDriverCtx);
DWORD FindDeviceByUniqueID(DWORD dwUniqueID, DEVICE_CTX **ppDeviceCtx);
DWORD FindDeviceByCtx(DEVICE_CTX *pDeviceCtx);

// Translate WD_functions into IOCTLs

#define PARAMS_SET(param) Params.param = param
#define GET_HWD(h) (((DEVICE_CTX *)(h))->pDriverCtx->hWD)

// unique ID is passed in IOCTLs to identify the device/interface instead of
// hDevice like in the old API
#define PARAMS_INIT(T) \
    T Params; \
    BZERO(Params); \
    if (!hDevice || FindDeviceByCtx((DEVICE_CTX *)hDevice) != WD_STATUS_SUCCESS) \
        return WD_DEVICE_NOT_FOUND; \
    Params.dwUniqueID = ((DEVICE_CTX *)hDevice)->dwUniqueID;


DWORD DLLCALLCONV WDU_SetInterface(WDU_DEVICE_HANDLE hDevice, DWORD dwInterfaceNum,
    DWORD dwAlternateSetting)
{
    PARAMS_INIT(WDU_SET_INTERFACE);
    PARAMS_SET(dwInterfaceNum);
    PARAMS_SET(dwAlternateSetting);
    return WD_USetInterface(GET_HWD(hDevice), &Params);
}

// currently not implemented
DWORD DLLCALLCONV WDU_SetConfig(WDU_DEVICE_HANDLE hDevice, DWORD dwConfigNum);

DWORD DLLCALLCONV WDU_ResetPipe(WDU_DEVICE_HANDLE hDevice, DWORD dwPipeNum)
{
    PARAMS_INIT(WDU_RESET_PIPE);
    PARAMS_SET(dwPipeNum);
    return WD_UResetPipe(GET_HWD(hDevice), &Params);
}

// currently not implemented
DWORD DLLCALLCONV WDU_ResetDevice(WDU_DEVICE_HANDLE hDevice);

DWORD DLLCALLCONV WDU_Transfer(WDU_DEVICE_HANDLE hDevice, DWORD dwPipeNum, DWORD fRead, 
    DWORD dwOptions, PVOID pBuffer, DWORD dwBufferSize, PDWORD pdwBytesTransferred, 
    PBYTE pSetupPacket, DWORD dwTimeout)
{
    DWORD dwStatus;
    PARAMS_INIT(WDU_TRANSFER);
    PARAMS_SET(dwPipeNum);
    PARAMS_SET(fRead);
    PARAMS_SET(dwOptions);
    PARAMS_SET(pBuffer);
    PARAMS_SET(dwBufferSize);
    if (pSetupPacket)
        memcpy(&Params.SetupPacket, pSetupPacket, 8);
    PARAMS_SET(dwTimeout);
    dwStatus = WD_UTransfer(GET_HWD(hDevice), &Params);
    *pdwBytesTransferred = Params.dwBytesTransferred;
    return dwStatus;
}

DWORD DLLCALLCONV WDU_HaltTransfer(WDU_DEVICE_HANDLE hDevice, DWORD dwPipeNum)
{
    PARAMS_INIT(WDU_HALT_TRANSFER);
    PARAMS_SET(dwPipeNum);
    return WD_UHaltTransfer(GET_HWD(hDevice), &Params);
}

// User-mode wrappers for kernel functions

void EventHandler(WD_EVENT *pEvent, void *pDriverContext);

DWORD DLLCALLCONV WDU_Init(WDU_DRIVER_HANDLE *phDriver, 
    WDU_MATCH_TABLE *pMatchTables, DWORD dwNumMatchTables, 
    WDU_EVENT_TABLE *pEventTable, const char *sLicense, DWORD dwOptions)
{
    DWORD dwStatus;
    DRIVER_CTX *pDriverCtx = NULL;
    WD_VERSION ver;
    WD_EVENT *event = NULL;
    WD_LICENSE lic;
   
    *phDriver = INVALID_HANDLE_VALUE;

    pDriverCtx = (DRIVER_CTX *) calloc(1, sizeof(DRIVER_CTX));
    if (!pDriverCtx)
    {
        ERR("WDU_Init: cannot malloc memory\n");
        dwStatus = WD_INSUFFICIENT_RESOURCES;
        goto Error;
    }

    // init the device list event
    if (DevList.iRefCount == 0)
    {
        DevList.iRefCount++;
        
        dwStatus = OsEventCreate(&DevList.hEvent);
        if (dwStatus)
        {
            ERR("WDU_Init: cannot create event: dwStatus (0x%x) - %s\n", 
                dwStatus, Stat2Str(dwStatus));
            goto Error;
        }
        dwStatus = OsEventSignal(DevList.hEvent);
        if (dwStatus)
        {
            ERR("WDU_Init: error signaling device list event: dwStatus (0x%x) - %s\n", 
                dwStatus, Stat2Str(dwStatus));
            goto Error;
        }
    }
    
    pDriverCtx->hWD = INVALID_HANDLE_VALUE;
    pDriverCtx->hWD = WD_Open();

    // Check whether handle is valid and version OK
    if (pDriverCtx->hWD==INVALID_HANDLE_VALUE) 
    {
        ERR("WDU_Init: cannot open " WD_PROD_NAME " device\n");
        dwStatus = WD_SYSTEM_INTERNAL_ERROR;
        goto Error;
    }

    strcpy_s(lic.cLicense, sizeof(lic.cLicense), sLicense);
    WD_License(pDriverCtx->hWD, &lic);

    BZERO(ver);
    dwStatus = WD_Version(pDriverCtx->hWD, &ver);
    if ((dwStatus != WD_STATUS_SUCCESS) || (ver.dwVer<WD_VER)) 
    {
        ERR("WDU_Init: incorrect " WD_PROD_NAME " version\n");
        if (!dwStatus)
            dwStatus = WD_INCORRECT_VERSION;
        goto Error;
    }

    pDriverCtx->EventTable = *pEventTable;

    if (pEventTable->pfDeviceAttach)
    {
        DWORD dwAction;
        dwAction = WD_INSERT |
            (pEventTable->pfDeviceDetach ?  WD_REMOVE : 0) |
            (pEventTable->pfPowerChange ? WD_POWER_CHANGED_D0 | 
                                        WD_POWER_CHANGED_D1 |
                                        WD_POWER_CHANGED_D2 |
                                        WD_POWER_CHANGED_D3 |
                                        WD_POWER_SYSTEM_WORKING |
                                        WD_POWER_SYSTEM_SLEEPING1 |
                                        WD_POWER_SYSTEM_SLEEPING2 |
                                        WD_POWER_SYSTEM_SLEEPING3 |
                                        WD_POWER_SYSTEM_HIBERNATE |
                                        WD_POWER_SYSTEM_SHUTDOWN : 0); 
        event = UsbEventCreate(pMatchTables, dwNumMatchTables, dwOptions,
            dwAction);
        if (!event)
        {
            ERR("WDU_Init: cannot malloc memory\n");
            dwStatus = WD_INSUFFICIENT_RESOURCES;
            goto Error;
        }

        dwStatus = EventRegister(&pDriverCtx->hEvents, pDriverCtx->hWD, event,
            EventHandler, pDriverCtx);
        if (dwStatus)
        {
            ERR("WDU_Init: EventRegister failed with dwStatus (0x%x) - %s\n", 
                dwStatus, Stat2Str(dwStatus));
            goto Error;
        }
    }

    *phDriver = pDriverCtx;
    goto Exit;

Error:
    if (pDriverCtx)
        WDU_Uninit(pDriverCtx);
Exit:
    if (event)
        EventFree(event);
    return dwStatus;
}

void DLLCALLCONV WDU_Uninit(WDU_DRIVER_HANDLE hDriver)
{
    DRIVER_CTX *pDriverCtx = (DRIVER_CTX *)hDriver;

    if (pDriverCtx && hDriver != INVALID_HANDLE_VALUE)
    {
        if (pDriverCtx->hWD) 
        {
            if (pDriverCtx->hEvents)
                EventUnregister(pDriverCtx->hEvents);
            WD_Close(pDriverCtx->hWD);
        }
        RemoveAllDevicesFromDevList(pDriverCtx);
        free (pDriverCtx);
    }

    DevList.iRefCount--;
    if (DevList.iRefCount == 0)
        OsEventClose(DevList.hEvent);
}

void EventHandler(WD_EVENT *pEvent, void *pDriverContext)
{
    DRIVER_CTX *pDriverCtx = (DRIVER_CTX *)pDriverContext;
    DEVICE_CTX *pDeviceCtx, *pDummyDeviceCtx;
    WDU_DEVICE_HANDLE hDevice;
    BOOL bControlDevice = FALSE, bChangePower;
    DWORD dwStatus;

    TRACE("EventHandler: got event %d handle 0x%x\n", pEvent->dwAction, pEvent->handle);
    switch (pEvent->dwAction)
    {
    case WD_INSERT:
        // create device context
        pDeviceCtx = (DEVICE_CTX *)calloc(1, sizeof(DEVICE_CTX));
        if (!pDeviceCtx)
        {
            ERR("EventHandler: cannot alloc memory\n");
            return;
        }
        
        // DEVICE_CTX * is used as WDU_DEVICE_HANDLE 
        hDevice = pDeviceCtx; 

        pDeviceCtx->dwUniqueID = pEvent->u.Usb.dwUniqueID;
        pDeviceCtx->pDriverCtx = pDriverCtx;

        // add the device handle to the device list for future IOCTLs
        dwStatus = AddDeviceToDevList(pDeviceCtx);
        if (dwStatus)
        {
            free(pDeviceCtx);
            return;
        }

        // get device info
        dwStatus = WDU_GetDeviceInfo(hDevice, &pDeviceCtx->pDevice);
        if (dwStatus)
        {
            ERR("EventHandler: unable to get device info for device"
                " handle 0x%x dwUniqueID 0x%x\n", 
                hDevice, pDeviceCtx->dwUniqueID);
            RemoveDeviceFromDevList(pEvent->u.Usb.dwUniqueID, &pDummyDeviceCtx);
            free(pDeviceCtx);
            return;
        }

        bControlDevice = pDriverCtx->EventTable.pfDeviceAttach(hDevice, 
            pDeviceCtx->pDevice, pDriverCtx->EventTable.pUserData);
        if (!bControlDevice)
        {
            TRACE("EventHandler: bControlDevice==FALSE; pDriverCtx 0x%x\n", pDriverCtx);
            RemoveDeviceFromDevList(pEvent->u.Usb.dwUniqueID, &pDummyDeviceCtx);
            free(pDeviceCtx->pDevice); // because GetDeviceInfo calls malloc
            free(pDeviceCtx);
        }

        pEvent->dwOptions |= bControlDevice ? WD_ACCEPT_CONTROL : 0;
        break;
    case WD_REMOVE:
        dwStatus = RemoveDeviceFromDevList(pEvent->u.Usb.dwUniqueID, &pDeviceCtx);
        if (dwStatus)
            // device is not mine or may has been closed by WDU_Uninit
            break;

        // DEVICE_CTX * is used as WDU_DEVICE_HANDLE
        pDriverCtx->EventTable.pfDeviceDetach((WDU_DEVICE_HANDLE) pDeviceCtx, pDriverCtx->EventTable.pUserData);

        free(pDeviceCtx->pDevice); // because GetDeviceInfo calls malloc
        free(pDeviceCtx);

        break;
    case WD_POWER_CHANGED_D0:
    case WD_POWER_CHANGED_D1:
    case WD_POWER_CHANGED_D2:
    case WD_POWER_CHANGED_D3:
    case WD_POWER_SYSTEM_WORKING:
    case WD_POWER_SYSTEM_SLEEPING1:
    case WD_POWER_SYSTEM_SLEEPING2:
    case WD_POWER_SYSTEM_SLEEPING3:
    case WD_POWER_SYSTEM_HIBERNATE:
    case WD_POWER_SYSTEM_SHUTDOWN:
        dwStatus = FindDeviceByUniqueID(pEvent->u.Usb.dwUniqueID, &pDeviceCtx);
        if (dwStatus)
        {
            // device is not mine or may have been closed by WDU_Uninit
            break;
        }

        bChangePower = pDriverCtx->EventTable.pfPowerChange((WDU_DEVICE_HANDLE) pDeviceCtx, pEvent->dwAction, 
            pDriverCtx->EventTable.pUserData);
        // XXX return in the event structure if the user says it's ok to change power
        //  (not implemented yet)
        break;
    }
}

// ppDeviceInfo is set to point to an allocated buffer containing the info.
// The caller should free the buffer after the use.
DWORD DLLCALLCONV WDU_GetDeviceInfo(WDU_DEVICE_HANDLE hDevice, WDU_DEVICE **ppDeviceInfo)
{
    DWORD dwStatus;

    PARAMS_INIT(WDU_GET_DEVICE_DATA);
    // first call with pBuf NULL, return dwBytes
    dwStatus = WD_UGetDeviceData(GET_HWD(hDevice), &Params);
    if (dwStatus != WD_STATUS_SUCCESS)
        return dwStatus;
    *ppDeviceInfo = (WDU_DEVICE *) malloc(Params.dwBytes);
    if (!ppDeviceInfo)
    {
        ERR("WDU_GetDeviceInfo: cannot alloc memory\n");
        return WD_INSUFFICIENT_RESOURCES;
    }

    Params.pBuf = *ppDeviceInfo;
    // second call with correct pBuf and dwBytes
    dwStatus = WD_UGetDeviceData(GET_HWD(hDevice), &Params);
    return dwStatus;
}

//
// simplified transfers
//

DWORD DLLCALLCONV WDU_TransferDefaultPipe(WDU_DEVICE_HANDLE hDevice, DWORD fRead, 
    DWORD dwOptions, PVOID pBuffer, DWORD dwBufferSize, PDWORD pdwBytesTransferred, 
    PBYTE pSetupPacket, DWORD dwTimeout)
{
    return WDU_Transfer(hDevice, 0, fRead, 
    dwOptions, pBuffer, dwBufferSize, pdwBytesTransferred, 
    pSetupPacket, dwTimeout);
}

DWORD DLLCALLCONV WDU_TransferBulk(WDU_DEVICE_HANDLE hDevice, DWORD dwPipeNum, DWORD fRead, 
    DWORD dwOptions, PVOID pBuffer, DWORD dwBufferSize, PDWORD pdwBytesTransferred, 
    DWORD dwTimeout)
{
    return WDU_Transfer(hDevice, dwPipeNum, fRead, 
    dwOptions, pBuffer, dwBufferSize, pdwBytesTransferred, 
    NULL, dwTimeout);
}

DWORD DLLCALLCONV WDU_TransferIsoch(WDU_DEVICE_HANDLE hDevice, DWORD dwPipeNum, DWORD fRead, 
    DWORD dwOptions, PVOID pBuffer, DWORD dwBufferSize, PDWORD pdwBytesTransferred, 
    DWORD dwTimeout)
{
    return WDU_Transfer(hDevice, dwPipeNum, fRead, 
    dwOptions, pBuffer, dwBufferSize, pdwBytesTransferred, 
    NULL, dwTimeout);
}

DWORD DLLCALLCONV WDU_TransferInterrupt(WDU_DEVICE_HANDLE hDevice, DWORD dwPipeNum, DWORD fRead, 
    DWORD dwOptions, PVOID pBuffer, DWORD dwBufferSize, PDWORD pdwBytesTransferred, 
    DWORD dwTimeout)
{
    return WDU_Transfer(hDevice, dwPipeNum, fRead, 
    dwOptions, pBuffer, dwBufferSize, pdwBytesTransferred, 
    NULL, dwTimeout);
}

// Private Functions 

DWORD AddDeviceToDevList(DEVICE_CTX *pDeviceCtx)
{
    WDU_DEVICE_LIST_ITEM *pDevItem;
    DWORD dwStatus;

    TRACE("AddDeviceToDevList device 0x%x before 0x%x\n", pDeviceCtx, DevList);

    dwStatus = OsEventWait(DevList.hEvent, WDU_DEVLIST_TIMEOUT);
    if (dwStatus)
    {
        ERR("AddDeviceToDevList: error waiting for device list event: dwStatus (0x%x) - %s\n", 
            dwStatus, Stat2Str(dwStatus));
        return dwStatus;
    }

    pDevItem = (WDU_DEVICE_LIST_ITEM *) calloc(1, sizeof(WDU_DEVICE_LIST_ITEM));
    pDevItem->pDeviceCtx = pDeviceCtx;
    pDevItem->next = DevList.pHead;

    DevList.pHead = pDevItem;

    dwStatus = OsEventSignal(DevList.hEvent);
    if (dwStatus)
    {
        ERR("AddDeviceToDevList: error signaling device list event: dwStatus (0x%x) - %s\n", 
            dwStatus, Stat2Str(dwStatus));
    }
    return dwStatus;
}

DWORD RemoveAllDevicesFromDevList(DRIVER_CTX *pDriverCtx)
{
    DEVICE_CTX *pDeviceCtx;
    WDU_DEVICE_LIST_ITEM **ppDev = &DevList.pHead;
    WDU_DEVICE_LIST_ITEM *pDevTmp;
    DWORD dwStatus;

    TRACE("RemoveAllDevicesFromDevList: pDriverCtx 0x%x\n", pDriverCtx);

    dwStatus = OsEventWait(DevList.hEvent, WDU_DEVLIST_TIMEOUT);
    if (dwStatus)
    {
        ERR("RemoveAllDevicesFromDevList: error waiting for device list event: dwStatus (0x%x) - %s\n", 
            dwStatus, Stat2Str(dwStatus));
        return dwStatus;
    }

    while (*ppDev)
    {
        pDeviceCtx = (*ppDev)->pDeviceCtx;
        if (pDeviceCtx->pDriverCtx == pDriverCtx)
        {
            pDevTmp = *ppDev;
            *ppDev = (*ppDev)->next;

            free(pDeviceCtx->pDevice); // because GetDeviceInfo calls malloc
            free(pDeviceCtx);
            free(pDevTmp);
        }
        else
            ppDev = &(*ppDev)->next;
    }

    dwStatus = OsEventSignal(DevList.hEvent);
    if (dwStatus)
    {
        ERR("RemoveAllDevicesFromDevList: error signaling device list event: dwStatus (0x%x) - %s\n", 
            dwStatus, Stat2Str(dwStatus));
    }
    return dwStatus;
}

DWORD FindDeviceByUniqueID(DWORD dwUniqueID, DEVICE_CTX **ppDeviceCtx)
{
    WDU_DEVICE_LIST_ITEM *iter;
    DWORD dwStatus;
    BOOL Found = FALSE;

    TRACE("FindDeviceByUniqueID: DevList.pHead 0x%x, dwUniqueID 0x%x\n", DevList.pHead, dwUniqueID);
    *ppDeviceCtx = NULL;

    dwStatus = OsEventWait(DevList.hEvent, WDU_DEVLIST_TIMEOUT);
    if (dwStatus)
    {
        ERR("FindDeviceByUniqueID: error waiting for device list event: dwStatus (0x%x) - %s\n", 
            dwStatus, Stat2Str(dwStatus));
        return dwStatus;
    }

    for (iter = DevList.pHead; iter; iter = iter->next)
    {
        if (iter->pDeviceCtx->dwUniqueID == dwUniqueID)
        {
            Found = TRUE;
            break;
        }
    }

    dwStatus = OsEventSignal(DevList.hEvent);
    if (dwStatus)
    {
        ERR("FindDeviceByUniqueID: error signaling device list event: dwStatus (0x%x) - %s\n", 
            dwStatus, Stat2Str(dwStatus));
        return dwStatus;
    }

    if (Found)
        *ppDeviceCtx = iter->pDeviceCtx;
    else
        dwStatus = WD_DEVICE_NOT_FOUND;

    return dwStatus;
}

DWORD FindDeviceByCtx(DEVICE_CTX *pDeviceCtx)
{
    WDU_DEVICE_LIST_ITEM *iter;
    DWORD dwStatus;
    BOOL Found = FALSE;

    TRACE("FindDeviceByCtx: DevList.pHead 0x%x, pDeviceCtx 0x%x\n", DevList.pHead, pDeviceCtx);

    dwStatus = OsEventWait(DevList.hEvent, WDU_DEVLIST_TIMEOUT);
    if (dwStatus)
    {
        ERR("FindDeviceByCtx: error waiting for device list event: dwStatus (0x%x) - %s\n", 
            dwStatus, Stat2Str(dwStatus));
        return dwStatus;
    }

    for (iter = DevList.pHead; iter; iter = iter->next)
    {
        if (iter->pDeviceCtx == pDeviceCtx)
        {
            Found = TRUE;
            break;
        }
    }

    dwStatus = OsEventSignal(DevList.hEvent);
    if (dwStatus)
    {
        ERR("FindDeviceByCtx: error signaling device list event: dwStatus (0x%x) - %s\n", 
            dwStatus, Stat2Str(dwStatus));
        return dwStatus;
    }
    if (!Found)
        dwStatus = WD_DEVICE_NOT_FOUND;
    return dwStatus;
}

DWORD RemoveDeviceFromDevList(DWORD dwUniqueID, DEVICE_CTX **ppDeviceCtx)
{
    WDU_DEVICE_LIST_ITEM **iter, *tmp;
    DWORD dwStatus;
    BOOL Found = FALSE;

    TRACE("RemoveDeviceFromDevList: DevList 0x%x, dwUniqueID 0x%x\n", DevList, dwUniqueID);
    *ppDeviceCtx = NULL;

    dwStatus = OsEventWait(DevList.hEvent, WDU_DEVLIST_TIMEOUT);
    if (dwStatus)
    {
        ERR("RemoveDeviceFromDevList: error waiting for device list event: dwStatus (0x%x) - %s\n", 
            dwStatus, Stat2Str(dwStatus));
        return dwStatus;
    }

    for (iter = &DevList.pHead; *iter; iter = &(*iter)->next)
    {
        if ((*iter)->pDeviceCtx->dwUniqueID == dwUniqueID)
        {
            Found = TRUE;
            break;
        }
    }
    if (Found)
    {
        tmp = *iter;
        *ppDeviceCtx = (*iter)->pDeviceCtx;

        // remove the device
        *iter = (*iter)->next;
        free(tmp);
    }

    dwStatus = OsEventSignal(DevList.hEvent);
    if (dwStatus)
    {
        ERR("RemoveDeviceFromDevList: error signaling device list event: dwStatus (0x%x) - %s\n", 
            dwStatus, Stat2Str(dwStatus));
        return dwStatus;
    }

    if (!Found)
        dwStatus = WD_DEVICE_NOT_FOUND;
    return dwStatus;
}

