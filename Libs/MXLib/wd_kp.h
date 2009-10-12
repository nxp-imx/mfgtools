#ifndef _WD_KP_H_
#define _WD_KP_H_

#ifndef __KERNEL__
    #define __KERNEL__
#endif

#ifndef __KERPLUG__
    #define __KERPLUG__
#endif

#include "windrvr.h"

#ifdef __cplusplus
    extern "C" {
#endif  // __cplusplus 

// called when WD_KernelPlugInClose() is called
typedef void (__cdecl *KP_FUNC_CLOSE)(PVOID pDrvContext);
// called when WD_KernelPlugInCall() is called
typedef void (__cdecl *KP_FUNC_CALL)(PVOID pDrvContext, WD_KERNEL_PLUGIN_CALL *kpCall, BOOL fKernelMode);
// called when WD_IntEnable() is called, with a kernel plugin handler specified
// the pIntContext will be passed to the rest of the functions handling interrupts.
// returns TRUE if enable is successful
typedef BOOL (__cdecl *KP_FUNC_INT_ENABLE)(PVOID pDrvContext, WD_KERNEL_PLUGIN_CALL *kpCall, PVOID *ppIntContext);
// called when WD_IntDisable() is called
typedef void (__cdecl *KP_FUNC_INT_DISABLE)(PVOID pIntContext);
// returns TRUE if needs DPC.
typedef BOOL (__cdecl *KP_FUNC_INT_AT_IRQL)(PVOID pIntContext, BOOL *pfIsMyInterrupt);
// returns the number of times to notify user-mode (i.e. return from WD_IntWait)
typedef DWORD (__cdecl *KP_FUNC_INT_AT_DPC)(PVOID pIntContext, DWORD dwCount);
// returns TRUE if user need notification
typedef BOOL (__cdecl *KP_FUNC_EVENT)(PVOID pDrvContext, WD_EVENT *wd_event);

typedef struct {
    KP_FUNC_CLOSE       funcClose;
    KP_FUNC_CALL        funcCall;
    KP_FUNC_INT_ENABLE  funcIntEnable;
    KP_FUNC_INT_DISABLE funcIntDisable;
    KP_FUNC_INT_AT_IRQL funcIntAtIrql;
    KP_FUNC_INT_AT_DPC  funcIntAtDpc;
    KP_FUNC_EVENT       funcEvent;
} KP_OPEN_CALL;

// called when WD_KernelPlugInOpen() is called. pDrvContext returned will be passed to 
// rest of the functions
typedef BOOL (__cdecl *KP_FUNC_OPEN)(KP_OPEN_CALL *kpOpenCall, HANDLE hWD, PVOID pOpenData, PVOID *ppDrvContext);

typedef struct {
    DWORD        dwVerWD;        // version of WinDriver library WD_KP.LIB
    CHAR         cDriverName[12]; // return the device driver name, up to 12 chars.
    KP_FUNC_OPEN funcOpen;       // returns the KP_Open function
} KP_INIT;

// You must define KP_Init() functions in order to link the device driver
BOOL __cdecl KP_Init(KP_INIT *kpInit);

#ifdef __cplusplus
    }
#endif  // __cplusplus 

#endif // _WD_KP_H_
