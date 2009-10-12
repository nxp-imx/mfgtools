////////////////////////////////////////////////////////////////
// File - windrvr_events.c
//
// API for receiving events from WinDriver (PCI, USB)
//
// Copyright (c) 2003 Jungo Ltd.  http://www.jungo.com 
////////////////////////////////////////////////////////////////

#ifdef __KERNEL__
    #include "kpstdlib.h"
#else
   #include "windrvr_int_thread.h"
   #include "utils.h"
#endif
#include "windrvr_events.h"

// backward compatibility
struct event_handle_t *event_register(HANDLE hWD, WD_EVENT_V60 *event,
    EVENT_HANDLER func, void *data)
{
    HANDLE h;
    EventRegister(&h, hWD, event, func, data);
    return (struct event_handle_t *)h;
}

void event_unregister(HANDLE hWD, struct event_handle_t *handle)
{
    EventUnregister((HANDLE)handle);
}

WD_EVENT * DLLCALLCONV EventAlloc(DWORD dwNumMatchTables)
{
    DWORD dwSize;
    DWORD dwNeedAllocate;
    WD_EVENT *peNew;

    dwNeedAllocate = dwNumMatchTables > 0 ? dwNumMatchTables-1 : 0;

    dwSize = sizeof(WD_EVENT) + sizeof(WDU_MATCH_TABLE) * dwNeedAllocate;
    peNew = (WD_EVENT *)malloc(dwSize);
    if (!peNew)
        return NULL;

    memset(peNew, 0, dwSize);
    peNew->dwNumMatchTables = dwNumMatchTables;
    peNew->dwEventVer = 1;
    return peNew;
}

WD_EVENT * DLLCALLCONV PciEventCreate(WD_PCI_ID cardId, WD_PCI_SLOT pciSlot,
    DWORD dwOptions, DWORD dwAction)
{
    WD_EVENT *peNew;
    peNew = EventAlloc(1);
    if (!peNew)
        return NULL;

    peNew->dwCardType = WD_BUS_PCI;
    peNew->dwAction = dwAction;
    peNew->dwOptions = dwOptions;
    peNew->u.Pci.cardId = cardId;
    peNew->u.Pci.pciSlot = pciSlot;
    return peNew;
}

WD_EVENT * DLLCALLCONV UsbEventCreate(WDU_MATCH_TABLE *pMatchTables, 
    DWORD dwNumMatchTables, DWORD dwOptions, DWORD dwAction)
{
    WD_EVENT *peNew;


    if ((pMatchTables && !dwNumMatchTables)||
        (!pMatchTables && dwNumMatchTables))
    {
        return NULL;
    }

    peNew = EventAlloc(dwNumMatchTables);
    if (!peNew)
        return NULL;

    peNew->dwCardType = (DWORD)WD_BUS_USB;
    peNew->dwAction = dwAction;
    peNew->dwOptions = dwOptions;

    if (dwNumMatchTables)
    {
        memcpy(peNew->matchTables, pMatchTables, sizeof(WDU_MATCH_TABLE) * 
            dwNumMatchTables);
        
    }
    return peNew;
}

void DLLCALLCONV EventFree(WD_EVENT *pe)
{
    free(pe);
}

WD_EVENT * DLLCALLCONV EventDup(WD_EVENT *peSrc)
{
    WD_EVENT *peNew;
    DWORD dwNeedAllocate, dwSize;

    dwNeedAllocate = peSrc->dwNumMatchTables > 0 ? peSrc->dwNumMatchTables-1 : 0;
    dwSize = sizeof(WD_EVENT) + sizeof(WDU_MATCH_TABLE) * dwNeedAllocate;

    peNew = EventAlloc(peSrc->dwNumMatchTables);
    if (peNew)
        memcpy(peNew, peSrc, dwSize);
    return peNew;
}

#if defined(__KERNEL__)

DWORD EventRegister(HANDLE *phEvent, HANDLE hWD, WD_EVENT_V60 *pEvent,
    EVENT_HANDLER pFunc, void *pData)
{
    *phEvent = NULL;
    return WD_NOT_IMPLEMENTED;
}

DWORD EventUnregister(HANDLE hEvent)
{
    return WD_NOT_IMPLEMENTED;
}

#else

typedef struct
{
    EVENT_HANDLER func;
    HANDLE hWD;
    void *data;
    HANDLE thread;
    WD_INTERRUPT Int;
} event_handle_t;

static void event_handler(void *h)
{
    event_handle_t *handle = (event_handle_t *)h;
    WD_EVENT_V60 *pePulled;

    pePulled = EventAlloc(1);
    pePulled->handle = handle->Int.hInterrupt;
    WD_EventPull(handle->hWD, pePulled);
    if (pePulled->dwAction)
        handle->func(pePulled, handle->data);
    if (pePulled->dwOptions & WD_ACKNOWLEDGE)
        WD_EventSend(handle->hWD, pePulled);
    EventFree(pePulled);
}

DWORD DLLCALLCONV EventRegister(HANDLE *phEvent, HANDLE hWD,
    WD_EVENT_V60 *pEvent, EVENT_HANDLER pFunc, void *pData)
{
    DWORD dwStatus;
    event_handle_t *handle = NULL;
    *phEvent = NULL;

    handle = (event_handle_t *)malloc(sizeof(*handle));
    if (!handle)
    {
        dwStatus = WD_INSUFFICIENT_RESOURCES;
        goto Error;
    }
    BZERO(*handle);
    handle->func = pFunc;
    handle->data = pData;
    handle->hWD = hWD;

    dwStatus = WD_EventRegister(hWD, pEvent);
    if (dwStatus)
        goto Error;
    handle->Int.hInterrupt = pEvent->handle;
    dwStatus = InterruptEnable(&handle->thread, hWD, &handle->Int,
        event_handler, (PVOID)handle);
    if (dwStatus)
        goto Error;
    *phEvent = (HANDLE)handle;
    return dwStatus;

Error:
    if (handle && handle->Int.hInterrupt)
        WD_EventUnregister(hWD, pEvent);
    if (handle)
        free(handle);
    return dwStatus;
}

DWORD DLLCALLCONV EventUnregister(HANDLE hEvent)
{
    event_handle_t *handle = hEvent;
    WD_EVENT_V60 *pe;
    DWORD dwStatus;

    if (!handle)
        return WD_INVALID_HANDLE;
    pe = EventAlloc(1);
    pe->handle = handle->Int.hInterrupt;
    InterruptDisable(handle->thread);
    dwStatus = WD_EventUnregister(handle->hWD, pe);
    EventFree(pe);
    free(handle);
    return dwStatus;
}
#endif

