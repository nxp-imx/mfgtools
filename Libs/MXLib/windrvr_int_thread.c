////////////////////////////////////////////////////////////////
// File - windrvr_int_thread.c
//
// Implementation of a thread that waits for WinDriver events.
//
// Copyright (c) 2003 Jungo Ltd.  http://www.jungo.com 
////////////////////////////////////////////////////////////////

#ifdef __KERNEL__
#include "kpstdlib.h"
#else
#include "utils.h"
#endif

#include "windrvr_int_thread.h"

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

// backward compatibility
BOOL InterruptThreadEnable(HANDLE *phThread, HANDLE hWD,
    WD_INTERRUPT *pInt, INT_HANDLER_FUNC func, PVOID pData)
{
    DWORD rc;
    rc = InterruptEnable(phThread, hWD, pInt, func, pData);
    return rc==WD_STATUS_SUCCESS;
}

void InterruptThreadDisable(HANDLE hThread)
{
    InterruptDisable(hThread);
}

#if defined(__KERNEL__)

    typedef struct
    {
        HANDLE hWD;
        WD_INTERRUPT *pInt;
        INT_HANDLER_FUNC func;
        PVOID pData;
    } INT_THREAD_DATA;

    static void __cdecl InterruptHandler(PVOID pContext)
    {
        INT_THREAD_DATA *pThread = (INT_THREAD_DATA *)pContext;

        WD_IntCount(pThread->hWD, pThread->pInt);
        pThread->func(pThread->pData);
    }

    DWORD InterruptEnable(HANDLE *phThread, HANDLE hWD,
        WD_INTERRUPT *pInt, INT_HANDLER_FUNC func, PVOID pData)
    {
        INT_THREAD_DATA *pThread;
        DWORD dwStatus;
        *phThread = NULL;

        pThread = (INT_THREAD_DATA *)malloc(sizeof(INT_THREAD_DATA));
        if (!pThread)
            return WD_INSUFFICIENT_RESOURCES;

        pInt->kpCall.hKernelPlugIn = WD_KERNEL_DRIVER_PLUGIN_HANDLE;
        pInt->kpCall.dwMessage = (DWORD)InterruptHandler;
        pInt->kpCall.pData = pThread;
        dwStatus = WD_IntEnable(hWD, pInt);
        // check if WD_IntEnable failed
        if (dwStatus)
        {
            free(pThread);
            return dwStatus;
        }

        pThread->pInt = pInt;
        pThread->hWD = hWD;
        pThread->func = func;
        pThread->pData = pData;

        *phThread = (HANDLE)pThread;
        return WD_STATUS_SUCCESS;
    }

    DWORD InterruptDisable(HANDLE hThread)
    {
        DWORD dwStatus;
        INT_THREAD_DATA *pThread = (INT_THREAD_DATA *)hThread;

        if (!pThread)
            return WD_INVALID_HANDLE;
        dwStatus = WD_IntDisable(pThread->hWD, pThread->pInt);
        free(pThread);
        return dwStatus;
    }

#else

    typedef struct
    {
        HANDLE hWD;
        HANDLER_FUNC func;
        PVOID pData;
        WD_INTERRUPT *pInt;
        void *thread;
    } INT_THREAD_DATA;

    static void interrupt_thread_handler(void *data)
    {
        INT_THREAD_DATA *pThread = (INT_THREAD_DATA *)data;
        for (;;)
        {
            WD_IntWait (pThread->hWD, pThread->pInt);
            if (pThread->pInt->fStopped==INTERRUPT_STOPPED)
                break;
            if (pThread->pInt->fStopped==INTERRUPT_INTERRUPTED)
                continue;
            pThread->func(pThread->pData);
        }
    }

    DWORD DLLCALLCONV InterruptEnable(HANDLE *phThread, HANDLE hWD, WD_INTERRUPT *pInt,
        HANDLER_FUNC func, PVOID pData)
    {
        DWORD dwStatus;
        INT_THREAD_DATA *pThread;
        *phThread = NULL;

        pThread = (INT_THREAD_DATA *)malloc(sizeof(INT_THREAD_DATA));
        if (!pThread)
            return WD_INSUFFICIENT_RESOURCES;

        dwStatus = WD_IntEnable(hWD, pInt);
        // check if WD_IntEnable failed
        if (dwStatus)
        {
            free(pThread);
            return dwStatus;
        }

        BZERO(*pThread);
        pThread->func = func;
        pThread->pData = pData;
        pThread->hWD = hWD;
        pThread->pInt = pInt;

        dwStatus = ThreadStart(&pThread->thread, interrupt_thread_handler, (void *)pThread);
        if (dwStatus)
        {
            WD_IntDisable(hWD, pInt);
            free(pThread);
            return dwStatus;
        }
        *phThread = (HANDLE) pThread;
        return WD_STATUS_SUCCESS;
    }

    DWORD DLLCALLCONV InterruptDisable(HANDLE hThread)
    {
        INT_THREAD_DATA *pThread = (INT_THREAD_DATA *)hThread;
        DWORD dwStatus;
        if (!pThread)
            return WD_INVALID_HANDLE;

        dwStatus = WD_IntDisable(pThread->hWD, pThread->pInt);
        ThreadStop(pThread->thread);
        free(pThread);
        return dwStatus;
    }
#endif

#ifdef __cplusplus
}
#endif  // __cplusplus

