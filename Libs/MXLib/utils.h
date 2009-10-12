#ifndef _WD_UTILS_H_
#define _WD_UTILS_H_

#include "windrvr.h"

#if defined(__cplusplus)
extern "C" {
#endif

typedef void (*HANDLER_FUNC)(void *pData);

DWORD DLLCALLCONV ThreadStart(HANDLE *phThread, HANDLER_FUNC pFunc, void *pData);
void DLLCALLCONV ThreadStop(HANDLE hThread);

DWORD DLLCALLCONV OsEventCreate(HANDLE *phOsEvent);
void DLLCALLCONV OsEventClose(HANDLE hOsEvent);
DWORD DLLCALLCONV OsEventWait(HANDLE hOsEvent, DWORD dwSecTimeout);
DWORD DLLCALLCONV OsEventSignal(HANDLE hOsEvent);
void DLLCALLCONV PrintDbgMessage(DWORD dwLevel, DWORD dwSection, 
    const char *format, ...);
#ifdef __cplusplus
}
#endif

#endif
